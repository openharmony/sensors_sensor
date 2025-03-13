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

#include "sensor_service.h"

#include <cinttypes>
#include <string_ex.h>
#include <tokenid_kit.h>

#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
#include "hisysevent.h"
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
#ifdef MEMMGR_ENABLE
#include "mem_mgr_client.h"
#endif // MEMMGR_ENABLE
#include "ipc_skeleton.h"
#include "permission_util.h"

#include "print_sensor_data.h"
#include "sensor_dump.h"
#include "system_ability_definition.h"

#undef LOG_TAG
#define LOG_TAG "SensorService"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
auto g_sensorService = SensorDelayedSpSingleton<SensorService>::GetInstance();
const bool G_REGISTER_RESULT = SystemAbility::MakeAndRegisterAbility(g_sensorService.GetRefPtr());
constexpr int32_t INVALID_PID = -1;
constexpr int64_t MAX_EVENT_COUNT = 1000;
std::atomic_bool g_isRegister = false;
} // namespace

std::atomic_bool SensorService::isAccessTokenServiceActive_ = false;

SensorService::SensorService()
    : SystemAbility(SENSOR_SERVICE_ABILITY_ID, true), state_(SensorServiceState::STATE_STOPPED)
{
    SEN_HILOGD("Add SystemAbility");
}

SensorService::~SensorService() {}

void SensorService::OnDump()
{
    SEN_HILOGI("OnDump");
}

void SensorService::OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    SEN_HILOGI("OnAddSystemAbility systemAbilityId:%{public}d", systemAbilityId);
#ifdef MEMMGR_ENABLE
    if (systemAbilityId == MEMORY_MANAGER_SA_ID) {
        Memory::MemMgrClient::GetInstance().NotifyProcessStatus(getpid(),
            PROCESS_TYPE_SA, PROCESS_STATUS_STARTED, SENSOR_SERVICE_ABILITY_ID);
    }
#endif // MEMMGR_ENABLE
#ifdef ACCESS_TOKEN_ENABLE
    if (systemAbilityId == ACCESS_TOKEN_MANAGER_SERVICE_ID) {
        isAccessTokenServiceActive_ = true;
    }
#endif // ACCESS_TOKEN_ENABLE
}

void SensorService::OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    SEN_HILOGI("OnRemoveSystemAbility systemAbilityId:%{public}d", systemAbilityId);
#ifdef ACCESS_TOKEN_ENABLE
    if (systemAbilityId == ACCESS_TOKEN_MANAGER_SERVICE_ID) {
        isAccessTokenServiceActive_ = false;
    }
#endif // ACCESS_TOKEN_ENABLE
}

void SensorService::OnStart()
{
    CALL_LOG_ENTER;
    if (state_ == SensorServiceState::STATE_RUNNING) {
        SEN_HILOGW("SensorService has already started");
        return;
    }
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
    if (!InitInterface()) {
        SEN_HILOGE("Init interface error");
    }
    if (!InitDataCallback()) {
        SEN_HILOGE("Init data callback error");
    }
    if (!InitSensorList()) {
        SEN_HILOGE("Init sensor list error");
    }
    sensorDataProcesser_ = new (std::nothrow) SensorDataProcesser(sensorMap_);
    CHKPV(sensorDataProcesser_);
#endif // HDF_DRIVERS_INTERFACE_SENSOR
    if (!InitSensorPolicy()) {
        SEN_HILOGE("Init sensor policy error");
    }
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
    sensorManager_.InitSensorMap(sensorMap_, sensorDataProcesser_, reportDataCallback_);
#else
    sensorManager_.InitSensorMap(sensorMap_);
#endif // HDF_DRIVERS_INTERFACE_SENSOR
    if (!SystemAbility::Publish(SensorDelayedSpSingleton<SensorService>::GetInstance())) {
        SEN_HILOGE("Publish SensorService error");
        return;
    }
    state_ = SensorServiceState::STATE_RUNNING;
#ifdef MEMMGR_ENABLE
    AddSystemAbilityListener(MEMORY_MANAGER_SA_ID);
#endif // MEMMGR_ENABLE
#ifdef ACCESS_TOKEN_ENABLE
    AddSystemAbilityListener(ACCESS_TOKEN_MANAGER_SERVICE_ID);
#endif // ACCESS_TOKEN_ENABLE
}

