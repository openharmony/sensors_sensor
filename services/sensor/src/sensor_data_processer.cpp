/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "sensor_data_processer.h"

#include <sys/socket.h>
#include <thread>

#include "permission_util.h"
#include "securec.h"
#include "sensor_basic_data_channel.h"
#include "sensor_catalog.h"
#include "sensors_errors.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SensorsLogDomain::SENSOR_UTILS, "SensorDataProcesser" };

enum {
    FIRST_INDEX = 1,
    SECOND_INDEX = 2,
    THIRD_INDEX = 3,
};

constexpr uint32_t SENSOR_INDEX_SHIFT = 8;
constexpr uint32_t SENSOR_TYPE_SHIFT = 16;
constexpr uint32_t SENSOR_CATAGORY_SHIFT = 24;

constexpr uint32_t FLUSH_COMPLETE_ID = ((uint32_t)OTHER << SENSOR_CATAGORY_SHIFT) |
                                       ((uint32_t)SENSOR_TYPE_FLUSH << SENSOR_TYPE_SHIFT) |
                                       ((uint32_t)FIRST_INDEX << SENSOR_INDEX_SHIFT);
}  // namespace

SensorDataProcesser::SensorDataProcesser(const std::unordered_map<uint32_t, Sensor> &sensorMap)
{
    sensorMap_.insert(sensorMap.begin(), sensorMap.end());
    HiLog::Debug(LABEL, "%{public}s sensorMap_.size : %{public}d", __func__, int32_t { sensorMap_.size() });
}

SensorDataProcesser::~SensorDataProcesser()
{
    dataCountMap_.clear();
    sensorMap_.clear();
}

void SensorDataProcesser::SendNoneFifoCacheData(std::unordered_map<uint32_t, struct SensorEvent> &cacheBuf,
                                                sptr<SensorBasicDataChannel> &channel, struct SensorEvent &event,
                                                uint64_t periodCount)
{
    std::vector<struct SensorEvent> sendEvents;
    std::lock_guard<std::mutex> dataCountLock(dataCountMutex_);
    sendEvents.push_back(event);
    uint32_t sensorId = static_cast<uint32_t>(event.sensorTypeId);
    if (sensorId == FLUSH_COMPLETE_ID) {
        sensorId = static_cast<uint32_t>(event.sensorTypeId);
    }
    auto dataCountIt = dataCountMap_.find(sensorId);
    if (dataCountIt == dataCountMap_.end()) {
        std::vector<sptr<FifoCacheData>> channelFifoList;
        sptr<FifoCacheData> fifoCacheData = new (std::nothrow) FifoCacheData();
        if (fifoCacheData == nullptr) {
            HiLog::Error(LABEL, "%{public}s fifoCacheData cannot be null", __func__);
            return;
        }
        fifoCacheData->SetChannel(channel);
        channelFifoList.push_back(fifoCacheData);
        dataCountMap_.insert(std::make_pair(sensorId, channelFifoList));
        SendRawData(cacheBuf, channel, sendEvents);
        return;
    }
    bool channelExist = false;
    for (const auto &fifoCacheData : dataCountIt->second) {
        if (fifoCacheData->GetChannel() != channel) {
            continue;
        }
        channelExist = true;
        uint64_t curCount = fifoCacheData->GetPeriodCount();
        curCount++;
        fifoCacheData->SetPeriodCount(curCount);
        if (periodCount != 0 && fifoCacheData->GetPeriodCount() % periodCount != 0UL) {
            continue;
        }
        SendRawData(cacheBuf, channel, sendEvents);
        fifoCacheData->SetPeriodCount(0);
        return;
    }
    if (!channelExist) {
        sptr<FifoCacheData> fifoCacheData = new (std::nothrow) FifoCacheData();
        if (fifoCacheData == nullptr) {
            HiLog::Error(LABEL, "%{public}s failed, fifoCacheData cannot be null", __func__);
            return;
        }
        fifoCacheData->SetChannel(channel);
        dataCountIt->second.push_back(fifoCacheData);
        SendRawData(cacheBuf, channel, sendEvents);
    }
}

