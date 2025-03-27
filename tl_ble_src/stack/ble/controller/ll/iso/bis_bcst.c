/********************************************************************************************************
 * @file    bis_bcst.c
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




#if (LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER)


#ifndef     OPTIMIZE_BIS_TIME
#define     OPTIMIZE_BIS_TIME           1
#endif


_attribute_ble_data_retention_  rf_packet_ll_data_t*    pCurrBisPdu;
bis_tx_pdu_fifo_t       bltBisPduTxfifo;


_attribute_ble_data_retention_
rf_packet_ll_data_t    gBisEmptyPdu = {
    .dma_len = rf_tx_packet_dma_len(2),
    .llPhysChnPdu.llPduHdr.bisPduHdr.llid = 0,
    .llPhysChnPdu.llPduHdr.bisPduHdr.cssn = 0,
    .llPhysChnPdu.llPduHdr.bisPduHdr.cstf = 0,
    .llPhysChnPdu.llPduHdr.bisPduHdr.rf_len = 0,
};

_attribute_ble_data_retention_  u8  gBisTermIndPdu[10+4] = {
    U32_BYTE0(rf_tx_packet_dma_len(6)), U32_BYTE1(rf_tx_packet_dma_len(6)), U32_BYTE2(rf_tx_packet_dma_len(6)), U32_BYTE3(rf_tx_packet_dma_len(6)), //DMA length
    3, 4,                   //Header
    BIG_TERMINATE_IND,      //OpCode
    0,                      //Reason
    0, 0,                   //Instant
    0, 0, 0, 0              //MIC
};

_attribute_ble_data_retention_  u8  gBisChmIndPdu[14+4] = {
    U32_BYTE0(rf_tx_packet_dma_len(10)), U32_BYTE1(rf_tx_packet_dma_len(10)), U32_BYTE2(rf_tx_packet_dma_len(10)), U32_BYTE3(rf_tx_packet_dma_len(10)), //DMA length
    3, 8,                   //Header: LLID(2 bits)  CSSN(3 bits)    CSTF(1 bit) RFU(2 bits) Length(8 bits)
    BIG_CHANNEL_MAP_IND,    //OpCode
    0, 0, 0, 0, 0,          //ChM
    0, 0,                   //Instant
    0, 0, 0, 0              //MIC
};


_attribute_ble_data_retention_  ll_big_bcst_t       *global_pBigBcst = NULL;  //global BIG BCST parameter pointer

_attribute_ble_data_retention_  ll_big_bcst_t       *latest_pBigBcst = NULL;  //last used BIG BCST parameter pointer

_attribute_ble_data_retention_  ll_big_bcst_t       *blt_pBigBcst = NULL;

_attribute_ble_data_retention_  int blt_bigBcst_sel = 0;

_attribute_ble_data_retention_  static u32 SubEvtStepTick = 0;

_attribute_ble_data_retention_  u32 bctx_start_tick;

_attribute_ble_data_retention_ blt_ll_pushIsoDataFun  blt_ll_pushBisDataFun =NULL;
ble_sts_t blt_ll_pushBisData(u16 connHandle, iso_pb_flag_t PB_Flag, u8 TS_Flag, u32 time_stamp, u16 seqnum, u16 total_len, u16 cur_len, u8 *pData);


/*
 * @brief      This function is used to initialize broadcast sdu in fifo buffer.
 *             sdu in indicate the data if from host to controller.
 * @param[in]  in_fifo      - the start address of broadcast sdu in fifo buffer.
 * @param[in]  in_fifo_size - the fifo size.
 * @param[in]  in_fifo_num  - the fifo number.
 */
ble_sts_t blc_ll_initBisBcstSduInBuffer(u8 *in_fifo,u16 in_fifo_size, u8 in_fifo_num)
{
    if( IS_POWER_OF_2(in_fifo_num)){
        sduBisMng.in_fifo_num = in_fifo_num;
        sduBisMng.in_fifo_mask = in_fifo_num - 1 ;
    }
    else{
        return LL_ERR_INVALID_PARAMETER;
    }


    /* size must be 4*n */
    if((in_fifo_size & 3) == 0){
        sduBisMng.max_in_fifo_size = in_fifo_size;
    }
    else{
        return LL_ERR_INVALID_PARAMETER;
    }

    sduBisMng.in_fifo_b =  in_fifo;
    blt_ll_pushBisDataFun = blt_ll_pushBisData;

    return BLE_SUCCESS;
}


int blc_ll_getBisSduInBufferFreeNum(u16 bisHandle){

    ll_bis_t *pBis = (ll_bis_t *) (global_pBis + (bisHandle & BLT_BIS_IDX_MSK));
    return pBis->bisSduInFreeNum;
}

static int  blt_ll_perdAdvAcadUpdateBigInfo(int prdadv_idx);


_attribute_ram_code_
int         blt_ll_buildBigBcstSchedulerLinklist(void)
{
    int intvl_jump_big;
    s32 sSlot_start_big;
    ll_big_bcst_t *cur_pBigPara = NULL;

    /*The absolute value on the time axis corresponding to Task->begin:
    sSlot_tick_start + Task->begin*SSLOT_TICK_NUM, sSlot_idx_base is the relative value */
//  u32 sSlot_idx_base = (bltSche.bSlot_idx_next - bltSche.bSlot_idx_start)*32;

    for(int i=0; i<bltBisMng.maxNum_bigBcst; i++)
    {
        if( bltSche.task_mask & (TSKMSK_BIG_BCST_0<<i ))
        {
            cur_pBigPara = (ll_big_bcst_t *)(global_pBigBcst + i);
            cur_pBigPara->schTsk_wptr = cur_pBigPara->schTsk_rptr = 0;

            if(bltSche.build_index == 0 && bltSche.sSlot_idx_reset == 1){
                cur_pBigPara->sSlot_mark_big -= bltSche.sSlot_idx_past;
            }

            if( cur_pBigPara->sSlot_mark_big >= bltSche.sSlot_idx_next){
                sSlot_start_big = cur_pBigPara->sSlot_mark_big + cur_pBigPara->sSlot_interval_big;
                intvl_jump_big = 0;
            }
            else{
                intvl_jump_big = (bltSche.sSlot_idx_next - 1 - cur_pBigPara->sSlot_mark_big) / cur_pBigPara->sSlot_interval_big;
                sSlot_start_big = cur_pBigPara->sSlot_mark_big + (intvl_jump_big + 1) * cur_pBigPara->sSlot_interval_big;

                blt_ll_incSchedulerTaskCalPriority( TSKOFT_BIG_BCST + i, bltPri.step_final[TSKOFT_BIG_BCST + i]*4*intvl_jump_big );
            }

            if(sSlot_start_big >= bltSche.sSlot_endIdx_dft){ //to save some time for big interval
                continue; //attention: can not use break !!!
            }


            int new_task_cnt = 0;
            for(int j=0;j<BIG_FIFONUM;j++){

                sch_task_t  *pCur_schTask = (sch_task_t *)&cur_pBigPara->bigb_schTsk_fifo[j];


//              pCur_schTask->begin = sSlot_idx_base + (((bSlot_start_big + j*cur_pBigPara->bSlot_interval_big) - bltSche.bSlot_idx_next )<<5);
                pCur_schTask->begin = sSlot_start_big + j*cur_pBigPara->sSlot_interval_big;
                pCur_schTask->end = pCur_schTask->begin + cur_pBigPara->sSlot_duration_big - 1;


                if( pCur_schTask->begin >=  bltSche.sSlot_endIdx_dft){  //new task beyond correct range, finish
                    break;
                }
                else if(pCur_schTask->end < bltSche.sSlot_endIdx_dft){ //new task in correct range
                    cur_pBigPara->schTsk_wptr = j;
                    new_task_cnt ++;
                    blt_ll_incSchedulerTaskCalPriority( TSKOFT_BIG_BCST + i, -bltPri.step_final[TSKOFT_BIG_BCST + i]);
                }
                else{ //new task across "sSlot_endIdx_dft"

                    //for those task across end_idx, find the task with highest priority, to guarantee that task not missed
                    if(bltPri.pri_cal[TSKOFT_BIG_BCST + i] > bltPri.priMax_value){
                        bltPri.priMax_value = bltPri.pri_cal[TSKOFT_BIG_BCST + i];
                        bltPri.priMax_index = TSKOFT_BIG_BCST + i;
                        bltSche.sSlot_endIdx_maxPri = pCur_schTask->begin;
                        tlkapi_send_string_u32s(SCHE_TIMING_IMPROVE_DBG_EN,"across IDX bigBcst", i, bltSche.sSlot_endIdx_dft, bltSche.sSlot_endIdx_maxPri, bltPri.priMax_value);
                    }
                    break;
                }
            }

            if(new_task_cnt){
                blt_ll_addTask2ExistLinklist( &cur_pBigPara->bigb_schTsk_fifo[0], cur_pBigPara->schTsk_wptr + 1);
            }
        }
    }

    return 0;
}


#if (SUB_INTERVAL_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
static int  blt_ll_triggerBigSlotTask(void)
{
    if( blmsParam.big_sche_build_pending & BIG_SLOT_BUILD_MSK ){
        u8 big_idx = (blmsParam.big_sche_build_pending & SLOT_BUILD_IDX_MSK);
        ll_big_bcst_t *pBigPara = (ll_big_bcst_t *)(global_pBigBcst + big_idx);  //Current used BIG parameter pointer
        st_prd_adv_t *cur_pPerdadv = blt_ll_search_existing_perdAdv_index_by_advHandle(pBigPara->adv_handle);
        if(cur_pPerdadv == NULL){
            //tlkapi_send_string_data(DEB_BIG_BCST_EN,"impossible here!!!", &big_idx, 1);
            return 0;
        }

        if(cur_pPerdadv->pda_tx.prdadv_send_cnt >= 1){

            u32 pdaAdvNextBslotMask = cur_pPerdadv->pda_tx.bSlot_mark_prdadv + cur_pPerdadv->pda_tx.bSlot_prdadv_itvl;

            //need to place before "pBigPara->sSlot_mark_big = (cur_pPerdadv->pda_tx.bSlot_mark_prdadv +"
            //cause now add the bigInfor section and the sSlot_duration_pda need to be updated.
            //sSlot_duration_pda will be used to calculate the big mark --- sSlot_mark_big
            cur_pPerdadv->acad_chaged = 1;
            blt_prdadv_updateAcadPram(cur_pPerdadv, PERD_ACAD_BIGINFO_ENA);
            cur_pPerdadv->acad_chaged = 2;

            pBigPara->sSlot_mark_big = BSLOT_ABS_2_SSLOT_ABS(pdaAdvNextBslotMask) + cur_pPerdadv->pda_tx.sSlot_duration_pda + 32; //margin: 1 bSlot


            //mark indicate the previous event anchor point.now when the first, sSlot_mark_big indicate the real first anchor.
            //here process is different from other module. so need to notice.
            //if(pBigPara->sSlot_mark_big > pBigPara->sSlot_interval_big){
            //  pBigPara->sSlot_mark_big -= pBigPara->sSlot_interval_big; //1st BIG slot timing
            //}

            #if (ONLY_FOR_EBQ_TEST_LATER_REMOVE)
                bigExtAuxPda_conflictCtrl.acadInfor_changeCtrl = ACAD_VALID_PENDING;
                bigExtAuxPda_conflictCtrl.bigTask_timingStart = 1;
                bigExtAuxPda_conflictCtrl.auxAdv_sendNum = 0;
                bigExtAuxPda_conflictCtrl.pdaAdv_sendNum = 0;
            #endif

            blmsParam.big_sche_build_pending = 0;  //clear, must after getting "big_idx"
            pBigPara->big_terminated = 0; //clear

            u32 r = irq_disable();
            blt_sche_addTaskMask(pm_check_info ? TSKMSK_BIG_BCST_0 << big_idx : 0);
            blt_sche_addUpdate(SLOT_UPDT_BIS_BCST_CREATE);
            irq_restore(r);
        }
    }

    return 1;
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
int blt_ial_bisBroadcast_tx_process(ll_bis_t *pBis){

    /******************************* Bis broadcast ******************************************************************/
        sdu_packet_t *sdu;
        u8 numOfcmpPkt = 0;


        ll_big_bcst_t* pBig = (ll_big_bcst_t *) (global_pBigBcst + pBis->big_idx);

        if(((!pBig->framing) || ((pBig->framing)&& pBig->big_start_tick &&clock_time_exceed(pBig->big_start_tick, (pBig->iso_itvl*1250 - SPILT_SDU2PDU_PRE_PROCESS_US))))){


            while(pBis->bisSduIn_rptr != pBis->bisSduIn_wptr)
//              if(pBis->bisSduIn_rptr != pBis->bisSduIn_wptr)?
            {
                sdu = (sdu_packet_t*)(pBis->bis_sduInBuf + sduBisMng.max_in_fifo_size * (pBis->bisSduIn_rptr & sduBisMng.in_fifo_mask));

                if(pBig->framing == 0)
                {
                    if(blt_bis_splitSdu2UnframedPdu(pBis->bis_handle, sdu, &numOfcmpPkt)==BLE_SUCCESS){
                        pBis->bisSduIn_rptr++;
                        pBis->bisSduInFreeNum++;
                    }
                }
                else
                {
                    blt_bis_splitSdu2FramedPdu(pBis->bis_handle, &numOfcmpPkt);
                }

                if(numOfcmpPkt)
                {
                    #if (HCI_SEND_NUM_OF_CMP_AFT_ACK)
                        u32 r = irq_disable();

                        pBis->nocAclTxWptr[pBis->nocAckWptr & pBis->nocAckMsk] = pBis->bisPduTxFifoWptr;
                        pBis->numOfCmpCnt[pBis->nocAckWptr & pBis->nocAckMsk] = numOfcmpPkt;

                        tlkapi_send_string_u32s(DBG_NUM_COM_PKT, "numComPkt, mark", pBis->nocAckWptr, pBis->bisPduTxFifoWptr, numOfcmpPkt, 0);

                        pBis->nocAckWptr++;

                        irq_restore(r);
                    #else
                        hci_numOfCmpPktEvt_t CmpPktEvt;
                        CmpPktEvt.numHandles = 1;
                        CmpPktEvt.retParams->connHandle = pBis->bis_handle;
                        CmpPktEvt.retParams->numOfCmpPkts = numOfcmpPkt;
                        if(blc_hci_send_event (HCI_FLAG_EVENT_BT_STD | HCI_EVT_NUM_OF_COMPLETE_PACKETS, (u8*)&CmpPktEvt, 1 + 1*sizeof(numCmpPktParamRet_t)) != 0){
                            //BLMS_ERR_DEBUG(DBG_HCI_FIFO, 0xCC110623);
                        }
                    #endif

                    numOfcmpPkt = 0;
                }
            }
        }


    return 0;
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
int         blt_ll_bigBcstMainloop(void)
{

    /*
     *  Prepare to trigger BIG slot task (Calculate the first BIG anchor point).
     */
    blt_ll_triggerBigSlotTask();

    for(int i=0; i<bltBisMng.curNum_bigBcst; i++)
    {
        u8 evt_status = BLE_SUCCESS;
        ll_big_bcst_t *big = (ll_big_bcst_t *)(global_pBigBcst + i);
        //Clear BIS Bcst concerned global variables
        ll_bis_t *cur_pBis = NULL;

        if(big->big_create_cmp_evt)
        {
            evt_status = big->big_create_cmp_evt == BIG_CREATE_COMPLETE ? BLE_SUCCESS : HCI_ERR_OP_CANCELLED_BY_HOST;
            big->big_create_cmp_evt = 0;

            if((hci_le_eventMask&HCI_LE_EVT_MASK_CREATE_BIG_COMPLETE))
            {
                hci_le_createBigComplete_evt(evt_status, big->big_handle, (u8*)&big->big_sync_delay_us,(u8*)&big->transLatency_us, big->curBisPhy,
                                             big->nse, big->bn, big->pto, big->irc, big->max_pdu, big->iso_itvl, big->bis_cnt, big->bis_handle);
            }
        }
        else if(big->big_term_cmp_evt)
        {
            if((hci_le_eventMask&HCI_LE_EVT_MASK_TERMINATE_BIG_COMPLETE))
            {
                hci_le_terminateBigComplete_evt(big->big_handle, HCI_ERR_CONN_TERM_BY_LOCAL_HOST);
            }

            big->big_term_cmp_evt = 0;
            evt_status = HCI_ERR_CONN_TERM_BY_LOCAL_HOST;
        }


        /* BIG create operation was Cancelled by Host */
        if(evt_status){
            ////////// Destroy BIS and BIG control block ///////////



            for(int j = 0; j<big->bis_cnt; j++){
                cur_pBis = global_pBis + (big->bis_handle[j] & BLT_BIS_IDX_MSK);

                cur_pBis->bis_occupied = 0;
                bltBisMng.curNum_bisBcst --;  //update current BIS number
                cur_pBis->bisSuccessiveMiss = 0; //QW
                cur_pBis->link_big_handle = BIG_HANDLE_INVALID;
                cur_pBis->big_idx = 0;
                cur_pBis->bis_dapth_setup = 0;
                cur_pBis->dpID = 0xff;
                cur_pBis->bisPduTxFifoRptr = cur_pBis->bisPduTxFifoWptr = 0;
                cur_pBis->bisSduIn_rptr = cur_pBis->bisSduIn_wptr = 0;
                cur_pBis->bisSduInFreeNum = sduBisMng.in_fifo_num;
            }


            //Clear BIG Bcst concerned global variables
            bltBisMng.curNum_bigBcst--;
            big->adv_handle = INVALID_ADVHD_FLAG;
            big->big_handle = BIG_HANDLE_INVALID;
            big->cmd_status = BIG_IN_IDLE;
            big->big_terminated = 0; //clear
            big->big_sc_mask = 0;
            big->big_term_inst = 0; // clear
            big->big_sc_send_cnt = 0;
            big->big_sc_cssn = 0;
            big->big_term_cmp_evt = 0;
            big->big_create_cmp_evt = 0;
            big->bis_cnt = 0;
        }
        else
        {
            if(big->cmd_status == BIG_CREATE_COMPLETE)
            {
                for(int j = 0; j<big->bis_cnt; j++)
                {
                    cur_pBis = blt_ll_findBisByHandle(big->bis_handle[j]);


                    if(cur_pBis!=NULL)
                    {
                        iso_test_param_t *isoTest = cur_pBis->pBisTestParam;

                        if((isoTest!=NULL)&&(isoTest->isoTestMode == ISO_TEST_TRANSMIT_MODE))
                        {

                            sdu_packet_t *sdu = (sdu_packet_t*)(cur_pBis->bis_sduInBuf + sduBisMng.max_in_fifo_size * (cur_pBis->bisSduIn_wptr & sduBisMng.in_fifo_mask));

                            ll_big_bcst_t *pBig =(ll_big_bcst_t *) (global_pBigBcst + cur_pBis->big_idx);

                            if(!blt_iso_test_transmit_mainloop(isoTest, pBig->sdu_intvl,pBig->max_sdu, sdu, pBig->framing)){
                                sdu->numOfCmplt_en = 0;
                                cur_pBis->bisSduIn_wptr++;
                            }
                        }

                        if(cur_pBis->bisSduIn_rptr != cur_pBis->bisSduIn_wptr)
                        {
                            blt_ial_bisBroadcast_tx_process(cur_pBis);
                        }
                    }
                }
            }
        }
    }

    return 1;
}

#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
int         blt_ll_ctrlBigBcstChClassUpd(unsigned char *pChm)
{
    for(int i=0; i<bltBisMng.curNum_bigBcst; i++)
    {
        ll_big_bcst_t *pBig = (ll_big_bcst_t *)(global_pBigBcst + i);

        if(pBig->cmd_status == BIG_CREATE_COMPLETE){

            smemcpy ( pBig->nextChnParam.chmTbl, pChm, 5);

            csa2_calculateMapInfo(&pBig->nextChnParam);

            /* It will be cleared after other BIG Control procedures are executed */
            if(pBig->big_sc_mask == 0){
                pBig->big_sc_send_cnt = 0;
                tlkapi_send_string_data(DEB_BIG_BCST_EN,"big_sc_send_cnt: 0", 0, 0);
            }

            //How to ensure 6 consecutive pens, you need to consider the priority of BIG task:
            pBig->big_chm_inst = (u16)(pBig->bigEventCnt + 12); //in future at least 6

            pBig->big_sc_mask |= BIG_SC_CHM_IND;

            u32 r = irq_disable();

            //Rebuild sch task table ASAP.
            blt_sche_addUpdate(SLOT_UPDT_SLOTTBL_RESCHED);
            //big channel map update control PDU need 6 consecutive,not jump.
            blt_ll_setSchedulerTaskPriority(TSKOFT_BIG_BCST + i, TASK_PRIORITY_MAX);

            irq_restore(r);

            tlkapi_send_string_data(DEB_BIG_BCST_EN,"LE BIG_CHANNEL_MAP_IND", 0, 0);

        }
    }

    return 1;
}

/**
 * @brief      for user to initialize BIG broadcast module and allocate BIG broadcast parameters buffer.
 * @param[in]  pBigBcstPara - start address of BIG broadcast parameters buffer
 * @param[in]  bigBcstNum - BIG broadcast number application layer may use
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_initBigBcstModule_initBigBcstParametersBuffer(u8 *pBigBcstPara, u8 bigBcstNum)
{

    STATIC_ASSERT_FILE(BIG_BCST_PARAM_LENGTH == sizeof(ll_big_bcst_t), bis_bcst);

    #if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
        STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_bis_t)),     bis_bcst);
        STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_big_bcst_t)), bis_bcst);
    #endif

    //Refer to Core5.2 Page3013, Table 4.6: FeatureSet field's bit mapping to Controller features, Bit30
    LL_FEATURE_MASK_0 |= (LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER <<30);

    //BIS need this feature bit enable. for standard controller, bit will be cleared when received HCI_RESET command
    LL_FEATURE_MASK_1 |= LL_FEATURE_MASK_ISOCHRONOUS_CHANNELS;

    /* Special protection code for use */
    if(pm_check_info){
        ll_big_bcst_irq_task_cb     = blt_big_bcst_interrupt_task;
        ll_big_bcst_mlp_task_cb     = blt_big_bcst_mainloop_task;
        perd_adv_biginfo_update_cb  = blt_ll_perdAdvAcadUpdateBigInfo;
    }

    blmsParam.big_bcst_en = 1;

    /////////////////////////////////////////////////////
    if(bigBcstNum > LL_BIG_BCST_NUM_MAX)
    {
        bigBcstNum = LL_BIG_BCST_NUM_MAX;
    }

    global_pBigBcst = (ll_big_bcst_t*)pBigBcstPara;
    bltBisMng.maxNum_bigBcst = bigBcstNum;
    bltBisMng.curNum_bigBcst = 0;


    ll_big_bcst_t *pBigBcst = NULL;
    for(u8 i=0; i<bltBisMng.maxNum_bigBcst; i++)
    {
        pBigBcst = global_pBigBcst + i;

        pBigBcst->adv_handle = INVALID_ADVHD_FLAG;
        pBigBcst->big_handle = BIG_HANDLE_INVALID;
        pBigBcst->cmd_status = BIG_IN_IDLE;


        for(int j=0; j<BIG_FIFONUM; j++){
            pBigBcst->bigb_schTsk_fifo[j].scheTask_oft = TSKOFT_BIG_BCST + i;
            pBigBcst->bigb_schTsk_fifo[j].scheTask_idx = i;
            pBigBcst->bigb_schTsk_fifo[j].scheTask_flg = TSKFLG_BIG_BCST;
        }
    }

    return BLE_SUCCESS;

}



/**
 * @brief      for user to initialize BIS ISO TX FIFO.
 * @param[in]  pRxbuf - TX FIFO buffer address.
 * @param[in]  fifo_size - RX FIFO size, must use BIS_PDU_ALIGN4_TXBUFF to calculate.
 * @param[in]  fifo_number - RX FIFO number, must be: 2^n, (power of 2),recommended value: 2, 4, 8, 16, 32, 64
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t blc_ll_initBisTxFifo(u8 *pTxbuf, int fifo_size, int fifo_number)
{
    bltempParam.ll_bisTxFifo_set = 1;
    bltBisPduTxfifo.bis_tx_pdu = NULL;

    /* number must be 2^n */
    if( IS_POWER_OF_2(fifo_number)){
        bltBisPduTxfifo.fifo_num = fifo_number;
        bltBisPduTxfifo.mask = fifo_number - 1;
    }
    else{
        return LL_ERR_INVALID_PARAMETER;
    }

    //remove the header offset
    u32 bis_fifo_size = fifo_size - OFFSETOF(bis_tx_pdu_t, isoTxPdu);

    /* size must be 4*n */
    if( (bis_fifo_size & 3) == 0 || (fifo_size & 3) == 0){
        bltBisPduTxfifo.fifo_size = bis_fifo_size;
        bltBisPduTxfifo.full_size = fifo_size;
    }
    else{
        return LL_ERR_INVALID_PARAMETER;
    }

    bltBisPduTxfifo.txPduLenMax = bltBisPduTxfifo.fifo_size - TLK_RF_TX_EXT_LEN;
    bltBisPduTxfifo.bis_tx_pdu = (bis_tx_pdu_t*)pTxbuf;

    return BLE_SUCCESS;
}



