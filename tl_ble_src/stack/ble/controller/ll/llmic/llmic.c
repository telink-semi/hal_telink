/********************************************************************************************************
 * @file    llmic.c
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

#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/controller/ble_controller.h"


#include "llmic.h"
#include "llmic_internal.h"

#if (BLE_LLMIC_CONCURRENT_EN)



_attribute_ble_data_retention_  _attribute_aligned_(4) ll_mic_t  bltLlmic;


_attribute_ble_data_retention_  _attribute_aligned_(4) signal_fifo_t  ble_llmic_signal;
_attribute_ble_data_retention_  _attribute_aligned_(4) blc_ll_llmic_callback_t bltLlmic_taskFiash_cb = 0;
void blt_ll_init_llmic_module(void)
{
    #if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
        STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_mic_t)), llmic);
    #endif
     memset(&bltLlmic, 0, sizeof(ll_mic_t));//clean 0
     memset(&ble_llmic_signal, 0, sizeof(signal_fifo_t));//clean 0

     bltLlmic.taskMiniGapSslotNum = SSLOT_NUM_6MS; //default 6ms
     bltLlmic.mode = LLMIC_MANUAL;
}

void blc_ll_registerTelinkControllerFinishCallback ( blc_ll_llmic_callback_t  p)
{
    bltLlmic_taskFiash_cb = p;
}


void blc_ll_set_llmic_enable(int enable)
{
    u32 r = irq_disable();
    u8 enable_value = (enable)? 0x01:0x00;
    if(enable_value != bltLlmic.llmic_task_en){
        if(bltSche.task_mask & (TSKMSK_ACL_SLAVE_ALL | TSKMSK_EXT_ADV_ALL)){
            blmsParam.state_chng |= STATE_CHANGE_LEG_ADV;
            bltLlmic.change_sch = 0x01;
        }
    }

    bltLlmic.llmic_task_en = enable_value;
    irq_restore(r);
}


llmic_sts_e blc_ll_set_llmic_Parameters(u16 minTaskIntervalMs,llmic_controller_mode_e workType)
{
    if(minTaskIntervalMs > 11) //max 11ms
    {
       return  LLMIC_ERR_LIMIT_EXCEEDED;
    }
    u32 r = irq_disable();
    bltLlmic.taskMiniGapSslotNum = (minTaskIntervalMs * 1000) / 19;
    bltLlmic.mode = workType;
    irq_restore(r);
    return LLMIC_SUCCESS;
}


_attribute_ram_code_
signal_fifo_t * blc_ll_get_llmic_param(void)
{
    return &ble_llmic_signal;
}

_attribute_ram_code_
void blt_ll_set_llmic_status(llmic_ble_sigl_e ble_signal)
{
     ble_llmic_signal.ble_signal =  ble_signal;
     bltLlmic.occupy_cur_task = 0;
     ble_llmic_signal.llmic_signal = LLMIC_SIGL_NONE;
     if(BLE_SIGL_IDLE == ble_signal)
     {
        ble_llmic_signal.ble_new_notify = 0;
        unsigned int sys_tick = clock_time() + 5 * SYSTEM_TIMER_TICK_1MS;
        ble_llmic_signal.stick_task_begin = sys_tick;
        ble_llmic_signal.stick_task_end  = sys_tick + 500 * SYSTEM_TIMER_TICK_1US;
     }
     else
     {
         unsigned int capture_tick = systimer_get_irq_capture();
         ble_llmic_signal.ble_new_notify = 1;
         ble_llmic_signal.stick_task_begin = capture_tick;
         ble_llmic_signal.stick_task_end  = capture_tick + 500 * SYSTEM_TIMER_TICK_1US;
     }
     if((bltLlmic.llmic_task_en) && (bltLlmic_taskFiash_cb))
     {
         bltLlmic_taskFiash_cb();
     }
}

_attribute_ram_code_
void blt_ll_check_llmic_status(void)
{
    bltLlmic.occupy_cur_task = 0;

    if((ble_llmic_signal.llmic_signal == LLMIC_SIGL_REJECT || ble_llmic_signal.llmic_signal == LLMIC_SIGL_NONE) && bltLlmic.llmic_task_en){
        bltLlmic.occupy_cur_task = 1;
        DBG_SIHUI_CHN3_TOGGLE;
    }

   if(bltLlmic.mode == LLMIC_ATUO)
   {
       //in automatic mode, the application layer can update the value of llmic_signal
       bltLlmic.occupy_cur_task = 0;  //
   }

    ble_llmic_signal.ble_new_notify = 0;
}


_attribute_ram_code_
void blt_llmic_updateNextTask(void)
{
    if(bltSche.task_mask){
        if(bltSche.pTask_next){

            //bltSche.sSlot_tick_irq = bltSche.sSlot_tick_start + bltSche.pTask_next->begin*SSLOT_TICK_NUM;
            ble_llmic_signal.ble_new_notify = 1;
            DBG_SIHUI_CHN8_TOGGLE;
            ble_llmic_signal.ble_signal = bltLlmic.acl_per_sync ? BLE_SIGL_TSYNC : BLE_SIGL_NORMAL;
            ble_llmic_signal.llmic_signal = LLMIC_SIGL_NONE;
            ble_llmic_signal.stick_task_begin = bltSche.sSlot_tick_irq;
            ble_llmic_signal.stick_task_end = bltSche.sSlot_tick_start + bltSche.pTask_next->end*SSLOT_TICK_NUM;

            if((bltLlmic.llmic_task_en) && (bltLlmic_taskFiash_cb))
            {
                bltLlmic_taskFiash_cb();
            }
        }
        else{
            ble_llmic_signal.ble_new_notify = 0;
        }
    }
    else{
        ble_llmic_signal.ble_new_notify = 0;
    }
    //flow:blt_llmic_build_acl_slave_schedule - >  blt_llmic_build_extadv_task -> blt_llmic_updateNextTask
    bltLlmic.acl_per_sync = 0; //must be cleaned up at the end of use

}



_attribute_ram_code_
int blt_llmic_build_extadv_task(void)
{

#if (BLE_LLMIC_CONCURRENT_EN)
    /* if any ACL sync, do not allocate primary channel ADV */
    if(bltLlmic.llmic_task_en && bltLlmic.acl_per_sync){
        return 0;
    }
