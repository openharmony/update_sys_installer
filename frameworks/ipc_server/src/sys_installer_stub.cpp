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

#include "hap_token_info.h"
#include "ipc_skeleton.h"
#include "log/log.h"
#include "securec.h"

namespace OHOS {
namespace SysInstaller {
using namespace std;
using namespace Updater;
using namespace std::placeholders;

SysInstallerStub::SysInstallerStub()
{
    requestFuncMap_.emplace(ISysInstaller::SYS_INSTALLER_INIT,
        bind(&SysInstallerStub::SysInstallerInitStub, this, _1, _2, _3, _4));
    requestFuncMap_.emplace(ISysInstaller::UPDATE_PACKAGE,
        bind(&SysInstallerStub::StartUpdatePackageZipStub, this, _1, _2, _3, _4));
    requestFuncMap_.emplace(ISysInstaller::SET_UPDATE_CALLBACK,
        bind(&SysInstallerStub::SetUpdateCallbackStub, this, _1, _2, _3, _4));
    requestFuncMap_.emplace(ISysInstaller::GET_UPDATE_STATUS,
        bind(&SysInstallerStub::GetUpdateStatusStub, this, _1, _2, _3, _4));
    requestFuncMap_.emplace(ISysInstaller::UPDATE_PARA_PACKAGE,
        bind(&SysInstallerStub::StartUpdateParaZipStub, this, _1, _2, _3, _4));
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
        return inter->second(this, data, reply, option);
    }

    LOG(INFO) << "UpdateServiceStub OnRemoteRequest code " << code << "not found";
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}
} // namespace SysInstaller
} // namespace OHOS