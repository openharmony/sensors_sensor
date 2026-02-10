/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include "death_recipient_template.h"
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
#include "hisysevent.h"
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
#ifdef HIVIEWDFX_HITRACE_ENABLE
#include "hitrace_meter.h"
#endif // HIVIEWDFX_HITRACE_ENABLE
#include "sensor_agent_proxy.h"
#include "system_ability_definition.h"

#undef LOG_TAG
#define LOG_TAG "SensorServiceClient"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr int32_t LOCAL_DEVICE = 1;
constexpr int32_t LOADSA_TIMEOUT_MS = 10000;
} // namespace

SensorServiceClient::~SensorServiceClient()
{
    if (sensorServer_ != nullptr && serviceDeathObserver_ != nullptr) {
        auto remoteObject = sensorServer_->AsObject();
        if (remoteObject != nullptr) {
            remoteObject->RemoveDeathRecipient(serviceDeathObserver_);
        }
    }
    DestroyClientRemoteObject();
    Disconnect();
}

int32_t SensorServiceClient::DealAfterServiceAlive()
{
    CALL_LOG_ENTER;
    serviceDeathObserver_ = new (std::nothrow) DeathRecipientTemplate(*const_cast<SensorServiceClient *>(this));
    CHKPR(serviceDeathObserver_, SENSOR_NATIVE_GET_SERVICE_ERR);
    sptr<IRemoteObject> remoteObject = sensorServer_->AsObject();
    CHKPR(remoteObject, SENSOR_NATIVE_GET_SERVICE_ERR);
    remoteObject->AddDeathRecipient(serviceDeathObserver_);
    sensorList_.clear();
    int32_t ret = sensorServer_->GetSensorList(sensorList_);
    WriteHiSysIPCEvent(ISensorServiceIpcCode::COMMAND_GET_SENSOR_LIST, ret);
    if (sensorList_.empty()) {
        SEN_HILOGW("sensorList_ is empty when connecting to the service for the first time");
    }
    ret = TransferClientRemoteObject();
    if (ret != ERR_OK) {
        SEN_HILOGE("TransferClientRemoteObject failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t SensorServiceClient::InitServiceClient()
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    if (sensorServer_ != nullptr) {
        SEN_HILOGD("Already init");
        if (sensorList_.empty()) {
            int32_t ret = sensorServer_->GetSensorList(sensorList_);
            WriteHiSysIPCEvent(ISensorServiceIpcCode::COMMAND_GET_SENSOR_LIST, ret);
            SEN_HILOGW("sensorList is %{public}s", sensorList_.empty() ? "empty" : "not empty");
        }
        return ERR_OK;
    }
    if (sensorClientStub_ == nullptr) {
        sensorClientStub_ = new (std::nothrow) SensorClientStub();
    }
    sptr<ISystemAbilityManager> systemAbilityManager =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHKPR(systemAbilityManager, SENSOR_NATIVE_SAM_ERR);
    sensorServer_ = iface_cast<ISensorService>(systemAbilityManager->CheckSystemAbility(SENSOR_SERVICE_ABILITY_ID));
    if (sensorServer_ == nullptr) {
        if (!LoadSensorService()) {
            SEN_HILOGE("LoadSensorService failed");
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
            HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::SENSOR, "SERVICE_EXCEPTION",
                HiSysEvent::EventType::FAULT, "PKG_NAME", "InitServiceClient", "ERROR_CODE",
                SENSOR_NATIVE_GET_SERVICE_ERR);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
            return SENSOR_NATIVE_GET_SERVICE_ERR;
        }
        return ERR_OK;
    }
    SEN_HILOGD("Get service success");
    int32_t ret = DealAfterServiceAlive();
    if (ret != ERR_OK) {
        SEN_HILOGE("DealAfterServiceAlive failed, ret:%{public}d", ret);
    }
    return ret;
}

bool SensorServiceClient::LoadSensorService()
{ // LCOV_EXCL_START
    SEN_HILOGI("LoadSensorService in");
    sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        SEN_HILOGE("samgr is nullptr");
        return false;
    }
    auto sensorSa = samgr->LoadSystemAbility(SENSOR_SERVICE_ABILITY_ID, LOADSA_TIMEOUT_MS);
    if (sensorSa == nullptr) {
        SEN_HILOGE("Load sensor sa failed");
        return false;
    }
    sensorServer_ = iface_cast<ISensorService>(sensorSa);
    if (sensorServer_ == nullptr) {
        SEN_HILOGI("LoadSensorService out");
        return false;
    }
    SEN_HILOGW("LoadSensorService success");
    int32_t ret = DealAfterServiceAlive();
    if (ret != ERR_OK) {
        SEN_HILOGE("DealAfterServiceAlive failed, ret:%{public}d", ret);
        return false;
    }
    return true;
} // LCOV_EXCL_STOP

