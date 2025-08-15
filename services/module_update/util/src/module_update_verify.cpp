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

#include "module_update_verify.h"
#include "cert_verify.h"
#include "directory_ex.h"
#include "diff_patch/diff_patch_interface.h" // update diff interface
#include "hash_data_verifier.h"
#include "log/log.h"
#include "json_node.h"
#include "openssl/sha.h"
#include "parameters.h"
#include "scope_guard.h"
#include "utils.h"
#include "module_constants.h"
#include "module_file.h"
#include "module_utils.h"

namespace OHOS {
namespace SysInstaller {
using namespace Hpackage;
using namespace Updater;

namespace {
bool GetHmpType(const JsonNode &root, std::string &type)
{
    const JsonNode &typeJson = root["type"];
    std::optional<std::string> hmpType = typeJson.As<std::string>();
    if (!hmpType.has_value()) {
        LOG(ERROR) << "HmpInfo: Failed to get type val";
        return false;
    }
    type = hmpType.value();
    return true;
}

bool CheckApiVersion(const std::string &apiVersion)
{
    if (apiVersion.empty()) {
        LOG(INFO) << "apiVersion is empty, default is true.";
        return true;
    }
    int sysApiVersion = GetDeviceApiVersion();
    int hmpApiVersion = Utils::String2Int<int>(apiVersion, Utils::N_DEC);
    if (hmpApiVersion <= sysApiVersion) {
        return true;
    }
    LOG(ERROR) << "sysApiVersion: " << sysApiVersion << "; hmpApiVersion: " << hmpApiVersion;
    return false;
}

bool CheckSaSdkVersion(const std::string &saSdkVersion)
{
    //sasdk_M.S.F.B
    if (saSdkVersion.empty()) {
        LOG(INFO) << "saSdkVersion is empty, default is true.";
        return true;
    }
    std::vector<std::string> saSdkVersionVec {};
    std::string sysSaSdkVersion = GetDeviceSaSdkVersion();
    if (!ParseVersion(sysSaSdkVersion, "_", saSdkVersionVec)) {
        LOG(ERROR) << "ParseVersion sysSaSdkVersion failed: " << sysSaSdkVersion;
        return false;
    }
    std::vector<std::string> hmpVersionVec {};
    if (!ParseVersion(saSdkVersion, "_", hmpVersionVec)) {
        LOG(ERROR) << "ParseVersion hmpSaSdkVersion failed: " << saSdkVersion;
        return false;
    }
    if (!CompareSaSdkVersion(saSdkVersionVec, hmpVersionVec)) {
        LOG(ERROR) << "saSdkVersion compare fail, sys:" << sysSaSdkVersion << "; hmp:" << saSdkVersion;
        return false;
    }
    return true;
}

bool GetPackInfoVer(const JsonNode &root, const std::string &key, std::string &version)
{
    const JsonNode &package = root["package"];
    std::optional<std::string> tmpVersion = package[key].As<std::string>();
    if (!tmpVersion.has_value()) {
        LOG(ERROR) << "count get version val";
        return false;
    }
    version = tmpVersion.value();
    LOG(INFO) << key << " " << version;
    return true;
}

bool GetPackageType(const JsonNode &root, std::string &type)
{
    const JsonNode &typeJson = root["packageType"];
    std::optional<std::string> hmpPackageType = typeJson.As<std::string>();
    if (!hmpPackageType.has_value()) {
        LOG(ERROR) << "HmpInfo: Failed to get type val";
        return false;
    }
    type = hmpPackageType.value();
    return true;
}
}

bool CheckPackInfoVer(const std::string &pkgPackInfoPath)
{
    std::string packInfo = GetContentFromZip(pkgPackInfoPath, PACK_INFO_NAME);
    JsonNode root(packInfo);
    std::string type;
    if (!GetHmpType(root, type)) {
        return false;
    }
    LOG(INFO) << pkgPackInfoPath << "; type = " << type;
    std::string apiVersion;
    if (type == HMP_APP_TYPE && GetPackInfoVer(root, HMP_API_VERSION, apiVersion)) {
        return CheckApiVersion(apiVersion);
    }
    std::string saSdkVersion;
    if ((type == HMP_SA_TYPE || type == HMP_SA_TYPE_OLD) &&
        GetPackInfoVer(root, HMP_SA_SDK_VERSION, saSdkVersion)) {
        return CheckSaSdkVersion(saSdkVersion);
    }
    if ((type == HMP_MIX_TYPE || type == HMP_TRAIN_TYPE) &&
        GetPackInfoVer(root, HMP_SA_SDK_VERSION, saSdkVersion) &&
        GetPackInfoVer(root, HMP_API_VERSION, apiVersion)) {
        return CheckApiVersion(apiVersion) && CheckSaSdkVersion(saSdkVersion);
    }
    return false;
}

void CleanErrDir(const std::string &fpInfo)
{
    if (fpInfo.find(UPDATE_ACTIVE_DIR) != std::string::npos ||
        fpInfo.find(UPDATE_BACKUP_DIR) != std::string::npos) {
        LOG(INFO) << "delete err dir :"<< fpInfo;
        ForceRemoveDirectory(fpInfo.substr(0, fpInfo.rfind("/")));
    }
}

bool IsIncrementPackage(const std::string &pkgPackInfoPath)
{
    std::string packInfo = GetContentFromZip(pkgPackInfoPath, PACK_INFO_NAME);
    JsonNode root(packInfo);
    std::string packageType;
    if (!GetPackageType(root, packageType)) {
        LOG(INFO) << pkgPackInfoPath << " not support increment";
        return false;
    }
    LOG(INFO) << pkgPackInfoPath << "; packageType = " << packageType;
    if (packageType == HMP_INCR_PACKAGE_TYPE) {
        return true;
    }
    return false;
}

bool ReadHashFromPackInfo(const std::string &pkgPackInfoPath, std::string &hashValue)
{
    std::string packInfo = GetContentFromZip(pkgPackInfoPath, PACK_INFO_NAME);
    JsonNode root(packInfo);
    const JsonNode &imageHashJson = root["imageHash"];
    std::optional<std::string> imageHash = imageHashJson.As<std::string>();
    if (!imageHash.has_value()) {
        LOG(ERROR) << "HmpInfo: Failed to get imageHash val";
        return false;
    }
    hashValue = imageHash.value();
    return true;
}

bool RestorePackage(const std::string &dstFile, const std::string &sourceFile)
{
    LOG(INFO) << "Start restore file " << dstFile << "; source is " << sourceFile;
    Timer timer;
    if (dstFile.find(UPDATE_INSTALL_DIR) == std::string::npos) {
        LOG(ERROR) << dstFile << " is not installDir, restore fail.";
        return false;
    }
    std::string diffFile = ExtractFilePath(dstFile) + IMG_DIFF_FILE_NAME;
    std::string restoreImgFile = ExtractFilePath(dstFile) + IMG_FILE_NAME;
    if (!CheckPathExists(diffFile) || CheckPathExists(restoreImgFile)) {
        LOG(ERROR) << diffFile << " is not exist or dest image exists, restore fail.";
        return false;
    }
    if (!CheckPathExists(sourceFile)) {
        LOG(ERROR) << sourceFile << " is not exist.";
        return false;
    }
    int32_t result = Updater::ApplyPatch(diffFile, sourceFile, restoreImgFile);
    if (result != 0) {
        LOG(ERROR) << "Restore package failed, ret is " << result << " err:" << strerror(errno);
        return false;
    }
    if (unlink(diffFile.c_str()) != 0) {
        LOG(WARNING) << "Failed to unlink " << diffFile << " err:" << strerror(errno);
    }
    LOG(INFO) << "restore image succ, restore timer:" << timer;

    std::string hashValue;
    if (!ReadHashFromPackInfo(dstFile, hashValue)) {
        LOG(ERROR) << "read hash from pack.info fail";
        return false;
    }
    LOG(INFO) << "read hash, " << hashValue;
    std::string calculateHash;
    if (!CalculateSHA256(restoreImgFile, calculateHash)) {
        LOG(ERROR) << "calculate restore image hash fail";
        return false;
    }
    return (hashValue == calculateHash);
}

bool CalculateSHA256(const std::string &filePath, std::string &digest)
{
    char realPath[PATH_MAX] = {0};
    if (realpath(filePath.c_str(), realPath) == nullptr) {
        LOG(ERROR) << "invalid file path, " << filePath;
        return false;
    }
    std::ifstream readFile(realPath, std::ios::binary);
    if (!readFile) {
        LOG(ERROR) << "open file fail, " << realPath;
        return false;
    }
    SHA256_CTX sha256Context;
    SHA256_Init(&sha256Context);
    constexpr int32_t bufferSize = 4096;
    char buffer[bufferSize] = {0};
    while (!readFile.eof()) {
        readFile.read(buffer, sizeof(buffer));
        SHA256_Update(&sha256Context, buffer, readFile.gcount());
    }
    uint8_t digestBuffer[SHA256_DIGEST_LENGTH] = {0};
    SHA256_Final(digestBuffer, &sha256Context);
    digest = Utils::ConvertSha256Hex(digestBuffer, SHA256_DIGEST_LENGTH);
    std::transform(digest.begin(), digest.end(), digest.begin(), ::toupper);
    LOG(INFO) << "CalculateSHA256, " << digest;
    return true;
}
}
}