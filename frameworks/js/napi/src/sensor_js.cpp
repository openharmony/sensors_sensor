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

#include <algorithm>
#include <cinttypes>
#include <cstdlib>
#include <cmath>
#include <string>
#include <unistd.h>

#include "refbase.h"
#include "securec.h"

#include "geomagnetic_field.h"
#include "sensor_algorithm.h"
#include "sensor_napi_error.h"
#include "sensor_napi_utils.h"
#include "sensor_system_js.h"

namespace OHOS {
namespace Sensors {
namespace {
constexpr int32_t QUATERNION_LENGTH = 4;
constexpr int32_t ROTATION_VECTOR_LENGTH = 3;
constexpr int32_t REPORTING_INTERVAL = 200000000;
constexpr int32_t INVALID_SENSOR_TYPE = -1;
constexpr int32_t DEFAULT_DEVICE_ID = -1;
constexpr int32_t DEFAULT_SENSOR_ID = 0;
constexpr int32_t IS_LOCAL_DEVICE = 1;
constexpr int32_t NON_LOCAL_DEVICE = 0;
constexpr int32_t INVALID_SUBSCRIBE_SIZE = -1;
constexpr int32_t DEFAULT_SUBSCRIBE_SIZE = 0;
constexpr int32_t SENSOR_SUBSCRIBE_FAILURE = 1001;
constexpr int32_t INPUT_ERROR = 202;
constexpr float BODY_STATE_EXCEPT = 1.0f;
constexpr float THRESHOLD = 0.000001f;
constexpr uint32_t COMPATIBILITY_CHANGE_VERSION_API12 = 12;
constexpr int32_t ARGC_NUM_TWO = 2;
constexpr int32_t ARGC_NUM_THREE = 3;
constexpr int32_t ARGS_NUM_TWO = 2;
} // namespace
static std::map<std::string, int64_t> g_samplingPeriod = {
    {"normal", 200000000},
    {"ui", 60000000},
    {"game", 20000000},
};
static std::mutex g_mutex;
static std::mutex g_bodyMutex;
static float g_bodyState = -1.0f;
static std::map<SensorDescription, std::vector<sptr<AsyncCallbackInfo>>> g_subscribeCallbacks;
static std::mutex g_onMutex;
static std::mutex g_onceMutex;
static std::mutex g_plugMutex;
static std::map<SensorDescription, std::vector<sptr<AsyncCallbackInfo>>> g_onceCallbackInfos;
static std::map<SensorDescription, std::vector<sptr<AsyncCallbackInfo>>> g_onCallbackInfos;
static std::vector<sptr<AsyncCallbackInfo>> g_plugCallbackInfo;

static bool CheckSubscribe(SensorDescription sensorDesc)
{
    std::lock_guard<std::mutex> onCallbackLock(g_onMutex);
    auto iter = g_onCallbackInfos.find(sensorDesc);
    return iter != g_onCallbackInfos.end();
}

static bool copySensorData(sptr<AsyncCallbackInfo> callbackInfo, SensorEvent *event)
{
    CHKPF(callbackInfo);
    CHKPF(event);
    int32_t sensorTypeId = event->sensorTypeId;
    callbackInfo->data.sensorData.sensorTypeId = sensorTypeId;
    callbackInfo->data.sensorData.dataLength = event->dataLen;
    callbackInfo->data.sensorData.timestamp = event->timestamp;
    callbackInfo->data.sensorData.sensorAccuracy = event->option;
    CHKPF(event->data);
    if (event->dataLen < sizeof(float)) {
        SEN_HILOGE("Event dataLen less than float size");
        return false;
    }
    auto data = reinterpret_cast<float *>(event->data);
    if (sensorTypeId == SENSOR_TYPE_ID_WEAR_DETECTION && callbackInfo->type == SUBSCRIBE_CALLBACK) {
        std::lock_guard<std::mutex> onBodyLock(g_bodyMutex);
        g_bodyState = *data;
        callbackInfo->data.sensorData.data[0] =
            (fabs(g_bodyState - BODY_STATE_EXCEPT) < THRESHOLD) ? true : false;
        return true;
    }
    if (sizeof(callbackInfo->data.sensorData.data) < event->dataLen) {
        SEN_HILOGE("callbackInfo space is insufficient");
        return false;
    }
    if (memcpy_s(callbackInfo->data.sensorData.data, sizeof(callbackInfo->data.sensorData.data),
        data, event->dataLen) != EOK) {
        SEN_HILOGE("Copy data failed");
        return false;
    }
    return true;
}

static bool CheckSystemSubscribe(SensorDescription sensorDesc)
{
    std::lock_guard<std::mutex> subscribeLock(g_mutex);
    auto iter = g_subscribeCallbacks.find(sensorDesc);
    return iter != g_subscribeCallbacks.end();
}

static void EmitSubscribeCallback(SensorEvent *event)
{
    CHKPV(event);
    if (!CheckSystemSubscribe({event->deviceId, event->sensorTypeId, event->sensorId, event->location})) {
        return;
    }
    std::lock_guard<std::mutex> subscribeLock(g_mutex);
    auto callbacks = g_subscribeCallbacks[{event->deviceId, event->sensorTypeId, event->sensorId, event->location}];
    for (auto &callback : callbacks) {
        if (!copySensorData(callback, event)) {
            SEN_HILOGE("Copy sensor data failed");
            continue;
        }
        EmitUvEventLoop(callback);
    }
}

static void EmitOnCallback(SensorEvent *event)
{
    CHKPV(event);
    if (!CheckSubscribe({event->deviceId, event->sensorTypeId, event->sensorId, event->location})) {
        return;
    }
    std::lock_guard<std::mutex> onCallbackLock(g_onMutex);
    auto onCallbackInfos = g_onCallbackInfos[{event->deviceId, event->sensorTypeId, event->sensorId, event->location}];
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
    std::lock_guard<std::mutex> onceCallbackLock(g_onceMutex);
    auto iter = g_onceCallbackInfos.find({event->deviceId, event->sensorTypeId, event->sensorId, event->location});
    if (iter == g_onceCallbackInfos.end()) {
        return;
    }
    auto &onceCallbackInfos = iter->second;
    while (!onceCallbackInfos.empty()) {
        auto onceCallbackInfo = onceCallbackInfos.front();
        auto beginIter = onceCallbackInfos.begin();
        onceCallbackInfos.erase(beginIter);
        if (!copySensorData(onceCallbackInfo, event)) {
            SEN_HILOGE("Copy sensor data failed");
            continue;
        }
        EmitUvEventLoop(std::move(onceCallbackInfo));
    }
    g_onceCallbackInfos.erase({event->deviceId, event->sensorTypeId, event->sensorId, event->location});

    CHKCV((!CheckSubscribe({event->deviceId, event->sensorTypeId, event->sensorId, event->location})),
        "Has client subscribe, not need cancel subscribe");
    CHKCV((!CheckSystemSubscribe({event->deviceId, event->sensorTypeId, event->sensorId, event->location})),
        "Has client subscribe system api, not need cancel subscribe");
    UnsubscribeSensor({event->deviceId, event->sensorTypeId, event->sensorId, event->location});
}

void DataCallbackImpl(SensorEvent *event)
{
    CHKPV(event);
    EmitOnCallback(event);
    EmitSubscribeCallback(event);
    EmitOnceCallback(event);
}

static void UpdatePlugInfo(SensorStatusEvent *plugEvent, sptr<AsyncCallbackInfo> &asyncCallbackInfo)
{
    CALL_LOG_ENTER;
    CHKPV(plugEvent);
    asyncCallbackInfo->sensorStatusEvent = *plugEvent;
    SEN_HILOGD("asyncCallbackInfo->sensorStatus : [ deviceId = %{public}d, isSensorOnline = %{public}d]",
        asyncCallbackInfo->sensorStatusEvent.deviceId, asyncCallbackInfo->sensorStatusEvent.isSensorOnline);
    return;
}

void PlugDataCallbackImpl(SensorStatusEvent *plugEvent)
{
    CALL_LOG_ENTER;
    CHKPV(plugEvent);
    std::lock_guard<std::mutex> plugCallbackLock(g_plugMutex);
    for (auto& callback : g_plugCallbackInfo) {
        UpdatePlugInfo(plugEvent, callback);
        EmitUvEventLoop(callback);
    }
}

const SensorUser user = {
    .callback = DataCallbackImpl,
    .plugCallback = PlugDataCallbackImpl
};

int32_t UnsubscribeSensor(SensorDescription sensorDesc)
{
    CALL_LOG_ENTER;
    int32_t ret = DeactivateSensorEnhanced({sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId,
        sensorDesc.location}, &user);
    if (ret != ERR_OK) {
        SEN_HILOGE("DeactivateSensor failed");
        return ret;
    }
    return UnsubscribeSensorEnhanced({sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId,
        sensorDesc.location}, &user);
}

int32_t SubscribeSensor(SensorDescription sensorDesc, int64_t interval, RecordSensorCallback callback)
{
    CALL_LOG_ENTER;
    int32_t ret = SubscribeSensorEnhanced({sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId,
        sensorDesc.location}, &user);
    if (ret != ERR_OK) {
        SEN_HILOGE("SubscribeSensor failed");
        return ret;
    }
    ret = SetBatchEnhanced({sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId,
        sensorDesc.location}, &user, interval, 0);
    if (ret != ERR_OK) {
        SEN_HILOGE("SetBatch failed");
        return ret;
    }
    return ActivateSensorEnhanced({sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId,
        sensorDesc.location}, &user);
}

void CleanCallbackInfo(napi_env env, std::map<SensorDescription, std::vector<sptr<AsyncCallbackInfo>>> &callbackInfo)
{
    for (auto &event : callbackInfo) {
        auto &vecCallbackInfo = event.second;
        // Automatically call the destructor of the AsyncCallbackInfo
        vecCallbackInfo.erase(std::remove_if(vecCallbackInfo.begin(), vecCallbackInfo.end(),
            [&env](const sptr<AsyncCallbackInfo> &myCallbackInfo) {
                return env == myCallbackInfo->env;
            }), vecCallbackInfo.end());
    }
}

void CleanOnCallbackInfo(napi_env env)
{
    std::lock_guard<std::mutex> onCallbackLock(g_onMutex);
    CleanCallbackInfo(env, g_onCallbackInfos);
}

void CleanOnceCallbackInfo(napi_env env)
{
    std::lock_guard<std::mutex> onceCallbackLock(g_onceMutex);
    CleanCallbackInfo(env, g_onceCallbackInfos);
}

void CleanSubscribeCallbackInfo(napi_env env)
{
    std::lock_guard<std::mutex> subscribeLock(g_mutex);
    CleanCallbackInfo(env, g_subscribeCallbacks);
}

void CleanUp(void *data)
{
    auto env = *(reinterpret_cast<napi_env*>(data));
    CleanOnCallbackInfo(env);
    CleanOnceCallbackInfo(env);
    CleanSubscribeCallbackInfo(env);
    delete reinterpret_cast<napi_env*>(data);
    data = nullptr;
}

static bool IsOnceSubscribed(napi_env env, SensorDescription sensorDesc, napi_value callback)
{
    CALL_LOG_ENTER;
    auto iter = g_onceCallbackInfos.find(sensorDesc);
    if (iter == g_onceCallbackInfos.end()) {
        SEN_HILOGW("Already subscribed, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
            sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
        return false;
    }
    std::vector<sptr<AsyncCallbackInfo>> callbackInfos = g_onceCallbackInfos[sensorDesc];
    for (auto callbackInfo : callbackInfos) {
        CHKPC(callbackInfo);
        if (callbackInfo->env != env) {
            continue;
        }
        napi_value sensorCallback = nullptr;
        CHKNRF(env, napi_get_reference_value(env, callbackInfo->callback[0], &sensorCallback),
            "napi_get_reference_value");
        if (IsSameValue(env, callback, sensorCallback)) {
            return true;
        }
    }
    return false;
}

static void UpdateOnceCallback(napi_env env, SensorDescription sensorDesc, napi_value callback)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> onceCallbackLock(g_onceMutex);
    CHKCV((!IsOnceSubscribed(env, sensorDesc, callback)), "The callback has been subscribed");
    sptr<AsyncCallbackInfo> asyncCallbackInfo = new (std::nothrow) AsyncCallbackInfo(env, ONCE_CALLBACK);
    CHKPV(asyncCallbackInfo);
    napi_status status = napi_create_reference(env, callback, 1, &asyncCallbackInfo->callback[0]);
    if (status != napi_ok) {
        ThrowErr(env, PARAMETER_ERROR, "napi_create_reference fail");
        return;
    }
    std::vector<sptr<AsyncCallbackInfo>> callbackInfos = g_onceCallbackInfos[sensorDesc];
    callbackInfos.push_back(asyncCallbackInfo);
    g_onceCallbackInfos[sensorDesc] = callbackInfos;
}

static bool GetDeviceId(napi_env env, napi_value value, int32_t &deviceId)
{
    napi_value napi_deviceId = GetNamedProperty(env, value, "deviceId");
    if (!IsMatchType(env, napi_deviceId, napi_number)) {
        SEN_HILOGE("deviceId failed");
        return false;
    }
    if (!GetNativeInt32(env, napi_deviceId, deviceId)) {
        SEN_HILOGE("GetNativeInt64 failed");
        return false;
    }
    return true;
}

static bool GetSensorId(napi_env env, napi_value value, int32_t &sensorId)
{
    napi_value napi_sensorId = GetNamedProperty(env, value, "sensorIndex");
    if (!IsMatchType(env, napi_sensorId, napi_number)) {
        SEN_HILOGE("sensorIndex failed");
        return false;
    }
    if (!GetNativeInt32(env, napi_sensorId, sensorId)) {
        SEN_HILOGE("GetNativeInt64 failed");
        return false;
    }
    return true;
}

static bool GetLocationDeviceId(int32_t &deviceId)
{
    SensorInfo *sensorInfos = nullptr;
    int32_t count = 0;
    int32_t ret = GetAllSensors(&sensorInfos, &count);
    if (ret != OHOS::ERR_OK) {
        SEN_HILOGE("GetAllSensors failed");
        return false;
    }
    for (int32_t i = 0; i < count; ++i) {
        if (sensorInfos[i].location == IS_LOCAL_DEVICE) {
            SEN_HILOGD("The location deviceId is %{public}d", sensorInfos[i].deviceId);
            deviceId = sensorInfos[i].deviceId;
            return true;
        }
    }
    return false;
}

static napi_value Once(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 3;
    napi_value args[3] = { 0 };
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr);
    if (status != napi_ok || argc < 2) {
        ThrowErr(env, PARAMETER_ERROR, "napi_get_cb_info fail or number of parameter invalid");
        return nullptr;
    }
    if ((!IsMatchType(env, args[0], napi_number)) || (!IsMatchType(env, args[1], napi_function))) {
        ThrowErr(env, PARAMETER_ERROR, "Wrong argument type");
        return nullptr;
    }
    int32_t sensorType = INVALID_SENSOR_TYPE;
    if (!GetNativeInt32(env, args[0], sensorType)) {
        ThrowErr(env, PARAMETER_ERROR, "Wrong argument type, get number fail");
        return nullptr;
    }
    if (!CheckSubscribe({DEFAULT_DEVICE_ID, sensorType, DEFAULT_SENSOR_ID, IS_LOCAL_DEVICE})) {
        SEN_HILOGD("No subscription to change sensor data, registration is required");
        int32_t ret = SubscribeSensor({DEFAULT_DEVICE_ID, sensorType, DEFAULT_SENSOR_ID, IS_LOCAL_DEVICE},
            REPORTING_INTERVAL, DataCallbackImpl);
        if (ret != ERR_OK) {
            ThrowErr(env, ret, "SubscribeSensor fail");
            return nullptr;
        }
    }
    UpdateOnceCallback(env, {DEFAULT_DEVICE_ID, sensorType, DEFAULT_SENSOR_ID, IS_LOCAL_DEVICE}, args[1]);
    return nullptr;
}

static bool IsSubscribed(napi_env env, SensorDescription sensorDesc, napi_value callback)
{
    CALL_LOG_ENTER;
    auto iter = g_onCallbackInfos.find(sensorDesc);
    if (iter == g_onCallbackInfos.end()) {
        SEN_HILOGW("No client subscribe, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
            sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
        return false;
    }
    std::vector<sptr<AsyncCallbackInfo>> callbackInfos = g_onCallbackInfos[sensorDesc];
    for (auto callbackInfo : callbackInfos) {
        CHKPC(callbackInfo);
        if (callbackInfo->env != env) {
            continue;
        }
        napi_value sensorCallback = nullptr;
        CHKNRF(env, napi_get_reference_value(env, callbackInfo->callback[0], &sensorCallback),
            "napi_get_reference_value");
        if (IsSameValue(env, callback, sensorCallback)) {
            return true;
        }
    }
    return false;
}

static void UpdateCallbackInfos(napi_env env, SensorDescription sensorDesc, napi_value callback)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> onCallbackLock(g_onMutex);
    CHKCV((!IsSubscribed(env, sensorDesc, callback)), "The callback has been subscribed");
    sptr<AsyncCallbackInfo> asyncCallbackInfo = new (std::nothrow) AsyncCallbackInfo(env, ON_CALLBACK);
    CHKPV(asyncCallbackInfo);
    napi_status status = napi_create_reference(env, callback, 1, &asyncCallbackInfo->callback[0]);
    if (status != napi_ok) {
        ThrowErr(env, PARAMETER_ERROR, "napi_create_reference fail");
        return;
    }
    std::vector<sptr<AsyncCallbackInfo>> callbackInfos = g_onCallbackInfos[sensorDesc];
    callbackInfos.push_back(asyncCallbackInfo);
    g_onCallbackInfos[sensorDesc] = callbackInfos;
}

static bool GetInterval(napi_env env, napi_value value, int64_t &interval)
{
    napi_value napiInterval = GetNamedProperty(env, value, "interval");
    if (IsMatchType(env, napiInterval, napi_number)) {
        if (!GetNativeInt64(env, napiInterval, interval)) {
            SEN_HILOGE("GetNativeInt64 failed");
            return false;
        }
    } else if (IsMatchType(env, napiInterval, napi_string)) {
        std::string mode;
        if (!GetStringValue(env, napiInterval, mode)) {
            SEN_HILOGE("GetStringValue failed");
            return false;
        }
        auto iter = g_samplingPeriod.find(mode);
        if (iter == g_samplingPeriod.end()) {
            SEN_HILOGE("Find interval mode failed");
            return false;
        }
        interval = iter->second;
        SEN_HILOGI("%{public}s", mode.c_str());
    } else {
        SEN_HILOGE("Interval failed");
        return false;
    }
    return true;
}

static bool IsPlugSubscribed(napi_env env, napi_value callback)
{
    CALL_LOG_ENTER;
    for (auto callbackInfo : g_plugCallbackInfo) {
        CHKPC(callbackInfo);
        if (callbackInfo->env != env) {
            continue;
        }
        napi_value plugCallback = nullptr;
        CHKNRF(env, napi_get_reference_value(env, callbackInfo->callback[0], &plugCallback),
            "napi_get_reference_value");
        if (IsSameValue(env, callback, plugCallback)) {
            return true;
        }
    }
    return false;
}

static void UpdatePlugCallbackInfos(const napi_env& env, napi_value callback)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> plugCallbackLock(g_plugMutex);
    CHKCV((!IsPlugSubscribed(env, callback)), "The plugCallback has been subscribed");
    sptr<AsyncCallbackInfo> asyncCallbackInfo = new (std::nothrow) AsyncCallbackInfo(env, SENSOR_STATE_CHANGE);
    CHKPV(asyncCallbackInfo);
    napi_status status = napi_create_reference(env, callback, 1, &asyncCallbackInfo->callback[0]);
    if (status != napi_ok) {
        ThrowErr(env, PARAMETER_ERROR, "napi_create_reference fail");
        return;
    }
    g_plugCallbackInfo.push_back(asyncCallbackInfo);
}

