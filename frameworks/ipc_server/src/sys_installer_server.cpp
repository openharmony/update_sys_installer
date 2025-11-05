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

#include "accesstoken_kit.h"
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
    DEFINE_EXIT_GUARD();
    LOG(INFO) << "SysInstallerInit";
    if (!logInit_) {
        (void)Utils::MkdirRecursive(SYS_LOG_DIR, 0777); // 0777 : rwxrwxrwx
        InitLogger("SysInstaller");
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
    DEFINE_EXIT_GUARD();
    return SysInstallerManager::GetInstance().StartUpdatePackageZip(taskId, pkgPath);
}

int32_t SysInstallerServer::StartStreamUpdate()
{
    LOG(INFO) << "StartStreamUpdate";
    DEFINE_EXIT_GUARD();
    return StreamInstallerManager::GetInstance().StartStreamUpdate();
}

int32_t SysInstallerServer::StopStreamUpdate()
{
    LOG(INFO) << "StopStreamUpdate";
    DEFINE_EXIT_GUARD();
    return StreamInstallerManager::GetInstance().StopStreamUpdate();
}

int32_t SysInstallerServer::ProcessStreamData(const BufferInfoParcel &bufferParcel)
{
    LOG(INFO) << "ProcessStreamData";
    DEFINE_EXIT_GUARD();
    return StreamInstallerManager::GetInstance().ProcessStreamData(bufferParcel.bufferInfo.buffer,
        bufferParcel.bufferInfo.size);
}

int32_t SysInstallerServer::SetUpdateCallback(const std::string &taskId,
    const sptr<ISysInstallerCallback> &updateCallback)
{
    LOG(INFO) << "SetUpdateCallback";
    DEFINE_EXIT_GUARD();
    if (bStreamUpgrade_) {
        return StreamInstallerManager::GetInstance().SetUpdateCallback(updateCallback);
    } else {
        return SysInstallerManager::GetInstance().SetUpdateCallback(taskId, updateCallback);
    }
}

int32_t SysInstallerServer::GetUpdateStatus(const std::string &taskId)
{
    LOG(INFO) << "GetUpdateStatus";
    DEFINE_EXIT_GUARD();
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
    DEFINE_EXIT_GUARD();
    return SysInstallerManager::GetInstance().StartUpdateParaZip(taskId, pkgPath, location, cfgDir);
}

int32_t SysInstallerServer::StartDeleteParaZip(const std::string &taskId, const std::string &location,
    const std::string &cfgDir)
{
    LOG(INFO) << "StartDeleteParaZip";
    DEFINE_EXIT_GUARD();
    return SysInstallerManager::GetInstance().StartDeleteParaZip(taskId, location, cfgDir);
}

int32_t SysInstallerServer::AccDecompressAndVerifyPkg(const std::string &taskId, const std::string &srcPath,
    const std::string &dstPath, const uint32_t type)
{
    LOG(INFO) << "AccDecompressAndVerifyPkg";
    DEFINE_EXIT_GUARD();
    return SysInstallerManager::GetInstance().AccDecompressAndVerifyPkg(taskId, srcPath, dstPath, type);
}

int32_t SysInstallerServer::AccDeleteDir(const std::string &taskId, const std::string &dstPath)
{
    LOG(INFO) << "AccDeleteDir";
    DEFINE_EXIT_GUARD();
    return SysInstallerManager::GetInstance().AccDeleteDir(taskId, dstPath);
}

int32_t SysInstallerServer::StartUpdateVabPackageZip(const std::string &taskId,
    const std::vector<std::string> &pkgPath)
{
    LOG(INFO) << "StartUpdateVabPackageZip";
    DEFINE_EXIT_GUARD();
    return SysInstallerManager::GetInstance().StartUpdateVabPackageZip(taskId, pkgPath);
}

int32_t SysInstallerServer::CancelUpdateVabPackageZip(const std::string &taskId)
{
    LOG(INFO) << "CancelUpdateVabPackageZip";
    DEFINE_EXIT_GUARD();
    return SysInstallerManager::GetInstance().CancelUpdateVabPackageZip(taskId);
}

int32_t SysInstallerServer::GetPartitionAvailableSize(const std::map<std::string, uint64_t>& dtsImgsSize,
    uint64_t& availSize)
{
    LOG(INFO) << "GetPartitionAvailableSize";
    DEFINE_EXIT_GUARD();
    return SysInstallerManager::GetInstance().GetPartitionAvailableSize(dtsImgsSize, availSize);
}

int32_t SysInstallerServer::StartVabMerge(const std::string &taskId)
{
    LOG(INFO) << "StartVabMerge";
    DEFINE_EXIT_GUARD();
    return SysInstallerManager::GetInstance().StartVabMerge(taskId);
}

int32_t SysInstallerServer::CreateVabSnapshotCowImg(const std::unordered_map<std::string, uint64_t> &partitionInfo)
{
    LOG(INFO) << "CreateVabSnapshotCowImg";
    DEFINE_EXIT_GUARD();
    return SysInstallerManager::GetInstance().CreateVabSnapshotCowImg(partitionInfo);
}

int32_t SysInstallerServer::CreateVabSnapshotCowImg(const std::string &name, uint64_t size, uint64_t splitSize,
    uint64_t &createdSize, bool &isCreated)
{
    LOG(INFO) << "CreateVabSnapshotCowImg";
    DEFINE_EXIT_GUARD();
    return SysInstallerManager::GetInstance().CreateVabSnapshotCowImg(name, size, splitSize, createdSize, isCreated);
}

int32_t SysInstallerServer::ClearVabMetadataAndCow()
{
    LOG(INFO) << "ClearVabMetadataAndCow";
    DEFINE_EXIT_GUARD();
    return SysInstallerManager::GetInstance().ClearVabMetadataAndCow();
}

