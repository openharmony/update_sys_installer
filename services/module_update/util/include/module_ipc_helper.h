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

#ifndef SYS_INSTALLER_MODULE_IPC_HELPER_H
#define SYS_INSTALLER_MODULE_IPC_HELPER_H

#include <functional>
#include <list>

#include "message_parcel.h"
#include "module_file.h"

namespace OHOS {
namespace SysInstaller {
static constexpr int32_t IPC_MIN_SIZE = 0;
static constexpr int32_t IPC_MAX_SIZE = 128;

struct ModuleUpdateStatus {
    std::string hmpName;
    bool isPreInstalled {false};
    bool isAllMountSuccess {false};
};

class ModuleIpcHelper {
public:
    static int32_t ReadModulePackageInfos(MessageParcel &reply, std::list<ModulePackageInfo> &infos);
    static int32_t WriteModulePackageInfos(MessageParcel &data, const std::list<ModulePackageInfo> &infos);
    static int32_t ReadModuleUpdateStatus(MessageParcel &reply, ModuleUpdateStatus &status);
    static int32_t WriteModuleUpdateStatus(MessageParcel &data, const ModuleUpdateStatus &status);

    template<typename T>
    static void ReadList(MessageParcel &reply, std::list<T> &list,
        const std::function<void(MessageParcel &, T &)> &read)
    {
        int32_t size = reply.ReadInt32();
        if (size > IPC_MAX_SIZE || size < IPC_MIN_SIZE) {
            return;
        }
        for (int32_t i = 0; i < size; ++i) {
            T item;
            read(reply, item);
            list.emplace_back(std::move(item));
        }
    }

    template<typename T>
    static void WriteList(MessageParcel &data, const std::list<T> &list,
        const std::function<void(MessageParcel &, const T &)> &write)
    {
        data.WriteInt32(static_cast<int32_t>(list.size()));
        for (auto &iter : list) {
            write(data, iter);
        }
    }
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_IPC_HELPER_H