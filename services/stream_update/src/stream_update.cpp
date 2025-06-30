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

#include "stream_update.h"

#include "log/log.h"
#include "package/package.h"
#include "package/pkg_manager.h"
#include "scope_guard.h"
#include "securec.h"
#include "updater/updater_const.h"
#include "utils.h"
#include <thread>
#include "updater/updater.h"
#include "slot_info/slot_info.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

constexpr uint32_t BUFFER_SIZE = 50 * 1024;
constexpr uint16_t MAX_RING_BUFFER_NUM = 2;
constexpr uint32_t MAX_UPDATER_BUFFER_SIZE = 2 * BUFFER_SIZE;

StreamInstallProcesser &StreamInstallProcesser::GetInstance()
{
    static StreamInstallProcesser instance;
    return instance;
}

int32_t StreamInstallProcesser::Start()
{
    LOG(INFO) << "StreamInstallProcesser Start";

    // 处理头部数据的时候设置参数，用于ab流式升级
    if (SetUpdateSuffixParam() != UPDATE_SUCCESS) {
        LOG(ERROR) << "SetUpdateSuffixParam failed";
        return -1;
    }

    if (!ringBuffer_.Init(BUFFER_SIZE, MAX_RING_BUFFER_NUM)) {
        LOG(ERROR) << "StreamInstallProcesser Start Init ringBuffer_ failed";
        return -1;
    }

    binChunkUpdate_ = std::make_unique<Updater::BinChunkUpdate>(MAX_UPDATER_BUFFER_SIZE);
    isExitThread_ = false;
    isRunning_ = true;
    pComsumeThread_ = new (std::nothrow) std::thread([this] { this->ThreadExecuteFunc(); });
    if (pComsumeThread_ == nullptr) {
        LOG(ERROR) << "StreamInstallProcesser Start new pComsumeThread_ failed";
        return -1;
    }
    UpdateResult(UpdateStatus::UPDATE_STATE_INIT, 0, "");
    return 0;
}

void StreamInstallProcesser::Stop()
{
    LOG(INFO) << "StreamInstallProcesser Stop enter";
    if (!isRunning_) {
        LOG(WARNING) << "Action not running";
        return;
    }

    isRunning_ = false;
    isExitThread_ = true;
    ringBuffer_.Stop();
    if (pComsumeThread_ != nullptr) {
        pComsumeThread_->join();
        delete pComsumeThread_;
        pComsumeThread_ = nullptr;
    }
    ThreadExitProc();
    LOG(INFO) << "StreamInstallProcesser Stop leave";
    return;
}

bool StreamInstallProcesser::IsRunning()
{
    return isRunning_;
}

void StreamInstallProcesser::ThreadExecuteFunc()
{
    LOG(INFO) << "StreamInstallProcesser ThreadExecuteFunc enter";
    while (!isExitThread_) {
        uint8_t buffer[BUFFER_SIZE]{0};
        uint32_t len = 0;
        uint32_t dealLen = 0;
        if (!ringBuffer_.Pop(buffer, sizeof(buffer), len)) break;
        LOG(INFO) << "start bun chunk update ,len = " << len;
        UpdateResultCode ret = binChunkUpdate_->StartBinChunkUpdate(buffer, len, dealLen);
        if (STREAM_UPDATE_SUCCESS == ret) {
            LOG(INFO) << "StreamInstallProcesser ThreadExecuteFunc STREM_UPDATE_SUCCESS";
            UpdateResult(UpdateStatus::UPDATE_STATE_ONGOING, dealLen, "");
        } else if (STREAM_UPDATE_FAILURE == ret) {
            LOG(ERROR) << "StreamInstallProcesser ThreadExecuteFunc STREM_UPDATE_FAILURE";
            UpdateResult(UpdateStatus::UPDATE_STATE_FAILED, dealLen, "");
            break;
        } else if (STREAM_UPDATE_COMPLETE == ret) {
            LOG(INFO) << "StreamInstallProcesser ThreadExecuteFunc STREM_UPDATE_COMPLETE";
            UpdateResult(UpdateStatus::UPDATE_STATE_SUCCESSFUL, dealLen, "");
            // 升级完成，切换分区
            SetActiveSlot();
            break;
        }
    }
}

void StreamInstallProcesser::ThreadExitProc()
{
    LOG(INFO) << "StreamInstallProcesser ThreadExitProc enter";
    isRunning_ = false;
    isExitThread_ = true;
    ringBuffer_.Stop();
    ringBuffer_.Reset();
}

int32_t StreamInstallProcesser::ProcessStreamData(const uint8_t *buffer, uint32_t size)
{
    uint8_t tmpBuff[BUFFER_SIZE]{0};
    errno_t ret = memcpy_s(tmpBuff, BUFFER_SIZE, buffer, size);
    if (ret != 0) {
        LOG(ERROR) << "ProcessStreamData memcpy_s failed: " << ret;
        return -1;
    }
    return ringBuffer_.Push(tmpBuff, size) ? 0 : -1;
}

void StreamInstallProcesser::UpdateResult(UpdateStatus updateStatus, int dealLen, const std::string &resultMsg)
{
    if (statusManager_ == nullptr) {
        LOG(ERROR) << "statusManager_ nullptr";
        return;
    }
    statusManager_->UpdateCallback(updateStatus, dealLen, resultMsg);
}
} // namespace SysInstaller
} // namespace OHOS
