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
#include "sensors_log_domain.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
using namespace OHOS::AppExecFwk;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SensorsLogDomain::SENSOR_SERVICE, "MyFileDescriptorListener" };
constexpr int32_t RECEIVE_DATA_SIZE = 100;
}  // namespace

MyFileDescriptorListener::MyFileDescriptorListener()
{
    channel_ = nullptr;
    receiveDataBuff_ =
        new (std::nothrow) TransferSensorEvents[sizeof(struct TransferSensorEvents) * RECEIVE_DATA_SIZE];
    if (receiveDataBuff_ == nullptr) {
        HiLog::Error(LABEL, "%{public}s receiveDataBuff_ memory request failed", __func__);
    }
}

MyFileDescriptorListener::~MyFileDescriptorListener()
{
    HiLog::Debug(LABEL, "%{public}s begin", __func__);
    if (receiveDataBuff_ != nullptr) {
        delete[] receiveDataBuff_;
        receiveDataBuff_ = nullptr;
    }
}

void MyFileDescriptorListener::OnReadable(int32_t fileDescriptor)
{
    HiLog::Debug(LABEL, "%{public}s begin", __func__);
    if (fileDescriptor < 0) {
        HiLog::Error(LABEL, "%{public}s fileDescriptor: %{public}d", __func__, fileDescriptor);
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
                .dataLen = receiveDataBuff_[i].dataLen,
                .data = receiveDataBuff_[i].data
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
        HiLog::Error(LABEL, "%{public}s param is error: %{public}d", __func__, fileDescriptor);
        return;
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
        HiLog::Error(LABEL, "%{public}s param is error: %{public}d", __func__, fileDescriptor);
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