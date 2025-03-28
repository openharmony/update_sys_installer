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

#include "stream_installer_manager_helper.h"

#include "log/log.h"
#include "stream_update.h"
#include "utils.h"

namespace OHOS {
namespace SysInstaller {
using namespace Hpackage;
using namespace Updater;

int32_t StreamInstallerManagerHelper::SysInstallerInit()
{
    LOG(INFO) << "SysInstallerInit";

    if (statusManager_ == nullptr) {
        statusManager_ = std::make_shared<StreamStatusManager>();
    }
    statusManager_->Init();
    StreamInstallProcesser::GetInstance().SetStatusManager(statusManager_);

    return 0;
}

int32_t StreamInstallerManagerHelper::StartStreamUpdate()
{
    LOG(INFO) << "StartStreamUpdate start";
    if (statusManager_ == nullptr) {
        LOG(ERROR) << "statusManager_ nullptr";
        return -1;
    }
    if (StreamInstallProcesser::GetInstance().IsRunning()) {
        LOG(ERROR) << "StreamInstallProcesser IsRunning";
        return -1;
    }
    if (StreamInstallProcesser::GetInstance().Start() == -1) {
        LOG(ERROR) << "StreamInstallProcesser start fail";
        return -1;
    }
    return 0;
}

int32_t StreamInstallerManagerHelper::StopStreamUpdate()
{
    LOG(INFO) << "StopStreamUpdate enter";
    if (statusManager_ == nullptr) {
        LOG(ERROR) << "statusManager_ nullptr";
        return -1;
    }
    if (!StreamInstallProcesser::GetInstance().IsRunning()) {
        LOG(ERROR) << "StreamInstallProcesser is not Running";
        return -1;
    }
    StreamInstallProcesser::GetInstance().Stop();
    return 0;
}

int32_t StreamInstallerManagerHelper::ProcessStreamData(const std::vector<uint8_t>& buffer, uint32_t size)
{
    return StreamInstallProcesser::GetInstance().ProcessStreamData(buffer, size);
}

int32_t StreamInstallerManagerHelper::SetUpdateCallback(const sptr<ISysInstallerCallback> &updateCallback)
{
    if (statusManager_ == nullptr) {
        LOG(ERROR) << "statusManager_ nullptr";
        return -1;
    }
    return statusManager_->SetUpdateCallback(updateCallback);
}

int32_t StreamInstallerManagerHelper::GetUpdateStatus()
{
    if (statusManager_ == nullptr) {
        LOG(ERROR) << "statusManager_ nullptr";
        return -1;
    }
    return statusManager_->GetUpdateStatus();
}

} // namespace SysInstaller
} // namespace OHOS
