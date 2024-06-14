/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef MODULE_UPDATE_CONSUMER_H
#define MODULE_UPDATE_CONSUMER_H

#include "module_ipc_helper.h"
#include "module_update_queue.h"
#include <csignal>
#include <unordered_map>
#include <unordered_set>

namespace OHOS {
namespace SysInstaller {
class ModuleUpdateConsumer {
public:
    ModuleUpdateConsumer(
        ModuleUpdateQueue &queue, std::unordered_map<int32_t, std::string> &saIdHmpMap, volatile sig_atomic_t &exit);
    void Run();

private:
    ModuleUpdateQueue &queue_;
    std::unordered_map<int32_t, std::string> &saIdHmpMap_;
    volatile sig_atomic_t &exit_;
    std::unordered_set<std::string> hmpSet_;
    std::unordered_set<std::string> unLoadHmpSet_;
    void DoRevert(const std::string &hmpName, const int32_t &saId);
    void DoUnload(const std::string &hmpName, const int32_t &saId);
};
} // SysInstaller
} // namespace OHOS
#endif // MODULE_UPDATE_CONSUMER_H