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

#ifndef FREQUENCY_ESTIMATION_H
#define FREQUENCY_ESTIMATION_H

#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>

#include "utils.h"

namespace OHOS {
namespace Sensors {
/**
Conversion functions
*/
class FrequencyEstimation {
public:
    FrequencyEstimation() = default;
    ~FrequencyEstimation() = default;

    /**
     * @brief Get the zero crossing rate
     *
     * @param data Audio data.
     * @param frmLength The length of a frame, usually to the nth power of 2.
     * @param hopLength The length of the window to be processed in a frame, usually to the nth power of 2.
     *
     * @return std::vector<double> Zero crossing rate'vector.
     */
    std::vector<double> GetZeroCrossingRate(const std::vector<double> &data, int32_t frmLength, int32_t hopLength);

    /**
     * @brief Invalid frequency processing
     * The default value of freqReaderAbsFlag is true.
     *
     * @param frequencyHz The calculated frequency includes invalid frequencies.
     * @param voiceSegmentFlag Set of voice and silent segment flags.
     * @param rmseIntensityNorm Normalized intensity values.
     * @param freqNorm Normalized frequency values.
     */
    void FreqPostProcess(const std::vector<double> &frequencyHz, const std::vector<bool> &voiceSegmentFlag,
        const std::vector<int32_t> &rmseIntensityNorm, std::vector<int32_t> &freqNorm);

private:
    double Mean(const std::vector<double> &data);

private:
    bool freqReaderAbsFlag_ { true };
};
} // namespace Sensors
} // namespace OHOS
#endif // FREQUENCY_ESTIMATION_H