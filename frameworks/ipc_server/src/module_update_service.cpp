/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "module_update_service.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "directory_ex.h"
#include "init_reboot.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "module_utils.h"
#include "json_node.h"
#include "log/log.h"
#include "module_constants.h"
#include "module_file.h"
#include "module_error_code.h"
#include "module_update_main.h"
#include "package/package.h"
#include "init_reboot.h"
#include "scope_guard.h"
#include "system_ability_definition.h"
#include "utils.h"
#include "unique_fd.h"
#ifdef WITH_SELINUX
#include <policycoreutils.h>
#endif // WITH_SELINUX

namespace OHOS {
namespace SysInstaller {
REGISTER_SYSTEM_ABILITY_BY_ID(ModuleUpdateService, MODULE_UPDATE_SERVICE_ID, false)

using namespace Updater;

ModuleUpdateService::ModuleUpdateService(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(systemAbilityId, runOnCreate)
{
    LOG(INFO) << "ModuleUpdateService begin";
}

ModuleUpdateService::~ModuleUpdateService()
{
    LOG(INFO) << "ModuleUpdateService end";
}

int32_t ModuleUpdateService::InstallModulePackage(const std::string &pkgPath)
{
    LOG(INFO) << "InstallModulePackage " << pkgPath;
    std::string realPath;
    if (!CheckFileSuffix(pkgPath, HMP_PACKAGE_SUFFIX) || !PathToRealPath(pkgPath, realPath)) {
        LOG(ERROR) << "Invalid package path " << pkgPath;
        return ModuleErrorCode::ERR_INVALID_PATH;
    }
    return ModuleUpdateMain::GetInstance().ReallyInstallModulePackage(realPath, nullptr);
}

int32_t ModuleUpdateService::UninstallModulePackage(const std::string &hmpName)
{
    LOG(INFO) << "UninstallModulePackage " << hmpName;
    return ModuleUpdateMain::GetInstance().UninstallModulePackage(hmpName);
}

int32_t ModuleUpdateService::GetModulePackageInfo(const std::string &hmpName,
    std::list<ModulePackageInfo> &modulePackageInfos)
{
    LOG(INFO) << "GetModulePackageInfo " << hmpName;
    return ModuleUpdateMain::GetInstance().GetModulePackageInfo(hmpName, modulePackageInfos);
}


int32_t ModuleUpdateService::ExitModuleUpdate()
{
    LOG(INFO) << "ExitModuleUpdate";
    ModuleUpdateMain::GetInstance().ExitModuleUpdate();
    sptr<ISystemAbilityManager> sm = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (sm == nullptr) {
        LOG(ERROR) << "GetSystemAbilityManager samgr object null!";
        return 0;
    }
    if (sm->UnloadSystemAbility(MODULE_UPDATE_SERVICE_ID) != 0) {
        LOG(ERROR) << "UnloadSystemAbility error!";
    }
    return 0;
}

std::vector<HmpVersionInfo> ModuleUpdateService::GetHmpVersionInfo()
{
    LOG(INFO) << "GetHmpVersionInfo";
    return ModuleUpdateMain::GetInstance().GetHmpVersionInfo();
}

int32_t ModuleUpdateService::StartUpdateHmpPackage(const std::string &path,
    const sptr<ISysInstallerCallback> &updateCallback)
{
    int32_t ret = -1;
    Timer timer;
    ON_SCOPE_EXIT(saveResult) {
        ModuleUpdateMain::GetInstance().SaveInstallerResult(path, ret, std::to_string(ret), timer);
        if (updateCallback != nullptr) {
            updateCallback->OnUpgradeProgress(ret == 0 ? UPDATE_STATE_SUCCESSFUL : UPDATE_STATE_FAILED,
                100, ""); // 100 : 100% percent
        }
    };
    LOG(INFO) << "StartUpdateHmpPackage " << path;
    if (updateCallback == nullptr) {
        LOG(ERROR) << "StartUpdateHmpPackage updateCallback null";
        ret = ModuleErrorCode::ERR_INVALID_PATH;
        return ret;
    }

    updateCallback->OnUpgradeProgress(UPDATE_STATE_ONGOING, 0, "");
    if (VerifyModulePackageSign(path) != 0) {
        LOG(ERROR) << "Verify sign failed " << path;
        ret = ModuleErrorCode::ERR_VERIFY_SIGN_FAIL;
        return ret;
    }

    ret = InstallModulePackage(path);
    return ret;
}

std::vector<HmpUpdateInfo> ModuleUpdateService::GetHmpUpdateResult()
{
    LOG(INFO) << "GetHmpUpdateResult";
    std::vector<HmpUpdateInfo> updateInfo {};
    std::ifstream ifs { MODULE_RESULT_PATH };
    if (!ifs.is_open()) {
        LOG(ERROR) << "open " << MODULE_RESULT_PATH << " failed";
        return updateInfo;
    }
    std::string resultInfo {std::istreambuf_iterator<char> {ifs}, {}};
    std::vector<std::string> results {};
    SplitStr(resultInfo, "\n", results);
    for (auto &result : results) {
        HmpUpdateInfo tmpUpdateInfo {};
        std::vector<std::string> signalResult {};
        SplitStr(result, ";", signalResult);
        if (signalResult.size() < 3) { // 3: pkg; result; result info
            LOG(ERROR) << "parse " << result << " failed";
            continue;
        }
        tmpUpdateInfo.path = signalResult[0];
        tmpUpdateInfo.result = stoi(signalResult[1]);
        tmpUpdateInfo.resultMsg = signalResult[2]; // 2: result info
        bool isFind = false;
        for (auto &iter : updateInfo) {
            if (iter.path.find(tmpUpdateInfo.path) != std::string::npos) {
                iter.result = tmpUpdateInfo.result;
                iter.resultMsg = tmpUpdateInfo.resultMsg;
                isFind = true;
                break;
            }
        }
        if (!isFind) {
            updateInfo.emplace_back(tmpUpdateInfo);
        }
    }
    ifs.close();
    (void)unlink(MODULE_RESULT_PATH);
    LOG(INFO) << "after get hmpUpdateResult, delete module_update_result.";
    return updateInfo;
}

void ModuleUpdateService::OnStart(const SystemAbilityOnDemandReason &startReason)
{
    InitUpdaterLogger("ModuleUpdaterServer", "", "", "");
    LOG(INFO) << "OnStart, startReason name: " << startReason.GetName() << ", id: " <<
        static_cast<int32_t>(startReason.GetId()) << ", value: " << startReason.GetValue();
    SysInstaller::ModuleUpdateMain& moduleUpdate = SysInstaller::ModuleUpdateMain::GetInstance();
    moduleUpdate.ScanPreInstalledHmp();
    bool res = Publish(this);
    if (!res) {
        LOG(ERROR) << "OnStart failed";
    }
    if ((strcmp(startReason.GetName().c_str(), SA_START) == 0 &&
        strcmp(startReason.GetValue().c_str(), SA_ABNORMAL) == 0) ||
        (strcmp(startReason.GetName().c_str(), BMS_START_INSTALL) == 0 &&
        strcmp(startReason.GetValue().c_str(), BMS_REVERT) == 0)) {
        moduleUpdate.Start();
    }
    LOG(INFO) << "OnStart done";
}

void ModuleUpdateService::OnStop(const SystemAbilityOnDemandReason &stopReason)
{
    LOG(INFO) << "OnStop, stopReason name: " << stopReason.GetName() << ", id: " <<
        static_cast<int32_t>(stopReason.GetId()) << ", value: " << stopReason.GetValue();
    ModuleUpdateMain::GetInstance().ExitModuleUpdate();
}
} // namespace SysInstaller
} // namespace OHOS
