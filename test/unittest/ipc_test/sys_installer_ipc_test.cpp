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

    auto ret = SysInstallerKitsImpl::GetInstance().SysInstallerInit("ipc_ut_test");
    ASSERT_EQ(ret, 0);
}

class ProcessCallbackTest : public ISysInstallerCallbackFunc {
public:
    ProcessCallbackTest() = default;
    ~ProcessCallbackTest() = default;
    void OnUpgradeProgress(UpdateStatus updateStatus, int percent, const std::string &resultMsg) override
    {
        printf("ProgressCallback progress %d percent %d msg %s\n",
            updateStatus, percent, resultMsg.c_str());
    }
    void OnUpgradeDealLen(UpdateStatus updateStatus, int dealLen, const std::string &resultMsg) override
    {
        printf("ProgressCallback progress %d dealLen %d msg %s\n",
            updateStatus, dealLen, resultMsg.c_str());
    }
};

// callback
HWTEST_F(SysInstallerIpcUnitTest, SetCallBackTest001, TestSize.Level1)
{
    cout << " SetCallBackTest001 start " << std::endl;
    sptr<ISysInstallerCallbackFunc> callback = new ProcessCallbackTest;
    auto ret = SysInstallerKitsImpl::GetInstance().SetUpdateCallback("ipc_ut_test", callback);
    ASSERT_EQ(ret, 0);
}

// get status
HWTEST_F(SysInstallerIpcUnitTest, GetStatusTest001, TestSize.Level1)
{
    cout << " GetStatusTest001 start " << std::endl;
    std::string uniqueTaskId = "ipc_ut_test_status";
    SysInstallerKitsImpl::GetInstance().SysInstallerInit(uniqueTaskId);
    auto ret = SysInstallerKitsImpl::GetInstance().GetUpdateStatus(uniqueTaskId);
    ASSERT_EQ(ret, 0);
}

// update package
HWTEST_F(SysInstallerIpcUnitTest, UpdatePackageTest001, TestSize.Level1)
{
    cout << " UpdatePackageTest001 start " << std::endl;
    auto ret = SysInstallerKitsImpl::GetInstance().StartUpdatePackageZip("ipc_ut_test",
        "/data/ota_package/update.zip");
    cout << " UpdatePackageTest001 ret " << ret << std::endl;
    ASSERT_EQ(ret, 0);
}

// accessory update package
HWTEST_F(SysInstallerIpcUnitTest, AccUpdatePkgTest001, TestSize.Level1)
{
    cout << "AccUpdatePkgTest001 start " << std::endl;
    auto ret = SysInstallerKitsImpl::GetInstance().AccDecompressAndVerifyPkg("ipc_ut_test",
        "invalid path", "", 1);
    ASSERT_NE(ret, 0);
    ret = SysInstallerKitsImpl::GetInstance().AccDecompressAndVerifyPkg("ipc_ut_test",
        "", "/data/test/", 1);
    ASSERT_NE(ret, 0);
    ret = SysInstallerKitsImpl::GetInstance().AccDecompressAndVerifyPkg("ipc_ut_test",
        "invalid path", "/data/test/", 1);
    ASSERT_NE(ret, 0);
    ret = SysInstallerKitsImpl::GetInstance().AccDecompressAndVerifyPkg("ipc_ut_test",
        "/data/ota_package/update.zip", "invalid path", 1);
    ASSERT_NE(ret, 0);

    ret = SysInstallerKitsImpl::GetInstance().AccDecompressAndVerifyPkg("ipc_ut_test",
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
            ret = SysInstallerKitsImpl::GetInstance().AccDeleteDir("ipc_ut_test", path);
        }
    }

    ret = SysInstallerKitsImpl::GetInstance().AccDeleteDir("ipc_ut_test", "");
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
    auto ret = env.StartUpdateParaZip("ipc_ut_test", "", "", "");
    ASSERT_EQ(ret, 5); // 5 : ERR_INVALID_DATA
}

// StartDeleteParaZip test
HWTEST_F(SysInstallerIpcUnitTest, StartDeleteParaZipTest, TestSize.Level1)
{
    cout << " StartDeleteParaZip start " << std::endl;
    sptr<IRemoteObject> callback {};
    SysInstallerProxy env(callback);
    auto ret = env.StartDeleteParaZip("ipc_ut_test", "", "");
    ASSERT_EQ(ret, 5); // 5 : ERR_INVALID_DATA
}

// AccDecompressAndVerifyPkg test
HWTEST_F(SysInstallerIpcUnitTest, AccDecompressAndVerifyPkgTest, TestSize.Level1)
{
    cout << " AccDecompressAndVerifyPkgTest start " << std::endl;
    sptr<IRemoteObject> callback {};
    SysInstallerProxy env(callback);
    auto ret = env.AccDecompressAndVerifyPkg("ipc_ut_test", "", "", 0);
    ASSERT_EQ(ret, 5); // 5 : ERR_INVALID_DATA
}

