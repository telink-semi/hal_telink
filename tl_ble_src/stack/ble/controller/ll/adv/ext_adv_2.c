/********************************************************************************************************
 * @file    ext_adv_2.c
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


#if (LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING)


aux_ptr_t gbl_auxPtr = {
    0,                                     // chn_index
    EXT_ADV_PDU_AUXPTR_CA_0_50_PPM,        // ca
    EXT_ADV_PDU_AUXPTR_OFFSET_UNITS_30_US, // aux_unit
    0,                                     // aux_offset
    0,                                     // aux_phy
};


    #ifndef DEBUG_RF_EXTADV_WHILE_EN
        #define DEBUG_RF_EXTADV_WHILE_EN 0 //when test ok, need to delete related code. todo by qiuwei.
    #endif

    #if (DEBUG_RF_EXTADV_WHILE_EN)
typedef struct
{
    u32 primary_extAdv_tmCnt;
    u32 second_auxAdv_conn_tmCnt;
    u32 second_auxAdv_nonConn_tmCnt;
} rf_while_debug_t;

_attribute_data_retention_ rf_while_debug_t A_rf_while_debug;
    #endif

_attribute_ram_code_ void blt_adv_ext_ind_fill_advA_targetA(void)
{
    ll_adv_ext_ind_header_t *p_adv_ext_ind = (ll_adv_ext_ind_header_t *)&blt_pextadv->primary_adv;
    u8                       advA_len      = 0;
    if (p_adv_ext_ind->ext_hdr_flg & EXTHD_BIT_ADVA) {
        p_adv_ext_ind->txAddr = blt_pextadv->cur_advA_type;
        smemcpy(p_adv_ext_ind->data, blt_pextadv->cur_advA_addr, BLE_ADDR_LEN);
        advA_len = 6;
    }
    if (p_adv_ext_ind->ext_hdr_flg & EXTHD_BIT_TARGETA) {
        p_adv_ext_ind->rxAddr = blt_pextadv->cur_initA_type;
        smemcpy(p_adv_ext_ind->data + advA_len, blt_pextadv->cur_initA_addr, BLE_ADDR_LEN);
    }
}

_attribute_ram_code_ void blt_aux_adv_ind_fill_advA_targetA(void)
{
    ll_aux_adv_ind_header_t *p_aux_adv_ind = (ll_aux_adv_ind_header_t *)&blt_pextadv->aux_adv_1stPkt;
    u8                       advA_len      = 0;
    if (p_aux_adv_ind->ext_hdr_flg & EXTHD_BIT_ADVA) {
        p_aux_adv_ind->txAddr = blt_pextadv->cur_advA_type;
        smemcpy(p_aux_adv_ind->data, blt_pextadv->cur_advA_addr, BLE_ADDR_LEN);
        advA_len = 6;
    }
    if (p_aux_adv_ind->ext_hdr_flg & EXTHD_BIT_TARGETA) {
        p_aux_adv_ind->rxAddr = blt_pextadv->cur_initA_type;
        smemcpy(p_aux_adv_ind->data + advA_len, blt_pextadv->cur_initA_addr, BLE_ADDR_LEN);
    }
}

_attribute_ram_code_ static void blt_extadv_rpa_update_process(void)
{
    #if (LL_FEATURE_ENABLE_LOCAL_RPA)
    st_ext_adv_t *cur_pextadv = blt_pextadv;

    cur_pextadv->extAdv_advA_useRpa = 0;
    cur_pextadv->extAdv_initAUseRpa = 0;
    cur_pextadv->pRslvlst_extAdv    = NULL;


    blt_ll_addr_clear_local_rpa_flag();

    if (cur_pextadv->own_addr_rpa) {
        ll_resolv_list_t *pRL = blt_ll_searchResolvingListEntry(cur_pextadv->eAdvParaCmd_peerAdrType, cur_pextadv->eAdvParaCmd_peerAddr);
        if (pRL) {
            cur_pextadv->extAdv_advA_useRpa = pRL->localIrk_valid;
            cur_pextadv->extAdv_initAUseRpa = pRL->peerIrk_valid; //only for direct ADV

        #if 0                                                     //(DBG_PRVC_EXTADV_EN)
                    if(pRL->localIrk_valid && pRL->peerIrk_valid){
                        my_dump_str_data(DBG_PRVC_EXTADV_EN, "extadv, search RL OK, local & peer IRK valid", &pRL->rl_idx, 1);
                    }
                    else if(!pRL->localIrk_valid && pRL->peerIrk_valid){
                        my_dump_str_data(DBG_PRVC_EXTADV_EN, "extadv, search RL OK, local IRK invalid", &pRL->rl_idx, 1);
                    }
                    else if(pRL->localIrk_valid && !pRL->peerIrk_valid){
                        my_dump_str_data(DBG_PRVC_EXTADV_EN, "extadv, search RL OK, peer IRK invalid", &pRL->rl_idx, 1);
                    }
                    else{
                        my_dump_str_data(DBG_PRVC_EXTADV_EN, "extadv, search RL OK, local & peer IRK invalid", &pRL->rl_idx, 1);
                    }
        #endif

            if (cur_pextadv->extAdv_advA_useRpa || cur_pextadv->extAdv_initAUseRpa) {
                if (cur_pextadv->extAdv_advA_useRpa) {
                    blt_ll_addr_mark_local_rpa(pRL);
                }
                cur_pextadv->pRslvlst_extAdv = pRL;
                blt_ll_resolvSetRpaInUse(pRL);
            }
        } else {
            my_dump_str_data(DBG_PRVC_LEGADV_EN, "[PRV][ADV] extadv, search RL ERR", 0, 0);
        }
    }


    if (blt_pextadv->extAdv_advA_useRpa) {
        blt_pextadv->cur_advA_type = BLE_ADDR_RANDOM;
        smemcpy(blt_pextadv->cur_advA_addr, blt_pextadv->pRslvlst_extAdv->rlLocalRpa, BLE_ADDR_LEN);
    } else
    #endif
    {
        blt_pextadv->cur_advA_type = blt_pextadv->extadv_mac_type;
        smemcpy(blt_pextadv->cur_advA_addr, blt_pextadv->extadv_mac_addr, BLE_ADDR_LEN);
    }

    #if (LL_FEATURE_ENABLE_LOCAL_RPA)
    if (blt_pextadv->extAdv_initAUseRpa) {
        blt_pextadv->cur_initA_type = BLE_ADDR_RANDOM;
        smemcpy(blt_pextadv->cur_initA_addr, blt_pextadv->pRslvlst_extAdv->genrt_peerRpa, BLE_ADDR_LEN);
    } else
    #endif
    {
        blt_pextadv->cur_initA_type = blt_pextadv->eAdvParaCmd_peerAdrType;
        smemcpy(blt_pextadv->cur_initA_addr, blt_pextadv->eAdvParaCmd_peerAddr, BLE_ADDR_LEN);
    }
}

_attribute_ram_code_ void blt_ll_procLegacyRxPacket(u8 *prx, u16 *pmac)
{
    u32              rx_begin_tick = clock_time();
    rf_pkt_adv_rx_t *pAdvRx        = (rf_pkt_adv_rx_t *)(prx + RF_BLE_DMA_RFRX_LEN_HW_INFO);
    u16             *advA16        = (u16 *)pkt_Adv.advA;

    rf_set_tx_packet_address(&pkt_scanRsp); //get ready scan_rsp as early as possible


    // (rf_len + 10)*8:  10 = preamble 1B + access_code 4B + header 2B + CRC 3B
    // header 2B + PDU "rf_len" B + CRC 3B  is dma data, at least 4B is arrived when  "*ph" not 0
    //  so rest longest data is: rf_len + 2 + 3 - 4 = rf_len + 1
    // only 2 kind of request:  scan_req rf_len = 12, conn_req rf_len = 34
    // RX packet reset timing is (rf_len+1)*8 = (34+1)*8 = 280
    // add 20 uS for RX status delay, 280+20 = 300uS

    //here I think no need give RX 400uS, another method is using core_431,rx_max_len, try later, optimize

    //Add margin 80us by Yafei,240530, 300 us looks like a narrow margin
    while (!HAL_GET_RF_RX_IRQ && (clock_time() - rx_begin_tick) < (300 + 80) * SYSTEM_TIMER_TICK_1US) {
        if (usr_irq_handler_cb) {
            usr_irq_handler_cb();
        }
    }

    /* safe, here delay Sequence for Onca chip, adv_duration time is enough, add by Yafei */
    HAL_WAIT_MODEM_SEQ_TIME;

    //////////////////////////////////////
    u32 scanrsp_trigger_tick = 0;

    if (pAdvRx->rf_len == 12) //maybe scan_req
    {
        rf_ble_set_tx_settle(TX_STL_LEGADV_SCANRSP_SET);
        rf_ble_csem_set_tx_rx_settle(0, TX_STL_LEGADV_SCANRSP_SET, 0);
        /*
         * hal_rf_get_rx_timestamp() - HW_DELAY_1M + 12*8 + 5*8(2 header+3crc) + 150 - TX_STL_LEGADV_SCANRSP_REAL
         */
        scanrsp_trigger_tick = hal_rf_get_rx_timestamp() + (12 * 8 + (190 - HW_DELAY_1M - TX_STL_LEGADV_SCANRSP_REAL)) * SYSTEM_TIMER_TICK_1US;
    #if 1                                        //will cost some time
        if (tick1_exceed_tick2(clock_time(), scanrsp_trigger_tick)) {
            scanrsp_trigger_tick = clock_time(); //need to check frequency.
        }
    #endif
        rf_start_fsm(FSM_STX, NULL, scanrsp_trigger_tick);
        if (blc_rf_pa_cb) {
            blc_rf_pa_cb(PA_TYPE_TX_ON);
        }
    }

    HAL_CLEAR_RF_TX_IRQ; //important: clear RX status
    //////////////////////////////////////
    st_ext_adv_t *pExtAdv = blt_pextadv;

    if (RF_BLE_PACKET_VALIDITY_CHECK(prx)) {
        /* advA in scan_req/conn_req must be same as advertiser's advA */
        if (MAC_MATCH16(pmac, advA16)) //equal
        {
            do {
                u8 is_connect_req = 0, filter_enable = 0;
                /* step 1, quick check if scan_req or connect_req basic logic pass
                *         skill:  Put the hardest conditions first */
                if (pAdvRx->rf_len == 12 && pAdvRx->type == LL_TYPE_SCAN_REQ && pExtAdv->scnReq_response) {           //scan_req
                    filter_enable = pExtAdv->adv_filterPolicy & ALLOW_SCAN_WL;
                } else if (pAdvRx->rf_len == 34 && pAdvRx->type == LL_TYPE_CONNECT_REQ && pExtAdv->conReq_response) { //conn_req
                    filter_enable         = pExtAdv->adv_filterPolicy & ALLOW_CONN_WL;
                    is_connect_req        = 1;
                    blc_rcvd_connReq_tick = clock_time();                                                             //need to provide API to host and need to distinguish connHandle
                } else {
                    my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "extadv, not expected pkt, stop", pAdvRx->type, 0, 0, 0);
                    break;                                                                                            //stop
                }

                ll_resolv_list_t *pRL_match   = NULL;
                u8                peer_is_rpa = IS_RESOLVABLE_PRIVATE_ADDR(pAdvRx->txAddr, pAdvRx->peerA);

    /* step 2, network privacy ignore IDA process */
    #if (NETWORK_PRIVACY_IGNORE_IDA_CHECK)
                /* check if network privacy mode ignore IDA exist */
                if (!peer_is_rpa) {
                    if (pExtAdv->pRslvlst_extAdv) {
                        pRL_match = pExtAdv->pRslvlst_extAdv;
                    } else {
                        pRL_match = blt_ll_searchResolvingListEntry(pAdvRx->txAddr, pAdvRx->peerA);
                    }

                    if (pRL_match && pRL_match->peerIrk_valid) {             //peer device has distributed its IRK
                        if (pRL_match->rlPrivMode == NETWORK_PRIVACY_MODE) { //not allowed
                            /* LL/SEC/ADV/BV-15-C  LL/SEC/ADV/BV-16-C  LL/SEC/ADV/BV-17-C*/
                            my_dump_str_data(DBG_PRVC_EXTADV_EN, "extadv, network privacy ignore IDA, stop", 0, 0);
                            break; //stop
                        } else {   //DEVICE_PRIVACY_MODE, allowed
                            /* LL/SEC/ADV/BV-18-C  LL/SEC/ADV/BV-19-C  LL/SEC/ADV/BV-20-C*/
                            my_dump_str_data(DBG_PRVC_EXTADV_EN, "extadv, device privacy accept IDA", 0, 0);
                        }
                    }
                }
    #else
                    //special process:
    #endif


                blt_ll_addr_set_peer_address(0, pAdvRx->txAddr, pAdvRx->peerA);

                /* step 3, resolve RPA if needed, check whiteList if filtering needed
                * for peerA RPA
                * 1. scan_req, resolve if filter needed; do not resolve if no filter
                * 2. conn_req, if direct ADV,      must resolve, than can check if addressed to local device
                *              if none direct ADV, resolve if filtering needed; do not resolve if no filter
                * Consider accept list, change to:
                * 1. if direct ADV, must resolve, than check if addressed to local device, never use filtering/accept list
                * 2. if none direct ADV, resolve RPA depend on filtering, then check accept list
                */
                if (pExtAdv->directed_adv || filter_enable) {
    #if (LL_FEATURE_ENABLE_PRIVACY)
                    if (peer_is_rpa) {
                        pRL_match = blt_ll_resolve_rpa(0, pAdvRx->peerA, pExtAdv->pRslvlst_extAdv);
                        if (pRL_match) {
                            blt_ll_storePeerDeviceRpa(pRL_match, pAdvRx->peerA);
                            blt_ll_addr_set_peer_address(1, pRL_match->rlIdAddrType, pRL_match->rlIdAddr);
                            my_dump_str_data(DBG_PRVC_EXTADV_EN, "peer RPA resolve OK", pRL_match->rlIdAddr, 6);
                        } else {
                            my_dump_str_data(DBG_PRVC_EXTADV_EN, "peer RPA resolve ERR, stop", 0, 0);
                            break;
                        }
                    }
    #endif
                    if (pExtAdv->directed_adv) { //direct ADV, do not care about filter
                                                 /* for ADV_DIRECT_IND, check initA in connReq for direct ADV
                       * regardless of IDA or RPA. here we just add some extra check for directed ADV special.
                       * case 1. both IDA, advA and initA should be same. So we abandon different IDAs.
                       * case 2. both RPA, Spec
                       *                  The Link Layer should not set the InitA field to the same value
                                          as the TargetA field in the received advertising PDU.
                       *                  attention: here should not equal to must !!!
                       * case 3. Advertiser IDA, Initiator RPA, not exist for a correct privacy process.
                       *         But it need BLE host and controller both be correct, for host it's very hard to process.
                       *         So here we allow this situation exist, do not abandon.
                       *         Evaluation for this special process :For BQB, host is upper tester of other vendor,
                       *         we can guarantee our controller be correct. For mass production SDK, here if we
                       *         advertiser commit this error, and peer device do not detect this error, we let this flow go.
                       * case 4. Advertiser RPA, Initiator IDA
                       *         If peer should use RPA
                       *             If "NETWORK_PRIVACY IGNORE_IDA_CHECK" enable, can be rejected by network privacy, accept by device privacy
                       *             If "NETWORK_PRIVACY IGNORE_IDA_CHECK" disable, special process: now code take as device privacy (SiHui)
                       *         If peer no need use RPA, accept
                       */
                        if (smemcmp(bltAddr.peer_pka_or_ida_addr, pExtAdv->eAdvParaCmd_peerAddr, BLE_ADDR_LEN) ||
                            bltAddr.peer_pka_or_ida_type != pExtAdv->eAdvParaCmd_peerAdrType) {
                            my_dump_str_data(DBG_PRVC_EXTADV_EN, "direct ADV initA do not address to local, stop", 0, 0);
                            break;
                        }
                    } else { //none direct ADV but filter needed
                        if (!blt_ll_searchAddrInWhiteListTbl(bltAddr.peer_pka_or_ida_type, bltAddr.peer_pka_or_ida_addr)) {
                            my_dump_str_data(DBG_PRVC_EXTADV_EN, "peer advA not in WL, stop", bltAddr.peer_pka_or_ida_addr, 6);
                            break;
                        }
                    }
                } else { //none direct ADV, no filter, pass without any check
                         //consider later : even no filter, maybe we need try to resolve RPA, because
                         //enhanced connection complete event need peer RPA
                }


                /* final step, respond to scan_req(send scan_rsp) or conn_req(connect) */
                if (is_connect_req) { //CONNECT_REQ
                    my_dump_str_data(DBG_PRVC_EXTADV_EN, "extadv, accept conn_req", 0, 0);
                    //if(ll_adv_2_slave_cb)  //to save RamCode
                    {
                        if (ll_adv_2_slave_cb((rf_packet_connect_t *)pAdvRx, FALSE) == TRUE) // blt_s_connect()
                        {
    #if (EXT_ADV_EN_MORE_STRATEGY)
                            /*
                                    * Extended adv only use strategy_3, Refer to the description of LEG_ADV_EN_STRATEGY_3
                                    *
                                    * Core Spec:"LE Advertising Set Terminated event" shall be generated every time
                                    * connectable advertising in an advertising set results in a connection being created.
                                    *
                                    * The Controller shall only start an advertising event when the corresponding advertising set is enabled.
                                    * The Controller shall continue advertising until all advertising sets have been disabled.
                                    * This can happen when the Host issues an HCI_LE_Set_Extended_Advertising_Enable command with the
                                    * Enable parameter set to 0x00 (Advertising is disabled), a connection is created, the duration specified
                                    * in the Duration[i] parameter expires, or the number of extended advertising events transmitted for the set
                                    * exceeds the Max_Extended_Advertising_Events[i] parameter.
                                    */
                            //if(blmsParam.extadv_en_strategy == EXT_ADV_EN_STRATEGY_3 ){}
                            blt_sche_removeTaskMask(TSKMSK_EXT_ADV_0 << blt_pextadv->extadv_index);
                            blt_sche_disableTask(TSKMSK_EXT_ADV_0 << blt_pextadv->extadv_index);
                            blt_sche_removeTaskMask(TSKMSK_AUX_ADV_0 << blt_pextadv->extadv_index);
                            blt_pextadv->extadv_en = 0;
                            blmsParam.ext_adv_en &= ~BIT(blt_pextadv->extadv_index);
    #endif
    #if (SMP_LOCAL_IRK_MATCH_CONTROLLER_NEW_PRIVACY)
                            /* for SMP: SMP_LOCAL_IRK_MATCH_CONTROLLER_NEW_PRIVACY, save idenAdr_type/idenAdr_addr */
                            blt_ll_record_identity_address(blt_pextadv->extadv_mac_type, blt_pextadv->extadv_mac_addr);
    #endif
                        }

                        bltAdv.adv_scanReq_connReq = 2;
                    }
                } else { //SCAN_REQ
                    //DBG_C HN11_TOGGLE;
                    my_dump_str_data(DBG_PRVC_EXTADV_EN, "extadv, send scan_rsp", 0, 0);
                    int span_us = TX_STL_LEGADV_SCANRSP_REAL + (pkt_scanRsp.rf_len + 10) * 8 + 10; //10uS: margin
                    //DBG_C HN0_HIGH;
                    while (!HAL_GET_RF_TX_IRQ && tick1_exceed_tick2(scanrsp_trigger_tick + span_us * SYSTEM_TIMER_TICK_1US, clock_time()))
                        ;
                    //DBG_C HN0_LOW;
                    HAL_CLEAR_RF_TX_IRQ;
                    bltAdv.adv_scanReq_connReq = 1;
                }
            } while (0);
        } else {
            my_dump_str_data(DBG_PRVC_EXTADV_EN, "extadv, peer advA not same with local, stop", pAdvRx->advA, 6);
        }
    } //end of "RF_BLE_PACKET_VALIDITY_CHECK()"

    /*
     * CSEM IP, when RF is in the TX state, must use the reset baseband to stop TX, and can not use other methods.
     * legacy adv When the SCAN REQ data is received and RPA is parsed, it is possible that
     * the RF is already in the TX state (during TX settle) and is ready to send the SCAN RSP.
     * In the above case, must use reset baseband to solve the problem.
     *
     * In other cases, using reset baseband has no bad effect.
     * For simple code logic, reset baseband is used.
     *
     * User maybe use PA module, and blc_rf_pa_cb() may causes more time, here call reset baseband before PA callback!
     */
    HAL_CSEM_IP_RESET_BASEBAND;

    STOP_RF_STATE_MACHINE;
    if (blc_rf_pa_cb) {
        blc_rf_pa_cb(PA_TYPE_OFF);
    }

    prx[0] = 1;
}

