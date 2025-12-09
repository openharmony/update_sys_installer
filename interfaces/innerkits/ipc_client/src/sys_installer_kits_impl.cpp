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

#include "sys_installer_kits_impl.h"

#include <unistd.h>
#include <sys/stat.h>

#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "log/log.h"
#include "securec.h"
#include "system_ability_definition.h"
#include "utils.h"

#include "isys_installer.h"
#include "isys_installer_callback.h"
#include "sys_installer_callback.h"
#include "sys_installer_common.h"
#include "sys_installer_load_callback.h"
#include "sys_installer_proxy.h"
#include "buffer_info_parcel.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;
using namespace Updater::Utils;
using namespace Utils;
constexpr int LOAD_SA_TIMEOUT_MS = 3;

SysInstallerKitsImpl &SysInstallerKitsImpl::GetInstance()
{
    static SysInstallerKitsImpl instance;
    return instance;
}

void SysInstallerKitsImpl::ResetService(const wptr<IRemoteObject>& remote)
{
    LOG(INFO) << "Remote is dead, reset service instance";

    std::lock_guard<std::mutex> lock(sysInstallerLock_);
    if (sysInstaller_ != nullptr) {
        sptr<IRemoteObject> object = sysInstaller_->AsObject();
        if ((object != nullptr) && (remote == object)) {
            object->RemoveDeathRecipient(deathRecipient_);
            sysInstaller_ = nullptr;
        }
    }
}

sptr<ISysInstaller> SysInstallerKitsImpl::GetService()
{
    std::lock_guard<std::mutex> lock(sysInstallerLock_);
    if (sysInstaller_ != nullptr) {
        return sysInstaller_;
    }

    sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        LOG(ERROR) << "Get samgr failed";
        return nullptr;
    }
    sptr<IRemoteObject> object = samgr->GetSystemAbility(SYS_INSTALLER_DISTRIBUTED_SERVICE_ID);
    if (object == nullptr) {
        LOG(ERROR) << "Get update object from samgr failed";
        return nullptr;
    }

    if (deathRecipient_ == nullptr) {
        deathRecipient_ = new DeathRecipient();
    }

    if ((object->IsProxyObject()) && (!object->AddDeathRecipient(deathRecipient_))) {
        LOG(ERROR) << "Failed to add death recipient";
    }

    LOG(INFO) << "get remote object ok";
    sysInstaller_ = iface_cast<ISysInstaller>(object);
    if (sysInstaller_ == nullptr) {
        LOG(ERROR) << "account iface_cast failed";
    }
    return sysInstaller_;
}

void SysInstallerKitsImpl::DeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &remote)
{
    SysInstallerKitsImpl::GetInstance().ResetService(remote);
}

int32_t SysInstallerKitsImpl::Init()
{
    std::lock_guard<std::mutex> lock(sysInstallerLock_);
    if (sysInstaller_ != nullptr) {
        LOG(INFO) << "already init";
        return 0;
    }
    (void)Utils::MkdirRecursive(SYS_LOG_DIR, 0775); // 0775 : rwxrwxr-x
    InitUpdaterLogger("SysInstallerClient", SYS_LOG_FILE, SYS_STAGE_FILE, SYS_ERROR_FILE);
    mode_t mode = 0664; // 0664 : -rw-rw-r--
    (void)chown(SYS_LOG_FILE, USER_ROOT_AUTHORITY, GROUP_ROOT_AUTHORITY);
    (void)chown(SYS_STAGE_FILE, USER_ROOT_AUTHORITY, GROUP_ROOT_AUTHORITY);
    (void)chown(SYS_ERROR_FILE, USER_ROOT_AUTHORITY, GROUP_ROOT_AUTHORITY);
    (void)chmod(SYS_LOG_FILE, mode);
    (void)chmod(SYS_STAGE_FILE, mode);
    (void)chmod(SYS_ERROR_FILE, mode);

    // 构造步骤1的SystemAbilityLoadCallbackStub子类的实例
    sptr<SysInstallerLoadCallback> loadCallback_ = new SysInstallerLoadCallback();
    // 调用LoadSystemAbility方法
    sptr<ISystemAbilityManager> sm = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (sm == nullptr) {
        LOG(ERROR) << "GetSystemAbilityManager samgr object null";
        return -1;
    }
    int32_t result = sm->LoadSystemAbility(SYS_INSTALLER_DISTRIBUTED_SERVICE_ID, loadCallback_);
    if (result != ERR_OK) {
        LOG(ERROR) << "systemAbilityId " << SYS_INSTALLER_DISTRIBUTED_SERVICE_ID <<
            " load failed, result code:" << result;
        return -1;
    }

    std::unique_lock<std::mutex> callbackLock(serviceMutex_);
    serviceCv_.wait_for(callbackLock, std::chrono::seconds(LOAD_SA_TIMEOUT_MS));
    return 0;
}

