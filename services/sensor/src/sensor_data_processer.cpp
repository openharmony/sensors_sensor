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

#include "hisysevent.h"
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
constexpr HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "SensorDataProcesser" };

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
    SEN_HILOGD("sensorMap_.size:%{public}d", int32_t { sensorMap_.size() });
}

SensorDataProcesser::~SensorDataProcesser()
{
    dataCountMap_.clear();
    sensorMap_.clear();
}

void SensorDataProcesser::SendNoneFifoCacheData(std::unordered_map<uint32_t, SensorEvent> &cacheBuf,
                                                sptr<SensorBasicDataChannel> &channel, SensorEvent &event,
                                                uint64_t periodCount)
{
    std::vector<SensorEvent> sendEvents;
    std::lock_guard<std::mutex> dataCountLock(dataCountMutex_);
    sendEvents.push_back(event);
    uint32_t sensorId = static_cast<uint32_t>(event.sensorTypeId);
    auto dataCountIt = dataCountMap_.find(sensorId);
    if (dataCountIt == dataCountMap_.end()) {
        std::vector<sptr<FifoCacheData>> channelFifoList;
        sptr<FifoCacheData> fifoCacheData = new (std::nothrow) FifoCacheData();
        CHKPV(fifoCacheData);
        fifoCacheData->SetChannel(channel);
        channelFifoList.push_back(fifoCacheData);
        dataCountMap_.insert(std::make_pair(sensorId, channelFifoList));
        SendRawData(cacheBuf, channel, sendEvents);
        return;
    }
    bool channelExist = false;
    for (auto fifoIt = dataCountIt->second.begin(); fifoIt != dataCountIt->second.end();) {
        auto fifoCacheData = *fifoIt;
        CHKPC(fifoCacheData);
        auto fifoChannel = fifoCacheData->GetChannel();
        if (fifoChannel == nullptr) {
            fifoIt = dataCountIt->second.erase(fifoIt);
            continue;
        }
        ++fifoIt;
        if (fifoChannel != channel) {
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
        CHKPV(fifoCacheData);
        fifoCacheData->SetChannel(channel);
        dataCountIt->second.push_back(fifoCacheData);
        SendRawData(cacheBuf, channel, sendEvents);
    }
}

void SensorDataProcesser::SendFifoCacheData(std::unordered_map<uint32_t, SensorEvent> &cacheBuf,
                                            sptr<SensorBasicDataChannel> &channel, SensorEvent &event,
                                            uint64_t periodCount, uint64_t fifoCount)
{
    uint32_t sensorId = static_cast<uint32_t>(event.sensorTypeId);
    std::lock_guard<std::mutex> dataCountLock(dataCountMutex_);
    auto dataCountIt = dataCountMap_.find(sensorId);
    // there is no channelFifoList
    if (dataCountIt == dataCountMap_.end()) {
        std::vector<sptr<FifoCacheData>> channelFifoList;
        sptr<FifoCacheData> fifoCacheData = new (std::nothrow) FifoCacheData();
        CHKPV(fifoCacheData);
        fifoCacheData->SetChannel(channel);
        channelFifoList.push_back(fifoCacheData);
        dataCountMap_.insert(std::make_pair(sensorId, channelFifoList));
        return;
    }
    // find channel in channelFifoList
    bool channelExist = false;
    for (auto fifoIt = dataCountIt->second.begin(); fifoIt != dataCountIt->second.end();) {
        auto fifoData = *fifoIt;
        CHKPC(fifoData);
        auto fifoChannel = fifoData->GetChannel();
        if (fifoChannel == nullptr) {
            fifoIt = dataCountIt->second.erase(fifoIt);
            continue;
        }
        ++fifoIt;
        if (fifoChannel != channel) {
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
        std::vector<SensorEvent> fifoDataList = fifoData->GetFifoCacheData();
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
        CHKPV(fifoCacheData);
        fifoCacheData->SetChannel(channel);
        dataCountIt->second.push_back(fifoCacheData);
    }
}

void SensorDataProcesser::ReportData(sptr<SensorBasicDataChannel> &channel, SensorEvent &event)
{
    CHKPV(channel);
    uint32_t sensorId = static_cast<uint32_t>(event.sensorTypeId);
    if (sensorId == FLUSH_COMPLETE_ID) {
        sensorId = static_cast<uint32_t>(event.sensorTypeId);
    }
    auto &cacheBuf = const_cast<std::unordered_map<uint32_t, SensorEvent> &>(channel->GetDataCacheBuf());
    if (ReportNotContinuousData(cacheBuf, channel, event)) {
        return;
    }
    uint64_t periodCount = clientInfo_.ComputeBestPeriodCount(sensorId, channel);
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

bool SensorDataProcesser::ReportNotContinuousData(std::unordered_map<uint32_t, SensorEvent> &cacheBuf,
                                                  sptr<SensorBasicDataChannel> &channel, SensorEvent &event)
{
    uint32_t sensorId = static_cast<uint32_t>(event.sensorTypeId);
    if (sensorId == FLUSH_COMPLETE_ID) {
        sensorId = static_cast<uint32_t>(event.sensorTypeId);
    }
    std::lock_guard<std::mutex> sensorLock(sensorMutex_);
    auto sensor = sensorMap_.find(sensorId);
    if (sensor == sensorMap_.end()) {
        SEN_HILOGE("data's sensorId is not supported");
        return false;
    }
    sensor->second.SetFlags(event.mode);
    if (((SENSOR_ON_CHANGE & sensor->second.GetFlags()) == SENSOR_ON_CHANGE) ||
        ((SENSOR_ONE_SHOT & sensor->second.GetFlags()) == SENSOR_ONE_SHOT)) {
        std::vector<SensorEvent> sendEvents;
        sendEvents.push_back(event);
        SendRawData(cacheBuf, channel, sendEvents);
        return true;
    }
    return false;
}

void SensorDataProcesser::SendRawData(std::unordered_map<uint32_t, SensorEvent> &cacheBuf,
                                      sptr<SensorBasicDataChannel> channel, std::vector<SensorEvent> event)
{
    CHKPV(channel);
    if (event.empty()) {
        return;
    }
    size_t eventSize = event.size();
    std::vector<TransferSensorEvents> transferEvents;
    for (size_t i = 0; i < eventSize; i++) {
        TransferSensorEvents  transferEvent = {
            .sensorTypeId = event[i].sensorTypeId,
            .version = event[i].version,
            .timestamp = event[i].timestamp,
            .option = event[i].option,
            .mode = event[i].mode,
            .dataLen = event[i].dataLen
        };
        errno_t ret = memcpy_s(transferEvent.data, SENSOR_MAX_LENGTH, event[i].data, event[i].dataLen);
        if (ret != EOK) {
            SEN_HILOGE("copy data failed");
            return;
        }
        transferEvents.push_back(transferEvent);
    }
    auto ret = channel->SendData(transferEvents.data(), eventSize * sizeof(TransferSensorEvents));
    if (ret != ERR_OK) {
        SEN_HILOGE("send data failed, ret:%{public}d", ret);
        uint32_t sensorId = static_cast<uint32_t>(event[eventSize - 1].sensorTypeId);
        if (sensorId == FLUSH_COMPLETE_ID) {
            sensorId = static_cast<uint32_t>(event[eventSize - 1].sensorTypeId);
        }
        cacheBuf[sensorId] = event[eventSize - 1];
    }
}

int32_t SensorDataProcesser::CacheSensorEvent(const SensorEvent &event, sptr<SensorBasicDataChannel> &channel)
{
    CHKPR(channel, INVALID_POINTER);
    int32_t ret = ERR_OK;
    auto &cacheBuf = const_cast<std::unordered_map<uint32_t, SensorEvent> &>(channel->GetDataCacheBuf());
    uint32_t sensorId = static_cast<uint32_t>(event.sensorTypeId);
    if (sensorId == FLUSH_COMPLETE_ID) {
        sensorId = static_cast<uint32_t>(event.sensorTypeId);
    }
    auto cacheEvent = cacheBuf.find(sensorId);
    if (cacheEvent != cacheBuf.end()) {
        // Try to send the last failed value, if it still fails, replace the previous cache directly
        ret = channel->SendData(&cacheEvent->second, sizeof(SensorEvent));
        if (ret != ERR_OK) {
            SEN_HILOGE("ret:%{public}d", ret);
        }
        ret = channel->SendData(&event, sizeof(SensorEvent));
        if (ret != ERR_OK) {
            SEN_HILOGE("ret:%{public}d", ret);
            cacheBuf[sensorId] = event;
        } else {
            cacheBuf.erase(cacheEvent);
        }
    } else {
        ret = channel->SendData(&event, sizeof(SensorEvent));
        if (ret != ERR_OK) {
            SEN_HILOGE("ret:%{public}d", ret);
            cacheBuf[sensorId] = event;
        }
    }
    return ret;
}

void SensorDataProcesser::EventFilter(CircularEventBuf &eventsBuf)
{
    uint32_t realSensorId = 0;
    uint32_t sensorId = static_cast<uint32_t>(eventsBuf.circularBuf[eventsBuf.readPos].sensorTypeId);
    std::vector<sptr<SensorBasicDataChannel>> channelList;
    if (sensorId == FLUSH_COMPLETE_ID) {
        realSensorId = static_cast<uint32_t>(eventsBuf.circularBuf[eventsBuf.readPos].sensorTypeId);
        channelList = clientInfo_.GetSensorChannel(realSensorId);
    } else {
        channelList = clientInfo_.GetSensorChannel(sensorId);
    }
    auto flushInfo = flushInfo_.GetFlushInfo();
    std::vector<FlushInfo> flushVec;
    if (sensorId == FLUSH_COMPLETE_ID) {
        SEN_HILOGD("sensorId:%{public}u", sensorId);
        auto it = flushInfo.find(realSensorId);
        if (it != flushInfo.end()) {
            flushVec = it->second;
            for (auto &channel : flushVec) {
                auto flushChannel = channel.flushChannel.promote();
                if (flushInfo_.IsFlushChannelValid(channelList, flushChannel)) {
                    SendEvents(flushChannel, eventsBuf.circularBuf[eventsBuf.readPos]);
                    flushInfo_.ClearFlushInfoItem(realSensorId);
                    break;
                } else {
                    // The channel that store in the flushVec has invalid, so erase this channel directly
                    SEN_HILOGD("clear flush info");
                    flushInfo_.ClearFlushInfoItem(realSensorId);
                }
            }
        }
    } else {
        for (auto &channel : channelList) {
            int32_t index = flushInfo_.GetFlushChannelIndex(flushVec, channel);
            if (index >= 0) {
                if (flushVec[index].flushFromEnable) {
                    SEN_HILOGI("flushFromEnable");
                    continue;
                }
            }
            /* if has some suspend flush, but this flush come from the flush function rather than enable,
               so we need to calling GetSensorStatus to decided whether send this event. */
            if (channel->GetSensorStatus()) {
                SendEvents(channel, eventsBuf.circularBuf[eventsBuf.readPos]);
            }
        }
    }
}

int32_t SensorDataProcesser::ProcessEvents(sptr<ReportDataCallback> dataCallback)
{
    CHKPR(dataCallback, INVALID_POINTER);
    std::unique_lock<std::mutex> lk(ISensorHdiConnection::dataMutex_);
    ISensorHdiConnection::dataCondition_.wait(lk);
    auto &eventsBuf = dataCallback->GetEventData();
    if (eventsBuf.eventNum <= 0) {
        SEN_HILOGE("data cannot be empty");
        return NO_EVENT;
    }
    int32_t eventNum = eventsBuf.eventNum;
    for (int32_t i = 0; i < eventNum; i++) {
        EventFilter(eventsBuf);
        if (eventsBuf.circularBuf[eventsBuf.readPos].data != nullptr) {
            delete[] eventsBuf.circularBuf[eventsBuf.readPos].data;
            eventsBuf.circularBuf[eventsBuf.readPos].data = nullptr;
        }
        eventsBuf.readPos++;
        if (eventsBuf.readPos == CIRCULAR_BUF_LEN) {
            eventsBuf.readPos = 0;
        }
        eventsBuf.eventNum--;
    }
    return SUCCESS;
}

int32_t SensorDataProcesser::SendEvents(sptr<SensorBasicDataChannel> &channel, SensorEvent &event)
{
    CHKPR(channel, INVALID_POINTER);
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
            SEN_HILOGE("callback cannot be null");
            return INVALID_POINTER;
        }
    } while (1);
}
}  // namespace Sensors
}  // namespace OHOS
