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
#include "app_async.h"
#include "vendor/common/tlk_api/tlk_tone.h"
#include "vendor/common/tlk_api/tlk_key.h"

#if (UNICAST_SERVER_SELECT == UNICAST_SERVER_ASYNC)
#if (TLK_KEY_ENABLE)

static int key_func_null_func(void);
static int key_func_switch(void);
static int key_func_test2(void);
static int key_func_test3(void);
static int key_func_test4(void);
static int key_func_test5(void);

/** key event table */
static const tlk_key_func s_key_evt_table[KEY_MODE_MAX] = {
    key_func_null_func,                       /** KEY_MODE_NULL */
    key_func_switch,                          /** KEY_MODE_SWITCH */
    key_func_test2,                           /** KEY_MODE_TEST2 */
    key_func_test3,                           /** KEY_MODE_TEST3 */
    key_func_test4,                           /** KEY_MODE_TEST4 */
    key_func_test5,                           /** KEY_MODE_TEST5 */
};

/** key mode table 1 */
static tlk_key_event_class_t key_mode_table_1 = {
  ._short  = KEY_MODE_SWITCH,
  ._dclick = KEY_MODE_NULL,
  ._tclick = KEY_MODE_NULL,
  ._hold   = KEY_MODE_NULL,
  ._long   = KEY_MODE_NULL,
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
static tlk_key_event_class_t __attribute__((unused)) key_mode_table_5 = {
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

static int key_func_switch(void)
{
    tlkapi_printf(APP_LOG_EN, "key_func_switch\n");

#if (0)
    blc_async_message_t message;
    message.type = TYPE_SYNC;
    message.opcode = OPCODE_SWITCH;//OPCODE_LED;

//  test use
//  message.data[0]=0x01;
//  message.data[1]=0x02;
//  message.data[2]=0x03;
//  message.data[3]=0x04;
//  message.data[4]=0x05;
//  message.data[5]=0x06;
//  message.data[6]=0x07;
//  message.data[7]=0x08;
//  message.data[8]=0x09;
//  message.data[9]=0x0a;
    blc_async_push_message(500*1000,&message);
#else
    extern void app_key_switch_boot(void);
    app_key_switch_boot();
#endif

    return 0;
}

static int key_func_test2(void)
{
    tlkapi_printf(APP_LOG_EN, "key_func_test2\n");
    app_tone_play(TLK_TONE_RING);
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

void app_key_async(u8* data)
{
    tlkapi_printf(APP_LOG_EN, "[APP][KEY][ASYNC]\n");
    tlkapi_send_string_data(1,"data",data,10);
}

void app_key_sync(u8* data)
{
    tlkapi_printf(APP_LOG_EN, "[APP][KEY][SYNC]\n");
    tlkapi_send_string_data(1,"data",data,10);
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
    int ret = 0;
#ifdef KEY1_ID
    /** add key1 */
    key_config.key_pin           = KEY1_GPIO_IN;
    key_config.key_out_pin       = KEY1_GPIO_OUT;
    key_config.key_down_level    = TLK_KEY_VALID_LEVEL;
    key_config.key_hold_cnt      = 0;
    key_config.key_long_cnt      = 200;
    key_config.key_intervel_cnt  = 40;
    ret = tlk_key_add(&key_config);
    if(ret!=TLK_KEY_SUCCESS)
    {
        tlkapi_printf(APP_LOG_EN, "key_1 init fail,ret - %d\n",ret);
    }
    else
    {
        tlkapi_printf(APP_LOG_EN, "key_1 init success\n");
        if(KEY1_ID!=0)
        {
            tlk_key_evt_mode_register(KEY1_ID, &key_mode_table_1);
        }
    }
#endif

#ifdef KEY2_ID
    /** add key2 */
    key_config.key_pin           = KEY2_GPIO_IN;
    key_config.key_out_pin       = KEY2_GPIO_OUT;
    key_config.key_down_level    = TLK_KEY_VALID_LEVEL;
    key_config.key_hold_cnt      = 0;
    key_config.key_long_cnt      = 200;
    key_config.key_intervel_cnt  = 40;
    ret = tlk_key_add(&key_config);
    if(ret!=TLK_KEY_SUCCESS)
    {
        tlkapi_printf(APP_LOG_EN, "key_2 init fail,ret - %d\n",ret);
    }
    else
    {
        tlkapi_printf(APP_LOG_EN, "key_2 init success\n");
        if(KEY2_ID!=0)
        {
            tlk_key_evt_mode_register(KEY2_ID, &key_mode_table_2);
        }
    }
#endif

#ifdef KEY3_ID
    /** add key3 */
    key_config.key_pin           = KEY3_GPIO_IN;
    key_config.key_out_pin       = KEY3_GPIO_OUT;
    key_config.key_down_level    = TLK_KEY_VALID_LEVEL;
    key_config.key_hold_cnt      = 0;
    key_config.key_long_cnt      = 200;
    key_config.key_intervel_cnt  = 40;
    ret = tlk_key_add(&key_config);
    if(ret!=TLK_KEY_SUCCESS)
    {
        tlkapi_printf(APP_LOG_EN, "key_3 init fail,ret - %d\n",ret);
    }
    else
    {
        tlkapi_printf(APP_LOG_EN, "key_3 init success\n");
        if(KEY3_ID!=0)
        {
            tlk_key_evt_mode_register(KEY3_ID, &key_mode_table_3);
        }
    }
#endif

#ifdef KEY4_ID
    /** add key4 */
    key_config.key_pin           = KEY4_GPIO_IN;
    key_config.key_out_pin       = KEY4_GPIO_OUT;
    key_config.key_down_level    = TLK_KEY_VALID_LEVEL;
    key_config.key_hold_cnt      = 0;
    key_config.key_long_cnt      = 200;
    key_config.key_intervel_cnt  = 40;
    ret = tlk_key_add(&key_config);
    if(ret!=TLK_KEY_SUCCESS)
    {
        tlkapi_printf(APP_LOG_EN, "key_4 init fail,ret - %d\n",ret);
    }
    else
    {
        tlkapi_printf(APP_LOG_EN, "key_4 init success\n");
        if(KEY4_ID!=0)
        {
            tlk_key_evt_mode_register(KEY4_ID, &key_mode_table_4);
        }
    }
#endif

}

#endif   //end of TLK_KEY_ENABLE
#endif
