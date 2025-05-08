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

#include "ohos.sensor.impl.hpp"

#include <cinttypes>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>
#include <unistd.h>

#include "ohos.sensor.proj.hpp"
#include "refbase.h"
#include "securec.h"
#include "sensor_agent.h"
#include "sensor_agent_type.h"
#include "sensor_algorithm.h"
#include "sensor_errors.h"
#include "sensor_log.h"
#include "stdexcept"
#include "taihe/runtime.hpp"

#undef LOG_TAG
#define LOG_TAG "OhosSensorImpl"

using namespace taihe;
using namespace ohos::sensor;
using namespace OHOS::Sensors;
using namespace OHOS;

using responseSensorData = std::variant<int32_t, int64_t, float, double, ohos::sensor::SensorAccuracy>;
using callbackType = std::variant<taihe::callback<void(WearDetectionResponse const &)>,
    taihe::callback<void(SignificantMotionResponse const &)>, taihe::callback<void(RotationVectorResponse const &)>,
    taihe::callback<void(ProximityResponse const &)>, taihe::callback<void(PedometerDetectionResponse const &)>,
    taihe::callback<void(PedometerResponse const &)>, taihe::callback<void(OrientationResponse const &)>,
    taihe::callback<void(MagneticFieldUncalibratedResponse const &)>,
    taihe::callback<void(MagneticFieldResponse const &)>, taihe::callback<void(LinearAccelerometerResponse const &)>,
    taihe::callback<void(HumidityResponse const &)>, taihe::callback<void(HeartRateResponse const &)>,
    taihe::callback<void(HallResponse const &)>, taihe::callback<void(GyroscopeUncalibratedResponse const &)>,
    taihe::callback<void(GyroscopeResponse const &)>, taihe::callback<void(GravityResponse const &)>,
    taihe::callback<void(BarometerResponse const &)>, taihe::callback<void(AmbientTemperatureResponse const &)>,
    taihe::callback<void(LightResponse const &)>, taihe::callback<void(AccelerometerUncalibratedResponse const &)>,
    taihe::callback<void(AccelerometerResponse const &)>, taihe::callback<void(SarResponse const &)>,
    taihe::callback<void(ColorResponse const &)>>;

struct CallbackObject : public RefBase {
    CallbackObject(callbackType cb, ani_ref ref) : callback(cb), ref(ref)
    {
    }
    ~CallbackObject()
    {
    }
    callbackType callback;
    ani_ref ref;
};

