# HDI 连接 — 驱动连接/重连/容错

## HDI 连接架构

```
SensorManager (services/)
  → SensorHdiConnection (interface/)
    → HdiConnection (hardware/)     — V3.0 主路径
    → CompatibleConnection (adapter/) — V1.0 兼容路径
      → HdiServiceImpl (hardware/)   — ENG Mock
```

详见 codewiki core.md §3.1(整体架构)、modules.md §3(HDI Connection)。

## 接口

HDI 连接接口定义在 `services/hdi_connection/interface/include/i_sensor_hdi_connection.h`：

| 接口 | 说明 |
|------|------|
| `ConnectHdi()` | 建立 HDI 连接 |
| `GetSensorList()` | 获取传感器列表 |
| `EnableSensor(desc)` | 启用传感器 |
| `DisableSensor(desc)` | 禁用传感器 |
| `SetBatch(desc, interval, report)` | 配置采样和报告延迟 |
| `RegisterDataReport(cb, callback)` | 注册数据上报回调 |
| `RegSensorPlugCallback(cb)` | 注册插拔回调 |
| `TransformSensorData(state, policy, data)` | 折叠屏数据归一化 |

详见 codewiki core.md §4.4(HDI 接口)。

## 重连容错机制

1. HDI 服务死亡时 `ProcessDeathObserver` 触发
2. 调用 `ISensorInterface::Get()` 重新获取代理
3. 最多重试 25 次
4. 重连成功后调用 `ReEnableSensor()` 恢复所有已启用的传感器
5. 重连失败写入 HiSysEvent 用于问题追踪

详见 codewiki core.md §3.5(服务连接与重连流程)、§7.4.2(HDI 服务容错)。

## Mock 传感器

- 仅 ENG 版本可用
- `HdiServiceImpl`（`services/hdi_connection/hardware/`）提供虚拟传感器
- 用于无硬件环境的开发测试

详见 codewiki core.md §3.1(整体架构 Mock 节点)。

## 数据转换

`TransformSensorData` 支持折叠屏设备姿态变化时的数据归一化处理，通过 `ISensorConvertInterface` V1.0 实现。

详见 codewiki core.md §2(功能列表 - 数据转换功能)。
