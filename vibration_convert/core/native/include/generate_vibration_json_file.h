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

#ifndef GENERATE_VIBRATION_JSON_FILE_H
#define GENERATE_VIBRATION_JSON_FILE_H

#include <fstream>
#include <vector>

#include <unistd.h>

#include "vibration_convert_type.h"

namespace OHOS {
namespace Sensors {
class GenerateVibrationJsonFile {
public:
    GenerateVibrationJsonFile() = default;
    ~GenerateVibrationJsonFile() = default;
    int32_t GenerateJsonFile(std::vector<HapticEvent> &hapticEvents);
    template<typename T>
    int32_t DebugJsonFile(const std::string &pathName, const std::vector<T> &srcDatas);
};
}  // namespace Sensors
}  // namespace OHOS
#endif // GENERATE_VIBRATION_JSON_FILE_H