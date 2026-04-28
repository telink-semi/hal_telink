/******************************************************************************
 * Copyright (c) 2023 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/

#include "tl_flash.h"
#include "drivers.h"
#include "flash.h"
#include "flash_prot.h"

 /*******************************************************************************************************************
 *									This function serves to 1m area flash protection
 ******************************************************************************************************************/
#define FLASH_1M_ADR_OFFSET 		0x100000
#define FLASH_1920K_ADR_OFFSET		0x1e0000
#define FLASH_2M_ADR_OFFSET 		0x200000
#define FLASH_4M_ADR_OFFSET 		0x400000

#define FLASH_ADR_OFFSET_SELECT		FLASH_1920K_ADR_OFFSET

uint16_t flash_protection_lock_select(uint32_t addr)
{
	/* FLASH_LOCK_NONE_MID156085 or FLASH_LOCK_NONE_MID1560C8 is 0x0*/
	unsigned int flash_lockBlock_cmd = 0x0;
	unsigned int app_lockBlock = FLASH_LOCK_FW_LOW_1M; 
	unsigned int blc_flash_mid;

	if(addr == FLASH_1M_ADR_OFFSET) {
		app_lockBlock = FLASH_LOCK_FW_LOW_1M;
		flash_lockBlock_cmd = flash_change_app_lock_block_to_flash_lock_block(app_lockBlock);
	}else if (addr == FLASH_2M_ADR_OFFSET || addr == FLASH_4M_ADR_OFFSET){
		app_lockBlock = FLASH_LOCK_ALL_AREA;
		flash_lockBlock_cmd = flash_change_app_lock_block_to_flash_lock_block(app_lockBlock);
	}else if (addr == FLASH_1920K_ADR_OFFSET){
		blc_flash_mid = ble_flash_read_mid();
		if(blc_flash_mid == MID156085){
			flash_lockBlock_cmd = FLASH_LOCK_LOW_1920K_MID156085;
		}else if (blc_flash_mid == MID1560C8){
			flash_lockBlock_cmd = FLASH_LOCK_LOW_1920K_MID1560C8;
		}
	}
	return flash_lockBlock_cmd;
}

void flash_protection_lock_init(void)
{
    flash_protection_init();

    unsigned int flash_lockBlock_cmd = flash_protection_lock_select(FLASH_ADR_OFFSET_SELECT);

    flash_lock(flash_lockBlock_cmd);
}

void flash_protection_lock_operation(unsigned int offset)
{
	/* no need to lock again, detect fw addr will unlock flash. */
	if(offset < FLASH_ADR_OFFSET_SELECT){
		flash_unlock();
	}
}

void flash_protection_unlock_operation(unsigned int offset)
{
	/* suppose we will operate lock area, it will do ota, will unlock first and not lock again until it will reboot. */
	if(offset < FLASH_ADR_OFFSET_SELECT){
		flash_unlock();
	}
}
