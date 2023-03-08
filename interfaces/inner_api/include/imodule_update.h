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

#ifndef SYS_INSTALLER_IMODULE_UPDATE_H
#define SYS_INSTALLER_IMODULE_UPDATE_H

#include "iremote_broker.h"
#include "module_ipc_helper.h"

namespace OHOS {
namespace SysInstaller {
class IModuleUpdate : public OHOS::IRemoteBroker {
public:
    enum {
        INSTALL_MODULE_PACKAGE = 1,
        UNINSTALL_MODULE_PACKAGE,
        GET_MODULE_PACKAGE_INFO,
        REPORT_MODULE_UPDATE_STATUS,
        EXIT_MODULE_UPDATE
    };

    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.Updater.IModuleUpdate");

public:
    virtual int32_t InstallModulePackage(const std::string &pkgPath) = 0;
    virtual int32_t UninstallModulePackage(const std::string &hmpName) = 0;
    virtual int32_t GetModulePackageInfo(const std::string &hmpName,
        std::list<ModulePackageInfo> &modulePackageInfos) = 0;
    virtual int32_t ReportModuleUpdateStatus(const ModuleUpdateStatus &status) = 0;
    virtual int32_t ExitModuleUpdate() = 0;
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_IMODULE_UPDATE_H
