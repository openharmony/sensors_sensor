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

#ifndef SENSOR_SERVICE_H
#define SENSOR_SERVICE_H

#include <mutex>
#include <thread>
#include <unordered_map>

#include "nocopyable.h"
#include "system_ability.h"

#include "client_info.h"
#include "death_recipient_template.h"
#include "sensor_agent_type.h"
#include "sensor_hdi_connection.h"
#include "sensor_manager.h"
#include "sensor_service_stub.h"

namespace OHOS {
namespace Sensors {
enum class SensorServiceState {
    STATE_STOPPED,
    STATE_RUNNING,
};

class SensorService : public SystemAbility, public SensorServiceStub {
    DECLARE_SYSTEM_ABILITY(SensorService)

public:
    explicit SensorService(int32_t systemAbilityId, bool runOnCreate = false);
    virtual ~SensorService() = default;
    void OnDump() override;
    void OnStart() override;
    void OnStop() override;
    int Dump(int fd, const std::vector<std::u16string> &args) override;
    ErrCode EnableSensor(uint32_t sensorId, int64_t samplingPeriodNs, int64_t maxReportDelayNs) override;
    ErrCode DisableSensor(uint32_t sensorId) override;
    std::vector<Sensor> GetSensorList() override;
    ErrCode TransferDataChannel(const sptr<SensorBasicDataChannel> &sensorBasicDataChannel,
                                const sptr<IRemoteObject> &sensorClient) override;
    ErrCode DestroySensorChannel(sptr<IRemoteObject> sensorClient) override;
    void ProcessDeathObserver(const wptr<IRemoteObject> &object);

private:
    DISALLOW_COPY_AND_MOVE(SensorService);
    void RegisterClientDeathRecipient(sptr<IRemoteObject> sensorClient, int32_t pid);
    void UnregisterClientDeathRecipient(sptr<IRemoteObject> sensorClient);
    bool InitInterface();
    bool InitDataCallback();
    bool InitSensorList();
    bool InitSensorPolicy();
    void ReportOnChangeData(uint32_t sensorId);
    void ReportSensorSysEvent(uint32_t sensorId, bool enable, int32_t pid);
    ErrCode DisableSensor(uint32_t sensorId, int32_t pid);
    SensorServiceState state_;
    std::mutex serviceLock_;
    std::mutex sensorsMutex_;
    std::mutex sensorMapMutex_;
    std::vector<Sensor> sensors_;
    std::unordered_map<uint32_t, Sensor> sensorMap_;
    SensorHdiConnection &sensorHdiConnection_ = SensorHdiConnection::GetInstance();
    ClientInfo &clientInfo_ = ClientInfo::GetInstance();
    SensorManager &sensorManager_ = SensorManager::GetInstance();
    sptr<SensorDataProcesser> sensorDataProcesser_;
    sptr<ReportDataCallback> reportDataCallback_;
    std::mutex uidLock_;
    // death recipient of sensor client
    sptr<IRemoteObject::DeathRecipient> clientDeathObserver_;
    ErrCode SaveSubscriber(uint32_t sensorId, int64_t samplingPeriodNs, int64_t maxReportDelayNs);
};
}  // namespace Sensors
}  // namespace OHOS
#endif  // SENSOR_SERVICE_H
