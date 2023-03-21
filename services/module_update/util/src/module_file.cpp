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

#include "module_file.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "json_node.h"
#include "log/log.h"
#include "module_constants.h"
#include "module_utils.h"
#include "module_zip_helper.h"
#include "package/package.h"
#include "scope_guard.h"
#include "securec.h"
#include "string_ex.h"
#include "unique_fd.h"
#include "utils.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;
using std::string;

namespace {
constexpr const char *JSON_NODE_NAME = "name";
constexpr const char *JSON_NODE_SAID = "id";
constexpr const char *JSON_NODE_VERSION = "version";
constexpr const char *VERSION_DELIMITER = ".";
constexpr size_t API_VERSION_INDEX = 0;
constexpr size_t VERSION_CODE_INDEX = 1;
constexpr size_t PATCH_VERSION_INDEX = 2;
constexpr size_t VERSION_VECTOR_SIZE = 3;

struct FsMagic {
    const char *type;
    int32_t offset;
    int16_t len;
    const char *magic;
};
constexpr const FsMagic FS_TYPES[] = {{"f2fs", 1024, 4, "\x10\x20\xF5\xF2"},
                                      {"ext4", 1080, 2, "\x53\xEF"}};

const char *RetrieveFsType(int fd, uint32_t imageOffset)
{
    for (const auto &fs : FS_TYPES) {
        uint8_t buf[fs.len];
        if (!ReadFullyAtOffset(fd, buf, fs.len, imageOffset + fs.offset)) {
            LOG(ERROR) << "Couldn't read filesystem magic";
            return nullptr;
        }
        if (memcmp(buf, fs.magic, fs.len) == 0) {
            return fs.type;
        }
    }
    LOG(ERROR) << "Couldn't find filesystem magic";
    return nullptr;
}

bool ParseImageStat(ModuleZipHelper &helper, const string &path, ImageStat &imageStat)
{
    if (!helper.LocateFile(IMG_FILE_NAME)) {
        LOG(ERROR) << "Could not find " << IMG_FILE_NAME << " in package " << path;
        return false;
    }
    if (!helper.GetFileOffset(imageStat.imageOffset)) {
        LOG(ERROR) << "Failed to get file offset from package " << path;
        return false;
    }
    if (!helper.GetFileSize(imageStat.imageSize)) {
        LOG(ERROR) << "Failed to get file size from package " << path;
        return false;
    }

    UniqueFd fd(open(path.c_str(), O_RDONLY | O_CLOEXEC));
    if (fd.Get() == -1) {
        LOG(ERROR) << "Failed to open package " << path << ": I/O error";
        return false;
    }
    const char *fsTypePtr = RetrieveFsType(fd.Get(), imageStat.imageOffset);
    if (fsTypePtr == nullptr) {
        LOG(ERROR) << "Failed to get fs type " << path;
        return false;
    }
    errno_t ret = strcpy_s(imageStat.fsType, FS_TYPE_MAX_SIZE, fsTypePtr);
    if (ret != EOK) {
        LOG(ERROR) << "Failed to copy fs type " << fsTypePtr;
        return false;
    }
    return true;
}

bool ExtractZipFile(ModuleZipHelper &helper, const string &fileName, string &buf)
{
    if (!helper.LocateFile(fileName)) {
        LOG(ERROR) << "Could not find " << fileName;
        return false;
    }
    if (!helper.GetFileContent(buf)) {
        LOG(ERROR) << "Failed to get content of " << fileName;
        return false;
    }
    return true;
}

bool ParseModuleInfo(const string &moduleInfo, string &saName, int32_t &saId, ModuleVersion &versionInfo)
{
    JsonNode root(moduleInfo);

    std::optional<string> name = root[JSON_NODE_NAME].As<string>();
    if (!name.has_value()) {
        LOG(ERROR) << "Failed to get name string";
        return false;
    }
    saName = name.value();

    std::optional<int> optionalSaId = root[JSON_NODE_SAID].As<int>();
    if (!optionalSaId.has_value()) {
        LOG(ERROR) << "Failed to get saId";
        return false;
    }
    saId = static_cast<int32_t>(optionalSaId.value());

    std::optional<string> versionStr = root[JSON_NODE_VERSION].As<string>();
    if (!versionStr.has_value()) {
        LOG(ERROR) << "Failed to get version string";
        return false;
    }
    std::vector<string> versionVec;
    SplitStr(versionStr.value(), VERSION_DELIMITER, versionVec);
    if (versionVec.size() != VERSION_VECTOR_SIZE) {
        LOG(ERROR) << "invalid version " << versionStr.value();
        return false;
    }
    versionInfo.apiVersion = static_cast<uint32_t>(std::stoi(versionVec.at(API_VERSION_INDEX)));
    versionInfo.versionCode = static_cast<uint32_t>(std::stoi(versionVec.at(VERSION_CODE_INDEX)));
    versionInfo.patchVersion = static_cast<uint32_t>(std::stoi(versionVec.at(PATCH_VERSION_INDEX)));

    LOG(INFO) << "ParseModuleInfo success. name: " << saName << " saId: " << saId << " version: " <<
        static_cast<string>(versionInfo);
    return true;
}
} // namespace

bool ModuleFile::VerifyModulePackageSign(const std::string &path)
{
    return VerifyPackage(path.c_str(), Utils::GetCertName().c_str(), "", nullptr, 0) == 0;
}

std::unique_ptr<ModuleFile> ModuleFile::Open(const string &path)
{
    ModuleZipHelper helper(path);
    if (!helper.IsValid()) {
        LOG(ERROR) << "Failed to open file " << path;
        return nullptr;
    }

    string moduleInfo;
    if (!ExtractZipFile(helper, CONFIG_FILE_NAME, moduleInfo)) {
        LOG(ERROR) << "Failed to extract " << CONFIG_FILE_NAME << " from package " << path;
        return nullptr;
    }
    string saName;
    int32_t saId = 0;
    ModuleVersion versionInfo;
    if (!ParseModuleInfo(moduleInfo, saName, saId, versionInfo)) {
        LOG(ERROR) << "Failed to parse version info of package " << path;
        return nullptr;
    }

    ImageStat tmpStat;
    std::optional<ImageStat> imageStat;
    if (ParseImageStat(helper, path, tmpStat)) {
        imageStat = std::move(tmpStat);
    } else if (!StartsWith(path, MODULE_PREINSTALL_DIR)) {
        LOG(ERROR) << "Update package without image " << path;
        return nullptr;
    }

    string modulePubkey = "";
    return std::make_unique<ModuleFile>(path, saName, saId, versionInfo, modulePubkey, imageStat);
}

bool ModuleFile::CompareVersion(const ModuleFile &file1, const ModuleFile &file2)
{
    ModuleVersion version1 = file1.GetVersionInfo();
    ModuleVersion version2 = file2.GetVersionInfo();
    if (version1.apiVersion != version2.apiVersion) {
        return false;
    }
    if (version1.versionCode != version2.versionCode) {
        return version1.versionCode > version2.versionCode;
    }
    return version1.patchVersion >= version2.patchVersion;
}

bool ModuleFile::VerifyModuleVerity(const string &publicKey) const
{
    LOG(INFO) << "do not support hvb";
    return true;
}

void ModuleFile::ClearVerifiedData() const
{
    // using hvb_chain_verify_data_free to clear data
}
} // namespace SysInstaller
} // namespace OHOS