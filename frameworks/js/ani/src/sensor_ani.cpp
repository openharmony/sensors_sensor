/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "sensor_ani.h"
#include "securec.h"
#include "ani_utils.h"
#include "sensor_agent.h"
#include "event_handler.h"

#undef LOG_TAG
#define LOG_TAG "SensorAniAPI"

using namespace OHOS;
using namespace OHOS::Sensors;

constexpr int32_t REPORTING_INTERVAL = 200000000;
constexpr int32_t INVALID_SENSOR_ID = -1;
constexpr float BODY_STATE_EXCEPT = 1.0f;
constexpr float THRESHOLD = 0.000001f;
constexpr int32_t ANI_SCOPE_SIZE = 16;

static std::unordered_map<int, std::string> g_sensorTypeToClassName = {
    {256, "LOrientationResponseImpl;"},
};

std::map<int32_t, vector<string>> g_sensorAttributeList = {
    { 0, { "x" } },
    { SENSOR_TYPE_ID_ACCELEROMETER, { "x", "y", "z" } },
    { SENSOR_TYPE_ID_GYROSCOPE, { "x", "y", "z" } },
    { SENSOR_TYPE_ID_AMBIENT_LIGHT, { "intensity", "colorTemperature", "infraredLuminance" } },
    { SENSOR_TYPE_ID_MAGNETIC_FIELD, { "x", "y", "z" } },
    { SENSOR_TYPE_ID_BAROMETER, { "pressure" } },
    { SENSOR_TYPE_ID_HALL, { "status" } },
    { SENSOR_TYPE_ID_TEMPERATURE, { "temperature" } },
    { SENSOR_TYPE_ID_PROXIMITY, { "distance" } },
    { SENSOR_TYPE_ID_HUMIDITY, { "humidity" } },
    { SENSOR_TYPE_ID_ORIENTATION, { "alpha", "beta", "gamma" } },
    { SENSOR_TYPE_ID_GRAVITY, { "x", "y", "z" } },
    { SENSOR_TYPE_ID_LINEAR_ACCELERATION, { "x", "y", "z" } },
    { SENSOR_TYPE_ID_ROTATION_VECTOR, { "x", "y", "z", "w" } },
    { SENSOR_TYPE_ID_AMBIENT_TEMPERATURE, { "temperature" } },
    { SENSOR_TYPE_ID_MAGNETIC_FIELD_UNCALIBRATED, { "x", "y", "z", "biasX", "biasY", "biasZ" } },
    { SENSOR_TYPE_ID_GYROSCOPE_UNCALIBRATED, { "x", "y", "z", "biasX", "biasY", "biasZ" } },
    { SENSOR_TYPE_ID_SIGNIFICANT_MOTION, { "scalar" } },
    { SENSOR_TYPE_ID_PEDOMETER_DETECTION, { "scalar" } },
    { SENSOR_TYPE_ID_PEDOMETER, { "steps" } },
    { SENSOR_TYPE_ID_HEART_RATE, { "heartRate" } },
    { SENSOR_TYPE_ID_WEAR_DETECTION, { "value" } },
    { SENSOR_TYPE_ID_ACCELEROMETER_UNCALIBRATED, { "x", "y", "z", "biasX", "biasY", "biasZ" } },
    { SENSOR_TYPE_ID_COLOR, { "lightIntensity", "colorTemperature" } },
    { SENSOR_TYPE_ID_SAR, { "absorptionRatio" } }
};

std::unordered_map<std::string, int> stringToNumberMap = {
    {"ORIENTATION", 256},
};

static std::map<std::string, int64_t> g_samplingPeriod = {
    {"normal", 200000000},
    {"ui", 60000000},
    {"game", 20000000},
};

using ConvertDataFunc = bool(*)(sptr<AsyncCallbackInfo> asyncCallbackInfo, std::vector<ani_ref> &data);
bool ConvertToSensorData(sptr<AsyncCallbackInfo> asyncCallbackInfo, std::vector<ani_ref> &data);

std::map<int32_t, ConvertDataFunc> g_convertfuncList = {
    {ON_CALLBACK, ConvertToSensorData},
};

static std::mutex mutex_;
static std::mutex bodyMutex_;
static float g_bodyState = -1.0f;
static std::map<int32_t, std::vector<sptr<AsyncCallbackInfo>>> g_subscribeCallbacks;
static std::mutex onMutex_;
static std::map<int32_t, std::vector<sptr<AsyncCallbackInfo>>> g_onCallbackInfos;

static ani_error CreateAniError(ani_env *env, std::string &&errMsg)
{
    static const char *errorClsName = "Lescompat/Error;";
    ani_class cls {};
    if (ANI_OK != env->FindClass(errorClsName, &cls)) {
        SEN_HILOGE("Not found namespace %{public}s.", errorClsName);
        return nullptr;
    }
    ani_method ctor;
    if (ANI_OK != env->Class_FindMethod(cls, "<ctor>", "Lstd/core/String;:V", &ctor)) {
        SEN_HILOGE("Not found <ctor> in %{public}s.", errorClsName);
        return nullptr;
    }
    ani_string error_msg;
    env->String_NewUTF8(errMsg.c_str(), 17U, &error_msg);
    ani_object errorObject;
    env->Object_New(cls, ctor, &errorObject, error_msg);
    return static_cast<ani_error>(errorObject);
}

static bool CheckSubscribe(int32_t sensorTypeId)
{
    std::lock_guard<std::mutex> onCallbackLock(onMutex_);
    auto iter = g_onCallbackInfos.find(sensorTypeId);
    return iter != g_onCallbackInfos.end();
}

static bool CopySensorData(sptr<AsyncCallbackInfo> callbackInfo, SensorEvent *event)
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
        std::lock_guard<std::mutex> onBodyLock(bodyMutex_);
        g_bodyState = *data;
        callbackInfo->data.sensorData.data[0] =
            (fabs(g_bodyState - BODY_STATE_EXCEPT) < THRESHOLD) ? true : false;
        return true;
    }
    if (memcpy_s(callbackInfo->data.sensorData.data, sizeof(callbackInfo->data.sensorData.data),
        data, event->dataLen) != EOK) {
        SEN_HILOGE("Copy data failed");
        return false;
    }
    return true;
}

static bool CheckSystemSubscribe(int32_t sensorTypeId)
{
    std::lock_guard<std::mutex> subscribeLock(mutex_);
    auto iter = g_subscribeCallbacks.find(sensorTypeId);
    if (iter == g_subscribeCallbacks.end()) {
        return false;
    }
    return true;
}

static ani_boolean IsInstanceOf(ani_env *env, const std::string &cls_name, ani_object obj)
{
    ani_class cls;
    if (ANI_OK != env->FindClass(cls_name.c_str(), &cls)) {
        SEN_HILOGE("FindClass failed");
        return ANI_FALSE;
    }

    ani_boolean ret;
    env->Object_InstanceOf(obj, cls, &ret);
    return ret;
}

static bool SendEventToMainThread(const std::function<void()> func)
{
    if (func == nullptr) {
        SEN_HILOGE("func == nullptr");
        return false;
    }
    auto runner = OHOS::AppExecFwk::EventRunner::GetMainEventRunner();
    if (!runner) {
        SEN_HILOGE("runner == nullptr");
        return false;
    }
    auto handler = std::make_shared<OHOS::AppExecFwk::EventHandler>(runner);
    handler->PostTask(func, "", 0, OHOS::AppExecFwk::EventQueue::Priority::HIGH, {});
    SEN_HILOGD("PostTask success");
    return true;
}

bool ValidateAndInitialize(sptr<AsyncCallbackInfo> asyncCallbackInfo, ani_object &obj)
{
    CHKPF(asyncCallbackInfo);
    int32_t sensorTypeId = asyncCallbackInfo->data.sensorData.sensorTypeId;
    if (g_sensorAttributeList.find(sensorTypeId) == g_sensorAttributeList.end()) {
        SEN_HILOGE("Invalid sensor type");
        return false;
    }
    if (sensorTypeId == SENSOR_TYPE_ID_WEAR_DETECTION && asyncCallbackInfo->type == SUBSCRIBE_CALLBACK) {
        return false;
    }
    size_t size = g_sensorAttributeList[sensorTypeId].size();
    uint32_t dataLength = asyncCallbackInfo->data.sensorData.dataLength / sizeof(float);
    if (size > dataLength) {
        SEN_HILOGE("Data length mismatch");
        return false;
    }

    if (g_sensorTypeToClassName.find(sensorTypeId) == g_sensorTypeToClassName.end()) {
        SEN_HILOGE("Find class by sensorType failed");
        return false;
    }

    ani_namespace ns;
    static const char *namespaceName = "L@ohos/sensor/sensor;";
    if (ANI_OK != asyncCallbackInfo->env->FindNamespace(namespaceName, &ns)) {
        SEN_HILOGE("Not found '%{public}s'", namespaceName);
        return false;
    }

    ani_class cls;
    const char *className = g_sensorTypeToClassName[sensorTypeId].c_str();
    if (ANI_OK != asyncCallbackInfo->env->Namespace_FindClass(ns, className, &cls)) {
        SEN_HILOGE("FindClass %{public}s failed", className);
        return false;
    }

    ani_method ctor;
    if (ANI_OK != asyncCallbackInfo->env->Class_FindMethod(cls, "<ctor>", nullptr, &ctor)) {
        SEN_HILOGE("Class_FindMethod 'constructor' failed");
        return false;
    }

    if (ANI_OK != asyncCallbackInfo->env->Object_New(cls, ctor, &obj)) {
        SEN_HILOGE("Object_New '%{public}s' failed", className);
        return false;
    }
    return true;
}

ani_enum_item GetEnumItem(ani_env *env, int32_t accuracy)
{
    ani_namespace ns;
    static const char *namespaceName = "L@ohos/sensor/sensor;";
    if (ANI_OK != env->FindNamespace(namespaceName, &ns)) {
        SEN_HILOGE("Not found '%{public}s'", namespaceName);
        return nullptr;
    }

    ani_enum aniEnum{};
    const char *enumName = "LSensorAccuracy;";
    if (ANI_OK != env->Namespace_FindEnum(ns, enumName, &aniEnum)) {
        SEN_HILOGE("Not found '%{public}s'", enumName);
        return nullptr;
    }

    constexpr int32_t loopMaxNum = 1000;
    for (int32_t index = 0U; index < loopMaxNum; index++) {
        ani_enum_item enumItem{};
        if (ANI_OK != env->Enum_GetEnumItemByIndex(aniEnum, index, &enumItem)) {
            SEN_HILOGE("Enum_GetIntemByIndex failed");
            return nullptr;
        }
        ani_int intValue = -1;
        if (ANI_OK != env->EnumItem_GetValue_Int(enumItem, &intValue)) {
            SEN_HILOGE("EnumItem_GetValue_Int FAILD.");
            return nullptr;
        }
        if (intValue == accuracy) {
            return enumItem;
        }
    }
    SEN_HILOGE("Get enumItem by %{public}d failed.", accuracy);
    return nullptr;
}

bool SetSensorPropertiesAndPushData(sptr<AsyncCallbackInfo> asyncCallbackInfo, ani_object obj,
    std::vector<ani_ref> &data)
{
    CALL_LOG_ENTER;
    int32_t sensorTypeId = asyncCallbackInfo->data.sensorData.sensorTypeId;
    size_t size = g_sensorAttributeList[sensorTypeId].size();
    auto sensorAttributes = g_sensorAttributeList[sensorTypeId];
    for (uint32_t i = 0; i < size; ++i) {
        if (ANI_OK != asyncCallbackInfo->env->Object_SetPropertyByName_Double(obj, sensorAttributes[i].c_str(),
            asyncCallbackInfo->data.sensorData.data[i])) {
            SEN_HILOGE("Object_SetPropertyByName_Double failed");
            return false;
        }
    }

    if (ANI_OK != asyncCallbackInfo->env->Object_SetPropertyByName_Double(obj, "timestamp",
        asyncCallbackInfo->data.sensorData.timestamp)) {
        SEN_HILOGE("Object_SetPropertyByName_Double timestamp failed");
        return false;
    }

    ani_enum_item accuracy = GetEnumItem(asyncCallbackInfo->env, asyncCallbackInfo->data.sensorData.sensorAccuracy);
    if (accuracy == nullptr) {
        SEN_HILOGE("GetEnumItem failed");
        return false;
    }
    if (ANI_OK != asyncCallbackInfo->env->Object_SetPropertyByName_Ref(obj, "accuracy", accuracy)) {
        SEN_HILOGE("Object_SetPropertyByName_Ref accuracy failed");
        return false;
    }

    data.push_back(obj);
    return true;
}

bool ConvertToSensorData(sptr<AsyncCallbackInfo> asyncCallbackInfo, std::vector<ani_ref> &data)
{
    ani_object obj;
    if (!ValidateAndInitialize(asyncCallbackInfo, obj)) {
        return false;
    }

    if (!SetSensorPropertiesAndPushData(asyncCallbackInfo, obj, data)) {
        return false;
    }

    return true;
}

static void EmitUvEventLoop(sptr<AsyncCallbackInfo> asyncCallbackInfo)
{
    CHKPV(asyncCallbackInfo);
    auto task = [asyncCallbackInfo]() {
        SEN_HILOGD("Begin to call task");
        AniLocalScopeGuard aniLocalScopeGuard(asyncCallbackInfo->env, ANI_SCOPE_SIZE);
        if (!aniLocalScopeGuard.IsStatusOK()) {
            SEN_HILOGE("CreateLocalScope failed");
            return;
        }

        ani_error error;
        if (!(g_convertfuncList.find(asyncCallbackInfo->type) != g_convertfuncList.end())) {
            SEN_HILOGE("asyncCallbackInfo type is invalid");
            error = CreateAniError(asyncCallbackInfo->env, "asyncCallbackInfo type is invalid");
            asyncCallbackInfo->env->ThrowError(error);
            return;
        }
        std::vector<ani_ref> args;
        g_convertfuncList[asyncCallbackInfo->type](asyncCallbackInfo, args);

        auto fnObj = reinterpret_cast<ani_fn_object>(asyncCallbackInfo->callback[0]);
        SEN_HILOGD("Begin to call FunctionalObject_Call");
        if (fnObj == nullptr) {
            SEN_HILOGE("fnObj == nullptr");
            error = CreateAniError(asyncCallbackInfo->env, "fnObj == nullptr");
            asyncCallbackInfo->env->ThrowError(error);
            return;
        }
        if (IsInstanceOf(asyncCallbackInfo->env, "Lstd/core/Function1;", fnObj) == 0) {
            SEN_HILOGE("fnObj is not instance Of function");
            error = CreateAniError(asyncCallbackInfo->env, "fnObj is not instance Of function");
            asyncCallbackInfo->env->ThrowError(error);
            return;
        }

        ani_ref result;
        if (ANI_OK != asyncCallbackInfo->env->FunctionalObject_Call(fnObj, 1, args.data(), &result)) {
            SEN_HILOGE("FunctionalObject_Call failed");
            error = CreateAniError(asyncCallbackInfo->env, "FunctionalObject_Call failed");
            asyncCallbackInfo->env->ThrowError(error);
            return;
        }
        SEN_HILOGD("FunctionalObject_Call success");
    };
    if (!SendEventToMainThread(task)) {
        SEN_HILOGE("failed to send event");
    }
}
static void EmitOnCallback(SensorEvent *event)
{
    CHKPV(event);
    int32_t sensorTypeId = event->sensorTypeId;
    if (!CheckSubscribe(sensorTypeId)) {
        return;
    }
    std::lock_guard<std::mutex> onCallbackLock(onMutex_);
    auto onCallbackInfos = g_onCallbackInfos[sensorTypeId];
    for (auto &onCallbackInfo : onCallbackInfos) {
        if (!CopySensorData(onCallbackInfo, event)) {
            SEN_HILOGE("Copy sensor data failed");
            continue;
        }
        EmitUvEventLoop(onCallbackInfo);
    }
}

void DataCallbackImpl(SensorEvent *event)
{
    CHKPV(event);
    EmitOnCallback(event);
}

const SensorUser user = {
    .callback = DataCallbackImpl
};

int32_t SubscribeSensor(int32_t sensorTypeId, int64_t interval, RecordSensorCallback callback)
{
    CALL_LOG_ENTER;
    int32_t ret = SubscribeSensor(sensorTypeId, &user);
    if (ret != ERR_OK) {
        SEN_HILOGE("SubscribeSensor failed");
        return ret;
    }
    ret = SetBatch(sensorTypeId, &user, interval, 0);
    if (ret != ERR_OK) {
        SEN_HILOGE("SetBatch failed");
        return ret;
    }
    return ActivateSensor(sensorTypeId, &user);
}