#ifdef HDF_DRIVERS_INTERFACE_SENSOR
bool SensorService::InitInterface()
{
    auto ret = sensorHdiConnection_.ConnectHdi();
    if (ret != ERR_OK) {
        SEN_HILOGE("Connect hdi failed");
        return false;
    }
    return true;
}

bool SensorService::InitDataCallback()
{
    reportDataCallback_ = new (std::nothrow) ReportDataCallback();
    CHKPF(reportDataCallback_);
    ReportDataCb cb = &ReportDataCallback::ReportEventCallback;
    auto ret = sensorHdiConnection_.RegisterDataReport(cb, reportDataCallback_);
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
    if (ret != 0) {
        SEN_HILOGE("GetSensorList is failed");
        return false;
    }
    {
        std::lock_guard<std::mutex> sensorMapLock(sensorMapMutex_);
        for (const auto &it : sensors_) {
            if (!(sensorMap_.insert(std::make_pair(it.GetSensorId(), it)).second)) {
                SEN_HILOGW("sensorMap_ insert failed");
            }
        }
    }
    return true;
}
#endif // HDF_DRIVERS_INTERFACE_SENSOR

bool SensorService::InitSensorPolicy()
{
    return true;
}

void SensorService::OnStop()
{
    CALL_LOG_ENTER;
    if (state_ == SensorServiceState::STATE_STOPPED) {
        SEN_HILOGW("Already stopped");
        return;
    }
    state_ = SensorServiceState::STATE_STOPPED;
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
    int32_t ret = sensorHdiConnection_.DestroyHdiConnection();
    if (ret != ERR_OK) {
        SEN_HILOGE("Destroy hdi connect fail");
    }
#endif // HDF_DRIVERS_INTERFACE_SENSOR
    UnregisterPermCallback();
#ifdef MEMMGR_ENABLE
    Memory::MemMgrClient::GetInstance().NotifyProcessStatus(getpid(), PROCESS_TYPE_SA, PROCESS_STATUS_DIED,
        SENSOR_SERVICE_ABILITY_ID);
#endif // MEMMGR_ENABLE
}

void SensorService::ReportSensorSysEvent(int32_t sensorId, bool enable, int32_t pid, int64_t samplingPeriodNs,
    int64_t maxReportDelayNs)
{
    std::string packageName("");
    AccessTokenID tokenId = clientInfo_.GetTokenIdByPid(pid);
    sensorManager_.GetPackageName(tokenId, packageName, isAccessTokenServiceActive_);
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
    const int logLevel = 4;
    int32_t uid = clientInfo_.GetUidByPid(pid);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
    if (enable) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "ENABLE_SENSOR", HiSysEvent::EventType::STATISTIC,
            "LEVEL", logLevel, "PKG_NAME", packageName, "TYPE", sensorId, "UID", uid, "PID", pid);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        SEN_HILOGI("PackageName:%{public}s open the sensor, sensorId:%{public}d, pid:%{public}d, "
            "samplingPeriodNs:%{public}" PRId64 ", samplingPeriodNs:%{public}" PRId64, packageName.c_str(),
            sensorId, pid, samplingPeriodNs, maxReportDelayNs);
    } else {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "DISABLE_SENSOR", HiSysEvent::EventType::STATISTIC,
            "LEVEL", logLevel, "TYPE", sensorId, "PKG_NAME", packageName, "UID", uid, "PID", pid);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        SEN_HILOGI("PackageName:%{public}s close the sensor, sensorId:%{public}d, pid:%{public}d",
            packageName.c_str(), sensorId, pid);
    }
}

