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
#include <thread>

#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
#include "hisysevent.h"
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
#include "iproxy_broker.h"
#include "v3_0/isensor_interface.h"

#include "sensor_agent_type.h"
#include "sensor_errors.h"
#include "sensor_event_callback.h"
#include "sensor_plug_callback.h"

#undef LOG_TAG
#define LOG_TAG "HdiConnection"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
using OHOS::HDI::Sensor::V3_0::ISensorInterface;
using OHOS::HDI::Sensor::V3_0::ISensorCallback;
using OHOS::HDI::Sensor::V3_0::HdfSensorInformation;
using OHOS::HDI::Sensor::V3_0::ISensorPlugCallback;
namespace {
sptr<ISensorInterface> g_sensorInterface = nullptr;
sptr<ISensorCallback> g_eventCallback = nullptr;
sptr<ISensorPlugCallback> g_plugCallback = nullptr;
std::map<std::string, SensorBasicInfo> g_sensorBasicInfoMap;
std::mutex g_sensorBasicInfoMutex;
constexpr int32_t GET_HDI_SERVICE_COUNT = 25;
constexpr uint32_t WAIT_MS = 200;
constexpr int32_t HEADPOSTURE_FIFO_COUNT = 5;
constexpr int32_t DEFAULT_BASE = 10;
}  // namespace

ReportDataCb HdiConnection::reportDataCb_ = nullptr;
DevicePlugCallback HdiConnection::reportPlugDataCb_ = nullptr;
sptr<ReportDataCallback> HdiConnection::reportDataCallback_ = nullptr;

int32_t HdiConnection::ConnectHdi()
{
    CALL_LOG_ENTER;
    int32_t retry = 0;
    while (retry < GET_HDI_SERVICE_COUNT) {
        g_sensorInterface = ISensorInterface::Get();
        if (g_sensorInterface != nullptr) {
            SEN_HILOGI("Connect v3_0 hdi success");
            g_eventCallback = new (std::nothrow) SensorEventCallback();
            CHKPR(g_eventCallback, ERR_NO_INIT);
            g_plugCallback = new (std::nothrow) SensorPlugCallback();
            CHKPR(g_plugCallback, ERR_NO_INIT);
            RegisterHdiDeathRecipient();
            return ERR_OK;
        }
        retry++;
        SEN_HILOGW("Connect hdi service failed, retry:%{public}d", retry);
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_MS));
    }
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
    HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::SENSOR, "HDF_SERVICE_EXCEPTION",
        HiSysEvent::EventType::FAULT, "PKG_NAME", "ConnectHdi", "ERROR_CODE", CONNECT_SENSOR_HDF_ERR);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
    SEN_HILOGE("Connect v3_0 hdi failed");
    return ERR_NO_INIT;
}

int32_t HdiConnection::GetSensorList(std::vector<Sensor> &sensorList)
{
    CALL_LOG_ENTER;
    CHKPR(g_sensorInterface, ERR_NO_INIT);
    std::vector<HdfSensorInformation> sensorInfos;
    int32_t ret = g_sensorInterface->GetAllSensorInfo(sensorInfos);
    if (ret != 0) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::SENSOR, "HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "GetSensorList", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        SEN_HILOGE("Get sensor list failed");
        return ret;
    }
    size_t count = sensorInfos.size();
    if (count > MAX_SENSOR_COUNT) {
        SEN_HILOGD("SensorInfos size:%{public}zu", count);
        count = MAX_SENSOR_COUNT;
    }
    for (size_t i = 0; i < count; i++) {
        Sensor sensor;
        sensor.SetDeviceId(sensorInfos[i].deviceSensorInfo.deviceId);
        sensor.SetSensorId(sensorInfos[i].deviceSensorInfo.sensorId);
        sensor.SetSensorTypeId(sensorInfos[i].deviceSensorInfo.sensorType);
        sensor.SetLocation(sensorInfos[i].deviceSensorInfo.location);
        sensor.SetFirmwareVersion(sensorInfos[i].firmwareVersion);
        sensor.SetHardwareVersion(sensorInfos[i].hardwareVersion);
        sensor.SetMaxRange(sensorInfos[i].maxRange);
        sensor.SetSensorName(sensorInfos[i].sensorName);
        sensor.SetVendorName(sensorInfos[i].vendorName);
        sensor.SetResolution(sensorInfos[i].accuracy);
        sensor.SetPower(sensorInfos[i].power);
        sensor.SetMinSamplePeriodNs(sensorInfos[i].minDelay);
        sensor.SetMaxSamplePeriodNs(sensorInfos[i].maxDelay);
        if (sensorInfos[i].deviceSensorInfo.sensorId == SENSOR_TYPE_ID_HEADPOSTURE) {
            sensor.SetFifoMaxEventCount(HEADPOSTURE_FIFO_COUNT);
        }
        sensorList.push_back(sensor);
    }
    return ERR_OK;
}

