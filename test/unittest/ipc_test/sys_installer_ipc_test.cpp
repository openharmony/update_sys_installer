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

#include <fstream>
#include "gtest/gtest.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "isys_installer.h"
#include "isys_installer_callback.h"
#include "system_ability_definition.h"
#include "sys_installer_kits_impl.h"
#include "sys_installer_proxy.h"
#include "sys_installer_load_callback.h"

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
    void OnUpgradeProgress(UpdateStatus updateStatus, int percent, const std::string &resultMsg) override
    {
        printf("ProgressCallback progress %d percent %d msg %s\n", updateStatus, percent, resultMsg.c_str());
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
    ASSERT_EQ(ret, 0);
}

// accessory update package
HWTEST_F(SysInstallerIpcUnitTest, AccUpdatePkgTest001, TestSize.Level1)
{
    cout << "AccUpdatePkgTest001 start " << std::endl;
    auto ret = SysInstallerKitsImpl::GetInstance().AccDecompressAndVerifyPkg(
        "invalid path", "", 1);
    ASSERT_NE(ret, 0);
    ret = SysInstallerKitsImpl::GetInstance().AccDecompressAndVerifyPkg(
        "", "/data/test/", 1);
    ASSERT_NE(ret, 0);
    ret = SysInstallerKitsImpl::GetInstance().AccDecompressAndVerifyPkg(
        "invalid path", "/data/test/", 1);
    ASSERT_NE(ret, 0);
    ret = SysInstallerKitsImpl::GetInstance().AccDecompressAndVerifyPkg(
        "/data/ota_package/update.zip", "invalid path", 1);
    ASSERT_NE(ret, 0);

    ret = SysInstallerKitsImpl::GetInstance().AccDecompressAndVerifyPkg(
        "/data/ota_package/update.zip", "/data/test/", 1);
    ASSERT_NE(ret, 0);

    std::string path = "/data/updater/rmDir";
    ret = mkdir(path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (ret == 0) {
        ofstream tmpFile;
        string filePath = path + "/tmpFile";
        tmpFile.open(filePath.c_str());
        if (tmpFile.is_open()) {
            tmpFile.close();
            ret = SysInstallerKitsImpl::GetInstance().AccDeleteDir(path);
        }
    }

    ret = SysInstallerKitsImpl::GetInstance().AccDeleteDir("");
    ASSERT_NE(ret, 0);
}

// StartUpdateParaZip test
HWTEST_F(SysInstallerIpcUnitTest, StartUpdateParaZipTest, TestSize.Level1)
{
    cout << " StartUpdateParaZip start " << std::endl;
    SysInstallerLoadCallback sysInstallerLoadCallback {};
    sysInstallerLoadCallback.OnLoadSystemAbilityFail(0);
    sysInstallerLoadCallback.OnLoadSystemAbilityFail(4101); // 4041 : SYS_INSTALLER_DISTRIBUTED_SERVICE_ID
    sptr<IRemoteObject> callback {};
    SysInstallerProxy env(callback);
    auto ret = env.StartUpdateParaZip("", "", "");
    ASSERT_EQ(ret, 3); // 3 : ERR_FLATTEN_OBJECT
}

// StartDeleteParaZip test
HWTEST_F(SysInstallerIpcUnitTest, StartDeleteParaZipTest, TestSize.Level1)
{
    cout << " StartDeleteParaZip start " << std::endl;
    sptr<IRemoteObject> callback {};
    SysInstallerProxy env(callback);
    auto ret = env.StartDeleteParaZip("", "");
    ASSERT_EQ(ret, 3); // 3 : ERR_FLATTEN_OBJECT
}

// AccDecompressAndVerifyPkg test
HWTEST_F(SysInstallerIpcUnitTest, AccDecompressAndVerifyPkgTest, TestSize.Level1)
{
    cout << " AccDecompressAndVerifyPkgTest start " << std::endl;
    sptr<IRemoteObject> callback {};
    SysInstallerProxy env(callback);
    auto ret = env.AccDecompressAndVerifyPkg("", "", 0);
    ASSERT_EQ(ret, 3); // 3 : ERR_FLATTEN_OBJECT
}

// AccDeleteDir test
HWTEST_F(SysInstallerIpcUnitTest, AccDeleteDirTest, TestSize.Level1)
{
    cout << " AccDeleteDirTest start " << std::endl;
    sptr<IRemoteObject> callback {};
    SysInstallerProxy env(callback);
    auto ret = env.AccDeleteDir("");
    ASSERT_EQ(ret, 3); // 3 : ERR_FLATTEN_OBJECT
}

// SysInstallerKitsImpl test
HWTEST_F(SysInstallerIpcUnitTest, SysInstallerKitsImplTest, TestSize.Level1)
{
    cout << " SysInstallerKitsImplTest start " << std::endl;
    wptr<IRemoteObject> remote;
    SysInstallerKitsImpl::GetInstance().ResetService(remote);
    auto ret = SysInstallerKitsImpl::GetInstance().StartUpdateParaZip("", "", "");
    ASSERT_EQ(ret, -1);
    ret = SysInstallerKitsImpl::GetInstance().StartDeleteParaZip("", "");
    ASSERT_EQ(ret, -1);
    ret = SysInstallerKitsImpl::GetInstance().AccDecompressAndVerifyPkg("", "", 0);
    ASSERT_EQ(ret, -1);
    ret = SysInstallerKitsImpl::GetInstance().AccDeleteDir("");
    ASSERT_EQ(ret, -1);
}
} // SysInstaller
} // OHOS
