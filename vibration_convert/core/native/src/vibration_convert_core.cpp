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

#include <algorithm>
#include <numeric>

#include "generate_vibration_json_file.h"
#include "sensor_log.h"
#include "sensors_errors.h"
#include "vibration_convert_core.h"

namespace OHOS {
namespace Sensors {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, SENSOR_LOG_DOMAIN, "VibrationConvertCore" };
constexpr double AMP_INVALIDE_DELTA { 0.001 };
constexpr int32_t FRAGMENT_MIN_LEN { 32 };
constexpr double COEF { 0.01 };
constexpr double RMS_INVALIDE_DELTA { 0.04 };
// A transient event is distributed within three 1024 (46ms) windows
constexpr int32_t ONSET_ONE_WIN { 3072 };
// A transient event rising range
constexpr double ONSET_SPLIT_RATIO { 0.33334 };
constexpr double LOWER_AMP { 0.03 };
constexpr double FRAME_DURATION { 0.046 }; // 0.046s
constexpr double TRANSIENT_DURATION_DEFAULT { 0.03 };
constexpr double CONTINOUS_MIN_DURATION { 0.01 };
constexpr int32_t FRAME_LEN { 2048 };
constexpr int32_t SILENCE_COUNT_MIN { 32 };
constexpr int32_t CONTINUOUS_EVENT_LOW_DELTA_MULTIPLES { 5 };
constexpr int32_t ONE_TRANSIENT_DURATION_MS { 30 };
constexpr int32_t ONSET_MINSKIP_MAX { 5 };
constexpr int32_t ONSET_MINSKIP_MIN { 1 };
constexpr double TRANSIENT_UNZERO_DENSITY_MIN { 0.3 };
constexpr double DB_ZOOM_COEF { 3.846 }; // 100 / 26
constexpr int32_t TRANSIENT_EVENT_INTENSITY_MIN { 30 };
constexpr int32_t TRANSIENT_EVENT_FREQUENCY_MIN { 80 };
constexpr double RMSEMAX_THRESHOLD_HIGH { 0.4 };
constexpr double RMSEMAX_THRESHIOLD_MID { 0.2 };
constexpr double RMSE_LOWDELTA_RATIO_HIGH { 0.1 };
constexpr double RMSE_LOWDELTA_RATIO_MID { 0.05 };
constexpr double RMSE_LOWDELTA_RATIO_LOW { 0.03 };
constexpr double RMSE_LOWDELTA_RATIO_STEP { 0.01 };
constexpr int32_t RMSE_LOWDELTA_ITERATION_TIMES { 8 };
constexpr double SLOP_DELTA_MIN { 0.1745329251 }; // PI / 18
constexpr int32_t RESAMPLE_MULTIPLE { 4 };
constexpr size_t LOCAL_ENVELOPE_MAX_LEN { 16 };
constexpr size_t MIN_SKIP { 3 };
constexpr double AMPLITUDE_DB_MAX { 1.0 };
constexpr int32_t ADSR_BOUNDARY_STATUS_NONE { 0 };
constexpr int32_t ADSR_BOUNDARY_STATUS_ONE { 1 };
constexpr int32_t ADSR_BOUNDARY_STATUS_BOTH { 2 };
}  // namespace

int32_t VibrationConvertCore::AudioToHaptic(const AudioSetting &audioSetting, const std::vector<double> &audioData,
    std::vector<HapticEvent> &hapticEvents)
{
    CALL_LOG_ENTER;
    if (audioData.empty()) {
        SEN_HILOGE("audioData is empty");
        return Sensors::ERROR;
    }
    audioSetting_ = audioSetting;
    int32_t ret = ResampleAudioData(audioData, srcAudioData_);
    if (ret != Sensors::SUCCESS) {
        SEN_HILOGE("ResampleAudioData failed");
        return ret;
    }
    std::vector<double> data = PreprocessAudioData();
    int32_t onsetHopLength = WINDOW_LENGTH;// window length
    double rmsILowerDelta = 0.0;
    if (PreprocessParameter(data, onsetHopLength, rmsILowerDelta) != Sensors::SUCCESS) {
        SEN_HILOGE("PreprocessParameter failed");
        return Sensors::ERROR;
    }
    std::vector<UnionTransientEvent> unionTransientEvents;
    if (ConvertTransientEvent(data, onsetHopLength, unionTransientEvents) != Sensors::SUCCESS) {
        SEN_HILOGE("ConvertTransientEvent failed");
        return Sensors::ERROR;
    }
    std::vector<IntensityData> intensityData;
    // Processing intensity data, output parameters:intensityData
    ret = DetectRmsIntensity(data, rmsILowerDelta, intensityData);
    if (ret != Sensors::SUCCESS) {
        SEN_HILOGE("DetectRmsIntensity failed");
        return ret;
    }
    // Frequency detection
    std::vector<int32_t> rmseIntensityNorm;
    for (size_t i = 0; i < intensityData.size(); i++) {
        rmseIntensityNorm.push_back(intensityData[i].rmseIntensityNorm);
    }
    std::vector<int32_t> freqNorm = DetectFrequency(data, rmseIntensityNorm);
    if (freqNorm.empty()) {
        SEN_HILOGE("DetectFrequency failed");
        return Sensors::ERROR;
    }
    std::vector<int32_t> transientIndexes;
    std::vector<double> transientEventTime;
    // Output all Transient events, output parameters: transientIndex transientEventTime
    OutputTransientEvents(unionTransientEvents, intensityData, freqNorm, transientIndexes, transientEventTime);
    if (continuousEventExistFlag_) {
        std::vector<bool> transientEventFlags;
        for (size_t i = 0; i < unionTransientEvents.size(); i++) {
            transientEventFlags.push_back(unionTransientEvents[i].transientEventFlag);
        }
        OutputAllContinuousEvent(intensityData, transientIndexes, freqNorm, transientEventFlags);
    }
    StoreHapticEvent();
    hapticEvents = hapticEvents_;
    GenerateVibrationJsonFile jsonFile;
    jsonFile.GenerateJsonFile(hapticEvents_);
    return Sensors::SUCCESS;
}