void SensorService::ReportOnChangeData(int32_t sensorId)
{
    std::lock_guard<std::mutex> sensorMapLock(sensorMapMutex_);
    auto it = sensorMap_.find(sensorId);
    if (it == sensorMap_.end()) {
        SEN_HILOGE("sensorId is invalid");
        return;
    }
    if ((SENSOR_ON_CHANGE & it->second.GetFlags()) != SENSOR_ON_CHANGE) {
        SEN_HILOGW("The data has not changed , no need to report");
        return;
    }
    SensorData sensorData;
    auto ret = clientInfo_.GetStoreEvent(sensorId, sensorData);
    if (ret != ERR_OK) {
        SEN_HILOGE("There is no data to be reported");
        return;
    }
    sptr<SensorBasicDataChannel> channel = clientInfo_.GetSensorChannelByPid(GetCallingPid());
    CHKPV(channel);
    auto sendRet = channel->SendData(&sensorData, sizeof(sensorData));
    if (sendRet != ERR_OK) {
        SEN_HILOGE("Send data failed");
        return;
    }
}

ErrCode SensorService::SaveSubscriber(int32_t sensorId, int64_t samplingPeriodNs, int64_t maxReportDelayNs)
{
    SEN_HILOGI("In, sensorId:%{public}d", sensorId);
    if (!sensorManager_.SaveSubscriber(sensorId, GetCallingPid(), samplingPeriodNs, maxReportDelayNs)) {
        SEN_HILOGE("SaveSubscriber failed");
        return UPDATE_SENSOR_INFO_ERR;
    }
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
    sensorManager_.StartDataReportThread();
    SensorBasicInfo sensorInfo = clientInfo_.GetCurPidSensorInfo(sensorId, GetCallingPid());
    if (!sensorManager_.SetBestSensorParams(sensorId,
        sensorInfo.GetSamplingPeriodNs(), sensorInfo.GetMaxReportDelayNs())) {
        SEN_HILOGE("SetBestSensorParams failed");
        clientInfo_.RemoveSubscriber(sensorId, GetCallingPid());
        return SET_SENSOR_CONFIG_ERR;
    }
#endif // HDF_DRIVERS_INTERFACE_SENSOR
    SEN_HILOGI("Done, sensorId:%{public}d", sensorId);
    return ERR_OK;
}

bool SensorService::CheckSensorId(int32_t sensorId)
{
    std::lock_guard<std::mutex> sensorMapLock(sensorMapMutex_);
    auto it = sensorMap_.find(sensorId);
    if (it == sensorMap_.end()) {
        SEN_HILOGE("Invalid sensorId, sensorId:%{public}d", sensorId);
        return false;
    }
    return true;
}

bool SensorService::IsSystemServiceCalling()
{
    const auto tokenId = IPCSkeleton::GetCallingTokenID();
    const auto flag = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId);
    if (flag == Security::AccessToken::ATokenTypeEnum::TOKEN_NATIVE ||
        flag == Security::AccessToken::ATokenTypeEnum::TOKEN_SHELL) {
        SEN_HILOGD("system service calling, flag: %{public}u", flag);
        return true;
    }
    return false;
}

bool SensorService::IsSystemCalling()
{
    if (IsSystemServiceCalling()) {
        return true;
    }
    return Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(IPCSkeleton::GetCallingFullTokenID());
}

ErrCode SensorService::CheckAuthAndParameter(int32_t sensorId, int64_t samplingPeriodNs, int64_t maxReportDelayNs)
{
    if ((sensorId == SENSOR_TYPE_ID_COLOR || sensorId == SENSOR_TYPE_ID_SAR ||
            sensorId > GL_SENSOR_TYPE_PRIVATE_MIN_VALUE) &&
        !IsSystemCalling()) {
        SEN_HILOGE("Permission check failed. A non-system application uses the system API");
        return NON_SYSTEM_API;
    }
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckSensorPermission(GetCallingTokenID(), sensorId);
    if (ret != PERMISSION_GRANTED) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "VERIFY_ACCESS_TOKEN_FAIL", HiSysEvent::EventType::SECURITY,
            "PKG_NAME", "SensorEnableInner", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        SEN_HILOGE("sensorId:%{public}d grant failed, ret:%{public}d", sensorId, ret);
        return PERMISSION_DENIED;
    }
    if ((!CheckSensorId(sensorId)) || (maxReportDelayNs != 0L && samplingPeriodNs != 0L &&
        ((maxReportDelayNs / samplingPeriodNs) > MAX_EVENT_COUNT))) {
        SEN_HILOGE("sensorId is invalid or maxReportDelayNs exceeded the maximum value");
        return ERR_NO_INIT;
    }
    return ERR_OK;
}

