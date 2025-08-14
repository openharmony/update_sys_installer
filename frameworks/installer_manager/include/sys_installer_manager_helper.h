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

#ifndef SYS_INSTALLER_MANAGER_HELPER_H
#define SYS_INSTALLER_MANAGER_HELPER_H

#include <map>
#include <mutex>

#include "action_processer.h"
#include "status_manager.h"

namespace OHOS {
namespace SysInstaller {
class SysInstallerManagerHelper {
public:
    SysInstallerManagerHelper() = default;
    virtual ~SysInstallerManagerHelper() = default;

    virtual int32_t SysInstallerInit(const std::string &taskId);
    virtual int32_t StartUpdatePackageZip(const std::string &taskId, const std::string &pkgPath);
    virtual int32_t SetUpdateCallback(const std::string &taskId, const sptr<ISysInstallerCallback> &updateCallback);
    virtual int32_t GetUpdateStatus(const std::string &taskId);
    virtual int32_t StartUpdateParaZip(const std::string &taskId, const std::string &pkgPath,
        const std::string &location, const std::string &cfgDir);
    virtual int32_t StartDeleteParaZip(const std::string &taskId, const std::string &location,
        const std::string &cfgDir);
    virtual int32_t AccDecompressAndVerifyPkg(const std::string &taskId, const std::string &srcPath,
        const std::string &dstPath, const uint32_t type);
    virtual int32_t AccDeleteDir(const std::string &taskId, const std::string &dstPath);
    virtual int32_t CancelUpdateVabPackageZip(const std::string &taskId);
    virtual int32_t StartUpdateVabPackageZip(const std::string &taskId, const std::vector<std::string> &pkgPath);
    virtual int32_t CreateVabSnapshotCowImg(const std::unordered_map<std::string, uint64_t> &partitionInfo);
    virtual int32_t StartVabMerge(const std::string &taskId);
    virtual int32_t ClearVabMetadataAndCow();
    virtual std::string GetUpdateResult(const std::string &taskId, const std::string &taskType,
        const std::string &resultType);
    virtual int32_t VabUpdateActive();
    virtual int32_t GetMetadataResult(const std::string &action, bool &result);
    virtual int32_t StartAbSync();
    virtual int32_t SetCpuAffinity(const std::string &taskId, unsigned int reservedCores);

    virtual int32_t InstallCloudRom(const std::string &taskId, InstallMode installMode,
        const std::vector<FeatureInfo> &featureInfos, RebootStatus rebootStatus);
    virtual int32_t UninstallCloudRom(const std::string &taskId,
        const std::vector<FeatureInfo> &featureInfos, RebootStatus rebootStatus);
    virtual int32_t GetFeatureStatus(const std::vector<FeatureInfo> &featureInfos,
        std::vector<FeatureStatus> &statusInfos);
    virtual int32_t GetAllFeatureStatus(const std::string &baseVersion, std::vector<FeatureStatus> &statusInfos);
    virtual int32_t ClearCloudRom(const std::string &baseVersion, const std::string &featureName);

protected:
    std::map<std::string, std::shared_ptr<StatusManager>> statusManagerMap_;
    std::map<std::string, std::shared_ptr<ActionProcesser>> actionProcesserMap_;
    std::recursive_mutex statusLock_;
    std::recursive_mutex processerLock_;

protected:
    std::shared_ptr<StatusManager> GetStatusManager(const std::string &taskId);
    std::shared_ptr<ActionProcesser> GetActionProcesser(const std::string &taskId);
};
} // SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MANAGER_HELPER_H
