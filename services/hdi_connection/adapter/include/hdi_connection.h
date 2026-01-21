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
    int32_t EnableSensor(const SensorDescription &sensorDesc) override;
    int32_t DisableSensor(const SensorDescription &sensorDesc)  override;
    int32_t SetBatch(const SensorDescription &sensorDesc, int64_t samplingInterval, int64_t reportInterval) override;
    int32_t SetMode(const SensorDescription &sensorDesc, int32_t mode) override;
    int32_t RegisterDataReport(ReportDataCb cb, sptr<ReportDataCallback> reportDataCallback) override;
    int32_t DestroyHdiConnection() override;
    ReportDataCb GetReportDataCb();
    sptr<ReportDataCallback> GetReportDataCallback();
    void ProcessDeathObserver(const wptr<IRemoteObject> &object);
    int32_t RegSensorPlugCallback(DevicePlugCallback cb) override;
    DevicePlugCallback GetSensorPlugCb() override;
    int32_t ConnectSensorTransformHdi() override;
    int32_t TransformSensorData(uint32_t state, uint32_t policy, SensorData* sensorData) override;

private:
    DISALLOW_COPY_AND_MOVE(HdiConnection);
    static ReportDataCb reportDataCb_;
    static DevicePlugCallback reportPlugDataCb_;
    static sptr<ReportDataCallback> reportDataCallback_;
    sptr<IRemoteObject::DeathRecipient> hdiDeathObserver_ = nullptr;
    bool isRegisterDataCallBack_ = false;
    bool isRegisterPlugCallBack_ = false;
    bool GetHdiInterface();
    bool InitHdiInterface();
    void RegisterHdiDeathRecipient();
    void UnregisterHdiDeathRecipient();
    void Reconnect();
    void ReEnableSensor();
    void UpdateSensorBasicInfo(const SensorDescription &sensorDesc, int64_t samplingPeriodNs, int64_t maxReportDelayNs);
    void SetSensorBasicInfoState(const SensorDescription &sensorDesc, bool state);
    void DeleteSensorBasicInfoState(const SensorDescription &sensorDesc);
};
} // namespace Sensors
} // namespace OHOS
#endif // HDI_CONNECTION_H