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

#ifndef ONSET_H
#define ONSET_H

#include <cfloat>
#include <cmath>
#include <functional>
#include <numeric>
#include <optional>

#include "peak_finder.h"

namespace OHOS {
namespace Sensors {
struct OnsetInfo {
    /** If True, detected onset events are backtracked to the nearest. preceding minimum of energy.
     *  This is primarily useful when using onsets as slice points for segmentation.
     */
    bool backTrackFlag { false };
    /** Compute a spectral flux onset strength envelope. */
    std::vector<double> envelopes;
    std::vector<int32_t> idxs;
    std::vector<double> times;
    void Clear() {
        backTrackFlag = false;
        envelopes.clear();
        idxs.clear();
        times.clear();
    }
};

class Onset {
public:
    Onset() = default;
    ~Onset() = default;

    /**
     * @brief Locate note onset events by picking peaks in an onset strength envelope..
     *
     * @param data audio time-series.
     * @param nFft length of the windowed signal after padding with zeros. The number of rows in the STFT matrix D
     *  is (1 + n_fft/2). The default value, n_fft=2048 samples.
     * @param hopLength If unspecified, defaults to win_length / 4.
     * @param onsets
     *
     * @return Returns <b>0</b> if the operation is successful; returns a negative value otherwise.
     */
    int32_t CheckOnset(const std::vector<double> &data, int32_t nFft, int32_t hopLength, OnsetInfo &onsetInfo);

private:
    int32_t Sfft(const std::vector<double> &data, int32_t hopLength, int32_t &frmCount,
        std::vector<float> &magnitudes, int32_t &numBins);
    int32_t GetMelBias(int32_t numBins, int32_t nFft, size_t &frmCount, std::vector<double> &melBias);
    std::vector<double> MatrixDot(size_t matrixAcols, const std::vector<double> &matrixA,
        size_t matrixBcols, const std::vector<double> &matrixB);
    std::vector<double> MatrixDiff(size_t valueCols, const std::vector<double> &values);
    std::vector<double> PowerDB(const std::vector<double> &values);
    std::optional<double> Median(const std::vector<double> &values);
    std::optional<double> Mean(const std::vector<double> &values);

private:
    bool htkFlag_ { false };
    OnsetInfo onsetInfo_;
};
}  // namespace Sensors
}  // namespace OHOS
#endif // ONSET_H