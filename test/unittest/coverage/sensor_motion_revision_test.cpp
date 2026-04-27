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

#include <cinttypes>
#include <memory>
#include <gtest/gtest.h>
#include <sys/socket.h>

#include "message_parcel.h"

#include "sensor_errors.h"
#include "motion_plugin.h"

#undef LOG_TAG
#define LOG_TAG "SensorMotionRevisionTest"

namespace OHOS {
namespace Sensors {
using namespace testing::ext;
using namespace OHOS::HiviewDFX;

struct AccData {
    float x;
    float y;
    float z;
};

struct RotationData {
    float x;
    float y;
    float z;
    float w;
};

class SensorMotionRevisionTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    void ReceiveData(int32_t length);
};

void SensorMotionRevisionTest::SetUpTestCase() {}

void SensorMotionRevisionTest::TearDownTestCase() {}

void SensorMotionRevisionTest::SetUp() {}

void SensorMotionRevisionTest::TearDown() {}

void SensorMotionRevisionTest::ReceiveData(int32_t length) {}

HWTEST_F(SensorMotionRevisionTest, SensorMotionRevisionTest_001, TestSize.Level1)
{
    SEN_HILOGI("SensorMotionRevisionTestTest_001 in");
    bool ret = LoadMotionSensorRevision();
    ASSERT_EQ(ret, true);
}

HWTEST_F(SensorMotionRevisionTest, SensorMotionRevisionTest_002, TestSize.Level1)
{
    SEN_HILOGI("SensorMotionRevisionTestTest_002 in");
    bool ret = LoadMotionSensorRevision();
    ASSERT_EQ(ret, true);
    UnloadMotionSensorRevision();
}

HWTEST_F(SensorMotionRevisionTest, SensorMotionRevisionTest_003, TestSize.Level1)
{
    SEN_HILOGI("SensorMotionRevisionTestTest_002 in");
    bool ret = LoadMotionSensorRevision();
    ASSERT_EQ(ret, true);
    SensorData data;
    data.sensorTypeId = 1;
    data.version = 2;
    data.timestamp = 123456789;
    data.option = 200000000;
    data.mode = 1;
    float values[3] = {0.264569, -0.361812, 9.842889};
    size_t floatSize = 3 * sizeof(float);
    memcpy_s(data.data, floatSize, values, floatSize);
    data.dataLen = 3;
    MotionSensorRevision(4, &data);
    auto tmp = reinterpret_cast<float *>(data.data);
    EXPECT_FLOAT_EQ(tmp[0], 0.361812f);
    EXPECT_FLOAT_EQ(tmp[1], 0.264569f);
    EXPECT_FLOAT_EQ(tmp[2], 9.842889f);
    UnloadMotionSensorRevision();
}

HWTEST_F(SensorMotionRevisionTest, SensorMotionRevisionTest_004, TestSize.Level1)
{
    SEN_HILOGI("SensorMotionRevisionTestTest_002 in");
    bool ret = LoadMotionSensorRevision();
    ASSERT_EQ(ret, true);
    SensorData data;
    data.sensorTypeId = 256;
    data.version = 2;
    data.timestamp = 123456789;
    data.option = 200000000;
    data.mode = 1;
    float values[3] = {293.750000, 1.410000, 0.970000};
    size_t floatSize = 3 * sizeof(float);
    memcpy_s(data.data, floatSize, values, floatSize);
    data.dataLen = 3;
    MotionSensorRevision(4, &data);
    auto tmp = reinterpret_cast<float *>(data.data);
    EXPECT_FLOAT_EQ(tmp[0], 23.750000f);
    EXPECT_FLOAT_EQ(tmp[1], -0.970000f);
    EXPECT_FLOAT_EQ(tmp[2], 1.410000f);
    UnloadMotionSensorRevision();
}

HWTEST_F(SensorMotionRevisionTest, SensorMotionRevisionTest_005, TestSize.Level1)
{
    SEN_HILOGI("SensorMotionRevisionTestTest_002 in");
    bool ret = LoadMotionSensorRevision();
    ASSERT_EQ(ret, true);
    SensorData data;
    data.sensorTypeId = 259;
    data.version = 2;
    data.timestamp = 123456789;
    data.option = 200000000;
    data.mode = 1;
    float values[4] = {-0.030738f, -0.022079f, 0.144792f, 0.988738f};
    size_t floatSize = 4 * sizeof(float);
    memcpy_s(data.data, floatSize, values, floatSize);
    data.dataLen = 4;
    MotionSensorRevision(4, &data); // grlb G 
    auto tmp = reinterpret_cast<float *>(data.data);
    EXPECT_FLOAT_EQ(tmp[2], -0.596760f);
    EXPECT_FLOAT_EQ(tmp[3], 0.801527f);
    UnloadMotionSensorRevision();
}
} // namespace Sensors
} // namespace OHOS