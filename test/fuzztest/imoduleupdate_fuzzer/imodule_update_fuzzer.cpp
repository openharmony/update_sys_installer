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

#include "imodule_update_fuzzer.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include "module_ipc_helper.h"
#include "module_update_kits.h"
#include "module_update_kits_impl.h"

namespace OHOS {
using namespace SysInstaller;
void FuzzModuleUpdate(const uint8_t* data, size_t size)
{
    ModuleUpdateKits &moduleUpdateKits = ModuleUpdateKits::GetInstance();
    moduleUpdateKits.InstallModulePackage(std::string(reinterpret_cast<const char*>(data), size));
    moduleUpdateKits.UninstallModulePackage(std::string(reinterpret_cast<const char*>(data), size));
    std::list<ModulePackageInfo> infos;
    moduleUpdateKits.GetModulePackageInfo(std::string(reinterpret_cast<const char*>(data), size), infos);
    ModuleUpdateStatus updateStatus;
    updateStatus.process = std::string(reinterpret_cast<const char*>(data), size);
    moduleUpdateKits.ReportModuleUpdateStatus(updateStatus);
    moduleUpdateKits.ExitModuleUpdate();
}

class ProcessCallback : public ISysInstallerCallbackFunc {
public:
    ProcessCallback() = default;
    ~ProcessCallback() = default;
    void OnUpgradeProgress(UpdateStatus updateStatus, int percent, const std::string &resultMsg) override
    {
        printf("ProgressCallback progress %d percent %d msg %s\n", updateStatus, percent, resultMsg.c_str());
    }
};

void FuzzModuleUpdateOther(const uint8_t* data, size_t size)
{
    ModuleUpdateKits &moduleUpdateKits = ModuleUpdateKits::GetInstance();
    std::vector<HmpVersionInfo> versionInfo = moduleUpdateKits.GetHmpVersionInfo();
    sptr<ISysInstallerCallbackFunc> callback = new ProcessCallback;
    moduleUpdateKits.StartUpdateHmpPackage(std::string(reinterpret_cast<const char*>(data), size),
        callback);
    std::vector<HmpUpdateInfo> updateInfo = moduleUpdateKits.GetHmpUpdateResult();
    moduleUpdateKits.ExitModuleUpdate();
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzModuleUpdate(data, size);
    return 0;
}

