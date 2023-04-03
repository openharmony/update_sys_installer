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

#include "log/log.h"
#include "string_ex.h"

namespace OHOS {
namespace SysInstaller {
using namespace std;
using namespace Updater;
using namespace std::placeholders;

ModuleUpdateStub::ModuleUpdateStub()
{
    requestFuncMap_.emplace(IModuleUpdate::INSTALL_MODULE_PACKAGE,
        bind(&ModuleUpdateStub::InstallModulePackageStub, this, _1, _2, _3, _4));
    requestFuncMap_.emplace(IModuleUpdate::UNINSTALL_MODULE_PACKAGE,
        bind(&ModuleUpdateStub::UninstallModulePackageStub, this, _1, _2, _3, _4));
    requestFuncMap_.emplace(IModuleUpdate::GET_MODULE_PACKAGE_INFO,
        bind(&ModuleUpdateStub::GetModulePackageInfoStub, this, _1, _2, _3, _4));
    requestFuncMap_.emplace(IModuleUpdate::REPORT_MODULE_UPDATE_STATUS,
        bind(&ModuleUpdateStub::ReportModuleUpdateStatusStub, this, _1, _2, _3, _4));
    requestFuncMap_.emplace(IModuleUpdate::EXIT_MODULE_UPDATE,
        bind(&ModuleUpdateStub::ExitModuleUpdateStub, this, _1, _2, _3, _4));
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
        return inter->second(this, data, reply, option);
    }

    LOG(INFO) << "ModuleUpdateStub OnRemoteRequest code " << code << "not found";
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}
} // namespace SysInstaller
} // namespace OHOS