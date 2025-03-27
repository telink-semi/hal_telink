/********************************************************************************************************
 * @file    acl_sniffer.c
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

#include "acl_sniffer.h"


#if (LL_RSSI_SNIFFER_MODE_ENABLE)


extern blt_event_callback_t blt_p_event_callback;//pay attention here, must need, prevents running a null pointer


_attribute_ble_data_retention_ u8 sniffer_rx_null_fifo_b[SNIFFER_RX_NULL_FIFO_SIZE * SNIFFER_RX_NULL_FIFO_NUM] = {0};

_attribute_ble_data_retention_ sniffer_fifo_t sniffer_rx_null_fifo = {
                                                    SNIFFER_RX_NULL_FIFO_SIZE,
                                                    SNIFFER_RX_NULL_FIFO_NUM,
                                                    0,
                                                    0,
                                                    sniffer_rx_null_fifo_b,};


#if (SNIFFER_USE_SOME_COMMON_APIS)
_attribute_ble_data_retention_  acl_sniffer_seek_param_t *pSnifferSeek = NULL;
_attribute_ble_data_retention_  volatile acl_sniffer_param_t *pAclSnifferParam = NULL;

struct st_conn
{
    ////////////////// common for acl slave/master sniffer used vars begin /////////////////////
    u8      timing_update;
    u8      rx_pkt_miss;  //for slave, here name: brx_pkt_miss
    u8      sync_timing;
    u8      latency_wakeup_flg;
    u8      role; //refer to 'acl_connection_role_t'
    u8      slave_sleep_flg; //mst unused now
    u16     tolerance_max_us;

    u16     conn_tolerance_us;
    s16     sSlot_shift_tor;  //sSlot number shift for current tolerance
    s16     sSlot_offset;
    u16     latency_available;

    u32     sleep_32k_rc;
    u32     tick_1st_rx;
    u32     tick_last_1st_rx;  //mst unused now
    u32     expectTimeMark;
    u32     connExpectTime;
    u32     conn_start_time;
    s32     conn_offset_tick;
    u32     sSlot_interval;  //u16: 65536*20uS = 1.3S, not enough, max value 4S for BQB, so use u32
    s32     sSlot_mark_conn;
    u32     sleep_sys_ms; //slv used only
    u32     tick_conn_expect; //mst used only
    ////////////////// common for acl slave used vars end /////////////////////
};

union st_snif_conn
{
    union
    {
        #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
            st_llm_conn_t *m;
        #endif
        #if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
            st_lls_conn_t *s;
        #endif
    };
    struct st_conn *c; //common vars
};
_attribute_ble_data_retention_ union st_snif_conn sniffer_conn;
#endif


_attribute_ram_code_ void blt_debug_gpio_toggle_acl_sniffer(void)
{
    if(blms_conn_sel == 0){
        DBG_CHN4_TOGGLE;
    }
#if (LL_MAX_ACL_CONN_NUM > 1)
    else if(blms_conn_sel == 1){
        DBG_CHN5_TOGGLE;
    }
#endif
#if (LL_MAX_ACL_CONN_NUM > 2)
    else if(blms_conn_sel == 2){
        DBG_CHN6_TOGGLE;
    }
#endif
#if (LL_MAX_ACL_CONN_NUM > 3)
    else if(blms_conn_sel == 3){
        DBG_CHN7_TOGGLE;
    }
#endif
#if (LL_MAX_ACL_CONN_NUM > 4)
    else if(blms_conn_sel == 4){
        DBG_CHN8_TOGGLE;
    }
#endif
#if (LL_MAX_ACL_CONN_NUM > 5)
    else if(blms_conn_sel == 5){
        DBG_CHN9_TOGGLE;
    }
#endif
#if (LL_MAX_ACL_CONN_NUM > 6)
    else if(blms_conn_sel == 6){
        DBG_CHN10_TOGGLE;
    }
#endif
#if (LL_MAX_ACL_CONN_NUM > 7)
    else if(blms_conn_sel == 7){
        DBG_CHN11_TOGGLE;
    }
#endif
}

_attribute_ram_code_ void blt_sniffer_stop_rx_window(int time_us)
{
    //stop the Rx window in advance
    u32 new_post_time = clock_time () + time_us*SYSTEM_TIMER_TICK_1US;// end RX early
    //u32 new_post_time = clock_time () + 50*SYSTEM_TIMER_TICK_1US;// end RX early
    //u32 new_post_time = clock_time () + 2270*SYSTEM_TIMER_TICK_1US;// 2270us = (251B + 4B(MIC) + 10B)*8 + 150
    //if(new_post_time < (aclConn_param.task_end_tick - (SLOT_PROCESS_MAX_TICK + BOUNDARY_RX_RELOAD_TICK))){
    if(new_post_time < systimer_get_irq_capture()){
        systimer_set_irq_capture(new_post_time);
    }
}


int blc_ll_getLegacyAdvStatus(void)
{
    int status = BLE_SUCCESS;           //0 Means Adv Enable

    status = (bltSche.task_mask & TSKMSK_LEG_ADV) ? 0: 1;
    status |= !blmsParam.leg_adv_en << 1;
    status |= blmsParam.new_conn_forbidden << 2;
    status |= blmsParam.newConn_forbidden_slave << 3;

    return status;
}


void  blt_ll_acl_sniffer_sync_result(u16 syncHandle, u8 *p)
{
    // 0x00, 0x08,
    // 0x3E, 0x07,
    // 0x13, 0x0A,
    if( (p[0] == BLE_SUCCESS) || (p[0] == HCI_ERR_CONN_TIMEOUT) || \
        (p[0] == HCI_ERR_CONN_FAILED_TO_ESTABLISH) || (p[0] == SNIFFER_SEEK_FAIL) || \
        (p[0] == HCI_ERR_REMOTE_USER_TERM_CONN) || (p[0] == SNIFFER_USER_STOP_EFFECTIVE) ){

        u8 sync_result[2];
        acl_sniffer_sync_statusEvt_t *pa = (acl_sniffer_sync_statusEvt_t *)sync_result;

        pa->snifHandle = syncHandle;

        if(p[0] == BLE_SUCCESS){
            pa->status = SNIFFER_SYNC_CREATE;
        }
        else if(p[0] == HCI_ERR_CONN_TIMEOUT){
            pa->status = SNIFFER_TIMEOUT;
        }
        else if(p[0] == HCI_ERR_CONN_FAILED_TO_ESTABLISH){
            pa->status = SNIFFER_ESTABLISH_FAIL;
        }
        else if(p[0] == SNIFFER_SEEK_FAIL){
            pa->status = SNIFFER_SEEK_FAIL;
        }
        else if(p[0] == HCI_ERR_REMOTE_USER_TERM_CONN || p[0] == SNIFFER_USER_STOP_EFFECTIVE){
            pa->status = SNIFFER_USER_STOP_EFFECTIVE;
        }

        blt_p_event_callback (BLT_EV_FLAG_SNIFFER_SYNC_STATUS, sync_result, 2);
    }
}

_attribute_noinline_
int blt_ll_acl_sniffer_main_loop_data(u16 snifHandle, u8 *raw_pkt)
{
//  if(raw_pkt[1] & BLT_ACL_SNIFFER_MASTER_FLAG){
//      printf("sync_OK:%x,%d,%d\n",raw_pkt[2], raw_pkt[1], raw_pkt[3] - 110);
//  }
//  raw_pkt[1] &= BLT_ACL_SNIFFER_CHANNEL_MASK;//for debug

    u8 snif_idx = snifHandle & CONN_IDX_MASK;
    if(blms[snif_idx].connState)
    {
        #if (DEBUG_SNIFFER_REPORT_INSTANT_EN)
            blt_p_event_callback (BLT_EV_FLAG_SNIFFER_RSSI_REPORT, (u8 *)&raw_pkt[1], 4);
        #else
            blt_p_event_callback (BLT_EV_FLAG_SNIFFER_RSSI_REPORT, (u8 *)&raw_pkt[1], 3);
        #endif
    }

    return BLE_SUCCESS;
}

_attribute_no_inline_
void  blt_ll_acl_sniffer_mainloop(void)
{
    if (blt_rxfifo.rptr != blt_rxfifo.wptr){
        while (blt_rxfifo.rptr != blt_rxfifo.wptr)
        {
            wd_clear(); //clear watch dog

            u8 *raw_pkt = (u8 *) (blt_rxfifo.p_base + (blt_rxfifo.rptr & blt_rxfifo.mask) * blt_rxfifo.size);

            if (raw_pkt[2])
            {
                blt_ll_acl_sniffer_main_loop_data(raw_pkt[2], raw_pkt);
            }

            blt_rxfifo.rptr++; //handle rx data overflow in irq to prevent rx_fifo.rptr from re-entering
        }
    }

    if (sniffer_rx_null_fifo.rptr != sniffer_rx_null_fifo.wptr){
        while (sniffer_rx_null_fifo.rptr != sniffer_rx_null_fifo.wptr)
        {
            wd_clear(); //clear watch dog

            u8 *raw_pkt = (u8 *) (sniffer_rx_null_fifo.p + sniffer_rx_null_fifo.rptr * sniffer_rx_null_fifo.size);

            if (raw_pkt[2])
            {
                blt_ll_acl_sniffer_main_loop_data(raw_pkt[2], raw_pkt);
            }

            sniffer_rx_null_fifo.rptr == (sniffer_rx_null_fifo.num - 1) ? sniffer_rx_null_fifo.rptr = 0 : sniffer_rx_null_fifo.rptr++;
        }
    }

    for(int snif_idx = 0; snif_idx < LL_MAX_ACL_CONN_NUM; snif_idx++)
    {
        st_ll_conn_t *pc_conn = (st_ll_conn_t*)&blms[snif_idx];

        if(pc_conn->acl_sniffer_sync_update_ignore){
            pc_conn->acl_sniffer_sync_update_ignore = 0;

            if(pc_conn->connState){
                u8 reason[1];
                reason[0] = BLE_SUCCESS;
                blt_ll_acl_sniffer_sync_result(pc_conn->acl_conHandle, reason);
            }
        }
    }
}

#if (SNIFFER_USE_SOME_COMMON_APIS)
_attribute_ram_code_ int irq_acl_sniffer_rx(void)
{

    acl_connection_role_t role = sniffer_conn.c->role;
    if(role == ACL_ROLE_CENTRAL){
        #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
            pAclSnifferParam = &aclSniffer_mst_param;
        #endif

        #if (DEBUG_SNIFFER_NULL_POINTER_EN)
            if(!blm_pconn){
                printf("err:%s Line:%d\n", __FILE__, __LINE__);
            }
        #endif
    }
    else{
        #if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
            pAclSnifferParam = &aclSniffer_slv_param;
        #endif

        #if (DEBUG_SNIFFER_NULL_POINTER_EN)
            if(!bls_pconn){
                printf("err:%s Line:%d\n", __FILE__, __LINE__);
            }
        #endif
    }

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

        if(blms_state  ==  BLMS_STATE_SNIFM_S || blms_state  ==  BLMS_STATE_SNIFS_S){
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
                if(!pAclSnifferParam->sniffer_rx_num){
                    if(blms_state == BLMS_STATE_SNIFM_S || blms_state == BLMS_STATE_SNIFS_S){
                        #if (LL_ACL_CEN_EN || LL_ACL_PER_EN)
                            if(!sniffer_conn.c->tick_1st_rx){
                                if(blms_pconn->acl_sniffer_sync_creating){
                                    //need to continue monitor the peer-slave
                                    sniffer_conn.c->tick_1st_rx = bltRxPkt.rx_header_tick;
                                    rfLen_1st_rx = raw_pkt[DMA_RFRX_OFFSET_RFLEN];
                                    rssi_1st_rx = raw_pkt[DMA_RFRX_OFFSET_RSSI(raw_pkt)];

                                    DBG_CHN3_TOGGLE;
                                    DBG_CHN3_TOGGLE;
                                }
                                else{
                                    u32 diff;
                                    if(bltRxPkt.rx_header_tick > sniffer_conn.c->expectTimeMark){
                                        diff = bltRxPkt.rx_header_tick - sniffer_conn.c->expectTimeMark;
                                    }
                                    else{
                                        diff = sniffer_conn.c->expectTimeMark - bltRxPkt.rx_header_tick;
                                    }
                                    //printf("us:%d,%d,%d\n", bltRxPkt.rx_header_tick>>4, sniffer_conn.c->expectTimeMark>>4,diff>>4);

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

                                        //monitor the peer-master successful

                                        sniffer_conn.c->tick_1st_rx = bltRxPkt.rx_header_tick;

                                        if(role == ACL_ROLE_CENTRAL){
                                            rfLen_1st_rx = raw_pkt[DMA_RFRX_OFFSET_RFLEN];
                                        }
                                        else{
                                            raw_pkt[1] = BLT_ACL_SNIFFER_MASTER_FLAG;//peer master RSSI
                                            raw_pkt[1] |= blms_pconn->conn_chn;
                                            raw_pkt[2] = blms_pconn->acl_conHandle;
                                            raw_pkt[3] = raw_pkt[DMA_RFRX_OFFSET_RSSI(raw_pkt)];
                                            #if (DEBUG_SNIFFER_REPORT_INSTANT_EN)
                                                raw_pkt[4] = blms_pconn->conn_inst;
                                            #endif

                                            pAclSnifferParam->sniffer_rssi_validFlag = 1;
                                            next_buffer = 1;
                                        }

                                        DBG_CHN3_TOGGLE;
                                        DBG_CHN3_TOGGLE;
                                    }
                                    
                                    if(role == ACL_ROLE_PERIPHERAL){
                                        blt_sniffer_stop_rx_window(50);
                                    }
                                }
                            }
                        #endif
                    }
                }
                else if(pAclSnifferParam->sniffer_rx_num == 1)
                {
                    if(sniffer_conn.c->tick_1st_rx && ((role == ACL_ROLE_CENTRAL) ? 1 : blms_pconn->acl_sniffer_sync_creating)){
                        u32 diff = bltRxPkt.rx_header_tick - sniffer_conn.c->tick_1st_rx;

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

                            //record peer-device RSSI
                            raw_pkt[1] =  (role == ACL_ROLE_CENTRAL) ? BLT_ACL_SNIFFER_SLAVE_FLAG : BLT_ACL_SNIFFER_MASTER_FLAG;//M: peer-slave RSSI, S: peer master RSSI
                            raw_pkt[1] |= blms_pconn->conn_chn;
                            raw_pkt[2] = blms_pconn->acl_conHandle;
                            raw_pkt[3] = (role == ACL_ROLE_CENTRAL) ? raw_pkt[DMA_RFRX_OFFSET_RSSI(raw_pkt)] : rssi_1st_rx;
                            #if (DEBUG_SNIFFER_REPORT_INSTANT_EN)
                                raw_pkt[4] = blms_pconn->conn_inst;
                            #endif
                            pAclSnifferParam->sniffer_rssi_validFlag = 1;

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

        pAclSnifferParam->sniffer_rx_num ++;  //care CRC
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


_attribute_ram_code_ int blt_sniffer_start(int conn_idx)
{
    blms_start_pre_process(conn_idx);

    if(blms_conn_sel < LL_MAX_ACL_CEN_NUM){
        #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
            sniffer_conn.m = blm_pconn = (st_llm_conn_t *)&blmsMaster[blms_conn_sel];
            sniffer_conn.c->role = ACL_ROLE_CENTRAL;
            aclSniffer_mst_param.sniffer_rssi_validFlag = 0;
            aclSniffer_mst_param.sniffer_rx_num = 0;
            blms_state = BLMS_STATE_SNIFM_S;
            systick_irq_trigger = SYS_IRQ_TRIG_BTX_POST;  //will set reg_system_tick_irq for task_post immediately
        #endif
    }
    else if(blms_conn_sel < LL_MAX_ACL_CONN_NUM){
        #if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
            sniffer_conn.s = bls_pconn = (st_lls_conn_t *)&blmsSlave[blms_conn_sel - LL_MAX_ACL_CEN_NUM];//pay attention here
            sniffer_conn.c->role = ACL_ROLE_PERIPHERAL;
            aclSniffer_slv_param.sniffer_rssi_validFlag = 0;
            aclSniffer_slv_param.sniffer_rx_num = 0;
            blms_state = BLMS_STATE_SNIFS_S;
            systick_irq_trigger = SYS_IRQ_TRIG_BRX_POST;  //will set reg_system_tick_irq for task_post immediately
        #endif
    }
#if (DEBUG_SNIFFER_NULL_POINTER_EN)
    else{
        printf("err:%s Line:%d\n", __FILE__, __LINE__);
    }
#endif

    blms_start_common_1(blms_pconn);

    rf_set_rxmode();

    if( aclConn_param.connSync & (1<<blms_conn_sel) ){
        rf_set_1st_rx_timeout(0xffffff);
    }
    else{
        rf_set_1st_rx_timeout(300 + sniffer_conn.c->conn_tolerance_us*2 + bltPHYs.prmb_ac_us);
    }

    rf_ble_set_rx_settle(RX_SETTLE_US);

    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_RX_ON);  }

    //these logic setting executing after RX setting to save time
    sniffer_conn.c->timing_update = 0;
    sniffer_conn.c->tick_1st_rx = 0;
    sniffer_conn.c->sSlot_mark_conn = bltSche.sSlot_idx_irq_real;
    sniffer_conn.c->sSlot_shift_tor = sniffer_conn.c->conn_tolerance_us*SSLOT_US_REVERSE;
    sniffer_conn.c->expectTimeMark = sniffer_conn.c->connExpectTime = bltSche.sSlot_tick_irq + BRX_LEFT_EARLY_TICK + sniffer_conn.c->conn_tolerance_us * SYSTEM_TIMER_TICK_1US;
    #if (0)//(BLMS_PM_ENABLE)
        if(blmsPm.slave_idx_calib == 0xFF){
            blmsPm.slave_idx_calib = blms_conn_sel;
        }
    #endif

    blms_start_common_2(blms_pconn);

    return BLE_SUCCESS;
}


_attribute_ram_code_ int blt_sniffer_post(void)
{

    acl_connection_role_t role = sniffer_conn.c->role;

    if(blmsParam.rf_fsm_busy) {
        blmsParam.rf_fsm_busy = 0;

        rf_set_tx_rx_off();
        STOP_RF_STATE_MACHINE;  //stop state machine
        CLEAR_ALL_RFIRQ_STATUS;
    }

    if(blms_pconn->acl_sniffer_sync_creating){
        // clear tick_1st_rx
        sniffer_conn.c->tick_1st_rx = 0;
    }

    int brx_sync = blms_pconn->sync_timing;

    blms_state = (role == ACL_ROLE_CENTRAL) ? BLMS_STATE_SNIFM_E : BLMS_STATE_SNIFS_E;

    if ( blms_post_common_1(blms_pconn) ){  // return 1: terminate happens
        if(role == ACL_ROLE_CENTRAL){
            blmsParam.cur_master_num --;
        }
        else{
            blmsParam.cur_slave_num --;
        }
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
        blt_ll_acl_conn_sync_process(sniffer_conn.c->tick_1st_rx);

        /* value below are invalid when old conn_interval change to new conn_interval */
        //sniffer_conn.c->connExpectTime = (sniffer_conn.c->tick_1st_rx ? sniffer_conn.c->tick_1st_rx : blms_pconn->conn_tick_mark) + blms_pconn->conn_intvl_tick;
        if(sniffer_conn.c->tick_1st_rx){
            sniffer_conn.c->expectTimeMark = sniffer_conn.c->tick_1st_rx;
            sniffer_conn.c->conn_offset_tick = sniffer_conn.c->connExpectTime - sniffer_conn.c->tick_1st_rx;
            sniffer_conn.c->connExpectTime = sniffer_conn.c->tick_1st_rx + blms_pconn->conn_intvl_tick;

            blt_debug_gpio_toggle_acl_sniffer();
            blt_debug_gpio_toggle_acl_sniffer();
        }
        else{
            sniffer_conn.c->connExpectTime += blms_pconn->conn_intvl_tick;
        }


        /* update sync status according to RX packet receiving result */
        if(sniffer_conn.c->tick_1st_rx)
        {
            if(blms_pconn->sync_timing)
            {
                blms_pconn->sync_timing = 0;
                sniffer_conn.c->conn_tolerance_us = blmsParam.min_tolerance_us;
                sniffer_conn.c->conn_start_time = sniffer_conn.c->connExpectTime - BRX_LEFT_EARLY_TICK - sniffer_conn.c->conn_tolerance_us * SYSTEM_TIMER_TICK_1US;
                int n_sSlot = TICKS_DUR_2_SSLOT_DUR(sniffer_conn.c->conn_start_time - bltSche.sSlot_tick_irq_real);
                sniffer_conn.c->sSlot_mark_conn = bltSche.sSlot_idx_irq_real + n_sSlot - sniffer_conn.c->sSlot_interval;

                sniffer_conn.c->sSlot_offset = 0;
                sniffer_conn.c->timing_update = 1;
                blt_sche_addUpdate(SLOT_UPDT_SLAVE_SYNC_DONE);

                #if (0)//(BLMS_PM_ENABLE)
                    blmsPm.slave_no_sleep &= ~(1<<blms_conn_sel);
                #endif
            }
            else{
                if(sniffer_conn.c->conn_tolerance_us > blmsParam.min_tolerance_us){
                    sniffer_conn.c->conn_tolerance_us = blmsParam.min_tolerance_us;
                    sniffer_conn.c->conn_start_time = sniffer_conn.c->connExpectTime - BRX_LEFT_EARLY_TICK - sniffer_conn.c->conn_tolerance_us * SYSTEM_TIMER_TICK_1US;
                    int n_sSlot = TICKS_DUR_2_SSLOT_DUR(sniffer_conn.c->conn_start_time - bltSche.sSlot_tick_irq_real);
                    sniffer_conn.c->sSlot_mark_conn = bltSche.sSlot_idx_irq_real + n_sSlot - sniffer_conn.c->sSlot_interval;

                    sniffer_conn.c->sSlot_offset = 0;
                    sniffer_conn.c->timing_update = 1;
                    blt_sche_addUpdate(SLOT_UPDT_SLAVE_SYNC_DONE);
                }
                else{
                    u32 tick_offset_1st_rx = sniffer_conn.c->tick_1st_rx - bltSche.sSlot_tick_irq_real;
                    u32 tick_offset_expect = blmsParam.min_tolerance_us * SYSTEM_TIMER_TICK_1US + BRX_LEFT_EARLY_TICK;
                    sniffer_conn.c->sSlot_offset = TICKS_DUR_2_SSLOT_DUR(tick_offset_1st_rx - tick_offset_expect);

                    #if (BRX_HALF_MARGIN_SSLOT_NUM == 3)
                        if(sniffer_conn.c->sSlot_offset < -1 || sniffer_conn.c->sSlot_offset > 2)
                    #elif (BRX_HALF_MARGIN_SSLOT_NUM == 2)
                        if(sniffer_conn.c->sSlot_offset < 0 || sniffer_conn.c->sSlot_offset > 1)
                    #else
                        #error "add code here"
                    #endif
                        {
                            sniffer_conn.c->conn_tolerance_us = blmsParam.min_tolerance_us;
                            sniffer_conn.c->timing_update = 1;
                            blt_sche_addUpdate(SLOT_UPDT_SLAVE_SSLOT_ADJUST);
                        }
                }
            }

            if(sniffer_conn.c->timing_update){
                sniffer_conn.c->sSlot_shift_tor = blmsParam.min_tolerance_us*SSLOT_US_REVERSE;
                /* tolerance*2/19.53 = tolerances/10 */
                #if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
                    blms_pconn->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + pdu_27b_tifs_27b_sslot[blms_pconn->connPhyCtrl.conn_cur_phy - 1][blms_pconn->crypt.enable] + sniffer_conn.c->conn_tolerance_us/10;
                #else
                    blms_pconn->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + pdu_27b_tifs_27b_sslot[0][blms_pconn->crypt.enable] + sniffer_conn.c->conn_tolerance_us/10;
                #endif
            }
        }

        #if (BLMS_PM_ENABLE)
            if(brx_sync){
                if(role == ACL_ROLE_CENTRAL){
                    sniffer_conn.c->rx_pkt_miss = 0;
                }
                else{
                    #if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
                        blt_brx_timing_init();
                    #endif
                }
            }
            else{
                if(role == ACL_ROLE_CENTRAL)
                {
                    if(sniffer_conn.c->tick_1st_rx) {
                        sniffer_conn.c->rx_pkt_miss = 0;
                        blms_pconn->pm_error_us = 0;
                        sniffer_conn.c->sleep_32k_rc = 0;
                    }
                    else {
                        if (sniffer_conn.c->rx_pkt_miss < 5){
                            sniffer_conn.c->rx_pkt_miss ++;
                        }
                    }
                }
                else //if (role == ACL_ROLE_PERIPHERAL)
                {
                    #if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
                        blt_brx_timing_update ();
                    #endif
                }
            }

            sniffer_conn.c->slave_sleep_flg = 0;
            sniffer_conn.c->tick_last_1st_rx = sniffer_conn.c->tick_1st_rx;
        #endif

        u8 rx_null_flag = 0;
        if(role == ACL_ROLE_CENTRAL)
        {
            #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
                if(aclSniffer_mst_param.sniffer_rssi_reportType == RSSI_TYPE_ALL){
                    if(!aclSniffer_mst_param.sniffer_rssi_validFlag){
                        rx_null_flag = 1;
                    }
                }
            #endif
        }
        else //if (role == ACL_ROLE_PERIPHERAL)
        {
            #if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
            if(aclSniffer_slv_param.sniffer_rssi_reportType == RSSI_TYPE_ALL){
                if(!aclSniffer_slv_param.sniffer_rssi_validFlag){
                    rx_null_flag = 1;
                }
            }
            #endif
        }

        if(rx_null_flag){
            u8 *raw_pkt = (u8 *) (sniffer_rx_null_fifo.p + sniffer_rx_null_fifo.wptr * sniffer_rx_null_fifo.size);

            raw_pkt[1] = blms_pconn->conn_chn;
            raw_pkt[2] = blms_pconn->acl_conHandle;
            raw_pkt[3] = 0;//invalid RSSI
            #if (DEBUG_SNIFFER_REPORT_INSTANT_EN)
                raw_pkt[4] = blms_pconn->conn_inst - 1;
            #endif

            sniffer_rx_null_fifo.wptr == (sniffer_rx_null_fifo.num - 1) ? sniffer_rx_null_fifo.wptr = 0 : sniffer_rx_null_fifo.wptr++;
        }
    }

    blms_post_common_2();

    return BLE_SUCCESS;
}


