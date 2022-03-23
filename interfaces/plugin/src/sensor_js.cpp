/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "sensor_js.h"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory.h>
#include <pthread.h>
#include <string>
#include <thread>
#include <unistd.h>

#include "geomagnetic_field.h"

#include "hilog/log.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "refbase.h"
#include "securec.h"
#include "sensor_agent.h"
#include "sensor_algorithm.h"
#include "sensor_napi_utils.h"
#include "sensor_system_js.h"

using namespace OHOS::HiviewDFX;
static constexpr HiLogLabel LABEL = {LOG_CORE, 0xD002708, "SensorJsAPI"};

const int32_t REPORTING_INTERVAL = 200000000;

static std::map<int32_t, struct AsyncCallbackInfo*> g_onceCallbackInfos;
static std::map<int32_t, std::vector<struct AsyncCallbackInfo*>> g_onCallbackInfos;

static void DataCallbackImpl(SensorEvent *event)
{
    if (event == nullptr) {
        HiLog::Error(LABEL, "%{public}s event is null", __func__);
        return;
    }
    int32_t sensorTypeId = event->sensorTypeId;
    float *data = (float *)(event->data);
    if (g_onCallbackInfos.find(sensorTypeId) != g_onCallbackInfos.end()) {
        for (uint32_t i = 0; i < g_onCallbackInfos[sensorTypeId].size(); ++i) {
            g_onCallbackInfos[sensorTypeId][i]->data.sensorData.sensorTypeId = sensorTypeId;
            g_onCallbackInfos[sensorTypeId][i]->data.sensorData.dataLength = event->dataLen;
            g_onCallbackInfos[sensorTypeId][i]->data.sensorData.timestamp = event->timestamp;
            errno_t ret = memcpy_s(g_onCallbackInfos[sensorTypeId][i]->data.sensorData.data,
                event->dataLen, data, event->dataLen);
            if (ret != EOK) {
                HiLog::Error(LABEL, "%{public}s copy data failed", __func__);
                return;
            }
            g_onCallbackInfos[sensorTypeId][i]->type = ON_CALLBACK;
            EmitUvEventLoop(&g_onCallbackInfos[sensorTypeId][i]);
        }
    }

    if (g_onceCallbackInfos.find(sensorTypeId) == g_onceCallbackInfos.end()) {
        return;
    }
    struct AsyncCallbackInfo *onceCallbackInfo = g_onceCallbackInfos[sensorTypeId];
    onceCallbackInfo->data.sensorData.sensorTypeId = sensorTypeId;
    onceCallbackInfo->data.sensorData.dataLength = event->dataLen;
    onceCallbackInfo->data.sensorData.timestamp = event->timestamp;
    errno_t ret = memcpy_s(onceCallbackInfo->data.sensorData.data, event->dataLen, data, event->dataLen);
    if (ret != EOK) {
        HiLog::Error(LABEL, "%{public}s copy data failed", __func__);
        return;
    }
    onceCallbackInfo->type = ONCE_CALLBACK;
    EmitUvEventLoop(&g_onceCallbackInfos[sensorTypeId]);
    if (g_onCallbackInfos.find(sensorTypeId) == g_onCallbackInfos.end()) {
        HiLog::Debug(LABEL, "%{public}s no subscription to change sensor data, need to cancel registration", __func__);
        UnsubscribeSensor(sensorTypeId);
    }
    g_onceCallbackInfos.erase(sensorTypeId);
}

static const SensorUser user = {
    .callback = DataCallbackImpl
};

static int32_t UnsubscribeSensor(int32_t sensorTypeId)
{
    HiLog::Info(LABEL, "%{public}s in", __func__);
    int32_t ret = DeactivateSensor(sensorTypeId, &user);
    if (ret != OHOS::ERR_OK) {
        HiLog::Error(LABEL, "%{public}s  DeactivateSensor failed", __func__);
        return ret;
    }
    ret = UnsubscribeSensor(sensorTypeId, &user);
    if (ret != OHOS::ERR_OK) {
        HiLog::Error(LABEL, "%{public}s  UnsubscribeSensor failed", __func__);
        return ret;
    }
    HiLog::Info(LABEL, "%{public}s left", __func__);
    return 0;
}

static int32_t SubscribeSensor(int32_t sensorTypeId, int64_t interval, RecordSensorCallback callback)
{
    HiLog::Info(LABEL, "%{public}s in, sensorTypeId: %{public}d", __func__, sensorTypeId);
    int32_t ret = SubscribeSensor(sensorTypeId, &user);
    if (ret != OHOS::ERR_OK) {
        HiLog::Error(LABEL, "%{public}s subscribeSensor failed", __func__);
        return ret;
    }
    ret = SetBatch(sensorTypeId, &user, interval, 0);
    if (ret != OHOS::ERR_OK) {
        HiLog::Error(LABEL, "%{public}s set batch failed", __func__);
        return ret;
    }
    ret = ActivateSensor(sensorTypeId, &user);
    if (ret != OHOS::ERR_OK) {
        HiLog::Error(LABEL, "%{public}s activateSensor failed", __func__);
        return ret;
    }
    return 0;
}

