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

#include <cinttypes>
#include <memory>
#include <gtest/gtest.h>
#include <sys/socket.h>

#include "message_parcel.h"

#include "sensor_errors.h"
#include "sensor_basic_data_channel.h"

#undef LOG_TAG
#define LOG_TAG "SensorBasicDataChannelTest"

namespace OHOS {
namespace Sensors {
using namespace testing::ext;
using namespace OHOS::HiviewDFX;

namespace {
constexpr int32_t INVALID_FD = 2;
}  // namespace

class SensorBasicDataChannelTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SensorBasicDataChannelTest::SetUpTestCase() {}

void SensorBasicDataChannelTest::TearDownTestCase() {}

void SensorBasicDataChannelTest::SetUp() {}

void SensorBasicDataChannelTest::TearDown() {}

HWTEST_F(SensorBasicDataChannelTest, SensorBasicDataChannelTest_001, TestSize.Level1)
{
    SEN_HILOGI("SensorBasicDataChannelTest_001 in");

    SensorBasicDataChannel sensorChannel = SensorBasicDataChannel();
    MessageParcel data;
    int32_t ret = sensorChannel.CreateSensorBasicChannel();
    ASSERT_EQ(ret, ERR_OK);
    ret = sensorChannel.CreateSensorBasicChannel(data);
    ASSERT_EQ(ret, ERR_OK);

    ret = sensorChannel.SendToBinder(data);
    ASSERT_EQ(ret, ERR_OK);

    char buff[128] = {};
    ret = sensorChannel.SendData(static_cast<void *>(buff), sizeof(buff));
    ASSERT_EQ(ret, ERR_OK);

    ret = sensorChannel.ReceiveData(static_cast<void *>(buff), sizeof(buff));
    ASSERT_NE(ret, SENSOR_CHANNEL_RECEIVE_ADDR_ERR);

    sensorChannel.DestroySensorBasicChannel();
}

HWTEST_F(SensorBasicDataChannelTest, CreateSensorBasicChannel_001, TestSize.Level1)
{
    SEN_HILOGI("CreateSensorBasicChannel_001 in");

    SensorBasicDataChannel sensorChannel = SensorBasicDataChannel();
    MessageParcel data;
    data.WriteFileDescriptor(INVALID_FD);
    int32_t ret = sensorChannel.CreateSensorBasicChannel(data);
    ASSERT_EQ(ret, ERR_OK);

    ret = sensorChannel.CreateSensorBasicChannel(data);
    ASSERT_EQ(ret, ERR_OK);
}

HWTEST_F(SensorBasicDataChannelTest, CreateSensorBasicChannel_002, TestSize.Level1)
{
    SEN_HILOGI("CreateSensorBasicChannel_002 in");
    SensorBasicDataChannel sensorChannel = SensorBasicDataChannel();
    MessageParcel data;
    int32_t ret = sensorChannel.CreateSensorBasicChannel(data);
    ASSERT_EQ(ret, SENSOR_CHANNEL_READ_DESCRIPTOR_ERR);
}

HWTEST_F(SensorBasicDataChannelTest, SendToBinder_001, TestSize.Level1)
{
    SEN_HILOGI("SendToBinder_001 in");
    SensorBasicDataChannel sensorChannel = SensorBasicDataChannel();
    MessageParcel data;
    int32_t ret = sensorChannel.SendToBinder(data);
    ASSERT_EQ(ret, SENSOR_CHANNEL_SENDFD_ERR);
}

HWTEST_F(SensorBasicDataChannelTest, SendData_001, TestSize.Level1)
{
    SEN_HILOGI("SendData_001 in");
    SensorBasicDataChannel sensorChannel = SensorBasicDataChannel();
    char buff[128] = {};
    int32_t ret = sensorChannel.SendData(static_cast<void *>(buff), sizeof(buff));
    ASSERT_EQ(ret, SENSOR_CHANNEL_SEND_ADDR_ERR);
}

HWTEST_F(SensorBasicDataChannelTest, SendData_002, TestSize.Level1)
{
    SEN_HILOGI("SendData_002 in");
    SensorBasicDataChannel sensorChannel = SensorBasicDataChannel();
    MessageParcel data;
    data.WriteFileDescriptor(INVALID_FD);
    int32_t ret = sensorChannel.CreateSensorBasicChannel(data);
    ASSERT_EQ(ret, ERR_OK);
    char buff[128] = {};
    ret = sensorChannel.SendData(static_cast<void *>(buff), sizeof(buff));
    ASSERT_EQ(ret, SENSOR_CHANNEL_SEND_DATA_ERR);
}

HWTEST_F(SensorBasicDataChannelTest, ReceiveData_001, TestSize.Level1)
{
    SEN_HILOGI("ReceiveData_001 in");
    SensorBasicDataChannel sensorChannel = SensorBasicDataChannel();
    char buff[128] = {};
    int32_t ret = sensorChannel.ReceiveData(static_cast<void *>(buff), sizeof(buff));
    ASSERT_EQ(ret, SENSOR_CHANNEL_RECEIVE_ADDR_ERR);

    sensorChannel.CreateSensorBasicChannel();
    char *buff1 = nullptr;
    ret = sensorChannel.ReceiveData(static_cast<void *>(buff1), sizeof(buff1));
    ASSERT_EQ(ret, SENSOR_CHANNEL_RECEIVE_ADDR_ERR);

    sensorChannel.DestroySensorBasicChannel();
}

}  // namespace Sensors
}  // namespace OHOS
