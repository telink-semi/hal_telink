/********************************************************************************************************
 * @file    leg_init.c
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


#if (LL_ACL_CEN_EN)

#if OS_SUP_EN
#include "stack/ble/os_sup/os_sup.h"
#include "stack/ble/os_sup/os_sup_stack.h"
#endif



_attribute_ble_data_retention_  _attribute_aligned_(4) ll_init_t  bltInit;



/**
 * @brief      for user to initialize legacy initiating module
 *             notice that only one module can be selected between legacy initiating module and extended initiating module
 * @param      none
 * @return     none
 */
void blc_ll_initLegacyInitiating_module(void)
{
    blt_ll_initInitiatingCommon();


}




_attribute_ram_code_ int  blt_legacy_initiate_process(u8 *raw_pkt)
{

    rf_packet_adv_t * pAdv = (rf_packet_adv_t *) (raw_pkt + DMA_RFRX_LEN_HW_INFO);
    int initiate_success = 0;

    /* attention: pkt_init.rxAddr &  pkt_init.txAddr &  pkt_init.advA &  pkt_init.initA
     * are set in "blt_ll_init_filter" !!! */
    if(blt_ll_init_filter(bltScn.direct_adv, pAdv->txAddr, pAdv->rxAddr, pAdv->advA, pAdv->data)){


        bltInit.sec_chn_init = 0;
        if(blms_m_connect( (rf_packet_connect_t *)&pkt_init, raw_pkt)){

            initiate_success = 1;
            blmsParam.scanInitEn_union.leg_init_en = 0;
            blmsParam.scanInitEn_union.ext_init_en = 0;  //important: ext Init may also send conn_req on primary channel !!!

            #if (SCAN_EN_MORE_STRATEGY)
                int remove_prichn_scan = 0;
                //both extended scan and legacy scan can run here.so here need to judge both.
                if(!blmsParam.scanInitEn_union.ext_scan_init_en && !blmsParam.scanInitEn_union.leg_scan_en){
                    remove_prichn_scan = 1;
                }
                else if(blmsParam.cur_master_num == blmsParam.max_master_num){
                    if(!bltScn.scan_en_strategy && (blmsParam.scanInitEn_union.leg_scan_en || blmsParam.scanInitEn_union.ext_scan_en) ){
                        remove_prichn_scan = 1;
                    }
                }

                if(remove_prichn_scan){
                    blt_sche_removeTaskMask(TSKMSK_PRICHN_SCAN);
                }
            #else
                /* If current scanning is triggered by initialization, Scan mask should also removed */
                #if (LL_FEATURE_ENABLE_LE_EXTENDED_SCAN || LL_FEATURE_ENABLE_LE_EXTENDED_INITIATE)
                    if(!blmsParam.scanInitEn_union.ext_scan_init_en && (!blmsParam.scanInitEn_union.leg_scan_en || blmsParam.cur_master_num == blmsParam.max_master_num))
                #else
                    if(!blmsParam.scanInitEn_union.leg_scan_en || blmsParam.cur_master_num == blmsParam.max_master_num)
                #endif
                    {
                        blt_sche_removeTaskMask(TSKMSK_PRICHN_SCAN);  // master connection triggers "update", no need triggers "update" for scan remove
                    }
            #endif


            my_dump_str_data(DBG_PRVC_INIT_EN, "[PRV][INI] legacy initiate success", 0, 0);

            /* wait TX done
             * must add timeout check for RF status traverse, to avoid potential risk */
            //DBG_C HN7_HIGH;
            //RF 1M mode: (rf_len + 10)*8: 10 = PREAMBLE 1B + AA 4B + HDR 2B + CRC 3B:
            //conn_req_rf_len = 34: u32 connReq_timeout_us = 532;  //(34+10)*8=352; 352 + 150 + 30(margin)
            while( !HAL_GET_RF_TX_IRQ && (u32)(clock_time() - bltRxPkt.rx_irq_tick) < (532 * SYSTEM_TIMER_TICK_1US)){
                if(usr_irq_handler_cb){usr_irq_handler_cb();}
            }
            //DBG_C HN7_LOW;
            HAL_CLEAR_RF_TX_IRQ;
        }
    }

    /*
     * CSEM IP, when RF is in the TX state, must use the reset baseband to stop TX, and can not use other methods.
     * legacy init When the ADV data is received and RPA is parsed failed, it is possible that
     * the RF is already in the TX state (during TX settle) and is ready to send the Connect_Req.
     * In the above case, must use reset baseband to solve the problem.
     *
     * In other cases, using reset baseband has no bad effect.
     * For simple code logic, reset baseband is used.
     */
    HAL_CSEM_IP_RESET_BASEBAND; //RF rx dma config keep, tx dma config lost after reset baseband for TL751X chip

    STOP_RF_STATE_MACHINE;  /* if initiate fail, stop FSM */
    HAL_CLEAR_RF_TX_IRQ;  //clear TX

    if(initiate_success){
        blmsParam.rf_fsm_busy = 0;
        rf_set_tx_rx_off();
    }
    else{ //back to scan
        rf_ble_tx_done ();
        /**
         * For CSEM IP, We use special process to disable RX continue mode:
         *  rf_ble_csem_close_rx_continue_mode();
         *  HAL_CSEM_IP_RESET_BASEBAND;
         *
         * We can use two strategies to restore RX continue mode:
         *
         *  1: Because the reset_baseband will cause the TX DMA registers to be lost,
         *  other RF registers will not be lost, we can use the original code,
         *  need to ensure that the function: rf_set_rxmode can take effect;
         *
         *  2: we package a new function that restore RX continue mode, which
         *  will involve the registers are re-set again, cumbersome, but
         *  insurance, currently we use 1 strategy, the simplest way.
         *  TODO: we need to test and verify;
         **/
        blmsParam.rf_fsm_busy = 1;
        rf_set_rxmode ();  //go on Scanning
        if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_RX_ON);  }
    }


    return initiate_success;
}





