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

#include "module_update_main.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

#include "directory_ex.h"
#include "module_utils.h"
#include "hisysevent_manager.h"
#include "json_node.h"
#include "log/log.h"
#include "module_update_service.h"
#include "parameter.h"
#include "system_ability_definition.h"
#include "module_constants.h"
#include "module_update_consumer.h"
#include "module_update_producer.h"
#include "module_error_code.h"
#include "module_file.h"
#include "module_update_verify.h"
#include "package/package.h"
#include "scope_guard.h"
#include "utils.h"
#include "unique_fd.h"

#ifdef WITH_SELINUX
#include <policycoreutils.h>
#endif // WITH_SELINUX

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

namespace {
constexpr int32_t RETRY_TIMES_FOR_SAMGR = 10;
constexpr std::chrono::milliseconds MILLISECONDS_WAITING_SAMGR_ONE_TIME(100);
constexpr mode_t DIR_MODE = 0750;

static volatile sig_atomic_t g_exit = 0;

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
    return ModuleErrorCode::MODULE_UPDATE_SUCCESS;
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

ModuleUpdateMain::ModuleUpdateMain()
{
}

ModuleUpdateMain::~ModuleUpdateMain() = default;

int32_t ModuleUpdateMain::CheckHmpName(const std::string &hmpName)
{
    if (hmpName.empty()) {
        LOG(ERROR) << "Failed to get hmpName=" << hmpName;
        return ModuleErrorCode::ERR_INVALID_PATH;
    }
    if (hmpSet_.find(hmpName) == hmpSet_.end()) {
        LOG(ERROR) << "Failed to install hmp without preInstall:" << hmpName;
        return ModuleErrorCode::ERR_INSTALL_FAIL;
    }
    int32_t ret = CreateModuleDirs(hmpName);
    if (ret != ModuleErrorCode::MODULE_UPDATE_SUCCESS) {
        RemoveSpecifiedDir(std::string(UPDATE_INSTALL_DIR) + "/" + hmpName, false);
        return ret;
    }
    return ModuleErrorCode::MODULE_UPDATE_SUCCESS;
}

/*
 * backup activeDir;
 * new create activeDir;
 * set param to notify bms update
 */
int32_t ModuleUpdateMain::PrepareResourceForReboot(const std::string &hmpName) const
{
    if (installModule_ == nullptr) {
        LOG(ERROR) << "install module is null";
        return ModuleErrorCode::ERR_DEAL_HVB_INFO_FAIL;
    }
    if (!BackupActiveModules(hmpName)) {
        LOG(ERROR) << "Failed to backup active hmp: " << hmpName;
        return ModuleErrorCode::ERR_BACKUP_FAIL;
    }
    if (!VerityInfoWrite(*installModule_)) {
        LOG(ERROR) << "verity info write fail";
        return ModuleErrorCode::ERR_DEAL_HVB_INFO_FAIL;
    }
    std::string hmpActiveDir = std::string(UPDATE_ACTIVE_DIR) + "/" + hmpName;
    if (SetParameter(BMS_START_INSTALL, BMS_UPDATE) != 0) {
        LOG(ERROR) << "Failed to set bms scan params: " << hmpActiveDir;
        return ModuleErrorCode::ERR_SET_PARAM_FAIL;
    }
    // create avtive hmp dir finally, avoid to be backup.
    if (!CreateDirIfNeeded(hmpActiveDir, DIR_MODE)) {
        LOG(ERROR) << "Failed to create hmp active dir " << hmpActiveDir;
        return ModuleErrorCode::ERR_CREATE_ACTIVE_FAIL;
    }
    sync();
    return ModuleErrorCode::MODULE_UPDATE_SUCCESS;
}

int32_t ModuleUpdateMain::ReallyInstallModulePackage(const std::string &pkgPath,
    const sptr<ISysInstallerCallback> &updateCallback)
{
    std::string hmpName = GetFileName(pkgPath);
    int32_t ret = CheckHmpName(hmpName);
    if (ret != ModuleErrorCode::MODULE_UPDATE_SUCCESS) {
        return ret;
    }
    std::string hmpDir = std::string(UPDATE_INSTALL_DIR) + "/" + hmpName;
    ON_SCOPE_EXIT(rmdir) {
        RemoveSpecifiedDir(hmpDir, false);
    };
    std::string outPath = hmpDir + "/";
    if (!PrepareFileToDestDir(pkgPath, outPath)) {
        LOG(ERROR) << "Failed to prepare file, " << pkgPath;
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
            updateCallback->OnUpgradeProgress(UpdateStatus::UPDATE_STATE_ONGOING, percent, "");
        }
        index++;
    }
    ret = PrepareResourceForReboot(hmpName);
    if (ret != ModuleErrorCode::MODULE_UPDATE_SUCCESS) {
        return ret;
    }
    CANCEL_SCOPE_EXIT_GUARD(rmdir);
    return ModuleErrorCode::MODULE_UPDATE_SUCCESS;
}

int32_t ModuleUpdateMain::ValidateVersion(ModuleFile &installFile, const std::string &hmpName) const
{
    std::string preInstalledPath = std::string(MODULE_PREINSTALL_DIR) + "/" + hmpName + "/" + HMP_INFO_NAME;
    std::unique_ptr<ModuleFile> preInstalledFile = ModuleFile::Open(preInstalledPath);
    if (preInstalledFile == nullptr) {
        LOG(ERROR) << "Invalid preinstalled file " << preInstalledPath;
        return ModuleErrorCode::ERR_INSTALL_FAIL;
    }
    if (!ModuleFile::CompareVersion(installFile, *preInstalledFile)) {
        LOG(ERROR) << "Installed version is lower than preInstall.";
        return ModuleErrorCode::ERR_LOWER_VERSION;
    }
    if (!installFile.VerifyModuleVerity()) {
        LOG(ERROR) << "Failed to verify install img: " << hmpName;
        return ModuleErrorCode::ERR_VERIFY_FAIL;
    }

    std::string activePath = std::string(UPDATE_ACTIVE_DIR) + "/" + hmpName + "/" + HMP_INFO_NAME;
    if (!Utils::IsFileExist(activePath)) {
        return ModuleErrorCode::MODULE_UPDATE_SUCCESS;
    }
    std::unique_ptr<ModuleFile> activeFile = ModuleFile::Open(activePath);
    if (activeFile == nullptr) {
        LOG(WARNING) << "Invalid active file " << activePath;
        return ModuleErrorCode::MODULE_UPDATE_SUCCESS;
    }
    if (!ModuleFile::CompareVersion(installFile, *activeFile)) {
        LOG(ERROR) << "Installed version is lower than active.";
        return ModuleErrorCode::ERR_LOWER_VERSION;
    }
    return ModuleErrorCode::MODULE_UPDATE_SUCCESS;
}

std::string ModuleUpdateMain::GetWorkHmpImagePath(const std::string &hmpName)
{
    // Get the path where the latest version is located
    GetHmpVersionInfo();
    std::string prefixPath = MODULE_PREINSTALL_DIR;
    for (const auto &[workHmp, imagePath] : hmpWorkDirMap_) {
        if (hmpName == workHmp) {
            prefixPath = imagePath;
        }
    }
    std::string result = prefixPath + "/" + hmpName + "/" + IMG_FILE_NAME;
    LOG(INFO) << "current work hmp path is " << result;
    return result;
}

int32_t ModuleUpdateMain::InstallModuleFile(const std::string &hmpName, const std::string &file)
{
    if (!CheckFileSuffix(file, MODULE_PACKAGE_SUFFIX)) {
        return ModuleErrorCode::MODULE_UPDATE_SUCCESS;
    }
    std::string fileName = GetFileName(file);
    if (fileName.empty()) {
        return ModuleErrorCode::MODULE_UPDATE_SUCCESS;
    }
    // verify first, then open module file.
    if (VerifyModulePackageSign(file) != 0) {
        LOG(ERROR) << "Verify sign failed " << file;
        return ModuleErrorCode::ERR_VERIFY_FAIL;
    }

    if (IsIncrementPackage(file) && !RestorePackage(file, GetWorkHmpImagePath(hmpName))) {
        LOG(ERROR) << "Restore package fail: " << file;
        return ModuleErrorCode::ERR_RESTORE_PACKAGE_FAIL;
    }

    std::unique_ptr<ModuleFile> moduleFile = ModuleFile::Open(file);
    if (moduleFile == nullptr) {
        LOG(ERROR) << "Wrong module file " << file << " in hmp package " << hmpName;
        return ModuleErrorCode::ERR_INSTALL_FAIL;
    }
    if (!moduleFile->GetImageStat().has_value()) {
        LOG(ERROR) << "Could not install empty module package " << moduleFile->GetVersionInfo().version;
        return ModuleErrorCode::ERR_INSTALL_FAIL;
    }
    int32_t ret = ValidateVersion(*moduleFile, hmpName);
    if (ret != ModuleErrorCode::MODULE_UPDATE_SUCCESS) {
        LOG(ERROR) << "Validate version fail: " << moduleFile->GetVersionInfo().version;
        return ret;
    }
    installModule_ = std::move(moduleFile);

    return ModuleErrorCode::MODULE_UPDATE_SUCCESS;
}

