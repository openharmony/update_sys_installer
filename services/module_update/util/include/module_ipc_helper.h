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

#ifndef SYS_INSTALLER_MODULE_IPC_HELPER_H
#define SYS_INSTALLER_MODULE_IPC_HELPER_H

#include <list>

#include "module_file.h"

namespace OHOS {
namespace SysInstaller {
struct SaStatus {
    int32_t saId;
    bool isPreInstalled;
    bool isMountSuccess;
};

struct ModuleUpdateStatus {
    std::string process;
    std::list<SaStatus> saStatusList;
};

struct SaInfo {
    std::string saName;
    int32_t saId;
    ModuleVersion version;
};

struct ModulePackageInfo {
    std::string hmpName;
    std::list<SaInfo> saInfoList;
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_IPC_HELPER_H