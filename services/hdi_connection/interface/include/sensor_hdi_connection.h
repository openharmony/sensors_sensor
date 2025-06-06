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

#ifndef SENSOR_HDI_CONNECTION_H
#define SENSOR_HDI_CONNECTION_H

#include<unordered_set>

#include "i_sensor_hdi_connection.h"
#include "singleton.h"

namespace OHOS {
namespace Sensors {
class SensorHdiConnection : public ISensorHdiConnection, public Singleton<SensorHdiConnection> {
public:
    SensorHdiConnection() = default;
    virtual ~SensorHdiConnection() {}
    int32_t ConnectHdi() override;
    int32_t GetSensorList(std::vector<Sensor> &sensorList) override;
    int32_t GetSensorListByDevice(int32_t deviceId, std::vector<Sensor> &singleDevSensors) override;
    int32_t EnableSensor(const SensorDescription &sensorDesc) override;
    int32_t DisableSensor(const SensorDescription &sensorDesc)  override;
    int32_t SetBatch(const SensorDescription &sensorDesc, int64_t samplingInterval, int64_t reportInterval) override;
    int32_t SetMode(const SensorDescription &sensorDesc, int32_t mode) override;
    int32_t RegisterDataReport(ReportDataCb cb, sptr<ReportDataCallback> reportDataCallback) override;
    int32_t DestroyHdiConnection() override;
    int32_t RegSensorPlugCallback(DevicePlugCallback cb) override;
    DevicePlugCallback GetSensorPlugCb() override;
    bool PlugEraseSensorData(SensorPlugInfo info);

private:
    DISALLOW_COPY_AND_MOVE(SensorHdiConnection);
    std::unique_ptr<ISensorHdiConnection> iSensorHdiConnection_ { nullptr };
    std::unique_ptr<ISensorHdiConnection> iSensorCompatibleHdiConnection_ { nullptr };
    std::mutex sensorMutex_;
    std::vector<Sensor> sensorList_;
    std::unordered_set<int32_t> sensorSet_;
    std::unordered_set<int32_t> mockSet_;
    int32_t ConnectHdiService();
    int32_t ConnectCompatibleHdi();
    bool FindAllInSensorSet(const std::unordered_set<int32_t> &sensors);
    bool FindOneInMockSet(int32_t sensorType);
    Sensor GenerateColorSensor();
    Sensor GenerateSarSensor();
    Sensor GenerateHeadPostureSensor();
    Sensor GenerateProximitySensor();
    void UpdataSensorList(std::vector<Sensor> &singleDevSensors);
    std::atomic_bool hdiConnectionStatus_ = false;
};
} // namespace Sensors
} // namespace OHOS
#endif // SENSOR_HDI_CONNECTION_H