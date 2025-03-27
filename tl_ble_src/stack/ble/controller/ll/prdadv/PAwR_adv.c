/********************************************************************************************************
 * @file    PAwR_adv.c
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

//Based on LL Periodic ADV feature, use common CB, && Initiate (Central) feature
//LL_FEATURE_ENABLE_LE_EXTENDED_INITIATE && LL_ACL_CEN_EN && LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING
#if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)




_attribute_ble_data_retention_ my_fifo_t pawra_RxFifo;

_attribute_ble_data_retention_ pawra_mng_para_t pawraMng;

_attribute_ble_data_retention_ u8 pawra_rx_fifo[PAWRA_RXFIFO_SIZE * PAWRA_RXFIFO_NUM]; //PAwR RX FIFO


ble_sts_t   blc_ll_initPeriodicAdvWrModule_initPeriodicdAdvWrSetParamBuffer(u8 *pBuff, int num_periodic_adv)
{
    STATIC_ASSERT_FILE(PERD_ADV_PARAM_LENGTH == sizeof(st_prd_adv_t), prd_adv);

    #if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
        STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(st_prd_adv_t)), prd_adv);
    #endif

    #if (TSKNUM_PERD_ADV != TSKNUM_PAWRA_SUB)
        #error "PAwR task number size error"
    #endif

    if( num_periodic_adv > TSKNUM_PAWRA_SUB ){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    #if (1) /* periodic adv feature && extInit module must be used TODO: rewrite */
        if(!(LL_FEATURE_MASK_0 & LL_FEATURE_MASK_LE_PERIODIC_ADVERTISING)){
            blc_ll_initPeriodicAdvModule_initPeriodicdAdvSetParamBuffer(pBuff, num_periodic_adv);//already enabled in periodic_adv initialization
        }
        if(!blmsParam.extInitModule_en){
            blc_ll_initExtendedInitiating_module();
        }
    #endif

    LL_FEATURE_MASK_1 |= LL_FEATURE_MASK_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER;

    blmsParam.prdAdvWr_en = 1;

    ll_pawra_rsp_irq_task_cb = blt_pawra_rsp_interrupt_task;
    ll_pawra_sub_irq_task_cb = blt_pawra_subx_interrupt_task;
    ll_pawra_mlp_task_cb = blt_pawra_mainloop_task;

    //Secondary Channel Scan RX buffer initialize
    pawra_RxFifo.p = pawra_rx_fifo;
    pawra_RxFifo.num = PAWRA_RXFIFO_NUM;
    //pawra_RxFifo.size = PAWRA_RXFIFO_SIZE;
    pawra_RxFifo.rptr = pawra_RxFifo.wptr = 0;
    pawraMng.pawra_rx_dma_buff = (u32)(pawra_RxFifo.p + (pawra_RxFifo.wptr & PAWRA_RXFIFO_MASK) * PAWRA_RXFIFO_SIZE);
    pawraMng.pawra_rx_dma_size = (PAWRA_RXFIFO_SIZE>>4);


    st_prd_adv_t *cur_pPerdadv = NULL;
    for(int i=0;i<num_periodic_adv; i++){
        cur_pPerdadv = ((st_prd_adv_t *)pBuff) + i;

        cur_pPerdadv->num_subevents = 0;
        cur_pPerdadv->responseAA = 0;
        cur_pPerdadv->subevent_interval = 0;
        cur_pPerdadv->num_response_slots = 0;
        cur_pPerdadv->response_slot_delay = 0;
        cur_pPerdadv->response_slot_spacing = 0;

        cur_pPerdadv->maxLen_subeventData = 0;
        cur_pPerdadv->num_subeventData = 0;

        cur_pPerdadv->initSubevent = 0;

        for(int j=0; j<15; j++){
            cur_pPerdadv->pdaSubevtDataCtrl[j].pSubevt_data = NULL;
            cur_pPerdadv->pdaSubevtDataCtrl[i].subevent_idx = 0xFF; //invalid
        }

        /* PAwR-Advertiser response slots task concerned */
        cur_pPerdadv->rspSchTsk_fifo.scheTask_oft = TSKOFT_PAWRA_RSP + i;
        cur_pPerdadv->rspSchTsk_fifo.scheTask_idx = i;
        cur_pPerdadv->rspSchTsk_fifo.scheTask_flg = TSKFLG_PAWRA_RSP;

        /* PAwR-Advertiser subevent task (not contain subevent0) concerned */
        for(int j=0; j<PERD_ADV_FIFONUM; j++){
            cur_pPerdadv->subSchTsk_fifo[j].scheTask_oft = TSKOFT_PAWRA_SUB + i;
            cur_pPerdadv->subSchTsk_fifo[j].scheTask_idx = i;
            cur_pPerdadv->subSchTsk_fifo[j].scheTask_flg = TSKFLG_PAWRA_SUB;
        }
        //bltPriority[TSKOFT_PAWRA_SUB + i] = 0;  //debug
        //bltPriority[TSKOFT_PAWRA_RSP + i] = 0;  //debug
    }

    return BLE_SUCCESS;
}


void blc_ll_initPeriodicAdvWrDataBuffer(u8 *pSubeventData, int subeventDataLenMax, int subeventDataCnt)
{

    if(pSubeventData == NULL || subeventDataLenMax == 0 || subeventDataLenMax > 251 || subeventDataCnt > 128) return ;

    for(int i=0;i<bltPrdA.maxNum_perdAdv; i++){
        (global_pPerdadv+i)->maxLen_subeventData = subeventDataLenMax;
        (global_pPerdadv+i)->num_subeventData = subeventDataCnt;
        (global_pPerdadv+i)->subeventData_validPaEvt = 0;
        for(int j=0; j<subeventDataCnt; j++){
            (global_pPerdadv+i)->pdaSubevtDataCtrl[j].pSubevt_data = pSubeventData + (subeventDataLenMax*subeventDataCnt) * i + subeventDataLenMax * j ;
        }
    }
}

_attribute_ram_code_
int blt_pawra_rsp_interrupt_task (int flag, void*p)
{
    int index = flag & FLAG_SCHEDULE_TASK_IDX_MASK;

    if(flag & FLAG_INSERT_PAWRA_SLOT_TASK){
        blt_pawra_rsp_task_insert(index);
    }
    else if(flag & FLAG_SCHEDULE_START){
        blt_pawra_rsp_start(index);
    }
    else if(flag & FLAG_SCHEDULE_PAWRA_SLOT_START){
        blt_pawra_rsp_slot_start();
    }
    else if (flag & FLAG_IRQ_RX){
        irq_pawra_rsp_slot_rx();
    }
    else if(flag & FLAG_SCHEDULE_PAWRA_SLOT_POST){
        blt_pawra_rsp_slot_post();
    }
    else if(flag & FLAG_SCHEDULE_BUILD){
        blt_pawra_rsp_sch_build();
    }
    else if(flag & FLAG_INSERT_SCHTSK_CONFLICT){
        return blt_pawra_task_conflic((sch_task_t *)p, TSKOFT_PAWRA_RSP + index);
    }

    return 0;
}

_attribute_ram_code_
int blt_pawra_rsp_start(int index)
{
    DBG_CHN9_HIGH;
    pawraMng.prd_adv_sel = index;
    blt_pPerdadv = (st_prd_adv_t *) (global_pPerdadv + pawraMng.prd_adv_sel);
    blt_pextadv = (st_ext_adv_t *) (global_pextadv + blt_pPerdadv->mapping_extadv_idx);
    blt_pPda = (st_pda_t *) &blt_pPerdadv->pda_tx;

    /* PHY switch, do not consider S2 */
    rf_ble_switch_phy(blt_pPda->pda_phy, LE_CODED_S8);
    if(bltPHYs.cur_llPhy == BLE_PHY_CODED){
        rf_trigger_codedPhy_accesscode();
    }

    //RF Hardware register setting
    rf_set_tx_rx_off();
    rf_set_ble_channel(blt_pPerdadv->rspChnIdx);
    rf_set_ble_access_code((u8*)&blt_pPerdadv->responseAA);
    rf_set_ble_crc_value(blt_pPda->paCrcInit);

    blt_ll_set_tx_power_by_strategy(TX_POWER_STRATEGY_CUSTOMER_OR_DEFAULT, 0);
    /* the max len for ESL is 48B, but for controller we should support the max len. */
    rf_set_rx_maxlen(255);//TODO: by lihaojie 2024..6.7
    rf_set_1st_rx_timeout(300 + bltPHYs.prmb_ac_us); //make sure PHY switch before this code
    ble_rf_set_rx_dma((u8*)pawraMng.pawra_rx_dma_buff, pawraMng.pawra_rx_dma_size);

    /* Attention: must used sSlot_idx_irq_real, not bSlot_idx_irq_real(anchor-point maybe with 625us error) */
    /* for each slot,duration is constant,too large margin will make that can not receive long packet completely, so decrease 100 us margin, by lihaojie 2024..6.7*/
    pawraMng.slot_trigger_tick = bltSche.sSlot_tick_irq_real + 100 * SYSTEM_TIMER_TICK_1US; 

    /* jump N slots ( N = rsp_slot_start) */
    blt_pPerdadv->response_slot_idx = blt_pPerdadv->rsp_slot_start;

    blt_pawra_rsp_slot_start();

    blt_sche_removeTaskMask(TSKMSK_PAWRA_RSP_0 << index);

    if(FALSE == blt_remove_future_task(TSKOFT_PAWRA_RSP + index)){
        my_dump_str_u32s(0, "future task err", bltFutTask.number, index, 0, 0);
    }

    return 0;
}

_attribute_ram_code_
int blt_pawra_rsp_post(void)
{
    blms_state = BLMS_STATE_PAWRA_SLOT_GRP_E;

    blt_pPerdadv->response_slot_idx = 0;

    blt_ll_calculate_sSlot_next(clock_time() + SLOT_PROCESS_MAX_TICK);

    /* important: ensure that FSM stopped */
    STOP_RF_STATE_MACHINE;
    /* clear status as late as possible, cause if clear too early, some status did not come, e.g. STX cmd done*/
    CLEAR_ALL_RFIRQ_STATUS;

    DBG_CHN9_LOW;
    return 0;
}

_attribute_ram_code_
int blt_pawra_rsp_slot_start(void)
{
    DBG_CHN10_HIGH;
    DBG_CHN11_HIGH;
    blms_state = BLMS_STATE_PAWRA_SLOT_S;

    //system trigger point: consider that RX IRQ must processed
    systick_irq_trigger = SYS_IRQ_TRIG_PAWRA_SLOT_POST;

    DBG_CHN9_TOGGLE;
    DBG_CHN9_TOGGLE;
    rf_start_fsm(FSM_SRX, NULL, clock_time());

    bltRxPkt.rx_header_tick = 0; //clear RX CRC OK status

    pawraMng.slot_trigger_tick += blt_pPerdadv->response_slot_spacing*SYSTEM_TIMER_TICK_125US;
    systimer_set_irq_capture(pawraMng.slot_trigger_tick - 20*SYSTEM_TIMER_TICK_1US);

    DBG_CHN10_LOW;
    return 0;
}