int32_t SysInstallerKitsImpl::SysInstallerInit(const std::string &taskId, bool bStreamUpgrade)
{
    LOG(INFO) << "SysInstallerInit";
    int ret = Init();
    if (ret != 0) {
        LOG(ERROR) << "Init failed";
        return ret;
    }

    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    updateService->SysInstallerInit(taskId, bStreamUpgrade);
    return 0;
}

int32_t SysInstallerKitsImpl::StartUpdatePackageZip(const std::string &taskId, const std::string &pkgPath)
{
    LOG(INFO) << "StartUpdatePackageZip";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->StartUpdatePackageZip(taskId, pkgPath);
    LOG(INFO) << "StartUpdatePackageZip ret:" << ret;
    return ret;
}

int32_t SysInstallerKitsImpl::StartStreamUpdate()
{
    LOG(INFO) << "StartStreamUpdate";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->StartStreamUpdate();
    LOG(INFO) << "StartStreamUpdate ret:" << ret;
    return ret;
}

int32_t SysInstallerKitsImpl::StopStreamUpdate()
{
    LOG(INFO) << "StopStreamUpdate";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->StopStreamUpdate();
    LOG(INFO) << "StopStreamUpdate ret:" << ret;
    return ret;
}

int32_t SysInstallerKitsImpl::ProcessStreamData(const uint8_t *buffer, uint32_t size)
{
    LOG(INFO) << "ProcessStreamData";
    auto updateService = GetService();
    BufferInfoParcel bufferParcel;
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    bufferParcel.bufferInfo.buffer = buffer;
    bufferParcel.bufferInfo.size = size;
    int32_t ret = updateService->ProcessStreamData(bufferParcel);
    LOG(INFO) << "ProcessStreamData ret:" << ret;
    return ret;
}

int32_t SysInstallerKitsImpl::SetUpdateCallback(const std::string &taskId, sptr<ISysInstallerCallbackFunc> callback)
{
    LOG(INFO) << "SetUpdateCallback";
    if (callback == nullptr) {
        LOG(ERROR) << "callback null";
        return -1;
    }

    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }

    sptr<ISysInstallerCallback> updateCallBack = sptr<SysInstallerCallback>::MakeSptr(callback);
    if (updateCallBack == nullptr) {
        LOG(ERROR) << "updateCallBack nullptr";
        return -1;
    }

    return updateService->SetUpdateCallback(taskId, updateCallBack);
}

int32_t SysInstallerKitsImpl::GetUpdateStatus(const std::string &taskId)
{
    LOG(INFO) << "GetUpdateStatus";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    return updateService->GetUpdateStatus(taskId);
}

int32_t SysInstallerKitsImpl::StartUpdateParaZip(const std::string &taskId, const std::string &pkgPath,
    const std::string &location, const std::string &cfgDir)
{
    LOG(INFO) << "StartUpdateParaZip";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->StartUpdateParaZip(taskId, pkgPath, location, cfgDir);
    LOG(INFO) << "StartUpdateParaZip ret:" << ret;
#ifdef UPDATER_UT
    return -1;
#else
    return ret;
#endif
}

int32_t SysInstallerKitsImpl::StartDeleteParaZip(const std::string &taskId, const std::string &location,
    const std::string &cfgDir)
{
    LOG(INFO) << "StartDeleteParaZip";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->StartDeleteParaZip(taskId, location, cfgDir);
    LOG(INFO) << "StartDeleteParaZip ret:" << ret;
#ifdef UPDATER_UT
    return -1;
#else
    return ret;
#endif
}

int32_t SysInstallerKitsImpl::AccDecompressAndVerifyPkg(const std::string &taskId, const std::string &srcPath,
    const std::string &dstPath, const uint32_t type)
{
    LOG(INFO) << "AccDecompressAndVerifyPkg";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->AccDecompressAndVerifyPkg(taskId, srcPath, dstPath, type);
    LOG(INFO) << "AccDecompressAndVerifyPkg ret:" << ret;
#ifdef UPDATER_UT
    return -1;
#else
    return ret;
#endif
}

