/********************************************************************************************************
 * @file    ll_system.c
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



_attribute_ble_data_retention_  volatile    int     blm_btxbrx_state;


_attribute_ble_data_retention_  scan_fifo_t scan_priRxFifo;
_attribute_ble_data_retention_  scan_fifo_t scan_secRxFifo;


_attribute_ble_data_retention_  ll_task_callback_2_t                ll_leg_adv_irq_task_cb = NULL;
_attribute_ble_data_retention_  ll_task_callback_t                  ll_leg_adv_mlp_task_cb = NULL;
_attribute_ble_data_retention_  ll_task_callback_2_t                ll_ext_adv_irq_task_cb = NULL;
_attribute_ble_data_retention_  ll_task_callback_2_t                ll_ext_adv_mlp_task_cb = NULL;


_attribute_ble_data_retention_  ll_task_callback_t                  ll_prichn_scan_irq_task_cb = NULL;

_attribute_ble_data_retention_  ll_task_callback_t                  ll_leg_scan_mlp_task_cb = NULL;   //main_loop task callback
_attribute_ble_data_retention_  ll_task_callback_2_t                ll_ext_scan_irq_task_cb = NULL;
_attribute_ble_data_retention_  ll_task_callback_t                  ll_ext_scan_mlp_task_cb = NULL;
_attribute_ble_data_retention_  ll_task_callback_t                  ll_secchn_scan_task_cb = NULL; //secondary channel scan task CallBack


_attribute_ble_data_retention_  ll_task_callback_t                  ll_init_mlp_task_cb = NULL;      //init main_loop task callback
_attribute_ble_data_retention_  ll_task_callback_t                  ll_ext_init_irq_task_cb = NULL;   //ext_init IRQ task callback


_attribute_ble_data_retention_  ll_task_callback_2_t                ll_prd_adv_irq_task_cb = NULL;
_attribute_ble_data_retention_  ll_task_callback_2_t                ll_prd_adv_mlp_task_cb = NULL;
_attribute_ble_data_retention_  ll_task_callback_2_t                ll_pawra_sub_irq_task_cb = NULL;  //for PAwR-Advertiser subevent
_attribute_ble_data_retention_  ll_task_callback_2_t                ll_pawra_rsp_irq_task_cb = NULL;  //for PAwR-Advertiser rsp_slots
_attribute_ble_data_retention_  ll_task_callback_2_t                ll_pawra_mlp_task_cb = NULL;      //for PAwR-Advertiser
_attribute_ble_data_retention_  ll_task_callback_2_t                ll_pda_sync_irq_task_cb = NULL;
_attribute_ble_data_retention_  ll_task_callback_t                  ll_pda_sync_mlp_task_cb = NULL;

_attribute_ble_data_retention_  ll_prd_sync_pawr_sync_common_t      ll_pda_sync_pawr_sync_common_cb = NULL;

_attribute_ble_data_retention_  ll_task_callback_3_t                ll_pawr_sync_sub_irq_task_cb = NULL;
_attribute_ble_data_retention_  ll_task_callback_t                  ll_pawr_sync_mlp_task_cb = NULL;
_attribute_ble_data_retention_  ll_task_callback_t                  ll_pawr_sync_rspTx_irq_task_cb = NULL;


_attribute_ble_data_retention_  ll_task_callback_t                  ll_acl_conn_irq_task_cb = NULL;
_attribute_ble_data_retention_  ll_task_callback_2_t                ll_acl_conn_mlp_task_cb = NULL;

_attribute_ble_data_retention_  ll_task_callback_2_t                ll_acl_slave_irq_task_cb = NULL;
_attribute_ble_data_retention_  ll_task_callback_2_t                ll_acl_master_irq_task_cb = NULL;

_attribute_ble_data_retention_  ll_rpa_tmo_mainloop_callback_t      ll_rpa_tmo_main_loop_cb = NULL;



_attribute_ble_data_retention_  ll_irq_rx_callback_t                ll_irq_scan_rx_pri_chn_cb = NULL;
_attribute_ble_data_retention_  ll_irq_rx_callback_t                ll_irq_scan_rx_sec_chn_cb = NULL;


_attribute_ble_data_retention_  ll_task_callback_2_t                ll_cis_conn_irq_task_cb = NULL;
_attribute_ble_data_retention_  ll_task_callback_2_t                ll_cis_conn_mlp_task_cb = NULL;


_attribute_ble_data_retention_  ll_task_callback_2_t                ll_cig_mst_irq_task_cb = NULL;
_attribute_ble_data_retention_  ll_task_callback_2_t                ll_cig_mst_mlp_task_cb = NULL;

_attribute_ble_data_retention_  ll_task_callback_2_t                ll_cis_slv_irq_task_cb = NULL;
_attribute_ble_data_retention_  ll_task_callback_t                  ll_cis_slv_mlp_task_cb = NULL;




_attribute_ble_data_retention_  ll_task_callback_2_t                ll_big_bcst_irq_task_cb = NULL;
_attribute_ble_data_retention_  ll_task_callback_2_t                ll_big_bcst_mlp_task_cb = NULL;

_attribute_ble_data_retention_  ll_task_callback_2_t                ll_big_sync_irq_task_cb = NULL;
_attribute_ble_data_retention_  ll_task_callback_2_t                ll_big_sync_mlp_task_cb = NULL;

_attribute_ble_data_retention_  ll_task_callback_t                  ll_aoa_aod_mlp_task_cb = NULL;


_attribute_ble_data_retention_  ll_task_callback_t                  ll_bisSyncRec_IsoTest_handle = NULL;


//channel sounding
_attribute_ble_data_retention_  ll_task_callback_2_t                ll_chn_sounding_irq_task_cb = NULL;
_attribute_ble_data_retention_  ll_task_callback_2_t                ll_chn_sounding_mlp_task_cb = NULL;

_attribute_ble_data_retention_  ll_task_callback_t                  ll_cs_initiator_irq_task_cb = NULL;
_attribute_ble_data_retention_  ll_task_callback_t                  ll_cs_reflector_irq_task_cb = NULL;


#if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
_attribute_ble_data_retention_  ll_task_callback_t                  ll_acl_sniffer_slv_irq_task_cb = NULL;
_attribute_ble_data_retention_  ll_task_callback_t                  ll_acl_sniffer_slv_mlp_task_cb = NULL;
#endif

#if (LL_RSSI_SNIFFER_MASTER_ENABLE)
_attribute_ble_data_retention_  ll_task_callback_t                  ll_acl_sniffer_mst_irq_task_cb = NULL;
_attribute_ble_data_retention_  ll_task_callback_t                  ll_acl_sniffer_mst_mlp_task_cb = NULL;
#endif



#if (PRICHN_SCAN_SMALL_INTV_WITH_ANOTHER_BIG_INTV_ADV_RESOLVE_EN)
_attribute_ble_data_retention_  _attribute_aligned_(4) ll_ad_scan_t  bltAdScn;
#endif


#if FAST_SETTLE
    _attribute_data_retention_ unsigned int fast_settle_cal_tick = 0;
#endif






void blt_ll_registerHostMainloopCallback (ll_host_mainloop_callback_t cb)
{
    ll_host_main_loop_cb = cb; ////blt_gap_mainloop
}


void blt_ll_registerConnectionEncryptionDoneCallback(ll_enc_done_callback_t  cb)
{
    ll_encryption_done_cb = cb;
}

#if(LL_PAUSE_ENC_FIX_EN)
void blc_ll_registerConnectionEncryptionPauseCallback(ll_enc_pause_callback_t  cb)
{
    ll_encryption_pause_cb = cb;
}
#endif

void blt_ll_registerConnectionCompleteHandler(ll_conn_complete_handler_t  handler)
{
    ll_connComplete_handler = handler; ////blt_gap_conn_complete_handler
}

void blt_ll_registerConnectionTerminateHandler(ll_conn_terminate_handler_t  handler)
{
    ll_connTerminate_handler = handler; ///blt_gap_conn_terminate_handler
}




u8  blt_ll_getCurrentState(void)
{
    return blms_state;
}



_attribute_ram_code_ u32 blt_ll_get_rx_packet_tick(u8 rf_len)
{
    /* packet length for 1M: 1(preamble) + 4(accesscode) + 2(header) + rf_len + 3(CRC)
     *                   2M: 2(preamble) + 4(accesscode) + 2(header) + rf_len + 3(CRC)
     *                Coded: ...
     * timeStamp is captured after accesscode, so only consider  2(header) + rf_len + 3(CRC),  leave some margin  */

    /* timing after "access_code"
     * 1M:       (rf_len+5)*8          = rf_len*8 + 40
     * 2M:       (rf_len+5)*4          = rf_len*4 + 20
     * Coded S8: rf_len*64 + 720 - 336 = rf_len*64 + 384 = (rf_len+5)*64 + 64
     * Coded S2: rf_len*16 + 462 - 336 = rf_len*16 + 126 = (rf_len+5)*16 + 46
     */
    //if((bltRxPkt.rx_irq_tick - bltRxPkt.rx_timeStamp) < ((rf_len + 5) * bltPHYs.peer_oneByte_us + 300) * SYSTEM_TIMER_TICK_1US)
    if((s32)(bltRxPkt.rx_irq_tick - bltRxPkt.rx_timeStamp) < (s32)(rf_len * bltPHYs.peer_oneByte_us + 600) * SYSTEM_TIMER_TICK_1US)  //1000 > (384 + IRQ_delay_margin)
    {
        return (u32)(bltRxPkt.rx_timeStamp - bltPHYs.prmb_ac_us * SYSTEM_TIMER_TICK_1US) | 1;  //none zero
    }

    return 0;
}




