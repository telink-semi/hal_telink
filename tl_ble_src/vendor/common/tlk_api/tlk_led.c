/********************************************************************************************************
 * @file    tlk_led.c
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
#include "tlk_led.h"

#define ST_FLASH_DUTY        (1 << 0)
#define ST_FLASH_LAT         (1 << 1)
#define ST_FLASH_IDLE        (1 << 2)
#define ST_BRT_RISE          (1 << 0)
#define ST_BRT_KEEP_HIGH     (1 << 1)
#define ST_BRT_FALL          (1 << 2)
#define ST_BRT_KEEP_LOW      (1 << 3)
#define ST_BRT_IDLE          (1 << 4)
#define FLG_BRT_REVERT_LEVEL (1 << 0)
#define FLG_PATTERN_FOLLOW   (1 << 1)

#define BHV_BREATH           (1 << 0)
#define BHV_HIGH_LIHGT_FLASH (1 << 1)

/** All led info */
static tlk_led_ctrl_t          ledInfo[TLK_LED_NUM_MAX] = {0};
static tlk_led_pattern_table_t ledUserPatterns          = {0};

/** Default default led info */
static const tlk_led_pattern_t ledDfltPatterns[] = {
    /** behavior ,            duty ,        latency ,     flash_times ,  keep_high ,   keep_low */
    /** LED flash */
    {0,                    TIME_NO_TIME, TIME_FOREVER, 0,             0,            0           }, /** LED_PATTERN_LED_OFF                 */
    {0,                    TIME_FOREVER, TIME_NO_TIME, 0,             0,            0           }, /** LED_PATTERN_LED_ON                  */
    {0,                    200,          2000,         FLASH_FOREVER, 0,            0           }, /** LED_PATTERN_LED_FLASH_SLOW          */
    {0,                    100,          100,          FLASH_FOREVER, 0,            0           }, /** LED_PATTERN_LED_FLASH_FAST          */
    {0,                    500,          1500,         FLASH_FOREVER, 0,            0           }, /** LED_PATTERN_LED_PAIR                */
    {0,                    100,          1000,         FLASH_FOREVER, 0,            0           }, /** LED_PATTERN_LED_IDLE                */
    {0,                    50,           100,          1,             0,            0           }, /** LED_PATTERN_LED_FAST_FLASH_1_TIME   */
    {0,                    200,          200,          2,             0,            0           }, /** LED_PATTERN_LED_FAST_FLASH_2_TIMES  */
    {0,                    500,          500,          2,             0,            0           }, /** LED_PATTERN_LED_SLOW_FLASH_2_TIMES  */
    {0,                    500,          500,          3,             0,            0           }, /** LED_PATTERN_LED_FAST_FLASH_3_TIMES  */
    {0,                    500,          50,           1,             0,            0           }, /** LED_PATTERN_LED_ON_500MS            */
    {0,                    1000,         50,           1,             0,            0           }, /** LED_PATTERN_LED_ON_1S               */
    {0,                    500,          500,          3,             0,            0           }, /** LED_PATTERN_LED_SLOW_FLASH_3_TIMES  */

                                              /** LED breath */
    {BHV_BREATH,           100,          100,          1,             800,          0           }, /** LED_PATTERN_LED_BREATH_ON_OFF      */
    {BHV_BREATH,           100,          0,            0,             TIME_FOREVER, 0           }, /** LED_PATTERN_LED_BREATH_ON          */
    {BHV_BREATH,           TIME_NO_TIME, 400,          0,             0,            TIME_FOREVER}, /** LED_PATTERN_LED_BREATH_OFF         */
    {BHV_BREATH,           200,          200,          1,             TIME_NO_TIME, TIME_NO_TIME}, /** LED_PATTERN_LED_BREATH_ONE_TIME    */
    {BHV_BREATH,           200,          200,          2,             TIME_NO_TIME, TIME_NO_TIME}, /** LED_PATTERN_LED_BREATH_TWO_TIMES   */
    {BHV_BREATH,           200,          200,          3,             TIME_NO_TIME, TIME_NO_TIME}, /** LED_PATTERN_LED_BREATH_THREE_TIMES */
    {BHV_BREATH,           600,          600,          FLASH_FOREVER, 800,          800         }, /** LED_PATTERN_LED_BREATH             */
    {BHV_BREATH,           400,          400,          FLASH_FOREVER, 300,          300         }, /** LED_PATTERN_LED_BREATH_PAIR        */
    {BHV_HIGH_LIHGT_FLASH, 1000,         0,            1,             0,            0           }, /** LED_PATTERN_LED_BREATH_PAIR        */
};

