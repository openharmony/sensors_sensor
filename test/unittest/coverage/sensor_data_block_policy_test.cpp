/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "sensor_basic_data_channel.h"
#include "sensor_agent_type.h"
#include "sensor_data_block_policy.h"
#include "sensor_errors.h"

#undef LOG_TAG
#define LOG_TAG "SensorDataBlockPolicyTest"

namespace OHOS {
namespace Sensors {
using namespace testing;
using namespace testing::ext;
using namespace OHOS::HiviewDFX;

namespace {
constexpr int32_t TEST_TARGET_PID = 1001;
constexpr int32_t TEST_CLIENT_PID = 2001;
constexpr int32_t TEST_CLIENT_PID_2 = 2002;
constexpr int32_t INVALID_PID = -1;
constexpr int32_t SENSOR_TYPE_ACCELEROMETER = SENSOR_TYPE_ID_ACCELEROMETER;
constexpr int32_t SENSOR_TYPE_GYROSCOPE = SENSOR_TYPE_ID_GYROSCOPE;
constexpr int32_t SENSOR_TYPE_MAGNETIC_FIELD = SENSOR_TYPE_ID_MAGNETIC_FIELD;
} // namespace

class SensorDataBlockPolicyTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    void SetUp() override
    {
        // Clear policies before each test case
        SensorDataBlockPolicy::GetInstance().ClearAllBlockPolicies();
    }

    void TearDown() override
    {
        // Clear policies after each test case
        SensorDataBlockPolicy::GetInstance().ClearAllBlockPolicies();
    }
};

/**
 * @tc.name: SensorDataBlockPolicy_BlockSensorDataByPid_001
 * @tc.desc: Test adding blocking policy normally
 * @tc.type: FUNC
 */
HWTEST_F(SensorDataBlockPolicyTest, SensorDataBlockPolicy_BlockSensorDataByPid_001, TestSize.Level1)
{
    auto &blockPolicy = SensorDataBlockPolicy::GetInstance();
    std::vector<int32_t> sensorTypes = {SENSOR_TYPE_ACCELEROMETER, SENSOR_TYPE_GYROSCOPE};

    ErrCode ret = blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes, TEST_CLIENT_PID);
    ASSERT_EQ(ret, ERR_OK);

    // Verify blocking policy is effective
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_GYROSCOPE));
    EXPECT_FALSE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_MAGNETIC_FIELD));
}

/**
 * @tc.name: SensorDataBlockPolicy_BlockSensorDataByPid_002
 * @tc.desc: Test adding empty sensor type list
 * @tc.type: FUNC
 */
HWTEST_F(SensorDataBlockPolicyTest, SensorDataBlockPolicy_BlockSensorDataByPid_002, TestSize.Level1)
{
    auto &blockPolicy = SensorDataBlockPolicy::GetInstance();
    std::vector<int32_t> sensorTypes; // empty list

    ErrCode ret = blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes, TEST_CLIENT_PID);
    ASSERT_EQ(ret, PARAMETER_ERROR);
}

/**
 * @tc.name: SensorDataBlockPolicy_BlockSensorDataByPid_003
 * @tc.desc: Test using invalid PID
 * @tc.type: FUNC
 */
HWTEST_F(SensorDataBlockPolicyTest, SensorDataBlockPolicy_BlockSensorDataByPid_003, TestSize.Level1)
{
    auto &blockPolicy = SensorDataBlockPolicy::GetInstance();
    std::vector<int32_t> sensorTypes = {SENSOR_TYPE_ACCELEROMETER};

    // Test invalid target PID
    ErrCode ret1 = blockPolicy.BlockSensorDataByPid(INVALID_PID, sensorTypes, TEST_CLIENT_PID);
    ASSERT_EQ(ret1, PARAMETER_ERROR);

    // Test invalid client PID
    ErrCode ret2 = blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes, INVALID_PID);
    ASSERT_EQ(ret2, PARAMETER_ERROR);
}

/**
 * @tc.name: SensorDataBlockPolicy_BlockSensorDataByPid_004
 * @tc.desc: Test updating existing blocking policy
 * @tc.type: FUNC
 */
HWTEST_F(SensorDataBlockPolicyTest, SensorDataBlockPolicy_BlockSensorDataByPid_004, TestSize.Level1)
{
    auto &blockPolicy = SensorDataBlockPolicy::GetInstance();

    // First time adding blocking policy
    std::vector<int32_t> sensorTypes1 = {SENSOR_TYPE_ACCELEROMETER};
    ErrCode ret1 = blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes1, TEST_CLIENT_PID);
    ASSERT_EQ(ret1, ERR_OK);
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));

    // Update blocking policy, add new sensor types
    std::vector<int32_t> sensorTypes2 = {SENSOR_TYPE_GYROSCOPE, SENSOR_TYPE_MAGNETIC_FIELD};
    ErrCode ret2 = blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes2, TEST_CLIENT_PID);
    ASSERT_EQ(ret2, ERR_OK);

    // Verify all sensor types are blocked
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_GYROSCOPE));
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_MAGNETIC_FIELD));
}

/**
 * @tc.name: SensorDataBlockPolicy_UnblockSensorDataByClient_001
 * @tc.desc: Test canceling client blocking policy with specific target PID
 * @tc.type: FUNC
 */
HWTEST_F(SensorDataBlockPolicyTest, SensorDataBlockPolicy_UnblockSensorDataByClient_001, TestSize.Level1)
{
    auto &blockPolicy = SensorDataBlockPolicy::GetInstance();

    // Add blocking policy
    std::vector<int32_t> sensorTypes = {SENSOR_TYPE_ACCELEROMETER};
    ErrCode ret1 = blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes, TEST_CLIENT_PID);
    ASSERT_EQ(ret1, ERR_OK);
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));

    // Cancel blocking policy with specific target PID
    ErrCode ret2 = blockPolicy.UnblockSensorDataByClient(TEST_CLIENT_PID, TEST_TARGET_PID);
    ASSERT_EQ(ret2, ERR_OK);
    EXPECT_FALSE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));
}

/**
 * @tc.name: SensorDataBlockPolicy_UnblockSensorDataByClient_002
 * @tc.desc: Test canceling client with non-existent policy
 * @tc.type: FUNC
 */
HWTEST_F(SensorDataBlockPolicyTest, SensorDataBlockPolicy_UnblockSensorDataByClient_002, TestSize.Level1)
{
    auto &blockPolicy = SensorDataBlockPolicy::GetInstance();

    // Cancel non-existent client policy with specific target PID, should return success (idempotent operation)
    ErrCode ret = blockPolicy.UnblockSensorDataByClient(TEST_CLIENT_PID, TEST_TARGET_PID);
    ASSERT_EQ(ret, ERR_OK);
}

/**
 * @tc.name: SensorDataBlockPolicy_UnblockSensorDataByClient_003
 * @tc.desc: Test independent policies of multiple clients
 * @tc.type: FUNC
 */
HWTEST_F(SensorDataBlockPolicyTest, SensorDataBlockPolicy_UnblockSensorDataByClient_003, TestSize.Level1)
{
    auto &blockPolicy = SensorDataBlockPolicy::GetInstance();

    // Client 1 adds blocking policy for target PID 1
    std::vector<int32_t> sensorTypes1 = {SENSOR_TYPE_ACCELEROMETER};
    ErrCode ret1 = blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes1, TEST_CLIENT_PID);
    ASSERT_EQ(ret1, ERR_OK);

    // Client 2 adds blocking policy for target PID 2
    constexpr int32_t testTargetPid2 = 1002;
    std::vector<int32_t> sensorTypes2 = {SENSOR_TYPE_GYROSCOPE};
    ErrCode ret2 = blockPolicy.BlockSensorDataByPid(testTargetPid2, sensorTypes2, TEST_CLIENT_PID_2);
    ASSERT_EQ(ret2, ERR_OK);

    // Cancel client 1's policy with specific target PID
    ErrCode ret3 = blockPolicy.UnblockSensorDataByClient(TEST_CLIENT_PID, TEST_TARGET_PID);
    ASSERT_EQ(ret3, ERR_OK);

    // Verify client 1's policy is cleared, client 2's policy still exists
    EXPECT_FALSE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(testTargetPid2, SENSOR_TYPE_GYROSCOPE));
}

/**
 * @tc.name: SensorDataBlockPolicy_IsSensorDataBlocked_001
 * @tc.desc: Test checking blocking status
 * @tc.type: FUNC
 */
HWTEST_F(SensorDataBlockPolicyTest, SensorDataBlockPolicy_IsSensorDataBlocked_001, TestSize.Level1)
{
    auto &blockPolicy = SensorDataBlockPolicy::GetInstance();

    // When not blocked
    EXPECT_FALSE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));

    // After blocking
    std::vector<int32_t> sensorTypes = {SENSOR_TYPE_ACCELEROMETER};
    blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes, TEST_CLIENT_PID);
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));
}

/**
 * @tc.name: SensorDataBlockPolicy_GetBlockedSensorTypes_001
 * @tc.desc: Test getting list of blocked sensor types
 * @tc.type: FUNC
 */
HWTEST_F(SensorDataBlockPolicyTest, SensorDataBlockPolicy_GetBlockedSensorTypes_001, TestSize.Level1)
{
    auto &blockPolicy = SensorDataBlockPolicy::GetInstance();

    // When not blocked
    std::vector<int32_t> blockedTypes = blockPolicy.GetBlockedSensorTypes(TEST_TARGET_PID);
    ASSERT_TRUE(blockedTypes.empty());

    // After blocking
    std::vector<int32_t> sensorTypes = {SENSOR_TYPE_ACCELEROMETER, SENSOR_TYPE_GYROSCOPE};
    blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes, TEST_CLIENT_PID);
    blockedTypes = blockPolicy.GetBlockedSensorTypes(TEST_TARGET_PID);
    ASSERT_EQ(blockedTypes.size(), 2u);
}

/**
 * @tc.name: SensorDataBlockPolicy_ClearBlockPolicyByClient_001
 * @tc.desc: Test clearing client policies
 * @tc.type: FUNC
 */
HWTEST_F(SensorDataBlockPolicyTest, SensorDataBlockPolicy_ClearBlockPolicyByClient_001, TestSize.Level1)
{
    auto &blockPolicy = SensorDataBlockPolicy::GetInstance();

    // Add blocking policy
    std::vector<int32_t> sensorTypes = {SENSOR_TYPE_ACCELEROMETER};
    blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes, TEST_CLIENT_PID);

    // Clear client policies
    blockPolicy.ClearBlockPolicyByClient(TEST_CLIENT_PID);
    EXPECT_FALSE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));
}

/**
 * @tc.name: SensorDataBlockPolicy_ClearAllBlockPolicies_001
 * @tc.desc: Test clearing all policies
 * @tc.type: FUNC
 */
HWTEST_F(SensorDataBlockPolicyTest, SensorDataBlockPolicy_ClearAllBlockPolicies_001, TestSize.Level1)
{
    auto &blockPolicy = SensorDataBlockPolicy::GetInstance();

    // Add multiple blocking policies
    std::vector<int32_t> sensorTypes1 = {SENSOR_TYPE_ACCELEROMETER};
    std::vector<int32_t> sensorTypes2 = {SENSOR_TYPE_GYROSCOPE};
    blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes1, TEST_CLIENT_PID);
    blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID + 1, sensorTypes2, TEST_CLIENT_PID_2);

    // Clear all policies
    blockPolicy.ClearAllBlockPolicies();
    EXPECT_FALSE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));
    EXPECT_FALSE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID + 1, SENSOR_TYPE_GYROSCOPE));
}

/**
 * @tc.name: SensorDataBlockPolicy_DumpBlockPolicies_001
 * @tc.desc: Test exporting blocking policy information
 * @tc.type: FUNC
 */
HWTEST_F(SensorDataBlockPolicyTest, SensorDataBlockPolicy_DumpBlockPolicies_001, TestSize.Level1)
{
    auto &blockPolicy = SensorDataBlockPolicy::GetInstance();

    // When policy is empty
    std::string dumpInfo = blockPolicy.DumpBlockPolicies();
    ASSERT_FALSE(dumpInfo.empty());

    // After adding policy
    std::vector<int32_t> sensorTypes = {SENSOR_TYPE_ACCELEROMETER};
    blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes, TEST_CLIENT_PID);
    dumpInfo = blockPolicy.DumpBlockPolicies();
    ASSERT_FALSE(dumpInfo.empty());
    // Verify includes target PID and client PID
    EXPECT_NE(dumpInfo.find(std::to_string(TEST_TARGET_PID)), std::string::npos);
    EXPECT_NE(dumpInfo.find(std::to_string(TEST_CLIENT_PID)), std::string::npos);
}

