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

#include "module_update.h"
#include "module_update_task.h"
#include "log/log.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;
namespace {
void TaskCallback(ModuleUpdateTask task)
{
    std::string hmpName = task.GetHmpName();
    LOG(INFO) << "module update callback, hmp name=" << hmpName;
    ModuleUpdateStatus status;
    status.hmpName = hmpName;
    status.isHotInstall = false;
    auto ret = ModuleUpdate::GetInstance().DoModuleUpdate(status);
    ModuleUpdateTaskManager::GetInstance().SetTaskResult(ret);
}
}

ModuleUpdateTask::ModuleUpdateTask(const std::string &hmpName)
{
    SetHmpName(hmpName);
}

void ModuleUpdateTask::SetHmpName(const std::string &hmpName)
{
    hmpName_ = hmpName;
}

std::string &ModuleUpdateTask::GetHmpName()
{
    return hmpName_;
}

void ModuleUpdateTaskManager::SetTaskResult(bool result)
{
    taskResult_ = taskResult_ && result;
}

bool ModuleUpdateTaskManager::GetTaskResult()
{
    return taskResult_;
}

void ModuleUpdateTaskManager::ClearTask()
{
    taskNum_ = 0;
}

bool ModuleUpdateTaskManager::AddTask(std::string hmpName)
{
    LOG(INFO) << "add task, hmp name=" << hmpName;
    if (taskNum_ >= MAX_TASK_NUM) {
        LOG(ERROR) << "add task failed:" << taskNum_;
        return false;
    }
    pool_.AddTask([hmpName] {
        ModuleUpdateTask task = ModuleUpdateTask(hmpName);
        TaskCallback(task);
        });
    taskNum_++;
    return true;
}

size_t ModuleUpdateTaskManager::GetCurTaskNum()
{
    return pool_.GetCurTaskNum();
}

void ModuleUpdateTaskManager::Stop()
{
    pool_.Stop();
}

void ModuleUpdateTaskManager::Start()
{
    pool_.Start(1);
    pool_.SetMaxTaskNum(MAX_TASK_NUM);
    taskNum_ = 0;
}
}
}