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

namespace OHOS {
namespace SysInstaller {
using namespace Updater;

constexpr uint32_t BUFFER_SIZE = 50 * 1024;
constexpr uint16_t MAX_RING_BUFFER_NUM = 2;
constexpr uint32_t MAX_UPDATER_BUFFER_SIZE = 2 * BUFFER_SIZE;
constexpr uint16_t TOTAL_TL_BYTES = 6;
constexpr uint16_t ONE_BYTE = 8;
constexpr uint8_t ZIP_HEADER_TLV_TYPE = 0xaa;

StreamInstallProcesser &StreamInstallProcesser::GetInstance()
{
    static StreamInstallProcesser instance;
    return instance;
}

uint16_t ReadLE16(const uint8_t *buff, size_t length)
{
    if (buff == nullptr) {
        LOG(ERROR) << "buff is null";
        return 0;
    }
    if (length < sizeof(uint16_t)) {
        LOG(ERROR) << "Buffer length is insufficient for ReadLE16";
    }
    uint16_t value16 = buff[0];
    value16 += static_cast<uint16_t>(buff[1] << ONE_BYTE);
    return value16;
}

uint32_t ReadLE32(const uint8_t *buff, size_t length)
{
    if (buff == nullptr) {
        LOG(ERROR) << "buff is null";
        return 0;
    }
    if (length < sizeof(uint32_t)) {
        LOG(ERROR) << "Buffer length is insufficient for ReadLE32";
    }
    uint16_t low = ReadLE16(buff, length);
    uint16_t high = ReadLE16(buff + sizeof(uint16_t), length - sizeof(uint16_t));
    uint32_t value = ((static_cast<uint32_t>(high)) << (ONE_BYTE * sizeof(uint16_t))) | low;
    return value;
}

int32_t StreamInstallProcesser::Start()
{
    LOG(INFO) << "StreamInstallProcesser Start";

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
    UpdateResult(UPDATE_STATE_INIT, 0, "");
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
        uint8_t temporaryBuffer[BUFFER_SIZE]{0};
        uint32_t len = 0;
        if (!ringBuffer_.Pop(temporaryBuffer, sizeof(temporaryBuffer), len)) {
            if (!partial_data_.empty()) {
                ProcessPartialData();
            }
            break;
        }

        partial_data_.insert(partial_data_.end(), temporaryBuffer, temporaryBuffer + len);
        ProcessPartialData();
    }
}

bool StreamInstallProcesser::ProcessHeader()
{
    if (header_processed_) {
        return true;
    }

    if (partial_data_.size() < TOTAL_TL_BYTES) {
        return false;
    }
    
    const uint16_t type = ReadLE16(partial_data_.data(), partial_data_.size());
    if (type != ZIP_HEADER_TLV_TYPE) {
        header_processed_ = true;
        LOG(ERROR) << "No match type: " << type;
        return true;
    }
    const uint32_t length = ReadLE32(partial_data_.data() + 2, partial_data_.size() - 2);
    LOG(INFO) << "Length = " << length;
    skip_remaining_ = length;
    partial_data_.erase(partial_data_.begin(), partial_data_.begin() + TOTAL_TL_BYTES);
    header_processed_ = true;

    return true;
}

bool StreamInstallProcesser::SkipTargetData()
{
    if (skip_remaining_ <= 0) {
        LOG(ERROR) << "no valid skip_remaining_ = " << skip_remaining_;
        return false; // 不需要跳过数据，直接返回
    }
    const size_t skip = std::min<size_t>(skip_remaining_, partial_data_.size());
    partial_data_.erase(partial_data_.begin(), partial_data_.begin() + skip);
    skip_remaining_ -= skip;
    LOG(INFO) << "skip_remaining_ = " << skip_remaining_;
    return true; // 跳过数据后继续循环
}

bool StreamInstallProcesser::ProcessValidData()
{
    const size_t process_size = std::min<size_t>(partial_data_.size(), BUFFER_SIZE);
    if (process_size == 0) {
        return true; // 无数据可处理，退出循环
    }
    uint8_t buffer[BUFFER_SIZE]{0};
    errno_t ret = memcpy_s(buffer, BUFFER_SIZE, partial_data_.data(), process_size);
    if (ret != 0) {
        LOG(ERROR) << "ProcessStreamData memcpy_s failed: " << ret;
        return false;
    }
    partial_data_.erase(partial_data_.begin(), partial_data_.begin() + process_size);
    uint32_t dealLen = 0;
    const UpdateResultCode ret_update = binChunkUpdate_->StartBinChunkUpdate(buffer, process_size, dealLen);
    if (ret_update == STREAM_UPDATE_SUCCESS) {
        LOG(INFO) << "StreamInstallProcesser ThreadExecuteFunc STREAM_UPDATE_SUCCESS";
        UpdateResult(UPDATE_STATE_ONGOING, dealLen, "");
    } else if (ret_update == STREAM_UPDATE_FAILURE) {
        LOG(ERROR) << "StreamInstallProcesser ThreadExecuteFunc STREAM_UPDATE_FAILURE";
        UpdateResult(UPDATE_STATE_FAILED, dealLen, "");
        isExitThread_ = true;
    } else if (ret_update == STREAM_UPDATE_COMPLETE) {
        LOG(INFO) << "StreamInstallProcesser ThreadExecuteFunc STREAM_UPDATE_COMPLETE";
        UpdateResult(UPDATE_STATE_SUCCESSFUL, dealLen, "");
        isExitThread_ = true;
    }
    return isExitThread_;
}

void StreamInstallProcesser::ProcessPartialData()
{
    while (!partial_data_.empty() && !isExitThread_) {
        // 阶段1：处理头部数据
        if (!ProcessHeader()) {
            break;
        }

        // 阶段2：跳过目标数据
        if (SkipTargetData()) {
            continue;
        }

        // 阶段3：处理有效数据
        if (ProcessValidData()) {
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

int32_t StreamInstallProcesser::ProcessStreamData(const uint8_t *buffer, size_t size)
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
