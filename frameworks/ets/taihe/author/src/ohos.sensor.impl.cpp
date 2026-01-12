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

#include "geomagnetic_field.h"
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

using responseSensorData = std::variant<int32_t, int64_t, float, double, bool, string, ohos::sensor::SensorAccuracy>;
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
    taihe::callback<void(FusionPressureResponse const &)>,
    taihe::callback<void(ohos::sensor::SensorStatusEvent const &)>,
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
constexpr int32_t QUATERNION_LENGTH = 4;
constexpr int32_t REPORTING_INTERVAL = 200000000;
constexpr int32_t THREE_DIMENSIONAL_MATRIX_LENGTH = 9;
constexpr int32_t DATA_LENGTH = 16;
constexpr int32_t CALLBACK_MAX_DATA_LENGTH = 16;

constexpr int32_t DEFAULT_DEVICE_ID = 0;
constexpr int32_t INVALID_SENSOR_TYPE = -1;
constexpr int32_t GL_SENSOR_TYPE_PRIVATE_MIN_VALUE = 0x80000000;
constexpr int32_t SENSOR_TYPE_ID_AMBIENT_LIGHT1 = 5;
constexpr int32_t SENSOR_TYPE_ID_PROXIMITY1 = 8;
constexpr int32_t IS_LOCAL_DEVICE = 1;

std::mutex g_statusChangeMutex;
std::vector<sptr<CallbackObject>> g_statusChangeCallbackInfos;

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
void CallBackFusionPressure(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
void CallBackSensorStatusChange(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject);
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
        { SENSOR_TYPE_ID_FUSION_PRESSURE, CallBackFusionPressure },
    };

std::mutex g_mutex;
std::mutex g_bodyMutex;
std::map<int32_t, std::vector<sptr<CallbackObject>>> g_subscribeCallbacks;
std::mutex g_onMutex;
std::mutex g_onceMutex;
std::map<int32_t, std::vector<sptr<CallbackObject>>> g_onCallbackInfos;
std::map<int32_t, std::vector<sptr<CallbackObject>>> g_onceCallbackInfos;
std::mutex g_sensorTaiheAttrListMutex;

std::vector<float> transformDoubleToFloat(array_view<double> doubleArray)
{
    std::vector<float> floatArray;
    for (size_t i = 0; i < doubleArray.size(); ++i) {
        floatArray.push_back(static_cast<float>(doubleArray[i]));
    }
    return floatArray;
}

array<double> transformFloatToDouble(std::vector<float> floatArray)
{
    array<double> doubleArray = array<double>::make(floatArray.size());
    for (size_t i = 0; i < floatArray.size(); ++i) {
        doubleArray[i] = static_cast<double>(floatArray[i]);
    }
    return doubleArray;
}

RotationMatrixResponse getRotationMatrixSyncGrav(array_view<double> gravity, array_view<double> geomagnetic)
{
    std::vector<float> rotation(THREE_DIMENSIONAL_MATRIX_LENGTH);
    std::vector<float> inclination(THREE_DIMENSIONAL_MATRIX_LENGTH);
    SensorAlgorithm sensorAlgorithm;
    std::vector<float> gravityVector = transformDoubleToFloat(gravity);
    std::vector<float> geomagneticVector = transformDoubleToFloat(geomagnetic);
    int32_t ret = sensorAlgorithm.CreateRotationAndInclination(gravityVector, geomagneticVector, rotation, inclination);
    ohos::sensor::RotationMatrixResponse rsp = {
        .rotation = array<double>::make(THREE_DIMENSIONAL_MATRIX_LENGTH),
        .inclination = array<double>::make(THREE_DIMENSIONAL_MATRIX_LENGTH),
    };
    if (ret != OHOS::ERR_OK) {
        taihe::set_business_error(ret, "Create rotation and inclination matrix fail");
        return rsp;
    }
    for (int32_t i = 0; i < THREE_DIMENSIONAL_MATRIX_LENGTH; ++i) {
        rsp.rotation[i] = static_cast<double>(rotation[i]);
        rsp.inclination[i] = static_cast<double>(inclination[i]);
    }
    return rsp;
}

array<double> getOrientationSync(array_view<double> rotationMatrix)
{
    std::vector<float> rotationAngle(ROTATION_VECTOR_LENGTH);
    SensorAlgorithm sensorAlgorithm;
    std::vector<float> rotationMatrixVector = transformDoubleToFloat(rotationMatrix);
    int32_t ret = sensorAlgorithm.GetDirection(rotationMatrixVector, rotationAngle);
    if (ret != OHOS::ERR_OK) {
        taihe::set_business_error(ret, "Get direction fail");
        return transformFloatToDouble(rotationAngle);
    }
    return transformFloatToDouble(rotationAngle);
}

