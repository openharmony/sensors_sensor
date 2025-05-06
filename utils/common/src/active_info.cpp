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

#include "active_info.h"

#include "sensor_errors.h"

#undef LOG_TAG
#define LOG_TAG "ActiveInfo"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

ActiveInfo::ActiveInfo(int32_t pid, int32_t deviceId, int32_t sensorTypeId, int32_t sensorId, int64_t samplingPeriodNs,
    int64_t maxReportDelayNs)
    :pid_(pid), deviceId_(deviceId), sensorTypeId_(sensorTypeId), sensorId_(sensorId),
    samplingPeriodNs_(samplingPeriodNs), maxReportDelayNs_(maxReportDelayNs)
{}

int32_t ActiveInfo::GetPid() const
{
    return pid_;
}

void ActiveInfo::SetPid(int32_t pid)
{
    pid_ = pid;
}

int32_t ActiveInfo::GetDeviceId() const
{
    return  deviceId_;
}

void ActiveInfo::SetDeviceId(int32_t deviceId)
{
    deviceId_ = deviceId;
}

int32_t ActiveInfo::GetSensorTypeId() const
{
    return sensorTypeId_;
}

void ActiveInfo::SetSensorTypeId(int32_t sensorTypeId)
{
    sensorTypeId_ = sensorTypeId;
}

int32_t ActiveInfo::GetSensorId() const
{
    return sensorId_;
}

void ActiveInfo::SetSensorId(int32_t sensorId)
{
    sensorId_ = sensorId;
}

int64_t ActiveInfo::GetSamplingPeriodNs() const
{
    return samplingPeriodNs_;
}

void ActiveInfo::SetSamplingPeriodNs(int64_t samplingPeriodNs)
{
    samplingPeriodNs_ = samplingPeriodNs;
}

int64_t ActiveInfo::GetMaxReportDelayNs() const
{
    return maxReportDelayNs_;
}

void ActiveInfo::SetMaxReportDelayNs(int64_t maxReportDelayNs)
{
    maxReportDelayNs_ = maxReportDelayNs;
}

bool ActiveInfo::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteInt32(pid_)) {
        SEN_HILOGE("Write pid failed");
        return false;
    }
    if (!parcel.WriteInt32(deviceId_)) {
        SEN_HILOGE("Write deviceId failed");
        return false;
    }
    if (!parcel.WriteInt32(sensorTypeId_)) {
        SEN_HILOGE("Write sensorTypeId failed");
        return false;
    }
    if (!parcel.WriteInt32(sensorId_)) {
        SEN_HILOGE("Write sensorId failed");
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

ActiveInfo* ActiveInfo::Unmarshalling(Parcel &parcel)
{
    int32_t pid = -1;
    int32_t sensorId = -1;
    int64_t samplingPeriodNs = -1;
    int64_t maxReportDelayNs = -1;
    int32_t deviceId = -1;
    int32_t sensorTypeId = -1;
    auto activeInfo = new (std::nothrow) ActiveInfo();
    if (activeInfo == nullptr || !(parcel.ReadInt32(pid) && parcel.ReadInt32(deviceId) &&
        parcel.ReadInt32(sensorTypeId) && parcel.ReadInt32(sensorId) &&
        parcel.ReadInt64(samplingPeriodNs) && parcel.ReadInt64(maxReportDelayNs))) {
        SEN_HILOGE("Read from parcel is failed");
        if (activeInfo != nullptr) {
            delete activeInfo;
            activeInfo = nullptr;
        }
        return activeInfo;
    }
    activeInfo->SetPid(pid);
    activeInfo->SetDeviceId(deviceId);
    activeInfo->SetSensorTypeId(sensorTypeId);
    activeInfo->SetSensorId(sensorId);
    activeInfo->SetSamplingPeriodNs(samplingPeriodNs);
    activeInfo->SetMaxReportDelayNs(maxReportDelayNs);
    return activeInfo;
}
} // namespace Sensors
} // namespace OHOS