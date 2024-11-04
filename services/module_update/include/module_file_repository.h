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

#ifndef SYS_INSTALLER_MODULE_FILE_REPOSITORY_H
#define SYS_INSTALLER_MODULE_FILE_REPOSITORY_H

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "module_file.h"
#include "module_utils.h"
#include "singleton.h"

namespace OHOS {
namespace SysInstaller {
class ModuleFileRepository final {
public:
    DISALLOW_COPY_AND_MOVE(ModuleFileRepository);
    ModuleFileRepository() = default;
    ~ModuleFileRepository();
    void InitRepository(const std::string &hmpName, const Timer &timer);
    std::unique_ptr<ModuleFile> GetModuleFile(const std::string &pathPrefix, const std::string &hmpName) const;
    bool IsPreInstalledModule(const ModuleFile &moduleFile) const;
    void Clear();
    void SaveInstallerResult(const std::string &path, const std::string &hmpName,
        int result, const std::string &resultInfo, const Timer &timer) const;
private:
    void ProcessFile(const std::string &hmpName, const std::string &path, const std::string &file,
        std::unordered_map<std::string, ModuleFile> &fileMap, const Timer &timer) const;
    bool CheckFilePath(const ModuleFile &moduleFile, const std::string &prefix) const;

    std::unordered_map<std::string, std::unordered_map<std::string, ModuleFile>> moduleFileMap_;
};
} // SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_FILE_REPOSITORY_H