_attribute_ram_code_ int blt_send_legacy_adv(void)
{
    STOP_RF_STATE_MACHINE; // stop SM
    HAL_CLEAR_RF_TX_RX_IRQ;

    u32 tx_begin_tick = 0;

    if (blt_pextadv->evt_prop_bit04 == ADV_EVT_PROP_LEGACY_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED) {
        rf_start_fsm(FSM_STX, (void *)&pkt_Adv, clock_time());
        tx_begin_tick = clock_time();
    } else {
        rf_start_fsm(FSM_TX2RX, (void *)&pkt_Adv, clock_time());
        tx_begin_tick = clock_time();
    }

    if (blc_rf_pa_cb) {
        blc_rf_pa_cb(PA_TYPE_TX_ON);
    }


    if (bltAdv.advChn_cnt == 0) {
        blt_extadv_rpa_update_process();
    }


    /*****************There are at least 120uS to process ADV packet data ***********************************/
    if (blt_pextadv->directed_adv) { // /ADV_INDIRECT_IND (high duty cycle) or ADV_INDIRECT_IND (low duty cycle)
        pkt_Adv.type   = LL_TYPE_ADV_DIRECT_IND;
        pkt_Adv.rf_len = 12;

        pkt_Adv.chan_sel = local_chsel;
        pkt_Adv.rxAddr   = blt_pextadv->cur_initA_type;
        smemcpy(pkt_Adv.data, blt_pextadv->cur_initA_addr, BLE_ADDR_LEN);                          // fill in ADV data
    } else {
        if (blt_pextadv->evt_prop_bit04 == ADV_EVT_PROP_LEGACY_CONNECTABLE_SCANNABLE_UNDIRECTED) { // ADV_IND
            pkt_Adv.type     = LL_TYPE_ADV_IND;
            pkt_Adv.chan_sel = local_chsel;

        } else if (blt_pextadv->evt_prop_bit04 == ADV_EVT_PROP_LEGACY_SCANNABLE_UNDIRECTED) {                     // ADV_SCAN_IND
            pkt_Adv.type = LL_TYPE_ADV_SCAN_IND;
        } else if (blt_pextadv->evt_prop_bit04 == ADV_EVT_PROP_LEGACY_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED) { // ADV_NONCONN_IND
            pkt_Adv.type = LL_TYPE_ADV_NONCONN_IND;
        }

        if (blt_pextadv->curLen_advData) { //fill in ADV data, ADV data is ready
            smemcpy(pkt_Adv.data, blt_pextadv->dat_extAdv, blt_pextadv->curLen_advData);
        }

        pkt_Adv.rf_len = blt_pextadv->curLen_advData + BLE_ADDR_LEN;
    }
    pkt_Adv.dma_len = rf_tx_packet_dma_len(pkt_Adv.rf_len + 2);


    pkt_Adv.txAddr = blt_pextadv->cur_advA_type;
    smemcpy(pkt_Adv.advA, blt_pextadv->cur_advA_addr, BLE_ADDR_LEN);


    //process ADV packet data, sihui test: 32M clock, biggset PDU 37B, 15uS, 20200710,  none direct no advData 5.7uS
    /*********************************************************************************************************/


    /***********************************  pkt_scanRsp  *******************************************************/
    if (blt_pextadv->evt_prop_bit04 & ADVEVT_PROP_MASK_SCANNABLE) { // ADV_IND or ADV_SCAN_IND

        pkt_scanRsp.txAddr = blt_pextadv->cur_advA_type;
        smemcpy(pkt_scanRsp.advA, blt_pextadv->cur_advA_addr, BLE_ADDR_LEN);

        pkt_scanRsp.rf_len  = blt_pextadv->curLen_scanRsp + 6;
        pkt_scanRsp.dma_len = rf_tx_packet_dma_len(pkt_scanRsp.rf_len + 2);
        smemcpy(pkt_scanRsp.data, blt_pextadv->dat_scanRsp, blt_pextadv->curLen_scanRsp);
    }
    /*********************************************************************************************************/


    //attention: "txdone_us" that calculate after "pkt_Adv.rf_len" confirmed !!!
    /***********************************************************************************************************************
    TX_STL_ADV_SET_1M      BLE data          T_ifs         RX dma 2 unit data(Preamble 1B, access_code 4B, 1st dma 4B, 2nd dma 4B )
           110 uS      (rf_len + 10)*8      150 uS            13*8= 104uS,  add some margin
    ***********************************************************************************************************************/
    u32 txdone_us   = blt_phy_getRfPacketTime_us(pkt_Adv.rf_len, blt_pextadv->pri_phy, blt_pextadv->coding_ind) + TX_STL_ADV_REAL_COMMON;
    u32 connscan_us = txdone_us + BLE_T_IFS + 130;


    if (blt_pextadv->evt_prop_bit04 == ADV_EVT_PROP_LEGACY_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED) {
        //attention: here I remove TX software timeout judge, cause I think TX status 100% will come
        //TODO need to calculate accuracy timeout time
        //2024.5.31 modify by qihang, change margin time 5us to 60us, margin time 60us same as legacy_adv code. 5us is not enough for CSEM IP.
        //2024.6.7 txdone_us == RF CRC last bit time, TX END IRQ = RF CRC last bit time + 15 us, margin time set 20us, is enough for CSEM IP.
        //ext adv module, timing is relatively tight, reducing margin time is meaningful .
        while (!HAL_GET_RF_TX_IRQ && (u32)(clock_time() - tx_begin_tick) < (txdone_us + 20) * SYSTEM_TIMER_TICK_1US) {
            if (usr_irq_handler_cb) {
                usr_irq_handler_cb();
            }
        }
        HAL_CLEAR_RF_TX_IRQ;
    } else {
        //Switch dma rx buffer to ADV's dma rx buffer
        ble_rf_set_rx_dma((u8 *)glb_temp_rx_buff, 4); //change
        rf_set_rx_maxlen(34);                         //conn_req 34 Byte; scan_req 12 Byte,so 34 byte is enough

        u8           *prx  = (u8 *)glb_temp_rx_buff;
        u16          *pmac = (u16 *)(glb_temp_rx_buff + 12);
        volatile u32 *ph   = (u32 *)(glb_temp_rx_buff + 4);


        ph[0] = 0;

        /////////////////////////////////////////////////////////////////////////////////////////
        //here waiting for TX done,
        // ADV data 0  bytes, rf_len = 6+ 0 = 6,  TX time = (10+6) *8 = 128 uS
        // ADV data 31 bytes, rf_len = 6+31 = 37, TX time = (10+37)*8 = 376 uS
        // 128 ~ 376 uS can be used

        //set scanRsp packet here to save some time(if set after scanRsp is trigger, cost some time, DMA data may ERROR)


        //TODO need to calculate accuracy timeout time
        //2024.5.31 modify by qihang, change margin time 5us to 60us, margin time 60us same as legacy_adv code. 5us is not enough for CSEM IP.
        //2024.6.7 txdone_us == RF CRC last bit time, TX END IRQ = RF CRC last bit time + 15 us, margin time set 20us, is enough for CSEM IP.
        //ext adv module, timing is relatively tight, reducing margin time is meaningful .
        while (!HAL_GET_RF_TX_IRQ && (u32)(clock_time() - tx_begin_tick) < (txdone_us + 20) * SYSTEM_TIMER_TICK_1US) {
            if (usr_irq_handler_cb) {
                usr_irq_handler_cb();
            }
        }
        HAL_CLEAR_RF_TX_IRQ;
        if (blc_rf_pa_cb) {
            blc_rf_pa_cb(PA_TYPE_RX_ON);
        }

        while (!(*ph) && (u32)(clock_time() - tx_begin_tick) < connscan_us * SYSTEM_TIMER_TICK_1US) { //wait packet from master
            if (usr_irq_handler_cb) {
                usr_irq_handler_cb();
            }
        }


        if (*ph) {
            blt_ll_procLegacyRxPacket(prx, pmac);
        }
    }

    if (blc_rf_pa_cb) {
        blc_rf_pa_cb(PA_TYPE_OFF);
    }
    STOP_RF_STATE_MACHINE;


    return 0;
}

/**********************************************************************************************************************

            37                 38               39
       _____________     _____________    _____________
      |             |   |             |  |             |
      | ADV_EXT_IND |   | ADV_EXT_IND |  | ADV_EXT_IND |
      |_____________|   |_____________|  |_____________|
                                                               _____________         _____________       _____________
                                                              |             |       |             |     |             |
                                                              | AUX_ADV_IND |       |AUX_CHAIN_IND|     |AUX_CHAIN_IND|
                                                              |_____________|       |_____________|     |_____________|


*********************************************************************************************************************/


/***
 * when aux adv exceeds the current sSlot_endIdx_maxPri, need to find the max value from bltFutTask.task_tbl and
 * compare it with bltSche.sSlot_endIdx_maxPri to get the right sSlot idx.
 * note:bltFutTask.task_tbl[] store all aux adv tick_s/tick_e, no matter whether exceed the sSlot_endIdx_maxPri.
 * maybe consider the build processing.
 */
static inline int find_auxadv_sSlot_idx(void)
{
    u8  i = 0;
    int tmp_sSlot_auxadv[FUTURE_TASK_MAX_NUM];
    int tmp_sSlot_max = 0x00;
    int auxadv_sSlot;

    //find the max value from bltFutTask.task_tbl
    for (i = 0; i < bltFutTask.number; i++) {
        tmp_sSlot_auxadv[i] = (u32)(bltFutTask.task_tbl[i].tick_e - bltSche.sSlot_tick_start) * SSLOT_TICK_REVERSE;
        if (tmp_sSlot_auxadv[i] > tmp_sSlot_max) {
            tmp_sSlot_max = tmp_sSlot_auxadv[i];
        }
    }

    u32 random = (bltFutTask.number + 1) * 250 + trng_rand() % 500;

    //compare with bltSche.sSlot_endIdx_maxPri to get right sSlot idx.
    if (tmp_sSlot_max > bltSche.sSlot_endIdx_maxPri) {
        auxadv_sSlot = tmp_sSlot_max + random;
    } else {
        auxadv_sSlot = bltSche.sSlot_endIdx_maxPri + random;
    }

    return auxadv_sSlot;
}

    #define AUX_ALLOCATE_ON_LINKLIST     BIT(0)
    #define AUX_ALLOCATE_BEYOND_LINKLIST BIT(1)
    #define AUX_REPEAT_LEAD              BIT(2)

_attribute_ram_code_ int blt_send_extend_adv(void)
{
    if (blt_pextadv->with_aux_adv_ind) //with auxiliary packet
    {
        /* trigger STX ASAP, cause ADV delay timing is not very sufficient */
        STOP_RF_STATE_MACHINE;
        HAL_CLEAR_RF_TX_RX_IRQ;

        u32 tx_begin_tick = bltSche.system_irq_tick + bltAdv.advTxDly_us * SYSTEM_TIMER_TICK_1US;
        rf_start_fsm(FSM_STX, (void *)&blt_pextadv->primary_adv, tx_begin_tick);
        if (bltAdv.advChn_cnt == 0) {
            blt_extadv_rpa_update_process();
        }
        blt_adv_ext_ind_fill_advA_targetA();

        if (blc_rf_pa_cb) {
            blc_rf_pa_cb(PA_TYPE_TX_ON);
        }

        ll_adv_ext_ind_header_t *p_adv_ext_ind = (ll_adv_ext_ind_header_t *)&blt_pextadv->primary_adv;
        aux_ptr_t               *pCur_auxPtr   = (aux_ptr_t *)(p_adv_ext_ind->data + p_adv_ext_ind->auxPtr_offset);

        int AUX_task_allocate = 0;
        if (bltAdv.advChn_cnt == 0) //first ADV packet of an ADV event
        {
            s32         cur_sSlot_auxadv;
            u32         cur_tick_auxadv;
            sch_task_t *pTsk_cur = &blt_pextadv->auxadv_schTsk_fifo;

            if (blt_pextadv->aux_adv_pending) {
                AUX_task_allocate = AUX_REPEAT_LEAD;
                cur_sSlot_auxadv  = blt_pextadv->sSlot_aux_mark;
                //only aux_offset need change, other information on RF packet no need change
            } else {
                sch_task_t *pExtLkTsk_left, *pExtLkTsk_right;
                pExtLkTsk_left  = bltSche.pTask_cur;
                pExtLkTsk_right = bltSche.pTask_next;

                s32 sSlot_idx_left, sSlot_idx_right;

                while (1) {
                    sSlot_idx_left = pExtLkTsk_left->end + 1;

                    if (pExtLkTsk_right == NULL) {
                        sSlot_idx_right = bltSche.sSlot_endIdx_maxPri;
                        if ((s32)(sSlot_idx_right - sSlot_idx_left) > blt_pextadv->sSlotDuration_auxadv) {
                            AUX_task_allocate = AUX_ALLOCATE_ON_LINKLIST;
                            cur_sSlot_auxadv  = sSlot_idx_left;
                        } else {
                            cur_sSlot_auxadv = find_auxadv_sSlot_idx();
                            //cur_tick_auxadv = bltSche.lklt_endTick;
                            AUX_task_allocate = AUX_ALLOCATE_BEYOND_LINKLIST;
                            break;
                        }
                    } else {
                        sSlot_idx_right = pExtLkTsk_right->begin;
                        if ((s32)(sSlot_idx_right - sSlot_idx_left) > blt_pextadv->sSlotDuration_auxadv) { // ">=" is OK, use ">" here
                            AUX_task_allocate = AUX_ALLOCATE_ON_LINKLIST;
                            cur_sSlot_auxadv  = sSlot_idx_left;
                        }
                    }

                    if (AUX_task_allocate == AUX_ALLOCATE_ON_LINKLIST) {
                        pTsk_cur->begin = cur_sSlot_auxadv;
                        pTsk_cur->end   = cur_sSlot_auxadv + (blt_pextadv->sSlotDuration_auxadv - 1);

                        //insert ADV task to existed LinkList
                        pExtLkTsk_left->next = pTsk_cur;
                        pTsk_cur->next       = pExtLkTsk_right;

                        if (pExtLkTsk_left == bltSche.pTask_cur) { //match in first traverse
                            bltSche.pTask_next = pTsk_cur;
                        }

    #if (SLEV_eadv_aux_insert)
                        log_event_irq(SL_STACK_EXT_PRD_BASE_TIMING_EN, SLEV_eadv_aux_insert);
    #endif
                        //bltPri.csctvAbandonCnt[pTsk_cur->scheTask_oft] = 0;
                        break; //exit while 1
                    } else {   //traverse to next
                        pExtLkTsk_left  = pExtLkTsk_left->next;
                        pExtLkTsk_right = pExtLkTsk_right->next;
                    }

                    //SiHui question:here timing not very long
                    if (usr_irq_handler_cb) {
                        usr_irq_handler_cb();
                    }
                } /* the end of while() */
            }


            if (AUX_task_allocate) {
                /* calculate "n_30us_aux_ind", then calculate "aux_offset" */
                //TODO: rpa may need more time to process, 2 sslot : 40us
                //if -2: EBQ test the offset is always 40us larger than the true value. LL/DDI/ADV/BV-28-C
                //I don't think here computation has anything to do with RPA.so not need -2. CONFIRM WITH SIHUI.
                //u32 irq_distance_us = (cur_sSlot_auxadv - bltSche.sSlot_idx_irq_real - 2)*SSLOT_US_NUM;
                u32 irq_distance_us = (cur_sSlot_auxadv - bltSche.sSlot_idx_irq_real) * SSLOT_US_NUM;

                blt_pextadv->aux_align_dly_us = 30 - irq_distance_us % 30;
                blt_pextadv->n_30us_aux_ind   = (irq_distance_us + blt_pextadv->aux_align_dly_us) / 30; //Round up 1 unit
                pCur_auxPtr->aux_offset       = blt_pextadv->n_30us_aux_ind;                            // - 0 * blt_pextadv->n_30us_ext_ind


                //maybe question:Above AUX_ALLOCATE_ON_LINKLIST has already insert the linker, why here still need to add the future task.
                //reason: some situation may rebuild the task linker before aux_adv_ind task run.
                if (AUX_task_allocate & (AUX_ALLOCATE_ON_LINKLIST | AUX_ALLOCATE_BEYOND_LINKLIST)) {
                    blt_pextadv->aux_chn_idx = BLT_GENERATE_AUX_CHN; //new calculate
                    pCur_auxPtr->chn_index   = blt_pextadv->aux_chn_idx;

                    blt_pextadv->sSlot_aux_mark  = cur_sSlot_auxadv;
                    blt_pextadv->aux_adv_pending = 1; //set flag
                    blt_sche_addTaskMask(TSKMSK_AUX_ADV_0 << bltExtA.extadv_sel);
                    cur_tick_auxadv = bltSche.sSlot_tick_start + cur_sSlot_auxadv * SSLOT_TICK_NUM;
                    blt_add_future_task(TSKFLG_AUX_ADV, TSKOFT_AUX_ADV + bltExtA.extadv_sel, cur_tick_auxadv, cur_tick_auxadv + blt_pextadv->tickDuration_auxadv);
                }
            } else {
                //Abandon the current EXT_ADV_IND and subsequent Aux_Adv_Ind
                STOP_RF_STATE_MACHINE; //stop RF FSM
                bltAdv.advChn_cnt = blt_pextadv->adv_chn_num;
                CLEAR_ALL_RFIRQ_STATUS;
                bltPri.csctvAbandonCnt[pTsk_cur->scheTask_oft]++;

                if (blc_rf_pa_cb) {
                    blc_rf_pa_cb(PA_TYPE_OFF);
                }
                return 1; //exit while 1 and return function
            }
        } /* The end of if( bltAdv.advChn_cnt == 0) //first ADV packet of an ADV event */

        /* second or third ADV packet(channel 38 or channel 39) of an ADV event
         * only change "aux_offset", other data keep same as first packet */
        else {
            /* "n_30us_aux_ind" calculated when sending first packet
             * "n_30us_ext_ind" calculated when EXT_ADV_ENABLE  */
            pCur_auxPtr->aux_offset = blt_pextadv->n_30us_aux_ind - blt_pextadv->n_30us_ext_ind * bltAdv.advChn_cnt;
        }

        if (bltExtA.extadv_sel == 2) {
            my_dump_str_data(0, "adv_ext_ind", p_adv_ext_ind, p_adv_ext_ind->rf_len + 6);
        }

    #if (ADV_DURATION_STALL_EN) //TODO: power optimize
        cpu_stall_WakeUp_By_RF_SystemTick(IRQ_ZB_RT, FLD_RF_IRQ_TX, 0);
    #else
        //TODO need to calculate accuracy time  ---ADV_EXT_IND
        u32 tx_timeout_us = blt_phy_getRfPacketTime_us(blt_pextadv->primary_adv.rf_len, bltPHYs.cur_llPhy, LE_CODED_S8) + TX_STL_ADV_REAL_COMMON + 100;
        u32 tick_target   = tx_begin_tick + tx_timeout_us * SYSTEM_TIMER_TICK_1US;

        #if (DEBUG_RF_EXTADV_WHILE_EN)
            #if 1 //stuck in while, do not use timeout
        while (!HAL_GET_RF_TX_IRQ)
            ;
            #else //exit with timeout, but record the count
        while (!HAL_GET_RF_TX_IRQ) {
            if (tick1_exceed_tick2(clock_time(), tick_target)) {
                A_rf_while_debug.primary_extAdv_tmCnt++;
                break;
            }
        }
            #endif
        #else
        while (!HAL_GET_RF_TX_IRQ && tick1_exceed_tick2(tick_target, clock_time())) {
            if (usr_irq_handler_cb) {
                usr_irq_handler_cb();
            }
        }
        #endif
    #endif
    } else //no auxiliary packet: Non_Connectable Non_Scannable undirected/directed
    {
        blt_send_extend_no_aux_adv();
    }

    if (blc_rf_pa_cb) {
        blc_rf_pa_cb(PA_TYPE_OFF);
    }

    return 0;
}

_attribute_ram_code_ void blt_send_extend_no_aux_adv(void)
{
    STOP_RF_STATE_MACHINE; // stop SM
    HAL_CLEAR_RF_TX_RX_IRQ;

    u32 tx_begin_tick = clock_time() + 20;
    rf_start_fsm(FSM_STX, (void *)&blt_pextadv->primary_adv, tx_begin_tick);

    if (blc_rf_pa_cb) {
        blc_rf_pa_cb(PA_TYPE_TX_ON);
    }


    if (bltAdv.advChn_cnt == 0) {
        blt_extadv_rpa_update_process();
    }
    blt_adv_ext_ind_fill_advA_targetA();


    #if (ADV_DURATION_STALL_EN) //TODO: power optimize
    cpu_stall_WakeUp_By_RF_SystemTick(IRQ_ZB_RT, FLD_RF_IRQ_TX, 0);
    #else
    //TODO need to calculate accuracy time  ---ADV_EXT_IND
    u32 tx_timeout_us = blt_phy_getRfPacketTime_us(blt_pextadv->primary_adv.rf_len, bltPHYs.cur_llPhy, LE_CODED_S8) + TX_STL_ADV_REAL_COMMON + 100;
    while (!HAL_GET_RF_TX_IRQ && (u32)(clock_time() - tx_begin_tick) < tx_timeout_us * SYSTEM_TIMER_TICK_1US) {
        if (usr_irq_handler_cb) {
            usr_irq_handler_cb();
        }
    }
    #endif
}

/*
 * Currently Qiuwei has avoided it. When periodicAdv is enabled, the sinfo_info field is set immediately,
 * and the main_loop has been updated task duration (call blt_updateExtAdvSet), neither the syncinfo_changed
 * mechanism nor the blt_updateExtAdvSet call again in the interruption, TODO: optimize the latter
 */
