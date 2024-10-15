/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_CJ_SENSOR_IMPL_H
#define OHOS_CJ_SENSOR_IMPL_H

#include <functional>
#include <map>
#include <optional>

#include "cj_sensor_ffi.h"
#include "sensor_agent_type.h"
#include "singleton.h"

namespace OHOS {
namespace Sensors {
using SensorCallbackType = std::function<void(SensorEvent *)>;

class CJSensorImpl {
    DECLARE_DELAYED_SINGLETON(CJSensorImpl);

public:
    DISALLOW_COPY_AND_MOVE(CJSensorImpl);
    int32_t OnSensorChange(int32_t sensorId, int64_t interval, void (*callback)(SensorEvent *event));
    int32_t OffSensorChange(int32_t sensorId);
    void EmitCallBack(SensorEvent *event);

    CGeomagneticData GetGeomagneticInfo(CLocationOptions location, int64_t timeMillis);
    int32_t GetAltitude(float seaPressure, float currentPressure, float *altitude);
    int32_t GetGeomagneticDip(CArrFloat32 inclinationMatrix, float *geomagneticDip);
    int32_t GetAngleModify(const CArrFloat32 &curRotationMatrix, const CArrFloat32 &preRotationMatrix,
                           CArrFloat32 *angleChange);
    int32_t GetRotationMatrix(const CArrFloat32 &rotationCArr, CArrFloat32 &rotation);
    int32_t TransformRotationMatrix(const CArrFloat32 &rotationCArr, int32_t axisX, int32_t axisY,
                                    CArrFloat32 &outRotationMatrix);
    int32_t GetQuaternion(const CArrFloat32 &rotationVector, CArrFloat32 &quaternionOut);
    int32_t GetOrientation(const CArrFloat32 &rotationMatrix, CArrFloat32 &rotationAngleOut);
    int32_t GetRotationMatrixByGraityAndGeomagnetic(const CArrFloat32 gravity, const CArrFloat32 geomagnetic,
                                                    CArrFloat32 &rotationMatrix, CArrFloat32 &inclinationMatrix);
    int32_t GetAllSensorList(CSensorArray &sensorList);

private:
    std::map<int32_t, SensorCallbackType> eventMap_;
    std::mutex mutex_;

    int32_t SubscribeSensorImpl(int32_t sensorId, int64_t interval);
    int32_t UnsubscribeSensorImpl(int32_t sensorTypeId);

    void DelCallback(int32_t type);
    void AddCallback2Map(int32_t type, SensorCallbackType callback);
    std::optional<SensorCallbackType> FindCallback(int32_t type);

    char *MallocCString(const std::string origin);
    void Transform2CSensor(const SensorInfo &in, CSensor &out);
    std::vector<float> ConvertCArr2Vector(const CArrFloat32 &in);
    CArrFloat32 ConvertVector2CArr(const std::vector<float> &in);

    static void CJDataCallbackImpl(SensorEvent *event);
    const SensorUser cjUser_ = {.callback = CJDataCallbackImpl};
};

#define CJ_SENSOR_IMPL OHOS::DelayedSingleton<CJSensorImpl>::GetInstance()

} // namespace Sensors
} // namespace OHOS

#endif // OHOS_CJ_SENSOR_IMPL_H