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

#include "sensor_data_channel.h"

#include "errors.h"
#include "sensor_file_descriptor_listener.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
using namespace OHOS::AppExecFwk;
std::shared_ptr<SensorEventHandler> SensorDataChannel::eventHandler_;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "SensorDataChannel" };
}  // namespace

int32_t SensorDataChannel::CreateSensorDataChannel(DataChannelCB callBack, void *data)
{
    CHKPR(callBack, SENSOR_NATIVE_REGSITER_CB_ERR);
    dataCB_ = callBack;
    privateData_ = data;
    return InnerSensorDataChannel();
}

int32_t SensorDataChannel::RestoreSensorDataChannel()
{
    CHKPR(dataCB_, SENSOR_NATIVE_REGSITER_CB_ERR);
    if (GetReceiveDataFd() != -1) {
        SEN_HILOGE("fd not close");
        return SENSOR_CHANNEL_RESTORE_FD_ERR;
    }
    return InnerSensorDataChannel();
}

int32_t SensorDataChannel::InnerSensorDataChannel()
{
    std::lock_guard<std::mutex> eventRunnerLock(eventRunnerMutex_);
    // create basic data channel
    int32_t ret = CreateSensorBasicChannel();
    if (ret != ERR_OK) {
        SEN_HILOGE("create basic channel failed, ret:%{public}d", ret);
        return ret;
    }
    auto listener = std::make_shared<SensorFileDescriptorListener>();
    listener->SetChannel(this);
    auto myRunner = AppExecFwk::EventRunner::Create(true);
    CHKPR(myRunner, ERROR);
    eventHandler_ = std::make_shared<SensorEventHandler>(myRunner);
    CHKPR(eventHandler_, ERROR);
    int32_t receiveFd = GetReceiveDataFd();
    auto inResult = eventHandler_->AddFileDescriptorListener(receiveFd,
        AppExecFwk::FILE_DESCRIPTOR_INPUT_EVENT, listener);
    if (inResult != 0) {
        SEN_HILOGE("AddFileDescriptorListener fail");
        return ERROR;
    }
    return ERR_OK;
}

int32_t SensorDataChannel::DestroySensorDataChannel()
{
    std::lock_guard<std::mutex> eventRunnerLock(eventRunnerMutex_);
    CHKPL(eventHandler_);
    eventHandler_ = nullptr;
    // destroy sensor basic channelx
    return DestroySensorBasicChannel();
}

SensorDataChannel::~SensorDataChannel()
{
    DestroySensorDataChannel();
}
}  // namespace Sensors
}  // namespace OHOS