static napi_value Once(napi_env env, napi_callback_info info)
{
    HiLog::Info(LABEL, "%{public}s in", __func__);
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, NULL));

    if (argc != 2) {
        HiLog::Error(LABEL, "%{public}s Invalid input, number of argument should be two", __func__);
        return nullptr;
    }
    if (!IsMatchType(env, args[0], napi_number) || !IsMatchType(env, args[1], napi_function)) {
        HiLog::Error(LABEL, "%{public}s argument is invalid", __func__);
        return nullptr;
    }
    int32_t sensorTypeId = GetCppInt32(args[0], env);
    AsyncCallbackInfo *asyncCallbackInfo = new AsyncCallbackInfo {
        .env = env,
        .asyncWork = nullptr,
        .deferred = nullptr,
    };
    napi_create_reference(env, args[1], 1, &asyncCallbackInfo->callback[0]);
    g_onceCallbackInfos[sensorTypeId] = asyncCallbackInfo;
    if (g_onCallbackInfos.find(sensorTypeId) == g_onCallbackInfos.end()) {
        HiLog::Debug(LABEL, "%{public}s no subscription to change sensor data, registration is required", __func__);
        int32_t ret = SubscribeSensor(sensorTypeId, 200000000, DataCallbackImpl);
        if (ret != OHOS::ERR_OK) {
            HiLog::Error(LABEL, "%{public}s subscribe Sensor failed", __func__);
            g_onceCallbackInfos.erase(sensorTypeId);
            return nullptr;
        }
    }
    HiLog::Info(LABEL, "%{public}s left", __func__);
    return nullptr;
}

static bool IsSubscribed(napi_env env, int32_t sensorTypeId, napi_value callback)
{
    HiLog::Info(LABEL, "%{public}s in, sensorTypeId: %{public}d", __func__, sensorTypeId);
    if (g_onCallbackInfos.find(sensorTypeId) == g_onCallbackInfos.end()) {
        return false;
    }
    std::vector<AsyncCallbackInfo*> callbackInfos = g_onCallbackInfos[sensorTypeId];
    for (auto callbackInfo : callbackInfos) {
        napi_value sensorCallback = nullptr;
        napi_get_reference_value(env, callbackInfo->callback[0], &sensorCallback);
        if (IsSameValue(env, callback, sensorCallback)) {
            return true;
        }
    }
    return false;
}

static void UpdateCallbackInfos(napi_env env, int32_t sensorTypeId, napi_value callback)
{
    if (IsSubscribed(env, sensorTypeId, callback)) {
        HiLog::Debug(LABEL, "%{public}s the callback has been subscribed", __func__);
        return;
    }
    AsyncCallbackInfo *asyncCallbackInfo = new AsyncCallbackInfo {
        .env = env,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .type = ON_CALLBACK,
    };
    napi_create_reference(env, callback, 1, &asyncCallbackInfo->callback[0]);
    std::vector<struct AsyncCallbackInfo*> callbackInfos = g_onCallbackInfos[sensorTypeId];
    callbackInfos.push_back(asyncCallbackInfo);
    g_onCallbackInfos[sensorTypeId] = callbackInfos;
}

static napi_value On(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    size_t argc = 3;
    napi_value args[3] = { 0 };
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, NULL));
    if (argc <= 1 || argc > 3) {
        HiLog::Error(LABEL, "%{public}s the number of input parameters does not match", __func__);
        return nullptr;
    }
    if (!IsMatchType(env, args[0], napi_number) || !IsMatchType(env, args[1], napi_function)) {
        HiLog::Error(LABEL, "%{public}s the first parameter should be napi_number type", __func__);
        return nullptr;
    }
    int32_t sensorTypeId = GetCppInt32(args[0], env);
    int64_t interval = REPORTING_INTERVAL;
    if (argc == 3) {
        napi_value value = NapiGetNamedProperty(args[2], "interval", env);
        if (!IsMatchType(env, value, napi_number)) {
            HiLog::Error(LABEL, "%{public}s argument should be napi_number type", __func__);
            return nullptr;
        }
        interval = GetCppInt64(value, env);
        HiLog::Debug(LABEL, "%{public}s interval is %{public}" PRId64, __func__, interval);
    }
    int32_t ret = SubscribeSensor(sensorTypeId, interval, DataCallbackImpl);
    if (ret != OHOS::ERR_OK) {
        HiLog::Error(LABEL, "%{public}s subscribeSensor failed", __func__);
        return nullptr;
    }
    UpdateCallbackInfos(env, sensorTypeId, args[1]);
    HiLog::Debug(LABEL, "%{public}s end", __func__);
    return nullptr;
}

static void RemoveAllCallback(napi_env env, int32_t sensorTypeId)
{
    std::vector<struct AsyncCallbackInfo*> callbackInfos = g_onCallbackInfos[sensorTypeId];
    for (auto callbackInfo : callbackInfos) {
        napi_delete_reference(env, callbackInfo->callback[0]);
        callbackInfo->callback[0] = nullptr;
        delete callbackInfo;
        callbackInfo = nullptr;
    }
    g_onCallbackInfos.erase(sensorTypeId);
}

