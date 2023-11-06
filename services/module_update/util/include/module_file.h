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

#ifndef SYS_INSTALLER_MODULE_FILE_H
#define SYS_INSTALLER_MODULE_FILE_H

#include <memory>
#include <optional>
#include <string>

#ifdef SUPPORT_HVB
#include "hvb.h"
#endif
#include "module_zip_helper.h"
namespace OHOS {
namespace SysInstaller {
namespace {
constexpr size_t FS_TYPE_MAX_SIZE = 10;
}

bool ExtractZipFile(ModuleZipHelper &helper, const std::string &fileName, std::string &buf);
bool ParseVersion(const std::string &version, const std::string &split, std::vector<std::string> &versionVec);
bool ComparePackInfoVer(const std::vector<std::string> &smallVersion, const std::vector<std::string> &bigVersion);
#ifdef __cplusplus
extern "C" {
#endif
int32_t VerifyModulePackageSign(const std::string &path);
#ifdef __cplusplus
}
#endif

struct ModuleVersion {
    uint32_t apiVersion;
    uint32_t versionCode;
    uint32_t patchVersion;

    operator std::string() const
    {
        return std::to_string(apiVersion) + std::to_string(versionCode) + std::to_string(patchVersion);
    }
};

struct ImageStat {
    uint32_t imageOffset;
    uint32_t imageSize;
    char fsType[FS_TYPE_MAX_SIZE];
};

class ModuleFile {
public:
    static std::unique_ptr<ModuleFile> Open(const std::string &path);
    static bool CompareVersion(const ModuleFile &file1, const ModuleFile &file2);
    ModuleFile(const std::string &modulePath,
               const std::string &saName,
               const int32_t saId,
               const ModuleVersion &versionInfo,
               const std::string &modulePubkey,
               const std::optional<ImageStat> &imageStat)
        : modulePath_(modulePath),
          saName_(saName),
          saId_(saId),
          versionInfo_(versionInfo),
          modulePubkey_(modulePubkey),
          imageStat_(imageStat) {}
    virtual ~ModuleFile() = default;
    ModuleFile(const ModuleFile&) = default;
    ModuleFile& operator=(const ModuleFile&) = default;
    ModuleFile(ModuleFile&&) = default;
    ModuleFile& operator=(ModuleFile&&) = default;

    const std::string &GetPath() const
    {
        return modulePath_;
    }
    const std::string &GetSaName() const
    {
        return saName_;
    }
    int32_t GetSaId() const
    {
        return saId_;
    }
    const ModuleVersion &GetVersionInfo() const
    {
        return versionInfo_;
    }
    const std::string &GetPublicKey() const
    {
        return modulePubkey_;
    }
    const std::optional<ImageStat> &GetImageStat() const
    {
        return imageStat_;
    }
    void SetPath(const std::string &path)
    {
        modulePath_ = path;
    }
    bool VerifyModuleVerity(const std::string &publicKey);
    void ClearVerifiedData();
#ifdef SUPPORT_HVB
    struct hvb_verified_data *GetVerifiedData() const
    {
        return vd_;
    }
#endif

private:
    std::string modulePath_;
    std::string saName_;
    int32_t saId_ = 0;
    ModuleVersion versionInfo_;
    std::string modulePubkey_;
    std::optional<ImageStat> imageStat_;

#ifdef SUPPORT_HVB
    struct hvb_verified_data *vd_ = nullptr;
#endif
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_FILE_H