/** Static functions */
static u8   tlk_get_unused_led_index(void);
static u16  tlk_color_calibrate(tlk_led_ctrl_t *p_led_ctrl, u16 duty);
static u16  tlk_lum_convert(int is_revert, u16 step);
static void tlk_pattern_recovery(tlk_led_ctrl_t *p_led_ctrl);
static void tlk_lum_state_init(tlk_led_ctrl_t    *p_led_ctrl,
                               tlk_lum_state_t   *p_lum_state,
                               tlk_led_pattern_t *p_led_pattern,
                               int                follow);
static void tlk_led_breath_process(tlk_led_ctrl_t *p_led_ctrl);
static void tlk_led_flash_process(tlk_led_ctrl_t *p_led_ctrl);

/**
 * @brief get the first unused led index.
 *
 * @param[in] none
 *
 * @returns 0xff-fail, else the address of the led
 */
static u8 tlk_get_unused_led_index(void)
{
    u8              i_for  = 0;
    tlk_led_ctrl_t *p_temp = ledInfo;

    for (i_for = 0; i_for < TLK_LED_NUM_MAX; ++i_for) {
        if (p_temp[i_for].led_id == 0) {
            return i_for;
        }
    }
    tlkapi_printf(TLK_LED_DBG_FLAG, "no unused led");

    return TLK_LED_INVALID;
}

/**
 * @brief  set the led color duty.
 *
 * @param[in] p_led_ctrl : led info
 * @param[in] duty : duty
 *
 * @returns 0-success, else fail
 */
static u16 tlk_color_calibrate(tlk_led_ctrl_t *p_led_ctrl, u16 duty)
{
    if (p_led_ctrl == NULL) {
        tlkapi_printf(TLK_LED_DBG_FLAG, "p_led_ctrl NULL");
        return TLK_LED_PARAMETER_ERROR;
    }
    u32             tmp;
    tlk_led_ctrl_t *p_temp = p_led_ctrl;

    tmp = p_temp->color_duty;
    // calibrate
    tmp = tmp * (duty * 1024) / BRT_BRIGHTEST_FULL_DUTY / 1024;

    // update duty
    pwm_set_tcmp(p_temp->led_info.pwmid, tmp);

    return TLK_LED_SUCCESS;
}

/**
 * @brief  set the led color duty.
 *
 * @param[in] is_revert : is revert step
 * @param[in] step : led exec step, full is "BRT_FULL_STEP"
 *
 * @returns duty of the step.
 */
static u16 tlk_lum_convert(int is_revert, u16 step)
{
    if (is_revert) {
        step = ((BRT_FULL_STEP / 2) + step) % (BRT_FULL_STEP + 1);
    }

    return step >= (BRT_FULL_STEP / 2) ? (BRT_FULL_STEP - step) * (BRT_FULL_STEP - step) : step * step;
}

static void tlk_pattern_recovery(tlk_led_ctrl_t *p_led_ctrl)
{
    tlk_led_env_t *p_env    = &p_led_ctrl->led_env;
    p_env->p_insert_pattern = NULL;
    p_env->lum_state.ref    = clock_time();
    tlk_color_calibrate(p_led_ctrl,
                        (p_env->p_pattern->behavior & BHV_BREATH) ?
                            tlk_lum_convert(p_env->lum_state.flags & FLG_BRT_REVERT_LEVEL, p_env->lum_state.lum) :
                            p_env->lum_state.lum);
}

/**
 * @brief  init the led state.
 *
 * @param[in] p_led_ctrl : led info
 * @param[in] lum_state : led state
 * @param[in] p_led_pattern : led pattern
 * @param[in] follow : unused,default 0.
 *
 * @returns none
 */
