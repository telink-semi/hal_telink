/********************************************************************************************************
 * @file    tlk_key.h
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
#ifndef APPLICATION_KEYBOARD_TLK_KEY_H_
#define APPLICATION_KEYBOARD_TLK_KEY_H_

#include "bit.h"
#include "types.h"
#include "compatibility_pack/cmpt.h"
#include "vendor/common/user_config.h"

#ifndef TLK_KEY_DEBUG_EN
    #define TLK_KEY_DEBUG_EN 0
#endif
//max key number
#ifndef TLK_KEY_NUM_MAX
    #define TLK_KEY_NUM_MAX 1
#endif
//gpio define
#ifndef TLK_GPIO_TYPE_DEF
    #define TLK_GPIO_TYPE_DEF gpio_pin_e
#endif
//key valid level
#ifndef TLK_KEY_VALID_LEVEL
    #define TLK_KEY_VALID_LEVEL 0
#endif

//key state
typedef enum
{
    KEY_STATE_DOWN    = 0,
    KEY_STATE_HOLD    = 1,
    KEY_STATE_LONG    = 2,
    KEY_STATE_RELEASE = 3
} tlk_key_state_e;

//key flag
typedef enum
{
    KEY_FLAG_SHORT        = 0,
    KEY_FLAG_DOUBLE_CLICK = 1,
    KEY_FLAG_TRIBLE_CLICK = 2,
    KEY_FLAG_HOLD         = 3,
    KEY_FLAG_LONG         = 4,
    KEY_FLAG_MAX
} tlk_key_flag_e;

//key flag
typedef enum
{
    TLK_KEY_SUCCESS         = 0,
    TLK_KEY_FAIL            = 1,
    TLK_KEY_PARAMETER_ERROR = 2,
    TLK_KEY_REPEAT          = 3,
    TLK_KEY_NOT_ENOUGH      = 4,
    TLK_KEY_NOT_FOUND       = 5,

    TLK_KEY_INVALID = 0xff,
} tlk_key_sts_e;

// key function
typedef int (*tlk_key_func)(void);

typedef struct
{
    TLK_GPIO_TYPE_DEF key_pin;
    TLK_GPIO_TYPE_DEF key_out_pin;      /**< if the key is matrix scan mode,the pin is used for output.else please set 0.  */
    u8                key_down_level;   /**< 0 or 1                                                              */
    u32               key_hold_cnt;     /**< unit: 10ms, min - MIN_HOLD_CNT;  else set default                   *
                                          * if set key_long_cnt, key hold should not set      */
    u32               key_long_cnt;     /**< unit: 10ms  min - MIN_LONG_CNT;  else set default                   */
    u32               key_intervel_cnt; /**< unit: 10ms  min - MIN_INTERVAL;  else set default                   */
} tlk_key_config_t;

/** Key control info struct ---------------------------------------------*/
typedef struct
{
    u8               key_id;
    u32              key_down_cnt;
    u32              key_up_cnt;
    u32              key_click_cnt;
    tlk_key_state_e  key_state;
    tlk_key_config_t key_info;
} tlk_key_ctl_t;

typedef union
{
    u32 flg;

    struct
    {
        /**< byte 0 */
        u8 _short  : 1; /*BIT(0) */
        u8 _dclick : 1; /*BIT(1)*/
        u8 _tclick : 1; /*BIT(2)*/
        u8 _hold   : 1; /*BIT(3)*/
        u8 _long   : 1; /*BIT(4)*/
        u8 _res    : 3;
        /**< byte 1~3 */
    };
} tlk_key_event_flag_u;

typedef struct
{
    u8 _short;  /* short key event class     */
    u8 _dclick; /* double click event class  */
    u8 _tclick; /* triple click event class  */
    u8 _hold;   /* hold key event class      */
    u8 _long;   /* long key event class      */
} tlk_key_event_class_t;

typedef struct
{
    tlk_key_event_flag_u  evt_occur; /**< some events occurrence  */
    tlk_key_event_class_t mode;      /**< current key event mode  */
} tlk_key_event_t;

/** Key event struct -----------------------------------------------------*/
typedef struct
{
    u8            count;  /**<  the number of pkey_table member      */
    tlk_key_func *p_func; /**<  the key event table                  */
} tlk_key_func_table_t;

/**
 * @brief      This function serves to add a new key.
 * @param[in]  p  - user configure parameters.
 * @return     Succeed or failed,search 'tlk_key_sts_e' for detail.
 */
tlk_key_sts_e tlk_key_add(tlk_key_config_t *p);

/**
 * @brief      This function serves to add a new key.
 * @param[in]  p  - user configure parameters.
 * @return     Succeed or failed,search 'tlk_key_sts_e' for detail.
 */
tlk_key_sts_e tlk_key_remove(u8 key_id);

/**
 * @brief  register the key happen event before key event.
 *
 * @param[in] p_event : the happen event
 *
 * @returns 0-success, else fail.
 */
tlk_key_sts_e tlk_key_evt_mode_register(uint8_t key_id, tlk_key_event_class_t *p_event);
/**
 * @brief      This function serves to register the key event table.
 * @param[in]  none.
 * @return     none.
 */
tlk_key_sts_e tlk_key_register_event_table(const tlk_key_func *table, u16 count);

/**
 * @brief      This function serves to start key scan operation.
 * @param[in]  none.
 * @return     none.
 */
void tlk_key_scan(void);

/**
 * @brief      This function serves to process key scan event.
 * @param[in]  none.
 * @return     none.
 */
void tlk_key_process(void);

/**
 * @brief      This function serves to get the key id by GPIO.
 * @param[in]  gpio     - key GPIO.
 * @param[in]  out_gpio - if key scan not used,key_out_pin should set 0.
 * @return     0        - fail
 *             others   - key id.
 */
u8 tlk_key_get_id(TLK_GPIO_TYPE_DEF gpio, TLK_GPIO_TYPE_DEF out_gpio);

/**
 * @brief      This function serves to get the empty key index.
 * @param[in]  none
 * @return     0xff     - fail
 *             others   - empty key index.
 */
u8 tlk_key_get_empty_index(void);


#endif /* APPLICATION_KEYBOARD_TLK_KEY_H_ */
