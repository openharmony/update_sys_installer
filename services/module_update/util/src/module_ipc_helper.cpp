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
void ReadModuleVersion(MessageParcel &reply, ModuleVersion &version)
{
    version.apiVersion = reply.ReadUint32();
    version.versionCode = reply.ReadUint32();
    version.patchVersion = reply.ReadUint32();
}

void WriteModuleVersion(MessageParcel &data, const ModuleVersion &version)
{
    data.WriteUint32(version.apiVersion);
    data.WriteUint32(version.versionCode);
    data.WriteUint32(version.patchVersion);
}

void ReadSaInfo(MessageParcel &reply, SaInfo &info)
{
    info.saName = Str16ToStr8(reply.ReadString16());
    info.saId = reply.ReadInt32();
    ReadModuleVersion(reply, info.version);
}

void WriteSaInfo(MessageParcel &data, const SaInfo &info)
{
    data.WriteString16(Str8ToStr16(info.saName));
    data.WriteInt32(info.saId);
    WriteModuleVersion(data, info.version);
}

void ReadModulePackageInfo(MessageParcel &reply, ModulePackageInfo &info)
{
    info.hmpName = Str16ToStr8(reply.ReadString16());
    ModuleIpcHelper::ReadList<SaInfo>(reply, info.saInfoList, ReadSaInfo);
}

void WriteModulePackageInfo(MessageParcel &data, const ModulePackageInfo &info)
{
    data.WriteString16(Str8ToStr16(info.hmpName));
    ModuleIpcHelper::WriteList<SaInfo>(data, info.saInfoList, WriteSaInfo);
}

void ReadSaStatus(MessageParcel &reply, SaStatus &status)
{
    status.saId = reply.ReadInt32();
    status.isPreInstalled = reply.ReadBool();
    status.isMountSuccess = reply.ReadBool();
}

void WriteSaStatus(MessageParcel &data, const SaStatus &status)
{
    data.WriteInt32(status.saId);
    data.WriteBool(status.isPreInstalled);
    data.WriteBool(status.isMountSuccess);
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
    status.process = Str16ToStr8(reply.ReadString16());
    ReadList<SaStatus>(reply, status.saStatusList, ReadSaStatus);
    return 0;
}

int32_t ModuleIpcHelper::WriteModuleUpdateStatus(MessageParcel &data, const ModuleUpdateStatus &status)
{
    data.WriteString16(Str8ToStr16(status.process));
    WriteList<SaStatus>(data, status.saStatusList, WriteSaStatus);
    return 0;
}
} // namespace SysInstaller
} // namespace OHOS