_attribute_ram_code_ void blt_ll_rx_start_tick_check(void)
{
    u8* raw_pkt = ble_curr_rx_dma_buff;
    bltRxPkt.rx_irq_tick = clock_time();
    bltRxPkt.rx_timeStamp = hal_rf_get_rx_timestamp(); //RX time_stamp should read ASAP

#if LL_CRC_CHECK_REGISTER_EN
    if(((reg_rf_dec_err & 0xf0) == 0) && RF_BLE_RF_PAYLOAD_LENGTH_OK(raw_pkt))
#else
    #if(HW_AES_CCM_ALG_EN)//Todo: B92 only
        if ( (!(reg_rf_dec_err&FLD_RF_PKT_DEC_ERR)))
    #else
        if(RF_BLE_PACKET_VALIDITY_CHECK(raw_pkt))//
    #endif
#endif
    {
        u8 rf_len = raw_pkt[DMA_RFRX_OFFSET_RFLEN];

        #if (LL_FEATURE_ENABLE_LE_CODED_PHY)
            if(bltPHYs.cur_llPhy == BLE_PHY_CODED && ll_coded_phy_ind_detect_cb){
                ll_coded_phy_ind_detect_cb(rf_len); //blt_coded_phy_detect_peer_code_phy_indication
            }
        #endif

        bltRxPkt.rx_header_tick = blt_ll_get_rx_packet_tick(rf_len);
        bltRxPkt.crc_correct = 1;
    }
    else{
        bltRxPkt.rx_header_tick = 0; //clear
        bltRxPkt.crc_correct = 0;
    }
}




static inline void blt_fsm_cmd_done_dbg(u16 src_rf)
{
#if (SL_STACK_FSM_TIMING_EN)
        if(src_rf & FLD_RF_IRQ_CMD_DONE){
            log_tick_irq(SL_STACK_FSM_TIMING_EN, SLET_11_c_cmdDone);
        }
        if(src_rf & FLD_RF_IRQ_FIRST_TIMEOUT){
            log_tick_irq(SL_STACK_FSM_TIMING_EN, SLET_12_c_1stRxTmt);
        }
        if(src_rf & FLD_RF_IRQ_RX_TIMEOUT){
            log_tick_irq(SL_STACK_FSM_TIMING_EN, SLET_13_c_rxTmt);
        }
        if(src_rf & FLD_RF_IRQ_RX_CRC_2){
            log_tick_irq(SL_STACK_FSM_TIMING_EN, SLET_14_c_rxCrc2);
        }
#else
        (void)src_rf; //unused, remove warning
#endif
}