//blt_updateExtAdvSet not need in Ramcode, not need to be called here.
void blt_updateExtAdvSet(st_ext_adv_t *cur_pextadv)
{
    //see <<Extended ADV Data Format.xlsx>>  ADV_EXT_IND
    if (cur_pextadv->legacy_adv) {
        u32 adv_without_rx_us;
        u32 adv_with_scanReq_no_accept; //advA not match or whitelist limited
        u32 adv_with_scanRsp_us;
        u32 adv_with_connReq_us;        // adv_with_connReq_us is same as adv_with_connReq_no_accept_us
        //u32 adv_with_rx_fail_us;

        //only 1M PHY can be used on legacy ADV
        u8 pri_pkt_rfLen;
        if (cur_pextadv->directed_adv) {
            pri_pkt_rfLen = 12;
        } else {
            pri_pkt_rfLen = cur_pextadv->curLen_advData + BLE_ADDR_LEN;
        }


        /* can only send on 1M, use (rf_len+10)*8 */
        u32 pkt_scanRsp_us = (cur_pextadv->curLen_scanRsp + BLE_ADDR_LEN + 10) * 8; //max: (37+10)*8=376

        u16 legAdv_tx_prepare_us = TX_STL_ADV_REAL_COMMON;
        if (cur_pextadv->pri_phy == BLE_PHY_1M) {
            cur_pextadv->pri_pkt_us = (pri_pkt_rfLen + 10) * 8;                       //max: (37+10)*8=376
        } else {                                                                      //coded phy
            cur_pextadv->pri_pkt_us = (pri_pkt_rfLen + 10) * 64;                      //S8
        }
        adv_without_rx_us = 50 + legAdv_tx_prepare_us + cur_pextadv->pri_pkt_us + 50; //pri_pkt_us + 216,  max:376+216=592

        //13: preamble(1) + accesscode(4) + dma(4) + margin(4)
        //150+104=254,  max:592+254=846
        //adv_with_rx_fail_us = adv_without_rx_us + 150 + 13*8;

        adv_with_scanReq_no_accept = adv_without_rx_us + (12 + 10) * 8 + 20; // + 196,  max:592+196=788

                                                                             //22*8=176, 300+176=476, max: adv_without_rx_us + 476 + pkt_scanRsp_us = 592+476+376=1444
        adv_with_scanRsp_us = adv_without_rx_us + 300 + (12 + 10) * 8 + pkt_scanRsp_us;

        //44*8=352, 150+352=502, max: adv_without_rx_us + 502 = 592+502=1094
        adv_with_connReq_us = adv_without_rx_us + 150 + (34 + 10) * 8;

        //we can see that adv_with_scanRsp_us is definitely bigger than adv_with_connReq_us


        /*   adv type           pkt_adv.type            SCAN_REQ    CONNECT_REQ
            ADV_IND             0 : LL_TYPE_ADV_IND             yes         yes
            ADV_DIRECT_IND      1 : LL_TYPE_ADV_DIRECT_IND      no          yes(*)      no need check whitelist
            ADV_NONCONN_IND     2 : LL_TYPE_ADV_NONCONN_IND     no          no
            ADV_SCAN_IND        6 : LL_TYPE_ADV_SCAN_IND        yes         no      */
        if (cur_pextadv->evt_prop_bit04 == ADV_EVT_PROP_LEGACY_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED) {
            //max: 3*592 = 1776
            cur_pextadv->pri_evt_us = cur_pextadv->adv_chn_num * adv_without_rx_us;
        } else if (cur_pextadv->evt_prop_bit04 == ADV_EVT_PROP_LEGACY_SCANNABLE_UNDIRECTED) {
            //method 1: send scan_rsp on each channel, max 3*1444 = 4332
            //cur_pextadv->pri_evt_us = cur_pextadv->adv_chn_num * adv_with_scanRsp_us;

            //method 2: if one channel OK, finish ADV event, max 1444 + 788*2 = 1444 + 1576 = 3020
            cur_pextadv->pri_evt_us = adv_with_scanRsp_us + (cur_pextadv->adv_chn_num - 1) * adv_with_scanReq_no_accept;
        } else if (cur_pextadv->evt_prop_bit04 & ADVEVT_PROP_MASK_DIRECTED) {
            //note that: direct ADV never send scan_rsp
            //max: 1094 * 3 =  3282
            cur_pextadv->pri_evt_us = adv_with_connReq_us * cur_pextadv->adv_chn_num;
        } else {
            //method 1: send scan_rsp on each channel, max 3*1444 = 4332
            //cur_pextadv->pri_evt_us = cur_pextadv->adv_chn_num * adv_with_scanRsp_us;

            //method 2: if one channel OK, finish ADV event, max 1444 + 1094*2 = 1444 + 2188 = 3632
            //note that: here "adv_with_connReq_us" means "adv_with_connReq_no_accept_us", they are same value
            cur_pextadv->pri_evt_us = adv_with_scanRsp_us + (cur_pextadv->adv_chn_num - 1) * adv_with_connReq_us;
        }

    } else { //Extended ADV

        /***********************  ADV_EXT_IND prepare  **********************************************************/
        //see <<Extended ADV Data Format.xlsx>>  ADV_EXT_IND


        int extended_header_len = 0;
        u8  extended_header_flg = 0;

        ll_adv_ext_ind_header_t *p_adv_ext_ind = (ll_adv_ext_ind_header_t *)&cur_pextadv->primary_adv;

        p_adv_ext_ind->type     = LL_TYPE_ADV_EXT_IND;
        p_adv_ext_ind->chan_sel = 0; //"ChSel" only valid in ADV_IND/ADV_DIRECT_IND/CONNECT_IND, other packet set 0'b
        p_adv_ext_ind->txAddr   = 0; //clear
        p_adv_ext_ind->rxAddr   = 0; //clear
        //rf_len calculate later
        //ext_hdr_len calculate later
        p_adv_ext_ind->adv_mode = cur_pextadv->cur_advMode;


        //2 type: Non_Connectable Non_Scannable without auxiliary packet, undirected and directed
        if (!cur_pextadv->cur_advMode) {
            if (cur_pextadv->curLen_advData || cur_pextadv->prdadv_api_en) {
                cur_pextadv->with_aux_adv_ind = 1;
            } else {
                cur_pextadv->with_aux_adv_ind = 0;
            }
        }
        // 6: Scannable(2), Connectable(2), Non_Connectable Non_Scannable with auxiliary packet(2)
        else {
            cur_pextadv->with_aux_adv_ind = 1;
        }

    #if(LL_FEATURE_ENABLE_DECISION_BASED_ADVERTISING_FILTER)
        if(cur_pextadv->useDecisionAdv){

            p_adv_ext_ind->type = LL_TYPE_ADV_DECISION_IND;
            //only if call the blc_ll_setDecisionData(), no matter whether the data length is zero.
            if(cur_pextadv->setDecisionDataFlag){
                cur_pextadv->with_aux_adv_ind = 1;
            }else{
                cur_pextadv->with_aux_adv_ind = 0;
            }
        }
    #endif

        //ADV_EXT_IND step 1: AdvA process
        if (cur_pextadv->useDecisionAdv || cur_pextadv->cur_advMode == LL_EXTADV_MODE_NON_CONN_NON_SCAN) {
            int advA_exist = 0;
    #if (ADV_EXT_IND_C1_ADVA_OPTIONAL_1M_PHY == FLAGS_INVALID)
            /* Non-Connectable and Non-Scannable Undirected */
            if (!cur_pextadv->with_aux_adv_ind) {
                advA_exist = 1;
            }
    #elif (ADV_EXT_IND_C1_ADVA_OPTIONAL_1M_PHY == FLAGS_VALID)
            /* Non-Connectable and Non-Scannable Undirected */
            /* Non-Connectable and Non-Scannable Directed 1M PHY */
            if (!cur_pextadv->with_aux_adv_ind || cur_pextadv->pri_phy == BLE_PHY_1M) {
                advA_exist = 1;
            }
    #endif

    #if(LL_FEATURE_ENABLE_DECISION_BASED_ADVERTISING_FILTER)
            if(cur_pextadv->useDecisionAdv && (cur_pextadv->evt_props&ADVEVT_PROP_MASK_DECISION_PDU_INC_ADVA) ){
                advA_exist = 1;
            }
    #endif

            if (advA_exist) {
                if (cur_pextadv->own_addr_type == OWN_ADDRESS_PUBLIC) {
                    p_adv_ext_ind->txAddr = BLE_ADDR_PUBLIC;
                    smemcpy((p_adv_ext_ind->data + 0), cur_pextadv->public_addr, BLE_ADDR_LEN);    //0:extended_header_len
                } else if (cur_pextadv->own_addr_type == OWN_ADDRESS_RANDOM) {
                    p_adv_ext_ind->txAddr = BLE_ADDR_RANDOM;
                    smemcpy((p_adv_ext_ind->data + 0), cur_pextadv->eAdv_rand_addr, BLE_ADDR_LEN); //0:extended_header_len
                } else {                                                                           //OWN_ADDRESS_RESOLVE_PRIVATE_PUBLIC / OWN_ADDRESS_RESOLVE_PRIVATE_RANDOM
                }
                extended_header_len += EXTHD_LEN_6_ADVA;
                extended_header_flg |= EXTHD_BIT_ADVA;
            }
        }


        //ADV_EXT_IND step 2: TargetA process
        /* Non-Connectable and Non-Scannable Undirected */
        if (cur_pextadv->evt_prop_bit04 == ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_DIRECTED) {
            int targetA_exist = 0;
    #if (ADV_EXT_IND_C1_TARGET_OPTIONAL_1M_PHY == FLAGS_INVALID)
            /* Non-Connectable and Non-Scannable Directed without auxiliary packet */
            if (!cur_pextadv->with_aux_adv_ind) {
                targetA_exist = 1;
            }
    #elif (ADV_EXT_IND_C1_TARGET_OPTIONAL_1M_PHY == FLAGS_VALID)
            /* Non-Connectable and Non-Scannable Directed without auxiliary packet */
            /* Non-Connectable and Non-Scannable Directed with auxiliary packet, 1M PHY */
            if (!cur_pextadv->with_aux_adv_ind || cur_pextadv->pri_phy == BLE_PHY_1M) {
                targetA_exist = 1;
            }
    #endif

            if (targetA_exist) {
                p_adv_ext_ind->rxAddr = cur_pextadv->eAdvParaCmd_peerAdrType;
                smemcpy((p_adv_ext_ind->data + extended_header_len), cur_pextadv->eAdvParaCmd_peerAddr, BLE_ADDR_LEN);
                extended_header_len += EXTHD_LEN_6_TARGETA;
                extended_header_flg |= EXTHD_BIT_TARGETA;
            }
        }

        //ADV_EXT_IND step 3: ADI & Aux Ptr process
        if (cur_pextadv->with_aux_adv_ind) {
            /*ADI
              u16   did :12;
              u16   sid : 4; */
            if(!cur_pextadv->useDecisionAdv || (cur_pextadv->useDecisionAdv && (cur_pextadv->evt_props&ADVEVT_PROP_MASK_DECISION_PDU_INC_ADI)) ){
                u16 adi_info = cur_pextadv->adv_sid << 12 | cur_pextadv->adv_did;
                smemcpy((p_adv_ext_ind->data + extended_header_len), &adi_info, EXTHD_LEN_2_ADI);
                extended_header_len += EXTHD_LEN_2_ADI;
                extended_header_flg |= EXTHD_BIT_ADI;
            }


            /* AuxPrt, only process part of AuxPtr, chn_index and aux_offset must calculate when sending ADV
            u8  chn_index    :6;
            u8  ca          :1;
            u8  offset_unit :1;
            u16 aux_offset  :13;
            u16 aux_phy     :3; */
            aux_ptr_t temp_auxPtr   = {0, 0, 0, 0, 0};
            temp_auxPtr.ca          = EXT_ADV_PDU_AUXPTR_CA_0_50_PPM;        // 0~50 ppm
            temp_auxPtr.offset_unit = EXT_ADV_PDU_AUXPTR_OFFSET_UNITS_30_US; //30us unit
            temp_auxPtr.aux_phy     = cur_pextadv->sec_phy - 1;              // le_phy_type_t 1/2/3 corresponding 0/1/2 in packet
            smemcpy((p_adv_ext_ind->data + extended_header_len), &temp_auxPtr, EXTHD_LEN_3_AUX_PTR);

            p_adv_ext_ind->auxPtr_offset = extended_header_len;              //mark position

            extended_header_len += EXTHD_LEN_3_AUX_PTR;
            extended_header_flg |= EXTHD_BIT_AUX_PTR;
        }

        //ADV_EXT_IND step 4: Tx Power process
        if (cur_pextadv->txPower_en_len) {
            /* Non-Connectable and Non-Scannable Undirected/Directed without auxiliary packet */
            /* Others: 1M PHY */
            if ((!cur_pextadv->cur_advMode && !cur_pextadv->with_aux_adv_ind) || cur_pextadv->pri_phy == BLE_PHY_1M) {
                //smemcpy( (p_adv_ext_ind->data + extended_header_len),&txPower_index, EXTHD_LEN_1_TX_POWER);
                p_adv_ext_ind->data[extended_header_len] = ble_txPowerLevel;
                extended_header_len += EXTHD_LEN_1_TX_POWER;
                extended_header_flg |= EXTHD_BIT_TX_POWER;
            }
        }
        p_adv_ext_ind->ext_hdr_flg = extended_header_flg;
        p_adv_ext_ind->ext_hdr_len = extended_header_len + 1;

        if(cur_pextadv->useDecisionAdv){
            //p_adv_ext_ind->decisionTypeFlag = cur_pextadv->decisionTypeFlag;
            p_adv_ext_ind->ext_hdr_len = cur_pextadv->decisionTypeFlag;
            p_adv_ext_ind->rf_len = extended_header_len + 2 + cur_pextadv->setDecisionDataLen;
        }else{
            p_adv_ext_ind->rf_len = extended_header_len + 2;
        }

        

        //p_adv_ext_ind->dma_len = extended_header_len + 4;
        if(cur_pextadv->useDecisionAdv){
            p_adv_ext_ind->dma_len = rf_tx_packet_dma_len(extended_header_len + 4 + cur_pextadv->setDecisionDataLen);
        }else{
            p_adv_ext_ind->dma_len = rf_tx_packet_dma_len(extended_header_len + 4);
        }
        


        /*
         *     PHYs           timing(uS)
         *   1M PHY   :    (rf_len + 10) * 8,      // 10 = 1(BLE preamble) + 9(accesscode 4 + crc 3 + header 2)
         *   2M PHY   :    (rf_len + 11) * 4       // 11 = 2(BLE preamble) + 9(accesscode 4 + crc 3 + header 2)
         *  Coded PHY :    376 + (rf_len*8+43)*S        // 376uS = 80uS(preamble) + 256uS(Access Code) + 16uS(CI) + 24uS(TERM1)
         */
        //1M/Coded PHY can be used on extended ADV primary channel
        cur_pextadv->pri_pkt_us = blt_phy_getRfPacketTime_us(p_adv_ext_ind->rf_len, cur_pextadv->pri_phy, cur_pextadv->coding_ind);


        if (cur_pextadv->with_aux_adv_ind) {
            /* Single Ext Adv pkt length */
            /*
             * note: fanqh +30 us for usb IRQ that have the same interrupt priority, This causes the value of capture to be set to the past tick
             */
            cur_pextadv->pri_single_chn_us = bltAdv.advTxDly_us + TX_STL_ADV_REAL_COMMON + cur_pextadv->pri_pkt_us + 20 + 30; //20: + 57

            int align_us = 30 - (cur_pextadv->pri_single_chn_us % 30);
            cur_pextadv->pri_single_chn_us += align_us;
            cur_pextadv->n_30us_ext_ind = cur_pextadv->pri_single_chn_us / 30;
            cur_pextadv->pri_evt_us     = cur_pextadv->adv_chn_num * cur_pextadv->pri_single_chn_us - align_us + ADV_TAIL_MARGIN_US;
        } else {
            //10: sw_delay   20: margin
            cur_pextadv->pri_evt_us = (10 + TX_STL_ADV_REAL_COMMON + cur_pextadv->pri_pkt_us + 20) * cur_pextadv->adv_chn_num;
        }

        /***********************  AUX_ADV_IND prepare  **************************************************************/
        u8 acad_en_len = 0;
    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
        if (cur_pextadv->acad_used & PERD_ACAD_PAwR_ENA) {
            acad_en_len = 10; //fixed length:1+1+8
            my_dump_str_data(0, "---pawr_timing_info", 0, 0);
        }
    #endif

        /* see <<Extended ADV Data Format.xlsx>>  AUX_ADV_IND
        //6 type: Scannable(2), Connectable(2), Non_Connectable Non_Scannable with auxiliary packet(2) */
        if (cur_pextadv->with_aux_adv_ind) //Scannable, Connectable, Non_Connectable Non_Scannable with auxiliary packet
        {
            extended_header_len = 0;
            extended_header_flg = 0;

            ll_aux_adv_ind_header_t *p_aux_adv_ind = (ll_aux_adv_ind_header_t *)&cur_pextadv->aux_adv_1stPkt;


            p_aux_adv_ind->type = LL_TYPE_AUX_ADV_IND;
            //p_aux_adv_ind->chan_sel = 0;  //"ChSel" only valid in ADV_IND/ADV_DIRECT_IND/CONNECT_IND, other packet set 0'b
            p_aux_adv_ind->txAddr = 0; //clear
            p_aux_adv_ind->rxAddr = 0; //clear
            //rf_len calculate later
            //ext_hdr_len calculate later
            p_aux_adv_ind->adv_mode = cur_pextadv->cur_advMode;

            //AUX_ADV_IND step 1: AdvA process
            int advA_exist = 0;
            if (cur_pextadv->cur_advMode) {             //Connectable & Scannable
                advA_exist = 1;
            } else if (cur_pextadv->with_aux_adv_ind) { //Non_Connectable Non_Scannable with auxiliary packet
    /* C4 This field is optional if the corresponding field in the ADV_EXT_IND PDU is not present,
                   otherwise it is reserved for future use. */
    #if (ADV_EXT_IND_C1_ADVA_OPTIONAL_1M_PHY == FLAGS_INVALID)
                if (!(cur_pextadv->evt_props & ADVEVT_PROP_MASK_ANON_ADV)) {
                    advA_exist = 1;
                }
    #elif (ADV_EXT_IND_C1_ADVA_OPTIONAL_1M_PHY == FLAGS_VALID)
                    //do nothing
    #endif
            }

            if (advA_exist) {
                if (cur_pextadv->own_addr_type == OWN_ADDRESS_PUBLIC) {
                    p_aux_adv_ind->txAddr = BLE_ADDR_PUBLIC;
                    smemcpy((p_aux_adv_ind->data + 0), cur_pextadv->public_addr, BLE_ADDR_LEN);    //0:extended_header_len
                } else if (cur_pextadv->own_addr_type == OWN_ADDRESS_RANDOM) {
                    p_aux_adv_ind->txAddr = BLE_ADDR_RANDOM;
                    smemcpy((p_aux_adv_ind->data + 0), cur_pextadv->eAdv_rand_addr, BLE_ADDR_LEN); //0:extended_header_len
                } else {                                                                           //OWN_ADDRESS_RESOLVE_PRIVATE_PUBLIC / OWN_ADDRESS_RESOLVE_PRIVATE_RANDOM
                }

                extended_header_len += EXTHD_LEN_6_ADVA;
                extended_header_flg |= EXTHD_BIT_ADVA;
            }

            //AUX_ADV_IND step 2: TargetA process
            int targetA_exist = 0;
            /* C2 This field is mandatory if the corresponding field in the ADV_EXT_IND PDU is not present,
               otherwise it is reserved for future use. */
            if (cur_pextadv->directed_adv) {
    #if (ADV_EXT_IND_C1_TARGET_OPTIONAL_1M_PHY == FLAGS_INVALID)
                targetA_exist = 1;
    #elif (ADV_EXT_IND_C1_TARGET_OPTIONAL_1M_PHY == FLAGS_VALID)
                if (cur_pextadv->cur_advMode) {
                    targetA_exist = 1;
                }
    #endif
            }

            if (targetA_exist) {
                p_aux_adv_ind->rxAddr = cur_pextadv->eAdvParaCmd_peerAdrType;
                smemcpy((p_aux_adv_ind->data + extended_header_len), cur_pextadv->eAdvParaCmd_peerAddr, BLE_ADDR_LEN);

                extended_header_len += EXTHD_LEN_6_TARGETA;
                extended_header_flg |= EXTHD_BIT_TARGETA;
            }

            //AUX_ADV_IND step 4: ADI process
            u16 adi_info = cur_pextadv->adv_sid << 12 | cur_pextadv->adv_did;
            smemcpy((p_aux_adv_ind->data + extended_header_len), &adi_info, EXTHD_LEN_2_ADI);
            extended_header_len += EXTHD_LEN_2_ADI;
            extended_header_flg |= EXTHD_BIT_ADI;


            //AUX_ADV_IND step 5: Aux Ptr process
            /* AuxPrt, only process part of AuxPtr, chn_index and aux_offset must calculate when sending ADV
            u8  chn_index    :6;
            u8  ca          :1;
            u8  offset_unit :1;
            u16 aux_offset  :13;
            u16 aux_phy     :3; */
            if (!cur_pextadv->cur_advMode) { //Non_Connectable Non_Scannable

                //ext_adv and periodic_adv are all enabled: The Sync Packet Offset field exist
                u8 syncInfo_len = cur_pextadv->syncinfo_used ? EXTHD_LEN_18_SYNC_INFO : 0;

                /*when code run here, TX_POWER did not calculated in extended_header_len, so here should consider its length*/
                int max_advData = 253 - extended_header_len - cur_pextadv->txPower_en_len - syncInfo_len - acad_en_len;

                if (cur_pextadv->curLen_advData > max_advData) { //with AUX_CHAIN_IND
                    cur_pextadv->with_aux_chain_ind  = 1;
                    cur_pextadv->aux_1st_pkt_dataLen = max_advData - EXTHD_LEN_3_AUX_PTR;

                    p_aux_adv_ind->auxPtr_offset = extended_header_len; //mark position
                    extended_header_len += EXTHD_LEN_3_AUX_PTR;
                    extended_header_flg |= EXTHD_BIT_AUX_PTR;
                } else {                                                //no AUX_CHAIN_IND, no Aux_Ptr
                    cur_pextadv->with_aux_chain_ind  = 0;
                    cur_pextadv->aux_1st_pkt_dataLen = cur_pextadv->curLen_advData;
                }

                //AUX_ADV_IND step 6: Sync Info process
                if (cur_pextadv->syncinfo_used) {
                    p_aux_adv_ind->syncInfo_offset = extended_header_len; //mark position
                    extended_header_len += EXTHD_LEN_18_SYNC_INFO;
                    extended_header_flg |= EXTHD_BIT_SYNC_INFO;
                }
            }

            //AUX_ADV_IND step 7: Tx Power process
            if (cur_pextadv->txPower_en_len) {
                //s8 tx_power = 3; //debug
                //smemcpy( (p_aux_adv_ind->data + extended_header_len), &tx_power, EXTHD_LEN_1_TX_POWER);
                p_aux_adv_ind->data[extended_header_len] = ble_txPowerLevel;
                extended_header_len += EXTHD_LEN_1_TX_POWER;
                extended_header_flg |= EXTHD_BIT_TX_POWER;
            }

            cur_pextadv->txPower_offset = extended_header_len; //Mark the start offset of the ACAD field

            //AUX_ADV_IND step 8: ACAD Info process
            if (acad_en_len) {
                //Only need update before send AUX_ADV_IND PKT !!!
                extended_header_len += acad_en_len; //ACAD_len(varies length)
            }

            int aux_rf_len, last_pkt_rf_len, last_pkt_tx_us;
            int available_data_len, cur_data_len = 0, auxChainInd_extendHeaderLen;

            /* step 1 for all : ADI process, all AUX packet have "ADI" */
            u16 auxAdv_tx_prepare_us = TX_STL_ADV_REAL_COMMON;
            if (cur_pextadv->cur_advMode) //Connectable & Scannable
            {
                /*
                 *   T_ifs         RX dma 1st data(Preamble 1B, access_code 4B, 1st dma 4B + margin 4B )
                 *   150 uS            104uS
                 */
                /*
                 *   1M PHY   :    (rf_len + 10) * 8,      // 10 = 1(BLE preamble) + 9(accesscode 4 + crc 3 + header 2)
                 *   2M PHY   :    (rf_len + 11) * 4       // 11 = 2(BLE preamble) + 9(accesscode 4 + crc 3 + header 2)
                 *  Coded PHY :    376 + (rf_len*8+43)*S // 376uS = 80uS(preamble) + 256uS(Access Code) + 16uS(CI) + 24uS(TERM1)
                */
                //rx 1st pdu length margin: 2 word(4B) => 1M:64us, 2M:32, S2:
                //rx finish tail margin: 30us
                if (cur_pextadv->sec_phy == BLE_PHY_1M) {
                    cur_pextadv->rx_1st_pdu_us = 150 + 5 * 8 + 64; //5: preamble(1) + access_code(4)
                    cur_pextadv->rx_finish_us  = 150 + AUX_CONN_REQ_1MPHY_US + 30;
                } else if (cur_pextadv->sec_phy == BLE_PHY_2M) {
                    cur_pextadv->rx_1st_pdu_us = 150 + 6 * 4 + 32; //6: preamble(2) + access_code(4)
                    cur_pextadv->rx_finish_us  = 150 + AUX_CONN_REQ_2MPHY_US + 30;
                } else {                                           //Coded PHY   if (cur_pextadv->sec_phy  == BLE_PHY_CODED){
                    //  80 + 256 + 16 + 24 = 376
                    //                      cur_pextadv->rx_1st_pdu_us  = 150 + 376 + (8*8)*cur_pextadv->coding_ind;
                    //                      cur_pextadv->rx_finish_us   = 150 + 376 + (34*8+43)*cur_pextadv->coding_ind + 30;

                    //The following assumptions need to be made: Aux_Conn_Req (S8) sent over
                    //If the Aux_Adv we send uses S2, but the mobile phone uses S8 Aux_Conn_req to connect,
                    //if it is not set to the maximum condition (using S8), an error will occur.
                    cur_pextadv->rx_1st_pdu_us = 150 + 376 + (8 * 8) * LE_CODED_S8;
                    cur_pextadv->rx_finish_us  = 150 + 376 + (34 * 8 + 43) * LE_CODED_S8 + 30;
                }


                /*aux_evt_us part 1 for Connectable & Scannable: common timing part*/
                cur_pextadv->aux_evt_us = bltAdv.advTxDly_us + 29 + auxAdv_tx_prepare_us + BLE_T_IFS * 2 + ADV_TAIL_MARGIN_US; //align_delay max value 29uS

                if (cur_pextadv->cur_advMode == LL_EXTADV_MODE_CONN)                                                           //Connectable
                {
                    /* ACAD(Optional): now do not know how to use, here consider 0
                     * ADV data(Optional): we think that ADV data is limited, rf_len can exceed 255 */

                    aux_rf_len = 2 + extended_header_len + cur_pextadv->curLen_advData;                             //2:  ext_hdr_len & adv_mode(1B) + ext_hdr_flg(1B)

                    int auxAdvData_extHeaderLen = EXTHD_LEN_6_ADVA + EXTHD_LEN_2_ADI + cur_pextadv->txPower_en_len; //ACAD_len
                    int max_advData             = 253 - auxAdvData_extHeaderLen;

                    cur_pextadv->with_aux_chain_ind = 0;

                    if (cur_pextadv->curLen_advData > max_advData) {
                        cur_pextadv->aux_1st_pkt_dataLen = max_advData;
                    } else {
                        cur_pextadv->aux_1st_pkt_dataLen = cur_pextadv->curLen_advData;
                    }

                    /*aux_evt_us part 2 for Connectable : aux_adv_ind + aux_connect_req + aux_connect_rsp */
                    if (cur_pextadv->sec_phy == BLE_PHY_1M) {
                        cur_pextadv->aux_evt_us += (aux_rf_len + 10) * 8 + AUX_CONN_REQ_1MPHY_US + AUX_CONN_RSP_1MPHY_US;
                    } else if (cur_pextadv->sec_phy == BLE_PHY_2M) {
                        cur_pextadv->aux_evt_us += (aux_rf_len + 11) * 4 + AUX_CONN_REQ_2MPHY_US + AUX_CONN_RSP_2MPHY_US;
                    } else { //Coded PHY
                        /* aux_conn_req: 376 + (34*8+43)*S
                         * aux_conn_rsp: 376 + (14*8+43)*S
                         * aux_conn_req + aux_conn_rsp = 752 + (315+155)*S = 752 + 470*S
                         * aux_conn_req + aux_conn_rsp + aux_adv = (752+470) + (rf_len*8 + 454 + 43)
                         *                                       = 1222 + (rf_len + 497)*S  */
                        //                          cur_pextadv->aux_evt_us += (aux_rf_len * 8 + 43) * cur_pextadv->coding_ind + 376 + ;
                        cur_pextadv->aux_evt_us += 1222 + (aux_rf_len * 8 + 497) * cur_pextadv->coding_ind;
                    }
                } else //Scannable
                {
                    /* ACAD(Optional): now do not know how to use, here consider 0
                     * ADV data(X): */
                    aux_rf_len = 2 + extended_header_len;                                         //2:  ext_hdr_len & adv_mode(1B) + ext_hdr_flg(1B)

                    cur_pextadv->chain_ind_num = 1;                                               //important

                    int auxScanRsp_extHeaderLen = EXTHD_LEN_6_ADVA + cur_pextadv->txPower_en_len; //ACAD_len
                    int max_advData             = 253 - auxScanRsp_extHeaderLen;

                    //EXT Scan - Aux adv: AdvA | ADI | TxPower(O) | ACAD(O)
                    cur_pextadv->aux_1st_pkt_dataLen = 0;            //must important!!! AUX_ADV_IND has no AuxPtr

                    if (cur_pextadv->curLen_scanRsp > max_advData) { //with AUX_CHAIN_IND
                        cur_pextadv->with_aux_chain_ind = 1;

                        cur_pextadv->chain_ind_dataLen[0] = max_advData - EXTHD_LEN_3_AUX_PTR;

                        /* AUX_CHAIN_IND after AUX_SCAN_RSP:  Aux Ptr(3) optional;  Tx Power(1) optional */
                        auxChainInd_extendHeaderLen = cur_pextadv->txPower_en_len;
                        available_data_len          = 253 - auxChainInd_extendHeaderLen;
                        int restScanRspData_len     = cur_pextadv->curLen_scanRsp - cur_pextadv->chain_ind_dataLen[0];
                        while (restScanRspData_len > 0) {
                            if (restScanRspData_len > available_data_len) {
                                cur_data_len = available_data_len - EXTHD_LEN_3_AUX_PTR;
                            } else {
                                cur_data_len = restScanRspData_len;
                            }

                            cur_pextadv->chain_ind_dataLen[cur_pextadv->chain_ind_num++] = cur_data_len;

                            restScanRspData_len -= cur_data_len;
                        }

                        last_pkt_rf_len = cur_data_len + auxChainInd_extendHeaderLen + 2;
                    } else { //no AUX_CHAIN_IND, no Aux_Ptr
                        /* last_packet is aux_scan_rsp, no any aux_chain_ind */
                        cur_pextadv->with_aux_chain_ind   = 0;
                        cur_pextadv->chain_ind_dataLen[0] = cur_pextadv->curLen_scanRsp;
                        last_pkt_rf_len                   = 2 + auxScanRsp_extHeaderLen + cur_pextadv->curLen_scanRsp;
                    }

                    /*aux_evt_us part 2 for Scannable : all rf_len 255 packet timing, number is (chain_ind_num - 1) */
                    cur_pextadv->aux_evt_us += cur_pextadv->rfLen_255_pkt_us * (cur_pextadv->chain_ind_num - 1); //attention: number - 1

                    /*aux_evt_us part 3 for Scannable : aux_adv_ind + last_packet(maybe aux_scan_rsp or last aux_chain_ind) */
                    if (cur_pextadv->sec_phy == BLE_PHY_1M) {
                        cur_pextadv->aux_evt_us += (aux_rf_len + last_pkt_rf_len + 20) * 8;
                    } else if (cur_pextadv->sec_phy == BLE_PHY_2M) {
                        cur_pextadv->aux_evt_us += (aux_rf_len + last_pkt_rf_len + 22) * 4;
                    } else { //Coded PHY
                        // 376 + (rf_len*8+43)*S
                        //cur_pextadv->aux_evt_us += 752 + ((aux_rf_len + last_pkt_rf_len) * 8 + 86) * cur_pextadv->coding_ind;
                        cur_pextadv->aux_evt_us += 752 + ((aux_rf_len + last_pkt_rf_len + 23) * 8 + 86) * cur_pextadv->coding_ind; //23 is scan request length
                    }
                }
            } else                                                                                                                 //if(!cur_pextadv->cur_advMode) //Non_Connectable Non_Scannable with auxiliary packet
            {
                /* Here we consider: for Sync Info & Aux Ptr, at most only one will be valid */
                if (0) {
                } else {
                    cur_pextadv->chain_ind_num = 0;
                    if (cur_pextadv->with_aux_chain_ind) { //with AUX_CHAIN_IND


    #if (0)                                                //debug
                        if ((extended_header_len + 2 + cur_pextadv->aux_1st_pkt_dataLen) != 255) {
                            ADV_ERR_DEBUG(0x77020000 | extended_header_len << 8 | cur_pextadv->aux_1st_pkt_dataLen);
                        }
    #endif


                        auxChainInd_extendHeaderLen = EXTHD_LEN_2_ADI + cur_pextadv->txPower_en_len;
                        available_data_len          = 253 - auxChainInd_extendHeaderLen;
                        /* AUX_CHAIN_IND after AUX_ADV_IND: ADI(2) must;  Aux Ptr(3) optional;  Tx Power(1) optional */

                        int restAdvData_len = cur_pextadv->curLen_advData - cur_pextadv->aux_1st_pkt_dataLen;
                        while (restAdvData_len > 0) {
                            if (restAdvData_len > available_data_len) {
                                cur_data_len = available_data_len - EXTHD_LEN_3_AUX_PTR;
                            } else {
                                cur_data_len = restAdvData_len;
                            }

                            cur_pextadv->chain_ind_dataLen[cur_pextadv->chain_ind_num++] = cur_data_len;

                            restAdvData_len -= cur_data_len;
                        }

                        last_pkt_rf_len = cur_data_len + auxChainInd_extendHeaderLen + 2;

                        //my_dump_str_data(0, "last_pkt_rf_len", &last_pkt_rf_len, 4);
                    } else { //no AUX_CHAIN_IND
                        last_pkt_rf_len = cur_pextadv->curLen_advData + extended_header_len + 2;
                    }
                    last_pkt_tx_us = blt_phy_getRfPacketTime_us(last_pkt_rf_len, cur_pextadv->sec_phy, cur_pextadv->coding_ind);


                    /* rfLen_255_pkt_us has been calculated when set ext_adv_parameters, here use it directly */
                    cur_pextadv->aux_evt_us = bltAdv.advTxDly_us + 29 + auxAdv_tx_prepare_us +
                                              cur_pextadv->rfLen_255_pkt_us * cur_pextadv->chain_ind_num + last_pkt_tx_us + ADV_TAIL_MARGIN_US; //align_delay max value 29uS
                }
            }


            p_aux_adv_ind->ACAD_advData_offset = extended_header_len;
            p_aux_adv_ind->ext_hdr_flg         = extended_header_flg;
            p_aux_adv_ind->ext_hdr_len         = extended_header_len + 1;
            p_aux_adv_ind->rf_len              = extended_header_len + 2 + cur_pextadv->aux_1st_pkt_dataLen;
            //p_aux_adv_ind->dma_len = p_aux_adv_ind->rf_len + 2;
            p_aux_adv_ind->dma_len = rf_tx_packet_dma_len(p_aux_adv_ind->rf_len + 2);


            /* Priority preset value */
            //blt_ll_set_interval_level(TSKOFT_AUX_ADV + cur_pextadv->extadv_index, cur_pextadv->advInt_max>>1); //remove not use ,fixed priority
            blt_ll_setSchedulerTaskPriority(TSKOFT_AUX_ADV + cur_pextadv->extadv_index, TASK_PRIORITY_AUX_ADV);


        } //end of if(cur_pextadv->with_aux_adv_ind)
        else {
            cur_pextadv->aux_evt_us = 0;
        }

    } //end of Extended ADV


    cur_pextadv->sSlotDuration_extadv = (cur_pextadv->pri_evt_us * SYSTEM_TIMER_TICK_1US + SLOT_PROCESS_MAX_TICK) * SSLOT_TICK_REVERSE; //neglect  "+1"
    cur_pextadv->tickDuration_auxadv  = cur_pextadv->aux_evt_us * SYSTEM_TIMER_TICK_1US + SLOT_PROCESS_MAX_TICK;
    cur_pextadv->sSlotDuration_auxadv = cur_pextadv->tickDuration_auxadv * SSLOT_TICK_REVERSE;
    cur_pextadv->bSlotDuration_auxadv = (cur_pextadv->sSlotDuration_auxadv >> 5);                                                       // + 1;  //bSlot duration is used for periodic ADV
}

