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

#include "sensor_fuzzer.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <fuzzer/FuzzedDataProvider.h>

#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "securec.h"
#include "sensor_agent.h"
#include "sensor_agent_type.h"
#include "sensor_errors.h"
#include "token_setproc.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
using namespace OHOS::Security::AccessToken;
using OHOS::Security::AccessToken::AccessTokenID;

namespace {

// ======= 常量定义区 =======
constexpr size_t MAX_OPS = 20;
constexpr size_t MAX_USER_NAME_LEN = 64;
constexpr int32_t DEFAULT_SENSOR_TYPE = SENSOR_TYPE_ID_ACCELEROMETER;
constexpr int64_t MIN_SAMPLING_INTERVAL = 0;
constexpr int64_t MAX_SAMPLING_INTERVAL = 1000000000LL; // 1秒
constexpr int32_t MIN_PID = 1;
constexpr int32_t MAX_PID = 65535;
constexpr size_t PERM_ARRAY_SIZE = 3;
constexpr int32_t PERMS_NUM = 3;
constexpr int32_t MIN_SLEEP_MS = 10;
constexpr int32_t MAX_SLEEP_MS = 100;
constexpr int32_t PERM_INDEX_ACCELEROMETER = 0;
constexpr int32_t PERM_INDEX_GYROSCOPE = 1;
constexpr int32_t PERM_INDEX_MANAGE_SENSOR = 2;

// ======= 动作类型枚举 =======
enum class ActionType : uint8_t {
    GET_ALL_SENSORS = 0,
    SUBSCRIBE_SENSOR,
    UNSUBSCRIBE_SENSOR,
    ACTIVATE_SENSOR,
    DEACTIVATE_SENSOR,
    SET_BATCH,
    SET_MODE,
    GET_ACTIVE_SENSOR_INFOS,
    RESET_SENSORS,
    SUSPEND_SENSORS,
    RESUME_SENSORS,
    REGISTER_CALLBACK,
    UNREGISTER_CALLBACK,
    SET_DEVICE_STATUS,
    ACTION_MAX
};

// ======= 辅助结构体：存储会话状态 =======
struct FuzzSession {
    int32_t sensorTypeId;
    int32_t sensorMode;
    int32_t pid;
    int64_t samplingInterval;
    int64_t reportInterval;
    SensorUser sensorUser;
    bool isSubscribed;
    bool isActivated;
};

// ======= 传感器数据回调实现 =======
void SensorDataCallbackImpl(SensorEvent* event)
{
    if (event == nullptr) {
        return;
    }
    // 空实现，只用于测试
}

// ======= 传感器活跃信息回调 =======
void SensorActiveInfoCallbackImpl(SensorActiveInfo& activeInfo)
{
    // 空实现，只用于测试
}

// ======= 权限设置 =======
void SetUpPermissions()
{
    static bool isPermSet = false;
    if (isPermSet) {
        return;
    }

    const char** perms = new (std::nothrow) const char* [PERM_ARRAY_SIZE];
    if (perms == nullptr) {
        return;
    }

    perms[PERM_INDEX_ACCELEROMETER] = "ohos.permission.ACCELEROMETER";
    perms[PERM_INDEX_GYROSCOPE] = "ohos.permission.GYROSCOPE";
    perms[PERM_INDEX_MANAGE_SENSOR] = "ohos.permission.MANAGE_SENSOR";

    TokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = PERMS_NUM,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "SensorFuzzTest",
        .aplStr = "system_core",
    };

    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    AccessTokenKit::ReloadNativeTokenInfo();

    delete[] perms;
    isPermSet = true;
}

// ======= 验证传感器ID是否有效 =======
bool IsValidSensorTypeId(int32_t sensorTypeId)
{
    SensorInfo* sensorInfo = nullptr;
    int32_t count = 0;
    int32_t ret = GetAllSensors(&sensorInfo, &count);
    if (ret != 0 || count <= 0 || sensorInfo == nullptr) {
        return false;
    }

    for (int32_t i = 0; i < count; i++) {
        if (sensorInfo[i].sensorTypeId == sensorTypeId) {
            return true;
        }
    }
    return false;
}

