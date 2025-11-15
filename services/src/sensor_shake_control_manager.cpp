/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "sensor_shake_control_manager.h"

#include "os_account_manager.h"
#include "security_privacy_manager_plugin.h"
#include "sensor_errors.h"

#undef LOG_TAG
#define LOG_TAG "SensorShakeControlManager"

namespace OHOS {
namespace Sensors {

SensorShakeControlManager::SensorShakeControlManager()
{}

SensorShakeControlManager::~SensorShakeControlManager()
{
    UnregisterShakeSensorControlObserver();
}

bool SensorShakeControlManager::Init()
{
    int32_t ret = UpdateCurrentUserId();
    if (ret != ERR_OK) {
        SEN_HILOGE("UpdateCurrentUserId failed ret:%{public}d", ret);
        return false;
    }
    ret = RegisterShakeSensorControlObserver();
    if (ret != ERR_OK) {
        SEN_HILOGE("RegisterShakeSensorControlObserver failed ret:%{public}d", ret);
        return false;
    }
    return true;
}

void SensorShakeControlManager::InitShakeSensorControlAppInfos()
{
    SEN_HILOGI("InitShakeSensorControlAppInfos start");
    std::lock_guard<std::mutex> shakeSensorControlAppInfoLock(shakeSensorControlAppInfoMutex_);
    std::vector<AppPolicyEventExt> appPolicyEventList;
    int32_t ret = QueryAppPolicyByPolicyName(currentUserId_.load(), PolicyName::MOTION_SENSOR,
        appPolicyEventList);
    if (ret != ERR_OK) {
        SEN_HILOGE("QueryAppPolicyByPolicyName failed, ret::%{public}d", ret);
        return;
    }
    if (appPolicyEventList.empty()) {
        SEN_HILOGI("appPolicyEventList empty");
        shakeSensorControlAppInfoList_.clear();
        shakeSensorNoControlAppInfoList_.clear();
        return;
    }
    shakeSensorControlAppInfoList_.clear();
    shakeSensorNoControlAppInfoList_.clear();
    size_t vecLength = appPolicyEventList.size();
    for (size_t i = 0; i < vecLength; i++) {
        ShakeControlAppInfo appInfo;
        appInfo.bundleName = appPolicyEventList[i].bundleName;
        appInfo.tokenId = appPolicyEventList[i].objectId;
        appInfo.userId = currentUserId_.load();
        if (appPolicyEventList[i].policyValue == PolicyValue::CLOSE) {
            shakeSensorControlAppInfoList_.push_back(appInfo);
        } else {
            shakeSensorNoControlAppInfoList_.push_back(appInfo);
        }
    }
}

int32_t SensorShakeControlManager::UpdateCurrentUserId()
{
    std::vector<int32_t> activeUserIds;
    int ret = AccountSA::OsAccountManager::QueryActiveOsAccountIds(activeUserIds);
    if (ret != ERR_OK) {
        SEN_HILOGE("QueryActiveOsAccountIds failed ret:%{public}d", ret);
        return ret;
    }
    if (activeUserIds.empty()) {
        SEN_HILOGE("activeUserIds empty");
        return ERROR;
    }
    currentUserId_ = activeUserIds[0];
    SEN_HILOGI("currentUserId_ is %{public}d", currentUserId_.load());
    return ERR_OK;
}

int32_t SensorShakeControlManager::RegisterShakeSensorControlObserver()
{
    SEN_HILOGI("RegisterShakeSensorControlObserver start");
    std::function<void()> updateFunc = [&]() { InitShakeSensorControlAppInfos(); };
    int32_t ret = RegisterAppPolicyObserver(currentUserId_.load(), updateFunc);
    if (ret != ERR_OK) {
        SEN_HILOGE("RegisterAppPolicyObserver failed, ret::%{public}d", ret);
        return ret;
    }
    InitShakeSensorControlAppInfos();
    return ret;
}

int32_t SensorShakeControlManager::UnregisterShakeSensorControlObserver()
{
    SEN_HILOGI("UnregisterShakeSensorControlObserver start");
    int32_t ret = UnregisterAppPolicyObserver(currentUserId_.load());
    if (ret != ERR_OK) {
        SEN_HILOGE("UnregisterAppPolicyObserver failed, ret::%{public}d", ret);
    }
    return ret;
}

void SensorShakeControlManager::UpdateRegisterShakeSensorControlObserver()
{
    SEN_HILOGI("UpdateRegisterShakeSensorControlObserver start");
    UnregisterShakeSensorControlObserver();
    UpdateCurrentUserId();
    RegisterShakeSensorControlObserver();
}

bool SensorShakeControlManager::CheckAppIsNeedControl(const std::string &bundleName,
    const std::string &tokenId, const int32_t &userId)
{
    SEN_HILOGD("CheckAppIsNeedControl start");
    std::lock_guard<std::mutex> shakeSensorControlAppInfoLock(shakeSensorControlAppInfoMutex_);
    if (!shakeSensorControlAppInfoList_.empty()) {
        size_t vecLength = shakeSensorControlAppInfoList_.size();
        for (size_t i = 0; i < vecLength; i++) {
            ShakeControlAppInfo tempData = shakeSensorControlAppInfoList_[i];
            if (bundleName == tempData.bundleName && tokenId == tempData.tokenId && userId == tempData.userId) {
                SEN_HILOGD("Shake the sensor data for control, bundleName:%{public}s", bundleName.c_str());
                return true;
            }
        }
    }
    return false;
}

bool SensorShakeControlManager::CheckAppInfoIsNeedModify(const std::string &bundleName,
    const std::string &tokenId, const int32_t &userId)
{
    SEN_HILOGD("CheckAppInfoIsNeedModify start");
    std::lock_guard<std::mutex> shakeSensorControlAppInfoLock(shakeSensorControlAppInfoMutex_);
    if (!shakeSensorNoControlAppInfoList_.empty()) {
        size_t vecLength = shakeSensorNoControlAppInfoList_.size();
        for (size_t i = 0; i < vecLength; i++) {
            ShakeControlAppInfo tempData = shakeSensorNoControlAppInfoList_[i];
            if (bundleName == tempData.bundleName && tokenId == tempData.tokenId && userId == tempData.userId) {
                SEN_HILOGD("The app info no need modify, bundleName:%{public}s", bundleName.c_str());
                return false;
            }
        }
    }
    if (!shakeSensorControlAppInfoList_.empty()) {
        size_t vecLength = shakeSensorControlAppInfoList_.size();
        for (size_t i = 0; i < vecLength; i++) {
            ShakeControlAppInfo tempData = shakeSensorControlAppInfoList_[i];
            if (bundleName == tempData.bundleName && tokenId == tempData.tokenId && userId == tempData.userId) {
                SEN_HILOGD("The app info no need modify, bundleName:%{public}s", bundleName.c_str());
                return false;
            }
        }
    }
    return true;
}

int32_t SensorShakeControlManager::GetCurrentUserId()
{
    return currentUserId_.load();
}
}  // namespace Sensors
}  // namespace OHOS