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

#include "sensor_agent_proxy.h"

#include <cstring>

#include "securec.h"
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
std::recursive_mutex SensorAgentProxy::subscribeMutex_;
std::mutex SensorAgentProxy::chanelMutex_;
std::mutex sensorInfoMutex_;
SensorInfo *sensorInfos_ = nullptr;
std::mutex sensorActiveInfoMutex_;
SensorActiveInfo *sensorActiveInfos_ = nullptr;
int32_t sensorInfoCount_ = 0;

SensorAgentProxy::SensorAgentProxy()
    : dataChannel_(new (std::nothrow) SensorDataChannel())
{}

SensorAgentProxy::~SensorAgentProxy()
{
    CALL_LOG_ENTER;
    ClearSensorInfos();
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
            SEN_HILOGE("Sensor is not subscribed");
            return;
        }
        const SensorUser *user = iter->second;
        CHKPV(user);
        user->callback(&eventStream);
    }
}

int32_t SensorAgentProxy::CreateSensorDataChannel()
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> chanelLock(chanelMutex_);
    if (g_isChannelCreated) {
        SEN_HILOGI("The channel has already been created");
        return ERR_OK;
    }
    CHKPR(dataChannel_, INVALID_POINTER);
    auto ret = dataChannel_->CreateSensorDataChannel(std::bind(&SensorAgentProxy::HandleSensorData,
        this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), nullptr);
    if (ret != ERR_OK) {
        SEN_HILOGE("Create data channel failed, ret:%{public}d", ret);
        return ret;
    }
    ret = SenClient.TransferDataChannel(dataChannel_);
    if (ret != ERR_OK) {
        auto destroyRet = dataChannel_->DestroySensorDataChannel();
        SEN_HILOGE("Transfer data channel failed, ret:%{public}d, destroyRet:%{public}d", ret, destroyRet);
        return ret;
    }
    g_isChannelCreated = true;
    return ERR_OK;
}

int32_t SensorAgentProxy::DestroySensorDataChannel()
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> chanelLock(chanelMutex_);
    if (!g_isChannelCreated) {
        SEN_HILOGI("Channel has been destroyed");
        return ERR_OK;
    }
    CHKPR(dataChannel_, INVALID_POINTER);
    int32_t ret = dataChannel_->DestroySensorDataChannel();
    if (ret != ERR_OK) {
        SEN_HILOGE("Destroy data channel failed, ret:%{public}d", ret);
        return ret;
    }
    ret = SenClient.DestroyDataChannel();
    if (ret != ERR_OK) {
        SEN_HILOGE("Destroy service data channel fail, ret:%{public}d", ret);
        return ret;
    }
    g_isChannelCreated = false;
    return ERR_OK;
}

int32_t SensorAgentProxy::ActivateSensor(int32_t sensorId, const SensorUser *user)
{
    CHKPR(user, OHOS::Sensors::ERROR);
    CHKPR(user->callback, OHOS::Sensors::ERROR);
    if (g_samplingInterval < 0 || g_reportInterval < 0) {
        SEN_HILOGE("SamplingPeriod or g_reportInterval is invalid");
        return ERROR;
    }
    if (!SenClient.IsValid(sensorId)) {
        SEN_HILOGE("sensorId is invalid, %{public}d", sensorId);
        return PARAMETER_ERROR;
    }
    std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
    if ((g_subscribeMap.find(sensorId) == g_subscribeMap.end()) || (g_subscribeMap[sensorId] != user)) {
        SEN_HILOGE("Subscribe sensorId first");
        return ERROR;
    }
    int32_t ret = SenClient.EnableSensor(sensorId, g_samplingInterval, g_reportInterval);
    g_samplingInterval = -1;
    g_reportInterval = -1;
    if (ret != 0) {
        SEN_HILOGE("Enable sensor failed, ret:%{public}d", ret);
        g_subscribeMap.erase(sensorId);
        return ret;
    }
    return ret;
}

int32_t SensorAgentProxy::DeactivateSensor(int32_t sensorId, const SensorUser *user)
{
    CHKPR(user, OHOS::Sensors::ERROR);
    CHKPR(user->callback, OHOS::Sensors::ERROR);
    if (!SenClient.IsValid(sensorId)) {
        SEN_HILOGE("sensorId is invalid, %{public}d", sensorId);
        return PARAMETER_ERROR;
    }
    std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
    if ((g_subscribeMap.find(sensorId) == g_subscribeMap.end()) || (g_subscribeMap[sensorId] != user)) {
        SEN_HILOGE("Subscribe sensorId first");
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
                                   int64_t reportInterval)
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
        SEN_HILOGE("Subscribe sensorId first");
        return OHOS::Sensors::ERROR;
    }
    g_samplingInterval = samplingInterval;
    g_reportInterval = reportInterval;
    return OHOS::Sensors::SUCCESS;
}

