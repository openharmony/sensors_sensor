/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

/**
 * @addtogroup Sensor
 * @{
 *
 * @brief Provides APIs to define common sensor attributes.
 *
 * @since 11
 */

/**
 * @file native_sensor_type.h
 *
 * @brief Declares the common sensor attributes.
 * @library libsensor_native.z.so
 *
 * @since 11
 */

#ifndef OH_NATIVE_SENSOR_TYPE_H
#define OH_NATIVE_SENSOR_TYPE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enumerates the sensor types.
 *
 * @since 11
 */
typedef enum Sensor_SensorType {
    /**
     * Acceleration sensor.
     * @since 11
     */
    SENSOR_TYPE_ACCELEROMETER = 1,
    /**
     * Gyroscope sensor.
     * @since 11
     */
    SENSOR_TYPE_GYROSCOPE = 2,
    /**
     * Ambient light sensor.
     * @since 11
     */
    SENSOR_TYPE_AMBIENT_LIGHT = 5,
    /**
     * Magnetic field sensor.
     * @since 11
     */
    SENSOR_TYPE_MAGNETIC_FIELD = 6,
    /**
     * Barometer sensor.
     * @since 11
     */
    SENSOR_TYPE_BAROMETER = 8,
    /**
     * Hall effect sensor.
     * @since 11
     */
    SENSOR_TYPE_HALL = 10,
    /**
     * Proximity sensor.
     * @since 11
     */
    SENSOR_TYPE_PROXIMITY = 12,
    /**
     * Orientation sensor.
     * @since 11
     */
    SENSOR_TYPE_ORIENTATION = 256,
    /**
     * Gravity sensor.
     * @since 11
     */
    SENSOR_TYPE_GRAVITY = 257,
    /**
     * Rotation vector sensor.
     * @since 11
     */
    SENSOR_TYPE_ROTATION_VECTOR = 259,
    /**
     * Pedometer detection sensor.
     * @since 11
     */
    SENSOR_TYPE_PEDOMETER_DETECTION = 265,
    /**
     * Pedometer sensor.
     * @since 11
     */
    SENSOR_TYPE_PEDOMETER = 266,
    /**
     * Heart rate sensor.
     * @since 11
     */
    SENSOR_TYPE_HEART_RATE = 278,
} Sensor_SensorType;

/**
 * @brief Enumerates the sensor result codes.
 *
 * @since 11
 */
typedef enum Sensor_Result {
    /**
     * The operation is successful.
     * @since 11
     */
    SENSOR_SUCCESS = 0,
    /**
     * Permission verification failed.
     * @since 11
     */
    SENSOR_PERMISSION_DENIED = 201,
    /**
     * Parameter check failed. For example, a mandatory parameter is not passed in,
     * or the parameter type passed in is incorrect.
     * @since 11
     */
    SENSOR_PARAMETER_ERROR = 401,
    /**
     * The sensor service is abnormal.
     * @since 11
     */
    SENSOR_SERVICE_EXCEPTION = 14500101,
} Sensor_Result;

/**
 * @brief Enumerates the accuracy levels of data reported by a sensor.
 *
 * @since 11
 */
typedef enum Sensor_SensorAccuracy {
    /**
     * The sensor data is unreliable. It is possible that the sensor does not contact with the device to measure.
     * @since 11
     */
    SENSOR_ACCURACY_UNRELIABLE = 0,
    /**
     * The sensor data is at a low accuracy level. The data must be calibrated based on the environment before being used.
     * @since 11
     */
    SENSOR_ACCURACY_LOW = 1,
    /**
     * The sensor data is at a medium accuracy level. You are advised to calibrate the data based on the environment before using it.
     * @since 11
     */
    SENSOR_ACCURACY_MEDIUM = 2,
    /**
     * The sensor data is at a high accuracy level. The data can be used directly.
     * @since 11
     */
    SENSOR_ACCURACY_HIGH = 3
} Sensor_SensorAccuracy;

/**
 * @brief Defines the sensor information.
 * @since 11
 */
typedef struct Sensor_Sensor Sensor_Sensor;

/**
 * @brief 创建给定个数{@link Sensor_Sensor}实例的数组。
 *
 * @param count - 表示要创建{@link Sensor_Sensor}实例的个数.
 * @return 如果创建成功，则返回{@link Sensor_Sensor}实例的数组，否则返回 NULL。。
 * @since 11
 */
Sensor_Sensor **OH_Sensor_CreateSensors(uint32_t count);

