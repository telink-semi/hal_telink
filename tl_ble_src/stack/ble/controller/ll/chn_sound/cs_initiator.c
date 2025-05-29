/********************************************************************************************************
 * @file    cs_initiator.c
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
#if (LL_FEATURE_ENABLE_CHANNEL_SOUNDING_INITIATOR)

#define CS_MODE0_INITIATOR_SAMPLE_NUM           320

/******************************IRQ handle*******************************************/
int blt_cs_init_step_stx_start(void);
int blt_cs_init_step_srx(void);
int blt_cs_initiator_rx(void);
int blt_cs_init_step_post(void);

/***********************************************************************************/

    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ int
    blt_cs_init_step_stx_start(void)
{
    blms_state = BLMS_STATE_CS_INIT_TX_S;

    u32 permu  = 0;
    slip_window_step_t *pStepInit = blt_cs_getSlipWindow();
    u8                  chn       = pStepInit->step_chnIdx;
    u8                  mode      = pStepInit->step_modeType;
    tlkapi_send_string_u8s(CS_TIMING_DEBUG_LOG_EN, "step index", chn);
    #if (SL01_cs_step_init_0)
    log_task_begin_irq(SL_STACK_CS_TIME_EN, SL01_cs_step_init_0);
    #endif

    DBG_CS_CHN2_HIGH;
    //set channel must do before rf_manual_fcal_start
    ble_rf_set_cs_channel(chn);
    u32 tick_fcal_start = 0;
    //enable manual frequency calibration.
    if (gCsMng.blt_pCsCfg->phaseContinue_cal_flag) {
        //48MHZ 9us
        //        DBG_CS_CHN3_HIGH;
        rf_manual_fcal_start();
        tick_fcal_start = clock_time();
    }

    #if 0
    /****************just for debug start**************************/
    u32 reflAA = pStepInit->step_reflAA;
    u8  extSlotAll  = pStepInit->extSlot.step_extSlotFlag;
    u32 chnModeIeRe = chn<<24 | mode<<16 | extSlotAll;
    tlkapi_send_string_u32s(CS_TIMING_DEBUG_LOG_EN|1, "stxCntiAArAAIdx", gCsMng.blt_pCsCfg->csProcCount,pStepInit->step_initAA, reflAA, chnModeIeRe);
    /****************just for debug end**************************/
    #endif

    u8 tx_early_us  = (gCsMng.blt_pCsCfg->CS_SYNC_PHY == BLE_1M_PHY) ? CS_tx_early_1M_phy[mode] : CS_tx_early_2M_phy[mode];
    u8 tx_settle_us = gCsMng.blt_pCsCfg->phaseContinue_cal_flag ? CS_COMMON_TX_SETTLE_US : TX_STL_TIFS_REAL_COMMON;
    rf_ble_set_tx_settle(tx_settle_us-1);

    u8  t_sw_us  = (mode == STEP_MODE_2) ? gCsMng.blt_pCsCfg->T_SW_Us : 0;
    u32 tx_tick  = gCsMng.blt_pCsCfg->step_expect_tick + t_sw_us * SYSTEM_TIMER_TICK_1US - (tx_early_us + tx_settle_us) * SYSTEM_TIMER_TICK_1US; //gCsMng.blt_pCsCfg->T_SW_Us
    u32 tick_now = clock_time();
    if (tick1_exceed_tick2(tick_now, tx_tick)) {
        write_dbg32(0x0018, tick_now);
        write_dbg32(0x001C, tx_tick);
        u32 diff = (u32)(tick_now - tx_tick) / SYSTEM_TIMER_TICK_1US;
        tlkapi_send_string_u32s(CS_TIMING_DEBUG_LOG_EN, "cs start", diff, gCsMng.blt_pCsCfg->step_expect_tick, gCsMng.blt_pCsCfg->tick_expect_csSubevent, mode);
        BLMS_ERR_DEBUG(DBG_CS_SCH_INIT, 0xBBff0100);
    }


    //wait manual facl done
    if (gCsMng.blt_pCsCfg->phaseContinue_cal_flag) {
        //as close as possible to FSM, thus can save some time.
        // 48MHZ 17.5us
        while (!tick1_exceed_tick2(clock_time(), tick_fcal_start + RF_FCAL_MANUAL_START2DONE_TIME_US * SYSTEM_TIMER_TICK_1US)) {
        }
        //48M 5.3us
        rf_manual_fcal_done();
        //        DBG_CS_CHN3_LOW;
    }

    if (mode == STEP_MODE_0) {
        rf_start_fsm(FSM_STX, (void *)&pkt_CS_m0, tx_tick); // change trailer to 16bits
    }
    else if (mode == STEP_MODE_1) {
        if(gCsMng.blt_pCsCfg->CS_SYNC_PHY == BLE_1M_PHY){
            rf_start_fsm(FSM_STX, (void *)&pkt_CS_m1, tx_tick);
        }
        else{
            rf_start_fsm(FSM_STX, (void *)&pkt_CS, tx_tick);
        }
    }
    else {
        rf_start_fsm(FSM_STX, (void *)&pkt_CS, tx_tick);
    }


    if (mode == STEP_MODE_0) {
        //48MHZ  4.7us
        // mode0 packet only have 4 bits trailer after sync access address, it's too short and will influence
        // mode0 RSSI value(abnormal value: 0x7F), send 16bits trailer to avoid. -- qinghua, biao, yuexin, sync with xuqiang.
        blt_cs_mode0_packetSyncPDU(&pkt_CS_m0, pStepInit->step_initAA);
        reg_rf_ll_irq_list_h = BIT(0); //clear tr_turnaround_irq
    } else if (mode == STEP_MODE_1) {
        #if (HAILI_REQUIRE_MODE1_PREAMBLE_1B)
            if(gCsMng.blt_pCsCfg->CS_SYNC_PHY == BLE_1M_PHY){
                blt_cs_packetSyncPDU_mode1_cali(&pkt_CS_m1, pStepInit->step_initAA, pStepInit, CS_INITIATOR_ROLE);
            }
            else{
                blt_cs_packetSyncPDU(&pkt_CS, pStepInit->step_initAA, pStepInit, CS_INITIATOR_ROLE);
            }
        #else
            blt_cs_packetSyncPDU(&pkt_CS, pStepInit->step_initAA, pStepInit, CS_INITIATOR_ROLE);
        #endif
        reg_rf_ll_irq_list_h = BIT(0); //clear tr_turnaround_irq
    } else if (mode == STEP_MODE_2) {
        //48MHZ 2.3us
        ble_rf_set_tx_modulation_index(RF_MI_P0p00);
        u32 t_pm_us    = (pStepInit->extSlot.step_extSlotInit ? gCsMng.blt_pCsCfg->mode2ToneUs : gCsMng.blt_pCsCfg->mode2ToneUs_noExtslot) - gCsMng.blt_pCsCfg->T_SW_Us; //'- t_sw' : because RF machine rx will start to receive after T_SW.
        if(gCsMng.blt_pCsCfg->stable_phase_en){
            t_pm_us    = 680;
        }
        pkt_CS.dma_len = rf_tx_packet_dma_len(CAL_LL_CS_TONE_TX_SIZE(t_pm_us));

    } else {
        BLMS_ERR_DEBUG(DBG_CS_SCH_INIT, 0xBBEE0000);
    }

    if (mode == STEP_MODE_2) {
        permu = gCsMng.blt_pCsCfg->perm_table[pStepInit->step_antPathPermIdx];
    } else // mode0,mode1. not consider mode3
    {
        permu = gCsMng.blt_pCsCfg->perm_table[24 + (gCsMng.blt_pCsCfg->sync_pky_cnt & 0x03)];
        gCsMng.blt_pCsCfg->sync_pky_cnt++;
    }

    if(ant_ctrl_cfg.set_ant_permu_func){
        ant_ctrl_cfg.set_ant_permu_func (permu);
    }


    //Consider mode0 and mode2, the time is not the same
    //  systimer_set_irq_capture(tx_tick + (gCsMng.blt_pCsCfg->t_synu_us + tx_early_us + tx_settle_us + TLK_TM_DELAY)*SYSTEM_TIMER_TICK_1US); //8 us = 1preamble extra
    systick_irq_trigger = SYS_IRQ_TRIG_CS_INIT_SRX;

    if (blc_rf_pa_cb) {
        blc_rf_pa_cb(PA_TYPE_TX_ON);
    }

    return 0;
}

    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ int
    blt_cs_init_step_srx(void)
{
    blms_state = BLMS_STATE_CS_INIT_RX_S;


    u16 iq_sample_number = 0;
    u8  start_point      = 0;
    u8  rx_early_us      = (gCsMng.blt_pCsCfg->CS_SYNC_PHY == BLE_2M_PHY) ? CS_RFRXEN_MODE_2M_EARLY_US : CS_RFRXEN_MODE_1M_EARLY_US;
    u32 rx_tick          = 0;
    u32 first_timeout    = 0;


    rf_cs_iq_sample_mode_e cs_rx_mode = RF_CS_IQ_SAMPLE_RXEN_MODE;

    slip_window_step_t *pStepInit = blt_cs_getSlipWindow();
    u8                  mode      = pStepInit->step_modeType;
    u32                 ac        = pStepInit->step_reflAA;

    u8 rx_settle_us = gCsMng.blt_pCsCfg->phaseContinue_cal_flag ? CS_COMMON_RX_SETTLE_US : RX_SETTLE_US;
    rf_ble_set_rx_settle(rx_settle_us-1);

    cs_rx_fifo.pCsRxAddr = cs_rx_fifo.p_base + (cs_rx_fifo.wptr & cs_rx_fifo.mask) * cs_rx_fifo.size;
    cs_rx_fifo.pCsRxAddr[0] = 0;
    cs_rx_fifo.pCsRxAddr[1] = 0;
    cs_rx_fifo.pCsRxAddr[2] = 0;
    ble_rf_set_rx_dma(cs_rx_fifo.pCsRxAddr, cs_rx_fifo.size_div_16);

    if (((u8)((cs_rx_fifo.wptr - cs_rx_fifo.rptr) & 127)) >= cs_rx_fifo.num) {
        tlkapi_send_string_u32s(DBG_CS_SCH_INIT, "cs rx fifo overflow - start", cs_rx_fifo.wptr, cs_rx_fifo.rptr);
        BLMS_ERR_DEBUG(DBG_CS_SCH_INIT, 0xBBff0500);
    }

    if (mode == STEP_MODE_0) {
        /**iq_sample_number: 0.25us unit (4MHz) ---> 0.25us * 320 = 80us ---> T_FM
         * iq_start_point: sync mode, sampling starts at (start_point + 1) * 0.125us after sync
         *      (111 + 1) * 0.125us = 14us = 4 + 10 ---> Trailer + T_GD
         */
        start_point      = 111;
        iq_sample_number = CS_MODE0_INITIATOR_SAMPLE_NUM;
        cs_rx_mode       = RF_CS_IQ_SAMPLE_SYNC_MODE;
        rf_set_ble_access_code((u8 *)&ac);

        /*1st rx timeout: rx_early_us + RX_SETTLE_US + T_SY + T_RD + margin_15us
                          rx_early_us + RX_SETTLE_US + 44   + 5    + 15         = rx_early_us + RX_SETTLE_US + 64
        */
        rx_early_us += 8;//margin
        first_timeout = rx_early_us * 2 + rx_settle_us + 64;
        //Theoretical rx point = tick_step_start + T_SY + T_RD + T_IP1 = tick_step_start + 44 + 5 + T_IP1 = tick_step_start + T_FCS + T_IP1 + 49
        rx_tick = gCsMng.blt_pCsCfg->step_expect_tick + (gCsMng.blt_pCsCfg->mode0TxIntvalUs - rx_early_us - rx_settle_us) * SYSTEM_TIMER_TICK_1US;

    } else if (mode == STEP_MODE_2) {
        start_point = 6;
        //todo check with lijing
        //        rf_cs_set_power_off_singletone();
        ble_rf_set_tx_modulation_index(RF_MI_P0p50);

        u32 t_pm_us = (pStepInit->extSlot.step_extSlotRefl ? gCsMng.blt_pCsCfg->mode2ToneUs : gCsMng.blt_pCsCfg->mode2ToneUs_noExtslot) - gCsMng.blt_pCsCfg->T_SW_Us; //because RF machine rx will start to receive after T_SW.

        iq_sample_number = (t_pm_us + rx_early_us) << 2;

        //1st rx timeout: RX_SETTLE_US + rx_early_us + T_PM + extension_slot + margin_25us
        //                RX_SETTLE_US + rx_early_us + T_PM + T_PM           + 25           = RX_SETTLE_US + rx_early_us + (T_PM_us << 1) + 25

        first_timeout = rx_settle_us + rx_early_us + t_pm_us + 25;
        rx_tick       = gCsMng.blt_pCsCfg->step_expect_tick + (gCsMng.blt_pCsCfg->mode2TxIntvalUs + gCsMng.blt_pCsCfg->T_SW_Us - rx_early_us - rx_settle_us) * SYSTEM_TIMER_TICK_1US; //5--RD(ramp down)

    } else if (mode == STEP_MODE_1) {
        u32 tick_tx_done = clock_time();
        //rf_agc_disable();
        start_point = 1;
    /**iq_sample_number: 0.25us unit (4MHz) ---> 0.25us * 1 = 0.25us
         * iq_start_point: rx_en mode, sampling starts at 0.25us+start_point*0.125us after settle
         *      0.25us + 1*0.125us = 0.375us
         */
    #if !(HAILI_REQUIRE_MODE1_PREAMBLE_1B)
        rx_early_us += CS_RF_RX_1M_EXTRA_PREAMBLE_US; //5us(1M PHY RXPATHDLY) RXEN MODE + 1 byte for preamble extra length
    #endif
        u8 rx_extend_us  = CS_RF_RX_1M_WINDOW_EXTEND_US; // ensure correct Sync
        iq_sample_number = (rx_early_us + gCsMng.blt_pCsCfg->none_mode_sync_us + rx_extend_us) << 2;

        rf_set_ble_access_code((u8 *)&ac);               //must

        //1st rx timeout: rx_settle_us + rx_early_us + gCsMng.blt_pCsCfg->none_mode_sync_us + rx_extend_us  + margin_25us
        //          =     rx_settle_us + rx_early_us + gCsMng.blt_pCsCfg->none_mode_sync_us + rx_extend_us  + 25
        first_timeout = rx_settle_us + rx_early_us + gCsMng.blt_pCsCfg->none_mode_sync_us + rx_extend_us + 25;
        rx_tick       = gCsMng.blt_pCsCfg->step_expect_tick + (gCsMng.blt_pCsCfg->mode1TxIntvalUs - rx_early_us - rx_settle_us) * SYSTEM_TIMER_TICK_1US;

        while ((!(reg_rf_ll_irq_list_h & BIT(0))) && (!tick1_exceed_tick2(clock_time(), tick_tx_done + 15 * SYSTEM_TIMER_TICK_1US))) {
        }
        reg_rf_ll_irq_list_h = BIT(0);                                                   //clear tr_turnaround_irq
        cs_tick_tx_on        = rf_cs_get_timestamp();                                    //now this time obtained is tx_on
    }

    ble_rf_channel_sounding_iq_sample_config(iq_sample_number, start_point, cs_rx_mode); //iq_sample_number cannot set to 0


    u32 clock_now = clock_time();
    if (tick1_exceed_tick2(clock_now, rx_tick)) {
        write_dbg32(0x0018, clock_now);
        write_dbg32(0x001C, rx_tick);
        BLMS_ERR_DEBUG(DBG_CS_SCH_INIT, 0xBBff0200);
    }


    rf_start_fsm(FSM_SRX, NULL, rx_tick);
    if (blc_rf_pa_cb) {
        blc_rf_pa_cb(PA_TYPE_RX_ON);
    }
    rf_set_1st_rx_timeout(first_timeout);
    gCsMng.blt_pCsCfg->step_rx_flag = 0;
    systick_irq_trigger             = SYS_IRQ_TRIG_CS_STEP_POST;

    //    DBG_CS_CHN5_TOGGLE;DBG_CS_CHN5_TOGGLE;

    return 0;
}

    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ int
    blt_cs_init_step_post(void)
{
    u8                  subevent_abort = 0;
    u16                 step_duration_us = 0;
    slip_window_step_t *pStepInit      = blt_cs_getSlipWindow();
    u8                  se_end         = pStepInit->subeventEndFlag;
    u8                  init_proc_end  = pStepInit->proceStopFlag;

    #if (SL01_cs_step_init_0)
    log_task_end_irq(SL_STACK_CS_TIME_EN, SL01_cs_step_init_0);
    #endif

    blms_state         = BLMS_STATE_CS_INIT_E;
    u8  mode           = pStepInit->step_modeType;

 /////////////////////////////////////////////////////////////////////////
    cs_rx_flag  *pRxFlag = (cs_rx_flag*)&cs_rx_fifo.pCsRxAddr[2];
    cs_rx_para_t *cs_rx_para = NULL;

    if(gCsMng.blt_pCsCfg->step_rx_flag)
    {
        pRxFlag->flag.rx_valid = 1;
    }

    rf_set_tx_rx_off();
    STOP_RF_STATE_MACHINE;
    reg_rf_irq_status = FLD_RF_IRQ_TX | FLD_RF_IRQ_RX | FLD_RF_IRQ_CMD_DONE | FLD_RF_IRQ_FIRST_TIMEOUT;

    if (blc_rf_pa_cb) {
        blc_rf_pa_cb(PA_TYPE_OFF);
    }


    u16 dma_len = cs_rx_fifo.pCsRxAddr[0] | (cs_rx_fifo.pCsRxAddr[1]<<8);
    u8  rx_early_us      = CS_RFRXEN_MODE_1M_EARLY_US;

    if(gCsMng.blt_pCsCfg->CS_SYNC_PHY == BLE_2M_PHY){
        rx_early_us      = CS_RFRXEN_MODE_2M_EARLY_US;
    }
    switch(mode){
        case STEP_MODE_0:
        {
            slip_window_step_t  *pNextStep = blt_cs_getNextSlipWindow();
            if((!pRxFlag->flag.rx_valid) ||
                    (dma_len != (CS_MODE0_INITIATOR_SAMPLE_NUM *5 + CS_RX_HD_EXT_LEN)))
            {
                cs_rx_fifo.pCsRxAddr[0] = 0x6c;
                cs_rx_fifo.pCsRxAddr[1] = 0x06;
                pRxFlag->flag.rx_valid = 0;
            }
            cs_rx_para = (cs_rx_para_t *)(cs_rx_fifo.pCsRxAddr + DMA_CS_RFRX_OFFSET_TIME_STAMP(cs_rx_fifo.pCsRxAddr));

            if ((pNextStep->step_modeType != STEP_MODE_0)) {
                if (gCsMng.blt_pCsCfg->mode0_rx_flag) {
                    rf_agc_disable();
                    cs_rx_agc_gain = rf_get_gain_lat_value();
                    tlkapi_send_string_u32s(CS_TIMING_DEBUG_LOG_EN, "agc gain ", cs_rx_agc_gain);
                } else {
                    se_end                        = 1;
                    slip_window_step_t *pStepLast = blt_cs_getLastSlipWindow();
                    if (pStepLast->proceStopFlag) {
                        init_proc_end = 1;
                    }
                    subevent_abort = (SUBEVT_ABORT_NO_MODE0_RECEIVED << 4) & 0xF0;
                }
            }
            break;
        }
        case STEP_MODE_1:
        {

            u32 iq_sample_len = (rx_early_us + gCsMng.blt_pCsCfg->none_mode_sync_us + CS_RF_RX_1M_WINDOW_EXTEND_US)
                                    *20 + CS_RX_HD_EXT_LEN;
            if((!pRxFlag->flag.rx_valid) || (dma_len != iq_sample_len))
            {
                cs_rx_fifo.pCsRxAddr[0] = 0x6c;
                cs_rx_fifo.pCsRxAddr[1] = 0x06;
                pRxFlag->flag.rx_valid = 0;
            }
            cs_rx_para = (cs_rx_para_t *)(cs_rx_fifo.pCsRxAddr + DMA_CS_RFRX_OFFSET_TIME_STAMP(cs_rx_fifo.pCsRxAddr));

            cs_rx_para->tx_on_tstamp      = cs_tick_tx_on;
            cs_rx_para->rx_access_address = pStepInit->step_reflAA;
            #if (MODE1_FINE_RTT &&((CHIP_TYPE == CHIP_TYPE_TL721X)||(CHIP_TYPE == CHIP_TYPE_TL322X)))
            slip_window_step_t *pStepCur = blt_cs_getSlipWindow();
            smemcpy(&cs_rx_para->step_RttSeq[0],&pStepCur->step_reflRttSeq[0],16);
            cs_rx_para->step_RttSSPos[0] = pStepCur->step_reflRttSSPos[0];
            cs_rx_para->step_RttSSPos[1] = pStepCur->step_reflRttSSPos[1];
            #endif
            break;
        }
        case STEP_MODE_2:
        {

            u32 t_pm_us = (pStepInit->extSlot.step_extSlotRefl ? gCsMng.blt_pCsCfg->mode2ToneUs : gCsMng.blt_pCsCfg->mode2ToneUs_noExtslot)
                    - gCsMng.blt_pCsCfg->T_SW_Us;

            u32 iq_sample_len = (rx_early_us + t_pm_us )*20 + CS_RX_HD_EXT_LEN;

            if((!pRxFlag->flag.rx_valid) || (dma_len != iq_sample_len))
            {
              cs_rx_fifo.pCsRxAddr[0] = 49;
              cs_rx_fifo.pCsRxAddr[1] = 0;
              pRxFlag->flag.rx_valid = 0;

              tlkapi_send_string_u32s((stkLog_mask & STK_LOG_LL_CS), "mode2 error ",pRxFlag->flag.rx_valid,  dma_len, iq_sample_len);
            }
            cs_rx_para = (cs_rx_para_t *)(cs_rx_fifo.pCsRxAddr + DMA_CS_RFRX_OFFSET_TIME_STAMP(cs_rx_fifo.pCsRxAddr));

            cs_rx_para->tick_cs_proc_start = gCsMng.blt_pCsCfg->tick_proc_start;
            cs_rx_para->ant_path_perm_idx = pStepInit->step_antPathPermIdx;

            pRxFlag->flag.ext_slot = pStepInit->extSlot.step_extSlotRefl ? 1 : 0;

            break;
        }
        default:{
            break;
        }
    }
    if(cs_rx_para==NULL){

        BLMS_ERR_DEBUG(DBG_CS_SCH_INIT, 0xBBff1B00);
        return -1;
    }


    pRxFlag->flag.mode =  mode;
    pRxFlag->flag.role = CHANNEL_SOUNDING_ROLE_INITIATOR;

    cs_rx_fifo.pCsRxAddr[3] = pStepInit->step_chnIdx;
    cs_rx_para->start_acl_conn_event  = gCsMng.blt_pCsCfg->cs_inst_acl;
    cs_rx_para->procedure_counter     = gCsMng.gGlobal_pCsCfg->csProcCount;
    cs_rx_para->procedure_done_status = ((init_proc_end) ? 0 : 1);
    cs_rx_para->subevent_done_status  = subevent_abort ? subevent_abort : (se_end ? 0 : 1);
    cs_rx_para->rx_agc_gain           = cs_rx_agc_gain;
    cs_rx_para->config_struct_addr    = gCsMng.blt_pCsCfg;

    gCsMng.blt_pCsCfg->slip_stepReadIdx++;

    cs_rx_fifo.wptr++;
    #if OS_SUP_EN //todo:
    if (blt_os_semCountIncrementIrq_cb) {
        blt_os_semCountIncrementIrq_cb();
    }
    #endif

    #if (DBG_CS_DATA_EN && LL_CS_SNIFFER_MODE_ENABLE)
    if ((u8)(cs_rx_fifo.wptr - cs_rx_fifo.rptr) > cs_rx_fifo.num) {
        tlkapi_send_string_u8s(DBG_CS_DATA_EN, "[STK][CS] initiator cs_rx_fifo overflow", mode, cs_rx_fifo.wptr, cs_rx_fifo.rptr, cs_rx_fifo.num);
        write_dbg32(DBG_SRAM_ADDR + 4, mode);
        write_dbg32(DBG_SRAM_ADDR + 8, cs_rx_fifo.wptr);
        write_dbg32(DBG_SRAM_ADDR + 12, cs_rx_fifo.rptr);
        BLMS_ERR_DEBUG(DBG_CS_DATA_EN, 0x55550002);
    }
    #endif
    ////////// for CS Subevent Result Event end //////////
    DBG_CS_CHN2_LOW;

    if (se_end) {
        if (init_proc_end) {
            DBG_CS_CHN3_TOGGLE;
            gCsMng.blt_pCsCfg->stopSch = 1;
            blt_sche_addUpdate(SLOT_UPDT_CHANNEL_SOUNDING_STATE_CHANGE);
        }
        blt_cs_subevent_post(init_proc_end);
    } else {
        if (mode == STEP_MODE_0) {
            /**1M PHY, receive succeed Air Timing
             *      Access_Address + Trailer + T_GD + T_FM + T_RD
             *      timestamp           4       10      80      5 = 99
             *
             * 1M PHY, IQ Sample Rx Path Delay Timing
             *      RXPATHDLY
             *          5   //RXEN MODE
             *          12  //SYNC MODE
             *
             *  Total time consumption = 99 - RXPATHDLY = 94    //RXEN MODE
             *  Total time consumption = 99 - RXPATHDLY = 87    //SYNC MODE
             */


            /**1M PHY, receive lose Air Timing
             *      T_SY + T_RD +   T_IP1 +     T_SY + T_GD + T_FM + T_RD  + T_FCS
             *      44      5       T_IP1_us    44      10      80      5  + T_FCS_us
             *
             *  Total time consumption = T_FCS_us + T_IP1_us + 188
             */
            step_duration_us = gCsMng.blt_pCsCfg->mode0Step_durUs;
            systick_irq_trigger = SYS_IRQ_TRIG_CS_INIT_TX_START;

        } else if (mode == STEP_MODE_2) {
            /**
             *      T_FCS + T_PM + extension_slot + T_RD +  T_IP2 + T_PM + extension_slot + T_RD
             *      T_FCS   T_PM        T_PM        5       T_IP2   T_PM        T_PM        5
             *
             *  Total time consumption = T_FCS_us + T_PM_us*4 + T_IP2_us + 10
             */
            step_duration_us = gCsMng.blt_pCsCfg->mode2Step_durUs;
        } else if (mode == STEP_MODE_1) {
            /**
             *      T_SY + T_RD + T_IP1 + T_SY +  T_RD  +   T_FCS
             *      T_SY    5     T_IP2   T_SY      5   +   T_FCS
             *
             *  Total time consumption = T_FCS_us + T_SY*2 + T_IP1_us + 10
             */
            step_duration_us = gCsMng.blt_pCsCfg->mode1Step_durUs;
        }

        gCsMng.blt_pCsCfg->step_expect_tick = gCsMng.blt_pCsCfg->step_expect_tick + step_duration_us * SYSTEM_TIMER_TICK_1US;
        blt_cs_init_step_stx_start();
    }
    u32 r = core_interrupt_disable();
    timer_set_init_tick(TIMER0, 0);
    timer_set_cap_tick(TIMER0, 3 * sys_clk.pclk);
    reg_tmr_ctrl0 |= FLD_TMR0_EN;
    core_restore_interrupt(r);
    return 0;
}

    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ int
    blt_cs_initiator_rx(void)
{
    gCsMng.blt_pCsCfg->step_rx_flag = 1;

    u8 slipIdx = gCsMng.blt_pCsCfg->slip_stepReadIdx % SLIP_WINDOW_STEP_NUM;
    u8 mode    = gCsMng.blt_pCsCfg->slip_window_step[slipIdx].step_modeType;
    if (mode == STEP_MODE_0) {
        if (!gCsMng.blt_pCsCfg->mode0_rx_flag) {
            gCsMng.blt_pCsCfg->mode0_rx_flag = 1;
        }
        gCsMng.blt_pCsCfg->mode0SyncMark--; // mode0 sync success
    }

    #if (SLVE_CS_RX_IRQ)
    log_event_irq(SL_STACK_CS_TIME_EN, SLVE_CS_RX_IRQ);
    #endif

    reg_rf_irq_status = FLD_RF_IRQ_RX;
    return 0;
}


    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ int
    blt_cs_initiator_irq_task(int flag)
{
    if (flag == FLAG_CS_STEP_INIT_STX_START) {
        blt_cs_init_step_stx_start();
    } else if (flag == FLAG_CS_STEP_INIT_SRX_START) {
        blt_cs_init_step_srx();
    } else if (flag == FLAG_CS_STEP_RX) {
        blt_cs_initiator_rx();
    } else if (flag == FLAG_STEP_POST) {
        blt_cs_init_step_post();
    }

    return 0;
}

ble_sts_t blc_ll_initCsInitiatorModule(chn_sound_capabilities_t *cap)
{
    ll_cs_initiator_irq_task_cb = blt_cs_initiator_irq_task;
    if (cap != NULL) {
        smemcpy((u8 *)&bltCsLocalSupportCap, cap, sizeof(chn_sound_capabilities_t));
    } else {
        return 1;
    }
    bltCsLocalSupportCap.Roles_Supported |= CS_INITIATOR_ROLE;

    return 0;
}


#endif