int32_t SensorAgentProxy::SubscribeSensor(int32_t sensorId, const SensorUser *user)
{
    SEN_HILOGI("In, sensorId:%{public}d", sensorId);
    CHKPR(user, OHOS::Sensors::ERROR);
    CHKPR(user->callback, OHOS::Sensors::ERROR);
    if (!SenClient.IsValid(sensorId)) {
        SEN_HILOGE("sensorId is invalid, %{public}d", sensorId);
        return PARAMETER_ERROR;
    }
    int32_t ret = CreateSensorDataChannel();
    if (ret != ERR_OK) {
        SEN_HILOGE("Create sensor data chanel failed");
        return OHOS::Sensors::ERROR;
    }
    std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
    g_subscribeMap[sensorId] = user;
    return OHOS::Sensors::SUCCESS;
}

int32_t SensorAgentProxy::UnsubscribeSensor(int32_t sensorId, const SensorUser *user)
{
    SEN_HILOGI("In, sensorId:%{public}d", sensorId);
    CHKPR(user, OHOS::Sensors::ERROR);
    CHKPR(user->callback, OHOS::Sensors::ERROR);
    if (!SenClient.IsValid(sensorId)) {
        SEN_HILOGE("sensorId is invalid, %{public}d", sensorId);
        return PARAMETER_ERROR;
    }
    std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
    if (g_unsubscribeMap.find(sensorId) == g_unsubscribeMap.end() || g_unsubscribeMap[sensorId] != user) {
        SEN_HILOGE("Deactivate sensorId first");
        return OHOS::Sensors::ERROR;
    }
    if (g_subscribeMap.empty()) {
        int32_t ret = DestroySensorDataChannel();
        if (ret != ERR_OK) {
            SEN_HILOGE("Destroy data channel fail, ret:%{public}d", ret);
            return ret;
        }
    }
    g_unsubscribeMap.erase(sensorId);
    return OHOS::Sensors::SUCCESS;
}

int32_t SensorAgentProxy::SetMode(int32_t sensorId, const SensorUser *user, int32_t mode)
{
    CHKPR(user, OHOS::Sensors::ERROR);
    CHKPR(user->callback, OHOS::Sensors::ERROR);
    if (!SenClient.IsValid(sensorId)) {
        SEN_HILOGE("sensorId is invalid, %{public}d", sensorId);
        return ERROR;
    }
    std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
    if ((g_subscribeMap.find(sensorId) == g_subscribeMap.end()) || (g_subscribeMap.at(sensorId) != user)) {
        SEN_HILOGE("Subscribe sensorId first");
        return OHOS::Sensors::ERROR;
    }
    return OHOS::Sensors::SUCCESS;
}

void SensorAgentProxy::ClearSensorInfos() const
{
    if (sensorActiveInfos_ != nullptr) {
        free(sensorActiveInfos_);
        sensorActiveInfos_ = nullptr;
    }
    CHKPV(sensorInfos_);
    free(sensorInfos_);
    sensorInfos_ = nullptr;
}

int32_t SensorAgentProxy::ConvertSensorInfos() const
{
    CALL_LOG_ENTER;
    std::vector<Sensor> sensorList = SenClient.GetSensorList();
    if (sensorList.empty()) {
        SEN_HILOGE("Get sensor lists failed");
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
        SensorInfo *sensorInfo = sensorInfos_ + i;
        errno_t ret = strcpy_s(sensorInfo->sensorName, NAME_MAX_LEN,
            sensorList[i].GetSensorName().c_str());
        CHKCR(ret == EOK, ERROR);
        ret = strcpy_s(sensorInfo->vendorName, NAME_MAX_LEN,
            sensorList[i].GetVendorName().c_str());
        CHKCR(ret == EOK, ERROR);
        ret = strcpy_s(sensorInfo->hardwareVersion, VERSION_MAX_LEN,
            sensorList[i].GetHardwareVersion().c_str());
        CHKCR(ret == EOK, ERROR);
        ret = strcpy_s(sensorInfo->firmwareVersion, VERSION_MAX_LEN,
            sensorList[i].GetFirmwareVersion().c_str());
        CHKCR(ret == EOK, ERROR);
        sensorInfo->sensorId = static_cast<int32_t>(sensorList[i].GetSensorId());
        sensorInfo->sensorTypeId = static_cast<int32_t>(sensorList[i].GetSensorTypeId());
        sensorInfo->maxRange = sensorList[i].GetMaxRange();
        sensorInfo->precision = sensorList[i].GetResolution();
        sensorInfo->power = sensorList[i].GetPower();
        sensorInfo->minSamplePeriod = sensorList[i].GetMinSamplePeriodNs();
        sensorInfo->maxSamplePeriod = sensorList[i].GetMaxSamplePeriodNs();
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
            SEN_HILOGE("Convert sensor lists failed");
            ClearSensorInfos();
            return ERROR;
        }
    }
    *sensorInfo = sensorInfos_;
    *count = sensorInfoCount_;
    return SUCCESS;
}

