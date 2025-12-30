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

#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
#include "hisysevent.h"
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
#include "os_account_manager.h"
#include "parameters.h"
#include "sensor_errors.h"

#undef LOG_TAG
#define LOG_TAG "SensorShakeControlManager"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
static constexpr int32_t SHAKE_CONTROL_SWITCH_CLOSE = 0;
static constexpr int32_t SHAKE_CONTROL_SWITCH_OPEN = 1;
static const std::string SHAKE_IGNORE_CONTROL_KEY = "security.privacy_indicator.shake_ignore_control";

SensorShakeControlManager::SensorShakeControlManager()
{}

SensorShakeControlManager::~SensorShakeControlManager()
{
    UnregisterShakeSensorControlObserver();
    {
        std::lock_guard<std::mutex> shakeIgnoreControlLock(shakeIgnoreControlListMutex_);
        if (parameterChangedCallback_ !=nullptr) {
            int32_t ret = RemoveParameterWatcher(SHAKE_IGNORE_CONTROL_KEY.c_str(), parameterChangedCallback_, nullptr);
            if (ret != ERR_OK) {
                SEN_HILOGE("RemoveParameterWatcher failed");
            }
            parameterChangedCallback_ = nullptr;
        }
    }
}

void SensorShakeControlManager::OnParameterChanged(const char *key, const char *value, void *context)
{ // LCOV_EXCL_START
    if (key == nullptr || value == nullptr) {
        SEN_HILOGW("key or value is null");
        return;
    }
    std::string paramKey(key);
    std::string paramValue(value);
    if (paramKey.empty() || paramValue.empty()) {
        SEN_HILOGW("paramKey or paramValue is empty");
        return;
    }
    std::lock_guard<std::mutex> shakeIgnoreControlLock(shakeIgnoreControlListMutex_);
    shakeIgnoreControlList_.clear();
    SEN_HILOGD("key:%{public}s, value:%{public}s", key, value);
    char delimiter = ',';
    shakeIgnoreControlList_ = GetShakeIgnoreControlList(paramValue, delimiter);
} // LCOV_EXCL_STOP

void SensorShakeControlManager::RegisterShakeControlParameter()
{ // LCOV_EXCL_START
    std::lock_guard<std::mutex> shakeIgnoreControlLock(shakeIgnoreControlListMutex_);
    GetShakeIgnoreControl();
    parameterChangedCallback_ = [](const char *key, const char *value, void *context) {
        SENSOR_SHAKE_CONTROL_MGR->OnParameterChanged(key, value, context);
    };
    int32_t ret = WatchParameter(SHAKE_IGNORE_CONTROL_KEY.c_str(), parameterChangedCallback_, nullptr);
    if (ret != ERR_OK) {
        SEN_HILOGE("WatchParameter failed, ret:%{public}d", ret);
    }
} // LCOV_EXCL_STOP

bool SensorShakeControlManager::Init(std::atomic_bool &shakeControlInitReady)
{
    int32_t ret = UpdateCurrentUserId();
    if (ret != ERR_OK) {
        SEN_HILOGE("UpdateCurrentUserId failed ret:%{public}d", ret);
        return false;
    }
    ret = RegisterShakeSensorControlObserver(shakeControlInitReady);
    if (ret != ERR_OK) {
        SEN_HILOGE("RegisterShakeSensorControlObserver failed ret:%{public}d", ret);
        return false;
    }
    return true;
}

