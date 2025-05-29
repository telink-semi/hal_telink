/********************************************************************************************************
 * @file    ext_init.c
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


#if (LL_FEATURE_ENABLE_LE_EXTENDED_INITIATE && LL_ACL_CEN_EN)


_attribute_aligned_(4) ll_extinit_t bltExtInit;

void blc_ll_initExtendedInitiating_module(void)
{
    #if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_init_t)), ext_init);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_extinit_t)), ext_init);
    #endif

    blc_ll_initChannelSelectionAlgorithm_2_feature(); //need CSA #2

    blt_ll_initInitiatingCommon();

    ll_secchn_initPkt_cb    = blt_secchn_procInitPkt;
    ll_ext_init_irq_task_cb = blt_ext_init_interrupt_task;

    blmsParam.extInitModule_en = 1;
}

//u16 pkt_auxConnReq_us[5] = {0, AUX_CONN_REQ_1MPHY_US, AUX_CONN_REQ_2MPHY_US, AUX_CONN_REQ_CODEDPHY_S2_US, AUX_CONN_REQ_CODEDPHY_S8_US};
//u16 pkt_auxConnRsp_us[5] = {0, AUX_CONN_RSP_1MPHY_US, AUX_CONN_RSP_2MPHY_US, AUX_CONN_RSP_CODEDPHY_S2_US, AUX_CONN_RSP_CODEDPHY_S8_US};


_attribute_ram_code_ int blt_ext_init_interrupt_task(int flag)
{
    if (flag == FLAG_SCHEDULE_EXTINIT_CHECK_CONNRSP) {
    #if 0
            u8 index = bltPHYs.cur_llPhy;
            if(bltPHYs.cur_llPhy == BLE_PHY_CODED){
                index += (bltPHYs.cur_own_CI == LE_CODED_S8 ? 1: 0);
            }
            u32 diff1_us = pkt_auxConnReq_us[index] + 150 + 20; //add 20uS margin

            index = bltPHYs.cur_llPhy;
            if(bltPHYs.cur_llPhy == BLE_PHY_CODED){
                index += (bltPHYs.cur_peer_CI == LE_CODED_S8 ? 1: 0);
            }
            u32 diff2_us = diff1_us + pkt_auxConnRsp_us[index] + 150 + 20;  //add 20uS margin, total 40uS margin
    #else
        u32 diff1_us;
        if (bltPHYs.cur_llPhy == BLE_PHY_CODED) {
            diff1_us = bltPHYs.cur_own_CI == LE_CODED_S8 ? AUX_CONN_REQ_CODEDPHY_S8_US : AUX_CONN_REQ_CODEDPHY_S2_US;
        } else {
            diff1_us = bltPHYs.cur_llPhy == BLE_PHY_1M ? AUX_CONN_REQ_1MPHY_US : AUX_CONN_REQ_2MPHY_US;
        }
        diff1_us += (150 + 20); //add 20uS margin

        u32 diff2_us;
        if (bltPHYs.cur_llPhy == BLE_PHY_CODED) {
            diff2_us = bltPHYs.cur_peer_CI == LE_CODED_S8 ? AUX_CONN_RSP_CODEDPHY_S8_US : AUX_CONN_RSP_CODEDPHY_S2_US;
        } else {
            diff2_us = bltPHYs.cur_llPhy == BLE_PHY_1M ? AUX_CONN_RSP_1MPHY_US : AUX_CONN_RSP_2MPHY_US;
        }
        diff2_us += diff1_us + (150 + 20); //add 20uS margin
    #endif

        /* wait TX done
         * must add timeout check for RF status traverse, to avoid potential risk */
        //DBG_C HN11_HIGH;
        while (!HAL_GET_RF_TX_IRQ && !clock_time_exceed(bltRxPkt.rx_irq_tick, diff1_us)) {
            if (usr_irq_handler_cb) {
                usr_irq_handler_cb();
            }
        }

        if (blc_rf_pa_cb) {
            blc_rf_pa_cb(PA_TYPE_RX_ON);
        }

        //DBG_C HN11_LOW;
        if (HAL_GET_RF_TX_IRQ) {
            HAL_CLEAR_RF_TX_IRQ;
        } else {
            //my_dump_str_data(0, "TX sending error", 0, 0);
            if (blc_rf_pa_cb) {
                blc_rf_pa_cb(PA_TYPE_OFF);
            }
            return 0; //TX sending error
        }

        int conn_rsp_correct = 0;

        //DBG_C HN14_HIGH;
        while (1) {
            if (HAL_GET_RF_RX_IRQ) {
                u8 *tmp_connRspBuf = glb_temp_rx_buff;            //remove warning
                if (RF_BLE_PACKET_VALIDITY_CHECK(tmp_connRspBuf)) //if(RF_BLE_PACKET_VALIDITY_CHECK(glb_temp_rx_buff))
                {
                    rf_pkt_aux_conn_rsp_t *pConnRsp = (rf_pkt_aux_conn_rsp_t *)(glb_temp_rx_buff + DMA_RFRX_LEN_HW_INFO);
                    if (pConnRsp->rf_len == 14 && pConnRsp->adv_mode == LL_EXTADV_MODE_NON_CONN_NON_SCAN && pConnRsp->ext_hdr_len == 13 && pConnRsp->ext_hdr_flg == (EXTHD_BIT_ADVA | EXTHD_BIT_TARGETA)) {
    #if (LL_ASYNC_LEA_EN)
                        if (bltScn.asyncScanIndex) {
                            DBG_TIANXIANG_CHN7_HIGH;
                            DBG_TIANXIANG_CHN7_LOW;
                            u32        aclApTick        = 0;
                            u32        cisAPTick        = 0;
                            u32        cisSyncDelayTick = 0;
                            u32        rx_header_tick   = hal_rf_get_rx_timestamp();
                            extern u32 btx_anchor_point;
                            u32        connWindowStartTick = BSLOT_ABS_2_TICKS_ABS(btx_anchor_point) - BSLOT_NUM_HALF_WINSIZE * SYSTEM_TIMER_TICK_625US;
                            aclApTick                      = (u32)pConnRsp->advA[0] | (u32)pConnRsp->advA[1] << 8 | (u32)pConnRsp->advA[2] << 16 | (u32)pConnRsp->advA[3] << 24;

                            cisAPTick = (u32)pConnRsp->advA[4] | (u32)pConnRsp->advA[5] << 8 | (u32)pConnRsp->targetA[0] << 16 | (u32)pConnRsp->targetA[1] << 24;

                            cisSyncDelayTick = (u32)pConnRsp->targetA[2] | (u32)pConnRsp->targetA[3] << 8 | (u32)pConnRsp->targetA[4] << 16 | (u32)pConnRsp->targetA[5] << 24;
                            blt_async_savePeerTimingInfo(connWindowStartTick, rx_header_tick, aclApTick, cisAPTick, cisSyncDelayTick);
                            blt_async_calWindowSizeOffset();
                            btx_anchor_point                    = btx_anchor_point - BSLOT_NUM_HALF_WINSIZE + asyncCtrl.windowSizeOffsetBslot;
                            aclMas_param.bSlot_mark_position[0] = btx_anchor_point;
                        }
    #endif

                        //EBQ test ext_init rpa cases all pass: check OK by tyf
                        //LL/CON/CEN/BV-84-C, LL/CON/CEN/BV-102-C, LL/CON/CEN/BV-152-C, LL/CON/CEN/BV-154-C ~ LL/CON/CEN/BV-156-C
                        if (!smemcmp(pConnRsp->targetA, pkt_init.initA, BLE_ADDR_LEN)) {
                            int conn_rsp_adv_correct = 0;
                            if (bltAddr.peer_use_rpa) {
                                if (blt_ll_resolve_rpa(0, pConnRsp->advA, bltInit.pRslvlst_extInit)) {
                                    conn_rsp_adv_correct = 1;
                                }
                            } else {
                                conn_rsp_adv_correct = !smemcmp(pConnRsp->advA, bltAddr.peer_pka_or_ida_addr, BLE_ADDR_LEN);
                            }

                            if (conn_rsp_adv_correct) {
                                conn_rsp_correct = 1;
                                my_dump_str_data(DBG_PRVC_INIT_EN, "[PRV][INI] aux_con_rsp check passed", 0, 0);
                            } else {
                                my_dump_str_data(DBG_PRVC_INIT_EN, "[PRV][INI] aux_con_rsp advA fail", pConnRsp->advA, 6);
                            }
                        } else {
                            my_dump_str_data(DBG_PRVC_INIT_EN, "[PRV][INI] aux_con_rsp targetA not match", pConnRsp->targetA, 6);
                        }
                    }
                }

                break;
            }

            if (reg_rf_irq_status & FLD_RF_IRQ_RX_TIMEOUT) {
                my_dump_str_data(0 || DBG_EXTSCAN_TIMING, "AUX_CONN_RSP RX timeout", 0, 0);
                break;
                ; //RX fail
            }

            /* must add timeout check for RF status traverse, to avoid potential risk */
            if (clock_time_exceed(bltRxPkt.rx_irq_tick, diff2_us)) {
                //DBG_C HN15_TOGGLE;
                my_dump_str_data(0 || DBG_EXTSCAN_TIMING, "AUX_CONN_RSP software timeout", 0, 0);
                break;
            }

            if (usr_irq_handler_cb) {
                usr_irq_handler_cb();
            }
        }
        if (blc_rf_pa_cb) {
            blc_rf_pa_cb(PA_TYPE_OFF);
        }
        //DBG_C HN14_LOW;
    #if (LL_ASYNC_LEA_EN)
        conn_rsp_correct = 1;
    #endif
        return conn_rsp_correct;
    }

    return 0;
}

ble_sts_t blc_ll_extended_createConnection(init_fp_t filter_policy, own_addr_type_t ownAdrType, u8 peerAdrType, u8 *peerAddr, init_phy_t init_phys, scan_inter_t scanInter_0, scan_wind_t scanWindow_0, conn_inter_t conn_min_0, conn_inter_t conn_max_0, conn_tm_t timeout_0, scan_inter_t scanInter_1, scan_wind_t scanWindow_1, conn_inter_t conn_min_1, conn_inter_t conn_max_1, conn_tm_t timeout_1, scan_inter_t scanInter_2, scan_wind_t scanWindow_2, conn_inter_t conn_min_2, conn_inter_t conn_max_2, conn_tm_t timeout_2)
{
    tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_CMD), "[LL][CMD] Ext_Create_Conn", peerAddr, 6);
    //my_dump_str_u8s(BLC_LL_LOG_EN, "@BLC_LL_Ext_Create_Conn", filter_policy, ownAdrType, conn_min_1, conn_max_1);

    /* some code use "hci_cmd_mask" even without HCI, so handle it here */
    /* HCI/GEV/BV-03-C [Disallow Mixing Legacy and Extended Scanning Commands] */
    if (IS_LEGACY_SCAN_VALID) {
        return HCI_ERR_CMD_DISALLOWED;
    } else {
        SET_EXTENDED_SCAN_VALID;
    }

    /*If the Own_Address_Type parameter is set to 0x01 and the random address for
      the device has not been initialized using the HCI_LE_Set_Random_Address command,
      the Controller shall return the error code Invalid HCI Command Parameters (0x12)
      HCI/CCO/BI-56-C
     */
    if ((ownAdrType & 0x01) && !(blmsParam.hci_cmd_mask & SET_RANDOM_ADDR_CMD_MASK)) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    u8 ret_status = (u8)blt_ll_createConnCommon(filter_policy, ownAdrType, peerAdrType, peerAddr);
    if (ret_status != BLE_SUCCESS) {
        return ret_status;
    }

    /* When code running here, initiation will start */


    /* scan_interval and scan_window for different PHYs should be hold in group, decide to use which value according to current scanning PHY */
    /* conn_interval and conn_time for different PHYs should be hold in group, decide to use which value according to current scanning PHY */
    //remember to set pkt_init.interval when send "AUX_CONNECT_REQ"
    //remember to set   pkt_init.timeout when send "AUX_CONNECT_REQ"
    //remember set new master_interval with "blc_ll_setAclCentralBaseConnectionInterval" if this need change(blmsParam.max_master_num is 1)
    bltScn.scanPhy_msk = init_phys & INIT_PHY_1M_CODED; //bltScn.scanPhy_msk is only used for primary channel scan

    u8 first_set_idx   = 0xFF;
    u8 set_flag_idx[3] = {0};

    if (init_phys & INIT_PHY_1M) {
        if (first_set_idx == 0xFF) {
            first_set_idx = 0;
        }
        set_flag_idx[0] = 1;

        if ((int)scanWindow_0 > (int)scanInter_0) {
            scanWindow_0 = (scan_wind_t)scanInter_0;
        }
        bltScn.scanPercent[0]   = (scanWindow_0 << 7) / scanInter_0; // scan_window*128
        bltScn.scanInter[0]     = scanInter_0;
        bltScn.scanInte_tick[0] = scanInter_0 * SYSTEM_TIMER_TICK_625US - 1000 * SYSTEM_TIMER_TICK_1US;


        bltExtInit.mas_hold_intv_mul[0] = blt_init_calculateMasterIntervalMultiplier(aclMas_param.master_connInter, conn_min_0, conn_max_0);
        if (bltExtInit.mas_hold_intv_mul[0] == 24) {
            bltExtInit.mas_hold_intv_msk[0] = INTV_MSK_24_TIME;
        } else {
            bltExtInit.mas_hold_intv_msk[0] = interMask_tbl[bltExtInit.mas_hold_intv_mul[0]];
        }


        //my_dump_str_u8s(0, "AAAAAAAA", aclMas_param.master_connInter, conn_min_0, conn_max_0, bltExtInit.mas_hold_intv_mul[0]);

    #if (IMPROVE_MASTER_INTERVAL)
        u16 conn_inter_use = aclMas_param.master_connInter * bltExtInit.mas_hold_intv_mul[0];
        if (conn_min_0 <= conn_inter_use && conn_max_0 >= conn_inter_use) { //totally meet host's requirement
            //do nothing
        } else { //not exactly host's requirement
            if (conn_min_0 > aclMas_param.master_connInter) {
                int mod        = conn_max_0 % aclMas_param.master_connInter;
                u16 conn_inter = conn_max_0 - mod;
                if (conn_inter >= conn_min_0) {
                    bltExtInit.mas_hold_intv_mul[0] = conn_inter / aclMas_param.master_connInter;
                    bltExtInit.mas_hold_intv_msk[0] = 0xFFFFFF;

                    //my_dump_str_u8s(0, "BBBBBBBBB", aclMas_param.master_connInter, conn_min_0, conn_max_0, bltExtInit.mas_hold_intv_mul[0]);
                }
            }
        }
    #endif


        bltExtInit.hold_timeout[0] = timeout_0; //hold timeout


        /* extended create connection for a legacy ADV:
         * use extended create connection command, but the device is sending ADV on primary channel */
        bltInit.mas_intv_mul = bltExtInit.mas_hold_intv_mul[0];
        bltInit.mas_intv_msk = bltExtInit.mas_hold_intv_msk[0];
        pkt_init.interval    = aclMas_param.master_connInter * bltInit.mas_intv_mul;
        pkt_init.timeout     = timeout_0;
        /* extended create connection for a legacy ADV */

        //my_dump_str_u8s(0, "AAAAAAAAA", aclMas_param.master_connInter, conn_min_0, conn_max_0, bltExtInit.mas_hold_intv_mul[0]);
        //my_dump_str_u32s(0, "BBBBBBBBB", bltExtInit.mas_hold_intv_mul[0], bltExtInit.mas_hold_intv_msk[0], 0, 0);
    }
    if (init_phys & INIT_PHY_2M) {
        if (first_set_idx == 0xFF) {
            first_set_idx = 1;
        }
        set_flag_idx[1] = 1;

        if ((int)scanWindow_1 > (int)scanInter_1) {
            scanWindow_1 = (scan_wind_t)scanInter_1;
        }
        bltScn.scanPercent[1]   = (scanWindow_1 << 7) / scanInter_1; // scan_window*128
        bltScn.scanInter[1]     = scanInter_1;
        bltScn.scanInte_tick[1] = scanInter_1 * SYSTEM_TIMER_TICK_625US - 1000 * SYSTEM_TIMER_TICK_1US;


        bltExtInit.mas_hold_intv_mul[1] = blt_init_calculateMasterIntervalMultiplier(aclMas_param.master_connInter, conn_min_1, conn_max_1);
        if (bltExtInit.mas_hold_intv_mul[1] == 24) {
            bltExtInit.mas_hold_intv_msk[1] = INTV_MSK_24_TIME;
        } else {
            bltExtInit.mas_hold_intv_msk[1] = interMask_tbl[bltExtInit.mas_hold_intv_mul[1]];
        }


    #if (IMPROVE_MASTER_INTERVAL)
        u16 conn_inter_use = aclMas_param.master_connInter * bltExtInit.mas_hold_intv_mul[1];
        if (conn_min_1 <= conn_inter_use && conn_max_1 >= conn_inter_use) { //totally meet host's requirement
            //do nothing
        } else { //not exactly host's requirement
            if (conn_min_1 > aclMas_param.master_connInter) {
                int mod        = conn_max_1 % aclMas_param.master_connInter;
                u16 conn_inter = conn_max_1 - mod;
                if (conn_inter >= conn_min_1) {
                    bltExtInit.mas_hold_intv_mul[1] = conn_inter / aclMas_param.master_connInter;
                    bltExtInit.mas_hold_intv_msk[1] = 0xFFFFFF;
                }
            }
        }
    #endif


        bltExtInit.hold_timeout[1] = timeout_1; //hold timeout
    }
    if (init_phys & INIT_PHY_CODED) {
        if (first_set_idx == 0xFF) {
            first_set_idx = 2;
        }
        set_flag_idx[2] = 1;

        if ((int)scanWindow_2 > (int)scanInter_2) {
            scanWindow_2 = (scan_wind_t)scanInter_2;
        }
        bltScn.scanPercent[2]   = (scanWindow_2 << 7) / scanInter_2; // scan_window*128
        bltScn.scanInter[2]     = scanInter_2;
        bltScn.scanInte_tick[2] = scanInter_2 * SYSTEM_TIMER_TICK_625US - 1000 * SYSTEM_TIMER_TICK_1US;


        bltExtInit.mas_hold_intv_mul[2] = blt_init_calculateMasterIntervalMultiplier(aclMas_param.master_connInter, conn_min_2, conn_max_2);
        if (bltExtInit.mas_hold_intv_mul[2] == 24) {
            bltExtInit.mas_hold_intv_msk[2] = INTV_MSK_24_TIME;
        } else {
            bltExtInit.mas_hold_intv_msk[2] = interMask_tbl[bltExtInit.mas_hold_intv_mul[2]];
        }


    #if (IMPROVE_MASTER_INTERVAL)
        u16 conn_inter_use = aclMas_param.master_connInter * bltExtInit.mas_hold_intv_mul[2];
        if (conn_min_2 <= conn_inter_use && conn_max_2 >= conn_inter_use) { //totally meet host's requirement
            //do nothing
        } else { //not exactly host's requirement
            if (conn_min_2 > aclMas_param.master_connInter) {
                int mod        = conn_max_2 % aclMas_param.master_connInter;
                u16 conn_inter = conn_max_2 - mod;
                if (conn_inter >= conn_min_2) {
                    bltExtInit.mas_hold_intv_mul[2] = conn_inter / aclMas_param.master_connInter;
                    bltExtInit.mas_hold_intv_msk[2] = 0xFFFFFF;
                }
            }
        }
    #endif

        bltExtInit.hold_timeout[2] = timeout_2; //hold timeout
    }


    /*
    Where the connection is made on a PHY whose bit is not set in the Initiating_-PHYs parameter, the Controller shall use the Connection_Interval_Min[i],
    Connection_Interval_Max[i], Max_Latency[i], Supervision_Timeout[i],
    Min_CE_Length[i], and Max_CE_Length[i] parameters for an implementation-chosen PHY whose bit is set in the Initiating_PHYs parameter.
    */
    if (first_set_idx == 0xFF) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    for (int i = 0; i < 3; i++) {
        if (set_flag_idx[i] == 0) {
            bltScn.scanPercent[i]           = bltScn.scanPercent[first_set_idx];
            bltScn.scanInter[i]             = bltScn.scanInter[first_set_idx];
            bltScn.scanInte_tick[i]         = bltScn.scanInte_tick[first_set_idx];
            bltExtInit.mas_hold_intv_mul[i] = bltExtInit.mas_hold_intv_mul[first_set_idx];
            bltExtInit.mas_hold_intv_msk[i] = bltExtInit.mas_hold_intv_msk[first_set_idx];
            bltExtInit.hold_timeout[i]      = bltExtInit.hold_timeout[first_set_idx];
        }
    }


    if (filter_policy == INITIATE_FP_ADV_SPECIFY) {
        //TODO: check if current device is in scan ADV device table, to see event_type, decide to send conn_req or aux_conn_req
    }

    u32 r = irq_disable(); //very important to disable IRQ

    if (blmsParam.scanInitEn_union.ext_scan_en) {
        blmsParam.create_connection = CONNECT_REQ_EXT_PENDING;
    } else {
        /* if scan not enabled, */
        blmsParam.state_chng |= STATE_CHANGE_INIT;
        blmsParam.create_connection = CONNECT_REQ_GOING;
        bltScn.initiate_going       = EXT_INITIATE_GOING;
    }

    irq_restore(r);


    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_extended_createConnection(hci_le_ext_createConn_cmdParam_t *pCmdParam)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Extended_Create_Connection", pCmdParam, sizeof(hci_le_ext_createConn_cmdParam_t));

    /*If the Host specifies a PHY that is not supported by the Controller,
    including a bit that is reserved for future use,
    the latter should return the error code Unsupported Feature or Parameter Value (0x11).*/
    if (pCmdParam->init_PHYs & (~INIT_PHY_1M_2M_CODED)) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    /*If the Initiating_PHYs parameter does not have at least one bit set for a PHY
    allowed for scanning on the primary advertising physical channel, the
    Controller shall return the error code Invalid HCI Command Parameters (0x12).
    primary scan only support 1M PHY and coded PHY*/
    if ((pCmdParam->init_PHYs & INIT_PHY_1M_CODED) == 0) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }


    /*
 *    Coded  2M  1M
 *                          0    0    0     ->  Invalid HCI Command Parameters
 *  INIT_PHY_1M :           0    0    1  ------------------------------------------------|
 *  INIT_PHY_2M :           0    1    0     ->  Invalid HCI Command Parameters           |
 *  INIT_PHY_1M_2M :        0    1    1  ------------------------------------------------|
 *  INIT_PHY_CODED :        1    0    0                                                  |------ these 3 case can use same entry
 *  INIT_PHY_1M_CODED :     1    0    1                                                  |
 *  INIT_PHY_2M_CODED :     1    1    0                                                  |
 *  INIT_PHY_1M_2M_CODED :  1    1    1  ------------------------------------------------|
 */
    u8 init_phys = pCmdParam->init_PHYs & INIT_PHY_1M_2M_CODED;
    if (init_phys == INIT_PHY_CODED) {
        return blc_ll_extended_createConnection(pCmdParam->fp, pCmdParam->ownAddr_type, pCmdParam->peerAddr_type, pCmdParam->peer_addr, init_phys, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, pCmdParam->initCfg[0].scan_inter, pCmdParam->initCfg[0].scan_wind, pCmdParam->initCfg[0].conn_min, pCmdParam->initCfg[0].conn_max, pCmdParam->initCfg[0].timeout);
    }
    #if 0
    else if(init_phys == INIT_PHY_1M_CODED ){
        return blc_ll_extended_createConnection (pCmdParam->fp, pCmdParam->ownAddr_type, pCmdParam->peerAddr_type, pCmdParam->peer_addr, init_phys,
                                                 pCmdParam->initCfg[0].scan_inter, pCmdParam->initCfg[0].scan_wind, pCmdParam->initCfg[0].conn_min, pCmdParam->initCfg[0].conn_max, pCmdParam->initCfg[0].timeout,
                                                 0,0,0,0,0,
                                                 pCmdParam->initCfg[1].scan_inter, pCmdParam->initCfg[1].scan_wind, pCmdParam->initCfg[1].conn_min, pCmdParam->initCfg[1].conn_max, pCmdParam->initCfg[1].timeout);
    }
    else if(init_phys == INIT_PHY_2M_CODED ){
        return blc_ll_extended_createConnection (pCmdParam->fp, pCmdParam->ownAddr_type, pCmdParam->peerAddr_type, pCmdParam->peer_addr, init_phys,
                                                 0,0,0,0,0,
                                                 pCmdParam->initCfg[0].scan_inter, pCmdParam->initCfg[0].scan_wind, pCmdParam->initCfg[0].conn_min, pCmdParam->initCfg[0].conn_max, pCmdParam->initCfg[0].timeout,
                                                 pCmdParam->initCfg[1].scan_inter, pCmdParam->initCfg[1].scan_wind, pCmdParam->initCfg[1].conn_min, pCmdParam->initCfg[1].conn_max, pCmdParam->initCfg[1].timeout);
    }
    #else //optimize, use same entry
    else if (init_phys == INIT_PHY_1M_CODED || init_phys == INIT_PHY_2M_CODED) {
        return blc_ll_extended_createConnection(pCmdParam->fp, pCmdParam->ownAddr_type, pCmdParam->peerAddr_type, pCmdParam->peer_addr, init_phys, pCmdParam->initCfg[0].scan_inter, pCmdParam->initCfg[0].scan_wind, pCmdParam->initCfg[0].conn_min, pCmdParam->initCfg[0].conn_max, pCmdParam->initCfg[0].timeout, pCmdParam->initCfg[0].scan_inter, pCmdParam->initCfg[0].scan_wind, pCmdParam->initCfg[0].conn_min, pCmdParam->initCfg[0].conn_max, pCmdParam->initCfg[0].timeout, pCmdParam->initCfg[1].scan_inter, pCmdParam->initCfg[1].scan_wind, pCmdParam->initCfg[1].conn_min, pCmdParam->initCfg[1].conn_max, pCmdParam->initCfg[1].timeout);
    }
    #endif
    else if (init_phys == INIT_PHY_1M || init_phys == INIT_PHY_1M_2M || init_phys == INIT_PHY_1M_2M_CODED) {
        return blc_ll_extended_createConnection(pCmdParam->fp, pCmdParam->ownAddr_type, pCmdParam->peerAddr_type, pCmdParam->peer_addr, init_phys, pCmdParam->initCfg[0].scan_inter, pCmdParam->initCfg[0].scan_wind, pCmdParam->initCfg[0].conn_min, pCmdParam->initCfg[0].conn_max, pCmdParam->initCfg[0].timeout, pCmdParam->initCfg[1].scan_inter, pCmdParam->initCfg[1].scan_wind, pCmdParam->initCfg[1].conn_min, pCmdParam->initCfg[1].conn_max, pCmdParam->initCfg[1].timeout, pCmdParam->initCfg[2].scan_inter, pCmdParam->initCfg[2].scan_wind, pCmdParam->initCfg[2].conn_min, pCmdParam->initCfg[2].conn_max, pCmdParam->initCfg[2].timeout);
    } else {
    /*If the Host specifies a PHY that is not supported by the Controller,
        including a bit that is reserved for future use, the latter should return
        the error code Unsupported Feature or Parameter Value (0x11)
        HCI/CM/BI-01-C */
    #if 1
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    #else
        /* HCI/GEV/BV-03-C [Disallow Mixing Legacy and Extended Scanning Commands] processed in "blc_ll_extended createConnection"
             * to guarantee "HCI_ERR_CMD_DISALLOWED" is before "HCI_ERR_INVALID_HCI_CMD_PARAMS" when 2 error happens at same time,
             * put this error handling in "blc_ll_extended createConnection"*/
        return blc_ll_extended_createConnection(pCmdParam->fp, pCmdParam->ownAddr_type, pCmdParam->peerAddr_type, pCmdParam->peer_addr, init_phys, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    #endif
    }
}

_attribute_ram_code_ int blt_secchn_procInitPkt(u8 *raw_pkt)
{
    //DBG_C HN6_TOGGLE;

    int initiate_success = 0;

    rf_pkt_ext_adv_t *pExtAdv = (rf_pkt_ext_adv_t *)(raw_pkt + DMA_RFRX_LEN_HW_INFO);

    int direct_adv = pExtAdv->ext_hdr_flg & EXTHD_BIT_TARGETA;
    /* attention: pkt_init.rxAddr &  pkt_init.txAddr &  pkt_init.advA &  pkt_init.initA
     * are set in "blt_ll_init_filter" !!! */
    if (blt_ll_init_filter(direct_adv, pExtAdv->txAddr, pExtAdv->rxAddr, pExtAdv->data, pExtAdv->data + 6)) {
        //DBG_C HN7_TOGGLE;

        /* set pkt_init.interval & pkt_init.timeout */
        u8 phy_idx           = bltPHYs.cur_llPhy - 1; // //"le_phy_type_t"    1:1M    2:2M   3:Coded
        bltInit.mas_intv_mul = bltExtInit.mas_hold_intv_mul[phy_idx];
        bltInit.mas_intv_msk = bltExtInit.mas_hold_intv_msk[phy_idx];

        //my_dump_str_u32s(0, "CCCCCCCC", phy_idx, bltExtInit.mas_hold_intv_mul[phy_idx], bltInit.mas_intv_mul, 0);

        pkt_init.interval = aclMas_param.master_connInter * bltInit.mas_intv_mul;
        pkt_init.timeout  = bltExtInit.hold_timeout[phy_idx];

        bltInit.sec_chn_init = 1;
        if (blms_m_connect((rf_packet_connect_t *)&pkt_init, raw_pkt)) {
            //DBG_C HN8_TOGGLE;
            initiate_success = 1;

            blmsParam.scanInitEn_union.ext_init_en = 0;


    /*
             * only extended scan can run here, so not need to judge legacy scan.
             * if user initial error:both legacy scan and extended scan are be initial, we can directly process according to extended scan state.
             */
    #if (SCAN_EN_MORE_STRATEGY)
            int remove_prichn_scan = 0;
            if (!blmsParam.scanInitEn_union.ext_scan_en) {
                remove_prichn_scan = 1;
            } else if (blmsParam.cur_master_num == blmsParam.max_master_num) {
                if (!bltScn.scan_en_strategy && blmsParam.scanInitEn_union.ext_scan_en) {
                    remove_prichn_scan = 1;
                }
            }

            if (remove_prichn_scan) {
                blt_sche_removeTaskMask(TSKMSK_PRICHN_SCAN);
            }
    #else
        /* If current scanning is triggered by initialization, Scan mask should also removed */
        #if (LL_FEATURE_ENABLE_LE_EXTENDED_SCAN || LL_FEATURE_ENABLE_LE_EXTENDED_INITIATE)
            if (!blmsParam.scanInitEn_union.ext_scan_en && (blmsParam.cur_master_num == blmsParam.max_master_num))
        #else
            if (!blmsParam.scanInitEn_union.ext_scan_en || blmsParam.cur_master_num == blmsParam.max_master_num)
        #endif
            {
                blt_sche_removeTaskMask(TSKMSK_PRICHN_SCAN); // master connection triggers "update", no need triggers "update" for scan remove
            }
    #endif
        }
    }


    STOP_RF_STATE_MACHINE;
    CLEAR_ALL_RFIRQ_STATUS; //clear


    return initiate_success;
}


#endif //end of LL_FEATURE_ENABLE_LE_EXTENDED_INITIATE && LL_ACL_CEN_EN
