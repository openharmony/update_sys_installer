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
#include <cstring>
#include "module_update.h"
#include "log/log.h"
#include "securec.h"

using namespace OHOS;
using namespace Updater;

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

int main(int argc, char **argv)
{
    LOG(INFO) << "enter module update main";
    auto &instance = OHOS::SysInstaller::ModuleUpdate::GetInstance();
    if (argc <= 1) {
        instance.CheckModuleUpdate();
        return 0;
    }
    instance.HandleExtraArgs(argc, argv);
    return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif