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

#include "module_update_service.h"
#include "iservice_registry.h"
#include "singleton.h"

namespace OHOS {
namespace SysInstaller {
class ModuleUpdateMain final : public Singleton<ModuleUpdateMain> {
    DECLARE_SINGLETON(ModuleUpdateMain);
public:
    void Start();
    bool RegisterModuleUpdateService();

private:
    void BuildSaIdHmpMap(std::unordered_map<int32_t, std::string> &saIdHmpMap);
    sptr<ISystemAbilityManager> &GetSystemAbilityManager();

    sptr<ISystemAbilityManager> samgr_ = nullptr;
    sptr<ModuleUpdateService> moduleUpdate_ = nullptr;
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_UPDATE_MAIN_H