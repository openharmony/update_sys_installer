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
#include "sys_installer_load_callback.h"

#include "sys_installer_kits_impl.h"
#include "log/log.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

void SysInstallerLoadCallback::OnLoadSystemAbilitySuccess(int32_t systemAbilityId,
    const sptr<IRemoteObject> &remoteObject)
{
    LOG(INFO) << "OnLoadSystemAbilitySuccess systemAbilityId: " << systemAbilityId << " IRmoteObject result:" <<
        ((remoteObject != nullptr) ? "true" : "false");
    if (systemAbilityId != SYS_INSTALLER_DISTRIBUTED_SERVICE_ID) {
        LOG(ERROR) << "start aystemabilityId is not sinkSAId!";
        return;
    }
    if (remoteObject == nullptr) {
        LOG(ERROR) << "remoteObject is null.";
        return;
    }
    SysInstallerKitsImpl::GetInstance().LoadServiceSuccess();
}

void SysInstallerLoadCallback::OnLoadSystemAbilityFail(int32_t systemAbilityId)
{
    LOG(INFO) << "OnLoadSystemAbilityFail systemAbilityId:" << systemAbilityId;
    if (systemAbilityId != SYS_INSTALLER_DISTRIBUTED_SERVICE_ID) {
        LOG(ERROR) << "start aystemabilityId is not sinkSAId!";
        return;
    }
    SysInstallerKitsImpl::GetInstance().LoadServiceFail();
}
} // SysInstaller
} // OHOS