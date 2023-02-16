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
#include "refbase.h"
#include "securec.h"
#include "sensor_algorithm.h"
#include "sensor_napi_utils.h"
#include "sensor_system_js.h"
namespace OHOS {
namespace Sensors {
namespace {
constexpr int32_t QUATERNION_LENGTH = 4;
constexpr int32_t ROTATION_VECTOR_LENGTH = 3;
constexpr int32_t REPORTING_INTERVAL = 200000000;
constexpr int32_t INVALID_SENSOR_ID = -1;
constexpr int32_t SENSOR_SUBSCRIBE_FAILURE = 1001;
constexpr int32_t INPUT_ERROR = 202;
constexpr float BODY_STATE_EXCEPT = 1.0f;
constexpr float THREESHOLD = 0.000001f;
}
static std::map<std::string, int64_t> g_samplingPeriod = {
    {"normal", 200000000},
    {"ui", 60000000},
    {"game", 20000000},
};
static std::mutex mutex_;
static std::mutex bodyMutex_;
static float g_bodyState = -1.0f;
static std::map<int32_t, sptr<AsyncCallbackInfo>> g_subscribeCallbacks;
static std::mutex onMutex_;
static std::mutex onceMutex_;
static std::map<int32_t, std::vector<sptr<AsyncCallbackInfo>>> g_onceCallbackInfos;
static std::map<int32_t, std::vector<sptr<AsyncCallbackInfo>>> g_onCallbackInfos;

static bool CheckSubscribe(int32_t sensorTypeId)
{
    std::lock_guard<std::mutex> onCallbackLock(onMutex_);
    auto iter = g_onCallbackInfos.find(sensorTypeId);
    CHKCF((iter != g_onCallbackInfos.end()), "No client subscribe");
    return true;
}

static bool copySensorData(sptr<AsyncCallbackInfo> callbackInfo, SensorEvent *event)
{
    CHKPF(callbackInfo);
    CHKPF(event);
    int32_t sensorTypeId = event->sensorTypeId;
    callbackInfo->data.sensorData.sensorTypeId = sensorTypeId;
    callbackInfo->data.sensorData.dataLength = event->dataLen;
    callbackInfo->data.sensorData.timestamp = event->timestamp;
    auto data = reinterpret_cast<float *>(event->data);
    if (sensorTypeId == SENSOR_TYPE_ID_WEAR_DETECTION && callbackInfo->type == SUBSCRIBE_CALLBACK) {
        std::lock_guard<std::mutex> onBodyLock(bodyMutex_);
        g_bodyState = *data;
        callbackInfo->data.sensorData.data[0] =
            (fabs(g_bodyState - BODY_STATE_EXCEPT) < THREESHOLD) ? true : false;
        return true;
    }
    CHKPF(data);
    if (memcpy_s(callbackInfo->data.sensorData.data, event->dataLen, data, event->dataLen) != EOK) {
        SEN_HILOGE("Copy data failed");
        return false;
    }
    return true;
}

static bool CheckSystemSubscribe(int32_t sensorTypeId)
{
    std::lock_guard<std::mutex> subscribeLock(mutex_);
    auto iter = g_subscribeCallbacks.find(sensorTypeId);
    CHKCF((iter != g_subscribeCallbacks.end()), "No client subscribe");
    return true;
}

static void EmitSubscribeCallback(SensorEvent *event)
{
    CHKPV(event);
    int32_t sensorTypeId = event->sensorTypeId;
    CHKCV(CheckSystemSubscribe(sensorTypeId), "No client subscribe");

    std::lock_guard<std::mutex> subscribeLock(mutex_);
    auto callback = g_subscribeCallbacks[sensorTypeId];
    CHKCV(copySensorData(callback, event), "Copy sensor data failed");
    EmitUvEventLoop(callback);
}

static void EmitOnCallback(SensorEvent *event)
{
    CHKPV(event);
    int32_t sensorTypeId = event->sensorTypeId;
    CHKCV(CheckSubscribe(sensorTypeId), "No client subscribe");

    std::lock_guard<std::mutex> onCallbackLock(onMutex_);
    auto onCallbackInfos = g_onCallbackInfos[sensorTypeId];
    for (auto &onCallbackInfo : onCallbackInfos) {
        if (!copySensorData(onCallbackInfo, event)) {
            SEN_HILOGE("Copy sensor data failed");
            continue;
        }
        EmitUvEventLoop(onCallbackInfo);
    }
}

static void EmitOnceCallback(SensorEvent *event)
{
    CHKPV(event);
    int32_t sensorTypeId = event->sensorTypeId;
    std::lock_guard<std::mutex> onceCallbackLock(onceMutex_);
    auto iter = g_onceCallbackInfos.find(sensorTypeId);
    CHKCV((iter != g_onceCallbackInfos.end()), "No client subscribe once");

    auto onceCallbackInfos = g_onceCallbackInfos[sensorTypeId];
    for (auto &onceCallbackInfo : onceCallbackInfos) {
        if (!copySensorData(onceCallbackInfo, event)) {
            SEN_HILOGE("Copy sensor data failed");
            continue;
        }
        EmitUvEventLoop(onceCallbackInfo);
    }
    g_onceCallbackInfos[sensorTypeId].clear();
    g_onceCallbackInfos.erase(sensorTypeId);

    CHKCV((!CheckSubscribe(sensorTypeId)), "Has client subscribe, not need cancel subscribe");
    CHKCV((!CheckSystemSubscribe(sensorTypeId)), "Has client subscribe system api, not need cancel subscribe");
    UnsubscribeSensor(sensorTypeId);
}

void DataCallbackImpl(SensorEvent *event)
{
    CHKPV(event);
    EmitOnCallback(event);
    EmitSubscribeCallback(event);
    EmitOnceCallback(event);
}

const SensorUser user = {
    .callback = DataCallbackImpl
};

bool UnsubscribeSensor(int32_t sensorTypeId)
{
    CALL_LOG_ENTER;
    CHKCF((DeactivateSensor(sensorTypeId, &user) == SUCCESS), "DeactivateSensor failed");
    CHKCF((UnsubscribeSensor(sensorTypeId, &user) == SUCCESS), "UnsubscribeSensor failed");
    return true;
}

bool SubscribeSensor(int32_t sensorTypeId, int64_t interval, RecordSensorCallback callback)
{
    CALL_LOG_ENTER;
    CHKCF((SubscribeSensor(sensorTypeId, &user) == ERR_OK), "SubscribeSensor failed");
    CHKCF((SetBatch(sensorTypeId, &user, interval, 0) == ERR_OK), "Set batch failed");
    CHKCF((ActivateSensor(sensorTypeId, &user) == ERR_OK), "ActivateSensor failed");
    return true;
}

static bool IsOnceSubscribed(napi_env env, int32_t sensorTypeId, napi_value callback)
{
    CALL_LOG_ENTER;
    if (auto iter = g_onceCallbackInfos.find(sensorTypeId); iter == g_onceCallbackInfos.end()) {
        SEN_HILOGW("already subscribed, sensorTypeId: %{public}d", sensorTypeId);
        return false;
    }

    std::vector<sptr<AsyncCallbackInfo>> callbackInfos = g_onceCallbackInfos[sensorTypeId];
    for (auto callbackInfo : callbackInfos) {
        CHKNCC(env, (callbackInfo != nullptr), "callbackInfo is null");
        napi_value sensorCallback = nullptr;
        CHKNRF(env, napi_get_reference_value(env, callbackInfo->callback[0], &sensorCallback),
            "napi_get_reference_value");
        if (IsSameValue(env, callback, sensorCallback)) {
            return true;
        }
    }
    return false;
}

static void UpdateOnceCallback(napi_env env, int32_t sensorTypeId, napi_value callback)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> onceCallbackLock(onceMutex_);
    CHKNCV(env, !IsOnceSubscribed(env, sensorTypeId, callback), "The callback has been subscribed");

    sptr<AsyncCallbackInfo> asyncCallbackInfo = new (std::nothrow) AsyncCallbackInfo(env, ONCE_CALLBACK);
    CHKPV(asyncCallbackInfo);
    CHKNRV(env, napi_create_reference(env, callback, 1, &asyncCallbackInfo->callback[0]),
        "napi_create_reference");
    std::vector<sptr<AsyncCallbackInfo>> callbackInfos = g_onceCallbackInfos[sensorTypeId];
    callbackInfos.push_back(asyncCallbackInfo);
    g_onceCallbackInfos[sensorTypeId] = callbackInfos;
}

static napi_value Once(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_value thisVar = nullptr;
    CHKNRP(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr), "napi_get_cb_info");
    CHKNCP(env, (argc == 2), "The number of parameters is not valid");
    CHKNCP(env, IsMatchType(env, args[0], napi_number), "Wrong argument type, should be number");
    CHKNCP(env, IsMatchType(env, args[1], napi_function), "Wrong argument type, should be function");
    int32_t sensorTypeId = INVALID_SENSOR_ID;
    CHKNCP(env, GetCppInt32(env, args[0], sensorTypeId), "Wrong argument type, get number fail");

    if (!CheckSubscribe(sensorTypeId)) {
        SEN_HILOGD("No subscription to change sensor data, registration is required");
        CHKNCP(env, SubscribeSensor(sensorTypeId, REPORTING_INTERVAL, DataCallbackImpl), "SubscribeSensor failed");
    }
    UpdateOnceCallback(env, sensorTypeId, args[1]);
    return nullptr;
}

static bool IsSubscribed(napi_env env, int32_t sensorTypeId, napi_value callback)
{
    CALL_LOG_ENTER;
    if (auto iter = g_onCallbackInfos.find(sensorTypeId); iter == g_onCallbackInfos.end()) {
        SEN_HILOGW("no client subscribe, sensorTypeId: %{public}d", sensorTypeId);
        return false;
    }
    std::vector<sptr<AsyncCallbackInfo>> callbackInfos = g_onCallbackInfos[sensorTypeId];
    for (auto callbackInfo : callbackInfos) {
        CHKNCC(env, (callbackInfo != nullptr), "callbackInfo is null");
        napi_value sensorCallback = nullptr;
        CHKNRF(env, napi_get_reference_value(env, callbackInfo->callback[0], &sensorCallback),
            "napi_get_reference_value");
        if (IsSameValue(env, callback, sensorCallback)) {
            return true;
        }
    }
    return false;
}

static void UpdateCallbackInfos(napi_env env, int32_t sensorTypeId, napi_value callback)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> onCallbackLock(onMutex_);
    CHKNCV(env, !IsSubscribed(env, sensorTypeId, callback), "This callback has been subscribed");
    sptr<AsyncCallbackInfo> asyncCallbackInfo = new (std::nothrow) AsyncCallbackInfo(env, ON_CALLBACK);
    CHKPV(asyncCallbackInfo);
    CHKNRV(env, napi_create_reference(env, callback, 1, &asyncCallbackInfo->callback[0]),
        "napi_create_reference");
    std::vector<sptr<AsyncCallbackInfo>> callbackInfos = g_onCallbackInfos[sensorTypeId];
    callbackInfos.push_back(asyncCallbackInfo);
    g_onCallbackInfos[sensorTypeId] = callbackInfos;
}