/**
 * @brief 销毁{@link Sensor_Sensor}实例的数组并回收内存。
 *
 * @param sensors - 表示要销毁的{@link Sensor_Sensor}实例的数组.
 * @param count - 表示要销毁的{@link Sensor_Sensor}实例的个数.
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 * @since 11
 */
int32_t OH_Sensor_DestroySensors(Sensor_Sensor **sensors, uint32_t count);

/**
 * @brief Obtains the sensor name.
 *
 * @param sensor - Pointer to the sensor information.
 * @param sensorName - Pointer to the sensor name.
 * @param length - Pointer to the length, in bytes.
 * @return Returns <b>SENSOR_SUCCESS</b> if the operation is successful;
 * returns an error code defined in {@link Sensor_Result} otherwise.
 * @since 11
 */
int32_t OH_Sensor_GetSensorName(Sensor_Sensor* sensor, char *sensorName, uint32_t *length);

/**
 * @brief Obtains the sensor's vendor name.
 *
 * @param sensor - Pointer to the sensor information.
 * @param vendorName - Pointer to the vendor name.
 * @param length - Pointer to the length, in bytes.
 * @return Returns <b>SENSOR_SUCCESS</b> if the operation is successful;
 * returns an error code defined in {@link Sensor_Result} otherwise.
 * @since 11
 */
int32_t OH_Sensor_GetVendorName(Sensor_Sensor* sensor, char *vendorName, uint32_t *length);

/**
 * @brief Obtains the sensor type.
 *
 * @param sensor - Pointer to the sensor information.
 * @param sensorType - Pointer to the sensor type.
 * @return Returns <b>SENSOR_SUCCESS</b> if the operation is successful;
 * returns an error code defined in {@link Sensor_Result} otherwise.
 * @since 11
 */
int32_t OH_Sensor_GetSensorType(Sensor_Sensor* sensor, Sensor_SensorType *sensorType);

/**
 * @brief Obtains the sensor resolution.
 *
 * @param sensor - Pointer to the sensor information.
 * @param resolution - Pointer to the sensor resolution.
 * @return Returns <b>SENSOR_SUCCESS</b> if the operation is successful;
 * returns an error code defined in {@link Sensor_Result} otherwise.
 * @since 11
 */
int32_t OH_Sensor_GetSensorResolution(Sensor_Sensor* sensor, float *resolution);

/**
 * @brief Obtains the minimum data reporting interval of a sensor.
 *
 * @param sensor - Pointer to the sensor information.
 * @param minSamplePeriod - Pointer to the minimum data reporting interval, in nanoseconds.
 * @return Returns <b>SENSOR_SUCCESS</b> if the operation is successful;
 * returns an error code defined in {@link Sensor_Result} otherwise.
 * @since 11
 */
int32_t OH_Sensor_GetSensorMinSamplePeriod(Sensor_Sensor* sensor, int64_t *minSamplePeriod);

/**
 * @brief Obtains the maximum data reporting interval of a sensor.
 *
 * @param sensor - Pointer to the sensor information.
 * @param maxSamplePeriod - Pointer to the maximum data reporting interval, in nanoseconds.
 * @return Returns <b>SENSOR_SUCCESS</b> if the operation is successful;
 * returns an error code defined in {@link Sensor_Result} otherwise.
 * @since 11
 */
int32_t OH_Sensor_GetSensorMaxSamplePeriod(Sensor_Sensor* sensor, int64_t *maxSamplePeriod);

/**
 * @brief Defines the sensor data information.
 * @since 11
 */
typedef struct Sensor_SensorEvent Sensor_SensorEvent;

/**
 * @brief Obtains the sensor type.
 *
 * @param sensorEvent - Pointer to the sensor data information.
 * @param sensorType - Pointer to the sensor type.
 * @return Returns <b>SENSOR_SUCCESS</b> if the operation is successful;
 * returns an error code defined in {@link Sensor_Result} otherwise.
 * @since 11
 */
int32_t OH_Sensor_GetEventSensorType(Sensor_SensorEvent* sensorEvent, Sensor_SensorType *sensorType);

/**
 * @brief Obtains the timestamp of sensor data.
 *
 * @param sensorEvent - Pointer to the sensor data information.
 * @param timestamp - Pointer to the timestamp.
 * @return Returns <b>SENSOR_SUCCESS</b> if the operation is successful;
 * returns an error code defined in {@link Sensor_Result} otherwise.
 * @since 11
 */
int32_t OH_Sensor_GetEventTimestamp(Sensor_SensorEvent* sensorEvent, int64_t *timestamp);

/**
 * @brief Obtains the accuracy of sensor data.
 *
 * @param sensorEvent - Pointer to the sensor data information.
 * @param accuracy - Pointer to the accuracy.
 * @return Returns <b>SENSOR_SUCCESS</b> if the operation is successful;
 * returns an error code defined in {@link Sensor_Result} otherwise.
 * @since 11
 */
int32_t OH_Sensor_GetEventAccuracy(Sensor_SensorEvent* sensorEvent, Sensor_SensorAccuracy *accuracy);

/**
 * @brief Obtains sensor data. The data length and content depend on the sensor type.
 * The format of the sensor data reported is as follows:
 * SENSOR_TYPE_ACCELEROMETER: data[0], data[1], and data[2], indicating the acceleration around
 * the x, y, and z axes of the device, respectively, in m/s2.
 * SENSOR_TYPE_GYROSCOPE: data[0], data[1], and data[2], indicating the angular velocity of rotation around
 *  the x, y, and z axes of the device, respectively, in rad/s.
 * SENSOR_TYPE_AMBIENT_LIGHT: data[0], indicating the ambient light intensity, in lux.
 * SENSOR_TYPE_MAGNETIC_FIELD: data[0], data[1], and data[2], indicating the magnetic field strength around
 * the x, y, and z axes of the device, respectively, in μT.
 * SENSOR_TYPE_BAROMETER: data[0], indicating the atmospheric pressure, in hPa.
 * SENSOR_TYPE_HALL: data[0], indicating the opening/closing state of the flip cover. The value <b>0</b> means that
 * the flip cover is opened, and a value greater than <b>0</b> means that the flip cover is closed.
 * SENSOR_TYPE_PROXIMITY: data[0], indicates the approaching state. The value <b>0</b> means the two objects are close
 * to each other, and a value greater than <b>0</b> means that they are far away from each other.
 * SENSOR_TYPE_ORIENTATION: data[0], data[1], and data[2], indicating the rotation angles of a device around
 * the z, x, and y axes, respectively, in degree.
 * SENSOR_TYPE_GRAVITY: data[0], data[1], and data[2], indicating the gravitational acceleration around
 * the x, y, and z axes of a device, respectively, in m/s2.
 * SENSOR_TYPE_ROTATION_VECTOR: data[0], data[1] and data[2], indicating the rotation angles of a device around
 * the x, y, and z axes, respectively, in degree. data[3] indicates the rotation vector.
 * SENSOR_TYPE_PEDOMETER_DETECTION: data[0], indicating the pedometer detection status.
 * The value <b>1</b> means that the number of detected steps changes.
 * SENSOR_TYPE_PEDOMETER: data[0], indicating the number of steps a user has walked.
 * SENSOR_TYPE_HEART_RATE: data[0], indicating the heart rate value.
 *
 * @param sensorEvent - Pointer to the sensor data information.
 * @param data - Double pointer to the sensor data.
 * @param length - Pointer to the array length.
 * @return Returns <b>SENSOR_SUCCESS</b> if the operation is successful;
 * returns an error code defined in {@link Sensor_Result} otherwise.
 * @since 11
 */
int32_t OH_Sensor_GetSensorData(Sensor_SensorEvent* sensorEvent, float **data, uint32_t *length);

/**
 * @brief Defines the sensor subscription ID, which uniquely identifies a sensor.
 * @since 11
 */
typedef struct Sensor_SensorSubscribeId Sensor_SensorSubscribeId;

/**
 * @brief 创建指向{@link Sensor_SensorSubscribeId}实例的指针。
 *
 * @return 如果创建成功，则返回指向{@link Sensor_SensorSubscribeId} 结构实例的指针，否则返回NULL.
 * @since 11
 */
Sensor_SensorSubscribeId *OH_Sensor_CreateSubscribeId();

/**
 * @brief 销毁{@link Sensor_SensorSubscribeId}实例并回收内存。
 *
 * @param subscribeId - 表示指向 {@link Sensor_SensorSubscribeId} 实例的指针.
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 * @since 11
 */
int32_t OH_Sensor_DestroySubscribeId(Sensor_SensorSubscribeId *subscribeId);

/**
 * @brief Obtains the sensor type.
 *
 * @param subscribeId - Pointer to the sensor subscription ID.
 * @param sensorType - Pointer to the sensor type.
 * @return Returns <b>SENSOR_SUCCESS</b> if the operation is successful;
 * returns an error code defined in {@link Sensor_Result} otherwise.
 * @since 11
 */
int32_t OH_Sensor_GetSubscribeSensorType(Sensor_SensorSubscribeId* subscribeId, Sensor_SensorType *sensorType);

/**
 * @brief Sets the sensor type.
 *
 * @param subscribeId - Pointer to the sensor subscription ID.
 * @param sensorType - Sensor type to set.
 * @return Returns <b>SENSOR_SUCCESS</b> if the operation is successful;
 * returns an error code defined in {@link Sensor_Result} otherwise.
 * @since 11
 */
int32_t OH_Sensor_SetSubscribeSensorType(Sensor_SensorSubscribeId* subscribeId, const Sensor_SensorType sensorType);

/**
 * @brief Defines the sensor subscription attribute.
 * @since 11
 */
typedef struct Sensor_SubscribeAttribute Sensor_SubscribeAttribute;

/**
 * @brief 创建指向{@link Sensor_SubscribeAttribute}实例的指针。
 *
 * @return 如果创建成功，则返回指向{@link Sensor_SubscribeAttribute} 结构实例的指针，否则返回NULL.
 * @since 11
 */
Sensor_SubscribeAttribute *OH_Sensor_CreateAttribute();

/**
 * @brief 销毁{@link Sensor_SubscribeAttribute}实例并回收内存。
 *
 * @param attribute - 表示指向 {@link Sensor_SubscribeAttribute} 实例的指针.
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 * @since 11
 */
int32_t OH_Sensor_DestroyAttribute(Sensor_SubscribeAttribute *attribute);

/**
 * @brief Sets the sensor data reporting interval.
 *
 * @param attribute - Pointer to the sensor subscription attribute.
 * @param samplingInterval - Data reporting interval to set, in nanoseconds.
 * @return Returns <b>SENSOR_SUCCESS</b> if the operation is successful;
 * returns an error code defined in {@link Sensor_Result} otherwise.
 * @since 11
 */
int32_t OH_Sensor_SetSamplingInterval(Sensor_SubscribeAttribute* attribute, const int64_t samplingInterval);

/**
 * @brief Obtains the sensor data reporting interval.
 *
 * @param attribute - Pointer to the sensor subscription attribute.
 * @param samplingInterval - Pointer to the data reporting interval, in nanoseconds.
 * @return Returns <b>SENSOR_SUCCESS</b> if the operation is successful;
 * returns an error code defined in {@link Sensor_Result} otherwise.
 * @since 11
 */
int32_t OH_Sensor_GetSamplingInterval(Sensor_SubscribeAttribute* attribute, int64_t *samplingInterval);

/**
 * @brief Defines the callback function used to report sensor data.
 * @since 11
 */
typedef void (*Sensor_RecordSensorCallback)(Sensor_SensorEvent *event);

/**
 * @brief Defines the sensor subscriber information.
 * @since 11
 */
typedef struct Sensor_SubscribeUser Sensor_SubscribeUser;

/**
 * @brief 创建指向{@link Sensor_SubscribeUser}实例的指针。
 *
 * @return 如果创建成功，则返回指向{@link Sensor_SubscribeUser} 结构实例的指针，否则返回NULL.
 * @since 11
 */
Sensor_SubscribeUser *OH_Sensor_CreateUser();

/**
 * @brief 销毁{@link Sensor_SubscribeUser}实例并回收内存。
 *
 * @param user - 表示指向{@link Sensor_SubscribeUser}实例的指针.
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 * @since 11
 */
int32_t OH_Sensor_DestroyUser(Sensor_SubscribeUser *user);

/**
 * @brief Sets a callback function to report sensor data.
 *
 * @param user - Pointer to the sensor subscriber information.
 * @param callback - Callback function to set.
 * @return Returns <b>SENSOR_SUCCESS</b> if the operation is successful;
 * returns an error code defined in {@link Sensor_Result} otherwise.
 * @since 11
 */
int32_t OH_Sensor_SetSensorCallback(Sensor_SubscribeUser* user, const Sensor_RecordSensorCallback callback);

/**
 * @brief Obtains the callback function used to report sensor data.
 *
 * @param user - Pointer to the sensor subscriber information.
 * @param callback - Pointer to the callback function.
 * @return Returns <b>SENSOR_SUCCESS</b> if the operation is successful;
 * returns an error code defined in {@link Sensor_Result} otherwise.
 * @since 11
 */
int32_t OH_Sensor_GetSensorCallback(Sensor_SubscribeUser* user, Sensor_RecordSensorCallback *callback);
#ifdef __cplusplus
}
#endif
#endif // OH_NATIVE_SENSOR_TYPE_H