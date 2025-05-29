/********************************************************************************************************
 * @file    app_detect.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    2020.06
 *
 * @par     Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"


#define RAM_CHECK_SIZE 4

/**
 * @brief       This is detect callback function
 * @param[in]   de_evt - detect event
 * @return      none
 */
_attribute_ram_code_ void app_detect_callback(u32 de_evt)
{
    (void)de_evt;
    start_reboot();
}

/**
 * @brief       This is check RAM Data function
 * @param[in]   de_evt - detect event
 * @return      result: 0 - Success
 *                      1 - error
 */
_attribute_ram_code_ u32 app_detect_data(u32 *vma_start, u32 *vma_end, u32 *lma_start)
{
    for (u32 read_count = 0; read_count < (u32)(vma_end - vma_start); read_count += RAM_CHECK_SIZE) {
        u32 flash_4_byte   = *(volatile u32 *)(lma_start + read_count); //get flash value
        u32 ramcode_4_byte = *(volatile u32 *)(vma_start + read_count); //get ramcode value

        if (flash_4_byte != ramcode_4_byte) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief       This is detect initialization function
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void app_detect_init(void)
{
    //Check whether the contents of flash and ramcode are consistent
    u8 err_ramcode = 0;

    extern u32 _RAMCODE_VMA_START, _RAMCODE_VMA_END, _RAMCODE_LMA_START;
    extern u32 _RETENTION_RESET_VMA_START, _RETENTION_RESET_VMA_END, _RETENTION_RESET_LMA_START;
    err_ramcode |= app_detect_data(&_RAMCODE_VMA_START, &_RAMCODE_VMA_END, &_RAMCODE_LMA_START);
    err_ramcode |= app_detect_data(&_RETENTION_RESET_VMA_START, &_RETENTION_RESET_VMA_END, &_RETENTION_RESET_LMA_START);

    if (err_ramcode) {
        app_detect_callback(1);
    }
}
