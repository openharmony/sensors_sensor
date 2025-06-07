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
#include <gtest/gtest.h>
#include <thread>

#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"

#include "sensor_agent.h"
#include "sensor_errors.h"
#include "system_info.h"

#undef LOG_TAG
#define LOG_TAG "SensorAgentTest"

namespace OHOS {
namespace Sensors {
using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace Security::AccessToken;
using Security::AccessToken::AccessTokenID;

namespace {
constexpr int32_t SENSOR_ID { 1 };
constexpr int32_t INVALID_VALUE { -1 };
constexpr int32_t DEVICE_STATUS { 0 };
static int32_t g_localDeviceId = -1;

PermissionStateFull g_infoManagerTestState = {
    .grantFlags = {1},
    .grantStatus = {PermissionState::PERMISSION_GRANTED},
    .isGeneral = true,
    .permissionName = "ohos.permission.ACCELEROMETER",
    .resDeviceID = {"local"}
};

HapPolicyParams g_infoManagerTestPolicyPrams = {
    .apl = APL_NORMAL,
    .domain = "test.domain",
    .permList = {},
    .permStateList = {g_infoManagerTestState}
};

HapInfoParams g_infoManagerTestInfoParms = {
    .bundleName = "sensoragent_test",
    .userID = 1,
    .instIndex = 0,
    .appIDDesc = "sensorAgentTest"
};
} // namespace

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

void SensorDataCallbackImpl(SensorEvent *event)
{
    if (event == nullptr) {
        SEN_HILOGE("event is null");
        return;
    }
    if (event[0].data == nullptr) {
        SEN_HILOGE("event[0].data is nullptr");
        return;
    }
    if (event[0].dataLen < sizeof(AccelData)) {
        SEN_HILOGE("Event dataLen less than acc data size, event.dataLen:%{public}u", event[0].dataLen);
        return;
    }
    AccelData *accelData = reinterpret_cast<AccelData *>(event[0].data);
    SEN_HILOGI("sensorId:%{public}d, version:%{public}d, dataLen:%{public}d,"
        "x:%{public}f, y:%{public}f,z:%{public}f, option:%{public}d",
        event[0].sensorTypeId, event[0].version, event[0].dataLen,
        accelData->x, accelData->y, accelData->z, event[0].option);
}

void SensorDataCallbackImpl2(SensorEvent *event)
{
    if (event == nullptr) {
        SEN_HILOGE("event is null");
        return;
    }
    if (event[0].data == nullptr) {
        SEN_HILOGE("event[0].data is nullptr");
        return;
    }
    if (event[0].dataLen < sizeof(AccelData)) {
        SEN_HILOGE("Event dataLen less than acc data size, event.dataLen:%{public}u", event[0].dataLen);
        return;
    }
    AccelData *accelData = reinterpret_cast<AccelData *>(event[0].data);
    SEN_HILOGI("sensorId:%{public}d, version:%{public}d, dataLen:%{public}d,"
        "x:%{public}f, y:%{public}f,z:%{public}f, option:%{public}d",
        event[0].sensorTypeId, event[0].version, event[0].dataLen,
        accelData->x, accelData->y, accelData->z, event[0].option);
}

void SensorDataCallbackImpl3(SensorStatusEvent *statusEvent)
{
    if (statusEvent == nullptr) {
        SEN_HILOGE("statusEvent is null");
        return;
    }
}

HWTEST_F(SensorAgentTest, GetAllSensorsTest_001, TestSize.Level1)
{
    SEN_HILOGI("GetAllSensorsTest_001 in");
    SensorInfo *sensorInfos { nullptr };
    int32_t count { 0 };
    int32_t ret = GetAllSensors(&sensorInfos, &count);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(count, 0);
}

HWTEST_F(SensorAgentTest, GetAllSensorsTest_002, TestSize.Level1)
{
    SEN_HILOGI("GetAllSensorsTest_002 in");
    int32_t count = 0;
    int32_t ret = GetAllSensors(nullptr, &count);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    ASSERT_EQ(count, 0);
}

HWTEST_F(SensorAgentTest, GetAllSensorsTest_003, TestSize.Level1)
{
    SEN_HILOGI("GetAllSensorsTest_003 in");
    SensorInfo *sensorInfos { nullptr };
    int32_t ret = GetAllSensors(&sensorInfos, nullptr);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, ActivateSensorTest_001, TestSize.Level1)
{
    SEN_HILOGI("ActivateSensorTest_001 in");
    SensorUser user;
    user.callback = nullptr;
    int32_t ret = SubscribeSensor(SENSOR_ID, &user);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    ret = SetBatch(SENSOR_ID, &user, 100000000, 100000000);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    ret = ActivateSensor(SENSOR_ID, &user);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    ret = DeactivateSensor(SENSOR_ID, &user);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    ret = UnsubscribeSensor(SENSOR_ID, &user);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, ActivateSensorTest_002, TestSize.Level1)
{
    SEN_HILOGI("ActivateSensorTest_002 in");
    SensorUser user;
    user.callback = SensorDataCallbackImpl;
    int32_t ret = ActivateSensor(INVALID_VALUE, &user);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, ActivateSensorTest_003, TestSize.Level1)
{
    SEN_HILOGI("ActivateSensorTest_003 in");
    int32_t ret = ActivateSensor(SENSOR_ID, nullptr);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, DeactivateSensorTest_001, TestSize.Level1)
{
    SEN_HILOGI("DeactivateSensorTest_001 in");
    SensorUser user;
    user.callback = nullptr;
    int32_t ret = DeactivateSensor(SENSOR_ID, &user);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, DeactivateSensorTest_002, TestSize.Level1)
{
    SEN_HILOGI("DeactivateSensorTest_002 in");
    SensorUser user;
    user.callback = SensorDataCallbackImpl;
    int32_t ret = DeactivateSensor(INVALID_VALUE, &user);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, DeactivateSensorTest_003, TestSize.Level1)
{
    SEN_HILOGI("DeactivateSensorTest_003 in");
    int32_t ret = DeactivateSensor(SENSOR_ID, nullptr);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, DeactivateSensorTest_004, TestSize.Level1)
{
    SEN_HILOGI("DeactivateSensorTest_004 in");
    SensorUser user;
    user.callback = nullptr;
    int32_t ret = DeactivateSensor(SENSOR_ID, &user);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, SetBatchTest_001, TestSize.Level1)
{
    SEN_HILOGI("SetBatchTest_001 in");
    SensorUser user;
    user.callback = nullptr;
    int32_t ret = SubscribeSensor(SENSOR_ID, &user);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    ret = SetBatch(SENSOR_ID, &user, 100000000, 100000000);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, SetBatchTest_002, TestSize.Level1)
{
    SEN_HILOGI("SetBatchTest_002 in");
    SensorUser user;
    user.callback = SensorDataCallbackImpl;
    int32_t ret = SetBatch(INVALID_VALUE, &user, 100000000, 100000000);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, SetBatchTest_003, TestSize.Level1)
{
    SEN_HILOGI("SetBatchTest_003 in");
    int32_t ret = SetBatch(SENSOR_ID, nullptr, INVALID_VALUE, INVALID_VALUE);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, SubscribeSensorTest_001, TestSize.Level1)
{
    SEN_HILOGI("SubscribeSensorTest_001 in");
    SensorUser user;
    user.callback = nullptr;
    int32_t ret = SubscribeSensor(SENSOR_ID, &user);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, SubscribeSensorTest_002, TestSize.Level1)
{
    SEN_HILOGI("SubscribeSensorTest_002 in");
    SensorUser user;
    user.callback = SensorDataCallbackImpl;
    int32_t ret = SubscribeSensor(INVALID_VALUE, &user);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, SubscribeSensorTest_003, TestSize.Level1)
{
    SEN_HILOGI("SubscribeSensorTest_003 in");
    int32_t ret = SubscribeSensor(SENSOR_ID, nullptr);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, UnsubscribeSensorTest_001, TestSize.Level1)
{
    SEN_HILOGI("UnsubscribeSensorTest_001 in");
    SensorUser user;
    user.callback = nullptr;
    int32_t ret = SubscribeSensor(SENSOR_ID, &user);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    ret = UnsubscribeSensor(SENSOR_ID, &user);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, UnsubscribeSensorTest_002, TestSize.Level1)
{
    SEN_HILOGI("UnsubscribeSensorTest_002 in");
    SensorUser user;
    user.callback = SensorDataCallbackImpl;
    int32_t ret = UnsubscribeSensor(INVALID_VALUE, &user);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, UnsubscribeSensorTest_003, TestSize.Level1)
{
    SEN_HILOGI("UnsubscribeSensorTest_003 in");
    int32_t ret = UnsubscribeSensor(SENSOR_ID, nullptr);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, SetModeTest_001, TestSize.Level1)
{
    SEN_HILOGI("SetModeTest_001 in");
    SensorUser user;
    user.callback = SensorDataCallbackImpl;
    int32_t ret = SubscribeSensor(SENSOR_ID, &user);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
    int32_t mode { 0 };
    ret = SetMode(SENSOR_ID, &user, mode);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
    ret = UnsubscribeSensor(SENSOR_ID, &user);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, SetModeTest_002, TestSize.Level1)
{
    SEN_HILOGI("SetModeTest_002 in");
    SensorUser user;
    user.callback = SensorDataCallbackImpl;
    int32_t ret = SetMode(INVALID_VALUE, &user, INVALID_VALUE);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, SetModeTest_003, TestSize.Level1)
{
    SEN_HILOGI("SetModeTest_003 in");
    int32_t mode { 0 };
    int32_t ret = SetMode(SENSOR_ID, nullptr, mode);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

/*
 * Feature: sensor
 * Function: SubscribeSensor
 * FunctionPoints: Check the interface function
 * EnvConditions: mobile that can run ohos test framework
 * CaseDescription: Verify the sensor service framework process.
 */
HWTEST_F(SensorAgentTest, SensorNativeApiTest_001, TestSize.Level0)
{
    SEN_HILOGI("SensorNativeApiTest_001 in");

    SensorUser user;
    user.callback = SensorDataCallbackImpl;

    int32_t ret = SubscribeSensor(SENSOR_ID, &user);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);

    ret = SetBatch(SENSOR_ID, &user, 100000000, 100000000);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);

