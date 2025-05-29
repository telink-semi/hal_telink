/********************************************************************************************************
 * @file    leg_scan.c
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

#if OS_SUP_EN
    #include "stack/ble/os_sup/os_sup.h"
    #include "stack/ble/os_sup/os_sup_stack.h"
#endif


#if (LL_FEATURE_SUPPORT_LE_LEGACY_SCANNING)

void blc_ll_initLegacyScanning_module(void)
{
    blt_ll_initScanningCommon();

    ll_leg_scan_mlp_task_cb = blt_leg_scan_mainloop_task;


    blt_set_leg_scan_default();
#if (PRICHN_SCAN_SMALL_INTV_WITH_ANOTHER_BIG_INTV_ADV_RESOLVE_EN)
    bltAdScn.legadv_int_thres = LEGADV_THRES;
    bltAdScn.extAdv_num_thres = 1;
    bltAdScn.scanTask_Policy  = 0X00;
#endif
}

void blt_set_leg_scan_default(void)
{
    //TODO, init some critical parameters, in case adv_param not set but SCAN enabled
}

_attribute_noinline_ void blt_reset_leg_scan(void)
{
    blmsParam.scanInitEn_union.leg_scan_en = 0;

    blt_set_scan_default();
    blt_set_leg_scan_default();
#if (PRICHN_SCAN_SMALL_INTV_WITH_ANOTHER_BIG_INTV_ADV_RESOLVE_EN)
    bltAdScn.legadv_int_thres = LEGADV_THRES;
    bltAdScn.extAdv_num_thres = 1;
    bltAdScn.scanTask_Policy  = 0X00;
#endif
}

ble_sts_t blc_ll_setScanParameter(scan_type_t scanType, scan_inter_t scan_interval, scan_wind_t scan_window, own_addr_type_t ownAddrType, scan_fp_type_t scan_fp)
{
    tlkapi_send_string_u32s(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_CMD), "[LL][CMD] Set_Scan_Param", scanType, scan_interval, scan_window, scan_fp);

    /* HCI/GEV/BV-03-C [Disallow Mixing Legacy and Extended Scanning Commands] */
    if (IS_EXTENDED_SCAN_VALID) {
        return HCI_ERR_CMD_DISALLOWED;
    } else {
        SET_LEGACY_SCAN_VALID;
    }

    if (scanType > SCAN_TYPE_ACTIVE || ownAddrType > OWN_ADDRESS_RESOLVE_PRIVATE_RANDOM || scan_fp > SCAN_FP_ALLOW_ADV_WL_DIRECT_ADV_MATCH ||
        scan_interval < 4 || scan_window < 4) //scan interval and scan window: range 0 to 0x4000(2.5ms to 10.24s)
    {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    /*
    The Host shall not issue this command when scanning is enabled in the
    Controller; if it is the Command Disallowed error code shall be used.
    */
    if (blmsParam.scanInitEn_union.leg_scan_en) {
        return HCI_ERR_CMD_DISALLOWED;
    }

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


    bltScn.scan_type               = (u8)scanType; //0 for passive scan; 1 for active scan
    bltScn.scan_filterPolicy       = (u8)scan_fp;  //need for leg_adv now, can optimize in ext_adv later
    bltScn.scan_fp_wl              = (scan_fp & SCAN_FP_WHITELIST_MASK) ? 1 : 0;
    bltScn.scan_fp_targetA_rpaPass = (scan_fp & SCAN_FP_DIRECT_RPA_PASS_MASK) ? 1 : 0;


#if (PRICHN_SCAN_SMALL_INTV_WITH_ANOTHER_BIG_INTV_ADV_RESOLVE_EN)
    //at least 20mS interval to save some SCNALIGN_FIFO_NUM
    if (scan_interval < SCAN_INTERVAL_20MS) {
        scan_interval *= 2;
        scan_window *= 2;
    }
#endif

    if ((int)scan_window > (int)scan_interval) {
        scan_window = (scan_wind_t)scan_interval;
    }
    bltScn.scan_percent     = (scan_window << 7) / scan_interval; // scan_window*128
    bltScn.scanInterval     = scan_interval;
    bltScn.scnInterval_tick = scan_interval * SYSTEM_TIMER_TICK_625US - 2000 * SYSTEM_TIMER_TICK_1US;


#if (DBG_PRVC_LEGSCAN_EN)
    if (scanType == SCAN_TYPE_PASSIVE) {
        my_dump_str_u8s(DBG_PRVC_LEGSCAN_EN, "[PRV][SCN] passive scan: Own_addr_type, filter policy", ownAddrType, scan_fp, 0, 0);
    } else {
        my_dump_str_u8s(DBG_PRVC_LEGSCAN_EN, "[PRV][SCN]active scan: Own_addr_type, filter policy", ownAddrType, scan_fp, 0, 0);
    }
#endif


    return BLE_SUCCESS;
}

