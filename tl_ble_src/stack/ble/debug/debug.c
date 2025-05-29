/********************************************************************************************************
 * @file    debug.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#include "tl_common.h"
#include "stack/ble/ble.h"


_attribute_ble_data_retention_ u32 stkLog_mask = 0;

//_attribute_ble_data_retention_    u32* pErrMarkAddr = 0;
//_attribute_ble_data_retention_    int errMark_len = 0;


void blc_debug_enableStackLog(stk_log_msk_t mask)
{
    stkLog_mask = mask;
}

void blc_debug_addStackLog(stk_log_msk_t mask)
{
    stkLog_mask |= mask;
}

void blc_debug_removeStackLog(stk_log_msk_t mask)
{
    stkLog_mask &= ~mask;
}


#if 0
/**
 * @brief      for user to configure stack error mark log on SRAM
 * @param[in]  enable - 1: enable error mark; 0: disable error mark
 * @param[in]  start_sram_addr - start SRAM address for error mark log. this parameter should be ignored when "enable" is 0
 * @param[in]  byte_length - SRAM byte number. this parameter should be ignored when "enable" is 0
 * @return     none
 */
void blc_debug_configStackErrorMark(int enable, int start_sram_addr, int byte_length)
{
    if(enable){
        pErrMarkAddr = start_sram_addr;
        errMark_len =
    }
    else{
        pErrMarkAddr = 0;
    }
}
#endif


#if (BLT_ERR_PROCESS == ERR_TRIGGER_CODE_STUCK)
_attribute_ram_code_ void blt_ll_error_debug(u32 x)
{
    irq_disable();
    write_dbg32(DBG_SRAM_ADDR, x);

    #if (TLKAPI_DEBUG_ENABLE)
    tlkapi_send_string_data(1, "*** error code mark ***", DBG_SRAM_ADDR, 16);
    #endif


    DBG_CS_CHN6_TOGGLE;
    DBG_CS_CHN6_TOGGLE;
    DBG_CS_CHN6_TOGGLE;
    DBG_CS_CHN6_TOGGLE;

    #if (UI_LED_ENABLE)
        #ifdef GPIO_LED_RED
    gpio_write(GPIO_LED_RED, LED_ON_LEVEL);
        #endif

        #ifdef GPIO_LED_BLUE
    gpio_write(GPIO_LED_BLUE, LED_ON_LEVEL);
        #endif

        #ifdef GPIO_LED_WHITE
    gpio_write(GPIO_LED_WHITE, LED_ON_LEVEL);
        #endif

        #ifdef GPIO_LED_GREEN
    gpio_write(GPIO_LED_GREEN, LED_ON_LEVEL);
        #endif
    #endif

    while (1) {
    #if (VCD_EN)
        static u32 tick = 0;
        if (clock_time_exceed(tick, 500)) {
            tick = clock_time();
            log_event_irq(SL_STACK_VCD_EN, SLEV_timestamp);
        }
    #endif

    #if (TLKAPI_DEBUG_ENABLE)
        tlkapi_debug_handler();
    #endif
    }
}

#elif (BLT_ERR_PROCESS == ERR_LOG_ON_SRAM)


_attribute_ram_code_ void blt_ll_error_debug(u32 x)
{
    write_dbg32(DBG_SRAM_ADDR, x);
}


#endif
