/********************************************************************************************************
 * @file    acl_sniffer_mst.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    2020.06
 *
 * @par     Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd.
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

#include "acl_sniffer_mst.h"


#if (LL_RSSI_SNIFFER_MASTER_ENABLE)




_attribute_ble_data_retention_  _attribute_aligned_(4) volatile acl_sniffer_param_t aclSniffer_mst_param;
_attribute_ble_data_retention_  u8 acl_sniffer_mst_sync_info[sizeof(acl_sniffer_sync_param_t)];

_attribute_ble_data_retention_  _attribute_aligned_(4) volatile acl_sniffer_seek_param_t aclSniffer_mst_seek[TSKNUM_SNIFM_SEEK];
_attribute_ble_data_retention_  acl_sniffer_seek_param_t *pSnifferMstSeek = NULL;

_attribute_ble_data_retention_  u16 acl_mst_connParamUpdateRsp_latency_max = 2;

#if (!SNIFFER_USE_SOME_COMMON_APIS)
_attribute_ram_code_ int irq_acl_sniffer_mst_rx(void)
{
    u8 * raw_pkt = (u8 *) (blt_rxfifo.p_base + (blt_rxfifo.wptr++ & blt_rxfifo.mask) * blt_rxfifo.size);
    u8 * new_pkt = (u8 *) (blt_rxfifo.p_base + (blt_rxfifo.wptr & blt_rxfifo.mask) * blt_rxfifo.size);

    aclConn_param.acl_rx_dma_buff = (u32)new_pkt; //Update the next acl dma rx buffer
    ble_rf_set_rx_dma((u8*)aclConn_param.acl_rx_dma_buff, aclConn_param.acl_rx_dma_size);

    HAL_CLEAR_RF_RX_IRQ;

    u8 drop_rx_data = 0;
    u8 next_buffer = 0;
    static u8 rfLen_1st_rx;

    raw_pkt[1] = 0; //for peer device RSSI mark, BIT(7)~BIT(6), BIT(7): master RSSI, BIT(6): slave RSSI, 0: invalid RSSI
                    //for RF channel index, BIT(5)~BIT(0)
    raw_pkt[2] = 0; //for data mark, snifHandle
    raw_pkt[3] = 0; //for peer device RSSI value
    #if (DEBUG_SNIFFER_REPORT_INSTANT_EN)
        raw_pkt[4] = 0; //for sniffer instant value, only for debug
    #endif

    /* "rx header tick" none zero is under condition CRC correct, so here do not check CRC by
       "RF_BLE_PACKET_VALIDITY_CHECK" to save RamCode and running timing
       Or we can use "bltRxPkt.crc correct" */
    if(bltRxPkt.crc_correct)
    {

        if(blms_state  ==  BLMS_STATE_SNIFM_S){
            DBG_CHN2_TOGGLE;
        }

        #if ((MCU_CORE_TYPE == MCU_CORE_825x) && (FIX_HW_CRC24_EN) ) //sys clk: 32M CRC24 software check time -> T(us) = 0.44*len + 17
            /* Different process for different MCU: set RX DMA FIFO and RX threshold **********/
            //Kite need fix this problem, Eagle/Vulture hardware does not have this problem, no special treatment is required

            /////////////////////////// software CRC24 check //////////////////////////////////
            u16  crc24_payload_off = DMA_RFRX_OFFSET_CRC24(raw_pkt); //notice: crc24_payload_off maybe bigger then 255!!
            extern u32 blt_packet_crc24_opt();//remove warning
            u32  crc24_check_val = blt_packet_crc24_opt(raw_pkt+DMA_RFRX_OFFSET_HEADER, raw_pkt[DMA_RFRX_OFFSET_RFLEN]+2, blms_pconn->conn_crc_revert, Crc24Lookup);
            drop_rx_data = !CRC_MATCH8(((u8*)&crc24_check_val),(raw_pkt+crc24_payload_off));  //CRC ERR -> drop rx data

            if(!drop_rx_data){ //CRC24 check ok
                ///// update rx rcvd time stamp /////
                blms_pconn->conn_tick = clock_time ();
                blms_pconn->conn_receive_packet = 1;

                ///////////////////////////// RX overflow check ///////////////////////////////////
                if(((u8)(blt_rxfifo.wptr - blt_rxfifo.rptr)&63) >= blt_rxfifo.num){
                    drop_rx_data = 1;
                }
            }
            if(drop_rx_data == 1){
                rf_set_tx_rx_off();
                STOP_RF_STATE_MACHINE;
                systimer_set_irq_capture(clock_time() + 50 * SYSTEM_TIMER_TICK_1US);
                blmsParam.rf_fsm_busy = 0;
            }
            /**********************************************************************************/
        #else
            blms_pconn->conn_tick = clock_time ();
            blms_pconn->conn_receive_packet = 1;
        #endif

        if(((u8)(blt_rxfifo.wptr - blt_rxfifo.rptr) & 63)  >= blt_rxfifo.num){
            drop_rx_data = 1;
        }

        if(drop_rx_data){
            rf_set_tx_rx_off();
            STOP_RF_STATE_MACHINE;
            blt_sniffer_stop_rx_window(50);
        }

        if(!drop_rx_data)
        {
            if(bltRxPkt.rx_header_tick)
            {
                if(!aclSniffer_mst_param.sniffer_rx_num){
                    if(blms_state == BLMS_STATE_SNIFM_S){
                        #if (LL_ACL_CEN_EN)
                            if(!blm_pconn->tick_1st_rx){
                                if(blms_pconn->acl_sniffer_sync_creating){
                                    //need to continue monitor the peer-slave
                                    blm_pconn->tick_1st_rx = bltRxPkt.rx_header_tick;
                                    rfLen_1st_rx = raw_pkt[DMA_RFRX_OFFSET_RFLEN];

                                    DBG_CHN3_TOGGLE;
                                    DBG_CHN3_TOGGLE;
                                }
                                else{
                                    u32 diff;
                                    if(bltRxPkt.rx_header_tick > blm_pconn->expectTimeMark){
                                        diff = bltRxPkt.rx_header_tick - blm_pconn->expectTimeMark;
                                    }
                                    else{
                                        diff = blm_pconn->expectTimeMark - bltRxPkt.rx_header_tick;
                                    }
                                    //printf("us:%d,%d,%d\n", bltRxPkt.rx_header_tick>>4, blm_pconn->expectTimeMark>>4,diff>>4);

                                    /*
                                     *    PHYs            timing(uS)
                                     *   1M PHY   :    (rf_len + 10) * 8  // 10 = 1(BLE preamble) + 9(accesscode 4 + crc 3 + header 2)
                                     *   2M PHY   :    (rf_len + 11) * 4  // 11 = 2(BLE preamble) + 9(accesscode 4 + crc 3 + header 2)
                                     *  Coded PHY : S2:rf_len * 16 + 462
                                     *              S8:rf_len * 64 + 720
                                     */
                                    //remote:  |1st_timing|<---T_IFS--->|2st_timing|
                                    //local:   |rx_head_tick + empty_1st_timing + 50us_margin
                                    if(diff < (blt_phy_getRfPacketTime_us(0, bltPHYs.cur_llPhy, bltPHYs.cur_peer_CI) + 50) * SYSTEM_TIMER_TICK_1US){

                                        blm_pconn->tick_1st_rx = bltRxPkt.rx_header_tick;

                                        rfLen_1st_rx = raw_pkt[DMA_RFRX_OFFSET_RFLEN];

                                        DBG_CHN3_TOGGLE;
                                        DBG_CHN3_TOGGLE;
                                    }
                                }
                            }
                        #endif
                    }
                }
                else if(aclSniffer_mst_param.sniffer_rx_num == 1)
                {
                    if(blm_pconn->tick_1st_rx){
                        u32 diff = bltRxPkt.rx_header_tick - blm_pconn->tick_1st_rx;

                        //e.g.: For 1M: 10 Byte = 1B(preamble) + 4B(accesscode) + 2B(header) + 3B(CRC), 150 is T_IFS

                        //remote:  |1st_timing|<---T_IFS--->|2st_timing|
                        //local:   |rx_head_tick + 1st_timing + T_IFS
                        u32 diff_ideal = (blt_phy_getRfPacketTime_us(rfLen_1st_rx, bltPHYs.cur_llPhy, bltPHYs.cur_peer_CI) + BLE_T_IFS) * SYSTEM_TIMER_TICK_1US;

                        // T_IFS within 20us
                        if((diff > (diff_ideal - 20 * SYSTEM_TIMER_TICK_1US)) && (diff < (diff_ideal + 20 * SYSTEM_TIMER_TICK_1US))){
                            //monitor the peer-slave successful

                            if(blms_pconn->acl_sniffer_sync_creating){
                                blms_pconn->acl_sniffer_sync_creating = 0;
                            }

                            //record first peer-slave RSSI
                            raw_pkt[1] = BLT_ACL_SNIFFER_SLAVE_FLAG;//peer-slave RSSI
                            raw_pkt[1] |= blms_pconn->conn_chn;
                            raw_pkt[2] = blms_pconn->acl_conHandle;
                            raw_pkt[3] = raw_pkt[DMA_RFRX_OFFSET_RSSI(raw_pkt)];;
                            #if (DEBUG_SNIFFER_REPORT_INSTANT_EN)
                                raw_pkt[4] = blms_pconn->conn_inst;
                            #endif
                            aclSniffer_mst_param.sniffer_rssi_validFlag = 1;

                            next_buffer = 1;

                            DBG_CHN3_TOGGLE;
                            DBG_CHN3_TOGGLE;
                            DBG_CHN3_TOGGLE;
                            DBG_CHN3_TOGGLE;
                        }
                    }

                    blt_sniffer_stop_rx_window(50);
                }
                else{
                    blt_sniffer_stop_rx_window(50);
                }
            }
        }

        aclSniffer_mst_param.sniffer_rx_num ++;  //care CRC
    }

    if (!next_buffer)           //reuse buffer
    {
        blt_rxfifo.wptr--;
        aclConn_param.acl_rx_dma_buff = (u32)raw_pkt; //Reuse the last dma rx buffer
        ble_rf_set_rx_dma((u8*)aclConn_param.acl_rx_dma_buff, aclConn_param.acl_rx_dma_size);
    }


    /* for Kite/Vulture, this is must; for Eagle, no effect. So we keep code compatible*/
    raw_pkt[0] = 1;

    return BLE_SUCCESS;
}


