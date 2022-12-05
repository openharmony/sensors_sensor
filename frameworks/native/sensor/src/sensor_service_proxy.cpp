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

#include "sensor_service_proxy.h"

#include <vector>

#include "hisysevent.h"
#include "message_parcel.h"
#include "sensor_client_proxy.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "SensorServiceProxy" };
constexpr int32_t MAX_SENSOR_COUNT = 200;
enum {
    FLUSH = 0,
    SET_MODE,
    RESERVED,
};
}  // namespace

SensorServiceProxy::SensorServiceProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<ISensorService>(impl)
{}

ErrCode SensorServiceProxy::EnableSensor(uint32_t sensorId, int64_t samplingPeriodNs, int64_t maxReportDelayNs)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(SensorServiceProxy::GetDescriptor())) {
        SEN_HILOGE("write descriptor failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteUint32(sensorId)) {
        SEN_HILOGE("write sensorId failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt64(samplingPeriodNs)) {
        SEN_HILOGE("write samplingPeriodNs failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteInt64(maxReportDelayNs)) {
        SEN_HILOGE("write maxReportDelayNs failed");
        return WRITE_MSG_ERR;
    }
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    int32_t ret = remote->SendRequest(ISensorService::ENABLE_SENSOR, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEvent::Write(HiSysEvent::Domain::SENSOR, "SENSOR_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "EnaleSensor", "ERROR_CODE", ret);
        SEN_HILOGE("failed, ret:%{public}d", ret);
    }
    return static_cast<ErrCode>(ret);
}

ErrCode SensorServiceProxy::DisableSensor(uint32_t sensorId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(SensorServiceProxy::GetDescriptor())) {
        SEN_HILOGE("write descriptor failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteUint32(sensorId)) {
        SEN_HILOGE("write sensorId failed");
        return WRITE_MSG_ERR;
    }
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    int32_t ret = remote->SendRequest(ISensorService::DISABLE_SENSOR, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEvent::Write(HiSysEvent::Domain::SENSOR, "SENSOR_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "DisableSensor", "ERROR_CODE", ret);
        SEN_HILOGE("failed, ret:%{public}d", ret);
    }
    return static_cast<ErrCode>(ret);
}

std::vector<Sensor> SensorServiceProxy::GetSensorList()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    std::vector<Sensor> sensors;
    if (!data.WriteInterfaceToken(SensorServiceProxy::GetDescriptor())) {
        SEN_HILOGE("write descriptor failed");
        return sensors;
    }
    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        SEN_HILOGE("remote is null");
        return sensors;
    }
    int32_t ret = remote->SendRequest(ISensorService::GET_SENSOR_LIST, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEvent::Write(HiSysEvent::Domain::SENSOR, "SENSOR_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "GetSensorList", "ERROR_CODE", ret);
        SEN_HILOGE("failed, ret:%{public}d", ret);
        return sensors;
    }
    int32_t sensorCount = reply.ReadInt32();
    SEN_HILOGD("sensorCount:%{public}d", sensorCount);
    if (sensorCount > MAX_SENSOR_COUNT) {
        sensorCount = MAX_SENSOR_COUNT;
    }
    Sensor sensor;
    for (int32_t i = 0; i < sensorCount; i++) {
        auto tmpSensor = sensor.Unmarshalling(reply);
        if (tmpSensor == nullptr) {
            continue;
        }
        sensors.push_back(*tmpSensor);
    }
    return sensors;
}

ErrCode SensorServiceProxy::TransferDataChannel(const sptr<SensorBasicDataChannel> &sensorBasicDataChannel,
                                                const sptr<IRemoteObject> &sensorClient)
{
    CHKPR(sensorBasicDataChannel, OBJECT_NULL);
    CHKPR(sensorClient, OBJECT_NULL);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(SensorServiceProxy::GetDescriptor())) {
        SEN_HILOGE("write descriptor failed");
        return WRITE_MSG_ERR;
    }
    sensorBasicDataChannel->SendToBinder(data);
    if (!data.WriteRemoteObject(sensorClient)) {
        SEN_HILOGE("sensorClient failed");
        return WRITE_MSG_ERR;
    }
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    int32_t ret = remote->SendRequest(ISensorService::TRANSFER_DATA_CHANNEL, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEvent::Write(HiSysEvent::Domain::SENSOR, "SENSOR_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "TransferDataChannel", "ERROR_CODE", ret);
        SEN_HILOGE("failed, ret:%{public}d", ret);
    }
    sensorBasicDataChannel->CloseSendFd();
    return static_cast<ErrCode>(ret);
}

ErrCode SensorServiceProxy::DestroySensorChannel(sptr<IRemoteObject> sensorClient)
{
    CHKPR(sensorClient, OBJECT_NULL);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(SensorServiceProxy::GetDescriptor())) {
        SEN_HILOGE("write descriptor failed");
        return WRITE_MSG_ERR;
    }
    if (!data.WriteRemoteObject(sensorClient)) {
        SEN_HILOGE("write sensorClient failed");
        return WRITE_MSG_ERR;
    }
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    int32_t ret = remote->SendRequest(ISensorService::DESTROY_SENSOR_CHANNEL, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEvent::Write(HiSysEvent::Domain::SENSOR, "SENSOR_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "DestroySensorChannel", "ERROR_CODE", ret);
        SEN_HILOGE("failed, ret:%{public}d", ret);
    }
    return static_cast<ErrCode>(ret);
}
}  // namespace Sensors
}  // namespace OHOS
