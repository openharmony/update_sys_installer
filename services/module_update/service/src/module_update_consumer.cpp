/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "module_update_consumer.h"
#include <vector>
#include "directory_ex.h"
#include "log/log.h"
#include "module_constants.h"
#include "module_error_code.h"
#include "module_update.h"
#include "module_update_main.h"
#include "module_utils.h"
#include "parameter.h"
#include "scope_guard.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

ModuleUpdateConsumer::ModuleUpdateConsumer(ModuleUpdateQueue &queue,
    std::unordered_map<int32_t, std::string> &saIdHmpMap, volatile sig_atomic_t &exit)
    : queue_(queue),
      saIdHmpMap_(saIdHmpMap),
      exit_(exit) {}

void ModuleUpdateConsumer::DoInstall(ModuleUpdateStatus &status)
{
    ON_SCOPE_EXIT(rmdir) {
        RemoveSpecifiedDir(std::string(UPDATE_INSTALL_DIR) + "/" + status.hmpName);
    };
    if (ModuleUpdate::GetInstance().DoModuleUpdate(status)) {
        LOG(INFO) << "hmp package successful install, hmp name=" << status.hmpName;
    } else {
        LOG(ERROR) << "hmp package fail install, hmp name=" << status.hmpName;
    }
}

void ModuleUpdateConsumer::DoRevert(const std::string &hmpName, int32_t saId)
{
    LOG(INFO) << "hmp package revert,hmp name=" << hmpName << "; said=" << saId;
    bool isHotHmp = IsHotHmpPackage(hmpName);
    ModuleUpdateStatus status;
    status.hmpName = hmpName;
    status.isHotInstall = isHotHmp;
    Revert(hmpName, !isHotHmp);
    DoInstall(status);
}

void ModuleUpdateConsumer::DoUnload(const std::string &hmpName, int32_t saId)
{
    LOG(INFO) << "hmp package unload,hmp name=" << hmpName << "; said=" << saId;
    ModuleUpdateStatus status;
    status.hmpName = hmpName;
    status.isHotInstall = true;
    if (IsRunning(saId)) {
        LOG(INFO) << "sa is running, saId=" << saId;
        return;
    }
    // check whether install hmp exists
    DoInstall(status);
}

void ModuleUpdateConsumer::Run()
{
    LOG(INFO) << "ModuleUpdateConsumer Consume";
    do {
        if (exit_ == 1 && queue_.IsEmpty()) {
            queue_.Stop();
            break;
        }
        std::pair<int32_t, std::string> saStatusPair = queue_.Pop();
        if (saStatusPair.first == 0 && saStatusPair.second == "") {
            LOG(INFO) << "producer and consumer stop";
            break;
        }
        Timer timer;
        if (saStatusPair.first == APP_SERIAL_NUMBER) {
            ModuleUpdateMain::GetInstance().SaveInstallerResult(saStatusPair.second, ModuleErrorCode::ERR_BMS_REVERT,
                saStatusPair.second + " revert", timer);
            DoRevert(saStatusPair.second, APP_SERIAL_NUMBER);
            continue;
        }
        int32_t saId = saStatusPair.first;
        std::string saStatus = saStatusPair.second;
        auto it = saIdHmpMap_.find(saId);
        if (it == saIdHmpMap_.end() || it->second == "") {
            LOG(ERROR) << "find hmp fail, saId=" << saId;
            continue;
        }
        std::string hmpName = it->second;
        if (strcmp(saStatus.c_str(), LOAD_FAIL) == 0 || strcmp(saStatus.c_str(), CRASH) == 0) {
            ModuleUpdateMain::GetInstance().SaveInstallerResult(hmpName, ModuleErrorCode::ERR_SAMGR_REVERT,
                std::to_string(saId) + " revert", timer);
            DoRevert(hmpName, saId);
        } else if (IsHotSa(saId) && strcmp(saStatus.c_str(), UNLOAD) == 0) {
            DoUnload(hmpName, saId);
        } else {
            LOG(ERROR) << "sa status not exist, said=" << saId;
        }
    } while (true);
    LOG(INFO) << "consumer exit";
}
} // SysInstaller
} // namespace OHOS