/**
 * @tc.name: SensorDataBlockPolicy_UnblockSensorDataByClient_WithTarget_001
 * @tc.desc: Test unblocking sensor data by client with specific target PID
 * @tc.type: FUNC
 */
HWTEST_F(SensorDataBlockPolicyTest, SensorDataBlockPolicy_UnblockSensorDataByClient_WithTarget_001, TestSize.Level1)
{
    auto &blockPolicy = SensorDataBlockPolicy::GetInstance();
    constexpr int32_t testTargetPid2 = 1002;

    // Add blocking policies for different target PIDs
    std::vector<int32_t> sensorTypes1 = {SENSOR_TYPE_ACCELEROMETER};
    std::vector<int32_t> sensorTypes2 = {SENSOR_TYPE_GYROSCOPE};
    blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes1, TEST_CLIENT_PID);
    blockPolicy.BlockSensorDataByPid(testTargetPid2, sensorTypes2, TEST_CLIENT_PID);

    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(testTargetPid2, SENSOR_TYPE_GYROSCOPE));

    // Unblock specific target PID
    ErrCode ret = blockPolicy.UnblockSensorDataByClient(TEST_CLIENT_PID, TEST_TARGET_PID);
    ASSERT_EQ(ret, ERR_OK);

    // Verify only the specific target PID policy is cleared
    EXPECT_FALSE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(testTargetPid2, SENSOR_TYPE_GYROSCOPE));
}

/**
 * @tc.name: SensorDataBlockPolicy_UnblockSensorDataByClient_WithTarget_002
 * @tc.desc: Test unblocking with invalid parameters
 * @tc.type: FUNC
 */
HWTEST_F(SensorDataBlockPolicyTest, SensorDataBlockPolicy_UnblockSensorDataByClient_WithTarget_002, TestSize.Level1)
{
    auto &blockPolicy = SensorDataBlockPolicy::GetInstance();

    // Test invalid client PID
    ErrCode ret1 = blockPolicy.UnblockSensorDataByClient(INVALID_PID, TEST_TARGET_PID);
    ASSERT_EQ(ret1, PARAMETER_ERROR);

    // Test invalid target PID
    ErrCode ret2 = blockPolicy.UnblockSensorDataByClient(TEST_CLIENT_PID, INVALID_PID);
    ASSERT_EQ(ret2, PARAMETER_ERROR);
}

/**
 * @tc.name: SensorDataBlockPolicy_MultipleClients_SameTarget_001
 * @tc.desc: Test multiple clients blocking same target PID with different sensor types
 * @tc.type: FUNC
 */
HWTEST_F(SensorDataBlockPolicyTest, SensorDataBlockPolicy_MultipleClients_SameTarget_001, TestSize.Level1)
{
    auto &blockPolicy = SensorDataBlockPolicy::GetInstance();
    constexpr int32_t testTargetPid2 = 1002;

    // Client 1 blocks accelerometer
    std::vector<int32_t> sensorTypes1 = {SENSOR_TYPE_ACCELEROMETER};
    blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes1, TEST_CLIENT_PID);

    // Client 2 blocks gyroscope
    std::vector<int32_t> sensorTypes2 = {SENSOR_TYPE_GYROSCOPE};
    blockPolicy.BlockSensorDataByPid(testTargetPid2, sensorTypes2, TEST_CLIENT_PID_2);

    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(testTargetPid2, SENSOR_TYPE_GYROSCOPE));

    // Client 1 unblocks with specific target PID
    blockPolicy.UnblockSensorDataByClient(TEST_CLIENT_PID, TEST_TARGET_PID);
    EXPECT_FALSE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));
    // Client 2 policy still exists
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(testTargetPid2, SENSOR_TYPE_GYROSCOPE));
}

