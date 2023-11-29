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

#include <cinttypes>
#include <memory>
#include <gtest/gtest.h>
#include <thread>

#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"

#include "native_sensor.h"
#include "sensor_errors.h"

namespace OHOS {
namespace Sensors {
using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace Security::AccessToken;
using Security::AccessToken::AccessTokenID;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, OHOS::Sensors::SENSOR_LOG_DOMAIN, "SensorAgentTest" };
constexpr Sensor_SensorType SENSOR_ID { SENSOR_TYPE_ACCELEROMETER };
constexpr uint32_t SENSOR_NAME_LENGTH_MAX = 64;
constexpr int64_t SENSOR_SAMPLE_PERIOD = 200000000;
constexpr int32_t SLEEP_TIME_MS = 1000;
constexpr int64_t INVALID_VALUE = -1;
constexpr float INVALID_RESOLUTION = -1.0F;

PermissionDef g_infoManagerTestPermDef = {
    .permissionName = "ohos.permission.ACCELEROMETER",
    .bundleName = "accesstoken_test",
    .grantMode = 1,
    .availableLevel = APL_NORMAL,
    .label = "label",
    .labelId = 1,
    .description = "test sensor agent",
    .descriptionId = 1
};

PermissionStateFull g_infoManagerTestState = {
    .permissionName = "ohos.permission.ACCELEROMETER",
    .isGeneral = true,
    .resDeviceID = {"local"},
    .grantStatus = {PermissionState::PERMISSION_GRANTED},
    .grantFlags = {1}
};

HapPolicyParams g_infoManagerTestPolicyPrams = {
    .apl = APL_NORMAL,
    .domain = "test.domain",
    .permList = {g_infoManagerTestPermDef},
    .permStateList = {g_infoManagerTestState}
};

HapInfoParams g_infoManagerTestInfoParms = {
    .userID = 1,
    .bundleName = "sensoragent_test",
    .instIndex = 0,
    .appIDDesc = "sensorAgentTest"
};

Sensor_SubscribeUser *g_user = nullptr;
}  // namespace

class SensorAgentTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
private:
    static AccessTokenID tokenID_;
};

AccessTokenID SensorAgentTest::tokenID_ = 0;

void SensorAgentTest::SetUpTestCase()
{
    AccessTokenIDEx tokenIdEx = {0};
    tokenIdEx = AccessTokenKit::AllocHapToken(g_infoManagerTestInfoParms, g_infoManagerTestPolicyPrams);
    tokenID_ = tokenIdEx.tokenIdExStruct.tokenID;
    ASSERT_NE(0, tokenID_);
    ASSERT_EQ(0, SetSelfTokenID(tokenID_));
}

void SensorAgentTest::TearDownTestCase()
{
    int32_t ret = AccessTokenKit::DeleteToken(tokenID_);
    if (tokenID_ != 0) {
        ASSERT_EQ(RET_SUCCESS, ret);
    }
}

void SensorAgentTest::SetUp()
{}

void SensorAgentTest::TearDown()
{}

void SensorDataCallbackImpl(Sensor_SensorEvent *event)
{
    if (event == nullptr) {
        SEN_HILOGE("event is null");
        return;
    }
    int64_t timestamp = INVALID_VALUE;
    int32_t ret = OH_Sensor_GetEventTimestamp(event, &timestamp);
    ASSERT_EQ(ret, SENSOR_SUCCESS);
    Sensor_SensorType sensorType;
    ret = OH_Sensor_GetEventSensorType(event, &sensorType);
    ASSERT_EQ(ret, SENSOR_SUCCESS);
    Sensor_SensorAccuracy accuracy = SENSOR_ACCURACY_UNRELIABLE;
    ret = OH_Sensor_GetEventAccuracy(event, &accuracy);
    ASSERT_EQ(ret, SENSOR_SUCCESS);
    float *data = nullptr;
    uint32_t length = 0;
    ret = OH_Sensor_GetSensorData(event, &data, &length);
    ASSERT_EQ(ret, SENSOR_SUCCESS);
    SEN_HILOGI("sensorType:%{public}d, dataLen:%{public}d, accuracy:%{public}d"
        "x:%{public}f, y:%{public}f, z:%{public}f", sensorType, length, accuracy,
        data[0], data[1], data[2]);
}

HWTEST_F(SensorAgentTest, OH_Sensor_GetAllSensors_001, TestSize.Level1)
{
    SEN_HILOGI("OH_Sensor_GetAllSensors_001 in");
    uint32_t count = 0;
    int32_t ret = OH_Sensor_GetAllSensors(nullptr, &count);
    ASSERT_EQ(ret, SENSOR_SUCCESS);
    Sensor_Sensor **sensors = OH_Sensor_CreateSensors(count);
    ASSERT_NE(sensors, nullptr);
    ret = OH_Sensor_GetAllSensors(sensors, &count);
    ASSERT_EQ(ret, SENSOR_SUCCESS);
    for (uint32_t i = 0; i < count; ++i) {
        char sensorName[SENSOR_NAME_LENGTH_MAX] = {};
        uint32_t length = SENSOR_NAME_LENGTH_MAX;
        ret = OH_Sensor_GetSensorName(sensors[i], sensorName, &length);
        ASSERT_EQ(ret, SENSOR_SUCCESS);
        char vendorName[SENSOR_NAME_LENGTH_MAX] = {};
        length = SENSOR_NAME_LENGTH_MAX;
        ret = OH_Sensor_GetVendorName(sensors[i], vendorName, &length);
        ASSERT_EQ(ret, SENSOR_SUCCESS);
        Sensor_SensorType sensorType;
        ret = OH_Sensor_GetSensorType(sensors[i], &sensorType);
        ASSERT_EQ(ret, SENSOR_SUCCESS);
        float resolution = INVALID_RESOLUTION;
        ret = OH_Sensor_GetSensorResolution(sensors[i], &resolution);
        ASSERT_EQ(ret, SENSOR_SUCCESS);
        int64_t minSamplePeriod = INVALID_VALUE;
        ret = OH_Sensor_GetSensorMinSamplePeriod(sensors[i], &minSamplePeriod);
        ASSERT_EQ(ret, SENSOR_SUCCESS);
        int64_t maxSamplePeriod = INVALID_VALUE;
        ret = OH_Sensor_GetSensorMaxSamplePeriod(sensors[i], &maxSamplePeriod);
        ASSERT_EQ(ret, SENSOR_SUCCESS);
        SEN_HILOGI("sensorType:%{public}d, sensorName:%{public}s, vendorName:%{public}s,"
            "resolution:%{public}f, minSamplePeriod:%{public}" PRId64 "maxSamplePeriod:%{public}" PRId64,
            static_cast<int32_t>(sensorType), sensorName, vendorName,
            resolution, minSamplePeriod, maxSamplePeriod);
    }
    ret = OH_Sensor_DestroySensors(sensors, count);
    ASSERT_EQ(ret, SENSOR_SUCCESS);
}

HWTEST_F(SensorAgentTest, OH_Sensor_GetAllSensors_002, TestSize.Level1)
{
    SEN_HILOGI("OH_Sensor_GetAllSensors_003 in");
    Sensor_Sensor *sensors { nullptr };
    int32_t ret = OH_Sensor_GetAllSensors(&sensors, nullptr);
    ASSERT_NE(ret, SENSOR_SUCCESS);
}

HWTEST_F(SensorAgentTest, OH_Sensor_SubscribeSensor_001, TestSize.Level1)
{
    SEN_HILOGI("OH_Sensor_SubscribeSensor_001 in");
    g_user = OH_Sensor_CreateUser();
    int32_t ret = OH_Sensor_SetSensorCallback(g_user, SensorDataCallbackImpl);
    ASSERT_EQ(ret, SENSOR_SUCCESS);

    Sensor_SensorSubscribeId *id = OH_Sensor_CreateSubscribeId();
    ret = OH_Sensor_SetSubscribeSensorType(id, SENSOR_ID);
    ASSERT_EQ(ret, SENSOR_SUCCESS);

    Sensor_SubscribeAttribute *attr = OH_Sensor_CreateAttribute();
    ret = OH_Sensor_SetSamplingInterval(attr, SENSOR_SAMPLE_PERIOD);
    ASSERT_EQ(ret, SENSOR_SUCCESS);

    ret = OH_Sensor_SubscribeSensor(id, attr, g_user);
    ASSERT_EQ(ret, SENSOR_SUCCESS);

    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_MS));
    ret = OH_Sensor_UnsubscribeSensor(id, g_user);
    ASSERT_EQ(ret, SENSOR_SUCCESS);
    if (id != nullptr) {
        OH_Sensor_DestroySubscribeId(id);
    }
    if (attr != nullptr) {
        OH_Sensor_DestroyAttribute(attr);
    }
    if (g_user != nullptr) {
        OH_Sensor_DestroyUser(g_user);
        g_user = nullptr;
    }
}

