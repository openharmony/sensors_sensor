/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#include "sensor_parcel.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "SensorServiceProxy" };
}  // namespace

SensorServiceProxy::SensorServiceProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<ISensorService>(impl)
{}

ErrCode SensorServiceProxy::EnableSensor(int32_t sensorId, int64_t samplingPeriodNs, int64_t maxReportDelayNs)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(SensorServiceProxy::GetDescriptor())) {
        SEN_HILOGE("Write descriptor failed");
        return WRITE_PARCEL_ERR;
    }
    WRITEINT32(data, sensorId, WRITE_PARCEL_ERR);
    WRITEINT64(data, samplingPeriodNs, WRITE_PARCEL_ERR);
    WRITEINT64(data, maxReportDelayNs, WRITE_PARCEL_ERR);
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    int32_t ret = remote->SendRequest(ISensorService::ENABLE_SENSOR, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "EnableSensor", "ERROR_CODE", ret);
        SEN_HILOGE("Failed, ret:%{public}d", ret);
    }
    return static_cast<ErrCode>(ret);
}

ErrCode SensorServiceProxy::DisableSensor(int32_t sensorId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(SensorServiceProxy::GetDescriptor())) {
        SEN_HILOGE("Write descriptor failed");
        return WRITE_PARCEL_ERR;
    }
    WRITEINT32(data, sensorId, WRITE_PARCEL_ERR);
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    int32_t ret = remote->SendRequest(ISensorService::DISABLE_SENSOR, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "DisableSensor", "ERROR_CODE", ret);
        SEN_HILOGE("Failed, ret:%{public}d", ret);
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
        SEN_HILOGE("Write descriptor failed");
        return sensors;
    }
    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        SEN_HILOGE("remote is null");
        return sensors;
    }
    int32_t ret = remote->SendRequest(ISensorService::GET_SENSOR_LIST, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "GetSensorList", "ERROR_CODE", ret);
        SEN_HILOGE("Failed, ret:%{public}d", ret);
        return sensors;
    }
    uint32_t sensorCount;
    if (!reply.ReadUint32(sensorCount)) {
        SEN_HILOGE("Parcel read failed");
        return sensors;
    }
    SEN_HILOGD("sensorCount:%{public}u", sensorCount);
    if (sensorCount > MAX_SENSOR_COUNT) {
        sensorCount = MAX_SENSOR_COUNT;
    }
    Sensor sensor;
    for (uint32_t i = 0; i < sensorCount; ++i) {
        auto tmpSensor = sensor.Unmarshalling(reply);
        CHKPC(tmpSensor);
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
        SEN_HILOGE("Write descriptor failed");
        return WRITE_PARCEL_ERR;
    }
    sensorBasicDataChannel->SendToBinder(data);
    WRITEREMOTEOBJECT(data, sensorClient, WRITE_PARCEL_ERR);
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    int32_t ret = remote->SendRequest(ISensorService::TRANSFER_DATA_CHANNEL, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "TransferDataChannel", "ERROR_CODE", ret);
        SEN_HILOGE("Failed, ret:%{public}d", ret);
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
        SEN_HILOGE("Write descriptor failed");
        return WRITE_PARCEL_ERR;
    }
    WRITEREMOTEOBJECT(data, sensorClient, WRITE_PARCEL_ERR);
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    int32_t ret = remote->SendRequest(ISensorService::DESTROY_SENSOR_CHANNEL, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "DestroySensorChannel", "ERROR_CODE", ret);
        SEN_HILOGE("Failed, ret:%{public}d", ret);
    }
    return static_cast<ErrCode>(ret);
}

ErrCode SensorServiceProxy::SuspendSensors(int32_t pid)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(SensorServiceProxy::GetDescriptor())) {
        SEN_HILOGE("Parcel write descriptor failed");
        return WRITE_PARCEL_ERR;
    }
    WRITEINT32(data, pid, WRITE_PARCEL_ERR);
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(ISensorService::SUSPEND_SENSORS, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SENSOR_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "SuspendSensors", "ERROR_CODE", ret);
        SEN_HILOGE("Failed, ret:%{public}d", ret);
    }
    return static_cast<ErrCode>(ret);
}

ErrCode SensorServiceProxy::ResumeSensors(int32_t pid)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(SensorServiceProxy::GetDescriptor())) {
        SEN_HILOGE("Parcel write descriptor failed");
        return WRITE_PARCEL_ERR;
    }
    WRITEINT32(data, pid, WRITE_PARCEL_ERR);
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(ISensorService::RESUME_SENSORS, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SENSOR_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "ResumeSensors", "ERROR_CODE", ret);
        SEN_HILOGE("Failed, ret:%{public}d", ret);
    }
    return static_cast<ErrCode>(ret);
}

