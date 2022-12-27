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

/**
 * @addtogroup PanSensor
 * @{
 *
 * @brief Provides standard open APIs for you to use common capabilities of sensors.
 *
 * For example, you can call these APIs to obtain sensor information,
 * subscribe to or unsubscribe from sensor data, enable or disable a sensor,
 * and set the sensor data reporting mode.
 *
 * @since 5
 */

/**
 * @file sensor_agent_type.h
 *
 * @brief Defines the basic data used by the sensor agent to manage sensors.
 *
 * @since 5
 */

#ifndef SENSOR_AGENT_TYPE_H
#define SENSOR_AGENT_TYPE_H

#include <stdint.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/** Maximum length of the sensor name */
#ifndef NAME_MAX_LEN
#define NAME_MAX_LEN 128
#endif /* NAME_MAX_LEN */
/** Size of sensor data */
#ifndef SENSOR_USER_DATA_SIZE
#define SENSOR_USER_DATA_SIZE 104
#endif /* SENSOR_USER_DATA_SIZE */
/** Maximum length of the sensor version */
#ifndef VERSION_MAX_LEN
#define VERSION_MAX_LEN 16
#endif /* SENSOR_USER_DATA_SIZE */

/**
 * @brief Enumerates sensor types.
 *
 * @since 5
 */
typedef enum SensorTypeId {
    SENSOR_TYPE_ID_NONE = 0,                   /**< None */
    SENSOR_TYPE_ID_ACCELEROMETER = 1,          /**< Acceleration sensor */
    SENSOR_TYPE_ID_GYROSCOPE = 2,              /**< Gyroscope sensor */
    SENSOR_TYPE_ID_AMBIENT_LIGHT = 5,          /**< Ambient light sensor */
    SENSOR_TYPE_ID_MAGNETIC_FIELD = 6,         /**< Magnetic field sensor */
    SENSOR_TYPE_ID_CAPACITIVE = 7,             /**< Capacitive sensor */
    SENSOR_TYPE_ID_BAROMETER = 8,              /**< Barometric pressure sensor */
    SENSOR_TYPE_ID_TEMPERATURE = 9,            /**< Temperature sensor */
    SENSOR_TYPE_ID_HALL = 10,                  /**< Hall effect sensor */
    SENSOR_TYPE_ID_GESTURE = 11,               /**< Gesture sensor */
    SENSOR_TYPE_ID_PROXIMITY = 12,             /**< Proximity sensor */
    SENSOR_TYPE_ID_HUMIDITY = 13,              /**< Humidity sensor */
    SENSOR_TYPE_ID_PHYSICAL_MAX = 0xFF,        /**< Maximum type ID of a physical sensor */
    SENSOR_TYPE_ID_ORIENTATION = 256,          /**< Orientation sensor */
    SENSOR_TYPE_ID_GRAVITY = 257,              /**< Gravity sensor */
    SENSOR_TYPE_ID_LINEAR_ACCELERATION = 258,  /**< Linear acceleration sensor */
    SENSOR_TYPE_ID_ROTATION_VECTOR = 259,      /**< Rotation vector sensor */
    SENSOR_TYPE_ID_AMBIENT_TEMPERATURE = 260,  /**< Ambient temperature sensor */
    SENSOR_TYPE_ID_MAGNETIC_FIELD_UNCALIBRATED = 261,  /**< Uncalibrated magnetic field sensor */
    SENSOR_TYPE_ID_GAME_ROTATION_VECTOR = 262,    /**< Game rotation vector sensor */
    SENSOR_TYPE_ID_GYROSCOPE_UNCALIBRATED = 263,  /**< Uncalibrated gyroscope sensor */
    SENSOR_TYPE_ID_SIGNIFICANT_MOTION = 264,    /**< Significant motion sensor */
    SENSOR_TYPE_ID_PEDOMETER_DETECTION = 265,   /**< Pedometer detection sensor */
    SENSOR_TYPE_ID_PEDOMETER = 266,             /**< Pedometer sensor */
    SENSOR_TYPE_ID_GEOMAGNETIC_ROTATION_VECTOR = 277,  /**< Geomagnetic rotation vector sensor */
    SENSOR_TYPE_ID_HEART_RATE = 278,            /**< Heart rate sensor */
    SENSOR_TYPE_ID_DEVICE_ORIENTATION = 279,    /**< Device orientation sensor */
    SENSOR_TYPE_ID_WEAR_DETECTION = 280,        /**< Wear detection sensor */
    SENSOR_TYPE_ID_ACCELEROMETER_UNCALIBRATED = 281,   /**< Uncalibrated acceleration sensor */
    SENSOR_TYPE_ID_MAX = 30,      /**< Maximum number of sensor type IDs*/
} SensorTypeId;

