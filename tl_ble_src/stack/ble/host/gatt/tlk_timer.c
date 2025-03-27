/********************************************************************************************************
 * @file    tlk_timer.c
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
#include "tlk_timer_stack.h"

#include "tlk_list_stack.h"
#include "compatibility_pack/cmpt.h"

static int softTimer_getTimerValue(void* node)
{
    return ((struct soft_timer*)node)->timer;
}

_attribute_ble_data_retention_
static SPLIST_DEF(softTimer, softTimer_getTimerValue);

_attribute_ble_data_retention_
static unsigned int lastTimer;

void soft_timer_initial(void)
{
    lastTimer = clock_time();
}

int soft_timer_add(struct soft_timer* pTimer)
{
    if(pTimer == NULL)  return APP_SOFT_TIMER_RET_STATUS_PARAM_CHECK_FAILED;

    SPLIST_INSERT_NODE(&softTimer, pTimer);

    return APP_SOFT_TIMER_RET_STATUS_SUCCESS;
}

int soft_timer_delete(struct soft_timer* pTimer)
{
    if(pTimer == NULL)  return APP_SOFT_TIMER_RET_STATUS_PARAM_CHECK_FAILED;

    SPLIST_DELETE_NODE(&softTimer, pTimer);

    return APP_SOFT_TIMER_RET_STATUS_SUCCESS;
}

void soft_timer_process(int type)
{
    if(type == CALLBACK_ENTRY){ //callback trigger

    }

    struct soft_timer* head = (struct soft_timer*)softTimer.list.slh_first;

    if(NULL == head){
        lastTimer = clock_time();
//      bls_pm_setAppWakeupLowPower(0, 0);  //disable
        return;
    }
    int timeDiff = ((unsigned int)(clock_time()-lastTimer))/SYSTEM_TIMER_TICK_1MS;  //

    if(timeDiff < 5)        return ;

    lastTimer += timeDiff*SYSTEM_TIMER_TICK_1MS;

    SPLIST_DEF(softTimeout, softTimer_getTimerValue);

    struct soft_timer* prev = head;
    struct soft_timer* cur = prev->next;

    for(; cur; cur=cur->next) {

        if(cur->timer <= timeDiff) {
            prev->next = cur->next;
            SPLIST_INSERT_NODE(&softTimeout, cur);
        }
        else{
            prev = cur;
            cur->timer -= timeDiff;
        }
    }

    if(head->timer <= timeDiff)
    {
        SLIST_REMOVE_HEAD(&softTimer.list, next);
        SPLIST_INSERT_NODE(&softTimeout, head);
    }
    else
    {
        head->timer -= timeDiff;
    }

    prev = (struct soft_timer*)softTimeout.list.slh_first;

    do{
        if(prev)
        {
            int timer = prev->cb(prev->arg);
            if(timer > 0)
            {
                prev->timer = timer;
                soft_timer_add(prev);
            }
            prev = prev->next;
        }
    }while(prev);
}
