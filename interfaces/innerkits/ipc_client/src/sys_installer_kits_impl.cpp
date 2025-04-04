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

namespace OHOS {
namespace SysInstaller {
using namespace Updater;
constexpr int GROUP_UPDATE_AUTHORITY = 6666;
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
    (void)Utils::MkdirRecursive(SYS_LOG_DIR, 0777); // 0777 : rwxrwxrwx
    InitUpdaterLogger("SysInstallerClient", SYS_LOG_FILE, SYS_STAGE_FILE, SYS_ERROR_FILE);
    (void)chown(SYS_LOG_FILE, GROUP_UPDATE_AUTHORITY, GROUP_UPDATE_AUTHORITY);
    (void)chown(SYS_STAGE_FILE, GROUP_UPDATE_AUTHORITY, GROUP_UPDATE_AUTHORITY);
    (void)chown(SYS_ERROR_FILE, GROUP_UPDATE_AUTHORITY, GROUP_UPDATE_AUTHORITY);

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

int32_t SysInstallerKitsImpl::SysInstallerInit(bool bStreamUpgrade)
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
    updateService->SysInstallerInit(bStreamUpgrade);
    return 0;
}

int32_t SysInstallerKitsImpl::StartUpdatePackageZip(const std::string &pkgPath)
{
    LOG(INFO) << "StartUpdatePackageZip";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->StartUpdatePackageZip(pkgPath);
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

int32_t SysInstallerKitsImpl::ProcessStreamData(uint8_t *buffer, uint32_t size)
{
    LOG(INFO) << "ProcessStreamData";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    std::vector<uint8_t> vecBuffer(buffer, buffer + size);
    int32_t ret = updateService->ProcessStreamData(vecBuffer, size);
    LOG(INFO) << "ProcessStreamData ret:" << ret;
    return ret;
}

int32_t SysInstallerKitsImpl::SetUpdateCallback(sptr<ISysInstallerCallbackFunc> callback)
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

    if (updateCallBack_ == nullptr) {
        updateCallBack_ = new SysInstallerCallback;
    }
    static_cast<SysInstallerCallback *>(updateCallBack_.GetRefPtr())->RegisterCallback(callback);
    return updateService->SetUpdateCallback(updateCallBack_);
}

int32_t SysInstallerKitsImpl::GetUpdateStatus()
{
    LOG(INFO) << "GetUpdateStatus";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    return updateService->GetUpdateStatus();
}

int32_t SysInstallerKitsImpl::StartUpdateParaZip(const std::string &pkgPath,
    const std::string &location, const std::string &cfgDir)
{
    LOG(INFO) << "StartUpdateParaZip";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->StartUpdateParaZip(pkgPath, location, cfgDir);
    LOG(INFO) << "StartUpdateParaZip ret:" << ret;
#ifdef UPDATER_UT
    return -1;
#endif
    return ret;
}

int32_t SysInstallerKitsImpl::StartDeleteParaZip(const std::string &location, const std::string &cfgDir)
{
    LOG(INFO) << "StartDeleteParaZip";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->StartDeleteParaZip(location, cfgDir);
    LOG(INFO) << "StartDeleteParaZip ret:" << ret;
#ifdef UPDATER_UT
    return -1;
#endif
    return ret;
}

int32_t SysInstallerKitsImpl::AccDecompressAndVerifyPkg(const std::string &srcPath,
    const std::string &dstPath, const uint32_t type)
{
    LOG(INFO) << "AccDecompressAndVerifyPkg";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->AccDecompressAndVerifyPkg(srcPath, dstPath, type);
    LOG(INFO) << "AccDecompressAndVerifyPkg ret:" << ret;
#ifdef UPDATER_UT
    return -1;
#endif
    return ret;
}

int32_t SysInstallerKitsImpl::AccDeleteDir(const std::string &dstPath)
{
    LOG(INFO) << "AccDeleteDir";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->AccDeleteDir(dstPath);
    LOG(INFO) << "AccDeleteDir ret:" << ret;
#ifdef UPDATER_UT
    return -1;
#endif
    return ret;
}

int32_t SysInstallerKitsImpl::StartUpdateVabPackageZip(const std::vector<std::string> &pkgPath)
{
    LOG(INFO) << "StartUpdateVabPackageZip";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->StartUpdateVabPackageZip(pkgPath);
    LOG(INFO) << "StartUpdateVabPackageZip ret:" << ret;
    #ifdef UPDATER_UT
    return -1;
    #endif
    return ret;
}

int32_t SysInstallerKitsImpl::StartVabMerge()
{
    LOG(INFO) << "StartVabMerge";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->StartVabMerge();
    LOG(INFO) << "StartVabMerge ret:" << ret;
    #ifdef UPDATER_UT
    return -1;
    #endif
    return ret;
}

int32_t SysInstallerKitsImpl::EnableVabCheckpoint()
{
    LOG(INFO) << "EnableVabCheckpoint";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->EnableVabCheckpoint();
    LOG(INFO) << "EnableVabCheckpoint ret:" << ret;
    #ifdef UPDATER_UT
    return -1;
    #endif
    return ret;
}

int32_t SysInstallerKitsImpl::AbortVabActiveSnapshot()
{
    LOG(INFO) << "AbortVabActiveSnapshot";
    auto updateService = GetService();
    if (updateService == nullptr) {
        LOG(ERROR) << "Get updateService failed";
        return -1;
    }
    int32_t ret = updateService->AbortVabActiveSnapshot();
    LOG(INFO) << "AbortVabActiveSnapshot ret:" << ret;
    #ifdef UPDATER_UT
    return -1;
    #endif
    return ret;
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
    #endif
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
}
} // namespace OHOS
