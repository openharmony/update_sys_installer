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

#ifndef BUFFER_INFO_PARCEL_H
#define BUFFER_INFO_PARCEL_H
#include "parcel.h"

namespace OHOS {
namespace SysInstaller {
struct BufferInfo final {
public:
    const uint8_t *buffer;
    uint32_t size;
};
struct BufferInfoParcel final : public Parcelable {
    BufferInfoParcel() = default;
    ~BufferInfoParcel() override = default;
    bool Marshalling(Parcel &out) const override;
    static BufferInfoParcel* Unmarshalling(Parcel &in);
    BufferInfo bufferInfo;
};
} // namespace SysInstaller
} // namespace OHOS
#endif // BUFFER_INFO_PARCEL_H