static uint32_t RemoveCallback(napi_env env, int32_t sensorTypeId, napi_value callback)
{
    std::vector<struct AsyncCallbackInfo*> callbackInfos = g_onCallbackInfos[sensorTypeId];
    std::vector<struct AsyncCallbackInfo*>::iterator iter;
    for (iter = callbackInfos.begin(); iter != callbackInfos.end(); ++iter) {
        if (*iter == nullptr || (*iter)->callback[0] == nullptr) {
            continue;
        }
        napi_value sensorCallback = nullptr;
        napi_get_reference_value(env, (*iter)->callback[0], &sensorCallback);
        if (IsSameValue(env, callback, sensorCallback)) {
            napi_delete_reference(env, (*iter)->callback[0]);
            (*iter)->callback[0] = nullptr;
            delete *iter;
            *iter = nullptr;
            callbackInfos.erase(iter++);
            break;
        }
    }
    if (callbackInfos.empty()) {
        g_onCallbackInfos.erase(sensorTypeId);
        return 0;
    }
    g_onCallbackInfos[sensorTypeId] = callbackInfos;
    return callbackInfos.size();
}

static napi_value Off(napi_env env, napi_callback_info info)
{
    HiLog::Info(LABEL, "%{public}s in", __func__);
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, NULL));
    if (argc < 1 || argc > 2 || !IsMatchType(env, args[0], napi_number)) {
        HiLog::Error(LABEL, "%{public}s Invalid input", __func__);
        return nullptr;
    }
    int32_t sensorTypeId = GetCppInt32(args[0], env);
    if (g_onCallbackInfos.find(sensorTypeId) == g_onCallbackInfos.end()
        || (argc == 2 && !IsSubscribed(env, sensorTypeId, args[1]))) {
        HiLog::Error(LABEL, "%{public}s should subscribe first", __func__);
        return nullptr;
    }
    if (argc == 2 && RemoveCallback(env, sensorTypeId, args[1]) != 0) {
        HiLog::Debug(LABEL, "%{public}s there are other client registrations as well", __func__);
        return nullptr;
    }
    if (argc == 1) {
        RemoveAllCallback(env, sensorTypeId);
    }
    int32_t ret = UnsubscribeSensor(sensorTypeId);
    if (ret != OHOS::ERR_OK) {
        HiLog::Error(LABEL, "%{public}s UnsubscribeSensor failed", __func__);
    }
    return nullptr;
}

