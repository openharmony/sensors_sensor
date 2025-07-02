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

#include <charconv>
#include <cinttypes>
#include <string_ex.h>
#include <sys/time.h>
#include <tokenid_kit.h>

#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
#include "hisysevent.h"
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
#ifdef MEMMGR_ENABLE
#include "mem_mgr_client.h"
#endif // MEMMGR_ENABLE
#include "motion_plugin.h"
#include "ipc_skeleton.h"
#include "permission_util.h"
#include "parameters.h"

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
constexpr int32_t SENSOR_ONLINE = 1;
std::atomic_bool g_isRegister = false;
constexpr int32_t SINGLE_DISPLAY_SMALL_FOLD = 4;
constexpr int32_t SINGLE_DISPLAY_THREE_FOLD = 6;
const std::string DEFAULTS_FOLD_TYPE = "0,0,0,0";
const std::set<int32_t> g_systemApiSensorCall = {
    SENSOR_TYPE_ID_COLOR, SENSOR_TYPE_ID_SAR, SENSOR_TYPE_ID_HEADPOSTURE
};
} // namespace

std::atomic_bool SensorService::isAccessTokenServiceActive_ = false;
std::atomic_bool SensorService::isCritical_ = false;

SensorService::SensorService()
    : SystemAbility(SENSOR_SERVICE_ABILITY_ID, true), state_(SensorServiceState::STATE_STOPPED)
{
    SEN_HILOGD("Add SystemAbility");
}

SensorService::~SensorService()
{
    UnloadMotionSensor();
}

void SensorService::OnDump()
{
    SEN_HILOGI("OnDump");
}

std::string GetDmsDeviceStatus()
{
    return OHOS::system::GetParameter("persist.dms.device.status", "0");
}

bool SensorService::IsNeedLoadMotionLib()
{
    std::string supportDevice = OHOS::system::GetParameter("const.window.foldscreen.type", DEFAULTS_FOLD_TYPE);
    size_t index = supportDevice.find(',');
    if (index != std::string::npos) {
        std::string firstValue = supportDevice.substr(0, index);
        SEN_HILOGI("firstValue:%{public}s", firstValue.c_str());
        if (std::isdigit(firstValue[0]) == 0) {
            SEN_HILOGI("firstValue is not number");
            return false;
        }
        int32_t firstValueNum = 0;
        auto res = std::from_chars(firstValue.data(), firstValue.data() + firstValue.size(), firstValueNum);
        if (res.ec != std::errc()) {
            SEN_HILOGE("Failed to convert string %{public}s to number", firstValue.c_str());
            return false;
        }
        if (firstValueNum == SINGLE_DISPLAY_SMALL_FOLD || firstValueNum == SINGLE_DISPLAY_THREE_FOLD) {
            return true;
        }
    }
    SEN_HILOGI("Not support in this device");
    return false;
}

void SensorService::OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    SEN_HILOGI("OnAddSystemAbility systemAbilityId:%{public}d", systemAbilityId);
#ifdef MEMMGR_ENABLE
    if (systemAbilityId == MEMORY_MANAGER_SA_ID) {
        Memory::MemMgrClient::GetInstance().NotifyProcessStatus(getpid(),
            PROCESS_TYPE_SA, PROCESS_STATUS_STARTED, SENSOR_SERVICE_ABILITY_ID);
        SetCritical();
    }
#endif // MEMMGR_ENABLE
#ifdef ACCESS_TOKEN_ENABLE
    if (systemAbilityId == ACCESS_TOKEN_MANAGER_SERVICE_ID) {
        isAccessTokenServiceActive_ = true;
    }
#endif // ACCESS_TOKEN_ENABLE
#ifdef MSDP_MOTION_ENABLE
    if (systemAbilityId == MSDP_MOTION_SERVICE_ID) {
        if (!IsNeedLoadMotionLib()) {
            SEN_HILOGI("No need to load motion lib");
        } else if (!LoadMotionSensor()) {
            SEN_HILOGI("LoadMotionSensor fail");
        }
    }