st_ext_adv_t *blt_extadv_search_existed_and_allocate_new_adv_set(u8 advHandle)
{
    st_ext_adv_t *available_pextadv = NULL;
    st_ext_adv_t *cur_pextadv;
    for (int i = 0; i < bltExtA.maxNum_advSets; i++) {
        cur_pextadv = (st_ext_adv_t *)(global_pextadv + i);
        if (cur_pextadv->adv_handle == advHandle) { //existing ADV set match
            return cur_pextadv;
        } else if (cur_pextadv->adv_handle == INVALID_ADVHD_FLAG) {
            /* find new available ADV SET */
            if (!available_pextadv) {
                available_pextadv = cur_pextadv;
                //TODO: blt_clearAdvSetsParam
            }
        }
    }

    if (available_pextadv) {
        available_pextadv->adv_handle = advHandle;
        blt_extadv_clear_adv_set_param(available_pextadv); //very important
    }

    return available_pextadv;
}

st_ext_adv_t *blt_extadv_search_existed_adv_set(u8 advHandle)
{
    st_ext_adv_t *cur_pextadv;
    for (int i = 0; i < bltExtA.maxNum_advSets; i++) {
        cur_pextadv = (st_ext_adv_t *)(global_pextadv + i);
        if (cur_pextadv->adv_handle == advHandle) { //existing ADV set match
            return cur_pextadv;
        }
    }

    return NULL;
}

//int AA_cnt = 0;
//int AA_bSlot_mark;
//int AA_sSlot_mark;
//int AA_diff_adv;
//int AA_diff_next;

_attribute_ram_code_ int blt_ll_build_extadv_task(void)
{
    #if (PRICHN_SCAN_SMALL_INTV_WITH_ANOTHER_BIG_INTV_ADV_RESOLVE_EN && ANOTHER_BIG_INTV_EXTENDED_ADV)
    bltAdScn.legadv_alloc = 0;
    bltAdScn.extAdv_num   = 0;
    #endif


    st_ext_adv_t *cur_pextadv; //attention: do not use global "cur_pextadv", in case of mainloop & IRQ conflict
    for (int ii = 0; ii < bltExtA.maxNum_advSets; ii++) {
        if (bltSche.task_mask & (TSKMSK_EXT_ADV_0 << ii)) {
            cur_pextadv                     = (st_ext_adv_t *)(global_pextadv + ii);
            cur_pextadv->extadv_change_flag = 0; //important
            s32 sSlot_mark_extadv;

    #if (PRICHN_SCAN_SMALL_INTV_WITH_ANOTHER_BIG_INTV_ADV_RESOLVE_EN && ANOTHER_BIG_INTV_EXTENDED_ADV)
            bltAdScn.extAdv_num++;
            bltAdScn.extAdv_legacyMode = cur_pextadv->legacy_adv;
            bltAdScn.legadv_int        = cur_pextadv->advInt_min;
    #endif

            if (cur_pextadv->syncinfo_changed == 2) {
                cur_pextadv->syncinfo_changed = 0;
            }

            u32 bSlot_distance = (u32)(bltSche.bSlot_idx_next - cur_pextadv->bSlot_mark_extadv);
            if ((u32)(bSlot_distance - 2) > cur_pextadv->advInt_maxAddRandom) { //two area: >x  || <0
                sSlot_mark_extadv = bltSche.sSlot_idx_next - cur_pextadv->advInt_maxAddRandom * 32;
            } else {
                sSlot_mark_extadv = bltSche.sSlot_idx_next - bSlot_distance * 32 + bltSche.sSlot_diff_next + cur_pextadv->sSlot_diff_extadv;
            }


            sch_task_t *pTsk_cur, *pExtLkTsk_left, *pExtLkTsk_right;


            pExtLkTsk_left  = bltSche.pTask_head;
            pExtLkTsk_right = bltSche.pTask_head->next;

            int allocate_sSlot_adv;
            int cur_sSlot_adv = sSlot_mark_extadv; //must use s32


            s32 adv_min_left, adv_min_right, adv_max_left, adv_max_right;
            s32 sSlot_idx_left, sSlot_idx_right;

            for (int jj = 0; jj < EXT_ADV_FIFONUM; jj++) {
                //first, we use minimum timing to find available timing block
                adv_min_left  = (s32)(cur_sSlot_adv + cur_pextadv->advInt_min * 32); //s32
                adv_min_right = adv_min_left + cur_pextadv->sSlotDuration_extadv;

                /* current ADV task exceed time line, this function can finish */
                if (adv_min_right > bltSche.sSlot_endIdx_dft) {
                    //return 0;
                    goto extadv_loop_end;
                }

    //Old comment:2022.7.7 LL/DDI/ADV/BV-45-C: min_interval = 20ms; max_interval = 45.625ms.the actual interval not larger than max_interval.
    //New comment:before when test EBQ case LL/DDI/ADV/BV-45-C, the real interval can not larger than max interval.i.e old comment.
    //But on 2022.10.26 test the case again(cause resolve profile case--BASS/SR/CP/BV-06-C from Qihang),
    //although real interval is larger than max interval(close LL_DDI_ADV_BV45C), the case still pass stably.
    //Now use macro to distinguish, later need to sync with sihui how to understand min_interval/max_interval/advInterval+advDelay.
    //Now still use the sihui's method.
    #if (LL_DDI_ADV_BV45C)
                if (cur_pextadv->advInt_diff) {
                    adv_max_left = adv_min_left + cur_pextadv->advInt_diff * 32;
                } else {
                    adv_max_left = adv_min_left + bltAdv.delay_sSlot_value;
                }
    #else
                adv_max_left = adv_min_left + cur_pextadv->advInt_diff * 32 + bltAdv.delay_sSlot_value;
    #endif
                adv_max_right = adv_max_left + cur_pextadv->sSlotDuration_extadv;


    #if 0 //debug
                if(0 && AA_cnt == 2){
                    AA_bSlot_mark = cur_pextadv->bSlot_mark_extadv;
                    AA_sSlot_mark = sSlot_mark_extadv;
                    AA_diff_adv = cur_pextadv->sSlot_diff_extadv;
                    AA_diff_next = bltSche.sSlot_diff_next;

                    write_reg8(0x40000, 0x33);
                    while(1);
                    write_reg8(0x40000, 0x22);
                }
    #endif

                while (1) {
                    int ADV_task_allocate = 0;

                    if (pExtLkTsk_left == bltSche.pTask_head) {
                        sSlot_idx_left = bltSche.sSlot_idx_next;
                    } else {
                        sSlot_idx_left = pExtLkTsk_left->end + 1;
                    }

                    if (pExtLkTsk_right == NULL) {
                        sSlot_idx_right = bltSche.sSlot_endIdx_dft;
                    } else {
                        sSlot_idx_right = pExtLkTsk_right->begin;
                    }


                    /* very quick to judge if current idle timing block is behind ADV task */
                    if (sSlot_idx_right < adv_min_right) {
                        ADV_task_allocate = 0; //not allocate, traverse to next

                    }
                    /* Case 1 */
                    /* compensate match, ADV timing exceed, must insert here, do not care delay */
                    else if (sSlot_idx_left >= adv_max_left) {
                        ADV_task_allocate  = 1;
                        allocate_sSlot_adv = sSlot_idx_left; //left aligned

                    } else {                                 //window match, but we need find a proper subset window for ADV

                        ADV_task_allocate = 1;

                        //                          sSlot_idx_left  = sSlot_idx_left > adv_min_left ? sSlot_idx_left : adv_min_left;
                        //                          sSlot_idx_right = sSlot_idx_right < adv_max_right ? sSlot_idx_right : adv_max_right;
                        //                          if( (s32)(sSlot_idx_right - sSlot_idx_left) >= cur_pextadv->sSlotDuration_extadv ){
                        //                              ADV_task_allocate = 1;
                        //                          }

                        /* Case 4 & 5: left aligned */
                        if (sSlot_idx_left > adv_min_left) {
                            //sSlot_idx_left = sSlot_idx_left;
                            sSlot_idx_right = sSlot_idx_right < adv_max_right ? sSlot_idx_right : adv_max_right;

                            allocate_sSlot_adv = sSlot_idx_left;
                        } else { // sSlot_idx_left <= adv_min_left
                            sSlot_idx_left = adv_min_left;
                            /* Case 3: right aligned */
                            if (sSlot_idx_right < adv_max_right) {
                                //sSlot_idx_right = sSlot_idx_right;
                                allocate_sSlot_adv = sSlot_idx_right - cur_pextadv->sSlotDuration_extadv;
                            }
                            /* Case 2: timing enough, use random */
                            else { // sSlot_idx_right >= adv_max_right
                                sSlot_idx_right = adv_max_right;
    #if (LL_DDI_ADV_BV45C)
                                u16 random = cur_pextadv->advInt_diff ? (clock_time() % (cur_pextadv->advInt_diff * 32)) :
                                                                        ((clock_time() << 2) & bltAdv.delay_sSlot_mask);
    #else
                                u16 random = (clock_time() << 2) & bltAdv.delay_sSlot_mask;
    #endif

                                allocate_sSlot_adv = adv_max_left - random;
                            }
                        }
                    }


                    if ((ADV_task_allocate == 1) && (s32)(sSlot_idx_right - sSlot_idx_left) >= cur_pextadv->sSlotDuration_extadv) {
                        ADV_task_allocate = 2;
                    }


                    if (ADV_task_allocate == 2) {
                        pTsk_cur = &cur_pextadv->extadv_schTsk_fifo[jj];

                        cur_sSlot_adv = allocate_sSlot_adv;

    #if (PRICHN_SCAN_SMALL_INTV_WITH_ANOTHER_BIG_INTV_ADV_RESOLVE_EN && ANOTHER_BIG_INTV_EXTENDED_ADV)
                        bltAdScn.legadv_alloc = 1;
                        bltAdScn.legadv_sSlot = cur_sSlot_adv;
                            //bltAdScn.legadv_sSLot_durn = cur_pextadv->sSlotDuration_extadv;
    #endif


                        pTsk_cur->begin = cur_sSlot_adv;
                        pTsk_cur->end   = cur_sSlot_adv + (cur_pextadv->sSlotDuration_extadv - 1);

                        //insert ADV task to existed LinkList
                        pExtLkTsk_left->next                           = pTsk_cur;
                        pTsk_cur->next                                 = pExtLkTsk_right;
                        bltPri.csctvAbandonCnt[pTsk_cur->scheTask_oft] = 0;
                        pExtLkTsk_left                                 = pTsk_cur; //move forward pLeft

                        break;                                                     //exit while 1
                    } else {                                                       //traverse to next
                        if (pExtLkTsk_right == NULL) {
                            //return 0;  //meet the end , can not traverse, finish
                            goto extadv_loop_end;
                        } else {
                            pExtLkTsk_left  = pExtLkTsk_left->next;
                            pExtLkTsk_right = pExtLkTsk_right->next;
                        }
                    }

                    if (usr_irq_handler_cb) {
                        usr_irq_handler_cb();
                    }

                } //while(1)


            } // for(int i=0; i<EXT_ADV_FIFONUM; i++)
        }


extadv_loop_end:
        TNOP;
    }

    return 0;
}

_attribute_ram_code_ int blt_ll_build_auxadv_task(void)
{
    st_ext_adv_t *cur_pextadv;

    /* update sSlot mark if sSlot reset happens */
    //A build only run once (if the first 80ms not find task,next 80ms not run these lines code).
    //bltSche.build_index == 0 is to limit run only once in a build processing.
    if (bltSche.sSlot_idx_reset == 1 && bltSche.build_index == 0) {
        for (int i = 0; i < bltExtA.maxNum_advSets; i++) {
            /* In fact,here use TSKMSK_EXT_ADV_0 is more reasonable.But here is just for safety reasons to use TSKMSK_AUX_ADV_0.
             * For example, the variable cur_pextadv->aux_adv_pending is cleared or set incorrectly.
             */
            if (bltSche.task_mask & (TSKMSK_AUX_ADV_0 << i)) //attention: can not delete, the application layer may be configured with fewer NUMBER !!!
            {
                cur_pextadv = (st_ext_adv_t *)(global_pextadv + i);
                if (cur_pextadv->aux_adv_pending) {
                    cur_pextadv->sSlot_aux_mark -= bltSche.sSlot_idx_past;
                }
            }
        }
    }

    for (int i = 0; i < bltFutTask.number; i++) {
        future_task_e *pFutTask = (future_task_e *)&bltFutTask.task_tbl[i];

        /* must calculate in for loop, cause "sSlot_endIdx_maxPri" may changed in for loop */
        u32 ll_endTick = bltSche.sSlot_tick_start + bltSche.sSlot_endIdx_dft * SSLOT_TICK_NUM;
        /* current task start tick is beyond link_list, finish */
        if (tick1_exceed_tick2(pFutTask->tick_s, ll_endTick)) { //for big interval
            continue;                                           //attention: can not use break !!!
        }

        if (pFutTask->task_flg == TSKFLG_AUX_ADV) {
            int task_abandon = 0;
            u8  aux_idx      = pFutTask->task_oft - TSKOFT_AUX_ADV;
            cur_pextadv      = (st_ext_adv_t *)(global_pextadv + aux_idx);

            /* for ext_adv & ext_scan, future timing not exceed aux_offset max 2^13 * 300 uS = 8192*300uS = 2457600uS = 2.46 S*/
            //if( (u32)(pFutTask->tick_s - bltSche.sSlot_tick_next) > 4 * SYSTEM_TIMER_TICK_1S) //timing passed
            if (tick1_exceed_tick2(bltSche.sSlot_tick_next, pFutTask->tick_s)) { //timing passed
                task_abandon = 1;
                bltPri.csctvAbandonCnt[pFutTask->task_oft]++;
                my_dump_str_u32s(DBG_EXTADV_TIMING, "abandon aux task: time passed", bltSche.sSlot_tick_next, pFutTask->tick_s, pFutTask->task_oft, bltPri.csctvAbandonCnt[pFutTask->task_oft]);
            } else {
    //here need to pay attention to rounding. or the syncInfor is larger 19.5us than the real offset.
    #if (SYSTICK_NUM_PER_US == 16)
                s32 sSlot_start = ((pFutTask->tick_s - bltSche.sSlot_tick_start) * 2 + 313) / 625; //SSLOT_TICK_REVERSE
                s32 sSlot_end   = ((pFutTask->tick_e - bltSche.sSlot_tick_start) * 2 + 313) / 625;
    #elif (SYSTICK_NUM_PER_US == 24)
                s32 sSlot_start = ((pFutTask->tick_s - bltSche.sSlot_tick_start) * 4 + 938) / 1875; //SSLOT_TICK_REVERSE
                s32 sSlot_end   = ((pFutTask->tick_e - bltSche.sSlot_tick_start) * 4 + 938) / 1875;
    #else
        #error "add process for new SYSTICK_NUM_PER_US !!!"
    #endif


                if (sSlot_end < bltSche.sSlot_endIdx_dft) { //new task in correct range
                    sch_task_t *pTsk_cur = (sch_task_t *)&cur_pextadv->auxadv_schTsk_fifo;
                    pTsk_cur->begin      = sSlot_start;
                    pTsk_cur->end        = sSlot_end;
                    if (blt_ll_addTask2ExistLinklist(pTsk_cur, 1) == 1) {
                        task_abandon = 1;
                        my_dump_str_data(DBG_EXTADV_TIMING, "aux task abandon", 0, 0);
                    }
                } else { //new task across "sSlot_endIdx_dft"
                    my_dump_str_data(DBG_EXTADV_TIMING, "across end_idx", 0, 0);
                    //for those task across end_idx, find the task with highest priority, to guarantee that task not missed
                    if (bltPri.pri_cal[pFutTask->task_oft] > bltPri.priMax_value) {
                        bltPri.priMax_value = bltPri.pri_cal[pFutTask->task_oft];
                        //bltPri.priMax_index = pFutTask->task_oft;  //not used now
                        bltSche.sSlot_endIdx_maxPri = sSlot_start;
                    }
                    break;
                }
            }

            if (task_abandon) {
                cur_pextadv->aux_adv_pending = 0;
                blt_sche_removeTaskMask(TSKMSK_AUX_ADV_0 << aux_idx);

                bltFutTask.number--;
                if (i != bltFutTask.number) {
                    smemcpy4(&bltFutTask.task_tbl[i], &bltFutTask.task_tbl[i + 1], sizeof(future_task_e) * (bltFutTask.number - i));
                    i--; //add by qiuwei, note: i-- and i++ in "(int i=0; i<bltFutTask.number; i++)"
                }
            }
        }
    }

    return 0;
}

