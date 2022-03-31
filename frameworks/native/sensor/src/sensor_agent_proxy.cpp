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

#include "sensor_agent_proxy.h"

#include <cstring>

#include "securec.h"
#include "sensor_catalog.h"
#include "sensor_service_client.h"
#include "sensors_errors.h"
#include "sensors_log_domain.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace Sensors {
namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, OHOS::SensorsLogDomain::SENSORS_IMPLEMENT, "SensorAgentProxy" };

using OHOS::ERR_OK;
using OHOS::Sensors::BODY;
using OHOS::Sensors::DEVICE_MOTION;
using OHOS::Sensors::ENVIRONMENT;
using OHOS::Sensors::INVALID_POINTER;
using OHOS::Sensors::LIGHT;
using OHOS::Sensors::ORIENTATION;
using OHOS::Sensors::OTHER;
using OHOS::Sensors::SENSOR_TYPE_6DOF_ATTITUDE;
using OHOS::Sensors::SENSOR_TYPE_ACCELEROMETER;
using OHOS::Sensors::SENSOR_TYPE_ACCELEROMETER_UNCALIBRATED;
using OHOS::Sensors::SENSOR_TYPE_AMBIENT_LIGHT;
using OHOS::Sensors::SENSOR_TYPE_AMBIENT_TEMPERATURE;
using OHOS::Sensors::SENSOR_TYPE_BAROMETER;
using OHOS::Sensors::SENSOR_TYPE_DEVICE_ORIENTATION;
using OHOS::Sensors::SENSOR_TYPE_GAME_ROTATION_VECTOR;
using OHOS::Sensors::SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR;
using OHOS::Sensors::SENSOR_TYPE_GRAVITY;
using OHOS::Sensors::SENSOR_TYPE_GYROSCOPE;
using OHOS::Sensors::SENSOR_TYPE_GYROSCOPE_UNCALIBRATED;
using OHOS::Sensors::SENSOR_TYPE_HEART_RATE_DETECTOR;
using OHOS::Sensors::SENSOR_TYPE_HUMIDITY;
using OHOS::Sensors::SENSOR_TYPE_LINEAR_ACCELERATION;
using OHOS::Sensors::SENSOR_TYPE_MAGNETIC_FIELD;
using OHOS::Sensors::SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED;
using OHOS::Sensors::SENSOR_TYPE_ORIENTATION;
using OHOS::Sensors::SENSOR_TYPE_PRESSURE_DETECTOR;
using OHOS::Sensors::SENSOR_TYPE_PROXIMITY;
using OHOS::Sensors::SENSOR_TYPE_ROTATION_VECTOR;
using OHOS::Sensors::SENSOR_TYPE_SIGNIFICANT_MOTION;
using OHOS::Sensors::SENSOR_TYPE_STEP_COUNTER;
using OHOS::Sensors::SENSOR_TYPE_STEP_DETECTOR;
using OHOS::Sensors::SENSOR_TYPE_WEAR_DETECTOR;
using OHOS::Sensors::SensorDataChannel;
using OHOS::Sensors::SensorServiceClient;
}  // namespace

OHOS::sptr<SensorAgentProxy> SensorAgentProxy::sensorObj_ = nullptr;
bool SensorAgentProxy::g_isChannelCreated;
int64_t SensorAgentProxy::g_samplingInterval;
int64_t SensorAgentProxy::g_reportInterval;
std::mutex SensorAgentProxy::subscribeMutex_;
std::mutex SensorAgentProxy::chanelMutex_;
std::map<int32_t, const SensorUser *> SensorAgentProxy::g_subscribeMap;
std::map<int32_t, const SensorUser *> SensorAgentProxy::g_unsubscribeMap;

SensorAgentProxy::SensorAgentProxy()
    : dataChannel_(new (std::nothrow) SensorDataChannel())
{}

const SensorAgentProxy *SensorAgentProxy::GetSensorsObj()
{
    CALL_LOG_ENTER;

    if (sensorObj_ == nullptr) {
        SEN_HILOGD("sensorObj_ new object");
        sensorObj_ = new (std::nothrow) SensorAgentProxy();
    }
    return sensorObj_;
}

