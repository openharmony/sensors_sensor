/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "sensoragent_fuzzer.h"

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
        .processName = "SensorAgentFuzzTest",
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

bool CheckSensorTypeId(int32_t sensorTypeId)
{
    int32_t count = -1;
    SensorInfo *sensorInfo = nullptr;
    int32_t ret = GetAllSensors(&sensorInfo, &count);
    if (ret != 0) {
        return false;
    }
    for (int32_t i = 0; i < count; i++) {
        if ((sensorInfo + i)->sensorTypeId == sensorTypeId) {
            return true;
        }
    }
    return false;
}

void SensorAgentFuzzTest(const uint8_t *data, size_t size)
{
    SetUpTestCase();
    size_t startPos = 0;
    int32_t sensorTypeId = 0;
    GetObject<int32_t>(sensorTypeId, data + startPos, size - startPos);
    bool validSensorId = CheckSensorTypeId(sensorTypeId);
    if (!validSensorId) {
        sensorTypeId = SENSOR_TYPE_ID_ACCELEROMETER;
    }
    SensorUser user;
    user.callback = SensorDataCallbackImpl;
    SubscribeSensor(sensorTypeId, &user);
    SetBatch(sensorTypeId, &user, g_samplingInterval, g_reportInterval);
    ActivateSensor(sensorTypeId, &user);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    DeactivateSensor(sensorTypeId, &user);
    UnsubscribeSensor(sensorTypeId, &user);
}
} // namespace Sensors
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Sensors::SensorAgentFuzzTest(data, size);
    return 0;
}