// AccDeleteDir test
HWTEST_F(SysInstallerIpcUnitTest, AccDeleteDirTest, TestSize.Level1)
{
    cout << " AccDeleteDirTest start " << std::endl;
    sptr<IRemoteObject> callback {};
    SysInstallerProxy env(callback);
    auto ret = env.AccDeleteDir("ipc_ut_test", "");
    ASSERT_EQ(ret, 5); // 5 : ERR_INVALID_DATA
}

// SysInstallerKitsImpl test
HWTEST_F(SysInstallerIpcUnitTest, SysInstallerKitsImplTest, TestSize.Level1)
{
    cout << " SysInstallerKitsImplTest start " << std::endl;
    wptr<IRemoteObject> remote;
    SysInstallerKitsImpl::GetInstance().ResetService(remote);
    auto ret = SysInstallerKitsImpl::GetInstance().StartUpdateParaZip("ipc_ut_test", "", "", "");
    ASSERT_EQ(ret, -1);
    ret = SysInstallerKitsImpl::GetInstance().StartDeleteParaZip("ipc_ut_test", "", "");
    ASSERT_EQ(ret, -1);
    ret = SysInstallerKitsImpl::GetInstance().AccDecompressAndVerifyPkg("ipc_ut_test", "", "", 0);
    ASSERT_EQ(ret, -1);
    ret = SysInstallerKitsImpl::GetInstance().AccDeleteDir("ipc_ut_test", "");
    ASSERT_EQ(ret, -1);
    ret = SysInstallerKitsImpl::GetInstance().SetUpdateVabMode("ipc_ut_test", UpdateVabMode::BACKGROUND_UPDATE_MODE);
    ASSERT_EQ(ret, -1);
}

// GetPartitionStashSize test when service is unavailable
HWTEST_F(SysInstallerIpcUnitTest, GetPartitionStashSizeWhenServiceUnavailable, TestSize.Level1)
{
    cout << " GetPartitionStashSizeWhenServiceUnavailable start " << std::endl;
    wptr<IRemoteObject> remote;
    SysInstallerKitsImpl::GetInstance().ResetService(remote);
    std::string taskId = "test_task_stash";
    std::vector<std::string> pkgPaths = {"/data/updater/package.zip"};
    uint64_t stashSize = 0;
    // ret 0: success, -1: error
    auto ret = SysInstallerKitsImpl::GetInstance().GetPartitionStashSize(taskId, pkgPaths, stashSize);
    ASSERT_EQ(ret, -1);
}

// GetPartitionStashSize test with valid parameters but service unavailable
HWTEST_F(SysInstallerIpcUnitTest, GetPartitionStashSizeValidParamsServiceUnavailable, TestSize.Level1)
{
    cout << " GetPartitionStashSizeValidParamsServiceUnavailable start " << std::endl;
    wptr<IRemoteObject> remote;
    SysInstallerKitsImpl::GetInstance().ResetService(remote);
    std::string taskId = "valid_task_id_for_stash_test";
    std::vector<std::string> pkgPaths = {
        "/data/updater/package1.zip",
        "/data/updater/package2.zip"
    };
    uint64_t stashSize = 100;
    // ret 0: success, -1: error
    auto ret = SysInstallerKitsImpl::GetInstance().GetPartitionStashSize(taskId, pkgPaths, stashSize);
    ASSERT_EQ(ret, -1);
}

// CreateVabSnapshotCowImg test when service is unavailable
HWTEST_F(SysInstallerIpcUnitTest, CreateVabSnapshotCowImgWhenServiceUnavailable, TestSize.Level1)
{
    cout << " CreateVabSnapshotCowImgWhenServiceUnavailable start " << std::endl;
    wptr<IRemoteObject> remote;
    SysInstallerKitsImpl::GetInstance().ResetService(remote);
    VabCowInfo vabCowInfo;
    vabCowInfo.name = "test_partition";
    vabCowInfo.size = 1024;
    vabCowInfo.splitSize = 512;
    vabCowInfo.pkgPartition = PartitionType::Y;
    vabCowInfo.trcPartition = PartitionType::N;
    uint64_t createdSize = 0;
    bool isCreated = false;
    auto ret = SysInstallerKitsImpl::GetInstance().CreateVabSnapshotCowImg(vabCowInfo, createdSize, isCreated);
    ASSERT_EQ(ret, -1);
}

