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

#ifndef SYS_INSTALLER_MODULE_ERROR_CODE_H
#define SYS_INSTALLER_MODULE_ERROR_CODE_H

namespace OHOS {
namespace SysInstaller {
enum ModuleErrorCode {
    MODULE_UPDATE_SUCCESS = 0,
    ERR_SERVICE_PARA_ERROR,
    ERR_SERVICE_NOT_FOUND,
    ERR_INVALID_PATH,
    ERR_LOWER_VERSION,
    ERR_VERIFY_SIGN_FAIL,
    ERR_INSTALL_FAIL,
    ERR_UNINSTALL_FAIL,
    ERR_REPORT_STATUS_FAIL
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_ERROR_CODE_H