static void tlk_lum_state_init(tlk_led_ctrl_t    *p_led_ctrl,
                               tlk_lum_state_t   *p_lum_state,
                               tlk_led_pattern_t *p_led_pattern,
                               int                follow)
{
    u16            duty;
    tlk_led_env_t *p_env = &p_led_ctrl->led_env;
    if (p_env == NULL || p_lum_state == NULL || p_led_pattern == NULL) {
        tlkapi_printf(TLK_LED_DBG_FLAG, "init parameter NULL");
        return;
    }

    p_lum_state->ref   = clock_time();
    p_lum_state->cnt   = 0;
    p_lum_state->flags = follow ? FLG_PATTERN_FOLLOW : 0;

    // LED breath init
    if (p_led_pattern->behavior & BHV_BREATH) {
        if (p_lum_state == &p_env->lum_state) {
            // main state, force change
            p_lum_state->lum   = 0;
            p_lum_state->state = ST_BRT_RISE;

            // condition for off state !
            if (!p_led_pattern->duty) {
                p_lum_state->lum = BRT_FULL_STEP / 2;
            }

            duty = tlk_lum_convert(p_lum_state->flags & FLG_BRT_REVERT_LEVEL, p_lum_state->lum);
        } else {
            // main state is flashing, determine on-to-off or off-to-on
            p_lum_state->flags |= (p_env->lum_state.lum) ? FLG_BRT_REVERT_LEVEL : 0;
            p_lum_state->lum = 0;

            // Auto off
            if (!p_led_pattern->duty) {
                p_lum_state->flags &= FLG_PATTERN_FOLLOW;
                p_lum_state->lum = BRT_FULL_STEP / 2;
            }
            p_lum_state->state = ST_BRT_RISE;

            duty = tlk_lum_convert(p_lum_state->flags & FLG_BRT_REVERT_LEVEL, p_lum_state->lum);
        }
    } else {
        // directly light up.
        if (p_led_pattern->duty == TIME_NO_TIME) {
            p_lum_state->state = ST_FLASH_LAT;
            p_lum_state->lum   = 0;
            duty               = 0;
        } else {
            p_lum_state->state = ST_FLASH_DUTY;
            p_lum_state->lum =
                (p_led_pattern->behavior & BHV_HIGH_LIHGT_FLASH) ? BRT_BRIGHTEST_FULL_DUTY : BRT_FULL_DUTY;
            duty = p_lum_state->lum;
        }
    }

    tlk_color_calibrate(p_led_ctrl, (p_lum_state->flags & FLG_PATTERN_FOLLOW) ? 0 : duty);
    return;
}

/**
 * @brief  the led breath process.
 *
 * @param[in] p_led_ctrl : led info
 *
 * @returns none
 */
static void tlk_led_breath_process(tlk_led_ctrl_t *p_led_ctrl)
{
    if (p_led_ctrl == NULL) {
        return;
    }

    // u32 duty;
    tlk_led_pattern_t *p_pattern;
    tlk_lum_state_t   *p_lum_state;
    u32                tmp;
    u16                diff;
    tlk_led_env_t     *env = &p_led_ctrl->led_env;

    p_pattern   = env->p_insert_pattern ? env->p_insert_pattern : env->p_pattern;
    p_lum_state = env->p_insert_pattern ? &env->insert_lum_state : &env->lum_state;

    // default state
    if (!p_pattern) {
        tlk_color_calibrate(p_led_ctrl, 0);
        return;
    }

    if (!(p_pattern->behavior & BHV_BREATH)) {
        return;
    }

    // breath on
    switch (p_lum_state->state) {
    case ST_BRT_RISE:
    {
        if (p_pattern->duty == TIME_NO_TIME) {
            p_lum_state->ref = clock_time();
            p_lum_state->state <<= 1;
            break;
        }

        tmp = p_pattern->duty / (BRT_FULL_STEP / 2) + 1;
        if (clock_time_exceed(p_lum_state->ref, tmp * 1000)) {
            diff             = (unsigned int)(stimer_get_tick() - p_lum_state->ref) - tmp * 1000 * SYSTEM_TIMER_TICK_1US;
            p_lum_state->ref = clock_time() - diff;
            p_lum_state->lum++;

            if (p_lum_state->lum >= (BRT_FULL_STEP / 2)) {
                p_lum_state->lum = (BRT_FULL_STEP / 2);
                p_lum_state->state <<= 1;
            }

            tlk_color_calibrate(p_led_ctrl,
                                (p_lum_state->flags & FLG_PATTERN_FOLLOW) ?
                                    0 :
                                    tlk_lum_convert(p_lum_state->flags & FLG_BRT_REVERT_LEVEL, p_lum_state->lum));
        }
    } break;

    case ST_BRT_KEEP_HIGH:
    {
        if (p_pattern->keep_high == TIME_FOREVER) {
            p_lum_state->state = ST_BRT_IDLE;
            break;
        }

        if (p_pattern->keep_high && !(clock_time_exceed(p_lum_state->ref, p_pattern->keep_high * 1000))) {
            return;
        }

        diff =
            (unsigned int)(stimer_get_tick() - p_lum_state->ref) - p_pattern->keep_high * 1000 * SYSTEM_TIMER_TICK_1US;

        p_lum_state->ref = clock_time() - diff;
        p_lum_state->state <<= 1;
    } break;

    case ST_BRT_FALL:
    {
        tmp = p_pattern->latency / (BRT_FULL_STEP / 2) + 1;
        if (clock_time_exceed(p_lum_state->ref, tmp * 1000)) {
            diff             = (unsigned int)(stimer_get_tick() - p_lum_state->ref) - tmp * 1000 * SYSTEM_TIMER_TICK_1US;
            p_lum_state->ref = clock_time() - diff;
            p_lum_state->lum++;
            tlk_color_calibrate(p_led_ctrl,
                                (p_lum_state->flags & FLG_PATTERN_FOLLOW) ?
                                    0 :
                                    tlk_lum_convert(p_lum_state->flags & FLG_BRT_REVERT_LEVEL, p_lum_state->lum));

            if (p_lum_state->lum >= BRT_FULL_STEP) {
                // next state
                p_lum_state->lum = 0;
                p_lum_state->state <<= 1;
            }
        }
    } break;

    case ST_BRT_KEEP_LOW:
    {
        if (p_pattern->keep_low && !clock_time_exceed(p_lum_state->ref, p_pattern->keep_low * 1000)) {
            return;
        }

        p_lum_state->cnt++;

        // check flash times
        if (p_pattern->flash_times == FLASH_FOREVER || p_lum_state->cnt < p_pattern->flash_times) {
            diff = (unsigned int)(stimer_get_tick() - p_lum_state->ref) -
                   p_pattern->keep_low * 1000 * SYSTEM_TIMER_TICK_1US;
            p_lum_state->state = ST_BRT_RISE;
            p_lum_state->lum   = 0;
            p_lum_state->ref   = clock_time() - diff;
            tlk_color_calibrate(p_led_ctrl,
                                (p_lum_state->flags & FLG_PATTERN_FOLLOW) ?
                                    0 :
                                    tlk_lum_convert(p_lum_state->flags & FLG_BRT_REVERT_LEVEL, p_lum_state->lum));
        } else {
            // task finished !
            p_lum_state->state = ST_BRT_IDLE;
            if (env->p_insert_pattern) {
                tlk_pattern_recovery(p_led_ctrl);
                return;
            }
        }
    } break;

    case ST_BRT_IDLE:
        break;
    default:
        break;
    }

    return;
}

/**
 * @brief  the led flash process.
 *
 * @param[in] p_led_ctrl : led info
 *
 * @returns none
 */
