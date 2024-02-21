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

#undef LOG_TAG
#define LOG_TAG "VibrationConvertCore"

namespace OHOS {
namespace Sensors {
namespace {
constexpr double AMP_INVALIDE_DELTA { 0.001 };
constexpr int32_t FRAGMENT_MIN_LEN { 32 };
constexpr double COEF { 0.01 };
constexpr double RMS_INVALIDE_DELTA { 0.04 };
// A transient event is distributed within three 1024 (46ms) windows
constexpr int32_t ONSET_ONE_WIN { 3072 };
// A transient event rising range
constexpr double ONSET_SPLIT_RATIO { 0.33334 };
constexpr double LOWER_AMP { 0.03 };
constexpr double FRAME_DURATION { 0.046 };
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

int32_t VibrationConvertCore::GetAudioData()
{
    std::vector<double> data = PreprocessAudioData();
    int32_t onsetHopLength = WINDOW_LENGTH;
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
    std::vector<IntensityData> intensityDatas;
    // Processing intensity data, output parameters:intensityDatas
    ret = DetectRmsIntensity(data, rmsILowerDelta, intensityDatas);
    if (ret != Sensors::SUCCESS) {
        SEN_HILOGE("DetectRmsIntensity failed");
        return ret;
    }
    // Frequency detection
    std::vector<int32_t> rmseIntensityNorm;
    for (size_t i = 0; i < intensityDatas.size(); i++) {
        rmseIntensityNorm.push_back(intensityDatas[i].rmseIntensityNorm);
    }
    std::vector<int32_t> freqNorms = DetectFrequency(data, rmseIntensityNorm);
    if (freqNorms.empty()) {
        SEN_HILOGE("DetectFrequency failed");
        return Sensors::ERROR;
    }
    std::vector<int32_t> transientIndexes;
    std::vector<double> transientEventTimes;
    // Output all Transient events, output parameters: transientIndex transientEventTimes
    OutputTransientEvents(unionTransientEvents, intensityDatas, freqNorms, transientIndexes, transientEventTimes);
    if (continuousEventExistFlag_) {
        std::vector<bool> transientEventFlags;
        for (size_t i = 0; i < unionTransientEvents.size(); i++) {
            transientEventFlags.push_back(unionTransientEvents[i].transientEventFlag);
        }
        OutputAllContinuousEvent(intensityDatas, transientIndexes, freqNorms, transientEventFlags);
    }
    return Sensors::SUCCESS;
}
int32_t VibrationConvertCore::ConvertAudioToHaptic(const AudioSetting &audioSetting,
    const std::vector<double> &audioDatas, std::vector<HapticEvent> &hapticEvents)
{
    CALL_LOG_ENTER;
    if (audioDatas.empty()) {
        SEN_HILOGE("audioDatas is empty");
        return Sensors::ERROR;
    }
    audioSetting_ = audioSetting;
    int32_t ret = ResampleAudioData(audioDatas);
    if (ret != Sensors::SUCCESS) {
        SEN_HILOGE("ResampleAudioData failed");
        return ret;
    }
    if (GetAudioData() != Sensors::SUCCESS) {
        SEN_HILOGE("GetAudioData failed");
        return Sensors::ERROR;
    }
    StoreHapticEvent();
    hapticEvents = hapticEvents_;
    GenerateVibrationJsonFile jsonFile;
    jsonFile.GenerateJsonFile(hapticEvents_);
    return Sensors::SUCCESS;
}

int32_t VibrationConvertCore::ResampleAudioData(const std::vector<double> &srcDatas)
{
    if (srcDatas.empty()) {
        SEN_HILOGE("srcDatas is empty");
        return Sensors::ERROR;
    }
    size_t originDataSize = srcDatas.size();
    srcAudioDatas_.clear();
    for (size_t i = 0; i < (originDataSize - 1); i += RESAMPLE_MULTIPLE) {
        srcAudioDatas_.push_back(srcDatas[i]);
        srcAudioDatas_.push_back(srcDatas[i+1]);
    }
    return Sensors::SUCCESS;
}

std::vector<double> VibrationConvertCore::PreprocessAudioData()
{
    CALL_LOG_ENTER;
    if (srcAudioDatas_.empty()) {
        SEN_HILOGE("invalid parameter");
        return {};
    }
    std::vector<double> absData = srcAudioDatas_;
    std::vector<double> dstData = srcAudioDatas_;
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

int32_t VibrationConvertCore::PreprocessParameter(const std::vector<double> &datas, int32_t &onsetHopLength,
    double &lowerDelta)
{
    CALL_LOG_ENTER;
    if (datas.empty()) {
        SEN_HILOGE("datas is empty");
        return Sensors::ERROR;
    }
    std::vector<double> rmses = intensityProcessor_.GetRMS(datas, ENERGY_HOP_LEN, systemPara_.centerPaddingFlag);
    OnsetInfo onsetInfo;
    if (onset_.CheckOnset(datas, NFFT, onsetHopLength, onsetInfo) != Sensors::SUCCESS) {
        SEN_HILOGE("CheckOnset Failed");
        return Sensors::ERROR;
    }
    std::vector<int32_t> newDrwIdxs = MapOnsetHop(onsetInfo.idxs, onsetHopLength);
    lowerDelta = CalcRmsLowerData(datas.size(), rmses, newDrwIdxs);
    double rmseMax = *std::max_element(rmses.begin(), rmses.end());
    size_t newDrwIdxLen = newDrwIdxs.size();
    bool continuousEventFlag = false;
    int32_t newOnsetHopLen = onsetHopLength;
    // Amplitude density is used to classify audio files, determine whether there are long events in the audio file,
    // and adjust the onset detection window
    if (CalcOnsetHopLength(datas, rmseMax, newDrwIdxLen, continuousEventFlag, newOnsetHopLen) != Sensors::SUCCESS) {
        SEN_HILOGE("CalcOnsetHopLength failed");
        return Sensors::ERROR;
    }
    continuousEventExistFlag_ = continuousEventFlag;
    onsetHopLength = newOnsetHopLen;
    return Sensors::SUCCESS;
}

std::vector<int32_t> VibrationConvertCore::MapOnsetHop(const std::vector<int32_t> &drwIdxs, int32_t onsetHopLength)
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
    for (const auto &idx : drwIdxs) {
        newIdx.push_back(static_cast<int32_t>(round(static_cast<double>(idx) / coef)));
    }
    return newIdx;
}

bool VibrationConvertCore::GetRmseLowerDelta(double lowerDelta, const std::vector<double> &rmses, double &deltaByTime)
{
    int32_t rmseSize = static_cast<int32_t>(rmses.size());
    double rmseMax = *std::max_element(rmses.begin(), rmses.end());
    double rmseMin = *std::min_element(rmses.begin(), rmses.end());
    double rmseRange = rmseMax - rmseMin;
    int32_t i = 0;
    deltaByTime = lowerDelta;
    for (i = 0; i < rmseSize; ++i) {
        if (rmses[i] > lowerDelta) {
            break;
        }
    }
    double rmsTimePerFrame = static_cast<double>(ENERGY_HOP_LEN) / SAMPLE_RATE;
    int32_t j = rmseSize - 1;
    for (; j >= 0 ; j = (j - 1)) {
        if (rmses[j] > lowerDelta) {
            break;
        }
    }
    if ((j - i) <= 0) {
        return false;
    }
    double totalDuration = (j - i) * rmsTimePerFrame;
    if (IsLessNotEqual(totalDuration, 1.0)) { // 1s
        deltaByTime = rmseRange * RMSE_LOWDELTA_RATIO_LOW + rmseMin;
        return true;
    }
    return false;
}

double VibrationConvertCore::CalcRmsLowerData(size_t dataSize, const std::vector<double> &rmses,
    const std::vector<int32_t> &newDrwIdxs)
{
    double soundSizeDelta = 0.0;
    double rmseMax = *std::max_element(rmses.begin(), rmses.end());
    double rmseMin = *std::min_element(rmses.begin(), rmses.end());
    double rmseRange = rmseMax - rmseMin;
    if (rmseMax > RMSEMAX_THRESHOLD_HIGH) {
        soundSizeDelta = rmseRange * RMSE_LOWDELTA_RATIO_HIGH + rmseMin;
    } else if (rmseMax > RMSEMAX_THRESHIOLD_MID) {
        soundSizeDelta = rmseRange * RMSE_LOWDELTA_RATIO_MID + rmseMin;
    } else {
        soundSizeDelta = rmseRange * RMSE_LOWDELTA_RATIO_LOW + rmseMin;
    }
    double lowerDelta = 0.0;
    if (newDrwIdxs.size() > 0) {
        for (int32_t i = 0; i < RMSE_LOWDELTA_ITERATION_TIMES; i++) {
            int32_t j = newDrwIdxs[0];
            lowerDelta = rmseRange * (RMSE_LOWDELTA_RATIO_HIGH - i * RMSE_LOWDELTA_RATIO_STEP ) + rmseMin;
            if ((rmses[j] > lowerDelta) || (rmses[j + 1] > lowerDelta)) {
                break;
            }
        }
    }
    double audioDurationDelta = soundSizeDelta;
    double audioDuration = static_cast<double>(dataSize) / SAMPLE_RATE;
    if (IsLessOrEqual(audioDuration, 1.0)) {
        audioDurationDelta = rmseRange * RMSE_LOWDELTA_RATIO_LOW + rmseMin;
        return audioDurationDelta;
    }
    double soundDurationDelta = soundSizeDelta;
    bool findFlag = GetRmseLowerDelta(soundSizeDelta, rmses, soundDurationDelta);
    if (findFlag) {
        return soundDurationDelta;
    }
    if (soundSizeDelta < lowerDelta) {
        return soundSizeDelta;
    }
    return lowerDelta;
}

int32_t VibrationConvertCore::CalcOnsetHopLength(const std::vector<double> &datas, double rmseMax,
    size_t newDrwIdxLen, bool &continuousEventFlag, int32_t &newOnsetHopLen)
{
    if (datas.empty()) {
        SEN_HILOGE("Invalid parameter");
        return Sensors::ERROR;
    }
    int32_t longestCount = 0;
    double unzeroDensity = 0.0;
    bool continuousEventExistFlag = false;
    int32_t ret = IsIncludeContinuoustEvent(datas, longestCount, unzeroDensity, continuousEventExistFlag);
    if (ret != Sensors::SUCCESS) {
        SEN_HILOGE("IsIncludeContinuoustEvent failed");
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

std::vector<double> VibrationConvertCore::GetLocalEnvelope(const std::vector<double> &datas)
{
    if (datas.empty()) {
        SEN_HILOGE("invalid parameter");
        return {};
    }
    std::vector<double> envelopes = datas;
    for (auto &elem : envelopes) {
        elem = std::fabs(elem);
    }
    double threshold = COEF * (*std::max_element(envelopes.begin(), envelopes.end()));
    for (auto &elem : envelopes) {
        if (elem < threshold) {
            elem = 0;
        }
    }
    size_t dataSize = envelopes.size();
    for (size_t i = 0; i < dataSize; ++i) {
        if ((i + LOCAL_ENVELOPE_MAX_LEN) > dataSize) {
            break;
        }
        std::vector<double> segmentEnvelope;
        for (size_t j = i; j < (i + LOCAL_ENVELOPE_MAX_LEN); ++j) {
            segmentEnvelope.push_back(envelopes[j]);
        }
        envelopes[i] = *std::max_element(segmentEnvelope.begin(), segmentEnvelope.end());
    }
    return envelopes;
}

int32_t VibrationConvertCore::IsIncludeContinuoustEvent(const std::vector<double> &datas,
    int32_t &longestCount, double &unzeroDensity, bool &isIncludeContinuoustEvent)
{
    // envelope must be a non-negative number.
    std::vector<double> envelopes = GetLocalEnvelope(datas);
    if (envelopes.empty()) {
        SEN_HILOGE("GetLocalEnvelope failed");
        return Sensors::ERROR;
    }
    size_t envelopeSize = envelopes.size();
    size_t j = 0;
    size_t k = 0;
    int32_t atLeastCnt = 2 * ENERGY_HOP_LEN;
    int32_t adsrCompleteStatus = ADSR_BOUNDARY_STATUS_NONE;
    std::vector<int32_t> countList;
    for (size_t i = 0; i < envelopeSize; ++i) {
        if ((envelopes[i] >= EPS_MIN) && (adsrCompleteStatus == ADSR_BOUNDARY_STATUS_NONE)) {
            j = i;
            adsrCompleteStatus = ADSR_BOUNDARY_STATUS_ONE;
        }
        if ((envelopes[i] < EPS_MIN) && (adsrCompleteStatus == ADSR_BOUNDARY_STATUS_ONE)) {
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
    double preTime = -1.0; // invalid time.
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

int32_t VibrationConvertCore::ConvertTransientEvent(const std::vector<double> &datas, int32_t onsetHopLength,
    std::vector<UnionTransientEvent> &unionTransientEvents)
{
    CALL_LOG_ENTER;
    if (datas.empty()) {
        SEN_HILOGE("datas is empty");
        return Sensors::ERROR;
    }
    // Using System Methods to Detect Onset
    std::vector<UnionTransientEvent> unionTransientValues = DetectOnset(datas, onsetHopLength);
    // Is the onset just a transient event
    std::vector<int32_t> onsetIdxs;
    for (size_t i = 0; i < unionTransientValues.size(); i++) {
        onsetIdxs.push_back(unionTransientValues[i].onsetIdx);
    }
    std::vector<bool> transientEventFlags = IsTransientEvent(datas, onsetIdxs);
    for (size_t i = 0; i < unionTransientValues.size(); i++) {
        unionTransientValues[i].transientEventFlag = transientEventFlags[i];
    }
    IsolatedEnvelopeInfo isolatedEnvelopeInfo;
    int32_t ret = peakFinder_.ObtainTransientByAmplitude(datas, isolatedEnvelopeInfo);
    if (ret != Sensors::SUCCESS) {
        SEN_HILOGE("ObtainTransientByAmplitude failed.");
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
        for (size_t i = 0; i < size; ++i) {
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

std::vector<UnionTransientEvent> VibrationConvertCore::DetectOnset(const std::vector<double> &datas,
    int32_t onsetHopLength)
{
    OnsetInfo onsetInfo;
    if (onset_.CheckOnset(datas, NFFT, onsetHopLength, onsetInfo) != Sensors::SUCCESS) {
        SEN_HILOGE("CheckOnset Failed");
        return {};
    }
    onsetInfo.idxs = MapOnsetHop(onsetInfo.idxs, onsetHopLength);
    std::vector<int32_t> newIdx;
    std::vector<double> newTime;
    UniqueIdx(onsetInfo.idxs, onsetInfo.times, newIdx, newTime);
    int32_t minSkip = ONSET_MINSKIP_MAX;
    if (newIdx.size() > 1) {
        std::vector<int32_t> idxDiff;
        for (size_t i = 1; i < newIdx.size(); ++i) {
            idxDiff.push_back(newIdx[i] - newIdx[i - 1]);
        }
        minSkip = *std::min_element(idxDiff.begin(), idxDiff.end());
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

bool VibrationConvertCore::GetTransientEventFlag(const std::vector<double> &datas, int32_t onsetIdx)
{
    if (datas.empty()) {
        SEN_HILOGE("datas is empty");
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
    int32_t dataSize = static_cast<int32_t>(datas.size());
    if (endIdx >= dataSize) {
        endIdx = dataSize - 1;
    }
    std::vector<double> localDatas;
    for (int32_t i = beginIdx; i <= endIdx; ++i) {
        localDatas.push_back(datas[i]);
    }
    double unzeroDensity = 0.0;
    int32_t unzeroCount = 0;
    // Output parameter: unzeroCnt unzeroDensity
    GetUnzeroCount(localDatas, unzeroCount, unzeroDensity);
    return IsTransientEventFlag(unzeroCount, unzeroDensity);
}

std::vector<bool> VibrationConvertCore::GetTransientEventFlags(const std::vector<double> &datas,
    const std::vector<int32_t> &onsetIdxs)
{
    if (datas.empty() || (onsetIdxs.size() <= 1)) {
        SEN_HILOGE("Invalid parameter");
        return {};
    }
    int32_t partLen = 0;
    std::vector<bool> transientEventFlags;
    size_t onsetIdxsSize = onsetIdxs.size();
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
        int32_t dataSize = static_cast<int32_t>(datas.size());
        if (endIdx >= dataSize) {
            endIdx = dataSize - 1;
        }
        std::vector<double> localData;
        for (int32_t j = beginIdx; j <= endIdx; ++j) {
            localData.push_back(datas[i]);
        }
        double unzeroDensity = 0.0;
        int32_t unzeroCount = 0;
        GetUnzeroCount(localData, unzeroCount, unzeroDensity);
        transientEventFlags.push_back(IsTransientEventFlag(unzeroCount, unzeroDensity));
    }
    return transientEventFlags;
}

std::vector<bool> VibrationConvertCore::IsTransientEvent(const std::vector<double> &datas,
    const std::vector<int32_t> &onsetIdxs)
{
    if (datas.empty() || onsetIdxs.empty()) {
        SEN_HILOGE("datas or onsetIdxs is empty");
        return {};
    }
    std::vector<bool> transientEventFlags;
    if (onsetIdxs.size() == 1) {
        bool transientEventFlag = GetTransientEventFlag(datas, onsetIdxs[0]);
        transientEventFlags.push_back(transientEventFlag);
    } else {
        transientEventFlags = GetTransientEventFlags(datas, onsetIdxs);
    }
    return transientEventFlags;
}

void VibrationConvertCore::GetUnzeroCount(const std::vector<double> &localDatas,
    int32_t &unzeroCount, double &unzeroDensity)
{
    if (localDatas.empty()) {
        unzeroCount = 0;
        unzeroDensity = 0;
        SEN_HILOGE("localDatas is empty");
        return;
    }
    std::vector<double> envelope = localDatas;
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
    if (IsLessNotEqual(duration, FRAME_DURATION) || IsLessNotEqual(unzeroDensity, TRANSIENT_UNZERO_DENSITY_MIN)) {
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

std::vector<int32_t> VibrationConvertCore::DetectFrequency(const std::vector<double> &datas,
    const std::vector<int32_t> &rmseIntensityNorms)
{
    CALL_LOG_ENTER;
    std::vector<double> zcrs =  frequencyEstimation_.GetZeroCrossingRate(datas, FRAME_LEN, ENERGY_HOP_LEN);
    for (auto &elem : zcrs) {
        elem = elem * SAMPLE_RATE * F_HALF;
    }
    std::vector<bool> voiceSegmentFlag = peakFinder_.GetVoiceSegmentFlag();
    std::vector<int32_t> freqNorms;
    frequencyEstimation_.FreqPostProcess(zcrs, voiceSegmentFlag, rmseIntensityNorms, freqNorms);
    return freqNorms;
}

int32_t VibrationConvertCore::DetectRmsIntensity(const std::vector<double> &datas, double rmsILowerDelta,
    std::vector<IntensityData> &intensityDatas)
{
    CALL_LOG_ENTER;
    std::vector<double> rmses = intensityProcessor_.GetRMS(datas, ENERGY_HOP_LEN, systemPara_.centerPaddingFlag);
    if (rmses.empty()) {
        SEN_HILOGE("rmses is empty");
        return Sensors::ERROR;
    }
    std::vector<double> rmseTime;
    double rmsTimePerFrame = static_cast<double>(ENERGY_HOP_LEN) / SAMPLE_RATE;
    for (auto elem : rmses) {
        rmseTime.push_back(elem * rmsTimePerFrame);
    }
    std::vector<double> rmseBand;
    std::vector<int32_t> rmseNorm;
    if (intensityProcessor_.RmseNormalize(rmses, rmsILowerDelta, rmseBand, rmseNorm) != Sensors::SUCCESS) {
        SEN_HILOGE("RmseNormalize failed");
        return Sensors::ERROR;
    }
    std::vector<double> rmseTimeNorms = StartTimeNormalize(rmses.size());
    std::vector<double> ampLin = intensityProcessor_.VolumeInLinary(datas, ENERGY_HOP_LEN);
    std::vector<double> ampDb = intensityProcessor_.VolumeInDB(datas, ENERGY_HOP_LEN);
    double ampDbMax = *std::max_element(ampDb.begin(), ampDb.end());
    bool intensityUseLinearFlag = systemPara_.intensityUseLinearFlag;
    if (!intensityUseLinearFlag && (ampDbMax > AMPLITUDE_DB_MAX)) {
        for (size_t i = 0; i < ampDb.size(); ++i) {
            rmseNorm[i] = ampDb[i] * DB_ZOOM_COEF;
        }
    }
    for (size_t i = 0; i < rmses.size(); i++) {
        intensityDatas.push_back(IntensityData(rmses[i], rmseTime[i], rmseBand[i], rmseNorm[i], rmseTimeNorms[i]));
    }
    return Sensors::SUCCESS;
}

std::vector<double> VibrationConvertCore::StartTimeNormalize(int32_t rmseLen)
{
    CALL_LOG_ENTER;
    std::vector<double> rmseTimeNorms;
    double startTime = 0.0;
    for (int32_t i = 0; i < rmseLen; ++i) {
        rmseTimeNorms.push_back(startTime);
        startTime += FRAME_DURATION;
    }
    return rmseTimeNorms;
}

void VibrationConvertCore::OutputTransientEvents(const std::vector<UnionTransientEvent> &unionTransientEvents,
    const std::vector<IntensityData> &intensityDatas, const std::vector<int32_t> &freqNorms,
    std::vector<int32_t> &transientIndexs, std::vector<double> &transientEventTimes)
{
    CALL_LOG_ENTER;
    bool onsetBacktrackFlag = systemPara_.onsetBacktrackFlag;
    if (!continuousEventExistFlag_) {
        std::vector<double> onsetTimes;
        for (size_t i = 0; i < unionTransientEvents.size(); i++) {
            onsetTimes.push_back(unionTransientEvents[i].onsetTime);
        }
        OutputTransientEventsByInsertTime(onsetTimes, intensityDatas, freqNorms, transientIndexs, transientEventTimes);
        return;
    }
    if (onsetBacktrackFlag) {
        OutputTransientEventsAlign(unionTransientEvents, intensityDatas, freqNorms, transientIndexs, transientEventTimes);
    } else {
        OutputTransientEventsDirect(unionTransientEvents, intensityDatas, freqNorms, transientIndexs, transientEventTimes);
    }
}

void VibrationConvertCore::OutputTransientEventsByInsertTime(const std::vector<double> &onsetTimes,
    const std::vector<IntensityData> &intensityDatas, const std::vector<int32_t> &freqNorms,
    std::vector<int32_t> &transientIndexs, std::vector<double> &transientEventTimes)
{
    double rmsTimePerFrame = static_cast<double>(ENERGY_HOP_LEN) / SAMPLE_RATE;
    for (size_t i = 0; i < onsetTimes.size(); ++i) {
        bool flag = false;
        size_t j = 0;
        while (j < intensityDatas.size()) {
            if ((onsetTimes[i] >= intensityDatas[j].rmseTimeNorm) &&
                (onsetTimes[i] < (intensityDatas[j].rmseTimeNorm + rmsTimePerFrame))) {
                flag = true;
                break;
            }
            ++j;
        }
        if (flag) {
            auto it = std::find(transientIndexs.begin(), transientIndexs.end(), j);
            if (it == transientIndexs.end()) {
                size_t endIndex = std::min(j + MIN_SKIP, intensityDatas.size() - 1);
                // get max index.
                size_t maxIndex = j;
                double maxRmseEnvelope = intensityDatas[j].rmseEnvelope;
                for (size_t k = (j + 1); k < endIndex; k++) {
                    if (intensityDatas[k].rmseEnvelope > maxRmseEnvelope) {
                        maxRmseEnvelope = intensityDatas[k].rmseEnvelope;
                        maxIndex = k;
                    }
                }
                double onsetTimeNorm = static_cast<double>(round(SAMPLE_IN_MS * onsetTimes[i]) / SAMPLE_IN_MS);
                transientEventTimes.push_back(onsetTimeNorm);
                transientIndexs.push_back(j);
                AddTransientEventData(TransientEvent(onsetTimeNorm, intensityDatas[maxIndex].rmseIntensityNorm,
                    freqNorms[maxIndex]));
            }
        }
    }
}

void VibrationConvertCore::AddTransientEventData(TransientEvent transientEvent)
{
    if (transientEvent.intensity < TRANSIENT_EVENT_INTENSITY_MIN) {
        transientEvent.intensity = TRANSIENT_EVENT_INTENSITY_MIN;
    }
    if (transientEvent.frequency < TRANSIENT_EVENT_FREQUENCY_MIN) {
        transientEvent.frequency = TRANSIENT_EVENT_FREQUENCY_MIN;
    }
    transientEvents_.push_back(transientEvent);
}

void VibrationConvertCore::GetIdex(const UnionTransientEvent &unionTransientEvent,
    const std::vector<IntensityData> &intensityDatas)
{
    // get max index.
    size_t beginIndex = unionTransientEvent.onsetIdx;
    size_t endIndex = beginIndex + onsetMinSkip_;
    size_t maxIndex = beginIndex;
    double maxRmseEnvelope = intensityDatas[beginIndex].rmseEnvelope;
    for (size_t k = (beginIndex + 1); k < endIndex; k++) {
        if (intensityDatas[k].rmseEnvelope > maxRmseEnvelope) {
            maxRmseEnvelope = intensityDatas[k].rmseEnvelope;
            maxIndex = k;
        }
    }
    int32_t fromIndex = unionTransientEvent.onsetIdx - onsetMinSkip_;
    if (fromIndex < 0) {
        fromIndex = 0;
    }
    // get min index.
    beginIndex = fromIndex;
    endIndex = unionTransientEvent.onsetIdx + 1;
    size_t minIndex = beginIndex;
    double minRmseEnvelope = intensityDatas[beginIndex].rmseEnvelope;
    for (size_t k = (beginIndex + 1); k < endIndex; k++) {
        if (intensityDatas[k].rmseEnvelope < minRmseEnvelope) {
            minRmseEnvelope = intensityDatas[k].rmseEnvelope;
            minIndex = k;
        }
    }
    if (minIndex == (unionTransientEvent.onsetIdx + 1)) {
        minIndex = unionTransientEvent.onsetIdx;
    }
}

void VibrationConvertCore::OutputTransientEventsAlign(const std::vector<UnionTransientEvent> &unionTransientEvents,
    const std::vector<IntensityData> &intensityDatas, const std::vector<int32_t> &freqNorms,
    std::vector<int32_t> &transientIndexs, std::vector<double> &transientEventTimes)
{
    size_t size = unionTransientEvents.size();
    for (size_t i = 1; i < size; ++i) {
        if ((unionTransientEvents[i].onsetIdx != 0) &&
            (unionTransientEvents[i - 1].onsetIdx == (unionTransientEvents[i].onsetIdx - 1))) {
            auto it = find(transientIndexs.begin(), transientIndexs.end(), unionTransientEvents[i].onsetIdx);
            if (it == transientIndexs.end()) {
                int32_t index = unionTransientEvents[i].onsetIdx;
                transientEventTimes.push_back(intensityDatas[index].rmseTimeNorm);
                transientIndexs.push_back(index);
                AddTransientEventData(TransientEvent(intensityDatas[index].rmseTimeNorm,
                    intensityDatas[index].rmseIntensityNorm, freqNorms[index]));
                continue;
            }
        }
        GetIdex(unionTransientEvents[i], intensityDatas);
        auto it = find(transientIndexs.begin(), transientIndexs.end(), minIndex);
        if (it == transientIndexs.end()) {
            transientEventTimes.push_back(intensityDatas[minIndex].rmseTimeNorm);
            transientIndexs.push_back(minIndex);
            AddTransientEventData(TransientEvent(intensityDatas[minIndex].rmseTimeNorm,
                intensityDatas[maxIndex].rmseIntensityNorm, freqNorms[maxIndex]));
        }
    }
}

void VibrationConvertCore::OutputTransientEventsDirect(const std::vector<UnionTransientEvent> &unionTransientEvents,
    const std::vector<IntensityData> &intensityDatas, const std::vector<int32_t> &freqNorms,
    std::vector<int32_t> &transientIndexs, std::vector<double> &transientEventTimes)
{
    size_t size = unionTransientEvents.size();
    for (size_t i = 0; i < size; ++i) {
        auto it = find(transientIndexs.begin(), transientIndexs.end(), unionTransientEvents[i].onsetIdx);
        if (it == transientIndexs.end()) {
            int32_t index = unionTransientEvents[i].onsetIdx;
            transientEventTimes.push_back(intensityDatas[index].rmseTimeNorm);
            transientIndexs.push_back(index);
             AddTransientEventData(TransientEvent(intensityDatas[index].rmseTimeNorm,
                intensityDatas[index].rmseIntensityNorm, freqNorms[index]));
        }
    }
}

void VibrationConvertCore::OutputAllContinuousEvent(const std::vector<IntensityData> &intensityDatas,
    const std::vector<int32_t> transientIndexs, const std::vector<int32_t> &freqNorms,
    const std::vector<bool> &transientEventFlags)
{
    CALL_LOG_ENTER;
    if (!systemPara_.splitSegmentFlag) {
        OutputAllContinuousEventByUnseg(intensityDatas, transientIndexs, freqNorms, transientEventFlags);
    }
}

void VibrationConvertCore::OutputAllContinuousEventByUnseg(const std::vector<IntensityData> &intensityDatas,
    const std::vector<int32_t> transientIndexs, const std::vector<int32_t> &freqNorms,
    const std::vector<bool> &transientEventFlags)
{
    std::vector<double> rmseTimeNorm;
    for (size_t i = 0; i < intensityDatas.size(); i++) {
        rmseTimeNorm.push_back(intensityDatas[i].rmseTimeNorm);
    }
    std::vector<double> times;
    std::vector<double> durations;
    FillDefaultContinuousEvents(rmseTimeNorm, times, durations);
    int32_t ret = InsertTransientEvent(rmseTimeNorm, transientIndexs, transientEventFlags,
        times, durations);
    if (ret != Sensors::SUCCESS) {
        SEN_HILOGE("InsertTransientEvent failed");
        return;
    }
    std::vector<ContinuousEvent> continuousEvents;
    for (size_t i = 0; i < times.size(); i++) {
        continuousEvents.push_back(ContinuousEvent(times[i], durations[i],
            intensityDatas[i].rmseIntensityNorm, freqNorms[i]));
    }
    MergeContinuousEvents(continuousEvents);
}

void VibrationConvertCore::FillDefaultContinuousEvents(const std::vector<double> &rmseTimeNorms,
    std::vector<double> &times, std::vector<double> &durations)
{
    double value = 0.0;
    for (size_t i = 0; i < rmseTimeNorms.size(); ++i) {
        times.push_back(value);
        durations.push_back(FRAME_DURATION);
        value += FRAME_DURATION;
    }
}

int32_t VibrationConvertCore::InsertTransientEvent(const std::vector<double> &rmseTimeNorms,
    const std::vector<int32_t> &transientIndexs, const std::vector<bool> &transientEventFlags,
    std::vector<double> &times, std::vector<double> &durations)
{
    if ((times.empty()) || (transientIndexs.size() != transientEventFlags.size())) {
        SEN_HILOGE("times is empty");
        return Sensors::ERROR;
    }
    for (size_t i = 0; i < (times.size() - 1); ++i) {
        auto it = find(transientIndexs.begin(), transientIndexs.end(), i);
        if (it == transientIndexs.end()) {
            continue;
        }
        int32_t index = it - transientIndexs.begin();

        double preDuration = 0.0;
        if (i > 0) {
            preDuration = durations[i - 1];
        }
        double transientTime = transientEvents_[index].time;
        double preInterDuration = transientTime - rmseTimeNorms[i];
        double postInterDuration = (rmseTimeNorms[i] + FRAME_DURATION) - (transientTime + TRANSIENT_DURATION_DEFAULT);
        if (IsGreatOrEqual(preInterDuration, CONTINOUS_MIN_DURATION)) {
            times[i] = rmseTimeNorms[i];
            durations[i] = preInterDuration;
            if (IsLessNotEqual(postInterDuration, CONTINOUS_MIN_DURATION)) {
                times[i + 1] = transientTime + TRANSIENT_DURATION_DEFAULT;
                durations[i + 1] = postInterDuration + durations[i + 1];
            }
        } else {
            if (i > 0) {
                durations[i - 1] = preDuration + preInterDuration;
            }
            if (IsLessNotEqual(postInterDuration, CONTINOUS_MIN_DURATION)) {
                times[i] = transientTime + TRANSIENT_DURATION_DEFAULT;
                durations[i] = 0;
                times[i + 1] = transientTime + TRANSIENT_DURATION_DEFAULT;
                durations[i + 1] = postInterDuration + durations[i + 1];
            } else {
                times[i] = transientTime + TRANSIENT_DURATION_DEFAULT;
                durations[i] = postInterDuration;
            }
        }
        if (transientEventFlags[index]) {
            durations[i] = 0;
        }
    }
    return Sensors::SUCCESS;
}

void VibrationConvertCore::CombinateContinuousEvents(const std::vector<ContinuousEvent> &continuousEvents,
    int32_t startIdx, int32_t endIdx)
{
    int32_t begIdx = startIdx;
    int32_t mergeCnt = 0;
    int32_t endIndex = 0;
    bool intensityCmbFlag = systemPara_.intensityCmbFlag;
    int32_t intensityCombinateDelta = 5;
    bool monotonicityCmbFlag = systemPara_.monotonicityCmbFlag;
    bool slopCmbFlag = systemPara_.slopCmbFlag;
    double slopDelta = SLOP_DELTA_MIN;
    double durationSum = 0.0;
    for (int32_t k = startIdx; k < endIdx; ++k) {
        if (mergeCnt == 0) {
            endIndex = begIdx;
        }
        if (intensityCmbFlag && (begIdx != (endIdx - 1))) {
            int32_t intensityDiff = std::abs(continuousEvents[endIndex].intensity -
                continuousEvents[begIdx + 1].intensity);
            int32_t frequencyDiff = std::abs(continuousEvents[endIndex].frequency -
                continuousEvents[begIdx + 1].frequency);
            if ((intensityDiff < intensityCombinateDelta) && (frequencyDiff < intensityCombinateDelta)) {
                ++mergeCnt;
                ++begIdx;
                continue;
            }
        }
        if (monotonicityCmbFlag && ((begIdx != 0) && (begIdx != (k - 1)))) {
            double slope1 = (continuousEvents[begIdx].intensity - continuousEvents[begIdx - 1].intensity);
            double slope2 = (continuousEvents[begIdx].intensity - continuousEvents[begIdx - 1].intensity);
            if ((slope1 * slope2) > 0) {
                ++mergeCnt;
                ++begIdx;
                continue;
            }
        }
        if (slopCmbFlag && ((begIdx != 0) && (begIdx != (k - 1)))) {
            double slope1 = (continuousEvents[begIdx].intensity - continuousEvents[begIdx - 1].intensity) / FRAME_DURATION;
            double slope2 = (continuousEvents[begIdx].intensity - continuousEvents[begIdx - 1].intensity) / FRAME_DURATION;
            if (((slope1 * slope2) > 0) && (std::abs(slope2 - slope1) < slopDelta)) {
                ++mergeCnt;
                ++begIdx;
                continue;
            }
        }
        if (endIndex != (begIdx - mergeCnt)) {
            SEN_HILOGW("endIndex is out of range");
            return;
        }
        endIndex = begIdx - mergeCnt;
        durationSum = 0;
        for (size_t i = endIndex; i < (begIdx + 1); ++i) {
            durationSum += continuousEvents[i].duration;
        }
        AddContinuousEventData(ContinuousEvent(continuousEvents[endIndex].time, durationSum,
            continuousEvents[endIndex].intensity, continuousEvents[endIndex].frequency));
        mergeCnt = 0;
        ++begIdx;
    }
}

void VibrationConvertCore::MergeContinuousEvents(const std::vector<ContinuousEvent> &interContinuousEvents)
{
    if (interContinuousEvents.empty()) {
        SEN_HILOGE("interContinuousEvents is empty");
        return;
    }
    size_t interTimeSize = interContinuousEvents.size();
    size_t j = 0;
    for (size_t i = 0; i < interTimeSize; ++i) {
        if (i < j) {
            continue;
        }
        if ((interContinuousEvents[i].intensity == 0) || (interContinuousEvents[i].duration < EPS_MIN)) {
            continue;
        }
        double durationSum = 0.0;
        for (; j < (interTimeSize - 1); ++j) {
            durationSum += interContinuousEvents[j].duration;
            if ((interContinuousEvents[j].intensity != 0) &&
                ((interContinuousEvents[j].time + durationSum) == interContinuousEvents[j + 1].time)) {
                continue;
            }
            if (j == i) {
                AddContinuousEventData(ContinuousEvent(interContinuousEvents[i].time, interContinuousEvents[i].duration,
                    interContinuousEvents[i].intensity, interContinuousEvents[i].frequency));
                break;
            }
            CombinateContinuousEvents(interContinuousEvents, i, j);
        }
    }
}

void VibrationConvertCore::AddContinuousEventData(const ContinuousEvent &continuousEvent)
{
    continuousEvents_.push_back(continuousEvent);
}
}  // namespace Sensors
}  // namespace OHOS