ble_sts_t blc_ll_setScanEnable(scan_en_t scan_enable, dupFilter_en_t filter_duplicate)
{
    /* core_5.3
    If LE_Scan_Enable is set to 0x01, the scanning parameters' Own_Address_-
    Type parameter is set to 0x00 or 0x02, and the device does not have a public
    address, the Controller should return an error code which should be Invalid HCI
    Command Parameters (0x12).                                                           Ignore, controller always have public address

    If LE_Scan_Enable is set to 0x01, the scanning parameters' Own_Address_-
    Type parameter is set to 0x01 or 0x03, and the random address for the device
    has not been initialized using the HCI_LE_Set_Random_Address command,
    the Controller shall return the error code Invalid HCI Command Parameters
    (0x12).                                                                              Done

    If the LE_Scan_Enable parameter is set to 0x01 and scanning is already
    enabled, any change to the Filter_Duplicates setting shall take effect.

    Disabling scanning when it is disabled has no effect.
    */


    tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_CMD), "[LL][CMD] Set_Scan_Enable", &scan_enable, 1);

    /* HCI/GEV/BV-03-C [Disallow Mixing Legacy and Extended Scanning Commands] */
    if (IS_EXTENDED_SCAN_VALID) {
        return HCI_ERR_CMD_DISALLOWED;
    } else {
        SET_LEGACY_SCAN_VALID;
    }

    if (bltScn.scan_ownAddr_random && !(blmsParam.hci_cmd_mask & SET_RANDOM_ADDR_CMD_MASK)) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }


    if (scan_enable) {
        blt_ll_filterAdvDevice(filter_duplicate, 0);

#if (SCAN_BACKOFF_FEATURE_EN)
        //Upon entering the Scanning State, the upperLimit and backoffCount are set to one.
        bltScn.upperLimit   = 1;
        bltScn.backoffCount = 1;
#else
        blt_ll_clearScanRspDevice();
#endif
    } else {
        if (blc_rf_pa_cb) {
            blc_rf_pa_cb(PA_TYPE_OFF);
        }
    }

    //Important to disable IRQ
    u32 r = irq_disable();

    if (scan_enable != blmsParam.scanInitEn_union.leg_scan_en) {
        blmsParam.state_chng |= STATE_CHANGE_LEG_SCAN;
    }

    blmsParam.scanInitEn_union.leg_scan_en = (u8)scan_enable;

    irq_restore(r);

#if OS_SUP_EN
    if (blt_os_giveSem_cb) {
        blt_os_giveSem_cb();
    }
#endif

    return BLE_SUCCESS;
}

int blt_ll_procLegacyScanData(void)
{
    u32 advRptMask = (hci_le_eventMask & HCI_LE_EVT_MASK_ADVERTISING_REPORT);

    while (scan_priRxFifo.rptr != scan_priRxFifo.wptr) {
        /* when user set scan disable in main_loop, this can guarantee upper layer can not receive any ADV report packet */
        /* Fix one bug: maybe RX buffer hold some scan data, when user initiate a connection, controller should not
         * report ADV to Host*/
        if (blmsParam.scanInitEn_union.leg_scan_en && !blmsParam.scanInitEn_union.leg_init_en && advRptMask) {
            u8  raw_fifo_idx = scan_priRxFifo.rptr & SCAN_PRICHN_RXFIFO_MASK;
            u8 *raw_pkt      = NULL;
            /*
             * when the FIFO will be full,copy the FIFO data to a temporary buffer.note: need to use irq protection like the following code.
             * and use the temporary buffer to process report. that can be sure when rptr is changed in irq and all data is right.
             * For detailed explanation, please refer to the description "Scan Data Rf_len error            Summarized by SiHui 20221105"
             */
            if (((u8)(scan_priRxFifo.wptr - scan_priRxFifo.rptr) & 63) >= (SCAN_PRICHN_RXFIFO_NUM - 2)) {
                u8 tLegAdvRawData[SCAN_PRICHN_RXFIFO_SIZE]; //not use 37+4,consider kite software crc and the following code use rssi etc.

                u32 r = irq_disable();                      //The execution time here to irq_restore() under the 48M system clock is 8.58us

                raw_fifo_idx = scan_priRxFifo.rptr & SCAN_PRICHN_RXFIFO_MASK;
                raw_pkt      = (u8 *)(scan_priRxFifo.p + SCAN_PRICHN_RXFIFO_SIZE * raw_fifo_idx);

                /*Copy all raw data for software crc calculate and the following code may use to other information(rssi).
                  so not use this smemcpy(xxx, yyy, raw_pkt[5]+6).*/
                smemcpy(tLegAdvRawData, raw_pkt, SCAN_PRICHN_RXFIFO_SIZE);

                irq_restore(r);

                raw_pkt = tLegAdvRawData;
            } else {
                raw_pkt = (u8 *)(scan_priRxFifo.p + SCAN_PRICHN_RXFIFO_SIZE * raw_fifo_idx);
            }


#if (BQB_TEST_EN)
            st_prichn_scn_t *cur_pPrichnScn = (st_prichn_scn_t *)&priChnScn_tbl[raw_fifo_idx];
#endif


            //ADV type & rf_len have already checked by RX IRQ
            rf_packet_adv_t *pAdvPkt = (rf_packet_adv_t *)(raw_pkt + DMA_RFRX_LEN_HW_INFO);
            u8               rf_len  = pAdvPkt->rf_len;


/* Scan Data Rf_len error            Summarized by SiHui 20221105
             * 1. Kite IC
             *   (1.1) for very old code of Kite(hardware CRC24 bug), we do not use software CRC24 check in IRQ when receive a new data,
             *        data reported here maybe error, including rf_len error(not in correct range of 6~37).
             *   (1.2) We add rf_len check when report, can drop only a small part of error, but can not detect all error data
             *   (1.3) After rf_len check, we add a software CRC24 check, can drop all error data
             * 2. rf_len check is necessary for all IC & SDK, we move rf_len check to RX IRQ, to drop error data earlier, can save our code
             *    running time and improving code efficiency.  Error rf_len in IRQ include some situation as below:
             *   (2.1) receive a correct data whose rf_len is out of legacy ADV range(6~37), maybe a hacker attack or maybe a un_standard BLE
             *         design data in common ADV channel.   Actually if we set RX max length by "rf_set rx_maxlen" in advance, rf_len bigger
             *         than 37 can be rejected by hardware.
             *   (2.2) for Kite only. peer device send correct data, RX error, but give a correct CRC due to CRC24 bug
             * 3. Even all issues resolved above, another case may lead rf_len error here when reporting in MainLoop code:
             *   (3.1) If scan RX FIFO number is 8, e.g. FIFO_0 ~ FIFO7, when wptr is 7, rptr is 0, now new RX data will fill in FIFO_7.
             *         When this new data come, if we want to use overwrite oldest FIFO in IRQ, but do not processed correctly, setting new
             *         RX DMA for next FIFO but not changing rptr(our old code). then wptr will 8, new RX data dma address will point to FIFO_0.
             *         If code run to RX data report now, first reported data will be FIFO_0 because rptr is 0. Maybe at this moment RX is
             *         ongoing, FIFO_0 data is changed by hardware RX dma action. So here we may report a error data to host.
             *         rf_len error is just one situation of what described above, this problem is reported GaoQiu. The root reason is hardware
             *         RX DMA point to a un_processed data.
             *   (3.2) To solve problem (3.1), we can use a correct overwrite oldest FIFO code, or abandon new RX data when RX FIFO is full.
             *         Consider that abandon new data is hard to control: our code now(20221104) want to abandon newest RX data, but actually
             *         drop the second new data.  So I advice to use correct overwrite oldest FIFO.
             *         (Qiuwei 20221121) Now we use the method of overwrite the oldest FIFO.
             *         When FIFO is full, for example, rptr point to FIFO_0, wptr point to FIFO_0.
             *         rptr will be ++ if wptr++ in "primary channel RX IRQ". i.e. rptr will point the next fifo---FIFO_1 and wptr point FIFO_0.
             *         the oldest FIFO(FIFO_0) data will has two situation in mainloop:
             *         case 1: the oldest FIFO(FIFO_0) data is not still processed. this situation the data will be abandoned(rptr point to the next fifo--FIFO_1),mainloop get the data by rptr.
             *         case 2: the oldest FIFO(FIFO_0) data is being processed. we copy the oldest FIFO(FIFO_0) data to the temporary buffer that make sure RF rx not affect the data.
             */
#if 0 //refer to above description (3.1).
      //These code is not needed if RX IRQ processed correctly, but do not delete it, for warning our member.
                if(rf_len > 37 || rf_len < 6){ //debug
                    BLMS_ERR_DEBUG(DBG_EXTSCAN_LOGIC, 0xFE110000 | rf_len); //GaoQiu has encountered this
                    scan_priRxFifo.rptr ++;
                    continue;
                }
#endif


            u8 report_adv_type = raw_pkt[DMA_RFRX_OFFSET_HEADER] >> 6; // 0/1/2/3, have processed in IRQ

            if (pAdvPkt->type == LL_TYPE_SCAN_RSP) {
                ;
            } else if (blt_ll_filterAdvDevice((report_adv_type & PEERATYPE_RANDOM_MASK), pAdvPkt->advA)) {
                scan_priRxFifo.rptr++;
                continue;
            }


            s8 rssi = (s8)raw_pkt[DMA_RFRX_OFFSET_RSSI(raw_pkt)] - 110 + blt_ll_getRfRxPathComp();
#if (LL_FEATURE_ENABLE_MONITORING_ADVERTISERS)
            if (ll_mon_adv_mlp_task_cb && mon_adv_en) {
                s8 adv_info[8] = {report_adv_type, rssi};
                smemcpy(adv_info+2, pAdvPkt->advA, 6);
                ll_mon_adv_mlp_task_cb(FLAG_MON_ADV_DATA_REPORT_LEGADV, adv_info);  //blt_mon_adv_mainloop_task
            }
#endif

#if (BQB_TEST_EN)
            /*
                The HCI_LE_Directed_Advertising_Report event indicates that directed
                advertisements have been received where the advertiser is using a resolvable
                private address for the TargetA field of the advertising PDU which the
                Controller is unable to resolve and the Scanning_Filter_Policy is equal to 0x02
                or 0x03
                */
            if (cur_pPrichnScn->directA_rpa_resolve_fail) {
                hci_le_directAdvertisingReport_evt(report_adv_type, pAdvPkt->advA, pAdvPkt->data, rssi);
            } else
#endif
            {
                u8                  temp_buff[48]; //14 + 31
                event_adv_report_t *pAdvRpt = (event_adv_report_t *)temp_buff;

                pAdvRpt->subcode = HCI_SUB_EVT_LE_ADVERTISING_REPORT;
                pAdvRpt->nreport = 1;


                //note that: adv report event type value different from adv type
                if (pAdvPkt->type == LL_TYPE_ADV_NONCONN_IND) {
                    pAdvRpt->event_type = ADV_REPORT_EVENT_TYPE_NONCONN_IND;
                } else if (pAdvPkt->type == LL_TYPE_ADV_SCAN_IND) {
                    pAdvRpt->event_type = ADV_REPORT_EVENT_TYPE_SCAN_IND;
                } else { //ADV_IND/ADV_DIRECT_IND/SCAN_RSP, adv_type value same as ADV report event type value
                    pAdvRpt->event_type = pAdvPkt->type;

                    //note that: direct_adv should not report data[0..5](initA)
                    if (pAdvPkt->type == LL_TYPE_ADV_DIRECT_IND) {
                        rf_len -= 6;
                    }
                }


                pAdvRpt->adr_type = report_adv_type; // 0/1/2/3, have set in IRQ
                smemcpy(pAdvRpt->mac, pAdvPkt->advA, BLE_ADDR_LEN);
                pAdvRpt->len = rf_len - 6;           //6: length of advAddress
                smemcpy(pAdvRpt->data, pAdvPkt->data, pAdvRpt->len);
                pAdvRpt->data[pAdvRpt->len] = rssi;

                blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, (u8 *)pAdvRpt, pAdvRpt->len + 12);


#if (DBG_PRVC_LEGSCAN_EN)
                my_dump_str_data(DBG_PRVC_LEGSCAN_EN, "[PRV][SCN] adv report", pAdvRpt, pAdvRpt->len + 12);
                if (pAdvRpt->adr_type & PEERATYPE_IDENTITY_MASK) {
                    my_dump_str_data(DBG_PRVC_LEGSCAN_EN, "[PRV][SCN] legscan report IDA", &pAdvRpt->adr_type, 7);
                } else {
                    my_dump_str_data(DBG_PRVC_LEGSCAN_EN, "[PRV][SCN] legscan report", &pAdvRpt->adr_type, 7);
                }
#endif
            }
        }


        scan_priRxFifo.rptr++;
    }


    return 1;
}

ble_sts_t blc_hci_le_setScanEnable(hci_le_setScanEnable_cmdParam_t *pCmdParam)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Set_Scan_Enable", pCmdParam, sizeof(hci_le_setScanEnable_cmdParam_t));

    return blc_ll_setScanEnable(pCmdParam->le_scan_enable, pCmdParam->filter_duplicate);
}

ble_sts_t blc_hci_le_setScanParameter(hci_le_setScanParam_cmdParam_t *pCmdParam)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Set_Scan_Param", pCmdParam, sizeof(hci_le_setScanParam_cmdParam_t));

    return blc_ll_setScanParameter(pCmdParam->le_scanType, pCmdParam->le_scanInterval, pCmdParam->le_scanWindow, pCmdParam->ownAddrType, pCmdParam->scanFilterPolicy);
}

#if (PRICHN_SCAN_SMALL_INTV_WITH_ANOTHER_BIG_INTV_ADV_RESOLVE_EN)
ble_sts_t blc_ll_setScanIntervalOptimizeParameter(adv_inter_t Interval_Min_thres, u8 Adv_num_thres, u8 scanTask_Policy)
{
    u32 r                     = irq_disable();
    bltAdScn.legadv_int_thres = Interval_Min_thres;
    bltAdScn.extAdv_num_thres = Adv_num_thres;
    bltAdScn.scanTask_Policy  = scanTask_Policy;
    irq_restore(r);
    return BLE_SUCCESS;
}
#endif


int blt_leg_scan_mainloop_task(int flag)
{
    if (flag == FLAG_SCAN_DATA_REPORT) {
        blt_ll_procLegacyScanData();
    } else if (flag == FLAG_MODULE_RESET) {
        blt_reset_leg_scan();
    }


    return 0;
}

#endif  /*!< LL_FEATURE_SUPPORT_LE_LEGACY_SCANNING */

