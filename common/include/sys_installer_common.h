/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef SYS_INSTALLER_COMMON_H
#define SYS_INSTALLER_COMMON_H

#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>

namespace OHOS {
namespace SysInstaller {
constexpr const char *SYS_LOG_DIR = "/data/updater/log";
constexpr const char *SYS_LOG_FILE = "/data/updater/log/sys_installer.log";
constexpr const char *SYS_STAGE_FILE = "/data/updater/log/sys_installer_stage.log";
constexpr const char *SYS_ERROR_FILE = "/data/updater/log/sys_installer_error_code.log";

enum InstallerErrCode {
    SYS_UPDATE_SUCCESS = 0,
    SYS_PKG_NOT_EXIST,
    SYS_AUTH_CHECK_FAIL = 10,
    SYS_SIGN_VERIFY_FAIL = 20,
    SYS_VERSION_VERIFY_FAIL = 30,
    SYS_PACKAGE_TRANS_FAIL = 40,
    SYS_INSTALL_FAIL = 50,
    SYS_INSTALL_PARA_FAIL = 60,
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_COMMON_H
