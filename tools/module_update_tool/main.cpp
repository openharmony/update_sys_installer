/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific lan
 * guage governing permissions and
 * limitations under the License.
 */

#include <cstring>

#include "module_update_kits.h"
#include "module_error_code.h"

static const int32_t MIN_PARAM_NUM = 2;
static const int32_t MAX_PARAM_NUM = 3;

static const std::string HELP_MSG =
    "usage: module_update_tool\n"
    "example: ./module_update_tool install /data/tmp/xxx.hmp \n"
    "command list:\n"
    "  install        : upgrade some SA via hmp package"
    "  uninstall      : degrade some SA via hmp name"
    "  show hmpname   : show upgrade sa info, if hmap name is null, show all";

static const std::string INSTALL_PARAM = "install";
static const std::string UNINSTALL_PARAM = "uninstall";
static const std::string SHOW_INFO = "show";
static const int32_t RET_FAILED = -1;

static bool CheckParam(int argc)
{
    if (argc < MIN_PARAM_NUM || argc > MAX_PARAM_NUM) {
        printf("Invalid module update command\n");
        printf("%s", HELP_MSG.c_str());
        return false;
    }
    return true;
}

static std::string GetFailReasonByErrCode(int32_t err)
{
    switch (err) {
        case 0:
            return "success";
        case OHOS::SysInstaller::ERR_SERVICE_NOT_FOUND:
            return "ERR_SERVICE_NOT_FOUND";
        case OHOS::SysInstaller::ERR_INVALID_PATH:
            return "ERR_INVALID_PATH";
        case OHOS::SysInstaller::ERR_LOWER_VERSION:
            return "ERR_LOWER_VERSION";
        case OHOS::SysInstaller::ERR_VERIFY_SIGN_FAIL:
            return "ERR_VERIFY_SIGN_FAIL";
        case OHOS::SysInstaller::ERR_INSTALL_FAIL:
            return "ERR_INSTALL_FAIL";
        case OHOS::SysInstaller::ERR_UNINSTALL_FAIL:
            return "ERR_UNINSTALL_FAIL";
        case OHOS::SysInstaller::ERR_REPORT_STATUS_FAIL:
            return "ERR_REPORT_STATUS_FAIL";
        default:
            return "Unknown Error";
    }
}

static void PrintErrMsg(std::string errMsg)
{
    printf("%s", errMsg.c_str());
    printf("\n");
}

static void PrintUpgradeInfo(std::list<OHOS::SysInstaller::ModulePackageInfo> &modulePackageInfos)
{
    std::list<OHOS::SysInstaller::ModulePackageInfo>::iterator it;
    for (it = modulePackageInfos.begin(); it != modulePackageInfos.end(); it++) {
        OHOS::SysInstaller::ModulePackageInfo moduleInfo = *it;
        printf("%s\n", moduleInfo.hmpName.c_str());
        std::list<OHOS::SysInstaller::SaInfo>::iterator saIt;
        for (saIt = moduleInfo.saInfoList.begin(); saIt != moduleInfo.saInfoList.end(); saIt++) {
            std::string verStr = (*saIt).version;
            printf(" {saName:%s saId:%d version:%s}\n", (*saIt).saName.c_str(), (*saIt).saId, verStr.c_str());
        }
        printf(" \n");
    }
}

int main(int argc, char **argv)
{
    if (!CheckParam(argc)) {
        return RET_FAILED;
    }

    int32_t ret = 0;
    OHOS::SysInstaller::ModuleUpdateKits& moduleUpdateKits = OHOS::SysInstaller::ModuleUpdateKits::GetInstance();
    ret = moduleUpdateKits.InitModuleUpdate();
    if (ret != 0) {
        PrintErrMsg(GetFailReasonByErrCode(ret));
        return ret;
    }

    if (INSTALL_PARAM.compare(argv[1]) == 0 && argc == MAX_PARAM_NUM) {
        printf("try to update a mudule\n");
        ret = moduleUpdateKits.InstallModulePackage(argv[MIN_PARAM_NUM]);
        PrintErrMsg(GetFailReasonByErrCode(ret));
        return ret;
    }
    if (UNINSTALL_PARAM.compare(argv[1]) == 0 && argc == MAX_PARAM_NUM) {
        printf("try to uninstall an upgrade package\n");
        ret = moduleUpdateKits.UninstallModulePackage(argv[MIN_PARAM_NUM]);
        PrintErrMsg(GetFailReasonByErrCode(ret));
        return ret;
    }
    if (SHOW_INFO.compare(argv[1]) == 0) {
        printf("try to show module update info\n");
        std::list<OHOS::SysInstaller::ModulePackageInfo> modulePackageInfos;
        std::string hmpStr;
        if (argc != MIN_PARAM_NUM) {
            hmpStr = argv[MIN_PARAM_NUM];
        }
        const std::string &hmpName = (argc == MIN_PARAM_NUM ? NULL : hmpStr);
        ret = moduleUpdateKits.GetModulePackageInfo(hmpName, modulePackageInfos);
        PrintUpgradeInfo(modulePackageInfos);
        PrintErrMsg(GetFailReasonByErrCode(ret));
        return ret;
    }
    
    printf("invalid command. \n");
    printf("%s", HELP_MSG.c_str());
    return ret;
}