bool SensorServiceClient::IsValid(const SensorDescription &sensorDesc)
{
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    if (sensorList_.empty()) {
        SEN_HILOGE("sensorList_ cannot be empty");
        return false;
    }
    for (auto &sensor : sensorList_) {
        if ((sensor.GetDeviceId() == sensorDesc.deviceId) && (sensor.GetSensorTypeId() == sensorDesc.sensorType) &&
            (sensor.GetSensorId() == sensorDesc.sensorId)) {
            return true;
        }
    }
    return false;
}

int32_t SensorServiceClient::EnableSensor(const SensorDescription &sensorDesc, int64_t samplingPeriod,
    int64_t maxReportDelay)
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return ret;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPR(sensorServer_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "EnableSensor");
#endif // HIVIEWDFX_HITRACE_ENABLE
    ret = sensorServer_->EnableSensor({sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId,
        sensorDesc.location}, samplingPeriod, maxReportDelay);
    WriteHiSysIPCEvent(ISensorServiceIpcCode::COMMAND_ENABLE_SENSOR, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    if (ret == ERR_OK) {
        UpdateSensorInfoMap(sensorDesc, samplingPeriod, maxReportDelay);
    }
    return ret;
}

int32_t SensorServiceClient::DisableSensor(const SensorDescription &sensorDesc)
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return ret;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPR(sensorServer_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "DisableSensor");
#endif // HIVIEWDFX_HITRACE_ENABLE
    ret = sensorServer_->DisableSensor({sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId,
        sensorDesc.location});
    WriteHiSysIPCEvent(ISensorServiceIpcCode::COMMAND_DISABLE_SENSOR, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    if (ret == ERR_OK) {
        DeleteSensorInfoItem(sensorDesc);
    }
    return ret;
}

std::vector<Sensor> SensorServiceClient::GetSensorList()
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return {};
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    if (sensorList_.empty()) {
        SEN_HILOGE("sensorList_ cannot be empty");
    }
    return sensorList_;
}

int32_t SensorServiceClient::GetSensorListByDevice(int32_t deviceId, std::vector<Sensor> &singleDevSensors)
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return ret;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    for (const auto& sensor : sensorList_) {
        if (sensor.GetDeviceId() == deviceId) {
            SEN_HILOGD("sensor.GetDeviceId():%{public}d, deviceIndex:%{public}d", sensor.GetDeviceId(), deviceId);
            singleDevSensors.push_back(sensor);
        }
    }
    if (singleDevSensors.empty()) {
        singleDevSensors = GetSensorListByDevice(deviceId);
        if (singleDevSensors.empty()) {
            SEN_HILOGW("singleDevSensors is empty");
        }
    }
    return ERR_OK;
}

std::vector<Sensor> SensorServiceClient::GetSensorListByDevice(int32_t deviceId)
{
    CALL_LOG_ENTER;
    std::vector<Sensor> singleDevSensors;
    int32_t ret = sensorServer_->GetSensorListByDevice(deviceId, singleDevSensors);
    if (ret != ERR_OK) {
        SEN_HILOGE("GetSensorListByDevice failed, ret:%{public}d", ret);
        return {};
    }
    if (singleDevSensors.empty()) {
        SEN_HILOGE("GetSensorListByDevice failed,singleDevSensors cannot be empty");
        return {};
    }
    for (const auto& newSensor : singleDevSensors) {
        bool found = false;
        for (const auto& oldSensor : sensorList_) {
            if (oldSensor.GetDeviceId() == newSensor.GetDeviceId() &&
                oldSensor.GetSensorId() == newSensor.GetSensorId() &&
                oldSensor.GetSensorTypeId() == newSensor.GetSensorTypeId()) {
                found = true;
                break;
            }
        }
        if (!found) {
            SEN_HILOGD("Sensor not found in sensorList_");
            sensorList_.push_back(newSensor);
        }
    }
    return singleDevSensors;
}

