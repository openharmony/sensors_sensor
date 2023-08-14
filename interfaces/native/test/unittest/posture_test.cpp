/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "sensor_agent_type.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace testing::ext;
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, OHOS::Sensors::SENSOR_LOG_DOMAIN, "PostureTest" };
constexpr int32_t INVALID_VALUE { -1 };
bool g_existPosture = false;
}  // namespace

class PostureTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void PostureTest::SetUpTestCase()
{
    SensorInfo *sensorInfo = nullptr;
    int32_t count = 0;
    int32_t ret = GetAllSensors(&sensorInfo, &count);
    ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
    if (sensorInfo == nullptr || count == 0) {
        SEN_HILOGE("SensorInfo is null or count is 0");
        return;
    }
    for (int32_t i = 0; i < count; ++i) {
        if (sensorInfo[i].sensorId == SENSOR_TYPE_ID_POSTURE) {
            g_existPosture = true;
            SEN_HILOGD("Exist posture sensor");
            break;
        }
    }
    SEN_HILOGD("Not exist posture sensor");
}

void PostureTest::TearDownTestCase() {}

void PostureTest::SetUp() {}

void PostureTest::TearDown() {}

void PostureDataCallbackImpl(SensorEvent *event)
{
    if (event == nullptr) {
        SEN_HILOGE("Event is null");
        return;
    }
    if (event[0].dataLen < sizeof(PostureData)) {
        SEN_HILOGE("Event dataLen less than posture data size, event.dataLen:%{public}u", event[0].dataLen);
        return;
    }
    PostureData *postureData = (PostureData *)event[0].data;
    SEN_HILOGD("sensorId:%{public}d, version:%{public}d, dataLen:%{public}u, gxm:%{public}f, "
        "gym:%{public}f, gzm:%{public}f, gxs:%{public}f, gys:%{public}f, gzs:%{public}f, angle:%{public}f",
        event[0].sensorTypeId, event[0].version, event[0].dataLen, (*postureData).gxm, (*postureData).gym,
        (*postureData).gzm, (*postureData).gxs, (*postureData).gys, (*postureData).gzs, (*postureData).angle);
}

void PostureDataCallbackImpl2(SensorEvent *event)
{
    if (event == nullptr) {
        SEN_HILOGE("Event is null");
        return;
    }
    if (event[0].dataLen < sizeof(PostureData)) {
        SEN_HILOGE("Event dataLen less than posture data size, event.dataLen:%{public}u", event[0].dataLen);
        return;
    }
    PostureData *postureData = (PostureData *)event[0].data;
    SEN_HILOGD("sensorId:%{public}d, version:%{public}d, dataLen:%{public}u, gxm:%{public}f, "
        "gym:%{public}f, gzm:%{public}f, gxs:%{public}f, gys:%{public}f, gzs:%{public}f, angle:%{public}f",
        event[0].sensorTypeId, event[0].version, event[0].dataLen, (*postureData).gxm, (*postureData).gym,
        (*postureData).gzm, (*postureData).gxs, (*postureData).gys, (*postureData).gzs, (*postureData).angle);
}

HWTEST_F(PostureTest, PostureTest_001, TestSize.Level1)
{
    SEN_HILOGI("PostureTest_001 in");
    if (g_existPosture) {
        int32_t ret = ActivateSensor(SENSOR_TYPE_ID_POSTURE, nullptr);
        ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    }
}

HWTEST_F(PostureTest, PostureTest_002, TestSize.Level1)
{
    SEN_HILOGI("PostureTest_002 in");
    if (g_existPosture) {
        SensorUser user;
        user.callback = PostureDataCallbackImpl;
        int32_t ret = ActivateSensor(INVALID_VALUE, &user);
        ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    }
}

HWTEST_F(PostureTest, PostureTest_003, TestSize.Level1)
{
    SEN_HILOGI("PostureTest_003 in");
    if (g_existPosture) {
        SensorUser user;
        user.callback = nullptr;
        int32_t ret = ActivateSensor(SENSOR_TYPE_ID_POSTURE, &user);
        ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    }
}

HWTEST_F(PostureTest, PostureTest_004, TestSize.Level1)
{
    SEN_HILOGI("PostureTest_004 in");
    if (g_existPosture) {
        int32_t ret = DeactivateSensor(SENSOR_TYPE_ID_POSTURE, nullptr);
        ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    }
}

HWTEST_F(PostureTest, PostureTest_005, TestSize.Level1)
{
    SEN_HILOGI("PostureTest_005 in");
    if (g_existPosture) {
        SensorUser user;
        user.callback = PostureDataCallbackImpl;
        int32_t ret = DeactivateSensor(INVALID_VALUE, &user);
        ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    }
}

HWTEST_F(PostureTest, PostureTest_006, TestSize.Level1)
{
    SEN_HILOGI("PostureTest_006 in");
    if (g_existPosture) {
        SensorUser user;
        user.callback = nullptr;
        int32_t ret = DeactivateSensor(SENSOR_TYPE_ID_POSTURE, &user);
        ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    }
}

HWTEST_F(PostureTest, PostureTest_007, TestSize.Level1)
{
    SEN_HILOGI("PostureTest_007 in");
    if (g_existPosture) {
        int32_t ret = SetBatch(SENSOR_TYPE_ID_POSTURE, nullptr, 100000000, 100000000);
        ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    }
}

