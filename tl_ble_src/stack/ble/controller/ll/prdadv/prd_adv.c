/********************************************************************************************************
 * @file    prd_adv.c
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

#if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING)
#if OS_SUP_EN
#include "stack/ble/os_sup/os_sup.h"
#include "stack/ble/os_sup/os_sup_stack.h"
#endif


_attribute_ble_data_retention_  ll_prdadv_mng_t     bltPrdA;

_attribute_ble_data_retention_  st_prd_adv_t        *global_pPerdadv = NULL;
_attribute_ble_data_retention_  st_prd_adv_t        *blt_pPerdadv = NULL;

_attribute_ble_data_retention_  rf_pkt_ext_adv_t    pkt_periodic = {0};

_attribute_ble_data_retention_  ll_perd_adv_acad_callback_t             perd_adv_biginfo_update_cb = NULL;




ble_sts_t   blc_ll_initPeriodicAdvModule_initPeriodicdAdvSetParamBuffer(u8 *pBuff, int num_periodic_adv)
{
    STATIC_ASSERT_FILE(PERD_ADV_PARAM_LENGTH == sizeof(st_prd_adv_t), prd_adv);

    #if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
        STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(st_prd_adv_t)), prd_adv);
    #endif

    memset(pBuff, 0, num_periodic_adv * PERD_ADV_PARAM_LENGTH);

#if (1) /* periodic adv feature && extInit module must be used TODO: rewrite */
    if(!(LL_FEATURE_MASK_0 & LL_FEATURE_MASK_LE_EXTENDED_ADVERTISING)){
        blc_ll_init2MPhyCodedPhy_feature(); //already enabled in ext_adv initialization
        blc_ll_initChannelSelectionAlgorithm_2_feature(); //already enabled in ext_adv initialization
    }
#else
    //blc_ll_init2MPhyCodedPhy_feature();               //already enabled in ext_adv initialization
    //blc_ll_initChannelSelectionAlgorithm_2_feature(); //already enabled in ext_adv initialization
#endif

    LL_FEATURE_MASK_0 |= LL_FEATURE_MASK_LE_PERIODIC_ADVERTISING;

    blmsParam.prdAdvModule_en = 1;

    ll_prd_adv_irq_task_cb = blt_prd_adv_interrupt_task;
    ll_prd_adv_mlp_task_cb = blt_prd_adv_mainloop_task;

    blt_reset_prd_adv();

    ////////////////////////////////////////////////////////////


    if( num_periodic_adv > TSKNUM_PERD_ADV ){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    global_pPerdadv = (st_prd_adv_t *)pBuff;

    bltPrdA.maxNum_perdAdv = num_periodic_adv;


    st_prd_adv_t *cur_pPerdadv;
    for(int i=0;i<num_periodic_adv; i++){
        cur_pPerdadv = (st_prd_adv_t *)(global_pPerdadv + i);

        cur_pPerdadv->prdadv_index = i;
        cur_pPerdadv->advSet_idx = INVALID_ADVSET_IDX;
        cur_pPerdadv->advHand_mark = INVALID_ADVHD_FLAG;

        cur_pPerdadv->link_big_handle =BIG_HANDLE_INVALID;
        cur_pPerdadv->acad_used = 0;

        //never changed RF data init_ed
        ll_prd_adv_ind_header_t * p_adv_prd_ind = (ll_prd_adv_ind_header_t* )&cur_pPerdadv->prd_adv_1stPkt;
        p_adv_prd_ind->type = LL_TYPE_AUX_SYNC_IND;
        p_adv_prd_ind->chan_sel = 0;  //"ChSel" only valid in ADV_IND/ADV_DIRECT_IND/CONNECT_IND, other packet set 0'b
        p_adv_prd_ind->txAddr = 0;  //clear
        p_adv_prd_ind->rxAddr = 0;  //clear
        //rf_len calculate later
        //ext_hdr_len calculate later
        p_adv_prd_ind->adv_mode = LL_EXTADV_MODE_NON_CONN_NON_SCAN;


        for(int j=0; j<PERD_ADV_FIFONUM; j++){
            cur_pPerdadv->schTsk_fifo[j].scheTask_oft = TSKOFT_PERD_ADV + i;
            cur_pPerdadv->schTsk_fifo[j].scheTask_idx = i;
            cur_pPerdadv->schTsk_fifo[j].scheTask_flg = TSKFLG_PERD_ADV;
        }

//      bltPriority[TSKOFT_PERD_ADV + i] = 0;  //debug

    }

    return BLE_SUCCESS;
}




_attribute_noinline_
void        blt_reset_prd_adv(void)
{
    st_prd_adv_t *cur_pPerdadv;
    for(int i=0; i<bltPrdA.maxNum_perdAdv; i++){
        cur_pPerdadv = (st_prd_adv_t *)(global_pPerdadv + i);
        cur_pPerdadv->advSet_idx = INVALID_ADVSET_IDX;
        cur_pPerdadv->advHand_mark = INVALID_ADVHD_FLAG;
        cur_pPerdadv->link_big_handle =BIG_HANDLE_INVALID;
        cur_pPerdadv->acad_used = 0;
        cur_pPerdadv->pda_tx.update_map = 0;
        cur_pPerdadv->prd_adv_en = 0;
        cur_pPerdadv->unfinished_advData = 0;
        cur_pPerdadv->curLen_perdAdvData = 0;
        cur_pPerdadv->prd_1st_pkt_dataLen = 0;
        cur_pPerdadv->include_ADI_flag = 0;
        cur_pPerdadv->prd_DID_changed = 0;

        cur_pPerdadv->chain_ind_num = 0;
        //......
    }

#if (ONLY_FOR_EBQ_TEST_LATER_REMOVE)
    bigExtAuxPda_conflictCtrl.acadInfor_changeCtrl = ACAD_NOT_CHANGE;
    bigExtAuxPda_conflictCtrl.pdaAdv_sendNum = 0;
#endif
}











void        blc_ll_initPeriodicAdvDataBuffer(u8 *perdAdvData, int max_len_perdAdvData)
{
    for(int i=0;i<bltPrdA.maxNum_perdAdv; i++){
        (global_pPerdadv+i)->maxLen_perdAdvData = max_len_perdAdvData;
        (global_pPerdadv+i)->dat_perdAdvData    = (u8*)(perdAdvData + max_len_perdAdvData*i);
    }
}


#if 0  //consider later
ble_sts_t   blc_ll_initPeriodicAdvDataBuffer_by_advHandle(u8 adv_handle, u8 *perdAdvData, int max_len_perdAdvData)
{
    st_prd_adv_t *cur_pPerdadv;
    for(int i=0;i<bltPrdA.maxNum_perdAdv; i++){
        cur_pPerdadv = (st_prd_adv_t *)(global_pPerdadv + i);
        if( cur_pPerdadv->advHand_mark == adv_handle ){  //existing ADV set match
            cur_pPerdadv->maxLen_perdAdvData = max_len_perdAdvData;
            cur_pPerdadv->dat_perdAdvData      = (u8*)perdAdvData;
            return BLE_SUCCESS;
        }
    }

    return HCI_ERR_INVALID_HCI_CMD_PARAMS;
}
#endif








_attribute_ram_code_
int         blt_prd_adv_interrupt_task (int flag, void*p)
{
    int index = flag & FLAG_SCHEDULE_TASK_IDX_MASK;

    if(flag & FLAG_SCHEDULE_START){
        #if (SL01_prd_adv)
            log_task_begin_irq(SL_STACK_EXT_PRD_BASE_TIMING_EN, SL01_prd_adv);
        #endif
        blt_prdadv_start(index);
        #if (SL01_prd_adv)
            log_task_end_irq(SL_STACK_EXT_PRD_BASE_TIMING_EN, SL01_prd_adv);
        #endif
    }
    else if(flag & FLAG_SCHEDULE_AUX_SYNCINFO_UPDATE){
        blt_ll_aux_syncinfo_update(index);
    }
    else if(flag & FLAG_SCHEDULE_BUILD){
        blt_ll_buildPerdAdvSchedulerLinklist();
    }
    else if(flag & FLAG_INSERT_SCHTSK_CONFLICT){
        sch_task_t *pTgtTsk = (sch_task_t *)p;
        u8 tgtTskFlg = pTgtTsk->scheTask_flg & TSKFLG_VALID_MASK;
        u8 curSchTaskOft = TSKOFT_PERD_ADV + index;

        #if(SL08_pdaAdv_conflict)
        log_b8_irq(SL_STACK_EXT_PRD_BASE_TIMING_EN, SL08_pdaAdv_conflict, tgtTskFlg);
        #endif

        #if(SCH_TASK_PRIORITY_IN_CB_EN)
            s32 pri_taskCur = bltPri.pri_cal[curSchTaskOft];
            s32 pri_taskTra = bltPri.pri_cal[pTgtTsk->scheTask_oft];
             //priority higher than exist task, can insert target task
            if(pri_taskCur > pri_taskTra){
                if(bltPri.csctvAbandonCnt[curSchTaskOft] >= 5){
                    my_dump_str_data(0, "[per_adv]consecutive abandon count", &bltPri.csctvAbandonCnt[curSchTaskOft], 2);
                    return 1; /* 1:conflict resolved; 0: insert task failed */
                }
                if(1 && tgtTskFlg == TSKFLG_BIG_BCST){
                    my_dump_str_data(0, "[per_adv]abandon, bis_bcst proc-1", &bltPri.csctvAbandonCnt[pTgtTsk->scheTask_oft], 2);
                    return 0;
                }
                return 1;
            }
        #endif

        my_dump_str_data(0, "[per_adv]insertTsk conflict, tgtTsk=", &tgtTskFlg, 1);

    #if (LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER)
        ///the following code is not valid. cause PDA build is before BIGBCST build. so here not conflict with BIGBCST.
        if(tgtTskFlg == TSKFLG_BIG_BCST){
            my_dump_str_data(0, "[per_adv]abandon, bis_bcst proc0", &bltPri.csctvAbandonCnt[pTgtTsk->scheTask_oft], 2);
            return 0;
        }
        if(tgtTskFlg == TSKFLG_BIG_BCST && bltPri.csctvAbandonCnt[pTgtTsk->scheTask_oft] >= 5){
            my_dump_str_data(0, "[per_adv]abandon, bis_bcst proc1", &bltPri.csctvAbandonCnt[pTgtTsk->scheTask_oft], 2);
            return 0;
        }
        if(tgtTskFlg == TSKFLG_BIG_BCST && (global_pBigBcst+pTgtTsk->scheTask_idx)->big_sc_mask){
            my_dump_str_data(0, "[per_adv]abandon, bis_SC proc2", &bltPri.csctvAbandonCnt[curSchTaskOft], 2);
            return 0;
        }
    #endif

        //Task scheduler has been abandoned bigger than 5 times
        if(bltPri.csctvAbandonCnt[curSchTaskOft] >= 5){
            my_dump_str_data(0, "[per_adv]consecutive abandon count", &bltPri.csctvAbandonCnt[curSchTaskOft], 2);
            return 1; /* 1:conflict resolved; 0: insert task failed */
        }
    }
    return 0;
}