// ======= 初始化会话 =======
FuzzSession InitSession(FuzzedDataProvider& fdp)
{
    FuzzSession session;

    // 设置权限
    SetUpPermissions();

    // 获取有效传感器ID
    session.sensorTypeId = fdp.ConsumeIntegral<int32_t>();
    if (!IsValidSensorTypeId(session.sensorTypeId)) {
        session.sensorTypeId = DEFAULT_SENSOR_TYPE;
    }

    // 初始化其他参数
    session.sensorMode = fdp.ConsumeIntegralInRange<int32_t>(
        static_cast<int32_t>(SENSOR_DEFAULT_MODE), static_cast<int32_t>(SENSOR_FIFO_MODE));

    session.pid = fdp.ConsumeIntegralInRange<int32_t>(MIN_PID, MAX_PID);

    session.samplingInterval = fdp.ConsumeIntegralInRange<int64_t>(MIN_SAMPLING_INTERVAL, MAX_SAMPLING_INTERVAL);

    session.reportInterval = fdp.ConsumeIntegralInRange<int64_t>(MIN_SAMPLING_INTERVAL, MAX_SAMPLING_INTERVAL);

    // 确保reportInterval >= samplingInterval
    if (session.reportInterval < session.samplingInterval) {
        session.reportInterval = session.samplingInterval;
    }

    // 初始化SensorUser
    std::string userName = fdp.ConsumeRandomLengthString(MAX_USER_NAME_LEN);
    if (userName.empty()) {
        userName = "SensorFuzzTest";
    }
    // 安全复制字符串，确保不越界
    size_t copyLen = std::min(userName.length(), size_t(NAME_MAX_LEN - 1));
    errno_t ret = memcpy_s(session.sensorUser.name, NAME_MAX_LEN, userName.c_str(), copyLen);
    if (ret != EOK) {
        (void)strncpy_s(session.sensorUser.name, NAME_MAX_LEN, "SensorFuzzTest", NAME_MAX_LEN - 1);
    } else {
        session.sensorUser.name[copyLen] = '\0';
    }
    session.sensorUser.callback = SensorDataCallbackImpl;
    session.sensorUser.plugCallback = nullptr;
    session.sensorUser.userData = nullptr;

    session.isSubscribed = false;
    session.isActivated = false;

    return session;
}

// ======= 各种动作处理函数 =======

// 动作1: 获取所有传感器
static void HandleGetAllSensors(FuzzSession& session, FuzzedDataProvider& fdp)
{
    SensorInfo* sensorInfo = nullptr;
    int32_t count = 0;
    (void)GetAllSensors(&sensorInfo, &count);
}

// 动作2: 订阅传感器
static void HandleSubscribeSensor(FuzzSession& session, FuzzedDataProvider& fdp)
{
    if (!session.isSubscribed) {
        (void)SubscribeSensor(session.sensorTypeId, &session.sensorUser);
        session.isSubscribed = true;
    }
}

// 动作3: 取消订阅传感器
static void HandleUnsubscribeSensor(FuzzSession& session, FuzzedDataProvider& fdp)
{
    if (session.isSubscribed) {
        (void)UnsubscribeSensor(session.sensorTypeId, &session.sensorUser);
        session.isSubscribed = false;
        session.isActivated = false;
    }
}

// 动作4: 激活传感器
static void HandleActivateSensor(FuzzSession& session, FuzzedDataProvider& fdp)
{
    if (session.isSubscribed && !session.isActivated) {
        (void)ActivateSensor(session.sensorTypeId, &session.sensorUser);
        session.isActivated = true;
    }
}

// 动作5: 停用传感器
static void HandleDeactivateSensor(FuzzSession& session, FuzzedDataProvider& fdp)
{
    if (session.isActivated) {
        (void)DeactivateSensor(session.sensorTypeId, &session.sensorUser);
        session.isActivated = false;
    }
}

// 动作6: 设置采样和上报间隔
static void HandleSetBatch(FuzzSession& session, FuzzedDataProvider& fdp)
{
    if (session.isSubscribed) {
        // 重新生成间隔值
        int64_t newSampling = fdp.ConsumeIntegralInRange<int64_t>(MIN_SAMPLING_INTERVAL, MAX_SAMPLING_INTERVAL);
        int64_t newReport = fdp.ConsumeIntegralInRange<int64_t>(MIN_SAMPLING_INTERVAL, MAX_SAMPLING_INTERVAL);

        if (newReport < newSampling) {
            newReport = newSampling;
        }

        (void)SetBatch(session.sensorTypeId, &session.sensorUser, newSampling, newReport);
    }
}

// 动作7: 设置传感器模式
static void HandleSetMode(FuzzSession& session, FuzzedDataProvider& fdp)
{
    if (session.isSubscribed) {
        int32_t mode = fdp.ConsumeIntegralInRange<int32_t>(
            static_cast<int32_t>(SENSOR_DEFAULT_MODE), static_cast<int32_t>(SENSOR_FIFO_MODE));
        (void)SetMode(session.sensorTypeId, &session.sensorUser, mode);
    }
}

// 动作8: 获取激活传感器信息
static void HandleGetActiveSensorInfos(FuzzSession& session, FuzzedDataProvider& fdp)
{
    SensorActiveInfo* activeInfos = nullptr;
    int32_t count = 0;
    (void)GetActiveSensorInfos(session.pid, &activeInfos, &count);
}

// 动作9: 重置传感器
static void HandleResetSensors(FuzzSession& session, FuzzedDataProvider& fdp)
{
    (void)ResetSensors();
}

// 动作10: 暂停传感器
static void HandleSuspendSensors(FuzzSession& session, FuzzedDataProvider& fdp)
{
    (void)SuspendSensors(session.pid);
}

// 动作11: 恢复传感器
static void HandleResumeSensors(FuzzSession& session, FuzzedDataProvider& fdp)
{
    (void)ResumeSensors(session.pid);
}

