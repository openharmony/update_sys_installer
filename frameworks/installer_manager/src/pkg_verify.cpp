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
#include "utils.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;
using namespace Hpackage;

constexpr const char *CERT_NAME = "/updater/certificate/signing_cert.crt";

void PkgVerify::Init()
{
    CertVerify::GetInstance().RegisterCertHelper(std::make_unique<SingleCertHelper>());
}

int PkgVerify::UpdatePreCheck(const std::string &pkgPath)
{
    if (statusManager_ == nullptr) {
        LOG(ERROR) << "statusManager_ nullptr";
        return -1;
    }

    if (!verifyInit_) {
        Init();
        verifyInit_ = true;
    }

    statusManager_->UpdateCallback(UPDATE_STATE_ONGOING, 1); // 1 : 1%
    int ret = VerifyPackage(pkgPath.c_str(), CERT_NAME, "", nullptr, 0);
    if (ret != 0) {
        LOG(ERROR) << "VerifyPackage failed:" << ret;
        statusManager_->UpdateCallback(UPDATE_STATE_ONGOING, 1); // 1 : 1%
        return -1;
    }

    statusManager_->UpdateCallback(UPDATE_STATE_ONGOING, 5); // 5 : %5
    LOG(INFO) << "UpdatePreCheck successful";
    return 0;
}
} // namespace SysInstaller
} // namespace OHOS