int32_t SensorServiceClient::GetLocalDeviceId(int32_t& deviceId)
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return ret;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    for (const auto& sensor : sensorList_) {
        if (sensor.GetLocation() == LOCAL_DEVICE) {
            SEN_HILOGD("local deviceId is:%{public}d", sensor.GetDeviceId());
            deviceId = sensor.GetDeviceId();
            return ERR_OK;
        }
    }
    SEN_HILOGE("Get local deviceId failed, sensor list size: %{public}zu", sensorList_.size());
    return SERVICE_EXCEPTION;
}

int32_t SensorServiceClient::TransferDataChannel(sptr<SensorDataChannel> sensorDataChannel)
{
    CALL_LOG_ENTER;
    CHKPR(sensorDataChannel, INVALID_POINTER);
    {
        std::lock_guard<std::mutex> channelLock(channelMutex_);
        dataChannel_ = sensorDataChannel;
    }
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return ret;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPR(sensorServer_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "TransferDataChannel");
#endif // HIVIEWDFX_HITRACE_ENABLE
    CHKPR(sensorClientStub_, INVALID_POINTER);
    auto remoteObject = sensorClientStub_->AsObject();
    CHKPR(remoteObject, INVALID_POINTER);
    ret = sensorServer_->TransferDataChannel(sensorDataChannel->GetSendDataFd(), remoteObject);
    WriteHiSysIPCEvent(ISensorServiceIpcCode::COMMAND_TRANSFER_DATA_CHANNEL, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    return ret;
}

int32_t SensorServiceClient::DestroyDataChannel()
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return ret;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPR(sensorServer_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "DestroyDataChannel");
#endif // HIVIEWDFX_HITRACE_ENABLE
    CHKPR(sensorClientStub_, INVALID_POINTER);
    auto remoteObject = sensorClientStub_->AsObject();
    CHKPR(remoteObject, INVALID_POINTER);
    ret = sensorServer_->DestroySensorChannel(remoteObject);
    if (ret != NO_ERROR) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION", HiSysEvent::EventType::FAULT, "PKG_NAME",
            "DestroySensorChannel", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
    }
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    return ret;
}

void SensorServiceClient::ReenableSensor()
{ // LCOV_EXCL_START
    CALL_LOG_ENTER;
    {
        std::lock_guard<std::mutex> clientLock(clientMutex_);
        std::lock_guard<std::mutex> mapLock(mapMutex_);
        for (const auto &it : sensorInfoMap_) {
            if (sensorServer_ != nullptr) {
                int32_t ret = sensorServer_->EnableSensor({it.first.deviceId, it.first.sensorType, it.first.sensorId,
                    it.first.location}, it.second.GetSamplingPeriodNs(), it.second.GetMaxReportDelayNs());
                WriteHiSysIPCEvent(ISensorServiceIpcCode::COMMAND_ENABLE_SENSOR, ret);
            }
        }
    }
    if (!isConnected_) {
        SEN_HILOGD("Previous socket channel status is false, not need retry creat socket channel");
        return;
    }
    Disconnect();
    CreateSocketChannel();
} // LCOV_EXCL_STOP

int32_t SensorServiceClient::CreateClientRemoteObject()
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t SensorServiceClient::TransferClientRemoteObject()
{
    CALL_LOG_ENTER;
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "TransferClientRemoteObject");
#endif // HIVIEWDFX_HITRACE_ENABLE
    CHKPR(sensorClientStub_, INVALID_POINTER);
    auto remoteObject = sensorClientStub_->AsObject();
    CHKPR(remoteObject, INVALID_POINTER);
    int32_t ret = sensorServer_->TransferClientRemoteObject(remoteObject);
    SEN_HILOGI("TransferClientRemoteObject ret:%{public}d", ret);
    WriteHiSysIPCEvent(ISensorServiceIpcCode::COMMAND_TRANSFER_CLIENT_REMOTE_OBJECT, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    SEN_HILOGI("Done");
    return ret;
}

