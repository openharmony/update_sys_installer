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

#ifndef SYS_INSTALLER_MODULE_CONSTANTS_H
#define SYS_INSTALLER_MODULE_CONSTANTS_H

namespace OHOS {
namespace SysInstaller {
static constexpr const char *UPDATE_INSTALL_DIR = "/data/module_update_package";
static constexpr const char *UPDATE_ACTIVE_DIR = "/data/module_update/active";
static constexpr const char *UPDATE_BACKUP_DIR = "/data/module_update/backup";
static constexpr const char *MODULE_PREINSTALL_DIR = "/system/module_update";
static constexpr const char *MODULE_ROOT_DIR = "/module_update";
static constexpr const char *HMP_PACKAGE_SUFFIX = ".zip";
static constexpr const char *MODULE_PACKAGE_SUFFIX = ".zip";
static constexpr const char *CONFIG_FILE_NAME = "config.json";
static constexpr const char *IMG_FILE_NAME = "module.img";
static constexpr const char *PUBLIC_KEY_NAME = "pub_key.pem";
static constexpr const char *PACK_INFO_NAME = "pack.info";
static constexpr const char *MODULE_RESULT_PATH = "/data/updater/module_update_result";
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_CONSTANTS_H