static void tlk_led_flash_process(tlk_led_ctrl_t *p_led_ctrl)
{
    if (p_led_ctrl == NULL) {
        return;
    }
    tlk_led_pattern_t *p_pattern;
    tlk_lum_state_t   *p_lum_state;
    tlk_led_env_t     *env = &p_led_ctrl->led_env;

    p_pattern   = env->p_insert_pattern ? env->p_insert_pattern : env->p_pattern;
    p_lum_state = env->p_insert_pattern ? &env->insert_lum_state : &env->lum_state;

    // default state
    if (!p_pattern) {
        tlk_color_calibrate(p_led_ctrl, 0);
        return;
    }

    // is LED flashing or not
    if (p_pattern->behavior & BHV_BREATH) {
        return;
    }

    switch (p_lum_state->state) {
    case ST_FLASH_DUTY:
    {
        // always on
        if (p_pattern->duty == TIME_FOREVER) {
            return;
        }

        // duty check
        if (!clock_time_exceed(p_lum_state->ref, p_pattern->duty * 1000)) {
            return;
        }

        p_lum_state->state <<= 1;
        p_lum_state->ref = clock_time();

        // Turn off LED
        p_lum_state->lum = 0;
        tlk_color_calibrate(p_led_ctrl, (p_lum_state->flags & FLG_PATTERN_FOLLOW) ? 0 : p_lum_state->lum);
    } break;

    case ST_FLASH_LAT:
    {
        // Latency time out check !
        if (!clock_time_exceed(p_lum_state->ref, p_pattern->latency * 1000)) {
            return;
        }

        // flash completed !
        p_lum_state->cnt++;

        if (p_pattern->flash_times == FLASH_FOREVER || (p_lum_state->cnt < p_pattern->flash_times)) {
            p_lum_state->state = ST_FLASH_DUTY;
            p_lum_state->ref   = clock_time();
            p_lum_state->lum   = (p_pattern->behavior & BHV_HIGH_LIHGT_FLASH) ? BRT_BRIGHTEST_FULL_DUTY : BRT_FULL_DUTY;
            tlk_color_calibrate(p_led_ctrl, (p_lum_state->flags & FLG_PATTERN_FOLLOW) ? 0 : p_lum_state->lum);
        } else {
            // change to idle state
            p_lum_state->state <<= 1;
            if (env->p_insert_pattern) {
                tlk_pattern_recovery(p_led_ctrl);
                return;
            }
        }
    } break;

    case ST_FLASH_IDLE:
        break;
    default:
        break;
    }

    return;
}

/**
 * @brief  add a new led parameters.
 *
 * @param[in] p : User Configuration Parameters
 * @param[in] new_mode : key mew mode
 *
 * @returns 0-success, else fail
 */
tlk_led_sts_e tlk_led_add(tlk_led_config_t *p)
{
    if (p == NULL) {
        tlkapi_printf(TLK_LED_DBG_FLAG, "add parameter NULL");
        return TLK_LED_PARAMETER_ERROR;
    }

    if (tlk_get_led_id_by_gpio(p->led_pin) != 0) {
        tlkapi_printf(TLK_LED_DBG_FLAG, "led exist, pin=%d", p->led_pin);
        return TLK_LED_REPEAT;
    }

    tlk_led_ctrl_t *p_temp = ledInfo;
    u8              index  = tlk_get_unused_led_index();

    if (index == TLK_LED_INVALID) {
        tlkapi_printf(TLK_LED_DBG_FLAG, "led index err");
        return TLK_LED_NOT_ENOUGH;
    }

    p_temp[index].led_id                = index + 1;
    p_temp[index].led_info.led_pin      = p->led_pin;
    p_temp[index].led_info.led_on_level = (p->led_on_level <= 1) ? p->led_on_level : TLK_LED_VALID_LEVEL;
    p_temp[index].led_info.pwmid        = p->pwmid;
    p_temp[index].color_duty            = LED_MAX_COLOR;

#if (MCU_CORE_TYPE == MCU_CORE_B91)
    pwm_set_pin(p_temp[index].led_info.led_pin);
#elif (MCU_CORE_TYPE == MCU_CORE_B92)
    pwm_set_pin(p_temp[index].led_info.led_pin, GET_PWM_GPIO_FUNC(p_temp[index].led_info.pwmid));
#endif
    pwm_clr_irq_mask(GET_PWM_FRAME_DONE_IRQ_MASK(p_temp[index].led_info.pwmid));
    pwm_clr_irq_status(GET_PWM_FRAME_DONE_IRQ_MASK(p_temp[index].led_info.pwmid));

    if (p_temp[index].led_info.pwmid == 0) {
        pwm_set_pwm0_mode(PWM_NORMAL_MODE);
    }

    pwm_set_tcmp(p_temp[index].led_info.pwmid, 0);
    pwm_set_tmax(p_temp[index].led_info.pwmid, 500);
#if (MCU_CORE_TYPE == MCU_CORE_B91 && HARDWARE_TYPE == TLSR9517CDK56D)
    pwm_n_invert_en(p_temp[index].led_info.pwmid);
#endif
    pwm_start(GET_PWM_EN_TYPE(p_temp[index].led_info.pwmid));

    tlkapi_printf(TLK_LED_DBG_FLAG, "led add success, led_id=%d", p_temp[index].led_id);
    return TLK_LED_SUCCESS;
}

/**
 * @brief  remove the led_id function.
 *
 * @param[in] led_id : led id
 *
 * @returns 0-success, else fail
 */