_attribute_noinline_
int         blt_prd_adv_mainloop_task (int flag, void *p)
{
    int index = flag & FLAG_SCHEDULE_TASK_IDX_MASK;
    st_prd_adv_t *cur_pPerdadv = (st_prd_adv_t *)(global_pPerdadv + index);

    if(flag & FLAG_MODULE_MAINLOOP){

    }
    else if(flag & FLAG_SCHEDULE_PRDADV_TASK_BEGIN){
        blt_prdadv_task_begin(cur_pPerdadv);
    }
    else if(flag & FLAG_SCHEDULE_PRDADV_PARAM_UPDATE){
        blt_prdadv_updatePram(cur_pPerdadv);
        cur_pPerdadv->acad_chaged = 0;
    }
    else if(flag & FLAG_MODULE_RESET){
        blt_reset_prd_adv();
    }
    else if(flag & FLAG_MODULE_SET_HOST_CHM){
        blt_ll_ctrlPerdAdvChClassUpd((u8*)p);
    }

    return 0;
}



/*If the advertising set already contains periodic advertising data and the length
 *of the data is greater than the maximum that the Controller can transmit within
 *a periodic advertising interval of Periodic_Advertising_Interval_Max.If advertising
 *on the LE Coded PHY, the S=8 coding shall be assumed
 */
bool blt_ll_advPeriodicChkDataItvl(st_prd_adv_t* cur_pPerdadv, u16 itvl, u8 phy)
{

    u16 firstPkt_extHdrLen = cur_pPerdadv->prd_adv_1stPkt.ext_hdr_len + 1;
    u8  chainPkt_extHdrLen = 4+1; //
    u16 payloadLen = 0;
    u8 T_MAFS_num = cur_pPerdadv->chain_ind_num;

    payloadLen += (cur_pPerdadv->prd_1st_pkt_dataLen + firstPkt_extHdrLen);

    for(int i=0; i < cur_pPerdadv->chain_ind_num; i++){
        payloadLen += (cur_pPerdadv->chain_ind_dataLen[i] + chainPkt_extHdrLen);
    }

    u16 extra_us = 0;
    u32 totalTime_us = 0;
    if(phy == BLE_PHY_1M){
        extra_us = 80 + 80*T_MAFS_num + 150; //preamble(1B) + AA(4B) + crc(3B) + header(2B)
        totalTime_us = payloadLen*8 + T_MAFS_num*300 + extra_us;
    }
    else if(phy == BLE_PHY_2M){
        extra_us = 44 + 44*T_MAFS_num + 150;//preamble(2B) + AA(4B) + crc(3B) + header(2B)
        totalTime_us = payloadLen*4 + T_MAFS_num*300 + extra_us;
    }
    else if(phy == BLE_PHY_CODED){
        extra_us = (376 + 128+ 216) + (376 + 128 + 216)*T_MAFS_num + 150;
        totalTime_us = payloadLen*8*8 + T_MAFS_num*300 + extra_us; //If advertising on the LE Coded PHY, the S=8 coding shall be assumed
    }

    if(totalTime_us >= itvl*1250){
        return TRUE;
    }

    return FALSE;
}




ble_sts_t   blc_ll_setPeriodicAdvParam(adv_handle_t adv_handle, u16 advInter_min, u16 advInter_max, perd_adv_prop_t property)
{
    //HCI/GEV/BV-02-C [Disallow Mixing Legacy and Extended Advertising Commands]
    if(IS_LEGACY_ADV_VALID){
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_EXTENDED_ADV_VALID;

    st_ext_adv_t *cur_pextadv;
    if(adv_handle == INVALID_ADVHD_FLAG){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }
    else{
        /*If the corresponding advertising set does not already exist, then the Controller shall return the error
          code Unknown Advertising Identifier (0x42). */
        cur_pextadv = blt_extadv_search_existed_adv_set(adv_handle);
        if(!cur_pextadv){
            return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
        }
    }


    /*If the Advertising_Handle does not identify an advertising set that is already configured for periodic advertising
     * and the Controller is unable to support more periodic advertising at present, the Controller shall return the
     * error code Memory Capacity Exceeded (0x07). */
    st_prd_adv_t *cur_pPerdadv = blt_prdadv_search_existed_and_allocate_new_periodic_adv(adv_handle);
    if(!cur_pPerdadv){
        return HCI_ERR_MEM_CAP_EXCEEDED;
    }


    /* If the advertising set identified by the Advertising_Handle specified scannable, connectable, legacy,
     * or anonymous advertising, the Controller shall return the error code Invalid HCI Command Parameters (0x12).*/
    if(cur_pextadv->evt_props & (ADVEVT_PROP_MASK_CONNECTABLE | ADVEVT_PROP_MASK_SCANNABLE | ADVEVT_PROP_MASK_LEGACY | ADVEVT_PROP_MASK_ANON_ADV) ){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }


    /*If the Host issues this command when periodic advertising is enabled for the specified advertising set,
     * the Controller shall return the error code Command Disallowed (0x0C). */
    if(cur_pextadv->prdadv_api_en){
        return HCI_ERR_CMD_DISALLOWED;
    }

    /* HCI/DDI/BI-50-C [LE Set Periodic Advertising Parameters, Reject, Data Too Long, LE 1M PHY]*/
    /* HCI/DDI/BI-51-C [LE Set Periodic Advertising Parameters, Reject, Data Too Long, LE Coded PHY]*/
    /*If the advertising set already contains periodic advertising data and the length of the data is greater than the maximum that
     *the Controller can transmit within a periodic advertising interval of Periodic_Advertising_Interval_Max,
     *the the Controller shall return the error code Packet Too Long (0x45). If advertising on the LE Coded PHY,the S=8 coding shall be assumed.*/
    if(blt_ll_advPeriodicChkDataItvl(cur_pPerdadv, advInter_max, cur_pextadv->sec_phy)){
        return HCI_ERR_PACKET_TOO_LONG;
    }



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

    return BLE_SUCCESS;
}


ble_sts_t   blc_hci_le_setPeriodicAdvParam(hci_le_setPeriodicAdvParam_cmdParam_t* pCmdParam)
{
    /* validate intervals */
    if ((pCmdParam->advInter_min < 0x0006) || (pCmdParam->advInter_max < 0x006) || (pCmdParam->advInter_min > pCmdParam->advInter_max)) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS; //Here HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE is also OK.
    }

#if (0)
    //according to the IXIT periodic adv interval setting. now the EBQ setting min pda interval is 100ms
    //HCI/DDI/BI-67-C,but HCI/DDI/BI-15-C ~ HCI/DDI/BI-25-C not use the limit.
    if ((pCmdParam->advInter_min < 80) || (pCmdParam->advInter_max < 80) || (pCmdParam->advInter_min > pCmdParam->advInter_max)) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }
#endif

    return blc_ll_setPeriodicAdvParam(pCmdParam->adv_handle, pCmdParam->advInter_min, pCmdParam->advInter_max, pCmdParam->property);
}


