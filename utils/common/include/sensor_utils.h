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

#ifndef SENSOR_UTILS_H
#define SENSOR_UTILS_H

#include <charconv>
#include <cstdint>
#include <string>
#include <system_error>

namespace OHOS {
namespace Sensors {

template<typename T>
bool IsEqual(const T &left, const T &right)
{
    return std::abs(left - right) <= std::numeric_limits<T>::epsilon();
}

inline bool ParseDecimalInt32(const std::string &text, int32_t &out)
{
    if (text.empty()) {
        return false;
    }
    int32_t value = 0;
    const char *first = text.data();
    const char *last = first + text.size();
    auto result = std::from_chars(first, last, value);
    if (result.ec != std::errc() || result.ptr != last) {
        return false;
    }
    out = value;
    return true;
}

constexpr int32_t SINGLE_DISPLAY_SMALL_FOLD = 4;
constexpr int32_t SINGLE_DISPLAY_THREE_FOLD = 6;
constexpr int32_t SINGLE_DISPLAY_HP_FOLD = 7;
constexpr int32_t SINGLE_DISPLAY_LAP_FOLD = 8;
enum class DMDeviceStatus : uint32_t {
    UNKNOWN = 0,
    STATUS_FOLDED,
    STATUS_TENT_HOVER,
    STATUS_TENT,
    STATUS_GLOBAL_FULL,
    STATUS_EXPAND,
    STATUS_COORDINATION
};

enum class FoldDisplayMode: uint32_t {
    UNKNOWN = 0,
    FULL = 1,
    MAIN = 2,
    SUB = 3,
    COORDINATION = 4,
    GLOBAL_FULL = 5,
};
} // namespace Sensors
} // namespace OHOS
#endif // SENSOR_UTILS_H
