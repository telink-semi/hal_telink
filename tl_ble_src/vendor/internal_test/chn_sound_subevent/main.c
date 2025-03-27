/********************************************************************************************************
 * @file    main.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#include "app.h"


#if (INTER_TEST_MODE == TEST_CS_SUBEVENT)

/**
 * @brief       BLE RF interrupt handler.
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void rf_irq_handler(void)
{
    DBG_CHN14_HIGH;

    blc_sdk_irq_handler ();

    DBG_CHN14_LOW;
}
PLIC_ISR_REGISTER(rf_irq_handler, IRQ_ZB_RT)
/**
 * @brief       System timer interrupt handler.
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void stimer_irq_handler(void)
{
    DBG_CHN15_HIGH;

    blc_sdk_irq_handler ();

    cs_subevent_test();

    DBG_CHN15_LOW;
}
PLIC_ISR_REGISTER(stimer_irq_handler, IRQ_SYSTIMER)
#if (GPIO_TRIGGER_TEST_ENABLE)
_attribute_ram_code_ void gpio_irq_handler(void)
{
    gpio_clr_irq_status(FLD_GPIO_IRQ_CLR);

    gpio_trigger_cs_subevent_test();
}
#endif

/**
 * @brief       This is main function
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ int main(void)
{
    /* this function must called before "sys_init()" when:
     * (1). For all IC: using 32K RC for power management,
       (2). For B91 only: even no power management */
    blc_pm_select_internal_32k_crystal();

#if (MCU_CORE_TYPE == MCU_CORE_B91)
    sys_init(DCDC_1P4_LDO_1P8, VBAT_MAX_VALUE_GREATER_THAN_3V6,INTERNAL_CAP_XTAL24M);
#elif (MCU_CORE_TYPE == MCU_CORE_B92)
    sys_init(DCDC_1P4_LDO_2P0, VBAT_MAX_VALUE_GREATER_THAN_3V6, GPIO_VOLTAGE_3V3, INTERNAL_CAP_XTAL24M);
    wd_32k_stop();          //todo: Deep wakeup shall not call wd stop after A1. Jaguar A0 have problem on PM now, so call 32k watchdog stop here now. See <Skype-B91m driver: 2022-10-25>
#endif

    /* detect if MCU is wake_up from deep retention mode */
    int deepRetWakeUp = pm_is_MCU_deepRetentionWakeup();  //MCU deep retention wakeUp

    //CCLK_32M_HCLK_32M_PCLK_16M;
    CCLK_96M_HCLK_48M_PCLK_24M;//for hadm algorithm

    rf_drv_ble_init();

    gpio_init(!deepRetWakeUp);

    if( deepRetWakeUp ){ //MCU wake_up from deepSleep retention mode
        user_init_deepRetn ();
    }
    else{ //MCU power_on or wake_up from deepSleep mode
        user_init_normal();
    }

    #if (MCU_CORE_TYPE == MCU_CORE_B91)

    #elif (MCU_CORE_TYPE == MCU_CORE_B92)
        rf_enable_bb_debug();
    #endif

    #if (0)
        extern void initiator_Tone_PCT_test();
        initiator_Tone_PCT_test();
    #endif
    #if (0)
        extern void initiator_Tone_PCT_test_2();
        initiator_Tone_PCT_test_2();
    #endif
    #if (0)
        extern void distance_estimation_RTT_test();
        distance_estimation_RTT_test();
    #endif
    #if (0)
        extern void estimation_calcPesInfoSDK_test();
        estimation_calcPesInfoSDK_test();
    #endif
    #if (0)
        extern void distance_estimation_Phase_test();
        distance_estimation_Phase_test();
    #endif
    #if (0)
        extern void distance_estimation_Phase_test_2();
        distance_estimation_Phase_test_2();
    #endif

    #if UART_PRINT_DEBUG_ENABLE
        printf("[APP]UART_PRINT,TX_PIN_PF7,BAUD_RATE=%d\n", PRINT_BAUD_RATE);
        printf("[CS]mode-1 internal circuit delay,initiator=%d,reflector=%d\n", blc_ll_cs_getMode1InternalCircuitDelay(CS_PARAM_INITIATOR_ROLE), blc_ll_cs_getMode1InternalCircuitDelay(CS_PARAM_REFLECTOR_ROLE));
    #endif

    irq_enable();

    while(1)
    {
        main_loop ();
    }

    return 0;
}

#endif
