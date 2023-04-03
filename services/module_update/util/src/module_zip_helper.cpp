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

#include "module_zip_helper.h"

#include <fcntl.h>

#include "log/log.h"
#include "module_utils.h"
#include "unique_fd.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

namespace {
struct __attribute__((packed)) LocalFileHeader {
    uint32_t signature = 0;
    uint16_t versionNeeded = 0;
    uint16_t flags = 0;
    uint16_t compressionMethod = 0;
    uint16_t modifiedTime = 0;
    uint16_t modifiedDate = 0;
    uint32_t crc = 0;
    uint32_t compressedSize = 0;
    uint32_t uncompressedSize = 0;
    uint16_t nameSize = 0;
    uint16_t extraSize = 0;
};

struct __attribute__((packed)) CentralDirEntry {
    uint32_t signature = 0;
    uint16_t versionMade = 0;
    uint16_t versionNeeded = 0;
    uint16_t flags = 0; // general purpose bit flag
    uint16_t compressionMethod = 0;
    uint16_t modifiedTime = 0;
    uint16_t modifiedDate = 0;
    uint32_t crc = 0;
    uint32_t compressedSize = 0;
    uint32_t uncompressedSize = 0;
    uint16_t nameSize = 0;
    uint16_t extraSize = 0;
    uint16_t commentSize = 0;
    uint16_t diskNumStart = 0;
    uint16_t internalAttr = 0;
    uint32_t externalAttr = 0;
    uint32_t localHeaderOffset = 0;
};

constexpr uint32_t LOCAL_HEADER_SIGNATURE = 0x04034b50;
constexpr uint32_t CENTRAL_SIGNATURE = 0x02014b50;
}

ModuleZipHelper::ModuleZipHelper(const std::string &path)
{
    handle_ = unzOpen(path.c_str());
    hasLocated_ = false;
    zipPath_ = path;
}

ModuleZipHelper::~ModuleZipHelper()
{
    if (IsValid()) {
        int err = unzClose(handle_);
        if (err != UNZ_OK) {
            LOG(WARNING) << "Close handle error " << err << ". path=" << zipPath_;
        }
    }
}

bool ModuleZipHelper::GetNumberOfEntry(uint32_t &number)
{
    if (!IsValid()) {
        LOG(ERROR) << "Cannot get entry number with invalid handle. path=" << zipPath_;
        return false;
    }
    unz_global_info info;
    int err = unzGetGlobalInfo(handle_, &info);
    if (err != UNZ_OK) {
        LOG(ERROR) << "Get global info error " << err << ". path=" << zipPath_;
        return false;
    }
    number = static_cast<uint32_t>(info.number_entry);
    return true;
}

bool ModuleZipHelper::LocateFile(const std::string &filename)
{
    if (!IsValid()) {
        LOG(ERROR) << "Cannot locate file with invalid handle. path=" << zipPath_;
        return false;
    }
    int err = unzLocateFile2(handle_, filename.c_str(), 0);
    if (err != UNZ_OK) {
        LOG(ERROR) << filename << " is not found in " << zipPath_;
        hasLocated_ = false;
        return false;
    }
    hasLocated_ = true;
    filename_ = filename;
    return true;
}

bool ModuleZipHelper::GetFileSize(uint32_t &size)
{
    if (!hasLocated_) {
        LOG(ERROR) << "Cannot get file size without located file. path=" << zipPath_;
        return false;
    }
    unz_file_info info;
    int err = unzGetCurrentFileInfo(handle_, &info, nullptr, 0, nullptr, 0, nullptr, 0);
    if (err != UNZ_OK) {
        LOG(ERROR) << "Get file size error " << err << ". " << filename_ << " in " << zipPath_;
        return false;
    }
    size = static_cast<uint32_t>(info.uncompressed_size);
    return true;
}

bool ModuleZipHelper::GetFileOffset(uint32_t &offset)
{
    if (!hasLocated_) {
        LOG(ERROR) << "Cannot get file offset without located file. path=" << zipPath_;
        return false;
    }
    unz_file_pos filePos;
    int err = unzGetFilePos(handle_, &filePos);
    if (err != UNZ_OK) {
        LOG(ERROR) << "Get file pos error " << err << ". " << filename_ << " in " << zipPath_;
        return false;
    }
    uint32_t centralDirOffset = static_cast<uint32_t>(filePos.pos_in_zip_directory);
    if (!GetFileEntryOffset(offset, centralDirOffset)) {
        LOG(ERROR) << "Cannot get file entry offset";
        return false;
    }
    return true;
}

bool ModuleZipHelper::GetFileEntryOffset(uint32_t &offset, uint32_t centralDirOffset) const
{
    std::string realPath = GetRealPath(zipPath_);
    if (realPath.empty()) {
        LOG(ERROR) << "Invalid path " << zipPath_;
        return false;
    }
    UniqueFd fd(open(realPath.c_str(), O_RDONLY | O_CLOEXEC));
    if (fd.Get() == -1) {
        LOG(ERROR) << "Cannot open package " << zipPath_;
        return false;
    }
    uint8_t centralDirBuf[sizeof(CentralDirEntry)];
    if (!ReadFullyAtOffset(fd.Get(), centralDirBuf, sizeof(CentralDirEntry), centralDirOffset)) {
        LOG(ERROR) << "Unable to read central directory";
        return false;
    }
    uint32_t centralSignature = ReadLE32(centralDirBuf + offsetof(CentralDirEntry, signature));
    if (centralSignature != CENTRAL_SIGNATURE) {
        LOG(ERROR) << "Check central signature error";
        return false;
    }
    uint32_t localHeaderOffset = ReadLE32(centralDirBuf + offsetof(CentralDirEntry, localHeaderOffset));
    uint8_t localHeaderBuf[sizeof(LocalFileHeader)];
    if (!ReadFullyAtOffset(fd.Get(), localHeaderBuf, sizeof(LocalFileHeader), localHeaderOffset)) {
        LOG(ERROR) << "Unable to read local file header";
        return false;
    }
    uint32_t localHeaderSignature = ReadLE32(localHeaderBuf + offsetof(LocalFileHeader, signature));
    if (localHeaderSignature != LOCAL_HEADER_SIGNATURE) {
        LOG(ERROR) << "Check local header signature error";
        return false;
    }
    uint16_t nameSize = ReadLE16(localHeaderBuf + offsetof(LocalFileHeader, nameSize));
    uint16_t extraSize = ReadLE16(localHeaderBuf + offsetof(LocalFileHeader, extraSize));
    offset = localHeaderOffset + sizeof(LocalFileHeader) + nameSize + extraSize;
    return true;
}

bool ModuleZipHelper::GetFileContent(std::string &buf)
{
    if (!hasLocated_) {
        LOG(ERROR) << "Cannot get file content without located file. path=" << zipPath_;
        return false;
    }
    uint32_t length;
    if (!GetFileSize(length)) {
        LOG(ERROR) << "Cannot get buf length.";
        return false;
    }
    buf.resize(length, '\0');

    int err = unzOpenCurrentFile(handle_);
    if (err != UNZ_OK) {
        LOG(ERROR) << "Open current file error " << err << ". " << filename_ << " in " << zipPath_;
        return false;
    }
    int size = unzReadCurrentFile(handle_, reinterpret_cast<void *>(&(buf)[0]), length);
    err = unzCloseCurrentFile(handle_);
    if (size < 0) {
        LOG(ERROR) << "Read current file error. " << filename_ << " in " << zipPath_;
        return false;
    }
    if (err != UNZ_OK) {
        LOG(ERROR) << "Close current file error " << err << ". " << filename_ << " in " << zipPath_;
        return false;
    }
    return true;
}
} // namespace SysInstaller
} // namespace OHOS