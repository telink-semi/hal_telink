/********************************************************************************************************
 * @file    tlk_timer_stack.h
 *
 * @brief   This is the header file for BLE SDK
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
#pragma once


#define MAINLOOP_ENTRY 0
#define CALLBACK_ENTRY 1

enum
{
    APP_SOFT_TIMER_RET_STATUS_SUCCESS = 0,
    APP_SOFT_TIMER_RET_STATUS_PARAM_CHECK_FAILED,
};

typedef int (*softTimeoutCallback_t)(void *arg);

struct soft_timer
{
    struct soft_timer    *next;
    softTimeoutCallback_t cb;
    int                   timer; //unit ms.
    void                 *arg;
} __attribute__((packed));

#define SOFT_TIMER_CONFIG(pTimer, func, timerMs, argv) \
    (pTimer)->cb    = func;                            \
    (pTimer)->timer = timerMs;                         \
    (pTimer)->arg   = argv

void soft_timer_initial(void);
int  soft_timer_add(struct soft_timer *pTimer);
int  soft_timer_delete(struct soft_timer *pTimer);
void soft_timer_process(int type);
