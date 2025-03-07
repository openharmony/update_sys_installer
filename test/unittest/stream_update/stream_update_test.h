/*
* Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef STREAM_UPDATE_UNITTEST_H
#define STREAM_UPDATE_UNITTEST_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "stream_status_manager.h"
#include "stream_update.h"

namespace OHOS {
namespace SysInstaller {

class MockStatusManager : public StreamStatusManager {
public:
    MOCK_METHOD(void, UpdateCallback, (UpdateStatus updateStatus, int dealLen,
        const std::string &resultMsg), (override));
};

class StreamInstallProcesserTest : public ::testing::Test {
protected:
    std::shared_ptr<MockStatusManager > statusManager {};

    void SetUp() override
    {
        statusManager = std::make_shared<MockStatusManager >();
        statusManager->Init();
        StreamInstallProcesser::GetInstance().SetStatusManager(statusManager);
    }

    void TearDown() override
    {
    }
};

} // SysInstaller
} // OHOS
#endif // STREAM_UPDATE_UNITTEST_H
