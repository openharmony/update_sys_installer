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

#ifndef SYS_INSTALLER_CALLBACK_PROXY_H
#define SYS_INSTALLER_CALLBACK_PROXY_H

#include "iremote_proxy.h"
#include "isys_installer_callback.h"

namespace OHOS {
namespace SysInstaller {
class SysInstallerCallbackProxy : public IRemoteProxy<ISysInstallerCallback> {
public:
    explicit SysInstallerCallbackProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<ISysInstallerCallback>(impl) {}

    virtual ~SysInstallerCallbackProxy() = default;

    void OnUpgradeProgress(int updateStatus, int percent) override;

private:
    static inline BrokerDelegator<SysInstallerCallbackProxy> delegator_;
};
} // SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_CALLBACK_PROXY_H
