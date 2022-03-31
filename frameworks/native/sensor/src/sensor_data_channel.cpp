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

#include <cerrno>
#include <unistd.h>

#include <fcntl.h>
#include <sys/socket.h>

#include "my_file_descriptor_listener.h"
#include "sensors_errors.h"
#include "sensors_log_domain.h"
#include "string_ex.h"

#ifndef O_NONBLOCK
# define O_NONBLOCK	  04000
#endif

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
using namespace OHOS::AppExecFwk;
std::shared_ptr<MyEventHandler> SensorDataChannel::eventHandler_;
std::shared_ptr<AppExecFwk::EventRunner> SensorDataChannel::eventRunner_;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SensorsLogDomain::SENSOR_NATIVE, "SensorDataChannel" };
// max 100 data in cache buffer
constexpr uint32_t STOP_EVENT_ID = 0;
}  // namespace

int32_t SensorDataChannel::CreateSensorDataChannel(DataChannelCB callBack, void *data)
{
    if (callBack == nullptr) {
        SEN_HILOGE("callBack cannot be null");
        return SENSOR_NATIVE_REGSITER_CB_ERR;
    }
    dataCB_ = callBack;
    privateData_ = data;
    return InnerSensorDataChannel();
}

int32_t SensorDataChannel::RestoreSensorDataChannel()
{
    if (dataCB_ == nullptr) {
        SEN_HILOGE("dataCB_ cannot be null");
        return SENSOR_CHANNEL_RESTORE_CB_ERR;
    }
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
        SEN_HILOGE("create basic channel failed, ret : %{public}d", ret);
        return ret;
    }
    auto listener = std::make_shared<MyFileDescriptorListener>();
    listener->SetChannel(this);
    auto myRunner = AppExecFwk::EventRunner::Create(true);
    if (myRunner == nullptr) {
        SEN_HILOGE("myRunner is null");
        return ERROR;
    }
    auto handler = std::make_shared<MyEventHandler>(myRunner);
    if (handler == nullptr) {
        SEN_HILOGE("handler is null");
        return ERROR;
    }
    int32_t receiveFd = GetReceiveDataFd();
    auto inResult = handler->AddFileDescriptorListener(receiveFd, AppExecFwk::FILE_DESCRIPTOR_INPUT_EVENT, listener);
    if (inResult != 0) {
        SEN_HILOGE("AddFileDescriptorListener fail");
        return ERROR;
    }
    eventHandler_ = handler;
    eventRunner_ = myRunner;
    int64_t delayTime = 100;
    int64_t param = 0;
    bool sendEventResult = eventHandler_->SendEvent(STOP_EVENT_ID, param, delayTime);
    if (!sendEventResult) {
        SEN_HILOGE("EventHandler SendEvent fail");
        return ERROR;
    }
    int32_t runResult = eventRunner_->Run();
    if (!runResult) {
        SEN_HILOGE("EventRunner run fail");
        return ERROR;
    }
    return ERR_OK;
}

int32_t SensorDataChannel::DestroySensorDataChannel()
{
    std::lock_guard<std::mutex> eventRunnerLock(eventRunnerMutex_);
    if (eventHandler_ == nullptr || eventRunner_ == nullptr) {
        SEN_HILOGE("handler or eventRunner is null");
        return ERROR;
    }
    int32_t receiveFd = GetReceiveDataFd();
    eventHandler_->RemoveFileDescriptorListener(receiveFd);
    eventHandler_ = nullptr;
    eventRunner_->Stop();
    eventRunner_ = nullptr;
    // destroy sensor basic channelx
    return DestroySensorBasicChannel();
}

SensorDataChannel::~SensorDataChannel()
{
    DestroySensorDataChannel();
}
}  // namespace Sensors
}  // namespace OHOS