#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_ void blc_sdk_irq_handler(void)//blt_ll_updateScheduler //irq_system_timer
{

#if (DYNAMIC_SCHE_CAL_TIME_EN)
    bltSche.sche_tick_begin = clock_time();
    bltSche.cal_time_en = 0;
#endif


    #if (SL01_IRQ)
        log_task_begin_irq(SL_STACK_IRQ_TIMING_EN, SL01_IRQ);
    #endif

    blmsParam.stimer_irq_process_en = 0;


    u16  src_rf = reg_rf_irq_status;
    if(src_rf & FLD_RF_IRQ_RX)
    {
        blt_ll_rx_start_tick_check();
        #if (SLEV_irq_rx)
            log_event_irq(SL_STACK_IRQ_TIMING_EN, SLEV_irq_rx);
        #endif
        /* primary channel/secondary channel scan should be judged and executed first,
         * because they may need send scan_req/aux_scan_req in 150uS, timing is urgent */
        if(blms_state == BLMS_STATE_PRICHN_SCAN_S){
            //if(ll_irq_scan_rx_pri_chn_cb)  // not judge to save RamCode
            {
                ll_irq_scan_rx_pri_chn_cb();  //irq_scan_rx_primary_channel
            }
        }
        else if(blms_state & (BLMS_STATE_SECCHN_SCAN_S | BLMS_STATE_PDA_SYNC_S | BLMS_STATE_PAWRS_SUB_S)) {
            //if(ll_irq_scan_rx_sec_chn_cb)  // not judge to save RamCode
            {
                ll_irq_scan_rx_sec_chn_cb();  // irq_scan_rx_secondary_channel
            }
        }
        else if( blms_state & (BLMS_STATE_BTX_S | BLMS_STATE_BRX_S)){
            DBG_CHN2_TOGGLE;
            DBG_SIHUI_CHN2_TOGGLE;
            //if(ll_acl_conn_irq_task_cb)  // not judge to save RamCode
            {
                ll_acl_conn_irq_task_cb(FLAG_IRQ_RX);  // blt_acl_conn_interrupt_task()   irq_acl_conn_rx()
                if( blc_rf_pa_cb){  blc_rf_pa_cb(PA_TYPE_TX_ON); }
            }
        }
    #if (LL_FEATURE_ENABLE_CONNECTED_ISO)
        else if( blms_state & (BLMS_STATE_CTX_S | BLMS_STATE_CRX_S)){
            DBG_CHN3_TOGGLE;
            DBG_SIHUI_CHN3_TOGGLE;
            DBG_FANQH_CHN3_TOGGLE;
            //if(ll_cis_conn_irq_task_cb) // not judge to save RamCode
            {
                ll_cis_conn_irq_task_cb(FLAG_IRQ_RX, NULL);  // blt_cis_conn_interrupt_task()  irq_cis_rx()
            }
        }
    #endif
    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
        else if ( blms_state & BLMS_STATE_PAWRA_SLOT_S)
        {
            //if(ll_pawra_rsp_irq_task_cb) // not judge to save RamCode
            {
                ll_pawra_rsp_irq_task_cb(FLAG_IRQ_RX, NULL);  // blt_pawra_rsp_interrupt_task()  irq_pawra_slot_rx()
            }
        }
    #endif
    #if (LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER)
        else if( blms_state == BLMS_STATE_BSYNC_S){
            //if(ll_big_sync_irq_task_cb) // not judge "ll_big_sync_irq_task_cb != NULL" to save RamCode
            {
                ll_big_sync_irq_task_cb(FLAG_SCHEDULE_BISSYNC_RX, NULL);  // blt_big_sync_interrupt_task() irq_big_sync_rx()
            }
        }
    #endif
    #if(LL_FEATURE_ENABLE_CHANNEL_SOUNDING)
        else if (blms_state & (BLMS_STATE_CS_INIT_RX_S ))
        {
            if(ll_cs_initiator_irq_task_cb){
                DBG_CS_CHN13_TOGGLE;

                ll_cs_initiator_irq_task_cb(FLAG_CS_STEP_RX); //blt_cs_initiator_irq_task  blt_cs_initiator_rx
            }
        }
        else if(blms_state & BLMS_STATE_CS_REFL_STEP_S)
        {
            if(ll_cs_reflector_irq_task_cb)
            {
                DBG_CS_CHN13_TOGGLE;
                ll_cs_reflector_irq_task_cb(FLAG_CS_STEP_RX); //blt_cs_reflector_irq_task  blt_cs_refl_stepRev
            }
        }
    #endif
    #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
        else if( blms_state == BLMS_STATE_SNIFM_S ){
            //if(ll_acl_sniffer_mst_irq_task_cb) // not judge "ll_acl_sniffer_mst_irq_task_cb != NULL" to save RamCode
            {
                ll_acl_sniffer_mst_irq_task_cb(FLAG_IRQ_RX);  //irq_acl_sniffer_mst_rx
            }
        }
        else if( blms_state == BLMS_STATE_SNIFM_SEEK_S ){
            //if(ll_acl_sniffer_mst_irq_task_cb) // not judge "ll_acl_sniffer_mst_irq_task_cb != NULL" to save RamCode
            {
                ll_acl_sniffer_mst_irq_task_cb(FLAG_ACL_SNIFFER_SEEK_RX);  //irq_acl_sniffer_mst_seek_rx
            }
        }
    #endif
    #if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
        else if( blms_state == BLMS_STATE_SNIFS_S ){
            //if(ll_acl_sniffer_slv_irq_task_cb) // not judge "ll_acl_sniffer_slv_irq_task_cb != NULL" to save RamCode
            {
                ll_acl_sniffer_slv_irq_task_cb(FLAG_IRQ_RX);  //irq_acl_sniffer_slv_rx
            }
        }
        else if( blms_state == BLMS_STATE_SNIFS_SEEK_S ){
            //if(ll_acl_sniffer_slv_irq_task_cb) // not judge "ll_acl_sniffer_slv_irq_task_cb != NULL" to save RamCode
            {
                ll_acl_sniffer_slv_irq_task_cb(FLAG_ACL_SNIFFER_SEEK_RX);  //irq_acl_sniffer_slv_seek_rx
            }
        }
    #endif
        else{

            #if (0) //find out all RF status problem
                write_dbg32(0x00018, blms_state);
                BLMS_ERR_DEBUG(0, 0xFE0A0000);
            #endif

            reg_rf_irq_status = FLD_RF_IRQ_RX;
        }
    }



    if(src_rf & FLD_RF_IRQ_TX)
    {
        reg_rf_irq_status = FLD_RF_IRQ_TX;
        #if (SLEV_irq_tx)
            log_event_irq(SL_STACK_IRQ_TIMING_EN, SLEV_irq_tx);
        #endif

        if ( blms_state & (BLMS_STATE_BTX_S | BLMS_STATE_BRX_S) )
        {
            //if(ll_acl_conn_irq_task_cb)  // not judge to save RamCode
            {
                ll_acl_conn_irq_task_cb(FLAG_IRQ_TX);  // irq_acl_conn_tx()
                if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_RX_ON);  }
            }
        }
    #if (LL_FEATURE_ENABLE_CONNECTED_ISO)
        else if( blms_state & (BLMS_STATE_CTX_S | BLMS_STATE_CRX_S)){
            #if 0 //no need TX logic now, so save some RamCode and timing
                //if(ll_cis_conn_irq_task_cb) // not judge to save RamCode
                {
                    ll_cis_conn_irq_task_cb(FLAG_IRQ_TX, NULL);  // blt_cis_conn_interrupt_task
                }
            #endif

            if(blms_state == BLMS_STATE_CTX_S){
                if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_RX_ON);  }
            }
        }
    #endif

    #if(LL_FEATURE_ENABLE_CHANNEL_SOUNDING)
        else if(blms_state & BLMS_STATE_CS_REFL_STEP_S){
            DBG_CS_CHN12_TOGGLE;
            ll_cs_reflector_irq_task_cb(FLAG_CS_STEP_REFL_STX_POST); //blt_cs_reflector_irq_task blt_cs_refl_m0SysIrq_m1m2RfTxIrq_txDone
            bltSche.sche_process_en = 1;
        }
    #endif
    }




    if (src_rf & BLMS_FLG_RF_CONN_DONE)
    {

        reg_rf_irq_status = BLMS_FLG_RF_CONN_DONE;

        #if (SLEV_irq_rfdone)
            log_event_irq(SL_STACK_IRQ_TIMING_EN, SLEV_irq_rfdone);
        #endif

        blt_fsm_cmd_done_dbg(src_rf);


        if (blms_state & (BLMS_STATE_BTX_S | BLMS_STATE_BRX_S) )
        {
            blmsParam.rf_fsm_busy = 0;

    #if(OPTIMIZE_INSERT_EMPTY_EN)
            if ((blms_state & BLMS_STATE_BTX_S) && (src_rf & (FLD_RF_IRQ_FIRST_TIMEOUT | FLD_RF_IRQ_RX_TIMEOUT | FLD_RF_IRQ_RX_CRC_2))){
                blms_pconn->llcp_flag.bit.peer_ack_flag =0;
            }
    #endif

            systimer_set_irq_capture(clock_time () + 50 * SYSTEM_TIMER_TICK_1US);
            systimer_clr_irq_status();
        }
    #if (LL_FEATURE_ENABLE_CONNECTED_ISO)
        else if (blms_state & (BLMS_STATE_CTX_S | BLMS_STATE_CRX_S) )
        {
            blmsParam.stimer_irq_process_en = 1;
        }
    #endif
    #if (LL_FEATURE_ENABLE_CONNECTIONLESS_ISO)
        else if (blms_state & (BLMS_STATE_BBCST_S | BLMS_STATE_BSYNC_S) )
        {
            blmsParam.stimer_irq_process_en = 1;
        }
    #endif
    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
        else if (blms_state & (BLMS_STATE_PAWRA_SLOT_S | 0) )
        {
            blmsParam.stimer_irq_process_en = 1;
        }
    #endif
    #if(LL_FEATURE_ENABLE_CHANNEL_SOUNDING)
        else if (blms_state & (BLMS_STATE_CS_INIT_TX_S | BLMS_STATE_CS_INIT_RX_S))
        {
            DBG_CS_CHN12_TOGGLE;
            blmsParam.stimer_irq_process_en = 1;
        }
    #endif
        else if(blms_state & (BLMS_STATE_SECCHN_SCAN_S | BLMS_STATE_PDA_SYNC_S | BLMS_STATE_PAWRS_SUB_S))
        {
            blmsParam.rf_fsm_busy = 0;
            blmsParam.stimer_irq_process_en = 1;
            if(blms_state == BLMS_STATE_SECCHN_SCAN_S){
                systick_irq_trigger = SYS_IRQ_TRIG_SECCHN_SCAN_POST;
            }
            else if(blms_state == BLMS_STATE_PDA_SYNC_S){
                systick_irq_trigger = SYS_IRQ_TRIG_PDA_SYNC_POST;
            }
            else{ //BLMS_STATE_PAWRS_SUB_S
                systick_irq_trigger = SYS_IRQ_TRIG_PAWRS_SUB_POST;
            }
        }

    }




    if(systimer_get_irq_status() || blmsParam.stimer_irq_process_en ){
        bltSche.system_irq_tick = systimer_get_irq_capture();

        #if (FIX_STIMER_SET_CAPTURE_ERR)
            /* set to a long time to avoid signal pulling which may lead to new IRQ no trigger */
            systimer_set_irq_capture(clock_time() ^ BIT(31));
        #else
            if(blmsParam.stimer_irq_process_en){
                systimer_set_irq_capture(clock_time()^BIT(30));
            }
        #endif

        systimer_clr_irq_status();

        #if (SLEV_irq_stimer)
            log_event_irq(SL_STACK_IRQ_TIMING_EN, SLEV_irq_stimer);
        #endif

        irq_system_timer();

        bltSche.sche_process_en = 1;

        #if (BLMS_PM_ENABLE)
            blmsPm.pm_entered = 0;
            blmsPm.wkpTsk_oft = WKPTASK_INVALID;
        #endif


    }

#if OS_SUP_EN
    //if( bltHci_txfifo.wptr != bltHci_txfifo.rptr || bltSche.sche_process_en){
    if( bltHci_txfifo.wptr != bltHci_txfifo.rptr){
        if(blt_os_semCountIncrementIrq_cb)
        {
            blt_os_semCountIncrementIrq_cb();
        }
    }
#endif


    if( bltSche.sche_process_en && BLMS_STATE_UPDATE_SCHEDULER )
    {

        if(blmsParam.state_chng){
            blt_ll_procStateChange();
        }

        #if (LL_RSSI_SNIFFER_MODE_ENABLE)
            #if (LL_RSSI_SNIFFER_SLAVE_ENABLE && LL_RSSI_SNIFFER_MASTER_ENABLE)
                bool snif_used = (ll_acl_sniffer_mst_irq_task_cb || ll_acl_sniffer_slv_irq_task_cb) ? TRUE : FALSE;
            #elif (LL_RSSI_SNIFFER_MASTER_ENABLE)
                bool snif_used = ll_acl_sniffer_mst_irq_task_cb ? TRUE : FALSE;
            #elif (LL_RSSI_SNIFFER_SLAVE_ENABLE)
                bool snif_used = ll_acl_sniffer_slv_irq_task_cb ? TRUE : FALSE;
            #endif
                if(!snif_used && ll_acl_conn_irq_task_cb)// can not save, must judge
        #else
                if(ll_acl_conn_irq_task_cb)// can not save, must judge
        #endif
                {
                    ll_acl_conn_irq_task_cb(FLAG_ACL_CONN_PARAM_UPDATE_CHECK);  // blt_acl_conn_interrupt_task  blt_llms_procConnCreateConnParamUpdate
                }
        
        #if (BLMS_PM_ENABLE)
            blmsPm.sleep_allowed = 0;
        #endif


        extern void blt_hal_reset_baseband(void);
        blt_hal_reset_baseband();

        /* link layer scheduler core process */
        blt_ll_updateScheduler();

        /*must stop system timer irq when task is empty*/
        if(bltSche.task_mask == 0){
            systimer_clr_irq_status();
            systimer_irq_disable();
        }

        if(blms_state != BLMS_STATE_PRICHN_SCAN_S){
            blms_state = BLMS_STATE_NONE;
        }
    }

    /* Process rf status && rx boundary irq status clear */
    bltSche.sche_process_en = 0;
    if(blmsParam.delay_clear_rf_status){
        if(blmsParam.dly_start_tick_clr_rf_sts){
            while(!clock_time_exceed(blmsParam.dly_start_tick_clr_rf_sts, blmsParam.delay_clear_rf_status));
            blmsParam.dly_start_tick_clr_rf_sts = 0;
            my_dump_str_data(DBG_BOUNDARY_RX, "potential bound rx clr", 0, 0);
        }

        CLEAR_ALL_RFIRQ_STATUS;
        blmsParam.delay_clear_rf_status = 0;
    }



    #if (DYNAMIC_SCHE_CAL_TIME_EN)
        if(bltSche.cal_time_en){
            u32 sche_us = (clock_time() - bltSche.sche_tick_begin)/SYSTEM_TIMER_TICK_1US;
            bltSche.sche_process_us = (bltSche.sche_process_us + sche_us + 1)>>1;

            #if (DBG_SCHE_CAL_TIME_EN)
                if(bltSche.sche_process_us > 350){
                    my_dump_str_u32s(DBG_SCHE_CAL_TIME_EN, "err", bltSche.sche_process_us, sche_us, bltSche.sche_max_us, 0);
                    BLMS_ERR_DEBUG(DBG_SCHE_CAL_TIME_EN, 0xFF100000);
                }

                if(sche_us > bltSche.sche_max_us){
                    bltSche.sche_max_us = sche_us;
                }
                my_dump_str_u32s(DBG_SCHE_CAL_TIME_EN, "scheduler time", bltSche.sche_process_us, sche_us, bltSche.sche_max_us, 0);
            #endif
        }
    #endif


#if OS_SUP_EN
    if(blt_os_giveSemFromIrq_cb)
    {
        blt_os_giveSemFromIrq_cb();
    }
#endif


#if (SL01_IRQ)
    log_task_end_irq(SL_STACK_IRQ_TIMING_EN, SL01_IRQ);
#endif
}






#if (DBG_BOUNDARY_RX)
int Adbg_boundary_rx_num = 0;
#endif

