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

#include "directory_ex.h"
#include "hisysevent_manager.h"
#include "log/log.h"
#include "module_constants.h"
#include "module_update_consumer.h"
#include "module_update_producer.h"
#include "module_update_main.h"
#include "module_update_service.h"
#include "module_utils.h"
#include "parameter.h"
#include "system_ability_definition.h"
#include <unistd.h>

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

namespace {
constexpr int32_t RETRY_TIMES_FOR_SAMGR = 10;
constexpr std::chrono::milliseconds MILLISECONDS_WAITING_SAMGR_ONE_TIME(100);
}

ModuleUpdateMain::ModuleUpdateMain()
{
    moduleUpdate_ = new ModuleUpdateService();
    moduleUpdate_->ScanPreInstalledHmp();
}

ModuleUpdateMain::~ModuleUpdateMain() = default;

bool ModuleUpdateMain::RegisterModuleUpdateService()
{
    LOG(INFO) << "RegisterModuleUpdateService";
    auto samgr = GetSystemAbilityManager();
    if (samgr == nullptr) {
        LOG(ERROR) << "Failed to get system ability manager";
        return false;
    }
    int32_t ret = samgr->AddSystemAbility(MODULE_UPDATE_SERVICE_ID, moduleUpdate_);
    if (ret != 0) {
        LOG(ERROR) << "AddSystemAbility error " << ret;
        return false;
    }
    return true;
}

sptr<ISystemAbilityManager> &ModuleUpdateMain::GetSystemAbilityManager()
{
    if (samgr_ != nullptr) {
        return samgr_;
    }
    int32_t times = RETRY_TIMES_FOR_SAMGR;
    constexpr int32_t duration = std::chrono::microseconds(MILLISECONDS_WAITING_SAMGR_ONE_TIME).count();
    while (times > 0) {
        times--;
        samgr_ = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (samgr_ == nullptr) {
            LOG(INFO) << "waiting for samgr";
            usleep(duration);
        } else {
            break;
        }
    }
    return samgr_;
}

void ModuleUpdateMain::BuildSaIdHmpMap(std::unordered_map<int32_t, std::string> &saIdHmpMap)
{
    std::vector<std::string> files;
    GetDirFiles(MODULE_PREINSTALL_DIR, files);
    for (auto &file : files) {
        if (!CheckFileSuffix(file, MODULE_PACKAGE_SUFFIX)) {
            continue;
        }
        std::unique_ptr<ModuleFile> moduleFile = ModuleFile::Open(file);
        if (moduleFile == nullptr) {
            continue;
        }
        std::string hmpName = GetHmpName(file);
        if (hmpName.empty()) {
            continue;
        }
        saIdHmpMap.emplace(moduleFile->GetSaId(), hmpName);
    }
}

void ModuleUpdateMain::Start()
{
    LOG(INFO) << "ModuleUpdateMain Start";
    std::unordered_map<int32_t, std::string> saIdHmpMap;
    BuildSaIdHmpMap(saIdHmpMap);
    ModuleUpdateQueue queue;
    ModuleUpdateProducer producer(queue, saIdHmpMap);
    ModuleUpdateConsumer consumer(queue, saIdHmpMap);
    std::thread produceThread(std::bind(&ModuleUpdateProducer::Run, &producer));
    std::thread consumeThread(std::bind(&ModuleUpdateConsumer::Run, &consumer));
    consumeThread.join();
    produceThread.join();
    LOG(INFO) << "module update main exit";
}
} // namespace SysInstaller
} // namespace OHOS