_attribute_ram_code_
int         blt_big_bcst_interrupt_task (int flag, void*p)
{
    int big_idx = flag & FLAG_SCHEDULE_TASK_IDX_MASK;

    if(flag & FLAG_SCHEDULE_BIGBCST_START){
        blt_bigBcst_start(big_idx);
    }
    else if(flag & FLAG_SCHEDULE_BISBCST_START){
        blt_bisBcst_tx_start();
    }
    else if(flag & FLAG_SCHEDULE_BISBCST_POST){
        blt_bisBcst_tx_post();
    }
    else if(flag & FLAG_SCHEDULE_BIGBCST_BUILD){
        blt_ll_buildBigBcstSchedulerLinklist();
    }
    else if(flag & FLAG_INSERT_SCHTSK_CONFLICT){
        sch_task_t *pTgtTsk = (sch_task_t *)p;
        u8 tgtTskFlg = pTgtTsk->scheTask_flg & TSKFLG_VALID_MASK;
        u8 curSchTaskOft = TSKOFT_BIG_BCST + big_idx;
        (void)tgtTskFlg;
        #if(SL08_bisBcst_conflict)
        log_b8_irq(SL_STACK_BIS_SOURCE_TIMING_EN, SL08_bisBcst_conflict, tgtTskFlg);
        #endif

    #if (ONLY_FOR_EBQ_TEST_LATER_REMOVE)
        //pdaAdv_sendNum only ++ when send pda. if interval < 80ms,PDA task will run more then 2, but ignore this situation.
        if(tgtTskFlg == TSKFLG_PERD_ADV && !(global_pBigBcst+big_idx)->big_sc_mask){
            u8 pda_limitionNum = 2;
            st_prd_adv_t *cur_pPerdadv = (st_prd_adv_t*)(global_pPerdadv + pTgtTsk->scheTask_idx);

            if(cur_pPerdadv->pda_tx.bSlot_prdadv_itvl > 1600){ //interval = 1s
                pda_limitionNum = 1;
            }

            if(bigExtAuxPda_conflictCtrl.pdaAdv_sendNum < pda_limitionNum){
                return 0; /* 1:conflict resolved; 0: insert task failed */
            }else{
                return 1; /* 1:conflict resolved; 0: insert task failed */
            }
        }
    #endif


        #if(SCH_TASK_PRIORITY_IN_CB_EN)
            s32 pri_taskCur = bltPri.pri_cal[curSchTaskOft];
            s32 pri_taskTra = bltPri.pri_cal[pTgtTsk->scheTask_oft];
             //priority higher than exist task, can insert target task
            if(pri_taskCur > pri_taskTra){
                return 1;
            }
        #endif

        tlkapi_send_string_data(DEB_BIG_BCST_EN, "[big_bcst]insertTsk conflict, tgtTsk=", &tgtTskFlg, 1);

        //Task scheduler has been abandoned bigger than 5 times
        if(bltPri.csctvAbandonCnt[curSchTaskOft] >= 5){
            tlkapi_send_string_data(DEB_BIG_BCST_EN, "[big_bcst]consecutive abandon count", &bltPri.csctvAbandonCnt[curSchTaskOft], 2);
            return 1; /* 1:conflict resolved; 0: insert task failed */
        }else if((global_pBigBcst+big_idx)->big_sc_mask){
            bltPri.pri_cal[curSchTaskOft] = TASK_PRIORITY_MAX;
            tlkapi_send_string_data(DEB_BIG_BCST_EN, "[big_bcst]subctrl processing", &bltPri.csctvAbandonCnt[curSchTaskOft], 2);
            return 1;
        }else{
            //DBG_CHN8_TOGGLE;
        }
    }
    return 0;
}


init_err_t blt_ll_checkBisBroadcastInit(void){

    if(bltempParam.ll_bisTxFifo_set){
        if(bltBisPduTxfifo.bis_tx_pdu == NULL){
            return LL_BIS_TX_BUF_PARAM_INVALID;
        }
    }
    else{//BIS TX buffer not set
        if(blmsParam.big_bcst_en){  //BIG BCST module init_d but BIS TX buffer not set
            return LL_BIS_TX_BUF_NO_INIT;
        }
    }

    if(sduBisMng.in_fifo_b==NULL){
        return LL_BIS_TX_IAL_BUF_NO_INIT;
    }

#if(FANQH_OPTIMIZE_BIS_API)
    for(int i = 0; i<bltBisMng.maxNum_bisBcst; i++){
        ll_bis_t *pBis = global_pBis + i;
        pBis->bis_sduInBuf = sduBisMng.in_fifo_b + (i*sduBisMng.max_in_fifo_size*sduBisMng.in_fifo_num);
        pBis->bisSduInFreeNum = sduBisMng.in_fifo_num;
    }
#endif

    return INIT_SUCCESS;

}

#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
int         blt_big_bcst_mainloop_task (int flag, void *p)
{
    if(flag == (int)FLAG_MODULE_RESET){
        blt_ll_reset_big_bcst();
    }
    else if(flag ==  (int)FLAG_CHECK_INIT){
        return blt_ll_checkBisBroadcastInit();
    }
    else if(flag ==  (int)FLAG_MODULE_MAINLOOP){
        blt_ll_bigBcstMainloop();
    }
    else if(flag ==  (int)FLAG_MODULE_SET_HOST_CHM){
        blt_ll_ctrlBigBcstChClassUpd((u8*)p);
    }
    else if(flag ==  (int)FLAG_BIG_BRD_HANDLE_SEARCH){
        u8 bigHandle = *((u8*)p);
        return blt_ll_searchExistingBigBcstHdl(bigHandle);
    }

    return 0;
}


_attribute_noinline_
void        blt_ll_reset_big_bcst(void)
{
    ll_big_bcst_t *pBigBcst = NULL;
    ll_bis_t *cur_pBis = NULL;
    //Clear BIG Bcst concerned global variables
    bltBisMng.curNum_bigBcst = 0;
    blmsParam.big_sche_build_pending = 0; //clear
    for(u8 i=0; i<bltBisMng.maxNum_bigBcst; i++)
    {
        pBigBcst = global_pBigBcst + i;
        pBigBcst->adv_handle = INVALID_ADVHD_FLAG;
        pBigBcst->big_handle = BIG_HANDLE_INVALID;
        pBigBcst->cmd_status = BIG_IN_IDLE;
        pBigBcst->big_terminated = 0; //clear
        pBigBcst->big_sc_mask = 0;
        pBigBcst->big_term_inst = 0; // clear
        pBigBcst->big_sc_send_cnt = 0;
        pBigBcst->big_term_cmp_evt = 0;
        pBigBcst->big_create_cmp_evt = 0;
        pBigBcst->bis_cnt = 0;
        pBigBcst->big_sc_cssn = 0;
    }

    //Clear BIS Bcst concerned global variables
    bltBisMng.curNum_bisBcst = 0;
    for(int i=0; i<bltBisMng.maxNum_bisBcst; i++)
    {
        cur_pBis = (ll_bis_t *) (global_pBis + i);
        cur_pBis->bis_occupied = 0;
        cur_pBis->bisSuccessiveMiss = 0; //QW
        cur_pBis->link_big_handle = BIG_HANDLE_INVALID;
        cur_pBis->big_idx = 0;
        cur_pBis->bis_dapth_setup = 0;
        cur_pBis->dpID = 0xff;
        cur_pBis->bisPduTxFifoRptr = cur_pBis->bisPduTxFifoWptr = 0;
        cur_pBis->bisSduIn_rptr = cur_pBis->bisSduIn_wptr = 0;
        cur_pBis->bisSduInFreeNum = sduBisMng.in_fifo_num;
    }
}

void blc_ll_closeBigCtrlPdu(void)
{
    bltBisMng.bisBcst_ctrl_pdu_disable = 1;
}

ble_sts_t   blc_ll_createBigParams( u8      big_handle,         /* Used to identify the BIG */
                                    u8      adv_handle,         /* Used to identify the periodic advertising train */
                                    u8      num_bis,            /* Total number of BISes in the BIG */
                                    u8      sdu_intvl[3],       /* The interval, in microseconds, of periodic SDUs */
                                    u16     max_sdu,            /* Maximum size of an SDU, in octets */
                                    u16     max_trans_lat,      /* Maximum time, in milliseconds, for transmitting an SDU */
                                    u8      rtn,                /* The maximum number of times that every BIS Data PDU should be retransmitted */
                                    u8      phy,                /* The transmitter PHY of packets */
                                    packing_type_t      packing,//type same as u8
                                    bis_framing_t       framing,//type same as u8
                                    u8      enc,                /* Encryption flag */
                                    u8      broadcast_code[16]) /* The code used to derive the session key that is used to encrypt and decrypt BIS payloads */
{
    hci_le_createBigParams_t createBigParams;
    createBigParams.big_handle = big_handle;
    createBigParams.adv_handle = adv_handle;
    createBigParams.num_bis = num_bis;
    smemcpy(createBigParams.sdu_intvl, sdu_intvl, 3);
    createBigParams.max_sdu = max_sdu;
    createBigParams.max_trans_lat = max_trans_lat;
    createBigParams.rtn = rtn;
    createBigParams.phy = phy;
    createBigParams.packing = packing;
    createBigParams.framing = framing;
    createBigParams.enc = enc;
    smemcpy(createBigParams.broadcast_code, broadcast_code, 16);

    ble_sts_t status = blc_hci_le_createBigParams(&createBigParams);
    return status;
}

ble_sts_t   blc_ll_createBigParamsTest( u8      big_handle,         /* Used to identify the BIG */
                                        u8      adv_handle,         /* Used to identify the periodic advertising train */
                                        u8      num_bis,            /* Total number of BISes in the BIG */
                                        u8      sdu_intvl[3],       /* The interval, in microseconds, of periodic SDUs */
                                        u16     iso_intvl,          /* The time between consecutive BIG anchor points */
                                        u8      nse,                /* The total number of subevents in each interval of each BIS in the BIG */
                                        u16     max_sdu,            /* Maximum size of an SDU, in octets */
                                        u16     max_pdu,            /* Maximum size, in octets, of payload */
                                        u8      phy,                /* The transmitter PHY of packets */
                                        u8      packing,//type same as u8
                                        u8      framing,//type same as u8
                                        u8      bn,                 /* The number of new payloads in each interval for each BIS */
                                        u8      irc,                /* The number of times the scheduled payload(s) are transmitted in a given event*/
                                        u8      pto,                /* Offset used for pre-transmissions */
                                        u8      enc,                /* Encryption flag */
                                        u8      broadcast_code[16]) /* The code used to derive the session key that is used to encrypt and decrypt BIS payloads */
{
    hci_le_createBigParamsTest_t  createBigParamsTest;
    createBigParamsTest.big_handle = big_handle;
    createBigParamsTest.adv_handle = adv_handle;
    createBigParamsTest.num_bis = num_bis;
    smemcpy(createBigParamsTest.sdu_intvl, sdu_intvl, 3);
    createBigParamsTest.iso_intvl = iso_intvl;
    createBigParamsTest.nse = nse;
    createBigParamsTest.max_sdu = max_sdu;
    createBigParamsTest.max_pdu = max_pdu;
    createBigParamsTest.phy = phy;
    createBigParamsTest.packing = packing;
    createBigParamsTest.framing = framing;
    createBigParamsTest.bn = bn;
    createBigParamsTest.irc = irc;
    createBigParamsTest.pto = pto;
    createBigParamsTest.enc = enc;
    smemcpy(createBigParamsTest.broadcast_code, broadcast_code, 16);

    ble_sts_t status = blc_hci_le_createBigParamsTest(&createBigParamsTest);
    return status;
}

ble_sts_t   blc_ll_terminateBig(u8 bigHandle, u8 reason)
{
    hci_le_terminateBigParams_t terminateBigParams;
    terminateBigParams.big_handle = bigHandle;
    terminateBigParams.reason = reason;

    ble_sts_t status = blc_hci_le_terminateBig(&terminateBigParams);
    return status;
}

static bool blt_ll_isBigParamsValid(hci_le_createBigParams_t *pParam)
{
    const u8 MAX_BIG_HANDLE = 0xEF;
    const u8 MAX_ADV_HANDLE = 0xEF;
    const u8 MIN_NUM_BIS = 0x01;
    const u8 MAX_NUM_BIS = 0x1F;
    const u32 MIN_SDU_INTERVAL = 0x00100;
    const u32 MAX_SDU_INTERVAL = 0xFFFFF;
    const u16 MAX_SDU = 0x0FFF;
    const u16 MIN_TRANSPORT_LATENCY = 0x0005; //refer to Core5.3, Page 2573
    const u16 MAX_TRANSPORT_LATENCY = 0x0FA0;
    const u8 MAX_RTN = 0x0F;
    const u8 MIN_PHY = 0x01;
    const u8 MAX_PACKING = 0x01;
    const u8 MAX_FRAMING = 0x01;
    const u8 MAX_ENCRYPTION = 0x01;

    u32 sdu_intvl = MAKE_U24(pParam->sdu_intvl[2], pParam->sdu_intvl[1], pParam->sdu_intvl[0]); //unit: us

    if (pParam->big_handle > MAX_BIG_HANDLE)
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"bigHandle out of range", &pParam->big_handle, 1);
        return FALSE;
    }
    if (pParam->adv_handle > MAX_ADV_HANDLE)
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"advHandle out of range", &pParam->adv_handle, 1);
        return FALSE;
    }
    if ((pParam->num_bis < MIN_NUM_BIS) || (pParam->num_bis > MAX_NUM_BIS))
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"num_bis out of range", &pParam->num_bis, 1);
        return FALSE;
    }
    if ((sdu_intvl < MIN_SDU_INTERVAL) || (sdu_intvl > MAX_SDU_INTERVAL))
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"sdu_intvl out of range", &sdu_intvl, 4);
        return FALSE;
    }
    if (pParam->max_sdu > MAX_SDU)
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"max_sdu out of range", &sdu_intvl, 2);
        return FALSE;
    }
    if (pParam->max_trans_lat < MIN_TRANSPORT_LATENCY || pParam->max_trans_lat > MAX_TRANSPORT_LATENCY)
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"max_trans_lat out of range", &pParam->max_trans_lat, 2);
        return FALSE;
    }
    if (pParam->rtn > MAX_RTN)
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"rtn out of range", &pParam->rtn, 1);
        return FALSE;
    }
    if (pParam->phy < MIN_PHY)
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"phy out of range", &pParam->phy, 1);
        return FALSE;
    }
    if (pParam->packing > MAX_PACKING)
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"packing out of range", &pParam->packing, 1);
        return FALSE;
    }
    if (pParam->framing > MAX_FRAMING)
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"framing out of range", &pParam->framing, 1);
        return FALSE;
    }
    if (pParam->enc > MAX_ENCRYPTION)
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"enc out of range", &pParam->enc, 1);
        return FALSE;
    }

    return TRUE;
}

