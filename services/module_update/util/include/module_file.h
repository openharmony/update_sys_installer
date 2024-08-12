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

#include <list>
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

constexpr int32_t HMP_HOT_TYPE_BITS = 0xf0;  // high bits not 0 means hot
constexpr const char *HMP_APP_TYPE = "APP";
constexpr const char *HMP_SA_TYPE = "systemAbility";
constexpr const char *HMP_SA_TYPE_OLD = "SYSTEM ability";
constexpr const char *HMP_MIX_TYPE = "combineType";
constexpr const char *HMP_API_VERSION = "apiVersion";
constexpr const char *HMP_SA_SDK_VERSION = "saSdkVersion";

bool ExtractZipFile(ModuleZipHelper &helper, const std::string &fileName, std::string &buf);
bool ParseVersion(const std::string &version, const std::string &split, std::vector<std::string> &versionVec);
bool CompareHmpVersion(const std::vector<std::string> &smallVersion, const std::vector<std::string> &bigVersion);
bool CompareSaSdkVersion(const std::vector<std::string> &smallVersion, const std::vector<std::string> &bigVersion);
#ifdef __cplusplus
extern "C" {
#endif
int32_t VerifyModulePackageSign(const std::string &path);
#ifdef __cplusplus
}
#endif

struct SaVersion {
    uint32_t apiVersion;
    uint32_t versionCode;
    uint32_t patchVersion;

    operator std::string() const
    {
        return std::to_string(apiVersion) + std::to_string(versionCode) + std::to_string(patchVersion);
    }
};

struct SaInfo {
    std::string saName;
    int32_t saId {0};
    SaVersion version;
};

struct BundleInfo {
    std::string bundleName;
    std::string bundleVersion;
};

struct ModulePackageInfo {
    std::string hmpName;
    std::string version;
    std::string saSdkVersion;
    std::string type;
    int apiVersion {-1};
    int hotApply {0};
    std::list<SaInfo> saInfoList;
    std::list<BundleInfo> bundleInfoList;
};

struct ImageStat {
    uint32_t imageOffset;
    uint32_t imageSize;
    char fsType[FS_TYPE_MAX_SIZE];
};

enum HmpInstallType {
    COLD_SA_TYPE = 0x01,
    COLD_APP_TYPE,
    COLD_MIX_TYPE,
    HOT_SA_TYPE = 0x10,
    HOT_APP_TYPE,
    HOT_MIX_TYPE
};

class ModuleFile {
public:
    static std::unique_ptr<ModuleFile> Open(const std::string &path);
    static bool CompareVersion(const ModuleFile &newFile, const ModuleFile &oldFile);
    ModuleFile(const std::string &modulePath,
               const ModulePackageInfo &versionInfo,
               const std::string &modulePubkey,
               const std::optional<ImageStat> &imageStat)
        : modulePath_(modulePath),
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
    const ModulePackageInfo &GetVersionInfo() const
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
    HmpInstallType GetHmpPackageType(void) const;
#ifdef SUPPORT_HVB
    struct hvb_verified_data *GetVerifiedData() const
    {
        return vd_;
    }
#endif

private:
    std::string modulePath_;
    ModulePackageInfo versionInfo_;
    std::string modulePubkey_;
    std::optional<ImageStat> imageStat_;

#ifdef SUPPORT_HVB
    struct hvb_verified_data *vd_ = nullptr;
#endif
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_FILE_H