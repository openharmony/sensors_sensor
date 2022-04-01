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

#include "sensor_service.h"

#include <cinttypes>
#include <sys/socket.h>
#include <unistd.h>

#include "hisysevent.h"
#include "iservice_registry.h"
#include "permission_util.h"
#include "securec.h"
#include "sensor.h"
#include "sensor_dump.h"
#include "sensors_errors.h"
#include "sensors_log_domain.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SensorsLogDomain::SENSOR_SERVICE, "SensorService" };
constexpr uint32_t INVALID_SENSOR_ID = -1;
constexpr int32_t MAX_DMUP_PARAM = 2;
constexpr int32_t INVALID_PID = -1;
constexpr int64_t MAX_EVENT_COUNT = 1000;
constexpr uint32_t REPORT_STATUS_LEN = 20;
enum {
    FLUSH = 0,
    SET_MODE,
    RESERVED,
};
}  // namespace

REGISTER_SYSTEM_ABILITY_BY_ID(SensorService, SENSOR_SERVICE_ABILITY_ID, true);

SensorService::SensorService(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(systemAbilityId, runOnCreate), state_(SensorServiceState::STATE_STOPPED)
{}

void SensorService::OnDump()
{
    SEN_HILOGI("OnDump");
}

void SensorService::OnStart()
{
    CALL_LOG_ENTER;
    if (state_ == SensorServiceState::STATE_RUNNING) {
        SEN_HILOGW("SensorService has already started");
        return;
    }
    if (!InitInterface()) {
        SEN_HILOGE("Init interface error");
        return;
    }
    if (!InitDataCallback()) {
        SEN_HILOGE("Init data callback error");
        return;
    }
    if (!InitSensorList()) {
        SEN_HILOGE("Init sensor list error");
        return;
    }
    sensorDataProcesser_ = new (std::nothrow) SensorDataProcesser(sensorMap_);
    if (sensorDataProcesser_ == nullptr) {
        SEN_HILOGE("failed, sensorDataProcesser_ cannot be null");
        return;
    }
    if (!InitSensorPolicy()) {
        SEN_HILOGE("Init sensor policy error");
    }

    if (!SystemAbility::Publish(this)) {
        SEN_HILOGE("publish SensorService error");
        return;
    }
    sensorManager_.InitSensorMap(sensorMap_, sensorDataProcesser_, reportDataCallback_);
    state_ = SensorServiceState::STATE_RUNNING;
}

bool SensorService::InitInterface()
{
    auto ret = sensorHdiConnection_.ConnectHdi();
    if (ret != ERR_OK) {
        SEN_HILOGE("connect hdi failed");
        return false;
    }
    return true;
}

bool SensorService::InitDataCallback()
{
    reportDataCallback_ = new (std::nothrow) ReportDataCallback();
    if (reportDataCallback_ == nullptr) {
        SEN_HILOGE("failed, reportDataCallback_ cannot be null");
        return false;
    }
    ZReportDataCb cb = &ReportDataCallback::ReportEventCallback;
    auto ret = sensorHdiConnection_.RegisteDataReport(cb, reportDataCallback_);
    if (ret != ERR_OK) {
        SEN_HILOGE("RegisterDataReport failed");
        return false;
    }
    return true;
}

bool SensorService::InitSensorList()
{
    std::lock_guard<std::mutex> sensorLock(sensorsMutex_);
    int32_t ret = sensorHdiConnection_.GetSensorList(sensors_);
    if (ret < 0) {
        SEN_HILOGE("GetSensorList is failed");
        return false;
    }
    {
        std::lock_guard<std::mutex> sensorMapLock(sensorMapMutex_);
        for (const auto &it : sensors_) {
            if (!(sensorMap_.insert(std::make_pair(it.GetSensorId(), it)).second)) {
                SEN_HILOGW("sensorMap_ Insert failed");
            }
        }
    }
    return true;
}

bool SensorService::InitSensorPolicy()
{
    return true;
}

void SensorService::OnStop()
{
    CALL_LOG_ENTER;
    if (state_ == SensorServiceState::STATE_STOPPED) {
        SEN_HILOGW("already stopped");
        return;
    }
    state_ = SensorServiceState::STATE_STOPPED;
    int32_t ret = sensorHdiConnection_.DestroyHdiConnection();
    if (ret != ERR_OK) {
        SEN_HILOGE("destroy hdi connect fail");
    }
}

void SensorService::ReportSensorSysEvent(uint32_t sensorId, bool enable)
{
    char uidChar[REPORT_STATUS_LEN] = {0};
    int32_t uid = this->GetCallingUid();
    std::string packageName("");
    sensorManager_.GetPackageNameFromUid(uid, packageName);
    int32_t ret = sprintf_s(uidChar, sizeof(uidChar) - 1, "%d", uid);
    if (ret < 0) {
        SEN_HILOGE("sprintf uidChar failed");
        return;
    }

    const int logLevel = 4;
    std::string message;
    if (enable) {
        // define in LogPower.java, 500 stand for enable sensor
        message.append("uid : ").append(std::to_string(uid)).append(" pkgName : ").append(packageName)
            .append(" type : ");
        HiSysEvent::Write(HiSysEvent::Domain::SENSORS, "EnableSensor", HiSysEvent::EventType::FAULT,
            "LEVEL", logLevel, "TAG", "DUBAI_TAG_HSENSOR_ENABLE", "MESSAGE", message);
    } else {
        // define in LogPower.java, 501 stand for disable sensor
        message.append("uid : ").append(std::to_string(uid)).append(" pkgName : ").append(packageName)
            .append(" type : ");
        HiSysEvent::Write(HiSysEvent::Domain::SENSORS, "DisableSensor", HiSysEvent::EventType::FAULT,
            "LEVEL", logLevel, "TAG", "DUBAI_TAG_HSENSOR_DISABLE", "MESSAGE", message);
    }

    SEN_HILOGI("end, packageName : %{public}s", packageName.c_str());
}

void SensorService::ReportOnChangeData(uint32_t sensorId)
{
    std::lock_guard<std::mutex> sensorMapLock(sensorMapMutex_);
    auto it = sensorMap_.find(sensorId);
    if (it == sensorMap_.end()) {
        SEN_HILOGE("sensorId is invalid");
        return;
    }
    if ((SENSOR_ON_CHANGE & it->second.GetFlags()) != SENSOR_ON_CHANGE) {
        SEN_HILOGW("it is not onchange data, no need to report");
        return;
    }
    struct SensorEvent event;
    auto ret = clientInfo_.GetStoreEvent(sensorId, event);
    if (ret != ERR_OK) {
        SEN_HILOGE("there is no data to be reported");
        return;
    }
    sptr<SensorBasicDataChannel> channel = clientInfo_.GetSensorChannelByPid(this->GetCallingPid());
    if (channel == nullptr) {
        SEN_HILOGE("there is no channel to be reported");
        return;
    }
    auto sendRet = channel->SendData(&event, sizeof(event));
    if (sendRet != ERR_OK) {
        SEN_HILOGE("send data failed");
        return;
    }
}

ErrCode SensorService::SaveSubscriber(uint32_t sensorId, int64_t samplingPeriodNs, int64_t maxReportDelayNs)
{
    auto ret = sensorManager_.SaveSubscriber(sensorId, this->GetCallingPid(), samplingPeriodNs, maxReportDelayNs);
    if (ret != ERR_OK) {
        SEN_HILOGE("SaveSubscriber failed");
        return ret;
    }
    sensorManager_.StartDataReportThread();

    if (!sensorManager_.SetBestSensorParams(sensorId, samplingPeriodNs, maxReportDelayNs)) {
        SEN_HILOGE("SetBestSensorParams failed");
        clientInfo_.RemoveSubscriber(sensorId, this->GetCallingPid());
        return ENABLE_SENSOR_ERR;
    }
    return ret;
}

ErrCode SensorService::EnableSensor(uint32_t sensorId, int64_t samplingPeriodNs, int64_t maxReportDelayNs)
{
    CALL_LOG_ENTER;
    if ((sensorId == INVALID_SENSOR_ID) ||
        ((samplingPeriodNs != 0L) && ((maxReportDelayNs / samplingPeriodNs) > MAX_EVENT_COUNT))) {
        SEN_HILOGE("sensorId is 0 or maxReportDelayNs exceeded the maximum value");
        return ERR_NO_INIT;
    }
    ReportSensorSysEvent(sensorId, true);
    std::lock_guard<std::mutex> serviceLock(serviceLock_);
    if (clientInfo_.GetSensorState(sensorId)) {
        SEN_HILOGW("sensor has been enabled already");
        auto ret = SaveSubscriber(sensorId, samplingPeriodNs, maxReportDelayNs);
        if (ret != ERR_OK) {
            SEN_HILOGE("SaveSubscriber failed");
            return ret;
        }
        uint32_t flag = sensorManager_.GetSensorFlag(sensorId);
        int32_t pid = this->GetCallingPid();
        ret = flushInfo_.FlushProcess(sensorId, flag, pid, true);
        if (ret != ERR_OK) {
            SEN_HILOGE("ret : %{public}d", ret);
        }
        ReportOnChangeData(sensorId);
        return ERR_OK;
    }
    auto ret = SaveSubscriber(sensorId, samplingPeriodNs, maxReportDelayNs);
    if (ret != ERR_OK) {
        SEN_HILOGE("SaveSubscriber failed");
        clientInfo_.RemoveSubscriber(sensorId, this->GetCallingPid());
        return ret;
    }

    ret = sensorHdiConnection_.EnableSensor(sensorId);
    if (ret != ERR_OK) {
        SEN_HILOGE("EnableSensor failed");
        clientInfo_.RemoveSubscriber(sensorId, this->GetCallingPid());
        return ENABLE_SENSOR_ERR;
    }

    return ret;
}

ErrCode SensorService::DisableSensor(uint32_t sensorId)
{
    CALL_LOG_ENTER;
    if (sensorId == INVALID_SENSOR_ID) {
        SEN_HILOGE("sensorId is invalid");
        return ERR_NO_INIT;
    }
    ReportSensorSysEvent(sensorId, false);
    std::lock_guard<std::mutex> serviceLock(serviceLock_);
    const int32_t clientPid = this->GetCallingPid();
    if (clientPid < 0) {
        SEN_HILOGE("clientPid is invalid, clientPid : %{public}d", clientPid);
        return CLIENT_PID_INVALID_ERR;
    }
    if (!clientInfo_.GetSensorState(sensorId)) {
        SEN_HILOGE("sensor should be enabled first");
        return DISABLE_SENSOR_ERR;
    }
    if (sensorManager_.IsOtherClientUsingSensor(sensorId, clientPid)) {
        SEN_HILOGW("other client is using this sensor now, cannot disable");
        return ERR_OK;
    }
    if (sensorHdiConnection_.DisableSensor(sensorId) != ERR_OK) {
        SEN_HILOGE("DisableSensor is failed");
        return DISABLE_SENSOR_ERR;
    }
    clientInfo_.DestroyCmd(this->GetCallingUid());
    clientInfo_.ClearDataQueue(sensorId);
    return sensorManager_.AfterDisableSensor(sensorId);
}

int32_t SensorService::GetSensorState(uint32_t sensorId)
{
    if (sensorId == INVALID_SENSOR_ID) {
        SEN_HILOGE("sensorId is 0");
        return ERR_NO_INIT;
    }
    auto state = clientInfo_.GetSensorState(sensorId);
    return static_cast<int32_t>(state);
}

ErrCode SensorService::RunCommand(uint32_t sensorId, uint32_t cmdType, uint32_t params)
{
    CALL_LOG_ENTER;
    if (sensorId == INVALID_SENSOR_ID || ((cmdType != FLUSH) && (cmdType != SET_MODE))) {
        SEN_HILOGE("sensorId or cmd is invalid");
        return ERR_NO_INIT;
    }
    std::lock_guard<std::mutex> serviceLock(serviceLock_);
    uint32_t flag = sensorManager_.GetSensorFlag(sensorId);
    if (cmdType == FLUSH) {
        int32_t pid = this->GetCallingPid();
        SEN_HILOGI("sensorId : %{public}u, flag : %{public}u", sensorId, flag);
        auto retFlush = flushInfo_.FlushProcess(sensorId, flag, pid, false);
        if (retFlush != ERR_OK) {
            SEN_HILOGE("ret : %{public}d", retFlush);
        }
        return retFlush;
    }
    if (sensorHdiConnection_.RunCommand(sensorId, cmdType, params) != ERR_OK) {
        SEN_HILOGE("RunCommand is failed");
        return RUN_COMMAND_ERR;
    }
    auto uid = this->GetCallingUid();
    clientInfo_.UpdateCmd(sensorId, uid, cmdType);
    return ERR_OK;
}

std::vector<Sensor> SensorService::GetSensorList()
{
    std::lock_guard<std::mutex> sensorLock(sensorsMutex_);
    int32_t ret = sensorHdiConnection_.GetSensorList(sensors_);
    if (ret < 0) {
        SEN_HILOGE("GetSensorList is failed");
        return sensors_;
    }
    for (const auto &it : sensors_) {
        std::lock_guard<std::mutex> sensorMapLock(sensorMapMutex_);
        sensorMap_.insert(std::make_pair(it.GetSensorId(), it));
    }
    return sensors_;
}

ErrCode SensorService::TransferDataChannel(const sptr<SensorBasicDataChannel> &sensorBasicDataChannel,
                                           const sptr<IRemoteObject> &sensorClient)
{
    if ((sensorBasicDataChannel == nullptr)) {
        SEN_HILOGE("sensorBasicDataChannel cannot be null");
        return ERR_NO_INIT;
    }
    auto pid = this->GetCallingPid();
    auto uid = this->GetCallingUid();
    auto callerToken = this->GetCallingTokenID();
    if (!clientInfo_.UpdateAppThreadInfo(pid, uid, callerToken)) {
        SEN_HILOGE("UpdateUid is failed");
        return UPDATE_UID_ERR;
    }
    if (!clientInfo_.UpdateSensorChannel(pid, sensorBasicDataChannel)) {
        SEN_HILOGE("UpdateSensorChannel is failed");
        return UPDATE_SENSOR_CHANNEL_ERR;
    }
    sensorBasicDataChannel->SetSensorStatus(true);
    RegisterClientDeathRecipient(sensorClient, pid);
    return ERR_OK;
}

ErrCode SensorService::DestroySensorChannel(sptr<IRemoteObject> sensorClient)
{
    CALL_LOG_ENTER;
    const int32_t clientPid = this->GetCallingPid();
    if (clientPid < 0) {
        SEN_HILOGE("clientPid is invalid, clientPid : %{public}d", clientPid);
        
        return CLIENT_PID_INVALID_ERR;
    }
    std::lock_guard<std::mutex> serviceLock(serviceLock_);
    bool destoryRet = clientInfo_.DestroySensorChannel(clientPid);
    if (!destoryRet) {
        SEN_HILOGE("DestroySensorChannel is failed");
        return DESTROY_SENSOR_CHANNEL_ERR;
    }
    clientInfo_.DestroyCmd(this->GetCallingUid());
    UnregisterClientDeathRecipient(sensorClient);
    return ERR_OK;
}

void SensorService::ProcessDeathObserver(const wptr<IRemoteObject> &object)
{
    CALL_LOG_ENTER;
    sptr<IRemoteObject> client = object.promote();
    if (client == nullptr) {
        SEN_HILOGE("client cannot be null");
        return;
    }
    int32_t pid = clientInfo_.FindClientPid(client);
    if (pid == INVALID_PID) {
        SEN_HILOGE("pid is -1");
        return;
    }
    SEN_HILOGI("pid is %d", pid);
    clientInfo_.DestroySensorChannel(pid);
    clientInfo_.DestroyClientPid(client);
    clientInfo_.DestroyCmd(this->GetCallingUid());
}

void SensorService::RegisterClientDeathRecipient(sptr<IRemoteObject> sensorClient, int32_t pid)
{
    CALL_LOG_ENTER;
    sptr<ISensorClient> client = iface_cast<ISensorClient>(sensorClient);
    clientDeathObserver_ = new (std::nothrow) DeathRecipientTemplate(*const_cast<SensorService *>(this));
    if (clientDeathObserver_ == nullptr) {
        SEN_HILOGE("clientDeathObserver_ cannot be null");
        return;
    }
    client->AsObject()->AddDeathRecipient(clientDeathObserver_);
    clientInfo_.SaveClientPid(sensorClient, pid);
}

void SensorService::UnregisterClientDeathRecipient(sptr<IRemoteObject> sensorClient)
{
    CALL_LOG_ENTER;
    sptr<ISensorClient> client = iface_cast<ISensorClient>(sensorClient);
    clientDeathObserver_ = new (std::nothrow) DeathRecipientTemplate(*const_cast<SensorService *>(this));
    if (clientDeathObserver_ == nullptr) {
        SEN_HILOGE("clientDeathObserver_ cannot be null");
        return;
    }
    client->AsObject()->RemoveDeathRecipient(clientDeathObserver_);
    clientInfo_.DestroyClientPid(sensorClient);
}

int32_t SensorService::Dump(int32_t fd, const std::vector<std::u16string> &args)
{
    CALL_LOG_ENTER;
    SensorDump &sensorDump = SensorDump::GetInstance();
    if ((args.empty()) || (args[0].size() != MAX_DMUP_PARAM)) {
        SEN_HILOGE("param cannot be empty or the length is not 2");
        dprintf(fd, "cmd param number is not equal to 2\n");
        sensorDump.DumpHelp(fd);
        return DUMP_PARAM_ERR;
    }
    bool helpRet = sensorDump.DumpSensorHelp(fd, args);
    bool listRet = sensorDump.DumpSensorList(fd, sensors_, args);
    bool channelRet = sensorDump.DumpSensorChannel(fd, clientInfo_, args);
    bool openRet = sensorDump.DumpOpeningSensor(fd, sensors_, clientInfo_, args);
    bool dataRet = sensorDump.DumpSensorData(fd, clientInfo_, args);
    bool total = helpRet + listRet + channelRet + openRet + dataRet;
    if (!total) {
        dprintf(fd, "cmd param is error\n");
        sensorDump.DumpHelp(fd);
        return DUMP_PARAM_ERR;
    }
    return ERR_OK;
}
}  // namespace Sensors
}  // namespace OHOS
