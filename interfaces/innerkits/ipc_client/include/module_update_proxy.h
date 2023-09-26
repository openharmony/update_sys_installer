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

#ifndef SYS_INSTALLER_MODULE_UPDATE_PROXY_H
#define SYS_INSTALLER_MODULE_UPDATE_PROXY_H

#include "imodule_update.h"
#include "iremote_proxy.h"

namespace OHOS {
namespace SysInstaller {
class ModuleUpdateProxy : public IRemoteProxy<IModuleUpdate> {
public:
    explicit ModuleUpdateProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<IModuleUpdate>(impl) {}

    virtual int32_t InstallModulePackage(const std::string &pkgPath);
    virtual int32_t UninstallModulePackage(const std::string &hmpName);
    virtual int32_t GetModulePackageInfo(const std::string &hmpName,
        std::list<ModulePackageInfo> &modulePackageInfos);
    virtual int32_t ReportModuleUpdateStatus(const ModuleUpdateStatus &status);
    virtual int32_t ExitModuleUpdate();

    virtual std::vector<HmpVersionInfo> GetHmpVersionInfo();
    virtual int32_t StartUpdateHmpPackage(const std::string &path,
        const sptr<ISysInstallerCallback> &updateCallback);
    virtual std::vector<HmpUpdateInfo> GetHmpUpdateResult();
private:
    static inline BrokerDelegator<ModuleUpdateProxy> delegator_;
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_UPDATE_PROXY_H
