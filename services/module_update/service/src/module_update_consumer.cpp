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

#include "log/log.h"
#include "module_constants.h"
#include "module_update.h"
#include "module_update_consumer.h"
#include "module_utils.h"
#include "parameter.h"
#include <vector>

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

ModuleUpdateConsumer::ModuleUpdateConsumer(ModuleUpdateQueue &queue,
    std::unordered_map<int32_t, std::string> &saIdHmpMap, volatile sig_atomic_t &exit)
    : queue_(queue),
      saIdHmpMap_(saIdHmpMap),
      exit_(exit) {}

void ModuleUpdateConsumer::DoRevert(const std::string &hmpName, const int32_t &saId)
{
    LOG(INFO) << "hmp package revert,hmp name=" << hmpName << "; said=" << saId;
    hmpSet_.insert(hmpName);
    Revert(hmpName, !IsHotSa(saId));
}

void ModuleUpdateConsumer::DoUnload(const std::string &hmpName, const int32_t &saId)
{
    LOG(INFO) << "hmp package unload,hmp name=" << hmpName << "; said=" << saId;
    ModuleUpdateStatus status;
    status.hmpName = hmpName;
    status.isHotInstall = true;
    if (IsRunning(saId)) {
        LOG(INFO) << "sa is running, saId=" << saId;
        return;
    }
    if (ModuleUpdate::GetInstance().DoModuleUpdate(status)) {
        unLoadHmpSet_.insert(hmpName);
        LOG(INFO) << "hmp package successful install, hmp name=" << hmpName << "; said=" << saId;
    } else {
        LOG(ERROR) << "hmp package fail install, hmp name=" << hmpName << "; said=" << saId;
    }
}

void ModuleUpdateConsumer::Run()
{
    do {
        if (exit_ == 1 && queue_.IsEmpty()) {
            LOG(INFO) << "producer and consumer stop";
            queue_.Stop();
            break;
        }
        std::pair<int32_t, std::string> saStatusPair = queue_.Pop();
        int32_t saId = saStatusPair.first;
        std::string saStatus = saStatusPair.second;
        auto it = saIdHmpMap_.find(saId);
        if (it == saIdHmpMap_.end() || it->second == "") {
            LOG(ERROR) << "find hmp fail, saId=" << saId;
            continue;
        }
        std::string hmpName = it->second;
        if (hmpSet_.find(hmpName) != hmpSet_.end() || unLoadHmpSet_.find(hmpName) != unLoadHmpSet_.end()) {
            LOG(INFO) << "hmp has been reverted or unloaded, hmp name=" << hmpName << "; said=" << saId;
            continue;
        }
        if (strcmp(saStatus.c_str(), LOAD_FAIL) == 0 || strcmp(saStatus.c_str(), CRASH) == 0) {
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