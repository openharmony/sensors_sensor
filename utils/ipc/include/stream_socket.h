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

#ifndef STREAM_SOCKET_H
#define STREAM_SOCKET_H

#include <functional>

#include <sys/socket.h>
#include <unistd.h>

#include "nocopyable.h"

#include "circle_stream_buffer.h"
#include "net_packet.h"

namespace OHOS {
namespace Sensors {
class StreamSocket {
public:
    using PacketCallBackFun = std::function<void(NetPacket&)>;
    StreamSocket();
    virtual ~StreamSocket();
    void OnReadPackets(CircleStreamBuffer &buf, PacketCallBackFun callbackFun);
    void Close();
    int32_t GetFd() const;

    DISALLOW_COPY_AND_MOVE(StreamSocket);

protected:
    int32_t fd_ { -1 };
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // STREAM_SOCKET_H