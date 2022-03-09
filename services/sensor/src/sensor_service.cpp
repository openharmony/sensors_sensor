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
    HiLog::Info(LABEL, "OnDump");
}

void SensorService::OnStart()
{
    HiLog::Debug(LABEL, "%{public}s begin", __func__);
    if (state_ == SensorServiceState::STATE_RUNNING) {
        HiLog::Warn(LABEL, "%{public}s SensorService has already started", __func__);
        return;
    }
    if (!InitInterface()) {
        HiLog::Error(LABEL, "%{public}s Init interface error", __func__);
        return;
    }
    if (!InitDataCallback()) {
        HiLog::Error(LABEL, "%{public}s Init data callback error", __func__);
        return;
    }
    if (!InitSensorList()) {
        HiLog::Error(LABEL, "%{public}s Init sensor list error", __func__);
        return;
    }
    sensorDataProcesser_ = new (std::nothrow) SensorDataProcesser(sensorMap_);
    if (sensorDataProcesser_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s failed, sensorDataProcesser_ cannot be null", __func__);
        return;
    }
    if (!InitSensorPolicy()) {
        HiLog::Error(LABEL, "%{public}s Init sensor policy error", __func__);
    }

    if (!SystemAbility::Publish(this)) {
        HiLog::Error(LABEL, "%{public}s publish SensorService error", __func__);
        return;
    }
    sensorManager_.InitSensorMap(sensorMap_, sensorDataProcesser_, reportDataCallback_);
    state_ = SensorServiceState::STATE_RUNNING;
}

bool SensorService::InitInterface()
{
    auto ret = sensorHdiConnection_.ConnectHdi();
    if (ret != ERR_OK) {
        HiLog::Error(LABEL, "%{public}s connect hdi failed", __func__);
        return false;
    }
    return true;
}

bool SensorService::InitDataCallback()
{
    reportDataCallback_ = new (std::nothrow) ReportDataCallback();
    if (reportDataCallback_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s failed, reportDataCallback_ cannot be null", __func__);
        return false;
    }
    ZReportDataCb cb = &ReportDataCallback::ZReportDataCallback;
    auto ret = sensorHdiConnection_.RegisteDataReport(cb, reportDataCallback_);
    if (ret != ERR_OK) {
        HiLog::Error(LABEL, "%{public}s RegisterDataReport failed", __func__);
        return false;
    }
    return true;
}

