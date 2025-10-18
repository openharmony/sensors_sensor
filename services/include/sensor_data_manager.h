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

#ifndef SENSOR_DATA_MANAGER_H
#define SENSOR_DATA_MANAGER_H

#include "nlohmann/json.hpp"

#include "datashare_helper.h"
#include "singleton.h"

// #include "sensor_delayed_sp_singleton.h"
#include "sensor_observer.h"

namespace OHOS {
namespace Sensors {
class SensorDataManager {
    DECLARE_DELAYED_SINGLETON(SensorDataManager);
public:
    DISALLOW_COPY_AND_MOVE(SensorDataManager);
    bool Init();
    std::vector<std::string> GetCompatibleAppStragegyList();
    template<typename T>
    static bool GetJsonValue(const nlohmann::json& payload, const std::string& key, T& result)
    {
        if (!payload.contains(key)) {
            return false;
        }
        if constexpr (std::is_same_v<T, std::string>) {
            if (payload[key].is_string()) {
                result = payload[key].get<std::string>();
                return true;
            }
        } else if constexpr (std::is_same_v<T, bool>) {
            if (payload[key].is_boolean()) {
                result = payload[key].get<bool>();
                return true;
            }
        } else if constexpr (std::is_arithmetic_v<T>) {
            if (payload[key].is_number()) {
                result = payload[key].get<int32_t>();
                return true;
            }
        }
        return false;
    }

private:
    static void ExecRegisterCb(const sptr<SensorObserver> &observer);
    int32_t RegisterObserver(const sptr<SensorObserver> &observer);
    int32_t UnregisterObserver(const sptr<SensorObserver> &observer);
    int32_t GetIntValue(const std::string &key, int32_t &value);
    int32_t GetLongValue(const std::string &key, int64_t &value);
    int32_t GetStringValue(const std::string &key, std::string &value);
    Uri AssembleUri(const std::string &key);
    std::shared_ptr<DataShare::DataShareHelper> CreateDataShareHelper(const std::string &tableUrl);
    bool ReleaseDataShareHelper(std::shared_ptr<DataShare::DataShareHelper> &helper);
    sptr<SensorObserver> CreateObserver(const SensorObserver::UpdateFunc &func);
    void ParseCompatibleAppStragegyList(const std::string &compatibleAppStraegy);
    sptr<IRemoteObject> remoteObj_ { nullptr };
    sptr<SensorObserver> observer_ { nullptr };
    std::mutex compatibleAppStraegyMutex_;
    std::vector<std::string> compatibleAppStragegyList_;
};
#define SensorDataMgr DelayedSingleton<SensorDataManager>::GetInstance()
}  // namespace Sensors
}  // namespace OHOS
#endif  // SENSOR_DATA_MANAGER_H