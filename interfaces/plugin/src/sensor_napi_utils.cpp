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

#include "sensor_napi_utils.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "hilog/log.h"

using namespace OHOS::HiviewDFX;
static constexpr HiLogLabel LABEL = {LOG_CORE, 0xD002708, "SensorJsAPI"};

bool IsNapiValueSame(napi_env env, napi_value lhs, napi_value rhs)
{
    bool result = false;
    napi_strict_equals(env, lhs, rhs, &result);
    return result;
}

bool IsMatchType(napi_env env, napi_value value, napi_valuetype type)
{
    napi_valuetype paramType = napi_undefined;
    napi_typeof(env, value, &paramType);
    if (paramType != type) {
        HiLog::Error(LABEL, "%{public}s  failed!", __func__);
        return false;
    }
    return true;
}

bool IsMatchArrayType(napi_env env, napi_value value)
{
    bool result = false;
    napi_is_array(env, value, &result);
    return result;
}

vector<float> GetCppArrayFloat(napi_env env, napi_value value)
{
    uint32_t arrayLength = 0;
    napi_get_array_length(env, value, &arrayLength);
    if (arrayLength <= 0) {
        HiLog::Error(LABEL, "%{public}s The array is empty.", __func__);
        return vector<float>();
    }
    vector<float> paramArrays;
    for (size_t i = 0; i < arrayLength; i++) {
        napi_value napiElement = nullptr;
        napi_get_element(env, value, i, &napiElement);

        napi_valuetype valuetype0 = napi_undefined;
        napi_typeof(env, napiElement, &valuetype0);
        if (valuetype0 != napi_number) {
            HiLog::Error(LABEL, "%{public}s Wrong argument type. Numbers expected.", __func__);
            return vector<float>();
        }
        double value0 = 0;
        napi_get_value_double(env, napiElement, &value0);
        paramArrays.push_back(static_cast<float>(value0));
    }
    return paramArrays;
}

bool IsMatchArrayType(napi_env env, napi_value value, napi_typedarray_type type)
{
    napi_typedarray_type paramType = napi_int8_array;
    size_t length;
    void *data = nullptr;
    napi_value arraybuffer = nullptr;
    size_t byteOffset;
    napi_get_typedarray_info(env, value, &paramType, &length, &data, &arraybuffer, &byteOffset);
    if (paramType != type) {
        HiLog::Error(LABEL, "%{public}s paramType:%{public}d type:%{public}d failed!", __func__, paramType, type);
        return false;
    }
    return true;
}

napi_value GetNapiInt32(int32_t number, napi_env env)
{
    napi_value value = nullptr;
    napi_create_int32(env, number, &value);
    return value;
}

napi_value NapiGetNamedProperty(napi_value jsonObject, string name, napi_env env)
{
    napi_value value = nullptr;
    napi_get_named_property(env, jsonObject, name.c_str(), &value);
    return value;
}

float GetCppFloat(napi_env env, napi_value value)
{
    double number;
    napi_get_value_double(env, value, &number);
    return static_cast<float>(number);
}

int32_t GetCppInt32(napi_value value, napi_env env)
{
    int32_t number;
    napi_get_value_int32(env, value, &number);
    return number;
}

int64_t GetCppInt64(napi_value value, napi_env env)
{
    int64_t number;
    napi_get_value_int64(env, value, &number);
    return number;
}

bool GetCppBool(napi_value value, napi_env env)
{
    bool number;
    napi_get_value_bool(env, value, &number);
    return number;
}

napi_value GetUndefined(napi_env env)
{
    napi_value value;
    napi_get_undefined(env, &value);
    return value;
}

