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
#include <set>

#include "singleton.h"

#include "accesstoken_kit.h"
#include "iremote_object.h"

#include "app_thread_info.h"
#include "sensor_agent_type.h"
#include "sensor_basic_data_channel.h"
#include "sensor_basic_info.h"
#include "sensor_channel_info.h"

namespace OHOS {
namespace Sensors {
using Security::AccessToken::AccessTokenID;
class ClientInfo : public Singleton<ClientInfo> {
public:
    ClientInfo() = default;
    virtual ~ClientInfo() = default;
    bool GetSensorState(const SensorDescription &sensorDesc);
    SensorBasicInfo GetBestSensorInfo(const SensorDescription &sensorDesc);
    bool OnlyCurPidSensorEnabled(const SensorDescription &sensorDesc, int32_t pid);
    std::vector<sptr<SensorBasicDataChannel>> GetSensorChannel(const SensorDescription &sensorDesc);
    std::vector<sptr<SensorBasicDataChannel>> GetSensorChannelByUid(int32_t uid);
    sptr<SensorBasicDataChannel> GetSensorChannelByPid(int32_t pid);
    bool UpdateSensorInfo(const SensorDescription &sensorDesc, int32_t pid, const SensorBasicInfo &sensorInfo);
    void RemoveSubscriber(const SensorDescription &sensorDesc, uint32_t pid);
    bool UpdateSensorChannel(int32_t pid, const sptr<SensorBasicDataChannel> &channel);
    bool UpdateAppThreadInfo(int32_t pid, int32_t uid, AccessTokenID callerToken);
    void ClearSensorInfo(const SensorDescription &sensorDesc);
    void ClearCurPidSensorInfo(const SensorDescription &sensorDesc, int32_t pid);
    bool DestroySensorChannel(int32_t pid);
    void DestroyAppThreadInfo(int32_t pid);
    SensorBasicInfo GetCurPidSensorInfo(const SensorDescription &sensorDesc, int32_t pid);
    uint64_t ComputeBestPeriodCount(const SensorDescription &sensorDesc, sptr<SensorBasicDataChannel> &channel);
    uint64_t ComputeBestFifoCount(const SensorDescription &sensorDesc, sptr<SensorBasicDataChannel> &channel);
    int32_t GetStoreEvent(const SensorDescription &sensorDesc, SensorData &data);
    void StoreEvent(const SensorData &data);
    void ClearEvent();
    AppThreadInfo GetAppInfoByChannel(const sptr<SensorBasicDataChannel> &channel);
    bool SaveClientPid(const sptr<IRemoteObject> &sensorClient, int32_t pid);
    int32_t FindClientPid(const sptr<IRemoteObject> &sensorClient);
    void DestroyClientPid(const sptr<IRemoteObject> &sensorClient);
    std::vector<SensorDescription> GetSensorIdByPid(int32_t pid);
    void GetSensorChannelInfo(std::vector<SensorChannelInfo> &channelInfo);
    void UpdateCmd(int32_t sensorType, int32_t uid, int32_t cmdType);
    void DestroyCmd(int32_t uid);
    void UpdateDataQueue(int32_t sensorType, SensorData &data);
    std::unordered_map<SensorDescription, std::queue<SensorData>> GetDumpQueue();
    int32_t GetUidByPid(int32_t pid);
    void ClearDataQueue(const SensorDescription &sensorDesc);
    AccessTokenID GetTokenIdByPid(int32_t pid);
    int32_t AddActiveInfoCBPid(int32_t pid);
    int32_t DelActiveInfoCBPid(int32_t pid);
    std::vector<int32_t> GetActiveInfoCBPid();
    bool CallingService(int32_t pid);
    int32_t GetPidByTokenId(AccessTokenID tokenId);
    void UpdatePermState(int32_t pid, int32_t sensorType, bool state);
    void ChangeSensorPerm(AccessTokenID tokenId, const std::string &permName, bool state);
    void SetDeviceStatus(uint32_t deviceStatus);
    uint32_t GetDeviceStatus();
    void SetDeviceType(int32_t deviceType);
    int32_t GetDeviceType() const;
    void SaveSensorClient(const sptr<IRemoteObject> &sensorClient);
    void DestroySensorClient(const sptr<IRemoteObject> &sensorClient);
    void SendMsgToClient(SensorPlugData info);
    bool IsSubscribe();

private:
    DISALLOW_COPY_AND_MOVE(ClientInfo);
    std::vector<int32_t> GetCmdList(int32_t sensorType, int32_t uid);
    std::mutex clientMutex_;
    std::mutex channelMutex_;
    std::mutex eventMutex_;
    std::mutex uidMutex_;
    std::mutex clientPidMutex_;
    std::mutex cmdMutex_;
    std::mutex dataQueueMutex_;
    std::mutex sensorClientMutex_;
    std::unordered_map<SensorDescription, std::unordered_map<int32_t, SensorBasicInfo>> clientMap_;
    std::unordered_map<int32_t, sptr<SensorBasicDataChannel>> channelMap_;
    std::unordered_map<SensorDescription, SensorData> storedEvent_;
    std::unordered_map<int32_t, AppThreadInfo> appThreadInfoMap_;
    std::map<sptr<IRemoteObject>, int32_t> clientPidMap_;
    std::unordered_map<int32_t, std::unordered_map<int32_t, std::vector<int32_t>>> cmdMap_;
    std::unordered_map<SensorDescription, std::queue<SensorData>> dumpQueue_;
    std::mutex activeInfoCBPidMutex_;
    std::unordered_set<int32_t> activeInfoCBPidSet_;
    static std::unordered_map<std::string, std::set<int32_t>> userGrantPermMap_;
    std::atomic<uint32_t> deviceStatus_;
    std::atomic<int32_t> deviceType_;
    std::vector<sptr<IRemoteObject>> sensorClients_;
};
} // namespace Sensors
} // namespace OHOS
#endif // CLIENT_INFO_H
