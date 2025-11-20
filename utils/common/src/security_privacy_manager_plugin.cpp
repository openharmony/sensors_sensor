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

#include "security_privacy_manager_plugin.h"

#include "sensor_errors.h"
#include "sensor_log.h"

#undef LOG_TAG
#define LOG_TAG "SecurityPrivacyManagerPlugin"

namespace OHOS {
namespace Sensors {
namespace {
    const int32_t SENSOR_SYSTEM_ABILITY_ID = 3601;
    constexpr uint32_t SLEEP_TIME_US = 10000;
}

static void *g_handle = nullptr;

bool LoadSecurityPrivacyServer(void)
{
    SEN_HILOGI("Load security privacy manager plugin in");
    if (g_handle != nullptr) {
        SEN_HILOGW("Security privacy manager plugin has already exits");
        return true;
    }
    int32_t cnt = 0;
    int32_t retryTimes = 3;
    do {
        cnt++;
        g_handle = dlopen(SECURITY_PRIVACY_SDK_SO_PATH.c_str(), RTLD_LAZY);
        SEN_HILOGI("dlopen %{public}s, retry cnt: %{public}d", SECURITY_PRIVACY_SDK_SO_PATH.c_str(), cnt);
        usleep(SLEEP_TIME_US);
    } while (!g_handle && cnt < retryTimes);
    return (g_handle != nullptr);
}

void UnloadSecurityPrivacyServer(void)
{
    SEN_HILOGI("Unload security privacy manager plugin in");
    if (g_handle != nullptr) {
        dlclose(g_handle);
        g_handle = nullptr;
    }
}

__attribute__((no_sanitize("cfi"))) int32_t ModifyAppPolicy(int32_t userId,
    const AppPolicyEventExt &appPolicyEvent)
{
    if (g_handle == nullptr) {
        SEN_HILOGD("g_handle is nullptr");
        return ERROR;
    }
    ModifyAppPolicyPtr func = (ModifyAppPolicyPtr)(dlsym(g_handle, "ModifyAppPolicy"));
    const char* dlsymError = dlerror();
    if  (dlsymError) {
        SEN_HILOGE("dlsym error: %{public}s", dlsymError);
        return ERROR;
    }
    if (func == nullptr) {
        SEN_HILOGE("func is nullptr");
        return ERROR;
    }
    return func(SENSOR_SYSTEM_ABILITY_ID, userId, appPolicyEvent);
}

__attribute__((no_sanitize("cfi"))) int32_t QueryAppPolicyByPolicyName(int32_t userId,
    PolicyName policyName, std::vector<AppPolicyEventExt> &appPolicies)
{
    if (g_handle == nullptr) {
        SEN_HILOGD("g_handle is nullptr");
        return ERROR;
    }
    QueryAppPolicyByPolicyNamePtr func = (QueryAppPolicyByPolicyNamePtr)(dlsym(g_handle,
        "QueryAppPolicyByPolicyName"));
    const char* dlsymError = dlerror();
    if  (dlsymError) {
        SEN_HILOGE("dlsym error: %{public}s", dlsymError);
        return ERROR;
    }
    if (func == nullptr) {
        SEN_HILOGE("func is nullptr");
        return ERROR;
    }
    return func(SENSOR_SYSTEM_ABILITY_ID, userId, policyName, appPolicies);
}

__attribute__((no_sanitize("cfi"))) int32_t RegisterAppPolicyObserver(int32_t userId,
    const std::function<void()> &callback)
{
    if (g_handle == nullptr) {
        SEN_HILOGD("g_handle is nullptr");
        return ERROR;
    }
    RegisterAppPolicyObserverPtr func = (RegisterAppPolicyObserverPtr)(dlsym(g_handle,
        "RegisterAppPolicyObserver"));
    const char* dlsymError = dlerror();
    if  (dlsymError) {
        SEN_HILOGE("dlsym error: %{public}s", dlsymError);
        return ERROR;
    }
    if (func == nullptr) {
        SEN_HILOGE("func is nullptr");
        return ERROR;
    }
    return func(SENSOR_SYSTEM_ABILITY_ID, userId, callback);
}

__attribute__((no_sanitize("cfi"))) int32_t UnregisterAppPolicyObserver(int32_t userId)
{
    if (g_handle == nullptr) {
        SEN_HILOGD("g_handle is nullptr");
        return ERROR;
    }
    UnregisterAppPolicyObserverPtr func = (UnregisterAppPolicyObserverPtr)(dlsym(g_handle,
        "UnregisterAppPolicyObserver"));
    const char* dlsymError = dlerror();
    if  (dlsymError) {
        SEN_HILOGE("dlsym error: %{public}s", dlsymError);
        return ERROR;
    }
    if (func == nullptr) {
        SEN_HILOGE("func is nullptr");
        return ERROR;
    }
    return func(SENSOR_SYSTEM_ABILITY_ID, userId);
}

__attribute__((no_sanitize("cfi"))) int32_t CreateAppPolicyDB(int32_t userId)
{
    if (g_handle == nullptr) {
        SEN_HILOGD("g_handle is nullptr");
        return ERROR;
    }
    CreateAppPolicyDBPtr func = (CreateAppPolicyDBPtr)(dlsym(g_handle,
        "CreateAppPolicyDB"));
    const char* dlsymError = dlerror();
    if  (dlsymError) {
        SEN_HILOGE("dlsym error: %{public}s", dlsymError);
        return ERROR;
    }
    if (func == nullptr) {
        SEN_HILOGE("func is nullptr");
        return ERROR;
    }
    return func(userId);
}
}
}