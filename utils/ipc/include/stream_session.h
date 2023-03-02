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

#ifndef STREAM_SESSION_H
#define STREAM_SESSION_H

#include <list>
#include <memory>
#include <map>

#include <sys/socket.h>
#include <sys/un.h>

#include "accesstoken_kit.h"
#include "nocopyable.h"

#include "net_packet.h"
#include "proto.h"

namespace OHOS {
namespace Sensors {
class StreamSession;
using SessionPtr = std::shared_ptr<StreamSession>;
using namespace Security::AccessToken;
class StreamSession : public std::enable_shared_from_this<StreamSession> {
public:
    StreamSession(const std::string &programName, const int32_t moduleType, const int32_t fd, const int32_t uid,
                  const int32_t pid);
    virtual ~StreamSession() = default;
    bool SendMsg(const char *buf, size_t size) const;
    bool SendMsg(NetPacket &pkt) const;
    void Close();
    int32_t GetUid() const;
    int32_t GetPid() const;
    int32_t GetModuleType() const;
    SessionPtr GetSharedPtr();
    int32_t GetFd() const;
    const std::string& GetDescript() const;
    const std::string GetProgramName() const;
    void SetTokenType(int32_t type);
    int32_t GetTokenType() const;
    void UpdateDescript();
    DISALLOW_COPY_AND_MOVE(StreamSession);

protected:
    struct EventTime {
        int32_t id { 0 };
        int64_t eventTime { 0 };
        int32_t timerId { -1 };
    };
    std::map<int32_t, std::vector<EventTime>> events_;
    std::string descript_;
    const std::string programName_;
    const int32_t moduleType_ { -1 };
    int32_t fd_ { -1 };
    const int32_t uid_ { -1 };
    const int32_t pid_ { -1 };
    int32_t tokenType_ { ATokenTypeEnum::TOKEN_INVALID };
};
}  // namespace Sensors
}  // namespace OHOS
#endif // STREAM_SESSION_H