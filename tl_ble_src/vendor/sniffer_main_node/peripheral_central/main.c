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
#include "app_main_node.h"

#if (MAIN_NODE_ROLE_SELECT == MAIN_NODE_PERIPHERAL_CENTRAL)

/**
 * @brief       BLE RF interrupt handler.
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void rf_irq_handler(void)
{
    DBG_CHN14_HIGH;

    blc_sdk_irq_handler();

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

    blc_sdk_irq_handler();

    DBG_CHN15_LOW;
}
PLIC_ISR_REGISTER(stimer_irq_handler, IRQ_SYSTIMER)

/**
 * @brief      application system initialization
 * @param[in]  none.
 * @return     none.
 */
__INLINE void blc_app_system_init(void)
{
    #if (MCU_CORE_TYPE == MCU_CORE_B92)
    sys_init(LDO_1P4_LDO_2P0, VBAT_MAX_VALUE_GREATER_THAN_3V6, GPIO_VOLTAGE_3V3, INTERNAL_CAP_XTAL24M);
    pm_update_status_info(1);
    gpio_set_up_down_res(GPIO_SWS, GPIO_PIN_PULLUP_1M);
    /* software reboot(sys_reboot()) come back,the interface status remains. */
    if (wd_get_status()) {
        wd_clear_status();
    }
    wd_set_interval_ms(2000);
    wd_start();

    wd_32k_stop();

    CCLK_48M_HCLK_48M_PCLK_24M;
    #else
        #error "Tis is not a chip spported by the sniffer sdk!!!"
    #endif
}

/**
 * @brief       This is main function
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ int main(void)
{
    DBG_CHN0_LOW;

    //Check whether the contents of flash and ramcode are consistent
    app_detect_init();

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
        //now do not use this mode, code will never enter here
    } else { //MCU power_on or wake_up from deepSleep mode
        user_init_normal();
        snif_main_node_init();
    }

    irq_enable();

    while (1) {
        wd_clear(); //clear watch dog, mandatory

        main_loop();
    }
    return 0;
}
#endif
