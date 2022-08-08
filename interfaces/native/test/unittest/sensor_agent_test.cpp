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

#include <gtest/gtest.h>
#include <thread>

#include "sensor_agent.h"
#include "sensors_errors.h"
#include "system_info.h"

namespace OHOS {
namespace Sensors {
using namespace testing::ext;
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, OHOS::Sensors::SENSOR_LOG_DOMAIN, "SensorAgentTest" };
constexpr int32_t sensorId { 0 };
constexpr int32_t invalidValue { -1 };
}  // namespace

class SensorAgentTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SensorAgentTest::SetUpTestCase()
{}

void SensorAgentTest::TearDownTestCase()
{}

void SensorAgentTest::SetUp()
{}

void SensorAgentTest::TearDown()
{}

void SensorDataCallbackImpl(SensorEvent *event)
{
    if (event == nullptr) {
        SEN_HILOGE("SensorEvent is null");
        return;
    }
    float *sensorData = (float *)event[0].data;
    SEN_HILOGI("SensorId:%{public}d, version:%{public}d,dataLen:%{public}d,data:%{public}f",
        event[0].sensorTypeId, event[0].version, event[0].dataLen, *(sensorData));
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
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
    ASSERT_EQ(count, 0);
}

HWTEST_F(SensorAgentTest, GetAllSensorsTest_003, TestSize.Level1)
{
    SEN_HILOGI("GetAllSensorsTest_003 in");
    SensorInfo *sensorInfos { nullptr };
    int32_t ret = GetAllSensors(&sensorInfos, nullptr);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
}

HWTEST_F(SensorAgentTest, ActivateSensorTest_001, TestSize.Level1)
{
    SEN_HILOGI("ActivateSensorTest_001 in");
    SensorUser user;
    user.callback = nullptr;
    int32_t ret = SubscribeSensor(sensorId, &user);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
    ret = SetBatch(sensorId, &user, 100000000, 100000000);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
    ret = ActivateSensor(sensorId, &user);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
    ret = DeactivateSensor(sensorId, &user);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
    ret = UnsubscribeSensor(sensorId, &user);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
}

HWTEST_F(SensorAgentTest, ActivateSensorTest_002, TestSize.Level1)
{
    SEN_HILOGI("ActivateSensorTest_002 in");
    SensorUser user;
    user.callback = SensorDataCallbackImpl;
    int32_t ret = ActivateSensor(invalidValue, &user);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
}

HWTEST_F(SensorAgentTest, ActivateSensorTest_003, TestSize.Level1)
{
    SEN_HILOGI("ActivateSensorTest_003 in");
    int32_t ret = ActivateSensor(sensorId, nullptr);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
}

HWTEST_F(SensorAgentTest, DeactivateSensorTest_001, TestSize.Level1)
{
    SEN_HILOGI("DeactivateSensorTest_001 in");
    SensorUser user;
    user.callback = nullptr;
    int32_t ret = DeactivateSensor(sensorId, &user);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
}

HWTEST_F(SensorAgentTest, DeactivateSensorTest_002, TestSize.Level1)
{
    SEN_HILOGI("DeactivateSensorTest_002 in");
    SensorUser user;
    user.callback = SensorDataCallbackImpl;
    int32_t ret = DeactivateSensor(invalidValue, &user);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
}

HWTEST_F(SensorAgentTest, DeactivateSensorTest_003, TestSize.Level1)
{
    SEN_HILOGI("DeactivateSensorTest_003 in");
    int32_t ret = DeactivateSensor(sensorId, nullptr);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
}

HWTEST_F(SensorAgentTest, DeactivateSensorTest_004, TestSize.Level1)
{
    SEN_HILOGI("DeactivateSensorTest_004 in");
    SensorUser user;
    user.callback = nullptr;
    int32_t ret = DeactivateSensor(sensorId, &user);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
}

HWTEST_F(SensorAgentTest, SetBatchTest_001, TestSize.Level1)
{
    SEN_HILOGI("SetBatchTest_001 in");
    SensorUser user;
    user.callback = nullptr;
    int32_t ret = SubscribeSensor(sensorId, &user);
    ASSERT_EQ(ret,  OHOS::Sensors::ERROR);
    ret = SetBatch(sensorId, &user, 100000000, 100000000);
    ASSERT_EQ(ret,  OHOS::Sensors::ERROR);
}

HWTEST_F(SensorAgentTest, SetBatchTest_002, TestSize.Level1)
{
    SEN_HILOGI("SetBatchTest_002 in");
    SensorUser user;
    user.callback = SensorDataCallbackImpl;
    int32_t ret = SetBatch(invalidValue, &user, 100000000, 100000000);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
}

