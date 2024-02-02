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

#ifndef VIBRATION_CONVERT_CORE_H
#define VIBRATION_CONVERT_CORE_H

#include <mutex>
#include <vector>

#include <fcntl.h>

#include "frequency_estimation.h"
#include "intensity_processor.h"
#include "onset.h"
#include "peak_finder.h"
#include "singleton.h"
#include "vibration_convert_type.h"

namespace OHOS {
namespace Sensors {
/**
 * Merge data with onset and transient
*/
struct UnionTransientEvent {
    bool transientEventFlag = false;;
    int32_t onsetIdx { 0 };
    double onsetTime { 0.0 };

    UnionTransientEvent() = default;
    UnionTransientEvent(int32_t onsetIdx, double onsetTime, bool transientEventFlag = false) :
        transientEventFlag(transientEventFlag), onsetIdx(onsetIdx), onsetTime(onsetTime) {}
};

struct IntensityData {
    double rmseEnvelope { 0.0 };
    double rmseTimex { 0.0 };
    double rmseBand { 0.0 };
    int32_t rmseIntensityNorm { 0 };
    double rmseTimeNorm { 0.0 };

    IntensityData() = default;
    IntensityData(double envelope, double time, double band, int32_t intensityNorm, double timeNorm) :
        rmseEnvelope(envelope), rmseTimex(time), rmseBand(band), rmseIntensityNorm(intensityNorm),
        rmseTimeNorm(timeNorm) {}
};

struct ContinuousEvent {
    double time { 0.0 };
    double duration { 0.0 };
    int32_t intensity { 0 };
    int32_t frequency { 0 };

    ContinuousEvent(double time, double duration, int32_t intensity, int32_t frequency) : time(time), duration(duration),
        intensity(intensity), frequency(frequency) {}
};

struct TransientEvent {
    double time { 0.0 };
    int32_t intensity { 0 };
    int32_t frequency { 0 };

    TransientEvent(double time, int32_t intensity, int32_t frequency) : time(time), intensity(intensity),
        frequency(frequency) {}
};

/**
 * @brief The system parameters for converting audio to vibration will be read out from the system parameter
 *  file in the future
 */
struct ConvertSystemParameters {
    /** if the # flag is Yes, long and short events are stored in blocks, and if No, they are stored in
     *  chronological order*/
    bool blockStoreFlag { false };
    /** Has the onset alignment algorithm been used */
    bool onsetBacktrackFlag { false };
    /** Should use absolute time or window start time */
    bool useAbsTimeFlag { true };
    /** Linear processing for intensity. */
    bool intensityUseLinearFlag { true };
    /** Segmented envelope processing. */
    bool splitSegmentFlag { false };
    /** The energy difference within an envelope is less than c_ i_ combinate_ Delta is merged,
     *  with a value range of<5 */
    bool intensityCmbFlag { true };
    /** Monotonic merging within the envelope, if c_monotonicity_flag is true,
     *  c_slop_cmb_flag There's no need to deal. */
    bool monotonicityCmbFlag { false };
    /** Support monotonic adjacent point slopes less than c_slop_delta, and c_slop_cmb_flag with true then merge. */
    bool slopCmbFlag { false };
    /** When calculating intensity, align the center and add half frames of 0 at the beginning and end respectively. */
    bool centerPaddingFlag { true };
};

class VibrationConvertCore : public Singleton<VibrationConvertCore> {
public:
    VibrationConvertCore() = default;
    ~VibrationConvertCore() = default;

    /**
     * @brief Convert audio files to haptic events.
     *
     * @param audioSetting: the set audio sampling threshold. For details, see {@link AudioSetting}.
     * @param audioDatas: the audio data.
     * @param hapticEvents: the information of the event, after translated. For details, see {@link HapticEvent}.
     *
     * @return Returns <b>0</b> if the operation is successful; returns a negative value otherwise.
     */
    int32_t ConvertAudioToHaptic(const AudioSetting &audioSetting, const std::vector<double> &audioDatas,
        std::vector<HapticEvent> &hapticEvents);

private:
    DISALLOW_COPY_AND_MOVE(VibrationConvertCore);
    int32_t ResampleAudioData(const std::vector<double> &srcDatas);
    std::vector<double> PreprocessAudioData();
    int32_t PreprocessParameter(const std::vector<double> &datas, int32_t &onsetHopLength, double &lowerDelta);
    std::vector<int32_t> MapOnsetHop(const std::vector<int32_t> &drwIdxs, int32_t onsetHopLength);
    double CalcRmsLowerData(size_t dataSize, const std::vector<double> &rmses, const std::vector<int32_t> &newDrwIdxs);
    int32_t CalcOnsetHopLength(const std::vector<double> &datas, double rmseMax, size_t newDrwIdxLen,
        bool &continuousEventFlag, int32_t &newOnsetHopLen);
    /**
     * Before vibration conversion, amplitude density is used to classify audio files, determine whether there are
     * long events in the audio files, and adjust the onset detection window.
     */
    int32_t IsIncludeContinuoustEvent(const std::vector<double> &datas, int32_t &longestCount,
        double &unzeroDensity, bool &isIncludeContinuoustEvent);
    int32_t ConvertTransientEvent(const std::vector<double> &datas, int32_t onsetHopLength,
        std::vector<UnionTransientEvent> &unionTransientEvents);
    std::vector<UnionTransientEvent> DetectOnset(const std::vector<double> &audioDatas, int32_t onsetHopLength);
    std::vector<bool> IsTransientEvent(const std::vector<double> &datas, const std::vector<int32_t> &onsetIdxs);
    void GetUnzeroCount(const std::vector<double> &localDatas, int32_t &unzeroCount, double &unzeroDensity);
    bool IsTransientEventFlag(int32_t unzeroCount, double unzeroDensity);
    void TranslateAnchorPoint(const std::vector<int32_t> &amplitudePeakPos,
        std::vector<UnionTransientEvent> &unionTransientEvents);
    void TranslateAnchorPoint(int32_t amplitudePeakPos, int32_t &amplitudePeakIdx, double &amplitudePeakTime);
    int32_t DetectRmsIntensity(const std::vector<double> &datas, double rmsILowerDelta,
        std::vector<IntensityData> &intensityDatas);
    std::vector<int32_t> DetectFrequency(const std::vector<double> &datas, const std::vector<int32_t> &rmseIntensityNorms);
    std::vector<double> StartTimeNormalize(int32_t rmseLen);

private:
    void OutputTransientEvents(const std::vector<UnionTransientEvent> &unionTransientEvents,
        const std::vector<IntensityData> &intensityDatas, const std::vector<int32_t> &freqNorms,
        std::vector<int32_t> &transientIndexs, std::vector<double> &transientEventTimes);
    void OutputTransientEventsByInsertTime(const std::vector<double> &onsetTimes,
        const std::vector<IntensityData> &intensityDatas, const std::vector<int32_t> &freqNorms,
        std::vector<int32_t> &transientIndexs, std::vector<double> &transientEventTimes);
    void GetIndex(const UnionTransientEvent &unionTransientEvent, const std::vector<IntensityData> &intensityDatas);
    void OutputTransientEventsAlign(const std::vector<UnionTransientEvent> &unionTransientEvents,
        const std::vector<IntensityData> &intensityDatas, const std::vector<int32_t> &freqNorms,
        std::vector<int32_t> &transientIndexs, std::vector<double> &transientEventTimes);
    void OutputTransientEventsDirect(const std::vector<UnionTransientEvent> &unionTransientEvents,
        const std::vector<IntensityData> &intensityDatas, const std::vector<int32_t> &freqNorms,
        std::vector<int32_t> &transientIndexs, std::vector<double> &transientEventTimes);
    void AddTransientEventData(TransientEvent transientEvent);
    void OutputAllContinuousEvent(const std::vector<IntensityData> &intensityDatas,
        const std::vector<int32_t> transientIndexs, const std::vector<int32_t> &freqNorms,
        const std::vector<bool> &transientEventFlags);
    void OutputAllContinuousEventByUnseg(const std::vector<IntensityData> &intensityDatas,
        const std::vector<int32_t> transientIndexs, const std::vector<int32_t> &freqNorms,
        const std::vector<bool> &transientEventFlags);
    void FillDefaultContinuousEvents(const std::vector<double> &rmseTimeNorms, std::vector<double> &times,
        std::vector<double> &durations);
    int32_t InsertTransientEvent(const std::vector<double> &rmseTimeNorms,
        const std::vector<int32_t> &transientIndexs, const std::vector<bool> &transientEventFlags,
        std::vector<double> &times, std::vector<double> &durations);
    void MergeContinuousEvents(const std::vector<ContinuousEvent> &interContinuousEvents);
    void AddContinuousEventData(const ContinuousEvent &continuousEvent);
    int32_t GetAudioData();
    void StoreHapticEvent();
    void StoreEventSequence();
    void StoreEventBlock();
    void CombinateContinuousEvents(const std::vector<ContinuousEvent> &continuousEvents, int32_t startIdx, int32_t endIdx);
    /**
     * Using transient events instead of onse
     */
    void EmplaceOnsetTime(bool flag, int32_t idx, double time, std::vector<UnionTransientEvent> &unionTransientEvents);
    std::vector<double> GetLocalEnvelope(const std::vector<double> &datas);
    bool GetRmseLowerDelta(double lowerDelta, const std::vector<double> &rmses, double &lowestDelta);
    bool GetTransientEventFlag(const std::vector<double> &datas, int32_t onsetIdx);
    std::vector<bool> GetTransientEventFlags(const std::vector<double> &datas, const std::vector<int32_t> &onsetIdxs);

private:
    ConvertSystemParameters systemPara_;
    AudioSetting audioSetting_;
    std::vector<double> srcAudioDatas_;
    std::vector<ContinuousEvent> continuousEvents_;
    std::vector<TransientEvent> transientEvents_;
    std::vector<HapticEvent> hapticEvents_;
    bool continuousEventExistFlag_ { false };
    IntensityProcessor intensityProcessor_;
    FrequencyEstimation frequencyEstimation_;
    PeakFinder peakFinder_;
    Onset onset_;
    int32_t onsetMinSkip_ { 0 };
};
}  // namespace Sensors
}  // namespace OHOS
#endif // VIBRATION_CONVERT_CORE_H