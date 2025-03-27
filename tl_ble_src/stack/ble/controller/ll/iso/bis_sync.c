/********************************************************************************************************
 * @file    bis_sync.c
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




#if (LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER)

#ifndef     DBG_BIG_CHN_UPDATE_CMD_EN
#define     DBG_BIG_CHN_UPDATE_CMD_EN               0
#endif


_attribute_ble_data_retention_  ll_big_sync_t   *global_pBigSync = NULL;  //global BIG SYNC parameter pointer

_attribute_ble_data_retention_  ll_big_sync_t   *latest_pBigSync = NULL;  //last used BIG SYNC parameter pointer

_attribute_ble_data_retention_  ll_big_sync_t   *blt_pBigSync = NULL;

_attribute_ble_data_retention_  int              blt_bigSync_sel;

_attribute_ble_data_retention_  u32              bsrx_start_tick;

_attribute_ble_data_retention_  u32      SubEvtStepTick = 0;

_attribute_ble_data_retention_  bis_sync_para_t  bisSync_param;





bis_rx_pdu_chain_t      bltBisRxPduChain[LL_BIG_SYNC_NUM_MAX*LL_BIS_IN_PER_BIG_SYNC_NUM_MAX];
bis_rx_pdu_chain_t      *gBltBisRxPduChain;



/******the following only for Qihang: broadcast sink conflict with ACL. Later will delete******/
typedef     int (*bigBuildTsk_confilctACL_cb_t)(unsigned char);
_attribute_ble_data_retention_  bigBuildTsk_confilctACL_cb_t   bigBuildTsk_conflictACL_cb = NULL;

_attribute_ble_data_retention_  volatile u32 gExpiredTimeUs = 1*1000*1000;//default 1s

_attribute_ram_code_
int bigBuildTsk_confilctACL_proc(u8 aclTask_idx){

    u32 tick_mark = blms[aclTask_idx].tick_qihang_mark_conn;

    if(clock_time_exceed(tick_mark, gExpiredTimeUs)){

        blms[aclTask_idx].tick_qihang_mark_conn = clock_time();
        return 0; //ACL insert success.
    }

    return 1; //ACL insert fail.
}

void blc_ll_register_bigConflictACL_CB(void){
    bigBuildTsk_conflictACL_cb = bigBuildTsk_confilctACL_proc;
}

void blc_ll_changeConflictExpeirdTimeUs(u32 expiredTimeUs){
    gExpiredTimeUs = expiredTimeUs;
}
/**********above only for Qihang: broadcast sink conflict with ACL. Later will delete *********/



/*
 * @brief      This function is used to initialize Synchronize sdu out fifo buffer.
 *             sdu out indicate the data if from controller to host.
 * @param[in]  out_fifo      - the start address of Synchronize sdu out fifo buffer.
 * @param[in]  out_fifo_size - the fifo size.
 * @param[in]  out_fifo_num  - the fifo number.
 */
ble_sts_t blc_ll_initBisSyncSduOutBuffer(u8 *out_fifo, u16 out_fifo_size, u8 out_fifo_num)
{
    if( IS_POWER_OF_2(out_fifo_num)){
        sduBisMng.out_fifo_num = out_fifo_num;
        sduBisMng.out_fifo_mask = out_fifo_num -1;
    }
    else{
        return LL_ERR_INVALID_PARAMETER;
    }

    /* size must be 4*n */
    if((out_fifo_size & 3) == 0){
        sduBisMng.max_out_fifo_size = out_fifo_size;
    }
    else{
        return LL_ERR_INVALID_PARAMETER;
    }

    sduBisMng.out_fifo_b = out_fifo;

    return BLE_SUCCESS;
}

/**
 * @brief      for user to initialize BIS ISO RX FIFO.
 * @param[in]  pRxbuf - RX FIFO buffer address.
 * @param[in]  fifo_size - RX FIFO size
 * @param[in]  fifo_number - RX FIFO number, can only be 4, 8, 16 or 32
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t blc_ll_initBisRxFifo(u8 *pRxbuf, int full_size, int fifo_number, u8 bis_sync_num)
{
    bltempParam.ll_bisRxFifo_set = 1;

    /* size must be 4*n */
    if(((full_size & 3) != 0) || (bis_sync_num>LL_BIG_SYNC_NUM_MAX*LL_BIS_IN_PER_BIG_SYNC_NUM_MAX)){
        return LL_ERR_INVALID_PARAMETER;
    }



    for(u8 n = 0; n<bis_sync_num; n++){
        bis_rx_pdu_chain_t *pBisRxPduChain = &bltBisRxPduChain[n];

        tlkapi_send_string_data(0, "Chain", &pBisRxPduChain, 4);

        pBisRxPduChain->total_num = fifo_number;
        pBisRxPduChain->freeNum = fifo_number;
        pBisRxPduChain->fifo_size = full_size -12 ;//*next(4), payloadNum(4), tick(4)
        pBisRxPduChain->full_size = full_size;
        pBisRxPduChain->pUsed = NULL;


        bis_rx_pdu_t *iso_pdu =(bis_rx_pdu_t*)  (pRxbuf  + n*(full_size)*fifo_number);
        pBisRxPduChain->pFree = iso_pdu;
        pBisRxPduChain->pBackup = (u8*)iso_pdu;

        tlkapi_send_string_data(0,"pRxbuf", &pRxbuf,4);

        tlkapi_send_string_data(0, "full size", &full_size, 2);

        for(int i = 0; i<fifo_number-1; i++){

            iso_pdu->next = (bis_rx_pdu_t*)(((u32)iso_pdu) + (full_size));

            tlkapi_send_string_u32s(0,"iso_pdu", i, iso_pdu, iso_pdu->next, 0);

            iso_pdu = iso_pdu->next;
        }

        iso_pdu->next = NULL;
    }

    /*
     * 12 = *next(4), payloadNum(4), tick(4)
     */
    blmsParam.bisSyncRfLenMax = full_size -12 - TLK_RF_RX_EXT_LEN;

    return BLE_SUCCESS;
}


ble_sts_t blt_ll_ResetBisRxFifo(u8 bis_sync_handle){

    u8 sync_sel = (bis_sync_handle & BLT_BIS_IDX_MSK)  - bltBisMng.maxNum_bisBcst;

    bis_rx_pdu_chain_t *pBisRxPduChain = &bltBisRxPduChain[sync_sel];


    pBisRxPduChain->freeNum = pBisRxPduChain->total_num;
    pBisRxPduChain->pUsed = NULL;

    bis_rx_pdu_t *iso_pdu =(bis_rx_pdu_t*) pBisRxPduChain->pBackup;
    pBisRxPduChain->pFree = iso_pdu;

    tlkapi_send_string_u32s(0, "reset rx fifo", bis_sync_handle, iso_pdu, pBisRxPduChain->freeNum,0);

    for(int i = 0; i<pBisRxPduChain->total_num-1; i++){

        iso_pdu->next = (bis_rx_pdu_t*)(((u8*)iso_pdu) + (pBisRxPduChain->full_size));

        tlkapi_send_string_u32s(0,"iso_pdu",i,iso_pdu, iso_pdu->next,0);

        iso_pdu = iso_pdu->next;
    }

    iso_pdu->next = NULL;

    return BLE_SUCCESS;
}


init_err_t  blt_bis_insertRxPdu(ll_bis_t *pBisSync);


_attribute_ram_code_
int         blt_ll_buildBigSyncSchedulerLinklist(void)
{
    int intvl_jump_big;
    s32 sSlot_start_big;
    ll_big_sync_t *pBigSync = NULL;

    /*The absolute value on the time axis corresponding to Task->begin:
    sSlot_tick_start + Task->begin*SSLOT_TICK_NUM, sSlot_idx_base is the relative value */
//  u32 sSlot_idx_base = (bltSche.bSlot_idx_next - bltSche.bSlot_idx_start)*32;

    for(int i=0; i<bltBisMng.maxNum_bigSync; i++)
    {
        if( bltSche.task_mask & (TSKMSK_BIG_SYNC_0<<i ))
        {

            pBigSync = (ll_big_sync_t *)(global_pBigSync + i);
            pBigSync->schTsk_wptr = pBigSync->schTsk_rptr = 0;


            //sSlot time axis reset, need to consider compensation
            if(bltSche.sSlot_idx_reset == 1 && (bltSche.build_index == 0)){//
                pBigSync->sSlot_mark_big -= bltSche.sSlot_idx_past;
            }



            if( pBigSync->sSlot_mark_big >= bltSche.sSlot_idx_next){
                sSlot_start_big = pBigSync->sSlot_mark_big + pBigSync->sSlot_interval_big;
                intvl_jump_big = 0;
            }
            else{
                intvl_jump_big = (bltSche.sSlot_idx_next - 1 - pBigSync->sSlot_mark_big) / pBigSync->sSlot_interval_big;
                sSlot_start_big = pBigSync->sSlot_mark_big + (intvl_jump_big + 1) * pBigSync->sSlot_interval_big;

                blt_ll_incSchedulerTaskCalPriority( TSKOFT_BIG_SYNC + i, bltPri.step_final[TSKOFT_BIG_SYNC + i]*2*intvl_jump_big );
            }

            if(sSlot_start_big >= bltSche.sSlot_endIdx_dft){ //to save some time for big interval
                continue; //attention: can not use break !!!
            }

            int new_task_cnt = 0;
            for(int j=0;j<BIG_FIFONUM;j++){

                sch_task_t  *pCur_schTask = (sch_task_t *)&pBigSync->bigs_schTsk_fifo[j];

                pCur_schTask->begin = sSlot_start_big + j*pBigSync->sSlot_interval_big; ///ISO_interval
                pCur_schTask->end = pCur_schTask->begin + pBigSync->sSlot_duration_big - 1;

                if( pCur_schTask->begin >=  bltSche.sSlot_endIdx_dft){  //new task beyond correct range, finish
                    break;
                }
                else if(pCur_schTask->end < bltSche.sSlot_endIdx_dft){ //new task in correct range
                    pBigSync->schTsk_wptr = j;
                    new_task_cnt ++;
                    blt_ll_incSchedulerTaskCalPriority( TSKOFT_BIG_SYNC + i, -bltPri.step_final[TSKOFT_BIG_SYNC + i]);
                }
                else{ //new task across "sSlot_endIdx_dft"

                    //for those task across end_idx, find the task with highest priority, to guarantee that task not missed
                    if(bltPri.pri_cal[TSKOFT_BIG_SYNC + i] > bltPri.priMax_value){
                        bltPri.priMax_value = bltPri.pri_cal[TSKOFT_BIG_SYNC + i];
                        bltPri.priMax_index = TSKOFT_BIG_SYNC + i;
                        bltSche.sSlot_endIdx_maxPri = pCur_schTask->begin;
                        tlkapi_send_string_u32s(SCHE_TIMING_IMPROVE_DBG_EN,"across IDX bigSync", i, bltSche.sSlot_endIdx_dft, bltSche.sSlot_endIdx_maxPri, bltPri.priMax_value);
                    }
                    break;
                }
            }

            if(new_task_cnt){
                blt_ll_addTask2ExistLinklist( &pBigSync->bigs_schTsk_fifo[0], pBigSync->schTsk_wptr + 1);
            }
        }
    }

    return 0;
}


#if BLS_PROC_BIS_SYNC_UPDATE_REQ_IN_IRQ_ENABLE

#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
#endif
void blt_bisSync_rx_procUpdateReq(u8 *raw_pkt)
{
    u8 llid = raw_pkt[DMA_RFRX_OFFSET_HEADER] & 0x03;
    u8 rf_len = raw_pkt[DMA_RFRX_OFFSET_RFLEN];
    u8 enc_en = blt_pBigSync->bigCtrlCrypt.enable;
    if( llid == 3 && (enc_en ? (rf_len==8 || rf_len==12) : (rf_len==4 || rf_len==8))){
        smemcpy(blt_pBigSync->bigCtrlRawPkt, raw_pkt, rf_len+2+4);

        if(enc_en){
            blt_pBigSync->bigCtrlPktDecPending = blt_pBigSync->bigCtrlRawPkt;
            blt_pBigSync->bigCtrlPktRcvd_no = blt_pBigSync->bigEventCnt * blt_pBigSync->bn;
            bisSync_param.updateCmd_pending |= BIT(blt_bigSync_sel);
            tlkapi_send_string_data(DBG_BIG_CHN_UPDATE_CMD_EN, "bis rcv bigctrl holding", &blt_pBigSync->bigCtrlPktRcvd_no, 4);
        }
        else{
            blt_ll_bisSync_chnm_term_update(blt_pBigSync);
        }
    }
}

#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
#endif
void blt_bisSync_slotgap_procUpdateReq(void)
{
    ll_big_sync_t *pBigSync = NULL;
    for(int i=0; i<bltBisMng.maxNum_bigSync; i++){
        if(bisSync_param.updateCmd_pending & BIT(i)){

            if(tick1_exceed_tick2(bltSche.sSlot_tick_irq, clock_time() + AES_CCM_DEC_US*SYSTEM_TIMER_TICK_1US)){
                pBigSync = (ll_big_sync_t *)(global_pBigSync + i);

                blt_ll_bisSync_chnm_term_update(pBigSync);
                bisSync_param.updateCmd_pending &= ~BIT(i);
                pBigSync->bigCtrlPktDecPending = NULL;
            }
            else{
                break;
            }
        }
    }
}

/*
 * brief: in core spec v5.3, only there are two big control commands:
 * 1. BIG_CHANNEL_MAP_IND; 2. BIG_TERMINATE_IND
 */
#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
#endif
int blt_ll_bisSync_chnm_term_update(ll_big_sync_t* pBigSync)
{
    rf_packet_ll_data_t* pBisRawPkt = (rf_packet_ll_data_t*)pBigSync->bigCtrlRawPkt;

    //BIS decrypt
    if(pBigSync->bigCtrlCrypt.enable){
        ble_crypt_para_t* pLeCryptCtrl = &pBigSync->bigCtrlCrypt;
        pLeCryptCtrl->dec_pno = pBigSync->bigCtrlPktRcvd_no;
        /* The directionBit shall be set to 1 for Broadcast Isochronous PDUs sent by the Isochronous Broadcaster. */
        aes_enc_dec_busy = 1;
        int st = aes_ll_ccm_decryption(&pBisRawPkt->llPhysChnPdu, 1, CRYPT_NONCE_TYPE_BIS, pLeCryptCtrl);
        aes_enc_dec_busy = 0;

        if(st){  //decrypt err
            pBigSync->bigCtrlCrypt.mic_fail = 1;

            pBigSync->bigTermSyncFlag = 2;
            pBigSync->bigSyncEvtStatus = HCI_ERR_CONN_TERM_MIC_FAILURE; //mark lost reason
            tlkapi_send_string_data(STACK_DUMP_EN, "LE BIG Sync lost: MIC Failure", 0, 0);

            #if (DBG_DECRYPTION_ERR_EN)
                BLMS_ERR_DEBUG(IAL_DEBUG_EN, 0xAA990000);
            #endif

            return 1;
        }
    }

    rf_packet_ll_control_t *pll = (rf_packet_ll_control_t *)&pBisRawPkt->llPhysChnPdu;
    u8 rf_len = pll->rf_len;
    u8 type = pll->type & 3;
    u8 opcode = pll->opcode;
    if(type == 3){
        if(opcode == BIG_CHANNEL_MAP_IND && rf_len == 8){
            big_chmInd_data_t* pChm = (big_chmInd_data_t*)&pll->opcode;
            tlkapi_send_string_data(DBG_BIG_CHN_UPDATE_CMD_EN, "BIG_CHANNEL_MAP_IND", 0, 0);

            s16 diff_inst = pChm->instant - pBigSync->bigEventCnt;

            if(diff_inst > 0 && !(pBigSync->bigctrl_update&BIG_SC_CHM_IND)){
                smemcpy ( pBigSync->nextChnMap.chmTbl, pChm->chm, 5);

                tlkapi_send_string_data(DBG_BIG_CHN_UPDATE_CMD_EN, "chnMap", pBigSync->nextChnMap.chmTbl, 5);
                csa2_calculateMapInfo(&pBigSync->nextChnMap);
                pBigSync->bigctrl_update |= BIG_SC_CHM_IND;
                pBigSync->nxtChmInst = pChm->instant;
                tlkapi_send_string_data(DBG_BIG_CHN_UPDATE_CMD_EN, "BIG_CHANNEL_MAP_IND mark", &pChm->chm, 5);

                DBG_CHN1_TOGGLE;DBG_CHN1_TOGGLE;
            }
        }else if(opcode == BIG_TERMINATE_IND && rf_len == 4){
            big_termInd_data_t* pTerm = (big_termInd_data_t*)&pll->opcode;
            tlkapi_send_string_data(DBG_BIG_CHN_UPDATE_CMD_EN, "BIG_TERMINATE_IND", 0, 0);
            s16 diff_inst = pTerm->instant - pBigSync->bigEventCnt;

            if(diff_inst > 0 && !(pBigSync->bigctrl_update&BIG_SC_TERM_IND)){
                pBigSync->nxtTermInst = pTerm->instant;
                pBigSync->nxtTermRsn = pTerm->reason;
                pBigSync->bigctrl_update |= BIG_SC_TERM_IND;
                tlkapi_send_string_data(DBG_BIG_CHN_UPDATE_CMD_EN, "BIG_TERMINATE_IND mark", 0, 0);
            }
        }
    }

    return 0;
}