HWTEST_F(SensorAgentTest, SetBatchTest_003, TestSize.Level1)
{
    SEN_HILOGI("SetBatchTest_003 in");
    int32_t ret = SetBatch(sensorId, nullptr, invalidValue, invalidValue);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
}

HWTEST_F(SensorAgentTest, SubscribeSensorTest_001, TestSize.Level1)
{
    SEN_HILOGI("SubscribeSensorTest_001 in");
    SensorUser user;
    user.callback = nullptr;
    int32_t ret = SubscribeSensor(sensorId, &user);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
}

HWTEST_F(SensorAgentTest, SubscribeSensorTest_002, TestSize.Level1)
{
    SEN_HILOGI("SubscribeSensorTest_002 in");
    SensorUser user;
    user.callback = SensorDataCallbackImpl;
    int32_t ret = SubscribeSensor(invalidValue, &user);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
}

HWTEST_F(SensorAgentTest, SubscribeSensorTest_003, TestSize.Level1)
{
    SEN_HILOGI("SubscribeSensorTest_003 in");
    int32_t ret = SubscribeSensor(sensorId, nullptr);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
}

HWTEST_F(SensorAgentTest, UnsubscribeSensorTest_001, TestSize.Level1)
{
    SEN_HILOGI("UnsubscribeSensorTest_001 in");
    SensorUser user;
    user.callback = nullptr;
    int32_t ret = SubscribeSensor(sensorId, &user);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
    ret = UnsubscribeSensor(sensorId, &user);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
}

HWTEST_F(SensorAgentTest, UnsubscribeSensorTest_002, TestSize.Level1)
{
    SEN_HILOGI("UnsubscribeSensorTest_002 in");
    SensorUser user;
    user.callback = SensorDataCallbackImpl;
    int32_t ret = UnsubscribeSensor(invalidValue, &user);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
}

HWTEST_F(SensorAgentTest, UnsubscribeSensorTest_003, TestSize.Level1)
{
    SEN_HILOGI("UnsubscribeSensorTest_003 in");
    int32_t ret = UnsubscribeSensor(sensorId, nullptr);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
}

HWTEST_F(SensorAgentTest, SetModeTest_001, TestSize.Level1)
{
    SEN_HILOGI("SetModeTest_001 in");
    SensorUser user;
    user.callback = SensorDataCallbackImpl;
    int32_t ret = SubscribeSensor(sensorId, &user);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
    int32_t mode { 0 };
    ret = SetMode(sensorId, &user, mode);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
    ret = UnsubscribeSensor(sensorId, &user);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
}

HWTEST_F(SensorAgentTest, SetModeTest_002, TestSize.Level1)
{
    SEN_HILOGI("SetModeTest_002 in");
    SensorUser user;
    user.callback = SensorDataCallbackImpl;
    int32_t ret = SetMode(invalidValue, &user, invalidValue);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
}

HWTEST_F(SensorAgentTest, SetModeTest_003, TestSize.Level1)
{
    SEN_HILOGI("SetModeTest_003 in");
    int32_t mode { 0 };
    int32_t ret = SetMode(sensorId, nullptr, mode);
    ASSERT_EQ(ret, OHOS::Sensors::ERROR);
}

/*
 * Feature: sensor
 * Function: SubscribeSensor
 * FunctionPoints: Check the interface function
 * EnvConditions: mobile that can run ohos test framework
 * CaseDescription: Verify the senser service framework process.
 */
HWTEST_F(SensorAgentTest, SensorNativeApiTest_001, TestSize.Level1)
{
    SEN_HILOGI("SensorNativeApiTest_001 in");

    SensorUser user;
    user.callback = SensorDataCallbackImpl;

    int32_t ret = SubscribeSensor(sensorId, &user);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);

    ret = SetBatch(sensorId, &user, 100000000, 100000000);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);

    ret = ActivateSensor(sensorId, &user);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);

    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);

    ret = DeactivateSensor(sensorId, &user);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);

    ret = UnsubscribeSensor(sensorId, &user);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
}

HWTEST_F(SensorAgentTest, GetProcCpuUsageTest_001, TestSize.Level1)
{
    SEN_HILOGI("GetProcCpuUsageTest_001 in");
    SYSTEM_INFO::CpuInfo cpuInfo;
    const std::string process_name = "sensors";
    auto usage = cpuInfo.GetProcCpuUsage(process_name);
    SEN_HILOGD("The CPU usage of the %{public}s process is %{public}.2f", process_name.c_str(), usage);
    ASSERT_TRUE(usage < SYSTEM_INFO::CPU_USAGE_LOAD && usage != SYSTEM_INFO::CPU_USAGE_UNKONW);
}
}  // namespace Sensors
}  // namespace OHOS