#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_ void irq_system_timer(void)
{

    if(systick_irq_trigger == SYS_IRQ_TRIG_NEW_TASK)
    {
        bltSche.pTask_cur = bltSche.pTask_pre = bltSche.pTask_next;  //new
        bltSche.pTask_next = bltSche.pTask_cur->next;  //find next


        #if(SCH_DEBUG_EN)
            if(NULL == bltSche.pTask_cur){ BLMS_ERR_DEBUG(SCH_DEBUG_EN, 0xFF020000); }
        #endif

        int slotTask_flg  = bltSche.pTask_cur->scheTask_flg;
        int slotTask_idx  = bltSche.pTask_cur->scheTask_idx;

        bltSche.sSlot_idx_irq_real = 0 + bltSche.pTask_cur->begin; //TODO SiHui, can save a variable
        bltSche.sSlot_tick_irq_real = bltSche.sSlot_tick_irq;
//      DBG_C HN5_TOGGLE;

        int bSlot_offset = bltSche.pTask_cur->begin>>5;
        bltSche.bSlot_idx_irq_real = bltSche.bSlot_idx_start + bSlot_offset;  // /32
        bltSche.bSlot_tick_irq_real = bltSche.bSlot_tick_start + bSlot_offset*SYSTEM_TIMER_TICK_625US;
        bltSche.sSlot_diff_irq = bltSche.pTask_cur->begin & 0x1F;

        slotTask_flg &= TSKFLG_VALID_MASK;  //must
        if(slotTask_flg != TSKFLG_PRICHN_SCAN){
            bltSche.lklt_taskNum --;
        }
        #if (SCH_DEBUG_EN)
            u32 tick_margin = 150*SYSTEM_TIMER_TICK_1US;
            #if (BLMS_PM_ENABLE)
                if(blmsPm.pm_entered){tick_margin = 500*SYSTEM_TIMER_TICK_1US;}
            #endif
            if(tick1_exceed_tick2(clock_time(), bltSche.sSlot_tick_irq + tick_margin)){
                //my_dump_str_u32s(DBG_EXTSCAN_TIMING, "debug 8", clock_time(), bltSche.sSlot_tick_irq, bltSche.system_irq_tick, slotTask_flg);
                write_dbg32(0x00018, clock_time());
                write_dbg32(0x0001C, bltSche.sSlot_tick_irq);
                BLMS_ERR_DEBUG(SCH_DEBUG_EN, 0x88010000 | (((clock_time() - bltSche.sSlot_tick_irq)>>4)&0xFFFF)  );
            }
        #endif

        #if (BLE_LLMIC_CONCURRENT_EN)
            extern void blt_ll_check_llmic_status(void);
             blt_ll_check_llmic_status();
        #endif

        if( slotTask_flg == TSKFLG_ACL_MASTER || slotTask_flg == TSKFLG_ACL_SLAVE )
        {  //BTX start or BRX start
            if(slotTask_flg == TSKFLG_ACL_MASTER){  //Master, BTX start
            #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
                if(ll_acl_sniffer_mst_irq_task_cb){
                    ll_acl_sniffer_mst_irq_task_cb(FLAG_SCHEDULE_START | slotTask_idx);  // blt_acl_sniffer_mst_irq_task() blt_sniffer_mst_start()
                }
                else
            #endif
                //if(ll_acl_master_irq_task_cb) //not judge, to save RamCode
                {
                    ll_acl_master_irq_task_cb(FLAG_SCHEDULE_START | slotTask_idx, (void*)bltSche.pTask_cur);  // blt_acl_master_interrupt_task() blt_btx_start()
                }
            }
            else{  //Slave, BRX start
            #if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
                if(ll_acl_sniffer_slv_irq_task_cb){
                    ll_acl_sniffer_slv_irq_task_cb(FLAG_SCHEDULE_START | slotTask_idx);  // blt_acl_sniffer_slv_irq_task() blt_sniffer_slv_start()
                }
                else
            #endif
                //if(ll_acl_slave_irq_task_cb) //not judge, to save RamCode
                {
                    ll_acl_slave_irq_task_cb(FLAG_SCHEDULE_START | slotTask_idx, (void*)bltSche.pTask_cur);  // blt_acl_slave_interrupt_task() blt_brx_start()
                }
            }
        }
        #if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER)
            else if( slotTask_flg == TSKFLG_CIG_MST){
                //if(ll_cig_mst_irq_task_cb)  //save RamCode
                {
                    ll_cig_mst_irq_task_cb(FLAG_SCHEDULE_CIGMST_START | slotTask_idx, NULL);  // blt_cig_mst_interrupt_task()
                }
            }
        #endif
        #if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE)
            else if( slotTask_flg == TSKFLG_CIG_SLV){
                //if(ll_cis_slv_irq_task_cb)  //save RamCode
                {
                    ll_cis_slv_irq_task_cb(FLAG_SCHEDULE_CIGSLV_START | slotTask_idx, NULL);  // blt_cig_slv_interrupt_task()
                }
            }
        #endif
        #if(LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER)
            else if( slotTask_flg == TSKFLG_BIG_BCST){
                //if(ll_big_bcst_irq_task_cb)  //save RamCode
                {
                    ll_big_bcst_irq_task_cb(FLAG_SCHEDULE_BIGBCST_START | slotTask_idx, NULL);  // blt_big_bcst_interrupt_task()
                }
            }
        #endif
        #if(LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC)
            else if( slotTask_flg == TSKFLG_PDA_SYNC){
                //if(ll_pda_sync_irq_task_cb)  //save RamCode
                {
                    ll_pda_sync_irq_task_cb(FLAG_SCHEDULE_START | slotTask_idx, NULL); //blt_pda_sync_interrupt_task
                }
            }
            #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
                else if( slotTask_flg == TSKFLG_PAWRS_SUB){
                    //if(ll_pawr_sync_sub_irq_task_cb)  //save RamCode
                    {
                        ll_pawr_sync_sub_irq_task_cb(FLAG_SCHEDULE_START | slotTask_idx, NULL, NULL); //blt_pawr_sync_sub_interrupt_task
                    }
                }
            #endif
        #endif
        #if(LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER)
            else if( slotTask_flg == TSKFLG_BIG_SYNC){
                //if(ll_big_sync_irq_task_cb) //save RamCode
                {
                    ll_big_sync_irq_task_cb(FLAG_SCHEDULE_BIGSYNC_START | slotTask_idx, NULL);  // blt_big_sync_interrupt_task()
                }
            }
        #endif
        #if (LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING)
            else if(slotTask_flg == TSKFLG_EXT_ADV){
                //if(ll_ext_adv_irq_task_cb)  //save RamCode
                {
                    ll_ext_adv_irq_task_cb(FLAG_SCHEDULE_START | slotTask_idx, NULL);   //blt_ext_adv_interrupt_task();
                    #if OS_SUP_EN
                    if(blt_os_semCountIncrementIrq_cb)
                    {
                        blt_os_semCountIncrementIrq_cb();
                    }
                    #endif
                }
            }
            else if(slotTask_flg == TSKFLG_AUX_ADV){
                //if(ll_ext_adv_irq_task_cb)  //save RamCode
                {
                    ll_ext_adv_irq_task_cb(FLAG_SCHEDULE_SEND_AUXADV | slotTask_idx, NULL);     //blt_ext_adv_interrupt_task();
                }
            }
        #endif
        #if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING)
            else if(slotTask_flg == TSKFLG_PERD_ADV){
                //if(ll_prd_adv_irq_task_cb)  //save RamCode
                {
                    ll_prd_adv_irq_task_cb(FLAG_SCHEDULE_START | slotTask_idx, NULL);   //blt_prd_adv_interrupt_task();
                    #if OS_SUP_EN
                    if(blt_os_semCountIncrementIrq_cb)
                    {
                        blt_os_semCountIncrementIrq_cb();
                    }
                    #endif
                }
            }
        #endif
        #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
            else if(slotTask_flg == TSKFLG_PAWRA_SUB){
                //if(ll_pawra_sub_irq_task_cb)  //save RamCode
                {
                    ll_pawra_sub_irq_task_cb(FLAG_SCHEDULE_START | slotTask_idx, NULL); //blt_pawra_subx_interrupt_task(); blt_pawra_subx_start
                }
            }
            else if(slotTask_flg == TSKFLG_PAWRA_RSP){
                //if(ll_pawra_rsp_irq_task_cb)  //save RamCode
                {
                    ll_pawra_rsp_irq_task_cb(FLAG_SCHEDULE_START | slotTask_idx, NULL); //blt_pawra_rsp_interrupt_task(); blt_pawra_rsp_start
                }
            }
        #endif
            else if(slotTask_flg == TSKFLG_LEG_ADV){
                //if(ll_leg_adv_irq_task_cb)  //not judge, to save RamCode
                {
                    ll_leg_adv_irq_task_cb(FLAG_SCHEDULE_START, NULL);  // blt_leg_adv_interrupt_task();
                    #if OS_SUP_EN
                    if(blt_os_semCountIncrementIrq_cb)
                    {
                        blt_os_semCountIncrementIrq_cb();
                    }
                    #endif
                }
            }
            else if(slotTask_flg == TSKFLG_PRICHN_SCAN){  //only primary channel SCAN Task
                if(ll_prichn_scan_irq_task_cb)
                {
                    ll_prichn_scan_irq_task_cb(FLAG_SCHEDULE_START); //blt_prichn_scan_interrupt_task
                }
            }
            #if (PRICHN_SCAN_SMALL_INTV_WITH_ANOTHER_BIG_INTV_ADV_RESOLVE_EN)
                else if(slotTask_flg == TSKFLG_SCAN_ALIGN){
                    blms_state = BLMS_STATE_PRICHN_SCAN_ALIGN;
                    blt_ll_calculate_sSlot_next(clock_time() + (bltSche.sche_process_us + SCHE_NEW_TASK_MARGIN_US)*SYSTEM_TIMER_TICK_1US);
                    DBG_SIHUI_CHN3_TOGGLE;
                }
            #endif
            else if(slotTask_flg == TSKFLG_SECCHN_SCAN){
                //my_dump_str_u32s(DBG_EXTSCAN_TIMING, "debug 4", bltSche.pTask_cur, bltSche.pTask_cur->begin, bltSche.sSlot_idx_irq_real, 0);
                //if(ll_secchn_scan_task_cb) //to save RamCode
                {
                    ll_secchn_scan_task_cb(FLAG_SCHEDULE_START | slotTask_idx); //blt_ll_procAuxiliaryScanTask
                }
            }
            #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
                else if(slotTask_flg == TSKFLG_PAWRS_RSP){
                    //if(ll_pawr_sync_rspTx_irq_task_cb)
                    {
                        ll_pawr_sync_rspTx_irq_task_cb(FLAG_SCHEDULE_START | slotTask_idx); //blt_ll_PAwRsync_rspSlotTxProc
                    }
                }
            #endif

            #if(LL_FEATURE_ENABLE_CHANNEL_SOUNDING)
            else if(slotTask_flg == TSKFLG_CS){
                if(ll_chn_sounding_irq_task_cb){
                    ll_chn_sounding_irq_task_cb(FLAG_CS_SUBEVENT_START | slotTask_idx, (void*)bltSche.pTask_cur); //blt_cs_interrupt_task   blt_cs_subevent_start
                }
            }
            #endif

            #if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
            else if(slotTask_flg == TSKFLG_SNIFS_SEEK){
                //if(ll_acl_sniffer_slv_irq_task_cb) //to save RamCode
                {
                    ll_acl_sniffer_slv_irq_task_cb(FLAG_ACL_SNIFFER_SEEK_START | slotTask_idx);  // blt_acl_sniffer_slv_irq_task() blt_sniffer_slv_seek_start()
                }
            }
            #endif

            #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
            else if(slotTask_flg == TSKFLG_SNIFM_SEEK){
                //if(ll_acl_sniffer_mst_irq_task_cb) //to save RamCode
                {
                    ll_acl_sniffer_mst_irq_task_cb(FLAG_ACL_SNIFFER_SEEK_START | slotTask_idx);  // blt_acl_sniffer_mst_irq_task() blt_sniffer_mst_seek_start()
                }
            }
            #endif
    }
    else  //post task
    {
        if(systick_irq_trigger & (SYS_IRQ_TRIG_BTX_POST | SYS_IRQ_TRIG_BRX_POST))
        {
            if(systick_irq_trigger == SYS_IRQ_TRIG_BTX_POST){  //Master, BTX post
            #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
                if(ll_acl_sniffer_mst_irq_task_cb){
                    ll_acl_sniffer_mst_irq_task_cb(FLAG_SCHEDULE_DONE);  // blt_acl_sniffer_mst_irq_task() blt_sniffer_mst_post()
                }
                else
            #endif
                //if(ll_acl_master_irq_task_cb) //not judge, to save RamCode
                {
                    ll_acl_master_irq_task_cb(FLAG_SCHEDULE_DONE, NULL);  // blt_acl_master_interrupt_task() blt_brx_post()
                }
            }
            else{   //Slave, BRX post
            #if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
                if(ll_acl_sniffer_slv_irq_task_cb){
                    ll_acl_sniffer_slv_irq_task_cb(FLAG_SCHEDULE_DONE);  // blt_acl_sniffer_slv_irq_task() blt_sniffer_slv_post()
                }
                else
            #endif
                //if(ll_acl_slave_irq_task_cb) //not judge, to save RamCode
                {
                    ll_acl_slave_irq_task_cb(FLAG_SCHEDULE_DONE, NULL);  // blt_acl_slave_interrupt_task() blt_brx_post()
                }
            }
        }
        #if(LL_FEATURE_ENABLE_CONNECTED_ISO || LL_FEATURE_ENABLE_CONNECTIONLESS_ISO)
            else if(systick_irq_trigger & (   SYS_IRQ_TRIG_CTX_START | SYS_IRQ_TRIG_CRX_START \
                                            | SYS_IRQ_TRIG_CTX_POST  | SYS_IRQ_TRIG_CRX_POST \
                                            | SYS_IRQ_TRIG_BIS_TX_START | SYS_IRQ_TRIG_BIS_TX_POST \
                                            | SYS_IRQ_TRIG_BIS_RX_START | SYS_IRQ_TRIG_BIS_RX_POST)){
                //make sure state machine is clean
                STOP_RF_STATE_MACHINE;

                if(systick_irq_trigger == SYS_IRQ_TRIG_CTX_START){      //Master, CIS BTX start
                    //The current CIS has been updated in CTX_POST or CIG_TRIGGER.
                    //attention: "blms_state = BLMS_STATE_CTX_S" is set in function
                    //if(ll_cig_mst_irq_task_cb)  //save RamCode
                    {
                        ll_cig_mst_irq_task_cb(FLAG_SCHEDULE_CTX_START, NULL);  // blt_cig_mst_interrupt_task()
                    }
                }
                else if(systick_irq_trigger == SYS_IRQ_TRIG_CTX_POST){      //Master, CIS BTX post
                    //if(ll_cig_mst_irq_task_cb)  //save RamCode
                    {
                        ll_cig_mst_irq_task_cb(FLAG_SCHEDULE_CTX_POST, NULL);  // blt_cig_mst_interrupt_task()
                    }
                }
                else if(systick_irq_trigger == SYS_IRQ_TRIG_CRX_START){ ///Slave, CIS BRX start
                    //if(ll_cis_slv_irq_task_cb)  //save RamCode
                    {
                        ll_cis_slv_irq_task_cb(FLAG_SCHEDULE_CISSLV_START, NULL);  // blt_cig_slv_interrupt_task()
                    }
                }
                else if(systick_irq_trigger == SYS_IRQ_TRIG_CRX_POST){  ///Slave, CIS BRX post
                    //if(ll_cis_slv_irq_task_cb)  //save RamCode
                    {
                        ll_cis_slv_irq_task_cb(FLAG_SCHEDULE_CISSLV_POST, NULL);  // blt_cig_slv_interrupt_task()
                    }
                }
                else if(systick_irq_trigger == SYS_IRQ_TRIG_BIS_TX_START){
                    //if(ll_big_bcst_irq_task_cb)  //save RamCode
                    {
                        ll_big_bcst_irq_task_cb(FLAG_SCHEDULE_BISBCST_START, NULL);  // blt_big_bcst_interrupt_task()
                    }
                }
                else if(systick_irq_trigger == SYS_IRQ_TRIG_BIS_TX_POST){
                    //if(ll_big_bcst_irq_task_cb)  //save RamCode
                    {
                        ll_big_bcst_irq_task_cb(FLAG_SCHEDULE_BISBCST_POST, NULL);  // blt_big_bcst_interrupt_task()
                    }
                }
                else if(systick_irq_trigger == SYS_IRQ_TRIG_BIS_RX_START){
                    //if(ll_big_sync_irq_task_cb) //save RamCode
                    {
                        ll_big_sync_irq_task_cb(FLAG_SCHEDULE_BISSYNC_START, NULL);  // blt_big_sync_interrupt_task()  blt_bisSync_rx_start
                    }
                }
                else if(systick_irq_trigger == SYS_IRQ_TRIG_BIS_RX_POST){
                    //if(ll_big_sync_irq_task_cb) //save RamCode
                    {
                        ll_big_sync_irq_task_cb(FLAG_SCHEDULE_BISSYNC_POST, NULL);  // blt_big_sync_interrupt_task()  blt_bisSync_rx_post
                    }
                }
            }
        #endif
        #if(LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
            else if(systick_irq_trigger & (SYS_IRQ_TRIG_PAWRA_SLOT_START | SYS_IRQ_TRIG_PAWRA_SLOT_POST)){
                //make sure state machine is clean
                STOP_RF_STATE_MACHINE;

                if(systick_irq_trigger == SYS_IRQ_TRIG_PAWRA_SLOT_START){
                    //if(ll_pawra_rsp_irq_task_cb) //save RamCode
                    {
                        ll_pawra_rsp_irq_task_cb(FLAG_SCHEDULE_PAWRA_SLOT_START, NULL);  // blt_pawra_rsp_interrupt_task()  blt_pawra_slot_start
                    }
                }
                else if(systick_irq_trigger == SYS_IRQ_TRIG_PAWRA_SLOT_POST){
                    //if(ll_pawra_rsp_irq_task_cb) //save RamCode
                    {
                        ll_pawra_rsp_irq_task_cb(FLAG_SCHEDULE_PAWRA_SLOT_POST, NULL);  // blt_pawra_rsp_interrupt_task()  blt_pawra_slot_post
                    }
                }
            }
        #endif
        #if(LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC)
            else if( systick_irq_trigger == SYS_IRQ_TRIG_PDA_SYNC_POST){
                //if(ll_pda_sync_irq_task_cb)  //to save RamCode
                {
                    ll_pda_sync_irq_task_cb(FLAG_SCHEDULE_DONE, NULL); //blt_pda_sync_interrupt_task
                }
            }
            #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
                else if( systick_irq_trigger == SYS_IRQ_TRIG_PAWRS_SUB_POST){
                    //if(ll_pawr_sync_sub_irq_task_cb)  //save RamCode
                    {
                        ll_pawr_sync_sub_irq_task_cb(FLAG_SCHEDULE_DONE, NULL, NULL); //blt_pawr_sync_sub_interrupt_task
                    }
                }
            #endif
        #endif
        #if (LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING)
            else if(systick_irq_trigger == SYS_IRQ_TRIG_EXTADV_SEND){
                //if(ll_ext_adv_irq_task_cb)  //save RamCode
                {
                    ll_ext_adv_irq_task_cb(FLAG_SCHEDULE_SEND_EXTADV, NULL);    //blt_ext_adv_interrupt_task();
                    #if OS_SUP_EN
                        if(blt_os_semCountIncrementIrq_cb)
                        {
                            blt_os_semCountIncrementIrq_cb();
                        }
                    #endif
                }
            }
        #endif
        #if(LL_FEATURE_ENABLE_CHANNEL_SOUNDING)
            else if(systick_irq_trigger & (SYS_IRQ_TRIG_CS_INIT_SRX | SYS_IRQ_TRIG_CS_INIT_TX_START |  SYS_IRQ_TRIG_CS_STEP_POST|\
                                           SYS_IRQ_TRIG_CS_REFL_RX_START | SYS_IRQ_TRIG_CS_REFL_TX_START | SYS_IRQ_TRIG_CS_REFL_TX_POST))
            {
                //make sure state machine is clean
                STOP_RF_STATE_MACHINE;

                if(systick_irq_trigger == SYS_IRQ_TRIG_CS_INIT_SRX){
                    if(ll_cs_initiator_irq_task_cb)//to save RamCode
                    {
                        ll_cs_initiator_irq_task_cb(FLAG_CS_STEP_INIT_SRX_START); //blt_cs_initiator_irq_task  blt_cs_init_step_srx
                    }
                }
                else if(systick_irq_trigger == SYS_IRQ_TRIG_CS_STEP_POST){

                    if(ll_cs_initiator_irq_task_cb)//to save RamCode
                    {
                        ll_cs_initiator_irq_task_cb(FLAG_STEP_POST); //blt_cs_initiator_irq_task  blt_cs_init_step_post
                    }
                }
                else if(systick_irq_trigger == SYS_IRQ_TRIG_CS_REFL_RX_START){
                    if(ll_cs_reflector_irq_task_cb)//to save RamCode
                    {
                        ll_cs_reflector_irq_task_cb(FLAG_CS_STEP_REFL_SRX_START); //blt_cs_reflector_irq_task     blt_cs_refl_stepSrx
                    }
                }
                else if(systick_irq_trigger == SYS_IRQ_TRIG_CS_REFL_TX_START){
                    if(ll_cs_reflector_irq_task_cb)//to save RamCode
                    {
                        ll_cs_reflector_irq_task_cb(FLAG_CS_STEP_REFL_STX_START); //blt_cs_reflector_irq_task      blt_cs_refl_stepStx
                    }
                }
                else if(systick_irq_trigger == SYS_IRQ_TRIG_CS_REFL_TX_POST){
                    if(ll_cs_reflector_irq_task_cb){
                        ll_cs_reflector_irq_task_cb(FLAG_CS_STEP_REFL_STX_POST); //blt_cs_reflector_irq_task      blt_cs_refl_m0SysIrq_m1m2RfTxIrq_txDone
                    }
                }
            }
        #endif
        #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
            else if( systick_irq_trigger == SYS_IRQ_TRIG_SNIFM_SEEK_POST){
                //if(ll_acl_sniffer_mst_irq_task_cb) //to save RamCode
                {
                    ll_acl_sniffer_mst_irq_task_cb(FLAG_ACL_SNIFFER_SEEK_POST);  // blt_acl_sniffer_mst_irq_task() blt_sniffer_mst_seek_post()
                }
            }
        #endif
        #if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
            else if( systick_irq_trigger == SYS_IRQ_TRIG_SNIFS_SEEK_POST){
                //if(ll_acl_sniffer_slv_irq_task_cb) //to save RamCode
                {
                    ll_acl_sniffer_slv_irq_task_cb(FLAG_ACL_SNIFFER_SEEK_POST);  // blt_acl_sniffer_slv_irq_task() blt_sniffer_slv_seek_post()
                }
            }
        #endif
            else if (systick_irq_trigger == SYS_IRQ_TRIG_PRICHN_SCAN_POST){
                if(ll_prichn_scan_irq_task_cb)
                {
                    ll_prichn_scan_irq_task_cb(FLAG_SCHEDULE_DONE); //blt_prichn_scan_interrupt_task
                }
            }
            else if (systick_irq_trigger == SYS_IRQ_TRIG_SECCHN_SCAN_POST){
                //if(ll_secchn_scan_task_cb) //to save RamCode
                {
                    ll_secchn_scan_task_cb(FLAG_SCHEDULE_DONE);  //blt_ll_procAuxiliaryScanTask
                }
            }
            else if (systick_irq_trigger == SYS_IRQ_TRIG_SCHE_START){
                blt_ll_irq_startScheduler();
            }
        #if (LEG_ADV_DELAY_CTRL_EN || EXT_ADV_DELAY_CTRL_EN || LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE || LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
            else if (systick_irq_trigger == SYS_IRQ_TRIG_SCHE_INSERT){
                if(blms_state == BLMS_STATE_PRICHN_SCAN_S){
                    #if (LL_ACL_CEN_EN || LL_FEATURE_ENABLE_LE_EXTENDED_SCAN)
                        blt_ll_clear_prichn_scan_status();
                    #endif
                }
                else if(BLMS_STATE_SHORT_TASK_START){
                    write_dbg32(0x00018, blms_state);
                    BLMS_ERR_DEBUG(SCH_DEBUG_EN, 0xFF120000);
                }

                blms_state = BLMS_STATE_SCHE_INSERT;

                #if (DYNAMIC_SCHE_CAL_TIME_EN)
                    int sSlot_cost_num = (u32)(clock_time() + (bltSche.sche_process_us + 50)*SYSTEM_TIMER_TICK_1US - bltSche.sSlot_tick_irq_real)*SSLOT_TICK_REVERSE + 1;
                    bltSche.sSlot_idx_next = (bltSche.sSlot_idx_irq_real + sSlot_cost_num);
                    bltSche.sSlot_tick_next = bltSche.sSlot_tick_irq + sSlot_cost_num*SSLOT_TICK_NUM;
                #else
                    int sSlot_cost_num = (u32)(clock_time() + SLOT_PROCESS_MAX_TICK - bltSche.sSlot_tick_irq_real)*SSLOT_TICK_REVERSE + 1;
                    bltSche.sSlot_idx_next = (bltSche.sSlot_idx_irq_real  + sSlot_cost_num);
                    bltSche.sSlot_tick_next = bltSche.sSlot_tick_irq_real + sSlot_cost_num*SSLOT_TICK_NUM;
                #endif

                bltSche.pTask_cur->end = bltSche.sSlot_idx_next;


                my_dump_str_data(DBG_CIS_1ST_AP_TIMING_EN, "[CIS][TIM] ciss 1st ap create, insert 2", 0, 0);
            }
        #endif
    }
}

