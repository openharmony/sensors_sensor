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

#include "hisysevent.h"
#include "iproxy_broker.h"
#include "sensor_event_callback.h"
#include "sensors_errors.h"
#include "v1_0/isensor_interface.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
using OHOS::HDI::Sensor::V1_0::ISensorInterface;
using OHOS::HDI::Sensor::V1_0::ISensorCallback;
using OHOS::HDI::Sensor::V1_0::HdfSensorInformation;
namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "HdiConnection" };
sptr<ISensorInterface> sensorInterface_ = nullptr;
sptr<ISensorCallback> eventCallback_ = nullptr;
std::map<int32_t, SensorBasicInfo> sensorBasicInfoMap_;
std::mutex sensorBasicInfoMutex_;
constexpr int32_t GET_HDI_SERVICE_COUNT = 30;
constexpr uint32_t WAIT_MS = 200;
}

ReportDataCb HdiConnection::reportDataCb_ = nullptr;
sptr<ReportDataCallback> HdiConnection::reportDataCallback_ = nullptr;

int32_t HdiConnection::ConnectHdi()
{
    CALL_LOG_ENTER;
    int32_t retry = 0;
    while (retry < GET_HDI_SERVICE_COUNT) {
        sensorInterface_ = ISensorInterface::Get();
        if (sensorInterface_ != nullptr) {
            SEN_HILOGI("connect v1_0 hdi success");
            eventCallback_ = new (std::nothrow) SensorEventCallback();
            CHKPR(eventCallback_, ERR_NO_INIT);
            RegisterHdiDeathRecipient();
            return ERR_OK;
        }
        retry++;
        SEN_HILOGW("connect hdi service failed, retry:%{public}d", retry);
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_MS));
    }
    HiSysEvent::Write(HiviewDFX::HiSysEvent::Domain::SENSOR, "SENSOR_HDF_SERVICE_EXCEPTION",
        HiSysEvent::EventType::FAULT, "PKG_NAME", "ConnectHdi", "ERROR_CODE", CONNECT_SENSOR_HDF_ERR);
    SEN_HILOGE("connect v1_0 hdi failed");
    return ERR_NO_INIT;
}

