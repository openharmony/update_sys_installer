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

#include "module_utils.h"
#include <cerrno>
#include <cstdio>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <fcntl.h>
#include "init_reboot.h"
#include "if_system_ability_manager.h"
#include "iremote_object.h"
#include "iservice_registry.h"
#include "directory_ex.h"
#include "log/log.h"
#include "parameter.h"
#include "parameters.h"
#include "singleton.h"
#include "utils.h"
#include "module_constants.h"
#include "module_file.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

namespace {
constexpr const char *BOOT_COMPLETE_PARAM = "bootevent.boot.completed";
constexpr const char *BOOT_SUCCESS_VALUE = "true";
constexpr int32_t PARAM_VALUE_SIZE = 10;
constexpr std::chrono::milliseconds WAIT_FOR_FILE_TIME(5);
constexpr uint32_t BYTE_SIZE = 8;
constexpr mode_t ALL_PERMISSIONS = 0777;
constexpr const char *PREFIXES[] = {UPDATE_INSTALL_DIR, UPDATE_ACTIVE_DIR, UPDATE_BACKUP_DIR, MODULE_PREINSTALL_DIR};
}

bool CreateDirIfNeeded(const std::string &path, mode_t mode)
{
    struct stat statData;

    if (stat(path.c_str(), &statData) != 0) {
        if (errno == ENOENT) {
            if (mkdir(path.c_str(), mode) != 0) {
                LOG(ERROR) << "Could not mkdir " << path;
                return false;
            }
        } else {
            LOG(ERROR) << "Could not stat " << path;
            return false;
        }
    } else {
        if (!S_ISDIR(statData.st_mode)) {
            LOG(ERROR) << path << " exists and is not a directory";
            return false;
        }
    }

    // Need to manually call chmod because mkdir will create a folder with
    // permissions mode & ~umask.
    if (chmod(path.c_str(), mode) != 0) {
        LOG(WARNING) << "Could not chmod " << path;
    }
    return true;
}

bool CheckPathExists(const std::string &path)
{
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0;
}

bool CheckFileSuffix(const std::string &file, const std::string &suffix)
{
    std::size_t pos = file.find_last_of('.');
    if (pos == std::string::npos) {
        LOG(ERROR) << "Invalid file name " << file;
        return false;
    }
    std::string fileSuffix = file.substr(pos);
    return fileSuffix == suffix;
}

std::string GetFileName(const std::string &file)
{
    std::size_t startPos = file.find_last_of('/') + 1;
    std::size_t endPos = file.find_last_of('.');
    return file.substr(startPos, endPos - startPos);
}

// Get hmpName from path such as "/data/module_update_package/hmpName/sa1.zip"
std::string GetHmpName(const std::string &filePath)
{
    std::size_t endPos = filePath.find_last_of('/');
    if (endPos == std::string::npos) {
        LOG(ERROR) << "Invalid package path " << filePath;
        return "";
    }

    std::size_t startPos = 0;
    for (auto &iter : PREFIXES) {
        if (StartsWith(filePath, iter)) {
            startPos = strlen(iter) + 1;
            break;
        }
    }
    if (startPos == 0 || startPos >= endPos) {
        LOG(ERROR) << "Invalid package path " << filePath;
        return "";
    }
    return filePath.substr(startPos, endPos - startPos);
}

bool WaitForFile(const std::string &path, const std::chrono::nanoseconds &timeout)
{
    Timer timer;
    bool hasSlept = false;
    while (timer.duration() < timeout) {
        struct stat buffer;
        if (stat(path.c_str(), &buffer) != -1) {
            if (hasSlept) {
                LOG(INFO) << "wait for '" << path << "' took " << timer;
            }
            return true;
        }
        std::this_thread::sleep_for(WAIT_FOR_FILE_TIME);
        hasSlept = true;
    }
    LOG(ERROR) << "wait for '" << path << "' timed out and took " << timer;
    return false;
}

bool StartsWith(const std::string &str, const std::string &prefix)
{
    return str.substr(0, prefix.size()) == prefix;
}

bool ReadFullyAtOffset(int fd, uint8_t *data, size_t count, off_t offset)
{
    while (count > 0) {
        ssize_t readSize = pread(fd, data, count, offset);
        if (readSize <= 0) {
            return false;
        }
        data += readSize;
        count -= static_cast<size_t>(readSize);
        offset += readSize;
    }
    return true;
}

