# 数据流与架构

## 整体架构

四层结构：应用层（JS/ArkTS/NDK）→ 接口层（NAPI/Native）→ 服务层（Sensor Service SA 3601）→ 驱动层（HDI/Sensor Driver）

**进程模型**：SA 3601 运行在 `sensors` 进程（不是 foundation 进程）。客户端代码（Proxy/DataChannel）运行在调用者进程中，如 foundation 或应用进程。

详见 codewiki core.md §3.1(整体架构)。

## 数据流向

传感器数据从硬件到应用的完整链路：

```
传感器硬件
  → HDF Driver (HdfSensorEvents)
  → ISensorInterface::Get() 回调
  → SensorEventCallback (OnDataEventAsync)
  → SensorData (HDI 层数据格式)
  → SensorDataProcesser (Filter/Transform)
  → NetPacket (PackHead + Body)
  → StreamSocket (AF_UNIX SOCK_SEQPACKET, sendmsg)
  → 客户端 SensorDataChannel (recv)
  → SensorEvent 构造
  → 应用回调 (x/y/z 或各属性)
```

详见 codewiki core.md §3.2(数据流向)、§5.3(数据流转换)。

### 关键数据结构

| 结构 | 位置 | 说明 |
|------|------|------|
| SensorData | HDI 层 | `int32_t sensorTypeId; int64_t timestamp; uint8_t data[64];` |
| NetPacket | utils/ipc | `StreamBuffer` 基类，带包头和长度校验 |
| SensorEvent | 客户端 | `int32_t sensorTypeId; uint8_t *data; uint32_t dataLen;` |
| CallbackSensorData | NAPI 层 | NAPI 内部数据传递结构 |

数据结构定义详见 core.md §5.1(核心数据结构)。

## 订阅状态机

```
Idle → SubscribeSensor → Subscribed → ActivateSensor → Activated
Activated → DeactivateSensor → Subscribed
Subscribed → UnsubscribeSensor → Idle (最后订阅者)
```

- 订阅顺序必须为 Subscribe → SetBatch → Activate
- 取消顺序为 Deactivate → Unsubscribe
- 最后一个订阅者取消时，自动销毁数据通道

详见 codewiki core.md §3.4(订阅状态机)。

## 服务连接与重连

- 客户端通过 `SystemAbilityManager` 获取 SA 3601 代理
- 首次调用 `TransferClientRemoteObject` 传递 Stub，建立双向通信
- 服务死亡时 `ProcessDeathObserver` 触发，重建数据通道
- HDI 服务死亡时自动重连，最多 25 次，重连后 `ReEnableSensor()` 恢复

详见 codewiki core.md §3.5(服务连接与重连流程)。

## 功耗管理

- 传感器进程被冻结（Suspend）时保存配置
- 恢复（Resume）时自动重新配置传感器参数
- `SensorPowerPolicy` 管理冻结/恢复逻辑
- 这些接口专为功耗管理侧调用而设计

详见 codewiki core.md §7.4.1(电源管理)。

## 摇一摇管控

`SensorShakeControlManager`（`services/include/sensor_shake_control_manager.h`）负责摇一摇数据管控：

- 受产品控制策略控制，决定是否生效
- 当管控生效时，摇一摇相关的传感器数据会被管控（不上报给应用）
- 开发涉及摇一摇/加速度计相关需求时需注意此管控逻辑

## 数据阻断策略

`SensorDataBlockPolicy`（`services/include/sensor_data_block_policy.h`）：

- 由 inner API 控制给某些应用不上报某些传感器数据
- 开发需求时需注意：新增传感器数据上报路径需检查是否受数据阻断策略影响
- `fifo_cache_data` 相关的 FIFO 缓存数据上报目前不支持

## 模块依赖关系

```
Level 1: utils/ipc ← utils/common (基础)
Level 2: frameworks/native ← services ← services/hdi_connection (核心)
Level 3: frameworks/js/napi ← frameworks/native (接口)
```

详见 codewiki core.md §3.3(模块依赖关系)。
