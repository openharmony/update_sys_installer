/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "module_update_kits_impl.h"

#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "log/log.h"
#include "module_error_code.h"
#include "module_update_load_callback.h"
#include "module_update_proxy.h"
#include "service_control.h"
#include "system_ability_definition.h"
#include "sys_installer_callback.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

namespace {
constexpr int LOAD_SA_TIMEOUT_MS = 3;
static volatile std::atomic_long g_request(0);
}

ModuleUpdateKits &ModuleUpdateKits::GetInstance()
{
    return DelayedRefSingleton<ModuleUpdateKitsImpl>::GetInstance();
}

ModuleUpdateKitsImpl::ModuleUpdateKitsImpl() {}

ModuleUpdateKitsImpl::~ModuleUpdateKitsImpl() {}

void ModuleUpdateKitsImpl::ResetService(const wptr<IRemoteObject>& remote)
{
    LOG(INFO) << "Remote is dead, reset service instance";

    std::lock_guard<std::mutex> lock(moduleUpdateLock_);
    if (moduleUpdate_ != nullptr) {
        sptr<IRemoteObject> object = moduleUpdate_->AsObject();
        if ((object != nullptr) && (remote == object)) {
            object->RemoveDeathRecipient(deathRecipient_);
            moduleUpdate_ = nullptr;
        }
    }
}

sptr<IModuleUpdate> ModuleUpdateKitsImpl::GetService()
{
    std::lock_guard<std::mutex> lock(moduleUpdateLock_);
    if (moduleUpdate_ != nullptr) {
        return moduleUpdate_;
    }

    sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        LOG(ERROR) << "Get samgr object failed";
        return nullptr;
    }
    sptr<IRemoteObject> object = samgr->GetSystemAbility(MODULE_UPDATE_SERVICE_ID);
    if (object == nullptr) {
        LOG(ERROR) << "Get module update object from samgr failed";
        return nullptr;
    }

    if (deathRecipient_ == nullptr) {
        deathRecipient_ = new DeathRecipient();
    }

    if ((object->IsProxyObject()) && (!object->AddDeathRecipient(deathRecipient_))) {
        LOG(ERROR) << "Failed to add death recipient";
    }

    LOG(INFO) << "get remote object ok";
    moduleUpdate_ = iface_cast<IModuleUpdate>(object);
    if (moduleUpdate_ == nullptr) {
        LOG(ERROR) << "module update object iface_cast failed";
        return nullptr;
    }
    return moduleUpdate_;
}

void ModuleUpdateKitsImpl::DeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &remote)
{
    DelayedRefSingleton<ModuleUpdateKitsImpl>::GetInstance().ResetService(remote);
}

int32_t ModuleUpdateKitsImpl::InstallModulePackage(const std::string &pkgPath)
{
    LOG(INFO) << "InstallModulePackage " << pkgPath;
    auto moduleUpdate = GetService();
    if (moduleUpdate == nullptr) {
        LOG(ERROR) << "Get moduleUpdate failed";
        return ModuleErrorCode::ERR_SERVICE_NOT_FOUND;
    }
    return moduleUpdate->InstallModulePackage(pkgPath);
}

int32_t ModuleUpdateKitsImpl::UninstallModulePackage(const std::string &hmpName)
{
    LOG(INFO) << "UninstallModulePackage " << hmpName;
    auto moduleUpdate = GetService();
    if (moduleUpdate == nullptr) {
        LOG(ERROR) << "Get moduleUpdate failed";
        return ModuleErrorCode::ERR_SERVICE_NOT_FOUND;
    }
    return moduleUpdate->UninstallModulePackage(hmpName);
}

int32_t ModuleUpdateKitsImpl::GetModulePackageInfo(const std::string &hmpName,
    std::list<ModulePackageInfo> &modulePackageInfos)
{
    LOG(INFO) << "GetModulePackageInfo";
    auto moduleUpdate = GetService();
    if (moduleUpdate == nullptr) {
        LOG(ERROR) << "Get moduleUpdate failed";
        return ModuleErrorCode::ERR_SERVICE_NOT_FOUND;
    }
    return moduleUpdate->GetModulePackageInfo(hmpName, modulePackageInfos);
}

int32_t ModuleUpdateKitsImpl::ExitModuleUpdate()
{
    LOG(INFO) << "ExitModuleUpdate, g_request = " << g_request;
    auto moduleUpdate = GetService();
    if (moduleUpdate == nullptr) {
        LOG(ERROR) << "Get moduleUpdate failed";
        return ModuleErrorCode::ERR_SERVICE_NOT_FOUND;
    }
    if (--g_request <= 0) {
        return moduleUpdate->ExitModuleUpdate();
    }
    return 0;
}

int32_t ModuleUpdateKitsImpl::Init()
{
    std::lock_guard<std::mutex> lock(moduleUpdateLock_);
    if (moduleUpdate_ != nullptr) {
        LOG(INFO) << "already init";
        return 0;
    }

    LOG(INFO) << "InitModuleUpdate Init start";
    sptr<ModuleUpdateLoadCallback> loadCallback_ = new ModuleUpdateLoadCallback();
    sptr<ISystemAbilityManager> sm = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (sm == nullptr) {
        LOG(ERROR) << "GetSystemAbilityManager samgr object null";
        return -1;
    }
    LOG(INFO) << "InitModuleUpdate Init GetSystemAbilityManager done";
    int32_t result = sm->LoadSystemAbility(MODULE_UPDATE_SERVICE_ID, loadCallback_);
    if (result != ERR_OK) {
        LOG(ERROR) << "systemAbilityId " << MODULE_UPDATE_SERVICE_ID <<
            " load failed, result code:" << result;
        return -1;
    }

    LOG(INFO) << "InitModuleUpdate Init Load done";
    std::unique_lock<std::mutex> callbackLock(serviceMutex_);
    serviceCv_.wait_for(callbackLock, std::chrono::seconds(LOAD_SA_TIMEOUT_MS));
    return 0;
}

int32_t ModuleUpdateKitsImpl::InitModuleUpdate()
{
    InitUpdaterLogger("ModuleUpdaterClient", "", "", "");
    LOG(INFO) << "InitModuleUpdate";
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
    g_request++;
    return ModuleErrorCode::MODULE_UPDATE_SUCCESS;
}

std::vector<HmpVersionInfo> ModuleUpdateKitsImpl::GetHmpVersionInfo()
{
    LOG(INFO) << "GetHmpVersionInfo";
    std::vector<HmpVersionInfo> versionInfo {};
    auto moduleUpdate = GetService();
    if (moduleUpdate == nullptr) {
        LOG(ERROR) << "Get moduleUpdate failed";
        return versionInfo;
    }
    versionInfo = moduleUpdate->GetHmpVersionInfo();
    return versionInfo;
}

int32_t ModuleUpdateKitsImpl::StartUpdateHmpPackage(const std::string &path,
    sptr<ISysInstallerCallbackFunc> callback)
{
    LOG(INFO) << "StartUpdateHmpPackage";
    if (callback == nullptr) {
        LOG(ERROR) << "callback null";
        return ModuleErrorCode::ERR_SERVICE_PARA_ERROR;
    }

    auto moduleUpdate = GetService();
    if (moduleUpdate == nullptr) {
        LOG(ERROR) << "Get moduleUpdate failed";
        return ModuleErrorCode::ERR_SERVICE_NOT_FOUND;
    }

    if (updateCallBack_ == nullptr) {
        updateCallBack_ = new SysInstallerCallback;
    }
    static_cast<SysInstallerCallback *>(updateCallBack_.GetRefPtr())->RegisterCallback(callback);

    return moduleUpdate->StartUpdateHmpPackage(path, updateCallBack_);
}

std::vector<HmpUpdateInfo> ModuleUpdateKitsImpl::GetHmpUpdateResult()
{
    LOG(INFO) << "GetHmpUpdateResult";
    std::vector<HmpUpdateInfo> updateInfo {};
    auto moduleUpdate = GetService();
    if (moduleUpdate == nullptr) {
        LOG(ERROR) << "Get moduleUpdate failed";
        return updateInfo;
    }
    updateInfo = moduleUpdate->GetHmpUpdateResult();
    return updateInfo;
}

void ModuleUpdateKitsImpl::LoadServiceSuccess()
{
    serviceCv_.notify_all();
}

void ModuleUpdateKitsImpl::LoadServiceFail()
{
    serviceCv_.notify_all();
}
}
} // namespace OHOS
