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

#include "module_update.h"

#include <chrono>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>

#include "directory_ex.h"
#include "log/log.h"
#include "module_constants.h"
#include "module_dm.h"
#include "module_error_code.h"
#include "module_file_repository.h"
#include "module_loop.h"
#include "module_update_task.h"
#include "module_utils.h"
#include "scope_guard.h"
#include "string_ex.h"
#include "utils.h"

#ifdef SUPPORT_HVB
#include "fs_hvb.h"
#endif

namespace OHOS {
namespace SysInstaller {
using namespace Updater;
using std::string;

namespace {
constexpr mode_t MOUNT_POINT_MODE = 0755;
constexpr int32_t LOOP_DEVICE_SETUP_ATTEMPTS = 3;

bool CreateLoopDevice(const string &path, const ImageStat &imageStat, Loop::LoopbackDeviceUniqueFd &loopbackDevice)
{
    for (int32_t attempts = 1; attempts <= LOOP_DEVICE_SETUP_ATTEMPTS; ++attempts) {
        std::unique_ptr<Loop::LoopbackDeviceUniqueFd> device =
            Loop::CreateLoopDevice(path, imageStat.imageOffset, imageStat.imageSize);
        if (device != nullptr) {
            loopbackDevice = std::move(*device);
            break;
        }
    }
    return loopbackDevice.deviceFd.Get() != -1;
}

bool StageUpdateModulePackage(const string &updatePath, const string &stagePath)
{
    int ret = 0;
    if (CheckPathExists(stagePath)) {
        LOG(INFO) << stagePath << " already exists. Deleting";
        ret = unlink(stagePath.c_str());
        if (ret != 0) {
            LOG(ERROR) << "Failed to unlink " << stagePath;
            return false;
        }
    }
    string path = ExtractFilePath(stagePath);
    // active dir should create by module_update_sa
    if (!CheckPathExists(path)) {
        LOG(ERROR) << "active module path doesn't exist.";
        return false;
    }
    ret = link(updatePath.c_str(), stagePath.c_str());
    if (ret != 0) {
        LOG(ERROR) << "Unable to link " << updatePath << " to " << stagePath;
        return false;
    }

    // stage other files
    std::vector<std::string> files;
    if (Updater::Utils::GetFilesFromDirectory(updatePath.substr(0, updatePath.rfind("/")), files, true) <= 0) {
        LOG(ERROR) << "Failed to get files form " << updatePath;
        return false;
    }
    for (const auto &file : files) {
        std::string targetFile = path + ExtractFileName(file);
        (void)unlink(targetFile.c_str());
        ret = link(file.c_str(), targetFile.c_str());
        if (ret != 0) {
            LOG(ERROR) << "Unable to link " << file << " to " << targetFile;
            return false;
        }
    }
    LOG(INFO) << "success to link " << updatePath << " to " << stagePath;
    return true;
}

bool CheckModulePackage(const std::string &mountPoint, const ModuleFile &moduleFile)
{
    if (!IsEmptyFolder(mountPoint)) {
        LOG(ERROR) << mountPoint << " is not empty";
        return false;
    }
    if (!moduleFile.GetImageStat().has_value()) {
        LOG(ERROR) << "Could not mount empty module package " << moduleFile.GetPath();
        return false;
    }
    return true;
}

bool VerifyAndCreateDm(ModuleFile &moduleFile, string &blockDevice)
{
    LOG(ERROR) << "Verify and create dm.";
    if (!moduleFile.VerifyModuleVerity()) {
        LOG(ERROR) << "verify image failed of " << moduleFile.GetPath();
        return false;
    }
    if (!CreateDmDevice(moduleFile, blockDevice)) {
        LOG(ERROR) << "Could not create dm-verity device on " << blockDevice;
        Loop::ClearDmLoopDevice(blockDevice, false);
        return false;
    }
    return true;
}
}

ModuleUpdate &ModuleUpdate::GetInstance()
{
    static ModuleUpdate instance;
    return instance;
}

std::unique_ptr<ModuleFile> ModuleUpdate::GetLatestUpdateModulePackage(const string &hmpName)
{
    std::unique_ptr<ModuleFile> activeModuleFile = repository_.GetModuleFile(UPDATE_ACTIVE_DIR, hmpName);
    std::unique_ptr<ModuleFile> updateModuleFile = repository_.GetModuleFile(UPDATE_INSTALL_DIR, hmpName);
    std::unique_ptr<ModuleFile> ret = nullptr;
    if (updateModuleFile != nullptr) {
        if (activeModuleFile == nullptr || ModuleFile::CompareVersion(*updateModuleFile, *activeModuleFile)) {
            string updatePath = updateModuleFile->GetPath();
            string activePath = UPDATE_ACTIVE_DIR +
                updatePath.substr(strlen(UPDATE_INSTALL_DIR), updatePath.length());
            if (!StageUpdateModulePackage(updatePath, activePath)) {
                return ret;
            }
            updateModuleFile->SetPath(activePath);
            ret = std::move(updateModuleFile);
            LOG(INFO) << "add updateModuleFile " << updatePath;
        }
    }
    if (ret == nullptr && activeModuleFile != nullptr) {
        LOG(INFO) << "add activeModuleFile " << activeModuleFile->GetPath();
        ret = std::move(activeModuleFile);
    }
    return ret;
}

bool ModuleUpdate::CheckMountComplete(const string &hmpName) const
{
    string path = std::string(MODULE_ROOT_DIR) + "/" + hmpName;
    return CheckPathExists(path);
}

void ModuleUpdate::ProcessHmpFile(const string &hmpFile, const ModuleUpdateStatus &status, const Timer &timer)
{
    LOG(INFO) << "process hmp file=" << hmpFile;
    std::unique_ptr<ModuleFile> moduleFile = ModuleFile::Open(hmpFile);
    if (moduleFile == nullptr) {
        LOG(ERROR) << "Process hmp file fail, module file is null";
        return;
    }
    if (CheckMountComplete(status.hmpName)) {
        LOG(INFO) << "Check mount complete, hmpName=" << status.hmpName;
        return;
    }
    repository_.InitRepository(status.hmpName, timer);
    PrepareModuleFileList(status);
}

bool ModuleUpdate::DoModuleUpdate(ModuleUpdateStatus &status)
{
    LOG(INFO) << "enter domoduleupdate";
    Timer timer;
    std::string hmpPackagePath = std::string(MODULE_PREINSTALL_DIR) + "/" + status.hmpName;
    LOG(INFO) << "DoModuleUpdate hmp package path=" << hmpPackagePath;
    std::vector<std::string> files;
    GetDirFiles(hmpPackagePath, files);
    ON_SCOPE_EXIT(clear) {
        repository_.Clear();
        moduleFileList_.clear();
    };
    for (auto &file : files) {
        std::string hmpPackage = GetFileName(file);
        if (!CheckFileSuffix(file, MODULE_PACKAGE_SUFFIX) || hmpPackage.empty()) {
            continue;
        }
        ProcessHmpFile(file, status, timer);
    }
    if (moduleFileList_.size() != 1) {
        LOG(INFO) << status.hmpName << " module size is invalid: " << moduleFileList_.size();
        return false;
    }
    if (!Loop::PreAllocateLoopDevices(moduleFileList_.size())) {
        LOG(ERROR) << "Failed to pre allocate loop devices, hmp package name=" << status.hmpName;
        return false;
    }
    if (!ActivateModules(status, timer)) {
        LOG(ERROR) << "Failed to activate modules, hmp package name=" << status.hmpName;
        return false;
    }
    LOG(INFO) << "Success to activate modules, hmp package name=" << status.hmpName;
    return true;
}

void ModuleUpdate::CheckModuleUpdate()
{
    InitUpdaterLogger("CheckModuleUpdate", MODULE_UPDATE_LOG_FILE, "", "");
    LOG(INFO) << "CheckModuleUpdate begin";
    Timer timer;
    std::vector<std::string> files;
    std::unordered_set<std::string> hmpNameSet;
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
        hmpNameSet.emplace(hmpName);
    }
    auto &instance = ModuleUpdateTaskManager::GetInstance();
    instance.Start();
    ON_SCOPE_EXIT(clear) {
        instance.ClearTask();
        instance.Stop();
        LOG(INFO) << "CheckModuleUpdate done, duration=" << timer;
        if (!ForceRemoveDirectory(UPDATE_INSTALL_DIR)) {
            LOG(ERROR) << "Failed to remove " << UPDATE_INSTALL_DIR << " err=" << errno;
        }
    };
    for (auto &hmpName : hmpNameSet) {
        instance.AddTask(hmpName);
    }
    while (instance.GetCurTaskNum() != 0) {
        usleep(3000); // 3000: 3ms
    }
}

bool ModuleUpdate::CheckRevert(const std::string &hmpName)
{
    if (!CheckPathExists(std::string(UPDATE_BACKUP_DIR) + "/" + hmpName)) {
        return false;
    }
    auto &moduleMap = repository_.GetModuleMap();
    for (const auto &[key, value] : moduleMap) {
        if (key != hmpName) {
            continue;
        }
        // only system hmp
        if (value.size() == 1) {
            LOG(ERROR) << "active dir destroyed, but backup dir exists, try to revert.";
            return true;
        }
    }
    return false;
}

void ModuleUpdate::PrepareModuleFileList(const ModuleUpdateStatus &status)
{
    std::unique_ptr<ModuleFile> systemModuleFile = repository_.GetModuleFile(MODULE_PREINSTALL_DIR, status.hmpName);
    if (systemModuleFile == nullptr) {
        LOG(ERROR) << "Failed to get preinstalled hmp " << status.hmpName;
        return;
    }
    std::unique_ptr<ModuleFile> latestModuleFile = GetLatestUpdateModulePackage(status.hmpName);
    if (latestModuleFile != nullptr && ModuleFile::CompareVersion(*latestModuleFile, *systemModuleFile)) {
        moduleFileList_.emplace_back(std::move(*latestModuleFile));
    } else {
        moduleFileList_.emplace_back(std::move(*systemModuleFile));
        if (CheckRevert(status.hmpName)) {
            LOG(ERROR) << "some error happened, revert.";
            NotifyBmsRevert(status.hmpName, true);
            Revert(status.hmpName, true);
            return;
        }
        // when choose preInstall hmp, remove activeHmp and backupHmp
        RemoveSpecifiedDir(std::string(UPDATE_ACTIVE_DIR) + "/" + status.hmpName, false);
        RemoveSpecifiedDir(std::string(UPDATE_BACKUP_DIR) + "/" + status.hmpName, false);
    }
}

bool ModuleUpdate::ActivateModules(ModuleUpdateStatus &status, const Timer &timer)
{
    // size = 1
    for (auto &moduleFile : moduleFileList_) {
        if (!moduleFile.GetImageStat().has_value()) {
            LOG(INFO) << moduleFile.GetPath() << " is empty module package";
            continue;
        }
        status.isPreInstalled = repository_.IsPreInstalledModule(moduleFile);
        status.isAllMountSuccess = MountModulePackage(moduleFile, !status.isPreInstalled);
        if (!status.isAllMountSuccess) {
            LOG(ERROR) << "Failed to mount module package " << moduleFile.GetPath();
            repository_.SaveInstallerResult(moduleFile.GetPath(), status.hmpName,
                ERR_INSTALL_FAIL, "mount fail", timer);
        }
        // bugfix: when sise = 1, for() find the second item
        break;
    }
    ReportModuleUpdateStatus(status);
    LOG(INFO) << "ActivateModule mount result:" << status.isAllMountSuccess << ", hmp package name:" << status.hmpName;
    return status.isAllMountSuccess;
}

void ModuleUpdate::WaitDevice(const std::string &blockDevice) const
{
    const int waitTime = 150; // wait max 3s
    int time = 0;
    while (!CheckPathExists(blockDevice) && time++ < waitTime) {
        usleep(20000); // 20000: 20ms
    }
}

bool ModuleUpdate::VerifyImageAndCreateDm(ModuleFile &moduleFile, bool mountOnVerity, string &blockDevice)
{
    if (!mountOnVerity) {
        LOG(INFO) << "Current hmp path is preInstalled, do not check.";
        return true;
    }
    if (ImageVerifyFunc_ != nullptr) {
        return ImageVerifyFunc_(moduleFile, blockDevice);
    }
    LOG(ERROR) << "ImageVerifyFunc_ is nullptr, error.";
    return false;
}

string ModuleUpdate::CreateMountPoint(const ModuleFile &moduleFile) const
{
    string mountPoint = string(MODULE_ROOT_DIR);
    if (moduleFile.GetVersionInfo().type != HMP_TRAIN_TYPE) {
        mountPoint = string(MODULE_ROOT_DIR) + "/" + moduleFile.GetVersionInfo().hmpName;
    }
    LOG(INFO) << "Creating mount point: " << mountPoint;
    if (!CreateDirIfNeeded(mountPoint, MOUNT_POINT_MODE)) {
        LOG(ERROR) << "Could not create mount point " << mountPoint << " errno: " << errno;
        return "";
    }
    return mountPoint;
}

bool ModuleUpdate::MountModulePackage(ModuleFile &moduleFile, const bool mountOnVerity)
{
    string mountPoint = CreateMountPoint(moduleFile);
    if (mountPoint.empty()) {
        return false;
    }
    Timer timer;
    int ret = 0;
    ON_SCOPE_EXIT(rmDir) {
        ret = rmdir(mountPoint.c_str());
        if (ret != 0) {
            LOG(WARNING) << "Could not rmdir " << mountPoint << " errno: " << errno;
        }
    };
    if (!CheckModulePackage(mountPoint, moduleFile)) {
        return false;
    }
    const string &fpInfo = ExtractFilePath(moduleFile.GetPath()) + IMG_FILE_NAME;
    const ImageStat &imageStat = moduleFile.GetImageStat().value();
    Loop::LoopbackDeviceUniqueFd loopbackDevice;
    if (!CreateLoopDevice(fpInfo, imageStat, loopbackDevice)) {
        LOG(ERROR) << "Could not create loop device for " << fpInfo;
        return false;
    }
    LOG(INFO) << "Loopback device created: " << loopbackDevice.name << " fsType=" << imageStat.fsType;
    string blockDevice = loopbackDevice.name;
    if (!VerifyImageAndCreateDm(moduleFile, mountOnVerity, blockDevice)) {
        return false;
    }
    WaitDevice(blockDevice);
    uint32_t mountFlags = MS_NOATIME | MS_NODEV | MS_DIRSYNC | MS_RDONLY;
    ret = mount(blockDevice.c_str(), mountPoint.c_str(), imageStat.fsType, mountFlags, nullptr);
    if (ret != 0) {
        LOG(ERROR) << "Mounting failed for module package " << fpInfo << " errno:" << errno;
        Loop::ClearDmLoopDevice(blockDevice, true);
        return false;
    }
    LOG(INFO) << "Successfully mounted module package " << fpInfo << " on " << mountPoint << " duration=" << timer;
    SetModuleVersion(moduleFile);
    loopbackDevice.CloseGood();
    CANCEL_SCOPE_EXIT_GUARD(rmDir);
    return true;
}

void ModuleUpdate::ReportModuleUpdateStatus(const ModuleUpdateStatus &status) const
{
    auto &instance = ModuleUpdateTaskManager::GetInstance();
    if (!instance.GetTaskResult()) {
        LOG(ERROR) << "ReportModuleUpdateStatus, module update fail";
        instance.ClearTask();
    }
    if (!status.isAllMountSuccess) {
        LOG(ERROR) << "ReportModuleUpdateStatus mount fail, hmp name=" << status.hmpName;
        RemoveSpecifiedDir(std::string(UPDATE_INSTALL_DIR) + "/" + status.hmpName, false);
        NotifyBmsRevert(status.hmpName, true);
        Revert(status.hmpName, true);
        return;
    }
    LOG(INFO) << "ReportModuleUpdateStatus mount success, hmp name=" << status.hmpName;
}

void ModuleUpdate::RegisterImageVerifyFunc(ImageVerifyFunc ptr, int32_t level)
{
    if (level <= registeredLevel_) {
        LOG(WARNING) << "register level is smaller, " << level;
        return;
    }
    registeredLevel_ = level;
    ImageVerifyFunc_ = std::move(ptr);
}

extern "C" __attribute__((constructor)) void RegisterImgVerifyFunc(void)
{
    ModuleUpdate::GetInstance().RegisterImageVerifyFunc(VerifyAndCreateDm, REGISTER_LEVEL_ONE);
}

void ModuleUpdate::SetParameterFromFile(void) const
{
    const std::chrono::milliseconds waitTime(100); // 100ms: wait for file create
    if (!WaitForFile(MODULE_UPDATE_PARAMS_FILE, waitTime)) {
        LOG(ERROR) << "paramsFile is not ready.";
        return;
    }
    std::ifstream fin {MODULE_UPDATE_PARAMS_FILE};
    if (!fin.is_open()) {
        LOG(ERROR) << "read params file fail " << MODULE_UPDATE_PARAMS_FILE;
        return;
    }
    std::string line;
    while (std::getline(fin, line)) {
        size_t pos = line.find("=");
        if (pos == std::string::npos) {
            LOG(ERROR) << "read params error: " << line;
            continue;
        }
        std::string paramName = line.substr(0, pos);
        std::string paramValue = line.substr(pos + 1);
        if (Utils::SetParameter(paramName.c_str(), paramValue.c_str()) != 0) {
            LOG(WARNING) << "Failed to set module params: " << paramName << "; value is " << paramValue;
        }
    }
    if (unlink(MODULE_UPDATE_PARAMS_FILE) != 0) {
        LOG(WARNING) << "Failed to unlink params file, error is " << strerror(errno);
    }
}

void ModuleUpdate::HandleExtraArgs(int argc, char **argv) const
{
    InitUpdaterLogger("CheckModuleUpdate", MODULE_UPDATE_LOG_FILE, "", "");
    if (argc == 0 || argv == nullptr) {
        LOG(ERROR) << "argc is 0 or argv is nullptr.";
        return;
    }
    const std::unordered_map<std::string, std::function<void(void)>> handleFuncMap = {
        {"setParam", [] () { return ModuleUpdate::GetInstance().SetParameterFromFile(); }},
    };
    for (int i = 1; i < argc; i++) {
        LOG(INFO) << "i = " << i << "; argv[i] = " << argv[i];
        auto iter = handleFuncMap.find(argv[i]);
        if (iter != handleFuncMap.end()) {
            iter->second();
        }
    }
}
} // namespace SysInstaller
} // namespace OHOS
