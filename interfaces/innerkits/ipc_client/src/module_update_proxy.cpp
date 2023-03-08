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

#include "module_update_proxy.h"

#include "log/log.h"
#include "module_ipc_helper.h"
#include "string_ex.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

int32_t ModuleUpdateProxy::InstallModulePackage(const std::string &pkgPath)
{
    LOG(INFO) << "InstallModulePackage " << pkgPath;
    auto remote = Remote();
    if (remote == nullptr) {
        LOG(ERROR) << "Can not get remote";
        return ERR_FLATTEN_OBJECT;
    }

    MessageParcel data;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        LOG(ERROR) << "WriteInterfaceToken error";
        return ERR_FLATTEN_OBJECT;
    }
    data.WriteString16(Str8ToStr16(pkgPath));

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(INSTALL_MODULE_PACKAGE, data, reply, option);
    if (ret != ERR_OK) {
        LOG(ERROR) << "SendRequest error";
        return ERR_FLATTEN_OBJECT;
    }

    return reply.ReadInt32();
}

int32_t ModuleUpdateProxy::UninstallModulePackage(const std::string &hmpName)
{
    LOG(INFO) << "UninstallModulePackage " << hmpName;
    auto remote = Remote();
    if (remote == nullptr) {
        LOG(ERROR) << "Can not get remote";
        return ERR_FLATTEN_OBJECT;
    }

    MessageParcel data;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        LOG(ERROR) << "WriteInterfaceToken error";
        return ERR_FLATTEN_OBJECT;
    }
    data.WriteString16(Str8ToStr16(hmpName));

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(UNINSTALL_MODULE_PACKAGE, data, reply, option);
    if (ret != ERR_OK) {
        LOG(ERROR) << "SendRequest error";
        return ERR_FLATTEN_OBJECT;
    }

    return reply.ReadInt32();
}

int32_t ModuleUpdateProxy::GetModulePackageInfo(const std::string &hmpName,
    std::list<ModulePackageInfo> &modulePackageInfos)
{
    LOG(INFO) << "GetModulePackageInfo " << hmpName;
    auto remote = Remote();
    if (remote == nullptr) {
        LOG(ERROR) << "Can not get remote";
        return ERR_FLATTEN_OBJECT;
    }

    MessageParcel data;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        LOG(ERROR) << "WriteInterfaceToken error";
        return ERR_FLATTEN_OBJECT;
    }
    data.WriteString16(Str8ToStr16(hmpName));

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(GET_MODULE_PACKAGE_INFO, data, reply, option);
    if (ret != ERR_OK) {
        LOG(ERROR) << "SendRequest error";
        return ERR_FLATTEN_OBJECT;
    }

    ModuleIpcHelper::ReadModulePackageInfos(reply, modulePackageInfos);
    return reply.ReadInt32();
}

int32_t ModuleUpdateProxy::ReportModuleUpdateStatus(const ModuleUpdateStatus &status)
{
    LOG(INFO) << "ReportModuleUpdateStatus process=" << status.process;
    auto remote = Remote();
    if (remote == nullptr) {
        LOG(ERROR) << "Can not get remote";
        return ERR_FLATTEN_OBJECT;
    }

    MessageParcel data;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        LOG(ERROR) << "WriteInterfaceToken error";
        return ERR_FLATTEN_OBJECT;
    }
    ModuleIpcHelper::WriteModuleUpdateStatus(data, status);

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(REPORT_MODULE_UPDATE_STATUS, data, reply, option);
    if (ret != ERR_OK) {
        LOG(ERROR) << "SendRequest error";
        return ERR_FLATTEN_OBJECT;
    }

    return reply.ReadInt32();
}

int32_t ModuleUpdateProxy::ExitModuleUpdate()
{
    LOG(INFO) << "ExitModuleUpdate";
    auto remote = Remote();
    if (remote == nullptr) {
        LOG(ERROR) << "Can not get remote";
        return ERR_FLATTEN_OBJECT;
    }

    MessageParcel data;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        LOG(ERROR) << "WriteInterfaceToken error";
        return ERR_FLATTEN_OBJECT;
    }

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(EXIT_MODULE_UPDATE, data, reply, option);
    if (ret != ERR_OK) {
        LOG(ERROR) << "SendRequest error";
        return ERR_FLATTEN_OBJECT;
    }

    return reply.ReadInt32();
}
} // namespace SysInstaller
} // namespace OHOS
