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
#ifndef SENSOR_NAPI_UTILS_H
#define SENSOR_NAPI_UTILS_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include <uv.h>
#include "sensor_agent.h"
#include <iostream>

using std::vector;
using std::string;

const int32_t THREE_DIMENSIONAL_MATRIX_LENGTH = 9;
const int32_t FOUR_DIMENSIONAL_MATRIX_LENGTH = 16;
const int32_t QUATERNION_LENGTH = 4;
const int32_t ROTATION_VECTOR_LENGTH = 3;
struct GeomagneticData {
    float x;
    float y;
    float z;
    float geomagneticDip;
    float deflectionAngle;
    float levelIntensity;
    float totalIntensity;
};

struct RationMatrixData {
    float rotationMatrix[THREE_DIMENSIONAL_MATRIX_LENGTH];
    float inclinationMatrix[THREE_DIMENSIONAL_MATRIX_LENGTH];
};

struct SensorData {
    int32_t sensorTypeId;
    int32_t dataLength;
    float data[16];
    int64_t timestamp;
};

struct ReserveData {
    float reserve[16];
    int32_t length;
};

union CallbackData {
    SensorData sensorData;
    GeomagneticData geomagneticData;
    RationMatrixData rationMatrixData;
    ReserveData reserveData;
};

struct BusinessError {
    int32_t code;
    string message;
    string name;
    string stack;
};

typedef enum CallbackDataType {
    FAIL = -1,
    OFF_CALLBACK = 0,
    ON_CALLBACK = 1,
    ONCE_CALLBACK = 2,
    GET_GEOMAGNETIC_FIELD = 3,
    GET_ALTITUDE = 4,
    GET_GEOMAGNITIC_DIP = 5,
    GET_ANGLE_MODIFY = 6,
    CREATE_ROTATION_MATRIX = 7,
    TRANSFORM_COORDINATE_SYSTEM = 8,
    CREATE_QUATERNION = 9,
    GET_DIRECTION = 10,
    ROTATION_INCLINATION_MATRIX = 11,
    GET_SENSOR_LIST = 12,
    GET_SINGLE_SENSOR = 13
} CallbackDataType;

struct AsyncCallbackInfo {
    napi_env env;
    napi_async_work asyncWork;
    napi_deferred deferred;
    napi_ref callback[1] = { 0 };
    CallbackData data;
    BusinessError error;
    CallbackDataType type;
    vector<SensorInfo> sensorInfos;
};

using ConvertDataFunc = void(*)(napi_env env, AsyncCallbackInfo *asyncCallbackInfo, napi_value result[2]);

bool IsMatchType(napi_env env, napi_value value, napi_valuetype type);

napi_value GetNapiInt32(int32_t number, napi_env env);

int32_t GetCppInt32(napi_value value, napi_env env);

bool GetCppBool(napi_value value, napi_env env);

void EmitAsyncCallbackWork(AsyncCallbackInfo *async_callback_info);

void EmitUvEventLoop(AsyncCallbackInfo *async_callback_info);

int64_t GetCppInt64(napi_value value, napi_env env);

napi_value NapiGetNamedProperty(napi_value jsonObject, string name, napi_env env);

napi_value GetUndefined(napi_env env);

void EmitPromiseWork(AsyncCallbackInfo *asyncCallbackInfo);

bool IsMatchArrayType(napi_env env, napi_value value, napi_typedarray_type type);

vector<float> GetCppArrayFloat(napi_env env, napi_value value);

float GetCppFloat(napi_env env, napi_value value);

napi_value GreateBusinessError(napi_env env, int32_t errCode, string errMessage,
    string errName, string errStack);

bool IsMatchArrayType(napi_env env, napi_value value);

void ConvertToFailData(napi_env env, AsyncCallbackInfo *asyncCallbackInfo, napi_value result[2]);

void ConvertToNoData(napi_env env, AsyncCallbackInfo *asyncCallbackInfo, napi_value result[2]);

void ConvertToGeomagneticData(napi_env env, AsyncCallbackInfo *asyncCallbackInfo, napi_value result[2]);

void ConvertToNumber(napi_env env, AsyncCallbackInfo *asyncCallbackInfo, napi_value result[2]);

void ConvertToArray(napi_env env, AsyncCallbackInfo *asyncCallbackInfo, napi_value result[2]);

void ConvertToRotationMatrix(napi_env env, AsyncCallbackInfo *asyncCallbackInfo, napi_value result[2]);

void ConvertToSensorData(napi_env env, AsyncCallbackInfo *asyncCallbackInfo, napi_value result[2]);

void CreateNapiArray(napi_env env, float *data, int32_t dataLength, napi_value result);

void ConvertToSensorInfos(napi_env env, AsyncCallbackInfo *asyncCallbackInfo, napi_value result[2]);

void ConvertToSingleSensor(napi_env env, AsyncCallbackInfo *asyncCallbackInfo, napi_value result[2]);
#endif // SENSOR_NAPI_UTILS_H