ErrCode SensorService::EnableSensor(int32_t sensorId, int64_t samplingPeriodNs, int64_t maxReportDelayNs)
{
    CALL_LOG_ENTER;
    ErrCode checkResult = CheckAuthAndParameter(sensorId, samplingPeriodNs, maxReportDelayNs);
    if (checkResult != ERR_OK) {
        return checkResult;
    }
    int32_t pid = GetCallingPid();
    std::lock_guard<std::mutex> serviceLock(serviceLock_);
    if (clientInfo_.GetSensorState(sensorId)) {
        SEN_HILOGW("Sensor has been enabled already");
        auto ret = SaveSubscriber(sensorId, samplingPeriodNs, maxReportDelayNs);
        if (ret != ERR_OK) {
            SEN_HILOGE("SaveSubscriber failed");
            return ret;
        }
        ReportSensorSysEvent(sensorId, true, pid, samplingPeriodNs, maxReportDelayNs);
        if (ret != ERR_OK) {
            SEN_HILOGE("ret:%{public}d", ret);
        }
        ReportOnChangeData(sensorId);
        if (isReportActiveInfo_) {
            ReportActiveInfo(sensorId, pid);
        }
        PrintSensorData::GetInstance().ResetHdiCounter(sensorId);
        SEN_HILOGI("Done, sensorId:%{public}d", sensorId);
        return ERR_OK;
    }
    auto ret = SaveSubscriber(sensorId, samplingPeriodNs, maxReportDelayNs);
    if (ret != ERR_OK) {
        SEN_HILOGE("SaveSubscriber failed");
        clientInfo_.RemoveSubscriber(sensorId, GetCallingPid());
        return ret;
    }
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
    ret = sensorHdiConnection_.EnableSensor(sensorId);
    if (ret != ERR_OK) {
        SEN_HILOGE("EnableSensor failed");
        clientInfo_.RemoveSubscriber(sensorId, GetCallingPid());
        return ENABLE_SENSOR_ERR;
    }
#endif // HDF_DRIVERS_INTERFACE_SENSOR
    if ((!g_isRegister) && (RegisterPermCallback(sensorId))) {
        g_isRegister = true;
    }
    ReportSensorSysEvent(sensorId, true, pid, samplingPeriodNs, maxReportDelayNs);
    if (isReportActiveInfo_) {
        ReportActiveInfo(sensorId, pid);
    }
    PrintSensorData::GetInstance().ResetHdiCounter(sensorId);
    return ret;
}

ErrCode SensorService::DisableSensor(int32_t sensorId, int32_t pid)
{
    CALL_LOG_ENTER;
    if (!CheckSensorId(sensorId)) {
        SEN_HILOGE("sensorId is invalid");
        return ERR_NO_INIT;
    }
    if (pid < 0) {
        SEN_HILOGE("pid is invalid, pid:%{public}d", pid);
        return CLIENT_PID_INVALID_ERR;
    }
    ReportSensorSysEvent(sensorId, false, pid);
    std::lock_guard<std::mutex> serviceLock(serviceLock_);
    if (sensorManager_.IsOtherClientUsingSensor(sensorId, pid)) {
        SEN_HILOGW("Other client is using this sensor now, can't disable");
        return ERR_OK;
    }
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
    if (sensorHdiConnection_.DisableSensor(sensorId) != ERR_OK) {
        SEN_HILOGE("DisableSensor is failed");
        return DISABLE_SENSOR_ERR;
    }
#endif // HDF_DRIVERS_INTERFACE_SENSOR
    int32_t uid = clientInfo_.GetUidByPid(pid);
    clientInfo_.DestroyCmd(uid);
    clientInfo_.ClearDataQueue(sensorId);
    return sensorManager_.AfterDisableSensor(sensorId);
}