// CreateVabSnapshotCowImg test with different PartitionType values
HWTEST_F(SysInstallerIpcUnitTest, CreateVabSnapshotCowImgWithDifferentPartitionType, TestSize.Level1)
{
    cout << " CreateVabSnapshotCowImgWithDifferentPartitionType start " << std::endl;
    wptr<IRemoteObject> remote;
    SysInstallerKitsImpl::GetInstance().ResetService(remote);
    uint64_t createdSize = 0;
    bool isCreated = false;

    // Test with PartitionType::None
    VabCowInfo vabCowInfo1;
    vabCowInfo1.name = "test_none";
    vabCowInfo1.size = 1024;
    vabCowInfo1.splitSize = 0;
    vabCowInfo1.pkgPartition = PartitionType::None;
    vabCowInfo1.trcPartition = PartitionType::None;
    auto ret = SysInstallerKitsImpl::GetInstance().CreateVabSnapshotCowImg(vabCowInfo1, createdSize, isCreated);
    ASSERT_EQ(ret, -1);

    // Test with PartitionType::Y
    VabCowInfo vabCowInfo2;
    vabCowInfo2.name = "test_y";
    vabCowInfo2.size = 2048;
    vabCowInfo2.splitSize = 1024;
    vabCowInfo2.pkgPartition = PartitionType::Y;
    vabCowInfo2.trcPartition = PartitionType::Y;
    ret = SysInstallerKitsImpl::GetInstance().CreateVabSnapshotCowImg(vabCowInfo2, createdSize, isCreated);
    ASSERT_EQ(ret, -1);

    // Test with PartitionType::N
    VabCowInfo vabCowInfo3;
    vabCowInfo3.name = "test_n";
    vabCowInfo3.size = 4096;
    vabCowInfo3.splitSize = 2048;
    vabCowInfo3.pkgPartition = PartitionType::N;
    vabCowInfo3.trcPartition = PartitionType::N;
    ret = SysInstallerKitsImpl::GetInstance().CreateVabSnapshotCowImg(vabCowInfo3, createdSize, isCreated);
    ASSERT_EQ(ret, -1);
}

// GetPartitionAvailableSize test when service is unavailable
HWTEST_F(SysInstallerIpcUnitTest, GetPartitionAvailableSizeWhenServiceUnavailable, TestSize.Level1)
{
    cout << " GetPartitionAvailableSizeWhenServiceUnavailable start " << std::endl;
    wptr<IRemoteObject> remote;
    SysInstallerKitsImpl::GetInstance().ResetService(remote);
    std::map<std::string, uint64_t> dtsCowsSize;
    dtsCowsSize["partition_a"] = 1024;
    std::map<std::string, uint64_t> dtsImgsSize;
    dtsImgsSize["partition_b"] = 2048;
    PartitionInfo partitionInfo;
    partitionInfo.pkgPartition = PartitionType::Y;
    partitionInfo.trcPartition = PartitionType::N;
    uint64_t availSize = 0;
    auto ret = SysInstallerKitsImpl::GetInstance().GetPartitionAvailableSize(
        dtsCowsSize, dtsImgsSize, partitionInfo, availSize);
    ASSERT_EQ(ret, -1);
}

// GetPartitionAvailableSize test with different PartitionType values
HWTEST_F(SysInstallerIpcUnitTest, GetPartitionAvailableSizeWithDifferentPartitionType, TestSize.Level1)
{
    cout << " GetPartitionAvailableSizeWithDifferentPartitionType start " << std::endl;
    wptr<IRemoteObject> remote;
    SysInstallerKitsImpl::GetInstance().ResetService(remote);
    std::map<std::string, uint64_t> dtsCowsSize;
    dtsCowsSize["partition_a"] = 1024;
    std::map<std::string, uint64_t> dtsImgsSize;
    dtsImgsSize["partition_b"] = 2048;
    uint64_t availSize = 0;

    // Test with PartitionType::None
    PartitionInfo partitionInfo1;
    partitionInfo1.pkgPartition = PartitionType::None;
    partitionInfo1.trcPartition = PartitionType::None;
    auto ret = SysInstallerKitsImpl::GetInstance().GetPartitionAvailableSize(
        dtsCowsSize, dtsImgsSize, partitionInfo1, availSize);
    ASSERT_EQ(ret, -1);

    // Test with PartitionType::Y
    PartitionInfo partitionInfo2;
    partitionInfo2.pkgPartition = PartitionType::Y;
    partitionInfo2.trcPartition = PartitionType::Y;
    ret = SysInstallerKitsImpl::GetInstance().GetPartitionAvailableSize(
        dtsCowsSize, dtsImgsSize, partitionInfo2, availSize);
    ASSERT_EQ(ret, -1);

    // Test with PartitionType::N
    PartitionInfo partitionInfo3;
    partitionInfo3.pkgPartition = PartitionType::N;
    partitionInfo3.trcPartition = PartitionType::N;
    ret = SysInstallerKitsImpl::GetInstance().GetPartitionAvailableSize(
        dtsCowsSize, dtsImgsSize, partitionInfo3, availSize);
    ASSERT_EQ(ret, -1);

    // Test with mixed PartitionType values
    PartitionInfo partitionInfo4;
    partitionInfo4.pkgPartition = PartitionType::Y;
    partitionInfo4.trcPartition = PartitionType::N;
    ret = SysInstallerKitsImpl::GetInstance().GetPartitionAvailableSize(
        dtsCowsSize, dtsImgsSize, partitionInfo4, availSize);
    ASSERT_EQ(ret, -1);

    PartitionInfo partitionInfo5;
    partitionInfo5.pkgPartition = PartitionType::N;
    partitionInfo5.trcPartition = PartitionType::Y;
    ret = SysInstallerKitsImpl::GetInstance().GetPartitionAvailableSize(
        dtsCowsSize, dtsImgsSize, partitionInfo5, availSize);
    ASSERT_EQ(ret, -1);
}
} // SysInstaller
} // OHOS
