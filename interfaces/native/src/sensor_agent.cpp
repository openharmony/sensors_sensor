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

#include "sensor_agent.h"

#include "sensor_agent_proxy.h"
#include "sensors_errors.h"

using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

static const HiLogLabel LABEL = {LOG_CORE, OHOS::Sensors::SENSOR_LOG_DOMAIN, "SensorNativeAPI"};

static const OHOS::Sensors::SensorAgentProxy *GetInstance()
{
    const OHOS::Sensors::SensorAgentProxy *obj = OHOS::Sensors::SensorAgentProxy::GetSensorsObj();
    return obj;
}

int32_t GetAllSensors(SensorInfo **sensorInfo, int32_t *count)
{
    const OHOS::Sensors::SensorAgentProxy *proxy = GetInstance();
    if (proxy == nullptr) {
        SEN_HILOGE("proxy is nullptr");
        return OHOS::Sensors::ERROR;
    }
    return proxy->GetAllSensors(sensorInfo, count);
}

int32_t ActivateSensor(int32_t sensorId, const SensorUser *user)
{
    const OHOS::Sensors::SensorAgentProxy *proxy = GetInstance();
    if (proxy == nullptr) {
        SEN_HILOGE("proxy is nullptr");
        return OHOS::Sensors::ERROR;
    }
    return proxy->ActivateSensor(sensorId, user);
}

int32_t DeactivateSensor(int32_t sensorId, const SensorUser *user)
{
    const OHOS::Sensors::SensorAgentProxy *proxy = GetInstance();
    if (proxy == nullptr) {
        SEN_HILOGE("proxy is nullptr");
        return OHOS::Sensors::ERROR;
    }
    return proxy->DeactivateSensor(sensorId, user);
}

int32_t SetBatch(int32_t sensorId, const SensorUser *user, int64_t samplingInterval, int64_t reportInterval)
{
    const OHOS::Sensors::SensorAgentProxy *proxy = GetInstance();
    if (proxy == nullptr) {
        SEN_HILOGE("proxy is nullptr");
        return OHOS::Sensors::ERROR;
    }
    return proxy->SetBatch(sensorId, user, samplingInterval, reportInterval);
}

int32_t SubscribeSensor(int32_t sensorId, const SensorUser *user)
{
    const OHOS::Sensors::SensorAgentProxy *proxy = GetInstance();
    if (proxy == nullptr) {
        SEN_HILOGE("proxy is nullptr");
        return OHOS::Sensors::ERROR;
    }
    return proxy->SubscribeSensor(sensorId, user);
}

int32_t UnsubscribeSensor(int32_t sensorId, const SensorUser *user)
{
    const OHOS::Sensors::SensorAgentProxy *proxy = GetInstance();
    if (proxy == nullptr) {
        SEN_HILOGE("proxy is nullptr");
        return OHOS::Sensors::ERROR;
    }
    return proxy->UnsubscribeSensor(sensorId, user);
}

int32_t SetMode(int32_t sensorId, const SensorUser *user, int32_t mode)
{
    const OHOS::Sensors::SensorAgentProxy *proxy = GetInstance();
    if (proxy == nullptr) {
        SEN_HILOGE("proxy is nullptr");
        return OHOS::Sensors::ERROR;
    }
    return proxy->SetMode(sensorId, user, mode);
}

int32_t SetOption(int32_t sensorId, const SensorUser *user, int32_t option)
{
    const OHOS::Sensors::SensorAgentProxy *proxy = GetInstance();
    if (proxy == nullptr) {
        SEN_HILOGE("proxy is nullptr");
        return OHOS::Sensors::ERROR;
    }
    return proxy->SetOption(sensorId, user, option);
}