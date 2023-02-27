/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sensor_basic_data_channel.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "hisysevent.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "SensorBasicChannel" };
constexpr int32_t SENSOR_READ_DATA_SIZE = sizeof(SensorEvent) * 100;
constexpr int32_t DEFAULT_CHANNEL_SIZE = 2 * 1024;
constexpr int32_t SOCKET_PAIR_SIZE = 2;
}  // namespace

SensorBasicDataChannel::SensorBasicDataChannel() : sendFd_(-1), receiveFd_(-1), isActive_(false)
{
    SEN_HILOGD("isActive_:%{public}d,sendFd:%{public}d", isActive_, sendFd_);
}

int32_t SensorBasicDataChannel::CreateSensorBasicChannel()
{
    if ((sendFd_ != -1) || (receiveFd_ != -1)) {
        SEN_HILOGD("already create socketpair");
        return ERR_OK;
    }

    int32_t socketPair[SOCKET_PAIR_SIZE] = { 0 };
    if (socketpair(AF_UNIX, SOCK_SEQPACKET, 0, socketPair) != 0) {
        HiSysEvent::Write(HiviewDFX::HiSysEvent::Domain::SENSOR, "SENSOR_DATA_CHANNEL_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "CreateSensorBasicChannel", "ERROR_CODE", errno);
        SEN_HILOGE("create socketpair failed");
        sendFd_ = -1;
        receiveFd_ = -1;
        return SENSOR_CHANNEL_SOCKET_CREATE_ERR;
    }
    // set socket attr
    int32_t ret = setsockopt(socketPair[0], SOL_SOCKET, SO_SNDBUF, &SENSOR_READ_DATA_SIZE,
                             sizeof(SENSOR_READ_DATA_SIZE));
    if (ret != 0) {
        SEN_HILOGE("setsockopt socketpair 0 failed");
        return SENSOR_CHANNEL_SOCKET_CREATE_ERR;
    }
    ret = setsockopt(socketPair[1], SOL_SOCKET, SO_RCVBUF, &SENSOR_READ_DATA_SIZE, sizeof(SENSOR_READ_DATA_SIZE));
    if (ret != 0) {
        SEN_HILOGE("setsockopt socketpair 1 failed");
        return SENSOR_CHANNEL_SOCKET_CREATE_ERR;
    }
    int32_t bufferSize = DEFAULT_CHANNEL_SIZE;
    ret = setsockopt(socketPair[0], SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize));
    if (ret != 0) {
        SEN_HILOGE("setsockopt socketpair 0 failed");
        return SENSOR_CHANNEL_SOCKET_CREATE_ERR;
    }
    ret = setsockopt(socketPair[1], SOL_SOCKET, SO_SNDBUF, &bufferSize, sizeof(bufferSize));
    if (ret != 0) {
        SEN_HILOGE("setsockopt socketpair 1 failed");
        return SENSOR_CHANNEL_SOCKET_CREATE_ERR;
    }
    ret = fcntl(socketPair[0], F_SETFL, O_NONBLOCK);
    if (ret != 0) {
        SEN_HILOGE("fcntl socketpair 0 failed");
        return SENSOR_CHANNEL_SOCKET_CREATE_ERR;
    }
    ret = fcntl(socketPair[1], F_SETFL, O_NONBLOCK);
    if (ret != 0) {
        SEN_HILOGE("fcntl socketpair 1 failed");
        return SENSOR_CHANNEL_SOCKET_CREATE_ERR;
    }
    sendFd_ = socketPair[0];
    receiveFd_ = socketPair[1];
    SEN_HILOGD("create socketpair success,receiveFd_:%{public}d,sendFd_:%{public}d", receiveFd_, sendFd_);
    return ERR_OK;
}

int32_t SensorBasicDataChannel::CreateSensorBasicChannel(MessageParcel &data)
{
    CALL_LOG_ENTER;
    if ((sendFd_ != -1) || (receiveFd_ != -1)) {
        SEN_HILOGD("already create socketpair");
        return ERR_OK;
    }
    sendFd_ = data.ReadFileDescriptor();
    if (sendFd_ < 0) {
        SEN_HILOGE("ReadFileDescriptor is failed");
        sendFd_ = -1;
        return SENSOR_CHANNEL_READ_DESCRIPTOR_ERR;
    }
    return ERR_OK;
}

SensorBasicDataChannel::~SensorBasicDataChannel()
{
    DestroySensorBasicChannel();
}

int32_t SensorBasicDataChannel::SendToBinder(MessageParcel &data)
{
    SEN_HILOGD("sendFd:%{public}d", sendFd_);
    if (sendFd_ < 0) {
        SEN_HILOGE("sendFd FileDescriptor error");
        return SENSOR_CHANNEL_SENDFD_ERR;
    }
    bool result = data.WriteFileDescriptor(sendFd_);
    if (!result) {
        SEN_HILOGE("send sendFd_ failed");
        CloseSendFd();
        return SENSOR_CHANNEL_WRITE_DESCRIPTOR_ERR;
    }
    return ERR_OK;
}

void SensorBasicDataChannel::CloseSendFd()
{
    if (sendFd_ != -1) {
        close(sendFd_);
        sendFd_ = -1;
        SEN_HILOGD("close sendFd_");
    }
}

int32_t SensorBasicDataChannel::SendData(const void *vaddr, size_t size)
{
    CHKPR(vaddr, SENSOR_CHANNEL_SEND_ADDR_ERR);
    if (sendFd_ < 0) {
        SEN_HILOGE("failed, param is invalid");
        return SENSOR_CHANNEL_SEND_ADDR_ERR;
    }
    ssize_t length;
    do {
        length = send(sendFd_, vaddr, size, MSG_DONTWAIT | MSG_NOSIGNAL);
    } while (errno == EINTR);
    if (length < 0) {
        HiSysEvent::Write(HiviewDFX::HiSysEvent::Domain::SENSOR, "SENSOR_DATA_CHANNEL_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "SendData", "ERROR_CODE", errno);
        SEN_HILOGE("send fail:%{public}d,length:%{public}d", errno, (int32_t)length);
        return SENSOR_CHANNEL_SEND_DATA_ERR;
    }
    return ERR_OK;
}

int32_t SensorBasicDataChannel::ReceiveData(void *vaddr, size_t size)
{
    CHKPR(vaddr, SENSOR_CHANNEL_SEND_ADDR_ERR);
        SEN_HILOGE("failed, vaddr is null or receiveFd_ invalid");
        return SENSOR_CHANNEL_RECEIVE_ADDR_ERR;
    ssize_t length;
    do {
        length = recv(receiveFd_, vaddr, size, MSG_DONTWAIT);
    } while (errno == EINTR);
    return length;
}

int32_t SensorBasicDataChannel::GetSendDataFd() const
{
    return sendFd_;
}

int32_t SensorBasicDataChannel::GetReceiveDataFd() const
{
    return receiveFd_;
}

int32_t SensorBasicDataChannel::DestroySensorBasicChannel()
{
    if (sendFd_ >= 0) {
        close(sendFd_);
        sendFd_ = -1;
        SEN_HILOGD("close sendFd_ success");
    }
    if (receiveFd_ >= 0) {
        close(receiveFd_);
        receiveFd_ = -1;
        SEN_HILOGD("close receiveFd_ success");
    }
    return ERR_OK;
}

const std::unordered_map<uint32_t, SensorEvent> &SensorBasicDataChannel::GetDataCacheBuf() const
{
    return dataCacheBuf_;
}

bool SensorBasicDataChannel::GetSensorStatus() const
{
    return isActive_;
}

void SensorBasicDataChannel::SetSensorStatus(bool isActive)
{
    SEN_HILOGD("isActive_:%{public}d", isActive);
    std::unique_lock<std::mutex> lock(statusLock_);
    isActive_ = isActive;
    return;
}
}  // namespace Sensors
}  // namespace OHOS