_attribute_ram_code_ int blt_ll_sniffer_seek_anchor(u8 *cmd)
{
    acl_sniffer_sync_param_t* param = (acl_sniffer_sync_param_t*)cmd;

    blms_conn_sel = param->sync_connHandle & CONN_IDX_MASK;

    volatile int snif_seek_sel;
    u32 role = ACL_ROLE_PERIPHERAL;
    u8 snif_1st_rxWindowMaxFlag = 1;
    u8 snif_seek_halfWindow_ms = 20;

    if(blms_conn_sel < LL_MAX_ACL_CEN_NUM){
        #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
            role = ACL_ROLE_CENTRAL;
            snif_seek_sel = blms_conn_sel;
            pSnifferSeek = (acl_sniffer_seek_param_t *)&aclSniffer_mst_seek[snif_seek_sel];

            snif_1st_rxWindowMaxFlag = aclSniffer_mst_param.sniffer_1st_rxWindowMaxFlag;
            snif_seek_halfWindow_ms = aclSniffer_mst_param.sniffer_seek_halfWindow_ms;
        #endif
    }
    else if(blms_conn_sel < LL_MAX_ACL_CONN_NUM){
        #if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
            role = ACL_ROLE_PERIPHERAL;
            snif_seek_sel = blms_conn_sel - LL_MAX_ACL_CEN_NUM; //pay attention here
            pSnifferSeek = (acl_sniffer_seek_param_t *)&aclSniffer_slv_seek[snif_seek_sel];

            snif_1st_rxWindowMaxFlag = aclSniffer_slv_param.sniffer_1st_rxWindowMaxFlag;
            snif_seek_halfWindow_ms = aclSniffer_slv_param.sniffer_seek_halfWindow_ms;
        #endif
    }
#if (DEBUG_SNIFFER_NULL_POINTER_EN)
    else{
        printf("err:%s Line:%d\n", __FILE__, __LINE__);
    }
#endif

    smemcpy((u8*)&pSnifferSeek->sync.sync_connHandle, (u8*)&param->sync_connHandle, sizeof(acl_sniffer_sync_param_t));

    smemcpy(pSnifferSeek->sync.sync_conn_chm, param->sync_conn_chm, 5);
    #if (LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2)
        if(pSnifferSeek->sync.sync_conn_chnSel)
        {
            pSnifferSeek->seek_chnIdentifier = (pSnifferSeek->sync.sync_accessAddr>>16) ^ (pSnifferSeek->sync.sync_accessAddr&0xffff);
            csa2_calculateMapInfo(&pSnifferSeek->chnParam);
        }
        else
        {
            blt_csa1_calculateChannelTable (pSnifferSeek->sync.sync_conn_chm, pSnifferSeek->sync.sync_conn_hop, pSnifferSeek->chnParam.rempChmTbl);
        }
    #else
        blt_csa1_calculateChannelTable (pSnifferSeek->sync.sync_conn_chm, pSnifferSeek->sync.sync_conn_hop, pSnifferSeek->chnParam.rempChmTbl);
    #endif

    pSnifferSeek->tick_seek_interval = param->sync_intvl_n_1m25 * SYSTEM_TIMER_TICK_1250US;
    pSnifferSeek->sSlot_seek_interval = TICKS_DUR_2_SSLOT_DUR(pSnifferSeek->tick_seek_interval);

    // seek_tolerance cannot be greater than bltSche.bSlot_maxLen/2
    if(snif_1st_rxWindowMaxFlag){
        pSnifferSeek->tick_seek_tolerance_ms = pSnifferSeek->tick_seek_tolerance_max_ms;
    }
    else{
        pSnifferSeek->tick_seek_tolerance_ms = snif_seek_halfWindow_ms;
    }
    pSnifferSeek->sSlot_seek_tolerance = TICKS_DUR_2_SSLOT_DUR(pSnifferSeek->tick_seek_tolerance_ms * SYSTEM_TIMER_TICK_1MS);

    if(pSnifferSeek->sync.sync_expectTime < bltSche.sSlot_tick_irq_real){
        u16 jump_num;
        jump_num = (bltSche.sSlot_tick_irq_real - pSnifferSeek->sync.sync_expectTime) / pSnifferSeek->tick_seek_interval + 1;
        pSnifferSeek->sync.sync_expectTime += pSnifferSeek->tick_seek_interval*jump_num;
        pSnifferSeek->sync.sync_conn_inst += jump_num;
        pSnifferSeek->sync.sync_chn_idx = ((u32)(pSnifferSeek->sync.sync_chn_idx + jump_num))%37;
    }

    int n_sSlot = TICKS_DUR_2_SSLOT_DUR(pSnifferSeek->sync.sync_expectTime - bltSche.sSlot_tick_irq_real);
    pSnifferSeek->sSlot_sync_expectTime = bltSche.sSlot_idx_irq_real + n_sSlot;
    pSnifferSeek->seek_count = 0;
    pSnifferSeek->seek_state = 0;
    pSnifferSeek->seeking = 1;
    pSnifferSeek->seek_stop = 0;
    pSnifferSeek->tick_seek_start = clock_time() | 1;

    if(role == ACL_ROLE_CENTRAL){
        blt_ll_setSchedulerTaskPriority( TSKOFT_SNIFM_SEEK + snif_seek_sel, TASK_PRIORITY_MAX );
        blt_sche_addTaskMask(TSKMSK_SNIFM_SEEK_0 << snif_seek_sel);
    }
    else{
        blt_ll_setSchedulerTaskPriority( TSKOFT_SNIFS_SEEK + snif_seek_sel, TASK_PRIORITY_MAX );
        blt_sche_addTaskMask(TSKMSK_SNIFS_SEEK_0 << snif_seek_sel);
    }

    blt_sche_addUpdate(SLOT_UPDT_SNIF_SEEK_CREATE);

    return BLE_SUCCESS;
}


