/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include <cinttypes>
#include <ctime>
#include <queue>

#include "sensors_errors.h"
#include "sensors_log_domain.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, SensorsLogDomain::SENSOR_SERVICE, "SensorDump" };
constexpr uint32_t MAX_DUMP_DATA_SIZE = 10;
constexpr uint32_t MS_NS = 1000000;
constexpr uint32_t ACCELEROMETER = 256;
constexpr uint32_t ACCELEROMETER_UNCALIBRATED = 65792;
constexpr uint32_t LINEAR_ACCELERATION = 131328;
constexpr uint32_t GRAVITY = 196864;
constexpr uint32_t GYROSCOPE = 262400;
constexpr uint32_t GYROSCOPE_UNCALIBRATED = 327936;
constexpr uint32_t SIGNIFICANT_MOTION = 393472;
constexpr uint32_t DROP_DETECTION = 459008;
constexpr uint32_t PEDOMETER_DETECTION = 524544;
constexpr uint32_t PEDOMETER = 590080;
constexpr uint32_t AMBIENT_TEMPERATURE = 16777472;
constexpr uint32_t MAGNETIC_FIELD = 16843008;
constexpr uint32_t MAGNETIC_FIELD_UNCALIBRATED = 16908544;
constexpr uint32_t HUMIDITY = 16974080;
constexpr uint32_t BAROMETER = 17039616;
constexpr uint32_t SAR = 17105152;
constexpr uint32_t SIXDOF_ATTITUDE = 33554688;
constexpr uint32_t SCREEN_ROTATION = 33620224;
constexpr uint32_t DEVICE_ORIENTATION = 33685760;
constexpr uint32_t ORIENTATION = 33751296;
constexpr uint32_t ROTATION_VECTOR = 33816832;
constexpr uint32_t GAME_ROTATION_VECTOR = 33882368;
constexpr uint32_t GEOMAGNETIC_ROTATION_VECTOR = 33947904;
constexpr uint32_t PROXIMITY = 50331904;
constexpr uint32_t TOF = 50397440;
constexpr uint32_t AMBIENT_LIGHT = 50462976;
constexpr uint32_t COLOR_TEMPERATURE = 50528512;
constexpr uint32_t COLOR_RGB = 50594048;
constexpr uint32_t COLOR_XYZ = 50659584;
constexpr uint32_t HALL = 67109120;
constexpr uint32_t GRIP_DETECTOR = 67174656;
constexpr uint32_t MAGNET_BRACKET = 67240192;
constexpr uint32_t PRESSURE_DETECTOR = 67305728;
constexpr uint32_t HEART_RATE = 83886336;
constexpr uint32_t WEAR_DETECTION = 83951872;

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
    { DROP_DETECTION, "DROP DETECTION" },
    { PEDOMETER_DETECTION, "PEDOMETER DETECTION" },
    { PEDOMETER, "PEDOMETER" },
    { AMBIENT_TEMPERATURE, "AMBIENT TEMPERATURE" },
    { MAGNETIC_FIELD, "MAGNETIC FIELD" },
    { MAGNETIC_FIELD_UNCALIBRATED, "MAGNETIC FIELD UNCALIBRATED" },
    { HUMIDITY, "HUMIDITY" },
    { BAROMETER, "BAROMETER" },
    { SAR, "SAR" },
    { SIXDOF_ATTITUDE, "6DOF ATTITUDE" },
    { SCREEN_ROTATION, "SCREEN ROTATION" },
    { DEVICE_ORIENTATION, "DEVICE ORIENTATION" },
    { ORIENTATION, "ORIENTATION" },
    { ROTATION_VECTOR, "ROTATION VECTOR" },
    { GAME_ROTATION_VECTOR, "GAME ROTATION VECTOR" },
    { GEOMAGNETIC_ROTATION_VECTOR, "GEOMAGNETIC ROTATION VECTOR" },
    { PROXIMITY, "PROXIMITY" },
    { TOF, "TOF" },
    { AMBIENT_LIGHT, "AMBIENT LIGHT" },
    { COLOR_TEMPERATURE, "COLOR TEMPERATURE" },
    { COLOR_RGB, "COLOR RGB" },
    { COLOR_XYZ, "COLOR XYZ" },
    { HALL, "HALL" },
    { GRIP_DETECTOR, "GRIP DETECTOR" },
    { MAGNET_BRACKET, "MAGNET BRACKET" },
    { PRESSURE_DETECTOR, "PRESSURE DETECTOR" },
    { HEART_RATE, "HEART RATE" },
    { WEAR_DETECTION, "WEAR DETECTION" },
};

bool SensorDump::DumpSensorHelp(int32_t fd, const std::vector<std::u16string> &args)
{
    if ((args.empty()) || (args[0].compare(u"-h") != 0)) {
        SEN_HILOGE("args cannot be empty or invalid");
        return false;
    }
    DumpHelp(fd);
    return true;
}

void SensorDump::DumpHelp(int32_t fd)
{
    dprintf(fd, "Usage:\n");
    dprintf(fd, "      -h: dump help\n");
    dprintf(fd, "      -l: dump the sensor list\n");
    dprintf(fd, "      -c: dump the sensor data channel info\n");
    dprintf(fd, "      -o: dump the opening sensors\n");
    dprintf(fd, "      -d: dump the last 10 packages sensor data\n");
}

