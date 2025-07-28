/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "print_sensor_data.h"

#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
#include "hisysevent.h"
#endif // HIVIEWDFX_HISYSEVENT_ENABLE

#include "sensor_errors.h"

#undef LOG_TAG
#define LOG_TAG "PrintSensorData"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
namespace {
enum {
    ONE_DIMENSION = 1,
    TWO_DIMENSION = 2,
    THREE_DIMENSION = 3,
    FOUR_DIMENSION = 4,
    SEVEN_DIMENSION = 7,
    EIGHT_DIMENSION = 8,
    THIRTEEN_DIMENSION = 13,
    DEFAULT_DIMENSION = 16
};
constexpr int64_t LOG_INTERVAL = 60000000000L;
constexpr int32_t FIRST_PRINT_TIMES = 20;
constexpr float LOG_FORMAT_DIVIDER = 1e9f;

const std::vector<int32_t> g_triggerSensorType = {
    SENSOR_TYPE_ID_DROP_DETECTION,
    SENSOR_TYPE_ID_HALL,
    SENSOR_TYPE_ID_HALL_EXT,
    SENSOR_TYPE_ID_PROXIMITY,
    SENSOR_TYPE_ID_PROXIMITY1,
    SENSOR_TYPE_ID_WEAR_DETECTION,
};
const std::vector<int32_t> g_continuousSensorType = {
    SENSOR_TYPE_ID_ACCELEROMETER,
    SENSOR_TYPE_ID_AMBIENT_LIGHT,
    SENSOR_TYPE_ID_AMBIENT_LIGHT1,
    SENSOR_TYPE_ID_GRAVITY,
    SENSOR_TYPE_ID_GYROSCOPE,
    SENSOR_TYPE_ID_MAGNETIC_FIELD,
    SENSOR_TYPE_ID_ORIENTATION,
    SENSOR_TYPE_ID_POSTURE,
    SENSOR_TYPE_ID_ROTATION_VECTOR,
};
}

void PrintSensorData::ControlSensorHdiPrint(const SensorData &sensorData)
{
    auto triggerIt = std::find(g_triggerSensorType.begin(), g_triggerSensorType.end(), sensorData.sensorTypeId);
    if (triggerIt != g_triggerSensorType.end()) {
        PrintHdiData(sensorData);
        ProcessHdiDFX(sensorData);
    }
    std::lock_guard<std::mutex> hdiLoginfoLock(hdiLoginfoMutex_);
    auto it = hdiLoginfo_.find(sensorData.sensorTypeId);
    if (it == hdiLoginfo_.end()) {
        return;
    }
    it->second.hdiTimes++;
    if (it->second.count < FIRST_PRINT_TIMES) {
        PrintHdiData(sensorData);
        if (it->second.count == FIRST_PRINT_TIMES - 1) {
            it->second.lastTime = sensorData.timestamp;
        }
        it->second.count++;
    } else {
        if (sensorData.timestamp - it->second.lastTime >= LOG_INTERVAL) {
            PrintHdiData(sensorData);
            it->second.lastTime = sensorData.timestamp;
        }
    }
    if (it->second.hdiTimesFlag == 0) {
        it->second.hdiTimesFlag = sensorData.timestamp;
    }
    if (sensorData.timestamp - it->second.hdiTimesFlag >= LOG_INTERVAL) {
        SEN_HILOGI("sensorType:%{public}d, hdiTimes:%{public}s", sensorData.sensorTypeId,
            std::to_string(it->second.hdiTimes).c_str());
        it->second.hdiTimesFlag = sensorData.timestamp;
        it->second.hdiTimes = 0;
    }
}

void PrintSensorData::PrintHdiData(const SensorData &sensorData)
{
    std::string str;
    str += "deviceId:" + std::to_string(sensorData.deviceId) + ", ";
    str += "sensorType:" + std::to_string(sensorData.sensorTypeId) + ", ";
    str += "sensorId:" + std::to_string(sensorData.sensorId) + ", ";
    str += "location:" + std::to_string(sensorData.location) + ", ";
    str += "timestamp:" + std::to_string(sensorData.timestamp / LOG_FORMAT_DIVIDER) + ", ";
    int32_t dataDim = GetDataDimension(sensorData.sensorTypeId);
    auto data = reinterpret_cast<const float *>(sensorData.data);
    for (int32_t i = 0; i < dataDim; ++i) {
        CHKPV(data);
        str.append(std::to_string(*data));
        if (i != dataDim - 1) {
            str.append(", ");
        }
        ++data;
    }
    str.append("\n");
    SEN_HILOGI("SensorData:%{public}s", str.c_str());
}