#endif



void blt_ll_resetBisSync(u16 sync_handle)
{
    ll_bis_t *pBis = global_pBis + (sync_handle&BLT_BIS_IDX_MSK);

    pBis->bis_occupied = 0;
    pBis->bisSduOut_wptr = pBis->bisSduOut_rptr = 0;
    pBis->dpID = 0xff;
    pBis->bis_dapth_setup = 0;
    pBis->bisSuccessiveMiss = 0;

    if(bltBisMng.curNum_bisSync>=1){
        bltBisMng.curNum_bisSync--;
    }
    tlkapi_send_string_data(0,"release BIS", &pBis->bis_handle, 2);
//  blt_ll_ResetBisRxFifo(pBis->bis_handle);
}

void blt_ll_resetBigSync(ll_big_sync_t *pBigSync){


    for(int i= 0; i<pBigSync->sync_bis_num; i++)
    {
        blt_ll_resetBisSync(pBigSync->bis_handle[i]);
    }

    tlkapi_send_string_data(0, "hci term",0,0);

    if(bltBisMng.curNum_bigSync>=1)//In case it goes negative
    {
        bltBisMng.curNum_bigSync --;
    }

    pBigSync->sync_bis_num = 0;
    pBigSync->biss_arrgmtMap_msk = 0;
    pBigSync->bis_total_se_num=0;
    pBigSync->big_handle = BIG_HANDLE_INVALID;
    pBigSync->big_state = BIG_SYNC_IN_IDLE;
    pBigSync->sync_handle = BIG_HANDLE_INVALID; /* SYNC_HANDLE_INVALID */

    pBigSync->last_cssn = 0xFF;
    pBigSync->bigCtrlPktDecPending = NULL;

}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
void blt_ll_bisSync_sdu_out_loop(ll_bis_t* pBis)
{

    while(pBis->bisSduOut_rptr != pBis->bisSduOut_wptr)
    {
        sdu_packet_t* sdu = (sdu_packet_t*)(pBis->bis_sduOutBuf + sduBisMng.max_out_fifo_size * \
                            ((pBis->bisSduOut_rptr&sduBisMng.out_fifo_mask)));

        if(sdu->iso_sdu_len){
            tlkapi_send_string_u32s(0, "sdu loop", pBis->bisSduOut_rptr, pBis->bisSduOut_wptr, sdu->iso_sdu_len,0);
        }

        if(blt_iso_proSduPacket(sdu)==BLE_SUCCESS){
            pBis->bisSduOut_rptr++;
        }
    }

}

#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
int         blt_ll_bigSyncMainloop(void)
{
    ll_big_sync_t *pBigSync = NULL;
    ll_bis_t      *pBisSync = NULL;

    for(int i=0; i<bltBisMng.maxNum_bigSync; i++)
    {
        pBigSync = (ll_big_sync_t *)(global_pBigSync + i);

        if(pBigSync->big_handle == BIG_HANDLE_INVALID){
            continue;
        }

        st_pda_sync_t *pPda_sync =(st_pda_sync_t *)&pdAsync_tbl[pBigSync->sync_handle & BLT_SYNC_IDX_MARK];

        ////////////////// Notify host Big Sync establish/terminate/Sync lost event ///////////////////
        if(pBigSync->irq_bigSyncEvt==2)
        {
            if(hci_le_eventMask & HCI_LE_EVT_MASK_BIG_SYNC_LOST)
            {
                hci_le_bigSyncLost_evt(pBigSync->big_handle, pBigSync->bigSyncEvtStatus);
            }
            pBigSync->bigSyncEvtStatus = 0;
            pBigSync->irq_bigSyncEvt = 0;
            blt_ll_resetBigSync(pBigSync);
            pPda_sync->bigInfor_para.bigInfor_flag = 0;
        }
        else if(pBigSync->irq_bigSyncEvt==1)
        { //notify host the establish(or terminate, use same evt) evt
            pBigSync->irq_bigSyncEvt = 0;

            if(hci_le_eventMask & HCI_LE_EVT_MASK_BIG_SYNC_ESTABLISHED)
            {
                hci_le_bigSyncEstablished_evt(pBigSync->bigSyncEvtStatus, pBigSync->big_handle, \
                                             (u8*)&pBigSync->transLatency_us, pBigSync->nse, pBigSync->bn, pBigSync->pto, \
                                              pBigSync->irc, pBigSync->max_pdu, pBigSync->iso_itvl,  pBigSync->sync_bis_num, pBigSync->bis_handle);
            }

            if(pBigSync->bigSyncEvtStatus)
            { //0: success; others: failure, should destroy BISes and BIG's control block
                pBigSync->bigSyncEvtStatus = 0; //clear

                blt_ll_resetBigSync(pBigSync);

                pPda_sync->bigInfor_para.bigInfor_flag = 0;

                tlkapi_send_string_data(DEB_BIG_SYNC_EN,"Destroy BIS and BIG control block", 0, 0);
            }
        }

    #if BLS_PROC_BIS_SYNC_UPDATE_REQ_IN_IRQ_ENABLE
        if(pBigSync->big_state == BIG_SYNCHRONIZED)
        {
            if (pBigSync->bigCtrlPktDecPending){
                u32 r = irq_disable(); //IRQ boundary protect
                pBigSync->bigCtrlPktDecPending = NULL;
                bisSync_param.updateCmd_pending &= ~BIT(i);
                irq_restore(r);
                tlkapi_send_string_data(DBG_BIG_CHN_UPDATE_CMD_EN, "loop process: bigctrl holding", 0, 0);
                blt_ll_bisSync_chnm_term_update (pBigSync);
            }
        }
    #endif

        if(pBigSync->big_state == BIG_SYNCHRONIZED)//
        {
            for(int j = 0; j< pBigSync->sync_bis_num; j++)
            {
                u8 bis_idx = pBigSync->bis_handle[j] & BLT_BIS_IDX_MSK;
                pBisSync = (ll_bis_t *) (global_pBis + bis_idx);
                iso_test_param_t    *pBisTestParam = pBisSync->pBisTestParam;

                blt_bisSync_process(pBisSync); // reassemble Pdu to SDU
                if(pBisSync->bisSduOut_wptr !=  pBisSync->bisSduOut_rptr)
                {

                    if((pBisTestParam != NULL) && (pBisTestParam->isoTestMode==ISO_RECEIVE_MODE))
                    {
                        sdu_packet_t *sdu = (sdu_packet_t*)(pBisSync->bis_sduOutBuf + sduBisMng.max_out_fifo_size * \
                                                  ((pBisSync->bisSduOut_rptr&(sduBisMng.out_fifo_mask))));

                        blt_iso_test_receive_mainloop(sdu, pBisTestParam, pBigSync->max_sdu, pBigSync->framing);
                        pBisSync->bisSduOut_rptr++;
                    }
                #if (1)
                    //only controller to register "blt_hci_iso_data_handler"
                    if(blt_hci_iso_data_handler){
                        blt_ll_bisSync_sdu_out_loop(pBisSync);
                    }
                #else

                    if((pBisSync->dpID == Data_Path_HCI) && blc_hci_event_handler)
                    {
                        sdu_packet_t* sdu = (sdu_packet_t*)(sduBisMng.out_fifo_b + sduBisMng.max_out_fifo_size * \
                                            ((bis_sync_idx)*sduBisMng.out_fifo_num + (pBisSync->bisSduOut_rptr&(sduBisMng.out_fifo_mask))));

                        if(blc_hci_event_handler(pBisSync->bis_handle|HCI_FLAG_ISO_DATE_STD, (u8*)sdu, sdu->iso_sdu_len)==0) //blc_hci_send_data
                        {
                            pBisSync->bisSduOut_rptr++;
                        }
                    }
                #endif

                }
            }
        }

    }

    return 1;
}


_attribute_ram_code_
int         irq_big_sync_rx(void)
{
    HAL_CLEAR_RF_RX_IRQ;
    u32 tick_now = clock_time();


    u8 *raw_pkt = ble_curr_rx_dma_buff;
    raw_pkt[0] = 0;


    tlkapi_send_string_data(0,"pFree   ", &gBltBisRxPduChain->pFree,4);
    tlkapi_send_string_data(0,"pFree->next", &(gBltBisRxPduChain->pFree->next),4);
    tlkapi_send_string_u32s(0,"rx IRQ pFree", gBltBisRxPduChain->pFree, (gBltBisRxPduChain->pFree->next),gBltBisRxPduChain->pFree->next->next, gBltBisRxPduChain->pBackup);


    u8* new_pkt = (u8*)(gBltBisRxPduChain->pFree->next->rawData);
    if(gBltBisRxPduChain->pFree->next != NULL){
        ble_rf_set_rx_dma(new_pkt, gBltBisRxPduChain->fifo_size>>4);
    }
    else{
        tlkapi_send_string_data(DBG_BISNC_RX_PDU,"#####Error2!!!!!!!", &blt_pBigSync->bigEventCnt, 4);
        BLMS_ERR_DEBUG(DBG_BISNC_RX_PDU, 0xDD0F0002);
    }


    if(blms_state == BLMS_STATE_BSYNC_S)
    {

        /* "rx header tick" none zero is under condition CRC correct, so here do not check CRC by
           "RF_BLE_PACKET_VALIDITY_CHECK" to save RamCode and running timing
           Or we can use "bltRxPkt.crc correct" */
        if(bltRxPkt.rx_header_tick)
        {
            blt_pBis->bisReceivePkt = 1; //RX with CRC correct
            //Used for BIG Sync to monitor Sync timeout
            blt_pBigSync->bigSyncConnTick = tick_now;
            blt_pBis->bisSubEvtRecFlag = 1;
            tlkapi_send_string_u32s(DEB_BIG_SYNC_LL_DATA_EN&0,  "[bSync][rx irq]", blt_pBis->bis_handle, blt_pBigSync->bigEventCnt, blt_pBis->bisSubEventCnt,0);


            if(raw_pkt[DMA_RFRX_OFFSET_RFLEN]){
                #if (SLEV_bis0_rx_len)
                    log_event_irq(SL_STACK_BIS_SINK_TIMING_EN, (SLEV_bis0_rx_len + blt_bis_sel - bltBisMng.maxNum_bisBcst));
                #endif
            }
            else{
                #if (SLEV_bsync_rev)
                    log_event_irq(SL_STACK_BIS_SINK_TIMING_EN, SLEV_bsync_rev);
                #endif
            }

            DBG_FANQH_CHN5_TOGGLE;
            if(!blt_pBigSync->bisSyncRxTick)
            {
                if(blms_state  ==  BLMS_STATE_BSYNC_S)
                { //BIS Sync used only
                    blt_pBigSync->bisSyncRxTick = bltRxPkt.rx_header_tick;

                        blt_pBigSync->bSync_expect_tick = blt_pBigSync->bisSyncRxTick;
                        blt_pBigSync->bigs_1st_expect_tick = (blt_pBigSync->bisSyncRxTick - (blt_pBis->bisSubEventCnt-1)*blt_pBis->sub_interval_tick)
                                - (blt_pBis->bisSyncIdx-1) * blt_pBis->bis_spacing_tick + blt_pBigSync->bigs_ap_offset_us*SYSTEM_TIMER_TICK_1US;
                        s32 diff = blt_pBigSync->bigs_1st_expect_tick - bltSche.sSlot_tick_irq_real;


                        // The precision of sch adjustment is sSlot so the threshold setting here should be greater than sSlot
                        // note : irq delay, flash write(825x disable irq)will affect here
                        if((diff > (BYNC_FIRST_EARLY_US + 30)*SYSTEM_TIMER_TICK_1US  || diff < (BYNC_FIRST_EARLY_US-30)*SYSTEM_TIMER_TICK_1US) || (blt_pBigSync->big_state == BIG_SYNCHRONIZING))
                        {
                            DBG_CHN0_TOGGLE;DBG_CHN0_TOGGLE;
                            DBG_FANQH_CHN0_TOGGLE;DBG_FANQH_CHN0_TOGGLE;



                            if(blt_pBigSync->big_state == BIG_SYNCHRONIZING)
                            {// delete the 30/300us tolerance
                                blt_pBigSync->sSlot_duration_big -= blt_pBigSync->bigSyncToleranceTime*SSLOT_US_REVERSE;
                                blt_pBigSync->bigSyncToleranceTime =0;
                            }
                            blt_pBigSync->sSlot_mark_big = blt_pBigSync->sSlot_mark_big + 2*diff/625 - BYNC_FIRST_EARLY_SLOT_NUM; //7sSlot = 10*19.5 = 195us
                            blt_pBigSync->bSlot_mark_big = bltSche.bSlot_idx_start + (blt_pBigSync->sSlot_mark_big>>5);
                            blt_sche_addUpdate(SLOT_UPDT_BIS_BSYNC_CREATE);//refresh sch map
                        }

                #if(!WALKAROUND_ISO_TIMESTAMP_EN)
                    //todo The accuracy range required by EQB is 1us, which is not satisfied here
                    blt_pBigSync->bigRefAP = blt_pBigSync->bigs_1st_expect_tick - blt_pBigSync->bigs_ap_offset_us*SYSTEM_TIMER_TICK_1US;

                #endif
                tlkapi_send_string_u32s(DEB_BIG_SYNC_TIMESTAM_EN&0, "Ref:rx", blt_pBis->bis_handle,blt_debug_hex_2_dec_display(blt_pBigSync->bigRefAP), blt_pBigSync->bisSyncRxTick, blt_pBis->bisSubEventCnt);
                }
            }

            if(blt_bis_insertRxPdu(blt_pBis) != INIT_SUCCESS){
                ble_rf_set_rx_dma(raw_pkt, gBltBisRxPduChain->fifo_size>>4);
            }
            else{
                raw_pkt[0] = blt_pBis->bis_handle; //mark value valid

                tlkapi_send_string_data(DEB_BIG_SYNC_EN, "mark bis rx valid", &raw_pkt[0], 1);
            }

            rf_packet_ll_data_t* pBisRawPkt = (rf_packet_ll_data_t*)raw_pkt;
            u8 rcvdCstf = pBisRawPkt->llPhysChnPdu.llPduHdr.bisPduHdr.cstf; //Control Subevent Transmission Flag
            u8 rcvdLlid = pBisRawPkt->llPhysChnPdu.llPduHdr.bisPduHdr.llid; //0b11 = BIG Control PDU


            /* ready to receive big sub control packet. */
            if(rcvdCstf && !blt_pBigSync->big_sc_flg)
            {
                blt_pBigSync->big_sc_flg = BIG_SC_RCVD_CSTF;
                tlkapi_send_string_data(DEB_BIG_SYNC_EN, "bis rcv cstf", 0, 0);
            }

            if(blt_pBigSync->big_sc_flg & BIG_SC_RDY2RCV_BIGCTRL)
            {
                tlkapi_send_string_data(DEB_BIG_SYNC_EN, "big ctrl pkt", &raw_pkt, raw_pkt[5]+6);

                if(rcvdLlid == 0x03 && rcvdCstf == 0)
                { /* Big ctrl temp buffer 16B: special */
                    u8 rcvdCssn = pBisRawPkt->llPhysChnPdu.llPduHdr.bisPduHdr.cssn; //Control Subevent Sequence Number
                    if(rcvdCssn != blt_pBigSync->last_cssn)
                    {
                        blt_pBigSync->last_cssn = rcvdCssn;

                        #if BLS_PROC_BIS_SYNC_UPDATE_REQ_IN_IRQ_ENABLE
                            tlkapi_send_string_data(DBG_BIG_CHN_UPDATE_CMD_EN, "bis rcv buffer,rcvdCssn=", &rcvdCssn, 1);
                            blt_bisSync_rx_procUpdateReq(raw_pkt);
                        #endif
                    }
                    else
                    { //same pkt received, skip rx pkt
                        tlkapi_send_string_data(DEB_BIG_SYNC_EN, "repeat bis rcv buffer, drop it", &rcvdCssn, 1);
                    }
                }else if(rcvdLlid == 0x03)
                {
                    tlkapi_send_string_data(DEB_BIG_SYNC_EN, "bis rcv bigctrl pkt, err", 0, 0);
                }
            }
        }
        else{ //CRC invalid or rf_len invalid

        }
    }

//  blt_pBigSync->bisSyncRxNum++; //do not care CRC


//  raw_pkt[0] = 1;

    return 0;
}


