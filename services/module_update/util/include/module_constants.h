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
static constexpr int32_t APP_SERIAL_NUMBER = -1;
static constexpr int32_t REGISTER_LEVEL_ONE = 1;
static constexpr int32_t REGISTER_LEVEL_TWO = 2;
static constexpr const char *UPDATE_INSTALL_DIR = "/data/module_update_package";
static constexpr const char *UPDATE_ACTIVE_DIR = "/data/module_update/active";
static constexpr const char *UPDATE_BACKUP_DIR = "/data/module_update/backup";
static constexpr const char *MODULE_PREINSTALL_DIR = "/system/module_update";
static constexpr const char *MODULE_ROOT_DIR = "/module_update";
static constexpr const char *HMP_PACKAGE_SUFFIX = ".zip";
static constexpr const char *MODULE_PACKAGE_SUFFIX = ".zip";
static constexpr const char *MODULE_IMAGE_SUFFIX = ".img";
static constexpr const char *HMP_INFO_NAME = "module_info.zip";
static constexpr const char *IMG_FILE_NAME = "module.img";
static constexpr const char *IMG_DIFF_FILE_NAME = "module.diff";
static constexpr const char *PACK_INFO_NAME = "pack.info";
static constexpr const char *MODULE_RESULT_PATH = "/data/updater/module_update_result";
static constexpr const char *MODULE_UPDATE_LOG_FILE = "/data/updater/log/module_update.log";
static constexpr const char *MODULE_UPDATE_PARAMS_FILE = "/data/updater/module_update_params";
static constexpr const char *SA_ABNORMAL = "true";
static constexpr const char *SA_NORMAL = "false";
static constexpr const char *SA_START = "persist.samgr.moduleupdate.start";
static constexpr const char *SA_START_PREFIX = "persist.samgr.moduleupdate";
static constexpr const char *UNLOAD = "unload";
static constexpr const char *LOAD_FAIL = "loadfail";
static constexpr const char *CRASH = "crash";
static constexpr const char *BMS_START_INSTALL = "persist.moduleupdate.bms.scan";
static constexpr const char *BMS_RESULT_PREFIX = "persist.moduleupdate.bms.install";
static constexpr const char *BMS_UPDATE = "update";
static constexpr const char *BMS_REVERT = "revert";
static constexpr const char *NOTIFY_BMS_REVERT = "revert_bms";
static constexpr const char *BMS_INSTALL_FAIL = "false";
static constexpr const char *HMP_INCR_PACKAGE_TYPE = "increment";
static constexpr const char *HVB_PARTITION_NAME = "module_update";
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_CONSTANTS_H