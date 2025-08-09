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
#include "sensor_xcollie.h"

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
std::map<SensorDescription, SensorBasicInfo> g_sensorBasicInfoMap;
std::mutex g_sensorBasicInfoMutex;
std::mutex g_sensorInterfaceMutex;
constexpr int32_t DEFAULT_GROUP_ID = 0;
constexpr int32_t GET_HDI_SERVICE_COUNT = 25;
constexpr uint32_t WAIT_MS = 200;
constexpr int32_t HEADPOSTURE_FIFO_COUNT = 5;
}  // namespace

ReportDataCb HdiConnection::reportDataCb_ = nullptr;
DevicePlugCallback HdiConnection::reportPlugDataCb_ = nullptr;
sptr<ReportDataCallback> HdiConnection::reportDataCallback_ = nullptr;

int32_t HdiConnection::ConnectHdi()
{
    CALL_LOG_ENTER;
    int32_t retry = 0;
    while (retry < GET_HDI_SERVICE_COUNT) {
        std::lock_guard<std::mutex> sensorInterface(g_sensorInterfaceMutex);
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
    std::lock_guard<std::mutex> sensorInterface(g_sensorInterfaceMutex);
    CHKPR(g_sensorInterface, ERR_NO_INIT);
    std::vector<HdfSensorInformation> sensorInfos;
    SensorXcollie sensorXcollie("HdiConnection:GetSensorList", XCOLLIE_TIMEOUT_5S);
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
        if (sensorInfos[i].deviceSensorInfo.sensorType == SENSOR_TYPE_ID_HEADPOSTURE) {
            sensor.SetFifoMaxEventCount(HEADPOSTURE_FIFO_COUNT);
        }
        sensorList.push_back(sensor);
    }
    return ERR_OK;
}