int32_t SubscribeSensorPlug(SensorPlugCallback callback)
{
    CALL_LOG_ENTER;
    int32_t ret = SubscribeSensorPlug(&user);
    if (ret != ERR_OK) {
        SEN_HILOGE("SubscribeSensorPlug failed");
    }
    return ret;
}

static napi_value OnPlugSensor(napi_env env, const napi_value type, const napi_value callback)
{
    CALL_LOG_ENTER;
    std::string plugType;
    CHKCP(GetStringValue(env, type, plugType), "get plugType fail");
    if (plugType != "sensorStatusChange") {
        ThrowErr(env, PARAMETER_ERROR, "Wrong sensorStatusChange type");
        return nullptr;
    }
    int32_t ret = SubscribeSensorPlug(PlugDataCallbackImpl);
    if (ret != ERR_OK) {
        ThrowErr(env, ret, "SubscribeSensorPlug fail");
        return nullptr;
    }
    UpdatePlugCallbackInfos(env, callback);
    return nullptr;
}

static bool GetDeviceIdBySensorInfoParam(napi_env env, napi_value value, int32_t &deviceId)
{
    napi_value napiSensorInfoParam = GetNamedProperty(env, value, "sensorInfoParam");
    if (!IsMatchType(env, napiSensorInfoParam, napi_object)) {
        SEN_HILOGW("sensorInfoParam failed");
        return false;
    }
    if (!GetDeviceId(env, napiSensorInfoParam, deviceId)) {
        return false;
    }
    return true;
}

