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

#ifndef SENSOR_BASIC_DATA_CHANNEL_H
#define SENSOR_BASIC_DATA_CHANNEL_H

#include <mutex>

#include "message_parcel.h"
#include "sensor.h"
#include "sensor_data_event.h"

namespace OHOS {
namespace Sensors {
using ClientExcuteCB = std::function<void(int32_t)>;
class SensorBasicDataChannel : public RefBase {
public:
    SensorBasicDataChannel();
    virtual ~SensorBasicDataChannel();
    int32_t CreateSensorBasicChannel();
    int32_t CreateSensorBasicChannel(MessageParcel &data);
    int32_t CreateSensorBasicChannelBySendFd(int32_t sendFd);
    int32_t DestroySensorBasicChannel();
    int32_t GetSendDataFd();
    int32_t GetReceiveDataFd();
    int32_t SendToBinder(MessageParcel &data);
    void CloseSendFd();
    int32_t SendData(const void *vaddr, size_t size);
    int32_t ReceiveData(ClientExcuteCB callBack, void *vaddr, size_t size);
    bool GetSensorStatus() const;
    void SetSensorStatus(bool isActive);
    const std::unordered_map<SensorDescription, SensorData> &GetDataCacheBuf() const;
    std::string GetPackageName();
    void SetPackageName(std::string packageName);

private:
    std::mutex fdLock_;
    int32_t sendFd_;
    int32_t receiveFd_;
    bool isActive_;
    std::mutex statusLock_;
    std::unordered_map<SensorDescription, SensorData> dataCacheBuf_;
    std::string packageName_;
    std::mutex pkNameLock_;
};
} // namespace Sensors
} // namespace OHOS
#endif // SENSOR_H
