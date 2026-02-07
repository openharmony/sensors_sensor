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

#include "sensor_data_manager.h"

#include <charconv>

#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
#include "hisysevent.h"
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

#include "sensor_errors.h"
#include "sensor_utils.h"

#undef LOG_TAG
#define LOG_TAG "SensorDataManager"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
const std::string SETTING_COLUMN_KEYWORD = "KEYWORD";
const std::string SETTING_COLUMN_VALUE = "VALUE";
const std::string SETTING_COMPATIBLE_APP_STRATEGY_KEY = "COMPATIBLE_APP_STRATEGY";
const std::string SETTING_APP_LOGICAL_DEVICE_CONFIGURATION_KEY = "APP_LOGICAL_DEVICE_CONFIGURATION";
const std::string SETTING_URI_PROXY = "datashare:///com.ohos.settingsdata/entry/settingsdata/SETTINGSDATA?Proxy=true";
constexpr const char *SETTINGS_DATA_EXT_URI = "datashare:///com.ohos.settingsdata.DataAbility";
constexpr int32_t DATA_SHARE_READY = 0;
}  // namespace

SensorDataManager::SensorDataManager()
{}

SensorDataManager::~SensorDataManager()
{
    remoteObj_ = nullptr;
    std::lock_guard<std::mutex> observerMutex(observerMutex_);
    if (UnregisterObserver(observer_) != ERR_OK) {
        SEN_HILOGE("UnregisterObserver failed");
    }
}

bool SensorDataManager::Init(int32_t deviceMode)
{
    auto sm = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (sm == nullptr) {
        SEN_HILOGE("sm cannot be nullptr");
        return false;
    }
    remoteObj_ = sm->GetSystemAbility(SENSOR_SERVICE_ABILITY_ID);
    if (remoteObj_ == nullptr) {
        SEN_HILOGE("GetSystemAbility return nullptr");
        return false;
    }
    deviceMode_.store(deviceMode);
    SensorObserver::UpdateFunc updateFunc = [this]() {
        std::string compatibleAppStrategy;
        int32_t currentDeviceMode = deviceMode_.load();
        if (currentDeviceMode == SINGLE_DISPLAY_THREE_FOLD) {
            if (GetStringValue(SETTING_COMPATIBLE_APP_STRATEGY_KEY, compatibleAppStrategy) != ERR_OK) {
                SEN_HILOGE("Get compatible app strategy failed");
            }
            ParseCompatibleAppStrategyList(compatibleAppStrategy);
        }
        if (currentDeviceMode == SINGLE_DISPLAY_HP_FOLD) {
            if (GetStringValue(SETTING_APP_LOGICAL_DEVICE_CONFIGURATION_KEY, compatibleAppStrategy) != ERR_OK) {
                SEN_HILOGE("Get app logical device configuration failed");
            }
            ParseAppLogicalDeviceList(compatibleAppStrategy);
        }
        SEN_HILOGI("compatibleAppStrategy:%{public}s", compatibleAppStrategy.c_str());
    };
    std::lock_guard<std::mutex> observerMutex(observerMutex_);
    observer_ = CreateObserver(updateFunc);
    if (observer_ == nullptr) {
        SEN_HILOGE("observer is null");
        return false;
    }
    if (RegisterObserver(observer_) != ERR_OK) {
        SEN_HILOGE("RegisterObserver failed");
        return false;
    }
    return true;
}

int32_t SensorDataManager::GetStringValue(const std::string &key, std::string &value)
{
    std::string callingIdentity = IPCSkeleton::ResetCallingIdentity();
    auto helper = CreateDataShareHelper(SETTING_URI_PROXY);
    if (helper == nullptr) {
        SEN_HILOGE("helper is nullptr");
        IPCSkeleton::SetCallingIdentity(callingIdentity);
        return ERROR;
    }
    std::vector<std::string> columns = {SETTING_COLUMN_VALUE};
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(SETTING_COLUMN_KEYWORD, key);
    Uri uri(AssembleUri(key));
    auto resultSet = helper->Query(uri, predicates, columns);
    ReleaseDataShareHelper(helper);
    if (resultSet == nullptr) {
        SEN_HILOGE("resultSet is nullptr");
        IPCSkeleton::SetCallingIdentity(callingIdentity);
        return ERROR;
    }
    int32_t count;
    resultSet->GetRowCount(count);
    if (count == 0) {
        SEN_HILOGW("Not found value, key:%{public}s, count:%{public}d", key.c_str(), count);
        IPCSkeleton::SetCallingIdentity(callingIdentity);
        return ERROR;
    }
    const int32_t index = 0;
    resultSet->GoToRow(index);
    int32_t ret = resultSet->GetString(index, value);
    if (ret != ERR_OK) {
        SEN_HILOGW("GetString failed, ret:%{public}d", ret);
        IPCSkeleton::SetCallingIdentity(callingIdentity);
        return ERROR;
    }
    resultSet->Close();
    IPCSkeleton::SetCallingIdentity(callingIdentity);
    return ERR_OK;
}