void SensorAgentProxy::HandleSensorData(struct SensorEvent *events, int32_t num, void *data)
{
    if (events == nullptr || num <= 0) {
        SEN_HILOGE("events is null or num is invalid");
        return;
    }
    struct SensorEvent eventStream;
    for (int32_t i = 0; i < num; ++i) {
        eventStream = events[i];
        CHKPV(eventStream.data);
        if (g_subscribeMap.find(eventStream.sensorTypeId) == g_subscribeMap.end()) {
            SEN_HILOGE("sensorTypeId not in g_subscribeMap");
            return;
        }
        if (g_subscribeMap[eventStream.sensorTypeId] == nullptr) {
            SEN_HILOGE("sensor user is null");
			return;
        }
        g_subscribeMap[eventStream.sensorTypeId]->callback(&eventStream);
    }
}

int32_t SensorAgentProxy::CreateSensorDataChannel() const
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> chanelLock(chanelMutex_);
    if (g_isChannelCreated) {
        SEN_HILOGI("the channel has already been created");
        return ERR_OK;
    }
    CHKPR(dataChannel_, INVALID_POINTER);
    auto ret = dataChannel_->CreateSensorDataChannel(HandleSensorData, nullptr);
    if (ret != ERR_OK) {
        SEN_HILOGE("create data channel failed, ret: %{public}d", ret);
        return ret;
    }
    auto &client = SensorServiceClient::GetInstance();
    ret = client.TransferDataChannel(dataChannel_);
    if (ret != ERR_OK) {
        auto destoryRet = dataChannel_->DestroySensorDataChannel();
        SEN_HILOGE("transfer data channel failed, ret : %{public}d, destoryRet : %{public}d", ret, destoryRet);
        return ret;
    }
    g_isChannelCreated = true;
    return ERR_OK;
}

int32_t SensorAgentProxy::DestroySensorDataChannel() const
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> chanelLock(chanelMutex_);
    if (!g_isChannelCreated) {
        SEN_HILOGI("channel has been destroyed");
        return ERR_OK;
    }
    CHKPR(dataChannel_, INVALID_POINTER);
    int32_t ret = dataChannel_->DestroySensorDataChannel();
    if (ret != ERR_OK) {
        SEN_HILOGE("destory data channel failed, ret : %{public}d", ret);
        return ret;
    }
    SensorServiceClient &client = SensorServiceClient::GetInstance();
    ret = client.DestroyDataChannel();
    if (ret != ERR_OK) {
        SEN_HILOGE("destory service data channel fail, ret : %{public}d", ret);
        return ret;
    }
    g_isChannelCreated = false;
    return ERR_OK;
}

int32_t SensorAgentProxy::ActivateSensor(int32_t sensorId, const SensorUser *user) const
{
    if (user == nullptr || sensorId < 0 || user->callback == nullptr) {
        SEN_HILOGE("user is null or sensorId is invalid");
        return OHOS::Sensors::ERROR;
    }
    if (g_samplingInterval < 0 || g_reportInterval < 0) {
        SEN_HILOGE("samplingPeroid or g_reportInterval is invalid");
        return OHOS::Sensors::ERROR;
    }
    std::lock_guard<std::mutex> subscribeLock(subscribeMutex_);
    if ((g_subscribeMap.find(sensorId) == g_subscribeMap.end()) || (g_subscribeMap[sensorId] != user)) {
        SEN_HILOGE("subscribe sensorId first");
        return OHOS::Sensors::ERROR;
    }
    SensorServiceClient &client = SensorServiceClient::GetInstance();
    int32_t ret = client.EnableSensor(sensorId, g_samplingInterval, g_reportInterval);
    g_samplingInterval = -1;
    g_reportInterval = -1;
    if (ret != 0) {
        SEN_HILOGE("enable sensor failed, ret: %{public}d", ret);
        g_subscribeMap.erase(sensorId);

        return OHOS::Sensors::ERROR;
    }
    return OHOS::Sensors::SUCCESS;
}