// 动作12: 注册活跃信息回调
static void HandleRegisterCallback(FuzzSession& session, FuzzedDataProvider& fdp)
{
    (void)Register(SensorActiveInfoCallbackImpl);
}

// 动作13: 取消注册回调
static void HandleUnregisterCallback(FuzzSession& session, FuzzedDataProvider& fdp)
{
    (void)Unregister(SensorActiveInfoCallbackImpl);
}

// 动作14: 设置设备状态
static void HandleSetDeviceStatus(FuzzSession& session, FuzzedDataProvider& fdp)
{
    uint32_t deviceStatus = fdp.ConsumeIntegral<uint32_t>();
    SetDeviceStatus(deviceStatus);
}

// ======= 执行单个动作 =======
static void ExecuteAction(ActionType action, FuzzSession& session, FuzzedDataProvider& fdp)
{
    switch (action) {
        case ActionType::GET_ALL_SENSORS:
            HandleGetAllSensors(session, fdp);
            break;
        case ActionType::SUBSCRIBE_SENSOR:
            HandleSubscribeSensor(session, fdp);
            break;
        case ActionType::UNSUBSCRIBE_SENSOR:
            HandleUnsubscribeSensor(session, fdp);
            break;
        case ActionType::ACTIVATE_SENSOR:
            HandleActivateSensor(session, fdp);
            break;
        case ActionType::DEACTIVATE_SENSOR:
            HandleDeactivateSensor(session, fdp);
            break;
        case ActionType::SET_BATCH:
            HandleSetBatch(session, fdp);
            break;
        case ActionType::SET_MODE:
            HandleSetMode(session, fdp);
            break;
        case ActionType::GET_ACTIVE_SENSOR_INFOS:
            HandleGetActiveSensorInfos(session, fdp);
            break;
        case ActionType::RESET_SENSORS:
            HandleResetSensors(session, fdp);
            break;
        case ActionType::SUSPEND_SENSORS:
            HandleSuspendSensors(session, fdp);
            break;
        case ActionType::RESUME_SENSORS:
            HandleResumeSensors(session, fdp);
            break;
        case ActionType::REGISTER_CALLBACK:
            HandleRegisterCallback(session, fdp);
            break;
        case ActionType::UNREGISTER_CALLBACK:
            HandleUnregisterCallback(session, fdp);
            break;
        case ActionType::SET_DEVICE_STATUS:
            HandleSetDeviceStatus(session, fdp);
            break;
        default:
            break;
    }
}

// ======= 更新会话状态 =======
static void UpdateSessionState(FuzzSession& session, FuzzedDataProvider& fdp)
{
    if (!fdp.ConsumeBool()) {
        return;
    }

    // 重新验证并更新传感器ID
    int32_t newSensorId = fdp.ConsumeIntegral<int32_t>();
    if (!IsValidSensorTypeId(newSensorId)) {
        return;
    }

    // 如果已订阅旧传感器，先取消
    if (session.isSubscribed) {
        HandleUnsubscribeSensor(session, fdp);
    }
    session.sensorTypeId = newSensorId;
}

// ======= 执行操作序列 =======
static void ExecuteOperationSequence(FuzzSession& session, FuzzedDataProvider& fdp, size_t ops)
{
    for (size_t i = 0; i < ops && fdp.remaining_bytes() > 0; i++) {
        uint8_t actionValue = fdp.ConsumeIntegral<uint8_t>() % static_cast<uint8_t>(ActionType::ACTION_MAX);
        ActionType action = static_cast<ActionType>(actionValue);
        ExecuteAction(action, session, fdp);

        UpdateSessionState(session, fdp);

        // 短暂休眠，模拟真实使用
        if (fdp.ConsumeBool() && i < ops - 1) {
            int32_t sleepMs = fdp.ConsumeIntegralInRange<int32_t>(MIN_SLEEP_MS, MAX_SLEEP_MS);
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
        }
    }
}

// ======= 清理会话资源 =======
static void CleanupSession(FuzzSession& session, FuzzedDataProvider& fdp)
{
    if (!session.isSubscribed) {
        return;
    }

    if (session.isActivated) {
        HandleDeactivateSensor(session, fdp);
    }
    HandleUnsubscribeSensor(session, fdp);
}

// ======= 主驱动函数 =======
void DriveSensorAgent(FuzzSession& session, FuzzedDataProvider& fdp)
{
    size_t ops = fdp.ConsumeIntegralInRange<size_t>(1, MAX_OPS);
    ExecuteOperationSequence(session, fdp, ops);
    CleanupSession(session, fdp);
}

} // namespace

// ======= 模糊测试主入口 =======
bool SensorAgentFuzzTest(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return false;
    }

    FuzzedDataProvider fdp(data, size);
    FuzzSession session = InitSession(fdp);
    DriveSensorAgent(session, fdp);

    return true;
}

} // namespace Sensors
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    OHOS::Sensors::SensorAgentFuzzTest(data, size);
    return 0;
}
