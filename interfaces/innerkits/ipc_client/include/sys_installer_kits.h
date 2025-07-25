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
    virtual int32_t SysInstallerInit(const std::string &taskId, bool bStreamUpgrade = false) = 0;
    virtual int32_t StartUpdatePackageZip(const std::string &taskId, const std::string &pkgPath) = 0;
    virtual int32_t StartStreamUpdate() = 0;
    virtual int32_t StopStreamUpdate() = 0;
    virtual int32_t ProcessStreamData(const uint8_t *buffer, uint32_t size) = 0;
    virtual int32_t SetUpdateCallback(const std::string &taskId, const sptr<ISysInstallerCallback> &cb) = 0;
    virtual int32_t GetUpdateStatus(const std::string &taskId) = 0;
    virtual int32_t StartUpdateParaZip(const std::string &taskId, const std::string &pkgPath,
        const std::string &location, const std::string &cfgDir) = 0;
    virtual int32_t StartDeleteParaZip(const std::string &taskId, const std::string &location,
        const std::string &cfgDir) = 0;
    virtual int32_t AccDecompressAndVerifyPkg(const std::string &taskId, const std::string &srcPath,
        const std::string &dstPath, const uint32_t type) = 0;
    virtual int32_t AccDeleteDir(const std::string &taskId, const std::string &dstPath) = 0;
    virtual int32_t CancelUpdateVabPackageZip(const std::string &taskId) = 0;
    virtual int32_t StartUpdateVabPackageZip(const std::string &taskId, const std::vector<std::string> &pkgPath) = 0;
    virtual int32_t CreateVabSnapshotCowImg(const std::unordered_map<std::string, uint64_t> &partitionInfo) = 0;
    virtual int32_t StartVabMerge(const std::string &taskId) = 0;
    virtual int32_t EnableVabCheckpoint() = 0;
    virtual int32_t AbortVabActiveSnapshot() = 0;
    virtual int32_t ClearVabMetadataAndCow() = 0;
    virtual int32_t MergeRollbackReasonFile() = 0;
    virtual std::string GetUpdateResult(const std::string &taskId, const std::string &taskType,
        const std::string &resultType) = 0;
    virtual int32_t GetMetadataUpdateStatus(int32_t &metadataStatus) = 0;
    virtual int32_t VabUpdateActive() = 0;
    virtual int32_t GetMetadataResult(const std::string &action, bool &result) = 0;
    virtual int32_t ExitSysInstaller() = 0;
    virtual int32_t StartAbSync() = 0;
    virtual int32_t SetCpuAffinity(const std::string &taskId, unsigned int reservedCores) = 0;
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_KITS_H
