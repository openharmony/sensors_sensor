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

void VibrationConvertCore::StoreHapticEvent()
{
    CALL_LOG_ENTER;
    bool blockStoreFlag = systemPara_.blockStoreFlag;
    if (blockStoreFlag) {
        StoreEventBlock();
    } else {
        StoreEventSequence();
    }
}

void VibrationConvertCore::StoreEventSequence()
{
    HapticEvent eventData;
    if (continuousEvents_.empty()) {
        for (size_t i = 0; i < transientEvents_.size(); ++i) {
            eventData.vibrateTag = EVENT_TAG_TRANSIENT;
            eventData.startTime = static_cast<int32_t>(round(transientEvents_[i].time * SAMPLE_IN_MS));
            eventData.duration = ONE_TRANSIENT_DURATION_MS;
            eventData.intensity = transientEvents_[i].intensity;
            eventData.frequency = transientEvents_[i].frequency;
            hapticEvents_.push_back(eventData);
        }
        return;
    }
    double preTime = -1.0; // invalide time.
    for (size_t i = 0; i < continuousEvents_.size(); ++i) {
        for (size_t j = 0; j < transientEvents_.size(); ++j) {
            int32_t seStartTime = static_cast<int32_t>(round(transientEvents_[j].time * SAMPLE_IN_MS));
            int32_t leStartTime = static_cast<int32_t>(round(continuousEvents_[i].time * SAMPLE_IN_MS));
            if ((seStartTime > static_cast<int32_t>(round(preTime * SAMPLE_IN_MS))) && (seStartTime < leStartTime)) {
                eventData.vibrateTag = EVENT_TAG_TRANSIENT;
                eventData.startTime = static_cast<int32_t>(round(transientEvents_[j].time * SAMPLE_IN_MS));
                eventData.duration = ONE_TRANSIENT_DURATION_MS;
                eventData.intensity = transientEvents_[j].intensity;
                eventData.frequency = transientEvents_[j].frequency;
                hapticEvents_.push_back(eventData);
            }
        }
        eventData.vibrateTag = EVENT_TAG_CONTINUOUS;
        eventData.startTime = static_cast<int32_t>(round(continuousEvents_[i].time * SAMPLE_IN_MS));
        eventData.duration = static_cast<int32_t>(round(continuousEvents_[i].duration * SAMPLE_IN_MS));
        eventData.intensity = continuousEvents_[i].intensity;
        eventData.frequency = continuousEvents_[i].frequency;
        hapticEvents_.push_back(eventData);
        preTime = continuousEvents_[i].time;
    }
}

void VibrationConvertCore::StoreEventBlock()
{
    if (continuousEvents_.empty()) {
        SEN_HILOGW("continuousEvents_ is empty");
        return;
    }
    HapticEvent eventData;
    ContinuousEvent continuousEvent = *(continuousEvents_.end() - 1);
    for (size_t i = 0; i < transientEvents_.size(); ++i) {
        if (transientEvents_[i].time > continuousEvent.time) {
            break;
        }
        eventData.vibrateTag = EVENT_TAG_TRANSIENT;
        eventData.startTime = static_cast<int32_t>(round(transientEvents_[i].time * SAMPLE_IN_MS));
        eventData.duration = ONE_TRANSIENT_DURATION_MS;
        eventData.intensity = transientEvents_[i].intensity;
        eventData.frequency = transientEvents_[i].frequency;
        hapticEvents_.push_back(eventData);
    }
    for (size_t i = 0; i < continuousEvents_.size(); ++i) {
        eventData.vibrateTag = EVENT_TAG_CONTINUOUS;
        eventData.startTime = static_cast<int32_t>(round(continuousEvents_[i].time * SAMPLE_IN_MS));
        eventData.duration = static_cast<int32_t>(round(continuousEvents_[i].duration * SAMPLE_IN_MS));
        eventData.intensity = continuousEvents_[i].intensity;
        eventData.frequency = continuousEvents_[i].frequency;
        hapticEvents_.push_back(eventData);
    }
}