int32_t ModuleUpdateMain::UninstallModulePackage(const std::string &hmpName)
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

int32_t ModuleUpdateMain::GetModulePackageInfo(const std::string &hmpName,
    std::list<ModulePackageInfo> &modulePackageInfos)
{
    LOG(INFO) << "GetModulePackageInfo " << hmpName;
    if (hmpName.empty()) {
        for (auto &hmp : hmpSet_) {
            CollectModulePackageInfo(hmp, modulePackageInfos);
        }
    } else if (find(hmpSet_.begin(), hmpSet_.end(), hmpName) != hmpSet_.end()) {
        CollectModulePackageInfo(hmpName, modulePackageInfos);
    } else {
        LOG(ERROR) << hmpName << " not exist in hmpSet";
        return ModuleErrorCode::MODULE_UPDATE_FAIL;
    }
    return ModuleErrorCode::MODULE_UPDATE_SUCCESS;
}

void ModuleUpdateMain::CollectModulePackageInfo(const std::string &hmpName,
    std::list<ModulePackageInfo> &modulePackageInfos) const
{
    if (hmpName.empty()) {
        return;
    }
    std::string installHmpPath = std::string(UPDATE_INSTALL_DIR) + "/" + hmpName;
    std::string activeHmpPath = std::string(UPDATE_ACTIVE_DIR) + "/" + hmpName;
    if (!CheckPathExists(installHmpPath) && !CheckPathExists(activeHmpPath)) {
        LOG(ERROR) << "hmpName: " << hmpName << " not exist active and install dir.";
        return;
    }
    std::vector<std::string> files;
    GetDirFiles(installHmpPath, files);
    GetDirFiles(activeHmpPath, files);
    for (auto &file : files) {
        if (!CheckFileSuffix(file, MODULE_PACKAGE_SUFFIX)) {
            continue;
        }
        std::unique_ptr<ModuleFile> moduleFile = ModuleFile::Open(file);
        if (moduleFile == nullptr) {
            return;
        }
        modulePackageInfos.emplace_back(std::move(moduleFile->GetVersionInfo()));
    }
}

void ModuleUpdateMain::ExitModuleUpdate()
{
    LOG(INFO) << "ExitModuleUpdate";
    Stop();
}

