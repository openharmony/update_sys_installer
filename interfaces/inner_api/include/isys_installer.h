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

#ifndef ISYS_INSTALLER_H
#define ISYS_INSTALLER_H

#include <iostream>
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "isys_installer_callback.h"

namespace OHOS {
namespace SysInstaller {
class ISysInstaller : public OHOS::IRemoteBroker {
public:
    enum {
        SYS_INSTALLER_INIT = 1,
        UPDATE_PACKAGE,
        SET_UPDATE_CALLBACK,
        GET_UPDATE_STATUS,
        UPDATE_PARA_PACKAGE
    };

    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.Updater.ISysInstaller");

public:
    virtual int32_t SysInstallerInit() = 0;
    virtual int32_t StartUpdatePackageZip(const std::string &pkgPath) = 0;
    virtual int32_t SetUpdateCallback(const sptr<ISysInstallerCallback> &updateCallback) = 0;
    virtual int32_t GetUpdateStatus() = 0;
    virtual int32_t StartUpdateParaZip(const std::string &pkgPath,
        const std::string &location, const std::string &cfgDir) = 0;
};
} // namespace SysInstaller
} // namespace OHOS
#endif // ISYS_INSTALLER_H
