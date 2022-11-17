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

#include "sys_installer_server.h"

#include "iservice_registry.h"
#include "log/log.h"
#include "securec.h"
#include "system_ability_definition.h"
#include "utils.h"

namespace OHOS {
namespace SysInstaller {
REGISTER_SYSTEM_ABILITY_BY_ID(SysInstaller, SYS_INSTALLER_DISTRIBUTED_SERVICE_ID, false)

using namespace Updater;

SysInstaller::SysInstaller(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(systemAbilityId, runOnCreate)
{
}

SysInstaller::~SysInstaller()
{
}

int32_t SysInstaller::SysInstallerInit()
{
    LOG(INFO) << "SysInstallerInit";
    if (!logInit_) {
        (void)Utils::MkdirRecursive(SYS_LOG_DIR, 0777); // 0777 : rwxrwxrwx
        InitUpdaterLogger("SysInstaller", SYS_LOG_FILE, SYS_STAGE_FILE, SYS_ERROR_FILE);
        logInit_ = true;
    }

    InstallerManager::GetInstance().SysInstallerInit();
    return 0;
}

int32_t SysInstaller::StartUpdatePackageZip(const std::string &pkgPath)
{
    LOG(INFO) << "StartUpdatePackageZip";
    return InstallerManager::GetInstance().StartUpdatePackageZip(pkgPath);
}

int32_t SysInstaller::SetUpdateCallback(const sptr<ISysInstallerCallback> &updateCallback)
{
    LOG(INFO) << "SetUpdateCallback";
    return InstallerManager::GetInstance().SetUpdateCallback(updateCallback);
}

int32_t SysInstaller::GetUpdateStatus()
{
    LOG(INFO) << "GetUpdateStatus";
    return InstallerManager::GetInstance().GetUpdateStatus();
}

int32_t SysInstaller::StartUpdateParaZip(const std::string &pkgPath,
    const std::string &location, const std::string &cfgDir)
{
    LOG(INFO) << "StartUpdateParaZip";
    return InstallerManager::GetInstance().StartUpdateParaZip(pkgPath, location, cfgDir);
}

void SysInstaller::OnStart()
{
    LOG(INFO) << "OnStart";
    bool res = Publish(this);
    if (!res) {
        LOG(ERROR) << "SysInstaller OnStart failed";
    }

    return;
}

void SysInstaller::OnStop()
{
    LOG(INFO) << "OnStop";
}
} // namespace SysInstaller
} // namespace OHOS