static napi_value On(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 3;
    napi_value args[3] = { 0 };
    napi_value thisVar = nullptr;
    CHKNRP(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr), "napi_get_cb_info");
    CHKNCP(env, ((argc == 2) || (argc == 3)), "The number of parameters is not valid");
    CHKNCP(env, IsMatchType(env, args[0], napi_number), "Wrong argument type, should be number");
    CHKNCP(env, IsMatchType(env, args[1], napi_function), "Wrong argument type, should be function");

    int32_t sensorTypeId = INVALID_SENSOR_ID;
    CHKNCP(env, GetCppInt32(env, args[0], sensorTypeId), "Wrong argument type, get number fail");
    int64_t interval = REPORTING_INTERVAL;

    if (argc == 3) {
        napi_value value = GetNamedProperty(env, args[2], "interval");
        CHKNCP(env, IsMatchType(env, value, napi_number), "Wrong argument type, interval should be number");
        CHKNCP(env, GetCppInt64(env, value, interval), "Wrong argument type, get INT64 number fail");
        SEN_HILOGD("Interval is %{public}" PRId64, interval);
    }
    CHKNCP(env, SubscribeSensor(sensorTypeId, interval, DataCallbackImpl), "SubscribeSensor failed");
    UpdateCallbackInfos(env, sensorTypeId, args[1]);
    return nullptr;
}

static void RemoveAllCallback(napi_env env, int32_t sensorTypeId)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> onCallbackLock(onMutex_);
    g_onCallbackInfos[sensorTypeId].clear();
    g_onCallbackInfos.erase(sensorTypeId);
}

static uint32_t RemoveCallback(napi_env env, int32_t sensorTypeId, napi_value callback)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> onCallbackLock(onMutex_);
    std::vector<sptr<AsyncCallbackInfo>> callbackInfos = g_onCallbackInfos[sensorTypeId];
    for (auto iter = callbackInfos.begin(); iter != callbackInfos.end(); ++iter) {
        CHKPC(*iter);
        napi_value sensorCallback = nullptr;
        CHKNRC(env, napi_get_reference_value(env, (*iter)->callback[0], &sensorCallback),
            "napi_get_reference_value");
        if (IsSameValue(env, callback, sensorCallback)) {
            callbackInfos.erase(iter++);
            SEN_HILOGD("Remove callback success");
            break;
        }
    }
    if (callbackInfos.empty()) {
        SEN_HILOGD("No subscription to change sensor data");
        g_onCallbackInfos.erase(sensorTypeId);
        return 0;
    }
    g_onCallbackInfos[sensorTypeId] = callbackInfos;
    return callbackInfos.size();
}

static napi_value Off(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_value thisVar = nullptr;
    CHKNRP(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr), "napi_get_cb_info");
    CHKNCP(env, ((argc == 1) || (argc == 2)), "The number of parameters is not valid");
    CHKNCP(env, IsMatchType(env, args[0], napi_number), "Wrong argument type, should be number");
    int32_t sensorTypeId = INVALID_SENSOR_ID;
    CHKNCP(env, GetCppInt32(env, args[0], sensorTypeId), "Wrong argument type, get number fail");
    CHKNCP(env, (CheckSubscribe(sensorTypeId)), "Should subscribe first");

    if (argc == 1) {
        RemoveAllCallback(env, sensorTypeId);
    } else {
        CHKNCP(env, IsMatchType(env, args[1], napi_function), "Wrong argument type, should be function");
        CHKNCP(env, (RemoveCallback(env, sensorTypeId, args[1]) == 0),
            "There are other client subscribe as well, not need unsubscribe");
    }
    if (CheckSystemSubscribe(sensorTypeId)) {
        SEN_HILOGW("There are other client subscribe system js api as well, not need unsubscribe");
        return nullptr;
    }
    CHKNCP(env, UnsubscribeSensor(sensorTypeId), "UnsubscribeSensor failed");
    return nullptr;
}

static napi_value GetGeomagneticField(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 3;
    napi_value args[3] = { 0 };
    napi_value thisVar = nullptr;
    CHKNRP(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr), "napi_get_cb_info");
    CHKNCP(env, ((argc == 2) || (argc == 3)), "The number of parameters is not valid");
    CHKNCP(env, IsMatchType(env, args[0], napi_object), "Wrong argument type, should be object");
    CHKNCP(env, IsMatchType(env, args[1], napi_number), "Wrong argument type, should be number");

    napi_value napiLatitude = GetNamedProperty(env, args[0], "latitude");
    CHKNCP(env, (napiLatitude != nullptr), "napiLatitude is null");
    double latitude = 0;
    CHKNCP(env, GetCppDouble(env, napiLatitude, latitude), "Get latitude fail");
    napi_value napiLongitude = GetNamedProperty(env, args[0], "longitude");
    CHKNCP(env, (napiLongitude != nullptr), "napiLongitude is null");
    double longitude = 0;
    CHKNCP(env, GetCppDouble(env, napiLongitude, longitude), "Get longitude fail");
    napi_value napiAltitude = GetNamedProperty(env, args[0], "altitude");
    CHKNCP(env, (napiAltitude != nullptr), "napiAltitude is null");
    double altitude = 0;
    CHKNCP(env, GetCppDouble(env, napiAltitude, altitude), "Get altitude fail");
    int64_t timeMillis = 0;
    CHKNCP(env, GetCppInt64(env, args[1], timeMillis), "Get timeMillis fail");

    GeomagneticField geomagneticField(latitude, longitude, altitude, timeMillis);
    sptr<AsyncCallbackInfo> asyncCallbackInfo =
        new (std::nothrow) AsyncCallbackInfo(env, GET_GEOMAGNETIC_FIELD);
    CHKPP(asyncCallbackInfo);
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
        napi_value promise = nullptr;
        CHKNRP(env, napi_create_promise(env, &asyncCallbackInfo->deferred, &promise),
            "napi_create_promise");
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    CHKNCP(env, IsMatchType(env, args[2], napi_function), "Wrong argument type, should be function");
    CHKNRP(env, napi_create_reference(env, args[2], 1, &asyncCallbackInfo->callback[0]),
        "napi_create_reference");
    EmitAsyncCallbackWork(asyncCallbackInfo);
    return nullptr;
}