int32_t HdiConnection::GetSensorList(std::vector<Sensor>& sensorList)
{
    CALL_LOG_ENTER;
    CHKPR(sensorInterface_, ERR_NO_INIT);
    std::vector<HdfSensorInformation> sensorInfos;
    int32_t ret = sensorInterface_->GetAllSensorInfo(sensorInfos);
    if (ret != 0) {
        HiSysEvent::Write(HiviewDFX::HiSysEvent::Domain::SENSOR, "SENSOR_HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "GetSensorList", "ERROR_CODE", ret);
        SEN_HILOGE("get sensor list failed");
        return ret;
    }
    for (size_t i = 0; i < sensorInfos.size(); i++) {
        Sensor sensor;
        sensor.SetSensorId(sensorInfos[i].sensorId);
        sensor.SetSensorTypeId(sensorInfos[i].sensorId);
        sensor.SetFirmwareVersion(sensorInfos[i].firmwareVersion);
        sensor.SetHardwareVersion(sensorInfos[i].hardwareVersion);
        sensor.SetMaxRange(sensorInfos[i].maxRange);
        sensor.SetSensorName(sensorInfos[i].sensorName);
        sensor.SetVendorName(sensorInfos[i].vendorName);
        sensor.SetResolution(sensorInfos[i].accuracy);
        sensor.SetPower(sensorInfos[i].power);
        sensor.SetMinSamplePeriodNs(sensorInfos[i].minDelay);
        sensor.SetMaxSamplePeriodNs(sensorInfos[i].maxDelay);
        sensorList.push_back(sensor);
    }
    return ERR_OK;
}

int32_t HdiConnection::EnableSensor(int32_t sensorId)
{
    CHKPR(sensorInterface_, ERR_NO_INIT);
    int32_t ret = sensorInterface_->Enable(sensorId);
    if (ret != 0) {
        HiSysEvent::Write(HiviewDFX::HiSysEvent::Domain::SENSOR, "SENSOR_HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "EnableSensor", "ERROR_CODE", ret);
        SEN_HILOGE("connect v1_0 hdi failed");
        return ret;
    }
    SetSensorBasicInfoState(sensorId, true);
    return ERR_OK;
}

int32_t HdiConnection::DisableSensor(int32_t sensorId)
{
    CHKPR(sensorInterface_, ERR_NO_INIT);
    int32_t ret = sensorInterface_->Disable(sensorId);
    if (ret != 0) {
        HiSysEvent::Write(HiviewDFX::HiSysEvent::Domain::SENSOR, "SENSOR_HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "DisableSensor", "ERROR_CODE", ret);
        SEN_HILOGE("Disable is failed");
        return ret;
    }
    DeleteSensorBasicInfoState(sensorId);
    return ERR_OK;
}

int32_t HdiConnection::SetBatch(int32_t sensorId, int64_t samplingInterval, int64_t reportInterval)
{
    CHKPR(sensorInterface_, ERR_NO_INIT);
    int32_t ret = sensorInterface_->SetBatch(sensorId, samplingInterval, reportInterval);
    if (ret != 0) {
        HiSysEvent::Write(HiviewDFX::HiSysEvent::Domain::SENSOR, "SENSOR_HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "SetBatch", "ERROR_CODE", ret);
        SEN_HILOGE("SetBatch is failed");
        return ret;
    }
    UpdateSensorBasicInfo(sensorId, samplingInterval, reportInterval);
    return ERR_OK;
}

int32_t HdiConnection::SetMode(int32_t sensorId, int32_t mode)
{
    CALL_LOG_ENTER;
    CHKPR(sensorInterface_, ERR_NO_INIT);
    int32_t ret = sensorInterface_->SetMode(sensorId, mode);
    if (ret != 0) {
        HiSysEvent::Write(HiviewDFX::HiSysEvent::Domain::SENSOR, "SENSOR_HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "SetMode", "ERROR_CODE", ret);
        SEN_HILOGE("SetMode is failed");
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnection::RegisteDataReport(ReportDataCb cb, sptr<ReportDataCallback> reportDataCallback)
{
    CALL_LOG_ENTER;
    CHKPR(reportDataCallback, ERR_NO_INIT);
    CHKPR(sensorInterface_, ERR_NO_INIT);
    int32_t ret = sensorInterface_->Register(0, eventCallback_);
    if (ret != 0) {
        HiSysEvent::Write(HiviewDFX::HiSysEvent::Domain::SENSOR, "SENSOR_HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "RegisteDataReport", "ERROR_CODE", ret);
        SEN_HILOGE("Register is failed");
        return ret;
    }
    reportDataCb_ = cb;
    reportDataCallback_ = reportDataCallback;
    return ERR_OK;
}

int32_t HdiConnection::DestroyHdiConnection()
{
    CALL_LOG_ENTER;
    CHKPR(sensorInterface_, ERR_NO_INIT);
    int32_t ret = sensorInterface_->Unregister(0, eventCallback_);
    if (ret != 0) {
        HiSysEvent::Write(HiviewDFX::HiSysEvent::Domain::SENSOR, "SENSOR_HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "DestroyHdiConnection", "ERROR_CODE", ret);
        SEN_HILOGE("Unregister is failed");
        return ret;
    }
    eventCallback_ = nullptr;
    UnregisterHdiDeathRecipient();
    return ERR_OK;
}

ReportDataCb HdiConnection::GetReportDataCb()
{
    if (reportDataCb_ == nullptr) {
        SEN_HILOGE("reportDataCb_ cannot be null");
    }
    return reportDataCb_;
}

sptr<ReportDataCallback> HdiConnection::GetReportDataCallback()
{
    if (reportDataCallback_ == nullptr) {
        SEN_HILOGE("reportDataCallback_ cannot be null");
    }
    return reportDataCallback_;
}

void HdiConnection::UpdateSensorBasicInfo(int32_t sensorId, int64_t samplingPeriodNs, int64_t maxReportDelayNs)
{
    std::lock_guard<std::mutex> sensorInfoLock(sensorBasicInfoMutex_);
    SensorBasicInfo sensorBasicInfo;
    sensorBasicInfo.SetSamplingPeriodNs(samplingPeriodNs);
    sensorBasicInfo.SetMaxReportDelayNs(maxReportDelayNs);
    sensorBasicInfoMap_[sensorId] = sensorBasicInfo;
}

void HdiConnection::SetSensorBasicInfoState(int32_t sensorId, bool state)
{
    std::lock_guard<std::mutex> sensorInfoLock(sensorBasicInfoMutex_);
    auto it = sensorBasicInfoMap_.find(sensorId);
    if (it == sensorBasicInfoMap_.end()) {
        SEN_HILOGW("should set batch first");
        return;
    }
    sensorBasicInfoMap_[sensorId].SetSensorState(state);
}

void HdiConnection::DeleteSensorBasicInfoState(int32_t sensorId)
{
    std::lock_guard<std::mutex> sensorInfoLock(sensorBasicInfoMutex_);
    auto it = sensorBasicInfoMap_.find(sensorId);
    if (it != sensorBasicInfoMap_.end()) {
        sensorBasicInfoMap_.erase(sensorId);
    }
}

void HdiConnection::RegisterHdiDeathRecipient()
{
    CALL_LOG_ENTER;
    CHKPV(sensorInterface_);
    hdiDeathObserver_ = new (std::nothrow) DeathRecipientTemplate(*const_cast<HdiConnection *>(this));
    CHKPV(hdiDeathObserver_);
    OHOS::HDI::hdi_objcast<ISensorInterface>(sensorInterface_)->AddDeathRecipient(hdiDeathObserver_);
}

void HdiConnection::UnregisterHdiDeathRecipient()
{
    CALL_LOG_ENTER;
    CHKPV(sensorInterface_);
    CHKPV(hdiDeathObserver_);
    OHOS::HDI::hdi_objcast<ISensorInterface>(sensorInterface_)->RemoveDeathRecipient(hdiDeathObserver_);
}

void HdiConnection::ProcessDeathObserver(const wptr<IRemoteObject> &object)
{
    CALL_LOG_ENTER;
    sptr<IRemoteObject> hdiService = object.promote();
    CHKPV(hdiService);
    hdiService->RemoveDeathRecipient(hdiDeathObserver_);
    eventCallback_ = nullptr;
    Reconnect();
}

void HdiConnection::Reconnect()
{
    CALL_LOG_ENTER;
    int32_t ret = ConnectHdi();
    if (ret != 0) {
        SEN_HILOGE("failed to get an instance of hdi service");
        return;
    }
    ret = sensorInterface_->Register(0, eventCallback_);
    if (ret != 0) {
        SEN_HILOGE("register callback fail");
        return;
    }
    std::vector<Sensor> sensorList;
    ret = GetSensorList(sensorList);
    if (ret != 0) {
        SEN_HILOGE("get sensor list fail");
        return;
    }
    std::lock_guard<std::mutex> sensorInfoLock(sensorBasicInfoMutex_);
    for (const auto &sensorInfo: sensorBasicInfoMap_) {
        int32_t sensorTypeId = sensorInfo.first;
        SensorBasicInfo info = sensorInfo.second;
        if (info.GetSensorState() != true) {
            SEN_HILOGE("sensorTypeId: %{public}d don't need enable sensor", sensorTypeId);
            continue;
        }
        ret = sensorInterface_->SetBatch(sensorTypeId, info.GetSamplingPeriodNs(), info.GetMaxReportDelayNs());
        if (ret != 0) {
            SEN_HILOGE("sensorTypeId:%{public}d set batch fail,error:%{public}d", sensorTypeId, ret);
            continue;
        }
        ret = sensorInterface_->Enable(sensorTypeId);
        if (ret != 0) {
            SEN_HILOGE("enable sensor fail, sensorTypeId:%{public}d,error:%{public}d", sensorTypeId, ret);
        }
    }
}
}  // namespace Sensors
}  // namespace OHOS
