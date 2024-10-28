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

#ifndef MODULE_UPDATE_QUEUE_H
#define MODULE_UPDATE_QUEUE_H

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

namespace OHOS {
namespace SysInstaller {
class ModuleUpdateQueue {
public:
    ModuleUpdateQueue();
    void Put(std::pair<int32_t, std::string> &saStatus);
    std::pair<int32_t, std::string> Pop();
    bool IsEmpty();
    bool IsFull();
    void Stop();
    bool isStop_ = false;

private:
    std::mutex mtx_;
    std::condition_variable notEmpty_;
    std::condition_variable notFull_;
    int size_;
    int head_;
    int tail_;
    std::vector<std::pair<int32_t, std::string>> queue_;
};
} // SysInstaller
} // namespace OHOS
#endif // MODULE_UPDATE_QUEUE_H