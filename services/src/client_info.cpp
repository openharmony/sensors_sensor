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

#include "i_sensor_client.h"
#include "permission_util.h"
#include "securec.h"
#include "sensor_manager.h"
#include "sensor_client_proxy.h"
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
#include "sensor_hdi_connection.h"
#endif // HDF_DRIVERS_INTERFACE_SENSOR

#undef LOG_TAG
#define LOG_TAG "ClientInfo"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr int32_t INVALID_SENSOR_ID = -1;
constexpr int32_t INVALID_PID = -1;
constexpr int32_t INVALID_UID = -1;
constexpr int32_t MIN_MAP_SIZE = 0;
constexpr uint32_t NO_STORE_EVENT = -2;
constexpr uint32_t MAX_SUPPORT_CHANNEL = 200;
constexpr uint32_t MAX_DUMP_DATA_SIZE = 10;
} // namespace

std::unordered_map<std::string, std::set<int32_t>> ClientInfo::userGrantPermMap_ = {
    { ACTIVITY_MOTION_PERMISSION, { SENSOR_TYPE_ID_PEDOMETER_DETECTION, SENSOR_TYPE_ID_PEDOMETER } },
    { READ_HEALTH_DATA_PERMISSION, { SENSOR_TYPE_ID_HEART_RATE } }
};

