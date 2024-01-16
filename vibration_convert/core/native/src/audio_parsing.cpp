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

#include "audio_parsing.h"

#include <algorithm>
#include <cerrno>
#include <cinttypes>
#include <fstream>

#include <sys/stat.h>

#include <securec.h>

#include "generate_vibration_json_file.h"
#include "sensor_log.h"
#include "sensors_errors.h"
#include "vibration_convert_core.h"
#include "vibration_convert_type.h"

#undef LOG_TAG
#define LOG_TAG "AudioParsing"

namespace OHOS {
namespace Sensors {
namespace {
constexpr int32_t MIN_SAMPLE_COUNT = 4096;
constexpr uint32_t AUDIO_DATA_CONVERSION_FACTOR = INT32_MAX;
constexpr int32_t AUDIO_DATA_MAX_NUMBER = 100000;
constexpr int64_t LSEEK_FAIL = -1;
constexpr int32_t TIME_MS = 1000;
constexpr int32_t BITS_PER_BYTE = 8;
}  // namespace

AudioParsing::AudioParsing(const RawFileDescriptor &rawFd)
{
    CALL_LOG_ENTER;
    SEN_HILOGD("handle:%{public}d, offset:%{public}" PRId64 ", length:%{public}" PRId64,
        rawFd.fd, rawFd.offset, rawFd.length);
    rawFd_ = rawFd;
}

int32_t AudioParsing::RawFileDescriptorCheck()
{
    CALL_LOG_ENTER;
    if ((rawFd_.fd < 0) || (rawFd_.offset < 0) || (rawFd_.length <= 0)) {
        SEN_HILOGE("Invalid parameter");
        return Sensors::PARAMETER_ERROR;
    }
    struct stat64 statbuf = { 0 };
    if (fstat64(rawFd_.fd, &statbuf) != 0) {
        SEN_HILOGE("fstat error, errno:%{public}d", errno);
        return Sensors::ERROR;
    }
    int64_t fileSize = static_cast<int64_t>(statbuf.st_size);
    if ((fileSize - rawFd_.offset) < rawFd_.length) {
        SEN_HILOGE("Invalid parameter rawFd");
        return Sensors::PARAMETER_ERROR;
    }
    return Sensors::SUCCESS;
}

int32_t AudioParsing::ParseAudioFile()
{
    CALL_LOG_ENTER;
    if (RawFileDescriptorCheck() != Sensors::SUCCESS) {
        SEN_HILOGE("RawFileDescriptorCheck failed");
        return Sensors::ERROR;
    }
    int64_t oft = lseek(rawFd_.fd, rawFd_.offset, SEEK_SET);
    if (oft == LSEEK_FAIL) {
        SEN_HILOGE("lseek fail, oft:%{public}" PRId64, oft);
        return Sensors::ERROR;
    }
    (void)memset_s(&attributeChunk_, sizeof(AttributeChunk), 0, sizeof(AttributeChunk));
    if (rawFd_.length < sizeof(AttributeChunk)) {
        SEN_HILOGE("Invalid parameter");
        return Sensors::PARAMETER_ERROR;
    }

    ssize_t ret = read(rawFd_.fd, &attributeChunk_, sizeof(AttributeChunk));
    if (ret <= 0) {
        SEN_HILOGE("read audio attribute failed, errno:%{public}d", errno);
        return Sensors::ERROR;
    }
    if ((attributeChunk_.bitsPerSample == 0) || (attributeChunk_.fmtChannels == 0)) {
        SEN_HILOGE("The divisor cannot be 0");
        return Sensors::ERROR;
    }
    size_t dataCount = attributeChunk_.dataSize /
        ((attributeChunk_.bitsPerSample / BITS_PER_BYTE) * attributeChunk_.fmtChannels);
    PrintAttributeChunk();
    int32_t *dataBuffer = static_cast<int32_t *>(malloc(attributeChunk_.dataSize));
    CHKPR(dataBuffer, Sensors::ERROR);
    (void)memset_s(dataBuffer, attributeChunk_.dataSize, 0, attributeChunk_.dataSize);

    if (rawFd_.length < attributeChunk_.dataSize) {
        free(dataBuffer);
        SEN_HILOGE("Invalid parameter");
        return Sensors::PARAMETER_ERROR;
    }

    ret = read(rawFd_.fd, dataBuffer, attributeChunk_.dataSize);
    if (ret <= 0) {
        free(dataBuffer);
        SEN_HILOGE("read audio data failed");
        return Sensors::ERROR;
    }
    for (size_t i = 0; i < dataCount; ++i) {
        double data = static_cast<double>(dataBuffer[i]) / AUDIO_DATA_CONVERSION_FACTOR;
        audioData_.audioDatas.push_back(data);
    }
    audioData_.max = *std::max_element(audioData_.audioDatas.begin(), audioData_.audioDatas.end());
    audioData_.min = *std::min_element(audioData_.audioDatas.begin(), audioData_.audioDatas.end());
    free(dataBuffer);
    return Sensors::SUCCESS;
}

int32_t AudioParsing::GetAudioAttribute(AudioAttribute &audioAttribute) const
{
    CALL_LOG_ENTER;
    if ((attributeChunk_.bitsPerSample == 0) || (attributeChunk_.fmtChannels == 0) || (attributeChunk_.byteRate == 0)) {
        SEN_HILOGE("The divisor cannot be 0");
        return Sensors::ERROR;
    }
    audioAttribute.sampleRate = attributeChunk_.sampleRate;
    audioAttribute.dataCount = attributeChunk_.dataSize / ((attributeChunk_.bitsPerSample / BITS_PER_BYTE) *
        attributeChunk_.fmtChannels);
    float dataSize = static_cast<float>(attributeChunk_.dataSize);
    audioAttribute.duration = static_cast<uint32_t>(dataSize / attributeChunk_.byteRate * TIME_MS);
    SEN_HILOGD("sampleRate:%{public}u, duration:%{public}u, dataCount:%{public}u",
        audioAttribute.sampleRate, audioAttribute.duration, audioAttribute.dataCount);
    return Sensors::SUCCESS;
}

int32_t AudioParsing::GetAudioData(int32_t samplingInterval, AudioData &data) const
{
    CALL_LOG_ENTER;
    if (audioData_.audioDatas.empty()) {
        SEN_HILOGE("audioDatas is empty");
        return Sensors::ERROR;
    }
    size_t dataCount = audioData_.audioDatas.size();
    if (samplingInterval < 0) {
        SEN_HILOGE("Invalid parameter");
        return Sensors::PARAMETER_ERROR;
    }
    if (samplingInterval == 0) {
        if ((dataCount % AUDIO_DATA_MAX_NUMBER) == 0 && (dataCount >= AUDIO_DATA_MAX_NUMBER)) {
            samplingInterval = dataCount / AUDIO_DATA_MAX_NUMBER;
        } else {
            samplingInterval = dataCount / AUDIO_DATA_MAX_NUMBER + 1;
        }
    }
    for (size_t i = 0; i < dataCount; ++i) {
        if (i % samplingInterval == 0) {
            data.audioDatas.push_back(audioData_.audioDatas[i]);
        }
    }
    data.max = audioData_.max;
    data.min = audioData_.min;
    SEN_HILOGD("min:%{public}lf, max:%{public}lf, audioDatas.size():%{public}zu",
        data.min, data.max, data.audioDatas.size());
    return Sensors::SUCCESS;
}

int32_t AudioParsing::ConvertAudioToHaptic(const AudioSetting &audioSetting, std::vector<HapticEvent> &hapticEvents)
{
    CALL_LOG_ENTER;
    if (audioData_.audioDatas.size() < MIN_SAMPLE_COUNT) {
        SEN_HILOGE("audioDatas less then MIN_SAMPLE_COUNT, audioDatas.size():%{public}zu", audioData_.audioDatas.size());
        return Sensors::ERROR;
    }
    VibrationConvertCore vibrationConvertCore;
    if (vibrationConvertCore.ConvertAudioToHaptic(audioSetting, audioData_.audioDatas, hapticEvents) != Sensors::SUCCESS) {
        SEN_HILOGE("ConvertAudioToHaptic failed");
        return Sensors::ERROR;
    }
    GenerateVibrationJsonFile generateJson;
    generateJson.GenerateJsonFile(hapticEvents);
    return Sensors::SUCCESS;
}

void AudioParsing::PrintAttributeChunk()
{
    CALL_LOG_ENTER;
    SEN_HILOGD("chunkID:%{public}.4s", attributeChunk_.chunkID);
    SEN_HILOGD("chunkSize:%{public}u", attributeChunk_.chunkSize);
    SEN_HILOGD("format:%{public}.4s", attributeChunk_.format);
    SEN_HILOGD("fmtID:%{public}.4s", attributeChunk_.fmtID);
    SEN_HILOGD("fmtSize:%{public}u", attributeChunk_.fmtSize);
    SEN_HILOGD("fmtTag:%{public}hu", attributeChunk_.fmtTag);
    SEN_HILOGD("fmtChannels:%{public}hu", attributeChunk_.fmtChannels);
    SEN_HILOGD("sampleRate:%{public}u", attributeChunk_.sampleRate);
    SEN_HILOGD("byteRate:%{public}u", attributeChunk_.byteRate);
    SEN_HILOGD("blockAilgn:%{public}hu", attributeChunk_.blockAilgn);
    SEN_HILOGD("bitsPerSample:%{public}hu", attributeChunk_.bitsPerSample);
    SEN_HILOGD("dataID:%{public}.4s", attributeChunk_.dataID);
    SEN_HILOGD("dataSize:%{public}u", attributeChunk_.dataSize);
}
}  // namespace Sensors
}  // namespace OHOS