int32_t SysInstallerKitsImpl::AccDeleteDir(const std::string &taskId, const std::string &dstPath)
{
    LOG(INFO) << "AccDeleteDir";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->AccDeleteDir(taskId, dstPath);
    LOG(INFO) << "AccDeleteDir ret:" << ret;
#ifdef UPDATER_UT
    return -1;
#else
    return ret;
#endif
}

int32_t SysInstallerKitsImpl::CancelUpdateVabPackageZip(const std::string &taskId)
{
    LOG(INFO) << "CancelUpdateVabPackageZip";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->CancelUpdateVabPackageZip(taskId);
    LOG(INFO) << "CancelUpdateVabPackageZip ret:" << ret;
    return ret;
}

int32_t SysInstallerKitsImpl::StartUpdateVabPackageZip(const std::string &taskId,
    const std::vector<std::string> &pkgPath)
{
    LOG(INFO) << "StartUpdateVabPackageZip";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->StartUpdateVabPackageZip(taskId, pkgPath);
    LOG(INFO) << "StartUpdateVabPackageZip ret:" << ret;
#ifdef UPDATER_UT
    return -1;
#else
    return ret;
#endif
}

int32_t SysInstallerKitsImpl::StartUpdateSingularPackageZip(const std::string &taskId,
    const std::vector<std::string> &pkgPath)
{
    LOG(INFO) << "StartUpdateSingularPackageZip";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->StartUpdateSingularPackageZip(taskId, pkgPath);
    LOG(INFO) << "StartUpdateSingularPackageZip ret:" << ret;
#ifdef UPDATER_UT
    return -1;
#else
    return ret;
#endif
}

int32_t SysInstallerKitsImpl::CreateVabSnapshotCowImg(const std::unordered_map<std::string, uint64_t> &partitionInfo)
{
    LOG(INFO) << "CreateVabSnapshotCowImg";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->CreateVabSnapshotCowImg(partitionInfo);
    LOG(INFO) << "CreateVabSnapshotCowImg ret:" << ret;
#ifdef UPDATER_UT
    return -1;
#else
    return ret;
#endif
}

int32_t SysInstallerKitsImpl::CreateVabSnapshotCowImg(const std::string &name, uint64_t size, uint64_t splitSize,
    uint64_t &createdSize, bool &isCreated)
{
    LOG(INFO) << "CreateVabSnapshotCowImg";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->CreateVabSnapshotCowImg(name, size, splitSize, createdSize, isCreated);
    LOG(INFO) << "CreateVabSnapshotCowImg ret:" << ret;
#ifdef UPDATER_UT
    return -1;
#else
    return ret;
#endif
}

int32_t SysInstallerKitsImpl::GetPartitionAvailableSize(const std::map<std::string, uint64_t>& dtsCowsSize,
    const std::map<std::string, uint64_t>& dtsImgsSize, uint64_t& availSize)
{
    LOG(INFO) << "GetPartitionAvailableSize";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->GetPartitionAvailableSize(dtsCowsSize, dtsImgsSize, availSize);
    LOG(INFO) << "GetPartitionAvailableSize ret:" << ret;
#ifdef UPDATER_UT
    return -1;
#else
    return ret;
#endif
}

int32_t SysInstallerKitsImpl::StartVabMerge(const std::string &taskId)
{
    LOG(INFO) << "StartVabMerge";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->StartVabMerge(taskId);
    LOG(INFO) << "StartVabMerge ret:" << ret;
#ifdef UPDATER_UT
    return -1;
#else
    return ret;
#endif
}

int32_t SysInstallerKitsImpl::ClearVabMetadataAndCow()
{
    LOG(INFO) << "ClearVabMetadataAndCow";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->ClearVabMetadataAndCow();
    LOG(INFO) << "ClearVabMetadataAndCow ret:" << ret;
#ifdef UPDATER_UT
    return -1;
#else
    return ret;
#endif
}

int32_t SysInstallerKitsImpl::VabUpdateActive()
{
    LOG(INFO) << "VabUpdateActive";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->VabUpdateActive();
    LOG(INFO) << "VabUpdateActive ret:" << ret;
#ifdef UPDATER_UT
    return -1;
#else
    return ret;
#endif
}

