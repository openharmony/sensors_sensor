/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "cjsensor_fuzzer.h"

#include <cstdlib>

#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "securec.h"
#include "token_setproc.h"

#include "cj_sensor_ffi.h"

#undef LOG_TAG
#define LOG_TAG "CJsensorFuzzTest"

namespace OHOS {
namespace Sensors {

namespace {
constexpr size_t DATA_MIN_SIZE = 4;
constexpr int32_t ROTATION_VECTOR_LENGTH = 3;
constexpr int32_t QUATERNION_LENGTH = 4;
constexpr int32_t THREE_DIMENSIONAL_MATRIX_LENGTH = 9;
}

class FuzzDataProvider {
public:
    FuzzDataProvider(const uint8_t *data, size_t size) : data_(data), size_(size) {}

    template<typename T>
    bool Get(T &out)
    {
        if (size_ < sizeof(T)) {
            return false;
        }
        if (memcpy_s(&out, sizeof(T), data_, sizeof(T)) != EOK) {
            return false;
        }
        data_ += sizeof(T);
        size_ -= sizeof(T);
        return true;
    }

    bool GetBytes(void *dst, size_t len)
    {
        if (size_ < len) {
            return false;
        }
        if (memcpy_s(dst, len, data_, len) != EOK) {
            return false;
        }
        data_ += len;
        size_ -= len;
        return true;
    }

    size_t Remaining() const
    {
        return size_;
    }

private:
    const uint8_t *data_ {nullptr};
    size_t size_ {0};
};

static CArrFloat32 CreateFloatArray(FuzzDataProvider &provider, int64_t length)
{
    CArrFloat32 res = {nullptr, 0};
    if (length <= 0) {
        return res;
    }

    int64_t maxLenFromData =
        static_cast<int64_t>(provider.Remaining() / sizeof(float));
    if (maxLenFromData <= 0) {
        return res;
    }
    if (length > maxLenFromData) {
        length = maxLenFromData;
    }

    float *buf = static_cast<float *>(malloc(sizeof(float) * length));
    if (buf == nullptr) {
        return res;
    }
    if (!provider.GetBytes(buf, sizeof(float) * length)) {
        free(buf);
        return res;
    }
    res.head = buf;
    res.size = length;
    return res;
}

static void FreeCArr(CArrFloat32 &arr)
{
    if (arr.head != nullptr) {
        free(arr.head);
        arr.head = nullptr;
    }
    arr.size = 0;
}

static void FreeSensorArray(CSensorArray &sensorList)
{
    if (sensorList.head == nullptr || sensorList.size <= 0) {
        return;
    }
    for (int64_t i = 0; i < sensorList.size; ++i) {
        CSensor &s = sensorList.head[i];
        if (s.sensorName != nullptr) {
            free(s.sensorName);
        }
        if (s.vendorName != nullptr) {
            free(s.vendorName);
        }
        if (s.firmwareVersion != nullptr) {
            free(s.firmwareVersion);
        }
        if (s.hardwareVersion != nullptr) {
            free(s.hardwareVersion);
        }
    }
    free(sensorList.head);
    sensorList.head = nullptr;
    sensorList.size = 0;
}

static void SetUpTestCase()
{
    const char **perms = new (std::nothrow) const char *[2];
    if (perms == nullptr) {
        return;
    }
    perms[0] = "ohos.permission.ACCELEROMETER";
    perms[1] = "ohos.permission.MANAGE_SENSOR";

    TokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 2,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "CJsensorFuzzTest",
        .aplStr = "system_core",
    };

    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();

