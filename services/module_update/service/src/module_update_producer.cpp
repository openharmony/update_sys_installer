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

#include "log/log.h"
#include "securec.h"
#include "string_ex.h"
#include <sys/stat.h>
#include <sys/statfs.h>
#include "module_constants.h"
#include "module_ipc_helper.h"
#include "module_update_producer.h"
#include "module_utils.h"
#include "parameter.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;
namespace {
static const std::vector<std::string> saStatusVec {UNLOAD, LOAD_FAIL, CRASH};
constexpr int32_t PARAM_VALUE_SIZE = 10;
constexpr const char *BOOT_COMPLETE_PARAM = "bootevent.boot.completed";
constexpr const char *BOOT_SUCCESS_VALUE = "true";
}

ModuleUpdateProducer::ModuleUpdateProducer(ModuleUpdateQueue &queue,
    std::unordered_map<int32_t, std::string> &saIdHmpMap) : queue_(queue), saIdHmpMap_(saIdHmpMap) {}

void ModuleUpdateProducer::AddAbnormalSa()
{
    char saStatus[PARAM_VALUE_SIZE] = "";
    for (auto it : saIdHmpMap_) {
        int32_t saId = it.first;
        std::string attr = std::string(SA_START_PREFIX) + "." + std::to_string(saId);
        int ret = GetParameter(attr.c_str(), "", saStatus, PARAM_VALUE_SIZE);
        if (ret < 0) {
            LOG(ERROR) << "failed to get parameter " << attr;
            continue;
        }
        if (find(saStatusVec.begin(), saStatusVec.end(), saStatus) != saStatusVec.end()) {
            LOG(INFO) << "add abnormal sa=" << saId << "; status=" << saStatus;
            std::pair<int32_t, std::string> saStatusPair = std::make_pair(saId, saStatus);
            char bootValue[PARAM_VALUE_SIZE] = "";
            int bootVal = GetParameter(BOOT_COMPLETE_PARAM, "", bootValue, PARAM_VALUE_SIZE);
            if (bootVal < 0) {
                LOG(ERROR) << "Failed to get parameter " << BOOT_COMPLETE_PARAM;
                continue;
            }
            if (strcmp(saStatus, UNLOAD) != 0 || (strcmp(saStatus, UNLOAD) == 0 &&
                strcmp(bootValue, BOOT_SUCCESS_VALUE) == 0 &&
                IsHotSa(saId))) {
                queue_.Put(saStatusPair);
            }
            SetParameter(attr.c_str(), "");
        }
        if (strcpy_s(saStatus, PARAM_VALUE_SIZE, "") != EOK) {
            LOG(ERROR) << "fail to strcpy saStatus";
            return;
        }
    }
}

void ModuleUpdateProducer::Run()
{
    LOG(INFO) << "ModuleUpdateProducer Produce";
    char saValue[PARAM_VALUE_SIZE] = "";
    int ret = GetParameter(SA_START, "", saValue, PARAM_VALUE_SIZE);
    if (ret < 0) {
        LOG(ERROR) << "failed to get parameter " << SA_START;
        return;
    }
    do {
        if (strcmp(saValue, SA_ABNORMAL) == 0) {
            SetParameter(SA_START, SA_NORMAL);
            AddAbnormalSa();
        }
        if (strcpy_s(saValue, PARAM_VALUE_SIZE, "") != EOK) {
            LOG(ERROR) << "fail to strcpy saValue";
            return;
        }
        ret = GetParameter(SA_START, "", saValue, PARAM_VALUE_SIZE);
    } while (true);
    LOG(INFO) << "producer exit";
}
} // SysInstaller
} // namespace OHOS