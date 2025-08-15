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
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzSysInstaller(data, size);
    return 0;
}

