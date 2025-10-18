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

#ifdef OHOS_BUILD_ENABLE_DO_NOT_DISTURB
#include "cJSON.h"
#endif // OHOS_BUILD_ENABLE_DO_NOT_DISTURB
#include "datashare_helper.h"
#include "singleton.h"

// #include "sensor_delayed_sp_singleton.h"
#include "sensor_observer.h"

namespace OHOS {
namespace Sensors {
enum RingerMode {
    RINGER_MODE_INVALID = -1,
    RINGER_MODE_SILENT = 0,
    RINGER_MODE_VIBRATE = 1,
    RINGER_MODE_NORMAL = 2
};

enum FeedbackMode {
    FEEDBACK_MODE_INVALID = -1,
    FEEDBACK_MODE_OFF = 0,
    FEEDBACK_MODE_ON = 1
};
class SensorDataManager {
    DECLARE_DELAYED_SINGLETON(SensorDataManager);
public:
    DISALLOW_COPY_AND_MOVE(SensorDataManager);
    bool Init();

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
    void UpdateStatus();
    sptr<IRemoteObject> remoteObj_ { nullptr };
    sptr<SensorObserver> observer_ { nullptr };
    std::atomic_int32_t miscFeedback_ = FEEDBACK_MODE_INVALID;
    std::atomic_int32_t miscAudioRingerMode_ = RINGER_MODE_INVALID;
};
#define SensorDataMgr DelayedSingleton<SensorDataManager>::GetInstance()
}  // namespace Sensors
}  // namespace OHOS
#endif  // SENSOR_DATA_MANAGER_H