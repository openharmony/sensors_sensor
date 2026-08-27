# 振动转换 — 音频转触觉反馈

> 位于 `vibration_convert/`，将音频信号转换为触觉振动事件。
>
> **注意**：本模块暂无应用使用。新增振动相关需求请修改 `sensors_miscdevice` 仓。

## 模块结构

```
vibration_convert/
├── core/
│   ├── algorithm/     # FFT/MFCC/峰值检测
│   ├── native/        # 核心实现
│   └── utils/         # WAV 解析等工具
└── interfaces/js/     # NAPI 接口
```

## 处理流程

```
WAV 文件
  → VibrationConvertCore (Parse)
  → STFT 频谱分析
  → ConversionFFT (FFT 变换)
  → ConversionMfcc (MFCC 特征提取)
  → PeakFinder (峰值检测)
  → HapticEvent 生成
  → demo.json 输出
```

详见 codewiki core.md §3.1(振动转换路径)、§4.5(振动转换 API)。

## JS 接口

```typescript
// 创建转换实例
const converter = vibratorConvert(fd: RawFileDescriptor)

// 获取音频属性
const attr = await converter.getAudioAttribute()
// { sampleRate, duration }

// 获取音频数据
const audioData = await converter.getAudioData(samplingInterval?)

// 执行转换
const hapticEvents = await converter.convertAudioToHaptic(settings: AudioSetting)
```

### 转换配置

| 参数 | 范围 | 说明 |
|------|------|------|
| `transientDetection` | 0-100 | 瞬态检测阈值 |
| `intensityTreshold` | 0-100 | 强度阈值 |
| `frequencyTreshold` | 0-100 | 频率阈值 |
| `frequencyMaxValue` | Hz | 最大频率 |
| `frequencyMinValue` | Hz | 最小频率 |

## HapticEvent 结构

```cpp
struct HapticEvent {
    VibrateTag vibrateTag;   // CONTINUOUS / TRANSIENT
    int32_t startTime;        // 开始时间 (ms)
    int32_t duration;         // 持续时间 (ms)
    int32_t intensity;        // 强度 (0-100)
    int32_t frequency;        // 频率 (Hz)
};
```

详见 codewiki core.md §5.1.6(HapticEvent)。

## 依赖

`vibration_convert` 是独立算法模块，不依赖 `services/`，只依赖 `utils/common`（基础工具）和 `cJSON`（JSON 生成）。