bool ClientInfo::GetSensorState(const SensorDescription &sensorDesc)
{
    SEN_HILOGD("In, deviceIndex:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    if (sensorDesc.sensorType == INVALID_SENSOR_ID) {
        SEN_HILOGE("sensorType is invalid");
        return false;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    auto it = clientMap_.find(sensorDesc);
    if (it == clientMap_.end()) {
        SEN_HILOGE("Can't find sensorType:%{public}d", sensorDesc.sensorType);
        return false;
    }
    for (const auto &pidIt : it->second) {
        if (pidIt.second.GetSensorState()) {
            return true;
        }
    }
    SEN_HILOGE("Can't find sensorInfo, deviceIndex:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    return false;
}

SensorBasicInfo ClientInfo::GetBestSensorInfo(const SensorDescription &sensorDesc)
{
    int64_t minSamplingPeriodNs = LLONG_MAX;
    int64_t minReportDelayNs = LLONG_MAX;
    SensorBasicInfo sensorInfo;
    sensorInfo.SetSamplingPeriodNs(minSamplingPeriodNs);
    sensorInfo.SetMaxReportDelayNs(minReportDelayNs);
    if (sensorDesc.sensorType == INVALID_SENSOR_ID) {
        SEN_HILOGE("sensorType is invalid");
        return sensorInfo;
    }

    std::lock_guard<std::mutex> clientLock(clientMutex_);
    auto it = clientMap_.find(sensorDesc);
    if (it == clientMap_.end()) {
        SEN_HILOGE("Can't find deviceIndex:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d,"
            "peripheralId:%{public}d", sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId,
            sensorDesc.location);
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

bool ClientInfo::OnlyCurPidSensorEnabled(const SensorDescription &sensorDesc, int32_t pid)
{
    SEN_HILOGD("In, sensorType:%{public}d, pid:%{public}d", sensorDesc.sensorType, pid);
    if ((sensorDesc.sensorType == INVALID_SENSOR_ID) || (pid <= INVALID_PID)) {
        SEN_HILOGE("sensorType or pid is invalid");
        return false;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    auto it = clientMap_.find(sensorDesc);
    if (it == clientMap_.end()) {
        SEN_HILOGE("Can't find deviceIndex:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
            sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
        return false;
    }
    bool ret = false;
    for (const auto &pidIt : it->second) {
        if (!pidIt.second.GetSensorState()) {
            continue;
        }
        if (pidIt.first != pid) {
            SEN_HILOGE("Current sensor is also used by other pid");
            return false;
        }
        ret = true;
    }
    SEN_HILOGI("Done, sensorType:%{public}d, pid:%{public}d", sensorDesc.sensorType, pid);
    return ret;
}

bool ClientInfo::UpdateAppThreadInfo(int32_t pid, int32_t uid, AccessTokenID callerToken)
{
    SEN_HILOGD("In, pid:%{public}d", pid);
    if ((uid == INVALID_UID) || (pid <= INVALID_PID)) {
        SEN_HILOGE("uid or pid is invalid");
        return false;
    }
    std::lock_guard<std::mutex> uidLock(uidMutex_);
    AppThreadInfo appThreadInfo(pid, uid, callerToken);
    auto appThreadInfoItr = appThreadInfoMap_.find(pid);
    if (appThreadInfoItr == appThreadInfoMap_.end()) {
        if (appThreadInfoMap_.size() == MAX_SUPPORT_CHANNEL) {
            SEN_HILOGE("Max support channel size is %{public}d", MAX_SUPPORT_CHANNEL);
            return false;
        }
        auto ret = appThreadInfoMap_.insert(std::make_pair(pid, appThreadInfo));
        return ret.second;
    }
    appThreadInfoMap_[pid] = appThreadInfo;
    SEN_HILOGI("Done, pid:%{public}d", pid);
    return true;
}

void ClientInfo::DestroyAppThreadInfo(int32_t pid)
{
    SEN_HILOGD("In, pid:%{public}d", pid);
    if (pid == INVALID_PID) {
        SEN_HILOGE("pid is invalid");
        return;
    }
    std::lock_guard<std::mutex> uidLock(uidMutex_);
    auto appThreadInfoItr = appThreadInfoMap_.find(pid);
    if (appThreadInfoItr == appThreadInfoMap_.end()) {
        SEN_HILOGD("pid not exist, no need to destroy it");
        return;
    }
    appThreadInfoMap_.erase(appThreadInfoItr);
    SEN_HILOGI("Done, pid:%{public}d", pid);
}

std::vector<sptr<SensorBasicDataChannel>> ClientInfo::GetSensorChannelByUid(int32_t uid)
{
    SEN_HILOGD("In");
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
    SEN_HILOGI("Done");
    return sensorChannel;
}

sptr<SensorBasicDataChannel> ClientInfo::GetSensorChannelByPid(int32_t pid)
{
    SEN_HILOGD("In, pid:%{public}d", pid);
    if (pid == INVALID_PID) {
        SEN_HILOGE("pid is invalid");
        return nullptr;
    }
    std::lock_guard<std::mutex> channelLock(channelMutex_);
    auto channelIt = channelMap_.find(pid);
    if (channelIt == channelMap_.end()) {
        SEN_HILOGE("There is no channel belong to the pid");
        return nullptr;
    }
    SEN_HILOGI("Done, pid:%{public}d", pid);
    return channelIt->second;
}

std::vector<sptr<SensorBasicDataChannel>> ClientInfo::GetSensorChannel(const SensorDescription &sensorDesc)
{
    if (sensorDesc.sensorType == INVALID_SENSOR_ID) {
        SEN_HILOGE("sensorType is invalid");
        return {};
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    auto clientIt = clientMap_.find(sensorDesc);
    if (clientIt == clientMap_.end()) {
        SEN_HILOGD("There is no channel belong to sensor,"
            "deviceIndex:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
            sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
        return {};
    }
    std::vector<sptr<SensorBasicDataChannel>> sensorChannel;
    for (const auto &sensorInfoIt : clientIt->second) {
        std::lock_guard<std::mutex> channelLock(channelMutex_);
        auto channelIt = channelMap_.find(sensorInfoIt.first);
        if (channelIt == channelMap_.end()) {
            continue;
        }
        if (!sensorInfoIt.second.GetPermState()) {
            continue;
        }
        sensorChannel.push_back(channelIt->second);
    }
    return sensorChannel;
}

bool ClientInfo::UpdateSensorInfo(const SensorDescription &sensorDesc, int32_t pid, const SensorBasicInfo &sensorInfo)
{
    SEN_HILOGD("In, sensorType:%{public}d, pid:%{public}d", sensorDesc.sensorType, pid);
    if ((sensorDesc.sensorType == INVALID_SENSOR_ID) || (pid <= INVALID_PID) || (!sensorInfo.GetSensorState())) {
        SEN_HILOGE("Params are invalid");
        return false;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    auto it = clientMap_.find(sensorDesc);
    if (it == clientMap_.end()) {
        std::unordered_map<int32_t, SensorBasicInfo> pidMap;
        auto pidRet = pidMap.insert(std::make_pair(pid, sensorInfo));
        auto clientRet = clientMap_.insert(std::make_pair(sensorDesc, pidMap));
        return pidRet.second && clientRet.second;
    }
    auto pidIt = it->second.find(pid);
    if (pidIt == it->second.end()) {
        auto ret = it->second.insert(std::make_pair(pid, sensorInfo));
        return ret.second;
    }
    it->second[pid] = sensorInfo;
    SEN_HILOGI("Done, sensorType:%{public}d, pid:%{public}d", sensorDesc.sensorType, pid);
    return true;
}

void ClientInfo::RemoveSubscriber(const SensorDescription &sensorDesc, uint32_t pid)
{
    SEN_HILOGD("In, sensorTypeId:%{public}d, pid:%{public}u", sensorDesc.sensorType, pid);
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    auto it = clientMap_.find(sensorDesc);
    if (it == clientMap_.end()) {
        SEN_HILOGW("sensorDesc not exist");
        return;
    }
    auto pidIt = it->second.find(pid);
    if (pidIt != it->second.end()) {
        it->second.erase(pidIt);
    }
    SEN_HILOGI("Done, sensorType:%{public}d, pid:%{public}u", sensorDesc.sensorType, pid);
}

bool ClientInfo::UpdateSensorChannel(int32_t pid, const sptr<SensorBasicDataChannel> &channel)
{
    SEN_HILOGD("In, pid:%{public}d", pid);
    CHKPR(channel, false);
    if (pid <= INVALID_PID) {
        SEN_HILOGE("pid is invalid");
        return false;
    }
    std::lock_guard<std::mutex> channelLock(channelMutex_);
    auto it = channelMap_.find(pid);
    if (it == channelMap_.end()) {
        if (channelMap_.size() == MAX_SUPPORT_CHANNEL) {
            SEN_HILOGE("Max support channel size:%{public}d", MAX_SUPPORT_CHANNEL);
            return false;
        }
        auto ret = channelMap_.insert(std::make_pair(pid, channel));
        SEN_HILOGD("ret.second:%{public}d", ret.second);
        return ret.second;
    }
    channelMap_[pid] = channel;
    SEN_HILOGI("Done, pid:%{public}d", pid);
    return true;
}

void ClientInfo::ClearSensorInfo(const SensorDescription &sensorDesc)
{
    SEN_HILOGD("In, deviceIndex:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    if (sensorDesc.sensorType == INVALID_SENSOR_ID) {
        SEN_HILOGE("sensorType is invalid");
        return;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    auto it = clientMap_.find(sensorDesc);
    if (it == clientMap_.end()) {
        SEN_HILOGD("sensorDesc not exist, no need to clear it");
        return;
    }
    clientMap_.erase(it);
    SEN_HILOGI("Done, sensorType:%{public}d", sensorDesc.sensorType);
}

void ClientInfo::ClearCurPidSensorInfo(const SensorDescription &sensorDesc, int32_t pid)
{
    SEN_HILOGD("In, sensorType:%{public}d, pid:%{public}d", sensorDesc.sensorType, pid);
    if ((sensorDesc.sensorType == INVALID_SENSOR_ID) || (pid <= INVALID_PID)) {
        SEN_HILOGE("sensorType or pid is invalid");
        return;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    auto it = clientMap_.find(sensorDesc);
    if (it == clientMap_.end()) {
        SEN_HILOGD("sensorDesc not exist, no need to clear it");
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
    SEN_HILOGI("Done, sensorType:%{public}d, pid:%{public}d", sensorDesc.sensorType, pid);
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
        SEN_HILOGD("There is no channel belong to pid, no need to destroy");
        return true;
    }
    it = channelMap_.erase(it);
    return true;
}

SensorBasicInfo ClientInfo::GetCurPidSensorInfo(const SensorDescription &sensorDesc, int32_t pid)
{
    int64_t minSamplingPeriodNs = LLONG_MAX;
    int64_t minReportDelayNs = LLONG_MAX;
    SensorBasicInfo sensorInfo;
    sensorInfo.SetSamplingPeriodNs(minSamplingPeriodNs);
    sensorInfo.SetMaxReportDelayNs(minReportDelayNs);
    if ((sensorDesc.sensorType == INVALID_SENSOR_ID) || (pid <= INVALID_PID)) {
        SEN_HILOGE("sensorType or channel is invalid");
        return sensorInfo;
    }
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    auto it = clientMap_.find(sensorDesc);
    if (it == clientMap_.end()) {
        SEN_HILOGE("Can't find deviceIndex:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
            sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
        return sensorInfo;
    }
    auto pidIt = it->second.find(pid);
    if (pidIt == it->second.end()) {
        SEN_HILOGE("Can't find pid:%{public}d", pid);
        return sensorInfo;
    }
    sensorInfo.SetSamplingPeriodNs(pidIt->second.GetSamplingPeriodNs());
    sensorInfo.SetMaxReportDelayNs(pidIt->second.GetMaxReportDelayNs());
    return sensorInfo;
}

uint64_t ClientInfo::ComputeBestPeriodCount(const SensorDescription &sensorDesc, sptr<SensorBasicDataChannel> &channel)
{
    if (sensorDesc.sensorType == INVALID_SENSOR_ID || channel == nullptr) {
        SEN_HILOGE("sensorType is invalid or channel cannot be null");
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
    int64_t bestSamplingPeriod = GetBestSensorInfo(sensorDesc).GetSamplingPeriodNs();
    int64_t curSamplingPeriod = GetCurPidSensorInfo(sensorDesc, pid).GetSamplingPeriodNs();
    if (bestSamplingPeriod == 0L) {
        SEN_HILOGE("Best sensor sampling period is 0");
        return 0UL;
    }
    int64_t ret = curSamplingPeriod / bestSamplingPeriod;
    return (ret <= 0L) ? 0UL : ret;
}

uint64_t ClientInfo::ComputeBestFifoCount(const SensorDescription &sensorDesc, sptr<SensorBasicDataChannel> &channel)
{
    if (channel == nullptr || sensorDesc.sensorType == INVALID_SENSOR_ID) {
        SEN_HILOGE("sensorType is invalid or channel cannot be null");
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
    int64_t curReportDelay = GetCurPidSensorInfo(sensorDesc, pid).GetMaxReportDelayNs();
    int64_t curSamplingPeriod = GetCurPidSensorInfo(sensorDesc, pid).GetSamplingPeriodNs();
    if (curSamplingPeriod == 0L) {
        SEN_HILOGE("Best sensor fifo count is 0");
        return 0UL;
    }
    int64_t ret = curReportDelay / curSamplingPeriod;
    return (ret <= 0L) ? 0UL : ret;
}

int32_t ClientInfo::GetStoreEvent(const SensorDescription &sensorDesc, SensorData &data)
{
    std::lock_guard<std::mutex> lock(eventMutex_);
    auto storedEvent = storedEvent_.find(sensorDesc);
    if (storedEvent != storedEvent_.end()) {
        errno_t ret = memcpy_s(&data, sizeof(data), &storedEvent->second, sizeof(storedEvent->second));
        if (ret != EOK) {
            SEN_HILOGE("memcpy_s failed, deviceIndex:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
                sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
            return ret;
        }
        return ERR_OK;
    }

    SEN_HILOGE("Can't get store event, deviceIndex:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    return NO_STORE_EVENT;
}

void ClientInfo::StoreEvent(const SensorData &data)
{
    bool foundSensor = false;
    SensorData storedEvent;
    std::vector<Sensor> sensors;
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
    auto sensorHdiConnection = &SensorHdiConnection::GetInstance();
    if (sensorHdiConnection == nullptr) {
        SEN_HILOGE("sensorHdiConnection cannot be null");
        return;
    }
    int32_t ret = sensorHdiConnection->GetSensorList(sensors);
    if (ret != 0) {
        SEN_HILOGE("GetSensorList is failed");
        return;
    }
#endif // HDF_DRIVERS_INTERFACE_SENSOR
    errno_t retVal = memcpy_s(&storedEvent, sizeof(storedEvent), &data, sizeof(data));
    if (retVal != EOK) {
        SEN_HILOGE("memcpy_s is failed");
        return;
    }
    for (size_t i = 0; i < sensors.size(); i++) {
        if (sensors[i].GetSensorTypeId() == storedEvent.sensorTypeId &&
            sensors[i].GetDeviceId() == storedEvent.deviceId && sensors[i].GetSensorId() == storedEvent.sensorId) {
            foundSensor = true;
            break;
        }
    }

    if (foundSensor) {
        std::lock_guard<std::mutex> lock(eventMutex_);
        storedEvent_[{storedEvent.deviceId, storedEvent.sensorTypeId, storedEvent.sensorId,
            storedEvent.location}] = storedEvent;
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
        SEN_HILOGE("Cannot find client pid");
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
        SEN_HILOGE("Cannot find client pid");
        return;
    }
    clientPidMap_.erase(it);
}

void ClientInfo::ClearEvent()
{
    std::lock_guard<std::mutex> lock(eventMutex_);
    storedEvent_.clear();
}

std::vector<SensorDescription> ClientInfo::GetSensorIdByPid(int32_t pid)
{
    CALL_LOG_ENTER;
    std::vector<SensorDescription> sensorDescVec;
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    for (const auto &itClientMap : clientMap_) {
        auto it = itClientMap.second.find(pid);
        if (it != itClientMap.second.end()) {
            sensorDescVec.push_back(itClientMap.first);
        }
    }
    return sensorDescVec;
}

AppThreadInfo ClientInfo::GetAppInfoByChannel(const sptr<SensorBasicDataChannel> &channel)
{
    CALL_LOG_ENTER;
    AppThreadInfo appThreadInfo;
    if (channel == nullptr) {
        SEN_HILOGE("channel is nullptr");
        return appThreadInfo;
    }
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
            channel.SetDeviceId(sensorIt.first.deviceId);
            channel.SetSensorType(sensorIt.first.sensorType);
            channel.SetSensorId(sensorIt.first.sensorId);
            std::string packageName;
            SensorManager::GetInstance().GetPackageName(GetTokenIdByPid(pid), packageName);
            channel.SetPackageName(packageName);
            int64_t samplingPeriodNs = pidIt.second.GetSamplingPeriodNs();
            int64_t maxReportDelayNs = pidIt.second.GetMaxReportDelayNs();
            channel.SetSamplingPeriodNs(samplingPeriodNs);
            uint32_t fifoCount = (samplingPeriodNs == 0) ? 0 : (uint32_t)(maxReportDelayNs / samplingPeriodNs);
            channel.SetFifoCount(fifoCount);
            channel.SetCmdType(GetCmdList(sensorIt.first.sensorType, uid));
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

void ClientInfo::UpdateCmd(int32_t sensorType, int32_t uid, int32_t cmdType)
{
    std::lock_guard<std::mutex> cmdLock(cmdMutex_);
    auto cmdIt = cmdMap_.find(sensorType);
    if (cmdIt == cmdMap_.end()) {
        std::unordered_map<int32_t, std::vector<int32_t>> cmds;
        std::vector<int32_t> tmp;
        tmp.push_back(cmdType);
        cmds.insert(std::make_pair(uid, tmp));
        cmdMap_.insert(std::make_pair(sensorType, cmds));
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

std::vector<int32_t> ClientInfo::GetCmdList(int32_t sensorType, int32_t uid)
{
    std::lock_guard<std::mutex> cmdLock(cmdMutex_);
    auto cmdIt = cmdMap_.find(sensorType);
    if (cmdIt == cmdMap_.end()) {
        return {};
    }
    auto uidIt = cmdIt->second.find(uid);
    if (uidIt == cmdIt->second.end()) {
        return {};
    }
    return uidIt->second;
}

void ClientInfo::UpdateDataQueue(int32_t sensorType, SensorData &data)
{
    if (sensorType == SENSOR_TYPE_ID_HEART_RATE) {
        return;
    }
    std::lock_guard<std::mutex> queueLock(dataQueueMutex_);
    auto it = dumpQueue_.find({data.deviceId, data.sensorTypeId, data.sensorId, data.location});
    if (it == dumpQueue_.end()) {
        std::queue<SensorData> q;
        q.push(data);
        dumpQueue_.insert(std::pair<SensorDescription, std::queue<SensorData>>(
            {data.deviceId, data.sensorTypeId, data.sensorId, data.location}, q));
        return;
    }
    it->second.push(data);
    if (it->second.size() > MAX_DUMP_DATA_SIZE) {
        it->second.pop();
    }
}

std::unordered_map<SensorDescription, std::queue<SensorData>> ClientInfo::GetDumpQueue()
{
    return dumpQueue_;
}

void ClientInfo::ClearDataQueue(const SensorDescription &sensorDesc)
{
    std::lock_guard<std::mutex> queueLock(dataQueueMutex_);
    auto it = dumpQueue_.find(sensorDesc);
    if (it != dumpQueue_.end()) {
        dumpQueue_.erase(it);
    }
}

int32_t ClientInfo::AddActiveInfoCBPid(int32_t pid)
{
    std::lock_guard<std::mutex> activeInfoCBPidLock(activeInfoCBPidMutex_);
    auto pairRet = activeInfoCBPidSet_.insert(pid);
    if (!pairRet.second) {
        SEN_HILOGE("Pid is duplicated");
        return ERROR;
    }
    return ERR_OK;
}

int32_t ClientInfo::DelActiveInfoCBPid(int32_t pid)
{
    std::lock_guard<std::mutex> activeInfoCBPidLock(activeInfoCBPidMutex_);
    auto it = activeInfoCBPidSet_.find(pid);
    if (it == activeInfoCBPidSet_.end()) {
        SEN_HILOGE("Pid is not exists");
        return ERROR;
    }
    activeInfoCBPidSet_.erase(it);
    return ERR_OK;
}

std::vector<int32_t> ClientInfo::GetActiveInfoCBPid()
{
    std::vector<int32_t> activeInfoCBPids;
    std::lock_guard<std::mutex> activeInfoCBPidLock(activeInfoCBPidMutex_);
    for (auto it = activeInfoCBPidSet_.begin(); it != activeInfoCBPidSet_.end(); ++it) {
        activeInfoCBPids.push_back(*it);
    }
    return activeInfoCBPids;
}

bool ClientInfo::CallingService(int32_t pid)
{
    std::lock_guard<std::mutex> channelLock(channelMutex_);
    auto channelIt = channelMap_.find(pid);
    if (channelIt != channelMap_.end()) {
        return false;
    }
    SEN_HILOGD("Pid is not exists in channelMap");
    std::lock_guard<std::mutex> activeInfoCBPidLock(activeInfoCBPidMutex_);
    auto pidIt = activeInfoCBPidSet_.find(pid);
    if (pidIt != activeInfoCBPidSet_.end()) {
        return false;
    }
    SEN_HILOGD("Pid is not exists in activeInfoCBPidSet");
    return true;
}


int32_t ClientInfo::GetPidByTokenId(AccessTokenID tokenId)
{
    std::lock_guard<std::mutex> uidLock(uidMutex_);
    int32_t pid = INVALID_PID;
    auto iter = std::find_if(appThreadInfoMap_.begin(), appThreadInfoMap_.end(), [tokenId] (auto appThreadInfo) {
            return appThreadInfo.second.callerToken == tokenId;
        });
    if (iter != appThreadInfoMap_.end()) {
        pid = iter->second.pid;
    }
    return pid;
}

void ClientInfo::UpdatePermState(int32_t pid, int32_t sensorType, bool state)
{
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    for (auto it = clientMap_.begin(); it != clientMap_.end();) {
        if (it->first.sensorType != sensorType) {
            it++;
            continue;
        }
        auto clientInfo = it->second.find(pid);
        if (clientInfo != it->second.end()) {
            clientInfo->second.SetPermState(state);
        }
        it++;
    }
}

void ClientInfo::ChangeSensorPerm(AccessTokenID tokenId, const std::string &permName, bool state)
{
    int32_t pid = GetPidByTokenId(tokenId);
    if (pid <= INVALID_PID) {
        SEN_HILOGE("Invalid pid:%{public}d", pid);
        return;
    }
    auto it = userGrantPermMap_.find(permName);
    if (it == userGrantPermMap_.end()) {
        SEN_HILOGE("Invalid permission name:%{public}s", permName.c_str());
        return;
    }
    for (int32_t sensorType : it->second) {
        UpdatePermState(pid, sensorType, state);
    }
}

void ClientInfo::SetDeviceStatus(uint32_t deviceStatus)
{
    deviceStatus_ = deviceStatus;
}

uint32_t ClientInfo::GetDeviceStatus()
{
    return deviceStatus_;
}

void ClientInfo::SaveSensorClient(const sptr<IRemoteObject> &sensorClient)
{
    CALL_LOG_ENTER;
    CHKPV(sensorClient);
    std::lock_guard<std::mutex> lock(sensorClientMutex_);
    sensorClients_.push_back(sensorClient);
}

void ClientInfo::DestroySensorClient(const sptr<IRemoteObject> &sensorClient)
{
    CALL_LOG_ENTER;
    CHKPV(sensorClient);
    std::lock_guard<std::mutex> lock(sensorClientMutex_);
    auto it = std::find(sensorClients_.begin(), sensorClients_.end(), sensorClient);
    if (it != sensorClients_.end()) {
        sensorClients_.erase(it);
    }
}

void ClientInfo::SendMsgToClient(SensorPlugData info)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> lock(sensorClientMutex_);
    for (const auto &client : sensorClients_) {
        sptr<ISensorClient> clientProxy = iface_cast<ISensorClient>(client);
        if (clientProxy != nullptr) {
            clientProxy->ProcessPlugEvent(info);
        }
    }
}

bool ClientInfo::IsClientSubscribe()
{
    std::lock_guard<std::mutex> clientLock(clientMutex_);
    return !clientMap_.empty();
}
} // namespace Sensors
} // namespace OHOS
