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
#include "app_llmic.h"
#if (INTER_TEST_MODE == TEST_LIVE_MIC_PROJECT)


/**
 * @brief       BLE RF interrupt handler.
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void rf_irq_handler(void)
{
    DBG_CHN14_HIGH;
    DBG_SIHUI_CHN14_HIGH;

    blc_sdk_irq_handler ();

    DBG_CHN14_LOW;
    DBG_SIHUI_CHN14_LOW;
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
    DBG_SIHUI_CHN15_HIGH;

    gpio_write(GPIO_PB6, 1);
    blc_sdk_irq_handler ();
    gpio_write(GPIO_PB6, 0);

    DBG_SIHUI_CHN15_LOW;
    DBG_CHN15_LOW;
}

PLIC_ISR_REGISTER(stimer_irq_handler, IRQ_SYSTIMER)



#define LED1                    GPIO_PF1
#define LED2                    GPIO_PE7
/**
 * @brief       This is main function
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ int main(void)
{
    DBG_CHN0_LOW;

    /* this function must called before "sys_init()" when:
     * (1). For all IC: using 32K RC for power management,
       (2). For B91 only: even no power management */
    blc_pm_select_internal_32k_crystal();

    #if (MCU_CORE_TYPE == MCU_CORE_B92)
        sys_init(DCDC_1P4_LDO_2P0, VBAT_MAX_VALUE_GREATER_THAN_3V6, GPIO_VOLTAGE_3V3, INTERNAL_CAP_XTAL24M);
        wd_32k_stop();
        CCLK_32M_HCLK_32M_PCLK_16M;
    #elif (MCU_CORE_TYPE == MCU_CORE_TL751X)
        sys_init(VBAT_MAX_VALUE_GREATER_THAN_3V6);
        CCLK_96M_HCLK_96M_PCLK_24M_MSPI_48M;
    #endif

    rf_drv_ble_init();
    user_init_normal();
    irq_enable();


    gpio_function_en(LED1);
    gpio_function_en(LED2);
    gpio_output_en(LED1);
    gpio_output_en(LED2);

    while(1)
    {
        main_loop ();

        static u32 cpuRunTime = 0;
        if(clock_time_exceed(cpuRunTime, 500*1000))
        {
            cpuRunTime = clock_time();
            gpio_toggle(LED2);
        }
    }

    return 0;
}



#endif