array<double> getRotationMatrixSync(array_view<double> rotation)
{
    std::vector<float> rotationMatrix(THREE_DIMENSIONAL_MATRIX_LENGTH);
    SensorAlgorithm sensorAlgorithm;
    std::vector<float> rotationVector = transformDoubleToFloat(rotation);
    int32_t ret = sensorAlgorithm.CreateRotationMatrix(rotationVector, rotationMatrix);
    if (ret != OHOS::ERR_OK) {
        taihe::set_business_error(ret, "Create rotation matrix fail");
        return transformFloatToDouble(rotationMatrix);
    }
    return transformFloatToDouble(rotationMatrix);
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
            .isMockSensor = taihe::optional<bool>(std::in_place_t{}, sensorInfos[i].isMockSensor),
        };
        result.push_back(sensorInfo);
    }
    return taihe::array<::ohos::sensor::Sensor>(result);
}

ohos::sensor::Sensor getSingleSensorSync(ohos::sensor::SensorId type)
{
    ohos::sensor::Sensor sensor = {
        .sensorName = "",
        .vendorName = "",
        .firmwareVersion = "",
        .hardwareVersion = "",
        .sensorId = 0,
        .maxRange = 0,
        .minSamplePeriod = 0,
        .maxSamplePeriod = 0,
        .precision = 0,
        .power = 0,
        .isMockSensor = taihe::optional<bool>(std::in_place_t{}, false)
    };
    int32_t count = 0;
    SensorInfo *sensorInfos = nullptr;
    int32_t ret = GetAllSensors(&sensorInfos, &count);
    if (ret != OHOS::ERR_OK) {
        taihe::set_business_error(ret, "Get sensor list fail");
        return sensor;
    }
    for (int32_t i = 0; i < count; ++i) {
        if (sensorInfos[i].sensorTypeId == type.get_value()) {
            sensor = {
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
                .isMockSensor = taihe::optional<bool>(std::in_place_t{}, sensorInfos[i].isMockSensor),
            };
            return sensor;
        }
    }
    taihe::set_business_error(SENSOR_NO_SUPPORT, "The sensor is not supported by the device");
    return sensor;
}

taihe::array<double> getQuaternionSync(taihe::array_view<double> rotationVector)
{
    SensorAlgorithm sensorAlgorithm;
    std::vector<float> vecRotation(transformDoubleToFloat(rotationVector));
    std::vector<float> vecQuaternion(QUATERNION_LENGTH);
    int32_t ret = sensorAlgorithm.CreateQuaternion(vecRotation, vecQuaternion);
    if (ret != OHOS::ERR_OK) {
        taihe::set_business_error(ret, "CreateQuaternion fail");
    }
    return transformFloatToDouble(vecQuaternion);
}

taihe::array<double> transformRotationMatrixSync(taihe::array_view<double> inRotationVector,
    ohos::sensor::CoordinatesOptions const& coordinates)
{
    std::vector<float> vecInRotation(transformDoubleToFloat(inRotationVector));
    size_t length = inRotationVector.size();
    if ((length != DATA_LENGTH) && (length != THREE_DIMENSIONAL_MATRIX_LENGTH)) {
        taihe::set_business_error(PARAMETER_ERROR, "Wrong inRotationVector length");
        return transformFloatToDouble(vecInRotation);
    }
    std::vector<float> vecOutRotation(length);
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.TransformCoordinateSystem(
        vecInRotation, coordinates.x, coordinates.y, vecOutRotation);
    if (ret != OHOS::ERR_OK) {
        taihe::set_business_error(ret, "Transform coordinate system fail");
    }
    return transformFloatToDouble(vecOutRotation);
}

taihe::array<double> getAngleVariationSync(taihe::array_view<double> currentRotationMatrix,
    taihe::array_view<double> preRotationMatrix)
{
    std::vector<float> angleChange(ROTATION_VECTOR_LENGTH);
    std::vector<float> curRotationVector(transformDoubleToFloat(currentRotationMatrix));
    std::vector<float> preRotationVector(transformDoubleToFloat(preRotationMatrix));
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.GetAngleModify(curRotationVector, preRotationVector, angleChange);
    if (ret != OHOS::ERR_OK) {
        taihe::set_business_error(ret, "Get angle modify fail");
    }
    return transformFloatToDouble(angleChange);
}

double getInclinationSync(taihe::array_view<double> inclinationMatrix)
{
    float geomagneticDip = 0;
    SensorAlgorithm sensorAlgorithm;
    std::vector<float> vecInclinationMatrix(transformDoubleToFloat(inclinationMatrix));
    int32_t ret = sensorAlgorithm.GetGeomagneticDip(vecInclinationMatrix, &geomagneticDip);
    if (ret != OHOS::ERR_OK) {
        taihe::set_business_error(ret, "Get geomagnetic dip fail");
    }
    return static_cast<double>(geomagneticDip);
}

