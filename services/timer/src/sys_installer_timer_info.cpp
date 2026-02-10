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

#include "sys_installer_timer_info.h"

#include <sstream>

namespace OHOS {
namespace SysInstaller {

SysInstallerTimerInfo::SysInstallerTimerInfo(const TimerCallback &callback)
    : callback_(callback)
{
}

std::string SysInstallerTimerInfo::ToString() const
{
    std::ostringstream oss;
    oss << "TimerInfo [" << "type:" << this->type << ", repeat:" << std::boolalpha << this->repeat <<
        ", interval:" << this->interval << "]";
    return oss.str();
}

void SysInstallerTimerInfo::SetType(const int &type)
{
    this->type = type;
}

void SysInstallerTimerInfo::SetRepeat(bool repeat)
{
    this->repeat = repeat;
}

void SysInstallerTimerInfo::SetInterval(const uint64_t &interval)
{
    this->interval = interval;
}

void SysInstallerTimerInfo::SetWantAgent(std::shared_ptr<OHOS::AbilityRuntime::WantAgent::WantAgent> wantAgent)
{
    this->wantAgent = wantAgent;
}

void SysInstallerTimerInfo::OnTrigger()
{
    if (callback_) {
        callback_();
    }
}


} // namespace SysInstaller
} // namespace OHOS
