/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "stream_client.h"

#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "StreamClient" };
}

StreamClient::StreamClient() {}

StreamClient::~StreamClient() {}

int32_t StreamClient::ConnectTo()
{
    CALL_LOG_ENTER;
    if (Socket() < 0) {
        SEN_HILOGE("Socket failed");
        return ERROR;
    }
    OnConnected();
    return SUCCESS;
}

bool StreamClient::SendMsg(const char *buf, size_t size) const
{
    CALL_LOG_ENTER;
    CHKPF(buf);
    if ((size == 0) || (size > PROTO_MAX_PACKET_BUF_SIZE)) {
        SEN_HILOGE("Stream buffer size out of range");
        return false;
    }
    if (fd_ < 0) {
        SEN_HILOGE("The fd_ is less than 0");
        return false;
    }
    int32_t idx = 0;
    int32_t retryCount = 0;
    const int32_t bufSize = static_cast<int32_t>(size);
    int32_t remSize = bufSize;
    while (remSize > 0 && retryCount < PROTO_SEND_RETRY_LIMIT) {
        retryCount += 1;
        auto count = send(fd_, &buf[idx], remSize, MSG_DONTWAIT | MSG_NOSIGNAL);
        if (count < 0) {
            if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK) {
                SEN_HILOGW("Continue for errno EAGAIN|EINTR|EWOULDBLOCK, errno:%{public}d", errno);
                continue;
            }
            SEN_HILOGE("Send return failed,error:%{public}d fd:%{public}d", errno, fd_);
            return false;
        }
        idx += count;
        remSize -= count;
        if (remSize > 0) {
            usleep(PROTO_SEND_RETRY_SLEEP_TIME);
        }
    }
    if (retryCount >= PROTO_SEND_RETRY_LIMIT || remSize != 0) {
        SEN_HILOGE("Send too many times:%{public}d/%{public}d,size:%{public}d/%{public}d fd:%{public}d",
            retryCount, PROTO_SEND_RETRY_LIMIT, idx, bufSize, fd_);
        return false;
    }
    return true;
}

bool StreamClient::SendMsg(const NetPacket &pkt) const
{
    CALL_LOG_ENTER;
    if (pkt.ChkRWError()) {
        SEN_HILOGE("Read and write status is error");
        return false;
    }
    StreamBuffer buf;
    pkt.MakeData(buf);
    return SendMsg(buf.Data(), buf.Size());
}

bool StreamClient::StartClient(MsgClientFunCallback fun)
{
    CALL_LOG_ENTER;
    if (isRunning_ || isConnected_) {
        SEN_HILOGE("Client is connected or started");
        return false;
    }
    isExit = false;
    recvFun_ = fun;
    if (ConnectTo() < 0) {
        SEN_HILOGW("Client connection failed, Try again later");
    }
    return true;
}

void StreamClient::Stop()
{
    CALL_LOG_ENTER;
    isExit = true;
    isRunning_ = false;
    Close();
}

bool StreamClient::GetConnectedStatus() const
{
    return isConnected_;
}
} // namespace Sensors
} // namespace OHOS