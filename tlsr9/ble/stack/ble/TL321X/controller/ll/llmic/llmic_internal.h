/******************************************************************************
 * Copyright (c) 2024 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/
#ifndef STACK_BLE_CONTROLLER_LL_LLMIC_LLMIC_INTERNAL_H_
#define STACK_BLE_CONTROLLER_LL_LLMIC_LLMIC_INTERNAL_H_


#define ADVINTVL_RANDOM_MAX_5MS         8   //5mS
#define ADVINTVL_RANDOM_MAX_10MS        16  //10mS

#define SSLOT_NUM_5MS                   256

extern blc_ll_llmic_callback_t bltLlmic_taskFiash_cb;

_attribute_aligned_(4)
typedef struct {
    u8     llmic_task_en;
    u8     change_sch;
    u8     acl_per_sync;
    u8     occupy_cur_task;

    //u8     u8_rsvd[1];

    s32    sSlot_ble_mark;

}ll_mic_t;
extern ll_mic_t bltLlmic;


int blt_llmic_extadv_send(void);

int blt_llmic_extadv_start(int slotTask_idx);
int blt_llmic_build_extadv_task(void);


int blt_llmic_build_acl_slave_schedule(void);

int blt_llmic_quick_brx (int conn_idx);


#endif /* STACK_BLE_CONTROLLER_LL_LLMIC_LLMIC_INTERNAL_H_ */
