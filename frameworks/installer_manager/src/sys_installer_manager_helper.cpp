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

#include "sys_installer_manager_helper.h"

#include "action_processer.h"
#include "log/log.h"
#include "package/cert_verify.h"
#include "package/pkg_manager.h"
#include "utils.h"
#include "pkg_verify.h"
#include "ab_update.h"

namespace OHOS {
namespace SysInstaller {
using namespace Hpackage;
using namespace Updater;

int32_t SysInstallerManagerHelper::SysInstallerInit(const std::string &taskId)
{
    LOG(INFO) << "SysInstallerInit taskId : " << taskId;
    std::shared_ptr<StatusManager> statusManager = nullptr;
    {
        std::lock_guard<std::recursive_mutex> lock(statusLock_);
        if (statusManagerMap_.count(taskId) > 0) {
            LOG(INFO) << "is has been init";
            return 0;
        }

        statusManager = std::make_shared<StatusManager>();
        statusManagerMap_.emplace(taskId, statusManager);
        statusManager->Init();
    }

    std::lock_guard<std::recursive_mutex> lock(processerLock_);
    std::shared_ptr<ActionProcesser> actionProcesser = std::make_shared<ActionProcesser>(statusManager);
    actionProcesserMap_.emplace(taskId, actionProcesser);
    return 0;
}

int32_t SysInstallerManagerHelper::StartUpdatePackageZip(const std::string &taskId, const std::string &pkgPath)
{
    LOG(INFO) << "StartUpdatePackageZip start taskId : " << taskId;
    std::shared_ptr<StatusManager> statusManager = GetStatusManager(taskId);
    if (statusManager == nullptr) {
        LOG(ERROR) << "statusManager nullptr";
        return -1;
    }

    std::shared_ptr<ActionProcesser> actionProcesser = GetActionProcesser(taskId);
    if (actionProcesser == nullptr) {
        LOG(ERROR) << "actionProcesser nullptr";
        return -1;
    }

    if (actionProcesser->IsRunning()) {
        LOG(ERROR) << "ActionProcesser IsRunning";
        return -1;
    }
    std::vector<std::string> filePath = {};
    filePath.push_back(pkgPath);
    actionProcesser->AddAction(std::make_unique<PkgVerify>(statusManager, filePath));
    actionProcesser->AddAction(std::make_unique<ABUpdate>(statusManager, pkgPath));
    actionProcesser->Start();
    return 0;
}

int32_t SysInstallerManagerHelper::SetUpdateCallback(const std::string &taskId,
    const sptr<ISysInstallerCallback> &updateCallback)
{
    std::shared_ptr<StatusManager> statusManager = GetStatusManager(taskId);
    if (statusManager == nullptr) {
        LOG(ERROR) << "statusManager nullptr";
        return -1;
    }
    return statusManager->SetUpdateCallback(updateCallback);
}

int32_t SysInstallerManagerHelper::GetUpdateStatus(const std::string &taskId)
{
    std::shared_ptr<StatusManager> statusManager = GetStatusManager(taskId);
    if (statusManager == nullptr) {
        LOG(ERROR) << "statusManager nullptr";
        return -1;
    }
    return statusManager->GetUpdateStatus();
}

int32_t SysInstallerManagerHelper::StartUpdateParaZip(const std::string &taskId, const std::string &pkgPath,
    const std::string &location, const std::string &cfgDir)
{
    return -1;
}

int32_t SysInstallerManagerHelper::StartDeleteParaZip(const std::string &taskId,
    const std::string &location, const std::string &cfgDir)
{
    return -1;
}

int32_t SysInstallerManagerHelper::AccDecompressAndVerifyPkg(const std::string &taskId,
    const std::string &srcPath, const std::string &dstPath, const uint32_t type)
{
    return -1;
}

int32_t SysInstallerManagerHelper::AccDeleteDir(const std::string &taskId, const std::string &dstPath)
{
    return -1;
}

int32_t SysInstallerManagerHelper::StartUpdateVabPackageZip(const std::string &taskId,
    const std::vector<std::string> &pkgPath)
{
    return -1;
}

int32_t SysInstallerManagerHelper::CancelUpdateVabPackageZip(const std::string &taskId)
{
    return -1;
}

int32_t SysInstallerManagerHelper::StartVabMerge(const std::string &taskId)
{
    return -1;
}

int32_t SysInstallerManagerHelper::CreateVabSnapshotCowImg(const std::unordered_map<std::string,
                                                           uint64_t> &partitionInfo)
{
    return -1;
}

int32_t SysInstallerManagerHelper::EnableVabCheckpoint()
{
    return -1;
}

int32_t SysInstallerManagerHelper::AbortVabActiveSnapshot()
{
    return -1;
}

int32_t SysInstallerManagerHelper::ClearVabMetadataAndCow()
{
    return -1;
}

int32_t SysInstallerManagerHelper::MergeRollbackReasonFile()
{
    return -1;
}

std::string SysInstallerManagerHelper::GetUpdateResult(const std::string &taskId, const std::string &taskType,
    const std::string &resultType)
{
    LOG(INFO) << "GetUpdateResult start taskId : " << taskId;
    std::shared_ptr<ActionProcesser> actionProcesser = GetActionProcesser(taskId);
    if (actionProcesser == nullptr) {
        LOG(ERROR) << "actionProcesser nullptr";
        return "has not task";
    }

    if (actionProcesser->IsRunning()) {
        LOG(ERROR) << "ActionProcesser IsRunning";
        return "task is running";
    }

    {
        std::lock_guard<std::recursive_mutex> lock(statusLock_);
        auto inter = statusManagerMap_.find(taskId);
        if (inter != statusManagerMap_.end()) {
            statusManagerMap_.erase(inter);
        }
    }

    {
        std::lock_guard<std::recursive_mutex> lock(processerLock_);
        auto inter = actionProcesserMap_.find(taskId);
        if (inter != actionProcesserMap_.end()) {
            actionProcesserMap_.erase(inter);
        }
    }
    return "success";
}

std::shared_ptr<StatusManager> SysInstallerManagerHelper::GetStatusManager(const std::string &taskId)
{
    std::lock_guard<std::recursive_mutex> lock(statusLock_);
    auto inter = statusManagerMap_.find(taskId);
    if (inter != statusManagerMap_.end()) {
        return inter->second;
    }
    return nullptr;
}

std::shared_ptr<ActionProcesser> SysInstallerManagerHelper::GetActionProcesser(const std::string &taskId)
{
    std::lock_guard<std::recursive_mutex> lock(processerLock_);
    auto inter = actionProcesserMap_.find(taskId);
    if (inter != actionProcesserMap_.end()) {
        return inter->second;
    }
    return nullptr;
}

int32_t SysInstallerManagerHelper::GetMetadataUpdateStatus(int32_t &metadataStatus)
{
    return -1;
}

int32_t SysInstallerManagerHelper::VabUpdateActive()
{
    return -1;
}
} // namespace SysInstaller
} // namespace OHOS
