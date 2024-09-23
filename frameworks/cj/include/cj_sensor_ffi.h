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

#ifndef OHOS_CJ_SENSOR_FFI_H
#define OHOS_CJ_SENSOR_FFI_H

#include <cstdint>

#include "cj_sensor_visibility.h"
#include "sensor_agent_type.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

typedef struct {
    float latitude;
    float longitude;
    float altitude;
} CLocationOptions;

typedef struct {
    float x;
    float y;
    float z;
    float geomagneticDip;
    float deflectionAngle;
    float levelIntensity;
    float totalIntensity;
} CGeomagneticData;

typedef struct {
    float *head;
    int64_t size;
} CArrFloat32;

typedef struct {
    int32_t x;
    int32_t y;
} CCoordinatesOptions;

typedef struct {
    CArrFloat32 rotation;
    CArrFloat32 inclination;
} CRotationMatrixResponse;

typedef struct CSensor {
    char *sensorName;
    char *vendorName;
    char *firmwareVersion;
    char *hardwareVersion;
    int32_t sensorTypeId;
    float maxRange;
    int64_t minSamplePeriod;
    int64_t maxSamplePeriod;
    float precision;
    float power;
} CSensor;

typedef struct {
    CSensor *head;
    int64_t size;
} CSensorArray;

SENSOR_FFI_EXPORT int32_t FfiSensorSubscribeSensor(int32_t sensorId, int64_t interval,
                                                   void (*callback)(SensorEvent *event));

SENSOR_FFI_EXPORT int32_t FfiSensorUnSubscribeSensor(int32_t sensorId);
SENSOR_FFI_EXPORT CGeomagneticData FfiSensorGetGeomagneticInfo(CLocationOptions location, int64_t timeMillis);
SENSOR_FFI_EXPORT int32_t FfiSensorGetDeviceAltitude(float seaPressure, float currentPressure, float *altitude);
SENSOR_FFI_EXPORT int32_t FfiSensorGetInclination(CArrFloat32 inclinationMatrix, float *inclination);
SENSOR_FFI_EXPORT int32_t FfiSensorGetAngleVariation(CArrFloat32 currentRotationMatrix, CArrFloat32 preRotationMatrix,
                                                     CArrFloat32 *angleChange);
SENSOR_FFI_EXPORT int32_t FfiSensorGetRotationMatrix(CArrFloat32 rotationVector, CArrFloat32 *rotation);
SENSOR_FFI_EXPORT int32_t FfiSensorTransformRotationMatrix(CArrFloat32 inRotationVector,
                                                           CCoordinatesOptions coordinates,
                                                           CArrFloat32 *rotationMatrix);
SENSOR_FFI_EXPORT int32_t FfiSensorGetQuaternion(CArrFloat32 rotationVector, CArrFloat32 *quaternion);
SENSOR_FFI_EXPORT int32_t FfiSensorGetOrientation(CArrFloat32 rotationMatrix, CArrFloat32 *rotationAngle);
SENSOR_FFI_EXPORT int32_t FfiSensorGetRotationMatrixByGravityAndGeomagnetic(CArrFloat32 gravity,
                                                                            CArrFloat32 geomagnetic,
                                                                            CRotationMatrixResponse *rotationMaxtrix);
SENSOR_FFI_EXPORT int32_t FfiSensorGetAllSensors(CSensorArray *sensors);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif /* OHOS_CJ_SENSOR_FFI_H */
/**< @} */