int32_t SensorServiceClient::DestroyClientRemoteObject()
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPR(sensorServer_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "TransferClientRemoteObject");
#endif // HIVIEWDFX_HITRACE_ENABLE
    CHKPR(sensorClientStub_, INVALID_POINTER);
    auto remoteObject = sensorClientStub_->AsObject();
    CHKPR(remoteObject, INVALID_POINTER);
    ret = sensorServer_->DestroyClientRemoteObject(remoteObject);
    SEN_HILOGI("DestroyClientRemoteObject ret:%{public}d", ret);
    WriteHiSysIPCEvent(ISensorServiceIpcCode::COMMAND_DESTROY_CLIENT_REMOTE_OBJECT, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    SEN_HILOGI("Done");
    return ret;
}

void SensorServiceClient::WriteHiSysIPCEvent(ISensorServiceIpcCode code, int32_t ret)
{ // LCOV_EXCL_START
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
    if (ret != NO_ERROR) {
        switch (code) {
            case ISensorServiceIpcCode::COMMAND_ENABLE_SENSOR:
                HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION", HiSysEvent::EventType::FAULT,
                    "PKG_NAME", "EnableSensor", "ERROR_CODE", ret);
                break;
            case ISensorServiceIpcCode::COMMAND_DISABLE_SENSOR:
                HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION", HiSysEvent::EventType::FAULT,
                    "PKG_NAME", "DisableSensor", "ERROR_CODE", ret);
                break;
            case ISensorServiceIpcCode::COMMAND_GET_SENSOR_LIST:
                HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION", HiSysEvent::EventType::FAULT,
                    "PKG_NAME", "GetSensorList", "ERROR_CODE", ret);
                break;
            case ISensorServiceIpcCode::COMMAND_TRANSFER_DATA_CHANNEL:
                HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION", HiSysEvent::EventType::FAULT,
                    "PKG_NAME", "TransferDataChannel", "ERROR_CODE", ret);
                break;
            case ISensorServiceIpcCode::COMMAND_CREATE_SOCKET_CHANNEL:
                HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION", HiSysEvent::EventType::FAULT,
                    "PKG_NAME", "CreateSocketChannel", "ERROR_CODE", ret);
                break;
            case ISensorServiceIpcCode::COMMAND_DESTROY_SOCKET_CHANNEL:
                HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION", HiSysEvent::EventType::FAULT,
                    "PKG_NAME", "DestroySocketChannel", "ERROR_CODE", ret);
                break;
            case ISensorServiceIpcCode::COMMAND_ENABLE_ACTIVE_INFO_C_B:
                HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION", HiSysEvent::EventType::FAULT,
                    "PKG_NAME", "EnableActiveInfoCB", "ERROR_CODE", ret);
                break;
            case ISensorServiceIpcCode::COMMAND_DISABLE_ACTIVE_INFO_C_B:
                HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION", HiSysEvent::EventType::FAULT,
                    "PKG_NAME", "DisableActiveInfoCB", "ERROR_CODE", ret);
                break;
            case ISensorServiceIpcCode::COMMAND_RESET_SENSORS:
                HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION", HiSysEvent::EventType::FAULT,
                    "PKG_NAME", "ResetSensors", "ERROR_CODE", ret);
                break;
            default:
                break;
        }
    }
    WriteHiSysIPCEventSplit(code, ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
} // LCOV_EXCL_STOP

void SensorServiceClient::WriteHiSysIPCEventSplit(ISensorServiceIpcCode code, int32_t ret)
{ // LCOV_EXCL_START
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
    if (ret != NO_ERROR) {
        switch (code) {
            case ISensorServiceIpcCode::COMMAND_SET_DEVICE_STATUS:
                HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION", HiSysEvent::EventType::FAULT,
                    "PKG_NAME", "SetDeviceStatus", "ERROR_CODE", ret);
                break;
            case ISensorServiceIpcCode::COMMAND_TRANSFER_CLIENT_REMOTE_OBJECT:
                HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION", HiSysEvent::EventType::FAULT,
                    "PKG_NAME", "TransferClientRemoteObject", "ERROR_CODE", ret);
                break;
            case ISensorServiceIpcCode::COMMAND_DESTROY_CLIENT_REMOTE_OBJECT:
                HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION", HiSysEvent::EventType::FAULT,
                    "PKG_NAME", "DestroyClientRemoteObject", "ERROR_CODE", ret);
                break;
            default:
                SEN_HILOGW("Code does not exist, code:%{public}d", static_cast<int32_t>(code));
                break;
        }
    }
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
} // LCOV_EXCL_STOP