double getDeviceAltitudeSync(double seaPressure, double currentPressure)
{
    float altitude = 0;
    SensorAlgorithm sensorAlgorithm;
    int32_t ret = sensorAlgorithm.GetAltitude(static_cast<float>(seaPressure),
        static_cast<float>(currentPressure), &altitude);
    if (ret != OHOS::ERR_OK) {
        taihe::set_business_error(ret, "Get altitude fail");
    }
    return static_cast<double>(altitude);
}

ohos::sensor::GeomagneticResponse getGeomagneticInfoSync(
    ohos::sensor::LocationOptions const& locationOptions, int64_t timeMillis)
{
    GeomagneticField geomagneticField(locationOptions.latitude,
        locationOptions.longitude, locationOptions.altitude, timeMillis);
    return ohos::sensor::GeomagneticResponse {
        .x = geomagneticField.ObtainX(),
        .y = geomagneticField.ObtainY(),
        .z = geomagneticField.ObtainZ(),
        .geomagneticDip = geomagneticField.ObtainGeomagneticDip(),
        .deflectionAngle = geomagneticField.ObtainDeflectionAngle(),
        .levelIntensity = geomagneticField.ObtainLevelIntensity(),
        .totalIntensity = geomagneticField.ObtainTotalIntensity()
    };
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
        .x = std::get<double>(data["x"]),
        .y = std::get<double>(data["y"]),
        .z = std::get<double>(data["z"]),
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
        .x = std::get<double>(data["x"]),
        .y = std::get<double>(data["y"]),
        .z = std::get<double>(data["z"]),
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
        .intensity = std::get<double>(data["intensity"]),
        .colorTemperature = taihe::optional<double>(std::in_place_t{}, std::get<double>(data["colorTemperature"])),
        .infraredLuminance = taihe::optional<double>(std::in_place_t{}, std::get<double>(data["infraredLuminance"])),
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
        .x = std::get<double>(data["x"]),
        .y = std::get<double>(data["y"]),
        .z = std::get<double>(data["z"]),
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
        .pressure = std::get<double>(data["pressure"]),
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
        .status = std::get<double>(data["status"]),
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
        .distance = std::get<double>(data["distance"]),
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
        .humidity = std::get<double>(data["humidity"]),
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
        .alpha = std::get<double>(data["alpha"]),
        .beta = std::get<double>(data["beta"]),
        .gamma = std::get<double>(data["gamma"]),
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
        .x = std::get<double>(data["x"]),
        .y = std::get<double>(data["y"]),
        .z = std::get<double>(data["z"]),
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
        .x = std::get<double>(data["x"]),
        .y = std::get<double>(data["y"]),
        .z = std::get<double>(data["z"]),
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
        .x = std::get<double>(data["x"]),
        .y = std::get<double>(data["y"]),
        .z = std::get<double>(data["z"]),
        .w = std::get<double>(data["w"]),
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
        .temperature = std::get<double>(data["temperature"]),
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
        .x = std::get<double>(data["x"]),
        .y = std::get<double>(data["y"]),
        .z = std::get<double>(data["z"]),
        .biasX = std::get<double>(data["biasX"]),
        .biasY = std::get<double>(data["biasY"]),
        .biasZ = std::get<double>(data["biasZ"]),
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
        .x = std::get<double>(data["x"]),
        .y = std::get<double>(data["y"]),
        .z = std::get<double>(data["z"]),
        .biasX = std::get<double>(data["biasX"]),
        .biasY = std::get<double>(data["biasY"]),
        .biasZ = std::get<double>(data["biasZ"]),
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
        .scalar = std::get<double>(data["scalar"]),
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
        .scalar = std::get<double>(data["scalar"]),
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
        .steps = std::get<double>(data["steps"]),
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
        .heartRate = std::get<double>(data["heartRate"]),
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
        .value = std::get<double>(data["value"]),
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
        .x = std::get<double>(data["x"]),
        .y = std::get<double>(data["y"]),
        .z = std::get<double>(data["z"]),
        .biasX = std::get<double>(data["biasX"]),
        .biasY = std::get<double>(data["biasY"]),
        .biasZ = std::get<double>(data["biasZ"]),
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
        .lightIntensity = std::get<double>(data["lightIntensity"]),
        .colorTemperature = std::get<double>(data["colorTemperature"]),
    };
    auto &func = std::get<taihe::callback<void(ColorResponse const &)>>(callbackObject->callback);
    func(responseData);
}