sptr<SensorObserver> SensorDataManager::CreateObserver(const SensorObserver::UpdateFunc &func)
{
    sptr<SensorObserver> observer = new SensorObserver();
    if (observer == nullptr) {
        SEN_HILOGE("observer is null");
        return observer;
    }
    observer->SetUpdateFunc(func);
    return observer;
}

Uri SensorDataManager::AssembleUri(const std::string &key)
{
    Uri uri(SETTING_URI_PROXY + "&key=" + key);
    return uri;
}

std::shared_ptr<DataShare::DataShareHelper> SensorDataManager::CreateDataShareHelper(const std::string &tableUrl)
{
    if (remoteObj_ == nullptr) {
        SEN_HILOGE("remoteObj_ is nullptr");
        return nullptr;
    }
    auto [ret, helper] = DataShare::DataShareHelper::Create(remoteObj_, tableUrl, SETTINGS_DATA_EXT_URI);
    if (ret != DATA_SHARE_READY) {
        SEN_HILOGE("Create data_share helper failed, uri proxy:%{public}s", tableUrl.c_str());
        return nullptr;
    }
    SEN_HILOGI("Data_share create success");
    return helper;
}

bool SensorDataManager::ReleaseDataShareHelper(std::shared_ptr<DataShare::DataShareHelper> &helper)
{
    if (helper == nulltpr) {
        SEN_HILOGE("helper is nullptr");
        return false;
    }
    if (!helper->Release()) {
        SEN_HILOGW("Release helper fail");
        return false;
    }
    return true;
}

void SensorDataManager::ExecRegisterCb(const sptr<SensorObserver> &observer)
{
    if (observer == nullptr) {
        SEN_HILOGE("observer is nullptr");
        return;
    }
    observer->OnChange();
}

int32_t SensorDataManager::RegisterObserver(const sptr<SensorObserver> &observer)
{
    if (observer == nullptr) {
        SEN_HILOGE("observer is nullptr");
        return ERR_NO_INIT;
    }
    std::string callingIdentity = IPCSkeleton::ResetCallingIdentity();
    auto helper = CreateDataShareHelper(SETTING_URI_PROXY);
    if (helper == nullptr) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_EXCEPTION", HiSysEvent::EventType::FAULT,
            "PKG_NAME", "RegisterObserver", "ERROR_CODE", ERR_NO_INIT);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        IPCSkeleton::SetCallingIdentity(callingIdentity);
        return ERR_NO_INIT;
    }
    int32_t currentDeviceMode = deviceMode_.load();
    if (currentDeviceMode == SINGLE_DISPLAY_THREE_FOLD) {
        auto uriCompatibleAppStrategy = AssembleUri(SETTING_COMPATIBLE_APP_STRATEGY_KEY);
        helper->RegisterObserver(uriCompatibleAppStrategy, observer);
        helper->NotifyChange(uriCompatibleAppStrategy);
    }
    if (currentDeviceMode == SINGLE_DISPLAY_HP_FOLD) {
        auto uriAppLogicalStrategy = AssembleUri(SETTING_APP_LOGICAL_DEVICE_CONFIGURATION_KEY);
        helper->RegisterObserver(uriAppLogicalStrategy, observer);
        helper->NotifyChange(uriAppLogicalStrategy);
    }
    std::thread execCb(SensorDataManager::ExecRegisterCb, observer);
    execCb.detach();
    ReleaseDataShareHelper(helper);
    IPCSkeleton::SetCallingIdentity(callingIdentity);
    SEN_HILOGI("Succeed to register observer of uri");
    return ERR_OK;
}

int32_t SensorDataManager::UnregisterObserver(const sptr<SensorObserver> &observer)
{
    if (observer == nullptr) {
        SEN_HILOGE("observer is nullptr");
        return ERR_NO_INIT;
    }
    std::string callingIdentity = IPCSkeleton::ResetCallingIdentity();
    auto helper = CreateDataShareHelper(SETTING_URI_PROXY);
    if (helper == nullptr) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_EXCEPTION", HiSysEvent::EventType::FAULT,
            "PKG_NAME", "UnregisterObserver", "ERROR_CODE", ERR_NO_INIT);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        IPCSkeleton::SetCallingIdentity(callingIdentity);
        return ERR_NO_INIT;
    }
    int32_t currentDeviceMode = deviceMode_.load();
    if (currentDeviceMode == SINGLE_DISPLAY_THREE_FOLD) {
        auto uriCompatibleAppStrategy = AssembleUri(SETTING_COMPATIBLE_APP_STRATEGY_KEY);
        helper->UnregisterObserver(uriCompatibleAppStrategy, observer);
    }
    if (currentDeviceMode == SINGLE_DISPLAY_HP_FOLD) {
        auto uriCompatibleAppStrategy = AssembleUri(SETTING_APP_LOGICAL_DEVICE_CONFIGURATION_KEY);
        helper->UnregisterObserver(uriCompatibleAppStrategy, observer);
    }

    ReleaseDataShareHelper(helper);
    IPCSkeleton::SetCallingIdentity(callingIdentity);
    SEN_HILOGI("Succeed to unregister observer");
    return ERR_OK;
}

