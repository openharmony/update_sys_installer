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

#ifndef SYS_INSTALLER_I_ACTION_H
#define SYS_INSTALLER_I_ACTION_H

#include <cstdio>
#include "error_code.h"
#include "sys_installer_common.h"

namespace OHOS {
namespace SysInstaller {
using ActionCallbackFun = std::function<void (InstallerErrCode, const std::string &)>;

class IAction {
public:
    IAction() = default;
    virtual ~IAction() = default;

    virtual void SetCallback(ActionCallbackFun actionCallBack)
    {
        actionCallBack_ = actionCallBack;
    }
    virtual void PerformAction() = 0;
    virtual std::string GetActionName() = 0;
    virtual void TerminateAction() {};
    virtual void SuspendAction() {}
    virtual void ResumeAction() {}
    virtual std::string GetErrorStr()
    {
        return "";
    }

protected:
    ActionCallbackFun actionCallBack_;
};
} // SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_I_ACTION_H
