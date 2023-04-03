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
#include <thread>

#include "directory_ex.h"
#include "log/log.h"
#include "module_constants.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

namespace {
constexpr std::chrono::milliseconds WAIT_FOR_FILE_TIME(5);
constexpr uint32_t BYTE_SIZE = 8;
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
        LOG(ERROR) << "Could not chmod " << path;
        return false;
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
} // namespace SysInstaller
} // namespace OHOS