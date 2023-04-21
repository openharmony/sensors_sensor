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
    : programName_(programName)
#ifdef OHOS_BUILD_ENABLE_RUST
{
    rustStreamSession_.fd_ = fd;
    rustStreamSession_.uid_ = uid;
    rustStreamSession_.pid_ = pid;
    UpdateDescript();
}
#else
,
      fd_(fd),
      uid_(uid),
      pid_(pid)
{
    UpdateDescript();
}
#endif // OHOS_BUILD_ENABLE_RUST


bool StreamSession::SendMsg(const char *buf, size_t size) const
{
#ifdef OHOS_BUILD_ENABLE_RUST
    return session_send_msg(&rustStreamSession_, buf, size);
#else
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
#endif // OHOS_BUILD_ENABLE_RUST
}

void StreamSession::Close()
{
#ifdef OHOS_BUILD_ENABLE_RUST
    session_close(&rustStreamSession_);
    UpdateDescript();
#else
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
        UpdateDescript();
    }
#endif // OHOS_BUILD_ENABLE_RUST
}

void StreamSession::UpdateDescript()
{
#ifdef OHOS_BUILD_ENABLE_RUST
    std::ostringstream oss;
    oss << "fd = " << rustStreamSession_.fd_
        << ", programName = " << programName_
        << ", moduleType = " << rustStreamSession_.moduleType_
        << ((rustStreamSession_.fd_ < 0) ? ", closed" : ", opened")
        << ", uid = " << rustStreamSession_.uid_
        << ", pid = " << rustStreamSession_.pid_
        << ", tokenType = " << rustStreamSession_.tokenType_
        << std::endl;
    descript_ = oss.str().c_str();
#else
    std::ostringstream oss;
    oss << "fd = " << fd_
        << ", programName = " << programName_
        << ((fd_ < 0) ? ", closed" : ", opened")
        << ", uid = " << uid_
        << ", pid = " << pid_
        << ", tokenType = " << tokenType_
        << std::endl;
    descript_ = oss.str().c_str();
#endif // OHOS_BUILD_ENABLE_RUST
}

bool StreamSession::SendMsg(const NetPacket &pkt) const
{
#ifdef OHOS_BUILD_ENABLE_RUST
    if (chk_rwerror(&pkt.rustStreamBuffer_)) {
        SEN_HILOGE("Read and write status is error");
        return false;
    }
    StreamBuffer buf;
    pkt.MakeData(buf);
    return SendMsg(data(&buf.rustStreamBuffer_), size(&buf.rustStreamBuffer_));
#else
    if (pkt.ChkRWError()) {
        SEN_HILOGE("Read and write status failed");
        return false;
    }
    StreamBuffer buf;
    pkt.MakeData(buf);
    return SendMsg(buf.Data(), buf.Size());
#endif // OHOS_BUILD_ENABLE_RUST
}

int32_t StreamSession::GetUid() const
{
#ifdef OHOS_BUILD_ENABLE_RUST
    return get_uid(&rustStreamSession_);
#else
    return uid_;
#endif // OHOS_BUILD_ENABLE_RUST
}

int32_t StreamSession::GetPid() const
{
#ifdef OHOS_BUILD_ENABLE_RUST
    return get_pid(&rustStreamSession_);
#else
    return pid_;
#endif // OHOS_BUILD_ENABLE_RUST
}

SessionPtr StreamSession::GetSharedPtr()
{
    return shared_from_this();
}

int32_t StreamSession::GetFd() const
{
#ifdef OHOS_BUILD_ENABLE_RUST
    return get_session_fd(&rustStreamSession_);
#else
    return fd_;
#endif // OHOS_BUILD_ENABLE_RUST
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
#ifdef OHOS_BUILD_ENABLE_RUST
    set_token_type(&rustStreamSession_, type);
#else
    tokenType_ = type;
#endif // OHOS_BUILD_ENABLE_RUST
}

int32_t StreamSession::GetTokenType() const
{
#ifdef OHOS_BUILD_ENABLE_RUST
    return get_token_type(&rustStreamSession_);
#else
    return tokenType_;
#endif // OHOS_BUILD_ENABLE_RUST
}
}  // namespace Sensors
}  // namespace OHOS