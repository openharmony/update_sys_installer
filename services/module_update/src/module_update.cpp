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
#include "parse_util.h"
#include "scope_guard.h"
#include "string_ex.h"
#include "utils.h"

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
    if (!CheckPathExists(path)) {
        LOG(ERROR) << path << " doesn't exist.";
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

bool RemoveMountPoint(const std::string &mountPoint, const std::string &saFile, ModuleUpdateStatus &status)
{
    std::string saName = GetFileName(saFile);
    std::string prefixes[] = {MODULE_PREINSTALL_DIR, UPDATE_ACTIVE_DIR};
    int ret = -1;
    for (auto prefix : prefixes) {
        std::string imagePath = prefix + "/" + status.hmpName + "/" + saName + string(MODULE_PACKAGE_SUFFIX);
        if (Loop::RemoveDmLoopDevice(mountPoint, imagePath)) {
            ret = 0;
            LOG(INFO) << "Successful remove dm loop device, mountPoint=" << mountPoint
                << ", imagePath=" << imagePath;
            break;
        }
    }
    if (ret != 0) {
        LOG(ERROR) << "Fail remove dm loop device, mountPoint=" << mountPoint;
        return false;
    }
    ret = rmdir(mountPoint.c_str());
    if (ret != 0) {
        LOG(WARNING) << "Could not rmdir " << mountPoint << " errno: " << errno;
        return false;
    }
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
}

ModuleUpdate &ModuleUpdate::GetInstance()
{
    static ModuleUpdate instance;
    return instance;
}

std::unique_ptr<ModuleFile> ModuleUpdate::GetLatestUpdateModulePackage(const int32_t saId)
{
    std::unique_ptr<ModuleFile> activeModuleFile = repository_.GetModuleFile(UPDATE_ACTIVE_DIR, saId);
    std::unique_ptr<ModuleFile> updateModuleFile = repository_.GetModuleFile(UPDATE_INSTALL_DIR, saId);
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

bool ModuleUpdate::CheckMountComplete(int32_t saId) const
{
    string path = std::string(MODULE_ROOT_DIR) + "/" + std::to_string(saId);
    return CheckPathExists(path);
}

void ModuleUpdate::ProcessSaFile(const std::string &saFile, ModuleUpdateStatus &status)
{
    LOG(INFO) << "process sa file=" << saFile;
    std::unique_ptr<ModuleFile> moduleFile = ModuleFile::Open(saFile);
    if (moduleFile == nullptr) {
        LOG(ERROR) << "Process sa file fail, module file is null";
        return;
    }
    int32_t saId = moduleFile->GetSaId();
    if (IsHotHmpPackage(status.hmpName) && CheckBootComplete()) {
        if (IsRunning(saId)) {
            LOG(INFO) << "ondemand sa is running, saId=" << saId;
            return;
        }
        std::string mountPoint = string(MODULE_ROOT_DIR) + "/" + std::to_string(saId);
        LOG(INFO) << "Remove old mountpoint " << mountPoint;
        if (CheckPathExists(mountPoint)) {
            if (!RemoveMountPoint(mountPoint, saFile, status)) {
                return;
            }
            LOG(INFO) << "Remove mount point success, mountpoint=" << mountPoint;
        }
    } else if (CheckMountComplete(saId)) {
        LOG(INFO) << "Check mount complete, saId=" << saId;
        return;
    }
    repository_.InitRepository(saId);
    PrepareModuleFileList(saId, status);
}

bool ModuleUpdate::DoModuleUpdate(ModuleUpdateStatus &status)
{
    LOG(INFO) << "enter domoduleupdate";
    std::string hmpPackagePath = std::string(MODULE_PREINSTALL_DIR) + "/" + status.hmpName;
    LOG(INFO) << "DoModuleUpdate hmp package path=" << hmpPackagePath;
    std::vector<std::string> files;
    GetDirFiles(hmpPackagePath, files);
    ON_SCOPE_EXIT(clear) {
        repository_.Clear();
        moduleFileList_.clear();
    };
    for (auto &file : files) {
        std::string saPackage = GetFileName(file);
        if (!CheckFileSuffix(file, MODULE_PACKAGE_SUFFIX) || saPackage.empty()) {
            continue;
        }
        ProcessSaFile(file, status);
    }
    if (moduleFileList_.empty()) {
        LOG(INFO) << "No module needs to activate, hmp package name=" << status.hmpName;
        return false;
    }
    if (!Loop::PreAllocateLoopDevices(moduleFileList_.size())) {
        LOG(ERROR) << "Failed to pre allocate loop devices, hmp package name=" << status.hmpName;
        return false;
    }
    if (!ActivateModules(status)) {
        LOG(ERROR) << "Failed to activate modules, hmp package name=" << status.hmpName;
        return false;
    }
    LOG(INFO) << "Success to activate modules, hmp package name=" << status.hmpName;
    return true;
}

void ModuleUpdate::CheckModuleUpdate()
{
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
        sleep(1);
    }
}

void ModuleUpdate::PrepareModuleFileList(int32_t saId, ModuleUpdateStatus &status)
{
    std::unique_ptr<ModuleFile> systemModuleFile = repository_.GetModuleFile(MODULE_PREINSTALL_DIR, saId);
    if (systemModuleFile == nullptr) {
        LOG(ERROR) << "Failed to get preinstalled sa " << saId;
        return;
    }
    std::unique_ptr<ModuleFile> latestModuleFile = GetLatestUpdateModulePackage(saId);
    if (latestModuleFile != nullptr && ModuleFile::CompareVersion(*latestModuleFile, *systemModuleFile)) {
        moduleFileList_.emplace_back(std::move(*latestModuleFile));
    } else {
        moduleFileList_.emplace_back(std::move(*systemModuleFile));
    }
}

bool ModuleUpdate::ActivateModules(ModuleUpdateStatus &status)
{
    bool activateSuccess = true;
    for (const auto &moduleFile : moduleFileList_) {
        if (!moduleFile.GetImageStat().has_value()) {
            LOG(INFO) << moduleFile.GetPath() << " is empty module package";
            continue;
        }
        SaStatus saStatus;
        saStatus.saId = moduleFile.GetSaId();
        saStatus.isPreInstalled = repository_.IsPreInstalledModule(moduleFile);
        saStatus.isMountSuccess = MountModulePackage(moduleFile, !saStatus.isPreInstalled);
        if (!saStatus.isMountSuccess) {
            LOG(ERROR) << "Failed to mount module package " << moduleFile.GetPath();
            activateSuccess = false;
            repository_.SaveInstallerResult(moduleFile.GetPath(),
                GetHmpName(moduleFile.GetPath()), ERR_INSTALL_FAIL, "mount fail");
        }
        status.saStatusList.emplace_back(std::move(saStatus));
    }
    status.isAllMountSuccess = activateSuccess;
    ReportModuleUpdateStatus(status);
    LOG(INFO) << "ActivateModules activateSuccess:" << activateSuccess << ", hmp package name:" << status.hmpName;
    return activateSuccess;
}

void ModuleUpdate::WaitDevice(const std::string &blockDevice) const
{
    const int waitTime = 150; // wait max 3s
    int time = 0;
    while (!CheckPathExists(blockDevice) && time++ < waitTime) {
        usleep(20000); // 20000: 20ms
    }
}

bool ModuleUpdate::MountModulePackage(const ModuleFile &moduleFile, const bool mountOnVerity) const
{
    string mountPoint = string(MODULE_ROOT_DIR) + "/" + std::to_string(moduleFile.GetSaId());
    LOG(INFO) << "Creating mount point: " << mountPoint;
    Timer timer;
    int ret = 0;
    if (!CreateDirIfNeeded(mountPoint, MOUNT_POINT_MODE)) {
        LOG(ERROR) << "Could not create mount point " << mountPoint << " errno: " << errno;
        return false;
    }
    ON_SCOPE_EXIT(rmDir) {
        ret = rmdir(mountPoint.c_str());
        if (ret != 0) {
            LOG(WARNING) << "Could not rmdir " << mountPoint << " errno: " << errno;
        }
    };
    if (!CheckModulePackage(mountPoint, moduleFile)) {
        return false;
    }
    const string &fullPath = moduleFile.GetPath();
    const ImageStat &imageStat = moduleFile.GetImageStat().value();
    Loop::LoopbackDeviceUniqueFd loopbackDevice;
    if (!CreateLoopDevice(fullPath, imageStat, loopbackDevice)) {
        LOG(ERROR) << "Could not create loop device for " << fullPath;
        return false;
    }
    LOG(INFO) << "Loopback device created: " << loopbackDevice.name << " fsType=" << imageStat.fsType;
    string blockDevice = loopbackDevice.name;
    if (mountOnVerity) {
        if (!CreateDmDevice(moduleFile, blockDevice)) {
            LOG(ERROR) << "Could not create dm-verity device on " << blockDevice;
            Loop::ClearDmLoopDevice(blockDevice, false);
            return false;
        }
    }
    WaitDevice(blockDevice);
    uint32_t mountFlags = MS_NOATIME | MS_NODEV | MS_DIRSYNC | MS_RDONLY;
    ret = mount(blockDevice.c_str(), mountPoint.c_str(), imageStat.fsType, mountFlags, nullptr);
    if (ret != 0) {
        LOG(ERROR) << "Mounting failed for module package " << fullPath << " errno:" << errno;
        Loop::ClearDmLoopDevice(blockDevice, true);
        return false;
    }
    LOG(INFO) << "Successfully mounted module package " << fullPath << " on " << mountPoint << " duration=" << timer;
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
        ClearModuleDirs(status.hmpName);
        Revert(status.hmpName, !status.isHotInstall);
        return;
    }
    LOG(INFO) << "ReportModuleUpdateStatus mount success, hmp name=" << status.hmpName;
}
} // namespace SysInstaller
} // namespace OHOS
