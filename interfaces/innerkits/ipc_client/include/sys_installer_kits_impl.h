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

#ifndef SYS_INSTALLER_KITS_IMPL_H
#define SYS_INSTALLER_KITS_IMPL_H

#include "singleton.h"
#include "isys_installer.h"
#include "isys_installer_callback.h"
#include "isys_installer_callback_func.h"

namespace OHOS {
namespace SysInstaller {
class SysInstallerKitsImpl {
public:
    DISALLOW_COPY_AND_MOVE(SysInstallerKitsImpl);
    static SysInstallerKitsImpl &GetInstance();
    virtual int32_t SysInstallerInit();
    virtual int32_t StartUpdatePackageZip(const std::string &pkgPath);
    virtual int32_t SetUpdateCallback(sptr<ISysInstallerCallbackFunc> callback);
    virtual int32_t GetUpdateStatus();
    virtual int32_t StartUpdateParaZip(const std::string &pkgPath,
        const std::string &location, const std::string &cfgDir);
    virtual int32_t StartDeleteParaZip(const std::string &location, const std::string &cfgDir);
    virtual int32_t AccDecompressAndVerifyPkg(const std::string &srcPath,
        const std::string &dstPath, const uint32_t type);
    virtual int32_t AccDeleteDir(const std::string &dstPath);
    virtual void ResetService(const wptr<IRemoteObject> &remote);
    sptr<ISysInstaller> GetService();

    void LoadServiceSuccess();
    void LoadServiceFail();

private:
    int32_t Init();
    SysInstallerKitsImpl() = default;
    virtual ~SysInstallerKitsImpl() = default;

#ifndef UPDATER_UT
private:
#else
public:
#endif
    // For death event procession
    class DeathRecipient final : public IRemoteObject::DeathRecipient {
    public:
        DeathRecipient() = default;
        ~DeathRecipient() final = default;
        DISALLOW_COPY_AND_MOVE(DeathRecipient);
        virtual void OnRemoteDied(const wptr<IRemoteObject> &remote);
    };
    std::mutex sysInstallerLock_;
    sptr<ISysInstaller> sysInstaller_ {};
    sptr<IRemoteObject::DeathRecipient> deathRecipient_ {};
    sptr<ISysInstallerCallback> updateCallBack_ {};

    std::mutex serviceMutex_;
    std::condition_variable serviceCv_;
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_KITS_IMPL_H
