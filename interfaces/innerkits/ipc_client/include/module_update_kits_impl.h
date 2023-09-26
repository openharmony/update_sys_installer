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

#ifndef SYS_INSTALLER_MODULE_UPDATE_KITS_IMPL_H
#define SYS_INSTALLER_MODULE_UPDATE_KITS_IMPL_H

#include "imodule_update.h"
#include "module_update_kits.h"
#include "singleton.h"

namespace OHOS {
namespace SysInstaller {
class ModuleUpdateKitsImpl final : public ModuleUpdateKits,
    public DelayedRefSingleton<ModuleUpdateKitsImpl> {
    DECLARE_DELAYED_REF_SINGLETON(ModuleUpdateKitsImpl);
public:
    DISALLOW_COPY_AND_MOVE(ModuleUpdateKitsImpl);

    int32_t InitModuleUpdate() final;
    int32_t InstallModulePackage(const std::string &pkgPath) final;
    int32_t UninstallModulePackage(const std::string &hmpName) final;
    int32_t GetModulePackageInfo(const std::string &hmpName,
        std::list<ModulePackageInfo> &modulePackageInfos) final;
    int32_t ReportModuleUpdateStatus(const ModuleUpdateStatus &status) final;
    int32_t ExitModuleUpdate() final;

    std::vector<HmpVersionInfo> GetHmpVersionInfo() final;
    int32_t StartUpdateHmpPackage(const std::string &path,
        sptr<ISysInstallerCallbackFunc> callback) final;
    std::vector<HmpUpdateInfo> GetHmpUpdateResult() final;

#ifndef UPDATER_UT
private:
#endif
    // For death event procession
    class DeathRecipient final : public IRemoteObject::DeathRecipient {
    public:
        DeathRecipient() = default;
        ~DeathRecipient() final = default;
        DISALLOW_COPY_AND_MOVE(DeathRecipient);
        void OnRemoteDied(const wptr<IRemoteObject> &remote) final;
    };

    void ResetService(const wptr<IRemoteObject> &remote);
    sptr<IModuleUpdate> GetService();

    std::mutex moduleUpdateLock_;
    sptr<IModuleUpdate> moduleUpdate_ {};
    sptr<IRemoteObject::DeathRecipient> deathRecipient_ {};
    sptr<ISysInstallerCallback> updateCallBack_ {};
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_UPDATE_KITS_IMPL_H
