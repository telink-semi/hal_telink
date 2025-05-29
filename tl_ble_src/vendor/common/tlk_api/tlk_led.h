/********************************************************************************************************
 * @file    tlk_led.h
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
#ifndef __API_LED_H
#define __API_LED_H
#include "drivers.h"

/** for debug. */
#ifndef TLK_LED_DBG_FLAG
    #define TLK_LED_DBG_FLAG 0
#endif

/** Register led number max */
#ifndef TLK_LED_NUM_MAX
    #define TLK_LED_NUM_MAX 2
#endif

#ifndef TLK_GPIO_TYPE_DEF
    #define TLK_GPIO_TYPE_DEF gpio_pin_e
#endif

/** TLK_LED_VALID_LEVEL :default led on level is 1 */
#ifndef TLK_LED_VALID_LEVEL
    #define TLK_LED_VALID_LEVEL 1
#endif

/*
 *
 * LED flash descriptor:
 *
 *      keey_high/keep_low is useless for flash
 *
 *         |<---duty------>
 *         +--------------+
 *         |              |<---latency-->|
 *         +              +--------------+
 *
 *
 * LED breath descriptor:
 *
 *      duty     : is used for rise time, unit is mS
 *      keey_high: is used for full duty keep time, unit is mS
 *      latency  : is used for falling time, unit is mS
 *      keep_low : is used for off time keep.
 *
 *        duty                latency
 *       |<-->|<--keep_high->|<->|
 *           /---------------\
 *          /                 \
 *         /                   \ |<----keep_low-->|
 *        /                     \----------------+
 *
 *
 * Clock is 1MHz, Breath Freq. is 2KHz, so pwm counter is 500.
 */

#define BRT_FULL_DUTY                      256
#define BRT_FULL_STEP                      32
#define BRT_BRIGHTEST_FULL_DUTY            484

#define TIME_FOREVER                       0xffff
#define TIME_NO_TIME                       0x0000
#define FLASH_FOREVER                      0xffff

#define LED_COLOR_NOT_SET                  0xffff
#define LED_MAX_COLOR                      BRT_BRIGHTEST_FULL_DUTY
#define LED_DEFAULT_COLOR                  (LED_MAX_COLOR)
#define GET_PWM_GPIO_FUNC(pwmid)           ((pwmid <= PWM5_ID) ? (PWM0 + pwmid) : 0)
#define GET_PWM_FRAME_DONE_IRQ_MASK(pwmid) ((pwmid == PWM0_ID) ? FLD_PWM0_FRAME_DONE_IRQ : \
                                            (pwmid == PWM1_ID) ? FLD_PWM1_FRAME_DONE_IRQ : \
                                            (pwmid == PWM2_ID) ? FLD_PWM2_FRAME_DONE_IRQ : \
                                            (pwmid == PWM3_ID) ? FLD_PWM3_FRAME_DONE_IRQ : \
                                            (pwmid == PWM4_ID) ? FLD_PWM4_FRAME_DONE_IRQ : \
                                            (pwmid == PWM5_ID) ? FLD_PWM5_FRAME_DONE_IRQ : \
                                                                 0)
#define GET_PWM_EN_TYPE(pwmid) ((pwmid == PWM0_ID) ? FLD_PWM0_EN : \
                                (pwmid == PWM1_ID) ? FLD_PWM1_EN : \
                                (pwmid == PWM2_ID) ? FLD_PWM2_EN : \
                                (pwmid == PWM3_ID) ? FLD_PWM3_EN : \
                                (pwmid == PWM4_ID) ? FLD_PWM4_EN : \
                                (pwmid == PWM5_ID) ? FLD_PWM5_EN : \
                                                     0)

/** led_pattern mode */
typedef enum
{
    DEFAULT_PATTERN = 0,               /* use default pattern buffer            */
    USER_PATTERN    = !DEFAULT_PATTERN /* use pattern buffer registered by user */
} tlk_led_patt_from_e;