/**
 * @tc.name: SensorDataBlockPolicy_BlockSensorDataByPid_MultipleSensorTypes_001
 * @tc.desc: Test blocking multiple sensor types at once
 * @tc.type: FUNC
 */
HWTEST_F(SensorDataBlockPolicyTest, SensorDataBlockPolicy_BlockSensorDataByPid_MultipleSensorTypes_001, TestSize.Level1)
{
    auto &blockPolicy = SensorDataBlockPolicy::GetInstance();

    // Block multiple sensor types
    std::vector<int32_t> sensorTypes = {
        SENSOR_TYPE_ACCELEROMETER,
        SENSOR_TYPE_GYROSCOPE,
        SENSOR_TYPE_MAGNETIC_FIELD
    };
    ErrCode ret = blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes, TEST_CLIENT_PID);
    ASSERT_EQ(ret, ERR_OK);

    // Verify all sensor types are blocked
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_GYROSCOPE));
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_MAGNETIC_FIELD));
}

/**
 * @tc.name: SensorDataBlockPolicy_ClearBlockPolicyByClient_002
 * @tc.desc: Test clearing non-existent client policies
 * @tc.type: FUNC
 */
HWTEST_F(SensorDataBlockPolicyTest, SensorDataBlockPolicy_ClearBlockPolicyByClient_002, TestSize.Level1)
{
    auto &blockPolicy = SensorDataBlockPolicy::GetInstance();

    // Add a blocking policy
    std::vector<int32_t> sensorTypes = {SENSOR_TYPE_ACCELEROMETER};
    blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes, TEST_CLIENT_PID);

    // Clear a different client's policies (no effect on existing policy)
    blockPolicy.ClearBlockPolicyByClient(TEST_CLIENT_PID_2);
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));

    // Clear the actual client's policies
    blockPolicy.ClearBlockPolicyByClient(TEST_CLIENT_PID);
    EXPECT_FALSE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));
}

/**
 * @tc.name: SensorDataBlockPolicy_GetBlockedSensorTypes_002
 * @tc.desc: Test getting blocked sensor types when no policy exists
 * @tc.type: FUNC
 */
HWTEST_F(SensorDataBlockPolicyTest, SensorDataBlockPolicy_GetBlockedSensorTypes_002, TestSize.Level1)
{
    auto &blockPolicy = SensorDataBlockPolicy::GetInstance();

    // Get blocked sensor types for non-existent target PID
    std::vector<int32_t> blockedTypes = blockPolicy.GetBlockedSensorTypes(9999);
    ASSERT_TRUE(blockedTypes.empty());
}

/**
 * @tc.name: SensorDataBlockPolicy_UpdateExistingPolicy_001
 * @tc.desc: Test updating existing blocking policy with new sensor types
 * @tc.type: FUNC
 */
HWTEST_F(SensorDataBlockPolicyTest, SensorDataBlockPolicy_UpdateExistingPolicy_001, TestSize.Level1)
{
    auto &blockPolicy = SensorDataBlockPolicy::GetInstance();

    // First blocking: only accelerometer
    std::vector<int32_t> sensorTypes1 = {SENSOR_TYPE_ACCELEROMETER};
    blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes1, TEST_CLIENT_PID);
    ASSERT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));

    // Update: add gyroscope (should merge with existing)
    std::vector<int32_t> sensorTypes2 = {SENSOR_TYPE_GYROSCOPE};
    blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes2, TEST_CLIENT_PID);

    // Both should be blocked
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_GYROSCOPE));
}

/**
 * @tc.name: SensorDataBlockPolicy_PolicyIsolation_001
 * @tc.desc: Test that different target PIDs have isolated policies
 * @tc.type: FUNC
 */