    ret = ActivateSensor(SENSOR_ID, &user);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);

    ret = DeactivateSensor(SENSOR_ID, &user);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);

    ret = UnsubscribeSensor(SENSOR_ID, &user);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, GetProcCpuUsageTest_001, TestSize.Level1)
{
    SEN_HILOGI("GetProcCpuUsageTest_001 in");
    SYSTEM_INFO::CpuInfo cpuInfo;
    const std::string processName = "sensors";
    auto usage = cpuInfo.GetProcCpuUsage(processName);
    SEN_HILOGD("The CPU usage of the %{public}s process is %{public}.2f", processName.c_str(), usage);
    ASSERT_TRUE(usage < SYSTEM_INFO::CPU_USAGE_LOAD && usage != SYSTEM_INFO::CPU_USAGE_UNKNOWN);
}

/*
 * Feature: sensor
 * Function: GetAllSensors
 * FunctionPoints: Check the interface function
 * EnvConditions: mobile that can run ohos test framework
 * CaseDescription: Verify the sensor service framework process.
 */
HWTEST_F(SensorAgentTest, SensorListTest_001, TestSize.Level1)
{
    SEN_HILOGI("SensorListTest_001 in");

    SensorInfo *sensorInfo = nullptr;
    int32_t count = 0;
    int32_t ret = GetAllSensors(&sensorInfo, &count);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
    for (int32_t i = 0; i < count; ++i) {
        if (sensorInfo[i].location == 1) {
            g_localDeviceId = sensorInfo[i].deviceId;
        }
        SEN_HILOGD("sensorName:%{public}s, sensorId:%{public}d, minSamplePeriod:%{public}" PRId64
            " ns, maxSamplePeriod:%{public}" PRId64 " ns", sensorInfo[i].sensorName, sensorInfo[i].sensorId,
            sensorInfo[i].minSamplePeriod, sensorInfo[i].maxSamplePeriod);
    }
}