#endif


    st_ext_adv_t        *cur_pextadv;  //attention: do not use global "cur_pextadv", in case of mainloop & IRQ conflict
    for(int ii=0; ii<bltExtA.maxNum_advSets; ii++)
    {
        if( bltSche.task_mask & (TSKMSK_EXT_ADV_0<<ii) )
        {

            cur_pextadv = (st_ext_adv_t *)(global_pextadv + ii);
            cur_pextadv->extadv_change_flag = 0; //important
            s32 sSlot_mark_extadv;

            if(cur_pextadv->syncinfo_changed == 2){
                cur_pextadv->syncinfo_changed = 0;
            }

            u32 bSlot_distance = (u32)(bltSche.bSlot_idx_next - cur_pextadv->bSlot_mark_extadv);
            if( (u32)(bSlot_distance - 2) > cur_pextadv->advInt_maxAddRandom ){  //two area: >x  || <0
                sSlot_mark_extadv = bltSche.sSlot_idx_next - cur_pextadv->advInt_maxAddRandom*32;
            }
            else{
                sSlot_mark_extadv = bltSche.sSlot_idx_next - bSlot_distance*32 + bltSche.sSlot_diff_next + cur_pextadv->sSlot_diff_extadv;
            }


            sch_task_t  *pTsk_cur, *pExtLkTsk_left, *pExtLkTsk_right;


            pExtLkTsk_left = bltSche.pTask_head;
            pExtLkTsk_right = bltSche.pTask_head->next;

            int allocate_sSlot_adv;
            int cur_sSlot_adv  = sSlot_mark_extadv;  //must use s32



            s32 adv_min_left, adv_min_right, adv_max_left, adv_max_right;
            s32 sSlot_idx_left, sSlot_idx_right;

            for(int jj=0; jj<EXT_ADV_FIFONUM; jj++)
            {


                //first, we use minimum timing to find available timing block
                adv_min_left = (s32)(cur_sSlot_adv + cur_pextadv->advInt_min*32);  //s32
#if (BLE_LLMIC_CONCURRENT_EN)
                int extend_sslot = (bltLlmic.llmic_task_en) ? bltLlmic.taskMiniGapSslotNum : 0;
                adv_min_right = adv_min_left + cur_pextadv->sSlotDuration_extadv +extend_sslot;
#else
                adv_min_right = adv_min_left + cur_pextadv->sSlotDuration_extadv;
#endif

                /* current ADV task exceed time line, this function can finish */
                if( adv_min_right > bltSche.sSlot_endIdx_dft ){
                    //return 0;
                    goto extadv_loop_end;
                }

                adv_max_left  = adv_min_left + cur_pextadv->advInt_diff*32 + bltAdv.delay_sSlot_value;
#if (BLE_LLMIC_CONCURRENT_EN)
                adv_max_right = adv_max_left + cur_pextadv->sSlotDuration_extadv +extend_sslot;
#else
                adv_max_right = adv_max_left + cur_pextadv->sSlotDuration_extadv;
#endif
                while(1)
                {
                        int ADV_task_allocate = 0;

                        if(pExtLkTsk_left == bltSche.pTask_head){
                            #if (BLE_LLMIC_CONCURRENT_EN)
                                sSlot_idx_left = bltSche.sSlot_idx_next + extend_sslot;//5ms
                            #else
                                sSlot_idx_left = bltSche.sSlot_idx_next ;
                            #endif
                        }
                        else{
                            sSlot_idx_left = pExtLkTsk_left->end + 1;
                        }

                        if(pExtLkTsk_right == NULL){
                            sSlot_idx_right = bltSche.sSlot_endIdx_dft;
                        }
                        else{
                            sSlot_idx_right = pExtLkTsk_right->begin;
                        }


                        /* very quick to judge if current idle timing block is behind ADV task */
                        if(sSlot_idx_right < adv_min_right){
                            ADV_task_allocate = 0;   //not allocate, traverse to next

                        }
                        /* Case 1 */
                        /* compensate match, ADV timing exceed, must insert here, do not care delay */
                        else if(sSlot_idx_left >= adv_max_left){
                            ADV_task_allocate = 1;
                            allocate_sSlot_adv = sSlot_idx_left; //left aligned

                        }
                        else{  //window match, but we need find a proper subset window for ADV

                            ADV_task_allocate = 1;


                            /* Case 4 & 5: left aligned */
                            if(sSlot_idx_left > adv_min_left){
                                //sSlot_idx_left = sSlot_idx_left;
                                sSlot_idx_right = sSlot_idx_right < adv_max_right ? sSlot_idx_right : adv_max_right;

                                allocate_sSlot_adv = sSlot_idx_left;
                            }
                            else{  // sSlot_idx_left <= adv_min_left
                                sSlot_idx_left = adv_min_left;
                                /* Case 3: right aligned */
                                if(sSlot_idx_right < adv_max_right){
                                    //sSlot_idx_right = sSlot_idx_right;
                                    allocate_sSlot_adv = sSlot_idx_right - (cur_pextadv->sSlotDuration_extadv+extend_sslot);
                                }
                                /* Case 2: timing enough, use random */
                                else{  // sSlot_idx_right >= adv_max_right
                                    sSlot_idx_right = adv_max_right;
                                    u16 random = (clock_time()<<2) & bltAdv.delay_sSlot_mask;

                                    allocate_sSlot_adv = adv_max_left - random;
                                }
                            }
                        }


                        if( (ADV_task_allocate == 1) && (s32)(sSlot_idx_right - sSlot_idx_left) >= (cur_pextadv->sSlotDuration_extadv+extend_sslot) ){
                            ADV_task_allocate = 2;
                        }



                        if(ADV_task_allocate == 2){
                            pTsk_cur = &cur_pextadv->extadv_schTsk_fifo[jj];

                            cur_sSlot_adv = allocate_sSlot_adv;


                            pTsk_cur->begin = cur_sSlot_adv;
                            pTsk_cur->end = cur_sSlot_adv + (cur_pextadv->sSlotDuration_extadv-1) + extend_sslot;

                            //insert ADV task to existed LinkList
                            pExtLkTsk_left->next = pTsk_cur;
                            pTsk_cur->next = pExtLkTsk_right;
                            bltPri.csctvAbandonCnt[pTsk_cur->scheTask_oft] = 0;
                            pExtLkTsk_left = pTsk_cur;  //move forward pLeft

                            break;  //exit while 1
                        }
                        else{  //traverse to next
                            if(pExtLkTsk_right == NULL){
                                //return 0;  //meet the end , can not traverse, finish
                                goto extadv_loop_end;
                            }
                            else{
                                pExtLkTsk_left = pExtLkTsk_left->next;
                                pExtLkTsk_right = pExtLkTsk_right->next;
                            }
                        }

                } //while(1)

            }// for(int i=0; i<EXT_ADV_FIFONUM; i++)


        }


        extadv_loop_end: TNOP;
    }

    return 0;
}



