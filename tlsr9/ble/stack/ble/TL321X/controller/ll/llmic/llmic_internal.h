/********************************************************************************************************
 * @file    llmic_internal.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#ifndef STACK_BLE_CONTROLLER_LL_LLMIC_LLMIC_INTERNAL_H_
#define STACK_BLE_CONTROLLER_LL_LLMIC_LLMIC_INTERNAL_H_


#define ADVINTVL_RANDOM_MAX_5MS         8   //5mS
#define ADVINTVL_RANDOM_MAX_10MS        16  //10mS

#define SSLOT_NUM_5MS                   256  //LLMIC requirement
#define SSLOT_NUM_6MS                   308  //DAJIang requirement

#define LL_MIC_TASK_MINI_GAP_SSLOT_NUM   SSLOT_NUM_6MS  //A minimum interval value between tasks is required

extern blc_ll_llmic_callback_t bltLlmic_taskFiash_cb;



_attribute_aligned_(4)
typedef struct {
    u8     llmic_task_en:1; //0 or 1
    u8     change_sch:1;//0 or 1
    u8     acl_per_sync:1;//0 or 1
    u8     occupy_cur_task:1;//0 or 1
    u8     mode:2;//0 LLMIC_MANUAL or 1 LLMIC_ATUO
    u16    u8_rsvd:10;
    u16    taskMiniGapSslotNum; //  Task minimum interval setting value


    s32    sSlot_ble_mark;

}ll_mic_t;
extern ll_mic_t bltLlmic;


int blt_llmic_extadv_send(void);

int blt_llmic_extadv_start(int slotTask_idx);
int blt_llmic_build_extadv_task(void);


int blt_llmic_build_acl_slave_schedule(void);

int blt_llmic_quick_brx (int conn_idx);

int blt_llmic_getSslotGapValue(void);
#endif /* STACK_BLE_CONTROLLER_LL_LLMIC_LLMIC_INTERNAL_H_ */
