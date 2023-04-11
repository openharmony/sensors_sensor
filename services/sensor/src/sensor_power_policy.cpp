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

#include "sensor.h"
#include "sensor_agent_type.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "SensorPowerPolicy" };
constexpr int32_t INVALID_SENSOR_ID = -1;
constexpr int64_t MAX_EVENT_COUNT = 1000;
ClientInfo &clientInfo_ = ClientInfo::GetInstance();
SensorManager &sensorManager_ = SensorManager::GetInstance();
SensorHdiConnection &sensorHdiConnection_ = SensorHdiConnection::GetInstance();
}  // namespace

bool SensorPowerPolicy::CheckFreezingSensor(int32_t sensorId)
{
    return ((sensorId == SENSOR_TYPE_ID_PEDOMETER_DETECTION) || (sensorId == SENSOR_TYPE_ID_PEDOMETER));
}

ErrCode SensorPowerPolicy::SuspendSensors(int32_t pid)
{
    CALL_LOG_ENTER;
    std::vector<int32_t> sensorIdList = clientInfo_.GetSensorIdByPid(pid);
    if (sensorIdList.empty()) {
        SEN_HILOGW("SensorIdList is empty, pid not enable sensor or all sensor have bean suspend");
        return ERR_OK;
    }
    std::lock_guard<std::mutex> pidSensorInfoLock(pidSensorInfoMutex_);
    auto pidSensorInfoIt = pidSensorInfoMap_.find(pid);
    if (pidSensorInfoIt != pidSensorInfoMap_.end()) {
        SEN_HILOGI("Pid already call suspend, but some sensors suspend failed, suspend these sensors again");
        std::unordered_map<int32_t, SensorBasicInfo> sensorInfoMap = pidSensorInfoIt->second;
        if (!Suspend(pid, sensorIdList, sensorInfoMap)) {
            SEN_HILOGE("Suspend last failed sensors, but some failed");
            return DISABLE_SENSOR_ERR;
        }
        return ERR_OK;
    }
    std::unordered_map<int32_t, SensorBasicInfo> sensorInfoMap;
    auto isAllSuspend = Suspend(pid, sensorIdList, sensorInfoMap);
    pidSensorInfoMap_.insert(std::make_pair(pid, sensorInfoMap));
    if (!isAllSuspend) {
        SEN_HILOGE("Suspend all sensors, but some failed");
        return DISABLE_SENSOR_ERR;
    }
    return ERR_OK;
}

bool SensorPowerPolicy::Suspend(int32_t pid, std::vector<int32_t> &sensorIdList,
    std::unordered_map<int32_t, SensorBasicInfo> &sensorInfoMap)
{
    CALL_LOG_ENTER;
    bool isAllSuspend = true;
    for (auto &sensorId : sensorIdList) {
        if (CheckFreezingSensor(sensorId)) {
            SEN_HILOGD("Current sensor is pedometer detection or pedometer, can not suspend");
            continue;
        }
        auto sensorInfo = clientInfo_.GetCurPidSensorInfo(sensorId, pid);
        if (sensorManager_.IsOtherClientUsingSensor(sensorId, pid)) {
            SEN_HILOGD("Other client is using this sensor now, cannot suspend");
            sensorInfoMap.insert(std::make_pair(sensorId, sensorInfo));
            continue;
        }
        if (sensorHdiConnection_.DisableSensor(sensorId) != ERR_OK) {
            SEN_HILOGE("Disable sensor failed, sensorId:%{public}d", sensorId);
            isAllSuspend = false;
            continue;
        }
        sensorInfoMap.insert(std::make_pair(sensorId, sensorInfo));
        sensorManager_.AfterDisableSensor(sensorId);
    }
    return isAllSuspend;
}

ErrCode SensorPowerPolicy::ResumeSensors(int32_t pid)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> pidSensorInfoLock(pidSensorInfoMutex_);
    auto pidSensorInfoIt = pidSensorInfoMap_.find(pid);
    if (pidSensorInfoIt == pidSensorInfoMap_.end()) {
        SEN_HILOGE("Pid not have suspend sensors");
        return ERROR;
    }
    bool isAllResume = true;
    std::unordered_map<int32_t, SensorBasicInfo> sensorInfoMap = pidSensorInfoIt->second;
    for (auto sensorIt = sensorInfoMap.begin(); sensorIt != sensorInfoMap.end();) {
        int32_t sensorId = sensorIt->first;
        int64_t samplingPeriodNs = sensorIt->second.GetSamplingPeriodNs();
        int64_t maxReportDelayNs = sensorIt->second.GetMaxReportDelayNs();
        if (!Resume(pid, sensorId, samplingPeriodNs, maxReportDelayNs)) {
            SEN_HILOGE("Resume sensor failed. sensorId:%{public}d", sensorId);
            isAllResume = false;
            ++sensorIt;
        } else {
            sensorIt = sensorInfoMap.erase(sensorIt);
        }
    }
    if (!isAllResume) {
        SEN_HILOGE("Some sensor resume failed");
        return ENABLE_SENSOR_ERR;
    }
    pidSensorInfoMap_.erase(pidSensorInfoIt);
    return ERR_OK;
}

bool SensorPowerPolicy::Resume(int32_t pid, int32_t sensorId, int64_t samplingPeriodNs,
    int64_t maxReportDelayNs)
{
    CALL_LOG_ENTER;
    if ((sensorId == INVALID_SENSOR_ID) || (samplingPeriodNs <= 0) ||
        ((samplingPeriodNs != 0L) && (maxReportDelayNs / samplingPeriodNs > MAX_EVENT_COUNT))) {
        SEN_HILOGE("SensorId is invalid or maxReportDelayNs exceed the maximum value");
        return false;
    }
    if (clientInfo_.GetSensorState(sensorId)) {
        SEN_HILOGD("Sensor is enable, sensorId:%{public}d", sensorId);
        auto ret = RestoreSensorInfo(pid, sensorId, samplingPeriodNs, maxReportDelayNs);
        if (ret != ERR_OK) {
            SEN_HILOGE("Restore sensor info failed, ret:%{public}d", ret);
            return false;
        }
        return true;
    }
    auto ret = RestoreSensorInfo(pid, sensorId, samplingPeriodNs, maxReportDelayNs);
    if (ret != ERR_OK) {
        SEN_HILOGE("Restore sensor info failed, ret:%{public}d", ret);
        return false;
    }
    ret = sensorHdiConnection_.EnableSensor(sensorId);
    if (ret != ERR_OK) {
        SEN_HILOGE("Resume sensor failed, ret:%{public}d", ret);
        clientInfo_.RemoveSubscriber(sensorId, pid);
        return false;
    }
    return true;
}

ErrCode SensorPowerPolicy::RestoreSensorInfo(int32_t pid, int32_t sensorId, int64_t samplingPeriodNs,
    int64_t maxReportDelayNs)
{
    CALL_LOG_ENTER;
    auto ret = sensorManager_.SaveSubscriber(sensorId, pid, samplingPeriodNs, maxReportDelayNs);
    if (ret != ERR_OK) {
        SEN_HILOGE("SaveSubscriber failed, ret:%{public}d", ret);
        return ret;
    }
    sensorManager_.StartDataReportThread();
    if (!sensorManager_.SetBestSensorParams(sensorId, samplingPeriodNs, maxReportDelayNs)) {
        SEN_HILOGE("SetBestSensorParams failed");
        clientInfo_.RemoveSubscriber(sensorId, pid);
        return ENABLE_SENSOR_ERR;
    }
    return ERR_OK;
}

std::vector<ActiveInfo> SensorPowerPolicy::GetActiveInfoList(int32_t pid)
{
    CALL_LOG_ENTER;
    std::vector<ActiveInfo> activeInfoList;
    std::vector<int32_t> sensorIdList = clientInfo_.GetSensorIdByPid(pid);
    for (auto &sensorId : sensorIdList) {
        auto sensorInfo = clientInfo_.GetCurPidSensorInfo(sensorId, pid);
        ActiveInfo activeInfo(pid, sensorId, sensorInfo.GetSamplingPeriodNs(),
            sensorInfo.GetMaxReportDelayNs());
        activeInfoList.push_back(activeInfo);
    }
    return activeInfoList;
}

void SensorPowerPolicy::ReportActiveInfo(const ActiveInfo &activeInfo,
    const std::vector<SessionPtr> &sessionList)
{
    CALL_LOG_ENTER;
    NetPacket pkt(MessageId::ACTIVE_INFO);
    pkt << activeInfo.GetPid() << activeInfo.GetSensorId() <<
        activeInfo.GetSamplingPeriodNs() << activeInfo.GetMaxReportDelayNs();
    if (pkt.ChkRWError()) {
        SEN_HILOGE("Packet write data failed");
        return;
    }
    for (auto sess : sessionList) {
        if (!sess->SendMsg(pkt)) {
            SEN_HILOGE("Packet send failed");
            continue;
        }
    }
}
}  // namespace Sensors
}  // namespace OHOS