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

#ifndef SENSOR_EVENT_CALLBACK_H
#define SENSOR_EVENT_CALLBACK_H

#include "v3_0/isensor_callback.h"
#include "v3_0/sensor_types.h"

using OHOS::HDI::Sensor::V3_0::HdfSensorEvents;
using OHOS::HDI::Sensor::V3_0::ISensorCallback;

namespace OHOS {
namespace Sensors {
class SensorEventCallback : public ISensorCallback {
public:
    virtual ~SensorEventCallback() {}
    int32_t OnDataEvent(const HdfSensorEvents &event) override;
    int32_t OnDataEventAsync(const std::vector<HdfSensorEvents> &events) override;
};
} // namespace Sensors
} // namespace OHOS
#endif // SENSOR_EVENT_CALLBACK_H
