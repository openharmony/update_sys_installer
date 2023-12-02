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

#include "directory_ex.h"
#include "log/log.h"
#include "module_constants.h"
#include "module_dm.h"
#include "module_error_code.h"
#include "module_file_repository.h"
#include "module_loop.h"
#include "module_update_kits.h"
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
constexpr int32_t RETRY_TIMES_FOR_MODULE_UPDATE_SERVICE = 10;
constexpr std::chrono::milliseconds MILLISECONDS_WAITING_MODULE_UPDATE_SERVICE(100);

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

std::unique_ptr<ModuleFile> GetLatestUpdateModulePackage(const int32_t saId)
{
    auto &instance = ModuleFileRepository::GetInstance();
    std::unique_ptr<ModuleFile> activeModuleFile = instance.GetModuleFile(UPDATE_ACTIVE_DIR, saId);
    std::unique_ptr<ModuleFile> updateModuleFile = instance.GetModuleFile(UPDATE_INSTALL_DIR, saId);
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

string GetSuccessSaIdString(const ModuleUpdateStatus &status)
{
    string ret = "";
    for (const SaStatus &saStatus : status.saStatusList) {
        if (saStatus.isMountSuccess) {
            ret += std::to_string(saStatus.saId) + " ";
        }
    }
    return ret;
}
}

bool ModuleUpdate::CheckMountComplete(string &status) const
{
    bool mountStatus = false;
    for (int32_t saId : saIdSet_) {
        string path = std::string(MODULE_ROOT_DIR) + "/" + std::to_string(saId);
        if (CheckPathExists(path)) {
            mountStatus = true;
            status += std::to_string(saId) + " ";
        }
    }
    return mountStatus;
}

string ModuleUpdate::CheckModuleUpdate(const string &path)
{
    LOG(INFO) << "CheckModuleUpdate path=" << path;
    Timer timer;
    string ret = "";
    if (!ParseSaProfiles(path)) {
        LOG(ERROR) << "Failed to parse sa profile";
        return ret;
    }
    if (CheckMountComplete(ret)) {
        LOG(INFO) << "CheckMountComplete ret=" << ret;
        return ret;
    }

    ModuleFileRepository::GetInstance().InitRepository(saIdSet_);
    ON_SCOPE_EXIT(clear) {
        ModuleFileRepository::GetInstance().Clear();
    };
    PrepareModuleFileList();
    if (moduleFileList_.empty()) {
        LOG(INFO) << "No module needs to activate";
        return ret;
    }
    if (!Loop::PreAllocateLoopDevices(moduleFileList_.size())) {
        LOG(ERROR) << "Failed to pre allocate loop devices";
        return ret;
    }
    if (!ActivateModules()) {
        LOG(ERROR) << "Failed to activate modules";
        return ret;
    }
    ret = GetSuccessSaIdString(status_);
    LOG(INFO) << "CheckModuleUpdate done, duration=" << timer << ", ret=" << ret;
    return ret;
}

bool ModuleUpdate::ParseSaProfiles(const string &path)
{
    string realProfilePath = "";
    if (!PathToRealPath(path, realProfilePath)) {
        LOG(ERROR) << "Failed to get real path " << path;
        return false;
    }
    ParseUtil parser;
    if (!parser.ParseSaProfiles(realProfilePath)) {
        LOG(ERROR) << "ParseSaProfiles failed! path=" << realProfilePath;
        return false;
    }
    status_.process = Str16ToStr8(parser.GetProcessName());
    auto saInfos = parser.GetAllSaProfiles();
    for (const auto &saInfo : saInfos) {
        saIdSet_.insert(saInfo.saId);
    }
    return true;
}