static bool blt_ll_isBigTestParamsValid(hci_le_createBigParamsTest_t *pParam)
{
    const u8 MAX_BIG_HANDLE = 0xEF;
    const u8 MAX_ADV_HANDLE = 0xEF;
    const u8 MIN_NUM_BIS = 0x01;
    const u8 MAX_NUM_BIS = 0x1F;
    const u32 MIN_SDU_INTERVAL = 0x00100;
    const u32 MAX_SDU_INTERVAL = 0xFFFFF;
    const u16 MIN_ISO_INTERVAL = 0x0004;
    const u16 MAX_ISO_INTERVAL = 0x0C80;
    const u8 MIN_NUM_NSE = 0x01;
    const u8 MAX_NUM_NSE = 0x1F;
    const u16 MAX_SDU = 0x0FFF;
    const u8 MIN_PDU = 0x01;
    const u8 MAX_PDU = 0xFB;
    const u8 MIN_PHY = 0x01;
    const u8 MAX_PACKING = 0x01;
    const u8 MAX_FRAMING = 0x01;
    const u8 MIN_BN = 0x01;
    const u8 MAX_BN = 0x07;
    const u8 MIN_IRC = 0x01;
    const u8 MAX_IRC = 0x0F;
    const u8 MAX_PTO = 0x0F;
    const u8 MAX_ENCRYPTION = 0x01;

    u32 sdu_intvl = MAKE_U24(pParam->sdu_intvl[2], pParam->sdu_intvl[1], pParam->sdu_intvl[0]); //unit: us
    u32 iso_interval = pParam->iso_intvl * 1250;

    if (pParam->big_handle > MAX_BIG_HANDLE)
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"big_handle out of range", &pParam->big_handle, 1);
        return FALSE;
    }
    if (pParam->adv_handle > MAX_ADV_HANDLE)
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"adv_handle out of range", &pParam->adv_handle, 1);
        return FALSE;
    }
    if ((pParam->num_bis < MIN_NUM_BIS) || (pParam->num_bis > MAX_NUM_BIS))
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"num_bis out of range", &pParam->num_bis, 1);
        return FALSE;
    }
    if ((sdu_intvl < MIN_SDU_INTERVAL) || (sdu_intvl > MAX_SDU_INTERVAL))
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"sdu_intvl out of range", &sdu_intvl, 4);
        return FALSE;
    }
    if ((pParam->iso_intvl < MIN_ISO_INTERVAL) || (pParam->iso_intvl > MAX_ISO_INTERVAL))
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"iso_intvl out of range", &pParam->iso_intvl, 2);
        return FALSE;
    }
    if ((pParam->nse < MIN_NUM_NSE) || (pParam->nse > MAX_NUM_NSE))
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"nse out of range", &pParam->nse, 1);
        return FALSE;
    }
    if (pParam->max_sdu > MAX_SDU)
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"max_sdu out of range", &pParam->max_sdu, 1);
        return FALSE;
    }
    if ((pParam->max_pdu < MIN_PDU) || (pParam->max_pdu > MAX_PDU))
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"max_pdu out of range", &pParam->max_pdu, 2);
        return FALSE;
    }
    if (pParam->phy < MIN_PHY)
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"phy out of range", &pParam->phy, 1);
        return FALSE;
    }
    if (pParam->packing > MAX_PACKING)
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"packing out of range", &pParam->packing, 1);
        return FALSE;
    }
    if (pParam->framing > MAX_FRAMING)
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"framing out of range", &pParam->framing, 1);
        return FALSE;
    }
    if ((pParam->bn < MIN_BN) || (pParam->bn > MAX_BN))
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"bn out of range", &pParam->bn, 1);
        return FALSE;
    }
    if ((pParam->irc < MIN_IRC) || (pParam->irc > MAX_IRC))
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"irc out of range", &pParam->irc, 1);
        return FALSE;
    }
    if (pParam->pto > MAX_PTO)
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"pto out of range", &pParam->pto, 1);
        return FALSE;
    }
    if (pParam->enc > MAX_ENCRYPTION)
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"enc out of range", &pParam->enc, 1);
        return FALSE;
    }

    if ((pParam->framing == BIS_UNFRAMED) && ((iso_interval % sdu_intvl) > 0))
    {
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"Unframed PDUs must have ISO_Interval that are multiples of the SDU_Interval", 0, 0);
        return FALSE;
    }

    return TRUE;
}

#if 0
void blc_hci_le_creatBigParaTestFun(u32 sdu_intvl, u8 max_sdu,  u8 rtn,  u32 transportlatency, u8 packing, u8 frame, u8 bis_cnt){

    u32 max_pdu, bn, iso_intvl, nse,irc,pto;
    tlkapi_send_string_u32s(0, "input", blt_debug_hex_2_dec_display(sdu_intvl), blt_debug_hex_2_dec_display(max_sdu), rtn, blt_debug_hex_2_dec_display(transportlatency));

    iso_intvl = sdu_intvl;
    max_pdu = min(251, max_sdu);

    if(frame ==0){//unframed
        bn = (max_sdu + max_pdu-1)/max_pdu * (iso_intvl/sdu_intvl);
    }
    else{

    }

    bn = max(1,bn);
    bn = min(bn,7);

    nse = min(bn * (rtn+1), 16/bn *bn);
    irc = max(nse/2, 1);


    u8 ctrl_pdu_len_max = max(sizeof(big_chmInd_data_t), sizeof(big_termInd_data_t));
    u32 tx_max_us = (latest_pBigBcst->max_pdu + 4 + 10) * 8; //mic 4 len
    u32 ctrl_pdu_length_us = (ctrl_pdu_len_max + 4 + 10) * 8;            //mic 4 len  : 2120us



    u32 bis_spacing_us=0;
    u32 sub_interval_us;
    u32 big_iso_pdu_length_total_us;

    if(packing==0){
        sub_interval_us = tx_max_us + TLK_T_MSS;
        bis_spacing_us = sub_interval_us * nse;
        big_iso_pdu_length_total_us = bis_spacing_us * bis_cnt;

        tlkapi_send_string_u32s(0, "space", blt_debug_hex_2_dec_display(sub_interval_us),
                                     blt_debug_hex_2_dec_display(bis_spacing_us), blt_debug_hex_2_dec_display(big_iso_pdu_length_total_us), blt_debug_hex_2_dec_display(tx_max_us));


    }
    else{

    }

    u32 big_sync_delay1 = (bis_cnt-1)*bis_spacing_us + (nse-1)*sub_interval_us + tx_max_us;
    tlkapi_send_string_data(DEB_BIG_BCST_EN,"bigSyncDelay1", &big_sync_delay1,4);

    if(frame==0){
        pto = ((((transportlatency - big_sync_delay1 + sdu_intvl)/
                iso_intvl)) - 1 )/ (nse/bn - irc);
    }
    else{

    }

    if(pto==0){
        irc = nse/bn;
    }

//  tlkapi_send_string_u32s(DEB_BIG_BCST_EN, "big para output", nse,bn,irc,pto);
    tlkapi_send_string_data(DEB_BIG_BCST_EN, "nse", &nse, 4);
    tlkapi_send_string_data(DEB_BIG_BCST_EN, "bn",  &bn,  4);
    tlkapi_send_string_data(DEB_BIG_BCST_EN, "irc", &irc, 4);
    tlkapi_send_string_data(DEB_BIG_BCST_EN, "pto", &pto, 4);

    u32 transportDelay1 = big_sync_delay1 + (pto * (nse/bn -irc)+1)*iso_intvl - sdu_intvl;
    tlkapi_send_string_u32s(DEB_BIG_BCST_EN, "transportLatency1", blt_debug_hex_2_dec_display(transportDelay1),0,0,0);




    nse = 9;
    bn = 3;
    iso_intvl = 30000;
    u32 big_sync_delay2 = (bis_cnt-1)*bis_spacing_us + (nse-1)*sub_interval_us + tx_max_us;
    u32 transportDelay2 = big_sync_delay2 + (1 * (nse/bn -irc)+1)*iso_intvl - sdu_intvl;
    tlkapi_send_string_u32s(DEB_BIG_BCST_EN, "nse = 9", blt_debug_hex_2_dec_display(transportDelay2), 0,0,0);

    nse = 5;
    bn = 1;
    iso_intvl = 10000;
    u32 big_sync_delay3 = (bis_cnt-1)*bis_spacing_us + (nse-1)*sub_interval_us + tx_max_us;
    u32 transportDelay3 = big_sync_delay3 + (1 * (nse/bn -irc)+1)*iso_intvl - sdu_intvl;
    tlkapi_send_string_u32s(DEB_BIG_BCST_EN, "nse = 5", blt_debug_hex_2_dec_display(transportDelay3), 0,0,0);

    nse = 8;
    bn = 2;
    iso_intvl = 20000;
    u32 big_sync_delay4 = (bis_cnt-1)*bis_spacing_us + (nse-1)*sub_interval_us + tx_max_us;
    u32 transportDelay4 = big_sync_delay4 + (1 * (nse/bn -irc)+1)*iso_intvl - sdu_intvl;
    tlkapi_send_string_u32s(DEB_BIG_BCST_EN, "nse = 8", blt_debug_hex_2_dec_display(transportDelay4), 0,0,0);

}
#endif
/*
 * The HCI_LE_Create_BIG command is used to create a BIG with one or more
 * BISes (see [Vol 6] Part B, Section 4.4.6). All BISes in a BIG have the same
 * value for all parameters.
 */