static napi_value GetGeomagneticField(napi_env env, napi_callback_info info)
{
    HiLog::Info(LABEL, "%{public}s in", __func__);
    size_t argc = 3;
    napi_value args[3] = { 0 };
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
    if (argc < 2 || argc > 3) {
        HiLog::Error(LABEL, "%{public}s the number of input parameters does not match", __func__);
        return nullptr;
    }
    if (!IsMatchType(env, args[0], napi_object) || !IsMatchType(env, args[1], napi_number)) {
        HiLog::Error(LABEL, "%{public}s argument is invalid", __func__);
        return nullptr;
    }
    napi_value napiLatitude = NapiGetNamedProperty(args[0], "latitude", env);
    double latitude = 0;
    napi_get_value_double(env, napiLatitude, &latitude);
    napi_value napiLongitude = NapiGetNamedProperty(args[0], "longitude", env);
    double longitude = 0;
    napi_get_value_double(env, napiLongitude, &longitude);
    napi_value napiAltitude = NapiGetNamedProperty(args[0], "altitude", env);
    double altitude = 0;
    napi_get_value_double(env, napiAltitude, &altitude);
    int64_t timeMillis = 0;
    timeMillis = GetCppInt64(args[1], env);

    GeomagneticField geomagneticField(latitude, longitude, altitude, timeMillis);
    AsyncCallbackInfo *asyncCallbackInfo = new AsyncCallbackInfo {
        .env = env,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .type = GET_GEOMAGNETIC_FIELD,
    };
    asyncCallbackInfo->data.geomagneticData = {
        .x = geomagneticField.ObtainX(),
        .y = geomagneticField.ObtainY(),
        .z = geomagneticField.ObtainZ(),
        .geomagneticDip = geomagneticField.ObtainGeomagneticDip(),
        .deflectionAngle = geomagneticField.ObtainDeflectionAngle(),
        .levelIntensity = geomagneticField.ObtainLevelIntensity(),
        .totalIntensity = geomagneticField.ObtainTotalIntensity(),
    };
    if (argc == 2) {
        napi_deferred deferred = nullptr;
        napi_value promise = nullptr;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    if (!IsMatchType(env, args[2], napi_function)) {
        HiLog::Error(LABEL, "%{public}s argument should be function type", __func__);
        delete asyncCallbackInfo;
        asyncCallbackInfo = nullptr;
        return nullptr;
    }
    napi_create_reference(env, args[2], 1, &asyncCallbackInfo->callback[0]);
    EmitAsyncCallbackWork(asyncCallbackInfo);
    return nullptr;
}

static napi_value TransformCoordinateSystem(napi_env env, napi_callback_info info)
{
    size_t argc = 3;
    napi_value args[3]  = { 0 };
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
    if (argc < 2 || argc > 3 || !IsMatchArrayType(env, args[0]) || !IsMatchType(env, args[1], napi_object)) {
        HiLog::Error(LABEL, "%{public}s the number of input parameters does not match", __func__);
        return nullptr;
    }
    std::vector<float> inRotationVector = GetCppArrayFloat(env, args[0]);
    napi_value napiAxisX = NapiGetNamedProperty(args[1], "axisX", env);
    napi_value napiAxisY = NapiGetNamedProperty(args[1], "axisY", env);
    if ((!IsMatchType(env, napiAxisX, napi_number)) || (!IsMatchType(env, napiAxisY, napi_number))) {
        HiLog::Error(LABEL, "%{public}s argument should be napi_number type", __func__);
        return nullptr;
    }
    int32_t axisX = GetCppInt32(napiAxisX, env);
    int32_t axisY = GetCppInt32(napiAxisY, env);
    AsyncCallbackInfo *asyncCallbackInfo = new AsyncCallbackInfo {
        .env = env,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .type = TRANSFORM_COORDINATE_SYSTEM,
    };
    int32_t inRotationVectorLength = static_cast<int32_t>(inRotationVector.size());
    std::vector<float> outRotationVector(inRotationVectorLength);
    std::unique_ptr<SensorAlgorithm> sensorAlgorithm = std::make_unique<SensorAlgorithm>();
    int32_t ret = sensorAlgorithm->transformCoordinateSystem(inRotationVector, axisX, axisY, outRotationVector);
    if (ret != OHOS::ERR_OK) {
        HiLog::Error(LABEL, "%{public}s EmitPromiseWork  failed", __func__);
        asyncCallbackInfo->type = FAIL;
        asyncCallbackInfo->error.code = ret;
    } else {
        for (int32_t i = 0; i < inRotationVectorLength; i++) {
            asyncCallbackInfo->data.reserveData.reserve[i] = outRotationVector[i];
        }
        asyncCallbackInfo->data.reserveData.length = inRotationVectorLength;
    }
    if (argc == 2) {
        napi_deferred deferred = nullptr;
        napi_value promise = nullptr;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    if (!IsMatchType(env, args[2], napi_function)) {
        HiLog::Error(LABEL, "%{public}s argument should be napi_function type", __func__);
        delete asyncCallbackInfo;
        asyncCallbackInfo = nullptr;
        return nullptr;
    }
    napi_create_reference(env, args[2], 1, &asyncCallbackInfo->callback[0]);
    EmitAsyncCallbackWork(asyncCallbackInfo);
    return nullptr;
}

static napi_value GetAngleModify(napi_env env, napi_callback_info info)
{
    size_t argc = 3;
    napi_value args[3] = { 0 };
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
    if (argc < 2 || argc > 4 || !IsMatchArrayType(env, args[0]) || !IsMatchArrayType(env, args[1])) {
        HiLog::Info(LABEL, "%{public}s argument error", __func__);
        return nullptr;
    }
    AsyncCallbackInfo *asyncCallbackInfo = new AsyncCallbackInfo {
        .env = env,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .type = GET_ANGLE_MODIFY,
    };
    std::vector<float> curRotationVector = GetCppArrayFloat(env, args[0]);
    std::vector<float> preRotationVector = GetCppArrayFloat(env, args[1]);
    std::vector<float> angleChange(ROTATION_VECTOR_LENGTH);
    std::unique_ptr<SensorAlgorithm> sensorAlgorithm = std::make_unique<SensorAlgorithm>();
    int32_t ret = sensorAlgorithm->getAngleModify(curRotationVector, preRotationVector, angleChange);
    if (ret != OHOS::ERR_OK) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        asyncCallbackInfo->type = FAIL;
        asyncCallbackInfo->error.code = ret;
    } else {
        asyncCallbackInfo->data.reserveData.length = ROTATION_VECTOR_LENGTH;
        for (int32_t i = 0; i < ROTATION_VECTOR_LENGTH; i++) {
            asyncCallbackInfo->data.reserveData.reserve[i] = angleChange[i];
        }
    }
    if (argc == 2) {
        napi_deferred deferred = nullptr;
        napi_value promise = nullptr;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    if (!IsMatchType(env, args[2], napi_function)) {
        HiLog::Error(LABEL, "%{public}s parameter should be napi_fouction type", __func__);
        delete asyncCallbackInfo;
        asyncCallbackInfo = nullptr;
        return nullptr;
    }
    napi_create_reference(env, args[2], 1, &asyncCallbackInfo->callback[0]);
    EmitAsyncCallbackWork(asyncCallbackInfo);
    return nullptr;
}

static napi_value GetDirection(napi_env env, napi_callback_info info)
{
    size_t argc = 3;
    napi_value args[3] = { 0 };
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, NULL));
    if (argc < 1 || argc > 2 || !IsMatchArrayType(env, args[0])) {
        HiLog::Error(LABEL, "%{public}s the number of input parameters does not match", __func__);
        return nullptr;
    }
    AsyncCallbackInfo *asyncCallbackInfo = new AsyncCallbackInfo {
        .env = env,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .type = GET_DIRECTION,
    };
    std::vector<float> rotationMatrix = GetCppArrayFloat(env, args[0]);
    std::vector<float> rotationAngle(ROTATION_VECTOR_LENGTH);
    std::unique_ptr<SensorAlgorithm> sensorAlgorithm = std::make_unique<SensorAlgorithm>();
    int32_t ret = sensorAlgorithm->getDirection(rotationMatrix, rotationAngle);
    if (ret != OHOS::ERR_OK) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        asyncCallbackInfo->type = FAIL;
        asyncCallbackInfo->error.code = ret;
    } else {
        asyncCallbackInfo->data.reserveData.length = ROTATION_VECTOR_LENGTH;
        for (int32_t i = 0; i < ROTATION_VECTOR_LENGTH; i++) {
            asyncCallbackInfo->data.reserveData.reserve[i] = rotationAngle[i];
        }
    }
    if (argc == 1) {
        napi_deferred deferred = nullptr;
        napi_value promise = nullptr;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    if (!IsMatchType(env, args[1], napi_function)) {
        HiLog::Error(LABEL, "%{public}s argument should be napi_function type", __func__);
        napi_value result;
        napi_get_undefined(env, &result);
        delete asyncCallbackInfo;
        asyncCallbackInfo = nullptr;
        return result;
    }
    napi_create_reference(env, args[1], 1, &asyncCallbackInfo->callback[0]);
    EmitAsyncCallbackWork(asyncCallbackInfo);
    return nullptr;
}

