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

#ifndef SYS_INSTALLER_CALLBACK_H
#define SYS_INSTALLER_CALLBACK_H

#include "iremote_stub.h"
#include "isys_installer_callback.h"
#include "isys_installer_callback_func.h"
#include "sys_installer_callback_stub.h"

namespace OHOS {
namespace SysInstaller {
class SysInstallerCallback : public SysInstallerCallbackStub {
public:
    SysInstallerCallback() = default;
    explicit SysInstallerCallback(sptr<ISysInstallerCallbackFunc> callback);
    ~SysInstallerCallback() = default;

    ErrCode OnUpgradeProgress(UpdateStatus updateStatus, int percent, const std::string &resultMsg) override;
    ErrCode OnUpgradeDealLen(UpdateStatus updateStatus, int dealLen, const std::string &resultMsg) override;

private:
    sptr<ISysInstallerCallbackFunc> callback_;
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_CALLBACK_H
