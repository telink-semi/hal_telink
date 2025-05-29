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

#if (INTER_TEST_MODE == TEST_RAS_CLIENT)

/**
 * @brief       BLE RF interrupt handler.
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void rf_irq_handler(void)
{
    DBG_CHN14_HIGH;
    DBG_CS_CHN14_HIGH;

    blc_sdk_irq_handler();

    DBG_CHN14_LOW;
    DBG_CS_CHN14_LOW;
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
    DBG_CS_CHN15_HIGH;

    blc_sdk_irq_handler();

    DBG_CHN15_LOW;
    DBG_CS_CHN15_LOW;
}
PLIC_ISR_REGISTER(stimer_irq_handler, IRQ_SYSTIMER)

/**
 * @brief       timer0 interrupt handler.
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void timer0_irq_handler(void)
{
#if (MCU_CORE_TYPE == MCU_CORE_TL721X)
    if (timer_get_irq_status(FLD_TMR0_MODE_IRQ))
#else
    if (timer_get_irq_status(TMR_STA_TMR0))
#endif
    {
        DBG_CS_CHN4_HIGH;
        u32 r = core_interrupt_disable();
        reg_tmr_ctrl0 &= ~FLD_TMR0_EN;
#if (MCU_CORE_TYPE == MCU_CORE_TL721X)
        timer_clr_irq_status(FLD_TMR0_MODE_IRQ); //clear irq status
#else
        timer_clr_irq_status(TMR_STA_TMR0); //clear irq status
#endif
        core_restore_interrupt(r);

        if (ll_cs_rawData_process_cb) {
            ll_cs_rawData_process_cb();
        }
        if (ll_cs_hci_subevent_report_cb) {
            ll_cs_hci_subevent_report_cb();
        }
        DBG_CS_CHN4_LOW;
    }
}
PLIC_ISR_REGISTER(timer0_irq_handler, IRQ_TIMER0)

/**
 * @brief      application system initialization
 * @param[in]  none.
 * @return     none.
 */
__INLINE void blc_app_system_init(void)
{
#if (MCU_CORE_TYPE == MCU_CORE_B91)
    sys_init(DCDC_1P4_LDO_1P8, VBAT_MAX_VALUE_GREATER_THAN_3V6, INTERNAL_CAP_XTAL24M);
    gpio_set_up_down_res(GPIO_SWS, GPIO_PIN_PULLUP_1M);
    wd_stop();
    CCLK_48M_HCLK_48M_PCLK_24M;
#elif (MCU_CORE_TYPE == MCU_CORE_B92)
    sys_init(DCDC_1P4_LDO_2P0, VBAT_MAX_VALUE_GREATER_THAN_3V6, GPIO_VOLTAGE_3V3, INTERNAL_CAP_XTAL24M);
    pm_update_status_info(1);
    gpio_set_up_down_res(GPIO_SWS, GPIO_PIN_PULLUP_1M);
    wd_32k_stop();
    CCLK_96M_HCLK_48M_PCLK_24M;
#elif (MCU_CORE_TYPE == MCU_CORE_TL721X)
    sys_init(DCDC_0P94_LDO_1P8, VBAT_MAX_VALUE_GREATER_THAN_3V6, INTERNAL_CAP_XTAL24M);
    pm_update_status_info(1);
    gpio_set_up_down_res(GPIO_SWS, GPIO_PIN_PULLUP_1M);
    wd_32k_stop();
    wd_stop();
    PLL_240M_CCLK_120M_HCLK_60M_PCLK_60M_MSPI_48M;
#elif (MCU_CORE_TYPE == MCU_CORE_TL321X)
    sys_init(DCDC_1P25_LDO_1P8, VBAT_MAX_VALUE_GREATER_THAN_3V6, INTERNAL_CAP_XTAL24M);
    pm_update_status_info(1);
    gpio_set_up_down_res(GPIO_SWS, GPIO_PIN_PULLUP_1M);
    wd_32k_stop();
    wd_stop();
    PLL_192M_CCLK_48M_HCLK_24M_PCLK_24M_MSPI_48M;
#elif (MCU_CORE_TYPE == MCU_CORE_TL751X)
    sys_init(VBAT_MAX_VALUE_GREATER_THAN_3V6);
    wd_32k_stop();
    wd_stop();
    CCLK_96M_HCLK_96M_PCLK_24M_MSPI_48M;
#endif
}

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

    blc_app_system_init();

    /* detect if MCU is wake_up from deep retention mode */
    int deepRetWakeUp = pm_is_MCU_deepRetentionWakeup(); //MCU deep retention wakeUp

    rf_drv_ble_init();

    gpio_init(!deepRetWakeUp);

    if (deepRetWakeUp) { //MCU wake_up from deepSleep retention mode
        user_init_deepRetn();
    } else {             //MCU power_on or wake_up from deepSleep mode
        user_init_normal();
    }

    //rf_enable_bb_debug();
    irq_enable();

    while (1) {
        main_loop();
    }
    return 0;
}
#endif //#if (INTER_TEST_MODE == TEST_RAS_CLIENT)