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

#include "stream_server.h"

#include <cinttypes>
#include <sys/socket.h>

#include "accesstoken_kit.h"

#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
namespace {
using namespace Security::AccessToken;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "StreamServer" };
constexpr int32_t INVALID_PID = -1;
constexpr int32_t INVALID_FD = -1;
}  // namespace
StreamServer::~StreamServer()
{
    CALL_LOG_ENTER;
    idxPidMap_.clear();
    for (const auto &item : sessionsMap_) {
        item.second->Close();
    }
    sessionsMap_.clear();
}

bool StreamServer::SendMsg(int32_t fd, NetPacket& pkt)
{
    CALL_LOG_ENTER;
    if (fd < 0) {
        SEN_HILOGE("Fd is invalid");
        return false;
    }
    auto ses = GetSession(fd);
    if (ses == nullptr) {
        SEN_HILOGE("Fd not found, The message was discarded.");
        return false;
    }
    return ses->SendMsg(pkt);
}

void StreamServer::Multicast(const std::vector<int32_t>& fdList, NetPacket& pkt)
{
    CALL_LOG_ENTER;
    for (const auto &item : fdList) {
        SendMsg(item, pkt);
    }
}

int32_t StreamServer::GetClientFd(int32_t pid)
{
    std::lock_guard<std::mutex> idxPidLock(idxPidMutex_);
    auto it = idxPidMap_.find(pid);
    if (it == idxPidMap_.end()) {
        return INVALID_FD;
    }
    return it->second;
}

int32_t StreamServer::GetClientPid(int32_t fd)
{
    std::lock_guard<std::mutex> sessionLock(sessionMutex_);
    auto it = sessionsMap_.find(fd);
    if (it == sessionsMap_.end()) {
        return INVALID_PID;
    }
    return it->second->GetPid();
}

SessionPtr StreamServer::GetSession(int32_t fd)
{
    std::lock_guard<std::mutex> sessionLock(sessionMutex_);
    auto it = sessionsMap_.find(fd);
    if (it == sessionsMap_.end()) {
        SEN_HILOGE("Session not found");
        return nullptr;
    }
    CHKPP(it->second);
    return it->second->GetSharedPtr();
}

SessionPtr StreamServer::GetSessionByPid(int32_t pid)
{
    int32_t fd = GetClientFd(pid);
    if (fd <= 0) {
        SEN_HILOGE("Session not found");
        return nullptr;
    }
    return GetSession(fd);
}

int32_t StreamServer::AddSocketPairInfo(int32_t uid, int32_t pid, int32_t tokenType,
                                        int32_t &serverFd, int32_t &clientFd)
{
    CALL_LOG_ENTER;
    std::string programName = "";
    int32_t sockFds[2] = { -1 };
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockFds) != 0) {
        SEN_HILOGE("Call socketpair failed, errno:%{public}d", errno);
        return ERROR;
    }
    serverFd = sockFds[0];
    clientFd = sockFds[1];
    if (serverFd < 0 || clientFd < 0) {
        SEN_HILOGE("Call fcntl failed, errno:%{public}d", errno);
        return ERROR;
    }
    static constexpr size_t bufferSize = 32 * 1024;
    static constexpr size_t nativeBufferSize = 64 * 1024;
    SessionPtr sess = nullptr;
    if (setsockopt(serverFd, SOL_SOCKET, SO_SNDBUF, &bufferSize, sizeof(bufferSize)) != 0) {
        SEN_HILOGE("Setsockopt serverFd failed, errno: %{public}d", errno);
        goto CLOSE_SOCK;
    }
    if (setsockopt(serverFd, SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize)) != 0) {
        SEN_HILOGE("Setsockopt serverFd failed, errno: %{public}d", errno);
        goto CLOSE_SOCK;
    }
    if (tokenType == ATokenTypeEnum::TOKEN_NATIVE) {
        if (setsockopt(clientFd, SOL_SOCKET, SO_SNDBUF, &nativeBufferSize, sizeof(nativeBufferSize)) != 0) {
            SEN_HILOGE("Setsockopt clientFd failed, errno: %{public}d", errno);
            goto CLOSE_SOCK;
        }
        if (setsockopt(clientFd, SOL_SOCKET, SO_RCVBUF, &nativeBufferSize, sizeof(nativeBufferSize)) != 0) {
            SEN_HILOGE("Setsockopt clientFd failed, errno: %{public}d", errno);
            goto CLOSE_SOCK;
        }
    } else {
        if (setsockopt(clientFd, SOL_SOCKET, SO_SNDBUF, &bufferSize, sizeof(bufferSize)) != 0) {
            SEN_HILOGE("Setsockopt clientFd failed, errno: %{public}d", errno);
            goto CLOSE_SOCK;
        }
        if (setsockopt(clientFd, SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize)) != 0) {
            SEN_HILOGE("Setsockopt clientFd failed, errno: %{public}d", errno);
            goto CLOSE_SOCK;
        }
    }
    sess = std::make_shared<StreamSession>(programName, serverFd, uid, pid);
    sess->SetTokenType(tokenType);
    if (!AddSession(sess)) {
        SEN_HILOGE("AddSession fail");
        goto CLOSE_SOCK;
    }
    return ERR_OK;

CLOSE_SOCK:
    close(serverFd);
    serverFd = -1;
    close(clientFd);
    clientFd = -1;
    return ERROR;
}

bool StreamServer::AddSession(SessionPtr ses)
{
    CALL_LOG_ENTER;
    CHKPF(ses);
    auto fd = ses->GetFd();
    if (fd < 0) {
        SEN_HILOGE("Fd is Invalid");
        return false;
    }
    auto pid = ses->GetPid();
    if (pid <= 0) {
        SEN_HILOGE("Get process failed");
        return false;
    }
    if (sessionsMap_.size() > MAX_SESSON_ALARM) {
        SEN_HILOGE("Too many clients. Warning Value:%{public}zu, Current Value:%{public}zu",
            MAX_SESSON_ALARM, sessionsMap_.size());
        return false;
    }
    DelSession(pid);
    std::lock_guard<std::mutex> idxPidLock(idxPidMutex_);
    idxPidMap_[pid] = fd;
    std::lock_guard<std::mutex> sessionLock(sessionMutex_);
    sessionsMap_[fd] = ses;
    return true;
}

void StreamServer::DelSession(int32_t pid)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> idxPidLock(idxPidMutex_);
    auto pidIt = idxPidMap_.find(pid);
    if (pidIt == idxPidMap_.end()) {
        return;
    }
    int32_t fd = pidIt->second;
    idxPidMap_.erase(pidIt);
    std::lock_guard<std::mutex> sessionLock(sessionMutex_);
    auto fdIt = sessionsMap_.find(fd);
    if (fdIt != sessionsMap_.end()) {
        sessionsMap_.erase(fdIt);
    }
    if (fd >= 0) {
        auto rf = close(fd);
        if (rf > 0) {
            SEN_HILOGE("Socket fd close failed, rf:%{public}d", rf);
        }
    }
}
}  // namespace Sensors
}  // namespace OHOS