# 传感器算法

> 地磁/海拔/四元数/旋转矩阵等算法，位于 `frameworks/native/src/sensor_algorithm.cpp` 和 `geomagnetic_field.cpp`。

## 算法接口

| 函数 | 功能 | 实现位置 |
|------|------|----------|
| `getGeomagneticField(location, timeMillis)` | 基于 WMM 2020-2025 计算地磁场分量 | `geomagnetic_field.cpp` |
| `getAltitude(seaPressure, currentPressure)` | 根据气压计算海拔高度 | `sensor_algorithm.cpp` |
| `getGeomagneticDip(inclinationMatrix)` | 计算地磁倾角 | `sensor_algorithm.cpp` |
| `getDirection(rotationMatrix)` / `getOrientation` | 从旋转矩阵计算方向角 | `sensor_algorithm.cpp` |
| `createQuaternion(rotationVector)` | 从旋转矢量创建四元数 | `sensor_algorithm.cpp` |
| `transformCoordinateSystem(inR, axis)` | 坐标系变换 | `sensor_algorithm.cpp` |
| `getAngleModify(curR, preR)` | 计算角度变化量 | `sensor_algorithm.cpp` |
| `createRotationMatrix(rotationVector, inclinationMatrix?)` | 创建旋转矩阵 | `sensor_algorithm.cpp` |

详见 codewiki modules.md §1 §2.3(传感器算法计算)、§2 §2.6(传感器算法)。

## GeomagneticField

基于 WMM（World Magnetic Model）2020-2025 模型，输入经纬度和时间，输出地磁场分量（X/Y/Z/总强度/倾角/偏角）。

详见 codewiki core.md §2(功能列表 - 地磁场计算)。

## 调用方式

JS 层通过 NAPI 调用，支持 Promise 和 Callback 两种模式：

```typescript
// Promise 模式
const result = await sensor.getGeomagneticField({latitude, longitude, altitude}, timeMillis)

// Callback 模式
sensor.getGeomagneticField(location, timeMillis, (err, data) => {})
```

算法在异步线程执行，结果通过 `napi_resolve_deferred`（Promise）或 `napi_call_function`（Callback）返回。

详见 codewiki modules.md §1 §8.2(Promise vs Callback 模式)。
