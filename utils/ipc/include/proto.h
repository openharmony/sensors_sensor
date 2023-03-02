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

#ifndef PROTO_H
#define PROTO_H

#include <sys/types.h>

namespace OHOS {
namespace Sensors {
#define PROTO_SEND_RETRY_LIMIT 32
#define PROTO_SEND_RETRY_SLEEP_TIME 10000

static const int32_t PROTO_STREAM_BUF_READ_FAIL = 1;
static const int32_t PROTO_STREAM_BUF_WRITE_FAIL = 2;
static const int32_t PROTO_MEM_OUT_OF_BOUNDS = 3;
static const int32_t PROTO_MEMCPY_SEC_FUN_FAIL = 4;
static const int32_t PROTO_PARAM_INPUT_INVALID = 5;
static const int32_t PROTO_SESSION_NOT_FOUND = 8;
static const int32_t PROTO_MAX_VECTOR_SIZE = 10;
static const int32_t PROTO_ADD_SESSION_FAIL = 11;
static const int32_t PROTO_MAX_SESSON_ALARM = 12;
static const int32_t PROTO_MAX_RECV_LIMIT = 13;

static const int32_t PROTO_MAX_STREAM_BUF_SIZE = 256;
static const int32_t PROTO_MAX_PACKET_BUF_SIZE = 256;
static const int32_t PROTO_ONCE_PROCESS_NETPACKET_LIMIT = 100;

enum class MessageId : int32_t {
    INVALID,
    CLIENT_INFO,
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // PROTO_H