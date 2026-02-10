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

#include "sys_installer_timer_manager.h"
#include <chrono>
#include <memory>

#include "log/log.h"
#include "sys_installer_timer_info.h"
#include "time_service_client.h"

using OHOS::MiscServices::ITimerInfo;
using OHOS::MiscServices::TimeServiceClient;

namespace {
using namespace Updater;
using OHOS::SysInstaller::TimerCallback;
using OHOS::SysInstaller::SysInstallerTimerInfo;
using OHOS::SysInstaller::SysInstallerTimerManager;

uint64_t StartCoreTimer(uint64_t triggerTime, uint64_t intervalMs, const TimerCallback &callback)
{
    const bool repeat = (intervalMs != 0);
    std::shared_ptr<SysInstallerTimerInfo> timerInfo = std::make_shared<SysInstallerTimerInfo>(callback);
    timerInfo->SetRepeat(repeat);
    if (repeat) {
        timerInfo->SetType(timerInfo->TIMER_TYPE_REALTIME);
        timerInfo->SetInterval(intervalMs);
    } else {
        const uint32_t timerType = static_cast<uint32_t>(timerInfo->TIMER_TYPE_EXACT) |
            static_cast<uint32_t>(timerInfo->TIMER_TYPE_WAKEUP) |
            static_cast<uint32_t>(timerInfo->TIMER_TYPE_REALTIME);
        timerInfo->SetType(static_cast<int32_t>(timerType));
    }
    OHOS::sptr<TimeServiceClient> coreTimer = TimeServiceClient::GetInstance();
    if (coreTimer == nullptr) {
        LOG(ERROR) << "coreTimer is nullptr";
        return 0;
    }
    uint64_t timerId = coreTimer->CreateTimer(timerInfo);
    if (timerId == 0) {
        LOG(ERROR) << "Create timer failed, triggerTime: " << triggerTime << ", " << timerInfo->ToString();
        return 0;
    }
    if (!coreTimer->StartTimer(timerId, triggerTime)) {
        LOG(ERROR) << "Start timer failed, triggerTime: " << triggerTime << ", " << timerInfo->ToString();
        coreTimer->DestroyTimer(timerId);
        return 0;
    }
    LOG(INFO) << "Start timer success, triggerTime: " << triggerTime << ", " << timerInfo->ToString();
    return timerId;
}

}

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

uint64_t GetSystemBootTime()
{
    int64_t currentTime = 0;
    OHOS::sptr<TimeServiceClient> coreTimer = TimeServiceClient::GetInstance();
    if (coreTimer == nullptr) {
        LOG(ERROR) << "coreTimer is nullptr";
        return 0;
    }
    int32_t ret = coreTimer->GetBootTimeMs(currentTime);
    if (ret != 0) {
        LOG(ERROR) << "Get boot time failed, ret: " << ret;
        return 0;
    }
    return static_cast<uint64_t>(currentTime);
}

uint64_t SysInstallerTimerManager::RegisterTimer(uint64_t delayMs, const TimerCallback &callback)
{
    const uint64_t now = GetSystemBootTime();
    if (now == 0) {
        LOG(ERROR) << "Register timer failed, delayMs: " << delayMs;
        return 0;
    }
    const uint64_t triggerTime = now + delayMs;
    return StartCoreTimer(triggerTime, 0, callback);
}

uint64_t SysInstallerTimerManager::RegisterRepeatTimer(uint64_t delayMs, uint64_t intervalMs,
    const TimerCallback &callback)
{
    if (intervalMs == 0) {
        LOG(ERROR) << "Register repeat timer failed, intervalMs cannot be 0";
        return 0;
    }
    const uint64_t now = GetSystemBootTime();
    if (now == 0) {
        LOG(ERROR) << "Register repeat timer failed, delayMs: " << delayMs << ", intervalMs: " << intervalMs;
        return 0;
    }
    const uint64_t triggerTime = now + delayMs;
    return StartCoreTimer(triggerTime, intervalMs, callback);
}

void SysInstallerTimerManager::UnRegisterTimer(uint64_t timerId)
{
    LOG(INFO) << "Unregister timer, timerId: " << timerId;
    OHOS::sptr<TimeServiceClient> coreTimer = TimeServiceClient::GetInstance();
    if (coreTimer == nullptr) {
        LOG(ERROR) << "Unregister timer failed, coreTimer is nullptr";
        return;
    }
    coreTimer->DestroyTimer(timerId);
}

} // namespace SysInstaller
}  // namespace OHOS
