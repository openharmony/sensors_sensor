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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "sensor_data_manager.h"
#include "sensor_errors.h"
#include "sensor_utils.h"

#undef LOG_TAG
#define LOG_TAG "SensorDataManager"

namespace OHOS {
namespace Sensors {
using namespace testing;
using namespace testing::ext;
using namespace OHOS::HiviewDFX;

namespace {
static sptr<IRemoteObject> g_remote = new (std::nothrow) IPCObjectStub();
constexpr int32_t APP_LIST_SIZE_ZERO = 0;
} // namespace

class SensorDataManagerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SensorDataManagerTest::SetUpTestCase() {}

void SensorDataManagerTest::TearDownTestCase() {}

void SensorDataManagerTest::SetUp() {}

void SensorDataManagerTest::TearDown() {}

HWTEST_F(SensorDataManagerTest, InitTest_001, TestSize.Level1)
{
    SEN_HILOGI("InitTest_001 in");
    int32_t deviceMode = SINGLE_DISPLAY_THREE_FOLD;
    bool ret = SENSOR_DATA_MGR->Init(deviceMode);
    SEN_HILOGI("ret:%{public}s", ret ? "true" : "false");
    EXPECT_EQ(ret, true);
}

HWTEST_F(SensorDataManagerTest, GetCompatibleAppStrategyListTest_001, TestSize.Level1)
{
    SEN_HILOGI("GetCompatibleAppStrategyListTest_001 in");
    std::vector<CompatibleAppData> appList = SENSOR_DATA_MGR->GetCompatibleAppStrategyList();
    SEN_HILOGI("appList size:%{public}zu", appList.size());
    EXPECT_EQ(appList.size(), APP_LIST_SIZE_ZERO);
}

HWTEST_F(SensorDataManagerTest, ExecRegisterCbTest_001, TestSize.Level1)
{
    SEN_HILOGI("ExecRegisterCbTest_001 in");
    sptr<SensorObserver> observer = nullptr;
    EXPECT_NO_FATAL_FAILURE(SENSOR_DATA_MGR->ExecRegisterCb(observer));
}

HWTEST_F(SensorDataManagerTest, ExecRegisterCbTest_002, TestSize.Level1)
{
    SEN_HILOGI("ExecRegisterCbTest_002 in");
    SensorObserver::UpdateFunc updateFunc = [&]() {
    };
    sptr<SensorObserver> observer = new SensorObserver();
    observer->SetUpdateFunc(updateFunc);
    EXPECT_NO_FATAL_FAILURE(SENSOR_DATA_MGR->ExecRegisterCb(observer));
}

HWTEST_F(SensorDataManagerTest, CreateObserverTest_001, TestSize.Level1)
{
    SEN_HILOGI("CreateObserverTest_001 in");
    SensorObserver::UpdateFunc updateFunc = [&]() {
    };
    auto observer = SENSOR_DATA_MGR->CreateObserver(updateFunc);
    EXPECT_NE(observer, nullptr);
}

HWTEST_F(SensorDataManagerTest, ParseCompatibleAppStrategyListTest_001, TestSize.Level1)
{
    SEN_HILOGI("ParseCompatibleAppStrategyListTest_001 in");
    std::string jsonStr = R"({
        "com.test.app1": {
            "name": "com.test.app4",
            "mode": 8,
            "exemptNaturalDirectionCorrect": true
        },
        "com.test.app2": {
            "name": "com.test.app5",
            "mode": 6,
            "exemptNaturalDirectionCorrect": true
        },
        "com.test.app3": {
            "name": "com.test.app6",
            "mode": 8,
            "exemptNaturalDirectionCorrect": false
        }
    })";
    SENSOR_DATA_MGR->ParseCompatibleAppStrategyList(jsonStr);
    EXPECT_NE(SENSOR_DATA_MGR->compatibleAppStrategyList_.size(), APP_LIST_SIZE_ZERO);
}

HWTEST_F(SensorDataManagerTest, ParseCompatibleAppStrategyListTest_002, TestSize.Level1)
{
    SEN_HILOGI("ParseCompatibleAppStrategyListTest_002 in");
    std::string jsonStr = "xxx";
    SENSOR_DATA_MGR->ParseCompatibleAppStrategyList(jsonStr);
    EXPECT_EQ(SENSOR_DATA_MGR->compatibleAppStrategyList_.size(), APP_LIST_SIZE_ZERO);
}

HWTEST_F(SensorDataManagerTest, CreateDataShareHelperTest_001, TestSize.Level1)
{
    SEN_HILOGI("CreateDataShareHelperTest_001 in");
    SENSOR_DATA_MGR->remoteObj_ = nullptr;
    std::string uriProxy = "datashare:///com.ohos.settingsdata/entry/settingsdata/SETTINGSDATA?Proxy=true";
    auto helper = SENSOR_DATA_MGR->CreateDataShareHelper(uriProxy);
    EXPECT_EQ(helper, nullptr);
}

