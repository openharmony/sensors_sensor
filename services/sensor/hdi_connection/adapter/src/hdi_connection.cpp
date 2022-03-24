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
#include "hdi_connection.h"

#include <map>
#include <mutex>
#include <thread>

#include "sensor_event_callback.h"
#include "sensors_errors.h"
#include "sensors_log_domain.h"
#include "v1_0/sensor_interface_proxy.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
using OHOS::HDI::Sensor::V1_0::ISensorInterface;
using OHOS::HDI::Sensor::V1_0::ISensorCallback;
using OHOS::HDI::Sensor::V1_0::HdfSensorInformation;
namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SensorsLogDomain::SENSOR_SERVICE, "HdiConnection" };
sptr<ISensorInterface> sensorInterface_ = nullptr;
sptr<ISensorCallback> eventCallback_ = nullptr;
std::map<int32_t, SensorBasicInfo> sensorBasicInfoMap_;
std::mutex sensorBasicInfoMutex_;
constexpr int32_t GET_HDI_SERVICE_COUNT = 30;
constexpr uint32_t WAIT_MS = 200;
}

ZReportDataCb HdiConnection::reportDataCb_ = nullptr;
sptr<ReportDataCallback> HdiConnection::reportDataCallback_ = nullptr;

int32_t HdiConnection::ConnectHdi()
{
    HiLog::Debug(LABEL, "%{public}s begin", __func__);
    int32_t retry = 0;
    while (retry < GET_HDI_SERVICE_COUNT) {
        sensorInterface_ = ISensorInterface::Get();
        if (sensorInterface_ != nullptr) {
            HiLog::Info(LABEL, "%{public}s connect v1_0 hdi success", __func__);
            eventCallback_ = new (std::nothrow) SensorEventCallback();
            if (eventCallback_ == nullptr) {
                HiLog::Error(LABEL, "%{public}s failed to initialize eventCallback", __func__);
                return ERR_NO_INIT;
            }
            RegisterHdiDeathRecipient();
            return ERR_OK;
        }
        retry++;
        HiLog::Warn(LABEL, "%{public}s connect hdi service failed, retry : %{public}d", __func__, retry);
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_MS));
    }
    HiLog::Error(LABEL, "%{public}s connect v1_0 hdi failed", __func__);
    return ERR_NO_INIT;
}

int32_t HdiConnection::GetSensorList(std::vector<Sensor>& sensorList)
{
    HiLog::Debug(LABEL, "%{public}s begin", __func__);
    if (sensorInterface_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s connect v1_0 hdi failed", __func__);
        return ERR_NO_INIT;
    }
    std::vector<HdfSensorInformation> sensorInfos;
    int32_t ret = sensorInterface_->GetAllSensorInfo(sensorInfos);
    if (ret != 0) {
        HiLog::Error(LABEL, "%{public}s get sensor list failed", __func__);
        return ret;
    }
    for (int32_t i = 0; i < static_cast<int32_t>(sensorInfos.size()); i++) {
        const std::string sensorName(sensorInfos[i].sensorName);
        const std::string vendorName(sensorInfos[i].vendorName);
        const std::string firmwareVersion(sensorInfos[i].firmwareVersion);
        const std::string hardwareVersion(sensorInfos[i].hardwareVersion);
        const int32_t sensorId = sensorInfos[i].sensorId;
        const float maxRange = sensorInfos[i].maxRange;
        Sensor sensor;
        sensor.SetSensorId(sensorId);
        sensor.SetSensorTypeId(sensorId);
        sensor.SetFirmwareVersion(firmwareVersion.c_str());
        sensor.SetHardwareVersion(hardwareVersion.c_str());
        sensor.SetMaxRange(maxRange);
        sensor.SetSensorName(sensorName.c_str());
        sensor.SetVendorName(vendorName.c_str());
        sensor.SetResolution(sensorInfos[i].accuracy);
        sensor.SetPower(sensorInfos[i].power);
        sensorList.push_back(sensor);
    }
    HiLog::Debug(LABEL, "%{public}s end", __func__);
    return ERR_OK;
}

int32_t HdiConnection::EnableSensor(int32_t sensorId)
{
    if (sensorInterface_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s connect v1_0 hdi failed", __func__);
        return ERR_NO_INIT;
    }
    int32_t ret = sensorInterface_->Enable(sensorId);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s is failed", __func__);
        return ret;
    }
    setSensorBasicInfoState(sensorId, true);
    return ERR_OK;
}

int32_t HdiConnection::DisableSensor(int32_t sensorId)
{
    if (sensorInterface_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s connect v1_0 hdi failed", __func__);
        return ERR_NO_INIT;
    }
    int32_t ret = sensorInterface_->Disable(sensorId);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s is failed", __func__);
        return ret;
    }
    deleteSensorBasicInfoState(sensorId);
    return ERR_OK;
}

int32_t HdiConnection::SetBatch(int32_t sensorId, int64_t samplingInterval, int64_t reportInterval)
{
    if (sensorInterface_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s connect v1_0 hdi failed", __func__);
        return ERR_NO_INIT;
    }
    int32_t ret = sensorInterface_->SetBatch(sensorId, samplingInterval, reportInterval);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        return ret;
    }
    updateSensorBasicInfo(sensorId, samplingInterval, reportInterval);
    return ERR_OK;
}

int32_t HdiConnection::SetMode(int32_t sensorId, int32_t mode)
{
    HiLog::Debug(LABEL, "%{public}s begin", __func__);
    if (sensorInterface_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s connect v1_0 hdi failed", __func__);
        return ERR_NO_INIT;
    }
    int32_t ret = sensorInterface_->SetMode(sensorId, mode);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s is failed", __func__);
        return ret;
    }
    HiLog::Debug(LABEL, "%{public}s end", __func__);
    return ERR_OK;
}

int32_t HdiConnection::SetOption(int32_t sensorId, int32_t option)
{
    HiLog::Debug(LABEL, "%{public}s begin", __func__);
    if (sensorInterface_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s connect v1_0 hdi failed", __func__);
        return ERR_NO_INIT;
    }
    int32_t ret = sensorInterface_->SetOption(sensorId, option);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s is failed", __func__);
        return ret;
    }
    HiLog::Debug(LABEL, "%{public}s end", __func__);
    return ERR_OK;
}

int32_t HdiConnection::RegisteDataReport(ZReportDataCb cb, sptr<ReportDataCallback> reportDataCallback)
{
    HiLog::Debug(LABEL, "%{public}s begin", __func__);
    if (reportDataCallback == nullptr || sensorInterface_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s failed, reportDataCallback or sensorInterface_ cannot be null", __func__);
        return ERR_NO_INIT;
    }
    int32_t ret = sensorInterface_->Register(0, eventCallback_);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        return ret;
    }
    reportDataCb_ = cb;
    reportDataCallback_ = reportDataCallback;
    HiLog::Debug(LABEL, "%{public}s end", __func__);
    return ERR_OK;
}

int32_t HdiConnection::DestroyHdiConnection()
{
    HiLog::Debug(LABEL, "%{public}s begin", __func__);
    if (sensorInterface_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s connect v1_0 hdi failed", __func__);
        return ERR_NO_INIT;
    }
    int32_t ret = sensorInterface_->Unregister(0, eventCallback_);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        return ret;
    }
    eventCallback_ = nullptr;
    UnregisterHdiDeathRecipient();
    HiLog::Debug(LABEL, "%{public}s end", __func__);
    return ERR_OK;
}

int32_t HdiConnection::RunCommand(int32_t sensorId, int32_t cmd, int32_t params)
{
    return 0;
}

ZReportDataCb HdiConnection::getReportDataCb()
{
    HiLog::Debug(LABEL, "%{public}s begin", __func__);
    if (reportDataCb_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s reportDataCb_ cannot be null", __func__);
    }
    HiLog::Debug(LABEL, "%{public}s end", __func__);
    return reportDataCb_;
}

sptr<ReportDataCallback> HdiConnection::getReportDataCallback()
{
    HiLog::Debug(LABEL, "%{public}s begin", __func__);
    if (reportDataCallback_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s reportDataCallback_ cannot be null", __func__);
    }
    HiLog::Debug(LABEL, "%{public}s end", __func__);
    return reportDataCallback_;
}

void HdiConnection::updateSensorBasicInfo(int32_t sensorId, int64_t samplingPeriodNs, int64_t maxReportDelayNs)
{
    std::lock_guard<std::mutex> sensorInfoLock(sensorBasicInfoMutex_);
    SensorBasicInfo sensorBasicInfo;
    sensorBasicInfo.SetSamplingPeriodNs(samplingPeriodNs);
    sensorBasicInfo.SetMaxReportDelayNs(maxReportDelayNs);
    sensorBasicInfoMap_[sensorId] = sensorBasicInfo;
}