void SensorServiceClient::ProcessDeathObserver(const wptr<IRemoteObject> &object)
{ // LCOV_EXCL_START
    CALL_LOG_ENTER;
    {
        std::lock_guard<std::mutex> channelLock(channelMutex_);
        if (dataChannel_ == nullptr) {
            SEN_HILOGI("dataChannel_ is nullptr");
            {
                std::lock_guard<std::mutex> clientLock(clientMutex_);
                sensorServer_ = nullptr;
            }
            if (InitServiceClient() != ERR_OK) {
                SEN_HILOGE("InitServiceClient failed");
                return;
            }
        } else {
            SEN_HILOGI("dataChannel_ is not nullptr");
            dataChannel_->DestroySensorDataChannel();
            int32_t ret = dataChannel_->RestoreSensorDataChannel();
            if (ret == ERR_OK) {
                SENSOR_AGENT_IMPL->SetIsChannelCreated(true);
            }
            {
                std::lock_guard<std::mutex> clientLock(clientMutex_);
                sensorServer_ = nullptr;
            }
            if (InitServiceClient() != ERR_OK) {
                SEN_HILOGE("InitServiceClient failed");
                dataChannel_->DestroySensorDataChannel();
                SENSOR_AGENT_IMPL->SetIsChannelCreated(false);
                return;
            }
            std::lock_guard<std::mutex> clientLock(clientMutex_);
            if (sensorServer_ != nullptr && sensorClientStub_ != nullptr) {
                auto remoteObject = sensorClientStub_->AsObject();
                if (remoteObject != nullptr) {
                    ret = sensorServer_->TransferDataChannel(dataChannel_->GetSendDataFd(), remoteObject);
                    WriteHiSysIPCEvent(ISensorServiceIpcCode::COMMAND_TRANSFER_DATA_CHANNEL, ret);
                }
            }
        }
    }
    ReenableSensor();
} // LCOV_EXCL_STOP

void SensorServiceClient::UpdateSensorInfoMap(const SensorDescription &sensorDesc, int64_t samplingPeriod,
    int64_t maxReportDelay)
{
    CALL_LOG_ENTER;
    SensorBasicInfo sensorInfo;
    sensorInfo.SetSamplingPeriodNs(samplingPeriod);
    sensorInfo.SetMaxReportDelayNs(maxReportDelay);
    sensorInfo.SetSensorState(true);
    std::lock_guard<std::mutex> mapLock(mapMutex_);
    sensorInfoMap_[sensorDesc] = sensorInfo;
    return;
}

void SensorServiceClient::DeleteSensorInfoItem(const SensorDescription &sensorDesc)
{
    SEN_HILOGD("In");
    std::lock_guard<std::mutex> mapLock(mapMutex_);
    auto it = sensorInfoMap_.find(sensorDesc);
    if (it != sensorInfoMap_.end()) {
        sensorInfoMap_.erase(it);
    }
    SEN_HILOGI("Done");
    return;
}

int32_t SensorServiceClient::SuspendSensors(int32_t pid)
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return ret;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPR(sensorServer_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "SuspendSensors");
#endif // HIVIEWDFX_HITRACE_ENABLE
    ret = sensorServer_->SuspendSensors(pid);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    return ret;
}

int32_t SensorServiceClient::ResumeSensors(int32_t pid)
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return ret;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPR(sensorServer_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "ResumeSensors");
#endif // HIVIEWDFX_HITRACE_ENABLE
    ret = sensorServer_->ResumeSensors(pid);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    return ret;
}

int32_t SensorServiceClient::GetActiveInfoList(int32_t pid, std::vector<ActiveInfo> &activeInfoList)
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return ret;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPR(sensorServer_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "GetActiveInfoList");
#endif // HIVIEWDFX_HITRACE_ENABLE
    ret = sensorServer_->GetActiveInfoList(pid, activeInfoList);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    return ret;
}

int32_t SensorServiceClient::Register(SensorActiveInfoCB callback, sptr<SensorDataChannel> sensorDataChannel)
{
    CALL_LOG_ENTER;
    if (!isConnected_) {
        CHKPR(sensorDataChannel, INVALID_POINTER);
        {
            std::lock_guard<std::mutex> channelLock(channelMutex_);
            dataChannel_ = sensorDataChannel;
        }
        int32_t ret = CreateSocketChannel();
        if (ret != ERR_OK) {
            SEN_HILOGE("Register sensor active info callback failed, ret:%{public}d", ret);
            return ret;
        }
    }
    std::lock_guard<std::mutex> activeInfoCBLock(activeInfoCBMutex_);
    activeInfoCBSet_.insert(callback);
    return ERR_OK;
}

int32_t SensorServiceClient::Unregister(SensorActiveInfoCB callback)
{
    CALL_LOG_ENTER;
    {
        std::lock_guard<std::mutex> activeInfoCBLock(activeInfoCBMutex_);
        activeInfoCBSet_.erase(callback);
        if (!activeInfoCBSet_.empty()) {
            return ERR_OK;
        }
    }
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return ret;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPR(sensorServer_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "DisableActiveInfoCB");
#endif // HIVIEWDFX_HITRACE_ENABLE
    ret = sensorServer_->DisableActiveInfoCB();
    WriteHiSysIPCEvent(ISensorServiceIpcCode::COMMAND_DISABLE_ACTIVE_INFO_C_B, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    if (ret != ERR_OK) {
        SEN_HILOGE("Disable active info callback failed, ret:%{public}d", ret);
        return ret;
    }
    Disconnect();
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "DestroySocketChannel");
#endif // HIVIEWDFX_HITRACE_ENABLE
    CHKPR(sensorClientStub_, INVALID_POINTER);
    auto remoteObject = sensorClientStub_->AsObject();
    CHKPR(remoteObject, INVALID_POINTER);
    ret = sensorServer_->DestroySocketChannel(remoteObject);
    WriteHiSysIPCEvent(ISensorServiceIpcCode::COMMAND_DESTROY_SOCKET_CHANNEL, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    if (ret != ERR_OK) {
        SEN_HILOGE("Destroy socket channel failed, ret:%{public}d", ret);
        return ret;
    }
    isConnected_ = false;
    return ERR_OK;
}

int32_t SensorServiceClient::ResetSensors()
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return ret;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPR(sensorServer_, ERROR);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "ResetSensors");
#endif // HIVIEWDFX_HITRACE_ENABLE
    ret = sensorServer_->ResetSensors();
    WriteHiSysIPCEvent(ISensorServiceIpcCode::COMMAND_RESET_SENSORS, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    return ret;
}

void SensorServiceClient::ReceiveMessage(const char *buf, size_t size)
{ // LCOV_EXCL_START
    CHKPV(buf);
    if (size == 0 || size > MAX_PACKET_BUF_SIZE) {
        SEN_HILOGE("Invalid input param size. size:%{public}zu", size);
        return;
    }
    if (!circBuf_.Write(buf, size)) {
        SEN_HILOGE("Write data failed. size:%{public}zu", size);
    }
    OnReadPackets(circBuf_, [this] (NetPacket &pkt) { this->HandleNetPacke(pkt); });
} // LCOV_EXCL_STOP

void SensorServiceClient::HandleNetPacke(NetPacket &pkt)
{ // LCOV_EXCL_START
    auto id = pkt.GetMsgId();
    if (id != MessageId::ACTIVE_INFO) {
        SEN_HILOGE("NetPacke message id is not ACTIVE_INFO");
        return;
    }
    SensorActiveInfo sensorActiveInfo;
    pkt >> sensorActiveInfo.pid >> sensorActiveInfo.sensorId >> sensorActiveInfo.samplingPeriodNs >>
        sensorActiveInfo.maxReportDelayNs;
    if (pkt.ChkRWError()) {
        SEN_HILOGE("Packet read type failed");
        return;
    }
    std::lock_guard<std::mutex> activeInfoCBLock(activeInfoCBMutex_);
    for (auto callback : activeInfoCBSet_) {
        if (callback != nullptr) {
            callback(sensorActiveInfo);
        }
    }
} // LCOV_EXCL_STOP

