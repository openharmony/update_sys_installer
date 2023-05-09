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

#include "module_update_service.h"

#include "directory_ex.h"
#include "log/log.h"
#include "module_constants.h"
#include "module_error_code.h"
#include "module_file.h"
#include "module_utils.h"
#include "package/package.h"
#include "scope_guard.h"
#include "system_ability_definition.h"
#include "utils.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

namespace {
constexpr mode_t DIR_MODE = 0750;
constexpr mode_t ALL_PERMISSIONS = 0777;

int32_t CreateModuleDirs(const std::string &hmpName)
{
    if (!CreateDirIfNeeded(UPDATE_INSTALL_DIR, DIR_MODE)) {
        LOG(ERROR) << "Failed to create install dir";
        return ModuleErrorCode::ERR_INSTALL_FAIL;
    }
    std::string hmpInstallDir = std::string(UPDATE_INSTALL_DIR) + "/" + hmpName;
    if (!CreateDirIfNeeded(hmpInstallDir, DIR_MODE)) {
        LOG(ERROR) << "Failed to create hmp install dir " << hmpInstallDir;
        return ModuleErrorCode::ERR_INSTALL_FAIL;
    }
    std::string hmpActiveDir = std::string(UPDATE_ACTIVE_DIR) + "/" + hmpName;
    if (!CreateDirIfNeeded(hmpActiveDir, DIR_MODE)) {
        LOG(ERROR) << "Failed to create hmp active dir " << hmpActiveDir;
        return ModuleErrorCode::ERR_INSTALL_FAIL;
    }
    return ModuleErrorCode::MODULE_UPDATE_SUCCESS;
}

bool ClearModuleDirs(const std::string &hmpName)
{
    std::string hmpInstallDir = std::string(UPDATE_INSTALL_DIR) + "/" + hmpName;
    std::string hmpActiveDir = std::string(UPDATE_ACTIVE_DIR) + "/" + hmpName;
    return ForceRemoveDirectory(hmpInstallDir) && ForceRemoveDirectory(hmpActiveDir);
}
}

ModuleUpdateService::ModuleUpdateService() : SystemAbility(MODULE_UPDATE_SERVICE_ID, true)
{
    LOG(INFO) << "ModuleUpdateService begin";
}

ModuleUpdateService::~ModuleUpdateService()
{
    LOG(INFO) << "ModuleUpdateService end";
}

int32_t ModuleUpdateService::InstallModulePackage(const std::string &pkgPath)
{
    LOG(INFO) << "InstallModulePackage " << pkgPath;
    std::string realPath;
    if (!CheckFileSuffix(pkgPath, HMP_PACKAGE_SUFFIX) || !PathToRealPath(pkgPath, realPath)) {
        LOG(ERROR) << "Invalid package path " << pkgPath;
        return ModuleErrorCode::ERR_INVALID_PATH;
    }
    std::string hmpName = GetFileName(realPath);
    if (hmpName.empty()) {
        LOG(ERROR) << "Failed to get hmp name " << realPath;
        return ModuleErrorCode::ERR_INVALID_PATH;
    }
    if (hmpSet_.find(hmpName) == hmpSet_.end()) {
        LOG(ERROR) << "Failed to install hmp without preInstall";
        return ModuleErrorCode::ERR_INSTALL_FAIL;
    }
    int32_t ret = CreateModuleDirs(hmpName);
    if (ret != ModuleErrorCode::MODULE_UPDATE_SUCCESS) {
        ClearModuleDirs(hmpName);
        return ret;
    }
    ON_SCOPE_EXIT(rmdir) {
        if (!ClearModuleDirs(hmpName)) {
            LOG(WARNING) << "Failed to remove " << hmpName;
        }
    };
    std::string hmpDir = std::string(UPDATE_INSTALL_DIR) + "/" + hmpName;
    std::string outPath = hmpDir + "/";
    ret = ExtraPackageDir(realPath.c_str(), nullptr, nullptr, outPath.c_str());
    if (ret != 0) {
        LOG(ERROR) << "Failed to unpack hmp package " << realPath;
        return ModuleErrorCode::ERR_INSTALL_FAIL;
    }
    std::vector<std::string> files;
    GetDirFiles(hmpDir, files);
    for (auto &file : files) {
        ret = InstallModuleFile(hmpName, file);
        if (ret != ModuleErrorCode::MODULE_UPDATE_SUCCESS) {
            return ret;
        }
    }
    if (!BackupActiveModules()) {
        LOG(ERROR) << "Failed to backup active modules";
        return ModuleErrorCode::ERR_INSTALL_FAIL;
    }
    CANCEL_SCOPE_EXIT_GUARD(rmdir);
    return ModuleErrorCode::MODULE_UPDATE_SUCCESS;
}

