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
    std::string taskId = "fuzz_test";
    SysInstallerKitsImpl::GetInstance().SysInstallerInit(taskId);
    SysInstallerKitsImpl::GetInstance().SetUpdateCallback(taskId, nullptr);
    SysInstallerKitsImpl::GetInstance().StartUpdatePackageZip(taskId,
        std::string(reinterpret_cast<const char*>(data), size));
    const std::string pkgPath = "/data/updater/fuzz/updater.zip";
    const std::string location = "location";
    SysInstallerKitsImpl::GetInstance().GetUpdateStatus(taskId);
    SysInstallerKitsImpl::GetInstance().StartUpdateParaZip(taskId, pkgPath,
        location, std::string(reinterpret_cast<const char*>(data), size));
    SysInstallerKitsImpl::GetInstance().AccDecompressAndVerifyPkg(taskId,
        pkgPath, std::string(reinterpret_cast<const char*>(data), size), 1);
    SysInstallerKitsImpl::GetInstance().AccDeleteDir(taskId, std::string(reinterpret_cast<const char*>(data), size));
    SysInstallerKitsImpl::GetInstance().ClearVabMetadataAndCow();
    SysInstallerKitsImpl::GetInstance().MergeRollbackReasonFile();
    int32_t metadataStatus = 0;
    SysInstallerKitsImpl::GetInstance().GetMetadataUpdateStatus(metadataStatus);
    SysInstallerKitsImpl::GetInstance().VabUpdateActive();
    SysInstallerKitsImpl::GetInstance().StartVabMerge(taskId);
    SysInstallerKitsImpl::GetInstance().EnableVabCheckpoint();
    SysInstallerKitsImpl::GetInstance().AbortVabActiveSnapshot();
    const std::string action = "needMerge";
    bool result = false;
    SysInstallerKitsImpl::GetInstance().GetMetadataResult(action, result);
    uint32_t reservedCores;
    std::memcpy(&reservedCores, data, std::min(sizeof(uint32_t), size));
    SysInstallerKitsImpl::GetInstance().SetCpuAffinity(taskId, reservedCores);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzSysInstaller(data, size);
    return 0;
}

