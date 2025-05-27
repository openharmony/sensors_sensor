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

#undef LOG_TAG
#define LOG_TAG "SensorManager"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
constexpr int32_t INVALID_SENSOR_ID = -1;
#endif // HDF_DRIVERS_INTERFACE_SENSOR
constexpr uint32_t PROXIMITY_SENSOR_ID = 50331904;
constexpr float PROXIMITY_FAR = 5.0;
} // namespace

#ifdef HDF_DRIVERS_INTERFACE_SENSOR
void SensorManager::InitSensorMap(const std::unordered_map<SensorDescription, Sensor> &sensorMap,
                                  sptr<SensorDataProcesser> dataProcesser, sptr<ReportDataCallback> dataCallback)
{
    std::lock_guard<std::mutex> sensorLock(sensorMapMutex_);
    sensorMap_.insert(sensorMap.begin(), sensorMap.end());
    sensorDataProcesser_ = dataProcesser;
    reportDataCallback_ = dataCallback;
    SEN_HILOGD("Begin sensorMap_.size:%{public}zu", sensorMap_.size());
}

bool SensorManager::SetBestSensorParams(const SensorDescription &sensorDesc, int64_t samplingPeriodNs,
    int64_t maxReportDelayNs)
{
    SEN_HILOGI("In, sensorType:%{public}d", sensorDesc.sensorType);
    if (sensorDesc.sensorType == INVALID_SENSOR_ID) {
        SEN_HILOGE("sensorType is invalid");
        return false;
    }
    SensorBasicInfo sensorInfo = clientInfo_.GetBestSensorInfo(sensorDesc);
    int64_t bestSamplingPeriodNs = sensorInfo.GetSamplingPeriodNs();
    int64_t bestReportDelayNs = sensorInfo.GetMaxReportDelayNs();
    if ((samplingPeriodNs > bestSamplingPeriodNs) && (maxReportDelayNs > bestReportDelayNs)) {
        SEN_HILOGD("No need to reset sensor params");
        return true;
    }
    bestSamplingPeriodNs = (samplingPeriodNs < bestSamplingPeriodNs) ? samplingPeriodNs : bestSamplingPeriodNs;
    bestReportDelayNs = (maxReportDelayNs < bestReportDelayNs) ? maxReportDelayNs : bestReportDelayNs;
    SEN_HILOGD("bestSamplingPeriodNs : %{public}" PRId64, bestSamplingPeriodNs);
    auto ret = sensorHdiConnection_.SetBatch(sensorDesc, bestSamplingPeriodNs, bestReportDelayNs);
    if (ret != ERR_OK) {
        SEN_HILOGE("SetBatch is failed");
        return false;
    }
    SEN_HILOGI("Done, sensorType:%{public}d", sensorDesc.sensorType);
    return true;
}

bool SensorManager::ResetBestSensorParams(const SensorDescription &sensorDesc)
{
    SEN_HILOGI("In, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    if (sensorDesc.sensorType == INVALID_SENSOR_ID) {
        SEN_HILOGE("sensorType is invalid");
        return false;
    }
    SensorBasicInfo sensorInfo = clientInfo_.GetBestSensorInfo(sensorDesc);
    auto ret = sensorHdiConnection_.SetBatch(sensorDesc,
        sensorInfo.GetSamplingPeriodNs(), sensorInfo.GetMaxReportDelayNs());
    if (ret != ERR_OK) {
        SEN_HILOGE("SetBatch is failed");
        return false;
    }
    SEN_HILOGI("Done, sensorType:%{public}d", sensorDesc.sensorType);
    return true;
}

void SensorManager::StartDataReportThread()
{
    CALL_LOG_ENTER;
    if (!dataThread_.joinable()) {
        SEN_HILOGW("dataThread_ started");
        std::thread dataProcessThread(SensorDataProcesser::DataThread, sensorDataProcesser_, reportDataCallback_);
        dataThread_ = std::move(dataProcessThread);
    }
}
#else
void SensorManager::InitSensorMap(const std::unordered_map<SensorDescription, Sensor> &sensorMap)
{
    std::lock_guard<std::mutex> sensorLock(sensorMapMutex_);
    sensorMap_ = sensorMap;
    SEN_HILOGD("Begin sensorMap_.size:%{public}zu", sensorMap_.size());
}
#endif // HDF_DRIVERS_INTERFACE_SENSOR

bool SensorManager::SaveSubscriber(const SensorDescription &sensorDesc, uint32_t pid, int64_t samplingPeriodNs,
    int64_t maxReportDelayNs)
{
    SEN_HILOGI("In, sensorType:%{public}d, pid:%{public}u", sensorDesc.sensorType, pid);
    SensorBasicInfo sensorInfo = GetSensorInfo(sensorDesc, samplingPeriodNs, maxReportDelayNs);
    if (!clientInfo_.UpdateSensorInfo(sensorDesc, pid, sensorInfo)) {
        SEN_HILOGE("UpdateSensorInfo is failed");
        return false;
    }
    SEN_HILOGI("Done, sensorType:%{public}d, pid:%{public}u", sensorDesc.sensorType, pid);
    return true;
}

SensorBasicInfo SensorManager::GetSensorInfo(const SensorDescription &sensorDesc, int64_t samplingPeriodNs,
    int64_t maxReportDelayNs)
{
    SEN_HILOGI("In, sensorType:%{public}d", sensorDesc.sensorType);
    SensorBasicInfo sensorInfo;
    std::lock_guard<std::mutex> sensorMapLock(sensorMapMutex_);
    auto it = sensorMap_.find(sensorDesc);
    if (it == sensorMap_.end()) {
        sensorInfo.SetSamplingPeriodNs(samplingPeriodNs);
        sensorInfo.SetMaxReportDelayNs(maxReportDelayNs);
        sensorInfo.SetSensorState(true);
        SEN_HILOGE("sensorDesc is invalid");
        return sensorInfo;
    }
    int64_t curSamplingPeriodNs =
        (samplingPeriodNs < it->second.GetMinSamplePeriodNs()) ? it->second.GetMinSamplePeriodNs() : samplingPeriodNs;
    int32_t maxEventCount = it->second.GetFifoMaxEventCount();
    if ((samplingPeriodNs == 0) || (maxEventCount > (INT64_MAX / samplingPeriodNs))) {
        SEN_HILOGE("Failed, samplingPeriodNs overflow");
        return sensorInfo;
    }
    int64_t supportDelay = samplingPeriodNs * maxEventCount;
    int64_t curReportDelayNs = (maxReportDelayNs > supportDelay) ? supportDelay : maxReportDelayNs;
    sensorInfo.SetSamplingPeriodNs(curSamplingPeriodNs);
    sensorInfo.SetMaxReportDelayNs(curReportDelayNs);
    sensorInfo.SetSensorState(true);
    SEN_HILOGI("Done, sensorType:%{public}d", sensorDesc.sensorType);
    return sensorInfo;
}

bool SensorManager::IsOtherClientUsingSensor(const SensorDescription &sensorDesc, int32_t clientPid)
{
    SEN_HILOGI("In, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d, clientPid:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId, clientPid);
    if (clientInfo_.OnlyCurPidSensorEnabled(sensorDesc, clientPid)) {
        SEN_HILOGD("Only current client using this sensor");
        return false;
    }
    clientInfo_.ClearCurPidSensorInfo(sensorDesc, clientPid);
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
    if (!ResetBestSensorParams(sensorDesc)) {
        SEN_HILOGW("ResetBestSensorParams is failed");
    }
#endif // HDF_DRIVERS_INTERFACE_SENSOR
    SEN_HILOGD("Other client is using this sensor");
    SEN_HILOGI("Done, sensorType:%{public}d, clientPid:%{public}d", sensorDesc.sensorType, clientPid);
    return true;
}

ErrCode SensorManager::AfterDisableSensor(const SensorDescription &sensorDesc)
{
    SEN_HILOGI("In, sensorType:%{public}d", sensorDesc.sensorType);
    clientInfo_.ClearSensorInfo(sensorDesc);
    if (sensorDesc.sensorType == PROXIMITY_SENSOR_ID) {
        SensorData sensorData;
        auto ret = clientInfo_.GetStoreEvent(sensorDesc, sensorData);
        if (ret == ERR_OK) {
            SEN_HILOGD("Change the default state is far");
            sensorData.data[0] = PROXIMITY_FAR;
            clientInfo_.StoreEvent(sensorData);
        }
    }
    SEN_HILOGI("Done, sensorType:%{public}d", sensorDesc.sensorType);
    return ERR_OK;
}

void SensorManager::GetPackageName(AccessTokenID tokenId, std::string &packageName, bool isAccessTokenServiceActive)
{
    CALL_LOG_ENTER;
    int32_t tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    switch (tokenType) {
        case ATokenTypeEnum::TOKEN_HAP: {
            HapTokenInfo hapInfo;
            if (AccessTokenKit::GetHapTokenInfo(tokenId, hapInfo) != 0) {
                SEN_HILOGE("Get hap token info fail");
                return;
            }
            packageName = hapInfo.bundleName;
            break;
        }
        case ATokenTypeEnum::TOKEN_NATIVE:
        case ATokenTypeEnum::TOKEN_SHELL: {
            if (!isAccessTokenServiceActive) {
                SEN_HILOGE("Access token service is inactive");
                return;
            }
            NativeTokenInfo tokenInfo;
            if (AccessTokenKit::GetNativeTokenInfo(tokenId, tokenInfo) != 0) {
                SEN_HILOGE("Get native token info fail");
                return;
            }
            packageName = tokenInfo.processName;
            break;
        }
        default: {
            SEN_HILOGW("Token type not match");
            break;
        }
    }
}
} // namespace Sensors
} // namespace OHOS
