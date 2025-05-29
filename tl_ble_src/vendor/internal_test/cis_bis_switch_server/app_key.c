/********************************************************************************************************
 * @file    app_ui.c
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
#include "app_audio.h"
#include "app_key.h"


#if (INTER_TEST_MODE == TEST_LE_AUDIO_SWTICH_SERVER)
#if (TLK_TONE_ENABLE)
    #include "tlk_api/tlk_tone.h"
#endif
    #if (TLK_KEY_ENABLE)

static int key_func_null_func(void);
static int key_func_test1(void);
static int key_func_test2(void);
static int key_func_test3(void);
static int key_func_test4(void);
static int key_func_test5(void);

/** key event table */
static const tlk_key_func s_key_evt_table[KEY_MODE_MAX] = {
    key_func_null_func, /**  */
    key_func_test1,     /**  */
    key_func_test2,     /**  */
    key_func_test3,     /**  */
    key_func_test4,     /**  */
    key_func_test5,     /**  */
};

/** key mode table 1 */
static tlk_key_event_class_t key_mode_table_1 = {
    ._short  = KEY_MODE_TEST1,
    ._dclick = KEY_MODE_TEST1,
    ._tclick = KEY_MODE_TEST1,
    ._hold   = KEY_MODE_TEST1,
    ._long   = KEY_MODE_TEST1,
};

/** key mode table 2*/
static tlk_key_event_class_t key_mode_table_2 = {
    ._short  = KEY_MODE_TEST2,
    ._dclick = KEY_MODE_TEST2,
    ._tclick = KEY_MODE_TEST2,
    ._hold   = KEY_MODE_TEST2,
    ._long   = KEY_MODE_TEST2,
};

/** key mode table 3*/
static tlk_key_event_class_t key_mode_table_3 = {
    ._short  = KEY_MODE_TEST3,
    ._dclick = KEY_MODE_TEST3,
    ._tclick = KEY_MODE_TEST3,
    ._hold   = KEY_MODE_TEST3,
    ._long   = KEY_MODE_TEST3,
};

/** key mode table 4*/
static tlk_key_event_class_t key_mode_table_4 = {
    ._short  = KEY_MODE_TEST4,
    ._dclick = KEY_MODE_TEST4,
    ._tclick = KEY_MODE_TEST4,
    ._hold   = KEY_MODE_TEST4,
    ._long   = KEY_MODE_TEST4,
};

/** key mode table 5*/
static tlk_key_event_class_t key_mode_table_5 = {
    ._short  = KEY_MODE_TEST5,
    ._dclick = KEY_MODE_TEST5,
    ._tclick = KEY_MODE_TEST5,
    ._hold   = KEY_MODE_TEST5,
    ._long   = KEY_MODE_TEST5,
};

static int key_func_null_func(void)
{
    tlkapi_printf(APP_LOG_EN, "key_func_null_func\n");
    return 0;
}
        #if (TLK_TONE_ENABLE)
static int i = 0;
        #endif
static int key_func_test1(void)
{
    tlkapi_printf(APP_LOG_EN, "key_func_test1\n");
        #if (TLK_TONE_ENABLE)
    tlk_tone_play(i++ % 6);
        #endif
    return 0;
}

static int key_func_test2(void)
{
    tlkapi_printf(APP_LOG_EN, "key_func_test2\n");
    return 0;
}

static int key_func_test3(void)
{
    tlkapi_printf(APP_LOG_EN, "key_func_test3\n");
    return 0;
}

static int key_func_test4(void)
{
    tlkapi_printf(APP_LOG_EN, "key_func_test4\n");
    return 0;
}

static int key_func_test5(void)
{
    tlkapi_printf(APP_LOG_EN, "key_func_test5\n");
    return 0;
}

/**
 * @brief     This function serves to init the key function.
 *
 * @param[in] None.
 *
 * @returns   None.
 */
void app_key_init(void)
{
    /** key event table register */
    tlk_key_register_event_table(s_key_evt_table, KEY_MODE_MAX);
    tlk_key_config_t key_config;
    int              ret = 0;
        #ifdef KEY1_ID
    /** add key1 */
    key_config.key_pin          = KEY1_GPIO_IN;
    key_config.key_out_pin      = KEY1_GPIO_OUT;
    key_config.key_down_level   = TLK_KEY_VALID_LEVEL;
    key_config.key_hold_cnt     = 0;
    key_config.key_long_cnt     = 200;
    key_config.key_intervel_cnt = 40;
    ret                         = tlk_key_add(&key_config);
    if (ret != TLK_KEY_SUCCESS) {
        tlkapi_printf(APP_LOG_EN, "key_1 init fail,ret - %d\n", ret);
    } else {
        tlkapi_printf(APP_LOG_EN, "key_1 init success\n");
        if (KEY1_ID != 0) {
            tlk_key_evt_mode_register(KEY1_ID, &key_mode_table_1);
        }
    }
        #endif

        #ifdef KEY2_ID
    /** add key2 */
    key_config.key_pin          = KEY2_GPIO_IN;
    key_config.key_out_pin      = KEY2_GPIO_OUT;
    key_config.key_down_level   = TLK_KEY_VALID_LEVEL;
    key_config.key_hold_cnt     = 0;
    key_config.key_long_cnt     = 200;
    key_config.key_intervel_cnt = 40;
    ret                         = tlk_key_add(&key_config);
    if (ret != TLK_KEY_SUCCESS) {
        tlkapi_printf(APP_LOG_EN, "key_2 init fail,ret - %d\n", ret);
    } else {
        tlkapi_printf(APP_LOG_EN, "key_2 init success\n");
        if (KEY2_ID != 0) {
            tlk_key_evt_mode_register(KEY2_ID, &key_mode_table_2);
        }
    }
        #endif

        #ifdef KEY3_ID
    /** add key3 */
    key_config.key_pin          = KEY3_GPIO_IN;
    key_config.key_out_pin      = KEY3_GPIO_OUT;
    key_config.key_down_level   = TLK_KEY_VALID_LEVEL;
    key_config.key_hold_cnt     = 0;
    key_config.key_long_cnt     = 200;
    key_config.key_intervel_cnt = 40;
    ret                         = tlk_key_add(&key_config);
    if (ret != TLK_KEY_SUCCESS) {
        tlkapi_printf(APP_LOG_EN, "key_3 init fail,ret - %d\n", ret);
    } else {
        tlkapi_printf(APP_LOG_EN, "key_3 init success\n");
        if (KEY3_ID != 0) {
            tlk_key_evt_mode_register(KEY3_ID, &key_mode_table_3);
        }
    }
        #endif

        #ifdef KEY4_ID
    /** add key4 */
    key_config.key_pin          = KEY4_GPIO_IN;
    key_config.key_out_pin      = KEY4_GPIO_OUT;
    key_config.key_down_level   = TLK_KEY_VALID_LEVEL;
    key_config.key_hold_cnt     = 0;
    key_config.key_long_cnt     = 200;
    key_config.key_intervel_cnt = 40;
    ret                         = tlk_key_add(&key_config);
    if (ret != TLK_KEY_SUCCESS) {
        tlkapi_printf(APP_LOG_EN, "key_4 init fail,ret - %d\n", ret);
    } else {
        tlkapi_printf(APP_LOG_EN, "key_4 init success\n");
        if (KEY4_ID != 0) {
            tlk_key_evt_mode_register(KEY4_ID, &key_mode_table_4);
        }
    }
        #endif
}

    #endif //end of TLK_KEY_ENABLE
#endif