int32_t SensorAgentProxy::SuspendSensors(int32_t pid)
{
    CALL_LOG_ENTER;
    if (pid < 0) {
        SEN_HILOGE("Pid is invalid, pid:%{public}d", pid);
        return PARAMETER_ERROR;
    }
    int32_t ret = SenClient.SuspendSensors(pid);
    if (ret != ERR_OK) {
        SEN_HILOGE("Suspend sensors failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t SensorAgentProxy::ResumeSensors(int32_t pid)
{
    CALL_LOG_ENTER;
    if (pid < 0) {
        SEN_HILOGE("Pid is invalid, pid:%{public}d", pid);
        return PARAMETER_ERROR;
    }
    int32_t ret = SenClient.ResumeSensors(pid);
    if (ret != ERR_OK) {
        SEN_HILOGE("Resume sensors failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t SensorAgentProxy::GetSensorActiveInfos(int32_t pid,
    SensorActiveInfo **sensorActiveInfos, int32_t *count) const
{
    CALL_LOG_ENTER;
    if (pid < 0) {
        SEN_HILOGE("Pid is invalid, pid:%{public}d", pid);
        return PARAMETER_ERROR;
    }
    CHKPR(sensorActiveInfos, OHOS::Sensors::ERROR);
    CHKPR(count, OHOS::Sensors::ERROR);
    std::lock_guard<std::mutex> sensorActiveInfoLock(sensorActiveInfoMutex_);
    if (sensorActiveInfos_ != nullptr) {
        free(sensorActiveInfos_);
        sensorActiveInfos_ = nullptr;
    }
    std::vector<ActiveInfo> activeInfoList;
    int32_t ret = SenClient.GetActiveInfoList(pid, activeInfoList);
    if (ret != ERR_OK) {
        SEN_HILOGE("Get active info list failed, ret:%{public}d", ret);
        return ret;
    }
    if (activeInfoList.empty()) {
        SEN_HILOGD("Active info list is empty");
        *sensorActiveInfos = nullptr;
        *count = 0;
        return ERR_OK;
    }
    size_t activeInfoCount = activeInfoList.size();
    if (activeInfoCount > MAX_SENSOR_LIST_SIZE) {
        SEN_HILOGE("The number of active info exceeds the maximum value, count:%{public}zu", activeInfoCount);
        return ERROR;
    }
    sensorActiveInfos_ = (SensorActiveInfo *)malloc(sizeof(SensorActiveInfo) * activeInfoCount);
    CHKPR(sensorActiveInfos_, ERROR);
    for (size_t i = 0; i < activeInfoCount; ++i) {
        SensorActiveInfo *curActiveInfo = sensorActiveInfos_ + i;
        curActiveInfo->pid = activeInfoList[i].GetPid();
        curActiveInfo->sensorId = activeInfoList[i].GetSensorId();
        curActiveInfo->samplingPeriodNs = activeInfoList[i].GetSamplingPeriodNs();
        curActiveInfo->maxReportDelayNs = activeInfoList[i].GetMaxReportDelayNs();
    }
    *sensorActiveInfos = sensorActiveInfos_;
    *count = static_cast<int32_t>(activeInfoCount);
    return ERR_OK;
}

int32_t SensorAgentProxy::Register(SensorActiveInfoCB callback)
{
    CHKPR(callback, OHOS::Sensors::ERROR);
    CHKPR(dataChannel_, INVALID_POINTER);
    int32_t ret = SenClient.Register(callback, dataChannel_);
    if (ret != ERR_OK) {
        SEN_HILOGE("Register sensor active info callback failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t SensorAgentProxy::Unregister(SensorActiveInfoCB callback)
{
    CHKPR(callback, OHOS::Sensors::ERROR);
    int32_t ret = SenClient.Unregister(callback);
    if (ret != ERR_OK) {
        SEN_HILOGE("Unregister sensor active info callback failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t SensorAgentProxy::ResetSensors() const
{
    int32_t ret = SenClient.ResetSensors();
    if (ret != ERR_OK) {
        SEN_HILOGE("Reset sensors failed, ret:%{public}d", ret);
    }
    return ret;
}
}  // namespace Sensors
}  // namespace OHOS