_attribute_ram_code_
init_err_t  blt_bis_insertRxPdu(ll_bis_t *pBisSync)
{
// remove for qihang, cis don't remove, (blmsParam.standard_hci_en)&&
    if((pBisSync->dpID ==0xff) && (pBisSync->pBisTestParam ==NULL)){// have not set data path, so discard this PDU
        return LL_BIS_RX_PDU_INVALID;
    }
    u8 bisPTO = pBisSync->pto;
    u8 bisEvtJmpNum = 0;


    u8 curBisGrp = (pBisSync->bisSubEventCnt-1)/pBisSync->bn; //curBisGrp (g) is started from zero.
    u8 bisPldCntrOffset = (pBisSync->bisSubEventCnt-1)%pBisSync->bn;

    if(curBisGrp < pBisSync->irc){
        pBisSync->bisRcvdPldNum = blt_pBigSync->bigEventCnt * pBisSync->bn + bisPldCntrOffset;
        if(curBisGrp==(pBisSync->irc-1))
        {
            pBisSync->rxPldLimite = pBisSync->bisRcvdPldNum;
        }
        //DBG_CHN7_TOGGLE;
        tlkapi_send_string_u32s(DEB_BIG_SYNC_EN&0,"g<irc:BisPldCtnr", pBisSync->bisRcvdPldNum, blt_pBigSync->bigEventCnt, bisPldCntrOffset, pBisSync->bn);
    }
    else{ //curBisGrp >= pBisSync->irc
        bisEvtJmpNum = bisPTO * (curBisGrp - pBisSync->irc + 1);
        pBisSync->bisRcvdPldNum = (blt_pBigSync->bigEventCnt + bisEvtJmpNum) * pBisSync->bn + bisPldCntrOffset;
        //DBG_C HN8_TOGGLE;
        tlkapi_send_string_u32s(DEB_BIG_SYNC_EN&0,"g>=irc:BisPldCtnr", pBisSync->bisRcvdPldNum, blt_pBigSync->bigEventCnt, bisPldCntrOffset, bisEvtJmpNum);
    }

    tlkapi_send_string_u32s(DEB_BIG_SYNC_EN&0, "LL PDU", pBisSync->bisRcvdPldNum, blt_pBigSync->bigEventCnt, blt_pBis->lastPayloadNum, pBisSync->bis_handle);

    //DBG_C HN8_TOGGLE;
    if((blt_pBigSync->big_state == BIG_SYNC_IN_IDLE) || ((pBisSync->rx_first_pdu)&&(pBisSync->bisRcvdPldNum <= blt_pBis->lastPayloadNum)))
    {
        tlkapi_send_string_u32s(DEB_BIG_SYNC_EN, "First PDU Received", blt_debug_hex_2_dec_display(pBisSync->bisRcvdPldNum), blt_pBis->lastPayloadNum, blt_pBigSync->big_state, 0);
        return LL_BIS_RX_PDU_INVALID;
    }

    bis_rx_pdu_t *pBisRxPdu = gBltBisRxPduChain->pFree;

#if 0
    if((blt_pBigSync->framing==0)&&(pBisRxPdu->rawData[DMA_RFRX_OFFSET_RFLEN]==0))
    {
        tlkapi_send_string_data(DEB_BIG_SYNC_EN, "PDU_NULL",&pBisSync->bisRcvdPldNum,4);
        return LL_BIS_RX_PDU_EMPTY; //empty discard
    }
#endif

    pBisRxPdu->payloadNum = pBisSync->bisRcvdPldNum; //keep received pkt's bisPayloadNum
    bis_rx_pdu_t *pUsedRxPdu = gBltBisRxPduChain->pUsed;
    bis_rx_pdu_t *pPreRxPdu = NULL;


#if (1)
    while(1)
    {

        /* pUsed list is empty, so insert this tag in usedList header*/
        if(pUsedRxPdu==NULL){
            gBltBisRxPduChain->pFree = pBisRxPdu->next;
            gBltBisRxPduChain->pUsed = pBisRxPdu; // insert RxEvent in the header of pUsed list
            pBisRxPdu->next = NULL;
            gBltBisRxPduChain->freeNum --;
            tlkapi_send_string_u32s(DEB_BIG_SYNC_EN," Insert ListHeader",pBisRxPdu->payloadNum, gBltBisRxPduChain->pUsed,gBltBisRxPduChain->pFree->next, 0);
            break;
        }

        if(pBisRxPdu->payloadNum < pUsedRxPdu->payloadNum) // pldNum less than the usedList header,insert header
        {
            gBltBisRxPduChain->pFree = pBisRxPdu->next;

            if(pPreRxPdu){
                pPreRxPdu->next = pBisRxPdu;
            }else{
                gBltBisRxPduChain->pUsed = pBisRxPdu;
            }

            pBisRxPdu->next = pUsedRxPdu;
            gBltBisRxPduChain->freeNum --;

            tlkapi_send_string_u32s(DEB_BIG_SYNC_EN&0,"<",pBisRxPdu->payloadNum, (pPreRxPdu==NULL)?0:pPreRxPdu->payloadNum, pUsedRxPdu->payloadNum, pBisRxPdu);

            break;
        }
        else if(pBisRxPdu->payloadNum > pUsedRxPdu->payloadNum){
            if( pUsedRxPdu->next == NULL){
                gBltBisRxPduChain->pFree = pBisRxPdu->next;

                pUsedRxPdu->next = pBisRxPdu; //insert RxEvent in the tail of pUsed List
                pBisRxPdu->next = NULL;

                gBltBisRxPduChain->freeNum --;

                tlkapi_send_string_u32s(DEB_BIG_SYNC_EN&0,">",pBisRxPdu->payloadNum, pUsedRxPdu->payloadNum, pBisRxPdu->next, pBisRxPdu);
                break;
            }
            else{
                pPreRxPdu = pUsedRxPdu;
                pUsedRxPdu = pUsedRxPdu->next;
            }
        }
        else{
            //discard this PDU
            tlkapi_send_string_data(DEB_BIG_SYNC_EN&0," Discard PDU",&pBisSync->bisRcvdPldNum,4);
            return LL_BIS_RX_PDU_INVALID;
        }

    }

    if(!pBisSync->rx_first_pdu){
        blt_pBis->lastPayloadNum = pBisRxPdu->payloadNum -1;
        blt_pBis->rx_first_pdu = 1;

//      DBG_FANQH_CHN3_TOGGLE;DBG_FANQH_CHN3_TOGGLE;
        tlkapi_send_string_u32s(DEB_BIG_SYNC_EN &0, "set rx_first_pdu", blt_pBis->lastPayloadNum, 0,0,0);
    }


#if (SL16_bis0_rx_pldNum)
    log_b16_irq(SL_STACK_BIS_SINK_TIMING_EN, SL16_bis0_rx_pldNum+blt_bis_sel-bltBisMng.maxNum_bisBcst, (u16)pBisRxPdu->payloadNum);
#endif

    #if(ADD_SUD_TIMESTAMP_EN)//SYSTEM_TIMER_TICK_1US
        pBisRxPdu->bigRefAnchorPoint = blt_pBigSync->bigRefAP + (pBisRxPdu->payloadNum/pBisSync->bn - blt_pBigSync->bigEventCnt)*blt_pBigSync->iso_itvl*SYSTEM_TIMER_TICK_1250US;
        tlkapi_send_string_u32s(DEB_BIG_SYNC_EN&0, "ref: insert",(blt_pBigSync->bigEventCnt),  pBisRxPdu->payloadNum, pBisRxPdu->bigRefAnchorPoint, pBisSync->bisSubEventCnt);
    #endif
    DBG_FANQH_CHN7_TOGGLE;
    tlkapi_send_string_u32s(DEB_BIG_SYNC_LL_DATA_EN,  "[bSync][Insert]", pBisSync->bis_handle,pBisSync->bisRcvdPldNum, blt_pBigSync->bigEventCnt, pBisSync->bisSubEventCnt);

#endif
    return INIT_SUCCESS;
}


_attribute_ram_code_
init_err_t  blt_bis_deletePdu(bis_rx_pdu_chain_t *bisRxPduChain)
{

    u32 r = irq_disable();
    bis_rx_pdu_t *pUsedRxPdu = bisRxPduChain->pUsed;

    if(pUsedRxPdu == NULL){
        return LL_BIS_RX_PDU_INVALID;
    }


    bisRxPduChain->pUsed = pUsedRxPdu->next; //remove pUsed header tag,this point maybe NULL

    pUsedRxPdu->next = bisRxPduChain->pFree->next;
    bisRxPduChain->pFree->next = pUsedRxPdu; //remove pUsed header tag to the tag after the pFree header tag

    bisRxPduChain->freeNum ++;
    irq_restore(r);

    return INIT_SUCCESS;
}



_attribute_ram_code_
void        blt_ll_bis_duration_jump (ll_bis_t *pBis)
{
    (void)pBis; //unused, remove warning
//  u8 *raw_pkt = (u8*)gBltBisRxPduChain->pFree->rawData;
//  raw_pkt[0] = 0;//invalid this pdu note: gBltBisRxPduChain is not init when sync is not establish

//  for(int i = 1; i <= pBis->bn; i++){
//      pBis->bisSubEventCnt = i; //offset 1
//      blt_bis_insertRxPdu(pBis);
//  }

}

/**
 * @brief      for user to initialize BIG Synchronize module and allocate BIG Synchronize parameters buffer.
 * @param[in]  pBigSyncPara - start address of BIG Synchronize parameters buffer
 * @param[in]  bigSyncNum - BIG Synchronize number application layer may use
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t  blc_ll_initBigSyncModule_initBigSyncParametersBuffer(u8 *pBigSyncPara, u8 bigSyncNum)
{

    STATIC_ASSERT_FILE(BIG_SYNC_PARAM_LENGTH == sizeof(ll_big_sync_t), bis_sync);

    #if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
        STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_bis_t)),  bis_sync);
        STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_big_sync_t)), bis_sync);
    #endif

    LL_FEATURE_MASK_0 |= (LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER <<31);

    //BIS need this feature bit enable. for standard controller, bit will be cleared when received HCI_RESET command
    LL_FEATURE_MASK_1 |= LL_FEATURE_MASK_ISOCHRONOUS_CHANNELS;

    /* Special protection code for use */
    if(pm_check_info){
        ll_big_sync_irq_task_cb = blt_big_sync_interrupt_task;
        ll_big_sync_mlp_task_cb = blt_big_sync_mainloop_task;
    }

    blmsParam.big_sync_en = 1;

    /////////////////////////////////////////////////////////


    if(bigSyncNum > LL_BIG_SYNC_NUM_MAX)
    {
        bigSyncNum = LL_BIG_SYNC_NUM_MAX;
    }

    global_pBigSync = (ll_big_sync_t*)pBigSyncPara;
    bltBisMng.maxNum_bigSync = bigSyncNum;
    bltBisMng.curNum_bigSync = 0;


    ll_big_sync_t *cur_bigSync;
    for(u8 i=0; i<bltBisMng.maxNum_bigSync; i++)
    {
        cur_bigSync = global_pBigSync + i;
        cur_bigSync->big_handle  = BIG_HANDLE_INVALID;
        cur_bigSync->big_state   = BIG_SYNC_IN_IDLE;
        cur_bigSync->sync_handle = BIG_HANDLE_INVALID; /* SYNC_HANDLE_INVALID */


        for(int j=0; j < BIG_FIFONUM; j++){
            cur_bigSync->bigs_schTsk_fifo[j].scheTask_oft = TSKOFT_BIG_SYNC + i;
            cur_bigSync->bigs_schTsk_fifo[j].scheTask_idx = i;
            cur_bigSync->bigs_schTsk_fifo[j].scheTask_flg = TSKFLG_BIG_SYNC;
        }

        blt_ll_setSchedulerTaskPriority(TSKOFT_BIG_SYNC + i, TASK_PRIORITY_HIGH_THRES);
    }

    return BLE_SUCCESS;

}




init_err_t blt_ll_checkBisSyncInit(void)
{
    if(bltempParam.ll_bisRxFifo_set){
        for(u8 n = 0; n<bltBisMng.maxNum_bigSync; n++){
            bis_rx_pdu_chain_t  *pBisRxPduChain =   &bltBisRxPduChain[n];
            if(pBisRxPduChain->pFree == NULL)
                return LL_BIS_RX_EVT_BUF_PARAM_INVALID;
        }
    }
    else{//bisSync moudule init but buffer not set
        if(blmsParam.big_sync_en){
            return LL_BIS_RX_BUF_NO_INIT;
        }
    }

    if(sduBisMng.out_fifo_b==NULL){
        return LL_BIS_RX_IAL_BUF_NO_INIT;
    }

#if(FANQH_OPTIMIZE_BIS_API)
    for(int i = bltBisMng.maxNum_bisBcst; i<bltBisMng.maxNum_bisTotal; i++){
        ll_bis_t *pBis = global_pBis + i;
        pBis->bis_sduOutBuf = sduBisMng.out_fifo_b + ((i-bltBisMng.maxNum_bisBcst)*sduBisMng.max_out_fifo_size*sduBisMng.out_fifo_num);
    }
#endif

    return INIT_SUCCESS;
}