bool SensorDump::DumpSensorList(int32_t fd, const std::vector<Sensor> &sensors, const std::vector<std::u16string> &args)
{
    if ((args.empty()) || (args[0].compare(u"-l") != 0)) {
        SEN_HILOGE("args cannot be empty or invalid");
        return false;
    }
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

bool SensorDump::DumpSensorChannel(int32_t fd, ClientInfo &clientInfo, const std::vector<std::u16string> &args)
{
    if ((args.empty()) || (args[0].compare(u"-c") != 0)) {
        SEN_HILOGE("args cannot be empty or invalid");
        return false;
    }
    DumpCurrentTime(fd);
    dprintf(fd, "Sensor channel info:\n");
    std::vector<SensorChannelInfo> channelInfo;
    clientInfo.GetSensorChannelInfo(channelInfo);
    for (const auto &channel : channelInfo) {
        auto sensorId = channel.GetSensorId();
        std::string cmds("");
        auto cmdList = channel.GetCmdType();
        for (auto cmd : cmdList) {
            cmds += (std::to_string(cmd) + " ");
        }
        dprintf(fd,
                "uid:%d | packageName:%s | sensorId:%8u | sensorType:%s | samplingPeriodNs:%d "
                "| fifoCount:%u | cmdType:%s\n",
                channel.GetUid(), channel.GetPackageName().c_str(), sensorId, sensorMap_[sensorId].c_str(),
                int32_t { channel.GetSamplingPeriodNs() }, channel.GetFifoCount(), cmds.c_str());
    }
    return true;
}

bool SensorDump::DumpOpeningSensor(int32_t fd, const std::vector<Sensor> &sensors, ClientInfo &clientInfo,
                                   const std::vector<std::u16string> &args)
{
    if ((args.empty()) || (args[0].compare(u"-o") != 0)) {
        SEN_HILOGE("args cannot be empty or invalid");
        return false;
    }
    DumpCurrentTime(fd);
    dprintf(fd, "Opening sensors:\n");
    for (const auto &sensor : sensors) {
        uint32_t sensorId = sensor.GetSensorId();
        if (clientInfo.GetSensorState(sensorId)) {
            dprintf(fd, "sensorId: %8u | sensorType: %s\n", sensorId, sensorMap_[sensorId].c_str());
        }
    }
    return true;
}

bool SensorDump::DumpSensorData(int32_t fd, ClientInfo &clientInfo, const std::vector<std::u16string> &args)
{
    if ((args.empty()) || (args[0].compare(u"-d") != 0)) {
        SEN_HILOGE("args cannot be empty or invalid");
        return false;
    }
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
        case SIXDOF_ATTITUDE:
            return POSE_6DOF_DIMENSION;
        case ACCELEROMETER_UNCALIBRATED:
        case MAGNETIC_FIELD_UNCALIBRATED:
        case GYROSCOPE_UNCALIBRATED:
            return UNCALIBRATED_DIMENSION;
        case GEOMAGNETIC_ROTATION_VECTOR:
        case GAME_ROTATION_VECTOR:
        case ROTATION_VECTOR:
            return VECTOR_DIMENSION;
        case SIGNIFICANT_MOTION:
        case PEDOMETER_DETECTION:
        case PEDOMETER:
        case AMBIENT_TEMPERATURE:
        case HUMIDITY:
        case HEART_RATE:
        case DEVICE_ORIENTATION:
        case WEAR_DETECTION:
            return SOLITARIES_DIMENSION;
        default:
            return COMMON_DIMENSION;
    }
}

std::string SensorDump::GetDataBySensorId(uint32_t sensorId, struct SensorEvent &sensorData)
{
    SEN_HILOGD("sensorId: %{public}u", sensorId);
    std::string buffer;
    int32_t dataLen = DataSizeBySensorId(sensorId);
    for (int32_t i = 0; i < dataLen; ++i) {
        if (sensorId >= ACCELEROMETER && sensorId <= PEDOMETER) {
            buffer.append(std::to_string(sensorData.data[i]));
        } else if (sensorId >= AMBIENT_TEMPERATURE && sensorId <= SAR) {
            buffer.append(std::to_string(sensorData.data[i]));
        } else if (sensorId >= SIXDOF_ATTITUDE && sensorId <= GEOMAGNETIC_ROTATION_VECTOR) {
            buffer.append(std::to_string(sensorData.data[i]));
        } else if (sensorId >= PROXIMITY && sensorId <= COLOR_XYZ) {
            buffer.append(std::to_string(sensorData.data[i]));
        } else if (sensorId >= HALL && sensorId <= PRESSURE_DETECTOR) {
            buffer.append(std::to_string(sensorData.data[i]));
        } else if (sensorId >= HEART_RATE && sensorId <= WEAR_DETECTION) {
            buffer.append(std::to_string(sensorData.data[i]));
        }
        buffer.append(",");
    }
    buffer.append("\n");
    return buffer;
}
}  // namespace Sensors
}  // namespace OHOS