#if( OS_SUP_EN && BLMS_PM_ENABLE )
_attribute_ram_code_
int blc_pm_handler(void)
{

    /* sleep_allowed is IRQ variable, need check again;  bltSche.task_mask can be guaranteed by  sleep_allowed, no need check */
    if( ll_module_pm_cb &&  blmsParam.sdk_mainloop_flg &&
           blmsPm.sleep_mask != PM_SLEEP_DISABLE && ((blmsPm.sleep_taskMask & bltSche.task_mask) == bltSche.task_mask)
        && blmsPm.sleep_allowed && !blmsParam.state_chng)
    {
        int sleep_enter_en= 0;
        u32 r = irq_disable();

        if( blmsPm.sleep_allowed && !blmsPm.slave_no_sleep &&
             blt_rxfifo.rptr == blt_rxfifo.wptr && scan_priRxFifo.rptr == scan_priRxFifo.wptr &&
            !blmsParam.connectEvt_mask && !blmsParam.disconnEvt_mask && !blmsParam.conupdtEvt_mask && !blmsParam.phyupdtEvt_mask)
        {
        #if ACL_SLAVE_PM_LATENCY_EN
            u32 tick_margin = (blmsPm.pTask_wakeup->scheTask_flg == TSKFLG_ACL_SLAVE ? 10 : PM_MIN_SLEEP_US) *SYSTEM_TIMER_TICK_1US;
            if(tick1_exceed_tick2(blmsPm.next_task_tick, clock_time() + tick_margin))
        #else
            if(tick1_exceed_tick2(blmsPm.next_task_tick, clock_time() + PM_MIN_SLEEP_US * SYSTEM_TIMER_TICK_1US))
        #endif
            {
                sleep_enter_en = 1;
            }
        }

        irq_restore(r);

        if( sleep_enter_en )
        {
            systimer_irq_disable();
            u32 res = ll_module_pm_cb();  //blt_sleep_process()
            systimer_irq_enable();

            return res;//0:SLEEP; 1: NO SLEEP
        }
    }

    return 1; //NO SLEEP
}
#endif