namespace {
constexpr int32_t ROTATION_VECTOR_LENGTH = 3;
constexpr int32_t REPORTING_INTERVAL = 200000000;
constexpr int32_t THREE_DIMENSIONAL_MATRIX_LENGTH = 9;
constexpr int32_t CALLBACK_MAX_DATA_LENGTH = 16;

void CallBackAccelermeter(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
void CallBackGyroscope(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
void CallBackAmbientLight(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
void CallBackMagneticField(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
void CallBackBarometer(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
void CallBackHall(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
void CallBackProximity(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
void CallBackHumidity(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
void CallBackOrientation(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
void CallBackGravity(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
void CallBackLinearAcceleration(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
void CallBackRotationVector(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
void CallBackAmbientTemperature(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
void CallBackMagneticFieldUncalibrated(std::map<std::string, responseSensorData> data,
    sptr<CallbackObject> callbackObject);
void CallBackGyroscopeUncalibrated(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
void CallBackSignificantMotion(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
void CallBackPedometerDetection(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
void CallBackPedometer(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
void CallBackHeartRate(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
void CallBackWearDetection(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
void CallBackAccelerometerUncalibrated(std::map<std::string, responseSensorData> data,
    sptr<CallbackObject> callbackObject);
void CallBackColor(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
void CallBackSar(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
void EmitOnceCallback(SensorEvent *event);

std::map<taihe::string, int64_t> g_samplingPeriod = {
    { "normal", 200000000 },
    { "ui", 60000000 },
    { "game", 20000000 },
};

std::map<int32_t, std::vector<string>> g_sensorAttributeList = {
    { 0, { "x" } },
    { SENSOR_TYPE_ID_ACCELEROMETER, { "x", "y", "z" } },
    { SENSOR_TYPE_ID_GYROSCOPE, { "x", "y", "z" } },
    { SENSOR_TYPE_ID_AMBIENT_LIGHT, { "intensity", "colorTemperature", "infraredLuminance" } },
    { SENSOR_TYPE_ID_MAGNETIC_FIELD, { "x", "y", "z" } },
    { SENSOR_TYPE_ID_BAROMETER, { "pressure" } },
    { SENSOR_TYPE_ID_HALL, { "status" } },
    { SENSOR_TYPE_ID_TEMPERATURE, { "temperature" } },
    { SENSOR_TYPE_ID_PROXIMITY, { "distance" } },
    { SENSOR_TYPE_ID_HUMIDITY, { "humidity" } },
    { SENSOR_TYPE_ID_ORIENTATION, { "alpha", "beta", "gamma" } },
    { SENSOR_TYPE_ID_GRAVITY, { "x", "y", "z" } },
    { SENSOR_TYPE_ID_LINEAR_ACCELERATION, { "x", "y", "z" } },
    { SENSOR_TYPE_ID_ROTATION_VECTOR, { "x", "y", "z", "w" } },
    { SENSOR_TYPE_ID_AMBIENT_TEMPERATURE, { "temperature" } },
    { SENSOR_TYPE_ID_MAGNETIC_FIELD_UNCALIBRATED, { "x", "y", "z", "biasX", "biasY", "biasZ" } },
    { SENSOR_TYPE_ID_GYROSCOPE_UNCALIBRATED, { "x", "y", "z", "biasX", "biasY", "biasZ" } },
    { SENSOR_TYPE_ID_SIGNIFICANT_MOTION, { "scalar" } },
    { SENSOR_TYPE_ID_PEDOMETER_DETECTION, { "scalar" } },
    { SENSOR_TYPE_ID_PEDOMETER, { "steps" } },
    { SENSOR_TYPE_ID_HEART_RATE, { "heartRate" } },
    { SENSOR_TYPE_ID_WEAR_DETECTION, { "value" } },
    { SENSOR_TYPE_ID_ACCELEROMETER_UNCALIBRATED, { "x", "y", "z", "biasX", "biasY", "biasZ" } },
    { SENSOR_TYPE_ID_COLOR, { "lightIntensity", "colorTemperature" } },
    { SENSOR_TYPE_ID_SAR, { "absorptionRatio" } },
    { SENSOR_TYPE_ID_FUSION_PRESSURE, { "fusionPressure" } },
};

std::map<int32_t, std::function<void(std::map<std::string, responseSensorData>, sptr<CallbackObject>)>>
    g_functionBySensorTypeIdMap = {
        { SENSOR_TYPE_ID_ACCELEROMETER, CallBackAccelermeter },
        { SENSOR_TYPE_ID_GYROSCOPE, CallBackGyroscope },
        { SENSOR_TYPE_ID_AMBIENT_LIGHT, CallBackAmbientLight },
        { SENSOR_TYPE_ID_MAGNETIC_FIELD, CallBackMagneticField },
        { SENSOR_TYPE_ID_BAROMETER, CallBackBarometer },
        { SENSOR_TYPE_ID_HALL, CallBackHall },
        { SENSOR_TYPE_ID_PROXIMITY, CallBackProximity },
        { SENSOR_TYPE_ID_HUMIDITY, CallBackHumidity },
        { SENSOR_TYPE_ID_ORIENTATION, CallBackOrientation },
        { SENSOR_TYPE_ID_GRAVITY, CallBackGravity },
        { SENSOR_TYPE_ID_LINEAR_ACCELERATION, CallBackLinearAcceleration },
        { SENSOR_TYPE_ID_ROTATION_VECTOR, CallBackRotationVector },
        { SENSOR_TYPE_ID_AMBIENT_TEMPERATURE, CallBackAmbientTemperature },
        { SENSOR_TYPE_ID_MAGNETIC_FIELD_UNCALIBRATED, CallBackMagneticFieldUncalibrated },
        { SENSOR_TYPE_ID_GYROSCOPE_UNCALIBRATED, CallBackGyroscopeUncalibrated },
        { SENSOR_TYPE_ID_SIGNIFICANT_MOTION, CallBackSignificantMotion },
        { SENSOR_TYPE_ID_PEDOMETER_DETECTION, CallBackPedometerDetection },
        { SENSOR_TYPE_ID_PEDOMETER, CallBackPedometer },
        { SENSOR_TYPE_ID_HEART_RATE, CallBackHeartRate },
        { SENSOR_TYPE_ID_WEAR_DETECTION, CallBackWearDetection },
        { SENSOR_TYPE_ID_ACCELEROMETER_UNCALIBRATED, CallBackAccelerometerUncalibrated },
        { SENSOR_TYPE_ID_COLOR, CallBackColor },
        { SENSOR_TYPE_ID_SAR, CallBackSar },
    };

std::mutex g_mutex;
std::mutex g_bodyMutex;
std::map<int32_t, std::vector<sptr<CallbackObject>>> g_subscribeCallbacks;
std::mutex g_onMutex;
std::mutex g_onceMutex;
std::map<int32_t, std::vector<sptr<CallbackObject>>> g_onceCallbackInfos;
std::map<int32_t, std::vector<sptr<CallbackObject>>> g_onCallbackInfos;
std::mutex g_sensorTaiheAttrListMutex;

RotationMatrixResponse getRotationMatrixSyncGrav(array_view<float> gravity, array_view<float> geomagnetic)
{
    std::vector<float> rotation(THREE_DIMENSIONAL_MATRIX_LENGTH);
    std::vector<float> inclination(THREE_DIMENSIONAL_MATRIX_LENGTH);
    SensorAlgorithm sensorAlgorithm;
    std::vector<float> gravityVector(gravity.begin(), gravity.end());
    std::vector<float> geomagneticVector(geomagnetic.begin(), geomagnetic.end());
    int32_t ret = sensorAlgorithm.CreateRotationAndInclination(gravityVector, geomagneticVector, rotation, inclination);
    ohos::sensor::RotationMatrixResponse rsp = {
        .rotation = array<float>::make(THREE_DIMENSIONAL_MATRIX_LENGTH),
        .inclination = array<float>::make(THREE_DIMENSIONAL_MATRIX_LENGTH),
    };
    if (ret != OHOS::ERR_OK) {
        taihe::set_business_error(ret, "Create rotation and inclination matrix fail");
        return rsp;
    }
    for (int32_t i = 0; i < THREE_DIMENSIONAL_MATRIX_LENGTH; ++i) {
        rsp.rotation[i] = rotation[i];
        rsp.inclination[i] = inclination[i];
    }
    return rsp;
}

array<float> getOrientationSync(array_view<float> rotationMatrix)
{
    std::vector<float> rotationAngle(ROTATION_VECTOR_LENGTH);
    SensorAlgorithm sensorAlgorithm;
    std::vector<float> rotationMatrixVector(rotationMatrix.begin(), rotationMatrix.end());
    int32_t ret = sensorAlgorithm.GetDirection(rotationMatrixVector, rotationAngle);
    if (ret != OHOS::ERR_OK) {
        taihe::set_business_error(ret, "Get direction fail");
        return taihe::array<float>(rotationAngle);
    }
    return taihe::array<float>(rotationAngle);
}

array<float> getRotationMatrixSync(array_view<float> rotation)
{
    std::vector<float> rotationMatrix(THREE_DIMENSIONAL_MATRIX_LENGTH);
    SensorAlgorithm sensorAlgorithm;
    std::vector<float> rotationVector(rotation.begin(), rotation.end());
    int32_t ret = sensorAlgorithm.CreateRotationMatrix(rotationVector, rotationMatrix);
    if (ret != OHOS::ERR_OK) {
        taihe::set_business_error(ret, "Create rotation matrix fail");
        return taihe::array<float>(rotationMatrix);
    }
    return taihe::array<float>(rotationMatrix);
}

array<Sensor> getSensorListSync()
{
    SensorInfo *sensorInfos = nullptr;
    int32_t count = 0;
    int32_t ret = GetAllSensors(&sensorInfos, &count);
    std::vector<::ohos::sensor::Sensor> result;
    if (ret != OHOS::ERR_OK) {
        taihe::set_business_error(ret, "Get sensor list fail");
        return taihe::array<::ohos::sensor::Sensor>(result);
    }
    for (int32_t i = 0; i < count; ++i) {
        if ((sensorInfos[i].sensorTypeId == SENSOR_TYPE_ID_AMBIENT_LIGHT1) ||
            (sensorInfos[i].sensorTypeId == SENSOR_TYPE_ID_PROXIMITY1) ||
            (sensorInfos[i].sensorTypeId > GL_SENSOR_TYPE_PRIVATE_MIN_VALUE)) {
            SEN_HILOGD("This sensor is secondary ambient light");
            continue;
        }
        ohos::sensor::Sensor sensorInfo = {
            .sensorName = sensorInfos[i].sensorName,
            .vendorName = sensorInfos[i].vendorName,
            .firmwareVersion = sensorInfos[i].firmwareVersion,
            .hardwareVersion = sensorInfos[i].hardwareVersion,
            .sensorId = sensorInfos[i].sensorId,
            .maxRange = sensorInfos[i].maxRange,
            .minSamplePeriod = sensorInfos[i].minSamplePeriod,
            .maxSamplePeriod = sensorInfos[i].maxSamplePeriod,
            .precision = sensorInfos[i].precision,
            .power = sensorInfos[i].power,
        };
        result.push_back(sensorInfo);
    }
    return taihe::array<::ohos::sensor::Sensor>(result);
}

bool GetInterval(const Options &options, int64_t &interval)
{
    if (options.interval.holds_interval_i64()) {
        interval = *options.interval.get_interval_i64_ptr();
    } else if (options.interval.holds_interval_string()) {
        taihe::string modeTemp = *options.interval.get_interval_string_ptr();
        auto iter = g_samplingPeriod.find(modeTemp);
        if (iter == g_samplingPeriod.end()) {
            SEN_HILOGE("Find interval mode failed");
            return false;
        }
        interval = static_cast<int64_t>(iter->second);
        SEN_HILOGI("Get interval by mode, mode:%{public}s", modeTemp.c_str());
    } else {
        SEN_HILOGE("Interval failed");
        return false;
    }
    return true;
}

bool CheckSubscribe(int32_t sensorTypeId)
{
    std::lock_guard<std::mutex> onCallbackLock(g_onMutex);
    return g_onCallbackInfos.find(sensorTypeId) != g_onCallbackInfos.end();
}

void CallBackAccelermeter(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(AccelerometerResponse const &)>>(callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback accelerometerResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    AccelerometerResponse responseData = {
        .base = res,
        .x = std::get<float>(data["x"]),
        .y = std::get<float>(data["y"]),
        .z = std::get<float>(data["z"]),
    };
    auto &func = std::get<taihe::callback<void(AccelerometerResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackGyroscope(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(GyroscopeResponse const &)>>(callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback gyroscopeResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    GyroscopeResponse responseData = {
        .base = res,
        .x = std::get<float>(data["x"]),
        .y = std::get<float>(data["y"]),
        .z = std::get<float>(data["z"]),
    };
    auto &func = std::get<taihe::callback<void(GyroscopeResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackAmbientLight(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(LightResponse const &)>>(callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback lightResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    LightResponse responseData = {
        .base = res,
        .intensity = std::get<float>(data["intensity"]),
        .colorTemperature = taihe::optional<float>(std::in_place_t{}, std::get<float>(data["colorTemperature"])),
        .infraredLuminance = taihe::optional<float>(std::in_place_t{}, std::get<float>(data["infraredLuminance"])),
    };
    auto &func = std::get<taihe::callback<void(LightResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackMagneticField(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(MagneticFieldResponse const &)>>(callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback magneticFieldResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    MagneticFieldResponse responseData = {
        .base = res,
        .x = std::get<float>(data["x"]),
        .y = std::get<float>(data["y"]),
        .z = std::get<float>(data["z"]),
    };
    auto &func = std::get<taihe::callback<void(MagneticFieldResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackBarometer(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(BarometerResponse const &)>>(callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback barometerResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    BarometerResponse responseData = {
        .base = res,
        .pressure = std::get<float>(data["pressure"]),
    };
    auto &func = std::get<taihe::callback<void(BarometerResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackHall(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(HallResponse const &)>>(callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback hallResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    HallResponse responseData = {
        .base = res,
        .status = std::get<float>(data["status"]),
    };
    auto &func = std::get<taihe::callback<void(HallResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackProximity(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(ProximityResponse const &)>>(callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback proximityResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    ProximityResponse responseData = {
        .base = res,
        .distance = std::get<float>(data["distance"]),
    };
    auto &func = std::get<taihe::callback<void(ProximityResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackHumidity(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(HumidityResponse const &)>>(callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback humidityResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    HumidityResponse responseData = {
        .base = res,
        .humidity = std::get<float>(data["humidity"]),
    };
    auto &func = std::get<taihe::callback<void(HumidityResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackOrientation(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(OrientationResponse const &)>>(callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback orientationResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    OrientationResponse responseData = {
        .base = res,
        .alpha = std::get<float>(data["alpha"]),
        .beta = std::get<float>(data["beta"]),
        .gamma = std::get<float>(data["gamma"]),
    };
    auto &func = std::get<taihe::callback<void(OrientationResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackGravity(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(GravityResponse const &)>>(callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback gravityResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    GravityResponse responseData = {
        .base = res,
        .x = std::get<float>(data["x"]),
        .y = std::get<float>(data["y"]),
        .z = std::get<float>(data["z"]),
    };
    auto &func = std::get<taihe::callback<void(GravityResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackLinearAcceleration(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(LinearAccelerometerResponse const &)>>(callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback linearAccelerometerResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    LinearAccelerometerResponse responseData = {
        .base = res,
        .x = std::get<float>(data["x"]),
        .y = std::get<float>(data["y"]),
        .z = std::get<float>(data["z"]),
    };
    auto &func = std::get<taihe::callback<void(LinearAccelerometerResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackRotationVector(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(RotationVectorResponse const &)>>(callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback rotationVectorResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    RotationVectorResponse responseData = {
        .base = res,
        .x = std::get<float>(data["x"]),
        .y = std::get<float>(data["y"]),
        .z = std::get<float>(data["z"]),
        .w = std::get<float>(data["w"]),
    };
    auto &func = std::get<taihe::callback<void(RotationVectorResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackAmbientTemperature(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(AmbientTemperatureResponse const &)>>(callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback ambientTemperatureResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    AmbientTemperatureResponse responseData = {
        .base = res,
        .temperature = std::get<float>(data["temperature"]),
    };
    auto &func = std::get<taihe::callback<void(AmbientTemperatureResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackMagneticFieldUncalibrated(std::map<std::string, responseSensorData> data,
    sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(MagneticFieldUncalibratedResponse const &)>>(
        callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback magneticFieldUncalibratedResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    MagneticFieldUncalibratedResponse responseData = {
        .base = res,
        .x = std::get<float>(data["x"]),
        .y = std::get<float>(data["y"]),
        .z = std::get<float>(data["z"]),
        .biasX = std::get<float>(data["biasX"]),
        .biasY = std::get<float>(data["biasY"]),
        .biasZ = std::get<float>(data["biasZ"]),
    };
    auto &func = std::get<taihe::callback<void(MagneticFieldUncalibratedResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackGyroscopeUncalibrated(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(GyroscopeUncalibratedResponse const &)>>(
        callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback gyroscopeUncalibratedResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    GyroscopeUncalibratedResponse responseData = {
        .base = res,
        .x = std::get<float>(data["x"]),
        .y = std::get<float>(data["y"]),
        .z = std::get<float>(data["z"]),
        .biasX = std::get<float>(data["biasX"]),
        .biasY = std::get<float>(data["biasY"]),
        .biasZ = std::get<float>(data["biasZ"]),
    };
    auto &func = std::get<taihe::callback<void(GyroscopeUncalibratedResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackSignificantMotion(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(SignificantMotionResponse const &)>>(callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback significantMotionResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    SignificantMotionResponse responseData = {
        .base = res,
        .scalar = std::get<float>(data["scalar"]),
    };
    auto &func = std::get<taihe::callback<void(SignificantMotionResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackPedometerDetection(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(PedometerDetectionResponse const &)>>(callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback pedometerDetectionResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    PedometerDetectionResponse responseData = {
        .base = res,
        .scalar = std::get<float>(data["scalar"]),
    };
    auto &func = std::get<taihe::callback<void(PedometerDetectionResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackPedometer(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(PedometerResponse const &)>>(callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback pedometerResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    PedometerResponse responseData = {
        .base = res,
        .steps = std::get<float>(data["steps"]),
    };
    auto &func = std::get<taihe::callback<void(PedometerResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackHeartRate(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(HeartRateResponse const &)>>(callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback heartRateResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    HeartRateResponse responseData = {
        .base = res,
        .heartRate = std::get<float>(data["heartRate"]),
    };
    auto &func = std::get<taihe::callback<void(HeartRateResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackWearDetection(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(WearDetectionResponse const &)>>(callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback wearDetectionResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    WearDetectionResponse responseData = {
        .base = res,
        .value = std::get<float>(data["value"]),
    };
    auto &func = std::get<taihe::callback<void(WearDetectionResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackAccelerometerUncalibrated(std::map<std::string, responseSensorData> data,
    sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(AccelerometerUncalibratedResponse const &)>>(
        callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback accelerometerUncalibratedResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    AccelerometerUncalibratedResponse responseData = {
        .base = res,
        .x = std::get<float>(data["x"]),
        .y = std::get<float>(data["y"]),
        .z = std::get<float>(data["z"]),
        .biasX = std::get<float>(data["biasX"]),
        .biasY = std::get<float>(data["biasY"]),
        .biasZ = std::get<float>(data["biasZ"]),
    };
    auto &func = std::get<taihe::callback<void(AccelerometerUncalibratedResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackColor(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(ColorResponse const &)>>(callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback colorResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    ColorResponse responseData = {
        .base = res,
        .lightIntensity = std::get<float>(data["lightIntensity"]),
        .colorTemperature = std::get<float>(data["colorTemperature"]),
    };
    auto &func = std::get<taihe::callback<void(ColorResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackSar(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(SarResponse const &)>>(callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback sarResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    SarResponse responseData = {
        .base = res,
        .absorptionRatio = std::get<float>(data["absorptionRatio"]),
    };
    auto &func = std::get<taihe::callback<void(SarResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallbackDataBySensorTypeId(int32_t sensorTypeId, std::map<std::string, responseSensorData> data,
    sptr<CallbackObject> callbackObject)
{
    if (g_functionBySensorTypeIdMap.find(sensorTypeId) == g_functionBySensorTypeIdMap.end()) {
        SEN_HILOGE("SensorTypeId not exist, id:%{public}d", sensorTypeId);
        return;
    }
    g_functionBySensorTypeIdMap[sensorTypeId](data, callbackObject);
}

void CallbackSensorData(sptr<CallbackObject> callbackObject, SensorEvent *event)
{
    if (event == nullptr) {
        SEN_HILOGE("event is null");
        return;
    }
    int32_t sensorTypeId = event->sensorTypeId;
    std::lock_guard<std::mutex> sensorTaiheAttrListLock(g_sensorTaiheAttrListMutex);
    size_t size = g_sensorAttributeList[sensorTypeId].size();
    uint32_t dataLength = event->dataLen / sizeof(float);
    if (size > dataLength) {
        SEN_HILOGE("Data length mismatch");
        return;
    }
    auto sensorAttributes = g_sensorAttributeList[sensorTypeId];
    auto dataOrigin = reinterpret_cast<float *>(event->data);
    float dataNow[CALLBACK_MAX_DATA_LENGTH] = { 0 };
    if (memcpy_s(dataNow, sizeof(dataNow), dataOrigin, event->dataLen) != EOK) {
        SEN_HILOGE("Copy data failed");
        return;
    }
    std::map<std::string, responseSensorData> dataMap;
    for (size_t i = 0; i < size; ++i) {
        dataMap.emplace(sensorAttributes[i].c_str(), static_cast<float>(dataNow[i]));
    }
    dataMap.emplace("timestamp", event->timestamp);
    ohos::sensor::SensorAccuracy sensorAccuracyTemp(static_cast<ohos::sensor::SensorAccuracy::key_t>(event->option));
    dataMap.emplace("accuracy", sensorAccuracyTemp);
    CallbackDataBySensorTypeId(sensorTypeId, dataMap, callbackObject);
}

void EmitOnCallback(SensorEvent *event)
{
    if (event == nullptr) {
        SEN_HILOGE("event is null");
        return;
    }
    int32_t sensorTypeId = event->sensorTypeId;
    if (!CheckSubscribe(sensorTypeId)) {
        return;
    }
    std::lock_guard<std::mutex> onCallbackLock(g_onMutex);
    auto onCallbackInfos = g_onCallbackInfos[sensorTypeId];
    for (auto &onCallbackInfo : onCallbackInfos) {
        CallbackSensorData(onCallbackInfo, event);
    }
}

bool CheckSystemSubscribe(int32_t sensorTypeId)
{
    std::lock_guard<std::mutex> subscribeLock(g_mutex);
    auto iter = g_subscribeCallbacks.find(sensorTypeId);
    return iter != g_subscribeCallbacks.end();
}

void EmitSubscribeCallback(SensorEvent *event)
{
    if (event == nullptr) {
        SEN_HILOGE("event is null");
        return;
    }
    int32_t sensorTypeId = event->sensorTypeId;
    if (!CheckSystemSubscribe(sensorTypeId)) {
        return;
    }
    std::lock_guard<std::mutex> subscribeLock(g_mutex);
    auto callbacks = g_subscribeCallbacks[sensorTypeId];
    for (auto &callback : callbacks) {
        CallbackSensorData(callback, event);
    }
}

void DataCallbackImpl(SensorEvent *event)
{
    if (event == nullptr) {
        SEN_HILOGE("event is null");
        return;
    }
    EmitOnCallback(event);
    EmitSubscribeCallback(event);
    EmitOnceCallback(event);
}

const SensorUser user = { .callback = DataCallbackImpl };

int32_t UnsubscribeSensor(int32_t sensorTypeId)
{
    int32_t ret = DeactivateSensor(sensorTypeId, &user);
    if (ret != ERR_OK) {
        SEN_HILOGE("DeactivateSensor failed");
        return ret;
    }
    return UnsubscribeSensor(sensorTypeId, &user);
}

void EmitOnceCallback(SensorEvent *event)
{
    if (event == nullptr) {
        SEN_HILOGE("event is null");
        return;
    }
    int32_t sensorTypeId = event->sensorTypeId;
    std::lock_guard<std::mutex> onceCallbackLock(g_onceMutex);
    auto iter = g_onceCallbackInfos.find(sensorTypeId);
    if (iter == g_onceCallbackInfos.end()) {
        return;
    }
    auto &onceCallbackInfos = iter->second;
    while (!onceCallbackInfos.empty()) {
        auto onceCallbackInfo = onceCallbackInfos.front();
        auto beginIter = onceCallbackInfos.begin();
        onceCallbackInfos.erase(beginIter);
        CallbackSensorData(onceCallbackInfo, event);
    }
    g_onceCallbackInfos.erase(sensorTypeId);

    CHKCV((!CheckSubscribe(sensorTypeId)), "Has client subscribe, not need cancel subscribe");
    CHKCV((!CheckSystemSubscribe(sensorTypeId)), "Has client subscribe system api, not need cancel subscribe");
    UnsubscribeSensor(sensorTypeId);
}

int32_t SubscribeSensor(int32_t sensorTypeId, int64_t interval, RecordSensorCallback callback)
{
    int32_t ret = SubscribeSensor(sensorTypeId, &user);
    if (ret != ERR_OK) {
        SEN_HILOGE("SubscribeSensor failed");
        return ret;
    }
    ret = SetBatch(sensorTypeId, &user, interval, 0);
    if (ret != ERR_OK) {
        SEN_HILOGE("SetBatch failed");
        return ret;
    }
    return ActivateSensor(sensorTypeId, &user);
}

void UpdateCallbackInfos(int32_t sensorTypeId, callbackType callback, uintptr_t opq)
{
    std::lock_guard<std::mutex> onCallbackLock(g_onMutex);
    ani_object callbackObj = reinterpret_cast<ani_object>(opq);
    ani_ref callbackRef;
    ani_env *env = taihe::get_env();
    if (env == nullptr || env->GlobalReference_Create(callbackObj, &callbackRef) != ANI_OK) {
        SEN_HILOGE("Failed to create callbackRef, sensorTypeId:%{public}d", sensorTypeId);
        return;
    }
    std::vector<sptr<CallbackObject>> callbackInfos = g_onCallbackInfos[sensorTypeId];
    bool isSubscribedCallback =
        std::any_of(callbackInfos.begin(), callbackInfos.end(), [env, callbackRef](const CallbackObject *obj) {
            ani_boolean isEqual = false;
            return (env->Reference_StrictEquals(callbackRef, obj->ref, &isEqual) == ANI_OK) && isEqual;
        });
    if (isSubscribedCallback) {
        env->GlobalReference_Delete(callbackRef);
        SEN_HILOGE("Callback is already subscribed, sensorTypeId:%{public}d", sensorTypeId);
        return;
    }
    sptr<CallbackObject> taiheCallbackInfo = new (std::nothrow) CallbackObject(callback, callbackRef);
    if (taiheCallbackInfo == nullptr) {
        SEN_HILOGE("taiheCallbackInfo is nullptr");
        return;
    }
    callbackInfos.push_back(taiheCallbackInfo);
    g_onCallbackInfos[sensorTypeId] = callbackInfos;
}

void OnCommon(int32_t sensorTypeId, callbackType cb, uintptr_t opq, optional_view<Options> options)
{
    int64_t interval = REPORTING_INTERVAL;
    if (options.has_value() && !GetInterval(options.value(), interval)) {
        SEN_HILOGW("Get interval failed");
    }
    SEN_HILOGD("Interval is %{public}" PRId64, interval);
    int32_t ret = SubscribeSensor(sensorTypeId, interval, DataCallbackImpl);
    if (ret != ERR_OK) {
        taihe::set_business_error(ret, "SubscribeSensor fail");
        return;
    }
    UpdateCallbackInfos(sensorTypeId, cb, opq);
}

void OnWearDetection(callback_view<void(WearDetectionResponse const &)> f, uintptr_t opq,
    optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_WEAR_DETECTION, f, opq, options);
}

void UpdateOnceCallback(int32_t sensorTypeId, callbackType cb, uintptr_t opq)
{
    std::lock_guard<std::mutex> onceCallbackLock(g_onceMutex);
    ani_object callbackObj = reinterpret_cast<ani_object>(opq);
    ani_ref callbackRef;
    ani_env *env = taihe::get_env();
    if (env == nullptr || env->GlobalReference_Create(callbackObj, &callbackRef) != ANI_OK) {
        SEN_HILOGE("Failed to create callbackRef, sensorTypeId:%{public}d", sensorTypeId);
        return;
    }
    std::vector<sptr<CallbackObject>> callbackInfos = g_onceCallbackInfos[sensorTypeId];
    bool isSubscribedCallback =
        std::any_of(callbackInfos.begin(), callbackInfos.end(), [env, callbackRef](const CallbackObject *obj) {
            ani_boolean isEqual = false;
            return (env->Reference_StrictEquals(callbackRef, obj->ref, &isEqual) == ANI_OK) && isEqual;
        });
    if (isSubscribedCallback) {
        env->GlobalReference_Delete(callbackRef);
        SEN_HILOGE("Callback is already subscribed, sensorTypeId:%{public}d", sensorTypeId);
        return;
    }
    sptr<CallbackObject> taiheCallbackInfo = new (std::nothrow) CallbackObject(cb, callbackRef);
    if (taiheCallbackInfo == nullptr) {
        SEN_HILOGE("taiheCallbackInfo is nullptr");
        return;
    }
    callbackInfos.push_back(taiheCallbackInfo);
    g_onceCallbackInfos[sensorTypeId] = callbackInfos;
}

void OnceCommon(int32_t sensorTypeId, callbackType cb, uintptr_t opq)
{
    if (!CheckSubscribe(sensorTypeId)) {
        SEN_HILOGD("No subscription to change sensor data, registration is required");
        int32_t ret = SubscribeSensor(sensorTypeId, REPORTING_INTERVAL, DataCallbackImpl);
        if (ret != ERR_OK) {
            taihe::set_business_error(ret, "SubscribeSensor fail");
            return;
        }
    }
    UpdateOnceCallback(sensorTypeId, cb, opq);
}

void OnceWearDetection(callback_view<void(WearDetectionResponse const &)> f, uintptr_t opq)
{
    OnceCommon(SENSOR_TYPE_ID_WEAR_DETECTION, f, opq);
}

int32_t RemoveAllCallback(int32_t sensorTypeId)
{
    std::lock_guard<std::mutex> onCallbackLock(g_onMutex);
    std::vector<sptr<CallbackObject>> callbackInfos = g_onCallbackInfos[sensorTypeId];
    for (auto iter = callbackInfos.begin(); iter != callbackInfos.end();) {
        CHKPC(*iter);
        if (auto *env = taihe::get_env()) {
            env->GlobalReference_Delete((*iter)->ref);
        }
        iter = callbackInfos.erase(iter);
    }
    if (callbackInfos.empty()) {
        SEN_HILOGD("No subscription to change sensor data");
        g_onCallbackInfos.erase(sensorTypeId);
        return 0;
    }
    g_onCallbackInfos[sensorTypeId] = callbackInfos;
    return static_cast<int32_t>(callbackInfos.size());
}

int32_t RemoveCallback(int32_t sensorTypeId, uintptr_t opq)
{
    std::lock_guard<std::mutex> onCallbackLock(g_onMutex);
    ani_object callbackObj = reinterpret_cast<ani_object>(opq);
    ani_ref callbackRef;
    ani_env *env = taihe::get_env();
    if (env == nullptr || env->GlobalReference_Create(callbackObj, &callbackRef) != ANI_OK) {
        SEN_HILOGE("Failed to create callbackRef, sensorTypeId:%{public}d", sensorTypeId);
        return 0;
    }
    std::vector<sptr<CallbackObject>> callbackInfos = g_onCallbackInfos[sensorTypeId];
    for (auto iter = callbackInfos.begin(); iter != callbackInfos.end();) {
        CHKPC(*iter);
        ani_boolean isEqual = false;
        if ((env->Reference_StrictEquals(callbackRef, (*iter)->ref, &isEqual) == ANI_OK) && isEqual) {
            env->GlobalReference_Delete((*iter)->ref);
            iter = callbackInfos.erase(iter);
            SEN_HILOGD("Remove callback success");
            break;
        } else {
            ++iter;
        }
    }
    env->GlobalReference_Delete(callbackRef);
    if (callbackInfos.empty()) {
        SEN_HILOGD("No subscription to change sensor data");
        g_onCallbackInfos.erase(sensorTypeId);
        return 0;
    }
    g_onCallbackInfos[sensorTypeId] = callbackInfos;
    return static_cast<int32_t>(callbackInfos.size());
}

void OffCommon(int32_t sensorTypeId, optional_view<uintptr_t> opq)
{
    int32_t subscribeSize = -1;
    if (!opq.has_value()) {
        subscribeSize = RemoveAllCallback(sensorTypeId);
    } else {
        subscribeSize = RemoveCallback(sensorTypeId, opq.value());
    }
    if (CheckSystemSubscribe(sensorTypeId) || (subscribeSize > 0)) {
        SEN_HILOGW("There are other client subscribe system js api as well, not need unsubscribe");
        return;
    }
    int32_t ret = UnsubscribeSensor(sensorTypeId);
    if (ret != ERR_OK) {
        SEN_HILOGE("UnsubscribeSensor fail, ret:%{public}d", ret);
        taihe::set_business_error(ret, "UnsubscribeSensor fail");
        return;
    }
}

void OffWearDetection(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_WEAR_DETECTION, opq);
}

void OnSignificantMotion(callback_view<void(SignificantMotionResponse const &)> f, uintptr_t opq,
    optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_SIGNIFICANT_MOTION, f, opq, options);
}

void OnceSignificantMotion(callback_view<void(SignificantMotionResponse const &)> f, uintptr_t opq)
{
    OnceCommon(SENSOR_TYPE_ID_SIGNIFICANT_MOTION, f, opq);
}

void OffSignificantMotion(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_SIGNIFICANT_MOTION, opq);
}

void OnRotationVector(callback_view<void(RotationVectorResponse const &)> f, uintptr_t opq,
    optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_ROTATION_VECTOR, f, opq, options);
}

void OnceRotationVector(callback_view<void(RotationVectorResponse const &)> f, uintptr_t opq)
{
    OnceCommon(SENSOR_TYPE_ID_ROTATION_VECTOR, f, opq);
}

void OffRotationVector(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_ROTATION_VECTOR, opq);
}

void OnProximity(callback_view<void(ProximityResponse const &)> f, uintptr_t opq, optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_PROXIMITY, f, opq, options);
}

void OnceProximity(callback_view<void(ProximityResponse const &)> f, uintptr_t opq)
{
    OnceCommon(SENSOR_TYPE_ID_PROXIMITY, f, opq);
}

void OffProximity(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_PROXIMITY, opq);
}

void OnPedometerDetection(callback_view<void(PedometerDetectionResponse const &)> f, uintptr_t opq,
    optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_PEDOMETER_DETECTION, f, opq, options);
}

void OncePedometerDetection(callback_view<void(PedometerDetectionResponse const &)> f, uintptr_t opq)
{
    OnceCommon(SENSOR_TYPE_ID_PEDOMETER_DETECTION, f, opq);
}

void OffPedometerDetection(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_PEDOMETER_DETECTION, opq);
}

void OnPedometer(callback_view<void(PedometerResponse const &)> f, uintptr_t opq, optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_PEDOMETER, f, opq, options);
}

void OncePedometer(callback_view<void(PedometerResponse const &)> f, uintptr_t opq)
{
    OnceCommon(SENSOR_TYPE_ID_PEDOMETER, f, opq);
}

void OffPedometer(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_PEDOMETER, opq);
}

void OnOrientation(callback_view<void(OrientationResponse const &)> f, uintptr_t opq, optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_ORIENTATION, f, opq, options);
}

void OnceOrientation(callback_view<void(OrientationResponse const &)> f, uintptr_t opq)
{
    OnceCommon(SENSOR_TYPE_ID_ORIENTATION, f, opq);
}

void OffOrientation(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_ORIENTATION, opq);
}

void OnMagneticFieldUncalibrated(callback_view<void(MagneticFieldUncalibratedResponse const &)> f, uintptr_t opq,
    optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_MAGNETIC_FIELD_UNCALIBRATED, f, opq, options);
}

void OnceMagneticFieldUncalibrated(callback_view<void(MagneticFieldUncalibratedResponse const &)> f, uintptr_t opq)
{
    OnceCommon(SENSOR_TYPE_ID_MAGNETIC_FIELD_UNCALIBRATED, f, opq);
}

void OffMagneticFieldUncalibrated(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_MAGNETIC_FIELD_UNCALIBRATED, opq);
}

void OnMagneticField(callback_view<void(MagneticFieldResponse const &)> f, uintptr_t opq,
    optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_MAGNETIC_FIELD, f, opq, options);
}

void OnceMagneticField(callback_view<void(MagneticFieldResponse const &)> f, uintptr_t opq)
{
    OnceCommon(SENSOR_TYPE_ID_MAGNETIC_FIELD, f, opq);
}

void OffMagneticField(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_MAGNETIC_FIELD, opq);
}

void OnLinearAccelerometer(callback_view<void(LinearAccelerometerResponse const &)> f, uintptr_t opq,
    optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_LINEAR_ACCELERATION, f, opq, options);
}

void OnceLinearAccelerometer(callback_view<void(LinearAccelerometerResponse const &)> f, uintptr_t opq)
{
    OnceCommon(SENSOR_TYPE_ID_LINEAR_ACCELERATION, f, opq);
}

void OffLinearAccelerometer(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_LINEAR_ACCELERATION, opq);
}

void OnHumidity(callback_view<void(HumidityResponse const &)> f, uintptr_t opq, optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_HUMIDITY, f, opq, options);
}

void OnceHumidity(callback_view<void(HumidityResponse const &)> f, uintptr_t opq)
{
    OnceCommon(SENSOR_TYPE_ID_HUMIDITY, f, opq);
}

void OffHumidity(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_HUMIDITY, opq);
}

void OnHeartRate(callback_view<void(HeartRateResponse const &)> f, uintptr_t opq, optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_HEART_RATE, f, opq, options);
}

void OnceHeartRate(callback_view<void(HeartRateResponse const &)> f, uintptr_t opq)
{
    OnceCommon(SENSOR_TYPE_ID_HEART_RATE, f, opq);
}

void OffHeartRate(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_HEART_RATE, opq);
}

void OnHall(callback_view<void(HallResponse const &)> f, uintptr_t opq, optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_HALL, f, opq, options);
}

void OnceHall(callback_view<void(HallResponse const &)> f, uintptr_t opq)
{
    OnceCommon(SENSOR_TYPE_ID_HALL, f, opq);
}

void OffHall(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_HALL, opq);
}

void OnGyroscopeUncalibrated(callback_view<void(GyroscopeUncalibratedResponse const &)> f, uintptr_t opq,
    optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_GYROSCOPE_UNCALIBRATED, f, opq, options);
}

void OnceGyroscopeUncalibrated(callback_view<void(GyroscopeUncalibratedResponse const &)> f, uintptr_t opq)
{
    OnceCommon(SENSOR_TYPE_ID_GYROSCOPE_UNCALIBRATED, f, opq);
}

void OffGyroscopeUncalibrated(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_GYROSCOPE_UNCALIBRATED, opq);
}

void OnGyroscope(callback_view<void(GyroscopeResponse const &)> f, uintptr_t opq, optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_GYROSCOPE, f, opq, options);
}

void OnceGyroscope(callback_view<void(GyroscopeResponse const &)> f, uintptr_t opq)
{
    OnceCommon(SENSOR_TYPE_ID_GYROSCOPE, f, opq);
}

void OffGyroscope(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_GYROSCOPE, opq);
}

void OnGravity(callback_view<void(GravityResponse const &)> f, uintptr_t opq, optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_GRAVITY, f, opq, options);
}

void OnceGravity(callback_view<void(GravityResponse const &)> f, uintptr_t opq)
{
    OnceCommon(SENSOR_TYPE_ID_GRAVITY, f, opq);
}

void OffGravity(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_GRAVITY, opq);
}

void OnBarometer(callback_view<void(BarometerResponse const &)> f, uintptr_t opq, optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_BAROMETER, f, opq, options);
}

void OnceBarometer(callback_view<void(BarometerResponse const &)> f, uintptr_t opq)
{
    OnceCommon(SENSOR_TYPE_ID_BAROMETER, f, opq);
}

void OffBarometer(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_BAROMETER, opq);
}

void OnAmbientTemperature(callback_view<void(AmbientTemperatureResponse const &)> f, uintptr_t opq,
    optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_AMBIENT_TEMPERATURE, f, opq, options);
}

void OnceAmbientTemperature(callback_view<void(AmbientTemperatureResponse const &)> f, uintptr_t opq)
{
    OnceCommon(SENSOR_TYPE_ID_AMBIENT_TEMPERATURE, f, opq);
}

void OffAmbientTemperature(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_AMBIENT_TEMPERATURE, opq);
}

void OnAmbientLight(callback_view<void(LightResponse const &)> f, uintptr_t opq, optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_AMBIENT_LIGHT, f, opq, options);
}

void OnceAmbientLight(callback_view<void(LightResponse const &)> f, uintptr_t opq)
{
    OnceCommon(SENSOR_TYPE_ID_AMBIENT_LIGHT, f, opq);
}

void OffAmbientLight(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_AMBIENT_LIGHT, opq);
}

void OnAccelerometerUncalibrated(callback_view<void(AccelerometerUncalibratedResponse const &)> f, uintptr_t opq,
    optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_ACCELEROMETER_UNCALIBRATED, f, opq, options);
}

void OnceAccelerometerUncalibrated(callback_view<void(AccelerometerUncalibratedResponse const &)> f, uintptr_t opq)
{
    OnceCommon(SENSOR_TYPE_ID_ACCELEROMETER_UNCALIBRATED, f, opq);
}

void OffAccelerometerUncalibrated(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_ACCELEROMETER_UNCALIBRATED, opq);
}

void OnAccelerometer(callback_view<void(AccelerometerResponse const &)> f, uintptr_t opq,
    optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_ACCELEROMETER, f, opq, options);
}

void OnceAccelerometer(callback_view<void(AccelerometerResponse const &)> f, uintptr_t opq)
{
    OnceCommon(SENSOR_TYPE_ID_ACCELEROMETER, f, opq);
}

void OffAccelerometer(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_ACCELEROMETER, opq);
}

void OnSar(callback_view<void(SarResponse const &)> f, uintptr_t opq, optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_SAR, f, opq, options);
}

void OffSar(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_SAR, opq);
}

void OnColor(callback_view<void(ColorResponse const &)> f, uintptr_t opq, optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_COLOR, f, opq, options);
}

void OffColor(optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_COLOR, opq);
}
} // namespace

// Since there macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_getRotationMatrixSyncGrav(getRotationMatrixSyncGrav);
TH_EXPORT_CPP_API_getOrientationSync(getOrientationSync);
TH_EXPORT_CPP_API_getRotationMatrixSync(getRotationMatrixSync);
TH_EXPORT_CPP_API_getSensorListSync(getSensorListSync);
TH_EXPORT_CPP_API_OnWearDetection(OnWearDetection);
TH_EXPORT_CPP_API_OnceWearDetection(OnceWearDetection);
TH_EXPORT_CPP_API_OffWearDetection(OffWearDetection);
TH_EXPORT_CPP_API_OnSignificantMotion(OnSignificantMotion);
TH_EXPORT_CPP_API_OnceSignificantMotion(OnceSignificantMotion);
TH_EXPORT_CPP_API_OffSignificantMotion(OffSignificantMotion);
TH_EXPORT_CPP_API_OnRotationVector(OnRotationVector);
TH_EXPORT_CPP_API_OnceRotationVector(OnceRotationVector);
TH_EXPORT_CPP_API_OffRotationVector(OffRotationVector);
TH_EXPORT_CPP_API_OnProximity(OnProximity);
TH_EXPORT_CPP_API_OnceProximity(OnceProximity);
TH_EXPORT_CPP_API_OffProximity(OffProximity);
TH_EXPORT_CPP_API_OnPedometerDetection(OnPedometerDetection);
TH_EXPORT_CPP_API_OncePedometerDetection(OncePedometerDetection);
TH_EXPORT_CPP_API_OffPedometerDetection(OffPedometerDetection);
TH_EXPORT_CPP_API_OnPedometer(OnPedometer);
TH_EXPORT_CPP_API_OncePedometer(OncePedometer);
TH_EXPORT_CPP_API_OffPedometer(OffPedometer);
TH_EXPORT_CPP_API_OnOrientation(OnOrientation);
TH_EXPORT_CPP_API_OnceOrientation(OnceOrientation);
TH_EXPORT_CPP_API_OffOrientation(OffOrientation);
TH_EXPORT_CPP_API_OnMagneticFieldUncalibrated(OnMagneticFieldUncalibrated);
TH_EXPORT_CPP_API_OnceMagneticFieldUncalibrated(OnceMagneticFieldUncalibrated);
TH_EXPORT_CPP_API_OffMagneticFieldUncalibrated(OffMagneticFieldUncalibrated);
TH_EXPORT_CPP_API_OnMagneticField(OnMagneticField);
TH_EXPORT_CPP_API_OnceMagneticField(OnceMagneticField);
TH_EXPORT_CPP_API_OffMagneticField(OffMagneticField);
TH_EXPORT_CPP_API_OnLinearAccelerometer(OnLinearAccelerometer);
TH_EXPORT_CPP_API_OnceLinearAccelerometer(OnceLinearAccelerometer);
TH_EXPORT_CPP_API_OffLinearAccelerometer(OffLinearAccelerometer);
TH_EXPORT_CPP_API_OnHumidity(OnHumidity);
TH_EXPORT_CPP_API_OnceHumidity(OnceHumidity);
TH_EXPORT_CPP_API_OffHumidity(OffHumidity);
TH_EXPORT_CPP_API_OnHeartRate(OnHeartRate);
TH_EXPORT_CPP_API_OnceHeartRate(OnceHeartRate);
TH_EXPORT_CPP_API_OffHeartRate(OffHeartRate);
TH_EXPORT_CPP_API_OnHall(OnHall);
TH_EXPORT_CPP_API_OnceHall(OnceHall);
TH_EXPORT_CPP_API_OffHall(OffHall);
TH_EXPORT_CPP_API_OnGyroscopeUncalibrated(OnGyroscopeUncalibrated);
TH_EXPORT_CPP_API_OnceGyroscopeUncalibrated(OnceGyroscopeUncalibrated);
TH_EXPORT_CPP_API_OffGyroscopeUncalibrated(OffGyroscopeUncalibrated);
TH_EXPORT_CPP_API_OnGyroscope(OnGyroscope);
TH_EXPORT_CPP_API_OnceGyroscope(OnceGyroscope);
TH_EXPORT_CPP_API_OffGyroscope(OffGyroscope);
TH_EXPORT_CPP_API_OnGravity(OnGravity);
TH_EXPORT_CPP_API_OnceGravity(OnceGravity);
TH_EXPORT_CPP_API_OffGravity(OffGravity);
TH_EXPORT_CPP_API_OnBarometer(OnBarometer);
TH_EXPORT_CPP_API_OnceBarometer(OnceBarometer);
TH_EXPORT_CPP_API_OffBarometer(OffBarometer);
TH_EXPORT_CPP_API_OnAmbientTemperature(OnAmbientTemperature);
TH_EXPORT_CPP_API_OnceAmbientTemperature(OnceAmbientTemperature);
TH_EXPORT_CPP_API_OffAmbientTemperature(OffAmbientTemperature);
TH_EXPORT_CPP_API_OnAmbientLight(OnAmbientLight);
TH_EXPORT_CPP_API_OnceAmbientLight(OnceAmbientLight);
TH_EXPORT_CPP_API_OffAmbientLight(OffAmbientLight);
TH_EXPORT_CPP_API_OnAccelerometerUncalibrated(OnAccelerometerUncalibrated);
TH_EXPORT_CPP_API_OnceAccelerometerUncalibrated(OnceAccelerometerUncalibrated);
TH_EXPORT_CPP_API_OffAccelerometerUncalibrated(OffAccelerometerUncalibrated);
TH_EXPORT_CPP_API_OnAccelerometer(OnAccelerometer);
TH_EXPORT_CPP_API_OnceAccelerometer(OnceAccelerometer);
TH_EXPORT_CPP_API_OffAccelerometer(OffAccelerometer);
TH_EXPORT_CPP_API_OnSar(OnSar);
TH_EXPORT_CPP_API_OffSar(OffSar);
TH_EXPORT_CPP_API_OnColor(OnColor);
TH_EXPORT_CPP_API_OffColor(OffColor);
// NOLINTEND