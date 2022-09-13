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

#include "sys_installer_callback_stub.h"
#include "sys_installer_kits_impl.h"

using namespace OHOS;
using namespace std;
using namespace OHOS::SysInstaller;

class SysInstallerCallback : public SysInstallerCallbackStub {
public:
    SysInstallerCallback() = default;
    ~SysInstallerCallback() = default;

    void OnUpgradeProgress(int updateStatus, int percent)
    {
        printf("SysInstallerCallback OnUpgradeProgress progress %d percent %d\n", updateStatus, percent);
    }
};

int main(int argc, char **argv)
{
    int32_t ret = SysInstallerKitsImpl::GetInstance().SysInstallerInit();
    printf("SysInstallerInit ret:%d\n", ret);
    sptr<ISysInstallerCallback> cb = new SysInstallerCallback;
    SysInstallerKitsImpl::GetInstance().SetUpdateCallback(cb);
    SysInstallerKitsImpl::GetInstance().GetUpdateStatus();
    SysInstallerKitsImpl::GetInstance().StartUpdatePackageZip("/data/ota_package/update.zip");
    SysInstallerKitsImpl::GetInstance().StartUpdateParaZip(
        "/data/ota_package/update_para.zip", "/data/cota/para", "/taboo");
    return 0;
}
