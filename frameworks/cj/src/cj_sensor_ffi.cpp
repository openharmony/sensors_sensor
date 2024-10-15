/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cj_sensor_ffi.h"

#include "cj_sensor_impl.h"
#include "sensor_errors.h"

using OHOS::Sensors::CJSensorImpl;
using OHOS::Sensors::PARAMETER_ERROR;

extern "C" {
int32_t FfiSensorSubscribeSensor(int32_t sensorId, int64_t interval, void (*callback)(SensorEvent *event))
{
    return CJ_SENSOR_IMPL->OnSensorChange(sensorId, interval, callback);
}

int32_t FfiSensorUnSubscribeSensor(int32_t sensorId)
{
    return CJ_SENSOR_IMPL->OffSensorChange(sensorId);
}

CGeomagneticData FfiSensorGetGeomagneticInfo(CLocationOptions location, int64_t timeMillis)
{
    return CJ_SENSOR_IMPL->GetGeomagneticInfo(location, timeMillis);
}

int32_t FfiSensorGetDeviceAltitude(float seaPressure, float currentPressure, float *altitude)
{
    return CJ_SENSOR_IMPL->GetAltitude(seaPressure, currentPressure, altitude);
}

int32_t FfiSensorGetInclination(CArrFloat32 inclinationMatrix, float *geomagneticDip)
{
    return CJ_SENSOR_IMPL->GetGeomagneticDip(inclinationMatrix, geomagneticDip);
}

int32_t FfiSensorGetAngleVariation(CArrFloat32 currentRotationMatrix, CArrFloat32 preRotationMatrix,
                                   CArrFloat32 *angleChange)
{
    return CJ_SENSOR_IMPL->GetAngleModify(currentRotationMatrix, preRotationMatrix, angleChange);
}

int32_t FfiSensorGetRotationMatrix(CArrFloat32 rotationVector, CArrFloat32 *rotation)
{
    if (rotation == nullptr) {
        SEN_HILOGE("Invalid parameter, rotation is nullptr!");
        return PARAMETER_ERROR;
    }

    return CJ_SENSOR_IMPL->GetRotationMatrix(rotationVector, *rotation);
}

int32_t FfiSensorTransformRotationMatrix(CArrFloat32 inRotationVector, CCoordinatesOptions coordinates,
                                         CArrFloat32 *rotationMatrix)
{
    if (rotationMatrix == nullptr) {
        SEN_HILOGE("Invalid parameter, rotationMatrix is nullptr!");
        return PARAMETER_ERROR;
    }

    return CJ_SENSOR_IMPL->TransformRotationMatrix(inRotationVector, coordinates.x, coordinates.y, *rotationMatrix);
}

int32_t FfiSensorGetQuaternion(CArrFloat32 rotationVector, CArrFloat32 *quaternion)
{
    if (quaternion == nullptr) {
        SEN_HILOGE("Invalid parameter, quaternion is nullptr!");
        return PARAMETER_ERROR;
    }

    return CJ_SENSOR_IMPL->GetQuaternion(rotationVector, *quaternion);
}

int32_t FfiSensorGetOrientation(CArrFloat32 rotationMatrix, CArrFloat32 *rotationAngle)
{
    if (rotationAngle == nullptr) {
        SEN_HILOGE("Invalid parameter, rotationAngle is nullptr!");
        return PARAMETER_ERROR;
    }

    return CJ_SENSOR_IMPL->GetOrientation(rotationMatrix, *rotationAngle);
}

int32_t FfiSensorGetRotationMatrixByGravityAndGeomagnetic(CArrFloat32 gravity, CArrFloat32 geomagnetic,
                                                          CRotationMatrixResponse *rotationMaxtrix)
{
    if (rotationMaxtrix == nullptr) {
        SEN_HILOGE("Invalid parameter, rotationMaxtrix is nullptr!");
        return PARAMETER_ERROR;
    }

    return CJ_SENSOR_IMPL->GetRotationMatrixByGraityAndGeomagnetic(gravity, geomagnetic, rotationMaxtrix->rotation,
                                                                   rotationMaxtrix->inclination);
}

int32_t FfiSensorGetAllSensors(CSensorArray *sensors)
{
    if (sensors == nullptr) {
        SEN_HILOGE("Invalid parameter, sensors is nullptr!");
        return PARAMETER_ERROR;
    }

    return CJ_SENSOR_IMPL->GetAllSensorList(*sensors);
}
}