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

#include "check_module_update.h"

#include <cstring>
#include "module_update.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

namespace {
constexpr int SA_ARG_COUNT = 2;
constexpr const char *SA_PATH = "/system/bin/sa_main";
}

char *CheckModuleUpdate(int argc, char **argv)
{
    if (argc >= SA_ARG_COUNT && std::strcmp(argv[0], SA_PATH) == 0) {
        OHOS::SysInstaller::ModuleUpdate update;
        std::string updateResult = update.CheckModuleUpdate(argv[1]);
        if (updateResult.empty()) {
            return nullptr;
        }
        size_t length = updateResult.size() + 1;
        char *result = new (std::nothrow) char[length];
        if (result == nullptr) {
            return nullptr;
        }
        errno_t ret = strcpy_s(result, length, updateResult.c_str());
        if (ret != EOK) {
            return nullptr;
        }
        return result;
    }
    return nullptr;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif