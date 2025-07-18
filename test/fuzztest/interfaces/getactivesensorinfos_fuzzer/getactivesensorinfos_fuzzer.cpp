/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#include "nativetoken_kit.h"
#include "securec.h"
#include "token_setproc.h"

#include "sensor_agent.h"
#include "sensor_agent_type.h"
#include "sensor_errors.h"
#include "sensor.h"
#include "sensor_service_client.h"

#undef LOG_TAG
#define LOG_TAG "GetActiveSensorInfosFuzzTest"
#define SEN_CLIENT SensorServiceClient::GetInstance()

namespace OHOS {
namespace Sensors {
using namespace OHOS::HiviewDFX;
using namespace Security::AccessToken;
using Security::AccessToken::AccessTokenID;
namespace {
constexpr size_t DATA_MIN_SIZE = 4;
} // namespace

template<class T>
void GetObject(const uint8_t *data, size_t size, T &object)
{
    size_t objectSize = sizeof(object);
    if (objectSize > size) {
        return;
    }
    memcpy_s(&object, objectSize, data, objectSize);
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
        .processName = "GetActiveSensorInfosFuzzTest",
        .aplStr = "system_core",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    AccessTokenKit::ReloadNativeTokenInfo();
    delete[] perms;
}

void GetActiveSensorInfosFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < DATA_MIN_SIZE) {
        return;
    }
    SetUpTestCase();
    int32_t pid { 0 };
    GetObject<int32_t>(data, size, pid);
    SensorActiveInfo *sensorActiveInfos { nullptr };
    int32_t count { 0 };
    GetActiveSensorInfos(pid, &sensorActiveInfos, &count);
    std::vector<ActiveInfo> sensorActiveInfoList;
    SEN_CLIENT.GetActiveInfoList(pid, sensorActiveInfoList);
}
} // namespace Sensors
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Sensors::GetActiveSensorInfosFuzzTest(data, size);
    return 0;
}
