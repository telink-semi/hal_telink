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

#include "hci_transport/hci_dfu.h"

/**
 * @brief       BLE RF interrupt handler.
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void rf_irq_handler(void)
{
    DBG_CHN14_HIGH;
    //gpio_write(GPIO_PC1, 1);

    blc_sdk_irq_handler();
#if 0
    if(blm_btxbrx_state){
        gpio_write(GPIO_PC2, 1);
    }
    else{
        gpio_write(GPIO_PC2, 0);
    }
#endif
    DBG_CHN14_LOW;
    //gpio_write(GPIO_PC1, 0);
}
#ifdef MCU_CORE_N22_ENABLE
CLIC_ISR_REGISTER(rf_irq_handler, IRQ_ZB_RT)
#else
PLIC_ISR_REGISTER(rf_irq_handler, IRQ_ZB_RT)
#endif

/**
 * @brief       System timer interrupt handler.
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void stimer_irq_handler(void)
{
    DBG_CHN15_HIGH;
    //gpio_write(GPIO_PC0, 1);

    blc_sdk_irq_handler();
#if 0
    if(blm_btxbrx_state){
        gpio_write(GPIO_PC2, 1);
    }
    else{
        gpio_write(GPIO_PC2, 0);
    }
#endif
    DBG_CHN15_LOW;
    //gpio_write(GPIO_PC0, 0);
}
CLIC_ISR_REGISTER(stimer_irq_handler, IRQ_SYSTIMER)

#if (APP_LE_CHANNEL_SOUNDING_TEST_MODE || APP_LE_CHANNEL_SOUNDING)
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
    if (timer_get_irq_status(FLD_TMR0_MODE_IRQ))
#endif
    {
        DBG_CS_CHN4_HIGH;
        u32 r = core_interrupt_disable();
        reg_tmr_ctrl0 &= ~FLD_TMR0_EN;
#if (MCU_CORE_TYPE == MCU_CORE_TL721X)
        timer_clr_irq_status(FLD_TMR0_MODE_IRQ); //clear irq status
#else
        timer_clr_irq_status(FLD_TMR0_MODE_IRQ); //clear irq status
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
CLIC_ISR_REGISTER(timer0_irq_handler, IRQ_TIMER0)
#endif
/**
 * @brief      application system initialization
 * @param[in]  none.
 * @return     none.
 */
__INLINE void blc_app_system_init(void)
{
    clic_init();
    clic_preempt_feature_en();
    rf_n22_dig_init();

    /* These parameters must remain consistent with the D25F configuration. */
#if 0
    //PLL_144M_CCLK_72M_HCLK_D25F_N22_36M_PCLK_36M_MSPI_48M;
    sys_clk.pll_clk  = 144;
    sys_clk.cclk     = 72;
    sys_clk.hclk_n22 = 36;
    sys_clk.pclk     = 36;
    sys_clk.mspi_clk = 48;
#else
    //PLL_192M_CCLK_192M_HCLK_D25F_N22_96M_PCLK_96M_MSPI_48M;
    sys_clk.pll_clk  = 192;
    sys_clk.cclk     = 192;
    sys_clk.hclk_n22 = 96;
    sys_clk.pclk     = 96;
    sys_clk.mspi_clk = 48;
#endif
}


/**
 * @brief       This is main function
 * @param[in]   none
 * @return      none
 */
int main(void)
{
#if HCI_DFU_EN
    blc_ota_setFirmwareSizeAndBootAddress(DFU_NEW_FW_MAX_SIZE, DFU_NEW_FW_ADDR_BASE);
#endif
    gpio_write(GPIO_PB0, 0);
    /* this function must called before "sys_init()" when:
     * (1). For all IC: using 32K RC for power management,
       (2). For B91 only: even no power management */
    blc_pm_select_internal_32k_crystal();

    blc_app_system_init();

    /* detect if MCU is wake_up from deep retention mode */
    int deepRetWakeUp = pm_is_MCU_deepRetentionWakeup(); //MCU deep retention wakeUp

    rf_drv_ble_init();
    gpio_write(GPIO_PB0, 1);
    if (deepRetWakeUp) { //MCU wake_up from deepSleep retention mode
        user_init_deepRetn();
    }
    else {             //MCU power_on or wake_up from deepSleep mode
        user_init_normal();
    }

    rf_enable_bb_debug(); //ble sdk remove!!!

    irq_enable();

    gpio_write(GPIO_PB0, 0);

    while (1)
    {
        main_loop();
    }

    return 0;
}