_attribute_ram_code_
int         blt_big_sync_interrupt_task (int flag, void* p)
{
    int big_idx = flag & FLAG_SCHEDULE_TASK_IDX_MASK;

    if(flag & FLAG_SCHEDULE_BIGSYNC_START){
        blt_bigSync_start(big_idx);
    }
    else if(flag & FLAG_SCHEDULE_BISSYNC_START){
        blt_bisSync_rx_start();
    }
    else if(flag & FLAG_SCHEDULE_BISSYNC_POST){
        blt_bisSync_rx_post();
    }
    else if(flag & FLAG_SCHEDULE_BIGSYNC_BUILD){
        blt_ll_buildBigSyncSchedulerLinklist();
    }
    else if(flag & FLAG_SCHEDULE_BISSYNC_RX){
        irq_big_sync_rx();
    }
#if BLS_PROC_BIS_SYNC_UPDATE_REQ_IN_IRQ_ENABLE
    else if(flag & FLAG_BIS_SYNC_CHECK_UPDATE_CMD_DEC){
        blt_bisSync_slotgap_procUpdateReq();
    }
#endif
    else if(flag & FLAG_INSERT_SCHTSK_CONFLICT){
        sch_task_t *pTgtTsk = (sch_task_t *)p;
        u8 tgtTskFlg = pTgtTsk->scheTask_flg & TSKFLG_VALID_MASK;
        u8 curSchTaskOft = TSKOFT_BIG_SYNC + big_idx;
        (void)tgtTskFlg; //remove compiler warning
        #if(SL08_bisSync_conflict)
        log_b8_irq(SL_STACK_BIS_SINK_TIMING_EN, SL08_bisSync_conflict, tgtTskFlg);
        #endif


        if(bigBuildTsk_conflictACL_cb){
            if(tgtTskFlg == TSKFLG_ACL_SLAVE){
                return bigBuildTsk_conflictACL_cb(pTgtTsk->scheTask_idx);
            }
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
            tlkapi_send_string_data(0, "[big_bcst]consecutive abandon count", &bltPri.csctvAbandonCnt[curSchTaskOft], 2)
            return 1; /* 1:conflict resolved; 0: insert task failed */
        }
    }
    return 0;
}


_attribute_noinline_
int         blt_big_sync_mainloop_task (int flag, void *p)
{
    if(flag == (int)FLAG_MODULE_RESET){
        blt_ll_reset_big_sync();
    }
    else if(flag == (int)FLAG_CHECK_INIT){
        return blt_ll_checkBisSyncInit();
    }
    else if(flag == (int)FLAG_MODULE_MAINLOOP){
        blt_ll_bigSyncMainloop();
    }
    else if(flag == (int)FLAG_BIG_SYNC_HANDLE_SEARCH){
        u8 big_handle = *(u8*)p;
        return blt_ll_findExistingBigSyncByBigHdl(big_handle);
    }

    return 0;
}


_attribute_noinline_
void        blt_ll_reset_big_sync(void)
{

    //Clear BIG Sync concerned global variables
    bltBisMng.curNum_bigSync = 0;
    for(u8 i=0; i<bltBisMng.maxNum_bigSync; i++)
    {
        ll_big_sync_t *pBigSync = global_pBigSync + i;;

        blt_ll_resetBigSync(pBigSync);
    }

}




bigInfor_para_t*
            blt_ll_getBigInorBySyncHdl(u16 sync_handle) //only support 2 sync_handle: 0 and 1
{
    tlkapi_send_string_data(DEB_BIG_SYNC_EN,"sync_handle", &sync_handle, 2);
    u16 sync_index = sync_handle & BLT_SYNC_IDX_MARK;

    if(pdAsync_tbl[sync_index].bigInfor_para.bigInfor_flag){
        return &pdAsync_tbl[sync_index].bigInfor_para;
    }
    else{
        return NULL;
    }

}



_attribute_ram_code_
static int  blt_ll_find_next_bisSync_subevent (int start_idx)
{
    int ret = 100;  //important

    for(int i = start_idx; i <blt_pBigSync->bis_total_se_num; i++){
        if(blt_pBigSync->biss_arrgmtMap_msk & BIT(i)){

            //Get the valid BIS
            blt_bis_sel = blt_pBigSync->bis_arrgmt_map[i];
            blt_pBis = (ll_bis_t*)(global_pBis + blt_bis_sel); //The first valid BIS


            #if (DBG_BIS_SYNC_TIMING)
            if(blt_bis_sel < bltBisMng.maxNum_bisBcst){
                BLMS_ERR_DEBUG(DBG_BIS_SYNC_TIMING, 0xFF150000 | blt_bis_sel);
            }

            #endif

            gBltBisRxPduChain = &bltBisRxPduChain[blt_bis_sel - bltBisMng.maxNum_bisBcst];

            //Update the starting index of the current BIS arrangement map to the next IDX of the currently valid IDX.
            blt_pBigSync->bisSync_next_id = i + 1;



            return i;
        }
    }

    return ret;
}


_attribute_ram_code_
int         blt_bigSync_start(int slotTask_idx)
{
    DBG_FANQH_CHN3_HIGH;
    #if (SL01_big_sync)
        log_task_begin_irq(SL_STACK_BIS_SINK_TIMING_EN, SL01_big_sync);
    #endif

    blt_bigSync_sel = slotTask_idx;
    //1.First locate the BIG that belongs to
    blt_pBigSync = (ll_big_sync_t*)(global_pBigSync + blt_bigSync_sel);
    blt_pBigSync->bisSync_next_id = 0;
    blt_pBigSync->SyncNextSubEvtStepTick = 0;
    blt_pBigSync->bisSyncRxTick = 0;


    /* BIG slot skipped, need compensation for every BIS belong to the BIG */
    int inter_jump_num = (bltSche.bSlot_idx_irq_real + 4 - blt_pBigSync->bSlot_mark_big)/blt_pBigSync->bSlot_interval_big - 1;

    //important! ! ! Compensation for X BIG Events skipped due to timing conflicts.
    //fanqh todo compensation payloadNum of bises in BIG
    if(blt_pBigSync->bigEventCnt && inter_jump_num > 0){
        for(int i = 0; i<inter_jump_num; i++){
            blt_pBigSync->bigEventCnt ++;
            blt_pBigSync->bigRefAP += blt_pBigSync->iso_itvl*1250 * SYSTEM_TIMER_TICK_1US;
            blt_pBigSync->bigs_1st_expect_tick += blt_pBigSync->iso_itvl *1250*SYSTEM_TIMER_TICK_1US;
            blt_pBigSync->bSync_expect_tick = blt_pBigSync->bigs_1st_expect_tick;
//          for(int j = 0; j< blt_pBigSync->sync_bis_num; j++)
//          {
//              u8 bsync_idx = blt_pBigSync->bis_handle[j] & BLT_BIS_IDX_MSK;
//              ll_bis_t *pBisSync = (ll_bis_t *) (global_pBis + bsync_idx);
//              blt_ll_bis_duration_jump(pBisSync); //TODO process PDU to notify IAL
//          }
        }

        #if (SL01_bsync0_tsk_jump)
            log_b16_irq(SL_STACK_BIS_SINK_TIMING_EN, SL01_bsync0_tsk_jump, (u16)inter_jump_num);
        #endif

        blt_ll_incSchedulerTaskPriority( TSKOFT_BIG_SYNC + blt_bigSync_sel, bltPri.step_final[TSKOFT_BIG_SYNC + blt_bigSync_sel]*2*inter_jump_num );
    }

    #if (SL16_bigs_eventCnt)
        log_b16_irq(SL_STACK_BIS_SINK_TIMING_EN, SL16_bigs_eventCnt, (u16)blt_pBigSync->bigEventCnt);
    #endif




    blt_pBigSync->bSlot_mark_big = bltSche.bSlot_idx_irq_real; //update slot index mark;
    blt_pBigSync->sSlot_mark_big = bltSche.sSlot_idx_irq_real;
    bsrx_start_tick = bltSche.sSlot_tick_irq_real;


    /* 2M/Coded PHY feature must be enabled for EXT BIS, so do not use pointer "ll_phy_switch_cb" */
    rf_ble_switch_phy(blt_pBigSync->curBisPhy, (le_coding_ind_t)blt_pBigSync->codingInd);


    blt_ll_find_next_bisSync_subevent(0);
    //625 + (BRX_EARLY_SET_TICK>>4) + 25); //800 uS, need confirm with JunWen if timeout end point is after access_code
    rf_set_1st_rx_timeout((bltPHYs.cur_llPhy == BLE_PHY_CODED) ? (500 + 296) : 500); //make sure PHY switch before this code

    //Check if you need to receive BIS Control Packet. //BIG Control Procedures: BIG_TERMINATE_IND or BIG_CHANNEL_MAP_IND
    blt_pBigSync->big_sc_flg = 0;

    if(blt_pBigSync->bigctrl_update & BIG_SC_CHM_IND){
        //pay attention here, BIG slot may dropped, blt_pBigSync->bigEventCnt >= big_sc_inst(consider 0xffff->0 problem, (u16).... < 1024 )
        if((u16)(blt_pBigSync->bigEventCnt - blt_pBigSync->nxtChmInst) < BIT(10)){
            blt_pBigSync->bigctrl_update &= ~BIG_SC_CHM_IND;
            blt_pBigSync->nxtChmInst = 0;

            //Ready to use the new channel map
            smemcpy(&blt_pBigSync->chnParam.map, &blt_pBigSync->nextChnMap, sizeof(struct le_channel_map)); //Update new channel map table

            tlkapi_send_string_data(DBG_BIG_CHN_UPDATE_CMD_EN,"bigctrl_update chm instant arrived", &blt_pBigSync->bigEventCnt, 4);
        }
    }else if(blt_pBigSync->bigctrl_update & BIG_SC_TERM_IND){
        //pay attention here, BIG slot may dropped, blt_pBigSync->bigEventCnt >= big_sc_inst(consider 0xffff->0 problem, (u16).... < 1024 )
        if((u16)(blt_pBigSync->bigEventCnt - blt_pBigSync->nxtTermInst) < BIT(10)){
            blt_pBigSync->bigctrl_update &= ~BIG_SC_TERM_IND;
            blt_pBigSync->nxtTermInst = 0;

            blt_pBigSync->bigTermSyncFlag = 2;
            blt_pBigSync->bigSyncEvtStatus = blt_pBigSync->nxtTermRsn ? blt_pBigSync->nxtTermRsn : HCI_ERR_REMOTE_USER_TERM_CONN; //maybe zero ?
            tlkapi_send_string_data(DEB_BIG_SYNC_EN,"bigctrl_update term instant arrived", &blt_pBigSync->bigEventCnt, 4);
        }
    }
    for(int i= 0; i<blt_pBigSync->sync_bis_num; i++)
    {
        ll_bis_t *pBis = global_pBis + (blt_pBigSync->bis_handle[i]&BLT_BIS_IDX_MSK);
        pBis->bisSubEventCnt = 0;

        for(int j = 1; j<= pBis->nse; j++){//NSE = 5,   35-40us,  NSE=16,   71us
            pBis->subEventChnIdx[j-1] = blt_ll_generateNextChannel(&blt_pBigSync->chnParam, blt_pBigSync->bigEventCnt,
                    pBis->chnIdentifier, j);

        }

        pBis->ctrlSubEventChnIdx = blt_ll_generateNextChannel(&blt_pBigSync->chnParam, blt_pBigSync->bigEventCnt,
                blt_pBigSync->chnIdentifier, 1);
    }

//  DBG_CHN 3_HIGH;
    rf_ble_set_rx_settle(RX_SETTLE_US);

    blt_bisSync_rx_start();

    tlkapi_send_string_u32s(0,"Brx pFree", gBltBisRxPduChain->pFree, (gBltBisRxPduChain->pFree->next),gBltBisRxPduChain->pFree->next->next, gBltBisRxPduChain->pFree->next->next->next);



    return 0;
}


_attribute_ram_code_
int         blt_bigSync_post(void)
{
    blms_state = BLMS_STATE_BIG_E;
    blt_ll_calculate_sSlot_next(clock_time() + SLOT_PROCESS_MAX_TICK);


    if(blt_pBigSync->big_state == BIG_SYNCHRONIZING){

        if(blt_pBigSync->bisSyncRxTick){
            blt_pBigSync->bigRxLostCnt = 0;
            st_pda_sync_t *pda_sync = &pdAsync_tbl[ blt_pBigSync->sync_handle & BLT_SYNC_IDX_MARK];
            pda_sync->bigInfor_para.creating_bisSync_flag =0;

            blt_pBigSync->big_state = BIG_SYNCHRONIZED;
            blt_pBigSync->irq_bigSyncEvt = 1; //mainloop process this event
            blt_pBigSync->bigSyncEvtStatus = BLE_SUCCESS;


            DBG_CHN6_TOGGLE;DBG_CHN6_TOGGLE;DBG_CHN6_TOGGLE;DBG_CHN6_TOGGLE;
            tlkapi_send_string_data(DEB_BIG_SYNC_EN,"BIG_SYNCHRONIZED",&blt_pBigSync->bigEventCnt,4);

        }
        else{
            blt_pBigSync->bigRxLostCnt++;
        }

        if(blt_pBigSync->bigRxLostCnt >= 5) {
            st_pda_sync_t *pda_sync = &pdAsync_tbl[ blt_pBigSync->sync_handle & BLT_SYNC_IDX_MARK];
            pda_sync->bigInfor_para.creating_bisSync_flag =0;

            blt_pBigSync->irq_bigSyncEvt = 1;
            blt_pBigSync->bigSyncEvtStatus = HCI_ERR_CONN_FAILED_TO_ESTABLISH; //TODO: not clear
            tlkapi_send_string_data(DEB_BIG_SYNC_EN,"Big Sync establish timeout", 0, 0);
        }
    }

    if(blt_pBigSync->bigTermSyncFlag)
    {
        blt_pBigSync->irq_bigSyncEvt = blt_pBigSync->bigTermSyncFlag; //mainloop process the flag and destroy BIG/Bis control block
        blt_pBigSync->bigTermSyncFlag = 0;
        tlkapi_send_string_data(DEB_BIG_SYNC_EN,"irq_bigSyncEvt=1", &blt_pBigSync->bigSyncEvtStatus, 1);
    }
    else if(clock_time_exceed(blt_pBigSync->bigSyncConnTick, blt_pBigSync->bigSyncTimeoutUs))
    {
        blt_pBigSync->irq_bigSyncEvt = 2;
        blt_pBigSync->bigSyncEvtStatus = HCI_ERR_CONN_TIMEOUT; //mark lost reason
        tlkapi_send_string_data(DEB_BIG_SYNC_EN,"Big Sync lost mark rsn", 0, 0);

    }

    if((blt_pBigSync->irq_bigSyncEvt) && (blt_pBigSync->bigSyncEvtStatus))
    {
        #if (BLS_PROC_BIS_SYNC_UPDATE_REQ_IN_IRQ_ENABLE)
            bisSync_param.updateCmd_pending &= ~BIT(blt_bigSync_sel);
            tlkapi_send_string_data(DEB_BIG_SYNC_EN, "bis term: clr bigctrl holding", 0, 0);
        #endif

        //remove BigSync task
        blt_sche_removeTaskMask(TSKMSK_BIG_SYNC_0 << blt_bigSync_sel);
        blt_sche_addUpdate(SLOT_UPDT_BIS_BSYNC_REMOVE);
    }
    else{
        blt_pBigSync->bigs_1st_expect_tick += blt_pBigSync->iso_itvl *1250*SYSTEM_TIMER_TICK_1US;
        blt_pBigSync->bSync_expect_tick = blt_pBigSync->bigs_1st_expect_tick;
    }

    blt_pBigSync->bigEventCnt++;
    blt_pBigSync->bigRefAP += blt_pBigSync->iso_itvl*1250*SYSTEM_TIMER_TICK_1US;


    #if (SL01_big_sync)
        log_task_end_irq(SL_STACK_BIS_SINK_TIMING_EN, SL01_big_sync);
    #endif
    DBG_FANQH_CHN3_LOW;


    return 0;
}


