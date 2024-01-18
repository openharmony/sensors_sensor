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

#include "onset.h"

#include <algorithm>
#include <cmath>
#include <iterator>

#include "conversion_fft.h"
#include "conversion_mfcc.h"
#include "sensor_log.h"
#include "sensors_errors.h"
#include "utils.h"

#undef LOG_TAG
#define LOG_TAG "Onset"

namespace OHOS {
namespace Sensors {
namespace {
// Effective threshold of note envelope
constexpr double C_ONSET_ENV_VALIDE_THRESHOLD = 0.0001;
constexpr double POWER_DB_COEF = 10.0;
constexpr size_t N_MELS_OR_FILTERS = 128;
/** 12 + 1 semitones*/
constexpr uint32_t SEMITONE_NUM_COEFFS = 13;
constexpr double ONSET_PEAK_THRESHOLD_RATIO = 0.4;
constexpr double MIN_FREQ = 0.0;
constexpr double MAX_FREQ = SAMPLE_RATE / 2.0;
}  // namespace

std::vector<double> Onset::MatrixDot(size_t matrixAcols, const std::vector<double> &matrixA,
    size_t matrixBcols, const std::vector<double> &matrixB)
{
    if ((matrixAcols == 0) || (matrixBcols == 0)) {
        SEN_HILOGE("Invalid parameter");
        return {};
    }
    if ((matrixA.empty()) || (matrixB.empty())) {
        SEN_HILOGE("matrixA or matrixB is empty");
        return {};
    }
    size_t aRows = matrixA.size() / matrixAcols;
    size_t bRows = matrixB.size() / matrixBcols;
    std::vector<double> result(aRows * matrixBcols);
    for (size_t i = 0; i < aRows; ++i) {
        for (size_t j = 0; j < matrixBcols; ++j) {
            // bRows must equal to matrixAcols.
            int32_t idx = j * aRows + i;
            double sum = 0.0;
            for (size_t k = 0; k < bRows; ++k) {
                // Multiply each column of the matrixB by each row of the matrixA, and then sum it up.
                sum += matrixA[k * aRows + i] * matrixB[j * bRows + k];
            }
            result[idx] = sum;
        }
    }
    return result;
}

std::vector<double> Onset::MatrixDiff(size_t valueCols, const std::vector<double> &values)
{
    if ((valueCols == 0) || (values.empty())) {
        SEN_HILOGE("Invalid parameter");
        return {};
    }
    size_t valueRows = values.size() / valueCols;
    std::vector<double> result;
    for (size_t i = 0; i < (valueCols - 1); ++i) {
        for (size_t j = 0; j < valueRows; ++j) {
            result.push_back(values[(i + 1) * valueRows + j] - values[i * valueRows + j]);
        }
    }
    return result;
}

std::optional<double> Onset::Median(const std::vector<double> &values)
{
    if (values.empty()) {
        SEN_HILOGE("values is empty");
        return std::nullopt;
    }
    std::vector<double> result = values;
    sort(result.begin(), result.end());
    double valueMedian = result[result.size() / 2];
    return valueMedian;
}

std::optional<double> Onset::Mean(const std::vector<double> &values)
{
    if (values.empty()) {
        SEN_HILOGE("values is empty");
        return std::nullopt;
    }
    double sumValue = accumulate(values.begin(), values.end(), 0);
    return sumValue / values.size();
}

// Need to subtract an offset value.
std::vector<double> Onset::PowerDB(const std::vector<double> &values)
{
    std::vector<double> logSpectrum;
    for (size_t i = 0; i < values.size(); ++i) {
        logSpectrum.push_back(POWER_DB_COEF * log(std::max(EPS_MIN, values[i])));
    }
    return logSpectrum;
}

int32_t Onset::Sfft(const std::vector<double> &data, int32_t hopLength, int32_t &frmCount,
    std::vector<float> &magnitudes, int32_t &numBins)
{
    if (data.empty()) {
        SEN_HILOGE("data is empty");
        return Sensors::ERROR;
    }
    ConversionFFT convFft;
    FFTInputPara fftPara;
    fftPara.sampleRate = SAMPLE_RATE;
    fftPara.fftSize = NFFT;
    fftPara.hopSize = hopLength;
    fftPara.windowSize = NFFT;
    int32_t ret = convFft.Init(fftPara);
    if (ret != Sensors::SUCCESS) {
        SEN_HILOGE("Init failed");
        return Sensors::ERROR;
    }
    numBins = convFft.GetNumBins();
    ret = convFft.Process(data, frmCount, magnitudes);
    if (ret != Sensors::SUCCESS) {
        SEN_HILOGE("Process failed");
        return Sensors::ERROR;
    }
    return Sensors::SUCCESS;
}

int32_t Onset::GetMelBias(int32_t numBins, int32_t nFft, size_t &frmCount, std::vector<double> &melBias)
{
    MfccInputPara para;
    para.sampleRate = SAMPLE_RATE;
    para.nMels = static_cast<int32_t>(N_MELS_OR_FILTERS);
    para.minFreq = MIN_FREQ;
    para.maxFreq = MAX_FREQ;
    // Use slaneyBias if htkFlag_ is true.
    if (htkFlag_) {
        ConversionMfcc mfcc;
        uint32_t numCoeffs = SEMITONE_NUM_COEFFS;
        if (mfcc.Init(numBins, numCoeffs, para) != Sensors::SUCCESS) {
            SEN_HILOGE("Init failed");
            return Sensors::ERROR;
        }
        melBias = mfcc.GetMelFilterBank();
        frmCount = melBias.size() / N_MELS_OR_FILTERS;
        return Sensors::SUCCESS;
    } else {
        ConversionMfcc mfcc;
        if (mfcc.FiltersMel(nFft, para, frmCount, melBias) != Sensors::SUCCESS) {
            SEN_HILOGE("FiltersMel failed");
            return Sensors::ERROR;
        }
        return Sensors::SUCCESS;
    }
}

int32_t Onset::CheckOnset(const std::vector<double> &data, int32_t nFft, int32_t hopLength, OnsetInfo &onsetInfo)
{
    CALL_LOG_ENTER;
    if ((data.size() < ONSET_HOP_LEN) || (nFft == 0) || (hopLength == 0)) {
        SEN_HILOGE("Invalid parameter, data:%{public}zu, nFft:%{public}d, hopLength:%{public}d",
            data.size(), nFft, hopLength);
        return Sensors::PARAMETER_ERROR;;
    }
    std::vector<float> magnitudes;
    int32_t sfftFrmCount;
    int32_t numBins;
    if (Sfft(data, hopLength, sfftFrmCount, magnitudes, numBins) != Sensors::SUCCESS) {
        SEN_HILOGE("Sfft failed");
        return Sensors::ERROR;
    }
    if (magnitudes.empty()) {
        SEN_HILOGE("magnitudes is empty");
        return Sensors::ERROR;
    }
    std::vector<double> frmMagnitudes;
    for (size_t i = 0; i < magnitudes.size(); ++i) {
        frmMagnitudes.push_back(static_cast<double>(magnitudes[i]));
        frmMagnitudes[i] = pow(frmMagnitudes[i], 2);
    }
    size_t biasFrmCount = 0;
    std::vector<double> melBias;
    if (GetMelBias(numBins, nFft, biasFrmCount, melBias) != Sensors::SUCCESS) {
        SEN_HILOGE("GetMelBias failed");
        return Sensors::ERROR;
    }
    std::vector<double> onsetEnvelope = MatrixDot(biasFrmCount, melBias, sfftFrmCount, frmMagnitudes);
    if (onsetEnvelope.empty()) {
        SEN_HILOGE("onsetEnvelope is empty");
        return Sensors::ERROR;
    }
    std::vector<double> dbEnvelope = PowerDB(onsetEnvelope);
    if (dbEnvelope.empty()) {
        SEN_HILOGE("dbEnvelope is empty");
        return Sensors::ERROR;
    }
    std::vector<double> dbEnvelopeDiff = MatrixDiff(sfftFrmCount, dbEnvelope);
    if (dbEnvelopeDiff.empty()) {
        SEN_HILOGE("dbEnvelopeDiff is empty");
        return Sensors::ERROR;
    }
    for (size_t i = 0; i < dbEnvelopeDiff.size(); ++i) {
        dbEnvelopeDiff[i] = (IsGreatNotEqual(dbEnvelopeDiff[i], 0.0)) ? dbEnvelopeDiff[i] : 0.0;
    }
    if (sfftFrmCount <= 1) {
        SEN_HILOGE("sfftFrmCount is less than or equal to 1");
        return Sensors::ERROR;
    }
    size_t cols = sfftFrmCount - 1;
    size_t rows = dbEnvelopeDiff.size() / cols;
    onsetInfo_.Clear();
    std::vector<double> oneFrmValues;
    for (size_t i = 0; i < cols; ++i) {
        oneFrmValues.assign(dbEnvelopeDiff.begin() + i * rows, dbEnvelopeDiff.begin() + ((i + 1) * rows));
        std::optional<double> median = Median(oneFrmValues);
        if (!median) {
            SEN_HILOGE("Median failed");
            return Sensors::ERROR;
        }
        onsetInfo_.envelopes.push_back(median.value());
    }
    double envelopeMax = *max_element(onsetInfo_.envelopes.begin(), onsetInfo_.envelopes.end());
    if (envelopeMax < C_ONSET_ENV_VALIDE_THRESHOLD) {
        onsetInfo_.envelopes.clear();
        for (size_t i = 0; i < cols; ++i) {
            oneFrmValues.assign(dbEnvelopeDiff.begin() + i * rows, dbEnvelopeDiff.begin() + ((i + 1) * rows));
            std::optional<double> mean = Mean(oneFrmValues);
            if (!mean) {
                SEN_HILOGE("Mean failed");
                return Sensors::ERROR;
            }
            onsetInfo_.envelopes.push_back(mean.value());
        }
    }
    double tPerFrame = static_cast<double>(hopLength) / SAMPLE_RATE;
    PeakFinder peakFinder;
    onsetInfo_.idxs = peakFinder.DetectPeak(onsetInfo_.envelopes, ONSET_PEAK_THRESHOLD_RATIO);
    for (size_t i = 0; i < onsetInfo_.idxs.size(); ++i) {
        onsetInfo_.times.push_back(onsetInfo_.idxs[i] * tPerFrame);
    }
    onsetInfo = onsetInfo_;
    return Sensors::SUCCESS;
}
}  // namespace Sensors
}  // namespace OHOS