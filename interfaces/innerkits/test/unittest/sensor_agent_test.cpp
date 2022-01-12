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
#include "sensors_log_domain.h"

namespace OHOS {
namespace Sensors {
using namespace testing::ext;
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SensorsLogDomain::SENSOR_TEST, "SensorAgentTest" };
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
        HiLog::Error(LABEL, "SensorDataCallbackImpl event is null");
        return;
    }
    float *sensorData = (float *)event[0].data;
    HiLog::Info(LABEL, "SensorDataCallbackImpl sensorTypeId: %{public}d, version: %{public}d, dataLen: %{public}d, dataLen: %{public}f\n",
		event[0].sensorTypeId, event[0].version, event[0].dataLen, *(sensorData));
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
    HiLog::Info(LABEL, "%{public}s begin", __func__);

    int32_t sensorTypeId = 0;
    SensorUser user;

    user.callback = SensorDataCallbackImpl;

    int32_t ret = SubscribeSensor(sensorTypeId, &user);
    ASSERT_EQ(ret, 0);

    ret = SetBatch(sensorTypeId, &user, 100000000, 100000000);
    ASSERT_EQ(ret, 0);

    ret = ActivateSensor(sensorTypeId, &user);
    ASSERT_EQ(ret, 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    ASSERT_EQ(ret, 0);

    ret = DeactivateSensor(sensorTypeId, &user);
    ASSERT_EQ(ret, 0);

    ret = UnsubscribeSensor(sensorTypeId, &user);
    ASSERT_EQ(ret, 0);
}
}  // namespace Sensors
}  // namespace OHOS