int32_t HdiConnection::EnableSensor(SensorDescription sensorDesc)
{
    SEN_HILOGI("In, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    CHKPR(g_sensorInterface, ERR_NO_INIT);
    int32_t ret = g_sensorInterface->Enable({sensorDesc.deviceId, sensorDesc.sensorType,
        sensorDesc.sensorId, sensorDesc.location});
    if (ret != 0) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::SENSOR, "HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "EnableSensor", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        SEN_HILOGE("Connect v3_0 hdi failed");
        return ret;
    }
    SetSensorBasicInfoState(sensorDesc, true);
    SEN_HILOGI("Done, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    return ERR_OK;
}

int32_t HdiConnection::DisableSensor(SensorDescription sensorDesc)
{
    SEN_HILOGI("In, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    CHKPR(g_sensorInterface, ERR_NO_INIT);
    int32_t ret = g_sensorInterface->Disable({sensorDesc.deviceId, sensorDesc.sensorType,
        sensorDesc.sensorId, sensorDesc.location});
    if (ret != 0) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::SENSOR, "HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "DisableSensor", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        SEN_HILOGE("Disable is failed");
        return ret;
    }
    DeleteSensorBasicInfoState(sensorDesc);
    SEN_HILOGI("Done, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    return ERR_OK;
}

int32_t HdiConnection::SetBatch(SensorDescription sensorDesc, int64_t samplingInterval, int64_t reportInterval)
{
    CHKPR(g_sensorInterface, ERR_NO_INIT);
    int32_t ret = g_sensorInterface->SetBatch({sensorDesc.deviceId, sensorDesc.sensorType,
        sensorDesc.sensorId, sensorDesc.location}, samplingInterval, reportInterval);
    if (ret != 0) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::SENSOR, "HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "SetBatch", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        SEN_HILOGE("SetBatch is failed");
        return ret;
    }
    UpdateSensorBasicInfo(sensorDesc, samplingInterval, reportInterval);
    return ERR_OK;
}

int32_t HdiConnection::SetMode(SensorDescription sensorDesc, int32_t mode)
{
    CALL_LOG_ENTER;
    CHKPR(g_sensorInterface, ERR_NO_INIT);
    int32_t ret = g_sensorInterface->SetMode({sensorDesc.deviceId, sensorDesc.sensorType,
        sensorDesc.sensorId, sensorDesc.location}, mode);
    if (ret != 0) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::SENSOR, "HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "SetMode", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        SEN_HILOGE("SetMode is failed");
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnection::RegisterDataReport(ReportDataCb cb, sptr<ReportDataCallback> reportDataCallback)
{
    CALL_LOG_ENTER;
    CHKPR(reportDataCallback, ERR_NO_INIT);
    CHKPR(g_sensorInterface, ERR_NO_INIT);
    int32_t ret = g_sensorInterface->RegisterAsync(0, g_eventCallback);
    if (ret != 0) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::SENSOR, "HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "RegisterDataReport", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        SEN_HILOGE("RegisterAsync is failed");
        return ret;
    }
    reportDataCb_ = cb;
    reportDataCallback_ = reportDataCallback;
    return ERR_OK;
}

int32_t HdiConnection::DestroyHdiConnection()
{
    CALL_LOG_ENTER;
    CHKPR(g_sensorInterface, ERR_NO_INIT);
    int32_t ret = g_sensorInterface->UnregisterAsync(0, g_eventCallback);
    if (ret != 0) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::SENSOR, "HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "DestroyHdiConnection", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        SEN_HILOGE("UnregisterAsync is failed");
        return ret;
    }
    ret = g_sensorInterface->UnRegSensorPlugCallBack(g_plugCallback);
    if (ret != 0) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::SENSOR, "HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "UnRegSensorPlugCallback", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        SEN_HILOGE("UnRegSensorPlugCallback is failed");
        return ret;
    }
    g_plugCallback = nullptr;
    g_eventCallback = nullptr;
    UnregisterHdiDeathRecipient();
    return ERR_OK;
}

int32_t HdiConnection::RegSensorPlugCallback(DevicePlugCallback cb)
{
    CALL_LOG_ENTER;
    CHKPR(cb, ERR_NO_INIT);
    CHKPR(g_sensorInterface, ERR_NO_INIT);
    reportPlugDataCb_ = cb;
    int32_t ret = g_sensorInterface->RegSensorPlugCallBack(g_plugCallback);
    if (ret != 0) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::SENSOR, "HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "RegSensorPlugCallback", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        SEN_HILOGE("RegSensorPlugCallback is failed");
        return ret;
    }
    return ERR_OK;
}

