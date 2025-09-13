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

#include "sys_installer_fuzzer.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include "isys_installer.h"
#include "isys_installer_callback_func.h"
#include "sys_installer_kits_impl.h"

namespace OHOS {
using namespace SysInstaller;

constexpr int DATA_IDX_NEED_REBOOT = 0;
constexpr int DATA_IDX_INSTALL_MODE = 1;
constexpr int DATA_IDX_REBOOT = 2;
constexpr int DATA_FEATURE_MIN_LEN = 3;
constexpr int DATA_FEATURE_CLEAR_LEN = 2;

FeatureInfo ParseFeatureInfo(const uint8_t* data, size_t size)
{
    FeatureInfo featureInfo {};

    size_t len = size / DATA_FEATURE_MIN_LEN;
    size_t offset = 0;
    std::string featureName = std::string(reinterpret_cast<const char*>(data + offset), len);
    offset += len;
    std::string version = std::string(reinterpret_cast<const char*>(data + offset), len);
    offset += len;
    std::string path = std::string(reinterpret_cast<const char*>(data + offset), size - offset);
    return {(data[DATA_IDX_NEED_REBOOT] & 1) == 0, featureName, version, path};
}

void InstallCloudRomFuzzTest(const std::string &taskId, const uint8_t* data, size_t size)
{
    if (size < DATA_FEATURE_MIN_LEN) {
        return;
    }
    FeatureInfo featureInfo = ParseFeatureInfo(data, size);
    std::vector<FeatureInfo> featureInfos = {featureInfo};
    InstallMode installMode = (data[DATA_IDX_INSTALL_MODE] & 1) == 0 ?
        InstallMode::FEATURE_INSTALL : InstallMode::REGULAR_OTA;
    RebootStatus rebootStatus = (data[DATA_IDX_REBOOT] & 1) == 0 ? RebootStatus::NOT_REBOOT : RebootStatus::REBOOTED;
    SysInstallerKitsImpl::GetInstance().InstallCloudRom(taskId, installMode, featureInfos, rebootStatus);
}

void UninstallCloudRomFuzzTest(const std::string &taskId, const uint8_t* data, size_t size)
{
    if (size < DATA_FEATURE_MIN_LEN) {
        return;
    }
    FeatureInfo featureInfo = ParseFeatureInfo(data, size);
    std::vector<FeatureInfo> featureInfos = {featureInfo};
    RebootStatus rebootStatus = (data[DATA_IDX_REBOOT] & 1) == 0 ? RebootStatus::NOT_REBOOT : RebootStatus::REBOOTED;
    SysInstallerKitsImpl::GetInstance().UninstallCloudRom(taskId, featureInfos, rebootStatus);
}

void GetFeatureStatusFuzzTest(const uint8_t* data, size_t size)
{
    if (size < DATA_FEATURE_MIN_LEN) {
        return;
    }
    FeatureInfo featureInfo = ParseFeatureInfo(data, size);
    std::vector<FeatureInfo> featureInfos = {featureInfo};
    std::vector<FeatureStatus> statusInfos = {};
    SysInstallerKitsImpl::GetInstance().GetFeatureStatus(featureInfos, statusInfos);
}

void GetAllFeatureStatusFuzzTest(const uint8_t* data, size_t size)
{
    std::vector<FeatureStatus> statusInfos = {};
    SysInstallerKitsImpl::GetInstance().GetAllFeatureStatus(
        std::string(reinterpret_cast<const char*>(data), size), statusInfos);
}

void ClearCloudRomFuzzTest(const uint8_t* data, size_t size)
{
    if (size < 0) {
        return;
    }
    std::string baseVersion = "";
    std::string featureName = "";
    if ((data[0] & 1) == 0) {
        baseVersion = std::string(reinterpret_cast<const char*>(data), size);
    } else {
        size_t len = size / DATA_FEATURE_CLEAR_LEN;
        baseVersion = std::string(reinterpret_cast<const char*>(data), len);
        featureName = std::string(reinterpret_cast<const char*>(data + len), size - len);
    }
    SysInstallerKitsImpl::GetInstance().ClearCloudRom(baseVersion, featureName);
}

void FuzzSysInstaller(const uint8_t* data, size_t size)
{
    std::string taskId = std::string(reinterpret_cast<const char*>(data), size);
    std::vector<std::string> pkgPaths = {"/data/updater/fuzz/updater.zip"};
    const std::string taskType = "taskType";
    const std::string resultType = "resultType";
    SysInstallerKitsImpl::GetInstance().SysInstallerInit(taskId);
    SysInstallerKitsImpl::GetInstance().SetUpdateCallback(taskId, nullptr);
    SysInstallerKitsImpl::GetInstance().GetUpdateStatus(taskId);
    SysInstallerKitsImpl::GetInstance().CancelUpdateVabPackageZip(taskId);
    SysInstallerKitsImpl::GetInstance().StartVabMerge(taskId);
    SysInstallerKitsImpl::GetInstance().GetUpdateResult(taskId, taskType, resultType);
    SysInstallerKitsImpl::GetInstance().StartUpdateVabPackageZip(taskId, pkgPaths);
    taskId = "fuzz_test";
    std::string pkgPath = std::string(reinterpret_cast<const char*>(data), size);
    SysInstallerKitsImpl::GetInstance().StartUpdatePackageZip(taskId, pkgPath);
    pkgPath = "/data/updater/fuzz/updater.zip";
    const std::string location = "location";
    std::string cfgDir = std::string(reinterpret_cast<const char*>(data), size);
    SysInstallerKitsImpl::GetInstance().StartUpdateParaZip(taskId, pkgPath, location, cfgDir);
    SysInstallerKitsImpl::GetInstance().StartDeleteParaZip(taskId, location, cfgDir);
    std::string dstPath = std::string(reinterpret_cast<const char*>(data), size);
    SysInstallerKitsImpl::GetInstance().AccDecompressAndVerifyPkg(taskId, pkgPath, dstPath, 1);
    SysInstallerKitsImpl::GetInstance().AccDeleteDir(taskId, dstPath);
    std::string action = std::string(reinterpret_cast<const char*>(data), size);
    bool result = false;
    SysInstallerKitsImpl::GetInstance().GetMetadataResult(action, result);
    uint32_t reservedCores;
    std::copy(data, data + std::min(sizeof(uint32_t), size), reinterpret_cast<uint8_t*>(&reservedCores));
    SysInstallerKitsImpl::GetInstance().SetCpuAffinity(taskId, reservedCores);
    SysInstallerKitsImpl::GetInstance().ClearVabMetadataAndCow();
    SysInstallerKitsImpl::GetInstance().VabUpdateActive();
    SysInstallerKitsImpl::GetInstance().ExitSysInstaller();
    SysInstallerKitsImpl::GetInstance().StartAbSync();
}

void FuzzSysInstallerCloudRom(const uint8_t* data, size_t size)
{
    std::string taskId = "fuzz_test";
    InstallCloudRomFuzzTest(taskId, data, size);
    UninstallCloudRomFuzzTest(taskId, data, size);
    GetFeatureStatusFuzzTest(data, size);
    GetAllFeatureStatusFuzzTest(data, size);
    ClearCloudRomFuzzTest(data, size);
}

void FuzzSysInstallerCreateSplitCow(const uint8_t* data, size_t size)
{
    uint64_t createdSize = 0;
    SysInstallerKitsImpl::GetInstance().CreateVabSnapshotCowImg(
        std::string(reinterpret_cast<const char*>(data), size), 1, 0, createdSize);
    if (size < sizeof(uint64_t)) {
        return;
    }
    std::string name = "fuzz_test";
    SysInstallerKitsImpl::GetInstance().CreateVabSnapshotCowImg(
        name, *(reinterpret_cast<const uint64_t*>(data)), 0, createdSize);
    SysInstallerKitsImpl::GetInstance().CreateVabSnapshotCowImg(
        name, 1, *(reinterpret_cast<const uint64_t*>(data)), createdSize);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzSysInstaller(data, size);
    OHOS::FuzzSysInstallerCloudRom(data, size);
    OHOS::FuzzSysInstallerCreateSplitCow(data, size);
    return 0;
}

