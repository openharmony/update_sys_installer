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

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "gtest/gtest.h"
#include "log/log.h"
#include "sys_installer_timer_info.h"

namespace {
using namespace testing;
using namespace testing::ext;
using namespace Updater;
using namespace OHOS::SysInstaller;
using OHOS::MiscServices::ITimerInfo;

class SysInstallerTimerInfoUnitTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void SysInstallerTimerInfoUnitTest::SetUpTestCase()
{
    SetLogLevel(DEBUG);
    InitUpdaterLogger("UPDATER", "updater_log.log", "updater_status.log", "error_code.log");
}

void SysInstallerTimerInfoUnitTest::TearDownTestCase()
{
}

void SysInstallerTimerInfoUnitTest::SetUp()
{
}

void SysInstallerTimerInfoUnitTest::TearDown()
{
}

HWTEST_F(SysInstallerTimerInfoUnitTest, test_CallbackIsNotNull, TestSize.Level0)
{
    bool callbackCalled = false;
    std::shared_ptr<ITimerInfo> timerInfo = std::make_shared<SysInstallerTimerInfo>(
        [&callbackCalled]() {
            callbackCalled = true;
        });
    timerInfo->OnTrigger();
    EXPECT_TRUE(callbackCalled);
}

HWTEST_F(SysInstallerTimerInfoUnitTest, test_CallbackIsNull, TestSize.Level0)
{
    std::shared_ptr<ITimerInfo> timerInfo = std::make_shared<SysInstallerTimerInfo>(nullptr);
    timerInfo->OnTrigger();
    GTEST_SUCCEED();
}

HWTEST_F(SysInstallerTimerInfoUnitTest, test_SetRepeat, TestSize.Level0)
{
    std::shared_ptr<ITimerInfo> timerInfo = std::make_shared<SysInstallerTimerInfo>(nullptr);
    timerInfo->SetRepeat(true);
    EXPECT_EQ(timerInfo->repeat, true);

    timerInfo->SetRepeat(false);
    EXPECT_EQ(timerInfo->repeat, false);
}

HWTEST_F(SysInstallerTimerInfoUnitTest, test_SetWantAgent, TestSize.Level0)
{
    std::shared_ptr<ITimerInfo> timerInfo = std::make_shared<SysInstallerTimerInfo>(nullptr);
    EXPECT_EQ(timerInfo->wantAgent, nullptr);
    timerInfo->SetWantAgent(nullptr);
    EXPECT_EQ(timerInfo->wantAgent, nullptr);

    auto wantAgent = std::make_shared<OHOS::AbilityRuntime::WantAgent::WantAgent>();
    timerInfo->SetWantAgent(wantAgent);
    EXPECT_EQ(timerInfo->wantAgent, wantAgent);
}

HWTEST_F(SysInstallerTimerInfoUnitTest, test_SetInterval, TestSize.Level0)
{
    std::shared_ptr<ITimerInfo> timerInfo = std::make_shared<SysInstallerTimerInfo>(nullptr);
    timerInfo->SetInterval(1000);
    EXPECT_EQ(timerInfo->interval, 1000);

    timerInfo->SetInterval(5000);
    EXPECT_EQ(timerInfo->interval, 5000);
}

HWTEST_F(SysInstallerTimerInfoUnitTest, test_SetType, TestSize.Level0)
{
    std::shared_ptr<ITimerInfo> timerInfo = std::make_shared<SysInstallerTimerInfo>(nullptr);
    for (int flag = 0; flag < 16; ++flag) {
        timerInfo->SetType(flag);
        EXPECT_EQ(timerInfo->type, flag);
    }
}

}