static napi_value TransformCoordinateSystem(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 3;
    napi_value args[3]  = { 0 };
    napi_value thisVar = nullptr;
    CHKNRP(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr), "napi_get_cb_info");
    CHKNCP(env, ((argc == 2) || (argc == 3)), "The number of parameters is not valid");
    CHKNCP(env, IsMatchArrayType(env, args[0]), "Wrong argument type, should be array");
    CHKNCP(env, IsMatchType(env, args[1], napi_object), "Wrong argument type, should be object");

    std::vector<float> inRotationVector;
    CHKNCP(env, GetFloatArray(env, args[0], inRotationVector), "Wrong argument type, get inRotationVector fail");
    napi_value napiAxisX = GetNamedProperty(env, args[1], "x");
    CHKNCP(env, (napiAxisX != nullptr), "napiAxisX is null");
    int32_t axisX = 0;
    CHKNCP(env, GetCppInt32(env, napiAxisX, axisX), "Get axisY fail");
    napi_value napiAxisY = GetNamedProperty(env, args[1], "y");
    CHKNCP(env, (napiAxisY != nullptr), "napiAxisY is null");
    int32_t axisY = 0;
    CHKNCP(env, GetCppInt32(env, napiAxisY, axisY), "Get axisY fail");

    sptr<AsyncCallbackInfo> asyncCallbackInfo =
        new (std::nothrow) AsyncCallbackInfo(env, TRANSFORM_COORDINATE_SYSTEM);
    CHKPP(asyncCallbackInfo);
    size_t length = inRotationVector.size();
    std::vector<float> outRotationVector(length);
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.transformCoordinateSystem(inRotationVector, axisX, axisY, outRotationVector);
    if (ret != OHOS::ERR_OK) {
        SEN_HILOGE("Transform coordinate system fail");
        asyncCallbackInfo->type = FAIL;
        asyncCallbackInfo->error.code = ret;
    } else {
        for (size_t i = 0; i < length; ++i) {
            asyncCallbackInfo->data.reserveData.reserve[i] = outRotationVector[i];
        }
        asyncCallbackInfo->data.reserveData.length = static_cast<int32_t>(length);
    }
    if (argc == 2) {
        napi_value promise = nullptr;
        CHKNRP(env, napi_create_promise(env, &asyncCallbackInfo->deferred, &promise), "napi_create_promise");
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    CHKNCP(env, IsMatchType(env, args[2], napi_function), "Wrong argument type, should be function");
    CHKNRP(env, napi_create_reference(env, args[2], 1, &asyncCallbackInfo->callback[0]),
        "napi_create_reference");
    EmitAsyncCallbackWork(asyncCallbackInfo);
    return nullptr;
}

static napi_value GetAngleModify(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 3;
    napi_value args[3] = { 0 };
    napi_value thisVar = nullptr;
    CHKNRP(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr), "napi_get_cb_info");
    CHKNCP(env, ((argc == 2) || (argc == 3)), "The number of parameters is not valid");
    CHKNCP(env, IsMatchArrayType(env, args[0]), "Wrong argument type, the first parameter should be array");
    CHKNCP(env, IsMatchArrayType(env, args[1]), "Wrong argument type, the second parameter should be array");

    sptr<AsyncCallbackInfo> asyncCallbackInfo =
        new (std::nothrow) AsyncCallbackInfo(env, GET_ANGLE_MODIFY);
    CHKPP(asyncCallbackInfo);
    std::vector<float> curRotationVector;
    CHKNCP(env, GetFloatArray(env, args[0], curRotationVector), "Wrong argument type, get curRotationVector fail");
    std::vector<float> preRotationVector;
    CHKNCP(env, GetFloatArray(env, args[1], preRotationVector), "Wrong argument type, get preRotationVector fail");

    std::vector<float> angleChange(ROTATION_VECTOR_LENGTH);
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.getAngleModify(curRotationVector, preRotationVector, angleChange);
    if (ret != OHOS::ERR_OK) {
        SEN_HILOGE("Get angle modify fail");
        asyncCallbackInfo->type = FAIL;
        asyncCallbackInfo->error.code = ret;
    } else {
        asyncCallbackInfo->data.reserveData.length = ROTATION_VECTOR_LENGTH;
        for (int32_t i = 0; i < ROTATION_VECTOR_LENGTH; ++i) {
            asyncCallbackInfo->data.reserveData.reserve[i] = angleChange[i];
        }
    }
    if (argc == 2) {
        napi_value promise = nullptr;
        CHKNRP(env, napi_create_promise(env, &asyncCallbackInfo->deferred, &promise),
            "napi_create_promise");
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    CHKNCP(env, IsMatchType(env, args[2], napi_function), "Wrong argument type, should be function");
    CHKNRP(env, napi_create_reference(env, args[2], 1, &asyncCallbackInfo->callback[0]),
        "napi_create_reference");
    EmitAsyncCallbackWork(asyncCallbackInfo);
    return nullptr;
}

static napi_value GetDirection(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 3;
    napi_value args[3] = { 0 };
    napi_value thisVar = nullptr;
    CHKNRP(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr), "napi_get_cb_info fail");
    CHKNCP(env, ((argc == 1) || (argc == 2)), "The number of parameters is not valid");
    CHKNCP(env, IsMatchArrayType(env, args[0]), "Wrong argument type, should be array");

    sptr<AsyncCallbackInfo> asyncCallbackInfo =
        new (std::nothrow) AsyncCallbackInfo(env, GET_DIRECTION);
    CHKPP(asyncCallbackInfo);
    std::vector<float> rotationMatrix;
    CHKNCP(env, GetFloatArray(env, args[0], rotationMatrix), "Wrong argument type, get rotationMatrix fail");
    std::vector<float> rotationAngle(ROTATION_VECTOR_LENGTH);
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.getDirection(rotationMatrix, rotationAngle);
    if (ret != OHOS::ERR_OK) {
        SEN_HILOGE("Get direction fail");
        asyncCallbackInfo->type = FAIL;
        asyncCallbackInfo->error.code = ret;
    } else {
        asyncCallbackInfo->data.reserveData.length = ROTATION_VECTOR_LENGTH;
        for (int32_t i = 0; i < ROTATION_VECTOR_LENGTH; ++i) {
            asyncCallbackInfo->data.reserveData.reserve[i] = rotationAngle[i];
        }
    }
    if (argc == 1) {
        napi_value promise = nullptr;
        CHKNRP(env, napi_create_promise(env, &asyncCallbackInfo->deferred, &promise),
            "napi_create_promise");
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    CHKNCP(env, IsMatchType(env, args[1], napi_function), "Wrong argument type, should be function");
    CHKNRP(env, napi_create_reference(env, args[1], 1, &asyncCallbackInfo->callback[0]),
        "napi_create_reference");
    EmitAsyncCallbackWork(asyncCallbackInfo);
    return nullptr;
}