_attribute_ram_code_ int blt_sniffer_mst_start(int conn_idx)
{
    blms_start_pre_process(conn_idx);

    blm_pconn = (st_llm_conn_t *)&blmsMaster[blms_conn_sel];

    blms_start_common_1(blms_pconn);
    aclSniffer_mst_param.sniffer_rx_num = 0;

    rf_set_rxmode();

    if( aclConn_param.connSync & (1<<blms_conn_sel) ){
        rf_set_1st_rx_timeout(0xffffff);
    }
    else{
        rf_set_1st_rx_timeout(300 + blm_pconn->conn_tolerance_us*2 + bltPHYs.prmb_ac_us);
    }

    rf_ble_set_rx_settle(RX_SETTLE_US);

    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_RX_ON);  }

    //these logic setting executing after RX setting to save time
    blms_state = BLMS_STATE_SNIFM_S;
    systick_irq_trigger = SYS_IRQ_TRIG_BTX_POST;  //will set reg_system_tick_irq for task_post immediately

    blm_pconn->timing_update = 0;
    blm_pconn->tick_1st_rx = 0;
    blm_pconn->sSlot_mark_conn = bltSche.sSlot_idx_irq_real;
    blm_pconn->sSlot_shift_tor = blm_pconn->conn_tolerance_us*SSLOT_US_REVERSE;
    blm_pconn->expectTimeMark = blm_pconn->connExpectTime = bltSche.sSlot_tick_irq + BRX_LEFT_EARLY_TICK + blm_pconn->conn_tolerance_us * SYSTEM_TIMER_TICK_1US;
    #if (0)//(BLMS_PM_ENABLE)
        if(blmsPm.slave_idx_calib == 0xFF){
            //blmsPm.slave_idx_calib = blms_conn_sel;
        }
    #endif

    aclSniffer_mst_param.sniffer_rssi_validFlag = 0;

    blms_start_common_2(blms_pconn);

    return BLE_SUCCESS;
}


_attribute_ram_code_ int blt_sniffer_mst_post(void)
{
    if(blmsParam.rf_fsm_busy) {
        blmsParam.rf_fsm_busy = 0;

        rf_set_tx_rx_off();
        STOP_RF_STATE_MACHINE;  //stop state machine
        CLEAR_ALL_RFIRQ_STATUS;
    }

    if(blms_pconn->acl_sniffer_sync_creating){
        // clear tick_1st_rx
        blm_pconn->tick_1st_rx = 0;
    }

    int brx_sync = blms_pconn->sync_timing;

    blms_state = BLMS_STATE_SNIFM_E;
    if ( blms_post_common_1(blms_pconn) ){  // return 1: terminate happens

        blmsParam.cur_master_num --;
        blms_pconn->acl_sniffer_sync_creating = 0;
        blms_pconn->acl_sniffer_sync_update_ignore = 0;

        blt_sche_removeTaskMask(TSKMSK_ACL_CONN_0<<blms_conn_sel);  //pay attention here
        blt_sche_addUpdate(SLOT_UPDT_CONN_TERMINATE);   //triggers "bltSlot.update" valid

        #if (0)//(BLMS_PM_ENABLE)
            blmsPm.slave_no_sleep &= ~(1<<blms_conn_sel);
            if(blmsPm.slave_idx_calib == blms_conn_sel){
                blmsPm.slave_idx_calib = 0xFF;
            }
        #endif

        #if (0)//(BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE)
            blms_pconn->conn_pkt_dec_pending = 0;
            aclConn_param.updateCmd_pending &= ~BIT(blms_conn_sel);
        #endif

            blt_debug_gpio_toggle_acl_sniffer();
            blt_debug_gpio_toggle_acl_sniffer();
            blt_debug_gpio_toggle_acl_sniffer();
            blt_debug_gpio_toggle_acl_sniffer();
    }
    else{   //no terminate
        blt_ll_acl_conn_sync_process(blm_pconn->tick_1st_rx);

        /* value below are invalid when old conn_interval change to new conn_interval */
        //blm_pconn->connExpectTime = (blm_pconn->tick_1st_rx ? blm_pconn->tick_1st_rx : blms_pconn->conn_tick_mark) + blms_pconn->conn_intvl_tick;
        if(blm_pconn->tick_1st_rx){
            blm_pconn->expectTimeMark = blm_pconn->tick_1st_rx;
            blm_pconn->conn_offset_tick = blm_pconn->connExpectTime - blm_pconn->tick_1st_rx;
            blm_pconn->connExpectTime = blm_pconn->tick_1st_rx + blms_pconn->conn_intvl_tick;

            blt_debug_gpio_toggle_acl_sniffer();
            blt_debug_gpio_toggle_acl_sniffer();
        }
        else{
            blm_pconn->connExpectTime += blms_pconn->conn_intvl_tick;
        }


        /* update sync status according to RX packet receiving result */
        if(blm_pconn->tick_1st_rx)
        {
            if(blms_pconn->sync_timing)
            {
                blms_pconn->sync_timing = 0;
                blm_pconn->conn_tolerance_us = blmsParam.min_tolerance_us;
                blm_pconn->conn_start_time = blm_pconn->connExpectTime - BRX_LEFT_EARLY_TICK - blm_pconn->conn_tolerance_us * SYSTEM_TIMER_TICK_1US;
                int n_sSlot = TICKS_DUR_2_SSLOT_DUR(blm_pconn->conn_start_time - bltSche.sSlot_tick_irq_real);
                blm_pconn->sSlot_mark_conn = bltSche.sSlot_idx_irq_real + n_sSlot - blm_pconn->sSlot_interval;

                blm_pconn->sSlot_offset = 0;
                blm_pconn->timing_update = 1;
                blt_sche_addUpdate(SLOT_UPDT_SLAVE_SYNC_DONE);

                #if (0)//(BLMS_PM_ENABLE)
                    blmsPm.slave_no_sleep &= ~(1<<blms_conn_sel);
                #endif
            }
            else{
                if(blm_pconn->conn_tolerance_us > blmsParam.min_tolerance_us){
                    blm_pconn->conn_tolerance_us = blmsParam.min_tolerance_us;
                    blm_pconn->conn_start_time = blm_pconn->connExpectTime - BRX_LEFT_EARLY_TICK - blm_pconn->conn_tolerance_us * SYSTEM_TIMER_TICK_1US;
                    int n_sSlot = TICKS_DUR_2_SSLOT_DUR(blm_pconn->conn_start_time - bltSche.sSlot_tick_irq_real);
                    blm_pconn->sSlot_mark_conn = bltSche.sSlot_idx_irq_real + n_sSlot - blm_pconn->sSlot_interval;

                    blm_pconn->sSlot_offset = 0;
                    blm_pconn->timing_update = 1;
                    blt_sche_addUpdate(SLOT_UPDT_SLAVE_SYNC_DONE);
                }
                else{
                    u32 tick_offset_1st_rx = blm_pconn->tick_1st_rx - bltSche.sSlot_tick_irq_real;
                    u32 tick_offset_expect = blmsParam.min_tolerance_us * SYSTEM_TIMER_TICK_1US + BRX_LEFT_EARLY_TICK;
                    blm_pconn->sSlot_offset = TICKS_DUR_2_SSLOT_DUR(tick_offset_1st_rx - tick_offset_expect);

                    #if (BRX_HALF_MARGIN_SSLOT_NUM == 3)
                        if(blm_pconn->sSlot_offset < -1 || blm_pconn->sSlot_offset > 2)
                    #elif (BRX_HALF_MARGIN_SSLOT_NUM == 2)
                        if(blm_pconn->sSlot_offset < 0 || blm_pconn->sSlot_offset > 1)
                    #else
                        #error "add code here"
                    #endif
                        {
                            blm_pconn->conn_tolerance_us = blmsParam.min_tolerance_us;
                            blm_pconn->timing_update = 1;
                            blt_sche_addUpdate(SLOT_UPDT_SLAVE_SSLOT_ADJUST);
                        }
                }
            }

            if(blm_pconn->timing_update){
                blm_pconn->sSlot_shift_tor = blmsParam.min_tolerance_us*SSLOT_US_REVERSE;
                /* tolerance*2/19.53 = tolerances/10 */
                #if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
                    blms_pconn->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + pdu_27b_tifs_27b_sslot[blms_pconn->connPhyCtrl.conn_cur_phy - 1][blms_pconn->crypt.enable] + blm_pconn->conn_tolerance_us/10;
                #else
                    blms_pconn->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + pdu_27b_tifs_27b_sslot[0][blms_pconn->crypt.enable] + blm_pconn->conn_tolerance_us/10;
                #endif
            }
        }

        #if (BLMS_PM_ENABLE)
            if(brx_sync){
                blm_pconn->rx_pkt_miss = 0;
            }
            else{
                if(blm_pconn->tick_1st_rx)
                {
                    blm_pconn->rx_pkt_miss = 0;
                    blms_pconn->pm_error_us = 0;
                    blm_pconn->sleep_32k_rc = 0;
                }
                else
                {
                    if (blm_pconn->rx_pkt_miss < 5){
                        blm_pconn->rx_pkt_miss ++;
                    }
                }
            }
        #endif

        if(aclSniffer_mst_param.sniffer_rssi_reportType == RSSI_TYPE_ALL){
            if(!aclSniffer_mst_param.sniffer_rssi_validFlag){
                u8 *raw_pkt = (u8 *) (sniffer_rx_null_fifo.p + sniffer_rx_null_fifo.wptr * sniffer_rx_null_fifo.size);

                raw_pkt[1] = blms_pconn->conn_chn;
                raw_pkt[2] = blms_pconn->acl_conHandle;
                raw_pkt[3] = 0;
                #if (DEBUG_SNIFFER_REPORT_INSTANT_EN)
                    raw_pkt[4] = blms_pconn->conn_inst - 1;
                #endif

                sniffer_rx_null_fifo.wptr == (sniffer_rx_null_fifo.num - 1) ? sniffer_rx_null_fifo.wptr = 0 : sniffer_rx_null_fifo.wptr++;
            }
        }
    }

    blms_post_common_2();

    return BLE_SUCCESS;
}
#endif