static bool IsSubscribed(ani_env *env, int32_t sensorTypeId, ani_object callback)
{
    CALL_LOG_ENTER;
    if (auto iter = g_onCallbackInfos.find(sensorTypeId); iter == g_onCallbackInfos.end()) {
        SEN_HILOGW("No client subscribe, sensorTypeId:%{public}d", sensorTypeId);
        return false;
    }
    std::vector<sptr<AsyncCallbackInfo>> callbackInfos = g_onCallbackInfos[sensorTypeId];
    for (auto callbackInfo : callbackInfos) {
        CHKPC(callbackInfo);
        if (callbackInfo->env != env) {
            continue;
        }

        ani_boolean isEquals = false;
        if (ANI_OK != env->Reference_StrictEquals(callback, callbackInfo->callback[0], &isEquals)) {
            SEN_HILOGE("StrictEquals failed");
            return false;
        }
        if (isEquals) {
            return true;
        }
    }
    return false;
}
static void UpdateCallbackInfos(ani_env *env, int32_t sensorTypeId, ani_object callback)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> onCallbackLock(onMutex_);
    CHKCV((!IsSubscribed(env, sensorTypeId, callback)), "The callback has been subscribed");
    sptr<AsyncCallbackInfo> asyncCallbackInfo = new (std::nothrow) AsyncCallbackInfo(env, ON_CALLBACK);
    CHKPV(asyncCallbackInfo);

    if (ANI_OK != env->GlobalReference_Create(callback, &asyncCallbackInfo->callback[0])) {
        SEN_HILOGE("GlobalReference_Create failed");
        return;
    }

    std::vector<sptr<AsyncCallbackInfo>> callbackInfos = g_onCallbackInfos[sensorTypeId];
    callbackInfos.push_back(asyncCallbackInfo);
    g_onCallbackInfos[sensorTypeId] = callbackInfos;
}

bool GetIntervalValue(ani_env *env, ani_object options, int64_t& interval)
{
    ani_boolean isUndefined;
    env->Reference_IsUndefined(options, &isUndefined);
    if (isUndefined) {
        return true;
    }

    ani_ref intervalRef;
    ani_boolean isIntervalUndefined;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(options, "interval", &intervalRef)) {
        SEN_HILOGE("Failed to get property named interval");
        return false;
    }

    env->Reference_IsUndefined(intervalRef, &isIntervalUndefined);
    if (isIntervalUndefined) {
        SEN_HILOGE("interval is undefined");
        return false;
    }

    ani_class stringClass;
    env->FindClass("Lstd/core/String;", &stringClass);
    ani_class doubleClass;
    env->FindClass("Lstd/core/Double;", &doubleClass);

    ani_boolean isDouble;
    env->Object_InstanceOf(static_cast<ani_object>(intervalRef), doubleClass, &isDouble);
    ani_boolean isString;
    env->Object_InstanceOf(static_cast<ani_object>(intervalRef), stringClass, &isString);
    if (isDouble) {
        ani_double doubleValue;
        auto ret = env->Object_CallMethodByName_Double(static_cast<ani_object>(intervalRef), "unboxed",
            nullptr, &doubleValue);
        if (ret != ANI_OK) {
            SEN_HILOGE("Failed to get property named doubleValue");
            return false;
        }
        interval = static_cast<int64_t>(doubleValue);
        return true;
    }

    if (isString) {
        auto mode = AniStringUtils::ToStd(env, static_cast<ani_string>(intervalRef));
        auto iter = g_samplingPeriod.find(mode);
        if (iter == g_samplingPeriod.end()) {
            SEN_HILOGE("Find interval mode failed");
            return false;
        }
        interval = iter->second;
        SEN_HILOGI("GetIntervalValue mode: %{public}s", mode.c_str());
        return true;
    }

    SEN_HILOGE("Invalid interval type");
    return false;
}

static void On([[maybe_unused]] ani_env *env, ani_string typeId, ani_object callback, ani_object options)
{
    CALL_LOG_ENTER;
    if (!IsInstanceOf(env, "Lstd/core/Function1;", callback)) {
        SEN_HILOGE("Wrong argument type");
        return;
    }

    int32_t sensorTypeId = INVALID_SENSOR_ID;
    auto typeIdStr = AniStringUtils::ToStd(env, static_cast<ani_string>(typeId));
    if (stringToNumberMap.find(typeIdStr) == stringToNumberMap.end()) {
        SEN_HILOGE("Invalid sensor typeId: %{public}s", typeIdStr.c_str());
        return;
    }
    sensorTypeId = stringToNumberMap[typeIdStr];

    int64_t interval = REPORTING_INTERVAL;
    if (!GetIntervalValue(env, options, interval)) {
        SEN_HILOGW("Get interval failed");
    }
    int32_t ret = SubscribeSensor(sensorTypeId, interval, DataCallbackImpl);
    if (ret != ERR_OK) {
        auto error = CreateAniError(env, "SubscribeSensor fail");
        env->ThrowError(error);
        return;
    }
    UpdateCallbackInfos(env, sensorTypeId, callback);
}

