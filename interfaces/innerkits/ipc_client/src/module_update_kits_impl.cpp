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
#include "module_update_proxy.h"
#include "service_control.h"
#include "system_ability_definition.h"
#include "sys_installer_callback.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

namespace {
constexpr const char *SERVICE_NAME = "module_update_service";
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

int32_t ModuleUpdateKitsImpl::ReportModuleUpdateStatus(const ModuleUpdateStatus &status)
{
    LOG(INFO) << "ReportModuleUpdateStatus process=" << status.process;
    auto moduleUpdate = GetService();
    if (moduleUpdate == nullptr) {
        LOG(ERROR) << "Get moduleUpdate failed";
        return ModuleErrorCode::ERR_SERVICE_NOT_FOUND;
    }
    return moduleUpdate->ReportModuleUpdateStatus(status);
}

int32_t ModuleUpdateKitsImpl::ExitModuleUpdate()
{
    LOG(INFO) << "ExitModuleUpdate";
    auto moduleUpdate = GetService();
    if (moduleUpdate == nullptr) {
        LOG(ERROR) << "Get moduleUpdate failed";
        return ModuleErrorCode::ERR_SERVICE_NOT_FOUND;
    }
    return moduleUpdate->ExitModuleUpdate();
}

int32_t ModuleUpdateKitsImpl::InitModuleUpdate()
{
    LOG(INFO) << "InitModuleUpdate";
    int ret = ServiceControl(SERVICE_NAME, ServiceAction::START);
    if (ret != 0) {
        LOG(ERROR) << "Failed to start service";
        return ModuleErrorCode::ERR_SERVICE_NOT_FOUND;
    }
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
}
} // namespace OHOS