void SensorShakeControlManager::InitShakeSensorControlAppInfos(bool isAutoMonitor)
{
    SEN_HILOGI("InitShakeSensorControlAppInfos start");
    std::vector<AppPolicyEventExt> appPolicyEventList;
    int32_t ret = QueryAppPolicyByPolicyName(currentUserId_.load(), PolicyName::MOTION_SENSOR,
        appPolicyEventList);
    if (ret != ERR_OK) {
        SEN_HILOGE("QueryAppPolicyByPolicyName failed, ret::%{public}d", ret);
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "QueryAppPolicyByPolicyName", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        return;
    }
    std::lock_guard<std::mutex> shakeSensorControlAppInfoLock(shakeSensorControlAppInfoMutex_);
    if (appPolicyEventList.empty()) {
        SEN_HILOGI("appPolicyEventList empty");
        shakeSensorControlAppInfoList_.clear();
        shakeSensorNoControlAppInfoList_.clear();
        return;
    }
    std::unordered_set<ShakeControlAppInfo> oldClosedApps(shakeSensorControlAppInfoList_.begin(),
        shakeSensorControlAppInfoList_.end());
    shakeSensorControlAppInfoList_.clear();
    shakeSensorNoControlAppInfoList_.clear();
    size_t vecLength = appPolicyEventList.size();
    int32_t tempCurrentUserId = currentUserId_.load();
    for (size_t i = 0; i < vecLength; i++) {
        ShakeControlAppInfo appInfo = {appPolicyEventList[i].bundleName, appPolicyEventList[i].objectId,
            tempCurrentUserId};
        if (appPolicyEventList[i].policyValue == PolicyValue::CLOSE) {
            shakeSensorControlAppInfoList_.insert(appInfo);
        } else {
            shakeSensorNoControlAppInfoList_.insert(appInfo);
        }
    }
    if (isAutoMonitor) {
        ReportAppSwitchChangeLog(oldClosedApps, shakeSensorControlAppInfoList_, shakeSensorNoControlAppInfoList_);
    }
    SEN_HILOGI("InitShakeSensorControlAppInfos end");
}

void SensorShakeControlManager::ReportAppSwitchChangeLog(const std::unordered_set<ShakeControlAppInfo> &oldClosedApps,
    const std::unordered_set<ShakeControlAppInfo> &latestClosedApps,
    const std::unordered_set<ShakeControlAppInfo> &latestOpenedApps)
{
    std::vector<ShakeControlAppInfo> openSwitchApps;
    std::vector<ShakeControlAppInfo> closeSwitchApps;
    for (const auto& item : oldClosedApps) {
        if (latestClosedApps.find(item) == latestClosedApps.end()) {
            openSwitchApps.push_back(item);
        }
    }
    for (const auto& item : latestClosedApps) {
        if (oldClosedApps.find(item) == oldClosedApps.end()) {
            closeSwitchApps.push_back(item);
        }
    }
    for (const auto& item : closeSwitchApps) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_EXCEPTION", HiSysEvent::EventType::FAULT,
            "PKG_NAME", item.bundleName, "ERROR_CODE", SHAKE_CONTROL_SWITCH_CLOSE);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
    }
    for (const auto& item : openSwitchApps) {
        if (latestOpenedApps.find(item) != latestOpenedApps.end()) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
            HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_EXCEPTION", HiSysEvent::EventType::FAULT,
                "PKG_NAME", item.bundleName, "ERROR_CODE", SHAKE_CONTROL_SWITCH_OPEN);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
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
    currentUserId_.store(activeUserIds[0]);
    SEN_HILOGI("currentUserId_ is %{public}d", currentUserId_.load());
    return ERR_OK;
}

int32_t SensorShakeControlManager::RegisterShakeSensorControlObserver(std::atomic_bool &shakeControlInitReady)
{
    SEN_HILOGI("RegisterShakeSensorControlObserver start");
    std::shared_ptr<AppPolicyCallbackImpl> createCallBack = std::make_shared<AppPolicyCallbackImpl>([this,
        &shakeControlInitReady] (int32_t result) {
        if (result != ERR_OK) {
            SEN_HILOGE("CreateAppPolicyDB failed, result:%{public}d", result);
            shakeControlInitReady.store(false);
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
            HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_EXCEPTION",
                HiSysEvent::EventType::FAULT, "PKG_NAME", "CreateAppPolicyDB", "ERROR_CODE", result);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
            return;
        }
        std::function<void()> updateFunc = [&]() { this->InitShakeSensorControlAppInfos(true); };
        int32_t ret = RegisterAppPolicyObserver(currentUserId_.load(), updateFunc);
        if (ret != ERR_OK) {
            SEN_HILOGE("RegisterAppPolicyObserver failed, ret::%{public}d", ret);
            shakeControlInitReady.store(false);
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
            HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_EXCEPTION",
                HiSysEvent::EventType::FAULT, "PKG_NAME", "RegisterAppPolicyObserver", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
            return;
        }
        this->InitShakeSensorControlAppInfos(false);
        shakeControlInitReady.store(true);
    });
    int32_t ret = CreateAppPolicyDB(currentUserId_.load(), createCallBack);
    if (ret != ERR_OK) {
        SEN_HILOGE("CreateAppPolicyDB failed, ret::%{public}d", ret);
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "CreateAppPolicyDB", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
    }
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

void SensorShakeControlManager::UpdateRegisterShakeSensorControlObserver(std::atomic_bool &shakeControlInitReady)
{
    SEN_HILOGI("UpdateRegisterShakeSensorControlObserver start");
    UnregisterShakeSensorControlObserver();
    UpdateCurrentUserId();
    RegisterShakeSensorControlObserver(shakeControlInitReady);
}

bool SensorShakeControlManager::CheckAppIsNeedControl(const std::string &bundleName,
    const std::string &tokenId, const int32_t &userId)
{
    SEN_HILOGD("CheckAppIsNeedControl start");
    {
        std::lock_guard<std::mutex> shakeIgnoreControlLock(shakeIgnoreControlListMutex_);
        if (shakeIgnoreControlList_.find(tokenId) != shakeIgnoreControlList_.end()) {
            SEN_HILOGD("Shake ignore control, bundleName:%{public}s", bundleName.c_str());
            return false;
        }
    }
    std::lock_guard<std::mutex> shakeSensorControlAppInfoLock(shakeSensorControlAppInfoMutex_);
    if (!shakeSensorControlAppInfoList_.empty()) {
        ShakeControlAppInfo appInfo = {bundleName, tokenId, userId};
        if (shakeSensorControlAppInfoList_.find(appInfo) != shakeSensorControlAppInfoList_.end()) {
            SEN_HILOGD("Shake the sensor data for control, bundleName:%{public}s", bundleName.c_str());
            return true;
        }
    }
    return false;
}

std::unordered_set<std::string> SensorShakeControlManager::GetShakeIgnoreControlList(
    const std::string &shakeIgnoreControlStr, char delimiter)
{ // LCOV_EXCL_START
    std::unordered_set<std::string> result;
    std::stringstream ss(shakeIgnoreControlStr);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        std::ostringstream noTrimToken;
        for (char c : token) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                noTrimToken << c;
            }
        }
        if (!noTrimToken.str().empty()) {
            auto status = result.insert(noTrimToken.str());
            if (!status.second) {
                SEN_HILOGE("tokenId insert faild");
            }
        }
    }
    return result;
} // LCOV_EXCL_STOP

void SensorShakeControlManager::GetShakeIgnoreControl()
{ // LCOV_EXCL_START
    CALL_LOG_ENTER;
    std::string shakeIgnoreControlStr
        = OHOS::system::GetParameter(SHAKE_IGNORE_CONTROL_KEY, "");
    char delimiter = ',';
    SEN_HILOGI("shakeIgnoreControlStr:%{public}s", shakeIgnoreControlStr.c_str());
    std::lock_guard<std::mutex> shakeIgnoreControlLock(shakeIgnoreControlListMutex_);
    shakeIgnoreControlList_ = GetShakeIgnoreControlList(shakeIgnoreControlStr, delimiter);
} // LCOV_EXCL_STOP

bool SensorShakeControlManager::CheckAppInfoIsNeedModify(const std::string &bundleName,
    const std::string &tokenId, const int32_t &userId)
{
    SEN_HILOGD("CheckAppInfoIsNeedModify start");
    std::lock_guard<std::mutex> shakeSensorControlAppInfoLock(shakeSensorControlAppInfoMutex_);
    if (!shakeSensorNoControlAppInfoList_.empty()) {
        ShakeControlAppInfo appInfo = {bundleName, tokenId, userId};
        if (shakeSensorNoControlAppInfoList_.find(appInfo) != shakeSensorNoControlAppInfoList_.end()) {
            SEN_HILOGD("The app info no need modify, bundleName:%{public}s", bundleName.c_str());
            return false;
        }
    }
    if (!shakeSensorControlAppInfoList_.empty()) {
        ShakeControlAppInfo appInfo = {bundleName, tokenId, userId};
        if (shakeSensorControlAppInfoList_.find(appInfo) != shakeSensorControlAppInfoList_.end()) {
            SEN_HILOGD("The app info no need modify, bundleName:%{public}s", bundleName.c_str());
            return false;
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