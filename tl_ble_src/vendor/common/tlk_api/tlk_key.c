/********************************************************************************************************
 * @file    tlk_key.c
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
#include "tlk_key.h"

/** default key event cnt time unit:10ms */
#define TLK_KEY_LONG_CNT     185
#define TLK_KEY_HOLD_CNT     0
#define TLK_KEY_INTERVAL     42
#define TLK_KEY_DOWN_LEVEL   TLK_KEY_VALID_LEVEL
#define TLK_KEY_MIN_FILTER   4
#define TLK_KEY_MAX_DOWN_CNT 50

/** key event min value .*/
#define MIN_LONG_CNT      100
#define MIN_HOLD_CNT      100
#define MIN_LONG_LONG_CNT 400
#define MIN_INTERVAL      10

/** Key scan interval unit:us */
#define KEY_SCAN_INTERVAL (10000)

static tlk_key_ctl_t        tlkKeyInfo[TLK_KEY_NUM_MAX]  = {0};
static tlk_key_event_t      tlkKeyEvent[TLK_KEY_NUM_MAX] = {0};
static tlk_key_func_table_t tlkKeyFuncTable              = {0};
static tlk_key_func         tlk_key_scan_not_execte_cb   = NULL;

static u8 tlk_key_check_release(u8 index);

tlk_key_sts_e tlk_key_add(tlk_key_config_t *p)
{
    if (p == NULL) {
        return TLK_KEY_PARAMETER_ERROR;
    }
    if (tlk_key_get_id(p->key_pin, p->key_out_pin) != 0) {
        return TLK_KEY_REPEAT;
    }
    tlk_key_ctl_t *pKey  = tlkKeyInfo;
    u8             index = tlk_key_get_empty_index();
    if (index == TLK_KEY_INVALID) {
        return TLK_KEY_NOT_ENOUGH;
    }
    pKey[index].key_id                    = index + 1;
    pKey[index].key_down_cnt              = 0;
    pKey[index].key_up_cnt                = 0;
    pKey[index].key_state                 = KEY_STATE_DOWN;
    pKey[index].key_info.key_pin          = p->key_pin;
    pKey[index].key_info.key_out_pin      = p->key_out_pin;
    pKey[index].key_info.key_down_level   = ((p->key_down_level <= 1) ? p->key_down_level : TLK_KEY_DOWN_LEVEL);
    pKey[index].key_info.key_hold_cnt     = ((p->key_hold_cnt >= MIN_HOLD_CNT) ? p->key_hold_cnt : 0);
    pKey[index].key_info.key_long_cnt     = ((p->key_long_cnt >= MIN_LONG_CNT) ? p->key_long_cnt : TLK_KEY_LONG_CNT);
    pKey[index].key_info.key_intervel_cnt = ((p->key_intervel_cnt >= MIN_INTERVAL) ? p->key_intervel_cnt : MIN_INTERVAL);

    tlkapi_printf(TLK_KEY_DEBUG_EN, "[key]:key[%d] add success\n", pKey[index].key_id);
    return TLK_KEY_SUCCESS;
}

tlk_key_sts_e tlk_key_remove(u8 key_id)
{
    if (key_id > TLK_KEY_NUM_MAX || key_id == 0) {
        return TLK_KEY_PARAMETER_ERROR;
    }
    tlk_key_ctl_t *pKey = tlkKeyInfo;
    for (u8 i = 0; i < TLK_KEY_NUM_MAX; ++i) {
        if (key_id == pKey[i].key_id) {
            memset(&pKey[i].key_id, 0, sizeof(tlk_key_ctl_t));
            tlkapi_printf(TLK_KEY_DEBUG_EN, "[key]:key[%d] remove success\n", pKey[i].key_id);
            return TLK_KEY_SUCCESS;
        }
    }
    return TLK_KEY_NOT_FOUND;
}

