/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sensor_dump.h"

#include <getopt.h>

#include <cinttypes>
#include <cstring>
#include <ctime>
#include <queue>

#include "securec.h"
#include "sensors_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "SensorDump" };
constexpr int32_t MAX_DUMP_PARAMETERS = 32;
constexpr uint32_t MAX_DUMP_DATA_SIZE = 10;
constexpr uint32_t MS_NS = 1000000;
constexpr uint32_t ACCELEROMETER = 1;
constexpr uint32_t GYROSCOPE = 2;
constexpr uint32_t AMBIENT_LIGHT = 5;
constexpr uint32_t MAGNETIC_FIELD = 6;
constexpr uint32_t BAROMETER = 8;
constexpr uint32_t HALL = 10;
constexpr uint32_t PROXIMITY = 12;
constexpr uint32_t HUMIDITY = 13;
constexpr uint32_t ORIENTATION = 256;
constexpr uint32_t GRAVITY = 257;
constexpr uint32_t LINEAR_ACCELERATION = 258;
constexpr uint32_t ROTATION_VECTOR = 259;
constexpr uint32_t AMBIENT_TEMPERATURE = 260;
constexpr uint32_t MAGNETIC_FIELD_UNCALIBRATED = 261;
constexpr uint32_t GAME_ROTATION_VECTOR = 262;
constexpr uint32_t GYROSCOPE_UNCALIBRATED = 263;
constexpr uint32_t SIGNIFICANT_MOTION = 264;
constexpr uint32_t PEDOMETER_DETECTION = 265;
constexpr uint32_t PEDOMETER = 266;
constexpr uint32_t GEOMAGNETIC_ROTATION_VECTOR = 277;
constexpr uint32_t HEART_RATE = 278;
constexpr uint32_t DEVICE_ORIENTATION = 279;
constexpr uint32_t WEAR_DETECTION = 280;
constexpr uint32_t ACCELEROMETER_UNCALIBRATED = 281;

enum {
    SOLITARIES_DIMENSION = 1,
    COMMON_DIMENSION = 3,
    VECTOR_DIMENSION = 4,
    POSE_6DOF_DIMENSION = 15,
    UNCALIBRATED_DIMENSION = 6,
    DEFAULT_DIMENSION = 16,
};
}  // namespace

std::unordered_map<uint32_t, std::string> SensorDump::sensorMap_ = {
    { ACCELEROMETER, "ACCELEROMETER" },
    { ACCELEROMETER_UNCALIBRATED, "ACCELEROMETER UNCALIBRATED" },
    { LINEAR_ACCELERATION, "LINEAR ACCELERATION" },
    { GRAVITY, "GRAVITY" },
    { GYROSCOPE, "GYROSCOPE" },
    { GYROSCOPE_UNCALIBRATED, "GYROSCOPE UNCALIBRATED" },
    { SIGNIFICANT_MOTION, "SIGNIFICANT MOTION" },
    { PEDOMETER_DETECTION, "PEDOMETER DETECTION" },
    { PEDOMETER, "PEDOMETER" },
    { AMBIENT_TEMPERATURE, "AMBIENT TEMPERATURE" },
    { MAGNETIC_FIELD, "MAGNETIC FIELD" },
    { MAGNETIC_FIELD_UNCALIBRATED, "MAGNETIC FIELD UNCALIBRATED" },
    { HUMIDITY, "HUMIDITY" },
    { BAROMETER, "BAROMETER" },
    { DEVICE_ORIENTATION, "DEVICE ORIENTATION" },
    { ORIENTATION, "ORIENTATION" },
    { ROTATION_VECTOR, "ROTATION VECTOR" },
    { GAME_ROTATION_VECTOR, "GAME ROTATION VECTOR" },
    { GEOMAGNETIC_ROTATION_VECTOR, "GEOMAGNETIC ROTATION VECTOR" },
    { PROXIMITY, "PROXIMITY" },
    { AMBIENT_LIGHT, "AMBIENT LIGHT" },
    { HALL, "HALL" },
    { HEART_RATE, "HEART RATE" },
    { WEAR_DETECTION, "WEAR DETECTION" },
};

