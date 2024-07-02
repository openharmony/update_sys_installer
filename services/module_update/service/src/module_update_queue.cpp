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

#include <module_update_queue.h>

namespace OHOS {
namespace SysInstaller {
namespace {
constexpr int MAX_SIZE = 100;
}

ModuleUpdateQueue::ModuleUpdateQueue() : size_(0), head_(0), tail_(0)
{
    queue_.resize(MAX_SIZE);
}

void ModuleUpdateQueue::Stop()
{
    isStop_ = true;
    notFull_.notify_all();
    notEmpty_.notify_all();
}

void ModuleUpdateQueue::Put(std::pair<int32_t, std::string> &saStatus)
{
    while (IsFull()) {
        std::unique_lock<std::mutex> locker(mtx_);
        if (isStop_) {
            return;
        }
        notFull_.wait(locker);
    }
    std::unique_lock<std::mutex> locker(mtx_);
    queue_[tail_] = saStatus;
    tail_ = (tail_ + 1) % MAX_SIZE;
    size_++;
    notEmpty_.notify_one();
}

std::pair<int32_t, std::string> ModuleUpdateQueue::Pop()
{
    while (IsEmpty()) {
        std::unique_lock<std::mutex> locker(mtx_);
        if (isStop_) {
            return std::make_pair(0, "");
        }
        notEmpty_.wait(locker);
    }
    std::unique_lock<std::mutex> locker(mtx_);
    std::pair<int32_t, std::string> saStatus = std::move(queue_[head_]);
    head_ = (head_ + 1) % MAX_SIZE;
    size_--;
    notFull_.notify_one();
    return saStatus;
}

bool ModuleUpdateQueue::IsEmpty()
{
    std::unique_lock<std::mutex> locker(mtx_);
    return size_ == 0;
}

bool ModuleUpdateQueue::IsFull()
{
    std::unique_lock<std::mutex> locker(mtx_);
    return size_ == MAX_SIZE;
}
} // SysInstaller
} // namespace OHOS