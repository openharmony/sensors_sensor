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

#ifndef AUDIO_PARSING_H
#define AUDIO_PARSING_H

#include <fstream>
#include <mutex>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

#include "singleton.h"
#include "vibration_convert_type.h"

namespace OHOS {
namespace Sensors {
class AudioParsing {
public:
    explicit AudioParsing(const RawFileDescriptor &fd);
    ~AudioParsing() = default;
    int32_t GetAudioAttribute(AudioAttribute &audioAttribute) const;
    int32_t GetAudioData(int32_t samplingInterval, AudioData &audioData) const;
    int32_t ConvertAudioToHaptic(const AudioSetting &audioSetting, std::vector<HapticEvent> &hapticEvents);
    int32_t ParseAudioFile();

private:
    int32_t RawFileDescriptorCheck();
    void PrintAttributeChunk();

private:
    RawFileDescriptor rawFd_;
    AudioData audioData_;
    AttributeChunk attributeChunk_;
};
}  // namespace Sensors
}  // namespace OHOS
#endif // AUDIO_PARSING_H