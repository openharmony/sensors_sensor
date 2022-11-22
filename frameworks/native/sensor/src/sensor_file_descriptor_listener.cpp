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

#include "sensor_file_descriptor_listener.h"
#include "sensor_agent_type.h"
#include "sensor_basic_data_channel.h"
#include "sensors_errors.h"
#include "sys/socket.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
using namespace OHOS::AppExecFwk;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "SensorFileDescriptorListener" };
constexpr int32_t RECEIVE_DATA_SIZE = 100;
}  // namespace

void SensorFileDescriptorListener::OnReadable(int32_t fileDescriptor)
{
    if (fileDescriptor < 0) {
        SEN_HILOGE("fileDescriptor:%{public}d", fileDescriptor);
        return;
    }
    CHKPV(channel_);
    SensorData receiveDataBuff[RECEIVE_DATA_SIZE] = {};
    ssize_t len =
        recv(fileDescriptor, receiveDataBuff, sizeof(SensorData) * RECEIVE_DATA_SIZE, MSG_DONTWAIT);
    int32_t eventSize = static_cast<int32_t>(sizeof(SensorData));
    while (len > 0) {
        int32_t num = len / eventSize;
        for (int i = 0; i < num; i++) {
            SensorEvent event = {
                .sensorTypeId = receiveDataBuff[i].sensorTypeId,
                .version = receiveDataBuff[i].version,
                .timestamp = receiveDataBuff[i].timestamp,
                .option = receiveDataBuff[i].option,
                .mode = receiveDataBuff[i].mode,
                .data = receiveDataBuff[i].data,
                .dataLen = receiveDataBuff[i].dataLen
            };
            channel_->dataCB_(&event, 1, channel_->privateData_);
        }
        len = recv(fileDescriptor, receiveDataBuff, sizeof(SensorData) * RECEIVE_DATA_SIZE, MSG_DONTWAIT);
    }
}

void SensorFileDescriptorListener::SetChannel(SensorDataChannel* channel)
{
    channel_ = channel;
}

void SensorFileDescriptorListener::OnShutdown(int32_t fileDescriptor)
{
    if (fileDescriptor < 0) {
        SEN_HILOGE("param is error:%{public}d", fileDescriptor);
    }
    CHKPV(channel_);
    channel_->DestroySensorDataChannel();
}

void SensorFileDescriptorListener::OnException(int32_t fileDescriptor)
{
    if (fileDescriptor < 0) {
        SEN_HILOGE("param is error:%{public}d", fileDescriptor);
    }
    CHKPV(channel_);
    channel_->DestroySensorDataChannel();
}
}  // namespace Sensors
}  // namespace OHOS