_attribute_ram_code_ int blt_auxadv_send(int slotTask_idx)
{
    bltExtA.extadv_sel = slotTask_idx;

    if (bltExtA.extadv_sel == 0) {
        //      DBG_C HN7_TOGGLE;
    } else if (bltExtA.extadv_sel == 1) {
        //      DBG_C HN8_TOGGLE;
    } else if (bltExtA.extadv_sel == 2) {
        //      DBG_C HN9_TOGGLE;
    } else if (bltExtA.extadv_sel == 3) {
        //      DBG_C HN10_TOGGLE;
    }

    blt_pextadv = (st_ext_adv_t *)(global_pextadv + bltExtA.extadv_sel);

    //switch to secondary ADV PHY
    #if (LL_FEATURE_ENABLE_ADVERTISING_CODING_SELECTION)
    u8 secchn_coded_phy_ind = blt_pextadv->sec_codedPhy_option;
    #else
    u8 secchn_coded_phy_ind = blt_pextadv->coding_ind;
    #endif
    rf_ble_switch_phy(blt_pextadv->sec_phy, secchn_coded_phy_ind);

    if (bltPHYs.cur_llPhy == BLE_PHY_CODED) {
        rf_trigger_codedPhy_accesscode();
    }

    //TX wait no need set, because only STX & TX2RX mode used in aux_adv
    rf_ble_set_rx_wait(RF_RX_WAIT_MIN_VALUE);                          //only involved in BTX/BRX/TX2RX
    rf_ble_set_rx_settle(RX_SETTLE_US);
    rf_ble_set_tx_settle(bltPHYs.tx_stl_adv);                          //attention: must set after PHY switch !!!
    rf_ble_csem_set_tx_rx_settle(0, bltPHYs.tx_stl_adv, RX_SETTLE_US); //attention: must set after PHY switch !!!

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
    rf_set_ble_crc_adv();
    rf_set_ble_access_code_adv();
    rf_set_ble_channel(blt_pextadv->aux_chn_idx);

    //////////
    blt_ll_set_tx_power_by_strategy(TX_POWER_STRATEGY_CUSTOMER_OR_DEFAULT, 0);


    STOP_RF_STATE_MACHINE; // stop SM
    HAL_CLEAR_RF_TX_RX_IRQ;

    int send_dataLen  = 0;
    u8  aux_chn_index = 0, aux_chn_backup;
    u8  tx_settle_us  = bltPHYs.tx_stl_adv;

    u32 tick_wait;
    u32 tx_begin_tick              = bltSche.sSlot_tick_irq + (bltAdv.advTxDly_us + blt_pextadv->aux_align_dly_us + 1) * SYSTEM_TIMER_TICK_1US; //plus 1us offset
    blt_pextadv->aux_adv_tx_off_us = bltAdv.advTxDly_us + blt_pextadv->aux_align_dly_us + TX_STL_ADV_REAL_COMMON;


    /* step 1: trigger RF mode  */
    if (blt_pextadv->cur_advMode)                                                //Connectable or Scannable
    {
        rf_ble_set_rx_timeout((bltPHYs.cur_llPhy == BLE_PHY_CODED) ? 600 : 250); //make sure PHY switch before this code


        //Switch dma rx buffer to ADV's dma rx buffer
        ble_rf_set_rx_dma((u8 *)glb_temp_rx_buff, 4);                 //change
        rf_set_rx_maxlen(34);                                         //conn_req 34 Byte; scan_req 12 Byte,so 34 byte is enough

        rf_start_fsm(FSM_TX2RX, (void *)&pkt_secondary, tx_begin_tick);
    } else                                                            //None Scannable None Connectable
    {
        rf_start_fsm(FSM_STX, (void *)&pkt_secondary, tx_begin_tick); //TODO, timing accurate control
    }

    if (blc_rf_pa_cb) {
        blc_rf_pa_cb(PA_TYPE_TX_ON);
    }

    /* step 2: set AUX_ADV_IND packet parameters by order  */
    blt_aux_adv_ind_fill_advA_targetA();

    smemcpy(&pkt_secondary, &blt_pextadv->aux_adv_1stPkt, blt_pextadv->aux_adv_1stPkt.ext_hdr_len - 1 + AUX_ADV_FORMAT_LEN);


    //TODO need to calculate accuracy timeout time
    u32 tx_timeout_us = blt_phy_getRfPacketTime_us(pkt_secondary.rf_len, blt_pextadv->sec_phy, secchn_coded_phy_ind) + TX_STL_ADV_REAL_COMMON + 100;
    u32 tick_target   = tx_begin_tick + tx_timeout_us * SYSTEM_TIMER_TICK_1US;

    /* step 3:  */
    if (blt_pextadv->cur_advMode) //Connectable or Scannable
    {
        if (blt_pextadv->cur_advMode == LL_EXTADV_MODE_CONN) {
            //if(blt_pextadv->aux_1st_pkt_dataLen) //Aux_Adv_Ind(Connected Undirected) no AuxPtr field!!!
            {
                smemcpy((pkt_secondary.data + blt_pextadv->aux_adv_1stPkt.ACAD_advData_offset), blt_pextadv->dat_extAdv, blt_pextadv->aux_1st_pkt_dataLen);
            }
        }

        u8           *prx = (u8 *)glb_temp_rx_buff;
        volatile u32 *ph  = (u32 *)(glb_temp_rx_buff + 4);
        ph[0]             = 0;

        /* must wait for TX finish, then change TX packet content, cause they are sharing one buffer */
    #if (DEBUG_RF_EXTADV_WHILE_EN)
        #if 1 //stuck in while, do not use timeout
        while (!HAL_GET_RF_TX_IRQ)
            ;
        #else //exit with timeout, but record the count
        while (!HAL_GET_RF_TX_IRQ) {
            if (tick1_exceed_tick2(clock_time(), tick_target)) {
                A_rf_while_debug.second_auxAdv_conn_tmCnt++;
                break;
            }
        }
        #endif
    #else
        while (!HAL_GET_RF_TX_IRQ && tick1_exceed_tick2(tick_target, clock_time())) {
            if (usr_irq_handler_cb) {
                usr_irq_handler_cb();
            }
        }
    #endif

        HAL_CLEAR_RF_TX_IRQ;
        u32 tx_end_tick = clock_time();

        if (blc_rf_pa_cb) {
            blc_rf_pa_cb(PA_TYPE_RX_ON);
        }
        /*here waiting for RX packet, at least 150uS available: 150uS + 8uS(1 byte preamble) + 32uS(4 byte AccessCode) + 32uS(DMA 4 byte)
          can prepare AUX_SCAN_RSP(first packet) for "Scannable only" event, cause ScanRsp packet may very long,
                                                                and 150uS is too quick when AUX_SCAN_REQ is coming */
        if (blt_pextadv->cur_advMode == LL_EXTADV_MODE_SCAN) {
            //pkt_secondary.type = LL_TYPE_AUX_SCAN_RSP;  //type always 7, no need set
            //pkt_secondary.chan_sel;  //no need set
            pkt_secondary.rxAddr   = 0; //LL_TYPE_AUX_CHAIN_IND/LL_TYPE_AUX_SCAN no "TargetA"
            pkt_secondary.adv_mode = LL_EXTADV_MODE_NON_CONN_NON_SCAN;

            /* txAddr & advA process for "AUX_SCAN_RSP"
             * special: no need re_set advA for AUX_SCAN_RSP, here advA is "M"
             * previous pkt_secondary is for AUX_ADV_IND Scannable, and advA is "M" in two kind of Scannable,
             * so here just keep same value */

            int auxScanRsp_extHdrLen = EXTHD_LEN_6_ADVA;
            int auxScanRsp_extHdrFlg = EXTHD_BIT_ADVA;

            if (blt_pextadv->with_aux_chain_ind) {
                gbl_auxPtr.chn_index  = blt_pextadv->aux_chn_idx;
                gbl_auxPtr.aux_offset = blt_pextadv->n_30us_chain_ind;
                gbl_auxPtr.aux_phy    = blt_pextadv->sec_phy - 1; // le_phy_type_t 1/2/3 corresponding 0/1/2 in packet
                smemcpy((pkt_secondary.data + auxScanRsp_extHdrLen), &gbl_auxPtr, EXTHD_LEN_3_AUX_PTR);
                auxScanRsp_extHdrLen += EXTHD_LEN_3_AUX_PTR;
                auxScanRsp_extHdrFlg |= EXTHD_BIT_AUX_PTR;
            }

            //          if(blt_pextadv->txPower_en_len){
            //              pkt_secondary.data[auxScanRsp_extHdrLen] = ble_txPowerLevel;
            //              auxScanRsp_extHdrLen += EXTHD_LEN_1_TX_POWER;
            //              auxScanRsp_extHdrFlg |= EXTHD_BIT_TX_POWER;
            //          }

            //AUX_ADV_RSP: ACAD Info process
            if (0) {
                //smemcpy((pkt_secondary.data + auxScanRsp_extHdrLen), &ACAD, ACAD_len);
                //auxScanRsp_extHdrLen += ACAD_len; //ACAD_len(varies length)
            }

            pkt_secondary.ext_hdr_len = auxScanRsp_extHdrLen + 1;
            pkt_secondary.ext_hdr_flg = auxScanRsp_extHdrFlg;
            pkt_secondary.rf_len      = 2 + auxScanRsp_extHdrLen + blt_pextadv->chain_ind_dataLen[0];

            //pkt_secondary.dma_len = pkt_secondary.rf_len + 2;
            pkt_secondary.dma_len = rf_tx_packet_dma_len(pkt_secondary.rf_len + 2);

            smemcpy((pkt_secondary.data + auxScanRsp_extHdrLen), blt_pextadv->dat_scanRsp, blt_pextadv->chain_ind_dataLen[0]);
            send_dataLen = blt_pextadv->chain_ind_dataLen[0];                                                      //mark first packet TX data length
        }

        while (!(*ph) && (u32)(clock_time() - tx_end_tick) < blt_pextadv->rx_1st_pdu_us * SYSTEM_TIMER_TICK_1US) { //wait packet from master
            if (usr_irq_handler_cb) {
                usr_irq_handler_cb();
            }
        }

        if (*ph) {
            rf_pkt_adv_rx_t *pAdvRx        = (rf_pkt_adv_rx_t *)(prx + RF_BLE_DMA_RFRX_LEN_HW_INFO);
            u16             *advA16        = (u16 *)pkt_secondary.data; //local device's Address, SiHui have confirmed that "data" is AdvA
            u16             *peerSearchA16 = (u16 *)pAdvRx->advA;       //advA in "AUX_SCAN_REQ" and "AUX_CONNECT_REQ"

            blt_pextadv->peerScanReq_addrType = pAdvRx->txAddr;
            smemcpy(blt_pextadv->peerScanReq_addr, pAdvRx->peerA, BLE_ADDR_LEN);

            while (!HAL_GET_RF_RX_IRQ && (u32)(clock_time() - tx_end_tick) < blt_pextadv->rx_finish_us * SYSTEM_TIMER_TICK_1US) {
                if (usr_irq_handler_cb) {
                    usr_irq_handler_cb();
                }
            }
            u32 rx_end_tick = clock_time();

    #if (LL_FEATURE_ENABLE_LE_CODED_PHY)
            /*
                * When CODED PHY is currently used, the peer device may use S2 or S8 when sending AUX_SCAN_REQ or AUX_CONNECT_REQ
                * after receiving the packet, the local device needs to first determine whether the received PHY is S2 or S8,
                * and then update the correct peer_oneByte_us and TIFS_offset_us.
                * They are used by the local device prepare for TX tick when replying to AUX_SCAN_RSP or AUX_CONNECT_RSP.
                */
            if (bltPHYs.cur_llPhy == BLE_PHY_CODED && ll_coded_phy_ind_detect_cb) {
                bltRxPkt.rx_irq_tick  = rx_end_tick;
                bltRxPkt.rx_timeStamp = hal_rf_get_rx_timestamp(); //RX time_stamp should read ASAP
                ll_coded_phy_ind_detect_cb(pAdvRx->rf_len);        //blt_coded_phy_detect_peer_code_phy_indication
            }
    #endif

            tx_begin_tick = blt_quick_tx_prepare(FSM_STX, (void *)&pkt_secondary, pAdvRx->rf_len); //when 48M, diff is 31us.

            tx_settle_us = bltPHYs.tx_stl_tifs;

            if (blc_rf_pa_cb) {
                blc_rf_pa_cb(PA_TYPE_TX_ON);
            }
            if (RF_BLE_PACKET_VALIDITY_CHECK(prx)) {
                if (MAC_MATCH16(peerSearchA16, advA16)) {
                    /*                                     AUX_SCAN_REQ  AUX_CONNECT_REQ
                    Connectable Undirected  AUX_ADV_IND :       NO              YES
                    Connectable   Directed  AUX_ADV_IND :       NO              YES_2
                    Scannable   Undirected  AUX_ADV_IND :       YES             NO
                    Scannable     Directed  AUX_ADV_IND :       YES_3           NO

                    YES_2 :     Initiators other than the correctly addressed initiator shall not respond.
                    YES_3 :     Scanners other than the correctly addressed scanner shall not respond.
                    */
                    do {
                        u8 is_connect_req = 0, filter_enable = 0;
                        /* step 1, quick check if scan_req or connect_req basic logic pass
                        *         skill:  Put the hardest conditions first */
                        if (pAdvRx->rf_len == 12 && pAdvRx->type == LL_TYPE_SCAN_REQ && blt_pextadv->scnReq_response) {           //scan_req
                            filter_enable = blt_pextadv->adv_filterPolicy & ALLOW_SCAN_WL;
                        } else if (pAdvRx->rf_len == 34 && pAdvRx->type == LL_TYPE_CONNECT_REQ && blt_pextadv->conReq_response) { //conn_req
                            filter_enable         = blt_pextadv->adv_filterPolicy & ALLOW_CONN_WL;
                            is_connect_req        = 1;                                                                //need to provide API to host and need to distinguish connHandle
                        } else {
                            my_dump_str_u8s(DBG_PRVC_EXTADV_EN, "extadv, not expected pkt, stop", pAdvRx->type, 0, 0, 0);
                            break;                                                                                                //stop
                        }

                        ll_resolv_list_t *pRL_match   = NULL;
                        u8                peer_is_rpa = IS_RESOLVABLE_PRIVATE_ADDR(pAdvRx->txAddr, pAdvRx->peerA);

    /* step 2, network privacy ignore IDA process */
    #if (NETWORK_PRIVACY_IGNORE_IDA_CHECK)
                        /* check if network privacy mode ignore IDA exist */
                        if (!peer_is_rpa) {
                            if (blt_pextadv->pRslvlst_extAdv) {
                                pRL_match = blt_pextadv->pRslvlst_extAdv;
                            } else {
                                pRL_match = blt_ll_searchResolvingListEntry(pAdvRx->txAddr, pAdvRx->peerA);
                            }

                            if (pRL_match && pRL_match->peerIrk_valid) {             //peer device has distributed its IRK
                                if (pRL_match->rlPrivMode == NETWORK_PRIVACY_MODE) { //not allowed
                                    /* LL/SEC/ADV/BV-15-C  LL/SEC/ADV/BV-16-C  LL/SEC/ADV/BV-17-C*/
                                    my_dump_str_data(DBG_PRVC_EXTADV_EN, "extadv, network privacy ignore IDA, stop", 0, 0);
                                    break; //stop
                                } else {   //DEVICE_PRIVACY_MODE, allowed
                                    /* LL/SEC/ADV/BV-18-C  LL/SEC/ADV/BV-19-C  LL/SEC/ADV/BV-20-C*/
                                    my_dump_str_data(DBG_PRVC_EXTADV_EN, "extadv, device privacy accept IDA", 0, 0);
                                }
                            }
                        }
    #else
                            //special process:
    #endif


                        blt_ll_addr_set_peer_address(0, pAdvRx->txAddr, pAdvRx->peerA);

                        /* step 3, resolve RPA if needed, check whiteList if filtering needed
                        * for peerA RPA
                        * 1. scan_req, resolve if filter needed; do not resolve if no filter
                        * 2. conn_req, if direct ADV,      must resolve, than can check if addressed to local device
                        *              if none direct ADV, resolve if filtering needed; do not resolve if no filter
                        * Consider accept list, change to:
                        * 1. if direct ADV, must resolve, than check if addressed to local device, never use filtering/accept list
                        * 2. if none direct ADV, resolve RPA depend on filtering, then check accept list
                        */
                        if (blt_pextadv->directed_adv || filter_enable) {
    #if (LL_FEATURE_ENABLE_PRIVACY)
                            if (peer_is_rpa) {
                                pRL_match = blt_ll_resolve_rpa(0, pAdvRx->peerA, blt_pextadv->pRslvlst_extAdv);
                                if (pRL_match) {
                                    blt_ll_storePeerDeviceRpa(pRL_match, pAdvRx->peerA);
                                    blt_ll_addr_set_peer_address(1, pRL_match->rlIdAddrType, pRL_match->rlIdAddr);
                                    my_dump_str_data(DBG_PRVC_EXTADV_EN, "peer RPA resolve OK", pRL_match->rlIdAddr, 6);
                                } else {
                                    my_dump_str_data(DBG_PRVC_EXTADV_EN, "peer RPA resolve ERR, stop", 0, 0);
                                    break;
                                }
                            }
    #endif
                            if (blt_pextadv->directed_adv) { //direct ADV, do not care about filter
                                                             /* for ADV_DIRECT_IND, check initA in connReq for direct ADV
                               * regardless of IDA or RPA. here we just add some extra check for directed ADV special.
                               * case 1. both IDA, advA and initA should be same. So we abandon different IDAs.
                               * case 2. both RPA, Spec
                               *                  The Link Layer should not set the InitA field to the same value
                                                  as the TargetA field in the received advertising PDU.
                               *                  attention: here should not equal to must !!!
                               * case 3. Advertiser IDA, Initiator RPA, not exist for a correct privacy process.
                               *         But it need BLE host and controller both be correct, for host it's very hard to process.
                               *         So here we allow this situation exist, do not abandon.
                               *         Evaluation for this special process :For BQB, host is upper tester of other vendor,
                               *         we can guarantee our controller be correct. For mass production SDK, here if we
                               *         advertiser commit this error, and peer device do not detect this error, we let this flow go.
                               * case 4. Advertiser RPA, Initiator IDA
                               *         If peer should use RPA
                               *             If "NETWORK_PRIVACY IGNORE_IDA_CHECK" enable, can be rejected by network privacy, accept by device privacy
                               *             If "NETWORK_PRIVACY IGNORE_IDA_CHECK" disable, special process: now code take as device privacy (SiHui)
                               *         If peer no need use RPA, accept
                               */
                                if (smemcmp(bltAddr.peer_pka_or_ida_addr, blt_pextadv->eAdvParaCmd_peerAddr, BLE_ADDR_LEN) ||
                                    bltAddr.peer_pka_or_ida_type != blt_pextadv->eAdvParaCmd_peerAdrType) {
                                    my_dump_str_data(DBG_PRVC_EXTADV_EN, "direct ADV initA do not address to local, stop", 0, 0);
                                    break;
                                }
                            } else { //none direct ADV but filter needed
                                if (!blt_ll_searchAddrInWhiteListTbl(bltAddr.peer_pka_or_ida_type, bltAddr.peer_pka_or_ida_addr)) {
                                    my_dump_str_data(DBG_PRVC_EXTADV_EN, "peer advA not in WL, stop", bltAddr.peer_pka_or_ida_addr, 6);
                                    break;
                                }
                            }
                        } else { //none direct ADV, no filter, pass without any check
                                 /* consider later : even no filter, maybe we need try to resolve RPA, because
                           * 1. enhanced connection complete event: Peer_Address should be IDA when RPA can be resolved
                           * 2. for AUX_CONNECT_RSP, targetA should not be same value with initA in AUX_CONNECT_REQ,
                           *    so here need find out the RL entry by resolving RPA
                           */
                        }


                        /* final step, respond to scan_req(send scan_rsp) or conn_req(connect) */
                        if (is_connect_req) {                            //AUX_CONNECT_REQ
                            my_dump_str_data(DBG_PRVC_EXTADV_EN, "extadv, accept aux_conn_req", 0, 0);

                            rf_set_tx_packet_address(&pkt_aux_conn_rsp); //get ready TX packet data
                            blc_rcvd_connReq_tick = rx_end_tick;


                            /* txAddr & advA process for "AUX_CONNECT_RSP" */
                            pkt_aux_conn_rsp.txAddr = blt_pextadv->cur_advA_type;
                            smemcpy(pkt_aux_conn_rsp.advA, blt_pextadv->cur_advA_addr, BLE_ADDR_LEN);

    /* rxAddr & targetA process for "AUX_CONNECT_RSP" */
    #if 0
                                /*  When an advertiser receives a connection request that contains a device
                                 *  Identity Address for the initiator's address field (InitA field), and if that device is
                                 *  in the Resolving List with a non-zero peer IRK for which the Host has specified
                                 *  device privacy mode, then the advertiser shall establish a connection. When
                                 *  responding with an AUX_CONNECT_RSP, the Link Layer should not set the
                                 *  TargetA field to the same value as the InitA field in the received PDU.
                                 *                               *
                                 *  two situation for initA in AUX_CONNECT_REQ:
                                 *  situation 1: peer advice use RPA
                                 *  situation 2: peer advice should use RPA, but it use IDA, and device network privacy allows it accepted
                                 *
                                 *  there are 2 kind of understanding:
                                 *  I.  for both situation 1 & situation 2, targetA in AUX_CONNECT_RSP should set different RPA
                                 *  II. for situation 2, targetA in AUX_CONNECT_RSP should be RPA; for situation 1, no limitation(maybe can use a copy)
                                 *  not sure what is correct understanding
                                 *
                                 *  code below now:
                                 *  LL/SEC/ADV/BV-23 ~ LL/SEC/ADV/BV-26 tested OK
                                 *  LL/CON/CEN/BV-84-C /LL/CON/PER/BV-88-C  */
                                if(peer_is_rpa && pRL_match && pRL_match->peerIrk_valid){ //Attention: here "pRL_match" may be not "pRslvlst_extAdv"
                                    pkt_aux_conn_rsp.rxAddr = BLE_ADDR_RANDOM;
                                    smemcpy(pkt_aux_conn_rsp.targetA, pRL_match->genrt_peerRpa, BLE_ADDR_LEN);
                                }
                                else{
                                    pkt_aux_conn_rsp.rxAddr = pAdvRx->txAddr;
                                    smemcpy(pkt_aux_conn_rsp.targetA, pAdvRx->peerA, BLE_ADDR_LEN);
                                }
    #else
                            /* copy from initA of AUX_CONNECT_REQ, may not accepted by peer device if they are very strict
                                 * LL/SEC/ADV/BV-23 ~ LL/SEC/ADV/BV-26
                                 * LL/CON/CEN/BV-84-C /LL/CON/PER/BV-88-C tested OK */
                            pkt_aux_conn_rsp.rxAddr = pAdvRx->txAddr;
                            smemcpy(pkt_aux_conn_rsp.targetA, pAdvRx->peerA, BLE_ADDR_LEN);
    #endif
                            // Timing executing codes below must ensure that AUX_CONNECT_RSP sending successfully
                            //1M PHY no problem, but when Coded PHY, we should consider the timing

                            //if(ll_adv_2_slave_cb)  //to save RamCode
                            {
                                if (TRUE == ll_adv_2_slave_cb((rf_packet_connect_t *)pAdvRx, TRUE)) { // blt_s_connect()
                                    //TODO need to calculate accuracy time  ---AUX_CONNECT_RSP
                                    /* pkt_aux_conn_rsp.rf_len = 14, never change */
                                    u32 aux_conn_rsp_pkt_us  = blt_phy_getRfPacketTime_us(14, blt_pextadv->sec_phy, secchn_coded_phy_ind);
                                    u32 tick_conn_rsp_target = rx_end_tick + (aux_conn_rsp_pkt_us + BLE_T_IFS + 10) * SYSTEM_TIMER_TICK_1US;
                                    while (!HAL_GET_RF_TX_IRQ && tick1_exceed_tick2(tick_conn_rsp_target, clock_time())) { //wait for TX finish
                                        if (usr_irq_handler_cb) {
                                            usr_irq_handler_cb();
                                        }
                                    }

    #if (EXT_ADV_EN_MORE_STRATEGY)
                                    /*
                                        * Extended adv only use strategy_3, Refer to the description of LEG_ADV_EN_STRATEGY_3
                                        *
                                        * Core Spec:"LE Advertising Set Terminated event" shall be generated every time
                                        * connectable advertising in an advertising set results in a connection being created.
                                        *
                                        * The Controller shall only start an advertising event when the corresponding advertising set is enabled.
                                        * The Controller shall continue advertising until all advertising sets have been disabled.
                                        * This can happen when the Host issues an HCI_LE_Set_Extended_Advertising_Enable command with the
                                        * Enable parameter set to 0x00 (Advertising is disabled), a connection is created, the duration specified
                                        * in the Duration[i] parameter expires, or the number of extended advertising events transmitted for the set
                                        * exceeds the Max_Extended_Advertising_Events[i] parameter.
                                        */
                                    //if(blmsParam.extadv_en_strategy == EXT_ADV_EN_STRATEGY_3 ){}
                                    blt_sche_removeTaskMask(TSKMSK_EXT_ADV_0 << blt_pextadv->extadv_index);
                                    blt_sche_disableTask(TSKMSK_EXT_ADV_0 << blt_pextadv->extadv_index);
                                    blt_sche_removeTaskMask(TSKMSK_AUX_ADV_0 << blt_pextadv->extadv_index);
                                    blt_pextadv->extadv_en = 0;
                                    blmsParam.ext_adv_en &= ~BIT(blt_pextadv->extadv_index);
    #endif
    #if (SMP_LOCAL_IRK_MATCH_CONTROLLER_NEW_PRIVACY)
                                    /* for SMP: SMP_LOCAL_IRK_MATCH_CONTROLLER_NEW_PRIVACY, save idenAdr_type/idenAdr_addr */
                                    blt_ll_record_identity_address(blt_pextadv->extadv_mac_type, blt_pextadv->extadv_mac_addr);
    #endif
                                }
                            }
                        } else { //AUX_SCAN_REQ
                            //DBG_C HN11_TOGGLE;
                            my_dump_str_data(DBG_PRVC_EXTADV_EN, "extadv, accept aux_scan_req", 0, 0);

                            if (blt_pextadv->scanReq_notify_en) {
                                blt_pextadv->peerScanReq_revFlag = 1; ///need to send event in mainloop
                            }
                            //TODO need to calculate accuracy time  ---AUX_SCAN_RSP
                            u32 aux_scan_rsp_pkt_us  = blt_phy_getRfPacketTime_us(pkt_secondary.rf_len, blt_pextadv->sec_phy, secchn_coded_phy_ind);
                            u32 tick_scan_rsp_target = rx_end_tick + (aux_scan_rsp_pkt_us + BLE_T_IFS + 10) * SYSTEM_TIMER_TICK_1US;
                            while (!HAL_GET_RF_TX_IRQ && tick1_exceed_tick2(tick_scan_rsp_target, clock_time())) { //wait for TX finish
                                if (usr_irq_handler_cb) {
                                    usr_irq_handler_cb();
                                }
                            }

                            HAL_CLEAR_RF_TX_IRQ;


                            //sending all scan_rsp data by AUX_CHAIN_IND
                            aux_chn_backup = blt_pextadv->aux_chn_idx;


                            for (int i = 1; i < blt_pextadv->chain_ind_num; i++) { //attention: start from "1"

                                int headr_len = 0;
                                int headr_flg = 0;

                                if (i == (blt_pextadv->chain_ind_num - 1)) { //last packet

                                } else {
                                    aux_chn_index = BLT_GENERATE_AUX_CHN;

                                    gbl_auxPtr.chn_index = aux_chn_index;
                                    //gbl_auxPtr.aux_offset = blt_pextadv->n_30us_chain_ind;  //same value as last, no set to save SRAM
                                    //gbl_auxPtr.aux_phy = blt_pextadv->sec_phy - 1;          //same value as last, no set to save SRAM
                                    smemcpy((pkt_secondary.data + headr_len), &gbl_auxPtr, EXTHD_LEN_3_AUX_PTR);
                                    headr_len += EXTHD_LEN_3_AUX_PTR;
                                    headr_flg |= EXTHD_BIT_AUX_PTR;
                                }

                                if (blt_pextadv->txPower_en_len) {
                                    pkt_secondary.data[headr_len] = ble_txPowerLevel;
                                    headr_len += EXTHD_LEN_1_TX_POWER;
                                    headr_flg |= EXTHD_BIT_TX_POWER;
                                }

                                smemcpy((pkt_secondary.data + headr_len), blt_pextadv->dat_scanRsp + send_dataLen, blt_pextadv->chain_ind_dataLen[i]);
                                send_dataLen += blt_pextadv->chain_ind_dataLen[i];

                                pkt_secondary.ext_hdr_len = headr_len + 1;
                                pkt_secondary.ext_hdr_flg = headr_flg;
                                pkt_secondary.rf_len      = 2 + headr_len + blt_pextadv->chain_ind_dataLen[i];
                                //pkt_secondary.dma_len = pkt_secondary.rf_len + 2;
                                pkt_secondary.dma_len = rf_tx_packet_dma_len(pkt_secondary.rf_len + 2);

                                tx_begin_tick += blt_pextadv->rfLen_255_pkt_us * SYSTEM_TIMER_TICK_1US;
                                tick_wait = tx_begin_tick - FSM_TRIGGER_EARLY_WAIT_TICK;

    #if (ADV_DURATION_STALL_EN)
    #else
                                while ((u32)(clock_time() - tick_wait) > BIT(30)) {
                                    if (usr_irq_handler_cb) {
                                        usr_irq_handler_cb();
                                    }
                                }
    #endif

                                rf_set_ble_channel(aux_chn_backup);
                                aux_chn_backup = aux_chn_index;

                                rf_start_fsm(FSM_STX, (void *)&pkt_secondary, tx_begin_tick);
                                HAL_CLEAR_RF_TX_RX_IRQ;

    #if (ADV_DURATION_STALL_EN)
                                cpu_stall_WakeUp_By_RF_SystemTick(IRQ_ZB_RT, FLD_RF_IRQ_TX, 0);
    #else
                                u32 auxScanRspChain_us         = blt_phy_getRfPacketTime_us(pkt_secondary.rf_len, blt_pextadv->sec_phy, secchn_coded_phy_ind);
                                u32 auxScanRspChain_targetTick = tx_begin_tick + (tx_settle_us + auxScanRspChain_us + 10) * SYSTEM_TIMER_TICK_1US;
                                while (!HAL_GET_RF_TX_IRQ && tick1_exceed_tick2(auxScanRspChain_targetTick, clock_time())) { //wait for TX finish
                                    if (usr_irq_handler_cb) {
                                        usr_irq_handler_cb();
                                    }
                                }
    #endif
                            }
                        }
                    } while (0);
                }
            }
        }
    } else //None Scannable None Connectable
    {
        //this is for not send aux_adv_ind. because periodic adv has been enable, aux_adv_ind need include valid syncInfor.
        //if syncInfor is invalid, EBQ maybe judge fail.but aux_adv_ind may not be sent.
        if (blt_pextadv->syncinfo_changed == 2) { //here syncinfo_changed must be 2.

            //Rebuild sch task table ASAP.
            blt_sche_addUpdate(SLOT_UPDT_SLOTTBL_RESCHED);
            blt_pextadv->syncinfo_changed = 0;

    #if (ONLY_FOR_EBQ_TEST_LATER_REMOVE)
            if (bigExtAuxPda_conflictCtrl.syncInfor_changeCtrl == SYNCINFOR_VAILD_PENDING) {
                bigExtAuxPda_conflictCtrl.syncInfor_changeCtrl = SYNCINFOR_VAILD_COMPLETE;
            }
    #endif
        } else {
            if (blt_pextadv->with_aux_chain_ind) {
                /* only one situation: non_connectable_non_scannable*/
                gbl_auxPtr.chn_index = aux_chn_index = BLT_GENERATE_AUX_CHN;
                gbl_auxPtr.aux_offset                = blt_pextadv->n_30us_chain_ind;
                gbl_auxPtr.aux_phy                   = blt_pextadv->sec_phy - 1; // le_phy_type_t 1/2/3 corresponding 0/1/2 in packet
                smemcpy((pkt_secondary.data + blt_pextadv->aux_adv_1stPkt.auxPtr_offset), &gbl_auxPtr, EXTHD_LEN_3_AUX_PTR);
            }

            //If periodic_adv is enabled, update syncInfo field before RF send it.
            //Sync_Packet_Offset + Offset_Units + Offset_Adjust + Event_Counter need update.
            if (blt_pextadv->syncinfo_used) {                                                             //If exist SyncInfo field(e.g.: Periodic ADV concerned) // & SYNC_INFO_VALID

                if (ll_prd_adv_irq_task_cb) {                                                             //blt_prd_adv_interrupt_task
                    ll_prd_adv_irq_task_cb(FLAG_SCHEDULE_AUX_SYNCINFO_UPDATE | bltExtA.extadv_sel, NULL); //blt_ll_aux_syncinfo_update
                }
                smemcpy((pkt_secondary.data + blt_pextadv->aux_adv_1stPkt.syncInfo_offset), &blt_pextadv->auxSyncInfo, EXTHD_LEN_18_SYNC_INFO);

                smemcpy(&pkt_secondary, &blt_pextadv->aux_adv_1stPkt, AUX_ADV_FORMAT_LEN); //add_qw

    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
                if (blt_pextadv->acad_used & PERD_ACAD_PAwR_ENA) {                         //If exist ACAD field(e.g.: PAwR)
                    // may optimize the array in rodata section.
                    //u8 acadPawr[10] = { 9, DT_PA_RESPONSE_TIMING_INFORMATION, 0, 0, 0, 0, 0, 0, 0, 0};
                    u8 acadPawr[10];
                    acadPawr[0] = 9;
                    acadPawr[1] = DT_PA_RESPONSE_TIMING_INFORMATION;
                    smemcpy(acadPawr + 2, (u8 *)&blt_pextadv->pawr_timing_info, sizeof(pawr_acad_t)); //pawr_timing
                    smemcpy((pkt_secondary.data + blt_pextadv->txPower_offset), acadPawr, sizeof(acadPawr));
                }
    #endif
            }


            smemcpy((pkt_secondary.data + blt_pextadv->aux_adv_1stPkt.ACAD_advData_offset), blt_pextadv->dat_extAdv, blt_pextadv->aux_1st_pkt_dataLen);


    #if (ADV_DURATION_STALL_EN)
            cpu_stall_WakeUp_By_RF_SystemTick(IRQ_ZB_RT, FLD_RF_IRQ_TX, 0);
    #else
                ///////////////////////////////////////////////////////////////////////////////////////////
                //TODO need to calculate accuracy timeout time
        #if (DEBUG_RF_EXTADV_WHILE_EN)
            #if 1 //stuck in while, do not use timeout
            while (!HAL_GET_RF_TX_IRQ)
                ;
            #else //exit with timeout, but record the count
            while (!HAL_GET_RF_TX_IRQ) {
                if (tick1_exceed_tick2(clock_time(), tick_target)) {
                    A_rf_while_debug.second_auxAdv_nonConn_tmCnt++;
                    break;
                }
            }
            #endif
        #else
            while (!HAL_GET_RF_TX_IRQ && tick1_exceed_tick2(tick_target, clock_time())) { //wait for TX finish
                if (usr_irq_handler_cb) {
                    usr_irq_handler_cb();
                }
            }
        #endif
    #endif


            if (blt_pextadv->with_aux_chain_ind) {
                aux_chn_backup = aux_chn_index;
                send_dataLen   = blt_pextadv->aux_1st_pkt_dataLen;


                //pkt_secondary.type = LL_TYPE_AUX_CHAIN_IND;  //type always 7, no need set
                //pkt_secondary.chan_sel;  //no need set
                pkt_secondary.txAddr   = 0;
                pkt_secondary.rxAddr   = 0; //LL_TYPE_AUX_CHAIN_IND/LL_TYPE_AUX_SCAN no "TargetA"
                pkt_secondary.adv_mode = LL_EXTADV_MODE_NON_CONN_NON_SCAN;

                u16 adi_info = blt_pextadv->adv_sid << 12 | blt_pextadv->adv_did;
                smemcpy((pkt_secondary.data + 0), &adi_info, EXTHD_LEN_2_ADI);


                for (int i = 0; i < blt_pextadv->chain_ind_num; i++) {
                    int headr_len = EXTHD_LEN_2_ADI;
                    int headr_flg = EXTHD_BIT_ADI;

                    if (i == (blt_pextadv->chain_ind_num - 1)) { //last packet

                    } else {
                        aux_chn_index = BLT_GENERATE_AUX_CHN;

                        gbl_auxPtr.chn_index = aux_chn_index;
                        //gbl_auxPtr.aux_offset = blt_pextadv->n_30us_chain_ind;  //same value as last, no set to save SRAM
                        //gbl_auxPtr.aux_phy = blt_pextadv->sec_phy - 1;          //same value as last, no set to save SRAM
                        smemcpy((pkt_secondary.data + headr_len), &gbl_auxPtr, EXTHD_LEN_3_AUX_PTR);

                        headr_len += EXTHD_LEN_3_AUX_PTR;
                        headr_flg |= EXTHD_BIT_AUX_PTR;
                    }

                    if (blt_pextadv->txPower_en_len) {
                        pkt_secondary.data[headr_len] = ble_txPowerLevel;
                        headr_len += EXTHD_LEN_1_TX_POWER;
                        headr_flg |= EXTHD_BIT_TX_POWER;
                    }

                    smemcpy((pkt_secondary.data + headr_len), blt_pextadv->dat_extAdv + send_dataLen, blt_pextadv->chain_ind_dataLen[i]);
                    send_dataLen += blt_pextadv->chain_ind_dataLen[i];

                    pkt_secondary.ext_hdr_len = headr_len + 1;
                    pkt_secondary.ext_hdr_flg = headr_flg;
                    pkt_secondary.rf_len      = 2 + headr_len + blt_pextadv->chain_ind_dataLen[i];
                    //pkt_secondary.dma_len = pkt_secondary.rf_len + 2;
                    pkt_secondary.dma_len = rf_tx_packet_dma_len(pkt_secondary.rf_len + 2);

                    tx_begin_tick += blt_pextadv->rfLen_255_pkt_us * SYSTEM_TIMER_TICK_1US;
                    tick_wait = tx_begin_tick - FSM_TRIGGER_EARLY_WAIT_TICK;

    #if (ADV_DURATION_STALL_EN)
    #else
                    while ((u32)(clock_time() - tick_wait) > BIT(30)) {
                        if (usr_irq_handler_cb) {
                            usr_irq_handler_cb();
                        }
                    }
    #endif

                    rf_set_ble_channel(aux_chn_backup);
                    aux_chn_backup = aux_chn_index;

                    rf_start_fsm(FSM_STX, (void *)&pkt_secondary, tx_begin_tick);
                    HAL_CLEAR_RF_TX_RX_IRQ;

    #if (ADV_DURATION_STALL_EN)
                    cpu_stall_WakeUp_By_RF_SystemTick(IRQ_ZB_RT, FLD_RF_IRQ_TX, 0);
    #else
                    u32 auxScanRspChain_us         = blt_phy_getRfPacketTime_us(pkt_secondary.rf_len, blt_pextadv->sec_phy, secchn_coded_phy_ind);
                    u32 auxScanRspChain_targetTick = tx_begin_tick + (tx_settle_us + auxScanRspChain_us + 10) * SYSTEM_TIMER_TICK_1US;
                    while (!HAL_GET_RF_TX_IRQ && tick1_exceed_tick2(auxScanRspChain_targetTick, clock_time())) { //wait for TX finish
                        if (usr_irq_handler_cb) {
                            usr_irq_handler_cb();
                        }
                    }
    #endif
                }
            }
        }
    }

    STOP_RF_STATE_MACHINE; //important: ensure that FSM stopped

    if (blc_rf_pa_cb) {
        blc_rf_pa_cb(PA_TYPE_OFF);
    }

    /* remove AUX_ADV task */
    blt_pextadv->aux_adv_pending = 0; //must clear
    blt_sche_removeTaskMask(TSKMSK_AUX_ADV_0 << bltExtA.extadv_sel);

    if (FALSE == blt_remove_future_task(TSKOFT_AUX_ADV + bltExtA.extadv_sel)) {
        my_dump_str_u32s(DBG_EXTADV_TIMING, "future task err", bltFutTask.number, bltExtA.extadv_sel, 0, 0);
    }
    //  if(bltFutTask.number){ //Multi extAdv set, here maybe an err
    //      BLMS_ERR_DEBUG(DBG_EXTADV_TIMING, 0xEEE10000 | bltFutTask.number);
    //  }

    #if (ONLY_FOR_EBQ_TEST_LATER_REMOVE)
    if (bigExtAuxPda_conflictCtrl.bigTask_timingStart && bigExtAuxPda_conflictCtrl.acadInfor_changeCtrl == ACAD_VALID_COMPLETE) {
        bigExtAuxPda_conflictCtrl.auxAdv_sendNum++;
    }
    #endif

    blt_ll_calculate_sSlot_next(clock_time() + SLOT_PROCESS_MAX_TICK);

    blms_state = BLMS_STATE_EXTADV_E;

    /* clear status as late as possible, cause if clear too early, some status did not come, e.g. STX cmd done*/
    CLEAR_ALL_RFIRQ_STATUS;

    if (bltExtA.extadv_sel == 0) {
        //      DBG_C HN7_TOGGLE;
    } else if (bltExtA.extadv_sel == 1) {
        //      DBG_C HN8_TOGGLE;
    } else if (bltExtA.extadv_sel == 2) {
        //      DBG_C HN9_TOGGLE;
    } else if (bltExtA.extadv_sel == 3) {
        //      DBG_C HN10_TOGGLE;
    }

    return 0;
}