int32_t SensorAgentProxy::DeactivateSensor(int32_t sensorId, const SensorUser *user) const
{
    if (user == nullptr || sensorId < 0 || user->callback == nullptr) {
        SEN_HILOGE("user is null or sensorId is invalid");
        return OHOS::Sensors::ERROR;
    }
    std::lock_guard<std::mutex> subscribeLock(subscribeMutex_);
    if ((g_subscribeMap.find(sensorId) == g_subscribeMap.end()) || (g_subscribeMap[sensorId] != user)) {
        SEN_HILOGE("subscribe sensorId first");
        return OHOS::Sensors::ERROR;
    }
    g_subscribeMap.erase(sensorId);
    g_unsubscribeMap[sensorId] = user;
    SensorServiceClient &client = SensorServiceClient::GetInstance();
    int32_t ret = client.DisableSensor(sensorId);
    if (ret != 0) {
        SEN_HILOGE("disable sensor failed, ret: %{public}d", ret);
        return OHOS::Sensors::ERROR;
    }
    return OHOS::Sensors::SUCCESS;
}

int32_t SensorAgentProxy::SetBatch(int32_t sensorId, const SensorUser *user, int64_t samplingInterval,
                                   int64_t reportInterval) const
{
    if (user == nullptr || sensorId < 0) {
        SEN_HILOGE("user is null or sensorId is invalid");
        return OHOS::Sensors::ERROR;
    }
    if (samplingInterval < 0 || reportInterval < 0) {
        SEN_HILOGE("samplingInterval or reportInterval is invalid");
        return OHOS::Sensors::ERROR;
    }
    std::lock_guard<std::mutex> subscribeLock(subscribeMutex_);
    if ((g_subscribeMap.find(sensorId) == g_subscribeMap.end()) || (g_subscribeMap.at(sensorId) != user)) {
        SEN_HILOGE("subscribe sensorId first");
        return OHOS::Sensors::ERROR;
    }
    g_samplingInterval = samplingInterval;
    g_reportInterval = reportInterval;
    return OHOS::Sensors::SUCCESS;
}

int32_t SensorAgentProxy::SubscribeSensor(int32_t sensorId, const SensorUser *user) const
{
    SEN_HILOGI("in, sensorId: %{public}d", sensorId);
    if (user == nullptr || sensorId < 0 || user->callback == nullptr) {
        SEN_HILOGE("user or sensorId is invalid");
        return OHOS::Sensors::ERROR;
    }
    int32_t ret = CreateSensorDataChannel();
    if (ret != ERR_OK) {
        SEN_HILOGE("create sensor data chanel failed");
        return OHOS::Sensors::ERROR;
    }
    std::lock_guard<std::mutex> subscribeLock(subscribeMutex_);
    g_subscribeMap[sensorId] = user;
    return OHOS::Sensors::SUCCESS;
}

int32_t SensorAgentProxy::UnsubscribeSensor(int32_t sensorId, const SensorUser *user) const
{
    SEN_HILOGI("in, sensorId: %{public}d", sensorId);
    if (user == nullptr || sensorId < 0  || user->callback == nullptr) {
        SEN_HILOGE("user is null or sensorId is invalid");
        return OHOS::Sensors::ERROR;
    }
    std::lock_guard<std::mutex> subscribeLock(subscribeMutex_);
    if (g_unsubscribeMap.find(sensorId) == g_unsubscribeMap.end() || g_unsubscribeMap[sensorId] != user) {
        SEN_HILOGE("deactivate sensorId first");
        return OHOS::Sensors::ERROR;
    }
    if (g_subscribeMap.empty()) {
        int32_t ret = DestroySensorDataChannel();
        if (ret != ERR_OK) {
            SEN_HILOGE("destory data channel fail, ret : %{public}d", ret);
            return ret;
        }
    }
    g_unsubscribeMap.erase(sensorId);
    return OHOS::Sensors::SUCCESS;
}

