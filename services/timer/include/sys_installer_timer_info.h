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

#ifndef SYS_INSTALLER_TIMER_INFO_H
#define SYS_INSTALLER_TIMER_INFO_H

#include <string>

#include "itimer_info.h"
#include "sys_installer_timer_base.h"

namespace OHOS {
namespace SysInstaller {

class SysInstallerTimerInfo : public OHOS::MiscServices::ITimerInfo {
public:
    explicit SysInstallerTimerInfo(const TimerCallback &callback);
    virtual ~SysInstallerTimerInfo() = default;

    std::string ToString() const;
    void SetType(const int &type) override;
    void SetRepeat(bool repeat) override;
    void SetInterval(const uint64_t &interval) override;
    void SetWantAgent(std::shared_ptr<OHOS::AbilityRuntime::WantAgent::WantAgent> wantAgent) override;

protected:
    void OnTrigger() override;

private:
    TimerCallback callback_{};
};

}
}
#endif // SYS_INSTALLER_TIMER_INFO_H