void SensorDump::ParseCommand(int32_t fd, const std::vector<std::string> &args, const std::vector<Sensor> &sensors,
    ClientInfo &clientInfo)
{
    int32_t count = 0;
    for (const auto &str : args) {
        if (str.find("--") == 0) {
            ++count;
            continue;
        }
        if (str.find("-") == 0) {
            count += str.size() - 1;
            continue;
        }
    }
    if (count > MAX_DUMP_PARAMETERS) {
        SEN_HILOGE("cmd param number not more than 32");
        dprintf(fd, "cmd param number not more than 32\n");
        return;
    }
    int32_t optionIndex = 0;
    struct option dumpOptions[] = {
        {"channel", no_argument, 0, 'c'},
        {"data", no_argument, 0, 'd'},
        {"open", no_argument, 0, 'o'},
        {"help", no_argument, 0, 'h'},
        {"list", no_argument, 0, 'l'},
        {NULL, 0, 0, 0}
    };
    char **argv = new (std::nothrow) char *[args.size()];
    CHKPV(argv);
    if (memset_s(argv, args.size() * sizeof(char *), 0, args.size() * sizeof(char *)) != EOK) {
        SEN_HILOGE("Call memset_s failed");
        delete[] argv;
        return;
    }
    for (size_t i = 0; i < args.size(); ++i) {
        argv[i] = new (std::nothrow) char[args[i].size() + 1];
        if (argv[i] == nullptr) {
            SEN_HILOGE("alloc failure");
            goto RELEASE_RES;
        }
        if (strcpy_s(argv[i], args[i].size() + 1, args[i].c_str()) != EOK) {
            SEN_HILOGE("strcpy_s error");
            goto RELEASE_RES;
        }
    }
    optind = 1;
    int32_t c;
    while ((c = getopt_long(args.size(), argv, "cdohl", dumpOptions, &optionIndex)) != -1) {
        switch (c) {
            case 'c': {
                DumpSensorChannel(fd, clientInfo);
                break;
            }
            case 'd': {
                DumpSensorData(fd, clientInfo);
                break;
            }
            case 'o': {
                DumpOpeningSensor(fd, sensors, clientInfo);
                break;
            }
            case 'h': {
                DumpHelp(fd);
                break;
            }
            case 'l': {
                DumpSensorList(fd, sensors);
                break;
            }
            default: {
                dprintf(fd, "Unrecognized option: %s\nMore info with: \"hidumper -s 3601 -a -h\"\n", argv[optind-1]);
                break;
            }
        }
    }
    RELEASE_RES:
    for (size_t i = 0; i < args.size(); ++i) {
        delete[] argv[i];
    }
    delete[] argv;
}

void SensorDump::DumpHelp(int32_t fd)
{
    dprintf(fd, "Usage:\n");
    dprintf(fd, "      -h, --help: dump help\n");
    dprintf(fd, "      -l, --list: dump the sensor list\n");
    dprintf(fd, "      -c, --channel: dump the sensor data channel info\n");
    dprintf(fd, "      -o, --open: dump the opening sensors\n");
    dprintf(fd, "      -d, --data: dump the last 10 packages sensor data\n");
}

bool SensorDump::DumpSensorList(int32_t fd, const std::vector<Sensor> &sensors)
{
    DumpCurrentTime(fd);
    dprintf(fd, "Total sensor:%d, Sensor list:\n", int32_t { sensors.size() });
    for (const auto &sensor : sensors) {
        auto sensorId = sensor.GetSensorId();
        dprintf(fd,
                "sensorId:%8u | sensorType:%s | sensorName:%s | vendorName:%s | maxRange:%f"
                "| fifoMaxEventCount:%d | minSamplePeriodNs:%" PRId64 " | maxSamplePeriodNs:%" PRId64 "\n",
                sensorId, sensorMap_[sensorId].c_str(), sensor.GetSensorName().c_str(), sensor.GetVendorName().c_str(),
                sensor.GetMaxRange(), sensor.GetFifoMaxEventCount(), sensor.GetMinSamplePeriodNs(),
                sensor.GetMaxSamplePeriodNs());
    }
    return true;
}

