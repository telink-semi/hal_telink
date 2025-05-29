/********************************************************************************************************
 * @file    ext_scan.c
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


#if (LL_FEATURE_ENABLE_LE_EXTENDED_SCAN)

    #if OS_SUP_EN
        #include "stack/ble/os_sup/os_sup.h"
        #include "stack/ble/os_sup/os_sup_stack.h"
    #endif

#define DECISION_INSTRUCTION_SUPPORT_MAX_NUM    8

_attribute_data_retention_ _attribute_aligned_(4) ll_rx_pkt_callback_t ll_secchn_initPkt_cb = NULL;

_attribute_data_retention_ _attribute_aligned_(4) u8 scan_sec_chn_rx_fifo[SCAN_SECCHN_RXFIFO_SIZE * SCAN_SECCHN_RXFIFO_NUM]; //extended scan secondary channel RX FIFO


_attribute_data_retention_ _attribute_aligned_(4) ll_extscn_t bltExtScn;


_attribute_data_retention_ _attribute_aligned_(4) st_secchn_scn_t secChnScn_tbl[TSKNUM_SECCHN_SCAN];
_attribute_data_retention_ _attribute_aligned_(4) st_secchn_scn_t *blt_pSecChnScn = NULL;
_attribute_data_retention_ _attribute_aligned_(4) auxscn_common_para_t auxScnCmnParam;


_attribute_data_retention_ _attribute_aligned_(4) aux_scn_fut_task_t AuxScnFutTask; //future task

#if(LL_FEATURE_ENABLE_DECISION_BASED_ADVERTISING_FILTER)
_attribute_ble_data_retention_ dec_ins_t        decison_instruct_test[DECISION_INSTRUCTION_SUPPORT_MAX_NUM]; //at least support 8 tests
_attribute_ble_data_retention_ decision_test_t  gDecisionInstructTest;
#endif

//_attribute_aligned_(4)    scan_dev_t  scnDevTbl[NUM_OF_EXT_SCAN_DEVICE];

/* chain data buffer, only only hold chain data, also hold first aux_data */
_attribute_data_retention_ _attribute_aligned_(4) u8 extadv_pda_rpt_hold_data_buf[TSKNUM_SECCHN_SCAN][EXTADV_PDA_RPT_DATA_HOLD_MAX_LEN]; //228*number

void blc_ll_initExtendedScanning_module(void)
{
    #if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_extscn_t)), ext_scan);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ext_pkt_info_t)), ext_scan);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(st_prichn_scn_t)), ext_scan);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(st_secchn_scn_t)), ext_scan);
    #endif

#if (!ESL_RAM_OPTIMIZATION)
    blc_ll_init2MPhyCodedPhy_feature(); //need 2M/Coded PHY feature
#endif                                  //(!ESL_RAM_OPTIMIZATION)

    blt_ll_initScanningCommon();

    ll_ext_scan_irq_task_cb = blt_ext_scan_interrupt_task;
    ll_ext_scan_mlp_task_cb = blt_ext_scan_mainloop_task;

    ll_secchn_scan_task_cb = blt_ll_procAuxiliaryScanTask;

    ll_irq_scan_rx_sec_chn_cb = irq_scan_rx_secondary_channel;
#if (!ESL_RAM_OPTIMIZATION)
    ll_extadv_pkt_cb = blt_ext_adv_rx_process;
#endif //(!ESL_RAM_OPTIMIZATION)

    blmsParam.extScanModule_en = 1;

    //Secondary Channel Scan RX buffer initialize
    scan_secRxFifo.p    = (u8 *)scan_sec_chn_rx_fifo;
    scan_secRxFifo.num  = SCAN_SECCHN_RXFIFO_NUM;
    scan_secRxFifo.mask = SCAN_SECCHN_RXFIFO_MASK;
    scan_secRxFifo.rptr = scan_secRxFifo.wptr = 0;
    bltExtScn.scan_rx_sec_chn_dma_buff        = (u32)(scan_secRxFifo.p + (scan_secRxFifo.wptr & SCAN_SECCHN_RXFIFO_MASK) * SCAN_SECCHN_RXFIFO_SIZE);
    bltExtScn.scan_rx_sec_chn_dma_size        = (SCAN_SECCHN_RXFIFO_SIZE >> 4);


    st_secchn_scn_t *cur_pauxscn;
    for (int i = 0; i < TSKNUM_SECCHN_SCAN; i++) {
        cur_pauxscn                         = (st_secchn_scn_t *)&secChnScn_tbl[i];
        cur_pauxscn->scnIndex               = i;
        cur_pauxscn->auxScnTsk.scheTask_oft = TSKOFT_SECCHN_SCAN + i;
        cur_pauxscn->auxScnTsk.scheTask_idx = i;
        cur_pauxscn->auxScnTsk.scheTask_flg = TSKFLG_SECCHN_SCAN;

        blt_ll_setSchedulerTaskPriority(TSKOFT_SECCHN_SCAN + i, TASK_PRIORITY_AUX_SCAN_DFT);
    }


    blt_set_ext_scan_default();
}

void blc_ll_setExtendedScanSecondaryChannelRxDataProcessEnable(u8 enable)
{
    u32 r = irq_disable();
    if (enable) {
#if (!ESL_RAM_OPTIMIZATION)
        ll_extadv_pkt_cb = blt_ext_adv_rx_process;
#endif //(!ESL_RAM_OPTIMIZATION)
    } else {
        ll_extadv_pkt_cb = NULL;
    }
    irq_restore(r);
}

void blt_set_ext_scan_default(void)
{
    bltScn.extScan_duration = 0;
    bltScn.extScan_period   = 0;
    /* if user forget to set Scan parameters, default value: */

    for (int i = 0; i < TSKNUM_SECCHN_SCAN; i++) {
        st_secchn_scn_t *pSecChnScn = &secChnScn_tbl[i];

        pSecChnScn->pdaSync_flag          = 0;
        pSecChnScn->scan_rx_flag          = 0;
        pSecChnScn->advrpt_hold_dat_len   = 0;
        pSecChnScn->advrpt_holdDataOffset = 0;
        pSecChnScn->priAdvType            = 0xff;
        blt_set_auxscan_enable(pSecChnScn, 0);
    }
}

_attribute_noinline_ void blt_reset_ext_scan(void)
{
    blmsParam.scanInitEn_union.ext_scan_en = 0;

    blt_set_scan_default();
    blt_set_ext_scan_default();

#if(LL_FEATURE_ENABLE_DECISION_BASED_ADVERTISING_FILTER)
    blt_set_decision_scan_default();
#endif
}

