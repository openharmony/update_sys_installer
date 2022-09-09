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

#include "sys_installer_callback_proxy.h"

#include "log/log.h"
#include "securec.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

void SysInstallerCallbackProxy::OnUpgradeProgress(int updateStatus, int percent)
{
    LOG(INFO) << "OnUpgradeProgress";
    MessageParcel data;
    MessageParcel reply;
    MessageOption option { MessageOption::TF_SYNC };

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        LOG(INFO) << "UpdateCallbackProxy WriteInterfaceToken fail";
        return;
    }

    auto remote = Remote();
    if (remote == nullptr) {
        LOG(ERROR) << "Can not get remote";
        return;
    }

    data.WriteInt32(updateStatus);
    data.WriteInt32(percent);
    int32_t result = remote->SendRequest(UPDATE_RESULT, data, reply, option);
    if (result != ERR_OK) {
        LOG(ERROR) << "Can not SendRequest " << result;
    }
}
}
} // namespace OHOS
