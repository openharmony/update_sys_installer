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

#include "module_dm.h"

#ifdef SUPPORT_HVB
#include "fs_dm.h"
#include "fs_hvb.h"
#include "hvb_cert.h"
#endif

#include "directory_ex.h"
#include "log/log.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

using namespace OHOS::SysInstaller;
using namespace Updater;

#ifdef SUPPORT_HVB
static bool CheckVerifiedData(const struct hvb_verified_data *vd)
{
    if (vd == nullptr) {
        LOG(ERROR) << "verified data is nullptr";
        return false;
    }
    if (vd->num_loaded_certs != 1) {
        LOG(ERROR) << "invalid cert num " << vd->num_loaded_certs;
        return false;
    }
    return true;
}
#endif

bool CreateDmDevice(const OHOS::SysInstaller::ModuleFile &moduleFile, std::string &deviceName)
{
#ifdef SUPPORT_HVB
    struct hvb_verified_data *vd = moduleFile.GetVerifiedData();
    struct hvb_cert cert;
    DmVerityTarget target;
    char *devPath = nullptr;
    std::string devName = OHOS::ExtractFileName(deviceName);
    enum hvb_errno hr = HVB_OK;
    int ret = 0;

    LOG(INFO) << "CreateDmDevice deviceName=" << deviceName;
    if (!CheckVerifiedData(vd)) {
        return false;
    }
    hr = hvb_cert_parser(&cert, &(vd->certs[0].data));
    if (hr != HVB_OK) {
        LOG(ERROR) << "parse cert error " << hr;
        return false;
    }
    ret = FsHvbConstructVerityTarget(&target, deviceName.c_str(), &cert);
    if (ret != 0) {
        LOG(ERROR) << "create dm verity target error " << ret;
        goto exit;
    }
    ret = FsDmCreateDevice(&devPath, devName.c_str(), &target);
    if (ret != 0) {
        LOG(ERROR) << "create dm verity device error " << ret;
        goto exit;
    }
    ret = FsDmInitDmDev(devPath, false);
    if (ret != 0) {
        LOG(ERROR) << "init dm device error " << ret;
        goto exit;
    }
    deviceName = std::string(devPath);
    LOG(INFO) << "Create dm device success. path=" << deviceName;
    free(devPath);

exit:
    FsHvbDestoryVerityTarget(&target);
    return ret == 0;
#else
    LOG(INFO) << "do not support hvb";
    return true;
#endif
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