/*
 * Feature: sensor
 * Function: SubscribeSensor
 * FunctionPoints: Check the interface function
 * EnvConditions: mobile that can run ohos test framework
 * CaseDescription: Verify the sensor service framework process.
 */
HWTEST_F(SensorAgentTest, SensorNativeApiTest_002, TestSize.Level1)
{
    SEN_HILOGI("SensorNativeApiTest_002 in");

    SensorUser user;
    user.callback = SensorDataCallbackImpl;

    int32_t ret = SubscribeSensor(SENSOR_ID, &user);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);

    ret = SetBatch(SENSOR_ID, &user, 100000000, 100000000);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);

    ret = ActivateSensor(SENSOR_ID, &user);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    SensorUser user2;
    user2.callback = SensorDataCallbackImpl2;

    ret = SubscribeSensor(SENSOR_ID, &user2);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);

    ret = SetBatch(SENSOR_ID, &user2, 200000000, 100000000);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);

    ret = ActivateSensor(SENSOR_ID, &user2);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    ret = DeactivateSensor(SENSOR_ID, &user2);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);

    ret = UnsubscribeSensor(SENSOR_ID, &user2);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);

    ret = DeactivateSensor(SENSOR_ID, &user);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);

    ret = UnsubscribeSensor(SENSOR_ID, &user);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, SensorNativeApiTest_003, TestSize.Level1)
{
    SEN_HILOGI("SensorNativeApiTest_003 in");
    SensorUser user;
    user.callback = SensorDataCallbackImpl;
    int32_t ret = DeactivateSensor(SENSOR_ID, &user);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, SensorNativeApiTest_004, TestSize.Level1)
{
    SEN_HILOGI("SensorNativeApiTest_004 in");
    SensorUser user;
    user.callback = SensorDataCallbackImpl;
    int32_t ret = SetMode(SENSOR_ID, &user, SENSOR_DEFAULT_MODE);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, SensorNativeApiTest_005, TestSize.Level1)
{
    SEN_HILOGI("SensorNativeApiTest_005 in");
    SetDeviceStatus(DEVICE_STATUS);
    SensorUser user;
    user.callback = SensorDataCallbackImpl;
    int32_t ret = SetMode(SENSOR_ID, &user, SENSOR_DEFAULT_MODE);
    ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, GetDeviceSensorsTest_001, TestSize.Level1)
{
    SEN_HILOGI("GetDeviceSensorsTest_001 in");
    SensorInfo *sensorInfos { nullptr };
    int32_t count { 0 };
    int32_t deviceId = g_localDeviceId;
    int32_t ret = GetDeviceSensors(deviceId, &sensorInfos, &count);
    ASSERT_EQ(ret, OHOS::ERR_OK);
}

