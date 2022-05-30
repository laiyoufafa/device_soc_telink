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
#if 1

#include "stack/ble/ble_config.h"

#include "drivers.h"
#include "tl_common.h"

#include "blt_common.h"
#include "blt_fw_sign.h"
#include "proj_lib/firmware_encrypt.h"

/**
 * @brief		This function is used to check digital signature of firmware
 * @param[in]	none
 * @return      none
 */
void blt_firmware_signature_check(void)
{
		unsigned int flash_mid;
		unsigned char flash_uid[16];
		unsigned char signature_enc_key[16];
		unsigned char signature_flash_key[16];
		extern int flash_read_mid_uid_with_check( unsigned int *flash_mid ,unsigned char *flash_uid);
		int flag = flash_read_mid_uid_with_check(&flash_mid, flash_uid);

		if(flag==0){  //reading flash UID error
			while(1) {
			}
		}

		firmware_encrypt_based_on_uid (flash_uid, signature_enc_key);

		flash_read_page(flash_sector_calibration + CALIB_OFFSET_FIRMWARE_SIGNKEY, 16, signature_flash_key);
		if(memcmp(signature_enc_key, signature_flash_key, 16)){  //signature not match
			while(1) { // user can change the code here to stop firmware running
			}
		}
}
#endif
