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

#ifndef SYS_INSTALLER_SA_IPC_INTERFACE_CODE_H
#define SYS_INSTALLER_SA_IPC_INTERFACE_CODE_H

/* SAID: 4101 */
namespace OHOS {
namespace SysInstaller {
enum ModuleUpdateInterfaceCode {
    INSTALL_MODULE_PACKAGE = 1,
    UNINSTALL_MODULE_PACKAGE,
    GET_MODULE_PACKAGE_INFO,
    REPORT_MODULE_UPDATE_STATUS,
    EXIT_MODULE_UPDATE,
    GET_HMP_VERSION_INFO,
    START_UPDATE_HMP_PACKAGE,
    GET_HMP_UPDATE_RESULT
};

enum SysInstallerCallbackInterfaceCode {
    UPDATE_RESULT = 1,
};

enum SysInstallerInterfaceCode {
    SYS_INSTALLER_INIT = 1,
    UPDATE_PACKAGE,
    SET_UPDATE_CALLBACK,
    GET_UPDATE_STATUS,
    UPDATE_PARA_PACKAGE,
    DELETE_PARA_PACKAGE,
    DECOMPRESS_ACC_PACKAGE,
    DELETE_ACC_PACKAGE,
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_IMODULE_UPDATE_H
