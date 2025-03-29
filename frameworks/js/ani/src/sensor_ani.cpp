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

#include <ani.h>
#include <iostream>
#include <unordered_map>
#include <string>
#include <cctype>
#include <map>
#include <unistd.h>
#include <vector>

#include "refbase.h"
#include "securec.h"

#include "ani_utils.h"
#include "sensor_agent.h"
#include "sensor_errors.h"
#include "sensor_agent_type.h"
#include "sensor_ani.h"

#undef LOG_TAG
#define LOG_TAG "SensorAniAPI"

using namespace OHOS;
using namespace OHOS::Sensors;

constexpr int32_t REPORTING_INTERVAL = 200000000;
constexpr int32_t INVALID_SENSOR_ID = -1;
static std::mutex onMutex_;

std::unordered_map<std::string, int> stringToNumberMap = {
    {"ORIENTATION", 256},
    {"MAGNETIC_FIELD_UNCALIBRATED", 261}
};

static std::map<std::string, int64_t> g_samplingPeriod = {
    {"normal", 200000000},
    {"ui", 60000000},
    {"game", 20000000},
};

static void EmitOnCallback(SensorEvent *event)
{
    return;
}

void DataCallbackImpl(SensorEvent *event)
{
    EmitOnCallback(event);
}

const SensorUser user = {
    .callback = DataCallbackImpl
};

int32_t SubscribeSensor(int32_t sensorTypeId, int64_t interval, RecordSensorCallback callback)
{
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

static void UpdateCallbackInfos(ani_env *env, int32_t sensorTypeId, ani_object callback)
{
    return;
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
        std::cerr << "interval is undefined" << std::endl;
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
        return true;
    }

    SEN_HILOGE("Invalid interval type");
    return false;
}

int ParseStringToNumber(const std::string& str)
{
    std::string upperStr;
    for (char c : str) {
        upperStr += std::toupper(c);
    }

    if (stringToNumberMap.find(upperStr) != stringToNumberMap.end()) {
        return stringToNumberMap[upperStr];
    } else {
        return -1;
    }
}

static void On([[maybe_unused]] ani_env *env, ani_string typeId, ani_object callback, ani_object options)
{
    SEN_HILOGD("sensor on start");

    int32_t sensorTypeId = INVALID_SENSOR_ID;
    auto typeIdStr = AniStringUtils::ToStd(env, static_cast<ani_string>(typeId));
    auto typeIdVal = ParseStringToNumber(typeIdStr);
    if (typeIdVal == -1) {
        SEN_HILOGE("Invalid sensor typeId: %{public}s", typeIdStr.c_str());
        return;
    }
    std::cout << "typeIdVal: " << typeIdVal << std::endl;
    sensorTypeId = typeIdVal;

    int64_t interval = REPORTING_INTERVAL;
    if (!GetIntervalValue(env, options, interval)) {
        SEN_HILOGW("Get interval failed");
    }
    int32_t ret = SubscribeSensor(sensorTypeId, interval, DataCallbackImpl);
    if (ret != ERR_OK) {
        SEN_HILOGE("SubscribeSensor fail");
        return;
    }
    UpdateCallbackInfos(env, sensorTypeId, callback);
    SEN_HILOGD("sensor on end");
}

static int32_t RemoveAllCallback(ani_env *env, int32_t sensorTypeId)
{
    return 0;
}

static int32_t RemoveCallback(ani_env *env, int32_t sensorTypeId, ani_object callback)
{
    return 0;
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
    SEN_HILOGD("sensor off start");

    int32_t sensorTypeId = INVALID_SENSOR_ID;
    auto typeStr = AniStringUtils::ToStd(env, static_cast<ani_string>(type));
    auto typeStrVal = ParseStringToNumber(typeStr);
    if (typeStrVal == -1) {
        SEN_HILOGE("Invalid sensor type: %{public}s", typeStr.c_str());
        return;
    }
    sensorTypeId = typeStrVal;

    int32_t subscribeSize = -1;
    ani_boolean isUndefined;
    env->Reference_IsUndefined(callback, &isUndefined);
    if (isUndefined) {
        subscribeSize = RemoveAllCallback(env, sensorTypeId);
    } else {
        ani_boolean result;
        if (env->Reference_IsNull(callback, &result) == ANI_OK && result) {
            subscribeSize = RemoveAllCallback(env, sensorTypeId);
        } else {
            subscribeSize = RemoveCallback(env, sensorTypeId, callback);
        }
    }

    int32_t ret = UnsubscribeSensor(sensorTypeId);
    if (ret == PARAMETER_ERROR || ret == PERMISSION_DENIED) {
        SEN_HILOGE("UnsubscribeSensor fail");
    }
    SEN_HILOGD("sensor off end");
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