_attribute_ram_code_ int blt_ll_buildAclSnifferSeekSchLinklist(u8 role)
{
    u32 i,j;

    u32 tsk_num_max;
    u64 tsk_mask_base;//u32 => u64, bltSche.task_mask use u64

    if(role == ACL_ROLE_CENTRAL){
        #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
            tsk_num_max = TSKNUM_SNIFM_SEEK;
            tsk_mask_base = TSKMSK_SNIFM_SEEK_0;
        #endif
    }
    else{
        #if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
            tsk_num_max = TSKNUM_SNIFS_SEEK;
            tsk_mask_base = TSKMSK_SNIFS_SEEK_0;
        #endif
    }

    for( i = 0;i < tsk_num_max;i++ )
    {
        if( bltSche.task_mask & (tsk_mask_base<<i) )
        {
            if(role == ACL_ROLE_CENTRAL){
                #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
                    pSnifferSeek = (acl_sniffer_seek_param_t *)&aclSniffer_mst_seek[i];
                #endif
            }
            else{
                #if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
                    pSnifferSeek = (acl_sniffer_seek_param_t *)&aclSniffer_slv_seek[i];
                #endif
            }

            if(bltSche.sSlot_idx_reset == 1 && bltSche.build_index == 0){
                pSnifferSeek->sSlot_sync_expectTime -= bltSche.sSlot_idx_past;
            }

            s32 sSlot_first_seek_window_tail = pSnifferSeek->sSlot_sync_expectTime + pSnifferSeek->sSlot_seek_tolerance;
            s32 sSlot_sche_start = bltSche.sSlot_idx_next;
            u16 first_seek_jump_num = 0;

            if(sSlot_first_seek_window_tail < sSlot_sche_start){
                first_seek_jump_num = (sSlot_sche_start - sSlot_first_seek_window_tail) / pSnifferSeek->sSlot_seek_interval + 1;
            }

            s32 sSlot_last_seek_window_head = pSnifferSeek->sSlot_sync_expectTime - pSnifferSeek->sSlot_seek_tolerance;
            s32 sSlot_sche_end = bltSche.sSlot_endIdx_dft;  // 126 -> 78.75ms
            u16 last_seek_jump_num = 0;

            if(sSlot_last_seek_window_head < sSlot_sche_end){
                last_seek_jump_num = (sSlot_sche_end - sSlot_last_seek_window_head) / pSnifferSeek->sSlot_seek_interval;
            }

            u16 jump_num_min = min(first_seek_jump_num, last_seek_jump_num);
            u16 jump_num_max = max(first_seek_jump_num, last_seek_jump_num);
            pSnifferSeek->jump_num_valid = 0;

            s32 sSlot_task_duration_max = 0;
            for(j = jump_num_min; j < jump_num_max; j++)
            {
                s32 sSlot_task_start = pSnifferSeek->sSlot_sync_expectTime + pSnifferSeek->sSlot_seek_interval * j - pSnifferSeek->sSlot_seek_tolerance;
                if(sSlot_task_start > sSlot_sche_end){
                    break;
                }
                s32 sSlot_task_end = pSnifferSeek->sSlot_sync_expectTime + pSnifferSeek->sSlot_seek_interval * j + pSnifferSeek->sSlot_seek_tolerance;
                s32 sSlot_task_duration_cur = min(sSlot_task_end, sSlot_sche_end) - max(sSlot_task_start, sSlot_sche_start);// less than seek_tolerance*2

                if(sSlot_task_duration_cur > sSlot_task_duration_max){
                    sSlot_task_duration_max = sSlot_task_duration_cur;
                    pSnifferSeek->jump_num_valid = j;
                    if(sSlot_task_duration_max >= (pSnifferSeek->sSlot_seek_tolerance << 1)){
                        break;
                    }
                }
            }

            pSnifferSeek->sSlot_seek_expectTime = pSnifferSeek->sSlot_sync_expectTime + pSnifferSeek->sSlot_seek_interval * pSnifferSeek->jump_num_valid;

            s32 sSlot_seek_window_head;
            sSlot_seek_window_head = pSnifferSeek->sSlot_seek_expectTime - pSnifferSeek->sSlot_seek_tolerance;
            if(sSlot_seek_window_head >= sSlot_sche_end){
                continue;
            }
            if(sSlot_seek_window_head < sSlot_sche_start + 2){
                sSlot_seek_window_head = sSlot_sche_start + 2;
            }

            s32 sSlot_seek_window_tail;
            sSlot_seek_window_tail = pSnifferSeek->sSlot_seek_expectTime + pSnifferSeek->sSlot_seek_tolerance;
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
                    BLMS_ERR_DEBUG(SCH_DEBUG_EN, 0xEE030000);
                #endif
                continue;
            }

            //pSnifferSeek->sSlot_seek_duration = (u32)(sSlot_seek_window_tail - sSlot_seek_window_head);
            pSnifferSeek->sSlot_seek_duration = sSlot_seek_window_tail - sSlot_seek_window_head;
            if(pSnifferSeek->sSlot_seek_duration < pSnifferSeek->sSlot_seek_tolerance){
                //actual seek duration less than setting seek half window
                continue;
            }

            sch_task_t  *pCur_schTask = (sch_task_t *)&pSnifferSeek->snifferTsk_fifo;

            pCur_schTask->begin = sSlot_seek_window_head;
            pCur_schTask->end = sSlot_seek_window_tail;

            blt_ll_addTask2ExistLinklist(pCur_schTask, 1);

            break;//currently only one seek task is assigned
        }
    }

    return BLE_SUCCESS;
}


