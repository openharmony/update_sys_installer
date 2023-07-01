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

#ifndef ISYS_INSTALLER_CALLBACK_H
#define ISYS_INSTALLER_CALLBACK_H

#include <iostream>
#include <string>
#include "iremote_broker.h"
#include "iremote_proxy.h"

namespace OHOS {
namespace SysInstaller {
enum UpdateStatus {
    UPDATE_STATE_INIT = 0,
    UPDATE_STATE_ONGOING,
    UPDATE_STATE_FAILED,
    UPDATE_STATE_SUCCESSFUL,
    UPDATE_STATE_MAX
};

class ISysInstallerCallback : public OHOS::IRemoteBroker {
public:
    virtual ~ISysInstallerCallback() = default;

    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.Update.ISysInstallerCallback");

    virtual void OnUpgradeProgress(UpdateStatus updateStatus, int percent, const std::string &resultMsg) = 0;
};
} // namespace SysInstaller
} // namespace OHOS
#endif // ISYS_INSTALLER_CALLBACK_H