void SensorDataManager::ParseCompatibleAppStrategyList(const std::string &compatibleAppStrategy)
{
    std::lock_guard<std::mutex> compatibleAppStrategyLock(compatibleAppStrategyMutex_);
    if (!compatibleAppStrategyList_.empty()) {
        compatibleAppStrategyList_.clear();
    }
    nlohmann::json compatibleAppStrategyJson = nlohmann::json::parse(compatibleAppStrategy, nullptr, false);
    if (compatibleAppStrategyJson.is_discarded()) {
        SEN_HILOGE("Parse json failed");
        return;
    }
    for (auto it = compatibleAppStrategyJson.begin(); it != compatibleAppStrategyJson.end(); ++it) {
        const std::string& key = it.key();
        SEN_HILOGD("key:%{public}s", key.c_str());
        const nlohmann::json& value = it.value();
        std::string name = "";
        GetJsonValue(value, "name", name);
        if (name.empty()) {
            continue;
        }
        bool exemptNaturalDirectionCorrect = false;
        GetJsonValue(value, "exemptNaturalDirectionCorrect", exemptNaturalDirectionCorrect);
        CompatibleAppData data;
        if (exemptNaturalDirectionCorrect) {
            data.name = name;
            data.policy = 0;
            SEN_HILOGI("name:%{public}s", name.c_str());
            compatibleAppStrategyList_.emplace_back(data);
        }
    }
}

void SensorDataManager::ParseAppLogicalDeviceList(const std::string &compatibleAppStrategy)
{
    std::lock_guard<std::mutex> compatibleAppStrategyLock(compatibleAppStrategyMutex_);
    if (!compatibleAppStrategyList_.empty()) {
        compatibleAppStrategyList_.clear();
    }
    nlohmann::json compatibleAppStrategyJson = nlohmann::json::parse(compatibleAppStrategy, nullptr, false);
    if (compatibleAppStrategyJson.is_discarded()) {
        SEN_HILOGE("Parse json failed");
        return;
    }
    for (auto it = compatibleAppStrategyJson.begin(); it != compatibleAppStrategyJson.end(); ++it) {
        const std::string& key = it.key();
        SEN_HILOGD("key:%{public}s", key.c_str());
        const nlohmann::json& value = it.value();
        std::string name = "";
        GetJsonValue(value, "name", name);
        if (name.empty()) {
            continue;
        }
        if (!value.contains("useLogicCamera")) {
            continue;
        }
        int32_t isOpenPolicy = ParseJsonValue(value, "useLogicCamera");
        int32_t policy = ParseJsonValue(value, "customLogicDirection");
        CompatibleAppData data;
        if (isOpenPolicy != 0) {
            data.name = name;
            data.policy = policy;
            SEN_HILOGI("name:%{public}s, policy:%{public}d", name.c_str(), policy);
            compatibleAppStrategyList_.emplace_back(data);
        }
    }
}

int32_t SensorDataManager::ParseJsonValue(const nlohmann::json &value, const std::string &strKey)
{
    int32_t tempValue = 0;
    if (value.contains(strKey)) {
        nlohmann::json policyJson = value.at(strKey);
        for (auto& [key, valueTmp] : policyJson.items()) {
            int32_t keyInt = -1;
            auto res = std::from_chars(key.data(), key.data() + key.size(), keyInt);
            if (res.ec != std::errc()) {
                SEN_HILOGE("Failed to convert string %{public}s to number", key.c_str());
                continue;
            }
            auto mode = static_cast<Sensors::FoldDisplayMode>(keyInt);
            if (mode == Sensors::FoldDisplayMode::FULL || mode == Sensors::FoldDisplayMode::GLOBAL_FULL) {
                if (valueTmp.is_number()) {
                    tempValue = valueTmp.get<int32_t>();
                }
            }
            SEN_HILOGI("parse json key:%{public}s, value:%{public}d", strKey.c_str(), tempValue);
        }
    }
    return tempValue;
}

std::vector<CompatibleAppData> SensorDataManager::GetCompatibleAppStrategyList()
{
    std::lock_guard<std::mutex> compatibleAppStrategyLock(compatibleAppStrategyMutex_);
    return compatibleAppStrategyList_;
}
}  // namespace Sensors
}  // namespace OHOS