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

#include <cinttypes>
#include "sensor_client_stub.h"

#include "sensor_agent_proxy.h"
#include "sensor_errors.h"
#include "sensor_log.h"

#undef LOG_TAG
#define LOG_TAG "SensorClientStub"
namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

int32_t SensorClientStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
                                          MessageOption &option)
{
    std::u16string descriptor = SensorClientStub::GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (descriptor != remoteDescriptor) {
        SEN_HILOGE("Client and service descriptors are inconsistent");
        return OBJECT_NULL;
    }
    SEN_HILOGD("Begin, cmd:%{public}u", code);
    if (code != PROCESS_PLUG_EVENT) {
        SEN_HILOGE("code parameter error.");
        return PARAMETER_ERROR;
    }
    SensorPlugData info;
    CHKCR(data.ReadInt32(info.deviceId), PARAMETER_ERROR);
    CHKCR(data.ReadInt32(info.sensorTypeId), PARAMETER_ERROR);
    CHKCR(data.ReadInt32(info.sensorId), PARAMETER_ERROR);
    CHKCR(data.ReadInt32(info.location), PARAMETER_ERROR);
    CHKCR(data.ReadString(info.deviceName), PARAMETER_ERROR);
    CHKCR(data.ReadInt32(info.status), PARAMETER_ERROR);
    CHKCR(data.ReadInt32(info.reserved), PARAMETER_ERROR);
    CHKCR(data.ReadInt64(info.timestamp), PARAMETER_ERROR);
    int32_t result = ProcessPlugEvent(info);
    if (result != NO_ERROR) {
        SEN_HILOGE("Process plug event failed");
    }
    return NO_ERROR;
}

int32_t SensorClientStub::ProcessPlugEvent(const SensorPlugData &info)
{
    CALL_LOG_ENTER;
    if (!(SENSOR_AGENT_IMPL->HandlePlugSensorData(info))) {
        SEN_HILOGE("Handle sensor data failed");
        return PARAMETER_ERROR;
    }
    SEN_HILOGD("Success to process plug event");
    return NO_ERROR;
}
} // namespace Sensors
} // namespace OHOS
