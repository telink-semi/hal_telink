/********************************************************************************************************
 * @file    acl_sniffer_slv.c
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

#include "acl_sniffer_slv.h"


#if (LL_RSSI_SNIFFER_SLAVE_ENABLE)




_attribute_ble_data_retention_  _attribute_aligned_(4) volatile acl_sniffer_param_t aclSniffer_slv_param;
_attribute_ble_data_retention_  u8 acl_sniffer_slv_sync_info[sizeof(acl_sniffer_sync_param_t)];
_attribute_ble_data_retention_  u8 acl_sniffer_sync_slv_backup_info[LL_MAX_ACL_PER_NUM][sizeof(acl_sniffer_sync_param_t)];//for compatible mode

_attribute_ble_data_retention_  _attribute_aligned_(4) volatile acl_sniffer_seek_param_t aclSniffer_slv_seek[TSKNUM_SNIFS_SEEK];
_attribute_ble_data_retention_  acl_sniffer_seek_param_t *pSnifferSlvSeek = NULL;


#if (!SNIFFER_USE_SOME_COMMON_APIS)
_attribute_ram_code_ int irq_acl_sniffer_slv_rx(void)
{
    u8 * raw_pkt = (u8 *) (blt_rxfifo.p_base + (blt_rxfifo.wptr++ & blt_rxfifo.mask) * blt_rxfifo.size);
    u8 * new_pkt = (u8 *) (blt_rxfifo.p_base + (blt_rxfifo.wptr & blt_rxfifo.mask) * blt_rxfifo.size);

    aclConn_param.acl_rx_dma_buff = (u32)new_pkt; //Update the next acl dma rx buffer
    ble_rf_set_rx_dma((u8*)aclConn_param.acl_rx_dma_buff, aclConn_param.acl_rx_dma_size);

    HAL_CLEAR_RF_RX_IRQ;

    u8 drop_rx_data = 0;
    u8 next_buffer = 0;
    static u8 rfLen_1st_rx;
    static u8 rssi_1st_rx;

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

        if(blms_state  ==  BLMS_STATE_SNIFS_S){
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
                if(!aclSniffer_slv_param.sniffer_rx_num){
                    if(blms_state == BLMS_STATE_SNIFS_S){
                        #if (LL_ACL_PER_EN)
                            if(!bls_pconn->tick_1st_rx){
                                if(blms_pconn->acl_sniffer_sync_creating){
                                    //need to continue monitor the peer-slave
                                    bls_pconn->tick_1st_rx = bltRxPkt.rx_header_tick;
                                    rfLen_1st_rx = raw_pkt[DMA_RFRX_OFFSET_RFLEN];
                                    rssi_1st_rx = raw_pkt[DMA_RFRX_OFFSET_RSSI(raw_pkt)];

                                    DBG_CHN3_TOGGLE;
                                    DBG_CHN3_TOGGLE;
                                }
                                else{
                                    u32 diff;
                                    if(bltRxPkt.rx_header_tick > bls_pconn->expectTimeMark){
                                        diff = bltRxPkt.rx_header_tick - bls_pconn->expectTimeMark;
                                    }
                                    else{
                                        diff = bls_pconn->expectTimeMark - bltRxPkt.rx_header_tick;
                                    }
                                    //printf("us:%d,%d,%d\n", bltRxPkt.rx_header_tick>>4, bls_pconn->expectTimeMark>>4,diff>>4);

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
                                        bls_pconn->tick_1st_rx = bltRxPkt.rx_header_tick;

                                        raw_pkt[1] = BLT_ACL_SNIFFER_MASTER_FLAG;//peer master RSSI
                                        raw_pkt[1] |= blms_pconn->conn_chn;
                                        raw_pkt[2] = blms_pconn->acl_conHandle;
                                        raw_pkt[3] = raw_pkt[DMA_RFRX_OFFSET_RSSI(raw_pkt)];
                                        #if (DEBUG_SNIFFER_REPORT_INSTANT_EN)
                                            raw_pkt[4] = blms_pconn->conn_inst;
                                        #endif
                                        aclSniffer_slv_param.sniffer_rssi_validFlag = 1;

                                        next_buffer = 1;

                                        DBG_CHN3_TOGGLE;
                                        DBG_CHN3_TOGGLE;
                                    }

                                    blt_sniffer_stop_rx_window(50);
                                }
                            }
                        #endif
                    }
                }
                else if(aclSniffer_slv_param.sniffer_rx_num == 1)
                {
                    if(bls_pconn->tick_1st_rx && blms_pconn->acl_sniffer_sync_creating){
                        u32 diff = bltRxPkt.rx_header_tick - bls_pconn->tick_1st_rx;

                        //e.g.: For 1M: 10 Byte = 1B(preamble) + 4B(accesscode) + 2B(header) + 3B(CRC), 150 is T_IFS

                        //remote:  |1st_timing|<---T_IFS--->|2st_timing|
                        //local:   |rx_head_tick + 1st_timing + T_IFS
                        u32 diff_ideal = (blt_phy_getRfPacketTime_us(rfLen_1st_rx, bltPHYs.cur_llPhy, bltPHYs.cur_peer_CI) + BLE_T_IFS) * SYSTEM_TIMER_TICK_1US;

                        // T_IFS within 20us
                        if((diff > (diff_ideal - 20 * SYSTEM_TIMER_TICK_1US)) && (diff < (diff_ideal + 20 * SYSTEM_TIMER_TICK_1US))){
                            //monitor the peer-slave successful
                            blms_pconn->acl_sniffer_sync_creating = 0;

                            //record first peer master RSSI
                            raw_pkt[1] = BLT_ACL_SNIFFER_MASTER_FLAG;//peer master RSSI
                            raw_pkt[1] |= blms_pconn->conn_chn;
                            raw_pkt[2] = blms_pconn->acl_conHandle;
                            raw_pkt[3] = rssi_1st_rx;
                            #if (DEBUG_SNIFFER_REPORT_INSTANT_EN)
                                raw_pkt[4] = blms_pconn->conn_inst;
                            #endif
                            aclSniffer_slv_param.sniffer_rssi_validFlag = 1;

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

        aclSniffer_slv_param.sniffer_rx_num ++;  //care CRC
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


_attribute_ram_code_ int blt_sniffer_slv_start(int conn_idx)
{
    blms_start_pre_process(conn_idx);

    bls_conn_sel = blms_conn_sel - LL_MAX_ACL_CEN_NUM; //pay attention here
    bls_pconn =  (st_lls_conn_t *)&blmsSlave[bls_conn_sel];

    blms_start_common_1(blms_pconn);
    aclSniffer_slv_param.sniffer_rx_num = 0;

    rf_set_rxmode();

    if( aclConn_param.connSync & (1<<blms_conn_sel) ){
        rf_set_1st_rx_timeout(0xffffff);
    }
    else{
        rf_set_1st_rx_timeout(300 + bls_pconn->conn_tolerance_us*2 + bltPHYs.prmb_ac_us);
    }

    rf_ble_set_rx_settle(RX_SETTLE_US);

    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_RX_ON);  }

    //these logic setting executing after RX setting to save time
    blms_state = BLMS_STATE_SNIFS_S;
    systick_irq_trigger = SYS_IRQ_TRIG_BRX_POST;  //will set reg_system_tick_irq for task_post immediately
    bls_pconn->timing_update = 0;
    bls_pconn->tick_1st_rx = 0;
    bls_pconn->sSlot_mark_conn = bltSche.sSlot_idx_irq_real;
    bls_pconn->sSlot_shift_tor = bls_pconn->conn_tolerance_us*SSLOT_US_REVERSE;
    bls_pconn->expectTimeMark = bls_pconn->connExpectTime = bltSche.sSlot_tick_irq + BRX_LEFT_EARLY_TICK + bls_pconn->conn_tolerance_us * SYSTEM_TIMER_TICK_1US;
    #if (BLMS_PM_ENABLE)
        if(blmsPm.slave_idx_calib == 0xFF){
            blmsPm.slave_idx_calib = bls_conn_sel;
        }
    #endif

    aclSniffer_slv_param.sniffer_rssi_validFlag = 0;

    blms_start_common_2(blms_pconn);

    return BLE_SUCCESS;
}


_attribute_ram_code_ int blt_sniffer_slv_post(void)
{
    if(blmsParam.rf_fsm_busy) {
        blmsParam.rf_fsm_busy = 0;

        rf_set_tx_rx_off();
        STOP_RF_STATE_MACHINE;  //stop state machine
        CLEAR_ALL_RFIRQ_STATUS;
    }

    if(blms_pconn->acl_sniffer_sync_creating){
        // clear tick_1st_rx
        bls_pconn->tick_1st_rx = 0;
    }

    int brx_sync = blms_pconn->sync_timing;

    blms_state = BLMS_STATE_SNIFS_E;
    if ( blms_post_common_1(blms_pconn) ){  // return 1: terminate happens

        blmsParam.cur_slave_num --;
        blms_pconn->acl_sniffer_sync_creating = 0;
        blms_pconn->acl_sniffer_sync_update_ignore = 0;

        blt_sche_removeTaskMask(TSKMSK_ACL_CONN_0<<blms_conn_sel);  //pay attention here
        blt_sche_addUpdate(SLOT_UPDT_CONN_TERMINATE);   //triggers "bltSlot.update" valid

        #if (BLMS_PM_ENABLE)
            blmsPm.slave_no_sleep &= ~(1<<bls_conn_sel);
            if(blmsPm.slave_idx_calib == bls_conn_sel){
                blmsPm.slave_idx_calib = 0xFF;
            }
        #endif

        #if (0)//(BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE)
            blms_pconn->conn_pkt_dec_pending = 0;
            aclConn_param.updateCmd_pending &= ~BIT(bls_conn_sel);
        #endif

            blt_debug_gpio_toggle_acl_sniffer();
            blt_debug_gpio_toggle_acl_sniffer();
            blt_debug_gpio_toggle_acl_sniffer();
            blt_debug_gpio_toggle_acl_sniffer();
    }
    else{   //no terminate
        blt_ll_acl_conn_sync_process(bls_pconn->tick_1st_rx);

        /* value below are invalid when old conn_interval change to new conn_interval */
        //bls_pconn->connExpectTime = (bls_pconn->tick_1st_rx ? bls_pconn->tick_1st_rx : blms_pconn->conn_tick_mark) + blms_pconn->conn_intvl_tick;
        if(bls_pconn->tick_1st_rx){
            bls_pconn->expectTimeMark = bls_pconn->tick_1st_rx;
            bls_pconn->conn_offset_tick = bls_pconn->connExpectTime - bls_pconn->tick_1st_rx;
            bls_pconn->connExpectTime = bls_pconn->tick_1st_rx + blms_pconn->conn_intvl_tick;

            blt_debug_gpio_toggle_acl_sniffer();
            blt_debug_gpio_toggle_acl_sniffer();
        }
        else{
            bls_pconn->connExpectTime += blms_pconn->conn_intvl_tick;
        }


        /* update sync status according to RX packet receiving result */
        if(bls_pconn->tick_1st_rx)
        {
            if(blms_pconn->sync_timing)
            {
                blms_pconn->sync_timing = 0;
                bls_pconn->conn_tolerance_us = blmsParam.min_tolerance_us;
                bls_pconn->conn_start_time = bls_pconn->connExpectTime - BRX_LEFT_EARLY_TICK - bls_pconn->conn_tolerance_us * SYSTEM_TIMER_TICK_1US;
                int n_sSlot = TICKS_DUR_2_SSLOT_DUR(bls_pconn->conn_start_time - bltSche.sSlot_tick_irq_real);
                bls_pconn->sSlot_mark_conn = bltSche.sSlot_idx_irq_real + n_sSlot - bls_pconn->sSlot_interval;

                bls_pconn->sSlot_offset = 0;
                bls_pconn->timing_update = 1;
                blt_sche_addUpdate(SLOT_UPDT_SLAVE_SYNC_DONE);

                #if (BLMS_PM_ENABLE)
                    blmsPm.slave_no_sleep &= ~(1<<bls_conn_sel);
                #endif
            }
            else{
                if(bls_pconn->conn_tolerance_us > blmsParam.min_tolerance_us){
                    bls_pconn->conn_tolerance_us = blmsParam.min_tolerance_us;
                    bls_pconn->conn_start_time = bls_pconn->connExpectTime - BRX_LEFT_EARLY_TICK - bls_pconn->conn_tolerance_us * SYSTEM_TIMER_TICK_1US;
                    int n_sSlot = TICKS_DUR_2_SSLOT_DUR(bls_pconn->conn_start_time - bltSche.sSlot_tick_irq_real);
                    bls_pconn->sSlot_mark_conn = bltSche.sSlot_idx_irq_real + n_sSlot - bls_pconn->sSlot_interval;

                    bls_pconn->sSlot_offset = 0;
                    bls_pconn->timing_update = 1;
                    blt_sche_addUpdate(SLOT_UPDT_SLAVE_SYNC_DONE);
                }
                else{
                    u32 tick_offset_1st_rx = bls_pconn->tick_1st_rx - bltSche.sSlot_tick_irq_real;
                    u32 tick_offset_expect = blmsParam.min_tolerance_us * SYSTEM_TIMER_TICK_1US + BRX_LEFT_EARLY_TICK;
                    bls_pconn->sSlot_offset = TICKS_DUR_2_SSLOT_DUR(tick_offset_1st_rx - tick_offset_expect);

                    #if (BRX_HALF_MARGIN_SSLOT_NUM == 3)
                        if(bls_pconn->sSlot_offset < -1 || bls_pconn->sSlot_offset > 2)
                    #elif (BRX_HALF_MARGIN_SSLOT_NUM == 2)
                        if(bls_pconn->sSlot_offset < 0 || bls_pconn->sSlot_offset > 1)
                    #else
                        #error "add code here"
                    #endif
                        {
                            bls_pconn->conn_tolerance_us = blmsParam.min_tolerance_us;
                            bls_pconn->timing_update = 1;
                            blt_sche_addUpdate(SLOT_UPDT_SLAVE_SSLOT_ADJUST);
                        }
                }
            }

            if(bls_pconn->timing_update){
                bls_pconn->sSlot_shift_tor = blmsParam.min_tolerance_us*SSLOT_US_REVERSE;
                /* tolerance*2/19.53 = tolerances/10 */
                #if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
                    blms_pconn->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + pdu_27b_tifs_27b_sslot[blms_pconn->connPhyCtrl.conn_cur_phy - 1][blms_pconn->crypt.enable] + bls_pconn->conn_tolerance_us/10;
                #else
                    blms_pconn->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + pdu_27b_tifs_27b_sslot[0][blms_pconn->crypt.enable] + bls_pconn->conn_tolerance_us/10;
                #endif
            }
        }
        else
        {
        #if (0)
            if(blms_pconn->sync_timing == SLAVE_SYNC_CONN_UPDATE && blms_pconn->sync_num >= BLMS_CONN_UPDATE_BRX_MAX_TRY_NUM){
                blms_pconn->sync_timing = SLAVE_SYNC_CONN_UPT_FAIL;
                //aclConn_param.connSync &= ~(1<<blms_conn_sel);  //fix: can not reset here, slave still have chance sync, duration shorter
                blms_pconn->conn_update_union.update_mark &= ~CONN_UPDATE_PARAM_MASK;
                blt_sche_addUpdate(SLOT_UPDT_SLAVE_CONNUPDATE_FAIL);
                blt_ll_setSchedulerTaskPriority(TSKOFT_ACL_CONN + blms_conn_sel, TASK_PRIORITY_LOW);

                /* 1.25mS/2.5mS: RX window not change, cover whole winSize
                 * others: RX begin at middle window - 1.25mS, RX end at  middle window + 0.625mS, default duration 1.875mS
                 * consider conn_window extend, RX end and duration will be better */
                if(blms_pconn->conn_winsize_next > 2){
                    blms_pconn->bSlot_mark_conn += (blms_pconn->conn_winsize_next - 2);
                    bls_pconn->sSlot_mark_conn += (blms_pconn->conn_winsize_next*32 - 2*32);
                    blms_pconn->sSlot_allocNum = 3*32;  //1.875mS
                }

                #if (BLMS_PM_ENABLE)
                    blmsPm.slave_no_sleep &= ~(1<<bls_conn_sel);
                #endif
            }
        #endif
        }

        #if (BLMS_PM_ENABLE)
            if(brx_sync){
                blt_brx_timing_init();
            }
            else{
                blt_brx_timing_update ();
            }

            bls_pconn->slave_sleep_flg = 0;
            bls_pconn->tick_last_1st_rx = bls_pconn->tick_1st_rx;
        #endif

        if(aclSniffer_slv_param.sniffer_rssi_reportType == RSSI_TYPE_ALL){
            if(!aclSniffer_slv_param.sniffer_rssi_validFlag){
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

_attribute_ram_code_ int blt_acl_sniffer_slv_irq_task(int flag)
{
    int idx = flag & FLAG_SCHEDULE_TASK_IDX_MASK;

    if(flag & FLAG_IRQ_RX){
        #if (SNIFFER_USE_SOME_COMMON_APIS)
            irq_acl_sniffer_rx();
        #else
            irq_acl_sniffer_slv_rx();
        #endif
    }
    else if(flag & FLAG_SCHEDULE_START){
        #if (SNIFFER_USE_SOME_COMMON_APIS)
            blt_sniffer_start(idx);
        #else
            blt_sniffer_slv_start(idx);
        #endif
    }
    else if(flag & FLAG_SCHEDULE_DONE){
        #if (SNIFFER_USE_SOME_COMMON_APIS)
            blt_sniffer_post();
        #else
            blt_sniffer_slv_post();
        #endif
    }
    else if(flag & FLAG_ACL_SNIFFER_SEEK_RX){
        #if (SNIFFER_USE_SOME_COMMON_APIS)
            irq_acl_sniffer_seek_rx();
        #else
            irq_acl_sniffer_slv_seek_rx();
        #endif
    }
    else if(flag & FLAG_ACL_SNIFFER_SEEK_START){
        #if (SNIFFER_USE_SOME_COMMON_APIS)
            blt_sniffer_seek_start(idx, ACL_ROLE_PERIPHERAL);
        #else
            blt_sniffer_slv_seek_start(idx);
        #endif
    }
    else if(flag & FLAG_ACL_SNIFFER_SEEK_POST){
        #if(SNIFFER_USE_SOME_COMMON_APIS)
            blt_sniffer_seek_post(ACL_ROLE_PERIPHERAL);
        #else
            blt_sniffer_slv_seek_post();
        #endif
    }
    else if(flag & FLAG_ACL_SNIFFER_SEEK){
        if(blmsParam.state_chng & STATE_CHANGE_ACL_SNIFS){
            #if (SNIFFER_USE_SOME_COMMON_APIS)
                blt_ll_sniffer_seek_anchor(acl_sniffer_slv_sync_info);
            #else
                blt_ll_sniffer_slv_seek_anchor(acl_sniffer_slv_sync_info);
            #endif
        }
    }
    else if(flag & FLAG_SCHEDULE_BUILD){
        #if (SNIFFER_USE_SOME_COMMON_APIS)
            blt_ll_buildAclSnifferSeekSchLinklist(ACL_ROLE_PERIPHERAL);
        #else
            blt_ll_buildAclSnifferSlvSeekSchLinklist();
        #endif
    }

    return BLE_SUCCESS;
}


void blc_ll_setAclSnifferSlvSeekTimeout(u16 timeout_ms)
{
    if(timeout_ms < 300){
        timeout_ms = 300;
    }

    aclSniffer_slv_param.sniffer_seek_timeout_ms = timeout_ms;
}


void blc_ll_setAclSnifferSlvSeekHalfWindow(u8 halfWindow_ms)
{
    if(halfWindow_ms < 5){
        halfWindow_ms = 5;
    }
    else if(halfWindow_ms > RSSI_SNIFFER_SEEK_TOLERANCE_MAX_MS){
        halfWindow_ms = RSSI_SNIFFER_SEEK_TOLERANCE_MAX_MS;
    }

    aclSniffer_slv_param.sniffer_seek_halfWindow_ms = halfWindow_ms;
}


void blc_ll_setAclSnifferSlvSeekCount(u8 seekCount)
{
    if(seekCount < 1){
        seekCount = 1;
    }
    else if(seekCount > 10){
        seekCount = 10;
    }

    aclSniffer_slv_param.sniffer_seek_count_max = seekCount;
}


int blc_ll_updateAclSnifferSlvSync(u8 *cmd)
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

    #if (1)//move from blt_ll_sniffer_slv_seek_anchor()
        u8 pc_conn_sel = param->sync_connHandle & CONN_IDX_MASK;

        if(pc_conn_sel < LL_MAX_ACL_CEN_NUM || pc_conn_sel >= LL_MAX_ACL_CONN_NUM){
            return SNIFFER_UNKNOWN_SNIFHANDLE;
        }

        st_ll_conn_t *pc_conn =  (st_ll_conn_t *)&blms[pc_conn_sel];
        u8 ps_conn_sel = pc_conn_sel - LL_MAX_ACL_CEN_NUM; //pay attention here
        st_lls_conn_t *ps_conn =  (st_lls_conn_t *)&blmsSlave[ps_conn_sel];

        acl_sniffer_seek_param_t *cur_SnifferSeek = (acl_sniffer_seek_param_t *)&aclSniffer_slv_seek[ps_conn_sel];

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
            if(ps_conn->brx_pkt_miss < 4){
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

    if(blmsParam.state_chng & STATE_CHANGE_ACL_SNIFS){
        return SNIFFER_CURRENT_STATE_NOT_SUPPORTED_THIS_CMD;
    }

    cur_SnifferSeek->seek_repeat_type = 0;//not need, support re-listening during a existing connection

    smemcpy(acl_sniffer_slv_sync_info, cmd, sizeof(acl_sniffer_sync_param_t));

    blmsParam.state_chng |= STATE_CHANGE_ACL_SNIFS;

    return BLE_SUCCESS;
}


u16 blc_ll_getAclSnifferSlvHandle_v2(u8 slave_index)
{
    if(slave_index >= LL_MAX_ACL_PER_NUM){
        return 0;
    }

    return ((slave_index + LL_MAX_ACL_CEN_NUM) | BLS_CONN_HANDLE);
}


int blc_ll_updateAclSnifferSlvSync_v2(u8 *cmd, u32 latencyTime)
{
    acl_slave_compatible_param_t* param = (acl_slave_compatible_param_t*)cmd;

    //index: 0 ~ 3
    //interval: 7.5ms ~ 4s -> 6 ~ 3200
    //Timeout: 100ms ~ 32s -> 10 ~ 3200
    //hop: bit<0-5> ,range is 5 ~ 16;
    if(param->slave_index >= LL_MAX_ACL_PER_NUM \
        || param->slave_connInterval < 6 || param->slave_connInterval > 3200 \
        || param->slave_connTimeout < 10 || param->slave_connTimeout > 3200 \
        || (param->slave_connHop & 0x1f) < 5 || (param->slave_connHop & 0x1f) > 16){

        return SNIFFER_PARAMETER_INVALID;
    }

    //winOffset: 0 ~ interval
    if(param->slave_status == SLAVE_STATUS_CONNECTION_SETUP || (param->slave_status == SLAVE_STATUS_CONNECTION_UPDATE)){
        if( param->slave_winOffset > param->slave_connInterval){
            return SNIFFER_PARAMETER_INVALID;
        }
    }

    #if (LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2)
        if((param->slave_CI_ChSel_PHY & 0x08) && (!ll_chn_index_calc_cb)){
            return SNIFFER_UNSUPPORTED_FEATURE;
        }
    #endif

    #if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
        if(((param->slave_CI_ChSel_PHY & 0x03) != BLE_PHY_1M) && (!ll_phy_switch_cb)){
            return SNIFFER_UNSUPPORTED_FEATURE;
        }
    #endif

    u8 pc_conn_sel = param->slave_index + LL_MAX_ACL_CEN_NUM; //pay attention here
    st_ll_conn_t *pc_conn =  (st_ll_conn_t *)&blms[pc_conn_sel];

    u8 ps_conn_sel = param->slave_index; //pay attention here
    st_lls_conn_t *ps_conn =  (st_lls_conn_t *)&blmsSlave[ps_conn_sel];

    if(pc_conn_sel < LL_MAX_ACL_CEN_NUM || pc_conn_sel >= LL_MAX_ACL_CONN_NUM){
        return SNIFFER_UNKNOWN_SNIFHANDLE;
    }

    if((blmsParam.state_chng & STATE_CHANGE_ACL_SNIFS) && (param->slave_status != SLAVE_STATUS_CONNECTION_SETUP)){
        return SNIFFER_CURRENT_STATE_NOT_SUPPORTED_THIS_CMD;
    }

    acl_sniffer_seek_param_t *cur_SnifferSeek = (acl_sniffer_seek_param_t *)&aclSniffer_slv_seek[ps_conn_sel];

    u32 r = irq_disable();

    if((cur_SnifferSeek->seeking) && (param->slave_status != SLAVE_STATUS_CONNECTION_SETUP)){
        irq_restore(r);
        return SNIFFER_SEEK_IN_PROGRESS;
    }

    acl_sniffer_sync_param_t *cur_SnifferSync = (acl_sniffer_sync_param_t *)&acl_sniffer_slv_sync_info;

    if(param->slave_status == SLAVE_STATUS_CONNECTION_SETUP){
        cur_SnifferSync->sync_connHandle = pc_conn_sel | BLS_CONN_HANDLE;

        if(param->slave_CI_ChSel_PHY == 0){
            //the primary node is not provided. The default parameters are used
            cur_SnifferSync->sync_CI_phy = 0x81;//(LE_CODED_S8 << 4) | BLE_PHY_1M
            cur_SnifferSync->sync_conn_chnSel = 0;//CSA#1
        }
        else{
            cur_SnifferSync->sync_CI_phy = param->slave_CI_ChSel_PHY & 0xF3;
            cur_SnifferSync->sync_conn_chnSel = param->slave_CI_ChSel_PHY & 0x08 ? 1 : 0;
        }

        cur_SnifferSync->sync_expectTime = clock_time() - latencyTime + (param->slave_winOffset + 1) * SYSTEM_TIMER_TICK_1250US;

        cur_SnifferSync->sync_conn_inst = 0;//important
        cur_SnifferSync->sync_intvl_n_1m25 = param->slave_connInterval;

        smemcpy(&cur_SnifferSync->sync_accessAddr, param->slave_accessAddress, 4);
        smemcpy(&cur_SnifferSync->sync_crcInit, param->slave_crcInit, 3);
        cur_SnifferSync->sync_conn_timeout = param->slave_connTimeout * 10 * SYSTEM_TIMER_TICK_1MS;

        smemcpy(cur_SnifferSync->sync_conn_chm, param->slave_connChannelMap, 5);
        cur_SnifferSync->sync_conn_hop = param->slave_connHop;
        cur_SnifferSync->sync_chn_idx = 0;//important
        cur_SnifferSync->sync_conn_num_config = CONN_MAX_NUM_CONFIG;

        if(param->slave_instant){
            cur_SnifferSync->sync_conn_inst += param->slave_instant;
            cur_SnifferSync->sync_chn_idx = ((u32)(cur_SnifferSync->sync_chn_idx + param->slave_instant))%37;
        }

        //backup all
        smemcpy(acl_sniffer_sync_slv_backup_info[ps_conn_sel], acl_sniffer_slv_sync_info, sizeof(acl_sniffer_sync_param_t));

        pc_conn->conn_offset_next = param->slave_winOffset;//important

        if(pc_conn->connState){
            pc_conn->conn_termin_union.peer_terminate = HCI_ERR_REMOTE_USER_TERM_CONN;
        }
    }
    else if(param->slave_status == SLAVE_STATUS_STANDBY){
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
    else{
        smemcpy(acl_sniffer_slv_sync_info, acl_sniffer_sync_slv_backup_info[ps_conn_sel], sizeof(acl_sniffer_sync_param_t));
        acl_sniffer_sync_param_t *backup_SnifferSync = (acl_sniffer_sync_param_t *)&acl_sniffer_sync_slv_backup_info[ps_conn_sel];

        if(cur_SnifferSeek->seek_state != 1 && pc_conn->connState == CONN_STATUS_DISCONNECT){
            //the first time sniffer seek fail
            cur_SnifferSync->sync_expectTime = backup_SnifferSync->sync_expectTime;
            cur_SnifferSync->sync_conn_inst = 0;
            cur_SnifferSync->sync_chn_idx = 0;
        }
        else{
            u8 rollback_num = 0;
            u16 interval_jump_num = 0;
            if(!param->slave_instant){
                #if (0)
                    if((blm_btxbrx_state == 1) && (blms_state == BLMS_STATE_SNIFS_S) && (bls_conn_sel == ps_conn_sel)){
                        if((param->slave_connInterval > CONN_INTERVAL_35MS) && (pc_conn->conn_intvl_n_1m25 < CONN_INTERVAL_16P25MS)){
                            rollback_num = 1;
                        }
                        //else{
                        //  rollback_num = 0;
                        //}
                    }
                    else{
                        if(tick1_exceed_tick2(ps_conn->connExpectTime, clock_time())){
                            if((param->slave_connInterval > CONN_INTERVAL_35MS) && (pc_conn->conn_intvl_n_1m25 < CONN_INTERVAL_16P25MS)){
                                rollback_num = 2;
                            }
                            else{
                                rollback_num = 1;
                            }
                        }
                        else{
                            //Note: need to consider interval jump situation
                            interval_jump_num = ((u32)(clock_time() - ps_conn->connExpectTime))/pc_conn->conn_intvl_tick;
                        }
                    }
                #else
                    if(!((blm_btxbrx_state == 1) && (blms_state == BLMS_STATE_SNIFS_S) && (bls_conn_sel == ps_conn_sel))){
                        //not in the sniffer window
                        if(tick1_exceed_tick2(ps_conn->connExpectTime, clock_time())){
                            rollback_num = 1;
                        }
                        else{
                            //Note: need to consider interval jump situation
                            interval_jump_num = ((u32)(clock_time() - ps_conn->connExpectTime))/pc_conn->conn_intvl_tick;
                        }
                    }
                #endif

                if(!rollback_num && !interval_jump_num){
                    cur_SnifferSync->sync_expectTime = ps_conn->connExpectTime;
                    cur_SnifferSync->sync_conn_inst = pc_conn->conn_inst;
                    cur_SnifferSync->sync_chn_idx = pc_conn->chn_idx;
                }
                else if(rollback_num){
                    cur_SnifferSync->sync_expectTime = ps_conn->connExpectTime - pc_conn->conn_intvl_tick * rollback_num;
                    cur_SnifferSync->sync_conn_inst = pc_conn->conn_inst - rollback_num;
                    #if (0)
                        if((pc_conn->chn_idx == 0) && (rollback_num == 1)){
                            cur_SnifferSync->sync_chn_idx = 36;
                        }
                        else if((pc_conn->chn_idx == 0) && (rollback_num == 2)){
                            cur_SnifferSync->sync_chn_idx = 35;
                        }
                        else if((pc_conn->chn_idx == 1) && (rollback_num == 2)){
                            cur_SnifferSync->sync_chn_idx = 36;
                        }
                        else{
                            cur_SnifferSync->sync_chn_idx = pc_conn->chn_idx - rollback_num;
                        }

                    #else
                        if(pc_conn->chn_idx >= rollback_num){
                            cur_SnifferSync->sync_chn_idx = pc_conn->chn_idx - rollback_num;
                        }
                        else{
                            cur_SnifferSync->sync_chn_idx = (37 + pc_conn->chn_idx) - rollback_num;
                        }
                    #endif
                }
                else if(interval_jump_num){
                    cur_SnifferSync->sync_expectTime = ps_conn->connExpectTime + pc_conn->conn_intvl_tick * interval_jump_num;
                    cur_SnifferSync->sync_conn_inst = pc_conn->conn_inst + interval_jump_num;
                    cur_SnifferSync->sync_chn_idx = ((u32)(pc_conn->chn_idx + interval_jump_num))%37;
                }
            }
            else{//param->slave_instant != 0
                //slave instant valid
                if((u16)(pc_conn->conn_inst - param->slave_instant) < BIT(10)){
                    rollback_num = pc_conn->conn_inst - param->slave_instant;

                    cur_SnifferSync->sync_expectTime = ps_conn->connExpectTime - pc_conn->conn_intvl_tick * rollback_num;
                    cur_SnifferSync->sync_conn_inst = param->slave_instant;
                    if(pc_conn->chn_idx >= rollback_num){
                        cur_SnifferSync->sync_chn_idx = pc_conn->chn_idx - rollback_num;
                    }
                    else{
                        cur_SnifferSync->sync_chn_idx = (37 + pc_conn->chn_idx) - rollback_num;
                    }
                }
                else{
                    interval_jump_num = param->slave_instant - pc_conn->conn_inst;

                    cur_SnifferSync->sync_expectTime = ps_conn->connExpectTime + pc_conn->conn_intvl_tick * interval_jump_num;
                    cur_SnifferSync->sync_conn_inst = pc_conn->conn_inst + interval_jump_num;
                    cur_SnifferSync->sync_chn_idx = ((u32)(pc_conn->chn_idx + interval_jump_num))%37;
                }
            }
        }

        if(param->slave_status == SLAVE_STATUS_CONNECTION_UPDATE){
            cur_SnifferSync->sync_intvl_n_1m25 = param->slave_connInterval;
            cur_SnifferSync->sync_conn_timeout = param->slave_connTimeout * 10 * SYSTEM_TIMER_TICK_1MS;;
            cur_SnifferSync->sync_expectTime += param->slave_winOffset * SYSTEM_TIMER_TICK_1250US;

            //backup
            backup_SnifferSync->sync_intvl_n_1m25 = cur_SnifferSync->sync_intvl_n_1m25;
            backup_SnifferSync->sync_conn_timeout = cur_SnifferSync->sync_conn_timeout;
            //backup_SnifferSync->sync_expectTime   //not need

            pc_conn->conn_offset_next = param->slave_winOffset;//important
        }
        else if(param->slave_status == SLAVE_STATUS_CONNECTION_CHANNEL_MAP){
            smemcpy(cur_SnifferSync->sync_conn_chm, param->slave_connChannelMap, 5);

            //backup
            smemcpy(backup_SnifferSync->sync_conn_chm, cur_SnifferSync->sync_conn_chm, 5);
        }
        else{//SLAVE_STATUS_CONNECTION_GENERAL
            /* if the sync parameters are the same does not update create
             * blt_ll_acl_sniffer_mainloop reply sync result 'SNIFFER_SYNC_CREATE'
             */
            if(pc_conn->connState == CONN_STATUS_ESTABLISH && !pc_conn->acl_sniffer_sync_creating){
                if(ps_conn->brx_pkt_miss < 4){
                    u32 aclAccess_addr;
                    u32 aclCrc_init;
                    smemcpy(&aclAccess_addr, param->slave_accessAddress, 4);
                    smemcpy(&aclCrc_init, param->slave_crcInit, 3);

                    if((pc_conn->conn_intvl_n_1m25 == param->slave_connInterval) \
                        && (pc_conn->conn_timeout == (param->slave_connTimeout * 10 * SYSTEM_TIMER_TICK_1MS)) \
                        && (!smemcmp(pc_conn->acl_chnParam.chmTbl, param->slave_connChannelMap, 5)) \
                        && (pc_conn->conn_chn_hop == param->slave_connHop) \
                        && (pc_conn->aclAccessAddr == aclAccess_addr) \
                        && (pc_conn->aclCrcInit == aclCrc_init) \
                        && (pc_conn->conn_offset_next == param->slave_winOffset))
                    {
                        pc_conn->acl_sniffer_sync_update_ignore = 1;
                        irq_restore(r);
                        return BLE_SUCCESS;
                    }
                }
            }

            cur_SnifferSync->sync_intvl_n_1m25 = param->slave_connInterval;
            smemcpy(&cur_SnifferSync->sync_accessAddr, param->slave_accessAddress, 4);
            smemcpy(&cur_SnifferSync->sync_crcInit, param->slave_crcInit, 3);
            cur_SnifferSync->sync_conn_timeout = param->slave_connTimeout * 10 * SYSTEM_TIMER_TICK_1MS;
            smemcpy(cur_SnifferSync->sync_conn_chm, param->slave_connChannelMap, 5);
            cur_SnifferSync->sync_conn_hop = param->slave_connHop;

            //backup
            backup_SnifferSync->sync_intvl_n_1m25 = cur_SnifferSync->sync_intvl_n_1m25;
            backup_SnifferSync->sync_accessAddr = cur_SnifferSync->sync_accessAddr;
            backup_SnifferSync->sync_conn_timeout = cur_SnifferSync->sync_conn_timeout;
            backup_SnifferSync->sync_crcInit = cur_SnifferSync->sync_crcInit;
            smemcpy(backup_SnifferSync->sync_conn_chm, cur_SnifferSync->sync_conn_chm, 5);
            backup_SnifferSync->sync_conn_hop = cur_SnifferSync->sync_conn_hop;

            pc_conn->conn_offset_next = param->slave_winOffset;//important
        }
    }

    cur_SnifferSeek->seek_repeat_type = param->slave_status;
    cur_SnifferSeek->seek_repeat_count = 0;

    irq_restore(r);

    blmsParam.state_chng |= STATE_CHANGE_ACL_SNIFS;

    return BLE_SUCCESS;
}


int blc_ll_updateAclSnifferSlvSync_v3(u8 *cmd)
{
    acl_slave_interoperable_param_t* param = (acl_slave_interoperable_param_t*)cmd;

    //index: 0 ~ 3
    //interval: 7.5ms ~ 4s -> 6 ~ 3200
    //Timeout: 100ms ~ 32s -> 10 ~ 3200
    //hop: bit<0-5> ,range is 5 ~ 16;
    if(param->slave_index >= LL_MAX_ACL_PER_NUM \
        || param->slave_connInterval < 6 || param->slave_connInterval > 3200 \
        || param->slave_connTimeout < 10 || param->slave_connTimeout > 3200 \
        || (param->slave_connHop & 0x1f) < 5 || (param->slave_connHop & 0x1f) > 16){

        return SNIFFER_PARAMETER_INVALID;
    }

    #if (LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2)
        if((param->slave_CI_ChSel_PHY & 0x08) && (!ll_chn_index_calc_cb)){
            return SNIFFER_UNSUPPORTED_FEATURE;
        }
    #endif

    #if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
        if(((param->slave_CI_ChSel_PHY & 0x03) != BLE_PHY_1M) && (!ll_phy_switch_cb)){
            return SNIFFER_UNSUPPORTED_FEATURE;
        }
    #endif

    u8 pc_conn_sel = param->slave_index + LL_MAX_ACL_CEN_NUM; //pay attention here

    if(pc_conn_sel < LL_MAX_ACL_CEN_NUM || pc_conn_sel >= LL_MAX_ACL_CONN_NUM){
        return SNIFFER_UNKNOWN_SNIFHANDLE;
    }

    ///////////////////////////// parameter advance convert Begin /////////////////////////////
    //IRQ need to be turned off later, convert in advance to save time

    u32 sync_conn_timeout = param->slave_connTimeout * 10 * SYSTEM_TIMER_TICK_1MS;

    u32 sync_accessAddr;
    smemcpy(&sync_accessAddr, param->slave_accessAddress, 4);
    u32 sync_crcInit;
    smemcpy(&sync_crcInit, param->slave_crcInit, 3);

    u8 sync_conn_chnSel;
    u8 sync_CI_phy;
    if(param->slave_CI_ChSel_PHY == 0){
        //the primary node is not provided. The default parameters are used
        sync_CI_phy = 0x81;//(LE_CODED_S8 << 4) | BLE_PHY_1M
        sync_conn_chnSel = 0;//CSA#1
    }
    else{
        sync_CI_phy = param->slave_CI_ChSel_PHY & 0xF3;
        sync_conn_chnSel = param->slave_CI_ChSel_PHY & 0x08 ? 1 : 0;
    }
    ///////////////////////////// parameter advance convert End /////////////////////////////

    st_ll_conn_t *pc_conn =  (st_ll_conn_t *)&blms[pc_conn_sel];

    u8 ps_conn_sel = param->slave_index; //pay attention here
    st_lls_conn_t *ps_conn =  (st_lls_conn_t *)&blmsSlave[ps_conn_sel];

    acl_sniffer_seek_param_t *cur_SnifferSeek = (acl_sniffer_seek_param_t *)&aclSniffer_slv_seek[ps_conn_sel];

    u32 r = irq_disable();

    /* if the sync parameters are the same does not update create
     * blt_ll_acl_sniffer_mainloop reply sync result 'SNIFFER_SYNC_CREATE'
     */
    if(pc_conn->connState == CONN_STATUS_ESTABLISH && !pc_conn->acl_sniffer_sync_creating){
        if(ps_conn->brx_pkt_miss < 4){
            if((pc_conn->conn_intvl_n_1m25 == param->slave_connInterval) \
                && (pc_conn->conn_timeout == sync_conn_timeout) \
                && (!smemcmp(pc_conn->acl_chnParam.chmTbl, param->slave_connChannelMap, 5)) \
                && (pc_conn->conn_chn_hop == param->slave_connHop) \
                && (pc_conn->aclAccessAddr == sync_accessAddr) \
                && (pc_conn->aclCrcInit == sync_crcInit) \
                && (pc_conn->conn_chnsel == sync_conn_chnSel) \
                && (pc_conn->connPhyCtrl.conn_cur_phy == (sync_CI_phy & 0x0F)) \
                && (pc_conn->connPhyCtrl.conn_cur_CI == (sync_CI_phy & 0xF0 >> 4)))
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

    if(blmsParam.state_chng & STATE_CHANGE_ACL_SNIFS){
        return SNIFFER_CURRENT_STATE_NOT_SUPPORTED_THIS_CMD;
    }

    ///////////////////////////// valid parameter allow update Begin /////////////////////////////
    acl_sniffer_sync_param_t *cur_SnifferSync = (acl_sniffer_sync_param_t *)&acl_sniffer_slv_sync_info;

    cur_SnifferSync->sync_connHandle = pc_conn_sel | BLS_CONN_HANDLE;
    cur_SnifferSync->sync_CI_phy = sync_CI_phy;
    cur_SnifferSync->sync_conn_chnSel = sync_conn_chnSel;

    cur_SnifferSync->sync_expectTime = param->slave_connExpectTime;

    cur_SnifferSync->sync_conn_inst = param->slave_connEventCounter;
    cur_SnifferSync->sync_intvl_n_1m25 = param->slave_connInterval;

    cur_SnifferSync->sync_accessAddr = sync_accessAddr;
    cur_SnifferSync->sync_crcInit = sync_crcInit;
    cur_SnifferSync->sync_conn_timeout = sync_conn_timeout;

    smemcpy(cur_SnifferSync->sync_conn_chm, param->slave_connChannelMap, 5);
    cur_SnifferSync->sync_conn_hop = param->slave_connHop;
    cur_SnifferSync->sync_chn_idx = param->slave_connChannelIndex;
    cur_SnifferSync->sync_conn_num_config = CONN_MAX_NUM_CONFIG;
    ///////////////////////////// valid parameter allow update Begin /////////////////////////////

    cur_SnifferSeek->seek_repeat_type = 0;//not need, support re-listening during a existing connection

    blmsParam.state_chng |= STATE_CHANGE_ACL_SNIFS;

    return BLE_SUCCESS;
}


#if (!SNIFFER_USE_SOME_COMMON_APIS)
_attribute_ram_code_ int blt_ll_sniffer_slv_seek_anchor(u8 *cmd)
{
    acl_sniffer_sync_param_t* param = (acl_sniffer_sync_param_t*)cmd;

    #if (0)//move to blc_ll_updateAclSnifferSlvSync()
        blms_conn_sel = param->sync_connHandle & CONN_IDX_MASK;
        blms_pconn =  (st_ll_conn_t *)   &blms[blms_conn_sel];

        bls_conn_sel = blms_conn_sel - LL_MAX_ACL_CEN_NUM; //pay attention here
        bls_pconn =  (st_lls_conn_t *)&blmsSlave[bls_conn_sel];

        /* if the sync parameters are the same does not update create
         * blt_ll_acl_sniffer_mainloop reply sync result 'SNIFFER_SYNC_CREATE'
         */
        if(blms_pconn->connState == CONN_STATUS_ESTABLISH && !blms_pconn->acl_sniffer_sync_creating){
            if(bls_pconn->brx_pkt_miss < 4){
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
        bls_conn_sel = blms_conn_sel - LL_MAX_ACL_CEN_NUM; //pay attention here
    #endif

    pSnifferSlvSeek = (acl_sniffer_seek_param_t *)&aclSniffer_slv_seek[bls_conn_sel];
    smemcpy((u8*)&pSnifferSlvSeek->sync.sync_connHandle, (u8*)&param->sync_connHandle, sizeof(acl_sniffer_sync_param_t));

    smemcpy(pSnifferSlvSeek->chnParam.chmTbl, param->sync_conn_chm, 5);
    #if (LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2)
        if(pSnifferSlvSeek->sync.sync_conn_chnSel)
        {
            pSnifferSlvSeek->seek_chnIdentifier = (pSnifferSlvSeek->sync.sync_accessAddr>>16) ^ (pSnifferSlvSeek->sync.sync_accessAddr&0xffff);
            csa2_calculateMapInfo(&pSnifferSlvSeek->chnParam);
        }
        else
        {
            blt_csa1_calculateChannelTable (pSnifferSlvSeek->sync.sync_conn_chm, pSnifferSlvSeek->sync.sync_conn_hop, pSnifferSlvSeek->chnParam.rempChmTbl);
        }
    #else
        blt_csa1_calculateChannelTable (pSnifferSlvSeek->sync.sync_conn_chm, pSnifferSlvSeek->sync.sync_conn_hop, pSnifferSlvSeek->chnParam.rempChmTbl);
    #endif

    pSnifferSlvSeek->tick_seek_interval = param->sync_intvl_n_1m25 * SYSTEM_TIMER_TICK_1250US;
    pSnifferSlvSeek->sSlot_seek_interval = TICKS_DUR_2_SSLOT_DUR(pSnifferSlvSeek->tick_seek_interval);

    // seek_tolerance cannot be greater than bltSche.bSlot_maxLen/2
    if(aclSniffer_slv_param.sniffer_1st_rxWindowMaxFlag){
        pSnifferSlvSeek->tick_seek_tolerance_ms = pSnifferSlvSeek->tick_seek_tolerance_max_ms;
    }
    else{
        pSnifferSlvSeek->tick_seek_tolerance_ms = aclSniffer_slv_param.sniffer_seek_halfWindow_ms;
    }
    pSnifferSlvSeek->sSlot_seek_tolerance = TICKS_DUR_2_SSLOT_DUR(pSnifferSlvSeek->tick_seek_tolerance_ms * SYSTEM_TIMER_TICK_1MS);

    if(pSnifferSlvSeek->sync.sync_expectTime < bltSche.sSlot_tick_irq_real){
        u16 jump_num;
        jump_num = (bltSche.sSlot_tick_irq_real - pSnifferSlvSeek->sync.sync_expectTime) / pSnifferSlvSeek->tick_seek_interval + 1;
        pSnifferSlvSeek->sync.sync_expectTime += pSnifferSlvSeek->tick_seek_interval*jump_num;
        pSnifferSlvSeek->sync.sync_conn_inst += jump_num;
        pSnifferSlvSeek->sync.sync_chn_idx = ((u32)(pSnifferSlvSeek->sync.sync_chn_idx + jump_num))%37;
    }

    int n_sSlot = TICKS_DUR_2_SSLOT_DUR(pSnifferSlvSeek->sync.sync_expectTime - bltSche.sSlot_tick_irq_real);
    pSnifferSlvSeek->sSlot_sync_expectTime = bltSche.sSlot_idx_irq_real + n_sSlot;
    pSnifferSlvSeek->seek_count = 0;
    pSnifferSlvSeek->seek_state = 0;
    pSnifferSlvSeek->seeking = 1;
    pSnifferSlvSeek->seek_stop = 0;
    pSnifferSlvSeek->tick_seek_start = clock_time() | 1;

    blt_ll_setSchedulerTaskPriority( TSKOFT_SNIFS_SEEK + bls_conn_sel, TASK_PRIORITY_MAX );

    blt_sche_addTaskMask(TSKMSK_SNIFS_SEEK_0 << bls_conn_sel);
    blt_sche_addUpdate(SLOT_UPDT_SNIF_SEEK_CREATE);

    return BLE_SUCCESS;
}


_attribute_ram_code_ int blt_ll_buildAclSnifferSlvSeekSchLinklist(void)
{
    u32 i,j;

    for( i = 0;i < TSKNUM_SNIFS_SEEK;i++ )
    {
        if( bltSche.task_mask & (TSKMSK_SNIFS_SEEK_0<<i) )
        {
            pSnifferSlvSeek = (acl_sniffer_seek_param_t *)&aclSniffer_slv_seek[i];

            if(bltSche.sSlot_idx_reset == 1 && bltSche.build_index == 0){
                pSnifferSlvSeek->sSlot_sync_expectTime -= bltSche.sSlot_idx_past;
            }

            s32 sSlot_first_seek_window_tail = pSnifferSlvSeek->sSlot_sync_expectTime + pSnifferSlvSeek->sSlot_seek_tolerance;
            s32 sSlot_sche_start = bltSche.sSlot_idx_next;
            u16 first_seek_jump_num = 0;

            if(sSlot_first_seek_window_tail < sSlot_sche_start){
                first_seek_jump_num = (sSlot_sche_start - sSlot_first_seek_window_tail) / pSnifferSlvSeek->sSlot_seek_interval + 1;
            }

            s32 sSlot_last_seek_window_head = pSnifferSlvSeek->sSlot_sync_expectTime - pSnifferSlvSeek->sSlot_seek_tolerance;
            s32 sSlot_sche_end = bltSche.sSlot_endIdx_dft;  // 126 -> 78.75ms
            u16 last_seek_jump_num = 0;

            if(sSlot_last_seek_window_head < sSlot_sche_end){
                last_seek_jump_num = (sSlot_sche_end - sSlot_last_seek_window_head) / pSnifferSlvSeek->sSlot_seek_interval;
            }

            u16 jump_num_min = min(first_seek_jump_num, last_seek_jump_num);
            u16 jump_num_max = max(first_seek_jump_num, last_seek_jump_num);
            pSnifferSlvSeek->jump_num_valid = 0;

            s32 sSlot_task_duration_max = 0;
            for(j = jump_num_min; j < jump_num_max; j++)
            {
                s32 sSlot_task_start = pSnifferSlvSeek->sSlot_sync_expectTime + pSnifferSlvSeek->sSlot_seek_interval * j - pSnifferSlvSeek->sSlot_seek_tolerance;
                if(sSlot_task_start > sSlot_sche_end){
                    break;
                }
                s32 sSlot_task_end = pSnifferSlvSeek->sSlot_sync_expectTime + pSnifferSlvSeek->sSlot_seek_interval * j + pSnifferSlvSeek->sSlot_seek_tolerance;
                s32 sSlot_task_duration_cur = min(sSlot_task_end, sSlot_sche_end) - max(sSlot_task_start, sSlot_sche_start);// less than seek_tolerance*2

                if(sSlot_task_duration_cur > sSlot_task_duration_max){
                    sSlot_task_duration_max = sSlot_task_duration_cur;
                    pSnifferSlvSeek->jump_num_valid = j;
                    if(sSlot_task_duration_max >= (pSnifferSlvSeek->sSlot_seek_tolerance << 1)){
                        break;
                    }
                }
            }

            pSnifferSlvSeek->sSlot_seek_expectTime = pSnifferSlvSeek->sSlot_sync_expectTime + pSnifferSlvSeek->sSlot_seek_interval * pSnifferSlvSeek->jump_num_valid;

            s32 sSlot_seek_window_head;
            sSlot_seek_window_head = pSnifferSlvSeek->sSlot_seek_expectTime - pSnifferSlvSeek->sSlot_seek_tolerance;
            if(sSlot_seek_window_head >= sSlot_sche_end){
                continue;
            }
            if(sSlot_seek_window_head < sSlot_sche_start + 2){
                sSlot_seek_window_head = sSlot_sche_start + 2;
            }

            s32 sSlot_seek_window_tail;
            sSlot_seek_window_tail = pSnifferSlvSeek->sSlot_seek_expectTime + pSnifferSlvSeek->sSlot_seek_tolerance;
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
                    BLMS_ERR_DEBUG(SCH_DEBUG_EN, 0xEE010000);
                #endif
                continue;
            }

            //pSnifferSlvSeek->sSlot_seek_duration = (u32)(sSlot_seek_window_tail - sSlot_seek_window_head);
            pSnifferSlvSeek->sSlot_seek_duration = sSlot_seek_window_tail - sSlot_seek_window_head;
            if(pSnifferSlvSeek->sSlot_seek_duration < pSnifferSlvSeek->sSlot_seek_tolerance){
                //actual seek duration less than setting seek half window
                continue;
            }

            sch_task_t  *pCur_schTask = (sch_task_t *)&pSnifferSlvSeek->snifferTsk_fifo;

            pCur_schTask->begin = sSlot_seek_window_head;
            pCur_schTask->end = sSlot_seek_window_tail;

            #if (0)//Note: does not match the design, no longer needed. correct fix reference SHA-1: 91d6f2b89188e79b17b73e137af18d3c34241bff
                //***Important***
                //record the task with highest priority, to guarantee that task not missed
                //prevent other tasks use bltPri.priMax_value to change bltSche.sSlot_endIdx_maxPri
                //finally cause the seek task is kicked out
                if(bltPri.pri_cal[TSKOFT_SNIFS_SEEK + i] > bltPri.priMax_value){
                    bltPri.priMax_value = bltPri.pri_cal[TSKOFT_SNIFS_SEEK + i];
                    bltPri.priMax_index = TSKOFT_SNIFS_SEEK + i;
                    //bltSche.sSlot_endIdx_maxPri = pCur_schTask->end + 1;// no need
                }
            #endif

            blt_ll_addTask2ExistLinklist(pCur_schTask, 1);

            break;//currently only one seek task is assigned
        }
    }

    return BLE_SUCCESS;
}


_attribute_ram_code_ int blt_sniffer_slv_seek_start (int seek_idx)
{
    blms_start_pre_process(seek_idx + LL_MAX_ACL_CEN_NUM);
    blt_debug_gpio_toggle_acl_sniffer();
    blt_debug_gpio_toggle_acl_sniffer();

    bls_conn_sel = seek_idx;//pay attention here, seek_idx 0~3(TSKNUM_SNIFS_SEEK-1)
    bls_pconn =  (st_lls_conn_t *)&blmsSlave[bls_conn_sel];

    aclSniffer_slv_param.sniffer_rx_num = 0;
    aclSniffer_slv_param.sniffer_seek_tick_1st_rx = 0;

    //reset_baseband(); //QiangKai: Eagle can not reset, all RF baseband setting will lost(But Kite/Vulture must add this)

    ble_rf_set_rx_dma((u8*)aclConn_param.acl_rx_dma_buff, aclConn_param.acl_rx_dma_size);
    u16 rx_max_len = blt_llms_get_connEffectiveMaxRxOctets_by_connIdx(blms_conn_sel);
    rf_set_rx_maxlen(rx_max_len+4);//MCI 4ytes

    pSnifferSlvSeek = (acl_sniffer_seek_param_t *)&aclSniffer_slv_seek[bls_conn_sel];

    u8 seek_chn;
    u8 seek_chn_idx = ((u32)(pSnifferSlvSeek->sync.sync_chn_idx + pSnifferSlvSeek->jump_num_valid))%37;
    u16 seek_inst = pSnifferSlvSeek->sync.sync_conn_inst + pSnifferSlvSeek->jump_num_valid;
    #if (LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2)
        if(pSnifferSlvSeek->sync.sync_conn_chnSel)
        {
            seek_chn = ll_chn_index_calc_cb(&pSnifferSlvSeek->chnParam, seek_inst, pSnifferSlvSeek->seek_chnIdentifier);
        }
        else
        {
            seek_chn = pSnifferSlvSeek->chnParam.rempChmTbl[seek_chn_idx];
        }
    #else
        seek_chn = pSnifferSlvSeek->chnParam.rempChmTbl[seek_chn_idx];
    #endif

    #if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
        if(ll_phy_switch_cb){
            u8 sync_CI_phy = pSnifferSlvSeek->sync.sync_CI_phy;//0~3bits: le_phy_type_t; 4~7bits: le_coding_ind_t
            ll_phy_switch_cb(sync_CI_phy & 0x0F, (sync_CI_phy & 0xF0) >> 4); //rf_ble_switch_phy
        }
    #endif

    rf_set_ble_channel(seek_chn);
    rf_set_ble_access_code((u8 *)&pSnifferSlvSeek->sync.sync_accessAddr);
    rf_set_ble_crc_value(pSnifferSlvSeek->sync.sync_crcInit);
    CLEAR_ALL_RFIRQ_STATUS;

    rf_set_rxmode();

    rf_set_1st_rx_timeout(0xffffff);
    rf_ble_set_rx_settle(RX_SETTLE_US);

    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_RX_ON);  }

    blms_state = BLMS_STATE_SNIFS_SEEK_S;
    systick_irq_trigger = SYS_IRQ_TRIG_SNIFS_SEEK_POST;

    aclConn_param.task_end_tick = bltSche.sSlot_tick_irq_real + SSLOT_DUR_2_TICKS_DUR(pSnifferSlvSeek->sSlot_seek_duration);

    systimer_set_irq_capture(aclConn_param.task_end_tick - (SLOT_PROCESS_MAX_TICK + BOUNDARY_RX_RELOAD_TICK));

    return BLE_SUCCESS;
}


_attribute_ram_code_ int blt_sniffer_slv_seek_post(void)
{
    if(blmsParam.rf_fsm_busy) {
        blmsParam.rf_fsm_busy = 0;

        rf_set_tx_rx_off();
        STOP_RF_STATE_MACHINE;  //stop state machine
        CLEAR_ALL_RFIRQ_STATUS;
    }

    blms_state = BLMS_STATE_SNIFS_SEEK_E;

    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_OFF);  }

    pSnifferSlvSeek->seek_count++;

    if((pSnifferSlvSeek->seek_count >= aclSniffer_slv_param.sniffer_seek_count_max) && (!pSnifferSlvSeek->seek_state)){
        //reach seek max count
        #if (0)
            pSnifferSlvSeek->seek_state = 2;//Fail
        #else
            if(pSnifferSlvSeek->seek_repeat_type){
                pSnifferSlvSeek->seek_repeat_count++;
                if(pSnifferSlvSeek->seek_repeat_count < 4){
                    if(pSnifferSlvSeek->seek_repeat_count < 3){
                        if(pSnifferSlvSeek->seek_repeat_type != SLAVE_STATUS_CONNECTION_SETUP){
                            pSnifferSlvSeek->sync.sync_conn_inst--;
                            if(pSnifferSlvSeek->sync.sync_chn_idx == 0){
                                pSnifferSlvSeek->sync.sync_chn_idx = 36;
                            }
                            else{
                                pSnifferSlvSeek->sync.sync_chn_idx--;
                            }
                        }
                        pSnifferSlvSeek->sync.sync_expectTime -= pSnifferSlvSeek->tick_seek_interval;
                        pSnifferSlvSeek->sSlot_sync_expectTime -= pSnifferSlvSeek->sSlot_seek_interval;
                    }
                    else if(pSnifferSlvSeek->seek_repeat_count == 3){
                        if(pSnifferSlvSeek->seek_repeat_type != SLAVE_STATUS_CONNECTION_SETUP){
                            pSnifferSlvSeek->sync.sync_conn_inst += 3;
                            pSnifferSlvSeek->sync.sync_chn_idx = ((u32)(pSnifferSlvSeek->sync.sync_chn_idx + 3))%37;
                        }
                        pSnifferSlvSeek->sync.sync_expectTime += pSnifferSlvSeek->tick_seek_interval * 3;
                        pSnifferSlvSeek->sSlot_sync_expectTime += pSnifferSlvSeek->sSlot_seek_interval * 3;
                    }
                    pSnifferSlvSeek->seek_count = 0;
                }
                else{
                    pSnifferSlvSeek->seek_state = 2;//Fail
                }
            }
            else{
                pSnifferSlvSeek->seek_state = 2;//Fail
            }
        #endif
    }

    if(pSnifferSlvSeek->seek_stop == 1){
        pSnifferSlvSeek->seek_state = 3;//report stop event
    }
    else if(pSnifferSlvSeek->seek_stop == 2){
        pSnifferSlvSeek->seek_state = 4;//not report stop event
    }

    if(pSnifferSlvSeek->seek_state){
        pSnifferSlvSeek->seeking = 0;
        pSnifferSlvSeek->seek_stop = 0;
        pSnifferSlvSeek->tick_seek_start = 0;

        blt_sche_removeTaskMask(TSKMSK_SNIFS_SEEK_0<<bls_conn_sel);  //pay attention here

        if(pSnifferSlvSeek->seek_state == 1){//Succeed
            blt_debug_gpio_toggle_acl_sniffer();
            blt_debug_gpio_toggle_acl_sniffer();

            blt_sniffer_slv_seek2sync();
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


_attribute_ram_code_ int irq_acl_sniffer_slv_seek_rx(void)
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
            if(!aclSniffer_slv_param.sniffer_rx_num){
                if(!aclSniffer_slv_param.sniffer_seek_tick_1st_rx){
                    //need to continue monitor the peer-slave
                    aclSniffer_slv_param.sniffer_seek_tick_1st_rx = bltRxPkt.rx_header_tick;
                    seek_rfLen_1st_rx = raw_pkt[DMA_RFRX_OFFSET_RFLEN];

                    //remote:  |1st_timing|<---T_IFS--->|2st_timing|<---MARGIN--->
                    u32 pkt_receive_cost_us = blt_phy_getRfPacketTime_us(seek_rfLen_1st_rx, bltPHYs.cur_llPhy, bltPHYs.cur_peer_CI);
                    pkt_receive_cost_us += BLE_T_IFS;
                    pkt_receive_cost_us += blt_phy_getRfPacketTime_us(255, bltPHYs.cur_llPhy, LE_CODED_S8);//the length is considered the maximum
                    pkt_receive_cost_us += SNIFFER_SEEK_RX_POST_MARGIN_US;

                    u32 seek_post_tick = aclSniffer_slv_param.sniffer_seek_tick_1st_rx + pkt_receive_cost_us * SYSTEM_TIMER_TICK_1US;
                    if((seek_post_tick < systimer_get_irq_capture()) && (seek_post_tick > (clock_time() + 50*SYSTEM_TIMER_TICK_1US))){
                        systimer_set_irq_capture(seek_post_tick);
                    }

                    DBG_CHN3_TOGGLE;
                    DBG_CHN3_TOGGLE;
                }
            }
            else if(aclSniffer_slv_param.sniffer_rx_num == 1)
            {
                if(aclSniffer_slv_param.sniffer_seek_tick_1st_rx){
                    u32 diff = bltRxPkt.rx_header_tick - aclSniffer_slv_param.sniffer_seek_tick_1st_rx;

                    //e.g.: For 1M: 10 Byte = 1B(preamble) + 4B(accesscode) + 2B(header) + 3B(CRC), 150 is T_IFS

                    //remote:  |1st_timing|<---T_IFS--->|2st_timing|
                    //local:   |rx_head_tick + 1st_timing + T_IFS
                    u32 diff_ideal = (blt_phy_getRfPacketTime_us(seek_rfLen_1st_rx, bltPHYs.cur_llPhy, bltPHYs.cur_peer_CI) + BLE_T_IFS) * SYSTEM_TIMER_TICK_1US;

                    // T_IFS within 20us
                    if((diff > (diff_ideal - 20 * SYSTEM_TIMER_TICK_1US)) && (diff < (diff_ideal + 20 * SYSTEM_TIMER_TICK_1US))){
                        //monitor the peer-slave successful
                        pSnifferSlvSeek->seek_state = 1;//Succeed

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

        aclSniffer_slv_param.sniffer_rx_num ++;  //care CRC
    }

    return BLE_SUCCESS;
}
#endif


_attribute_ram_code_ int blt_sniffer_slv_seek2sync(void){

#if (DEBUG_SNIFFER_NULL_POINTER_EN)
    if(!pSnifferSlvSeek){
        printf("err:%s Line:%d\n", __FILE__, __LINE__);
    }
#endif

    ///////////////////////////// blms_connect_common Begin /////////////////////////////
    blms_pconn->aclAccessAddr = pSnifferSlvSeek->sync.sync_accessAddr;
    blms_pconn->aclCrcInit = pSnifferSlvSeek->sync.sync_crcInit;

    blms_pconn->conn_intvl_n_1m25 = pSnifferSlvSeek->sync.sync_intvl_n_1m25;
    blms_pconn->conn_intvl_tick = pSnifferSlvSeek->sync.sync_intvl_n_1m25 * SYSTEM_TIMER_TICK_1250US;
    blt_ll_set_interval_level(TSKOFT_ACL_CONN + blms_conn_sel, pSnifferSlvSeek->sync.sync_intvl_n_1m25);

    blms_pconn->conn_latency = 0;
    blms_pconn->conn_timeout = pSnifferSlvSeek->sync.sync_conn_timeout;
    blms_pconn->conn_chn_hop = pSnifferSlvSeek->sync.sync_conn_hop;
    blms_pconn->conn_sca = 0;//0: 251 ppm to 500 ppm
    smemcpy(blms_pconn->acl_chnParam.chmTbl, pSnifferSlvSeek->sync.sync_conn_chm, 5);

    blms_pconn->conn_chnsel = pSnifferSlvSeek->sync.sync_conn_chnSel;
    blms_pconn->chn_idx = ((u32)(pSnifferSlvSeek->sync.sync_chn_idx + pSnifferSlvSeek->jump_num_valid))%37;
#if(LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2)
    if(blms_pconn->conn_chnsel)
    {
        blms_pconn->chnIdentifier = (blms_pconn->aclAccessAddr>>16) ^ (blms_pconn->aclAccessAddr&0xffff);
        csa2_calculateMapInfo(&blms_pconn->acl_chnParam);
    }
    else
    {
        blt_csa1_calculateChannelTable (pSnifferSlvSeek->sync.sync_conn_chm, blms_pconn->conn_chn_hop, blms_pconn->acl_chnParam.rempChmTbl);
    }
#else
    blt_csa1_calculateChannelTable (pSnifferSlvSeek->sync.sync_conn_chm, blms_pconn->conn_chn_hop, blms_pconn->chnParam.rempChmTbl);
#endif

    // need to check whether the sniffer sync_connState already exists.
    // if the Sniffer sync_connState does not exist, add 1
    if(blms_pconn->connState == CONN_STATUS_DISCONNECT){
        blmsParam.cur_slave_num ++;
        if(blmsParam.cur_slave_num > blmsParam.max_slave_num){
            blmsParam.cur_slave_num = blmsParam.max_slave_num;
        }
    }

    blms_pconn->connState = CONN_STATUS_COMPLETE;
    blms_pconn->irq_event1_union.connect_evt = 1;//CallBack process later in mainLoop
    aclConn_param.connSync |= (1<<blms_conn_sel);

    blms_pconn->conn_inst = pSnifferSlvSeek->sync.sync_conn_inst + pSnifferSlvSeek->jump_num_valid;
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

    bltPHYs.cur_llPhy = pSnifferSlvSeek->sync.sync_CI_phy & 0x0F;
    bltPHYs.cur_own_CI = (pSnifferSlvSeek->sync.sync_CI_phy & 0xF0 >> 4);
#if (LL_FEATURE_ENABLE_LE_2M_PHY | LL_FEATURE_ENABLE_LE_CODED_PHY)
    blt_cfg_conn_phy_param(&blms_pconn->connPhyCtrl, bltPHYs.cur_llPhy, bltPHYs.cur_own_CI); //Reset conn_cur_phy and conn_cur_CI to the dft settings.
#endif

    blt_ll_setSchedulerTaskPriority( TSKOFT_ACL_CONN + blms_conn_sel, TASK_PRIORITY_CONN_CREATE );

    #if (BLMS_PM_ENABLE)
        blms_pconn->pm_error_us = 0;
    #endif
    ///////////////////////////// blms_connect_common End /////////////////////////////


    blms_pconn->bSlot_interval = pSnifferSlvSeek->sync.sync_intvl_n_1m25<<1;
    bls_pconn->sSlot_interval = BSLOT_DUR_2_SSLOT_DUR(blms_pconn->bSlot_interval);

    blms_pconn->conn_tick = clock_time();
    bls_pconn->connExpectTime = aclSniffer_slv_param.sniffer_seek_tick_1st_rx;

    int tolerance_us = 0;
    #if (BLMS_PM_ENABLE)
        blt_ll_set_slave_conn_interval_level(blms_pconn, bls_pconn, pSnifferSlvSeek->sync.sync_intvl_n_1m25);

        bls_pconn->latency_available = 0;
        bls_pconn->latency_wakeup_flg = 0;
        bls_pconn->sleep_sys_ms = 0;
        bls_pconn->sleep_32k_rc = 0;

        tolerance_us = 2000 + aclSniffer_slv_param.sniffer_sync_earlyTime_add;
        u32 conn_interval_us = blms_pconn->conn_intvl_tick / SYSTEM_TIMER_TICK_1US;
        u32 tolerance_max_us = conn_interval_us * 9 / 20;

        if(tolerance_us > tolerance_max_us){
            tolerance_us = tolerance_max_us;
        }

        if(tolerance_us > 10000){
            tolerance_us = 10000;
        }

        blmsPm.slave_no_sleep |= (1<<bls_conn_sel);
    #endif

    bls_pconn->conn_tolerance_us = tolerance_us;
    bls_pconn->conn_start_time = bls_pconn->connExpectTime - BRX_LEFT_EARLY_TICK - bls_pconn->conn_tolerance_us*SYSTEM_TIMER_TICK_1US;

    u16 jump_num = 0;
    if(bls_pconn->conn_start_time < bltSche.sSlot_tick_irq_real){
        jump_num = (bltSche.sSlot_tick_irq_real - bls_pconn->conn_start_time) / blms_pconn->conn_intvl_tick + 1;
        bls_pconn->conn_start_time += blms_pconn->conn_intvl_tick*jump_num;
        bls_pconn->connExpectTime += blms_pconn->conn_intvl_tick*jump_num;
        blms_pconn->conn_inst += jump_num;
        blms_pconn->chn_idx = ((u32)(blms_pconn->chn_idx + jump_num))%37;
    }
    blms_pconn->acl_sniffer_establish_inst = blms_pconn->conn_inst;

    int n_sSlot = TICKS_DUR_2_SSLOT_DUR(bls_pconn->conn_start_time - bltSche.sSlot_tick_irq_real);
    bls_pconn->sSlot_mark_conn = bltSche.sSlot_idx_irq_real + n_sSlot - bls_pconn->sSlot_interval;
    blms_pconn->bSlot_mark_conn = bltSche.bSlot_idx_start + SSLOT_DUR_2_BSLOT_DUR(bls_pconn->sSlot_mark_conn);

    #if (LL_FEATURE_ENABLE_LE_2M_PHY | LL_FEATURE_ENABLE_LE_CODED_PHY)
        blms_pconn->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + pdu_27b_tifs_27b_sslot[blms_pconn->connPhyCtrl.conn_cur_phy - 1][1] + (2*bls_pconn->conn_tolerance_us*SSLOT_US_REVERSE);
    #else
        blms_pconn->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + PAYLOAD_27B_TIFS_27B_ENCRT_1MPHY_SSLOT_NUM + (2*bls_pconn->conn_tolerance_us*SSLOT_US_REVERSE);
    #endif

    /* here use scheduler process 15 small slot to simplify code, it's OK */
    blms_pconn->sSlot_sche_use = 15;   //give 19.5*15=292 uS
    blms_pconn->sSlot_duration = blms_pconn->sSlot_allocNum + blms_pconn->sSlot_sche_use;

    bls_pconn->sSlot_offset = 0; //clear when connect, no not clear when terminate

    blms_pconn->sync_timing = SLAVE_SYNC_CONN_CREATE;
    blms_pconn->acl_sniffer_sync_creating = 1;

    blt_sche_addUpdate(SLOT_UPDT_SLAVE_CONN_CREATE);
    blt_sche_addTaskMask(TSKMSK_ACL_CONN_0<<blms_conn_sel);

    return BLE_SUCCESS;
}


void blc_ll_addAclSnifferSlvSyncEarlyTime(u32 earlyTime_us)
{
    aclSniffer_slv_param.sniffer_sync_earlyTime_add = earlyTime_us;
}


ble_sts_t   blc_ll_setAclSnifferSlvReportRssiType(acl_sniffer_rssi_report_type_t rssi_type)
{
    if(rssi_type < RSSI_TYPE_MASTER || rssi_type > RSSI_TYPE_ALL)
    {
        return LL_ERR_INVALID_PARAMETER;
    }

    aclSniffer_slv_param.sniffer_rssi_reportType = rssi_type;

    return BLE_SUCCESS;
}


void blc_ll_setAclSnifferSlv1stSyncWinMaxEnable(u8 enable)
{
    aclSniffer_slv_param.sniffer_1st_rxWindowMaxFlag = enable;
}


int blc_ll_getAclSnifferSlvSyncNumber(void)
{
    return blmsParam.cur_slave_num;
}


int blc_ll_getAclSnifferSlvSyncStatus(u16 snifHandle)
{
    u8 idx = snifHandle & CONN_IDX_MASK;

    if(idx < LL_MAX_ACL_CEN_NUM || idx >= LL_MAX_ACL_CONN_NUM){
        return SNIFFER_UNKNOWN_SNIFHANDLE;
    }

    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[idx];

    acl_sniffer_seek_param_t *cur_SnifferSeek = (acl_sniffer_seek_param_t *)&aclSniffer_slv_seek[idx - LL_MAX_ACL_CEN_NUM];

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


int blc_ll_setAclSnifferSlvTerminateSync(u16 snifHandle)
{
    u8 pc_conn_sel = snifHandle & CONN_IDX_MASK;
    st_ll_conn_t *pc_conn =  (st_ll_conn_t *)&blms[pc_conn_sel];

    if(pc_conn_sel < LL_MAX_ACL_CEN_NUM || pc_conn_sel >= LL_MAX_ACL_CONN_NUM){
        return SNIFFER_UNKNOWN_SNIFHANDLE;
    }

    u8 ps_conn_sel = pc_conn_sel - LL_MAX_ACL_CEN_NUM; //pay attention here
    acl_sniffer_seek_param_t *cur_SnifferSeek = (acl_sniffer_seek_param_t *)&aclSniffer_slv_seek[ps_conn_sel];

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
void  blt_ll_acl_sniffer_slv_mainloop(void)
{
    blt_ll_acl_sniffer_mainloop();

    for(int snif_idx = LL_MAX_ACL_CEN_NUM; snif_idx < LL_MAX_ACL_CONN_NUM; snif_idx++)
    {
        u8 pSnif_seek_sel = snif_idx - LL_MAX_ACL_CEN_NUM;
        acl_sniffer_seek_param_t *cur_SnifferSeek = (acl_sniffer_seek_param_t *)&aclSniffer_slv_seek[pSnif_seek_sel];

        u32 seek_timeout_us = aclSniffer_slv_param.sniffer_seek_timeout_ms;
        seek_timeout_us *= 1000;
        if(cur_SnifferSeek->tick_seek_start && clock_time_exceed(cur_SnifferSeek->tick_seek_start, seek_timeout_us)){
            //seek state timeout
            u32 r = irq_disable();
            if(blms_state != BLMS_STATE_SNIFS_SEEK_S){
                cur_SnifferSeek->tick_seek_start = 0;
                if(cur_SnifferSeek->seeking){
                    cur_SnifferSeek->seeking = 0;
                    cur_SnifferSeek->seek_stop = 0;
                    cur_SnifferSeek->seek_state = 2;//Fail

                    blt_sche_removeTaskMask(TSKMSK_SNIFS_SEEK_0<<pSnif_seek_sel);
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


void  blt_ll_reset_acl_sniffer_slv(void)
{
    /*pay attention: all parameters which clear in connection terminate procedure should be considered if need clear in HCI reset callback function*/
    for(int snif_idx = LL_MAX_ACL_CEN_NUM; snif_idx < LL_MAX_ACL_CONN_NUM; snif_idx++)
    {
        st_ll_conn_t *pc_conn = (st_ll_conn_t*)&blms[snif_idx];

        pc_conn->acl_sniffer_sync_creating = 0;
        pc_conn->acl_sniffer_sync_update_ignore = 0;

        u8 pSnif_seek_sel = snif_idx - LL_MAX_ACL_CEN_NUM;
        acl_sniffer_seek_param_t *cur_SnifferSeek = (acl_sniffer_seek_param_t *)&aclSniffer_slv_seek[pSnif_seek_sel];

        cur_SnifferSeek->seek_state = 0;
        cur_SnifferSeek->seeking = 0;
        cur_SnifferSeek->seek_stop = 0;
        cur_SnifferSeek->tick_seek_start = 0;
    }
}


_attribute_noinline_
int blt_acl_sniffer_slv_mainloop_task(int flag)
{
    if(flag == FLAG_MODULE_MAINLOOP){
        blt_ll_acl_sniffer_slv_mainloop();
    }
    else if(flag == FLAG_MODULE_RESET){
        blt_ll_reset_acl_sniffer_slv();
    }

    return BLE_SUCCESS;
}


_attribute_noinline_
void blc_ll_initAclSnifferSlv_module(void)
{
    ll_acl_sniffer_slv_irq_task_cb = blt_acl_sniffer_slv_irq_task;
    ll_acl_sniffer_slv_mlp_task_cb = blt_acl_sniffer_slv_mainloop_task;

    aclSniffer_slv_param.sniffer_rssi_reportType = RSSI_TYPE_MASTER;

    sniffer_rx_null_fifo.wptr = sniffer_rx_null_fifo.rptr = 0;

    aclSniffer_slv_param.sniffer_seek_timeout_ms = 10000;//10 second
    aclSniffer_slv_param.sniffer_seek_halfWindow_ms = 20;// +-20ms
    aclSniffer_slv_param.sniffer_seek_count_max = 3;

    for(int i=0; i<TSKNUM_SNIFS_SEEK; i++){
        acl_sniffer_seek_param_t *cur_SnifferSeek = (acl_sniffer_seek_param_t *)&aclSniffer_slv_seek[i];

        cur_SnifferSeek->snifferTsk_fifo.scheTask_oft = TSKOFT_SNIFS_SEEK + i;
        cur_SnifferSeek->snifferTsk_fifo.scheTask_idx = i;
        cur_SnifferSeek->snifferTsk_fifo.scheTask_flg = TSKFLG_SNIFS_SEEK;

        //cur_SnifferSeek->tick_seek_tolerance_ms = 20; // +-20ms
        cur_SnifferSeek->tick_seek_tolerance_max_ms = RSSI_SNIFFER_SEEK_TOLERANCE_MAX_MS; // +-35ms
    }
}


// only for slave
ble_sts_t blc_ll_getAclSlaveConnectionTimingParameter(u16 connHandle, u8* aclSlaveParam)
{
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[conn_idx];

    if(conn_idx < LL_MAX_ACL_CEN_NUM || conn_idx >= LL_MAX_ACL_CONN_NUM){
        return LL_ERR_INVALID_PARAMETER;
    }

    if(blt_ll_isAclhdlInvalid(pc->acl_conHandle)){
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    if(!pc->conn_established_tick){
        return LL_ERR_CONNECTION_NOT_ESTABLISH;
    }

    st_lls_conn_t* ps = (st_lls_conn_t*)&blmsSlave[conn_idx - LL_MAX_ACL_CEN_NUM];
    acl_sniffer_sync_param_t* param = (acl_sniffer_sync_param_t*)aclSlaveParam;

    u32 r = irq_disable();

    param->sync_connHandle = connHandle;
    param->sync_CI_phy = pc->connPhyCtrl.conn_cur_phy | (pc->connPhyCtrl.conn_cur_CI << 4);
    param->sync_conn_chnSel = pc->conn_chnsel;

    param->sync_expectTime = ps->connExpectTime;

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


ble_sts_t blc_ll_getAclSlaveConnectionSetupParameter(u16 connHandle, acl_slave_compatible_param_t *aclSlaveConnSetupParam)
{
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[conn_idx];

    if(conn_idx < LL_MAX_ACL_CEN_NUM || conn_idx >= LL_MAX_ACL_CONN_NUM){
        return LL_ERR_INVALID_PARAMETER;
    }

    if(blt_ll_isAclhdlInvalid(pc->acl_conHandle)){
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    acl_slave_compatible_param_t* param = (acl_slave_compatible_param_t*)aclSlaveConnSetupParam;

    u32 r = irq_disable();

    param->slave_index = conn_idx - LL_MAX_ACL_CEN_NUM;
    param->slave_status = SLAVE_STATUS_CONNECTION_SETUP;

    smemcpy(param->slave_accessAddress, &pc->aclAccessAddr, 4);
    smemcpy(param->slave_crcInit, &pc->aclCrcInit, 3);

    param->slave_winOffset = pc->conn_offset_next;//common use
    param->slave_connInterval = pc->conn_intvl_n_1m25;
    param->slave_connTimeout = pc->conn_timeout / (10 * SYSTEM_TIMER_TICK_1MS);

    smemcpy(param->slave_connChannelMap, pc->acl_chnParam.chmTbl, 5);
    param->slave_connHop = pc->conn_chn_hop;
    param->slave_instant = 0;
    param->slave_CI_ChSel_PHY = pc->connPhyCtrl.conn_cur_phy | (pc->conn_chnsel << 3) | (pc->connPhyCtrl.conn_cur_CI << 4);

    irq_restore(r);

    return BLE_SUCCESS;
}


u16 blc_ll_getAclSlaveConnectionUpdateWinOffset(u16 connHandle)
{
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[conn_idx];

    if(conn_idx < LL_MAX_ACL_CEN_NUM || conn_idx >= LL_MAX_ACL_CONN_NUM){
        return 0;
    }

    if(blt_ll_isAclhdlInvalid(pc->acl_conHandle)){
        return 0;
    }

    return pc->conn_offset_next;
}


#endif  //end of LL_RSSI_SNIFFER_SLAVE_ENABLE

