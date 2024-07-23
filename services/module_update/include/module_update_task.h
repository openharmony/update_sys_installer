
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
 
#ifndef MODULE_UPDATE_TASK_H
#define MODULE_UPDATE_TASK_H

#include "module_ipc_helper.h"
#include "singleton.h"
#include "thread_pool.h"


namespace OHOS {
namespace SysInstaller {
class ModuleUpdateTask {
public:
    ModuleUpdateTask(const std::string &hmpName);
    void SetHmpName(const std::string &hmpName);
    std::string &GetHmpName();
private:
    std::string hmpName_;
};

class ModuleUpdateTaskManager : public OHOS::Singleton<ModuleUpdateTaskManager> {
    friend class OHOS::Singleton<ModuleUpdateTaskManager>;
public:
    ~ModuleUpdateTaskManager() override {}
    void SetTaskResult(bool result);
    bool GetTaskResult();
    bool AddTask(std::string hmpName);
    void ClearTask();
    void Start();
    void Stop();
    size_t GetCurTaskNum();

private:
    static constexpr size_t MAX_TASK_NUM = 100; // 100 is max task number
    ModuleUpdateTaskManager() {}
    ModuleUpdateTaskManager(const ModuleUpdateTaskManager&) = delete;
    OHOS::ThreadPool pool_;
    bool taskResult_ = true;
    size_t taskNum_;
};
} // SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_UPDATE_H