std::map<int32_t, vector<string>> g_sensorAttributeList = {
    { 0, { "x" } },
    { SENSOR_TYPE_ID_ACCELEROMETER, { "x", "y", "z" } },
    { SENSOR_TYPE_ID_GYROSCOPE, { "x", "y", "z" } },
    { SENSOR_TYPE_ID_AMBIENT_LIGHT, { "intensity" } },
    { SENSOR_TYPE_ID_MAGNETIC_FIELD, { "x", "y", "z" } },
    { SENSOR_TYPE_ID_BAROMETER, { "pressure" } },
    { SENSOR_TYPE_ID_HALL, { "status" } },
    { SENSOR_TYPE_ID_PROXIMITY, { "distance" } },
    { SENSOR_TYPE_ID_HUMIDITY, { "humidity" } },
    { SENSOR_TYPE_ID_ORIENTATION, { "x", "y", "z" } },
    { SENSOR_TYPE_ID_GRAVITY, { "x", "y", "z" } },
    { SENSOR_TYPE_ID_LINEAR_ACCELERATION, { "x", "y", "z" } },
    { SENSOR_TYPE_ID_ROTATION_VECTOR, { "x", "y", "z", "w" } },
    { SENSOR_TYPE_ID_AMBIENT_TEMPERATURE, { "temperature" } },
    { SENSOR_TYPE_ID_MAGNETIC_FIELD_UNCALIBRATED, { "x", "y", "z", "biasX", "biasY", "biasZ" } },
    { SENSOR_TYPE_ID_GYROSCOPE_UNCALIBRATED, { "x", "y", "z", "biasX", "biasY", "biasZ" } },
    { SENSOR_TYPE_ID_SIGNIFICANT_MOTION, { "scalar" } },
    { SENSOR_TYPE_ID_PEDOMETER_DETECTION, { "scalar" } },
    { SENSOR_TYPE_ID_PEDOMETER, { "step" } },
    { SENSOR_TYPE_ID_HEART_RATE, { "heartRate" } },
    { SENSOR_TYPE_ID_WEAR_DETECTION, { "value" } },
    { SENSOR_TYPE_ID_ACCELEROMETER_UNCALIBRATED, { "x", "y", "z", "biasX", "biasY", "biasZ" } }
};

std::map<int32_t, ConvertDataFunc> g_convertfuncList = {
    {FAIL, ConvertToFailData},
    {OFF_CALLBACK, ConvertToNoData},
    {ON_CALLBACK, ConvertToSensorData},
    {ONCE_CALLBACK, ConvertToSensorData},
    {GET_GEOMAGNETIC_FIELD, ConvertToGeomagneticData},
    {GET_ALTITUDE, ConvertToNumber},
    {GET_GEOMAGNITIC_DIP, ConvertToNumber},
    {GET_ANGLE_MODIFY, ConvertToArray},
    {CREATE_ROTATION_MATRIX, ConvertToArray},
    {TRANSFORM_COORDINATE_SYSTEM, ConvertToArray},
    {CREATE_QUATERNION, ConvertToArray},
    {GET_DIRECTION, ConvertToArray},
    {ROTATION_INCLINATION_MATRIX, ConvertToRotationMatrix},
    {GET_SENSOR_LIST, ConvertToSensorInfos},
    {GET_SINGLE_SENSOR, ConvertToSingleSensor},
};

void getJsonObject(napi_env env, AsyncCallbackInfo *asyncCallbackInfo, napi_value result)
{
    napi_value x = nullptr;
    napi_create_double(env, asyncCallbackInfo->data.geomagneticData.x, &x);
    napi_set_named_property(env, result, "x", x);

    napi_value y = nullptr;
    napi_create_double(env, asyncCallbackInfo->data.geomagneticData.y, &y);
    napi_set_named_property(env, result, "y", y);

    napi_value z = nullptr;
    napi_create_double(env, asyncCallbackInfo->data.geomagneticData.z, &z);
    napi_set_named_property(env, result, "z", z);

    napi_value geomagneticDip = nullptr;
    napi_create_double(env, asyncCallbackInfo->data.geomagneticData.geomagneticDip, &geomagneticDip);
    napi_set_named_property(env, result, "geomagneticDip", geomagneticDip);

    napi_value deflectionAngle = nullptr;
    napi_create_double(env, asyncCallbackInfo->data.geomagneticData.deflectionAngle, &deflectionAngle);
    napi_set_named_property(env, result, "deflectionAngle", deflectionAngle);

    napi_value levelIntensity = nullptr;
    napi_create_double(env, asyncCallbackInfo->data.geomagneticData.levelIntensity, &levelIntensity);
    napi_set_named_property(env, result, "levelIntensity", levelIntensity);

    napi_value totalIntensity = nullptr;
    napi_create_double(env, asyncCallbackInfo->data.geomagneticData.totalIntensity, &totalIntensity);
    napi_set_named_property(env, result, "totalIntensity", totalIntensity);
}

void ConvertToSensorInfo(napi_env env, SensorInfo sensorInfo, napi_value result)
{
    napi_value sensorName = nullptr;
    napi_create_string_latin1(env, sensorInfo.sensorName, NAPI_AUTO_LENGTH, &sensorName);
    napi_set_named_property(env, result, "sensorName", sensorName);

    napi_value vendorName = nullptr;
    napi_create_string_latin1(env, sensorInfo.vendorName, NAPI_AUTO_LENGTH, &vendorName);
    napi_set_named_property(env, result, "vendorName", vendorName);

    napi_value firmwareVersion = nullptr;
    napi_create_string_latin1(env, sensorInfo.firmwareVersion, NAPI_AUTO_LENGTH, &firmwareVersion);
    napi_set_named_property(env, result, "firmwareVersion", firmwareVersion);

    napi_value hardwareVersion = nullptr;
    napi_create_string_latin1(env, sensorInfo.hardwareVersion, NAPI_AUTO_LENGTH, &hardwareVersion);
    napi_set_named_property(env, result, "hardwareVersion", hardwareVersion);

    napi_value sensorTypeId = nullptr;
    napi_create_double(env, sensorInfo.sensorTypeId, &sensorTypeId);
    napi_set_named_property(env, result, "sensorTypeId", sensorTypeId);

    napi_value maxRange = nullptr;
    napi_create_double(env, sensorInfo.maxRange, &maxRange);
    napi_set_named_property(env, result, "maxRange", maxRange);

    napi_value precision = nullptr;
    napi_create_double(env, sensorInfo.precision, &precision);
    napi_set_named_property(env, result, "precision", precision);

    napi_value power = nullptr;
    napi_create_double(env, sensorInfo.power, &power);
    napi_set_named_property(env, result, "power", power);
}

void ConvertToSingleSensor(napi_env env, AsyncCallbackInfo *asyncCallbackInfo, napi_value result[2])
{
    napi_get_undefined(env, &result[0]);
    napi_create_object(env, &result[1]);
    ConvertToSensorInfo(env, asyncCallbackInfo->sensorInfos[0], result[1]);
}

void ConvertToSensorInfos(napi_env env, AsyncCallbackInfo *asyncCallbackInfo, napi_value result[2])
{
    napi_get_undefined(env, &result[0]);
    napi_create_array(env, &result[1]);
    for (size_t i = 0; i < asyncCallbackInfo->sensorInfos.size(); i++) {
        napi_value sensorInfo = nullptr;
        napi_create_object(env, &sensorInfo);
        ConvertToSensorInfo(env, asyncCallbackInfo->sensorInfos[i], sensorInfo);
        napi_set_element(env, result[1], i, sensorInfo);
    }
}

void ConvertToFailData(napi_env env, AsyncCallbackInfo *asyncCallbackInfo, napi_value result[2])
{
    result[0] = GreateBusinessError(env, asyncCallbackInfo->error.code, asyncCallbackInfo->error.message,
        asyncCallbackInfo->error.name, asyncCallbackInfo->error.stack);
    napi_get_undefined(env, &result[1]);
}

void ConvertToSensorData(napi_env env, AsyncCallbackInfo *asyncCallbackInfo, napi_value result[2])
{
    napi_get_undefined(env, &result[0]);
    int32_t sensorTypeId = asyncCallbackInfo->data.sensorData.sensorTypeId;
    if ((g_sensorAttributeList.count(sensorTypeId)) == 0
        || g_sensorAttributeList[sensorTypeId].size()
        > (asyncCallbackInfo->data.sensorData.dataLength / sizeof(float))) {
        HiLog::Error(LABEL, "%{public}s count of sensorTypeId is zero", __func__);
        return;
    }
    std::vector<std::string> sensorAttribute = g_sensorAttributeList[sensorTypeId];
    napi_create_object(env, &result[1]);
    napi_value message = nullptr;
    for (size_t i = 0; i < sensorAttribute.size(); i++) {
        napi_create_double(env, asyncCallbackInfo->data.sensorData.data[i], &message);
        napi_set_named_property(env, result[1], sensorAttribute[i].c_str(), message);
        message = nullptr;
    }
    napi_create_int64(env, asyncCallbackInfo->data.sensorData.timestamp, &message);
    napi_set_named_property(env, result[1], "timestamp", message);
}

void ConvertToNoData(napi_env env, AsyncCallbackInfo *asyncCallbackInfo, napi_value result[2])
{
    napi_get_undefined(env, &result[1]);
    napi_get_undefined(env, &result[0]);
}

void ConvertToGeomagneticData(napi_env env, AsyncCallbackInfo *asyncCallbackInfo, napi_value result[2])
{
    napi_get_undefined(env, &result[0]);
    napi_create_object(env, &result[1]);
    getJsonObject(env, asyncCallbackInfo, result[1]);
}

void ConvertToNumber(napi_env env, AsyncCallbackInfo *asyncCallbackInfo, napi_value result[2])
{
    napi_get_undefined(env, &result[0]);
    napi_create_double(env, static_cast<double>(asyncCallbackInfo->data.reserveData.reserve[0]), &result[1]);
}