void CallBackFusionPressure(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(FusionPressureResponse const &)>>(callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback FusionPressureResponse function");
        return;
    }
    ohos::sensor::Response res = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .accuracy = std::get<ohos::sensor::SensorAccuracy>(data["accuracy"]),
    };
    FusionPressureResponse responseData = {
        .base = res,
        .fusionPressure = std::get<double>(data["fusionPressure"]),
    };
    auto &func = std::get<taihe::callback<void(FusionPressureResponse const &)>>(callbackObject->callback);
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
        .absorptionRatio = std::get<double>(data["absorptionRatio"]),
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
        dataMap.emplace(sensorAttributes[i].c_str(), static_cast<double>(dataNow[i]));
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

void PlugDataCallbackImpl(::SensorStatusEvent *plugEvent)
{
    if (plugEvent == nullptr) {
        SEN_HILOGE("event is null");
        return;
    }
    SEN_HILOGD("PlugDataCallbackImpl: timestamp=%" PRId64 ", sensorId=%d, isSensorOnline=%d,\
        deviceId=%d, deviceName=%s", plugEvent->timestamp, plugEvent->sensorId,
        plugEvent->isSensorOnline, plugEvent->deviceId, plugEvent->deviceName.c_str());
    std::lock_guard<std::mutex> lock(g_statusChangeMutex);
    if (g_statusChangeCallbackInfos.empty()) {
        SEN_HILOGD("PlugDataCallbackImpl: no sensor status change callback subscribed");
        return;
    }
    std::map<std::string, responseSensorData> dataMap;
    dataMap.emplace("timestamp", plugEvent->timestamp);
    dataMap.emplace("sensorId", plugEvent->sensorId);
    dataMap.emplace("isSensorOnline", static_cast<bool>(plugEvent->isSensorOnline != 0));
    dataMap.emplace("deviceId", plugEvent->deviceId);
    dataMap.emplace("deviceName", std::string(plugEvent->deviceName));
    for (const auto &callbackObj : g_statusChangeCallbackInfos) {
        CallBackSensorStatusChange(dataMap, callbackObj);
    }
}

const SensorUser user = {
    .callback = DataCallbackImpl,
    .plugCallback = PlugDataCallbackImpl
};

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

