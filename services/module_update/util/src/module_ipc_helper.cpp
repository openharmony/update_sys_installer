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

#include "module_ipc_helper.h"

#include "string_ex.h"

namespace OHOS {
namespace SysInstaller {
void ReadSaVersion(MessageParcel &reply, SaVersion &version)
{
    version.apiVersion = reply.ReadUint32();
    version.versionCode = reply.ReadUint32();
    version.patchVersion = reply.ReadUint32();
}

void WriteSaVersion(MessageParcel &data, const SaVersion &version)
{
    data.WriteUint32(version.apiVersion);
    data.WriteUint32(version.versionCode);
    data.WriteUint32(version.patchVersion);
}

void ReadSaInfo(MessageParcel &reply, SaInfo &info)
{
    info.saName = Str16ToStr8(reply.ReadString16());
    info.saId = reply.ReadInt32();
    ReadSaVersion(reply, info.version);
}

void WriteSaInfo(MessageParcel &data, const SaInfo &info)
{
    data.WriteString16(Str8ToStr16(info.saName));
    data.WriteInt32(info.saId);
    WriteSaVersion(data, info.version);
}

void ReadBundleInfo(MessageParcel &reply, BundleInfo &info)
{
    info.bundleName = Str16ToStr8(reply.ReadString16());
    info.bundleVersion = Str16ToStr8(reply.ReadString16());
}

void WriteBundleInfo(MessageParcel &data, const BundleInfo &info)
{
    data.WriteString16(Str8ToStr16(info.bundleName));
    data.WriteString16(Str8ToStr16(info.bundleVersion));
}

void ReadModulePackageInfo(MessageParcel &reply, ModulePackageInfo &info)
{
    info.hmpName = Str16ToStr8(reply.ReadString16());
    info.version = Str16ToStr8(reply.ReadString16());
    info.saSdkVersion = Str16ToStr8(reply.ReadString16());
    info.type = Str16ToStr8(reply.ReadString16());
    info.apiVersion = reply.ReadInt32();
    info.hotApply = reply.ReadInt32();

    ModuleIpcHelper::ReadList<SaInfo>(reply, info.saInfoList, ReadSaInfo);
    ModuleIpcHelper::ReadList<BundleInfo>(reply, info.bundleInfoList, ReadBundleInfo);
}

void WriteModulePackageInfo(MessageParcel &data, const ModulePackageInfo &info)
{
    data.WriteString16(Str8ToStr16(info.hmpName));
    data.WriteString16(Str8ToStr16(info.version));
    data.WriteString16(Str8ToStr16(info.saSdkVersion));
    data.WriteString16(Str8ToStr16(info.type));
    data.WriteInt32(info.apiVersion);
    data.WriteInt32(info.hotApply);

    ModuleIpcHelper::WriteList<SaInfo>(data, info.saInfoList, WriteSaInfo);
    ModuleIpcHelper::WriteList<BundleInfo>(data, info.bundleInfoList, WriteBundleInfo);
}

int32_t ModuleIpcHelper::ReadModulePackageInfos(MessageParcel &reply, std::list<ModulePackageInfo> &infos)
{
    ReadList<ModulePackageInfo>(reply, infos, ReadModulePackageInfo);
    return 0;
}

int32_t ModuleIpcHelper::WriteModulePackageInfos(MessageParcel &data, const std::list<ModulePackageInfo> &infos)
{
    WriteList<ModulePackageInfo>(data, infos, WriteModulePackageInfo);
    return 0;
}

int32_t ModuleIpcHelper::ReadModuleUpdateStatus(MessageParcel &reply, ModuleUpdateStatus &status)
{
    status.hmpName = Str16ToStr8(reply.ReadString16());
    status.isPreInstalled = reply.ReadBool();
    status.isAllMountSuccess = reply.ReadBool();
    status.isHotInstall = reply.ReadBool();
    status.type = static_cast<HmpInstallType>(reply.ReadInt32());
    return 0;
}

int32_t ModuleIpcHelper::WriteModuleUpdateStatus(MessageParcel &data, const ModuleUpdateStatus &status)
{
    data.WriteString16(Str8ToStr16(status.hmpName));
    data.WriteBool(status.isPreInstalled);
    data.WriteBool(status.isAllMountSuccess);
    data.WriteBool(status.isHotInstall);
    data.WriteInt32(static_cast<int32_t>(status.type));
    return 0;
}
} // namespace SysInstaller
} // namespace OHOS