void tlk_key_scan(void)
{
    static u32       key_scan_tick          = 0;
    tlk_key_ctl_t   *pKeyInfo               = tlkKeyInfo;
    tlk_key_event_t *pKeyEvent              = tlkKeyEvent;
    u32              diff_tick              = 0;
    u32              key_block_cnt          = 0;
    u32              key_block_remain_tick  = 0;
    u32              key_scan_interval_tick = KEY_SCAN_INTERVAL * SYSTEM_TIMER_TICK_1US;
    if (tlk_key_scan_not_execte_cb) {
        if (tlk_key_scan_not_execte_cb()) {
            return;
        }
    }
    if (clock_time_exceed(key_scan_tick, KEY_SCAN_INTERVAL)) {
        if (key_scan_tick) {
            diff_tick             = stimer_get_tick() - key_scan_tick;
            key_block_remain_tick = diff_tick % key_scan_interval_tick;
            if (diff_tick > key_scan_interval_tick) {
                key_block_cnt = (diff_tick - key_scan_interval_tick) / key_scan_interval_tick;
            } else {
                key_block_cnt = 0;
            }
        }
        key_scan_tick = (stimer_get_tick() - key_block_remain_tick) | 1;
        for (u8 i = 0; i < TLK_KEY_NUM_MAX; ++i) {
            if (pKeyInfo[i].key_id == 0) {
                continue;
            }
            if (tlk_key_check_release(i)) //key release
            {
                if (pKeyInfo[i].key_down_cnt > TLK_KEY_MIN_FILTER && pKeyInfo[i].key_down_cnt < TLK_KEY_MAX_DOWN_CNT && pKeyInfo[i].key_state != KEY_STATE_HOLD && pKeyInfo[i].key_state != KEY_STATE_LONG) {
                    pKeyInfo[i].key_click_cnt++;
                }
                if (pKeyInfo[i].key_state == KEY_STATE_LONG && pKeyInfo[i].key_click_cnt == 0 && pKeyInfo[i].key_down_cnt > pKeyInfo[i].key_info.key_long_cnt) {
                    pKeyEvent[i].evt_occur._long = 1;
                    tlkapi_printf(TLK_KEY_DEBUG_EN, "[key]:key[%d] long click\n", pKeyInfo[i].key_id);
                }
                pKeyInfo[i].key_down_cnt = 0;
                if (pKeyInfo[i].key_state == KEY_STATE_RELEASE && pKeyInfo[i].key_click_cnt == 0) {
                    continue;
                }
                pKeyInfo[i].key_up_cnt++;
                pKeyInfo[i].key_up_cnt += key_block_cnt;
                //set event by click conut
                //tlk_printf("tlk_key_process key_id: %d, key_click_cnt: %d\r\n", i+1, pKeyInfo[i].key_click_cnt);
                if (pKeyInfo[i].key_click_cnt) {
                    if (pKeyInfo[i].key_up_cnt > pKeyInfo[i].key_info.key_intervel_cnt) {
                        if (pKeyInfo[i].key_click_cnt == 1) {
                            pKeyEvent[i].evt_occur._short = 1;
                            tlkapi_printf(TLK_KEY_DEBUG_EN, "[key]:key[%d] short click\n", pKeyInfo[i].key_id);
                        } else if (pKeyInfo[i].key_click_cnt == 2) {
                            pKeyEvent[i].evt_occur._dclick = 1;
                            tlkapi_printf(TLK_KEY_DEBUG_EN, "[key]:key[%d] double click\n", pKeyInfo[i].key_id);
                        } else if (pKeyInfo[i].key_click_cnt >= 3) {
                            pKeyEvent[i].evt_occur._tclick = 1;
                            tlkapi_printf(TLK_KEY_DEBUG_EN, "[key]:key[%d] trible click\n", pKeyInfo[i].key_id);
                        }
                        pKeyInfo[i].key_click_cnt = 0;
                        pKeyInfo[i].key_state     = KEY_STATE_RELEASE;
                    }
                }
                if (pKeyInfo[i].key_up_cnt > TLK_KEY_MIN_FILTER && pKeyInfo[i].key_state != KEY_STATE_RELEASE) {
                    pKeyInfo[i].key_state = KEY_STATE_RELEASE;
                }
            } else //key press down
            {
                if (pKeyInfo[i].key_state != KEY_STATE_DOWN) {
                    pKeyInfo[i].key_down_cnt++;
                    pKeyInfo[i].key_down_cnt += key_block_cnt;
                    pKeyInfo[i].key_up_cnt = 0;

                    if ((pKeyInfo[i].key_info.key_hold_cnt == 0) &&
                        (pKeyInfo[i].key_info.key_long_cnt == 0)) {
                        if (pKeyInfo[i].key_down_cnt > TLK_KEY_MIN_FILTER) {
                            pKeyInfo[i].key_down_cnt = 0;
                            pKeyInfo[i].key_state    = KEY_STATE_DOWN;
                            pKeyInfo[i].key_click_cnt++;
                        }
                    } else //hold or long process
                    {
                        if (pKeyInfo[i].key_click_cnt == 0) {
                            if (pKeyInfo[i].key_info.key_hold_cnt != 0 && (pKeyInfo[i].key_down_cnt >= pKeyInfo[i].key_info.key_hold_cnt)) {
                                pKeyInfo[i].key_click_cnt    = 0;
                                pKeyInfo[i].key_down_cnt     = 0; // loop trigger
                                pKeyInfo[i].key_state        = KEY_STATE_HOLD;
                                pKeyEvent[i].evt_occur._hold = 1;
                                tlkapi_printf(TLK_KEY_DEBUG_EN, "[key]:key[%d] hold click\n", pKeyInfo[i].key_id);
                            } else if (pKeyInfo[i].key_info.key_long_cnt != 0 && (pKeyInfo[i].key_down_cnt >= pKeyInfo[i].key_info.key_long_cnt)) {
                                goto LONG_PROCESS;
                            }
                        } else {
                            if (pKeyInfo[i].key_down_cnt > TLK_KEY_MIN_FILTER) {
                                //no process
                            }

                            if (pKeyInfo[i].key_down_cnt >= pKeyInfo[i].key_info.key_long_cnt && pKeyInfo[i].key_info.key_long_cnt != 0) {
LONG_PROCESS:
                                pKeyInfo[i].key_click_cnt = 0;
                                pKeyInfo[i].key_state     = KEY_STATE_LONG;
                            }
                        }
                    }
                }
            }
        }
    }
}

void tlk_key_process(void)
{
    tlk_key_event_t *pKeyEvent = tlkKeyEvent;

    for (u8 i = 0; i < TLK_KEY_NUM_MAX; ++i) {
        if (pKeyEvent[i].evt_occur.flg) {
            tlk_printf("[key]:key process key_id: %d, flg: %d\n", i + 1, pKeyEvent[i].evt_occur.flg);
            if (pKeyEvent[i].evt_occur._short && pKeyEvent[i].mode._short) {
                tlkKeyFuncTable.p_func[pKeyEvent[i].mode._short]();
                tlkapi_printf(TLK_KEY_DEBUG_EN, "[key]:key[%d] short event\n", tlkKeyInfo[i].key_id);
            }
            if (pKeyEvent[i].evt_occur._dclick && pKeyEvent[i].mode._dclick) {
                tlkKeyFuncTable.p_func[pKeyEvent[i].mode._dclick]();
                tlkapi_printf(TLK_KEY_DEBUG_EN, "[key]:key[%d] double event\n", tlkKeyInfo[i].key_id);
            }
            if (pKeyEvent[i].evt_occur._tclick && pKeyEvent[i].mode._tclick) {
                tlkKeyFuncTable.p_func[pKeyEvent[i].mode._tclick]();
                tlkapi_printf(TLK_KEY_DEBUG_EN, "[key]:key[%d] trible event\n", tlkKeyInfo[i].key_id);
            }
            if (pKeyEvent[i].evt_occur._hold && pKeyEvent[i].mode._hold) {
                tlkKeyFuncTable.p_func[pKeyEvent[i].mode._hold]();
                tlkapi_printf(TLK_KEY_DEBUG_EN, "[key]:key[%d] hold event\n", tlkKeyInfo[i].key_id);
            }
            if (pKeyEvent[i].evt_occur._long && pKeyEvent[i].mode._long) {
                tlkKeyFuncTable.p_func[pKeyEvent[i].mode._long]();
                tlkapi_printf(TLK_KEY_DEBUG_EN, "[key]:key[%d] long event\n", tlkKeyInfo[i].key_id);
            }

            pKeyEvent[i].evt_occur.flg = 0;
        }
    }
}

u8 tlk_key_get_id(TLK_GPIO_TYPE_DEF gpio, TLK_GPIO_TYPE_DEF out_gpio)
{
    tlk_key_ctl_t *pKey = tlkKeyInfo;
    for (u8 i = 0; i < TLK_KEY_NUM_MAX; ++i) {
        if (out_gpio == 0) {
            if (pKey[i].key_info.key_pin == gpio) {
                return pKey[i].key_id;
            }
        } else {
            if ((pKey[i].key_info.key_pin == gpio && pKey[i].key_info.key_out_pin == out_gpio) ||
                (pKey[i].key_info.key_pin == out_gpio && pKey[i].key_info.key_out_pin == gpio)) {
                return pKey[i].key_id;
            }
        }
    }
    return 0;
}

u8 tlk_key_get_empty_index(void)
{
    tlk_key_ctl_t *pKey = tlkKeyInfo;
    for (u8 i = 0; i < TLK_KEY_NUM_MAX; ++i) {
        if (pKey[i].key_id == 0) {
            return i;
        }
    }
    return TLK_KEY_INVALID;
}

static u8 tlk_key_check_release(u8 index)
{
    tlk_key_ctl_t *pKey    = tlkKeyInfo;
    u8             ret     = 0;
    u8             key_val = 0;
    if (pKey[index].key_info.key_out_pin == 0) {
        gpio_set_gpio_en(pKey[index].key_info.key_pin);
        gpio_set_input_en(pKey[index].key_info.key_pin, 1);
        gpio_set_output_en(pKey[index].key_info.key_pin, 0);
        if (pKey[index].key_info.key_down_level) {
            gpio_set_up_down_res(pKey[index].key_info.key_pin, GPIO_PIN_PULLDOWN_100K);
        } else {
            gpio_set_up_down_res(pKey[index].key_info.key_pin, GPIO_PIN_PULLUP_10K);
        }

        key_val = gpio_read(pKey[index].key_info.key_pin) ? 1 : 0;
        ret     = (key_val != pKey[index].key_info.key_down_level);

    } else {
        gpio_set_level(pKey[index].key_info.key_pin, !pKey[index].key_info.key_down_level);

        /** set key_pin  input */
        gpio_set_gpio_en(pKey[index].key_info.key_pin);
        gpio_set_input_en(pKey[index].key_info.key_pin, 1);
        gpio_set_output_en(pKey[index].key_info.key_pin, 0);

        /** set key_out_pin  output */
        gpio_set_gpio_en(pKey[index].key_info.key_out_pin);
        gpio_set_input_en(pKey[index].key_info.key_out_pin, 0);
        gpio_set_output_en(pKey[index].key_info.key_out_pin, 1);

        /** set key_out_pin  out level */
        gpio_set_level(pKey[index].key_info.key_out_pin, pKey[index].key_info.key_down_level);

        if (pKey[index].key_info.key_down_level) {
            gpio_set_up_down_res(pKey[index].key_info.key_pin, GPIO_PIN_PULLDOWN_100K);
        } else {
            gpio_set_up_down_res(pKey[index].key_info.key_pin, GPIO_PIN_PULLUP_10K);
        }
        key_val = gpio_read(pKey[index].key_info.key_pin);
        if (key_val != pKey[index].key_info.key_down_level) {
            ret = 1;
        } else {
            gpio_set_level(pKey[index].key_info.key_out_pin, !pKey[index].key_info.key_down_level);

            /** set key_pin  output */
            gpio_set_gpio_en(pKey[index].key_info.key_pin);
            gpio_set_input_en(pKey[index].key_info.key_pin, 0);
            gpio_set_output_en(pKey[index].key_info.key_pin, 1);

            /** set key_out_pin  input */
            gpio_set_gpio_en(pKey[index].key_info.key_out_pin);
            gpio_set_input_en(pKey[index].key_info.key_out_pin, 1);
            gpio_set_output_en(pKey[index].key_info.key_out_pin, 0);

            /** set key_pin  out level */
            gpio_set_level(pKey[index].key_info.key_pin, pKey[index].key_info.key_down_level);

            if (pKey[index].key_info.key_down_level) {
                gpio_set_up_down_res(pKey[index].key_info.key_out_pin, GPIO_PIN_PULLDOWN_100K);
            } else {
                gpio_set_up_down_res(pKey[index].key_info.key_out_pin, GPIO_PIN_PULLUP_10K);
            }

            key_val = gpio_read(pKey[index].key_info.key_out_pin) ? 1 : 0;
            if (key_val != pKey[index].key_info.key_down_level) {
                ret = 1;
            } else {
                ret = 0;
            }
        }
    }
    return ret;
}

/**
 * @brief  register the key happen event before key event.
 *
 * @param[in] p_event : the happen event
 *
 * @returns 0-success, else fail.
 */
tlk_key_sts_e tlk_key_evt_mode_register(uint8_t key_id, tlk_key_event_class_t *p_event)
{
    if (key_id > TLK_KEY_NUM_MAX || key_id == 0) {
        return TLK_KEY_PARAMETER_ERROR;
    }
    if (p_event == NULL) {
        return TLK_KEY_PARAMETER_ERROR;
    }

    tlk_key_event_class_t *p_temp = &tlkKeyEvent[key_id - 1].mode;
    memcpy(p_temp, p_event, sizeof(tlk_key_event_class_t));

    return TLK_KEY_SUCCESS;
}

/**
 * @brief  remove the key happen event before key event.
 *
 * @param[in] none
 *
 * @returns 0-success, else fail
 */
tlk_key_sts_e tlk_key_evt_happen_remove(uint8_t key_id)
{
    if (key_id > TLK_KEY_NUM_MAX || key_id == 0) {
        return TLK_KEY_PARAMETER_ERROR;
    }
    tlk_key_event_class_t *p_temp = &tlkKeyEvent[key_id - 1].mode;
    memcpy(p_temp, 0, sizeof(tlk_key_event_class_t));

    return TLK_KEY_SUCCESS;
}

tlk_key_sts_e tlk_key_register_event_table(const tlk_key_func *table, u16 count)
{
    if (table == NULL || count == 0) {
        return TLK_KEY_PARAMETER_ERROR;
    }
    tlkKeyFuncTable.count  = count;
    tlkKeyFuncTable.p_func = (tlk_key_func *)(size_t)table;
    return TLK_KEY_SUCCESS;
}
