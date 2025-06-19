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

#include "print_sensor_data.h"
#include "sensor_service_client.h"
#include "sensor_xcollie.h"
#undef LOG_TAG
#define LOG_TAG "SensorAgentProxy"
using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace Sensors {
namespace {
constexpr uint32_t MAX_SENSOR_LIST_SIZE = 0Xffff;
constexpr int32_t IS_LOCAL_DEVICE = 1;
constexpr int32_t SENSOR_ONLINE = 1;
std::mutex sensorInfoMutex_;
SensorInfoCheck sensorInfoCheck_;
std::mutex sensorActiveInfoMutex_;
SensorActiveInfo *sensorActiveInfos_ = nullptr;
int32_t sensorInfoCount_ = 0;
} // namespace

#define SEN_CLIENT SensorServiceClient::GetInstance()
std::recursive_mutex SensorAgentProxy::subscribeMutex_;
std::recursive_mutex SensorAgentProxy::subscribePlugMutex_;
std::mutex SensorAgentProxy::chanelMutex_;
std::mutex SensorAgentProxy::createChannelMutex_;

SensorAgentProxy::SensorAgentProxy()
    : dataChannel_(new (std::nothrow) SensorDataChannel())
{}

SensorAgentProxy::~SensorAgentProxy()
{
    CALL_LOG_ENTER;
    ClearSensorInfos();
}

std::set<RecordSensorCallback> SensorAgentProxy::GetSubscribeUserCallback(const SensorDescription &sensorDesc)
{
    std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
    auto iter = subscribeMap_.find(sensorDesc);
    if (iter == subscribeMap_.end()) {
        SEN_HILOGE("Sensor is not subscribed");
        return {};
    }
    std::set<RecordSensorCallback> callback;
    for (const auto &it : iter->second) {
        auto ret = callback.insert(it->callback);
        if (!ret.second) {
            SEN_HILOGE("callback insert fail");
        }
    }
    return callback;
}

void SensorAgentProxy::HandleSensorData(SensorEvent *events,
    int32_t num, void *data) __attribute__((no_sanitize("cfi")))
{
    CHKPV(events);
    if (num <= 0) {
        SEN_HILOGE("events is null or num is invalid");
        return;
    }
    SensorEvent eventStream;
    for (int32_t i = 0; i < num; ++i) {
        eventStream = events[i];
        auto callbacks = GetSubscribeUserCallback({eventStream.deviceId, eventStream.sensorTypeId,
            eventStream.sensorId, eventStream.location});
        for (const auto &callback : callbacks) {
            CHKPV(callback);
            if (eventStream.sensorTypeId == SENSOR_TYPE_ID_HALL_EXT) {
                PrintSensorData::GetInstance().ControlSensorClientPrint(callback, eventStream);
            }
            callback(&eventStream);
            PrintSensorData::GetInstance().ControlSensorClientPrint(callback, eventStream);
        }
    }
}

void SensorAgentProxy::SetIsChannelCreated(bool isChannelCreated)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> chanelLock(chanelMutex_);
    isChannelCreated_ = isChannelCreated;
}

int32_t SensorAgentProxy::CreateSensorDataChannel()
{
    SEN_HILOGI("In");
    std::lock_guard<std::mutex> chanelLock(chanelMutex_);
    if (isChannelCreated_) {
        SEN_HILOGI("The channel has already been created");
        return ERR_OK;
    }
    CHKPR(dataChannel_, INVALID_POINTER);
    auto ret = dataChannel_->CreateSensorDataChannel(
        [weakSelf = std::weak_ptr<SensorAgentProxy>(SENSOR_AGENT_IMPL)](SensorEvent *events, int32_t num, void *data) {
        if (auto sharedSelf = weakSelf.lock()) {
            sharedSelf->HandleSensorData(events, num, data);
        } else {
            SEN_HILOGD("SensorAgentProxy object has been released");
        }
    }, nullptr);
    if (ret != ERR_OK) {
        SEN_HILOGE("Create data channel failed, ret:%{public}d", ret);
        return ret;
    }
    {
        SensorXcollie SensorXcollie("SensorAgentProxy:TransferDataChannel", XCOLLIE_TIMEOUT_5S);
        ret = SEN_CLIENT.TransferDataChannel(dataChannel_);
    }
    if (ret != ERR_OK) {
        auto destroyRet = dataChannel_->DestroySensorDataChannel();
        SEN_HILOGE("Transfer data channel failed, ret:%{public}d, destroyRet:%{public}d", ret, destroyRet);
        return ret;
    }
    isChannelCreated_ = true;
    SEN_HILOGI("Done");
    return ERR_OK;
}

