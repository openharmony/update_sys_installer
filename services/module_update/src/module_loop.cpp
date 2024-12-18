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

#include "module_loop.h"
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <libgen.h>
#include <mutex>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <vector>
#include <linux/fs.h>
#include <linux/loop.h>
#include <linux/magic.h>
#include <cerrno>
#include "securec.h"
#include "string_ex.h"
#include "log/log.h"
#include "module_dm.h"
#include "module_utils.h"
#include "module_constants.h"
#include "sys/sysmacros.h"
#include "scope_guard.h"

namespace OHOS {
namespace SysInstaller {
namespace Loop {
using namespace Updater;
using std::string;

namespace {
const int LOOP_LENGTH = 4;
const int LOOP_CTL_LENGTH = 12;
constexpr const char *LOOP_CTL_PATH = "/dev/loop-control";
constexpr const char *BLOCK_DEV_PATH = "/dev/block";
constexpr const char *LOOP_PREFIX = "loop";
constexpr const char *DEVICE_PREFIX = "/dev/";
constexpr const char *SYSTEM_BLOCK_PATH = "/sys/block/";
constexpr const char *READ_AHEAD_NAME = "/queue/read_ahead_kb";
constexpr const char *READ_AHEAD_KB = "128";
constexpr const char *MODULE_LOOP_PREFIX = "module:";
constexpr const char *LOOP_DEV_PATH = "/dev/loop";
constexpr const char *LOOP_BLOCK_PATH = "/dev/block/loop";
const size_t LOOP_DEVICE_RETRY_ATTEMPTS = 6u;
const uint32_t LOOP_BLOCK_SIZE = 4096;
const std::chrono::milliseconds WAIT_FOR_DEVICE_TIME(50);
const std::chrono::seconds WAIT_FOR_LOOP_TIME(50);
}

static bool IsRealPath(std::string path)
{
    char buf[PATH_MAX] = { 0 };
    if (realpath(path.c_str(), buf) == nullptr) {
        return false;
    }
    std::string tmpPath = buf;
    return (path == tmpPath);
}

void LoopbackDeviceUniqueFd::MaybeCloseBad() const
{
    if (deviceFd.Get() != -1) {
        int ret = ioctl(deviceFd.Get(), LOOP_CLR_FD);
        if (ret < 0) {
            LOG(ERROR) << "Failed to clear fd for loopback device";
        }
    }
}

bool PreAllocateLoopDevices(const size_t num)
{
    if (!WaitForFile(LOOP_CTL_PATH, WAIT_FOR_LOOP_TIME)) {
        LOG(ERROR) << "loop-control is not ready";
        return false;
    }
    int fd = open(LOOP_CTL_PATH, O_RDWR | O_CLOEXEC);
    if (fd == -1) {
        LOG(ERROR) << "Failed to open loop-control";
        return false;
    }
    UniqueFd ctlFd(fd);

    bool found = false;
    size_t startId = 0;
    DIR *dir = opendir(BLOCK_DEV_PATH);
    if (dir == nullptr) {
        LOG(ERROR) << "Failed to open " << BLOCK_DEV_PATH;
        return false;
    }
    struct dirent *ptr = nullptr;
    while ((ptr = readdir(dir)) != nullptr) {
        if (strncmp(ptr->d_name, LOOP_PREFIX, strlen(LOOP_PREFIX)) != 0) {
            continue;
        }
        string idStr = ptr->d_name + strlen(LOOP_PREFIX);
        int id = 0;
        if (StrToInt(idStr, id)) {
            size_t devId = static_cast<size_t>(id);
            if (startId < devId) {
                startId = devId;
                found = true;
            }
        }
    }
    closedir(dir);
    if (found) {
        startId++;
    }
    LOG(INFO) << "start id is " << startId;

    for (size_t id = startId; id < num + startId; ++id) {
        int ret = ioctl(ctlFd.Get(), LOOP_CTL_ADD, id);
        if (ret < 0 && errno != EEXIST) {
            LOG(ERROR) << "Failed to add loop device";
            return false;
        }
    }
    LOG(INFO) << "Pre-allocated " << num << " loopback devices";
    return true;
}

bool ConfigureReadAhead(const string &devicePath)
{
    if (!StartsWith(devicePath, DEVICE_PREFIX)) {
        LOG(ERROR) << "invalid device path " << devicePath;
        return false;
    }
    string path(devicePath);
    string deviceName = basename(&path[0]);
    string sysfsDevice = SYSTEM_BLOCK_PATH + deviceName + READ_AHEAD_NAME;
    string realPath = GetRealPath(sysfsDevice);
    if (realPath.empty()) {
        LOG(ERROR) << "invalid device path " << sysfsDevice;
        return false;
    }

    struct stat fileState;
    if (stat(realPath.c_str(), &fileState) != 0) {
        LOG(ERROR) << "Fail to Stat file: " << realPath << ", errno=" << errno;
        return false;
    }
    ON_SCOPE_EXIT(recoveryMode) {
        (void)chmod(realPath.c_str(), fileState.st_mode);
    };
    UniqueFd sysfsFd(open(realPath.c_str(), O_RDWR | O_CLOEXEC));
    if (sysfsFd.Get() == -1) {
        // 0644: give permission to write
        if (chmod(realPath.c_str(), 0644) != 0) {
            LOG(WARNING) << "Fail to chmod file: " << realPath << ", errno=" << errno;
        }
        sysfsFd = UniqueFd(open(realPath.c_str(), O_RDWR | O_CLOEXEC));
        if (sysfsFd.Get() == -1) {
            LOG(ERROR) << "Fail to open file: " << realPath << ", errno=" << errno;
            return false;
        }
    }
    int writeBytes = write(sysfsFd.Get(), READ_AHEAD_KB, strlen(READ_AHEAD_KB) + 1);
    if (writeBytes < 0) {
        LOG(ERROR) << "Failed to write to " << realPath;
        return false;
    }
    return true;
}

bool CheckIfSupportLoopConfigure(const int deviceFd)
{
#ifdef LOOP_CONFIGURE
    struct loop_config config;
    (void)memset_s(&config, sizeof(config), 0, sizeof(config));
    config.fd = -1;
    return ioctl(deviceFd, LOOP_CONFIGURE, &config) == -1 && errno == EBADF;
#else
    return false;
#endif
}

bool ConfigureLoopDevice(const int deviceFd, const int targetFd, struct loop_info64 li, const bool useBufferedIo)
{
#ifdef LOOP_CONFIGURE
    struct loop_config config;
    (void)memset_s(&config, sizeof(config), 0, sizeof(config));
    config.fd = targetFd;
    config.info = li;
    config.block_size = LOOP_BLOCK_SIZE;
    if (!useBufferedIo) {
        li.lo_flags |= LO_FLAGS_DIRECT_IO;
    }
    int ret = ioctl(deviceFd, LOOP_CONFIGURE, &config);
    if (ret < 0) {
        LOG(ERROR) << "Failed to configure loop device err=" << errno;
        return false;
    }
    return true;
#else
    return false;
#endif
}

bool SetLoopDeviceStatus(const int deviceFd, const int targetFd, const struct loop_info64 *li)
{
    int ret = ioctl(deviceFd, LOOP_SET_FD, targetFd);
    if (ret < 0) {
        LOG(ERROR) << "Failed to set loop fd err=" << errno;
        return false;
    }
    ret = ioctl(deviceFd, LOOP_SET_STATUS64, li);
    if (ret < 0) {
        LOG(ERROR) << "Failed to set loop status err=" << errno;
        return false;
    }
    ret = ioctl(deviceFd, BLKFLSBUF, 0);
    if (ret < 0) {
        LOG(WARNING) << "Failed to flush buffers on the loop device err=" << errno;
    }
    ret = ioctl(deviceFd, LOOP_SET_BLOCK_SIZE, LOOP_BLOCK_SIZE);
    if (ret < 0) {
        LOG(WARNING) << "Failed to set block size err=" << errno;
    }
    return true;
}

bool SetUpLoopDevice(const int deviceFd, const string &target, const uint32_t imageOffset, const uint32_t imageSize)
{
    static bool useLoopConfigure = CheckIfSupportLoopConfigure(deviceFd);
    bool useBufferedIo = false;
    string realPath = GetRealPath(target);
    if (realPath.empty()) {
        LOG(ERROR) << "invalid target " << target;
        return false;
    }
    UniqueFd targetFd(open(realPath.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECT));
    if (targetFd.Get() == -1) {
        struct statfs stbuf;
        int savedErrno = errno;
        if (statfs(realPath.c_str(), &stbuf) != 0 ||
            (stbuf.f_type != EROFS_SUPER_MAGIC_V1 &&
             stbuf.f_type != SQUASHFS_MAGIC &&
             stbuf.f_type != OVERLAYFS_SUPER_MAGIC &&
             stbuf.f_type != EXT4_SUPER_MAGIC)) {
            LOG(ERROR) << "Failed to open " << realPath << " errno=" << savedErrno;
            return false;
        }
        LOG(WARNING) << "Fallback to buffered I/O for " << realPath;
        useBufferedIo = true;
        targetFd = UniqueFd(open(realPath.c_str(), O_RDONLY | O_CLOEXEC));
        if (targetFd.Get() == -1) {
            LOG(ERROR) << "Failed to open " << realPath;
            return false;
        }
    }

    struct loop_info64 li;
    (void)memset_s(&li, sizeof(li), 0, sizeof(li));
    errno_t ret = strcpy_s(reinterpret_cast<char*>(li.lo_crypt_name), LO_NAME_SIZE, MODULE_LOOP_PREFIX);
    if (ret != EOK) {
        LOG(ERROR) << "Failed to copy loop prefix " << MODULE_LOOP_PREFIX;
        return false;
    }
    li.lo_offset = imageOffset;
    li.lo_sizelimit = imageSize;
    li.lo_flags |= LO_FLAGS_AUTOCLEAR;
    return useLoopConfigure ? ConfigureLoopDevice(deviceFd, targetFd.Get(), li, useBufferedIo)
        : SetLoopDeviceStatus(deviceFd, targetFd.Get(), &li);
}

std::unique_ptr<LoopbackDeviceUniqueFd> WaitForDevice(const int num)
{
    const std::vector<string> candidateDevices = {
        LOOP_BLOCK_PATH + std::to_string(num),
        LOOP_DEV_PATH + std::to_string(num),
    };

    UniqueFd sysfsFd;
    for (size_t i = 0; i < LOOP_DEVICE_RETRY_ATTEMPTS; ++i) {
        for (const auto &device : candidateDevices) {
            string realPath = GetRealPath(device);
            if (realPath.empty()) {
                continue;
            }
            sysfsFd = UniqueFd(open(realPath.c_str(), O_RDWR | O_CLOEXEC));
            if (sysfsFd.Get() != -1) {
                return std::make_unique<LoopbackDeviceUniqueFd>(std::move(sysfsFd), realPath);
            }
        }
        LOG(WARNING) << "Loopback device " << num << " not ready. Waiting 50ms...";
        usleep(std::chrono::duration_cast<std::chrono::microseconds>(WAIT_FOR_DEVICE_TIME).count());
    }
    LOG(ERROR) << "Failed to open loopback device " << num;
    return nullptr;
}

std::unique_ptr<LoopbackDeviceUniqueFd> CreateLoopDevice(
    const string &target, const uint32_t imageOffset, const uint32_t imageSize)
{
    UniqueFd ctlFd(open(LOOP_CTL_PATH, O_RDWR | O_CLOEXEC));
    if (ctlFd.Get() == -1) {
        LOG(ERROR) << "Failed to open loop-control";
        return nullptr;
    }
    static std::mutex mlock;
    std::lock_guard lock(mlock);
    int num = ioctl(ctlFd.Get(), LOOP_CTL_GET_FREE);
    if (num < 0) {
        LOG(ERROR) << "Failed to get free loop device err=" << errno;
        return nullptr;
    }
    LOG(INFO) << "Get free loop device num " << num;
    std::unique_ptr<LoopbackDeviceUniqueFd> loopDevice = WaitForDevice(num);
    if (loopDevice == nullptr) {
        LOG(ERROR) << "Failed to create loop device " << num;
        return nullptr;
    }
    if (!SetUpLoopDevice(loopDevice->deviceFd.Get(), target, imageOffset, imageSize)) {
        LOG(ERROR) << "Failed to configure device";
        return nullptr;
    }
    if (!ConfigureReadAhead(loopDevice->name)) {
        LOG(ERROR) << "Failed to configure read ahead";
        return nullptr;
    }
    return loopDevice;
}

bool RemoveDmLoopDevice(const std::string &mountPoint, const std::string &imagePath)
{
    struct dirent *ent = nullptr;
    DIR *dir = nullptr;

    if ((dir = opendir(BLOCK_DEV_PATH)) == nullptr) {
        LOG(ERROR) << "Failed to open loop dir";
        return false;
    }
    bool ret = false;
    std::string loopDevPath = "";
    while ((ent = readdir(dir)) != nullptr) {
        if (strncmp(ent->d_name, "loop", LOOP_LENGTH) || !strncmp(ent->d_name, "loop-control", LOOP_CTL_LENGTH)) {
            continue;
        }

        loopDevPath = std::string(BLOCK_DEV_PATH) + "/" + std::string(ent->d_name);
        if (!IsRealPath(loopDevPath)) {
            LOG(ERROR) << "Dev is not exist, loopDevPath=" << loopDevPath;
            loopDevPath = "";
            continue;
        }
        if (!IsLoopDevMatchedImg(loopDevPath, imagePath)) {
            loopDevPath = "";
            continue;
        }
        if (umount(mountPoint.c_str()) != 0) {
            LOG(WARNING) << "Could not umount " << mountPoint << " errno: " << errno;
        }
        bool clearDm = (imagePath.find(UPDATE_ACTIVE_DIR) != std::string::npos);
        ret = ClearDmLoopDevice(loopDevPath, clearDm);
        break;
    }
    closedir(dir);
    return ret;
}

bool RemoveDmLoopDevice(const std::string &loopDevPath)
{
#ifndef USER_DEBUG_MODE
    if (!RemoveDmDevice(loopDevPath)) {
        LOG(ERROR) << "Close dm error, loopDevPath=" << loopDevPath.c_str() << ", errno=" << errno;
        return false;
    }
#endif
    if (!CloseLoopDev(loopDevPath)) {
        LOG(ERROR) << "Close loop error, loopDevPath=" << loopDevPath.c_str() << ", errno=" << errno;
        return false;
    }
    return true;
}

bool ClearDmLoopDevice(const std::string &loopDevPath, const bool clearDm)
{
#ifndef USER_DEBUG_MODE
    if (clearDm) {
        if (!RemoveDmDevice(loopDevPath)) {
            LOG(ERROR) << "Close dm error, loopDevPath=" << loopDevPath.c_str() << ", errno=" << errno;
            return false;
        }
    }
#endif
    if (!CloseLoopDev(loopDevPath)) {
        LOG(ERROR) << "Close loop error, loopDevPath=" << loopDevPath.c_str() << ", errno=" << errno;
        return false;
    }
    return true;
}

bool IsLoopDevMatchedImg(const std::string &loopPath, const std::string &imgFilePath)
{
    struct loop_info64 info;
    if (memset_s(&info, sizeof(struct loop_info64), 0, sizeof(struct loop_info64)) != EOK) {
        LOG(ERROR) << "memset_s failed";
        return false;
    }

    int fd = open(loopPath.c_str(), O_RDWR | O_CLOEXEC);
    if (fd == -1) {
        LOG(ERROR) << "Open failed, loopPath=" << loopPath.c_str() << ", errno=" << errno;
        return false;
    }

    if (ioctl(fd, LOOP_GET_STATUS64, &info) < 0) {
        close(fd);
        return false;
    }
    close(fd);
    return (imgFilePath == std::string(reinterpret_cast<char *>(info.lo_file_name)));
}

bool CloseLoopDev(const std::string &loopPath)
{
    struct stat st;
    if (stat(loopPath.c_str(), &st)) {
        LOG(INFO) << "Stat error, loopPath=" << loopPath.c_str() << ", errno=" << errno;
        return false;
    }

    int userFd = open(loopPath.c_str(), O_RDWR);
    if (userFd < 0) {
        LOG(ERROR) << "Open error, loopPath=" << loopPath.c_str() << ", errno=" << errno;
        return false;
    }

    int ret = ioctl(userFd, LOOP_CLR_FD);
    close(userFd);
    if (ret != 0) {
        LOG(ERROR) << "Clear error, loopPath=" << loopPath.c_str() << ", errno=" << errno;
        return false;
    }
    return true;
}
} // Loop
} // SysInstaller
} // namespace OHOS