uint16_t ReadLE16(const uint8_t *buff)
{
    if (buff == nullptr) {
        LOG(ERROR) << "buff is null";
        return 0;
    }
    uint16_t value16 = buff[0];
    value16 += static_cast<uint16_t>(buff[1] << BYTE_SIZE);
    return value16;
}

uint32_t ReadLE32(const uint8_t *buff)
{
    if (buff == nullptr) {
        LOG(ERROR) << "buff is null";
        return 0;
    }
    uint16_t low = ReadLE16(buff);
    uint16_t high = ReadLE16(buff + sizeof(uint16_t));
    uint32_t value = ((static_cast<uint32_t>(high)) << (BYTE_SIZE * sizeof(uint16_t))) | low;
    return value;
}

std::ostream &operator<<(std::ostream &os, const Timer &timer)
{
    os << timer.duration().count() << "ms";
    return os;
}

std::string GetRealPath(const std::string &filePath)
{
    char path[PATH_MAX] = {'\0'};
    if (realpath(filePath.c_str(), path) == nullptr) {
        LOG(ERROR) << "get real path fail " << filePath;
        return "";
    }
    if (!CheckPathExists(path)) {
        LOG(ERROR) << "path " << path << " doesn't exist";
        return "";
    }
    std::string realPath(path);
    return realPath;
}

void Revert(const std::string &hmpName, bool reboot)
{
    LOG(INFO) << "RevertAndReboot, reboot: " << reboot;
    std::string installPath = std::string(UPDATE_INSTALL_DIR) + "/" + hmpName;
    if (CheckPathExists(installPath)) {
        if (!ForceRemoveDirectory(installPath)) {
            LOG(ERROR) << "Failed to remove installPath: " << installPath;
            return;
        }
    }
    struct stat statData;
    std::string activePath = std::string(UPDATE_ACTIVE_DIR) + "/" + hmpName;
    int ret = stat(activePath.c_str(), &statData);
    if (ret != 0) {
        LOG(ERROR) << "Failed to access " << activePath << " err=" << errno;
        return;
    }
    if (!ForceRemoveDirectory(activePath)) {
        LOG(ERROR) << "Failed to remove " << activePath;
        return;
    }

    std::string backupPath = std::string(UPDATE_BACKUP_DIR) + "/" + hmpName;
    if (CheckPathExists(backupPath)) {
        ret = rename(backupPath.c_str(), activePath.c_str());
        if (ret != 0) {
            LOG(ERROR) << "Failed to rename " << backupPath << " to " << activePath << " err=" << errno;
        }
        if (ret == 0 && chmod(activePath.c_str(), statData.st_mode & ALL_PERMISSIONS) != 0) {
            LOG(ERROR) << "Failed to restore original permissions for " << activePath << " err=" << errno;
        }
    }
    sync();
    if (reboot) {
        LOG(INFO) << "Rebooting";
        DoReboot("");
    }
}

bool IsHotSa(int32_t saId)
{
    std::vector<int32_t> onDemandSaIds;
    sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        LOG(ERROR) << "get system ability manager error";
        return false;
    }
    samgr->GetOnDemandSystemAbilityIds(onDemandSaIds);
    if (onDemandSaIds.empty()) {
        LOG(ERROR) << "get ondemand saIds fail";
        return false;
    }
    if (find(onDemandSaIds.begin(), onDemandSaIds.end(), saId) == onDemandSaIds.end()) {
        LOG(INFO) << "this is not an ondemand sa, saId=" << saId;
        return false;
    }
    return true;
}

bool IsRunning(int32_t saId)
{
    sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        LOG(ERROR) << "get system ability manager error";
        return false;
    }
    auto object = samgr->CheckSystemAbility(saId);
    if (object == nullptr) {
        LOG(INFO) << "sa not exists, saId=" << saId;
        return false;
    }
    return true;
}