static bool GetSensorIdBySensorInfoParam(napi_env env, napi_value value, int32_t &sensorId)
{
    napi_value napiSensorInfoParam = GetNamedProperty(env, value, "sensorInfoParam");
    if (!IsMatchType(env, napiSensorInfoParam, napi_object)) {
        SEN_HILOGW("sensorInfoParam failed");
        return false;
    }
    if (!GetSensorId(env, napiSensorInfoParam, sensorId)) {
        return false;
    }
    return true;
}

static bool GetOptionalParameter(napi_env env, size_t argc, napi_value args, int64_t &interval,
    SensorDescription &sensorDesc)
{
    int32_t localDeviceId = DEFAULT_DEVICE_ID;
    if (!GetLocationDeviceId(localDeviceId)) {
        SEN_HILOGW("Cant fand local deviceId, default loacl deviceId :%{public}d", localDeviceId);
    }
    sensorDesc.deviceId = localDeviceId;
    sensorDesc.sensorId = DEFAULT_SENSOR_ID;
    sensorDesc.location = IS_LOCAL_DEVICE;
    if (argc >= ARGC_NUM_THREE && IsMatchType(env, args, napi_object)) {
        if (!GetInterval(env, args, interval)) {
            SEN_HILOGW("Get interval failed");
        }
        if (!GetDeviceIdBySensorInfoParam(env, args, sensorDesc.deviceId)) {
            SEN_HILOGW("No deviceId, This device is selected by default");
            sensorDesc.deviceId = localDeviceId;
        }
        if (!GetSensorIdBySensorInfoParam(env, args, sensorDesc.sensorId)) {
            sensorDesc.sensorId = DEFAULT_SENSOR_ID;
            SEN_HILOGW("No sensorId, The first sensor of the type is selected by default");
        }
    }
    if (sensorDesc.deviceId != localDeviceId) {
        sensorDesc.location = NON_LOCAL_DEVICE;
    }
    SEN_HILOGD("Interval is %{public}" PRId64, interval);
    return true;
}

static napi_value On(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 3;
    napi_value args[3] = { 0 };
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr);
    if (status != napi_ok || argc < 2) {
        ThrowErr(env, PARAMETER_ERROR, "napi_get_cb_info fail or number of parameter invalid");
        return nullptr;
    }
    if (IsMatchType(env, args[0], napi_string) && IsMatchType(env, args[1], napi_function)) {
        return OnPlugSensor(env, args[0], args[1]);
    }
    if ((!IsMatchType(env, args[0], napi_number)) || (!IsMatchType(env, args[1], napi_function))) {
        ThrowErr(env, PARAMETER_ERROR, "Wrong argument type");
        return nullptr;
    }
    SensorDescription sensorDesc;
    sensorDesc.sensorType = INVALID_SENSOR_TYPE;
    if (!GetNativeInt32(env, args[0], sensorDesc.sensorType)) {
        ThrowErr(env, PARAMETER_ERROR, "Wrong argument type, get number fail");
        return nullptr;
    }
    int64_t interval = REPORTING_INTERVAL;
    if (!GetOptionalParameter(env, argc, args[ARGS_NUM_TWO], interval, sensorDesc)) {
        SEN_HILOGE("location deviceId fail");
        return nullptr;
    }
    int32_t ret = SubscribeSensor(sensorDesc, interval, DataCallbackImpl);
    if (ret != ERR_OK) {
        ThrowErr(env, ret, "SubscribeSensor fail");
        return nullptr;
    }
    UpdateCallbackInfos(env, sensorDesc, args[1]);
    return nullptr;
}

static int32_t RemoveAllPlugCallback(napi_env env)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> plugCallbackLock(g_plugMutex);
    for (auto iter = g_plugCallbackInfo.begin(); iter != g_plugCallbackInfo.end();) {
        CHKPC(*iter);
        if ((*iter)->env != env) {
            ++iter;
            continue;
        }
        iter = g_plugCallbackInfo.erase(iter);
    }
    if (g_plugCallbackInfo.empty()) {
        SEN_HILOGD("No subscription to change sensor data");
        return DEFAULT_SUBSCRIBE_SIZE;
    }
    return g_plugCallbackInfo.size();
}

static int32_t RemovePlugCallback(napi_env env, napi_value callback)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> plugCallbackLock(g_plugMutex);
    for (auto iter = g_plugCallbackInfo.begin(); iter != g_plugCallbackInfo.end();) {
        CHKPC(*iter);
        if ((*iter)->env != env) {
            continue;
        }
        napi_value sensorCallback = nullptr;
        if (napi_get_reference_value(env, (*iter)->callback[0], &sensorCallback) != napi_ok) {
            SEN_HILOGE("napi_get_reference_value fail");
            continue;
        }
        if (IsSameValue(env, callback, sensorCallback)) {
            iter = g_plugCallbackInfo.erase(iter);
            SEN_HILOGD("Remove callback success");
            break;
        } else {
            ++iter;
        }
    }
    if (g_plugCallbackInfo.empty()) {
        SEN_HILOGD("No subscription to change sensor data");
        return DEFAULT_SUBSCRIBE_SIZE;
    }
    return g_plugCallbackInfo.size();
}

static napi_value OffPlugSensor(napi_env env, size_t argc, const napi_value type, const napi_value callback)
{
    CALL_LOG_ENTER;
    std::string plugType;
    CHKCP(GetStringValue(env, type, plugType), "get plugType fail");
    if (plugType != "sensorStatusChange") {
        ThrowErr(env, PARAMETER_ERROR, "Wrong sensorStatusChange type");
        return nullptr;
    }
    int32_t subscribeSize = INVALID_SUBSCRIBE_SIZE;
    if (argc == 1) {
        subscribeSize = RemoveAllPlugCallback(env);
    } else if (IsMatchType(env, callback, napi_undefined) || IsMatchType(env, callback, napi_null)) {
        subscribeSize = RemoveAllPlugCallback(env);
    } else if (IsMatchType(env, callback, napi_function)) {
        subscribeSize = RemovePlugCallback(env, callback);
    } else {
        ThrowErr(env, PARAMETER_ERROR, "Wrong argument type, args[1] should is napi_function");
        return nullptr;
    }
    if (subscribeSize > DEFAULT_SUBSCRIBE_SIZE) {
        SEN_HILOGW("There are other client subscribe system js api as well, not need unsubscribe");
        return nullptr;
    }
    int32_t ret = UnsubscribeSensorPlug(&user);
    if (ret != ERR_OK) {
        ThrowErr(env, ret, "UnSubscribeSensorPlug fail");
        return nullptr;
    }
    return nullptr;
}

static int32_t RemoveAllCallback(napi_env env, SensorDescription sensorDesc)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> onCallbackLock(g_onMutex);
    std::vector<sptr<AsyncCallbackInfo>> callbackInfos = g_onCallbackInfos[sensorDesc];
    for (auto iter = callbackInfos.begin(); iter != callbackInfos.end();) {
        CHKPC(*iter);
        if ((*iter)->env != env) {
            ++iter;
            continue;
        }
        iter = callbackInfos.erase(iter);
    }
    if (callbackInfos.empty()) {
        SEN_HILOGD("No subscription to change sensor data");
        g_onCallbackInfos.erase(sensorDesc);
        return DEFAULT_SUBSCRIBE_SIZE;
    }
    g_onCallbackInfos[sensorDesc] = callbackInfos;
    return callbackInfos.size();
}

static int32_t RemoveCallback(napi_env env, SensorDescription sensorDesc, napi_value callback)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> onCallbackLock(g_onMutex);
    std::vector<sptr<AsyncCallbackInfo>> callbackInfos = g_onCallbackInfos[sensorDesc];
    for (auto iter = callbackInfos.begin(); iter != callbackInfos.end();) {
        CHKPC(*iter);
        if ((*iter)->env != env) {
            continue;
        }
        napi_value sensorCallback = nullptr;
        if (napi_get_reference_value(env, (*iter)->callback[0], &sensorCallback) != napi_ok) {
            SEN_HILOGE("napi_get_reference_value fail");
            continue;
        }
        if (IsSameValue(env, callback, sensorCallback)) {
            iter = callbackInfos.erase(iter);
            SEN_HILOGD("Remove callback success");
            break;
        } else {
            ++iter;
        }
    }
    if (callbackInfos.empty()) {
        SEN_HILOGD("No subscription to change sensor data");
        g_onCallbackInfos.erase(sensorDesc);
        return DEFAULT_SUBSCRIBE_SIZE;
    }
    g_onCallbackInfos[sensorDesc] = callbackInfos;
    return callbackInfos.size();
}

static bool GetSensorInfoParameter(napi_env env, size_t argc, napi_value args, SensorDescription &sensorDesc)
{
    int32_t localDeviceId = DEFAULT_DEVICE_ID;
    if (!GetLocationDeviceId(localDeviceId)) {
        SEN_HILOGW("Cant fand local deviceId, default loacl deviceId :%{public}d", localDeviceId);
    }
    sensorDesc.deviceId = localDeviceId;
    sensorDesc.sensorId = DEFAULT_SENSOR_ID;
    sensorDesc.location = IS_LOCAL_DEVICE;
    if (argc >= ARGC_NUM_TWO && IsMatchType(env, args, napi_object)) {
        if (!GetDeviceId(env, args, sensorDesc.deviceId)) {
            SEN_HILOGW("No deviceId, This device is selected by default");
            sensorDesc.deviceId = localDeviceId;
        }
        if (!GetSensorId(env, args, sensorDesc.sensorId)) {
            sensorDesc.sensorId = DEFAULT_SENSOR_ID;
            SEN_HILOGW("No sensorId, The first sensor of the type is selected by default");
        }
    } else if ((argc == 1) || IsMatchType(env, args, napi_undefined) ||
        IsMatchType(env, args, napi_null)) {
            SEN_HILOGW("no deviceId, sensorIndex, Select the default deviceId and sensorIndex.");
    } else {
        return false;
    }
    if (sensorDesc.deviceId != localDeviceId) {
        sensorDesc.location = NON_LOCAL_DEVICE;
    }
    return true;
}

