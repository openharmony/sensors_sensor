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

#ifndef SECURITY_PRIVACY_MANAGER_PLUGIN_H
#define SECURITY_PRIVACY_MANAGER_PLUGIN_H

#include <string>
#include <stdio.h>
#include <stdlib.h>

#include <dlfcn.h>
#include <unistd.h>

namespace OHOS {
namespace Sensors {
enum PolicyName {
    MOTION_SENSOR,
};

enum ObjectType {
    ACCESS_TOKEN_ID,
};

enum PolicyValue {
    CLOSE,
    OPEN
};

struct AppPolicyEventExt {
    std::string objectId;
    ObjectType objectType;
    std::string bundleName;
    PolicyName policyName;
    PolicyValue policyValue = PolicyValue::OPEN;
};

#if (defined(__aarch64__) || defined(__x86_64__))
const std::string SECURITY_PRIVACY_SDK_SO_PATH = "/system/lib64/libsps_security_privacy_sdk.z.so";
#else
const std::string SECURITY_PRIVACY_SDK_SO_PATH = "/system/lib/libsps_security_privacy_sdk.z.so";
#endif

using ModifyAppPolicyPtr = int32_t (*)(int32_t systemAbilityId, int32_t userId,
    const AppPolicyEventExt &appPolicyEvent);
using QueryAppPolicyByPolicyNamePtr = int32_t (*)(int32_t systemAbilityId, int32_t userId,
    PolicyName policyName, std::vector<AppPolicyEventExt> &appPolicies);
using RegisterAppPolicyObserverPtr = int32_t (*)(int32_t systemAbilityId, int32_t userId,
    const std::function<void()> &callback);
using UnregisterAppPolicyObserverPtr = int32_t (*)(int32_t systemAbilityId, int32_t userId);
bool LoadSecurityPrivacyServer(void);
void UnloadSecurityPrivacyServer(void);
int32_t ModifyAppPolicy(int32_t userId, const AppPolicyEventExt &appPolicyEvent);
int32_t QueryAppPolicyByPolicyName(int32_t userId, PolicyName policyName, std::vector<AppPolicyEventExt> &appPolicies);
int32_t RegisterAppPolicyObserver(int32_t userId, const std::function<void()> &callback);
int32_t UnregisterAppPolicyObserver(int32_t userId);
}
}
#endif /* SECURITY_PRIVACY_MANAGER_PLUGIN_H */