/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "pkg_verify.h"

#include "log/log.h"
#include "package/cert_verify.h"
#include "package/pkg_manager.h"
#include "scope_guard.h"
#include "utils.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;
using namespace Hpackage;

constexpr size_t PKG_VERIFY_PERCENT = 4;
void PkgVerify::Init()
{
    CertVerify::GetInstance().RegisterCertHelper(std::make_unique<SingleCertHelper>());
}

int PkgVerify::Verify(const std::vector<std::string> &pkgPath)
{
    if (statusManager_ == nullptr) {
        LOG(ERROR) << "statusManager_ nullptr";
        return -1;
    }

    if (!verifyInit_) {
        Init();
        verifyInit_ = true;
    }
    std::vector<std::string> pkgList {};
    for (auto &pkg : pkgPath) {
        if (pkg != "") {
            pkgList.push_back(pkg);
        }
    }
    size_t pkgNum = pkgList.size();
    if (pkgNum == 0) {
        LOG(INFO) << "there is no package";
        return 0;
    }
    size_t count = 0;
    statusManager_->SetUpdatePercent(1); // 1 : 1%
    for (const auto &file : pkgList) {
        count++;
        std::string realpath {};
        if (!Utils::PathToRealPath(file, realpath)) {
            LOG(ERROR) << "get real path failed: " << file;
            return -1;
        }
        int ret = VerifyPackage(realpath.c_str(), Utils::GetCertName().c_str(), "", nullptr, 0);
        if (ret != 0) {
            LOG(ERROR) << "VerifyPackage failed: " << file << ", " << ret;
            return ret;
        }
        int percent = static_cast<int>(
            PKG_VERIFY_PERCENT * static_cast<double>(count) / static_cast<double>(pkgNum));
        statusManager_->SetUpdatePercent(percent + 1); // 1 : 1%
        LOG(INFO) << "VerifyPackage success: " << file;
    }
    statusManager_->SetUpdatePercent(5); // 5 : %5
    LOG(INFO) << "UpdatePreCheck successful";
    return 0;
}

void PkgVerify::PerformAction()
{
    InstallerErrCode errCode = SYS_UPDATE_SUCCESS;
    std::string errStr = "";
    int ret = 0;
    Detail::ScopeGuard guard([&] {
        LOG(INFO) << "PerformAction ret:" << ret;
        if (ret != 0) {
            errCode = SYS_SIGN_VERIFY_FAIL;
            errStr = std::to_string(ret);
        }
        if (actionCallBack_ != nullptr) {
            actionCallBack_(errCode, errStr);
        }
    });

    ret = Verify(pkgPath_);
}
} // namespace SysInstaller
} // namespace OHOS
