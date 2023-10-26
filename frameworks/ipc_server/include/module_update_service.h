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

#include <unordered_map>
#include <unordered_set>

#include "module_update_stub.h"
#include "system_ability.h"

namespace OHOS {
namespace SysInstaller {
class ModuleUpdateService : public SystemAbility, public ModuleUpdateStub {
    DECLARE_SYSTEM_ABILITY(ModuleUpdateService);

public:
    ModuleUpdateService();
    ~ModuleUpdateService();

    int32_t InstallModulePackage(const std::string &pkgPath) override;
    int32_t UninstallModulePackage(const std::string &hmpName) override;
    int32_t GetModulePackageInfo(const std::string &hmpName,
        std::list<ModulePackageInfo> &modulePackageInfos) override;
    int32_t ReportModuleUpdateStatus(const ModuleUpdateStatus &status) override;
    int32_t ExitModuleUpdate() override;

    std::vector<HmpVersionInfo> GetHmpVersionInfo() override;
    int32_t StartUpdateHmpPackage(const std::string &path,
        const sptr<ISysInstallerCallback> &updateCallback) override;
    std::vector<HmpUpdateInfo> GetHmpUpdateResult() override;

    void ScanPreInstalledHmp();
    void OnProcessCrash(const std::string &processName);
    void OnBootCompleted();

#ifndef UPDATER_UT
protected:
#endif
    void OnStart() override;
    void OnStop() override;

private:
    int32_t InstallModuleFile(const std::string &hmpName, const std::string &file) const;
    void CollectModulePackageInfo(const std::string &hmpName, std::list<ModulePackageInfo> &modulePackageInfos) const;
    bool BackupActiveModules() const;
    bool RevertAndReboot() const;
    void OnHmpError(const std::string &hmpName);
    void ProcessSaStatus(const SaStatus &status, std::unordered_set<std::string> &hmpSet);
    bool GetHmpVersion(const std::string &hmpPath, HmpVersionInfo &versionInfo);
    void SaveInstallerResult(const std::string &hmpPath, int result, const std::string &resultInfo);
    int32_t ReallyInstallModulePackage(const std::string &pkgPath, const sptr<ISysInstallerCallback> &updateCallback);
    void ParseHmpVersionInfo(std::vector<HmpVersionInfo> &versionInfos, const HmpVersionInfo &preInfo,
        const HmpVersionInfo &actInfo);

    std::unordered_set<std::string> hmpSet_;
    std::unordered_map<int32_t, std::string> saIdHmpMap_;
    std::unordered_map<std::string, std::unordered_set<std::string>> processHmpMap_;
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_UPDATE_SERVICE_H