    delete[] perms;
}

static void TestSensorCallback(SensorEvent *event)
{
    (void)event;
}


static void FuzzSubscribeLifecycle(FuzzDataProvider &provider)
{
    int32_t sensorId = 0;
    int64_t interval = 0;
    (void)provider.Get(sensorId);
    (void)provider.Get(interval);

    (void)FfiSensorSubscribeSensor(sensorId, interval, TestSensorCallback);
    (void)FfiSensorUnSubscribeSensor(sensorId);
}

static void FuzzGeomagneticQuery(FuzzDataProvider &provider)
{
    CLocationOptions location = {};
    int64_t timeMillis = 0;
    (void)provider.Get(location);
    (void)provider.Get(timeMillis);

    (void)FfiSensorGetGeomagneticInfo(location, timeMillis);
}

static void FuzzAltitudeAndInclination(FuzzDataProvider &provider)
{
    float seaPressure = 0.0f;
    float currentPressure = 0.0f;
    float altitude = 0.0f;
    (void)provider.Get(seaPressure);
    (void)provider.Get(currentPressure);
    (void)FfiSensorGetDeviceAltitude(seaPressure, currentPressure, &altitude);

    CArrFloat32 inclinationMatrix = CreateFloatArray(provider, THREE_DIMENSIONAL_MATRIX_LENGTH);
    float geomagneticDip = 0.0f;
    (void)FfiSensorGetInclination(inclinationMatrix, &geomagneticDip);
    FreeCArr(inclinationMatrix);
}

static void FuzzAngleVariation(FuzzDataProvider &provider)
{
    CArrFloat32 currentRotationMatrix = CreateFloatArray(provider, THREE_DIMENSIONAL_MATRIX_LENGTH);
    CArrFloat32 preRotationMatrix = CreateFloatArray(provider, THREE_DIMENSIONAL_MATRIX_LENGTH);

    CArrFloat32 angleChange = {nullptr, 0};
    (void)FfiSensorGetAngleVariation(currentRotationMatrix, preRotationMatrix, &angleChange);

    FreeCArr(currentRotationMatrix);
    FreeCArr(preRotationMatrix);
    FreeCArr(angleChange);
}

static void FuzzRotationMatrixOps(FuzzDataProvider &provider)
{
    // 1) rotationVector -> rotationMatrixOut
    CArrFloat32 rotationVector = CreateFloatArray(provider, ROTATION_VECTOR_LENGTH);
    CArrFloat32 rotationMatrixOut = {nullptr, 0};
    (void)FfiSensorGetRotationMatrix(rotationVector, &rotationMatrixOut);
    FreeCArr(rotationVector);
    FreeCArr(rotationMatrixOut);

    // 2) inRotationVector + coordOptions -> transformedRotationMatrix
    CArrFloat32 inRotationVector = CreateFloatArray(provider, THREE_DIMENSIONAL_MATRIX_LENGTH);
    CCoordinatesOptions coordOptions = {};
    (void)provider.Get(coordOptions);

    CArrFloat32 transformedRotationMatrix = {nullptr, 0};
    (void)FfiSensorTransformRotationMatrix(inRotationVector, coordOptions, &transformedRotationMatrix);
    FreeCArr(inRotationVector);
    FreeCArr(transformedRotationMatrix);
}

static void FuzzQuaternionAndOrientation(FuzzDataProvider &provider)
{
    // rotationVectorForQuat -> quaternion
    CArrFloat32 rotationVectorForQuat = CreateFloatArray(provider, QUATERNION_LENGTH);
    CArrFloat32 quaternion = {nullptr, 0};
    (void)FfiSensorGetQuaternion(rotationVectorForQuat, &quaternion);
    FreeCArr(rotationVectorForQuat);
    FreeCArr(quaternion);

    // rotationMatrixForOrientation -> rotationAngleOut
    CArrFloat32 rotationMatrixForOrientation = CreateFloatArray(provider, THREE_DIMENSIONAL_MATRIX_LENGTH);
    CArrFloat32 rotationAngleOut = {nullptr, 0};
    (void)FfiSensorGetOrientation(rotationMatrixForOrientation, &rotationAngleOut);
    FreeCArr(rotationMatrixForOrientation);
    FreeCArr(rotationAngleOut);
}

static void FuzzRotationByGravityAndGeomagnetic(FuzzDataProvider &provider)
{
    CArrFloat32 gravity = CreateFloatArray(provider, ROTATION_VECTOR_LENGTH);
    CArrFloat32 geomagnetic = CreateFloatArray(provider, ROTATION_VECTOR_LENGTH);

    CRotationMatrixResponse rotationResponse = {};
    (void)FfiSensorGetRotationMatrixByGravityAndGeomagnetic(gravity, geomagnetic, &rotationResponse);

    FreeCArr(rotationResponse.rotation);
    FreeCArr(rotationResponse.inclination);
    FreeCArr(gravity);
    FreeCArr(geomagnetic);
}

static void FuzzSensorEnumeration()
{
    CSensorArray sensorList = {nullptr, 0};
    (void)FfiSensorGetAllSensors(&sensorList);
    FreeSensorArray(sensorList);
}


void CJsensorFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < DATA_MIN_SIZE) {
        return;
    }

    SetUpTestCase();
    FuzzDataProvider provider(data, size);

    FuzzSubscribeLifecycle(provider);
    FuzzGeomagneticQuery(provider);
    FuzzAltitudeAndInclination(provider);
    FuzzAngleVariation(provider);
    FuzzRotationMatrixOps(provider);
    FuzzQuaternionAndOrientation(provider);
    FuzzRotationByGravityAndGeomagnetic(provider);
    FuzzSensorEnumeration();
}

}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Sensors::CJsensorFuzzTest(data, size);
    return 0;
}
