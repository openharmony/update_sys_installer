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
#ifndef SYS_INSTALLER_CRASH_SYS_EVENT_CALLBACK_H
#define SYS_INSTALLER_CRASH_SYS_EVENT_CALLBACK_H

#include "hisysevent_listener.h"

namespace OHOS {
namespace SysInstaller {
using namespace HiviewDFX;

static constexpr const char *PROCESS_NAME_KEY = "PNAME";
static constexpr const char *CRASH_DOMAIN = "RELIABILITY";
static constexpr const char *CRASH_NAME = "CPP_CRASH";
static const HiSysEvent::EventType CRASH_TYPE = HiSysEvent::EventType::FAULT;

class CrashSysEventListener : public HiSysEventListener {
public:
    void OnEvent(std::shared_ptr<HiSysEventRecord> sysEvent) override;
    void OnServiceDied() override;
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_CRASH_SYS_EVENT_CALLBACK_H