_attribute_ram_code_ int blt_acl_sniffer_mst_irq_task(int flag)
{
    int idx = flag & FLAG_SCHEDULE_TASK_IDX_MASK;

    if(flag & FLAG_IRQ_RX){
        #if (SNIFFER_USE_SOME_COMMON_APIS)
            irq_acl_sniffer_rx();
        #else
            irq_acl_sniffer_mst_rx();
        #endif
    }
    else if(flag & FLAG_SCHEDULE_START){
        #if (SNIFFER_USE_SOME_COMMON_APIS)
            blt_sniffer_start(idx);
        #else
            blt_sniffer_mst_start(idx);
        #endif
    }
    else if(flag & FLAG_SCHEDULE_DONE){
        #if (SNIFFER_USE_SOME_COMMON_APIS)
            blt_sniffer_post();
        #else
            blt_sniffer_mst_post();
        #endif
    }
    else if(flag & FLAG_ACL_SNIFFER_SEEK_RX){
        #if (SNIFFER_USE_SOME_COMMON_APIS)
            irq_acl_sniffer_seek_rx();
        #else
            irq_acl_sniffer_mst_seek_rx();
        #endif
    }
    else if(flag & FLAG_ACL_SNIFFER_SEEK_START){
        #if (SNIFFER_USE_SOME_COMMON_APIS)
            blt_sniffer_seek_start(idx, ACL_ROLE_CENTRAL);
        #else
            blt_sniffer_mst_seek_start(idx);
        #endif
    }
    else if(flag & FLAG_ACL_SNIFFER_SEEK_POST){
        #if(SNIFFER_USE_SOME_COMMON_APIS)
            blt_sniffer_seek_post(ACL_ROLE_CENTRAL);
        #else
            blt_sniffer_mst_seek_post();
        #endif
    }
    else if(flag & FLAG_ACL_SNIFFER_SEEK){
        if(blmsParam.state_chng & STATE_CHANGE_ACL_SNIFM){
            #if (SNIFFER_USE_SOME_COMMON_APIS)
                blt_ll_sniffer_seek_anchor(acl_sniffer_mst_sync_info);
            #else
                blt_ll_sniffer_mst_seek_anchor(acl_sniffer_mst_sync_info);
            #endif
        }
    }
    else if(flag & FLAG_SCHEDULE_BUILD){
        #if (SNIFFER_USE_SOME_COMMON_APIS)
            blt_ll_buildAclSnifferSeekSchLinklist(ACL_ROLE_CENTRAL);
        #else
            blt_ll_buildAclSnifferMstSeekSchLinklist();
        #endif
    }

    return BLE_SUCCESS;
}



void blc_ll_setAclSnifferMstSeekTimeout(u16 timeout_ms)
{
    if(timeout_ms < 300){
        timeout_ms = 300;
    }

    aclSniffer_mst_param.sniffer_seek_timeout_ms = timeout_ms;
}


void blc_ll_setAclSnifferMstSeekHalfWindow(u8 halfWindow_ms)
{
    if(halfWindow_ms < 5){
        halfWindow_ms = 5;
    }
    else if(halfWindow_ms > RSSI_SNIFFER_SEEK_TOLERANCE_MAX_MS){
        halfWindow_ms = RSSI_SNIFFER_SEEK_TOLERANCE_MAX_MS;
    }

    aclSniffer_mst_param.sniffer_seek_halfWindow_ms = halfWindow_ms;
}


void blc_ll_setAclSnifferMstSeekCount(u8 seekCount)
{
    if(seekCount < 1){
        seekCount = 1;
    }
    else if(seekCount > 10){
        seekCount = 10;
    }

    aclSniffer_mst_param.sniffer_seek_count_max = seekCount;
}


int blc_ll_updateAclSnifferMstSync(u8 *cmd)
{
    acl_sniffer_sync_param_t* param = (acl_sniffer_sync_param_t*)cmd;

    if(param->sync_conn_num_config != CONN_MAX_NUM_CONFIG){
        return SNIFFER_PARAMETER_INVALID;
    }

    #if (LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2)
        if(param->sync_conn_chnSel && (!ll_chn_index_calc_cb)){
            return SNIFFER_UNSUPPORTED_FEATURE;
        }
    #endif

    #if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
        if(((param->sync_CI_phy & 0x0F) != BLE_PHY_1M) && (!ll_phy_switch_cb)){
            return SNIFFER_UNSUPPORTED_FEATURE;
        }
    #endif

    #if (1)//move from blt_ll_sniffer_mst_seek_anchor()
        u8 pc_conn_sel = param->sync_connHandle & CONN_IDX_MASK;

        if(pc_conn_sel >= LL_MAX_ACL_CEN_NUM){
            return SNIFFER_UNKNOWN_SNIFHANDLE;
        }

        st_ll_conn_t *pc_conn =  (st_ll_conn_t *)&blms[pc_conn_sel];
        st_llm_conn_t *blm_pconn = (st_llm_conn_t *)&blmsMaster[pc_conn_sel];

        acl_sniffer_seek_param_t *cur_SnifferSeek = (acl_sniffer_seek_param_t *)&aclSniffer_mst_seek[pc_conn_sel];

        u32 r = irq_disable();

        if(param->sync_conn_timeout == 0){
            //stop sniffer
            if(pc_conn->connState && !cur_SnifferSeek->seeking){
                pc_conn->conn_termin_union.peer_terminate = HCI_ERR_REMOTE_USER_TERM_CONN;
            }
            else if(!pc_conn->connState && cur_SnifferSeek->seeking){
                cur_SnifferSeek->seek_stop = 1;
            }
            else if(pc_conn->connState && cur_SnifferSeek->seeking){
                pc_conn->conn_termin_union.peer_terminate = HCI_ERR_REMOTE_USER_TERM_CONN;
                cur_SnifferSeek->seek_stop = 2;
            }
            else{
                irq_restore(r);
                return SNIFFER_USER_STOP_NOT_SUPPORTED;
            }

            irq_restore(r);
            return BLE_SUCCESS;
        }

        /* if the sync parameters are the same does not update create
         * blt_ll_acl_sniffer_mainloop reply sync result 'SNIFFER_SYNC_CREATE'
         */
        if(pc_conn->connState == CONN_STATUS_ESTABLISH && !pc_conn->acl_sniffer_sync_creating){
            if(blm_pconn->rx_pkt_miss < 4){
                if((pc_conn->conn_intvl_n_1m25 == param->sync_intvl_n_1m25) \
                    && (pc_conn->conn_timeout == param->sync_conn_timeout) \
                    && (!smemcmp(pc_conn->acl_chnParam.chmTbl, param->sync_conn_chm, 5)) \
                    && (pc_conn->conn_chn_hop == param->sync_conn_hop) \
                    && (pc_conn->aclAccessAddr == param->sync_accessAddr) \
                    && (pc_conn->aclCrcInit == param->sync_crcInit) \
                    && (pc_conn->conn_chnsel == param->sync_conn_chnSel) \
                    && (pc_conn->connPhyCtrl.conn_cur_phy == (param->sync_CI_phy & 0x0F)) \
                    && (pc_conn->connPhyCtrl.conn_cur_CI == (param->sync_CI_phy & 0xF0 >> 4)))
                {
                    pc_conn->acl_sniffer_sync_update_ignore = 1;
                    irq_restore(r);
                    return BLE_SUCCESS;
                }
            }
        }

        if(cur_SnifferSeek->seeking){
            irq_restore(r);
            return SNIFFER_SEEK_IN_PROGRESS;
        }

        irq_restore(r);
    #endif

    if(blmsParam.state_chng & STATE_CHANGE_ACL_SNIFM){
        return SNIFFER_CURRENT_STATE_NOT_SUPPORTED_THIS_CMD;
    }

    cur_SnifferSeek->seek_repeat_type = 0;//not need, support re-listening during a existing connection

    smemcpy(acl_sniffer_mst_sync_info, cmd, sizeof(acl_sniffer_sync_param_t));

    blmsParam.state_chng |= STATE_CHANGE_ACL_SNIFM;

    return BLE_SUCCESS;
}


