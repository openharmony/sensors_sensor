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
#include "sensor_system_js.h"

#include "hilog/log.h"
#include "sensor_agent_type.h"
namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
static constexpr HiLogLabel LABEL = {LOG_CORE, 0xD002708, "SensorSystemJs"};

static napi_value Subscribe(napi_env env, napi_callback_info info, int32_t sensorTypeId)
{
    return nullptr;
}

static napi_value Unsubscribe(napi_env env, napi_callback_info info, int32_t sensorTypeId)
{
    return nullptr;
}

napi_value SubscribeAccelerometer(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Subscribe(env, info, SENSOR_TYPE_ID_ACCELEROMETER);
}

napi_value UnsubscribeAccelerometer(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Unsubscribe(env, info, SENSOR_TYPE_ID_ACCELEROMETER);
}

napi_value SubscribeCompass(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Subscribe(env, info, SENSOR_TYPE_ID_ORIENTATION);
}

napi_value UnsubscribeCompass(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Unsubscribe(env, info, SENSOR_TYPE_ID_ORIENTATION);
}

napi_value SubscribeProximity(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Subscribe(env, info, SENSOR_TYPE_ID_PROXIMITY);
}

napi_value UnsubscribeProximity(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Unsubscribe(env, info, SENSOR_TYPE_ID_PROXIMITY);
}

napi_value SubscribeLight(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Subscribe(env, info, SENSOR_TYPE_ID_AMBIENT_LIGHT);
}

napi_value UnsubscribeLight(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Unsubscribe(env, info, SENSOR_TYPE_ID_AMBIENT_LIGHT);
}

napi_value SubscribeStepCounter(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Subscribe(env, info, SENSOR_TYPE_ID_PEDOMETER);
}

napi_value UnsubscribeStepCounter(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Unsubscribe(env, info, SENSOR_TYPE_ID_PEDOMETER);
}

napi_value SubscribeBarometer(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Subscribe(env, info, SENSOR_TYPE_ID_BAROMETER);
}

napi_value UnsubscribeBarometer(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Unsubscribe(env, info, SENSOR_TYPE_ID_BAROMETER);
}

napi_value SubscribeHeartRate(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Subscribe(env, info, SENSOR_TYPE_ID_HEART_RATE);
}

napi_value UnsubscribeHeartRate(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Unsubscribe(env, info, SENSOR_TYPE_ID_HEART_RATE);
}

napi_value SubscribeOnBodyState(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Subscribe(env, info, SENSOR_TYPE_ID_WEAR_DETECTION);
}

napi_value UnsubscribeOnBodyState(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Unsubscribe(env, info, SENSOR_TYPE_ID_WEAR_DETECTION);
}

napi_value GetOnBodyState(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return nullptr;
}

napi_value SubscribeDeviceOrientation(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Subscribe(env, info, SENSOR_TYPE_ID_ORIENTATION);
}

napi_value UnsubscribeDeviceOrientation(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Unsubscribe(env, info, SENSOR_TYPE_ID_ORIENTATION);
}

napi_value SubscribeGyroscope(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Subscribe(env, info, SENSOR_TYPE_ID_GYROSCOPE);
}

napi_value UnsubscribeGyroscope(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Unsubscribe(env, info, SENSOR_TYPE_ID_GYROSCOPE);
}

napi_value SubscribeGravity(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Subscribe(env, info, SENSOR_TYPE_ID_GRAVITY);
}

napi_value UnsubscribeGravity(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Unsubscribe(env, info, SENSOR_TYPE_ID_GRAVITY);
}

napi_value SubscribeMagnetic(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Subscribe(env, info, SENSOR_TYPE_ID_MAGNETIC_FIELD);
}

napi_value UnsubscribeMagnetic(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Unsubscribe(env, info, SENSOR_TYPE_ID_MAGNETIC_FIELD);
}

napi_value SubscribeHall(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Subscribe(env, info, SENSOR_TYPE_ID_HALL);
}

napi_value UnsubscribeHall(napi_env env, napi_callback_info info)
{
    HiLog::Debug(LABEL, "%{public}s in", __func__);
    return Unsubscribe(env, info, SENSOR_TYPE_ID_HALL);
}
}  // namespace Sensors
}  // namespace OHOS