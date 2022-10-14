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

#include "sys_installer_ipc_test.h"

#include "gtest/gtest.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "isys_installer.h"
#include "isys_installer_callback.h"
#include "system_ability_definition.h"
#include "sys_installer_kits_impl.h"

using namespace testing::ext;
using namespace std;

namespace OHOS {
namespace SysInstaller {

void SysInstallerIpcUnitTest::SetUpTestCase()
{
    cout << "SysInstallerIpcUnitTest SetUpTestCase" << std::endl;
}

void SysInstallerIpcUnitTest::TearDownTestCase()
{
    cout << "SysInstallerIpcUnitTest TearDownTestCase" << std::endl;
}

void SysInstallerIpcUnitTest::SetUp()
{
    cout << "SysInstallerIpcUnitTest SetUp" << std::endl;
}

void SysInstallerIpcUnitTest::TearDown()
{
    cout << "SysInstallerIpcUnitTest TearDown" << std::endl;
}

// init
HWTEST_F(SysInstallerIpcUnitTest, SysInstallerInit001, TestSize.Level1)
{
    cout << " SysInstallerInit001 start " << std::endl;

    auto ret = SysInstallerKitsImpl::GetInstance().SysInstallerInit();
    ASSERT_EQ(ret, 0);
}

class ProcessCallbackTest : public ISysInstallerCallbackFunc {
public:
    ProcessCallbackTest() = default;
    ~ProcessCallbackTest() = default;
    void OnUpgradeProgress(UpdateStatus updateStatus, int percent) override
    {
        printf("ProgressCallback progress %d percent %d\n", updateStatus, percent);
    }
};

// callback
HWTEST_F(SysInstallerIpcUnitTest, SetCallBackTest001, TestSize.Level1)
{
    cout << " SetCallBackTest001 start " << std::endl;
    sptr<ISysInstallerCallbackFunc> callback = new ProcessCallbackTest;
    auto ret = SysInstallerKitsImpl::GetInstance().SetUpdateCallback(callback);
    ASSERT_EQ(ret, 0);
}

// get status
HWTEST_F(SysInstallerIpcUnitTest, GetStatusTest001, TestSize.Level1)
{
    cout << " GetStatusTest001 start " << std::endl;
    auto ret = SysInstallerKitsImpl::GetInstance().GetUpdateStatus();
    ASSERT_EQ(ret, 0);
}

// update package
HWTEST_F(SysInstallerIpcUnitTest, UpdatePackageTest001, TestSize.Level1)
{
    cout << " UpdatePackageTest001 start " << std::endl;
    auto ret = SysInstallerKitsImpl::GetInstance().StartUpdatePackageZip(
        "/data/ota_package/update.zip");
    cout << " UpdatePackageTest001 ret " << ret << std::endl;
}

// update para package
HWTEST_F(SysInstallerIpcUnitTest, UpdateParaPackageTest001, TestSize.Level1)
{
    cout << " UpdateParaPackageTest001 start " << std::endl;
    auto ret = SysInstallerKitsImpl::GetInstance().StartUpdateParaZip(
        "/data/ota_package/update_para.zip", "/data/service/el1/pulbic/update", "/taboo");
    cout << " UpdateParaPackageTest001 ret " << ret << std::endl;
}
} // SysInstaller
} // OHOS