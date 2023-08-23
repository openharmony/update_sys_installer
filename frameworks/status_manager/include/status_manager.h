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

#ifndef SYS_INSTALLER_STATUS_MANAGER_H
#define SYS_INSTALLER_STATUS_MANAGER_H

#include "isys_installer.h"
#include "isys_installer_callback.h"
#include "refbase.h"
#include "sys_installer_common.h"

namespace OHOS {
namespace SysInstaller {
class StatusManager {
public:
    StatusManager() = default;
    virtual ~StatusManager() = default;

    virtual void Init();
    virtual int GetUpdateStatus();
    virtual int SetUpdateCallback(const sptr<ISysInstallerCallback> &updateCallback);
    virtual void UpdateCallback(UpdateStatus updateStatus, int percent, const std::string &resultMsg);

    void SetUpdatePercent(int percent);
    float GetUpdateProgress();

protected:
    UpdateStatus updateStatus_ = UPDATE_STATE_INIT;
    int percent_ = 0;
    std::mutex updateCbMutex_ {};
    sptr<ISysInstallerCallback> updateCallback_ {};
};
} // SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_STATUS_MANAGER_H
