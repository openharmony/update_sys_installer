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

#include <string>

namespace OHOS {
namespace SysInstaller {
bool CheckPackInfoVer(const std::string &pkgPackInfoPath);
void CleanErrDir(const std::string &fpInfo);
bool IsIncrementPackage(const std::string &pkgPackInfoPath);
bool RestorePackage(const std::string &dstFile, const std::string &sourceFile);
bool ReadHashFromPackInfo(const std::string &pkgPackInfoPath, std::string &hashValue);
bool CalculateSHA256(const std::string &filePath, std::string &digest);
} // namespace SysInstaller
} // namespace OHOS
#endif // MODULE_UPDATE_VERIFY_H