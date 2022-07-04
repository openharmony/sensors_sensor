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

#include "my_file_descriptor_listener.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
using namespace OHOS::AppExecFwk;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "MyFileDescriptorListener" };
constexpr int32_t RECEIVE_DATA_SIZE = 100;
}  // namespace

MyFileDescriptorListener::MyFileDescriptorListener()
{
    channel_ = nullptr;
    receiveDataBuff_ =
        new (std::nothrow) TransferSensorEvents[sizeof(struct TransferSensorEvents) * RECEIVE_DATA_SIZE];
    CHKPL(receiveDataBuff_);
}

MyFileDescriptorListener::~MyFileDescriptorListener()
{
    CALL_LOG_ENTER;
    if (receiveDataBuff_ != nullptr) {
        delete[] receiveDataBuff_;
        receiveDataBuff_ = nullptr;
    }
}

void MyFileDescriptorListener::OnReadable(int32_t fileDescriptor)
{
    CALL_LOG_ENTER;
    if (fileDescriptor < 0) {
        SEN_HILOGE("fileDescriptor: %{public}d", fileDescriptor);
        return;
    }

    FileDescriptorListener::OnReadable(fileDescriptor);
    if (receiveDataBuff_ == nullptr) {
        return;
    }
    int32_t len =
        recv(fileDescriptor, receiveDataBuff_, sizeof(struct TransferSensorEvents) * RECEIVE_DATA_SIZE, 0);
    int32_t eventSize = static_cast<int32_t>(sizeof(struct TransferSensorEvents));
    while (len > 0) {
        int32_t num = len / eventSize;
        for (int i = 0; i < num; i++) {
            SensorEvent event = {
                .sensorTypeId = receiveDataBuff_[i].sensorTypeId,
                .version = receiveDataBuff_[i].version,
                .timestamp = receiveDataBuff_[i].timestamp,
                .option = receiveDataBuff_[i].option,
                .mode = receiveDataBuff_[i].mode,
                .data = receiveDataBuff_[i].data,
                .dataLen = receiveDataBuff_[i].dataLen
            };
            channel_->dataCB_(&event, 1, channel_->privateData_);
        }
        len = recv(fileDescriptor, receiveDataBuff_, sizeof(struct TransferSensorEvents) * RECEIVE_DATA_SIZE, 0);
    }
}

void MyFileDescriptorListener::OnWritable(int32_t fileDescriptor){}

void MyFileDescriptorListener::SetChannel(SensorDataChannel* channel)
{
    channel_ = channel;
}

void MyFileDescriptorListener::OnShutdown(int32_t fileDescriptor)
{
    if (fileDescriptor < 0) {
        SEN_HILOGE("param is error: %{public}d", fileDescriptor);
    }

    FileDescriptorListener::OnShutdown(fileDescriptor);
    if (receiveDataBuff_ != nullptr) {
        delete[] receiveDataBuff_;
        receiveDataBuff_ = nullptr;
    }
}

void MyFileDescriptorListener::OnException(int32_t fileDescriptor)
{
    if (fileDescriptor < 0) {
        SEN_HILOGE("param is error: %{public}d", fileDescriptor);
        return;
    }

    FileDescriptorListener::OnException(fileDescriptor);
    if (receiveDataBuff_ != nullptr) {
        delete[] receiveDataBuff_;
        receiveDataBuff_ = nullptr;
    }
}
}  // namespace Sensors
}  // namespace OHOS