/**
 * @brief Defines sensor information.
 *
 * @since 5
 */
typedef struct SensorInfo {
    char sensorName[NAME_MAX_LEN];   /**< Sensor name */
    char vendorName[NAME_MAX_LEN];   /**< Sensor vendor */
    char firmwareVersion[VERSION_MAX_LEN];  /**< Sensor firmware version */
    char hardwareVersion[VERSION_MAX_LEN];  /**< Sensor hardware version */
    int32_t sensorTypeId;  /**< Sensor type ID */
    int32_t sensorId;      /**< Sensor ID */
    float maxRange;        /**< Maximum measurement range of the sensor */
    float precision;       /**< Sensor accuracy */
    float power;           /**< Sensor power */
    int64_t minSamplePeriod; /**< Minimum sample period allowed, in ns */
    int64_t maxSamplePeriod; /**< Maxmum sample period allowed, in ns */
} SensorInfo;

/**
 * @brief Defines the data reported by the sensor.
 *
 * @since 5
 */
typedef struct SensorEvent {
    int32_t sensorTypeId;  /**< Sensor type ID */
    int32_t version;       /**< Sensor algorithm version */
    int64_t timestamp;     /**< Time when sensor data was reported */
    uint32_t option;       /**< Sensor data options, including the measurement range and accuracy */
    int32_t mode;          /**< Sensor data reporting mode (described in {@link SensorMode}) */
    uint8_t *data;         /**< Sensor data */
    uint32_t dataLen;      /**< Sensor data length */
} SensorEvent;

/**
 * @brief Defines the callback for data reporting by the sensor agent.
 *
 * @since 5
 */
typedef void (*RecordSensorCallback)(SensorEvent *event);

/**
 * @brief Defines a reserved field for the sensor data subscriber.
 *
 * @since 5
 */
typedef struct UserData {
    char userData[SENSOR_USER_DATA_SIZE];  /**< Reserved for the sensor data subscriber */
} UserData;

/**
 * @brief Defines information about the sensor data subscriber.
 *
 * @since 5
 */
typedef struct SensorUser {
    char name[NAME_MAX_LEN];  /**< Name of the sensor data subscriber */
    RecordSensorCallback callback;   /**< Callback for reporting sensor data */
    UserData *userData;              /**< Reserved field for the sensor data subscriber */
} SensorUser;

/**
 * @brief Enumerates data reporting modes of sensors.
 *
 * @since 5
 */
typedef enum SensorMode {
    SENSOR_DEFAULT_MODE = 0,   /**< Default data reporting mode */
    SENSOR_REALTIME_MODE = 1,  /**< Real-time data reporting mode to report a group of data each time */
    SENSOR_ON_CHANGE = 2,   /**< Real-time data reporting mode to report data upon status changes */
    SENSOR_ONE_SHOT = 3,    /**< Real-time data reporting mode to report data only once */
    SENSOR_FIFO_MODE = 4,   /**< FIFO-based data reporting mode to report data based on the <b>BatchCnt</b> setting */
    SENSOR_MODE_MAX2,        /**< Maximum sensor data reporting mode */
} SensorMode;

/**
 * @brief Defines the accelerometer data structure. Measures the acceleration applied to
 * the device on three physical axes (x, y, and z) in m/s2.
 *
 */
typedef struct AccelData {
    float x;
    float y;
    float z;
} AccelData;

/**
 * @brief Defines the linear accelerometer data structure. Measures the linear acceleration applied to
 * the device on three physical axes (x, y, and z) in m/s2.
 */
typedef struct LinearAccelData {
    float x;
    float y;
    float z;
} LinearAccelData;

/**
 * @brief Defines the gyroscope sensor data structure. Measures the rotational angular velocity of the
 * device on three physical axes (x, y, and z) in rad/s.
 */
typedef struct GyroscopeData {
    float x;
    float y;
    float z;
} GyroscopeData;

/**
 * @brief Defines the gravity sensor data structure. Measures the acceleration of gravity applied
 * to the device on three physical axes (x, y, and z) in m/s2.
 */
typedef struct GravityData {
    float x;
    float y;
    float z;
} GravityData;

/**
 * @brief Defines the uncalibrated accelerometer data structure. Measures the uncalibrated accelerometer applied to
 * the device on three physical axes (x, y, and z) in m/s2.
 */
typedef struct AccelUncalibratedData {
    float x;
    float y;
    float z;
    float biasX;
    float biasY;
    float biasZ;
} AccelUncalibratedData;

/**
 * @brief Defines the uncalibrated gyroscope sensor data structure. Measures the uncalibrated rotational angular velocity of the
 * device on three physical axes (x, y, and z) in rad/s.
 */
typedef struct GyroUncalibratedData {
    float x;
    float y;
    float z;
    float biasX;
    float biasY;
    float biasZ;
} GyroUncalibratedData;