/**
 * @brief      for user to obtain the sniffer handle for compatible mode.
 * @param[in]  master_index - 0 ~ 3.
 * @return     sniffer handle
 *                  0x00:  input parameter is incorrect
 *                  other: correct sniffer handle
 */
u16 blc_ll_getAclSnifferMstHandle_v2(u8 master_index)
{
    if(master_index >= LL_MAX_ACL_CEN_NUM){
        return 0;
    }

    return (master_index | BLM_CONN_HANDLE);
}


#if (!SNIFFER_USE_SOME_COMMON_APIS)
_attribute_ram_code_ int blt_ll_sniffer_mst_seek_anchor(u8 *cmd)
{
    acl_sniffer_sync_param_t* param = (acl_sniffer_sync_param_t*)cmd;

    #if (0)//move to blc_ll_updateAclSnifferMstSync()
        blms_conn_sel = param->sync_connHandle & CONN_IDX_MASK;
        blms_pconn =  (st_ll_conn_t *)   &blms[blms_conn_sel];
        blm_pconn = (st_llm_conn_t *)&blmsMaster[blms_conn_sel];

        /* if the sync parameters are the same does not update create
         * blt_ll_acl_sniffer_mainloop reply sync result 'SNIFFER_SYNC_CREATE'
         */
        if(blms_pconn->connState == CONN_STATUS_ESTABLISH && !blms_pconn->acl_sniffer_sync_creating){
            if(blm_pconn->rx_pkt_miss < 4){
                if((blms_pconn->conn_intvl_n_1m25 == param->sync_intvl_n_1m25) \
                    && (blms_pconn->conn_timeout == param->sync_conn_timeout) \
                    && (!smemcmp(blms_pconn->acl_chnParam.chmTbl, param->sync_conn_chm, 5)) \
                    && (blms_pconn->conn_chn_hop == param->sync_conn_hop) \
                    && (blms_pconn->aclAccessAddr == param->sync_accessAddr) \
                    && (blms_pconn->aclCrcInit == param->sync_crcInit) \
                    && (blms_pconn->conn_chnsel == param->sync_conn_chnSel) \
                    && (blms_pconn->connPhyCtrl.conn_cur_phy == (param->sync_CI_phy & 0x0F)) \
                    && (blms_pconn->connPhyCtrl.conn_cur_CI == (param->sync_CI_phy & 0xF0 >> 4)))
                {
                    blms_pconn->acl_sniffer_sync_update_ignore = 1;

                    return BLE_SUCCESS;
                }
            }
        }
    #else
        blms_conn_sel = param->sync_connHandle & CONN_IDX_MASK;
    #endif

    pSnifferMstSeek = (acl_sniffer_seek_param_t *)&aclSniffer_mst_seek[blms_conn_sel];
    smemcpy((u8*)&pSnifferMstSeek->sync.sync_connHandle, (u8*)&param->sync_connHandle, sizeof(acl_sniffer_sync_param_t));

    smemcpy(pSnifferMstSeek->chnParam.chmTbl, param->sync_conn_chm, 5);
    #if (LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2)
        if(pSnifferMstSeek->sync.sync_conn_chnSel)
        {
            pSnifferMstSeek->seek_chnIdentifier = (pSnifferMstSeek->sync.sync_accessAddr>>16) ^ (pSnifferMstSeek->sync.sync_accessAddr&0xffff);
            csa2_calculateMapInfo(&pSnifferMstSeek->chnParam);
        }
        else
        {
            blt_csa1_calculateChannelTable (pSnifferMstSeek->sync.sync_conn_chm, pSnifferMstSeek->sync.sync_conn_hop, pSnifferMstSeek->chnParam.rempChmTbl);
        }
    #else
        blt_csa1_calculateChannelTable (pSnifferMstSeek->sync.sync_conn_chm, pSnifferMstSeek->sync.sync_conn_hop, pSnifferMstSeek->chnParam.rempChmTbl);
    #endif

    pSnifferMstSeek->tick_seek_interval = param->sync_intvl_n_1m25 * SYSTEM_TIMER_TICK_1250US;
    pSnifferMstSeek->sSlot_seek_interval = TICKS_DUR_2_SSLOT_DUR(pSnifferMstSeek->tick_seek_interval);

    // seek_tolerance cannot be greater than bltSche.bSlot_maxLen/2
    if(aclSniffer_mst_param.sniffer_1st_rxWindowMaxFlag){
        pSnifferMstSeek->tick_seek_tolerance_ms = pSnifferMstSeek->tick_seek_tolerance_max_ms;
    }
    else{
        pSnifferMstSeek->tick_seek_tolerance_ms = aclSniffer_mst_param.sniffer_seek_halfWindow_ms;
    }
    pSnifferMstSeek->sSlot_seek_tolerance = TICKS_DUR_2_SSLOT_DUR(pSnifferMstSeek->tick_seek_tolerance_ms * SYSTEM_TIMER_TICK_1MS);

    if(pSnifferMstSeek->sync.sync_expectTime < bltSche.sSlot_tick_irq_real){
        u16 jump_num;
        jump_num = (bltSche.sSlot_tick_irq_real - pSnifferMstSeek->sync.sync_expectTime) / pSnifferMstSeek->tick_seek_interval + 1;
        pSnifferMstSeek->sync.sync_expectTime += pSnifferMstSeek->tick_seek_interval*jump_num;
        pSnifferMstSeek->sync.sync_conn_inst += jump_num;
        pSnifferMstSeek->sync.sync_chn_idx = ((u32)(pSnifferMstSeek->sync.sync_chn_idx + jump_num))%37;
    }

    int n_sSlot = TICKS_DUR_2_SSLOT_DUR(pSnifferMstSeek->sync.sync_expectTime - bltSche.sSlot_tick_irq_real);
    pSnifferMstSeek->sSlot_sync_expectTime = bltSche.sSlot_idx_irq_real + n_sSlot;
    pSnifferMstSeek->seek_count = 0;
    pSnifferMstSeek->seek_state = 0;
    pSnifferMstSeek->seeking = 1;
    pSnifferMstSeek->seek_stop = 0;
    pSnifferMstSeek->tick_seek_start = clock_time() | 1;

    blt_ll_setSchedulerTaskPriority( TSKOFT_SNIFM_SEEK + blms_conn_sel, TASK_PRIORITY_MAX );

    blt_sche_addTaskMask(TSKMSK_SNIFM_SEEK_0 << blms_conn_sel);
    blt_sche_addUpdate(SLOT_UPDT_SNIF_SEEK_CREATE);

    return BLE_SUCCESS;
}


