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
#include "sensor_service_load.h"
#include "system_ability_definition.h"

#undef LOG_TAG
#define LOG_TAG "LoadSensorServiceTest"

namespace OHOS {
namespace Sensors {
using namespace testing;
using namespace testing::ext;
using namespace OHOS::HiviewDFX;

class LoadSensorServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    SensorServiceLoad *sensorServiceLoad;
    SensorLoadCallback *callback;
};

void LoadSensorServiceTest::SetUpTestCase()
{
}

void LoadSensorServiceTest::TearDownTestCase()
{
}

void LoadSensorServiceTest::SetUp()
{
    sensorServiceLoad = new (std::nothrow) SensorServiceLoad();
    ASSERT_NE(sensorServiceLoad, nullptr);
    callback = new (std::nothrow) SensorLoadCallback();
    ASSERT_NE(callback, nullptr);
}

void LoadSensorServiceTest::TearDown()
{
    if (sensorServiceLoad != nullptr) {
        delete sensorServiceLoad;
        sensorServiceLoad = nullptr;
    }
    if (callback!= nullptr) {
        delete callback;
        callback = nullptr;
    }
}

HWTEST_F(LoadSensorServiceTest, SetLoadFinishTest_001, TestSize.Level1)
{
    SEN_HILOGI("SetLoadFinishTest_001 enter");
    ASSERT_NO_FATAL_FAILURE(sensorServiceLoad->SetLoadFinish());
}

HWTEST_F(LoadSensorServiceTest, OnLoadSystemAbilitySuccessTest_001, TestSize.Level1)
{
    SEN_HILOGI("OnLoadSystemAbilitySuccessTest_001 enter");
    int32_t systemAbilityId = 1;
    sptr<IRemoteObject> remoteObject = new (std::nothrow) IPCObjectStub();
    ASSERT_NO_FATAL_FAILURE(callback->OnLoadSystemAbilitySuccess(systemAbilityId, remoteObject));
}

HWTEST_F(LoadSensorServiceTest, OnLoadSystemAbilitySuccessTest_002, TestSize.Level1)
{
    SEN_HILOGI("OnLoadSystemAbilitySuccessTest_002 enter");
    int32_t systemAbilityId = 1;
    sptr<IRemoteObject> remoteObject = new (std::nothrow) IPCObjectStub();
    ASSERT_NO_FATAL_FAILURE(callback->OnLoadSystemAbilitySuccess(systemAbilityId, remoteObject));
}

HWTEST_F(LoadSensorServiceTest, OnLoadSystemAbilityFailTest_001, TestSize.Level1)
{
    SEN_HILOGI("OnLoadSystemAbilityFailTest_001 enter");
    int32_t systemAbilityId = 1;
    ASSERT_NO_FATAL_FAILURE(callback->OnLoadSystemAbilityFail(systemAbilityId));
}
} // namespace Sensors
} // namespace OHOS