/**
 * @brief Defines the significant Motion sensor data structure. Measures whether there is substantial motion in the device on
 * the three physical axes (x, y, and z); a value of 1 indicates the presence of large motion; and a value of 0 indicates that
 * there is no large movement.
 */
typedef struct SignificantMotionData {
    float scalar;
} SignificantMotionData;

/**
 * @brief Defines the pedometer detection sensor data structure. Detects the user's step counting action; if the value is 1, it
 * means that the user has generated the action of counting walking; if the value is 0, it means that the user has not moved.
 */
typedef struct PedometerDetectData {
    float scalar;
} PedometerDetectData;

/**
 * @brief Defines the pedometer sensor data structure. Counts the number of steps taken by the user.
 */
typedef struct PedometerData {
    float steps;
} PedometerData;

/**
 * @brief Defines the ambient temperature sensor data structure. Measures ambient temperature in degrees Celsius (°C)
 */
typedef struct AmbientTemperatureData {
    float temperature;
} AmbientTemperatureData;

/**
 * @brief Define the humidity sensor data structure. Measures the relative humidity of the environment,
 * expressed as a percentage (%).
 */
typedef struct HumidityData {
    float humidity;
} HumidityData;

/**
 * @brief Defines the magnetic field sensor data structure. Measure the ambient geomagnetic field in three
 * physical axes (x, y, z) in μT.
 */
typedef struct MagneticFieldData {
    float x;
    float y;
    float z;
} MagneticFieldData;

/**
 * @brief Defines the uncalibrated magnetic field sensor data structure. Measure the uncalibrated ambient geomagnetic field in three
 * physical axes (x, y, z) in μT.
 */
typedef struct MagneticFieldUncalibratedData {
    float x;
    float y;
    float z;
    float biasX;
    float biasY;
    float biasZ;
} MagneticFieldUncalibratedData;

/**
 * @brief Defines the barometer sensor data structure. Measures ambient air pressure in : hPa or mbar.
 */
typedef struct BarometerData {
    float pressure;
} BarometerData;

/**
 * @brief Defines the device orientation sensor data structure. Measure the direction of rotation of the device in rad.
 */
typedef struct DeviceOrientationData {
    float scalar;
} DeviceOrientationData;

/**
 * @brief Defines the orientation sensor data structure. Measures the angle value of the rotation of the device
 * around all three physical axes (z, x, y), in rad.
 */
typedef struct OrientationData {
    float alpha; /**< The device rotates at an angle around the Z axis */
    float beta;  /**< The device rotates at an angle around the X axis */
    float gamma; /**< The device rotates at an angle around the Y axis */
} OrientationData;

/**
 * @brief Defines the rotation vector sensor data structure. Measuring device game rotation vector, composite sensor:
 * synthesized by acceleration sensor, gyroscope sensor.
 */
typedef struct RotationVectorData {
    float x;
    float y;
    float z;
    float w;
} RotationVectorData;

/**
 * @brief Defines the game rotation vector sensor data structure. Measuring device game rotation vector, composite sensor:
 * synthesized by acceleration sensor, gyroscope sensor.
 */
typedef struct GameRotationVectorData {
    float x;
    float y;
    float z;
    float w;
} GameRotationVectorData;

/**
 * @brief Defines the geomagnetic rotation vector sensor data structure. Measuring device geomagnetic rotation vector, composite
 *  sensor: synthesized by acceleration sensor and magnetic field sensor.
 */
typedef struct GeomagneticRotaVectorData {
    float x;
    float y;
    float z;
    float w;
} GeomagneticRotaVectorData;

/**
 * @brief Defines the proximity light sensor data structure. Measures the proximity or distance of visible objects relative to
 * the device display, where 0 indicates proximity and 1 indicates distance.
 */
typedef struct ProximityData {
    float distance;
} ProximityData;

/**
 * @brief Defines the ambient light sensor data structure. Measures the intensity of light around the device in lux.
 */
typedef struct AmbientLightData {
    float intensity;
} AmbientLightData;

/**
 * @brief Defines the hall sensor data structure. Measure whether there is magnetic attraction around the device,
 * 0 means no magnet attraction, and 1 means there is magnet attraction.
 */
typedef struct HallData {
    float status;
} HallData;

/**
 * @brief Define the heart rate sensor data structure. Measures the user's heart rate, in bpm.
 */
typedef struct HeartRateData {
    float heartRate;
} HeartRateData;

/**
 * @brief Defines the wear detection sensor data structure. To detect whether the user is wearing it,
 * 0 means not wearing it, while 1 means wearing it
 */
typedef struct WearDetectionData {
    float value;
} WearDetectionData;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif /* SENSOR_AGENT_TYPE_H */
/**< @} */