_attribute_ram_code_ int blt_ll_buildAclSnifferMstSeekSchLinklist(void)
{
    u32 i,j;

    for( i = 0;i < TSKNUM_SNIFM_SEEK;i++ )
    {
        if( bltSche.task_mask & (TSKMSK_SNIFM_SEEK_0<<i) )
        {
            pSnifferMstSeek = (acl_sniffer_seek_param_t *)&aclSniffer_mst_seek[i];

            if(bltSche.sSlot_idx_reset == 1 && bltSche.build_index == 0){
                pSnifferMstSeek->sSlot_sync_expectTime -= bltSche.sSlot_idx_past;
            }

            s32 sSlot_first_seek_window_tail = pSnifferMstSeek->sSlot_sync_expectTime + pSnifferMstSeek->sSlot_seek_tolerance;
            s32 sSlot_sche_start = bltSche.sSlot_idx_next;
            u16 first_seek_jump_num = 0;

            if(sSlot_first_seek_window_tail < sSlot_sche_start){
                first_seek_jump_num = (sSlot_sche_start - sSlot_first_seek_window_tail) / pSnifferMstSeek->sSlot_seek_interval + 1;
            }

            s32 sSlot_last_seek_window_head = pSnifferMstSeek->sSlot_sync_expectTime - pSnifferMstSeek->sSlot_seek_tolerance;
            s32 sSlot_sche_end = bltSche.sSlot_endIdx_dft;  // 126 -> 78.75ms
            u16 last_seek_jump_num = 0;

            if(sSlot_last_seek_window_head < sSlot_sche_end){
                last_seek_jump_num = (sSlot_sche_end - sSlot_last_seek_window_head) / pSnifferMstSeek->sSlot_seek_interval;
            }

            u16 jump_num_min = min(first_seek_jump_num, last_seek_jump_num);
            u16 jump_num_max = max(first_seek_jump_num, last_seek_jump_num);
            pSnifferMstSeek->jump_num_valid = 0;

            s32 sSlot_task_duration_max = 0;
            for(j = jump_num_min; j < jump_num_max; j++)
            {
                s32 sSlot_task_start = pSnifferMstSeek->sSlot_sync_expectTime + pSnifferMstSeek->sSlot_seek_interval * j - pSnifferMstSeek->sSlot_seek_tolerance;
                if(sSlot_task_start > sSlot_sche_end){
                    break;
                }
                s32 sSlot_task_end = pSnifferMstSeek->sSlot_sync_expectTime + pSnifferMstSeek->sSlot_seek_interval * j + pSnifferMstSeek->sSlot_seek_tolerance;
                s32 sSlot_task_duration_cur = min(sSlot_task_end, sSlot_sche_end) - max(sSlot_task_start, sSlot_sche_start);// less than seek_tolerance*2

                if(sSlot_task_duration_cur > sSlot_task_duration_max){
                    sSlot_task_duration_max = sSlot_task_duration_cur;
                    pSnifferMstSeek->jump_num_valid = j;
                    if(sSlot_task_duration_max >= (pSnifferMstSeek->sSlot_seek_tolerance << 1)){
                        break;
                    }
                }
            }

            pSnifferMstSeek->sSlot_seek_expectTime = pSnifferMstSeek->sSlot_sync_expectTime + pSnifferMstSeek->sSlot_seek_interval * pSnifferMstSeek->jump_num_valid;

            s32 sSlot_seek_window_head;
            sSlot_seek_window_head = pSnifferMstSeek->sSlot_seek_expectTime - pSnifferMstSeek->sSlot_seek_tolerance;
            if(sSlot_seek_window_head >= sSlot_sche_end){
                continue;
            }
            if(sSlot_seek_window_head < sSlot_sche_start + 2){
                sSlot_seek_window_head = sSlot_sche_start + 2;
            }

            s32 sSlot_seek_window_tail;
            sSlot_seek_window_tail = pSnifferMstSeek->sSlot_seek_expectTime + pSnifferMstSeek->sSlot_seek_tolerance;
            if(sSlot_seek_window_tail <= sSlot_sche_start){
                continue;
            }
            if(sSlot_seek_window_tail > sSlot_sche_end - 2){
                sSlot_seek_window_tail = sSlot_sche_end - 2;
            }

            u32 scheduler_use_us = bltSche.sche_process_us + SCHE_NEW_TASK_MARGIN_US;
            s32 sSlot_sche_use = scheduler_use_us * SSLOT_US_REVERSE;
            sSlot_seek_window_tail = sSlot_seek_window_tail - sSlot_sche_use;

            if(sSlot_seek_window_head >= sSlot_seek_window_tail){
                #if(SCH_DEBUG_EN)
                    write_dbg32(DBG_SRAM_ADDR + 4, sSlot_sche_start);
                    write_dbg32(DBG_SRAM_ADDR + 8, sSlot_seek_window_head);
                    write_dbg32(DBG_SRAM_ADDR + 12, sSlot_seek_window_tail);
                    BLMS_ERR_DEBUG(SCH_DEBUG_EN, 0xEE020000);
                #endif
                continue;
            }

            //pSnifferMstSeek->sSlot_seek_duration = (u32)(sSlot_seek_window_tail - sSlot_seek_window_head);
            pSnifferMstSeek->sSlot_seek_duration = sSlot_seek_window_tail - sSlot_seek_window_head;
            if(pSnifferMstSeek->sSlot_seek_duration < pSnifferMstSeek->sSlot_seek_tolerance){
                //actual seek duration less than setting seek half window
                continue;
            }

            sch_task_t  *pCur_schTask = (sch_task_t *)&pSnifferMstSeek->snifferTsk_fifo;

            pCur_schTask->begin = sSlot_seek_window_head;
            pCur_schTask->end = sSlot_seek_window_tail;

            #if (0)//Note: does not match the design, no longer needed. correct fix reference SHA-1: 91d6f2b89188e79b17b73e137af18d3c34241bff
                //***Important***
                //record the task with highest priority, to guarantee that task not missed
                //prevent other tasks use bltPri.priMax_value to change bltSche.sSlot_endIdx_maxPri
                //finally cause the seek task is kicked out
                if(bltPri.pri_cal[TSKOFT_SNIFM_SEEK + i] > bltPri.priMax_value){
                    bltPri.priMax_value = bltPri.pri_cal[TSKOFT_SNIFM_SEEK + i];
                    bltPri.priMax_index = TSKOFT_SNIFM_SEEK + i;
                    //bltSche.sSlot_endIdx_maxPri = pCur_schTask->end + 1;// no need
                }
            #endif

            blt_ll_addTask2ExistLinklist(pCur_schTask, 1);

            break;//currently only one seek task is assigned
        }
    }

    return BLE_SUCCESS;
}


_attribute_ram_code_ int blt_sniffer_mst_seek_start (int seek_idx)
{
    //pay attention here, seek_idx 0~3(TSKNUM_SNIFM_SEEK-1)
    blms_start_pre_process(seek_idx);
    blt_debug_gpio_toggle_acl_sniffer();
    blt_debug_gpio_toggle_acl_sniffer();

    blm_pconn = (st_llm_conn_t *)&blmsMaster[blms_conn_sel];

    aclSniffer_mst_param.sniffer_rx_num = 0;
    aclSniffer_mst_param.sniffer_seek_tick_1st_rx = 0;

    //reset_baseband(); //QiangKai: Eagle can not reset, all RF baseband setting will lost(But Kite/Vulture must add this)

    ble_rf_set_rx_dma((u8*)aclConn_param.acl_rx_dma_buff, aclConn_param.acl_rx_dma_size);
    u16 rx_max_len = blt_llms_get_connEffectiveMaxRxOctets_by_connIdx(blms_conn_sel);
    rf_set_rx_maxlen(rx_max_len+4);//MCI 4ytes

    pSnifferMstSeek = (acl_sniffer_seek_param_t *)&aclSniffer_mst_seek[blms_conn_sel];

    u8 seek_chn;
    u8 seek_chn_idx = ((u32)(pSnifferMstSeek->sync.sync_chn_idx + pSnifferMstSeek->jump_num_valid))%37;
    u16 seek_inst = pSnifferMstSeek->sync.sync_conn_inst + pSnifferMstSeek->jump_num_valid;
    #if (LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2)
        if(pSnifferMstSeek->sync.sync_conn_chnSel)
        {
            seek_chn = ll_chn_index_calc_cb(&pSnifferMstSeek->chnParam, seek_inst, pSnifferMstSeek->seek_chnIdentifier);
        }
        else
        {
            seek_chn = pSnifferMstSeek->chnParam.rempChmTbl[seek_chn_idx];
        }
    #else
        seek_chn = pSnifferMstSeek->chnParam.rempChmTbl[seek_chn_idx];
    #endif

    #if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
        if(ll_phy_switch_cb){
            u8 sync_CI_phy = pSnifferMstSeek->sync.sync_CI_phy;//0~3bits: le_phy_type_t; 4~7bits: le_coding_ind_t
            ll_phy_switch_cb(sync_CI_phy & 0x0F, (sync_CI_phy & 0xF0) >> 4); //rf_ble_switch_phy
        }
    #endif

    rf_set_ble_channel(seek_chn);
    rf_set_ble_access_code((u8 *)&pSnifferMstSeek->sync.sync_accessAddr);
    rf_set_ble_crc_value(pSnifferMstSeek->sync.sync_crcInit);
    CLEAR_ALL_RFIRQ_STATUS;

    rf_set_rxmode();

    rf_set_1st_rx_timeout(0xffffff);

    rf_ble_set_rx_settle(RX_SETTLE_US);

    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_RX_ON);  }

    blms_state = BLMS_STATE_SNIFM_SEEK_S;
    systick_irq_trigger = SYS_IRQ_TRIG_SNIFM_SEEK_POST;

    aclConn_param.task_end_tick = bltSche.sSlot_tick_irq_real + SSLOT_DUR_2_TICKS_DUR(pSnifferMstSeek->sSlot_seek_duration);

    systimer_set_irq_capture(aclConn_param.task_end_tick - (SLOT_PROCESS_MAX_TICK + BOUNDARY_RX_RELOAD_TICK));

    return BLE_SUCCESS;
}