void VibrationConvertCore::EmplaceOnsetTime(bool flag, int32_t idx, double time,
    std::vector<UnionTransientEvent> &unionTransientEvents)
{
    if (unionTransientEvents.empty()) {
        SEN_HILOGE("unionTransientEvents is empty");
        return;
    }
    if (idx < unionTransientEvents[0].onsetIdx) {
            UnionTransientEvent unionSeData(idx, time, flag);
            unionTransientEvents.emplace(unionTransientEvents.begin(), unionSeData);
    }
    if (idx > unionTransientEvents[unionTransientEvents.size() - 1].onsetIdx) {
        UnionTransientEvent unionSeData(idx, time, flag);
        unionTransientEvents.push_back(unionSeData);
    }
    for (size_t j = 0; j < (unionTransientEvents.size() - 1); ++j) {
        if ((idx > unionTransientEvents[j].onsetIdx) && (idx < unionTransientEvents[j + 1].onsetIdx)) {
            size_t index = j + 1;
            UnionTransientEvent unionSeData(idx, time, flag);
            unionTransientEvents.emplace(unionTransientEvents.begin() + index, unionSeData);
            break;
        }
    }
}

int32_t VibrationConvertCore::ConvertTransientEvent(const std::vector<double> &data, int32_t onsetHopLength,
    std::vector<UnionTransientEvent> &unionTransientEvents)
{
    CALL_LOG_ENTER;
    if (data.empty()) {
        SEN_HILOGE("data is empty");
        return Sensors::ERROR;
    }
    // Using System Methods to Detect Onset
    std::vector<UnionTransientEvent> unionTransientValues = DetectOnset(data, onsetHopLength);
    // Is the onset just a transient event
    std::vector<int32_t> onsetIdxs;
    for (size_t i = 0; i < unionTransientValues.size(); i++) {
        onsetIdxs.push_back(unionTransientValues[i].onsetIdx);
    }
    std::vector<bool> transientEventFlags = IsTransientEvent(data, onsetIdxs);
    for (size_t i = 0; i < unionTransientValues.size(); i++) {
        unionTransientValues[i].transientEventFlag = transientEventFlags[i];
    }
    IsolatedEnvelopeInfo isolatedEnvelopeInfo;
    int32_t ret = peakFinder_.GetTransientByAmplitude(data, isolatedEnvelopeInfo);
    if (ret != Sensors::SUCCESS) {
        SEN_HILOGE("GetTransientByAmplitude failed.");
        return ret;
    }
    if (!isolatedEnvelopeInfo.isHaveContinuousEvent) {
        unionTransientValues.clear();
        TranslateAnchorPoint(isolatedEnvelopeInfo.mountainPosition.peakPos, unionTransientValues);
        for (size_t i = 0; i < unionTransientValues.size(); ++i) {
            unionTransientValues[i].transientEventFlag = isolatedEnvelopeInfo.transientEventFlags[i];
        }
    } else {
        size_t size =  isolatedEnvelopeInfo.mountainPosition.peakPos.size();
        for (size_t i = 0; i <size; ++i) {
            if (!isolatedEnvelopeInfo.transientEventFlags[i]) {
                continue;
            }
            bool flag = isolatedEnvelopeInfo.transientEventFlags[i];
            int32_t idx = 0;
            double time = 0.0;
            TranslateAnchorPoint(isolatedEnvelopeInfo.mountainPosition.peakPos[i], idx, time);
            size_t findIndex = -1;
            for (size_t j = 0; j < unionTransientValues.size(); j++) {
                if (unionTransientValues[j].onsetIdx == idx) {
                    findIndex = j;
                    break;
                }
            }
            if (findIndex != -1) {
                unionTransientValues[findIndex].onsetTime = time;
                unionTransientValues[findIndex].transientEventFlag = flag;
            } else {
                EmplaceOnsetTime(flag, idx, time, unionTransientValues);
            }
        }
    }
    unionTransientEvents = unionTransientValues;
    return Sensors::SUCCESS;
}

std::vector<UnionTransientEvent> VibrationConvertCore::DetectOnset(const std::vector<double> &data,
    int32_t onsetHopLength)
{
    OnsetInfo onsetInfo;
    if (onset_.CheckOnset(data, NFFT, onsetHopLength, onsetInfo) != Sensors::SUCCESS) {
        SEN_HILOGE("CheckOnset Failed");
        return {};
    }
    onsetInfo.idx = MapOnsetHop(onsetInfo.idx, onsetHopLength);
    std::vector<int32_t> newIdx;
    std::vector<double> newTime;
    IdxUnique(onsetInfo.idx, onsetInfo.time, newIdx, newTime);
    int32_t minSkip = ONSET_MINSKIP_MAX;
    if (newIdx.size() > 1) {
        std::vector<int32_t> idxDiff;
        for (size_t i = 1; i < newIdx.size(); ++i) {
            idxDiff.push_back(newIdx[i] - newIdx[i - 1]);
        }
        minSkip = *std::min_element(idxDiff.begin(),idxDiff.end());
    }
    if (minSkip < ONSET_MINSKIP_MIN) {
        SEN_HILOGE("minSkip is less than 1");
        return {};
    }
    bool onsetBacktrackFlag = systemPara_.onsetBacktrackFlag;
    if (onsetBacktrackFlag) {
        minSkip = ONSET_MINSKIP_MIN;
    } else {
        if (minSkip > ONSET_MINSKIP_MAX) {
            minSkip = ONSET_MINSKIP_MAX;
        }
    }
    std::vector<UnionTransientEvent> unionTransientEvents;
    for (size_t i = 0; i < newIdx.size(); ++i) {
        unionTransientEvents.push_back(UnionTransientEvent(newIdx[i], newTime[i]));
    }
    onsetMinSkip_ = minSkip;
    return unionTransientEvents;
}

bool VibrationConvertCore::GetTransientEventFlag(const std::vector<double> &data, int32_t onsetIdx)
{
    if (data.empty()) {
        SEN_HILOGE("data is empty");
        return false;
    }
    int32_t partLen = ONSET_ONE_WIN;
    int32_t partCount = static_cast<int32_t>(round(partLen * ONSET_SPLIT_RATIO));
    int32_t partNumber = partLen - partCount;
    int32_t beginIdx = onsetIdx * ENERGY_HOP_LEN - partCount;
    int32_t endIdx = onsetIdx * ENERGY_HOP_LEN + partNumber;
    if (beginIdx < 0) {
        beginIdx = 0;
    }
    size_t dataSize = data.size();
    if (endIdx >= dataSize) {
        endIdx = dataSize - 1;
    }
    std::vector<double> localData;
    for (int32_t i = beginIdx; i <= endIdx; ++i) {
        localData.push_back(data[i]);
    }
    double unzeroDensity = 0.0;
    int32_t unzeroCount = 0;
    // Output parameter: unzeroCnt unzeroDensity
    GetUnzeroCount(localData, unzeroCount, unzeroDensity);
    return IsTransientEventFlag(unzeroCount, unzeroDensity);
}

std::vector<bool> VibrationConvertCore::GetTransientEventFlags(const std::vector<double> &data,
    const std::vector<int32_t> &onsetIdxs)
{
    size_t onsetIdxsSize = onsetIdxs.size();
    if (onsetIdxsSize <= 1) {
        SEN_HILOGE("onsetIdxsSize must be less than or equal to 1");
        return {};
    }
    size_t dataSize = data.size();
    if (dataSize == 0) {
        SEN_HILOGE("dataSize is equal to 0");
        return {};
    }
    int32_t partLen = 0;
    std::vector<bool> transientEventFlags;
    for (size_t i = 0; i < onsetIdxsSize; ++i) {
        if (i == (onsetIdxsSize - 1)) {
            partLen = (onsetIdxs[i] - onsetIdxs[i - 1]) * ENERGY_HOP_LEN;
        } else {
            partLen = (onsetIdxs[i + 1] - onsetIdxs[i]) * ENERGY_HOP_LEN;
        }
        if (partLen == 0) {
            SEN_HILOGE("partLen is equal to 0");
            return {};
        }
        int32_t partCount = static_cast<int32_t>(round(partLen * ONSET_SPLIT_RATIO));
        int32_t partNumber = partLen - partCount;
        int32_t beginIdx = onsetIdxs[i] * ENERGY_HOP_LEN - partCount;
        int32_t endIdx = onsetIdxs[i] * ENERGY_HOP_LEN + partNumber;
        if (beginIdx < 0) {
            beginIdx = 0;
        }
        if ((i != 0) && (beginIdx < onsetIdxs[i - 1] * ENERGY_HOP_LEN)) {
            beginIdx = onsetIdxs[i - 1] * ENERGY_HOP_LEN;
        }
        if (endIdx >= dataSize) {
            endIdx = dataSize - 1;
        }
        std::vector<double> localData;
        for (int32_t j = beginIdx; j <= endIdx; ++j) {
            localData.push_back(data[i]);
        }
        double unzeroDensity = 0.0;
        int32_t unzeroCount = 0;
        GetUnzeroCount(localData, unzeroCount, unzeroDensity);
        transientEventFlags.push_back(IsTransientEventFlag(unzeroCount, unzeroDensity));
    }
    return transientEventFlags;
}

std::vector<bool> VibrationConvertCore::IsTransientEvent(const std::vector<double> &data,
    const std::vector<int32_t> &onsetIdxs)
{
    if (data.empty() || onsetIdxs.empty()) {
        SEN_HILOGE("data or onsetIdxs is empty");
        return {};
    }
    std::vector<bool> transientEventFlags;
    if (onsetIdxs.size() == 1) {
        bool transientEventFlag = GetTransientEventFlag(data, onsetIdxs[0]);
        transientEventFlags.push_back(transientEventFlag);
    } else {
        transientEventFlags = GetTransientEventFlags(data, onsetIdxs);
    }
    return transientEventFlags;
}

void VibrationConvertCore::GetUnzeroCount(const std::vector<double> &localData,
    int32_t &unzeroCount, double &unzeroDensity)
{
    if (localData.empty()) {
        unzeroCount = 0;
        unzeroDensity = 0;
        SEN_HILOGE("localData is empty");
        return;
    }
    std::vector<double> envelope = localData;
    for (auto &elem : envelope) {
        elem = std::fabs(elem);
        if (elem < LOWER_AMP) {
            elem = 0;
        }
    }
    size_t envelopeSize = envelope.size();
    for (size_t i = 0; i < envelopeSize; ++i) {
        if ((i + LOCAL_ENVELOPE_MAX_LEN) >= envelopeSize) {
            break;
        }
        std::vector<double> segmentEnvelope;
        for (size_t j = i; j < (i + LOCAL_ENVELOPE_MAX_LEN); ++j) {
            segmentEnvelope.push_back(envelope[j]);
        }
        envelope[i] = *std::max_element(segmentEnvelope.begin(), segmentEnvelope.end());
    }
    for (auto elem : envelope) {
        if (elem > 0) {
            ++unzeroCount;
        }
    }
    unzeroDensity = static_cast<double>(unzeroCount) / envelopeSize;
}

bool VibrationConvertCore::IsTransientEventFlag(int32_t unzeroCount, double unzeroDensity)
{
    double duration = static_cast<double>(unzeroCount) / SAMPLE_RATE;
    if ((duration < FRAME_DURATION) || (unzeroDensity < TRANSIENT_UNZERO_DENSITY_MIN)) {
        return true;
    }
    return false;
}

void VibrationConvertCore::TranslateAnchorPoint(const std::vector<int32_t> &amplitudePeakPos,
    std::vector<UnionTransientEvent> &unionTransientEvents)
{
    bool useAbsTimeFlag = systemPara_.useAbsTimeFlag;
    if (useAbsTimeFlag) {
        for (auto pos : amplitudePeakPos) {
            unionTransientEvents.emplace_back(
                UnionTransientEvent(static_cast<int32_t>(round(1.0 * pos / ENERGY_HOP_LEN)), 1.0 * pos / SAMPLE_RATE));
        }
    } else {
        double rmsTimePerFrame = static_cast<double>(ENERGY_HOP_LEN) / SAMPLE_RATE;
        for (auto pos : amplitudePeakPos) {
            int32_t idx = static_cast<int32_t>(round(1.0 * pos / ENERGY_HOP_LEN));
            unionTransientEvents.emplace_back(UnionTransientEvent(idx, idx * rmsTimePerFrame));
        }
    }
}

