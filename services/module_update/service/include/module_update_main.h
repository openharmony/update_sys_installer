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
#ifndef SYS_INSTALLER_MODULE_UPDATE_MAIN_H
#define SYS_INSTALLER_MODULE_UPDATE_MAIN_H

#include "crash_sys_event_listener.h"
#include "iservice_registry.h"
#include "module_update_service.h"
#include "singleton.h"
#include "sys_event_service_listener.h"

namespace OHOS {
namespace SysInstaller {
using namespace HiviewDFX;

class ModuleUpdateMain final : public Singleton<ModuleUpdateMain> {
    DECLARE_SINGLETON(ModuleUpdateMain);

public:
    bool RegisterModuleUpdateService();
    bool WaitForSysEventService();
    bool CheckBootComplete() const;
    void WatchBootComplete() const;
    bool RegisterSysEventListener();
    void OnSysEventServiceDied();
    void OnProcessCrash(const std::string &processName);

private:
    static void BootCompleteCallback(const char *key, const char *value, void *context);
    void OnBootCompleted();
    sptr<ISystemAbilityManager> &GetSystemAbilityManager();

    sptr<ISystemAbilityManager> samgr_ = nullptr;
    sptr<ModuleUpdateService> moduleUpdate_ = nullptr;
    sptr<SysEventServiceListener> sysEventListener_ = nullptr;
    std::shared_ptr<CrashSysEventListener> crashListener_ = nullptr;
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_UPDATE_MAIN_H