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

#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
#include "hisysevent.h"
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

#include "sensor_errors.h"

#undef LOG_TAG
#define LOG_TAG "SensorDataManager"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
const std::string SETTING_COLUMN_KEYWORD = "KEYWORD";
const std::string SETTING_COLUMN_VALUE = "VALUE";
const std::string SETTING_COMPATIBLE_APP_STRATEGY_KEY = "COMPATIBLE_APP_STRATEGY";
const std::string SETTING_URI_PROXY = "datashare:///com.ohos.settingsdata/entry/settingsdata/SETTINGSDATA?Proxy=true";
constexpr const char *SETTINGS_DATA_EXT_URI = "datashare:///com.ohos.settingsdata.DataAbility";
constexpr int32_t DECEM_BASE = 10;
constexpr int32_t DATA_SHARE_READY = 0;
constexpr int32_t DATA_SHARE_NOT_READY = 1055;
}  // namespace

SensorDataManager::SensorDataManager()
{}

SensorDataManager::~SensorDataManager()
{
    remoteObj_ = nullptr;
    if (UnregisterObserver(observer_) != ERR_OK) {
        SEN_HILOGE("UnregisterObserver failed");
    }
}

bool SensorDataManager::Init()
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
    SensorObserver::UpdateFunc updateFunc = [&]() {
        std::string compatibleAppStraegy;
        if (GetStringValue(SETTING_COMPATIBLE_APP_STRATEGY_KEY, compatibleAppStraegy) != ERR_OK) {
            SEN_HILOGE("Get compatible app strategy failed");
        }
        ParseCompatibleAppStragegyList(compatibleAppStraegy);
        SEN_HILOGI("compatibleAppStraegy:%{public}s", compatibleAppStraegy.c_str());
    };
    auto observer_ = CreateObserver(updateFunc);
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

int32_t SensorDataManager::GetIntValue(const std::string &key, int32_t &value)
{
    int64_t valueLong;
    int32_t ret = GetLongValue(key, valueLong);
    if (ret != ERR_OK) {
        return ret;
    }
    value = static_cast<int32_t>(valueLong);
    return ERR_OK;
}

int32_t SensorDataManager::GetLongValue(const std::string &key, int64_t &value)
{
    std::string valueStr;
    int32_t ret = GetStringValue(key, valueStr);
    if (ret != ERR_OK) {
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_EXCEPTION", HiSysEvent::EventType::FAULT,
            "PKG_NAME", "GetStringValue", "ERROR_CODE", ret);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
        SEN_HILOGE("GetStringValue failed, ret:%{public}d", ret);
        return ret;
    }
    value = static_cast<int64_t>(strtoll(valueStr.c_str(), nullptr, DECEM_BASE));
    return ERR_OK;
}

int32_t SensorDataManager::GetStringValue(const std::string &key, std::string &value)
{
    std::string callingIdentity = IPCSkeleton::ResetCallingIdentity();
    auto helper = CreateDataShareHelper(SETTING_URI_PROXY);
    if (helper == nullptr) {
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
    if (ret == DATA_SHARE_READY) {
        return helper;
    } else if (ret == DATA_SHARE_NOT_READY) {
        SEN_HILOGE("Create data_share helper failed, uri proxy:%{public}s", tableUrl.c_str());
        return nullptr;
    }
    SEN_HILOGI("Data_share create unknown");
    return nullptr;
}

bool SensorDataManager::ReleaseDataShareHelper(std::shared_ptr<DataShare::DataShareHelper> &helper)
{
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
    auto uriCompatibleAppStraegy = AssembleUri(SETTING_COMPATIBLE_APP_STRATEGY_KEY);
    helper->RegisterObserver(uriCompatibleAppStraegy, observer);
    helper->NotifyChange(uriCompatibleAppStraegy);
    std::thread execCb(SensorDataManager::ExecRegisterCb, observer);
    execCb.detach();
    ReleaseDataShareHelper(helper);
    IPCSkeleton::SetCallingIdentity(callingIdentity);
    SEN_HILOGI("Succeed to register observer of uri");
    std::string compatibleAppStraegy;
    if (GetStringValue(SETTING_COMPATIBLE_APP_STRATEGY_KEY, compatibleAppStraegy) != ERR_OK) {
        SEN_HILOGE("Get compatible app strategy failed");
    }
    SEN_HILOGI("compatibleAppStraegy:%{public}s", compatibleAppStraegy.c_str());
    ParseCompatibleAppStragegyList(compatibleAppStraegy);
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
    auto uriCompatibleAppStraegy = AssembleUri(SETTING_COMPATIBLE_APP_STRATEGY_KEY);
    helper->UnregisterObserver(uriCompatibleAppStraegy, observer);
    ReleaseDataShareHelper(helper);
    IPCSkeleton::SetCallingIdentity(callingIdentity);
    SEN_HILOGI("Succeed to unregister observer");
    return ERR_OK;
}

void SensorDataManager::ParseCompatibleAppStragegyList(const std::string &compatibleAppStraegy)
{
    {
        std::lock_guard<std::mutex> compatibleAppStraegyLock(compatibleAppStraegyMutex_);
        if (!compatibleAppStragegyList_.empty()) {
            compatibleAppStragegyList_.clear();
        }
    }
    nlohmann::json compatibleAppStragegyJson = nlohmann::json::parse(compatibleAppStraegy, nullptr, false);
    if (compatibleAppStragegyJson.is_discarded()) {
        SEN_HILOGE("Parse json failed");
        return;
    }
    for (auto it = compatibleAppStragegyJson.begin(); it != compatibleAppStragegyJson.end(); ++it) {
        const std::string& key = it.key();
        SEN_HILOGD("key:%{public}s", key.c_str());
        const nlohmann::json& value = it.value();
        std::string name = "";
        bool exemptNaturalDirectionCorrect = false;
        GetJsonValue(value, "name", name);
        if (name.empty()) {
            continue;
        }
        int32_t policy = 0;
        if (value.contains("customLogicDirection")) {
            nlohmann::json policyJson = value.at("customLogicDirection");
            for (auto& [key, valueTmp] : policyJson.items()) {
                if (valueTmp.is_number()) {
                    policy = valueTmp.get<int32_t>();
                }
                SEN_HILOGD("policy:%{public}s", policy);
            }
        }
        GetJsonValue(value, "exemptNaturalDirectionCorrect", exemptNaturalDirectionCorrect);
        CompatibleAppData data;
        if (policy != 0 || exemptNaturalDirectionCorrect) {
            std::lock_guard<std::mutex> compatibleAppStraegyLock(compatibleAppStraegyMutex_);
            data.name = name;
            data.policy = policy;
            SEN_HILOGI("name:%{public}s, policy:%{public}s", name.c_str(), policy);
            compatibleAppStragegyList_.emplace_back(data);
        }
    }
}

std::vector<CompatibleAppData> SensorDataManager::GetCompatibleAppStragegyList()
{
    std::lock_guard<std::mutex> compatibleAppStraegyLock(compatibleAppStraegyMutex_);
    return compatibleAppStragegyList_;
}
}  // namespace Sensors
}  // namespace OHOS