void SensorServiceClient::Disconnect()
{
    CALL_LOG_ENTER;
    int32_t fd = GetFd();
    if (fd < 0) {
        return;
    }
    {
        std::lock_guard<std::mutex> channelLock(channelMutex_);
        if (dataChannel_ != nullptr) {
            int32_t ret = dataChannel_->DelFdListener(fd);
            if (ret != ERR_OK) {
                SEN_HILOGE("Delete fd listener failed, fd:%{public}d, ret:%{public}d", fd, ret);
            }
        }
    }
    Close();
}

int32_t SensorServiceClient::CreateSocketChannel()
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return ret;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPR(sensorServer_, ERROR);
    int32_t clientFd = -1;
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "CreateSocketChannel");
#endif // HIVIEWDFX_HITRACE_ENABLE
    CHKPR(sensorClientStub_, INVALID_POINTER);
    ret = sensorServer_->CreateSocketChannel(sensorClientStub_->AsObject(), clientFd);
    WriteHiSysIPCEvent(ISensorServiceIpcCode::COMMAND_CREATE_SOCKET_CHANNEL, ret);
    fdsan_exchange_owner_tag(clientFd, 0, TAG);
    FinishTrace(HITRACE_TAG_SENSORS);
    if (ret != ERR_OK || clientFd < 0) {
        Close();
        SEN_HILOGE("Create socket channel failed, ret:%{public}d", ret);
        return ret;
    }
    fd_ = clientFd;
    {
        std::lock_guard<std::mutex> channelLock(channelMutex_);
        if (dataChannel_->AddFdListener(GetFd(),
            [this] (const char *buf, size_t size) { this->ReceiveMessage(buf, size); },
            [this] { this->Disconnect(); })!= ERR_OK) {
            Close();
            SEN_HILOGE("Add fd listener failed, fd:%{public}d", GetFd());
            return ERROR;
        }
    }
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "EnableActiveInfoCB");
#endif // HIVIEWDFX_HITRACE_ENABLE
    ret = sensorServer_->EnableActiveInfoCB();
    WriteHiSysIPCEvent(ISensorServiceIpcCode::COMMAND_ENABLE_ACTIVE_INFO_C_B, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
    if (ret != ERR_OK) {
        SEN_HILOGE("Enable active info callback failed, ret:%{public}d", ret);
        Disconnect();
        return ret;
    }
    isConnected_ = true;
    return ERR_OK;
}

void SensorServiceClient::SetDeviceStatus(uint32_t deviceStatus)
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    CHKPV(sensorServer_);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_SENSORS, "SetDeviceStatus");
#endif // HIVIEWDFX_HITRACE_ENABLE
    ret = sensorServer_->SetDeviceStatus(deviceStatus);
    if (ret != ERR_OK) {
        SEN_HILOGE("SetDeviceStatus failed, ret:%{public}d", ret);
    }
    WriteHiSysIPCEvent(ISensorServiceIpcCode::COMMAND_SET_DEVICE_STATUS, ret);
#ifdef HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_SENSORS);
#endif // HIVIEWDFX_HITRACE_ENABLE
}

bool SensorServiceClient::EraseCacheSensorList(const SensorPlugData &info)
{ // LCOV_EXCL_START
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    if (sensorList_.empty()) {
        SEN_HILOGE("sensorList_ cannot be empty");
        return false;
    }
    auto it = std::find_if(sensorList_.begin(), sensorList_.end(), [&](const Sensor& sensor) {
        return sensor.GetDeviceId() == info.deviceId &&
            sensor.GetSensorTypeId() == info.sensorTypeId &&
            sensor.GetSensorId() == info.sensorId;
    });
    if (it != sensorList_.end()) {
        sensorList_.erase(it);
        return true;
    }
    SEN_HILOGD("sensorList_ cannot find the sensor");
    return true;
} // LCOV_EXCL_STOP
} // namespace Sensors
} // namespace OHOS
