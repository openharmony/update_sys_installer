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

#ifndef SYS_INSTALLER_MANAGER_H
#define SYS_INSTALLER_MANAGER_H

#include "sys_installer_manager_helper.h"
#include "macros_updater.h"
#include "status_manager.h"

namespace OHOS {
namespace SysInstaller {
class SysInstallerManager {
    DISALLOW_COPY_MOVE(SysInstallerManager);
public:
    void RegisterDump(std::unique_ptr<SysInstallerManagerHelper> ptr);
    static SysInstallerManager &GetInstance();

    virtual int32_t SysInstallerInit(const std::string &taskId);
    virtual int32_t StartUpdatePackageZip(const std::string &taskId, const std::string &pkgPath);
    virtual int32_t SetUpdateCallback(const std::string &taskId, const sptr<ISysInstallerCallback> &updateCallback);
    virtual int32_t GetUpdateStatus(const std::string &taskId);
    virtual int32_t StartUpdateParaZip(const std::string &taskId, const std::string &pkgPath,
        const std::string &location, const std::string &cfgDir);
    virtual int32_t StartDeleteParaZip(const std::string &taskId, const std::string &location,
        const std::string &cfgDir);
    virtual int32_t AccDecompressAndVerifyPkg(const std::string &taskId, const std::string &srcPath,
        const std::string &dstPath, const uint32_t type);
    virtual int32_t AccDeleteDir(const std::string &taskId, const std::string &dstPath);
    virtual int32_t CancelUpdateVabPackageZip(const std::string &taskId);
    virtual int32_t StartUpdateVabPackageZip(const std::string &taskId, const std::vector<std::string> &pkgPath);
    virtual int32_t CreateVabSnapshotCowImg(const std::unordered_map<std::string, uint64_t> &partitionInfo);
    virtual int32_t StartVabMerge(const std::string &taskId);
    virtual int32_t EnableVabCheckpoint();
    virtual int32_t AbortVabActiveSnapshot();
    virtual int32_t ClearVabMetadataAndCow();
    virtual int32_t MergeRollbackReasonFile();
    virtual std::string GetUpdateResult(const std::string &taskId, const std::string &taskType,
        const std::string &resultType);
    virtual int32_t GetMetadataUpdateStatus(int32_t &metadataStatus);
    virtual int32_t VabUpdateActive();
    virtual int32_t GetMetadataResult(const std::string &action, bool &result);
    virtual int32_t StartAbSync();

protected:
    std::unique_ptr<SysInstallerManagerHelper> helper_ {};

private:
    SysInstallerManager() = default;
    ~SysInstallerManager() = default;
};

enum SysInstallerInitEvent {
    SYS_PRE_INIT_EVENT = 0,
    SYS_POST_INIT_EVENT,
    SYS_POST_EVENT,
    SYS_APP_QUICKFIX_EVENT,
    SYS_INIT_EVENT_BUTT
};
using InitHandler = void (*)(void);

class SysInstallerManagerInit {
    DISALLOW_COPY_MOVE(SysInstallerManagerInit);
public:
    static SysInstallerManagerInit &GetInstance()
    {
        static SysInstallerManagerInit instance;
        return instance;
    }
    void InvokeEvent(enum SysInstallerInitEvent eventId) const
    {
        if (eventId >= SYS_INIT_EVENT_BUTT) {
            return;
        }
        for (const auto &handler : initEvent_[eventId]) {
            if (handler != nullptr) {
                handler();
            }
        }
    }
    void SubscribeEvent(enum SysInstallerInitEvent eventId, InitHandler handler)
    {
        if (eventId < SYS_INIT_EVENT_BUTT) {
            initEvent_[eventId].push_back(handler);
        }
    }
private:
    SysInstallerManagerInit() = default;
    ~SysInstallerManagerInit() = default;
    std::vector<InitHandler> initEvent_[SYS_INIT_EVENT_BUTT];
};
} // SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MANAGER_HELPER_H
