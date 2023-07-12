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

#include "peak_finder.h"

#include <algorithm>
#include <cmath>
#include <iterator>

#include "sensor_log.h"
#include "sensors_errors.h"
#include "utils.h"

namespace OHOS {
namespace Sensors {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "PeakFinder" };
constexpr size_t MAX_N { 32 };
constexpr int32_t PEAKS_MIN_SAMPLE_COUNT { 512 };
constexpr double REMOVE_RATIO { 0.1 };
constexpr double PEAKMAX_THRESHOLD_HIGH { 0.6 };
constexpr double PEAKMAX_THRESHIOLD_MID { 0.4 };
constexpr double PEAK_LOWDELTA_RATIO_HIGH { 0.3 };
constexpr double PEAK_LOWDELTA_RATIO_MID { 0.2 };
constexpr double PEAK_LOWDELTA_RATIO_LOW { 0.1 };
constexpr int32_t LOOP_TIMES_MAX { 1000000 };
constexpr size_t GRADIENT_SIZE_MIN { 3 };
// The smaller the value, the faster the descent speed
constexpr double DOWN_TREND_MAX { 0.3 };
constexpr size_t DESCENT_WNDLEN { 128 };
constexpr int32_t HUNDRED_POINT_DESCENT_HEIGHT { 100 };
constexpr double DROP_HIGHT { 1.0 };
constexpr int32_t AMPLITUDE_ENVELOPE_HOP_LENGTH { 256 };
constexpr double DROP_HIGHT_THRESHOLD { 0.3 }; // 30%
}  // namespace

std::vector<double> PeakFinder::ExtractValues(const std::vector<double> &envelope, const std::vector<int32_t> &idxs)
{
    size_t envelopSize = envelope.size();
    size_t idxsSize = idxs.size();
    std::vector<double> values;
    for (size_t i = 0; i < idxsSize; i++) {
        if (idxs[i] >= envelopSize) {
            break;
        }
        values.push_back(envelope[idxs[i]]);
    }
    return values;
}

// In order to reduce the impact of voiceless and noise on the frequency of voiced sounds, the threshold is increased.
std::vector<bool> PeakFinder::GetVoiceFlag(const std::vector<double> &data, const std::vector<int32_t> &peaks, double lowerAmp)
{
    if (data.empty()) {
        SEN_HILOGE("data is empty");
        return {};
    }
    std::vector<int32_t> peakAmp;
    for (size_t i = 0; i < peaks.size(); i++) {
        if (peaks[i] > data.size()) {
            break;
        }
        peakAmp.push_back(fabs(data[peaks[i]]));
    }
    lowerAmp = *max_element(peakAmp.begin(), peakAmp.end()) * INTERSITY_BOUNDARY_POINT;
    if (peakAmp.size() > 2) {
        lowerAmp = peakAmp[static_cast<int32_t>(peakAmp.size() * INTERSITY_NUMBER_BOUNDARY_POINT )];
    }
    std::vector<double> triangularEnvelope(data.size(), 0.0);
    for (size_t i = 0; i < data.size(); i++) {
        triangularEnvelope[i] = fabs(data[i]);
    }
    double threshold = lowerAmp;
    for (size_t i = 0; i < triangularEnvelope.size(); i++) {
        if (triangularEnvelope[i] < threshold) {
            triangularEnvelope[i] = 0;
        }
    }
    size_t envelopeLength = triangularEnvelope.size();
    auto it = triangularEnvelope.begin();
    for (size_t i = 0; i < envelopeLength; i++) {
        if ((i + MAX_N) >= envelopeLength) {
            break;
        }
        triangularEnvelope[i] = *max_element(it + i, it + i + MAX_N);
    }
    MountainPosition mountainPosition;
    // Obtain the independent envelope of each peak point
    GetEachIndependentEnvelope(triangularEnvelope, peaks, lowerAmp, mountainPosition);
    // Obtain sound and silent envelopes
    return SplitVoiceSlienceRange(envelopeLength, mountainPosition.firstPos, mountainPosition.lastPos);
}

std::vector<bool> PeakFinder::GetVoiceSegmentFlag() const
{
    return voiceSegmentFlag_;
}

// Obtain soundand silent envelopes.
std::vector<bool> PeakFinder::SplitVoiceSlienceRange(int32_t dataSize, const std::vector<int32_t> &envelopeStart,
    const std::vector<int32_t> &envelopeLast)
{
    if (dataSize < 0 || hopLength_ == 0) {
        SEN_HILOGE("Invalid parameter");
        return {};
    }
    std::vector<bool> vioceFlag(dataSize, false);
    std::vector<bool> segmentFlag(ceil(static_cast<double>(dataSize) / hopLength_), false);
    for (size_t k = 0; k < envelopeStart.size(); k++) {
        for (int32_t boundary = envelopeStart[k]; boundary <= envelopeLast[k]; boundary++) {
            if (boundary > vioceFlag.size()) {
                break;
            }
            vioceFlag[boundary] = true;
        }
    }
    for (int32_t i = 0; i < dataSize; ) {
        if (vioceFlag[i]) {
            int32_t j = i / hopLength_;
            segmentFlag[j] = true;
            i = (j + 1) * hopLength_;
        } else {
            ++i;
        }
    }
    return segmentFlag;
}

// Obtain the independent envelope of each peak point.
void PeakFinder::GetEachIndependentEnvelope(const std::vector<double> &data, const std::vector<int32_t> &peaks,
    double lowerAmp, MountainPosition &mountainPosition)
{
    int32_t lastLeftPos = -1;
    int32_t lastRightPos = -1;
    bool findBoundary = false;
    BothSidesOfPeak bothSides;
    for (size_t i = 0; i < peaks.size(); i++) {
        int32_t index = peaks[i];
        // Peak points in the same envelope.
        if ((index >= lastLeftPos) && (index < lastRightPos)) {
            mountainPosition.firstPos.push_back(bothSides.leftPos);
            mountainPosition.peakPos.push_back(index);
            mountainPosition.lastPos.push_back(bothSides.rightPos);
            continue;
        }
        // Find boundary point from peak to both sides.
        findBoundary = FindPeakBoundary(data, index, lowerAmp, bothSides);
        if (findBoundary) {
            lastLeftPos = bothSides.leftPos;
            lastRightPos = bothSides.rightPos;
            mountainPosition.firstPos.push_back(bothSides.leftPos);
            mountainPosition.peakPos.push_back(index);
            mountainPosition.lastPos.push_back(bothSides.rightPos);
        }
    }
}

// Find boundary point from peak to both sides.
bool PeakFinder::FindPeakBoundary(const std::vector<double> &data, int32_t peakPlace, double threshold,
    BothSidesOfPeak &bothSides)
{
    if (data.empty() || (peakPlace > data.size())) {
        SEN_HILOGE("data is empty or peakPlace greater than data");
        return false;
    }
    bool findLeft = false;
    int32_t i = 0;
    for (i = peakPlace; i >= 0; i--) {
        if (data[i] < threshold) {
            findLeft = true;
            bothSides.leftPos = i;
            break;
        }
    }
    // Zero hour high opening
    if (!findLeft && (i == -1)) {
        bothSides.leftPos = 0;
        findLeft = true;
    }
    size_t dataSize = data.size();
    bool findRight = false;
    size_t j = 0;
    // Find Right
    for (j = peakPlace; j < dataSize; j++) {
        if (data[j] < threshold) {
            findRight = true;
            bothSides.rightPos = j;
            break;
        }
    }
    if (!findRight && (j == dataSize)) {
        bothSides.rightPos = dataSize - 1;
        findRight = true;
    }
    return (findLeft && findRight);
}

// Based on the time difference between peak points, retain a larger value and filter the peak points.
std::vector<int32_t> PeakFinder::PeakFilterMinRange(const std::vector<double> &data, std::vector<int32_t> &peaks,
    int32_t minSampleCount)
{
    if (peaks.size() <= 1) {
        return peaks;
    }
    for (size_t i = 0; i < (peaks.size() - 1); i++) {
        if (fabs(peaks[i] - peaks[i + 1]) < minSampleCount) {
            if (peaks[i + 1] > data.size()) {
                SEN_HILOGW("peaks value greater than data size");
                return peaks;
            }
            if (data[peaks[i]] >= data[peaks[i + 1]]) {
                auto it = (peaks.begin() + i + 1);
                peaks.erase(it);
            } else {
                auto it = (peaks.begin() + i);
                peaks.erase(it);
            }
            --i;
        }
    }
    return peaks;
}

// As a short event peak, directly remove peaks smaller than 0.1 * max().
std::vector<int32_t> PeakFinder::FilterLowPeak(const std::vector<double> &envelope, const std::vector<int32_t> &peaks,
    double removeRatio)
{
    std::vector<int32_t> peakPoint = peaks;
    size_t peaksSize = peakPoint.size();
    if (peaksSize == 0) {
        return peakPoint;
    }
    std::vector<double> peakValue;

    for (size_t i = 0; i < peaksSize; i++) {
        peakValue.push_back(envelope[peakPoint[i]]);
    }
    double threshold = *max_element(peakValue.begin(), peakValue.end()) * removeRatio;
    for (int32_t i = (peaksSize - 1); i >= 0; i--) {
        if (peakValue[i] < threshold) {
            peakPoint.erase(peakPoint.begin() + i);
        }
    }
    return peakPoint;
}

// Filter secondary peak points.
// Like heartbeat.wav, mp5.wav and clavels.wav process here.
std::vector<int32_t> PeakFinder::FilterSecondaryPeak(const std::vector<double> &envelope,
    const std::vector<int32_t> &peaks, double lowerAmp)
{
    MountainPosition wholeEnvelop;
    // Obtain the independent envelope of each peak point
    GetEachIndependentEnvelope(envelope, peaks, lowerAmp, wholeEnvelop);
    if (wholeEnvelop.firstPos.empty()) {
        SEN_HILOGE("firstPos is empty");
        return {};
    }
    int32_t lastFirstPos = wholeEnvelop.firstPos[0];
    int32_t lastEndPos = wholeEnvelop.lastPos[0];
    size_t frontIndex = 0;
    int32_t n = 0;
    int32_t times = 0;
    size_t i = 1;
    while (i < wholeEnvelop.peakPos.size()) {
        ++times;
        if ((wholeEnvelop.firstPos[i] == lastFirstPos) && (wholeEnvelop.lastPos[i] == lastEndPos)) {
            ++n;
            if (i != (wholeEnvelop.peakPos.size() - 1)) {
                ++i;
                continue;
            }
        }
        if (n == 0) {
            lastFirstPos = wholeEnvelop.firstPos[i];
            lastEndPos = wholeEnvelop.lastPos[i];
            frontIndex = i;
            ++i;
            continue;
        }
        if (frontIndex == (wholeEnvelop.peakPos.size() - 1)) {
            break;
        }
        int32_t toIndex = frontIndex + n;
        // The peak points are enveloped together, and the drop of the secondary peak is less than 30 %.
        // Delete this peak point
        int32_t index = 0;
        if (DeletePeaks(envelope, frontIndex, toIndex, wholeEnvelop, index) != Sensors::SUCCESS) {
            SEN_HILOGE("DeletePeaks failed");
            return {};
        }
        if (times > LOOP_TIMES_MAX) {
            SEN_HILOGW("Entered die process, %{public}d", times);
            break;
        }
        frontIndex = index;
        i = index;
        n = 0;
    }
    return wholeEnvelop.peakPos;
}

// The peak points are enveloped together, and the drop of the secondary peak is less than 30 %. Delete this peak point.
int32_t PeakFinder::DeletePeaks(const std::vector<double> &envelope, int32_t startPos, int32_t endPos,
    MountainPosition &mountainPosition, int32_t &endIndex)
{
    bool delFlag = true;
    while (delFlag) {
        ValleyPoint valleyPoint;
        // The valley in both peaks detection.
        if (DetectValley(envelope, startPos, endPos, mountainPosition, valleyPoint) != Sensors::SUCCESS) {
            SEN_HILOGE("DetectValley failed");
            return Sensors::ERROR;
        }
        std::vector<double> valleysValue = valleyPoint.values;
        // Filter out secondary peak points
        delFlag = false;
        int32_t valleyIndex = 0;
        for (int32_t peakIndex = startPos; peakIndex < (endPos + 1); peakIndex++) {
            delFlag = GetDeleteFlagOfPeak(envelope, peakIndex, valleyIndex, valleysValue, mountainPosition);
            if (delFlag && (peakIndex < mountainPosition.firstPos.size())) {
                mountainPosition.firstPos.erase(mountainPosition.firstPos.begin() + peakIndex);
                mountainPosition.peakPos.erase(mountainPosition.peakPos.begin() + peakIndex);
                mountainPosition.lastPos.erase(mountainPosition.lastPos.begin() + peakIndex);
                --endPos;
                break;
            }
            ++valleyIndex;
        }
        if (endPos <= startPos) {
            break;
        }
    }
    endIndex = endPos;
    return Sensors::SUCCESS;
}

bool PeakFinder::GetDeleteFlagOfPeak(const std::vector<double> &envelope, int32_t peakIndex, int32_t valleyIndex,
    const std::vector<double> &valleysValue, const MountainPosition &mountainPosition)
{
    if (valleyIndex >= valleysValue.size() || peakIndex >= mountainPosition.peakPos.size()) {
        SEN_HILOGW("invalid parameter");
        return false;
    }
    double peakValue = envelope[mountainPosition.peakPos[peakIndex]];
    double hightThreshold = DROP_HIGHT_THRESHOLD * peakValue;
    bool delFlag = false;
    if (((peakValue - valleysValue[valleyIndex]) < hightThreshold) &&
        ((peakValue - valleysValue[valleyIndex + 1]) < hightThreshold)) {
        delFlag = true;
    } else if (((peakValue - valleysValue[valleyIndex]) > hightThreshold) &&
        ((peakValue - valleysValue[valleyIndex + 1]) < hightThreshold)) {
        if (peakValue < envelope[mountainPosition.peakPos[peakIndex+1]]) {
            delFlag = true;
        }
    } else if (((peakValue - valleysValue[valleyIndex]) < hightThreshold) &&
        ((peakValue - valleysValue[valleyIndex + 1]) > hightThreshold)) {
        if (peakValue < envelope[mountainPosition.peakPos[peakIndex-1]]) {
            delFlag = true;
        }
    } else {
        delFlag = false;
    }
    return delFlag;
}
}  // namespace Sensors
}  // namespace OHOS
