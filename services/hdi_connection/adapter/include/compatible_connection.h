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

#ifndef COMPATIBLE_CONNECTION_H
#define COMPATIBLE_CONNECTION_H

#include "hdi_service_impl.h"
#include "i_sensor_hdi_connection.h"

namespace OHOS {
namespace Sensors {
class CompatibleConnection : public ISensorHdiConnection {
public:
    CompatibleConnection() = default;
    virtual ~CompatibleConnection() {}
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
    int32_t ConnectSensorTransformHdi() override;
    int32_t TransformSensorData(uint32_t state, uint32_t policy, SensorData* sensorData) override;

private:
    DISALLOW_COPY_AND_MOVE(CompatibleConnection);
    static void ReportSensorDataCallback(SensorEvent *event);
    static ReportDataCb reportDataCb_;
    static sptr<ReportDataCallback> reportDataCallback_;
    static DevicePlugCallback reportPlugDataCb_;
    HdiServiceImpl &hdiServiceImpl_ = HdiServiceImpl::GetInstance();
};
} // namespace Sensors
} // namespace OHOS
#endif // COMPATIBLE_CONNECTION_H