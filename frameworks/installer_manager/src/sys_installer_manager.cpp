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

#include "sys_installer_manager.h"

#include "log/log.h"
#include "package/pkg_manager.h"
#include "utils.h"
#include "updater_main.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

void SysInstallerManager::RegisterDump(std::unique_ptr<SysInstallerManagerHelper> ptr)
{
    helper_ = std::move(ptr);
}

SysInstallerManager &SysInstallerManager::GetInstance()
{
    static SysInstallerManager instance;
    return instance;
}

int32_t SysInstallerManager::SysInstallerInit(const std::string &taskId)
{
    if (helper_ == nullptr) {
        SysInstallerManagerInit::GetInstance().InvokeEvent(SYS_PRE_INIT_EVENT);
        UpdaterInit::GetInstance().InvokeEvent(UPDATER_PRE_INIT_EVENT);
        UpdaterInit::GetInstance().InvokeEvent(UPDATER_INIT_EVENT);
        if (helper_ == nullptr) {
            RegisterDump(std::make_unique<SysInstallerManagerHelper>());
        }
    }
    return helper_->SysInstallerInit(taskId);
}

int32_t SysInstallerManager::StartUpdatePackageZip(const std::string &taskId, const std::string &pkgPath)
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->StartUpdatePackageZip(taskId, pkgPath);
}

int32_t SysInstallerManager::SetUpdateCallback(const std::string &taskId,
    const sptr<ISysInstallerCallback> &updateCallback)
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->SetUpdateCallback(taskId, updateCallback);
}

int32_t SysInstallerManager::GetUpdateStatus(const std::string &taskId)
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->GetUpdateStatus(taskId);
}

int32_t SysInstallerManager::StartUpdateParaZip(const std::string &taskId, const std::string &pkgPath,
    const std::string &location, const std::string &cfgDir)
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->StartUpdateParaZip(taskId, pkgPath, location, cfgDir);
}

int32_t SysInstallerManager::StartDeleteParaZip(const std::string &taskId, const std::string &location,
    const std::string &cfgDir)
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->StartDeleteParaZip(taskId, location, cfgDir);
}

int32_t SysInstallerManager::AccDecompressAndVerifyPkg(const std::string &taskId, const std::string &srcPath,
    const std::string &dstPath, const uint32_t type)
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->AccDecompressAndVerifyPkg(taskId, srcPath, dstPath, type);
}

int32_t SysInstallerManager::AccDeleteDir(const std::string &taskId, const std::string &dstPath)
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->AccDeleteDir(taskId, dstPath);
}

int32_t SysInstallerManager::StartUpdateVabPackageZip(const std::string &taskId,
    const std::vector<std::string> &pkgPath)
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->StartUpdateVabPackageZip(taskId, pkgPath);
}

int32_t SysInstallerManager::CancelUpdateVabPackageZip(const std::string &taskId)
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->CancelUpdateVabPackageZip(taskId);
}

int32_t SysInstallerManager::StartVabMerge(const std::string &taskId)
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->StartVabMerge(taskId);
}

int32_t SysInstallerManager::CreateVabSnapshotCowImg(const std::unordered_map<std::string, uint64_t> &partitionInfo)
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->CreateVabSnapshotCowImg(partitionInfo);
}

int32_t SysInstallerManager::EnableVabCheckpoint()
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->EnableVabCheckpoint();
}

int32_t SysInstallerManager::AbortVabActiveSnapshot()
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->AbortVabActiveSnapshot();
}

int32_t SysInstallerManager::ClearVabMetadataAndCow()
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->ClearVabMetadataAndCow();
}

int32_t SysInstallerManager::MergeRollbackReasonFile()
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->MergeRollbackReasonFile();
}

std::string  SysInstallerManager::GetUpdateResult(const std::string &taskId, const std::string &taskType,
    const std::string &resultType)
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return std::string("");
    }
    return helper_->GetUpdateResult(taskId, taskType, resultType);
}

int32_t SysInstallerManager::GetMetadataUpdateStatus(int32_t &metadataStatus)
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->GetMetadataUpdateStatus(metadataStatus);
}

int32_t SysInstallerManager::VabUpdateActive()
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->VabUpdateActive();
}
} // namespace SysInstaller
} // namespace OHOS
