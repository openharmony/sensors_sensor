# 代码结构

> sensors_sensor 仓库目录分层与模块职责。

## 目录结构

```
sensor/
├── frameworks/                  # 接口框架层
│   ├── native/                  # Native API + Proxy-Stub IPC 客户端
│   ├── js/napi/                 # JS/ArkTS NAPI 绑定层
│   ├── cj/                      # Cangjie 语言绑定层
│   └── ets/taihe/               # Taihe/ETS 绑定层
├── services/                    # Sensor Service 服务层（SA 3601）
│   ├── include/
│   ├── src/
│   └── hdi_connection/          # HDI 驱动连接层
│       ├── adapter/             # HDI 适配器（V1.0 兼容）
│       ├── hardware/            # HDI 硬件实现（含 Mock）
│       └── interface/           # HDI 连接接口
├── utils/                      # 通用工具层
│   ├── common/                  # libsensor_utils：权限/日志/单例/数据结构
│   └── ipc/                     # libsensor_ipc：StreamBuffer/NetPacket/Socket
├── vibration_convert/          # 振动转换模块（音频→触觉）
│   ├── core/                    # 核心算法（FFT/MFCC/峰值检测）
│   └── interfaces/js/           # JS NAPI 接口
├── interfaces/                  # 外部接口定义
│   ├── inner_api/               # 内部 API 头文件（sensor_agent.h）
│   └── kits/c/                  # C 公共 API（oh_sensor.h）
├── sa_profile/                  # 系统能力配置（SA 3601）
├── test/                        # 测试
│   ├── unittest/                # 单元测试
│   └── fuzztest/                # Fuzz 测试
├── bundle.json                  # 部件配置
└── hisysevent.yaml              # 事件埋点配置
```

## 模块职责

| 模块 | 路径 | 职责 | 入口文件 |
|------|------|------|----------|
| Sensor JS API | `frameworks/js/napi/` | JS/ArkTS 传感器接口绑定，on/once/off/查询/算法/状态监控 | `src/sensor_js.cpp` |
| Sensor Native Framework | `frameworks/native/` | Native API 封装 + Proxy-Stub IPC 客户端 + C API + 数据通道 + 算法 | `src/sensor_agent.cpp`、`src/sensor_agent_proxy.cpp` |
| Cangjie 绑定 | `frameworks/cj/` | Cangjie 语言传感器 FFI 绑定（其余团队维护，本仓不改动） | `src/` |
| Taihe/ETS 绑定 | `frameworks/ets/taihe/` | Taihe 声明式传感器接口（涉及接口层修改时需同步） | - |
| Sensor Service | `services/` | SA 3601 服务端，传感器注册/连接/分发/电源/数据通道 | `src/sensor_service.cpp` |
| HDI Connection | `services/hdi_connection/` | HDI 驱动连接/重连/容错，V3.0 为主路径，V1.0 为设备兜底 | `interface/src/sensor_hdi_connection.cpp` |
| HDI 硬件实现 | `services/hdi_connection/hardware/` | ISensorInterface V3.0 调用实现 + ENG Mock | `include/hdi_service_impl.h` |
| HDI 兼容适配 | `services/hdi_connection/adapter/` | V1.0 兼容连接（CompatibleConnection），旧设备兜底 | `include/compatible_connection.h` |
| Sensor Utils | `utils/common/` | 权限校验、日志、延迟单例、传感器数据结构、Parcel | `include/sensor_utils.h` |
| Sensor IPC | `utils/ipc/` | StreamBuffer/CircleStreamBuffer/NetPacket/StreamSocket | `include/stream_buffer.h` |
| Vibration Convert | `vibration_convert/core/` | WAV 解析 + STFT/FFT/MFCC + 峰值检测 + 触觉事件生成（暂无应用，新增振动需求改 miscdevice 仓） | `core/algorithm/` |
| Vibration Convert JS | `vibration_convert/interfaces/js/` | 振动转换 NAPI 接口 | - |

## 模块依赖关系

```
Level 1 (基础):  utils/ipc ← utils/common
Level 2 (核心):  frameworks/native ← services ← services/hdi_connection
Level 3 (接口):  frameworks/js/napi ← frameworks/native
                 frameworks/cj ← frameworks/native
                 frameworks/ets/taihe ← frameworks/native
                 vibration_convert/interfaces/js ← vibration_convert/core
```

依赖方向：上层 → 下层，不可反向。`frameworks/` 不直接依赖 `services/` 的内部实现，只通过 IPC 接口（`ISensorService.idl`）通信。
