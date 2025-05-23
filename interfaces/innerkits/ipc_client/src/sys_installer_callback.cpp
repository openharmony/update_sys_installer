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

#include "sys_installer_callback.h"

#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "log/log.h"
#include "securec.h"
#include "utils.h"

#include "isys_installer_callback.h"
#include "sys_installer_common.h"
#include "sys_installer_sa_ipc_interface_code.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;
SysInstallerCallback::SysInstallerCallback(sptr<ISysInstallerCallbackFunc> callback)
{
    callback_ = callback;
}

ErrCode SysInstallerCallback::OnUpgradeProgress(UpdateStatus updateStatus, int percent,
    const std::string &resultMsg)
{
    LOG(INFO) << "updateStatus:" << static_cast<int>(updateStatus) << " percent:" << percent << " msg:" << resultMsg;
    if (callback_ != nullptr) {
        callback_->OnUpgradeProgress(updateStatus, percent, resultMsg);
    }
    return 0;
}

ErrCode SysInstallerCallback::OnUpgradeDealLen(UpdateStatus updateStatus, int dealLen,
    const std::string &resultMsg)
{
    LOG(INFO) << "updateStatus: " << static_cast<int>(updateStatus) << " dealLen:" << dealLen << " msg:" << resultMsg;
    if (callback_ != nullptr) {
        callback_->OnUpgradeDealLen(updateStatus, dealLen, resultMsg);
    }
    return 0;
}
}
} // namespace OHOS
