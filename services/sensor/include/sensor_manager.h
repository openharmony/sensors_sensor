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

#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <mutex>
#include <thread>
#include <unordered_map>

#include "client_info.h"
#include "flush_info_record.h"
#include "sensor_data_processer.h"
#include "sensor_hdi_connection.h"
#include "sensor_agent_type.h"
namespace OHOS {
namespace Sensors {
using namespace Security::AccessToken;
class SensorManager : public Singleton<SensorManager> {
public:
    bool SetBestSensorParams(uint32_t sensorId, int64_t samplingPeriodNs, int64_t maxReportDelayNs);
    bool ResetBestSensorParams(uint32_t sensorId);
    ErrCode SaveSubscriber(uint32_t sensorId, uint32_t pid, int64_t samplingPeriodNs, int64_t maxReportDelayNs);
    void StartDataReportThread();
    SensorBasicInfo GetSensorInfo(uint32_t sensorId, int64_t samplingPeriodNs, int64_t maxReportDelayNs);
    bool IsOtherClientUsingSensor(uint32_t sensorId, int32_t clientPid);
    ErrCode AfterDisableSensor(uint32_t sensorId);
    void InitSensorMap(std::unordered_map<uint32_t, Sensor> &sensorMap, sptr<SensorDataProcesser> dataProcesser,
                       sptr<ReportDataCallback> dataCallback);
    uint32_t GetSensorFlag(uint32_t sensorId);
    void GetPackageName(AccessTokenID tokenId, std::string &packageName);

private:
    SensorHdiConnection &sensorHdiConnection_ = SensorHdiConnection::GetInstance();
    ClientInfo &clientInfo_ = ClientInfo::GetInstance();
    std::thread dataThread_;
    sptr<SensorDataProcesser> sensorDataProcesser_;
    sptr<ReportDataCallback> reportDataCallback_;
    std::unordered_map<uint32_t, Sensor> sensorMap_;
    std::mutex sensorMapMutex_;
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // SENSOR_MANAGER_H