static napi_value CreateQuaternion(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_value thisVar = nullptr;
    CHKNRP(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr), "napi_get_cb_info");
    CHKNCP(env, ((argc == 1) || (argc == 2)), "The number of parameters is not valid");
    CHKNCP(env, IsMatchArrayType(env, args[0]), "Wrong argument type, should be array");

    sptr<AsyncCallbackInfo> asyncCallbackInfo =
        new (std::nothrow) AsyncCallbackInfo(env, CREATE_QUATERNION);
    CHKPP(asyncCallbackInfo);
    std::vector<float> rotationVector;
    CHKNCP(env, GetFloatArray(env, args[0], rotationVector),
        "Wrong argument type, get rotationVector fail");
    std::vector<float> quaternion(QUATERNION_LENGTH);
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.createQuaternion(rotationVector, quaternion);
    if (ret != OHOS::ERR_OK) {
        SEN_HILOGE("Create quaternin fail");
        asyncCallbackInfo->type = FAIL;
        asyncCallbackInfo->error.code = ret;
    } else {
        asyncCallbackInfo->data.reserveData.length = QUATERNION_LENGTH;
        for (int32_t i = 0; i < QUATERNION_LENGTH; ++i) {
            asyncCallbackInfo->data.reserveData.reserve[i] = quaternion[i];
        }
    }
    if (argc == 1) {
        napi_value promise = nullptr;
        CHKNRP(env, napi_create_promise(env, &asyncCallbackInfo->deferred, &promise),
            "napi_create_promise");
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    CHKNCP(env, IsMatchType(env, args[1], napi_function), "Wrong argument type, should be function");
    CHKNRP(env, napi_create_reference(env, args[1], 1, &asyncCallbackInfo->callback[0]),
        "napi_create_reference");
    EmitAsyncCallbackWork(asyncCallbackInfo);
    return nullptr;
}

static napi_value GetAltitude(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 3;
    napi_value args[3] = { 0 };
    napi_value thisVar = nullptr;
    CHKNRP(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr), "napi_get_cb_info");
    CHKNCP(env, ((argc == 2) || (argc == 3)), "The number of parameters is not valid");
    CHKNCP(env, IsMatchType(env, args[0], napi_number), "Wrong argument type, should be number");
    CHKNCP(env, IsMatchType(env, args[1], napi_number), "Wrong argument type, should be number");

    sptr<AsyncCallbackInfo> asyncCallbackInfo =
        new (std::nothrow) AsyncCallbackInfo(env, GET_ALTITUDE);
    CHKPP(asyncCallbackInfo);
    float seaPressure = 0;
    CHKNCP(env, GetCppFloat(env, args[0], seaPressure), "Wrong argument type, get seaPressure fail");
    float currentPressure = 0;
    CHKNCP(env, GetCppFloat(env, args[1], currentPressure), "Wrong argument type, get currentPressure fail");
    float altitude = 0;
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.getAltitude(seaPressure, currentPressure, &altitude);
    if (ret != OHOS::ERR_OK) {
        SEN_HILOGE("Get altitude fail");
        asyncCallbackInfo->type = FAIL;
        asyncCallbackInfo->error.code = ret;
    } else {
        asyncCallbackInfo->data.reserveData.reserve[0] = altitude;
    }
    if (argc == 2) {
        napi_value promise = nullptr;
        CHKNRP(env, napi_create_promise(env, &asyncCallbackInfo->deferred, &promise),
            "napi_create_promise");
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    CHKNCP(env, IsMatchType(env, args[2], napi_function), "Wrong argument type, should be function");
    CHKNRP(env, napi_create_reference(env, args[2], 1, &asyncCallbackInfo->callback[0]),
        "napi_create_reference");
    EmitAsyncCallbackWork(asyncCallbackInfo);
    return nullptr;
}

static napi_value GetGeomagneticDip(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_value thisVar = nullptr;
    CHKNRP(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr), "napi_get_cb_info");
    CHKNCP(env, ((argc == 1) || (argc == 2)), "The number of parameters is not valid");
    CHKNCP(env, IsMatchArrayType(env, args[0]), "Wrong argument type, should be array");

    sptr<AsyncCallbackInfo> asyncCallbackInfo =
        new (std::nothrow) AsyncCallbackInfo(env, GET_GEOMAGNITIC_DIP);
    CHKPP(asyncCallbackInfo);
    std::vector<float> inclinationMatrix;
    CHKNCP(env, GetFloatArray(env, args[0], inclinationMatrix),
        "Wrong argument type, get inclinationMatrix fail");
    float geomagneticDip = 0;
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.getGeomagneticDip(inclinationMatrix, &geomagneticDip);
    if (ret != OHOS::ERR_OK) {
        SEN_HILOGE("Get geomagnetic dip fail");
        asyncCallbackInfo->type = FAIL;
        asyncCallbackInfo->error.code = ret;
    } else {
        asyncCallbackInfo->data.reserveData.reserve[0] = geomagneticDip;
    }
    if (argc == 1) {
        napi_value promise = nullptr;
        CHKNRP(env, napi_create_promise(env, &asyncCallbackInfo->deferred, &promise),
            "napi_create_promise");
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    CHKNCP(env, IsMatchType(env, args[1], napi_function), "Wrong argument type, should be function");
    CHKNRP(env, napi_create_reference(env, args[1], 1, &asyncCallbackInfo->callback[0]),
        "napi_create_reference");
    EmitAsyncCallbackWork(asyncCallbackInfo);
    return nullptr;
}

static napi_value createRotationAndInclination(const napi_env &env, napi_value args[], size_t argc)
{
    CALL_LOG_ENTER;
    CHKNCP(env, ((argc == 2) || (argc == 3)), "The number of parameters is not valid");
    std::vector<float> gravity;
    CHKNCP(env, GetFloatArray(env, args[0], gravity), "Wrong argument type, get gravity fail");
    std::vector<float> geomagnetic;
    CHKNCP(env, GetFloatArray(env, args[1], geomagnetic), "Wrong argument type, get geomagnetic fail");
    std::vector<float> rotation(THREE_DIMENSIONAL_MATRIX_LENGTH);
    std::vector<float> inclination(THREE_DIMENSIONAL_MATRIX_LENGTH);
    sptr<AsyncCallbackInfo> asyncCallbackInfo =
        new (std::nothrow) AsyncCallbackInfo(env, ROTATION_INCLINATION_MATRIX);
    CHKPP(asyncCallbackInfo);
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.createRotationAndInclination(gravity, geomagnetic, rotation, inclination);
    if (ret != OHOS::ERR_OK) {
        SEN_HILOGE("Create rotation and inclination matrix fail");
        asyncCallbackInfo->type = FAIL;
        asyncCallbackInfo->error.code = ret;
    } else {
        asyncCallbackInfo->data.reserveData.length = THREE_DIMENSIONAL_MATRIX_LENGTH;
        for (int32_t i = 0; i < THREE_DIMENSIONAL_MATRIX_LENGTH; ++i) {
            asyncCallbackInfo->data.rationMatrixData.rotationMatrix[i] = rotation[i];
        }
        for (int32_t i = 0; i < THREE_DIMENSIONAL_MATRIX_LENGTH; ++i) {
            asyncCallbackInfo->data.rationMatrixData.inclinationMatrix[i] = inclination[i];
        }
    }
    if (argc == 2) {
        napi_value promise = nullptr;
        CHKNRP(env, napi_create_promise(env, &asyncCallbackInfo->deferred, &promise),
            "napi_create_promise");
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    CHKNCP(env, IsMatchType(env, args[2], napi_function), "Wrong argument type, should be function");
    CHKNRP(env, napi_create_reference(env, args[2], 1, &asyncCallbackInfo->callback[0]),
        "napi_create_reference");
    EmitAsyncCallbackWork(asyncCallbackInfo);
    return nullptr;
}

static napi_value GetRotationMatrix(const napi_env &env, napi_value args[], size_t argc)
{
    CALL_LOG_ENTER;
    CHKNCP(env, ((argc == 1) || (argc == 2)), "The number of parameters is not valid");
    std::vector<float> rotationVector;
    CHKNCP(env, GetFloatArray(env, args[0], rotationVector),
        "Wrong argument type, get rotationVector fail");

    sptr<AsyncCallbackInfo> asyncCallbackInfo =
        new (std::nothrow) AsyncCallbackInfo(env, CREATE_ROTATION_MATRIX);
    CHKPP(asyncCallbackInfo);
    std::vector<float> rotationMatrix(THREE_DIMENSIONAL_MATRIX_LENGTH);
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.createRotationMatrix(rotationVector, rotationMatrix);
    if (ret != OHOS::ERR_OK) {
        SEN_HILOGE("Create rotation matrix fail");
        asyncCallbackInfo->type = FAIL;
        asyncCallbackInfo->error.code = ret;
    } else {
        asyncCallbackInfo->data.reserveData.length = THREE_DIMENSIONAL_MATRIX_LENGTH;
        for (int32_t i = 0; i < THREE_DIMENSIONAL_MATRIX_LENGTH; ++i) {
            asyncCallbackInfo->data.reserveData.reserve[i] = rotationMatrix[i];
        }
    }

    if (argc == 1) {
        napi_value promise = nullptr;
        CHKNRP(env, napi_create_promise(env, &asyncCallbackInfo->deferred, &promise),
            "napi_create_promise");
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    CHKNCP(env, IsMatchType(env, args[1], napi_function), "Wrong argument type, should be function");
    CHKNRP(env, napi_create_reference(env, args[1], 1, &asyncCallbackInfo->callback[0]),
        "napi_create_reference");
    EmitAsyncCallbackWork(asyncCallbackInfo);
    return nullptr;
}

static napi_value CreateRotationMatrix(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 3;
    napi_value args[3] = { 0 };
    napi_value thisVar = nullptr;
    CHKNRP(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr), "napi_get_cb_info");
    CHKNCP(env, ((argc >= 1) && (argc <= 3)), "The number of parameters is not valid");
    CHKNCP(env, IsMatchArrayType(env, args[0]), "Wrong argument type, should be array");

    if (argc == 1 || (argc == 2 && IsMatchType(env, args[1], napi_function))) {
        return GetRotationMatrix(env, args, argc);
    } else if (IsMatchArrayType(env, args[1])) {
        return createRotationAndInclination(env, args, argc);
    }
    return nullptr;
}

static napi_value GetSensorList(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_value thisVar = nullptr;
    CHKNRP(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr), "napi_get_cb_info");
    CHKNCP(env, (argc <= 1), "The number of parameters is not valid");
    sptr<AsyncCallbackInfo> asyncCallbackInfo =
        new (std::nothrow) AsyncCallbackInfo(env, GET_SENSOR_LIST);
    CHKPP(asyncCallbackInfo);

    SensorInfo *sensorInfos = nullptr;
    int32_t count = 0;
    int32_t ret = GetAllSensors(&sensorInfos, &count);
    if (ret != OHOS::ERR_OK) {
        SEN_HILOGE("Get sensor list fail");
        asyncCallbackInfo->type = FAIL;
        asyncCallbackInfo->error.code = ret;
    } else {
        for (int32_t i = 0; i < count; ++i) {
            asyncCallbackInfo->sensorInfos.push_back(*(sensorInfos + i));
        }
        free(sensorInfos);
        sensorInfos = nullptr;
    }
    if (argc == 0) {
        napi_value promise = nullptr;
        CHKNRP(env, napi_create_promise(env, &asyncCallbackInfo->deferred, &promise),
            "napi_create_promise");
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    CHKNCP(env, IsMatchType(env, args[0], napi_function), "Wrong argument type, should be function");
    napi_create_reference(env, args[0], 1, &asyncCallbackInfo->callback[0]);
    EmitAsyncCallbackWork(asyncCallbackInfo);
    return nullptr;
}

static napi_value GetSingleSensor(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_value thisVar = nullptr;
    CHKNRP(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr), "napi_get_cb_info");
    CHKNCP(env, ((argc == 1) || (argc == 2)), "The number of parameters is not valid");
    int32_t sensorTypeId = INVALID_SENSOR_ID;
    CHKNCP(env, GetCppInt32(env, args[0], sensorTypeId), "Wrong argument type, get number fail");

    sptr<AsyncCallbackInfo> asyncCallbackInfo =
        new (std::nothrow) AsyncCallbackInfo(env, GET_SINGLE_SENSOR);
    CHKPP(asyncCallbackInfo);
    SensorInfo *sensorInfos = nullptr;
    int32_t count = 0;
    int32_t ret = GetAllSensors(&sensorInfos, &count);
    if (ret != OHOS::ERR_OK) {
        SEN_HILOGE("Get sensor list fail");
        asyncCallbackInfo->type = FAIL;
        asyncCallbackInfo->error.code = ret;
    } else {
        for (int32_t i = 0; i < count; ++i) {
            if (sensorInfos[i].sensorTypeId == sensorTypeId) {
                asyncCallbackInfo->sensorInfos.push_back(*(sensorInfos + i));
                break;
            }
        }
        if (asyncCallbackInfo->sensorInfos.empty()) {
            SEN_HILOGE("Not find sensorTypeId: %{public}d", sensorTypeId);
            asyncCallbackInfo->type = FAIL;
        }
        free(sensorInfos);
        sensorInfos = nullptr;
    }
    if (argc == 1) {
        napi_value promise = nullptr;
        CHKNRP(env, napi_create_promise(env, &asyncCallbackInfo->deferred, &promise), "napi_create_promise");
        EmitPromiseWork(asyncCallbackInfo);
        return promise;
    }
    CHKNCP(env, IsMatchType(env, args[1], napi_function), "Wrong argument type, should be function");
    napi_create_reference(env, args[1], 1, &asyncCallbackInfo->callback[0]);
    EmitAsyncCallbackWork(asyncCallbackInfo);
    return nullptr;
}

napi_value Subscribe(napi_env env, napi_callback_info info, int32_t sensorTypeId, CallbackDataType type)
{
    CALL_LOG_ENTER;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_value thisVar = nullptr;
    CHKNRP(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr), "napi_get_cb_info");
    CHKNCP(env, (argc == 1), "The number of parameters is not valid");
    CHKNCP(env, IsMatchType(env, args[0], napi_object), "Wrong argument type, should be object");

    string interval = "normal";
    if ((sensorTypeId == SENSOR_TYPE_ID_ACCELEROMETER) ||
        ((sensorTypeId == SENSOR_TYPE_ID_ORIENTATION) && (type != SUBSCRIBE_COMPASS))
        || (sensorTypeId == SENSOR_TYPE_ID_GYROSCOPE)) {
        napi_value napiInterval = GetNamedProperty(env, args[0], "interval");
        CHKNCP(env, GetStringValue(env, napiInterval, interval), "get interval fail");
    }
    sptr<AsyncCallbackInfo> asyncCallbackInfo = new (std::nothrow) AsyncCallbackInfo(env, type);
    CHKPP(asyncCallbackInfo);
    napi_value napiSuccess = GetNamedProperty(env, args[0], "success");
    CHKNCP(env, (napiSuccess != nullptr), "get napiSuccess fail");
    CHKNCP(env, RegisterNapiCallback(env, napiSuccess, asyncCallbackInfo->callback[0]),
        "register callback fail");
    napi_value napiFail = GetNamedProperty(env, args[0], "fail");
    if (napiFail != nullptr) {
        SEN_HILOGD("has fail callback");
        CHKNCP(env, RegisterNapiCallback(env, napiFail, asyncCallbackInfo->callback[1]),
            "register callback fail");
    }
    if (auto iter = g_samplingPeriod.find(interval); iter == g_samplingPeriod.end()) {
        CHKNCP(env, (napiFail != nullptr), "input error, interval is invalid");
        CreateFailMessage(SUBSCRIBE_FAIL, INPUT_ERROR, "input error", asyncCallbackInfo);
        EmitAsyncCallbackWork(asyncCallbackInfo);
        return nullptr;
    }
    std::lock_guard<std::mutex> subscribeCallbackLock(mutex_);
    bool ret = SubscribeSensor(sensorTypeId, g_samplingPeriod[interval], DataCallbackImpl);
    if (!ret) {
        CHKNCP(env, (napiFail != nullptr), "subscribe fail");
        CreateFailMessage(SUBSCRIBE_FAIL, SENSOR_SUBSCRIBE_FAILURE, "subscribe fail", asyncCallbackInfo);
        EmitAsyncCallbackWork(asyncCallbackInfo);
        return nullptr;
    }
    g_subscribeCallbacks[sensorTypeId] = asyncCallbackInfo;
    return nullptr;
}

