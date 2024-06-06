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
#ifndef SYS_INSTALLER_MODULE_UPDATE_SERVICE_H
#define SYS_INSTALLER_MODULE_UPDATE_SERVICE_H

#include "module_update_stub.h"
#include "system_ability.h"

namespace OHOS {
namespace SysInstaller {
class ModuleUpdateService : public SystemAbility, public ModuleUpdateStub {
    DECLARE_SYSTEM_ABILITY(ModuleUpdateService);

public:
    ModuleUpdateService(int32_t systemAbilityId, bool runOnCreate = false);
    ~ModuleUpdateService() override;

    int32_t InstallModulePackage(const std::string &pkgPath) override;
    int32_t UninstallModulePackage(const std::string &hmpName) override;
    int32_t GetModulePackageInfo(const std::string &hmpName,
        std::list<ModulePackageInfo> &modulePackageInfos) override;
    int32_t ExitModuleUpdate() override;

    std::vector<HmpVersionInfo> GetHmpVersionInfo() override;
    int32_t StartUpdateHmpPackage(const std::string &path,
        const sptr<ISysInstallerCallback> &updateCallback) override;
    std::vector<HmpUpdateInfo> GetHmpUpdateResult() override;

#ifndef UPDATER_UT
protected:
#endif
    void OnStart(const SystemAbilityOnDemandReason &startReason) override;
    void OnStop(const SystemAbilityOnDemandReason &stopReason) override;
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_UPDATE_SERVICE_H