_attribute_ram_code_
int blt_llmic_extadv_start(int slotTask_idx)
{
    DBG_CHN1_HIGH;

    bltExtA.extadv_sel = slotTask_idx;

    blt_pextadv = (st_ext_adv_t *) (global_pextadv + bltExtA.extadv_sel);

    blms_state = BLMS_STATE_EXTADV_S;

    bltAdv.adv_irq_tick = bltSche.sSlot_tick_irq;
    //bltAdv.advChn_idx = blt_pextadv->adv_chnIdx_1st; //blt_pextadv->llmic_advIdx = blt_pextadv->adv_chnIdx_1st;
    //bltAdv.advChn_cnt = 0;


    blt_pextadv->bSlot_mark_extadv = bltSche.bSlot_idx_irq_real;
    blt_pextadv->sSlot_diff_extadv = bltSche.sSlot_diff_irq;

    if(!blt_pextadv->run_ext_adv_evt){ //first ext adv, record the begin tick;
        blt_pextadv->extAdv_begin_tick = clock_time();
    }
    if(!bltLlmic.occupy_cur_task) // llmic occupy ble
    {
#if SL01_ble_ext_adv
        log_task(1, SL01_ble_ext_adv, 1);
#endif
        blt_extadv_send();
#if SL01_ble_ext_adv
        log_task(1, SL01_ble_ext_adv, 0);
#endif
    }
    else
    {
        blt_extadv_post();
    }
    return 0;
}