_attribute_ram_code_
int         blt_bisSync_rx_start (void)
{
//  DBG_C HN4_HIGH;
    #if (SL01_bsync0)
        log_task_begin_irq(SL_STACK_BIS_SINK_TIMING_EN, SL01_bsync0+(blt_bis_sel-bltBisMng.maxNum_bisBcst));
    #endif

    //make sure state machine is clean
    STOP_RF_STATE_MACHINE;
    CLEAR_ALL_RFIRQ_STATUS;

    blms_state = BLMS_STATE_BSYNC_S;

    u8  rf_chnIdx;
    u32 rf_access_code, rf_crc_init_val;

    if(blt_pBigSync->big_sc_flg & BIG_SC_RDY2RCV_BIGCTRL){
        //BIG Control AccessCode and CrcInitValue
        rf_access_code = blt_pBigSync->scAccessCode;
        rf_crc_init_val = blt_pBigSync->scCrcInit;
        rf_chnIdx = blt_pBis->ctrlSubEventChnIdx;
    }
    else{
        //Current BIS AccessCode and CrcInitValue
        blt_pBis->bisSubEventCnt++; //SubEventNum increments from 1

        if(blt_pBis->bisSubEventCnt == 1){ //first sub_event of current BIS event
            blt_pBis->bisReceivePkt = 0;   //RX with CRC correct
        }
        blt_pBis->bisSubEvtRecFlag = 0;

        rf_access_code = blt_pBis->bisAccessAddr;
        rf_crc_init_val = blt_pBis->bisCrcInit;
        rf_chnIdx = blt_pBis->subEventChnIdx[blt_pBis->bisSubEventCnt-1];
    }

    //2. RF Hardware register setting
    rf_set_tx_rx_off();
    rf_set_ble_channel(rf_chnIdx);
    rf_set_ble_access_code((u8*)&rf_access_code); //TODO: can use revert value to speed up setting action
    rf_set_ble_crc_value(rf_crc_init_val);

    blt_ll_set_tx_power_by_strategy(TX_POWER_STRATEGY_CUSTOMER_OR_DEFAULT, 0);

    #if(LL_FEATURE_ENABLE_LE_CODED_PHY)
        rf_trigger_codedPhy_accesscode();
    #endif


    //Switch dma rx buffer to BIS dam rx buffer
    if((gBltBisRxPduChain->pFree == NULL) || (!gBltBisRxPduChain->freeNum)){
        tlkapi_send_string_u32s(DBG_BISNC_RX_PDU,"#####Error1!!!!!!!", gBltBisRxPduChain->pFree, gBltBisRxPduChain->freeNum,gBltBisRxPduChain,blt_pBigSync->bigEventCnt);
        BLMS_ERR_DEBUG(DBG_BISNC_RX_PDU, 0xDD0F0001);
    }

    tlkapi_send_string_data(0, "Free_", &gBltBisRxPduChain->pFree, 4);

    gBltBisRxPduChain->pFree->rawData[0] = 0;
    ble_rf_set_rx_dma(gBltBisRxPduChain->pFree->rawData, gBltBisRxPduChain->fifo_size>>4);
    rf_set_rx_maxlen(blmsParam.bisSyncRfLenMax);//replace by fanqh



    //pay attention, this time should be measured
    reg_rf_ll_cmd_schedule = clock_time(); //bsrx_start_tick + TLK_TX_TRIG_OFFSET * SYSTEM_TIMER_TICK_1US;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
    reg_rf_ll_cmd = FLD_RF_R_CMD_TRIG | FLD_RF_R_SRX;


    if( blc_rf_pa_cb){  blc_rf_pa_cb(PA_TYPE_RX_ON); } //Turn on PA ~100us in advance with RF packet head

#if(HW_AES_CCM_ALG_EN)
    // can save payloadNum in global and use in rx IRQ
    if(blt_pBis->bisCryptCtrl.enable)
    {
        iso_evtcnt_t payloadNum=0;
        u8 curBisGrp = (blt_pBis->bisSubEventCnt-1)/blt_pBis->bn; //curBisGrp (g) is started from zero.
        u8 bisPldCntrOffset = (blt_pBis->bisSubEventCnt-1)%blt_pBis->bn;

        if(curBisGrp < blt_pBis->irc){
            payloadNum = blt_pBigSync->bigEventCnt * blt_pBis->bn + bisPldCntrOffset;
        }
        else{ //curBisGrp >= pBisSync->irc
            u8 bisEvtJmpNum =  blt_pBis->pto * (curBisGrp - blt_pBis->irc + 1);
            payloadNum = (blt_pBigSync->bigEventCnt + bisEvtJmpNum) * blt_pBis->bn + bisPldCntrOffset;
        }

        blt_ll_setAesCcmPara(1, blt_pBis->bisCryptCtrl.sk, blt_pBis->bisCryptCtrl.nonce.iv, 0xc3,\
                0, payloadNum,  0);
    }
#endif

    //time between IRQ and here cost 10us
    DBG_FANQH_CHN4_HIGH;

    systimer_set_irq_capture(blt_pBigSync->bSync_expect_tick + (blt_pBigSync->MPT+TLK_TM_DELAY+10 + blt_pBigSync->bigSyncToleranceTime)*SYSTEM_TIMER_TICK_1US);
    systick_irq_trigger = SYS_IRQ_TRIG_BIS_RX_POST;

    tlkapi_send_string_u32s(0, "brx_start", blt_pBigSync->bigEventCnt, bsrx_start_tick + (blt_pBigSync->MPT + (blt_pBigSync->bigSyncToleranceTime + TLK_TM_DELAY))*SYSTEM_TIMER_TICK_1US, clock_time(),0);

    tlkapi_send_string_u32s(0, "RF setup", blt_debug_hex_2_dec_display(blt_pBigSync->bigEventCnt), blt_debug_hex_2_dec_display(rf_chnIdx), rf_crc_init_val,rf_access_code);

    return 1;
}


_attribute_ram_code_
int         blt_bisSync_rx_post (void)
{

    blms_state = BLMS_STATE_BSYNC_E;
    int big_end = 0;

//  if((!(blt_pBigSync->big_sc_flg & BIG_SC_RDY2RCV_BIGCTRL)) && (!blt_pBis->bisSubEvtRecFlag)){
//
//      u8 *raw_pkt = (u8*)gBltBisRxPduChain->pFree->rawData;
//      raw_pkt[0] = 0;//invalid this pdu
//      blt_bis_insertRxPdu(blt_pBis);
//  }




#if (SL01_bsync0)
    log_task_end_irq(SL_STACK_BIS_SINK_TIMING_EN, SL01_bsync0+(blt_bis_sel-bltBisMng.maxNum_bisBcst));
#endif

    int next_nse_idx = blt_ll_find_next_bisSync_subevent(blt_pBigSync->bisSync_next_id);
    bool rdy2RcvBigSC = next_nse_idx >= blt_pBigSync->bis_total_se_num && (blt_pBigSync->big_sc_flg & BIG_SC_RCVD_CSTF);



    //Get the next CTX_Start time point: Normal or BIG Control Procedures(BIG_TERMINATE_IND or BIG_CHANNEL_MAP_IND)
    if(next_nse_idx < blt_pBigSync->bis_total_se_num || rdy2RcvBigSC){
        systick_irq_trigger = SYS_IRQ_TRIG_BIS_RX_START;

        if(rdy2RcvBigSC){
            next_nse_idx =  blt_pBigSync->bis_total_se_num;//blt_pBigSync->nse;
        }
        blt_pBigSync->bSync_expect_tick = blt_pBigSync->bigs_1st_expect_tick + blt_pBigSync->se_length_tick * (next_nse_idx-blt_pBigSync->bigStartOffset_se_num);
        u32 srx_start_tick = blt_pBigSync->bSync_expect_tick - BYNC_RX_RF_TRIGGER_EARLY_US*SYSTEM_TIMER_TICK_1US;//

        u32 tick_now = clock_time();
        DBG_CHN6_TOGGLE;
        DBG_FANQH_CHN8_TOGGLE;

        if(tick1_exceed_tick2(tick_now, srx_start_tick))
        {
            if(tick1_exceed_tick2(srx_start_tick + 10*SYSTEM_TIMER_TICK_1US, tick_now))
            {
                DBG_CHN8_TOGGLE;DBG_CHN8_TOGGLE;
                srx_start_tick = tick_now + 15*SYSTEM_TIMER_TICK_1US;
            }

            #if(DBG_BIS_SYNC_TIMING)
            else
            {
                write_dbg32(0x0018, srx_start_tick);
                write_dbg32(0x001C, tick_now);
                BLMS_ERR_DEBUG(DBG_BIS_SYNC_TIMING, 0xBBEEFF00);
            }
            #endif
        }
        systimer_set_irq_capture(srx_start_tick);

        tlkapi_send_string_u32s(0, "post", blt_pBigSync->bigEventCnt,next_nse_idx, clock_time(),bsrx_start_tick);

        if(rdy2RcvBigSC){
            blt_pBigSync->big_sc_flg = BIG_SC_RDY2RCV_BIGCTRL;
            tlkapi_send_string_data(DEB_BIG_SYNC_EN, "bis rdy to rcv bigctrl", 0, 0);
        }
    }
    else{
        big_end = 1;
    }

    if( blc_rf_pa_cb){  blc_rf_pa_cb(PA_TYPE_OFF); }// turn off PA ~20us after RF done

    DBG_FANQH_CHN4_LOW;
    if(big_end){
        blt_bigSync_post();
    }

#if(HW_AES_CCM_ALG_EN)
//  if(blt_pBis->bisCryptCtrl.enable)
    {
        reg_rf_tx_mode2 &= ~FLD_TLK_CRYPT_ENABLE;
    }
#endif


    return 1;
}


ble_sts_t   blc_ll_bigCreateSync(u8  big_handle,        /* Used to identify the BIG */
                                 u16 sync_handle,       /* Identifier of the periodic advertising train */
                                 u8  enc,               /* Encryption flag */
                                 u8  broadcast_code[16],/* The code used to derive the session key that is used to encrypt and decrypt BIS payloads */
                                 u8  mse,               /* The Controller can schedule reception of any number of subevents up to NSE */
                                 u16 big_sync_timeout,  /* Synchronization timeout for the BIG */
                                 u8  num_bis,           /* Total number of BISes to synchronize */
                                 u8 *bis)               /* List of indices of BISes */
{
    (void)bis; //unused, remove warning
    u8 bigCreateSyncBuff[sizeof(hci_le_bigCreateSyncParams_t) - 4 + LL_BIS_IN_PER_BIG_SYNC_NUM_MAX];
    hci_le_bigCreateSyncParams_t* pBigCreateSyncParam = (hci_le_bigCreateSyncParams_t*)bigCreateSyncBuff;
    pBigCreateSyncParam->big_handle = big_handle;   /* Used to identify the BIG */
    pBigCreateSyncParam->sync_handle = sync_handle; /* Identifier of the periodic advertising train */
    pBigCreateSyncParam->enc = enc;                 /* Encryption flag */
    smemcpy(pBigCreateSyncParam->broadcast_code, broadcast_code, 16);
    pBigCreateSyncParam->mse = mse;                 /* The Controller can schedule reception of any number of subevents up to NSE */
    pBigCreateSyncParam->big_sync_timeout = big_sync_timeout;   /* Synchronization timeout for the BIG */
    pBigCreateSyncParam->num_bis = num_bis;         /* Total number of BISes to synchronize */

    for(int i = 0; i < (pBigCreateSyncParam->num_bis); ++i){
        pBigCreateSyncParam->bis[i] = i + 1;            /* List of indices of BISes */
    }

    return blc_hci_le_bigCreateSync(pBigCreateSyncParam);
}


ble_sts_t   blc_ll_bigTerminateSync(u8 bigHandle)
{
    u8 retParam[2];

    return blc_hci_le_bigTerminateSync(bigHandle, retParam);
}


static bool blt_ll_isBigCreateSyncParamsValid(hci_le_bigCreateSyncParams_t *pParam)
{
    const u8 MAX_BIG_HANDLE = 0xEF;
    const u16 MAX_SYNC_HANDLE = 0x0EFF;
    const u8 MAX_ENCRYPTION = 0x01;
    const u8 MAX_MSE = 0x1F;
    const u16 MIN_BIG_SYNC_TIMEOUT = 0x000A;
    const u16 MAX_BIG_SYNC_TIMEOUT = 0x4000;
    const u8 MIN_BIS = 1;
    const u8 MAX_BIS = 0x1F;

    if (pParam->big_handle > MAX_BIG_HANDLE)
    {
        tlkapi_send_string_data(DEB_BIG_SYNC_EN,"big_handle out of range", &pParam->big_handle, 1);
        return FALSE;
    }
    if (pParam->sync_handle > MAX_SYNC_HANDLE)
    {
        tlkapi_send_string_data(DEB_BIG_SYNC_EN,"sync_handle out of range", &pParam->sync_handle, 2);
        return FALSE;
    }
    if (pParam->enc > MAX_ENCRYPTION)
    {
        tlkapi_send_string_data(DEB_BIG_SYNC_EN,"Encryption flag out of range", &pParam->enc, 1);
        return FALSE;
    }
    if (pParam->mse > MAX_MSE)
    {
        tlkapi_send_string_data(DEB_BIG_SYNC_EN,"mse out of range", &pParam->mse, 1);
        return FALSE;
    }
    if ((pParam->big_sync_timeout < MIN_BIG_SYNC_TIMEOUT) || (pParam->big_sync_timeout > MAX_BIG_SYNC_TIMEOUT))
    {
        tlkapi_send_string_data(DEB_BIG_SYNC_EN,"big_sync_timeout out of range", &pParam->big_sync_timeout, 2);
        return FALSE;
    }
    if ((pParam->num_bis < MIN_BIS) || (pParam->num_bis > MAX_BIS))
    {
        tlkapi_send_string_data(DEB_BIG_SYNC_EN,"num_bis out of range", &pParam->num_bis, 1);
        return FALSE;
    }

    return TRUE;
}



/**
 * The HCI_LE_BIG_Create_Sync command is used to synchronize to a BIG
 * described in the periodic advertising train specified by the Sync_Handle
 * parameter.
 */
