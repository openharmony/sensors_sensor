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

#include "dmd_report.h"
#include "sensors_errors.h"
#include "sensors_log_domain.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SensorsLogDomain::SENSOR_UTILS, "SensorBasicChannel" };
constexpr int32_t SENSOR_READ_DATA_SIZE = sizeof(struct SensorEvent) * 100;
constexpr int32_t DEFAULT_CHANNEL_SIZE = 2 * 1024;
constexpr int32_t SOCKET_PAIR_SIZE = 2;
}  // namespace

SensorBasicDataChannel::SensorBasicDataChannel() : sendFd_(-1), receiveFd_(-1), isActive_(false)
{
    HiLog::Debug(LABEL, "%{public}s isActive_ : %{public}d, sendFd: %{public}d", __func__, isActive_, sendFd_);
}

int32_t SensorBasicDataChannel::CreateSensorBasicChannel()
{
    if ((sendFd_ != -1) || (receiveFd_ != -1)) {
        HiLog::Debug(LABEL, "%{public}s already create socketpair", __func__);
        return ERR_OK;
    }

    int32_t socketPair[SOCKET_PAIR_SIZE] = { 0 };
    if (socketpair(AF_UNIX, SOCK_SEQPACKET, 0, socketPair) != 0) {
        DmdReport::ReportException(SENSOR_DATA_CHANNEL_EXCEPTION, "CreateSensorBasicChannel",
                                   SENSOR_CHANNEL_SOCKET_CREATE_ERR);
        HiLog::Error(LABEL, "%{public}s create socketpair failed", __func__);
        sendFd_ = -1;
        receiveFd_ = -1;
        return SENSOR_CHANNEL_SOCKET_CREATE_ERR;
    }
    // set socket attr
    int32_t ret = setsockopt(socketPair[0], SOL_SOCKET, SO_SNDBUF, &SENSOR_READ_DATA_SIZE, sizeof(SENSOR_READ_DATA_SIZE));
    if (ret != 0) {
        HiLog::Error(LABEL, "%{public}s setsockopt socketpair 0 failed", __func__);
        return SENSOR_CHANNEL_SOCKET_CREATE_ERR;
    }
    ret = setsockopt(socketPair[1], SOL_SOCKET, SO_RCVBUF, &SENSOR_READ_DATA_SIZE, sizeof(SENSOR_READ_DATA_SIZE));
    if (ret != 0) {
        HiLog::Error(LABEL, "%{public}s setsockopt socketpair 1 failed", __func__);
        return SENSOR_CHANNEL_SOCKET_CREATE_ERR;
    }
    int32_t bufferSize = DEFAULT_CHANNEL_SIZE;
    ret = setsockopt(socketPair[0], SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize));
    if (ret != 0) {
        HiLog::Error(LABEL, "%{public}s setsockopt socketpair 0 failed", __func__);
        return SENSOR_CHANNEL_SOCKET_CREATE_ERR;
    }
    ret = setsockopt(socketPair[1], SOL_SOCKET, SO_SNDBUF, &bufferSize, sizeof(bufferSize));
    if (ret != 0) {
        HiLog::Error(LABEL, "%{public}s setsockopt socketpair 1 failed", __func__);
        return SENSOR_CHANNEL_SOCKET_CREATE_ERR;
    }
    ret = fcntl(socketPair[0], F_SETFL, O_NONBLOCK);
    if (ret != 0) {
        HiLog::Error(LABEL, "%{public}s fcntl socketpair 0 failed", __func__);
        return SENSOR_CHANNEL_SOCKET_CREATE_ERR;
    }
    ret = fcntl(socketPair[1], F_SETFL, O_NONBLOCK);
    if (ret != 0) {
        HiLog::Error(LABEL, "%{public}s fcntl socketpair 1 failed", __func__);
        return SENSOR_CHANNEL_SOCKET_CREATE_ERR;
    }
    sendFd_ = socketPair[0];
    receiveFd_ = socketPair[1];
    HiLog::Debug(LABEL, "%{public}s create socketpair success, receiveFd_ : %{public}d, sendFd_ : %{public}d", __func__,
                 receiveFd_, sendFd_);
    return ERR_OK;
}

int32_t SensorBasicDataChannel::CreateSensorBasicChannel(MessageParcel &data)
{
    CALL_LOG_ENTER;
    if ((sendFd_ != -1) || (receiveFd_ != -1)) {
        HiLog::Debug(LABEL, "%{public}s already create socketpair", __func__);
        return ERR_OK;
    }
    int32_t tmpFd = data.ReadFileDescriptor();
    if (tmpFd < 0) {
        HiLog::Error(LABEL, "%{public}s ReadFileDescriptor failed", __func__);
        sendFd_ = -1;
        return SENSOR_CHANNEL_DUP_ERR;
    }
    sendFd_ = dup(tmpFd);
    if (sendFd_ < 0) {
        HiLog::Error(LABEL, "%{public}s dup FileDescriptor failed", __func__);
        sendFd_ = -1;
        return SENSOR_CHANNEL_DUP_ERR;
    }
    return ERR_OK;
}

SensorBasicDataChannel::~SensorBasicDataChannel()
{
    DestroySensorBasicChannel();
}

int32_t SensorBasicDataChannel::SendToBinder(MessageParcel &data)
{
    HiLog::Debug(LABEL, "%{public}s sendFd: %{public}d", __func__, sendFd_);
    if (sendFd_ < 0) {
        HiLog::Error(LABEL, "%{public}s sendFd FileDescriptor error", __func__);
        return SENSOR_CHANNEL_SENDFD_ERR;
    }
    bool result = data.WriteFileDescriptor(sendFd_);
    if (!result) {
        HiLog::Error(LABEL, "%{public}s send sendFd_ failed", __func__);
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
        HiLog::Debug(LABEL, "%{public}s close sendFd_", __func__);
    }
}

int32_t SensorBasicDataChannel::SendData(const void *vaddr, size_t size)
{
    if (vaddr == nullptr || sendFd_ < 0) {
        HiLog::Error(LABEL, "%{public}s failed, param is invalid", __func__);
        return SENSOR_CHANNEL_SEND_ADDR_ERR;
    }
    ssize_t length;
    do {
        length = send(sendFd_, vaddr, size, MSG_DONTWAIT | MSG_NOSIGNAL);
    } while (errno == EINTR);
    if (length < 0) {
        HiLog::Error(LABEL, "%{public}s send fail : %{public}d, length = %{public}d", __func__, errno, (int32_t)length);
        return SENSOR_CHANNEL_SEND_DATA_ERR;
    }
    return ERR_OK;
}

int32_t SensorBasicDataChannel::ReceiveData(void *vaddr, size_t size)
{
    if (vaddr == nullptr || (receiveFd_ == -1)) {
        HiLog::Error(LABEL, "%{public}s failed, vaddr is null or receiveFd_ invalid", __func__);
        return SENSOR_CHANNEL_RECEIVE_ADDR_ERR;
    }
    ssize_t length;
    do {
        length = recv(receiveFd_, vaddr, size, MSG_DONTWAIT);
    } while (errno == EINTR);
    if (length < 0) {
        return 0;
    }
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
        HiLog::Debug(LABEL, "%{public}s close sendFd_ success", __func__);
    }
    if (receiveFd_ >= 0) {
        close(receiveFd_);
        receiveFd_ = -1;
        HiLog::Debug(LABEL, "%{public}s close receiveFd_ success", __func__);
    }
    return ERR_OK;
}

const std::unordered_map<uint32_t, struct SensorEvent> &SensorBasicDataChannel::GetDataCacheBuf() const
{
    return dataCacheBuf_;
}

bool SensorBasicDataChannel::GetSensorStatus() const
{
    return isActive_;
}

void SensorBasicDataChannel::SetSensorStatus(bool isActive)
{
    HiLog::Debug(LABEL, "%{public}s isActive_ : %{public}d", __func__, isActive);
    std::unique_lock<std::mutex> lock(statusLock_);
    isActive_ = isActive;
    return;
}
}  // namespace Sensors
}  // namespace OHOS