void ConvertToArray(napi_env env, AsyncCallbackInfo *asyncCallbackInfo, napi_value result[2])
{
    napi_get_undefined(env, &result[0]);
    napi_create_array(env, &result[1]);
    CreateNapiArray(env, asyncCallbackInfo->data.reserveData.reserve,
        asyncCallbackInfo->data.reserveData.length, result[1]);
}

void ConvertToRotationMatrix(napi_env env, AsyncCallbackInfo *asyncCallbackInfo, napi_value result[2])
{
    napi_get_undefined(env, &result[0]);
    napi_create_object(env, &result[1]);
    napi_value rotation = nullptr;
    napi_create_array(env, &rotation);
    CreateNapiArray(env, asyncCallbackInfo->data.rationMatrixData.rotationMatrix,
        THREE_DIMENSIONAL_MATRIX_LENGTH, rotation);
    napi_value inclination = nullptr;
    napi_create_array(env, &inclination);
    CreateNapiArray(env, asyncCallbackInfo->data.rationMatrixData.inclinationMatrix,
        THREE_DIMENSIONAL_MATRIX_LENGTH, inclination);
    napi_set_named_property(env, result[1], "rotation", rotation);
    napi_set_named_property(env, result[1], "inclination", inclination);
}

napi_value GreateBusinessError(napi_env env, int32_t errCode, string errMessage, string errName, string errStack)
{
    napi_value result = nullptr;
    napi_value code = nullptr;
    napi_value message = nullptr;
    napi_value name = nullptr;
    napi_value stack = nullptr;
    NAPI_CALL(env, napi_create_int32(env, errCode, &code));
    NAPI_CALL(env, napi_create_string_utf8(env, errMessage.data(), NAPI_AUTO_LENGTH, &message));
    NAPI_CALL(env, napi_create_string_utf8(env, errName.data(), NAPI_AUTO_LENGTH, &name));
    NAPI_CALL(env, napi_create_string_utf8(env, errStack.data(), NAPI_AUTO_LENGTH, &stack));
    NAPI_CALL(env, napi_create_object(env, &result));
    NAPI_CALL(env, napi_set_named_property(env, result, "code", code));
    NAPI_CALL(env, napi_set_named_property(env, result, "message", message));
    NAPI_CALL(env, napi_set_named_property(env, result, "name", name));
    NAPI_CALL(env, napi_set_named_property(env, result, "stack", stack));
    return result;
}

void CreateNapiArray(napi_env env, float *data, int32_t dataLength, napi_value result)
{
    for (int32_t index = 0; index < dataLength; index++) {
        napi_value message = nullptr;
        napi_create_double(env, data[index], &message);
        napi_set_element(env, result, index, message);
    }
}

void EmitAsyncCallbackWork(AsyncCallbackInfo *asyncCallbackInfo)
{
    HiLog::Debug(LABEL, "%{public}s begin", __func__);
    if (asyncCallbackInfo == nullptr) {
        HiLog::Error(LABEL, "%{public}s asyncCallbackInfo is null!", __func__);
        return;
    }
    napi_value resourceName = nullptr;
    if (napi_create_string_utf8(asyncCallbackInfo->env, "AsyncCallback", NAPI_AUTO_LENGTH, &resourceName) != napi_ok) {
        HiLog::Error(LABEL, "%{public}s create string utf8 failed", __func__);
        return;
    }
    napi_create_async_work(
        asyncCallbackInfo->env, nullptr, resourceName,
        [](napi_env env, void* data) {},
        [](napi_env env, napi_status status, void* data) {
            HiLog::Debug(LABEL, "%{public}s napi_create_async_work in", __func__);
            AsyncCallbackInfo *asyncCallbackInfo = reinterpret_cast<AsyncCallbackInfo *>(data);
            if (asyncCallbackInfo == nullptr) {
                HiLog::Error(LABEL, "%{public}s asyncCallbackInfo is null", __func__);
                return;
            }
            napi_value callback;
            napi_get_reference_value(env, asyncCallbackInfo->callback[0], &callback);
            napi_value callResult = nullptr;
            napi_value result[2] = {0};
            if (g_convertfuncList.count(asyncCallbackInfo->type) == 0) {
                HiLog::Error(LABEL, "%{public}s type invalid", __func__);
                return;
            }
            g_convertfuncList[asyncCallbackInfo->type](env, asyncCallbackInfo, result);
            napi_call_function(env, nullptr, callback, 2, result, &callResult);
            napi_delete_reference(env, asyncCallbackInfo->callback[0]);
            napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
            delete asyncCallbackInfo;
            asyncCallbackInfo = nullptr;
            HiLog::Debug(LABEL, "%{public}s napi_create_async_work left", __func__);
        },
        asyncCallbackInfo, &asyncCallbackInfo->asyncWork);

    napi_queue_async_work(asyncCallbackInfo->env, asyncCallbackInfo->asyncWork);
    HiLog::Debug(LABEL, "%{public}s end", __func__);
}

void EmitUvEventLoop(AsyncCallbackInfo **asyncCallbackInfo)
{
    uv_loop_s *loop(nullptr);
    if (asyncCallbackInfo == nullptr || *asyncCallbackInfo == nullptr
        || (*asyncCallbackInfo)->env == nullptr) {
        HiLog::Error(LABEL, "%{public}s asyncCallbackInfo is null", __func__);
        return;
    }
    napi_get_uv_event_loop((*asyncCallbackInfo)->env, &loop);
    if (loop == nullptr) {
        HiLog::Error(LABEL, "%{public}s loop is null", __func__);
        return;
    }
    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        HiLog::Error(LABEL, "%{public}s work is null", __func__);
        return;
    }
    work->data = reinterpret_cast<void *>(asyncCallbackInfo);
    uv_queue_work(loop, work, [] (uv_work_t *work) { }, [] (uv_work_t *work, int status) {
        AsyncCallbackInfo *asyncCallbackInfo = *reinterpret_cast<AsyncCallbackInfo **>(work->data);
        if (asyncCallbackInfo == nullptr || asyncCallbackInfo->env == nullptr
            || asyncCallbackInfo->callback[0] == nullptr) {
            HiLog::Error(LABEL, "%{public}s asyncCallbackInfo is null", __func__);
            return;
        }
        napi_env env = asyncCallbackInfo->env;
        napi_value undefined = nullptr;
        napi_get_undefined(env, &undefined);
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncCallbackInfo->callback[0], &callback);
        napi_value callResult = nullptr;
        napi_value result[2] = {0};
        if (g_convertfuncList.count(asyncCallbackInfo->type) == 0) {
            HiLog::Error(LABEL, "%{public}s type invalid", __func__);
            return;
        }
        g_convertfuncList[asyncCallbackInfo->type](env, asyncCallbackInfo, result);
        napi_call_function(env, undefined, callback, 1, &result[1], &callResult);
        if (asyncCallbackInfo->type != ON_CALLBACK) {
            napi_delete_reference(env, asyncCallbackInfo->callback[0]);
            delete asyncCallbackInfo;
            asyncCallbackInfo = nullptr;
        }
        delete work;
        work = nullptr;
    });
}

void EmitPromiseWork(AsyncCallbackInfo *asyncCallbackInfo)
{
    if (asyncCallbackInfo == nullptr) {
        HiLog::Error(LABEL, "%s asyncCallbackInfo is nullptr!", __func__);
        return;
    }
    napi_value resourceName = nullptr;
    napi_create_string_latin1(asyncCallbackInfo->env, "Promise", NAPI_AUTO_LENGTH, &resourceName);
    napi_create_async_work(
        asyncCallbackInfo->env, nullptr, resourceName, [](napi_env env, void* data) {},
        [](napi_env env, napi_status status, void* data) {
            AsyncCallbackInfo *asyncCallbackInfo = (AsyncCallbackInfo *)data;
            if (asyncCallbackInfo == nullptr) {
                HiLog::Error(LABEL, "%{public}s asyncCallbackInfo is null", __func__);
                return;
            }
            napi_value result[2] = {0};
            if (g_convertfuncList.count(asyncCallbackInfo->type) == 0) {
                HiLog::Error(LABEL, "%{public}s type invalid", __func__);
                return;
            }
            g_convertfuncList[asyncCallbackInfo->type](env, asyncCallbackInfo, result);
            if (asyncCallbackInfo->type == FAIL) {
                napi_reject_deferred(env, asyncCallbackInfo->deferred, result[0]);
            } else {
                napi_resolve_deferred(env, asyncCallbackInfo->deferred, result[1]);
            }
            napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
            delete asyncCallbackInfo;
            asyncCallbackInfo = nullptr;
        },
        (void*)asyncCallbackInfo, &asyncCallbackInfo->asyncWork);
    napi_queue_async_work(asyncCallbackInfo->env, asyncCallbackInfo->asyncWork);
}