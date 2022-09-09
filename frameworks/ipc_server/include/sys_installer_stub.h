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

#ifndef SYS_INSTALLER_STUB_H
#define SYS_INSTALLER_STUB_H

#include <functional>
#include <iostream>
#include <map>
#include "iremote_stub.h"
#include "isys_installer.h"
#include "message_parcel.h"
#include "parcel.h"

namespace OHOS {
namespace SysInstaller {
class SysInstallerStub : public IRemoteStub<ISysInstaller> {
public:
    using RequestFuncType = std::function<int (SysInstallerStub *service,
        MessageParcel &data, MessageParcel &reply, MessageOption &option)>;

    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
        MessageOption &option) override;
    SysInstallerStub();
    virtual ~SysInstallerStub();

private:
    int32_t SysInstallerInitStub(SysInstallerStub *service,
        MessageParcel &data, MessageParcel &reply, MessageOption &option);
    int32_t StartUpdatePackageZipStub(SysInstallerStub *service,
        MessageParcel &data, MessageParcel &reply, MessageOption &option);
    int32_t SetUpdateCallbackStub(SysInstallerStub *service,
        MessageParcel &data, MessageParcel &reply, MessageOption &option);
    int32_t GetUpdateStatusStub(SysInstallerStub *service,
        MessageParcel &data, MessageParcel &reply, MessageOption &option);
    int32_t StartUpdateParaZipStub(SysInstallerStub *service,
        MessageParcel &data, MessageParcel &reply, MessageOption &option);

private:
    std::unordered_map<uint32_t, RequestFuncType> requestFuncMap_ {};
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_STUB_H
