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

#include <vector>

#include <gtest/gtest.h>

#include "audio_utils.h"
#include "data.h"
#include "generate_vibration_json_file.h"
#include "sensor_log.h"
#include "sensors_errors.h"
#include "utils.h"
#include "vibration_convert_core.h"
#include "vibration_convert_type.h"

#undef LOG_TAG
#define LOG_TAG "GenerateJsonFileTest"

namespace OHOS {
namespace Sensors {
using namespace testing::ext;
using namespace OHOS::HiviewDFX;

class GenerateJsonFileTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void GenerateJsonFileTest::SetUpTestCase() {}

void GenerateJsonFileTest::TearDownTestCase() {}

void GenerateJsonFileTest::SetUp() {}

void GenerateJsonFileTest::TearDown() {}

HWTEST_F(GenerateJsonFileTest, GenerateJsonFileTest_001, TestSize.Level1)
{
    HapticEvent event = {
        .tag = EVENT_TAG_TRANSIENT,
        .startTime = 10,
        .duration = 30,
        .intensity = 90,
        .frequency = 100
    };
    HapticEvent event1 = {
        .tag = EVENT_TAG_CONTINUOUS,
        .startTime = 40,
        .duration = 56,
        .intensity = 90,
        .frequency = 100
    };
    HapticEvent event2 = {
        .tag = EVENT_TAG_CONTINUOUS,
        .startTime = 97,
        .duration = 86,
        .intensity = 77,
        .frequency = 80
    };
    HapticEvent event3 = {
        .tag = EVENT_TAG_TRANSIENT,
        .startTime = 200,
        .duration = 30,
        .intensity = 44,
        .frequency = 55
    };
    std::vector<HapticEvent> vtEvents;
    vtEvents.push_back(event);
    vtEvents.push_back(event1);
    vtEvents.push_back(event2);
    vtEvents.push_back(event3);
    OHOS::Sensors::GenerateVibrationJsonFile generateJson;
    int32_t ret = generateJson.GenerateJsonFile(vtEvents);
    EXPECT_EQ(ret, 0);
}
HWTEST_F(GenerateJsonFileTest, GenerateJsonFileTest_002, TestSize.Level1)
{
    AudioSetting audioSetting;
    audioSetting.transientDetection = 30;
    audioSetting.intensityTreshold = 30;
    audioSetting.frequencyTreshold = 50;
    audioSetting.frequencyMaxValue = 80;
    audioSetting.frequencyMinValue = 20;
    std::vector<double> data = AudioSrcDatas;
    std::vector<HapticEvent> vtEvents = {};
    VibrationConvertCore vibrationConvertCore;
    int32_t ret = vibrationConvertCore.ConvertAudioToHaptic(audioSetting, data, vtEvents);
    EXPECT_EQ(ret, 0);
}
}  // namespace Sensors
}  // namespace OHOS