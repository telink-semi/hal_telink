/********************************************************************************************************
 * @file    app_led.h
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
#ifndef LED_H
#define LED_H

#include "app_config.h"


#if (INTER_TEST_MODE == TEST_LE_AUDIO_SWTICH_SERVER)
#include "tlk_api/tlk_led.h"
    /** led index define */
    #define APP_LED_INDEX_RED   0
    #define APP_LED_INDEX_BLUE  1
    #define APP_LED_INDEX_GREEN 2
    #define APP_LED_INDEX_WHITE 3

/** System main state */
typedef enum
{
    SYS_STATE_IDLE,
    SYS_STATE_PAIR,
    SYS_STATE_CONNECTED,
    SYS_STATE_POWER_OFF,

    SYS_STATE_MAX
} sys_led_state_e;

typedef struct __attribute__((packed))
{
    u8 led_power_on_flag      : 1;   /**< TRUE: power on just now and flash the led */
    u8 led_task_not_exec_flag : 1;   /**< TRUE: the led task will return */
    u8 led_is_insert          : 1;   /**< TRUE: the led is insert patt */
    u8 led_pair_discon_sync   : 1;   /**< enter pair, disconn time for master and slave is different */

    u8 log_flag_led_callbck     : 1; /**  */
    u8 log_flag_led_set_patt    : 1; /**< TRUE: 'app_led_set_all_ctrl_pattern' is exec */
    u8 log_flag_led_insert_patt : 1; /**< TRUE: 'app_led_insert_all_ctrl_pattern' is exec */
    u8 log_flag_led_error       : 1; /**< TRUE: led error */

    u8  led_id[TLK_LED_NUM_MAX];     /**< value 0:unused ,else the led id */
    u8  conn_state;                  /**< refer to sys_led_state_e */
    u32 led_insert_flag;             /**< everybit:1-insert patt,0-not. bit refer to 'led_id[TLK_LED_NUM_MAX]' index number
                               when (led_insert_flag==0 && led_is_insert== TRUE), resync led. */
    u16 log_flag_patt;               /**< record the led set patt or led insert patt for log */
} app_led_state_t;

/**
 * @brief  get app led id.
 *
 * @param[in] index_flag  led index.
 *
 * @returns led_id.
 */
u8 app_led_get_id(u8 index_flag);

/**
 * @brief  set power_on state led.
 *
 * @param[in] None.
 *
 * @returns None.
 */
void app_led_set_power_on_state(void);

/**
 * @brief  set pattern to all app led.
 *
 * @param[in] patt  patt index.
 * @param[in] is_user_patt  refer to tlk_led_patt_from_e.
 *
 * @returns None.
 */
void app_led_set_all_ctrl_pattern(int patt, tlk_led_patt_from_e is_user_patt);

/**
 * @brief  insert pattern to all app led.
 *
 * @param[in] patt  patt index.
 * @param[in] is_user_patt  refer to tlk_led_patt_from_e.
 *
 * @returns None.
 */
void app_led_insert_all_ctrl_pattern(int patt, tlk_led_patt_from_e is_user_patt);

/**
 * @brief  led evb init.
 *
 * @param[in] None.
 *
 * @returns None.
 */
void app_led_init(void);

/**
 * @brief  app led task.
 *
 * @param[in] None.
 *
 * @returns None.
 */
void app_led_task(void);

#endif
#endif
