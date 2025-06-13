/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sensor_service_load.h"

#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "sensor_errors.h"
#include "sensor_log.h"
#include "system_ability_definition.h"

#undef LOG_TAG
#define LOG_TAG "SensorServiceLoad"

namespace OHOS {
namespace Sensors {

#define SEN_SERVICE_LOAD SensorServiceLoad::GetInstance()

SensorServiceLoad::~SensorServiceLoad() {}

int32_t SensorServiceLoad::LoadSensorService(void)
{
    if (isSensorServiceLoading_) {
        SEN_HILOGI("Sensor service is loading.");
        return ERR_OK;
    }
    SEN_HILOGI("start");
    isSensorServiceLoading_ = true;
    sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        isSensorServiceLoading_ = false;
        SEN_HILOGE("Failed to get system ability mgr");
        return SENSOR_NATIVE_SAM_ERR;
    }
    sptr<SensorLoadCallback> sensorLoadCallback(new SensorLoadCallback());
    int32_t ret = samgr->LoadSystemAbility(SENSOR_SERVICE_ABILITY_ID, sensorLoadCallback);
    if (ret != ERR_OK) {
        isSensorServiceLoading_ = false;
        SEN_HILOGE("Failed to load sensor service, ret code:%{public}d", ret);
        return ret;
    }
    return ERR_OK;
}

void SensorServiceLoad::SetLoadFinish(void)
{
    isSensorServiceLoading_ = false;
}

void SensorLoadCallback::OnLoadSystemAbilitySuccess(int32_t systemAbilityId,
    const sptr<IRemoteObject> &remoteObject)
{
    SEN_HILOGI("Load sensor service success remoteObject result:%{public}s",
        (remoteObject != nullptr) ? "true" : "false");
    SEN_SERVICE_LOAD.SetLoadFinish();
    if (remoteObject == nullptr) {
        SEN_HILOGE("remoteObject is nullptr");
    }
}

void SensorLoadCallback::OnLoadSystemAbilityFail(int32_t systemAbilityId)
{
    SEN_HILOGE("Load sensor service failed.");
    SEN_SERVICE_LOAD.SetLoadFinish();
}
} // namespace Sensors
} // namespace OHOS
