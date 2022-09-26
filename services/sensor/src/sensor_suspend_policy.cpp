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

#include "sensor_suspend_policy.h"

#include "sensor.h"
#include "sensor_service.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "SensorSuspendPolicy" };
constexpr uint32_t INVALID_SENSOR_ID = -1;
constexpr int64_t MAX_EVENT_COUNT = 1000;
constexpr int64_t DEFAULT_SAMPLEING_RATE = 200000000;
constexpr int64_t DEFAULT_REPORT_DELAY = 0;
constexpr uint32_t STEP_COUNTER_ID = 524544;
constexpr uint32_t STEP_DETECOTR_ID = 590080;
}  // namespace

SensorSuspendPolicy::~SensorSuspendPolicy()
{}

bool SensorSuspendPolicy::CheckFreezingSensor(uint32_t sensorId)
{
    return ((sensorId == STEP_COUNTER_ID) || (sensorId == STEP_DETECOTR_ID));
}

ErrCode SensorSuspendPolicy::DisableSensor(uint32_t sensorId, int32_t pid)
{
    CALL_LOG_ENTER;
    if (sensorId == INVALID_SENSOR_ID) {
        SEN_HILOGE("sensorId is invalid");
        return ERR_NO_INIT;
    }
    if (sensorManager_.IsOtherClientUsingSensor(sensorId, pid)) {
        SEN_HILOGW("other client is using this sensor now, cannot disable");
        return ERR_OK;
    }
    if (interface_.DisableSensor(sensorId) != ERR_OK) {
        SEN_HILOGE("DisableSensor is failed");
        return DISABLE_SENSOR_ERR;
    }
    return sensorManager_.AfterDisableSensor(sensorId);
}

void SensorSuspendPolicy::DoSuspend(const std::shared_ptr<ResourceSchedule::SuspendAppInfo> &info)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> suspendLock(suspendMutex_);
    auto &list = info->GetAppIPCInfoList();
    for (const auto &appInfo : list) {
        std::vector<uint32_t> sensorIdList = clientInfo_.GetSensorIdByPid(appInfo.pid);
        if (sensorIdList.empty()) {
            continue;
        }
        pidSensorIdMap_.emplace(std::make_pair(appInfo.pid, sensorIdList));
        for (auto &sensorId : sensorIdList) {
            if (CheckFreezingSensor(sensorId)) {
                continue;
            }
            auto sensorInfo = clientInfo_.GetCurPidSensorInfo(sensorId, appInfo.pid);
            sensorIdInfoMap_.emplace(std::make_pair(sensorId, sensorInfo));
            DisableSensor(sensorId, appInfo.pid);
        }
    }
}

ErrCode SensorSuspendPolicy::SaveSubscriber(uint32_t sensorId, int64_t samplingPeriodNs,
    int64_t maxReportDelayNs, int32_t pid)
{
    auto ret = sensorManager_.SaveSubscriber(sensorId, pid, samplingPeriodNs, maxReportDelayNs);
    if (ret != ERR_OK) {
        SEN_HILOGE("SaveSubscriber is failed");
        return ret;
    }
    sensorManager_.StartDataReportThread();
    if (!sensorManager_.SetBestSensorParams(sensorId, samplingPeriodNs, maxReportDelayNs)) {
        SEN_HILOGE("SetBestSensorParams is failed");
        clientInfo_.RemoveSubscriber(sensorId, pid);
        return ENABLE_SENSOR_ERR;
    }
    return ret;
}

ErrCode SensorSuspendPolicy::EnableSensor(uint32_t sensorId, int32_t pid, int64_t samplingPeriodNs,
                                          int64_t maxReportDelayNs)
{
    CALL_LOG_ENTER;
    if ((sensorId == INVALID_SENSOR_ID) || (samplingPeriodNs == 0) ||
        ((samplingPeriodNs != 0L) && (maxReportDelayNs / samplingPeriodNs > MAX_EVENT_COUNT))) {
        SEN_HILOGE("sensorId is 0 or maxReportDelayNs exceed the maximum value");
        return ERR_NO_INIT;
    }
    if (clientInfo_.GetSensorState(sensorId) == SENSOR_ENABLED) {
        SEN_HILOGW("sensor has been enabled already");
        auto ret = SaveSubscriber(sensorId, samplingPeriodNs, maxReportDelayNs, pid);
        if (ret != ERR_OK) {
            SEN_HILOGE("SaveSubscriber is failed");
            return ret;
        }
        uint32_t flag = sensorManager_.GetSensorFlag(sensorId);
        ret = flushInfo_.FlushProcess(sensorId, flag, pid, true);
        if (ret != ERR_OK) {
            SEN_HILOGW("ret:%{public}d", ret);
        }
        return ERR_OK;
    }
    auto ret = SaveSubscriber(sensorId, samplingPeriodNs, maxReportDelayNs, pid);
    if (ret != ERR_OK) {
        SEN_HILOGE("SaveSubscriber is failed");
        return ret;
    }
    ret = interface_.EnableSensor(sensorId);
    if (ret != ERR_OK) {
        SEN_HILOGE("EnableSensor is failed");
        clientInfo_.RemoveSubscriber(sensorId, pid);
        return ENABLE_SENSOR_ERR;
    }
    return ret;
}

std::vector<uint32_t> SensorSuspendPolicy::GetSensorIdByPid(int32_t pid)
{
    SEN_HILOGD("pid:%{public}d", pid);
    auto it = pidSensorIdMap_.find(pid);
    if (it != pidSensorIdMap_.end()) {
        SEN_HILOGD("pid:%{public}d found", pid);
        return it->second;
    }
    SEN_HILOGD("pid:%{public}d not found", pid);
    return {};
}

void SensorSuspendPolicy::DoActive(const std::shared_ptr<ResourceSchedule::SuspendAppInfo> &info)
{
    CALL_LOG_ENTER;
    int64_t samplePeriod = DEFAULT_SAMPLEING_RATE;
    int64_t maxReportDelay = DEFAULT_REPORT_DELAY;
    std::vector<uint32_t> sensorIdList;
    std::lock_guard<std::mutex> suspendLock(suspendMutex_);
    auto &list = info->GetAppIPCInfoList();
    for (const auto &appInfo : list) {
        sensorIdList = GetSensorIdByPid(appInfo.pid);
        for (auto &sensorId : sensorIdList) {
            if (CheckFreezingSensor(sensorId)) {
                continue;
            }
            auto it = sensorIdInfoMap_.find(sensorId);
            if (it != sensorIdInfoMap_.end()) {
                samplePeriod = it->second.GetSamplingPeriodNs();
                maxReportDelay = it->second.GetMaxReportDelayNs();
            }
            auto ret = EnableSensor(sensorId, appInfo.pid, samplePeriod, maxReportDelay);
            if (ret != ERR_OK) {
                SEN_HILOGE("sensorId:%{public}u,pid:%{public}d,ret:%{public}d", sensorId, appInfo.pid, ret);
            }
        }
    }
    pidSensorIdMap_.clear();
    sensorIdInfoMap_.clear();
}
}  // namespace Sensors
}  // namespace OHOS