/** led_pattern default mode */
typedef enum
{
    LED_PATTERN_LED_OFF                = 0,
    LED_PATTERN_LED_ON                 = 1,
    LED_PATTERN_LED_FLASH_SLOW         = 2,
    LED_PATTERN_LED_FLASH_FAST         = 3,
    LED_PATTERN_LED_FLASH_PAIR         = 4,
    LED_PATTERN_LED_IDLE               = 5,
    LED_PATTERN_LED_FAST_FLASH_1_TIME  = 6,
    LED_PATTERN_LED_FAST_FLASH_2_TIMES = 7,
    LED_PATTERN_LED_SLOW_FLASH_2_TIMES = 8,
    LED_PATTERN_LED_FAST_FLASH_3_TIMES = 9,
    LED_PATTERN_LED_ON_500MS           = 10,
    LED_PATTERN_LED_ON_1S              = 11,

    LED_PATTERN_LED_SLOW_FLASH_3_TIMES = 12,

    LED_PATTERN_LED_BREATH_ON_OFF      = 13,
    LED_PATTERN_LED_BREATH_ON          = 14,
    LED_PATTERN_LED_BREATH_OFF         = 15,
    LED_PATTERN_LED_BREATH_ONE_TIME    = 16,
    LED_PATTERN_LED_BREATH_TWO_TIMES   = 17,
    LED_PATTERN_LED_BREATH_THREE_TIMES = 18,
    LED_PATTERN_LED_BREATH             = 19,
    LED_PATTERN_LED_BREATH_PAIR        = 20,

    LED_PATTERN_LED_HIGH_LIGHT_FLASH_1_TIME = 21,

    LED_PATTERN_LED_MAX_STATE,
} led_pattern_index_e;

typedef enum
{
    TLK_LED_SUCCESS         = 0,
    TLK_LED_IS_INSERTING    = 1,
    TLK_LED_PARAMETER_ERROR = 2,
    TLK_LED_PARTTEN_ERROR   = 3,
    TLK_LED_NOT_ENOUGH      = 4,
    TLK_LED_REPEAT          = 5,
    TLK_LED_INSERT_CAUTION  = 6,

    TLK_LED_INVALID = 0xff,
} tlk_led_sts_e;

/** Led config struct */
typedef struct
{
    TLK_GPIO_TYPE_DEF led_pin;
    u8                led_on_level; /**< 0 or 1    default:TLK_LED_VALID_LEVEL   */
    pwm_id_e          pwmid;        /**< io pwm */
} tlk_led_config_t;

/** Led pattern struct */
typedef struct
{
    u16 behavior;    /**< zero is flash, non-zero is breath          */
    u16 duty;        /**< Duty for LED on                             */
    u16 latency;     /**< Off time                                    */
    u16 flash_times; /**< Breath or flash counter                     */
    u16 keep_high;   /**< unit is mS                                  */
    u16 keep_low;    /**< unit is mS                                  */
} tlk_led_pattern_t;

/** led lum state struct */
typedef struct
{
    u16 state;
    u16 flags;
    u16 lum;
    u16 cnt;
    u32 ref;
} tlk_lum_state_t;

/** led pattern table struct */
typedef struct
{
    u16                pattern_count;   /**< the number of color      */
    tlk_led_pattern_t *p_pattern_table; /**< the led event table     */
} tlk_led_pattern_table_t;

/** led color type struct */
typedef struct
{
    u16 color_value[TLK_LED_NUM_MAX]; /**< the led event table -LED_USE_DEFAULT_COLOR:use default color */
} tlk_led_color_t;

/** led color table struct */
typedef struct
{
    u16              color_count;   /**< the number of color     */
    tlk_led_color_t *p_color_table; /**< the led color table     */
} tlk_tlk_led_color_table_t;

/** led env struct */
typedef struct
{
    tlk_led_pattern_t *p_pattern;
    tlk_lum_state_t    lum_state;

    tlk_led_pattern_t *p_insert_pattern; /**< after execute insert pattern, return to pattern. */
    tlk_lum_state_t    insert_lum_state;
} tlk_led_env_t;

