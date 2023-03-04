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

#ifndef SUBSCRIBE_INFO_H
#define SUBSCRIBE_INFO_H

#include "parcel.h"

#include "sensor_agent_type.h"

namespace OHOS {
namespace Sensors {
class SubscribeInfo : public Parcelable {
public:
    SubscribeInfo() = default;
    SubscribeInfo(SubscribeSensorInfo subscribeSensorInfo);
    virtual ~SubscribeInfo() = default;
    int32_t GetPid() const;
    void SetPid(int32_t pid);
    int32_t GetSensorId() const;
    void SetSensorId(int32_t sensorId);
    bool IsActive() const;
    void Enable(bool isActive);
    int64_t GetSamplingPeriodNs() const;
    void SetSamplingPeriodNs(int64_t samplingPeriodNs);
    int64_t GetMaxReportDelayNs() const;
    void SetMaxReportDelayNs(int64_t maxReportDelayNs);
    bool Marshalling(Parcel &parcel) const;
    std::unique_ptr<SubscribeInfo> Unmarshalling(Parcel &parcel);

private:
    int32_t pid_ { -1 };
    int32_t sensorId_ { -1 };
    bool isActive_ { -1 };
    int64_t samplingPeriodNs_ { -1 };
    int64_t maxReportDelayNs_ { -1 };
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // SUBSCRIBE_INFO_H