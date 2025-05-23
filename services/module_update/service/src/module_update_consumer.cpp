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

void ModuleUpdateConsumer::DoRevert(int32_t code) const
{
    std::string revertHmpName = GetCurrentHmpName();
    if (revertHmpName.empty()) {
        LOG(ERROR) << "hmp package revert, hmpName is empty.";
        return;
    }
    Timer timer;
    ModuleUpdateMain::GetInstance().SaveInstallerResult(revertHmpName, code, "revert", timer);
    NotifyBmsRevert(revertHmpName, false);
    Revert(revertHmpName, true);
}

void ModuleUpdateConsumer::Run()
{
    LOG(INFO) << "ModuleUpdateConsumer Consume";
    ModuleErrorCode code = ModuleErrorCode::ERR_BMS_REVERT;
    bool revertFlag = false;
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
        if (saStatusPair.first == APP_SERIAL_NUMBER) {
            LOG(INFO) << "hmp package revert, module name=" << saStatusPair.second;
            revertFlag = true;
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
            code = ModuleErrorCode::ERR_SAMGR_REVERT;
            LOG(INFO) << "hmp package revert, module name=" << hmpName << "; said=" << saId;
            revertFlag = true;
        } else if (IsHotSa(saId) && strcmp(saStatus.c_str(), UNLOAD) == 0) {
            LOG(INFO) << "sa is unload, said=" << saId;
        } else {
            LOG(ERROR) << "sa status not exist, said=" << saId;
        }
    } while (true);
    if (revertFlag) {
        DoRevert(code);
    }
    LOG(INFO) << "consumer exit";
}
} // SysInstaller
} // namespace OHOS