/** led control info struct */
typedef struct
{
    u8               led_id;   /**< led index, automatic allocation  0:unused,other:be using  */
    u16              color_duty;
    tlk_led_env_t    led_env;
    tlk_led_config_t led_info; /**< led user config                                            */
} tlk_led_ctrl_t;

/** Led functions table */
typedef int (*led_func)(void);

/**
 * @brief  add a new led parameters.
 *
 * @param[in] p : User Configuration Parameters
 * @param[in] new_mode : key mew mode
 *
 * @returns 0-success, else fail
 */
tlk_led_sts_e tlk_led_add(tlk_led_config_t *p);

/**
 * @brief  remove the led_id function.
 *
 * @param[in] led_id : led id
 *
 * @returns 0-success, else fail
 */
tlk_led_sts_e tlk_led_remove(u8 led_id);

/**
 * @brief get the led_id by GPIO.
 *
 * @param[in] gpio : led gpio
 *
 * @returns 0-fail, else led_id
 */
u8 tlk_get_led_id_by_gpio(TLK_GPIO_TYPE_DEF gpio);

/**
 * @brief  register the led user patterns table.
 *
 * @param[in] table : the user patterns table
 * @param[in] tbl_mode_num : table counts(table max_index + 1)
 *
 * @returns 0-success, else fail
 */
tlk_led_sts_e tlk_led_user_patterns_register(tlk_led_pattern_table_t *table, u16 tbl_mode_num);

/**
 * @brief  remove the led user patterns table.
 *
 * @param[in] none
 *
 * @returns 0-success, else fail
 */
tlk_led_sts_e tlk_led_user_patterns_remove(void);

/**
 * @brief  set the led color duty.
 *
 * @param[in] color : all led color will be set.
 *                  if the color value is 'LED_COLOR_NOT_SET',no be set.
 *                  if the color value > 'LED_MAX_COLOR',it will be set 'LED_MAX_COLOR'.
 * @param[in] new_mode : key mew mode
 *
 * @returns 0-success, else fail
 */
void tlk_set_led_color_duty(tlk_led_color_t *color);

/**
 * @brief  get the led color duty.
 *
 * @param[out] out_color : all led color out.
 *
 * @returns none
 */
void tlk_get_led_color_duty(tlk_led_color_t *out_color);

/**
 * @brief  get led pattern duty.
 *
 * @param[in] led_id
 *
 * @returns pattern duty
 */
u16 tlk_led_get_pattern_duty(int led_id);

/**
 * @brief  get led current_lum.
 *
 * @param[in] led_id
 *
 * @returns current lum
 */
u16 tlk_led_get_current_lum(int led_id);

/**
 * @brief  set the led pattern.
 *
 * @param[in] led_id : led id
 * @param[in] patt : led pattern the type is 'tlk_led_pattern_t'
 * @param[in] is_user_patt : the patt is defined by user or default.
 *
 * @returns 0-success, else fail
 */
tlk_led_sts_e tlk_led_pattern_set(int led_id, int patt, tlk_led_patt_from_e is_user_patt);

/**
 * @brief  insert a led pattern.
 *
 * @param[in] led_id : led id
 * @param[in] patt : led pattern the type is 'tlk_led_pattern_t'
 * @param[in] is_user_patt : the patt is defined by user or default.
 *
 * @returns 0-success, else fail
 */

tlk_led_sts_e tlk_led_pattern_insert(int led_id, int patt, tlk_led_patt_from_e is_user_patt);

/**
 * @brief  get led status - led_is_insert_pattern_running.
 *
 * @param[in] led_id
 *
 * @returns 0-not_insert, 1-insert, 2-led err
 */
tlk_led_sts_e tlk_led_is_insert_pattern_running(int led_id);

/**
 * @brief  led breath and flash process.
 *
 * @param[in] none
 *
 * @returns none
 */
void tlk_led_refresh_process(void);


#endif /**__API_LED_H */
