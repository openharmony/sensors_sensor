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
#include "hdi_connection.h"

#include "sensor_interface_proxy.h"
#include "sensor_event_callback.h"
#include "sensors_errors.h"
#include "sensors_log_domain.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
using sensor::v1_0::ISensorInterface;
using sensor::v1_0::ISensorCallback;
using sensor::v1_0::HdfSensorInformation;
namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SensorsLogDomain::SENSOR_SERVICE, "HdiConnection" };
sptr<ISensorInterface> sensorInterface_ = nullptr;
sptr<ISensorCallback> eventCallback_ = new SensorEventCallback();
}

ZReportDataCb HdiConnection::reportDataCb_ = nullptr;
sptr<ReportDataCallback> HdiConnection::reportDataCallback_ = nullptr;

int32_t HdiConnection::ConnectHdi()
{
    sensorInterface_ = ISensorInterface::Get();
    if (sensorInterface_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s connect v1_0 hdi failed", __func__);
        return ERR_NO_INIT;
    }
    return ERR_OK;
}

int32_t HdiConnection::GetSensorList(std::vector<Sensor>& sensorList)
{
    HiLog::Info(LABEL, "%{public}s in", __func__);
    std::vector<HdfSensorInformation> sensorInfos;
    int32_t ret = sensorInterface_->GetAllSensorInfo(sensorInfos);
    if (ret != 0) {
        HiLog::Error(LABEL, "%{public}s get sensor list failed", __func__);
        return ret;
    }
    for (int32_t i = 0; i < static_cast<int32_t>(sensorInfos.size()); i++) {
        const std::string sensorName(sensorInfos[i].sensorName);
        const std::string vendorName(sensorInfos[i].vendorName);
        const int32_t sensorId = sensorInfos[i].sensorId;
        const float maxRange = sensorInfos[i].maxRange;
        Sensor sensor;
        sensor.SetSensorId(sensorId);
        sensor.SetMaxRange(maxRange);
        sensor.SetName(sensorName.c_str());
        sensor.SetVendor(vendorName.c_str());
        sensorList.push_back(sensor);
    }
    return ERR_OK;
}

int32_t HdiConnection::EnableSensor(uint32_t sensorId)
{
    int32_t ret = sensorInterface_->Enable(sensorId);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s is failed", __func__);
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnection::DisableSensor(uint32_t sensorId)
{
    int32_t ret = sensorInterface_->Disable(sensorId);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s is failed", __func__);
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnection::SetBatch(int32_t sensorId, int64_t samplingInterval, int64_t reportInterval)
{
    int32_t ret = sensorInterface_->SetBatch(sensorId, samplingInterval, reportInterval);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnection::SetMode(int32_t sensorId, int32_t mode)
{
    int32_t ret = sensorInterface_->SetMode(sensorId, mode);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s is failed", __func__);
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnection::SetOption(int32_t sensorId, uint32_t option)
{
    int32_t ret = sensorInterface_->SetOption(sensorId, option);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s is failed", __func__);
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnection::RegisteDataReport(ZReportDataCb cb, sptr<ReportDataCallback> reportDataCallback)
{
    if (reportDataCallback == nullptr) {
        HiLog::Error(LABEL, "%{public}s failed, reportDataCallback cannot be null", __func__);
        return ERR_NO_INIT;
    }
    int32_t ret = sensorInterface_->Register(0, eventCallback_);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        return ret;
    }
    reportDataCb_ = cb;
    reportDataCallback_ = reportDataCallback;
    return ERR_OK;
}

int32_t HdiConnection::DestroyHdiConnection()
{
    int32_t ret = sensorInterface_->Unregister(0, eventCallback_);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnection::RunCommand(uint32_t sensorId, int32_t cmd, int32_t params)
{
    return 0;
}

ZReportDataCb HdiConnection::getReportDataCb()
{
    if (reportDataCb_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s reportDataCb_ cannot be null", __func__);
    }
    return reportDataCb_;
}

sptr<ReportDataCallback> HdiConnection::getReportDataCallback()
{
    if (reportDataCallback_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s reportDataCallback_ cannot be null", __func__);
    }
    return reportDataCallback_;
}
}  // namespace Sensors
}  // namespace OHOS
