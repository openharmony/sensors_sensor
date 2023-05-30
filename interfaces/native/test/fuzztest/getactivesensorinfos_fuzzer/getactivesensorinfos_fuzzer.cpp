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

#include "getactivesensorinfos_fuzzer.h"

#include "accesstoken_kit.h"
#include "securec.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"

#include "sensor_agent.h"
#include "sensor_agent_type.h"

namespace OHOS {
namespace Sensors {
using namespace Security::AccessToken;
using Security::AccessToken::AccessTokenID;
namespace {
constexpr size_t DATA_MIN_SIZE = 4;
} // namespace

template<class T>
size_t GetObject(const uint8_t *data, size_t size, T &object)
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
    const char **perms = new const char *[1];
    perms[0] = "ohos.permission.ACCELEROMETER";
    TokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 1,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "SensorPowerTest",
        .aplStr = "system_core",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    AccessTokenKit::ReloadNativeTokenInfo();
    delete[] perms;
}

void GetActiveSensorInfosFuzzTest(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < DATA_MIN_SIZE) {
        return;
    }
    SetUpTestCase();
    size_t startPos = 0;
    int32_t pid {-1};
    GetObject<int32_t>(data + startPos, size - startPos, pid);
    SensorActiveInfo *sensorActiveInfos {nullptr};
    int32_t count { 0 };
    GetActiveSensorInfos(pid, &sensorActiveInfos, &count);
}
} // Sensors
} // OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    OHOS::Sensors::GetActiveSensorInfosFuzzTest(data, size);
    return 0;
}
