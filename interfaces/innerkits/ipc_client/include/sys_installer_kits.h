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

#ifndef SYS_INSTALLER_KITS_H
#define SYS_INSTALLER_KITS_H

#include <iostream>
#include "isys_installer_callback.h"
#include "singleton.h"

namespace OHOS {
namespace SysInstaller {
class SysInstallerKits {
public:
    SysInstallerKits() = default;
    virtual ~SysInstallerKits() = default;
    DISALLOW_COPY_AND_MOVE(SysInstallerKits);

    /**
     * Get instance of ohos sys installer.
     *
     * @return Instance of ohos sys installer.sys_installer_client.cpp
     */
    static SysInstallerKits &GetInstance();

    virtual int32_t Init() = 0;
    virtual int32_t SysInstallerInit(bool bStreamUpgrade = false) = 0;
    virtual int32_t StartUpdatePackageZip(const std::string &pkgPath) = 0;
    virtual int32_t StartStreamUpdate() = 0;
    virtual int32_t StopStreamUpdate() = 0;
    virtual int32_t ProcessStreamData(const std::vector<uint8_t>& buffer, uint32_t size) = 0;
    virtual int32_t SetUpdateCallback(const sptr<ISysInstallerCallback> &cb) = 0;
    virtual int32_t GetUpdateStatus() = 0;
    virtual int32_t StartUpdateParaZip(const std::string &pkgPath,
        const std::string &location, const std::string &cfgDir) = 0;
    virtual int32_t StartDeleteParaZip(const std::string &location, const std::string &cfgDir) = 0;
    virtual int32_t AccDecompressAndVerifyPkg(const std::string &srcPath,
        const std::string &dstPath, const uint32_t type) = 0;
    virtual int32_t AccDeleteDir(const std::string &dstPath) = 0;
    virtual int32_t StartUpdateVabPackageZip(const std::vector<std::string> &pkgPath) = 0;
    virtual int32_t StartVabMerge() = 0;
    virtual int32_t EnableVabCheckpoint() = 0;
    virtual int32_t AbortVabActiveSnapshot() = 0;
    virtual int32_t ClearVabMetadataAndCow() = 0;
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_KITS_H
