/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef MOTION_PLUGIN_H
#define MOTION_PLUGIN_H

#include <string>
#include <stdio.h>
#include <stdlib.h>

#include <dlfcn.h>
#include <unistd.h>

#include "sensor_data_event.h"

namespace OHOS {
namespace Sensors {
#if (defined(__aarch64__) || defined(__x86_64__))
const std::string PLUGIN_SO_PATH = "/system/lib64/libmotion_param.z.so";
#else
const std::string PLUGIN_SO_PATH = "/system/lib/libmotion_param.z.so";
#endif

using MotionTransformIfRequiredPtr = void (*)(const std::string& pkName, uint32_t state, SensorData* sensorData);
bool LoadMotionSensor(void);
void UnloadMotionSensor(void);
void MotionTransformIfRequired(const std::string& pkName, uint32_t state, SensorData* sensorData);
}
}
#endif /* MOTION_PLUGIN_H */