_attribute_ram_code_ int blt_sniffer_seek_start (int seek_idx, u8 role)
{
    u8 snif_idx = 0;

    //pay attention here, seek_idx 0~3(TSKNUM_SNIFM_SEEK/TSKNUM_SNIFS_SEEK-1)
    if(role == ACL_ROLE_CENTRAL){
        #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
            snif_idx = seek_idx;
            blm_pconn = (st_llm_conn_t *)&blmsMaster[seek_idx];
            pSnifferSeek = (acl_sniffer_seek_param_t *)&aclSniffer_mst_seek[seek_idx];
            pAclSnifferParam = &aclSniffer_mst_param;

            extern acl_sniffer_seek_param_t *pSnifferMstSeek;
            pSnifferMstSeek = pSnifferSeek;

            blms_state = BLMS_STATE_SNIFM_SEEK_S;
            systick_irq_trigger = SYS_IRQ_TRIG_SNIFM_SEEK_POST;
        #endif

        #if (DEBUG_SNIFFER_NULL_POINTER_EN)
            if(!blm_pconn){
                printf("err:%s Line:%d\n", __FILE__, __LINE__);
            }
        #endif
    }
    else{
        #if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
            snif_idx = seek_idx + LL_MAX_ACL_CEN_NUM;
            bls_pconn = (st_lls_conn_t *)&blmsSlave[seek_idx];
            pSnifferSeek = (acl_sniffer_seek_param_t *)&aclSniffer_slv_seek[seek_idx];
            pAclSnifferParam = &aclSniffer_slv_param;

            extern acl_sniffer_seek_param_t *pSnifferSlvSeek;
            pSnifferSlvSeek = pSnifferSeek;

            bls_conn_sel = seek_idx;

            blms_state = BLMS_STATE_SNIFS_SEEK_S;
            systick_irq_trigger = SYS_IRQ_TRIG_SNIFS_SEEK_POST;
        #endif

        #if (DEBUG_SNIFFER_NULL_POINTER_EN)
            if(!bls_pconn){
                printf("err:%s Line:%d\n", __FILE__, __LINE__);
            }
        #endif
    }

    #if (DEBUG_SNIFFER_NULL_POINTER_EN)
        if(!pSnifferSeek){
            printf("err:%s Line:%d\n", __FILE__, __LINE__);
        }

        if(!pAclSnifferParam){
            printf("err:%s Line:%d\n", __FILE__, __LINE__);
        }
    #endif

    blms_start_pre_process(snif_idx);
    blt_debug_gpio_toggle_acl_sniffer();
    blt_debug_gpio_toggle_acl_sniffer();

    #if (DEBUG_SNIFFER_NULL_POINTER_EN)
        if(!blms_pconn){
            printf("err:%s Line:%d\n", __FILE__, __LINE__);
        }
    #endif

    pAclSnifferParam->sniffer_rx_num = 0;
    pAclSnifferParam->sniffer_seek_tick_1st_rx = 0;

    //reset_baseband(); //QiangKai: Eagle can not reset, all RF baseband setting will lost(But Kite/Vulture must add this)

    ble_rf_set_rx_dma((u8*)aclConn_param.acl_rx_dma_buff, aclConn_param.acl_rx_dma_size);
    u16 rx_max_len = blt_llms_get_connEffectiveMaxRxOctets_by_connIdx(blms_conn_sel);
    rf_set_rx_maxlen(rx_max_len+4);//MCI 4ytes

    u8 seek_chn;
    u8 seek_chn_idx = ((u32)(pSnifferSeek->sync.sync_chn_idx + pSnifferSeek->jump_num_valid))%37;
    u16 seek_inst = pSnifferSeek->sync.sync_conn_inst + pSnifferSeek->jump_num_valid;
    #if (LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2)
        if(pSnifferSeek->sync.sync_conn_chnSel)
        {
            seek_chn = ll_chn_index_calc_cb(&pSnifferSeek->chnParam, seek_inst, pSnifferSeek->seek_chnIdentifier);
        }
        else
        {
            seek_chn = pSnifferSeek->chnParam.rempChmTbl[seek_chn_idx];
        }
    #else
        seek_chn = pSnifferSeek->chnParam.rempChmTbl[seek_chn_idx];
    #endif

    #if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
        if(ll_phy_switch_cb){
            u8 sync_CI_phy = pSnifferSeek->sync.sync_CI_phy;//0~3bits: le_phy_type_t; 4~7bits: le_coding_ind_t
            ll_phy_switch_cb(sync_CI_phy & 0x0F, (sync_CI_phy & 0xF0) >> 4); //rf_ble_switch_phy
        }
    #endif

    rf_set_ble_channel(seek_chn);
    rf_set_ble_access_code((u8 *)&pSnifferSeek->sync.sync_accessAddr);
    rf_set_ble_crc_value(pSnifferSeek->sync.sync_crcInit);
    CLEAR_ALL_RFIRQ_STATUS;

    rf_set_rxmode();

    rf_set_1st_rx_timeout(0xffffff);
    rf_ble_set_rx_settle(RX_SETTLE_US);

    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_RX_ON);  }

    aclConn_param.task_end_tick = bltSche.sSlot_tick_irq_real + SSLOT_DUR_2_TICKS_DUR(pSnifferSeek->sSlot_seek_duration);

    systimer_set_irq_capture(aclConn_param.task_end_tick - (SLOT_PROCESS_MAX_TICK + BOUNDARY_RX_RELOAD_TICK));

    return BLE_SUCCESS;
}


