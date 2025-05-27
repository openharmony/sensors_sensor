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

#ifndef SENSOR_CLIENT_PROXY_H
#define SENSOR_CLIENT_PROXY_H

#include "iremote_proxy.h"

#include "sensor_agent_type.h"
#include "sensor_log.h"

namespace OHOS {
namespace Sensors {
class SensorClientProxy : public IRemoteProxy<ISensorClient> {
public:
    explicit SensorClientProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<ISensorClient>(impl)
    {}
    virtual ~SensorClientProxy() = default;
    int32_t ProcessPlugEvent(const SensorPlugData &info) override
    {
        MessageOption option;
        MessageParcel dataParcel;
        MessageParcel replyParcel;
        if (!dataParcel.WriteInterfaceToken(GetDescriptor())) {
            SEN_HILOGD("Failed to write descriptor to parcelable");
            return PARAMETER_ERROR;
        }
        if (!dataParcel.WriteInt32(info.deviceId)) {
            SEN_HILOGD("Failed to write deviceId to parcelable");
            return PARAMETER_ERROR;
        }
        if (!dataParcel.WriteInt32(info.sensorTypeId)) {
            SEN_HILOGD("Failed to write sensorTypeId to parcelable");
            return PARAMETER_ERROR;
        }
        if (!dataParcel.WriteInt32(info.sensorId)) {
            SEN_HILOGD("Failed to write sensorId to parcelable");
            return PARAMETER_ERROR;
        }
        if (!dataParcel.WriteInt32(info.location)) {
            SEN_HILOGD("Failed to write location to parcelable");
            return PARAMETER_ERROR;
        }
        if (!dataParcel.WriteString(info.deviceName)) {
            SEN_HILOGD("Failed to write deviceName to parcelable");
            return PARAMETER_ERROR;
        }
        if (!dataParcel.WriteInt32(info.status)) {
            SEN_HILOGD("Failed to write status to parcelable");
            return PARAMETER_ERROR;
        }
        if (!dataParcel.WriteInt32(info.reserved)) {
            SEN_HILOGD("Failed to write reserved to parcelable");
            return PARAMETER_ERROR;
        }
        if (!dataParcel.WriteInt64(info.timestamp)) {
            SEN_HILOGD("Failed to write timestamp to parcelable");
            return PARAMETER_ERROR;
        }
        int error = Remote()->SendRequest(PROCESS_PLUG_EVENT, dataParcel, replyParcel, option);
        if (error != ERR_NONE) {
            SEN_HILOGD("failed, error code is: %{public}d", error);
            return PARAMETER_ERROR;
        }
        int result = (error == ERR_NONE) ? replyParcel.ReadInt32() : -1;
        return result;
    }

private:
    DISALLOW_COPY_AND_MOVE(SensorClientProxy);
    static inline BrokerDelegator<SensorClientProxy> delegator_;
};
} // namespace Sensors
} // namespace OHOS
#endif // SENSOR_CLIENT_PROXY_H
