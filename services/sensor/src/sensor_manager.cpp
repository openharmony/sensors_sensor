/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "sensor_manager.h"

#include <cinttypes>

#include "iservice_registry.h"
#include "sensor.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "SensorManager" };
constexpr uint32_t INVALID_SENSOR_ID = -1;
constexpr uint32_t PROXIMITY_SENSOR_ID = 50331904;
constexpr float PROXIMITY_FAR = 5.0;
}  // namespace

void SensorManager::InitSensorMap(std::unordered_map<uint32_t, Sensor> &sensorMap,
                                  sptr<SensorDataProcesser> dataProcesser, sptr<ReportDataCallback> dataCallback)
{
    std::lock_guard<std::mutex> sensorLock(sensorMapMutex_);
    sensorMap_.insert(sensorMap.begin(), sensorMap.end());
    sensorDataProcesser_ = dataProcesser;
    reportDataCallback_ = dataCallback;
    SEN_HILOGD("begin sensorMap_.size:%{public}d", int32_t { sensorMap_.size() });
    return;
}

uint32_t SensorManager::GetSensorFlag(uint32_t sensorId)
{
    uint32_t flag = SENSOR_ONE_SHOT;
    auto sensor = sensorMap_.find(sensorId);
    if (sensor != sensorMap_.end()) {
        flag = sensor->second.GetFlags();
    }
    return flag;
}

bool SensorManager::SetBestSensorParams(uint32_t sensorId, int64_t samplingPeriodNs, int64_t maxReportDelayNs)
{
    CALL_LOG_ENTER;
    if (sensorId == INVALID_SENSOR_ID) {
        SEN_HILOGE("sensorId is invalid");
        return false;
    }
    SensorBasicInfo sensorInfo = clientInfo_.GetBestSensorInfo(sensorId);
    int64_t bestSamplingPeriodNs = sensorInfo.GetSamplingPeriodNs();
    int64_t bestReportDelayNs = sensorInfo.GetMaxReportDelayNs();
    if ((samplingPeriodNs > bestSamplingPeriodNs) && (maxReportDelayNs > bestReportDelayNs)) {
        SEN_HILOGD("no need to reset sensor params");
        return true;
    }
    bestSamplingPeriodNs = (samplingPeriodNs < bestSamplingPeriodNs) ? samplingPeriodNs : bestSamplingPeriodNs;
    bestReportDelayNs = (maxReportDelayNs < bestReportDelayNs) ? maxReportDelayNs : bestReportDelayNs;
    SEN_HILOGD("bestSamplingPeriodNs : %{public}" PRId64, bestSamplingPeriodNs);
    auto ret = sensorHdiConnection_.SetBatch(sensorId, bestSamplingPeriodNs, bestReportDelayNs);
    if (ret != ERR_OK) {
        SEN_HILOGE("SetBatch is failed");
        return false;
    }
    return true;
}

bool SensorManager::ResetBestSensorParams(uint32_t sensorId)
{
    CALL_LOG_ENTER;
    if (sensorId == INVALID_SENSOR_ID) {
        SEN_HILOGE("sensorId is invalid");
        return false;
    }
    SensorBasicInfo sensorInfo = clientInfo_.GetBestSensorInfo(sensorId);
    auto ret = sensorHdiConnection_.SetBatch(sensorId,
        sensorInfo.GetSamplingPeriodNs(), sensorInfo.GetMaxReportDelayNs());
    if (ret != ERR_OK) {
        SEN_HILOGE("SetBatch is failed");
        return false;
    }
    return true;
}

