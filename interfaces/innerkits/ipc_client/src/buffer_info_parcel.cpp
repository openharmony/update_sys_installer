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

#include "buffer_info_parcel.h"

namespace OHOS {
namespace SysInstaller {
bool BufferInfoParcel::Marshalling(Parcel &out) const
{
    RETURN_IF_FALSE(out.WriteUint32(bufferInfo.size));
    RETURN_IF_FALSE(out.WriteBuffer(bufferInfo.buffer, bufferInfo.size));
    return true;
}

BufferInfoParcel* BufferInfoParcel::Unmarshalling(Parcel &in)
{
    BufferInfoParcel* bufferInfoParcel = new (std::nothrow) BufferInfoParcel();
    if (bufferInfoParcel == nullptr) {
        return nullptr;
    }

    bufferInfoParcel->bufferInfo.size = in.ReadUint32();
    bufferInfoParcel->bufferInfo.buffer = in.ReadBuffer(bufferInfoParcel->bufferInfo.size);
    return bufferInfoParcel;
}
} // namespace SysInstaller
} // namespace OHOS