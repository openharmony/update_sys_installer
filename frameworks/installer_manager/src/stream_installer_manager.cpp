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

#include "stream_installer_manager.h"

#include "log/log.h"
#include "package/pkg_manager.h"
#include "utils.h"
#include "updater_main.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

void StreamInstallerManager::RegisterDump(std::unique_ptr<StreamInstallerManagerHelper> ptr)
{
    helper_ = std::move(ptr);
}

StreamInstallerManager &StreamInstallerManager::GetInstance()
{
    static StreamInstallerManager instance;
    return instance;
}

int32_t StreamInstallerManager::SysInstallerInit()
{
    if (helper_ == nullptr) {
        if (helper_ == nullptr) {
            RegisterDump(std::make_unique<StreamInstallerManagerHelper>());
        }
    }
    return helper_->SysInstallerInit();
}

int32_t StreamInstallerManager::StartStreamUpdate()
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->StartStreamUpdate();
}

int32_t StreamInstallerManager::StopStreamUpdate()
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->StopStreamUpdate();
}

int32_t StreamInstallerManager::ProcessStreamData(const std::vector<uint8_t>& buffer, uint32_t size)
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->ProcessStreamData(buffer, size);
}

int32_t StreamInstallerManager::SetUpdateCallback(const sptr<ISysInstallerCallback> &updateCallback)
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->SetUpdateCallback(updateCallback);
}

int32_t StreamInstallerManager::GetUpdateStatus()
{
    if (helper_ == nullptr) {
        LOG(ERROR) << "helper_ null";
        return -1;
    }
    return helper_->GetUpdateStatus();
}

} // namespace SysInstaller
} // namespace OHOS
