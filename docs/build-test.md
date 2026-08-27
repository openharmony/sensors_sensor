# 构建和测试方法

## 编译

构建命令从 OpenHarmony 源码根目录执行：

```sh
# 编译 sensor 部件（含框架+服务）
./build.sh --product-name rk3568 --build-target sensor --ccache

# 单独编译框架层
prebuilts/build-tools/linux-x86/bin/ninja -C out/rk3568 sensor_target

# 单独编译服务层
prebuilts/build-tools/linux-x86/bin/ninja -C out/rk3568 sensor_service_target
```

### 编译产物

| 产物 | 路径 | 来源 |
|------|------|------|
| libsensor_service.z.so | `out/{device}/system/lib64/` | services/ |
| libsensor_utils.z.so | `out/{device}/system/lib64/` | utils/common/ |
| libsensor_ipc.z.so | `out/{device}/system/lib64/` | utils/ipc/ |
| libsensor.z.so | `out/{device}/system/lib64/` | frameworks/js/napi/ |
| libsensor_native.z.so | `out/{device}/system/lib64/` | frameworks/native/ |
| libsensor_vibration_convert.z.so | `out/{device}/system/lib64/` | vibration_convert/ |

### 构建目标（bundle.json）

| group_type | target |
|------------|--------|
| fwk_group | sensor_js_target, cj_sensor_ffi, sensor_target, ohsensor, sensor_taihe, sensor_utils_target |
| service_group | sensor_service_target, sensors_sa_profiles |

## 最小验证

```sh
# 1. 检查 Sensor Service SA 3601 是否存活
hdc shell "hidumper -s 3601"

# 2. 查看传感器服务进程
hdc shell "ps -ef | grep sensors"

# 3. 检查 SA 配置文件
hdc shell "cat /system/profile/3601.json"
```

## 测试

```sh
# 单元测试 target（来自 bundle.json build.test）
# JS 接口测试
prebuilts/build-tools/linux-x86/bin/ninja -C out/rk3568 unittest

# Fuzz 测试
prebuilts/build-tools/linux-x86/bin/ninja -C out/rk3568 fuzztest
```

| 测试类型 | 路径 | target |
|----------|------|--------|
| JS 接口单元测试 | test/unittest/interfaces/js/ | unittest |
| NDK 接口单元测试 | test/unittest/interfaces/kits/ | unittest |
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

## 部署推送

```sh
# 编译后推送 .so 到设备
hdc file send out/rk3568/system/lib64/libsensor_service.z.so /system/lib64/
hdc file send out/rk3568/system/lib64/libsensor_utils.z.so /system/lib64/
hdc file send out/rk3568/system/lib64/libsensor_ipc.z.so /system/lib64/

# 重启传感器服务
hdc shell "kill -9 $(pidof sensors)"  # SA 会被自动拉起
```

> 涉及真实传感器硬件的行为，需要补充板侧证据。提交使用 `git commit -s`。