HWTEST_F(SensorDataBlockPolicyTest, SensorDataBlockPolicy_PolicyIsolation_001, TestSize.Level1)
{
    auto &blockPolicy = SensorDataBlockPolicy::GetInstance();
    constexpr int32_t testTargetPid2 = 1002;

    // Block different sensor types for different target PIDs
    std::vector<int32_t> sensorTypes1 = {SENSOR_TYPE_ACCELEROMETER};
    std::vector<int32_t> sensorTypes2 = {SENSOR_TYPE_GYROSCOPE};
    blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes1, TEST_CLIENT_PID);
    blockPolicy.BlockSensorDataByPid(testTargetPid2, sensorTypes2, TEST_CLIENT_PID);

    // Verify isolation
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));
    EXPECT_FALSE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_GYROSCOPE));
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(testTargetPid2, SENSOR_TYPE_GYROSCOPE));
    EXPECT_FALSE(blockPolicy.IsSensorDataBlocked(testTargetPid2, SENSOR_TYPE_ACCELEROMETER));
}

/**
 * @tc.name: SensorDataBlockPolicy_MultipleClients_SameTargetPid_001
 * @tc.desc: Test multiple clients blocking the same target PID with different sensor types
 * @tc.type: FUNC
 */
HWTEST_F(SensorDataBlockPolicyTest, SensorDataBlockPolicy_MultipleClients_SameTargetPid_001, TestSize.Level1)
{
    auto &blockPolicy = SensorDataBlockPolicy::GetInstance();

    // Client 1 blocks accelerometer for target PID
    std::vector<int32_t> sensorTypes1 = {SENSOR_TYPE_ACCELEROMETER};
    ErrCode ret1 = blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes1, TEST_CLIENT_PID);
    ASSERT_EQ(ret1, ERR_OK);

    // Client 2 blocks gyroscope for the same target PID
    std::vector<int32_t> sensorTypes2 = {SENSOR_TYPE_GYROSCOPE};
    ErrCode ret2 = blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes2, TEST_CLIENT_PID_2);
    ASSERT_EQ(ret2, ERR_OK);

    // Both sensor types should be blocked (different clients)
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_GYROSCOPE));

    // Client 1 unblocks - only client 1's policy should be removed
    ErrCode ret3 = blockPolicy.UnblockSensorDataByClient(TEST_CLIENT_PID, TEST_TARGET_PID);
    ASSERT_EQ(ret3, ERR_OK);

    // Client 1's policy is cleared, client 2's policy still exists
    EXPECT_FALSE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_GYROSCOPE));

    // Client 2 unblocks
    ErrCode ret4 = blockPolicy.UnblockSensorDataByClient(TEST_CLIENT_PID_2, TEST_TARGET_PID);
    ASSERT_EQ(ret4, ERR_OK);

    // All policies should be cleared
    EXPECT_FALSE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));
    EXPECT_FALSE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_GYROSCOPE));
}

/**
 * @tc.name: SensorDataBlockPolicy_MultipleClients_SameTargetPid_002
 * @tc.desc: Test same client adding different sensor types to same target PID
 * @tc.type: FUNC
 */
HWTEST_F(SensorDataBlockPolicyTest, SensorDataBlockPolicy_MultipleClients_SameTargetPid_002, TestSize.Level1)
{
    auto &blockPolicy = SensorDataBlockPolicy::GetInstance();

    // Client adds accelerometer first
    std::vector<int32_t> sensorTypes1 = {SENSOR_TYPE_ACCELEROMETER};
    ErrCode ret1 = blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes1, TEST_CLIENT_PID);
    ASSERT_EQ(ret1, ERR_OK);

    // Same client adds gyroscope
    std::vector<int32_t> sensorTypes2 = {SENSOR_TYPE_GYROSCOPE};
    ErrCode ret2 = blockPolicy.BlockSensorDataByPid(TEST_TARGET_PID, sensorTypes2, TEST_CLIENT_PID);
    ASSERT_EQ(ret2, ERR_OK);

    // Both sensor types should be blocked (same client, policies should merge)
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));
    EXPECT_TRUE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_GYROSCOPE));

    // Client unblocks - all policies for this client should be cleared
    ErrCode ret3 = blockPolicy.UnblockSensorDataByClient(TEST_CLIENT_PID, TEST_TARGET_PID);
    ASSERT_EQ(ret3, ERR_OK);

    // All sensor types should be unblocked
    EXPECT_FALSE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_ACCELEROMETER));
    EXPECT_FALSE(blockPolicy.IsSensorDataBlocked(TEST_TARGET_PID, SENSOR_TYPE_GYROSCOPE));
}
} // namespace Sensors
} // namespace OHOS