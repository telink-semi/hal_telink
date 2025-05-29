/********************************************************************************************************
 * @file    app_led.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BT Audio Group
 * @date    2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#include "app_led.h"

#if (UNICAST_SERVER_SELECT == UNICAST_SERVER_TWS)

app_led_state_t appLedState;

/**
 * @brief  get app led id.
 *
 * @param[in] index_flag  led index.
 *
 * @returns led_id.
 */
u8 app_led_get_id(u8 index_flag)
{
    if (index_flag >= TLK_LED_NUM_MAX) {
        return 0;
    }
    return appLedState.led_id[index_flag];
}

/**
 * @brief  set power_on state led.
 *
 * @param[in] None.
 *
 * @returns None.
 */
void app_led_set_power_on_state(void)
{
    app_led_set_all_ctrl_pattern(LED_PATTERN_LED_FLASH_SLOW, DEFAULT_PATTERN);

    appLedState.conn_state        = SYS_STATE_IDLE;
    appLedState.led_power_on_flag = TRUE;
}

/**
 * @brief   set pattern to all app led.
 *
 * @param[in] patt  patt index.
 * @param[in] is_user_patt  refer to tlk_led_patt_from_e.
 *
 * @returns None.
 */
_attribute_ram_code_sec_noinline_ void app_led_set_all_ctrl_pattern(int patt, tlk_led_patt_from_e is_user_patt)
{
    u8 i_for  = 0;
    u8 led_id = 0;
    for (i_for = 0; i_for < TLK_LED_NUM_MAX; i_for++) {
        led_id = app_led_get_id(i_for);

        if (led_id == 0 || led_id > TLK_LED_NUM_MAX) {
            continue;
        }

        tlk_led_pattern_set(led_id, patt, is_user_patt);
    }

    appLedState.log_flag_led_set_patt = 1;
    appLedState.log_flag_patt         = (u16)patt;
}

/**
 * @brief  insert pattern to all app led.
 *
 * @param[in] patt  patt index.
 * @param[in] is_user_patt  refer to tlk_led_patt_from_e.
 *
 * @returns None.
 */
_attribute_ram_code_sec_noinline_ void app_led_insert_all_ctrl_pattern(int patt, tlk_led_patt_from_e is_user_patt)
{
    u8 i_for  = 0;
    u8 led_id = 0;
    for (i_for = 0; i_for < TLK_LED_NUM_MAX; i_for++) {
        led_id = app_led_get_id(i_for);

        if (led_id == 0 || led_id > TLK_LED_NUM_MAX) {
            continue;
        }

        tlk_led_pattern_insert(led_id, patt, is_user_patt);
    }
    appLedState.log_flag_led_insert_patt = 1;
    appLedState.log_flag_patt            = (u16)patt;
}

/**
 * @brief  led evb init.
 *
 * @param[in] None.
 *
 * @returns None.
 */
void app_led_init(void)
{
    tlk_led_config_t led_config;
    int              ret = 0;
    #ifdef LED1_ID
    /** add led1 */
    led_config.led_pin      = LED1_PIN;
    led_config.led_on_level = TLK_LED_VALID_LEVEL;
    led_config.pwmid        = PWM0_ID;
    ret                     = tlk_led_add(&led_config);
    if (ret != TLK_LED_SUCCESS) {
        tlkapi_printf(APP_LOG_EN, "led_1 init fail,ret - %d\n", ret);
    } else {
        tlkapi_printf(APP_LOG_EN, "led_1 init success\n");
        appLedState.led_id[APP_LED_INDEX_BLUE] = tlk_get_led_id_by_gpio(led_config.led_pin);
    }
    #endif

    #ifdef LED2_ID
    /** add led2 */
    led_config.led_pin      = LED2_PIN;
    led_config.led_on_level = TLK_LED_VALID_LEVEL;
    led_config.pwmid        = PWM1_ID;
    ret                     = tlk_led_add(&led_config);
    if (ret != TLK_LED_SUCCESS) {
        tlkapi_printf(APP_LOG_EN, "led_2 init fail,ret - %d\n", ret);
    } else {
        tlkapi_printf(APP_LOG_EN, "led_2 init success\n");
        appLedState.led_id[APP_LED_INDEX_GREEN] = tlk_get_led_id_by_gpio(led_config.led_pin);
    }
    #endif

    #ifdef LED3_ID
    /** add led3 */
    led_config.led_pin      = LED3_PIN;
    led_config.led_on_level = TLK_LED_VALID_LEVEL;
    led_config.pwmid        = PWM2_ID;
    ret                     = tlk_led_add(&led_config);
    if (ret != TLK_LED_SUCCESS) {
        tlkapi_printf(APP_LOG_EN, "led_3 init fail,ret - %d\n", ret);
    } else {
        tlkapi_printf(APP_LOG_EN, "led_3 init success\n");
        appLedState.led_id[APP_LED_INDEX_WHITE] = tlk_get_led_id_by_gpio(led_config.led_pin);
    }
    #endif

    #ifdef LED4_ID
    /** add led4 */
    led_config.led_pin      = LED4_PIN;
    led_config.led_on_level = TLK_LED_VALID_LEVEL;
        #if (MCU_CORE_TYPE == MCU_CORE_B91 && HARDWARE_TYPE == TLSR9517CDK56D)
    led_config.pwmid = PWM3_ID;
        #elif (MCU_CORE_TYPE == MCU_CORE_B91 && HARDWARE_TYPE == TLSR9518ADK80D)
    led_config.pwmid = PWM2_ID;
        #endif
    ret = tlk_led_add(&led_config);
    if (ret != TLK_LED_SUCCESS) {
        tlkapi_printf(APP_LOG_EN, "led_4 init fail,ret - %d\n", ret);
    } else {
        tlkapi_printf(APP_LOG_EN, "led_4 init success\n");
        appLedState.led_id[APP_LED_INDEX_RED] = tlk_get_led_id_by_gpio(led_config.led_pin);
    }
    #endif


    app_led_set_power_on_state();
}

/**
 * @brief  app led task.
 *
 * @param[in] None.
 *
 * @returns None.
 */
void app_led_task(void)
{
    tlk_led_refresh_process();
}

#endif
