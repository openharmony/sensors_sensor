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

#ifndef SENSOR_SHAKE_CONTROL_MANAGER_H
#define SENSOR_SHAKE_CONTROL_MANAGER_H

#include <unordered_set>

#include "parameter.h"
#include "security_privacy_manager_plugin.h"
#include "singleton.h"

struct ShakeControlAppInfo {
    std::string bundleName;
    std::string tokenId;
    int32_t userId;
    bool operator == (const ShakeControlAppInfo& other) const
    {
        return bundleName == other.bundleName && tokenId == other.tokenId && userId == other.userId;
    }
};

namespace std {
    template <>
    struct hash<ShakeControlAppInfo> {
        std::size_t operator()(const ShakeControlAppInfo& appInfo) const
        {
            std::size_t h1 = std::hash<std::string>{}(appInfo.bundleName);
            std::size_t h2 = std::hash<std::string>{}(appInfo.tokenId);
            std::size_t h3 = std::hash<int32_t>{}(appInfo.userId);
            return h1 ^ h2 ^ h3;
        }
    };
}

namespace OHOS {
namespace Sensors {
class AppPolicyCallbackImpl : public AppPolicyCallback {
    std::function<void(int32_t)> func_;
public:
    AppPolicyCallbackImpl() = default;
    ~AppPolicyCallbackImpl() override = default;
    explicit AppPolicyCallbackImpl(std::function<void(int32_t)> func) : func_(func) {}
    void OnResult(int32_t result) override
    {
        func_(result);
    }
};
const int32_t INVALID_USERID = 100;
class SensorShakeControlManager {
    DECLARE_DELAYED_SINGLETON(SensorShakeControlManager);
public:
    DISALLOW_COPY_AND_MOVE(SensorShakeControlManager);
    bool Init(std::atomic_bool &shakeControlInitReady);
    void UpdateRegisterShakeSensorControlObserver(std::atomic_bool &shakeControlInitReady);
    bool CheckAppIsNeedControl(const std::string &bundleName, const std::string &tokenId, const int32_t &userId);
    bool CheckAppInfoIsNeedModify(const std::string &bundleName, const std::string &tokenId, const int32_t &userId);
    int32_t GetCurrentUserId();
    int32_t UpdateCurrentUserId();

private:
    void InitShakeSensorControlAppInfos(bool isAutoMonitor);
    void ReportAppSwitchChangeLog(const std::unordered_set<ShakeControlAppInfo> &oldClosedApps,
        const std::unordered_set<ShakeControlAppInfo> &latestClosedApps,
        const std::unordered_set<ShakeControlAppInfo> &latestOpenedApps);
    int32_t RegisterShakeSensorControlObserver(std::atomic_bool &shakeControlInitReady);
    int32_t UnregisterShakeSensorControlObserver();
    std::vector<std::string> GetShakeIgnoreControlList(const std::string &shakeIgnoreControlStr, char delimiter);
    void GetShakeIgnoreControl();
    void OnParameterChanged(const char *key, const char *value, void *context);
    std::mutex shakeSensorControlAppInfoMutex_;
    std::unordered_set<ShakeControlAppInfo> shakeSensorControlAppInfoList_;
    std::unordered_set<ShakeControlAppInfo> shakeSensorNoControlAppInfoList_;
    std::atomic_int32_t currentUserId_ = INVALID_USERID;
    std::vector<std::string> shakeIgnoreControlList_;
    ParameterChgPtr parameterChangedCallback_;
    std::mutex shakeIgnoreControlListMutex_;
};
#define SENSOR_SHAKE_CONTROL_MGR DelayedSingleton<SensorShakeControlManager>::GetInstance()
}  // namespace Sensors
}  // namespace OHOS
#endif  // SENSOR_SHAKE_CONTROL_MANAGER_H