HWTEST_F(SensorAgentTest, OH_Sensor_SubscribeSensor_002, TestSize.Level1)
{
    SEN_HILOGI("OH_Sensor_SubscribeSensor_002 in");
    Sensor_SensorSubscribeId *id = OH_Sensor_CreateSubscribeId();
    int32_t ret = OH_Sensor_SetSubscribeSensorType(id, SENSOR_ID);
    ASSERT_EQ(ret, SENSOR_SUCCESS);

    Sensor_SubscribeAttribute *attr = OH_Sensor_CreateAttribute();
    ret = OH_Sensor_SetSamplingInterval(attr, SENSOR_SAMPLE_PERIOD);
    ASSERT_EQ(ret, SENSOR_SUCCESS);

    ret = OH_Sensor_SubscribeSensor(id, attr, nullptr);
    ASSERT_EQ(ret, SENSOR_PARAMETER_ERROR);
    if (id != nullptr) {
        OH_Sensor_DestroySubscribeId(id);
    }
    if (attr != nullptr) {
        OH_Sensor_DestroyAttribute(attr);
    }
}

HWTEST_F(SensorAgentTest, OH_Sensor_SubscribeSensor_003, TestSize.Level1)
{
    SEN_HILOGI("OH_Sensor_SubscribeSensor_003 in");
    g_user = OH_Sensor_CreateUser();
    int32_t ret = OH_Sensor_SetSensorCallback(g_user, SensorDataCallbackImpl);
    ASSERT_EQ(ret, SENSOR_SUCCESS);

    Sensor_SubscribeAttribute *attr = OH_Sensor_CreateAttribute();
    ret = OH_Sensor_SetSamplingInterval(attr, SENSOR_SAMPLE_PERIOD);
    ASSERT_EQ(ret, SENSOR_SUCCESS);

    ret = OH_Sensor_SubscribeSensor(nullptr, attr, g_user);
    ASSERT_EQ(ret, SENSOR_PARAMETER_ERROR);
    if (attr != nullptr) {
        OH_Sensor_DestroyAttribute(attr);
    }
    if (g_user != nullptr) {
        OH_Sensor_DestroyUser(g_user);
        g_user = nullptr;
    }
}

HWTEST_F(SensorAgentTest, OH_Sensor_SubscribeSensor_004, TestSize.Level1)
{
    SEN_HILOGI("OH_Sensor_SubscribeSensor_004 in");
    g_user = OH_Sensor_CreateUser();
    int32_t ret = OH_Sensor_SetSensorCallback(g_user, SensorDataCallbackImpl);
    ASSERT_EQ(ret, SENSOR_SUCCESS);

    Sensor_SensorSubscribeId *id = OH_Sensor_CreateSubscribeId();
    ret = OH_Sensor_SetSubscribeSensorType(id, SENSOR_ID);
    ASSERT_EQ(ret, SENSOR_SUCCESS);

    ret = OH_Sensor_SubscribeSensor(id, nullptr, g_user);
    ASSERT_EQ(ret, SENSOR_PARAMETER_ERROR);
    if (id != nullptr) {
        OH_Sensor_DestroySubscribeId(id);
    }
    if (g_user != nullptr) {
        OH_Sensor_DestroyUser(g_user);
        g_user = nullptr;
    }
}

HWTEST_F(SensorAgentTest, OH_Sensor_UnsubscribeSensor_001, TestSize.Level1)
{
    SEN_HILOGI("OH_Sensor_UnsubscribeSensor_001 in");
    g_user = OH_Sensor_CreateUser();
    int32_t ret = OH_Sensor_SetSensorCallback(g_user, SensorDataCallbackImpl);
    ASSERT_EQ(ret, SENSOR_SUCCESS);

    ret = OH_Sensor_UnsubscribeSensor(nullptr, g_user);
    ASSERT_EQ(ret, SENSOR_PARAMETER_ERROR);
    if (g_user != nullptr) {
        OH_Sensor_DestroyUser(g_user);
        g_user = nullptr;
    }
}

HWTEST_F(SensorAgentTest, OH_Sensor_UnsubscribeSensor_002, TestSize.Level1)
{
    SEN_HILOGI("OH_Sensor_UnsubscribeSensor_002 in");
    Sensor_SensorSubscribeId *id = OH_Sensor_CreateSubscribeId();
    int32_t ret = OH_Sensor_SetSubscribeSensorType(id, SENSOR_ID);
    ASSERT_EQ(ret, SENSOR_SUCCESS);

    ret = OH_Sensor_UnsubscribeSensor(id, nullptr);
    ASSERT_EQ(ret, SENSOR_PARAMETER_ERROR);
    if (id != nullptr) {
        OH_Sensor_DestroySubscribeId(id);
    }
}

HWTEST_F(SensorAgentTest, OH_Sensor_SetSubscribeSensorType_001, TestSize.Level1)
{
    SEN_HILOGI("OH_Sensor_SetSubscribeSensorType_001 in");
    int32_t ret = OH_Sensor_SetSubscribeSensorType(nullptr, SENSOR_ID);
    ASSERT_EQ(ret, SENSOR_PARAMETER_ERROR);
}