static napi_value CreateQuaternion(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
    if (argc < 1 || argc > 2 || !IsMatchArrayType(env, args[0])) {
        HiLog::Error(LABEL, "%{public}s argument error", __func__);
        return nullptr;
    }
    AsyncCallbackInfo *asyncCallbackInfo = new AsyncCallbackInfo {
        .env = env,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .type = CREATE_QUATERNION,
    };
    std::vector<float> rotationVector = GetCppArrayFloat(env, args[0]);
    std::vector<float> quaternion(QUATERNION_LENGTH);
    std::unique_ptr<SensorAlgorithm> sensorAlgorithm = std::make_unique<SensorAlgorithm>();
    int32_t ret = sensorAlgorithm->createQuaternion(rotationVector, quaternion);
    if (ret != OHOS::ERR_OK) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        asyncCallbackInfo->type = FAIL;
        asyncCallbackInfo->error.code = ret;
    } else {
        asyncCallbackInfo->data.reserveData.length = QUATERNION_LENGTH;
        for (int32_t i = 0; i < QUATERNION_LENGTH; i++) {
            asyncCallbackInfo->data.reserveData.reserve[i] = quaternion[i];
        }
    }
    if (argc == 1) {
        napi_deferred deferred = nullptr;
        napi_value promise = nullptr;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    if (!IsMatchType(env, args[1], napi_function)) {
        HiLog::Error(LABEL, "%{public}s argument should be function", __func__);
        delete asyncCallbackInfo;
        asyncCallbackInfo = nullptr;
        return nullptr;
    }
    napi_create_reference(env, args[1], 1, &asyncCallbackInfo->callback[0]);
    EmitAsyncCallbackWork(asyncCallbackInfo);
    return nullptr;
}

static napi_value GetAltitude(napi_env env, napi_callback_info info)
{
    size_t argc = 3;
    napi_value args[3] = { 0 };
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
    if (argc < 2 || argc > 3 || !IsMatchType(env, args[0], napi_number)
        || !IsMatchType(env, args[1], napi_number)) {
        HiLog::Error(LABEL, "%{public}s Invalid input", __func__);
        return nullptr;
    }
    AsyncCallbackInfo *asyncCallbackInfo = new AsyncCallbackInfo {
        .env = env,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .type = GET_ALTITUDE,
    };
    float seaPressure = GetCppFloat(env, args[0]);
    float currentPressure = GetCppFloat(env, args[1]);
    float altitude = 0;
    std::unique_ptr<SensorAlgorithm> sensorAlgorithm = std::make_unique<SensorAlgorithm>();
    int32_t ret = sensorAlgorithm->getAltitude(seaPressure, currentPressure, &altitude);
    if (ret != OHOS::ERR_OK) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        asyncCallbackInfo->type = FAIL;
        asyncCallbackInfo->error.code = ret;
    } else {
        asyncCallbackInfo->data.reserveData.reserve[0] = altitude;
    }
    if (argc == 2) {
        napi_deferred deferred = nullptr;
        napi_value promise = nullptr;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    if (!IsMatchType(env, args[2], napi_function)) {
        HiLog::Error(LABEL, "%{public}s parameter should be napi_fouction type", __func__);
        delete asyncCallbackInfo;
        asyncCallbackInfo = nullptr;
        return nullptr;
    }
    napi_create_reference(env, args[2], 1, &asyncCallbackInfo->callback[0]);
    EmitAsyncCallbackWork(asyncCallbackInfo);
    return nullptr;
}

static napi_value GetGeomagneticDip(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
    if (argc < 1 || argc > 2 || !IsMatchArrayType(env, args[0])) {
        HiLog::Error(LABEL, "%{public}s Invalid input", __func__);
        return nullptr;
    }
    AsyncCallbackInfo *asyncCallbackInfo = new AsyncCallbackInfo {
        .env = env,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .type = GET_GEOMAGNITIC_DIP,
    };
    std::vector<float> inclinationMatrix = GetCppArrayFloat(env, args[0]);
    float geomagneticDip = 0;
    std::unique_ptr<SensorAlgorithm> sensorAlgorithm = std::make_unique<SensorAlgorithm>();
    int32_t ret = sensorAlgorithm->getGeomagneticDip(inclinationMatrix, &geomagneticDip);
    if (ret != OHOS::ERR_OK) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        asyncCallbackInfo->type = FAIL;
        asyncCallbackInfo->error.code = ret;
    } else {
        asyncCallbackInfo->data.reserveData.reserve[0] = geomagneticDip;
    }
    if (argc == 1) {
        napi_deferred deferred = nullptr;
        napi_value promise = nullptr;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    if (!IsMatchType(env, args[1], napi_function)) {
        HiLog::Error(LABEL, "%{public}s argument should be function", __func__);
        delete asyncCallbackInfo;
        asyncCallbackInfo = nullptr;
        return nullptr;
    }
    napi_create_reference(env, args[1], 1, &asyncCallbackInfo->callback[0]);
    EmitAsyncCallbackWork(asyncCallbackInfo);
    return nullptr;
}

static napi_value CreateRotationMatrix(napi_env env, napi_callback_info info)
{
    size_t argc = 3;
    napi_value args[3] = { 0 };
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, NULL));
    if (argc < 1 || argc > 3 || !IsMatchArrayType(env, args[0])) {
        HiLog::Info(LABEL, "%{public}s argument error", __func__);
        return nullptr;
    }
    AsyncCallbackInfo *asyncCallbackInfo = new AsyncCallbackInfo {
        .env = env,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .type = CREATE_ROTATION_MATRIX,
    };
    std::unique_ptr<SensorAlgorithm> sensorAlgorithm = std::make_unique<SensorAlgorithm>();
    if (argc == 1 || (argc == 2 && IsMatchType(env, args[1], napi_function))) {
        std::vector<float> rotationVector = GetCppArrayFloat(env, args[0]);
        std::vector<float> rotationMatrix(THREE_DIMENSIONAL_MATRIX_LENGTH);
        int32_t ret = sensorAlgorithm->createRotationMatrix(rotationVector, rotationMatrix);
        if (ret != OHOS::ERR_OK) {
            HiLog::Error(LABEL, "%{public}s failed", __func__);
            asyncCallbackInfo->type = FAIL;
            asyncCallbackInfo->error.code = ret;
        } else {
            asyncCallbackInfo->data.reserveData.length = THREE_DIMENSIONAL_MATRIX_LENGTH;
            for (int32_t i = 0; i < THREE_DIMENSIONAL_MATRIX_LENGTH; i++) {
                asyncCallbackInfo->data.reserveData.reserve[i] = rotationMatrix[i];
            }
        }
        if (argc == 1) {
            napi_deferred deferred = nullptr;
            napi_value promise = nullptr;
            NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
            asyncCallbackInfo->deferred = deferred;
            EmitPromiseWork(asyncCallbackInfo);
            return promise;
        }
        napi_create_reference(env, args[1], 1, &asyncCallbackInfo->callback[0]);
        EmitAsyncCallbackWork(asyncCallbackInfo);
        return nullptr;
    }

    if (!IsMatchArrayType(env, args[1]) || (argc == 3 && !IsMatchType(env, args[2], napi_function))) {
        HiLog::Info(LABEL, "%{public}s argument error", __func__);
        delete asyncCallbackInfo;
        asyncCallbackInfo = nullptr;
        return nullptr;
    }
    asyncCallbackInfo->type = ROTATION_INCLINATION_MATRIX;
    std::vector<float> gravity = GetCppArrayFloat(env, args[0]);
    std::vector<float> geomagnetic = GetCppArrayFloat(env, args[1]);
    std::vector<float> rotation(THREE_DIMENSIONAL_MATRIX_LENGTH);
    std::vector<float> inclination(THREE_DIMENSIONAL_MATRIX_LENGTH);
    int32_t ret = sensorAlgorithm->createRotationAndInclination(gravity, geomagnetic, rotation, inclination);
    if (ret != OHOS::ERR_OK) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        asyncCallbackInfo->type = FAIL;
        asyncCallbackInfo->error.code = ret;
    } else {
        for (int32_t i = 0; i < THREE_DIMENSIONAL_MATRIX_LENGTH; i++) {
            asyncCallbackInfo->data.rationMatrixData.rotationMatrix[i] = rotation[i];
        }
        for (int32_t i = 0; i < THREE_DIMENSIONAL_MATRIX_LENGTH; i++) {
            asyncCallbackInfo->data.rationMatrixData.inclinationMatrix[i] = inclination[i];
        }
    }
    if (argc == 2) {
        napi_deferred deferred = nullptr;
        napi_value promise = nullptr;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    napi_create_reference(env, args[2], 1, &asyncCallbackInfo->callback[0]);
    EmitAsyncCallbackWork(asyncCallbackInfo);
    return nullptr;
}

static napi_value GetSensorList(napi_env env, napi_callback_info info)
{
    HiLog::Info(LABEL, "%{public}s in", __func__);
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, NULL));
    if (argc != 1) {
        HiLog::Error(LABEL, "%{public}s the number of input parameters does not match", __func__);
        return nullptr;
    }
    SensorInfo *sensorInfos = nullptr;
    int32_t count = 0;
    AsyncCallbackInfo *asyncCallbackInfo = new AsyncCallbackInfo {
        .env = env,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .type = GET_SENSOR_LIST,
    };
    int32_t ret = GetAllSensors(&sensorInfos, &count);
    if (ret != OHOS::ERR_OK) {
        HiLog::Error(LABEL, "%{public}s get sensor list failed", __func__);
        asyncCallbackInfo->type = FAIL;
        asyncCallbackInfo->error.code = ret;
    } else {
        for (int32_t i = 0; i < count; i++) {
            asyncCallbackInfo->sensorInfos.push_back(*(sensorInfos + i));
        }
    }
    if (argc == 0) {
        napi_deferred deferred = nullptr;
        napi_value promise = nullptr;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    if (!IsMatchType(env, args[0], napi_function)) {
        HiLog::Error(LABEL, "%{public}s argument should be napi_function type", __func__);
        delete asyncCallbackInfo;
        asyncCallbackInfo = nullptr;
        return nullptr;
    }
    napi_create_reference(env, args[0], 1, &asyncCallbackInfo->callback[0]);
    EmitAsyncCallbackWork(asyncCallbackInfo);
    return nullptr;
}

static napi_value GetSingleSensor(napi_env env, napi_callback_info info)
{
    HiLog::Info(LABEL, "%{public}s in", __func__);
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, NULL));
    if (argc < 1 || argc > 2 || !IsMatchType(env, args[0], napi_number)) {
        HiLog::Error(LABEL, "%{public}s the number of input parameters does not match", __func__);
        return nullptr;
    }
    AsyncCallbackInfo *asyncCallbackInfo = new AsyncCallbackInfo {
        .env = env,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .type = GET_SINGLE_SENSOR,
    };
    SensorInfo *sensorInfos = nullptr;
    int32_t count = 0;
    int32_t ret = GetAllSensors(&sensorInfos, &count);
    if (ret != OHOS::ERR_OK) {
        HiLog::Error(LABEL, "%{public}s get sensorlist failed", __func__);
        asyncCallbackInfo->type = FAIL;
        asyncCallbackInfo->error.code = ret;
    } else {
        int32_t sensorTypeId = GetCppInt32(args[0], env);
        for (int32_t i = 0; i < count; i++) {
            if (sensorInfos[i].sensorTypeId == sensorTypeId) {
                asyncCallbackInfo->sensorInfos.push_back(*(sensorInfos + i));
                break;
            }
        }
        if (asyncCallbackInfo->sensorInfos.empty()) {
            HiLog::Error(LABEL, "%{public}s not find sensorTypeId: %{public}d", __func__, sensorTypeId);
            asyncCallbackInfo->type = FAIL;
        }
    }
    if (argc == 1) {
        napi_deferred deferred = nullptr;
        napi_value promise = nullptr;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    if (!IsMatchType(env, args[1], napi_function)) {
        HiLog::Error(LABEL, "%{public}s argument should be napi_function type", __func__);
        delete asyncCallbackInfo;
        asyncCallbackInfo = nullptr;
        return nullptr;
    }
    napi_create_reference(env, args[1], 1, &asyncCallbackInfo->callback[0]);
    EmitAsyncCallbackWork(asyncCallbackInfo);
    return nullptr;
}

static napi_value EnumClassConstructor(napi_env env, napi_callback_info info)
{
    size_t argc = 0;
    napi_value args[1] = {0};
    napi_value res = nullptr;
    void *data = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &res, &data);
    if (status != napi_ok) {
        return nullptr;
    }
    return res;
}

static napi_value CreateEnumSensorType(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_ACCELEROMETER", GetNapiInt32(SENSOR_TYPE_ID_ACCELEROMETER, env)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_GYROSCOPE", GetNapiInt32(SENSOR_TYPE_ID_GYROSCOPE, env)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_AMBIENT_LIGHT", GetNapiInt32(SENSOR_TYPE_ID_AMBIENT_LIGHT, env)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_MAGNETIC_FIELD", GetNapiInt32(SENSOR_TYPE_ID_MAGNETIC_FIELD, env)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_BAROMETER", GetNapiInt32(SENSOR_TYPE_ID_BAROMETER, env)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_HALL", GetNapiInt32(SENSOR_TYPE_ID_HALL, env)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_PROXIMITY", GetNapiInt32(SENSOR_TYPE_ID_PROXIMITY, env)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_HUMIDITY", GetNapiInt32(SENSOR_TYPE_ID_HUMIDITY, env)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_ORIENTATION", GetNapiInt32(SENSOR_TYPE_ID_ORIENTATION, env)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_GRAVITY", GetNapiInt32(SENSOR_TYPE_ID_GRAVITY, env)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_LINEAR_ACCELERATION",
            GetNapiInt32(SENSOR_TYPE_ID_LINEAR_ACCELERATION, env)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_ROTATION_VECTOR",
            GetNapiInt32(SENSOR_TYPE_ID_ROTATION_VECTOR, env)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_AMBIENT_TEMPERATURE",
            GetNapiInt32(SENSOR_TYPE_ID_AMBIENT_TEMPERATURE, env)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_MAGNETIC_FIELD_UNCALIBRATED",
            GetNapiInt32(SENSOR_TYPE_ID_MAGNETIC_FIELD_UNCALIBRATED, env)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_GYROSCOPE_UNCALIBRATED",
            GetNapiInt32(SENSOR_TYPE_ID_GYROSCOPE_UNCALIBRATED, env)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_SIGNIFICANT_MOTION",
            GetNapiInt32(SENSOR_TYPE_ID_SIGNIFICANT_MOTION, env)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_PEDOMETER_DETECTION",
            GetNapiInt32(SENSOR_TYPE_ID_PEDOMETER_DETECTION, env)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_PEDOMETER", GetNapiInt32(SENSOR_TYPE_ID_PEDOMETER, env)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_HEART_RATE", GetNapiInt32(SENSOR_TYPE_ID_HEART_RATE, env)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_WEAR_DETECTION", GetNapiInt32(SENSOR_TYPE_ID_WEAR_DETECTION, env)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_ACCELEROMETER_UNCALIBRATED",
            GetNapiInt32(SENSOR_TYPE_ID_ACCELEROMETER_UNCALIBRATED, env)),
    };
    napi_value result = nullptr;
    napi_define_class(env, "SensorType", NAPI_AUTO_LENGTH, EnumClassConstructor, nullptr,
        sizeof(desc) / sizeof(*desc), desc, &result);
    napi_set_named_property(env, exports, "SensorType", result);
    return exports;
}

EXTERN_C_START

static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("on", On),
        DECLARE_NAPI_FUNCTION("once", Once),
        DECLARE_NAPI_FUNCTION("off", Off),
        DECLARE_NAPI_FUNCTION("getGeomagneticField", GetGeomagneticField),
        DECLARE_NAPI_FUNCTION("transformCoordinateSystem", TransformCoordinateSystem),
        DECLARE_NAPI_FUNCTION("getAngleModify", GetAngleModify),
        DECLARE_NAPI_FUNCTION("getDirection", GetDirection),
        DECLARE_NAPI_FUNCTION("createQuaternion", CreateQuaternion),
        DECLARE_NAPI_FUNCTION("getAltitude", GetAltitude),
        DECLARE_NAPI_FUNCTION("getGeomagneticDip", GetGeomagneticDip),
        DECLARE_NAPI_FUNCTION("createRotationMatrix", CreateRotationMatrix),
        DECLARE_NAPI_FUNCTION("getSensorList", GetSensorList),
        DECLARE_NAPI_FUNCTION("getSingleSensor", GetSingleSensor),
        DECLARE_NAPI_FUNCTION("subscribeAccelerometer", SubscribeAccelerometer),
        DECLARE_NAPI_FUNCTION("unsubscribeAccelerometer", UnsubscribeAccelerometer),
        DECLARE_NAPI_FUNCTION("subscribeCompass", SubscribeCompass),
        DECLARE_NAPI_FUNCTION("unsubscribeCompass", UnsubscribeCompass),
        DECLARE_NAPI_FUNCTION("subscribeProximity", SubscribeProximity),
        DECLARE_NAPI_FUNCTION("unsubscribeProximity", UnsubscribeProximity),
        DECLARE_NAPI_FUNCTION("subscribeLight", SubscribeLight),
        DECLARE_NAPI_FUNCTION("unsubscribeLight", UnsubscribeLight),
        DECLARE_NAPI_FUNCTION("subscribeStepCounter", SubscribeStepCounter),
        DECLARE_NAPI_FUNCTION("unsubscribeStepCounter", UnsubscribeStepCounter),
        DECLARE_NAPI_FUNCTION("subscribeBarometer", SubscribeBarometer),
        DECLARE_NAPI_FUNCTION("unsubscribeBarometer", UnsubscribeBarometer),
        DECLARE_NAPI_FUNCTION("subscribeHeartRate", SubscribeHeartRate),
        DECLARE_NAPI_FUNCTION("unsubscribeHeartRate", UnsubscribeHeartRate),
        DECLARE_NAPI_FUNCTION("subscribeOnBodyState", SubscribeOnBodyState),
        DECLARE_NAPI_FUNCTION("unsubscribeOnBodyState", UnsubscribeOnBodyState),
        DECLARE_NAPI_FUNCTION("getOnBodyState", GetOnBodyState),
        DECLARE_NAPI_FUNCTION("subscribeDeviceOrientation", SubscribeDeviceOrientation),
        DECLARE_NAPI_FUNCTION("unsubscribeDeviceOrientation", UnsubscribeDeviceOrientation),
        DECLARE_NAPI_FUNCTION("subscribeGyroscope", SubscribeGyroscope),
        DECLARE_NAPI_FUNCTION("unsubscribeGyroscope", UnsubscribeGyroscope),
        DECLARE_NAPI_FUNCTION("subscribeGravity", SubscribeGravity),
        DECLARE_NAPI_FUNCTION("unsubscribeGravity", UnsubscribeGravity),
        DECLARE_NAPI_FUNCTION("subscribeMagnetic", SubscribeMagnetic),
        DECLARE_NAPI_FUNCTION("unsubscribeMagnetic", UnsubscribeMagnetic),
        DECLARE_NAPI_FUNCTION("subscribeHall", SubscribeHall),
        DECLARE_NAPI_FUNCTION("unsubscribeHall", UnsubscribeHall),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(napi_property_descriptor), desc));
    CreateEnumSensorType(env, exports);
    return exports;
}
EXTERN_C_END

static napi_module _module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = NULL,
    .nm_register_func = Init,
    .nm_modname = "sensor",
    .nm_priv = ((void *)0),
    .reserved = {0}
};

extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&_module);
}
