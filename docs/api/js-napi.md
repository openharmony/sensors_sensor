# JS NAPI 接口

> 传感器 JS/ArkTS API，位于 `frameworks/js/napi/`。

## 订阅接口

| 模式 | 函数 | 说明 |
|------|------|------|
| 持续订阅 | `sensor.on(sensorTypeId, callback, options?)` | 数据持续回调 |
| 单次订阅 | `sensor.once(sensorTypeId, callback)` | 第一个数据到达后自动取消 |
| 取消订阅 | `sensor.off(sensorTypeId, callback?, options?)` | 移除回调 |

详见 codewiki modules.md §1 §2.1(传感器订阅与数据回调)、§8.1(传感器订阅时序图)。

### 订阅选项

| 参数 | 类型 | 说明 |
|------|------|------|
| `options.interval` | string \| number | `"normal"`(~200ms) / `"ui"`(~60ms) / `"game"`(~20ms) / 纳秒数值 |
| `options.sensorInfoParam.deviceId` | number | 设备 ID |
| `options.sensorInfoParam.sensorIndex` | number | 传感器索引 |

## 查询接口

| 函数 | 说明 |
|------|------|
| `getSensorList(callback?)` | 获取所有传感器列表，返回 Promise/Sync |
| `getSingleSensor(sensorTypeId, callback?)` | 获取指定类型传感器信息 |
| `getSensorListByDeviceSync(deviceId)` | 按设备查询 |
| `getSingleSensorByDeviceSync(sensorTypeId, deviceId?)` | 按设备+类型查询 |

详见 codewiki modules.md §1 §2.2(传感器查询)。

## 传感器状态订阅与取消订阅

```typescript
sensor.on("sensorStatusChange", (state: SensorState) => {})
sensor.off("sensorStatusChange", callback?)
```

`SensorState` 包含 `sensorId`、`isSensorOnline`、`deviceId`、`isLocalSensor` 等。

详见 codewiki modules.md §1 §2.5(传感器状态监控)。

## 回调管理

全局回调映射（`frameworks/js/napi/src/sensor_js.cpp`）：

| 变量 | 用途 |
|------|------|
| `g_onCallbackInfos` | on 模式回调映射（SensorDescription → 回调列表） |
| `g_onceCallbackInfos` | once 模式回调映射 |
| `g_subscribeCallbacks` | 系统 API 回调映射 |
| `g_plugCallbackInfo` | 插拔回调列表 |

详见 codewiki modules.md §1 §6.2(全局回调管理)。

## 数据属性映射

`g_sensorAttributeList`（`sensor_napi_utils.cpp`）按传感器类型映射属性名：

| 传感器类型 | 属性 |
|------------|------|
| 加速度计 | x, y, z |
| 陀螺仪 | x, y, z |
| 磁场 | x, y, z |
| 环境光 | intensity, colorTemperature, infraredLuminance |
| 气压计 | pressure |
| 接近 | distance |
| 心率 | heartRate |
| 霍尔 | |
| 温度 | |
| 湿度 | |
| 方向 | |
| 重力 | x, y, z |
| 线性加速度 | x, y, z |
| 旋转矢量 | x, y, z, w |
| 计步器 | steps |
| 显著运动 | |

详见 codewiki modules.md §1 §6.3(传感器属性映射)。

## 错误处理

| 错误码 | 名称 | 说明 |
|--------|------|------|
| 0 | ERR_OK | 成功 |
| 201 | PERMISSION_DENIED | 权限不足 |
| 202 | NON_SYSTEM_API | 非系统 API |
| 401 | PARAMETER_ERROR | 参数错误 |
| 12900001 | SERVICE_EXCEPTION | 服务异常（旧码 14500101） |
| 12900002 | SENSOR_NO_SUPPORT | 传感器不支持（旧码 14500102） |

通过 `napi_throw` 抛给 JS 层，应用用 `try-catch` 捕获。详见 codewiki modules.md §1 §7.3(错误处理机制)。
