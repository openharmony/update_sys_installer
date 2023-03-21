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

#ifndef SYS_INSTALLER_MODULE_ZIP_HELPER_H
#define SYS_INSTALLER_MODULE_ZIP_HELPER_H

#include <cstdint>
#include <string>

#include "unzip.h"
#include "zip.h"

namespace OHOS {
namespace SysInstaller {
class ModuleZipHelper {
public:
    explicit ModuleZipHelper(const std::string &path);
    ~ModuleZipHelper();

    bool IsValid() const
    {
        return handle_ != nullptr;
    }
    bool GetNumberOfEntry(uint32_t &number);
    bool LocateFile(const std::string &filename);
    bool GetFileSize(uint32_t &size);
    bool GetFileOffset(uint32_t &offset);
    bool GetFileContent(std::string &buf);
private:
    bool GetFileEntryOffset(uint32_t &offset, uint32_t centralDirOffset) const;
    unzFile handle_ = nullptr;
    bool hasLocated_ = false;
    std::string filename_;
    std::string zipPath_;
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_ZIP_HELPER_H