#if 0 //This function is not currently in use 
_attribute_ram_code_
int blt_llmic_extadv_send(void)
{
    /* 1. IRQ timing is running, but main_loop ADV disable command take effect
     * 2. when prd_adv take effect, data and timing may change, old allocated task not execute */
    if(!blt_pextadv->extadv_en || blt_pextadv->extadv_change_flag){
        blt_extadv_post();
        return 0;
    }

    #if !LL_EXT_ADV_DURATION_OPTIMIZE_EN
        if(blt_extadv_dur_maxEvt_proc()){//return 1: duration or maxEvt has expired.
            return 0; //
        }
    #endif


    rf_ble_switch_phy(blt_pextadv->pri_phy, blt_pextadv->coding_ind);


    if(bltPHYs.cur_llPhy == BLE_PHY_CODED){
        rf_trigger_codedPhy_accesscode();
    }

    rf_ble_set_tx_settle(bltPHYs.tx_stl_adv); //attention: must set after PHY switch !!!

    /* Different process for different MCU: ******************************************/
    ble_rf_set_tx_dma(0, 17);
     //  This register exists in Kite/Vulture/826x, riscv architecture chips such as
     //  Eagle do not have this register, but DMA fifo can be turned off by forcibly
     //  setting DMA TX rptr = DMA TX wptr to use DMA default tx fifo.If the hardware
     //  TX rptr of DMA TX fifo == hardware TX wptr, then send DMA Tx default fifo,
     //  otherwise send DMA Tx fifo non-default area.
    HAL_REG_RF_DMA_FIFO_TX_RPTR = FLD_DMA_RPTR_CLR;
    /**********************************************************************************/

    rf_set_tx_rx_off();
    rf_set_ble_crc_adv ();
    rf_set_ble_access_code_adv ();


    rf_set_ble_channel (blc_extadv_channel[bltAdv.advChn_idx]);


    blt_ll_set_tx_power_by_strategy(TX_POWER_STRATEGY_CUSTOMER_OR_DEFAULT, 0);

#if (LL_FEATURE_ENABLE_LE_CODED_PHY)
    rf_trigger_codedPhy_accesscode();
#endif

    bltAdv.adv_scanReq_connReq = 0;


    /* SiHui found problem on B91. same situation for other RISV MCU with PLIC module.
     * but process method maybe different for new MCU, so move this function to HAL.
     * FSM IRQ status will send to PLIC module, clear reg_rf_irq_status can not drop RF IRQ,
     * so we disable RF mask to prevent RF IRQ status sending to PLIC */
    HAL_BLE_STACK_RF_IRQ_MASK_CLEAR;



    if(blt_pextadv->legacy_adv){
        blt_send_legacy_adv();
    }
    else{
        blt_send_extend_adv();
    }

    bltAdv.advChn_cnt ++;

    if((bltAdv.advChn_cnt >= blt_pextadv->adv_chn_num)){
//      bltAdv.advChn_idx = blt_pextadv->adv_chnIdx_1st; //before ext_adv_start, it'll be initialized.
    }
    else{
        bltAdv.advChn_idx++;
        u8 mask = 1 << (blc_extadv_channel[bltAdv.advChn_idx] - 37);
        if ((mask & blt_pextadv->adv_chn_mask) == 0) {
            bltAdv.advChn_idx++;
        }
    }

    if( bltAdv.adv_scanReq_connReq || (bltAdv.advChn_cnt >= blt_pextadv->adv_chn_num) ){
        blt_extadv_post();
    }
    else{
        if(blt_pextadv->legacy_adv){
            systimer_set_irq_capture(clock_time () + 20 * SYSTEM_TIMER_TICK_1US);
        }
        else{ //extended ADV
            if(blt_pextadv->with_aux_adv_ind){
                systimer_set_irq_capture(bltSche.system_irq_tick + blt_pextadv->pri_single_chn_us * SYSTEM_TIMER_TICK_1US);
            }
            else{
                systimer_set_irq_capture(clock_time () + 20 * SYSTEM_TIMER_TICK_1US);
            }
        }

        systick_irq_trigger = SYS_IRQ_TRIG_EXTADV_SEND;

    }


    /* clear status as late as possible, cause if clear too early, some status did not come, e.g. STX cmd done*/
    CLEAR_ALL_RFIRQ_STATUS;



    /* SiHui found problem on B91. same situation for other RISV MCU with PLIC module.
     * but process method maybe different for new MCU, so move this function to HAL.
     * FSM IRQ status will send to PLIC module, clear reg_rf_irq_status can not drop RF IRQ,
     * so we disable RF mask to prevent RF IRQ status sending to PLIC */
    HAL_BLE_STACK_RF_IRQ_MASK_SET;


    return 0;
}

