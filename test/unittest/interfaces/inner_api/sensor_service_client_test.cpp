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

#include <atomic>
#include <thread>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "sensor_agent.h"
#include "sensor_agent_type.h"
#include "sensor_errors.h"
#include "sensor_service_client.h"
#include "system_ability_definition.h"

#undef LOG_TAG
#define LOG_TAG "SensorServiceClientTest"

namespace OHOS {
namespace Sensors {
using namespace testing;
using namespace testing::ext;
using namespace OHOS::HiviewDFX;

namespace {
static sptr<IRemoteObject> g_remote = new (std::nothrow) IPCObjectStub();
} // namespace

class SensorServiceClientTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    SensorServiceClient *sensorServiceClient;
};

void SensorServiceClientTest::SetUpTestCase()
{
}

void SensorServiceClientTest::TearDownTestCase()
{
}

void SensorServiceClientTest::SetUp()
{
    sensorServiceClient = new (std::nothrow) SensorServiceClient();
    ASSERT_NE(sensorServiceClient, nullptr);
}

void SensorServiceClientTest::TearDown()
{
    if (sensorServiceClient != nullptr) {
        delete sensorServiceClient;
        sensorServiceClient = nullptr;
    }
}

HWTEST_F(SensorServiceClientTest, InitServiceClientTest_001, TestSize.Level1)
{
    SEN_HILOGI("InitServiceClientTest_001 in");
    sensorServiceClient->sensorServer_ = iface_cast<ISensorService>(g_remote);
    sensorServiceClient->sensorList_.clear();
    int32_t ret = sensorServiceClient->InitServiceClient();
    EXPECT_EQ(ret, ERR_OK);
}

HWTEST_F(SensorServiceClientTest, InitServiceClientTest_002, TestSize.Level1)
{
    SEN_HILOGI("InitServiceClientTest_002 in");
    sensorServiceClient->sensorServer_ = iface_cast<ISensorService>(g_remote);
    Sensor sensor;
    sensorServiceClient->sensorList_.push_back(sensor);
    int32_t ret = sensorServiceClient->InitServiceClient();
    EXPECT_EQ(ret, ERR_OK);
}

HWTEST_F(SensorServiceClientTest, InitServiceClientTest_003, TestSize.Level1)
{
    SEN_HILOGI("InitServiceClientTest_003 in");
    sensorServiceClient->sensorServer_ = iface_cast<ISensorService>(g_remote);
    int32_t ret = sensorServiceClient->InitServiceClient();
    EXPECT_EQ(ret, ERR_OK);
}

HWTEST_F(SensorServiceClientTest, InitServiceClientTest_004, TestSize.Level1)
{
    SEN_HILOGI("InitServiceClientTest_004 in");
    sensorServiceClient->sensorServer_ = nullptr;
    int32_t ret = sensorServiceClient->InitServiceClient();
    EXPECT_EQ(ret, ERR_OK);
}

HWTEST_F(SensorServiceClientTest, InitServiceClientTest_005, TestSize.Level1)
{
    SEN_HILOGI("InitServiceClientTest_005 in");
    auto systemAbilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    EXPECT_NE(systemAbilityManager, nullptr);
    auto sa = systemAbilityManager->CheckSystemAbility(SENSOR_SERVICE_ABILITY_ID);
    EXPECT_NE(sa, nullptr);
    sensorServiceClient->sensorServer_ = iface_cast<ISensorService>(sa);
    EXPECT_NE(sensorServiceClient->sensorServer_, nullptr);
    int32_t ret = sensorServiceClient->InitServiceClient();
    EXPECT_EQ(ret, ERR_OK);
}

HWTEST_F(SensorServiceClientTest, LoadSensorServiceTest_001, TestSize.Level1)
{
    SEN_HILOGI("LoadSensorServiceTest_001 in");
    sensorServiceClient->sensorServer_ = nullptr;
    int32_t ret = sensorServiceClient->LoadSensorService();
    EXPECT_EQ(ret, ERR_OK);
}
} // namespace Sensors
} // namespace OHOS