ErrCode SensorServiceProxy::GetActiveInfoList(int32_t pid, std::vector<ActiveInfo> &activeInfoList)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(SensorServiceProxy::GetDescriptor())) {
        SEN_HILOGE("Parcel write descriptor failed");
        return WRITE_PARCEL_ERR;
    }
    WRITEINT32(data, pid, WRITE_PARCEL_ERR);
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(ISensorService::GET_ACTIVE_INFO_LIST, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SENSOR_SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "GetActiveInfoList", "ERROR_CODE", ret);
        SEN_HILOGE("Failed, ret:%{public}d", ret);
        return static_cast<ErrCode>(ret);
    }
    uint32_t activeInfoCount;
    READUINT32(reply, activeInfoCount, READ_PARCEL_ERR);
    SEN_HILOGD("activeInfoCount:%{public}u", activeInfoCount);
    if (activeInfoCount > MAX_SENSOR_COUNT) {
        activeInfoCount = MAX_SENSOR_COUNT;
    }
    ActiveInfo activeInfo;
    for (uint32_t i = 0; i < activeInfoCount; ++i) {
        auto tmpActiveInfo = activeInfo.Unmarshalling(reply);
        CHKPC(tmpActiveInfo);
        activeInfoList.push_back(*tmpActiveInfo);
    }
    return static_cast<ErrCode>(ret);
}

ErrCode SensorServiceProxy::CreateSocketChannel(sptr<IRemoteObject> sensorClient, int32_t &clientFd)
{
    CHKPR(sensorClient, INVALID_POINTER);
    MessageParcel data;
    if (!data.WriteInterfaceToken(SensorServiceProxy::GetDescriptor())) {
        SEN_HILOGE("Parcel write descriptor failed");
        return WRITE_PARCEL_ERR;
    }
    WRITEREMOTEOBJECT(data, sensorClient, WRITE_PARCEL_ERR);
    MessageParcel reply;
    MessageOption option;
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    int32_t ret = remote->SendRequest(ISensorService::CREATE_SOCKET_CHANNEL, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "CreateSocketChannel", "ERROR_CODE", ret);
        SEN_HILOGE("failed, ret:%{public}d", ret);
        return ERROR;
    }
    clientFd = reply.ReadFileDescriptor();
    if (clientFd < 0) {
        SEN_HILOGE("Invalid fd, clientFd:%{public}d", clientFd);
        return ERROR;
    }
    return static_cast<ErrCode>(ret);
}

ErrCode SensorServiceProxy::DestroySocketChannel(sptr<IRemoteObject> sensorClient)
{
    CHKPR(sensorClient, INVALID_POINTER);
    MessageParcel data;
    if (!data.WriteInterfaceToken(SensorServiceProxy::GetDescriptor())) {
        SEN_HILOGE("Parcel write descriptor failed");
        return WRITE_PARCEL_ERR;
    }
    WRITEREMOTEOBJECT(data, sensorClient, WRITE_PARCEL_ERR);
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(ISensorService::DESTROY_SOCKET_CHANNEL, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "DestroySocketChannel", "ERROR_CODE", ret);
        SEN_HILOGE("failed, ret:%{public}d", ret);
    }
    return static_cast<ErrCode>(ret);
}

ErrCode SensorServiceProxy::EnableActiveInfoCB()
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(SensorServiceProxy::GetDescriptor())) {
        SEN_HILOGE("Parcel write descriptor failed");
        return WRITE_PARCEL_ERR;
    }
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(ISensorService::ENABLE_ACTIVE_INFO_CB, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "EnableActiveInfoCB", "ERROR_CODE", ret);
        SEN_HILOGE("failed, ret:%{public}d", ret);
    }
    return static_cast<ErrCode>(ret);
}

ErrCode SensorServiceProxy::DisableActiveInfoCB()
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(SensorServiceProxy::GetDescriptor())) {
        SEN_HILOGE("Parcel write descriptor failed");
        return WRITE_PARCEL_ERR;
    }
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(ISensorService::DISABLE_ACTIVE_INFO_CB, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "DisableActiveInfoCB", "ERROR_CODE", ret);
        SEN_HILOGE("failed, ret:%{public}d", ret);
    }
    return static_cast<ErrCode>(ret);
}

ErrCode SensorServiceProxy::ResetSensors()
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(SensorServiceProxy::GetDescriptor())) {
        SEN_HILOGE("Parcel write descriptor failed");
        return WRITE_PARCEL_ERR;
    }
    sptr<IRemoteObject> remote = Remote();
    CHKPR(remote, ERROR);
    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(ISensorService::RESET_SENSORS, data, reply, option);
    if (ret != NO_ERROR) {
        HiSysEventWrite(HiSysEvent::Domain::SENSOR, "SERVICE_IPC_EXCEPTION",
            HiSysEvent::EventType::FAULT, "PKG_NAME", "ResetSensors", "ERROR_CODE", ret);
        SEN_HILOGE("failed, ret:%{public}d", ret);
    }
    return static_cast<ErrCode>(ret);
}
}  // namespace Sensors
}  // namespace OHOS
