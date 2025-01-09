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

#ifndef SYS_INSTALLER_MODULE_UPDATE_H
#define SYS_INSTALLER_MODULE_UPDATE_H

#include <memory>
#include <list>
#include <unordered_set>

#include "module_file.h"
#include "module_file_repository.h"
#include "module_ipc_helper.h"

namespace OHOS {
namespace SysInstaller {
using ImageVerifyFunc = std::function<bool(ModuleFile &, std::string &)>;

class ModuleUpdate {
public:
    static ModuleUpdate &GetInstance();
    virtual ~ModuleUpdate() = default;
    void CheckModuleUpdate();
    bool DoModuleUpdate(ModuleUpdateStatus &status);
    void RegisterImageVerifyFunc(ImageVerifyFunc ptr, int32_t level);

private:
    void PrepareModuleFileList(const ModuleUpdateStatus &status);
    bool ActivateModules(ModuleUpdateStatus &status, const Timer &timer);
    bool MountModulePackage(ModuleFile &moduleFile, const bool mountOnVerity);
    void ReportModuleUpdateStatus(const ModuleUpdateStatus &status) const;
    void WaitDevice(const std::string &blockDevice) const;
    bool CheckMountComplete(const std::string &hmpName) const;
    void ProcessHmpFile(const std::string &hmpFile, const ModuleUpdateStatus &status, const Timer &timer);
    std::unique_ptr<ModuleFile> GetLatestUpdateModulePackage(const std::string &hmpName);
    bool CheckRevert(const std::string &hmpName);
    std::string CreateMountPoint(const ModuleFile &moduleFile) const;
    bool VerifyImageAndCreateDm(ModuleFile &moduleFile, bool mountOnVerity, std::string &blockDevice);

    std::list<ModuleFile> moduleFileList_;
    ModuleFileRepository repository_;
    ImageVerifyFunc ImageVerifyFunc_ = nullptr;
    int32_t registeredLevel_ = 0;
};
} // SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_UPDATE_H