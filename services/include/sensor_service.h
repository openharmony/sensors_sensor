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

#ifndef SENSOR_SERVICE_H
#define SENSOR_SERVICE_H

#include "common_event_manager.h"
#include "system_ability.h"

#include "death_recipient_template.h"
#include "sensor_common_event_subscriber.h"
#include "sensor_delayed_sp_singleton.h"
#include "sensor_power_policy.h"
#include "sensor_service_stub.h"
#include "stream_server.h"
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
#include "sensor_hdi_connection.h"
#endif // HDF_DRIVERS_INTERFACE_SENSOR

namespace OHOS {
namespace Sensors {
enum class SensorServiceState {
    STATE_STOPPED,
    STATE_RUNNING,
};

class SensorService : public SystemAbility, public StreamServer, public SensorServiceStub {
    DECLARE_SYSTEM_ABILITY(SensorService)
    SENSOR_DECLARE_DELAYED_SP_SINGLETON(SensorService);

public:
    void OnDump() override;
    void OnStart() override;
    void OnStop() override;
    int Dump(int fd, const std::vector<std::u16string> &args) override;
    ErrCode EnableSensor(const SensorDescriptionIPC &sensorDesc, int64_t samplingPeriodNs,
        int64_t maxReportDelayNs) override;
    ErrCode DisableSensor(const SensorDescriptionIPC &sensorDesc) override;
    ErrCode GetSensorList(std::vector<Sensor> &sensorList) override;
    ErrCode GetSensorListByDevice(int32_t deviceId, std::vector<Sensor> &sensorList) override;
    ErrCode TransferDataChannel(int32_t sendFd, const sptr<IRemoteObject> &sensorClient) override;
    ErrCode DestroySensorChannel(const sptr<IRemoteObject> &sensorClient) override;
    void ProcessDeathObserver(const wptr<IRemoteObject> &object);
    ErrCode SuspendSensors(int32_t pid) override;
    ErrCode ResumeSensors(int32_t pid) override;
    ErrCode GetActiveInfoList(int32_t pid, std::vector<ActiveInfo> &activeInfoList) override;
    ErrCode CreateSocketChannel(const sptr<IRemoteObject> &sensorClient, int32_t &clientFd) override;
    ErrCode DestroySocketChannel(const sptr<IRemoteObject> &sensorClient) override;
    ErrCode EnableActiveInfoCB() override;
    ErrCode DisableActiveInfoCB() override;
    ErrCode ResetSensors() override;
    ErrCode SetDeviceStatus(uint32_t deviceStatus) override;
    ErrCode TransferClientRemoteObject(const sptr<IRemoteObject> &sensorClient) override;
    ErrCode DestroyClientRemoteObject(const sptr<IRemoteObject> &sensorClient) override;

private:
    DISALLOW_COPY_AND_MOVE(SensorService);
    std::vector<Sensor> GetSensorList();
    std::vector<Sensor> GetSensorListByDevice(int32_t deviceId);
    void InitShakeControl();
    void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
    void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
    ErrCode CheckAuthAndParameter(const SensorDescription &sensorDesc, int64_t samplingPeriodNs,
        int64_t maxReportDelayNs);
    void ReportPlugEventCallback(const SensorPlugInfo &sensorPlugInfo);
    ErrCode SensorReportEvent(const SensorDescription &sensorDesc, int64_t samplingPeriodNs, int64_t maxReportDelayNs,
        int32_t pid);

    class PermStateChangeCb : public Security::AccessToken::PermStateChangeCallbackCustomize {
    public:
        PermStateChangeCb(const Security::AccessToken::PermStateChangeScope &scope,
            sptr<SensorService> server) : PermStateChangeCallbackCustomize(scope), server_(server) {}
        void PermStateChangeCallback(PermStateChangeInfo &result) override;

    private:
        sptr<SensorService> server_ = nullptr;
    };

    void RegisterClientDeathRecipient(sptr<IRemoteObject> sensorClient, int32_t pid);
    void UnregisterClientDeathRecipient(sptr<IRemoteObject> sensorClient);
    bool InitSensorPolicy();
    void ReportOnChangeData(const SensorDescription &sensorDesc);
    void ReportSensorSysEvent(int32_t sensorType, bool enable, int32_t pid, int64_t samplingPeriodNs = 0,
        int64_t maxReportDelayNs = 0);
    ErrCode DisableSensor(const SensorDescription &sensorDesc, int32_t pid);
    bool RegisterPermCallback(int32_t sensorType);
    void UnregisterPermCallback();
    bool CheckSensorId(const SensorDescription &sensorDesc);
    void ReportActiveInfo(const SensorDescription &sensorDesc, int32_t pid);
    bool IsSystemServiceCalling();
    bool IsSystemCalling();
    int32_t GetDeviceType();
    void SetCritical();
    void LoadMotionTransform(int32_t systemAbilityId);
    void MotionSensorRevision();
    void UpdateDeviceStatus();
    int32_t SubscribeCommonEvent(const std::string &eventName, EventReceiver receiver);
    void OnReceiveEvent(const EventFwk::CommonEventData &data);
    void OnReceiveUserSwitchEvent(const EventFwk::CommonEventData &data);
    void OnReceiveBootEvent(const EventFwk::CommonEventData &data);
    bool LoadSecurityPrivacyManager();
    void NotifyAppSubscribeSensor(int32_t sensorTypeId);
    void UpdateCurrentUserId();
    SensorServiceState state_;
    std::mutex serviceLock_;
    std::mutex sensorsMutex_;
    std::mutex sensorMapMutex_;
    std::vector<Sensor> sensors_;
    std::unordered_map<SensorDescription, Sensor> sensorMap_;
#ifdef HDF_DRIVERS_INTERFACE_SENSOR
    bool InitInterface();
    bool InitDataCallback();
    bool InitSensorList();
    bool InitPlugCallback();
    SensorHdiConnection &sensorHdiConnection_ = SensorHdiConnection::GetInstance();
    sptr<SensorDataProcesser> sensorDataProcesser_ = nullptr;
    sptr<ReportDataCallback> reportDataCallback_ = nullptr;
#endif // HDF_DRIVERS_INTERFACE_SENSOR
    ClientInfo &clientInfo_ = ClientInfo::GetInstance();
    SensorManager &sensorManager_ = SensorManager::GetInstance();
    std::mutex uidLock_;
    // death recipient of sensor client
    std::mutex clientDeathObserverMutex_;
    sptr<IRemoteObject::DeathRecipient> clientDeathObserver_ = nullptr;
    std::shared_ptr<PermStateChangeCb> permStateChangeCb_ = nullptr;
    ErrCode SaveSubscriber(const SensorDescription &sensorDesc, int64_t samplingPeriodNs, int64_t maxReportDelayNs);
    std::atomic_bool isReportActiveInfo_ = false;
    static std::atomic_bool isAccessTokenServiceActive_;
    static std::atomic_bool isMemoryMgrServiceActive_;
    static std::atomic_bool isCritical_;
    static std::atomic_bool isDataShareReady_;
    static std::atomic_bool isSensorShakeControlManagerReady_;
    static std::atomic_bool isUpdateCurrentUserId_;
    static std::mutex updateCurrentUserIdMutex_;
    static std::atomic_int32_t deviceType_;
};

#define POWER_POLICY SensorPowerPolicy::GetInstance()
} // namespace Sensors
} // namespace OHOS
#endif // SENSOR_SERVICE_H
