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

#ifndef SYS_INSTALLER_AB_UPDATE_H
#define SYS_INSTALLER_AB_UPDATE_H

#include "iaction.h"
#include "installer_manager.h"
#include "updater/updater.h"

namespace OHOS {
namespace SysInstaller {
class ABUpdate : public IAction {
public:
    ABUpdate(std::shared_ptr<StatusManager> statusManager,
        const std::string &pkgPath) : statusManager_(statusManager), pkgPath_(pkgPath) {}
    ~ABUpdate() = default;

    void PerformAction() override;
    std::string GetActionName() override
    {
        return "ab_update";
    }

private:
    Updater::UpdaterStatus StartABUpdate(const std::string &pkgPath);
    void SetProgress(float value);

private:
    std::shared_ptr<StatusManager> statusManager_ {};
    std::string pkgPath_;
};
} // SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_PARA_UPDATE_H