ble_sts_t   blt_ll_createConnection( scan_inter_t scanInter, scan_wind_t scanWindow, init_fp_t fp, u8 peerAdrType, u8 *peerAddr, own_addr_type_t ownAdrType,
                                     conn_inter_t conn_min,  conn_inter_t conn_max, u16 conn_latency, conn_tm_t timeout, u16 ce_min,   u16 ce_max )
{
    (void)scanInter; //unused, remove warning
    (void)scanWindow; //unused, remove warning
    (void)conn_latency; //unused, remove warning
    (void)ce_min; //unused, remove warning
    (void)ce_max; //unused, remove warning

    /* some code use "hci_cmd_mask" even without HCI, so handle it here */
    /* HCI/GEV/BV-03-C [Disallow Mixing Legacy and Extended Scanning Commands] */
    if(IS_EXTENDED_SCAN_VALID){
        return HCI_ERR_CMD_DISALLOWED;
    }
    else{
        SET_LEGACY_SCAN_VALID;
    }




    /*
    If the Host issues this command when another HCI_LE_Extended_Create_-
    Connection command is pending in the Controller, the Controller shall return
    the error code Command Disallowed (0x0C).

    If the Initiating_PHYs parameter does not have at least one bit set for a PHY
    allowed for scanning on the primary advertising physical channel, the
    Controller shall return the error code Invalid HCI Command Parameters (0x12).
    */

    u8 ret_status = (u8)blt_ll_createConnCommon(fp, ownAdrType, peerAdrType, peerAddr);
    if(ret_status != BLE_SUCCESS){
        return ret_status;
    }

    /* When code running here, initiation will start */




    /**************************** master conn_interval process begin *********************************************************************************/

    #if (ACL_CENTRAL_BASE_INTERVAL_FOLLOW_UPPER_LAYER)
        if(blmsParam.cur_master_num == 0){  //first ACL master
            blc_ll_setAclCentralBaseConnectionInterval(conn_min);
            u32 oct_conn_min_us = blt_debug_hex_2_dec_display(conn_min*1250);
            my_dump_str_data(DBG_SET_CIG_PARAMS, "[LEGINIT] ACL base interval", &oct_conn_min_us, 4)
        }
    #endif




    bltInit.mas_intv_mul = blt_init_calculateMasterIntervalMultiplier(aclMas_param.master_connInter, conn_min, conn_max);
    if(bltInit.mas_intv_mul == 24){
        bltInit.mas_intv_msk = INTV_MSK_24_TIME;
    }
    else{
        bltInit.mas_intv_msk = interMask_tbl[bltInit.mas_intv_mul];
    }


    #if (IMPROVE_MASTER_INTERVAL)
        u16 conn_inter_use = aclMas_param.master_connInter*bltInit.mas_intv_mul;
        if(conn_min <= conn_inter_use && conn_max >= conn_inter_use){ //totally meet host's requirement
            //do nothing
        }
        else{ //not exactly host's requirement
            if(conn_min > aclMas_param.master_connInter){
                int mod = conn_max % aclMas_param.master_connInter;
                u16 conn_inter = conn_max - mod;
                if(conn_inter >= conn_min){
                    bltInit.mas_intv_mul = conn_inter/aclMas_param.master_connInter;
                    bltInit.mas_intv_msk = 0xFFFFFF;
                }
            }
        }
    #endif

    my_dump_str_data(ACL_MASTER_INITIATE,"master intv mul",&bltInit.mas_intv_mul, 1);
    /**************************** master conn_interval process end *********************************************************************************/

    pkt_init.interval = aclMas_param.master_connInter*bltInit.mas_intv_mul;
    pkt_init.timeout = timeout;



    u32 r_sts = irq_disable();  //very important to disable IRQ

    if(blmsParam.scanInitEn_union.leg_scan_en){
        blmsParam.create_connection = CONNECT_REQ_LEG_PENDING;
    }
    else{
        /* if scan not enabled, */
        blmsParam.state_chng |= STATE_CHANGE_INIT;
        blmsParam.create_connection = CONNECT_REQ_GOING;
        bltScn.initiate_going = LEG_INITIATE_GOING;
    }

    irq_restore(r_sts);
#if OS_SUP_EN
    if(blt_os_giveSem_cb)
    {
        blt_os_giveSem_cb();
    }
#endif

    return BLE_SUCCESS;
}



ble_sts_t   blc_ll_createConnection( scan_inter_t scanInter, scan_wind_t scanWindow, init_fp_t fp, u8 peerAdrType, u8 *peerAddr, own_addr_type_t ownAdrType,
                                     conn_inter_t conn_min,  conn_inter_t conn_max, u16 conn_latency, conn_tm_t timeout, u16 ce_min,   u16 ce_max )
{
    (void)ce_min; //unused, remove warning
    (void)ce_max; //unused, remove warning

    tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_CMD), "[LL][CMD] Create Connection", peerAddr, 6);

    #if (CUSTOM_CONNECTION_ESTABLISH_EVT_ENABLE)
        if(aclConn_etbsh.crtConn_retry_num){
            if(aclConn_etbsh.crtConn_cur_cnt){
                return HCI_ERR_CMD_DISALLOWED;  //previous create command is pending in the Controller
            }

            aclConn_etbsh.crtConn_cur_cnt = 1;

            aclConn_etbsh.crtConn_buf.scan_inter = scanInter;
            aclConn_etbsh.crtConn_buf.scan_wind = scanWindow;
            aclConn_etbsh.crtConn_buf.fp = fp;
            aclConn_etbsh.crtConn_buf.peerAddr_type = peerAdrType;
            smemcpy(aclConn_etbsh.crtConn_buf.peer_addr, peerAddr, 6);
            aclConn_etbsh.crtConn_buf.ownAddr_type = ownAdrType;
            aclConn_etbsh.crtConn_buf.conn_min = conn_min;
            aclConn_etbsh.crtConn_buf.conn_max = conn_max;
            aclConn_etbsh.crtConn_buf.connLatency = conn_latency;
            aclConn_etbsh.crtConn_buf.timeout = timeout;
        }
    #endif

    return  blt_ll_createConnection(scanInter, scanWindow, fp, peerAdrType, peerAddr, ownAdrType, conn_min, conn_max, conn_latency, timeout, 0, 0);
}


ble_sts_t   blc_hci_le_createConnection( hci_le_createConn_cmdParam_t * pCmdParam)
{
    //my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Create_Connection", &pCmdParam->fp, 8);
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Create_Connection", pCmdParam, sizeof(hci_le_createConn_cmdParam_t));

    my_dump_str_u32s(DBG_CIS_CENTRAL_PARAM, "ACL interval", blt_debug_hex_2_dec_display(pCmdParam->conn_min * 1250), \
                                                           blt_debug_hex_2_dec_display(pCmdParam->conn_max * 1250), 0, 0);


    return blc_ll_createConnection ( pCmdParam->scan_inter, pCmdParam->scan_wind, pCmdParam->fp,
                                     pCmdParam->peerAddr_type, pCmdParam->peer_addr, pCmdParam->ownAddr_type,
                                     pCmdParam->conn_min, pCmdParam->conn_max, 0, pCmdParam->timeout, 0, 0);  //latency & ce_min & ce_max not process

}





#else    //else of LL_ACL_CEN_EN







void        blc_ll_initLegacyInitiating_module(void)
{

}

ble_sts_t   blc_ll_createConnection( scan_inter_t scan_interval, scan_wind_t scan_window, init_fp_t filter_policy, u8 adr_type, u8  *mac, own_addr_type_t own_adr_type,
                                     conn_inter_t conn_min, conn_inter_t conn_max, u16 conn_latency, conn_tm_t timeout, u16 ce_min, u16 ce_max )
{
    (void)scan_interval;
    (void)scan_window;
    (void)filter_policy;
    (void)adr_type;
    (void)mac;
    (void)own_adr_type;
    (void)conn_min;
    (void)conn_max;
    (void)conn_latency;
    (void)timeout;
    (void)ce_min;
    (void)ce_max;
    return HCI_ERR_CMD_DISALLOWED;
}


#endif   //end of LL_ACL_CEN_EN