HWTEST_F(SensorDataManagerTest, CreateDataShareHelperTest_002, TestSize.Level1)
{
    SEN_HILOGI("CreateDataShareHelperTest_002 in");
    SENSOR_DATA_MGR->Init(SINGLE_DISPLAY_THREE_FOLD);
    std::string uriProxy = "datashare:///com.ohos.settingsdata/entry/settingsdata/SETTINGSDATA?Proxy=true";
    auto helper = SENSOR_DATA_MGR->CreateDataShareHelper(uriProxy);
    EXPECT_NE(helper, nullptr);
}

HWTEST_F(SensorDataManagerTest, ReleaseDataShareHelperTest_001, TestSize.Level1)
{
    SEN_HILOGI("ReleaseDataShareHelperTest_001 in");
    std::shared_ptr<DataShare::DataShareHelper> helper;
    bool ret = SENSOR_DATA_MGR->ReleaseDataShareHelper(helper);
    EXPECT_EQ(ret, false);
}

HWTEST_F(SensorDataManagerTest, ReleaseDataShareHelperTest_002, TestSize.Level1)
{
    SEN_HILOGI("ReleaseDataShareHelperTest_002 in");
    SENSOR_DATA_MGR->Init(SINGLE_DISPLAY_THREE_FOLD);
    std::string uriProxy = "datashare:///com.ohos.settingsdata/entry/settingsdata/SETTINGSDATA?Proxy=true";
    auto helper = SENSOR_DATA_MGR->CreateDataShareHelper(uriProxy);
    bool ret =  SENSOR_DATA_MGR->ReleaseDataShareHelper(helper);
    EXPECT_NE(ret, false);
}

HWTEST_F(SensorDataManagerTest, RegisterObserverTest_001, TestSize.Level1)
{
    SEN_HILOGI("RegisterObserverTest_001 in");
    sptr<SensorObserver> observer = nullptr;
    auto ret = SENSOR_DATA_MGR->RegisterObserver(observer);
    EXPECT_NE(ret, ERR_OK);
}

HWTEST_F(SensorDataManagerTest, RegisterObserverTest_002, TestSize.Level1)
{
    SEN_HILOGI("RegisterObserverTest_002 in");
    sptr<SensorObserver> observer = new SensorObserver();
    auto ret = SENSOR_DATA_MGR->RegisterObserver(observer);
    EXPECT_EQ(ret, ERR_OK);
}

HWTEST_F(SensorDataManagerTest, GetStringValueTest_001, TestSize.Level1)
{
    SEN_HILOGI("GetStringValueTest_001 in");
    std::string testUri = "test";
    std::string compatibleAppStrategy;
    auto ret = SENSOR_DATA_MGR->GetStringValue(testUri, compatibleAppStrategy);
    EXPECT_NE(ret, ERR_OK);
}

HWTEST_F(SensorDataManagerTest, UnregisterObserverTest_001, TestSize.Level1)
{
    SEN_HILOGI("UnregisterObserverTest_001 in");
    sptr<SensorObserver> observer = new SensorObserver();
    auto ret = SENSOR_DATA_MGR->UnregisterObserver(observer);
    EXPECT_EQ(ret, ERR_OK);
}

HWTEST_F(SensorDataManagerTest, UnregisterObserverTest_002, TestSize.Level1)
{
    SEN_HILOGI("UnregisterObserverTest_002 in");
    sptr<SensorObserver> observer = nullptr;
    auto ret = SENSOR_DATA_MGR->UnregisterObserver(observer);
    EXPECT_NE(ret, ERR_OK);
}

HWTEST_F(SensorDataManagerTest, AssembleUriTest_001, TestSize.Level1)
{
    SEN_HILOGI("AssembleUriTest_001 in");
    std::string uriStr = "test";
    EXPECT_NO_FATAL_FAILURE(SENSOR_DATA_MGR->AssembleUri(uriStr));
}

HWTEST_F(SensorDataManagerTest, ParseAppLogicalDeviceListTest_001, TestSize.Level1)
{
    SEN_HILOGI("ParseAppLogicalDeviceListTest_001 in");
    std::string jsonStr = R"({
        "com.test.app1": {
            "name": "com.test.app4",
            "useLogicCamera": {
                "1": 1,
                "2": 0
            },
            "customLogicDirection": {
                "1": 3
            },
            "cameraPageDisable": {
                "1": 1
            },
            "cameraPageMode": {
                "1": 2
            },
            "xcomponentCompensationAngle": {
                "1": 90,
                "2": 0
            }
        }
    })";
    SENSOR_DATA_MGR->ParseAppLogicalDeviceList(jsonStr);
    EXPECT_NE(SENSOR_DATA_MGR->compatibleAppStrategyList_.size(), APP_LIST_SIZE_ZERO);
}
} // namespace Sensors
} // namespace OHOS