bool CheckBootComplete(void)
{
    char value[PARAM_VALUE_SIZE] = "";
    int ret = GetParameter(BOOT_COMPLETE_PARAM, "", value, PARAM_VALUE_SIZE);
    if (ret < 0) {
        LOG(ERROR) << "Failed to get parameter " << BOOT_COMPLETE_PARAM;
        return false;
    }
    return strcmp(value, BOOT_SUCCESS_VALUE) == 0;
}

bool IsHotHmpPackage(int32_t type)
{
    return false;
}

bool IsHotHmpPackage(const std::string &hmpName)
{
    std::string preInstalledPath = std::string(MODULE_PREINSTALL_DIR) + "/" + hmpName + "/" + HMP_INFO_NAME;
    if (!Utils::IsFileExist(preInstalledPath)) {
        LOG(ERROR) << "preInstalled hmp is not exist: " << preInstalledPath;
        return false;
    }
    std::unique_ptr<ModuleFile> preInstalledFile = ModuleFile::Open(preInstalledPath);
    if (preInstalledFile == nullptr) {
        LOG(ERROR) << "preInstalled file is invalid: " << preInstalledPath;
        return false;
    }
    return IsHotHmpPackage(static_cast<int32_t>(preInstalledFile->GetHmpPackageType()));
}

std::string GetDeviceSaSdkVersion(void)
{
    std::string sdkVersion = system::GetParameter("const.build.sa_sdk_version", "");
    if (sdkVersion.empty()) {
        LOG(ERROR) << "get device sa sdk version failed.";
        return sdkVersion;
    }
    return sdkVersion;
}

int GetDeviceApiVersion(void)
{
    std::string apiVersion = system::GetParameter("const.ohos.apiversion", "");
    if (apiVersion.empty()) {
        LOG(ERROR) << "get device api version failed.";
        return 0;
    }
    return Utils::String2Int<int>(apiVersion, Utils::N_DEC);
}

std::string GetContentFromZip(const std::string &zipPath, const std::string &fileName)
{
    ModuleZipHelper helper(zipPath);
    if (!helper.IsValid()) {
        LOG(ERROR) << "Failed to open file: " << zipPath;
        return "";
    }
    std::string content;
    if (!ExtractZipFile(helper, fileName, content)) {
        LOG(ERROR) << "Failed to extract: " << fileName << " from package: " << zipPath;
        return "";
    }
    return content;
}

void RemoveSpecifiedDir(const std::string &path)
{
    if (!CheckPathExists(path)) {
        return;
    }
    LOG(INFO) << "Remove specified dir: " << path;
    if (!ForceRemoveDirectory(path)) {
        LOG(ERROR) << "Failed to remove: " << path << ", err: " << errno;
    }
}

bool CheckAndUpdateRevertResult(const std::string &hmpPath, const std::string &resultInfo, const std::string &keyWord)
{
    if (resultInfo.find(keyWord) == std::string::npos) {
        return false;
    }
    std::ifstream ifs { MODULE_RESULT_PATH };
    if (!ifs.is_open()) {
        LOG(ERROR) << "ifs open result_file fail" << strerror(errno);
        return false;
    }
    std::string line;
    std::vector<std::string> lines;
    bool ret = false;
    while (getline(ifs, line)) {
        if (line.find(hmpPath) == std::string::npos) {
            lines.push_back(line);
            continue;
        }
        std::vector<std::string> results = Utils::SplitString(line, ";");
        if (results.size() < 3) {  // 3: hmp|result|msg
            LOG(ERROR) << "Split result fail: " << line;
            continue;
        }
        if (results[1] != "0") {  // 1: index of result
            lines.push_back(line);
            continue;
        }
        ret = true;
        lines.push_back(resultInfo);
    }
    ifs.close();
    std::ofstream outfile(MODULE_RESULT_PATH, std::ios::binary | std::ios::trunc);
    if (!outfile) {
        LOG(ERROR) << "ofs open result_file fail" << strerror(errno);
        return false;
    }
    for (const auto &info : lines) {
        outfile << info;
    }
    LOG(INFO) << "Update revert result succ";
    sync();
    return ret;
}

void KillProcessOnArkWeb(void)
{
}

bool InstallHmpBundle(const std::string &hmpPath, bool revert)
{
    LOG(INFO) << "Start to install hmp bundle: " << hmpPath << " ,revert: " << revert;
    return false;
}
} // namespace SysInstaller
} // namespace OHOS