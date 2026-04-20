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

#include "sensor_data_block_policy.h"

#include "sensor_errors.h"
#include "sensor_log.h"

#undef LOG_TAG
#define LOG_TAG "SensorDataBlockPolicy"

namespace OHOS {
namespace Sensors {

using namespace OHOS::HiviewDFX;

ErrCode SensorDataBlockPolicy::BlockSensorDataByPid(int32_t targetPid, const std::vector<int32_t> &sensorTypes,
                                                    int32_t clientPid)
{
    CALL_LOG_ENTER;
    if (targetPid <= 0 || clientPid <= 0) {
        SEN_HILOGE("Invalid pid, targetPid:%{public}d, clientPid:%{public}d", targetPid, clientPid);
        return PARAMETER_ERROR;
    }
    if (sensorTypes.empty()) {
        SEN_HILOGE("SensorTypes is empty");
        return PARAMETER_ERROR;
    }

    std::lock_guard<std::mutex> lock(blockPolicyMutex_);

    auto targetIt = blockPolicies_.find(targetPid);
    if (targetIt != blockPolicies_.end()) {
        auto clientIt = targetIt->second.find(clientPid);
        if (clientIt != targetIt->second.end()) {
            for (const auto &sensorType : sensorTypes) {
                clientIt->second.sensorTypes.insert(sensorType);
            }
            SEN_HILOGI("Update existing block policy for targetPid:%{public}d, clientPid:%{public}d",
                targetPid, clientPid);
            return ERR_OK;
        }
    }

    BlockPolicy policy(targetPid, clientPid);
    for (const auto &sensorType : sensorTypes) {
        policy.sensorTypes.insert(sensorType);
    }
    blockPolicies_[targetPid][clientPid] = policy;

    SEN_HILOGI("Add block policy for targetPid:%{public}d, clientPid:%{public}d, sensorCount:%{public}zu",
               targetPid, clientPid, policy.sensorTypes.size());
    return ERR_OK;
}

ErrCode SensorDataBlockPolicy::UnblockSensorDataByClient(int32_t clientPid, int32_t targetPid)
{
    CALL_LOG_ENTER;
    if (clientPid <= 0 || targetPid <= 0) {
        SEN_HILOGE("Invalid clientPid:%{public}d or targetPid:%{public}d", clientPid, targetPid);
        return PARAMETER_ERROR;
    }

    std::lock_guard<std::mutex> lock(blockPolicyMutex_);

    auto targetIt = blockPolicies_.find(targetPid);
    if (targetIt == blockPolicies_.end()) {
        SEN_HILOGW("No block policy found for targetPid:%{public}d", targetPid);
        return ERR_OK;
    }
    auto clientIt = targetIt->second.find(clientPid);
    if (clientIt == targetIt->second.end()) {
        SEN_HILOGW("No block policy found for clientPid:%{public}d", clientPid);
        return ERR_OK;
    }
    targetIt->second.erase(clientIt);
    SEN_HILOGI("Remove block policy for targetPid:%{public}d, clientPid:%{public}d",
        targetPid, clientPid);

    if (targetIt->second.empty()) {
        blockPolicies_.erase(targetIt);
        SEN_HILOGI("Remove targetPid:%{public}d as all client policies are cleared", targetPid);
    }
    return ERR_OK;
}

bool SensorDataBlockPolicy::IsSensorDataBlocked(int32_t targetPid, int32_t sensorType) const
{
    std::lock_guard<std::mutex> lock(blockPolicyMutex_);
    auto targetIt = blockPolicies_.find(targetPid);
    if (targetIt == blockPolicies_.end()) {
        return false;
    }
    for (const auto &clientEntry : targetIt->second) {
        if (clientEntry.second.sensorTypes.find(sensorType) != clientEntry.second.sensorTypes.end()) {
            return true;
        }
    }
    return false;
}

void SensorDataBlockPolicy::ClearBlockPolicyByClient(int32_t clientPid)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> lock(blockPolicyMutex_);
    for (auto targetIt = blockPolicies_.begin(); targetIt != blockPolicies_.end();) {
        targetIt->second.erase(clientPid);
        if (targetIt->second.empty()) {
            targetIt = blockPolicies_.erase(targetIt);
        } else {
            ++targetIt;
        }
    }
    SEN_HILOGI("Remove all block policies for clientPid:%{public}d", clientPid);
}

std::vector<int32_t> SensorDataBlockPolicy::GetBlockedSensorTypes(int32_t targetPid) const
{
    std::vector<int32_t> blockedTypes;
    std::lock_guard<std::mutex> lock(blockPolicyMutex_);
    auto targetIt = blockPolicies_.find(targetPid);
    if (targetIt == blockPolicies_.end()) {
        return blockedTypes;
    }
    for (const auto &clientEntry : targetIt->second) {
        for (const auto &sensorType : clientEntry.second.sensorTypes) {
            blockedTypes.push_back(sensorType);
        }
    }
    return blockedTypes;
}

void SensorDataBlockPolicy::ClearAllBlockPolicies()
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> lock(blockPolicyMutex_);
    size_t count = blockPolicies_.size();
    blockPolicies_.clear();
    SEN_HILOGI("Cleared all block policies, count:%{public}zu", count);
}

std::string SensorDataBlockPolicy::DumpBlockPolicies() const
{
    std::lock_guard<std::mutex> lock(blockPolicyMutex_);
    std::string dumpInfo = "Sensor Data Block Policies:\n";
    if (blockPolicies_.empty()) {
        dumpInfo += "  No active block policies\n";
        return dumpInfo;
    }

    for (const auto &targetEntry : blockPolicies_) {
        dumpInfo += "  TargetPid: " + std::to_string(targetEntry.first) + "\n";
        for (const auto &clientEntry : targetEntry.second) {
            const BlockPolicy &policy = clientEntry.second;
            dumpInfo += "  ClientPid: " + std::to_string(policy.clientPid) + "\n";
            dumpInfo += "  BlockedSensorTypes: ";
            for (const auto &sensorType : policy.sensorTypes) {
                dumpInfo += std::to_string(sensorType) + " ";
            }
            dumpInfo += "\n";
        }
    }
    return dumpInfo;
}
} // namespace Sensors
} // namespace OHOS