static bool GetSensorType(napi_env env, napi_value args, SensorDescription &sensorDesc)
{
    sensorDesc.sensorType = INVALID_SENSOR_TYPE;
    if ((!IsMatchType(env, args, napi_number)) || (!GetNativeInt32(env, args, sensorDesc.sensorType))) {
        return false;
    }
    int32_t localDeviceId = DEFAULT_DEVICE_ID;
    if (!GetLocationDeviceId(localDeviceId)) {
        SEN_HILOGW("Cant fand local deviceId, default loacl deviceId :%{public}d", localDeviceId);
    }
    sensorDesc.deviceId = localDeviceId;
    sensorDesc.sensorId = DEFAULT_SENSOR_ID;
    sensorDesc.location = IS_LOCAL_DEVICE;
    return true;
}

static napi_value Off(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 3;
    napi_value args[3] = { 0 };
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr);
    if (status != napi_ok || argc < 1) {
        ThrowErr(env, PARAMETER_ERROR, "napi_get_cb_info fail or number of parameter invalid");
        return nullptr;
    }
    if (IsMatchType(env, args[0], napi_string)) {
        return OffPlugSensor(env, argc, args[0], args[1]);
    }
    SensorDescription sensorDesc;
    if (!GetSensorType(env, args[0], sensorDesc)) {
        ThrowErr(env, PARAMETER_ERROR, "Wrong argument type or get number fail");
        return nullptr;
    }
    napi_value args_tmp = args[1];
    if (!IsMatchType(env, args_tmp, napi_function)) {
        args_tmp = args[ARGS_NUM_TWO];
        if (!GetSensorInfoParameter(env, argc, args[1], sensorDesc)) {
            ThrowErr(env, PARAMETER_ERROR, "Wrong argument type, args[1] should is napi_object");
            return nullptr;
        }
    }
    int32_t subscribeSize = INVALID_SUBSCRIBE_SIZE;
    if (argc == 1) {
        subscribeSize = RemoveAllCallback(env, sensorDesc);
    } else if (IsMatchType(env, args_tmp, napi_undefined) ||
        IsMatchType(env, args_tmp, napi_null)) {
        subscribeSize = RemoveAllCallback(env, sensorDesc);
    } else if (IsMatchType(env, args_tmp, napi_function)) {
        subscribeSize = RemoveCallback(env, sensorDesc, args_tmp);
    } else {
        ThrowErr(env, PARAMETER_ERROR, "Wrong argument type, args[2] should is napi_function");
        return nullptr;
    }
    if (CheckSystemSubscribe(sensorDesc) || (subscribeSize > DEFAULT_SUBSCRIBE_SIZE)) {
        SEN_HILOGW("There are other client subscribe system js api as well, not need unsubscribe");
        return nullptr;
    }
    int32_t ret = UnsubscribeSensor(sensorDesc);
    if (ret == PARAMETER_ERROR || ret == PERMISSION_DENIED) {
        ThrowErr(env, ret, "UnsubscribeSensor fail");
    }
    return nullptr;
}

static napi_value EmitAsyncWork(napi_value param, sptr<AsyncCallbackInfo> info)
{
    CHKPP(info);
    napi_status status = napi_generic_failure;
    if (param != nullptr) {
        status = napi_create_reference(info->env, param, 1, &info->callback[0]);
        if (status != napi_ok) {
            SEN_HILOGE("napi_create_reference fail");
            return nullptr;
        }
        EmitAsyncCallbackWork(info);
        return nullptr;
    }
    napi_value promise = nullptr;
    status = napi_create_promise(info->env, &info->deferred, &promise);
    if (status != napi_ok) {
        SEN_HILOGE("napi_create_promise fail");
        return nullptr;
    }
    EmitPromiseWork(info);
    return promise;
}

static napi_value GetGeomagneticField(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 3;
    napi_value args[3] = { 0 };
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr);
    if (status != napi_ok || argc < 2) {
        ThrowErr(env, PARAMETER_ERROR, "napi_get_cb_info fail or number of parameter invalid");
        return nullptr;
    }
    if ((!IsMatchType(env, args[0], napi_object)) || (!IsMatchType(env, args[1], napi_number))) {
        ThrowErr(env, PARAMETER_ERROR, "Wrong argument type");
        return nullptr;
    }
    napi_value napiLatitude = GetNamedProperty(env, args[0], "latitude");
    if (napiLatitude == nullptr) {
        ThrowErr(env, PARAMETER_ERROR, "napiLatitude is null");
        return nullptr;
    }
    double latitude = 0;
    if (!GetNativeDouble(env, napiLatitude, latitude)) {
        ThrowErr(env, PARAMETER_ERROR, "Get latitude fail");
        return nullptr;
    }
    napi_value napiLongitude = GetNamedProperty(env, args[0], "longitude");
    if (napiLongitude == nullptr) {
        ThrowErr(env, PARAMETER_ERROR, "napiLongitude is null");
        return nullptr;
    }
    double longitude = 0;
    if (!GetNativeDouble(env, napiLongitude, longitude)) {
        ThrowErr(env, PARAMETER_ERROR, "Get longitude fail");
        return nullptr;
    }
    napi_value napiAltitude = GetNamedProperty(env, args[0], "altitude");
    if (napiAltitude == nullptr) {
        ThrowErr(env, PARAMETER_ERROR, "napiAltitude is null");
        return nullptr;
    }
    double altitude = 0;
    if (!GetNativeDouble(env, napiAltitude, altitude)) {
        ThrowErr(env, PARAMETER_ERROR, "Get altitude fail");
        return nullptr;
    }
    int64_t timeMillis = 0;
    if (!GetNativeInt64(env, args[1], timeMillis)) {
        ThrowErr(env, PARAMETER_ERROR, "Get timeMillis fail");
        return nullptr;
    }
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
    if (argc >= 3 && IsMatchType(env, args[2], napi_function)) {
        return EmitAsyncWork(args[2], asyncCallbackInfo);
    }
    return EmitAsyncWork(nullptr, asyncCallbackInfo);
}

static napi_value GetAxisX(napi_env env, napi_value value)
{
    return GetNamedProperty(env, value, "x");
}

static napi_value GetAxisY(napi_env env, napi_value value)
{
    return GetNamedProperty(env, value, "y");
}

static napi_value TransformCoordinateSystem(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 3;
    napi_value args[3]  = { 0 };
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr);
    if (status != napi_ok || argc < 2) {
        ThrowErr(env, PARAMETER_ERROR, "napi_get_cb_info fail or number of parameter invalid");
        return nullptr;
    }
    if ((!IsMatchArrayType(env, args[0])) || (!IsMatchType(env, args[1], napi_object))) {
        ThrowErr(env, PARAMETER_ERROR, "Wrong argument type");
        return nullptr;
    }
    std::vector<float> inRotationVector;
    if (!GetFloatArray(env, args[0], inRotationVector)) {
        ThrowErr(env, PARAMETER_ERROR, "Get inRotationVector fail");
        return nullptr;
    }
    size_t length = inRotationVector.size();
    if ((length != DATA_LENGTH) && (length != THREE_DIMENSIONAL_MATRIX_LENGTH)) {
        ThrowErr(env, PARAMETER_ERROR, "Wrong inRotationVector length");
        return nullptr;
    }
    napi_value napiAxisX = GetAxisX(env, args[1]);
    if (napiAxisX == nullptr) {
        ThrowErr(env, PARAMETER_ERROR, "napiAxisX is null");
        return nullptr;
    }
    int32_t axisX = 0;
    if (!GetNativeInt32(env, napiAxisX, axisX)) {
        ThrowErr(env, PARAMETER_ERROR, "Get axisY fail");
        return nullptr;
    }
    napi_value napiAxisY = GetAxisY(env, args[1]);
    if (napiAxisY == nullptr) {
        ThrowErr(env, PARAMETER_ERROR, "napiAxisY is null");
        return nullptr;
    }
    int32_t axisY = 0;
    if (!GetNativeInt32(env, napiAxisY, axisY)) {
        ThrowErr(env, PARAMETER_ERROR, "Get axisY fail");
        return nullptr;
    }
    sptr<AsyncCallbackInfo> asyncCallbackInfo =
        new (std::nothrow) AsyncCallbackInfo(env, TRANSFORM_COORDINATE_SYSTEM);
    CHKPP(asyncCallbackInfo);
    std::vector<float> outRotationVector(length);
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.TransformCoordinateSystem(inRotationVector, axisX, axisY, outRotationVector);
    if (ret != OHOS::ERR_OK) {
        ThrowErr(env, ret, "Transform coordinate system fail");
        return nullptr;
    } else {
        for (size_t i = 0; i < length; ++i) {
            asyncCallbackInfo->data.reserveData.reserve[i] = outRotationVector[i];
        }
        asyncCallbackInfo->data.reserveData.length = static_cast<int32_t>(length);
    }
    if (argc >= 3 && IsMatchType(env, args[2], napi_function)) {
        return EmitAsyncWork(args[2], asyncCallbackInfo);
    }
    return EmitAsyncWork(nullptr, asyncCallbackInfo);
}

