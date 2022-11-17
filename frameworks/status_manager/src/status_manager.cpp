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

#include "log/log.h"
#include "utils.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

void StatusManager::Init()
{
    updateStatus_ = UPDATE_STATE_INIT;
    percent_ = 0;
}

int StatusManager::SetUpdateCallback(const sptr<ISysInstallerCallback> &updateCallback)
{
    if (updateCallback == nullptr) {
        LOG(ERROR) << "para error";
        return -1;
    }
    updateCallback_ = updateCallback;
    return 0;
}

int StatusManager::GetUpdateStatus()
{
    return updateStatus_;
}

void StatusManager::UpdateCallback(UpdateStatus updateStatus, int percent)
{
    if (updateCallback_ == nullptr) {
        return;
    }

    if (updateStatus > UPDATE_STATE_SUCCESSFUL) {
        LOG(INFO) << "status error:" << updateStatus;
        return;
    }
    if (percent >=0 && percent <= 100 && percent >= percent_) { // 100 : max percent
        percent_ = percent;
    }

    updateStatus_ = updateStatus;
    LOG(INFO) << "status:" << updateStatus_ << " percent:"  << percent_;
    updateCallback_->OnUpgradeProgress(updateStatus_, percent_);
}
} // namespace SysInstaller
} // namespace OHOS
