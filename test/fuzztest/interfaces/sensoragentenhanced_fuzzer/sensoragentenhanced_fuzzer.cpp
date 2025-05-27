/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "sensoragentenhanced_fuzzer.h"

#include <unistd.h>
#include <thread>

#include "accesstoken_kit.h"
#include "token_setproc.h"
#include "nativetoken_kit.h"
#include "securec.h"

#include "sensor_agent.h"
#include "sensor_agent_type.h"
#include "sensor_errors.h"

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
using namespace OHOS::Security::AccessToken;
using OHOS::Security::AccessToken::AccessTokenID;
namespace {
constexpr int64_t g_samplingInterval = 200000000;
constexpr int64_t g_reportInterval = 200000000;
constexpr size_t g_dataMinSize = sizeof(int32_t) + sizeof(SensorDescription) + sizeof(SensorUser);
constexpr int32_t g_sleepTime = 1000;
} // namespace

template<class T>
size_t GetObject(T &object, const uint8_t *data, size_t size)
{
    size_t objectSize = sizeof(object);
    if (objectSize > size) {
        return 0;
    }
    errno_t ret = memcpy_s(&object, objectSize, data, objectSize);
    if (ret != EOK) {
        return 0;
    }
    return objectSize;
}

void SetUpTestCase()
{
    const char **perms = new (std::nothrow) const char *[2];
    CHKPV(perms);
    perms[0] = "ohos.permission.ACCELEROMETER";
    perms[1] = "ohos.permission.MANAGE_SENSOR";
    TokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 2,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "SensorAgentEnhancedFuzzTest",
        .aplStr = "system_core",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    AccessTokenKit::ReloadNativeTokenInfo();
    delete[] perms;
}

void SensorDataCallbackImpl(SensorEvent *event)
{
    if (event == nullptr) {
        return;
    }
}

void SensorAgentEnhancedFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < DATA_MIN_SIZE) {
        return;
    }
    SetUpTestCase();
    size_t startPos = 0;
    int32_t deviceId = 0;
    GetObject<int32_t>(deviceId, data + startPos, size - startPos);

    SensorDescription sensorDesc;
    GetObject<SensorDescription>(sensorDesc, data + startPos, size - startPos);

    SensorUser user;
    user.callback = SensorDataCallbackImpl;

    SensorInfo *sensorInfo = nullptr;
    int32_t count = 0;
    GetDeviceSensors(deviceId, &sensorInfo, &count);

    SubscribeSensorEnhanced(sensorDesc, &user);
    SetBatchEnhanced(sensorDesc, &user, g_samplingInterval, g_reportInterval);
    ActivateSensorEnhanced(sensorDesc, &user);

    std::this_thread::sleep_for(std::chrono::milliseconds(g_sleepTime));

    DeactivateSensorEnhanced(sensorDesc, &user);
    UnsubscribeSensorEnhanced(sensorDesc, &user);

    SubscribeSensorPlug(&user);
    UnsubscribeSensorPlug(&user);
}
} // namespace Sensors
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Sensors::SensorAgentEnhancedFuzzTest(data, size);
    return 0;
}