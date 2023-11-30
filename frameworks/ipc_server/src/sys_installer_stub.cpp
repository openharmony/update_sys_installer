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

#include "sys_installer_stub.h"

#include <string_ex.h>

#include "accesstoken_kit.h"
#include "hap_token_info.h"
#include "ipc_skeleton.h"
#include "log/log.h"
#include "securec.h"
#include "sys_installer_sa_ipc_interface_code.h"
#include "utils.h"

namespace OHOS {
namespace SysInstaller {
using namespace std;
using namespace Updater;
using namespace std::placeholders;

SysInstallerStub::SysInstallerStub()
{
    requestFuncMap_.emplace(SysInstallerInterfaceCode::SYS_INSTALLER_INIT,
        bind(&SysInstallerStub::SysInstallerInitStub, this, _1, _2, _3, _4));
    requestFuncMap_.emplace(SysInstallerInterfaceCode::UPDATE_PACKAGE,
        bind(&SysInstallerStub::StartUpdatePackageZipStub, this, _1, _2, _3, _4));
    requestFuncMap_.emplace(SysInstallerInterfaceCode::SET_UPDATE_CALLBACK,
        bind(&SysInstallerStub::SetUpdateCallbackStub, this, _1, _2, _3, _4));
    requestFuncMap_.emplace(SysInstallerInterfaceCode::GET_UPDATE_STATUS,
        bind(&SysInstallerStub::GetUpdateStatusStub, this, _1, _2, _3, _4));
    requestFuncMap_.emplace(SysInstallerInterfaceCode::UPDATE_PARA_PACKAGE,
        bind(&SysInstallerStub::StartUpdateParaZipStub, this, _1, _2, _3, _4));
    requestFuncMap_.emplace(SysInstallerInterfaceCode::DELETE_PARA_PACKAGE,
        bind(&SysInstallerStub::StartDeleteParaZipStub, this, _1, _2, _3, _4));
    requestFuncMap_.emplace(SysInstallerInterfaceCode::DECOMPRESS_ACC_PACKAGE,
        bind(&SysInstallerStub::AccDecompressAndVerifyPkgStub, this, _1, _2, _3, _4));
    requestFuncMap_.emplace(SysInstallerInterfaceCode::DELETE_ACC_PACKAGE,
        bind(&SysInstallerStub::AccDeleteDirStub, this, _1, _2, _3, _4));
}

SysInstallerStub::~SysInstallerStub()
{
    requestFuncMap_.clear();
}

int32_t SysInstallerStub::SysInstallerInitStub(SysInstallerStub *service,
    MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (service == nullptr) {
        LOG(ERROR) << "Invalid param";
        return -1;
    }
    int ret = service->SysInstallerInit();
    reply.WriteInt32(ret);
    return 0;
}

int32_t SysInstallerStub::StartUpdatePackageZipStub(SysInstallerStub *service,
    MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (service == nullptr) {
        LOG(ERROR) << "Invalid param";
        return -1;
    }
    string pkgPath = Str16ToStr8(data.ReadString16());
    int ret = service->StartUpdatePackageZip(pkgPath);
    reply.WriteInt32(ret);
    return 0;
}

int32_t SysInstallerStub::SetUpdateCallbackStub(SysInstallerStub *service,
    MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (service == nullptr) {
        LOG(ERROR) << "Invalid param";
        return -1;
    }
    auto remote = data.ReadRemoteObject();
    if (remote == nullptr) {
        LOG(ERROR) << "Failed to read remote obj";
        return -1;
    }
    int ret = service->SetUpdateCallback(iface_cast<ISysInstallerCallback>(remote));
    reply.WriteInt32(ret);
    return 0;
}

int32_t SysInstallerStub::GetUpdateStatusStub(SysInstallerStub *service,
    MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (service == nullptr) {
        LOG(ERROR) << "Invalid param";
        return -1;
    }
    int32_t ret = service->GetUpdateStatus();
    reply.WriteInt32(ret);
    return 0;
}

int32_t SysInstallerStub::StartUpdateParaZipStub(SysInstallerStub *service,
    MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (service == nullptr) {
        LOG(ERROR) << "Invalid param";
        return -1;
    }
    string pkgPath = Str16ToStr8(data.ReadString16());
    string location = Str16ToStr8(data.ReadString16());
    string cfgDir = Str16ToStr8(data.ReadString16());
    LOG(INFO) << "StartUpdateParaZipStub path:" << pkgPath << " location:" << location << " cfgDir:" << cfgDir;

    int32_t ret = service->StartUpdateParaZip(pkgPath, location, cfgDir);
    reply.WriteInt32(ret);
    return 0;
}

int32_t SysInstallerStub::StartDeleteParaZipStub(SysInstallerStub *service,
    MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (service == nullptr) {
        LOG(ERROR) << "Invalid param";
        return -1;
    }
    string location = Str16ToStr8(data.ReadString16());
    string cfgDir = Str16ToStr8(data.ReadString16());
    LOG(INFO) << "StartDeleteParaZipStub location:" << location << " cfgDir:" << cfgDir;

    int32_t ret = service->StartDeleteParaZip(location, cfgDir);
    reply.WriteInt32(ret);
    return 0;
}

int32_t SysInstallerStub::AccDecompressAndVerifyPkgStub(SysInstallerStub *service,
    MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (service == nullptr) {
        LOG(ERROR) << "Invalid param";
        return -1;
    }
    string srcPath = Str16ToStr8(data.ReadString16());
    string dstPath = Str16ToStr8(data.ReadString16());
    uint32_t type = static_cast<uint32_t>(data.ReadInt32());
    LOG(INFO) << "StartUpdateParaZipStub srcPath:" << srcPath << " dstPath:" << dstPath;

    int32_t ret = service->AccDecompressAndVerifyPkg(srcPath, dstPath, type);
    reply.WriteInt32(ret);
    return 0;
}

int32_t SysInstallerStub::AccDeleteDirStub(SysInstallerStub *service,
    MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (service == nullptr) {
        LOG(ERROR) << "Invalid param";
        return -1;
    }
    string dstPath = Str16ToStr8(data.ReadString16());
    LOG(INFO) << "AccDeleteDirStub dstPath:" << dstPath;

    int32_t ret = service->AccDeleteDir(dstPath);
    reply.WriteInt32(ret);
    return 0;
}

bool SysInstallerStub::IsPermissionGranted(void)
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

bool SysInstallerStub::CheckCallingPerm(void)
{
    int32_t callingUid = OHOS::IPCSkeleton::GetCallingUid();
    LOG(INFO) << "CheckCallingPerm callingUid:" << callingUid;
    if (callingUid == 0) {
        return true;
    }
    return callingUid == Updater::Utils::USER_UPDATE_AUTHORITY && IsPermissionGranted();
}

int32_t SysInstallerStub::OnRemoteRequest(uint32_t code,
    MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (data.ReadInterfaceToken() != GetDescriptor()) {
        LOG(ERROR) << "SysInstallerStub ReadInterfaceToken fail";
        return -1;
    }

    LOG(INFO) << "OnRemoteRequest func code " << code;
    auto inter = requestFuncMap_.find(code);
    if (inter != requestFuncMap_.end()) {
        if (!CheckCallingPerm()) {
            LOG(ERROR) << "SysInstallerStub CheckCallingPerm fail";
            return -1;
        }
        return inter->second(this, data, reply, option);
    }

    LOG(INFO) << "UpdateServiceStub OnRemoteRequest code " << code << "not found";
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}
} // namespace SysInstaller
} // namespace OHOS