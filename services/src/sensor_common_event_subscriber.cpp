/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "sensor_common_event_subscriber.h"

#include "sensor_errors.h"

#undef LOG_TAG
#define LOG_TAG "SensorCommonEventSubscriber"

namespace OHOS {
namespace Sensors {
SensorCommonEventSubscriber::SensorCommonEventSubscriber(
    const EventFwk::CommonEventSubscribeInfo &subscribeInfo, EventReceiver receiver)
    : EventFwk::CommonEventSubscriber(subscribeInfo), eventReceiver_(receiver) {}

void SensorCommonEventSubscriber::OnReceiveEvent(const EventFwk::CommonEventData &data)
{
    if (eventReceiver_ == nullptr) {
        SEN_HILOGE("eventReceiver_ is nullptr");
        return;
    }
    eventReceiver_(data);
}
} // namespace Sensors
} // namespace OHOS