ble_sts_t   blc_hci_le_createBigParams(hci_le_createBigParams_t* pCmdParam)
{
    /*
     * The parameters are not in the specified range, the Controller shall
     * return the error code Unsupported Feature or Parameter Value(0x11).
     */
    if(!blt_ll_isBigParamsValid(pCmdParam)){
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }
    /*
     * If the Controller cannot create all BISes of the BIG or if Num_BIS exceeds the
     * maximum value supported by the Controller, it shall return the error code
     * Connection Rejected due to Limited Resources (0x0D).
     */
    if(pCmdParam->num_bis > LL_BIS_IN_PER_BIG_BCST_NUM_MAX){ //per big_bcst: max bis num supported by Link Layer
        return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
    }
    if(pCmdParam->num_bis > blc_ll_getAvailBisNum(BIS_ROLE_BCST)){
        return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
    }

    /*
    * If the Advertising_Handle does not identify a periodic advertising train or the
    * periodic advertising train is associated with another BIG, the Controller shall
    * return the error code Unknown Advertising Identifier (0x42)
    */
    st_prd_adv_t *cur_pPerdadv = blt_ll_search_existing_perdAdv_index_by_advHandle(pCmdParam->adv_handle);
    if(cur_pPerdadv == NULL || (blt_ll_isPerdAdvEnable(pCmdParam->adv_handle)!=1) || \
                               (cur_pPerdadv->link_big_handle != BIG_HANDLE_INVALID)){
        return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
    }

    /*
     * If the Host issues this command with a BIG_Handle for a BIG that is already
     * created, the Controller shall return the error code Command Disallowed (0x0C).
     */
    if(blt_ll_searchExistingBigBcstHdl(pCmdParam->big_handle)!=BIG_HANDLE_INVALID){
        return HCI_ERR_CMD_DISALLOWED;
    }

    /*
     * If the Host sets PHY to a value that the Controller does not support, including a
     * bit that is reserved for future use, the Controller shall return the error code
     * Unsupported Feature or Parameter Value (0x11).
     */
    if(!(pCmdParam->phy&0x07) || (pCmdParam->phy&0xF8)){
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    /*
     * Allocate new Big handle
     */
    u8 big_idx = blt_ll_AllocateNewBigBcstHdl(pCmdParam->big_handle);
    if(big_idx == BIG_HANDLE_INVALID){//update latest_pBigBcst if allocate successfully
        return HCI_ERR_MEM_CAP_EXCEEDED;
    }

    //Associate Periodic ADV train with big_handle
    cur_pPerdadv->link_big_handle = pCmdParam->big_handle;
    cur_pPerdadv->big_idx = big_idx;


//  latest_pBigBcst->big_role = BIS_ROLE_BCST;//Broadcast BIS
    latest_pBigBcst->adv_handle = pCmdParam->adv_handle;
    latest_pBigBcst->bis_cnt = pCmdParam->num_bis;
    latest_pBigBcst->sdu_intvl = MAKE_U24(pCmdParam->sdu_intvl[2], pCmdParam->sdu_intvl[1], pCmdParam->sdu_intvl[0]); //unit:us
    latest_pBigBcst->max_sdu = pCmdParam->max_sdu;

#if(BIS_BRD_SET_PARAM)
    latest_pBigBcst->max_pdu = min(bltBisPduTxfifo.txPduLenMax, latest_pBigBcst->max_sdu);
#else
    latest_pBigBcst->max_pdu = min(251, latest_pBigBcst->max_sdu);
#endif
    latest_pBigBcst->phy = pCmdParam->phy & 0x07;
    latest_pBigBcst->packing = pCmdParam->packing; //0x00 Sequential, 0x01 Interleaved
    latest_pBigBcst->framing = pCmdParam->framing; //0x00 Unframed,   0x01 Framed
    latest_pBigBcst->encryption = pCmdParam->enc; //0x00 Unencrypted, 0x01 Encrypted

    latest_pBigBcst->big_start_tick = 0;

    latest_pBigBcst->seedAccessAddress = blt_ll_bis_getSeedAccessAddr();
    generateRandomNum(2, (u8*)&latest_pBigBcst->baseCrcInit); //generate random for baseCrcInit

    /* The Access Address for each BIS and for the BIG Control logical link (see
     * Section 4.4.6.7) in a BIG shall be derived from the SAA for that BIG.
     * For each BIS logical transport, the Access Address shall be equal to the SAA
     * bit-wise XORed with a diversifier word (DW) for that logical transport derived
     * from a Diversifier (D) as follows:
     *      D = ((35 * n) + 42) MOD 128 where n is the BIS number, or 0 for the BIG Control logical link
     *      DW = 0bD0D0D0D0D0D0D1D6_D10D5D40D3D20_00000000_00000000 */
    latest_pBigBcst->scAccessCode = blt_ll_bis_getAccessCode(latest_pBigBcst->seedAccessAddress, 0); //big control AccessCode
    /* For every Broadcast Isochronous PDU, the shift register shall be preset with the
     * BaseCRCInit value from the BIGInfo data (see Section 4.4.6.11) in the most
     * significant 2 octets and the BIS_Number for the specific BIS in the least
     * significant octet. For BIG Control PDUs, the least significant octet shall be 0. */
    latest_pBigBcst->scCrcInit    = (latest_pBigBcst->baseCrcInit << 8) | 0;


    //todo There is link lay adaptive configuration.
    latest_pBigBcst->iso_itvl = latest_pBigBcst->sdu_intvl / 1250; //1.25ms unit

    /*
     * 1.Unframed PDUs shall only be used when the ISO_Interval is equal to or an
     *   integer multiple of the SDU_Interval and a constant time offset alignment is
     *   maintained between the SDU generation and the timing in the isochronous transport.
     * 2.The Framing parameter indicates the format for sending BIS Data PDUs. If the
     *   Framing parameter is set to 1 then BIS Data PDUs shall be Framed and when
     *   set to 0 they "may be" unframed (see [Vol 6] Part G, Section 1)
     */
    if(latest_pBigBcst->framing == BIS_UNFRAMED && (latest_pBigBcst->sdu_intvl % 1250)){
        latest_pBigBcst->framing = BIS_FRAMED;
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"framing switch from UNFRAMED to FRAMED", 0, 0);
    }
    tlkapi_send_string_data(DEB_BIG_BCST_EN,"framing:", &latest_pBigBcst->framing, 1);



#if(BIS_BRD_SET_PARAM)
    u8 bn = 1;
    if(latest_pBigBcst->packing==BIS_UNFRAMED){
        /*
         * bn = ceil(max_sdu/max_pdu)*(iso_intvl/sdu_intvl)
         */
        bn = ((latest_pBigBcst->max_sdu + latest_pBigBcst->max_pdu -1 ) / latest_pBigBcst->max_pdu) \
                                        * (latest_pBigBcst->iso_itvl/latest_pBigBcst->sdu_intvl);

    }
    else{
        /*
         * core spec 5.2 P3216
         * Max_PDU>=ceil(ISO_Interval * (5+Max_SDU)*(1+MaxDrift)/(SDU_Interval * BN)+2)
         */
        bn = ((latest_pBigBcst->max_sdu + 5)*latest_pBigBcst->iso_itvl) / ((latest_pBigBcst->max_pdu-2)*latest_pBigBcst->sdu_intvl);
    }

    bn = max(1,bn);
    latest_pBigBcst->bn = min(bn, 7);

    latest_pBigBcst->nse = min(latest_pBigBcst->bn * (pCmdParam->rtn + 1), LL_SE_IN_PER_BIS_NUM_MAX/bn*bn);//core spec range 1~31
    latest_pBigBcst->irc = max(latest_pBigBcst->nse/(bn*2), 1);
#else
    latest_pBigBcst->irc = pCmdParam->rtn + 1;          //The number of times the scheduled payload(s) are transmitted in a given event.
    latest_pBigBcst->pto = 0;                           //Offset used for pre-transmissions

    latest_pBigBcst->bn  = (latest_pBigBcst->max_sdu + latest_pBigBcst->max_pdu -1 ) / latest_pBigBcst->max_pdu;
    latest_pBigBcst->nse = (latest_pBigBcst->bn + latest_pBigBcst->pto) * latest_pBigBcst->irc;
#endif
                                                        //The number of new payloads in each interval for each BIS.

    u8 micLen = pCmdParam->enc ? 4: 0;

    //  2M PHY   :     (rf_len + 11) * 4
    // Coded PHY :  = 376 + (rf_len*8+43)*S
    u32 tx_max_us;
    u32 ctrl_pdu_length_us; //TX max PDU translate time, uS
    u8  curBisPhy;
    //TODO: here only consider S8 here, need add S2 later
    u8 coding_ind = bltPHYs.dft_CI ? bltPHYs.dft_CI : LE_CODED_S8;
    u8 ctrl_pdu_len_max = max(sizeof(big_chmInd_data_t), sizeof(big_termInd_data_t));

    if(latest_pBigBcst->phy & PHY_PREFER_1M){
        curBisPhy = BLE_PHY_1M; //The Controller only supports asymmetric PHYs.
        tx_max_us = (latest_pBigBcst->max_pdu + micLen + 10) * 8; //mic 4 len
        ctrl_pdu_length_us = (ctrl_pdu_len_max + micLen + 10) * 8;           //mic 4 len  : 2120us
    }
    else if(latest_pBigBcst->phy & PHY_PREFER_2M){
        curBisPhy = BLE_PHY_2M;
        tx_max_us = (latest_pBigBcst->max_pdu + micLen + 11) * 4; //mic 4 len
        ctrl_pdu_length_us = (ctrl_pdu_len_max + micLen + 11) * 4;           //mic 4 len
    }
    else{
        curBisPhy = BLE_PHY_CODED;  //dft: LE_CODED_S8
        tx_max_us = 376 + ((latest_pBigBcst->max_pdu + micLen)*64 + 43) * (coding_ind == LE_CODED_S8 ? 8 : 2); //mic 4 len
        ctrl_pdu_length_us =  376 + ((ctrl_pdu_len_max + micLen)*64 + 43) * (coding_ind == LE_CODED_S8 ? 8 : 2); //mic 4 len
    }

    if(bltBisMng.bisBcst_ctrl_pdu_disable){
        ctrl_pdu_length_us = 0;
    }


    u16 tx_prepare_us = TX_STL_ADV_REAL_COMMON;
    latest_pBigBcst->bis_distance_schToAir_us = TLK_TX_TRIG_OFFSET + 4 + tx_prepare_us;


    latest_pBigBcst->curBisPhy = curBisPhy;
    latest_pBigBcst->codingInd = coding_ind;

    u32 bis_spacing_us;
    u32 sub_interval_us;
    u32 big_iso_pdu_length_total_us;
    //Calculate BIS_Spacing && Sub_Interval: Both BIS_Spacing and Sub_Interval shall be at least T_MSS + MPT..
    if(latest_pBigBcst->packing == PACK_SEQUENTIAL){ // sequential
        //For sequential arrangement: BIS_Spacing >= NSE * Sub_Interval
        sub_interval_us = tx_max_us + TLK_T_MSS;
        bis_spacing_us = sub_interval_us * latest_pBigBcst->nse;
        //For sequential arrangement: BIG_Control_Offset = Num_BIS * BIS_Spacing
        big_iso_pdu_length_total_us = bis_spacing_us * latest_pBigBcst->bis_cnt;
    }
    else{ //interleaved
        //For interleaved arrangement: Sub_Interval >= Num_BIS * BIS_Spacing
        bis_spacing_us = tx_max_us + TLK_T_MSS;
        sub_interval_us = bis_spacing_us * latest_pBigBcst->bis_cnt;
        //For interleaved arrangement: BIG_Control_Offset = NSE * Sub_Interval
        big_iso_pdu_length_total_us = sub_interval_us * latest_pBigBcst->nse;
    }


    //The BIG_Sync_Delay = (Num_BIS - 1) * BIS_Spacing + (NSE - 1) * Sub_Interval + MPT.
    latest_pBigBcst->big_sync_delay_us = (latest_pBigBcst->bis_cnt - 1) * bis_spacing_us + (latest_pBigBcst->nse - 1) * sub_interval_us + tx_max_us;
    latest_pBigBcst->iso_pdu_task_us = big_iso_pdu_length_total_us;
    latest_pBigBcst->MPT = tx_max_us;
    latest_pBigBcst->SCPT = ctrl_pdu_length_us;
    latest_pBigBcst->se_length_tick    = (tx_max_us + TLK_T_MSS) * SYSTEM_TIMER_TICK_1US;

#if(BIS_BRD_SET_PARAM)
    /*
     * Transport_Latency = BIG_Sync_Delay + (PTO *(NSE % BN - IRC) +1) * ISO_Interval -  SDU_Interval
     */
    if(latest_pBigBcst->packing==BIS_UNFRAMED){
        latest_pBigBcst->pto = ((((pCmdParam->max_trans_lat*1000 - latest_pBigBcst->big_sync_delay_us + latest_pBigBcst->sdu_intvl)/
                latest_pBigBcst->iso_itvl*1250)) - 1 )/ (latest_pBigBcst->nse/latest_pBigBcst->bn - latest_pBigBcst->irc);
    }
    else{
        /*
         * Transport_Latency_BIG = BIG_Sync_Delay + PTO * (NSE / BN - IRC) * ISO_Interval + ISO_Interval + SDU_Interval
         */
        latest_pBigBcst->pto = ((pCmdParam->max_trans_lat*1000 - latest_pBigBcst->big_sync_delay_us - latest_pBigBcst->sdu_intvl -
                latest_pBigBcst->iso_itvl*1250))/
                (latest_pBigBcst->iso_itvl*1250 * (latest_pBigBcst->nse/latest_pBigBcst->bn - latest_pBigBcst->irc));
    }
#endif

    //latest_pBigBcst->bigEventCnt = 0; //u64
    latest_pBigBcst->bigEventCnt = 0;

    smemcpy(latest_pBigBcst->chnParam.map.chmTbl, blmhostChnClassUpt.gLlChannelMap, 5);

    latest_pBigBcst->chnIdentifier = (latest_pBigBcst->scAccessCode>>16) ^ (latest_pBigBcst->scAccessCode&0xffff);
    csa2_calculateMapInfo(&latest_pBigBcst->chnParam.map);


    ///// find available BIS for current BIG ///////////////////
    int new_bis_cnt = 0;
    ll_bis_t *cur_pBis = NULL;
    u8 bis_order_tbl[LL_BIS_IN_PER_BIG_BCST_NUM_MAX];
    u8 bis_order_cnt = 0;
    latest_pBigBcst->bis_alloc_msk = 0;

    for(int i = 0; i< bltBisMng.maxNum_bisBcst; i++){
        cur_pBis = (ll_bis_t *) (global_pBis + i);

        if(cur_pBis->bis_occupied == 0){
            cur_pBis->bis_occupied = 1;
            bltBisMng.curNum_bisBcst ++;  //update current BIS number

            latest_pBigBcst->bis_handle[new_bis_cnt] = cur_pBis->bis_handle;

            cur_pBis->link_big_handle   = pCmdParam->big_handle;
            cur_pBis->big_idx           = big_idx;
            cur_pBis->lastEventCnt      = 0;
            cur_pBis->lastPayloadNum    = 0;
            cur_pBis->curBisPldNum      = 0;
            cur_pBis->bis_dapth_setup   = 0;
//          cur_pBis->curBisPhy         = curBisPhy;
//          cur_pBis->codingInd         = coding_ind; //S2 or S8: LE_CODED_S2 / LE_CODED_S8
            cur_pBis->sub_interval_us   = sub_interval_us;
            cur_pBis->sub_interval_tick = sub_interval_us * SYSTEM_TIMER_TICK_1US;
            cur_pBis->bis_spacing_us    = bis_spacing_us;
            cur_pBis->bis_spacing_tick  = cur_pBis->bis_spacing_us * SYSTEM_TIMER_TICK_1US;



//          cur_pBis->bisEventCnt = 0;
            cur_pBis->bisPduTxFifoWptr = cur_pBis->bisPduTxFifoRptr = 0;

            cur_pBis->nse = latest_pBigBcst->nse;
            cur_pBis->bn  = latest_pBigBcst->bn;
            cur_pBis->irc = latest_pBigBcst->irc;
            cur_pBis->pto = latest_pBigBcst->pto;

            cur_pBis->txBnIdx =0;
            cur_pBis->txSduIdx = 0;
            cur_pBis->tx_first_pdu = 1;
            cur_pBis->bisSduIn_wptr = cur_pBis->bisSduIn_rptr = 0;
            cur_pBis->bisSduInFreeNum = sduBisMng.in_fifo_num;
            cur_pBis->lossFlag = 0;
            bis_order_tbl[bis_order_cnt++] = i;
            latest_pBigBcst->bis_alloc_msk |= BIT(i);

            new_bis_cnt++; //0 for BIG Control, other s for BIS use
            cur_pBis->bisAccessAddr =blt_ll_bis_getAccessCode(latest_pBigBcst->seedAccessAddress, new_bis_cnt); //bisAccessCode
            cur_pBis->bisCrcInit = (latest_pBigBcst->baseCrcInit << 8) | new_bis_cnt;

            cur_pBis->chnIdentifier = (cur_pBis->bisAccessAddr >> 16) ^ (cur_pBis->bisAccessAddr >> 0);

            //////////////////////////////////////////////////
            // Encryption parameters init
            //////////////////////////////////////////////////
            if(latest_pBigBcst->encryption){
                //The IV for a CIS or BIS is calculated from an IVbase and the Access Address of the CIS or BIS respectively.
                //Generation of IV for a CIS or BIS: IV[31:0] shall equal IVbase[31:0] XORed with the Access Address of the
                //CIS or BIS while IV [63:32] shall equal IVbase[63:32].
                u32 iv[2] = { 0};
                //Initialize the IV          //BIS IV: little--endian, our SDK's AES CCM's iv need little--endian
                smemcpy(&iv, latest_pBigBcst->BigInfor.giv, 8);//For a BIS, the IVbase shall be set to the value of GIV contained in the BIGInfo.
                iv[0] ^= cur_pBis->bisAccessAddr; //Generation of IV for a CIS or BIS
                smemcpy(cur_pBis->bisCryptCtrl.nonce.iv, &iv, 8);
                //tlkapi_send_string_data(DEB_BIG_BCST_EN,"IV", cur_pBis->bisCryptCtrl.ccmNonce.iv, 8);

                //Initialize the session key //BIS's SK equal to BIG CTRL's SK: big--endian, our SDK's AES CCM's sk need big--endian
                smemcpy (cur_pBis->bisCryptCtrl.sk, latest_pBigBcst->bigCtrlCrypt.sk, 16);

                //Open BIS encryption enable flg
                cur_pBis->bisCryptCtrl.enable = 1; //Enable encryption
                cur_pBis->bisCryptCtrl.mic_fail = 0;
            }
            else{
                cur_pBis->bisCryptCtrl.enable = 0;
            }

            sdu_packet_t *bis_sdu;
            for(int n = 0; n<sduBisMng.in_fifo_num; n++){
                bis_sdu =(sdu_packet_t*) (sduBisMng.in_fifo_b + sduBisMng.max_in_fifo_size * (n + i*sduBisMng.in_fifo_num));
                bis_sdu->sduOffset = 0;
            }

            #if (HCI_SEND_NUM_OF_CMP_AFT_ACK)
                cur_pBis->nocAckNum = 32;
                cur_pBis->nocAckMsk = 32-1;
                cur_pBis->nocAckWptr = cur_pBis->nocAckRptr = 0;
                smemset(cur_pBis->nocAclTxWptr, 0, 32*2);
            #endif
            
            if(new_bis_cnt >= pCmdParam->num_bis){
                break;
            }
        }
    }


    tlkapi_send_string_data(DBG_IAL_EN,"max_sdu", &latest_pBigBcst->max_sdu, 2);
    tlkapi_send_string_data(DBG_IAL_EN,"max_pdu", &latest_pBigBcst->max_pdu, 2);
    tlkapi_send_string_data(DBG_IAL_EN,"numSdu2Pdu", &cur_pBis->numSdu2Pdu, 1);
    tlkapi_send_string_data(DBG_IAL_EN,"numSduEachEvent", &cur_pBis->numSduEachEvent, 1);

    ////////////////////  BIS arrangement map ////////////////////////
    latest_pBigBcst->bis_total_se_num = 0;
    for(int i = 0; i < latest_pBigBcst->bis_cnt; i++){
        if(latest_pBigBcst->packing == PACK_SEQUENTIAL){ //Sequential: e.g.: 112233
            for(int j=0; j<latest_pBigBcst->nse ;j++){
                latest_pBigBcst->bis_arrgmt_map[latest_pBigBcst->bis_total_se_num + j] = bis_order_tbl[i];
            }
        }
        else{ //== PACK_INTERLEAVED  //Interleaved: e.g.: 123123
            for(int j=0; j<latest_pBigBcst->nse ;j++){ //NSE same for all BISes
                latest_pBigBcst->bis_arrgmt_map[i + j * latest_pBigBcst->bis_cnt] = bis_order_tbl[i];
            }
        }

        latest_pBigBcst->bis_total_se_num += latest_pBigBcst->nse;
    }

    //For specific defined data, we default the ISO Interval and SDU Interval to the same value.
    u32 bigTaskDurMaxUs = big_iso_pdu_length_total_us + ctrl_pdu_length_us + (SLOT_PROCESS_MAX_TICK/SYSTEM_TIMER_TICK_1US);
    u32 bigTmpItvlUs = latest_pBigBcst->iso_itvl*1250;

    if(bigTmpItvlUs<bigTaskDurMaxUs){
        u32 mod = bigTaskDurMaxUs%bigTmpItvlUs ? 1 : 0;
        latest_pBigBcst->iso_itvl = (bigTaskDurMaxUs/bigTmpItvlUs + mod)*latest_pBigBcst->iso_itvl;
    }

    /*  Refer to Core 5.2 | Vol 6, Part G, Page 3220  */
    if(latest_pBigBcst->framing == BIS_UNFRAMED){
        /*
         * Transport_Latency = BIG_Sync_Delay + (PTO * (NSE / BN - IRC) + 1) * ISO_Interval - SDU_Interval
         */
        latest_pBigBcst->transLatency_us = latest_pBigBcst->big_sync_delay_us +
                                           ((latest_pBigBcst->pto*(latest_pBigBcst->nse/latest_pBigBcst->bn - latest_pBigBcst->irc)) + 1) * (latest_pBigBcst->iso_itvl*1250) -
                                           latest_pBigBcst->sdu_intvl;
    }
    else{ /* FRAMED */
        /*
         *Transport_Latency_BIG = BIG_Sync_Delay + (PTO * (NSE / BN - IRC) + 1) * ISO_Interval + SDU_Interval
         */
        latest_pBigBcst->transLatency_us = latest_pBigBcst->big_sync_delay_us +
                                           ((latest_pBigBcst->pto*(latest_pBigBcst->nse/latest_pBigBcst->bn - latest_pBigBcst->irc)) + 1) * (latest_pBigBcst->iso_itvl*1250) +
                                           latest_pBigBcst->sdu_intvl;
    }

    /*
     * Fix /HCI/BIS/BI-07-C [Broadcast Isochronous Stream Using Non-Test Command, Invalid Transport Latency]
     */
    if(latest_pBigBcst->transLatency_us > pCmdParam->max_trans_lat*1000)
    {
        //Clear BIS Bcst concerned global variables
        for(int i = 0; i< bltBisMng.maxNum_bisBcst; i++){
            cur_pBis = (ll_bis_t *) (global_pBis + i);
            if(latest_pBigBcst->big_handle == cur_pBis->link_big_handle && cur_pBis->bis_occupied){
                cur_pBis->bis_occupied = 0;
                bltBisMng.curNum_bisBcst --;  //update current BIS number
                cur_pBis->bisSuccessiveMiss = 0; //QW
                cur_pBis->link_big_handle = BIG_HANDLE_INVALID;
                cur_pBis->big_idx = 0;
                cur_pBis->bis_dapth_setup = 0;
                cur_pBis->dpID = 0xff;
                cur_pBis->bisPduTxFifoRptr = cur_pBis->bisPduTxFifoWptr = 0;
                cur_pBis->bisSduIn_rptr = cur_pBis->bisSduIn_wptr = 0;
                cur_pBis->bisSduInFreeNum = sduBisMng.in_fifo_num;
            }
        }

        //Clear BIG Bcst concerned global variables
        bltBisMng.curNum_bigBcst--;
        latest_pBigBcst->adv_handle = INVALID_ADVHD_FLAG;
        latest_pBigBcst->big_handle = BIG_HANDLE_INVALID;
        latest_pBigBcst->cmd_status = BIG_IN_IDLE;
        latest_pBigBcst->big_terminated = 0; //clear
        latest_pBigBcst->big_sc_mask = 0;
        latest_pBigBcst->big_term_inst = 0; // clear
        latest_pBigBcst->big_sc_send_cnt = 0;
        latest_pBigBcst->big_sc_cssn = 0;
        latest_pBigBcst->big_term_cmp_evt = 0;
        latest_pBigBcst->big_create_cmp_evt = 0;
        latest_pBigBcst->bis_cnt = 0;

        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    //Not sure TODO: min or max how to decide?
    latest_pBigBcst->transLatency_us = min(pCmdParam->max_trans_lat*1000, latest_pBigBcst->transLatency_us);

    for(int i = 0; i< bltBisMng.maxNum_bisBcst; i++){
        cur_pBis = (ll_bis_t *) (global_pBis + i);
        if(cur_pBis->bis_occupied == 1){
            cur_pBis->numSdu2Pdu = ((latest_pBigBcst->bn * latest_pBigBcst->sdu_intvl) + (latest_pBigBcst->iso_itvl*1250) -1 )/(latest_pBigBcst->iso_itvl*1250);
            cur_pBis->numSduEachEvent = (latest_pBigBcst->iso_itvl*1250 + latest_pBigBcst->sdu_intvl -1)/(latest_pBigBcst->sdu_intvl);
        }
    }
    //////////////////////////// BIG slot Timing && task create Start //////////////////////////
    u32 r = irq_disable();
    //move sch process time to here, there can dynamic modify
    latest_pBigBcst->sSlot_duration_big = (big_iso_pdu_length_total_us + ctrl_pdu_length_us + SLOT_PROCESS_MAX_US
            + latest_pBigBcst->bis_distance_schToAir_us + TLK_TM_DELAY )*SSLOT_US_REVERSE + 1;
    latest_pBigBcst->sSlot_interval_big = BSLOT_DUR_2_SSLOT_DUR(latest_pBigBcst->iso_itvl*2);  //1.25mS -> 625 uS -> 19.53us

    irq_restore(r);
    ///////////////////////////// BIG slot Timing && task create End ///////////////////////////


    //////////////////////////////////  Assemble BIG Info begin //////////////////////////////////
    latest_pBigBcst->BigInfor.bigOffset = 600;                                      //The time from the start of the packet containing the BIGInfo to the next BIG anchor point. bigOffset >= 600us
    latest_pBigBcst->BigInfor.bigOffsetUnits = BIG_PDU_BIG_OFFSET_UNITS_30_US;  //The actual time offset is determined by multiplying the value of BIG_Offset by the unit. unit: 300us or 30us

    latest_pBigBcst->BigInfor.isoItvl = latest_pBigBcst->iso_itvl;                  //ISO_Interval is the time between two adjacent BIG anchor points, in units of 1.25 ms. The value shall be between 4 and 3200 (i.e. 5 ms to 4 s).
    latest_pBigBcst->BigInfor.numBis = latest_pBigBcst->bis_cnt;                    //The Num_BIS field shall contain the number of BISes in the BIG.
    latest_pBigBcst->BigInfor.nse = latest_pBigBcst->nse;                           //NSE is the number of subevents per BIS in each BIG event. The value shall be between 1 and 31 and shall be an integer multiple of BN.
    latest_pBigBcst->BigInfor.bn = latest_pBigBcst->bn;                             //The value of BN shall be between 1 and 7.
    latest_pBigBcst->BigInfor.subItvl = sub_interval_us;                            //Sub_Interval is the time between the start of two consecutive subevents of each BIS.
    latest_pBigBcst->BigInfor.pto = latest_pBigBcst->pto;                           //The value of PTO shall be between 0 and 15.
    latest_pBigBcst->BigInfor.bisSpacing = bis_spacing_us;                          //BIS_Spacing is the time between the start of corresponding subevents in adjacent BISes in the BIG and also the time between the start of the first
                                                                                    //subevent of the last BIS and the control subevent, if present.
    latest_pBigBcst->BigInfor.irc = latest_pBigBcst->irc;                           //The value of IRC shall be between 1 and 15.
    latest_pBigBcst->BigInfor.maxPdu = latest_pBigBcst->max_pdu;                    //Max_PDU is the maximum number of data octets (excluding the MIC, if any) that can be carried in each BIS Data PDU in the BIG. The value shall be
                                                                                    //between 0 and 251 octets.
    latest_pBigBcst->BigInfor.seedAA = latest_pBigBcst->seedAccessAddress;          //The SeedAccessAddress field shall contain the Seed Access Address for the BIG
    latest_pBigBcst->BigInfor.sduItvl = latest_pBigBcst->sdu_intvl;             //Sub_Interval is the time between the start of two consecutive subevents of each BIS.
    latest_pBigBcst->BigInfor.maxSdu = latest_pBigBcst->max_sdu;                    //Max_SDU is the maximum size of an SDU on this BIG
    latest_pBigBcst->BigInfor.baseCrcInit = latest_pBigBcst->baseCrcInit;
    smemcpy(latest_pBigBcst->BigInfor.chm37Phy3, blmhostChnClassUpt.gLlChannelMap, 5); //The ChM field shall have the same meaning as the corresponding field in the CONNECT_IND PDU
    latest_pBigBcst->BigInfor.chm37Phy3[4] |= ((curBisPhy-1) << 5);                 //The PHY field shall be set to indicate the PHY used by the BIG.0 LE 1M PHY; 1 LE 2M PHY; 2 LE Coded PHY
    //u64 bisPldCntr = latest_pBigBcst->bigEventCnt * latest_pBigBcst->bn;
    u32 bisPldCntr = latest_pBigBcst->bigEventCnt * latest_pBigBcst->bn;            //bisPayloadCounter belong to [bigEventCounter * BN, (bigEventCounter + 1) * BN - 1]
    smemcpy(latest_pBigBcst->BigInfor.bisPldCnt39Framing1, &bisPldCntr, 4);         //The value shall be for the first subevent of the BIG event referred to by the BIG_Offset field
    //NOTE: bisPldCntr use u32 Not u64, so here should not use "|"
    //latest_pBigBcst->BigInfor.bisPldCnt39Framing1[4] |= (latest_pBigBcst->framing << 7);
    latest_pBigBcst->BigInfor.bisPldCnt39Framing1[4] = (latest_pBigBcst->framing << 7); //The Framing bit shall be set if the BIG carries framed data.
    if(latest_pBigBcst->encryption){
        generateRandomNum(8, latest_pBigBcst->BigInfor.giv);   //generate random for GIV  //GIV:  little--endian
        generateRandomNum(16, latest_pBigBcst->BigInfor.gskd); //generate random for GSKD //GSKD: little--endian

        //broadcast_code: little--endian ==> big--endian
        swapX(pCmdParam->broadcast_code, latest_pBigBcst->broadcast_code, 16); //The code used to derive the session key that is used to encrypt and decrypt BIS payloads.

        cur_pPerdadv->acad_field_len = 57;

        ///////////// Calculate the BIG/BIS session key //////////////////
        //IGLTK = h7("BIG1", Broadcast_Code)
        //GLTK = h6(IGLTK, "BIG2")
        //GSK = h8 (GLTK, GSKD, "BIG3")
        //GSK is used as the session key in the CCM algorithm when encrypting the BIG.
        u8 tmp_igltk[16], tmp_gltk[16], tmp_gskd[16];
        u8 BIG1[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x42,0x49,0x47,0x31};    //big-endian
        u8 BIG2[4]  = {0x42,0x49,0x47,0x32};                                                                //big-endian
        u8 BIG3[4]  = {0x42,0x49,0x47,0x33};                                                                //big-endian

        swapX(latest_pBigBcst->BigInfor.gskd, tmp_gskd, 16);                            //GSKD: little--endian ==> big-endian
        blt_crypto_alg_h7 (tmp_igltk, BIG1, latest_pBigBcst->broadcast_code);           //Broadcast_Code: big-endian
        blt_crypto_alg_h6 (tmp_gltk, tmp_igltk, BIG2);
        blt_crypto_alg_h8 (latest_pBigBcst->bigCtrlCrypt.sk, tmp_gltk, tmp_gskd, BIG3); //Our SDK's AES CCM's sk need big--endian.
        //tlkapi_send_string_data(0,"BIG/BIS SK", latest_pBigBcst->bigCtrlCrypt.sk, 16);

        ///////////// Calculate the BIG CTRL IV key //////////////////
        //The IV for a CIS or BIS is calculated from an IVbase and the Access Address of the CIS or BIS respectively.
        //Generation of IV for a CIS or BIS: IV[31:0] shall equal IVbase[31:0] XORed with the Access Address of the
        //CIS or BIS while IV [63:32] shall equal IVbase[63:32].
        //The IV for a BIG control logical link shall be determined in the same way as for a BIS.
        u32 iv[2] = { 0};
        //Initialize the IV          //IV: little--endian, our SDK's AES CCM's iv need little--endian
        smemcpy(&iv, latest_pBigBcst->BigInfor.giv, 8); //For a BIS, the IVbase shall be set to the value of GIV contained in the BIGInfo.
        iv[0] ^= latest_pBigBcst->scAccessCode;         //Generation of IV for a CIS or BIS
        smemcpy(latest_pBigBcst->bigCtrlCrypt.nonce.iv, &iv, 8);
        //tlkapi_send_string_data(0,"BIG CTRL IV", latest_pBigBcst->bigCtrlCrypt.nonce.iv, 8);

        //Open BIG Control PDU encryption enable flg
        latest_pBigBcst->bigCtrlCrypt.enable = 1; //Enable encryption
        latest_pBigBcst->bigCtrlCrypt.mic_fail = 0;
    }
    else{
        smemset(latest_pBigBcst->broadcast_code, 0, 16);      //clear Broadcast Code
        smemset(latest_pBigBcst->BigInfor.giv, 0, 8);   //clear GIV
        smemset(latest_pBigBcst->BigInfor.gskd, 0, 16); //clear GSKD
        latest_pBigBcst->bigCtrlCrypt.enable = 0;

        cur_pPerdadv->acad_field_len = 33;
    }

    cur_pPerdadv->acad[0] = cur_pPerdadv->acad_field_len+1;
    /*
     * https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile/
     */
    cur_pPerdadv->acad[1] = DT_BIGINFO; //adv type: 0x2C    <<BIGInfo>>
    smemcpy(cur_pPerdadv->acad+2,  (u8*)&latest_pBigBcst->BigInfor, cur_pPerdadv->acad_field_len);
    cur_pPerdadv->acad_field_len += 2; //len + type

    cur_pPerdadv->acad_used &= ~PERD_ACAD_BIGINFO_ENA; //Aux_Sync_Ind { + ACAD(BIGInfo) : after BIG task begin, we will enable ACAD later!!! }

    //////////////////////////////////  Assemble BIG Info end //////////////////////////////////


    /* Priority preset value */
    blt_ll_set_interval_level(TSKOFT_BIG_BCST + big_idx, latest_pBigBcst->iso_itvl);
    blt_ll_setSchedulerTaskPriority(TSKOFT_BIG_BCST + big_idx, TASK_PRIORITY_BIG_BCST_DFT);


    latest_pBigBcst->cmd_status = BIG_CREATE_PENDING;//pending
    blmsParam.big_sche_build_pending = BIG_SLOT_BUILD_MSK | big_idx;

    return BLE_SUCCESS;
}


/*
 * The HCI_LE_Create_BIG_Test command should only be used for testing
 *  purposes
 *  note: BIG handle is allocated by host, BIS handle is allocated by controller
 */
ble_sts_t   blc_hci_le_createBigParamsTest(hci_le_createBigParamsTest_t* pCmdParam)
{
    /*
     * The parameters are not in the specified range, the Controller shall
     * return the error code Unsupported Feature or Parameter Value(0x11).
     */
    if(!blt_ll_isBigTestParamsValid(pCmdParam)){
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }
    /*
     * If the Controller cannot create all BISes of the BIG or if Num_BIS exceeds the
     * maximum value supported by the Controller, it shall return the error code
     * Connection Rejected due to Limited Resources (0x0D).
     */
    if(pCmdParam->num_bis > LL_BIS_IN_PER_BIG_BCST_NUM_MAX){ //per big_bcst: max bis num supported by Link Layer
        return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
    }
    if(pCmdParam->num_bis > blc_ll_getAvailBisNum(BIS_ROLE_BCST)){
        return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
    }

    /*
    * If the Advertising_Handle does not identify a periodic advertising train or the
    * periodic advertising train is associated with another BIG, the Controller shall
    * return the error code Unknown Advertising Identifier (0x42)
     */
    st_prd_adv_t *cur_pPerdadv = blt_ll_search_existing_perdAdv_index_by_advHandle(pCmdParam->adv_handle);
    if(cur_pPerdadv == NULL || (blt_ll_isPerdAdvEnable(pCmdParam->adv_handle)!=1) ||
                               (cur_pPerdadv->link_big_handle != BIG_HANDLE_INVALID)){//so link_big_handle must clean when BIG terminate
        return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
    }

    /*
     * If the Host issues this command with a BIG_Handle for a BIG that is already
     * created, the Controller shall return the error code Command Disallowed (0x0C).
     */
    if(blt_ll_searchExistingBigBcstHdl(pCmdParam->big_handle)!=BIG_HANDLE_INVALID){
        return HCI_ERR_CMD_DISALLOWED;
    }

    /*
     * Allocate new Big handle
     */
    u8 big_idx = blt_ll_AllocateNewBigBcstHdl(pCmdParam->big_handle);
    if(big_idx == BIG_HANDLE_INVALID){//update latest_pBigBcst if allocate successfully
        return HCI_ERR_MEM_CAP_EXCEEDED;
    }


    /*
     * If the value of the NSE parameter is not an integer multiple of BN, or NSE is
     * less than (IRC * BN), or the parameters are not in the specified range, the
     * Controller shall return the error code Unsupported Feature or Parameter Value (0x11).
     * If IRC = GC then PTO shall be ignored. Otherwise PTO shall be greater than zero.
     */
    if((pCmdParam->nse%pCmdParam->bn !=0) || (pCmdParam->nse < pCmdParam->bn * pCmdParam->irc) || \
      ((pCmdParam->nse/pCmdParam->bn != pCmdParam->irc) && pCmdParam->pto == 0)){
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }


    /*
     * If the Host sets PHY to a value that the Controller does not support, including a
     * bit that is reserved for future use, the Controller shall return the error code
     * Unsupported Feature or Parameter Value (0x11).
     */
    if(!(pCmdParam->phy&0x07) || (pCmdParam->phy&0xF8)){
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }


    //Associate Periodic ADV train with big_handle
    cur_pPerdadv->link_big_handle = pCmdParam->big_handle;
    cur_pPerdadv->big_idx = big_idx;


//  latest_pBigBcst->big_role = BIS_ROLE_BCST;//Broadcast BIS
    latest_pBigBcst->adv_handle = pCmdParam->adv_handle;
    latest_pBigBcst->bis_cnt = pCmdParam->num_bis;
    latest_pBigBcst->sdu_intvl = MAKE_U24(pCmdParam->sdu_intvl[2], pCmdParam->sdu_intvl[1], pCmdParam->sdu_intvl[0]); //unit: us
    latest_pBigBcst->iso_itvl = pCmdParam->iso_intvl; //unit: 1.25 ms, Time Range: 5 ms to 4 s
    latest_pBigBcst->nse = pCmdParam->nse;
    latest_pBigBcst->max_sdu = pCmdParam->max_sdu;
    latest_pBigBcst->max_pdu = pCmdParam->max_pdu;
    latest_pBigBcst->phy = pCmdParam->phy & 0x07;
    latest_pBigBcst->packing = pCmdParam->packing; //0x00 Sequential, 0x01 Interleaved
    latest_pBigBcst->framing = pCmdParam->framing; //0x00 Unframed,   0x01 Framed
    latest_pBigBcst->bn = pCmdParam->bn;           //The number of new payloads in each interval for each BIS.
    latest_pBigBcst->irc = pCmdParam->irc;         //The number of times the scheduled payload(s) are transmitted in a given event.
    latest_pBigBcst->pto = pCmdParam->pto;         //Offset used for pre-transmissions
    if(pCmdParam->nse/pCmdParam->bn == pCmdParam->irc){ /* If IRC = GC then PTO shall be ignored. */
        latest_pBigBcst->pto = 0;
    }

    latest_pBigBcst->encryption = pCmdParam->enc;  //0x00 Unencrypted, 0x01 Encrypted
    //broadcast_code: little--endian ==> big--endian
    swapX(pCmdParam->broadcast_code, latest_pBigBcst->broadcast_code, 16); //The code used to derive the session key that is used to encrypt and decrypt BIS payloads.

    latest_pBigBcst->seedAccessAddress = blt_ll_bis_getSeedAccessAddr();
    generateRandomNum(2, (u8*)&latest_pBigBcst->baseCrcInit); //generate random for baseCrcInit

    /* The Access Address for each BIS and for the BIG Control logical link (see
     * Section 4.4.6.7) in a BIG shall be derived from the SAA for that BIG.
     * For each BIS logical transport, the Access Address shall be equal to the SAA
     * bit-wise XORed with a diversifier word (DW) for that logical transport derived
     * from a Diversifier (D) as follows:
     *      D = ((35 * n) + 42) MOD 128 where n is the BIS number, or 0 for the BIG Control logical link
     *      DW = 0bD0D0D0D0D0D0D1D6_D10D5D40D3D20_00000000_00000000 */
    latest_pBigBcst->scAccessCode = blt_ll_bis_getAccessCode(latest_pBigBcst->seedAccessAddress, 0); //big control AccessCode
    /* For every Broadcast Isochronous PDU, the shift register shall be preset with the
     * BaseCRCInit value from the BIGInfo data (see Section 4.4.6.11) in the most
     * significant 2 octets and the BIS_Number for the specific BIS in the least
     * significant octet. For BIG Control PDUs, the least significant octet shall be 0. */
    latest_pBigBcst->scCrcInit    = (latest_pBigBcst->baseCrcInit << 8) | 0;


    //  2M PHY   :     (rf_len + 11) * 4
    // Coded PHY :  = 376 + (rf_len*8+43)*S
    u32 tx_max_us;
    u32 ctrl_pdu_length_us; //TX max PDU translate time, uS
    u8  curBisPhy;
    u8  coding_ind = bltPHYs.dft_CI ? bltPHYs.dft_CI : LE_CODED_S8;
    u8 ctrl_pdu_len_max = 8; //max(sizeof(big_chmInd_data_t), sizeof(big_termInd_data_t));

    u8 micLen = pCmdParam->enc ? 4: 0;

    if(latest_pBigBcst->phy & PHY_PREFER_1M){
        curBisPhy = BLE_PHY_1M; //The Controller only supports asymmetric PHYs.
        tx_max_us = (latest_pBigBcst->max_pdu + micLen + 10) * 8; //mic 4 len
        ctrl_pdu_length_us = (ctrl_pdu_len_max + micLen + 10) * 8;           //mic 4 len  : 2120us

    }
    else if(latest_pBigBcst->phy & PHY_PREFER_2M){
        curBisPhy = BLE_PHY_2M;
        tx_max_us = (latest_pBigBcst->max_pdu + micLen + 11) * 4; //mic 4 len
        ctrl_pdu_length_us = (ctrl_pdu_len_max + micLen + 11) * 4;           //mic 4 len

    }
    else{
        curBisPhy = BLE_PHY_CODED;  //dft: CODED_PHY_PREFER_S8
        tx_max_us = 376 + ((latest_pBigBcst->max_pdu + micLen)*64 + 43) * coding_ind; //mic 4 len
        ctrl_pdu_length_us =  376 + ((ctrl_pdu_len_max + micLen)*64 + 43) * coding_ind;             //mic 4 len
    }



    if(bltBisMng.bisBcst_ctrl_pdu_disable){
        ctrl_pdu_length_us = 0;
    }


    u16 tx_prepare_us = TX_STL_ADV_REAL_COMMON;
    latest_pBigBcst->bis_distance_schToAir_us = TLK_TX_TRIG_OFFSET + tx_prepare_us + 4;

    latest_pBigBcst->curBisPhy = curBisPhy;
    latest_pBigBcst->codingInd = coding_ind;

    u32 bis_spacing_us;
    u32 sub_interval_us;
    u32 big_iso_pdu_length_total_us;
    //Calculate BIS_Spacing && Sub_Interval: Both BIS_Spacing and Sub_Interval shall be at least T_MSS + MPT..
    if(latest_pBigBcst->packing == PACK_SEQUENTIAL){ // sequential
        //For sequential arrangement: BIS_Spacing >= NSE * Sub_Interval
        sub_interval_us = tx_max_us + TLK_T_MSS;
        bis_spacing_us = sub_interval_us * latest_pBigBcst->nse;
        //For sequential arrangement: BIG_Control_Offset = Num_BIS * BIS_Spacing
        big_iso_pdu_length_total_us = bis_spacing_us * latest_pBigBcst->bis_cnt;
    }
    else{ //interleaved
        //For interleaved arrangement: Sub_Interval >= Num_BIS * BIS_Spacing
        bis_spacing_us = tx_max_us + TLK_T_MSS;
        sub_interval_us = bis_spacing_us * latest_pBigBcst->bis_cnt;
        //For interleaved arrangement: BIG_Control_Offset = NSE * Sub_Interval
        big_iso_pdu_length_total_us = sub_interval_us * latest_pBigBcst->nse;
    }

    tlkapi_send_string_data(DBG_IAL_EN,"bis_spacing_us", &bis_spacing_us, 4);
    tlkapi_send_string_data(DBG_IAL_EN,"sub_interval_us", &sub_interval_us, 4);


    //The BIG_Sync_Delay = (Num_BIS - 1) * BIS_Spacing + (NSE - 1) * Sub_Interval + MPT.
    latest_pBigBcst->big_sync_delay_us = (latest_pBigBcst->bis_cnt - 1) * bis_spacing_us + (latest_pBigBcst->nse - 1) * sub_interval_us + tx_max_us;
    latest_pBigBcst->iso_pdu_task_us = big_iso_pdu_length_total_us;
    latest_pBigBcst->MPT = tx_max_us;
    latest_pBigBcst->SCPT = ctrl_pdu_length_us;
    latest_pBigBcst->se_length_tick    = (tx_max_us + TLK_T_MSS) * SYSTEM_TIMER_TICK_1US;

    //latest_pBigBcst->bigEventCnt = 0; //u64
    latest_pBigBcst->bigEventCnt = 0;

    smemcpy(latest_pBigBcst->chnParam.map.chmTbl, blmhostChnClassUpt.gLlChannelMap, 5);

    latest_pBigBcst->chnIdentifier = (latest_pBigBcst->scAccessCode>>16) ^ (latest_pBigBcst->scAccessCode&0xffff);
    csa2_calculateMapInfo(&latest_pBigBcst->chnParam.map);


    //////////////////////////////////  Assemble BIG Info begin //////////////////////////////////
    latest_pBigBcst->BigInfor.bigOffset = 600;                                      //The time from the start of the packet containing the BIGInfo to the next BIG anchor point. bigOffset >= 600us
    latest_pBigBcst->BigInfor.bigOffsetUnits = BIG_PDU_BIG_OFFSET_UNITS_30_US;  //The actual time offset is determined by multiplying the value of BIG_Offset by the unit. unit: 300us or 30us

    latest_pBigBcst->BigInfor.isoItvl = latest_pBigBcst->iso_itvl;                  //ISO_Interval is the time between two adjacent BIG anchor points, in units of 1.25 ms. The value shall be between 4 and 3200 (i.e. 5 ms to 4 s).
    latest_pBigBcst->BigInfor.numBis = latest_pBigBcst->bis_cnt;                    //The Num_BIS field shall contain the number of BISes in the BIG.
    latest_pBigBcst->BigInfor.nse = latest_pBigBcst->nse;                           //NSE is the number of subevents per BIS in each BIG event. The value shall be between 1 and 31 and shall be an integer multiple of BN.
    latest_pBigBcst->BigInfor.bn = latest_pBigBcst->bn;                             //The value of BN shall be between 1 and 7.
    latest_pBigBcst->BigInfor.subItvl = sub_interval_us;                            //Sub_Interval is the time between the start of two consecutive subevents of each BIS.
    latest_pBigBcst->BigInfor.pto = latest_pBigBcst->pto;                           //The value of PTO shall be between 0 and 15.
    latest_pBigBcst->BigInfor.bisSpacing = bis_spacing_us;                          //BIS_Spacing is the time between the start of corresponding subevents in adjacent BISes in the BIG and also the time between the start of the first
                                                                                    //subevent of the last BIS and the control subevent, if present.
    latest_pBigBcst->BigInfor.irc = latest_pBigBcst->irc;                           //The value of IRC shall be between 1 and 15.
    latest_pBigBcst->BigInfor.maxPdu = latest_pBigBcst->max_pdu;                    //Max_PDU is the maximum number of data octets (excluding the MIC, if any) that can be carried in each BIS Data PDU in the BIG. The value shall be
                                                                                    //between 0 and 251 octets.
    latest_pBigBcst->BigInfor.seedAA = latest_pBigBcst->seedAccessAddress;          //The SeedAccessAddress field shall contain the Seed Access Address for the BIG
    latest_pBigBcst->BigInfor.sduItvl = latest_pBigBcst->sdu_intvl;             //Sub_Interval is the time between the start of two consecutive subevents of each BIS.
    latest_pBigBcst->BigInfor.maxSdu = latest_pBigBcst->max_sdu;                    //Max_SDU is the maximum size of an SDU on this BIG
    latest_pBigBcst->BigInfor.baseCrcInit = latest_pBigBcst->baseCrcInit;
    smemcpy(latest_pBigBcst->BigInfor.chm37Phy3, blmhostChnClassUpt.gLlChannelMap, 5); //The ChM field shall have the same meaning as the corresponding field in the CONNECT_IND PDU
    latest_pBigBcst->BigInfor.chm37Phy3[4] |= ((curBisPhy-1) << 5);                 //The PHY field shall be set to indicate the PHY used by the BIG.0 LE 1M PHY; 1 LE 2M PHY; 2 LE Coded PHY
    //u64 bisPldCntr = latest_pBigBcst->bigEventCnt * latest_pBigBcst->bn;
    u32 bisPldCntr = latest_pBigBcst->bigEventCnt * latest_pBigBcst->bn;            //bisPayloadCounter belong to [bigEventCounter * BN, (bigEventCounter + 1) * BN - 1]
    smemcpy(latest_pBigBcst->BigInfor.bisPldCnt39Framing1, &bisPldCntr, 4);         //The value shall be for the first subevent of the BIG event referred to by the BIG_Offset field
    //NOTE: bisPldCntr use u32 Not u64, so here should not use "|"
    //latest_pBigBcst->BigInfor.bisPldCnt39Framing1[4] |= (latest_pBigBcst->framing << 7);
    latest_pBigBcst->BigInfor.bisPldCnt39Framing1[4] = (latest_pBigBcst->framing << 7); //The Framing bit shall be set if the BIG carries framed data.
    if(latest_pBigBcst->encryption){
        generateRandomNum(8, latest_pBigBcst->BigInfor.giv);   //generate random for GIV  //GIV:  little--endian
        generateRandomNum(16, latest_pBigBcst->BigInfor.gskd); //generate random for GSKD //GSKD: little--endian

        cur_pPerdadv->acad_field_len = 57;

        ///////////// Calculate the BIG/BIS session key //////////////////
        //IGLTK = h7("BIG1", Broadcast_Code)
        //GLTK = h6(IGLTK, "BIG2")
        //GSK = h8 (GLTK, GSKD, "BIG3")
        //GSK is used as the session key in the CCM algorithm when encrypting the BIG.
        u8 tmp_igltk[16], tmp_gltk[16], tmp_gskd[16];
        u8 BIG1[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x42,0x49,0x47,0x31};    //big-endian
        u8 BIG2[4]  = {0x42,0x49,0x47,0x32};                                                                //big-endian
        u8 BIG3[4]  = {0x42,0x49,0x47,0x33};                                                                //big-endian

        swapX(latest_pBigBcst->BigInfor.gskd, tmp_gskd, 16);                            //GSKD: little--endian ==> big-endian
        blt_crypto_alg_h7 (tmp_igltk, BIG1, latest_pBigBcst->broadcast_code);           //Broadcast_Code: big-endian
        blt_crypto_alg_h6 (tmp_gltk, tmp_igltk, BIG2);
        blt_crypto_alg_h8 (latest_pBigBcst->bigCtrlCrypt.sk, tmp_gltk, tmp_gskd, BIG3); //Our SDK's AES CCM's sk need big--endian.
        //tlkapi_send_string_data(0,"BIG/BIS SK", latest_pBigBcst->bigCtrlCrypt.sk, 16);

        ///////////// Calculate the BIG CTRL IV key //////////////////
        //The IV for a CIS or BIS is calculated from an IVbase and the Access Address of the CIS or BIS respectively.
        //Generation of IV for a CIS or BIS: IV[31:0] shall equal IVbase[31:0] XORed with the Access Address of the
        //CIS or BIS while IV [63:32] shall equal IVbase[63:32].
        //The IV for a BIG control logical link shall be determined in the same way as for a BIS.
        u32 iv[2] = { 0};
        //Initialize the IV          //IV: little--endian, our SDK's AES CCM's iv need little--endian
        smemcpy(&iv, latest_pBigBcst->BigInfor.giv, 8); //For a BIS, the IVbase shall be set to the value of GIV contained in the BIGInfo.
        iv[0] ^= latest_pBigBcst->scAccessCode;         //Generation of IV for a CIS or BIS
        smemcpy(latest_pBigBcst->bigCtrlCrypt.nonce.iv, &iv, 8);
        //tlkapi_send_string_data(0,"BIG CTRL IV", latest_pBigBcst->bigCtrlCrypt.nonce.iv, 8);

        //Open BIG Control PDU encryption enable flg
        latest_pBigBcst->bigCtrlCrypt.enable = 1; //Enable encryption
        latest_pBigBcst->bigCtrlCrypt.mic_fail = 0;
    }
    else{
        smemset(latest_pBigBcst->broadcast_code, 0, 16);      //clear Broadcast Code
        smemset(latest_pBigBcst->BigInfor.giv, 0, 8);   //clear GIV
        smemset(latest_pBigBcst->BigInfor.gskd, 0, 16); //clear GSKD
        latest_pBigBcst->bigCtrlCrypt.enable = 0;

        cur_pPerdadv->acad_field_len = 33;
    }

    cur_pPerdadv->acad[0] = cur_pPerdadv->acad_field_len+1;
    /*
     * https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile/
     */
    cur_pPerdadv->acad[1] = DT_BIGINFO; //adv type: 0x2C    <<BIGInfo>>
    smemcpy(cur_pPerdadv->acad+2,  (u8*)&latest_pBigBcst->BigInfor, cur_pPerdadv->acad_field_len);
    cur_pPerdadv->acad_field_len += 2; //len + type

    cur_pPerdadv->acad_used &= ~PERD_ACAD_BIGINFO_ENA; //Aux_Sync_Ind { + ACAD(BIGInfo) : after BIG task begin, we will enable ACAD later!!! }

    //////////////////////////////////  Assemble BIG Info end //////////////////////////////////




    ///// find available BIS for current BIG ///////////////////
    int new_bis_cnt = 0;
    ll_bis_t *cur_pBis = NULL;
    u8 bis_order_tbl[LL_BIS_IN_PER_BIG_BCST_NUM_MAX];
    u8 bis_order_cnt = 0;
    latest_pBigBcst->bis_alloc_msk = 0;

    for(int i = 0; i< bltBisMng.maxNum_bisBcst; i++){
        cur_pBis = (ll_bis_t *) (global_pBis + i);

        if(cur_pBis->bis_occupied == 0){
            cur_pBis->bis_occupied = 1;
            bltBisMng.curNum_bisBcst ++;  //update current BIS number

            cur_pBis->link_big_handle = pCmdParam->big_handle;
            cur_pBis->big_idx = big_idx;
            cur_pBis->bis_dapth_setup = 0;

            latest_pBigBcst->bis_handle[new_bis_cnt] = cur_pBis->bis_handle;

//          cur_pBis->curBisPhy = curBisPhy;
//          cur_pBis->codingInd = coding_ind; //S2 or S8: LE_CODED_S2 / LE_CODED_S8
            cur_pBis->sub_interval_us   = sub_interval_us;
            cur_pBis->sub_interval_tick = sub_interval_us * SYSTEM_TIMER_TICK_1US;
            cur_pBis->bis_spacing_us    = bis_spacing_us;
            cur_pBis->bis_spacing_tick  = cur_pBis->bis_spacing_us * SYSTEM_TIMER_TICK_1US;



//          cur_pBis->bisEventCnt = 0;
            cur_pBis->bisPduTxFifoWptr = cur_pBis->bisPduTxFifoRptr = 0;

            cur_pBis->nse = pCmdParam->nse;
            cur_pBis->bn = pCmdParam->bn;
            cur_pBis->irc = pCmdParam->irc;
            cur_pBis->pto = pCmdParam->pto;

            cur_pBis->lastEventCnt = 0;
            cur_pBis->lastPayloadNum    = 0;
            cur_pBis->curBisPldNum = 0;
            cur_pBis->txBnIdx =0;
            cur_pBis->txSduIdx = 0;
            cur_pBis->tx_first_pdu = 1;
            cur_pBis->bisSduIn_wptr = cur_pBis->bisSduIn_rptr = 0;
            cur_pBis->bisSduInFreeNum = sduBisMng.in_fifo_num;
            cur_pBis->lossFlag = 0;
            cur_pBis->numSdu2Pdu = ((latest_pBigBcst->bn * latest_pBigBcst->sdu_intvl) + (latest_pBigBcst->iso_itvl*1250) -1 )/(latest_pBigBcst->iso_itvl*1250);
            cur_pBis->numSduEachEvent = (latest_pBigBcst->iso_itvl*1250 + latest_pBigBcst->sdu_intvl -1)/(latest_pBigBcst->sdu_intvl); //(latest_pBigBcst->sdu_intvl + latest_pBigBcst->iso_itvl*1250 -1)/(latest_pBigBcst->iso_itvl*1250);
            bis_order_tbl[bis_order_cnt++] = i;
            latest_pBigBcst->bis_alloc_msk |= BIT(i);

            new_bis_cnt++; //0 for BIG Control, other s for BIS use
            cur_pBis->bisAccessAddr = blt_ll_bis_getAccessCode(latest_pBigBcst->seedAccessAddress, new_bis_cnt); //bisAccessCode   sample data 0x8E89BED6; //
            cur_pBis->bisCrcInit = (latest_pBigBcst->baseCrcInit << 8) | new_bis_cnt;

            cur_pBis->chnIdentifier = (cur_pBis->bisAccessAddr >> 16) ^ (cur_pBis->bisAccessAddr >> 0);

            //////////////////////////////////////////////////
            // Encryption parameters init
            //////////////////////////////////////////////////
            if(latest_pBigBcst->encryption){
                //The IV for a CIS or BIS is calculated from an IVbase and the Access Address of the CIS or BIS respectively.
                //Generation of IV for a CIS or BIS: IV[31:0] shall equal IVbase[31:0] XORed with the Access Address of the
                //CIS or BIS while IV [63:32] shall equal IVbase[63:32].
                u32 iv[2] = { 0};
                //Initialize the IV          //BIS IV: little--endian, our SDK's AES CCM's iv need little--endian
                smemcpy(&iv, latest_pBigBcst->BigInfor.giv, 8);//For a BIS, the IVbase shall be set to the value of GIV contained in the BIGInfo.
                iv[0] ^= cur_pBis->bisAccessAddr; //Generation of IV for a CIS or BIS
                smemcpy(cur_pBis->bisCryptCtrl.nonce.iv, &iv, 8);
                //tlkapi_send_string_data(0,"IV", cur_pBis->bisCryptCtrl.nonce.iv, 8);

                //Initialize the session key //BIS's SK equal to BIG CTRL's SK: big--endian, our SDK's AES CCM's sk need big--endian
                smemcpy (cur_pBis->bisCryptCtrl.sk, latest_pBigBcst->bigCtrlCrypt.sk, 16);

                //Open BIS encryption enable flg
                cur_pBis->bisCryptCtrl.enable = 1; //Enable encryption
                cur_pBis->bisCryptCtrl.mic_fail = 0;
            }
            else{
                cur_pBis->bisCryptCtrl.enable = 0;
            }

            sdu_packet_t *bis_sdu;
            for(int n = 0; n<sduBisMng.in_fifo_num; n++){
                bis_sdu =(sdu_packet_t*) (sduBisMng.in_fifo_b + sduBisMng.max_in_fifo_size * (n + i*sduBisMng.in_fifo_num));
                bis_sdu->sduOffset = 0;
            }

            #if (HCI_SEND_NUM_OF_CMP_AFT_ACK)
                cur_pBis->nocAckNum = 32;
                cur_pBis->nocAckMsk = 32-1;
                cur_pBis->nocAckWptr = cur_pBis->nocAckRptr = 0;
                smemset(cur_pBis->nocAclTxWptr, 0, 32*2);
            #endif

            if(new_bis_cnt >= pCmdParam->num_bis){
                break;
            }
        }
    }


    tlkapi_send_string_data(DBG_IAL_EN,"max_sdu", &latest_pBigBcst->max_sdu, 2);
    tlkapi_send_string_data(DBG_IAL_EN,"max_pdu", &latest_pBigBcst->max_pdu, 2);
    tlkapi_send_string_data(DBG_IAL_EN,"numSdu2Pdu", &cur_pBis->numSdu2Pdu, 1);
    tlkapi_send_string_data(DBG_IAL_EN,"numSduEachEvent", &cur_pBis->numSduEachEvent, 1);

    ////////////////////  BIS arrangement map ////////////////////////
    latest_pBigBcst->bis_total_se_num = 0;
    for(int i = 0; i < latest_pBigBcst->bis_cnt; i++){
        if(latest_pBigBcst->packing == PACK_SEQUENTIAL){ //Sequential: e.g.: 112233
            for(int j=0; j<latest_pBigBcst->nse ;j++){
                latest_pBigBcst->bis_arrgmt_map[latest_pBigBcst->bis_total_se_num + j] = bis_order_tbl[i];
            }
        }
        else{ //== PACK_INTERLEAVED  //Interleaved: e.g.: 123123
            for(int j=0; j<latest_pBigBcst->nse ;j++){ //NSE same for all BISes
                latest_pBigBcst->bis_arrgmt_map[i + j * latest_pBigBcst->bis_cnt] = bis_order_tbl[i];
            }
        }

        latest_pBigBcst->bis_total_se_num += latest_pBigBcst->nse;
    }

    /*  Refer to Core 5.2 | Vol 6, Part G, Page 3220  */
    if(latest_pBigBcst->framing == BIS_UNFRAMED){
        /*
         * Transport_Latency = BIG_Sync_Delay + (PTO * (NSE / BN - IRC) + 1) * ISO_Interval - SDU_Interval
         */
        latest_pBigBcst->transLatency_us = latest_pBigBcst->big_sync_delay_us +
                                           ((latest_pBigBcst->pto*(latest_pBigBcst->nse/latest_pBigBcst->bn - latest_pBigBcst->irc)) + 1) * (latest_pBigBcst->iso_itvl*1250) -
                                           latest_pBigBcst->sdu_intvl;
    }
    else{ /* FRAMED */
        /*
         *Transport_Latency_BIG = BIG_Sync_Delay + (PTO * (NSE / BN - IRC) + 1) * ISO_Interval + SDU_Interval
         */
        latest_pBigBcst->transLatency_us = latest_pBigBcst->big_sync_delay_us +
                                           ((latest_pBigBcst->pto*(latest_pBigBcst->nse/latest_pBigBcst->bn - latest_pBigBcst->irc)) + 1) * (latest_pBigBcst->iso_itvl*1250) +
                                           latest_pBigBcst->sdu_intvl;
    }

    //////////////////////////// BIG slot Timing && task create Start //////////////////////////
    u32 r = irq_disable();

    latest_pBigBcst->sSlot_duration_big = (big_iso_pdu_length_total_us + ctrl_pdu_length_us + SLOT_PROCESS_MAX_US
            + latest_pBigBcst->bis_distance_schToAir_us + TLK_TM_DELAY )*SSLOT_US_REVERSE + 1;
    latest_pBigBcst->sSlot_interval_big = BSLOT_DUR_2_SSLOT_DUR(latest_pBigBcst->iso_itvl*2);  //1.25mS -> 625 uS -> 19.53us

    irq_restore(r);

    /* Priority preset value */
    blt_ll_set_interval_level(TSKOFT_BIG_BCST + big_idx, latest_pBigBcst->iso_itvl);
    blt_ll_setSchedulerTaskPriority(TSKOFT_BIG_BCST + big_idx, TASK_PRIORITY_BIG_BCST_DFT);

    ///////////////////////////// BIG slot Timing && task create End ///////////////////////////






    latest_pBigBcst->cmd_status = BIG_CREATE_PENDING;//pending
    blmsParam.big_sche_build_pending = BIG_SLOT_BUILD_MSK | big_idx;

    return BLE_SUCCESS;
}


/*
 * The HCI_LE_Terminate_BIG command is used to terminate a BIG identified by
 * the BIG_Handle parameter.The command also terminates the transmission of
 * all BISes of the BIG, destroys the associated connection handles of the BISes
 * in the BIG and removes the data paths for all BISes in the BIG.
 */
ble_sts_t   blc_hci_le_terminateBig(hci_le_terminateBigParams_t* pCmdParam)
{
    /*
     * If the Synchronized Receiver Host issues this command, the Controller shall
     * return the error code Command Disallowed (0x0C).
     */
    if(ll_big_sync_mlp_task_cb && (ll_big_sync_mlp_task_cb(FLAG_BIG_SYNC_HANDLE_SEARCH, (void*)(&pCmdParam->big_handle)) != BIG_HANDLE_INVALID))
    {//blt_ll_findExistingBigSyncByBigHdl
        return HCI_ERR_CMD_DISALLOWED;
    }
    /*
     * If the Host attempts to terminate the BIG with a BIG_Handle that does not
     * exist, the Controller shall return the error code Unknown Advertising Identifier (0x42).
     */
    u8 big_idx;
    if((big_idx = blt_ll_searchExistingBigBcstHdl(pCmdParam->big_handle)) == BIG_HANDLE_INVALID){
        return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
    }

    /* If it is already being processed, return directly */
    if(latest_pBigBcst->big_sc_mask & BIG_SC_TERM_IND){
        return HCI_ERR_CMD_DISALLOWED;
    }

    /*
     * If the Host attempts to terminate a BIG while the process of establishment of
     * the BIG is in progress (i.e. HCI_LE_Create_BIG_Complete event has not been
     * generated) the process of establishment shall stop and the Controller shall
     * generate the HCI_LE_Create_BIG_Complete event to the Host with the error
     * code Operation Cancelled by Host (0x44).
     */
    //u32 r = irq_disable();

    if(latest_pBigBcst->cmd_status == BIG_CREATE_PENDING){
        latest_pBigBcst->cmd_status = BIG_IN_IDLE;

        if(blmsParam.big_sche_build_pending){
            blmsParam.big_sche_build_pending = 0;

            st_prd_adv_t *cur_pPerdadv = blt_ll_search_existing_perdAdv_index_by_advHandle(latest_pBigBcst->adv_handle);
            if(cur_pPerdadv != NULL){
                cur_pPerdadv->acad_chaged = 1;
                blt_prdadv_updateAcadPram(cur_pPerdadv, PERD_ACAD_BIGINFO_DIS);
                cur_pPerdadv->acad_chaged = 2;

                cur_pPerdadv->link_big_handle = BIG_HANDLE_INVALID;
            }
        }

        u32 r = irq_disable();
        blt_sche_removeTaskMask(TSKMSK_BIG_BCST_0 << big_idx);
        blt_sche_addUpdate(SLOT_UPDT_BIS_BCST_REMOVE);
        irq_restore(r);

        /// After placing hci_le_createBigComplete_evt, destroy the BIG/BIS control block ///
        latest_pBigBcst->big_create_cmp_evt = BIG_CREATE_CANCELED;
    }

    //How to ensure 6 consecutive pens, you need to consider the priority of BIG task
    latest_pBigBcst->big_term_inst = (u16)(latest_pBigBcst->bigEventCnt + 18); //in future at least 6,  [8(consecutive_ISO_itvl] + 8 + 2(margin)]

    blt_sche_addUpdate(SLOT_UPDT_SLOTTBL_RESCHED); //Rebuild sch task table ASAP.
    #if(BIS_ADV_EBQ)//add_qw; big terminate control PDU need 6 consecutive,not jump.
        blt_ll_setSchedulerTaskPriority(TSKOFT_BIG_BCST + big_idx, TASK_PRIORITY_MAX);
    #endif
    
    //irq_restore(r);

    if(latest_pBigBcst->big_sc_mask == 0){
        latest_pBigBcst->big_sc_send_cnt = 0;
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"big_sc_send_cnt 0", 0, 0);
    }

    latest_pBigBcst->big_term_rsn = pCmdParam->reason;
    latest_pBigBcst->big_sc_mask |= BIG_SC_TERM_IND;

    tlkapi_send_string_data(DEB_BIG_BCST_EN, "LE BIG_TERMINATE_IND: inst", &latest_pBigBcst->big_term_inst, 2);

    return BLE_SUCCESS;
}