bool SensorService::InitSensorList()
{
    std::lock_guard<std::mutex> sensorLock(sensorsMutex_);
    int32_t ret = sensorHdiConnection_.GetSensorList(sensors_);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s GetSensorList failed", __func__);
        return false;
    }
    {
        std::lock_guard<std::mutex> sensorMapLock(sensorMapMutex_);
        for (const auto &it : sensors_) {
            sensorMap_.insert(std::make_pair(it.GetSensorId(), it));
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
    HiLog::Debug(LABEL, "%{public}s begin", __func__);
    if (state_ == SensorServiceState::STATE_STOPPED) {
        HiLog::Warn(LABEL, "%{public}s already stopped", __func__);
        return;
    }
    state_ = SensorServiceState::STATE_STOPPED;
    int32_t ret = sensorHdiConnection_.DestroyHdiConnection();
    if (ret != ERR_OK) {
        HiLog::Error(LABEL, "%{public}s destroy hdi connect fail", __func__);
    }
}

void SensorService::ReportSensorUsedInfo(uint32_t sensorId, bool enable)
{
    char uidChar[REPORT_STATUS_LEN];
    int32_t uid = this->GetCallingUid();
    std::string packageName("");
    sensorManager_.GetPackageNameFromUid(uid, packageName);
    int32_t ret = sprintf_s(uidChar, sizeof(uidChar), "%d", uid);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s sprintf uidChar failed", __func__);
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

    HiLog::Info(LABEL, "%{public}s end, packageName : %{public}s", __func__, packageName.c_str());
}

void SensorService::ReportOnChangeData(uint32_t sensorId)
{
    std::lock_guard<std::mutex> sensorMapLock(sensorMapMutex_);
    auto it = sensorMap_.find(sensorId);
    if (it == sensorMap_.end()) {
        HiLog::Error(LABEL, "%{public}s sensorId is invalid", __func__);
        return;
    }
    if ((SENSOR_ON_CHANGE & it->second.GetFlags()) != SENSOR_ON_CHANGE) {
        HiLog::Warn(LABEL, "%{public}s it is not onchange data, no need to report", __func__);
        return;
    }
    struct SensorEvent event;
    auto ret = clientInfo_.GetStoreEvent(sensorId, event);
    if (ret != ERR_OK) {
        HiLog::Error(LABEL, "%{public}s there is no data to be reported", __func__);
        return;
    }
    sptr<SensorBasicDataChannel> channel = clientInfo_.GetSensorChannelByPid(this->GetCallingPid());
    if (channel == nullptr) {
        HiLog::Error(LABEL, "%{public}s there is no channel to be reported", __func__);
        return;
    }
    auto sendRet = channel->SendData(&event, sizeof(event));
    if (sendRet != ERR_OK) {
        HiLog::Error(LABEL, "%{public}s send data failed", __func__);
        return;
    }
}

ErrCode SensorService::SaveSubscriber(uint32_t sensorId, int64_t samplingPeriodNs, int64_t maxReportDelayNs)
{
    auto ret = sensorManager_.SaveSubscriber(sensorId, this->GetCallingPid(), samplingPeriodNs, maxReportDelayNs);
    if (ret != ERR_OK) {
        HiLog::Error(LABEL, "%{public}s SaveSubscriber failed", __func__);
        return ret;
    }
    sensorManager_.StartDataReportThread();

    if (!sensorManager_.SetBestSensorParams(sensorId, samplingPeriodNs, maxReportDelayNs)) {
        HiLog::Error(LABEL, "%{public}s SetBestSensorParams failed", __func__);
        clientInfo_.RemoveSubscriber(sensorId, this->GetCallingPid());
        return ENABLE_SENSOR_ERR;
    }
    return ret;
}

ErrCode SensorService::EnableSensor(uint32_t sensorId, int64_t samplingPeriodNs, int64_t maxReportDelayNs)
{
    HiLog::Debug(LABEL, "%{public}s begin, sensorId : %{public}u, samplingPeriodNs : %{public}" 
        PRId64, __func__, sensorId, samplingPeriodNs);
    if ((sensorId == INVALID_SENSOR_ID) ||
        ((samplingPeriodNs != 0L) && ((maxReportDelayNs / samplingPeriodNs) > MAX_EVENT_COUNT))) {
        HiLog::Error(LABEL, "%{public}s sensorId is 0 or maxReportDelayNs exceeded the maximum value", __func__);
        return ERR_NO_INIT;
    }
    ReportSensorUsedInfo(sensorId, SENSOR_ENABLED);
    std::lock_guard<std::mutex> serviceLock(serviceLock_);
    if (clientInfo_.GetSensorState(sensorId) == SENSOR_ENABLED) {
        HiLog::Warn(LABEL, "%{public}s sensor has been enabled already", __func__);
        auto ret = SaveSubscriber(sensorId, samplingPeriodNs, maxReportDelayNs);
        if (ret != ERR_OK) {
            HiLog::Error(LABEL, "%{public}s SaveSubscriber failed", __func__);
            return ret;
        }
        uint32_t flag = sensorManager_.GetSensorFlag(sensorId);
        int32_t pid = this->GetCallingPid();
        ret = flushInfo_.FlushProcess(sensorId, flag, pid, true);
        if (ret != ERR_OK) {
            HiLog::Error(LABEL, "%{public}s ret : %{public}d", __func__, ret);
        }
        ReportOnChangeData(sensorId);
        return ERR_OK;
    }
    auto ret = SaveSubscriber(sensorId, samplingPeriodNs, maxReportDelayNs);
    if (ret != ERR_OK) {
        HiLog::Error(LABEL, "%{public}s SaveSubscriber failed", __func__);
        clientInfo_.RemoveSubscriber(sensorId, this->GetCallingPid());
        return ret;
    }

    ret = sensorHdiConnection_.EnableSensor(sensorId);
    if (ret != ERR_OK) {
        HiLog::Error(LABEL, "%{public}s EnableSensor failed", __func__);
        clientInfo_.RemoveSubscriber(sensorId, this->GetCallingPid());
        return ENABLE_SENSOR_ERR;
    }

    return ret;
}

ErrCode SensorService::DisableSensor(uint32_t sensorId)
{
    HiLog::Debug(LABEL, "%{public}s begin", __func__);
    if (sensorId == INVALID_SENSOR_ID) {
        HiLog::Error(LABEL, "%{public}s sensorId is invalid", __func__);
        return ERR_NO_INIT;
    }
    ReportSensorUsedInfo(sensorId, SENSOR_DISABLED);
    std::lock_guard<std::mutex> serviceLock(serviceLock_);
    const int32_t clientPid = this->GetCallingPid();
    if (clientPid < 0) {
        HiLog::Error(LABEL, "%{public}s clientPid is invalid, clientPid : %{public}d", __func__, clientPid);
        return CLIENT_PID_INVALID_ERR;
    }
    if (clientInfo_.GetSensorState(sensorId) != SENSOR_ENABLED) {
        HiLog::Error(LABEL, "%{public}s sensor should be enabled first", __func__);
        return DISABLE_SENSOR_ERR;
    }
    if (sensorManager_.IsOtherClientUsingSensor(sensorId, clientPid)) {
        HiLog::Warn(LABEL, "%{public}s other client is using this sensor now, cannot disable", __func__);
        return ERR_OK;
    }
    if (sensorHdiConnection_.DisableSensor(sensorId) != ERR_OK) {
        HiLog::Error(LABEL, "%{public}s DisableSensor failed", __func__);
        return DISABLE_SENSOR_ERR;
    }
    clientInfo_.DestroyCmd(this->GetCallingUid());
    clientInfo_.ClearDataQueue(sensorId);
    return sensorManager_.AfterDisableSensor(sensorId);
}

int32_t SensorService::GetSensorState(uint32_t sensorId)
{
    if (sensorId == INVALID_SENSOR_ID) {
        HiLog::Error(LABEL, "%{public}s sensorId is 0", __func__);
        return ERR_NO_INIT;
    }
    auto state = clientInfo_.GetSensorState(sensorId);
    return static_cast<int32_t>(state);
}

ErrCode SensorService::RunCommand(uint32_t sensorId, uint32_t cmdType, uint32_t params)
{
    HiLog::Debug(LABEL, "%{public}s begin", __func__);
    if (sensorId == INVALID_SENSOR_ID || ((cmdType != FLUSH) && (cmdType != SET_MODE))) {
        HiLog::Error(LABEL, "%{public}s sensorId or cmd is invalid", __func__);
        return ERR_NO_INIT;
    }
    std::lock_guard<std::mutex> serviceLock(serviceLock_);
    uint32_t flag = sensorManager_.GetSensorFlag(sensorId);
    if (cmdType == FLUSH) {
        int32_t pid = this->GetCallingPid();
        HiLog::Info(LABEL, "%{public}s sensorId : %{public}u, flag : %{public}u", __func__, sensorId, flag);
        auto retFlush = flushInfo_.FlushProcess(sensorId, flag, pid, false);
        if (retFlush != ERR_OK) {
            HiLog::Error(LABEL, "%{public}s ret : %{public}d", __func__, retFlush);
        }
        return retFlush;
    }
    if (sensorHdiConnection_.RunCommand(sensorId, cmdType, params) != ERR_OK) {
        HiLog::Error(LABEL, "%{public}s RunCommand failed", __func__);
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
        HiLog::Error(LABEL, "%{public}s GetSensorList failed", __func__);
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
        HiLog::Error(LABEL, "%{public}s sensorBasicDataChannel cannot be null", __func__);
        return ERR_NO_INIT;
    }
    auto pid = this->GetCallingPid();
    auto uid = this->GetCallingUid();
    auto callerToken = this->GetCallingTokenID();
    if (!clientInfo_.UpdateAppThreadInfo(pid, uid, callerToken)) {
        HiLog::Error(LABEL, "%{public}s UpdateUid failed", __func__);
        return UPDATE_UID_ERR;
    }
    if (!clientInfo_.UpdateSensorChannel(pid, sensorBasicDataChannel)) {
        HiLog::Error(LABEL, "%{public}s UpdateSensorChannel failed", __func__);
        return UPDATE_SENSOR_CHANNEL_ERR;
    }
    sensorBasicDataChannel->SetSensorStatus(true);
    RegisterClientDeathRecipient(sensorClient, pid);
    return ERR_OK;
}

ErrCode SensorService::DestroySensorChannel(sptr<IRemoteObject> sensorClient)
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    const int32_t clientPid = this->GetCallingPid();
    if (clientPid < 0) {
        HiLog::Error(LABEL, "%{public}s clientPid is invalid, clientPid : %{public}d", __func__, clientPid);
        return CLIENT_PID_INVALID_ERR;
    }
    std::lock_guard<std::mutex> serviceLock(serviceLock_);
    bool destoryRet = clientInfo_.DestroySensorChannel(clientPid);
    if (!destoryRet) {
        HiLog::Error(LABEL, "%{public}s DestroySensorChannel failed", __func__);
        return DESTROY_SENSOR_CHANNEL_ERR;
    }
    clientInfo_.DestroyCmd(this->GetCallingUid());
    UnregisterClientDeathRecipient(sensorClient);
    HiLog::Info(LABEL, "%{public}s end", __func__);
    return ERR_OK;
}

