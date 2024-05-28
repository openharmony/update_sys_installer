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

#ifndef MODULE_UPDATE_VERIFY_H
#define MODULE_UPDATE_VERIFY_H

#include "cert_verify.h"
#include "directory_ex.h"
#include "hash_data_verifier.h"
#include "module_constants.h"
#include "module_file.h"
#include "module_utils.h"
#include "module_zip_helper.h"
#include "parameters.h"
#include "scope_guard.h"
#include "log/log.h"
#include "json_node.h"
#include "utils.h"

namespace OHOS {
namespace SysInstaller {
using namespace Hpackage;
using namespace Updater;

bool VerifyFileHashSign(PkgManager::PkgManagerPtr pkgManager, HashDataVerifier *verifier,
    std::string &tagBuffer, const std::string &tagName);
bool LoadHashSignedData(const std::string &packagePath, HashDataVerifier *verifier,
    ModuleZipHelper &helper);
int32_t VerifyPackageHashSign(const std::string &packagePath);
bool GetPackInfoVer(const std::string &packInfoPath, const std::string &key, const std::string &split,
    std::vector<std::string> &versionVec);
bool GetDeviceSdkVer(std::vector<std::string> &sdkVersionVec);
int32_t DoCheckPackInfoVer(const std::string &prePackInfoPath, const std::string &pkgPackInfoPath);
int32_t CheckPackInfoVer(const std::string &pkgPackInfoPath);
int32_t VerifyPackagePackInfo(const std::string &packagePath, const std::string &packinfo,
    const std::string &hashSignPath);
int32_t VerifyAndComparePackInfo(const std::string &packagePath, const std::string &packinfo,
    const std::string &hashSignPath);
void CleanErrDir(const std::string &path);
} // namespace SysInstaller
} // namespace OHOS
#endif // MODULE_UPDATE_VERIFY_H