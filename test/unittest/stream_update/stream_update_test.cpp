/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "stream_update_test.h"
#include "updater/updater.h"
#include "bin_chunk_update.h"
#include "log/log.h"
#include "scope_guard.h"
#include "securec.h"
#include "updater/updater_const.h"
#include "utils.h"
#include <thread>
#include <atomic>
#include <algorithm>

#include "gtest/gtest.h"

using namespace testing::ext;
using namespace std;

namespace OHOS {
namespace SysInstaller {

HWTEST_F(StreamInstallProcesserTest, StartStopTest, TestSize.Level1)
{
    StreamInstallProcesser::GetInstance().Start();
    EXPECT_TRUE(StreamInstallProcesser::GetInstance().IsRunning());
    StreamInstallProcesser::GetInstance().Stop();
    EXPECT_FALSE(StreamInstallProcesser::GetInstance().IsRunning());
}

HWTEST_F(StreamInstallProcesserTest, ProcessStreamDataSuccess, TestSize.Level1)
{
    EXPECT_EQ(StreamInstallProcesser::GetInstance().Start(), 0);

    uint8_t data[1024] = {0};
    std::fill_n(data, sizeof(data), 0x55);

    EXPECT_EQ(StreamInstallProcesser::GetInstance().ProcessStreamData(data, sizeof(data)), 0);
    StreamInstallProcesser::GetInstance().Stop();
}

HWTEST_F(StreamInstallProcesserTest, UpdateResultTest, TestSize.Level1)
{
    // 预期 UpdateCallback 方法被调用
    EXPECT_CALL(*statusManager, UpdateCallback(UpdateStatus::UPDATE_STATE_INIT, 0, "Initializing")).Times(1);
    StreamInstallProcesser::GetInstance().UpdateResult(UpdateStatus::UPDATE_STATE_INIT, 0, "Initializing");
}

} // namespace SysInstaller
} // namespace OHOS