int32_t HdiConnection::EnableSensor(const SensorDescription &sensorDesc)
{
    SEN_HILOGI("In, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    std::lock_guard<std::mutex> sensorInterface(g_sensorInterfaceMutex);
    CHKPR(g_sensorInterface, ERR_NO_INIT);
    SensorXcollie sensorXcollie("HdiConnection:EnableSensor", XCOLLIE_TIMEOUT_5S);
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

int32_t HdiConnection::DisableSensor(const SensorDescription &sensorDesc)
{
    SEN_HILOGI("In, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    std::lock_guard<std::mutex> sensorInterface(g_sensorInterfaceMutex);
    CHKPR(g_sensorInterface, ERR_NO_INIT);
    SensorXcollie sensorXcollie("HdiConnection:DisableSensor", XCOLLIE_TIMEOUT_5S);
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

int32_t HdiConnection::SetBatch(const SensorDescription &sensorDesc, int64_t samplingInterval, int64_t reportInterval)
{
    std::lock_guard<std::mutex> sensorInterface(g_sensorInterfaceMutex);
    CHKPR(g_sensorInterface, ERR_NO_INIT);
    SensorXcollie sensorXcollie("HdiConnection:SetBatch", XCOLLIE_TIMEOUT_5S);
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

int32_t HdiConnection::SetMode(const SensorDescription &sensorDesc, int32_t mode)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> sensorInterface(g_sensorInterfaceMutex);
    CHKPR(g_sensorInterface, ERR_NO_INIT);
    SensorXcollie sensorXcollie("HdiConnection:SetMode", XCOLLIE_TIMEOUT_5S);
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
    std::lock_guard<std::mutex> sensorInterface(g_sensorInterfaceMutex);
    CHKPR(g_sensorInterface, ERR_NO_INIT);
    SensorXcollie sensorXcollie("HdiConnection:RegisterDataReport", XCOLLIE_TIMEOUT_5S);
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
    {
        std::lock_guard<std::mutex> sensorInterface(g_sensorInterfaceMutex);
        CHKPR(g_sensorInterface, ERR_NO_INIT);
        SensorXcollie unregisterAsyncXcollie("HdiConnection:UnregisterAsync", XCOLLIE_TIMEOUT_5S);
        int32_t ret = g_sensorInterface->UnregisterAsync(DEFAULT_GROUP_ID, g_eventCallback);
        if (ret != ERR_OK) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
            HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::SENSOR, "HDF_SERVICE_EXCEPTION",
                HiSysEvent::EventType::FAULT, "PKG_NAME", "DestroyHdiConnection", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
            SEN_HILOGE("UnregisterAsync is failed");
            return ret;
        }
        SensorXcollie unRegSensorPlugCallBackXcollie("HdiConnection:UnRegSensorPlugCallBack", XCOLLIE_TIMEOUT_5S);
        ret = g_sensorInterface->UnRegSensorPlugCallBack(g_plugCallback);
        if (ret != ERR_OK) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
            HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::SENSOR, "HDF_SERVICE_EXCEPTION",
                HiSysEvent::EventType::FAULT, "PKG_NAME", "UnRegSensorPlugCallback", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
            SEN_HILOGE("UnRegSensorPlugCallback is failed");
            return ret;
        }
        g_plugCallback = nullptr;
        g_eventCallback = nullptr;
    }
    UnregisterHdiDeathRecipient();
    return ERR_OK;
}

int32_t HdiConnection::RegSensorPlugCallback(DevicePlugCallback cb)
{
    CALL_LOG_ENTER;
    CHKPR(cb, ERR_NO_INIT);
    std::lock_guard<std::mutex> sensorInterface(g_sensorInterfaceMutex);
    CHKPR(g_sensorInterface, ERR_NO_INIT);
    reportPlugDataCb_ = cb;
    SensorXcollie sensorXcollie("HdiConnection:RegSensorPlugCallback", XCOLLIE_TIMEOUT_5S);
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

void HdiConnection::UpdateSensorBasicInfo(const SensorDescription &sensorDesc, int64_t samplingPeriodNs,
    int64_t maxReportDelayNs)
{
    std::lock_guard<std::mutex> sensorInfoLock(g_sensorBasicInfoMutex);
    SensorBasicInfo sensorBasicInfo;
    sensorBasicInfo.SetSamplingPeriodNs(samplingPeriodNs);
    sensorBasicInfo.SetMaxReportDelayNs(maxReportDelayNs);
    auto it = g_sensorBasicInfoMap.find(sensorDesc);
    if (it != g_sensorBasicInfoMap.end()) {
        if (g_sensorBasicInfoMap[sensorDesc].GetSensorState()) {
            sensorBasicInfo.SetSensorState(true);
        }
    }
    g_sensorBasicInfoMap[sensorDesc] = sensorBasicInfo;
}

void HdiConnection::SetSensorBasicInfoState(const SensorDescription &sensorDesc, bool state)
{
    SEN_HILOGI("In, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    std::lock_guard<std::mutex> sensorInfoLock(g_sensorBasicInfoMutex);
    auto it = g_sensorBasicInfoMap.find(sensorDesc);
    if (it == g_sensorBasicInfoMap.end()) {
        SEN_HILOGW("Should set batch first");
        return;
    }
    g_sensorBasicInfoMap[sensorDesc].SetSensorState(state);
    SEN_HILOGI("Done, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
}

void HdiConnection::DeleteSensorBasicInfoState(const SensorDescription &sensorDesc)
{
    SEN_HILOGI("In, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    std::lock_guard<std::mutex> sensorInfoLock(g_sensorBasicInfoMutex);
    auto it = g_sensorBasicInfoMap.find(sensorDesc);
    if (it != g_sensorBasicInfoMap.end()) {
        g_sensorBasicInfoMap.erase(sensorDesc);
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
    SensorXcollie sensorXcollie("HdiConnection:RegisterHdiDeathRecipient", XCOLLIE_TIMEOUT_5S);
    OHOS::HDI::hdi_objcast<ISensorInterface>(g_sensorInterface)->AddDeathRecipient(hdiDeathObserver_);
}

void HdiConnection::UnregisterHdiDeathRecipient()
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> sensorInterface(g_sensorInterfaceMutex);
    CHKPV(g_sensorInterface);
    CHKPV(hdiDeathObserver_);
    SensorXcollie sensorXcollie("HdiConnection:UnregisterHdiDeathRecipient", XCOLLIE_TIMEOUT_5S);
    OHOS::HDI::hdi_objcast<ISensorInterface>(g_sensorInterface)->RemoveDeathRecipient(hdiDeathObserver_);
}

void HdiConnection::ProcessDeathObserver(const wptr<IRemoteObject> &object)
{
    CALL_LOG_ENTER;
    sptr<IRemoteObject> hdiService = object.promote();
    CHKPV(hdiService);
    CHKPV(hdiDeathObserver_);
    {
        std::lock_guard<std::mutex> sensorInterface(g_sensorInterfaceMutex);
        hdiService->RemoveDeathRecipient(hdiDeathObserver_);
        g_eventCallback = nullptr;
    }
    Reconnect();
}

void HdiConnection::ReEnableSensor()
{
    std::lock_guard<std::mutex> sensorInfoLock(g_sensorBasicInfoMutex);
    for (const auto &sensorInfo: g_sensorBasicInfoMap) {
        SensorBasicInfo info = sensorInfo.second;
        if (!info.GetSensorState()) {
            SEN_HILOGE("deviceId:%{public}d, sensortype:%{public}d, sensorId:%{public}d don't need enable sensor",
                sensorInfo.first.deviceId, sensorInfo.first.sensorType, sensorInfo.first.sensorId);
            continue;
        }
        SensorXcollie setBatchXcollie("HdiConnection:Reconnect:SetBatch", XCOLLIE_TIMEOUT_5S);
        int32_t ret = g_sensorInterface->SetBatch({sensorInfo.first.deviceId, sensorInfo.first.sensorType,
            sensorInfo.first.sensorId, sensorInfo.first.location},
            info.GetSamplingPeriodNs(), info.GetMaxReportDelayNs());
        if (ret != ERR_OK) {
            SEN_HILOGE("deviceId:%{public}d, sensortype:%{public}d, sensorId:%{public}d set batch fail, err:%{public}d",
                sensorInfo.first.deviceId, sensorInfo.first.sensorType, sensorInfo.first.sensorId, ret);
            continue;
        }
        SensorXcollie enableXcollie("HdiConnection:Reconnect:ReEnable", XCOLLIE_TIMEOUT_5S);
        ret = g_sensorInterface->Enable({sensorInfo.first.deviceId, sensorInfo.first.sensorType,
            sensorInfo.first.sensorId, sensorInfo.first.location});
        if (ret != ERR_OK) {
            SEN_HILOGE("Enable fail, deviceId:%{public}d, sensortype:%{public}d, sensorId:%{public}d, err:%{public}d",
                sensorInfo.first.deviceId, sensorInfo.first.sensorType, sensorInfo.first.sensorId, ret);
        }
    }
}

void HdiConnection::Reconnect()
{
    CALL_LOG_ENTER;
    int32_t ret = ConnectHdi();
    if (ret != 0) {
        SEN_HILOGE("Failed to get an instance of hdi service");
        return;
    }
    {
        std::lock_guard<std::mutex> sensorInterface(g_sensorInterfaceMutex);
        SensorXcollie registerXcollie("HdiConnection:Reconnect:RegisterAsync", XCOLLIE_TIMEOUT_5S);
        ret = g_sensorInterface->RegisterAsync(DEFAULT_GROUP_ID, g_eventCallback);
        if (ret != ERR_OK) {
            SEN_HILOGE("RegisterAsync callback fail");
            return;
        }
        SensorXcollie regSensorPlugCallBackXcollie("HdiConnection:Reconnect:RegSensorPlugCallBack", XCOLLIE_TIMEOUT_5S);
        ret = g_sensorInterface->RegSensorPlugCallBack(g_plugCallback);
        if (ret != ERR_OK) {
            SEN_HILOGE("RegisterAsync plug callback fail");
            return;
        }
    }
    std::vector<Sensor> sensorList;
    ret = GetSensorList(sensorList);
    if (ret != ERR_OK) {
        SEN_HILOGE("Get sensor list fail");
        return;
    }
    std::lock_guard<std::mutex> sensorInterface(g_sensorInterfaceMutex);
    ReEnableSensor();
}

int32_t HdiConnection::GetSensorListByDevice(int32_t deviceId, std::vector<Sensor> &singleDevSensors)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> sensorInterface(g_sensorInterfaceMutex);
    CHKPR(g_sensorInterface, ERR_NO_INIT);
    std::vector<HdfSensorInformation> sensorInfos;
    SensorXcollie sensorXcollie("HdiConnection:GetSensorListByDevice", XCOLLIE_TIMEOUT_5S);
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
        if (sensorInfos[i].deviceSensorInfo.sensorType == SENSOR_TYPE_ID_HEADPOSTURE) {
            sensor.SetFifoMaxEventCount(HEADPOSTURE_FIFO_COUNT);
        }
        singleDevSensors.push_back(sensor);
    }
    return ERR_OK;
}
} // namespace Sensors
} // namespace OHOS
