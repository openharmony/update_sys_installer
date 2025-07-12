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

#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_set>

#include "directory_ex.h"
#include "module_utils.h"
#include "json_node.h"
#include "log/log.h"
#include "module_constants.h"
#include "module_zip_helper.h"
#include "package/package.h"
#include "scope_guard.h"
#include "securec.h"
#include "string_ex.h"
#include "unique_fd.h"
#include "utils.h"

#ifdef SUPPORT_HVB
#include "hvb.h"
#include "hvb_footer.h"
#include "module_hvb_ops.h"
#include "module_hvb_utils.h"
#endif

namespace OHOS {
namespace SysInstaller {
using namespace Updater;
using std::string;

namespace {
constexpr const char *VERSION_DELIMITER = ".";
constexpr size_t API_VERSION_INDEX = 0;
constexpr size_t VERSION_CODE_INDEX = 1;
constexpr size_t PATCH_VERSION_INDEX = 2;
constexpr size_t VERSION_VECTOR_SIZE = 3;
constexpr size_t PACKINFO_VERSION_VECTOR_SIZE = 5;
constexpr size_t HMP_VERSION_TYPE_NUM = 4;
constexpr size_t SA_SDK_VERSION_TYPE_NUM = 3;

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

bool ParseImageStat(const string &fpInfo, ImageStat &imageStat)
{
    string realPath = GetRealPath(fpInfo);
    if (realPath.empty() || !Utils::IsFileExist(realPath)) {
        LOG(ERROR) << "Invalid path " << fpInfo;
        return false;
    }
    struct stat buffer;
    if (stat(realPath.c_str(), &buffer) != 0) {
        LOG(ERROR) << "stat file " << fpInfo << " failed.";
        return false;
    }
    imageStat.imageOffset = 0;
    imageStat.imageSize = static_cast<uint32_t>(buffer.st_size);

    UniqueFd fd(open(realPath.c_str(), O_RDONLY | O_CLOEXEC));
    if (fd.Get() == -1) {
        LOG(ERROR) << "Failed to open package " << fpInfo << ": I/O error";
        return false;
    }
    const char *fsTypePtr = RetrieveFsType(fd.Get(), imageStat.imageOffset);
    if (fsTypePtr == nullptr) {
        LOG(ERROR) << "Failed to get fs type " << fpInfo;
        return false;
    }
    errno_t ret = strcpy_s(imageStat.fsType, FS_TYPE_MAX_SIZE, fsTypePtr);
    if (ret != EOK) {
        LOG(ERROR) << "Failed to copy fs type " << fsTypePtr;
        return false;
    }
    return true;
}

bool ParseHmpVersionInfo(const JsonNode &package, ModulePackageInfo &versionInfo)
{
    std::optional<string> name = package["name"].As<string>();
    if (!name.has_value()) {
        LOG(ERROR) << "Hmpinfo: Failed to get hmp name val";
        return false;
    }
    std::optional<string> version = package["version"].As<string>();
    if (!version.has_value()) {
        LOG(ERROR) << "Hmpinfo: Failed to get hmp version val";
        return false;
    }
    std::optional<string> displayVersion = package["displayVersion"].As<string>();
    if (!displayVersion.has_value()) {
        LOG(ERROR) << "Hmpinfo: Failed to get hmp display version val";
        return false;
    }
    versionInfo.hmpName = name.value();
    versionInfo.version = version.value();
    versionInfo.displayVersion = displayVersion.value();

    if (versionInfo.type != HMP_SA_TYPE && versionInfo.type != HMP_SA_TYPE_OLD) {
        std::optional<string> apiVersion = package[HMP_API_VERSION].As<string>();
        if (!apiVersion.has_value()) {
            LOG(ERROR) << "Hmpinfo: Failed to get apiVersion val";
            return false;
        }
        versionInfo.apiVersion = Utils::String2Int<int>(apiVersion.value(), Utils::N_DEC);
    } else if (versionInfo.type != HMP_APP_TYPE) {
        std::optional<string> saSdkVersion = package[HMP_SA_SDK_VERSION].As<string>();
        if (!saSdkVersion.has_value()) {
            LOG(ERROR) << "Hmpinfo: Failed to get saSdkVersion val";
            return false;
        }
    }
    return true;
}

bool ParseSaVersion(const string &versionStr, SaInfo &info)
{
    std::vector<string> versionVec;
    SplitStr(versionStr, VERSION_DELIMITER, versionVec);
    if (versionVec.size() != VERSION_VECTOR_SIZE) {
        LOG(ERROR) << "SaVersion: Invalid version: " << versionStr;
        return false;
    }
    if (!Utils::ConvertToUnsignedLong(versionVec.at(API_VERSION_INDEX), info.version.apiVersion) ||
        !Utils::ConvertToUnsignedLong(versionVec.at(VERSION_CODE_INDEX), info.version.versionCode) ||
        !Utils::ConvertToUnsignedLong(versionVec.at(PATCH_VERSION_INDEX), info.version.patchVersion)) {
        LOG(ERROR) << "ConvertToUnsignedLong failed";
        return false;
    }
    return true;
}

bool ParseSaList(const JsonNode &package, ModuleInfo &versionInfo)
{
    const auto &saListJson = package["saList"];
    for (const auto &saInfo : saListJson) {
        std::optional<string> saInfoStr = saInfo.get().As<string>();
        if (!saInfoStr.has_value()) {
            LOG(ERROR) << "SaList: Failed to get saInfoStr val";
            return false;
        }
        std::vector<string> saInfoVec;
        SplitStr(saInfoStr.value(), " ", saInfoVec);
        if (saInfoVec.size() != 3) {  // 3: name id version
            LOG(ERROR) << "SaList: Invalid saInfoStr: " << saInfoStr.value();
            return false;
        }
        SaInfo &infoTmp = versionInfo.saInfoList.emplace_back();
        infoTmp.saName = saInfoVec.at(0);  // 0:index of name
        if (!Utils::ConvertToLong(saInfoVec.at(1), infoTmp.saId)) { // 1: index of saId
            LOG(ERROR) << "ConvertToLong failed";
            return false;
        }
        if (!ParseSaVersion(saInfoVec.at(2), infoTmp)) {  // 2:index of version
            return false;
        }
    }
    return true;
}

bool ParseBundleList(const JsonNode &package, ModuleInfo &versionInfo)
{
    const auto &bundleListJson = package["bundleList"];
    for (const auto &bundleInfo : bundleListJson) {
        const auto &bundleInfoStr = bundleInfo.get().Key();
        if (!bundleInfoStr.has_value()) {
            LOG(ERROR) << "BundleList: Failed to get bundleInfo val";
            return false;
        }
        std::vector<string> bundleInfoVec;
        SplitStr(bundleInfoStr.value(), " ", bundleInfoVec);
        if (bundleInfoVec.size() != 2) {  // 2: name version
            LOG(ERROR) << "BundleList: Invalid bundleInfoStr: " << bundleInfoStr.value();
            return false;
        }
        BundleInfo &infoTmp = versionInfo.bundleInfoList.emplace_back();
        infoTmp.bundleName = bundleInfoVec.at(0);  // 0:index of bundleName
        infoTmp.bundleVersion = bundleInfoVec.at(1);  // 1:index of bundleVersion
    }
    return true;
}

bool ParseTrainInfo(const JsonNode &package, ModulePackageInfo &versionInfo)
{
    const auto &moduleList = package[HMP_MODULE_INFO];

    for (const auto &moduleInfo : moduleList) {
        ModuleInfo infoTmp;
        const JsonNode &modulePackage = moduleInfo.get()["package"];
        std::optional<string> name = modulePackage["name"].As<string>();
        if (!name.has_value()) {
            LOG(ERROR) << "Hmpinfo: Failed to get name val";
            return false;
        }
        if (!ParseSaList(modulePackage, infoTmp)) {
            return false;
        }
        if (!ParseBundleList(modulePackage, infoTmp)) {
            return false;
        }
        versionInfo.moduleMap.emplace(name.value(), std::move(infoTmp));
    }
    return true;
}

bool ParseModuleInfo(const string &packInfo, ModulePackageInfo &versionInfo)
{
    JsonNode root(packInfo);
    const JsonNode &type = root["type"];
    std::optional<string> hmpType = type.As<string>();
    if (!hmpType.has_value()) {
        LOG(ERROR) << "HmpInfo: Failed to get type val";
        return false;
    }
    versionInfo.type = hmpType.value();

    const JsonNode &package = root["package"];
    if (!ParseHmpVersionInfo(package, versionInfo)) {
        return false;
    }

    if (versionInfo.type == HMP_TRAIN_TYPE) {
        return ParseTrainInfo(package, versionInfo);
    }
    ModuleInfo infoTmp;
    // parse sa info
    if (versionInfo.type == HMP_SA_TYPE || versionInfo.type == HMP_SA_TYPE_OLD ||
        versionInfo.type == HMP_MIX_TYPE) {
        if (!ParseSaList(package, infoTmp)) {
            return false;
        }
    }
    // parse bundle info
    if (versionInfo.type == HMP_APP_TYPE || versionInfo.type == HMP_MIX_TYPE) {
        if (!ParseBundleList(package, infoTmp)) {
            return false;
        }
    }
    versionInfo.moduleMap.emplace(versionInfo.hmpName, std::move(infoTmp));
    return true;
}

// avp a= v>= p>=
bool CompareSaVersion(const SaVersion &smaller, const SaVersion &bigger)
{
    if (smaller.apiVersion != bigger.apiVersion) {
        return false;
    }
    if (smaller.versionCode > bigger.versionCode) {
        return false;
    }
    if (smaller.versionCode < bigger.versionCode) {
        return true;
    }
    return bigger.patchVersion >= smaller.patchVersion;
}

bool CompareSaListVersion(const std::list<SaInfo> &smallList, const std::list<SaInfo> &bigList)
{
    if (smallList.size() != bigList.size()) {
        LOG(ERROR) << "smallList size: " << smallList.size() << " not equal to big: " << bigList.size();
        return false;
    }
    std::unordered_map<int32_t, SaInfo> saMap {};
    for (const auto &info : bigList) {
        saMap.emplace(info.saId, info);
    }
    for (const auto &info : smallList) {
        auto saIter = saMap.find(info.saId);
        if (saIter == saMap.end()) {
            LOG(ERROR) << info.saId << "not found when compare saList";
            return false;
        }
        if (!CompareSaVersion(info.version, saIter->second.version)) {
            return false;
        }
    }
    return true;
}

bool CompareBundleList(const std::list<BundleInfo> &smallList, const std::list<BundleInfo> &bigList)
{
    if (smallList.size() != bigList.size()) {
        LOG(ERROR) << "Bundle smallList size: " << smallList.size() << " not equal to big: " << bigList.size();
        return false;
    }
    std::unordered_set<std::string> bundleSet {};
    for (const auto &info : bigList) {
        bundleSet.insert(info.bundleName);
    }
    for (const auto &info : smallList) {
        auto bundleIter = bundleSet.find(info.bundleName);
        if (bundleIter == bundleSet.end()) {
            LOG(ERROR) << info.bundleName << " not found when compare bundleList";
            return false;
        }
    }
    return true;
}
} // namespace

bool ExtractZipFile(ModuleZipHelper &helper, const string &fpInfo, string &buf)
{
    if (!helper.LocateFile(fpInfo)) {
        LOG(ERROR) << "Could not find " << fpInfo;
        return false;
    }
    if (!helper.GetFileContent(buf)) {
        LOG(ERROR) << "Failed to get content of " << fpInfo;
        return false;
    }
    return true;
}

// MSFB M= S= F= B>
bool CompareHmpVersion(const std::vector<string> &smallVersion, const std::vector<string> &bigVersion)
{
    if (smallVersion.size() != PACKINFO_VERSION_VECTOR_SIZE || bigVersion.size() != PACKINFO_VERSION_VECTOR_SIZE) {
        LOG(ERROR) << "invalid smallVersion " << smallVersion.size() << " invalid bigVersion " << bigVersion.size();
        return false;
    }
    if (smallVersion[0] != bigVersion[0]) {
        LOG(ERROR) << "pre " << smallVersion[0] << " not same as pkg " << bigVersion[0];
        return false;
    }

    int32_t smallVer[HMP_VERSION_TYPE_NUM + 1] = {0};
    int32_t bigVer[HMP_VERSION_TYPE_NUM + 1] = {0};
    for (size_t i = 1; i < HMP_VERSION_TYPE_NUM + 1; i++) {
        if (!Utils::ConvertToLong(smallVersion.at(i), smallVer[i])) {
            LOG(ERROR) << "smallVersion ConvertToLong failed, index: " << i;
            return false;
        }
        if (!Utils::ConvertToLong(bigVersion.at(i), bigVer[i])) {
            LOG(ERROR) << "bigVersion ConvertToLong failed, index: " << i;
            return false;
        }
    }
    if (smallVer[1] == bigVer[1] && // 1: index of M
        smallVer[2] == bigVer[2] && // 2: index of S
        smallVer[3] == bigVer[3] && // 3: index of F
        smallVer[4] < bigVer[4]) { // 4: index of B
        return true;
    }
    return false;
}

// MSFB M= S= F<=
bool CompareSaSdkVersion(const std::vector<string> &smallVersion, const std::vector<string> &bigVersion)
{
    if (smallVersion.size() != PACKINFO_VERSION_VECTOR_SIZE || bigVersion.size() != PACKINFO_VERSION_VECTOR_SIZE) {
        LOG(ERROR) << "invalid smallSaSdk " << smallVersion.size() << " ;invalid bigSaSdk " << bigVersion.size();
        return false;
    }
    if (smallVersion[0] != bigVersion[0]) {
        LOG(ERROR) << "pre " << smallVersion[0] << " not same as pkg " << bigVersion[0];
        return false;
    }

    int32_t smallVer[SA_SDK_VERSION_TYPE_NUM + 1] = {0};
    int32_t bigVer[SA_SDK_VERSION_TYPE_NUM + 1] = {0};
    for (size_t i = 1; i < SA_SDK_VERSION_TYPE_NUM + 1; i++) {
        if (!Utils::ConvertToLong(smallVersion.at(i), smallVer[i])) {
            LOG(ERROR) << "smallVersion ConvertToLong failed, index: " << i;
            return false;
        }
        if (!Utils::ConvertToLong(bigVersion.at(i), bigVer[i])) {
            LOG(ERROR) << "bigVersion ConvertToLong failed, index: " << i;
            return false;
        }
    }
    if (smallVer[1] == bigVer[1] && // 1: index of M
        smallVer[2] == bigVer[2] && // 2: index of S
        smallVer[3] >= bigVer[3]) { // 3: index of F
        return true;
    }
    return false;
}

bool ParseVersion(const string &version, const string &split, std::vector<string> &versionVec)
{
    size_t index = version.rfind(split);
    if (index == std::string::npos) {
        LOG(ERROR) << "ParseVersion failed " << version;
        return false;
    }
    versionVec.emplace_back(version.substr(0, index));
    std::string versionNumber = version.substr(index + split.length());

    std::vector<std::string> tmpVersionVec = Utils::SplitString(versionNumber, "."); // xxx-d01_4.10.0.1
    if (tmpVersionVec.size() != 4) { // 4: version number size
        LOG(ERROR) << version << " is not right";
        return false;
    }
    versionVec.insert(versionVec.end(), tmpVersionVec.begin(), tmpVersionVec.end());
    return true;
}

__attribute__((weak)) int32_t VerifyModulePackageSign(const std::string &fpInfo)
{
    LOG(INFO) << "VerifyModulePackageSign " << fpInfo;
    return VerifyPackage(fpInfo.c_str(), Utils::GetCertName().c_str(), "", nullptr, 0);
}

ModuleFile::~ModuleFile()
{
    ClearVerifiedData();
}

std::unique_ptr<ModuleFile> ModuleFile::Open(const string &fpInfo)
{
    ModuleZipHelper helper(fpInfo);
    if (!helper.IsValid()) {
        LOG(ERROR) << "Failed to open file " << fpInfo;
        return nullptr;
    }

    string moduleInfo;
    if (!ExtractZipFile(helper, PACK_INFO_NAME, moduleInfo)) {
        LOG(ERROR) << "Failed to extract " << PACK_INFO_NAME << " from package " << fpInfo;
        return nullptr;
    }
    ModulePackageInfo versionInfo;
    if (!ParseModuleInfo(moduleInfo, versionInfo)) {
        LOG(ERROR) << "Failed to parse version info of package " << fpInfo;
        return nullptr;
    }

    ImageStat tmpStat;
    std::optional<ImageStat> imageStat;
    string imagePath = ExtractFilePath(fpInfo) + IMG_FILE_NAME;
    if (ParseImageStat(imagePath, tmpStat)) {
        imageStat = std::move(tmpStat);
    } else if (!StartsWith(imagePath, MODULE_PREINSTALL_DIR)) {
        LOG(ERROR) << "Update package without image " << imagePath;
        return nullptr;
    }

    return std::make_unique<ModuleFile>(fpInfo, versionInfo, imageStat);
}

bool ModuleFile::CompareVersion(const ModuleFile &newFile, const ModuleFile &oldFile)
{
    if (newFile.GetPath() == oldFile.GetPath()) {
        return true;
    }
    std::vector<string> newVersion {};
    std::vector<string> oldVersion {};
    // version: xxx-d01 M.S.F.B
    if (!ParseVersion(newFile.GetVersionInfo().version, " ", newVersion) ||
        !ParseVersion(oldFile.GetVersionInfo().version, " ", oldVersion)) {
        LOG(ERROR) << "when compare version, parse version failed.";
        return false;
    }

    if (!CompareHmpVersion(oldVersion, newVersion)) {
        LOG(ERROR) << "old hmp version: " << oldFile.GetVersionInfo().version <<
            "is higher than " << newFile.GetVersionInfo().version;
        return false;
    }
    if (oldFile.GetVersionInfo().moduleMap.size() != newFile.GetVersionInfo().moduleMap.size()) {
        LOG(ERROR) << "old hmp module size: " << oldFile.GetVersionInfo().moduleMap.size() <<
            "; new hmp module size: " << newFile.GetVersionInfo().moduleMap.size();
        return false;
    }
    for (const auto& [key, value] : oldFile.GetVersionInfo().moduleMap) {
        if (newFile.GetVersionInfo().moduleMap.find(key) == newFile.GetVersionInfo().moduleMap.end()) {
            LOG(ERROR) << key << " is not exist in new hmp.";
            return false;
        }
        if (!CompareSaListVersion(value.saInfoList, newFile.GetVersionInfo().moduleMap.at(key).saInfoList)) {
            LOG(ERROR) << "old hmp sa version is higher.";
            return false;
        }
        if (!CompareBundleList(value.bundleInfoList, newFile.GetVersionInfo().moduleMap.at(key).bundleInfoList)) {
            LOG(ERROR) << "new hmp bundle list do not meet expectation.";
            return false;
        }
    }
    return true;
}

bool ModuleFile::ProcessModuleUpdateVerityInfo(const std::string &partition) const
{
#ifdef SUPPORT_HVB
    string imagePath = ExtractFilePath(GetPath()) + IMG_FILE_NAME;
    if (!DealModuleUpdateHvbInfo(imagePath, imageStat_->imageSize, partition)) {
        LOG(ERROR) << "deal with module update partition hvb info fail";
        return false;
    }
    return true;
#else
    LOG(INFO) << "do not support hvb";
    return true;
#endif
}

bool ModuleFile::VerifyModuleVerity()
{
#ifdef SUPPORT_HVB
    if (vd_ != nullptr) {
        LOG(INFO) << "already verified verity";
        return true;
    }
    struct hvb_buf pubkey;
    vd_ = hvb_init_verified_data();
    if (vd_ == nullptr) {
        LOG(ERROR) << "init verified data failed";
        return false;
    }
    ON_SCOPE_EXIT(clear) {
        ClearVerifiedData();
    };
    string imagePath = ExtractFilePath(GetPath()) + IMG_FILE_NAME;
    enum hvb_errno ret = footer_init_desc(ModuleHvbGetOps(), imagePath.c_str(), nullptr, &pubkey, vd_);
    if (ret != HVB_OK) {
        LOG(ERROR) << "hvb verify failed err=" << ret;
        return false;
    }
    CANCEL_SCOPE_EXIT_GUARD(clear);
    return true;
#else
    LOG(INFO) << "do not support hvb";
    return true;
#endif
}

void ModuleFile::ClearVerifiedData()
{
#ifdef SUPPORT_HVB
    if (vd_ != nullptr) {
        hvb_chain_verify_data_free(vd_);
        vd_ = nullptr;
    }
#endif
}
} // namespace SysInstaller
} // namespace OHOS