_attribute_ram_code_ int blt_sniffer_mst_seek_post(void)
{
    if(blmsParam.rf_fsm_busy) {
        blmsParam.rf_fsm_busy = 0;

        rf_set_tx_rx_off();
        STOP_RF_STATE_MACHINE;  //stop state machine
        CLEAR_ALL_RFIRQ_STATUS;
    }

    blms_state = BLMS_STATE_SNIFM_SEEK_E;

    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_OFF);  }

    pSnifferMstSeek->seek_count++;

    if((pSnifferMstSeek->seek_count >= aclSniffer_mst_param.sniffer_seek_count_max) && (!pSnifferMstSeek->seek_state)){
        //reach seek max count
        #if (0)
            pSnifferMstSeek->seek_state = 2;//Fail
        #else
            if(pSnifferMstSeek->seek_repeat_type){
                pSnifferMstSeek->seek_repeat_count++;
                if(pSnifferMstSeek->seek_repeat_count < 4){
                    if(pSnifferMstSeek->seek_repeat_count < 3){
                        if(pSnifferMstSeek->seek_repeat_type != MASTER_STATUS_CONNECTION_SETUP){
                            pSnifferMstSeek->sync.sync_conn_inst--;
                            if(pSnifferMstSeek->sync.sync_chn_idx == 0){
                                pSnifferMstSeek->sync.sync_chn_idx = 36;
                            }
                            else{
                                pSnifferMstSeek->sync.sync_chn_idx--;
                            }
                        }
                        pSnifferMstSeek->sync.sync_expectTime -= pSnifferMstSeek->tick_seek_interval;
                        pSnifferMstSeek->sSlot_sync_expectTime -= pSnifferMstSeek->sSlot_seek_interval;
                    }
                    else if(pSnifferMstSeek->seek_repeat_count == 3){
                        if(pSnifferMstSeek->seek_repeat_type != MASTER_STATUS_CONNECTION_SETUP){
                            pSnifferMstSeek->sync.sync_conn_inst += 3;
                            pSnifferMstSeek->sync.sync_chn_idx = ((u32)(pSnifferMstSeek->sync.sync_chn_idx + 3))%37;
                        }
                        pSnifferMstSeek->sync.sync_expectTime += pSnifferMstSeek->tick_seek_interval * 3;
                        pSnifferMstSeek->sSlot_sync_expectTime += pSnifferMstSeek->sSlot_seek_interval * 3;
                    }
                    pSnifferMstSeek->seek_count = 0;
                }
                else{
                    pSnifferMstSeek->seek_state = 2;//Fail
                }
            }
            else{
                pSnifferMstSeek->seek_state = 2;//Fail
            }
        #endif
    }

    if(pSnifferMstSeek->seek_stop == 1){
        pSnifferMstSeek->seek_state = 3;//report stop event
    }
    else if(pSnifferMstSeek->seek_stop == 2){
        pSnifferMstSeek->seek_state = 4;//not report stop event
    }

    if(pSnifferMstSeek->seek_state){
        pSnifferMstSeek->seeking = 0;
        pSnifferMstSeek->seek_stop = 0;
        pSnifferMstSeek->tick_seek_start = 0;

        blt_sche_removeTaskMask(TSKMSK_SNIFM_SEEK_0<<blms_conn_sel);  //pay attention here

        if(pSnifferMstSeek->seek_state == 1){//Succeed
            blt_debug_gpio_toggle_acl_sniffer();
            blt_debug_gpio_toggle_acl_sniffer();

            blt_sniffer_mst_seek2sync();
        }
        else{
            blt_sche_addUpdate(SLOT_UPDT_SNIF_SEEK_FAIL);
        }
    }
    else{
        blt_sche_addUpdate(SLOT_UPDT_SNIF_SEEK_FAIL);
    }

    blms_post_common_2();

    return BLE_SUCCESS;
}


_attribute_ram_code_ int irq_acl_sniffer_mst_seek_rx(void)
{
    u8 * raw_pkt = (u8 *) aclConn_param.acl_rx_dma_buff;

    HAL_CLEAR_RF_RX_IRQ;

    static u8 seek_rfLen_1st_rx;

    /* "rx header tick" none zero is under condition CRC correct, so here do not check CRC by
       "RF_BLE_PACKET_VALIDITY_CHECK" to save RamCode and running timing
       Or we can use "bltRxPkt.crc correct" */
    if(bltRxPkt.crc_correct)
    {
        DBG_CHN2_TOGGLE;

        if(bltRxPkt.rx_header_tick)
        {
            if(!aclSniffer_mst_param.sniffer_rx_num){
                if(!aclSniffer_mst_param.sniffer_seek_tick_1st_rx){
                    //need to continue monitor the peer-slave
                    aclSniffer_mst_param.sniffer_seek_tick_1st_rx = bltRxPkt.rx_header_tick;
                    seek_rfLen_1st_rx = raw_pkt[DMA_RFRX_OFFSET_RFLEN];

                    //remote:  |1st_timing|<---T_IFS--->|2st_timing|<---MARGIN--->
                    u32 pkt_receive_cost_us = blt_phy_getRfPacketTime_us(seek_rfLen_1st_rx, bltPHYs.cur_llPhy, bltPHYs.cur_peer_CI);
                    pkt_receive_cost_us += BLE_T_IFS;
                    pkt_receive_cost_us += blt_phy_getRfPacketTime_us(255, bltPHYs.cur_llPhy, LE_CODED_S8);//the length is considered the maximum
                    pkt_receive_cost_us += SNIFFER_SEEK_RX_POST_MARGIN_US;

                    u32 seek_post_tick = aclSniffer_mst_param.sniffer_seek_tick_1st_rx + pkt_receive_cost_us * SYSTEM_TIMER_TICK_1US;
                    if((seek_post_tick < systimer_get_irq_capture()) && (seek_post_tick > (clock_time () + 50*SYSTEM_TIMER_TICK_1US))){
                        systimer_set_irq_capture(seek_post_tick);
                    }

                    DBG_CHN3_TOGGLE;
                    DBG_CHN3_TOGGLE;
                }
            }
            else if(aclSniffer_mst_param.sniffer_rx_num == 1)
            {
                if(aclSniffer_mst_param.sniffer_seek_tick_1st_rx){
                    u32 diff = bltRxPkt.rx_header_tick - aclSniffer_mst_param.sniffer_seek_tick_1st_rx;

                    //e.g.: For 1M: 10 Byte = 1B(preamble) + 4B(accesscode) + 2B(header) + 3B(CRC), 150 is T_IFS

                    //remote:  |1st_timing|<---T_IFS--->|2st_timing|
                    //local:   |rx_head_tick + 1st_timing + T_IFS
                    u32 diff_ideal = (blt_phy_getRfPacketTime_us(seek_rfLen_1st_rx, bltPHYs.cur_llPhy, bltPHYs.cur_peer_CI) + BLE_T_IFS) * SYSTEM_TIMER_TICK_1US;

                    // T_IFS within 20us
                    if((diff > (diff_ideal - 20 * SYSTEM_TIMER_TICK_1US)) && (diff < (diff_ideal + 20 * SYSTEM_TIMER_TICK_1US))){
                        //monitor the peer-slave successful
                        pSnifferMstSeek->seek_state = 1;//Succeed

                        DBG_CHN3_TOGGLE;
                        DBG_CHN3_TOGGLE;
                        DBG_CHN3_TOGGLE;
                        DBG_CHN3_TOGGLE;
                    }
                }

                blt_sniffer_stop_rx_window(50);
            }
            else{
                blt_sniffer_stop_rx_window(50);
            }
        }

        aclSniffer_mst_param.sniffer_rx_num ++;  //care CRC
    }

    return BLE_SUCCESS;
}
#endif


