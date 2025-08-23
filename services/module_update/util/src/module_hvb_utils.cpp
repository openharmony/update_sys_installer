/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "module_hvb_utils.h"

#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>

#include "module_utils.h"
#include "log/log.h"
#include "scope_guard.h"
#include "securec.h"
#include "utils.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;
using std::string;

namespace {
constexpr uint64_t DEFAULT_MODULE_HVB_INFO_SIZE = 4 * 1024;   // 4K
}

uint64_t GetBlockDeviceSize(int fd)
{
    uint64_t size = 0;
    return (ioctl(fd, BLKGETSIZE64, &size) == 0) ? size : 0;
}

bool SetModuleFooterData(struct hvb_footer &footer, uint64_t blockSize, uint64_t cert_size)
{
    if (memcpy_s(footer.magic, HVB_FOOTER_MAGIC_LEN, HVB_FOOTER_MAGIC, HVB_FOOTER_MAGIC_LEN) != EOK) {
        LOG(ERROR) << "copy footer magic fail";
        return false;
    }
    footer.cert_offset = blockSize - DEFAULT_MODULE_HVB_INFO_SIZE;
    footer.cert_size = cert_size;
    footer.image_size = blockSize - DEFAULT_MODULE_HVB_INFO_SIZE;
    footer.partition_size = blockSize;
    return true;
}

bool GetFooterFromImage(int fd, uint64_t imageSize, struct hvb_footer &footer)
{
    if (imageSize <= HVB_FOOTER_SIZE) {
        LOG(ERROR) << "image imageSize is too small, get footer fail";
        return false;
    }
    uint64_t offset = imageSize - HVB_FOOTER_SIZE;
    if (!ReadFullyAtOffset(fd, reinterpret_cast<uint8_t *>(&footer), HVB_FOOTER_SIZE, offset)) {
        LOG(ERROR) << "read footer info fail";
        return false;
    }
    return true;
}

bool GetCertDataFromImage(int fd, uint64_t imageSize, uint64_t offset, uint8_t *pubkey, uint64_t keySize)
{
    if (imageSize < offset + keySize) {
        LOG(ERROR) << "image imageSize is too small, get pubkey fail";
        return false;
    }
    if (!ReadFullyAtOffset(fd, pubkey, keySize, offset)) {
        LOG(ERROR) << "read image info fail";
        return false;
    }
    return true;
}

bool WriteModuleUpdateBlock(const struct hvb_buf &pubkey, const std::string &partition)
{
    if (pubkey.size + HVB_FOOTER_SIZE > DEFAULT_MODULE_HVB_INFO_SIZE) {
        LOG(ERROR) << "the size of pubkey and footer is override";
        return false;
    }
    std::string partitionPath = Utils::GetPartitionRealPath(partition);
    if (partitionPath.empty()) {
        LOG(ERROR) << "partition is empty, invalid";
        return false;
    }

    auto fd = open(partitionPath.c_str(), O_RDWR);
    if (fd < 0) {
        LOG(ERROR) << "open partition fail, error = " << errno;
        return false;
    }
    ON_SCOPE_EXIT(clear) {
        close(fd);
    };
    // generate footer for module_update partition
    uint64_t blockSize = GetBlockDeviceSize(fd);
    if (blockSize < DEFAULT_MODULE_HVB_INFO_SIZE) {
        LOG(ERROR) << "module update blockSize is too small, blockSize: " << blockSize;
        return false;
    }
    struct hvb_footer footer;
    if (!SetModuleFooterData(footer, blockSize, pubkey.size)) {
        LOG(ERROR) << "set footer fail";
        return false;
    }
    auto buffer = std::make_unique<uint8_t[]>(DEFAULT_MODULE_HVB_INFO_SIZE);
    if (buffer == nullptr) {
        LOG(ERROR) << "make_unique fail";
        return false;
    }
    uint64_t footerOffset = DEFAULT_MODULE_HVB_INFO_SIZE - HVB_FOOTER_SIZE;
    if (memcpy_s(buffer.get(), DEFAULT_MODULE_HVB_INFO_SIZE , pubkey.addr, pubkey.size) != EOK ||
        memcpy_s(buffer.get() + footerOffset, HVB_FOOTER_SIZE, &footer, HVB_FOOTER_SIZE) != EOK) {
        LOG(ERROR) << "copy footer or pubkey fail";
        return false;
    }

    if (!WriteFullyAtOffset(fd, buffer.get(), DEFAULT_MODULE_HVB_INFO_SIZE, blockSize - DEFAULT_MODULE_HVB_INFO_SIZE)) {
        LOG(ERROR) << "write fully hvb info fail";
        return false;
    }
    return true;
}

bool DealModuleUpdateHvbInfo(const std::string &imagePath, uint64_t imageSize, const std::string &partition)
{
    std::string realPath = GetRealPath(imagePath);
    if (realPath.empty()) {
        LOG(ERROR) << "invalid path " << imagePath;
        return false;
    }
    auto fd = open(realPath.c_str(), O_RDWR);
    if (fd < 0) {
        LOG(ERROR) << "open partition "<<  realPath << " fail, error = " << errno;
        return false;
    }
    ON_SCOPE_EXIT(clear) {
        close(fd);
    };

    struct hvb_footer footer;
    if (!GetFooterFromImage(fd, imageSize, footer)) {
        LOG(ERROR) << "get footer from image fail";
        return false;
    }

    // get cert data by footer info
    struct hvb_buf pubkey;
    pubkey.size = footer.cert_size;
    if (pubkey.size > DEFAULT_MODULE_HVB_INFO_SIZE - HVB_FOOTER_SIZE) {
        LOG(ERROR) << "cert size in footer is override";
        return false;
    }
    auto buffer = std::make_unique<uint8_t[]>(pubkey.size);
    if (buffer == nullptr) {
        LOG(ERROR) << "make_unique fail";
        return false;
    }
    pubkey.addr = buffer.get();

    if (!GetCertDataFromImage(fd, imageSize, footer.cert_offset, pubkey.addr, pubkey.size)) {
        LOG(ERROR) << "get pubkey from image fail";
        return false;
    }

    // generate new footer, write cert data and footer into partition
    if (!WriteModuleUpdateBlock(pubkey, partition)) {
        LOG(ERROR) << "write module update partition fail";
        return false;
    }
    return true;
}
} // namespace SysInstaller
} // namespace OHOS