ErrCode SensorService::DisableSensor(int32_t sensorId)
{
    CALL_LOG_ENTER;
    if ((sensorId == SENSOR_TYPE_ID_COLOR || sensorId == SENSOR_TYPE_ID_SAR ||
            sensorId > GL_SENSOR_TYPE_PRIVATE_MIN_VALUE) &&
        !IsSystemCalling()) {
        SEN_HILOGE("Permission check failed. A non-system application uses the system API");
        return NON_SYSTEM_API;
    }
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckSensorPermission(GetCallingTokenID(), sensorId);
    if (ret != PERMISSION_GRANTED) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "VERIFY_ACCESS_TOKEN_FAIL", HiSysEvent::EventType::SECURITY,
            "PKG_NAME", "SensorDisableInner", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        SEN_HILOGE("sensorId:%{public}d grant failed, ret:%{public}d", sensorId, ret);
        return PERMISSION_DENIED;
    }
    return DisableSensor(sensorId, GetCallingPid());
}

ErrCode SensorService::GetSensorList(std::vector<Sensor> &sensorList)
{
    std::vector<Sensor> sensors = GetSensorList();
    int32_t sensorCount = static_cast<int32_t>(sensors.size());
    if (sensorCount > MAX_SENSOR_COUNT) {
        SEN_HILOGD("SensorCount:%{public}u", sensorCount);
        sensorCount = MAX_SENSOR_COUNT;
    }
    for (int32_t i = 0; i < sensorCount; ++i) {
        sensorList.push_back(sensors[i]);
    }
    return ERR_OK;
}

std::vector<Sensor> SensorService::GetSensorList()
{
    std::lock_guard<std::mutex> sensorLock(sensorsMutex_);
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
    int32_t ret = sensorHdiConnection_.GetSensorList(sensors_);
    if (ret != 0) {
        SEN_HILOGE("GetSensorList is failed");
        return sensors_;
    }
#endif // HDF_DRIVERS_INTERFACE_SENSOR
    for (const auto &it : sensors_) {
        std::lock_guard<std::mutex> sensorMapLock(sensorMapMutex_);
        sensorMap_.insert(std::make_pair(it.GetSensorId(), it));
    }
    return sensors_;
}

ErrCode SensorService::TransferDataChannel(int32_t sendFd, const sptr<IRemoteObject> &sensorClient)
{
    SEN_HILOGI("In");
    sptr<SensorBasicDataChannel> sensorBasicDataChannel = new (std::nothrow) SensorBasicDataChannel();
    CHKPR(sensorBasicDataChannel, OBJECT_NULL);
    auto ret = sensorBasicDataChannel->CreateSensorBasicChannelBySendFd(sendFd);
    if (ret != ERR_OK) {
        SEN_HILOGE("CreateSensorBasicChannelBySendFd ret:%{public}d", ret);
        return OBJECT_NULL;
    }
    CHKPR(sensorBasicDataChannel, ERR_NO_INIT);
    auto pid = GetCallingPid();
    auto uid = GetCallingUid();
    auto callerToken = GetCallingTokenID();
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
    SEN_HILOGI("Done");
    return ERR_OK;
}

ErrCode SensorService::DestroySensorChannel(const sptr<IRemoteObject> &sensorClient)
{
    CALL_LOG_ENTER;
    const int32_t clientPid = GetCallingPid();
    if (clientPid < 0) {
        SEN_HILOGE("clientPid is invalid, clientPid:%{public}d", clientPid);
        return CLIENT_PID_INVALID_ERR;
    }
    std::lock_guard<std::mutex> serviceLock(serviceLock_);
    bool destroyRet = clientInfo_.DestroySensorChannel(clientPid);
    if (!destroyRet) {
        SEN_HILOGE("DestroySensorChannel is failed");
        return DESTROY_SENSOR_CHANNEL_ERR;
    }
    clientInfo_.DestroyCmd(GetCallingUid());
    UnregisterClientDeathRecipient(sensorClient);
    return ERR_OK;
}