_attribute_ram_code_ int blt_sniffer_mst_seek2sync(void){

#if (DEBUG_SNIFFER_NULL_POINTER_EN)
    if(!pSnifferMstSeek){
        printf("err:%s Line:%d\n", __FILE__, __LINE__);
    }
#endif

#if (0)
    //no need
    if(blms_conn_sel >= LL_MAX_ACL_CEN_NUM){
        return SNIFFER_UNKNOWN_SNIFHANDLE;
    }
    
    blms_pconn = (st_ll_conn_t *)&blms[blms_conn_sel];
    blm_pconn = (st_llm_conn_t *)&blmsMaster[blms_conn_sel];
#endif

    ///////////////////////////// blms_connect_common Begin /////////////////////////////
    blms_pconn->aclAccessAddr = pSnifferMstSeek->sync.sync_accessAddr;
    blms_pconn->aclCrcInit = pSnifferMstSeek->sync.sync_crcInit;

    blms_pconn->conn_intvl_n_1m25 = pSnifferMstSeek->sync.sync_intvl_n_1m25;
    blms_pconn->conn_intvl_tick = pSnifferMstSeek->sync.sync_intvl_n_1m25 * SYSTEM_TIMER_TICK_1250US;
    blt_ll_set_interval_level(TSKOFT_ACL_CONN + blms_conn_sel, pSnifferMstSeek->sync.sync_intvl_n_1m25);

    blms_pconn->conn_latency = 0;
    blms_pconn->conn_timeout = pSnifferMstSeek->sync.sync_conn_timeout;
    blms_pconn->conn_chn_hop = pSnifferMstSeek->sync.sync_conn_hop;
    blms_pconn->conn_sca = 0;//0: 251 ppm to 500 ppm
    smemcpy(blms_pconn->acl_chnParam.chmTbl, pSnifferMstSeek->sync.sync_conn_chm, 5);

    blms_pconn->conn_chnsel = pSnifferMstSeek->sync.sync_conn_chnSel;
    blms_pconn->chn_idx = ((u32)(pSnifferMstSeek->sync.sync_chn_idx + pSnifferMstSeek->jump_num_valid))%37;
#if(LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2)
    if(blms_pconn->conn_chnsel)
    {
        blms_pconn->chnIdentifier = (blms_pconn->aclAccessAddr>>16) ^ (blms_pconn->aclAccessAddr&0xffff);
        csa2_calculateMapInfo(&blms_pconn->acl_chnParam);
    }
    else
    {
        blt_csa1_calculateChannelTable (pSnifferMstSeek->sync.sync_conn_chm, blms_pconn->conn_chn_hop, blms_pconn->acl_chnParam.rempChmTbl);
    }
#else
    blt_csa1_calculateChannelTable (pSnifferMstSeek->sync.sync_conn_chm, blms_pconn->conn_chn_hop, blms_pconn->chnParam.rempChmTbl);
#endif

    // need to check whether the sniffer sync_connState already exists.
    // if the Sniffer sync_connState does not exist, add 1
    if(blms_pconn->connState == CONN_STATUS_DISCONNECT){
        blmsParam.cur_master_num ++;
        if(blmsParam.cur_master_num > blmsParam.max_master_num){
            blmsParam.cur_master_num = blmsParam.max_master_num;
        }
    }

    blms_pconn->connState = CONN_STATUS_COMPLETE;
    blms_pconn->irq_event1_union.connect_evt = 1;//CallBack process later in mainLoop
    aclConn_param.connSync |= (1<<blms_conn_sel);

    blms_pconn->conn_inst = pSnifferMstSeek->sync.sync_conn_inst + pSnifferMstSeek->jump_num_valid;
    blms_pconn->conn_successive_miss = 0;
    blms_pconn->tx_wptr = blms_pconn->tx_rptr = 0;

    /* clear in connect start and connect end, for more secure */
    blms_pconn->conn_update_union.update_pack = 0; //clear: update_cmd & update_param & update_map & update_phy

    /* can not clear terminate_reason, main_loop event callback need use, if new connect too quick, clearing will lead to reason lost */
    blms_pconn->conn_termin_union.peer_terminate = blms_pconn->conn_termin_union.local_terminate = 0; //clear local terminate/peer terminate

    blms_pconn->conn_established_tick = 0;
    blms_pconn->conn_complete_tick = clock_time() | 1;

    blms_pconn->inter_jump_num = 0;
    blms_pconn->connUpt_inst_jump = 0;

    #if ((MCU_CORE_TYPE == MCU_CORE_825x) && (FIX_HW_CRC24_EN) )
        extern u32 reverse_32bit(volatile u32 x);
        blms_pconn->conn_crc_revert = (reverse_32bit(blms_pconn->aclCrcInit) >> 8) & 0xffffff;
    #endif

    bltPHYs.cur_llPhy = pSnifferMstSeek->sync.sync_CI_phy & 0x0F;
    bltPHYs.cur_own_CI = (pSnifferMstSeek->sync.sync_CI_phy & 0xF0 >> 4);
#if (LL_FEATURE_ENABLE_LE_2M_PHY | LL_FEATURE_ENABLE_LE_CODED_PHY)
    blt_cfg_conn_phy_param(&blms_pconn->connPhyCtrl, bltPHYs.cur_llPhy, bltPHYs.cur_own_CI); //Reset conn_cur_phy and conn_cur_CI to the dft settings.
#endif

    blt_ll_setSchedulerTaskPriority( TSKOFT_ACL_CONN + blms_conn_sel, TASK_PRIORITY_CONN_CREATE );

    #if (BLMS_PM_ENABLE)
        blms_pconn->pm_error_us = 0;
    #endif
    ///////////////////////////// blms_connect_common End /////////////////////////////


    blms_pconn->bSlot_interval = pSnifferMstSeek->sync.sync_intvl_n_1m25<<1;
    blm_pconn->sSlot_interval = BSLOT_DUR_2_SSLOT_DUR(blms_pconn->bSlot_interval);  //*32

    blms_pconn->conn_tick = clock_time();
    blm_pconn->connExpectTime = aclSniffer_mst_param.sniffer_seek_tick_1st_rx;

    int tolerance_us = 0;
    #if (BLMS_PM_ENABLE)
        /***********************************************************************
                         tolerance_max_us       shift_margin
        6  ->  7.5   mS:    2   mS            5 bSlot: 3125 uS
        7  ->  8.75  mS:    2.5 mS            6 bSlot: 3750 uS
        8  -> 10      mS:       3   mS            7 bSlot: 4375 uS
        9  -> 11.25  mS:        3.5 mS            8 bSlot: 5000 uS
        10  -> 12.5   mS:       4   mS            8 bSlot: 5000 uS
        *************************************************************************/
        if(pSnifferMstSeek->sync.sync_intvl_n_1m25 < 10){     // < 12.5mS
            blm_pconn->tolerance_max_us = 500*(pSnifferMstSeek->sync.sync_intvl_n_1m25 - 2);
            blms_pconn->bSlot_shift_margin = pSnifferMstSeek->sync.sync_intvl_n_1m25 - 1;
        }
        else{
            blm_pconn->tolerance_max_us = 4000;
            blms_pconn->bSlot_shift_margin = 8;
        }

        blm_pconn->latency_available = 0;
        blm_pconn->latency_wakeup_flg = 0;
        blm_pconn->sleep_32k_rc = 0;

        tolerance_us = 2000 + aclSniffer_mst_param.sniffer_sync_earlyTime_add;
        u32 conn_interval_us = blms_pconn->conn_intvl_tick / SYSTEM_TIMER_TICK_1US;
        u32 tolerance_max_us = conn_interval_us * 9 / 20;

        if(tolerance_us > tolerance_max_us){
            tolerance_us = tolerance_max_us;
        }

        if(tolerance_us > 10000){
            tolerance_us = 10000;
        }

        #if (0)
            blmsPm.slave_no_sleep |= (1<<blms_conn_sel);
        #endif
    #endif

    blm_pconn->conn_tolerance_us = tolerance_us;
    blm_pconn->conn_start_time = blm_pconn->connExpectTime - BRX_LEFT_EARLY_TICK - blm_pconn->conn_tolerance_us*SYSTEM_TIMER_TICK_1US;

    u16 jump_num = 0;
    if(blm_pconn->conn_start_time < bltSche.sSlot_tick_irq_real){
        jump_num = (bltSche.sSlot_tick_irq_real - blm_pconn->conn_start_time) / blms_pconn->conn_intvl_tick + 1;
        blm_pconn->conn_start_time += blms_pconn->conn_intvl_tick*jump_num;
        blm_pconn->connExpectTime += blms_pconn->conn_intvl_tick*jump_num;
        blms_pconn->conn_inst += jump_num;
        blms_pconn->chn_idx = ((u32)(blms_pconn->chn_idx + jump_num))%37;
    }
    blms_pconn->acl_sniffer_establish_inst = blms_pconn->conn_inst;

    int n_sSlot = TICKS_DUR_2_SSLOT_DUR(blm_pconn->conn_start_time - bltSche.sSlot_tick_irq_real);
    blm_pconn->sSlot_mark_conn = bltSche.sSlot_idx_irq_real + n_sSlot - blm_pconn->sSlot_interval;
    blms_pconn->bSlot_mark_conn = bltSche.bSlot_idx_start + SSLOT_DUR_2_BSLOT_DUR(blm_pconn->sSlot_mark_conn);

    #if (LL_FEATURE_ENABLE_LE_2M_PHY | LL_FEATURE_ENABLE_LE_CODED_PHY)
        blms_pconn->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + pdu_27b_tifs_27b_sslot[blms_pconn->connPhyCtrl.conn_cur_phy - 1][1] + (2*blm_pconn->conn_tolerance_us*SSLOT_US_REVERSE);
    #else
        blms_pconn->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + PAYLOAD_27B_TIFS_27B_ENCRT_1MPHY_SSLOT_NUM + (2*blm_pconn->conn_tolerance_us*SSLOT_US_REVERSE);
    #endif

    /* here use scheduler process 15 small slot to simplify code, it's OK */
    blms_pconn->sSlot_sche_use = 15;   //give 19.5*15=292 uS
    blms_pconn->sSlot_duration = blms_pconn->sSlot_allocNum + blms_pconn->sSlot_sche_use;

    blm_pconn->sSlot_offset = 0; //clear when connect, no not clear when terminate

    blms_pconn->sync_timing = SLAVE_SYNC_CONN_CREATE;
    blms_pconn->acl_sniffer_sync_creating = 1;

    blt_sche_addUpdate(SLOT_UPDT_SLAVE_CONN_CREATE);
    blt_sche_addTaskMask(TSKMSK_ACL_CONN_0<<blms_conn_sel);

    return BLE_SUCCESS;
}

void blc_ll_addAclSnifferMstSyncEarlyTime(u32 earlyTime_us)
{
    aclSniffer_mst_param.sniffer_sync_earlyTime_add = earlyTime_us;
}


ble_sts_t blc_ll_setAclSnifferMstReportRssiType(acl_sniffer_rssi_report_type_t rssi_type)
{
    if(rssi_type < RSSI_TYPE_SLAVE || rssi_type == RSSI_TYPE_MASTER || rssi_type > RSSI_TYPE_ALL)
    {
        return LL_ERR_INVALID_PARAMETER;
    }

    aclSniffer_mst_param.sniffer_rssi_reportType = rssi_type;

    return BLE_SUCCESS;
}


void blc_ll_setAclSnifferMst1stSyncWinMaxEnable(u8 enable)
{
    aclSniffer_mst_param.sniffer_1st_rxWindowMaxFlag = enable;
}


int blc_ll_getAclSnifferMstSyncNumber(void)
{
    return blmsParam.cur_master_num;
}


