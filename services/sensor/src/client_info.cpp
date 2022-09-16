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

#include "client_info.h"

#include <mutex>

#include "permission_util.h"
#include "securec.h"
#include "sensor_hdi_connection.h"
#include "sensor_manager.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "ClientInfo" };
constexpr uint32_t INVALID_SENSOR_ID = -1;
constexpr int32_t INVALID_PID = -1;
constexpr int32_t INVALID_UID = -1;
constexpr int32_t MIN_MAP_SIZE = 0;
constexpr uint32_t NO_STROE_EVENT = -2;
constexpr uint32_t MAX_SUPPORT_CHANNEL = 200;
constexpr uint32_t MAX_DUMP_DATA_SIZE = 10;
constexpr uint32_t HEART_RATE_SENSOR_ID = 83886336;
}  // namespace

bool ClientInfo::GetSensorState(uint32_t sensorId)
{
    CALL_LOG_ENTER;
    if (sensorId == INVALID_SENSOR_ID) {
        SEN_HILOGE("sensorId is invalid");
        return false;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    auto it = clientMap_.find(sensorId);
    if (it == clientMap_.end()) {
        SEN_HILOGE("cannot find sensorId:%{public}u", sensorId);
        return false;
    }
    for (const auto &pidIt : it->second) {
        if (pidIt.second.GetSensorState() == true) {
            return true;
        }
    }
    SEN_HILOGE("cannot find sensorinfo, sensorId:%{public}u", sensorId);
    return false;
}

SensorBasicInfo ClientInfo::GetBestSensorInfo(uint32_t sensorId)
{
    int64_t minSamplingPeriodNs = LLONG_MAX;
    int64_t minReportDelayNs = LLONG_MAX;
    SensorBasicInfo sensorInfo;
    sensorInfo.SetSamplingPeriodNs(minSamplingPeriodNs);
    sensorInfo.SetMaxReportDelayNs(minReportDelayNs);
    if (sensorId == INVALID_SENSOR_ID) {
        SEN_HILOGE("sensorId is invalid");
        return sensorInfo;
    }

    std::lock_guard<std::mutex> clientLock(clientMutex_);
    auto it = clientMap_.find(sensorId);
    if (it == clientMap_.end()) {
        SEN_HILOGE("cannot find sensorId:%{public}u", sensorId);
        return sensorInfo;
    }
    for (const auto &pidIt : it->second) {
        int64_t curSamplingPeriodNs = pidIt.second.GetSamplingPeriodNs();
        int64_t curReportDelayNs = pidIt.second.GetMaxReportDelayNs();
        minSamplingPeriodNs = (curSamplingPeriodNs < minSamplingPeriodNs) ? curSamplingPeriodNs : minSamplingPeriodNs;
        minReportDelayNs = (curReportDelayNs < minReportDelayNs) ? curReportDelayNs : minReportDelayNs;
    }
    sensorInfo.SetSamplingPeriodNs(minSamplingPeriodNs);
    sensorInfo.SetMaxReportDelayNs(minReportDelayNs);
    return sensorInfo;
}

bool ClientInfo::OnlyCurPidSensorEnabled(uint32_t sensorId, int32_t pid)
{
    CALL_LOG_ENTER;
    if ((sensorId == INVALID_SENSOR_ID) || (pid <= INVALID_PID)) {
        SEN_HILOGE("sensorId or pid is invalid");
        return false;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    auto it = clientMap_.find(sensorId);
    if (it == clientMap_.end()) {
        SEN_HILOGE("cannot find sensorId:%{public}u", sensorId);
        return false;
    }
    bool ret = false;
    for (const auto &pidIt : it->second) {
        if (!pidIt.second.GetSensorState()) {
            continue;
        }
        if (pidIt.first != pid) {
            return false;
        }
        ret = true;
    }
    return ret;
}

bool ClientInfo::UpdateAppThreadInfo(int32_t pid, int32_t uid, AccessTokenID callerToken)
{
    CALL_LOG_ENTER;
    if ((uid == INVALID_UID) || (pid <= INVALID_PID)) {
        SEN_HILOGE("uid or pid is invalid");
        return false;
    }
    std::lock_guard<std::mutex> uidLock(uidMutex_);
    AppThreadInfo appThreadInfo(pid, uid, callerToken);
    auto appThreadInfoItr = appThreadInfoMap_.find(pid);
    if (appThreadInfoItr == appThreadInfoMap_.end()) {
        if (appThreadInfoMap_.size() == MAX_SUPPORT_CHANNEL) {
            SEN_HILOGE("max support channel size is %{public}u", MAX_SUPPORT_CHANNEL);
            return false;
        }
        auto ret = appThreadInfoMap_.insert(std::make_pair(pid, appThreadInfo));
        return ret.second;
    }
    appThreadInfoMap_[pid] = appThreadInfo;
    return true;
}

void ClientInfo::DestroyAppThreadInfo(int32_t pid)
{
    CALL_LOG_ENTER;
    if (pid == INVALID_PID) {
        SEN_HILOGE("pid is invalid");
        return;
    }
    std::lock_guard<std::mutex> uidLock(uidMutex_);
    auto appThreadInfoItr = appThreadInfoMap_.find(pid);
    if (appThreadInfoItr == appThreadInfoMap_.end()) {
        SEN_HILOGD("uid not exist, no need to destroy it");
        return;
    }
    appThreadInfoMap_.erase(appThreadInfoItr);
}

std::vector<sptr<SensorBasicDataChannel>> ClientInfo::GetSensorChannelByUid(int32_t uid)
{
    CALL_LOG_ENTER;
    if (uid == INVALID_UID) {
        SEN_HILOGE("uid is invalid");
        return {};
    }
    std::vector<sptr<SensorBasicDataChannel>> sensorChannel;
    std::lock_guard<std::mutex> uidLock(uidMutex_);
    for (const auto &appThreadInfoIt : appThreadInfoMap_) {
        if (uid != appThreadInfoIt.second.uid) {
            continue;
        }
        std::lock_guard<std::mutex> channelLock(channelMutex_);
        auto channelIt = channelMap_.find(appThreadInfoIt.first);
        if (channelIt == channelMap_.end()) {
            continue;
        }
        sensorChannel.push_back(channelIt->second);
    }
    return sensorChannel;
}

sptr<SensorBasicDataChannel> ClientInfo::GetSensorChannelByPid(int32_t pid)
{
    CALL_LOG_ENTER;
    if (pid == INVALID_PID) {
        SEN_HILOGE("pid is invalid");
        return nullptr;
    }
    std::lock_guard<std::mutex> channelLock(channelMutex_);
    auto channelIt = channelMap_.find(pid);
    if (channelIt == channelMap_.end()) {
        SEN_HILOGE("there is no channel belong to the pid");
        return nullptr;
    }
    return channelIt->second;
}

std::vector<sptr<SensorBasicDataChannel>> ClientInfo::GetSensorChannel(uint32_t sensorId)
{
    if (sensorId == INVALID_SENSOR_ID) {
        SEN_HILOGE("sensorId is invalid");
        return {};
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    auto clientIt = clientMap_.find(sensorId);
    if (clientIt == clientMap_.end()) {
        SEN_HILOGD("there is no channel belong to sensorId:%{public}u", sensorId);
        return {};
    }
    std::vector<sptr<SensorBasicDataChannel>> sensorChannel;
    for (const auto &sensorInfoIt : clientIt->second) {
        std::lock_guard<std::mutex> channelLock(channelMutex_);
        auto channelIt = channelMap_.find(sensorInfoIt.first);
        if (channelIt == channelMap_.end()) {
            continue;
        }
        sensorChannel.push_back(channelIt->second);
    }
    return sensorChannel;
}

bool ClientInfo::UpdateSensorInfo(uint32_t sensorId, int32_t pid, const SensorBasicInfo &sensorInfo)
{
    CALL_LOG_ENTER;
    if ((sensorId == INVALID_SENSOR_ID) || (pid <= INVALID_PID) || (!sensorInfo.GetSensorState())) {
        SEN_HILOGE("params are invalid");
        return false;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    auto it = clientMap_.find(sensorId);
    if (it == clientMap_.end()) {
        std::unordered_map<int32_t, SensorBasicInfo> pidMap;
        auto pidRet = pidMap.insert(std::make_pair(pid, sensorInfo));
        auto clientRet = clientMap_.insert(std::make_pair(sensorId, pidMap));
        return pidRet.second && clientRet.second;
    }
    auto pidIt = it->second.find(pid);
    if (pidIt == it->second.end()) {
        auto ret = it->second.insert(std::make_pair(pid, sensorInfo));
        return ret.second;
    }
    it->second[pid] = sensorInfo;
    return true;
}

void ClientInfo::RemoveSubscriber(uint32_t sensorId, uint32_t pid)
{
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    auto it = clientMap_.find(sensorId);
    if (it == clientMap_.end()) {
        SEN_HILOGW("sensorId not exist");
        return;
    }
    auto pidIt = it->second.find(pid);
    if (pidIt != it->second.end()) {
        it->second.erase(pidIt);
    }
}

bool ClientInfo::UpdateSensorChannel(int32_t pid, const sptr<SensorBasicDataChannel> &channel)
{
    CALL_LOG_ENTER;
    CHKPR(channel, false);
    if (pid <= INVALID_PID) {
        SEN_HILOGE("pid or channel is invalid or channel cannot be null");
        return false;
    }
    std::lock_guard<std::mutex> channelLock(channelMutex_);
    auto it = channelMap_.find(pid);
    if (it == channelMap_.end()) {
        if (channelMap_.size() == MAX_SUPPORT_CHANNEL) {
            SEN_HILOGE("max support channel size:%{public}u", MAX_SUPPORT_CHANNEL);
            return false;
        }
        auto ret = channelMap_.insert(std::make_pair(pid, channel));
        SEN_HILOGD("ret.second:%{public}d", ret.second);
        return ret.second;
    }
    channelMap_[pid] = channel;
    return true;
}

void ClientInfo::ClearSensorInfo(uint32_t sensorId)
{
    CALL_LOG_ENTER;
    if (sensorId == INVALID_SENSOR_ID) {
        SEN_HILOGE("sensorId is invalid");
        return;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    auto it = clientMap_.find(sensorId);
    if (it == clientMap_.end()) {
        SEN_HILOGD("sensorId not exist, no need to clear it");
        return;
    }
    clientMap_.erase(it);
}

void ClientInfo::ClearCurPidSensorInfo(uint32_t sensorId, int32_t pid)
{
    CALL_LOG_ENTER;
    if ((sensorId == INVALID_SENSOR_ID) || (pid <= INVALID_PID)) {
        SEN_HILOGE("sensorId or pid is invalid");
        return;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    auto it = clientMap_.find(sensorId);
    if (it == clientMap_.end()) {
        SEN_HILOGD("sensorId not exist, no need to clear it");
        return;
    }
    auto pidIt = it->second.find(pid);
    if (pidIt == it->second.end()) {
        SEN_HILOGD("pid not exist, no need to clear it");
        return;
    }
    pidIt = it->second.erase(pidIt);
    if (it->second.size() == MIN_MAP_SIZE) {
        it = clientMap_.erase(it);
    }
}

bool ClientInfo::DestroySensorChannel(int32_t pid)
{
    CALL_LOG_ENTER;
    if (pid <= INVALID_PID) {
        SEN_HILOGE("pid is invalid");
        return false;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    for (auto it = clientMap_.begin(); it != clientMap_.end();) {
        auto pidIt = it->second.find(pid);
        if (pidIt == it->second.end()) {
            it++;
            continue;
        }
        pidIt = it->second.erase(pidIt);
        if (it->second.size() != MIN_MAP_SIZE) {
            it++;
            continue;
        }
        it = clientMap_.erase(it);
    }
    DestroyAppThreadInfo(pid);
    std::lock_guard<std::mutex> channelLock(channelMutex_);
    auto it = channelMap_.find(pid);
    if (it == channelMap_.end()) {
        SEN_HILOGD("there is no channel belong to pid, no need to destroy");
        return true;
    }
    it = channelMap_.erase(it);
    return true;
}

SensorBasicInfo ClientInfo::GetCurPidSensorInfo(uint32_t sensorId, int32_t pid)
{
    int64_t minSamplingPeriodNs = LLONG_MAX;
    int64_t minReportDelayNs = LLONG_MAX;
    SensorBasicInfo sensorInfo;
    sensorInfo.SetSamplingPeriodNs(minSamplingPeriodNs);
    sensorInfo.SetMaxReportDelayNs(minReportDelayNs);
    if ((sensorId == INVALID_SENSOR_ID) || (pid <= INVALID_PID)) {
        SEN_HILOGE("sensorId or channel is invalid");
        return sensorInfo;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    auto it = clientMap_.find(sensorId);
    if (it == clientMap_.end()) {
        SEN_HILOGE("cannot find sensorId:%{public}u", sensorId);
        return sensorInfo;
    }
    auto pidIt = it->second.find(pid);
    if (pidIt == it->second.end()) {
        SEN_HILOGE("cannot find pid:%{public}d", pid);
        return sensorInfo;
    }
    sensorInfo.SetSamplingPeriodNs(pidIt->second.GetSamplingPeriodNs());
    sensorInfo.SetMaxReportDelayNs(pidIt->second.GetMaxReportDelayNs());
    return sensorInfo;
}

uint64_t ClientInfo::ComputeBestPeriodCount(uint32_t sensorId, sptr<SensorBasicDataChannel> &channel)
{
    if (sensorId == INVALID_SENSOR_ID || channel == nullptr) {
        SEN_HILOGE("sensorId is invalid or channel cannot be null");
        return 0UL;
    }
    int32_t pid = INVALID_PID;
    {
        std::lock_guard<std::mutex> channelLock(channelMutex_);
        for (const auto &channelIt : channelMap_) {
            if (channelIt.second == channel) {
                pid = channelIt.first;
            }
        }
    }
    int64_t bestSamplingPeriod = GetBestSensorInfo(sensorId).GetSamplingPeriodNs();
    int64_t curSamplingPeriod = GetCurPidSensorInfo(sensorId, pid).GetSamplingPeriodNs();
    if (bestSamplingPeriod == 0L) {
        SEN_HILOGE("best Sensor Sampling Period is 0");
        return 0UL;
    }
    int64_t ret = curSamplingPeriod / bestSamplingPeriod;
    return (ret <= 0L) ? 0UL : ret;
}

uint64_t ClientInfo::ComputeBestFifoCount(uint32_t sensorId, sptr<SensorBasicDataChannel> &channel)
{
    if (channel == nullptr || sensorId == INVALID_SENSOR_ID) {
        SEN_HILOGE("sensorId is invalid or channel cannot be null");
        return 0UL;
    }
    int32_t pid = INVALID_PID;
    {
        std::lock_guard<std::mutex> channelLock(channelMutex_);
        for (const auto &channelIt : channelMap_) {
            if (channelIt.second == channel) {
                pid = channelIt.first;
            }
        }
    }
    int64_t curReportDelay = GetCurPidSensorInfo(sensorId, pid).GetMaxReportDelayNs();
    int64_t curSamplingPeriod = GetCurPidSensorInfo(sensorId, pid).GetSamplingPeriodNs();
    if (curSamplingPeriod == 0L) {
        SEN_HILOGE("best sensor fifo count is 0");
        return 0UL;
    }
    int64_t ret = curReportDelay / curSamplingPeriod;
    return (ret <= 0L) ? 0UL : ret;
}

int32_t ClientInfo::GetStoreEvent(int32_t sensorId, SensorEvent &event)
{
    std::lock_guard<std::mutex> lock(eventMutex_);
    auto storedEvent = storedEvent_.find(sensorId);
    if (storedEvent != storedEvent_.end()) {
        errno_t ret = memcpy_s(&event, sizeof(SensorEvent), &storedEvent->second, sizeof(SensorEvent));
        if (ret != EOK) {
            SEN_HILOGE("memcpy_s failed, sensorId:%{public}d", sensorId);
            return ret;
        }
        return ERR_OK;
    }

    SEN_HILOGE("can't get store event, sensorId:%{public}u", sensorId);
    return NO_STROE_EVENT;
}

void ClientInfo::StoreEvent(const SensorEvent &event)
{
    bool foundSensor = false;
    SensorEvent storedEvent;
    auto sensorHdiConnection = &SensorHdiConnection::GetInstance();
    if (sensorHdiConnection == nullptr) {
        SEN_HILOGE("sensorHdiConnection cannot be null");
        return;
    }
    std::vector<Sensor> sensors;
    int32_t ret = sensorHdiConnection->GetSensorList(sensors);
    if (ret != 0) {
        SEN_HILOGE("GetSensorList is failed");
        return;
    }
    errno_t retVal = memcpy_s(&storedEvent, sizeof(storedEvent), &event, sizeof(event));
    if (retVal != EOK) {
        SEN_HILOGE("memcpy_s is failed");
        return;
    }
    for (size_t i = 0; i < sensors.size(); i++) {
        if ((int32_t)(sensors[i].GetSensorId()) == storedEvent.sensorTypeId) {
            foundSensor = true;
            break;
        }
    }

    if (foundSensor) {
        std::lock_guard<std::mutex> lock(eventMutex_);
        storedEvent_[storedEvent.sensorTypeId] = storedEvent;
    }
}

bool ClientInfo::SaveClientPid(const sptr<IRemoteObject> &sensorClient, int32_t pid)
{
    CALL_LOG_ENTER;
    CHKPF(sensorClient);
    std::lock_guard<std::mutex> lock(clientPidMutex_);
    auto it = clientPidMap_.find(sensorClient);
    if (it == clientPidMap_.end()) {
        clientPidMap_.insert(std::make_pair(sensorClient, pid));
        return true;
    }
    clientPidMap_.insert(std::make_pair(sensorClient, pid));
    return true;
}

int32_t ClientInfo::FindClientPid(const sptr<IRemoteObject> &sensorClient)
{
    CALL_LOG_ENTER;
    CHKPR(sensorClient, INVALID_PID);
    std::lock_guard<std::mutex> lock(clientPidMutex_);
    auto it = clientPidMap_.find(sensorClient);
    if (it == clientPidMap_.end()) {
        SEN_HILOGE("cannot find client pid");
        return INVALID_PID;
    }
    return it->second;
}

void ClientInfo::DestroyClientPid(const sptr<IRemoteObject> &sensorClient)
{
    CALL_LOG_ENTER;
    CHKPV(sensorClient);
    std::lock_guard<std::mutex> lock(clientPidMutex_);
    auto it = clientPidMap_.find(sensorClient);
    if (it == clientPidMap_.end()) {
        SEN_HILOGE("cannot find client pid");
        return;
    }
    clientPidMap_.erase(it);
}

void ClientInfo::ClearEvent()
{
    std::lock_guard<std::mutex> lock(eventMutex_);
    storedEvent_.clear();
}

std::vector<uint32_t> ClientInfo::GetSensorIdByPid(int32_t pid)
{
    CALL_LOG_ENTER;
    std::vector<uint32_t> sensorIdVec;
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    for (const auto &itClientMap : clientMap_) {
        auto it = itClientMap.second.find(pid);
        if (it != itClientMap.second.end()) {
            sensorIdVec.push_back(itClientMap.first);
        }
    }
    return sensorIdVec;
}

AppThreadInfo ClientInfo::GetAppInfoByChannel(const sptr<SensorBasicDataChannel> &channel)
{
    CALL_LOG_ENTER;
    AppThreadInfo appThreadInfo;
    {
        std::lock_guard<std::mutex> channelLock(channelMutex_);
        for (auto channelIt = channelMap_.begin(); channelIt != channelMap_.end(); channelIt++) {
            if (channelIt->second == channel) {
                appThreadInfo.pid = channelIt->first;
            }
        }
    }
    {
        std::lock_guard<std::mutex> uidLock(uidMutex_);
        auto it = appThreadInfoMap_.find(appThreadInfo.pid);
        if (it != appThreadInfoMap_.end()) {
            appThreadInfo.uid = it->second.uid;
            appThreadInfo.callerToken = it->second.callerToken;
        }
    }
    return appThreadInfo;
}

void ClientInfo::GetSensorChannelInfo(std::vector<SensorChannelInfo> &channelInfo)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    for (const auto &sensorIt : clientMap_) {
        for (const auto &pidIt : sensorIt.second) {
            int32_t pid = pidIt.first;
            int32_t uid = GetUidByPid(pid);
            if (uid == INVALID_UID) {
                SEN_HILOGW("uid is invalid, uid:%{public}d", uid);
                continue;
            }
            SensorChannelInfo channel;
            channel.SetUid(uid);
            channel.SetSensorId(sensorIt.first);
            std::string packageName;
            SensorManager::GetInstance().GetPackageName(GetTokenIdByPid(pid), packageName);
            channel.SetPackageName(packageName);
            int64_t samplingPeriodNs = pidIt.second.GetSamplingPeriodNs();
            int64_t maxReportDelayNs = pidIt.second.GetMaxReportDelayNs();
            channel.SetSamplingPeriodNs(samplingPeriodNs);
            uint32_t fifoCount = (samplingPeriodNs == 0) ? 0 : (uint32_t)(maxReportDelayNs / samplingPeriodNs);
            channel.SetFifoCount(fifoCount);
            channel.SetCmdType(GetCmdList(sensorIt.first, uid));
            channelInfo.push_back(channel);
        }
    }
}

int32_t ClientInfo::GetUidByPid(int32_t pid)
{
    std::lock_guard<std::mutex> uidLock(uidMutex_);
    auto appThreadInfoIt = appThreadInfoMap_.find(pid);
    if (appThreadInfoIt == appThreadInfoMap_.end()) {
        return INVALID_UID;
    }
    return appThreadInfoIt->second.uid;
}

AccessTokenID ClientInfo::GetTokenIdByPid(int32_t pid)
{
    std::lock_guard<std::mutex> uidLock(uidMutex_);
    auto appThreadInfoIt = appThreadInfoMap_.find(pid);
    if (appThreadInfoIt == appThreadInfoMap_.end()) {
        return INVALID_UID;
    }
    return appThreadInfoIt->second.callerToken;
}

void ClientInfo::UpdateCmd(uint32_t sensorId, int32_t uid, int32_t cmdType)
{
    std::lock_guard<std::mutex> cmdLock(cmdMutex_);
    auto cmdIt = cmdMap_.find(sensorId);
    if (cmdIt == cmdMap_.end()) {
        std::unordered_map<int32_t, std::vector<int32_t>> cmds;
        std::vector<int32_t> tmp;
        tmp.push_back(cmdType);
        cmds.insert(std::make_pair(uid, tmp));
        cmdMap_.insert(std::make_pair(sensorId, cmds));
        return;
    }
    auto tmpIt = cmdIt->second.find(uid);
    if (tmpIt == cmdIt->second.end()) {
        std::vector<int32_t> tmp;
        tmp.push_back(cmdType);
        cmdIt->second.insert(std::make_pair(uid, tmp));
        return;
    }
    auto tmp = tmpIt->second;
    tmp.push_back(cmdType);
    cmdIt->second.insert(std::make_pair(uid, tmp));
}

void ClientInfo::DestroyCmd(int32_t uid)
{
    std::lock_guard<std::mutex> cmdLock(cmdMutex_);
    cmdMap_.erase(uid);
}

std::vector<int32_t> ClientInfo::GetCmdList(uint32_t sensorId, int32_t uid)
{
    std::lock_guard<std::mutex> cmdLock(cmdMutex_);
    auto cmdIt = cmdMap_.find(sensorId);
    if (cmdIt == cmdMap_.end()) {
        return {};
    }
    auto uidIt = cmdIt->second.find(uid);
    if (uidIt == cmdIt->second.end()) {
        return {};
    }
    return uidIt->second;
}

void ClientInfo::UpdateDataQueue(int32_t sensorId, SensorEvent &event)
{
    if (sensorId == HEART_RATE_SENSOR_ID) {
        return;
    }
    std::lock_guard<std::mutex> queueLock(dataQueueMutex_);
    TransferSensorEvents  transferEvent = {
        .sensorTypeId = event.sensorTypeId,
        .version = event.version,
        .timestamp = event.timestamp,
        .option = event.option,
        .mode = event.mode,
        .dataLen = event.dataLen
    };
    if (memcpy_s(transferEvent.data, event.dataLen, event.data, event.dataLen) != EOK) {
        SEN_HILOGE("Copy data failed");
        return;
    }
    auto it = dumpQueue_.find(sensorId);
    if (it == dumpQueue_.end()) {
        std::queue<TransferSensorEvents> q;
        q.push(transferEvent);
        dumpQueue_.insert(std::make_pair(sensorId, q));
        return;
    }
    it->second.push(transferEvent);
    if (it->second.size() > MAX_DUMP_DATA_SIZE) {
        it->second.pop();
    }
}

std::unordered_map<uint32_t, std::queue<TransferSensorEvents>> ClientInfo::GetDumpQueue()
{
    return dumpQueue_;
}

void ClientInfo::ClearDataQueue(int32_t sensorId)
{
    std::lock_guard<std::mutex> queueLock(dataQueueMutex_);
    auto it = dumpQueue_.find(sensorId);
    if (it != dumpQueue_.end()) {
        dumpQueue_.erase(it);
    }
}
}  // namespace Sensors
}  // namespace OHOS
