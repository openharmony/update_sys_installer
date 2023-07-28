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

#include "module_hvb_ops.h"

#include <cerrno>
#include <fcntl.h>

#include "log/log.h"
#include "module_file.h"
#include "module_utils.h"
#include "unique_fd.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

using namespace OHOS::SysInstaller;
using namespace Updater;

static bool ParseReadParam(const std::string &path, const int64_t offset, const uint64_t numBytes, off_t &outOffset,
    size_t &outCount)
{
    std::unique_ptr<ModuleFile> file = ModuleFile::Open(path);
    if (file == nullptr) {
        LOG(ERROR) << "failed to parse file " << path;
        return false;
    }
    if (!file->GetImageStat().has_value()) {
        LOG(ERROR) << path << " has no image";
        return false;
    }
    int64_t imageOffset = static_cast<int64_t>(file->GetImageStat().value().imageOffset);
    int64_t imageSize = static_cast<int64_t>(file->GetImageStat().value().imageSize);
    outOffset = offset + imageOffset;
    if (offset < 0) {
        outOffset += imageSize;
    }
    if (outOffset < imageOffset) {
        LOG(ERROR) << "invalid offset " << offset;
        return false;
    }
    outCount = imageOffset + imageSize - outOffset;
    if (outCount > numBytes) {
        outCount = numBytes;
    }
    return true;
}

static enum hvb_io_errno HvbReadFromPartition(
    struct hvb_ops *ops, const char *partition, int64_t offset, uint64_t numBytes, void *buf, uint64_t *outNumRead)
{
    if (partition == nullptr) {
        LOG(ERROR) << "partition is null";
        return HVB_IO_ERROR_IO;
    }
    if (buf == nullptr) {
        LOG(ERROR) << "buf is null";
        return HVB_IO_ERROR_IO;
    }

    std::string path = std::string(partition);
    std::string realPath = GetRealPath(path);
    if (realPath.empty()) {
        LOG(ERROR) << "invalid path " << path;
        return HVB_IO_ERROR_IO;
    }
    off_t realOffset = 0;
    size_t count = 0;
    if (!ParseReadParam(path, offset, numBytes, realOffset, count)) {
        return HVB_IO_ERROR_IO;
    }

    OHOS::UniqueFd fd(open(realPath.c_str(), O_RDONLY | O_CLOEXEC));
    if (fd.Get() == -1) {
        LOG(ERROR) << "failed to open file " << realPath << " err=" << errno;
        return HVB_IO_ERROR_IO;
    }
    if (!ReadFullyAtOffset(fd.Get(), reinterpret_cast<uint8_t *>(buf), count, realOffset)) {
        LOG(ERROR) << "failed to read file " << realPath;
        return HVB_IO_ERROR_IO;
    }
    if (outNumRead != nullptr) {
        *outNumRead = count;
    }

    return HVB_IO_OK;
}

static enum hvb_io_errno HvbWriteToPartition(
    struct hvb_ops *ops, const char *partition, int64_t offset, uint64_t numBytes, const void *buf)
{
    return HVB_IO_OK;
}

static enum hvb_io_errno HvbInvalidateKey(struct hvb_ops *ops, const uint8_t *publicKeyData, uint64_t publicKeyLength,
    const uint8_t *publicKeyMetadata, uint64_t publicKeyMetadataLength, bool *outIsTrusted)
{
    if (outIsTrusted == nullptr) {
        return HVB_IO_ERROR_IO;
    }

    *outIsTrusted = true;

    return HVB_IO_OK;
}

static  enum hvb_io_errno HvbReadRollbackIdx(
    struct hvb_ops *ops, uint64_t rollBackIndexLocation, uint64_t *outRollbackIndex)
{
    if (outRollbackIndex == nullptr) {
        return HVB_IO_ERROR_IO;
    }

    // return 0 as we only need to set up HVB HASHTREE partition
    *outRollbackIndex = 0;

    return HVB_IO_OK;
}

static enum hvb_io_errno HvbWriteRollbackIdx(
    struct hvb_ops *ops, uint64_t rollBackIndexLocation, uint64_t rollbackIndex)
{
    return HVB_IO_OK;
}

static enum hvb_io_errno HvbReadLockState(struct hvb_ops *ops, bool *lock_state)
{
    return HVB_IO_OK;
}

static enum hvb_io_errno HvbGetSizeOfPartition(struct hvb_ops *ops, const char *partition, uint64_t *size)
{
    if (size == nullptr) {
        return HVB_IO_ERROR_IO;
    }

    // The function is for bootloader to load entire content of HVB HASH
    // partition. In user-space, return 0 as we only need to set up HVB
    // HASHTREE partitions.
    *size = 0;
    return HVB_IO_OK;
}

static struct hvb_ops g_hvb_ops = {
    .user_data = &g_hvb_ops,
    .read_partition = HvbReadFromPartition,
    .write_partition = HvbWriteToPartition,
    .valid_rvt_key = HvbInvalidateKey,
    .read_rollback = HvbReadRollbackIdx,
    .write_rollback = HvbWriteRollbackIdx,
    .read_lock_state = HvbReadLockState,
    .get_partiton_size = HvbGetSizeOfPartition,
};

struct hvb_ops *ModuleHvbGetOps(void)
{
    return &g_hvb_ops;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif