/******************************************************************************
 * Copyright (c) 2023 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

#ifndef HDF_LOG_ADAPTER_DEBUG_H
#define HDF_LOG_ADAPTER_DEBUG_H

#define HDF_LOGV(fmt, args...) printf(fmt "\n", ##args)
#define HDF_LOGD(fmt, args...) printf(fmt "\n", ##args)
#define HDF_LOGI(fmt, args...) printf(fmt "\n", ##args)
#define HDF_LOGW(fmt, args...) printf(fmt "\n", ##args)
#define HDF_LOGE(fmt, args...) printf("[ HDF ERROR ] " fmt "\n", ##args)

#endif /* HDF_LOG_ADAPTER_DEBUG_H */

