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

#include "module_update_main.h"

#include "hisysevent_manager.h"
#include "log/log.h"
#include "module_update_service.h"
#include "parameter.h"
#include "sys_event_service_listener.h"
#include "system_ability_definition.h"

#include <unistd.h>

namespace OHOS {
namespace SysInstaller {
using namespace HiviewDFX;
using namespace Updater;

namespace {
constexpr const char *BOOT_COMPLETE_PARAM = "bootevent.boot.completed";
constexpr const char *BOOT_SUCCESS_VALUE = "true";
constexpr int32_t PARAM_VALUE_SIZE = 10;
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

bool ModuleUpdateMain::WaitForSysEventService()
{
    LOG(INFO) << "WaitForSysEventService";
    auto samgr = GetSystemAbilityManager();
    if (samgr == nullptr) {
        LOG(ERROR) << "Failed to get system ability manager";
        return false;
    }
    sysEventListener_ = new SysEventServiceListener();
    if (sysEventListener_ == nullptr) {
        LOG(ERROR) << "Failed to new SysEventServiceListener";
        return false;
    }
    int32_t ret = samgr->SubscribeSystemAbility(DFX_SYS_EVENT_SERVICE_ABILITY_ID, sysEventListener_);
    if (ret != 0) {
        LOG(ERROR) << "SubscribeSystemAbility error " << ret;
        return false;
    }
    return true;
}

bool ModuleUpdateMain::CheckBootComplete() const
{
    char value[PARAM_VALUE_SIZE] = "";
    int ret = GetParameter(BOOT_COMPLETE_PARAM, "", value, PARAM_VALUE_SIZE);
    if (ret < 0) {
        LOG(ERROR) << "Failed to get parameter " << BOOT_COMPLETE_PARAM;
        return false;
    }
    return strcmp(value, BOOT_SUCCESS_VALUE) == 0;
}

void ModuleUpdateMain::WatchBootComplete() const
{
    LOG(INFO) << "WatchBootComplete";
    int ret = WatchParameter(BOOT_COMPLETE_PARAM, BootCompleteCallback, nullptr);
    if (ret == -1) {
        LOG(ERROR) << "Failed to watch parameter " << BOOT_COMPLETE_PARAM;
    }
}

bool ModuleUpdateMain::RegisterSysEventListener()
{
    LOG(INFO) << "RegisterSysEventListener";
    crashListener_ = std::make_shared<CrashSysEventListener>();
    std::vector<ListenerRule> rules;
    rules.emplace_back(CRASH_DOMAIN, CRASH_NAME, "", RuleType::WHOLE_WORD, static_cast<uint32_t>(CRASH_TYPE));
    int32_t ret = HiSysEventManager::AddListener(crashListener_, rules);
    if (ret != 0) {
        LOG(ERROR) << "HiSysEventManager::AddListener error " << ret;
        return false;
    }
    return true;
}

void ModuleUpdateMain::OnSysEventServiceDied()
{
    crashListener_ = nullptr;
}

void ModuleUpdateMain::OnProcessCrash(const std::string &processName)
{
    LOG(INFO) << "OnProcessCrash " << processName;
    moduleUpdate_->OnProcessCrash(processName);
}

void ModuleUpdateMain::BootCompleteCallback(const char *key, const char *value, void *context)
{
    LOG(INFO) << "BootCompleteCallback key=" << key << ", value=" << value;
    if (strcmp(key, BOOT_COMPLETE_PARAM) != 0 || strcmp(value, BOOT_SUCCESS_VALUE) != 0) {
        return;
    }
    ModuleUpdateMain::GetInstance().OnBootCompleted();
}

void ModuleUpdateMain::OnBootCompleted()
{
    LOG(INFO) << "OnBootCompleted";
    if (crashListener_ != nullptr) {
        int32_t ret = HiSysEventManager::RemoveListener(crashListener_);
        if (ret != 0) {
            LOG(ERROR) << "HiSysEventManager::RemoveListener error " << ret;
        }
    }
    if (sysEventListener_ != nullptr) {
        int32_t ret = samgr_->UnSubscribeSystemAbility(DFX_SYS_EVENT_SERVICE_ABILITY_ID, sysEventListener_);
        if (ret != 0) {
            LOG(ERROR) << "UnSubscribeSystemAbility sysEventListener failed, ret is  " << ret;
        }
    }
    moduleUpdate_->OnBootCompleted();
}

sptr<ISystemAbilityManager> &ModuleUpdateMain::GetSystemAbilityManager()
{
    if (samgr_ != nullptr) {
        return samgr_;
    }
    int32_t times = RETRY_TIMES_FOR_SAMGR;
    constexpr int32_t duration = std::chrono::microseconds(MILLISECONDS_WAITING_SAMGR_ONE_TIME).count();
    LOG(INFO) << "waiting for samgr";
    while (times > 0) {
        times--;
        samgr_ = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (samgr_ == nullptr) {
            usleep(duration);
        } else {
            break;
        }
    }
    return samgr_;
}
} // namespace SysInstaller
} // namespace OHOS