void SensorService::ProcessDeathObserver(const wptr<IRemoteObject> &object)
{
    CALL_LOG_ENTER;
    sptr<IRemoteObject> client = object.promote();
    CHKPV(client);
    int32_t pid = clientInfo_.FindClientPid(client);
    if (pid == INVALID_PID) {
        SEN_HILOGE("pid is invalid");
        return;
    }
    SEN_HILOGI("pid is %{public}d", pid);
    std::vector<int32_t> activeSensors = clientInfo_.GetSensorIdByPid(pid);
    for (size_t i = 0; i < activeSensors.size(); ++i) {
        int32_t ret = DisableSensor(activeSensors[i], pid);
        if (ret != ERR_OK) {
            SEN_HILOGE("DisableSensor failed, ret:%{public}d", ret);
        }
    }
    DelSession(pid);
    clientInfo_.DelActiveInfoCBPid(pid);
    clientInfo_.DestroySensorChannel(pid);
    clientInfo_.DestroyClientPid(client);
    clientInfo_.DestroyCmd(clientInfo_.GetUidByPid(pid));
    POWER_POLICY.DeleteDeathPidSensorInfo(pid);
}

void SensorService::RegisterClientDeathRecipient(sptr<IRemoteObject> sensorClient, int32_t pid)
{
    CALL_LOG_ENTER;
    CHKPV(sensorClient);
    std::lock_guard<std::mutex> clientDeathObserverLock(clientDeathObserverMutex_);
    if (clientDeathObserver_ == nullptr) {
        clientDeathObserver_ = new (std::nothrow) DeathRecipientTemplate(*const_cast<SensorService *>(this));
        CHKPV(clientDeathObserver_);
    }
    sensorClient->AddDeathRecipient(clientDeathObserver_);
    clientInfo_.SaveClientPid(sensorClient, pid);
}

void SensorService::UnregisterClientDeathRecipient(sptr<IRemoteObject> sensorClient)
{
    CALL_LOG_ENTER;
    CHKPV(sensorClient);
    int32_t pid = clientInfo_.FindClientPid(sensorClient);
    if (pid == INVALID_PID) {
        SEN_HILOGE("Pid is invalid");
        return;
    }
    if (!clientInfo_.CallingService(pid)) {
        SEN_HILOGD("Can't unregister client death recipient");
        return;
    }
    std::lock_guard<std::mutex> clientDeathObserverLock(clientDeathObserverMutex_);
    sensorClient->RemoveDeathRecipient(clientDeathObserver_);
    clientInfo_.DestroyClientPid(sensorClient);
}

int32_t SensorService::Dump(int32_t fd, const std::vector<std::u16string> &args)
{
    CALL_LOG_ENTER;
    if (fd < 0) {
        SEN_HILOGE("Invalid fd");
        return DUMP_PARAM_ERR;
    }
    SensorDump &sensorDump = SensorDump::GetInstance();
    if (args.empty()) {
        SEN_HILOGE("Param cannot be empty");
        dprintf(fd, "param cannot be empty\n");
        sensorDump.DumpHelp(fd);
        return DUMP_PARAM_ERR;
    }
    std::vector<std::string> argList = { "" };
    std::transform(args.begin(), args.end(), std::back_inserter(argList),
        [](const std::u16string &arg) {
        return Str16ToStr8(arg);
    });
    sensorDump.ParseCommand(fd, argList, sensors_, clientInfo_);
    return ERR_OK;
}

