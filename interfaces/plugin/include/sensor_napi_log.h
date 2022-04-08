/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef SENSOR_NAPI_LOG_H
#define SENSOR_NAPI_LOG_H

#include "native_common.h"
#include "sensor_log.h"

namespace OHOS {
namespace Sensors {
#define CHKNCR(env, cond, message, retVal)                                                   \
    do {                                                                                     \
        if (!(cond)) {                                                                       \
            SEN_HILOGE("CK(%{public}s) fail, %{public}s", #cond, message);                   \
            napi_throw_error((env), nullptr, "cond (" #cond ") failed: " message);           \
            return retVal;                                                                   \
        }                                                                                    \
    } while (0)

#define CHKNCP(env, cond, message)                                                           \
    do {                                                                                     \
        if (!(cond)) {                                                                       \
            SEN_HILOGE("CK(%{public}s) fail, %{public}s", #cond, message);                   \
            napi_throw_error((env), nullptr, "cond (" #cond ") failed: " message);           \
            return nullptr;                                                                  \
        }                                                                                    \
    } while (0)

#define CHKNCF(env, cond, message)                                                           \
    do {                                                                                     \
        if (!(cond)) {                                                                       \
            SEN_HILOGE("CK(%{public}s) fail, %{public}s", #cond, message);                   \
            napi_throw_error((env), nullptr, "cond (" #cond ") failed: " message);           \
            return false;                                                                    \
        }                                                                                    \
    } while (0)

#define CHKNCV(env, cond, message)                                                           \
    do {                                                                                     \
        if (!(cond)) {                                                                       \
            SEN_HILOGE("CK(%{public}s) fail, %{public}s", #cond, message);                   \
            napi_throw_error((env), nullptr, "cond (" #cond ") failed: " message);           \
            return;                                                                          \
        }                                                                                    \
    } while (0)

#define CHKNCC(env, cond, message)                                                           \
    do {                                                                                     \
        if (!(cond)) {                                                                       \
            SEN_HILOGE("CK(%{public}s) fail, %{public}s", #cond, message);                   \
            napi_throw_error((env), nullptr, "cond (" #cond ") failed: " message);           \
            continue;                                                                        \
        }                                                                                    \
    } while (0)

#define CHKNRR(env, theCall, retVal)                         \
    do {                                                     \
        if ((theCall) != napi_ok) {                          \
            SEN_HILOGE("(%{public}s) fail", #theCall);       \
            GET_AND_THROW_LAST_ERROR((env));                 \
            return retVal;                                   \
        }                                                    \
    } while (0)

#define CHKNRP(env, theCall)                                 \
    do {                                                     \
        if ((theCall) != napi_ok) {                          \
            SEN_HILOGE("(%{public}s) fail", #theCall);       \
            GET_AND_THROW_LAST_ERROR((env));                 \
            return nullptr;                                  \
        }                                                    \
    } while (0)

#define CHKNRF(env, theCall)                                 \
    do {                                                     \
        if ((theCall) != napi_ok) {                          \
            SEN_HILOGE("(%{public}s) fail", #theCall);       \
            GET_AND_THROW_LAST_ERROR((env));                 \
            return false;                                    \
        }                                                    \
    } while (0)

#define CHKNRV(env, theCall)                                 \
    do {                                                     \
        if ((theCall) != napi_ok) {                          \
            SEN_HILOGE("(%{public}s) fail", #theCall);       \
            GET_AND_THROW_LAST_ERROR((env));                 \
            return;                                          \
        }                                                    \
    } while (0)

#define CHKNRC(env, theCall)                                 \
    do {                                                     \
        if ((theCall) != napi_ok) {                          \
            SEN_HILOGE("(%{public}s) fail", #theCall);       \
            GET_AND_THROW_LAST_ERROR((env));                 \
            continue;                                        \
        }                                                    \
    } while (0)
}  // namespace Sensors
}  // namespace OHOS
#endif // SENSOR_NAPI_LOG_H