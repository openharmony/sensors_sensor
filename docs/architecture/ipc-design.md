# IPC 设计 — Proxy/Stub

## IPC 接口定义

IPC 契约定义在 `frameworks/native/ISensorService.idl`，服务端为 `SensorServiceStub`，客户端为 `SensorServiceProxy`。

详见 codewiki modules.md §2 §5.3(ISensorService.idl)。

### 核心接口

| 接口 | 方向 | 说明 |
|------|------|------|
| `EnableSensor` | App → Service | 启用传感器，传递 SensorDescriptionIPC + 采样周期 + 报告延迟 |
| `DisableSensor` | App → Service | 禁用传感器 |
| `GetSensorList` | App → Service | 获取所有传感器列表 |
| `TransferDataChannel` | App → Service | 传递发送端 FD + 客户端 RemoteObject，建立数据通道 |
| `DestroySensorChannel` | App → Service | 销毁数据通道 |
| `SuspendSensors` / `ResumeSensors` | App → Service | 进程级冻结/恢复 |
| `CreateSocketChannel` | App → Service | 创建 Socket 数据通道，返回客户端 FD |
| `TransferClientRemoteObject` | App → Service | 传递客户端 Stub，建立双向 IPC |

### 数据通道架构

```
服务端 SensorService
  → StreamServer (AF_UNIX SOCK_SEQPACKET)
  → 发送端 FD
  → 客户端接收端 FD
  → EventRunner (FFRT 线程)
  → FileDescriptorListener
  → SensorDataChannel
  → 用户回调集合
```

详见 codewiki modules.md §2 §4.2(数据通道架构)。

## 客户端 Stub

`SensorClientStub`（`frameworks/native/src/sensor_client_stub.cpp`）接收服务端推送的：
- 传感器数据事件
- 传感器插拔事件

服务端通过 `OnRemoteRequest` 分发到 `HandleSensorData` / `HandlePlugSensorData`。

详见 codewiki modules.md §2 §8.4(插拔事件处理流程)。

## SA 配置

- SA ID: 3601
- 进程: sensors
- 库: libsensor_service.z.so
- 启动: run-on-create=true（按需启动）
- HDI 代理版本: libsensor_proxy_3.0.z.so

配置文件: `sa_profile/3601.json`