void PrintSensorData::ProcessHdiDFX(const SensorData &sensorData)
{
    std::string strData;
    auto data = reinterpret_cast<const float *>(sensorData.data);
    int32_t dataDim = GetDataDimension(sensorData.sensorTypeId);
    for (int32_t i = 0; i < dataDim; i++) {
        CHKPV(data);
        strData.append(std::to_string(*data));
        if (i != dataDim - 1) {
            strData.append(", ");
        }
        ++data;
    }
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
    HiSysEventWrite(HiSysEvent::Domain::SENSOR, "EVENT_REPORT",
        HiSysEvent::EventType::BEHAVIOR, "SENSOR_ID", sensorData.sensorTypeId, "TIMESTAMP",
        std::to_string(sensorData.timestamp), "DATA", strData);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
}

int32_t PrintSensorData::GetDataDimension(int32_t sensorType)
{
    switch (sensorType) {
        case SENSOR_TYPE_ID_DROP_DETECTION:
        case SENSOR_TYPE_ID_HALL:
        case SENSOR_TYPE_ID_PROXIMITY:
        case SENSOR_TYPE_ID_WEAR_DETECTION:
            return ONE_DIMENSION;
        case SENSOR_TYPE_ID_AMBIENT_LIGHT:
        case SENSOR_TYPE_ID_AMBIENT_LIGHT1:
        case SENSOR_TYPE_ID_ACCELEROMETER:
        case SENSOR_TYPE_ID_GRAVITY:
        case SENSOR_TYPE_ID_GYROSCOPE:
        case SENSOR_TYPE_ID_MAGNETIC_FIELD:
        case SENSOR_TYPE_ID_ORIENTATION:
            return THREE_DIMENSION;
        case SENSOR_TYPE_ID_ROTATION_VECTOR:
            return FOUR_DIMENSION;
        case SENSOR_TYPE_ID_HALL_EXT:
            return EIGHT_DIMENSION;
        case SENSOR_TYPE_ID_POSTURE:
            return THIRTEEN_DIMENSION;
        default:
            SEN_HILOGW("Unknown sensorType:%{public}d, size:%{public}d", sensorType, DEFAULT_DIMENSION);
            return DEFAULT_DIMENSION;
    }
}

void PrintSensorData::ControlSensorClientPrint(const RecordSensorCallback callback, const SensorEvent &event)
{
    auto triggerIt = std::find(g_triggerSensorType.begin(), g_triggerSensorType.end(), event.sensorTypeId);
    if (triggerIt != g_triggerSensorType.end()) {
        PrintClientData(event);
        ProcessClientDFX(event);
    }

    auto continuosIt = std::find(g_continuousSensorType.begin(), g_continuousSensorType.end(), event.sensorTypeId);
    if (continuosIt == g_continuousSensorType.end()) {
        return;
    }
    std::lock_guard<std::mutex> clientLoginfoLock(clientLoginfoMutex_);
    auto it = clientLoginfo_.find(callback);
    if (it == clientLoginfo_.end()) {
        return;
    }
    it->second.clientTimes++;
    if (it->second.count < FIRST_PRINT_TIMES) {
        PrintClientData(event);
        if (it->second.count == FIRST_PRINT_TIMES - 1) {
            it->second.lastTime = event.timestamp;
        }
        it->second.count++;
    } else {
        if (event.timestamp - it->second.lastTime >= LOG_INTERVAL) {
            PrintClientData(event);
            it->second.lastTime = event.timestamp;
        }
    }
    if (it->second.clientTimesFlag == 0) {
        it->second.clientTimesFlag = event.timestamp;
    }
    if (event.timestamp - it->second.clientTimesFlag >= LOG_INTERVAL) {
        SEN_HILOGI("sensorType:%{public}d, clientTimes:%{public}s", event.sensorTypeId,
            std::to_string(it->second.clientTimes).c_str());
        it->second.clientTimesFlag = event.timestamp;
        it->second.clientTimes = 0;
    }
}

void PrintSensorData::PrintClientData(const SensorEvent &event)
{
    std::string str;
    str += "deviceId:" + std::to_string(event.deviceId) + ", ";
    str += "sensorType:" + std::to_string(event.sensorTypeId) + ", ";
    str += "sensorId:" + std::to_string(event.sensorId) + ", ";
    str += "location:" + std::to_string(event.location) + ", ";
    str += "timestamp:" + std::to_string(event.timestamp / LOG_FORMAT_DIVIDER) + ", ";
    int32_t dataDim = GetDataDimension(event.sensorTypeId);
    auto data = reinterpret_cast<const float *>(event.data);
    for (int32_t i = 0; i < dataDim; ++i) {
        CHKPV(data);
        str.append(std::to_string(*data));
        if (i != dataDim - 1) {
            str.append(", ");
        }
        ++data;
    }
    str.append("\n");
    SEN_HILOGI("SensorData:%{public}s", str.c_str());
}