ErrCode SensorService::SuspendSensors(int32_t pid)
{
    CALL_LOG_ENTER;
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    if (!permissionUtil.IsNativeToken(GetCallingTokenID())) {
        SEN_HILOGE("TokenType is not TOKEN_NATIVE");
        return PERMISSION_DENIED;
    }
    int32_t ret = permissionUtil.CheckManageSensorPermission(GetCallingTokenID());
    if (ret != PERMISSION_GRANTED) {
        SEN_HILOGE("Check manage sensor permission failed, ret:%{public}d", ret);
        return PERMISSION_DENIED;
    }
    if (pid < 0) {
        SEN_HILOGE("Pid is invalid");
        return CLIENT_PID_INVALID_ERR;
    }
    return POWER_POLICY.SuspendSensors(pid);
}

ErrCode SensorService::ResumeSensors(int32_t pid)
{
    CALL_LOG_ENTER;
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    if (!permissionUtil.IsNativeToken(GetCallingTokenID())) {
        SEN_HILOGE("TokenType is not TOKEN_NATIVE");
        return PERMISSION_DENIED;
    }
    int32_t ret = permissionUtil.CheckManageSensorPermission(GetCallingTokenID());
    if (ret != PERMISSION_GRANTED) {
        SEN_HILOGE("Check manage sensor permission failed, ret:%{public}d", ret);
        return PERMISSION_DENIED;
    }
    if (pid < 0) {
        SEN_HILOGE("Pid is invalid");
        return CLIENT_PID_INVALID_ERR;
    }
    return POWER_POLICY.ResumeSensors(pid);
}

ErrCode SensorService::GetActiveInfoList(int32_t pid, std::vector<ActiveInfo> &activeInfoList)
{
    CALL_LOG_ENTER;
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    if (!permissionUtil.IsNativeToken(GetCallingTokenID())) {
        SEN_HILOGE("TokenType is not TOKEN_NATIVE");
        return PERMISSION_DENIED;
    }
    if (pid < 0) {
        SEN_HILOGE("Pid is invalid");
        return CLIENT_PID_INVALID_ERR;
    }
    activeInfoList = POWER_POLICY.GetActiveInfoList(pid);
    uint32_t activeInfoCount = static_cast<uint32_t>(activeInfoList.size());
    if (activeInfoCount > MAX_SENSOR_COUNT) {
        SEN_HILOGD("ActiveInfoCount:%{public}u", activeInfoCount);
        activeInfoList.erase(activeInfoList.begin() + MAX_SENSOR_COUNT, activeInfoList.begin() + activeInfoCount - 1);
    }
    return ERR_OK;
}

ErrCode SensorService::CreateSocketChannel(const sptr<IRemoteObject> &sensorClient, int32_t &clientFd)
{
    CALL_LOG_ENTER;
    CHKPR(sensorClient, INVALID_POINTER);
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    if (!permissionUtil.IsNativeToken(GetCallingTokenID())) {
        SEN_HILOGE("TokenType is not TOKEN_NATIVE");
        return PERMISSION_DENIED;
    }
    int32_t serverFd = -1;
    int32_t ret = AddSocketPairInfo(GetCallingUid(), GetCallingPid(),
        AccessTokenKit::GetTokenTypeFlag(GetCallingTokenID()),
        serverFd, std::ref(clientFd));
    if (ret != ERR_OK) {
        SEN_HILOGE("Add socket pair info failed, ret:%{public}d", ret);
        return ret;
    }
    RegisterClientDeathRecipient(sensorClient, GetCallingPid());
    return ERR_OK;
}

ErrCode SensorService::DestroySocketChannel(const sptr<IRemoteObject> &sensorClient)
{
    CALL_LOG_ENTER;
    CHKPR(sensorClient, INVALID_POINTER);
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    if (!permissionUtil.IsNativeToken(GetCallingTokenID())) {
        SEN_HILOGE("TokenType is not TOKEN_NATIVE");
        return PERMISSION_DENIED;
    }
    DelSession(GetCallingPid());
    UnregisterClientDeathRecipient(sensorClient);
    return ERR_OK;
}

