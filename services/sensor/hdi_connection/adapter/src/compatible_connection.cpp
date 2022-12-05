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
#include "compatible_connection.h"

#include <cstring>

#include "securec.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "CompatibleConnection" };
}

ReportDataCb CompatibleConnection::reportDataCb_ = nullptr;
sptr<ReportDataCallback> CompatibleConnection::reportDataCallback_ = nullptr;
std::mutex ISensorHdiConnection::dataMutex_;
std::condition_variable ISensorHdiConnection::dataCondition_;
int32_t CompatibleConnection::ConnectHdi()
{
    SEN_HILOGI("connect hdi success");
    return ERR_OK;
}

int32_t CompatibleConnection::GetSensorList(std::vector<Sensor>& sensorList)
{
    std::vector<SensorInformation> sensorInfos;
    int32_t ret = hdiServiceImpl_.GetSensorList(sensorInfos);
    if (ret != 0) {
        SEN_HILOGE("get sensor list failed");
        return ret;
    }
    for (int32_t i = 0; i < static_cast<int32_t>(sensorInfos.size()); i++) {
        const std::string sensorName(sensorInfos[i].sensorName);
        const std::string vendorName(sensorInfos[i].vendorName);
        const std::string firmwareVersion(sensorInfos[i].firmwareVersion);
        const std::string hardwareVersion(sensorInfos[i].hardwareVersion);
        const int32_t sensorId = sensorInfos[i].sensorId;
        const float maxRange = sensorInfos[i].maxRange;
        Sensor sensor;
        sensor.SetSensorId(sensorId);
        sensor.SetSensorTypeId(sensorId);
        sensor.SetFirmwareVersion(firmwareVersion);
        sensor.SetHardwareVersion(hardwareVersion);
        sensor.SetMaxRange(maxRange);
        sensor.SetSensorName(sensorName);
        sensor.SetVendorName(vendorName);
        sensor.SetResolution(sensorInfos[i].accuracy);
        sensor.SetPower(sensorInfos[i].power);
        sensor.SetMinSamplePeriodNs(sensorInfos[i].minDelay);
        sensor.SetMaxSamplePeriodNs(sensorInfos[i].maxDelay);
        sensorList.push_back(sensor);
    }
    return ERR_OK;
}

int32_t CompatibleConnection::EnableSensor(int32_t sensorId)
{
    int32_t ret = hdiServiceImpl_.EnableSensor(sensorId);
    if (ret != 0) {
        SEN_HILOGE("enable sensor failed, sensorId:%{public}d", sensorId);
        return ret;
    }
    return ERR_OK;
};

int32_t CompatibleConnection::DisableSensor(int32_t sensorId)
{
    int32_t ret = hdiServiceImpl_.DisableSensor(sensorId);
    if (ret != 0) {
        SEN_HILOGE("disable sensor failed, sensorId:%{public}d", sensorId);
        return ret;
    }
    return ERR_OK;
}

int32_t CompatibleConnection::SetBatch(int32_t sensorId, int64_t samplingInterval, int64_t reportInterval)
{
    int32_t ret = hdiServiceImpl_.SetBatch(sensorId, samplingInterval, reportInterval);
    if (ret != 0) {
        SEN_HILOGE("set batch failed, sensorId:%{public}d", sensorId);
        return ret;
    }
    return ERR_OK;
}

int32_t CompatibleConnection::SetMode(int32_t sensorId, int32_t mode)
{
    int32_t ret = hdiServiceImpl_.SetMode(sensorId, mode);
    if (ret != 0) {
        SEN_HILOGI("set mode failed, sensorId:%{public}d", sensorId);
        return ret;
    }
    return ERR_OK;
}

int32_t CompatibleConnection::SensorDataCallback(const SensorEvents *event)
{
    CHKPR(event, ERR_INVALID_VALUE);
    if ((event->dataLen == 0)) {
        SEN_HILOGE("event is NULL");
        return ERR_INVALID_VALUE;
    }

    SensorEvent sensorEvent = {
        .sensorTypeId = event->sensorId,
        .version = event->version,
        .timestamp = event->timestamp,
        .option = event->option,
        .mode = event->mode,
        .dataLen = event->dataLen
    };
    sensorEvent.data = new (std::nothrow) uint8_t[SENSOR_DATA_LENGHT];
    CHKPR(sensorEvent.data, ERR_INVALID_VALUE);
    errno_t ret = memcpy_s(sensorEvent.data, event->dataLen, event->data, event->dataLen);
    if (ret != EOK) {
        SEN_HILOGE("copy data failed");
        delete[] sensorEvent.data;
        return COPY_ERR;
    }
    CHKPR(reportDataCallback_, ERR_NO_INIT);
    CHKPR(reportDataCb_, ERR_NO_INIT);
    std::unique_lock<std::mutex> lk(ISensorHdiConnection::dataMutex_);
    (void)(reportDataCallback_->*reportDataCb_)(&sensorEvent, reportDataCallback_);
    ISensorHdiConnection::dataCondition_.notify_one();
    return ERR_OK;
}

int32_t CompatibleConnection::RegisteDataReport(ReportDataCb cb, sptr<ReportDataCallback> reportDataCallback)
{
    CHKPR(reportDataCallback, ERR_INVALID_VALUE);
    int32_t ret = hdiServiceImpl_.Register(SensorDataCallback);
    if (ret != 0) {
        SEN_HILOGE("Register is failed");
        return ret;
    }
    reportDataCb_ = cb;
    reportDataCallback_ = reportDataCallback;
    return ERR_OK;
}

int32_t CompatibleConnection::DestroyHdiConnection()
{
    int32_t ret = hdiServiceImpl_.Unregister();
    if (ret != 0) {
        SEN_HILOGE("Unregister is failed");
        return ret;
    }
    return ERR_OK;
}
}  // namespace Sensors
}  // namespace OHOS