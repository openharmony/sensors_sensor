/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef SENSOR_JS_H
#define SENSOR_JS_H
#include "sensor_agent.h"
namespace OHOS {
namespace Sensors {
static bool UnsubscribeSensor(int32_t sensorTypeId);
static void DataCallbackImpl(SensorEvent *event);
static bool SubscribeSensor(int32_t sensorTypeId, int64_t interval, RecordSensorCallback callback);
}  // namespace Sensors
}  // namespace OHOS
#endif // SENSOR_JS_H