void VibrationConvertCore::TranslateAnchorPoint(int32_t amplitudePeakPos, int32_t &amplitudePeakIdx,
    double &amplitudePeakTime)
{
    bool useAbsTimeFlag = systemPara_.useAbsTimeFlag;
    amplitudePeakIdx = static_cast<int32_t>(round(1.0 * amplitudePeakPos / ENERGY_HOP_LEN));
    if (useAbsTimeFlag) {
        amplitudePeakTime = static_cast<double>(amplitudePeakPos) / SAMPLE_RATE;
    } else {
        double rmsTimePerFrame = static_cast<double>(ENERGY_HOP_LEN) / SAMPLE_RATE;
        amplitudePeakTime = amplitudePeakIdx * rmsTimePerFrame;
    }
}

std::vector<int32_t> VibrationConvertCore::DetectFrequency(const std::vector<double> &data,
    const std::vector<int32_t> &rmseIntensityNorm)
{
    CALL_LOG_ENTER;
    std::vector<double> zcr =  freqPyin_.GetZeroCrossingRate(data, FRAME_LEN, ENERGY_HOP_LEN);
    for (auto &elem : zcr) {
        elem = elem * SAMPLE_RATE * F_HALF;
    }
    std::vector<bool> voiceSegmentFlag = peakFinder_.GetVoiceSegmentFlag();
    std::vector<int32_t> freqNorm;
    freqPyin_.FreqPostProcess(zcr, voiceSegmentFlag, rmseIntensityNorm, freqNorm);
    return freqNorm;
}

int32_t VibrationConvertCore::DetectRmsIntensity(const std::vector<double> &data, double rmsILowerDelta,
    std::vector<IntensityData> &intensityData)
{
    CALL_LOG_ENTER;
    std::vector<double> rmse = calcIntensity_.GetRMS(data, ENERGY_HOP_LEN, systemPara_.centerPaddingFlag);
    if (rmse.empty()) {
        SEN_HILOGE("GetRMS failed.");
        return Sensors::ERROR;
    }
    std::vector<double> rmseTime;
    double rmsTimePerFrame = static_cast<double>(ENERGY_HOP_LEN) / SAMPLE_RATE;
    for (auto elem : rmse) {
        rmseTime.push_back(elem * rmsTimePerFrame);
    }
    std::vector<double> rmseBand;
    std::vector<int32_t> rmseNorm;
    if (calcIntensity_.RmseNormilize(rmse, rmsILowerDelta, rmseBand, rmseNorm) != Sensors::SUCCESS) {
        SEN_HILOGE("RmseNormilize failed");
        return Sensors::ERROR;
    }
    std::vector<double> rmseTimeNorm = StartTimeNormalize(rmse.size());
    std::vector<double> ampLin = calcIntensity_.VolumeInLinary(data, ENERGY_HOP_LEN);
    std::vector<double> ampDb = calcIntensity_.VolumeInDB(data, ENERGY_HOP_LEN);
    double ampDbMax = *std::max_element(ampDb.begin(), ampDb.end());
    bool intensityUseLinearFlag = systemPara_.intensityUseLinearFlag;
    if (!intensityUseLinearFlag && (ampDbMax > AMPLITUDE_DB_MAX)) {
        for (size_t i = 0; i < ampDb.size(); ++i) {
            rmseNorm[i] = ampDb[i] * DB_ZOOM_COEF;
        }
    }
    for (size_t i = 0; i < rmse.size(); i++) {
        intensityData.push_back(IntensityData(rmse[i], rmseTime[i], rmseBand[i], rmseNorm[i], rmseTimeNorm[i]));
    }
    return Sensors::SUCCESS;
}
}  // namespace Sensors
}  // namespace OHOS
