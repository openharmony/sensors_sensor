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
#include <gtest/gtest.h>

#include "security_privacy_manager_plugin.h"
#include "sensor_shake_control_manager.h"
#include "sensor_errors.h"

#undef LOG_TAG
#define LOG_TAG "SensorShakeControlManagerTest"

namespace OHOS {
namespace Sensors {
using namespace testing::ext;
constexpr int32_t DEFAULT_USER_ID = 100;

class SensorShakeControlManagerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SensorShakeControlManagerTest::SetUpTestCase() {}

void SensorShakeControlManagerTest::TearDownTestCase() {}

void SensorShakeControlManagerTest::SetUp() {}

void SensorShakeControlManagerTest::TearDown() {}

HWTEST_F(SensorShakeControlManagerTest, ModifyAppPolicyTest, TestSize.Level1)
{
    SEN_HILOGI("ModifyAppPolicyTest in");
    bool isSupport = LoadSecurityPrivacyServer();
    if (isSupport) {
        AppPolicyEventExt appPolicyEventExt;
        appPolicyEventExt.objectId = "36012700";
        appPolicyEventExt.objectType = ObjectType::ACCESS_TOKEN_ID;
        appPolicyEventExt.bundleName = "sensor_unit_test";
        appPolicyEventExt.policyName = PolicyName::MOTION_SENSOR;
        int32_t ret = ModifyAppPolicy(DEFAULT_USER_ID, appPolicyEventExt);
        ASSERT_NE(ret, SUCCESS);
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(SensorShakeControlManagerTest, QueryAppPolicyByPolicyNameTest, TestSize.Level1)
{
    SEN_HILOGI("QueryAppPolicyByPolicyNameTest in");
    bool isSupport = LoadSecurityPrivacyServer();
    if (isSupport) {
        std::vector<AppPolicyEventExt> appPolicyEventList;
        int32_t ret = QueryAppPolicyByPolicyName(DEFAULT_USER_ID, PolicyName::MOTION_SENSOR,
            appPolicyEventList);
        ASSERT_EQ(ret, SUCCESS);
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(SensorShakeControlManagerTest, RegisterAppPolicyObserverTest, TestSize.Level1)
{
    SEN_HILOGI("RegisterAppPolicyObserverTest in");
    bool isSupport = LoadSecurityPrivacyServer();
    if (isSupport) {
        std::function<void()> updateFunc = [&]() { };
        int32_t ret = RegisterAppPolicyObserver(DEFAULT_USER_ID, updateFunc);
        ASSERT_EQ(ret, SUCCESS);
        ret = UnregisterAppPolicyObserver(DEFAULT_USER_ID);
        ASSERT_EQ(ret, SUCCESS);
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(SensorShakeControlManagerTest, UpdateRegisterObserverTest, TestSize.Level1)
{
    SEN_HILOGI("UpdateRegisterObserverTest in");
    bool isSupport = LoadSecurityPrivacyServer();
    if (isSupport) {
        std::atomic_bool shakeControlInitReady = false;
        SENSOR_SHAKE_CONTROL_MGR->UpdateRegisterShakeSensorControlObserver(shakeControlInitReady);
        ASSERT_EQ(shakeControlInitReady.load(), false);
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(SensorShakeControlManagerTest, CheckAppIsNeedControlTest, TestSize.Level1)
{
    SEN_HILOGI("CheckAppIsNeedControlTest in");
    bool isSupport = LoadSecurityPrivacyServer();
    if (isSupport) {
        std::string bundleName = "sensor_unit_test_mock";
        std::string tokenId = "36012700";
        bool result = SENSOR_SHAKE_CONTROL_MGR->CheckAppIsNeedControl(bundleName, tokenId, DEFAULT_USER_ID);
        ASSERT_EQ(result, false);
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(SensorShakeControlManagerTest, CheckAppInfoIsNeedModifyTest, TestSize.Level1)
{
    SEN_HILOGI("CheckAppInfoIsNeedModifyTest in");
    bool isSupport = LoadSecurityPrivacyServer();
    if (isSupport) {
        std::string bundleName = "sensor_unit_test_mock";
        std::string tokenId = "36012700";
        bool result = SENSOR_SHAKE_CONTROL_MGR->CheckAppInfoIsNeedModify(bundleName, tokenId, DEFAULT_USER_ID);
        ASSERT_EQ(result, true);
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(SensorShakeControlManagerTest, GetCurrentUserIdTest, TestSize.Level1)
{
    SEN_HILOGI("GetCurrentUserIdTest in");
    bool isSupport = LoadSecurityPrivacyServer();
    if (isSupport) {
        int32_t currentUserId = SENSOR_SHAKE_CONTROL_MGR->GetCurrentUserId();
        ASSERT_EQ(currentUserId, DEFAULT_USER_ID);
    } else {
        ASSERT_EQ(isSupport, false);
    }
}

HWTEST_F(SensorShakeControlManagerTest, UpdateCurrentUserIdTest, TestSize.Level1)
{
    SEN_HILOGI("UpdateCurrentUserIdTest in");
    bool isSupport = LoadSecurityPrivacyServer();
    if (isSupport) {
        int32_t ret = SENSOR_SHAKE_CONTROL_MGR->UpdateCurrentUserId();
        ASSERT_EQ(ret, SUCCESS);
    } else {
        ASSERT_EQ(isSupport, false);
    }
}
} // namespace Sensors
} // namespace OHOS