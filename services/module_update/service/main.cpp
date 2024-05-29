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

#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "log/log.h"
#include "module_update_kits.h"
#include "module_update_kits_impl.h"

using namespace OHOS;
using namespace Updater;

int main()
{
    InitUpdaterLogger("ModuleUpdaterClient", "", "", "");
    sptr<ISystemAbilityManager> samgr = nullptr;
    int32_t times = 30;
    const int32_t wiat_time_ms = 100 * 1000;
    int32_t duration = std::chrono::microseconds(wiat_time_ms).count();
    LOG(INFO) << "waiting for samgr...";
    while (times > 0) {
        times--;
        samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (samgr == nullptr) {
            usleep(duration);
        } else {
            break;
        }
    }

    if (samgr == nullptr) {
        LOG(ERROR) <<"ModuleUpdateInit wait samgr fail";
        return -1;
    }

    int32_t ret = OHOS::SysInstaller::ModuleUpdateKits::GetInstance().InitModuleUpdate();
    LOG(INFO) << "ModuleUpdateInit ret: " << ret;
    return ret;
}