void PrintSensorData::ProcessClientDFX(const SensorEvent &event)
{
    std::string strData;
    auto data = reinterpret_cast<const float *>(event.data);
    int32_t dataDim = GetDataDimension(event.sensorTypeId);
    for (int32_t i = 0; i < dataDim; i++) {
        CHKPV(data);
        strData.append(std::to_string(*data));
        if (i != dataDim - 1) {
            strData.append(", ");
        }
        ++data;
    }
#ifdef HIVIEWDFX_HISYSEVENT_ENABLE
    HiSysEventWrite(HiSysEvent::Domain::SENSOR, "EVENT_REPORT",
        HiSysEvent::EventType::BEHAVIOR, "SENSOR_ID", event.sensorTypeId, "TIMESTAMP",
        std::to_string(event.timestamp), "DATA", strData);
#endif // HIVIEWDFX_HISYSEVENT_ENABLE
}

bool PrintSensorData::IsContinuousType(int32_t sensorType)
{
    return std::find(g_continuousSensorType.begin(), g_continuousSensorType.end(),
        sensorType) != g_continuousSensorType.end();
}

void PrintSensorData::SavePrintUserInfo(const RecordSensorCallback callback)
{
    CHKPV(callback);
    std::lock_guard<std::mutex> clientLoginfoLock(clientLoginfoMutex_);
    if (clientLoginfo_.find(callback) != clientLoginfo_.end()) {
        return;
    }
    LogPrintInfo info;
    auto status = clientLoginfo_.insert(std::make_pair(callback, info));
    if (!status.second) {
        SEN_HILOGD("callback has been saved");
    }
}

void PrintSensorData::RemovePrintUserInfo(const RecordSensorCallback callback)
{
    CHKPV(callback);
    std::lock_guard<std::mutex> clientLoginfoLock(clientLoginfoMutex_);
    if (clientLoginfo_.find(callback) == clientLoginfo_.end()) {
        return;
    }
    clientLoginfo_.erase(callback);
}

void PrintSensorData::ResetHdiCounter(int32_t sensorType)
{
    std::lock_guard<std::mutex> hdiLoginfoLock(hdiLoginfoMutex_);
    auto it = hdiLoginfo_.find(sensorType);
    if (it == hdiLoginfo_.end()) {
        return;
    }
    it->second.count = 0;
    it->second.lastTime = 0;
}

void PrintSensorData::ResetHdiTimes(int32_t sensorType)
{
    std::lock_guard<std::mutex> hdiLoginfoLock(hdiLoginfoMutex_);
    auto it = hdiLoginfo_.find(sensorType);
    if (it == hdiLoginfo_.end()) {
        return;
    }
    it->second.hdiTimes = 0;
    it->second.hdiTimesFlag = 0;
}

void PrintSensorData::PrintSensorDataLog(const std::string &name, const SensorData &data)
{
    std::string str;
    str += "deviceId:" + std::to_string(data.deviceId) + ", ";
    str += "sensorType:" + std::to_string(data.sensorTypeId) + ", ";
    str += "sensorId:" + std::to_string(data.sensorId) + ", ";
    str += "location:" + std::to_string(data.location) + ", ";
    str += "timestamp:" + std::to_string(data.timestamp / LOG_FORMAT_DIVIDER) + ", ";
    int32_t dataDim = GetDataDimension(data.sensorTypeId);
    auto tempData = reinterpret_cast<const float *>(data.data);
    for (int32_t i = 0; i < dataDim; ++i) {
        CHKPV(tempData);
        str.append(std::to_string(*tempData));
        if (i != dataDim - 1) {
            str.append(", ");
        }
        ++tempData;
    }
    str.append("\n");
    SEN_HILOGI("%{public}s SensorData:%{public}s", name.c_str(), str.c_str());
}

void PrintSensorData::PrintSensorInfo(SensorInfo *sensorInfos, int32_t sensorInfoCount)
{
    std::string combineSensorIds = "";
    for (int32_t i = 0; i < sensorInfoCount; ++i) {
        combineSensorIds = combineSensorIds + std::to_string(sensorInfos[i].sensorTypeId) + " ";
    }
    SEN_HILOGI("PrintSensorInfo success, sensorIds:%{public}s, sensorInfoCount:%{public}d", combineSensorIds.c_str(),
        sensorInfoCount);
}
} // namespace Sensors
} // namespace OHOS
