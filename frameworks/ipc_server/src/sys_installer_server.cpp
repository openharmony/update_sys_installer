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

int32_t SysInstallerServer::SysInstallerInit(bool bStreamUpgrade)
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
        SysInstallerManager::GetInstance().SysInstallerInit();
    }
    
    return 0;
}

int32_t SysInstallerServer::StartUpdatePackageZip(const std::string &pkgPath)
{
    LOG(INFO) << "StartUpdatePackageZip";
    return SysInstallerManager::GetInstance().StartUpdatePackageZip(pkgPath);
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

int32_t SysInstallerServer::ProcessStreamData(const std::vector<uint8_t>& buffer, uint32_t size)
{
    LOG(INFO) << "ProcessStreamData";
    return StreamInstallerManager::GetInstance().ProcessStreamData(buffer, size);
}

int32_t SysInstallerServer::SetUpdateCallback(const sptr<ISysInstallerCallback> &updateCallback)
{
    LOG(INFO) << "SetUpdateCallback";
    if (bStreamUpgrade_) {
        return StreamInstallerManager::GetInstance().SetUpdateCallback(updateCallback);
    } else {
        return SysInstallerManager::GetInstance().SetUpdateCallback(updateCallback);
    }
}

int32_t SysInstallerServer::GetUpdateStatus()
{
    LOG(INFO) << "GetUpdateStatus";
    if (bStreamUpgrade_) {
        return StreamInstallerManager::GetInstance().GetUpdateStatus();
    } else {
        return SysInstallerManager::GetInstance().GetUpdateStatus();
    }
}

int32_t SysInstallerServer::StartUpdateParaZip(const std::string &pkgPath,
    const std::string &location, const std::string &cfgDir)
{
    LOG(INFO) << "StartUpdateParaZip";
    return SysInstallerManager::GetInstance().StartUpdateParaZip(pkgPath, location, cfgDir);
}

int32_t SysInstallerServer::StartDeleteParaZip(const std::string &location, const std::string &cfgDir)
{
    LOG(INFO) << "StartDeleteParaZip";
    return SysInstallerManager::GetInstance().StartDeleteParaZip(location, cfgDir);
}

int32_t SysInstallerServer::AccDecompressAndVerifyPkg(const std::string &srcPath,
    const std::string &dstPath, const uint32_t type)
{
    LOG(INFO) << "AccDecompressAndVerifyPkg";
    return SysInstallerManager::GetInstance().AccDecompressAndVerifyPkg(srcPath, dstPath, type);
}

int32_t SysInstallerServer::AccDeleteDir(const std::string &dstPath)
{
    LOG(INFO) << "AccDeleteDir";
    return SysInstallerManager::GetInstance().AccDeleteDir(dstPath);
}

int32_t SysInstallerServer::StartUpdateVabPackageZip(const std::vector<std::string> &pkgPath)
{
    LOG(INFO) << "StartUpdateVabPackageZip";
    return SysInstallerManager::GetInstance().StartUpdateVabPackageZip(pkgPath);
}

int32_t SysInstallerServer::CancelUpdateVabPackageZip(void)
{
    LOG(INFO) << "CancelUpdateVabPackageZip";
    return SysInstallerManager::GetInstance().CancelUpdateVabPackageZip();
}

int32_t SysInstallerServer::StartVabMerge()
{
    LOG(INFO) << "StartVabMerge";
    return SysInstallerManager::GetInstance().StartVabMerge();
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
