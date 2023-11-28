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
 * @brief 为您提供标准的开放api，定义常用传感器属性。
 *
 * @since 11
 */

/**
 * @file native_sensor_type.h
 *
 * @brief 定义常用传感器属性。
 * @library libohsensor.z.so
 * @syscap SystemCapability.Sensors.Sensor
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
 * @brief 传感器类型。
 * @syscap SystemCapability.Sensors.Sensor
 * 
 * @since 11
 */
typedef enum Sensor_SensorType {
    /**
     * 加速度传感器
     * @syscap SystemCapability.Sensors.Sensor
     * @since 11
     */
    SENSOR_TYPE_ACCELEROMETER = 1,
    /**
     * 陀螺仪传感测器
     * @syscap SystemCapability.Sensors.Sensor
     * @since 11
     */
    SENSOR_TYPE_GYROSCOPE = 2,
    /**
     * 环境光传感器
     * @syscap SystemCapability.Sensors.Sensor
     * @since 11
     */
    SENSOR_TYPE_AMBIENT_LIGHT = 5,
    /**
     * 地磁传感器
     * @syscap SystemCapability.Sensors.Sensor
     * @since 11
     */
    SENSOR_TYPE_MAGNETIC_FIELD = 6,
    /**
     * 气压计传感器
     * @syscap SystemCapability.Sensors.Sensor
     * @since 11
     */
    SENSOR_TYPE_BAROMETER = 8,
    /**
     * 霍尔传感器
     * @syscap SystemCapability.Sensors.Sensor
     * @since 11
     */
    SENSOR_TYPE_HALL = 10,
    /**
     * 接近光传感器
     * @syscap SystemCapability.Sensors.Sensor
     * @since 11
     */
    SENSOR_TYPE_PROXIMITY = 12,
    /**
     * 方向传感器
     * @syscap SystemCapability.Sensors.Sensor
     * @since 11
     */
    SENSOR_TYPE_ORIENTATION = 256,
    /**
     * 重力传感器
     * @syscap SystemCapability.Sensors.Sensor
     * @since 11
     */
    SENSOR_TYPE_GRAVITY = 257,
    /**
     * 旋转矢量传感器
     * @syscap SystemCapability.Sensors.Sensor
     * @since 11
     */
    SENSOR_TYPE_ROTATION_VECTOR = 259,
    /**
     * 计步器检测传感器
     * @syscap SystemCapability.Sensors.Sensor
     * @since 11
     */
    SENSOR_TYPE_PEDOMETER_DETECTION = 265,
    /**
     * 计步器传感器
     * @syscap SystemCapability.Sensors.Sensor
     * @since 11
     */
    SENSOR_TYPE_PEDOMETER = 266,
    /**
     * 心率传感器
     * @syscap SystemCapability.Sensors.Sensor
     * @since 11
     */
    SENSOR_TYPE_HEART_RATE = 278,
} Sensor_SensorType;

/**
 * @brief 定义传感器错误码。
 * @syscap SystemCapability.Sensors.Sensor
 *
 * @since 11
 */
typedef enum Sensor_Result {
    /**
     * 操作成功。
     * @syscap SystemCapability.Sensors.Sensor
     * @since 11
     */
    SENSOR_SUCCESS = 0,
    /**
     * Permission verification failed.
     * @syscap SystemCapability.Sensors.Sensor
     * @since 11
     */
    SENSOR_PERMISSION_DENIED = 201,
    /**
     * Parameter check failed. For example, a mandatory parameter is not passed in,
     * or the parameter type passed in is incorrect.
     * @syscap SystemCapability.Sensors.Sensor
     * @since 11
     */
    SENSOR_PARAMETER_ERROR = 401,
    /**
     * 传感器服务异常。
     * @syscap SystemCapability.Sensors.Sensor
     * @since 11
     */
    SENSOR_SERVICE_EXCEPTION = 14500101,
} Sensor_Result;

/**
 * @brief Enumerates the accuracy levels of data reported by a sensor.
 * @syscap SystemCapability.Sensors.Sensor
 *
 * @since 11
 */
typedef enum Sensor_SensorAccuracy {
    /**
     * The sensor data is unreliable. It is possible that the sensor does not contact with the device to measure.
     * @syscap SystemCapability.Sensors.Sensor
     * @since 11
     */
    SENSOR_ACCURACY_UNRELIABLE = 0,
    /**
     * The sensor data is at a low accuracy level. The data must be calibrated based on the environment before being used.
     * @syscap SystemCapability.Sensors.Sensor
     * @since 11
     */
    SENSOR_ACCURACY_LOW = 1,
    /**
     * The sensor data is at a medium accuracy level. You are advised to calibrate the data based on the environment before using it.
     * @syscap SystemCapability.Sensors.Sensor
     * @since 11
     */
    SENSOR_ACCURACY_MEDIUM = 2,
    /**
     * The sensor data is at a high accuracy level. The data can be used directly.
     * @syscap SystemCapability.Sensors.Sensor
     * @since 11
     */
    SENSOR_ACCURACY_HIGH = 3
} Sensor_SensorAccuracy;

/**
 * @brief 传感器信息。
 * @syscap SystemCapability.Sensors.Sensor
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
 * @brief 获取传感器名称.
 *
 * @param sensor - 传感器信息.
 * @param sensorName - 出参，传感器名称.
 * @param length - 出参，字节数.
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 * @since 11
 */
int32_t OH_Sensor_GetSensorName(Sensor_Sensor* sensor, char *sensorName, uint32_t *length);

/**
 * @brief 获取传感器厂商名称.
 *
 * @param sensor - 传感器信息.
 * @param vendorName - 出参，厂商名称.
 * @param length - 出参，字节数.
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 * @since 11
 */
int32_t OH_Sensor_GetVendorName(Sensor_Sensor* sensor, char *vendorName, uint32_t *length);

/**
 * @brief 获取传感器类型。
 *
 * @param sensor - 传感器信息。
 * @param sensorType - 出参，传感器类型。
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 * @since 11
 */
int32_t OH_Sensor_GetSensorType(Sensor_Sensor* sensor, Sensor_SensorType *sensorType);

/**
 * @brief 获取传感器分辨率。
 *
 * @param sensor - 传感器信息。
 * @param resolution - 出参，传感器分辨率。
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 * @since 11
 */
int32_t OH_Sensor_GetSensorResolution(Sensor_Sensor* sensor, float *resolution);

/**
 * @brief 获取传感器最小上报时间间隔。
 *
 * @param sensor - 传感器信息。
 * @param minSamplePeriod - 出参，传感器最小上报时间间隔，单位纳秒。
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 * @since 11
 */
int32_t OH_Sensor_GetSensorMinSamplePeriod(Sensor_Sensor* sensor, int64_t *minSamplePeriod);

/**
 * @brief 获取传感器最大上报时间间隔。
 *
 * @param sensor - 传感器信息。
 * @param maxSamplePeriod - 出参，传感器最大上报时间间隔，单位纳秒。
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 * @since 11
 */
int32_t OH_Sensor_GetSensorMaxSamplePeriod(Sensor_Sensor* sensor, int64_t *maxSamplePeriod);

/**
 * @brief 传感器数据信息。
 * @syscap SystemCapability.Sensors.Sensor
 * @since 11
 */
typedef struct Sensor_SensorEvent Sensor_SensorEvent;

/**
 * @brief 获取传感器类型。
 *
 * @param sensor - 传感器数据信息。
 * @param sensorType - 出参，传感器类型。
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 * @since 11
 */
int32_t OH_Sensor_GetEventSensorType(Sensor_SensorEvent* sensorEvent, Sensor_SensorType *sensorType);

/**
 * @brief 获取时间戳。
 *
 * @param sensor - 传感器数据信息。
 * @param timestamp - 出参，时间戳。
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 * @since 11
 */
int32_t OH_Sensor_GetEventTimestamp(Sensor_SensorEvent* sensorEvent, int64_t *timestamp);

/**
 * @brief 获取传感器数据精度。
 *
 * @param sensor - 传感器数据信息。
 * @param accuracy - 出参，传感器数据精度。
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 * @since 11
 */
int32_t OH_Sensor_GetEventAccuracy(Sensor_SensorEvent* sensorEvent, Sensor_SensorAccuracy *accuracy);

/**
 * @brief 获取传感器数据。数据的长度和内容依赖于监听的传感器类型，传感器上报的数据格式如下：
 * SENSOR_TYPE_ACCELEROMETER: data[0]、data[1]、data[2]分别表示设备x、y、z轴的加速度分量，单位m/s2；
 * SENSOR_TYPE_GYROSCOPE: data[0]、data[1]、data[2]分别表示设备x、y、z轴的旋转角速度，单位弧度/s；
 * SENSOR_TYPE_AMBIENT_LIGHT: data[0]表示环境光强度，in lux；
 * SENSOR_TYPE_MAGNETIC_FIELD: data[0]、data[1]、data[2]分别表示设备x、y、z轴的地磁分量，单位微特斯拉；
 * SENSOR_TYPE_BAROMETER：data[0]表示气压值，单位hPa;
 * SENSOR_TYPE_HALL: data[0]表示皮套吸合状态，0表示打开，大于0表示吸附;
 * SENSOR_TYPE_PROXIMITY：data[0]表示接近状态，0表示接近，大于0表示远离；
 * SENSOR_TYPE_ORIENTATION:data[0]、data[1]、data[2]分别表示设备绕z、x、y轴的角度，单位度；
 * SENSOR_TYPE_GRAVITY：data[0]、data[1]、data[2]分别表示设备x、y、z轴的重力加速度分量，单位m/s2;
 * SENSOR_TYPE_ROTATION_VECTOR:data[0]、data[1]、data[2]分别表示设备x、y、z轴的旋转角度，单位度，data[3]表示旋转向量元素;
 * SENSOR_TYPE_PEDOMETER_DETECTION:data[0]表示几步检测状态，1表示检测到了步数变化
 * SENSOR_TYPE_PEDOMETER:data[0]表示步数；
 * SENSOR_TYPE_HEART_RATE：data[0]表示心率数值；
 * 
 * @param sensor - 传感器数据信息。
 * @param data - 出参，传感器数据。
 * @param length - 出参，数组长度。
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 * @since 11
 */
int32_t OH_Sensor_GetSensorData(Sensor_SensorEvent* sensorEvent, float **data, uint32_t *length);

/**
 * @brief 传感器订阅Id，唯一标识传感器。
 * @syscap SystemCapability.Sensors.Sensor
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
 * @brief 获取传感器类型。
 *
 * @param subscribeId - 传感器订阅Id。
 * @param sensorType - 出参，传感器类型。
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 * @since 11
 */
int32_t OH_Sensor_GetSubscribeSensorType(Sensor_SensorSubscribeId* subscribeId, Sensor_SensorType *sensorType);

/**
 * @brief 设置传感器类型。
 *
 * @param subscribeId - 传感器订阅Id。
 * @param sensorType - 传感器类型。
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 * @since 11
 */
int32_t OH_Sensor_SetSubscribeSensorType(Sensor_SensorSubscribeId* subscribeId, const Sensor_SensorType sensorType);

/**
 * @brief 传感器订阅属性。
 * @syscap SystemCapability.Sensors.Sensor
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
 * @brief 设置上报时间间隔。
 *
 * @param attribute - 传感器订阅属性。
 * @param samplingInterval - 上报时间间隔，单位ns。
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 * @since 11
 */
int32_t OH_Sensor_SetSamplingInterval(Sensor_SubscribeAttribute* attribute, const int64_t samplingInterval);

/**
 * @brief 获取上报时间间隔。
 *
 * @param attribute - 传感器订阅属性。
 * @param samplingInterval - 出参，上报时间间隔，单位ns。
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 * @since 11
 */
int32_t OH_Sensor_GetSamplingInterval(Sensor_SubscribeAttribute* attribute, int64_t *samplingInterval);

/**
 * @brief 传感器数据的回调函数。
 * @syscap SystemCapability.Sensors.Sensor
 * @since 11
 */
typedef void (*Sensor_RecordSensorCallback)(Sensor_SensorEvent *event);

/**
 * @brief 传感器订阅者信息。
 * @syscap SystemCapability.Sensors.Sensor
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
 * @brief 设置传感器数据的回调函数。
 *
 * @param user - 传感器订阅者信息。
 * @param callback - 传感器数据的回调函数。
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 * @since 11
 */
int32_t OH_Sensor_SetSensorCallback(Sensor_SubscribeUser* user, const Sensor_RecordSensorCallback callback);

/**
 * @brief 获取传感器数据的回调函数。
 *
 * @param user - 传感器订阅者信息。
 * @param callback - 出参，传感器数据的回调函数。
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 * @since 11
 */
int32_t OH_Sensor_GetSensorCallback(Sensor_SubscribeUser* user, Sensor_RecordSensorCallback *callback);
#ifdef __cplusplus
}
#endif
#endif // OH_NATIVE_SENSOR_TYPE_H