static napi_value GetAngleModify(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 3;
    napi_value args[3] = { 0 };
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr);
    if (status != napi_ok || argc < 2) {
        ThrowErr(env, PARAMETER_ERROR, "napi_get_cb_info fail or number of parameter invalid");
        return nullptr;
    }
    if (!IsMatchArrayType(env, args[0])) {
        ThrowErr(env, PARAMETER_ERROR, "Wrong argument type, should be array");
        return nullptr;
    }
    if (!IsMatchArrayType(env, args[1])) {
        ThrowErr(env, PARAMETER_ERROR, "Wrong argument type, should be array");
        return nullptr;
    }
    sptr<AsyncCallbackInfo> asyncCallbackInfo =
        new (std::nothrow) AsyncCallbackInfo(env, GET_ANGLE_MODIFY);
    CHKPP(asyncCallbackInfo);
    std::vector<float> curRotationVector;
    if (!GetFloatArray(env, args[0], curRotationVector)) {
        ThrowErr(env, PARAMETER_ERROR, "Get curRotationVector fail");
        return nullptr;
    }
    std::vector<float> preRotationVector;
    if (!GetFloatArray(env, args[1], preRotationVector)) {
        ThrowErr(env, PARAMETER_ERROR, "Get preRotationVector fail");
        return nullptr;
    }
    std::vector<float> angleChange(ROTATION_VECTOR_LENGTH);
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.GetAngleModify(curRotationVector, preRotationVector, angleChange);
    if (ret != OHOS::ERR_OK) {
        ThrowErr(env, ret, "Get angle modify fail");
        return nullptr;
    } else {
        asyncCallbackInfo->data.reserveData.length = ROTATION_VECTOR_LENGTH;
        for (int32_t i = 0; i < ROTATION_VECTOR_LENGTH; ++i) {
            asyncCallbackInfo->data.reserveData.reserve[i] = angleChange[i];
        }
    }
    if (argc >= 3 && IsMatchType(env, args[2], napi_function)) {
        return EmitAsyncWork(args[2], asyncCallbackInfo);
    }
    return EmitAsyncWork(nullptr, asyncCallbackInfo);
}

static napi_value GetDirection(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 3;
    napi_value args[3] = { 0 };
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr);
    if (status != napi_ok || argc < 1) {
        ThrowErr(env, PARAMETER_ERROR, "napi_get_cb_info fail or number of parameter invalid");
        return nullptr;
    }
    if (!IsMatchArrayType(env, args[0])) {
        ThrowErr(env, PARAMETER_ERROR, "Wrong argument type, should be array");
        return nullptr;
    }
    sptr<AsyncCallbackInfo> asyncCallbackInfo =
        new (std::nothrow) AsyncCallbackInfo(env, GET_DIRECTION);
    CHKPP(asyncCallbackInfo);
    std::vector<float> rotationMatrix;
    if (!GetFloatArray(env, args[0], rotationMatrix)) {
        ThrowErr(env, PARAMETER_ERROR, "Get rotationMatrix fail");
        return nullptr;
    }
    std::vector<float> rotationAngle(ROTATION_VECTOR_LENGTH);
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.GetDirection(rotationMatrix, rotationAngle);
    if (ret != OHOS::ERR_OK) {
        ThrowErr(env, ret, "Get direction fail");
        return nullptr;
    } else {
        asyncCallbackInfo->data.reserveData.length = ROTATION_VECTOR_LENGTH;
        for (int32_t i = 0; i < ROTATION_VECTOR_LENGTH; ++i) {
            asyncCallbackInfo->data.reserveData.reserve[i] = rotationAngle[i];
        }
    }
    if (argc >= 2 && IsMatchType(env, args[1], napi_function)) {
        return EmitAsyncWork(args[1], asyncCallbackInfo);
    }
    return EmitAsyncWork(nullptr, asyncCallbackInfo);
}

static napi_value CreateQuaternion(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr);
    if (status != napi_ok || argc < 1) {
        ThrowErr(env, PARAMETER_ERROR, "napi_get_cb_info failed or number of parameter invalid");
        return nullptr;
    }
    if (!IsMatchArrayType(env, args[0])) {
        ThrowErr(env, PARAMETER_ERROR, "Wrong argument type, should be array");
        return nullptr;
    }
    sptr<AsyncCallbackInfo> asyncCallbackInfo =
        new (std::nothrow) AsyncCallbackInfo(env, CREATE_QUATERNION);
    CHKPP(asyncCallbackInfo);
    std::vector<float> rotationVector;
    if (!GetFloatArray(env, args[0], rotationVector)) {
        ThrowErr(env, PARAMETER_ERROR, "Get rotationVector failed");
        return nullptr;
    }
    std::vector<float> quaternion(QUATERNION_LENGTH);
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.CreateQuaternion(rotationVector, quaternion);
    if (ret != OHOS::ERR_OK) {
        ThrowErr(env, ret, "CreateQuaternion fail");
        return nullptr;
    } else {
        asyncCallbackInfo->data.reserveData.length = QUATERNION_LENGTH;
        for (int32_t i = 0; i < QUATERNION_LENGTH; ++i) {
            asyncCallbackInfo->data.reserveData.reserve[i] = quaternion[i];
        }
    }
    if (argc >= 2 && IsMatchType(env, args[1], napi_function)) {
        return EmitAsyncWork(args[1], asyncCallbackInfo);
    }
    return EmitAsyncWork(nullptr, asyncCallbackInfo);
}

static napi_value GetAltitude(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 3;
    napi_value args[3] = { 0 };
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr);
    if (status != napi_ok || argc < 2) {
        ThrowErr(env, PARAMETER_ERROR, "napi_get_cb_info fail or number of parameter invalid");
        return nullptr;
    }
    if ((!IsMatchType(env, args[0], napi_number)) || (!IsMatchType(env, args[1], napi_number))) {
        ThrowErr(env, PARAMETER_ERROR, "Wrong argument type");
        return nullptr;
    }
    sptr<AsyncCallbackInfo> asyncCallbackInfo =
        new (std::nothrow) AsyncCallbackInfo(env, GET_ALTITUDE);
    CHKPP(asyncCallbackInfo);
    float seaPressure = 0;
    if (!GetNativeFloat(env, args[0], seaPressure)) {
        ThrowErr(env, PARAMETER_ERROR, "Wrong argument type, get seaPressure fail");
        return nullptr;
    }
    float currentPressure = 0;
    if (!GetNativeFloat(env, args[1], currentPressure)) {
        ThrowErr(env, PARAMETER_ERROR, "Wrong argument type, get currentPressure fail");
        return nullptr;
    }
    float altitude = 0;
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.GetAltitude(seaPressure, currentPressure, &altitude);
    if (ret != OHOS::ERR_OK) {
        ThrowErr(env, ret, "Get altitude fail");
        return nullptr;
    } else {
        asyncCallbackInfo->data.reserveData.reserve[0] = altitude;
    }
    if (argc >= 3 && IsMatchType(env, args[2], napi_function)) {
        return EmitAsyncWork(args[2], asyncCallbackInfo);
    }
    return EmitAsyncWork(nullptr, asyncCallbackInfo);
}

static napi_value GetGeomagneticDip(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr);
    if (status != napi_ok || argc < 1) {
        ThrowErr(env, PARAMETER_ERROR, "napi_get_cb_info fail or number of parameter invalid");
        return nullptr;
    }
    if (!IsMatchArrayType(env, args[0])) {
        ThrowErr(env, PARAMETER_ERROR, "Wrong argument type, should be array");
        return nullptr;
    }
    sptr<AsyncCallbackInfo> asyncCallbackInfo =
        new (std::nothrow) AsyncCallbackInfo(env, GET_GEOMAGNETIC_DIP);
    CHKPP(asyncCallbackInfo);
    std::vector<float> inclinationMatrix;
    if (!GetFloatArray(env, args[0], inclinationMatrix)) {
        ThrowErr(env, PARAMETER_ERROR, "Get inclinationMatrix fail");
        return nullptr;
    }
    float geomagneticDip = 0;
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.GetGeomagneticDip(inclinationMatrix, &geomagneticDip);
    if (ret != OHOS::ERR_OK) {
        ThrowErr(env, ret, "Get geomagnetic dip fail");
        return nullptr;
    } else {
        asyncCallbackInfo->data.reserveData.reserve[0] = geomagneticDip;
    }
    if (argc >= 2 && IsMatchType(env, args[1], napi_function)) {
        return EmitAsyncWork(args[1], asyncCallbackInfo);
    }
    return EmitAsyncWork(nullptr, asyncCallbackInfo);
}

static napi_value CreateRotationAndInclination(const napi_env &env, napi_value args[], size_t argc)
{
    CALL_LOG_ENTER;
    if (argc < 2) {
        ThrowErr(env, PARAMETER_ERROR, "The number of parameters is not valid");
        return nullptr;
    }
    std::vector<float> gravity;
    if (!GetFloatArray(env, args[0], gravity)) {
        ThrowErr(env, PARAMETER_ERROR, "Get gravity fail");
        return nullptr;
    }
    std::vector<float> geomagnetic;
    if (!GetFloatArray(env, args[1], geomagnetic)) {
        ThrowErr(env, PARAMETER_ERROR, "Get geomagnetic fail");
        return nullptr;
    }
    std::vector<float> rotation(THREE_DIMENSIONAL_MATRIX_LENGTH);
    std::vector<float> inclination(THREE_DIMENSIONAL_MATRIX_LENGTH);
    sptr<AsyncCallbackInfo> asyncCallbackInfo =
        new (std::nothrow) AsyncCallbackInfo(env, ROTATION_INCLINATION_MATRIX);
    CHKPP(asyncCallbackInfo);
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.CreateRotationAndInclination(gravity, geomagnetic, rotation, inclination);
    if (ret != OHOS::ERR_OK) {
        ThrowErr(env, ret, "Create rotation and inclination matrix fail");
        return nullptr;
    } else {
        asyncCallbackInfo->data.reserveData.length = THREE_DIMENSIONAL_MATRIX_LENGTH;
        for (int32_t i = 0; i < THREE_DIMENSIONAL_MATRIX_LENGTH; ++i) {
            asyncCallbackInfo->data.rationMatrixData.rotationMatrix[i] = rotation[i];
        }
        for (int32_t i = 0; i < THREE_DIMENSIONAL_MATRIX_LENGTH; ++i) {
            asyncCallbackInfo->data.rationMatrixData.inclinationMatrix[i] = inclination[i];
        }
    }
    if (argc >= 3 && IsMatchType(env, args[2], napi_function)) {
        return EmitAsyncWork(args[2], asyncCallbackInfo);
    }
    return EmitAsyncWork(nullptr, asyncCallbackInfo);
}

static napi_value GetRotationMatrix(const napi_env &env, napi_value args[], size_t argc)
{
    CALL_LOG_ENTER;
    if (argc < 1) {
        ThrowErr(env, PARAMETER_ERROR, "The number of parameters is not valid");
        return nullptr;
    }
    std::vector<float> rotationVector;
    if (!GetFloatArray(env, args[0], rotationVector)) {
        ThrowErr(env, PARAMETER_ERROR, "Get rotationVector fail");
        return nullptr;
    }
    sptr<AsyncCallbackInfo> asyncCallbackInfo =
        new (std::nothrow) AsyncCallbackInfo(env, CREATE_ROTATION_MATRIX);
    CHKPP(asyncCallbackInfo);
    std::vector<float> rotationMatrix(THREE_DIMENSIONAL_MATRIX_LENGTH);
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.CreateRotationMatrix(rotationVector, rotationMatrix);
    if (ret != OHOS::ERR_OK) {
        ThrowErr(env, ret, "Create rotation matrix fail");
        return nullptr;
    } else {
        asyncCallbackInfo->data.reserveData.length = THREE_DIMENSIONAL_MATRIX_LENGTH;
        for (int32_t i = 0; i < THREE_DIMENSIONAL_MATRIX_LENGTH; ++i) {
            asyncCallbackInfo->data.reserveData.reserve[i] = rotationMatrix[i];
        }
    }
    if (argc >= 2 && IsMatchType(env, args[1], napi_function)) {
        return EmitAsyncWork(args[1], asyncCallbackInfo);
    }
    return EmitAsyncWork(nullptr, asyncCallbackInfo);
}

static napi_value CreateRotationMatrix(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 3;
    napi_value args[3] = { 0 };
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr);
    if (status != napi_ok || argc < 1) {
        ThrowErr(env, PARAMETER_ERROR, "napi_get_cb_info fail or number of parameter invalid");
        return nullptr;
    }
    if (!IsMatchArrayType(env, args[0])) {
        ThrowErr(env, PARAMETER_ERROR, "Wrong argument type, should be array");
        return nullptr;
    }
    if (argc >= 2 && IsMatchArrayType(env, args[1])) {
        return CreateRotationAndInclination(env, args, argc);
    } else {
        return GetRotationMatrix(env, args, argc);
    }
}

static napi_value GetSensorList(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr);
    if (status != napi_ok) {
        ThrowErr(env, PARAMETER_ERROR, "napi_get_cb_info fail");
        return nullptr;
    }
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
            if ((sensorInfos[i].sensorTypeId == SENSOR_TYPE_ID_AMBIENT_LIGHT1) ||
                (sensorInfos[i].sensorTypeId == SENSOR_TYPE_ID_PROXIMITY1) ||
                (sensorInfos[i].sensorTypeId > GL_SENSOR_TYPE_PRIVATE_MIN_VALUE)) {
                SEN_HILOGD("This sensor is secondary ambient light");
                continue;
            }
            asyncCallbackInfo->sensorInfos.push_back(*(sensorInfos + i));
        }
    }
    if (argc >= 1 && IsMatchType(env, args[0], napi_function)) {
        return EmitAsyncWork(args[0], asyncCallbackInfo);
    }
    return EmitAsyncWork(nullptr, asyncCallbackInfo);
}

static napi_value GetSensorListSync(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 0;
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, nullptr, &thisVar, nullptr);
    if (status != napi_ok) {
        ThrowErr(env, PARAMETER_ERROR, "napi_get_cb_info fail");
        return result;
    }
    SensorInfo *sensorInfos = nullptr;
    int32_t count = 0;
    int32_t ret = GetAllSensors(&sensorInfos, &count);
    if (ret != OHOS::ERR_OK) {
        ThrowErr(env, ret, "Get sensor list fail");
        return result;
    }
    vector<SensorInfo> sensorInfoVec;
    for (int32_t i = 0; i < count; ++i) {
        if ((sensorInfos[i].sensorTypeId == SENSOR_TYPE_ID_AMBIENT_LIGHT1) ||
            (sensorInfos[i].sensorTypeId == SENSOR_TYPE_ID_PROXIMITY1) ||
            (sensorInfos[i].sensorTypeId > GL_SENSOR_TYPE_PRIVATE_MIN_VALUE)) {
            SEN_HILOGD("This sensor is secondary ambient light");
            continue;
        }
        sensorInfoVec.push_back(*(sensorInfos + i));
    }
    if (napi_create_array(env, &result) != napi_ok) {
        ThrowErr(env, PARAMETER_ERROR, "napi_create_array fail");
        return result;
    }
    for (uint32_t i = 0; i < sensorInfoVec.size(); ++i) {
        napi_value value = nullptr;
        if (!ConvertToSensorInfo(env, sensorInfoVec[i], value)) {
            ThrowErr(env, PARAMETER_ERROR, "Convert sensor info fail");
            return result;
        }
        if (napi_set_element(env, result, i, value) != napi_ok) {
            ThrowErr(env, PARAMETER_ERROR, "napi_set_element fail");
        }
    }
    return result;
}

static napi_value GetSingleSensor(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr);
    if (status != napi_ok || argc < 1) {
        ThrowErr(env, PARAMETER_ERROR, "napi_get_cb_info fail or number of parameter invalid");
        return nullptr;
    }
    int32_t sensorTypeId = INVALID_SENSOR_TYPE;
    if (!GetNativeInt32(env, args[0], sensorTypeId)) {
        ThrowErr(env, PARAMETER_ERROR, "Wrong argument type, get number fail");
        return nullptr;
    }
    int32_t deviceId = DEFAULT_DEVICE_ID;
    if (!GetLocationDeviceId(deviceId)) {
        SEN_HILOGW("Cant fand local deviceId, default deviceId :%{public}d", deviceId);
    }
    sptr<AsyncCallbackInfo> asyncCallbackInfo = new (std::nothrow) AsyncCallbackInfo(env, GET_SINGLE_SENSOR);
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
            if ((sensorInfos[i].sensorTypeId == SENSOR_TYPE_ID_AMBIENT_LIGHT1) ||
                (sensorInfos[i].sensorTypeId == SENSOR_TYPE_ID_PROXIMITY1) ||
                (sensorInfos[i].sensorTypeId > GL_SENSOR_TYPE_PRIVATE_MIN_VALUE)) {
                SEN_HILOGD("This sensor is secondary ambient light");
                continue;
            }
            if (sensorInfos[i].deviceId == deviceId && sensorInfos[i].sensorTypeId == sensorTypeId) {
                asyncCallbackInfo->sensorInfos.push_back(*(sensorInfos + i));
                break;
            }
        }
        if (asyncCallbackInfo->sensorInfos.empty()) {
            uint32_t targetVersion = 0;
            if (GetSelfTargetVersion(targetVersion) && (targetVersion < COMPATIBILITY_CHANGE_VERSION_API12)) {
                ThrowErr(env, PARAMETER_ERROR, "The sensor is not supported by the device");
                return nullptr;
            }
            ThrowErr(env, SENSOR_NO_SUPPORT, "The sensor is not supported by the device");
            return nullptr;
        }
    }
    if (argc >= 2 && IsMatchType(env, args[1], napi_function)) {
        return EmitAsyncWork(args[1], asyncCallbackInfo);
    }
    return EmitAsyncWork(nullptr, asyncCallbackInfo);
}

static napi_value GetSingleSensorSync(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr);
    if (status != napi_ok || argc == 0) {
        ThrowErr(env, PARAMETER_ERROR, "napi_get_cb_info fail or number of parameter invalid");
        return result;
    }
    int32_t sensorTypeId = INVALID_SENSOR_TYPE;
    if (!GetNativeInt32(env, args[0], sensorTypeId)) {
        ThrowErr(env, PARAMETER_ERROR, "Wrong argument type, get number fail");
        return result;
    }
    int32_t deviceId = DEFAULT_DEVICE_ID;
    if (!GetLocationDeviceId(deviceId)) {
        SEN_HILOGW("Cant fand local deviceId, default deviceId :%{public}d", deviceId);
    }
    SensorInfo *sensorInfos = nullptr;
    int32_t count = 0;
    int32_t ret = GetAllSensors(&sensorInfos, &count);
    if (ret != OHOS::ERR_OK) {
        ThrowErr(env, ret, "Get sensor list fail");
        return result;
    }
    vector<SensorInfo> sensorInfoVec;
    for (int32_t i = 0; i < count; ++i) {
        if ((sensorInfos[i].sensorTypeId == SENSOR_TYPE_ID_AMBIENT_LIGHT1) ||
            (sensorInfos[i].sensorTypeId == SENSOR_TYPE_ID_PROXIMITY1) ||
            (sensorInfos[i].sensorTypeId > GL_SENSOR_TYPE_PRIVATE_MIN_VALUE)) {
            SEN_HILOGD("This sensor is secondary ambient light");
            continue;
        }
        if (sensorInfos[i].deviceId == deviceId && sensorInfos[i].sensorTypeId == sensorTypeId) {
            sensorInfoVec.push_back(*(sensorInfos + i));
            break;
        }
    }
    if (sensorInfoVec.empty()) {
        ThrowErr(env, SENSOR_NO_SUPPORT, "The sensor is not supported by the device");
        return result;
    }
    if (!ConvertToSensorInfo(env, sensorInfoVec[0], result)) {
        ThrowErr(env, PARAMETER_ERROR, "Convert sensor info fail");
    }
    return result;
}

static void FilteringSensorList(SensorInfo *sensorInfos, vector<SensorInfo> &callbackSensorInfo, int32_t count)
{
    for (int32_t i = 0; i < count; ++i) {
        if ((sensorInfos[i].sensorTypeId == SENSOR_TYPE_ID_AMBIENT_LIGHT1) ||
            (sensorInfos[i].sensorTypeId == SENSOR_TYPE_ID_PROXIMITY1) ||
            (sensorInfos[i].sensorTypeId > GL_SENSOR_TYPE_PRIVATE_MIN_VALUE)) {
            SEN_HILOGD("This sensor is secondary ambient light");
            continue;
        }
        callbackSensorInfo.push_back(*(sensorInfos + i));
    }
    return;
}

