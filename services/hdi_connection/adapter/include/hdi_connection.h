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

#ifndef HDI_CONNECTION_H
#define HDI_CONNECTION_H

#include "death_recipient_template.h"
#include "i_sensor_hdi_connection.h"
#include "sensor_basic_info.h"

namespace OHOS {
namespace Sensors {
class HdiConnection : public ISensorHdiConnection {
public:
    HdiConnection() = default;
    virtual ~HdiConnection() {}
    int32_t ConnectHdi() override;
    int32_t GetSensorList(std::vector<Sensor> &sensorList) override;
    int32_t GetSensorListByDevice(int32_t deviceId, std::vector<Sensor> &singleDevSensors) override;
    int32_t EnableSensor(SensorDescription sensorDesc) override;
    int32_t DisableSensor(SensorDescription sensorDesc)  override;
    int32_t SetBatch(SensorDescription sensorDesc, int64_t samplingInterval, int64_t reportInterval) override;
    int32_t SetMode(SensorDescription sensorDesc, int32_t mode) override;
    int32_t RegisterDataReport(ReportDataCb cb, sptr<ReportDataCallback> reportDataCallback) override;
    int32_t DestroyHdiConnection() override;
    ReportDataCb GetReportDataCb();
    sptr<ReportDataCallback> GetReportDataCallback();
    void ProcessDeathObserver(const wptr<IRemoteObject> &object);
    int32_t RegSensorPlugCallback(DevicePlugCallback cb) override;
    DevicePlugCallback GetSensorPlugCb() override;

private:
    DISALLOW_COPY_AND_MOVE(HdiConnection);
    static ReportDataCb reportDataCb_;
    static DevicePlugCallback reportPlugDataCb_;
    static sptr<ReportDataCallback> reportDataCallback_;
    sptr<IRemoteObject::DeathRecipient> hdiDeathObserver_ = nullptr;
    void RegisterHdiDeathRecipient();
    void UnregisterHdiDeathRecipient();
    void Reconnect();
    void UpdateSensorBasicInfo(int32_t sensorId, int64_t samplingPeriodNs, int64_t maxReportDelayNs);
    void SetSensorBasicInfoState(int32_t sensorId, bool state);
    void DeleteSensorBasicInfoState(int32_t sensorId);
    void UpdateSensorBasicInfo(SensorDescription sensorDesc, int64_t samplingPeriodNs, int64_t maxReportDelayNs);
    void SetSensorBasicInfoState(SensorDescription sensorDesc, bool state);
    void DeleteSensorBasicInfoState(SensorDescription sensorDesc);
    void GetSensorDescName(SensorDescription sensorDesc, std::string &sensorDescName);
    void ParseIndex(const std::string &sensorDescName, int32_t &deviceId, int32_t &sensorType, int32_t &sensorId,
        int32_t &location);
};
} // namespace Sensors
} // namespace OHOS
#endif // HDI_CONNECTION_H