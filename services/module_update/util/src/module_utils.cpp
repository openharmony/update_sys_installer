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
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <fcntl.h>
#include "init_reboot.h"
#include "if_system_ability_manager.h"
#include "iremote_object.h"
#include "iservice_registry.h"
#include "directory_ex.h"
#include "file_ex.h"
#include "log/log.h"
#include "package/package.h"
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

bool CreateDirIfNeeded(const std::string &fpInfo, mode_t mode)
{
    struct stat statData;

    if (stat(fpInfo.c_str(), &statData) != 0) {
        if (errno == ENOENT) {
            if (mkdir(fpInfo.c_str(), mode) != 0) {
                LOG(ERROR) << "Could not mkdir " << fpInfo;
                return false;
            }
        } else {
            LOG(ERROR) << "Could not stat " << fpInfo;
            return false;
        }
    } else {
        if (!S_ISDIR(statData.st_mode)) {
            LOG(ERROR) << fpInfo << " exists and is not a directory";
            return false;
        }
    }

    // Need to manually call chmod because mkdir will create a folder with
    // permissions mode & ~umask.
    if (chmod(fpInfo.c_str(), mode) != 0) {
        LOG(WARNING) << "Could not chmod " << fpInfo;
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
    std::size_t startPos = file.find_last_of('/');
    startPos = (startPos == std::string::npos) ? 0 : startPos + 1;
    std::size_t endPos = file.find_last_of('.');
    if (endPos == std::string::npos || endPos <= startPos) {
        endPos = file.size();
    }
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

bool WaitForFile(const std::string &fpInfo, const std::chrono::nanoseconds &timeout)
{
    Timer timer;
    bool hasSlept = false;
    while (timer.duration() < timeout) {
        struct stat buffer;
        if (stat(fpInfo.c_str(), &buffer) != -1) {
            if (hasSlept) {
                LOG(INFO) << "wait for '" << fpInfo << "' took " << timer;
            }
            return true;
        }
        std::this_thread::sleep_for(WAIT_FOR_FILE_TIME);
        hasSlept = true;
    }
    LOG(ERROR) << "wait for '" << fpInfo << "' timed out and took " << timer;
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

bool WriteFullyAtOffset(int fd, const uint8_t *data, size_t count, off_t offset)
{
    while (count > 0) {
        ssize_t writeSize = pwrite(fd, data, count, offset);
        if (writeSize <= 0) {
            return false;
        }
        data += writeSize;
        count -= static_cast<size_t>(writeSize);
        offset += writeSize;
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
    char fpInfo[PATH_MAX] = {'\0'};
    if (realpath(filePath.c_str(), fpInfo) == nullptr) {
        LOG(ERROR) << "get real path fail " << filePath;
        return "";
    }
    if (!CheckPathExists(fpInfo)) {
        LOG(ERROR) << "path " << fpInfo << " doesn't exist";
        return "";
    }
    std::string realPath(fpInfo);
    return realPath;
}

__attribute__((weak)) bool RevertImageCert(const std::string &hmpName, bool revertMore)
{
    LOG(INFO) << "Revert image cert, default is true";
    return true;
}

__attribute__((weak)) bool VerityInfoWrite(const ModuleFile &file)
{
    LOG(INFO) << "VerityInfoWrite, default is true.";
    return true;
}

__attribute__((weak)) bool PrepareFileToDestDir(const std::string &pkgPath, const std::string &outPath)
{
    if (ExtraPackageDir(pkgPath.c_str(), nullptr, nullptr, outPath.c_str()) != 0) {
        LOG(ERROR) << "Failed to unpack hmp package " << pkgPath;
        return false;
    }
    return true;
}

__attribute__((weak)) void SetModuleVersion(const ModuleFile &file)
{
    LOG(INFO) << "Set module version.";
}

void Revert(const std::string &hmpName, bool reboot)
{
    LOG(INFO) << "RevertAndReboot, reboot: " << reboot << "; hmpName: " << hmpName;
    std::string installPath = std::string(UPDATE_INSTALL_DIR) + "/" + hmpName;
    if (CheckPathExists(installPath)) {
        if (!ForceRemoveDirectory(installPath)) {
            LOG(ERROR) << "Failed to remove installPath: " << installPath << " err=" << errno;
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
        LOG(ERROR) << "Failed to remove " << activePath << " err=" << errno;
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
    RevertImageCert(hmpName, true);
    sync();
    if (reboot) {
        LOG(INFO) << "Rebooting";
        DoReboot("module_update revert.");
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

std::string GetContentFromZip(const std::string &zipPath, const std::string &fpInfo)
{
    ModuleZipHelper helper(zipPath);
    if (!helper.IsValid()) {
        LOG(ERROR) << "Failed to open file: " << zipPath;
        return "";
    }
    std::string content;
    if (!ExtractZipFile(helper, fpInfo, content)) {
        LOG(ERROR) << "Failed to extract: " << fpInfo << " from package: " << zipPath;
        return "";
    }
    return content;
}

bool RemoveSpecifiedDir(const std::string &fpInfo, bool keepDir)
{
    if (!CheckPathExists(fpInfo)) {
        return false;
    }
    LOG(INFO) << "Clear specified dir: " << fpInfo << "; keepdir: " << keepDir;
    if (!keepDir) {
        if (!ForceRemoveDirectory(fpInfo)) {
            LOG(WARNING) << "Failed to remove: " << fpInfo << ", err: " << errno;
            return false;
        }
        return true;
    }
    if (!std::filesystem::is_directory(fpInfo)) {
        LOG(WARNING) << "The file is not a directory: " << fpInfo.c_str();
        return false;
    }
    bool ret = true;
    for (const auto &entry : std::filesystem::directory_iterator(fpInfo)) {
        std::error_code errorCode;
        LOG(INFO) << "deleted " << entry.path().c_str() << ";";
        if (!std::filesystem::remove_all(entry.path(), errorCode)) {
            LOG(ERROR) << "Failed to deleted " << entry.path().c_str() << "; errorCode: " << errorCode.value();
            ret = false;
        }
    }
    return ret;
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

std::string GetCurrentHmpName(void)
{
    std::vector<std::string> files;
    GetDirFiles(MODULE_PREINSTALL_DIR, files);
    for (const auto &file : files) {
        if (!CheckFileSuffix(file, MODULE_PACKAGE_SUFFIX)) {
            continue;
        }
        std::string hmpName = GetHmpName(file);
        if (hmpName.empty()) {
            continue;
        }
        return hmpName;
    }
    return "";
}

int32_t NotifyBmsRevert(const std::string &hmpName, bool record)
{
    LOG(INFO) << "Start to collect module name which contains hap or hsp, record is " << record;
    SetParameter(BMS_START_INSTALL, NOTIFY_BMS_REVERT);
    std::string preInstalledPath = std::string(MODULE_PREINSTALL_DIR) + "/" + hmpName + "/" + HMP_INFO_NAME;
    std::unique_ptr<ModuleFile> moduleFile = ModuleFile::Open(preInstalledPath);
    if (moduleFile == nullptr) {
        LOG(ERROR) << "Invalid preinstalled file " << preInstalledPath;
        return -1;
    }
    int32_t result = 0;
    std::ostringstream oss;
    oss << BMS_START_INSTALL << "=" << NOTIFY_BMS_REVERT << "\n";
    for (const auto &[key, value] : moduleFile->GetVersionInfo().moduleMap) {
        if (value.bundleInfoList.empty()) {
            continue;
        }
        std::string attr = std::string(BMS_RESULT_PREFIX) + "." + key;
        if (!record && SetParameter(attr.c_str(), BMS_INSTALL_FAIL) != 0) {
            LOG(WARNING) << "Failed to set module params: " << attr;
            result++;
        }
        oss << attr << "=" << BMS_INSTALL_FAIL << "\n";
    }
    if (record) {
        if (!OHOS::SaveStringToFile(MODULE_UPDATE_PARAMS_FILE, oss.str(), false)) {
            LOG(ERROR) << "save revert params to file fail";
        }
    }
    return result;
}
} // namespace SysInstaller
} // namespace OHOS