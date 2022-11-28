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

#ifndef SENSOR_PROXY_H
#define SENSOR_PROXY_H

#include <map>
#include <thread>

#include "refbase.h"

#include "sensor_agent_type.h"
#include "sensor_data_channel.h"

namespace OHOS {
namespace Sensors {
struct SensorNativeData;
struct SensorIdList;
typedef int32_t (*SensorDataCallback)(struct SensorNativeData *events, uint32_t num);

struct SensorAgentProxy : public OHOS::RefBase {
public:
    SensorAgentProxy();
    ~SensorAgentProxy();
    static const SensorAgentProxy *GetSensorsObj();
    int32_t ActivateSensor(int32_t sensorId, const SensorUser *user) const;
    int32_t DeactivateSensor(int32_t sensorId, const SensorUser *user) const;
    int32_t SetBatch(int32_t sensorId, const SensorUser *user, int64_t samplingInterval, int64_t reportInterval) const;
    int32_t SubscribeSensor(int32_t sensorId, const SensorUser *user) const;
    int32_t UnsubscribeSensor(int32_t sensorId, const SensorUser *user) const;
    int32_t SetMode(int32_t sensorId, const SensorUser *user, int32_t mode) const;
    int32_t SetOption(int32_t sensorId, const SensorUser *user, int32_t option) const;
    int32_t GetAllSensors(SensorInfo **sensorInfo, int32_t *count) const;

private:
    int32_t CreateSensorDataChannel() const;
    int32_t DestroySensorDataChannel() const;
    static void HandleSensorData(SensorEvent *events, int32_t num, void *data);
    int32_t ConvertSensorInfos() const;
    void ClearSensorInfos() const;
    static OHOS::sptr<SensorAgentProxy> sensorObj_;
    static std::recursive_mutex subscribeMutex_;
    static std::mutex chanelMutex_;
    OHOS::sptr<OHOS::Sensors::SensorDataChannel> dataChannel_;
    static bool g_isChannelCreated;
    static int64_t g_samplingInterval;
    static int64_t g_reportInterval;
    static std::map<int32_t, const SensorUser *> g_subscribeMap;
    static std::map<int32_t, const SensorUser *> g_unsubscribeMap;
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // endif SENSOR_PROXY_H