void OffWearDetection(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
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

void OffSignificantMotion(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
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

void OffRotationVector(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
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

void OffProximity(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
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

void OffPedometerDetection(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
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

void OffPedometer(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
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

void OffOrientation(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
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

void OffMagneticFieldUncalibrated(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
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

void OffMagneticField(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
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

void OffLinearAccelerometer(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
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

void OffHumidity(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
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

void OffHeartRate(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
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

void OffHall(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
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

void OffGyroscopeUncalibrated(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
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

void OffGyroscope(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
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

void OffGravity(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
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

void OffBarometer(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
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

void OffAmbientTemperature(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
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

void OffAmbientLight(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
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

void OffAccelerometerUncalibrated(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
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

void OffAccelerometer(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_ACCELEROMETER, opq);
}

void OnSar(callback_view<void(SarResponse const &)> f, uintptr_t opq, optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_SAR, f, opq, options);
}

void OffSar(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_SAR, opq);
}

void OnColor(callback_view<void(ColorResponse const &)> f, uintptr_t opq, optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_COLOR, f, opq, options);
}

void OffColor(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_COLOR, opq);
}

void OnFusionPressure(
    callback_view<void(FusionPressureResponse const &)> f, uintptr_t opq, optional_view<Options> options)
{
    OnCommon(SENSOR_TYPE_ID_FUSION_PRESSURE, f, opq, options);
}

void OffFusionPressure(optional_view<SensorInfoParam> sensorInfoParam, optional_view<uintptr_t> opq)
{
    OffCommon(SENSOR_TYPE_ID_FUSION_PRESSURE, opq);
}


void CallBackSensorStatusChange(std::map<std::string, responseSensorData> data, sptr<CallbackObject> callbackObject)
{
    if (callbackObject == nullptr) {
        SEN_HILOGE("callbackObject is null");
        return;
    }
    if (!std::holds_alternative<taihe::callback<void(ohos::sensor::SensorStatusEvent const &)>>(
        callbackObject->callback)) {
        SEN_HILOGE("callbackObject is not of type callback SensorStatusEvent function");
        return;
    }
    if (!data.count("timestamp") || !data.count("sensorId") || !data.count("deviceId") ||
        !data.count("isSensorOnline")) {
        SEN_HILOGE("SensorStatusChange data missing core field!");
        return;
    }
    ohos::sensor::SensorStatusEvent responseData = {
        .timestamp = std::get<int64_t>(data["timestamp"]),
        .sensorId = std::get<int32_t>(data["sensorId"]),
        .deviceId = std::get<int32_t>(data["deviceId"]),
        .sensorIndex = std::get<int32_t>(data["sensorIndex"]),
        .isSensorOnline = std::get<bool>(data["isSensorOnline"]),
        .deviceName = std::get<string>(data["deviceName"]),
    };

    auto &func = std::get<taihe::callback<void(ohos::sensor::SensorStatusEvent const &)>>(callbackObject->callback);
    func(responseData);
}


void UpdateStatusChangeCallbackInfos(callbackType callback, uintptr_t opq)
{
    std::lock_guard<std::mutex> lock(g_statusChangeMutex);
    ani_object callbackObj = reinterpret_cast<ani_object>(opq);
    ani_ref callbackRef;
    ani_env *env = taihe::get_env();

    if (env == nullptr || env->GlobalReference_Create(callbackObj, &callbackRef) != ANI_OK) {
        SEN_HILOGE("Failed to create status change callbackRef");
        return;
    }

    bool isSubscribed = std::any_of(g_statusChangeCallbackInfos.begin(), g_statusChangeCallbackInfos.end(),
        [env, callbackRef](const CallbackObject *obj) {
        ani_boolean isEqual = false;
        return (env->Reference_StrictEquals(callbackRef, obj->ref, &isEqual) == ANI_OK) && isEqual;
    });
    if (isSubscribed) {
        env->GlobalReference_Delete(callbackRef);
        SEN_HILOGE("Status change callback is already subscribed");
        return;
    }

    sptr<CallbackObject> callbackInfo = new (std::nothrow) CallbackObject(callback, callbackRef);
    if (callbackInfo == nullptr) {
        env->GlobalReference_Delete(callbackRef);
        SEN_HILOGE("Create status change CallbackObject failed");
        return;
    }

    g_statusChangeCallbackInfos.push_back(callbackInfo);
    SEN_HILOGI("Add status change callback success, count:%{public}zu", g_statusChangeCallbackInfos.size());
}


int32_t RemoveStatusChangeCallback(uintptr_t opq)
{
    std::lock_guard<std::mutex> lock(g_statusChangeMutex);
    ani_object callbackObj = reinterpret_cast<ani_object>(opq);
    ani_ref callbackRef;
    ani_env *env = taihe::get_env();

    if (env == nullptr || env->GlobalReference_Create(callbackObj, &callbackRef) != ANI_OK) {
        SEN_HILOGE("Failed to create status change callbackRef for remove");
        return 0;
    }

    for (auto iter = g_statusChangeCallbackInfos.begin(); iter != g_statusChangeCallbackInfos.end();) {
        CHKPC(*iter);
        ani_boolean isEqual = false;
        if ((env->Reference_StrictEquals(callbackRef, (*iter)->ref, &isEqual) == ANI_OK) && isEqual) {
            env->GlobalReference_Delete((*iter)->ref);
            iter = g_statusChangeCallbackInfos.erase(iter);
            SEN_HILOGD("Remove status change callback success");
        } else {
            ++iter;
        }
    }

    env->GlobalReference_Delete(callbackRef);
    return static_cast<int32_t>(g_statusChangeCallbackInfos.size());
}


int32_t RemoveAllStatusChangeCallback()
{
    std::lock_guard<std::mutex> lock(g_statusChangeMutex);
    ani_env *env = taihe::get_env();

    for (auto &info : g_statusChangeCallbackInfos) {
        CHKPC(info);
        if (env != nullptr) {
            env->GlobalReference_Delete(info->ref);
        }
    }

    g_statusChangeCallbackInfos.clear();
    SEN_HILOGD("Remove all status change callback success");
    return 0;
}


void OnSensorStatusChange(::taihe::callback_view<void(ohos::sensor::SensorStatusEvent const &)> f, uintptr_t opq)
{
    UpdateStatusChangeCallbackInfos(f, opq);
}


void OffSensorStatusChange(::taihe::callback_view<void(ohos::sensor::SensorStatusEvent const& info)> f, uintptr_t opq)
{
    int32_t subscribeSize = -1;
    if (opq != 0) {
        subscribeSize = RemoveStatusChangeCallback(opq);
    } else {
        subscribeSize = RemoveAllStatusChangeCallback();
    }
    if (subscribeSize > 0) {
        SEN_HILOGW("There are other status change subscribers, skip unsubscribe");
        return;
    }
    SEN_HILOGI("OffSensorStatusChange success, no more subscribers");
}

::ohos::sensor::Sensor getSingleSensorSyncFunc(::ohos::sensor::SensorId type)
{
    return getSingleSensorSync(type);
}

::taihe::array<::ohos::sensor::Sensor> getSensorListSyncFunc()
{
    return getSensorListSync();
}

int32_t GetLocalDeviceIdInner()
{
    SensorInfo* localSensors = nullptr;
    int32_t localCount = 0;
    int32_t localDeviceId = DEFAULT_DEVICE_ID;

    if (GetAllSensors(&localSensors, &localCount) == OHOS::ERR_OK && localSensors != nullptr && localCount > 0) {
        for (int32_t i = 0; i < localCount; ++i) {
            if (localSensors[i].location == IS_LOCAL_DEVICE) {
                localDeviceId = localSensors[i].deviceId;
                break;
            }
        }
        free(localSensors);
    }
    SEN_HILOGD("GetLocalDeviceIdInner: local deviceId=%{public}d", localDeviceId);
    return localDeviceId;
}

::taihe::array<::ohos::sensor::Sensor> FilterAndConvertSensorInner(const SensorInfo* sensorInfos,
    int32_t count, int32_t targetTypeId)
{
    std::vector<::ohos::sensor::Sensor> result;
    if (sensorInfos == nullptr || count <= 0 || targetTypeId == INVALID_SENSOR_TYPE) {
        return taihe::array<::ohos::sensor::Sensor>(result);
    }

    for (int32_t i = 0; i < count; ++i) {
        if ((sensorInfos[i].sensorTypeId == SENSOR_TYPE_ID_AMBIENT_LIGHT1) ||
            (sensorInfos[i].sensorTypeId == SENSOR_TYPE_ID_PROXIMITY1) ||
            (sensorInfos[i].sensorTypeId > GL_SENSOR_TYPE_PRIVATE_MIN_VALUE)) {
            SEN_HILOGD("FilterAndConvertSensorInner: skip secondary/private sensor, typeId=%{public}d",
                sensorInfos[i].sensorTypeId);
            continue;
        }

        if (sensorInfos[i].sensorTypeId == targetTypeId) {
                ohos::sensor::Sensor sensorInfo = {
                .sensorName = sensorInfos[i].sensorName,
                .sensorId = sensorInfos[i].sensorId,
                .hardwareVersion = sensorInfos[i].hardwareVersion,
                .vendorName  = sensorInfos[i].vendorName,
                .firmwareVersion = sensorInfos[i].firmwareVersion,
                .maxRange = sensorInfos[i].maxRange,
                .minSamplePeriod = sensorInfos[i].minSamplePeriod,
                .power = sensorInfos[i].power,
                .maxSamplePeriod = sensorInfos[i].maxSamplePeriod,
                .precision = sensorInfos[i].precision,
            };
            result.push_back(sensorInfo);
        }
    }
    return taihe::array<::ohos::sensor::Sensor>(result);
}

::taihe::array<::ohos::sensor::Sensor> getSingleSensorByDeviceSync(::ohos::sensor::SensorId type,
    ::taihe::optional_view<int32_t> deviceId)
{
    int32_t targetDeviceId = DEFAULT_DEVICE_ID;
    if (deviceId.has_value()) {
        targetDeviceId = deviceId.value();
        SEN_HILOGD("getSingleSensorByDeviceSync: specified deviceId=%{public}d", targetDeviceId);
    } else {
        targetDeviceId = GetLocalDeviceIdInner();
    }

    std::vector<::ohos::sensor::Sensor> result;
    int32_t sensorTypeId = type.get_value();
    if (sensorTypeId == INVALID_SENSOR_TYPE) {
        SEN_HILOGE("getSingleSensorByDeviceSync: invalid SensorId=%{public}d", static_cast<int32_t>(type));
        return taihe::array<::ohos::sensor::Sensor>(result);
    }

    SensorInfo* sensorInfos = nullptr;
    int32_t sensorCount = 0;
    int32_t ret = GetDeviceSensors(targetDeviceId, &sensorInfos, &sensorCount);
    if (ret != OHOS::ERR_OK || sensorInfos == nullptr || sensorCount <= 0) {
        SEN_HILOGE("getSingleSensorByDeviceSync: GetDeviceSensors failed,\
            deviceId=%{public}d, ret=%{public}d", targetDeviceId, ret);
        if (sensorInfos != nullptr) {
            free(sensorInfos);
            sensorInfos = nullptr;
        }
        return taihe::array<::ohos::sensor::Sensor>(result);
    }

    auto resultInfo = FilterAndConvertSensorInner(sensorInfos, sensorCount, sensorTypeId);

    if (sensorInfos != nullptr) {
        free(sensorInfos);
        sensorInfos = nullptr;
    }

    if (resultInfo.empty()) {
        SEN_HILOGW("getSingleSensorByDeviceSync: no sensor found, type=%{public}d, deviceId=%{public}d",
                   static_cast<int32_t>(type), targetDeviceId);
    } else {
        SEN_HILOGI("getSingleSensorByDeviceSync: found sensor, type=%{public}d, deviceId=%{public}d,\
            sensorId=%{public}d",
                   static_cast<int32_t>(type), targetDeviceId, resultInfo[0].sensorId);
    }

    return resultInfo;
}

::taihe::array<::ohos::sensor::Sensor> getSensorListByDeviceSync(::taihe::optional_view<int32_t> deviceId)
{
    int32_t targetDeviceId = DEFAULT_DEVICE_ID;
    if (deviceId.has_value()) {
        targetDeviceId = deviceId.value();
        SEN_HILOGD("getSensorListByDeviceSync: specified deviceId=%{public}d", targetDeviceId);
    } else {
        targetDeviceId = GetLocalDeviceIdInner();
    }

    SensorInfo* sensorInfos = nullptr;
    int32_t sensorCount = 0;
    int32_t ret = GetDeviceSensors(targetDeviceId, &sensorInfos, &sensorCount);
    std::vector<::ohos::sensor::Sensor> result;
    if (ret != OHOS::ERR_OK || sensorInfos == nullptr || sensorCount <= 0) {
        SEN_HILOGE("getSensorListByDeviceSync: GetDeviceSensors failed, deviceId=%{public}d, ret=%{public}d",
            targetDeviceId, ret);
        if (sensorInfos != nullptr) {
            free(sensorInfos);
            sensorInfos = nullptr;
        }
        return taihe::array<::ohos::sensor::Sensor>(result);
    }

    for (int32_t i = 0; i < sensorCount; ++i) {
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
} // namespace

// Since there macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_getRotationMatrixSyncGrav(getRotationMatrixSyncGrav);
TH_EXPORT_CPP_API_getOrientationSync(getOrientationSync);
TH_EXPORT_CPP_API_getRotationMatrixSync(getRotationMatrixSync);
TH_EXPORT_CPP_API_getSensorListSync(getSensorListSync);
TH_EXPORT_CPP_API_getSingleSensorSync(getSingleSensorSync);
TH_EXPORT_CPP_API_getQuaternionSync(getQuaternionSync);
TH_EXPORT_CPP_API_transformRotationMatrixSync(transformRotationMatrixSync);
TH_EXPORT_CPP_API_getAngleVariationSync(getAngleVariationSync);
TH_EXPORT_CPP_API_getInclinationSync(getInclinationSync);
TH_EXPORT_CPP_API_getDeviceAltitudeSync(getDeviceAltitudeSync);
TH_EXPORT_CPP_API_getGeomagneticInfoSync(getGeomagneticInfoSync);
TH_EXPORT_CPP_API_OnWearDetectionChange(OnWearDetection);
TH_EXPORT_CPP_API_OnceWearDetectionChange(OnceWearDetection);
TH_EXPORT_CPP_API_OffWearDetectionChange(OffWearDetection);
TH_EXPORT_CPP_API_OnSignificantMotionChange(OnSignificantMotion);
TH_EXPORT_CPP_API_OnceSignificantMotionChange(OnceSignificantMotion);
TH_EXPORT_CPP_API_OffSignificantMotionChange(OffSignificantMotion);
TH_EXPORT_CPP_API_OnRotationVectorChange(OnRotationVector);
TH_EXPORT_CPP_API_OnceRotationVectorChange(OnceRotationVector);
TH_EXPORT_CPP_API_OffRotationVectorChange(OffRotationVector);
TH_EXPORT_CPP_API_OnProximityChange(OnProximity);
TH_EXPORT_CPP_API_OnceProximityChange(OnceProximity);
TH_EXPORT_CPP_API_OffProximityChange(OffProximity);
TH_EXPORT_CPP_API_OnPedometerDetectionChange(OnPedometerDetection);
TH_EXPORT_CPP_API_OncePedometerDetectionChange(OncePedometerDetection);
TH_EXPORT_CPP_API_OffPedometerDetectionChange(OffPedometerDetection);
TH_EXPORT_CPP_API_OnPedometerChange(OnPedometer);
TH_EXPORT_CPP_API_OncePedometerChange(OncePedometer);
TH_EXPORT_CPP_API_OffPedometerChange(OffPedometer);
TH_EXPORT_CPP_API_OnOrientationChange(OnOrientation);
TH_EXPORT_CPP_API_OnceOrientationChange(OnceOrientation);
TH_EXPORT_CPP_API_OffOrientationChange(OffOrientation);
TH_EXPORT_CPP_API_OnMagneticFieldUncalibratedChange(OnMagneticFieldUncalibrated);
TH_EXPORT_CPP_API_OnceMagneticFieldUncalibratedChange(OnceMagneticFieldUncalibrated);
TH_EXPORT_CPP_API_OffMagneticFieldUncalibratedChange(OffMagneticFieldUncalibrated);
TH_EXPORT_CPP_API_OnMagneticFieldChange(OnMagneticField);
TH_EXPORT_CPP_API_OnceMagneticFieldChange(OnceMagneticField);
TH_EXPORT_CPP_API_OffMagneticFieldChange(OffMagneticField);
TH_EXPORT_CPP_API_OnLinearAccelerometerChange(OnLinearAccelerometer);
TH_EXPORT_CPP_API_OnceLinearAccelerometerChange(OnceLinearAccelerometer);
TH_EXPORT_CPP_API_OffLinearAccelerometerChange(OffLinearAccelerometer);
TH_EXPORT_CPP_API_OnHumidityChange(OnHumidity);
TH_EXPORT_CPP_API_OnceHumidityChange(OnceHumidity);
TH_EXPORT_CPP_API_OffHumidityChange(OffHumidity);
TH_EXPORT_CPP_API_OnHeartRateChange(OnHeartRate);
TH_EXPORT_CPP_API_OnceHeartRateChange(OnceHeartRate);
TH_EXPORT_CPP_API_OffHeartRateChange(OffHeartRate);
TH_EXPORT_CPP_API_OnHallChange(OnHall);
TH_EXPORT_CPP_API_OnceHallChange(OnceHall);
TH_EXPORT_CPP_API_OffHallChange(OffHall);
TH_EXPORT_CPP_API_OnGyroscopeUncalibratedChange(OnGyroscopeUncalibrated);
TH_EXPORT_CPP_API_OnceGyroscopeUncalibratedChange(OnceGyroscopeUncalibrated);
TH_EXPORT_CPP_API_OffGyroscopeUncalibratedChange(OffGyroscopeUncalibrated);
TH_EXPORT_CPP_API_OnGyroscopeChange(OnGyroscope);
TH_EXPORT_CPP_API_OnceGyroscopeChange(OnceGyroscope);
TH_EXPORT_CPP_API_OffGyroscopeChange(OffGyroscope);
TH_EXPORT_CPP_API_OnGravityChange(OnGravity);
TH_EXPORT_CPP_API_OnceGravityChange(OnceGravity);
TH_EXPORT_CPP_API_OffGravityChange(OffGravity);
TH_EXPORT_CPP_API_OnBarometerChange(OnBarometer);
TH_EXPORT_CPP_API_OnceBarometerChange(OnceBarometer);
TH_EXPORT_CPP_API_OffBarometerChange(OffBarometer);
TH_EXPORT_CPP_API_OnAmbientTemperatureChange(OnAmbientTemperature);
TH_EXPORT_CPP_API_OnceAmbientTemperatureChange(OnceAmbientTemperature);
TH_EXPORT_CPP_API_OffAmbientTemperatureChange(OffAmbientTemperature);
TH_EXPORT_CPP_API_OnAmbientLightChange(OnAmbientLight);
TH_EXPORT_CPP_API_OnceAmbientLightChange(OnceAmbientLight);
TH_EXPORT_CPP_API_OffAmbientLightChange(OffAmbientLight);
TH_EXPORT_CPP_API_OnAccelerometerUncalibratedChange(OnAccelerometerUncalibrated);
TH_EXPORT_CPP_API_OnceAccelerometerUncalibratedChange(OnceAccelerometerUncalibrated);
TH_EXPORT_CPP_API_OffAccelerometerUncalibratedChange(OffAccelerometerUncalibrated);
TH_EXPORT_CPP_API_OnAccelerometerChange(OnAccelerometer);
TH_EXPORT_CPP_API_OnceAccelerometerChange(OnceAccelerometer);
TH_EXPORT_CPP_API_OffAccelerometerChange(OffAccelerometer);
TH_EXPORT_CPP_API_OnSarChange(OnSar);
TH_EXPORT_CPP_API_OffSarChange(OffSar);
TH_EXPORT_CPP_API_OnColorChange(OnColor);
TH_EXPORT_CPP_API_OffColorChange(OffColor);
TH_EXPORT_CPP_API_OnFusionPressureChange(OnFusionPressure);
TH_EXPORT_CPP_API_OffFusionPressureChange(OffFusionPressure);
TH_EXPORT_CPP_API_getSingleSensorSyncFunc(getSingleSensorSyncFunc);
TH_EXPORT_CPP_API_getSensorListSyncFunc(getSensorListSyncFunc);
TH_EXPORT_CPP_API_getSingleSensorByDeviceSync(getSingleSensorByDeviceSync);
TH_EXPORT_CPP_API_getSensorListByDeviceSync(getSensorListByDeviceSync);
TH_EXPORT_CPP_API_OnSensorStatusChange(OnSensorStatusChange);
TH_EXPORT_CPP_API_OffSensorStatusChange(OffSensorStatusChange);
// NOLINTEND