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

#ifndef SYS_INSTALLER_MODULE_LOOP_H
#define SYS_INSTALLER_MODULE_LOOP_H

#include <memory>
#include <string>

#include "unique_fd.h"

namespace OHOS {
namespace SysInstaller {
namespace Loop {
class LoopbackDeviceUniqueFd {
public:
    UniqueFd deviceFd;
    std::string name;

    LoopbackDeviceUniqueFd() {}
    LoopbackDeviceUniqueFd(UniqueFd &&fd, const std::string &name)
        : deviceFd(std::move(fd)), name(name) {}

    LoopbackDeviceUniqueFd(LoopbackDeviceUniqueFd &&fd) noexcept
        : deviceFd(std::move(fd.deviceFd)), name(std::move(fd.name)) {}
    LoopbackDeviceUniqueFd &operator=(LoopbackDeviceUniqueFd &&other) noexcept
    {
        MaybeCloseBad();
        deviceFd = std::move(other.deviceFd);
        name = std::move(other.name);
        return *this;
    }

    ~LoopbackDeviceUniqueFd()
    {
        MaybeCloseBad();
    }

    void MaybeCloseBad() const;

    void CloseGood()
    {
        deviceFd.Release();
    }

    int Get() const
    {
        return deviceFd.Get();
    }
};
bool ConfigureReadAhead(const std::string &devicePath);
bool PreAllocateLoopDevices(const size_t num);
std::unique_ptr<LoopbackDeviceUniqueFd> CreateLoopDevice(
    const std::string &target, const uint32_t imageOffset, const uint32_t imageSize);
} // Loop
} // SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_LOOP_H