ble_sts_t blc_ll_setExtScanParam(own_addr_type_t ownAddrType, scan_fp_type_t scan_fp, scan_phy_t scan_phys, scan_type_t scanType_0, scan_inter_t scanInter_0, scan_wind_t scanWindow_0, scan_type_t scanType_1, scan_inter_t scanInter_1, scan_wind_t scanWindow_1)
{
    tlkapi_send_string_u8s(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_CMD), "[LL][CMD] Set_Ext_Scan_Param", ownAddrType, scan_fp, scan_phys, scanType_0);

    SET_EXTENDED_SCAN_VALID;

    /*If the Host specifies a PHY that is not supported by the Controller, including a bit that is reserved for future use,
      it should return the error code Unsupported Feature or Parameter Value(0x11).
      BIT(0) indicate 1M phy, BIT(2)indicate coded phy, other value is reserved for future use*/
    if (scan_phys & (~SCAN_PHY_1M_CODED)) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    /* check the related parameter. refer to core specification related chapter.
     * LE Set Extended Scan Parameters command, scan interval and scan window range: 0 to 0xffff. unit is 0.625ms*/
    if (scan_phys & SCAN_PHY_1M) {
        if (scanInter_0 < 4 || scanWindow_0 < 4) {
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
    }
    if (scan_phys & SCAN_PHY_CODED) {
        if (scanInter_1 < 4 || scanWindow_1 < 4) {
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
    }

    #if (DBG_PRVC_EXTSCAN_EN)
    if (scanType_0 == SCAN_TYPE_PASSIVE) {
        my_dump_str_u8s(0, "passive scan: Own_addr_type, filter policy", ownAddrType, scan_fp, 0, 0);
    } else if (scanType_0 == SCAN_TYPE_ACTIVE) {
        my_dump_str_u8s(0, "active scan: Own_addr_type, filter policy", ownAddrType, scan_fp, 0, 0);
    }
    #endif

    bltScn.scan_filterPolicy       = (u8)scan_fp;
    bltScn.scan_fp_wl              = (scan_fp & SCAN_FP_WHITELIST_MASK) ? 1 : 0;
    bltScn.scan_fp_targetA_rpaPass = (scan_fp & SCAN_FP_DIRECT_RPA_PASS_MASK) ? 1 : 0;


    ///////////////////////////
    bltScn.scan_ownAddr_random = (ownAddrType & OWN_ADDRESS_TYPE_RANDOM_MASK);
    bltScn.scan_ownAddr_rpa    = (ownAddrType & OWN_ADDRESS_TYPE_RPA_MASK);
    if (bltScn.scan_ownAddr_random) {
        bltScn.scan_mac_type = BLE_ADDR_RANDOM;
        smemcpy(bltScn.scan_mac_addr, bltMac.macAddress_random, BLE_ADDR_LEN);
    } else {
        bltScn.scan_mac_type = BLE_ADDR_PUBLIC;
        smemcpy(bltScn.scan_mac_addr, bltMac.macAddress_public, BLE_ADDR_LEN);
    }

    pkt_scanReq.txAddr = bltScn.scan_mac_type;
    smemcpy(pkt_scanReq.scanA, bltScn.scan_mac_addr, BLE_ADDR_LEN);
    ///////////////////////////


    bltScn.scanPhy_msk = scan_phys;

    if (scan_phys & SCAN_PHY_1M) {
        bltScn.extScanType[0] = (u8)scanType_0;

        if ((int)scanWindow_0 > (int)scanInter_0) {
            scanWindow_0 = (scan_wind_t)scanInter_0;
        }

    #if (ONLY_FOR_EBQ_TEST_LATER_REMOVE)
        bltScn.scanPercent[0] = 128; //LL_DDI_SCN_BV_25_C
    #else
        bltScn.scanPercent[0] = (scanWindow_0 << 7) / scanInter_0; // scan_window*128;
    #endif

        bltScn.scanInter[0]     = scanInter_0;
        bltScn.scanInte_tick[0] = scanInter_0 * SYSTEM_TIMER_TICK_625US - 1000 * SYSTEM_TIMER_TICK_1US;
    }
    if (scan_phys & SCAN_PHY_CODED) {
        bltScn.extScanType[2] = (u8)scanType_1;

        if ((int)scanWindow_1 > (int)scanInter_1) {
            scanWindow_1 = (scan_wind_t)scanInter_1;
        }

    #if (ONLY_FOR_EBQ_TEST_LATER_REMOVE)
        bltScn.scanPercent[2] = 128; //LL_DDI_SCN_BV_25_C
    #else
        bltScn.scanPercent[2] = (scanWindow_1 << 7) / scanInter_1; // scan_window*128;
    #endif

        bltScn.scanInter[2]     = scanInter_1;
        bltScn.scanInte_tick[2] = scanInter_1 * SYSTEM_TIMER_TICK_625US - 1000 * SYSTEM_TIMER_TICK_1US;
    }

    blmsParam.hci_cmd_mask |= SET_EXTSCAN_PARAM_CMD_MASK;


    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_setExtScanParam(hci_le_setExtScanParam_cmdParam_t *pCmdParam)
{
    int phy_cnt = 1;
    (void)phy_cnt; //remove compiler warning

    if (!blmsParam.extScanModule_en) {
        return HCI_ERR_UNKNOWN_HCI_CMD;
    }

    if (pCmdParam->scan_PHYs == SCAN_PHY_1M_CODED) {
        phy_cnt = 2;
    }
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Set_Ext_Scan_Param", pCmdParam, 3 + 5 * phy_cnt);


    /* HCI/GEV/BV-03-C [Disallow Mixing Legacy and Extended Scanning Commands] */
    if (IS_LEGACY_SCAN_VALID) {
        return HCI_ERR_CMD_DISALLOWED;
    } else {
        SET_EXTENDED_SCAN_VALID;
    }


    /*If the Host issues this command when scanning is enabled in the Controller,
    the Controller shall return the error code Command Disallowed (0x0C). */
    if (blmsParam.scanInitEn_union.ext_scan_en) {
        return HCI_ERR_CMD_DISALLOWED;
    }


    if (pCmdParam->scan_PHYs == SCAN_PHY_1M) {
        my_dump_str_data(DBG_EXTSCAN_LOGIC, "1M PHY", 0, 0);
        return blc_ll_setExtScanParam(pCmdParam->ownAddress_type, pCmdParam->scan_filter_policy, SCAN_PHY_1M, pCmdParam->scanCfg[0].scan_type, pCmdParam->scanCfg[0].scan_interval, pCmdParam->scanCfg[0].scan_window, 0, 0, 0);
    } else if (pCmdParam->scan_PHYs == SCAN_PHY_CODED) {
        my_dump_str_data(DBG_EXTSCAN_LOGIC, "Coded PHY", 0, 0);
        return blc_ll_setExtScanParam(pCmdParam->ownAddress_type, pCmdParam->scan_filter_policy, SCAN_PHY_CODED, 0, 0, 0, pCmdParam->scanCfg[0].scan_type, pCmdParam->scanCfg[0].scan_interval, pCmdParam->scanCfg[0].scan_window);
    } else if (pCmdParam->scan_PHYs == SCAN_PHY_1M_CODED) {
        my_dump_str_data(DBG_EXTSCAN_LOGIC, "1M Coded PHY", 0, 0);
        return blc_ll_setExtScanParam(pCmdParam->ownAddress_type, pCmdParam->scan_filter_policy, SCAN_PHY_1M_CODED, pCmdParam->scanCfg[0].scan_type, pCmdParam->scanCfg[0].scan_interval, pCmdParam->scanCfg[0].scan_window, pCmdParam->scanCfg[1].scan_type, pCmdParam->scanCfg[1].scan_interval, pCmdParam->scanCfg[1].scan_window);
    } else {
        /*If the Host specifies a PHY that
        is not supported by the Controller, including a bit that is reserved for future use,
        it should return the error code Unsupported Feature or Parameter Value
        (0x11). */
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }


    return BLE_SUCCESS;
}

ble_sts_t blc_ll_setExtScanEnable(scan_en_t extScan_en, dupe_fltr_en_t filter_duplicate, scan_durn_t duration, scan_period_t period)
{
    tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_CMD), "[LL][CMD] Set_Ext_Scan_En", &extScan_en, 1);

    SET_EXTENDED_SCAN_VALID;

    /* HCI/DDI/BV-06-C [Default Extended Scan Enable Command] */
    if (!(blmsParam.hci_cmd_mask & SET_EXTSCAN_PARAM_CMD_MASK)) {
        return HCI_ERR_CMD_DISALLOWED;
    }


    //if ownAddrType = OWN_ADDRESS_RANDOM(0x01) or OWN_ADDRESS_RESOLVE_PRIVATE_RANDOM(0x03)
    if ((bltScn.scan_ownAddr_random & 0x01) && !(blmsParam.hci_cmd_mask & SET_RANDOM_ADDR_CMD_MASK)) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if (duration != 0 && period != 0) {
        if (duration * 10 * SYSTEM_TIMER_TICK_1MS >= period * 1280 * SYSTEM_TIMER_TICK_1MS) {
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
    }


    if (extScan_en) {
        //TODO: DUPE_FLTR_ENABLE_RST_PERIOD
        blt_ll_filterAdvDevice(filter_duplicate, 0);

    #if (SCAN_BACKOFF_FEATURE_EN)
        //Upon entering the Scanning State, the upperLimit and backoffCount are set to one.
        bltScn.upperLimit   = 1;
        bltScn.backoffCount = 1;
    #else
        blt_ll_clearScanRspDevice();
    #endif
    } else {
    }

    //Important to disable IRQ
    u32 r = irq_disable();

    if (extScan_en != blmsParam.scanInitEn_union.ext_scan_en) {
        blmsParam.state_chng |= STATE_CHANGE_EXT_SCAN;

        if (extScan_en) {                             //ext_scan_en: 0 -> 1
            bltScn.chn_index        = CHN_INDEX_INIT; //special use
            bltScn.phy_index        = PHY_INDEX_1M;   //PHY will start from 2M
            bltScn.extScan_duration = duration * 10 * SYSTEM_TIMER_TICK_1MS;
            bltScn.extScan_period   = period * 1280 * SYSTEM_TIMER_TICK_1MS;

    #if (SL01_ext_scan_endis)
            log_task_begin_irq(SL_STACK_EXTSCAN_BASIC_TIMING_EN, SL01_ext_scan_endis);
    #endif
        } else {
            bltScn.extScan_1stFlag = 0; ///when disable, need to clear

    #if (SL01_ext_scan_endis)
            log_task_end_irq(SL_STACK_EXTSCAN_BASIC_TIMING_EN, SL01_ext_scan_endis);
    #endif
        }
    }


    blmsParam.scanInitEn_union.ext_scan_en = (u8)extScan_en;

    irq_restore(r);
    #if OS_SUP_EN
    if (blt_os_giveSem_cb) {
        blt_os_giveSem_cb();
    }
    #endif
    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_setExtScanEnable(hci_le_setExtScanEnable_cmdParam_t *pCmdParam)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Set_Ext_Scan_Enable", pCmdParam, sizeof(hci_le_setExtScanEnable_cmdParam_t));

    if (!blmsParam.extScanModule_en) {
        return HCI_ERR_UNKNOWN_HCI_CMD;
    }

    /* HCI/GEV/BV-03-C [Disallow Mixing Legacy and Extended Scanning Commands] */
    if (IS_LEGACY_SCAN_VALID) {
        return HCI_ERR_CMD_DISALLOWED;
    } else {
        SET_EXTENDED_SCAN_VALID;
    }

    return blc_ll_setExtScanEnable(pCmdParam->Enable, pCmdParam->Filter_Duplicates, pCmdParam->Duration, pCmdParam->Period);
}


    #if 0
_attribute_ram_code_ void blt_ll_switchExtScanChannel (int set_chn)
{
    int scan_window_hit;
    if(bltScn.chn_index == CHN_INDEX_INIT){
        scan_window_hit = 1;
    }
    else{
        scan_window_hit = (u32)(clock_time() - bltScn.tick_scan) > bltScn.scnInterval_tick;
    }

    if (scan_window_hit){
        bltScn.tick_scan = clock_time ();
    }

    if(scan_window_hit || set_chn){
        bltScn.chn_index ++;
        if(bltScn.chn_index == 3){ //channel 37/38/39 executed, jump to next PHY
            bltScn.chn_index = 0;
        }

        if(bltScn.chn_index == 0){
            /* jump to next PHY*/
            do{
                bltScn.phy_index = (bltScn.phy_index + 1)%3;
            }while( (bltScn.scanPhy_msk & BIT(bltScn.phy_index)) == 0);

            bltScn.scan_type = bltScn.extScanType[bltScn.phy_index];
            bltScn.scan_percent = bltScn.scanPercent[bltScn.phy_index];
            bltScn.scanInterval = bltScn.scanInter[bltScn.phy_index];
            bltScn.scnInterval_tick = bltScn.scanInte_tick[bltScn.phy_index];
        }
    }


    //2M/Coded PHY feature must be enabled for EXT SCAN, so do not use pointer "ll_phy_switch_cb"
    u8 new_phy_type = bltScn.phy_index + 1; //phy_index + 1 -> "le_phy_type_t"
    rf_ble_switch_phy(new_phy_type, LE_CODED_S8);



    u8 chn = blc_extadv_channel[bltScn.chn_index];
    rf_set_tx_rx_off ();
    rf_set_ble_channel (chn);
    rf_set_ble_access_code_adv ();
    rf_set_ble_crc_adv ();

    blt_ll_set_tx_power_by_strategy(TX_POWER_STRATEGY_CUSTOMER_OR_DEFAULT, 0);

    rf_set_rx_maxlen(37);  //legADV max data length 37, extADV on primary channel data length smaller than 37
    //Switch dma rx buffer to SCAN's dma rx buffer
    ble_rf_set_rx_dma((u8*)bltScn.scan_rx_pri_chn_dma_buff, SCAN_PRICHN_RX_DMA_SIZE);

    CLEAR_ALL_RFIRQ_STATUS;
    rf_set_rxmode ();
}
    #endif


#if (!ESL_RAM_OPTIMIZATION)
_attribute_ram_code_ int blt_ext_scan_disable_process(void)
{
    bltSche.immediate_task = 0;
    AuxScnFutTask.number   = 0; //clear aux_scan future task

    st_secchn_scn_t *cur_pauxscn;
    ;
    for (int i = 0; i < TSKNUM_SECCHN_SCAN; i++) {
        cur_pauxscn = (st_secchn_scn_t *)&secChnScn_tbl[i];
        blt_set_auxscan_enable(cur_pauxscn, 0);
        blt_sche_removeTaskMask(TSKMSK_SECCHN_SCAN_0 << i);

        blt_ll_setSchedulerTaskPriority(TSKOFT_SECCHN_SCAN + i, TASK_PRIORITY_AUX_SCAN_DFT);
    }

    return 0;
}
#endif //(!ESL_RAM_OPTIMIZATION)

_attribute_ram_code_ int blt_ext_scan_interrupt_task(int flag, void *p)
{
    if (flag & FLAG_SCHEDULE_SECCHN_SCAN_INSERT) {
        return blt_insert_aux_scan_future_task();
    } else if (flag & FLAG_SCHEDULE_EXTSCAN_DISABLE) {
#if (!ESL_RAM_OPTIMIZATION)
        blt_ext_scan_disable_process();
#endif //(!ESL_RAM_OPTIMIZATION)
    } else if (flag & FLAG_INSERT_SCHTSK_CONFLICT) {
        int         sec_scn_idx   = flag & FLAG_SCHEDULE_TASK_IDX_MASK;
        sch_task_t *pTgtTsk       = (sch_task_t *)p;
        u8          tgtTskFlg     = pTgtTsk->scheTask_flg & TSKFLG_VALID_MASK;
        u8          curSchTaskOft = TSKOFT_SECCHN_SCAN + sec_scn_idx;
        (void)tgtTskFlg; //remove compiler warning
    #if (SCH_TASK_PRIORITY_IN_CB_EN)
        s32 pri_taskCur = bltPri.pri_cal[curSchTaskOft];
        s32 pri_taskTra = bltPri.pri_cal[pTgtTsk->scheTask_oft];
        //priority higher than exist task, can insert target task
        if (pri_taskCur > pri_taskTra) {
            return 1;
        }
    #endif

        my_dump_str_data(0, "[ext_scn]insertTsk conflict, tgtTsk=", &tgtTskFlg, 1);
    }


    return 0;
}


#if (ESL_CURRENT_OPTIMIZATION)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
int blt_ext_scan_mainloop_task (int flag)
{
    if (flag == FLAG_SCAN_DATA_REPORT) {
        blt_ll_procExtAdvReportEvent();
    } else if (flag == FLAG_EXT_SCAN_MAINLOOP_PEND_TASK) {
        blt_ll_ExtScan_mainloop_checkPendTask();
    } else if (flag == FLAG_MODULE_RESET) {
        blt_reset_ext_scan();
    }

    return 0;
}

_attribute_ram_code_ int blt_ll_procAuxiliaryScanTask(int flag)
{
    //if(bltSche.task_mask & TSKMSK_SECCHN_SCAN_ALL)

    int aux_idx = flag & FLAG_SCHEDULE_TASK_IDX_MASK;

    if (flag & FLAG_SCHEDULE_START) {
        //DBG_C HN1_HIGH;
        DBG_SIHUI_CHN1_HIGH;
    //DBG_QIUWEI_CHN2_HIGH;
    #if (SL01_scn_secchn)
        log_task_begin_irq(SL_STACK_EXTSCAN_BASIC_TIMING_EN, SL01_scn_secchn);
    #endif
#if (!ESL_RAM_OPTIMIZATION)
        blt_aux_scan_start(aux_idx);
#endif //(!ESL_RAM_OPTIMIZATION)
    } else if (flag & FLAG_SCHEDULE_DONE) {
#if (!ESL_RAM_OPTIMIZATION)
        blt_aux_scan_post();
#endif //(!ESL_RAM_OPTIMIZATION)
    #if (SL01_scn_secchn)
        log_task_end_irq(SL_STACK_EXTSCAN_BASIC_TIMING_EN, SL01_scn_secchn);
    #endif
        //DBG_C HN1_LOW;
        //DBG_QIUWEI_CHN2_LOW;
        DBG_SIHUI_CHN1_LOW;
    }


    return 0;
}

#if (!ESL_RAM_OPTIMIZATION)
_attribute_ram_code_ void blt_aux_scan_start(int aux_idx)
{
    bltExtScn.auxadv_sel = aux_idx;
    blt_pSecChnScn       = (st_secchn_scn_t *)&secChnScn_tbl[aux_idx];

    #if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC)
    if (blt_pSecChnScn->pdaSync_flag) {
        blt_pPdAsync = (st_pda_sync_t *)&pdAsync_tbl[blt_pSecChnScn->pdaSync_idx];
        blt_pPda     = (st_pda_t *)&blt_pPdAsync->pda_rx;
    }
    #endif

    /* RX FIFO not released, can not scan, must abandon this aux_scan task  */
    if (((u8)(scan_secRxFifo.wptr - scan_secRxFifo.rptr) & 31) >= SCAN_SECCHN_RXFIFO_NUM) {
        systimer_set_irq_capture(bltSche.sSlot_tick_irq + 100 * SYSTEM_TIMER_TICK_1US);
        blmsParam.rf_fsm_busy = 0;
    } else {
        //gpio change may cost some time, so place here and there will be decades us before RF start.
        if (blc_rf_pa_cb) {
            blc_rf_pa_cb(PA_TYPE_RX_ON);
        } //QW OK

        /* PHY & channel & access_code & CRC_init */
        //2M/Coded PHY feature must be enabled for EXT SCAN, so do not use pointer "ll_phy_switch_cb"
        rf_ble_switch_phy(blt_pSecChnScn->secchn_phy, LE_CODED_S8);

        rf_set_ble_channel(blt_pSecChnScn->next_chnIdx);


        //TX wait no need set, because only SRX & STX & TX2RX mode used in aux_scan
        /* may trigger TX2RXfor aux_connect_req/aux_connect_rsp or aux_scan_req/aux_scan_rsp, so set rx wait value in advance */
        rf_ble_set_rx_wait(RF_RX_WAIT_MIN_VALUE); //only involved in BTX/BRX/TX2RX
        rf_ble_set_rx_settle(RX_SETTLE_US);
        rf_ble_csem_set_tx_rx_settle(0, 0, RX_SETTLE_US);

        rf_start_fsm(FSM_SRX, NULL, clock_time()); // bltSche.system_irq_tick + xxx*SYSTEM_TIMER_TICK_1US;


    #if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC)
        if (blt_pSecChnScn->pdaSync_flag) { //aux_scan for prd_adv
            rf_set_ble_access_code((u8 *)&blt_pPda->paAccessAddr);
            rf_set_ble_crc_value(blt_pPda->paCrcInit);
        } else
    #endif
        { //aux_scan for ext_adv
            rf_set_ble_access_code_adv();
            rf_set_ble_crc_adv();
        }
        rf_trigger_codedPhy_accesscode();

        u16 auxScn_1stRxTm_margin = blt_pSecChnScn->secchn_phy == BLE_PHY_CODED ? 300 : 0;
        if (blt_pSecChnScn->aux_pkt_1stRxTm_us) {
            rf_set_1st_rx_timeout(blt_pSecChnScn->aux_pkt_1stRxTm_us + auxScn_1stRxTm_margin);
            blt_pSecChnScn->aux_pkt_1stRxTm_us = 0;
        } else {
            rf_set_1st_rx_timeout(blt_pSecChnScn->scan_early_set_us + bltPHYs.prmb_ac_us + 150 + auxScn_1stRxTm_margin);
        }
        ble_rf_set_rx_dma((u8 *)bltExtScn.scan_rx_sec_chn_dma_buff, bltExtScn.scan_rx_sec_chn_dma_size);
        //rf_set_rx_maxlen(blt_pSecChnScn->rfLen_max); //sometimes blt_pSecChnScn->rfLen_max = 0; later to debug.
        rf_set_rx_maxlen(255);

        systimer_set_irq_capture(bltSche.sSlot_tick_irq + blt_pSecChnScn->scan_duration_us * SYSTEM_TIMER_TICK_1US);

        blmsParam.rf_fsm_busy = 1;
    }


    //logic setting executing after SRX setting to save time
    auxScnCmnParam.rx_received = 0;

    blms_state          = BLMS_STATE_SECCHN_SCAN_S;
    systick_irq_trigger = SYS_IRQ_TRIG_SECCHN_SCAN_POST;


    if (blt_pSecChnScn->aux_scan_cnt) {
        blt_pSecChnScn->aux_chain_flag = 1;
    } else { //first AUX
        blt_pSecChnScn->scan_rx_flag        = 0;
        blt_pSecChnScn->aux_chain_flag      = 0;
        blt_pSecChnScn->peerAdvA_exist      = 0;
        blt_pSecChnScn->peerTargetA_exist   = 0;
        blt_pSecChnScn->advrpt_hold_dat_len = 0;
        blt_pSecChnScn->perdAdv_interval    = 0;
    }

    blt_pSecChnScn->aux_scan_cnt++;


    blt_remove_aux_scan_future_task(bltExtScn.auxadv_sel);
}

_attribute_ram_code_ void blt_aux_scan_post(void)
{
    if (blc_rf_pa_cb) {
        blc_rf_pa_cb(PA_TYPE_OFF);
    }

    if (blmsParam.rf_fsm_busy) {
        /* timing extending function */
        if (blt_pSecChnScn->scan_duration_flag == DURATION_FLAG_MIN_TIME && rf_receiving_flag()) {
            //1M:        16B 128uS
            //Coded S8   16B  1024uS
            //TODO
            //systimer_set_irq_capture(bltSche.sSlot_tick_irq + blt_pSecChnScn->scan_duration_us*SYSTEM_TIMER_TICK_1US);
            auxScnCmnParam.auxscn_updateScheFlg = 1;
            return;
        }

        STOP_RF_STATE_MACHINE;
        blmsParam.rf_fsm_busy           = 0;
        blmsParam.delay_clear_rf_status = 1;
    }


    blt_ll_calculate_sSlot_next(clock_time() + SLOT_PROCESS_MAX_TICK);


    if (!auxScnCmnParam.rx_received) {
        my_dump_str_data(DBG_AUXSCAN_LOGIC_QW, "secChn no rxPkt", 0, 0);
        blt_release_secchn_scan(blt_pSecChnScn, bltExtScn.auxadv_sel);
    }

    if (auxScnCmnParam.auxscn_updateScheFlg) {
        auxScnCmnParam.auxscn_updateScheFlg = 0;
        /*
         * when first entry aux scan post,if 'if(blt_pSecChnScn->scan_duration_flag == DURATION_FLAG_MIN_TIME && rf_receiving_flag())' is true,
         * because blms_state is not BLMS_STATE_SECCHN_SCAN_E,will not run the next task, and RF will occur cmd done irq, then entry aux scan post again.
         * if do not update the scheduler, maybe occur oxFFOA error.
         */
        blt_sche_addUpdate(SLOT_UPDT_SLOTTBL_RESCHED);
    }
    blms_state = BLMS_STATE_SECCHN_SCAN_E;
}
#endif //(!ESL_RAM_OPTIMIZATION)


    #if (PDA_SCAN_PENDING_FIX_EN)
//scan_rx_flag will be cleared when aux scan start and pda scan start
_attribute_ram_code_ void blt_release_secchn_scan(st_secchn_scn_t *cur_pauxscn, u8 aux_idx)
{
    u32 r = irq_disable();


    u8 tPreEvtCntOfCurPdaEvtCntExist = 0;
    u8 tAuxAdvSamePktExistInRestFifo = 0;

    u8 rx_idx = scan_secRxFifo.rptr;

    u16 tCurPdaEvtCnt = 0;
    if (cur_pauxscn->pdaSync_flag) {
        tCurPdaEvtCnt = pdAsync_tbl[cur_pauxscn->pdaSync_idx].pda_rx.paEvtCnt;
    }

    //step 1. traverse the FIFO packet to find all related packet. maybe aux_adv_ind/periodic adv/chain pkt etc
    while (rx_idx != scan_secRxFifo.wptr) {
        u8 *raw_pkt = (u8 *)(scan_secRxFifo.p + SCAN_SECCHN_RXFIFO_SIZE * (rx_idx & SCAN_SECCHN_RXFIFO_MASK));
        if (raw_pkt[2] == (SECCHN_IDX_MARK | cur_pauxscn->scnIndex)) {
            if (cur_pauxscn->pdaSync_flag) {
                if (raw_pkt[0] == (tCurPdaEvtCnt & 0xFF)) {
                    raw_pkt[3] = SCANRX_FLAG_DATA_DROP;
                }

                if ((raw_pkt[0] + 1) == (tCurPdaEvtCnt & 0xff)) {
                    tPreEvtCntOfCurPdaEvtCntExist = 1;
                }
            } else {
                raw_pkt[3]                    = SCANRX_FLAG_DATA_DROP;
                tAuxAdvSamePktExistInRestFifo = 1;
            }
        }
        rx_idx++;
    }

    //step 2. according different scan type to handle truncated logical.
    //2.1 periodic scan truncated is different from aux scan truncated.
    if (cur_pauxscn->pdaSync_flag) { //if periodic sync
        /*
         * In the rest FIFO there are same paEventCounter packet and previous paEventCounter not exist
         * If previous paEventCounter exist, all same paEventCounter are dropped, so not need truncated processing
         */
        if (!tPreEvtCntOfCurPdaEvtCntExist) {
            if (cur_pauxscn->scan_rx_flag & SCANRX_FLAG_REPORT2HOST) {
                cur_pauxscn->scan_rx_flag |= SCANRX_FLAG_PDASCAN_TRUNCATED_PEND;
            }
        }
    } else { //if aux scan: include aux_adv_ind and its chain packet.
        //situation 1: there are still rx data in the rest second FIFO.
        if (tAuxAdvSamePktExistInRestFifo) { //In the rest second rx fifo,there still be the same packet not send to host.
            //1.1 before not report any packet to host.
            //method:set drop flag(raw_pkt[3]=SCANRX_FLAG_DATA_DROP) and release secTbl right now.
            if (!(cur_pauxscn->scan_rx_flag & SCANRX_FLAG_REPORT2HOST)) { //directly release the related resource.
                cur_pauxscn->scan_rx_flag = 0;
                blt_set_auxscan_enable(cur_pauxscn, 0);                   //pay attention: operate IRQ variable
            }
            //1.2 before already reported some packets to host.
            //method:
            //step1. set truncated pending flag(scan_rx_flag |= SCANRX_FLAG_EXTSCAN_TRUNCATED_PEND) and release secTbl in truncated processing.
            //step2. set the drop flag(raw_pkt[3]=SCANRX_FLAG_DATA_DROP)
            //the code above and other code have processed this situation. here do nothing.
            else {
                cur_pauxscn->scan_rx_flag |= SCANRX_FLAG_EXTSCAN_TRUNCATED_PEND;
            }
        } else { //situation 2: no rx data in the rest second FIFO.
            //2.1 already sent some packet to host.need to send incomplete truncated event separately.
            if (cur_pauxscn->scan_rx_flag & SCANRX_FLAG_REPORT2HOST) {           //1. has already sent some packet.

                cur_pauxscn->scan_rx_flag |= SCANRX_FLAG_EXTSCAN_TRUNCATED_PEND; //if has reported one or more event,pending then mainloop process
                my_dump_str_data(DBG_AUXSCAN_LOGIC_QW, "release pend", 0, 0);
            } else {                                                             //2.2 before not send any packet.

                blt_set_auxscan_enable(cur_pauxscn, 0);                          ///if not report any event,immediately release.
                my_dump_str_data(DBG_AUXSCAN_LOGIC_QW, "release no pend", 0, 0);
            }
        }
    }

    irq_restore(r);
}

    #else
//scan_rx_flag will be cleared when aux scan start and pda scan start
_attribute_ram_code_ void blt_release_secchn_scan(st_secchn_scn_t *cur_pauxscn, u8 aux_idx)
{
    (void)aux_idx; //unused, remove warning

    u32 r = irq_disable();

    u8 restSecRxFifo_samePkt = 0;
    u8 rx_idx                = scan_secRxFifo.rptr;

    //traverse the FIFO packet to find all related packet. maybe aux_adv_ind/periodic adv/chain pkt etc
    while (rx_idx != scan_secRxFifo.wptr) {
        u8 *raw_pkt = (u8 *)(scan_secRxFifo.p + SCAN_SECCHN_RXFIFO_SIZE * (rx_idx & SCAN_SECCHN_RXFIFO_MASK));
        if (raw_pkt[2] == (SECCHN_IDX_MARK | cur_pauxscn->scnIndex)) {
            if ((cur_pauxscn->scan_rx_flag & SCANRX_FLAG_REPORT2HOST) == SCANRX_FLAG_REPORT2HOST) {
                if (cur_pauxscn->scan_rx_flag & SCANRX_FLAG_PDA) {
                    //not use 'scan_rx_flag' to pend,this method is error. later need to rewrite.
                    cur_pauxscn->scan_rx_flag |= SCANRX_FLAG_PDASCAN_TRUNCATED_PEND;
                } else {
                    cur_pauxscn->scan_rx_flag |= SCANRX_FLAG_EXTSCAN_TRUNCATED_PEND;
                }
            }

            raw_pkt[3] = SCANRX_FLAG_DATA_DROP; //drop in mainloop

            //////////////////////////////////////////////////////
            restSecRxFifo_samePkt = 1;
            my_dump_str_data(DBG_AUXSCAN_LOGIC_QW, "match", &raw_pkt[3], 1);
        }

        rx_idx++;
    }

    //situation 1: there are still rx data in the rest second FIFO.
    if (restSecRxFifo_samePkt) { //In the rest second rx fifo,there still be the same packet not send to host.
        //1.1 before not report any packet to host.
        //method:set drop flag(raw_pkt[3]=SCANRX_FLAG_DATA_DROP) and release secTbl right now.
        if (!(cur_pauxscn->scan_rx_flag & SCANRX_FLAG_REPORT2HOST)) { //directly release the related resource.
            cur_pauxscn->scan_rx_flag = 0;

            if (!(cur_pauxscn->scan_rx_flag & SCANRX_FLAG_PDA)) {     //pda can not release
                blt_set_auxscan_enable(cur_pauxscn, 0);               //pay attention: operate IRQ variable
            }
        }
        //1.2 before already reported some packets to host.
        //method:
        //step1. set truncated pending flag(scan_rx_flag |= SCANRX_FLAG_EXTSCAN_TRUNCATED_PEND) and release secTbl in truncated processing.
        //step2. set the drop flag(raw_pkt[3]=SCANRX_FLAG_DATA_DROP)
        //the code above and other code have processed this situation. here do nothing.
    } else { //situation 2: no rx data in the rest second FIFO.
        //2.1 already sent some packet to host.need to send incomplete truncated event separately.
        if (cur_pauxscn->scan_rx_flag & SCANRX_FLAG_REPORT2HOST) { //1. has already sent some packet.
            if (cur_pauxscn->scan_rx_flag & SCANRX_FLAG_PDA) {
                //not use 'scan_rx_flag' to pend,this method is error. later need to rewrite.
                cur_pauxscn->scan_rx_flag |= SCANRX_FLAG_PDASCAN_TRUNCATED_PEND; //if has reported one or more event,pending then mainloop process
            } else {
                cur_pauxscn->scan_rx_flag |= SCANRX_FLAG_EXTSCAN_TRUNCATED_PEND; //if has reported one or more event,pending then mainloop process
            }

            my_dump_str_data(DBG_AUXSCAN_LOGIC_QW, "release pend", 0, 0);
        } else {                                                  //2.2 before not send any packet.

            if (!(cur_pauxscn->scan_rx_flag & SCANRX_FLAG_PDA)) { //pda can not release
                blt_set_auxscan_enable(cur_pauxscn, 0);           ///if not report any event,immediately release.
            }
            my_dump_str_data(DBG_AUXSCAN_LOGIC_QW, "release no pend", 0, 0);
        }
    }


    irq_restore(r);
}
    #endif //PDA_SCAN_PENDING_FIX_EN


_attribute_ram_code_ void blt_add_aux_scan_future_task(u8 llTask_idx, u8 llTask_offset, u32 tick_start, u32 tick_end)
{
    if (llTask_idx >= TSKNUM_SECCHN_SCAN) {
        BLMS_ERR_DEBUG(DBG_EXTSCAN_TIMING, 0xFE0D0000 | llTask_idx);
    }

    #if (SLEV_auxscanFutrTsk_add)
    log_event_irq(SL_STACK_EXTSCAN_BASIC_TIMING_EN, SLEV_auxscanFutrTsk_add);
    #endif

    if (AuxScnFutTask.number < AUX_SCAN_FUTURE_TASK_MAX_NUM) {
        int i = 0;
        if (AuxScnFutTask.number != 0) {
            for (i = 0; i < AuxScnFutTask.number; i++) { //locate insert position in a ordered queue
                if (tick1_exceed_tick2(AuxScnFutTask.auxScan_tbl[i].tick_s, tick_start)) {
                    break;
                }
            }

            if (i != AuxScnFutTask.number) { //not last one
                /* attention: can note use smemcpy4( &tbl[i], &tbl[i+1], number - i) !!!
                 * cause smemcpy4 do not support SRAM data hold when source and destination overlap */
                for (int j = AuxScnFutTask.number - 1; j >= i; j--) { //locate insert position in a ordered queue
                    //smemcpy4(&AuxScnFutTask.auxScan_tbl[j], &AuxScnFutTask.auxScan_tbl[j+1], sizeof(fut_task_type_2) );
                    smemcpy4(&AuxScnFutTask.auxScan_tbl[j + 1], &AuxScnFutTask.auxScan_tbl[j], sizeof(fut_task_type_2));
                }
            }
        }


        fut_task_type_2 *pFutTask = (fut_task_type_2 *)&AuxScnFutTask.auxScan_tbl[i];
        pFutTask->task_idx        = llTask_idx;
        pFutTask->task_oft        = llTask_offset;
        pFutTask->tick_s          = tick_start;
        pFutTask->tick_e          = tick_end;

        AuxScnFutTask.number++;

    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
        if (llTask_offset >= TSKOFT_PAWRS_RSP && llTask_offset < (TSKOFT_PAWRS_RSP + TSKNUM_PAWRS_RSP)) {
            blt_sche_addTaskMask(TSKMSK_PAWRS_RSP_0 << llTask_idx);
        } else {
            blt_sche_addTaskMask(TSKMSK_SECCHN_SCAN_0 << llTask_idx);
        }
    #else
        blt_sche_addTaskMask(TSKMSK_SECCHN_SCAN_0 << llTask_idx);
    #endif

        //my_dump_str_data(DBG_EXTSCAN_TIMING, "add aux fut task", &pFutTask->tick_s, 4);
    } else {
        my_dump_str_data(DBG_EXTSCAN_TIMING, "add aux fut task err", &AuxScnFutTask.number, 1);
    }
}

_attribute_ram_code_ bool blt_remove_aux_scan_future_task(u8 llTask_idx)
{
    for (int i = 0; i < AuxScnFutTask.number; i++) {
        if (AuxScnFutTask.auxScan_tbl[i].task_idx == llTask_idx) {
    #if 1 //optimize
            AuxScnFutTask.number--;
            if (i != AuxScnFutTask.number) {
                smemcpy4(&AuxScnFutTask.auxScan_tbl[i], &AuxScnFutTask.auxScan_tbl[i + 1], sizeof(future_task_e) * (AuxScnFutTask.number - i));
            }
    #else
            if (i != AuxScnFutTask.number - 1) {
                smemcpy4(&AuxScnFutTask.auxScan_tbl[i], &AuxScnFutTask.auxScan_tbl[i + 1], sizeof(fut_task_type_2) * (AuxScnFutTask.number - 1 - i));
            }
            AuxScnFutTask.number--;
    #endif

    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
            if (AuxScnFutTask.auxScan_tbl[i].task_oft >= TSKOFT_PAWRS_RSP && AuxScnFutTask.auxScan_tbl[i].task_oft < (TSKOFT_PAWRS_RSP + TSKNUM_PAWRS_RSP)) {
                blt_sche_removeTaskMask(TSKMSK_PAWRS_RSP_0 << llTask_idx);
            } else {
                blt_sche_removeTaskMask(TSKMSK_SECCHN_SCAN_0 << llTask_idx);
            }

    #else
            blt_sche_removeTaskMask(TSKMSK_SECCHN_SCAN_0 << llTask_idx);
    #endif

            return TRUE;
        }
    }

    return FALSE;
}

_attribute_ram_code_ int blt_check_aux_scan_future_task(u32 tick_available)
{
    int i;
    /* check if any task timing passed */
    for (i = 0; i < AuxScnFutTask.number; i++) {
        if (tick1_exceed_tick2(AuxScnFutTask.auxScan_tbl[i].tick_s, tick_available)) {
            break;
        } else {
            my_dump_str_data(DBG_EXTSCAN_TIMING, "aux task timing passed", &tick_available, 4);
            //BLMS_ERR_DEBUG(DBG_EXTSCAN_TIMING, 0xFE040000);

            // clear some information
            u8 aux_idx = AuxScnFutTask.auxScan_tbl[i].task_idx;
            blt_sche_removeTaskMask(TSKMSK_SECCHN_SCAN_0 << aux_idx);

            st_secchn_scn_t *cur_pauxscn = (st_secchn_scn_t *)&secChnScn_tbl[aux_idx];
            blt_release_secchn_scan(cur_pauxscn, aux_idx);
        }
    }


    /* delete timing passed task, move un_passed task in front of the queue */
    if (i != 0) {
        int delete_task_num = i;
        AuxScnFutTask.number -= delete_task_num;
        smemcpy4(&AuxScnFutTask.auxScan_tbl[0], &AuxScnFutTask.auxScan_tbl[i], sizeof(future_task_e) * AuxScnFutTask.number);

        my_dump_str_data(DBG_EXTSCAN_TIMING, "aux task passed", &delete_task_num, 1);
    }


    return 0;
}

extern u8 blt_ll_resolve_insertSchTskConflict(sch_task_t *pInsertTsk, sch_task_t *pTgtTsk);

_attribute_ram_code_ int blt_insert_aux_scan_future_task(void)
{
    blt_check_aux_scan_future_task(clock_time() + 50 * SYSTEM_TIMER_TICK_1US);


    /* process first task of the queue if queue not empty */
    if (AuxScnFutTask.number) {
        //int allocate = 0;
        fut_task_type_2 *pFutTask = (fut_task_type_2 *)&AuxScnFutTask.auxScan_tbl[0];
        sch_task_t      *pSchTsk  = (sch_task_t *)&secChnScn_tbl[pFutTask->task_idx].auxScnTsk;

    #if 1                                                          //very important to fix one bug: sSlot_idx_reset will eat some timing, other task also under risk !!! TODO: SiHui
        s32 delta_tick      = (s32)(pFutTask->tick_s - bltSche.sSlot_tick_start);
        s32 sSlot_aux_begin = delta_tick * SSLOT_TICK_REVERSE - 0; //0: sSlot_idx_start
        s32 sSlot_aux_end   = sSlot_aux_begin + (pFutTask->tick_e - pFutTask->tick_s) * SSLOT_TICK_REVERSE + 1;
    #else
        s32 sSlot_aux_begin = (pFutTask->tick_s - bltSche.sSlot_tick_start) * SSLOT_TICK_REVERSE - 0; //0: sSlot_idx_start
        s32 sSlot_aux_end   = (pFutTask->tick_e - bltSche.sSlot_tick_start) * SSLOT_TICK_REVERSE - 0; //0: sSlot_idx_start
    #endif

        pSchTsk->begin = sSlot_aux_begin;
        pSchTsk->end   = sSlot_aux_end;

        secChnScn_tbl[pFutTask->task_idx].pawr_rsp_tick = pFutTask->tick_s | 0x01;

        //my_dump_str_u32s(DBG_EXTSCAN_TIMING, "debug 3", sSlot_aux_begin, pFutTask->tick_s, bltSche.sSlot_tick_start, 0);

        /* for immediate task, may happens that no more task on link_list */
        if (bltSche.pTask_next == NULL) {
            my_dump_str_data(DBG_EXTSCAN_TIMING, "next NULL", 0, 0);
            BLMS_ERR_DEBUG(DBG_EXTSCAN_TIMING, 0xFE030000);
        }
        /* 2. first future task is totally after link_list next task, no need process any more ,finish */
        else if (sSlot_aux_begin > bltSche.pTask_next->end) {
        }
        /* 3. first future task is totally before link_list next task, run it directly */
        else if (bltSche.pTask_next->begin > sSlot_aux_end) {
            //allocate = 1;
            pSchTsk->next      = bltSche.pTask_next;
            bltSche.pTask_next = pSchTsk;
            //TODO, some other link_list operation
        }
        /* 4. other situation: overlap with link_list next task, and may overlap with some more link_list task,
         * should check task timing and priority, to decide which task be abandoned  */
        else {
            //my_dump_str_data(DBG_EXTSCAN_TIMING, "overwrite", 0, 0);

            int         task_conflict_num = 0;
            u8          task_offset[MAX_CONFLICT_NUM + 1]; //attention: should add 1
            sch_task_t *pTsk_traverse = bltSche.pTask_next;

    #if (SCH_TASK_PRIORITY_IN_CB_EN == 0)
            s32 pri_taskCur = bltPri.pri_cal[pFutTask->task_oft];
    #endif

            int overwrite = 1;
            while (pTsk_traverse && sSlot_aux_end > pTsk_traverse->begin) { //overlap

    #if (SCH_TASK_PRIORITY_IN_CB_EN == 0)
                s32 pri_taskTra = bltPri.pri_cal[pTsk_traverse->scheTask_oft];
                //priority lower than exist task, can not replace, abandon current task
                if (pri_taskCur <= pri_taskTra) {
                    if (!blt_ll_resolve_insertSchTskConflict(pSchTsk, pTsk_traverse)) {
                        overwrite = 0;
                        break;
                    }
                }
    #else
                if (!blt_ll_resolve_insertSchTskConflict(pSchTsk, pTsk_traverse)) {
                    overwrite = 0;
                    break;
                }
    #endif

                task_offset[task_conflict_num++] = pTsk_traverse->scheTask_oft;
                if (task_conflict_num > MAX_CONFLICT_NUM) {
                    overwrite = 0;
                    break;
                }

                //my_dump_str_u32s(DBG_EXTSCAN_LOGIC, "priority", pri_taskCur, pri_taskTra, pTsk_traverse->scheTask_oft, pTsk_traverse);

                pTsk_traverse = pTsk_traverse->next;
            }


            //current task priority higher than all task traversed, and conflict number not exceed 4, can replace all other task
            if (overwrite) {
                //allocate = 1;
                //blt_ll_addTask2AbandonTaskLinklist(bltSche.pTask_next, task_conflict_num); //attention: must execute before pExtLkTsk_left->next changed

                bltSche.pTask_next = pSchTsk;
                pSchTsk->next      = pTsk_traverse;

                // TODO: priority operation

                for (int j = 0; j < task_conflict_num; j++) {
                    blt_ll_incSchedulerTaskCalPriority(task_offset[j], bltPri.step_final[task_offset[j]] * 3);
                }
            } else { //can not overwrite
                /* for AUX SCAN task, no need abandon task management, only check if future task timing passed now and then */
                // TODO: priority operation
            }
        }
    }


    return 0;
}

    #define EXTADV_REPORT_PACKET_COMBINE_ENABLE 0


    #define EXTADV_TRUNCATED_EVT_SIZE           26

_attribute_data_retention_ extAdvRptEvt_t extAdv_truncatedEvt[TSKNUM_SECCHN_SCAN];

_attribute_data_retention_ addrInforBackup_t extAdvRpt_AdvAddrBk;

#if (ESL_CURRENT_OPTIMIZATION)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
int  blt_ll_procExtAdvReportEvent(void)
{
    //sleep_us(100000);  //debug data combine

    /* if ext_scan disable, drop all scan data */
    #if 0
    if(!blmsParam.scanInitEn_union.ext_scan_en){
        scan_priRxFifo.rptr = scan_priRxFifo.wptr;
        scan_secRxFifo.rptr = scan_secRxFifo.wptr;
        return 0;
    }
    #endif


    u8               *raw_pkt;
    extAdvEvt_info_t *pExtAdvInfo;
    st_secchn_scn_t  *cur_pauxscn    = NULL;
    rf_packet_adv_t  *pCommonAdv     = NULL;
    rf_pkt_ext_adv_t *pPriExtAdv     = NULL;
    rf_packet_adv_t  *pPriLegAdv     = NULL;
    rf_pkt_ext_adv_t *pAuxAdv        = NULL;
    st_prichn_scn_t  *cur_pPrichnScn = NULL;
    u8               *pHolDataBuf    = NULL;

    u8 temp_buff[256];        //process max length 255
    temp_buff[0]      = HCI_SUB_EVT_LE_EXTENDED_ADVERTISING_REPORT;
    u8  num_of_report = 0, manual_check_last = 0, aux_idx = 0;
    int total_rptevt_len = 2; // 2 = 1(subEvent_code) + 1(num_reports)


    if (bltExtScn.truncated_scan_msk) {
    }


    #if (EXTADV_REPORT_PACKET_COMBINE_ENABLE)
    while (scan_priRxFifo.rptr != scan_priRxFifo.wptr || scan_secRxFifo.rptr != scan_secRxFifo.wptr || !manual_check_last)
    #else
    while (scan_priRxFifo.rptr != scan_priRxFifo.wptr || scan_secRxFifo.rptr != scan_secRxFifo.wptr)
    #endif
    {

        u8 sec_chn_flag   = 0;
        u8 scanRx_flag    = 0;
        u8 new_advdat_len = 0; //  < 256


        u8 cur_rx_pkt_process = 0;
        u8 extadv_report      = 0;
        u8 old_hold_data_len  = 0;
        u8 new_hold_data_len  = 0;


        u8 rpt_1_src_data_offset = 0;
        u8 rpt_2_src_data_offset = 0;
        u8 rpt_2_exist           = 0;
        u8 last_adv_data_flag    = 0;

        u32 rpt_1_copy_data_len = 0;                      //change to u8 at last
        u32 advType_mask        = 0;

        if (scan_secRxFifo.rptr != scan_secRxFifo.wptr) { //Prioritize second FIFO data.

            //for second scan, here make sure at least one hci buffer to use.
            //make sure push UART Tx FIFO successfully.//blc_hci_send_event--blc_hci_send_data
            if (bltempParam.hci_aclRxFifo_set == 1) { //only controller project to judge. other project not use UART to send.
                if (blc_hci_isHciTxFIFOfull()) {
                    return -1;
                }
            }

            sec_chn_flag = 1;
            raw_pkt      = (u8 *)(scan_secRxFifo.p + SCAN_SECCHN_RXFIFO_SIZE * (scan_secRxFifo.rptr & SCAN_SECCHN_RXFIFO_MASK));

            scanRx_flag = raw_pkt[3];
            if (scanRx_flag & SCANRX_FLAG_DATA_DROP) {
                scan_secRxFifo.rptr++;
                continue;
            }

            new_advdat_len = raw_pkt[1];                      //raw_pkt[1] is pure ADV data length(not include header and extended header), already calculated in IRQ
            aux_idx        = raw_pkt[2] & (~SECCHN_IDX_MARK); // index stored on raw_pkt[2], BIT(7) is SECCHN_IDX_MARK

            pAuxAdv     = (rf_pkt_ext_adv_t *)(raw_pkt + DMA_RFRX_LEN_HW_INFO);
            cur_pauxscn = (st_secchn_scn_t *)&secChnScn_tbl[aux_idx];


            if (scanRx_flag & SCANRX_FLAG_PDA) {
                if (ll_pda_sync_mlp_task_cb) {
                    //after report pda_sync_established event, pda adv event can be reported
                    if (RTN_BREAK == ll_pda_sync_mlp_task_cb(FLAG_PRDADV_DATA_REPORT)) { //blt_pda_sync_prdadv_data_report
                        break;
                    }
                }
                scan_secRxFifo.rptr++;
                continue;
            }
    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
            else if (scanRx_flag & SCANRX_FALG_PAWR) {
                if (ll_pawr_sync_mlp_task_cb) {
                    if (RTN_BREAK == ll_pawr_sync_mlp_task_cb(FLAG_PRDADV_DATA_REPORT)) { //blt_ll_PAwRsync_adv_rpt
                        break;
                    }
                }
                scan_secRxFifo.rptr++;
                continue;
            }
    #endif
        } else if (scan_priRxFifo.rptr != scan_priRxFifo.wptr) {
            /* Legacy & Non_Connectable Non_Scannable without auxiliary packet */
            u8 raw_fifo_idx = scan_priRxFifo.rptr & SCAN_PRICHN_RXFIFO_MASK;
            raw_pkt         = (u8 *)(scan_priRxFifo.p + SCAN_PRICHN_RXFIFO_SIZE * raw_fifo_idx);

            new_advdat_len = raw_pkt[1];

            rf_packet_adv_t *pAdv = (rf_packet_adv_t *)(raw_pkt + DMA_RFRX_LEN_HW_INFO);
            advType_mask          = BIT(pAdv->type);

            if (advType_mask == TYPE_MASK_EXT_ADV) {
                pPriExtAdv = (rf_pkt_ext_adv_t *)(raw_pkt + DMA_RFRX_LEN_HW_INFO);
            } else {
                pPriLegAdv = (rf_packet_adv_t *)(raw_pkt + DMA_RFRX_LEN_HW_INFO);
            }
#if (!ESL_RAM_OPTIMIZATION)
            cur_pPrichnScn = (st_prichn_scn_t *)&priChnScn_tbl[raw_fifo_idx];
#endif //(!ESL_RAM_OPTIMIZATION)
        }
    #if (EXTADV_REPORT_PACKET_COMBINE_ENABLE)
        else {
            manual_check_last = 1;
        }
    #endif

        pCommonAdv = (rf_packet_adv_t *)(raw_pkt + DMA_RFRX_LEN_HW_INFO);

        //TODO: add CRC 24 check for Kite, cause Kite hardware CRC24 bug


        int new_advevt_len;      //may > 256

        if (manual_check_last) {
            if (num_of_report) { // 2 = 1(subEvent_code) + 1(num_reports)
                extadv_report = 1;
            }

            cur_rx_pkt_process = 0;
        } else {
            pExtAdvInfo = (extAdvEvt_info_t *)&temp_buff[total_rptevt_len];

            if (sec_chn_flag) { //check if previous hold data exist
                old_hold_data_len = cur_pauxscn->advrpt_hold_dat_len;
            }

            rpt_1_copy_data_len = new_advdat_len;
            new_advevt_len      = EXTADV_INFO_LENGTH + new_advdat_len;
            total_rptevt_len += new_advevt_len;

            if (old_hold_data_len) {
                if (num_of_report || old_hold_data_len > EXTADV_RPT_DATA_LEN_MAX) { //debug
                    BLMS_ERR_DEBUG(DBG_EXTSCAN_TIMING, 0xFE060000 | old_hold_data_len << 8 | num_of_report);
                }

                extadv_report      = 1;
                cur_rx_pkt_process = 1;

                total_rptevt_len += old_hold_data_len;
                if (total_rptevt_len > 255) {
                    new_hold_data_len = total_rptevt_len - 255;
                    total_rptevt_len  = 255;
                }
            } else {
                if (total_rptevt_len > 255) {
                    extadv_report = 1;
                    if (num_of_report) { //previous adv_event pending,
                        cur_rx_pkt_process = 0;
                    } else {             //new adv_event
                        cur_rx_pkt_process = 1;
                        new_hold_data_len  = total_rptevt_len - 255;
                    }

                    total_rptevt_len = 255;
                } else if (total_rptevt_len > 231) { //232~255,   231 = 255 - 24
                    extadv_report      = 1;
                    cur_rx_pkt_process = 1;
                } else {
    #if (EXTADV_REPORT_PACKET_COMBINE_ENABLE)
                    extadv_report = 0;
    #else
                    extadv_report = 1;
    #endif
                    cur_rx_pkt_process = 1;
                }
            }


            rpt_1_src_data_offset = cur_pauxscn->peerAdv_datOffset - 2; //-2 indicate extended header length(6bit)+adv mode(2bit)+extended header flag(1B).
            if (old_hold_data_len || new_hold_data_len) {
                /* data hold can only happen in sec_chn scan */
                if (sec_chn_flag == 0) {
                    BLMS_ERR_DEBUG(DBG_EXTSCAN_LOGIC, 0xFE0F0000 | old_hold_data_len << 8 | new_hold_data_len);
                }

                pHolDataBuf = (u8 *)&extadv_pda_rpt_hold_data_buf[aux_idx][0];

                if (old_hold_data_len) {
                    smemcpy(pExtAdvInfo->data, pHolDataBuf, old_hold_data_len);
                    new_advdat_len += old_hold_data_len;
                }

                if (new_hold_data_len) {
                    new_advdat_len      = EXTADV_RPT_DATA_LEN_MAX; //229, pay attention it changes here
                    rpt_1_copy_data_len = EXTADV_RPT_DATA_LEN_MAX - old_hold_data_len;

                    u8 pkt_hold_data_offset = rpt_2_src_data_offset = rpt_1_src_data_offset + rpt_1_copy_data_len;

                    if (new_hold_data_len > EXTADV_RPT_DATA_LEN_MAX) { // > 229
                        /* eg. old hold data length 210 Byte, new data length 250byte,  250+24= 274,
                         * 274+215=489,  489-255=234, one hold packet not enough */
                        my_dump_str_data(DBG_EXTSCAN_LOGIC, "[ext scn]2 rpt pkt", &new_hold_data_len, 1);
                        new_hold_data_len -= EXTADV_RPT_DATA_LEN_MAX;
                        pkt_hold_data_offset += EXTADV_RPT_DATA_LEN_MAX;
                        rpt_2_exist = 1;
                    }
                    if (new_hold_data_len >= EXTADV_RPT_DATA_LEN_MAX) { //debug
                        BLMS_ERR_DEBUG(DBG_EXTSCAN_LOGIC, 0xFE070000 | new_hold_data_len);
                    }


                    if (new_hold_data_len) { //can confirm that only sec_chn_scan trigger this
                        smemcpy(pHolDataBuf, pAuxAdv->data + pkt_hold_data_offset, new_hold_data_len);
                        //my_dump_str_u32s(DBG_EXTSCAN_LOGIC, "debug1", pkt_hold_data_offset, pHolDataBuf[0], pHolDataBuf, 0);

                        cur_pauxscn->advrpt_hold_dat_len = new_hold_data_len; //pay attention: operate IRQ variable
                    }
                }
            }
            if ((!new_hold_data_len) && sec_chn_flag) {
                cur_pauxscn->advrpt_hold_dat_len = 0; //if new_hold_data_len == 0, need to clear relevant variable.
            }
        }


        if (cur_rx_pkt_process) {
            num_of_report++; //update event number

            u8  advType_pkt      = pCommonAdv->type;
            u16 eventType_report = 0;


            //////////////////////////////////////////////////////////////////////////////////////////////////////////
            if (advType_pkt == LL_TYPE_ADV_EXT_IND) {
                if (sec_chn_flag) {
                    eventType_report = cur_pauxscn->ext_event_type_8bit;
                    if (cur_pauxscn->total_targetA_exist) {
                        eventType_report |= EXTADV_RPT_EVT_MASK_DIRECTED;
                    }

                    ///mac process
                    if (scanRx_flag & SCANRX_FLAG_FIRST_DATA) {
                        pExtAdvInfo->address_type = cur_pauxscn->advA_rpt_adrType;
                        extAdvRpt_AdvAddrBk.addr_type = cur_pauxscn->advA_rpt_adrType;

                        if(cur_pauxscn->advA_rpt_adrType == 0xFF){ //anonymous advertisement
                            smemset(pExtAdvInfo->address, 0, BLE_ADDR_LEN);
                            smemset(extAdvRpt_AdvAddrBk.address, 0, BLE_ADDR_LEN);
                        }else{
                            smemcpy(pExtAdvInfo->address, cur_pauxscn->advA_rpt_addr, BLE_ADDR_LEN);
                            smemcpy(extAdvRpt_AdvAddrBk.address, cur_pauxscn->advA_rpt_addr, BLE_ADDR_LEN);
                        }
                    }
                    if (cur_pauxscn->scan_rx_flag & SCANRX_FLAG_REPORT2HOST) {
                        pExtAdvInfo->address_type = extAdvRpt_AdvAddrBk.addr_type;
                        smemcpy(pExtAdvInfo->address, extAdvRpt_AdvAddrBk.address, BLE_ADDR_LEN);
                    }
                    ///other not first data, use the same address type and address with first.

                    my_dump_str_data(DBG_EXTSCAN_REPORT, "extended, data", 0, 0);
                } else { //Non_Connectable Non_Scannable without auxiliary packet
                    eventType_report = cur_pPrichnScn->direct_flag ? EXTADV_RPT_EVTTYPE_EXT_NON_CONN_NON_SCAN_DIRECTED : EXTADV_RPT_EVTTYPE_EXT_NON_CONN_NON_SCAN_UNDIRECTED;
                    eventType_report |= EXTADV_RPT_DATA_COMPLETE;

                    /* IRQ code have confirm: must have advA, so no need judge "EXTHD_BIT_ADVA" bit here
                     * "rpt_addr_type" and "data" have processed in IRQ, will be IDA if RPA resolve success */
                    pExtAdvInfo->address_type = cur_pPrichnScn->rpt_addr_type;
                    smemcpy(pExtAdvInfo->address, pPriExtAdv->data, BLE_ADDR_LEN); //have processed in IRQ

                    my_dump_str_data(DBG_EXTSCAN_REPORT, "extended, Non_Conn_Non_Scan without AUX", 0, 0);
                }
            } else //legacy ADV
            {
                if (advType_pkt == LL_TYPE_ADV_IND) {
                    eventType_report = EXTADV_RPT_EVTTYPE_LEGACY_ADV_IND;
                    my_dump_str_data(DBG_EXTSCAN_REPORT, "legacy, ADV_IND", 0, 0);
                } else if (advType_pkt == LL_TYPE_ADV_DIRECT_IND) {
                    eventType_report = EXTADV_RPT_EVTTYPE_LEGACY_ADV_DIRECT_IND;
                    my_dump_str_data(DBG_EXTSCAN_REPORT, "legacy,ADV_DIRECT_IND", 0, 0);
                } else if (advType_pkt == LL_TYPE_ADV_NONCONN_IND) {
                    eventType_report = EXTADV_RPT_EVTTYPE_LEGACY_ADV_NONCONN_IND;
                    my_dump_str_data(DBG_EXTSCAN_REPORT, "legacy,ADV_NONCONN_IND", 0, 0);
                } else if (advType_pkt == LL_TYPE_SCAN_RSP) {
                    if (1) {
                        eventType_report = EXTADV_RPT_EVTTYPE_LEGACY_SCAN_RSP_2_ADV_IND;
                    } else {
                        eventType_report = EXTADV_RPT_EVTTYPE_LEGACY_SCAN_RSP_2_ADV_SCAN_IND;
                    }
                    my_dump_str_data(DBG_EXTSCAN_REPORT, "legacy, SCAN_RSP", 0, 0);
                } else if (advType_pkt == LL_TYPE_ADV_SCAN_IND) {
                    eventType_report = EXTADV_RPT_EVTTYPE_LEGACY_ADV_SCAN_IND;
                    my_dump_str_data(DBG_EXTSCAN_REPORT, "legacy, ADV_SCAN_IND", 0, 0);
                }
                eventType_report |= EXTADV_RPT_DATA_COMPLETE;


                /* advA type: 0/1/2/3, have processed in IRQ
                   advA address have processed in IRQ, will be IDA if RPA resolve pass */
                u8 report_adv_type        = raw_pkt[DMA_RFRX_OFFSET_HEADER] >> 6;
                pExtAdvInfo->address_type = report_adv_type;
                smemcpy(pExtAdvInfo->address, pPriLegAdv->advA, BLE_ADDR_LEN);
            }
            //////////////////////////////////////////////////////////////////////////////////////////////////////


            pExtAdvInfo->event_type = eventType_report;
            //pExtAdvInfo->address_type = 0;
            //smemcpy(pExtAdvInfo->address, 0 , BLE_ADDR_LEN);
            //pExtAdvInfo->primary_phy = 0;
            //pExtAdvInfo->secondary_phy = 0;
            //pExtAdvInfo->advertising_sid = 0;
            //pExtAdvInfo->tx_power = 0;
            pExtAdvInfo->rssi = raw_pkt[DMA_RFRX_OFFSET_RSSI(raw_pkt)] - 110 + blt_ll_getRfRxPathComp();
            //pExtAdvInfo->perd_adv_inter = PERIODIC_ADV_INTER_NO_PERIODIC_ADV;
            //pExtAdvInfo->direct_address_type = 0;
            //smemcpy(pExtAdvInfo->direct_address, 0 , BLE_ADDR_LEN);
            pExtAdvInfo->data_length = new_advdat_len;
            //smemcpy(pExtAdvInfo->data);


#if (LL_FEATURE_ENABLE_MONITORING_ADVERTISERS)
            if (ll_mon_adv_mlp_task_cb && mon_adv_en) {
                ll_mon_adv_mlp_task_cb(FLAG_MON_ADV_DATA_REPORT_EXTADV, pExtAdvInfo);  //blt_mon_adv_mainloop_task
            }
#endif
            ////////////////////////////////////////////


            if (sec_chn_flag) { // Extended with auxiliary packet

                pExtAdvInfo->primary_phy     = cur_pauxscn->prichn_phy;
                pExtAdvInfo->secondary_phy   = cur_pauxscn->secchn_phy;
                pExtAdvInfo->advertising_sid = cur_pauxscn->peerAdv_id.sid;
                pExtAdvInfo->tx_power        = cur_pauxscn->peerAdv_txPower;
                pExtAdvInfo->perd_adv_inter  = cur_pauxscn->perdAdv_interval;

                if (cur_pauxscn->total_targetA_exist) {
                    pExtAdvInfo->direct_address_type = blt_extscan_convert_direct_adr_type(cur_pauxscn->record_direct_adrType,
                                                                                           cur_pauxscn->record_direct_addr,
                                                                                           cur_pauxscn->direct_rpa_resolve_fail);
                    smemcpy(pExtAdvInfo->direct_address, cur_pauxscn->record_direct_addr, BLE_ADDR_LEN);
                } else {
                    //no care, set 0 here
                    pExtAdvInfo->direct_address_type = 0;
                    smemset(pExtAdvInfo->direct_address, 0, BLE_ADDR_LEN);
                }

                if (rpt_1_copy_data_len > 229)
                //              if(new_hold_data_len || old_hold_data_len)
                {
                    my_dump_str_u32s(DBG_EXTSCAN_LOGIC, "debug 10", old_hold_data_len, new_advdat_len, new_hold_data_len, rpt_1_copy_data_len);
                    my_dump_str_u32s(DBG_EXTSCAN_LOGIC, "debug 11", rpt_1_src_data_offset, rpt_2_src_data_offset, 0, 0);
                    BLMS_ERR_DEBUG(DBG_EXTSCAN_LOGIC, 0xFE0E0000);

                    rpt_1_copy_data_len = 229;
                }

                smemcpy(pExtAdvInfo->data + old_hold_data_len, pAuxAdv->data + rpt_1_src_data_offset, rpt_1_copy_data_len);
            } else { // Legacy ADV & Non_Connectable Non_Scannable without auxiliary packet

                pExtAdvInfo->primary_phy     = cur_pPrichnScn->prichn_phy;
                pExtAdvInfo->secondary_phy   = SECONDARY_PHY_NO_PACKET_ON_SECONDARY_ADV_CHN;
                pExtAdvInfo->advertising_sid = ADVERTISING_SID_NO_ADI_FIELD;
                if (advType_pkt == LL_TYPE_ADV_EXT_IND) { //Non_Connectable Non_Scannable without auxiliary packet
                    pExtAdvInfo->tx_power = cur_pPrichnScn->tx_power_rpt;
                } else {                                  //Legacy
                    pExtAdvInfo->tx_power = TX_POWER_INFO_NOT_AVAILABLE;
                }
                pExtAdvInfo->perd_adv_inter = PERIODIC_ADV_INTER_NO_PERIODIC_ADV;

                if (cur_pPrichnScn->direct_flag) {
                    u8 direct_adrType;
                    if (advType_pkt == LL_TYPE_ADV_EXT_IND) { //Non_Connectable Non_Scannable directed
                        /* if targetA exist, advA definitely exist, so data[6] is targetA */
                        direct_adrType = pPriExtAdv->rxAddr;
                        smemcpy(pExtAdvInfo->direct_address, &pPriExtAdv->data[6], BLE_ADDR_LEN);
                    } else { //ADV_DIRECT_IND
                        direct_adrType = pPriLegAdv->rxAddr;
                        smemcpy(pExtAdvInfo->direct_address, pPriLegAdv->data, BLE_ADDR_LEN);
                    }

                    pExtAdvInfo->direct_address_type = blt_extscan_convert_direct_adr_type(direct_adrType,
                                                                                           pExtAdvInfo->direct_address,
                                                                                           cur_pPrichnScn->directA_rpa_resolve_fail);
                } else { //no care, set 0 here
                    pExtAdvInfo->direct_address_type = 0;
                    smemset(pExtAdvInfo->direct_address, 0, BLE_ADDR_LEN);
                }


                if (advType_mask == TYPE_MASK_EXT_ADV) {
                    //TODO: SiHui & QiuWei checked 20220901 here "new_advdat_len" must be 0, "pPriExtAdv->data" is actually advA
                    smemcpy(pExtAdvInfo->data, pPriExtAdv->data, new_advdat_len); //for direct ADV, length is 0
                } else {
                    smemcpy(pExtAdvInfo->data, pPriLegAdv->data, new_advdat_len); //for direct ADV, length is 0
                }
            }

            //////////////////////////////////////////////////
            //TODO: duplicate filtering
            //if(blt_ll_filterAdvDevice (address_type, pPriAdv->advA))


            if (sec_chn_flag) {
                scan_secRxFifo.rptr++;
                pExtAdvInfo->event_type &= EXTADV_RPT_EVTTYPE_MASK; ///need to clear the high 3 bit.

                if (scanRx_flag & SCANRX_FLAG_LAST_DATA) {
                    last_adv_data_flag               = 1;
                    cur_pauxscn->advrpt_hold_dat_len = 0; //optimize: no need this, new aux_scan will reset, delete later

                    /* important: release AUX ADV table resource */
                    //add IRQ disable
                    cur_pauxscn->scan_rx_flag = 0;
                    blt_set_auxscan_enable(cur_pauxscn, 0); //pay attention: operate IRQ variable
                    //add IRQ restore

                    temp_buff[1] = num_of_report;                        ///temporary
                    pExtAdvInfo->event_type |= EXTADV_RPT_DATA_COMPLETE; ///default is complete
                } else {
                    temp_buff[1] = num_of_report;                        ///temporary
                    pExtAdvInfo->event_type |= EXTADV_RPT_DATA_INCOMPLETE_MORE_TO_COME;
                }
            } else {
                scan_priRxFifo.rptr++;
            }

        } //end of cur_rx_pkt_process


        if (sec_chn_flag && extadv_report) {
            u32 r = irq_disable();
            /*1. Here temp_buff has been set completely. event_type/num_report/address etc.
             *   so directly copy and prepare for truncate event if needed.
             *2. If not exist chain packet, the first packet is also the last packet.
             *   so the scanRx_flag will be set with (SCANRX_FLAG_FIRST_DATA|SCANRX_FLAG_LAST_DATA)
             *3. If exist chain packet, the first packet only be set with SCANRX_FLAG_FIRST_DATA.
             */
            if ((scanRx_flag & SCANRX_FLAG_FIRST_DATA) && !(scanRx_flag & SCANRX_FLAG_LAST_DATA)) {
                cur_pauxscn->scan_rx_flag |= SCANRX_FLAG_REPORT2HOST;
                smemcpy(&extAdv_truncatedEvt[aux_idx], temp_buff, EXTADV_TRUNCATED_EVT_SIZE); ///save 26bytes is enough. //pExtAdvInfo
            }
            irq_restore(r);
        }
        ///////////////////////////////////////////////////////////////////////////////////////

        if (extadv_report) {
            if (hci_le_eventMask & HCI_LE_EVT_MASK_EXTENDED_ADVERTISING_REPORT) {
                if (total_rptevt_len > 255) {
                    BLMS_ERR_DEBUG(DBG_EXTSCAN_LOGIC, 0xFE0F0000 | (total_rptevt_len & 0xFFFF));
                }

                temp_buff[1] = num_of_report;

                u8 report_status = blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, temp_buff, total_rptevt_len);
                my_dump_str_data(IUT_HCI_LOG_EN && DBG_EXTSCAN_REPORT, "@HCI LE Ext ADV Report Evt", temp_buff, total_rptevt_len);

                if (BLE_SUCCESS != report_status) {
                    //we can make sure not run here---if( blc_hci_isHciTxFIFOfull() ).
                    //so not need to process truncate. if run here, code is error,need to debug.
                    if (sec_chn_flag) { //primary scan report not care
                        my_dump_str_data(DBG_EXTSCAN_LOGIC, "[ext scn]ext adv event report fail", 0, 0);
                    }
                }

                /* special case: (old_hold_data_len + new_rx_data_len) >= 229*2 */
                if (rpt_2_exist) {
                    pExtAdvInfo->event_type |= EXTADV_RPT_DATA_INCOMPLETE_MORE_TO_COME;
                    pExtAdvInfo->data_length = EXTADV_RPT_DATA_LEN_MAX;
                    smemcpy(pExtAdvInfo->data, pAuxAdv->data + rpt_2_src_data_offset, EXTADV_RPT_DATA_LEN_MAX);

                    blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, temp_buff, 255);
                    my_dump_str_data(IUT_HCI_LOG_EN && DBG_EXTSCAN_REPORT, "@HCI LE Ext ADV Report Evt, rpt_2_exist", temp_buff, 255);

                    my_dump_str_data(DBG_EXTSCAN_LOGIC, "[ext scn]rpt exist", 0, 0);
                }

                /*special case: last RX, send hold data directly */
                if (last_adv_data_flag && new_hold_data_len) {
                    pExtAdvInfo->event_type |= EXTADV_RPT_DATA_COMPLETE;
                    pExtAdvInfo->data_length = new_hold_data_len;
                    smemcpy(pExtAdvInfo->data, pHolDataBuf, new_hold_data_len);

                    blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, temp_buff, EXTADV_INFO_LENGTH + new_hold_data_len);
                    my_dump_str_data(IUT_HCI_LOG_EN && DBG_EXTSCAN_REPORT, "@HCI LE Ext ADV Report Evt, send hold data", temp_buff, EXTADV_INFO_LENGTH + new_hold_data_len);

                    my_dump_str_data(DBG_EXTSCAN_LOGIC, "[ext scn]last packet,send hold data", 0, 0);
                    //BLMS_ERR_DEBUG(DBG_EXTSCAN_LOGIC, 0xFE080000);
                }
            }

            total_rptevt_len = 2; //clear
            num_of_report    = 0; //clear number of report
        }


        /* solve one problem: when manual_check_last is 1, IRQ may change scan_priRxFifo.wptr or scan_secRxFifo.wptr,
         * which lead to packet combine login error, here when check last is running, we break the "while" loop,
         * even new RX packet not processed */
        if (manual_check_last) {
            break;
        }
    }


    return 1;
}