_attribute_ram_code_
int blt_pawra_rsp_slot_post(void)
{
    DBG_CHN10_HIGH;
    blms_state = BLMS_STATE_PAWRA_SLOT_E;
    /* rx_header_tick zero is under condition CRC error OR RX timeout */
    if(!bltRxPkt.rx_header_tick)
    {
        u8* raw_pkt = ble_curr_rx_dma_buff;  //or cisConn_param.cis_rx_dma_buff
        //Mark it's a PAwR response slot Data PDU
        raw_pkt[0] = blt_pPda->paEvtCnt - 1; //current PAwR event
        raw_pkt[1] = blt_pPerdadv->advHand_mark; //mark adv handle
        raw_pkt[2] = blt_pPerdadv->response_slot_idx; //offset start from 0, already plus ONE, here need recovey
        raw_pkt[3] = blt_pPerdadv->paSubEventCnt - 1; //offset start from 0
        //Clear rf_len and ext_hdr_len
        raw_pkt[DMA_RFRX_OFFSET_RFLEN] = 0;
        raw_pkt[DMA_RFRX_OFFSET_DATA] = 0;
        //switch to next rx buffer
        pawra_RxFifo.wptr++;
        u32 new_pkt = (u32)(pawra_RxFifo.p + (pawra_RxFifo.wptr & PAWRA_RXFIFO_MASK) * PAWRA_RXFIFO_SIZE);
        pawraMng.pawra_rx_dma_buff = new_pkt;
        /* setting the next RX DMA buffer */
        ble_rf_set_rx_dma((u8*)pawraMng.pawra_rx_dma_buff, pawraMng.pawra_rx_dma_size);
    }

    if(++blt_pPerdadv->response_slot_idx < (blt_pPerdadv->rsp_slot_count+blt_pPerdadv->rsp_slot_start)){
        systimer_set_irq_capture(pawraMng.slot_trigger_tick);
        systick_irq_trigger = SYS_IRQ_TRIG_PAWRA_SLOT_START;
    }
    else{
        blt_pawra_rsp_post();
    }



    DBG_CHN10_LOW;
    DBG_CHN11_LOW;
    return 0;
}

_attribute_ram_code_
int irq_pawra_rsp_slot_rx(void)
{
    u8* raw_pkt = ble_curr_rx_dma_buff;  //or cisConn_param.cis_rx_dma_buff

    HAL_CLEAR_RF_RX_IRQ;

    u8 next_buffer = 0;
    raw_pkt[2] = 0;

    /* "rx header tick" none zero is under condition CRC correct, so here do not check CRC by
       "RF_BLE_PACKET_VALIDITY_CHECK" to save RamCode and running timing
       Or we can use "bltRxPkt.crc correct" */
    if(bltRxPkt.rx_header_tick)
    {
        u8 rf_len = raw_pkt[DMA_RFRX_OFFSET_RFLEN];
        if(rf_len){
            //Mark it's a PAwR response slot Data PDU
            raw_pkt[0] = blt_pPda->paEvtCnt - 1; //current PAwR event
            raw_pkt[1] = blt_pPerdadv->advHand_mark; //mark adv handle
            raw_pkt[2] = blt_pPerdadv->response_slot_idx; //offset start from 0
            raw_pkt[3] = blt_pPerdadv->paSubEventCnt - 1; //offset start from 0
            //switch to next rx buffer
            next_buffer = 1;
        }
    }

    if (next_buffer) //update buffer
    {
        pawra_RxFifo.wptr++;
        u32 new_pkt = (u32)(pawra_RxFifo.p + (pawra_RxFifo.wptr & PAWRA_RXFIFO_MASK) * PAWRA_RXFIFO_SIZE);
        pawraMng.pawra_rx_dma_buff = new_pkt;
        /* setting the next RX DMA buffer */
        ble_rf_set_rx_dma((u8*)pawraMng.pawra_rx_dma_buff, pawraMng.pawra_rx_dma_size);
    }

    return 0;
}



_attribute_ram_code_
int blt_pawra_rsp_task_insert(int index)
{
    /*
     *  2nd: report request data, 1st : subevtCount = 0 and subevtStart = 0
     */
    st_prd_adv_t *pPerdadv = blt_pPerdadv; //global_pPerdadv + index;
    u8 subevtStart = pPerdadv->subDataReq.subevtStart;
    u8 subevtEnd = subevtStart + pPerdadv->subDataReq.subevtCount - 1;
    u8 currSubevt = pPerdadv->paSubEventCnt - 1;

    if(subevtEnd == 0xFF || subevtEnd == currSubevt || subevtStart == subevtEnd){
        /* The Subevent_Start parameter is the first subevent being requested  */
        pPerdadv->subDataReq.subevtStart = (subevtStart == subevtEnd) ? 0 : pPerdadv->paSubEventCnt; //paSubEventCnt offset from 1, subevtStart offset from 0.
        if(pPerdadv->subDataReq.subevtStart == pPerdadv->num_subevents){
            pPerdadv->subDataReq.subevtStart = 0;
        }

        /* the Subevent_Data_Count parameter determines the subsequent subevents being requested */
        pPerdadv->subDataReq.subevtCount = min(pPerdadv->num_subevents - pPerdadv->subDataReq.subevtStart,  pPerdadv->num_subeventData);
        if(pPerdadv->subDataReq.subevtCount == 0) { 
            pPerdadv->subDataReq.subevtCount = min(pPerdadv->num_subevents,  pPerdadv->num_subeventData);
        }
        pPerdadv->subeventData_evtTrig = 1;

        pPerdadv->subeventData_validPaEvt = (subevtStart == subevtEnd || pPerdadv->paSubEventCnt == pPerdadv->num_subevents) ? blt_pPda->paEvtCnt : (blt_pPda->paEvtCnt - 1); //attention!!!

        my_dump_str_u32s(DBG_PAwR_ADV_LOGIC, "S-C-E-U", pPerdadv->subDataReq.subevtStart, pPerdadv->subDataReq.subevtCount, pPerdadv->subeventData_validPaEvt, currSubevt);

        DBG_CHN5_HIGH;
        DBG_CHN5_LOW;

    }

    /* Skip RSP TASK if user not configure Response slot information */
    if(pPerdadv->rsp_slot_count == 0) {
        DBG_CHN10_TOGGLE;
        DBG_CHN10_TOGGLE;
        return 0;
    }

    /* 3th: RSP TASK insert */
    s32 sSlot_mark_rsp = pPerdadv->sSlot_mark_subx + TICKS_DUR_2_SSLOT_DUR(pPerdadv->response_slot_delay*SYSTEM_TIMER_TICK_1250US);  //1.25mS -> 625 uS
    sSlot_mark_rsp += TICKS_DUR_2_SSLOT_DUR(pPerdadv->response_slot_spacing*pPerdadv->rsp_slot_start*SYSTEM_TIMER_TICK_125US);
    /* minus 3 sSlot: before 60us prepare for RF ramp-up */
    sSlot_mark_rsp -= 3;

    //s32 sSlot_duration_rsp = pPerdadv->sSlot_duration_rsp - (TICKS_DUR_2_SSLOT_DUR(pPerdadv->response_slot_spacing*(pPerdadv->num_response_slots - pSubDataCfg->rsp_slot_count)*SYSTEM_TIMER_TICK_125US));
    s32 sSlot_duration_rsp = TICKS_DUR_2_SSLOT_DUR(pPerdadv->response_slot_spacing*pPerdadv->rsp_slot_count*SYSTEM_TIMER_TICK_125US + SLOT_PROCESS_MAX_TICK) + 1;

    sch_task_t *pTsk_cur = &pPerdadv->rspSchTsk_fifo;
    pTsk_cur->begin = sSlot_mark_rsp;
    pTsk_cur->end = pTsk_cur->begin + (sSlot_duration_rsp - 1);

    sch_task_t *pExtLkTsk_left, *pExtLkTsk_right, *pExtLkTsk_left_prev;
    pExtLkTsk_left = bltSche.pTask_cur;
    pExtLkTsk_right = bltSche.pTask_next;
    pExtLkTsk_left_prev = bltSche.pTask_cur;

    s32 sSlot_idx_left, sSlot_idx_right;
    u8 RSP_task_allocate = 0;

    while(1){
        sSlot_idx_left = pExtLkTsk_left->end + 1;
        if(pExtLkTsk_right == NULL){
            sSlot_idx_right = bltSche.sSlot_endIdx_maxPri;
            if((s32)(sSlot_idx_right - pTsk_cur->end) > 0){
                /* RSP_ALLOCATE_ON_LINKLIST */
                RSP_task_allocate = 1;
            }else{
                /* RSP_ALLOCATE_BEYOND_LINKLIST */
                RSP_task_allocate = 2;
                /*
                 *                   |....rebuild sch....|
                 *                              |-Margin-|********insertTask*********|
                 *                   |********insertTask*********|
                 *  ---|task0|------------|task1|---------|max_end_idx
                 */
                if(pTsk_cur->begin < pExtLkTsk_left->end + SLOT_PROCESS_MAX_SSLOT_NUM){
                    //Rebuild sch task table ASAP.
                    blt_sche_addUpdate(SLOT_UPDT_SLOTTBL_RESCHED);
                }
                DBG_CHN7_HIGH;
                DBG_CHN7_LOW;
                break;
            }
        }else{
            sSlot_idx_right = pExtLkTsk_right->begin;
            if((s32)(sSlot_idx_right - pTsk_cur->end) > 0){ // ">=" is OK, use ">" here
                /* RSP_ALLOCATE_ON_LINKLIST */
                RSP_task_allocate = 1;
            }
        }

        if(RSP_task_allocate == 1 /*RSP_ALLOCATE_ON_LINKLIST*/){
            if((pTsk_cur->begin - sSlot_idx_left > 0)){
                //insert RSP task to existed LinkList
                pExtLkTsk_left->next = pTsk_cur;
                pTsk_cur->next = pExtLkTsk_right;
                DBG_CHN8_HIGH;
                DBG_CHN8_LOW;
                if(pExtLkTsk_left == bltSche.pTask_cur){ //match in first traverse
                    bltSche.pTask_next = pTsk_cur;
                }
            } else {
                s32 pri_taskCur = bltPri.pri_cal[pTsk_cur->scheTask_oft];
                s32 pri_taskTra = bltPri.pri_cal[pExtLkTsk_left->scheTask_oft];
                 //priority higher than exist task, can insert target task
                if(pri_taskCur > pri_taskTra){
                    pExtLkTsk_left_prev->next = pTsk_cur;
                    pTsk_cur->next = pExtLkTsk_right;
                    if(pExtLkTsk_left_prev == bltSche.pTask_cur){ //match in first traverse
                        bltSche.pTask_next = pTsk_cur;
                        DBG_CHN0_HIGH;
                        DBG_CHN0_LOW;
                    }
                    DBG_CHN0_HIGH;
                    DBG_CHN0_LOW;
                    DBG_CHN0_HIGH;
                    DBG_CHN0_LOW;
                }else{
                    my_dump_str_data(0, "pawr_rsp task abandon", 0, 0);
                    DBG_CHN0_HIGH;
                    DBG_CHN0_LOW;
                    return 0;
                }
            }
            break;  //exit while 1
        }else{  //traverse to next
            pExtLkTsk_left_prev = pExtLkTsk_left;
            pExtLkTsk_left = pExtLkTsk_left->next;
            pExtLkTsk_right = pExtLkTsk_right->next;
        }
    } /* the end of while() */

    if(RSP_task_allocate){
        blt_sche_addTaskMask(TSKMSK_PAWRA_RSP_0 << index);
        /* Add to the future task table */
        u32 cur_tick_rspSlots = SSLOT_ABS_2_TICKS_ABS(pTsk_cur->begin);
        u32 tickDur_rspSlots = SSLOT_DUR_2_TICKS_DUR(sSlot_duration_rsp);
        blt_add_future_task(TSKFLG_PAWRA_RSP, TSKOFT_PAWRA_RSP + index, cur_tick_rspSlots, cur_tick_rspSlots + tickDur_rspSlots);
        DBG_CHN6_HIGH;
        DBG_CHN6_LOW;
    }else{
        DBG_CHN6_HIGH;
        DBG_CHN6_LOW;
        DBG_CHN6_HIGH;
        DBG_CHN6_LOW;
    }

    return 0;
}

