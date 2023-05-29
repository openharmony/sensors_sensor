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

#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "death_recipient_template.h"
#include "hisysevent.h"
#include "hitrace_meter.h"
#include "ipc_skeleton.h"
#include "sensor_service_proxy.h"
#include "sensors_errors.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "SensorServiceClient" };
constexpr int32_t GET_SERVICE_MAX_COUNT = 30;
constexpr uint32_t WAIT_MS = 200;
}  // namespace

SensorServiceClient::~SensorServiceClient()
{
    Disconnect();
}

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
            SEN_HILOGD("get service success, retry:%{public}d", retry);
            serviceDeathObserver_ = new (std::nothrow) DeathRecipientTemplate(*const_cast<SensorServiceClient *>(this));
            if (serviceDeathObserver_ != nullptr) {
                sensorServer_->AsObject()->AddDeathRecipient(serviceDeathObserver_);
            }
            sensorList_ = sensorServer_->GetSensorList();
            return ERR_OK;
        }
        SEN_HILOGW("get service failed, retry:%{public}d", retry);
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_MS));
        retry++;
    }
    HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::SENSOR, "SERVICE_EXCEPTION",
        HiSysEvent::EventType::FAULT, "PKG_NAME", "InitServiceClient", "ERROR_CODE", SENSOR_NATIVE_GET_SERVICE_ERR);
    SEN_HILOGE("get service failed");
    return SENSOR_NATIVE_GET_SERVICE_ERR;
}

bool SensorServiceClient::IsValid(int32_t sensorId)
{
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return false;
    }
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

int32_t SensorServiceClient::EnableSensor(int32_t sensorId, int64_t samplingPeriod, int64_t maxReportDelay)
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return ret;
    }
    CHKPR(sensorServer_, ERROR);
    StartTrace(HITRACE_TAG_SENSORS, "EnableSensor");
    ret = sensorServer_->EnableSensor(sensorId, samplingPeriod, maxReportDelay);
    FinishTrace(HITRACE_TAG_SENSORS);
    if (ret == ERR_OK) {
        UpdateSensorInfoMap(sensorId, samplingPeriod, maxReportDelay);
    }
    return ret;
}

int32_t SensorServiceClient::DisableSensor(int32_t sensorId)
{
    CALL_LOG_ENTER;
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return ret;
    }
    CHKPR(sensorServer_, ERROR);
    StartTrace(HITRACE_TAG_SENSORS, "DisableSensor");
    ret = sensorServer_->DisableSensor(sensorId);
    FinishTrace(HITRACE_TAG_SENSORS);
    if (ret == ERR_OK) {
        DeleteSensorInfoItem(sensorId);
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
        SEN_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
        return ret;
    }
    CHKPR(sensorServer_, ERROR);
    StartTrace(HITRACE_TAG_SENSORS, "TransferDataChannel");
    ret = sensorServer_->TransferDataChannel(sensorDataChannel, sensorClientStub_);
    FinishTrace(HITRACE_TAG_SENSORS);
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
    CHKPR(sensorServer_, ERROR);
    StartTrace(HITRACE_TAG_SENSORS, "DestroyDataChannel");
    ret = sensorServer_->DestroySensorChannel(sensorClientStub_);
    FinishTrace(HITRACE_TAG_SENSORS);
    return ret;
}

void SensorServiceClient::ProcessDeathObserver(const wptr<IRemoteObject> &object)
{
    CALL_LOG_ENTER;
    (void)object;
    CHKPV(dataChannel_);
    // STEP1 : Destroy previous data channel
    dataChannel_->DestroySensorDataChannel();
    // STEP2 : Restore data channel
    dataChannel_->RestoreSensorDataChannel();
    // STEP3 : Clear sensorlist and sensorServer_
    sensorList_.clear();
    sensorServer_ = nullptr;
    // STEP4 : ReGet sensors  3601 service
    int32_t ret = InitServiceClient();
    if (ret != ERR_OK) {
        SEN_HILOGE("InitServiceClient failed, ret:%{public}d", ret);
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

    if (!isConnected_) {
        SEN_HILOGD("Previous socket channel status is false, not need retry creat socket channel");
        return;
    }
    Disconnect();
    CreateSocketChannel();
}

void SensorServiceClient::UpdateSensorInfoMap(int32_t sensorId, int64_t samplingPeriod, int64_t maxReportDelay)
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

void SensorServiceClient::DeleteSensorInfoItem(int32_t sensorId)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> mapLock(mapMutex_);
    auto it = sensorInfoMap_.find(sensorId);
    if (it != sensorInfoMap_.end()) {
        sensorInfoMap_.erase(it);
    }
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
    CHKPR(sensorServer_, ERROR);
    StartTrace(HITRACE_TAG_SENSORS, "SuspendSensors");
    ret = sensorServer_->SuspendSensors(pid);
    FinishTrace(HITRACE_TAG_SENSORS);
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
    CHKPR(sensorServer_, ERROR);
    StartTrace(HITRACE_TAG_SENSORS, "ResumeSensors");
    ret = sensorServer_->ResumeSensors(pid);
    FinishTrace(HITRACE_TAG_SENSORS);
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
    CHKPR(sensorServer_, ERROR);
    StartTrace(HITRACE_TAG_SENSORS, "GetActiveInfoList");
    ret = sensorServer_->GetActiveInfoList(pid, activeInfoList);
    FinishTrace(HITRACE_TAG_SENSORS);
    return ret;
}

int32_t SensorServiceClient::Register(SensorActiveInfoCB callback, sptr<SensorDataChannel> sensorDataChannel)
{
    CALL_LOG_ENTER;
    if (!isConnected_) {
        CHKPR(sensorDataChannel, INVALID_POINTER);
        dataChannel_ = sensorDataChannel;
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
    CHKPR(sensorServer_, ERROR);
    StartTrace(HITRACE_TAG_SENSORS, "DisableActiveInfoCB");
    ret = sensorServer_->DisableActiveInfoCB();
    FinishTrace(HITRACE_TAG_SENSORS);
    if (ret != ERR_OK) {
        SEN_HILOGE("Disable active info callback failed, ret:%{public}d", ret);
        return ret;
    }
    Disconnect();
    StartTrace(HITRACE_TAG_SENSORS, "DestroySocketChannel");
    ret = sensorServer_->DestroySocketChannel(sensorClientStub_);
    FinishTrace(HITRACE_TAG_SENSORS);
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
    CHKPR(sensorServer_, ERROR);
    StartTrace(HITRACE_TAG_SENSORS, "ResetSensors");
    ret = sensorServer_->ResetSensors();
    FinishTrace(HITRACE_TAG_SENSORS);
    return ret;
}

void SensorServiceClient::ReceiveMessage(const char *buf, size_t size)
{
    CHKPV(buf);
    if (size == 0 || size > MAX_PACKET_BUF_SIZE) {
        SEN_HILOGE("Invalid input param size. size:%{public}zu", size);
        return;
    }
    if (!circBuf_.Write(buf, size)) {
        SEN_HILOGE("Write data failed. size:%{public}zu", size);
    }
    OnReadPackets(circBuf_, std::bind(&SensorServiceClient::HandleNetPacke, this, std::placeholders::_1));
}

void SensorServiceClient::HandleNetPacke(NetPacket &pkt)
{
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
}

void SensorServiceClient::Disconnect()
{
    CALL_LOG_ENTER;
    if (fd_ < 0) {
        return;
    }
    CHKPV(dataChannel_);
    int32_t ret = dataChannel_->DelFdListener(fd_);
    if (ret != ERR_OK) {
        SEN_HILOGE("Delete fd listener failed, fd:%{public}d, ret:%{public}d", fd_, ret);
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
    CHKPR(sensorServer_, ERROR);
    int32_t clientFd = -1;
    StartTrace(HITRACE_TAG_SENSORS, "CreateSocketChannel");
    ret = sensorServer_->CreateSocketChannel(sensorClientStub_, clientFd);
    FinishTrace(HITRACE_TAG_SENSORS);
    if (ret != ERR_OK || clientFd < 0) {
        Close();
        SEN_HILOGE("Create socket channel failed, ret:%{public}d", ret);
        return ret;
    }
    fd_ = clientFd;
    if (dataChannel_->AddFdListener(fd_,
        std::bind(&SensorServiceClient::ReceiveMessage, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&SensorServiceClient::Disconnect, this)) != ERR_OK) {
        Close();
        SEN_HILOGE("Add fd listener failed, fd:%{public}d", fd_);
        return ERROR;
    }
    StartTrace(HITRACE_TAG_SENSORS, "EnableActiveInfoCB");
    ret = sensorServer_->EnableActiveInfoCB();
    FinishTrace(HITRACE_TAG_SENSORS);
    if (ret != ERR_OK) {
        SEN_HILOGE("Enable active info callback failed, ret:%{public}d", ret);
        Disconnect();
        return ret;
    }
    isConnected_ = true;
    return ERR_OK;
}
}  // namespace Sensors
}  // namespace OHOS
