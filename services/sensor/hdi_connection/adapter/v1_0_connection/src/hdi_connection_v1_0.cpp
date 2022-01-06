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
#include "hdi_connection_v1_0.h"

#include "sensor_interface_proxy.h"
#include "sensor_event_callback.h"
#include "sensors_errors.h"
#include "sensors_log_domain.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
using hdi::sensor::v1_0::ISensorInterface;
using hdi::sensor::v1_0::ISensorCallback;
using hdi::sensor::v1_0::HdfSensorInformation;
namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SensorsLogDomain::SENSOR_SERVICE, "HdiConnectionV1_0" };
sptr<ISensorInterface> sensorInterface_ = nullptr;
sptr<ISensorCallback> eventCallback_ = new SensorEventCallback();
}

ZReportDataCb HdiConnectionV1_0::reportDataCb_ = nullptr;
sptr<ReportDataCallback> HdiConnectionV1_0::reportDataCallback_ = nullptr;

int32_t HdiConnectionV1_0::ConnectHdi()
{
    sensorInterface_ = ISensorInterface::Get();
    if (sensorInterface_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s connect v1_0 hdi failed", __func__);
        return ERR_NO_INIT;
    }
    return ERR_OK;
}

int32_t HdiConnectionV1_0::GetSensorList(std::vector<Sensor>& sensorList)
{
    HiLog::Info(LABEL, "%{public}s in", __func__);
    std::vector<HdfSensorInformation> sensorInfos;
    int32_t ret = sensorInterface_->GetAllSensorInfo(sensorInfos);
    if (ret != 0) {
        HiLog::Error(LABEL, "%{public}s get sensor list failed", __func__);
        return ret;
    }
    HiLog::Info(LABEL, "%{public}s sensor size: %{public}d", __func__, sensorInfos.size());
    for (int32_t i = 0; i < sensorInfos.size(); i++) {
        const std::string sensorName(sensorInfos[i].sensorName);
        const std::string vendorName(sensorInfos[i].vendorName);
        const int32_t sensorId = sensorInfos[i].sensorId;
        const float power = sensorInfos[i].power;
        const float maxRange = sensorInfos[i].maxRange;
        HiLog::Info(LABEL, "%{public}s i: %{public}d sensorid: %{public}d sensorName: %{public}s, vendorName: %{public}s, power: %{public}f, maxRange: %{public}f",
            __func__, i, sensorId, sensorName.c_str(), vendorName.c_str(), power, maxRange);
        Sensor sensor;
        sensor.SetSensorId(sensorId);
        sensor.SetMaxRange(maxRange);
        sensor.SetName(sensorName.c_str());
        sensor.SetVendor(vendorName.c_str());
        sensorList.push_back(sensor);
    }
    return ERR_OK;
}

int32_t HdiConnectionV1_0::EnableSensor(uint32_t sensorId)
{
    int32_t ret = sensorInterface_->Enable(sensorId);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s is failed", __func__);
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnectionV1_0::DisableSensor(uint32_t sensorId)
{
    int32_t ret = sensorInterface_->Disable(sensorId);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s is failed", __func__);
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnectionV1_0::SetBatch(int32_t sensorId, int64_t samplingInterval, int64_t reportInterval)
{
    int32_t ret = sensorInterface_->SetBatch(sensorId, samplingInterval, reportInterval);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnectionV1_0::SetMode(int32_t sensorId, int32_t mode)
{
    int32_t ret = sensorInterface_->SetMode(sensorId, mode);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s is failed", __func__);
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnectionV1_0::SetOption(int32_t sensorId, uint32_t option)
{
    int32_t ret = sensorInterface_->SetOption(sensorId, option);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s is failed", __func__);
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnectionV1_0::RegisteDataReport(ZReportDataCb cb, sptr<ReportDataCallback> reportDataCallback)
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

int32_t HdiConnectionV1_0::DestroyHdiConnection()
{
    int32_t ret = sensorInterface_->Unregister(0);
    if (ret < 0) {
        HiLog::Error(LABEL, "%{public}s failed", __func__);
        return ret;
    }
    return ERR_OK;
}

int32_t HdiConnectionV1_0::RunCommand(uint32_t sensorId, int32_t cmd, int32_t params)
{
    return 0;
}

ZReportDataCb HdiConnectionV1_0::getReportDataCb()
{
    if (reportDataCb_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s reportDataCb_ cannot be null", __func__);
    }
    return reportDataCb_;
}

sptr<ReportDataCallback> HdiConnectionV1_0::getReportDataCallback()
{
    if (reportDataCallback_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s reportDataCallback_ cannot be null", __func__);
    }
    return reportDataCallback_;
}
}  // namespace Sensors
}  // namespace OHOS
