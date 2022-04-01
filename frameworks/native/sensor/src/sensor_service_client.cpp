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

#include "sensor_service_client.h"

#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "death_recipient_template.h"
#include "dmd_report.h"
#include "ipc_skeleton.h"
#include "sensor_service_proxy.h"
#include "sensors_errors.h"
#include "sensors_log_domain.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SensorsLogDomain::SENSOR_NATIVE, "SensorServiceClient" };
constexpr int32_t GET_SERVICE_MAX_COUNT = 30;
constexpr uint32_t WAIT_MS = 200;
}  // namespace

int32_t SensorServiceClient::InitServiceClient()
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    if (sensorServer_ != nullptr) {
        SEN_HILOGD("already init");
        return ERR_OK;
    }
    if (sensorClientStub_ == nullptr) {
        sensorClientStub_ = new (std::nothrow) SensorClientStub();
    }
    auto systemAbilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHKPR(systemAbilityManager, SENSOR_NATIVE_SAM_ERR);
    int32_t retry = 0;
    while (retry < GET_SERVICE_MAX_COUNT) {
        sensorServer_ = iface_cast<ISensorService>(systemAbilityManager->GetSystemAbility(SENSOR_SERVICE_ABILITY_ID));
        if (sensorServer_ != nullptr) {
            SEN_HILOGD("get service success, retry : %{public}d", retry);
            serviceDeathObserver_ = new (std::nothrow) DeathRecipientTemplate(*const_cast<SensorServiceClient *>(this));
            if (serviceDeathObserver_ != nullptr) {
                sensorServer_->AsObject()->AddDeathRecipient(serviceDeathObserver_);
            }
            sensorList_ = sensorServer_->GetSensorList();
            return ERR_OK;
        }
        SEN_HILOGW("get service failed, retry : %{public}d", retry);
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_MS));
        retry++;
    }
    DmdReport::ReportException(SENSOR_SERVICE_EXCEPTION, "InitServiceClient", SENSOR_NATIVE_GET_SERVICE_ERR);
    SEN_HILOGE("get service failed");
    return SENSOR_NATIVE_GET_SERVICE_ERR;
}

bool SensorServiceClient::IsValidSensorId(uint32_t sensorId)
{
    if (sensorList_.empty()) {
        SEN_HILOGE("sensorList_ cannot be empty");
        return false;
    }
    for (auto &sensor : sensorList_) {
        if (sensor.GetSensorId() == sensorId) {
            return true;
        }
    }
    return false;
}

int32_t SensorServiceClient::EnableSensor(uint32_t sensorId, int64_t samplingPeriod, int64_t maxReportDelay)
{
    CALL_LOG_ENTER;
    if (!IsValidSensorId(sensorId)) {
        SEN_HILOGE("sensorId is invalid");
        return SENSOR_NATIVE_SAM_ERR;
    }
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret : %{public}d", ret);
        return ret;
    }
    ret = sensorServer_->EnableSensor(sensorId, samplingPeriod, maxReportDelay);
    if (ret == ERR_OK) {
        UpdateSensorInfoMap(sensorId, samplingPeriod, maxReportDelay);
    }
    return ret;
}

int32_t SensorServiceClient::DisableSensor(uint32_t sensorId)
{
    CALL_LOG_ENTER;
    if (!IsValidSensorId(sensorId)) {
        SEN_HILOGE("sensorId is invalid");
        return SENSOR_NATIVE_SAM_ERR;
    }
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret : %{public}d", ret);
        return ret;
    }
    ret = sensorServer_->DisableSensor(sensorId);
    if (ret == ERR_OK) {
        DeleteSensorInfoItem(sensorId);
    }
    return ret;
}

int32_t SensorServiceClient::RunCommand(uint32_t sensorId, int32_t cmdType, int32_t params)
{
    CALL_LOG_ENTER;
    if (!IsValidSensorId(sensorId)) {
        SEN_HILOGE("sensorId is invalid");
        return SENSOR_NATIVE_SAM_ERR;
    }
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret : %{public}d", ret);
        return ret;
    }
    ret = sensorServer_->RunCommand(sensorId, cmdType, params);
    if (ret != ERR_OK) {
        SEN_HILOGE("RunCommand failed");
        return ret;
    }
    return ret;
}

std::vector<Sensor> SensorServiceClient::GetSensorList()
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret : %{public}d", ret);
        return {};
    }
    if (sensorList_.empty()) {
        SEN_HILOGE("sensorList_ cannot be empty");
    }
    return sensorList_;
}

int32_t SensorServiceClient::TransferDataChannel(sptr<SensorDataChannel> sensorDataChannel)
{
    CALL_LOG_ENTER;
    dataChannel_ = sensorDataChannel;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret : %{public}d", ret);
        return ret;
    }
    return sensorServer_->TransferDataChannel(sensorDataChannel, sensorClientStub_);
}

int32_t SensorServiceClient::DestroyDataChannel()
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret : %{public}d", ret);
        return ret;
    }
    return sensorServer_->DestroySensorChannel(sensorClientStub_);
}

void SensorServiceClient::ProcessDeathObserver(const wptr<IRemoteObject> &object)
{
    CALL_LOG_ENTER;
    (void)object;
    CHKPL(dataChannel_);
    // STEP1 : Destroy revious data channel
    dataChannel_->DestroySensorDataChannel();

    // STEP2 : Restore data channel
    dataChannel_->RestoreSensorDataChannel();

    // STEP3 : Clear sensorlist and sensorServer_
    sensorList_.clear();
    sensorServer_ = nullptr;

    // STEP4 : ReGet sensors  3601 service
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret : %{public}d", ret);
        dataChannel_->DestroySensorDataChannel();
        return;
    }

    // STEP5 : Retransfer new channel to sensors
    sensorServer_->TransferDataChannel(dataChannel_, sensorClientStub_);

    // STEP6 : Restore Sensor status
    std::lock_guard<std::mutex> mapLock(mapMutex_);
    for (const auto &it : sensorInfoMap_) {
        sensorServer_->EnableSensor(it.first, it.second.GetSamplingPeriodNs(), it.second.GetMaxReportDelayNs());
    }
}

void SensorServiceClient::UpdateSensorInfoMap(uint32_t sensorId, int64_t samplingPeriod, int64_t maxReportDelay)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> mapLock(mapMutex_);
    SensorBasicInfo sensorInfo;
    sensorInfo.SetSamplingPeriodNs(samplingPeriod);
    sensorInfo.SetMaxReportDelayNs(maxReportDelay);
    sensorInfo.SetSensorState(true);
    sensorInfoMap_[sensorId] = sensorInfo;
    return;
}

void SensorServiceClient::DeleteSensorInfoItem(uint32_t sensorId)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> mapLock(mapMutex_);
    auto it = sensorInfoMap_.find(sensorId);
    if (it != sensorInfoMap_.end()) {
        sensorInfoMap_.erase(it);
    }
    return;
}
}  // namespace Sensors
}  // namespace OHOS
