/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SYS_INSTALLER_TIMER_MANAGER_H
#define SYS_INSTALLER_TIMER_MANAGER_H

#include "sys_installer_timer_base.h"

namespace OHOS {
namespace SysInstaller {

uint64_t GetSystemBootTime();

class SysInstallerTimerManager final {
public:
    static uint64_t RegisterTimer(uint64_t delayMs, const TimerCallback &callback);
    static uint64_t RegisterRepeatTimer(uint64_t delayMs, uint64_t intervalMs, const TimerCallback &callback);
    static void UnRegisterTimer(uint64_t timerId);
};

} // namespace SysInstaller
} // namespace OHOS

#endif // SYS_INSTALLER_TIMER_MANAGER_H