DevicePlugCallback HdiConnection::GetSensorPlugCb()
{
    if (reportPlugDataCb_ == nullptr) {
        SEN_HILOGE("reportPlugDataCb_ cannot be null");
    }
    return reportPlugDataCb_;
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

void HdiConnection::UpdateSensorBasicInfo(SensorDescription sensorDesc, int64_t samplingPeriodNs,
    int64_t maxReportDelayNs)
{
    std::lock_guard<std::mutex> sensorInfoLock(g_sensorBasicInfoMutex);
    SensorBasicInfo sensorBasicInfo;
    sensorBasicInfo.SetSamplingPeriodNs(samplingPeriodNs);
    sensorBasicInfo.SetMaxReportDelayNs(maxReportDelayNs);
    std::string sensorDescName;
    GetSensorDescName(sensorDesc, sensorDescName);
    auto it = g_sensorBasicInfoMap.find(sensorDescName);
    if (it != g_sensorBasicInfoMap.end()) {
        if (g_sensorBasicInfoMap[sensorDescName].GetSensorState()) {
            sensorBasicInfo.SetSensorState(true);
        }
    }
    g_sensorBasicInfoMap[sensorDescName] = sensorBasicInfo;
}

void HdiConnection::SetSensorBasicInfoState(SensorDescription sensorDesc, bool state)
{
    SEN_HILOGI("In, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    std::lock_guard<std::mutex> sensorInfoLock(g_sensorBasicInfoMutex);
    std::string sensorDescName;
    GetSensorDescName(sensorDesc, sensorDescName);
    auto it = g_sensorBasicInfoMap.find(sensorDescName);
    if (it == g_sensorBasicInfoMap.end()) {
        SEN_HILOGW("Should set batch first");
        return;
    }
    g_sensorBasicInfoMap[sensorDescName].SetSensorState(state);
    SEN_HILOGI("Done, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
}

void HdiConnection::DeleteSensorBasicInfoState(SensorDescription sensorDesc)
{
    SEN_HILOGI("In, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    std::lock_guard<std::mutex> sensorInfoLock(g_sensorBasicInfoMutex);
    std::string sensorDescName;
    GetSensorDescName(sensorDesc, sensorDescName);
    auto it = g_sensorBasicInfoMap.find(sensorDescName);
    if (it != g_sensorBasicInfoMap.end()) {
        g_sensorBasicInfoMap.erase(sensorDescName);
    }
    SEN_HILOGI("Done,deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
}

void HdiConnection::RegisterHdiDeathRecipient()
{
    CALL_LOG_ENTER;
    CHKPV(g_sensorInterface);
    if (hdiDeathObserver_ == nullptr) {
        hdiDeathObserver_ = new (std::nothrow) DeathRecipientTemplate(*const_cast<HdiConnection *>(this));
        CHKPV(hdiDeathObserver_);
    }
    OHOS::HDI::hdi_objcast<ISensorInterface>(g_sensorInterface)->AddDeathRecipient(hdiDeathObserver_);
}

void HdiConnection::UnregisterHdiDeathRecipient()
{
    CALL_LOG_ENTER;
    CHKPV(g_sensorInterface);
    CHKPV(hdiDeathObserver_);
    OHOS::HDI::hdi_objcast<ISensorInterface>(g_sensorInterface)->RemoveDeathRecipient(hdiDeathObserver_);
}

void HdiConnection::ProcessDeathObserver(const wptr<IRemoteObject> &object)
{
    CALL_LOG_ENTER;
    sptr<IRemoteObject> hdiService = object.promote();
    CHKPV(hdiService);
    CHKPV(hdiDeathObserver_);
    hdiService->RemoveDeathRecipient(hdiDeathObserver_);
    g_eventCallback = nullptr;
    Reconnect();
}

void HdiConnection::Reconnect()
{
    CALL_LOG_ENTER;
    int32_t ret = ConnectHdi();
    if (ret != 0) {
        SEN_HILOGE("Failed to get an instance of hdi service");
        return;
    }
    ret = g_sensorInterface->Register(0, g_eventCallback);
    if (ret != 0) {
        SEN_HILOGE("Register callback fail");
        return;
    }
    ret = g_sensorInterface->RegSensorPlugCallBack(g_plugCallback);
    if (ret != 0) {
        SEN_HILOGE("Register plug callback fail");
        return;
    }
    std::vector<Sensor> sensorList;
    ret = GetSensorList(sensorList);
    if (ret != 0) {
        SEN_HILOGE("Get sensor list fail");
        return;
    }
    std::lock_guard<std::mutex> sensorInfoLock(g_sensorBasicInfoMutex);
    for (const auto &sensorInfo: g_sensorBasicInfoMap) {
        SensorDescription sensorDesc;
        ParseIndex(sensorInfo.first, sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId,
            sensorDesc.location);
        SensorBasicInfo info = sensorInfo.second;
        if (info.GetSensorState() != true) {
            SEN_HILOGE("deviceId:%{public}d, sensortype:%{public}d, sensorId:%{public}d don't need enable sensor",
                sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
            continue;
        }
        ret = g_sensorInterface->SetBatch({sensorDesc.deviceId, sensorDesc.sensorType,
            sensorDesc.sensorId, sensorDesc.location}, info.GetSamplingPeriodNs(), info.GetMaxReportDelayNs());
        if (ret != 0) {
            SEN_HILOGE("deviceId:%{public}d, sensortype:%{public}d, sensorId:%{public}d set batch fail,"
                "error:%{public}d", sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId, ret);
            continue;
        }
        ret = g_sensorInterface->Enable({sensorDesc.deviceId, sensorDesc.sensorType,
            sensorDesc.sensorId, sensorDesc.location});
        if (ret != 0) {
            SEN_HILOGE("Enable fail, deviceId:%{public}d, sensortype:%{public}d, sensorId:%{public}d,"
                "error:%{public}d", sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId, ret);
        }
    }
}

int32_t HdiConnection::GetSensorListByDevice(int32_t deviceId, std::vector<Sensor> &singleDevSensors)
{
    CALL_LOG_ENTER;
    CHKPR(g_sensorInterface, ERR_NO_INIT);
    std::vector<HdfSensorInformation> sensorInfos;
    int32_t ret = g_sensorInterface->GetDeviceSensorInfo(deviceId, sensorInfos);
    if (ret != 0) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::SENSOR, "HDF_SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "GetSensorListByDevice", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        SEN_HILOGE("Get sensor list by device failed");
        return ret;
    }
    size_t count = sensorInfos.size();
    if (count > MAX_SENSOR_COUNT) {
        SEN_HILOGD("SensorInfos size:%{public}zu", count);
        count = MAX_SENSOR_COUNT;
    }
    for (size_t i = 0; i < count; i++) {
        Sensor sensor;
        sensor.SetDeviceId(sensorInfos[i].deviceSensorInfo.deviceId);
        sensor.SetSensorId(sensorInfos[i].deviceSensorInfo.sensorId);
        sensor.SetSensorTypeId(sensorInfos[i].deviceSensorInfo.sensorType);
        sensor.SetLocation(sensorInfos[i].deviceSensorInfo.location);
        sensor.SetFirmwareVersion(sensorInfos[i].firmwareVersion);
        sensor.SetHardwareVersion(sensorInfos[i].hardwareVersion);
        sensor.SetMaxRange(sensorInfos[i].maxRange);
        sensor.SetSensorName(sensorInfos[i].sensorName);
        sensor.SetVendorName(sensorInfos[i].vendorName);
        sensor.SetResolution(sensorInfos[i].accuracy);
        sensor.SetPower(sensorInfos[i].power);
        sensor.SetMinSamplePeriodNs(sensorInfos[i].minDelay);
        sensor.SetMaxSamplePeriodNs(sensorInfos[i].maxDelay);
        if (sensorInfos[i].deviceSensorInfo.sensorId == SENSOR_TYPE_ID_HEADPOSTURE) {
            sensor.SetFifoMaxEventCount(HEADPOSTURE_FIFO_COUNT);
        }
        singleDevSensors.push_back(sensor);
    }
    return ERR_OK;
}

void HdiConnection::GetSensorDescName(SensorDescription sensorDesc, std::string &sensorDescName)
{
    sensorDescName = std::to_string(sensorDesc.deviceId) + "#" + std::to_string(sensorDesc.sensorType) +
        "#" + std::to_string(sensorDesc.sensorId)+ "#" + std::to_string(sensorDesc.location);
    return;
}

void HdiConnection::ParseIndex(const std::string &sensorDescName, int32_t &deviceId, int32_t &sensorType,
    int32_t &sensorId, int32_t &location)
{
    size_t first_hash = sensorDescName.find('#');
    size_t second_hash = sensorDescName.find('#', first_hash + 1);
    size_t third_hash = sensorDescName.find('#', second_hash + 1);

    deviceId = static_cast<int32_t>(strtol(sensorDescName.substr(0, first_hash).c_str(), nullptr, DEFAULT_BASE));
    sensorType = static_cast<int32_t>(strtol(sensorDescName.substr(first_hash + 1,
        second_hash - first_hash - 1).c_str(), nullptr, DEFAULT_BASE));
    sensorId = static_cast<int32_t>(strtol(sensorDescName.substr(second_hash + 1,
        third_hash - second_hash - 1).c_str(), nullptr, DEFAULT_BASE));
    location = static_cast<int32_t>(strtol(sensorDescName.substr(third_hash + 1).c_str(), nullptr, DEFAULT_BASE));
    return;
}
} // namespace Sensors
} // namespace OHOS
