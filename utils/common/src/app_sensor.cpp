/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "app_sensor.h"

#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "AppSensor" };
}

AppSensor::AppSensor(int32_t pid, SensorStatus sensorStatus)
{
    pid_ = pid;
    sensorId_ = sensorStatus.sensorId;
    isActive_ = sensorStatus.isActive;
    samplingPeriodNs_ = sensorStatus.samplingPeriodNs;
    maxReportDelayNs_ = sensorStatus.maxReportDelayNs;
}

int32_t AppSensor::GetPid() const
{
    return pid_;
}

void AppSensor::SetPid(int32_t pid)
{
    pid_ = pid;
}

int32_t AppSensor::GetSensorId() const
{
    return sensorId_;
}

void AppSensor::SetSensorId(int32_t sensorId)
{
    sensorId_ = sensorId;
}

bool AppSensor::IsActive() const
{
    return isActive_;
}

void AppSensor::Enable(bool isActive)
{
    isActive_ = isActive;
}

int64_t AppSensor::GetSamplingPeriodNs() const
{
    return samplingPeriodNs_;
}

void AppSensor::SetSamplingPeriodNs(int64_t samplingPeriodNs)
{
    samplingPeriodNs_ = samplingPeriodNs;
}

int64_t AppSensor::GetMaxReportDelayNs() const
{
    return maxReportDelayNs_;
}

void AppSensor::SetMaxReportDelayNs(int64_t maxReportDelayNs)
{
    maxReportDelayNs_ = maxReportDelayNs;
}

bool AppSensor::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteInt32(pid_)) {
        SEN_HILOGE("Write pid failed");
        return false;
    }
    if (!parcel.WriteInt32(sensorId_)) {
        SEN_HILOGE("Write sensorId failed");
        return false;
    }
    if (!parcel.WriteBool(isActive_)) {
        SEN_HILOGE("Write isActive failed");
        return false;
    }
    if (!parcel.WriteInt64(samplingPeriodNs_)) {
        SEN_HILOGE("Write samplingPeriodNs failed");
        return false;
    }
    if (!parcel.WriteInt64(maxReportDelayNs_)) {
        SEN_HILOGE("Write maxReportDelayNs failed");
        return false;
    }
    return true;
}

std::unique_ptr<AppSensor> AppSensor::Unmarshalling(Parcel &parcel)
{
    int32_t pid = -1;
    int32_t sensorId = -1;
    bool isActive = false;
    int64_t samplingPeriodNs = -1;
    int64_t maxReportDelayNs = -1;
    if (!(parcel.ReadInt32(pid) && parcel.ReadInt32(sensorId) && parcel.ReadBool(isActive) &&
          parcel.ReadInt64(samplingPeriodNs) && parcel.ReadInt64(maxReportDelayNs))) {
        SEN_HILOGE("ReadFromParcel is failed");
        return nullptr;
    }
    auto appSensor = std::make_unique<AppSensor>();
    appSensor->SetPid(pid);
    appSensor->SetSensorId(sensorId);
    appSensor->Enable(isActive);
    appSensor->SetSamplingPeriodNs(samplingPeriodNs);
    appSensor->SetMaxReportDelayNs(maxReportDelayNs);
    return appSensor;
}
}  // namespace Sensors
}  // namespace OHOS