int32_t SensorAgentProxy::DestroySensorDataChannel()
{
    SEN_HILOGI("In");
    std::lock_guard<std::mutex> chanelLock(chanelMutex_);
    if (!isChannelCreated_) {
        SEN_HILOGI("Channel has been destroyed");
        return ERR_OK;
    }
    CHKPR(dataChannel_, INVALID_POINTER);
    int32_t ret = dataChannel_->DestroySensorDataChannel();
    if (ret != ERR_OK) {
        SEN_HILOGE("Destroy data channel failed, ret:%{public}d", ret);
        return ret;
    }
    {
        SensorXcollie SensorXcollie("SensorAgentProxy:DestroyDataChannel", XCOLLIE_TIMEOUT_5S);
        ret = SEN_CLIENT.DestroyDataChannel();
    }
    if (ret != ERR_OK) {
        SEN_HILOGE("Destroy service data channel fail, ret:%{public}d", ret);
        return ret;
    }
    isChannelCreated_ = false;
    SEN_HILOGI("Done");
    return ERR_OK;
}

int32_t SensorAgentProxy::ActivateSensor(const SensorDescription &sensorDesc, const SensorUser *user)
{
    SEN_HILOGI("In, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    CHKPR(user, OHOS::Sensors::ERROR);
    CHKPR(user->callback, OHOS::Sensors::ERROR);
    if (samplingInterval_ < 0 || reportInterval_ < 0) {
        SEN_HILOGE("SamplingPeriod or reportInterval_ is invalid");
        return ERROR;
    }
    if (!SEN_CLIENT.IsValid(sensorDesc)) {
        SEN_HILOGE("sensorDesc is invalid, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
            sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
        return PARAMETER_ERROR;
    }
    std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
    if (subscribeMap_.find(sensorDesc) == subscribeMap_.end()) {
        SEN_HILOGE("Subscribe sensorId first");
        return ERROR;
    }
    auto& subscribeSet = subscribeMap_[sensorDesc];
    if (subscribeSet.find(user) == subscribeSet.end()) {
        SEN_HILOGE("Subscribe user first");
        return ERROR;
    }
    int32_t ret = 0;
    {
        SensorXcollie SensorXcollie("SensorAgentProxy:EnableSensor", XCOLLIE_TIMEOUT_15S);
        ret = SEN_CLIENT.EnableSensor(sensorDesc, samplingInterval_, reportInterval_);
    }
    if (ret != 0) {
        SEN_HILOGE("Enable sensor failed, ret:%{public}d", ret);
        subscribeSet.erase(user);
        if (subscribeSet.empty()) {
            subscribeMap_.erase(sensorDesc);
        }
        return ret;
    }
    SEN_HILOGI("Done, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    return ret;
}

int32_t SensorAgentProxy::DeactivateSensor(const SensorDescription &sensorDesc, const SensorUser *user)
{
    SEN_HILOGI("In, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d, location:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId, sensorDesc.location);
    CHKPR(user, OHOS::Sensors::ERROR);
    CHKPR(user->callback, OHOS::Sensors::ERROR);
    std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
    if (!(SEN_CLIENT.IsValid(sensorDesc) ||
        ((!SEN_CLIENT.IsValid(sensorDesc)) && subscribeMap_.find(sensorDesc) != subscribeMap_.end()))) {
        SEN_HILOGE("sensorDesc is invalid, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
            sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
        return PARAMETER_ERROR;
    }
    if (subscribeMap_.find(sensorDesc) == subscribeMap_.end()) {
        SEN_HILOGE("Subscribe sensorId first");
        return OHOS::Sensors::ERROR;
    }
    auto& subscribeSet = subscribeMap_[sensorDesc];
    if (subscribeSet.find(user) == subscribeSet.end()) {
        SEN_HILOGE("Subscribe user first");
        return OHOS::Sensors::ERROR;
    }
    auto status = unsubscribeMap_[sensorDesc].insert(user);
    if (!status.second) {
        SEN_HILOGE("User has been unsubscribed");
    }
    subscribeSet.erase(user);
    if (subscribeSet.empty()) {
        subscribeMap_.erase(sensorDesc);
        int32_t ret = 0;
        {
            if (!SEN_CLIENT.IsValid(sensorDesc)) {
                SEN_HILOGW("No need to call DisableSensor");
                return OHOS::Sensors::SUCCESS;
            }
            SensorXcollie SensorXcollie("SensorAgentProxy:DisableSensor", XCOLLIE_TIMEOUT_15S);
            ret = SEN_CLIENT.DisableSensor(sensorDesc);
        }
        if (ret != 0) {
            SEN_HILOGE("DisableSensor failed, ret:%{public}d", ret);
            return ret;
        }
    }
    SEN_HILOGI("Done, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    return OHOS::Sensors::SUCCESS;
}

int32_t SensorAgentProxy::SetBatch(const SensorDescription &sensorDesc, const SensorUser *user,
    int64_t samplingInterval, int64_t reportInterval)
{
    CHKPR(user, OHOS::Sensors::ERROR);
    if (!SEN_CLIENT.IsValid(sensorDesc)) {
        SEN_HILOGE("sensorDesc is invalid, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
            sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
        return PARAMETER_ERROR;
    }
    if (samplingInterval < 0 || reportInterval < 0) {
        SEN_HILOGE("samplingInterval or reportInterval is invalid");
        return OHOS::Sensors::ERROR;
    }
    std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
    if (subscribeMap_.find(sensorDesc) == subscribeMap_.end()) {
        SEN_HILOGE("Subscribe sensorId first");
        return OHOS::Sensors::ERROR;
    }
    auto& subscribeSet = subscribeMap_[sensorDesc];
    if (subscribeSet.find(user) == subscribeSet.end()) {
        SEN_HILOGE("Subscribe user first");
        return OHOS::Sensors::ERROR;
    }
    samplingInterval_ = samplingInterval;
    reportInterval_ = reportInterval;
    return OHOS::Sensors::SUCCESS;
}

int32_t SensorAgentProxy::SubscribeSensor(const SensorDescription &sensorDesc, const SensorUser *user)
{
    SEN_HILOGI("In, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    CHKPR(user, OHOS::Sensors::ERROR);
    CHKPR(user->callback, OHOS::Sensors::ERROR);
    if (!SEN_CLIENT.IsValid(sensorDesc)) {
        SEN_HILOGE("sensorDesc is invalid, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
            sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
        return PARAMETER_ERROR;
    }
    std::lock_guard<std::mutex> createChannelLock(createChannelMutex_);
    int32_t ret = CreateSensorDataChannel();
    if (ret != ERR_OK) {
        SEN_HILOGE("Create sensor data chanel failed");
        return OHOS::Sensors::ERROR;
    }
    std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
    auto status = subscribeMap_[sensorDesc].insert(user);
    if (!status.second) {
        SEN_HILOGE("User has been subscribed");
    }
    if (PrintSensorData::GetInstance().IsContinuousType(sensorDesc.sensorType)) {
        PrintSensorData::GetInstance().SavePrintUserInfo(user->callback);
        PrintSensorData::GetInstance().ResetClientTimes();
    }
    SEN_HILOGI("Done, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    return OHOS::Sensors::SUCCESS;
}

bool SensorAgentProxy::IsSubscribeMapEmpty() const
{
    std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
    return subscribeMap_.empty();
}

int32_t SensorAgentProxy::UnsubscribeSensor(const SensorDescription &sensorDesc, const SensorUser *user)
{
    SEN_HILOGI("In, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    CHKPR(user, OHOS::Sensors::ERROR);
    CHKPR(user->callback, OHOS::Sensors::ERROR);
    {
        std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
        if (!(SEN_CLIENT.IsValid(sensorDesc) ||
            ((!SEN_CLIENT.IsValid(sensorDesc)) && unsubscribeMap_.find(sensorDesc) != unsubscribeMap_.end()))) {
            SEN_HILOGE("sensorDesc is invalid, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
                sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
            return PARAMETER_ERROR;
        }
        if (unsubscribeMap_.find(sensorDesc) == unsubscribeMap_.end()) {
            SEN_HILOGE("Deactivate sensorId first");
            return OHOS::Sensors::ERROR;
        }
        auto& unsubscribeSet = unsubscribeMap_[sensorDesc];
        if (unsubscribeSet.find(user) == unsubscribeSet.end()) {
            SEN_HILOGE("Deactivate user first");
            return OHOS::Sensors::ERROR;
        }
        unsubscribeSet.erase(user);
        if (unsubscribeSet.empty()) {
            unsubscribeMap_.erase(sensorDesc);
        }
    }
    std::lock_guard<std::mutex> createChannelLock(createChannelMutex_);
    if (IsSubscribeMapEmpty()) {
        int32_t ret = DestroySensorDataChannel();
        if (ret != ERR_OK) {
            SEN_HILOGE("Destroy data channel fail, ret:%{public}d", ret);
            return ret;
        }
    }
    if (PrintSensorData::GetInstance().IsContinuousType(sensorDesc.sensorType)) {
        PrintSensorData::GetInstance().RemovePrintUserInfo(user->callback);
        PrintSensorData::GetInstance().ResetClientTimes();
    }
    SEN_HILOGI("Done, deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
        sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
    return OHOS::Sensors::SUCCESS;
}

int32_t SensorAgentProxy::SetMode(const SensorDescription &sensorDesc, const SensorUser *user, int32_t mode)
{
    CHKPR(user, OHOS::Sensors::ERROR);
    CHKPR(user->callback, OHOS::Sensors::ERROR);
    if (!SEN_CLIENT.IsValid(sensorDesc)) {
        SEN_HILOGE("sensorDesc is invalid,deviceId:%{public}d, sensortypeId:%{public}d, sensorId:%{public}d",
            sensorDesc.deviceId, sensorDesc.sensorType, sensorDesc.sensorId);
        return ERROR;
    }
    std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
    if (subscribeMap_.find(sensorDesc) == subscribeMap_.end()) {
        SEN_HILOGE("Subscribe sensorId first");
        return OHOS::Sensors::ERROR;
    }
    auto& subscribeSet = subscribeMap_[sensorDesc];
    if (subscribeSet.find(user) == subscribeSet.end()) {
        SEN_HILOGE("Subscribe user first");
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
    CHKPV(sensorInfoCheck_.sensorInfos);
    free(sensorInfoCheck_.sensorInfos);
    sensorInfoCheck_.sensorInfos = nullptr;
    sensorInfoCheck_.checkCode = CHECK_CODE;
    sensorInfoCount_ = 0;
}

int32_t SensorAgentProxy::ConvertSensorInfos() const
{
    CALL_LOG_ENTER;
    std::vector<Sensor> sensorList;
    {
        SensorXcollie SensorXcollie("SensorAgentProxy:GetSensorList", XCOLLIE_TIMEOUT_5S);
        sensorList = SEN_CLIENT.GetSensorList();
    }
    if (sensorList.empty()) {
        SEN_HILOGE("Get sensor lists failed");
        return ERROR;
    }
    size_t count = sensorList.size();
    if (count > MAX_SENSOR_LIST_SIZE) {
        SEN_HILOGE("The number of sensors exceeds the maximum value");
        return ERROR;
    }
    if (sensorInfoCount_ > 0 && sensorInfoCount_ == static_cast<int32_t>(count)) {
        return SUCCESS;
    } else if (sensorInfoCount_ > 0 && sensorInfoCount_ != static_cast<int32_t>(count) &&
        sensorInfoCheck_.checkCode == CHECK_CODE) {
        SEN_HILOGW("sensorInfos_ error, sensorInfoCount_:%{public}d, sensorListCount:%{public}d", sensorInfoCount_,
            static_cast<int32_t>(count));
        ClearSensorInfos();
    } else if (sensorInfoCheck_.checkCode != CHECK_CODE) {
        SEN_HILOGE("CheckCode has been modified, %{public}d", sensorInfoCheck_.checkCode);
        ClearSensorInfos();
    }
    sensorInfoCheck_.sensorInfos = (SensorInfo *)malloc(sizeof(SensorInfo) * count);
    CHKPR(sensorInfoCheck_.sensorInfos, ERROR);
    SEN_HILOGI("Sensor count is %{public}zu", count);
    for (size_t i = 0; i < count; ++i) {
        SensorInfo *sensorInfo = sensorInfoCheck_.sensorInfos + i;
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
        sensorInfo->deviceId = sensorList[i].GetDeviceId();
        sensorInfo->sensorId = sensorList[i].GetSensorTypeId();
        sensorInfo->sensorTypeId = sensorList[i].GetSensorTypeId();
        sensorInfo->sensorIndex = sensorList[i].GetSensorId();
        sensorInfo->location = sensorList[i].GetLocation();
        sensorInfo->maxRange = sensorList[i].GetMaxRange();
        sensorInfo->precision = sensorList[i].GetResolution();
        sensorInfo->power = sensorList[i].GetPower();
        sensorInfo->minSamplePeriod = sensorList[i].GetMinSamplePeriodNs();
        sensorInfo->maxSamplePeriod = sensorList[i].GetMaxSamplePeriodNs();
        SEN_HILOGI("deviceId %{public}d: sensorTypeId %{public}d, sensorIndex %{public}d",
            sensorInfo->deviceId, sensorInfo->sensorTypeId, sensorInfo->sensorIndex);
    }
    sensorInfoCount_ = static_cast<int32_t>(count);
    return SUCCESS;
}

int32_t SensorAgentProxy::GetDeviceSensors(int32_t deviceId, SensorInfo **singleDevSensorInfo, int32_t *count)
{
    CALL_LOG_ENTER;
    CHKPR(singleDevSensorInfo, OHOS::Sensors::ERROR);
    CHKPR(count, OHOS::Sensors::ERROR);
    std::lock_guard<std::mutex> listLock(sensorInfoMutex_);
    if (sensorInfoCheck_.sensorInfos == nullptr) {
        int32_t ret = ConvertSensorInfos();
        if (ret != ERR_OK) {
            SEN_HILOGE("Convert sensor lists failed");
            ClearSensorInfos();
            return ERROR;
        }
    }
    std::vector<Sensor> singleDevSensors;
    SensorXcollie SensorXcollie("SensorAgentProxy:ConvertSensorInfosByDevice", XCOLLIE_TIMEOUT_5S);
    int32_t ret = SEN_CLIENT.GetSensorListByDevice(deviceId, singleDevSensors);
    if (ret != ERR_OK || singleDevSensors.empty()) {
        SEN_HILOGE("Get device sensor lists failed");
        return ERROR;
    }
    ret = UpdateSensorInfosCache(singleDevSensors);
    if (ret != ERR_OK) {
        SEN_HILOGE("Update sensor infos cache failed");
        return ERROR;
    }
    std::vector<SensorInfo> filteredSensorInfos;
    for (int32_t i = 0; i < sensorInfoCount_; ++i) {
        if (sensorInfoCheck_.sensorInfos[i].deviceId == deviceId) {
            filteredSensorInfos.push_back(sensorInfoCheck_.sensorInfos[i]);
        }
    }
    int32_t filteredCount = static_cast<int32_t>(filteredSensorInfos.size());
    auto newSensorInfo = std::make_unique<SensorInfo[]>(filteredCount);
    if (!newSensorInfo) {
        SEN_HILOGE("Failed to allocate memory for filtered sensorInfos");
        return ERROR;
    }
    for (int32_t i = 0; i < filteredCount; ++i) {
        newSensorInfo[i] = filteredSensorInfos[i];
    }
    free(*singleDevSensorInfo);
    *singleDevSensorInfo = newSensorInfo.release();
    *count = filteredCount;
    return SUCCESS;
}

int32_t SensorAgentProxy::UpdateSensorInfo(SensorInfo* sensorInfo, const Sensor& sensor)
{
    CALL_LOG_ENTER;
    CHKPR(sensorInfo, OHOS::Sensors::ERROR);
    errno_t ret = strcpy_s(sensorInfo->sensorName, NAME_MAX_LEN,
        sensor.GetSensorName().c_str());
    CHKCR(ret == EOK, ERROR);
    ret = strcpy_s(sensorInfo->vendorName, NAME_MAX_LEN,
        sensor.GetVendorName().c_str());
    CHKCR(ret == EOK, ERROR);
    ret = strcpy_s(sensorInfo->hardwareVersion, VERSION_MAX_LEN,
        sensor.GetHardwareVersion().c_str());
    CHKCR(ret == EOK, ERROR);
    ret = strcpy_s(sensorInfo->firmwareVersion, VERSION_MAX_LEN,
        sensor.GetFirmwareVersion().c_str());
    CHKCR(ret == EOK, ERROR);
    sensorInfo->deviceId = sensor.GetDeviceId();
    sensorInfo->sensorId = sensor.GetSensorTypeId();
    sensorInfo->sensorTypeId = sensor.GetSensorTypeId();
    sensorInfo->sensorIndex = sensor.GetSensorId();
    sensorInfo->location = sensor.GetLocation();
    sensorInfo->maxRange = sensor.GetMaxRange();
    sensorInfo->precision = sensor.GetResolution();
    sensorInfo->power = sensor.GetPower();
    sensorInfo->minSamplePeriod = sensor.GetMinSamplePeriodNs();
    sensorInfo->maxSamplePeriod = sensor.GetMaxSamplePeriodNs();
    return SUCCESS;
}

bool SensorAgentProxy::FindSensorInfo(int32_t deviceId, int32_t sensorIndex, int32_t sensorTypeId)
{
    CALL_LOG_ENTER;
    if (sensorInfoCheck_.sensorInfos == nullptr) {
        return false;
    }
    for (int32_t i = 0; i < sensorInfoCount_; ++i) {
        if (sensorInfoCheck_.sensorInfos[i].deviceId == deviceId &&
            sensorInfoCheck_.sensorInfos[i].sensorIndex == sensorIndex &&
            sensorInfoCheck_.sensorInfos[i].sensorTypeId == sensorTypeId) {
            SEN_HILOGI("FindSensorInfo deviceId:%{public}d, sensorTypeId:%{public}d, sensorId:%{public}d",
                deviceId, sensorTypeId, sensorIndex);
            return true;
        }
    }
    return false;
}

int32_t SensorAgentProxy::UpdateSensorInfosCache(const std::vector<Sensor>& singleDevSensors)
{
    CALL_LOG_ENTER;
    size_t newSensorsCount = 0;
    for (const auto& sensor : singleDevSensors) {
        if (!FindSensorInfo(sensor.GetDeviceId(), sensor.GetSensorId(), sensor.GetSensorTypeId())) {
            newSensorsCount++;
        }
    }
    if (newSensorsCount == 0) {
        return SUCCESS;
    }
    size_t newTotalCount = sensorInfoCount_ + newSensorsCount;
    if (newTotalCount > MAX_SENSOR_LIST_SIZE) {
        SEN_HILOGE("The number of sensors exceeds the maximum value");
        return ERROR;
    }
    std::unique_ptr<SensorInfo[]> newSensorInfos = std::make_unique<SensorInfo[]>(newTotalCount);
    if (newSensorInfos == nullptr) {
        SEN_HILOGE("Failed to allocate memory for newSensorInfos");
        return ERROR;
    }
    if (sensorInfoCheck_.sensorInfos) {
        std::copy(sensorInfoCheck_.sensorInfos, sensorInfoCheck_.sensorInfos + sensorInfoCount_, newSensorInfos.get());
    }
    size_t currentIndex = sensorInfoCount_;
    for (const auto& sensor : singleDevSensors) {
        if (!FindSensorInfo(sensor.GetDeviceId(), sensor.GetSensorId(), sensor.GetSensorTypeId())) {
            UpdateSensorInfo(&newSensorInfos[currentIndex++], sensor);
        }
    }
    sensorInfoCheck_.sensorInfos = newSensorInfos.release();
    sensorInfoCount_ = static_cast<int32_t>(newTotalCount);
    return SUCCESS;
}

int32_t SensorAgentProxy::GetLocalDeviceId(int32_t& deviceId) const
{
    CALL_LOG_ENTER;
    return SEN_CLIENT.GetLocalDeviceId(deviceId);
}

int32_t SensorAgentProxy::GetAllSensors(SensorInfo **sensorInfo, int32_t *count) const
{
    CALL_LOG_ENTER;
    CHKPR(sensorInfo, OHOS::Sensors::ERROR);
    CHKPR(count, OHOS::Sensors::ERROR);
    std::lock_guard<std::mutex> listLock(sensorInfoMutex_);
    int32_t ret = ConvertSensorInfos();
    if (ret != SUCCESS) {
        SEN_HILOGE("Convert sensor lists failed");
        ClearSensorInfos();
        return ERROR;
    }
    if (sensorInfoCheck_.checkCode != CHECK_CODE) {
        SEN_HILOGE("CheckCode has been modified, %{public}d", sensorInfoCheck_.checkCode);
        ClearSensorInfos();
        return ERROR;
    }
    CHKPR(sensorInfoCheck_.sensorInfos, OHOS::Sensors::ERROR);
    *sensorInfo = sensorInfoCheck_.sensorInfos;
    *count = sensorInfoCount_;
    PrintSensorData::GetInstance().PrintSensorInfo(sensorInfoCheck_.sensorInfos, sensorInfoCount_);
    return SUCCESS;
}

int32_t SensorAgentProxy::SuspendSensors(int32_t pid)
{
    CALL_LOG_ENTER;
    if (pid < 0) {
        SEN_HILOGE("Pid is invalid, pid:%{public}d", pid);
        return PARAMETER_ERROR;
    }
    int32_t ret = 0;
    {
        SensorXcollie SensorXcollie("SensorAgentProxy:SuspendSensors", XCOLLIE_TIMEOUT_5S);
        ret = SEN_CLIENT.SuspendSensors(pid);
    }
    if (ret != ERR_OK) {
        SEN_HILOGD("Suspend sensors failed, ret:%{public}d", ret);
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
    int32_t ret = 0;
    {
        SensorXcollie SensorXcollie("SensorAgentProxy:ResumeSensors", XCOLLIE_TIMEOUT_5S);
        ret = SEN_CLIENT.ResumeSensors(pid);
    }
    if (ret != ERR_OK) {
        SEN_HILOGD("Resume sensors failed, ret:%{public}d", ret);
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
    int32_t ret = 0;
    {
        SensorXcollie SensorXcollie("SensorAgentProxy:GetActiveInfoList", XCOLLIE_TIMEOUT_5S);
        ret = SEN_CLIENT.GetActiveInfoList(pid, activeInfoList);
    }
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
    int32_t ret = 0;
    {
        SensorXcollie SensorXcollie("SensorAgentProxy:Register", XCOLLIE_TIMEOUT_5S);
        ret = SEN_CLIENT.Register(callback, dataChannel_);
    }
    if (ret != ERR_OK) {
        SEN_HILOGE("Register sensor active info callback failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t SensorAgentProxy::Unregister(SensorActiveInfoCB callback)
{
    CHKPR(callback, OHOS::Sensors::ERROR);
    int32_t ret = 0;
    {
        SensorXcollie SensorXcollie("SensorAgentProxy:Unregister", XCOLLIE_TIMEOUT_5S);
        ret = SEN_CLIENT.Unregister(callback);
    }
    if (ret != ERR_OK) {
        SEN_HILOGE("Unregister sensor active info callback failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t SensorAgentProxy::ResetSensors() const
{
    int32_t ret = 0;
    {
        SensorXcollie SensorXcollie("SensorAgentProxy:ResetSensors", XCOLLIE_TIMEOUT_5S);
        ret = SEN_CLIENT.ResetSensors();
    }
    if (ret != ERR_OK) {
        SEN_HILOGE("Reset sensors failed, ret:%{public}d", ret);
    }
    return ret;
}

void SensorAgentProxy::SetDeviceStatus(uint32_t deviceStatus) const
{
    SEN_HILOGI("SetDeviceStatus in, deviceStatus:%{public}d", deviceStatus);
    SEN_CLIENT.SetDeviceStatus(deviceStatus);
}

int32_t SensorAgentProxy::SubscribeSensorPlug(const SensorUser *user)
{
    CALL_LOG_ENTER;
    CHKPR(user, OHOS::Sensors::ERROR);
    CHKPR(user->plugCallback, OHOS::Sensors::ERROR);

    int32_t ret = 0;
    {
        SensorXcollie SensorXcollie("SensorAgentProxy:CreateClientRemoteObject()", XCOLLIE_TIMEOUT_5S);
        ret = SEN_CLIENT.CreateClientRemoteObject();
    }
    if (ret != ERR_OK) {
        SEN_HILOGE("CreateClientRemoteObject failed, ret:%{public}d", ret);
        return ret;
    }

    std::lock_guard<std::recursive_mutex> subscribePlugLock(subscribePlugMutex_);
    auto status = subscribeSet_.insert(user);
    if (!status.second) {
        SEN_HILOGD("User has been subscribed");
    }
    return OHOS::Sensors::SUCCESS;
}

int32_t SensorAgentProxy::UnsubscribeSensorPlug(const SensorUser *user)
{
    CALL_LOG_ENTER;
    CHKPR(user, OHOS::Sensors::ERROR);
    CHKPR(user->plugCallback, OHOS::Sensors::ERROR);

    std::lock_guard<std::recursive_mutex> subscribePlugLock(subscribePlugMutex_);
    if (subscribeSet_.find(user) == subscribeSet_.end()) {
        SEN_HILOGD("Deactivate user first");
        return OHOS::Sensors::ERROR;
    }
    subscribeSet_.erase(user);
    return OHOS::Sensors::SUCCESS;
}

void SensorAgentProxy::UpdateSensorStatusEvent(SensorStatusEvent &event, SensorPlugData info)
{
    event.sensorType = info.sensorTypeId;
    event.sensorId = info.sensorId;
    event.isSensorOnline = false;
    if (info.status == SENSOR_ONLINE) {
        event.isSensorOnline = true;
    }
    event.deviceId = info.deviceId;
    event.deviceName = info.deviceName;
    event.location = false;
    if (info.location == IS_LOCAL_DEVICE) {
        event.location = true;
    }
    event.timestamp = info.timestamp;
}

bool SensorAgentProxy::UpdateSensorInfo(SensorPlugData info)
{
    CALL_LOG_ENTER;
    if (info.status == SENSOR_ONLINE) {
        SensorInfo *sensorInfos = nullptr;
        int32_t count = 0;
        int32_t ret = GetDeviceSensors(info.deviceId, &sensorInfos, &count);
        if (ret != ERR_OK) {
            SEN_HILOGE("UpdateSensorInfo failed");
            return false;
        }
        free(sensorInfos);
        sensorInfos = nullptr;
    } else {
        {
            std::lock_guard<std::recursive_mutex> subscribeLock(subscribeMutex_);
            int32_t ret = 0;
            if (subscribeMap_.find({info.deviceId, info.sensorTypeId, info.sensorId, info.location}) !=
                subscribeMap_.end()) {
                SensorXcollie SensorXcollie("SensorAgentProxy:DisableSensor", XCOLLIE_TIMEOUT_15S);
                ret = SEN_CLIENT.DisableSensor({info.deviceId, info.sensorTypeId, info.sensorId, info.location});
            }
            if (ret != 0) {
                SEN_HILOGE("DisableSensor failed, ret:%{public}d", ret);
            }
        }
        if (!(SEN_CLIENT.EraseCacheSensorList(info))) {
            SEN_HILOGE("EraseCacheSensorList failed");
            return false;
        }
        EraseCacheSensorInfos(info);
    }
    return true;
}

void SensorAgentProxy::EraseCacheSensorInfos(SensorPlugData info)
{
    std::lock_guard<std::mutex> listLock(sensorInfoMutex_);
    for (int32_t i = 0; i < sensorInfoCount_; ++i) {
        if ((sensorInfoCheck_.sensorInfos[i].deviceId == info.deviceId) &&
            (sensorInfoCheck_.sensorInfos[i].sensorTypeId == info.sensorTypeId) &&
            (sensorInfoCheck_.sensorInfos[i].sensorIndex == info.sensorId)) {
            for (int j = i; j < sensorInfoCount_ - 1; ++j) {
                sensorInfoCheck_.sensorInfos[j] = sensorInfoCheck_.sensorInfos[j+1];
            }
            sensorInfoCount_--;
            break;
        }
    }
}

bool SensorAgentProxy::HandlePlugSensorData(const SensorPlugData &info) __attribute__((no_sanitize("cfi")))
{
    CALL_LOG_ENTER;
    if (!UpdateSensorInfo(info)) {
        return false;
    }
    std::lock_guard<std::recursive_mutex> subscribePlugLock(subscribePlugMutex_);
    SensorStatusEvent event;
    UpdateSensorStatusEvent(event, info);
    for (const auto &it : subscribeSet_) {
        if (it->plugCallback != nullptr) {
            SEN_HILOGD("plug callback is run");
            it->plugCallback(&event);
        }
    }
    return true;
}
} // namespace Sensors
} // namespace OHOS