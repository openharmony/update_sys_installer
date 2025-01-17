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

#ifndef SYS_INSTALLER_MODULE_UTILS_H
#define SYS_INSTALLER_MODULE_UTILS_H

#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include "module_file.h"

namespace OHOS {
namespace SysInstaller {
bool CreateDirIfNeeded(const std::string &path, mode_t mode);
bool CheckPathExists(const std::string &path);
bool CheckFileSuffix(const std::string &file, const std::string &suffix);
std::string GetFileName(const std::string &file);
std::string GetHmpName(const std::string &filePath);
bool WaitForFile(const std::string &path, const std::chrono::nanoseconds &timeout);
bool StartsWith(const std::string &str, const std::string &prefix);
bool ReadFullyAtOffset(int fd, uint8_t *data, size_t count, off_t offset);
bool WriteFullyAtOffset(int fd, const uint8_t *data, size_t count, off_t offset);
uint16_t ReadLE16(const uint8_t *buff);
uint32_t ReadLE32(const uint8_t *buff);
std::string GetRealPath(const std::string &path);
void Revert(const std::string &hmpName, bool reboot);
bool IsHotSa(int32_t saId);
bool IsRunning(int32_t saId);
bool CheckBootComplete(void);
bool RemoveSpecifiedDir(const std::string &path, bool keepDir);
std::string GetDeviceSaSdkVersion(void);
int GetDeviceApiVersion(void);
std::string GetContentFromZip(const std::string &zipPath, const std::string &fileName);
bool CheckAndUpdateRevertResult(const std::string &hmpPath, const std::string &resultInfo, const std::string &keyWord);
std::string GetCurrentHmpName(void);
int32_t NotifyBmsRevert(const std::string &hmpName);

#ifdef __cplusplus
extern "C" {
#endif
bool RevertImageCert(const std::string &hmpName, bool revertMore);
bool VerityInfoWrite(const ModuleFile &file);
void MountModuleUpdateDir(void);
bool PrepareFileToDestDir(const std::string &pkgPath, const std::string &outPath);
void SetModuleVersion(const ModuleFile &file);
#ifdef __cplusplus
}
#endif

class Timer {
public:
    Timer() : start_(std::chrono::steady_clock::now()) {}

    std::chrono::milliseconds duration() const
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_);
    }
private:
    std::chrono::steady_clock::time_point start_;
};
std::ostream &operator<<(std::ostream &os, const Timer &timer);
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_UTILS_H
