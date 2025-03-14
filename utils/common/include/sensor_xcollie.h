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

#ifndef MISCDEVICE_XCOLLIE_H
#define MISCDEVICE_XCOLLIE_H

#include <string>

#include "xcollie/xcollie.h"
#include "xcollie/xcollie_define.h"

namespace OHOS {
namespace Sensors {
const unsigned int XCOLLIE_TIMEOUT_15S = 15;
const unsigned int XCOLLIE_TIMEOUT_5S = 5;

class SensorXcollie {
public:
    SensorXcollie(const std::string& tag, unsigned int timeoutSeconds = 10,
                  std::function<void(void*)> func = nullptr, void *arg = nullptr,
                  unsigned int flag = HiviewDFX::XCOLLIE_FLAG_LOG);
    ~SensorXcollie();
private:
    int id_;
    std::string tag_;
};
} // namespace Sensors
} // namespace OHOS
#endif // MISCDEVICE_XCOLLIE_H
