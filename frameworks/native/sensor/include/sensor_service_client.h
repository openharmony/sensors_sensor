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

#ifndef SENSOR_SERVICE_CLIENT_H
#define SENSOR_SERVICE_CLIENT_H

#include <map>
#include <vector>

#include "iservice_registry.h"
#include "singleton.h"

#include "sensor_agent_type.h"
#include "sensor_basic_data_channel.h"
#include "sensor_basic_info.h"
#include "sensor_client_stub.h"
#include "sensor_data_channel.h"
#include "sensor.h"
#include "sensor_service_proxy.h"


namespace OHOS {
namespace Sensors {
class SensorServiceClient : public Singleton<SensorServiceClient> {
public:
    std::vector<Sensor> GetSensorList();
    int32_t EnableSensor(uint32_t sensorId, int64_t samplingPeroid, int64_t maxReportDelay);
    int32_t DisableSensor(uint32_t sensorId);
    int32_t TransferDataChannel(sptr<SensorDataChannel> sensorDataChannel);
    int32_t DestroyDataChannel();
    void ProcessDeathObserver(const wptr<IRemoteObject> &object);
    bool IsValid(uint32_t sensorId);

private:
    int32_t InitServiceClient();
    void UpdateSensorInfoMap(uint32_t sensorId, int64_t samplingPeroid, int64_t maxReportDelay);
    void DeleteSensorInfoItem(uint32_t sensorId);
    std::mutex clientMutex_;
    sptr<IRemoteObject::DeathRecipient> serviceDeathObserver_;
    sptr<ISensorService> sensorServer_;
    std::vector<Sensor> sensorList_;
    sptr<SensorDataChannel> dataChannel_;
    sptr<SensorClientStub> sensorClientStub_;
    std::mutex mapMutex_;
    std::map<uint32_t, SensorBasicInfo> sensorInfoMap_;
};
}  // namespace Sensors
}  // namespace OHOS

#endif  // SENSOR_SERVICE_CLIENT_H
