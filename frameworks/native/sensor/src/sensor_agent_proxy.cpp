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

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace Sensors {
namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "SensorAgentProxy" };
constexpr uint32_t MAX_SENSOR_LIST_SIZE = 0Xffff;
}  // namespace

#define SenClient SensorServiceClient::GetInstance()
OHOS::sptr<SensorAgentProxy> SensorAgentProxy::sensorObj_ = nullptr;
bool SensorAgentProxy::g_isChannelCreated;
int64_t SensorAgentProxy::g_samplingInterval;
int64_t SensorAgentProxy::g_reportInterval;
std::recursive_mutex SensorAgentProxy::subscribeMutex_;
std::mutex SensorAgentProxy::chanelMutex_;
std::mutex sensorInfoMutex_;
SensorInfo *sensorInfos_ = nullptr;
int32_t sensorInfoCount_ = 0;
std::map<int32_t, const SensorUser *> SensorAgentProxy::g_subscribeMap;
std::map<int32_t, const SensorUser *> SensorAgentProxy::g_unsubscribeMap;

SensorAgentProxy::SensorAgentProxy()
    : dataChannel_(new (std::nothrow) SensorDataChannel())
{}

SensorAgentProxy::~SensorAgentProxy()
{
    CALL_LOG_ENTER;
    ClearSensorInfos();
}

const SensorAgentProxy *SensorAgentProxy::GetSensorsObj()
{
    CALL_LOG_ENTER;
    if (sensorObj_ == nullptr) {
        SEN_HILOGD("sensorObj_ new object");
        sensorObj_ = new (std::nothrow) SensorAgentProxy();
    }
    return sensorObj_;
}

void SensorAgentProxy::HandleSensorData(SensorEvent *events, int32_t num, void *data)
{
    CHKPV(events);
    if (num <= 0) {
        SEN_HILOGE("events is null or num is invalid");
        return;
    }
    SensorEvent eventStream;
    for (int32_t i = 0; i < num; ++i) {
        eventStream = events[i];
        std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
        auto iter = g_subscribeMap.find(eventStream.sensorTypeId);
        if (iter == g_subscribeMap.end()) {
            SEN_HILOGE("sensor is not subscribed");
            return;
        }
        const SensorUser *user = iter->second;
        CHKPV(user);
        user->callback(&eventStream);
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
        SEN_HILOGE("create data channel failed, ret:%{public}d", ret);
        return ret;
    }
    ret = SenClient.TransferDataChannel(dataChannel_);
    if (ret != ERR_OK) {
        auto destoryRet = dataChannel_->DestroySensorDataChannel();
        SEN_HILOGE("transfer data channel failed, ret:%{public}d,destoryRet:%{public}d", ret, destoryRet);
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
        SEN_HILOGE("destory data channel failed, ret:%{public}d", ret);
        return ret;
    }
    ret = SenClient.DestroyDataChannel();
    if (ret != ERR_OK) {
        SEN_HILOGE("destory service data channel fail, ret:%{public}d", ret);
        return ret;
    }
    g_isChannelCreated = false;
    return ERR_OK;
}

int32_t SensorAgentProxy::ActivateSensor(int32_t sensorId, const SensorUser *user) const
{
    CHKPR(user, OHOS::Sensors::ERROR);
    CHKPR(user->callback, OHOS::Sensors::ERROR);
    if (g_samplingInterval < 0 || g_reportInterval < 0) {
        SEN_HILOGE("samplingPeroid or g_reportInterval is invalid");
        return ERROR;
    }
    if (!SenClient.IsValid(sensorId)) {
        SEN_HILOGE("sensorId is invalid, %{public}d", sensorId);
        return PARAMETER_ERROR;
    }
    std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
    if ((g_subscribeMap.find(sensorId) == g_subscribeMap.end()) || (g_subscribeMap[sensorId] != user)) {
        SEN_HILOGE("subscribe sensorId first");
        return ERROR;
    }
    int32_t ret = SenClient.EnableSensor(sensorId, g_samplingInterval, g_reportInterval);
    g_samplingInterval = -1;
    g_reportInterval = -1;
    if (ret != 0) {
        SEN_HILOGE("enable sensor failed, ret:%{public}d", ret);
        g_subscribeMap.erase(sensorId);
        return ret;
    }
    return ret;
}