void SensorDataProcesser::SendFifoCacheData(std::unordered_map<uint32_t, struct SensorEvent> &cacheBuf,
                                            sptr<SensorBasicDataChannel> &channel, struct SensorEvent &event,
                                            uint64_t periodCount, uint64_t fifoCount)
{
    uint32_t sensorId = static_cast<uint32_t>(event.sensorTypeId);
    if (sensorId == FLUSH_COMPLETE_ID) {
        sensorId = static_cast<uint32_t>(event.sensorTypeId);
    }
    std::lock_guard<std::mutex> dataCountLock(dataCountMutex_);
    auto dataCountIt = dataCountMap_.find(sensorId);
    // there is no channelFifoList
    if (dataCountIt == dataCountMap_.end()) {
        std::vector<sptr<FifoCacheData>> channelFifoList;
        sptr<FifoCacheData> fifoCacheData = new (std::nothrow) FifoCacheData();
        if (fifoCacheData == nullptr) {
            HiLog::Error(LABEL, "%{public}s fifoCacheData cannot be null", __func__);
            return;
        }
        fifoCacheData->SetChannel(channel);
        channelFifoList.push_back(fifoCacheData);
        dataCountMap_.insert(std::make_pair(sensorId, channelFifoList));
        return;
    }
    // find channel in channelFifoList
    bool channelExist = false;
    for (auto &fifoData : dataCountIt->second) {
        if (fifoData->GetChannel() != channel) {
            continue;
        }
        channelExist = true;
        uint64_t curCount = fifoData->GetPeriodCount();
        curCount++;
        fifoData->SetPeriodCount(curCount);
        if (fifoData->GetPeriodCount() % periodCount != 0UL) {
            continue;
        }
        fifoData->SetPeriodCount(0);
        std::vector<struct SensorEvent> fifoDataList = fifoData->GetFifoCacheData();
        fifoDataList.push_back(event);
        fifoData->SetFifoCacheData(fifoDataList);
        if ((fifoData->GetFifoCacheData()).size() != fifoCount) {
            continue;
        }
        SendRawData(cacheBuf, channel, fifoData->GetFifoCacheData());
        fifoData->InitFifoCache();
        return;
    }
    // cannot find channel in channelFifoList
    if (!channelExist) {
        sptr<FifoCacheData> fifoCacheData = new (std::nothrow) FifoCacheData();
        if (fifoCacheData == nullptr) {
            HiLog::Error(LABEL, "%{public}s failed, fifoCacheData cannot be null", __func__);
            return;
        }
        fifoCacheData->SetChannel(channel);
        dataCountIt->second.push_back(fifoCacheData);
    }
}

void SensorDataProcesser::ReportData(sptr<SensorBasicDataChannel> &channel, struct SensorEvent &event)
{
    if (channel == nullptr) {
        HiLog::Error(LABEL, "%{public}s channel cannot be null", __func__);
        return;
    }
    uint32_t sensorId = static_cast<uint32_t>(event.sensorTypeId);
    if (sensorId == FLUSH_COMPLETE_ID) {
        sensorId = static_cast<uint32_t>(event.sensorTypeId);
    }
    auto &cacheBuf = const_cast<std::unordered_map<uint32_t, struct SensorEvent> &>(channel->GetDataCacheBuf());
    if (ReportNotContinuousData(cacheBuf, channel, event)) {
        return;
    }
    int32_t periodCount = clientInfo_.ComputeBestPeriodCount(sensorId, channel);
    if (periodCount == 0UL) {
        return;
    }
    auto fifoCount = clientInfo_.ComputeBestFifoCount(sensorId, channel);
    if (fifoCount <= 0) {
        SendNoneFifoCacheData(cacheBuf, channel, event, periodCount);
        return;
    }
    SendFifoCacheData(cacheBuf, channel, event, periodCount, fifoCount);
}

bool SensorDataProcesser::ReportNotContinuousData(std::unordered_map<uint32_t, struct SensorEvent> &cacheBuf,
                                                  sptr<SensorBasicDataChannel> &channel, struct SensorEvent &event)
{
    uint32_t sensorId = static_cast<uint32_t>(event.sensorTypeId);
    if (sensorId == FLUSH_COMPLETE_ID) {
        sensorId = static_cast<uint32_t>(event.sensorTypeId);
    }
    std::lock_guard<std::mutex> sensorLock(sensorMutex_);
    auto sensor = sensorMap_.find(sensorId);
    sensor->second.SetFlags(event.mode);
    if (sensor == sensorMap_.end()) {
        HiLog::Error(LABEL, "%{public}s data's sensorId is not supported", __func__);
        return false;
    }
    if (((SENSOR_ON_CHANGE & sensor->second.GetFlags()) == SENSOR_ON_CHANGE) ||
        ((SENSOR_ONE_SHOT & sensor->second.GetFlags()) == SENSOR_ONE_SHOT)) {
        std::vector<struct SensorEvent> sendEvents;
        sendEvents.push_back(event);
        SendRawData(cacheBuf, channel, sendEvents);
        return true;
    }
    return false;
}

bool SensorDataProcesser::CheckSendDataPermission(sptr<SensorBasicDataChannel> channel, uint32_t sensorId)
{
    AppThreadInfo appThreadInfo = clientInfo_.GetAppInfoByChannel(channel);
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    return permissionUtil.CheckSensorPermission(appThreadInfo.callerToken, sensorId);
}

void SensorDataProcesser::SendRawData(std::unordered_map<uint32_t, struct SensorEvent> &cacheBuf,
                                      sptr<SensorBasicDataChannel> channel, std::vector<struct SensorEvent> event)
{
    if (channel == nullptr || event.empty()) {
        HiLog::Error(LABEL, "%{public}s channel cannot be null or event cannot be empty", __func__);
        return;
    }
    if (!CheckSendDataPermission(channel, event[0].sensorTypeId)) {
        HiLog::Error(LABEL, "%{public}s permission denied", __func__);
        return;
    }

    int32_t eventSize = static_cast<int32_t>(event.size());
    std::vector<struct TransferSensorEvents> transferEvents;
    for (int32_t i = 0; i < eventSize; i++) {
        struct TransferSensorEvents  transferEvent = {
            .sensorTypeId = event[i].sensorTypeId,
            .version = event[i].version,
            .timestamp = event[i].timestamp,
            .option = event[i].option,
            .mode = event[i].mode,
            .dataLen = event[i].dataLen
        };
        errno_t ret = memcpy_s(transferEvent.data, SENSOR_MAX_LENGTH, event[i].data, event[i].dataLen);
        if (ret != EOK) {
            HiLog::Error(LABEL, "%{public}s copy data failed", __func__);
            return;
        }
        transferEvents.push_back(transferEvent);
    }
    auto ret = channel->SendData(transferEvents.data(), eventSize * sizeof(struct TransferSensorEvents));
    if (ret != ERR_OK) {
        HiLog::Error(LABEL, "%{public}s send data failed, ret : %{public}d", __func__, ret);
        uint32_t sensorId = static_cast<uint32_t>(event[eventSize - 1].sensorTypeId);
        if (sensorId == FLUSH_COMPLETE_ID) {
            sensorId = static_cast<uint32_t>(event[eventSize - 1].sensorTypeId);
        }
        cacheBuf[sensorId] = event[eventSize - 1];
    }
}

int32_t SensorDataProcesser::CacheSensorEvent(const struct SensorEvent &event, sptr<SensorBasicDataChannel> &channel)
{
    if (channel == nullptr) {
        HiLog::Error(LABEL, "%{public}s channel cannot be null", __func__);
        return INVALID_POINTER;
    }
    int32_t ret = ERR_OK;
    auto &cacheBuf = const_cast<std::unordered_map<uint32_t, struct SensorEvent> &>(channel->GetDataCacheBuf());
    uint32_t sensorId = static_cast<uint32_t>(event.sensorTypeId);
    if (sensorId == FLUSH_COMPLETE_ID) {
        sensorId = static_cast<uint32_t>(event.sensorTypeId);
    }
    auto cacheEvent = cacheBuf.find(sensorId);
    if (cacheEvent != cacheBuf.end()) {
        // Try to send the last failed value, if it still fails, replace the previous cache directly
        ret = channel->SendData(&cacheEvent->second, sizeof(struct SensorEvent));
        if (ret != ERR_OK) {
            HiLog::Error(LABEL, "%{public}s ret : %{public}d", __func__, ret);
        }
        ret = channel->SendData(&event, sizeof(struct SensorEvent));
        if (ret != ERR_OK) {
            HiLog::Error(LABEL, "%{public}s ret : %{public}d", __func__, ret);
            cacheBuf[sensorId] = event;
        } else {
            cacheBuf.erase(cacheEvent);
        }
    } else {
        ret = channel->SendData(&event, sizeof(struct SensorEvent));
        if (ret != ERR_OK) {
            HiLog::Error(LABEL, "%{public}s ret : %{public}d", __func__, ret);
            cacheBuf[sensorId] = event;
        }
    }
    return ret;
}

