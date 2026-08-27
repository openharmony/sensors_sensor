# Native API / C API

> C/C++ 传感器接口，位于 `frameworks/native/`。

## 公共 API（inner_api）

头文件：`interfaces/inner_api/sensor_agent.h`、`sensor_agent_type.h`

| 函数 | 说明 |
|------|------|
| `GetAllSensors(SensorInfo **info, int32_t *count)` | 获取所有传感器 |
| `GetDeviceSensors(deviceId, info, count)` | 获取指定设备传感器 |
| `SubscribeSensor(sensorId, user)` | 订阅传感器 |
| `UnsubscribeSensor(sensorId, user)` | 取消订阅 |
| `ActivateSensor(sensorId, user)` | 启用传感器 |
| `DeactivateSensor(sensorId, user)` | 停用传感器 |
| `SetBatch(sensorId, user, samplingInterval, reportInterval)` | 配置采样和报告延迟 |
| `SetMode(sensorId, user, mode)` | 设置传感器模式 |
| `SubscribeSensorPlug(user)` | 订阅插拔事件 |
| `UnsubscribeSensorPlug(user)` | 取消订阅插拔事件 |
| `SuspendSensors(pid)` / `ResumeSensors(pid)` | 进程级冻结/恢复 |
| `GetActiveSensorInfos(pid, info, count)` | 获取活跃传感器信息 |
| `ResetSensors()` | 重置所有传感器 |
| `SetDeviceStatus(deviceStatus)` | 设置设备状态 |

详见 codewiki modules.md §2 §5.1(公共 API)。

## C API（kits/c）

头文件：`interfaces/kits/c/oh_sensor.h`、`oh_sensor_type.h`

| 函数 | 说明 |
|------|------|
| `OH_Sensor_GetInfos(sensors, count)` | 获取传感器信息 |
| `OH_Sensor_CreateInfos(count)` / `OH_Sensor_DestroyInfos(sensors, count)` | 创建/销毁传感器信息数组 |
| `OH_Sensor_Subscribe(id, attribute, user)` | 订阅传感器 |
| `OH_Sensor_Unsubscribe(id, user)` | 取消订阅 |
| `OH_SensorEvent_GetType(event, type)` | 获取事件类型 |
| `OH_SensorEvent_GetTimestamp(event, timestamp)` | 获取时间戳 |
| `OH_SensorEvent_GetAccuracy(event, accuracy)` | 获取精度 |
| `OH_SensorEvent_GetData(event, data, length)` | 获取事件数据 |

详见 codewiki modules.md §2 §5.2(C API)。

## SensorUser 结构

```cpp
struct SensorUser {
    RecordSensorCallback callback;     // 数据回调函数指针
    SensorPlugCallback plugCallback;   // 插拔事件回调
    void *userData;                     // 用户私有数据
};
```

## 错误码归一化

`NormalizeErrCode`（`frameworks/native/src/sensor_agent.cpp`）将底层错误码统一为：
- `PERMISSION_DENIED` (201)
- `PARAMETER_ERROR` (401)
- `NON_SYSTEM_API` (202)
- `SERVICE_EXCEPTION` (默认)

详见 codewiki modules.md §2 §7.3(服务端校验)。