void HdiConnection::setSensorBasicInfoState(int32_t sensorId, bool state)
{
    std::lock_guard<std::mutex> sensorInfoLock(sensorBasicInfoMutex_);
    auto it = sensorBasicInfoMap_.find(sensorId);
    if (it == sensorBasicInfoMap_.end()) {
        HiLog::Warn(LABEL, "%{public}s should set batch first", __func__);
        return;
    }
    sensorBasicInfoMap_[sensorId].SetSensorState(state);
}

void HdiConnection::deleteSensorBasicInfoState(int32_t sensorId)
{
    std::lock_guard<std::mutex> sensorInfoLock(sensorBasicInfoMutex_);
    auto it = sensorBasicInfoMap_.find(sensorId);
    if (it != sensorBasicInfoMap_.end()) {
        sensorBasicInfoMap_.erase(sensorId);
    }
}

void HdiConnection::RegisterHdiDeathRecipient()
{
    HiLog::Debug(LABEL, "%{public}s begin", __func__);
    if (sensorInterface_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s connect v1_0 hdi failed", __func__);
        return;
    }
    hdiDeathObserver_ = new (std::nothrow) DeathRecipientTemplate(*const_cast<HdiConnection *>(this));
    if (hdiDeathObserver_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s hdiDeathObserver_ cannot be null", __func__);
        return;
    }
    sensorInterface_->AsObject()->AddDeathRecipient(hdiDeathObserver_);
    HiLog::Debug(LABEL, "%{public}s end", __func__);
}

void HdiConnection::UnregisterHdiDeathRecipient()
{
    HiLog::Debug(LABEL, "%{public}s begin", __func__);
    if (sensorInterface_ == nullptr || hdiDeathObserver_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s sensorInterface_ or hdiDeathObserver_ is null", __func__);
        return;
    }
    sensorInterface_->AsObject()->RemoveDeathRecipient(hdiDeathObserver_);
    HiLog::Debug(LABEL, "%{public}s end", __func__);
}

void HdiConnection::ProcessDeathObserver(const wptr<IRemoteObject> &object)
{
    HiLog::Debug(LABEL, "%{public}s begin", __func__);
    sptr<IRemoteObject> hdiService = object.promote();
    if (hdiService == nullptr) {
        HiLog::Error(LABEL, "%{public}s invalid remote object", __func__);
        return;
    }
    hdiService->RemoveDeathRecipient(hdiDeathObserver_);
    eventCallback_ = nullptr;
    reconnect();
    HiLog::Debug(LABEL, "%{public}s end", __func__);
}

void HdiConnection::reconnect()
{
    HiLog::Debug(LABEL, "%{public}s begin", __func__);
    int32_t ret = ConnectHdi();
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s failed to get an instance of hdi service", __func__);
        return;
    }
    ret = sensorInterface_->Register(0, eventCallback_);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s register callback fail", __func__);
        return;
    }
    std::vector<Sensor> sensorList;
    ret = GetSensorList(sensorList);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s get sensor list fail", __func__);
        return;
    }
    std::lock_guard<std::mutex> sensorInfoLock(sensorBasicInfoMutex_);
    for (const auto &sensorInfo: sensorBasicInfoMap_) {
        int32_t sensorTypeId = sensorInfo.first;
        ret = SetBatch(sensorTypeId, sensorInfo.second.GetSamplingPeriodNs(),
            sensorInfo.second.GetMaxReportDelayNs());
        if (ret < 0 || sensorInfo.second.GetSensorState() != true) {
            HiLog::Error(LABEL, "%{public}s sensorTypeId: %{public}d set batch fail or not need enable sensor",
                __func__, sensorTypeId);
            continue;
        }
        ret = EnableSensor(sensorTypeId);
        if (ret < 0) {
            HiLog::Error(LABEL, "%{public}s enable sensor fail, sensorTypeId: %{public}d", __func__, sensorTypeId);
        }
    }
    HiLog::Debug(LABEL, "%{public}s end", __func__);
}
}  // namespace Sensors
}  // namespace OHOS