bool ModuleUpdateMain::GetHmpVersion(const std::string &hmpPath, HmpVersionInfo &versionInfo)
{
    LOG(INFO) << "GetHmpVersion " << hmpPath;
    std::string packInfoPath = hmpPath + "/" + HMP_INFO_NAME;
    if (!Utils::IsFileExist(packInfoPath)) {
        LOG(ERROR) << "pack.info is not exist: " << packInfoPath;
        return false;
    }
    if (!StartsWith(packInfoPath, MODULE_PREINSTALL_DIR) &&
        VerifyModulePackageSign(packInfoPath) != 0) {
        LOG(ERROR) << "Verify sign failed " << packInfoPath;
        return false;
    }
    std::string packInfo = GetContentFromZip(packInfoPath, PACK_INFO_NAME);
    JsonNode root(packInfo);
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

void ModuleUpdateMain::ParseHmpVersionInfo(std::vector<HmpVersionInfo> &versionInfos, const HmpVersionInfo &preInfo,
    const HmpVersionInfo &actInfo)
{
    if (preInfo.version.size() == 0 && actInfo.version.size() == 0) {
        LOG(WARNING) << "version is empty";
        return;
    }

    if (actInfo.version.size() == 0) {
        LOG(INFO) << "add preinstaller info";
        versionInfos.emplace_back(preInfo);
        hmpWorkDirMap_.emplace(preInfo.name, MODULE_PREINSTALL_DIR);
        return;
    }
    std::vector<std::string> preVersion {};
    std::vector<std::string> actVersion {};
    // version: xxx-d01 M.S.F.B
    if (!ParseVersion(preInfo.version, " ", preVersion)) {
        LOG(ERROR) << "Parse preVersion failed.";
        return;
    }

    if (!ParseVersion(actInfo.version, " ", actVersion)) {
        LOG(WARNING) << "Parse actVersion failed.";
        versionInfos.emplace_back(preInfo);
        hmpWorkDirMap_.emplace(preInfo.name, MODULE_PREINSTALL_DIR);
        return;
    }

    if (CompareHmpVersion(preVersion, actVersion)) {
        LOG(INFO) << "add active info";
        versionInfos.emplace_back(actInfo);
        hmpWorkDirMap_.emplace(preInfo.name, UPDATE_ACTIVE_DIR);
    } else {
        LOG(INFO) << "add preinstaller info";
        versionInfos.emplace_back(preInfo);
        hmpWorkDirMap_.emplace(preInfo.name, MODULE_PREINSTALL_DIR);
    }
}

std::vector<HmpVersionInfo> ModuleUpdateMain::GetHmpVersionInfo()
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

void ModuleUpdateMain::SaveInstallerResult(const std::string &hmpPath, int result,
    const std::string &resultInfo, const Timer &timer)
{
    LOG(INFO) << "hmpPath:" << hmpPath << " result:" << result << " resultInfo:" << resultInfo;
    UniqueFd fd(open(MODULE_RESULT_PATH, O_APPEND | O_RDWR | O_CREAT | O_CLOEXEC,
        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH));
    if (fd.Get() == -1) {
        LOG(ERROR) << "Failed to open file";
        return;
    }
    std::string writeInfo = hmpPath + ";" + std::to_string(result) + ";" +
        resultInfo + "|" + std::to_string(timer.duration().count()) + "\n";
    if (CheckAndUpdateRevertResult(hmpPath, writeInfo, "revert")) {
        return;
    }
    if (write(fd, writeInfo.data(), writeInfo.length()) <= 0) {
        LOG(WARNING) << "write result file failed, err:" << errno;
    }
    fsync(fd.Get());
}

bool ModuleUpdateMain::BackupActiveModules(const std::string &hmpName) const
{
    std::string activePath = std::string(UPDATE_ACTIVE_DIR) + "/" + hmpName;
    if (!CheckPathExists(activePath)) {
        LOG(INFO) << "Nothing to backup, path: " << activePath;
        return true;
    }
    std::string backupPath = std::string(UPDATE_BACKUP_DIR) + "/" + hmpName;
    if (CheckPathExists(backupPath)) {
        if (!ForceRemoveDirectory(backupPath)) {
            LOG(ERROR) << "Failed to remove backup dir:" << backupPath;
            return false;
        }
    }
    if (!CreateDirIfNeeded(backupPath, DIR_MODE)) {
        LOG(ERROR) << "Failed to create backup dir:" << backupPath;
        return false;
    }

    std::vector<std::string> activeFiles;
    GetDirFiles(activePath, activeFiles);
    ON_SCOPE_EXIT(rmdir) {
        if (!ForceRemoveDirectory(backupPath)) {
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

void ModuleUpdateMain::ScanPreInstalledHmp()
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
        std::unique_lock<std::mutex> locker(mlock_);
        LOG(INFO) << "current hmp name: " << hmpName;
        hmpSet_.emplace(hmpName);
        for (const auto &[key, value] : moduleFile->GetVersionInfo().moduleMap) {
            moduleSet_.emplace(key);
            for (const auto &saInfo : value.saInfoList) {
                saIdHmpMap_.emplace(saInfo.saId, key);
            }
        }
    }
}

sptr<ISystemAbilityManager> &ModuleUpdateMain::GetSystemAbilityManager()
{
    if (samgr_ != nullptr) {
        return samgr_;
    }
    int32_t times = RETRY_TIMES_FOR_SAMGR;
    constexpr int32_t duration = std::chrono::microseconds(MILLISECONDS_WAITING_SAMGR_ONE_TIME).count();
    while (times > 0) {
        times--;
        samgr_ = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (samgr_ == nullptr) {
            LOG(INFO) << "waiting for samgr";
            usleep(duration);
        } else {
            break;
        }
    }
    return samgr_;
}

void ModuleUpdateMain::Start()
{
    LOG(INFO) << "ModuleUpdateMain Start";
    ModuleUpdateQueue queue;
    ModuleUpdateProducer producer(queue, saIdHmpMap_, moduleSet_, g_exit);
    ModuleUpdateConsumer consumer(queue, saIdHmpMap_, g_exit);
    std::thread produceThread([&producer] {
        producer.Run();
    });
    std::thread consumeThread([&consumer] {
        consumer.Run();
    });
    consumeThread.join();
    produceThread.join();
    LOG(INFO) << "module update main exit";
}

void ModuleUpdateMain::Stop()
{
    LOG(INFO) << "ModuleUpdateMain Stop";
    g_exit = 1;
}
} // namespace SysInstaller
} // namespace OHOS