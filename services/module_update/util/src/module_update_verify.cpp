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

namespace OHOS {
namespace SysInstaller {
using namespace Hpackage;
using namespace Updater;

bool VerifyFileHashSign(PkgManager::PkgManagerPtr pkgManager, HashDataVerifier *verifier,
    std::string &tagBuffer, const std::string &tagName)
{
    if (pkgManager == nullptr || verifier == nullptr) {
        LOG(ERROR) << "pkgManager or verifier is nullptr";
        return false;
    }

    PkgBuffer buffer(reinterpret_cast<uint8_t *>(tagBuffer.data()), tagBuffer.length());
    Hpackage::PkgManager::StreamPtr outStream = nullptr;
    int32_t ret = pkgManager->CreatePkgStream(outStream, tagName, buffer);
    if (outStream == nullptr || ret == -1) {
        LOG(ERROR) << "Create PkgStream Falied!";
        return false;
    }
    ON_SCOPE_EXIT(closeStream) {
        pkgManager->ClosePkgStream(outStream);
    };

    if (!verifier->VerifyHashData("", tagName, outStream)) {
        LOG(ERROR) << "VerifyHashData " << tagName << " failed";
        return false;
    }
    LOG(INFO) << "VerifyHashData " << tagName << " successfull";
    return true;
}

bool LoadHashSignedData(const std::string &packagePath, HashDataVerifier *verifier,
    ModuleZipHelper &helper)
{
    if (verifier == nullptr) {
        LOG(ERROR) << "verifier is nullptr";
        return false;
    }
    // load pkcs7 from package
    if (!verifier->LoadPkcs7FromPackage(packagePath)) {
        LOG(ERROR) << "LoadHashDataAndPkcs7 fail " << packagePath;
        return false;
    }
    std::string hashData {};
    if (!ExtractZipFile(helper, HASH_SIGN_FILE_NAME, hashData)) {
        LOG(ERROR) << "Failed to extract hash_signed_data from package " << packagePath;
        return false;
    }
    if (!verifier->LoadHashDataFromPackage(hashData)) {
        LOG(ERROR) << "LoadHashDataFromPackage fail";
        return false;
    }
    return true;
}

int32_t VerifyPackageHashSign(const std::string &packagePath)
{
    LOG(INFO) << "VerifyPackageHashSign packagePath :"<< packagePath;
    PkgManager::PkgManagerPtr pkgManager = PkgManager::CreatePackageInstance();
    if (pkgManager == nullptr) {
        LOG(ERROR) << "CreatePackageInstance fail";
        return -1;
    }
    ON_SCOPE_EXIT(closePkg) {
        PkgManager::ReleasePackageInstance(pkgManager);
    };

    std::vector<std::string> components;
    int32_t ret = pkgManager->LoadPackage(packagePath, Utils::GetCertName(), components);
    if (ret != PKG_SUCCESS) {
        LOG(ERROR) << "LoadPackage fail ret :"<< ret;
        return ret;
    }

    // 1: init pkcs7
    HashDataVerifier verifier {pkgManager};
    ModuleZipHelper helper(packagePath);
    if (!helper.IsValid()) {
        LOG(ERROR) << "Failed to open file " << packagePath;
        return -1;
    }
    if (!LoadHashSignedData(packagePath, &verifier, helper)) {
        LOG(ERROR) << "LoadHashSignedData fail";
        return -1;
    }

    // 2: verify config.json
    std::string tagBuffer {};
    if (!ExtractZipFile(helper, CONFIG_FILE_NAME, tagBuffer)) {
        LOG(ERROR) << "Failed to extract " << CONFIG_FILE_NAME << " from package";
        return -1;
    }
    if (!VerifyFileHashSign(pkgManager, &verifier, tagBuffer, CONFIG_FILE_NAME)) {
        LOG(ERROR) << "VerifyFileHashSign " << CONFIG_FILE_NAME << " fail";
        return -1;
    }

    // 3: verify pub key
    if (!ExtractZipFile(helper, PUBLIC_KEY_NAME, tagBuffer)) {
        LOG(ERROR) << "Failed to extract " << PUBLIC_KEY_NAME << " from package";
        return -1;
    }
    if (!VerifyFileHashSign(pkgManager, &verifier, tagBuffer, PUBLIC_KEY_NAME)) {
        LOG(ERROR) << "VerifyFileHashSign " << PUBLIC_KEY_NAME << " fail";
        return -1;
    }

    LOG(INFO) << "VerifyPackageHashSign successful packagePath:"<< packagePath;
    return 0;
}

bool GetPackInfoVer(const std::string &packInfoPath, const std::string &key, const std::string &split,
    std::vector<std::string> &versionVec)
{
    if (!Utils::IsFileExist(packInfoPath)) {
        LOG(ERROR) << "GetPackInfoVer " << packInfoPath << " not exist";
        return false;
    }
    JsonNode root(std::filesystem::path { packInfoPath });
    const JsonNode &package = root["package"];
    std::optional<std::string> tmpVersion = package[key].As<std::string>();
    if (!tmpVersion.has_value()) {
        LOG(ERROR) << "count get version val";
        return false;
    }
    if (!ParseVersion(tmpVersion.value(), split, versionVec)) {
        LOG(ERROR) << "ParseVersion failed";
        return false;
    }
    LOG(INFO) << key << " " << tmpVersion.value();
    return true;
}

bool GetDeviceSdkVer(std::vector<std::string> &sdkVersionVec)
{
    // sasdk_4.10.7.9
    std::string sdkVersion = system::GetParameter("const.build.sa_sdk_version", "");
    if (sdkVersion.length() == 0 || !ParseVersion(sdkVersion, "_", sdkVersionVec)) {
        LOG(ERROR) << "ParseVersion failed";
        return false;
    }
    LOG(INFO) << "sa_sdk_version " << sdkVersion;
    return true;
}

int32_t DoCheckPackInfoVer(const std::string &prePackInfoPath, const std::string &pkgPackInfoPath)
{
    std::vector<std::string> preVersion;
    std::vector<std::string> preSdkVersion;
    // version: DUE-d01 4.5.10.100
    // SaSdkVersion: SaSdk_4.10.3.2
    if (!GetPackInfoVer(prePackInfoPath, "version", " ", preVersion) || !GetDeviceSdkVer(preSdkVersion)) {
        LOG(ERROR) << "GetPackInfoVer failed " << prePackInfoPath;
        return -1;
    }

    std::vector<std::string> pkgVersion;
    std::vector<std::string> pkgSdkVersion;
    if (!GetPackInfoVer(pkgPackInfoPath, "version", " ", pkgVersion) ||
        !GetPackInfoVer(pkgPackInfoPath, "SaSdkVersion", "_", pkgSdkVersion)) {
        LOG(ERROR) << "GetPackInfoVer failed " << pkgPackInfoPath;
        return -1;
    }

    if (!ComparePackInfoVer(preVersion, pkgVersion)) {
        LOG(ERROR) << "ComparePackInfoVer version failed";
        return -1;
    }

    if (!ComparePackInfoVer(pkgSdkVersion, preSdkVersion)) {
        LOG(ERROR) << "ComparePackInfoVer sdkversion failed";
        return -1;
    }
    return 0;
}

int32_t CheckPackInfoVer(const std::string &pkgPackInfoPath)
{
    std::string subPackInfoPath = GetHmpName(pkgPackInfoPath) + pkgPackInfoPath.substr(pkgPackInfoPath.rfind("/"));
    const std::string prePackInfoPath = std::string(MODULE_PREINSTALL_DIR) + "/" + subPackInfoPath;
    if (DoCheckPackInfoVer(prePackInfoPath, pkgPackInfoPath) != 0) {
        LOG(ERROR) << "DoCheckPackInfoVer with preinstall fail";
        return -1;
    }

    const std::string actPackInfoPath = std::string(UPDATE_ACTIVE_DIR) + "/" + subPackInfoPath;
    if (pkgPackInfoPath.find(UPDATE_ACTIVE_DIR) != std::string::npos &&
        Utils::IsFileExist(actPackInfoPath) &&
        DoCheckPackInfoVer(actPackInfoPath, pkgPackInfoPath) != 0) {
        LOG(ERROR) << "DoCheckPackInfoVer with active fail";
        return -1;
    }
    return 0;
}

int32_t VerifyPackagePackInfo(const std::string &packagePath, const std::string &packinfo,
    const std::string &hashSignPath)
{
    LOG(INFO) << "VerifyPackagePackInfo packinfo:"<< packinfo << " packagePath:" << packagePath;

    if (!Utils::IsFileExist(packagePath) || !Utils::IsFileExist(packinfo) || !Utils::IsFileExist(hashSignPath)) {
        LOG(ERROR) << "VerifyPackagePackInfo some file not exist";
        return -1;
    }

    PkgManager::PkgManagerPtr manager = PkgManager::CreatePackageInstance();
    if (manager == nullptr) {
        LOG(ERROR) << "CreatePackageInstance fail";
        return -1;
    }
    ON_SCOPE_EXIT(closePkg) {
        PkgManager::ReleasePackageInstance(manager);
    };

    std::vector<std::string> components;
    int32_t ret = manager->LoadPackage(packagePath, Utils::GetCertName(), components);
    if (ret != PKG_SUCCESS) {
        LOG(ERROR) << "LoadPackage fail ret :"<< ret;
        return ret;
    }

    // 1: init pkcs7
    HashDataVerifier verifier {manager};
    // load pkcs7 from package
    if (!verifier.LoadPkcs7FromPackage(packagePath)) {
        LOG(ERROR) << "LoadHashDataAndPkcs7 fail " << packagePath;
        return -1;
    }

    std::ifstream ifs(hashSignPath);
    std::string hashData {std::istreambuf_iterator<char> {ifs}, {}};
    if (!verifier.LoadHashDataFromPackage(hashData)) {
        LOG(ERROR) << "LoadHashDataFromPackage fail";
        return -1;
    }

    std::ifstream ifsp(packinfo);
    std::string tagBuffer {std::istreambuf_iterator<char> {ifsp}, {}};
    if (!VerifyFileHashSign(manager, &verifier, tagBuffer, PACK_INFO_NAME)) {
        LOG(ERROR) << "VerifyFileHashSign " << PACK_INFO_NAME << " fail";
        return -1;
    }

    LOG(INFO) << "VerifyPackagePackInfo successful packagePath:"<< packagePath;
    return 0;
}

int32_t VerifyAndComparePackInfo(const std::string &packagePath, const std::string &packinfo,
    const std::string &hashSignPath)
{
    if (VerifyPackagePackInfo(packagePath, packinfo, hashSignPath) != 0) {
        LOG(INFO) << "VerifyPackagePackInfo failed";
        return -1;
    }

    // 2: check version
    if (CheckPackInfoVer(packinfo) != 0) {
        LOG(ERROR) << "CheckPackInfoVer fail";
        return -1;
    }

    LOG(INFO) << "VerifyAndComparePackInfo successful packagePath:"<< packagePath;
    return 0;
}

void CleanErrDir(const std::string &path)
{
    if (path.find(UPDATE_ACTIVE_DIR) != std::string::npos ||
        path.find(UPDATE_BACKUP_DIR) != std::string::npos) {
        LOG(INFO) << "delete err dir :"<< path;
        ForceRemoveDirectory(path.substr(0, path.rfind("/")));
    }
}
}
}