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
| Native API / C API | docs/api/native-api.md |
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
- 系统 API 传感器（COLOR/SAR/HEADPOSTURE）限制在 g_systemApiSensorCall 集合中，非系统应用访问返回 202（详见 docs/security/permission.md）
- 将传感器暴露给三方应用时需从 g_systemApiSensorCall 移除该类型，并检查 NAPI 数据属性映射是否已存在
