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

#include "installer_manager.h"

#include "log/log.h"
#include "package/pkg_manager.h"
#include "utils.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

void InstallerManager::RegisterDump(std::unique_ptr<IInstallerManagerHelper> ptr)
{
    helper_ = std::move(ptr);
}

InstallerManager &InstallerManager::GetInstance()
{
    static InstallerManager instance;
    return instance;
}

int32_t InstallerManager::SysInstallerInit()
{
    SysInstallerManagerInit::GetInstance().InvokeEvent(SYS_PRE_INIT_EVENT);

    if (helper_ == nullptr) {
        RegisterDump(std::make_unique<InstallerManagerHelper>());
    }
    return helper_->SysInstallerInit();
}

int32_t InstallerManager::StartUpdatePackageZip(const std::string &pkgPath)
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->StartUpdatePackageZip(pkgPath);
}

int32_t InstallerManager::SetUpdateCallback(const sptr<ISysInstallerCallback> &updateCallback)
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->SetUpdateCallback(updateCallback);
}

int32_t InstallerManager::GetUpdateStatus()
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->GetUpdateStatus();
}

int32_t InstallerManager::StartUpdateParaZip(const std::string &pkgPath,
    const std::string &location, const std::string &cfgDir)
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->StartUpdateParaZip(pkgPath, location, cfgDir);
}
} // namespace SysInstaller
} // namespace OHOS
