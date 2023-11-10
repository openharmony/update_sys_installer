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

#include "module_file_repository.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "directory_ex.h"
#include "log/log.h"
#include "module_constants.h"
#include "module_utils.h"
#include "module_error_code.h"
#include "scope_guard.h"
#include "unique_fd.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;
using std::string;

ModuleFileRepository::~ModuleFileRepository()
{
    Clear();
}

ModuleFileRepository &ModuleFileRepository::GetInstance()
{
    static ModuleFileRepository instance;
    return instance;
}

void ModuleFileRepository::InitRepository(const std::unordered_set<int32_t> &saIdSet)
{
    string allPath[] = {MODULE_PREINSTALL_DIR, UPDATE_INSTALL_DIR, UPDATE_ACTIVE_DIR};
    for (string &path : allPath) {
        std::vector<string> files;
        GetDirFiles(path, files);
        std::unordered_map<int32_t, ModuleFile> fileMap;
        for (string &file : files) {
            ProcessFile(saIdSet, path, file, fileMap);
        }
        moduleFileMap_.emplace(path, std::move(fileMap));
    }
}

void ModuleFileRepository::SaveInstallerResult(const std::string &path, const std::string &hmpName,
    int result, const std::string &resultInfo) const
{
    if (path.find(UPDATE_INSTALL_DIR) == std::string::npos && path.find(UPDATE_ACTIVE_DIR) == std::string::npos) {
        return;
    }
    if (!CheckFileSuffix(path, MODULE_PACKAGE_SUFFIX)) {
        return;
    }
    if (!CheckPathExists(MODULE_RESULT_PATH)) {
        LOG(ERROR) << MODULE_RESULT_PATH << " not exist";
        return;
    }
    LOG(INFO) << "path:" << path << "hmp:" << hmpName << "result:" << result << "Info:" << resultInfo << "\n";

    UniqueFd fd(open(MODULE_RESULT_PATH, O_APPEND | O_RDWR | O_CLOEXEC));
    if (fd.Get() == -1) {
        LOG(ERROR) << "Failed to open file";
        return;
    }

    std::string writeInfo = hmpName + ";" + std::to_string(result) + ";" + resultInfo + "\n";
    if (write(fd, writeInfo.data(), writeInfo.length()) <= 0) {
        LOG(WARNING) << "write result file failed, err:" << errno;
    }
    fsync(fd.Get());
}

void ModuleFileRepository::ProcessFile(const std::unordered_set<int32_t> &saIdSet, const string &path,
    const string &file, std::unordered_map<int32_t, ModuleFile> &fileMap) const
{
    if (!CheckFileSuffix(file, MODULE_PACKAGE_SUFFIX)) {
        return;
    }
    std::unique_ptr<ModuleFile> moduleFile = ModuleFile::Open(file);
    if (moduleFile == nullptr || saIdSet.find(moduleFile->GetSaId()) == saIdSet.end()) {
        return;
    }
    string pubkey = moduleFile->GetPublicKey();
    if (path != MODULE_PREINSTALL_DIR) {
        pubkey = GetPublicKey(moduleFile->GetSaId());
        if (!CheckFilePath(*moduleFile, path)) {
            LOG(ERROR) << "Open " << file << " failed";
            SaveInstallerResult(path, GetHmpName(moduleFile->GetPath()),
                ModuleErrorCode::ERR_VERIFY_SIGN_FAIL, "get pub key fail");
            return;
        }
        if (VerifyModulePackageSign(file) != 0) {
            LOG(ERROR) << "VerifyModulePackageSign failed of " << file;
            SaveInstallerResult(path, GetHmpName(moduleFile->GetPath()),
                ModuleErrorCode::ERR_VERIFY_SIGN_FAIL, "verify fail");
            return;
        }
    }
    if (moduleFile->GetImageStat().has_value() && !moduleFile->VerifyModuleVerity(pubkey)) {
        SaveInstallerResult(path, GetHmpName(moduleFile->GetPath()),
            ModuleErrorCode::ERR_VERIFY_SIGN_FAIL, "hvb fail");
        LOG(ERROR) << "verify verity failed of " << file;
        return;
    }
    LOG(INFO) << "ProcessFile  " << file << " successful";
    fileMap.emplace(moduleFile->GetSaId(), std::move(*moduleFile));
}

std::unique_ptr<ModuleFile> ModuleFileRepository::GetModuleFile(const std::string &pathPrefix, const int32_t saId) const
{
    auto mapIter = moduleFileMap_.find(pathPrefix);
    if (mapIter == moduleFileMap_.end()) {
        LOG(ERROR) << "Invalid path prefix " << pathPrefix;
        return nullptr;
    }
    std::unordered_map<int32_t, ModuleFile> fileMap = mapIter->second;
    auto fileIter = fileMap.find(saId);
    if (fileIter == fileMap.end()) {
        LOG(INFO) << saId << " not found in " << pathPrefix;
        return nullptr;
    }
    ModuleFile file = fileIter->second;
    return std::make_unique<ModuleFile>(std::move(file));
}

bool ModuleFileRepository::IsPreInstalledModule(const ModuleFile &moduleFile) const
{
    std::unique_ptr<ModuleFile> preInstalledModule = GetModuleFile(MODULE_PREINSTALL_DIR, moduleFile.GetSaId());
    if (preInstalledModule == nullptr) {
        return false;
    }
    return preInstalledModule->GetPath() == moduleFile.GetPath();
}

string ModuleFileRepository::GetPublicKey(const int32_t saId) const
{
    std::unique_ptr<ModuleFile> preInstalledModule = GetModuleFile(MODULE_PREINSTALL_DIR, saId);
    if (preInstalledModule == nullptr) {
        return "";
    }
    return preInstalledModule->GetPublicKey();
}

bool ModuleFileRepository::CheckFilePath(const ModuleFile &moduleFile, const string &prefix) const
{
    std::unique_ptr<ModuleFile> preInstalledModule = GetModuleFile(MODULE_PREINSTALL_DIR, moduleFile.GetSaId());
    if (preInstalledModule == nullptr) {
        return false;
    }
    string prePath = preInstalledModule->GetPath();
    string curPath = moduleFile.GetPath();
    return prePath.substr(strlen(MODULE_PREINSTALL_DIR), prePath.length()) ==
        curPath.substr(prefix.length(), curPath.length());
}

void ModuleFileRepository::Clear()
{
    for (auto mapIter = moduleFileMap_.begin(); mapIter != moduleFileMap_.end(); ++mapIter) {
        std::unordered_map<int32_t, ModuleFile> &fileMap = mapIter->second;
        for (auto fileIter = fileMap.begin(); fileIter != fileMap.end(); ++fileIter) {
            fileIter->second.ClearVerifiedData();
        }
        fileMap.clear();
    }
    moduleFileMap_.clear();
}
} // SysInstaller
} // namespace OHOS