void SensorService::ProcessDeathObserver(const wptr<IRemoteObject> &object)
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    sptr<IRemoteObject> client = object.promote();
    if (client == nullptr) {
        HiLog::Error(LABEL, "%{public}s client cannot be null", __func__);
        return;
    }
    int32_t pid = clientInfo_.FindClientPid(client);
    if (pid == INVALID_PID) {
        HiLog::Error(LABEL, "%{public}s pid is -1", __func__);
        return;
    }
    HiLog::Info(LABEL, "%{public}s pid is %d", __func__, pid);
    clientInfo_.DestroySensorChannel(pid);
    clientInfo_.DestroyClientPid(client);
    clientInfo_.DestroyCmd(this->GetCallingUid());
    HiLog::Info(LABEL, "%{public}s end", __func__);
}

void SensorService::RegisterClientDeathRecipient(sptr<IRemoteObject> sensorClient, int32_t pid)
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    sptr<ISensorClient> client = iface_cast<ISensorClient>(sensorClient);
    clientDeathObserver_ = new (std::nothrow) DeathRecipientTemplate(*const_cast<SensorService *>(this));
    if (clientDeathObserver_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s clientDeathObserver_ cannot be null", __func__);
        return;
    }
    client->AsObject()->AddDeathRecipient(clientDeathObserver_);
    clientInfo_.SaveClientPid(sensorClient, pid);
    HiLog::Info(LABEL, "%{public}s end", __func__);
}

void SensorService::UnregisterClientDeathRecipient(sptr<IRemoteObject> sensorClient)
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    sptr<ISensorClient> client = iface_cast<ISensorClient>(sensorClient);
    clientDeathObserver_ = new (std::nothrow) DeathRecipientTemplate(*const_cast<SensorService *>(this));
    if (clientDeathObserver_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s clientDeathObserver_ cannot be null", __func__);
        return;
    }
    client->AsObject()->RemoveDeathRecipient(clientDeathObserver_);
    clientInfo_.DestroyClientPid(sensorClient);
    HiLog::Info(LABEL, "%{public}s end", __func__);
}

int32_t SensorService::Dump(int32_t fd, const std::vector<std::u16string> &args)
{
    HiLog::Info(LABEL, "%{public}s begin", __func__);
    SensorDump &sensorDump = SensorDump::GetInstance();
    if ((args.empty()) || (args[0].size() != MAX_DMUP_PARAM)) {
        HiLog::Error(LABEL, "%{public}s param cannot be empty or the length is not 2", __func__);
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
    HiLog::Info(LABEL, "%{public}s end", __func__);
    return ERR_OK;
}
}  // namespace Sensors
}  // namespace OHOS
