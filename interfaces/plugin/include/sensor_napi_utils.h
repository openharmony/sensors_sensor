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
#ifndef SENSOR_NAPI_UTILS_H
#define SENSOR_NAPI_UTILS_H

#include <iostream>
#include <uv.h>

#include "async_callback_info.h"
#include "refbase.h"
namespace OHOS {
namespace Sensors {
using std::vector;
using std::string;
using ConvertDataFunc = bool(*)(const napi_env &env, sptr<AsyncCallbackInfo> asyncCallbackInfo,
    napi_value result[2]);

bool IsSameValue(const napi_env &env, const napi_value &lhs, const napi_value &rhs);
bool IsMatchType(const napi_env &env, const napi_value &value, const napi_valuetype &type);
bool IsMatchArrayType(const napi_env &env, const napi_value &value);
bool GetCppInt32(const napi_env &env, const napi_value &value, int32_t &number);
bool GetCppDouble(const napi_env &env, const napi_value &value, double &number);
bool GetCppBool(const napi_env &env, const napi_value &value);
bool GetFloatArray(const napi_env &env, const napi_value &value, vector<float> &array);
bool GetCppInt64(const napi_env &env, const napi_value &value, int64_t &number);
napi_value GetNamedProperty(const napi_env &env, const napi_value &object, string name);
bool GetCppFloat(const napi_env &env, const napi_value &value, float &number);
napi_value GetNapiInt32(const napi_env &env, int32_t number);
void EmitAsyncCallbackWork(sptr<AsyncCallbackInfo> asyncCallbackInfo);
void EmitUvEventLoop(sptr<AsyncCallbackInfo> asyncCallbackInfo);
void EmitPromiseWork(sptr<AsyncCallbackInfo> asyncCallbackInfo);

napi_value GreateBusinessError(const napi_env &env, int32_t errCode, string errMessage,
    string errName, string errStack);
bool ConvertToFailData(const napi_env &env, sptr<AsyncCallbackInfo> asyncCallbackInfo, napi_value result[2]);
bool ConvertToGeomagneticData(const napi_env &env, sptr<AsyncCallbackInfo> asyncCallbackInfo, napi_value result[2]);
bool ConvertToNumber(const napi_env &env, sptr<AsyncCallbackInfo> asyncCallbackInfo, napi_value result[2]);
bool ConvertToArray(const napi_env &env, sptr<AsyncCallbackInfo> asyncCallbackInfo, napi_value result[2]);
bool ConvertToRotationMatrix(const napi_env &env, sptr<AsyncCallbackInfo> asyncCallbackInfo, napi_value result[2]);
bool ConvertToSensorData(const napi_env &env, sptr<AsyncCallbackInfo> asyncCallbackInfo, napi_value result[2]);
bool CreateNapiArray(const napi_env &env, float *data, int32_t dataLength, napi_value &result);
bool ConvertToSensorInfos(const napi_env &env, sptr<AsyncCallbackInfo> asyncCallbackInfo, napi_value result[2]);
bool ConvertToSingleSensor(const napi_env &env, sptr<AsyncCallbackInfo> asyncCallbackInfo, napi_value result[2]);

#define GET_AND_THROW_NAPI_ERROR(env, message) \
    do { \
        const napi_extended_error_info* errorInfo = nullptr; \
        napi_get_last_error_info((env), &errorInfo); \
        bool isPending = false; \
        napi_is_exception_pending((env), &isPending); \
        if (!isPending && errorInfo != nullptr) { \
            std::string errDesc = std::string(__FUNCTION__) + ": " + #message + " fail. "; \
            std::string errorMessage = \
                errorInfo->error_message != nullptr ? errorInfo->error_message : "empty error message"; \
            errDesc += errorMessage; \
            napi_throw_error((env), nullptr, errDesc.c_str()); \
        } \
    } while (0)

#define CHKNCR(env, cond, message, retVal) \
    do { \
        if (!(cond)) { \
            SEN_HILOGE("(%{public}s)", #message); \
            auto errDesc = std::string(__FUNCTION__) + ": " + #message; \
            napi_throw_error(env, nullptr, errDesc.c_str()); \
            return retVal; \
        } \
    } while (0)

#define CHKNCP(env, cond, message) \
    do { \
        if (!(cond)) { \
            SEN_HILOGE("(%{public}s)", #message); \
            auto errDesc = std::string(__FUNCTION__) + ": " + #message; \
            napi_throw_error(env, nullptr, errDesc.c_str()); \
            return nullptr; \
        } \
    } while (0)

#define CHKNCF(env, cond, message) \
    do { \
        if (!(cond)) { \
            SEN_HILOGE("(%{public}s)", #message); \
            auto errDesc = std::string(__FUNCTION__) + ": " + #message; \
            napi_throw_error(env, nullptr, errDesc.c_str()); \
            return false; \
        } \
    } while (0)

#define CHKNCV(env, cond, message) \
    do { \
        if (!(cond)) { \
            SEN_HILOGE("(%{public}s)", #message); \
            auto errDesc = std::string(__FUNCTION__) + ": " + #message; \
            napi_throw_error(env, nullptr, errDesc.c_str()); \
            return; \
        } \
    } while (0)

#define CHKNCC(env, cond, message) \
    { \
        if (!(cond)) { \
            SEN_HILOGW("(%{public}s)", #message); \
            auto errDesc = std::string(__FUNCTION__) + ": " + #message; \
            napi_throw_error(env, nullptr, errDesc.c_str()); \
            continue; \
        } \
    }

#define CHKNRR(env, state, message, retVal) \
    do { \
        if ((state) != napi_ok) { \
            SEN_HILOGE("(%{public}s) fail", #message); \
            GET_AND_THROW_NAPI_ERROR((env), (message)); \
            return retVal; \
        } \
    } while (0)

#define CHKNRP(env, state, message) \
    do { \
        if ((state) != napi_ok) { \
            SEN_HILOGE("(%{public}s) fail", #message); \
            GET_AND_THROW_NAPI_ERROR((env), (message)); \
            return nullptr; \
        } \
    } while (0)

#define CHKNRF(env, state, message) \
    do { \
        if ((state) != napi_ok) { \
            SEN_HILOGE("(%{public}s) fail", #message); \
            GET_AND_THROW_NAPI_ERROR((env), (message)); \
            return false; \
        } \
    } while (0)

#define CHKNRV(env, state, message) \
    do { \
        if ((state) != napi_ok) { \
            SEN_HILOGE("(%{public}s) fail", #message); \
            GET_AND_THROW_NAPI_ERROR((env), (message)); \
            return; \
        } \
    } while (0)

#define CHKNRC(env, state, message) \
    { \
        if ((state) != napi_ok) { \
            SEN_HILOGW("(%{public}s) fail", #message); \
            GET_AND_THROW_NAPI_ERROR((env), (message)); \
            continue; \
        } \
    }
}  // namespace Sensors
}  // namespace OHOS
#endif // SENSOR_NAPI_UTILS_H
