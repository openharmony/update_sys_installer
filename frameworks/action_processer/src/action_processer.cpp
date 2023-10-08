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

#include "action_processer.h"

#include "installer_manager.h"
#include "log/log.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

ActionProcesser &ActionProcesser::GetInstance()
{
    static ActionProcesser instance;
    return instance;
}

bool ActionProcesser::IsRunning()
{
    return isRunning_ || isSuspend_;
}

void ActionProcesser::AddAction(std::unique_ptr<IAction> action)
{
    if (isRunning_ || action == nullptr) {
        LOG(ERROR) << "action running or action empty";
        return;
    }

    LOG(INFO) << "add " << action->GetActionName();
    auto callBack = [this](InstallerErrCode errCode, const std::string &errStr) {
        CompletedAction(errCode, errStr);
    };
    action->SetCallback(callBack);
    actionQue_.push_back(std::move(action));
}

void ActionProcesser::Start()
{
    if (isRunning_ || actionQue_.empty()) {
        LOG(WARNING) << "Action running or queue empty";
        return;
    }

    isRunning_ = true;
    isSuspend_ = false;
    statusManager_->UpdateCallback(UPDATE_STATE_ONGOING, 0, "");
    curAction_ = std::move(actionQue_.front());
    actionQue_.pop_front();
    if (curAction_ == nullptr) {
        LOG(WARNING) << "curAction_ nullptr";
        return;
    }
    LOG(INFO) << "Start " << curAction_->GetActionName();
    curAction_->PerformAction();
}

void ActionProcesser::Stop()
{
    if (!isRunning_) {
        LOG(WARNING) << "Action not running";
        return;
    }

    if (curAction_ != nullptr) {
        LOG(INFO) << "Stop " << curAction_->GetActionName();
        curAction_->TerminateAction();
    }

    isRunning_ = false;
    isSuspend_ = false;
    curAction_.reset();
    actionQue_.clear();
}

void ActionProcesser::Suspend()
{
    if (!isRunning_ || curAction_ == nullptr) {
        LOG(WARNING) << "ActionProcesser not running or action empty";
        return;
    }

    LOG(INFO) << "Suspend " << curAction_->GetActionName();
    isSuspend_ = true;
    curAction_->SuspendAction();
}

void ActionProcesser::Resume()
{
    if (isSuspend_) {
        LOG(WARNING) << "ActionProcesser is running";
        return;
    }

    isSuspend_ = false;
    if (curAction_ != nullptr) {
        LOG(INFO) << "Resume " << curAction_->GetActionName();
        return curAction_->ResumeAction();
    }

    StartNextAction();
}

void ActionProcesser::CompletedAction(InstallerErrCode errCode, const std::string &errStr)
{
    if (curAction_ == nullptr) {
        LOG(ERROR) << "curAction_ null error ";
        return;
    }

    LOG(INFO) << "Completed " << curAction_->GetActionName();
    curAction_.reset();
    if (errCode != SYS_UPDATE_SUCCESS) {
        isRunning_ = false;
        isSuspend_ = false;
        statusManager_->UpdateCallback(UPDATE_STATE_FAILED, 100, errStr); // 100 : action failed
        actionQue_.clear();
        LOG(ERROR) << "CompletedAction errCode:" << errCode << " str:" << errStr;
        SysInstallerManagerInit::GetInstance().InvokeEvent(SYS_POST_EVENT);
        return;
    }

    if (isSuspend_) {
        LOG(INFO) << "suspend";
        return;
    }

    StartNextAction();
}


void ActionProcesser::StartNextAction()
{
    if (actionQue_.empty()) {
        LOG(INFO) << "Action queue empty, successful";
        isRunning_ = false;
        isSuspend_ = false;
        statusManager_->UpdateCallback(UPDATE_STATE_SUCCESSFUL, 100, ""); // 100 : action completed
        return;
    }

    curAction_ = std::move(actionQue_.front());
    LOG(INFO) << "StartNextAction " << curAction_->GetActionName();
    actionQue_.pop_front();
    curAction_->PerformAction();
}
} // namespace SysInstaller
} // namespace OHOS