void SensorDataProcesser::EventFilter(struct CircularEventBuf &eventsBuf)
{
    uint32_t realSensorId = 0;
    uint32_t sensorId = static_cast<uint32_t>(eventsBuf.circularBuf[eventsBuf.readPosition].sensorTypeId);
    std::vector<sptr<SensorBasicDataChannel>> channelList;
    if (sensorId == FLUSH_COMPLETE_ID) {
        realSensorId = static_cast<uint32_t>(eventsBuf.circularBuf[eventsBuf.readPosition].sensorTypeId);
        channelList = clientInfo_.GetSensorChannel(realSensorId);
    } else {
        channelList = clientInfo_.GetSensorChannel(sensorId);
    }
    auto flushInfo = flushInfo_.GetFlushInfo();
    std::vector<struct FlushInfo> flushVec;
    if (sensorId == FLUSH_COMPLETE_ID) {
        HiLog::Debug(LABEL, "%{public}s sensorId : %{public}u", __func__, sensorId);
        auto it = flushInfo.find(realSensorId);
        if (it != flushInfo.end()) {
            flushVec = it->second;
            for (auto &channel : flushVec) {
                if (flushInfo_.IsFlushChannelValid(channelList, channel.flushChannel)) {
                    SendEvents(channel.flushChannel, eventsBuf.circularBuf[eventsBuf.readPosition]);
                    flushInfo_.ClearFlushInfoItem(realSensorId);
                    break;
                } else {
                    // The channel that store in the flushVec has invalid, so erase this channel directly
                    HiLog::Debug(LABEL, "%{public}s clear flush info", __func__);
                    flushInfo_.ClearFlushInfoItem(realSensorId);
                }
            }
        }
    } else {
        for (auto &channel : channelList) {
            int32_t index = flushInfo_.GetFlushChannelIndex(flushVec, channel);
            if (index >= 0) {
                if (flushVec[index].flushFromEnable) {
                    HiLog::Info(LABEL, "%{public}s flushFromEnable", __func__);
                    continue;
                }
            }
            /* if has some suspend flush, but this flush come from the flush function rather than enable,
               so we need to calling GetSensorStatus to decided whether send this event. */
            if (channel->GetSensorStatus()) {
                SendEvents(channel, eventsBuf.circularBuf[eventsBuf.readPosition]);
            }
        }
    }
}

int32_t SensorDataProcesser::ProcessEvents(sptr<ReportDataCallback> dataCallback)
{
    if (dataCallback == nullptr) {
        HiLog::Error(LABEL, "%{public}s dataCallback cannot be null", __func__);
        return INVALID_POINTER;
    }
    std::unique_lock<std::mutex> lk(ISensorHdiConnection::dataMutex_);
    ISensorHdiConnection::dataCondition_.wait(lk);
    auto &eventsBuf = dataCallback->GetEventData();
    if (eventsBuf.eventNum <= 0) {
        HiLog::Error(LABEL, "%{public}s data cannot be empty", __func__);
        return NO_EVENT;
    }
    int32_t eventNum = eventsBuf.eventNum;
    for (int32_t i = 0; i < eventNum; i++) {
        EventFilter(eventsBuf);
        delete[] eventsBuf.circularBuf[eventsBuf.readPosition].data;
        eventsBuf.readPosition++;
        if (eventsBuf.readPosition == CIRCULAR_BUF_LEN) {
            eventsBuf.readPosition = 0;
        }
        eventsBuf.eventNum--;
    }
    return SUCCESS;
}

int32_t SensorDataProcesser::SendEvents(sptr<SensorBasicDataChannel> &channel, struct SensorEvent &event)
{
    if (channel == nullptr) {
        HiLog::Error(LABEL, "%{public}s channel cannot be null", __func__);
        return INVALID_POINTER;
    }
    clientInfo_.UpdateDataQueue(event.sensorTypeId, event);
    auto &cacheBuf = channel->GetDataCacheBuf();
    if (cacheBuf.empty()) {
        ReportData(channel, event);
    } else {
        CacheSensorEvent(event, channel);
    }
    clientInfo_.StoreEvent(event);
    return SUCCESS;
}

int32_t SensorDataProcesser::DataThread(sptr<SensorDataProcesser> dataProcesser, sptr<ReportDataCallback> dataCallback)
{
    CALL_LOG_ENTER;
    do {
        if (dataProcesser->ProcessEvents(dataCallback) == INVALID_POINTER) {
            HiLog::Error(LABEL, "%{public}s callback cannot be null", __func__);
            return INVALID_POINTER;
        }
    } while (1);
}
}  // namespace Sensors
}  // namespace OHOS