void ModuleUpdate::PrepareModuleFileList()
{
    for (int32_t saId : saIdSet_) {
        auto &instance = ModuleFileRepository::GetInstance();
        std::unique_ptr<ModuleFile> systemModuleFile = instance.GetModuleFile(MODULE_PREINSTALL_DIR, saId);
        if (systemModuleFile == nullptr) {
            LOG(ERROR) << "Failed to get preinstalled sa " << saId;
            continue;
        }
        std::unique_ptr<ModuleFile> latestModuleFile = GetLatestUpdateModulePackage(saId);
        if (latestModuleFile != nullptr && ModuleFile::CompareVersion(*latestModuleFile, *systemModuleFile)) {
            moduleFileList_.emplace_back(std::move(*latestModuleFile));
        } else {
            moduleFileList_.emplace_back(std::move(*systemModuleFile));
        }
    }
}

bool ModuleUpdate::ActivateModules()
{
    bool activateSuccess = true;
    for (const auto &moduleFile : moduleFileList_) {
        if (!moduleFile.GetImageStat().has_value()) {
            LOG(INFO) << moduleFile.GetPath() << " is empty module package";
            continue;
        }
        SaStatus saStatus;
        saStatus.saId = moduleFile.GetSaId();
        saStatus.isPreInstalled = ModuleFileRepository::GetInstance().IsPreInstalledModule(moduleFile);
        saStatus.isMountSuccess = MountModulePackage(moduleFile, !saStatus.isPreInstalled);
        if (!saStatus.isMountSuccess) {
            LOG(ERROR) << "Failed to mount module package " << moduleFile.GetPath();
            activateSuccess = false;
            ModuleFileRepository::GetInstance().SaveInstallerResult(moduleFile.GetPath(),
                GetHmpName(moduleFile.GetPath()), ERR_INSTALL_FAIL, "mount fail");
        }
        status_.saStatusList.emplace_back(std::move(saStatus));
    }
    LOG(INFO) << "ActivateModules activateSuccess:" << activateSuccess;
    ReportMountStatus(status_);
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
    if (!IsEmptyFolder(mountPoint)) {
        LOG(ERROR) << mountPoint << " is not empty";
        return false;
    }
    const string &fullPath = moduleFile.GetPath();
    if (!moduleFile.GetImageStat().has_value()) {
        LOG(ERROR) << "Could not mount empty module package " << moduleFile.GetPath();
        return false;
    }
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
            return false;
        }
    }
    WaitDevice(blockDevice);
    uint32_t mountFlags = MS_NOATIME | MS_NODEV | MS_DIRSYNC | MS_RDONLY;
    ret = mount(blockDevice.c_str(), mountPoint.c_str(), imageStat.fsType, mountFlags, nullptr);
    if (ret != 0) {
        LOG(ERROR) << "Mounting failed for module package " << fullPath << " errno:" << errno;
        return false;
    }
    LOG(INFO) << "Successfully mounted module package " << fullPath << " on " << mountPoint << " duration=" << timer;
    loopbackDevice.CloseGood();
    CANCEL_SCOPE_EXIT_GUARD(rmDir);
    return true;
}

void ModuleUpdate::ReportMountStatus(const ModuleUpdateStatus &status) const
{
    int32_t times = RETRY_TIMES_FOR_MODULE_UPDATE_SERVICE;
    constexpr int32_t duration = std::chrono::microseconds(MILLISECONDS_WAITING_MODULE_UPDATE_SERVICE).count();
    while (times > 0) {
        times--;
        int32_t ret = ModuleUpdateKits::GetInstance().ReportModuleUpdateStatus(status);
        if (ret == ModuleErrorCode::ERR_SERVICE_NOT_FOUND) {
            LOG(INFO) << "retry to report mount failed";
            usleep(duration);
        } else {
            if (ret != ModuleErrorCode::MODULE_UPDATE_SUCCESS) {
                LOG(ERROR) << "Failed to report mount failed";
            }
            return;
        }
    }
    LOG(ERROR) << "Report mount failed timeout";
}
} // namespace SysInstaller
} // namespace OHOS
