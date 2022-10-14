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

#ifndef SYS_INSTALLER_ACTION_PROCESSER_H
#define SYS_INSTALLER_ACTION_PROCESSER_H

#include <deque>
#include <map>
#include "iaction.h"
#include "macros.h"
#include "status_manager.h"

namespace OHOS {
namespace SysInstaller {
class ActionProcesser {
    DISALLOW_COPY_MOVE(ActionProcesser);
public:
    static ActionProcesser &GetInstance();
    void SetStatusManager(std::shared_ptr<StatusManager> statusManager)
    {
        statusManager_ = statusManager;
    }

    bool IsRunning();
    void AddAction(std::unique_ptr<IAction> action);
    void Start();
    void Stop();
    void Suspend();
    void Resume();
    void CompletedAction(InstallerErrCode errCode, const std::string &errStr);

private:
    ActionProcesser() = default;
    ~ActionProcesser() = default;
    void StartNextAction();

    std::shared_ptr<StatusManager> statusManager_ {};
    std::deque<std::unique_ptr<IAction>> actionQue_ {};
    std::unique_ptr<IAction> curAction_ {};
    bool isRunning_ = false;
    bool isSuspend_ = false;
};
} // SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_ACTION_PROCESSER_H