_attribute_ram_code_
int blt_pawra_rsp_sch_build(void)
{
    st_prd_adv_t *cur_pPerdadv = NULL;

    for(int i=0; i<bltFutTask.number; i++)
    {
        future_task_e *pFutTask = (future_task_e *)&bltFutTask.task_tbl[i];

        /* must calculate in for loop, cause "sSlot_endIdx_maxPri" may changed in for loop */
        u32 ll_endTick = SSLOT_ABS_2_TICKS_ABS(bltSche.sSlot_endIdx_dft);
        /* current task start tick is beyond link_list, finish */
        if(tick1_exceed_tick2(pFutTask->tick_s, ll_endTick)){ //for big interval
            continue; //attention: can not use break !!!
        }

        if(pFutTask->task_flg == TSKFLG_PAWRA_RSP){
            int task_abandon = 0;
            u8 pawra_idx = pFutTask->task_oft - TSKOFT_PAWRA_RSP;
            cur_pPerdadv = (st_prd_adv_t *)(global_pPerdadv + pawra_idx);

            if(tick1_exceed_tick2(bltSche.sSlot_tick_next, pFutTask->tick_s)){ //timing passed
                task_abandon = 1;
                bltPri.csctvAbandonCnt[pFutTask->task_oft]++;
                my_dump_str_u32s(0, "abandon pawr_rsp task: time passed", bltSche.sSlot_tick_next, pFutTask->tick_s, pFutTask->task_oft, bltPri.csctvAbandonCnt[pFutTask->task_oft]);
            }
            else{
                s32 sSlot_start = TICKS_ABS_2_SSLOT_ABS(pFutTask->tick_s);
                s32 sSlot_end = TICKS_ABS_2_SSLOT_ABS(pFutTask->tick_e);

                if(sSlot_end < bltSche.sSlot_endIdx_dft){ //new task in correct range
                    sch_task_t *pTsk_cur = (sch_task_t *)&cur_pPerdadv->rspSchTsk_fifo;
                    pTsk_cur->begin = sSlot_start;
                    pTsk_cur->end = sSlot_end;
                    if(blt_ll_addTask2ExistLinklist(pTsk_cur, 1) == 1){
                        task_abandon = 1;
                        my_dump_str_data(0, "pawr_rsp task abandon", 0, 0);
                    }
                }
                else{ //new task across "sSlot_endIdx_dft"
                    my_dump_str_data(0, "across end_idx", 0, 0);
                    //for those task across end_idx, find the task with highest priority, to guarantee that task not missed
                    if(bltPri.pri_cal[pFutTask->task_oft] > bltPri.priMax_value){
                        bltPri.priMax_value = bltPri.pri_cal[pFutTask->task_oft];
                        bltSche.sSlot_endIdx_maxPri = sSlot_start;
                    }
                    break;
                }
            }

            if(task_abandon){

                DBG_CHN7_HIGH;
                DBG_CHN7_LOW;
                DBG_CHN7_HIGH;
                DBG_CHN7_LOW;
                blt_sche_removeTaskMask(TSKMSK_PAWRA_RSP_0 << pawra_idx);
                bltFutTask.number --;
                if(i != bltFutTask.number){
                    smemcpy4(&bltFutTask.task_tbl[i], &bltFutTask.task_tbl[i+1], sizeof(future_task_e)*(bltFutTask.number - i) );
                    i--; //add by qiuwei, note: i-- and i++ in "(int i=0; i<bltFutTask.number; i++)"
                }
            }
            else{
                DBG_CHN8_HIGH;
                DBG_CHN8_LOW;
                DBG_CHN8_HIGH;
                DBG_CHN8_LOW;
            }
        }
    }

    return 0;
}

_attribute_ram_code_
int blt_pawra_task_conflic(sch_task_t *pTgtTsk, u8 curSchTaskOft)
{
    u8 tgtTskFlg = pTgtTsk->scheTask_flg & TSKFLG_VALID_MASK;
    (void)tgtTskFlg;

    if(curSchTaskOft < TSKOFT_PAWRA_RSP){
        my_dump_str_u32s(0, "[pawra_sub],tgtTsk", tgtTskFlg, curSchTaskOft, bltPri.pri_cal[curSchTaskOft], bltPri.pri_cal[pTgtTsk->scheTask_oft]);
    }
    else{
        my_dump_str_u32s(0, "[pawra_rsp],tgtTsk", tgtTskFlg, curSchTaskOft, bltPri.pri_cal[curSchTaskOft], bltPri.pri_cal[pTgtTsk->scheTask_oft]);
    }

    #if(SCH_TASK_PRIORITY_IN_CB_EN)
        s32 pri_taskCur = bltPri.pri_cal[curSchTaskOft];
        s32 pri_taskTra = bltPri.pri_cal[pTgtTsk->scheTask_oft];
         //priority higher than exist task, can insert target task
        if(pri_taskCur > pri_taskTra){
            return 1;
        }
    #endif

    //Task scheduler has been abandoned bigger than 5 times
    if(bltPri.csctvAbandonCnt[curSchTaskOft] >= 5){
        my_dump_str_data(0, "consecutive abandon count", &bltPri.csctvAbandonCnt[curSchTaskOft], 2);
        return 1; /* 1:conflict resolved; 0: insert task failed */
    }

    return 0;
}

_attribute_ram_code_
int blt_pawra_subx_interrupt_task (int flag, void*p)
{
    int index = flag & FLAG_SCHEDULE_TASK_IDX_MASK;

    if(flag & FLAG_SCHEDULE_PAWRA_1ST_SUB) {
        /* subevent0 t task processed in periodic adv task. here mark 1st subevent info. */
        blt_pawra_sub0_mark(index);
    }
    else if(flag & FLAG_SCHEDULE_PAWRA_ADVDATA_UPT){
        blt_pawra_subx_advdata_update();
    }
    else if(flag & FLAG_SCHEDULE_PAWRA_CONN_REQ){
        blt_pawra_sub_prepare_connect(index);
    }
    else if(flag & FLAG_SCHEDULE_START){
        blt_pawra_subx_start(index);
    }
    else if(flag & FLAG_SCHEDULE_BUILD){
        blt_pawra_subx_sch_build();
    }
    else if(flag & FLAG_INSERT_SCHTSK_CONFLICT){
        return blt_pawra_task_conflic((sch_task_t *)p, TSKOFT_PAWRA_SUB + index);
    }

    return 0;
}

_attribute_ram_code_
int blt_pawra_sub0_mark(int index)
{
    /* Update RF channel (use CSA#2 subevent channel) */
    blt_pPerdadv->paSubEventCnt = 1;
    blt_pPerdadv->rspChnIdx = blt_ll_generateNextChannel(&blt_pPda->chnParam, blt_pPda->paEvtCnt, blt_pPda->chnIdentifier, 1);//blt_pPerdadv->paSubEventCnt real value is 0
    rf_set_ble_channel(blt_pPerdadv->rspChnIdx);

    #if (LL_FEATURE_ENABLE_LE_AOA_AOD)
    if(cte_connLess_switchPattern[bltPdaSync.pdA_sync_sel].cte_rx_mode_en){
        blt_pPdAsync->sync_cte_chnIdx = blt_pPerdadv->rspChnIdx;
    }
    #endif

    blt_pPerdadv->sSlot_mark_subx = bltSche.sSlot_idx_irq_real;
    blt_pPerdadv->sSlot_mark_end_subx = blt_pPerdadv->sSlot_mark_subx + blt_pPerdadv->sSlot_subevent_itvl*(blt_pPerdadv->num_subevents - 1) + blt_pPda->sSlot_duration_pda + 10; //margin 220us.

    blt_sche_addTaskMask(TSKMSK_PAWRA_SUB_0 << index);
    //Rebuild sch task table ASAP.
    blt_sche_addUpdate(SLOT_UPDT_SLOTTBL_RESCHED);

    return 0;
}

_attribute_ram_code_
int blt_pawra_sub_prepare_connect(int index)
{
    (void)index; //unused, remove warning

    /* RX buffer DMA set */
    ble_rf_set_rx_dma((u8*)glb_temp_rx_buff, 4);//64/16=4
    rf_set_rx_maxlen(14); //aux_conn_rsp 14 Byte
    rf_ble_set_rx_timeout(bltPHYs.prmb_ac_us + 150 + 20);  //leave 20 uS margin
    //reg_rf_irq_status = FLD_RF_IRQ_ALL; //clear all rf irq status
    rf_clr_irq_status(FLD_RF_IRQ_ALL); //add by Yafei,240530

    if(!blmsParam.new_conn_forbidden){
        u32 tx_begin_tick = bltSche.bSlot_tick_irq_real + TLK_TX_TRIG_OFFSET * SYSTEM_TIMER_TICK_1US;
        rf_start_fsm(FSM_TX2RX, &pkt_init, tx_begin_tick);

        if(blt_ll_init_filter(0, bltInit.creatConCmd_peerAdrType, pkt_init.txAddr, bltInit.creatConCmd_peerAddr, pkt_init.initA))
        {
            if(blms_m_connect((rf_packet_connect_t *)&pkt_init, NULL))
            {
                //clear in the complete event proc by lihaojie 2024.6.11.
                //blmsParam.create_connection = 0;
            }
        }

        STOP_RF_STATE_MACHINE;
        CLEAR_ALL_RFIRQ_STATUS; //clear
    }

    /* not insert RSP TASK */
    blt_pPerdadv->rsp_slot_count = 0;

    return 0;
}

