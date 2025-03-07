/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "stream_status_manager.h"

#include "sys_installer_common.h"
#include "log/log.h"
#include "utils.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

void StreamStatusManager::Init()
{
    updateStatus_ = UPDATE_STATE_INIT;
}

int StreamStatusManager::SetUpdateCallback(const sptr<ISysInstallerCallback> &updateCallback)
{
    std::lock_guard<std::mutex> lock(updateCbMutex_);
    if (updateCallback == nullptr) {
        LOG(ERROR) << "para error";
        return -1;
    }

    updateCallback_ = updateCallback;
    return 0;
}

int StreamStatusManager::GetUpdateStatus()
{
    return updateStatus_;
}

void StreamStatusManager::UpdateCallback(UpdateStatus updateStatus, int dealLen, const std::string &resultMsg)
{
    std::lock_guard<std::mutex> lock(updateCbMutex_);
    if (updateCallback_ == nullptr) {
        LOG(ERROR) << "updateCallback_ null";
        return;
    }

    if (updateStatus > UPDATE_STATE_MAX) {
        LOG(INFO) << "status error:" << updateStatus;
        return;
    }

    updateStatus_ = updateStatus;
    LOG(INFO) << "status:" << updateStatus_ << " dealLen:"  << dealLen << " msg:" << resultMsg;
    updateCallback_->OnUpgradeDealLen(updateStatus_, dealLen, resultMsg);
}

} // namespace SysInstaller
} // namespace OHOS
