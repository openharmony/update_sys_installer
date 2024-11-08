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

#ifndef SYS_INSTALLER_PROXY_H
#define SYS_INSTALLER_PROXY_H

#include "iremote_proxy.h"
#include "isys_installer.h"

namespace OHOS {
namespace SysInstaller {
class SysInstallerProxy : public IRemoteProxy<ISysInstaller> {
public:
    explicit SysInstallerProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<ISysInstaller>(impl) {}

    virtual int32_t SysInstallerInit();
    virtual int32_t StartUpdatePackageZip(const std::string &pkgPath);
    virtual int32_t SetUpdateCallback(const sptr<ISysInstallerCallback> &updateCallback);
    virtual int32_t GetUpdateStatus();
    virtual int32_t StartUpdateParaZip(const std::string &pkgPath,
        const std::string &location, const std::string &cfgDir);
    virtual int32_t StartDeleteParaZip(const std::string &location, const std::string &cfgDir);
    virtual int32_t AccDecompressAndVerifyPkg(const std::string &srcPath,
        const std::string &dstPath, const uint32_t type);
    virtual int32_t AccDeleteDir(const std::string &dstPath);
private:
    static inline BrokerDelegator<SysInstallerProxy> delegator_;
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_PROXY_H