_attribute_ram_code_
static int  blt_ll_find_next_bisBcst_subevent (int start_idx)
{
    int i = 100;  //important

    if(start_idx < blt_pBigBcst->bis_total_se_num){
        //Get the valid BIS
        blt_bis_sel = blt_pBigBcst->bis_arrgmt_map[start_idx];
        blt_pBis = (ll_bis_t*)(global_pBis + blt_bis_sel); //The first valid BIS

        //Update the starting index of the current BIS arrangement map to the next IDX of the currently valid IDX.
        blt_pBigBcst->bis_arrgmt_next_idx = start_idx + 1;

        return start_idx;
    }

    return i;
}


_attribute_ram_code_
int         blt_bigBcst_start(int slotTask_idx)
{
    DBG_CHN6_HIGH;
#if (SL01_big_bcst)
    log_task_begin_irq(SL_STACK_BIS_SOURCE_TIMING_EN, SL01_big_bcst);
#endif

    //BIG split into several bis's BTX
    blt_bigBcst_sel = slotTask_idx;

    //1.First locate the BIG that belongs to
    blt_pBigBcst = (ll_big_bcst_t*)(global_pBigBcst + blt_bigBcst_sel);
    blt_pBigBcst->bis_arrgmt_next_idx = 0;

    SubEvtStepTick = 0;
    //int fst_nse_idx =
    blt_ll_find_next_bisBcst_subevent(0);  //must 0, attention: will update blt_pBis &  blt_bis_sel

    /* BIG slot skipped, need compensation for every BIS belong to the BIG */
    int inter_jump_num = (bltSche.sSlot_idx_irq_real + BSLOT_DUR_2_SSLOT_DUR(4) - blt_pBigBcst->sSlot_mark_big)/blt_pBigBcst->sSlot_interval_big - 1;

    //important! ! ! Compensation for X BIG Events skipped due to timing conflicts.
    if(blt_pBigBcst->bigEventCnt && inter_jump_num > 0){
        blt_pBigBcst->bigEventCnt += inter_jump_num;

        if( !(blt_pBigBcst->big_sc_mask & BIG_SC_TERM_IND) && !(blt_pBigBcst->big_sc_mask & BIG_SC_CHM_IND) ){
            blt_ll_incSchedulerTaskPriority( TSKOFT_BIG_BCST + blt_bigBcst_sel, bltPri.step_final[TSKOFT_BIG_BCST + blt_bigBcst_sel]*2*inter_jump_num );
        }
    }

    for(int i = 0; i< bltBisMng.maxNum_bisBcst; i++){
        ll_bis_t *cur_pBis = (ll_bis_t *) (global_pBis + i);
        cur_pBis->bisSubEventCnt = 0;//reset current SE index

        for(int j = 1; j<= cur_pBis->nse; j++){//NSE = 5,   35-40us,  NSE=16,   71us
            cur_pBis->subEventChnIdx[j-1] = blt_ll_generateNextChannel(&blt_pBigBcst->chnParam, blt_pBigBcst->bigEventCnt,
                    cur_pBis->chnIdentifier, j);
        }
        cur_pBis->ctrlSubEventChnIdx = blt_ll_generateNextChannel(&blt_pBigBcst->chnParam, blt_pBigBcst->bigEventCnt,
                    blt_pBigBcst->chnIdentifier, 1);
    }   blt_pBigBcst->big_start_tick = clock_time()|1;

    //  if(bltSche.pTask_cur->scheTask_flg & TSKFLG_BSLOT_ALIGN) //save RamCode, this must be 1
    {
        blt_pBigBcst->sSlot_mark_big = bltSche.sSlot_idx_irq_real; //update slot index mark;
        //blt_pBigBcst->sSlot_mark_big = bltSche.sSlot_tick_irq;   //not used now
    }

    rf_ble_switch_phy(blt_pBigBcst->curBisPhy, (le_coding_ind_t)blt_pBigBcst->codingInd);

    rf_ble_set_tx_settle(bltPHYs.tx_stl_adv); //attention: must set after PHY switch !!!

    //Enable The first BIS in the BIG event is enabled and calculate the first SE BCTX_POST time of the first BIS
    bctx_start_tick = bltSche.sSlot_tick_irq_real;  //must set before "blt_ctx_start"

    //Check if you need to send BIS Control Packet. //BIG Control Procedures: BIG_TERMINATE_IND or BIG_CHANNEL_MAP_IND
    blt_pBigBcst->big_sc_flg = blt_pBigBcst->big_sc_cstf = 0;

    if(blt_pBigBcst->big_sc_mask){

        bool big_sc_term = blt_pBigBcst->big_sc_mask & BIG_SC_TERM_IND;
        u16 big_sc_inst = big_sc_term ? blt_pBigBcst->big_term_inst : blt_pBigBcst->big_chm_inst;

        if(!blt_pBigBcst->big_sc_send_cnt){
            blt_pBigBcst->big_sc_cssn++;
        }

        if(++blt_pBigBcst->big_sc_send_cnt <= 8){ //Expecting IUT to send six Terminate Ind PDUs in six consecutive ISO Intervals
            /* The value of the CSSN of every BIS PDU in a BIG event shall be the same.
             * The Link Layer shall increment CSSN by 1 (with 7 wrapping to 0) at the start of
             * a BIG event that contains the first transmission of a new BIG Control PDU and
             * shall leave the CSSN unchanged otherwise */

            blt_pBigBcst->big_sc_cssn &= 7; //Refer to <<LL.TS>>: CSSN=N (where N can be any value between 0 and 7)

            blt_pBigBcst->big_sc_cstf = 1;
            blt_pBigBcst->big_sc_flg = BIG_SC_RDY2SEND_BISES;
            tlkapi_send_string_data(DEB_BIG_BCST_EN,"BIG start: SC flg = 1", 0, 0);
            //DBG_CHN7_TOGGLE;
        }

        //pay attention here, BIG slot may dropped, blt_pBigBcst->bigEventCnt >= big_sc_inst(consider 0xffff->0 problem, (u16).... < 1024 )
        if ((u16)(blt_pBigBcst->bigEventCnt + 1 - big_sc_inst) < BIT(10)){ //Note: Need to add an event to bigEventCnt, here -1
//      if(blt_pBigBcst->bigEventCnt == big_sc_inst - 1){
            if(big_sc_term){ // Need terminate BIG and it's BISes
                blt_pBigBcst->big_terminated = 1;
                //DBG_C HN8_TOGGLE;
                tlkapi_send_string_data(DEB_BIG_BCST_EN,"BIG terminate", 0, 0);
            }
            else if ((u16)(blt_pBigBcst->bigEventCnt - big_sc_inst) < BIT(10)){ // Need use new channel map table for CSA#2

                smemcpy(&blt_pBigBcst->chnParam.map, &blt_pBigBcst->nextChnParam, sizeof(struct le_channel_map));

                // Clear BIG SC CHM flag
                blt_pBigBcst->big_sc_mask &= ~BIG_SC_CHM_IND;
                blt_pBigBcst->big_chm_inst = 0;
                blt_pBigBcst->big_sc_send_cnt = 0;
            #if(BIS_ADV_EBQ) ////add_qw; when channel map update complete, restore priority.
                //Big control instant 6 consecutive sch task complete, restore priority.
                blt_ll_setSchedulerTaskPriority(TSKOFT_BIG_BCST + blt_bigBcst_sel, TASK_PRIORITY_BIG_BCST_DFT);//restore priority.
            #endif
                tlkapi_send_string_data(DEB_BIG_BCST_EN,"BIG use new channel map", blt_pBigBcst->chnParam.map.chmTbl, 5);
            }
        }
    }

    blt_bisBcst_tx_start();


    return 0;
}


