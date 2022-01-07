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

#include "sensor_service_stub.h"

#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "ipc_skeleton.h"
#include "message_parcel.h"
#include "permission_util.h"
#include "sensor_client_proxy.h"
#include "sensors_errors.h"
#include "sensors_log_domain.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SensorsLogDomain::SENSOR_SERVICE, "SensorServiceStub" };
constexpr uint32_t SENSOR_ACCELEROMETER_ID = 256;
constexpr uint32_t SENSOR_ACCELEROMETER_UNCALIBRATED_ID = 65792;
constexpr uint32_t SENSOR_LINEAR_ACCELERATION_ID = 131328;
constexpr uint32_t SENSOR_GYROSCOPE_ID = 262400;
constexpr uint32_t SENSOR_GYROSCOPE_UNCALIBRATED_ID = 327936;
constexpr uint32_t SENSOR_PEDOMETER_DETECTION_ID = 524544;
constexpr uint32_t SENSOR_PEDOMETER_ID = 590080;
constexpr uint32_t SENSOR_HEART_RATE_ID = 83886336;
const std::string ACCELEROMETER_PERMISSION = "ohos.permission.ACCELEROMETER";
const std::string GYROSCOPE_PERMISSION = "ohos.permission.GYROSCOPE";
const std::string ACTIVITY_MOTION_PERMISSION = "ohos.permission.ACTIVITY_MOTION";
const std::string READ_HEALTH_DATA_PERMISSION = "ohos.permission.READ_HEALTH_DATA";
}  // namespace

std::unordered_map<uint32_t, std::string> SensorServiceStub::sensorIdPermissions_ = {
    { SENSOR_ACCELEROMETER_ID, ACCELEROMETER_PERMISSION },
    { SENSOR_ACCELEROMETER_UNCALIBRATED_ID, ACCELEROMETER_PERMISSION },
    { SENSOR_LINEAR_ACCELERATION_ID, ACCELEROMETER_PERMISSION },
    { SENSOR_GYROSCOPE_ID, GYROSCOPE_PERMISSION },
    { SENSOR_GYROSCOPE_UNCALIBRATED_ID, GYROSCOPE_PERMISSION },
    { SENSOR_PEDOMETER_DETECTION_ID, ACTIVITY_MOTION_PERMISSION },
    { SENSOR_PEDOMETER_ID, ACTIVITY_MOTION_PERMISSION },
    { SENSOR_HEART_RATE_ID, READ_HEALTH_DATA_PERMISSION }
};

SensorServiceStub::SensorServiceStub()
{
    HiLog::Info(LABEL, "%{public}s begin,  %{public}p", __func__, this);
    baseFuncs_[ENABLE_SENSOR] = &SensorServiceStub::SensorEnableInner;
    baseFuncs_[DISABLE_SENSOR] = &SensorServiceStub::SensorDisableInner;
    baseFuncs_[GET_SENSOR_STATE] = &SensorServiceStub::GetSensorStateInner;
    baseFuncs_[RUN_COMMAND] = &SensorServiceStub::RunCommandInner;
    baseFuncs_[GET_SENSOR_LIST] = &SensorServiceStub::GetAllSensorsInner;
    baseFuncs_[TRANSFER_DATA_CHANNEL] = &SensorServiceStub::CreateDataChannelInner;
    baseFuncs_[DESTROY_SENSOR_CHANNEL] = &SensorServiceStub::DestroyDataChannelInner;
}

SensorServiceStub::~SensorServiceStub()
{
    HiLog::Info(LABEL, "%{public}s begin, yigou %{public}p", __func__, this);
    baseFuncs_.clear();
}

int32_t SensorServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
                                           MessageOption &option)
{
    HiLog::Debug(LABEL, "%{public}s begin, cmd : %{public}u", __func__, code);
    std::u16string descriptor = SensorServiceStub::GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (descriptor != remoteDescriptor) {
        HiLog::Error(LABEL, "%{public}s client and service descriptors are inconsistent", __func__);
        return OBJECT_NULL;
    }
    auto itFunc = baseFuncs_.find(code);
    if (itFunc != baseFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            return (this->*memberFunc)(data, reply);
        }
    }
    HiLog::Debug(LABEL, "%{public}s no member func supporting, applying default process", __func__);
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

bool SensorServiceStub::CheckSensorPermission(uint32_t sensorId)
{
    auto permissionIt = sensorIdPermissions_.find(sensorId);
    if (permissionIt == sensorIdPermissions_.end()) {
        return true;
    }
    return true;
}

ErrCode SensorServiceStub::SensorEnableInner(MessageParcel &data, MessageParcel &reply)
{
    (void)reply;
    uint32_t sensorId = data.ReadUint32();
    if (!CheckSensorPermission(sensorId)) {
        HiLog::Error(LABEL, "%{public}s permission denied", __func__);
        return ERR_PERMISSION_DENIED;
    }
    return EnableSensor(sensorId, data.ReadInt64(), data.ReadInt64());
}

ErrCode SensorServiceStub::SensorDisableInner(MessageParcel &data, MessageParcel &reply)
{
    (void)reply;
    uint32_t sensorId = data.ReadUint32();
    if (!CheckSensorPermission(sensorId)) {
        HiLog::Error(LABEL, "%{public}s permission denied", __func__);
        return ERR_PERMISSION_DENIED;
    }
    return DisableSensor(sensorId);
}

ErrCode SensorServiceStub::GetSensorStateInner(MessageParcel &data, MessageParcel &reply)
{
    (void)reply;
    uint32_t sensorId = data.ReadUint32();
    if (!CheckSensorPermission(sensorId)) {
        HiLog::Error(LABEL, "%{public}s permission denied", __func__);
        return ERR_PERMISSION_DENIED;
    }
    return GetSensorState(sensorId);
}

ErrCode SensorServiceStub::RunCommandInner(MessageParcel &data, MessageParcel &reply)
{
    (void)reply;
    uint32_t sensorId = data.ReadUint32();
    if (!CheckSensorPermission(sensorId)) {
        HiLog::Error(LABEL, "%{public}s permission denied", __func__);
        return ERR_PERMISSION_DENIED;
    }
    return RunCommand(sensorId, data.ReadUint32(), data.ReadUint32());
}

ErrCode SensorServiceStub::GetAllSensorsInner(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    std::vector<Sensor> sensors(GetSensorList());
    int32_t sensorCount = int32_t { sensors.size() };
    reply.WriteInt32(sensorCount);
    for (int32_t i = 0; i < sensorCount; i++) {
        bool flag = sensors[i].Marshalling(reply);
        if (!flag) {
            HiLog::Error(LABEL, "Marshalling sensor %{public}d failed", i);
            return GET_SENSOR_LIST_ERR;
        }
    }
    return NO_ERROR;
}

ErrCode SensorServiceStub::CreateDataChannelInner(MessageParcel &data, MessageParcel &reply)
{
    (void)reply;
    sptr<SensorBasicDataChannel> sensorChannel = new (std::nothrow) SensorBasicDataChannel();
    if (sensorChannel == nullptr) {
        HiLog::Error(LABEL, "%{public}s sensorChannel cannot be null", __func__);
        return OBJECT_NULL;
    }
    auto ret = sensorChannel->CreateSensorBasicChannel(data);
    if (ret != ERR_OK) {
        HiLog::Error(LABEL, "%{public}s CreateSensorBasicChannel ret : %{public}d", __func__, ret);
        return OBJECT_NULL;
    }
    sptr<IRemoteObject> sensorClient = data.ReadRemoteObject();
    if (sensorClient == nullptr) {
        HiLog::Error(LABEL, "%{public}s sensorClient cannot be null", __func__);
        return OBJECT_NULL;
    }
    return TransferDataChannel(sensorChannel, sensorClient);
}

ErrCode SensorServiceStub::DestroyDataChannelInner(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> sensorClient = data.ReadRemoteObject();
    if (sensorClient == nullptr) {
        HiLog::Error(LABEL, "%{public}s sensorClient cannot be null", __func__);
        return OBJECT_NULL;
    }
    return DestroySensorChannel(sensorClient);
}
}  // namespace Sensors
}  // namespace OHOS
