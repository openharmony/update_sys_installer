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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "directory_ex.h"
#include "init_reboot.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "json_node.h"
#include "log/log.h"
#include "module_constants.h"
#include "module_error_code.h"
#include "module_file.h"
#include "module_utils.h"
#include "package/package.h"
#include "scope_guard.h"
#include "system_ability_definition.h"
#include "utils.h"
#include "unique_fd.h"
#ifdef WITH_SELINUX
#include <policycoreutils.h>
#endif // WITH_SELINUX

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
#ifdef WITH_SELINUX
    if (Restorecon(UPDATE_INSTALL_DIR) == -1) {
        LOG(WARNING) << "restore " << UPDATE_INSTALL_DIR << " failed";
    }
#endif // WITH_SELINUX
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
    return ForceRemoveDirectory(hmpInstallDir);
}

std::string GetFileAllName(const std::string &path)
{
    auto pos = path.find_last_of('/');
    if (pos == std::string::npos) {
        pos = path.find_last_of('\\');
        if (pos == std::string::npos) {
            return "";
        }
    }
    return path.substr(pos + 1);
}

bool BackupFile(const std::string &file)
{
    std::string fileName = GetFileAllName(file);
    std::string hmpName = GetHmpName(file);
    if (fileName.empty() || hmpName.empty()) {
        return true;
    }
    std::string destPath = std::string(UPDATE_BACKUP_DIR) + "/" + hmpName;
    if (!CreateDirIfNeeded(destPath, DIR_MODE)) {
        LOG(ERROR) << "Failed to create hmp dir " << destPath;
        return false;
    }
    std::string destFile = destPath + "/" + fileName;
    int ret = link(file.c_str(), destFile.c_str());
    if (ret != 0) {
        LOG(ERROR) << "Failed to link file " << file << " to dest " << destFile;
        return false;
    }
    return true;
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
    return ReallyInstallModulePackage(realPath, nullptr);
}

int32_t ModuleUpdateService::ReallyInstallModulePackage(const std::string &pkgPath,
    const sptr<ISysInstallerCallback> &updateCallback)
{
    std::string hmpName = GetFileName(pkgPath);
    if (hmpName.empty()) {
        LOG(ERROR) << "Failed to get hmp name " << pkgPath;
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
    ret = ExtraPackageDir(pkgPath.c_str(), nullptr, nullptr, outPath.c_str());
    if (ret != 0) {
        LOG(ERROR) << "Failed to unpack hmp package " << pkgPath;
        return ModuleErrorCode::ERR_INSTALL_FAIL;
    }
    std::vector<std::string> files;
    GetDirFiles(hmpDir, files);
    int index = 1;
    for (auto &file : files) {
        ret = InstallModuleFile(hmpName, file);
        if (ret != ModuleErrorCode::MODULE_UPDATE_SUCCESS) {
            return ret;
        }
        if (updateCallback != nullptr) {
            int percent = static_cast<float>(index) / files.size() * 95;  // 95 : 95% percent
            updateCallback->OnUpgradeProgress(UPDATE_STATE_ONGOING, percent, "");
        }
        index++;
    }
    if (!BackupActiveModules()) {
        LOG(ERROR) << "Failed to backup active modules";
        return ModuleErrorCode::ERR_INSTALL_FAIL;
    }
    CANCEL_SCOPE_EXIT_GUARD(rmdir);
    sync();
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
    if (VerifyModulePackageSign(file) != 0) {
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
        return ModuleErrorCode::ERR_INVALID_PATH;
    }
    std::vector<std::string> uninstallDir {UPDATE_INSTALL_DIR, UPDATE_ACTIVE_DIR, UPDATE_BACKUP_DIR};
    std::string hmpDir = "/" + hmpName;
    bool hmpIsValid = false;
    for (const auto &iter : uninstallDir) {
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
    for (const auto &iter : status.saStatusList) {
        ProcessSaStatus(iter, hmpSet);
    }
    processHmpMap_.emplace(status.process, hmpSet);
    return ModuleErrorCode::MODULE_UPDATE_SUCCESS;
}

int32_t ModuleUpdateService::ExitModuleUpdate()
{
    LOG(INFO) << "ExitModuleUpdate";
    sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        LOG(INFO) << "GetSystemAbilityManager samgr object null";
        return -1;
    }
    int32_t ret = samgr->RemoveSystemAbility(MODULE_UPDATE_SERVICE_ID);
    LOG(INFO) << "RemoveSystemAbility ret: " << ret;
    IPCSkeleton::StopWorkThread();
    return 0;
}

bool ModuleUpdateService::GetHmpVersion(const std::string &hmpPath, HmpVersionInfo &versionInfo)
{
    LOG(INFO) << "GetHmpVersion " << hmpPath;
    std::string packInfoPath = hmpPath + "/" + PACK_INFO_NAME;
    if (VerifyModulePackageSign(packInfoPath) != 0) {
        LOG(ERROR) << "Verify sign failed " << packInfoPath;
        return false;
    }
    JsonNode root(std::filesystem::path { packInfoPath });
    const JsonNode &package = root["package"];
    std::optional<std::string> name = package["name"].As<std::string>();
    if (!name.has_value()) {
        LOG(ERROR) << "count get name val";
        return false;
    }

    std::optional<std::string> version = package["version"].As<std::string>();
    if (!version.has_value()) {
        LOG(ERROR) << "count get version val";
        return false;
    }

    const JsonNode &laneInfoNode = package["laneInfo"];
    std::optional<std::string> compatibleVersion = laneInfoNode["compatibleVersion"].As<std::string>();
    if (!compatibleVersion.has_value()) {
        LOG(ERROR) << "count get compatibleVersion val";
        return false;
    }

    std::optional<std::string> laneCode = laneInfoNode["laneCode"].As<std::string>();
    if (!laneCode.has_value()) {
        LOG(ERROR) << "count get laneCode val";
        return false;
    }

    versionInfo.name = name.value();
    versionInfo.version = version.value();
    versionInfo.compatibleVersion = compatibleVersion.value();
    versionInfo.laneCode = laneCode.value();
    return true;
}

void ModuleUpdateService::ParseHmpVersionInfo(std::vector<HmpVersionInfo> &versionInfos, const HmpVersionInfo &preInfo,
    const HmpVersionInfo &actInfo)
{
    if (preInfo.version.size() == 0 && actInfo.version.size() == 0) {
        LOG(WARNING) << "version is empty";
        return;
    }

    if (actInfo.version.size() == 0) {
        LOG(INFO) << "add preinstaller info";
        versionInfos.emplace_back(preInfo);
        return;
    }
    std::vector<std::string> preVersion {};
    std::vector<std::string> actVersion {};
    // version: xxx-d01 4.5.10.100
    if (!ParseVersion(preInfo.version, " ", preVersion) || !ParseVersion(actInfo.version, " ", actVersion)) {
        LOG(ERROR) << "ParseVersion failed";
        return;
    }

    if (ComparePackInfoVer(preVersion, actVersion)) {
        LOG(INFO) << "add active info";
        versionInfos.emplace_back(actInfo);
    } else {
        LOG(INFO) << "add preinstaller info";
        versionInfos.emplace_back(preInfo);
    }
}

std::vector<HmpVersionInfo> ModuleUpdateService::GetHmpVersionInfo()
{
    LOG(INFO) << "GetHmpVersionInfo";
    std::vector<HmpVersionInfo> versionInfos {};
    ScanPreInstalledHmp();
    for (auto &hmp : hmpSet_) {
        std::string preInstallHmpPath = std::string(MODULE_PREINSTALL_DIR) + "/" + hmp;
        std::string activeHmpPath = std::string(UPDATE_ACTIVE_DIR) + "/" + hmp;
        LOG(INFO) << "preInstallHmpPath:" << preInstallHmpPath << " activeHmpPath:" << activeHmpPath;
        HmpVersionInfo actinfo {};
        HmpVersionInfo preinfo {};
        (void)GetHmpVersion(preInstallHmpPath, preinfo);
        (void)GetHmpVersion(activeHmpPath, actinfo);
        ParseHmpVersionInfo(versionInfos, preinfo, actinfo);
    }
    return versionInfos;
}

void ModuleUpdateService::SaveInstallerResult(const std::string &hmpPath, int result, const std::string &resultInfo)
{
    LOG(INFO) << "hmpPath:" << hmpPath << " result:" << result << " resultInfo:" << resultInfo;
    UniqueFd fd(open(MODULE_RESULT_PATH, O_APPEND | O_RDWR | O_CREAT | O_CLOEXEC));
    if (fd.Get() == -1) {
        LOG(ERROR) << "Failed to open file";
        return;
    }
    constexpr mode_t mode = 0755; // 0755 : rwx-r-x-r-x
    if (chmod(MODULE_RESULT_PATH, mode) != 0) {
        LOG(ERROR) << "Could not chmod " << MODULE_RESULT_PATH;
    }
    std::string writeInfo = hmpPath + ";" + std::to_string(result) + ";" + resultInfo + "\n";
    if (write(fd, writeInfo.data(), writeInfo.length()) <= 0) {
        LOG(WARNING) << "write result file failed, err:" << errno;
    }
    fsync(fd.Get());
}

int32_t ModuleUpdateService::StartUpdateHmpPackage(const std::string &path,
    const sptr<ISysInstallerCallback> &updateCallback)
{
    int32_t ret = -1;
    ON_SCOPE_EXIT(saveResult) {
        SaveInstallerResult(path, ret, std::to_string(ret));
        if (updateCallback != nullptr) {
            updateCallback->OnUpgradeProgress(ret == 0 ? UPDATE_STATE_SUCCESSFUL : UPDATE_STATE_FAILED,
                100, ""); // 100 : 100% percent
        }
    };
    LOG(INFO) << "StartUpdateHmpPackage " << path;
    if (updateCallback == nullptr) {
        LOG(ERROR) << "StartUpdateHmpPackage updateCallback null";
        ret = ModuleErrorCode::ERR_INVALID_PATH;
        return ret;
    }

    updateCallback->OnUpgradeProgress(UPDATE_STATE_ONGOING, 0, "");
    if (VerifyModulePackageSign(path) != 0) {
        LOG(ERROR) << "Verify sign failed " << path;
        ret = ModuleErrorCode::ERR_VERIFY_SIGN_FAIL;
        return ret;
    }

    ret = InstallModulePackage(path);
    return ret;
}

std::vector<HmpUpdateInfo> ModuleUpdateService::GetHmpUpdateResult()
{
    LOG(INFO) << "GetHmpUpdateResult";
    std::vector<HmpUpdateInfo> updateInfo {};
    std::ifstream ifs { MODULE_RESULT_PATH };
    if (!ifs.is_open()) {
        LOG(ERROR) << "open " << MODULE_RESULT_PATH << " failed";
        return updateInfo;
    }
    std::string resultInfo {std::istreambuf_iterator<char> {ifs}, {}};
    std::vector<std::string> results {};
    SplitStr(resultInfo, "\n", results);
    for (auto &result : results) {
        HmpUpdateInfo tmpUpdateInfo {};
        std::vector<std::string> signalResult {};
        SplitStr(result, ";", signalResult);
        if (signalResult.size() < 3) { // 3: pkg; result; result info
            LOG(ERROR) << "parse " << result << " failed";
            continue;
        }
        tmpUpdateInfo.path = signalResult[0];
        tmpUpdateInfo.result = stoi(signalResult[1]);
        tmpUpdateInfo.resultMsg = signalResult[2]; // 2: result info
        bool isFind = false;
        for (auto &iter : updateInfo) {
            if (iter.path.find(tmpUpdateInfo.path) != std::string::npos) {
                iter.result = tmpUpdateInfo.result;
                iter.resultMsg = tmpUpdateInfo.resultMsg;
                isFind = true;
                break;
            }
        }
        if (!isFind) {
            updateInfo.emplace_back(tmpUpdateInfo);
        }
    }
    ifs.close();
    (void)unlink(MODULE_RESULT_PATH);
    return updateInfo;
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
    std::string activePath = std::string(UPDATE_ACTIVE_DIR) + "/" + hmpName;
    if (!CheckPathExists(activePath)) {
        LOG(INFO) << "No update package in " << hmpName;
        return;
    }
    std::string errPath = std::string(UPDATE_INSTALL_DIR) + "/" + hmpName;
    if (CheckPathExists(errPath) && !ForceRemoveDirectory(errPath)) {
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
    for (const auto &file : activeFiles) {
        if (!BackupFile(file)) {
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
 
    sync();
    LOG(INFO) << "Rebooting";
    DoReboot("");
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
