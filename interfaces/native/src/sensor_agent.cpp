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
using OHOS::Sensors::SensorAgentProxy;
using OHOS::Sensors::SERVICE_EXCEPTION;
using OHOS::Sensors::PARAMETER_ERROR;
using OHOS::Sensors::PERMISSION_DENIED;

static const HiLogLabel LABEL = {LOG_CORE, OHOS::Sensors::SENSOR_LOG_DOMAIN, "SensorNativeAPI"};

static const SensorAgentProxy *GetInstance()
{
    const SensorAgentProxy *obj = SensorAgentProxy::GetSensorsObj();
    return obj;
}

static int32_t NormalizeErrCode(int32_t code)
{
    switch (code) {
        case PERMISSION_DENIED: {
            return PERMISSION_DENIED;
        }
        case PARAMETER_ERROR: {
            return PARAMETER_ERROR;
        }
        default: {
            return SERVICE_EXCEPTION;
        }
    }
}

int32_t GetAllSensors(SensorInfo **sensorInfo, int32_t *count)
{
    const SensorAgentProxy *proxy = GetInstance();
    if (proxy == nullptr) {
        SEN_HILOGE("proxy is nullptr");
        return SERVICE_EXCEPTION;
    }
    int32_t ret = proxy->GetAllSensors(sensorInfo, count);
    if (ret != OHOS::ERR_OK) {
        SEN_HILOGE("GetAllSensors failed");
        return NormalizeErrCode(ret);
    }
    return ret;
}

int32_t ActivateSensor(int32_t sensorId, const SensorUser *user)
{
    const SensorAgentProxy *proxy = GetInstance();
    if (proxy == nullptr) {
        SEN_HILOGE("proxy is nullptr");
        return SERVICE_EXCEPTION;
    }
    int32_t ret = proxy->ActivateSensor(sensorId, user);
    if (ret != OHOS::ERR_OK) {
        SEN_HILOGE("ActivateSensor failed");
        return NormalizeErrCode(ret);
    }
    return ret;
}

int32_t DeactivateSensor(int32_t sensorId, const SensorUser *user)
{
    const SensorAgentProxy *proxy = GetInstance();
    if (proxy == nullptr) {
        SEN_HILOGE("proxy is nullptr");
        return SERVICE_EXCEPTION;
    }
    int32_t ret = proxy->DeactivateSensor(sensorId, user);
    if (ret != OHOS::ERR_OK) {
        SEN_HILOGE("DeactivateSensor failed");
        return NormalizeErrCode(ret);
    }
    return ret;
}

int32_t SetBatch(int32_t sensorId, const SensorUser *user, int64_t samplingInterval, int64_t reportInterval)
{
    const SensorAgentProxy *proxy = GetInstance();
    if (proxy == nullptr) {
        SEN_HILOGE("proxy is nullptr");
        return SERVICE_EXCEPTION;
    }
    int32_t ret = proxy->SetBatch(sensorId, user, samplingInterval, reportInterval);
    if (ret != OHOS::ERR_OK) {
        SEN_HILOGE("SetBatch failed");
        return NormalizeErrCode(ret);
    }
    return ret;
}

int32_t SubscribeSensor(int32_t sensorId, const SensorUser *user)
{
    const SensorAgentProxy *proxy = GetInstance();
    if (proxy == nullptr) {
        SEN_HILOGE("proxy is nullptr");
        return SERVICE_EXCEPTION;
    }
    int32_t ret = proxy->SubscribeSensor(sensorId, user);
    if (ret != OHOS::ERR_OK) {
        SEN_HILOGE("SubscribeSensor failed");
        return NormalizeErrCode(ret);
    }
    return ret;
}

int32_t UnsubscribeSensor(int32_t sensorId, const SensorUser *user)
{
    const SensorAgentProxy *proxy = GetInstance();
    if (proxy == nullptr) {
        SEN_HILOGE("proxy is nullptr");
        return SERVICE_EXCEPTION;
    }
    int32_t ret = proxy->UnsubscribeSensor(sensorId, user);
    if (ret != OHOS::ERR_OK) {
        SEN_HILOGE("UnsubscribeSensor failed");
        return NormalizeErrCode(ret);
    }
    return ret;
}

int32_t SetMode(int32_t sensorId, const SensorUser *user, int32_t mode)
{
    const SensorAgentProxy *proxy = GetInstance();
    if (proxy == nullptr) {
        SEN_HILOGE("proxy is nullptr");
        return SERVICE_EXCEPTION;
    }
    return proxy->SetMode(sensorId, user, mode);
}