int32_t SysInstallerServer::GetUpdateResult(const std::string &taskId, const std::string &taskType,
    const std::string &resultType, std::string &updateResult)
{
    DEFINE_EXIT_GUARD();
    updateResult = SysInstallerManager::GetInstance().GetUpdateResult(taskId, taskType, resultType);
    return 0;
}

int32_t SysInstallerServer::VabUpdateActive()
{
    LOG(INFO) << "VabUpdateActive";
    DEFINE_EXIT_GUARD();
    return SysInstallerManager::GetInstance().VabUpdateActive();
}

int32_t SysInstallerServer::GetMetadataResult(const std::string &action, bool &result)
{
    LOG(INFO) << "GetMetadataResult";
    DEFINE_EXIT_GUARD();
    return SysInstallerManager::GetInstance().GetMetadataResult(action, result);
}

bool SysInstallerServer::IsTaskRunning(void)
{
    return !SysInstallerExitGuard::GetRunningSet().empty();
}

std::string SysInstallerServer::GetRunningTask(void)
{
    const auto &runningSet = SysInstallerExitGuard::GetRunningSet();
    if (runningSet.size() > SysInstallerExitGuard::MAX_RUNNING_SET_SIZE) {
        LOG(ERROR) << "size too big, size is " << runningSet.size();
        return "error: size too big";
    }
    std::ostringstream ss;
    for (const auto &tag : runningSet) {
        ss << tag;
    }
    return ss.str();
}

int32_t SysInstallerServer::InstallCloudRom(const std::string &taskId,
    InstallMode installMode, const std::vector<FeatureInfo> &featureInfos, RebootStatus rebootStatus)
{
    LOG(INFO) << "InstallCloudRom";
    DEFINE_EXIT_GUARD();
    return SysInstallerManager::GetInstance().InstallCloudRom(taskId, installMode, featureInfos, rebootStatus);
}

int32_t SysInstallerServer::UninstallCloudRom(const std::string &taskId,
    const std::vector<FeatureInfo> &featureInfos, RebootStatus rebootStatus)
{
    LOG(INFO) << "UninstallCloudRom";
    DEFINE_EXIT_GUARD();
    return SysInstallerManager::GetInstance().UninstallCloudRom(taskId, featureInfos, rebootStatus);
}

int32_t SysInstallerServer::GetFeatureStatus(const std::vector<FeatureInfo> &featureInfos,
    std::vector<FeatureStatus> &statusInfos)
{
    LOG(INFO) << "GetFeatureStatus";
    DEFINE_EXIT_GUARD();
    return SysInstallerManager::GetInstance().GetFeatureStatus(featureInfos, statusInfos);
}

int32_t SysInstallerServer::GetAllFeatureStatus(const std::string &baseVersion,
    std::vector<FeatureStatus> &statusInfos)
{
    LOG(INFO) << "GetAllFeatureStatus";
    DEFINE_EXIT_GUARD();
    return SysInstallerManager::GetInstance().GetAllFeatureStatus(baseVersion, statusInfos);
}

int32_t SysInstallerServer::ClearCloudRom(const std::string &baseVersion,
    const std::string &featureName)
{
    LOG(INFO) << "ClearCloudRom";
    DEFINE_EXIT_GUARD();
    return SysInstallerManager::GetInstance().ClearCloudRom(baseVersion, featureName);
}

int32_t SysInstallerServer::ExitSysInstaller()
{
    LOG(INFO) << "ExitSysInstaller";
    if (IsTaskRunning()) {
        LOG(ERROR) << "SysInstaller running, can't exit, running info " << GetRunningTask();
        return -1;
    }
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
    DEFINE_EXIT_GUARD();
    return SysInstallerManager::GetInstance().StartAbSync();
}

int32_t SysInstallerServer::SetCpuAffinity(const std::string &taskId, uint32_t reservedCores)
{
    LOG(INFO) << "SetCpuAffinity reservedCores:" << reservedCores;
    DEFINE_EXIT_GUARD();
    return SysInstallerManager::GetInstance().SetCpuAffinity(taskId, reservedCores);
}

bool SysInstallerServer::IsPermissionGranted(void)
{
    Security::AccessToken::AccessTokenID callerToken = IPCSkeleton::GetCallingTokenID();
    std::string permission = "ohos.permission.UPDATE_SYSTEM";

    int verifyResult = Security::AccessToken::AccessTokenKit::VerifyAccessToken(callerToken, permission);
    bool isPermissionGranted = (verifyResult == Security::AccessToken::PERMISSION_GRANTED);
    if (!isPermissionGranted) {
        LOG(ERROR) << "not granted " << permission;
    }
    return isPermissionGranted;
}

bool SysInstallerServer::CheckCallingPerm(void)
{
    int32_t callingUid = OHOS::IPCSkeleton::GetCallingUid();
    LOG(INFO) << "CheckCallingPerm callingUid:" << callingUid;
    if (callingUid == 0) {
        return true;
    }
    return callingUid == Updater::Utils::USER_UPDATE_AUTHORITY && IsPermissionGranted();
}

int32_t SysInstallerServer::CallbackEnter([[maybe_unused]] uint32_t code)
{
    LOG(INFO) << "Received stub message:" << code << ", callingUid:" << IPCSkeleton::GetCallingUid();
    if (!CheckCallingPerm()) {
        LOG(ERROR) << "SysInstallerServer CheckCallingPerm fail";
        return ERR_INVALID_VALUE;
    }
    return ERR_NONE;
}

int32_t SysInstallerServer::CallbackExit([[maybe_unused]] uint32_t code, [[maybe_unused]] int32_t result)
{
    return ERR_NONE;
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