#endif // MSDP_MOTION_ENABLE
    if (systemAbilityId == DISPLAY_MANAGER_SERVICE_SA_ID) {
        std::string statusStr = GetDmsDeviceStatus();
        int32_t statusNum;
        auto res = std::from_chars(statusStr.data(), statusStr.data() + statusStr.size(), statusNum);
        if (res.ec != std::errc()) {
            SEN_HILOGE("Failed to convert string %{public}s to number", statusStr.c_str());
            return;
        }
        uint32_t status = static_cast<uint32_t>(statusNum);
        clientInfo_.SetDeviceStatus(status);
        SEN_HILOGI("GetDeviceStatus, deviceStatus:%{public}d", status);
    }
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
    if (!InitPlugCallback()) {
        SEN_HILOGE("Init plug callback error");
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
#ifdef MSDP_MOTION_ENABLE
    AddSystemAbilityListener(MSDP_MOTION_SERVICE_ID);
#endif // MSDP_MOTION_ENABLE
    AddSystemAbilityListener(DISPLAY_MANAGER_SERVICE_SA_ID);
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

bool SensorService::InitPlugCallback()
{
    auto ret = sensorHdiConnection_.RegSensorPlugCallback(
        std::bind(&SensorService::ReportPlugEventCallback, this, std::placeholders::_1));
    if (ret != ERR_OK) {
        SEN_HILOGE("RegSensorPlugCallback failed");
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
            if (!(sensorMap_.insert(std::pair<SensorDescription, Sensor>({
                it.GetDeviceId(), it.GetSensorTypeId(), it.GetSensorId(), it.GetLocation()}, it)).second)) {
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

void SensorService::ReportSensorSysEvent(int32_t sensorType, bool enable, int32_t pid, int64_t samplingPeriodNs,
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
            "LEVEL", logLevel, "PKG_NAME", packageName, "TYPE", sensorType, "UID", uid, "PID", pid);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        SEN_HILOGI("PackageName:%{public}s open the sensor, sensorType:%{public}d, pid:%{public}d, "
            "samplingPeriodNs:%{public}" PRId64 ", samplingPeriodNs:%{public}" PRId64, packageName.c_str(),
            sensorType, pid, samplingPeriodNs, maxReportDelayNs);
    } else {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "DISABLE_SENSOR", HiSysEvent::EventType::STATISTIC,
            "LEVEL", logLevel, "TYPE", sensorType, "PKG_NAME", packageName, "UID", uid, "PID", pid);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        SEN_HILOGI("PackageName:%{public}s close the sensor, sensorType:%{public}d, pid:%{public}d",
            packageName.c_str(), sensorType, pid);
    }
}

void SensorService::ReportOnChangeData(const SensorDescription &sensorDesc)
{
    std::lock_guard<std::mutex> sensorMapLock(sensorMapMutex_);
    auto it = sensorMap_.find(sensorDesc);
    if (it == sensorMap_.end()) {
        SEN_HILOGE("sensorDesc is invalid");
        return;
    }
    if ((SENSOR_ON_CHANGE & it->second.GetFlags()) != SENSOR_ON_CHANGE) {
        SEN_HILOGW("The data has not changed , no need to report");
        return;
    }
    SensorData sensorData;
    auto ret = clientInfo_.GetStoreEvent(sensorDesc, sensorData);
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

ErrCode SensorService::SaveSubscriber(const SensorDescription &sensorDesc, int64_t samplingPeriodNs,
    int64_t maxReportDelayNs)
{
    SEN_HILOGI("In, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    if (!sensorManager_.SaveSubscriber(sensorDesc, GetCallingPid(), samplingPeriodNs, maxReportDelayNs)) {
        SEN_HILOGE("SaveSubscriber failed");
        return UPDATE_SENSOR_INFO_ERR;
    }
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
    sensorManager_.StartDataReportThread();
    SensorBasicInfo sensorInfo = clientInfo_.GetCurPidSensorInfo(sensorDesc, GetCallingPid());
    if (!sensorManager_.SetBestSensorParams(sensorDesc,
        sensorInfo.GetSamplingPeriodNs(), sensorInfo.GetMaxReportDelayNs())) {
        SEN_HILOGE("SetBestSensorParams failed");
        clientInfo_.RemoveSubscriber(sensorDesc, GetCallingPid());
        return SET_SENSOR_CONFIG_ERR;
    }
#endif // HDF_DRIVERS_INTERFACE_SENSOR
    SEN_HILOGI("Done, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    return ERR_OK;
}

bool SensorService::CheckSensorId(const SensorDescription &sensorDesc)
{
    std::lock_guard<std::mutex> sensorMapLock(sensorMapMutex_);
    auto it = sensorMap_.find(sensorDesc);
    if (it == sensorMap_.end()) {
        SEN_HILOGE("Invalid sensorDesc,"
            "deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d, location:%{public}d",
            sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId, sensorDesc.location);
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

ErrCode SensorService::CheckAuthAndParameter(const SensorDescription &sensorDesc, int64_t samplingPeriodNs,
    int64_t maxReportDelayNs)
{
    if (((g_systemApiSensorCall.find(sensorDesc.sensorType) != g_systemApiSensorCall.end()) ||
        (sensorDesc.sensorType > GL_SENSOR_TYPE_PRIVATE_MIN_VALUE)) && !IsSystemCalling()) {
        SEN_HILOGE("Permission check failed. A non-system application uses the system API");
        return NON_SYSTEM_API;
    }
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckSensorPermission(GetCallingTokenID(), sensorDesc.sensorType);
    if (ret != PERMISSION_GRANTED) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "VERIFY_ACCESS_TOKEN_FAIL", HiSysEvent::EventType::SECURITY,
            "PKG_NAME", "SensorEnableInner", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        SEN_HILOGE("sensorType:%{public}d grant failed, ret:%{public}d", sensorDesc.sensorType, ret);
        return PERMISSION_DENIED;
    }
    if ((!CheckSensorId(sensorDesc)) || (maxReportDelayNs != 0L && samplingPeriodNs != 0L &&
        ((maxReportDelayNs / samplingPeriodNs) > MAX_EVENT_COUNT))) {
        SEN_HILOGE("sensorDesc is invalid or maxReportDelayNs exceeded the maximum value");
        return ERR_NO_INIT;
    }
    return ERR_OK;
}

ErrCode SensorService::EnableSensor(const SensorDescriptionIPC &SensorDescriptionIPC, int64_t samplingPeriodNs,
    int64_t maxReportDelayNs)
{
    CALL_LOG_ENTER;
    SensorDescription sensorDesc {
        .deviceId = SensorDescriptionIPC.deviceId,
        .sensorType = SensorDescriptionIPC.sensorType,
        .sensorId = SensorDescriptionIPC.sensorId,
        .location = SensorDescriptionIPC.location
    };
    ErrCode checkResult = CheckAuthAndParameter(sensorDesc, samplingPeriodNs, maxReportDelayNs);
    if (checkResult != ERR_OK) {
        return checkResult;
    }
    int32_t pid = GetCallingPid();
    std::lock_guard<std::mutex> serviceLock(serviceLock_);
    if (clientInfo_.GetSensorState(sensorDesc)) {
        return SensorReportEvent(sensorDesc, samplingPeriodNs, maxReportDelayNs, pid);
    }
    auto ret = SaveSubscriber(sensorDesc, samplingPeriodNs, maxReportDelayNs);
    if (ret != ERR_OK) {
        SEN_HILOGE("SaveSubscriber failed");
        clientInfo_.RemoveSubscriber(sensorDesc, GetCallingPid());
        return ret;
    }
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
    ret = sensorHdiConnection_.EnableSensor(sensorDesc);
    if (ret != ERR_OK) {
        SEN_HILOGE("EnableSensor failed");
        clientInfo_.RemoveSubscriber(sensorDesc, GetCallingPid());
        return ENABLE_SENSOR_ERR;
    }
#endif // HDF_DRIVERS_INTERFACE_SENSOR
    SetCritical();
    if ((!g_isRegister) && (RegisterPermCallback(sensorDesc.sensorType))) {
        g_isRegister = true;
    }
    ReportSensorSysEvent(sensorDesc.sensorType, true, pid, samplingPeriodNs, maxReportDelayNs);
    if (isReportActiveInfo_) {
        ReportActiveInfo(sensorDesc, pid);
    }
    PrintSensorData::GetInstance().ResetHdiCounter(sensorDesc.sensorType);
    return ret;
}

ErrCode SensorService::SensorReportEvent(const SensorDescription &sensorDesc, int64_t samplingPeriodNs,
    int64_t maxReportDelayNs, int32_t pid)
{
    SEN_HILOGW("Sensor has been enabled already");
    auto ret = SaveSubscriber(sensorDesc, samplingPeriodNs, maxReportDelayNs);
    if (ret != ERR_OK) {
        SEN_HILOGE("SaveSubscriber failed");
        return ret;
    }
    ReportSensorSysEvent(sensorDesc.sensorType, true, pid, samplingPeriodNs, maxReportDelayNs);
    if (ret != ERR_OK) {
        SEN_HILOGE("ret:%{public}d", ret);
    }
    ReportOnChangeData(sensorDesc);
    if (isReportActiveInfo_) {
        ReportActiveInfo(sensorDesc, pid);
    }
    PrintSensorData::GetInstance().ResetHdiCounter(sensorDesc.sensorType);
    SEN_HILOGI("Done, sensorTypeId:%{public}d", sensorDesc.sensorType);
    return ERR_OK;
}

ErrCode SensorService::DisableSensor(const SensorDescription &sensorDesc, int32_t pid)
{
    CALL_LOG_ENTER;
    if (!(CheckSensorId(sensorDesc) || ((!CheckSensorId(sensorDesc)) && clientInfo_.GetSensorState(sensorDesc)))) {
        SEN_HILOGE("sensorDesc is invalid");
        return ERR_NO_INIT;
    }
    if (pid < 0) {
        SEN_HILOGE("pid is invalid, pid:%{public}d", pid);
        return CLIENT_PID_INVALID_ERR;
    }
    ReportSensorSysEvent(sensorDesc.sensorType, false, pid);
    std::lock_guard<std::mutex> serviceLock(serviceLock_);
    if (sensorManager_.IsOtherClientUsingSensor(sensorDesc, pid)) {
        SEN_HILOGW("Other client is using this sensor now, can't disable");
        return ERR_OK;
    }
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
    if (sensorHdiConnection_.DisableSensor(sensorDesc) != ERR_OK) {
        if (CheckSensorId(sensorDesc)) {
            SEN_HILOGE("DisableSensor is failed");
            return DISABLE_SENSOR_ERR;
        }
        SEN_HILOGW("DisableSensor is failed, deviceId:%{public}d, sensorType:%{public}d, sensorId:%{public}d",
            sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    }
#endif // HDF_DRIVERS_INTERFACE_SENSOR
    int32_t uid = clientInfo_.GetUidByPid(pid);
    clientInfo_.DestroyCmd(uid);
    clientInfo_.ClearDataQueue(sensorDesc);
    int32_t ret = sensorManager_.AfterDisableSensor(sensorDesc);
#ifdef MEMMGR_ENABLE
    if (!clientInfo_.IsClientSubscribe() && isCritical_) {
        if (Memory::MemMgrClient::GetInstance().SetCritical(getpid(), false, SENSOR_SERVICE_ABILITY_ID) != ERR_OK) {
            SEN_HILOGE("setCritical failed");
            return ret;
        }
        isCritical_ = false;
    }
#endif // MEMMGR_ENABLE
    return ret;
}

ErrCode SensorService::DisableSensor(const SensorDescriptionIPC &SensorDescriptionIPC)
{
    CALL_LOG_ENTER;
    SensorDescription sensorDesc {
        .deviceId = SensorDescriptionIPC.deviceId,
        .sensorType = SensorDescriptionIPC.sensorType,
        .sensorId = SensorDescriptionIPC.sensorId,
        .location = SensorDescriptionIPC.location
    };
    if ((sensorDesc.sensorType == SENSOR_TYPE_ID_COLOR || sensorDesc.sensorType == SENSOR_TYPE_ID_SAR ||
            sensorDesc.sensorType > GL_SENSOR_TYPE_PRIVATE_MIN_VALUE) &&
        !IsSystemCalling()) {
        SEN_HILOGE("Permission check failed. A non-system application uses the system API");
        return NON_SYSTEM_API;
    }
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckSensorPermission(GetCallingTokenID(), sensorDesc.sensorType);
    if (ret != PERMISSION_GRANTED) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "VERIFY_ACCESS_TOKEN_FAIL", HiSysEvent::EventType::SECURITY,
            "PKG_NAME", "SensorDisableInner", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        SEN_HILOGE("sensorType:%{public}d grant failed, ret:%{public}d", sensorDesc.sensorType, ret);
        return PERMISSION_DENIED;
    }
    return DisableSensor(sensorDesc, GetCallingPid());
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

ErrCode SensorService::GetSensorListByDevice(int32_t deviceId, std::vector<Sensor> &singleDevSensors)
{
    CALL_LOG_ENTER;
    {
        std::lock_guard<std::mutex> sensorLock(sensorsMutex_);
        for (const auto& sensor : sensors_) {
            if (sensor.GetDeviceId() == deviceId) {
                SEN_HILOGD("Sensor found: id is %{public}d", deviceId);
                singleDevSensors.push_back(sensor);
            }
        }
    }

    if (singleDevSensors.empty()) {
        std::vector<Sensor> sensors = GetSensorListByDevice(deviceId);
        int32_t sensorCount = static_cast<int32_t>(sensors.size());
        for (int32_t i = 0; i < sensorCount; ++i) {
            singleDevSensors.push_back(sensors[i]);
        }
    }
    return ERR_OK;
}

std::vector<Sensor> SensorService::GetSensorListByDevice(int32_t deviceId)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> sensorLock(sensorsMutex_);
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
    std::vector<Sensor> singleDevSensors;
    int32_t ret = sensorHdiConnection_.GetSensorListByDevice(deviceId, singleDevSensors);
    if (ret != 0 || singleDevSensors.empty()) {
        SEN_HILOGW("GetSensorListByDevice is failed or empty");
        return sensors_;
    }
    for (const auto& newSensor : singleDevSensors) {
        bool found = false;
        for (auto& oldSensor : sensors_) {
            if (oldSensor.GetSensorId() == newSensor.GetSensorId() &&
                oldSensor.GetDeviceId() == newSensor.GetDeviceId() &&
                oldSensor.GetSensorTypeId() == newSensor.GetSensorTypeId()) {
                SEN_HILOGD("Sensor found in sensorList_");
                found = true;
                break;
            }
        }
        if (!found) {
            SEN_HILOGD("Sensor not found in sensorList_");
                found = true;
            sensors_.push_back(newSensor);
        }
    }
#endif // HDF_DRIVERS_INTERFACE_SENSOR

    std::lock_guard<std::mutex> sensorMapLock(sensorMapMutex_);
    for (const auto &it : sensors_) {
        auto iter = sensorMap_.find({it.GetDeviceId(), it.GetSensorTypeId(), it.GetSensorId(), it.GetLocation()});
        if (iter != sensorMap_.end()) {
            iter->second = it;
        } else {
            sensorMap_.insert(std::pair<SensorDescription, Sensor>(
                {it.GetDeviceId(), it.GetSensorTypeId(), it.GetSensorId(), it.GetLocation()}, it));
            if (sensorDataProcesser_ != nullptr) {
                sensorDataProcesser_->UpdateSensorMap(sensorMap_);
            }
        }
    }
    return singleDevSensors;
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
        sensorMap_.insert(std::pair<SensorDescription, Sensor>({
            it.GetDeviceId(), it.GetSensorTypeId(), it.GetSensorId(), it.GetLocation()}, it));
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
    std::string packageName("");
    sensorManager_.GetPackageName(callerToken, packageName, isAccessTokenServiceActive_);
    SEN_HILOGI("Calling packageName:%{public}s", packageName.c_str());
    sensorBasicDataChannel->SetPackageName(packageName);
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
    POWER_POLICY.DeleteDeathPidSensorInfo(pid);
    SEN_HILOGI("pid is %{public}d", pid);
    std::vector<SensorDescription> activeSensors = clientInfo_.GetSensorIdByPid(pid);
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
    std::lock_guard<std::mutex> sensorLock(sensorsMutex_);
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

void SensorService::ReportActiveInfo(const SensorDescription &sensorDesc, int32_t pid)
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
    SensorBasicInfo sensorInfo = clientInfo_.GetCurPidSensorInfo(sensorDesc, pid);
    ActiveInfo activeInfo(pid, sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId,
        sensorInfo.GetSamplingPeriodNs(), sensorInfo.GetMaxReportDelayNs());
    POWER_POLICY.ReportActiveInfo(activeInfo, sessionList);
}

bool SensorService::RegisterPermCallback(int32_t sensorType)
{
    CALL_LOG_ENTER;
    if ((sensorType != SENSOR_TYPE_ID_PEDOMETER) && (sensorType != SENSOR_TYPE_ID_PEDOMETER_DETECTION) &&
        (sensorType != SENSOR_TYPE_ID_HEART_RATE)) {
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

ErrCode SensorService::SetDeviceStatus(uint32_t deviceStatus)
{
    SEN_HILOGI("SetDeviceStatus in, deviceStatus:%{public}d", deviceStatus);
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    if (!permissionUtil.IsNativeToken(GetCallingTokenID())) {
        SEN_HILOGE("TokenType is not TOKEN_NATIVE");
        return PERMISSION_DENIED;
    }
    clientInfo_.SetDeviceStatus(deviceStatus);
    return ERR_OK;
}

ErrCode SensorService::TransferClientRemoteObject(const sptr<IRemoteObject> &sensorClient)
{
    CALL_LOG_ENTER;
    clientInfo_.SaveSensorClient(sensorClient);
    return ERR_OK;
}

ErrCode SensorService::DestroyClientRemoteObject(const sptr<IRemoteObject> &sensorClient)
{
    CALL_LOG_ENTER;
    clientInfo_.DestroySensorClient(sensorClient);
    return ERR_OK;
}

void SensorService::ReportPlugEventCallback(const SensorPlugInfo &info)
{
    CALL_LOG_ENTER;
    if (info.status == SENSOR_ONLINE) {
        auto it = sensorMap_.find({info.deviceSensorInfo.deviceId, info.deviceSensorInfo.sensorType,
            info.deviceSensorInfo.sensorId, info.deviceSensorInfo.location});
        if (it == sensorMap_.end()) {
            GetSensorListByDevice(info.deviceSensorInfo.deviceId);
        }
    } else {
        if (!sensorHdiConnection_.PlugEraseSensorData(info)) {
            SEN_HILOGW("sensorHdiConnection Cache update failure");
        }
        std::lock_guard<std::mutex> sensorMapLock(sensorMapMutex_);
        auto it = std::find_if(sensors_.begin(), sensors_.end(), [&](const Sensor& sensor) {
            return sensor.GetDeviceId() == info.deviceSensorInfo.deviceId &&
                sensor.GetSensorTypeId() == info.deviceSensorInfo.sensorType &&
                sensor.GetSensorId() == info.deviceSensorInfo.sensorId;
        });
        if (it != sensors_.end()) {
            sensors_.erase(it);
        }
        auto iter = sensorMap_.find({info.deviceSensorInfo.deviceId, info.deviceSensorInfo.sensorType,
            info.deviceSensorInfo.sensorId, info.deviceSensorInfo.location});
        if (iter != sensorMap_.end()) {
            sensorMap_.erase(iter);
        }
    }
    struct timeval curTime;
    curTime.tv_sec = 0;
    curTime.tv_usec = 0;
    gettimeofday(&curTime, NULL);
    const SensorPlugData sensorPlugData = {
        .deviceId = info.deviceSensorInfo.deviceId,
        .sensorTypeId = info.deviceSensorInfo.sensorType,
        .sensorId = info.deviceSensorInfo.sensorId,
        .location = info.deviceSensorInfo.location,
        .deviceName = info.deviceName,
        .status = info.status,
        .reserved = info.reserved,
        .timestamp = static_cast<int64_t>(curTime.tv_sec * 1000 + curTime.tv_usec / 1000) //1000:milliSecond
    };
    clientInfo_.SendMsgToClient(sensorPlugData);
}

void SensorService::SetCritical()
{
#ifdef MEMMGR_ENABLE
    if (!isCritical_) {
        if (Memory::MemMgrClient::GetInstance().SetCritical(getpid(), true, SENSOR_SERVICE_ABILITY_ID) != ERR_OK) {
            SEN_HILOGE("setCritical failed");
        } else {
            isCritical_ = true;
        }
    }
#endif // MEMMGR_ENABLE
}
} // namespace Sensors
} // namespace OHOS
