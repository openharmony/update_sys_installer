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

#include "sys_installer_proxy.h"

#include <string_ex.h>

#include "log/log.h"
#include "securec.h"
#include "sys_installer_sa_ipc_interface_code.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

int32_t SysInstallerProxy::SysInstallerInit()
{
    LOG(INFO) << "SysInstallerInit";
    auto remote = Remote();
    if (remote == nullptr) {
        LOG(ERROR) << "Can not get remote";
        return ERR_FLATTEN_OBJECT;
    }

    MessageParcel data;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        return ERR_FLATTEN_OBJECT;
    }
    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(
        static_cast<uint32_t>(SysInstallerInterfaceCode::SYS_INSTALLER_INIT), data, reply, option);
    if (ret != ERR_OK) {
        LOG(ERROR) << "SendRequest error";
        return ERR_FLATTEN_OBJECT;
    }

    return reply.ReadInt32();
}

int32_t SysInstallerProxy::StartUpdatePackageZip(const std::string &pkgPath)
{
    LOG(INFO) << "StartUpdatePackageZip";
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
    MessageOption option { MessageOption::TF_ASYNC};
    int32_t ret = remote->SendRequest(
        static_cast<uint32_t>(SysInstallerInterfaceCode::UPDATE_PACKAGE), data, reply, option);
    if (ret != ERR_OK) {
        LOG(ERROR) << "SendRequest error";
        return ERR_FLATTEN_OBJECT;
    }

    return reply.ReadInt32();
}

int32_t SysInstallerProxy::SetUpdateCallback(const sptr<ISysInstallerCallback> &cb)
{
    if (cb == nullptr) {
        LOG(ERROR) <<  "Invalid param";
        return ERR_INVALID_VALUE;
    }
    LOG(INFO) << "RegisterUpdateCallback";

    auto remote = Remote();
    if (remote == nullptr) {
        LOG(ERROR) << "Can not get remote";
        return ERR_FLATTEN_OBJECT;
    }

    MessageParcel data;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        LOG(ERROR) << "WriteInterfaceToken fail";
        return -1;
    }

    bool ret = data.WriteRemoteObject(cb->AsObject());
    if (!ret) {
        LOG(ERROR) << "WriteRemoteObject error";
        return ERR_FLATTEN_OBJECT;
    }
    MessageParcel reply;
    MessageOption option { MessageOption::TF_ASYNC};
    int32_t res = remote->SendRequest(
        static_cast<uint32_t>(SysInstallerInterfaceCode::SET_UPDATE_CALLBACK), data, reply, option);
    if (res != ERR_OK) {
        LOG(ERROR) << "SendRequest error";
        return ERR_FLATTEN_OBJECT;
    }
    return reply.ReadInt32();
}

int32_t SysInstallerProxy::GetUpdateStatus()
{
    LOG(INFO) << "GetUpdateStatus";
    auto remote = Remote();
    if (remote == nullptr) {
        LOG(ERROR) << "Can not get remote";
        return ERR_FLATTEN_OBJECT;
    }

    MessageParcel data;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        return -1;
    }
    MessageParcel reply;
    MessageOption option { MessageOption::TF_ASYNC};
    int32_t ret = remote->SendRequest(
        static_cast<uint32_t>(SysInstallerInterfaceCode::GET_UPDATE_STATUS), data, reply, option);
    if (ret != ERR_OK) {
        LOG(ERROR) << "SendRequest error";
        return ERR_FLATTEN_OBJECT;
    }

    return reply.ReadInt32();
}

int32_t SysInstallerProxy::StartUpdateParaZip(const std::string &pkgPath,
    const std::string &location, const std::string &cfgDir)
{
    LOG(INFO) << "StartUpdateParaZip";
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
    data.WriteString16(Str8ToStr16(location));
    data.WriteString16(Str8ToStr16(cfgDir));

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(
        static_cast<uint32_t>(SysInstallerInterfaceCode::UPDATE_PARA_PACKAGE), data, reply, option);
    if (ret != ERR_OK) {
        LOG(ERROR) << "SendRequest error";
        return ERR_FLATTEN_OBJECT;
    }

    return reply.ReadInt32();
}

int32_t SysInstallerProxy::StartDeleteParaZip(const std::string &location, const std::string &cfgDir)
{
    LOG(INFO) << "StartDeleteParaZip";
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
    data.WriteString16(Str8ToStr16(location));
    data.WriteString16(Str8ToStr16(cfgDir));

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(
        static_cast<uint32_t>(SysInstallerInterfaceCode::DELETE_PARA_PACKAGE), data, reply, option);
    if (ret != ERR_OK) {
        LOG(ERROR) << "SendRequest error";
        return ERR_FLATTEN_OBJECT;
    }

    return reply.ReadInt32();
}

int32_t SysInstallerProxy::AccDecompressAndVerifyPkg(const std::string &srcPath,
    const std::string &dstPath, const uint32_t type)
{
    LOG(INFO) << "AccDecompressAndVerifyPkg";
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
    data.WriteString16(Str8ToStr16(srcPath));
    data.WriteString16(Str8ToStr16(dstPath));
    data.WriteInt32(static_cast<int32_t>(type));

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(
        static_cast<uint32_t>(SysInstallerInterfaceCode::DECOMPRESS_ACC_PACKAGE), data, reply, option);
    if (ret != ERR_OK) {
        LOG(ERROR) << "SendRequest error";
        return ERR_FLATTEN_OBJECT;
    }

    return reply.ReadInt32();
}

int32_t SysInstallerProxy::AccDeleteDir(const std::string &dstPath)
{
    LOG(INFO) << "AccDeleteDir";
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
    data.WriteString16(Str8ToStr16(dstPath));

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(
        static_cast<uint32_t>(SysInstallerInterfaceCode::DELETE_ACC_PACKAGE), data, reply, option);
    if (ret != ERR_OK) {
        LOG(ERROR) << "SendRequest error";
        return ERR_FLATTEN_OBJECT;
    }

    return reply.ReadInt32();
}
}
} // namespace OHOS