int32_t SensorAgentProxy::SetMode(int32_t sensorId, const SensorUser *user, int32_t mode) const
{
    if (user == nullptr || sensorId < 0 || user->callback == nullptr) {
        SEN_HILOGE("user is null or sensorId is invalid");
        return OHOS::Sensors::ERROR;
    }
    std::lock_guard<std::mutex> subscribeLock(subscribeMutex_);
    if ((g_subscribeMap.find(sensorId) == g_subscribeMap.end()) || (g_subscribeMap.at(sensorId) != user)) {
        SEN_HILOGE("subscribe sensorId first");
        return OHOS::Sensors::ERROR;
    }
    return OHOS::Sensors::SUCCESS;
}

int32_t SensorAgentProxy::SetOption(int32_t sensorId, const SensorUser *user, int32_t option) const
{
    if (user == nullptr || sensorId < 0 || user->callback == nullptr) {
        SEN_HILOGE("user is null or sensorId is invalid");
        return OHOS::Sensors::ERROR;
    }
    std::lock_guard<std::mutex> subscribeLock(subscribeMutex_);
    if ((g_subscribeMap.find(sensorId) == g_subscribeMap.end()) || (g_subscribeMap.at(sensorId) != user)) {
        SEN_HILOGE("subscribe sensorId first");
        return OHOS::Sensors::ERROR;
    }
    return OHOS::Sensors::SUCCESS;
}

int32_t SensorAgentProxy::GetAllSensors(SensorInfo **sensorInfo, int32_t *count) const
{
    if (sensorInfo == nullptr || count == nullptr) {
        SEN_HILOGE("sensorInfo or count is null");
        return OHOS::Sensors::ERROR;
    }
    SensorServiceClient &client = SensorServiceClient::GetInstance();
    std::vector<OHOS::Sensors::Sensor> sensorList_ = client.GetSensorList();
    if (sensorList_.empty()) {
        SEN_HILOGE("get sensor lists failed");
        return OHOS::Sensors::ERROR;
    }
    *count = sensorList_.size();
    *sensorInfo = (SensorInfo *)malloc(sizeof(SensorInfo) * (*count));
    CHKPR(*sensorInfo, ERROR);
    for (int32_t index = 0; index < *count; ++index) {
        errno_t ret = strcpy_s((*sensorInfo + index)->sensorName, NAME_MAX_LEN,
            sensorList_[index].GetSensorName().c_str());
        if (ret != EOK) {
            SEN_HILOGE("strcpy sensorName failed");
            return OHOS::Sensors::ERROR;
        }
        ret = strcpy_s((*sensorInfo + index)->vendorName, NAME_MAX_LEN,
            sensorList_[index].GetVendorName().c_str());
        if (ret != EOK) {
            SEN_HILOGE("strcpy vendorName failed");
            return OHOS::Sensors::ERROR;
        }
        ret = strcpy_s((*sensorInfo + index)->hardwareVersion, VERSION_MAX_LEN,
            sensorList_[index].GetHardwareVersion().c_str());
        if (ret != EOK) {
            SEN_HILOGE("strcpy hardwareVersion failed");
            return OHOS::Sensors::ERROR;
        }
        ret = strcpy_s((*sensorInfo + index)->firmwareVersion, VERSION_MAX_LEN,
            sensorList_[index].GetFirmwareVersion().c_str());
        if (ret != EOK) {
            SEN_HILOGE("strcpy hardwareVersion failed");
            return OHOS::Sensors::ERROR;
        }
        (*sensorInfo + index)->sensorId = static_cast<int32_t>(sensorList_[index].GetSensorId());
        (*sensorInfo + index)->sensorTypeId = static_cast<int32_t>(sensorList_[index].GetSensorTypeId());
        (*sensorInfo + index)->maxRange = sensorList_[index].GetMaxRange();
        (*sensorInfo + index)->precision = sensorList_[index].GetResolution();
        (*sensorInfo + index)->power = sensorList_[index].GetPower();
    }
    return OHOS::Sensors::SUCCESS;
}
}  // namespace Sensors
}  // namespace OHOS