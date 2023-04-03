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

#include "ipc_skeleton.h"
#include "log/log.h"
#include "module_update_main.h"

using namespace OHOS;
using namespace Updater;

int main()
{
    LOG(INFO) << "ModuleUpdateService main called";
    SysInstaller::ModuleUpdateMain& moduleUpdate = SysInstaller::ModuleUpdateMain::GetInstance();
    if (!moduleUpdate.RegisterModuleUpdateService()) {
        LOG(ERROR) << "Failed to register module update service";
        return -1;
    }
    if (!moduleUpdate.CheckBootComplete()) {
        if (!moduleUpdate.WaitForSysEventService()) {
            LOG(ERROR) << "Failed to wait for sysevent service";
            return -1;
        }
        moduleUpdate.WatchBootComplete();
    }

    IPCSkeleton::JoinWorkThread();
    return 0;
}