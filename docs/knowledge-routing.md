# 知识路由

> 遇到什么问题读什么文档。本文件是路由索引，指向 docs/ 子文档和 codewiki 段落。

## 场景路由表

| 场景/问题 | 先读 | 关键概念 |
|-----------|------|----------|
| 目录分层、模块职责、入口文件 | docs/code-map.md | frameworks/services/utils/vibration_convert |
| 编译命令、产物路径、测试 target | docs/build-test.md | hb build、sensor_target、SA 3601 |
| 约束、反模式、依赖禁忌 | docs/constraints.md | 权限校验、四元组、memcpy_s |
| 传感器订阅/取消/数据回调（on/once/off） | docs/api/js-napi.md | SensorDescription四元组、g_onCallbackInfos、EmitUvEventLoop |
| Native API / C API（C/C++） | docs/api/native-api.md | sensor_agent.h、oh_sensor.h、GetAllSensors |
| IPC 接口定义与 Proxy/Stub | docs/architecture/ipc-design.md | ISensorService.idl、Proxy-Stub、TransferDataChannel |
| 数据从硬件到应用完整链路 | docs/architecture/data-flow.md | HDI→SensorDataProcesser→NetPacket→Socket→SensorEvent |
| HDI 连接/重连/容错机制 | docs/architecture/hdi-connection.md | ISensorInterface V3.0、25次重试、ReEnableSensor |
| 权限模型与错误码 | docs/security/permission.md | AccessToken、201/202/401/14500101 |
| 传感器算法（地磁/海拔/四元数/旋转矩阵） | docs/api/sensor-algorithm.md | GeomagneticField WMM、SensorAlgorithm |
| 振动转换（音频→触觉） | docs/features/vibration-convert.md | WAV→STFT→FFT/MFCC→PeakFinder→HapticEvent |
| 订阅状态机与生命周期 | docs/architecture/data-flow.md §状态机 | Idle→Subscribed→Activated、subscribeMap_ |
| 功耗管理/冻结恢复 | docs/architecture/data-flow.md §功耗 | Suspend/Resume、SensorPowerPolicy |
| 系统 API 传感器管控/传感器暴露给三方/系统API限制 | docs/security/permission.md + docs/constraints.md | g_systemApiSensorCall、SENSOR_TYPE_ID_COLOR/SAR/HEADPOSTURE、IsSystemCalling、NON_SYSTEM_API(202) |
| 摇一摇数据管控 | docs/architecture/data-flow.md §摇一摇 | SensorShakeControlManager、g_shakeSensorControlList |
| 数据阻断策略 | docs/architecture/data-flow.md §数据阻断 | SensorDataBlockPolicy、BlockSensorDataByPid |
| 故障排查/日志/调试 | docs/build-test.md §调试 | hidumper -s 3601、hilog |

## 术语表

| 术语 | 含义 |
|------|------|
| SA 3601 | SensorService 系统能力 ID；服务运行在 `sensors` 进程（不是 foundation 进程），客户端代码运行在调用者进程。文档中"Sensor Service"泛指此 SA |
| HDI | Hardware Driver Interface，硬件驱动接口 |
| ISensorInterface V3.0 | HDI 传感器接口主版本，对应 `hdi_connection/hardware/` |
| CompatibleConnection V1.0 | HDI 兼容连接，旧设备兜底，对应 `hdi_connection/adapter/` |
| Mock 传感器 | ENG 版本虚拟传感器，`HdiServiceImpl` 实现，用于开发测试 |
| 触发型传感器 | 上报一次数据后自动去使能的传感器类型 |
| SensorDescription | `{deviceId, sensorType, sensorId, location}` 四元组，逻辑标识传感器实例；IPC 传递时序列化为 `SensorDescriptionIPC` |
| Proxy-Stub | IPC 通信的代理-存根模式，客户端 Proxy 调用，服务端 Stub 响应 |
| EmitUvEventLoop | NAPI 跨线程回调到 JS 线程的机制，基于 `napi_send_event` |
| WMM | World Magnetic Model，地磁场计算模型（2020-2025） |
| SensorDelayedSpSingleton | 延迟初始化线程安全单例模板 |
| CircularEventBuf | 环形缓冲区，CIRCULAR_BUF_LEN=1024，覆盖式写入 |
| NetPacket | IPC 数据包，PackHead + Body，带长度校验 |
| StreamSocket | AF_UNIX SOCK_SEQPACKET 数据通道 |
| sensor_rust_socket_ipc | bundle.json feature flag（已废弃，rust 代码已删除，flag 未清理干净） |
| Taihe | OpenHarmony 声明式 ETS 接口规范 |
| Cangjie | 华为仓颉编程语言，`frameworks/cj/` 提供传感器 FFI 绑定（其余团队维护） |
| SensorShakeControlManager | 摇一摇数据管控，受产品策略控制，生效时摇一摇数据被管控 |
| SensorDataBlockPolicy | 数据阻断策略，inner API 控制某些应用不上报某些传感器数据 |
| g_systemApiSensorCall | 系统API传感器限制集合，定义在 services/src/sensor_service.cpp，包含 SENSOR_TYPE_ID_COLOR/SAR/HEADPOSTURE，非系统应用访问时返回 NON_SYSTEM_API(202) |
| IsSystemCalling | 系统调用判断，TOKEN_NATIVE 视为系统调用，系统应用通过 TokenIdKit 判断 |
| NON_SYSTEM_API | 错误码 202，非系统应用调用系统API传感器时返回 |
