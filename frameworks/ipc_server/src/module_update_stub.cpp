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

#include "module_update_stub.h"

#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "isys_installer_callback.h"
#include "sys_installer_sa_ipc_interface_code.h"

#include "log/log.h"
#include "string_ex.h"

constexpr int USER_UPDATE_AUTHORITY = 6666;

namespace OHOS {
namespace SysInstaller {
using namespace std;
using namespace Updater;
using namespace std::placeholders;

ModuleUpdateStub::ModuleUpdateStub()
{
    requestFuncMap_.emplace(ModuleUpdateInterfaceCode::INSTALL_MODULE_PACKAGE,
        bind(&ModuleUpdateStub::InstallModulePackageStub, this, _1, _2, _3, _4));
    requestFuncMap_.emplace(ModuleUpdateInterfaceCode::UNINSTALL_MODULE_PACKAGE,
        bind(&ModuleUpdateStub::UninstallModulePackageStub, this, _1, _2, _3, _4));
    requestFuncMap_.emplace(ModuleUpdateInterfaceCode::GET_MODULE_PACKAGE_INFO,
        bind(&ModuleUpdateStub::GetModulePackageInfoStub, this, _1, _2, _3, _4));
    requestFuncMap_.emplace(ModuleUpdateInterfaceCode::REPORT_MODULE_UPDATE_STATUS,
        bind(&ModuleUpdateStub::ReportModuleUpdateStatusStub, this, _1, _2, _3, _4));
    requestFuncMap_.emplace(ModuleUpdateInterfaceCode::EXIT_MODULE_UPDATE,
        bind(&ModuleUpdateStub::ExitModuleUpdateStub, this, _1, _2, _3, _4));

    requestFuncMap_.emplace(ModuleUpdateInterfaceCode::GET_HMP_VERSION_INFO,
        bind(&ModuleUpdateStub::GetHmpVersionInfoStub, this, _1, _2, _3, _4));
    requestFuncMap_.emplace(ModuleUpdateInterfaceCode::START_UPDATE_HMP_PACKAGE,
        bind(&ModuleUpdateStub::StartUpdateHmpPackageStub, this, _1, _2, _3, _4));
    requestFuncMap_.emplace(ModuleUpdateInterfaceCode::GET_HMP_UPDATE_RESULT,
        bind(&ModuleUpdateStub::GetHmpUpdateResultStub, this, _1, _2, _3, _4));
}

ModuleUpdateStub::~ModuleUpdateStub()
{
    requestFuncMap_.clear();
}

int32_t ModuleUpdateStub::InstallModulePackageStub(ModuleUpdateStub *service,
    MessageParcel &data, MessageParcel &reply, MessageOption &option) const
{
    if (service == nullptr) {
        LOG(ERROR) << "Invalid param";
        return -1;
    }
    string pkgPath = Str16ToStr8(data.ReadString16());
    int32_t ret = service->InstallModulePackage(pkgPath);
    reply.WriteInt32(ret);
    return 0;
}

int32_t ModuleUpdateStub::UninstallModulePackageStub(ModuleUpdateStub *service,
    MessageParcel &data, MessageParcel &reply, MessageOption &option) const
{
    if (service == nullptr) {
        LOG(ERROR) << "Invalid param";
        return -1;
    }
    string hmpName = Str16ToStr8(data.ReadString16());
    int32_t ret = service->UninstallModulePackage(hmpName);
    reply.WriteInt32(ret);
    return 0;
}

int32_t ModuleUpdateStub::GetModulePackageInfoStub(ModuleUpdateStub *service,
    MessageParcel &data, MessageParcel &reply, MessageOption &option) const
{
    if (service == nullptr) {
        LOG(ERROR) << "Invalid param";
        return -1;
    }
    string hmpName = Str16ToStr8(data.ReadString16());
    std::list<ModulePackageInfo> infos;
    int32_t ret = service->GetModulePackageInfo(hmpName, infos);
    ModuleIpcHelper::WriteModulePackageInfos(reply, infos);
    reply.WriteInt32(ret);
    return 0;
}

int32_t ModuleUpdateStub::ReportModuleUpdateStatusStub(ModuleUpdateStub *service,
    MessageParcel &data, MessageParcel &reply, MessageOption &option) const
{
    if (service == nullptr) {
        LOG(ERROR) << "Invalid param";
        return -1;
    }
    ModuleUpdateStatus status;
    ModuleIpcHelper::ReadModuleUpdateStatus(data, status);
    int32_t ret = service->ReportModuleUpdateStatus(status);
    reply.WriteInt32(ret);
    return 0;
}

int32_t ModuleUpdateStub::ExitModuleUpdateStub(ModuleUpdateStub *service,
    MessageParcel &data, MessageParcel &reply, MessageOption &option) const
{
    if (service == nullptr) {
        LOG(ERROR) << "Invalid param";
        return -1;
    }
    int32_t ret = service->ExitModuleUpdate();
    reply.WriteInt32(ret);
    return 0;
}

int32_t ModuleUpdateStub::GetHmpVersionInfoStub(ModuleUpdateStub *service,
    MessageParcel &data, MessageParcel &reply, MessageOption &option) const
{
    if (service == nullptr) {
        LOG(ERROR) << "Invalid param";
        return -1;
    }
    std::vector<HmpVersionInfo> versionInfo = service->GetHmpVersionInfo();
    reply.WriteInt32(versionInfo.size());
    for (auto &info : versionInfo) {
        reply.WriteParcelable(&info);
    }
    return 0;
}

int32_t ModuleUpdateStub::StartUpdateHmpPackageStub(ModuleUpdateStub *service,
    MessageParcel &data, MessageParcel &reply, MessageOption &option) const
{
    if (service == nullptr) {
        LOG(ERROR) << "Invalid param";
        return -1;
    }

    sptr<IRemoteObject> object = data.ReadRemoteObject();
    if (object == nullptr) {
        LOG(ERROR) << "object null";
        return -1;
    }

    sptr<ISysInstallerCallback> updateCallback = iface_cast<ISysInstallerCallback>(object);
    if (updateCallback == nullptr) {
        LOG(ERROR) << "ISysInstallerCallback updateCallback is null";
        return ERR_NULL_OBJECT;
    }
    std::string path = Str16ToStr8(data.ReadString16());
    LOG(ERROR) << "StartUpdateHmpPackageStub path:" << path;

    int32_t ret = service->StartUpdateHmpPackage(path, updateCallback);
    reply.WriteInt32(ret);
    return 0;
}

int32_t ModuleUpdateStub::GetHmpUpdateResultStub(ModuleUpdateStub *service,
    MessageParcel &data, MessageParcel &reply, MessageOption &option) const
{
    if (service == nullptr) {
        LOG(ERROR) << "Invalid param";
        return -1;
    }
    std::vector<HmpUpdateInfo> updateInfo = service->GetHmpUpdateResult();
    reply.WriteInt32(updateInfo.size());
    for (auto &info : updateInfo) {
        reply.WriteParcelable(&info);
    }
    return 0;
}

bool ModuleUpdateStub::IsPermissionGranted(void)
{
    Security::AccessToken::AccessTokenID callerToken = IPCSkeleton::GetCallingTokenID();
    std::string permission = "ohos.permission.UPDATE_SYSTEM";

    int verifyResult = Security::AccessToken::AccessTokenKit::VerifyAccessToken(callerToken, permission);
    bool isPermissionGranted = (verifyResult == Security::AccessToken::PERMISSION_GRANTED);
    if (!isPermissionGranted) {
        LOG(ERROR) << "not granted " << permission.c_str();
    }
    return isPermissionGranted;
}

bool ModuleUpdateStub::CheckCallingPerm(void)
{
    int32_t callingUid = OHOS::IPCSkeleton::GetCallingUid();
    LOG(INFO) << "CheckCallingPerm callingUid:" << callingUid;
    if (callingUid == 0) {
        return true;
    }
    return callingUid == USER_UPDATE_AUTHORITY && IsPermissionGranted();
}

int32_t ModuleUpdateStub::OnRemoteRequest(uint32_t code,
    MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (data.ReadInterfaceToken() != GetDescriptor()) {
        LOG(ERROR) << "ModuleUpdateStub ReadInterfaceToken fail";
        return -1;
    }

    LOG(INFO) << "OnRemoteRequest func code " << code;
    auto inter = requestFuncMap_.find(code);
    if (inter != requestFuncMap_.end()) {
        if (!CheckCallingPerm()) {
            LOG(ERROR) << "ModuleUpdateStub CheckCallingPerm fail";
            return -1;
        }
        return inter->second(this, data, reply, option);
    }

    LOG(INFO) << "ModuleUpdateStub OnRemoteRequest code " << code << "not found";
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}
} // namespace SysInstaller
} // namespace OHOS