_attribute_ram_code_ int blt_sniffer_seek_post(u8 role)
{
    if(blmsParam.rf_fsm_busy) {
        blmsParam.rf_fsm_busy = 0;

        rf_set_tx_rx_off();
        STOP_RF_STATE_MACHINE;  //stop state machine
        CLEAR_ALL_RFIRQ_STATUS;
    }

    if(role == ACL_ROLE_CENTRAL){
        #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
            blms_state = BLMS_STATE_SNIFM_SEEK_E;
        #endif
    }
    else{
        #if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
            blms_state = BLMS_STATE_SNIFS_SEEK_E;
        #endif
    }

    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_OFF);  }

    pSnifferSeek->seek_count++;

    if((pSnifferSeek->seek_count >= pAclSnifferParam->sniffer_seek_count_max) && (!pSnifferSeek->seek_state)){
        //reach seek max count
        #if (0)
            pSnifferSeek->seek_state = 2;//Fail
        #else
            if(pSnifferSeek->seek_repeat_type){
                pSnifferSeek->seek_repeat_count++;
                if(pSnifferSeek->seek_repeat_count < 4){
                    if(pSnifferSeek->seek_repeat_count < 3){
                        if(pSnifferSeek->seek_repeat_type != ACL_STATUS_CONNECTION_SETUP){
                            pSnifferSeek->sync.sync_conn_inst--;
                            if(pSnifferSeek->sync.sync_chn_idx == 0){
                                pSnifferSeek->sync.sync_chn_idx = 36;
                            }
                            else{
                                pSnifferSeek->sync.sync_chn_idx--;
                            }
                        }
                        pSnifferSeek->sync.sync_expectTime -= pSnifferSeek->tick_seek_interval;
                        pSnifferSeek->sSlot_sync_expectTime -= pSnifferSeek->sSlot_seek_interval;
                    }
                    else if(pSnifferSeek->seek_repeat_count == 3){
                        if(pSnifferSeek->seek_repeat_type != ACL_STATUS_CONNECTION_SETUP){
                            pSnifferSeek->sync.sync_conn_inst += 3;
                            pSnifferSeek->sync.sync_chn_idx = ((u32)(pSnifferSeek->sync.sync_chn_idx + 3))%37;
                        }
                        pSnifferSeek->sync.sync_expectTime += pSnifferSeek->tick_seek_interval * 3;
                        pSnifferSeek->sSlot_sync_expectTime += pSnifferSeek->sSlot_seek_interval * 3;
                    }
                    pSnifferSeek->seek_count = 0;
                }
                else{
                    pSnifferSeek->seek_state = 2;//Fail
                }
            }
            else{
                pSnifferSeek->seek_state = 2;//Fail
            }
        #endif
    }

    if(pSnifferSeek->seek_stop == 1){
        pSnifferSeek->seek_state = 3;//report stop event
    }
    else if(pSnifferSeek->seek_stop == 2){
        pSnifferSeek->seek_state = 4;//not report stop event
    }

    if(pSnifferSeek->seek_state){
        pSnifferSeek->seeking = 0;
        pSnifferSeek->seek_stop = 0;
        pSnifferSeek->tick_seek_start = 0;

        if(role == ACL_ROLE_CENTRAL){
            blt_sche_removeTaskMask(TSKMSK_SNIFM_SEEK_0<<blms_conn_sel);  //pay attention here
        }
        else{
            blt_sche_removeTaskMask(TSKMSK_SNIFS_SEEK_0<<bls_conn_sel);  //pay attention here
        }

        if(pSnifferSeek->seek_state == 1){//Succeed
            blt_debug_gpio_toggle_acl_sniffer();
            blt_debug_gpio_toggle_acl_sniffer();

            if(role == ACL_ROLE_CENTRAL){
                #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
                    blt_sniffer_mst_seek2sync();
                #endif
            }
            else{
                #if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
                    blt_sniffer_slv_seek2sync();
                #endif
            }
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


_attribute_ram_code_ int irq_acl_sniffer_seek_rx(void)
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
            if(!pAclSnifferParam->sniffer_rx_num){
                if(!pAclSnifferParam->sniffer_seek_tick_1st_rx){
                    //need to continue monitor the peer-slave
                    pAclSnifferParam->sniffer_seek_tick_1st_rx = bltRxPkt.rx_header_tick;
                    seek_rfLen_1st_rx = raw_pkt[DMA_RFRX_OFFSET_RFLEN];

                    //remote:  |1st_timing|<---T_IFS--->|2st_timing|<---MARGIN--->
                    u32 pkt_receive_cost_us = blt_phy_getRfPacketTime_us(seek_rfLen_1st_rx, bltPHYs.cur_llPhy, bltPHYs.cur_peer_CI);
                    pkt_receive_cost_us += BLE_T_IFS;
                    pkt_receive_cost_us += blt_phy_getRfPacketTime_us(255, bltPHYs.cur_llPhy, LE_CODED_S8);//the length is considered the maximum
                    pkt_receive_cost_us += SNIFFER_SEEK_RX_POST_MARGIN_US;

                    u32 seek_post_tick = pAclSnifferParam->sniffer_seek_tick_1st_rx + pkt_receive_cost_us * SYSTEM_TIMER_TICK_1US;
                    if((seek_post_tick < systimer_get_irq_capture()) && (seek_post_tick > (clock_time () + 50*SYSTEM_TIMER_TICK_1US))){
                        systimer_set_irq_capture(seek_post_tick);
                    }

                    DBG_CHN3_TOGGLE;
                    DBG_CHN3_TOGGLE;
                }
            }
            else if(pAclSnifferParam->sniffer_rx_num == 1)
            {
                if(pAclSnifferParam->sniffer_seek_tick_1st_rx){
                    u32 diff = bltRxPkt.rx_header_tick - pAclSnifferParam->sniffer_seek_tick_1st_rx;

                    //e.g.: For 1M: 10 Byte = 1B(preamble) + 4B(accesscode) + 2B(header) + 3B(CRC), 150 is T_IFS

                    //remote:  |1st_timing|<---T_IFS--->|2st_timing|
                    //local:   |rx_head_tick + 1st_timing + T_IFS
                    u32 diff_ideal = (blt_phy_getRfPacketTime_us(seek_rfLen_1st_rx, bltPHYs.cur_llPhy, bltPHYs.cur_peer_CI) + BLE_T_IFS) * SYSTEM_TIMER_TICK_1US;

                    // T_IFS within 20us
                    if((diff > (diff_ideal - 20 * SYSTEM_TIMER_TICK_1US)) && (diff < (diff_ideal + 20 * SYSTEM_TIMER_TICK_1US))){
                        //monitor the peer-slave successful
                        pSnifferSeek->seek_state = 1;//Succeed

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

        pAclSnifferParam->sniffer_rx_num ++;  //care CRC
    }

    return BLE_SUCCESS;
}
#endif


int blc_ll_updateAclSnifferSync(u8 *cmd)//cmd - ACL sniffer sync command: refer to 'acl_sniffer_sync_param_t'.
{
    int err = SNIFFER_UNKNOWN_SNIFHANDLE;
    acl_sniffer_sync_param_t* param = (acl_sniffer_sync_param_t*)cmd;

    if(param->sync_connHandle & BLT_ACL_SNIFFER_MASTER_FLAG){  //master connection
        #if(LL_RSSI_SNIFFER_MASTER_ENABLE)
            err = blc_ll_updateAclSnifferMstSync((u8*)cmd); //sniffer master
        #endif
    }
    else if(param->sync_connHandle & BLT_ACL_SNIFFER_SLAVE_FLAG){
        #if(LL_RSSI_SNIFFER_SLAVE_ENABLE)
            err = blc_ll_updateAclSnifferSlvSync((u8*)cmd); //sniffer slave
        #endif
    }

    return err;
}


void blt_ll_acl_everyConnEvent(u16 connHandle, st_ll_conn_t* pc)
{
    u8  conn_idx = connHandle & CONN_IDX_MASK;

    if(pc->conn_established_tick){
        u8 conn_evt_result[8];
        acl_every_conn_eventEvt_t *pa = (acl_every_conn_eventEvt_t *)conn_evt_result;

        u32 r = irq_disable();
        if(conn_idx < LL_MAX_ACL_CEN_NUM){
            #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
                st_llm_conn_t  *pm = (st_llm_conn_t *) &blmsMaster[conn_idx];
                pa->connExpectTime = pm->tick_conn_expect;
            #else
                irq_restore(r);
                return;
            #endif
        }
        else{
            #if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
                st_lls_conn_t* ps = (st_lls_conn_t*)&blmsSlave[conn_idx - LL_MAX_ACL_CEN_NUM];
                pa->connExpectTime = ps->connExpectTime;
            #else
                irq_restore(r);
                return;
            #endif
        }
        pa->connEventCounter = pc->conn_inst;
        irq_restore(r);

        pa->connHandle = connHandle;

        blt_p_event_callback(BLT_EV_FLAG_ACL_EVERY_CONN_EVENT, conn_evt_result, 8);
    }
}


void blt_ll_acl_chnMapUpdateEvent(u16 connHandle, st_ll_conn_t* pc)
{
    if(pc->conn_established_tick){
        u8 chnMap_result[2];
        acl_channel_map_updateEvt_t *pa = (acl_channel_map_updateEvt_t *)chnMap_result;

        pa->connHandle = connHandle;

        blt_p_event_callback (BLT_EV_FLAG_CHANNEL_MAP_UPDATE, chnMap_result, 2);
    }
}


ble_sts_t blc_ll_setAclSnifferMaxRxBufferLen(u8 len)
{
    #if (LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION)
        if((len >= MAX_OCTETS_DATA_LEN_27) && (len <= MAX_OCTETS_DATA_LEN_EXTENSION))
    #else
        if(len == MAX_OCTETS_DATA_LEN_27)
    #endif
        {
            for(int i = 0; i < LL_MAX_ACL_CONN_NUM; i++) {
                blms[i].ext_data.connEffectiveMaxRxOctets = len;
            }
            return BLE_SUCCESS;
        }

    return LL_ERR_INVALID_PARAMETER;
}

#endif  //end of (LL_RSSI_SNIFFER_MODE_ENABLE)