static int32_t RemoveAllCallback(ani_env *env, int32_t sensorTypeId)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> onCallbackLock(onMutex_);
    std::vector<sptr<AsyncCallbackInfo>> callbackInfos = g_onCallbackInfos[sensorTypeId];
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
        g_onCallbackInfos.erase(sensorTypeId);
        return 0;
    }
    g_onCallbackInfos[sensorTypeId] = callbackInfos;
    return callbackInfos.size();
}

static int32_t RemoveCallback(ani_env *env, int32_t sensorTypeId, ani_object callback)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> onCallbackLock(onMutex_);
    std::vector<sptr<AsyncCallbackInfo>> callbackInfos = g_onCallbackInfos[sensorTypeId];
    for (auto iter = callbackInfos.begin(); iter != callbackInfos.end();) {
        CHKPC(*iter);
        if ((*iter)->env != env) {
            continue;
        }

        ani_boolean isEquals = false;
        if (ANI_OK != env->Reference_StrictEquals(callback, (*iter)->callback[0], &isEquals)) {
            SEN_HILOGE("Reference_StrictEquals failed");
            return false;
        }
        if (isEquals) {
            iter = callbackInfos.erase(iter);
            SEN_HILOGD("Remove callback success");
            break;
        } else {
            ++iter;
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

int32_t UnsubscribeSensor(int32_t sensorTypeId)
{
    int32_t ret = DeactivateSensor(sensorTypeId, &user);
    if (ret != ERR_OK) {
        SEN_HILOGE("DeactivateSensor failed");
        return ret;
    }
    return UnsubscribeSensor(sensorTypeId, &user);
}

static void Off([[maybe_unused]] ani_env *env, ani_string type, ani_object callback)
{
    CALL_LOG_ENTER;
    int32_t sensorTypeId = INVALID_SENSOR_ID;
    auto typeStr = AniStringUtils::ToStd(env, static_cast<ani_string>(type));
    ani_error error;
    if (stringToNumberMap.find(typeStr) == stringToNumberMap.end()) {
        SEN_HILOGE("Invalid sensor type: %{public}s", typeStr.c_str());
        error = CreateAniError(env, "Invalid sensor type");
        env->ThrowError(error);
        return;
    }
    sensorTypeId = stringToNumberMap[typeStr];

    int32_t subscribeSize = -1;
    ani_boolean isUndefined;
    env->Reference_IsUndefined(callback, &isUndefined);
    if (isUndefined) {
        subscribeSize = RemoveAllCallback(env, sensorTypeId);
    } else {
        ani_boolean result;
        if (env->Reference_IsNull(callback, &result) == ANI_OK && result) {
            subscribeSize = RemoveAllCallback(env, sensorTypeId);
        } else if (IsInstanceOf(env, "Lstd/core/Function1;", callback)) {
            subscribeSize = RemoveCallback(env, sensorTypeId, callback);
        } else {
            error = CreateAniError(env, "Invalid sensor type");
            env->ThrowError(error);
            return;
        }
    }

    if (CheckSystemSubscribe(sensorTypeId) || (subscribeSize > 0)) {
        SEN_HILOGW("There are other client subscribe system js api as well, not need unsubscribe");
        return;
    }
    int32_t ret = UnsubscribeSensor(sensorTypeId);
    if (ret == PARAMETER_ERROR || ret == PERMISSION_DENIED) {
        error = CreateAniError(env, "UnsubscribeSensor fail");
        env->ThrowError(error);
    }
    return;
}

ANI_EXPORT ani_status ANI_Constructor(ani_vm *vm, uint32_t *result)
{
    ani_env *env;
    if (ANI_OK != vm->GetEnv(ANI_VERSION_1, &env)) {
        SEN_HILOGE("Unsupported ANI_VERSION_1");
        return ANI_ERROR;
    }

    ani_namespace ns;
    static const char *spaceName = "L@ohos/sensor/sensor;";
    if (ANI_OK != env->FindNamespace(spaceName, &ns)) {
        SEN_HILOGE("Not found space name");
        return ANI_ERROR;
    }

    std::array methods = {
        ani_native_function {"on", nullptr, reinterpret_cast<void *>(On)},
        ani_native_function {"off", nullptr, reinterpret_cast<void *>(Off)},
    };

    if (ANI_OK != env->Namespace_BindNativeFunctions(ns, methods.data(), methods.size())) {
        SEN_HILOGE("Cannot bind native methods to %{public}s", spaceName);
        return ANI_ERROR;
    }

    *result = ANI_VERSION_1;
    return ANI_OK;
}