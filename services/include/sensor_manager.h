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

#include <thread>

#ifdef HDF_DRIVERS_INTERFACE_SENSOR
#include "sensor_data_processer.h"
#include "sensor_hdi_connection.h"
#else
#include "sensor.h"
#endif // HDF_DRIVERS_INTERFACE_SENSOR

namespace OHOS {
namespace Sensors {
using namespace Security::AccessToken;
class SensorManager : public Singleton<SensorManager> {
public:
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
    void InitSensorMap(const std::unordered_map<std::string, Sensor> &sensorMap,
        sptr<SensorDataProcesser> dataProcesser, sptr<ReportDataCallback> dataCallback);
    bool SetBestSensorParams(SensorDescription sensorDesc, int64_t samplingPeriodNs, int64_t maxReportDelayNs);
    bool ResetBestSensorParams(SensorDescription sensorDesc);
    void StartDataReportThread();
#else
    void InitSensorMap(const std::unordered_map<std::string, Sensor> &sensorMap);
#endif // HDF_DRIVERS_INTERFACE_SENSOR
    bool SaveSubscriber(SensorDescription sensorDesc, uint32_t pid, int64_t samplingPeriodNs,
        int64_t maxReportDelayNs);
    SensorBasicInfo GetSensorInfo(SensorDescription sensorDesc, int64_t samplingPeriodNs, int64_t maxReportDelayNs);
    bool IsOtherClientUsingSensor(SensorDescription sensorDesc, int32_t clientPid);
    ErrCode AfterDisableSensor(SensorDescription sensorDesc);
    void GetPackageName(AccessTokenID tokenId, std::string &packageName, bool isAccessTokenServiceActive = false);
    void UpdataSensorMap(SensorPlugData sensorPlugData);

private:
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
    SensorHdiConnection &sensorHdiConnection_ = SensorHdiConnection::GetInstance();
    std::thread dataThread_;
    sptr<SensorDataProcesser> sensorDataProcesser_ = nullptr;
    sptr<ReportDataCallback> reportDataCallback_ = nullptr;
#endif // HDF_DRIVERS_INTERFACE_SENSOR
    ClientInfo &clientInfo_ = ClientInfo::GetInstance();
    std::unordered_map<std::string, Sensor> sensorMap_;
    std::mutex sensorMapMutex_;
};
} // namespace Sensors
} // namespace OHOS
#endif // SENSOR_MANAGER_H
