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

#include "sensor_power_policy.h"

#ifdef OHOS_BUILD_ENABLE_RUST
#include "rust_binding.h"
#endif // OHOS_BUILD_ENABLE_RUST
#undef LOG_TAG
#define LOG_TAG "SensorPowerPolicy"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr int32_t INVALID_SENSOR_ID = -1;
constexpr int64_t MAX_EVENT_COUNT = 1000;
ClientInfo &clientInfo_ = ClientInfo::GetInstance();
SensorManager &sensorManager_ = SensorManager::GetInstance();
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
SensorHdiConnection &sensorHdiConnection_ = SensorHdiConnection::GetInstance();
#endif // HDF_DRIVERS_INTERFACE_SENSOR
} // namespace

bool SensorPowerPolicy::CheckFreezingSensor(int32_t sensorId)
{
    return ((sensorId == SENSOR_TYPE_ID_PEDOMETER_DETECTION) || (sensorId == SENSOR_TYPE_ID_PEDOMETER));
}

ErrCode SensorPowerPolicy::SuspendSensors(int32_t pid)
{
    CALL_LOG_ENTER;
    std::vector<std::string> sensorIndexList = clientInfo_.GetSensorIdByPid(pid);
    if (sensorIndexList.empty()) {
        SEN_HILOGD("Suspend sensors failed, sensorIdList is empty, pid:%{public}d", pid);
        return SUSPEND_ERR;
    }
    std::lock_guard<std::mutex> pidSensorInfoLock(pidSensorInfoMutex_);
    auto pidSensorInfoIt = pidSensorInfoMap_.find(pid);
    if (pidSensorInfoIt != pidSensorInfoMap_.end()) {
        std::unordered_map<std::string, SensorBasicInfo> sensorInfoMap = pidSensorInfoIt->second;
        if (!Suspend(pid, sensorIndexList, sensorInfoMap)) {
            SEN_HILOGE("Suspend part sensors, but some failed, pid:%{public}d", pid);
            return SUSPEND_ERR;
        }
        SEN_HILOGI("Suspend sensors success, pid:%{public}d", pid);
        return ERR_OK;
    }
    std::unordered_map<std::string, SensorBasicInfo> sensorInfoMap;
    auto isAllSuspend = Suspend(pid, sensorIndexList, sensorInfoMap);
    pidSensorInfoMap_.insert(std::make_pair(pid, sensorInfoMap));
    if (!isAllSuspend) {
        SEN_HILOGE("Suspend all sensors, but some failed, pid:%{public}d", pid);
        return SUSPEND_ERR;
    }
    SEN_HILOGI("Suspend sensors success, pid:%{public}d", pid);
    return ERR_OK;
}

bool SensorPowerPolicy::Suspend(int32_t pid, const std::vector<std::string> &sensorIndexList,
    std::unordered_map<std::string, SensorBasicInfo> &sensorInfoMap)
{
    CALL_LOG_ENTER;
    bool isAllSuspend = true;
    for (const auto &sensorIndex : sensorIndexList) {
        SensorDescription sensorDesc;
        clientInfo_.ParseIndex(sensorIndex, sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId,
            sensorDesc.location);
        if (CheckFreezingSensor(sensorDesc.sensorType)) {
            SEN_HILOGD("Current sensor is pedometer detection or pedometer, can not suspend");
            continue;
        }
        auto sensorInfo = clientInfo_.GetCurPidSensorInfo(sensorDesc, pid);
        if (sensorManager_.IsOtherClientUsingSensor(sensorDesc, pid)) {
            SEN_HILOGD("Other client is using this sensor now, cannot suspend, sensorId:%{public}d",
                sensorDesc.sensorType);
            sensorInfoMap.insert(std::make_pair(sensorIndex, sensorInfo));
            continue;
        }
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
        auto ret = sensorHdiConnection_.DisableSensor(sensorDesc);
        if (ret != ERR_OK) {
            isAllSuspend = false;
            SEN_HILOGE("Hdi disable sensor failed, sensorId:%{public}d, ret:%{public}d", sensorDesc.sensorType, ret);
        }
#endif // HDF_DRIVERS_INTERFACE_SENSOR
        sensorInfoMap.insert(std::make_pair(sensorIndex, sensorInfo));
        sensorManager_.AfterDisableSensor(sensorDesc);
    }
    return isAllSuspend;
}

ErrCode SensorPowerPolicy::ResumeSensors(int32_t pid)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> pidSensorInfoLock(pidSensorInfoMutex_);
    auto pidSensorInfoIt = pidSensorInfoMap_.find(pid);
    if (pidSensorInfoIt == pidSensorInfoMap_.end()) {
        SEN_HILOGD("Resume sensors failed, please suspend sensors first, pid:%{public}d", pid);
        return RESUME_ERR;
    }
    bool isAllResume = true;
    std::unordered_map<std::string, SensorBasicInfo> sensorInfoMap = pidSensorInfoIt->second;
    for (auto sensorIt = sensorInfoMap.begin(); sensorIt != sensorInfoMap.end();) {
        SensorDescription sensorDesc;
        clientInfo_.ParseIndex(sensorIt->first, sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId,
            sensorDesc.location);
        int64_t samplingPeriodNs = sensorIt->second.GetSamplingPeriodNs();
        int64_t maxReportDelayNs = sensorIt->second.GetMaxReportDelayNs();
        if (!Resume(pid, sensorDesc, samplingPeriodNs, maxReportDelayNs)) {
            SEN_HILOGE("Resume sensor failed, sensorId:%{public}d", sensorDesc.sensorType);
            isAllResume = false;
            ++sensorIt;
        } else {
            sensorIt = sensorInfoMap.erase(sensorIt);
        }
    }
    if (!isAllResume) {
        SEN_HILOGE("Resume all sensors, but some failed, pid:%{public}d", pid);
        return RESUME_ERR;
    }
    pidSensorInfoMap_.erase(pidSensorInfoIt);
    SEN_HILOGI("Resume sensors success, pid:%{public}d", pid);
    return ERR_OK;
}

bool SensorPowerPolicy::Resume(int32_t pid, SensorDescription sensorDesc, int64_t samplingPeriodNs,
    int64_t maxReportDelayNs)
{
    CALL_LOG_ENTER;
    if ((sensorDesc.sensorType == INVALID_SENSOR_ID) || (samplingPeriodNs <= 0) ||
        ((samplingPeriodNs != 0L) && (maxReportDelayNs / samplingPeriodNs > MAX_EVENT_COUNT))) {
        SEN_HILOGE("sensorId is invalid or maxReportDelayNs exceed the maximum value");
        return false;
    }
    if (clientInfo_.GetSensorState(sensorDesc)) {
        SEN_HILOGD("Sensor is enable, sensorId:%{public}d", sensorDesc.sensorType);
        auto ret = RestoreSensorInfo(pid, sensorDesc, samplingPeriodNs, maxReportDelayNs);
        if (ret != ERR_OK) {
            SEN_HILOGE("Restore sensor info failed, ret:%{public}d", ret);
            return false;
        }
        return true;
    }
    auto ret = RestoreSensorInfo(pid, sensorDesc, samplingPeriodNs, maxReportDelayNs);
    if (ret != ERR_OK) {
        SEN_HILOGE("Restore sensor info failed, ret:%{public}d", ret);
        return false;
    }
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
    ret = sensorHdiConnection_.EnableSensor(sensorDesc);
    if (ret != ERR_OK) {
        SEN_HILOGE("Hdi enable sensor failed, sensorId:%{public}d, ret:%{public}d", sensorDesc.sensorType, ret);
        clientInfo_.RemoveSubscriber(sensorDesc, pid);
        return false;
    }
#endif // HDF_DRIVERS_INTERFACE_SENSOR
    return true;
}

ErrCode SensorPowerPolicy::RestoreSensorInfo(int32_t pid, SensorDescription sensorDesc, int64_t samplingPeriodNs,
    int64_t maxReportDelayNs)
{
    CALL_LOG_ENTER;
    if (!sensorManager_.SaveSubscriber(sensorDesc, pid, samplingPeriodNs, maxReportDelayNs)) {
        SEN_HILOGE("SaveSubscriber failed");
        return UPDATE_SENSOR_INFO_ERR;
    }
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
    sensorManager_.StartDataReportThread();
    if (!sensorManager_.SetBestSensorParams(sensorDesc, samplingPeriodNs, maxReportDelayNs)) {
        SEN_HILOGE("SetBestSensorParams failed");
        clientInfo_.RemoveSubscriber(sensorDesc, pid);
        return SET_SENSOR_CONFIG_ERR;
    }
#endif // HDF_DRIVERS_INTERFACE_SENSOR
    return ERR_OK;
}

std::vector<int32_t> SensorPowerPolicy::GetSuspendPidList()
{
    CALL_LOG_ENTER;
    std::vector<int32_t> suspendPidList;
    std::lock_guard<std::mutex> pidSensorInfoLock(pidSensorInfoMutex_);
    for (const auto &pidSensorInfo : pidSensorInfoMap_) {
        int32_t pid = pidSensorInfo.first;
        suspendPidList.push_back(pid);
    }
    return suspendPidList;
}

ErrCode SensorPowerPolicy::ResetSensors()
{
    CALL_LOG_ENTER;
    std::vector<int32_t> suspendPidList = GetSuspendPidList();
    bool resetStatus = true;
    for (const auto &pid : suspendPidList) {
        if (ResumeSensors(pid) != ERR_OK) {
            SEN_HILOGE("Reset pid sensors failed, pid:%{public}d", pid);
            resetStatus = false;
        }
    }
    if (resetStatus) {
        SEN_HILOGI("Reset sensors success");
    }
    return resetStatus ? ERR_OK : RESET_ERR;
}


std::vector<ActiveInfo> SensorPowerPolicy::GetActiveInfoList(int32_t pid)
{
    CALL_LOG_ENTER;
    std::vector<ActiveInfo> activeInfoList;
    std::vector<std::string> sensorIdList = clientInfo_.GetSensorIdByPid(pid);
    for (const auto &sensorId : sensorIdList) {
        SensorDescription sensorDesc;
        clientInfo_.ParseIndex(sensorId, sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId,
            sensorDesc.location);
        auto sensorInfo = clientInfo_.GetCurPidSensorInfo(sensorDesc, pid);
        ActiveInfo activeInfo(pid, sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId,
            sensorInfo.GetSamplingPeriodNs(), sensorInfo.GetMaxReportDelayNs());
        activeInfoList.push_back(activeInfo);
    }
    if (activeInfoList.size() > 0) {
        SEN_HILOGI("Get active info list success, pid:%{public}d", pid);
    } else {
        SEN_HILOGW("activeInfoList is empty");
    }
    return activeInfoList;
}

void SensorPowerPolicy::ReportActiveInfo(const ActiveInfo &activeInfo,
    const std::vector<SessionPtr> &sessionList)
{
    CALL_LOG_ENTER;
    if (activeInfo.GetPid() < 0 || activeInfo.GetSensorId() < 0) {
        SEN_HILOGE("Invalid activeInfo");
        return;
    }
    NetPacket pkt(MessageId::ACTIVE_INFO);
    pkt << activeInfo.GetPid() << activeInfo.GetSensorId() <<
        activeInfo.GetSamplingPeriodNs() << activeInfo.GetMaxReportDelayNs();
#ifdef OHOS_BUILD_ENABLE_RUST
    if (StreamBufferChkRWError(pkt.streamBufferPtr_.get())) {
#else
    if (pkt.ChkRWError()) {
#endif // OHOS_BUILD_ENABLE_RUST
        SEN_HILOGE("Packet write data failed");
        return;
    }
    for (const auto &sess : sessionList) {
        if (!sess->SendMsg(pkt)) {
            SEN_HILOGE("Packet send failed");
            continue;
        }
    }
}

void SensorPowerPolicy::DeleteDeathPidSensorInfo(int32_t pid)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> pidSensorInfoLock(pidSensorInfoMutex_);
    auto pidSensorInfoIt = pidSensorInfoMap_.find(pid);
    if (pidSensorInfoIt != pidSensorInfoMap_.end()) {
        SEN_HILOGD("Delete death pid sensor info, pid:%{public}d", pid);
        pidSensorInfoMap_.erase(pidSensorInfoIt);
    }
}
} // namespace Sensors
} // namespace OHOS