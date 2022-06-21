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

#include "permission_util.h"

#include <thread>
#include "sensor_agent_type.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
namespace {
const std::string ACCELEROMETER_PERMISSION = "ohos.permission.ACCELEROMETER";
const std::string GYROSCOPE_PERMISSION = "ohos.permission.GYROSCOPE";
const std::string ACTIVITY_MOTION_PERMISSION = "ohos.permission.ACTIVITY_MOTION";
const std::string READ_HEALTH_DATA_PERMISSION = "ohos.permission.READ_HEALTH_DATA";
}  // namespace

std::unordered_map<uint32_t, std::string> PermissionUtil::sensorPermissions_ = {
    { SENSOR_TYPE_ID_ACCELEROMETER, ACCELEROMETER_PERMISSION },
    { SENSOR_TYPE_ID_ACCELEROMETER_UNCALIBRATED, ACCELEROMETER_PERMISSION },
    { SENSOR_TYPE_ID_LINEAR_ACCELERATION, ACCELEROMETER_PERMISSION },
    { SENSOR_TYPE_ID_GYROSCOPE, GYROSCOPE_PERMISSION },
    { SENSOR_TYPE_ID_GYROSCOPE_UNCALIBRATED, GYROSCOPE_PERMISSION },
    { SENSOR_TYPE_ID_PEDOMETER_DETECTION, ACTIVITY_MOTION_PERMISSION },
    { SENSOR_TYPE_ID_PEDOMETER, ACTIVITY_MOTION_PERMISSION },
    { SENSOR_TYPE_ID_HEART_RATE, READ_HEALTH_DATA_PERMISSION }
};

int32_t PermissionUtil::CheckSensorPermission(AccessTokenID callerToken, int32_t sensorTypeId)
{
    if (sensorPermissions_.find(sensorTypeId) == sensorPermissions_.end()) {
        return true;
    }
    std::string permissionName = sensorPermissions_[sensorTypeId];
    return AccessTokenKit::VerifyAccessToken(callerToken, permissionName);
}
}  // namespace Sensors
}  // namespace OHOS