_attribute_ram_code_
int         blt_bigBcst_post(void)
{
    blt_pBigBcst->bigEventCnt++;
    DBG_CHN6_LOW;

    blms_state = BLMS_STATE_BIG_E;

    for(int i =0; i<blt_pBigBcst->bis_cnt; i++){
        if(blt_pBigBcst->bis_alloc_msk & BIT(i)){
            ll_bis_t *pBis = (ll_bis_t *) (global_pBis + i);
            if((pBis->pBisTestParam!=NULL)&&(pBis->pBisTestParam->isoTestMode==ISO_TEST_TRANSMIT_MODE)
                    && (!pBis->pBisTestParam->tranMode.isoTestSendTick)){
                pBis->pBisTestParam->tranMode.isoTestSendTick = clock_time()|1;
            }
        }
    }

    blt_ll_calculate_sSlot_next(clock_time() + SLOT_PROCESS_MAX_TICK);

    #if (SL01_big_bcst)
        log_task_end_irq(SL_STACK_BIS_SOURCE_TIMING_EN, SL01_big_bcst);
    #endif

    return 0;
}


_attribute_ram_code_
static void blt_ll_prepareBigCtrlData(void)
{
    if(blt_pBigBcst->big_sc_mask & BIG_SC_TERM_IND){
        pCurrBisPdu= (rf_packet_ll_data_t*)gBisTermIndPdu;
        pCurrBisPdu->llPhysChnPdu.llPduHdr.bisPduHdr.rf_len = 4;
        pCurrBisPdu->llPhysChnPdu.llPduHdr.bisPduHdr.llid = 3;
        pCurrBisPdu->dma_len = rf_tx_packet_dma_len(6);
        big_termInd_data_t *pBigTerm = (big_termInd_data_t*)(pCurrBisPdu->llPhysChnPdu.llPayload);
        pBigTerm->opCode = BIG_TERMINATE_IND;
        pBigTerm->reason = blt_pBigBcst->big_term_rsn;
        pBigTerm->instant = blt_pBigBcst->big_term_inst;
        //tlkapi_send_string_data(DEB_BIG_BCST_EN,"prepare BIG term ind pkt", 0, 0);
    }
    else if(blt_pBigBcst->big_sc_mask & BIG_SC_CHM_IND){
        pCurrBisPdu= (rf_packet_ll_data_t*)gBisChmIndPdu;
        pCurrBisPdu->llPhysChnPdu.llPduHdr.bisPduHdr.rf_len = 8;
        pCurrBisPdu->dma_len = rf_tx_packet_dma_len(10);
        pCurrBisPdu->llPhysChnPdu.llPduHdr.bisPduHdr.llid = 3;
        big_chmInd_data_t *pBigChm = (big_chmInd_data_t*)(pCurrBisPdu->llPhysChnPdu.llPayload);
        pBigChm->opCode = BIG_CHANNEL_MAP_IND;
        smemcpy(pBigChm->chm, blt_pBigBcst->nextChnParam.chmTbl, 5); //TODO: Update big_chm_next's value By API: blc_ll_setHostChannel
        pBigChm->instant = blt_pBigBcst->big_chm_inst;
        //tlkapi_send_string_data(DEB_BIG_BCST_EN,"prepare BIG chm ind pkt", 0, 0);
    }
    else{
        tlkapi_send_string_data(DEB_BIG_BCST_EN,"Other BIG Control PDU, TODO", 0, 0);
    }

    //Encryption in IRQ, put the API in ramcode
    if(blt_pBigBcst->bigCtrlCrypt.enable){
        blt_ll_bis_encryptPld(&blt_pBigBcst->bigCtrlCrypt, pCurrBisPdu, blt_pBigBcst->bigEventCnt * blt_pBigBcst->bn);
    }
}


