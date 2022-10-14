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

#include "installer_manager_helper.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "action_processer.h"
#include "log/log.h"
#include "package/cert_verify.h"
#include "package/pkg_manager.h"
#include "utils.h"

#include "ab_update.h"

namespace OHOS {
namespace SysInstaller {
using namespace Hpackage;
using namespace Updater;

int32_t InstallerManagerHelper::SysInstallerInit()
{
    LOG(INFO) << "SysInstallerInit";

    if (statusManager_ == nullptr) {
        statusManager_ = std::make_shared<StatusManager>();
    }
    statusManager_->Init();
    ActionProcesser::GetInstance().SetStatusManager(statusManager_);
    return 0;
}

int32_t InstallerManagerHelper::StartUpdatePackageZip(const std::string &pkgPath)
{
    LOG(INFO) << "StartUpdatePackageZip start";
    if (ActionProcesser::GetInstance().IsRunning()) {
        LOG(ERROR) << "ActionProcesser IsRunning";
        return -1;
    }
    ActionProcesser::GetInstance().AddAction(std::make_unique<ABUpdate>(statusManager_, pkgPath));
    ActionProcesser::GetInstance().Start();
    return 0;
}

int32_t InstallerManagerHelper::SetUpdateCallback(const sptr<ISysInstallerCallback> &updateCallback)
{
    if (statusManager_ == nullptr) {
        LOG(ERROR) << "statusManager_ nullptr";
        return -1;
    }
    return statusManager_->SetUpdateCallback(updateCallback);
}

int32_t InstallerManagerHelper::GetUpdateStatus()
{
    if (statusManager_ == nullptr) {
        LOG(ERROR) << "statusManager_ nullptr";
        return -1;
    }
    return statusManager_->GetUpdateStatus();
}

int32_t InstallerManagerHelper::StartUpdateParaZip(const std::string &pkgPath,
    const std::string &location, const std::string &cfgDir)
{
    return -1;
}
} // namespace SysInstaller
} // namespace OHOS