tlk_led_sts_e tlk_led_remove(u8 led_id)
{
    if (led_id > TLK_LED_NUM_MAX) {
        return TLK_LED_PARAMETER_ERROR;
    }

    tlk_led_ctrl_t *p_temp = ledInfo;
    u8              index  = led_id - 1;

    if (p_temp[index].led_id == 0) {
        tlkapi_printf(TLK_LED_DBG_FLAG, "led_id == 0", led_id);
        return TLK_LED_NOT_ENOUGH;
    }

    gpio_output_dis(p_temp[index].led_info.led_pin);
    gpio_input_dis(p_temp[index].led_info.led_pin);
    gpio_function_dis(p_temp[index].led_info.led_pin);

    memset(&p_temp[index], 0, sizeof(tlk_led_ctrl_t));

    tlkapi_printf(TLK_LED_DBG_FLAG, "led remove success, led_id=%d", led_id);
    return TLK_LED_SUCCESS;
}

/**
 * @brief get the led_id by GPIO.
 *
 * @param[in] gpio : led gpio
 *
 * @returns 0-fail, else led_id
 */
u8 tlk_get_led_id_by_gpio(TLK_GPIO_TYPE_DEF gpio)
{
    u8              i_for  = 0;
    tlk_led_ctrl_t *p_temp = ledInfo;

    for (i_for = 0; i_for < TLK_LED_NUM_MAX; ++i_for) {
        if (p_temp[i_for].led_info.led_pin == gpio) {
            return p_temp[i_for].led_id;
        }
    }
    tlkapi_printf(TLK_LED_DBG_FLAG, "get led_id fail");
    return TLK_LED_SUCCESS;
}

/**
 * @brief  register the led user patterns table.
 *
 * @param[in] table : the user patterns table
 * @param[in] tbl_mode_num : table counts(table max_index + 1)
 *
 * @returns 0-success, else fail
 */
tlk_led_sts_e tlk_led_user_patterns_register(tlk_led_pattern_table_t *table, u16 tbl_mode_num)
{
    if (table == NULL) {
        tlkapi_printf(TLK_LED_DBG_FLAG, "pattern table == NULL");
        return TLK_LED_PARAMETER_ERROR;
    }

    tlk_led_pattern_table_t *p_temp = &ledUserPatterns;
    if (p_temp->p_pattern_table != NULL) {
        tlkapi_printf(TLK_LED_DBG_FLAG, "p_temp->pattern_table != NULL");
        return TLK_LED_PARTTEN_ERROR;
    }

    p_temp->p_pattern_table = table->p_pattern_table;
    p_temp->pattern_count   = tbl_mode_num;

    tlkapi_printf(TLK_LED_DBG_FLAG, "user patterns register success");
    return TLK_LED_SUCCESS;
}

/**
 * @brief  remove the led user patterns table.
 *
 * @param[in] none
 *
 * @returns 0-success, else fail
 */
tlk_led_sts_e tlk_led_user_patterns_remove(void)
{
    tlk_led_pattern_table_t *p_temp = &ledUserPatterns;
    if (p_temp->p_pattern_table == NULL) {
        tlkapi_printf(TLK_LED_DBG_FLAG, "p_temp->pattern_table == NULL");
        return TLK_LED_PARAMETER_ERROR;
    }

    p_temp->p_pattern_table = NULL;
    p_temp->pattern_count   = 0;

    tlkapi_printf(TLK_LED_DBG_FLAG, "user patterns remove success");
    return TLK_LED_SUCCESS;
}

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
void tlk_set_led_color_duty(tlk_led_color_t *color)
{
    u8              i_for  = 0;
    tlk_led_ctrl_t *p_temp = &ledInfo[0];
    if (color == NULL) {
        return;
    }

    for (i_for = 0; i_for < TLK_LED_NUM_MAX; i_for++) {
        if (p_temp[i_for].led_id == 0 || color->color_value[i_for] == LED_COLOR_NOT_SET) {
            continue;
        }

        if (color->color_value[i_for] > LED_MAX_COLOR) {
            p_temp[i_for].color_duty = LED_MAX_COLOR;
        } else {
            p_temp[i_for].color_duty = color->color_value[i_for];
        }
    }
}

/**
 * @brief  get the led color duty.
 *
 * @param[out] out_color : all led color out.
 *
 * @returns none
 */
