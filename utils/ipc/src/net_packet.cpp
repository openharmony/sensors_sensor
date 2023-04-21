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

#include "net_packet.h"

#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
NetPacket::NetPacket(MessageId msgId) : msgId_(msgId)
{}

NetPacket::NetPacket(const NetPacket &pkt) : NetPacket(pkt.GetMsgId())
{
    Clone(pkt);
}

void NetPacket::MakeData(StreamBuffer &buf) const
{
#ifdef OHOS_BUILD_ENABLE_RUST
    PACKHEAD head = {msgId_, rustStreamBuffer_.wPos_};
    buf << head;
    if (rustStreamBuffer_.wPos_ > 0) {
        if (!buf.Write(&rustStreamBuffer_.szBuff_[0], rustStreamBuffer_.wPos_)) {
            SEN_HILOGE("Write data to stream failed");
            return;
        }
    }
#else
    PACKHEAD head = {msgId_, wPos_};
    buf << head;
    if (wPos_ > 0) {
        if (!buf.Write(&szBuff_[0], wPos_)) {
            SEN_HILOGE("Write data to stream failed");
            return;
        }
    }
#endif // OHOS_BUILD_ENABLE_RUST
}

size_t NetPacket::GetSize() const
{
#ifdef OHOS_BUILD_ENABLE_RUST
    return size(&rustStreamBuffer_);
#else
    return Size();
#endif // OHOS_BUILD_ENABLE_RUST
}

size_t NetPacket::GetPacketLength() const
{
#ifdef OHOS_BUILD_ENABLE_RUST
    return (static_cast<int32_t>(sizeof(PackHead)) + rustStreamBuffer_.wPos_);
#else
    return sizeof(PackHead) + wPos_;
#endif // OHOS_BUILD_ENABLE_RUST
}

const char* NetPacket::GetData() const
{
#ifdef OHOS_BUILD_ENABLE_RUST
    return data(&rustStreamBuffer_);
#else
    return Data();
#endif // OHOS_BUILD_ENABLE_RUST
}

MessageId NetPacket::GetMsgId() const
{
    return msgId_;
}
} // namespace Sensors
} // namespace OHOS
