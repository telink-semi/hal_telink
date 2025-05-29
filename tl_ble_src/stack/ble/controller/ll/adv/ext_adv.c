/********************************************************************************************************
 * @file    ext_adv.c
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

#if (BLE_LLMIC_CONCURRENT_EN)
    #include "stack/ble/controller/ll/llmic/llmic.h"
    #include "stack/ble/controller/ll/llmic/llmic_internal.h"
#endif


#if (LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING)

    #if OS_SUP_EN
        #include "stack/ble/os_sup/os_sup.h"
        #include "stack/ble/os_sup/os_sup_stack.h"
    #endif


_attribute_ble_data_retention_ ll_extadv_t bltExtA;

_attribute_ble_data_retention_ st_ext_adv_t *global_pextadv = NULL; //global adv_set data pointer

_attribute_ble_data_retention_ st_ext_adv_t *blt_pextadv = NULL;    //used in IRQ

_attribute_ble_data_retention_ _attribute_aligned_(4) rf_pkt_ext_adv_t pkt_secondary = {0};


    #ifndef BQB_5P0_TEST_ENABLE
        #define BQB_5P0_TEST_ENABLE 0
    #endif


_attribute_ble_data_retention_
    rf_pkt_aux_conn_rsp_t pkt_aux_conn_rsp = {
        rf_tx_packet_dma_len(14 + 2), // dma_len: rf_len + 2

        LL_TYPE_AUX_CONNECT_RSP, // type
        0, // RFU
        0, // "ChSel" only valid in ADV_IND/ADV_DIRECT_IND/CONNECT_IND, other packet set 0'b
        0, // txAddr           may change
        0, // rxAddr           may change

        14, // rf_len:  sizeof(rf_pkt_aux_conn_rsp_t) - 6

        13, // ext_hdr_len: Extended Header Flags(1) + AdvA(6) + TargetA(6)
        LL_EXTADV_MODE_NON_CONN_NON_SCAN, // adv_mode

        EXTHD_BIT_ADVA | EXTHD_BIT_TARGETA, // ext_hdr_flg : AdvA | TargetA

        {0, 0, 0, 0, 0, 0}, // advA             need change
        {0, 0, 0, 0, 0, 0}, // targetA          need change
};


    #if (LL_FEATURE_ENABLE_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE && EXTADV_DATA_CHANGE_MANUAL_DATA_BUFFER)


        #define STK_ADV_SETS_NUMBER               2
        #define STK_MAX_LENGTH_ADV_DATA           320
        #define STK_MAX_LENGTH_SCAN_RESPONSE_DATA 320


_attribute_iram_noinit_data_ u8 backup_advData_buffer[STK_MAX_LENGTH_ADV_DATA * STK_ADV_SETS_NUMBER];
_attribute_iram_noinit_data_ u8 backup_scanRspData_buffer[STK_MAX_LENGTH_SCAN_RESPONSE_DATA * STK_ADV_SETS_NUMBER];


    #endif


// if pSecAdv can set to NULL, means using legacy ADV
_attribute_noinline_
    ble_sts_t
    blc_ll_initExtendedAdvModule_initExtendedAdvSetParamBuffer(u8 *pBuff_advSets, int num_advSets)
{
    STATIC_ASSERT_FILE(ADV_SET_PARAM_LENGTH >= sizeof(st_ext_adv_t), ext_adv);


    #if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_adv_t)), ext_adv);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_legadv_t)), ext_adv);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_extadv_t)), ext_adv);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(st_ext_adv_t)), ext_adv);
    #endif

    #if (TSKNUM_EXT_ADV != TSKNUM_AUX_ADV)
        #error "task number must be same !"
    #endif

    if (num_advSets > TSKNUM_EXT_ADV) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    memset(pBuff_advSets, 0, num_advSets * ADV_SET_PARAM_LENGTH);
#if (!ESL_RAM_OPTIMIZATION)
    blc_ll_init2MPhyCodedPhy_feature(); //need 2M/Coded PHY feature
#endif(!ESL_RAM_OPTIMIZATION)
    blc_ll_initChannelSelectionAlgorithm_2_feature(); //need CSA #2

    LL_FEATURE_MASK_0 |= (LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING << 12);

    blmsParam.extAdvModule_en = 1;

    blt_ll_initAdvertisingCommon();

    ll_ext_adv_irq_task_cb = blt_ext_adv_interrupt_task;
    ll_ext_adv_mlp_task_cb = blt_ext_adv_mainloop_task;

    /* can be set(e.g. set to 100uS) by user with an API to avoid other IRQ timing conflict or flash operation disable IRQ */
    bltAdv.advTxDly_us = ADV_DFT_TX_DELAY_US;


    /////////initExtendedAdvSetParamBuffer//////////////////////////
    global_pextadv         = (st_ext_adv_t *)pBuff_advSets;
    bltExtA.maxNum_advSets = num_advSets;

    st_ext_adv_t *cur_pextadv;
    for (int i = 0; i < bltExtA.maxNum_advSets; i++) { //clear ADV handle

        cur_pextadv               = (st_ext_adv_t *)(global_pextadv + i);
        cur_pextadv->extadv_index = i;

        //set some default value
        cur_pextadv->adv_handle    = INVALID_ADVHD_FLAG;
        cur_pextadv->extadv_en     = 0;
        cur_pextadv->prdadv_api_en = 0;

        blt_extadv_clear_adv_set_param(cur_pextadv);

    #if (NEED_MORE_TEST_TO_CONFIRM) //later will run more test to confirm. now two IAL cases are OK.
        cur_pextadv->bSlot_mark_extadv = trng_rand() & 0x0f;
    #else
        //-BIT(20), complement in store. bSlot_mark_extadv is u32,so the real value is 0xFFF00000. here should be error.
        //so qiuwei think
        cur_pextadv->bSlot_mark_extadv = -BIT(20);
    #endif
        cur_pextadv->sSlot_diff_extadv = 0;
        smemcpy(cur_pextadv->public_addr, bltMac.macAddress_public, BLE_ADDR_LEN);

        for (int j = 0; j < EXT_ADV_FIFONUM; j++) {
            cur_pextadv->extadv_schTsk_fifo[j].scheTask_oft = TSKOFT_EXT_ADV + i; //11 ~ 13
            cur_pextadv->extadv_schTsk_fifo[j].scheTask_idx = i;
            cur_pextadv->extadv_schTsk_fifo[j].scheTask_flg = TSKFLG_EXT_ADV;     //BIT(5)
        }
        //bltPriority[TSKOFT_EXT_ADV + i] = 0;  //debug

        cur_pextadv->auxadv_schTsk_fifo.scheTask_oft = TSKOFT_AUX_ADV + i;
        cur_pextadv->auxadv_schTsk_fifo.scheTask_idx = i;
        cur_pextadv->auxadv_schTsk_fifo.scheTask_flg = TSKFLG_AUX_ADV;

        blt_ll_setSchedulerTaskPriority(TSKOFT_AUX_ADV + i, TASK_PRIORITY_AUX_ADV);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////

    return BLE_SUCCESS;
}

_attribute_ram_code_ int blt_ext_adv_interrupt_task(int flag, void *p)
{
    int adv_idx = flag & FLAG_SCHEDULE_TASK_IDX_MASK;


    #if (BLE_LLMIC_CONCURRENT_EN)
    if (flag & FLAG_SCHEDULE_START) {
        if (adv_idx == 0) {
            DBG_SIHUI_CHN0_HIGH;
        } else {
            DBG_SIHUI_CHN1_HIGH;
        }

        blt_llmic_extadv_start(adv_idx);

        if (adv_idx == 0) {
            DBG_SIHUI_CHN0_LOW;
        } else {
            DBG_SIHUI_CHN1_LOW;
        }

    } else if (flag & FLAG_SCHEDULE_EXTADV_BUILD) {
        blt_llmic_build_extadv_task();
    }
    #else
    if (flag & FLAG_SCHEDULE_START) {
        blt_extadv_start(adv_idx);
    } else if (flag & FLAG_SCHEDULE_EXTADV_BUILD) {
        blt_ll_build_extadv_task();
    } else if (flag & FLAG_SCHEDULE_SEND_EXTADV) {
        blt_extadv_send();
    }
    #endif
    else if (flag & FLAG_SCHEDULE_BUILD) {
        blt_ll_build_auxadv_task();
    } else if (flag & FLAG_SCHEDULE_SEND_AUXADV) {
    #if (SL01_eadv_auxOrChain_ind)
        log_task_begin_irq(SL_STACK_EXT_PRD_BASE_TIMING_EN, SL01_eadv_auxOrChain_ind);
    #endif

        blt_auxadv_send(adv_idx);

    #if (SL01_eadv_auxOrChain_ind)
        log_task_end_irq(SL_STACK_EXT_PRD_BASE_TIMING_EN, SL01_eadv_auxOrChain_ind);
    #endif
    } else if (flag & FLAG_INSERT_SCHTSK_CONFLICT) {
    #if (0) /* ext_adv insert in the GAP, not encounter conflict */
        sch_task_t *pTgtTsk       = (sch_task_t *)p;
        u8          tgtTskFlg     = pTgtTsk->scheTask_flg & TSKFLG_VALID_MASK;
        u8          curSchTaskOft = TSKOFT_EXT_ADV + adv_idx;
        my_dump_str_data(0, "In[ext_adv],tgtTsk", &tgtTskFlg, 1);

        //It seems impossible
        if (tgtTskFlg == TSKFLG_PERD_ADV || tgtTskFlg == TSKFLG_BIG_BCST ||
            tgtTskFlg == TSKFLG_PDA_SYNC || tgtTskFlg == TSKFLG_BIG_SYNC) {
            my_dump_str_data(0, "In[ext_adv], it seems impossible", &tgtTskFlg, 1);
            return 0;
        }

        if (tgtTskFlg == TSKFLG_BIG_BCST && (global_pBigBcst + pTgtTsk->scheTask_idx)->big_sc_mask) {
            my_dump_str_data(0, "[ext_adv]abandon, bis_SC proc", &bltPri.csctvAbandonCnt[curSchTaskOft], 2) return 0;
        }
    #endif
    } else if (flag & FLAG_INSERT_AUXADV_SCHTSK_CONFLICT) {
        sch_task_t *pTgtTsk       = (sch_task_t *)p;
        u8          tgtTskFlg     = pTgtTsk->scheTask_flg & TSKFLG_VALID_MASK;
        u8          curSchTaskOft = TSKOFT_AUX_ADV + adv_idx;

    #if (SL08_auxAdv_conflict)
        log_b8_irq(SL_STACK_EXT_PRD_BASE_TIMING_EN, SL08_auxAdv_conflict, tgtTskFlg);
    #endif

    #if (ONLY_FOR_EBQ_TEST_LATER_REMOVE)
        //auxAdv_sendNum only ++ when send aux_adv_ind. if interval < 80ms,aux_adv_ind task will run more then 2, but ignore this situation.
        if (tgtTskFlg == TSKFLG_BIG_BCST) {
            if (bigExtAuxPda_conflictCtrl.auxAdv_sendNum < 2) {
                return 1; /* 1:conflict resolved; 0: insert task failed */
            } else {
                return 0; /* 1:conflict resolved; 0: insert task failed */
            }
        }
    #endif

    #if (SCH_TASK_PRIORITY_IN_CB_EN)
        s32 pri_taskCur = bltPri.pri_cal[curSchTaskOft];
        s32 pri_taskTra = bltPri.pri_cal[pTgtTsk->scheTask_oft];
        //priority higher than exist task, can insert target task
        if (pri_taskCur > pri_taskTra) {
            return 1;
        }
    #endif

        my_dump_str_u32s(0, "In[aux_adv],tgtTsk", tgtTskFlg, curSchTaskOft, bltPri.pri_cal[curSchTaskOft], bltPri.pri_cal[pTgtTsk->scheTask_oft]);

    #if (LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER)
        if (tgtTskFlg == TSKFLG_BIG_BCST && (global_pBigBcst + pTgtTsk->scheTask_idx)->big_sc_mask) {
            my_dump_str_data(0, "[aux_adv]abandon, bis_SC proc", &bltPri.csctvAbandonCnt[curSchTaskOft], 2);
            return 0;
        }
    #endif

        /*
         * The acl task is occupied by ext_adv or aux_adv (probabilistic), so the PAST packet
         * cannot be sent. TODO after EBQ certification (tuyf add 22-09-07)
         */
    #if (LL_CON_PER_BV88C) //temporary solution
        if (tgtTskFlg == TSKFLG_PERD_ADV || tgtTskFlg == TSKFLG_BIG_BCST ||
            tgtTskFlg == TSKFLG_PDA_SYNC || tgtTskFlg == TSKFLG_BIG_SYNC || tgtTskFlg == TSKFLG_ACL_SLAVE) {
    #else
        if (tgtTskFlg == TSKFLG_PERD_ADV || tgtTskFlg == TSKFLG_BIG_BCST ||
            tgtTskFlg == TSKFLG_PDA_SYNC || tgtTskFlg == TSKFLG_BIG_SYNC) {
    #endif
            my_dump_str_data(0, "[aux_adv]abandon", 0, 0);
            return 0;
        }

        //Task scheduler has been consecutive abandoned bigger than 2 times
        if (bltPri.csctvAbandonCnt[curSchTaskOft] >= 2) {
            my_dump_str_data(0, "[aux_adv]consecutive abandon count", &bltPri.csctvAbandonCnt[curSchTaskOft], 2);
            return 1; /* 1:conflict resolved; 0: insert task failed */
        }
    }
    return 0;
}

_attribute_noinline_ int blt_ext_adv_mainloop_task(int flag, void *p)
{
    (void)p; //unused, remove warning

    if (flag == (int)FLAG_MODULE_RESET) {
        blt_reset_ext_adv();
    } else if (flag == (int)FLAG_MODULE_MAINLOOP) {
        blt_extAdv_pendEvtProc_mainloop();
    }
    #if (LL_FEATURE_ENABLE_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE)
    else if (flag == (int)FLAG_EXTADV_SET_DATA_ADDR_CHANGE) {
        return blc_ll_setExtendedAdvDataRelatedAddressChange((hci_le_setDataAddrChange_cmdParams_t *)p);
    }
    #endif

    return 0;
}

//can not be "0"
ble_sts_t blc_ll_setAuxAdvChnIdxByCustomers(u8 aux_chn)
{
    //chn_index: 0~36 is OK
    if (aux_chn > 36) {
        return LL_ERR_INVALID_PARAMETER;
    } else {
        bltExtA.custom_aux_chn = aux_chn;
    }

    return BLE_SUCCESS;
}

//set extended ADV data buffer for all ADV sets
void blc_ll_initExtendedAdvDataBuffer(u8 *pExtAdvData, int max_len_advData)
{
    for (int i = 0; i < bltExtA.maxNum_advSets; i++) { //clear ADV handle
        (global_pextadv + i)->maxLen_advData = max_len_advData;
        (global_pextadv + i)->dat_extAdv     = (u8 *)(pExtAdvData + max_len_advData * i);

    #if (LL_FEATURE_ENABLE_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE && EXTADV_DATA_CHANGE_MANUAL_DATA_BUFFER)
        (global_pextadv + i)->backupDat_extAdv  = (u8 *)&backup_advData_buffer[max_len_advData * i];
        (global_pextadv + i)->backupDat_scanRsp = (u8 *)&backup_scanRspData_buffer[max_len_advData * i];
    #endif
    }
}

//set extended scan response data buffer for all ADV sets
void blc_ll_initExtendedScanRspDataBuffer(u8 *pScanRspData, int max_len_scanRspData)
{
    for (int i = 0; i < bltExtA.maxNum_advSets; i++) { //clear ADV handle
        (global_pextadv + i)->maxLen_scanRsp = max_len_scanRspData;
        (global_pextadv + i)->dat_scanRsp    = (u8 *)(pScanRspData + max_len_scanRspData * i);
    }
}

ble_sts_t blc_hci_le_readMaxAdvDataLength(u8 *maxAdvDataLength)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Read_Max_Advertising_Data_Length", 0, 0);
    //HCI/GEV/BV-02-C [Disallow Mixing Legacy and Extended Advertising Commands]
    if (IS_LEGACY_ADV_VALID) {
        return HCI_ERR_CMD_DISALLOWED;
    }

    SET_EXTENDED_ADV_VALID;

    //TODO: Use macro definition, LG ADV is fixed to 31, EXT_ADV macro definition is fixed, e.g.:200.
    //give the first adv_set data to host(if HCI project, all adv_set use same maxLength,
    //  if other project, we can provide different max Length for different adv_set to save SRAM)
    u16 max_length          = global_pextadv->maxLen_advData;
    *maxAdvDataLength       = U16_LO(max_length);
    *(maxAdvDataLength + 1) = U16_HI(max_length);

    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_readNumberOfSupportedAdvSets(u8 *Num_Supported_Advertising_Sets)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Read_Number_Of_Supported_Advertising_Sets", 0, 0);
    //HCI/GEV/BV-02-C [Disallow Mixing Legacy and Extended Advertising Commands]
    if (IS_LEGACY_ADV_VALID) {
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_EXTENDED_ADV_VALID;

    *Num_Supported_Advertising_Sets = bltExtA.maxNum_advSets;

    return BLE_SUCCESS;
}

//return 0: failed, 1:check ok
static bool blt_ll_extAdvChkDataItvl(st_ext_adv_t *pextadv, u16 extAdvItvl)
{
    /* If the advertising set already contains advertising data or scan response data,
    extended advertising is being used, and the length of the data is greater than
    the maximum that the Controller can transmit within the longest possible
    auxiliary advertising segment consistent with the parameters, the Controller
    shall return the error code Packet Too Long (0x45). */
    //Duration has taken into account different phy.
    u32 extAdvTotalDurationUs = ((u32)(pextadv->sSlotDuration_extadv + pextadv->sSlotDuration_auxadv)) * SSLOT_US_NUM;
    u32 extAdvIntvlUs         = extAdvItvl * 625;

    if (extAdvTotalDurationUs > extAdvIntvlUs) {
        return FALSE;
    }

    return TRUE;
}

    #if (LL_FEATURE_ENABLE_ADVERTISING_CODING_SELECTION)
ble_sts_t blc_ll_setExtAdvParam_v2(u8 adv_handle, advEvtProp_type_t adv_evt_prop, u32 pri_advInter_min, u32 pri_advInter_max, adv_chn_map_t pri_advChnMap, own_addr_type_t ownAddrType, u8 peerAddrType, u8 *peerAddr, adv_fp_type_t advFilterPolicy, tx_power_t adv_tx_pow, le_phy_type_t pri_adv_phy, u8 sec_adv_max_skip, le_phy_type_t sec_adv_phy, u8 adv_sid, u8 scan_req_notify_en, le_codedPhy_option pri_adv_codePhy_option, le_codedPhy_option sec_adv_codePhy_option)
{
    my_dump_str_u8s(BLC_LL_LOG_EN, "@BLC_LL_Set_Ext_Adv_Param", adv_handle, adv_evt_prop, ownAddrType, advFilterPolicy);

    st_ext_adv_t *cur_pextadv;
    if (adv_handle == INVALID_ADVHD_FLAG) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    } else {
        /*If the Advertising_Handle does not identify an existing advertising set and the
        Controller is unable to support a new advertising set at present, the Controller
        shall return the error code Memory Capacity Exceeded (0x07). */
        cur_pextadv = blt_extadv_search_existed_and_allocate_new_adv_set(adv_handle);
        if (!cur_pextadv) {
            return HCI_ERR_MEM_CAP_EXCEEDED;
        }
    }


    /*If the Host issues this command when advertising is enabled for the specified
    advertising set, the Controller shall return the error code Command Disallowed
    (0x0C).*/
    if (cur_pextadv->extadv_en) {
        return HCI_ERR_CMD_DISALLOWED;
    }

    /*
     * extended ADV can not be direct high duty. so the interval should be in the range 20 ms to 10,485.759375 s
     */
    if (pri_advInter_max < ADV_INTERVAL_20MS || pri_advInter_max > ADV_INTERVAL_10_24S) { //20ms/0.625 = 32
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }


    if (ownAddrType == OWN_ADDRESS_RESOLVE_PRIVATE_PUBLIC || ownAddrType == OWN_ADDRESS_RESOLVE_PRIVATE_RANDOM) {
        #if (LL_FEATURE_ENABLE_PRIVACY)

        #else
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        #endif

        //do nothing
    } else if (ownAddrType == OWN_ADDRESS_PUBLIC || ownAddrType == OWN_ADDRESS_RANDOM) {
        //do nothing
    } else {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }


    /* HCI/DDI/BI-15-C.
     * CORE SPEC V5.3 page2452:If the Host issues this command when periodic advertising is enabled for the specified advertising set and
     * connectable, scannable, legacy, or anonymous advertising is specified, the Controller shall return the error code Invalid HCI Command Parameters (0x12)
     */
    if (cur_pextadv->prdadv_api_en) { //not use prd_adv_en
        if (adv_evt_prop & (ADVEVT_PROP_MASK_ANON_ADV | ADV_EVT_PROP_LEGACY_CONNECTABLE_SCANNABLE_UNDIRECTED)) {
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
    }

    if (!blt_ll_extAdvChkDataItvl(cur_pextadv, pri_advInter_max)) {
        return HCI_ERR_PACKET_TOO_LONG;
    }

    /* If the Host issues this command when periodic advertising is enabled for the
    specified advertising set and connectable, scannable, legacy, or anonymous
    advertising is specified, the Controller shall return the error code Invalid HCI
    Command Parameters (0x12). */

    /* If periodic advertising is enabled for the advertising set and the
    Secondary_Advertising_PHY parameter does not specify the PHY currently
    being used for the periodic advertising, the Controller shall return the error
    code Command Disallowed (0x0C). */


    /* If the advertising set already contains advertising data or scan response data,
    extended advertising is being used, and the length of the data is greater than
    the maximum that the Controller can transmit within the longest possible
    auxiliary advertising segment consistent with the parameters, the Controller
    shall return the error code Packet Too Long (0x45). */


    cur_pextadv->evt_props      = adv_evt_prop;
    cur_pextadv->evt_prop_bit04 = adv_evt_prop & 0x1F;

    cur_pextadv->txPower_en_len = (adv_evt_prop & ADVEVT_PROP_MASK_INC_TX_PWR) ? 1 : 0; //1 and 0


    cur_pextadv->cur_advMode = (adv_evt_prop & ADVEVT_PROP_MASK_CONNECTABLE_SCANNABLE);

    /* using flash space to save Ram space and timing   */
    cur_pextadv->directed_adv = (adv_evt_prop & ADVEVT_PROP_MASK_DIRECTED);
    cur_pextadv->legacy_adv   = (adv_evt_prop & ADVEVT_PROP_MASK_LEGACY);

    cur_pextadv->adv_chn_mask   = pri_advChnMap;
    cur_pextadv->adv_chn_num    = 0;
    cur_pextadv->adv_chnIdx_1st = 0xFF;
    for (int i = 0; i < 3; i++) { //calculate how many channel used
        if (pri_advChnMap & BIT(i)) {
            cur_pextadv->adv_chn_num++;

            /* jump chn37/chn38 if ADV channel not set */
            if (0xFF == cur_pextadv->adv_chnIdx_1st) {
                cur_pextadv->adv_chnIdx_1st = i;
            }
        }
    }


    if (cur_pextadv->adv_chn_num == 0) {
        cur_pextadv->adv_chn_num = 1; //prevent (adv_chn_num - 1) == 0xff
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    //record owner ADR type
    cur_pextadv->own_addr_type   = ownAddrType;
    cur_pextadv->own_addr_rpa    = (ownAddrType & OWN_ADDRESS_TYPE_RPA_MASK);
    bltLegAdv.legadv_ownAddr_rpa = (ownAddrType & OWN_ADDRESS_TYPE_RPA_MASK);

    if (ownAddrType & OWN_ADDRESS_TYPE_RANDOM_MASK) {
        cur_pextadv->extadv_mac_type = BLE_ADDR_RANDOM;
        smemcpy(cur_pextadv->extadv_mac_addr, bltMac.macAddress_random, BLE_ADDR_LEN);
    } else {
        cur_pextadv->extadv_mac_type = BLE_ADDR_PUBLIC;
        smemcpy(cur_pextadv->extadv_mac_addr, bltMac.macAddress_public, BLE_ADDR_LEN);
    }

    cur_pextadv->adv_filterPolicy = (u8)advFilterPolicy;

        #if (DBG_PRVC_EXTADV_EN)
    if (adv_evt_prop & ADV_EVT_PROP_LEGACY_CONNECTABLE_SCANNABLE_UNDIRECTED) {                  //ADV_IND
        my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "LEG ADV_IND: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
    } else if (adv_evt_prop & ADV_EVT_PROP_LEGACY_CONNECTABLE_DIRECTED_LOW_DUTY) {              //ADV_DIRECT_IND(low duty cycle)
        my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "LEG ADV_DIRECT_IND(low duty cycle): Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
    } else if (adv_evt_prop & ADV_EVT_PROP_LEGACY_CONNECTABLE_DIRECTED_HIGH_DUTY) {             //ADV_DIRECT_IND(high duty cycle)
        my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "LEG ADV_DIRECT_IND(high duty cycle): Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
    } else if (adv_evt_prop & ADV_EVT_PROP_LEGACY_SCANNABLE_UNDIRECTED) {                       //ADV_SCAN_IND
        my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "LEG ADV_SCAN_IND: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
    } else if (adv_evt_prop & ADV_EVT_PROP_LEGACY_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED) {   //ADV_NONCONN_IND
        my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "LEG ADV_NONCONN_IND: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
    } else if (adv_evt_prop & ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED) { //ADV_EXT_IND + AUX_ADV_IND/AUX_CHAIN_IND
        my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "EXT ADV_NONCONN_IND: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
    } else if (adv_evt_prop & ADV_EVT_PROP_EXTENDED_CONNECTABLE_UNDIRECTED) {                   //ADV_EXT_IND + AUX_ADV_IND/AUX_CHAIN_IND
        my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "EXT ADV_IND: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
    } else if (adv_evt_prop & ADV_EVT_PROP_EXTENDED_SCANNABLE_UNDIRECTED) {                     //ADV_EXT_IND + AUX_ADV_IND/AUX_CHAIN_IND
        my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "EXT ADV_SCAN_IND: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
    } else if (adv_evt_prop & ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_DIRECTED) {   //ADV_EXT_IND + AUX_ADV_IND/AUX_CHAIN_IND
        my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "EXT ADV_NCNS_DIRECT_IND: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
    } else if (adv_evt_prop & ADV_EVT_PROP_EXTENDED_CONNECTABLE_DIRECTED) {                     //ADV_EXT_IND + AUX_ADV_IND/AUX_CHAIN_IND
        my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "EXT ADV_DIRECT_IND: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
    } else if (adv_evt_prop & ADV_EVT_PROP_EXTENDED_SCANNABLE_DIRECTED) {                       //ADV_EXT_IND + AUX_ADV_IND/AUX_CHAIN_IND
        my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "EXT ADV_SCAN_DIRECT_IND: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
    }
        #endif

    if (cur_pextadv->own_addr_rpa || cur_pextadv->directed_adv) {
        //record peer ADR type
        cur_pextadv->eAdvParaCmd_peerAdrType = peerAddrType;
        smemcpy(cur_pextadv->eAdvParaCmd_peerAddr, peerAddr, BLE_ADDR_LEN);


        #if (DBG_PRVC_EXTADV_EN)
        u8 temp_data[7];
        temp_data[0] = peerAddrType;
        smemcpy(temp_data + 1, peerAddr, 6);
        my_dump_str_data(DBG_PRVC_EXTADV_EN, "direct ADV or local RPA, peer address", temp_data, 7);
        #endif
    }


    cur_pextadv->pri_phy = pri_adv_phy;
    cur_pextadv->sec_phy = sec_adv_phy;

    cur_pextadv->pri_codedPhy_option = pri_adv_codePhy_option;
    cur_pextadv->sec_codedPhy_option = sec_adv_codePhy_option;

    /* rf_len 255B packet time and n_30us unit can be calculated once secondary_phy is set,
     * so we can do it here early to save running time when updateExtAdvSet*/
    if (cur_pextadv->sec_phy == BLE_PHY_1M) {
        cur_pextadv->rfLen_255_pkt_us = RFLEN_255_1MPHY_PKT_US;
        cur_pextadv->n_30us_chain_ind = RFLEN_255_1MPHY_N_30;
    } else if (cur_pextadv->sec_phy == BLE_PHY_2M) {
        cur_pextadv->rfLen_255_pkt_us = RFLEN_255_2MPHY_PKT_US;
        cur_pextadv->n_30us_chain_ind = RFLEN_255_2MPHY_N_30;
    } else {                                       //Coded PHY
        if (bltPHYs.dft_CI) {
            cur_pextadv->coding_ind = bltPHYs.dft_CI;
        } else {                                   //CODED_PHY_PREFER_NONE
            cur_pextadv->coding_ind = LE_CODED_S8; //fix dft S8
        }

        if (cur_pextadv->coding_ind == LE_CODED_S2) {
            cur_pextadv->rfLen_255_pkt_us = RFLEN_255_CODEDPHY_S2_PKT_US;
            cur_pextadv->n_30us_chain_ind = RFLEN_255_CODEDPHY_S2_N_30;
        } else {
            cur_pextadv->rfLen_255_pkt_us = RFLEN_255_CODEDPHY_S8_PKT_US;
            cur_pextadv->n_30us_chain_ind = RFLEN_255_CODEDPHY_S8_N_30;
        }
        //TODO: SiHui, when conFig S2/S8 with special API, recalculated for Coded PHY
    }


    cur_pextadv->adv_sid           = (adv_sid & 0x0F); //adv_sid: 4 bit
    cur_pextadv->scanReq_notify_en = scan_req_notify_en;


    if (adv_evt_prop == ADV_EVT_PROP_LEGACY_CONNECTABLE_DIRECTED_HIGH_DUTY) {
        cur_pextadv->advInt_min = cur_pextadv->advInt_max = ADV_INTERVAL_3_75MS;
    }

    //advInterval is 3 bytes(24 bit), max value is 0xFFFFFF(equal to 10485 S),
    //we now support 16bit(max 40.96 S), to save SRAM
    //BLE4.2 16bit (40.96S)
    cur_pextadv->advInt_min          = pri_advInter_min & 0xFFFF;
    cur_pextadv->advInt_max          = pri_advInter_max & 0xFFFF;
    cur_pextadv->advInt_maxAddRandom = cur_pextadv->advInt_max + 16; // 10mS/625us = 16
    cur_pextadv->advInt_diff         = cur_pextadv->advInt_max - cur_pextadv->advInt_min;

    /**************************************************************************
         adv type           pkt_adv.type                 SCAN_REQ   CONNECT_REQ

        ADV_IND             0 : LL_TYPE_ADV_IND             yes         yes
        ADV_DIRECT_IND      1 : LL_TYPE_ADV_DIRECT_IND      no          yes(*)
        ADV_NONCONN_IND     2 : LL_TYPE_ADV_NONCONN_IND     no          no
        ADV_SCAN_IND        6 : LL_TYPE_ADV_SCAN_IND        yes         no

        yes(*)      Only the correctly addressed initiator may respond
     *************************************************************************/


    cur_pextadv->scnReq_response = cur_pextadv->conReq_response = 0;

    if (cur_pextadv->cur_advMode == LL_EXTADV_MODE_CONN) {
        cur_pextadv->conReq_response = 1;
    } else if (cur_pextadv->cur_advMode == LL_EXTADV_MODE_SCAN) {
        cur_pextadv->scnReq_response = 1;
    } else if (cur_pextadv->cur_advMode == (LL_EXTADV_MODE_CONN | LL_EXTADV_MODE_SCAN)) { //ADV_IND
        cur_pextadv->scnReq_response = 1;
        cur_pextadv->conReq_response = 1;
    }


    cur_pextadv->param_update_flag = 1;


    return BLE_SUCCESS;
}
    #else
/* attention: here "adv_handle" is actually advSet_index, this API is only for application but can not used
 * in controller project. Tell users in handbook */
ble_sts_t blc_ll_setExtAdvParam(u8 adv_handle, advEvtProp_type_t adv_evt_prop, u32 pri_advInter_min, u32 pri_advInter_max, adv_chn_map_t pri_advChnMap, own_addr_type_t ownAddrType, u8 peerAddrType, u8 *peerAddr, adv_fp_type_t advFilterPolicy, tx_power_t adv_tx_pow, le_phy_type_t pri_adv_phy, u8 sec_adv_max_skip, le_phy_type_t sec_adv_phy, u8 adv_sid, u8 scan_req_notify_en)
{
    (void)sec_adv_max_skip; //unused, remove warning
    (void)adv_tx_pow;       //unused, remove warning
    my_dump_str_u8s(BLC_LL_LOG_EN, "@BLC_LL_Set_Ext_Adv_Param", adv_handle, adv_evt_prop, ownAddrType, advFilterPolicy);

    st_ext_adv_t *cur_pextadv;
    if (adv_handle == INVALID_ADVHD_FLAG) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    } else {
        /*If the Advertising_Handle does not identify an existing advertising set and the
        Controller is unable to support a new advertising set at present, the Controller
        shall return the error code Memory Capacity Exceeded (0x07). */
        cur_pextadv = blt_extadv_search_existed_and_allocate_new_adv_set(adv_handle);
        if (!cur_pextadv) {
            return HCI_ERR_MEM_CAP_EXCEEDED;
        }
    }


    /*If the Host issues this command when advertising is enabled for the specified
    advertising set, the Controller shall return the error code Command Disallowed
    (0x0C).*/
    if (cur_pextadv->extadv_en) {
        return HCI_ERR_CMD_DISALLOWED;
    }

    /*
     * extended ADV can not be direct high duty. so the interval should be in the range 20 ms to 10,485.759375 s
     */
    if (pri_advInter_min < ADV_INTERVAL_20MS || pri_advInter_max > ADV_INTERVAL_10_24S) { //20ms/0.625 = 32
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }


    if (ownAddrType == OWN_ADDRESS_RESOLVE_PRIVATE_PUBLIC || ownAddrType == OWN_ADDRESS_RESOLVE_PRIVATE_RANDOM) {
        #if (LL_FEATURE_ENABLE_PRIVACY)

        #else
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        #endif

        //do nothing
    } else if (ownAddrType == OWN_ADDRESS_PUBLIC || ownAddrType == OWN_ADDRESS_RANDOM) {
        //do nothing
    } else {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }


    /* HCI/DDI/BI-15-C.
     * CORE SPEC V5.3 page2452:If the Host issues this command when periodic advertising is enabled for the specified advertising set and
     * connectable, scannable, legacy, or anonymous advertising is specified, the Controller shall return the error code Invalid HCI Command Parameters (0x12)
     */
    if (cur_pextadv->prdadv_api_en) { //not use prd_adv_en
        if (adv_evt_prop & (ADVEVT_PROP_MASK_ANON_ADV | ADV_EVT_PROP_LEGACY_CONNECTABLE_SCANNABLE_UNDIRECTED)) {
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
    }

    if (!blt_ll_extAdvChkDataItvl(cur_pextadv, pri_advInter_max)) {
        return HCI_ERR_PACKET_TOO_LONG;
    }

    /* If the Host issues this command when periodic advertising is enabled for the
    specified advertising set and connectable, scannable, legacy, or anonymous
    advertising is specified, the Controller shall return the error code Invalid HCI
    Command Parameters (0x12). */

    /* If periodic advertising is enabled for the advertising set and the
    Secondary_Advertising_PHY parameter does not specify the PHY currently
    being used for the periodic advertising, the Controller shall return the error
    code Command Disallowed (0x0C). */


    /* If the advertising set already contains advertising data or scan response data,
    extended advertising is being used, and the length of the data is greater than
    the maximum that the Controller can transmit within the longest possible
    auxiliary advertising segment consistent with the parameters, the Controller
    shall return the error code Packet Too Long (0x45). */


    cur_pextadv->evt_props      = adv_evt_prop;
    cur_pextadv->evt_prop_bit04 = adv_evt_prop & 0x1F;

    cur_pextadv->useDecisionAdv = (adv_evt_prop & ADVEVT_PROP_MASK_USE_DECISION_PDU) ? 1 : 0;
    cur_pextadv->txPower_en_len = (adv_evt_prop & ADVEVT_PROP_MASK_INC_TX_PWR) ? 1 : 0; //1 and 0


    cur_pextadv->cur_advMode = (adv_evt_prop & ADVEVT_PROP_MASK_CONNECTABLE_SCANNABLE);

    /* using flash space to save Ram space and timing   */
    cur_pextadv->directed_adv = (adv_evt_prop & ADVEVT_PROP_MASK_DIRECTED);
    cur_pextadv->legacy_adv   = (adv_evt_prop & ADVEVT_PROP_MASK_LEGACY);

    cur_pextadv->adv_chn_mask   = pri_advChnMap;
    cur_pextadv->adv_chn_num    = 0;
    cur_pextadv->adv_chnIdx_1st = 0xFF;
    for (int i = 0; i < 3; i++) { //calculate how many channel used
        if (pri_advChnMap & BIT(i)) {
            cur_pextadv->adv_chn_num++;

            /* jump chn37/chn38 if ADV channel not set */
            if (0xFF == cur_pextadv->adv_chnIdx_1st) {
                cur_pextadv->adv_chnIdx_1st = i;

        #if (BLE_LLMIC_CONCURRENT_EN)
                cur_pextadv->llmic_advIdx = i;
        #endif
            }
        }
    }


    if (cur_pextadv->adv_chn_num == 0) {
        cur_pextadv->adv_chn_num = 1; //prevent (adv_chn_num - 1) == 0xff
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    //record owner ADR type
    cur_pextadv->own_addr_type = ownAddrType;
        #if (LL_FEATURE_ENABLE_PRIVACY)
    cur_pextadv->own_addr_rpa    = (ownAddrType & OWN_ADDRESS_TYPE_RPA_MASK);
    bltLegAdv.legadv_ownAddr_rpa = (ownAddrType & OWN_ADDRESS_TYPE_RPA_MASK);
        #endif
    if (ownAddrType & OWN_ADDRESS_TYPE_RANDOM_MASK) {
        cur_pextadv->extadv_mac_type = BLE_ADDR_RANDOM;

        /* merge from B85m 20240221, B85m fix this at 20240201
         * fix bug: legacy ADV in extended ADV mode use error random address */
        if (cur_pextadv->rand_adr_flg) {
            smemcpy(cur_pextadv->extadv_mac_addr, cur_pextadv->eAdv_rand_addr, BLE_ADDR_LEN);
        } else {
            smemcpy(cur_pextadv->extadv_mac_addr, bltMac.macAddress_random, BLE_ADDR_LEN);
        }
    } else {
        cur_pextadv->extadv_mac_type = BLE_ADDR_PUBLIC;
        smemcpy(cur_pextadv->extadv_mac_addr, bltMac.macAddress_public, BLE_ADDR_LEN);
    }

    cur_pextadv->adv_filterPolicy = (u8)advFilterPolicy;

        #if (DBG_PRVC_EXTADV_EN)
    if (adv_evt_prop & ADV_EVT_PROP_LEGACY_CONNECTABLE_SCANNABLE_UNDIRECTED) {                  //ADV_IND
        my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "LEG ADV_IND: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
    } else if (adv_evt_prop & ADV_EVT_PROP_LEGACY_CONNECTABLE_DIRECTED_LOW_DUTY) {              //ADV_DIRECT_IND(low duty cycle)
        my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "LEG ADV_DIRECT_IND(low duty cycle): Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
    } else if (adv_evt_prop & ADV_EVT_PROP_LEGACY_CONNECTABLE_DIRECTED_HIGH_DUTY) {             //ADV_DIRECT_IND(high duty cycle)
        my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "LEG ADV_DIRECT_IND(high duty cycle): Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
    } else if (adv_evt_prop & ADV_EVT_PROP_LEGACY_SCANNABLE_UNDIRECTED) {                       //ADV_SCAN_IND
        my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "LEG ADV_SCAN_IND: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
    } else if (adv_evt_prop & ADV_EVT_PROP_LEGACY_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED) {   //ADV_NONCONN_IND
        my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "LEG ADV_NONCONN_IND: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
    } else if (adv_evt_prop & ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED) { //ADV_EXT_IND + AUX_ADV_IND/AUX_CHAIN_IND
        my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "EXT ADV_NONCONN_IND: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
    } else if (adv_evt_prop & ADV_EVT_PROP_EXTENDED_CONNECTABLE_UNDIRECTED) {                   //ADV_EXT_IND + AUX_ADV_IND/AUX_CHAIN_IND
        my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "EXT ADV_IND: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
    } else if (adv_evt_prop & ADV_EVT_PROP_EXTENDED_SCANNABLE_UNDIRECTED) {                     //ADV_EXT_IND + AUX_ADV_IND/AUX_CHAIN_IND
        my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "EXT ADV_SCAN_IND: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
    } else if (adv_evt_prop & ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_DIRECTED) {   //ADV_EXT_IND + AUX_ADV_IND/AUX_CHAIN_IND
        my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "EXT ADV_NCNS_DIRECT_IND: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
    } else if (adv_evt_prop & ADV_EVT_PROP_EXTENDED_CONNECTABLE_DIRECTED) {                     //ADV_EXT_IND + AUX_ADV_IND/AUX_CHAIN_IND
        my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "EXT ADV_DIRECT_IND: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
    } else if (adv_evt_prop & ADV_EVT_PROP_EXTENDED_SCANNABLE_DIRECTED) {                       //ADV_EXT_IND + AUX_ADV_IND/AUX_CHAIN_IND
        my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "EXT ADV_SCAN_DIRECT_IND: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
    }
        #endif
        #if (LL_FEATURE_ENABLE_PRIVACY)
    if (cur_pextadv->own_addr_rpa || cur_pextadv->directed_adv)
        #else
    if (cur_pextadv->directed_adv)
        #endif
    {
        //record peer ADR type
        cur_pextadv->eAdvParaCmd_peerAdrType = peerAddrType;
        smemcpy(cur_pextadv->eAdvParaCmd_peerAddr, peerAddr, BLE_ADDR_LEN);


        #if (DBG_PRVC_EXTADV_EN)
        u8 temp_data[7];
        temp_data[0] = peerAddrType;
        smemcpy(temp_data + 1, peerAddr, 6);
        my_dump_str_data(DBG_PRVC_EXTADV_EN, "direct ADV or local RPA, peer address", temp_data, 7);
        #endif
    }


    cur_pextadv->pri_phy = pri_adv_phy;
    cur_pextadv->sec_phy = sec_adv_phy;

    /* rf_len 255B packet time and n_30us unit can be calculated once secondary_phy is set,
     * so we can do it here early to save running time when updateExtAdvSet*/
    if (cur_pextadv->sec_phy == BLE_PHY_1M) {
        cur_pextadv->rfLen_255_pkt_us = RFLEN_255_1MPHY_PKT_US;
        cur_pextadv->n_30us_chain_ind = RFLEN_255_1MPHY_N_30;
    } else if (cur_pextadv->sec_phy == BLE_PHY_2M) {
        cur_pextadv->rfLen_255_pkt_us = RFLEN_255_2MPHY_PKT_US;
        cur_pextadv->n_30us_chain_ind = RFLEN_255_2MPHY_N_30;
    } else {                                       //Coded PHY
        if (bltPHYs.dft_CI) {
            cur_pextadv->coding_ind = bltPHYs.dft_CI;
        } else {                                   //CODED_PHY_PREFER_NONE
            cur_pextadv->coding_ind = LE_CODED_S8; //fix dft S8
        }

        if (cur_pextadv->coding_ind == LE_CODED_S2) {
            cur_pextadv->rfLen_255_pkt_us = RFLEN_255_CODEDPHY_S2_PKT_US;
            cur_pextadv->n_30us_chain_ind = RFLEN_255_CODEDPHY_S2_N_30;
        } else {
            cur_pextadv->rfLen_255_pkt_us = RFLEN_255_CODEDPHY_S8_PKT_US;
            cur_pextadv->n_30us_chain_ind = RFLEN_255_CODEDPHY_S8_N_30;
        }
        //TODO: SiHui, when conFig S2/S8 with special API, recalculated for Coded PHY
    }


    cur_pextadv->adv_sid           = (adv_sid & 0x0F); //adv_sid: 4 bit
    cur_pextadv->scanReq_notify_en = scan_req_notify_en;


    if (adv_evt_prop == ADV_EVT_PROP_LEGACY_CONNECTABLE_DIRECTED_HIGH_DUTY) {
        cur_pextadv->advInt_min = cur_pextadv->advInt_max = ADV_INTERVAL_3_75MS;
    }


    if (pri_advInter_max < pri_advInter_min) {
        pri_advInter_max = pri_advInter_min;
    }

        //advInterval is 3 bytes(24 bit), max value is 0xFFFFFF(equal to 10485 S),
        //we now support 16bit(max 40.96 S), to save SRAM
        //BLE4.2 16bit (40.96S)
        #if (BLE_LLMIC_CONCURRENT_EN)
    if (pri_advInter_min < ADV_INTERVAL_30MS) {
        pri_advInter_min = ADV_INTERVAL_30MS;
    }
    if (pri_advInter_max < ADV_INTERVAL_30MS) {
        pri_advInter_max = ADV_INTERVAL_30MS;
    }
    cur_pextadv->advInt_min = ((pri_advInter_min & 0xFFFF) + 2) / 3;
    cur_pextadv->advInt_max = ((pri_advInter_max & 0xFFFF) + 2) / 3;
    if ((cur_pextadv->advInt_max - cur_pextadv->advInt_min) > 8) { //5mS
        cur_pextadv->advInt_maxAddRandom = cur_pextadv->advInt_max + ADVINTVL_RANDOM_MAX_5MS;
    } else {
        cur_pextadv->advInt_maxAddRandom = cur_pextadv->advInt_max + ADVINTVL_RANDOM_MAX_10MS;
    }

        #else
    cur_pextadv->advInt_min          = pri_advInter_min & 0xFFFF;
    cur_pextadv->advInt_max          = pri_advInter_max & 0xFFFF;
    cur_pextadv->advInt_maxAddRandom = cur_pextadv->advInt_max + 16; // 10mS/625us = 16
        #endif
    cur_pextadv->advInt_diff = cur_pextadv->advInt_max - cur_pextadv->advInt_min;

    /**************************************************************************
         adv type           pkt_adv.type                 SCAN_REQ   CONNECT_REQ

        ADV_IND             0 : LL_TYPE_ADV_IND             yes         yes
        ADV_DIRECT_IND      1 : LL_TYPE_ADV_DIRECT_IND      no          yes(*)
        ADV_NONCONN_IND     2 : LL_TYPE_ADV_NONCONN_IND     no          no
        ADV_SCAN_IND        6 : LL_TYPE_ADV_SCAN_IND        yes         no

        yes(*)      Only the correctly addressed initiator may respond
     *************************************************************************/


    cur_pextadv->scnReq_response = cur_pextadv->conReq_response = 0;

    if (cur_pextadv->cur_advMode == LL_EXTADV_MODE_CONN) {
        cur_pextadv->conReq_response = 1;
    } else if (cur_pextadv->cur_advMode == LL_EXTADV_MODE_SCAN) {
        cur_pextadv->scnReq_response = 1;
    } else if (cur_pextadv->cur_advMode == (LL_EXTADV_MODE_CONN | LL_EXTADV_MODE_SCAN)) { //ADV_IND
        cur_pextadv->scnReq_response = 1;
        cur_pextadv->conReq_response = 1;
    }


    cur_pextadv->param_update_flag = 1;


    return BLE_SUCCESS;
}
    #endif


ble_sts_t blc_hci_le_setExtAdvParam(hci_le_setExtAdvParam_cmdParam_t *pCmdParam, u8 *pTxPower)
{
    (void)pTxPower; //unused, remove warning

    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Set_Ext_Adv_Param", pCmdParam, sizeof(hci_le_setExtAdvParam_cmdParam_t));

    //HCI/GEV/BV-02-C [Disallow Mixing Legacy and Extended Advertising Commands]
    if (IS_LEGACY_ADV_VALID) {
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_EXTENDED_ADV_VALID;


    //primary ADV can only use 1M_PHY or Coded_PHY
    if (pCmdParam->pri_adv_phy != BLE_PHY_1M && pCmdParam->pri_adv_phy != BLE_PHY_CODED) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }


    if (pCmdParam->advEvt_props & ADVEVT_PROP_MASK_LEGACY) { //Legacy ADV

        if (pCmdParam->advEvt_props == ADV_EVT_PROP_LEGACY_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED) {
    #if BQB_5P0_TEST_ENABLE                                  //do not know the reason, to pass HCI/DDI/BI-01-C
            if (pri_advIntervalMin < 0x20 || pri_advIntervalMax < 0x20) {
                return HCI_ERR_INVALID_HCI_CMD_PARAMS;
            } else if (pri_advIntervalMin >= 0x20 && pri_advIntervalMax >= 0x20) {
                return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
            }
    #endif
        }

        if (pCmdParam->pri_adv_phy != BLE_PHY_1M) { //legacy ADV can only use 1M PHY
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }

    } else {
        //ADV_EXT_IND can only use 1M PHY & Coded PHY
        //      if(pCmdParam->pri_adv_phy == BLE_PHY_2M){
        //          return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        //      }
    }


    //extended ADV can not be 1. direct high duty
    //                        2. connectable and scannable both
    if (((pCmdParam->advEvt_props & ADVEVT_PROP_MASK_LEGACY_HD_DIRECTED) == ADVEVT_PROP_MASK_HD_DIRECTED) ||
        ((pCmdParam->advEvt_props & ADVEVT_PROP_MASK_LEGACY_CONNECTABLE_SCANNABLE) == ADVEVT_PROP_MASK_CONNECTABLE_SCANNABLE)) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

#if(LL_FEATURE_ENABLE_DECISION_BASED_ADVERTISING_FILTER)
    if((pCmdParam->advEvt_props&ADVEVT_PROP_MASK_USE_DECISION_PDU)){

        if(pCmdParam->advEvt_props & ADVEVT_PROP_MASK_DIRECTED){
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
    }

    if((pCmdParam->advEvt_props&ADVEVT_PROP_MASK_DECISION_PDU_INC_ADVA) || (pCmdParam->advEvt_props&ADVEVT_PROP_MASK_DECISION_PDU_INC_ADI)){
        if(!(pCmdParam->advEvt_props&ADVEVT_PROP_MASK_USE_DECISION_PDU)){
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
    }
#endif


    //attention: pCmdParam->adv_handle -> advSet_idx
    return blc_ll_setExtAdvParam(pCmdParam->adv_handle, pCmdParam->advEvt_props, pCmdParam->pri_advIntMin[0] | pCmdParam->pri_advIntMin[1] << 8, pCmdParam->pri_advIntMax[0] | pCmdParam->pri_advIntMax[1] << 8, pCmdParam->pri_advChnMap, pCmdParam->ownAddrType, pCmdParam->peerAddrType, pCmdParam->peerAddr, pCmdParam->advFilterPolicy, pCmdParam->adv_tx_pow, pCmdParam->pri_adv_phy, pCmdParam->sec_adv_max_skip, pCmdParam->sec_adv_phy, pCmdParam->adv_sid, pCmdParam->scan_req_notify_en);
}

u8 advData_backup[8];

ble_sts_t blt_ll_setSegmentExtendedAdvData(u8 adv_handle, data_oper_t operation, data_fragment_t fragment_prefer, u8 advData_len, u8 *advdata)
{
    (void)fragment_prefer; //unused, remove warning

    //HCI/GEV/BV-02-C [Disallow Mixing Legacy and Extended Advertising Commands]
    if (IS_LEGACY_ADV_VALID) {
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_EXTENDED_ADV_VALID;

    st_ext_adv_t *cur_pextadv;
    if (adv_handle == INVALID_ADVHD_FLAG) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    } else {
        cur_pextadv = blt_extadv_search_existed_adv_set(adv_handle);
        /*If the advertising set corresponding to the Advertising_Handle parameter does
        not exist, then the Controller shall return the error code Unknown Advertising
        Identifier (0x42). */
        if (!cur_pextadv) {
            return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
        }
    }


    if (operation == DATA_OPER_UNCHANGED) { //Unchanged data, just update the ADV_DID(not change any AdvData)
        cur_pextadv->adv_did = ((clock_time() >> 4) & 0xFFF) | 0x001;
        return BLE_SUCCESS;                 //not do anything else  (by sihui)
    }


    /* If the advertising set uses legacy advertising PDUs that support advertising data and either Operation is not 0x03 or the
    Advertising_Data_Length parameter exceeds 31 octets, the Controller shall return the error code Invalid HCI Command Parameters (0x12).*/
    if (cur_pextadv->evt_props & ADVEVT_PROP_MASK_LEGACY) {
        if (advData_len > 31 || operation != DATA_OPER_COMPLETE) {
            //my_dump_str_u8s(0, "ERROR 111", advData_len, operation, cur_pextadv->evt_props, 0);
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
    }

    /*If the advertising set specifies a type that does not support advertising data, the
    Controller shall return the error code Invalid HCI Parameters (0x12). */
    //LL/DDI/ADV/BV-68-C
    if ((cur_pextadv->evt_props & ADVEVT_PROP_MASK_LEGACY_CONNECTABLE_SCANNABLE) == ADVEVT_PROP_MASK_SCANNABLE) { //scannable event: no adv_data
        /* LL/DDI/ADV/BV-76-C, test Spec use set data command with length 0, but expect status 0x00
         * so here we use length none zero to return error. */
        if (advData_len) {
            cur_pextadv->curLen_advData = 0; //clear, in case that other event change to Extended Scannable event
            //my_dump_str_u8s(0, "ERROR 222", cur_pextadv->evt_props, 0, 0, 0);
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
    }


    //EBQ no has case to test. so close temporary.
    #if 0
    if(operation == DATA_OPER_UNCHANGED){
        if(!cur_pextadv->extadv_en){
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
        else if(cur_pextadv->evt_props & ADVEVT_PROP_MASK_LEGACY){
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
        else if(cur_pextadv->aux_1st_pkt_dataLen==0 || advData_len != 0){
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
    }
    #endif


    /*If advertising is currently enabled for the specified advertising set and
    Operation does not have the value 0x03 or 0x04, the Controller shall return the
    error code Command Disallowed (0x0C).*/
    /*If Operation is not 0x03 or 0x04 and Advertising_Data_Length is zero, the
    Controller shall return the error code Invalid HCI Command Parameters (0x12). */
    if ((u8)operation < DATA_OPER_COMPLETE) { //<
        if (cur_pextadv->extadv_en) {
            return HCI_ERR_CMD_DISALLOWED;
        }
        if (advData_len == 0) {
            //my_dump_str_u8s(0, "ERROR 333", operation, advData_len, 0, 0);
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
    }
    //  else{
    //      if(advData_len == 0){
    //          return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    //      }
    //  }


    /*If Operation indicates the start of new data (values 0x01 or 0x03), then any
    existing partial or complete scan response data shall be discarded. If the
    Scan_Response_Data_Length parameter is zero, then Operation shall be
    0x03; this indicates that any existing partial or complete data shall be deleted
    and no new data provided.*/
    int newLen_adv;
    if (advData_len == 0 && operation == DATA_OPER_COMPLETE) { //delete existing data
        cur_pextadv->curLen_advData = 0;
        newLen_adv                  = 0;
    } else if (operation == DATA_OPER_FIRST || operation == DATA_OPER_COMPLETE) {
        cur_pextadv->curLen_advData = 0;
        newLen_adv                  = advData_len;
    } else {
        newLen_adv = cur_pextadv->curLen_advData + advData_len;
    }

    /*If the combined length of the data
    exceeds the capacity of the advertising set identified by the
    Advertising_Handle parameter (see Section 7.8.57 LE Read Maximum
    Advertising Data Length Command) or the amount of memory currently
    available, all the data shall be discarded and the Controller shall return the
    error code Memory Capacity Exceeded (0x07).*/
    if (newLen_adv > cur_pextadv->maxLen_advData) {
        return HCI_ERR_MEM_CAP_EXCEEDED;
    }


    switch (operation) {
    case DATA_OPER_INTER:
    {
        cur_pextadv->unfinished_advData = 1;
    } break;


    case DATA_OPER_FIRST:
    {
        cur_pextadv->unfinished_advData = 1;
    } break;


    case DATA_OPER_LAST:
    {
        cur_pextadv->unfinished_advData = 0;
    } break;


    case DATA_OPER_COMPLETE:
    {
        cur_pextadv->unfinished_advData = 0;
    } break;


    case DATA_OPER_UNCHANGED:
    {
    } break;

    default:
    {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    } break;
    }


    #if 0
    //note that: Extended Scannable Undirected and Directed event can not set advData, so it can not set ADV_DID
    //but it need ADV_DID(ADI filed valid for EXT_ADV_IND in BLE Spec), we need set ADV_DID in API "blc_hci_le_setExtScanRspData"
    if(operation != DATA_OPER_UNCHANGED){ // DATA_OPER_UNCHANGED need update ADV_DID
        cur_pextadv->adv_did = ((clock_time()>>4) & 0xFFF)  | 0x001;   //quick random value for adv_did
    }
    #endif


    //copy data
    if (newLen_adv && advData_len) {
        smemcpy((cur_pextadv->dat_extAdv + cur_pextadv->curLen_advData), advdata, advData_len);
    }
    cur_pextadv->curLen_advData = newLen_adv;


    /*The LE_Set_Extended_Advertising_Data command is used to set the data
    used in advertising PDUs that have a data field. This command may be issued
    at any time after an advertising set identified by the Advertising_Handle
    parameter has been created using the LE Set Extended Advertising
    Parameters Command (see Section 7.8.53), regardless of whether advertising
    in that set is enabled or disabled.
    If advertising is currently enabled for the specified advertising set, the
    Controller shall use the new data in subsequent extended advertising events
    for this advertising set. If an extended advertising event is in progress when
    this command is issued, the Controller may use the old or new data for that
    event.
    If advertising is currently disabled for the specified advertising set, the data
    shall be kept by the Controller and used once advertising is enabled for that
    set */
    if (!cur_pextadv->unfinished_advData) { //data completed


        //last data, need generate 1 new ADV_DID now(even if new advData are all same as previous, so ADV_DID will change every time when application layer call API "blt_ll_setSegmentExtendedAdvData",)
        //note that: Extended Scannable Undirected and Directed event can not set advData, so it can not set ADV_DID
        //but it need ADV_DID information(ADI filed valid for EXT_ADV_IND in BLE Spec), we need set ADV_DID in API "blc_hci_le_setExtScanRspData"
        cur_pextadv->adv_did = ((clock_time() >> 4) & 0xFFF) | 0x001; //quick random value for adv_did


        //cur_pextadv->param_update_flag = 1;  //will update RF packet before sending_adv

        //core5.3 page2767 key words:"the Advertising DID shall be updated"
        //before update DID in blc_ll_setExtAdvEnable.but some case(LL/DDI/AVD/BV-27-C) only enable once,but data update will be more than one times.
        //HCI/DDI/BI-62-C check "Packet Too Long" and need to update "sSlotDuration_extadv + sSlotDuration_auxadv". only set data and set param, not extAdv enable.
        u32 r = irq_disable();
        blt_updateExtAdvSet(cur_pextadv); //interrupt protect. //13.4us CCLK_48M_HCLK_48M_PCLK_24M
        irq_restore(r);


    #if (LL_FEATURE_ENABLE_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE && EXTADV_DATA_CHANGE_MANUAL_DATA_BUFFER)
        /* here standard design: we should hold previous old data, when new data come, compare them,
             * if data are different, trigger RPA change. but this will cost extra
             * */
        //do not need consider ADV_DIRECT_IND, because test_case no have that
        if (cur_pextadv->extAdv_chngReason & REFRESH_RPA_ADVDATA_CHANGE) {
            if (cur_pextadv->backupCurLen_advData != cur_pextadv->curLen_advData ||
                smemcmp(cur_pextadv->backupDat_extAdv, cur_pextadv->dat_extAdv, cur_pextadv->curLen_advData)) {
                my_dump_str_data(DBG_PRVC_EXTADV_EN, "CHANGE extadv data, RPA refresh", 0, 0);
                u32               r1  = irq_disable();
                ll_resolv_list_t *pRL = cur_pextadv->pRslvlst_extAdv;
                irq_restore(r1);
                if (pRL) {
                    blt_ll_resolvRefreshRpa(pRL);
                }
            }
        }


        smemcpy(cur_pextadv->backupDat_extAdv, cur_pextadv->dat_extAdv, cur_pextadv->curLen_advData);
        cur_pextadv->backupCurLen_advData = cur_pextadv->curLen_advData;
    #endif
    }

    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_setExtendedAdvData(hci_le_setExtAdvData_cmdParam_t *pCmdParam)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Set_Ext_Adv_Data", pCmdParam, pCmdParam->length + 4);
    return blt_ll_setSegmentExtendedAdvData(pCmdParam->adv_handle, pCmdParam->operation, pCmdParam->fragmentPrefer, pCmdParam->length, pCmdParam->data);
}

/* TODO Sihui: when set ADV data and ScanRsp data API is called in main_loop(or HCI controller project) and ADV is enabled,
 * there might be conflict in IRQ, we need judge which case will case IRQ data and timing error.
 * for legacy ADV, ADV data and ScanRsp data changing has effect on primary channel
 * for extended ADV, ADV data and ScanRsp data length changing between zero and none zero on primary channel,
 *                   and has effect on secondary channel.
 * When situation happens above, add a flag in API, ADV in IRQ pending on see the flag, mainloop find a chance to
 *    update ADV parameters and timing.
 */


    #if (LL_FEATURE_ENABLE_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE)
ble_sts_t blc_ll_setExtendedAdvDataRelatedAddressChange(hci_le_setDataAddrChange_cmdParams_t *pCmdParam)
{
    /*
    If extended advertising commands (see Section 3.1.1) are being used and the
    advertising set corresponding to the Advertising_Handle parameter does not
    exist, or if no command specified in Table 3.2 has been used, then the
    Controller shall return the error code Unknown Advertising Identifier (0x42).
    */

    //pCmdParam->adv_handle;
    //pCmdParam->reasons;
    st_ext_adv_t *cur_pextadv;
    u8            adv_handle = pCmdParam->adv_handle;
    if (adv_handle == INVALID_ADVHD_FLAG) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    } else {
        cur_pextadv = blt_extadv_search_existed_adv_set(adv_handle);
        /*If the advertising set corresponding to the Advertising_Handle parameter does
        not exist, then the Controller shall return the error code Unknown Advertising
        Identifier (0x42). */
        if (!cur_pextadv) {
            return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
        }
    }


    cur_pextadv->extAdv_chngReason = pCmdParam->reasons;


    return BLE_SUCCESS;
}
    #endif


//TODO: check advData_len for Connectable & Scannable, cause we consider only 1 AUX_ADV_IND for these two type
ble_sts_t blc_ll_setExtAdvData(u8 adv_handle, int advData_len, const u8 *advData)
{
    my_dump_str_data(BLC_LL_LOG_EN, "@BLC_LL_Set_Ext_Adv_Data", advData, advData_len > 256 ? 256 : advData_len);

    //HCI/GEV/BV-02-C [Disallow Mixing Legacy and Extended Advertising Commands]
    if (IS_LEGACY_ADV_VALID) {
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_EXTENDED_ADV_VALID;

    st_ext_adv_t *cur_pextadv;
    if (adv_handle == INVALID_ADVHD_FLAG) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    } else {
        cur_pextadv = blt_extadv_search_existed_adv_set(adv_handle);
        /*If the advertising set corresponding to the Advertising_Handle parameter does
        not exist, then the Controller shall return the error code Unknown Advertising
        Identifier (0x42). */
        if (!cur_pextadv) {
            return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
        }
    }


    /* If the advertising set uses legacy advertising PDUs
    Advertising_Data_Length parameter exceeds 31 octets, the Controller shall return the error code Invalid HCI Command Parameters (0x12).*/
    if (cur_pextadv->evt_props & ADVEVT_PROP_MASK_LEGACY) {
        if (advData_len > 31) {
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
    }


    if (advData_len > cur_pextadv->maxLen_advData) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }


    smemcpy((cur_pextadv->dat_extAdv), advData, advData_len);
    cur_pextadv->curLen_advData = advData_len;

    cur_pextadv->param_update_flag = 1;


    return BLE_SUCCESS;
}

#if(LL_FEATURE_ENABLE_DECISION_BASED_ADVERTISING_FILTER)
ble_sts_t blc_ll_setDecisionData(u8 adv_handle, u8 decisionType, int decisionDataLen, const u8 *decisionData)
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
        cur_pextadv = blt_extadv_search_existed_adv_set(adv_handle);
        /*If the advertising set corresponding to the Advertising_Handle parameter does
        not exist, then the Controller shall return the error code Unknown Advertising
        Identifier (0x42). */
        if(!cur_pextadv){
            return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
        }
    }

    if(decisionDataLen > 8){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    //Resolvable Tag is 6 bytes
    if( (decisionType & DECISION_TYPE_RESOLVABLE_TAG_SUBFIELD_PRESENT) && (decisionDataLen<6) ){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }


    ////////////////////////////////////////////////
    //AUX_PTR must be exist.only if there are aux_adv_ind, ADI must be exist for telink sdk.
    u8 decisionDataOft = EXTHD_LEN_3_AUX_PTR;

    if(cur_pextadv->evt_props&ADVEVT_PROP_MASK_DECISION_PDU_INC_ADVA){
        decisionDataOft += EXTHD_LEN_6_ADVA;
    }

    if(cur_pextadv->evt_props&ADVEVT_PROP_MASK_DECISION_PDU_INC_ADI){
        decisionDataOft += EXTHD_LEN_2_ADI;
    }

    if(cur_pextadv->txPower_en_len){
        decisionDataOft += EXTHD_LEN_1_TX_POWER;
    }
    /////////////////////////////////////////////////


    u32 r = irq_disable();

    cur_pextadv->setDecisionDataFlag = 1;
    cur_pextadv->setDecisionDataLen = decisionDataLen;
    cur_pextadv->decisionTypeFlag = (decisionType & DECISION_TYPE_RESOLVABLE_TAG_SUBFIELD_PRESENT);

    smemcpy(cur_pextadv->primary_adv.data+decisionDataOft, decisionData, decisionDataLen);

    blt_updateExtAdvSet(cur_pextadv); //interrupt protect. //13.4us CCLK_48M_HCLK_48M_PCLK_24M

    irq_restore(r);

    return BLE_SUCCESS;
}
#endif

ble_sts_t blt_ll_setSegmentExtendedScanResponseData(u8 adv_handle, data_oper_t operation, data_fragment_t fragment_prefer, u8 scanRspData_len, u8 *scanRspData)
{
    (void)fragment_prefer; //unused, remove warning

    //HCI/GEV/BV-02-C [Disallow Mixing Legacy and Extended Advertising Commands]
    if (IS_LEGACY_ADV_VALID) {
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_EXTENDED_ADV_VALID;


    st_ext_adv_t *cur_pextadv;
    if (adv_handle == INVALID_ADVHD_FLAG) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    } else {
        cur_pextadv = blt_extadv_search_existed_adv_set(adv_handle);
        /*If the advertising set corresponding to the Advertising_Handle parameter does
        not exist, then the Controller shall return the error code Unknown Advertising
        Identifier (0x42). */
        if (!cur_pextadv) {
            return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
        }
    }


    /* If the advertising set uses scannable legacy advertising PDUs and either
    Operation is not 0x03 or the Scan_Response_Data_Length parameter
    exceeds 31 octets, the Controller shall return the error code Invalid HCI
    Command Parameters (0x12).*/
    if ((cur_pextadv->evt_props & ADVEVT_PROP_MASK_LEGACY_SCANNABLE) == ADVEVT_PROP_MASK_LEGACY_SCANNABLE) {
        if (scanRspData_len > 31 || operation != DATA_OPER_COMPLETE) {
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
    }


    /*If advertising is currently enabled for the specified advertising set and
    Operation does not have the value 0x03, the Controller shall return the error
    code Command Disallowed (0x0C). */
    if (cur_pextadv->extadv_en && (u8)operation != DATA_OPER_COMPLETE) {
        return HCI_ERR_CMD_DISALLOWED;
    }


    /*If the Advertising_Data_Length parameter is zero and Operation is 0x03, any
    existing partial or complete data shall be deleted (with no new data provided). */
    int newLen_scanRsp;
    if (scanRspData_len == 0 && operation == DATA_OPER_COMPLETE) {
        //    Delete data  //
        cur_pextadv->curLen_scanRsp     = 0;
        newLen_scanRsp                  = 0;
        cur_pextadv->unfinished_scanRsp = 0;
        return BLE_SUCCESS;
    }
    /*If the advertising set is non-scannable and the Host uses this command other
    than to delete existing data, the Controller shall return the error code Invalid
    HCI Parameters (0x12).*/
    else if (!(cur_pextadv->evt_props & ADVEVT_PROP_MASK_SCANNABLE)) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }
    /*If Operation indicates the start of new data (values 0x01 or 0x03), then any
    existing partial or complete advertising data shall be discarded.*/
    else if (operation == DATA_OPER_FIRST || operation == DATA_OPER_COMPLETE) {
        cur_pextadv->curLen_scanRsp = 0;
        newLen_scanRsp              = scanRspData_len; // discarded any existing partial or complete advertising data
    } else {
        newLen_scanRsp = cur_pextadv->curLen_scanRsp + scanRspData_len;
    }

    /*If the combined length of the data exceeds the capacity of the advertising set identified by the Advertising_Handle parameter
     * (see Section 7.8.57 LE Read Maximum Advertising Data Length Command) or the amount of memory currently available,
     * all the data shall be discarded and the Controller shall return the error code Memory Capacity Exceeded (0x07).
     *maxLen_advData need to equal maxLen_scanRsp. because The HCI_LE_Read_Maximum_Advertising_Data_Length command is used to
     *read the maximum length of data supported by the Controller for use as advertisement data or scan response data
     *in an advertising event or as periodic advertisement data.*/
    if (newLen_scanRsp > cur_pextadv->maxLen_advData) { // maxLen_scanRsp
        return HCI_ERR_MEM_CAP_EXCEEDED;
    }


    switch (operation) {
    case DATA_OPER_INTER:
    {
        cur_pextadv->unfinished_scanRsp = 1;
    } break;


    case DATA_OPER_FIRST:
    {
        cur_pextadv->unfinished_scanRsp = 1;
    } break;


    case DATA_OPER_LAST:
    {
        cur_pextadv->unfinished_scanRsp = 0;
    } break;


    case DATA_OPER_COMPLETE:
    {
        cur_pextadv->unfinished_scanRsp = 0;
    } break;


    case DATA_OPER_UNCHANGED:
    {
    } break;

    default:
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }


    //copy data
    if (newLen_scanRsp && scanRspData_len) {
        smemcpy((cur_pextadv->dat_scanRsp + cur_pextadv->curLen_scanRsp), scanRspData, scanRspData_len);
    }

    cur_pextadv->curLen_scanRsp = newLen_scanRsp;


    //note that: Extended Scannable Undirected and Directed event can not set advData, so it can not set ADV_DID
    //but it need ADV_DID(ADI filed valid for EXT_ADV_IND in BLE Spec), so we need ADV DID here
    if (!cur_pextadv->unfinished_scanRsp && (cur_pextadv->evt_props & ADVEVT_PROP_MASK_LEGACY_CONNECTABLE_SCANNABLE) == ADVEVT_PROP_MASK_SCANNABLE) {
        cur_pextadv->adv_did = ((clock_time() >> 4) & 0xFFF) | 0x001; //quick random value for adv_did
    }

    #if (LL_FEATURE_ENABLE_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE && EXTADV_DATA_CHANGE_MANUAL_DATA_BUFFER)
    if (!cur_pextadv->unfinished_scanRsp) {
        /* here standard design: we should hold previous old data, when new data come, compare them,
             * if data are different, trigger RPA change. but this will cost extra
             * */
        //do not need consider ADV_DIRECT_IND, because test_case no have that
        if (cur_pextadv->extAdv_chngReason & REFRESH_RPA_SCANRSPDATA_CHANGE) {
            if (cur_pextadv->backupCurLen_scanRsp != cur_pextadv->curLen_scanRsp ||
                smemcmp(cur_pextadv->backupDat_scanRsp, cur_pextadv->dat_scanRsp, cur_pextadv->curLen_scanRsp)) {
                my_dump_str_data(DBG_PRVC_EXTADV_EN, "CHANGE extadv scanrsp data, RPA refresh", 0, 0);
                u32               r   = irq_disable();
                ll_resolv_list_t *pRL = cur_pextadv->pRslvlst_extAdv;
                irq_restore(r);
                if (pRL) {
                    blt_ll_resolvRefreshRpa(pRL);
                }
            }
        }

        smemcpy(cur_pextadv->backupDat_scanRsp, cur_pextadv->dat_scanRsp, cur_pextadv->curLen_scanRsp);
        cur_pextadv->backupCurLen_scanRsp = cur_pextadv->curLen_scanRsp;
    }
    #endif


    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_setExtendedScanResponseData(hci_le_setExtScanRspData_cmdParam_t *pCmdParam)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Set_Ext_ScanRsp_Data", pCmdParam, pCmdParam->length + 4);
    return blt_ll_setSegmentExtendedScanResponseData(pCmdParam->adv_handle, pCmdParam->operation, pCmdParam->fragmentPrefer, pCmdParam->length, pCmdParam->data);
}

/* attention: here "adv_handle" is actually advSet_index, this API is only for application but can not used
 * in controller project. Tell users in handbook */
ble_sts_t blc_ll_setExtScanRspData(u8 adv_handle, int scanRspData_len, const u8 *scanRspData)
{
    my_dump_str_data(BLC_LL_LOG_EN, "@BLC_LL_Set_Ext_ScanRsp", scanRspData, scanRspData_len > 256 ? 256 : scanRspData_len);

    //HCI/GEV/BV-02-C [Disallow Mixing Legacy and Extended Advertising Commands]
    if (IS_LEGACY_ADV_VALID) {
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_EXTENDED_ADV_VALID;

    st_ext_adv_t *cur_pextadv;
    if (adv_handle == INVALID_ADVHD_FLAG) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    } else {
        cur_pextadv = blt_extadv_search_existed_adv_set(adv_handle);
        /*If the advertising set corresponding to the Advertising_Handle parameter does
        not exist, then the Controller shall return the error code Unknown Advertising
        Identifier (0x42). */
        if (!cur_pextadv) {
            return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
        }
    }

    if (scanRspData_len > cur_pextadv->maxLen_scanRsp) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }


    smemcpy((cur_pextadv->dat_scanRsp), scanRspData, scanRspData_len);
    cur_pextadv->curLen_scanRsp = scanRspData_len;

    cur_pextadv->param_update_flag = 1;


    return BLE_SUCCESS;
}

/* attention: here "adv_handle" is actually advSet_index, this API is only for application but can not used
 * in controller project. Tell users in handbook */
ble_sts_t blc_ll_setAdvRandomAddr(u8 adv_handle, u8 *rand_addr)
{
    st_ext_adv_t *cur_pextadv;
    if (adv_handle == INVALID_ADVHD_FLAG) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    } else {
        cur_pextadv = blt_extadv_search_existed_adv_set(adv_handle);
        /*If the advertising set corresponding to the Advertising_Handle parameter does
        not exist, then the Controller shall return the error code Unknown Advertising
        Identifier (0x42). */
        if (!cur_pextadv) {
            return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
        }
    }


    /* if the Host issues this command while the advertising set identified by the Advertising_Handle parameter
     * is using connectable advertising and is enabled, the Controller shall return the error code Command
      Disallowed (0x0C).*/
    if (cur_pextadv->extadv_en && (cur_pextadv->evt_props & ADVEVT_PROP_MASK_CONNECTABLE)) {
        return HCI_ERR_CMD_DISALLOWED;
    }


    smemcpy((cur_pextadv->eAdv_rand_addr), rand_addr, BLE_ADDR_LEN);

    /* merge from B85m 20240221, B85m fix this at 20240201
     * fix bug: legacy ADV in extended ADV mode use error random address */
    if (cur_pextadv->extadv_mac_type == BLE_ADDR_RANDOM) {
        smemcpy((cur_pextadv->extadv_mac_addr), rand_addr, BLE_ADDR_LEN);
    }


    cur_pextadv->rand_adr_flg = 1; //gaoqiu fixed bug

    cur_pextadv->param_update_flag = 1;

    return BLE_SUCCESS;
}

/* attention: here "adv_handle" is actually advSet_index, this API is only for application but can not used
 * in controller project. Tell users in handbook */
ble_sts_t blc_ll_setExtAdvEnable(adv_en_t enable, u8 adv_handle, u16 duration, u8 max_extAdvEvt)
{
    my_dump_str_u8s(BLC_LL_LOG_EN, "@BLC_LL_Set_Ext_Adv_En", enable, adv_handle, duration, max_extAdvEvt);

    st_ext_adv_t *cur_pextadv;
    if (adv_handle == INVALID_ADVHD_FLAG) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    } else {
        cur_pextadv = blt_extadv_search_existed_adv_set(adv_handle);
        /*If the advertising set corresponding to the Advertising_Handle parameter does
        not exist, then the Controller shall return the error code Unknown Advertising
        Identifier (0x42). */
        if (!cur_pextadv) {
            return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
        }
    }


    /*If the LE_Set_Extended_Advertising_Enable command is sent again for an advertising set while that set is enabled,
     * the timer used for the duration and the number of events counter are reset and any change to the random address
    shall take effect.*/

    /* Duration[i] and Max_Extended_Advertising_Events[i] are ignored when Enable is set to 0x00. */


    /* attention: one situation not handle: when EXT_ADV is enable, ADV data changed */
    u8 last_ext_en = cur_pextadv->extadv_en;
    if (enable && !last_ext_en) {
    #if (SL01_ext_adv_endis)
        log_task_begin_irq(SL_STACK_EXT_PRD_BASE_TIMING_EN, SL01_ext_adv_endis);
    #endif

        if (cur_pextadv->prdadv_api_en && !cur_pextadv->prdadv_task_en) {
            if (cur_pextadv->prdadv_update_flag) {
                if (ll_prd_adv_mlp_task_cb) {
                    ll_prd_adv_mlp_task_cb(FLAG_SCHEDULE_PRDADV_PARAM_UPDATE | cur_pextadv->mapping_prdadv_idx, NULL); //blt_prdadv_updatePram
                }
                cur_pextadv->prdadv_update_flag = 0;
                cur_pextadv->syncinfo_changed   = 0;
            }

            if (ll_prd_adv_mlp_task_cb) {                                                                        //blt_prd_adv_mainloop_task
                ll_prd_adv_mlp_task_cb(FLAG_SCHEDULE_PRDADV_TASK_BEGIN | cur_pextadv->mapping_prdadv_idx, NULL); //blt_prdadv_task_begin
            }
            cur_pextadv->prdadv_task_en = 1;

            cur_pextadv->param_update_flag = 1;

    //////////
    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
            st_prd_adv_t *cur_pPerdadv = blt_prdadv_search_existed_and_allocate_new_periodic_adv(adv_handle);
            cur_pextadv->acad_used     = cur_pPerdadv->num_subevents ? PERD_ACAD_PAwR_ENA : 0;
    #else
            cur_pextadv->acad_used = 0;
    #endif
            ///////////////


            u32 r = irq_disable();
            //need to notice:here |=SYNC_INFO_NEED may cause the first some aux adv has SyncInfor section but value are zero.
            //later need to optimize that.
            cur_pextadv->syncinfo_used |= SYNC_INFO_NEED;                              //if periodic adv has been enabled,here must set syncinfo_used to calculate aux adv duration.
            blt_sche_enableTask(TSKMSK_PERD_ADV_0 << cur_pextadv->mapping_prdadv_idx); //BY_QW
            irq_restore(r);
        }

        if (cur_pextadv->param_update_flag) {
            if (cur_pextadv->syncinfo_used) {
                cur_pextadv->syncinfo_changed = 1;
            }

            blt_updateExtAdvSet(cur_pextadv);

            if (cur_pextadv->syncinfo_used) {
                cur_pextadv->syncinfo_changed = 2;

    #if (ONLY_FOR_EBQ_TEST_LATER_REMOVE)
                bigExtAuxPda_conflictCtrl.syncInfor_changeCtrl = SYNCINFOR_VAILD_PENDING;
    #endif
            }

            cur_pextadv->param_update_flag = 0;
        }

        /* If the advertising set already contains advertising data or scan response data,
        extended advertising is being used, and the length of the data is greater than
        the maximum that the Controller can transmit within the longest possible
        auxiliary advertising segment consistent with the parameters, the Controller
        shall return the error code Packet Too Long (0x45). */
        if (!blt_ll_extAdvChkDataItvl(cur_pextadv, cur_pextadv->advInt_maxAddRandom)) {
            my_dump_str_data(DBG_EXTADV_LOGIC, "HCI_ERR_PACKET_TOO_LONG", 0, 0);
            return HCI_ERR_PACKET_TOO_LONG;
        }

        if (1) {
            u32 r = irq_disable();
            blmsParam.state_chng |= STATE_CHANGE_EXT_ADV;
            cur_pextadv->extadv_en = 1;
            blmsParam.ext_adv_en |= BIT(cur_pextadv->extadv_index);
            blt_sche_enableTask(TSKMSK_EXT_ADV_0 << cur_pextadv->extadv_index);

            //TODO: change to bSlot 625uS unit
            if (cur_pextadv->evt_prop_bit04 == ADV_EVT_PROP_LEGACY_CONNECTABLE_DIRECTED_HIGH_DUTY) {
                cur_pextadv->adv_duration_tick = 1280 * SYSTEM_TIMER_TICK_1MS; //high duty, 1.28 S
            } else {
                cur_pextadv->adv_duration_tick = duration * 10 * SYSTEM_TIMER_TICK_1MS;
            }

            cur_pextadv->max_ext_adv_evt = max_extAdvEvt;
            cur_pextadv->run_ext_adv_evt = 0;

    #if (EXT_ADV_DELAY_CTRL_EN)
            //if((blmsParam.state_chng & STATE_CHANGE_EXT_ADV) && bltSche.task_mask)
            if (bltSche.task_mask) //there is other task. if there are only adv task, no need to process here.
            {
                //only consider primary scan. secondary scan can not.
                if (blms_state == BLMS_STATE_NONE || (blms_state & BLMS_STATE_PRICHN_SCAN_S)) {
                    u32 cur_tick = clock_time();
                    if (tick1_exceed_tick2(systimer_get_irq_capture(), cur_tick + 8 * SYSTEM_TIMER_TICK_1MS)) {
                        systick_irq_trigger = SYS_IRQ_TRIG_SCHE_INSERT;
                        systimer_set_irq_capture(cur_tick + 4 * SYSTEM_TIMER_TICK_1MS);
                    }
                }
            }
    #endif


            irq_restore(r);
        }
    #if OS_SUP_EN
        if (blt_os_giveSem_cb) {
            blt_os_giveSem_cb();
        }
    #endif
    } else if (!enable && last_ext_en) {
        u32 r = irq_disable();
        blmsParam.state_chng |= STATE_CHANGE_EXT_ADV;
        cur_pextadv->extadv_en = 0;
        blmsParam.ext_adv_en &= ~BIT(cur_pextadv->extadv_index);
        blt_sche_disableTask(TSKMSK_EXT_ADV_0 << cur_pextadv->extadv_index);
        blt_remove_future_task(TSKOFT_AUX_ADV + cur_pextadv->extadv_index); // 1 -> 0, remove future task immediately

    #if (SL01_ext_adv_endis)
        log_task_end_irq(SL_STACK_EXT_PRD_BASE_TIMING_EN, SL01_ext_adv_endis);
    #endif

        //when disable extended adv,timing will rebuild. If not remove aux mask, it may occur that not find any task until 8s.
        //if when disable ext adv,still have aux adv task not run, it will occur above problem. if all aux task have been done,it is ok.
        blt_sche_removeTaskMask(TSKMSK_AUX_ADV_0 << cur_pextadv->extadv_index); //blt_sche_removeTaskMask should follow blt_remove_future_task
        cur_pextadv->aux_adv_pending = 0;                                       //pending need to clear, or the new aux adv will not run.


    #if (ONLY_FOR_EBQ_TEST_LATER_REMOVE)
        bigExtAuxPda_conflictCtrl.syncInfor_changeCtrl = SYNCINFOR_NOT_CHANGE;
    #endif

        irq_restore(r);
    #if OS_SUP_EN
        if (blt_os_giveSem_cb) {
            blt_os_giveSem_cb();
        }
    #endif
    }


    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_setExtAdvEnable(hci_le_setExtAdvEn_cmdParam_t *pCmdParam)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Set_Ext_Adv_Enable", pCmdParam, sizeof(hci_le_setExtAdvEn_cmdParam_t));

    //HCI/GEV/BV-02-C [Disallow Mixing Legacy and Extended Advertising Commands]
    if (IS_LEGACY_ADV_VALID) {
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_EXTENDED_ADV_VALID;

    /* core_5.3
    If the same advertising set is identified by more than one entry in the
    Advertising_Handle[i] arrayed parameter, then the Controller shall return the
    error code Invalid HCI Command Parameters (0x12).

    If the advertising set corresponding to the Advertising_Handle[i] parameter
    does not exist, then the Controller shall return the error code Unknown
    Advertising Identifier (0x42).                                                              Done !!!

    The remainder of this section only applies if Enable is set to 0x01.
    If Num_Sets is set to 0x00, the Controller shall return the error code Invalid HCI
    Command Parameters (0x12).                                                                  Done !!!

    If the advertising set contains partial advertising data or partial scan response
    data, the Controller shall return the error code Command Disallowed (0x0C).                 Done !!!

    If the advertising set uses scannable extended advertising PDUs and no scan
    response data is currently provided, the Controller shall return the error code
    Command Disallowed (0x0C).

    If the advertising set uses connectable extended advertising PDUs and the
    advertising data in the advertising set will not fit in the AUX_ADV_IND PDU,
    the Controller shall return the error code Invalid HCI Command Parameters
    (0x12).

    Note: The maximum amount of data that will fit in the PDU depends on which
    options are selected and on the maximum length of PDU that the Controller is
    able to transmit.
    If extended advertising is being used and the length of any advertising data or
    of any scan response data is greater than the maximum that the Controller can
    transmit within the longest possible auxiliary advertising segment consistent
    with the chosen advertising interval, the Controller shall return the error code
    Packet Too Long (0x45). If advertising on the LE Coded PHY, the S=8 coding
    shall be assumed.
    If the advertising set's Own_Address_Type parameter is set to 0x00 and the
    device does not have a public address, the Controller should return an error
    code which should be Invalid HCI Command Parameters (0x12).                                 Ignore, controller always have public address

    If the advertising set's Own_Address_Type parameter is set to 0x01 and the
    random address for the advertising set has not been initialized using the
    HCI_LE_Set_Advertising_Set_Random_Address command, the Controller
    shall return the error code Invalid HCI Command Parameters (0x12).

    If the advertising set's Own_Address_Type parameter is set to 0x02, the
    Controller's resolving list did not contain a matching entry, and the device does
    not have a public address, the Controller should return an error code which
    should be Invalid HCI Command Parameters (0x12).                                            Ignore, controller always have public address

    If the advertising set's Own_Address_Type parameter is set to 0x03, the
    controller's resolving list did not contain a matching entry, and the random
    address for the advertising set has not been initialized using the
    HCI_LE_Set_Advertising_Set_Random_Address command, the Controller
    shall return the error code Invalid HCI Command Parameters (0x12).
    */


    /*When Enable is set to 0x01*/
    /*If Num_Sets is set to 0x00, the Controller shall return the error code Invalid HCI
    Command Parameters (0x12).*/
    if (pCmdParam->enable && !pCmdParam->num_sets) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    st_ext_adv_t   *cur_pextadv = NULL;
    extAdvEn_Cfg_t *pextadv_enCfg;

    for (int i = 0; i < pCmdParam->num_sets; i++) {
        pextadv_enCfg = (extAdvEn_Cfg_t *)&pCmdParam->cisCfg[i];

        if (pextadv_enCfg->adv_handle == INVALID_ADVHD_FLAG) {
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        } else {
            cur_pextadv = blt_extadv_search_existed_adv_set(pextadv_enCfg->adv_handle);
            /* advertising set corresponding to the Advertising_Handle[i] parameter does not exist */
            if (cur_pextadv == NULL) {
                return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
            }
        }


        /*When Enable is set to 0x01*/
        /*If the advertising data or scan response data in the advertising set is not
        complete, the Controller shall return the error code Command Disallowed(0x0C).*/
        if (cur_pextadv->unfinished_advData || cur_pextadv->unfinished_scanRsp) {
            return HCI_ERR_CMD_DISALLOWED;
        }

        //1. HCI/GEV/BV-04-C[Extended Advertising Commands Without Scan Response Data]. Host set the curLen_scanRsp = 0, need to return error.
        //2. But according to ADVEVT_PROP_MASK_LEGACY_SCANNABLE to judge. Because host may not send LE_Set_Extended_Scan_Response_Data_Command to set curLen_scanRsp,
        //but LE_Set_Extended_Advertising_Parameters's Advertising_Event_Properties may include Scannable. Refer to LL/DDI/ADV/BV-22-C.
        //extended adv need to judge scanRsp length and legacy adv not need to judge scanRsp length.
        if (((cur_pextadv->evt_props & ADVEVT_PROP_MASK_LEGACY_SCANNABLE) == ADVEVT_PROP_MASK_SCANNABLE) && cur_pextadv->curLen_scanRsp == 0) {
            return HCI_ERR_CMD_DISALLOWED;
        }

        /*If the advertising set's Own_Address_Type parameter is set to 0x01 and the
        random address for the advertising set has not been initialized, the Controller
        shall return the error code Invalid HCI Command Parameters (0x12).*/
        if (cur_pextadv->own_addr_type == OWN_ADDRESS_RANDOM && !cur_pextadv->rand_adr_flg) {
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }

        /*If the advertising set's Own_Address_Type parameter is set to 0x03, the controller's
         * resolving list did not contain a matching entry, and the random address for the advertising set
         * has not been initialized using the HCI_LE_Set_Advertising_Set_Random_Address command,
         * the Controller shall return the error code Invalid HCI Command Parameters (0x12).*/
        if (cur_pextadv->own_addr_type == OWN_ADDRESS_RESOLVE_PRIVATE_RANDOM && !cur_pextadv->rand_adr_flg) {
            if (!blt_ll_resolve_rpa(1, cur_pextadv->extadv_mac_addr, 0)) {
                my_dump_str_data(0, "kma", 0, 0);
                return HCI_ERR_INVALID_HCI_CMD_PARAMS;
            }
        }

        /*If the same advertising set is identified by more than one entry in the
        Advertising_Handle[i] arrayed parameter, then the Controller shall return the
        error code Invalid HCI Command Parameters (0x12).*/
        //TODO: but not very important, if BQB test has this case, then add code here
    }


    for (int i = 0; i < pCmdParam->num_sets; i++) {
        pextadv_enCfg = (extAdvEn_Cfg_t *)&pCmdParam->cisCfg[i];

        blc_ll_setExtAdvEnable(pCmdParam->enable, pextadv_enCfg->adv_handle, pextadv_enCfg->duration, pextadv_enCfg->max_ext_adv_evts);
    }


    #if OS_SUP_EN
    if (blt_os_giveSem_cb) {
        blt_os_giveSem_cb();
    }
    #endif
    return BLE_SUCCESS;
}

_attribute_noinline_ void blt_extadv_clear_adv_set_param(st_ext_adv_t *cur_pextadv)
{
    cur_pextadv->mapping_prdadv_idx = INVALID_ADVHD_FLAG;

    cur_pextadv->extadv_change_flag = 0;
    cur_pextadv->prdadv_update_flag = 0;
    cur_pextadv->syncinfo_used      = 0;

#if(LL_FEATURE_ENABLE_DECISION_BASED_ADVERTISING_FILTER)
    cur_pextadv->setDecisionDataLen = 0;
    cur_pextadv->setDecisionDataFlag= 0;
    cur_pextadv->useDecisionAdv = 0;
#endif

    cur_pextadv->acad_used = 0;

    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
    smemset(&cur_pextadv->pawr_timing_info, 0, sizeof(pawr_acad_t));
    #endif

    cur_pextadv->rand_adr_flg   = 0;
    cur_pextadv->directed_adv   = 0;
    cur_pextadv->legacy_adv     = 0;
    cur_pextadv->txPower_en_len = 0;
    cur_pextadv->directed_adv   = 0;

    cur_pextadv->directed_adv = 0;

    cur_pextadv->pri_phy = cur_pextadv->sec_phy = BLE_PHY_1M;

    cur_pextadv->max_ext_adv_evt = 0;

    cur_pextadv->adv_filterPolicy  = 0;
    cur_pextadv->param_update_flag = 0;

    cur_pextadv->with_aux_adv_ind = 0;

    cur_pextadv->adv_did = 0;

    cur_pextadv->unfinished_advData = 0;
    cur_pextadv->unfinished_scanRsp = 0;
    cur_pextadv->curLen_advData     = 0; //The data shall be discarded when the advertising set is removed
    cur_pextadv->curLen_scanRsp     = 0;

    #if (LL_FEATURE_ENABLE_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE)
    cur_pextadv->extAdv_chngReason = 0;

        #if (EXTADV_DATA_CHANGE_MANUAL_DATA_BUFFER)
    cur_pextadv->backupCurLen_advData = 0;
    cur_pextadv->backupCurLen_scanRsp = 0;
        #endif
    #endif

    /* If advertising on the LE Coded PHY, the S=8 coding shall be assumed. */
    if (bltPHYs.dft_CI) {
        cur_pextadv->coding_ind = bltPHYs.dft_CI;
    } else {
        cur_pextadv->coding_ind = LE_CODED_S8;
    }


    smemset(cur_pextadv->eAdv_rand_addr, 0, BLE_ADDR_LEN);
    smemset(cur_pextadv->eAdvParaCmd_peerAddr, 0, BLE_ADDR_LEN);

    //Very important!!!, if not re-initialize this value, when after hci_reset, Ellisys, mobile phones
    //and other main devices will not recognize the sent extAdv[when Ext Adv Event Properties type: Extended, Scannable, Undirected].
    smemcpy(cur_pextadv->public_addr, bltMac.macAddress_public, BLE_ADDR_LEN);
}

ble_sts_t blc_ll_removeAdvSet(u8 adv_handle)
{
    //HCI/GEV/BV-02-C [Disallow Mixing Legacy and Extended Advertising Commands]
    if (IS_LEGACY_ADV_VALID) {
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_EXTENDED_ADV_VALID;


    /* If the advertising set corresponding to the Advertising_Handle parameter does
    not exist, then the Controller shall return the error code Unknown Advertising
    Identifier (0x42). If advertising or periodic advertising on the advertising set is
    enabled, then the Controller shall return the error code Command Disallowed
    (0x0C). */
    st_ext_adv_t *cur_pextadv;
    if (adv_handle == INVALID_ADVHD_FLAG) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    } else {
        cur_pextadv = blt_extadv_search_existed_adv_set(adv_handle);
        /*If the advertising set corresponding to the Advertising_Handle parameter does
        not exist, then the Controller shall return the error code Unknown Advertising
        Identifier (0x42). */
        if (!cur_pextadv) {
            return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
        }
    }


    if (cur_pextadv->extadv_en || cur_pextadv->prdadv_api_en) {
        return HCI_ERR_CMD_DISALLOWED;
    }


    cur_pextadv->adv_handle = INVALID_ADVHD_FLAG;

    return HCI_ERR_UNKNOWN_ADV_IDENTIFIER; //no adv_handle match
}

ble_sts_t blc_ll_clearAdvSets(void)
{
    //HCI/GEV/BV-02-C [Disallow Mixing Legacy and Extended Advertising Commands]
    if (IS_LEGACY_ADV_VALID) {
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_EXTENDED_ADV_VALID;


    /* If advertising or periodic advertising is enabled on any advertising set, then the
    Controller shall return the error code Command Disallowed (0x0C). */
    u8            status = BLE_SUCCESS;
    st_ext_adv_t *cur_pextadv;
    for (int i = 0; i < bltExtA.maxNum_advSets; i++) { //clear using ADV handle

        cur_pextadv = (st_ext_adv_t *)(global_pextadv + i);
        //      blt_extadv_clear_adv_set_param(cur_pextadv);

        if (cur_pextadv->adv_handle != INVALID_ADVHD_FLAG) {
            if (cur_pextadv->extadv_en || cur_pextadv->prdadv_api_en) {
                status = HCI_ERR_CMD_DISALLOWED;
                break;
            } else {
                cur_pextadv->adv_handle = INVALID_ADVHD_FLAG;
            }
        }
    }


    return status;
}

void blt_reset_ext_adv(void)
{
    //Reset AuxPkt sch task table
    smemset(&bltFutTask.task_tbl[0], 0, sizeof(future_task_e));
    bltFutTask.number = 0;
    //clear ext adv en
    blmsParam.ext_adv_en = 0;

    st_ext_adv_t *cur_pextadv;
    for (int i = 0; i < bltExtA.maxNum_advSets; i++) {
        cur_pextadv                  = (st_ext_adv_t *)(global_pextadv + i);
        cur_pextadv->extadv_en       = 0;
        cur_pextadv->prdadv_api_en   = 0;
        cur_pextadv->aux_adv_pending = 0; //must clear!!! 1: AUX ADV task has allocated but not executed
        cur_pextadv->prdadv_task_en  = 0;
        cur_pextadv->syncinfo_used   = 0;

        cur_pextadv->curLen_advData = 0;
        cur_pextadv->curLen_scanRsp = 0;

        cur_pextadv->peerScanReq_revFlag = 0;
        cur_pextadv->aux_1st_pkt_dataLen = 0;

        cur_pextadv->sSlotDuration_extadv = 0; //check data too long
        cur_pextadv->sSlotDuration_auxadv = 0; //check data too long

    #if(LL_FEATURE_ENABLE_DECISION_BASED_ADVERTISING_FILTER)
        cur_pextadv->setDecisionDataLen = 0;
        cur_pextadv->setDecisionDataFlag = 0;
        cur_pextadv->useDecisionAdv = 0;
    #endif

    #if (NEED_MORE_TEST_TO_CONFIRM)            //later will run more test to confirm. now two IAL cases are OK.
        cur_pextadv->bSlot_mark_extadv = trng_rand() & 0x0f;
    #endif

    #if (LL_FEATURE_ENABLE_PRIVACY)
        cur_pextadv->extAdv_advA_useRpa = 0;
        cur_pextadv->extAdv_initAUseRpa = 0;
        cur_pextadv->pRslvlst_extAdv    = NULL;
    #endif
    #if (LL_FEATURE_ENABLE_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE)
        cur_pextadv->extAdv_chngReason = 0;

        #if (EXTADV_DATA_CHANGE_MANUAL_DATA_BUFFER)
        cur_pextadv->backupCurLen_advData = 0;
        cur_pextadv->backupCurLen_scanRsp = 0;
        #endif
    #endif
    }

    #if (ONLY_FOR_EBQ_TEST_LATER_REMOVE)
    bigExtAuxPda_conflictCtrl.syncInfor_changeCtrl = ACAD_NOT_CHANGE;
    bigExtAuxPda_conflictCtrl.auxAdv_sendNum       = 0;
    #endif

    blc_ll_clearAdvSets();
}


#endif //end of LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING
