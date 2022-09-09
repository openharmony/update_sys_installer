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
#include "out/rk3568/obj/third_party/musl/intermidiates/linux/musl_src_ported/include/unistd.h"
#include "sys_installer_kits_impl.h"

#include "iremote_stub.h"

using namespace OHOS;
using namespace std;
using namespace OHOS::SysInstaller;

class SysInstallerCallbackStub : public IRemoteStub<ISysInstallerCallback> {
public:
    int32_t OnRemoteRequest(uint32_t code,
        MessageParcel &data, MessageParcel &reply, MessageOption &option) override
    {
        if (data.ReadInterfaceToken() != GetDescriptor()) {
            printf("ReadInterfaceToken fail");
            return -1;
        }
        switch (code) {
            case UPDATE_RESULT: {
                int updateStatus = data.ReadInt32();
                int percent  = data.ReadInt32();
                printf("SysInstallerCallbackStub OnUpgradeProgress progress %d percent %d\n", updateStatus, percent);
                break;
            }
            default: {
                return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
            }
        }
        return 0;
    }
};

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
    SysInstallerKitsImpl::GetInstance().SysInstallerInit();
    sleep(3);
    // sptr<ISysInstallerCallback> cb = new SysInstallerCallback;
    // SysInstallerKitsImpl::GetInstance().SetUpdateCallback(cb);
    // SysInstallerKitsImpl::GetInstance().GetUpdateStatus();
    // SysInstallerKitsImpl::GetInstance().StartUpdatePackageZip("/data/ota_package/update.zip");
    // SysInstallerKitsImpl::GetInstance().StartUpdateParaZip(
    //     "/data/ota_package/update_para.zip", "/data/cota/para", "/taboo");
    return 0;
}
