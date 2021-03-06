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

#include <cstdlib>
#include <cstring>

#include "errors.h"
#include "securec.h"
#include "sensors_errors.h"
#include "sensors_log_domain.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {
    LOG_CORE, SensorsLogDomain::SENSOR_UTILS, "ReportDataCallback"
};
}  // namespace
ReportDataCallback::ReportDataCallback()
{
    eventsBuf_.circularBuf = new struct SensorEvent[CIRCULAR_BUF_LEN];
    eventsBuf_.readPosition = 0;
    eventsBuf_.writePosition = 0;
    eventsBuf_.eventNum = 0;
}

ReportDataCallback::~ReportDataCallback()
{
    if (eventsBuf_.circularBuf != nullptr) {
        delete[] eventsBuf_.circularBuf;
    }
    eventsBuf_.circularBuf = nullptr;
    eventsBuf_.readPosition = 0;
    eventsBuf_.writePosition = 0;
    eventsBuf_.eventNum = 0;
}

int32_t ReportDataCallback::ZReportDataCallback(const struct SensorEvent* event, sptr<ReportDataCallback> cb)
{
    if (cb == nullptr || cb->eventsBuf_.circularBuf == nullptr || event == nullptr) {
        HiLog::Error(LABEL, "%{public}s callback or circularBuf or event cannot be null", __func__);
        return ERROR;
    }
    struct SensorEvent  eventCopy = {
        .sensorTypeId = event->sensorTypeId,
        .version = event->version,
        .timestamp = event->timestamp,
        .option = event->option,
        .mode = event->mode,
        .dataLen = event->dataLen
    };
    eventCopy.data = new uint8_t[SENSOR_DATA_LENGHT];
    HiLog::Info(LABEL, "%{public}s dataLength: %{public}d", __func__, event->dataLen);
    if (memcpy_s(eventCopy.data, event->dataLen, event->data, event->dataLen) != EOK) {
        HiLog::Error(LABEL, "%{public}s copy data failed", __func__);
        return COPY_ERR;
    }
    int32_t leftSize = CIRCULAR_BUF_LEN - cb->eventsBuf_.eventNum;
    int32_t toEndLen = CIRCULAR_BUF_LEN - cb->eventsBuf_.writePosition;
    if (toEndLen == 0) {
            cb->eventsBuf_.circularBuf[0] = eventCopy;
            cb->eventsBuf_.writePosition = 1 - toEndLen;
    } else {
            cb->eventsBuf_.circularBuf[cb->eventsBuf_.writePosition] = eventCopy;
            cb->eventsBuf_.writePosition += 1;
    }
    if (leftSize < 1) {
        cb->eventsBuf_.readPosition = cb->eventsBuf_.writePosition;
    }
    cb->eventsBuf_.eventNum += 1;
    if (cb->eventsBuf_.eventNum >= CIRCULAR_BUF_LEN) {
        cb->eventsBuf_.eventNum = CIRCULAR_BUF_LEN;
    }
    if (cb->eventsBuf_.writePosition == CIRCULAR_BUF_LEN) {
        cb->eventsBuf_.writePosition = 0;
    }
    return ERR_OK;
}

struct CircularEventBuf &ReportDataCallback::GetEventData()
{
    return eventsBuf_;
}
}  // namespace Sensors
}  // namespace OHOS