ble_sts_t   blc_hci_le_setPeriodicAdvData(adv_handle_t adv_handle, data_oper_t operation, u8 perdAdvData_len, u8 *perdAdvData)
{
    //HCI/GEV/BV-02-C [Disallow Mixing Legacy and Extended Advertising Commands]
    if(IS_LEGACY_ADV_VALID){
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_EXTENDED_ADV_VALID;


    st_ext_adv_t *cur_pextadv;
    if(adv_handle == INVALID_ADVHD_FLAG){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }
    else{
        /*If the corresponding advertising set does not already exist, then the Controller shall return the error
          code Unknown Advertising Identifier (0x42). */
        cur_pextadv = blt_extadv_search_existed_adv_set(adv_handle);
        if(!cur_pextadv){
            return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
        }
    }


    /*If the advertising set has not been configured for periodic advertising,
      then the Controller shall return the error code Command Disallowed (0x0C). */
    if(cur_pextadv->mapping_prdadv_idx == INVALID_ADVHD_FLAG){
        return HCI_ERR_CMD_DISALLOWED;
    }


    st_prd_adv_t *cur_pPerdadv = (st_prd_adv_t *)(global_pPerdadv + cur_pextadv->mapping_prdadv_idx);

#if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
    if (cur_pPerdadv->num_subevents){ //For Periodic ADV with Response
        return HCI_ERR_CMD_DISALLOWED;
    }
#endif

    //#define BLE_HCI_MAX_PERIODIC_ADV_DATA_LEN                (252)
    if(perdAdvData_len > 252){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    /*If periodic advertising is currently enabled for the specified advertising set and
    Operation does not have the value 0x03, the Controller shall return the error
    code Command Disallowed (0x0C).*/
    if((u8)operation < DATA_OPER_COMPLETE){
        if(cur_pextadv->prdadv_task_en){
            return HCI_ERR_CMD_DISALLOWED;
        }
    }

     /* If Operation is 0x04 and:
        * periodic advertising is currently disabled for the advertising set;
        * the periodic advertising set contains no data; or
        * Advertising_Data_Length is not zero;
       then the Controller shall return the error code Invalid HCI Command Parameters (0x12).
       //HCI/CCO/BI-33-C
     */
    cur_pPerdadv->prd_DID_changed = 0;

    if(operation == DATA_OPER_UNCHANGED){
        if(!cur_pPerdadv->prd_adv_en || cur_pPerdadv->prd_1st_pkt_dataLen == 0 || perdAdvData_len != 0){
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }

        //different from aux_adv_ind's DID, and different from the previous PDA adv DID.
        cur_pPerdadv->prd_DID = ((clock_time()>>4) & 0xFFF)  | 0x001;
        //while(cur_pPerdadv->prd_DID == cur_pextadv->adv_did){
        //  cur_pPerdadv->prd_DID = ((clock_time()>>4) & 0xFFF)  | 0x001;
        //}
        cur_pPerdadv->prd_DID_changed = 1;
        return BLE_SUCCESS;
    }

    /*If Operation indicates the start of new data (values 0x01 or 0x03), then any
    existing partial or complete data shall be discarded. If the periodic advertising
    data is discarded by the command or the combined length of the data after the
    command completes is zero, the advertising set will have no periodic advertising data..*/
    int newLen_adv;
    if(perdAdvData_len == 0 && operation == DATA_OPER_COMPLETE){  //delete existing data
        cur_pPerdadv->curLen_perdAdvData = 0;
        cur_pPerdadv->prd_1st_pkt_dataLen = 0; //HCI/CCO/BI-33-C
        newLen_adv = 0;
    }
    else if(operation==DATA_OPER_FIRST || operation==DATA_OPER_COMPLETE){
        cur_pPerdadv->curLen_perdAdvData = 0;
        newLen_adv = perdAdvData_len;
    }
    else{
        newLen_adv = cur_pPerdadv->curLen_perdAdvData + perdAdvData_len;
    }

    /*If the combined length of the data
    exceeds the capacity of the advertising set identified by the
    Advertising_Handle parameter (see Section 7.8.57 LE Read Maximum
    Advertising Data Length Command) or the amount of memory currently
    available, all the data shall be discarded and the Controller shall return the
    error code Memory Capacity Exceeded (0x07).*/
    if( newLen_adv > cur_pextadv->maxLen_advData){
        return HCI_ERR_MEM_CAP_EXCEEDED;
    }

    switch(operation){
        case DATA_OPER_INTER:
        case DATA_OPER_FIRST:
            cur_pPerdadv->unfinished_advData = 1;
        break;

        case DATA_OPER_LAST:
        case DATA_OPER_COMPLETE:
            cur_pPerdadv->unfinished_advData = 0;
        break;

        case DATA_OPER_UNCHANGED:
        break;

        default:
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    //copy data
    if(newLen_adv && perdAdvData_len){
        smemcpy((cur_pPerdadv->dat_perdAdvData + cur_pPerdadv->curLen_perdAdvData), perdAdvData, perdAdvData_len);
    }
    cur_pPerdadv->curLen_perdAdvData = newLen_adv;

    if(!cur_pPerdadv->unfinished_advData){   //data completed
        /*If the combined length of the data is greater than the maximum that the Controller can transmit within the
         *current periodic advertising interval (if periodic advertising is currently enabled) or the Periodic_Advertising_Interval_Max
         *for the advertising set (if currently disabled), all the data shall be discarded and the Controller shall return the error code
         *Packet Too Long (0x45). If advertising on the LE Coded PHY, the S=8 coding shall be assumed.
         *if periodic adv is enabled, compare with current pda interval; if pda disabled,compare with max interval.
         *but now we use the same interval, so not need to judge whether periodic adv is enabled*/
        blt_prdadv_updatePram(cur_pPerdadv);
        if(blt_ll_advPeriodicChkDataItvl(cur_pPerdadv, cur_pPerdadv->advInter_max, cur_pextadv->sec_phy)){ //if(cur_pextadv->prdadv_api_en)
            return HCI_ERR_PACKET_TOO_LONG;
        }

        cur_pextadv->prdadv_update_flag = 1;
    }

    return BLE_SUCCESS;
}



ble_sts_t   blc_ll_setPeriodicAdvData(adv_handle_t adv_handle, u16 perdAdvData_len, const u8 *perdAdvData)
{
    //HCI/GEV/BV-02-C [Disallow Mixing Legacy and Extended Advertising Commands]
    if(IS_LEGACY_ADV_VALID){
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_EXTENDED_ADV_VALID;


    st_ext_adv_t *cur_pextadv;
    if(adv_handle == INVALID_ADVHD_FLAG){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }
    else{
        /*If the corresponding advertising set does not already exist, then the Controller shall return the error
          code Unknown Advertising Identifier (0x42). */
        cur_pextadv = blt_extadv_search_existed_adv_set(adv_handle);
        if(!cur_pextadv){
            return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
        }
    }


    /*If the advertising set has not been configured for periodic advertising,
      then the Controller shall return the error code Command Disallowed (0x0C). */
    if(cur_pextadv->mapping_prdadv_idx == INVALID_ADVHD_FLAG){
        return HCI_ERR_CMD_DISALLOWED;
    }

    st_prd_adv_t *cur_pPerdadv = (st_prd_adv_t *)(global_pPerdadv + cur_pextadv->mapping_prdadv_idx);

#if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
    if (cur_pPerdadv->num_subevents){ //For Periodic ADV with Response
        return HCI_ERR_CMD_DISALLOWED;
    }
#endif

    if(perdAdvData_len > cur_pPerdadv->maxLen_perdAdvData){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    smemcpy(cur_pPerdadv->dat_perdAdvData, perdAdvData, perdAdvData_len);
    cur_pPerdadv->curLen_perdAdvData = perdAdvData_len;

    cur_pextadv->prdadv_update_flag = 1;

    return BLE_SUCCESS;
}



//note: adv handle allocate by host, we should not limit the range of advHandle,or it can't pass BQB
ble_sts_t   blc_ll_setPeriodicAdvEnable(u8 per_adv_enable, adv_handle_t adv_handle)
{
    //HCI/GEV/BV-02-C [Disallow Mixing Legacy and Extended Advertising Commands]
    if(IS_LEGACY_ADV_VALID){
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_EXTENDED_ADV_VALID;


    st_ext_adv_t *cur_pextadv;
    if(adv_handle == INVALID_ADVHD_FLAG){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }
    else{
        /*If the corresponding advertising set does not already exist, then the Controller shall return the error
          code Unknown Advertising Identifier (0x42). */
        cur_pextadv = blt_extadv_search_existed_adv_set(adv_handle);
        if(!cur_pextadv){
            return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
        }
    }

    /*If the advertising set has not been configured for periodic advertising,
      then the Controller shall return the error code Command Disallowed (0x0C). */
    if(cur_pextadv->mapping_prdadv_idx == INVALID_ADVHD_FLAG){
        return HCI_ERR_CMD_DISALLOWED;
    }

    st_prd_adv_t *cur_pPerdadv = (st_prd_adv_t *)(global_pPerdadv + cur_pextadv->mapping_prdadv_idx);

    if( (per_adv_enable & PERD_INCLUDE_ADI_BIT) == PERD_INCLUDE_ADI_BIT){
        cur_pPerdadv->include_ADI_flag = 0x01; //LL/DDI/ADV/BV-63-C
    }else{
        cur_pPerdadv->include_ADI_flag = 0x00;
    }

    //now support ADI, so ignore the BIT(1)---include the ADI.
    //HCI/DDI/BV-09-C
    per_adv_enable &= PERD_ENABLE_BIT; //only BIT(0) is PDA enable bit.

    /* If Enable is set to 0x01 (periodic advertising is enabled) and the periodic
    advertising data in the advertising set is not complete, the Controller shall
    return the error code Command Disallowed (0x0C). */

    /*If Enable is set to 0x01 and the advertising set identified by the
    Advertising_Handle specified scannable, connectable, legacy, or anonymous
    advertising, the Controller shall return the error code Command Disallowed
    (0x0C)..*/

    /* If Enable is set to 0x01 and the length of the periodic advertising data is greater
    than the maximum that the Controller can transmit within the chosen periodic
    advertising interval, the Controller shall return the error code Packet Too Long
    (0x45). */
    if(per_adv_enable){
    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
        if (cur_pPerdadv->num_subevents == 0)//For Periodic ADV
    #endif
        {
            if(cur_pPerdadv->unfinished_advData){
                return HCI_ERR_CMD_DISALLOWED;
            }

            if(cur_pextadv->evt_props & (ADVEVT_PROP_MASK_CONNECTABLE | ADVEVT_PROP_MASK_SCANNABLE | ADVEVT_PROP_MASK_LEGACY | ADVEVT_PROP_MASK_ANON_ADV) ){
                return HCI_ERR_CMD_DISALLOWED;
            }

            #if 1
                /*If bit 0 of Enable is set to 1 and the length of the periodic advertising data is greater than the maximum
                 * that the Controller can transmit within the chosen periodic advertising interval,
                 * the Controller shall return the error code Packet Too Long (0x45). If advertising on the LE Coded PHY,
                 * the S=8 coding shall be assumed*/
                if(blt_ll_advPeriodicChkDataItvl(cur_pPerdadv, cur_pextadv->advInt_max, cur_pextadv->sec_phy)){
                    my_dump_str_data(0, "PDAen data too long", 0, 0);
                    return HCI_ERR_PACKET_TOO_LONG;
                }
            #endif
        }
    }


    cur_pPerdadv->link_big_handle = BIG_HANDLE_INVALID;


    /*Enabling periodic advertising when it is already enabled can cause the random
    address to change. Disabling periodic advertising when it is already disabled
    has no effect.*/


    /* attention: one situation not handle: when prd_adv is enable, prd_adv data changed */

    cur_pextadv->prdadv_api_en = per_adv_enable;
    u64 extadv_running = bltSche.task_mask & (TSKMSK_EXT_ADV_0<<cur_pextadv->extadv_index);

    u8 pre_prdadv_en = cur_pextadv->prdadv_task_en;
    /* periodic ADV: 0 -> 1 */
    if(per_adv_enable && !pre_prdadv_en && cur_pextadv->extadv_en){
        #if(SL01_pda_adv_endis)
        log_task_begin_irq(SL_STACK_EXT_PRD_BASE_TIMING_EN, SL01_pda_adv_endis);
        #endif

        /* to handle: when prd_adv change, data & timing need change but old allocated task may error */
        if(extadv_running){
            u32 r = irq_disable();
            cur_pextadv->extadv_change_flag = EXTADV_CHANGE_FLAG;
            blt_remove_future_task(TSKOFT_AUX_ADV + cur_pextadv->extadv_index);
            cur_pextadv->aux_adv_pending = 0;//pending need to clear, or the new aux adv will not run.
            irq_restore(r);
        }


        if(cur_pextadv->prdadv_update_flag){
            blt_prdadv_updateAcadPram(cur_pPerdadv, PERD_ACAD_NONE); //same as: blt_prdadv_updatePram(cur_pPerdadv);
            cur_pextadv->prdadv_update_flag = 0;
            cur_pextadv->syncinfo_changed = 0;
            cur_pPerdadv->acad_chaged = 0;
        }

        blt_prdadv_task_begin(cur_pPerdadv);

        u32 r = irq_disable();
        //need to notice:here |=SYNC_INFO_NEED may cause the first some aux adv has SyncInfor section but value are zero.
        //later need to optimize that --- QW
        cur_pextadv->syncinfo_used |= SYNC_INFO_NEED;//when periodic enable,set syncinfo_used to calculate aux adv duration.
        blmsParam.state_chng |= STATE_CHANGE_PRD_ADV;
        cur_pextadv->prdadv_task_en = 1;
        blt_sche_enableTask(TSKMSK_PERD_ADV_0 << cur_pPerdadv->prdadv_index);
        irq_restore(r);

    
        if(cur_pextadv->extadv_en)
        {
            cur_pextadv->syncinfo_changed = 1;

            #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
                cur_pextadv->acad_used = cur_pPerdadv->num_subevents ? PERD_ACAD_PAwR_ENA : 0;
            #else
                cur_pextadv->acad_used = 0;
            #endif

            blt_updateExtAdvSet(cur_pextadv);
            cur_pextadv->syncinfo_changed = 2;

            #if (ONLY_FOR_EBQ_TEST_LATER_REMOVE)
                bigExtAuxPda_conflictCtrl.syncInfor_changeCtrl = SYNCINFOR_VAILD_PENDING;
            #endif
        }

    }
    /* periodic ADV: 1 -> 0 */
    else if(!per_adv_enable && pre_prdadv_en){
        /* to handle: when prd_adv change, data & timing need change but old allocated task may error */
        if(extadv_running){
            u32 r = irq_disable();
            cur_pextadv->extadv_change_flag = EXTADV_CHANGE_FLAG;
            blt_remove_future_task(TSKOFT_AUX_ADV + cur_pextadv->extadv_index);
            cur_pextadv->aux_adv_pending = 0;//pending need to clear, or the new aux adv will not run.
            irq_restore(r);
        }

        u32 r = irq_disable();
        cur_pextadv->syncinfo_used = 0;
        blmsParam.state_chng |= STATE_CHANGE_PRD_ADV;
        cur_pextadv->prdadv_task_en = 0;
        if(cur_pPerdadv->prd_adv_en){
            // LL/DDI/ADV/BV-39-C, if enable Periodic ADV again, can begin prdadv_task. add by lijing
            cur_pextadv->prdadv_update_flag = 1;
        }
        blt_prdadv_task_end(cur_pPerdadv);
        blt_sche_disableTask(TSKMSK_PERD_ADV_0 << cur_pPerdadv->prdadv_index);

        /* Don't need a pointer callback function for now, it's relatively brief. */
    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
        if (cur_pPerdadv->num_subevents){ //For Periodic ADV with Response
            blt_sche_removeTaskMask(TSKMSK_PAWRA_SUB_0 << cur_pPerdadv->prdadv_index);
            blt_sche_removeTaskMask(TSKMSK_PAWRA_RSP_0 << cur_pPerdadv->prdadv_index);
        }
    #endif

        irq_restore(r);

        #if (ONLY_FOR_EBQ_TEST_LATER_REMOVE)
            bigExtAuxPda_conflictCtrl.acadInfor_changeCtrl = ACAD_NOT_CHANGE;
            bigExtAuxPda_conflictCtrl.pdaAdv_sendNum = 0;
        #endif

        if(cur_pextadv->extadv_en)
        {
            cur_pextadv->syncinfo_changed = 1;
            cur_pextadv->acad_used = 0;
            blt_updateExtAdvSet(cur_pextadv);
            cur_pextadv->syncinfo_changed = 2;

            #if (ONLY_FOR_EBQ_TEST_LATER_REMOVE)
                bigExtAuxPda_conflictCtrl.syncInfor_changeCtrl = SYNCINFOR_VAILD_PENDING;
            #endif
        }

        #if(SL01_pda_adv_endis)
        log_task_end_irq(SL_STACK_EXT_PRD_BASE_TIMING_EN, SL01_pda_adv_endis);
        #endif
    }
#if OS_SUP_EN
    if(blt_os_giveSem_cb)
    {
        blt_os_giveSem_cb();
    }
#endif


    return BLE_SUCCESS;


}




void blt_prdadv_task_begin(st_prd_adv_t *cur_pPerdadv)
{
    st_ext_adv_t *cur_pextadv = (st_ext_adv_t *)(global_pextadv + cur_pPerdadv->mapping_extadv_idx);
    st_pda_t *p_curPda = (st_pda_t *) &cur_pPerdadv->pda_tx;
    cur_pPerdadv->pda_tx.prdadv_send_cnt = 0;


    u16 interval = cur_pPerdadv->advInter_max;    //TODO: optimize

    p_curPda->bSlot_prdadv_itvl = interval<<1; //     1.25mS unit -> 625uS unit
    p_curPda->sSlot_prdadv_itvl = interval<<6; //*64: 1.25mS unit -> sSlot unit


    /* first timing design is special, SiHui knows detail, bSlot_mark has two function */
    u32 r = irq_disable();

#if 0
    //when the first,bltSche.bSlot_tick_irq_real=0 and p_curPda->bSlot_mark_prdadv will be large(such as 0xC30E,31s)
    //that cause no periodic adv for long time.EBQ test case will fail---//LL/DDI/ADV/BV-26-C
    //in addition, if extended adv disable(such as 7s after running time) and periodic always enable,
    //that maybe not find task in 8s and occur 0xff0d0000 error.
    int n_bSlot = (clock_time() - bltSche.bSlot_tick_irq_real)/SYSTEM_TIMER_TICK_625US;
    p_curPda->bSlot_mark_prdadv =  bltSche.bSlot_idx_irq_real + n_bSlot;
#endif

    //1.mark not set too large,refer to LL/DDI/ADV/BV-26-C
    //2.use rand() to avoid different advertise set to use same mark.
    //if use same mark, if advSet0Invl=15ms and advSet1Invl=20ms,conflict will occur per 60ms.so need to avoid that.---//LL/DDI/ADV/BV-33-C
    //However, rand cannot completely avoid conflict problem, just probability is low.
    p_curPda->bSlot_mark_prdadv = (blmsParam.sche_run_flag ? bltSche.bSlot_idx_next : 0) + (rand()&0x1f);  //TODO: 22-02-11 0x0f
    //p_curPda->bSlot_mark_prdadv += 2;//at least 0.625*2 = 1.25ms from bltSche.bSlot_idx_next.

    if(p_curPda->bSlot_mark_prdadv >= p_curPda->bSlot_prdadv_itvl){
        p_curPda->bSlot_mark_prdadv -= p_curPda->bSlot_prdadv_itvl;
    }

#if (NEED_MORE_TEST_TO_CONFIRM) //later will run more test to confirm. now two IAL cases are OK.
    //Only when both periodic adv interval and extended adv interval are greater than 2.4573s can the offset adjust be 1.
    if(p_curPda->bSlot_prdadv_itvl*625 > 2457300 && cur_pextadv->advInt_max*625 > 2457300){

        u32 extadv_bSlotMark = cur_pextadv->bSlot_mark_extadv;
        u16 extadv_advIntMax = cur_pextadv->advInt_max;
        u32 prdadv_bSlotMark = p_curPda->bSlot_mark_prdadv;

        if(prdadv_bSlotMark > extadv_bSlotMark){
            u32 extIntvlNum = 0;
            extIntvlNum = (prdadv_bSlotMark - extadv_bSlotMark)/extadv_advIntMax;//advInt_min unit is 625us

            //The max distance between AUX_ADV_IND and ADV_EXT_IND is 25ms
            prdadv_bSlotMark = extadv_bSlotMark + extIntvlNum*extadv_advIntMax + (extadv_advIntMax>>2) + (rand()&0x1f);

        }else{
            prdadv_bSlotMark = extadv_bSlotMark + (extadv_advIntMax>>2) + (rand()&0x1f);
        }
        p_curPda->bSlot_mark_prdadv = prdadv_bSlotMark;
    }
#endif

    irq_restore(r);


    /* generate access_code & crc_init */
    p_curPda->paAccessAddr = blt_ll_connCalcAccessAddr_v2();
//  p_curPda->paAccessAddr = 0x1A1E5CF9;  //debug
    smemcpy(p_curPda->chnParam.map.chmTbl, blmhostChnClassUpt.gLlChannelMap, 5);

    smemcpy(cur_pextadv->auxSyncInfo.chm, blmhostChnClassUpt.gLlChannelMap, 5); //[0:36]chm, : [37:39]sca
    #if BLMS_PM_ENABLE
        cur_pextadv->auxSyncInfo.chm[4] |= (SCA_MASTER_SLAVE_251_500_PPM<<5);//251PPM - 500PPM
    #else
        cur_pextadv->auxSyncInfo.chm[4] |= (SCA_MASTER_SLAVE_31_50_PPM<<5);//31PPM - 50PPM
    #endif

    p_curPda->paCrcInit = (p_curPda->paAccessAddr ^ 0x55aa00)&0xFFFFFF;


    /* channelIdentifier = (Access Address31-16) XOR (Access Address15-0) */
    p_curPda->chnIdentifier = (p_curPda->paAccessAddr>>16) ^ (p_curPda->paAccessAddr&0xffff); //channel identifier
    csa2_calculateMapInfo(&p_curPda->chnParam.map);

    /*Each periodic advertising train shall have a 16-bit event counter (paEventCounter).
    The initial value of this counter is implementation specific.*/
//  p_curPda->paEvtCnt = (rand()&20) | 1;
    p_curPda->paEvtCnt = 0;  //debug value

    p_curPda->update_map = 0; //clr

    /*The value of the Sync Packet Offset field is in the unit of time indicated by
    the Offset Units field; the actual offset is determined by multiplying the value
    by the unit and then, if the Offset Adjust field is set to 1, adding 2.4576 seconds.
    The Offset Units field shall be set to 0 if the Sync Packet Offset is less than 245,700 us.
    The Offset Adjust field shall be set to 0 if the Offset Units field is set to 0 or
    if the SyncInfo field appears within an advertising PDU.*/
    //cur_pextadv->auxSyncInfo.syncPktOffset = //calculate when sending AUX_ADV
    cur_pextadv->auxSyncInfo.offsetUnit = EXT_ADV_PDU_SYNC_OFFSET_UNITS_30_US;//dft: 30us, Sync Packet Offset <= 245700us
    cur_pextadv->auxSyncInfo.offsetAdjust = 0;//dft: SyncInfo field appears within an ADV PDU

    cur_pextadv->auxSyncInfo.itvl = interval;

    cur_pextadv->auxSyncInfo.AA = p_curPda->paAccessAddr;
//  u8* pCrcInit = (u8*)&p_curPda->paCrcInit;
//  cur_pextadv->auxSyncInfo.crcInit[0] = pCrcInit[0];
//  cur_pextadv->auxSyncInfo.crcInit[1] = pCrcInit[1];
//  cur_pextadv->auxSyncInfo.crcInit[2] = pCrcInit[2];
    smemcpy(cur_pextadv->auxSyncInfo.crcInit, &p_curPda->paCrcInit, 3);
    //cur_pextadv->auxSyncInfo.evtCounter = // calculate when sending AUX_ADV

//  cur_pextadv->syncinfo_used |= SYNC_INFO_VALID;


    /* Priority preset value */
    blt_ll_set_interval_level(TSKOFT_PERD_ADV + cur_pPerdadv->prdadv_index, interval);
    blt_ll_setSchedulerTaskPriority( TSKOFT_PERD_ADV + cur_pPerdadv->prdadv_index, TASK_PRIORITY_PERD_ADV_DFT );
    cur_pPerdadv->prd_adv_en = 1; //important

    /* Don't need a pointer callback function for now, it's relatively brief. */
#if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
    if (cur_pPerdadv->num_subevents){ //For Periodic ADV with Response
        /* Priority preset value : for SUBX task */
        blt_ll_set_interval_level(TSKOFT_PAWRA_SUB + cur_pPerdadv->prdadv_index, interval);
        blt_ll_setSchedulerTaskPriority( TSKOFT_PAWRA_SUB + cur_pPerdadv->prdadv_index, TASK_PRIORITY_PERD_ADV_DFT );
        /* Priority preset value : for RSP_SLOT task */
        blt_ll_set_interval_level(TSKOFT_PAWRA_RSP + cur_pPerdadv->prdadv_index, cur_pPerdadv->subevent_interval);
        blt_ll_setSchedulerTaskPriority( TSKOFT_PAWRA_RSP + cur_pPerdadv->prdadv_index, TASK_PRIORITY_PERD_ADV_DFT );

        cur_pPerdadv->subDataReq.subevtStart = 0;
        cur_pPerdadv->subDataReq.subevtCount = 0;
    }
#endif

}



void blt_prdadv_task_end(st_prd_adv_t   *cur_pPerdadv)
{
    cur_pPerdadv->prd_adv_en = 0;
}

u8          blt_ll_isPerdAdvEnable(u8 adv_handle)
{
    u8 en = 0;

    st_prd_adv_t *perd = blt_ll_search_existing_perdAdv_index_by_advHandle(adv_handle);

    if((perd!=NULL) && (perd->prd_adv_en==1))
    {
        en = 1;
    }

    return en;
}





_attribute_ram_code_
st_prd_adv_t* blt_ll_search_existing_perdAdv_index_by_advHandle(u8 adv_handle)
{
    st_prd_adv_t *cur_pPerdadv;
    for(int i=0;i<bltPrdA.maxNum_perdAdv; i++)
    {
        cur_pPerdadv = (st_prd_adv_t *)(global_pPerdadv + i);
        if( cur_pPerdadv->advHand_mark == adv_handle ){  //existing ADV set match

            return cur_pPerdadv;
        }
    }

    return NULL;
}


u8          blt_ll_search_existing_perdAdv_index(u8 advSet_idx)
{
    st_prd_adv_t *cur_pPerdadv;
    /* search existing */
    for(int i=0;i<bltPrdA.maxNum_perdAdv; i++)
    {
        cur_pPerdadv = (st_prd_adv_t *)(global_pPerdadv + i);
        if( cur_pPerdadv->advSet_idx == advSet_idx ){  //existing ADV set match

            return i;
        }
    }


    return INVALID_ADVSET_IDX;
}



u8          blt_ll_search_existing_and_new_perdAdv_index(u8 advSet_idx)
{
    st_prd_adv_t *cur_pPerdadv;
    /* search existing */
    for(int i=0;i<bltPrdA.maxNum_perdAdv; i++)
    {
        cur_pPerdadv = (st_prd_adv_t *)(global_pPerdadv + i);
        if( cur_pPerdadv->advSet_idx == advSet_idx ){  //existing ADV set match

            return i;
        }
    }


    /* find new available */
    for(int i=0;i<bltPrdA.maxNum_perdAdv; i++)
    {
        cur_pPerdadv = (st_prd_adv_t *)(global_pPerdadv + i);
        if( cur_pPerdadv->advSet_idx == INVALID_ADVSET_IDX ){

            cur_pPerdadv->advSet_idx = advSet_idx;

            return i;
        }
    }

    return INVALID_ADVSET_IDX;
}








st_prd_adv_t * blt_prdadv_search_existed_and_allocate_new_periodic_adv(u8 advHandle)
{
    st_prd_adv_t *available_pPerdadv = NULL;
    st_prd_adv_t *cur_pPerdadv;
    for(int i=0;i<bltPrdA.maxNum_perdAdv; i++)
    {
        cur_pPerdadv = (st_prd_adv_t *)(global_pPerdadv + i);
        if( cur_pPerdadv->advHand_mark == advHandle ){  //existing ADV set match
            return cur_pPerdadv;
        }
        else if(cur_pPerdadv->advHand_mark == INVALID_ADVHD_FLAG){
            if(!available_pPerdadv){
                available_pPerdadv = cur_pPerdadv;
                //TODO: reset some parameters
            }
        }
    }

    if(available_pPerdadv){
        available_pPerdadv->advHand_mark = advHandle;
    }

    return available_pPerdadv;
}



st_prd_adv_t * blt_prdadv_search_existed_periodic_adv(u8 advHandle)
{
    st_prd_adv_t *cur_pPerdadv;
    for(int i=0;i<bltPrdA.maxNum_perdAdv; i++)
    {
        cur_pPerdadv = (st_prd_adv_t *)(global_pPerdadv + i);
        if( cur_pPerdadv->advHand_mark == advHandle ){  //existing ADV set match
            return cur_pPerdadv;
        }
    }

    return NULL;
}














_attribute_ram_code_
int         blt_ll_buildPerdAdvSchedulerLinklist(void)
{
    st_prd_adv_t *cur_pPerdadv = NULL;
    u16 intvl_jump_perd;
    /*The absolute value on the time axis corresponding to Task->begin:
    sSlot_tick_start + Task->begin*SSLOT_TICK_NUM, sSlot_idx_base is the relative value */
//  u32 sSlot_idx_base = (bltSche.bSlot_idx_next - bltSche.bSlot_idx_start)*32;

    u32 bSlot_start_prdadv;
    for(int i=0; i<TSKNUM_PERD_ADV; i++)
    {
        if( bltSche.task_mask & (TSKMSK_PERD_ADV_0<<i) )
        {
            cur_pPerdadv = (st_prd_adv_t*)(global_pPerdadv + i);
            cur_pPerdadv->schTsk_wptr = cur_pPerdadv->schTsk_rptr = 0;

            if( cur_pPerdadv->pda_tx.bSlot_mark_prdadv >= bltSche.bSlot_idx_next){
                bSlot_start_prdadv = cur_pPerdadv->pda_tx.bSlot_mark_prdadv + cur_pPerdadv->pda_tx.bSlot_prdadv_itvl;
                //bSlot_start_prdadv = cur_pPerdadv->pda_tx.bSlot_mark_prdadv;
                intvl_jump_perd = 0;
                //my_dump_str_data(STACK_DUMP_EN,"Insert task0: jump perd", &intvl_jump_perd , 2);
            }
            else{
                intvl_jump_perd = (bltSche.bSlot_idx_next - 1 - cur_pPerdadv->pda_tx.bSlot_mark_prdadv) / cur_pPerdadv->pda_tx.bSlot_prdadv_itvl;
                bSlot_start_prdadv = cur_pPerdadv->pda_tx.bSlot_mark_prdadv + (intvl_jump_perd + 1) * cur_pPerdadv->pda_tx.bSlot_prdadv_itvl;
                //my_dump_str_data(STACK_DUMP_EN,"Insert task1: jump perd", &intvl_jump_perd , 2);

                //blt_ll_incSchedulerTaskCalPriority( TSKOFT_PERD_ADV + i, bltPri.step_final[TSKOFT_PERD_ADV + i]*2*intvl_jump_perd );
            }



            if(bSlot_start_prdadv >= bltSche.bSlot_endIdx_dft){ //to save some time for big interval
                continue; //attention: can not use break !!!
            }



            int new_task_cnt = 0;
            for(int j=0;j<PERD_ADV_FIFONUM;j++){

                sch_task_t  *pCur_schTask = (sch_task_t *)&cur_pPerdadv->schTsk_fifo[j];

                pCur_schTask->begin = ((bSlot_start_prdadv + j*cur_pPerdadv->pda_tx.bSlot_prdadv_itvl) - bltSche.bSlot_idx_start )<<5;
                pCur_schTask->end = pCur_schTask->begin + cur_pPerdadv->pda_tx.sSlot_duration_pda - 1;

                if( pCur_schTask->begin >=  bltSche.sSlot_endIdx_dft){  //new task beyond correct range, finish
                    break;
                }
                else if(pCur_schTask->end < bltSche.sSlot_endIdx_dft){ //new task in correct range
                    cur_pPerdadv->schTsk_wptr = j;
                    new_task_cnt ++;
                    //blt_ll_incSchedulerTaskCalPriority( TSKOFT_PERD_ADV + i, -bltPri.step_final[TSKOFT_PERD_ADV + i]);
                }
                else{ //new task across "sSlot_endIdx_dft"

                    //for those task across end_idx, find the task with highest priority, to guarantee that task not missed
                    if(bltPri.pri_cal[TSKOFT_PERD_ADV + i] > bltPri.priMax_value){
                        bltPri.priMax_value = bltPri.pri_cal[TSKOFT_PERD_ADV + i];
                        bltPri.priMax_index = TSKOFT_PERD_ADV + i;
                        bltSche.sSlot_endIdx_maxPri = pCur_schTask->begin;
                        my_dump_str_u32s(SCHE_TIMING_IMPROVE_DBG_EN,"across IDX perd", i, bltSche.sSlot_endIdx_dft, bltSche.sSlot_endIdx_maxPri, bltPri.priMax_value);
                    }
                    break;
                }
            }

            if(new_task_cnt){

                //cur_pextadv->syncinfo_used |= SYNC_INFO_VALID;
                blt_ll_addTask2ExistLinklist( &cur_pPerdadv->schTsk_fifo[0],cur_pPerdadv->schTsk_wptr + 1);
            }
        }
    }

    return 0;
}

static inline void aux_adv_syncInfo_cal(st_ext_adv_t *cur_pextadv, s32 number_us){
    int num_us_30u = number_us/30; //Round down 1 unit

    num_us_30u = num_us_30u*30;

    if(num_us_30u >= 245700 + 30){
        if(num_us_30u > 2457600){
            cur_pextadv->auxSyncInfo.offsetAdjust = 1;
            num_us_30u -= 2457600;
        }

        int number_300us = num_us_30u/300;

        cur_pextadv->auxSyncInfo.offsetUnit = EXT_ADV_PDU_SYNC_OFFSET_UNITS_300_US;
        cur_pextadv->auxSyncInfo.syncPktOffset = number_300us;
    }
    else{
        int number_30us = number_us/30; //Round down 1 unit

        cur_pextadv->auxSyncInfo.syncPktOffset = number_30us;
    }
}
_attribute_ram_code_
int         blt_ll_aux_syncinfo_update(int extadv_index)
{
    st_ext_adv_t *cur_pextadv = (st_ext_adv_t *) (global_pextadv + extadv_index);
    st_prd_adv_t *cur_pPerdadv = (st_prd_adv_t *) (global_pPerdadv + cur_pextadv->mapping_prdadv_idx);

    if(cur_pextadv->syncinfo_used)
    {
        //periodic adv not start.two situation:
        //situation 1: PDA task mark is longer than current task;
        //situation 2: PDA task's priority is low, no change to add to task linker and not run.
        if(cur_pextadv->syncinfo_used == SYNC_INFO_NEED){//periodic adv not be sent once.

            cur_pextadv->auxSyncInfo.evtCounter   = 0;
            cur_pextadv->auxSyncInfo.offsetAdjust = 0;
            cur_pextadv->auxSyncInfo.offsetUnit   = EXT_ADV_PDU_SYNC_OFFSET_UNITS_30_US;

            //+interval is because the real pda time is mark+interval. later will optimize the mark processing.
            u32 pda_bSlot_1stSendMark = cur_pPerdadv->pda_tx.bSlot_mark_prdadv + cur_pPerdadv->pda_tx.bSlot_prdadv_itvl;
            s32 bSlot_diffWithCurTskTail = bltSche.bSlot_idx_irq_real + cur_pextadv->bSlotDuration_auxadv - pda_bSlot_1stSendMark;

            s32 diff_sSlot = 0;
            s32 number_us  = 0;

            if(bSlot_diffWithCurTskTail < 0){//situation 1.
                diff_sSlot = ((pda_bSlot_1stSendMark - bltSche.bSlot_idx_start)<<5) - bltSche.sSlot_idx_irq_real;
            }else{ //situation 2.
                int num_inter = bSlot_diffWithCurTskTail/cur_pPerdadv->pda_tx.bSlot_prdadv_itvl + 1;
                cur_pextadv->auxSyncInfo.evtCounter = cur_pPerdadv->pda_tx.paEvtCnt + num_inter;

                u32 bSlot_target = pda_bSlot_1stSendMark + num_inter*cur_pPerdadv->pda_tx.bSlot_prdadv_itvl;
                diff_sSlot =  ((bSlot_target - bltSche.bSlot_idx_start )<<5) - bltSche.sSlot_idx_irq_real;
            }

            number_us = diff_sSlot*SSLOT_US_NUM;

            //according aux_adv_ind's RF real start tick and aux_sync_ind's RF real start tick.
            u16 aux_sync_tx_offset_us = TLK_TX_TRIG_OFFSET + TX_STL_ADV_REAL_COMMON;
            u16 aux_adv_tx_offset_us = bltAdv.advTxDly_us + blt_pextadv->aux_align_dly_us + TX_STL_ADV_REAL_COMMON;

            s32 off_calib_us = (s32)(aux_sync_tx_offset_us - aux_adv_tx_offset_us);
            number_us += off_calib_us;

            aux_adv_syncInfo_cal(cur_pextadv, number_us);

            return 1;
        }

        ////////////////here:periodic adv has been sent at least once.//////////////////////
        u32 bSlot_aux_task_end = bltSche.bSlot_idx_irq_real + cur_pextadv->bSlotDuration_auxadv;
        u32 bSlot_distance = bSlot_aux_task_end - cur_pPerdadv->pda_tx.bSlot_mark_prdadv;
        int num_inter = bSlot_distance/cur_pPerdadv->pda_tx.bSlot_prdadv_itvl + 1;
        //int mod = bSlot_distance%cur_pPerdadv->pda_tx.bSlot_prdadv_itvl;


        DBG_CHN3_TOGGLE;DBG_CHN3_TOGGLE;
        cur_pextadv->auxSyncInfo.evtCounter = cur_pPerdadv->pda_tx.paEvtCnt + num_inter - 1;

        //my_dump_str_data(0, "evtCnt", &cur_pextadv->auxSyncInfo.evtCounter,4);

        u32 bSlot_target = cur_pPerdadv->pda_tx.bSlot_mark_prdadv + num_inter*cur_pPerdadv->pda_tx.bSlot_prdadv_itvl;
        s32 diff_sSlot =  ((bSlot_target - bltSche.bSlot_idx_start )<<5) - bltSche.sSlot_idx_irq_real;

        s32 number_us = diff_sSlot*SSLOT_US_NUM;   //sSlot -> us

        /**
         * (T1+ offset1) - (T0 + offset0) = (T1 - T0) + (offset1 - offset0)
         */
        s32 off_calib_us = (s32)(cur_pPerdadv->aux_sync_tx_off_us - cur_pextadv->aux_adv_tx_off_us);
        number_us += off_calib_us;


        //my_dump_str_u32s(DBG_PRDADV_LOGIC, "debug 1", cur_pPerdadv->pda_tx.bSlot_mark_prdadv, bltSche.bSlot_idx_irq_real, cur_pextadv->bSlotDuration_auxadv, bSlot_aux_task_end);
        //my_dump_str_u32s(DBG_PRDADV_LOGIC, "debug 2", cur_pPerdadv->pda_tx.bSlot_mark_prdadv, quotient, diff_sSlot, number_us);


        /*The value of the Sync Packet Offset field is in the unit of time indicated by
        the Offset Units field; the actual offset is determined by multiplying the value
        by the unit and then, if the Offset Adjust field is set to 1, adding 2.4576 seconds.
        The Offset Units field shall be set to 0 if the Sync Packet Offset is less than 245,700 us.
        The Offset Adjust field shall be set to 0 if the Offset Units field is set to 0 or
        if the SyncInfo field appears within an advertising PDU.*/
        cur_pextadv->auxSyncInfo.offsetAdjust = 0;
        cur_pextadv->auxSyncInfo.offsetUnit = EXT_ADV_PDU_SYNC_OFFSET_UNITS_30_US;

        ////
        aux_adv_syncInfo_cal(cur_pextadv, number_us);
    }
    else{   // no periodic ADV sent before, sync_info invalid
        /* special use for timing not available
         * SPEC define: value of 0 for Sync packet Offset indicate that the time to the next AUX_SYNC_IND packet
         * is greater than can be represented */
        cur_pextadv->auxSyncInfo.syncPktOffset = 0;
        cur_pextadv->auxSyncInfo.evtCounter = 0;
    }
    return 1;
}



_attribute_ram_code_
void blt_prdadv_updatePram(st_prd_adv_t *cur_pPerdadv) //irq used optim latter
{
    my_dump_str_data(0, "blt_prdadv_updatePram", 0, 0);
    //see <<Extended ADV Data Format.xlsx>>  AUX_SYNC_IND
    int extended_header_len = 0;
    u8  extended_header_flg = 0;

    /***********************  AUX_SYNC_IND prepare  **********************************************************/
    ll_prd_adv_ind_header_t * p_adv_prd_ind = (ll_prd_adv_ind_header_t* )&cur_pPerdadv->prd_adv_1stPkt;

    //AUX_ADV_IND step 1: CTEInfor process
#if(LL_FEATURE_ENABLE_CONNECTIONLESS_CTE_TRANSMITTER)
    u8 prdIdx = cur_pPerdadv->prdadv_index;
    if(cte_connLess_switchPattern[prdIdx].cte_transmit_en){
        u8 CTEInfor = ( (cte_connLess_switchPattern[prdIdx].cte_len&0x1f) | (cte_connLess_switchPattern[prdIdx].cte_type&0x03)<<6 );

        smemcpy( (p_adv_prd_ind->data + extended_header_len), &CTEInfor, EXTHD_LEN_1_CTE);
        extended_header_len += EXTHD_LEN_1_CTE;
        extended_header_flg |= EXTHD_BIT_CTE_INFO;
    }
#endif

    u8 txPower_en_len = cur_pPerdadv->txPower_en_len;
    u8 acad_en_len = 0;
    if(cur_pPerdadv->acad_used & PERD_ACAD_CHMUPT_ENA){ /* high priority */
        //Len-Type-Val: value: chM[0-4]+Instant[5-6]
        acad_en_len = 9; //fixed length:1+1+7
        my_dump_str_data(0, "---chm_up", 0, 0);
    }else if(cur_pPerdadv->acad_used & PERD_ACAD_BIGINFO_ENA){
        acad_en_len = cur_pPerdadv->acad_field_len;
        my_dump_str_data(0, "---biginfo", 0, 0);
    }

    /*when code run here, TX_POWER did not calculated in extended_header_len, so here should consider its length*/
    u32 max_advData = 253 - extended_header_len - txPower_en_len - acad_en_len;

    /* When Aux_Sync_Ind has Aux_Chain_Ind, BIS encrypted, ACAD(59B)+TX_Power(1B)+AuxPtr(3B)=63B, plus Extended Header Flags(1B) = 64B
     * Extended Header Length max length: 63B, The TX_POWER field is not used here, and the space is enough!!!
     * CTEInfo: CTEInfo has been closed, TODO: if open
     */
    if(cur_pPerdadv->curLen_perdAdvData > max_advData){
        if(txPower_en_len && acad_en_len >= 59){
            max_advData += txPower_en_len;
            txPower_en_len = 0;
        }
    }

    if(cur_pPerdadv->curLen_perdAdvData > max_advData){ //with AUX_CHAIN_IND
        cur_pPerdadv->with_aux_chain_ind = 1;
        cur_pPerdadv->prd_1st_pkt_dataLen = max_advData - EXTHD_LEN_3_AUX_PTR;

        cur_pPerdadv->auxPtr_offset = extended_header_len; //mark position

        /* AuxPrt, only process part of AuxPtr, chn_index and aux_offset must calculate when sending ADV  */
        extended_header_len += EXTHD_LEN_3_AUX_PTR;
        extended_header_flg |= EXTHD_BIT_AUX_PTR;
    }
    else{  //no AUX_CHAIN_IND, no Aux_Ptr
        cur_pPerdadv->with_aux_chain_ind = 0;
        cur_pPerdadv->prd_1st_pkt_dataLen = cur_pPerdadv->curLen_perdAdvData;

        /* Don't need a pointer callback function for now, it's relatively brief. */
    #if(LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
        if (cur_pPerdadv->num_subevents){ //For Periodic ADV with Response
            cur_pPerdadv->subevtAvaAdvDataLen = max_advData;
        }
    #endif
    }

    //ADI processing. For periodic adv, core5.3 include ADI. //LL/DDI/ADV/BV-63-C
    if(cur_pPerdadv->include_ADI_flag){
        st_ext_adv_t *cur_pextadv = (st_ext_adv_t *)(global_pextadv + cur_pPerdadv->mapping_extadv_idx);

        if(!cur_pPerdadv->prd_DID_changed){
            cur_pPerdadv->prd_DID = ((clock_time()>>4) & 0xFFF)  | 0x001;
        }
        u16 adi_info = cur_pextadv->adv_sid<<12|cur_pPerdadv->prd_DID;
        smemcpy( (p_adv_prd_ind->data + extended_header_len), &adi_info, EXTHD_LEN_2_ADI);

        cur_pPerdadv->ADI_offset = extended_header_len;
        extended_header_len += EXTHD_LEN_2_ADI;
        extended_header_flg |= EXTHD_BIT_ADI;
    }
    //AUX_ADV_IND step 3: Tx Power process
    if(txPower_en_len){
        p_adv_prd_ind->data[extended_header_len] = ble_txPowerLevel;
        extended_header_len += EXTHD_LEN_1_TX_POWER;
        extended_header_flg |= EXTHD_BIT_TX_POWER;
    }

    cur_pPerdadv->txPower_offset = extended_header_len; //Mark the start offset of the ACAD field

    //AUX_SYNC_IND: ACAD Info process
    if(acad_en_len){
        //Only need update Big timing offset concerned parameters when before send AUX_SYNC_IND PKT !!!
        //Removed : buffer overflow
        //smemcpy((p_adv_prd_ind->data + extended_header_len), cur_pPerdadv->acad, acad_en_len);//For chmUpt, it is useless
        //my_dump_str_data(STACK_DUMP_EN,"ACAD Info process", cur_pPerdadv->acad, acad_en_len);
        extended_header_len += acad_en_len; //ACAD_len(varies length)
    }

    int last_pkt_rf_len, last_pkt_tx_us;
    int available_data_len, cur_data_len = 0, auxChainInd_extendHeaderLen;

    cur_pPerdadv->chain_ind_num = 0;
    if(cur_pPerdadv->with_aux_chain_ind){ //with AUX_CHAIN_IND

        #if (0) //debug
            if( (extended_header_len + 2 + cur_pPerdadv->prd_1st_pkt_dataLen) != 255){
                ADV_ERR_DEBUG(0x77020000 | extended_header_len<<8 | cur_pPerdadv->prd_1st_pkt_dataLen);
            }
        #endif

        //AUX_ADV_IND step 1: CTEInfo process //TODO  EXTHD_LEN_1_CTE
        //AUX_CHAIN_IND's ADI field (M)
        auxChainInd_extendHeaderLen = EXTHD_LEN_2_ADI + cur_pPerdadv->txPower_en_len;
        available_data_len = 253 - auxChainInd_extendHeaderLen;

        int restAdvData_len = cur_pPerdadv->curLen_perdAdvData - cur_pPerdadv->prd_1st_pkt_dataLen;
        while( restAdvData_len > 0)
        {
            if( restAdvData_len > available_data_len){
                cur_data_len = available_data_len - EXTHD_LEN_3_AUX_PTR;
            }
            else{
                cur_data_len = restAdvData_len;
            }

            cur_pPerdadv->chain_ind_dataLen[cur_pPerdadv->chain_ind_num++] = cur_data_len;

            restAdvData_len -= cur_data_len;
        }

        last_pkt_rf_len = cur_data_len + auxChainInd_extendHeaderLen + 2;
    }
    else{  //no AUX_CHAIN_IND
        last_pkt_rf_len = cur_pPerdadv->curLen_perdAdvData + extended_header_len + 2;
    }

    /* Don't need a pointer callback function for now, it's relatively brief. No need  */
#if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
    if(cur_pPerdadv->num_subevents){
        /* AUX_SYNC_SUBEVENT_IND for Create LE connection: aux_connect_req + aux_connect_rsp */
        int aux_evt_us = blt_phy_getRfPacketTime_us(34/* AUX_CONN_REQ */ + 14/* AUX_CONN_RSP */, cur_pPerdadv->pda_tx.pda_phy, cur_pPerdadv->coding_ind) + BLE_T_IFS*2;
        last_pkt_rf_len = min(255, cur_pPerdadv->maxLen_subeventData + extended_header_len + 2);
        last_pkt_tx_us = blt_phy_getRfPacketTime_us(last_pkt_rf_len, cur_pPerdadv->pda_tx.pda_phy, cur_pPerdadv->coding_ind);

        /* task duration contain AUX_CONN_REQ/RSP */
        last_pkt_tx_us = max(last_pkt_tx_us, aux_evt_us);
    }
    else
#endif
    {
        last_pkt_tx_us = blt_phy_getRfPacketTime_us(last_pkt_rf_len, cur_pPerdadv->pda_tx.pda_phy, cur_pPerdadv->coding_ind);
    //  last_pkt_tx_us = blt_phy_getRfPacketTime_us(last_pkt_rf_len, cur_pPerdadv->pda_tx.pda_phy, LE_CODED_S8);
    }

    u16 pda_tx_prepare_us = TX_STL_ADV_REAL_COMMON;
    /* rfLen_255_pkt_us has been calculated when set ext_adv_parameters, here use it directly */
    cur_pPerdadv->prd_evt_us = bltAdv.advTxDly_us + 29 + pda_tx_prepare_us + cur_pPerdadv->rfLen_255_pkt_us * cur_pPerdadv->chain_ind_num + last_pkt_tx_us + ADV_TAIL_MARGIN_US;  //align_delay max value 29uS

    cur_pPerdadv->pda_tx.sSlot_duration_pda = (cur_pPerdadv->prd_evt_us + (SLOT_PROCESS_MAX_TICK/SYSTEM_TIMER_TICK_1US))*SSLOT_US_REVERSE + 1;

    cur_pPerdadv->ACAD_advData_offset = extended_header_len;
    p_adv_prd_ind->ext_hdr_flg = extended_header_flg;
    p_adv_prd_ind->ext_hdr_len = extended_header_len + 1;
    p_adv_prd_ind->rf_len  = extended_header_len + 2 + cur_pPerdadv->prd_1st_pkt_dataLen;
    p_adv_prd_ind->dma_len = rf_tx_packet_dma_len(p_adv_prd_ind->rf_len + 2);
}


_attribute_ram_code_ void blt_prdadv_updateAcadPram(st_prd_adv_t *cur_pPerdadv, u8 acad_used)
{
    if(acad_used & PERD_ACAD_BIGINFO_DIS){
        cur_pPerdadv->acad_used &= ~PERD_ACAD_BIGINFO_ENA;
        acad_used &= ~(PERD_ACAD_BIGINFO_ENA|PERD_ACAD_BIGINFO_DIS);
    }
    if(acad_used & PERD_ACAD_CHMUPT_DIS){
        cur_pPerdadv->acad_used &= ~PERD_ACAD_CHMUPT_ENA;
        acad_used &= ~(PERD_ACAD_CHMUPT_ENA|PERD_ACAD_CHMUPT_DIS);
    }

    cur_pPerdadv->acad_used |= acad_used; //Aux_Sync_Ind { + ACAD(BIGInfo) : after BIG task begin, we will enable ACAD later!!! }

    /////// Update ACAD field part ///////
    blt_prdadv_updatePram(cur_pPerdadv);
    /*
     * Important hint:
     * Because the ACAD update causes the current task length to change, pda_tx.sSlot_duration_pda
     * must be updated here, otherwise there is a bug!!!
     */
    cur_pPerdadv->pda_tx.sSlot_duration_pda = (cur_pPerdadv->prd_evt_us + (SLOT_PROCESS_MAX_TICK/SYSTEM_TIMER_TICK_1US))*SSLOT_US_REVERSE + 1;//Must update it
}


_attribute_ram_code_
int         blt_prdadv_start(int slotTask_idx)
{
    DBG_CHN0_HIGH;
    bltPrdA.prd_adv_sel = slotTask_idx;
    blt_pPerdadv = (st_prd_adv_t *) (global_pPerdadv + bltPrdA.prd_adv_sel);
    blt_pextadv = (st_ext_adv_t *) (global_pextadv + blt_pPerdadv->mapping_extadv_idx);
    blt_pPda = (st_pda_t *) &blt_pPerdadv->pda_tx;

    // LL/DDI/ADV/BV-39-C, if disable Periodic ADV, can not be sent. add by lijing
    if(!blt_pPerdadv->prd_adv_en){
        blt_ll_calculate_sSlot_next(clock_time() + SLOT_PROCESS_MAX_TICK);
        blms_state = BLMS_STATE_EXTADV_E;
        return 0;
    }

    /* Different process for different MCU: ******************************************/
    ble_rf_set_tx_dma(0, 17);
     //  This register exists in Kite/Vulture/826x, riscv architecture chips such as
     //  Eagle do not have this register, but DMA fifo can be turned off by forcibly
     //  setting DMA TX rptr = DMA TX wptr to use DMA default tx fifo.If the hardware
     //  TX rptr of DMA TX fifo == hardware TX wptr, then send DMA Tx default fifo,
     //  otherwise send DMA Tx fifo non-default area.
    HAL_REG_RF_DMA_FIFO_TX_RPTR = FLD_DMA_RPTR_CLR;
    /**********************************************************************************/


    int tmp_jump = blt_pda_start_common_1();
    u8 periodic_coded_phy_ind = bltPHYs.cur_llPhy;

#if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
    if(blt_pPerdadv->num_subevents){
        //if(ll_pawra_sub_irq_task_cb)
        {
            ll_pawra_sub_irq_task_cb(FLAG_SCHEDULE_PAWRA_1ST_SUB | slotTask_idx, NULL); //blt_pawra_sub0_mark
        }
    }
#endif

    if(tmp_jump > 0){
        blt_ll_incSchedulerTaskPriority( TSKOFT_PERD_ADV + bltPrdA.prd_adv_sel, bltPri.step_final[TSKOFT_PERD_ADV + bltPrdA.prd_adv_sel]*2*tmp_jump );
    }
    blt_pPda->paEvtCnt++;  //important here

    rf_ble_set_tx_settle(bltPHYs.tx_stl_adv); //attention: must set after PHY switch !!!

    STOP_RF_STATE_MACHINE;                      // stop SM
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

            #if (ONLY_FOR_EBQ_TEST_LATER_REMOVE)
                if(bigExtAuxPda_conflictCtrl.acadInfor_changeCtrl == ACAD_VALID_PENDING)
                {
                    bigExtAuxPda_conflictCtrl.acadInfor_changeCtrl = ACAD_VALID_COMPLETE;
                }
            #endif
        }
    }
    else{
        int send_dataLen = 0;
        u8 aux_chn_index = 0, aux_chn_backup;

        #if (ONLY_FOR_EBQ_TEST_LATER_REMOVE)
            if(bigExtAuxPda_conflictCtrl.bigTask_timingStart && bigExtAuxPda_conflictCtrl.acadInfor_changeCtrl == ACAD_VALID_COMPLETE){
                bigExtAuxPda_conflictCtrl.pdaAdv_sendNum++;
            }
        #endif

        u32 tick_wait;
    //  u32 tx_begin_tick = bltSche.sSlot_tick_irq + TLK_TX_TRIG_OFFSET * SYSTEM_TIMER_TICK_1US;
        u32 tx_begin_tick = bltSche.bSlot_tick_irq_real + TLK_TX_TRIG_OFFSET * SYSTEM_TIMER_TICK_1US;
        blt_pPerdadv->aux_sync_tx_off_us = TLK_TX_TRIG_OFFSET + TX_STL_ADV_REAL_COMMON;


    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
        if(blt_pPerdadv->num_subevents){
            if(blmsParam.create_connection == CONNECT_REQ_FOR_PAWR && 1 == blt_pPerdadv->initSubevent){
                //if(ll_pawra_sub_irq_task_cb)
                {
                    ll_pawra_sub_irq_task_cb(FLAG_SCHEDULE_PAWRA_CONN_REQ | slotTask_idx, NULL); //blt_pawra_sub_prepare_connect
                    goto skip_pawr_subevent0_send;
                }
            }
        }
    #endif

        /* step 1: trigger RF mode  */
        rf_start_fsm(FSM_STX, &pkt_periodic, tx_begin_tick);
        if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_TX_ON); }


        /* step 2: set AUX_SYNC_IND packet parameters by order  */
        smemcpy(&pkt_periodic, &blt_pPerdadv->prd_adv_1stPkt, blt_pPerdadv->prd_adv_1stPkt.ext_hdr_len - 1 + AUX_ADV_FORMAT_LEN);

        if(blt_pPerdadv->include_ADI_flag){
        /*
            for PAwR, we do not find evidence to verify that DID should be updated for each subevent, even if the adv data is always NULL. 
            but only update DID for each subevent can pass the case BV-88-C. by lihaojie 2024.06.12
        */
        #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
            if(blt_pPerdadv->num_subevents){
                blt_pPerdadv->prd_DID = ((clock_time()>>4) & 0xFFF)  | 0x001;
                u16 adi_info = blt_pextadv->adv_sid<<12|blt_pPerdadv->prd_DID;
                smemcpy((pkt_periodic.data + blt_pPerdadv->auxPtr_offset), &adi_info, EXTHD_LEN_2_ADI);
            }
        #endif
            if(blt_pPerdadv->prd_DID_changed){
                blt_pPerdadv->prd_DID = ((clock_time()>>4) & 0xFFF)  | 0x001;
                u16 adi_info = blt_pextadv->adv_sid<<12|blt_pPerdadv->prd_DID;
                smemcpy((pkt_periodic.data + blt_pPerdadv->auxPtr_offset), &adi_info, EXTHD_LEN_2_ADI);
            }
        }

        if(blt_pPerdadv->with_aux_chain_ind){
            /* only one situation: non_connectable_non_scannable */
            gbl_auxPtr.chn_index = aux_chn_index = BLT_GENERATE_AUX_CHN; //Generate random channel index for AUX_CHAIN_IND
            gbl_auxPtr.aux_offset = blt_pPerdadv->n_30us_chain_ind;
            gbl_auxPtr.aux_phy = blt_pPerdadv->pda_tx.pda_phy - 1;  // le_phy_type_t 1/2/3 corresponding 0/1/2 in packet
            smemcpy((pkt_periodic.data + blt_pPerdadv->auxPtr_offset), &gbl_auxPtr, EXTHD_LEN_3_AUX_PTR);
        }

        /* step3: Make AUX_SYNC_IND ACAD && data PDU field part */
        if(blt_pPerdadv->acad_used & PERD_ACAD_CHMUPT_ENA){ ////If exist ACAD field(e.g.: ChmUpt
            //u8 chmUptInd[9] = { 8, DT_CHM_UPT_IND, 0, 0, 0, 0, 0 , 0, 0};
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

        // 1st: update PAwR-AdvData ASAP
    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
        if(blt_pPerdadv->num_subevents){
            //if(ll_pawra_sub_irq_task_cb)
            {
                ll_pawra_sub_irq_task_cb(FLAG_SCHEDULE_PAWRA_ADVDATA_UPT | slotTask_idx, NULL); //blt_pawra_subx_advdata_update
            }
        }
        else
    #endif
        {
            //Update ADV data
            smemcpy((pkt_periodic.data + blt_pPerdadv->ACAD_advData_offset), blt_pPerdadv->dat_perdAdvData, blt_pPerdadv->prd_1st_pkt_dataLen);
        }

        #if(ADV_DURATION_STALL_EN)
            cpu_stall_WakeUp_By_RF_SystemTick(IRQ_ZB_RT, FLD_RF_IRQ_TX, 0);
        #else
            u32 pkt_periodic_us = blt_phy_getRfPacketTime_us(pkt_periodic.rf_len, blt_pPda->pda_phy, periodic_coded_phy_ind);
            u32 pkt_periodic_targetTick = tx_begin_tick + (bltPHYs.tx_stl_adv + pkt_periodic_us + 10)*SYSTEM_TIMER_TICK_1US;

            while (!HAL_GET_RF_TX_IRQ && tick1_exceed_tick2(pkt_periodic_targetTick, clock_time())){//wait for TX finish 11111
                if(usr_irq_handler_cb){ usr_irq_handler_cb(); }
            }
        #endif

        /* step4: Make AUX_CHAIN_IND PDU && send them */
        if(blt_pPerdadv->with_aux_chain_ind){
            aux_chn_backup = aux_chn_index;
            send_dataLen = blt_pPerdadv->prd_1st_pkt_dataLen;


            /* setting below, optimize, no need set, keep same with first AUX_SYNC_IND */
            //pkt_periodic.type = LL_TYPE_AUX_CHAIN_IND;
            //pkt_periodic.chan_sel;
            //pkt_periodic.txAddr = 0;
            //pkt_periodic.rxAddr = 0;   //LL_TYPE_AUX_CHAIN_IND no "TargetA"
            //pkt_periodic.adv_mode = LL_EXTADV_MODE_NON_CONN_NON_SCAN;


            /* ADI can not exist(C3, AUX_SYNC_IND's AUX_CHAIN_IND, here must not present) */
            #if 0
            u16 adi_info = blt_pextadv->adv_sid<<12 | blt_pextadv->adv_did;
            smemcpy( (pkt_periodic.data + 0), &adi_info, EXTHD_LEN_2_ADI);
            #endif


            for(int i=0; i<blt_pPerdadv->chain_ind_num; i++){

                /* ADI can not exist(C3, AUX_SYNC_IND's AUX_CHAIN_IND, here must not present) */
                #if 0
                    int headr_len = EXTHD_LEN_2_ADI;
                    int headr_flg = EXTHD_BIT_ADI;
                #else
                    int headr_len = 0;
                    int headr_flg = 0;
                #endif

                if(i == (blt_pPerdadv->chain_ind_num - 1)){ //last packet

                }
                else{
                    aux_chn_index = BLT_GENERATE_AUX_CHN; //Generate random channel index for AUX_CHAIN_IND

                    gbl_auxPtr.chn_index = aux_chn_index;
                    //gbl_auxPtr.aux_offset = blt_pextadv->n_30us_chain_ind;  //same value as last, no set to save SRAM
                    //gbl_auxPtr.aux_phy = blt_pextadv->pda_tx.pda_phy - 1;       //same value as last, no set to save SRAM
                    smemcpy( (pkt_periodic.data + headr_len), &gbl_auxPtr, EXTHD_LEN_3_AUX_PTR);

                    headr_len += EXTHD_LEN_3_AUX_PTR;
                    headr_flg |= EXTHD_BIT_AUX_PTR;
                }

            #if LL_DDI_ADV_BV61C
                if(0)//QW:AUX_ADV_IND can not include TXPOWER field. it is confusing how to add TXPOWER or other fields.
            #else
                if(blt_pPerdadv->txPower_en_len)
            #endif
                {
                    pkt_periodic.data[headr_len] = ble_txPowerLevel;
                    headr_len += EXTHD_LEN_1_TX_POWER;
                    headr_flg |= EXTHD_BIT_TX_POWER;
                }

                smemcpy((pkt_periodic.data + headr_len), blt_pPerdadv->dat_perdAdvData + send_dataLen, blt_pPerdadv->chain_ind_dataLen[i]);
                send_dataLen += blt_pPerdadv->chain_ind_dataLen[i];

                pkt_periodic.ext_hdr_len = headr_len + 1;
                pkt_periodic.ext_hdr_flg = headr_flg;
                pkt_periodic.rf_len = 2 + headr_len + blt_pPerdadv->chain_ind_dataLen[i];
                pkt_periodic.dma_len = rf_tx_packet_dma_len(pkt_periodic.rf_len + 2);

                tx_begin_tick += blt_pPerdadv->rfLen_255_pkt_us*SYSTEM_TIMER_TICK_1US;
                tick_wait = tx_begin_tick - FSM_TRIGGER_EARLY_WAIT_TICK;

            #if(ADV_DURATION_STALL_EN)
            #else
                while((u32)(clock_time() - tick_wait) > BIT(30));
            #endif

                rf_set_ble_channel (aux_chn_backup);
                aux_chn_backup = aux_chn_index;

                rf_start_fsm(FSM_STX, &pkt_periodic, tx_begin_tick);
                HAL_CLEAR_RF_TX_RX_IRQ;

            #if(ADV_DURATION_STALL_EN)
                cpu_stall_WakeUp_By_RF_SystemTick(IRQ_ZB_RT, FLD_RF_IRQ_TX, 0);
            #else
                u32 periodic_chain_us = blt_phy_getRfPacketTime_us(pkt_periodic.rf_len, blt_pPda->pda_phy, periodic_coded_phy_ind);
                u32 periodic_chain_targetTick = tx_begin_tick + (bltPHYs.tx_stl_adv + periodic_chain_us + 10)*SYSTEM_TIMER_TICK_1US;
                while (!HAL_GET_RF_TX_IRQ && tick1_exceed_tick2(periodic_chain_targetTick, clock_time())){//wait for TX finish  11111
                    if(usr_irq_handler_cb){ usr_irq_handler_cb(); }
                }
            #endif
            }
        }
    }

#if(LL_FEATURE_ENABLE_CONNECTIONLESS_CTE_TRANSMITTER)
    rf_set_aoa_aod_trx_mode(RF_AOA_OFF);
#endif

    // 1st: Subevent Data Request event; 2nd: insert RSP TASK
#if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)

skip_pawr_subevent0_send:

    if(blt_pPerdadv->num_subevents){
        //if(ll_pawra_rsp_irq_task_cb)
        {
            ll_pawra_rsp_irq_task_cb(FLAG_INSERT_PAWRA_SLOT_TASK | slotTask_idx, NULL); //blt_pawra_rsp_task_insert
        }
    }
#endif

    /* important: ensure that FSM stopped */
    STOP_RF_STATE_MACHINE;
    /* clear status as late as possible, cause if clear too early, some status did not come, e.g. STX cmd done*/
    CLEAR_ALL_RFIRQ_STATUS;

    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_OFF); }

    blms_state = BLMS_STATE_EXTADV_E;

    /* When sending AUX_ADV_IND, record the PE corresponding to the currently pointed Periodic ADV
     * and the first bit anchor point of the corresponding Periodic ADV packet in FUNC: blt_ll_aux_syncinfo_update.
     * This can be simplified, and the relevant information is recorded before sending Periodic ADV, process like below. */
    //for PAST sender timing calculation
    //Used to accurately obtain the starting anchor point of the data packet corresponding to the current PE
    blt_pPda->lastPaAnchorPoint = bltSche.bSlot_tick_irq_real + (blt_pPerdadv->aux_sync_tx_off_us + 2) * SYSTEM_TIMER_TICK_1US; //margin: plus 2
    blt_pPda->lastPaEvtCnt = blt_pPda->paEvtCnt - 1; //plus 1

    blt_pda_start_common_2();

    blt_pextadv->syncinfo_used |= SYNC_INFO_VALID;
    blt_pda_post_common();

    DBG_CHN0_LOW;

    //s32 sSlotCurr = TICKS_ABS_2_SSLOT_ABS(clock_time());
    s32 sSlotTaskEnd = bltSche.sSlot_idx_irq_real + blt_pPerdadv->pda_tx.sSlot_duration_pda;
    //my_dump_str_u32s(0, "xxx", sSlotCurr, sSlotTaskEnd, sSlotTaskEnd-sSlotCurr, sSlotCurr-sSlotTaskEnd);

    /* If reschedule task (here insert rsp task will trigger reschedule), need margin time, here 100us seems safe */
    blt_ll_calculate_sSlot_next(min(clock_time() + SLOT_PROCESS_MAX_TICK, SSLOT_ABS_2_TICKS_ABS(sSlotTaskEnd))); //SLOT_PROCESS_MAX_TICK);
    return 0;
}





int         blt_ll_ctrlPerdAdvChClassUpd(unsigned char *pChm)
{
    st_prd_adv_t *pPerdadv = NULL;
    for(int i=0; i < bltPrdA.maxNum_perdAdv; i++)
    {
        pPerdadv = (st_prd_adv_t *)(global_pPerdadv + i);

        if(pPerdadv->prd_adv_en){
            pPerdadv->pda_tx.prd_map_inst_next = pPerdadv->pda_tx.paEvtCnt + 10;
            smemcpy(pPerdadv->pda_tx.nextChn.chmTbl, pChm, 5);
            csa2_calculateMapInfo(&pPerdadv->pda_tx.nextChn);
            pPerdadv->pda_tx.update_map = PDA_UPDATE_MAP;
            pPerdadv->acad_chaged = 1;
            blt_prdadv_updateAcadPram(pPerdadv, PERD_ACAD_CHMUPT_ENA);
            pPerdadv->acad_chaged = 2;

            #if (ONLY_FOR_EBQ_TEST_LATER_REMOVE)
                bigExtAuxPda_conflictCtrl.acadInfor_changeCtrl = ACAD_VALID_PENDING;
            #endif
        }
    }

    return 1;
}





#endif  //end of LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING


