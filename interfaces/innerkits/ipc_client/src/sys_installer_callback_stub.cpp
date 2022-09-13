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

#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "log/log.h"
#include "securec.h"
#include "utils.h"

#include "isys_installer_callback.h"
#include "sys_installer_common.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

int32_t SysInstallerCallbackStub::OnRemoteRequest(uint32_t code,
    MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (data.ReadInterfaceToken() != GetDescriptor()) {
        printf("ReadInterfaceToken fail");
        return -1;
    }
    switch (code) {
        case UPDATE_RESULT: {
            int updateStatus = data.ReadInt32();
            int percent  = data.ReadInt32();
            OnUpgradeProgress(updateStatus, percent);
            break;
        }
        default: {
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
    return 0;
}
}
} // namespace OHOS