static napi_value GetSensorListByDeviceSync(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    if (napi_create_array(env, &result) != napi_ok) {
        SEN_HILOGE("napi_create_array fail");
        return result;
    }
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr);
    if (status != napi_ok) {
        SEN_HILOGE("napi_get_cb_info fail or number of parameter invalid");
        return result;
    }
    int32_t deviceId = DEFAULT_DEVICE_ID;
    if (!GetNativeInt32(env, args[0], deviceId)) {
        SEN_HILOGW("Get deviceId failed");
        if (!GetLocationDeviceId(deviceId)) {
            SEN_HILOGW("Cant fand local deviceId, default deviceId :%{public}d", deviceId);
        }
    }
    SensorInfo *sensorInfos = nullptr;
    int32_t count = 0;
    int32_t ret = GetDeviceSensors(deviceId, &sensorInfos, &count);
    if (ret != OHOS::ERR_OK) {
        SEN_HILOGE("Get deviceSensorList failed, ret:%{public}d", deviceId);
        return result;
    }
    vector<SensorInfo> sensorInfoVec;
    FilteringSensorList(sensorInfos, sensorInfoVec, count);
    for (uint32_t i = 0; i < sensorInfoVec.size(); ++i) {
        napi_value value = nullptr;
        if (!ConvertToSensorInfo(env, sensorInfoVec[i], value)) {
            SEN_HILOGE("Convert sensor info fail");
            return result;
        }
        if (napi_set_element(env, result, i, value) != napi_ok) {
            SEN_HILOGE("napi_set_element fail");
        }
    }
    return result;
}

static void FilteringSingleSensorList(SensorInfo *sensorInfos, vector<SensorInfo> &callbackSensorInfo, int32_t count,
    int32_t sensorTypeId)
{
    for (int32_t i = 0; i < count; ++i) {
        if ((sensorInfos[i].sensorTypeId == SENSOR_TYPE_ID_AMBIENT_LIGHT1) ||
            (sensorInfos[i].sensorTypeId == SENSOR_TYPE_ID_PROXIMITY1) ||
            (sensorInfos[i].sensorTypeId > GL_SENSOR_TYPE_PRIVATE_MIN_VALUE)) {
            SEN_HILOGD("This sensor is secondary ambient light");
            continue;
        }
        if (sensorInfos[i].sensorTypeId == sensorTypeId) {
            callbackSensorInfo.push_back(*(sensorInfos + i));
        }
    }
    return;
}

static napi_value GetSingleSensorByDeviceSync(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    if (napi_create_array(env, &result) != napi_ok) {
        SEN_HILOGE("napi_create_array fail");
        return result;
    }
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr);
    if (status != napi_ok || argc < 1) {
        SEN_HILOGE("napi_get_cb_info fail or number of parameter invalid");
        return result;
    }
    int32_t sensorTypeId = INVALID_SENSOR_TYPE;
    if (!GetNativeInt32(env, args[0], sensorTypeId)) {
        SEN_HILOGE("Wrong argument type, get number fail");
        return result;
    }
    int32_t deviceId = DEFAULT_DEVICE_ID;
    if (!GetNativeInt32(env, args[1], deviceId)) {
        SEN_HILOGW("Get deviceId failed");
        if (!GetLocationDeviceId(deviceId)) {
            SEN_HILOGW("Cant fand local deviceId, default deviceId :%{public}d", deviceId);
        }
    }
    SensorInfo *sensorInfos = nullptr;
    int32_t count = 0;
    int32_t ret = GetDeviceSensors(deviceId, &sensorInfos, &count);
    if (ret != OHOS::ERR_OK) {
        SEN_HILOGE("Get sensor list fail");
        return result;
    }
    vector<SensorInfo> sensorInfoVec;
    FilteringSingleSensorList(sensorInfos, sensorInfoVec, count, sensorTypeId);
    for (uint32_t i = 0; i < sensorInfoVec.size(); ++i) {
        napi_value value = nullptr;
        if (!ConvertToSensorInfo(env, sensorInfoVec[i], value)) {
            SEN_HILOGE("Convert sensor info fail");
            return result;
        }
        if (napi_set_element(env, result, i, value) != napi_ok) {
            SEN_HILOGE("napi_set_element fail");
        }
    }
    return result;
}

bool RegisterSubscribeCallback(napi_env env, napi_value args, sptr<AsyncCallbackInfo> &asyncCallbackInfo,
    napi_value &napiFail, string interval)
{
    napi_value napiSuccess = GetNamedProperty(env, args, "success");
    if (!IsMatchType(env, napiSuccess, napi_function)) {
        SEN_HILOGE("get napiSuccess fail");
        return false;
    }
    if (!RegisterNapiCallback(env, napiSuccess, asyncCallbackInfo->callback[0])) {
        SEN_HILOGE("register success callback fail");
        return false;
    }
    napiFail = GetNamedProperty(env, args, "fail");
    if (IsMatchType(env, napiFail, napi_function)) {
        SEN_HILOGD("Has fail callback");
        if (!RegisterNapiCallback(env, napiFail, asyncCallbackInfo->callback[1])) {
            SEN_HILOGE("register fail callback fail");
            return false;
        }
    }
    if (auto iter = g_samplingPeriod.find(interval); iter == g_samplingPeriod.end()) {
        if (!IsMatchType(env, napiFail, napi_function)) {
            SEN_HILOGE("input error, interval is invalid");
            return false;
        }
        CreateFailMessage(SUBSCRIBE_FAIL, INPUT_ERROR, "input error", asyncCallbackInfo);
        EmitAsyncCallbackWork(asyncCallbackInfo);
        return false;
    }
    return true;
}

napi_value Subscribe(napi_env env, napi_callback_info info, int32_t sensorTypeId, CallbackDataType type)
{
    CALL_LOG_ENTER;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr);
    if (status != napi_ok || argc < 1) {
        ThrowErr(env, PARAMETER_ERROR, "napi_get_cb_info fail or number of parameter invalid");
        return nullptr;
    }
    if (!IsMatchType(env, args[0], napi_object)) {
        ThrowErr(env, PARAMETER_ERROR, "Wrong argument type, should be object");
        return nullptr;
    }
    string interval = "normal";
    if ((sensorTypeId == SENSOR_TYPE_ID_ACCELEROMETER) ||
        ((sensorTypeId == SENSOR_TYPE_ID_ORIENTATION) && (type != SUBSCRIBE_COMPASS))
        || (sensorTypeId == SENSOR_TYPE_ID_GYROSCOPE)) {
        napi_value napiInterval = GetNamedProperty(env, args[0], "interval");
        CHKCP(GetStringValue(env, napiInterval, interval), "get interval fail");
    }
    sptr<AsyncCallbackInfo> asyncCallbackInfo = new (std::nothrow) AsyncCallbackInfo(env, type);
    CHKPP(asyncCallbackInfo);
    napi_value napiFail;
    CHKCP(RegisterSubscribeCallback(env, args[0], asyncCallbackInfo, napiFail, interval), "register callback failed");
    int32_t deviceId = DEFAULT_DEVICE_ID;
    if (!GetLocationDeviceId(deviceId)) {
        SEN_HILOGW("Cant fand local deviceId, default deviceId :%{public}d", deviceId);
    }
    int32_t ret = SubscribeSensor({deviceId, sensorTypeId, DEFAULT_SENSOR_ID, IS_LOCAL_DEVICE},
        g_samplingPeriod[interval], DataCallbackImpl);
    if (ret != OHOS::ERR_OK) {
        CHKCP(IsMatchType(env, napiFail, napi_function), "subscribe fail");
        CreateFailMessage(SUBSCRIBE_FAIL, SENSOR_SUBSCRIBE_FAILURE, "subscribe fail", asyncCallbackInfo);
        EmitAsyncCallbackWork(asyncCallbackInfo);
        return nullptr;
    }
    std::lock_guard<std::mutex> subscribeLock(g_mutex);
    std::vector<sptr<AsyncCallbackInfo>> callbackInfos = g_subscribeCallbacks[{deviceId, sensorTypeId,
        DEFAULT_SENSOR_ID, IS_LOCAL_DEVICE}];
    callbackInfos.push_back(asyncCallbackInfo);
    g_subscribeCallbacks[{deviceId, sensorTypeId, DEFAULT_SENSOR_ID, IS_LOCAL_DEVICE}] = callbackInfos;
    return nullptr;
}

static bool RemoveSubscribeCallback(napi_env env, SensorDescription sensorDesc)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> subscribeCallbackLock(g_mutex);
    std::vector<sptr<AsyncCallbackInfo>> callbackInfos = g_subscribeCallbacks[sensorDesc];
    for (auto iter = callbackInfos.begin(); iter != callbackInfos.end();) {
        CHKPC(*iter);
        if ((*iter)->env != env) {
            ++iter;
            continue;
        }
        iter = callbackInfos.erase(iter);
    }
    if (callbackInfos.empty()) {
        g_subscribeCallbacks.erase(sensorDesc);
        return true;
    }
    return false;
}

napi_value Unsubscribe(napi_env env, napi_callback_info info, int32_t sensorTypeId)
{
    CALL_LOG_ENTER;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr);
    if (status != napi_ok) {
        ThrowErr(env, PARAMETER_ERROR, "napi_get_cb_info fail");
        return nullptr;
    }
    int32_t deviceId = DEFAULT_DEVICE_ID;
    if (!GetLocationDeviceId(deviceId)) {
        SEN_HILOGW("Cant fand local deviceId, default deviceId :%{public}d", deviceId);
    }
    if (!RemoveSubscribeCallback(env, {deviceId, sensorTypeId, DEFAULT_SENSOR_ID, IS_LOCAL_DEVICE}) ||
        CheckSubscribe({deviceId, sensorTypeId, DEFAULT_SENSOR_ID, IS_LOCAL_DEVICE})) {
        SEN_HILOGW("There are other client subscribe as well, not need unsubscribe");
        return nullptr;
    }
    if (UnsubscribeSensor({deviceId, sensorTypeId, DEFAULT_SENSOR_ID, IS_LOCAL_DEVICE}) != OHOS::ERR_OK) {
        SEN_HILOGW("UnsubscribeSensor failed");
        return nullptr;
    }
    return nullptr;
}

