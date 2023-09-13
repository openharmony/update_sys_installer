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

#include "isys_installer_callback_func.h"
#include "isys_installer_callback.h"
#include "module_update_kits.h"
#include "module_error_code.h"

using namespace OHOS;
using namespace OHOS::SysInstaller;

static const int32_t MIN_PARAM_NUM = 2;
static const int32_t MAX_PARAM_NUM = 3;

static const std::string HELP_MSG =
    "usage: module_update_tool\n"
    "example: ./module_update_tool install /data/tmp/xxx.hmp \n"
    "command list:\n"
    "  install         : upgrade some SA via hmp package\n"
    "  uninstall       : degrade some SA via hmp name\n"
    "  update          : upgrade SA via hmp package\n"
    "  get_hmp_version : get hmp package version\n"
    "  get_result      : get hmp upgrade result\n"
    "  show hmpname    : show upgrade sa info, if hmp name is null, show all\n";

static const std::string INSTALL_PARAM = "install";
static const std::string UNINSTALL_PARAM = "uninstall";
static const std::string SHOW_INFO = "show";
static const std::string UPDATE_PARAM = "update";
static const std::string GET_HMP_VERSION = "get_hmp_version";
static const std::string GET_RESULT = "get_result";
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

static void PrintErrMsg(const std::string &errMsg)
{
    printf("%s\n", errMsg.c_str());
}

static void PrintUpgradeInfo(std::list<OHOS::SysInstaller::ModulePackageInfo> &modulePackageInfos)
{
    std::list<OHOS::SysInstaller::ModulePackageInfo>::iterator it;
    printf("Got %zu upgraded modules info\n", modulePackageInfos.size());
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

class ProcessCallback : public ISysInstallerCallbackFunc {
public:
    ProcessCallback() = default;
    ~ProcessCallback() = default;
    void OnUpgradeProgress(UpdateStatus updateStatus, int percent, const std::string &resultMsg) override
    {
        printf("ProgressCallback progress %d percent %d msg %s\n", updateStatus, percent, resultMsg.c_str());
    }
};

static int UpdateModulePackage(const std::string &path)
{
    printf("try to update an upgrade package\n");
    sptr<ISysInstallerCallbackFunc> callback = new ProcessCallback;
    if (callback == nullptr) {
        printf("callback new failed\n");
        return -1;
    }

    int ret = ModuleUpdateKits::GetInstance().StartUpdateHmpPackage(path, callback);
    PrintErrMsg(GetFailReasonByErrCode(ret));
    return ret;
}

static int GetHmpVersion()
{
    printf("try to get hmp version\n");

    std::vector<HmpVersionInfo> versioInfo = ModuleUpdateKits::GetInstance().GetHmpVersionInfo();
    for (auto &info : versioInfo) {
        printf("name:%s laneCode:%s compatibleVersion:%s version:%s\n",
            info.name.c_str(), info.laneCode.c_str(), info.compatibleVersion.c_str(), info.version.c_str());
    }
    return 0;
}

static int GetResult()
{
    printf("try to get hmp result\n");

    std::vector<HmpUpdateInfo> updateInfo = ModuleUpdateKits::GetInstance().GetHmpUpdateResult();
    for (auto &info : updateInfo) {
        printf("path:%s result:%d resultMsg:%s\n", info.path.c_str(), info.result, info.resultMsg.c_str());
    }
    return 0;
}

static int ShowInfo(const std::string &hmpStr)
{
    printf("try to show module update info\n");
    std::list<OHOS::SysInstaller::ModulePackageInfo> modulePackageInfos;
    int ret = OHOS::SysInstaller::ModuleUpdateKits::GetInstance().GetModulePackageInfo(hmpStr, modulePackageInfos);
    PrintUpgradeInfo(modulePackageInfos);
    PrintErrMsg(GetFailReasonByErrCode(ret));
    return ret;
}

int main(int argc, char **argv)
{
    if (!CheckParam(argc)) {
        return RET_FAILED;
    }

    int ret = 0;
    OHOS::SysInstaller::ModuleUpdateKits& moduleUpdateKits = OHOS::SysInstaller::ModuleUpdateKits::GetInstance();
    ret = moduleUpdateKits.InitModuleUpdate();
    if (ret != 0) {
        PrintErrMsg(GetFailReasonByErrCode(ret));
        return ret;
    }

    if (INSTALL_PARAM.compare(argv[1]) == 0 && argc == MAX_PARAM_NUM) {
        printf("try to update a module\n");
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
    if (UPDATE_PARAM.compare(argv[1]) == 0 && argc == MAX_PARAM_NUM) {
        return UpdateModulePackage(argv[MIN_PARAM_NUM]);
    }
    if (GET_HMP_VERSION.compare(argv[1]) == 0) {
        return GetHmpVersion();
    }
    if (GET_RESULT.compare(argv[1]) == 0) {
        return GetResult();
    }
    if (SHOW_INFO.compare(argv[1]) == 0) {
        return ShowInfo((argc != MIN_PARAM_NUM) ? argv[MIN_PARAM_NUM] : "");
    }
    
    printf("invalid command. \n");
    printf("%s", HELP_MSG.c_str());
    return ret;
}