#endif



_attribute_ram_code_
int blt_llmic_build_acl_slave_schedule(void)
{
    u32 i,j;

    st_ll_conn_t        *cur_pAclConn;
    st_lls_conn_t       *cur_pAclSlave;
    int int_jump_acl;
    s32 sSlot_start_conn;

    int slave_task_number = 0;

    s32 sSlot_TskPost_real = bltSche.sSlot_idx_next;
#if (BLE_LLMIC_CONCURRENT_EN)
    /* check if any ACL Peripheral Sync. If sync, can not drop */
    bltLlmic.acl_per_sync = 0;
    for(i=ACL_CONN_IDX_PER0; i<LL_MAX_ACL_CONN_NUM; i++)
    {
        if(bltLlmic.llmic_task_en && (bltSche.task_mask & (TSKMSK_ACL_CONN_0<<i) ))
        {
            cur_pAclConn  = (st_ll_conn_t *)&blms[i];
            if(cur_pAclConn->sync_timing){
                bltLlmic.acl_per_sync = 1;
                sSlot_TskPost_real = bltSche.sSlot_idx_next;
            }
            else
            {
                sSlot_TskPost_real = bltSche.sSlot_idx_next + bltLlmic.taskMiniGapSslotNum;
            }
        }
    }
#endif


    for(i=ACL_CONN_IDX_PER0; i<LL_MAX_ACL_CONN_NUM; i++)
    {
        if( bltSche.task_mask & (TSKMSK_ACL_CONN_0<<i) )
        {
            cur_pAclConn  = (st_ll_conn_t *)&blms[i];
            cur_pAclSlave = (st_lls_conn_t *)&blmsSlave[i-LL_MAX_ACL_CEN_NUM];
            cur_pAclSlave->aclTsk_wptr = cur_pAclSlave->aclTsk_rptr = 0;

            #if (BLMS_PM_ENABLE)
                int sSlot_mark_update = 0;
            #endif

            if(bltSche.build_index == 0){
                if(bltSche.sSlot_idx_reset == 1){
                    cur_pAclSlave->sSlot_mark_conn -= bltSche.sSlot_idx_past;
                    cur_pAclSlave->sSlot_mark_brx -= bltSche.sSlot_idx_past;
                }

                if(!cur_pAclConn->sync_timing){
                    if(cur_pAclSlave->sSlot_offset){
                        cur_pAclSlave->sSlot_mark_conn += cur_pAclSlave->sSlot_offset;
                        if(cur_pAclConn->conn_update_union.update_mark & CONN_UPDATE_PARAM_MASK){
                            cur_pAclSlave->conn_update_pre_sSlotIndex += cur_pAclSlave->sSlot_offset;  //TODO: test
                        }
                        cur_pAclSlave->sSlot_offset = 0;
                    }
                    #if (BLMS_PM_ENABLE)
                    else{
                        if(cur_pAclConn->pm_error_us || cur_pAclSlave->conn_tolerance_us > blmsParam.min_tolerance_us){
                            cur_pAclSlave->conn_tolerance_us = cur_pAclConn->pm_error_us + blmsParam.min_tolerance_us;
                            sSlot_mark_update = 1;
                        }
                    }
                    #endif
                }
            }

            else
            {
                #if (BLMS_PM_ENABLE)
                    if(!cur_pAclConn->sync_timing){
                        cur_pAclSlave->conn_tolerance_us += blmsParam.min_tolerance_us;
                        sSlot_mark_update = 1;
                    }
                #endif
            }


            #if (BLMS_PM_ENABLE)
                if(sSlot_mark_update){
                    if(cur_pAclSlave->conn_tolerance_us > cur_pAclSlave->tolerance_max_us){
                        cur_pAclSlave->conn_tolerance_us = cur_pAclSlave->tolerance_max_us;
                    }

                    s32 sSlot_shift_new = cur_pAclSlave->conn_tolerance_us*SSLOT_US_REVERSE;
                    cur_pAclSlave->sSlot_mark_conn -= (sSlot_shift_new - cur_pAclSlave->sSlot_shift_tor);
                    cur_pAclSlave->sSlot_shift_tor = sSlot_shift_new;

                    // tor*2 /sSlot_unit = tor*2 /(625/32) = tor*64/625
                    cur_pAclConn->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + pdu_27b_tifs_27b_sslot[cur_pAclConn->connPhyCtrl.conn_cur_phy - 1][cur_pAclConn->crypt.enable] + cur_pAclSlave->conn_tolerance_us*64/625;
                }
            #endif




            if( cur_pAclSlave->sSlot_mark_conn >= sSlot_TskPost_real){//sSlot_mark_conn init in "blt_s_connect" may make this happen
                sSlot_start_conn = cur_pAclSlave->sSlot_mark_conn + cur_pAclSlave->sSlot_interval;
                int_jump_acl = 0;
            }
            else
            {
                int_jump_acl = (sSlot_TskPost_real - 1 - cur_pAclSlave->sSlot_mark_conn)/cur_pAclSlave->sSlot_interval;

                sSlot_start_conn = cur_pAclSlave->sSlot_mark_conn + (int_jump_acl + 1)*cur_pAclSlave->sSlot_interval;
            }

            if(sSlot_start_conn >= bltSche.sSlot_endIdx_dft){ //to save some time for big interval
                continue; //attention: can not use break !!!
            }

            /* SiHui: consider update a new task add, so add some more time. here update may represent a task remove, neglect this
             * give another margin here */
            u32 scheduler_use_us = bltSche.sche_process_us + SCHE_NEW_TASK_MARGIN_US;
            cur_pAclConn->sSlot_sche_use = scheduler_use_us * SSLOT_US_REVERSE;
            cur_pAclConn->sSlot_duration = cur_pAclConn->sSlot_allocNum + cur_pAclConn->sSlot_sche_use;



            int new_task_cnt = 0;

            for(j=0;j<ACL_SLAVE_FIFONUM;j++){

                sch_task_t  *pCur_schTask = (sch_task_t *)&cur_pAclSlave->aclTsk_fifo[j];

                pCur_schTask->begin = sSlot_start_conn + j*cur_pAclSlave->sSlot_interval;




#if (BLE_LLMIC_CONCURRENT_EN)
                int acl_extend_sslot = (bltLlmic.llmic_task_en && !bltLlmic.acl_per_sync) ? bltLlmic.taskMiniGapSslotNum : 0;
                pCur_schTask->end = pCur_schTask->begin + cur_pAclConn->sSlot_duration + acl_extend_sslot - 1;
#else
                pCur_schTask->end = pCur_schTask->begin + cur_pAclConn->sSlot_duration - 1;
#endif
                pCur_schTask->cover_other = 0;




                if( pCur_schTask->begin >=  bltSche.sSlot_endIdx_dft){  //new task beyond correct range, finish
                    break;
                }
#if (BLE_LLMIC_CONCURRENT_EN)
                else if(pCur_schTask->end < (bltSche.sSlot_endIdx_dft + acl_extend_sslot))
#else
                else if(pCur_schTask->end < bltSche.sSlot_endIdx_dft)
#endif
                { //new task in correct range
                    cur_pAclSlave->aclTsk_wptr = j;
                    new_task_cnt ++;
                }
                else{ //new task across "sSlot_endIdx_dft"

                    //for those task across end_idx, find the task with highest priority, to guarantee that task not missed
                    if(bltPri.pri_cal[TSKOFT_ACL_CONN + i] > bltPri.priMax_value){
                        bltPri.priMax_value = bltPri.pri_cal[TSKOFT_ACL_CONN + i];
                        bltPri.priMax_index = TSKOFT_ACL_CONN + i;
                        bltSche.sSlot_endIdx_maxPri = pCur_schTask->begin;
                        my_dump_str_u32s(SCHE_TIMING_IMPROVE_DBG_EN,"across IDX salve", i, bltSche.sSlot_endIdx_dft, bltSche.sSlot_endIdx_maxPri, bltPri.priMax_value);
                    }

                    break;
                }

//              cur_pAclSlave->sSlot_mark_conn += j*pCur_schTask->begin;
            }

            slave_task_number += new_task_cnt;
            if(new_task_cnt){
                int t = blt_ll_addTask2ExistLinklist( &cur_pAclSlave->aclTsk_fifo[0],cur_pAclSlave->aclTsk_wptr + 1);
                (void)t; //remove compiler warning
                my_dump_str_u32s(0, "addTsk", t, cur_pAclSlave->aclTsk_wptr + 1,0,0);
            }
        }
    }






    return slave_task_number;
}