_attribute_ram_code_
    u8
    blt_extadv_dur_maxEvt_proc(void)
{
    u32 extAdv_durTick = blt_pextadv->adv_duration_tick;
    u8  extAdv_maxEvt  = blt_pextadv->max_ext_adv_evt;

    if (extAdv_durTick || extAdv_maxEvt) {
        static u8 extAdv_stop_flag = 0;
        if (extAdv_durTick && ((unsigned int)(clock_time() - blt_pextadv->extAdv_begin_tick) > extAdv_durTick)) {
            blt_pextadv->adv_duration_tick       = 0;
            blt_pextadv->advSet_terminate_status = HCI_ERR_ADVERTISING_TIMEOUT; //duration is 0x3c;
            extAdv_stop_flag                     = 0x01;
        }
        //Max_Extended_Advertising_Events reached
        if (extAdv_maxEvt && blt_pextadv->run_ext_adv_evt >= extAdv_maxEvt) {
            blt_pextadv->max_ext_adv_evt         = 0;
            blt_pextadv->advSet_terminate_status = HCI_ERR_LIMIT_REACHED; //max adv event is 0x43
            extAdv_stop_flag                     = 0x01;
            //if connection created, and event number reached at the same time, connection created takes higher priority
            //if(blt_state != BLS_LINK_STATE_CONN){ //process connection state in other place. connHandle
            //advSet_terminate_status = HCI_ERR_LIMIT_REACHED; //max adv event is 0x43;
            //}
        }
        ////////////////
        if (extAdv_stop_flag) {
            extAdv_stop_flag = 0;
            blt_extadv_post();

            blt_pextadv->extadv_en = 0;
            blmsParam.ext_adv_en &= ~BIT(blt_pextadv->extadv_index);
            blt_pextadv->aux_adv_pending             = 0; //pending need to clear, or the new aux adv will not run.
            blt_pextadv->extAdv_terminateEvt_pending = 0x01;

            blt_sche_disableTask(TSKMSK_EXT_ADV_0 << bltExtA.extadv_sel);
            blt_sche_removeTaskMask(TSKMSK_EXT_ADV_0 << bltExtA.extadv_sel);
            blt_sche_removeTaskMask(TSKMSK_AUX_ADV_0 << bltExtA.extadv_sel);
            blt_remove_future_task(TSKOFT_AUX_ADV + bltExtA.extadv_sel);

            blt_sche_addUpdate(SLOT_UPDT_EXT_ADV_DISABLE);

            return 1;
        }
    }

    return 0;
}

_attribute_ram_code_ int blt_extadv_send(void)
{
    /* 1. IRQ timing is running, but main_loop ADV disable command take effect
     * 2. when prd_adv take effect, data and timing may change, old allocated task not execute */
    if (!blt_pextadv->extadv_en || blt_pextadv->extadv_change_flag) {
        blt_extadv_post();
        return 0;
    }

    #if !LL_EXT_ADV_DURATION_OPTIMIZE_EN
    if (blt_extadv_dur_maxEvt_proc()) { //return 1: duration or maxEvt has expired.
        return 0;                       //
    }
    #endif


    //  DBG_C HN5_HIGH;

    if (bltExtA.extadv_sel == 0) {
        //      DBG_C HN7_TOGGLE;
    } else if (bltExtA.extadv_sel == 1) {
        //      DBG_C HN8_TOGGLE;
    } else if (bltExtA.extadv_sel == 2) {
        //      DBG_C HN9_TOGGLE;
        my_dump_str_data(0, "ExtAdv Channel", &blc_extadv_channel[bltAdv.advChn_idx], 1);

    } else if (bltExtA.extadv_sel == 3) {
        //      DBG_C HN10_TOGGLE;
    }

    #if (LL_FEATURE_ENABLE_ADVERTISING_CODING_SELECTION)
    rf_ble_switch_phy(blt_pextadv->pri_phy, blt_pextadv->pri_codedPhy_option);
    #else
    rf_ble_switch_phy(blt_pextadv->pri_phy, blt_pextadv->coding_ind);
    #endif

    if (bltPHYs.cur_llPhy == BLE_PHY_CODED) {
        rf_trigger_codedPhy_accesscode();
    }

    //TX wait no need set, because only STX & TX2RX mode used in ext_adv
    rf_ble_set_rx_wait(RF_RX_WAIT_MIN_VALUE); //only involved in BTX/BRX/TX2RX
    /* here RX settle value is only used in legacy ADV tx2rx(connectable or scannable) mode, so set to "RXSET_OPTM_ANTI_INTRF" first is OK */
    rf_ble_set_rx_settle(RXSET_OPTM_ANTI_INTRF);                                //RX settle value for optimize anti-interference
    rf_ble_set_tx_settle(bltPHYs.tx_stl_adv);                                   //attention: must set after PHY switch !!!
    rf_ble_csem_set_tx_rx_settle(0, bltPHYs.tx_stl_adv, RXSET_OPTM_ANTI_INTRF); //attention: must set after PHY switch !!!

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
    rf_set_ble_crc_adv();
    rf_set_ble_access_code_adv();
    #if (BLE_LLMIC_CONCURRENT_EN)
    rf_set_ble_channel(blc_extadv_channel[blt_pextadv->llmic_advIdx]);
    #else
    rf_set_ble_channel(blc_extadv_channel[bltAdv.advChn_idx]);
    #endif

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


    if (blt_pextadv->legacy_adv) {
        blt_send_legacy_adv();
    } else {
        blt_send_extend_adv();
    }

    #if (BLE_LLMIC_CONCURRENT_EN)

    //todo optimize. 1. judge jump count, then llmic_advIdx+jump count;
    //               2. check whether channel is set mask.
    blt_pextadv->llmic_advIdx++; //The variable llmic_advIdx has already been initialized in blc_ll_setExtAdvParam().

    blt_pextadv->llmic_advIdx %= 3;

    blt_extadv_post();
    #else
    bltAdv.advChn_cnt++;

    if ((bltAdv.advChn_cnt >= blt_pextadv->adv_chn_num)) {
        //      bltAdv.advChn_idx = blt_pextadv->adv_chnIdx_1st; //before ext_adv_start, it'll be initialized.
    } else {
        bltAdv.advChn_idx++;
        u8 mask = 1 << (blc_extadv_channel[bltAdv.advChn_idx] - 37);
        if ((mask & blt_pextadv->adv_chn_mask) == 0) {
            bltAdv.advChn_idx++;
        }
    }

    if (bltAdv.adv_scanReq_connReq || (bltAdv.advChn_cnt >= blt_pextadv->adv_chn_num)) {
        blt_extadv_post();
    } else {
        if (blt_pextadv->legacy_adv) {
            systimer_set_irq_capture(clock_time() + 20 * SYSTEM_TIMER_TICK_1US);
        } else { //extended ADV
            if (blt_pextadv->with_aux_adv_ind) {
                systimer_set_irq_capture(bltSche.system_irq_tick + blt_pextadv->pri_single_chn_us * SYSTEM_TIMER_TICK_1US);
            } else {
                systimer_set_irq_capture(clock_time() + 20 * SYSTEM_TIMER_TICK_1US);
            }
        }

        systick_irq_trigger = SYS_IRQ_TRIG_EXTADV_SEND;
    }
    #endif //#if(BLE_LLMIC_CONCURRENT_EN)


    /* clear status as late as possible, cause if clear too early, some status did not come, e.g. STX cmd done*/
    CLEAR_ALL_RFIRQ_STATUS;


    /* SiHui found problem on B91. same situation for other RISV MCU with PLIC module.
     * but process method maybe different for new MCU, so move this function to HAL.
     * FSM IRQ status will send to PLIC module, clear reg_rf_irq_status can not drop RF IRQ,
     * so we disable RF mask to prevent RF IRQ status sending to PLIC */
    HAL_BLE_STACK_RF_IRQ_MASK_SET;
    #if 0 //can also solve above problem, but too complex, do not use
          //attention that it's tested on B91.
        irq_disable();
        plic_set_priority(IRQ_ZB_RT, 3);
        u32 claim = plic_interrupt_claim();
        my_dump_str_data(DBG_PRDADV_LOGIC, "debug 1", &claim, 4);
        plic_interrupt_complete(IRQ_ZB_RT);//complete interrupt
        plic_set_priority(IRQ_ZB_RT, 2);
        irq_enable();
    #endif

    /*
     * CSEM IP, when RF is in the TX state, must use the reset baseband to stop TX, and can not use other methods.
     * It is used here to ensure that RF ends safely on each channel.
     */
    HAL_CSEM_IP_RESET_BASEBAND;

    return 0;
}

_attribute_ram_code_ int blt_extadv_start(int slotTask_idx)
{
    DBG_CHN1_HIGH;
    DBG_SIHUI_CHN1_HIGH;
    //  DBG_TIANXIANG_CHN2_HIGH;
    #if (SL01_eadv_ext_ind)
    log_task_begin_irq(SL_STACK_EXT_PRD_BASE_TIMING_EN, SL01_eadv_ext_ind);
    #endif

    bltExtA.extadv_sel = slotTask_idx;

    blt_pextadv = (st_ext_adv_t *)(global_pextadv + bltExtA.extadv_sel);

    blms_state = BLMS_STATE_EXTADV_S;

    bltAdv.adv_irq_tick = bltSche.sSlot_tick_irq;
    bltAdv.advChn_idx   = blt_pextadv->adv_chnIdx_1st;
    bltAdv.advChn_cnt   = 0;


    blt_pextadv->bSlot_mark_extadv = bltSche.bSlot_idx_irq_real;
    blt_pextadv->sSlot_diff_extadv = bltSche.sSlot_diff_irq;

    if (!blt_pextadv->run_ext_adv_evt) { //first ext adv, record the begin tick;
        blt_pextadv->extAdv_begin_tick = clock_time();
    }

    blt_extadv_send();

    return 0;
}

_attribute_ram_code_ int blt_extadv_post(void)
{
    #if (SL01_eadv_ext_ind)
    log_task_end_irq(SL_STACK_EXT_PRD_BASE_TIMING_EN, SL01_eadv_ext_ind);
    #endif

    blt_ll_calculate_sSlot_next(clock_time() + SLOT_PROCESS_MAX_TICK);

    blms_state = BLMS_STATE_EXTADV_E;


    blt_pextadv->run_ext_adv_evt++;

    DBG_CHN1_LOW;
    DBG_SIHUI_CHN1_LOW;
    //  DBG_TIANXIANG_CHN2_LOW;
    if (blc_rf_pa_cb) {
        blc_rf_pa_cb(PA_TYPE_OFF);
    } //here not need

    #if LL_EXT_ADV_DURATION_OPTIMIZE_EN
    ////////////////////////////////////////////////////////////////////
    if (blt_pextadv->adv_duration_tick || blt_pextadv->max_ext_adv_evt) {
        static u8 extAdv_stop_flag = 0;
        if (blt_pextadv->adv_duration_tick && ((unsigned int)(clock_time() - blt_pextadv->extAdv_begin_tick) > blt_pextadv->adv_duration_tick)) {
            blt_pextadv->adv_duration_tick       = 0;
            blt_pextadv->advSet_terminate_status = HCI_ERR_ADVERTISING_TIMEOUT; //duration is 0x3c;
            extAdv_stop_flag                     = 0x01;
        }
        //Max_Extended_Advertising_Events reached
        if (blt_pextadv->max_ext_adv_evt && blt_pextadv->run_ext_adv_evt >= blt_pextadv->max_ext_adv_evt) {
            blt_pextadv->max_ext_adv_evt         = 0;
            blt_pextadv->advSet_terminate_status = HCI_ERR_LIMIT_REACHED; //max adv event is 0x43
            extAdv_stop_flag                     = 0x01;
            //if connection created, and event number reached at the same time, connection created takes higher priority
            //if(blt_state != BLS_LINK_STATE_CONN){ //process connection state in other place. connHandle
            //advSet_terminate_status = HCI_ERR_LIMIT_REACHED; //max adv event is 0x43;
            //}
        }
        ////////////////
        if (extAdv_stop_flag) {
            extAdv_stop_flag = 0;
            //blt_extadv_post();//!!!!

            blt_pextadv->extadv_en = 0;
            blmsParam.ext_adv_en &= ~BIT(blt_pextadv->extadv_index);

            blt_sche_disableTask(TSKMSK_EXT_ADV_0 << bltExtA.extadv_sel);
            blt_sche_removeTaskMask(TSKMSK_EXT_ADV_0 << bltExtA.extadv_sel);
            blt_sche_removeTaskMask(TSKMSK_AUX_ADV_0 << bltExtA.extadv_sel);
            blt_remove_future_task(TSKOFT_AUX_ADV + bltExtA.extadv_sel);
            blt_pextadv->aux_adv_pending = 0; //pending need to clear, or the new aux adv will not run.

            blt_sche_addUpdate(SLOT_UPDT_EXT_ADV_DISABLE);

            blt_pextadv->extAdv_terminateEvt_pending = 0x01;

            return 0;
        }
    }
        ////////////////////////////////////////////////////////////////////////////
    #endif


    return 0;
}

_attribute_ram_code_ void blt_add_future_task(u8 llTask_flag, u8 llTask_offset, u32 tick_start, u32 tick_end)
{
    if (bltFutTask.number < FUTURE_TASK_MAX_NUM) {
        future_task_e *pFutTask = &bltFutTask.task_tbl[bltFutTask.number];
        pFutTask->task_flg      = llTask_flag;
        pFutTask->task_oft      = llTask_offset;
        pFutTask->tick_s        = tick_start;
        pFutTask->tick_e        = tick_end;

        bltFutTask.number++;
    } else {
        my_dump_str_data(DBG_EXTADV_TIMING, "add fut task err", &bltFutTask.number, 1);
    }
}

_attribute_ram_code_ bool blt_remove_future_task(u8 llTask_offset)
{
    for (int i = 0; i < bltFutTask.number; i++) {
        if (bltFutTask.task_tbl[i].task_oft == llTask_offset) {
    #if 1 //optimize
            bltFutTask.number--;
            if (i != bltFutTask.number) {
                smemcpy4(&bltFutTask.task_tbl[i], &bltFutTask.task_tbl[i + 1], sizeof(future_task_e) * (bltFutTask.number - i));
            }
    #else
            if (i != bltFutTask.number - 1) {
                smemcpy4(&bltFutTask.task_tbl[i], &bltFutTask.task_tbl[i + 1], sizeof(future_task_e) * (bltFutTask.number - 1 - i));
            }
            bltFutTask.number--;
    #endif

            return TRUE;
        }
    }

    return FALSE;
}


    #if (SUB_INTERVAL_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #endif
    void
    blt_extAdv_scanReqRevEvt(void)
{
    st_ext_adv_t *cur_pextadv = NULL;

    if (!(hci_le_eventMask & HCI_LE_EVT_MASK_SCAN_REQUEST_RECEIVED)) {
        return;
    }

    for (int i = 0; i < bltExtA.maxNum_advSets; i++) {
        cur_pextadv = (st_ext_adv_t *)(global_pextadv + i);

        if (!cur_pextadv->peerScanReq_revFlag) {
            continue;
        }


        u8                       result[10]; //10 bytes is enough
        hci_le_scanReqRcvdEvt_t *pEvt = (hci_le_scanReqRcvdEvt_t *)result;

        pEvt->subEventCode    = HCI_SUB_EVT_LE_SCAN_REQUEST_RECEIVED;
        pEvt->advHandle       = cur_pextadv->adv_handle;
        pEvt->scannerAddrType = cur_pextadv->peerScanReq_addrType;
        smemcpy(pEvt->scannerAddr, cur_pextadv->peerScanReq_addr, BLE_ADDR_LEN);

        blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_scanReqRcvdEvt_t));
        cur_pextadv->peerScanReq_revFlag = 0; //clear

        my_dump_str_data(0, "scan request received event", 0, 0);
    }
}

    #if (SUB_INTERVAL_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #endif
    void
    blt_extAdv_terminateEvt(u8 connHandle, u8 advHandle, u8 terminateEvtNum, u8 connState)
{
    st_ext_adv_t *cur_pextadv;

    if (connState > 1) { //0:advertise; 1:connection
        return;
    }

    if (!(hci_le_eventMask & HCI_LE_EVT_MASK_EXTENDED_ADVERTISING_SET_TERMINATED)) {
        return;
    }

    //TODO: can not send event in IRQ !!!  hci_le_advertising_set_terminated_evt
    u8                            result[8]; //6 byte is enough
    hci_le_advSetTerminatedEvt_t *pEvt = (hci_le_advSetTerminatedEvt_t *)result;

    if (connState == CONN_STATUS_DISCONNECT) {
        for (int i = 0; i < bltExtA.maxNum_advSets; i++) {
            cur_pextadv = (st_ext_adv_t *)(global_pextadv + i);

            if (cur_pextadv->extAdv_terminateEvt_pending) { //this can be sure adv state

                pEvt->subEventCode = HCI_SUB_EVT_LE_ADVERTISING_SET_TERMINATED;
                pEvt->advHandle    = cur_pextadv->adv_handle;

                pEvt->status            = cur_pextadv->advSet_terminate_status;
                pEvt->connHandle        = 0xFFFF; //invalid
                pEvt->num_compExtAdvEvt = cur_pextadv->run_ext_adv_evt;

                blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_advSetTerminatedEvt_t));
                my_dump_str_data(0, "adv terminateEvent", 0, 0);

                cur_pextadv->extAdv_terminateEvt_pending = 0; // last must clear.
            }
        }
    } else if (connState == CONN_STATUS_COMPLETE) {
        pEvt->subEventCode = HCI_SUB_EVT_LE_ADVERTISING_SET_TERMINATED;
        pEvt->advHandle    = advHandle;

        pEvt->status            = 0;          //0 indicate connection be created.
        pEvt->connHandle        = connHandle; //invalid
        pEvt->num_compExtAdvEvt = terminateEvtNum;

        blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_advSetTerminatedEvt_t));
        my_dump_str_data(0, "conn terminateEvent", 0, 0);
    }
}

void blt_extAdv_pendEvtProc_mainloop(void)
{
    blt_extAdv_terminateEvt(0, 0, 0, CONN_STATUS_DISCONNECT);


    blt_extAdv_scanReqRevEvt();
}


#endif //end of LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING
