/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef STREAM_INSTALLER_MANAGER_HELPER_H
#define STREAM_INSTALLER_MANAGER_HELPER_H

#include "stream_status_manager.h"

namespace OHOS {
namespace SysInstaller {
class StreamInstallerManagerHelper {
public:
    StreamInstallerManagerHelper() = default;
    virtual ~StreamInstallerManagerHelper() = default;

    virtual int32_t SysInstallerInit();
    virtual int32_t SetUpdateCallback(const sptr<ISysInstallerCallback> &updateCallback);
    virtual int32_t GetUpdateStatus();
    virtual int32_t StartStreamUpdate();
    virtual int32_t StopStreamUpdate();
    virtual int32_t ProcessStreamData(const uint8_t *buffer, size_t size);
    
protected:
    std::shared_ptr<StreamStatusManager> statusManager_ {};
};
} // SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MANAGER_HELPER_H
