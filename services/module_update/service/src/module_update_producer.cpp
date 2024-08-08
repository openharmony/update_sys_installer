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
    std::unordered_map<int32_t, std::string> &saIdHmpMap,
    std::unordered_set<std::string> &hmpSet,
    volatile sig_atomic_t &exit)
    : queue_(queue),
      saIdHmpMap_(saIdHmpMap),
      hmpNameSet_(hmpSet),
      exit_(exit) {}

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

void ModuleUpdateProducer::AddAbnormalApp()
{
    char appInstallRes[PARAM_VALUE_SIZE] = "";
    std::size_t resLength = strlen(BMS_INSTALL_FAIL);
    for (const auto &hmpName : hmpNameSet_) {
        std::string attr = std::string(BMS_RESULT_PREFIX) + "." + hmpName;
        if (GetParameter(attr.c_str(), "", appInstallRes, PARAM_VALUE_SIZE) < 0) {
            LOG(ERROR) << "failed to get parameter " << attr;
            continue;
        }
        if (strncmp(appInstallRes, BMS_INSTALL_FAIL, resLength) != 0) {
            (void)memset_s(appInstallRes, PARAM_VALUE_SIZE, 0, PARAM_VALUE_SIZE);
            continue;
        }
        std::pair<int32_t, std::string> appResultPair = std::make_pair(APP_SERIAL_NUMBER, hmpName);
        queue_.Put(appResultPair);
        (void)memset_s(appInstallRes, PARAM_VALUE_SIZE, 0, PARAM_VALUE_SIZE);
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
    char appValue[PARAM_VALUE_SIZE] = "";
    if (GetParameter(BMS_START_INSTALL, "", appValue, PARAM_VALUE_SIZE) < 0) {
        LOG(ERROR) << "failed to get parameter " << BMS_START_INSTALL;
        return;
    }
    do {
        if (exit_ == 1) {
            LOG(INFO) << "producer and consumer stop";
            queue_.Stop();
            break;
        }
        if (strcmp(saValue, SA_ABNORMAL) == 0) {
            SetParameter(SA_START, SA_NORMAL);
            AddAbnormalSa();
        }
        // bms write all hmp install result at once
        if (strcmp(appValue, BMS_REVERT) == 0) {
            SetParameter(BMS_START_INSTALL, NOTIFY_BMS_REVERT);
            AddAbnormalApp();
        }
        (void)memset_s(saValue, PARAM_VALUE_SIZE, 0, PARAM_VALUE_SIZE);
        (void)memset_s(appValue, PARAM_VALUE_SIZE, 0, PARAM_VALUE_SIZE);
        if (GetParameter(SA_START, "", saValue, PARAM_VALUE_SIZE) < 0 ||
            GetParameter(BMS_START_INSTALL, "", appValue, PARAM_VALUE_SIZE) < 0) {
            exit_ = 1;
        }
    } while (true);
    LOG(INFO) << "producer exit";
}
} // SysInstaller
} // namespace OHOS