HWTEST_F(PostureTest, PostureTest_008, TestSize.Level1)
{
    SEN_HILOGI("PostureTest_008 in");
    if (g_existPosture) {
        SensorUser user;
        user.callback = PostureDataCallbackImpl;
        int32_t ret = SetBatch(INVALID_VALUE, &user, 100000000, 100000000);
        ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    }
}

HWTEST_F(PostureTest, PostureTest_009, TestSize.Level1)
{
    SEN_HILOGI("PostureTest_009 in");
    if (g_existPosture) {
        SensorUser user;
        user.callback = nullptr;
        int32_t ret = SetBatch(SENSOR_TYPE_ID_POSTURE, &user, 100000000, 100000000);
        ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    }
}

HWTEST_F(PostureTest, PostureTest_010, TestSize.Level1)
{
    SEN_HILOGI("PostureTest_010 in");
    if (g_existPosture) {
        SensorUser user;
        user.callback = PostureDataCallbackImpl;
        int32_t ret = SetBatch(SENSOR_TYPE_ID_POSTURE, &user, INVALID_VALUE, INVALID_VALUE);
        ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    }
}

HWTEST_F(PostureTest, PostureTest_011, TestSize.Level1)
{
    SEN_HILOGI("PostureTest_011 in");
    if (g_existPosture) {
        int32_t ret = SubscribeSensor(SENSOR_TYPE_ID_POSTURE, nullptr);
        ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    }
}

HWTEST_F(PostureTest, PostureTest_012, TestSize.Level1)
{
    SEN_HILOGI("PostureTest_012 in");
    if (g_existPosture) {
        SensorUser user;
        user.callback = PostureDataCallbackImpl;
        int32_t ret = SubscribeSensor(INVALID_VALUE, &user);
        ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    }
}

HWTEST_F(PostureTest, PostureTest_013, TestSize.Level1)
{
    SEN_HILOGI("PostureTest_013 in");
    if (g_existPosture) {
        SensorUser user;
        user.callback = nullptr;
        int32_t ret = SubscribeSensor(SENSOR_TYPE_ID_POSTURE, &user);
        ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    }
}

HWTEST_F(PostureTest, PostureTest_014, TestSize.Level1)
{
    SEN_HILOGI("PostureTest_014 in");
    if (g_existPosture) {
        int32_t ret = UnsubscribeSensor(SENSOR_TYPE_ID_POSTURE, nullptr);
        ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    }
}

HWTEST_F(PostureTest, PostureTest_015, TestSize.Level1)
{
    SEN_HILOGI("PostureTest_015 in");
    if (g_existPosture) {
        SensorUser user;
        user.callback = PostureDataCallbackImpl;
        int32_t ret = UnsubscribeSensor(INVALID_VALUE, &user);
        ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    }
}

HWTEST_F(PostureTest, PostureTest_016, TestSize.Level1)
{
    SEN_HILOGI("PostureTest_016 in");
    if (g_existPosture) {
        SensorUser user;
        user.callback = nullptr;
        int32_t ret = UnsubscribeSensor(SENSOR_TYPE_ID_POSTURE, &user);
        ASSERT_NE(ret, OHOS::Sensors::SUCCESS);
    }
}

HWTEST_F(PostureTest, PostureTest_017, TestSize.Level1)
{
    SEN_HILOGI("PostureTest_017 in");
    if (g_existPosture) {
        SensorUser user;
        user.callback = PostureDataCallbackImpl;
        int32_t ret = SubscribeSensor(SENSOR_TYPE_ID_POSTURE, &user);
        ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
        ret = SetBatch(SENSOR_TYPE_ID_POSTURE, &user, 100000000, 100000000);
        ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
        ret = ActivateSensor(SENSOR_TYPE_ID_POSTURE, &user);
        ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
        ret = DeactivateSensor(SENSOR_TYPE_ID_POSTURE, &user);
        ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
        ret = UnsubscribeSensor(SENSOR_TYPE_ID_POSTURE, &user);
        ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
    }
}

HWTEST_F(PostureTest, PostureTest_018, TestSize.Level1)
{
    SEN_HILOGI("PostureTest_018 in");
    if (g_existPosture) {
        SensorUser user;
        user.callback = PostureDataCallbackImpl;
        int32_t ret = SubscribeSensor(SENSOR_TYPE_ID_POSTURE, &user);
        ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
        ret = SetBatch(SENSOR_TYPE_ID_POSTURE, &user, 100000000, 100000000);
        ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
        ret = ActivateSensor(SENSOR_TYPE_ID_POSTURE, &user);
        ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
        ret = DeactivateSensor(SENSOR_TYPE_ID_POSTURE, &user);
        ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
        ret = UnsubscribeSensor(SENSOR_TYPE_ID_POSTURE, &user);
        ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);

        SensorUser user2;
        user2.callback = PostureDataCallbackImpl2;
        ret = SubscribeSensor(SENSOR_TYPE_ID_POSTURE, &user2);
        ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
        ret = SetBatch(SENSOR_TYPE_ID_POSTURE, &user2, 200000000, 200000000);
        ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
        ret = ActivateSensor(SENSOR_TYPE_ID_POSTURE, &user2);
        ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
        ret = DeactivateSensor(SENSOR_TYPE_ID_POSTURE, &user2);
        ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
        ret = UnsubscribeSensor(SENSOR_TYPE_ID_POSTURE, &user2);
        ASSERT_EQ(ret, OHOS::Sensors::SUCCESS);
    }
}
}  // namespace Sensors
}  // namespace OHOS
