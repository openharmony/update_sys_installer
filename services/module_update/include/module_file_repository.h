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
#include "singleton.h"

namespace OHOS {
namespace SysInstaller {
class ModuleFileRepository final {
public:
    DISALLOW_COPY_AND_MOVE(ModuleFileRepository);
    static ModuleFileRepository &GetInstance();
    void InitRepository(const std::unordered_set<int32_t> &saIdSet);
    std::unique_ptr<ModuleFile> GetModuleFile(const std::string &pathPrefix, const int32_t saId) const;
    bool IsPreInstalledModule(const ModuleFile &moduleFile) const;
    std::string GetPublicKey(const int32_t saId) const;
    void Clear();
    void SaveInstallerResult(const std::string &path, const std::string &hmpName,
        int result, const std::string &resultInfo) const;
private:
    ModuleFileRepository() = default;
    ~ModuleFileRepository();
    void ProcessFile(const std::unordered_set<int32_t> &saIdSet, const std::string &path, const std::string &file,
        std::unordered_map<int32_t, ModuleFile> &fileMap) const;
    bool CheckFilePath(const ModuleFile &moduleFile, const std::string &prefix) const;

    std::unordered_map<std::string, std::unordered_map<int32_t, ModuleFile>> moduleFileMap_;
};
} // SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_FILE_REPOSITORY_H