HWTEST_F(SensorAgentTest, GetDeviceSensorsTest_002, TestSize.Level1)
{
    SEN_HILOGI("GetDeviceSensorsTest_002 in");
    SensorInfo *sensorInfos { nullptr };
    int32_t deviceId = g_localDeviceId;
    int32_t ret = GetDeviceSensors(deviceId, &sensorInfos, nullptr);
    ASSERT_NE(ret, OHOS::ERR_OK);
}

HWTEST_F(SensorAgentTest, SubscribeSensorEnhancedTest_001, TestSize.Level1)
{
    SEN_HILOGI("SubscribeSensorEnhancedTest_001 in");
    SensorIdentifier sensorIdentifier;
    int32_t ret = SubscribeSensorEnhanced(sensorIdentifier, nullptr);
    ASSERT_NE(ret, OHOS::ERR_OK);
}

HWTEST_F(SensorAgentTest, SubscribeSensorEnhancedTest_002, TestSize.Level1)
{
    SEN_HILOGI("SubscribeSensorEnhancedTest_002 in");
    SensorIdentifier sensorIdentifier {
        .deviceId = g_localDeviceId,
        .sensorType = 1,
        .sensorId = 0,
        .location = 1,
    };
    SensorUser user;
    user.callback = SensorDataCallbackImpl;
    int32_t ret = SubscribeSensorEnhanced(sensorIdentifier, &user);
    ASSERT_EQ(ret, OHOS::ERR_OK);
}

HWTEST_F(SensorAgentTest, UnsubscribeSensorEnhancedTest_001, TestSize.Level1)
{
    SEN_HILOGI("UnsubscribeSensorEnhancedTest_001 in");
    SensorIdentifier sensorIdentifier {
        .deviceId = g_localDeviceId,
        .sensorType = 1,
        .sensorId = 0,
        .location = 1,
    };
    SensorUser user;
    user.callback = SensorDataCallbackImpl;
    int32_t ret = SubscribeSensorEnhanced(sensorIdentifier, &user);
    ASSERT_EQ(ret, OHOS::ERR_OK);
    ret = SetBatchEnhanced(sensorIdentifier, &user, 100000000, 100000000);
    ASSERT_EQ(ret, OHOS::ERR_OK);
    ret = ActivateSensorEnhanced(sensorIdentifier, &user);
    ASSERT_EQ(ret, OHOS::ERR_OK);
    ret = DeactivateSensorEnhanced(sensorIdentifier, &user);
    ASSERT_EQ(ret, OHOS::ERR_OK);
    ret = UnsubscribeSensorEnhanced(sensorIdentifier, &user);
    ASSERT_EQ(ret, OHOS::ERR_OK);
}

HWTEST_F(SensorAgentTest, UnsubscribeSensorEnhancedTest_002, TestSize.Level1)
{
    SEN_HILOGI("UnsubscribeSensorEnhancedTest_002 in");
    SensorIdentifier sensorIdentifier;
    int32_t ret = UnsubscribeSensorEnhanced(sensorIdentifier, nullptr);
    ASSERT_NE(ret, OHOS::ERR_OK);
}

HWTEST_F(SensorAgentTest, SetBatchEnhancedTest_001, TestSize.Level1)
{
    SEN_HILOGI("SetBatchEnhancedTest_001 in");
    SensorIdentifier sensorIdentifier;
    int32_t ret = SetBatchEnhanced(sensorIdentifier, nullptr, 100, 100);
    ASSERT_NE(ret, OHOS::ERR_OK);
}


HWTEST_F(SensorAgentTest, ActivateSensorEnhancedTest_001, TestSize.Level1)
{
    SEN_HILOGI("ActivateSensorEnhancedTest_001 in");
    SensorIdentifier sensorIdentifier;
    int32_t ret = ActivateSensorEnhanced(sensorIdentifier, nullptr);
    ASSERT_NE(ret, OHOS::ERR_OK);
}

HWTEST_F(SensorAgentTest, DeactivateSensorEnhancedTest_001, TestSize.Level1)
{
    SEN_HILOGI("DeactivateSensorEnhancedTest_001 in");
    SensorIdentifier sensorIdentifier;
    int32_t ret = DeactivateSensorEnhanced(sensorIdentifier, nullptr);
    ASSERT_NE(ret, OHOS::ERR_OK);
}

HWTEST_F(SensorAgentTest, SetModeEnhancedTest_001, TestSize.Level1)
{
    SEN_HILOGI("SetModeEnhancedTest_001 in");
    SensorIdentifier sensorIdentifier;
    int32_t ret = SetModeEnhanced(sensorIdentifier, nullptr, 1);
    ASSERT_NE(ret, OHOS::ERR_OK);
}

HWTEST_F(SensorAgentTest, SetModeEnhancedTest_002, TestSize.Level1)
{
    SEN_HILOGI("SetModeEnhancedTest_002 in");
    SensorIdentifier sensorIdentifier {
        .deviceId = g_localDeviceId,
        .sensorType = 1,
        .sensorId = 0,
        .location = 1,
    };
    SensorUser user;
    user.callback = SensorDataCallbackImpl;
    int32_t ret = SubscribeSensorEnhanced(sensorIdentifier, &user);
    ASSERT_EQ(ret, OHOS::ERR_OK);
    ret = SetModeEnhanced(sensorIdentifier, &user, 1);
    ASSERT_EQ(ret, OHOS::ERR_OK);
}

HWTEST_F(SensorAgentTest, SubscribeSensorPlugTest_001, TestSize.Level1)
{
    SEN_HILOGI("SubscribeSensorPlugTest_001 in");
    int32_t ret = SubscribeSensorPlug(nullptr);
    ASSERT_NE(ret, OHOS::ERR_OK);
}

HWTEST_F(SensorAgentTest, SubscribeSensorPlugTest_002, TestSize.Level1)
{
    SEN_HILOGI("SubscribeSensorPlugTest_002 in");
    SensorUser user;
    user.plugCallback = SensorDataCallbackImpl3;
    int32_t ret = SubscribeSensorPlug(&user);
    ASSERT_EQ(ret, OHOS::ERR_OK);
}

HWTEST_F(SensorAgentTest, UnsubscribeSensorPlugTest_001, TestSize.Level1)
{
    SEN_HILOGI("UnsubscribeSensorPlugTest_001 in");
    int32_t ret = UnsubscribeSensorPlug(nullptr);
    ASSERT_NE(ret, OHOS::ERR_OK);
}

HWTEST_F(SensorAgentTest, UnsubscribeSensorPlugTest_002, TestSize.Level1)
{
    SEN_HILOGI("UnsubscribeSensorPlugTest_002 in");
    SensorUser user;
    user.plugCallback = SensorDataCallbackImpl3;
    int32_t ret = SubscribeSensorPlug(&user);
    ASSERT_EQ(ret, OHOS::ERR_OK);
    ret = UnsubscribeSensorPlug(&user);
    ASSERT_EQ(ret, OHOS::ERR_OK);
}
} // namespace Sensors
} // namespace OHOS
