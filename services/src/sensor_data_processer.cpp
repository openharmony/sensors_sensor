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

#include <cinttypes>
#include <sys/prctl.h>
#include <sys/socket.h>

#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
#include "hisysevent.h"
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
#include "motion_plugin.h"
#include "print_sensor_data.h"
#include "sensor_data_manager.h"
#include "sensor_shake_control_manager.h"

#undef LOG_TAG
#define LOG_TAG "SensorDataProcesser"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
const std::string SENSOR_REPORT_THREAD_NAME = "OS_SenProducer";
const std::set<int32_t> g_noNeedMotionTransform = {
    SENSOR_TYPE_ID_POSTURE, SENSOR_TYPE_ID_HALL, SENSOR_TYPE_ID_HALL_EXT,
    SENSOR_TYPE_ID_PROXIMITY, SENSOR_TYPE_ID_PROXIMITY1, SENSOR_TYPE_ID_AMBIENT_LIGHT
};
const std::set<int32_t> g_shakeSensorControlList = {
    SENSOR_TYPE_ID_ACCELEROMETER, SENSOR_TYPE_ID_MAGNETIC_FIELD, SENSOR_TYPE_ID_GYROSCOPE,
    SENSOR_TYPE_ID_GRAVITY, SENSOR_TYPE_ID_LINEAR_ACCELERATION, SENSOR_TYPE_ID_ROTATION_VECTOR,
    SENSOR_TYPE_ID_GAME_ROTATION_VECTOR, SENSOR_TYPE_ID_GYROSCOPE_UNCALIBRATED,
    SENSOR_TYPE_ID_GEOMAGNETIC_ROTATION_VECTOR
};
} // namespace

SensorDataProcesser::SensorDataProcesser(const std::unordered_map<SensorDescription, Sensor> &sensorMap)
{
    std::lock_guard<std::mutex> sensorLock(sensorMutex_);
    sensorMap_.insert(sensorMap.begin(), sensorMap.end());
    SEN_HILOGD("sensorMap_.size:%{public}d", int32_t { sensorMap_.size() });
}

SensorDataProcesser::~SensorDataProcesser()
{
    {
        std::lock_guard<std::mutex> dataCountLock(dataCountMutex_);
        dataCountMap_.clear();
    }
    std::lock_guard<std::mutex> sensorLock(sensorMutex_);
    sensorMap_.clear();
}

void SensorDataProcesser::UpdateSensorMap(const std::unordered_map<SensorDescription, Sensor> &sensorMap)
{
    std::lock_guard<std::mutex> sensorLock(sensorMutex_);
    sensorMap_.clear();
    sensorMap_.insert(sensorMap.begin(), sensorMap.end());
    SEN_HILOGD("sensorMap_.size:%{public}d", int32_t { sensorMap_.size() });
}

void SensorDataProcesser::SendNoneFifoCacheData(std::unordered_map<SensorDescription, SensorData> &cacheBuf,
                                                sptr<SensorBasicDataChannel> &channel, SensorData &data,
                                                uint64_t periodCount)
{
    std::vector<SensorData> sendEvents;
    std::lock_guard<std::mutex> dataCountLock(dataCountMutex_);
    sendEvents.push_back(data);
    auto dataCountIt = dataCountMap_.find({data.deviceId, data.sensorTypeId, data.sensorId, data.location});
    if (dataCountIt == dataCountMap_.end()) {
        std::vector<sptr<FifoCacheData>> channelFifoList;
        sptr<FifoCacheData> fifoCacheData = new (std::nothrow) FifoCacheData();
        CHKPV(fifoCacheData);
        fifoCacheData->SetChannel(channel);
        channelFifoList.push_back(fifoCacheData);
        dataCountMap_.insert(std::pair<SensorDescription, std::vector<sptr<FifoCacheData>>>(
            {data.deviceId, data.sensorTypeId, data.sensorId, data.location}, channelFifoList));
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

void SensorDataProcesser::SendFifoCacheData(std::unordered_map<SensorDescription, SensorData> &cacheBuf,
                                            sptr<SensorBasicDataChannel> &channel, SensorData &data,
                                            uint64_t periodCount, uint64_t fifoCount)
{
    std::lock_guard<std::mutex> dataCountLock(dataCountMutex_);
    auto dataCountIt = dataCountMap_.find({data.deviceId, data.sensorTypeId, data.sensorId, data.location});
    // there is no channelFifoList
    if (dataCountIt == dataCountMap_.end()) {
        std::vector<sptr<FifoCacheData>> channelFifoList;
        sptr<FifoCacheData> fifoCacheData = new (std::nothrow) FifoCacheData();
        CHKPV(fifoCacheData);
        fifoCacheData->SetChannel(channel);
        channelFifoList.push_back(fifoCacheData);
        dataCountMap_.insert(std::pair<SensorDescription, std::vector<sptr<FifoCacheData>>>(
            {data.deviceId, data.sensorTypeId, data.sensorId, data.location}, channelFifoList));
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
        std::vector<SensorData> fifoDataList = fifoData->GetFifoCacheData();
        fifoDataList.push_back(data);
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
        UpdataFifoDataChannel(channel, dataCountIt->second);
    }
}

void SensorDataProcesser::UpdataFifoDataChannel(sptr<SensorBasicDataChannel> &channel,
    std::vector<sptr<FifoCacheData>> &dataCount)
{
    sptr<FifoCacheData> fifoCacheData = new (std::nothrow) FifoCacheData();
    CHKPV(fifoCacheData);
    fifoCacheData->SetChannel(channel);
    dataCount.push_back(fifoCacheData);
}

void SensorDataProcesser::ReportData(sptr<SensorBasicDataChannel> &channel, SensorData &data)
{
    CHKPV(channel);
    int32_t sensorTypeId = data.sensorTypeId;
    if (sensorTypeId == SENSOR_TYPE_ID_HALL_EXT) {
        PrintSensorData::GetInstance().PrintSensorDataLog("ReportData", data);
    }
    auto &cacheBuf = const_cast<std::unordered_map<SensorDescription, SensorData> &>(channel->GetDataCacheBuf());
    if (ReportNotContinuousData(cacheBuf, channel, data)) {
        return;
    }
    uint64_t periodCount = clientInfo_.ComputeBestPeriodCount({data.deviceId, data.sensorTypeId, data.sensorId,
        data.location}, channel);
    if (periodCount == 0UL) {
        SEN_HILOGE("periodCount is zero");
        return;
    }
    auto fifoCount = clientInfo_.ComputeBestFifoCount({data.deviceId, data.sensorTypeId, data.sensorId,
        data.location}, channel);
    if (fifoCount <= 1) {
        SendNoneFifoCacheData(cacheBuf, channel, data, periodCount);
        return;
    }
    SendFifoCacheData(cacheBuf, channel, data, periodCount, fifoCount);
}

bool SensorDataProcesser::ReportNotContinuousData(std::unordered_map<SensorDescription, SensorData> &cacheBuf,
                                                  sptr<SensorBasicDataChannel> &channel, SensorData &data)
{
    int32_t sensorTypeId = data.sensorTypeId;
    std::lock_guard<std::mutex> sensorLock(sensorMutex_);
    auto sensor = sensorMap_.find({data.deviceId, data.sensorTypeId, data.sensorId, data.location});
    if (sensor == sensorMap_.end()) {
        SEN_HILOGE("Data's SensorDesc is not supported");
        return false;
    }
    sensor->second.SetFlags(data.mode);
    if (((SENSOR_ON_CHANGE & sensor->second.GetFlags()) == SENSOR_ON_CHANGE) ||
        ((SENSOR_ONE_SHOT & sensor->second.GetFlags()) == SENSOR_ONE_SHOT)) {
        std::vector<SensorData> sendEvents;
        sendEvents.push_back(data);
        if (sensorTypeId == SENSOR_TYPE_ID_HALL_EXT) {
            PrintSensorData::GetInstance().PrintSensorDataLog("ReportNotContinuousData", data);
        }
        SendRawData(cacheBuf, channel, sendEvents);
        return true;
    }
    return false;
}

void SensorDataProcesser::SendRawData(std::unordered_map<SensorDescription, SensorData> &cacheBuf,
                                      sptr<SensorBasicDataChannel> channel, std::vector<SensorData> events)
{
    CHKPV(channel);
    if (events.empty()) {
        return;
    }
    size_t eventSize = events.size();
    auto ret = channel->SendData(events.data(), eventSize * sizeof(SensorData));
    if (ret != ERR_OK) {
        SEN_HILOGE("Send data failed, ret:%{public}d, sensorTypeId:%{public}d, timestamp:%{public}" PRId64,
            ret, events[eventSize - 1].sensorTypeId, events[eventSize - 1].timestamp);
        cacheBuf[{events[eventSize - 1].deviceId, events[eventSize - 1].sensorTypeId,
            events[eventSize - 1].sensorId, events[eventSize - 1].location}] = events[eventSize - 1];
    }
}

int32_t SensorDataProcesser::CacheSensorEvent(const SensorData &data, sptr<SensorBasicDataChannel> &channel)
{
    CHKPR(channel, INVALID_POINTER);
    int32_t ret = ERR_OK;
    auto &cacheBuf = const_cast<std::unordered_map<SensorDescription, SensorData> &>(channel->GetDataCacheBuf());
    if (data.sensorTypeId == SENSOR_TYPE_ID_HALL_EXT) {
        PrintSensorData::GetInstance().PrintSensorDataLog("CacheSensorEvent", data);
    }
    auto cacheEvent = cacheBuf.find({data.deviceId, data.sensorTypeId, data.sensorId, data.location});
    if (cacheEvent != cacheBuf.end()) {
        // Try to send the last failed value, if it still fails, replace the previous cache directly
        const SensorData &cacheData = cacheEvent->second;
        ret = channel->SendData(&cacheData, sizeof(SensorData));
        if (ret != ERR_OK) {
            SEN_HILOGE("retry send cacheData failed, ret:%{public}d, sensorType:%{public}d, timestamp:%{public}" PRId64,
                ret, cacheData.sensorTypeId, cacheData.timestamp);
        }
        ret = channel->SendData(&data, sizeof(SensorData));
        if (ret != ERR_OK) {
            SEN_HILOGE("retry send data failed, ret:%{public}d, sensorType:%{public}d, timestamp:%{public}" PRId64,
                ret, data.sensorTypeId, data.timestamp);
            cacheBuf[{data.deviceId, data.sensorTypeId, data.sensorId, data.location}] = data;
        } else {
            cacheBuf.erase(cacheEvent);
        }
    } else {
        ret = channel->SendData(&data, sizeof(SensorData));
        if (ret != ERR_OK) {
            SEN_HILOGE("directly retry failed, ret:%{public}d, sensorType:%{public}d, timestamp:%{public}" PRId64,
                ret, data.sensorTypeId, data.timestamp);
            cacheBuf[{data.deviceId, data.sensorTypeId, data.sensorId, data.location}] = data;
        }
    }
    return ret;
}

void SensorDataProcesser::EventFilter(CircularEventBuf &eventsBuf)
{
    if (eventsBuf.circularBuf[eventsBuf.readPos].sensorTypeId == SENSOR_TYPE_ID_HALL_EXT) {
        PrintSensorData::GetInstance().PrintSensorDataLog("EventFilter", eventsBuf.circularBuf[eventsBuf.readPos]);
    }
    std::vector<sptr<SensorBasicDataChannel>> channelList = clientInfo_.GetSensorChannel({
        eventsBuf.circularBuf[eventsBuf.readPos].deviceId, eventsBuf.circularBuf[eventsBuf.readPos].sensorTypeId,
        eventsBuf.circularBuf[eventsBuf.readPos].sensorId, eventsBuf.circularBuf[eventsBuf.readPos].location});
    for (auto &channel : channelList) {
        if (channel == nullptr) {
            SEN_HILOGE("channel is null");
            continue;
        }
        if (!channel->GetSensorStatus()) {
            SEN_HILOGW("Sensor status is not active");
            continue;
        }
        SensorData sensorData = eventsBuf.circularBuf[eventsBuf.readPos];
#ifdef MSDP_MOTION_ENABLE
        if (g_noNeedMotionTransform.find(sensorData.sensorTypeId) == g_noNeedMotionTransform.end()) {
            MotionTransformIfRequired(channel->GetPackageName(), clientInfo_.GetDeviceStatus(), &sensorData);
            std::vector<std::string> appList = SENSOR_DATA_MGR->GetCompatibleAppStragegyList();
            if (std::find(appList.begin(), appList.end(), channel->GetPackageName()) != appList.end()) {
                MotionSensorRevision(clientInfo_.GetDeviceStatus(), &sensorData);
            }
        }
#endif // MSDP_MOTION_ENABLE
        if ((g_shakeSensorControlList.find(sensorData.sensorTypeId) != g_shakeSensorControlList.end())
            && (SENSOR_SHAKE_CONTROL_MGR->CheckAppIsNeedControl(channel->GetPackageName(), channel->GetAccessTokenId(),
            channel->GetUserId()))) {
            SEN_HILOGD("Shake the sensor data for control, bundleName:%{public}s", channel->GetPackageName().c_str());
            continue;
        }
        SendEvents(channel, sensorData);
    }
}

int32_t SensorDataProcesser::ProcessEvents(sptr<ReportDataCallback> dataCallback)
{
    CHKPR(dataCallback, INVALID_POINTER);
    std::unique_lock<std::mutex> lk(ISensorHdiConnection::dataMutex_);
    ISensorHdiConnection::dataCondition_.wait(lk, [this] { return ISensorHdiConnection::dataReady_.load(); });
    ISensorHdiConnection::dataReady_.store(false);
    auto &eventsBuf = dataCallback->GetEventData();
    if (eventsBuf.eventNum <= 0) {
        SEN_HILOGE("Data cannot be empty");
        return NO_EVENT;
    }
    int32_t eventNum = eventsBuf.eventNum;
    for (int32_t i = 0; i < eventNum; i++) {
        EventFilter(eventsBuf);
        eventsBuf.readPos++;
        if (eventsBuf.readPos >= CIRCULAR_BUF_LEN) {
            eventsBuf.readPos = 0;
        }
        eventsBuf.eventNum--;
    }
    return SUCCESS;
}

int32_t SensorDataProcesser::SendEvents(sptr<SensorBasicDataChannel> &channel, SensorData &data)
{
    CHKPR(channel, INVALID_POINTER);
    clientInfo_.UpdateDataQueue(data.sensorTypeId, data);
    auto &cacheBuf = channel->GetDataCacheBuf();
    if (cacheBuf.empty()) {
        ReportData(channel, data);
    } else {
        CacheSensorEvent(data, channel);
    }
    clientInfo_.StoreEvent(data);
    return SUCCESS;
}

int32_t SensorDataProcesser::DataThread(sptr<SensorDataProcesser> dataProcesser, sptr<ReportDataCallback> dataCallback)
{
    CALL_LOG_ENTER;
    prctl(PR_SET_NAME, SENSOR_REPORT_THREAD_NAME.c_str());
    do {
        if (dataProcesser == nullptr || dataCallback == nullptr) {
            SEN_HILOGE("dataProcesser or dataCallback is nullptr");
            return INVALID_POINTER;
        }
        if (dataProcesser->ProcessEvents(dataCallback) == INVALID_POINTER) {
            SEN_HILOGE("Callback cannot be null");
            return INVALID_POINTER;
        }
    } while (1);
}
} // namespace Sensors
} // namespace OHOS