_attribute_noinline_ int blt_extScan_timeoutEvt(void)
{
    if (!(hci_le_eventMask & HCI_LE_EVT_MASK_EXTENDED_SCAN_TIMEOUT)) {
        return 1;
    }

    if (bltScn.extScan_duration == 0) {
        return 1;
    }

    u8                       result[4]; //6 byte is enough
    hci_le_scanTimeoutEvt_t *pEvt = (hci_le_scanTimeoutEvt_t *)result;
    pEvt->subEventCode            = HCI_SUB_EVT_LE_SCAN_TIMEOUT;

    if (BLE_SUCCESS == blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_scanTimeoutEvt_t))) {
        return 0;
    }

    return 1;
}

    #if (SUB_INTERVAL_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #else
_attribute_noinline_
    #endif
    int
    blt_ll_procExtScan_periodExpired(void)
{
    if (bltScn.durationPeriod_stateFlag & EXTSCAN_DURATION_CHECK_PENDING) {
        //blc_ll_setExtScanEnable(0, 0, 0, 0);///has been done in duration over(irq)
        if (0 == blt_extScan_timeoutEvt()) {
            bltScn.durationPeriod_stateFlag &= ~EXTSCAN_DURATION_CHECK_PENDING;
        }
    }

    if ((bltScn.durationPeriod_stateFlag & EXTSCAN_PERIOD_CHECK_PENDING) && clock_time_exceed(bltScn.extScan_startTick, bltScn.extScan_period / SYSTICK_NUM_PER_US)) {
        bltScn.durationPeriod_stateFlag &= ~EXTSCAN_PERIOD_CHECK_PENDING;

        blc_ll_setExtScanEnable(1, bltScn.duplicate_filter, bltScn.extScan_duration, bltScn.extScan_period);
    }

    return 0;
}

_attribute_noinline_ int blt_ll_procAuxScanTruncatedPend(void)
{
    if (!(hci_le_eventMask & HCI_LE_EVT_MASK_EXTENDED_ADVERTISING_REPORT)) {
        return 0;
    }

    //process hold data. before send truncated event, need to send hold data to host.
    foreach (i, TSKNUM_SECCHN_SCAN) {
        if (!(secChnScn_tbl[i].scan_rx_flag & SCANRX_FLAG_EXTSCAN_TRUNCATED_PEND)) {
            continue;
        }

        //step 1: if there are hold data, firstly send hold data to host. then send truncated event.
        u8 extScn_holdDatLen = secChnScn_tbl[i].advrpt_hold_dat_len;
        if (extScn_holdDatLen) {
            u8 extScn_holdDatOffset = secChnScn_tbl[i].advrpt_holdDataOffset;
            do {
                u8 temp_buff[256];
                extScn_holdDatLen = (extScn_holdDatLen > EXTADV_RPT_DATA_LEN_MAX) ? EXTADV_RPT_DATA_LEN_MAX : extScn_holdDatLen;

                extAdvRptEvt_t *pExtAdvInfo = (extAdvRptEvt_t *)&temp_buff[0];
                u8             *pHolDataBuf = (u8 *)&extadv_pda_rpt_hold_data_buf[i][0];  //246B

                smemcpy(pExtAdvInfo, &extAdv_truncatedEvt[i], EXTADV_TRUNCATED_EVT_SIZE); //26
                pExtAdvInfo->subEventCode = HCI_SUB_EVT_LE_EXTENDED_ADVERTISING_REPORT;
                pExtAdvInfo->num_reports  = 1;
                pExtAdvInfo->event_type &= EXTADV_RPT_EVTTYPE_MASK;
                pExtAdvInfo->event_type |= EXTADV_RPT_DATA_INCOMPLETE_MORE_TO_COME;
                pExtAdvInfo->data_length = extScn_holdDatLen;

                smemcpy(pExtAdvInfo->data, pHolDataBuf + extScn_holdDatOffset, extScn_holdDatLen);

                if (0 == blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, (u8 *)&temp_buff, EXTADV_TRUNCATED_EVT_SIZE + extScn_holdDatLen)) {
                    if (extScn_holdDatLen > EXTADV_RPT_DATA_LEN_MAX) {
                        extScn_holdDatLen -= EXTADV_RPT_DATA_LEN_MAX;
                        secChnScn_tbl[i].advrpt_hold_dat_len   = extScn_holdDatLen;
                        secChnScn_tbl[i].advrpt_holdDataOffset = extScn_holdDatOffset = EXTADV_RPT_DATA_LEN_MAX;

                        my_dump_str_data(DBG_EXTSCAN_LOGIC, "[ext scn]holdData need 2 times", 0, 0);
                    } else {
                        secChnScn_tbl[i].advrpt_hold_dat_len = extScn_holdDatLen = 0;
                        secChnScn_tbl[i].advrpt_holdDataOffset = extScn_holdDatOffset = 0;
                        my_dump_str_data(DBG_EXTSCAN_LOGIC, "[ext scn]holdData need 1 time", 0, 0);
                    }
                } else {
                    //UART FIFO full, wait next mainloop to process. not run the following code,because next code also require UART FIFO.
                    return 1;
                }
            } while (extScn_holdDatLen);
        }
        //////////////////////////////////////////////////////////////
        //step 2: process truncated event

        extAdv_truncatedEvt[i].num_reports = 1;
        extAdv_truncatedEvt[i].data_length = 0; //clear adv report evt's dataLen
        extAdv_truncatedEvt[i].event_type &= EXTADV_RPT_EVTTYPE_MASK;
        extAdv_truncatedEvt[i].event_type |= EXTADV_RPT_DATA_INCOMPLETE_TRUNCATED;

        if (0 == blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, (u8 *)&extAdv_truncatedEvt[i], EXTADV_TRUNCATED_EVT_SIZE)) {
            secChnScn_tbl[i].scan_rx_flag = 0;                               //clear 0 after processing truncate.
            blt_set_auxscan_enable((st_secchn_scn_t *)&secChnScn_tbl[i], 0); ///release resource.
            //my_dump_str_data(0, "extScn pendReleaseData", &extAdv_truncatedEvt[i], EXTADV_TRUNCATED_EVT_SIZE);
            my_dump_str_data(DBG_EXTSCAN_LOGIC, "[ext scn]truncated pending release", 0, 0);
        }
    }

    return 1;
}