SensorBasicInfo SensorManager::GetSensorInfo(uint32_t sensorId, int64_t samplingPeriodNs, int64_t maxReportDelayNs)
{
    CALL_LOG_ENTER;
    SensorBasicInfo sensorInfo;
    std::lock_guard<std::mutex> sensorMapLock(sensorMapMutex_);
    auto it = sensorMap_.find(sensorId);
    if (it == sensorMap_.end()) {
        sensorInfo.SetSamplingPeriodNs(samplingPeriodNs);
        sensorInfo.SetMaxReportDelayNs(maxReportDelayNs);
        sensorInfo.SetSensorState(true);
        SEN_HILOGE("sensorId is invalid");
        return sensorInfo;
    }
    int64_t curSamplingPeriodNs =
        (samplingPeriodNs < it->second.GetMinSamplePeriodNs()) ? it->second.GetMinSamplePeriodNs() : samplingPeriodNs;
    if (it->second.GetMaxSamplePeriodNs() != 0) {
        curSamplingPeriodNs = (samplingPeriodNs > it->second.GetMaxSamplePeriodNs()) ? it->second.GetMaxSamplePeriodNs()
                                                                                     : curSamplingPeriodNs;
    }
    int32_t maxEventCount = it->second.GetFifoMaxEventCount();
    if ((samplingPeriodNs == 0) || (maxEventCount > (INT64_MAX / samplingPeriodNs))) {
        SEN_HILOGE("failed, samplingPeriodNs overflow");
        return sensorInfo;
    }
    int64_t supportDelay = samplingPeriodNs * maxEventCount;
    int64_t curReportDelayNs = (maxReportDelayNs > supportDelay) ? supportDelay : maxReportDelayNs;
    sensorInfo.SetSamplingPeriodNs(curSamplingPeriodNs);
    sensorInfo.SetMaxReportDelayNs(curReportDelayNs);
    sensorInfo.SetSensorState(true);
    return sensorInfo;
}

ErrCode SensorManager::SaveSubscriber(uint32_t sensorId, uint32_t pid, int64_t samplingPeriodNs,
    int64_t maxReportDelayNs)
{
    SensorBasicInfo sensorInfo = GetSensorInfo(sensorId, samplingPeriodNs, maxReportDelayNs);
    auto updateRet = clientInfo_.UpdateSensorInfo(sensorId, pid, sensorInfo);
    if (!updateRet) {
        SEN_HILOGE("UpdateSensorInfo is failed");
        return UPDATE_SENSOR_INFO_ERR;
    }
    return ERR_OK;
}

void SensorManager::StartDataReportThread()
{
    CALL_LOG_ENTER;
    if (!dataThread_.joinable()) {
        SEN_HILOGW("dataThread_ started");
        std::thread secondDataThread(SensorDataProcesser::DataThread, sensorDataProcesser_, reportDataCallback_);
        dataThread_ = std::move(secondDataThread);
    }
}

bool SensorManager::IsOtherClientUsingSensor(uint32_t sensorId, int32_t clientPid)
{
    CALL_LOG_ENTER;
    if (clientInfo_.OnlyCurPidSensorEnabled(sensorId, clientPid)) {
        SEN_HILOGW("Only current client using this sensor");
        return false;
    }
    clientInfo_.ClearCurPidSensorInfo(sensorId, clientPid);
    if (!ResetBestSensorParams(sensorId)) {
        SEN_HILOGW("ResetBestSensorParams is failed");
    }
    SEN_HILOGD("other client is using this sensor");
    return true;
}

ErrCode SensorManager::AfterDisableSensor(uint32_t sensorId)
{
    CALL_LOG_ENTER;
    clientInfo_.ClearSensorInfo(sensorId);
    if (sensorId == PROXIMITY_SENSOR_ID) {
        SensorEvent event;
        auto ret = clientInfo_.GetStoreEvent(sensorId, event);
        if (ret == ERR_OK) {
            SEN_HILOGD("change the default state is far");
            event.data[0] = PROXIMITY_FAR;
            clientInfo_.StoreEvent(event);
        }
    }
    return ERR_OK;
}

void SensorManager::GetPackageName(AccessTokenID tokenId, std::string &packageName)
{
    CALL_LOG_ENTER;
    int32_t tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    switch (tokenType) {
        case ATokenTypeEnum::TOKEN_HAP: {
            HapTokenInfo hapInfo;
            if (AccessTokenKit::GetHapTokenInfo(tokenId, hapInfo) != 0) {
                SEN_HILOGE("get hap token info fail");
                return;
            }
            packageName = hapInfo.bundleName;
            break;
        }
        case ATokenTypeEnum::TOKEN_NATIVE:
        case ATokenTypeEnum::TOKEN_SHELL: {
            NativeTokenInfo tokenInfo;
            if (AccessTokenKit::GetNativeTokenInfo(tokenId, tokenInfo) != 0) {
                SEN_HILOGE("get native token info fail");
                return;
            }
            packageName = tokenInfo.processName;
            break;
        }
        default: {
            SEN_HILOGW("token type not match");
            break;
        }
    }
}
}  // namespace Sensors
}  // namespace OHOS