napi_value GetBodyState(napi_env env, napi_callback_info info)
{
    CALL_LOG_ENTER;
    size_t argc = 1;
    napi_value args[1] = { 0 };
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr);
    if (status != napi_ok || argc < 1) {
        ThrowErr(env, PARAMETER_ERROR, "napi_get_cb_info fail or number of parameter invalid");
        return nullptr;
    }
    if (!IsMatchType(env, args[0], napi_object)) {
        ThrowErr(env, PARAMETER_ERROR, "Wrong argument type, should be object");
        return nullptr;
    }
    sptr<AsyncCallbackInfo> asyncCallbackInfo = new (std::nothrow) AsyncCallbackInfo(env, GET_BODY_STATE);
    CHKPP(asyncCallbackInfo);
    napi_value napiSuccess = GetNamedProperty(env, args[0], "success");
    CHKCP(IsMatchType(env, napiSuccess, napi_function), "get napiSuccess fail");
    CHKCP(RegisterNapiCallback(env, napiSuccess, asyncCallbackInfo->callback[0]),
        "register success callback fail");
    std::lock_guard<std::mutex> onBodyLock(g_bodyMutex);
    asyncCallbackInfo->data.sensorData.data[0] =
        (fabs(g_bodyState - BODY_STATE_EXCEPT) < THRESHOLD) ? true : false;
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
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_TEMPERATURE", GetNapiInt32(env, SENSOR_TYPE_ID_TEMPERATURE)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_PROXIMITY", GetNapiInt32(env, SENSOR_TYPE_ID_PROXIMITY)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_HUMIDITY", GetNapiInt32(env, SENSOR_TYPE_ID_HUMIDITY)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_COLOR", GetNapiInt32(env, SENSOR_TYPE_ID_COLOR)),
        DECLARE_NAPI_STATIC_PROPERTY("SENSOR_TYPE_ID_SAR", GetNapiInt32(env, SENSOR_TYPE_ID_SAR)),
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

static napi_value CreateEnumSensorId(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY("ACCELEROMETER", GetNapiInt32(env, SENSOR_TYPE_ID_ACCELEROMETER)),
        DECLARE_NAPI_STATIC_PROPERTY("GYROSCOPE", GetNapiInt32(env, SENSOR_TYPE_ID_GYROSCOPE)),
        DECLARE_NAPI_STATIC_PROPERTY("AMBIENT_LIGHT", GetNapiInt32(env, SENSOR_TYPE_ID_AMBIENT_LIGHT)),
        DECLARE_NAPI_STATIC_PROPERTY("MAGNETIC_FIELD", GetNapiInt32(env, SENSOR_TYPE_ID_MAGNETIC_FIELD)),
        DECLARE_NAPI_STATIC_PROPERTY("BAROMETER", GetNapiInt32(env, SENSOR_TYPE_ID_BAROMETER)),
        DECLARE_NAPI_STATIC_PROPERTY("HALL", GetNapiInt32(env, SENSOR_TYPE_ID_HALL)),
        DECLARE_NAPI_STATIC_PROPERTY("TEMPERATURE", GetNapiInt32(env, SENSOR_TYPE_ID_TEMPERATURE)),
        DECLARE_NAPI_STATIC_PROPERTY("PROXIMITY", GetNapiInt32(env, SENSOR_TYPE_ID_PROXIMITY)),
        DECLARE_NAPI_STATIC_PROPERTY("HUMIDITY", GetNapiInt32(env, SENSOR_TYPE_ID_HUMIDITY)),
        DECLARE_NAPI_STATIC_PROPERTY("COLOR", GetNapiInt32(env, SENSOR_TYPE_ID_COLOR)),
        DECLARE_NAPI_STATIC_PROPERTY("SAR", GetNapiInt32(env, SENSOR_TYPE_ID_SAR)),
        DECLARE_NAPI_STATIC_PROPERTY("ORIENTATION", GetNapiInt32(env, SENSOR_TYPE_ID_ORIENTATION)),
        DECLARE_NAPI_STATIC_PROPERTY("GRAVITY", GetNapiInt32(env, SENSOR_TYPE_ID_GRAVITY)),
        DECLARE_NAPI_STATIC_PROPERTY("LINEAR_ACCELEROMETER", GetNapiInt32(env, SENSOR_TYPE_ID_LINEAR_ACCELERATION)),
        DECLARE_NAPI_STATIC_PROPERTY("ROTATION_VECTOR", GetNapiInt32(env, SENSOR_TYPE_ID_ROTATION_VECTOR)),
        DECLARE_NAPI_STATIC_PROPERTY("AMBIENT_TEMPERATURE", GetNapiInt32(env, SENSOR_TYPE_ID_AMBIENT_TEMPERATURE)),
        DECLARE_NAPI_STATIC_PROPERTY("MAGNETIC_FIELD_UNCALIBRATED",
            GetNapiInt32(env, SENSOR_TYPE_ID_MAGNETIC_FIELD_UNCALIBRATED)),
        DECLARE_NAPI_STATIC_PROPERTY("GYROSCOPE_UNCALIBRATED",
            GetNapiInt32(env, SENSOR_TYPE_ID_GYROSCOPE_UNCALIBRATED)),
        DECLARE_NAPI_STATIC_PROPERTY("SIGNIFICANT_MOTION", GetNapiInt32(env, SENSOR_TYPE_ID_SIGNIFICANT_MOTION)),
        DECLARE_NAPI_STATIC_PROPERTY("PEDOMETER_DETECTION", GetNapiInt32(env, SENSOR_TYPE_ID_PEDOMETER_DETECTION)),
        DECLARE_NAPI_STATIC_PROPERTY("PEDOMETER", GetNapiInt32(env, SENSOR_TYPE_ID_PEDOMETER)),
        DECLARE_NAPI_STATIC_PROPERTY("HEART_RATE", GetNapiInt32(env, SENSOR_TYPE_ID_HEART_RATE)),
        DECLARE_NAPI_STATIC_PROPERTY("WEAR_DETECTION", GetNapiInt32(env, SENSOR_TYPE_ID_WEAR_DETECTION)),
        DECLARE_NAPI_STATIC_PROPERTY("ACCELEROMETER_UNCALIBRATED",
            GetNapiInt32(env, SENSOR_TYPE_ID_ACCELEROMETER_UNCALIBRATED)),
    };
    napi_value result = nullptr;
    CHKNRP(env, napi_define_class(env, "SensorId", NAPI_AUTO_LENGTH, EnumClassConstructor, nullptr,
        sizeof(desc) / sizeof(*desc), desc, &result), "napi_define_class");
    CHKNRP(env, napi_set_named_property(env, exports, "SensorId", result), "napi_set_named_property fail");
    return exports;
}

static napi_value CreateEnumSensorAccuracy(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY("ACCURACY_UNRELIABLE", GetNapiInt32(env, ACCURACY_UNRELIABLE)),
        DECLARE_NAPI_STATIC_PROPERTY("ACCURACY_LOW", GetNapiInt32(env, ACCURACY_LOW)),
        DECLARE_NAPI_STATIC_PROPERTY("ACCURACY_MEDIUM", GetNapiInt32(env, ACCURACY_MEDIUM)),
        DECLARE_NAPI_STATIC_PROPERTY("ACCURACY_HIGH", GetNapiInt32(env, ACCURACY_HIGH)),
    };
    napi_value result = nullptr;
    CHKNRP(env, napi_define_class(env, "SensorAccuracy", NAPI_AUTO_LENGTH, EnumClassConstructor, nullptr,
        sizeof(desc) / sizeof(*desc), desc, &result), "napi_define_class");
    CHKNRP(env, napi_set_named_property(env, exports, "SensorAccuracy", result), "napi_set_named_property fail");
    return exports;
}

static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("on", On),
        DECLARE_NAPI_FUNCTION("once", Once),
        DECLARE_NAPI_FUNCTION("off", Off),
        DECLARE_NAPI_FUNCTION("getGeomagneticField", GetGeomagneticField),
        DECLARE_NAPI_FUNCTION("getGeomagneticInfo", GetGeomagneticField),
        DECLARE_NAPI_FUNCTION("transformCoordinateSystem", TransformCoordinateSystem),
        DECLARE_NAPI_FUNCTION("transformRotationMatrix", TransformCoordinateSystem),
        DECLARE_NAPI_FUNCTION("getAngleModify", GetAngleModify),
        DECLARE_NAPI_FUNCTION("getAngleVariation", GetAngleModify),
        DECLARE_NAPI_FUNCTION("getDirection", GetDirection),
        DECLARE_NAPI_FUNCTION("getOrientation", GetDirection),
        DECLARE_NAPI_FUNCTION("createQuaternion", CreateQuaternion),
        DECLARE_NAPI_FUNCTION("getQuaternion", CreateQuaternion),
        DECLARE_NAPI_FUNCTION("getAltitude", GetAltitude),
        DECLARE_NAPI_FUNCTION("getDeviceAltitude", GetAltitude),
        DECLARE_NAPI_FUNCTION("getGeomagneticDip", GetGeomagneticDip),
        DECLARE_NAPI_FUNCTION("getInclination", GetGeomagneticDip),
        DECLARE_NAPI_FUNCTION("createRotationMatrix", CreateRotationMatrix),
        DECLARE_NAPI_FUNCTION("getRotationMatrix", CreateRotationMatrix),
        DECLARE_NAPI_FUNCTION("getSensorList", GetSensorList),
        DECLARE_NAPI_FUNCTION("getSensorListSync", GetSensorListSync),
        DECLARE_NAPI_FUNCTION("getSingleSensor", GetSingleSensor),
        DECLARE_NAPI_FUNCTION("getSingleSensorSync", GetSingleSensorSync),
        DECLARE_NAPI_FUNCTION("getSensorListByDeviceSync", GetSensorListByDeviceSync),
        DECLARE_NAPI_FUNCTION("getSingleSensorByDeviceSync", GetSingleSensorByDeviceSync),
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
    CHKCP(CreateEnumSensorType(env, exports), "Create enum sensor type fail");
    CHKCP(CreateEnumSensorId(env, exports), "Create enum sensor id fail");
    CHKCP(CreateEnumSensorAccuracy(env, exports), "Create enum sensor accuracy fail");
    // 注册env清理钩子函数
    napi_env *pEnv = new (std::nothrow) napi_env;
    if (pEnv == nullptr) {
        SEN_HILOGE("Init, pEnv is nullptr");
        return exports;
    }
    *pEnv = env;
    auto ret = napi_add_env_cleanup_hook(env, CleanUp, reinterpret_cast<void*>(pEnv));
    if (ret != napi_status::napi_ok) {
        SEN_HILOGE("Init, napi_add_env_cleanup_hook failed");
    }

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
} // namespace Sensors
} // namespace OHOS