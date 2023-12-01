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

#include "sys_installer_callback.h"
#include "sys_installer_kits_impl.h"
#include "sys_installer_sa_ipc_interface_code.h"
#include "isys_installer_callback_func.h"

using namespace OHOS;
using namespace std;
using namespace OHOS::SysInstaller;

class ProcessCallback : public ISysInstallerCallbackFunc {
public:
    ProcessCallback() = default;
    ~ProcessCallback() = default;
    void OnUpgradeProgress(UpdateStatus updateStatus, int percent, const std::string &resultMsg) override
    {
        printf("ProgressCallback progress %d percent %d msg %s\n", updateStatus, percent, resultMsg.c_str());
    }
};

int main(int argc, char **argv)
{
    if (argc != 2) { // 2: max para
        printf("argc para error\n");
        return -1;
    }

    int32_t ret = SysInstallerKitsImpl::GetInstance().SysInstallerInit();
    printf("SysInstallerInit ret:%d\n", ret);

    sptr<ISysInstallerCallbackFunc> callback = new ProcessCallback;
    if (callback == nullptr) {
        printf("callback new failed\n");
        return -1;
    }
    SysInstallerKitsImpl::GetInstance().SetUpdateCallback(callback);

    printf("argv[1]:%d\n", atoi(argv[1]));
    switch (atoi(argv[1])) {
        case SysInstallerInterfaceCode::UPDATE_PACKAGE:
            ret = SysInstallerKitsImpl::GetInstance().StartUpdatePackageZip("/data/ota_package/update.zip");
            break;
        case SysInstallerInterfaceCode::GET_UPDATE_STATUS:
            ret = SysInstallerKitsImpl::GetInstance().GetUpdateStatus();
            break;
        case SysInstallerInterfaceCode::UPDATE_PARA_PACKAGE:
            ret = SysInstallerKitsImpl::GetInstance().StartUpdateParaZip(
                "/data/ota_package/update_para.zip", "System", "/taboo");
            break;
        case SysInstallerInterfaceCode::DELETE_PARA_PACKAGE:
            ret = SysInstallerKitsImpl::GetInstance().StartDeleteParaZip("System", "/taboo");
            break;
        case SysInstallerInterfaceCode::DECOMPRESS_ACC_PACKAGE:
            ret = SysInstallerKitsImpl::GetInstance().AccDecompressAndVerifyPkg(
                "/data/ota_package/update.zip",
                "/data/ota_package/", 1);
            break;
        case SysInstallerInterfaceCode::DELETE_ACC_PACKAGE:
            ret = SysInstallerKitsImpl::GetInstance().AccDeleteDir("/data/ota_package/");
            break;
        default:
            break;
    }

    return ret;
}
