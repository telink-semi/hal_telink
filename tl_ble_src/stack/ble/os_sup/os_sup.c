/********************************************************************************************************
 * @file    os_sup.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "os_sup.h"
#include "os_sup_stack.h"
#include "stack/ble/controller/ll/ll_stack.h"
#include "stack/ble/controller/ble_controller.h"
#if OS_SUP_EN


_attribute_ble_data_retention_ os_give_sem_t blt_os_giveSem_cb = NULL;
_attribute_ble_data_retention_ os_give_sem_t blt_os_giveSemFromIrq_cb = NULL;

_attribute_ble_data_retention_ os_give_sem_t blt_os_semCountIncrement_cb = NULL;
_attribute_ble_data_retention_ os_give_sem_t blt_os_semCountIncrementIrq_cb = NULL;

_attribute_ble_data_retention_ static os_give_sem_t blt_os_give_sem = NULL;
_attribute_ble_data_retention_ static os_give_sem_t blt_os_give_sem_from_isr = NULL;

_attribute_ble_data_retention_ static os_mutex_sem_t blt_os_take_mutex_sem = NULL;
_attribute_ble_data_retention_ static os_mutex_sem_t blt_os_give_mutex_sem = NULL;

_attribute_ble_data_retention_ static volatile u32 SendSemCnt = 0;
_attribute_ble_data_retention_  bool is_os_sup_en = 0; //default disable OS support module.


void blc_setOsSupEnable(bool en)
{
    is_os_sup_en = en;
}

bool blc_isOsSupEnable(void)
{
    return is_os_sup_en;
}

_attribute_ram_code_
bool blc_isBleSchedulerBusy(void)
{
    //TODO: need to check if any other conditions are required.
    return (bltSche.task_mask == 0 && blmsParam.sche_run_flag == 0 && blmsParam.state_chng == 0) ? false : true;
}

_attribute_ram_code_ static void blt_ll_semCountIncrement(void)
{
    unsigned int irq = core_interrupt_disable();
    if(is_os_sup_en){
        SendSemCnt++;
    }
    core_restore_interrupt(irq);
}

_attribute_ram_code_ static void blt_ll_semCountIncrement_irq(void)
{
    if(is_os_sup_en){
        SendSemCnt++;
    }
}

_attribute_ram_code_ static void blt_ll_semGive(void)
{
    if(is_os_sup_en && blt_os_give_sem){
        blt_os_give_sem();
    }
}

_attribute_ram_code_ static void blt_ll_semGive_irq(void)
{
    if(is_os_sup_en && blt_os_give_sem_from_isr && SendSemCnt){
        blt_os_give_sem_from_isr();
    }
    SendSemCnt = 0;
}

void blc_ll_registerGiveSemCb(os_give_sem_t give_sem_from_isr, os_give_sem_t give_sem)
{
    ///
    blt_os_give_sem                = give_sem;
    blt_os_give_sem_from_isr       = give_sem_from_isr;
    ///
    blt_os_giveSem_cb              = blt_ll_semGive;
    blt_os_giveSemFromIrq_cb       = blt_ll_semGive_irq;
    ///
    blt_os_semCountIncrement_cb    = blt_ll_semCountIncrement;
    blt_os_semCountIncrementIrq_cb = blt_ll_semCountIncrement_irq;

}

_attribute_ram_code_ bool blt_llms_pushTxfifo_os(u16 connHandle, u8 *p)
{
    bool retval;
    if(is_os_sup_en && blt_os_take_mutex_sem){
        blt_os_take_mutex_sem();
    }

    retval = blt_llms_pushTxfifo(connHandle,p);
    //
    if(is_os_sup_en && blt_os_give_mutex_sem){
        blt_os_give_mutex_sem();
    }
    return retval;
}

void blc_ll_registerMutexSemCb(os_mutex_sem_t take_mutex_sem, os_mutex_sem_t give_mutex_sem)
{
    blt_os_take_mutex_sem = take_mutex_sem;
    blt_os_give_mutex_sem = give_mutex_sem;
    ll_push_tx_fifo_handler = blt_llms_pushTxfifo_os;
}





#if  0 //todo ronglu: In the following way, customers think that BLE's mainloop is executed too often. is enabled

#define MAX_MAINLOOP_NEED_RUN_TASK                32
_attribute_ble_data_retention_ volatile uint8_t blt_mainloopNeedRunflag[MAX_MAINLOOP_NEED_RUN_TASK]; //slotTask_flg && slotTask_idx
_attribute_ble_data_retention_ volatile uint8_t blt_deepRetWakeUpFlag; //

static uint8_t uslotTask_flg_static;
static uint8_t slotTask_idx_static;
/**
 * @brief This function uses mainloop to say that the state is in the pending state
 */
void blt_os_setBleLoopNeedAgainRunFlag(uint8_t uslotTask_flg,uint8_t slotTask_idx);

#define CHECK_CONTROLLER_CONNECT_HANDLE_ROLE(id)                ((id&0xF0) == BLM_CONN_HANDLE ? ACL_ROLE_CENTRAL:BLS_CONN_HANDLE)
#define blt_ll_LegAclSetNeedAgainRunFlag(Role,conn_idx)          {blt_os_setBleLoopNeedAgainRunFlag((Role+1),conn_idx);}

_attribute_ram_code_ void blt_os_setBleLoopNeedAgainRunFlag(uint8_t uslotTask_flg,uint8_t slotTask_idx)
{
    if(is_os_sup_en){
        if(uslotTask_flg >= MAX_MAINLOOP_NEED_RUN_TASK)
        {
            return;
        }
        unsigned int irq = core_interrupt_disable();
        blt_mainloopNeedRunflag[uslotTask_flg] |= (1<<slotTask_idx);
        core_restore_interrupt(irq);
    }
}

_attribute_ram_code_
bool blt_os_getBleLoopNeedAgainRunFlagStart_isr(uint8_t uslotTask_flg,uint8_t slotTask_idx)
{
    _Bool ret_val;
    ret_val = false;
    todo Each time the RF receives it, it will call to send a notification, and then follow up the actual situation processing
    if(is_os_sup_en){
        if(uslotTask_flg >= MAX_MAINLOOP_NEED_RUN_TASK)
        {
            return ret_val;
        }
        if(blt_mainloopNeedRunflag[uslotTask_flg] & (1<<slotTask_idx))
        {
            ret_val = true;
            //blt_mainloopNeedRunflag[uslotTask_flg] &= (~(1<<slotTask_idx));//clear
        }
        uslotTask_flg_static = uslotTask_flg;
        slotTask_idx_static  = slotTask_idx;
    }
    return ret_val;
}
_attribute_ram_code_
bool blt_os_getBleLoopNeedAgainRunFlagPost_isr(void)
{
    bool ret_val;
    ret_val = false;
    todo
    if(is_os_sup_en){
        if(uslotTask_flg_static >= MAX_MAINLOOP_NEED_RUN_TASK)
        {
            return ret_val;
        }
        if(blt_mainloopNeedRunflag[uslotTask_flg_static] & (1<<slotTask_idx_static))
        {
            ret_val = true;
            blt_mainloopNeedRunflag[uslotTask_flg_static] &= (~(1<<slotTask_idx_static));//clear
            uslotTask_flg_static = MAX_MAINLOOP_NEED_RUN_TASK;
        }
    }
    return ret_val;
}
#endif
#endif
