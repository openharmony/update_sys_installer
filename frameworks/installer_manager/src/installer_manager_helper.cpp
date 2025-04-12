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

#include "action_processer.h"
#include "log/log.h"
#include "package/cert_verify.h"
#include "package/pkg_manager.h"
#include "utils.h"
#include "pkg_verify.h"
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
    if (statusManager_ == nullptr) {
        LOG(ERROR) << "statusManager_ nullptr";
        return -1;
    }
    if (ActionProcesser::GetInstance().IsRunning()) {
        LOG(ERROR) << "ActionProcesser IsRunning";
        return -1;
    }
    std::vector<std::string> filePath = {};
    filePath.push_back(pkgPath);
    ActionProcesser::GetInstance().AddAction(std::make_unique<PkgVerify>(statusManager_, filePath));
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

int32_t InstallerManagerHelper::StartDeleteParaZip(const std::string &location, const std::string &cfgDir)
{
    return -1;
}

int32_t InstallerManagerHelper::AccDecompressAndVerifyPkg(const std::string &srcPath,
    const std::string &dstPath, const uint32_t type)
{
    return -1;
}

int32_t InstallerManagerHelper::AccDeleteDir(const std::string &dstPath)
{
    return -1;
}

int32_t InstallerManagerHelper::CancelUpdateVabPackageZip(void)
{
    return -1;
}

int32_t InstallerManagerHelper::StartUpdateVabPackageZip(const std::vector<std::string> &pkgPath)
{
    return -1;
}

int32_t InstallerManagerHelper::StartVabMerge()
{
    return -1;
}

int32_t InstallerManagerHelper::EnableVabCheckpoint()
{
    return -1;
}

int32_t InstallerManagerHelper::AbortVabActiveSnapshot()
{
    return -1;
}

int32_t InstallerManagerHelper::ClearVabMetadataAndCow()
{
    return -1;
}
} // namespace SysInstaller
} // namespace OHOS
