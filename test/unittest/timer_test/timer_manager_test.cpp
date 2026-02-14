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
#include "sys_installer_timer_manager.h"

namespace {
using namespace testing;
using namespace testing::ext;
using namespace Updater;
using namespace OHOS::SysInstaller;

class SysInstallerTimerManagerUnitTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void SysInstallerTimerManagerUnitTest::SetUpTestCase()
{
    SetLogLevel(DEBUG);
    InitUpdaterLogger("UPDATER", "updater_log.log", "updater_status.log", "error_code.log");
}

void SysInstallerTimerManagerUnitTest::TearDownTestCase()
{
}

void SysInstallerTimerManagerUnitTest::SetUp()
{
}

void SysInstallerTimerManagerUnitTest::TearDown()
{
}

HWTEST_F(SysInstallerTimerManagerUnitTest, test_RegisterTimer_immediately, TestSize.Level0)
{
    TimerCallback cb;
    uint64_t delayMs = 0;
    uint64_t id = SysInstallerTimerManager::RegisterTimer(delayMs, cb);
    EXPECT_NE(id, 0);
    SysInstallerTimerManager::UnRegisterTimer(id);
}

HWTEST_F(SysInstallerTimerManagerUnitTest, test_RegisterTimer_valid_afterMs, TestSize.Level0)
{
    TimerCallback cb;
    uint64_t delayMs = 100;
    uint64_t id = SysInstallerTimerManager::RegisterTimer(delayMs, cb);
    EXPECT_NE(id, 0);
    SysInstallerTimerManager::UnRegisterTimer(id);
}

HWTEST_F(SysInstallerTimerManagerUnitTest, test_RegisterTimer_callback, TestSize.Level0)
{
    const uint64_t registerAtMs = GetSystemBootTime();
    const uint64_t delayMs = 100;
    const auto timeout = std::chrono::milliseconds(2 * delayMs);
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> triggered{false};
    TimerCallback cb = [registerAtMs, delayMs, &mtx, &cv, &triggered]() {
        const uint64_t nowMs = GetSystemBootTime();
        GTEST_LOG_(INFO) << "Timer callback called " << nowMs - registerAtMs << " ms";
        EXPECT_GE(nowMs, registerAtMs + delayMs);
        std::lock_guard<std::mutex> lock(mtx);
        triggered = true;
        cv.notify_all();
    };
    uint64_t id = SysInstallerTimerManager::RegisterTimer(delayMs, cb);
    EXPECT_NE(id, 0);
    {
        std::unique_lock<std::mutex> lock(mtx);
        const bool callbackTriggered = cv.wait_for(lock, timeout, [&triggered]() { return triggered.load(); });
        EXPECT_TRUE(callbackTriggered) << "Timer callback timeout";
    }
    SysInstallerTimerManager::UnRegisterTimer(id);
}

HWTEST_F(SysInstallerTimerManagerUnitTest, test_RegisterRepeatTimer_callback, TestSize.Level0)
{
    constexpr int repeatLimit = 2;
    const uint64_t registerAtMs = GetSystemBootTime();
    const uint64_t delayMs = 50;
    const uint64_t intervalMs = 5 * 1000;
    const auto timeout = std::chrono::milliseconds(delayMs + (repeatLimit + 1) * intervalMs);
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<int> count{0};
    TimerCallback cb = [registerAtMs, delayMs, &mtx, &cv, &count]() {
        const uint64_t nowMs = GetSystemBootTime();
        GTEST_LOG_(INFO) << "Timer callback called " << nowMs - registerAtMs << " ms";
        EXPECT_GE(nowMs, registerAtMs + delayMs);
        std::lock_guard<std::mutex> lock(mtx);
        ++count;
        if (count >= repeatLimit) {
            cv.notify_all();
        }
    };
    uint64_t id = SysInstallerTimerManager::RegisterRepeatTimer(delayMs, intervalMs, cb);
    EXPECT_NE(id, 0);
    {
        std::unique_lock<std::mutex> lock(mtx);
        const bool callbackTriggered = cv.wait_for(lock, timeout, [&count]() { return count >= repeatLimit; });
        EXPECT_TRUE(callbackTriggered) << "Repeat timer callback timeout";
    }
    GTEST_LOG_(INFO) << "Repeat timer triggered count " << count;
    SysInstallerTimerManager::UnRegisterTimer(id);
}

HWTEST_F(SysInstallerTimerManagerUnitTest, test_RegisterRepeatTimer_interval_zero, TestSize.Level0)
{
    const uint64_t delayMs = 50;
    TimerCallback cb;
    uint64_t id = SysInstallerTimerManager::RegisterRepeatTimer(delayMs, 0, cb);
    ASSERT_EQ(id, 0) << "Repeat timer with interval zero should not be registered";
}

}