_attribute_ram_code_
int blt_pawra_subx_start(int index)
{
    DBG_CHN4_HIGH;
    bltPrdA.prd_adv_sel = index;
    blt_pPerdadv = (st_prd_adv_t *) (global_pPerdadv + bltPrdA.prd_adv_sel);
    blt_pextadv = (st_ext_adv_t *) (global_pextadv + blt_pPerdadv->mapping_extadv_idx);
    blt_pPda = (st_pda_t *) &blt_pPerdadv->pda_tx;

    /* Different process for different MCU: ******************************************/
    ble_rf_set_tx_dma(0, 17);
     //  This register exists in Kite/Vulture/826x, riscv architecture chips such as
     //  Eagle do not have this register, but DMA fifo can be turned off by forcibly
     //  setting DMA TX rptr = DMA TX wptr to use DMA default tx fifo.If the hardware
     //  TX rptr of DMA TX fifo == hardware TX wptr, then send DMA Tx default fifo,
     //  otherwise send DMA Tx fifo non-default area.
    HAL_REG_RF_DMA_FIFO_TX_RPTR = FLD_DMA_RPTR_CLR;
    /**********************************************************************************/

    /* PHY switch, do not consider S2 */
    rf_ble_switch_phy(blt_pPda->pda_phy, LE_CODED_S8);
    if(bltPHYs.cur_llPhy == BLE_PHY_CODED){
        rf_trigger_codedPhy_accesscode();
    }

    //--------------- interval jump process --------------------------------------------------------//
    int inter_jump_num = 0;

    /* 3125 uS = 625uS *5 = 32*5 sSlot
     * consider slot adjust when low power enable, margin set to 5 slot is more safer, minimum connection interval 7.5mS is 12 Slot,
     * 5 slot margin can handle 3.125 mS timing shit, and maximum 10 slot 6.25mS error no risk for inter_jump_num */
    inter_jump_num = (bltSche.sSlot_idx_irq_real + 5*32 - blt_pPerdadv->sSlot_mark_subx)/blt_pPerdadv->sSlot_subevent_itvl - 1;
    my_dump_str_u32s(0, "pda jump ", blt_pPerdadv->sSlot_mark_subx, bltSche.sSlot_idx_irq_real, blt_pPerdadv->sSlot_subevent_itvl, inter_jump_num);

    /* subevent jump, each jump need to calculate for the next subevent */
    for(int i = 0; i < inter_jump_num; i++){ //periodic_adv_subevent_interval jump happens
        blt_pPerdadv->paSubEventCnt++;
        blt_ll_generateNextChannel(&blt_pPda->chnParam, (blt_pPda->paEvtCnt - 1)^(blt_pPerdadv->paSubEventCnt-1), blt_pPda->chnIdentifier, 1); //plus ONE must careful
    }

    if(inter_jump_num > 0){
        blt_ll_incSchedulerTaskPriority(TSKOFT_PAWRA_SUB + index, bltPri.step_final[TSKOFT_PAWRA_SUB + index]*2*inter_jump_num);
    }

    /* Update RF channel (use CSA#2 subevent channel) */
    blt_pPerdadv->paSubEventCnt++;
    blt_pPerdadv->rspChnIdx = blt_ll_generateNextChannel(&blt_pPda->chnParam, (blt_pPda->paEvtCnt - 1)^(blt_pPerdadv->paSubEventCnt-1), blt_pPda->chnIdentifier, 1); //plus ONE must careful
    rf_set_tx_rx_off();
    rf_set_ble_channel(blt_pPerdadv->rspChnIdx);
    rf_set_ble_access_code((u8 *)&blt_pPda->paAccessAddr);
    rf_set_ble_crc_value(blt_pPda->paCrcInit);

    blt_ll_set_tx_power_by_strategy(TX_POWER_STRATEGY_CUSTOMER_OR_DEFAULT, 0);

    #if (LL_FEATURE_ENABLE_LE_AOA_AOD)
    if(cte_connLess_switchPattern[bltPdaSync.pdA_sync_sel].cte_rx_mode_en){
        blt_pPdAsync->sync_cte_chnIdx = blt_pPerdadv->rspChnIdx;
    }
    #endif

    rf_ble_set_tx_settle(bltPHYs.tx_stl_adv); //attention: must set after PHY switch !!!

    STOP_RF_STATE_MACHINE;  // stop SM
    HAL_CLEAR_RF_TX_RX_IRQ;

#if(LL_FEATURE_ENABLE_CONNECTIONLESS_CTE_TRANSMITTER)
    u8 prdIdx = blt_pPerdadv->prdadv_index;
    if(cte_connLess_switchPattern[prdIdx].cte_transmit_en){
        rf_set_aoa_aod_trx_mode(RF_TX_ADV_AOA_EN);
    }
#endif

    if(blt_pPerdadv->acad_chaged){
        if(blt_pPerdadv->acad_chaged == 2){
            //Rebuild sch task table ASAP.
            blt_sche_addUpdate(SLOT_UPDT_SLOTTBL_RESCHED);
            blt_pPerdadv->acad_chaged = 0;
        }
    }
    else{
        u32 tx_begin_tick = bltSche.sSlot_tick_irq_real + TLK_TX_TRIG_OFFSET * SYSTEM_TIMER_TICK_1US;

        if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_TX_ON); }

        if(blmsParam.create_connection == CONNECT_REQ_FOR_PAWR){
            if(blt_pPerdadv->paSubEventCnt == blt_pPerdadv->initSubevent){
                blt_pawra_sub_prepare_connect(index); //blt_pawra_sub_prepare_connect
                goto skip_pawr_subeventX_send;
            }
        }

        /* step 1: trigger RF mode  */
        rf_start_fsm(FSM_STX, &pkt_periodic, tx_begin_tick);
        /* step 2: set AUX_SYNC_IND packet parameters by order  */
        smemcpy(&pkt_periodic, &blt_pPerdadv->prd_adv_1stPkt, blt_pPerdadv->prd_adv_1stPkt.ext_hdr_len - 1 + AUX_ADV_FORMAT_LEN);
        if(blt_pPerdadv->include_ADI_flag){
            blt_pPerdadv->prd_DID = ((clock_time()>>4) & 0xFFF)  | 0x001;
            u16 adi_info = blt_pextadv->adv_sid<<12|blt_pPerdadv->prd_DID;
            smemcpy((pkt_periodic.data + blt_pPerdadv->auxPtr_offset), &adi_info, EXTHD_LEN_2_ADI);

        }
        /* step3: Make AUX_SYNC_IND ACAD && data PDU field part */
        if(blt_pPerdadv->acad_used & PERD_ACAD_CHMUPT_ENA){ ////If exist ACAD field(e.g.: ChmUpt
            //u8 chmUptInd[9]= { 8, DT_CHM_UPT_IND, 0, 0, 0, 0, 0 , 0, 0};
            u8 chmUptInd[9];
            chmUptInd[0] = 8;
            chmUptInd[1] = DT_CHM_UPT_IND;

            smemcpy(chmUptInd + 2, blt_pPerdadv->pda_tx.nextChn.chmTbl, 5); //chm
            *(u16*)&chmUptInd[7] = blt_pPerdadv->pda_tx.prd_map_inst_next; //Instant
            smemcpy((pkt_periodic.data + blt_pPerdadv->txPower_offset), chmUptInd, sizeof(chmUptInd));
        }
        else if(blt_pPerdadv->acad_used & PERD_ACAD_BIGINFO_ENA){ //If exist ACAD field(e.g.: BigInfo, BIS concerned)
            //If BIG is enabled, update BigInfo field before RF send it.
            if(perd_adv_biginfo_update_cb){
                perd_adv_biginfo_update_cb(bltPrdA.prd_adv_sel); //blt_ll_perdAdvAcadUpdateBigInfo
            }
            smemcpy((pkt_periodic.data + blt_pPerdadv->txPower_offset), blt_pPerdadv->acad, blt_pPerdadv->acad_field_len);
        }

        // 1st: update PAwR-AdvData ASAP.
        blt_pawra_subx_advdata_update();

        #if(ADV_DURATION_STALL_EN)
            cpu_stall_WakeUp_By_RF_SystemTick(IRQ_ZB_RT, FLD_RF_IRQ_TX, 0);
        #else
            while (!HAL_GET_RF_TX_IRQ){//wait for TX finish
                if(usr_irq_handler_cb){ usr_irq_handler_cb(); }
            }
        #endif
    }

#if(LL_FEATURE_ENABLE_CONNECTIONLESS_CTE_TRANSMITTER)
    rf_set_aoa_aod_trx_mode(RF_AOA_OFF);
#endif

skip_pawr_subeventX_send:

    /* Update current sSlot_mark_subx [update AP must before 'insert RSP TASK'] */
    blt_pPerdadv->sSlot_mark_subx = bltSche.sSlot_idx_irq_real;

    //2nd: Subevent Data Request event; 3th: insert RSP TASK
    blt_pawra_rsp_task_insert(index);

    /* important: ensure that FSM stopped */
    STOP_RF_STATE_MACHINE;
    /* clear status as late as possible, cause if clear too early, some status did not come, e.g. STX cmd done*/
    CLEAR_ALL_RFIRQ_STATUS;

    blms_state = BLMS_STATE_EXTADV_E;

    DBG_CHN4_LOW;

    //s32 sSlotCurr = TICKS_ABS_2_SSLOT_ABS(clock_time());
    s32 sSlotTaskEnd = bltSche.sSlot_idx_irq_real + blt_pPerdadv->pda_tx.sSlot_duration_pda;
    //my_dump_str_u32s(0, "xxx", sSlotCurr, sSlotTaskEnd, sSlotTaskEnd-sSlotCurr, sSlotCurr-sSlotTaskEnd);

    /* If reschedule task (here insert rsp task will trigger reschedule), need margin time, here 100us seems safe */
    blt_ll_calculate_sSlot_next(min(clock_time() + SLOT_PROCESS_MAX_TICK, SSLOT_ABS_2_TICKS_ABS(sSlotTaskEnd))); //SLOT_PROCESS_MAX_TICK);

    return 0;
}

_attribute_ram_code_
int blt_pawra_subx_advdata_update(void)
{
    /* clear Response slot configuration */
    blt_pPerdadv->rsp_slot_start = 0;
    blt_pPerdadv->rsp_slot_count = 0;
    /* 1st: update PAwR_IND's AdvData content ASAP. */
    if(blt_pPda->paEvtCnt - 1 == blt_pPerdadv->subeventData_validPaEvt){
        u8 subevent_idx = blt_pPerdadv->paSubEventCnt - 1;
        if(blt_pPerdadv->pdaSubevtDataCtrl[subevent_idx].subevent_idx != 0xFF) {
            u8 *p = blt_pPerdadv->pdaSubevtDataCtrl[subevent_idx].pSubevt_data;
            u8 len = blt_pPerdadv->pdaSubevtDataCtrl[subevent_idx].subevt_data_len;
            pkt_periodic.rf_len  += len;
            pkt_periodic.dma_len = rf_tx_packet_dma_len(pkt_periodic.rf_len + 2);
            smemcpy((pkt_periodic.data + blt_pPerdadv->ACAD_advData_offset), p, len);

            blt_pPerdadv->pdaSubevtDataCtrl[subevent_idx].subevent_idx = 0xFF; //mark it's value to invalid

            /* mark Response slot configuration */
            blt_pPerdadv->rsp_slot_start = blt_pPerdadv->pdaSubevtDataCtrl[subevent_idx].rsp_slot_start;
            blt_pPerdadv->rsp_slot_count = blt_pPerdadv->pdaSubevtDataCtrl[subevent_idx].rsp_slot_count;

            DBG_CHN12_HIGH;
            DBG_CHN12_LOW;
        }
    }

    return 0;
}

