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

#include "fft.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "sensor_log.h"
#include "sensors_errors.h"

#undef LOG_TAG
#define LOG_TAG "Fft"

namespace OHOS {
namespace Sensors {
namespace {
constexpr uint32_t MAX_FAST_BITS { 16 };
constexpr int32_t NUM_SAMPLES_MIN { 2 };
constexpr double HAMMING_WND_UP { 0.54 };
constexpr double HAMMING_WND_DOWN { 0.46 };
constexpr float VOLUME_MIN { 0.000001 };
constexpr double AMP_TO_DB_COEF { 20.0 };
constexpr bool TRANSFORM_INVERSE_FLAG { true };
} // namespace

Fft::~Fft()
{
    if (fftBitTable_ == nullptr) {
        return;
    }
    for (uint32_t i = 0; i < MAX_FAST_BITS; ++i) {
        if (fftBitTable_[i] != nullptr) {
            free(fftBitTable_[i]);
            fftBitTable_[i] = nullptr;
        }
    }
    free(fftBitTable_);
    fftBitTable_ = nullptr;
}

inline uint32_t Fft::FastReverseBits(uint32_t pos, uint32_t numBits)
{
    if (numBits <= MAX_FAST_BITS && numBits >= 1) {
        return fftBitTable_[numBits - 1][pos];
    }
    return ReverseBits(pos, numBits);
}

int32_t Fft::AlgInitFFT()
{
    // use malloc for 16 byte alignment
    fftBitTable_ = (uint32_t **)malloc(MAX_FAST_BITS * sizeof(uint32_t *));
    CHKPR(fftBitTable_, Sensors::ERROR);
    uint32_t length = 2;
    for (uint32_t j = 1; j <= MAX_FAST_BITS; ++j) {
        fftBitTable_[j - 1] = (uint32_t *)malloc(length * sizeof(uint32_t));
        CHKPR(fftBitTable_[j - 1], Sensors::ERROR);
        for (uint32_t i = 0; i < length; ++i) {
            fftBitTable_[j - 1][i] = ReverseBits(i, j);
        }
        length <<= 1;
    }
    return Sensors::SUCCESS;
}

/*
 * Complex Fast Fourier Transform
 */
int32_t Fft::AlgFFT(bool inverseTransform, FftParaAndResult &paraRes)
{
    uint32_t numSamples = static_cast<uint32_t>(paraRes.numSamples);
    if (!IsPowerOfTwo(numSamples)) {
        SEN_HILOGE("The parameter is invalid,numSamples:%{public}d", numSamples);
        return Sensors::PARAMETER_ERROR;
    }
    if (PreprocessData(numSamples, paraRes) != Sensors::SUCCESS) {
        SEN_HILOGE("PreprocessData failed");
        return Sensors::ERROR;
    }
    // Do the FFT itself...
    int32_t blockEnd = 1;
    double angleNumerator = inverseTransform ? -(2 * M_PI) : (2 * M_PI);
    for (int32_t blockSize = 2; blockSize <= paraRes.numSamples; blockSize <<= 1) {
        double deltaAngle = angleNumerator / static_cast<double>(blockSize);
        float twoSinMagnitude = sin(-2 * deltaAngle);
        float oneSinMagnitude = sin(-deltaAngle);
        float twoCosMagnitude = cos(-2 * deltaAngle);
        float oneCosMagnitude = cos(-deltaAngle);
        float w = 2 * oneCosMagnitude;
        for (int32_t i = 0; i < paraRes.numSamples; i += blockSize) {
            float secondAmpReal = twoCosMagnitude;
            float firstAmpReal = oneCosMagnitude;
            float thirdAmpImaginary = twoSinMagnitude;
            float secondAmpImaginary = oneSinMagnitude;
            for (int32_t j = i, n = 0; n < blockEnd; ++j, ++n) {
                float zerothAmpReal = w * firstAmpReal - secondAmpReal;
                secondAmpReal = firstAmpReal;
                firstAmpReal = zerothAmpReal;
                float firstAmpImaginary = w * secondAmpImaginary - thirdAmpImaginary;
                thirdAmpImaginary = secondAmpImaginary;
                secondAmpImaginary = firstAmpImaginary;
                int32_t k = j + blockEnd;
                float real = zerothAmpReal * paraRes.realOut[k] - firstAmpImaginary * paraRes.imagOut[k];
                float imaginary = zerothAmpReal * paraRes.imagOut[k] + firstAmpImaginary * paraRes.realOut[k];
                paraRes.realOut[k] = paraRes.realOut[j] - real;
                paraRes.imagOut[k] = paraRes.imagOut[j] - imaginary;
                paraRes.realOut[j] += real;
                paraRes.imagOut[j] += imaginary;
            }
        }
        blockEnd = blockSize;
    }
    // Need to normalize if inverse transform
    if (inverseTransform) {
        Normalize(paraRes);
    }
    return Sensors::SUCCESS;
}

void Fft::Normalize(FftParaAndResult &paraRes)
{
    float denom = static_cast<float>(paraRes.numSamples);
    for (int32_t i = 0; i < paraRes.numSamples; ++i) {
        paraRes.realOut[i] /= denom;
        paraRes.imagOut[i] /= denom;
    }
}

int32_t Fft::PreprocessData(uint32_t numSamples, FftParaAndResult &paraRes)
{
    if ((fftBitTable_ == nullptr) && (AlgInitFFT() != Sensors::SUCCESS)) {
        SEN_HILOGE("fftBitTable_ failed to allocate memory");
        return Sensors::ERROR;
    }
    uint32_t numBits = ObtainNumberOfBits(numSamples);
    if (numBits < 1) {
        SEN_HILOGE("numBits is an invalid value");
        return Sensors::ERROR;
    }
    // Do simultaneous data copy and bit-reversal ordering into outputs...
    for (uint32_t i = 0; i < numSamples; ++i) {
        uint32_t j = FastReverseBits(i, numBits);
        paraRes.realOut[j] = paraRes.realIn[i];
        paraRes.imagOut[j] = (paraRes.imagIn.empty()) ? 0.0F : paraRes.imagIn[i];
    }
    return Sensors::SUCCESS;
}

/*
 * Real Fast Fourier Transform
 *
 * This function was based on the code in Numerical Recipes in C.
 * In Num. Rec., the inner loop is based on a single 1-based array
 * of interleaved real and imaginary numbers.  Because we have two
 * separate zero-based arrays, our indices are quite different.
 * Here is the correspondence between Num. Rec. indices and our indices:
 *
 * i1  <->  real[i]
 * i2  <->  imag[i]
 * i3  <->  real[n/2-i]
 * i4  <->  imag[n/2-i]
 */
int32_t Fft::AlgRealFFT(FftParaAndResult &paraRes)
{
    if (paraRes.numSamples == 0) {
        SEN_HILOGE("The divisor should not be 0");
        return Sensors::PARAMETER_ERROR;
    }
    int32_t half = paraRes.numSamples / 2;
    float theta = M_PI / half;
    for (int32_t i = 0; i < half; ++i) {
        para_.realIn[i] = paraRes.realIn[2 * i];
        para_.imagIn[i] = paraRes.realIn[2 * i + 1];
    }
    AlgFFT(!TRANSFORM_INVERSE_FLAG, para_);
    float wtemp = static_cast<float>(sin(F_HALF * theta));
    float wpr = -2.0 * wtemp * wtemp;
    float wpi = static_cast<float>(sin(theta));
    float wr = 1.0 + wpr;
    float wi = wpi;
    float previousHalfReal = 0.0F;
    for (int32_t i = 1; i < (half / 2); ++i) {
        int32_t i3 = half - i;
        previousHalfReal = F_HALF * (para_.realOut[i] + para_.realOut[i3]);
        float previousHalfIm = F_HALF * (para_.imagOut[i] - para_.imagOut[i3]);
        float lastHalfReal = F_HALF * (para_.imagOut[i] + para_.imagOut[i3]);
        float lastHalfIm = -F_HALF * (para_.realOut[i] - para_.realOut[i3]);
        para_.realOut[i] = previousHalfReal + wr * lastHalfReal - wi * lastHalfIm;
        para_.imagOut[i] = previousHalfIm + wr * lastHalfIm + wi * lastHalfReal;
        para_.realOut[i3] = previousHalfReal - wr * lastHalfReal + wi * lastHalfIm;
        para_.imagOut[i3] = -previousHalfIm + wr * lastHalfIm + wi * lastHalfReal;
        wtemp = wr;
        wr = wtemp * wpr - wi * wpi + wr;
        wi = wi * wpr + wtemp * wpi + wi;
    }
    previousHalfReal = para_.realOut[0];
    para_.realOut[0] = previousHalfReal + para_.imagOut[0];
    para_.imagOut[0] = previousHalfReal - para_.imagOut[0];
    for (int32_t i = 0; i < half; ++i) {
        paraRes.realOut[i] = para_.realOut[i];
        paraRes.imagOut[i] = para_.imagOut[i];
    }
    return Sensors::SUCCESS;
}

/*
 * AlgPowerSpectrum
 *
 * This function computes the same as AlgRealFFT, above, but
 * adds the squares of the real and imaginary part of each
 * coefficient, extracting the power and throwing away the
 * phase.
 *
 * For speed, it does not call AlgRealFFT, but duplicates some
 * of its code.
 */
int32_t Fft::AlgPowerSpectrum(int32_t numSamples, const std::vector<float> &in, std::vector<float> &out)
{
    if (numSamples == 0) {
        SEN_HILOGE("The divisor should not be 0");
        return Sensors::PARAMETER_ERROR;
    }
    int32_t half = numSamples / 2;
    float theta = M_PI / half;
    for (int32_t i = 0; i < half; ++i) {
        para_.realIn[i] = in[2 * i];
        para_.imagIn[i] = in[2 * i + 1];
    }
    AlgFFT(!TRANSFORM_INVERSE_FLAG, para_);
    float wtemp = static_cast<float>(sin(F_HALF * theta));
    float wpr = -2.0 * wtemp * wtemp;
    float wpi = static_cast<float>(sin(theta));
    float wr = 1.0 + wpr;
    float wi = wpi;
    float previousHalfReal = 0.0F;
    float rt = 0.0F;
    float it = 0.0F;
    for (int32_t i = 1; i < (half / 2); ++i) {
        int32_t i3 = half - i;
        previousHalfReal = F_HALF * (para_.realOut[i] + para_.realOut[i3]);
        float previousHalfIm = F_HALF * (para_.imagOut[i] - para_.imagOut[i3]);
        float lastHalfReal = F_HALF * (para_.imagOut[i] + para_.imagOut[i3]);
        float lastHalfIm = -F_HALF * (para_.realOut[i] - para_.realOut[i3]);
        rt = previousHalfReal + wr * lastHalfReal - wi * lastHalfIm;
        it = previousHalfIm + wr * lastHalfIm + wi * lastHalfReal;
        out[i] = rt * rt + it * it;
        rt = previousHalfReal - wr * lastHalfReal + wi * lastHalfIm;
        it = -previousHalfIm + wr * lastHalfIm + wi * lastHalfReal;
        out[i3] = rt * rt + it * it;
        wr = (wtemp = wr) * wpr - wi * wpi + wr;
        wi = wi * wpr + wtemp * wpi + wi;
    }
    rt = (previousHalfReal = para_.realOut[0]) + para_.imagOut[0];
    it = previousHalfReal - para_.imagOut[0];
    out[0] = rt * rt + it * it;
    rt = para_.realOut[half / 2];
    it = para_.imagOut[half / 2];
    out[half / 2] = rt * rt + it * it;
    return Sensors::SUCCESS;
}

int32_t Fft::WindowFunc(int32_t whichFunction, int32_t numSamples, float *out)
{
    if (numSamples < NUM_SAMPLES_MIN) {
        SEN_HILOGE("numSamples are less than 2");
        return Sensors::PARAMETER_ERROR;
    }
    switch (whichFunction) {
        case WND_TYPE_BARTLETT: {
            float samplesCount = static_cast<float>(numSamples);
            // Bartlett (triangular) window
            for (int32_t i = 0; i < (numSamples / 2); ++i) {
                out[i] *= i / (samplesCount / 2);
                out[i + (numSamples / 2)] *= 1.0 - (i / (samplesCount / 2));
            }
            return Sensors::SUCCESS;
        }
        case WND_TYPE_HAMMING: {
            // Hamming
            for (int32_t i = 0; i < numSamples; ++i) {
                out[i] *= HAMMING_WND_UP - HAMMING_WND_DOWN * cos(2 * M_PI * i / (numSamples - 1));
            }
            return Sensors::SUCCESS;
        }
        case WND_TYPE_HANNING: {
            // Hanning
            for (int32_t i = 0; i < numSamples; ++i) {
                out[i] *= F_HALF - F_HALF * cos(2 * M_PI * i / (numSamples - 1));
            }
            return Sensors::SUCCESS;
        }
        default: {
            SEN_HILOGE("WindowType: %{public}d invalid", whichFunction);
            return Sensors::ERROR;
        }
    }
}

int32_t Fft::GenWindow(int32_t whichFunction, int32_t numSamples, std::vector<float> &window)
{
    if (numSamples < NUM_SAMPLES_MIN) {
        SEN_HILOGE("numSamples are less than 2");
        return Sensors::PARAMETER_ERROR;
    }
    switch (whichFunction) {
        case WND_TYPE_BARTLETT: {
            float samplesCount = static_cast<float>(numSamples);
            // Bartlett (triangular) window
            for (int32_t i = 0; i < (numSamples / 2); ++i) {
                window[i] = i / (samplesCount / 2);
                window[i + (numSamples / 2)] = 1.0 - (i / (samplesCount / 2));
            }
            return Sensors::SUCCESS;
        }
        case WND_TYPE_HAMMING: {
            for (int32_t i = 0; i < numSamples; ++i) {
                window[i] = HAMMING_WND_UP - HAMMING_WND_DOWN * cos(2 * M_PI * i / (numSamples - 1));
            }
            return Sensors::SUCCESS;
        }
        case WND_TYPE_HANNING: {
            // Hanning
            for (int32_t i = 0; i < numSamples; ++i) {
                window[i] = F_HALF - F_HALF * cos(2 * M_PI * i / (numSamples - 1));
            }
            return Sensors::SUCCESS;
        }
        default: {
            SEN_HILOGE("WindowType: %{public}d invalid", whichFunction);
            return Sensors::ERROR;
        }
    }
}

void Fft::Init(int32_t fftSize)
{
    fftSize_ = fftSize;
    half_ = fftSize / 2;

    fftParaRes_ = FftParaAndResult(fftSize_);
    para_ = FftParaAndResult(half_);
}

std::vector<float> Fft::GetReal() const
{
    return fftParaRes_.realOut;
}

std::vector<float> Fft::GetImg() const
{
    return fftParaRes_.imagOut;
}

void Fft::CalcFFT(const std::vector<float> &data, const std::vector<float> &window)
{
    // windowing
    for (int32_t i = 0; i < fftSize_; ++i) {
        fftParaRes_.realIn[i] = data[i] * window[i];
    }
    // fftParaRes_.realIn include realIn and imageIn!
    AlgRealFFT(fftParaRes_);
}

void Fft::ConvertPolar(std::vector<float> &magnitude, std::vector<float> &phase)
{
    for (int32_t i = 0; i < half_; ++i) {
        /* compute power */
        float power = pow(fftParaRes_.realOut[i], 2) + pow(fftParaRes_.imagOut[i], 2);
        /* compute magnitude and phase */
        magnitude[i] = sqrt(power);
        phase[i] = atan2(fftParaRes_.imagOut[i], fftParaRes_.realOut[i]);
    }
}

/* Calculate the power spectrum */
void Fft::CalculatePowerSpectrum(const std::vector<float> &data, const std::vector<float> &window,
    std::vector<float> &magnitude, std::vector<float> &phase)
{
    CalcFFT(data, window);
    ConvertPolar(magnitude, phase);
}

void Fft::ConvertDB(const std::vector<float> &in, std::vector<float> &out)
{
    for (int32_t i = 0; i < half_; ++i) {
        if (IsLessNotEqual(in[i], VOLUME_MIN)) {
            // less than 0.1 nV
            out[i] = 0.0F; // out of range
        } else {
            out[i] = AMP_TO_DB_COEF * log10(in[i] + 1); // get to to db scale
        }
    }
}

void Fft::ConvertCart(const std::vector<float> &magnitude, const std::vector<float> &phase)
{
    /* get real and imag part */
    for (int32_t i = 0; i < half_; ++i) {
        fftParaRes_.realIn[i] = magnitude[i] * cos(phase[i]);
        fftParaRes_.imagIn[i] = magnitude[i] * sin(phase[i]);
    }
    /* zero negative frequencies */
    std::fill(fftParaRes_.realIn.begin() + half_, fftParaRes_.realIn.begin() + half_ + half_, 0);
    std::fill(fftParaRes_.imagIn.begin() + half_, fftParaRes_.imagIn.begin() + half_ + half_, 0);
}

void Fft::CalcIFFT(const std::vector<float> &window, std::vector<float> &finalOut)
{
    // second parameter indicates inverse transform
    fftParaRes_.numSamples = fftSize_;
    AlgFFT(TRANSFORM_INVERSE_FLAG, fftParaRes_);
    for (int32_t i = 0; i < fftSize_; ++i) {
        finalOut[i] += fftParaRes_.realOut[i] * window[i];
    }
}

void Fft::InverseFFTComplex(const std::vector<float> &window, const std::vector<float> &real,
    const std::vector<float> &imaginary, std::vector<float> &finalOut)
{
    for (int32_t i = 0; i < half_; ++i) {
        fftParaRes_.realOut[i] = real[i];
        fftParaRes_.imagOut[i] = imaginary[i];
    }
    CalcIFFT(window, finalOut);
}

void Fft::InversePowerSpectrum(const std::vector<float> &window, const std::vector<float> &magnitude,
    const std::vector<float> &phase, std::vector<float> &finalOut)
{
    ConvertCart(magnitude, phase);
    CalcIFFT(window, finalOut);
}
} // namespace Sensors
} // namespace OHOS