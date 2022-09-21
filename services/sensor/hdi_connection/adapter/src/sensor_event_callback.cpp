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

#include "sensor_event_callback.h"

#include "hdi_connection.h"
#include "sensor_agent_type.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "HdiConnection" };
std::unique_ptr<HdiConnection> HdiConnection_ = std::make_unique<HdiConnection>();
}
int32_t SensorEventCallback::OnDataEvent(const HdfSensorEvents& event)
{
    ReportDataCb reportDataCb_ = HdiConnection_->GetReportDataCb();
    sptr<ReportDataCallback> reportDataCallback_ = HdiConnection_->GetReportDataCallback();
    CHKPR(reportDataCb_, ERR_NO_INIT);
    CHKPR(reportDataCallback_, ERR_NO_INIT);
    int32_t dataSize = static_cast<int32_t>(event.data.size());
    if (dataSize == 0) {
        SEN_HILOGI("data is empty");
        return ERR_INVALID_VALUE;
    }
    SensorEvent sensorEvent = {
        .sensorTypeId = event.sensorId,
        .version = event.version,
        .timestamp = event.timestamp,
        .option = event.option,
        .mode = event.mode,
        .dataLen = event.dataLen
    };
    sensorEvent.data = new (std::nothrow) uint8_t[SENSOR_DATA_LENGHT];
    CHKPR(sensorEvent.data, ERR_NO_INIT);
    for (int32_t i = 0; i < static_cast<int32_t>(dataSize); i++) {
        sensorEvent.data[i] = event.data[i];
    }
    std::unique_lock<std::mutex> lk(ISensorHdiConnection::dataMutex_);
    (void)(reportDataCallback_->*(reportDataCb_))(&sensorEvent, reportDataCallback_);
    ISensorHdiConnection::dataCondition_.notify_one();
    return ERR_OK;
}
}  // namespace Sensors
}  // namespace OHOS