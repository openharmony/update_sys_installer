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

#include "sys_event_service_listener.h"

#include "log/log.h"
#include "module_update_main.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

void SysEventServiceListener::OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    if (systemAbilityId == DFX_SYS_EVENT_SERVICE_ABILITY_ID) {
        LOG(INFO) << "OnAddSystemAbility SysEventServiceAbility";
        ModuleUpdateMain::GetInstance().RegisterSysEventListener();
    }
}

void SysEventServiceListener::OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    if (systemAbilityId == DFX_SYS_EVENT_SERVICE_ABILITY_ID) {
        LOG(INFO) << "OnRemoveSystemAbility SysEventServiceAbility";
        ModuleUpdateMain::GetInstance().OnSysEventServiceDied();
    }
}
} // namespace SysInstaller
} // namespace OHOS