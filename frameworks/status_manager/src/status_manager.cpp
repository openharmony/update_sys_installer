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

#include "status_manager.h"

#include "sys_installer_common.h"
#include "log/log.h"
#include "utils.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

void StatusManager::Init()
{
    updateStatus_ = UpdateStatus::UPDATE_STATE_INIT;
    percent_ = 0;
}

int StatusManager::SetUpdateCallback(const sptr<ISysInstallerCallback> &updateCallback)
{
    std::lock_guard<std::mutex> lock(updateCbMutex_);
    if (updateCallback == nullptr) {
        LOG(ERROR) << "para error";
        return -1;
    }

    updateCallback_ = updateCallback;
    updateCallback_->OnUpgradeProgress(updateStatus_, percent_, "");
    LOG(INFO) << "reset progress when reset callback " << percent_ << " " << updateStatus_;
    return 0;
}

int StatusManager::GetUpdateStatus()
{
    return static_cast<int32_t>(updateStatus_);
}

void StatusManager::UpdateCallback(UpdateStatus updateStatus, int percent, const std::string &resultMsg)
{
    std::lock_guard<std::mutex> lock(updateCbMutex_);
    if (updateCallback_ == nullptr) {
        LOG(ERROR) << "updateCallback_ null";
        return;
    }

    if (updateStatus > UpdateStatus::UPDATE_STATE_MAX) {
        LOG(INFO) << "status error:" << static_cast<int32_t>(updateStatus);
        return;
    }
    if (updateStatus_ == updateStatus && percent == percent_) {
        return;
    }
    if (updateStatus == UpdateStatus::UPDATE_STATE_SUCCESSFUL || updateStatus == UpdateStatus::UPDATE_STATE_FAILED) {
        percent_ = 100; // 100 : max percent
    } else if (percent >= 0 && percent <= 100 && percent > percent_) { // 100 : max percent
        percent_ = percent;
    }

    updateStatus_ = updateStatus;
    LOG(INFO) << "status:" << static_cast<int32_t>(updateStatus_) << " percent:"  << percent_ << " msg:" << resultMsg;
    updateCallback_->OnUpgradeProgress(updateStatus_, percent_, resultMsg);
}

void StatusManager::CallbackWithoutCheck(UpdateStatus updateStatus, int percent, const std::string &resultMsg)
{
    std::lock_guard<std::mutex> lock(updateCbMutex_);
    if (updateCallback_ == nullptr) {
        LOG(ERROR) << "updateCallback_ null";
        return;
    }
    if (updateStatus_ == updateStatus && percent == percent_) {
        return;
    }
    if (updateStatus > UpdateStatus::UPDATE_STATE_MAX) {
        LOG(INFO) << "status error:" << static_cast<int32_t>(updateStatus);
        return;
    }

    updateStatus_ = updateStatus;
    LOG(INFO) << "status:" << static_cast<int32_t>(updateStatus_) << " percent:"  << percent_ << " msg:" << resultMsg;
    updateCallback_->OnUpgradeProgress(updateStatus_, percent_, resultMsg);
}

void StatusManager::SetUpdatePercent(int percent)
{
    UpdateCallback(updateStatus_, percent, "");
}

float StatusManager::GetUpdateProgress()
{
    return percent_ / 100.0; // 100.0 : max percent
}
} // namespace SysInstaller
} // namespace OHOS
