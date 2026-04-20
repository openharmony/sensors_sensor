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

#ifndef SENSOR_DATA_BLOCK_POLICY_H
#define SENSOR_DATA_BLOCK_POLICY_H

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "errors.h"
#include "sensor_agent_type.h"
#include "singleton.h"

namespace OHOS {
namespace Sensors {

struct BlockPolicy {
    int32_t targetPid;                             /**< Target application PID */
    std::unordered_set<int32_t> sensorTypes;       /**< List of sensor types to block */
    int32_t clientPid;                             /**< Client PID that issued this policy (screen casting service) */

    BlockPolicy() = default;
    BlockPolicy(int32_t target, int32_t client)
        : targetPid(target), clientPid(client) {}
};

class SensorDataBlockPolicy : public Singleton<SensorDataBlockPolicy> {
public:
    SensorDataBlockPolicy() = default;
    ~SensorDataBlockPolicy() = default;

    ErrCode BlockSensorDataByPid(int32_t targetPid, const std::vector<int32_t> &sensorTypes, int32_t clientPid);
    ErrCode UnblockSensorDataByClient(int32_t clientPid, int32_t targetPid);
    bool IsSensorDataBlocked(int32_t targetPid, int32_t sensorType) const;
    void ClearBlockPolicyByClient(int32_t clientPid);
    std::vector<int32_t> GetBlockedSensorTypes(int32_t targetPid) const;
    void ClearAllBlockPolicies();
    std::string DumpBlockPolicies() const;

private:
    mutable std::mutex blockPolicyMutex_;
    std::unordered_map<int32_t, std::unordered_map<int32_t, BlockPolicy>> blockPolicies_;
};

} // namespace Sensors
} // namespace OHOS
#endif // SENSOR_DATA_BLOCK_POLICY_H
