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

#include "sensor.h"

#include "sensors_errors.h"
#include "sensors_log_domain.h"
namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SensorsLogDomain::SENSOR_UTILS, "Sensor" };
}

Sensor::Sensor()
    : sensorId_(0),
      sensorTypeId_(0),
      sensorName_(""),
      vendorName_(""),
      firmwareVersion_(""),
      hardwareVersion_(""),
      maxRange_(0.0),
      resolution_(0.0),
      power_(0.0),
      flags_(0),
      fifoMaxEventCount_(0),
      minSamplePeriodNs_(0),
      maxSamplePeriodNs_(0)
{}

uint32_t Sensor::GetSensorId() const
{
    return sensorId_;
}

void Sensor::SetSensorId(uint32_t sensorId)
{
    sensorId_ = sensorId;
}

uint32_t Sensor::GetSensorTypeId() const
{
    return sensorTypeId_;
}

void Sensor::SetSensorTypeId(uint32_t sensorTypeId)
{
    sensorTypeId_ = sensorTypeId;
}

std::string Sensor::GetSensorName() const
{
    return sensorName_;
}

void Sensor::SetSensorName(const std::string &sensorName)
{
    sensorName_ = sensorName;
}

std::string Sensor::GetVendorName() const
{
    return vendorName_;
}

void Sensor::SetVendorName(const std::string &vendorName)
{
    vendorName_ = vendorName;
}

std::string Sensor::GetHardwareVersion() const
{
    return hardwareVersion_;
}

void Sensor::SetHardwareVersion(const std::string &hardwareVersion)
{
    hardwareVersion_ = hardwareVersion;
}

std::string Sensor::GetFirmwareVersion() const
{
    return firmwareVersion_;
}

void Sensor::SetFirmwareVersion(const std::string &firmwareVersion)
{
    firmwareVersion_ = firmwareVersion;
}

float Sensor::GetMaxRange() const
{
    return maxRange_;
}

void Sensor::SetMaxRange(float maxRange)
{
    maxRange_ = maxRange;
}

float Sensor::GetResolution() const
{
    return resolution_;
}

void Sensor::SetResolution(float resolution)
{
    resolution_ = resolution;
}

float Sensor::GetPower() const
{
    return power_;
}

void Sensor::SetPower(float power)
{
    power_ = power;
}

uint32_t Sensor::Sensor::GetFlags() const
{
    return flags_;
}

void Sensor::SetFlags(uint32_t flags)
{
    flags_ = flags;
}

int32_t Sensor::GetFifoMaxEventCount() const
{
    return fifoMaxEventCount_;
}

void Sensor::SetFifoMaxEventCount(int32_t fifoMaxEventCount)
{
    fifoMaxEventCount_ = fifoMaxEventCount;
}

int64_t Sensor::GetMinSamplePeriodNs() const
{
    return minSamplePeriodNs_;
}

void Sensor::SetMinSamplePeriodNs(int64_t minSamplePeriodNs)
{
    minSamplePeriodNs_ = minSamplePeriodNs;
}

int64_t Sensor::GetMaxSamplePeriodNs() const
{
    return maxSamplePeriodNs_;
}

void Sensor::SetMaxSamplePeriodNs(int64_t maxSamplePeriodNs)
{
    maxSamplePeriodNs_ = maxSamplePeriodNs;
}

bool Sensor::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteUint32(sensorId_)) {
        HiLog::Error(LABEL, "%{public}s failed, write sensorId failed", __func__);
        return false;
    }
    if (!parcel.WriteUint32(sensorTypeId_)) {
        HiLog::Error(LABEL, "%{public}s failed, write sensorTypeId failed", __func__);
        return false;
    }
    if (!parcel.WriteString(sensorName_)) {
        HiLog::Error(LABEL, "%{public}s failed, write sensorName failed", __func__);
        return false;
    }
    if (!parcel.WriteString(vendorName_)) {
        HiLog::Error(LABEL, "%{public}s failed, write vendorName failed", __func__);
        return false;
    }
    if (!parcel.WriteString(firmwareVersion_)) {
        HiLog::Error(LABEL, "%{public}s failed, write firmwareVersion failed", __func__);
        return false;
    }
    if (!parcel.WriteString(hardwareVersion_)) {
        HiLog::Error(LABEL, "%{public}s failed, write hardwareVersion failed", __func__);
        return false;
    }
    if (!parcel.WriteFloat(maxRange_)) {
        HiLog::Error(LABEL, "%{public}s failed, write maxRange failed", __func__);
        return false;
    }
    if (!parcel.WriteFloat(resolution_)) {
        HiLog::Error(LABEL, "%{public}s failed, write resolution failed", __func__);
        return false;
    }
    if (!parcel.WriteFloat(power_)) {
        HiLog::Error(LABEL, "%{public}s failed, write power failed", __func__);
        return false;
    }
    if (!parcel.WriteUint32(flags_)) {
        HiLog::Error(LABEL, "%{public}s failed, write flags failed", __func__);
        return false;
    }
    if (!parcel.WriteInt32(fifoMaxEventCount_)) {
        HiLog::Error(LABEL, "%{public}s failed, write fifoMaxEventCount failed", __func__);
        return false;
    }
    if (!parcel.WriteInt64(minSamplePeriodNs_)) {
        HiLog::Error(LABEL, "%{public}s failed, write minSamplePeriodNs failed", __func__);
        return false;
    }
    if (!parcel.WriteInt64(maxSamplePeriodNs_)) {
        HiLog::Error(LABEL, "%{public}s failed, write maxSamplePeriodNs failed", __func__);
        return false;
    }
    return true;
}

std::unique_ptr<Sensor> Sensor::Unmarshalling(Parcel &parcel)
{
    auto sensor = std::make_unique<Sensor>();
    if (sensor == nullptr) {
        HiLog::Error(LABEL, "%{public}s sensor cannot be null", __func__);
        return nullptr;
    }

    if (!sensor->ReadFromParcel(parcel)) {
        HiLog::Error(LABEL, "%{public}s ReadFromParcel failed", __func__);
        return nullptr;
    }
    return sensor;
}

bool Sensor::ReadFromParcel(Parcel &parcel)
{
    sensorId_ = parcel.ReadUint32();
    sensorTypeId_ = parcel.ReadUint32();
    sensorName_ = parcel.ReadString();
    vendorName_ = parcel.ReadString();
    firmwareVersion_ = parcel.ReadString();
    hardwareVersion_ = parcel.ReadString();
    power_ = parcel.ReadFloat();
    maxRange_ = parcel.ReadFloat();
    resolution_ = parcel.ReadFloat();
    flags_ = parcel.ReadUint32();
    fifoMaxEventCount_ = parcel.ReadInt32();
    minSamplePeriodNs_ = parcel.ReadInt64();
    maxSamplePeriodNs_ = parcel.ReadInt64();
    return true;
}
}  // namespace Sensors
}  // namespace OHOS