HWTEST_F(SensorAgentTest, OH_Sensor_GetSubscribeSensorType_001, TestSize.Level1)
{
    SEN_HILOGI("OH_Sensor_GetSubscribeSensorType_001 in");
    Sensor_SensorType type;
    int32_t ret = OH_Sensor_GetSubscribeSensorType(nullptr, &type);
    ASSERT_EQ(ret, SENSOR_PARAMETER_ERROR);
}

HWTEST_F(SensorAgentTest, OH_Sensor_GetSubscribeSensorType_002, TestSize.Level1)
{
    SEN_HILOGI("OH_Sensor_GetSubscribeSensorType_002 in");
    Sensor_SensorSubscribeId *id = OH_Sensor_CreateSubscribeId();
    int32_t ret = OH_Sensor_GetSubscribeSensorType(id, nullptr);
    ASSERT_EQ(ret, SENSOR_PARAMETER_ERROR);
    if (id != nullptr) {
        OH_Sensor_DestroySubscribeId(id);
    }
}

HWTEST_F(SensorAgentTest, OH_Sensor_SetSamplingInterval_001, TestSize.Level1)
{
    SEN_HILOGI("OH_Sensor_SetSamplingInterval_001 in");
    int32_t ret = OH_Sensor_SetSamplingInterval(nullptr, SENSOR_SAMPLE_PERIOD);
    ASSERT_EQ(ret, SENSOR_PARAMETER_ERROR);
}

HWTEST_F(SensorAgentTest, OH_Sensor_SetSamplingInterval_002, TestSize.Level1)
{
    SEN_HILOGI("OH_Sensor_SetSamplingInterval_002 in");
    Sensor_SubscribeAttribute *attr = OH_Sensor_CreateAttribute();
    int32_t ret = OH_Sensor_SetSamplingInterval(attr, INVALID_VALUE);
    ASSERT_EQ(ret, SENSOR_PARAMETER_ERROR);
    if (attr != nullptr) {
        OH_Sensor_DestroyAttribute(attr);
    }
}

HWTEST_F(SensorAgentTest, OH_Sensor_GetSamplingInterval_001, TestSize.Level1)
{
    SEN_HILOGI("OH_Sensor_GetSamplingInterval_001 in");
    int64_t samplingInterval = 0;
    int32_t ret = OH_Sensor_GetSamplingInterval(nullptr, &samplingInterval);
    ASSERT_EQ(ret, SENSOR_PARAMETER_ERROR);
}

HWTEST_F(SensorAgentTest, OH_Sensor_GetSamplingInterval_002, TestSize.Level1)
{
    SEN_HILOGI("OH_Sensor_GetSamplingInterval_002 in");
    Sensor_SubscribeAttribute *attr = OH_Sensor_CreateAttribute();
    int32_t ret = OH_Sensor_GetSamplingInterval(attr, nullptr);
    ASSERT_EQ(ret, SENSOR_PARAMETER_ERROR);
    if (attr != nullptr) {
        OH_Sensor_DestroyAttribute(attr);
    }
}

HWTEST_F(SensorAgentTest, OH_Sensor_SetSensorCallback_001, TestSize.Level1)
{
    SEN_HILOGI("OH_Sensor_SetSensorCallback_001 in");
    int32_t ret = OH_Sensor_SetSensorCallback(nullptr, SensorDataCallbackImpl);
    ASSERT_EQ(ret, SENSOR_PARAMETER_ERROR);
}

HWTEST_F(SensorAgentTest, OH_Sensor_SetSensorCallback_002, TestSize.Level1)
{
    SEN_HILOGI("OH_Sensor_SetSensorCallback_002 in");
    g_user = OH_Sensor_CreateUser();
    int32_t ret = OH_Sensor_SetSensorCallback(g_user, nullptr);
    ASSERT_EQ(ret, SENSOR_PARAMETER_ERROR);
}

HWTEST_F(SensorAgentTest, OH_Sensor_GetSensorCallback_001, TestSize.Level1)
{
    SEN_HILOGI("OH_Sensor_GetSensorCallback_001 in");
    Sensor_RecordSensorCallback callback;
    int32_t ret = OH_Sensor_GetSensorCallback(nullptr, &callback);
    ASSERT_EQ(ret, SENSOR_PARAMETER_ERROR);
}

HWTEST_F(SensorAgentTest, OH_Sensor_GetSensorCallback_002, TestSize.Level1)
{
    SEN_HILOGI("OH_Sensor_GetSensorCallback_002 in");
    g_user = OH_Sensor_CreateUser();
    int32_t ret = OH_Sensor_GetSensorCallback(g_user, nullptr);
    ASSERT_EQ(ret, SENSOR_PARAMETER_ERROR);
    if (g_user != nullptr) {
        OH_Sensor_DestroyUser(g_user);
    }
}
}  // namespace Sensors
}  // namespace OHOS