int blc_ll_getAclSnifferMstSyncStatus(u16 snifHandle)
{
    u8 idx = snifHandle & CONN_IDX_MASK;
    
    if(idx >= LL_MAX_ACL_CEN_NUM){
        return SNIFFER_UNKNOWN_SNIFHANDLE;
    }
    
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[idx];

    acl_sniffer_seek_param_t *cur_SnifferSeek = (acl_sniffer_seek_param_t *)&aclSniffer_mst_seek[idx];

    int status = SNIFFER_STATUS_STANDBY;

    if(pc->acl_sniffer_sync_creating){
        status = SNIFFER_STATUS_CREATING;
    }
    else if(pc->connState == CONN_STATUS_COMPLETE){
        status = SNIFFER_STATUS_CREATING;
    }
    else if(pc->connState == CONN_STATUS_ESTABLISH){
        status = SNIFFER_STATUS_ESTABLISH;
    }
    else if(cur_SnifferSeek->seeking ){
        status = SNIFFER_STATUS_SEEK;
    }

    return status;
}


int blc_ll_setAclSnifferMstTerminateSync(u16 snifHandle)
{
    u8 pc_conn_sel = snifHandle & CONN_IDX_MASK;
    st_ll_conn_t *pc_conn =  (st_ll_conn_t *)&blms[pc_conn_sel];

    if(pc_conn_sel >= LL_MAX_ACL_CEN_NUM){
        return SNIFFER_UNKNOWN_SNIFHANDLE;
    }

    acl_sniffer_seek_param_t *cur_SnifferSeek = (acl_sniffer_seek_param_t *)&aclSniffer_mst_seek[pc_conn_sel];

    u32 r = irq_disable();

    //stop sniffer
    if(pc_conn->connState && !cur_SnifferSeek->seeking){
        pc_conn->conn_termin_union.peer_terminate = HCI_ERR_REMOTE_USER_TERM_CONN;
    }
    else if(!pc_conn->connState && cur_SnifferSeek->seeking){
        cur_SnifferSeek->seek_stop = 1;
    }
    else if(pc_conn->connState && cur_SnifferSeek->seeking){
        pc_conn->conn_termin_union.peer_terminate = HCI_ERR_REMOTE_USER_TERM_CONN;
        cur_SnifferSeek->seek_stop = 2;
    }
    else{
        irq_restore(r);
        return SNIFFER_USER_STOP_NOT_SUPPORTED;
    }

    irq_restore(r);
    return BLE_SUCCESS;
}


_attribute_no_inline_
void  blt_ll_acl_sniffer_mst_mainloop(void)
{
    blt_ll_acl_sniffer_mainloop();

    for(int snif_idx = 0; snif_idx < LL_MAX_ACL_CEN_NUM; snif_idx++)
    {
            acl_sniffer_seek_param_t *cur_SnifferSeek = (acl_sniffer_seek_param_t *)&aclSniffer_mst_seek[snif_idx];

            u32 seek_timeout_us = aclSniffer_mst_param.sniffer_seek_timeout_ms;
            seek_timeout_us *= 1000;
            if(cur_SnifferSeek->tick_seek_start && clock_time_exceed(cur_SnifferSeek->tick_seek_start, seek_timeout_us)){
                //seek state timeout
                u32 r = irq_disable();
                if(blms_state != BLMS_STATE_SNIFM_SEEK_S){
                    cur_SnifferSeek->tick_seek_start = 0;
                    if(cur_SnifferSeek->seeking){
                        cur_SnifferSeek->seeking = 0;
                        cur_SnifferSeek->seek_stop = 0;
                        cur_SnifferSeek->seek_state = 2;//Fail

                        blt_sche_removeTaskMask(TSKMSK_SNIFM_SEEK_0<<snif_idx);
                        if(!bltSche.task_mask){
                            blmsParam.sche_run_flag = 0;
                        }
                    }
                }
                irq_restore(r);
            }

            if(cur_SnifferSeek->seek_state == 2){
                cur_SnifferSeek->seek_state = 0;

                u8 reason[1];
                reason[0] = SNIFFER_SEEK_FAIL;
                blt_ll_acl_sniffer_sync_result(cur_SnifferSeek->sync.sync_connHandle, reason);
            }
            else if(cur_SnifferSeek->seek_state == 3){
                //report stop event
                cur_SnifferSeek->seek_state = 0;

                u8 reason[1];
                reason[0] = SNIFFER_USER_STOP_EFFECTIVE;
                blt_ll_acl_sniffer_sync_result(cur_SnifferSeek->sync.sync_connHandle, reason);
            }
            else if(cur_SnifferSeek->seek_state == 4){
                //not report stop event
                cur_SnifferSeek->seek_state = 0;
            }
    }
}


void  blt_ll_reset_acl_sniffer_mst(void)
{
    /*pay attention: all parameters which clear in connection terminate procedure should be considered if need clear in HCI reset callback function*/
    for(int snif_idx = 0; snif_idx < LL_MAX_ACL_CEN_NUM; snif_idx++)
    {
        st_ll_conn_t *pc_conn = (st_ll_conn_t*)&blms[snif_idx];

        pc_conn->acl_sniffer_sync_creating = 0;
        pc_conn->acl_sniffer_sync_update_ignore = 0;

        acl_sniffer_seek_param_t *cur_SnifferSeek = (acl_sniffer_seek_param_t *)&aclSniffer_mst_seek[snif_idx];
        cur_SnifferSeek->seek_state = 0;
        cur_SnifferSeek->seeking = 0;
        cur_SnifferSeek->seek_stop = 0;
        cur_SnifferSeek->tick_seek_start = 0;
    }
}


_attribute_noinline_
int blt_acl_sniffer_mst_mainloop_task(int flag)
{
    if(flag == FLAG_MODULE_MAINLOOP){
        blt_ll_acl_sniffer_mst_mainloop();
    }
    else if(flag == FLAG_MODULE_RESET){
        blt_ll_reset_acl_sniffer_mst();
    }

    return BLE_SUCCESS;
}


_attribute_noinline_
void blc_ll_initAclSnifferMst_module(void)
{
    ll_acl_sniffer_mst_irq_task_cb = blt_acl_sniffer_mst_irq_task;
    ll_acl_sniffer_mst_mlp_task_cb = blt_acl_sniffer_mst_mainloop_task;

    aclSniffer_mst_param.sniffer_rssi_reportType = RSSI_TYPE_SLAVE;

    sniffer_rx_null_fifo.wptr = sniffer_rx_null_fifo.rptr = 0;

    aclSniffer_mst_param.sniffer_seek_timeout_ms = 10000;//10 second
    aclSniffer_mst_param.sniffer_seek_halfWindow_ms = 20;// +-20ms
    aclSniffer_mst_param.sniffer_seek_count_max = 3;

    for(int i=0; i<TSKNUM_SNIFM_SEEK; i++){
        acl_sniffer_seek_param_t *cur_SnifferSeek = (acl_sniffer_seek_param_t *)&aclSniffer_mst_seek[i];

        cur_SnifferSeek->snifferTsk_fifo.scheTask_oft = TSKOFT_SNIFM_SEEK + i;
        cur_SnifferSeek->snifferTsk_fifo.scheTask_idx = i;
        cur_SnifferSeek->snifferTsk_fifo.scheTask_flg = TSKFLG_SNIFM_SEEK;

        //cur_SnifferSeek->tick_seek_tolerance_ms = 20; // +-20ms
        cur_SnifferSeek->tick_seek_tolerance_max_ms = RSSI_SNIFFER_SEEK_TOLERANCE_MAX_MS; // +-35ms
    }
}


// only for master
ble_sts_t blc_ll_getAclMasterConnectionTimingParameter(u16 connHandle, u8* aclMasterParam)
{
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[conn_idx];

    if(conn_idx >= LL_MAX_ACL_CEN_NUM)
    {
        return LL_ERR_INVALID_PARAMETER;
    }

    if(blt_ll_isAclhdlInvalid(pc->acl_conHandle)){
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    if(!pc->conn_established_tick){
        return LL_ERR_CONNECTION_NOT_ESTABLISH;
    }

    st_llm_conn_t  *pm = (st_llm_conn_t *) &blmsMaster[conn_idx];
    acl_sniffer_sync_param_t* param = (acl_sniffer_sync_param_t*)aclMasterParam;

    u32 r = irq_disable();

    param->sync_connHandle = connHandle;
    param->sync_CI_phy = pc->connPhyCtrl.conn_cur_phy | (pc->connPhyCtrl.conn_cur_CI << 4);
    param->sync_conn_chnSel = pc->conn_chnsel;

    param->sync_expectTime = pm->tick_conn_expect;

    param->sync_conn_inst = pc->conn_inst;
    param->sync_intvl_n_1m25 = pc->conn_intvl_n_1m25;

    param->sync_accessAddr = pc->aclAccessAddr;
    param->sync_crcInit = pc->aclCrcInit;
    param->sync_conn_timeout = pc->conn_timeout;

    smemcpy(param->sync_conn_chm, pc->acl_chnParam.chmTbl, 5);
    param->sync_conn_hop = pc->conn_chn_hop;
    param->sync_chn_idx = pc->chn_idx;
    param->sync_conn_num_config = CONN_MAX_NUM_CONFIG;

    irq_restore(r);

    return BLE_SUCCESS;
}


ble_sts_t blc_ll_setAclMasterConnParamUpdateRspLatency(u16 updateRsp_latency)
{
    if(updateRsp_latency > 100){
        return LL_ERR_INVALID_PARAMETER;
    }

    acl_mst_connParamUpdateRsp_latency_max = updateRsp_latency;

    return BLE_SUCCESS;
}

#endif  //end of LL_RSSI_SNIFFER_MASTER_ENABLE

