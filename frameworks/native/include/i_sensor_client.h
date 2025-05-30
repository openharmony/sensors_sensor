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

#ifndef I_SENSOR_CLIENT_H
#define I_SENSOR_CLIENT_H

#include "iremote_broker.h"
#include "sensor_data_event.h"

namespace OHOS {
namespace Sensors {
class ISensorClient : public IRemoteBroker {
public:
    enum SensorClientInterfaceId {
        PROCESS_PLUG_EVENT = 1,
    };
    ISensorClient() = default;
    virtual ~ISensorClient() = default;
    virtual int32_t ProcessPlugEvent(const SensorPlugData &info) = 0;
    DECLARE_INTERFACE_DESCRIPTOR(u"ISensorClient");
};
} // namespace Sensors
} // namespace OHOS
#endif // I_SENSOR_CLIENT_H
