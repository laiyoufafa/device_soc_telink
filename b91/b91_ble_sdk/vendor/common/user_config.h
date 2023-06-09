/******************************************************************************
 * Copyright (c) 2022 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 * All rights reserved.
 *
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
 *
 *****************************************************************************/
#ifndef B91_B91_BLE_SDK_VENDOR_COMMON_USER_CONFIG_H
#define B91_B91_BLE_SDK_VENDOR_COMMON_USER_CONFIG_H

#if (__PROJECT_B91_BLE_SAMPLE__)
#include "../B91_ble_sample/app_config.h"
#elif (__PROJECT_B91_MODULE__)
#include "../B91_module/app_config.h"
#elif (__PROJECT_B91_FEATURE_TEST__)
#include "../B91_feature/app_config.h"
#elif (__PROJECT_B91_EXTERNAL)
#include <app_config.h>
#else

#endif

#endif  // B91_B91_BLE_SDK_VENDOR_COMMON_USER_CONFIG_H