ble_sts_t   blc_hci_le_bigCreateSync(hci_le_bigCreateSyncParams_t* pCmdParam)
{

    tlkapi_send_string_data(IUT_HCI_LOG_EN, "[hci][cmd] cigCreatSync", (u8*)pCmdParam, sizeof(hci_le_bigCreateSyncParams_t)+2);

    /*
     * The parameters are not in the specified range, the Controller shall
     * return the error code Unsupported Feature or Parameter Value(0x11).
     */
    if(!blt_ll_isBigCreateSyncParamsValid(pCmdParam)){
        return HCI_ERR_PARAM_OUT_OF_MANDATORY_RANGE;
    }

    /*
     * If the Host sends this command when the Controller is in the process of
     * synchronizing to any BIG, i.e. the HCI_LE_BIG_Sync_Established event has
     * not been generated, the Controller shall return the error code Command
     * Disallowed (0x0C).
     */
    if(blt_ll_isBigSynchronizing()){
        tlkapi_send_string_data(DEB_BIG_SYNC_EN,"The Controller is in the process of synchronizing to another BIG", 0, 0);
        return HCI_ERR_CMD_DISALLOWED;
    }


    /*
     * If the Host issues this command with a BIG_Handle for a BIG that is already
     * in use, the Controller shall return the error code Command Disallowed (0x0C).
     */
    if(blt_ll_findExistingBigSyncByBigHdl(pCmdParam->big_handle) != BIG_HANDLE_INVALID){
        tlkapi_send_string_data(DEB_BIG_SYNC_EN,"Host issues this command with a BIG_Handle for a BIG that is already in use", 0, 0);
        return HCI_ERR_CMD_DISALLOWED;
    }

    /*
     * if Num_BIS exceeds the maximum value supported by the Controller, it
     * shall return the error code Connection Rejected due to Limited Resources (0x0D).
     */
    if(pCmdParam->num_bis > blc_ll_getAvailBisNum(BIS_ROLE_SYNC)){
        tlkapi_send_string_data(DEB_BIG_SYNC_EN,"Num_BIS exceeds the maximum value supported by the Controller", 0, 0);
        return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
    }




    if(blt_ll_findBigSyncBySyncHdl(pCmdParam->sync_handle) != BIG_HANDLE_INVALID){
        tlkapi_send_string_data(DEB_BIG_SYNC_EN,"syncHandle already in use", &pCmdParam->sync_handle, 2);
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    //Obtain the BIGInfo field directly through Ext ADV, and perform Match through sync_handle
    bigInfor_para_t *pAcadBuff = blt_ll_getBigInorBySyncHdl(pCmdParam->sync_handle);
    if(pAcadBuff == NULL){
        tlkapi_send_string_data(DEB_BIG_SYNC_EN,"bigInfor can't find in sync handle", &pCmdParam->sync_handle, 2);
        return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
    }
    bool BigEncrypt = pAcadBuff->bigEncrypt;
    bigInfo_t *pBigInfo = (bigInfo_t *)( pAcadBuff->bigInfo);

    /*
     * If the Num_BIS parameter is greater than the total number of BISes in the BIG,
     * the Controller shall return the error code Unsupported Feature or Parameter
     * Value (0x11).
     */
    if(pCmdParam->num_bis > min(pBigInfo->numBis, LL_BIS_IN_PER_BIG_SYNC_NUM_MAX)){
        tlkapi_send_string_data(DEB_BIG_SYNC_EN,"invalid value for numBis", &pCmdParam->num_bis, 1);
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    /*
     * If the Encryption parameter set by the Host does not match the encryption
     * status of the BIG, the Controller shall return the error Encryption Mode Not
     * Acceptable (0x25).
     */
    if(pCmdParam->enc != BigEncrypt){
        tlkapi_send_string_data(DEB_BIG_SYNC_EN,"encryption mode mismatch", 0, 0);
        return HCI_ERR_ENCRYPT_MODE_NOT_ACCEPTABLE;
    }


    foreach(i, pCmdParam->num_bis){
        if ((pCmdParam->bis[i] < 1) || (pCmdParam->bis[i] > LL_BIS_IN_PER_BIG_SYNC_NUM_MAX)){
            tlkapi_send_string_data(DEB_BIG_SYNC_EN,"at index out of range", &i, 1);
            return HCI_ERR_PARAM_OUT_OF_MANDATORY_RANGE;
        }
    }
    tlkapi_send_string_data(DEB_BIG_SYNC_TIMESTAM_EN,"BIS = ", &pCmdParam->bis, pCmdParam->num_bis);

    /*
     * Allocate new Big Sync context
     */
    u8 big_idx = blt_ll_AllocateNewBigSyncHdl(pCmdParam->big_handle);
    if(big_idx == BIG_HANDLE_INVALID){//update latest_pBigSync if allocate successfully
        tlkapi_send_string_data(DEB_BIG_SYNC_EN,"insufficient BIG SYNC context", 0, 0);
        return HCI_ERR_MEM_CAP_EXCEEDED;
    }

    //fanqh if user define the PDU buffer can't cover the maxPdu, return error
    if(pBigInfo->maxPdu > blmsParam.bisSyncRfLenMax){
        return HCI_ERR_PARAM_OUT_OF_MANDATORY_RANGE;
    }

    //For sequential arrangement: BIG_Control_Offset = Num_BIS * BIS_Spacing
    //For interleaved arrangement: Sub_Interval >= Num_BIS * BIS_Spacing
    //Parameters check TODO:

    /*
     * The BIG_Sync_Timeout parameter specifies the maximum permitted time
     * between successful receptions of BIS PDUs. If this time is exceeded,
     * synchronization is lost. When the Controller establishes synchronization and if
     * the BIG_Sync_Timeout set by the Host is less than 6 * ISO_Interval, the
     * Controller shall set the timeout to 6 * ISO_Interval.
     */
    u32 bigSyncTimeoutUs = pCmdParam->big_sync_timeout*10000;
    u32 minBigSyncTimeoutUs = pBigInfo->isoItvl*1250*6;
    if(bigSyncTimeoutUs < minBigSyncTimeoutUs){
        bigSyncTimeoutUs = minBigSyncTimeoutUs;
    }
    latest_pBigSync->bigSyncTimeoutUs = bigSyncTimeoutUs;
    latest_pBigSync->bigSyncToleranceTime = 0;  //TODO: if PM enable, this value change
    latest_pBigSync->sync_handle = pCmdParam->sync_handle;
    latest_pBigSync->mse =(pCmdParam->mse)? pBigInfo->nse:min(pCmdParam->mse,pBigInfo->nse);//todo not use
    latest_pBigSync->sync_bis_num = pCmdParam->num_bis;
    smemcpy(latest_pBigSync->bisIdx, pCmdParam->bis, pCmdParam->num_bis);

    //copy BIGInfo, It's important to get BIGInfo!!!
    smemcpy(&latest_pBigSync->BigInfor, pBigInfo, BigEncrypt ? 57 : 33);

    latest_pBigSync->bis_cnt = pBigInfo->numBis;
    latest_pBigSync->sdu_intvl = pBigInfo->sduItvl; //unit: us
    latest_pBigSync->max_sdu = pBigInfo->maxSdu;
    latest_pBigSync->max_pdu = pBigInfo->maxPdu;
    latest_pBigSync->phy = BIT((pBigInfo->chm37Phy3[4]>>5)&0x07); //BIT(0): LE 1M; BIT(1): LE 2M; BIT(3): LE Coded PHY
    latest_pBigSync->framing = (pBigInfo->bisPldCnt39Framing1[4]>>7)&0x01;
    latest_pBigSync->packing = pBigInfo->bisSpacing > pBigInfo->subItvl ? PACK_SEQUENTIAL : PACK_INTERLEAVED;
    latest_pBigSync->bn = pBigInfo->bn;   //The number of new payloads in each interval for each BIS.
    latest_pBigSync->irc = pBigInfo->irc; //The number of times the scheduled payload(s) are transmitted in a given event.
    latest_pBigSync->pto = pBigInfo->pto; //Offset used for pre-transmissions
    if(pBigInfo->nse/pBigInfo->bn == pBigInfo->irc){ /* If IRC = GC then PTO shall be ignored. */
        latest_pBigSync->pto = 0;
    }
    else if((pBigInfo->nse/pBigInfo->bn != pBigInfo->irc) && pBigInfo->pto == 0){
        latest_pBigSync->pto = 1;
    }

    latest_pBigSync->nse = pBigInfo->nse;
    latest_pBigSync->encrypt = BigEncrypt;
    latest_pBigSync->seedAccessAddress = pBigInfo->seedAA;
    latest_pBigSync->baseCrcInit = pBigInfo->baseCrcInit;
    latest_pBigSync->iso_itvl = pBigInfo->isoItvl; //unit: 1.25 ms, Time Range: 5 ms to 4 s

    /* The Access Address for each BIS and for the BIG Control logical link (see
     * Section 4.4.6.7) in a BIG shall be derived from the SAA for that BIG.
     * For each BIS logical transport, the Access Address shall be equal to the SAA
     * bit-wise XORed with a diversifier word (DW) for that logical transport derived
     * from a Diversifier (D) as follows:
     *      D = ((35 * n) + 42) MOD 128 where n is the BIS number, or 0 for the BIG Control logical link
     *      DW = 0bD0D0D0D0D0D0D1D6_D10D5D40D3D20_00000000_00000000 */
    latest_pBigSync->scAccessCode = blt_ll_bis_getAccessCode(latest_pBigSync->seedAccessAddress, 0); //big control AccessCode
    /* For every Broadcast Isochronous PDU, the shift register shall be preset with the
     * BaseCRCInit value from the BIGInfo data (see Section 4.4.6.11) in the most
     * significant 2 octets and the BIS_Number for the specific BIS in the least
     * significant octet. For BIG Control PDUs, the least significant octet shall be 0. */
    latest_pBigSync->scCrcInit    = (latest_pBigSync->baseCrcInit << 8) | 0;


    //tlkapi_send_string_data(DEB_BIG_SYNC_EN,"bigAccessAddr", &latest_pBigSync->scAccessCode, 4);
    //tlkapi_send_string_data(DEB_BIG_SYNC_EN,"bigCrcInit", &latest_pBigSync->scCrcInit, 4);

    //  2M PHY   :     (rf_len + 11) * 4
    // Coded PHY :  = 376 + (rf_len*8+43)*S
    u32 rx_max_us;
    u32 rx_max_big_ctrl_us; //TX max PDU translate time, uS
    u8  curBisPhy;
    //TODO: here only consider S8 here, need add S2 later
    u8  coding_ind = bltPHYs.dft_CI ? bltPHYs.dft_CI : LE_CODED_S8;
    u8 ctrl_pdu_len_max = 8;//big_chmInd_data_t

    u8 mic_len = 0;
    if(BigEncrypt){
        mic_len = 4;
    }

    if(latest_pBigSync->phy & PHY_PREFER_1M){
        curBisPhy = BLE_PHY_1M; //The Controller only supports asymmetric PHYs.
        rx_max_us = (latest_pBigSync->max_pdu + mic_len + 10) * 8; //mic 4 len
        rx_max_big_ctrl_us = (ctrl_pdu_len_max + mic_len + 10) * 8;              //mic 4 len  : 2120us
    }
    else if(latest_pBigSync->phy & PHY_PREFER_2M){
        curBisPhy = BLE_PHY_2M;
        rx_max_us = (latest_pBigSync->max_pdu + mic_len + 11) * 4; //mic 4 len
        rx_max_big_ctrl_us = (ctrl_pdu_len_max + mic_len + 11) * 4;              //mic 4 len
    }
    else{
        curBisPhy = BLE_PHY_CODED;  //dft: LE_CODED_S8
        rx_max_us = 376 + ((latest_pBigSync->max_pdu + mic_len)*64 + 43) * coding_ind; //mic 4 len
        rx_max_big_ctrl_us =  376 + ((ctrl_pdu_len_max + mic_len)*64 + 43) * coding_ind;            //mic 4 len
    }

    latest_pBigSync->curBisPhy = curBisPhy;
    latest_pBigSync->codingInd = coding_ind;

    u32 big_iso_pdu_length_total_us;
    if(latest_pBigSync->packing == PACK_SEQUENTIAL){ // sequential
        //For sequential arrangement: BIG_Control_Offset = Num_BIS * BIS_Spacing
        big_iso_pdu_length_total_us = pBigInfo->bisSpacing * latest_pBigSync->bis_cnt;
        latest_pBigSync->se_length_tick = pBigInfo->subItvl * SYSTEM_TIMER_TICK_1US;
    }
    else{ //interleaved
        //For interleaved arrangement: BIG_Control_Offset = NSE * Sub_Interval
        big_iso_pdu_length_total_us = pBigInfo->subItvl * latest_pBigSync->nse;
        latest_pBigSync->se_length_tick = pBigInfo->bisSpacing * SYSTEM_TIMER_TICK_1US;
    }

    //The BIG_Sync_Delay = (Num_BIS - 1) * BIS_Spacing + (NSE - 1) * Sub_Interval + MPT.
    latest_pBigSync->big_sync_delay_us = (latest_pBigSync->bis_cnt - 1) * pBigInfo->bisSpacing + (latest_pBigSync->nse - 1) * pBigInfo->subItvl + rx_max_us;
    latest_pBigSync->iso_pdu_task_us = big_iso_pdu_length_total_us;
    latest_pBigSync->MPT = rx_max_us;

    tlkapi_send_string_u32s(DEB_BIG_SYNC_TIMESTAM_EN, "Ref: Creat", latest_pBigSync->big_sync_delay_us, latest_pBigSync->iso_itvl*1250, latest_pBigSync->sdu_intvl, 0);



    //lock the periodic adv update the BigInofor
    st_pda_sync_t *pda_sync = &pdAsync_tbl[ pCmdParam->sync_handle & BLT_SYNC_IDX_MARK];
    pda_sync->bigInfor_para.creating_bisSync_flag =1;

    //todo 39 bits
    u32 bisPldCnt = pBigInfo->bisPldCnt39Framing1[0] | (pBigInfo->bisPldCnt39Framing1[1]<<8) | (pBigInfo->bisPldCnt39Framing1[2]<<16)
                        |(pBigInfo->bisPldCnt39Framing1[3]<<24);


    latest_pBigSync->bigEventCnt = (u32)bisPldCnt/pBigInfo->bn;

    tlkapi_send_string_u32s(DEB_BIG_SYNC_TIMESTAM_EN, "CreatSync BigEvtCnt", latest_pBigSync->bigEventCnt, bisPldCnt,0,0);


    //tlkapi_send_string_data(DEB_BIG_SYNC_EN,"bigEventCnt", &latest_pBigSync->bigEventCnt, 4);

    smemcpy(latest_pBigSync->chnParam.map.chmTbl, pBigInfo->chm37Phy3, 4);
    latest_pBigSync->chnParam.map.chmTbl[4] = pBigInfo->chm37Phy3[4]&0x1F;

    //tlkapi_send_string_data(DEB_BIG_SYNC_EN,"Channel Map Table", latest_pBigSync->chnParam.map.chmTbl, 5);

    latest_pBigSync->chnIdentifier = (latest_pBigSync->scAccessCode>>16) ^ (latest_pBigSync->scAccessCode&0xffff);
    csa2_calculateMapInfo(&latest_pBigSync->chnParam.map);


    if(BigEncrypt){
        //broadcast_code: little--endian ==> big--endian
        swapX(pCmdParam->broadcast_code, latest_pBigSync->broadcast_code, 16); //The code used to derive the session key that is used to encrypt and decrypt BIS payloads.

        ///////////// Calculate the BIG/BIS session key //////////////////
        //IGLTK = h7("BIG1", Broadcast_Code)
        //GLTK = h6(IGLTK, "BIG2")
        //GSK = h8 (GLTK, GSKD, "BIG3")
        //GSK is used as the session key in the CCM algorithm when encrypting the BIG.
        u8 tmp_igltk[16], tmp_gltk[16], tmp_gskd[16];
        u8 BIG1[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x42,0x49,0x47,0x31};    //big-endian
        u8 BIG2[4]  = {0x42,0x49,0x47,0x32};                                                                //big-endian
        u8 BIG3[4]  = {0x42,0x49,0x47,0x33};                                                                //big-endian

        swapX(latest_pBigSync->BigInfor.gskd, tmp_gskd, 16);                            //GSKD: little--endian ==> big-endian
        blt_crypto_alg_h7 (tmp_igltk, BIG1, latest_pBigSync->broadcast_code);           //Broadcast_Code: big-endian
        blt_crypto_alg_h6 (tmp_gltk, tmp_igltk, BIG2);
        blt_crypto_alg_h8 (latest_pBigSync->bigCtrlCrypt.sk, tmp_gltk, tmp_gskd, BIG3); //Our SDK's AES CCM's sk need big--endian.
        //tlkapi_send_string_data(0,"BIG/BIS SK", latest_pBigSync->bigCtrlCrypt.sk, 16);

        ///////////// Calculate the BIG CTRL IV key //////////////////
        //The IV for a CIS or BIS is calculated from an IVbase and the Access Address of the CIS or BIS respectively.
        //Generation of IV for a CIS or BIS: IV[31:0] shall equal IVbase[31:0] XORed with the Access Address of the
        //CIS or BIS while IV [63:32] shall equal IVbase[63:32].
        //The IV for a BIG control logical link shall be determined in the same way as for a BIS.
        u32 iv[2] = { 0};
        //Initialize the IV          //IV: little--endian, our SDK's AES CCM's iv need little--endian
        smemcpy(&iv, latest_pBigSync->BigInfor.giv, 8); //For a BIS, the IVbase shall be set to the value of GIV contained in the BIGInfo.
        iv[0] ^= latest_pBigSync->scAccessCode;         //Generation of IV for a CIS or BIS
        smemcpy(latest_pBigSync->bigCtrlCrypt.nonce.iv, &iv, 8);
        //tlkapi_send_string_data(0,"BIG CTRL IV", latest_pBigSync->bigCtrlCrypt.nonce.iv, 8);

        //Open BIG Control PDU encryption enable flg
        latest_pBigSync->bigCtrlCrypt.enable = 1; //Enable encryption
        latest_pBigSync->bigCtrlCrypt.mic_fail = 0;
    }
    else{
        smemset(latest_pBigSync->broadcast_code, 0, 16); //clear Broadcast Code
        smemset(latest_pBigSync->BigInfor.giv, 0, 8);    //clear GIV
        smemset(latest_pBigSync->BigInfor.gskd, 0, 16);  //clear GSKD
        latest_pBigSync->bigCtrlCrypt.enable = 0;
    }


    ///// find available BIS for current BIG ///////////////////
    int new_bis_cnt = 0;
    ll_bis_t *cur_pBis = NULL;
    u8 bis_order_tbl[LL_BIS_IN_PER_BIG_SYNC_NUM_MAX];
    u8 bis_order_cnt = 0;
    latest_pBigSync->bis_alloc_msk = 0;
    latest_pBigSync->bigRxLostCnt =0;


    for(int i = bltBisMng.maxNum_bisBcst; i< bltBisMng.maxNum_bisTotal; i++){
        cur_pBis = (ll_bis_t *) (global_pBis + i);

        if(cur_pBis->bis_occupied == 0){
            cur_pBis->bis_occupied = 1;
            cur_pBis->bisSuccessiveMiss = 0; //QW
            bltBisMng.curNum_bisSync++;  //update current BIS number

            cur_pBis->link_big_handle = pCmdParam->big_handle;
            cur_pBis->big_idx = big_idx;
            cur_pBis->dpID = 0xff;
            cur_pBis->bis_dapth_setup = 0;
            cur_pBis->bisSduOut_wptr = cur_pBis->bisSduOut_rptr = 0;
            cur_pBis->lossFlag = 0;
            cur_pBis->rx_first_pdu = 0;
            cur_pBis->rxSduStatus = SDU_STATE_NEW;
            blt_ll_ResetBisRxFifo(cur_pBis->bis_handle);


//          cur_pBis->curBisPhy = curBisPhy;
//          cur_pBis->codingInd = coding_ind; //S2 or S8: LE_CODED_S2 / LE_CODED_S8
            cur_pBis->sub_interval_us   = pBigInfo->subItvl;
            cur_pBis->sub_interval_tick = pBigInfo->subItvl * SYSTEM_TIMER_TICK_1US;
            cur_pBis->bis_spacing_us    = pBigInfo->bisSpacing;
            cur_pBis->bis_spacing_tick  = pBigInfo->bisSpacing * SYSTEM_TIMER_TICK_1US;

            cur_pBis->nse = pBigInfo->nse;
            cur_pBis->bn  = pBigInfo->bn;
            cur_pBis->irc = pBigInfo->irc;
            cur_pBis->pto = pBigInfo->pto;
            cur_pBis->bisSyncIdx = latest_pBigSync->bisIdx[new_bis_cnt];

            latest_pBigSync->bis_handle[new_bis_cnt] = cur_pBis->bis_handle;

            bis_order_tbl[bis_order_cnt++] = i;
            latest_pBigSync->bis_alloc_msk |= BIT(i);
            cur_pBis->numSdu2Pdu = ((latest_pBigSync->bn * latest_pBigSync->sdu_intvl) + (latest_pBigSync->iso_itvl*1250) -1 )/(latest_pBigSync->iso_itvl*1250);

//          new_bis_cnt++; //0 for BIG Control, other s for BIS use
            cur_pBis->bisAccessAddr = blt_ll_bis_getAccessCode(latest_pBigSync->seedAccessAddress, pCmdParam->bis[new_bis_cnt]); //bisAccessCode
            cur_pBis->bisCrcInit = (latest_pBigSync->baseCrcInit << 8) | pCmdParam->bis[new_bis_cnt];

            tlkapi_send_string_u32s(0, "ac,crc", cur_pBis->bisAccessAddr,cur_pBis->bisCrcInit, latest_pBigSync->seedAccessAddress, pCmdParam->bis[new_bis_cnt]);

            cur_pBis->rxPldLimite =0;

            //tlkapi_send_string_data(DEB_BIG_SYNC_EN,"bisAccessAddr", &cur_pBis->bisAccessAddr, 4);
            //tlkapi_send_string_data(DEB_BIG_SYNC_EN,"bisCrcInit", &cur_pBis->bisCrcInit, 4);


            cur_pBis->chnIdentifier = (cur_pBis->bisAccessAddr >> 16) ^ (cur_pBis->bisAccessAddr >> 0);

            //////////////////////////////////////////////////
            // Encryption parameters init
            //////////////////////////////////////////////////
            if(BigEncrypt){
                //The IV for a CIS or BIS is calculated from an IVbase and the Access Address of the CIS or BIS respectively.
                //Generation of IV for a CIS or BIS: IV[31:0] shall equal IVbase[31:0] XORed with the Access Address of the
                //CIS or BIS while IV [63:32] shall equal IVbase[63:32].
                u32 iv[2] = { 0};
                //Initialize the IV          //BIS IV: little--endian, our SDK's AES CCM's iv need little--endian
                smemcpy(&iv, latest_pBigSync->BigInfor.giv, 8);//For a BIS, the IVbase shall be set to the value of GIV contained in the BIGInfo.
                iv[0] ^= cur_pBis->bisAccessAddr; //Generation of IV for a CIS or BIS
                smemcpy(cur_pBis->bisCryptCtrl.nonce.iv, &iv, 8);
                //tlkapi_send_string_data(0,"IV", cur_pBis->bisCryptCtrl.nonce.iv, 8);

                //Initialize the session key //BIS's SK equal to BIG CTRL's SK: big--endian, our SDK's AES CCM's sk need big--endian
                smemcpy (cur_pBis->bisCryptCtrl.sk, latest_pBigSync->bigCtrlCrypt.sk, 16);

                //Open BIS encryption enable flg
                cur_pBis->bisCryptCtrl.enable = 1; //Enable encryption
                cur_pBis->bisCryptCtrl.mic_fail = 0;
            }
            else{
                cur_pBis->bisCryptCtrl.enable = 0;
            }

            new_bis_cnt++; //0 for BIG Control, other s for BIS use
            if(new_bis_cnt >= latest_pBigSync->sync_bis_num){
                break;
            }
        }
    }


    ////////////////////  BIS arrangement map ////////////////////////
    latest_pBigSync->bis_total_se_num = 0;

    latest_pBigSync->biss_arrgmtMap_msk = 0;
    u8 bis_sync_sel = 0;

    tlkapi_send_string_u32s(DEB_BIG_SYNC_TIMESTAM_EN, "bissnc", latest_pBigSync->sync_bis_num,  pCmdParam->bis[0],  pCmdParam->bis[1],  latest_pBigSync->MPT);


    for(int i = 0; i<latest_pBigSync->bis_cnt; i++){

        u8 need_sync  = 0;
        u8 sync_bis_index = (bis_sync_sel<latest_pBigSync->sync_bis_num)? pCmdParam->bis[bis_sync_sel]:0xff;

        if(sync_bis_index==(i+1)){
            bis_sync_sel++;
            need_sync = 1;
        }

        for(int j=0; j<latest_pBigSync->nse ;j++){

            if(latest_pBigSync->packing == PACK_SEQUENTIAL){ //Sequential: e.g.: 112233
                    latest_pBigSync->bis_arrgmt_map[latest_pBigSync->bis_total_se_num + j] = bis_order_tbl[bis_sync_sel-1];
                    if(need_sync){
                        latest_pBigSync->biss_arrgmtMap_msk |=  BIT((i)*latest_pBigSync->nse+j);
                    }
            }
            else{ //== PACK_INTERLEAVED  //Interleaved: e.g.: 123123
                    latest_pBigSync->bis_arrgmt_map[i + j * latest_pBigSync->bis_cnt] = bis_order_tbl[bis_sync_sel-1];
                    if(need_sync){
                        latest_pBigSync->biss_arrgmtMap_msk  |= BIT((i) + j * latest_pBigSync->bis_cnt);
                    }
            }
        }

        tlkapi_send_string_u32s(0, "sync", i,latest_pBigSync->bis_cnt,  bis_order_tbl[i],latest_pBigSync->nse);
        latest_pBigSync->bis_total_se_num += latest_pBigSync->nse;
    }


    tlkapi_send_string_data(DEB_BIG_SYNC_TIMESTAM_EN, "sync total SE", &latest_pBigSync->bis_total_se_num, 1);
    tlkapi_send_string_data(DEB_BIG_SYNC_EN, "sync tskMap",latest_pBigSync->bis_arrgmt_map, LL_SE_IN_PER_BIG_SYNC_NUM_MAX);
    tlkapi_send_string_data(DEB_BIG_SYNC_EN, "sync tskMsk",&latest_pBigSync->biss_arrgmtMap_msk, 4);
    tlkapi_send_string_u32s(0, "create big sync", latest_pBigSync->sync_bis_num, latest_pBigSync->bis_handle[0],latest_pBigSync->bis_handle[1], bltBisMng.curNum_bisSync);

    //tlkapi_send_string_data(DEB_BIG_SYNC_EN,"BIS arrangement map", latest_pBigSync->bis_arrgmt_map, latest_pBigSync->bis_total_se_num);

    /*
     *Transport_Latency_BIG = BIG_Sync_Delay + PTO * (NSE / BN - IRC) * ISO_Interval + ISO_Interval + SDU_Interval
     */
    latest_pBigSync->transLatency_us = latest_pBigSync->big_sync_delay_us +
                                     ((latest_pBigSync->pto*(latest_pBigSync->nse/latest_pBigSync->bn - latest_pBigSync->irc)) + 1) * (latest_pBigSync->iso_itvl*1250)
                                     + latest_pBigSync->sdu_intvl;

    //////////////////////////// BIG slot Timing && task create Start //////////////////////////
    u32 biginfo_offset;
    if(pBigInfo->bigOffsetUnits == BIG_PDU_BIG_OFFSET_UNITS_30_US){
        biginfo_offset = pBigInfo->bigOffset * 30;
        latest_pBigSync->bigSyncToleranceTime = 30;
    }
    else{ // == BIG_PDU_BIG_OFFSET_UNITS_300_US
        biginfo_offset = pBigInfo->bigOffset * 300;
        latest_pBigSync->bigSyncToleranceTime = 300;
    }

    if(latest_pBigSync->packing == PACK_SEQUENTIAL){
        latest_pBigSync->bigStartOffset_se_num = (pCmdParam->bis[0]-1)*latest_pBigSync->nse;
    }
    else{
        latest_pBigSync->bigStartOffset_se_num = (pCmdParam->bis[0]-1);
    }

    latest_pBigSync->bigs_ap_offset_us = (pCmdParam->bis[0]-1) * pBigInfo->bisSpacing;
    tlkapi_send_string_u32s(DEB_BIG_SYNC_EN, "bisSyncAPOffset", latest_pBigSync->bigs_ap_offset_us, pCmdParam->bis[0]-1,0,0);

    u32 r = irq_disable();

    latest_pBigSync->bSlot_interval_big = latest_pBigSync->iso_itvl*2;  //1.25mS -> 625 uS
    latest_pBigSync->sSlot_interval_big = latest_pBigSync->bSlot_interval_big<<5;  //625 uS * 32

#if(1)

    latest_pBigSync->sSlot_mark_big = pAcadBuff->sSlot_idx_Rx + (biginfo_offset+latest_pBigSync->bigs_ap_offset_us)*SSLOT_US_REVERSE - 8;
    latest_pBigSync->bSlot_mark_big = bltSche.bSlot_idx_start + (latest_pBigSync->sSlot_mark_big>>5);

    latest_pBigSync->bSync_expect_tick = pAcadBuff->bigInfor_rx_tick + (biginfo_offset + latest_pBigSync->bigs_ap_offset_us) *SYSTEM_TIMER_TICK_1US;
    latest_pBigSync->bigs_1st_expect_tick = latest_pBigSync->bSync_expect_tick;


    latest_pBigSync->sSlot_duration_big = (big_iso_pdu_length_total_us + rx_max_big_ctrl_us\
                    + latest_pBigSync->bigSyncToleranceTime)*SSLOT_US_REVERSE +  1 + SLOT_PROCESS_MAX_SSLOT_NUM + BYNC_FIRST_EARLY_SLOT_NUM;
    tlkapi_send_string_data(DEB_BIG_SYNC_EN, "sSlot_bsync_dur", &latest_pBigSync->sSlot_duration_big, 2);

    latest_pBigSync->bSlot_mark_big -= latest_pBigSync->bSlot_interval_big;
    latest_pBigSync->sSlot_mark_big -= latest_pBigSync->sSlot_interval_big ;

#else
    //TODO: ExtScan feature will support latter, process this in future
    (void)biginfo_offset;  //remove compiler warning
#endif

    blt_sche_addTaskMask(pm_check_info ? TSKMSK_BIG_SYNC_0 << big_idx : 0);
    blt_sche_addUpdate(SLOT_UPDT_BIS_BSYNC_CREATE);

    latest_pBigSync->bigSyncConnTick = clock_time();
    latest_pBigSync->big_state = BIG_SYNCHRONIZING;
    latest_pBigSync->last_cssn = 0xFF;
    latest_pBigSync->bigCtrlPktDecPending = NULL;

    latest_pBigSync->bigTermSyncFlag = 0;
    latest_pBigSync->irq_bigSyncEvt = 0;
    latest_pBigSync->bigSyncEvtStatus = 0;

    irq_restore(r);

    ////////////////////////////////////////////////////////////////////////////////
    //has been called blt_sche_addUpdate(SLOT_UPDT_BIS_BSYNC_CREATE), need to rebuild.if far away from next task, need to insert.
    if(blms_state == BLMS_STATE_NONE ||  (blms_state & BLMS_STATE_PRICHN_SCAN_S)){

        //////// immediately  to process/////////
        u32 cur_tick = clock_time();
        if(tick1_exceed_tick2(systimer_get_irq_capture(), cur_tick + 8*SYSTEM_TIMER_TICK_1MS)){ //if next task is far away from current time, such as 800ms
            systick_irq_trigger = SYS_IRQ_TRIG_SCHE_INSERT;
            systimer_set_irq_capture(cur_tick + 1*SYSTEM_TIMER_TICK_1MS);
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////

    /* Priority preset value */
    blt_ll_set_interval_level(TSKOFT_BIG_SYNC + big_idx, latest_pBigSync->iso_itvl);
    ///////////////////////////// BIG slot Timing && task create End ///////////////////////////

    return BLE_SUCCESS;
}

/**
 * The HCI_LE_BIG_Terminate_Sync command is used to stop synchronizing or
 * cancel the process of synchronizing to the BIG identified by the BIG_Handle
 * parameter. The command also terminates the reception of BISes in the BIG
 * specified in the HCI_LE_BIG_Create_Sync command, destroys the associated
 * connection handles of the BISes in the BIG and removes the data paths for all
 * BISes in the BIG.
 */
ble_sts_t   blc_hci_le_bigTerminateSync(u8 bigHandle, u8* pRetParam)
{
    ble_sts_t status = BLE_SUCCESS;

    if(ll_big_bcst_mlp_task_cb && (ll_big_bcst_mlp_task_cb(FLAG_BIG_BRD_HANDLE_SEARCH, (void*)&bigHandle)!=BIG_HANDLE_INVALID)){
        //HCI/BIS/BV-01-C. need to search big broadcast's handle.
        //but no corresponding description found in core spec. TS spec also not find detailed description. need QingHua to confirm.
        status = HCI_ERR_CMD_DISALLOWED;
    }
    else if(blt_ll_findExistingBigSyncByBigHdl(bigHandle) == BIG_HANDLE_INVALID){
        /*
         * If the Host issues this command with a BIG_Handle that does not exist, the
         * Controller shall return the error code Unknown Advertising Identifier (0x42).
         */
        status = HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
    }
    else if(latest_pBigSync->big_state == BIG_SYNC_IN_IDLE){
        /*
         * If the Host issues this command for a BIG which it is neither synchronized to
         * nor in the process of synchronizing to, then the Controller shall return the error
         * code Command Disallowed (0x0C).
         */
        status = HCI_ERR_CMD_DISALLOWED;
    }

    u32 r = irq_disable();

    if(!status && latest_pBigSync->bigTermSyncFlag){//only status == BLE_SUCCESS,the variable latest_pBigSync will point the right address.
        //Already terminate, do nothing
        tlkapi_send_string_data(DEB_BIG_SYNC_EN,"Already terminate set, do nothing", 0, 0);
    }
    else if(status == BLE_SUCCESS){
        /*
         * If the Host attempts to terminate synchronization with a BIG while the process
         * of synchronization with that BIG is in progress (i.e.
         * HCI_LE_BIG_Sync_Established event has not been generated) the process of
         * synchronization shall stop, and the Controller shall generate the
         * HCI_LE_BIG_Sync_Established event to the Host with the error code
         * Operation Cancelled by Host (0x44).
         */
        if(latest_pBigSync->big_state == BIG_SYNCHRONIZING){


            st_pda_sync_t *pda_sync = &pdAsync_tbl[ latest_pBigSync->sync_handle & BLT_SYNC_IDX_MARK];
            pda_sync->bigInfor_para.creating_bisSync_flag =0;

            latest_pBigSync->bigTermSyncFlag = 1;
            latest_pBigSync->bigSyncEvtStatus = HCI_ERR_OP_CANCELLED_BY_HOST; //mark reason
            tlkapi_send_string_data(DEB_BIG_SYNC_EN,"hci big terminate rsn: CANCELLED_BY_HOST", 0, 0);
        }
        else{
            latest_pBigSync->bigTermSyncFlag = 2;
            latest_pBigSync->bigSyncEvtStatus = HCI_ERR_CONN_TERM_BY_LOCAL_HOST; //mark reason
            tlkapi_send_string_data(DEB_BIG_SYNC_EN,"hci big terminate rsn: TERM_BY_LOCAL_HOST", 0, 0);
        }
    }

    irq_restore(r);

    *pRetParam = status;
    *(pRetParam+1) = bigHandle;

    return status;
}


//0xFF: no existing BIG SYNC
//other: big_handle is same as existing BIG SYNC's
int         blt_ll_findExistingBigSyncByBigHdl(u8 big_handle)
{
    ll_big_sync_t *pBigSync = NULL;
    for(u8 i=0; i<bltBisMng.maxNum_bigSync; i++)  //find existing BIG SYNC
    {
        pBigSync = global_pBigSync + i;
        if(pBigSync->big_handle == big_handle)
        {
            latest_pBigSync = pBigSync;
            return i;
        }
    }

    return BIG_HANDLE_INVALID;
}


int         blt_ll_AllocateNewBigSyncHdl(u8 big_handle)
{
    ll_big_sync_t *pBigSync = NULL;
    for(u8 i=0; i<bltBisMng.maxNum_bigSync; i++)  //find new BIG
    {
        pBigSync = global_pBigSync + i;
        if(pBigSync->big_handle == BIG_HANDLE_INVALID) //if BIG_HANDLE_INVALID, this BIG SYNC can be allocated to new BIG handle
        {
            pBigSync->biss_arrgmtMap_msk =0;
            pBigSync->big_handle = big_handle;
            bltBisMng.curNum_bigSync ++;

            latest_pBigSync = pBigSync;
            return i;
        }
    }

    return BIG_HANDLE_INVALID;
}


bool        blt_ll_isBigSynchronizing(void)
{
    ll_big_sync_t *pBigSync = NULL;
    for(u8 i=0; i<bltBisMng.maxNum_bigSync; i++)  //find new BIG
    {
        pBigSync = global_pBigSync + i;
        //current BigSync enabled && Synchronizing
        if(pBigSync->big_handle != BIG_HANDLE_INVALID && pBigSync->big_state == BIG_SYNCHRONIZING){
            return TRUE;
        }

    }

    return FALSE;
}


int         blt_ll_findBigSyncBySyncHdl(u16 sync_handle)
{
    ll_big_sync_t *pBigSync = NULL;
    for(u8 i=0; i<bltBisMng.maxNum_bigSync; i++)  //find existing BIG SYNC
    {
        pBigSync = global_pBigSync + i;
        tlkapi_send_string_data(DEB_BIG_SYNC_EN,"fnd:syncHandle", &pBigSync->sync_handle,2);

        if(pBigSync->big_handle != BIG_HANDLE_INVALID && pBigSync->sync_handle == sync_handle){

            latest_pBigSync = pBigSync;
            tlkapi_send_string_data(DEB_BIG_SYNC_EN,"fnd:i", &i,1);
            return i;
        }
    }

    return BIG_HANDLE_INVALID;
}




ble_sts_t blc_ll_bisSync_iso_receive_mode(u16 connHandle, itest_payload_type_t pdu_type){

    ll_bis_t *pBis = blt_ll_findBisByHandle(connHandle);

    if(pBis==NULL){
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    if(!pBis->bn){
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    if(pBis->bis_dapth_setup & DATA_PATH_OUTPUT_FLAG){
        return HCI_ERR_CMD_DISALLOWED;
    }

    ll_big_sync_t *pBig =(ll_big_sync_t *) (global_pBigSync + pBis->big_idx);
    if(pBig->big_state != BIG_SYNCHRONIZED){
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    if(pBis->pBisTestParam==NULL){
        pBis->pBisTestParam = &gIsoTestPara[0];
    }
    else
    {
        if(pBis->pBisTestParam->isoTestMode==ISO_RECEIVE_MODE){
            return HCI_ERR_CMD_DISALLOWED;
        }
    }

    iso_test_param_t *isoTest = pBis->pBisTestParam;


    isoTest->isoTestMode = ISO_RECEIVE_MODE;
    isoTest->isoTest_payload_type = pdu_type;
    isoTest->recMode.failedCnt = 0;
    isoTest->recMode.missedCnt = 0;
    isoTest->recMode.successCnt = 0;
    isoTest->recMode.expectCnt = 0;

    pBis->bisSduOut_rptr = pBis->bisSduOut_wptr;
    blt_ll_ResetBisRxFifo(pBis->bis_handle);

    tlkapi_send_string_u32s(DBG_ISO_TEST_EN, "Enter isoTest Rec mode",connHandle, pdu_type, isoTest->isoTestMode,0);

    return BLE_SUCCESS;

}

ble_sts_t blc_hci_bisSync_iso_receive_test(hci_le_isoTestCmdParams_t *pCmdParam, hci_le_isoTestRetParams_t *pRetParam)
{
    ble_sts_t ret_status = blc_ll_bisSync_iso_receive_mode(pCmdParam->conn_handle, pCmdParam->payload_type);

    pRetParam->status = ret_status;
    pRetParam->conn_handle = pCmdParam->conn_handle;

    return ret_status;
}

ble_sts_t blc_ll_bisSync_iso_read_test_count_cmd(u16 connHandle, hci_le_isoRxTestStatusParam_t *pRet)
{

    iso_test_param_t *pIsoTest = NULL;
    ble_sts_t status = BLE_SUCCESS;
    ll_bis_t *pBis = blt_ll_findBisByHandle(connHandle);
    /*
     * If the Host issues this command with a connection handle that does not exist,
     * or the Connection_Handle command parameter is not associated with a CIS or
     * a BIS, the Controller shall return the error code Unknown Connection Identifier
     * (0x02)
     */
    if(pBis==NULL){

        status = HCI_ERR_UNKNOWN_CONN_ID;
    }
    else{

        ll_big_sync_t *pBig =(ll_big_sync_t *)(global_pBigSync + pBis->big_idx);;
        if(pBig->big_state != BIG_SYNCHRONIZED){
            status = HCI_ERR_UNKNOWN_CONN_ID;
        }
        else{
            pIsoTest = pBis->pBisTestParam;
            if((pIsoTest == NULL) || (pIsoTest->isoTestMode != ISO_RECEIVE_MODE)){
                status = HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
            }
        }
    }

    pRet->status = status;
    pRet->conn_handle = connHandle;


    if(status==BLE_SUCCESS)
    {
        pRet->failed_packet_count = pIsoTest->recMode.failedCnt;
        pRet->miss_packet_count = pIsoTest->recMode.missedCnt;
        pRet->received_packet_count = pIsoTest->recMode.successCnt;
    }
    else{
        pRet->failed_packet_count = 0;
        pRet->miss_packet_count = 0;
        pRet->received_packet_count = 0;
    }

    return status;
}

ble_sts_t blc_hci_bisSync_iso_read_test_count_cmd(hci_le_isoReadTestCountsCmdParams_t *pcmd, hci_le_isoRxTestStatusParam_t *pRet){

    return blc_ll_bisSync_iso_read_test_count_cmd(pcmd->conn_handle, pRet);
}

#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
int blt_bisSync_process(ll_bis_t *pBis)
{

    /******************************* Bis Sync ******************************************************************/
    u8 bis_sync_sel = (pBis->bis_handle & BLT_BIS_IDX_MSK) - bltBisMng.maxNum_bisBcst;

    ll_big_sync_t *pBigSync = global_pBigSync + pBis->big_idx;

    bis_rx_pdu_chain_t *pBisRxPduChain = &bltBisRxPduChain[bis_sync_sel];
    bis_rx_pdu_t *pBisPdu = pBisRxPduChain->pUsed;
    u32 curLLPldBoundary = (pBigSync->bigEventCnt -1)*pBis->bn;//;pBis->rxPldLimite


    if(pBis->lastPayloadNum + 1 <= curLLPldBoundary){

        if(((pBisPdu !=NULL) && ((pBis->lastPayloadNum + 1) == pBisPdu->payloadNum))){


            u32 r = irq_disable();
            u8 bak[255+18];
            smemcpy((u8*)bak, pBisPdu, 12+6+pBisPdu->rawData[DMA_RFRX_OFFSET_RFLEN]);//12(next+pldNum+tick)6 (DMA+header)
            bis_rx_pdu_t *bisRxPduBak = (bis_rx_pdu_t*)bak;
            blt_bis_deletePdu(pBisRxPduChain);
            irq_restore(r);

            rf_packet_ll_data_t *rf_pkt =  (rf_packet_ll_data_t*)bisRxPduBak->rawData;
            //BIS decrypt
            if (rf_pkt->llPhysChnPdu.llPduHdr.bisPduHdr.rf_len > 0 && pBis->bisCryptCtrl.enable){
                ble_crypt_para_t* pLeCryptCtrl = &pBis->bisCryptCtrl;
                pLeCryptCtrl->dec_pno = bisRxPduBak->payloadNum;

            #if(HW_AES_CCM_ALG_EN)
                rf_pkt->llPhysChnPdu.llPduHdr.bisPduHdr.rf_len -=4;
            #else
                /* The directionBit shall be set to 1 for Broadcast Isochronous PDUs sent by the Isochronous Broadcaster. */
                aes_enc_dec_busy = 1;
                u8 st = aes_ll_ccm_decryption(&rf_pkt->llPhysChnPdu, 1, CRYPT_NONCE_TYPE_BIS, pLeCryptCtrl);
                aes_enc_dec_busy = 0;

                if(st){  //decrypt err
                    pBis->bisCryptCtrl.mic_fail = 1;

                    pBigSync->bigTermSyncFlag = 2;
                    pBigSync->bigSyncEvtStatus = HCI_ERR_CONN_TERM_MIC_FAILURE; //mark lost reason
                    tlkapi_send_string_u32s(STACK_DUMP_EN, "LE BIG Sync lost: MIC Failure1", pBisPdu->payloadNum, rf_pkt->llPhysChnPdu.llPduHdr.bisPduHdr.rf_len, 0,0);

                    #if (DBG_DECRYPTION_ERR_EN)
                        BLMS_ERR_DEBUG(DBG_IAL_LOGIC, 0xAA990000);
                    #endif

                    return 1;
                }
            #endif
            }
            if((rf_pkt->llPhysChnPdu.llPduHdr.bisPduHdr.rf_len==0) && (pBigSync->framing==1)){

                pBis->lastPayloadNum = bisRxPduBak->payloadNum;
                tlkapi_send_string_u32s(DBG_IAL_EN, "PDU Empty",   pBis->lastPayloadNum, 0, 0, 0);
            }
            else{

                tlkapi_send_string_u32s(DBG_IAL_EN, "IAL loop",    pBisPdu->payloadNum, pBisPdu, pBisPdu->rawData[7], pBis->bis_handle);
                blt_ial_bisSync_reassemblePdu2Sdu(pBis, bisRxPduBak);
                pBis->lastPayloadNum = bisRxPduBak->payloadNum;
            }

        }
        else if(pBis->rx_first_pdu){//compensate bis rx PDU

            bis_rx_pdu_t bisRxPduPad;
            bisRxPduPad.payloadNum = pBis->lastPayloadNum + 1;

            bisRxPduPad.rawData[0] = 0; // mark lost PDU
            bisRxPduPad.rawData[DMA_RFRX_OFFSET_RFLEN] = 0;//rf_len = 0
            pBis->lastPayloadNum += 1;

            #if (SLEV_bis0_rx_padding_pldNum)
                log_event_irq(SL_STACK_BIS_RX_DATA_EN, (SLEV_bis0_rx_padding_pldNum + ((pBis->bis_handle&BLT_BIS_HANDLE)) - bltBisMng.maxNum_bisBcst));
            #endif

//          DBG_FANQH_CHN8_TOGGLE;
            tlkapi_send_string_u32s(DBG_IAL_EN, "PDU Empty/Lost",  pBis->lastPayloadNum, 0, 0, 0);
            blt_ial_bisSync_reassemblePdu2Sdu(pBis, &bisRxPduPad);
        }

    }





    return 0;
}



sdu_packet_t* blc_ll_popBisSyncRxSduData(u16 bis_connHandle)
{
    sdu_packet_t* sdu= NULL;
    u8 bis_sel = bis_connHandle & BLT_BIS_IDX_MSK;
    ll_bis_t *pBis = (ll_bis_t *)(global_pBis + bis_sel);

    //tlkapi_send_string_data(STACK_DUMP_EN, "bis_sel", &bis_sel, 2);

    if(pBis == NULL){
        //tlkapi_send_string_data(STACK_DUMP_EN, "pBis == NULL", 0, 0);
        return NULL;
    }

    ll_big_sync_t *pBigSync = (ll_big_sync_t*)(global_pBigSync + pBis->big_idx);

    if(pBigSync->big_state != BIG_SYNCHRONIZED){
        //tlkapi_send_string_data(STACK_DUMP_EN, "big_state != BIG_SYNCHRONIZED", 0, 0);
        return NULL;
    }


    if(pBis->bisSduOut_wptr != pBis->bisSduOut_rptr)
    {
        sdu = (sdu_packet_t*)(pBis->bis_sduOutBuf + sduBisMng.max_out_fifo_size * \
                                  ((pBis->bisSduOut_rptr&(sduBisMng.out_fifo_mask))));

        //tlkapi_send_string_data(STACK_DUMP_EN, "&sdu =", sdu, 4);

        pBis->bisSduOut_rptr++;
    }
    else{
        //tlkapi_send_string_data(STACK_DUMP_EN, "outSduFifoWptr == pBis->bisSduOut_rptr", 0, 0);
    }

    return sdu;
}

#endif  //end of LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER

