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
 * @brief 为您提供标准的开放api，以使用传感器的常用功能。比如，您可以调用这些api来获取传感器属性信息、订阅或取消订阅传感器数据等。
 * @since 11
 */
/**
 * @file sensor.h
 *
 * @brief 声明操作传感器的常用api，用于获取传感器信息、订阅或取消订阅传感器数据等。
 * @library libsensor_native.z.so
 * @syscap SystemCapability.Sensors.Sensor
 * @since 11
 */

#ifndef OH_NATIVE_SENSOR_H
#define OH_NATIVE_SENSOR_H

#include <cstddef>

#include "native_sensor_type.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief 获取设备上的所有传感器信息。
 *
 * @param sensorInfo - 出参，返回设备上的所有传感器信息，详情请参见{@Link Sensor_Sensor}。
 * @param count - 出参，返回设备上的传感器数量。
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 *
 * @since 11
 */
Sensor_Result OH_Sensor_GetAllSensors(Sensor_Sensor **sensors, uint32_t *count);

/**
 * @brief 订阅传感器数据，系统将按照用户指定的上报频率向用户报告传感器数据。
 *
 * @param id - 标识要订阅的传感器, 详情请参见{@link Sensor_SensorSubscribeId}。
 * @param attribute - 订阅属性，可用于指定传感器的上报频率等，详情请参见{@Link Sensor_SubscribeAttribute}。
 * @param user - 订阅者信息，用于指定传感器数据回调函数等，详情请参见{@Link Sensor_SubscribeUser}。
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 *
 * @since 11
 */
Sensor_Result OH_Sensor_SubscribeSensor(const Sensor_SensorSubscribeId *subscribeId, const Sensor_SubscribeAttribute *attribute,
    const Sensor_SubscribeUser *user);

/**
 * @brief 取消订阅传感器数据。
 *
 * @param id - 标识要订阅的传感器, 详情请参见{@link Sensor_SensorSubscribeId}。
 * @param user - 订阅者信息，用于指定传感器数据回调函数等，详情请参见{@Link Sensor_SubscribeUser}。
 * @return 成功返回SENSOR_SUCCESS，否则返回对应的错误码，详情请参见{@Link Sensor_Result}。
 *
 * @since 11
 */
Sensor_Result OH_Sensor_UnsubscribeSensor(const Sensor_SensorSubscribeId *subscribeId, const Sensor_SubscribeUser *user);
#ifdef __cplusplus
}
#endif
#endif // OH_NATIVE_SENSOR_H