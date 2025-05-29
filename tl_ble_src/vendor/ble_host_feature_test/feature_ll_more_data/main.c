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
#include "app_config.h"
#include "app.h"
#include "../feature_common.h"

#if (FEATURE_TEST_MODE == TEST_LL_MD)


///**
// * @brief       BLE RF interrupt handler.
// * @param[in]   none
// * @return      none
// */
//_attribute_ram_code_ void rf_irq_handler(void)
//{
//    DBG_CHN14_HIGH;
//
//    blc_sdk_irq_handler();
//
//    DBG_CHN14_LOW;
//}
//PLIC_ISR_REGISTER(rf_irq_handler, IRQ_ZB_RT)
//
///**
// * @brief       System timer interrupt handler.
// * @param[in]   none
// * @return      none
// */
//_attribute_ram_code_ void stimer_irq_handler(void)
//{
//    DBG_CHN15_HIGH;
//
//    blc_sdk_irq_handler();
//
//    DBG_CHN15_LOW;
//}
//PLIC_ISR_REGISTER(stimer_irq_handler, IRQ_SYSTIMER)

/**
 * @brief       This is main function
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ int main(void)
{
    tlkapp_system_init();

    gpio_init(0);

    user_init();

    tlkapp_init();

    core_interrupt_enable();

    while(1)
    {
        main_loop();
    }
    return 0;
}

#endif //end of (FEATURE_TEST_MODE == ...)
