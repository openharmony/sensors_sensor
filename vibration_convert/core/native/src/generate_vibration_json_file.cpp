/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "generate_vibration_json_file.h"

#include <fstream>
#include <iostream>
#include <vector>

#include "json/json.h"

#include "sensor_log.h"
#include "sensors_errors.h"

#undef LOG_TAG
#define LOG_TAG "GenerateVibrationJsonFile"

namespace OHOS {
namespace Sensors {

int32_t GenerateVibrationJsonFile::GenerateJsonFile(std::vector<HapticEvent> &hapticEvents)
{
    Json::Value meta;
    meta["Create"] = "2023-04-27";
    meta["Discription"] = "A json file format demo";
    meta["Version"] = 1.0;
    meta["ChannelNumber"] = 1;
    Json::Value root;
    root["MetaData"] = meta;
    Json::Value pattern;
    for (const auto &event : hapticEvents) {
        Json::Value eventValue;
        eventValue["Type"] = ((event.vibrateTag != EVENT_TAG_TRANSIENT) ? "continuous" : "transient");
        eventValue["StartTime"] = event.startTime;
        eventValue["Parameters"]["Intensity"] = event.intensity;
        eventValue["Parameters"]["Frequency"] = event.frequency;
        if (event.vibrateTag != EVENT_TAG_TRANSIENT) {
            eventValue["Duration"] = event.duration;
        }
        Json::Value ev;
        ev["Event"] = eventValue;
        pattern.append(ev);
    }
    Json::Value channel;
    channel["Parameters"]["Index"] = 1;
    channel["Pattern"] = pattern;
    Json::Value channels;
    channels.append(channel);
    root["Channels"] = channels;
    std::ofstream ofs("demo.json", std::ios::out);
    if (!ofs.is_open()) {
        SEN_HILOGE("File open failed, errno:%{public}d", errno);
        return Sensors::ERROR;
    }
    ofs << root << std::endl;
    ofs.close();
    return Sensors::SUCCESS;
}

template<typename T>
int32_t GenerateVibrationJsonFile::DebugJsonFile(const std::string &pathName, const std::vector<T> &srcDatas)
{
    if (access(pathName.c_str(), 0) != 0) {
        SEN_HILOGE("File not exist, errno:%{public}d", errno);
        return Sensors::ERROR;
    }
    if (srcDatas.empty()) {
        SEN_HILOGE("srcDatas is empty");
        return Sensors::ERROR;
    }
    Json::Value dataValue;
    for (const auto &data : srcDatas) {
        dataValue.append(data);
    }
    std::ofstream ofs(pathName, std::ios::out);
    if (!ofs.is_open()) {
        SEN_HILOGE("File open failed, errno:%{public}d", errno);
        return Sensors::ERROR;
    }
    ofs << dataValue << std::endl;
    ofs.close();
    return Sensors::SUCCESS;
}
}  // namespace Sensors
}  // namespace OHOS
