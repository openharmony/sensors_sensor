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

#include "hisysevent.h"
#include "ipc_skeleton.h"
#include "message_parcel.h"
#include "permission_util.h"
#include "sensor_client_proxy.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "SensorServiceStub" };
}  // namespace

SensorServiceStub::SensorServiceStub()
{
    CALL_LOG_ENTER;
    baseFuncs_[ENABLE_SENSOR] = &SensorServiceStub::SensorEnableInner;
    baseFuncs_[DISABLE_SENSOR] = &SensorServiceStub::SensorDisableInner;
    baseFuncs_[GET_SENSOR_LIST] = &SensorServiceStub::GetAllSensorsInner;
    baseFuncs_[TRANSFER_DATA_CHANNEL] = &SensorServiceStub::CreateDataChannelInner;
    baseFuncs_[DESTROY_SENSOR_CHANNEL] = &SensorServiceStub::DestroyDataChannelInner;
}

SensorServiceStub::~SensorServiceStub()
{
    CALL_LOG_ENTER;
    baseFuncs_.clear();
}

int32_t SensorServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
                                           MessageOption &option)
{
    SEN_HILOGD("begin, cmd : %{public}u", code);
    std::u16string descriptor = SensorServiceStub::GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (descriptor != remoteDescriptor) {
        SEN_HILOGE("client and service descriptors are inconsistent");
        return OBJECT_NULL;
    }
    auto itFunc = baseFuncs_.find(code);
    if (itFunc != baseFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            return (this->*memberFunc)(data, reply);
        }
    }
    SEN_HILOGD("no member func supporting, applying default process");
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

ErrCode SensorServiceStub::SensorEnableInner(MessageParcel &data, MessageParcel &reply)
{
    (void)reply;
    uint32_t sensorId = data.ReadUint32();
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckSensorPermission(GetCallingTokenID(), sensorId);
    if (ret != PERMISSION_GRANTED) {
        HiSysEvent::Write(HiSysEvent::Domain::SENSOR, "SENSOR_VERIFY_ACCESS_TOKEN_FAIL",
            HiSysEvent::EventType::SECURITY, "PKG_NAME", "SensorEnableInner", "ERROR_CODE", ret);
        SEN_HILOGE("sensorId:%{public}u grant failed,result:%{public}d", sensorId, ret);
        return PERMISSION_DENIED;
    }
    return EnableSensor(sensorId, data.ReadInt64(), data.ReadInt64());
}

ErrCode SensorServiceStub::SensorDisableInner(MessageParcel &data, MessageParcel &reply)
{
    (void)reply;
    uint32_t sensorId = data.ReadUint32();
    PermissionUtil &permissionUtil = PermissionUtil::GetInstance();
    int32_t ret = permissionUtil.CheckSensorPermission(GetCallingTokenID(), sensorId);
    if (ret != PERMISSION_GRANTED) {
        HiSysEvent::Write(HiSysEvent::Domain::SENSOR, "SENSOR_VERIFY_ACCESS_TOKEN_FAIL",
            HiSysEvent::EventType::SECURITY, "PKG_NAME", "SensorDisableInner", "ERROR_CODE", ret);
        SEN_HILOGE("sensorId:%{public}u grant failed,result:%{public}d", sensorId, ret);
        return PERMISSION_DENIED;
    }
    return DisableSensor(sensorId);
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
            SEN_HILOGE("sensor %{public}d failed", i);
            return GET_SENSOR_LIST_ERR;
        }
    }
    return NO_ERROR;
}

ErrCode SensorServiceStub::CreateDataChannelInner(MessageParcel &data, MessageParcel &reply)
{
    (void)reply;
    sptr<SensorBasicDataChannel> sensorChannel = new (std::nothrow) SensorBasicDataChannel();
    CHKPR(sensorChannel, OBJECT_NULL);
    auto ret = sensorChannel->CreateSensorBasicChannel(data);
    if (ret != ERR_OK) {
        SEN_HILOGE("CreateSensorBasicChannel ret:%{public}d", ret);
        return OBJECT_NULL;
    }
    sptr<IRemoteObject> sensorClient = data.ReadRemoteObject();
    CHKPR(sensorClient, OBJECT_NULL);
    return TransferDataChannel(sensorChannel, sensorClient);
}

ErrCode SensorServiceStub::DestroyDataChannelInner(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> sensorClient = data.ReadRemoteObject();
    CHKPR(sensorClient, OBJECT_NULL);
    return DestroySensorChannel(sensorClient);
}
}  // namespace Sensors
}  // namespace OHOS
