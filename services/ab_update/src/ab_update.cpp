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

#include "ab_update.h"

#include "log/log.h"
#include "package/package.h"
#include "package/pkg_manager.h"
#include "scope_guard.h"
#include "utils.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

UpdaterStatus ABUpdate::StartABUpdate(const std::string &pkgPath)
{
    LOG(INFO) << "StartABUpdate start";
    if (statusManager_ == nullptr) {
        LOG(ERROR) << "statusManager_ nullptr";
        return UPDATE_ERROR;
    }

    Hpackage::PkgManager::PkgManagerPtr pkgManager = Hpackage::PkgManager::GetPackageInstance();
    if (pkgManager == nullptr) {
        LOG(ERROR) << "pkgManager is nullptr";
        return UPDATE_ERROR;
    }

    STAGE(UPDATE_STAGE_BEGIN) << "StartABUpdate start";
    LOG(INFO) << "ABUpdate start, pkg updaterPath : " << pkgPath.c_str();

    UpdaterParams upParams;
    upParams.updatePackage = {pkgPath};
    upParams.CallbackProgress = std::bind(&ABUpdate::SetProgress, this, std::placeholders::_1);
    UpdaterStatus updateRet = DoInstallUpdaterPackage(pkgManager, upParams, HOTA_UPDATE);
    if (updateRet != UPDATE_SUCCESS) {
        LOG(INFO) << "Install package failed!";
        STAGE(UPDATE_STAGE_FAIL) << "Install package failed";
        Hpackage::PkgManager::ReleasePackageInstance(pkgManager);
        return updateRet;
    }
    LOG(INFO) << "Install package successfully!";
    STAGE(UPDATE_STAGE_SUCCESS) << "Install package success";
    Hpackage::PkgManager::ReleasePackageInstance(pkgManager);
    if (!DeleteUpdaterPath(GetWorkPath())) {
        LOG(WARNING) << "Delete Updater Path fail.";
    }
    return updateRet;
}

void ABUpdate::PerformAction()
{
    InstallerErrCode errCode = SYS_UPDATE_SUCCESS;
    std::string errStr = "";
    UpdaterStatus updateRet = UpdaterStatus::UPDATE_SUCCESS;
    Detail::ScopeGuard guard([&] {
        LOG(INFO) << "PerformAction ret:" << updateRet;
        if (updateRet != UpdaterStatus::UPDATE_SUCCESS) {
            errCode = SYS_INSTALL_PARA_FAIL;
            errStr = std::to_string(updateRet);
        }
        if (actionCallBack_ != nullptr) {
            actionCallBack_(errCode, errStr);
        }
    });

    updateRet = StartABUpdate(pkgPath_);
}

void ABUpdate::SetProgress(float value)
{
    statusManager_->SetUpdatePercent(static_cast<int>(value));
}
} // namespace SysInstaller
} // namespace OHOS