int32_t SensorAgentProxy::DeactivateSensor(int32_t sensorId, const SensorUser *user) const
{
    CHKPR(user, OHOS::Sensors::ERROR);
    CHKPR(user->callback, OHOS::Sensors::ERROR);
    if (!SenClient.IsValid(sensorId)) {
        SEN_HILOGE("sensorId is invalid, %{public}d", sensorId);
        return PARAMETER_ERROR;
    }
    std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
    if ((g_subscribeMap.find(sensorId) == g_subscribeMap.end()) || (g_subscribeMap[sensorId] != user)) {
        SEN_HILOGE("subscribe sensorId first");
        return OHOS::Sensors::ERROR;
    }
    g_subscribeMap.erase(sensorId);
    g_unsubscribeMap[sensorId] = user;
    int32_t ret = SenClient.DisableSensor(sensorId);
    if (ret != 0) {
        SEN_HILOGE("DisableSensor failed, ret:%{public}d", ret);
        return ret;
    }
    return ret;
}

int32_t SensorAgentProxy::SetBatch(int32_t sensorId, const SensorUser *user, int64_t samplingInterval,
                                   int64_t reportInterval) const
{
    CHKPR(user, OHOS::Sensors::ERROR);
    if (!SenClient.IsValid(sensorId)) {
        SEN_HILOGE("sensorId is invalid, %{public}d", sensorId);
        return PARAMETER_ERROR;
    }
    if (samplingInterval < 0 || reportInterval < 0) {
        SEN_HILOGE("samplingInterval or reportInterval is invalid");
        return OHOS::Sensors::ERROR;
    }
    std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
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
    SEN_HILOGI("in, sensorId:%{public}d", sensorId);
    CHKPR(user, OHOS::Sensors::ERROR);
    CHKPR(user->callback, OHOS::Sensors::ERROR);
    if (!SenClient.IsValid(sensorId)) {
        SEN_HILOGE("sensorId is invalid, %{public}d", sensorId);
        return PARAMETER_ERROR;
    }
    int32_t ret = CreateSensorDataChannel();
    if (ret != ERR_OK) {
        SEN_HILOGE("create sensor data chanel failed");
        return OHOS::Sensors::ERROR;
    }
    std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
    g_subscribeMap[sensorId] = user;
    return OHOS::Sensors::SUCCESS;
}

int32_t SensorAgentProxy::UnsubscribeSensor(int32_t sensorId, const SensorUser *user) const
{
    SEN_HILOGI("in, sensorId: %{public}d", sensorId);
    CHKPR(user, OHOS::Sensors::ERROR);
    CHKPR(user->callback, OHOS::Sensors::ERROR);
    if (!SenClient.IsValid(sensorId)) {
        SEN_HILOGE("sensorId is invalid, %{public}d", sensorId);
        return PARAMETER_ERROR;
    }
    std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
    if (g_unsubscribeMap.find(sensorId) == g_unsubscribeMap.end() || g_unsubscribeMap[sensorId] != user) {
        SEN_HILOGE("deactivate sensorId first");
        return OHOS::Sensors::ERROR;
    }
    if (g_subscribeMap.empty()) {
        int32_t ret = DestroySensorDataChannel();
        if (ret != ERR_OK) {
            SEN_HILOGE("destory data channel fail, ret:%{public}d", ret);
            return ret;
        }
    }
    g_unsubscribeMap.erase(sensorId);
    return OHOS::Sensors::SUCCESS;
}

