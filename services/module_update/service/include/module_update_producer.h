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

#ifndef MODULE_UPDATE_PRODUCER_H
#define MODULE_UPDATE_PRODUCER_H

#include <csignal>
#include <unordered_set>
#include "module_update_queue.h"

namespace OHOS {
namespace SysInstaller {
class ModuleUpdateProducer {
public:
    ModuleUpdateProducer(
        ModuleUpdateQueue &queue, std::unordered_map<int32_t, std::string> &saIdHmpMap,
        std::unordered_set<std::string> &moduleSet, volatile sig_atomic_t &exit);
    void Run();

private:
    void AddAbnormalSa();
    void AddAbnormalApp();
    ModuleUpdateQueue &queue_;
    std::unordered_map<int32_t, std::string> &saIdHmpMap_;
    std::unordered_set<std::string> &moduleNameSet_;
    volatile sig_atomic_t &exit_;
};
} // SysInstaller
} // namespace OHOS
#endif // MODULE_UPDATE_PRODUCER_H