ErrCode SensorService::EnableActiveInfoCB()
{
    CALL_LOG_ENTER;
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    if (!permissionUtil.IsNativeToken(GetCallingTokenID())) {
        SEN_HILOGE("TokenType is not TOKEN_NATIVE");
        return PERMISSION_DENIED;
    }
    isReportActiveInfo_ = true;
    return clientInfo_.AddActiveInfoCBPid(GetCallingPid());
}

ErrCode SensorService::DisableActiveInfoCB()
{
    CALL_LOG_ENTER;
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    if (!permissionUtil.IsNativeToken(GetCallingTokenID())) {
        SEN_HILOGE("TokenType is not TOKEN_NATIVE");
        return PERMISSION_DENIED;
    }
    isReportActiveInfo_ = false;
    return clientInfo_.DelActiveInfoCBPid(GetCallingPid());
}

ErrCode SensorService::ResetSensors()
{
    CALL_LOG_ENTER;
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    if (!permissionUtil.IsNativeToken(GetCallingTokenID())) {
        SEN_HILOGE("TokenType is not TOKEN_NATIVE");
        return PERMISSION_DENIED;
    }
    int32_t ret = permissionUtil.CheckManageSensorPermission(GetCallingTokenID());
    if (ret != PERMISSION_GRANTED) {
        SEN_HILOGE("Check manage sensor permission failed, ret:%{public}d", ret);
        return PERMISSION_DENIED;
    }
    return POWER_POLICY.ResetSensors();
}

void SensorService::ReportActiveInfo(int32_t sensorId, int32_t pid)
{
    CALL_LOG_ENTER;
    std::vector<SessionPtr> sessionList;
    auto pidList = clientInfo_.GetActiveInfoCBPid();
    for (const auto &pid : pidList) {
        auto sess = GetSessionByPid(pid);
        if (sess != nullptr) {
            sessionList.push_back(sess);
        }
    }
    SensorBasicInfo sensorInfo = clientInfo_.GetCurPidSensorInfo(sensorId, pid);
    ActiveInfo activeInfo(pid, sensorId, sensorInfo.GetSamplingPeriodNs(),
        sensorInfo.GetMaxReportDelayNs());
    POWER_POLICY.ReportActiveInfo(activeInfo, sessionList);
}

bool SensorService::RegisterPermCallback(int32_t sensorId)
{
    CALL_LOG_ENTER;
    if ((sensorId != SENSOR_TYPE_ID_PEDOMETER) && (sensorId != SENSOR_TYPE_ID_PEDOMETER_DETECTION) &&
        (sensorId != SENSOR_TYPE_ID_HEART_RATE)) {
        SEN_HILOGD("No need listen for the sensor permission changes");
        return false;
    }
    Security::AccessToken::PermStateChangeScope scope = {
        .permList = { ACTIVITY_MOTION_PERMISSION, READ_HEALTH_DATA_PERMISSION }
    };
    permStateChangeCb_ = std::make_shared<PermStateChangeCb>(scope, this);
    int32_t ret = Security::AccessToken::AccessTokenKit::RegisterPermStateChangeCallback(permStateChangeCb_);
    if (ret != ERR_OK) {
        SEN_HILOGE("RegisterPermStateChangeCallback fail");
        return false;
    }
    return true;
}

void SensorService::UnregisterPermCallback()
{
    CALL_LOG_ENTER;
    CHKPV(permStateChangeCb_);
    int32_t ret = Security::AccessToken::AccessTokenKit::UnRegisterPermStateChangeCallback(permStateChangeCb_);
    if (ret != ERR_OK) {
        SEN_HILOGE("UnregisterPermStateChangeCallback fail");
        return;
    }
    g_isRegister = false;
}

void SensorService::PermStateChangeCb::PermStateChangeCallback(Security::AccessToken::PermStateChangeInfo &result)
{
    CALL_LOG_ENTER;
    CHKPV(server_);
    server_->clientInfo_.ChangeSensorPerm(result.tokenID, result.permissionName,
        (result.permStateChangeType != 0));
}
} // namespace Sensors
} // namespace OHOS
