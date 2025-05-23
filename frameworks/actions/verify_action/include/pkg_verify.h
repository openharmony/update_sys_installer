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

#ifndef SYS_INSTALLER_PKG_VERIFY_H
#define SYS_INSTALLER_PKG_VERIFY_H

#include <iostream>
#include "iaction.h"
#include "status_manager.h"

namespace OHOS {
namespace SysInstaller {
class PkgVerify : public IAction {
public:
    PkgVerify(std::shared_ptr<StatusManager> statusManager,
        const std::vector<std::string> &pkgPath) : statusManager_(statusManager), pkgPath_(pkgPath) {}
    virtual ~PkgVerify() = default;

    void PerformAction() override;
    std::string GetActionName() override
    {
        return "verify";
    };

protected:
    virtual void Init();
    virtual int Verify(const std::vector<std::string> &pkgPath);

protected:
    bool verifyInit_ = false;
    std::shared_ptr<StatusManager> statusManager_ {};
    std::vector<std::string> pkgPath_;
};
} // SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_PKG_VERIFY_H