int32_t SysInstallerKitsImpl::GetMetadataResult(const std::string &action, bool &result)
{
    LOG(INFO) << "GetMetadataResult";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->GetMetadataResult(action, result);
    LOG(INFO) << "GetMetadataResult ret:" << ret;
#ifdef UPDATER_UT
    return -1;
#else
    return ret;
#endif
}

std::string SysInstallerKitsImpl::GetUpdateResult(const std::string &taskId, const std::string &taskType,
    const std::string &resultType)
{
    LOG(INFO) << "GetUpdateResult";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return std::string("");
    }
    std::string updateResult;
    int32_t ret = updateService->GetUpdateResult(taskId, taskType, resultType, updateResult);
    if (ret != 0) {
        return std::string("");
    }
    return updateResult;
}

int32_t SysInstallerKitsImpl::InstallCloudRom(const std::string &taskId,
    InstallMode installMode, const std::vector<FeatureInfo> &featureInfos, RebootStatus rebootStatus)
{
    LOG(INFO) << "InstallCloudRom";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    return updateService->InstallCloudRom(taskId, installMode, featureInfos, rebootStatus);
}

int32_t SysInstallerKitsImpl::UninstallCloudRom(const std::string &taskId,
    const std::vector<FeatureInfo> &featureInfos, RebootStatus rebootStatus)
{
    LOG(INFO) << "UninstallCloudRom";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    return updateService->UninstallCloudRom(taskId, featureInfos, rebootStatus);
}

int32_t SysInstallerKitsImpl::GetFeatureStatus(const std::vector<FeatureInfo> &featureInfos,
    std::vector<FeatureStatus> &statusInfos)
{
    LOG(INFO) << "GetFeatureStatus";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    return updateService->GetFeatureStatus(featureInfos, statusInfos);
}

int32_t SysInstallerKitsImpl::GetAllFeatureStatus(const std::string &baseVersion,
    std::vector<FeatureStatus> &statusInfos)
{
    LOG(INFO) << "GetAllFeatureStatus";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    return updateService->GetAllFeatureStatus(baseVersion, statusInfos);
}

int32_t SysInstallerKitsImpl::ClearCloudRom(const std::string &baseVersion,
    const std::string &featureName)
{
    LOG(INFO) << "ClearCloudRom";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    return updateService->ClearCloudRom(baseVersion, featureName);
}

int32_t SysInstallerKitsImpl::UpdateCloudRomVersion(const std::string &baseVersion)
{
    LOG(INFO) << "UpdateCloudRomVersion";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    return updateService->UpdateCloudRomVersion(baseVersion);
}

int32_t SysInstallerKitsImpl::ExitSysInstaller()
{
    LOG(INFO) << "ExitSysInstaller";
    sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        LOG(ERROR) << "Get samgr failed";
        return -1;
    }
    bool isExist = false;
    sptr<IRemoteObject> object = samgr->CheckSystemAbility(SYS_INSTALLER_DISTRIBUTED_SERVICE_ID, isExist);
    if (!isExist) {
        LOG(ERROR) << "sys_installer not exist";
        return 0;
    }

    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->ExitSysInstaller();
    LOG(INFO) << "ExitSysInstaller ret:" << ret;
#ifdef UPDATER_UT
    return -1;
#else
    return ret;
#endif
}

int32_t SysInstallerKitsImpl::StartAbSync()
{
    LOG(INFO) << "StartAbSync";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->StartAbSync();
    LOG(INFO) << "StartAbSync ret:" << ret;
#ifdef UPDATER_UT
    return -1;
#else
    return ret;
#endif
}

int32_t SysInstallerKitsImpl::SetCpuAffinity(const std::string &taskId, unsigned int reservedCores)
{
    LOG(INFO) << "SetCpuAffinity taskId:" << taskId << ", reservedCores:" << reservedCores;
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    uint32_t reservedCpus = reservedCores;
    int32_t ret = updateService->SetCpuAffinity(taskId, reservedCpus);
    LOG(INFO) << "SetCpuAffinity ret:" << ret;
    return ret;
}

void SysInstallerKitsImpl::LoadServiceSuccess()
{
    serviceCv_.notify_all();
}

void SysInstallerKitsImpl::LoadServiceFail()
{
    serviceCv_.notify_all();
}

int32_t SysInstallerKitsImpl::ClearVabPatch()
{
    LOG(INFO) << "ClearVabPatch";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->ClearVabPatch();
    LOG(INFO) << "ClearVabPatch ret:" << ret;
#ifdef UPDATER_UT
    return -1;
#else
    return ret;
#endif
}
}
} // namespace OHOS