void tlk_get_led_color_duty(tlk_led_color_t *out_color)
{
    u8              i_for  = 0;
    tlk_led_ctrl_t *p_temp = &ledInfo[0];

    if (out_color == NULL) {
        return;
    }

    for (i_for = 0; i_for < TLK_LED_NUM_MAX; i_for++) {
        if (p_temp[i_for].led_id == 0) {
            continue;
        }
        out_color->color_value[i_for] = p_temp[i_for].color_duty;
    }
}

/**
 * @brief  get led pattern duty.
 *
 * @param[in] led_id
 *
 * @returns pattern duty
 */
u16 tlk_led_get_pattern_duty(int led_id)
{
    if (led_id == 0 || led_id > TLK_LED_NUM_MAX) {
        tlkapi_printf(TLK_LED_DBG_FLAG, "led_id err");
        return TLK_LED_PARAMETER_ERROR;
    }

    tlk_led_ctrl_t *p_temp = &ledInfo[led_id - 1];

    if (p_temp->led_env.p_insert_pattern != NULL) {
        return p_temp->led_env.p_insert_pattern->duty;
    } else if (p_temp->led_env.p_pattern != NULL) {
        return p_temp->led_env.p_pattern->duty;
    }

    return TLK_LED_SUCCESS;
}

/**
 * @brief  get led current_lum.
 *
 * @param[in] led_id
 *
 * @returns current lum
 */
u16 tlk_led_get_current_lum(int led_id)
{
    if (led_id == 0 || led_id > TLK_LED_NUM_MAX) {
        tlkapi_printf(TLK_LED_DBG_FLAG, "get led_id err");
        return TLK_LED_PARAMETER_ERROR;
    }

    tlk_led_ctrl_t *p_temp = &ledInfo[led_id - 1];

    if (p_temp->led_env.p_insert_pattern != NULL) {
        return p_temp->led_env.insert_lum_state.lum;
    } else if (p_temp->led_env.p_pattern != NULL) {
        return p_temp->led_env.lum_state.lum;
    }

    return TLK_LED_SUCCESS;
}

/**
 * @brief  set the led pattern.
 *
 * @param[in] led_id : led id
 * @param[in] patt : led pattern the type is 'tlk_led_pattern_t'
 * @param[in] is_user_patt : the patt is defined by user or default.
 *
 * @returns 0-success, else fail
 */
tlk_led_sts_e tlk_led_pattern_set(int led_id, int patt, tlk_led_patt_from_e is_user_patt)
{
    if (led_id == 0 || led_id > TLK_LED_NUM_MAX) {
        tlkapi_printf(TLK_LED_DBG_FLAG, "set led_id err");
        return TLK_LED_PARAMETER_ERROR;
    }

    tlk_led_pattern_t *p_pattern = NULL;
    tlk_led_ctrl_t    *p_temp    = &ledInfo[led_id - 1];

    if (is_user_patt == USER_PATTERN) {
        if (ledUserPatterns.p_pattern_table == NULL || patt >= ledUserPatterns.pattern_count) {
            tlkapi_printf(TLK_LED_DBG_FLAG, "set pattern err1");
            return TLK_LED_PARTTEN_ERROR;
        }

        p_pattern = &ledUserPatterns.p_pattern_table[patt];
    } else // is_user_patt==DEFAULT_PATTERN
    {
        if (patt >= LED_PATTERN_LED_MAX_STATE) {
            tlkapi_printf(TLK_LED_DBG_FLAG, "set pattern err2");
            return TLK_LED_PARTTEN_ERROR;
        }
        p_pattern = (tlk_led_pattern_t *)(size_t)&ledDfltPatterns[patt];
    }
    p_temp->led_env.p_pattern = p_pattern;
    //  if(p_temp->led_env.p_insert_pattern != NULL)
    //  {
    //      u8 i = 0;
    //      //Judge whether p_insert_pattern is valid ?
    //      for(i = 0; i<LED_PATTERN_LED_MAX_STATE; i++)
    //      {
    //          if(p_temp->led_env.p_insert_pattern == (&ledDfltPatterns[i]))
    //          {
    //              break; //valid
    //          }
    //      }
    //      if(i >= LED_PATTERN_LED_MAX_STATE)
    //      {
    //          &ledDfltPatterns[patt] = NULL; //invalid
    //      }
    //  }
    tlk_lum_state_init(p_temp, &p_temp->led_env.lum_state, p_temp->led_env.p_pattern, 0);
    tlkapi_printf(TLK_LED_DBG_FLAG, "set led=%d, pattern=%d,is_user_patt=%d", led_id, patt, is_user_patt);
    // tlkapi_send_string_u32s(LED_USB_DBG_EN, "tlk_led_pattern_set: led_idx:pattern", led_id | patt<<8, 0, 0, 0);
    return TLK_LED_SUCCESS;
}

/**
 * @brief  insert a led pattern.
 *
 * @param[in] led_id : led id
 * @param[in] patt : led pattern the type is 'tlk_led_pattern_t'
 * @param[in] is_user_patt : the patt is defined by user or default.
 *
 * @returns 0-success, else fail
 */
tlk_led_sts_e tlk_led_pattern_insert(int led_id, int patt, tlk_led_patt_from_e is_user_patt)
{
    if (led_id == 0 || led_id > TLK_LED_NUM_MAX) {
        tlkapi_printf(TLK_LED_DBG_FLAG, "insert led_id err");
        return TLK_LED_PARAMETER_ERROR;
    }

    tlk_led_pattern_t *p_pattern = NULL;
    tlk_led_ctrl_t    *p_temp    = &ledInfo[led_id - 1];

    if (is_user_patt == USER_PATTERN) {
        if (ledUserPatterns.p_pattern_table == NULL || patt >= ledUserPatterns.pattern_count) {
            tlkapi_printf(TLK_LED_DBG_FLAG, "insert pattern err1");
            return TLK_LED_PARTTEN_ERROR;
        }

        p_pattern = &ledUserPatterns.p_pattern_table[patt];
    } else // is_user_patt==DEFAULT_PATTERN
    {
        if (patt >= LED_PATTERN_LED_MAX_STATE) {
            tlkapi_printf(TLK_LED_DBG_FLAG, "insert pattern err2");
            return TLK_LED_PARTTEN_ERROR;
        }
        p_pattern = (tlk_led_pattern_t *)(size_t)&ledDfltPatterns[patt];
    }
    if (p_pattern->duty == TIME_FOREVER || p_pattern->latency == TIME_FOREVER ||
        p_pattern->flash_times == TIME_FOREVER || p_pattern->keep_high == TIME_FOREVER ||
        p_pattern->keep_low == TIME_FOREVER) {
        tlkapi_printf(TLK_LED_DBG_FLAG, "param forever, please use tlk_led_pattern_set");
        return TLK_LED_INSERT_CAUTION;
    }

    p_temp->led_env.p_insert_pattern = p_pattern;

    tlk_lum_state_init(p_temp, &p_temp->led_env.insert_lum_state, p_temp->led_env.p_insert_pattern, 0);
    tlkapi_printf(TLK_LED_DBG_FLAG, "insert led=%d, pattern=%d,is_user_patt=%d", led_id, patt, is_user_patt);
    return TLK_LED_SUCCESS;
}

/**
 * @brief  get led status - led_is_insert_pattern_running.
 *
 * @param[in] led_id
 *
 * @returns 0-not_insert, 1-insert, 2-led err
 */
tlk_led_sts_e tlk_led_is_insert_pattern_running(int led_id)
{
    if (led_id == 0 || led_id > TLK_LED_NUM_MAX) {
        tlkapi_printf(TLK_LED_DBG_FLAG, "running led_id err");
        return TLK_LED_PARAMETER_ERROR;
    }

    tlk_led_ctrl_t *p_temp = &ledInfo[led_id - 1];

    if (p_temp->led_env.p_insert_pattern != NULL) {
        return TLK_LED_IS_INSERTING;
    }

    return TLK_LED_SUCCESS;
}

/**
 * @brief  led breath and flash process.
 *
 * @param[in] none
 *
 * @returns none
 */
void tlk_led_refresh_process(void)
{
    u16             index  = 0;
    tlk_led_ctrl_t *p_temp = &ledInfo[0];
    if (TLK_LED_NUM_MAX == 0xFFFF) {
        return;
    }

    for (index = 0; index < TLK_LED_NUM_MAX; index++) {
        if (p_temp[index].led_id == 0) {
            continue;
        }
        tlk_led_breath_process(&p_temp[index]);
        tlk_led_flash_process(&p_temp[index]);
    }
}