_attribute_ram_code_
static u8*  blt_ll_prepareBisData(ll_bis_t *pBis, bool big_sc_start)
{
    if(!big_sc_start){

        pCurrBisPdu = &gBisEmptyPdu; //dft: BIS Null PDU
        if(blt_pBigBcst->framing == BIS_FRAMED){
            pCurrBisPdu->llPhysChnPdu.llPduHdr.bisPduHdr.llid = 2;
        }
        else{
            pCurrBisPdu->llPhysChnPdu.llPduHdr.bisPduHdr.llid = 1;
        }

        u8 bisEvtJmpNum = 0;
        u32 curSendBisPldCtnr;

        pBis->incTxRptrFlag = 0;
        u8 curBisGrp = (pBis->bisSubEventCnt-1)/pBis->bn; //curBisGrp (g) is started from zero.
        u8 bisPldCntrOffset = pBis->bisSubEventCnt-1 - curBisGrp * pBis->bn;
        if(curBisGrp < pBis->irc){
            curSendBisPldCtnr = blt_pBigBcst->bigEventCnt * pBis->bn + bisPldCntrOffset;
            pBis->bisSendPldNum = curSendBisPldCtnr;
            tlkapi_send_string_data(DEB_BIG_BCST_EN,"curBisGrp < pBis->irc: curSendBisPldCtnr", &curSendBisPldCtnr, 4);
        }
        else{ //curBisGrp >= pBis->irc
            bisEvtJmpNum = pBis->pto * (curBisGrp - pBis->irc + 1);
            curSendBisPldCtnr = (blt_pBigBcst->bigEventCnt + bisEvtJmpNum) * pBis->bn + bisPldCntrOffset;
            tlkapi_send_string_u32s(DEB_BIG_BCST_EN,"jump",bisEvtJmpNum,curBisGrp,bisPldCntrOffset,blt_pBigBcst->bigEventCnt);
            tlkapi_send_string_data(DEB_BIG_BCST_EN,"curBisGrp >= pBis->irc: curSendBisPldCtnr", &curSendBisPldCtnr, 4);
        }

    #if(HW_AES_CCM_ALG_EN)
        if(pBis->bisCryptCtrl.enable)
        {
            blt_ll_setAesCcmPara(0, pBis->bisCryptCtrl.sk, pBis->bisCryptCtrl.nonce.iv, 0xc3,\
                    curSendBisPldCtnr, 0, 0);
        }
    #endif

        pBis->curBisPldNum = max(curSendBisPldCtnr, pBis->curBisPldNum);

        /*
         * 1.The payloadNum corresponding to the current data taken from the buff cannot be smaller than the locally maintained send payloadNum
         */
        for(int i = (pBis->bisPduTxFifoRptr&bltBisPduTxfifo.mask); i != (pBis->bisPduTxFifoWptr&bltBisPduTxfifo.mask); i++, i &= bltBisPduTxfifo.mask)
        {
            bis_tx_pdu_t* pBisTxPduPkt = (bis_tx_pdu_t*)(((u8*)bltBisPduTxfifo.bis_tx_pdu) + (blt_bis_sel * bltBisPduTxfifo.fifo_num + (i & (bltBisPduTxfifo.mask)))* bltBisPduTxfifo.full_size);

            if(curSendBisPldCtnr == pBisTxPduPkt->payloadNumber)
            {
                if(curBisGrp == pBis->irc - 1){ //It's the last chance to send this group of data.
//                  pBis->bisPduTxFifoRptr++;
                    pBis->incTxRptrFlag = 1;
                    tlkapi_send_string_data(DEB_BIG_BCST_EN,"It's the last chance to send this group of data", 0, 0);
                }
                pCurrBisPdu = &pBisTxPduPkt->isoTxPdu;
                tlkapi_send_string_u32s(DEB_BIG_BCST_EN,"Get PDU",curSendBisPldCtnr, pBisTxPduPkt, pCurrBisPdu->dma_len,0);
                break;
            }
            else if(curSendBisPldCtnr < pBisTxPduPkt->payloadNumber)
            { //It indicates that the intermediate HCI ISO DATA PDU is missing
                break; //Send BIS empty packet
            }
            else
            {
                if(curBisGrp == pBis->irc - 1)// this the last sending opportunity
                {
                    pBis->bisPduTxFifoRptr++;
                    tlkapi_send_string_data(DEB_BIG_BCST_EN,"After the last sending opportunity in the group, the old data can be lost", 0, 0);
                }
            }
        }

        /*
         *  purpose: if BN = 1, Upper appp have not data send to controller, so controller will send empty PDU with continue type, then the receiver
         *  will receive the pdu, but resolve this pdu as lost SDU with mistake, so if BN = 1, the first payloadNum set Complete PDU, if BN >1, the
         *  first payloadNum set as complete and the fllowed PDU set as continue PDU, so the Receiver can resolver the PDU correctly
         */
        if((pCurrBisPdu == &gBisEmptyPdu) && (blt_pBigBcst->framing == BIS_UNFRAMED) && (curSendBisPldCtnr%pBis->numSdu2Pdu==0)){
            pCurrBisPdu->llPhysChnPdu.llPduHdr.bisPduHdr.llid = 0;
        }


    }

    pCurrBisPdu->llPhysChnPdu.llPduHdr.bisPduHdr.cssn = blt_pBigBcst->big_sc_cssn;
    pCurrBisPdu->llPhysChnPdu.llPduHdr.bisPduHdr.cstf = blt_pBigBcst->big_sc_cstf;
    rf_set_tx_packet_address((u32)pCurrBisPdu);

    return (u8*)pCurrBisPdu;
}


_attribute_ram_code_
int         blt_bisBcst_tx_start (void)
{
    DBG_CHN4_HIGH;

    #if (SL01_bis_bcst)
        log_task_begin_irq(SL_STACK_BIS_SOURCE_TIMING_EN, SL01_bis_bcst);
    #endif

    //make sure state machine is clean
    STOP_RF_STATE_MACHINE;

    blms_state = BLMS_STATE_BBCST_S;

    //tlkapi_send_string_data(DEB_BIG_BCST_EN,"bisSubEventCnt", &blt_pBis->bisSubEventCnt, 1);
    //tlkapi_send_string_data(DEB_BIG_BCST_EN,"  bis_arrgmt_next_idx", &blt_pBigBcst->bis_arrgmt_next_idx, 1);
    bool big_sc_start = FALSE;
    if(blt_pBigBcst->big_sc_flg & BIG_SC_RDY2SEND_BIGCTRL){
        blt_pBigBcst->big_sc_flg = BIG_SC_RDY2END_BIGCTRL;
        big_sc_start = TRUE;
        //tlkapi_send_string_data(DEB_BIG_BCST_EN,"bisSubCtrl", &blt_pBis->bisSubEventCnt, 1);
        //tlkapi_send_string_data(DEB_BIG_BCST_EN,"  bis total SE num", &blt_pBigBcst->bis_total_se_num, 1);
        //DBG_CHN10_TOGGLE;
    }

    u8  rf_chnIdx;
    u32 rf_access_code, rf_crc_init_val, max_pdu_time;

    if(big_sc_start){
        //BIG Control AccessCode and CrcInitValue
        max_pdu_time = blt_pBigBcst->SCPT;
        rf_access_code = blt_pBigBcst->scAccessCode;
        rf_crc_init_val = blt_pBigBcst->scCrcInit;
        //Get sub control event channel index, BISes use the same channel map as BIG, only ChnId different.
        rf_chnIdx = blt_pBis->ctrlSubEventChnIdx;
    }
    else{
        //Current BIS AccessCode and CrcInitValue
        blt_pBis->bisSubEventCnt++; //SubEventNum increments from 1
        max_pdu_time = blt_pBigBcst->MPT;
        rf_access_code = blt_pBis->bisAccessAddr;
        rf_crc_init_val = blt_pBis->bisCrcInit;
        //Get sub event channel index, BISes use the same channel map as BIG, only ChnId different.
        rf_chnIdx =blt_pBis->subEventChnIdx[blt_pBis->bisSubEventCnt-1];
    }


    //2. RF Hardware register setting
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
    rf_set_ble_channel(rf_chnIdx);
    rf_set_ble_crc_value(rf_crc_init_val);
    rf_set_ble_access_code((u8*)&rf_access_code); //TODO: can use revert value to speed up setting action

    blt_ll_set_tx_power_by_strategy(TX_POWER_STRATEGY_CUSTOMER_OR_DEFAULT, 0);

    rf_trigger_codedPhy_accesscode();
    u32 trigger_tick = bctx_start_tick + TLK_TX_TRIG_OFFSET * SYSTEM_TIMER_TICK_1US;
    u32 tick_now = clock_time();

    if(tick1_exceed_tick2(tick_now, trigger_tick))
    {
        write_dbg32(0x0018, trigger_tick);
        write_dbg32(0x001C, tick_now);
        BLMS_ERR_DEBUG(DBG_BIS_BCST_LOGIC, 0xBBBB0000);

    }

    DBG_CHN4_TOGGLE;DBG_CHN4_TOGGLE;
    //pay attention, this time should be measured
    reg_rf_ll_cmd_schedule = trigger_tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
    reg_rf_ll_cmd = FLD_RF_R_CMD_TRIG | FLD_RF_R_STX;

    //prepare bis data to send(BIG Control PDU or BIS Data PDU).
    blt_ll_prepareBisData(blt_pBis, big_sc_start);

    //system trigger point: consider that RX IRQ must processed
    systimer_set_irq_capture(bctx_start_tick + (max_pdu_time + (blt_pBigBcst->bis_distance_schToAir_us + TLK_TM_DELAY))*SYSTEM_TIMER_TICK_1US);
    systick_irq_trigger = SYS_IRQ_TRIG_BIS_TX_POST;
    if( blc_rf_pa_cb){  blc_rf_pa_cb(PA_TYPE_TX_ON); }

    SubEvtStepTick += blt_pBigBcst->se_length_tick;

    tlkapi_send_string_u32s(0,"ctx", rf_chnIdx, blt_pBigBcst->bigEventCnt, blt_pBis->bisSubEventCnt,0);

    return 1;
}


