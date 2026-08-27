# sensors_sensor 指引

## 项目定位

本仓库对应 OpenHarmony `泛sensor服务/sensors_sensor`。优先按这些目录定位问题：

- `frameworks/native/`：Native API + Proxy-Stub IPC 客户端，入口 sensor_agent.cpp
- `frameworks/js/napi/`：JS/ArkTS NAPI 绑定层，入口 sensor_js.cpp
- `services/`：Sensor Service（SA 3601），入口 sensor_service.cpp
- `services/hdi_connection/`：HDI 驱动连接层，含 V3.0/V1.0 适配和 Mock
- `utils/`：libsensor_ipc + libsensor_utils（权限/日志/单例/数据结构）
- `vibration_convert/`：音频转触觉反馈（FFT/MFCC/PeakFinder）

## 构建和验证

构建命令从 OpenHarmony 源码根目录执行：

```sh
./build.sh --product-name rk3568 --build-target sensor --ccache
hdc shell "hidumper -s 3601"    # 验证 SA 存活
```

详见 docs/build-test.md。涉及真实传感器硬件需补充板侧证据，提交使用 `git commit -s`。

## 知识索引

改动前按场景读取对应文档：

| 场景 | 先读 |
|------|------|
| 目录分层、模块职责 | docs/code-map.md |
| JS API（on/once/off/查询/状态监控） | docs/api/js-napi.md |
| Native API / NDK 接口 | docs/api/native-api.md |
| 传感器算法（地磁/海拔/四元数） | docs/api/sensor-algorithm.md |
| IPC 接口与 Proxy/Stub | docs/architecture/ipc-design.md |
| 数据流（硬件→HDI→Service→应用） | docs/architecture/data-flow.md |
| HDI 连接/重连/容错 | docs/architecture/hdi-connection.md |
| 权限模型与错误码 | docs/security/permission.md |
| 振动转换（音频→触觉） | docs/features/vibration-convert.md |
| 约束/反模式/依赖禁忌 | docs/constraints.md |
| 编译/测试/调试命令 | docs/build-test.md |
| 场景→文档完整路由表 + 术语表 | docs/knowledge-routing.md |

## 关键约束

- 权限校验在 Sensor Service 端完成，客户端不做额外校验（详见 docs/constraints.md）
- 传感器描述符 = `{deviceId, sensorType, sensorId, location}` 四元组，不可拆分使用
- HDI 重连最多 25 次，超出写 HiSysEvent
- 使用 `memcpy_s` 而非 `memcpy`；NAPI 回调不可在数据线程直接调用 JS
- 订阅顺序必须 Subscribe → SetBatch → Activate

## 参考资料

> 仅当涉及**接口签名、参数定义、枚举值、错误码含义、HDI IDL 定义**等接口相关问题时，才读取以下参考资料。做代码修改、架构分析、编译调试时不要读这些文档，避免跑偏。

### ArkTS API 参考

- [@ohos.sensor（传感器）](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-sensor-service-kit/js-apis-sensor.md) - 传感器订阅/查询/算法 API
- [@ohos.sensor（传感器）（系统接口）](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-sensor-service-kit/js-apis-sensor-sys.md) - 传感器系统 API
- [@system.sensor（传感器）](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-sensor-service-kit/js-apis-system-sensor.md) - 旧版传感器 API（兼容）
- [传感器错误码](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-sensor-service-kit/errorcode-sensor.md) - 错误码参考

### C API 参考

- [oh_sensor.h](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-sensor-service-kit/capi-oh-sensor-h.md) - 传感器操作 C API
- [oh_sensor_type.h](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-sensor-service-kit/capi-oh-sensor-type-h.md) - 传感器类型定义 C API
- [Sensor_Info](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-sensor-service-kit/capi-sensor-sensor-info.md) - 传感器信息结构体
- [Sensor_Event](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-sensor-service-kit/capi-sensor-sensor-event.md) - 传感器事件结构体
- [Sensor_SubscriptionId](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-sensor-service-kit/capi-sensor-sensor-subscriptionid.md) - 订阅 ID
- [Sensor_SubscriptionAttribute](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-sensor-service-kit/capi-sensor-sensor-subscriptionattribute.md) - 订阅属性
- [Sensor_Subscriber](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-sensor-service-kit/capi-sensor-sensor-subscriber.md) - 订阅者

### HDI 接口参考

- [ISensorInterface V3.0](https://gitcode.com/openharmony/docs/blob/master/zh-cn/device-dev/reference/hdi-apis/sensor/interface_i_sensor_interface_v20.md) - 传感器 HDI 接口 V2.0
- [ISensorInterface V3.1](https://gitcode.com/openharmony/docs/blob/master/zh-cn/device-dev/reference/hdi-apis/sensor/interface_i_sensor_interface_v20.md) - 传感器 HDI 接口 V3.x
- [ISensorCallback](https://gitcode.com/openharmony/docs/blob/master/zh-cn/device-dev/reference/hdi-apis/sensor/interface_i_sensor_callback_v20.md) - 传感器回调接口
- [SensorTypes](https://gitcode.com/openharmony/docs/blob/master/zh-cn/device-dev/reference/hdi-apis/sensor/_sensor_types_8idl_v20.md) - HDI 传感器类型定义

### 开发指南

- [Sensor Service Kit 简介](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/device/sensor/sensorservice-kit-intro.md) - 传感器服务 Kit 介绍
- [传感器开发指导](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/device/sensor/sensor-guidelines.md) - ArkTS 传感器开发指南
- [传感器开发指导（C/C++）](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/device/sensor/sensor-guidelines-capi.md) - C API 传感器开发指南
- [传感器开发术语](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/device/sensor/sensor-glossary.md) - 传感器术语定义
