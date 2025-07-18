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

#include "sys_installer_server.h"

#include "iservice_registry.h"
#include "log/log.h"
#include "securec.h"
#include "system_ability_definition.h"
#include "utils.h"
#include "buffer_info_parcel.h"

namespace OHOS {
namespace SysInstaller {
REGISTER_SYSTEM_ABILITY_BY_ID(SysInstallerServer, SYS_INSTALLER_DISTRIBUTED_SERVICE_ID, false)

using namespace Updater;

void __attribute__((weak)) InitSysLogger(const std::string &tag)
{
    InitUpdaterLogger(tag, SYS_LOG_FILE, SYS_STAGE_FILE, SYS_ERROR_FILE);
}

SysInstallerServer::SysInstallerServer(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(systemAbilityId, runOnCreate)
{
}

SysInstallerServer::~SysInstallerServer()
{
}

int32_t SysInstallerServer::SysInstallerInit(const std::string &taskId, bool bStreamUpgrade)
{
    std::lock_guard<std::mutex> lock(sysInstallerServerLock_);
    LOG(INFO) << "SysInstallerInit";
    if (!logInit_) {
        (void)Utils::MkdirRecursive(SYS_LOG_DIR, 0777); // 0777 : rwxrwxrwx
        InitUpdaterLogger("SysInstaller", SYS_LOG_FILE, SYS_STAGE_FILE, SYS_ERROR_FILE);
        logInit_ = true;
    }
    bStreamUpgrade_ = bStreamUpgrade;
    if (bStreamUpgrade_) {
        StreamInstallerManager::GetInstance().SysInstallerInit();
    } else {
        SysInstallerManager::GetInstance().SysInstallerInit(taskId);
    }
    
    return 0;
}

int32_t SysInstallerServer::StartUpdatePackageZip(const std::string &taskId, const std::string &pkgPath)
{
    LOG(INFO) << "StartUpdatePackageZip";
    return SysInstallerManager::GetInstance().StartUpdatePackageZip(taskId, pkgPath);
}

int32_t SysInstallerServer::StartStreamUpdate()
{
    LOG(INFO) << "StartStreamUpdate";
    return StreamInstallerManager::GetInstance().StartStreamUpdate();
}

int32_t SysInstallerServer::StopStreamUpdate()
{
    LOG(INFO) << "StopStreamUpdate";
    return StreamInstallerManager::GetInstance().StopStreamUpdate();
}

int32_t SysInstallerServer::ProcessStreamData(const BufferInfoParcel &bufferParcel)
{
    LOG(INFO) << "ProcessStreamData";
    return StreamInstallerManager::GetInstance().ProcessStreamData(bufferParcel.bufferInfo.buffer,
        bufferParcel.bufferInfo.size);
}

int32_t SysInstallerServer::SetUpdateCallback(const std::string &taskId,
    const sptr<ISysInstallerCallback> &updateCallback)
{
    LOG(INFO) << "SetUpdateCallback";
    if (bStreamUpgrade_) {
        return StreamInstallerManager::GetInstance().SetUpdateCallback(updateCallback);
    } else {
        return SysInstallerManager::GetInstance().SetUpdateCallback(taskId, updateCallback);
    }
}

int32_t SysInstallerServer::GetUpdateStatus(const std::string &taskId)
{
    LOG(INFO) << "GetUpdateStatus";
    if (bStreamUpgrade_) {
        return StreamInstallerManager::GetInstance().GetUpdateStatus();
    } else {
        return SysInstallerManager::GetInstance().GetUpdateStatus(taskId);
    }
}

int32_t SysInstallerServer::StartUpdateParaZip(const std::string &taskId, const std::string &pkgPath,
    const std::string &location, const std::string &cfgDir)
{
    LOG(INFO) << "StartUpdateParaZip";
    return SysInstallerManager::GetInstance().StartUpdateParaZip(taskId, pkgPath, location, cfgDir);
}

int32_t SysInstallerServer::StartDeleteParaZip(const std::string &taskId, const std::string &location,
    const std::string &cfgDir)
{
    LOG(INFO) << "StartDeleteParaZip";
    return SysInstallerManager::GetInstance().StartDeleteParaZip(taskId, location, cfgDir);
}

int32_t SysInstallerServer::AccDecompressAndVerifyPkg(const std::string &taskId, const std::string &srcPath,
    const std::string &dstPath, const uint32_t type)
{
    LOG(INFO) << "AccDecompressAndVerifyPkg";
    return SysInstallerManager::GetInstance().AccDecompressAndVerifyPkg(taskId, srcPath, dstPath, type);
}

int32_t SysInstallerServer::AccDeleteDir(const std::string &taskId, const std::string &dstPath)
{
    LOG(INFO) << "AccDeleteDir";
    return SysInstallerManager::GetInstance().AccDeleteDir(taskId, dstPath);
}

int32_t SysInstallerServer::StartUpdateVabPackageZip(const std::string &taskId,
    const std::vector<std::string> &pkgPath)
{
    LOG(INFO) << "StartUpdateVabPackageZip";
    return SysInstallerManager::GetInstance().StartUpdateVabPackageZip(taskId, pkgPath);
}

int32_t SysInstallerServer::CancelUpdateVabPackageZip(const std::string &taskId)
{
    LOG(INFO) << "CancelUpdateVabPackageZip";
    return SysInstallerManager::GetInstance().CancelUpdateVabPackageZip(taskId);
}

int32_t SysInstallerServer::StartVabMerge(const std::string &taskId)
{
    LOG(INFO) << "StartVabMerge";
    return SysInstallerManager::GetInstance().StartVabMerge(taskId);
}

int32_t SysInstallerServer::CreateVabSnapshotCowImg(const std::unordered_map<std::string, uint64_t> &partitionInfo)
{
    LOG(INFO) << "CreateVabSnapshotCowImg";
    return SysInstallerManager::GetInstance().CreateVabSnapshotCowImg(partitionInfo);
}

int32_t SysInstallerServer::EnableVabCheckpoint()
{
    LOG(INFO) << "EnableVabCheckpoint";
    return SysInstallerManager::GetInstance().EnableVabCheckpoint();
}

int32_t SysInstallerServer::AbortVabActiveSnapshot()
{
    LOG(INFO) << "AbortVabActiveSnapshot";
    return SysInstallerManager::GetInstance().AbortVabActiveSnapshot();
}

int32_t SysInstallerServer::ClearVabMetadataAndCow()
{
    LOG(INFO) << "ClearVabMetadataAndCow";
    return SysInstallerManager::GetInstance().ClearVabMetadataAndCow();
}

int32_t SysInstallerServer::MergeRollbackReasonFile()
{
    LOG(INFO) << "MergeRollbackReasonFile";
    return SysInstallerManager::GetInstance().MergeRollbackReasonFile();
}

int32_t SysInstallerServer::GetUpdateResult(const std::string &taskId, const std::string &taskType,
    const std::string &resultType, std::string &updateResult)
{
    updateResult = SysInstallerManager::GetInstance().GetUpdateResult(taskId, taskType, resultType);
    return 0;
}

int32_t SysInstallerServer::GetMetadataUpdateStatus(int32_t &metadataStatus)
{
    LOG(INFO) << "GetMetadataUpdateStatus";
    return SysInstallerManager::GetInstance().GetMetadataUpdateStatus(metadataStatus);
}

int32_t SysInstallerServer::VabUpdateActive()
{
    LOG(INFO) << "VabUpdateActive";
    return SysInstallerManager::GetInstance().VabUpdateActive();
}

int32_t SysInstallerServer::GetMetadataResult(const std::string &action, bool &result)
{
    LOG(INFO) << "GetMetadataResult";
    return SysInstallerManager::GetInstance().GetMetadataResult(action, result);
}

int32_t SysInstallerServer::ExitSysInstaller()
{
    LOG(INFO) << "ExitSysInstaller";
    sptr<ISystemAbilityManager> sm = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (sm == nullptr) {
        LOG(ERROR) << "GetSystemAbilityManager samgr object null!";
        return 0;
    }
    if (sm->UnloadSystemAbility(SYS_INSTALLER_DISTRIBUTED_SERVICE_ID) != 0) {
        LOG(ERROR) << "UnloadSystemAbility error!";
    }
    return 0;
}

int32_t SysInstallerServer::StartAbSync()
{
    LOG(INFO) << "StartAbSync";
    return SysInstallerManager::GetInstance().StartAbSync();
}

int32_t SysInstallerServer::SetCpuAffinity(const std::string &taskId, int32_t reservedCores)
{
    LOG(INFO) << "SetCpuAffinity reservedCores:" << reservedCores;
    return SysInstallerManager::GetInstance().SetCpuAffinity(taskId, reservedCores);
}

void SysInstallerServer::OnStart()
{
    LOG(INFO) << "OnStart";
    bool res = Publish(this);
    if (!res) {
        LOG(ERROR) << "OnStart failed";
    }

    return;
}

void SysInstallerServer::OnStop()
{
    LOG(INFO) << "OnStop";
}
} // namespace SysInstaller
} // namespace OHOS
