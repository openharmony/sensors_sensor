# 权限模型与错误码

## 权限映射表

| 传感器类型 | 所需权限 | 授权方式 |
|------------|---------|---------|
| 加速度计、线性加速度 | `ohos.permission.ACCELEROMETER` | system_grant |
| 陀螺仪、未校准陀螺仪 | `ohos.permission.GYROSCOPE` | system_grant |
| 计步器、显著运动 | `ohos.permission.ACTIVITY_MOTION` | user_grant |
| 心率传感器 | `ohos.permission.READ_HEALTH_DATA` | user_grant |
| 管理传感器（Suspend/Resume） | `ohos.permission.MANAGE_SENSORS` | system_grant |

详见 codewiki core.md §7.1.1(权限映射表)。

## 权限校验流程

1. 应用调用 `ActivateSensor` / `SubscribeSensor`
2. NAPI 层通过 IPC 调用 `SensorServiceProxy::EnableSensor`
3. `SensorService` 收到请求后调用 `VerifyAccessToken(callerToken, permission)`
4. AccessToken 返回 GRANTED 或 DENIED
5. 敏感权限调用后执行 `AddPermissionUsedRecord()` 记录

权限校验在 Service 端完成，客户端不做校验。详见 codewiki core.md §7.1.2(权限校验流程)。

## 系统 API 限制

部分传感器接口仅限系统应用：

```cpp
// services/src/sensor_service.cpp
if (IsSystemApiSensor(sensorType) && !IsSystemCalling()) {
    return NON_SYSTEM_API;  // 错误码 202
}
```

## 错误码

| 错误码 | 含义 | 处理建议 |
|--------|------|----------|
| 0 | 成功 | - |
| 201 | 权限不足 | 检查应用 `module.json5` 权限声明 |
| 202 | 非系统 API | 仅系统应用可调用 |
| 401 | 参数错误 | 检查 API 调用参数 |
| 12900001 | 服务异常 | 检查 Sensor Service 状态，`hidumper -s 3601` |
| 12900002 | 传感器不支持 | 检查设备硬件能力 |
| 14500101 | 服务异常（旧码） | 同 12900001 |
| 14500102 | 传感器不支持（旧码） | 同 12900002 |

详见 codewiki core.md §7.4.3(错误处理)、modules.md §1 附录(错误码定义)。

## 权限声明

应用在 `module.json5` 中声明：

```json
{
  "module": {
    "requestPermissions": [
      { "name": "ohos.permission.ACCELEROMETER", "reason": "$string:reason_accelerometer" },
      { "name": "ohos.permission.GYROSCOPE", "reason": "$string:reason_gyroscope" }
    ]
  }
}
```

详见 codewiki core.md §6.4.2(权限配置)。