napi_value Unsubscribe(napi_env env, napi_callback_info info, int32_t sensorTypeId)
{
    CALL_LOG_ENTER;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_value thisVar = nullptr;
    CHKNRP(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr), "napi_get_cb_info");
    CHKNCP(env, (argc == 0), "The number of parameters is not valid");
    std::lock_guard<std::mutex> subscribeCallbackLock(mutex_);
    g_subscribeCallbacks[sensorTypeId] = nullptr;
    if (CheckSubscribe(sensorTypeId)) {
        SEN_HILOGW("There are other client subscribe as well, not need unsubscribe");
        return nullptr;
    }
    CHKNCP(env, UnsubscribeSensor(sensorTypeId), "UnsubscribeSensor failed");
    return nullptr;
}

napi_value GetBodyState(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_value thisVar = nullptr;
    CHKNRP(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr), "napi_get_cb_info");
    CHKNCP(env, (argc == 1), "The number of parameters is not valid");
    CHKNCP(env, IsMatchType(env, args[0], napi_object), "Wrong argument type, should be object");
    sptr<AsyncCallbackInfo> asyncCallbackInfo = new (std::nothrow) AsyncCallbackInfo(env, GET_BODY_STATE);
    CHKPP(asyncCallbackInfo);
    napi_value napiSuccess = GetNamedProperty(env, args[0], "success");
    CHKNCP(env, RegisterNapiCallback(env, napiSuccess, asyncCallbackInfo->callback[0]),
        "register success callback fail");
    std::lock_guard<std::mutex> onBodyLock(bodyMutex_);
    asyncCallbackInfo->data.sensorData.data[0] =
        (fabs(g_bodyState - BODY_STATE_EXCEPT) < THREESHOLD) ? true : false;
    EmitUvEventLoop(asyncCallbackInfo);
    return nullptr;
}

static napi_value EnumClassConstructor(napi_env env, napi_callback_info info)
{
    size_t argc = 0;
    napi_value args[1] = {0};
    napi_value ret = nullptr;
    void *data = nullptr;
    CHKNRP(env, napi_get_cb_info(env, info, &argc, args, &ret, &data), "napi_get_cb_info");
    return ret;
}

static napi_value CreateEnumSensorType(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_ACCELEROMETER", GetNapiInt32(env, SENSOR_TYPE_ID_ACCELEROMETER)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_GYROSCOPE", GetNapiInt32(env, SENSOR_TYPE_ID_GYROSCOPE)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_AMBIENT_LIGHT", GetNapiInt32(env, SENSOR_TYPE_ID_AMBIENT_LIGHT)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_MAGNETIC_FIELD", GetNapiInt32(env, SENSOR_TYPE_ID_MAGNETIC_FIELD)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_BAROMETER", GetNapiInt32(env, SENSOR_TYPE_ID_BAROMETER)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_HALL", GetNapiInt32(env, SENSOR_TYPE_ID_HALL)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_PROXIMITY", GetNapiInt32(env, SENSOR_TYPE_ID_PROXIMITY)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_HUMIDITY", GetNapiInt32(env, SENSOR_TYPE_ID_HUMIDITY)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_ORIENTATION", GetNapiInt32(env, SENSOR_TYPE_ID_ORIENTATION)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_GRAVITY", GetNapiInt32(env, SENSOR_TYPE_ID_GRAVITY)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_LINEAR_ACCELERATION",
            GetNapiInt32(env, SENSOR_TYPE_ID_LINEAR_ACCELERATION)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_ROTATION_VECTOR",
            GetNapiInt32(env, SENSOR_TYPE_ID_ROTATION_VECTOR)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_AMBIENT_TEMPERATURE",
            GetNapiInt32(env, SENSOR_TYPE_ID_AMBIENT_TEMPERATURE)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_MAGNETIC_FIELD_UNCALIBRATED",
            GetNapiInt32(env, SENSOR_TYPE_ID_MAGNETIC_FIELD_UNCALIBRATED)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_GYROSCOPE_UNCALIBRATED",
            GetNapiInt32(env, SENSOR_TYPE_ID_GYROSCOPE_UNCALIBRATED)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_SIGNIFICANT_MOTION",
            GetNapiInt32(env, SENSOR_TYPE_ID_SIGNIFICANT_MOTION)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_PEDOMETER_DETECTION",
            GetNapiInt32(env, SENSOR_TYPE_ID_PEDOMETER_DETECTION)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_PEDOMETER", GetNapiInt32(env, SENSOR_TYPE_ID_PEDOMETER)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_HEART_RATE", GetNapiInt32(env, SENSOR_TYPE_ID_HEART_RATE)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_WEAR_DETECTION", GetNapiInt32(env, SENSOR_TYPE_ID_WEAR_DETECTION)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_ACCELEROMETER_UNCALIBRATED",
            GetNapiInt32(env, SENSOR_TYPE_ID_ACCELEROMETER_UNCALIBRATED)),
    };
    napi_value result = nullptr;
    CHKNRP(env, napi_define_class(env, "SensorType", NAPI_AUTO_LENGTH, EnumClassConstructor, nullptr,
        sizeof(desc) / sizeof(*desc), desc, &result), "napi_define_class");
    CHKNRP(env, napi_set_named_property(env, exports, "SensorType", result), "napi_set_named_property fail");
    return exports;
}

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
    CHKNRP(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(napi_property_descriptor), desc),
        "napi_define_properties");
    CHKNCP(env, CreateEnumSensorType(env, exports), "Create enum sensor type fail");
    return exports;
}

static napi_module _module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "sensor",
    .nm_priv = ((void *)0),
    .reserved = {0}
};

extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&_module);
}
}  // namespace Sensors
}  // namespace OHOS