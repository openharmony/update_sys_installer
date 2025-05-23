/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef SYS_INSTALLER_TASK_CONST_H
#define SYS_INSTALLER_TASK_CONST_H
  
namespace OHOS {
namespace SysInstaller {
struct TaskTypeConst {
    static constexpr const char *TASK_TYPE_AB_UPDATE = "ab_update";
};

struct ResultTypeConst {
    static constexpr const char *RESULT_TYPE_INSTALL = "install";
};

struct MetadataActionConst {
    static constexpr const char *NEED_MERGE = "needMerge";
    static constexpr const char *ACTIVE_ROLL_BACK = "activeRollBack";
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_TASK_CONST_H
 