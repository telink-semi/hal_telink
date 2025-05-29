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
#include "app_config.h"

/**
 * @brief      application system initialization
 * @param[in]  none.
 * @return     none.
 */
void tlkapp_system_init(void)
{
#if (MCU_CORE_TYPE == MCU_CORE_TL322X)
    sys_init(LDO_1P25_LDO_1P8, VBAT_MAX_VALUE_GREATER_THAN_3V6, INTERNAL_CAP_XTAL24M);
    gpio_shutdown(GPIO_ALL);
    gpio_set_up_down_res(GPIO_SWS, GPIO_PIN_PULLUP_1M);
    wd_32k_stop();
    wd_stop();
    #if 0
        PLL_144M_CCLK_72M_HCLK_D25F_N22_36M_PCLK_36M_MSPI_48M;
    #else
        pm_set_dig_ldo(DIG_VOL_1V1_MODE, 1000);
        PLL_192M_CCLK_192M_HCLK_D25F_N22_96M_PCLK_96M_MSPI_48M;
    #endif
#endif
}

/**
 * @brief       This is main function
 * @param[in]   none
 * @return      none
 */
int main(void)
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