_attribute_ram_code_
int blt_pawra_subx_sch_build(void)
{
    u16 int_jump_task;
    s32 sSlot_start_task;
    st_prd_adv_t *cur_pPerdadv = NULL;

    for(int i=0; i<TSKNUM_PAWRA_SUB; i++)
    {
        if(bltSche.task_mask & (TSKMSK_PAWRA_SUB_0<<i))
        {
            cur_pPerdadv = (st_prd_adv_t*)(global_pPerdadv + i);
            cur_pPerdadv->subSchTsk_wptr = cur_pPerdadv->subSchTsk_rptr = 0;

            if(bltSche.build_index == 0 && bltSche.sSlot_idx_reset == 1){
                cur_pPerdadv->sSlot_mark_subx -= bltSche.sSlot_idx_past;
                cur_pPerdadv->sSlot_mark_end_subx -= bltSche.sSlot_idx_past;
            }

            if( cur_pPerdadv->sSlot_mark_subx >= bltSche.sSlot_idx_next){
                int_jump_task = 0;
                sSlot_start_task = cur_pPerdadv->sSlot_mark_subx + cur_pPerdadv->sSlot_subevent_itvl;
                my_dump_str_data(0,"Insert task0: jump pawra_sub", &int_jump_task, 2);
            }
            else
            {
                int_jump_task = (bltSche.sSlot_idx_next - 1 - cur_pPerdadv->sSlot_mark_subx)/cur_pPerdadv->sSlot_subevent_itvl;
                sSlot_start_task = cur_pPerdadv->sSlot_mark_subx + (int_jump_task + 1)*cur_pPerdadv->sSlot_subevent_itvl;
                my_dump_str_data(0,"Insert task1: jump pawra_sub", &int_jump_task , 2);
                //blt_ll_incSchedulerTaskCalPriority( TSKOFT_PAWRA_SUB + i, bltPri.step_final[TSKOFT_PAWRA_SUB + i]*2*int_jump_task);
            }

            if(sSlot_start_task >= bltSche.sSlot_endIdx_dft){ //to save some time for big interval
                my_dump_str_data(0,"to save some time for big interval",  0, 0);
                continue; //attention: can not use break !!!
            }

            int new_task_cnt = 0;
            for(int j=0;j<PERD_ADV_FIFONUM;j++){

                sch_task_t  *pCur_schTask = (sch_task_t *)&cur_pPerdadv->subSchTsk_fifo[j];

                pCur_schTask->begin = sSlot_start_task + j*cur_pPerdadv->sSlot_subevent_itvl;
                pCur_schTask->end = pCur_schTask->begin + cur_pPerdadv->pda_tx.sSlot_duration_pda - 1;

                /* subevent task duration life expired */
                if(pCur_schTask->end > cur_pPerdadv->sSlot_mark_end_subx){
                    my_dump_str_data(0,"subevent task duration life expired", 0, 0);
                    blt_sche_removeTaskMask(TSKMSK_PAWRA_SUB_0 << i);
                    //DBG_CHN6_TOGGLE;
                    //DBG_CHN6_TOGGLE;
                    break;
                }
                else if(pCur_schTask->begin >= bltSche.sSlot_endIdx_dft){ //new task beyond correct range, finish
                    my_dump_str_data(0,"new task beyond correct range, finish",  0, 0);
                    break;
                }
                else if(pCur_schTask->end < bltSche.sSlot_endIdx_dft){ //new task in correct range
                    cur_pPerdadv->subSchTsk_wptr = j;
                    new_task_cnt ++;
                    //blt_ll_incSchedulerTaskCalPriority( TSKOFT_PAWRA_SUB + i, -bltPri.step_final[TSKOFT_PAWRA_SUB + i]);
                }
                else{ //new task across "sSlot_endIdx_dft"
                    my_dump_str_data(0,"new task across @sSlot_endIdx_dft@",  0, 0);
                    //for those task across end_idx, find the task with highest priority, to guarantee that task not missed
                    if(bltPri.pri_cal[TSKOFT_PAWRA_SUB + i] > bltPri.priMax_value){
                        bltPri.priMax_value = bltPri.pri_cal[TSKOFT_PAWRA_SUB + i];
                        bltPri.priMax_index = TSKOFT_PAWRA_SUB + i;
                        bltSche.sSlot_endIdx_maxPri = pCur_schTask->begin;
                        my_dump_str_u32s(SCHE_TIMING_IMPROVE_DBG_EN,"across IDX perd_wr", i, bltSche.sSlot_endIdx_dft, bltSche.sSlot_endIdx_maxPri, bltPri.priMax_value);
                    }
                    break;
                }
            }

            if(new_task_cnt){
                my_dump_str_data(0,"new_task_cnt",  &new_task_cnt, 1);
                blt_ll_addTask2ExistLinklist(&cur_pPerdadv->subSchTsk_fifo[0],cur_pPerdadv->subSchTsk_wptr + 1);
            }
        }
    }

    return 0;
}

_attribute_noinline_
static void blt_reset_pawra(void)
{
    st_prd_adv_t *cur_pPerdadv;
    for(int i=0; i<bltPrdA.maxNum_perdAdv; i++){
        cur_pPerdadv = (st_prd_adv_t *)(global_pPerdadv + i);
        cur_pPerdadv->num_subevents = 0;
        cur_pPerdadv->responseAA = 0;
        cur_pPerdadv->subevent_interval = 0;
        cur_pPerdadv->num_response_slots = 0;
        cur_pPerdadv->response_slot_delay = 0;
        cur_pPerdadv->response_slot_spacing = 0;
        cur_pPerdadv->initSubevent = 0;

        for(int j=0; j<15; j++){
            cur_pPerdadv->pdaSubevtDataCtrl[j].pSubevt_data = NULL;
            cur_pPerdadv->pdaSubevtDataCtrl[i].subevent_idx = 0xFF; //invalid
        }
        //......
    }
}

_attribute_noinline_
int blt_ll_procPeriodicAdvRspReportEvent(void)
{
    u8 *raw_pkt = NULL;
    while (pawra_RxFifo.rptr != pawra_RxFifo.wptr)
    {
        raw_pkt = (u8 *) (pawra_RxFifo.p + PAWRA_RXFIFO_SIZE * (pawra_RxFifo.rptr & PAWRA_RXFIFO_MASK));
        pawra_RxFifo.rptr++;
        u8 rf_len = raw_pkt[DMA_RFRX_OFFSET_RFLEN];
        //tlkapi_send_string_data(rf_len, "raw", &raw_pkt[DMA_RFRX_LEN_HW_INFO], 6+rf_len);

        //Mark it's a PAwR response slot Data PDU
        u16 currPaEvtCnt = raw_pkt[0]; //current PAwR event
        u8 paHandle = raw_pkt[1];
        u8 response_slot_idx = raw_pkt[2]; //offset start from 0
        u8 paSubEventCnt = raw_pkt[3]; //offset start from 0

        (void)currPaEvtCnt;

        s8 rssi = rf_len ? (raw_pkt[DMA_RFRX_OFFSET_RSSI(raw_pkt)] - 110) : 0x7F;
        s8 tx_power = TX_POWER_INFO_NOT_AVAILABLE;
        rf_pkt_ext_adv_t *pPawrRspSlotDat  = (rf_pkt_ext_adv_t *) (raw_pkt + DMA_RFRX_LEN_HW_INFO);

        /*
         * AdvA TargetA CTE_Info ADI Aux_Ptr Sync_Info Tx_Power ACAD AdvData
         *   O     X       O      X     X        X        O       O    O
         * AUX_SYNC_SUBEVENT_RSP PDU
         */
        u8 *pAdvData = NULL;
        u8 cte_info = 0, extHdr_offset = 0, advDataLen = 0;

        if( rf_len && (pPawrRspSlotDat->ext_hdr_len != 0)){
            if(pPawrRspSlotDat->ext_hdr_flg & EXTHD_BIT_ADVA){
                extHdr_offset += EXTHD_LEN_6_ADVA;
            }
            if(pPawrRspSlotDat->ext_hdr_flg & EXTHD_BIT_CTE_INFO){
                extHdr_offset += EXTHD_LEN_1_CTE;
                cte_info = *(u8*)(pPawrRspSlotDat->data + extHdr_offset);
            }
            if(pPawrRspSlotDat->ext_hdr_flg & EXTHD_BIT_TX_POWER){
                extHdr_offset += EXTHD_LEN_1_TX_POWER;
                tx_power = pPawrRspSlotDat->data[extHdr_offset];
            }

        }
        if(pPawrRspSlotDat->rf_len > pPawrRspSlotDat->ext_hdr_len + 1){ //with "Adv Data"
            advDataLen = pPawrRspSlotDat->rf_len - (pPawrRspSlotDat->ext_hdr_len + 1);
            pAdvData = (pPawrRspSlotDat->ext_hdr_len!=0) ? (&pPawrRspSlotDat->data[pPawrRspSlotDat->ext_hdr_len - 1]) : (&pPawrRspSlotDat->ext_hdr_flg);
            //tlkapi_send_string_data(rf_len, "AdvData", pAdvData, advDataLen);
        }
    



        u8 temp_buff[256] = { 0 };  //process max length 255
        hci_le_periodicAdvRspReportEvt_t *lePeriodAdvRspReportEvt = (hci_le_periodicAdvRspReportEvt_t *)&temp_buff[0];
        lePeriodAdvRspReportEvt->subEventCode = HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_RESPONSE_REPORT;
        lePeriodAdvRspReportEvt->advHandle = paHandle;
        lePeriodAdvRspReportEvt->Subevent = paSubEventCnt;
        lePeriodAdvRspReportEvt->Tx_Status = 0;
        lePeriodAdvRspReportEvt->Num_Responses = 1;
        lePeriodAdvRspReportEvt->rspReportDat[0].RSSI = rssi;
        lePeriodAdvRspReportEvt->rspReportDat[0].cteType = cte_info;
        memcpy(lePeriodAdvRspReportEvt->rspReportDat[0].data, pAdvData, advDataLen);
        lePeriodAdvRspReportEvt->rspReportDat[0].dataLength = advDataLen;
        /* dataStatus: 0xFF:  Failed to receive an AUX_SYNC_SUBEVENT_RSP PDU */
        lePeriodAdvRspReportEvt->rspReportDat[0].dataStatus =  rf_len ? PDA_SYNC_REPORT_DATA_COMPLETE : 0xFF;
        lePeriodAdvRspReportEvt->rspReportDat[0].responseSlot = response_slot_idx;
        lePeriodAdvRspReportEvt->rspReportDat[0].txPower = tx_power;
        int len = sizeof(hci_le_periodicAdvRspReportEvt_t) + sizeof(pawrRspReportDat_t) + advDataLen;

        if(hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_PERIODIC_ADVERTISING_RESPONSE_REPORT){
            blc_hci_send_event (HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, temp_buff, len);
        }
    }

    return 0;
}

_attribute_noinline_
int blt_pawra_mainloop_task (int flag, void *p)
{
    (void)p; //unused, remove warning

    //int index = flag & FLAG_SCHEDULE_TASK_IDX_MASK;
    //st_prd_adv_t *cur_pPerdadv = (st_prd_adv_t *)(global_pPerdadv + index);

    if(flag & FLAG_MODULE_MAINLOOP){
        blt_pawra_mainloop();
    }
    else if(flag & FLAG_MODULE_RESET){
        blt_reset_pawra();
    }

    return 0;
}

void blt_pawra_mainloop(void)
{
    st_prd_adv_t *pPerdadv = NULL;
    for(int i=0; i < bltPrdA.maxNum_perdAdv; i++)
    {
        pPerdadv = (st_prd_adv_t *)(global_pPerdadv + i);

        if(pPerdadv->prd_adv_en){
            /* LE Periodic Advertising Subevent Data Request event */
            if(pPerdadv->subeventData_evtTrig){
                blt_ll_periodicAdvSubeventDataReq(pPerdadv);

                u32 r = irq_disable();
                pPerdadv->subeventData_evtTrig = 0;
                irq_restore(r);
            }

            /* Process PAwR-Advertiser  */
            if(pawra_RxFifo.rptr != pawra_RxFifo.wptr){
                blt_ll_procPeriodicAdvRspReportEvent();
            }
        }
    }

    //LE Periodic Advertising Response Report event



}

/* If the combined data length is greater than the maximum that the Controller can transmit within the current
 * subevent interval, then all data shall be discarded and the Controller shall return the error code Packet Too Long (0x45).
 * If advertising on the LE Coded PHY, then the S=8 coding shall be assumed unless the current advertising parameters require
 * the use of S=2 for an advertising physical channel, in which case the S=2 coding shall be assumed for that advertising
 * physical channel.
 */
static bool blt_ll_advPeriodicChkRspDataItvl(st_prd_adv_t* cur_pPerdadv, u8 subevtDataLen, u16 subevtItvl)
{
    bool cte_en = 1;

    u16 firstPkt_extHdrLen = (cte_en ? EXTHD_LEN_1_CTE : 0) + \
                             (cur_pPerdadv->txPower_en_len ? EXTHD_LEN_1_TX_POWER : 0) + \
                             (cur_pPerdadv->include_ADI_flag ? EXTHD_LEN_2_ADI : 0) +
                              9 + /* ACAD for channel map update */ + \
                              1 /* Extended Header Flags */;
    u16 payloadLen = (1 + firstPkt_extHdrLen) + subevtDataLen;

    u32 totalTime_us = 150 + blt_phy_getRfPacketTime_us(payloadLen, cur_pPerdadv->pda_tx.pda_phy, cur_pPerdadv->coding_ind);

    if(totalTime_us >= subevtItvl*1250){
        return TRUE;
    }

    return FALSE;
}

hci_le_setPeriodicAdvParamV2_retParam_t blc_ll_setPeriodicAdvParam_v2(adv_handle_t adv_handle, u16 advInter_min, u16 advInter_max,  perd_adv_prop_t property,
                                          u8 numSubevents,u8 subeventInterval, u8 responseSlotDelay,u8 responseSlotSpace,u8 numResponseSlots)
{
    hci_le_setPeriodicAdvParamV2_retParam_t retParamV2;
    retParamV2.adv_handle = adv_handle;
    //HCI/GEV/BV-02-C [Disallow Mixing Legacy and Extended Advertising Commands]
    if(IS_LEGACY_ADV_VALID){
        retParamV2.status = HCI_ERR_CMD_DISALLOWED;
        return retParamV2;
    }
    SET_EXTENDED_ADV_VALID;

    st_ext_adv_t *cur_pextadv;
    if(adv_handle == INVALID_ADVHD_FLAG){
        retParamV2.status = HCI_ERR_INVALID_HCI_CMD_PARAMS;
        return retParamV2;
    }
    else{
        /*If the corresponding advertising set does not already exist, then the Controller shall return the error
          code Unknown Advertising Identifier (0x42). */
        cur_pextadv = blt_extadv_search_existed_adv_set(adv_handle);
        if(!cur_pextadv){
            retParamV2.status = HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
            return retParamV2;
        }
    }


    /*If the Advertising_Handle does not identify an advertising set that is already configured for periodic advertising
     * and the Controller is unable to support more periodic advertising at present, the Controller shall return the
     * error code Memory Capacity Exceeded (0x07). */
    st_prd_adv_t *cur_pPerdadv = blt_prdadv_search_existed_and_allocate_new_periodic_adv(adv_handle);
    if(!cur_pPerdadv){
        retParamV2.status = HCI_ERR_MEM_CAP_EXCEEDED;
        return retParamV2;
    }


    /* If the advertising set identified by the Advertising_Handle specified scannable, connectable, legacy,
     * or anonymous advertising, the Controller shall return the error code Invalid HCI Command Parameters (0x12).*/
    if(cur_pextadv->evt_props & (ADVEVT_PROP_MASK_CONNECTABLE | ADVEVT_PROP_MASK_SCANNABLE | ADVEVT_PROP_MASK_LEGACY | ADVEVT_PROP_MASK_ANON_ADV) ){
        retParamV2.status = HCI_ERR_INVALID_HCI_CMD_PARAMS;
        return retParamV2;
    }


    /*If the Host issues this command when periodic advertising is enabled for the specified advertising set,
     * the Controller shall return the error code Command Disallowed (0x0C). */
    if(cur_pextadv->prdadv_api_en){
        retParamV2.status = HCI_ERR_CMD_DISALLOWED;
        return retParamV2;
    }

    /* HCI/DDI/BI-50-C [LE Set Periodic Advertising Parameters, Reject, Data Too Long, LE 1M PHY]*/
    /* HCI/DDI/BI-51-C [LE Set Periodic Advertising Parameters, Reject, Data Too Long, LE Coded PHY]*/
    /*If the advertising set already contains periodic advertising data and the length of the data is greater than the maximum that
     *the Controller can transmit within a periodic advertising interval of Periodic_Advertising_Interval_Max,
     *the the Controller shall return the error code Packet Too Long (0x45). If advertising on the LE Coded PHY,the S=8 coding shall be assumed.*/
    if(blt_ll_advPeriodicChkDataItvl(cur_pPerdadv, advInter_max, cur_pextadv->sec_phy)){
        retParamV2.status = HCI_ERR_PACKET_TOO_LONG;
        return retParamV2;
    }

    //////////// PAwR Parameters setting //////////////////////////////
    cur_pPerdadv->num_subevents = numSubevents;

    if(numSubevents != 0){
        if(subeventInterval < 6){
            retParamV2.status = HCI_ERR_INVALID_HCI_CMD_PARAMS;
            return retParamV2;
        }
        else if((numResponseSlots != 0) && (responseSlotDelay == 0 || responseSlotSpace <= 1)){
            retParamV2.status = HCI_ERR_INVALID_HCI_CMD_PARAMS;
            return retParamV2;
        }
        else if((numResponseSlots == 0) && (responseSlotDelay != 0 || responseSlotSpace != 0)){
            retParamV2.status = HCI_ERR_INVALID_HCI_CMD_PARAMS;
            return retParamV2;
        }
        else if((responseSlotDelay > subeventInterval) || (responseSlotSpace > (subeventInterval - responseSlotDelay)*10/numResponseSlots)){
            
            my_dump_str_data(DBG_PAwR_ADV_LOGIC, "PAwR parameter error", 0, 0);
            retParamV2.status = HCI_ERR_INVALID_HCI_CMD_PARAMS;
            return retParamV2;
        }
        else if(subeventInterval > advInter_min / numSubevents) {
            
            retParamV2.status = HCI_ERR_INVALID_HCI_CMD_PARAMS;
            return retParamV2;
        }

        cur_pPerdadv->num_response_slots = numResponseSlots;

        cur_pextadv->pawr_timing_info.rsp_AA = cur_pPerdadv->responseAA = blt_ll_connCalcAccessAddr_v2();
        cur_pextadv->pawr_timing_info.subevent_intvl = cur_pPerdadv->subevent_interval = subeventInterval; //unit 1.25ms
        cur_pextadv->pawr_timing_info.rsp_slot_delay = cur_pPerdadv->response_slot_delay = responseSlotDelay; //unit 1.25ms
        cur_pextadv->pawr_timing_info.rsp_slot_spacing = cur_pPerdadv->response_slot_spacing = responseSlotSpace; //unit 0.125m
        cur_pextadv->pawr_timing_info.num_subevent = numSubevents;


        cur_pPerdadv->curLen_perdAdvData = 0; //initialize : no Adv Data field

        cur_pPerdadv->sSlot_duration_rsp = TICKS_DUR_2_SSLOT_DUR(cur_pPerdadv->response_slot_spacing*cur_pPerdadv->num_response_slots*SYSTEM_TIMER_TICK_125US + SLOT_PROCESS_MAX_TICK) + 1;
        cur_pPerdadv->sSlot_subevent_itvl = TICKS_DUR_2_SSLOT_DUR(cur_pPerdadv->subevent_interval*SYSTEM_TIMER_TICK_1250US);
        tlkapi_printf(1, "sSlot_duration_rsp:%d\n", (u32)cur_pPerdadv->sSlot_duration_rsp);
        tlkapi_printf(1, "sSlot_subevent_itvl:%d\n", (u32)cur_pPerdadv->sSlot_subevent_itvl);
    } else {
        if(subeventInterval > advInter_min) {
            retParamV2.status = HCI_ERR_INVALID_HCI_CMD_PARAMS;
            return retParamV2;
        }
    }
    /////////////////////////////////////////////////////////////////////////////


    cur_pextadv->mapping_prdadv_idx = cur_pPerdadv->prdadv_index;
    cur_pPerdadv->mapping_extadv_idx = cur_pextadv->extadv_index;
    cur_pPerdadv->advInter_min = advInter_min;
    cur_pPerdadv->advInter_max = advInter_max;
    cur_pPerdadv->property = property;
    cur_pPerdadv->txPower_en_len = (property&PERD_ADV_PROP_MASK_TX_POWER_INCLUDE) ? 1 : 0;


    cur_pPerdadv->pda_tx.pda_phy = cur_pextadv->sec_phy;  //PHY may change later
    cur_pPerdadv->coding_ind = cur_pextadv->coding_ind;
    /* rf_len 255B packet time and n_30us unit can be calculated once secondary_phy is set,
     * so we can do it here early to save running time when updateExtAdvSet*/
    if(cur_pPerdadv->pda_tx.pda_phy == BLE_PHY_1M){
        cur_pPerdadv->rfLen_255_pkt_us = RFLEN_255_1MPHY_PKT_US;
        cur_pPerdadv->n_30us_chain_ind = RFLEN_255_1MPHY_N_30;
    }
    else if(cur_pPerdadv->pda_tx.pda_phy == BLE_PHY_2M){
        cur_pPerdadv->rfLen_255_pkt_us = RFLEN_255_2MPHY_PKT_US;
        cur_pPerdadv->n_30us_chain_ind = RFLEN_255_2MPHY_N_30;
    }
    else{  //Coded PHY
        if(bltPHYs.dft_CI){
            cur_pPerdadv->coding_ind = bltPHYs.dft_CI;
        }
        else{ //CODED_PHY_PREFER_NONE
            cur_pPerdadv->coding_ind = LE_CODED_S8; //dft S8
        }

        if(cur_pextadv->coding_ind == LE_CODED_S2){
            cur_pPerdadv->rfLen_255_pkt_us = RFLEN_255_CODEDPHY_S2_PKT_US;
            cur_pPerdadv->n_30us_chain_ind = RFLEN_255_CODEDPHY_S2_N_30;
        }
        else{
            cur_pPerdadv->rfLen_255_pkt_us = RFLEN_255_CODEDPHY_S8_PKT_US;
            cur_pPerdadv->n_30us_chain_ind = RFLEN_255_CODEDPHY_S8_N_30;
        }
    }


    cur_pextadv->prdadv_update_flag = 1;

#if (LL_FEATURE_ENABLE_CONNECTIONLESS_CTE_TRANSMITTER)
    cte_connLess_switchPattern[adv_handle].sequence_ctrl |= PRD_ADV_SET_PARAM_DONE_FLAG;  //add by Qiuwei.
#endif
    retParamV2.status = BLE_SUCCESS;
    return retParamV2;

}


hci_le_setPeriodicAdvParamV2_retParam_t blc_hci_le_setPeriodicAdvParam_v2(hci_le_setPeriodicAdvParamV2_cmdParam_t* pCmdParam)
{
    /* validate intervals */
    hci_le_setPeriodicAdvParamV2_retParam_t retParamV2;
    retParamV2.adv_handle = pCmdParam->adv_handle;
    if ((pCmdParam->advInter_min < 0x0006) || (pCmdParam->advInter_max < 0x006) || (pCmdParam->advInter_min > pCmdParam->advInter_max)) {
        retParamV2.status = HCI_ERR_INVALID_HCI_CMD_PARAMS;
        return retParamV2;
    }

#if (0)
    //according to the IXIT periodic adv interval setting. now the EBQ setting min pda interval is 100ms
    //HCI/DDI/BI-67-C,but HCI/DDI/BI-15-C ~ HCI/DDI/BI-25-C not use the limit.
    if ((pCmdParam->advInter_min < 80) || (pCmdParam->advInter_max < 80) || (pCmdParam->advInter_min > pCmdParam->advInter_max)) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }
#endif

    return blc_ll_setPeriodicAdvParam_v2(pCmdParam->adv_handle, pCmdParam->advInter_min, pCmdParam->advInter_max,
                                        pCmdParam->property, pCmdParam->numSubevents, pCmdParam->subeventInterval,
                                        pCmdParam->responseSlotDelay, pCmdParam->responseSlotSpace, pCmdParam->numResponseSlots);
}

/*
 * LE Periodic Advertising Subevent Data Request event
 */
int blt_ll_periodicAdvSubeventDataReq(st_prd_adv_t *pPerdadv)
{
    u8 result[sizeof(hci_le_periodicAdvSubevtDataReqEvt_t)] = {0};
    hci_le_periodicAdvSubevtDataReqEvt_t* pEvt = (hci_le_periodicAdvSubevtDataReqEvt_t*)result;

    if(hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_PERIODIC_ADVERTISING_SUBEVENT_DATA_REQUEST){
        pEvt->subEventCode = HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SUBEVENT_DATA_REQUEST;
        pEvt->advHandle = pPerdadv->advHand_mark;
        pEvt->subevtStart = pPerdadv->subDataReq.subevtStart;
        pEvt->subevtDataCount = pPerdadv->subDataReq.subevtCount;

        return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_periodicAdvSubevtDataReqEvt_t));
    }

    return 0;
}

/*
 * LE Set Periodic Advertising Subevent Data command
 */