_attribute_ram_code_
int         blt_bisBcst_tx_post (void)
{

    blms_state = BLMS_STATE_BBCST_E;
    int big_end = 0;

    if(blt_pBis->incTxRptrFlag){ //It's the last chance to send this group of data so release the PDU buffer.
        blt_pBis->bisPduTxFifoRptr++;
    }

    int next_nse_idx = blt_ll_find_next_bisBcst_subevent(blt_pBigBcst->bis_arrgmt_next_idx);
    bool rdy2StartBigSC = next_nse_idx >= blt_pBigBcst->bis_total_se_num && (blt_pBigBcst->big_sc_flg & BIG_SC_RDY2SEND_BISES);

    //Get the next CTX_Start time point: Normal or BIG Control Procedures(BIG_TERMINATE_IND or BIG_CHANNEL_MAP_IND)
    if(next_nse_idx < blt_pBigBcst->bis_total_se_num || rdy2StartBigSC){
        systick_irq_trigger = SYS_IRQ_TRIG_BIS_TX_START;
        bctx_start_tick = bltSche.sSlot_tick_irq_real + SubEvtStepTick;
        systimer_set_irq_capture(bctx_start_tick);

        if(rdy2StartBigSC){
            blt_pBigBcst->big_sc_flg = BIG_SC_RDY2SEND_BIGCTRL;
            blt_pBigBcst->big_sc_cstf = 0;
            blt_ll_prepareBigCtrlData(); //If encryption enabled, prepare before next BCTS start period
        }else{
            #if (HCI_SEND_NUM_OF_CMP_AFT_ACK)

                if((blt_pBis->nocAckRptr != blt_pBis->nocAckWptr) ){//&& (blt_pBis->pBisTestParam==NULL)
                    u8 mrkTxwptr = blt_pBis->nocAclTxWptr[blt_pBis->nocAckRptr & blt_pBis->nocAckMsk];
                    u8 deltaTx = (mrkTxwptr - blt_pBis->bisPduTxFifoRptr)&63;

                    if(deltaTx == 0 || deltaTx > 31){
                        int numberPkt = blt_pBis->numOfCmpCnt[blt_pBis->nocAckRptr & blt_pBis->nocAckMsk];
                        hci_numberOfCompletePacket_evt(blt_pBis->bis_handle, numberPkt);

                        tlkapi_send_string_u32s(DBG_NUM_COM_PKT, "numComPkt, send", blt_pBis->nocAckRptr, blt_pBis->bisPduTxFifoRptr, deltaTx, numberPkt);

                        blt_pBis->nocAckRptr++;
                    }
                }
            #endif
        }
    }
    else{
        big_end = 1;

        if(blt_pBigBcst->big_terminated){
            //DBG_CHN9_TOGGLE;
            big_end = 1;

            #if (ONLY_FOR_EBQ_TEST_LATER_REMOVE)
                bigExtAuxPda_conflictCtrl.bigTask_timingStart = 0;
            #endif

            blt_sche_removeTaskMask(TSKMSK_BIG_BCST_0 << blt_bigBcst_sel);
            blt_sche_addUpdate(SLOT_UPDT_BIS_BCST_REMOVE);

            // Clear BIG SC TERM flag
            blt_pBigBcst->big_sc_mask &= ~BIG_SC_TERM_IND;
            blt_pBigBcst->big_term_inst = 0; // clear
            blt_pBigBcst->big_sc_send_cnt = 0;

            /// After placing hci_le_createBigComplete_evt, destroy the BIG/BIS control block ///
            blt_pBigBcst->big_term_cmp_evt = BIG_TERM_COMPLETE;
            
            //Effective immediately in IRQ, there is a risk of lag shutdown when placed in mainloop
            st_prd_adv_t *cur_pPerdadv = blt_ll_search_existing_perdAdv_index_by_advHandle(blt_pBigBcst->adv_handle);
            if(cur_pPerdadv != NULL){
                blt_prdadv_updateAcadPram(cur_pPerdadv, PERD_ACAD_BIGINFO_DIS);
                cur_pPerdadv->acad_chaged = 0; //in IRQ, acad_chaged clear to zero.
                cur_pPerdadv->link_big_handle = BIG_HANDLE_INVALID;
            }

        #if(BIS_ADV_EBQ) ////add_qw; when terminate complete, restore priority.
            //Big control instant 6 consecutive sch task complete, restore priority.
            blt_ll_setSchedulerTaskPriority(TSKOFT_BIG_BCST + blt_bigBcst_sel, TASK_PRIORITY_BIG_BCST_DFT);//restore priority.
        #endif
        }
        /* When the HCI_LE_Create_BIG command has completed, the HCI_LE_Create_BIG_Complete event
         * is generated. Until the event is generated, the command is considered pending. */
        else if(blt_pBigBcst->cmd_status == BIG_CREATE_PENDING){
            blt_pBigBcst->cmd_status = BIG_CREATE_COMPLETE;

            //process HCI_LE_Create_BIG_Complete event later in BIG main_loop.
            blt_pBigBcst->big_create_cmp_evt = BIG_CREATE_COMPLETE;
        }
    }

    if( blc_rf_pa_cb){  blc_rf_pa_cb(PA_TYPE_OFF); }

    DBG_CHN4_LOW;

#if(HW_AES_CCM_ALG_EN)
//  if(blt_pBis->bisCryptCtrl.enable)
    {
        reg_rf_tx_mode2 &= ~FLD_TLK_CRYPT_ENABLE;
    }
#endif

    #if (SL01_bis_bcst)
    log_task_end_irq(SL_STACK_BIS_SOURCE_TIMING_EN, SL01_bis_bcst);
    #endif

    if(big_end){
        blt_bigBcst_post();
    }

    return 1;
}


_attribute_ram_code_
int         blt_ll_perdAdvAcadUpdateBigInfo(int prdadv_idx)
{
    DBG_CHN5_TOGGLE;
    st_prd_adv_t *pPerdadv = (st_prd_adv_t *) (global_pPerdadv + prdadv_idx);

    if(pPerdadv->prd_adv_en){
        ll_big_bcst_t *cur_pBig = (ll_big_bcst_t *)(global_pBigBcst + pPerdadv->big_idx);

        if(cur_pBig->cmd_status != BIG_IN_IDLE){
            //calculate n_30us_aux_biginfo_ind
            s32 diff_sSlot;
            int jumpBigTask = 0;
            s32 sSlot_mark_big = cur_pBig->sSlot_mark_big;
            diff_sSlot = sSlot_mark_big - bltSche.sSlot_idx_irq_real;

            /* The offset shall be greater than 600 us: sSlot unit: [625/32]us, 31 sSlot => 605us */
            while(diff_sSlot < 31)
            {
                //DBG_CHN11_TOGGLE;
                sSlot_mark_big += cur_pBig->sSlot_interval_big;
                diff_sSlot = sSlot_mark_big - bltSche.sSlot_idx_irq_real;
                jumpBigTask++;             //next BIG slot
                //DBG_CHN11_TOGGLE;
            };

            /*
             * The value of the BIG_Offset field is in the unit of time indicated by the BIG_Offset_Units bit;
             * the actual time offset is determined by multiplying the value of BIG_Offset by the unit. The
             * offset shall be greater than 600 us. If the BIG_Offset_Units bit is set then the unit is 300 us;
             * otherwise it is 30 us. The BIG_Offset_Units bit shall not be set if the offset is less than 491,460 us.
             */
            s32 irq_distance_us = diff_sSlot*SSLOT_US_NUM;
            /**
             * (T1+ offset1) - (T0 + offset0) = (T1 - T0) + (offset1 - offset0)
             */
            s32 off_calib_us = (s32)(cur_pBig->bis_distance_schToAir_us - pPerdadv->aux_sync_tx_off_us);
            irq_distance_us += off_calib_us;
            /*
             * The reference point shall be no later than the Sync Packet Offset and no earlier than the Sync
             * Packet Offset plus one Offset unit prior to the start of the AUX_SYNC_IND.
             */
            //u8  aux_biginfo_align_dly_us = 30 - irq_distance_us%30;
            //u32 n_30or300us_biginfo_offset = (irq_distance_us + aux_biginfo_align_dly_us)/30; //Round up 1 unit
            u32 n_30or300us_biginfo_offset = (irq_distance_us)/30; //Round down 1 unit

            bigInfo_t* pBigInfo = (bigInfo_t*)(pPerdadv->acad+2);
            pBigInfo->bigOffsetUnits = BIG_PDU_BIG_OFFSET_UNITS_30_US;

            if(n_30or300us_biginfo_offset * 30 >= (491460 + 30)){
                pBigInfo->bigOffsetUnits = BIG_PDU_BIG_OFFSET_UNITS_300_US;
                //aux_biginfo_align_dly_us = 300 - irq_distance_us%300;
                //n_30or300us_biginfo_offset = (irq_distance_us + aux_biginfo_align_dly_us)/300;
                n_30or300us_biginfo_offset = (irq_distance_us)/300; //Round down 1 unit
            }
            else if(n_30or300us_biginfo_offset < 20){ //Impossible
                tlkapi_send_string_data(DEB_BIG_BCST_EN,"ERR: bigOffset < 600us, impossible!!!", &n_30or300us_biginfo_offset, 4);
                BLMS_ERR_DEBUG(DBG_BIS_BCST_LOGIC, 0xAABB1065);
            }

            pBigInfo->bigOffset = n_30or300us_biginfo_offset;

            //The BIG task conflicts with other tasks, resulting in not being executed, and X intervals need to be compensated
            //                      //next BIG event counter + compensated
            //u64 bisPldCntr = (cur_pBig->bigEventCnt + jumpBigTask) * cur_pBig->bn;
            u32 bisPldCntr = (cur_pBig->bigEventCnt - 1 + jumpBigTask) * cur_pBig->bn;  //bisPayloadCounter belong to [bigEventCounter * BN, (bigEventCounter + 1) * BN - 1]

            if(!cur_pBig->bigEventCnt){ //for the first time.
                bisPldCntr = 0;
            }

            smemcpy(pBigInfo->bisPldCnt39Framing1, &bisPldCntr, 4);       //The value shall be for the first subevent of the BIG event referred to by the BIG_Offset field
            //NOTE: bisPldCntr use u32 Not u64, so here should not use "|"
            //pBigInfo->bisPldCnt39Framing1[4] |= (cur_pBig->framing << 7);
            pBigInfo->bisPldCnt39Framing1[4] = (cur_pBig->framing << 7); //The Framing bit shall be set if the BIG carries framed data.


            /* If the Host updates the Channel map, it needs to update the syncInfo.chm field information in PerdAdv */
            if(smemcmp(pBigInfo->chm37Phy3, cur_pBig->chnParam.map.chmTbl, 5)){ //If Not equal, change channel map
                smemcpy(pBigInfo->chm37Phy3, cur_pBig->chnParam.map.chmTbl, 5); //[0:36]chm, : [37:39]phy
                pBigInfo->chm37Phy3[4] |= ((cur_pBig->curBisPhy-1) << 5); //The PHY field shall be set to indicate the PHY used by the BIG.0 LE 1M PHY; 1 LE 2M PHY; 2 LE Coded PHY
            }

            return 1;
        }
    }

    return 0;
}


//0xFF: no existing BIG
//other: big_handle is same as existing BIG's
int         blt_ll_searchExistingBigBcstHdl(u8 big_handle)
{
    ll_big_bcst_t *cur_big = NULL;
    for(u8 i=0; i<bltBisMng.maxNum_bigBcst; i++)  //find existing BIG
    {
        cur_big = global_pBigBcst + i;
        if(cur_big->big_handle == big_handle)
        {
            latest_pBigBcst = cur_big;
            return i;
        }
    }

    return BIG_HANDLE_INVALID;
}


int         blt_ll_AllocateNewBigBcstHdl(u8 big_handle)
{
    ll_big_bcst_t *cur_big = NULL;
    for(u8 i=0; i<bltBisMng.maxNum_bigBcst; i++)  //find new BIG
    {
        cur_big = global_pBigBcst + i;
        if(cur_big->big_handle == BIG_HANDLE_INVALID) // //if BIG_HANDLE_INVALID, this BIG can be allocated to new BIG handle
        {
            cur_big->big_handle = big_handle;
            bltBisMng.curNum_bigBcst ++;

            latest_pBigBcst = cur_big;
            return i;
        }
    }

    return BIG_HANDLE_INVALID;
}

#if(FANQH_OPTIMIZE_BIS_API)
ble_sts_t blt_ll_pushBisData(u16 connHandle, iso_pb_flag_t PB_Flag, u8 TS_Flag, u32 time_stamp, u16 seqnum, u16 total_len, u16 cur_len, u8 *pData)
{

#if (IUT_HCI_LOG_EN)
    tlkapi_send_string_u32s(IUT_HCI_LOG_EN, "HCI ISO info 2", connHandle, PB_Flag, total_len, cur_len);
    int dump_len = cur_len > 256 ? 256 : cur_len;
    tlkapi_send_string_data(IUT_HCI_LOG_EN, "@HCI_ISO_SDU in", pData, dump_len);
#endif

    u8 bis_sel = connHandle & BLT_BIS_IDX_MSK;
    ll_bis_t *pBis = (ll_bis_t *)(global_pBis + bis_sel);

    if(!pBis->bis_occupied){
        tlkapi_send_string_u32s(HOST_HCI_ERR_LOG_EN, "HCI ERROR, bis ctrl block error", connHandle, 0, 0, 0);
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    // bis channel parameter not allow transmit data
    if(!pBis->bn){
        tlkapi_send_string_u8s(HOST_HCI_ERR_LOG_EN, "HCI ERROR, bis data not allowed", \
                connHandle, pBis->bn, 0, 0);
        BLMS_ERR_DEBUG(HOST_HCI_ERR_LOG_EN, 0x99C80000);
        return LL_ERR_INVALID_PARAMETER;
    }

    // HCI_ISO_SDU_FIRST_FRAG or HCI_ISO_SDU_COMPLETE: total_len > max_in_fifo_size, total_len > max_sdu_loca
    if( !(PB_Flag & 1) && (total_len > sduBisMng.max_in_fifo_size)){
        tlkapi_send_string_u32s(HOST_HCI_ERR_LOG_EN, "HCI ERROR, iso data too long", total_len, sduBisMng.max_in_fifo_size, 0, 0);
        BLMS_ERR_DEBUG(HOST_HCI_ERR_LOG_EN, 0x99C80000);
        return HCI_ERR_PACKET_TOO_LONG;
    }

    if(pBis->bisSduInFreeNum <=0){
        return IAL_ERR_ISO_TX_FIFO_NOT_ENOUGH;
    }


    sdu_packet_t *iso_sdu = (sdu_packet_t*)(pBis->bis_sduInBuf + (pBis->bisSduIn_wptr & sduBisMng.in_fifo_mask) * sduBisMng.max_in_fifo_size);

    if(blt_iso_process_sdu_in_data(iso_sdu, PB_Flag, TS_Flag, time_stamp, seqnum, total_len, cur_len, pData)){
        pBis->bisSduInFreeNum--;
        pBis->bisSduIn_wptr ++;
    }
    else{
        if(blmsParam.standard_hci_en){
            //blt_cis_mark_numOfcmpEvt_status(pCisConn, numOfcmpPkt);
        }
    }



    return BLE_SUCCESS;
}
#endif

ble_sts_t blc_ll_bisBcst_iso_transmit_test_cmd(u16 connHandle, u8 type)
{

    ll_bis_t *pBis = blt_ll_findBisByHandle(connHandle);
    /*
     * If the Host issues this command with a connection handle that does not exist,
     * or the Connection_Handle command parameter is not associated with a CIS or
     * a BIS, the Controller shall return the error code Unknown Connection Identifier
     * (0x02)
     */
    if(pBis == NULL){
        return HCI_ERR_UNKNOWN_CONN_ID;
    }
    if(!pBis->bn){
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    ll_big_bcst_t *pBig =(ll_big_bcst_t *) (global_pBigBcst + pBis->big_idx);
    if(pBig->cmd_status != BIG_CREATE_COMPLETE){
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    if(pBis->pBisTestParam==NULL){
        pBis->pBisTestParam = &gIsoTestPara[0];
        smemset(pBis->pBisTestParam, 0, sizeof(iso_test_param_t));
    }
    else{
        if(pBis->pBisTestParam->isoTestMode==ISO_TEST_TRANSMIT_MODE){
            return HCI_ERR_CMD_DISALLOWED;
        }
    }

    iso_test_param_t *isoTest = pBis->pBisTestParam;
    isoTest->isoTestMode = ISO_TEST_TRANSMIT_MODE;
    isoTest->isoTest_payload_type = type;
    isoTest->tranMode.isoTestSendTick = 0;
    isoTest->tranMode.send_pkt_cnt = 0;

    return BLE_SUCCESS;
}

ble_sts_t blc_hci_bisBcst_iso_transmit_test_cmd(hci_le_isoTestCmdParams_t *pCmd, hci_le_isoTestRetParams_t *pRet){

    ble_sts_t ret = blc_ll_bisBcst_iso_transmit_test_cmd(pCmd->conn_handle, pCmd->payload_type);
    pRet->status = ret;
    pRet->conn_handle = pCmd->conn_handle;

    return ret;
}

#if(FANQH_OPTIMIZE_BIS_API)
#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
ble_sts_t blc_hci_le_pushBisData(iso_data_packet_t *pIsoDatPkt)
{
    if(pIsoDatPkt->pb & 1) // HCI_ISO_SDU_CONTINUE_FRAG   HCI_ISO_SDU_LAST_FRAG
    {
        return blt_ll_pushBisData(pIsoDatPkt->connHandle, pIsoDatPkt->pb, 0, 0, 0, 0, pIsoDatPkt->iso_dat_len, pIsoDatPkt->p_ISO_data_load);
    }
    else // HCI_ISO_SDU_FIRST_FRAG   HCI_ISO_SDU_COMPLETE
    {

        if(pIsoDatPkt->ts)
        {
            iso_data_load_1_t *pIso_load = (iso_data_load_1_t*)pIsoDatPkt->p_ISO_data_load;
            if(pIsoDatPkt->pb == HCI_ISO_SDU_COMPLETE && (pIso_load->iso_sdu_len + 8) != pIsoDatPkt->iso_dat_len){
                tlkapi_send_string_u32s(HOST_HCI_ERR_LOG_EN, "HCI ERROR, ISO data length not match ISO SDU length", \
                        pIsoDatPkt->iso_dat_len, pIso_load->iso_sdu_len, pIsoDatPkt->ts, 0);
                return HCI_ERR_INVALID_HCI_CMD_PARAMS;
            }

            return blt_ll_pushBisData(pIsoDatPkt->connHandle, pIsoDatPkt->pb, 1, pIso_load->timestamp, pIso_load->pkt_seq, pIso_load->iso_sdu_len, pIsoDatPkt->iso_dat_len - 8, pIso_load->iso_sdu);
        }
        else
        {
            iso_data_load_2_t *pIso_load = (iso_data_load_2_t*)pIsoDatPkt->p_ISO_data_load;
            if(pIsoDatPkt->pb == HCI_ISO_SDU_COMPLETE && (pIso_load->iso_sdu_len + 4) != pIsoDatPkt->iso_dat_len){
                tlkapi_send_string_u32s(HOST_HCI_ERR_LOG_EN, "HCI ERROR, ISO data length not match ISO SDU length", \
                        pIsoDatPkt->iso_dat_len, pIso_load->iso_sdu_len, pIsoDatPkt->ts, 0);
                return HCI_ERR_INVALID_HCI_CMD_PARAMS;
            }

            return blt_ll_pushBisData(pIsoDatPkt->connHandle, pIsoDatPkt->pb, 0, 0, pIso_load->pkt_seq, pIso_load->iso_sdu_len, pIsoDatPkt->iso_dat_len - 4, pIso_load->iso_sdu);
        }
    }
}
#endif

#endif // end of LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER




