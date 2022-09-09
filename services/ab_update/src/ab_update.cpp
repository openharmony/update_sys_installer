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

#include "ab_update.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "log/log.h"
#include "package/package.h"
#include "pkg_verify.h"
#include "utils.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;
// constexpr const char *CERT_NAME = "/updater/certificate/signing_cert.crt";

int32_t ABUpdate::StartABUpdate(const std::string &pkgPath)
{
    LOG(INFO) << "StartUpdateParaZip start";
    if (statusManager_ == nullptr) {
        LOG(ERROR) << "statusManager_ nullptr";
        return -1;
    }

    statusManager_->UpdateCallback(UPDATE_STATE_INSTALL_SUCCESS, 100); // 100 : success
    return 0;
}

} // namespace SysInstaller
} // namespace OHOS
