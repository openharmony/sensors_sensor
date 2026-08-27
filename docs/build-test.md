# 构建和测试方法

## 构建目标

编译配置见 `bundle.json`，构建目标从 OpenHarmony 源码根目录执行：

| group_type | target |
|------------|--------|
| fwk_group | sensor_js_target, cj_sensor_ffi, sensor_target, ohsensor, sensor_taihe, sensor_utils_target |
| service_group | sensor_service_target, sensors_sa_profiles |

### 编译产物

| 产物 | 来源 |
|------|------|
| libsensor_service.z.so | services/ |
| libsensor_utils.z.so | utils/common/ |
| libsensor_ipc.z.so | utils/ipc/ |
| libsensor.z.so | frameworks/js/napi/ |
| libsensor_native.z.so | frameworks/native/ |
| libsensor_vibration_convert.z.so | vibration_convert/ |

## 最小验证

```sh
# 检查 Sensor Service SA 3601 是否存活
hdc shell "hidumper -s 3601"

# 查看传感器服务进程
hdc shell "ps -ef | grep sensors"
```

## 测试

测试目标定义在 `bundle.json` 的 `build.test` 中：

| 测试类型 | 路径 | target |
|----------|------|--------|
| JS 接口单元测试 | test/unittest/interfaces/js/ | unittest |
| C API单元测试 | test/unittest/interfaces/kits/ | unittest |
| 内部 API 单元测试 | test/unittest/interfaces/inner_api/ | unittest |
| 服务层 Fuzz | test/fuzztest/services/ | fuzztest |
| 接口 Fuzz | test/fuzztest/interfaces/ | fuzztest |
| 独立 Fuzz | test/fuzztest/sensor_fuzzer/ | fuzztest |
| 覆盖率 | test/unittest/coverage/ | unittest |

## 调试

```sh
# 查看传感器日志
hilog -x | grep -i sensor

# 开启传感器调试日志（需 root）
hdc shell "lshilog -s sensor -l D"

# 查看 SA 3601 状态（含传感器列表、订阅状态）
hdc shell "hidumper -s 3601"

# 查看传感器设备信息
hdc shell "cat /dev/sensor_info"

# 查看 HiSysEvent 传感器事件
hdc shell "hisysevent -l"
```

> 涉及真实传感器硬件的行为，需要补充板侧证据。提交使用 `git commit -s`。