int32_t SensorAgentProxy::SetMode(int32_t sensorId, const SensorUser *user, int32_t mode) const
{
    CHKPR(user, OHOS::Sensors::ERROR);
    CHKPR(user->callback, OHOS::Sensors::ERROR);
    if (!SenClient.IsValid(sensorId)) {
        SEN_HILOGE("sensorId is invalid, %{public}d", sensorId);
        return ERROR;
    }
    std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
    if ((g_subscribeMap.find(sensorId) == g_subscribeMap.end()) || (g_subscribeMap.at(sensorId) != user)) {
        SEN_HILOGE("subscribe sensorId first");
        return OHOS::Sensors::ERROR;
    }
    return OHOS::Sensors::SUCCESS;
}

void SensorAgentProxy::ClearSensorInfos() const
{
    CHKPV(sensorInfos_);
    free(sensorInfos_);
    sensorInfos_ = nullptr;
}

int32_t SensorAgentProxy::ConvertSensorInfos() const
{
    CALL_LOG_ENTER;
    std::vector<Sensor> sensorList = SenClient.GetSensorList();
    if (sensorList.empty()) {
        SEN_HILOGE("get sensor lists failed");
        return ERROR;
    }
    size_t count = sensorList.size();
    if (count > MAX_SENSOR_LIST_SIZE) {
        SEN_HILOGE("The number of sensors exceeds the maximum value");
        return ERROR;
    }
    sensorInfos_ = (SensorInfo *)malloc(sizeof(SensorInfo) * count);
    CHKPR(sensorInfos_, ERROR);
    for (size_t i = 0; i < count; ++i) {
        errno_t ret = strcpy_s((sensorInfos_ + i)->sensorName, NAME_MAX_LEN,
            sensorList[i].GetSensorName().c_str());
        CHKCR(ret == EOK, ERROR);
        ret = strcpy_s((sensorInfos_ + i)->vendorName, NAME_MAX_LEN,
            sensorList[i].GetVendorName().c_str());
        CHKCR(ret == EOK, ERROR);
        ret = strcpy_s((sensorInfos_ + i)->hardwareVersion, VERSION_MAX_LEN,
            sensorList[i].GetHardwareVersion().c_str());
        CHKCR(ret == EOK, ERROR);
        ret = strcpy_s((sensorInfos_ + i)->firmwareVersion, VERSION_MAX_LEN,
            sensorList[i].GetFirmwareVersion().c_str());
        CHKCR(ret == EOK, ERROR);
        (sensorInfos_ + i)->sensorId = static_cast<int32_t>(sensorList[i].GetSensorId());
        (sensorInfos_ + i)->sensorTypeId = static_cast<int32_t>(sensorList[i].GetSensorTypeId());
        (sensorInfos_ + i)->maxRange = sensorList[i].GetMaxRange();
        (sensorInfos_ + i)->precision = sensorList[i].GetResolution();
        (sensorInfos_ + i)->power = sensorList[i].GetPower();
        (sensorInfos_ + i)->minSamplePeriod = sensorList[i].GetMinSamplePeriodNs();
        (sensorInfos_ + i)->maxSamplePeriod = sensorList[i].GetMaxSamplePeriodNs();
    }
    sensorInfoCount_ = static_cast<int32_t>(count);
    return SUCCESS;
}

int32_t SensorAgentProxy::GetAllSensors(SensorInfo **sensorInfo, int32_t *count) const
{
    CALL_LOG_ENTER;
    CHKPR(sensorInfo, OHOS::Sensors::ERROR);
    CHKPR(count, OHOS::Sensors::ERROR);
    std::lock_guard<std::mutex> listLock(sensorInfoMutex_);
    if (sensorInfos_ == nullptr) {
        int32_t ret = ConvertSensorInfos();
        if (ret != SUCCESS) {
            SEN_HILOGE("convert sensor lists failed");
            ClearSensorInfos();
            return ERROR;
        }
    }
    *sensorInfo = sensorInfos_;
    *count = sensorInfoCount_;
    return SUCCESS;
}
}  // namespace Sensors
}  // namespace OHOS