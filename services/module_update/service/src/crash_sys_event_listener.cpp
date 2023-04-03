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

#include "crash_sys_event_listener.h"

#include "log/log.h"
#include "module_update_main.h"

namespace OHOS {
namespace SysInstaller {
using namespace HiviewDFX;
using namespace Updater;

void CrashSysEventListener::OnEvent(std::shared_ptr<HiSysEventRecord> sysEvent)
{
    LOG(INFO) << "CrashSysEventListener OnEvent";
    if (sysEvent == nullptr) {
        LOG(ERROR) << "sysEvent is null";
        return;
    }
    if (strcmp(sysEvent->GetDomain().c_str(), CRASH_DOMAIN) == 0
        && strcmp(sysEvent->GetEventName().c_str(), CRASH_NAME) == 0
        && sysEvent->GetEventType() == CRASH_TYPE) {
        std::string processName;
        int ret = sysEvent->GetParamValue(PROCESS_NAME_KEY, processName);
        if (ret != 0) {
            LOG(ERROR) << "Get process name failed " << ret;
            return;
        }
        ModuleUpdateMain::GetInstance().OnProcessCrash(processName);
    }
}

void CrashSysEventListener::OnServiceDied()
{
    LOG(INFO) << "CrashSysEventListener OnServiceDied";
    ModuleUpdateMain::GetInstance().OnSysEventServiceDied();
}
} // namespace SysInstaller
} // namespace OHOS