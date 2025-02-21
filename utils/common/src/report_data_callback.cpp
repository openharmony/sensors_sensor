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

#include "report_data_callback.h"
#include "sensor_errors.h"

#undef LOG_TAG
#define LOG_TAG "ReportDataCallback"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
constexpr uint8_t EVENT_BLOCK_NUM = 64;
constexpr uint8_t BLOCK_EVENT_BUF_LEN = 16;
constexpr uint8_t RECENT_WRITE_BLOCK_NUM_SIZE = 5;
}
ReportDataCallback::ReportDataCallback()
{
    eventsBuf_.blockList.resize(EVENT_BLOCK_NUM);
    eventsBuf_.writeFullBlockNum = 0;
    recentWriteBlockNums_.resize(RECENT_WRITE_BLOCK_NUM_SIZE);
}

ReportDataCallback::~ReportDataCallback()
{
    for (auto& block : eventsBuf_.blockList) {
        if (block.dataBuf != nullptr) {
            delete[] block.dataBuf;
            block.dataBuf = nullptr;
        }
        block.eventNum = 0;
    }
    eventsBuf_.writeFullBlockNum = 0;
}

int32_t ReportDataCallback::ReportEventCallback(SensorData *sensorData, sptr<ReportDataCallback> cb)
{
    CHKPR(sensorData, ERROR);
    if (cb == nullptr) {
        SEN_HILOGE("Callback is null");
        return ERROR;
    }
    if (cb->eventsBuf_.writeFullBlockNum >= cb->eventsBuf_.blockList.size()) {
        SEN_HILOGE("Event buffer more than the blockList size");
        return ERROR;
    }
    auto& block = cb->eventsBuf_.blockList[cb->eventsBuf_.writeFullBlockNum];
    if (block.dataBuf == nullptr) {
        block.dataBuf = new(std::nothrow) SensorData[BLOCK_EVENT_BUF_LEN];
        if (block.dataBuf == nullptr) {
            SEN_HILOGE("New block buffer fail");
            return ERROR;
        }
        block.eventNum = 0;
    }
    if (block.eventNum < BLOCK_EVENT_BUF_LEN) {
        block.dataBuf[block.eventNum] = *sensorData;
        block.eventNum++;
    }
    if (block.eventNum >= BLOCK_EVENT_BUF_LEN) {
        cb->eventsBuf_.writeFullBlockNum++;
    }
    return ERR_OK;
}

void ReportDataCallback::GetEventData(std::vector<SensorData*> &events)
{
    int32_t writeBlockNum = 0;
    for (auto& block : eventsBuf_.blockList) {
        if (block.dataBuf == nullptr) {
            break;
        }
        if (block.eventNum <= 0) {
            SEN_HILOGE("Get eventNum fail");
            break;
        }
        writeBlockNum++;
        for (int32_t i = 0; i < block.eventNum; ++i) {
            events.push_back(&block.dataBuf[i]);
        }
        block.eventNum = 0;
    }
    eventsBuf_.writeFullBlockNum = 0;
    recentWriteBlockNums_[blockNumsUpdateIndex_] = writeBlockNum;
    blockNumsUpdateIndex_++;
    if (blockNumsUpdateIndex_ >= recentWriteBlockNums_.size()) {
        blockNumsUpdateIndex_ = 0;
    }
    if (!events.empty()) {
        FreeRedundantEventBuffer();
    }
}

void ReportDataCallback::FreeRedundantEventBuffer()
{
    int32_t maxWriteBlockNum = 0;
    for (auto num : recentWriteBlockNums_) {
        maxWriteBlockNum = std::max(maxWriteBlockNum, num);
    }

    // keep at least 1 block
    if (maxWriteBlockNum <= 0) {
        return;
    }

    for (int32_t index = maxWriteBlockNum; index < eventsBuf_.blockList.size(); ++index) {
        if (eventsBuf_.blockList[index].dataBuf == nullptr) {
            break;
        }
        delete[] eventsBuf_.blockList[index].dataBuf;
        eventsBuf_.blockList[index].dataBuf = nullptr;
    }
}
} // namespace Sensors
} // namespace OHOS