int32_t VibrationConvertCore::ResampleAudioData(const std::vector<double> &srcData, std::vector<double> &dstData)
{
    if (srcData.empty()) {
        SEN_HILOGE("srcData is empty");
        return Sensors::ERROR;
    }
    size_t originDataSize = srcData.size();
    dstData.clear();
    for (size_t i = 0; i < (originDataSize-1); i += RESAMPLE_MULTIPLE) {
        dstData.push_back(srcData[i]);
        dstData.push_back(srcData[i+1]);
    }
    return Sensors::SUCCESS;
}

std::vector<double> VibrationConvertCore::PreprocessAudioData()
{
    CALL_LOG_ENTER;
    if (srcAudioData_.empty()) {
        SEN_HILOGE("invalid parameter");
        return {};
    }
    std::vector<double> absData = srcAudioData_;
    std::vector<double> dstData = srcAudioData_;
    int32_t silence = 0;
    int32_t preClearEnd = 0;
    int32_t absDataSize = static_cast<int32_t>(absData.size());
    for (int32_t i = 0; i < absDataSize; ++i) {
        absData[i] = std::fabs(absData[i]);
        if (absData[i] < AMP_INVALIDE_DELTA) {
            ++silence;
            continue;
        }
        if (silence <= SILENCE_COUNT_MIN) {
            silence = 0;
            continue;
        }
        int32_t curClearBegin = i - silence;
        for (int32_t j = curClearBegin; j < i; ++j) {
            dstData[j] = 0;
        }
        if (((curClearBegin - preClearEnd) <  FRAGMENT_MIN_LEN) && (curClearBegin > preClearEnd)) {
            for (int32_t j = preClearEnd; j < curClearBegin; ++j) {
                dstData[j] = 0;
            }
            preClearEnd = i;
        }
        silence = 0;
    }
    if (silence != 0) {
        int32_t curClearBegin = absDataSize - silence;
        for (int32_t i = curClearBegin; i < absDataSize; ++i) {
            dstData[i] = 0;
        }
        if (((curClearBegin - preClearEnd) < FRAGMENT_MIN_LEN) && (curClearBegin > preClearEnd)) {
            for (int32_t i = preClearEnd; i < curClearBegin; ++i) {
                dstData[i] = 0;
            }
        }
    }
    return dstData;
}

int32_t VibrationConvertCore::PreprocessParameter(const std::vector<double> &data, int32_t &onsetHopLength,
    double &lowerDelta)
{
    CALL_LOG_ENTER;
    if (data.empty()) {
        SEN_HILOGE("data is empty");
        return Sensors::ERROR;
    }
    std::vector<double> rmse = calcIntensity_.GetRMS(data, ENERGY_HOP_LEN, systemPara_.centerPaddingFlag);
    OnsetInfo onsetInfo;
    if (onset_.CheckOnset(data, NFFT, onsetHopLength, onsetInfo) != Sensors::SUCCESS) {
        SEN_HILOGE("CheckOnset Failed");
        return Sensors::ERROR;
    }
    std::vector<int32_t> newDrwIdx = MapOnsetHop(onsetInfo.idx, onsetHopLength);
    size_t dataSize = data.size();
    lowerDelta = CalcRmsLowerData(dataSize, rmse, newDrwIdx);
    double rmseMax = *std::max_element(rmse.begin(),rmse.end());
    size_t newDrwIdxLen = newDrwIdx.size();
    bool continuousEventFlag = false;
    int32_t newOnsetHopLen = onsetHopLength;
    // Amplitude density is used to classify audio files, determine whether there are long events in the audio file,
    // and adjust the onset detection window
    if (CalcOnsetHopLength(data, rmseMax, newDrwIdxLen, continuousEventFlag, newOnsetHopLen) != Sensors::SUCCESS) {
        SEN_HILOGE("CalcOnsetHopLength failed");
        return Sensors::ERROR;
    }
    continuousEventExistFlag_ = continuousEventFlag;
    onsetHopLength = newOnsetHopLen;
    return Sensors::SUCCESS;
}

std::vector<int32_t> VibrationConvertCore::MapOnsetHop(const std::vector<int32_t> &drwIdx, int32_t onsetHopLength)
{
    if (onsetHopLength == 0) {
        SEN_HILOGE("onsetHopLength is equal to 0");
        return {};
    }
    std::vector<int32_t> newIdx;
    int32_t coef = static_cast<int32_t>(round(static_cast<double>(ENERGY_HOP_LEN) / onsetHopLength));
    if (coef == 0) {
        SEN_HILOGE("coef is equal to 0");
        return {};
    }
    for (auto idx : drwIdx) {
        newIdx.push_back(static_cast<int32_t>(round(static_cast<double>(idx) / coef)));
    }
    return newIdx;
}

bool VibrationConvertCore::GetRmseLowerDelta(double lowerDelta, const std::vector<double> &rmse, double &deltaByTime)
{
    int32_t rmseSize = static_cast<int32_t>(rmse.size());
    double rmseMax = *std::max_element(rmse.begin(),rmse.end());
    double rmseMin = *std::min_element(rmse.begin(),rmse.end());
    double rmseRange = rmseMax - rmseMin;
    int32_t i = 0;
    deltaByTime = lowerDelta;
    for (i = 0; i < rmseSize; ++i) {
        if (rmse[i] > lowerDelta) {
            break;
        }
    }
    double rmsTimePerFrame = static_cast<double>(ENERGY_HOP_LEN) / SAMPLE_RATE;
    int32_t j = rmseSize - 1;
    for (; j >= 0 ; j = (j - 1)) {
        if (rmse[j] > lowerDelta) {
            break;
        }
    }
    if ((j - i) <= 0) {
        return false;
    }
    double totalDuration = (j - i) * rmsTimePerFrame;
    if (totalDuration < 1.0) { // 1s
        deltaByTime = rmseRange * RMSE_LOWDELTA_RATIO_LOW + rmseMin;
        return true;
    }
    return false;
}

double VibrationConvertCore::CalcRmsLowerData(size_t dataSize, const std::vector<double> &rmse,
    const std::vector<int32_t> &newDrwIdx)
{
    double soundSizeDelta = 0.0;
    double rmseMax = *std::max_element(rmse.begin(),rmse.end());
    double rmseMin = *std::min_element(rmse.begin(),rmse.end());
    double rmseRange = rmseMax - rmseMin;
    if (rmseMax > RMSEMAX_THRESHOLD_HIGH) {
        soundSizeDelta = rmseRange * RMSE_LOWDELTA_RATIO_HIGH + rmseMin;
    } else if (rmseMax > RMSEMAX_THRESHIOLD_MID) {
        soundSizeDelta = rmseRange * RMSE_LOWDELTA_RATIO_MID + rmseMin;
    } else {
        soundSizeDelta = rmseRange * RMSE_LOWDELTA_RATIO_LOW + rmseMin;
    }
    double lowerDelta = 0.0;
    if (newDrwIdx.size() > 0) {
        for (int32_t i = 0; i < RMSE_LOWDELTA_ITERATION_TIMES; i++) {
            int32_t j = newDrwIdx[0];
            lowerDelta = rmseRange * (RMSE_LOWDELTA_RATIO_HIGH - i * RMSE_LOWDELTA_RATIO_STEP ) + rmseMin;
            if ((rmse[j] > lowerDelta) || (rmse[j + 1] > lowerDelta)) {
                break;
            }
        }
    }
    double audioDurationDelta = soundSizeDelta;
    double audioDuration = static_cast<double>(dataSize) / SAMPLE_RATE;
    if (audioDuration <= 1.0) {
        audioDurationDelta = rmseRange * RMSE_LOWDELTA_RATIO_LOW + rmseMin;
        return audioDurationDelta;
    }
    double soundDurationDelta = soundSizeDelta;
    bool findFlag = GetRmseLowerDelta(soundSizeDelta, rmse, soundDurationDelta);
    if (findFlag) {
        return soundDurationDelta;
    }
    if (soundSizeDelta < lowerDelta) {
        return soundSizeDelta;
    }
    return lowerDelta;
}

int32_t VibrationConvertCore::CalcOnsetHopLength(const std::vector<double> &data, double rmseMax,
    size_t newDrwIdxLen, bool &continuousEventFlag, int32_t &newOnsetHopLen)
{
    if (data.empty()) {
        SEN_HILOGE("invalid parameter");
        return Sensors::ERROR;
    }
    int32_t longestCount = 0;
    double unzeroDensity = 0.0;
    bool continuousEventExistFlag = false;
    int32_t ret = IsIncludeContinuoustEvent(data, longestCount, unzeroDensity, continuousEventExistFlag);
    if (ret != Sensors::SUCCESS) {
        return ret;
    }
    if ((!continuousEventExistFlag) && (longestCount > ENERGY_HOP_LEN)) {
        if (rmseMax > (CONTINUOUS_EVENT_LOW_DELTA_MULTIPLES * RMS_INVALIDE_DELTA)) {
            continuousEventExistFlag = true;
        }
    }
    if ((!continuousEventExistFlag) || (newDrwIdxLen < 2)) {
        newOnsetHopLen = static_cast<int32_t>(round(ENERGY_HOP_LEN * INTERSITY_BOUNDARY_POINT));
    }
    continuousEventFlag = continuousEventExistFlag;
    return Sensors::SUCCESS;
}

std::vector<double> VibrationConvertCore::GetLocalEnvelope(const std::vector<double> &data)
{
    if (data.empty()) {
        SEN_HILOGE("invalid parameter");
        return {};
    }
    std::vector<double> envelope = data;
    for (auto &elem : envelope) {
        elem = std::fabs(elem);
    }
    double threshold = COEF * (*std::max_element(envelope.begin(), envelope.end()));
    for (auto &elem : envelope) {
        if (elem < threshold) {
            elem = 0;
        }
    }
    size_t dataSize = envelope.size();
    for (size_t i = 0; i < dataSize; ++i) {
        if ((i + LOCAL_ENVELOPE_MAX_LEN) > dataSize) {
            break;
        }
        std::vector<double> segmentEnvelope;
        for (size_t j = i; j < (i + LOCAL_ENVELOPE_MAX_LEN); ++j) {
            segmentEnvelope.push_back(envelope[j]);
        }
        envelope[i] = *std::max_element(segmentEnvelope.begin(), segmentEnvelope.end());
    }
    return envelope;
}

int32_t VibrationConvertCore::IsIncludeContinuoustEvent(const std::vector<double> &data,
    int32_t &longestCount, double &unzeroDensity, bool &isIncludeContinuoustEvent)
{
    // envelope must be a non-negative number.
    std::vector<double> envelope = GetLocalEnvelope(data);
    if (envelope.empty()) {
        SEN_HILOGE("GetLocalEnvelope failed");
        return Sensors::ERROR;
    }
    size_t envelopeSize = envelope.size();
    size_t j = 0;
    size_t k = 0;
    int32_t atLeastCnt = 2 * ENERGY_HOP_LEN;
    int32_t adsrCompleteStatus = ADSR_BOUNDARY_STATUS_NONE;
    std::vector<int32_t> countList;
    for (size_t i = 0; i < envelopeSize; ++i) {
        if ((envelope[i] >= EPS_MIN) && (adsrCompleteStatus == ADSR_BOUNDARY_STATUS_NONE)) {
            j = i;
            adsrCompleteStatus = ADSR_BOUNDARY_STATUS_ONE;
        }
        if ((envelope[i] < EPS_MIN) && (adsrCompleteStatus == ADSR_BOUNDARY_STATUS_ONE)) {
            k = i;
            adsrCompleteStatus = ADSR_BOUNDARY_STATUS_BOTH;
        }
        if (adsrCompleteStatus == ADSR_BOUNDARY_STATUS_BOTH) {
            adsrCompleteStatus = ADSR_BOUNDARY_STATUS_NONE;
            countList.push_back(k - j);
            if ((k - j) > atLeastCnt) {
                isIncludeContinuoustEvent = true;
                break;
            }
        }
    }
    if (countList.empty()) {
        SEN_HILOGI("countList is empty");
        longestCount = 0;
        unzeroDensity = 0;
        return Sensors::SUCCESS;
    }
    longestCount = *std::max_element(countList.begin(), countList.end());
    unzeroDensity = static_cast<double>(accumulate(countList.begin(), countList.end(), 0)) / envelopeSize;
    return Sensors::SUCCESS;
}
}  // namespace Sensors
}  // namespace OHOS
