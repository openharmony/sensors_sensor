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

#include "stream_session.h"

#include <cinttypes>
#include <sstream>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "proto.h"
#include "sensors_errors.h"
#include "stream_socket.h"

namespace OHOS {
namespace Sensors {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "StreamSession" };
}

StreamSession::StreamSession(const std::string &programName, const int32_t fd, const int32_t uid, const int32_t pid)
    : programName_(programName),
      fd_(fd),
      uid_(uid),
      pid_(pid)
{
    UpdateDescript();
}

bool StreamSession::SendMsg(const char *buf, size_t size) const
{
    CHKPF(buf);
    if ((size == 0) || (size > MAX_PACKET_BUF_SIZE)) {
        SEN_HILOGE("Buf size:%{public}zu", size);
        return false;
    }
    if (fd_ < 0) {
        SEN_HILOGE("The fd_ is less than 0");
        return false;
    }
    size_t idx = 0;
    size_t retryCount = 0;
    size_t remSize = size;
    while (remSize > 0 && retryCount < SEND_RETRY_LIMIT) {
        ++retryCount;
        auto count = send(fd_, &buf[idx], remSize, MSG_DONTWAIT | MSG_NOSIGNAL);
        if (count < 0) {
            if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK) {
                usleep(SEND_RETRY_SLEEP_TIME);
                SEN_HILOGW("Continue for errno EAGAIN|EINTR|EWOULDBLOCK, errno:%{public}d", errno);
                continue;
            }
            SEN_HILOGE("Send return failed,error:%{public}d fd:%{public}d", errno, fd_);
            return false;
        }
        idx += static_cast<size_t>(count);
        remSize -= static_cast<size_t>(count);
        if (remSize > 0) {
            usleep(SEND_RETRY_SLEEP_TIME);
        }
    }
    if (retryCount >= SEND_RETRY_LIMIT || remSize != 0) {
        SEN_HILOGE("Send too many times:%{public}zu/%{public}zu, size:%{public}zu/%{public}zu, fd:%{public}d",
            retryCount, SEND_RETRY_LIMIT, idx, size, fd_);
        return false;
    }
    return true;
}

void StreamSession::Close()
{
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
        UpdateDescript();
    }
}

void StreamSession::UpdateDescript()
{
    std::ostringstream oss;
    oss << "fd = " << fd_
        << ", programName = " << programName_
        << ((fd_ < 0) ? ", closed" : ", opened")
        << ", uid = " << uid_
        << ", pid = " << pid_
        << ", tokenType = " << tokenType_
        << std::endl;
    descript_ = oss.str().c_str();
}

bool StreamSession::SendMsg(const NetPacket &pkt) const
{
    if (pkt.ChkRWError()) {
        SEN_HILOGE("Read and write status failed");
        return false;
    }
    StreamBuffer buf;
    pkt.MakeData(buf);
    return SendMsg(buf.Data(), buf.Size());
}

int32_t StreamSession::GetUid() const
{
    return uid_;
}

int32_t StreamSession::GetPid() const
{
    return pid_;
}

SessionPtr StreamSession::GetSharedPtr()
{
    return shared_from_this();
}

int32_t StreamSession::GetFd() const
{
    return fd_;
}

const std::string& StreamSession::GetDescript() const
{
    return descript_;
}

const std::string StreamSession::GetProgramName() const
{
    return programName_;
}

void StreamSession::SetTokenType(int32_t type)
{
    tokenType_ = type;
}

int32_t StreamSession::GetTokenType() const
{
    return tokenType_;
}
}  // namespace Sensors
}  // namespace OHOS