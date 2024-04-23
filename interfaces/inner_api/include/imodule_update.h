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

#ifndef SYS_INSTALLER_IMODULE_UPDATE_H
#define SYS_INSTALLER_IMODULE_UPDATE_H

#include "iremote_broker.h"
#include "isys_installer_callback.h"
#include "module_ipc_helper.h"
#include "parcel.h"
#include "string_ex.h"

namespace OHOS {
namespace SysInstaller {
struct HmpVersionInfo : public Parcelable {
    std::string name {};
    std::string laneCode {};
    std::string compatibleVersion {};
    std::string version {};

    virtual bool Marshalling(Parcel &parcel) const override
    {
        if (!parcel.WriteString16(Str8ToStr16(name))) {
            return false;
        }
        if (!parcel.WriteString16(Str8ToStr16(laneCode))) {
            return false;
        }
        if (!parcel.WriteString16(Str8ToStr16(compatibleVersion))) {
            return false;
        }
        if (!parcel.WriteString16(Str8ToStr16(version))) {
            return false;
        }
        return true;
    }

    bool ReadFromParcel(Parcel &parcel)
    {
        name = Str16ToStr8(parcel.ReadString16());
        laneCode = Str16ToStr8(parcel.ReadString16());
        compatibleVersion = Str16ToStr8(parcel.ReadString16());
        version = Str16ToStr8(parcel.ReadString16());
        return true;
    }

    static HmpVersionInfo *Unmarshalling(Parcel &parcel)
    {
        HmpVersionInfo *obj = new (std::nothrow) HmpVersionInfo();
        if (obj != nullptr && !obj->ReadFromParcel(parcel)) {
            delete obj;
            obj = nullptr;
        }
        return obj;
    }
};

struct HmpUpdateInfo : public Parcelable {
    std::string path {};
    int32_t result = 0; // 0 means success
    std::string resultMsg {};

    virtual bool Marshalling(Parcel &parcel) const override
    {
        if (!parcel.WriteString16(Str8ToStr16(path))) {
            return false;
        }
        if (!parcel.WriteInt32(result)) {
            return false;
        }
        if (!parcel.WriteString16(Str8ToStr16(resultMsg))) {
            return false;
        }
        return true;
    }

    bool ReadFromParcel(Parcel &parcel)
    {
        path = Str16ToStr8(parcel.ReadString16());
        result = parcel.ReadInt32();
        resultMsg = Str16ToStr8(parcel.ReadString16());
        return true;
    }

    static HmpUpdateInfo *Unmarshalling(Parcel &parcel)
    {
        HmpUpdateInfo *obj = new (std::nothrow) HmpUpdateInfo();
        if (obj != nullptr && !obj->ReadFromParcel(parcel)) {
            delete obj;
            obj = nullptr;
        }
        return obj;
    }
};

class IModuleUpdate : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.Updater.IModuleUpdate");

    virtual int32_t InstallModulePackage(const std::string &pkgPath) = 0;
    virtual int32_t UninstallModulePackage(const std::string &hmpName) = 0;
    virtual int32_t GetModulePackageInfo(const std::string &hmpName,
        std::list<ModulePackageInfo> &modulePackageInfos) = 0;
    virtual int32_t ReportModuleUpdateStatus(const ModuleUpdateStatus &status) = 0;
    virtual int32_t ExitModuleUpdate() = 0;

    virtual std::vector<HmpVersionInfo> GetHmpVersionInfo() = 0;
    virtual int32_t StartUpdateHmpPackage(const std::string &path,
        const sptr<ISysInstallerCallback> &updateCallback) = 0;
    virtual std::vector<HmpUpdateInfo> GetHmpUpdateResult() = 0;
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_IMODULE_UPDATE_H
