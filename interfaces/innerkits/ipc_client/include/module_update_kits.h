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

#ifndef SYS_INSTALLER_MODULE_UPDATE_KITS_H
#define SYS_INSTALLER_MODULE_UPDATE_KITS_H

#include "imodule_update.h"
#include "isys_installer_callback_func.h"
#include "module_ipc_helper.h"
#include "singleton.h"

namespace OHOS {
namespace SysInstaller {
class ModuleUpdateKits {
public:
    ModuleUpdateKits() = default;
    virtual ~ModuleUpdateKits() = default;
    DISALLOW_COPY_AND_MOVE(ModuleUpdateKits);

    static ModuleUpdateKits &GetInstance();

    virtual int32_t InitModuleUpdate() = 0;
    virtual int32_t InstallModulePackage(const std::string &pkgPath) = 0;
    virtual int32_t UninstallModulePackage(const std::string &hmpName) = 0;
    virtual int32_t GetModulePackageInfo(const std::string &hmpName,
        std::list<ModulePackageInfo> &modulePackageInfos) = 0;
    virtual int32_t ReportModuleUpdateStatus(const ModuleUpdateStatus &status) = 0;
    virtual int32_t ExitModuleUpdate() = 0;

    virtual std::vector<HmpVersionInfo> GetHmpVersionInfo() = 0;
    virtual int32_t StartUpdateHmpPackage(const std::string &path,
        sptr<ISysInstallerCallbackFunc> callback) = 0;
    virtual std::vector<HmpUpdateInfo> GetHmpUpdateResult() = 0;
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_UPDATE_KITS_H
