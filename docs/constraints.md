# 专家经验 — 约束与反模式

## 约束

### 权限
- 权限校验在 Sensor Service 端完成（`services/src/sensor_service.cpp`），客户端不做额外校验（来源：codewiki modules.md §2 §7.2）
- 系统 API 传感器接口需检查 `IsSystemCalling()`，否则返回错误码 202（来源：core.md §7.2.1）
- 加速度计/陀螺仪需 `ohos.permission.ACCELEROMETER`/`GYROSCOPE`（system_grant）；计步器/心率需 `ACTIVITY_MOTION`/`READ_HEALTH_DATA`（user_grant）
- 敏感权限调用后必须调用 `AddPermissionUsedRecord()` 记录（来源：core.md §7.1.2）

### 线程安全
- 传感器代理使用延迟单例模板 `SensorDelayedSpSingleton`，确保多线程安全初始化（来源：utils/common/include/sensor_delayed_sp_singleton.h）
- NAPI 回调通过 `EmitUvEventLoop` + `napi_send_event` 跨线程投递到 JS 线程，不可直接在数据回调线程调用 JS 函数（来源：modules.md §1 §8.3）
- `g_onCallbackInfos` / `g_onceCallbackInfos` 使用 `std::mutex` 保护（来源：modules.md §1 §6.2）

### 内存安全
- 使用 `memcpy_s` 而非 `memcpy`（来源：core.md §7.2.3）
- 环形缓冲区 `CircularEventBuf`（CIRCULAR_BUF_LEN=1024）覆盖式写入，防止溢出（来源：core.md §5.5）
- 消息包 `NetPacket` 带长度校验，防止畸形数据（来源：core.md §7.2.3）
- NAPI 回调必须使用 `napi_open_handle_scope` / `napi_close_handle_scope` 包裹（来源：modules.md §1 §8.3）

### 数据结构
- `SensorDescription` = `{deviceId, sensorType, sensorId, location}` 四元组，逻辑上唯一标识传感器实例，作为 `subscribeMap_` 的 key，不可拆分使用（来源：core.md §5.1.1）
- `SensorDescriptionIPC` 是 `SensorDescription` 的 IPC Parcel 序列化结构，通过 `ISensorService.idl` 传递，字段顺序必须与 IDL 定义一致（来源：modules.md §2 §5.3）
- `SensorDescription`（逻辑描述符）和 `SensorDescriptionIPC`（IPC 序列化结构）是同一数据的两种表示，文档中 `SensorDescription` 泛指两者

### HDI 连接
- HDI 服务死亡时自动重连，最多重试 25 次，超出后写 HiSysEvent（来源：core.md §3.5）
- 重连成功后自动调用 `ReEnableSensor()` 恢复已启用的传感器（来源：core.md §3.5）
- ENG 版本支持 Mock 传感器（`HdiServiceImpl`），非 ENG 版本不可使用（来源：core.md §3.1）

### 数据传输
- 服务端与客户端使用 `AF_UNIX SOCK_SEQPACKET` Socket 传输数据，FD 通过 Binder 传递（来源：core.md §5.3）
- `TransferDataChannel` 传递的是发送端 FD，客户端创建接收端（来源：modules.md §2 §5.3）

### 生命周期
- 订阅顺序必须为 Subscribe → SetBatch → Activate，取消顺序为 Deactivate → Unsubscribe（来源：core.md §3.4）
- NAPI 模块注册 `napi_add_env_cleanup_hook` 清理钩子，JS 环境销毁时清理回调（来源：modules.md §1 §7.4）
- once 模式回调执行后必须检查是否需要 `UnsubscribeSensor`（来源：modules.md §1 §8.5）

### 数据阻断与管控
- `SensorDataBlockPolicy` 由 inner API 控制某些应用不上报某些传感器数据，新增数据上报路径需检查是否受此策略影响
- `SensorShakeControlManager` 受产品策略控制，生效时摇一摇相关传感器数据会被管控
- `fifo_cache_data` 相关的 FIFO 缓存数据上报目前不支持

### HDI 版本
- V3.0 为主路径（`hdi_connection/hardware/`），V1.0 为旧设备兜底（`hdi_connection/adapter/`）
- V3.1 目前不涉及切换；新功能涉及 HDI 时由 HDI 侧一起修改，不单独在 sensor 仓改 HDI 接口

## 反模式

| 反模式 | 正确做法 |
|--------|----------|
| 在传感器数据回调中执行耗时操作 | 数据回调是高频路径，耗时操作会阻塞分发；应异步处理 |
| 预先分配分发/渲染上下文 | 保持懒分配，普通默认路径不应预分配 |
| 绕过 `subscribeMap_` 直接调用底层 `EnableSensor` | 必须先 Subscribe 再 Activate，通过 `SensorAgentProxy` 统一管理 |
| 把系统 API 传感器逻辑折叠进通用 on/off 路径 | 系统 API 和公共 API 回调表（`g_subscribeCallbacks` vs `g_onCallbackInfos`）分开维护 |
| 忽略 once 模式的自动取消订阅 | once 回调执行后必须检查无其余订阅时调用 `UnsubscribeSensor` |
| 在 HDI 回调线程直接处理数据 | 应通过 `SensorDataProcesser` 过滤/转换后再投递 |
| 直接 `memcpy` 传感器数据 | 使用 `memcpy_s` 安全拷贝 |
| 修改 `SensorDescriptionIPC` 字段顺序而不更新 IDL | IDL（`ISensorService.idl`）是 IPC 契约，字段顺序必须一致 |

## 依赖禁忌

- `frameworks/` **不可**直接依赖 `services/` 的内部头文件，只通过 IPC 接口通信
- `services/hdi_connection/adapter/`（V1.0 兜底）和 `hardware/`（V3.0）**不可**互相依赖
- `vibration_convert/` **不可**依赖 `services/`，它是独立算法模块（暂无应用，新需求走 miscdevice 仓）
- `utils/` **不可**依赖 `frameworks/` 或 `services/`，是最底层基础库
- `frameworks/cj/` 由其余团队维护，不在本仓修改范围，本仓不改动
- 涉及接口层修改时，需同步修改 `frameworks/ets/taihe/`
