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

#include "frequency_estimation.h"

#include <algorithm>
#include <iterator>
#include <numeric>

#include "sensor_log.h"
#include "sensors_errors.h"

#undef LOG_TAG
#define LOG_TAG "FrequencyEstimation"

namespace OHOS {
namespace Sensors {
namespace {
constexpr double BASE_SEMITONE { 69.0 };
constexpr double PITCH_INTERVAL_MIN { 12.0 };
constexpr double LA_FREQUENCE { 440.0 };
constexpr double PITCH_INTERVAL_MAX { 108.0 };
} // namespace

double FrequencyEstimation::Mean(const std::vector<double> &data)
{
    if (data.empty()) {
        SEN_HILOGE("data is empty");
        return 0.0;
    }
    double sumValue = accumulate(data.begin(), data.end(), 0.0);
    return (sumValue / data.size());
}

void FrequencyEstimation::FreqPostProcess(const std::vector<double> &frequencyHz, const std::vector<bool> &voiceSegmentFlag,
    const std::vector<int32_t> &rmseIntensityNorm, std::vector<int32_t> &freqNorm)
{
    // Processing of effective values for filling in mute positions.
    std::vector<double> hzTrims;
    int32_t ret = ProcessSilence(frequencyHz, voiceSegmentFlag, rmseIntensityNorm, hzTrims);
    if (ret != Sensors::SUCCESS) {
        SEN_HILOGE("ProcessSilence failed");
        return;
    }
    // Frequency conversion.
    std::vector<double> pitch(hzTrims.size(), 0.0);
    for (size_t i = 0; i < hzTrims.size(); ++i) {
        pitch[i] = BASE_SEMITONE + (PITCH_INTERVAL_MIN * log2(hzTrims[i] / LA_FREQUENCE));
    }
    if (freqReaderAbsFlag_) {
        freqNorm = OHOS::Sensors::NormalizePercentageRange(pitch, PITCH_INTERVAL_MIN,
            (PITCH_INTERVAL_MIN + PITCH_INTERVAL_MAX));
    } else {
        freqNorm = OHOS::Sensors::NormalizePercentageMin(pitch);
    }
    if (!freqNorm.empty() && (freqNorm.size() + 1) == rmseIntensityNorm.size()) {
        freqNorm.push_back(freqNorm.back());
    }
}

std::vector<double> FrequencyEstimation::GetZeroCrossingRate(const std::vector<double> &data, int32_t frmLength, int32_t hopLength)
{
    if (data.empty() || frmLength <= hopLength) {
        SEN_HILOGE("data is empty or frmLength is less than hopLength");
        return {};
    }
    size_t dataSize = data.size();
    size_t step = static_cast<size_t>(frmLength - hopLength);
    size_t frameNum = ceil(dataSize / step);
    std::vector<double> zcr(frameNum, 0.0);
    std::vector<double> oneFrmData;
    auto it = data.begin();
    for (size_t i = 0; i < frameNum; ++i) {
        int32_t beginIndex = i * step;
        int32_t endIndex = std::min(beginIndex + frmLength, static_cast<int32_t>(dataSize));
        oneFrmData.assign(it + beginIndex, it + endIndex);
        double curFrameMean = Mean(oneFrmData);
        int32_t crossingN = 0;
        size_t curFrameSize = oneFrmData.size();
        if (curFrameSize == 0) {
            SEN_HILOGE("oneFrmData is empty");
            return {};
        }
        for (size_t j = 0; j < (curFrameSize - 1); ++j) {
            oneFrmData[j] = oneFrmData[j] - curFrameMean;
            if (IsLessOrEqual(oneFrmData[j] * oneFrmData[j + 1], 0.0)) {
                ++crossingN;
            }
        }
        zcr[i] = (static_cast<double>(crossingN) / curFrameSize);
    }
    return zcr;
}
} // namespace Sensors
} // namespace OHOS