bool SensorDump::DumpSensorChannel(int32_t fd, ClientInfo &clientInfo)
{
    DumpCurrentTime(fd);
    dprintf(fd, "Sensor channel info:\n");
    std::vector<SensorChannelInfo> channelInfo;
    clientInfo.GetSensorChannelInfo(channelInfo);
    for (const auto &channel : channelInfo) {
        auto sensorId = channel.GetSensorId();
        dprintf(fd,
                "uid:%d | packageName:%s | sensorId:%8u | sensorType:%s | samplingPeriodNs:%" PRId64 ""
                "| fifoCount:%u\n",
                channel.GetUid(), channel.GetPackageName().c_str(), sensorId, sensorMap_[sensorId].c_str(),
                channel.GetSamplingPeriodNs(), channel.GetFifoCount());
    }
    return true;
}

bool SensorDump::DumpOpeningSensor(int32_t fd, const std::vector<Sensor> &sensors, ClientInfo &clientInfo)
{
    DumpCurrentTime(fd);
    dprintf(fd, "Opening sensors:\n");
    for (const auto &sensor : sensors) {
        uint32_t sensorId = sensor.GetSensorId();
        if (clientInfo.GetSensorState(sensorId)) {
            dprintf(fd, "sensorId: %8u | sensorType: %s | channelSize: %lu\n",
                sensorId, sensorMap_[sensorId].c_str(), clientInfo.GetSensorChannel(sensorId).size());
        }
    }
    return true;
}

bool SensorDump::DumpSensorData(int32_t fd, ClientInfo &clientInfo)
{
    dprintf(fd, "Last 10 packages sensor data:\n");
    auto dataMap = clientInfo.GetDumpQueue();
    int32_t j = 0;
    for (auto &sensorData : dataMap) {
        uint32_t sensorId = sensorData.first;
        dprintf(fd, "sensorId: %8u | sensorType: %s:\n", sensorId, sensorMap_[sensorId].c_str());
        for (uint32_t i = 0; i < MAX_DUMP_DATA_SIZE && (!sensorData.second.empty()); i++) {
            auto data = sensorData.second.front();
            sensorData.second.pop();
            timespec time = { 0, 0 };
            clock_gettime(CLOCK_REALTIME, &time);
            struct tm *timeinfo = localtime(&(time.tv_sec));
            CHKPF(timeinfo);
            dprintf(fd, "      %2d (ts=%.9f, time=%02d:%02d:%02d.%03d) | data:%s", ++j, data.timestamp / 1e9,
                    timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, int32_t { (time.tv_nsec / MS_NS) },
                    GetDataBySensorId(sensorId, data).c_str());
        }
    }
    return true;
}

void SensorDump::DumpCurrentTime(int32_t fd)
{
    timespec curTime = { 0, 0 };
    clock_gettime(CLOCK_REALTIME, &curTime);
    struct tm *timeinfo = localtime(&(curTime.tv_sec));
    CHKPV(timeinfo);
    dprintf(fd, "Current time: %02d:%02d:%02d.%03d\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
            int32_t { (curTime.tv_nsec / MS_NS) });
}

int32_t SensorDump::DataSizeBySensorId(uint32_t sensorId)
{
    switch (sensorId) {
        case AMBIENT_LIGHT:
        case BAROMETER:
        case HALL:
        case PROXIMITY:
        case HUMIDITY:
        case AMBIENT_TEMPERATURE:
        case SIGNIFICANT_MOTION:
        case PEDOMETER_DETECTION:
        case PEDOMETER:
        case HEART_RATE:
        case WEAR_DETECTION:
            return SOLITARIES_DIMENSION;
        case ROTATION_VECTOR:
            return VECTOR_DIMENSION;
        case MAGNETIC_FIELD_UNCALIBRATED:
        case GYROSCOPE_UNCALIBRATED:
        case ACCELEROMETER_UNCALIBRATED:
            return UNCALIBRATED_DIMENSION;
        default:
            SEN_HILOGW("sensorId: %{public}u,size: %{public}d", sensorId, COMMON_DIMENSION);
            return COMMON_DIMENSION;
    }
}

std::string SensorDump::GetDataBySensorId(uint32_t sensorId, TransferSensorEvents &sensorData)
{
    SEN_HILOGD("sensorId:%{public}u", sensorId);
    std::string str;
    int32_t dataLen = DataSizeBySensorId(sensorId);
    auto data = reinterpret_cast<float *>(sensorData.data);
    for (int32_t i = 0; i < dataLen; ++i) {
        str.append(std::to_string(*data));
        if (i != dataLen - 1) {
            str.append(",");
        }
        ++data;
    }
    str.append("\n");
    return str;
}
}  // namespace Sensors
}  // namespace OHOS