_attribute_ram_code_
int blt_llmic_quick_brx (int conn_idx)
{
    blms_conn_sel = conn_idx;
    bls_conn_sel = blms_conn_sel - LL_MAX_ACL_CEN_NUM; //pay attention here


    if(bls_conn_sel == 0){
        DBG_CHN8_HIGH;
        DBG_SIHUI_CHN8_HIGH;
    }
    else if(bls_conn_sel == 1){
        DBG_CHN9_HIGH;
        DBG_SIHUI_CHN9_HIGH;
    }





    blms_pconn =  (st_ll_conn_t *)&blms[blms_conn_sel];

    blms_state = BLMS_STATE_BRX_E;  //set to end state


    /* process priority*/
    blms_pconn->conn_successive_miss ++;
    if(blms_pconn->conn_successive_miss > 10){
        blt_ll_setSchedulerTaskPriority(TSKOFT_ACL_CONN + blms_conn_sel, TASK_PRIORITY_LOW);
        blms_pconn->conn_successive_miss = 5;
    }
    else{
        blt_ll_incSchedulerTaskPriority(TSKOFT_ACL_CONN + blms_conn_sel, bltPri.step_final[TSKOFT_ACL_CONN + blms_conn_sel] );
    }


    blt_ll_calculate_sSlot_next(clock_time() + blms_pconn->sSlot_sche_use*SSLOT_TICK_NUM);



    if(bls_conn_sel == 0){
        DBG_CHN8_LOW;
        DBG_SIHUI_CHN8_LOW;
    }
    else if(bls_conn_sel == 1){
        DBG_CHN9_LOW;
        DBG_SIHUI_CHN9_LOW;
    }

    return 0;
}


_attribute_ram_code_ void blc_llmic_switch_2M(void)
{
    extern void rf_ble_switch_phy(le_phy_type_t phy, le_coding_ind_t own_coding_ind);
    rf_ble_switch_phy(BLE_PHY_2M, LE_CI_NONE);
    reg_rf_dma_tx_rptr(0) = FLD_DMA_RPTR_CLR;
}


_attribute_ram_code_ int blt_llmic_getSslotGapValue(void)
{
    DBG_CHN5_TOGGLE;
   return ((bltLlmic.llmic_task_en) ? bltLlmic.taskMiniGapSslotNum : 0);
}


#endif
