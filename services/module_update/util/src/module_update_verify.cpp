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

#include "module_utils.h"
#include "cert_verify.h"
#include "directory_ex.h"
#include "hash_data_verifier.h"
#include "log/log.h"
#include "json_node.h"
#include "module_constants.h"
#include "module_file.h"
#include "parameters.h"
#include "scope_guard.h"
#include "utils.h"

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
    int sysApiVersion = GetDeviceApiVersion();
    int hmpApiVersion = Utils::String2Int<int>(apiVersion);
    if (hmpApiVersion <= sysApiVersion) {
        return true;
    }
    LOG(ERROR) << "sysApiVersion: " << sysApiVersion << "; hmpApiVersion: " << hmpApiVersion;
    return false;
}

bool CheckSaSdkVersion(const std::string &saSdkVersion)
{
    //sasdk_M.S.F.B
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
    return CompareSaSdkVersion(saSdkVersionVec, hmpVersionVec);
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
    if (type == HMP_MIX_TYPE &&
        GetPackInfoVer(root, HMP_SA_SDK_VERSION, saSdkVersion) &&
        GetPackInfoVer(root, HMP_API_VERSION, apiVersion)) {
        return CheckApiVersion(apiVersion) && CheckSaSdkVersion(saSdkVersion);
    }
    return false;
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