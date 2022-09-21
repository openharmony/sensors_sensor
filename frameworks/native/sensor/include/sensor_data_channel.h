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

#ifndef SENSOR_DATA_CHANNEL_H
#define SENSOR_DATA_CHANNEL_H

#include <memory>
#include <cstdint>

#include "sensor_event_handler.h"
#include "sensor_basic_data_channel.h"

namespace OHOS {
namespace Sensors {
typedef void (*DataChannelCB)(SensorEvent *events, int32_t num, void *data);
class SensorDataChannel : public SensorBasicDataChannel {
public:
    SensorDataChannel() = default;
    ~SensorDataChannel();
    static int32_t HandleEvent(int32_t fd, int32_t events, void *data);
    int32_t CreateSensorDataChannel(DataChannelCB callBack, void *data);
    int32_t DestroySensorDataChannel();
    bool IsThreadExit();
    bool IsThreadStart();
    int32_t RestoreSensorDataChannel();
    int32_t test = 10;
    DataChannelCB dataCB_ = nullptr;
    void *privateData_ = nullptr;

private:
    static void ThreadProcessTask(SensorDataChannel *sensorChannel);
    int32_t InnerSensorDataChannel();
    std::mutex eventRunnerMutex_;
    static std::shared_ptr<SensorEventHandler> eventHandler_;
    static std::shared_ptr<AppExecFwk::EventRunner> eventRunner_;
    static int32_t receiveFd_;
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // SENSOR_DATA_CHANNEL_H