int32_t ModuleUpdateService::InstallModuleFile(const std::string &hmpName, const std::string &file) const
{
    if (!CheckFileSuffix(file, MODULE_PACKAGE_SUFFIX)) {
        return ModuleErrorCode::MODULE_UPDATE_SUCCESS;
    }
    std::string fileName = GetFileName(file);
    if (fileName.empty()) {
        return ModuleErrorCode::MODULE_UPDATE_SUCCESS;
    }
    std::unique_ptr<ModuleFile> moduleFile = ModuleFile::Open(file);
    if (moduleFile == nullptr) {
        LOG(ERROR) << "Wrong module file " << file << " in hmp package " << hmpName;
        return ModuleErrorCode::ERR_INSTALL_FAIL;
    }
    if (!moduleFile->GetImageStat().has_value()) {
        LOG(ERROR) << "Could not install empty module package " << file;
        return ModuleErrorCode::ERR_INSTALL_FAIL;
    }
    if (saIdHmpMap_.find(moduleFile->GetSaId()) == saIdHmpMap_.end()) {
        LOG(ERROR) << "Could not update module file " << file << " without preInstalled";
        return ModuleErrorCode::ERR_INSTALL_FAIL;
    }
    std::string preInstalledHmp = saIdHmpMap_.at(moduleFile->GetSaId());
    if (preInstalledHmp != hmpName) {
        LOG(ERROR) << "Module file " << file << " should be in hmp " << preInstalledHmp;
        return ModuleErrorCode::ERR_INSTALL_FAIL;
    }
    if (!ModuleFile::VerifyModulePackageSign(file)) {
        LOG(ERROR) << "Verify sign failed " << file;
        return ModuleErrorCode::ERR_VERIFY_SIGN_FAIL;
    }

    std::string preInstalledPath = std::string(MODULE_PREINSTALL_DIR) + "/" + hmpName + "/" + fileName
        + MODULE_PACKAGE_SUFFIX;
    std::unique_ptr<ModuleFile> preInstalledFile = ModuleFile::Open(preInstalledPath);
    if (preInstalledFile == nullptr) {
        LOG(ERROR) << "Invalid preinstalled file " << preInstalledPath;
        return ModuleErrorCode::ERR_INSTALL_FAIL;
    }
    if (!ModuleFile::CompareVersion(*moduleFile, *preInstalledFile)) {
        LOG(ERROR) << "Installed lower version of " << file;
        return ModuleErrorCode::ERR_LOWER_VERSION;
    }
    if (!moduleFile->VerifyModuleVerity(preInstalledFile->GetPublicKey())) {
        LOG(ERROR) << "Failed to verify module verity " << file;
        return ModuleErrorCode::ERR_VERIFY_SIGN_FAIL;
    }
    moduleFile->ClearVerifiedData();
    return ModuleErrorCode::MODULE_UPDATE_SUCCESS;
}

int32_t ModuleUpdateService::UninstallModulePackage(const std::string &hmpName)
{
    LOG(INFO) << "UninstallModulePackage " << hmpName;
    int ret = ModuleErrorCode::MODULE_UPDATE_SUCCESS;
    if (hmpName.empty() || hmpSet_.find(hmpName) == hmpSet_.end()) {
        return ret;
    }
    std::vector<std::string> uninstallDir {UPDATE_INSTALL_DIR, UPDATE_ACTIVE_DIR, UPDATE_BACKUP_DIR};
    std::string hmpDir = "/" + hmpName;
    bool hmpIsValid = false;
    for (auto &iter : uninstallDir) {
        std::string dir = iter + hmpDir;
        if (!CheckPathExists(dir)) {
            continue;
        }
        hmpIsValid = true;
        if (!ForceRemoveDirectory(dir)) {
            LOG(ERROR) << "Failed to remove " << dir;
            ret = ModuleErrorCode::ERR_UNINSTALL_FAIL;
        }
    }
    if (!hmpIsValid) {
        ret = ModuleErrorCode::ERR_INVALID_PATH;
    }
    return ret;
}

int32_t ModuleUpdateService::GetModulePackageInfo(const std::string &hmpName,
    std::list<ModulePackageInfo> &modulePackageInfos)
{
    LOG(INFO) << "GetModulePackageInfo " << hmpName;
    if (hmpName.empty()) {
        for (auto &hmp : hmpSet_) {
            CollectModulePackageInfo(hmp, modulePackageInfos);
        }
    } else {
        CollectModulePackageInfo(hmpName, modulePackageInfos);
    }
    return ModuleErrorCode::MODULE_UPDATE_SUCCESS;
}

void ModuleUpdateService::CollectModulePackageInfo(const std::string &hmpName,
    std::list<ModulePackageInfo> &modulePackageInfos) const
{
    if (hmpName.empty()) {
        return;
    }
    std::string installHmpPath = std::string(UPDATE_INSTALL_DIR) + "/" + hmpName;
    std::string activeHmpPath = std::string(UPDATE_ACTIVE_DIR) + "/" + hmpName;
    if (!CheckPathExists(installHmpPath) && !CheckPathExists(activeHmpPath)) {
        return;
    }
    std::vector<std::string> files;
    GetDirFiles(installHmpPath, files);
    GetDirFiles(activeHmpPath, files);
    ModulePackageInfo info;
    info.hmpName = hmpName;
    for (auto &file : files) {
        if (!CheckFileSuffix(file, MODULE_PACKAGE_SUFFIX)) {
            continue;
        }
        std::unique_ptr<ModuleFile> moduleFile = ModuleFile::Open(file);
        if (moduleFile == nullptr) {
            continue;
        }
        SaInfo saInfo;
        saInfo.saName = moduleFile->GetSaName();
        saInfo.saId = moduleFile->GetSaId();
        saInfo.version = moduleFile->GetVersionInfo();
        info.saInfoList.emplace_back(std::move(saInfo));
    }
    modulePackageInfos.emplace_back(std::move(info));
}

int32_t ModuleUpdateService::ReportModuleUpdateStatus(const ModuleUpdateStatus &status)
{
    LOG(INFO) << "ReportModuleUpdateStatus process=" << status.process;
    if (status.process.empty()) {
        LOG(ERROR) << "empty process name";
        return ModuleErrorCode::ERR_REPORT_STATUS_FAIL;
    }
    std::unordered_set<std::string> hmpSet;
    for (auto &iter : status.saStatusList) {
        ProcessSaStatus(iter, hmpSet);
    }
    processHmpMap_.emplace(status.process, hmpSet);
    return ModuleErrorCode::MODULE_UPDATE_SUCCESS;
}

int32_t ModuleUpdateService::ExitModuleUpdate()
{
    LOG(INFO) << "ExitModuleUpdate";
    exit(0);
}

void ModuleUpdateService::ProcessSaStatus(const SaStatus &status, std::unordered_set<std::string> &hmpSet)
{
    if (saIdHmpMap_.find(status.saId) == saIdHmpMap_.end()) {
        return;
    }
    std::string hmpName = saIdHmpMap_.at(status.saId);
    hmpSet.emplace(hmpName);
    if (!status.isMountSuccess) {
        OnHmpError(hmpName);
    }
}

void ModuleUpdateService::OnStart()
{
    LOG(INFO) << "OnStart";
}

void ModuleUpdateService::OnStop()
{
    LOG(INFO) << "OnStop";
}

void ModuleUpdateService::OnProcessCrash(const std::string &processName)
{
    if (processHmpMap_.find(processName) == processHmpMap_.end()) {
        return;
    }
    std::unordered_set<std::string> &hmpSet = processHmpMap_.at(processName);
    for (auto &hmp : hmpSet) {
        OnHmpError(hmp);
    }
}

void ModuleUpdateService::OnBootCompleted()
{
    LOG(INFO) << "Deleting " << UPDATE_INSTALL_DIR;
    if (!ForceRemoveDirectory(UPDATE_INSTALL_DIR)) {
        LOG(ERROR) << "Failed to remove " << UPDATE_INSTALL_DIR << " err=" << errno;
    }
    ExitModuleUpdate();
}

void ModuleUpdateService::OnHmpError(const std::string &hmpName)
{
    LOG(INFO) << "OnHmpError hmpName=" << hmpName;
    std::string errPath = std::string(UPDATE_INSTALL_DIR) + "/" + hmpName;
    if (!CheckPathExists(errPath)) {
        LOG(INFO) << "No update package in " << hmpName;
        return;
    }
    if (!ForceRemoveDirectory(errPath)) {
        LOG(ERROR) << "Failed to remove " << errPath;
        return;
    }
    RevertAndReboot();
}

bool ModuleUpdateService::BackupActiveModules() const
{
    if (!CheckPathExists(UPDATE_ACTIVE_DIR)) {
        LOG(INFO) << "Nothing to backup";
        return true;
    }
    if (CheckPathExists(UPDATE_BACKUP_DIR)) {
        if (!ForceRemoveDirectory(UPDATE_BACKUP_DIR)) {
            LOG(ERROR) << "Failed to remove backup dir";
            return false;
        }
    }
    if (!CreateDirIfNeeded(UPDATE_BACKUP_DIR, DIR_MODE)) {
        LOG(ERROR) << "Failed to create backup dir";
        return false;
    }

    std::vector<std::string> activeFiles;
    GetDirFiles(UPDATE_ACTIVE_DIR, activeFiles);
    ON_SCOPE_EXIT(rmdir) {
        if (!ForceRemoveDirectory(UPDATE_BACKUP_DIR)) {
            LOG(WARNING) << "Failed to remove backup dir when backup failed";
        }
    };
    for (auto &file : activeFiles) {
        if (!CheckFileSuffix(file, MODULE_PACKAGE_SUFFIX)) {
            continue;
        }
        std::string fileName = GetFileName(file);
        std::string hmpName = GetHmpName(file);
        if (fileName.empty() || hmpName.empty()) {
            continue;
        }
        std::unique_ptr<ModuleFile> moduleFile = ModuleFile::Open(file);
        if (moduleFile == nullptr) {
            LOG(ERROR) << "Wrong module file " << file << " in active dir";
            return false;
        }
        std::string destPath = std::string(UPDATE_BACKUP_DIR) + "/" + hmpName;
        if (!CreateDirIfNeeded(destPath, DIR_MODE)) {
            LOG(ERROR) << "Failed to create hmp dir " << destPath;
            return false;
        }
        std::string destFile = destPath + "/" + fileName + MODULE_PACKAGE_SUFFIX;
        int ret = link(file.c_str(), destFile.c_str());
        if (ret != 0) {
            LOG(ERROR) << "Failed to link file " << file << " to dest " << destFile;
            return false;
        }
    }

    CANCEL_SCOPE_EXIT_GUARD(rmdir);
    return true;
}

bool ModuleUpdateService::RevertAndReboot() const
{
    LOG(INFO) << "RevertAndReboot";
    if (!CheckPathExists(UPDATE_BACKUP_DIR)) {
        LOG(ERROR) << UPDATE_BACKUP_DIR << " does not exist";
        return false;
    }
    struct stat statData;
    int ret = stat(UPDATE_ACTIVE_DIR, &statData);
    if (ret != 0) {
        LOG(ERROR) << "Failed to access " << UPDATE_ACTIVE_DIR << " err=" << errno;
        return false;
    }
    if (!ForceRemoveDirectory(UPDATE_ACTIVE_DIR)) {
        LOG(ERROR) << "Failed to remove " << UPDATE_ACTIVE_DIR;
        return false;
    }

    ret = rename(UPDATE_BACKUP_DIR, UPDATE_ACTIVE_DIR);
    if (ret != 0) {
        LOG(ERROR) << "Failed to rename " << UPDATE_BACKUP_DIR << " to " << UPDATE_ACTIVE_DIR << " err=" << errno;
        return false;
    }
    ret = chmod(UPDATE_ACTIVE_DIR, statData.st_mode & ALL_PERMISSIONS);
    if (ret != 0) {
        LOG(ERROR) << "Failed to restore original permissions for " << UPDATE_ACTIVE_DIR << " err=" << errno;
        return false;
    }

    LOG(INFO) << "Rebooting";
    Utils::UpdaterDoReboot("");
    return true;
}

void ModuleUpdateService::ScanPreInstalledHmp()
{
    std::vector<std::string> files;
    GetDirFiles(MODULE_PREINSTALL_DIR, files);
    for (auto &file : files) {
        if (!CheckFileSuffix(file, MODULE_PACKAGE_SUFFIX)) {
            continue;
        }
        std::unique_ptr<ModuleFile> moduleFile = ModuleFile::Open(file);
        if (moduleFile == nullptr) {
            continue;
        }
        std::string hmpName = GetHmpName(file);
        if (hmpName.empty()) {
            continue;
        }
        hmpSet_.emplace(hmpName);
        saIdHmpMap_.emplace(moduleFile->GetSaId(), hmpName);
    }
}
} // namespace SysInstaller
} // namespace OHOS
