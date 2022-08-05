/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef CLIENT_INFO_H
#define CLIENT_INFO_H

#include <map>
#include <queue>
#include <unordered_map>
#include <vector>

#include "refbase.h"
#include "singleton.h"

#include "accesstoken_kit.h"
#include "iremote_object.h"
#include "nocopyable.h"

#include "app_thread_info.h"
#include "sensor_basic_data_channel.h"
#include "sensor_basic_info.h"
#include "sensor_channel_info.h"
#include "sensor_agent_type.h"

namespace OHOS {
namespace Sensors {
using Security::AccessToken::AccessTokenID;
class ClientInfo : public Singleton<ClientInfo> {
public:
    ClientInfo() = default;
    virtual ~ClientInfo() = default;
    bool GetSensorState(uint32_t sensorId);
    SensorBasicInfo GetBestSensorInfo(uint32_t sensorId);
    bool OnlyCurPidSensorEnabled(uint32_t sensorId, int32_t pid);
    std::vector<sptr<SensorBasicDataChannel>> GetSensorChannel(uint32_t sensorId);
    std::vector<sptr<SensorBasicDataChannel>> GetSensorChannelByUid(int32_t uid);
    sptr<SensorBasicDataChannel> GetSensorChannelByPid(int32_t pid);
    bool UpdateSensorInfo(uint32_t sensorId, int32_t pid, const SensorBasicInfo &sensorInfo);
    void RemoveSubscriber(uint32_t sensorId, uint32_t pid);
    bool UpdateSensorChannel(int32_t pid, const sptr<SensorBasicDataChannel> &channel);
    bool UpdateAppThreadInfo(int32_t pid, int32_t uid, AccessTokenID callerToken);
    void ClearSensorInfo(uint32_t sensorId);
    void ClearCurPidSensorInfo(uint32_t sensorId, int32_t pid);
    bool DestroySensorChannel(int32_t pid);
    void DestroyAppThreadInfo(int32_t pid);
    SensorBasicInfo GetCurPidSensorInfo(uint32_t sensorId, int32_t pid);
    uint64_t ComputeBestPeriodCount(uint32_t sensorId, sptr<SensorBasicDataChannel> &channel);
    uint64_t ComputeBestFifoCount(uint32_t sensorId, sptr<SensorBasicDataChannel> &channel);
    int32_t GetStoreEvent(int32_t sensorId, SensorEvent &event);
    void StoreEvent(const SensorEvent &event);
    void ClearEvent();
    AppThreadInfo GetAppInfoByChannel(const sptr<SensorBasicDataChannel> &channel);
    bool SaveClientPid(const sptr<IRemoteObject> &sensorClient, int32_t pid);
    int32_t FindClientPid(const sptr<IRemoteObject> &sensorClient);
    void DestroyClientPid(const sptr<IRemoteObject> &sensorClient);
    std::vector<uint32_t> GetSensorIdByPid(int32_t pid);
    void GetSensorChannelInfo(std::vector<SensorChannelInfo> &channelInfo);
    void UpdateCmd(uint32_t sensorId, int32_t uid, int32_t cmdType);
    void DestroyCmd(int32_t uid);
    void UpdateDataQueue(int32_t sensorId, SensorEvent &event);
    std::unordered_map<uint32_t, std::queue<TransferSensorEvents>> GetDumpQueue();
    void ClearDataQueue(int32_t sensorId);
    int32_t GetUidByPid(int32_t pid);
    AccessTokenID GetTokenIdByPid(int32_t pid);

private:
    DISALLOW_COPY_AND_MOVE(ClientInfo);
    std::vector<int32_t> GetCmdList(uint32_t sensorId, int32_t uid);
    std::mutex clientMutex_;
    std::mutex channelMutex_;
    std::mutex eventMutex_;
    std::mutex uidMutex_;
    std::mutex clientPidMutex_;
    std::mutex cmdMutex_;
    std::mutex dataQueueMutex_;
    std::unordered_map<uint32_t, std::unordered_map<int32_t, SensorBasicInfo>> clientMap_;
    std::unordered_map<int32_t, sptr<SensorBasicDataChannel>> channelMap_;
    std::unordered_map<int32_t, SensorEvent> storedEvent_;
    std::unordered_map<int32_t, AppThreadInfo> appThreadInfoMap_;
    std::map<sptr<IRemoteObject>, int32_t> clientPidMap_;
    std::unordered_map<uint32_t, std::unordered_map<int32_t, std::vector<int32_t>>> cmdMap_;
    std::unordered_map<uint32_t, std::queue<TransferSensorEvents>> dumpQueue_;
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // CLIENT_INFO_H
