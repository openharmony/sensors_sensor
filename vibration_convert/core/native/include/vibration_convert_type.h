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

#ifndef VIBRATION_CONVERT_TYPE_H
#define VIBRATION_CONVERT_TYPE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enumerates basic event enumeration of haptic.
 *
 * @since 1.0
 */
typedef enum EventTag {
    /** Continuous event. */
    EVENT_TAG_CONTINUOUS = 0,
    /** Transient event. */
    EVENT_TAG_TRANSIENT = 1,
} VibrateTag;

/**
 * @brief Basic event-parameters definition of haptic.
 *
 * The parameters include the haptic basic event enumeration, start time, duration, intensity and frequency.
 *
 * @since 1.0
 */
struct HapticEvent {
    /** Basic event enumeration of haptic. */
    VibrateTag vibrateTag;
    /** Start time. */
    int32_t startTime { 0 };
    /** Duration. */
    int32_t duration { 0 };
    /** Intensity. */
    int32_t intensity { 0 };
    /** Frequency. */
    int32_t frequency { 0 };
};

/**
 * @brief Defines the audio setting parameters.
 *
 * The parameters include the setting transient detection treshold and intensity treshold and
 * frequency treshold and frequency range.
 *
 * @since 1.0
 */
struct AudioSetting {
    /** 0-100. */
    int32_t transientDetection { 0 };
    /** 0-100. */
    int32_t intensityTreshold { 0 };
    /** 0-100. */
    int32_t frequencyTreshold { 0 };
    /**< Max frequency(Hz). */
    int32_t frequencyMaxValue { 0 };
    /**< Min frequency(Hz). */
    int32_t frequencyMinValue { 0 };
};

/**
 * @brief Defines the audio file attribute.
 *
 * Parameters include data sampling frequency, data sampling duration, raw data number.
 *
 * @since 1.0
 */
struct AudioAttribute {
    /** sampling frequency. */
    uint32_t sampleRate { 0 };
    /** data sampling duration. */
    uint32_t duration { 0 };
    /** number of raw data. */
    uint32_t dataCount { 0 };
};

/**
 * @brief Defines the audio file data.
 *
 * Parameters include the number of data after the sampling interval,
 * audio data, and data maximum and minimum.
 *
 * @since 1.0
 */
struct AudioData {
    /** data minimum. */
    double min { 0.0 };
    /** data maximum. */
    double max { 0.0 };
    /** audio data. */
    std::vector<double> audioDatas;
};

/**
 * @brief Defines the audio file attribute.
 *
 * Parameters include ChunkID, ChunkSize, Format, FmtChannels, SampleRate, ByteRate, BitsPerSample.
 *
 * @since 1.0
 */
struct AttributeChunk {
    char chunkID[4] { 0 }; //'R','I','F','F'
    uint32_t chunkSize { 0 };
    char format[4] { 0 };  //'W','A','V','E'
    char fmtID[4] { 0 };
    uint32_t fmtSize { 0 };
    uint16_t fmtTag { 0 };
    uint16_t fmtChannels { 0 };
    uint32_t sampleRate { 0 };
    uint32_t byteRate { 0 };
    uint16_t blockAilgn { 0 };
    uint16_t bitsPerSample { 0 };
    char dataID[4] { 0 }; //'d','a','t','a'
    uint32_t dataSize { 0 };
};

struct RawFileDescriptor {
    int32_t fd = 0;
    int64_t offset = 0;
    int64_t length = -1;
};

/** @} */
#ifdef __cplusplus
};
#endif
#endif // VIBRATION_CONVERT_TYPE_H
