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

#ifndef SYS_INSTALLER_STREAM_UPDATE_H
#define SYS_INSTALLER_STREAM_UPDATE_H

#include "stream_status_manager.h"
#include "updater/updater.h"
#include "bin_chunk_update.h"
#include "ring_buffer.h"
#include <atomic>

namespace OHOS {
namespace SysInstaller {

class StreamInstallProcesser {
    DISALLOW_COPY_MOVE(StreamInstallProcesser);
public:
    static StreamInstallProcesser &GetInstance();
    void SetStatusManager(std::shared_ptr<StreamStatusManager> statusManager)
    {
        statusManager_ = statusManager;
    }

    bool IsRunning();
    int32_t Start();
    void Stop();
    int32_t ProcessStreamData(const uint8_t *buffer, size_t size);
    void UpdateResult(UpdateStatus updateStatus, int dealLen, const std::string &resultMsg);

private:
    StreamInstallProcesser() = default;
    ~StreamInstallProcesser() = default;
    void ThreadExecuteFunc();
    void ThreadExitProc();

private:
    Updater::RingBuffer ringBuffer_;
    std::shared_ptr<StreamStatusManager> statusManager_ {};
    std::shared_ptr<Updater::BinChunkUpdate> binChunkUpdate_ {};
    std::atomic<bool> isExitThread_ = false;
    std::thread *pComsumeThread_ { nullptr };
    bool isRunning_ = false;
    bool header_processed_ = false;
    uint32_t skip_remaining_ = 0;
    std::vector<uint8_t> partial_data_;
    void ProcessPartialData();
};
} // SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_STREAM_UPDATE_H
