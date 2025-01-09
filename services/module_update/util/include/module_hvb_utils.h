/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef SYS_INSTALLER_MODULE_HVB_UTILS_H
#define SYS_INSTALLER_MODULE_HVB_UTILS_H

#include <string>

#include "hvb.h"
#include "hvb_footer.h"

namespace OHOS {
namespace SysInstaller {
bool DealModuleUpdateHvbInfo(const std::string &imagePath, uint64_t imageSize, const std::string &partition);
bool WriteModuleUpdateBlock(const struct hvb_buf &pubkey, const std::string &partition);
bool GetCertDataFromImage(int fd, uint64_t imageSize, uint64_t offset, uint8_t *pubkey, uint64_t keySize);
bool GetFooterFromImage(int fd, uint64_t imageSize, struct hvb_footer &footer);
bool SetModuleFooterData(struct hvb_footer &footer, uint64_t blockSize, uint64_t cert_size);
uint64_t GetBlockDeviceSize(int fd);
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_HVB_UTILS_H