_attribute_noinline_ void blt_ll_ExtScan_mainloop_checkPendTask(void)
{
    // process extended scan report truncate event
    if (hci_le_eventMask & HCI_LE_EVT_MASK_EXTENDED_ADVERTISING_REPORT) {
        blt_ll_procAuxScanTruncatedPend();
    }

    // process periodic scan report truncate event
    if (hci_le_eventMask & HCI_LE_EVT_MASK_PERIODIC_ADVERTISING_REPORT) {
        blt_ll_procPdaScanTruncatedPend();
    }


    // process Extended scan duration & period
    blt_ll_procExtScan_periodExpired();
}

#if (LL_FEATURE_ENABLE_DECISION_BASED_ADVERTISING_FILTER)
void blt_set_decision_scan_default(void){

    smemset((u8*)&gDecisionInstructTest, 0, sizeof(decision_test_t));
//    gDecisionInstructTest.group_num = 0;
//    gDecisionInstructTest.num_test  = 0;
//
//    smemset(gDecisionInstructTest.testNum_perGroup, 0, DECISION_INSTRUCTION_SUPPORT_MAX_NUM);
}


ble_sts_t blc_ll_setDecisionInstructCmd(u8 num_test, dec_ins_t* pDec_test){

    if( (num_test == 0) || (pDec_test[0].test_flag&BIT(0) == 0) ){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if(num_test > DECISION_INSTRUCTION_SUPPORT_MAX_NUM){
        return HCI_ERR_LIMIT_REACHED; //now only support 8 tests.
    }

    if( (pDec_test[0].test_flag&BIT(0)) == 0){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    smemset((u8*)&gDecisionInstructTest, 0, sizeof(decision_test_t));
    gDecisionInstructTest.pTestData = (dec_ins_t *)&decison_instruct_test[0];

    gDecisionInstructTest.num_test = num_test;

    int i = 0;
    for(i=0; i<num_test; i++){

        gDecisionInstructTest.pTestData[i].test_flag = pDec_test[i].test_flag;
        gDecisionInstructTest.pTestData[i].test_field= pDec_test[i].test_field;
        smemcpy(gDecisionInstructTest.pTestData[i].test_param, pDec_test[i].test_param, 16);


        if(gDecisionInstructTest.pTestData[i].test_flag&BIT(0)){
            gDecisionInstructTest.group_num++;
        }

        if(gDecisionInstructTest.group_num < 1){
            //while(1); //todo
        }

        gDecisionInstructTest.testNum_perGroup[gDecisionInstructTest.group_num-1]++;
    }

    return BLE_SUCCESS;
}

#endif



#endif // end of LL_FEATURE_ENABLE_LE_EXTENDED_SCAN
