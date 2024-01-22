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
#include "sensor_errors.h"

#undef LOG_TAG
#define LOG_TAG "HdiConnection"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
std::unique_ptr<HdiConnection> HdiConnection_ = std::make_unique<HdiConnection>();
constexpr int32_t HEADPOSTURE_DATA_SIZE = 20;
constexpr int64_t LOG_INTERVAL = 60000000000;
enum {
    TWO_DIMENSION = 2,
    SEVEN_DIMENSION = 7,
    DEFAULT_DIMENSION = 16
};
} // namespace

int32_t SensorEventCallback::OnDataEvent(const HdfSensorEvents &event)
{
    ReportDataCb reportDataCb_ = HdiConnection_->GetReportDataCb();
    sptr<ReportDataCallback> reportDataCallback_ = HdiConnection_->GetReportDataCallback();
    CHKPR(reportDataCb_, ERR_NO_INIT);
    CHKPR(reportDataCallback_, ERR_NO_INIT);
    int32_t dataSize = static_cast<int32_t>(event.data.size());
    if (dataSize == 0) {
        SEN_HILOGI("Data is empty");
        return ERR_INVALID_VALUE;
    }
    SensorData sensorData = {
        .sensorTypeId = event.sensorId,
        .version = event.version,
        .timestamp = event.timestamp,
        .option = event.option,
        .mode = event.mode,
        .dataLen = event.dataLen
    };
    if (sensorData.sensorTypeId == SENSOR_TYPE_ID_PROXIMITY) {
        sensorData.mode = SENSOR_ON_CHANGE;
    }
    CHKPR(sensorData.data, ERR_NO_INIT);
    if (sensorData.sensorTypeId == SENSOR_TYPE_ID_HEADPOSTURE) {
        sensorData.dataLen = HEADPOSTURE_DATA_SIZE;
        const float *inputFloatPtr = reinterpret_cast<const float *>(event.data.data());
        float *outputFloatPtr = reinterpret_cast<float *>(sensorData.data);
        int32_t *outputIntPtr = reinterpret_cast<int32_t *>(sensorData.data);
        outputIntPtr[0] = static_cast<int32_t>(*(inputFloatPtr + 1));
        if (outputIntPtr[0] < 0) {
            SEN_HILOGE("The order of head posture sensor is invalid");
        }
        outputFloatPtr[1] = *(inputFloatPtr + 3);
        outputFloatPtr[2] = *(inputFloatPtr + 4);
        outputFloatPtr[3] = *(inputFloatPtr + 5);
        outputFloatPtr[4] = *(inputFloatPtr + 6);
    } else {
        for (int32_t i = 0; i < dataSize; i++) {
            sensorData.data[i] = event.data[i];
        }
    }
    ControlSensorPrint(sensorData);
    std::unique_lock<std::mutex> lk(ISensorHdiConnection::dataMutex_);
    (void)(reportDataCallback_->*(reportDataCb_))(&sensorData, reportDataCallback_);
    ISensorHdiConnection::dataCondition_.notify_one();
    return ERR_OK;
}

void SensorEventCallback::ControlSensorPrint(const SensorData &sensorData)
{
    if (sensorData.sensorTypeId == SENSOR_TYPE_ID_HALL_EXT) {
        PrintSensorData(sensorData);
    }
    if ((sensorData.sensorTypeId == SENSOR_TYPE_ID_POSTURE)
        && ((postureLastTs_ == 0) || (sensorData.timestamp - postureLastTs_ >= LOG_INTERVAL))) {
        PrintSensorData(sensorData);
        postureLastTs_ = sensorData.timestamp;
    }
}

void SensorEventCallback::PrintSensorData(const SensorData &sensorData)
{
    std::string str;
    str += "sensorId: " + std::to_string(sensorData.sensorTypeId) + "\n";
    str += "timestamp: " + std::to_string(sensorData.timestamp) + "\n";
    int32_t dataDim = GetDataDimension(sensorData.sensorTypeId);
    auto data = reinterpret_cast<const float *>(sensorData.data);
    for (int32_t i = 0; i < dataDim; ++i) {
        str.append(std::to_string(*data));
        if (i != dataDim - 1) {
            str.append(", ");
        }
        ++data;
    }
    str.append("\n");
    SEN_HILOGI("SensorData: %{public}s", str.c_str());
}

int32_t SensorEventCallback::GetDataDimension(int32_t sensorId)
{
    switch (sensorId) {
        case SENSOR_TYPE_ID_HALL_EXT:
            return TWO_DIMENSION;
        case SENSOR_TYPE_ID_POSTURE:
            return SEVEN_DIMENSION;
        default:
            SEN_HILOGW("Unknown sensorId:%{public}d, size:%{public}d", sensorId, DEFAULT_DIMENSION);
            return DEFAULT_DIMENSION;
    }
}
} // namespace Sensors
} // namespace OHOS