ble_sts_t   blc_ll_setPeriodicAdvSubeventData(adv_handle_t adv_handle, u8 num_subevent, pdaSubevtData_subevtCfg_t* pSubevtCfg)
{
    if(adv_handle > 0xEF) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }
    st_prd_adv_t * cur_pPerdadv = blt_ll_search_existing_perdAdv_index_by_advHandle(adv_handle);
    if(!cur_pPerdadv){
        return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
    }


    if(num_subevent > 15 || num_subevent == 0){
    
        my_dump_str_data(DBG_PAwR_ADV_LOGIC, "set PDA subevt data, subevent number error", 0, 0);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }
    my_dump_str_data(DBG_PAwR_ADV_LOGIC, "periodic adv idx", &adv_handle, 2);
    if(num_subevent > cur_pPerdadv->subDataReq.subevtCount) {
        return HCI_ERR_PACKET_TOO_LONG;
    }

    u8 *p = (u8*)&pSubevtCfg[0];
    pdaSubevtData_subevtCfg_t* pSubevtCfgInfo = &pSubevtCfg[0];

    for(int i=0; i<num_subevent; i++){
        if(pSubevtCfgInfo->subevent_idx > 0x7F) {
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
        if(pSubevtCfgInfo->subevent_idx < cur_pPerdadv->subDataReq.subevtStart ||\
           pSubevtCfgInfo->subevent_idx > (cur_pPerdadv->subDataReq.subevtStart+cur_pPerdadv->subDataReq.subevtCount) )
        {
            
            return HCI_ERR_CMD_DISALLOWED;
        }
        u8 subevent_idx = pSubevtCfgInfo->subevent_idx;
        if(cur_pPerdadv->pdaSubevtDataCtrl[subevent_idx].pSubevt_data == NULL){
            my_dump_str_data(DBG_PAwR_ADV_LOGIC, "set PDA subevt data, buffer not initial", 0, 0);
            return HCI_ERR_CMD_DISALLOWED;//TODO: other error code may used
        }
        if(pSubevtCfgInfo->subevt_data_len > 251) {
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
        /* Available PAwR-ADV  advData length max: 253 - 0 - 1(CTE) -2(ADI) - 1(TX_PWR) -9(ACAD_CHMUP) = 240 Byte */
        else if(pSubevtCfgInfo->subevt_data_len > min(cur_pPerdadv->maxLen_subeventData, cur_pPerdadv->subevtAvaAdvDataLen)){
            my_dump_str_data(DBG_PAwR_ADV_LOGIC, "set PDA subevt data, data too long", 0, 0);
            return HCI_ERR_PACKET_TOO_LONG;
        }

        if(blt_ll_advPeriodicChkRspDataItvl(cur_pPerdadv, pSubevtCfgInfo->subevt_data_len, cur_pPerdadv->subevent_interval)){
            return HCI_ERR_PACKET_TOO_LONG;
        }

        if(cur_pPerdadv->pdaSubevtDataCtrl[subevent_idx].subevent_idx != 0xFF) {
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
        if(pSubevtCfgInfo->rsp_slot_start > cur_pPerdadv->num_response_slots) {
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
        cur_pPerdadv->pdaSubevtDataCtrl[subevent_idx].subevent_idx = pSubevtCfgInfo->subevent_idx;
        cur_pPerdadv->pdaSubevtDataCtrl[subevent_idx].rsp_slot_start = pSubevtCfgInfo->rsp_slot_start;
        cur_pPerdadv->pdaSubevtDataCtrl[subevent_idx].rsp_slot_count = pSubevtCfgInfo->rsp_slot_count;
        cur_pPerdadv->pdaSubevtDataCtrl[subevent_idx].subevt_data_len = pSubevtCfgInfo->subevt_data_len;
        smemcpy(cur_pPerdadv->pdaSubevtDataCtrl[subevent_idx].pSubevt_data, pSubevtCfgInfo->pSubevt_data, pSubevtCfgInfo->subevt_data_len);
        //pointer to the next buffer
        p += sizeof(pdaSubevtData_subevtCfg_t) + pSubevtCfgInfo->subevt_data_len;
        pSubevtCfgInfo = (pdaSubevtData_subevtCfg_t*)p;
    }

    return BLE_SUCCESS;
}

ble_sts_t   blc_hci_le_setPeriodicAdvSubeventData(hci_le_setPeridAdvSubeventData_cmdParam_t* pcmdParam, hci_le_setPeridAdvSubeventDataRetParams_t *pRetParams)
{
    pRetParams->status = blc_ll_setPeriodicAdvSubeventData(pcmdParam->adv_handle, pcmdParam->num_subevent, pcmdParam->subevtCfg);
    pRetParams->advHandle = pcmdParam->adv_handle;
    return pRetParams->status;
}

ble_sts_t   blc_ll_extended_createConnection_v2 (adv_handle_t adv_handle, u8 subevent,
                                                init_fp_t  filter_policy, own_addr_type_t ownAdrType, u8 peerAdrType, u8 *peerAddr, init_phy_t init_phys,
                                                scan_inter_t scanInter_0, scan_wind_t scanWindow_0, conn_inter_t conn_min_0, conn_inter_t conn_max_0, conn_tm_t timeout_0,
                                                scan_inter_t scanInter_1, scan_wind_t scanWindow_1, conn_inter_t conn_min_1, conn_inter_t conn_max_1, conn_tm_t timeout_1,
                                                scan_inter_t scanInter_2, scan_wind_t scanWindow_2, conn_inter_t conn_min_2, conn_inter_t conn_max_2, conn_tm_t timeout_2 )
{

    tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_CMD), "[LL][CMD] Ext_Create_Conn_v2", peerAddr, 6);
    //my_dump_str_u8s(BLC_LL_LOG_EN, "@BLC_LL_Ext_Create_Conn", filter_policy, ownAdrType, conn_min_1, conn_max_1);

    /* some code use "hci_cmd_mask" even without HCI, so handle it here */
    /* HCI/GEV/BV-03-C [Disallow Mixing Legacy and Extended Scanning Commands] */
    if(IS_LEGACY_SCAN_VALID){
        return HCI_ERR_CMD_DISALLOWED;
    }
    else{
        SET_EXTENDED_SCAN_VALID;
    }

    if((adv_handle == 0xFF && subevent != 0xFF) || (adv_handle != 0xFF && subevent == 0xFF)){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    st_prd_adv_t *cur_pPerdadv;
    bool pawrInitUsed = FALSE;
    if((adv_handle != 0xFF) && (subevent != 0xFF)){
        /* If the Advertising_Handle and Subevent parameters are set to valid values,
           then the Initiator_Filter_Policy, Initiating_PHYs, Scan_Interval[i], and
           Scan_Window[i] parameters shall be ignored
         */
        cur_pPerdadv = blt_ll_search_existing_perdAdv_index_by_advHandle(adv_handle);
        if(!cur_pPerdadv){
            my_dump_str_data(DBG_PAwR_ADV_LOGIC, "PDA subevt data req, adv handle error", 0, 0);
            return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
        }

        if(cur_pPerdadv->num_subevents == 0 || subevent > cur_pPerdadv->num_subevents){
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }

        pawrInitUsed = TRUE;
        cur_pPerdadv->initSubevent = subevent + 1; //paSubevent offset from 1
        filter_policy = INITIATE_FP_ADV_SPECIFY; //ignore
    }



    /*If the Own_Address_Type parameter is set to 0x01 and the random address for
      the device has not been initialized using the HCI_LE_Set_Random_Address command,
      the Controller shall return the error code Invalid HCI Command Parameters (0x12)
      HCI/CCO/BI-56-C
     */
    if((ownAdrType & 0x01) && !(blmsParam.hci_cmd_mask & SET_RANDOM_ADDR_CMD_MASK))
    {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    /* In API: blt_ll_createConnCommon, changed blmsParam.scanInitEn_union.leg_init_en OR
     * blmsParam.scanInitEn_union.ext_init_en, here recovery to the original value */
    u32 r;
    u8 leg_init_en, ext_init_en;
    if(pawrInitUsed){
        r = irq_disable();
        leg_init_en = blmsParam.scanInitEn_union.leg_init_en;
        ext_init_en = blmsParam.scanInitEn_union.ext_init_en;
        irq_restore(r);
    }
    else{
        (void)r;
        (void)leg_init_en;
        (void)ext_init_en;
    }

    u8 ret_status = (u8)blt_ll_createConnCommon(filter_policy, ownAdrType, peerAdrType, peerAddr);
    if(ret_status != BLE_SUCCESS){
        return ret_status;
    }
    /* When code running here, initiation will start */
    if(pawrInitUsed){
        pkt_init.interval = cur_pPerdadv->subevent_interval;
        pkt_init.timeout = CONN_TIMEOUT_4S;
        bltInit.sec_chn_init = 1;
        blmsParam.create_connection = CONNECT_REQ_FOR_PAWR;

        /* In API: blt_ll_createConnCommon, changed blmsParam.scanInitEn_union.leg_init_en OR
         * blmsParam.scanInitEn_union.ext_init_en, here recovery to the original value */
        r = irq_disable();
        blmsParam.scanInitEn_union.leg_init_en = leg_init_en;
        blmsParam.scanInitEn_union.ext_init_en = ext_init_en;
        irq_restore(r);

        return BLE_SUCCESS;
    }




    /* scan_interval and scan_window for different PHYs should be hold in group, decide to use which value according to current scanning PHY */
    /* conn_interval and conn_time for different PHYs should be hold in group, decide to use which value according to current scanning PHY */
    //remember to set pkt_init.interval when send "AUX_CONNECT_REQ"
    //remember to set   pkt_init.timeout when send "AUX_CONNECT_REQ"
    //remember set new master_interval with "blc_ll_setAclCentralBaseConnectionInterval" if this need change(blmsParam.max_master_num is 1)
    bltScn.scanPhy_msk = init_phys;

    u8 first_set_idx = 0xFF;
    u8  set_flag_idx[3] = {0};

    if(init_phys & INIT_PHY_1M){
        if(first_set_idx == 0xFF){
            first_set_idx = 0;
        }
        set_flag_idx[0] = 1;

        if((int)scanWindow_0 > (int)scanInter_0){
            scanWindow_0 = (scan_wind_t)scanInter_0;
        }
        bltScn.scanPercent[0] = (scanWindow_0<<7)/scanInter_0;  // scan_window*128
        bltScn.scanInter[0] = scanInter_0;
        bltScn.scanInte_tick[0] = scanInter_0 * SYSTEM_TIMER_TICK_625US - 1000*SYSTEM_TIMER_TICK_1US;


        bltExtInit.mas_hold_intv_mul[0] = blt_init_calculateMasterIntervalMultiplier(aclMas_param.master_connInter, conn_min_0, conn_max_0);
        if(bltExtInit.mas_hold_intv_mul[0] == 24){
            bltExtInit.mas_hold_intv_msk[0] = INTV_MSK_24_TIME;
        }
        else{
            bltExtInit.mas_hold_intv_msk[0] = interMask_tbl[bltExtInit.mas_hold_intv_mul[0]];
        }



        //my_dump_str_u8s(0, "AAAAAAAA", aclMas_param.master_connInter, conn_min_0, conn_max_0, bltExtInit.mas_hold_intv_mul[0]);

        #if (IMPROVE_MASTER_INTERVAL)
            u16 conn_inter_use = aclMas_param.master_connInter*bltExtInit.mas_hold_intv_mul[0];
            if(conn_min_0 <= conn_inter_use && conn_max_0 >= conn_inter_use){ //totally meet host's requirement
                //do nothing
            }
            else{ //not exactly host's requirement
                if(conn_min_0 > aclMas_param.master_connInter){
                    int mod = conn_max_0 % aclMas_param.master_connInter;
                    u16 conn_inter = conn_max_0 - mod;
                    if(conn_inter >= conn_min_0){
                        bltExtInit.mas_hold_intv_mul[0] = conn_inter/aclMas_param.master_connInter;
                        bltExtInit.mas_hold_intv_msk[0] = 0xFFFFFF;

                        //my_dump_str_u8s(0, "BBBBBBBBB", aclMas_param.master_connInter, conn_min_0, conn_max_0, bltExtInit.mas_hold_intv_mul[0]);
                    }
                }
            }
        #endif


        bltExtInit.hold_timeout[0] = timeout_0;  //hold timeout


        /* extended create connection for a legacy ADV:
         * use extended create connection command, but the device is sending ADV on primary channel */
        bltInit.mas_intv_mul = bltExtInit.mas_hold_intv_mul[0];
        bltInit.mas_intv_msk = bltExtInit.mas_hold_intv_msk[0];
        pkt_init.interval = aclMas_param.master_connInter*bltInit.mas_intv_mul;
        pkt_init.timeout = timeout_0;
        /* extended create connection for a legacy ADV */

        //my_dump_str_u8s(0, "AAAAAAAAA", aclMas_param.master_connInter, conn_min_0, conn_max_0, bltExtInit.mas_hold_intv_mul[0]);
        //my_dump_str_u32s(0, "BBBBBBBBB", bltExtInit.mas_hold_intv_mul[0], bltExtInit.mas_hold_intv_msk[0], 0, 0);
    }
    if(init_phys & INIT_PHY_2M){
        if(first_set_idx == 0xFF){
            first_set_idx = 1;
        }
        set_flag_idx[1] = 1;

        if((int)scanWindow_1 > (int)scanInter_1){
            scanWindow_1 = (scan_wind_t)scanInter_1;
        }
        bltScn.scanPercent[1] = (scanWindow_1<<7)/scanInter_1;  // scan_window*128
        bltScn.scanInter[1] = scanInter_1;
        bltScn.scanInte_tick[1] = scanInter_1 * SYSTEM_TIMER_TICK_625US - 1000*SYSTEM_TIMER_TICK_1US;



        bltExtInit.mas_hold_intv_mul[1] = blt_init_calculateMasterIntervalMultiplier(aclMas_param.master_connInter, conn_min_1, conn_max_1);
        if(bltExtInit.mas_hold_intv_mul[1] == 24){
            bltExtInit.mas_hold_intv_msk[1] = INTV_MSK_24_TIME;
        }
        else{
            bltExtInit.mas_hold_intv_msk[1] = interMask_tbl[bltExtInit.mas_hold_intv_mul[1]];
        }


        #if (IMPROVE_MASTER_INTERVAL)
            u16 conn_inter_use = aclMas_param.master_connInter*bltExtInit.mas_hold_intv_mul[1];
            if(conn_min_1 <= conn_inter_use && conn_max_1 >= conn_inter_use){ //totally meet host's requirement
                //do nothing
            }
            else{ //not exactly host's requirement
                if(conn_min_1 > aclMas_param.master_connInter){
                    int mod = conn_max_1 % aclMas_param.master_connInter;
                    u16 conn_inter = conn_max_1 - mod;
                    if(conn_inter >= conn_min_1){
                        bltExtInit.mas_hold_intv_mul[1] = conn_inter/aclMas_param.master_connInter;
                        bltExtInit.mas_hold_intv_msk[1] = 0xFFFFFF;
                    }
                }
            }
        #endif


        bltExtInit.hold_timeout[1] = timeout_1;  //hold timeout
    }
    if(init_phys & INIT_PHY_CODED){
        if(first_set_idx == 0xFF){
            first_set_idx = 2;
        }
        set_flag_idx[2] = 1;

        if((int)scanWindow_2 > (int)scanInter_2){
            scanWindow_2 = (scan_wind_t)scanInter_2;
        }
        bltScn.scanPercent[2] = (scanWindow_2<<7)/scanInter_2;  // scan_window*128
        bltScn.scanInter[2] = scanInter_2;
        bltScn.scanInte_tick[2] = scanInter_2 * SYSTEM_TIMER_TICK_625US - 1000*SYSTEM_TIMER_TICK_1US;



        bltExtInit.mas_hold_intv_mul[2] = blt_init_calculateMasterIntervalMultiplier(aclMas_param.master_connInter, conn_min_2, conn_max_2);
        if(bltExtInit.mas_hold_intv_mul[2] == 24){
            bltExtInit.mas_hold_intv_msk[2] = INTV_MSK_24_TIME;
        }
        else{
            bltExtInit.mas_hold_intv_msk[2] = interMask_tbl[bltExtInit.mas_hold_intv_mul[2]];
        }


        #if (IMPROVE_MASTER_INTERVAL)
            u16 conn_inter_use = aclMas_param.master_connInter*bltExtInit.mas_hold_intv_mul[2];
            if(conn_min_2 <= conn_inter_use && conn_max_2 >= conn_inter_use){ //totally meet host's requirement
                //do nothing
            }
            else{ //not exactly host's requirement
                if(conn_min_2 > aclMas_param.master_connInter){
                    int mod = conn_max_2 % aclMas_param.master_connInter;
                    u16 conn_inter = conn_max_2 - mod;
                    if(conn_inter >= conn_min_2){
                        bltExtInit.mas_hold_intv_mul[2] = conn_inter/aclMas_param.master_connInter;
                        bltExtInit.mas_hold_intv_msk[2] = 0xFFFFFF;
                    }
                }
            }
        #endif

        bltExtInit.hold_timeout[2] = timeout_2;  //hold timeout
    }




    /*
    Where the connection is made on a PHY whose bit is not set in the Initiating_-PHYs parameter, the Controller shall use the Connection_Interval_Min[i],
    Connection_Interval_Max[i], Max_Latency[i], Supervision_Timeout[i],
    Min_CE_Length[i], and Max_CE_Length[i] parameters for an implementation-chosen PHY whose bit is set in the Initiating_PHYs parameter.
    */
    if(first_set_idx == 0xFF){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    for(int i=0;i<3;i++){
        if(set_flag_idx[i] == 0){
            bltScn.scanPercent[i] = bltScn.scanPercent[first_set_idx];
            bltScn.scanInter[i] = bltScn.scanInter[first_set_idx];
            bltScn.scanInte_tick[i] = bltScn.scanInte_tick[first_set_idx];
            bltExtInit.mas_hold_intv_mul[i] = bltExtInit.mas_hold_intv_mul[first_set_idx];
            bltExtInit.mas_hold_intv_msk[i] = bltExtInit.mas_hold_intv_msk[first_set_idx];
            bltExtInit.hold_timeout[i] = bltExtInit.hold_timeout[first_set_idx];
        }
    }




    if(filter_policy == INITIATE_FP_ADV_SPECIFY){
        //TODO: check if current device is in scan ADV device table, to see event_type, decide to send conn_req or aux_conn_req
    }

    r = irq_disable();  //very important to disable IRQ

    if(blmsParam.scanInitEn_union.ext_scan_en){
        blmsParam.create_connection = CONNECT_REQ_EXT_PENDING;
    }
    else{
        /* if scan not enabled, */
        blmsParam.state_chng |= STATE_CHANGE_INIT;
        blmsParam.create_connection = CONNECT_REQ_GOING;
        bltScn.initiate_going = EXT_INITIATE_GOING;
    }

    irq_restore(r);


    return BLE_SUCCESS;
}



ble_sts_t   blc_hci_le_extended_createConnection_v2( hci_le_ext_createConnV2_cmdParam_t * pCmdParam)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Extended_Creare_Connection_v2", pCmdParam, sizeof(hci_le_ext_createConn_cmdParam_t));

    return blc_ll_extended_createConnection_v2 (pCmdParam->adv_handle, pCmdParam->subevent, pCmdParam->fp, pCmdParam->ownAddr_type, pCmdParam->peerAddr_type,
                                                pCmdParam->peer_addr, pCmdParam->init_PHYs,
                                                pCmdParam->initCfg[0].scan_inter, pCmdParam->initCfg[0].scan_wind, pCmdParam->initCfg[0].conn_min, pCmdParam->initCfg[0].conn_max, pCmdParam->initCfg[0].timeout,
                                                pCmdParam->initCfg[1].scan_inter, pCmdParam->initCfg[1].scan_wind, pCmdParam->initCfg[1].conn_min, pCmdParam->initCfg[1].conn_max, pCmdParam->initCfg[1].timeout,
                                                pCmdParam->initCfg[2].scan_inter, pCmdParam->initCfg[2].scan_wind, pCmdParam->initCfg[2].conn_min, pCmdParam->initCfg[2].conn_max, pCmdParam->initCfg[2].timeout);
}

#endif