#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
void blc_sdk_main_loop (void)
{
    //Notice that phytest must before all other operation, and return
    if(blc_main_loop_phyTest_cb && blmsParam.phytest_en){
        blc_main_loop_phyTest_cb();
        return;
    }

    /* Scan rx pkt process */
    if (scan_priRxFifo.rptr != scan_priRxFifo.wptr || scan_secRxFifo.rptr != scan_secRxFifo.wptr)
    {
        //must judge pointer none zero, can not save
        if(ll_leg_scan_mlp_task_cb && (IS_LEGACY_SCAN_VALID))
        {
            ll_leg_scan_mlp_task_cb(FLAG_SCAN_DATA_REPORT);  // blt_leg_scan_mainloop_task  blt_ll_procLegacyScanData
        }

        if(ll_ext_scan_mlp_task_cb && (IS_EXTENDED_SCAN_VALID))
        {
            ll_ext_scan_mlp_task_cb(FLAG_SCAN_DATA_REPORT);  // blt_ext_scan_mainloop_task  blt_ll_procExtAdvReportEvent
        }
    }

    #if (LL_FEATURE_ENABLE_LE_EXTENDED_SCAN)
        if(ll_ext_scan_mlp_task_cb){
            ll_ext_scan_mlp_task_cb(FLAG_EXT_SCAN_MAINLOOP_PEND_TASK); //blt_ext_scan_mainloop_task
        }
    #endif

        if(ll_leg_adv_mlp_task_cb)
        {
            ll_leg_adv_mlp_task_cb(FLAG_MODULE_MAINLOOP); // blt_leg_adv_mainloop_task
        }

    #if (LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING)
        if(ll_ext_adv_mlp_task_cb)
        {
            ll_ext_adv_mlp_task_cb(FLAG_MODULE_MAINLOOP, NULL); // blt_ext_adv_mainloop_task
        }
    #endif

    if(ll_acl_conn_mlp_task_cb)
    {
        ll_acl_conn_mlp_task_cb(FLAG_MODULE_MAINLOOP, NULL); //blt_acl_conn_mainloop_task()
    }

    #if (LL_FEATURE_ENABLE_CONNECTED_ISO)
        if(ll_cis_conn_mlp_task_cb)
        {
            ll_cis_conn_mlp_task_cb(FLAG_MODULE_MAINLOOP, NULL); //blt_cis_conn_mainloop_task
        }
    #endif

    #if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING)
        if(ll_prd_adv_mlp_task_cb)
        {
            ll_prd_adv_mlp_task_cb(FLAG_MODULE_MAINLOOP, NULL); //blt_prd_adv_mainloop_task
        }
        #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
            if(ll_pawra_mlp_task_cb)
            {
                ll_pawra_mlp_task_cb(FLAG_MODULE_MAINLOOP, NULL); //blt_pawra_mainloop_task
            }
        #endif
    #endif

    #if(LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER)
        if(ll_big_bcst_mlp_task_cb)
        {
            ll_big_bcst_mlp_task_cb(FLAG_MODULE_MAINLOOP, NULL); /// blt_big_bcst_mainloop_task()
        }
    #endif
    
    #if (LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER)
        if(ll_big_sync_mlp_task_cb)
        {
            ll_big_sync_mlp_task_cb(FLAG_MODULE_MAINLOOP,NULL); /// blt_big_sync_mainloop_task()
        }
    #endif

    #if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC)
        if (ll_pda_sync_mlp_task_cb)
        {
            ll_pda_sync_mlp_task_cb(FLAG_MODULE_MAINLOOP); //blt_pda_sync_mainloop_task
        }
    #endif


    #if (LL_FEATURE_ENABLE_LE_AOA_AOD)
        if(ll_aoa_aod_mlp_task_cb)
        {
            ll_aoa_aod_mlp_task_cb(FLAG_MODULE_MAINLOOP); //blt_aoa_aod_mainloop_task
        }
    #endif



    #if(LL_FEATURE_ENABLE_PRIVACY)
        /* can not save resolving list relative variable, because peer device may use RPA,
         * so here no t using pointer */
        if(blt_ll_resolvIsLocalRpaUsed()){
            blt_ll_resolvRpaTimeoutLoop();
        }
    #endif


    #if(LL_FEATURE_ENABLE_CHANNEL_SOUNDING)
        if(ll_chn_sounding_mlp_task_cb){
            ll_chn_sounding_mlp_task_cb(FLAG_MODULE_MAINLOOP, NULL);//blt_cs_mainloop_task
        }
    #endif


    #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
        if(ll_acl_sniffer_mst_mlp_task_cb){
            ll_acl_sniffer_mst_mlp_task_cb(FLAG_MODULE_MAINLOOP); // blt_acl_sniffer_mst_mainloop_task() blt_ll_acl_sniffer_mst_mainloop()
        }
    #endif

    #if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
        if(ll_acl_sniffer_slv_mlp_task_cb){
            ll_acl_sniffer_slv_mlp_task_cb(FLAG_MODULE_MAINLOOP); // blt_acl_sniffer_slv_mainloop_task() blt_ll_acl_sniffer_slv_mainloop()
        }
    #endif


    //------------------   HCI -------------------------------
    if (blc_hci_rx_handler){
        blc_hci_rx_handler ();          // HCI_Tr_RxHandlerCback
    }
    if (blc_hci_tx_handler){
        blc_hci_tx_handler ();          // HCI_Tr_TxHandlerCback
    }



    #if (CONTROLLER_GEN_P256KEY_ENABLE) //HCI pending event process
        if(blmsParam.getP256pubKeyEvtPending){
            blt_ll_procGetP256pubKeyEvent();
        }
        else if(blmsParam.generateDHkeyEvtPending){
            blt_ll_procGenDHkeyEvent();
        }
    #endif


    if(ll_host_main_loop_cb){
        ll_host_main_loop_cb();  //blt_gap_mainloop
    }

    if(bltSche.task_mask){
        // manual reset for slot index: add this to avoid slot_idx 0xffffffff -> 0 bug (about 31 days will happen)
        //if slot_idx is too big(e.g. BIT(31) is about 15 days), clear slot_idx when no connection
        if( ((bltSche.bSlot_idx_start & SLOT_INDEX_ALARM_LOW) == SLOT_INDEX_ALARM_LOW) && !blm_btxbrx_state){
            blt_ll_proc_bSlot_idx_overflow();
        }

        #if 0 //debug
            static u32 now_tick = 0;
            if(clock_time_exceed(now_tick, 1000000)){
                now_tick = clock_time();
                my_dump_str_u32s(0, "bSlot", bltSche.bSlot_idx_start, bltSche.bSlot_idx_next, bltSche.bSlot_idx_irq_real, 0);
            }
        #endif
    }
    else{
        if( blmsParam.sche_run_flag==0 && blmsParam.state_chng){
            blt_ll_mainloop_startScheduler();
        }
    }



    mcu_oscillator_crystal_calibration();

#if (BLMS_PM_ENABLE)
    /* 20240221 SiHui & RongLu & YueXin
     * attention: temporary calibration for fast settle, now only B92 tested !!!
     * 1. under condition PM enable. Actually we should consider PM disable situation
     * 2. fast settle timing 5.5mS may not be find for some case, e.g, primacy scan always on, 4 ACL slave with 7.5mS interval
     * Future stable calibration solution will be calibrated in scheduler idle timing.
     * */
    #if 0 //(FAST_SETTLE) // todo ronglu Due to high temperature disconnection in stability test, it is temporarily disabled
        #define FAST_SETTLE_CAL_INTERVAL_US         60000000            // cal per minute, 60S
        #define FAST_SETTLE_EXPEND_TIME_US          5500                // cal expend 4.8ms

        if(clock_time_exceed(fast_settle_cal_tick, FAST_SETTLE_CAL_INTERVAL_US))
        {
            int fs_cal_en = 0; //fast settle calibration enable
            u32 r = irq_disable();
            if(blmsPm.sleep_allowed && tick1_exceed_tick2(blmsPm.next_task_tick, clock_time() + FAST_SETTLE_EXPEND_TIME_US * SYSTEM_TIMER_TICK_1US)){
                fs_cal_en = 1;
            }
            irq_restore(r);

            if(fs_cal_en){
                fast_settle_cal_tick = clock_time();
                rf_tx_fast_settle_dis();
                rf_rx_fast_settle_dis();
                blc_ll_initFastSettle(1,1);
            }
        }
    #endif

    blmsPm.sleep_enter_flag = 0;

   #if !OS_SUP_EN
    /* sleep_allowed is IRQ variable, need check again;  bltSche.task_mask can be guaranteed by  sleep_allowed, no need check */
    if( ll_module_pm_cb &&  blmsParam.sdk_mainloop_flg && \
           blmsPm.sleep_mask != PM_SLEEP_DISABLE && ((blmsPm.sleep_taskMask & bltSche.task_mask) == bltSche.task_mask) \
        && blmsPm.sleep_allowed && !blmsParam.state_chng)
  #else
    if( ll_module_pm_cb &&  blmsParam.sdk_mainloop_flg && \
           blmsPm.sleep_mask != PM_SLEEP_DISABLE && ((blmsPm.sleep_taskMask & bltSche.task_mask) == bltSche.task_mask) \
        && blmsPm.sleep_allowed && !blmsParam.state_chng && !blt_isOsSupEnable())
   #endif
    {
        int sleep_enter_en= 0;
        u32 r = irq_disable();


        if( blmsPm.sleep_allowed && !blmsPm.slave_no_sleep && \
             blt_rxfifo.rptr == blt_rxfifo.wptr && scan_priRxFifo.rptr == scan_priRxFifo.wptr && \
            !blmsParam.connectEvt_mask && !blmsParam.disconnEvt_mask && !blmsParam.conupdtEvt_mask && !blmsParam.phyupdtEvt_mask){
            #if ACL_SLAVE_PM_LATENCY_EN
                u32 tick_margin = (blmsPm.pTask_wakeup->scheTask_flg == TSKFLG_ACL_SLAVE ? 10 : PM_MIN_SLEEP_US) *SYSTEM_TIMER_TICK_1US;
                if(tick1_exceed_tick2(blmsPm.next_task_tick, clock_time() + tick_margin))
            #else
                if(tick1_exceed_tick2(blmsPm.next_task_tick, clock_time() + PM_MIN_SLEEP_US * SYSTEM_TIMER_TICK_1US))
            #endif
                {
                    sleep_enter_en = 1;
                }
        }

        irq_restore(r);

        if( sleep_enter_en ){
            systimer_irq_disable();
            ll_module_pm_cb();  //blt_sleep_process()
            systimer_irq_enable();
        }
    }

#if (MCU_STALL_EN)
    //note:the mcu stall must be placed after ll_module_pm_cb.
    //case 1:As long as not entry PM, mcu stall mode can run according to situation.
    //case 2:Exit from pm,need to run mainloop one time at least. Because I think application need to run once at least.
    //!blmsPm.sleep_allowed: if one task post IRQ occur between 'PM processing above' and 'the following code'.
    if(ll_module_pm_cb && pm_mcuStall_allowFlag && !blmsPm.sleep_enter_flag && !blmsPm.sleep_allowed){

        if(blmsParam.sche_run_flag==1){ //here ensure must exist RF irq or STimer irq.

            if(blt_rxfifo.rptr == blt_rxfifo.wptr && scan_priRxFifo.rptr == scan_priRxFifo.wptr){
                //all interrupt can wake up chip from wfi mode. and all chips of the SDK include this API.
                core_entry_wfi_mode();
            }
        }
    }
#endif

#endif

    blmsParam.sdk_mainloop_flg = 1;

}


#if (LL_ACL_CEN_EN || LL_FEATURE_ENABLE_LE_EXTENDED_SCAN)
_attribute_ram_code_
void    blt_ll_clear_prichn_scan_status(void)
{
    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_OFF);  }
    blmsParam.rf_fsm_busy = 0;
    rf_set_tx_rx_off();

    /* very important to clear RX status: boundary RX packet may enter other state
       consider timing margin, we clear it after a while, so add a mark here, clear it later */
    blmsParam.delay_clear_rf_status = 1;
}
#endif
