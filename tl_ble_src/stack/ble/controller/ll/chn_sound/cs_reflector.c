/********************************************************************************************************
 * @file    cs_reflector.c
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
#if (LL_FEATURE_ENABLE_CHANNEL_SOUNDING_REFLECTOR)

    #define DBG_CS_LOG_REFL_TIM_VCD_EN 0


    #define STEP_MODE0_STX_MARGIN_US   (40)
    #define STEP_MODE1_STX_MARGIN_US   (40)
    #define STEP_MODE2_STX_MARGIN_US   (40)

static _attribute_ram_code_ void blt_cs_refl_mode0ManualTxProc(u32 stepExpectTick);
static _attribute_ram_code_ void blt_cs_refl_mode1Mode2StxProc(u32 stepExpectTick, u8 extSlotRefl, u8 mode);
_attribute_ram_code_ void        blt_cs_refl_m0SysIrq_m1m2RfTxIrq_txDone(void);
void                             blt_cs_refl_stepStx(void);


    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ void
    blt_cs_refl_stepSrx(void)
{
    DBG_CS_CHN2_HIGH;
    u32 tick_fcal_start = 0;
    u16 rx_early_us     = (gCsMng.blt_pCsCfg->CS_SYNC_PHY == BLE_1M_PHY) ? CS_RFRXEN_MODE_1M_EARLY_US : CS_RFRXEN_MODE_2M_EARLY_US;
    u8  tx_settle_us    = gCsMng.blt_pCsCfg->phaseContinue_cal_flag ? CS_COMMON_TX_SETTLE_US : TX_STL_TIFS_REAL_COMMON;
    u8  rx_settle_us    = gCsMng.blt_pCsCfg->phaseContinue_cal_flag ? CS_COMMON_RX_SETTLE_US : RX_SETTLE_US;
    u32 permu           = 0;

    #if (SL01_cs_refl_step_start_post)
    log_task_begin_irq(DBG_CS_LOG_REFL_TIM_VCD_EN, SL01_cs_refl_step_start_post);
    #endif

    /*
     * Get slip window to set channel and mode
     */
    slip_window_step_t *pStepRefl = blt_cs_getSlipWindow();
    u8                  chnIdx    = pStepRefl->step_chnIdx;
    u8                  mode      = pStepRefl->step_modeType;

    /*
     * fcal must before channel setting
     */
    ble_rf_set_cs_channel(chnIdx);
    if (gCsMng.blt_pCsCfg->phaseContinue_cal_flag) {
        rf_manual_fcal_start(); //6us 96M
        tick_fcal_start = clock_time();
    }

    if (((u8)((cs_rx_fifo.wptr - cs_rx_fifo.rptr) & 127)) >= cs_rx_fifo.num) {
        tlkapi_send_string_u8s(DBG_CS_DATA_EN, "cs_rx_fifo overflow -rx", mode, cs_rx_fifo.wptr, cs_rx_fifo.rptr, cs_rx_fifo.num);
        BLMS_ERR_DEBUG(DBG_CS_SCH_INIT, 0xBBff0500);
    }
    /////////////////////////////////////////////////////////////////
    #if (SL16_step_count)
    log_b16_irq(DBG_CS_LOG_REFL_TIM_VCD_EN, SL16_step_count, stepCnt);
    #endif

    #if 0
    /****************just for debug start**************************/
    u32 reflAA = pStepRefl->step_reflAA;
    u8  extSlotAll  = pStepRefl->extSlot.step_extSlotFlag;
    u32 chnModeIeRe = chnIdx<<24 | mode<<16 | extSlotAll;
    tlkapi_send_string_u32s(CS_TIMING_DEBUG_LOG_EN|1, "stxCntiAArAAIdx", gCsMng.blt_pCsCfg->csProcCount,pStepRefl->step_initAA, reflAA, chnModeIeRe);
    /****************just for debug end**************************/
    #endif

    #if (SL08_step_chnIdx)
    log_b8_irq(DBG_CS_LOG_REFL_TIM_VCD_EN, SL08_step_chnIdx, chnIdx);
    #endif

    /////////////////////////////////////////////////////////////////////
    if ((mode == STEP_MODE_0) || (mode == STEP_MODE_1)) {
        rf_set_ble_access_code((u8 *)&pStepRefl->step_initAA); //must
    }
    rf_ble_set_rx_settle(rx_settle_us-1);

    /**
     * SYNC_MODE: sampling start after acesscode sync
     * RXEN_MODE: sampling start after Rx en
     * iq_sample_number: 0.25us unit (4MHz) ---> 0.25us * 1 = 0.25us = 5bytes
     * iq_start_point: sync mode, sampling starts at (start_point + 1) * 0.125us after sync
     */
    if (mode == STEP_MODE_0) {
    #if (CS_SLEEP_CLOCK_ACCURACY)
        /* if CS_SLEEP_CLOCK_ACCURACY enabled, the first rx window should extend according to ppm and sleep duration,
         * and the timeout tick should be larger to cover send early or late the subevent start tick.
         */
        rx_early_us               = (!gCsMng.blt_pCsCfg->mode0_rx_flag) ? (CS_MODE0_RX_EARLY_US_1M + gCsMng.blt_pCsCfg->winWideUs) : CS_MODE0_RX_EARLY_US_1M;
        unsigned int timeout_tick = (!gCsMng.blt_pCsCfg->mode0_rx_flag) ?
                                        (rx_early_us + rx_settle_us + gCsMng.blt_pCsCfg->mode0_sync_us + T_RD_US + 15 + gCsMng.blt_pCsCfg->winWideUs) :
                                        (rx_early_us + rx_settle_us + gCsMng.blt_pCsCfg->mode0_sync_us + T_RD_US + 15);
        rf_set_1st_rx_timeout(timeout_tick);
    #else
        rx_early_us = CS_MODE0_RX_EARLY_US_1M;
        //15us margin
        rf_set_1st_rx_timeout(rx_early_us * 2 + rx_settle_us + gCsMng.blt_pCsCfg->mode0_sync_us + T_RD_US + 15);
    #endif
        //1st rx timeout: rx_early_us + RX_SETTLE_US + T_SY + T_RD + margin_15us
        //                rx_early_us + RX_SETTLE_US + 44   + 5    + 15
        if (gCsMng.blt_pCsCfg->firstReflRx) {
            // first receive mode0 sync packet, set RX EN window very long
            // the tick value between hci test cmd and first mode0 sync packet, EBQ 4.6ms  RF Creation:
            rf_set_1st_rx_timeout(0xffff); //20+45+64=129
        }
        ble_rf_channel_sounding_iq_sample_config(1, 1, RF_CS_IQ_SAMPLE_SYNC_MODE); //iq_sample_number cannot set to 0

    } else if (mode == STEP_MODE_2 || mode == STEP_MODE_1) {
        /**iq_sample_number: 0.25us unit (4MHz) ---> 0.25us * 1 = 0.25us
         * iq_start_point: rx_en mode, sampling starts at 0.25us+start_point*0.125us after settle
         * mode 2:  0.25us + 6*0.125us = 1us
         * mode 1:  0.25us + 1*0.125us = 0.375us
         */
        u16 iq_sample_number = 0;                            //1us == 4sample
        u8  start_point      = (mode == STEP_MODE_2) ? 6 : 1;
        u8 rx_extend_us      = CS_RF_RX_1M_WINDOW_EXTEND_US; // ensure correct Sync

        if (mode == STEP_MODE_2) {
            u32 t_pm_us = (pStepRefl->extSlot.step_extSlotInit ? gCsMng.blt_pCsCfg->mode2ToneUs :
                                                                 gCsMng.blt_pCsCfg->mode2ToneUs_noExtslot) -
                          gCsMng.blt_pCsCfg->T_SW_Us;
            iq_sample_number = (rx_early_us + t_pm_us) << 2;
        } else { //MODE_1
            iq_sample_number = (rx_early_us + gCsMng.blt_pCsCfg->none_mode_sync_us + rx_extend_us) << 2;
        }
        //iq_sample_number cannot set to 0
        ble_rf_channel_sounding_iq_sample_config(iq_sample_number, start_point, RF_CS_IQ_SAMPLE_RXEN_MODE);
    }

    if (mode == STEP_MODE_2) {
        permu = gCsMng.blt_pCsCfg->perm_table[pStepRefl->step_antPathPermIdx];
    } else // mode0,mode1. not consider mode3
    {
        permu = gCsMng.blt_pCsCfg->perm_table[24 + (gCsMng.blt_pCsCfg->sync_pky_cnt & 0x03)];
        gCsMng.blt_pCsCfg->sync_pky_cnt++;
    }

    if(ant_ctrl_cfg.set_ant_permu_func){
        ant_ctrl_cfg.set_ant_permu_func (permu);
    }

    u8  exist_T_SW = (mode == STEP_MODE_2) ? gCsMng.blt_pCsCfg->T_SW_Us : 0;
    u32 rx_tick    = gCsMng.blt_pCsCfg->step_expect_tick + (exist_T_SW - rx_early_us - rx_settle_us) * SYSTEM_TIMER_TICK_1US;


    if (gCsMng.blt_pCsCfg->phaseContinue_cal_flag) {
        while (!tick1_exceed_tick2(clock_time(), tick_fcal_start + RF_FCAL_MANUAL_START2DONE_TIME_US * SYSTEM_TIMER_TICK_1US)) {
        }                      //RF_FCAL_MANUAL_START2DONE_TIME_US
        rf_manual_fcal_done(); //4us 96M
    }

    #if 1
    u32 tick_now = clock_time();
    if (tick1_exceed_tick2(tick_now, rx_tick)) {
        write_dbg32(0x0018, tick_now);
        write_dbg32(0x001C, rx_tick);
        u32 diff = (u32)(tick_now - rx_tick) / SYSTEM_TIMER_TICK_1US;

        tlkapi_send_string_u32s(DBG_CS_SCH_REFL, "mode0Srx", tick_now, rx_tick, diff, 0);
        BLMS_ERR_DEBUG(DBG_CS_SCH_REFL, 0xDDff0000);
    }
    #endif
    rf_start_fsm(FSM_SRX, NULL, rx_tick);
    cs_rx_fifo.pCsRxAddr = cs_rx_fifo.p_base + (cs_rx_fifo.wptr & cs_rx_fifo.mask) * cs_rx_fifo.size;

    cs_rx_fifo.pCsRxAddr[0] = 0;
    cs_rx_fifo.pCsRxAddr[1] = 0;
    cs_rx_fifo.pCsRxAddr[2] = 0;
    ble_rf_set_rx_dma(cs_rx_fifo.pCsRxAddr, cs_rx_fifo.size_div_16);

    if (blc_rf_pa_cb) {
        blc_rf_pa_cb(PA_TYPE_RX_ON);
    }

    u8 tx_early_us = (gCsMng.blt_pCsCfg->CS_SYNC_PHY == BLE_1M_PHY) ? CS_tx_early_1M_phy[mode] : CS_tx_early_2M_phy[mode];
    /////////////////Tx tick/////////////////////////////////////////////
    u32 step_tx_startTick = 0;
    if (mode == STEP_MODE_0) {
        step_tx_startTick = gCsMng.blt_pCsCfg->step_expect_tick + (gCsMng.blt_pCsCfg->mode0TxIntvalUs -
                                                                   tx_settle_us - tx_early_us - STEP_MODE0_STX_MARGIN_US) *
                                                                      SYSTEM_TIMER_TICK_1US;
    #if (CS_SLEEP_CLOCK_ACCURACY)
        /* if CS_SLEEP_CLOCK_ACCURACY enabled, the first step mode0 tx start tick should consider the rx window wide. */
        step_tx_startTick += (!gCsMng.blt_pCsCfg->mode0_rx_flag) ? (gCsMng.blt_pCsCfg->winWideUs * SYSTEM_TIMER_TICK_1US) : 0;
    #endif
    } else if (mode == STEP_MODE_1) {
        step_tx_startTick = gCsMng.blt_pCsCfg->step_expect_tick + (gCsMng.blt_pCsCfg->mode1TxIntvalUs -
                                                                   tx_settle_us - tx_early_us - STEP_MODE1_STX_MARGIN_US) *
                                                                      SYSTEM_TIMER_TICK_1US;
    } else if (mode == STEP_MODE_2) {
        step_tx_startTick = gCsMng.blt_pCsCfg->step_expect_tick + (gCsMng.blt_pCsCfg->mode2TxIntvalUs +
                                                                   gCsMng.blt_pCsCfg->T_SW_Us - tx_settle_us - tx_early_us -
                                                                   STEP_MODE2_STX_MARGIN_US) *
                                                                      SYSTEM_TIMER_TICK_1US;
    } else if (mode == STEP_MODE_3) {
        //TODO
    }
    if (gCsMng.blt_pCsCfg->firstReflRx) {
        systimer_set_irq_capture(clock_time() ^ BIT(31));
    }
    else
    {
        systimer_set_irq_capture(step_tx_startTick);
    }


    gCsMng.blt_pCsCfg->step_rx_flag = 0;
    blms_state                      = BLMS_STATE_CS_REFL_STEP_S;
    systick_irq_trigger             = SYS_IRQ_TRIG_CS_REFL_TX_START;
    gCsMng.blt_pCsCfg->firstReflRx  = 0;
}


    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ void
    blt_cs_refl_rx(void)
{
    DBG_CS_CHN2_LOW;
    #if (SL01_cs_refl_rev_ok)
    static u32 toggleCnt = 0;
    toggleCnt++;
    log_task_irq(DBG_CS_LOG_REFL_TIM_VCD_EN, SL01_cs_refl_rev_ok, toggleCnt & 0x01);
    #endif


    reg_rf_irq_status = FLD_RF_IRQ_RX;

    gCsMng.blt_pCsCfg->step_rx_flag = 1;

    u8 slipIdx = gCsMng.blt_pCsCfg->slip_stepReadIdx % SLIP_WINDOW_STEP_NUM;
    u8 mode    = gCsMng.blt_pCsCfg->slip_window_step[slipIdx].step_modeType;


    if ((mode == STEP_MODE_0)) {
        u32 tick_rx_timestamp = hal_rf_get_rx_timestamp();

        if (!gCsMng.blt_pCsCfg->mode0_rx_flag) {
            gCsMng.blt_pCsCfg->mode0_rx_flag = 1;
        }

        /* receive packet,calibrate the step expect tick.
         * RXPATHDLY:   1M: 12; 2M: 5us
         * Access code: 1M PHY: 32bit,32us; 2M PHY: 32bits, 16us
         * preamble :   1M PHY: 8bits, 8us; 2M PHY: 16bits, 8us
         */
        if(gCsMng.blt_pCsCfg->CS_SYNC_PHY == BLE_1M_PHY){
            gCsMng.blt_pCsCfg->step_expect_tick = tick_rx_timestamp - (CS_HW_DELAY_CS_1M  + 32 +8)*SYSTEM_TIMER_TICK_1US;
        }
        else{
            gCsMng.blt_pCsCfg->step_expect_tick = tick_rx_timestamp - (CS_HW_DELAY_CS_2M + 16 +8)*SYSTEM_TIMER_TICK_1US;
        }

       gCsMng.blt_pCsCfg->mode0SyncMark--; // mode0 sync success
    }
}


    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ void
    blt_cs_refl_stepStx(void)
{
    DBG_CS_CHN2_HIGH;

    #if (SL01_cs_refl_step_stx_start_post)
    log_task_begin_irq(DBG_CS_LOG_REFL_TIM_VCD_EN, SL01_cs_refl_step_stx_start_post);
    #endif

    systick_irq_trigger = SYS_IRQ_TRIG_CS_REFL_TX_POST;

    slip_window_step_t *pStepRefl   = blt_cs_getSlipWindow();
    u8                  mode        = pStepRefl->step_modeType;
    u8                  extSlotRefl = pStepRefl->extSlot.step_extSlotRefl ? 1 : 0;

    #if (0)
    u8  chnIdx = pStepRefl->step_chnIdx;
    u32 reflAA = pStepRefl->step_reflAA; //first receive, need set peer's access code.
    u32 initAA = pStepRefl->step_initAA; //first receive, need set peer's access code.
    tlkapi_send_string_u32s(0, "postChnMAAext", initAA, reflAA, chnIdx, mode);
    #endif

    switch (mode) {
    case STEP_MODE_0:
    {
        blt_cs_refl_mode0ManualTxProc(gCsMng.blt_pCsCfg->step_expect_tick); //here RF is sending tone.
    } break;
    case STEP_MODE_1:
    case STEP_MODE_2:
    {
        blt_cs_refl_mode1Mode2StxProc(gCsMng.blt_pCsCfg->step_expect_tick, extSlotRefl, mode);
    } break;
    case STEP_MODE_3:
    {
    } break;
    }

    #if (SL01_cs_refl_step_stx_start_post)
    log_task_end_irq(DBG_CS_LOG_REFL_TIM_VCD_EN, SL01_cs_refl_step_stx_start_post);
    #endif
    u32 r = core_interrupt_disable();
    timer_set_init_tick(TIMER0, 0);
    timer_set_cap_tick(TIMER0, 3 * sys_clk.pclk);
    reg_tmr_ctrl0 |= FLD_TMR0_EN;
    core_restore_interrupt(r);
}


    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ int
    blt_cs_reflector_irq_task(int step_flag)
{
    if (step_flag == FLAG_CS_STEP_REFL_SRX_START) {
        blt_cs_refl_stepSrx();
    } else if (step_flag == FLAG_CS_STEP_REFL_STX_START) {
        blt_cs_refl_stepStx();
    } else if (step_flag == FLAG_CS_STEP_RX) {
        blt_cs_refl_rx();
    } else if (step_flag == FLAG_CS_STEP_REFL_STX_POST) {
        blt_cs_refl_m0SysIrq_m1m2RfTxIrq_txDone();
    }

    return 0;
}

ble_sts_t blc_ll_initCsReflectorModule(chn_sound_capabilities_t *cap)
{
    ll_cs_hci_subevent_report_cb = blt_ll_cs_loop_hci_subevent;
    ll_cs_reflector_irq_task_cb = blt_cs_reflector_irq_task;
    if (cap != NULL) {
        smemcpy((u8 *)&bltCsLocalSupportCap, cap, sizeof(chn_sound_capabilities_t));
    } else {
        return 1;
    }
    bltCsLocalSupportCap.Roles_Supported |= CS_REFLECTOR_ROLE;
    return 0;
}

    // it's come from api clock_time_exceed, just add a judge to avoid set a the time to a future time and It will cause fault.
    #if (CS_EBQ_TEST)
_attribute_ram_code_
    _Bool
    ebq_test_clock_time_exceed(unsigned int ref, unsigned int us)
{
    unsigned int t0 = stimer_get_tick();
    if ((unsigned int)(t0 - ref) > 0x80000000UL) {
        return 0;
    }
    return ((unsigned int)(stimer_get_tick() - ref) > us * SYSTEM_TIMER_TICK_1US);
}
    #endif

    /*
 * why do CS_SYNC+tone together send? cause if use "RF TX done" to trigger Tone's sending?
 * because T_GD(10us) is not enough to process. only the time between hardware generation and software irq irq's coming require about 20us.
 */
    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ static void
    blt_cs_refl_mode0ManualTxProc(u32 stepExpectTick)
{
    /******************************* step 1: send CS_SYNC *******************************/

    /**1M PHY, receive succeed Air Timing
     *      Access_Address + Trailer + T_RD + T_IP1
     *      timestamp           4       5     T_IP1
     *
     * 1M PHY, IQ Sample Rx Path Delay Timing
     *      RXPATHDLY
     *          5   //RXEN MODE
     *          12  //SYNC MODE
     *
     *  Theoretical tx point = timestamp - RXPATHDLY + 4 + 5 + T_IP1 = timestamp + 4 + T_IP1    //RXEN MODE
     *  Theoretical tx point = timestamp - RXPATHDLY + 4 + 5 + T_IP1 = timestamp + T_IP1 - 3    //SYNC MODE
     *  49: 1B preamble(8us) + 4B access code(32us) + 4bit trailer(4us) + 5us T_RD; 1M PHY
     */
    u8 tx_settle_us = gCsMng.blt_pCsCfg->phaseContinue_cal_flag ? CS_COMMON_TX_SETTLE_US : TX_STL_TIFS_REAL_COMMON;

    u32 tx_tick     = 0;
    u32 tick_now    = clock_time();
    u8 tx_early_us = (gCsMng.blt_pCsCfg->CS_SYNC_PHY == BLE_1M_PHY) ? CS_tx_early_1M_phy[0] : CS_tx_early_2M_phy[0];
    tx_tick         = stepExpectTick + (gCsMng.blt_pCsCfg->mode0_sync_us + T_RD_US + gCsMng.blt_pCsCfg->T_IP1_Us - tx_early_us - tx_settle_us) * SYSTEM_TIMER_TICK_1US; //SYNC MODE

    if (tick1_exceed_tick2(tick_now, tx_tick)) {
        write_dbg32(0x0018, tick_now);
        write_dbg32(0x001C, tx_tick);
        u32 diff = (u32)(tick_now - tx_tick) / SYSTEM_TIMER_TICK_1US;

        tlkapi_send_string_u32s(DBG_CS_SCH_REFL, "mode0Stx", diff, gCsMng.blt_pCsCfg->step_expect_tick, gCsMng.blt_pCsCfg->tick_expect_csSubevent, 0);
        BLMS_ERR_DEBUG(DBG_CS_SCH_REFL, 0xCCff0000);
    }
    rf_ble_set_tx_settle(tx_settle_us-1);
    slip_window_step_t *pStep = blt_cs_getSlipWindow();
    // mode0 packet only have 4 bits trailer after sync access address, it's too short and will influence
    // mode0 RSSI value(abnormal value: 0x7F), send 16bits trailer to avoid. -- qinghua, biao, yuexin, sync with xuqiang.
    blt_cs_mode0_packetSyncPDU(&pkt_CS_m0, pStep->step_reflAA);


    ble_rf_set_manual_tx_mode();
    /* when we are reflector and test with EBQ, for mode0, tone timing is not correct, cause the method set stx dma len = sync pkt not accuracy.
     * Now,test with EBQ, set stx dma len = sync pkt + tone and set a tick to change the tx to single tone.
     * And this method will reduce frequency offset as driver team(pengcheng) said, but now not test with ffo. -- yuexin 2024.08.02
     */
    #if (CS_EBQ_TEST & 0)
    pkt_CS_m0.dma_len = rf_tx_packet_dma_len(7 + ((80) >> 3));
    ble_rf_set_tx_modulation_index(RF_MI_P0p50);
    rf_start_fsm(FSM_STX, (void *)&pkt_CS_m0, tx_tick);

    unsigned char m_change_flag = 1;
    unsigned int pkt_time_us;
    if(gCsMng.blt_pCsCfg->CS_SYNC_PHY == BLE_1M_PHY){
        pkt_time_us = tx_settle_us + 48 + 8;
    }
    else{ // 2M PHY
        pkt_time_us = tx_settle_us + 32 + 8;
    }

    while (!(rf_get_irq_status(FLD_RF_IRQ_TX))) // wait pkt send
    {
        if (ebq_test_clock_time_exceed(tx_tick, pkt_time_us) && m_change_flag) {
            ble_rf_set_tx_modulation_index(RF_MI_P0p00);
            rf_cs_set_power_level_singletone(gCsMng.cs_tx_power);
            m_change_flag = 0;
        }

        if (ebq_test_clock_time_exceed(tx_tick, 3 * 1000)) // time out 3ms
        {
            break;
        }
    }
    delay_us(6);
    rf_clr_irq_status(FLD_RF_IRQ_TX);
    rf_set_tx_rx_off();

    systimer_irq_enable();
    systimer_set_irq_capture(clock_time() + 1 * SYSTEM_TIMER_TICK_1US); //80us, 2us irq process

    #else
    rf_start_fsm(FSM_STX, (void *)&pkt_CS_m0, tx_tick);
    /*add by jiapeng 2024.08.23 base on SHA-1: 2c0b5574a5618ccf1bc867d24afb7594b7f42dec.
     * calculate idle time to reduce T_IP1(48M, T_IP1 = 145).
     * from this time to tx_on = 88.66us
     * 88.66 - tx_settle(53) - tx_early(13) = 22.66us
     * Mode 0: remained 22.66us
     * conclusion : Mode 0 T_IP1 couldn't cut to 80us.
     */

    if (blc_rf_pa_cb) {
        blc_rf_pa_cb(PA_TYPE_TX_ON);
    }

    while (!(reg_rf_irq_status & FLD_RF_IRQ_TX)) {
        if (usr_irq_handler_cb) {
            usr_irq_handler_cb();
        }
    }
    reg_rf_irq_status = FLD_RF_IRQ_TX;

        #if (MCU_CORE_TYPE == MCU_CORE_B92 || MCU_CORE_TYPE == CHIP_TYPE_TL721X || MCU_CORE_TYPE == CHIP_TYPE_TL322X)
            //In order to accurately detect RSSI, the mode0 trailer added 4+8 extra bits
            #if (SYNC_PDU_1M_MODE0_LEN != 8)
                u32 t         = clock_time();
                u8  delayTick = 0;
                delayTick     = gCsMng.blt_pCsCfg->phaseContinue_cal_flag ? 110 : 72; //62-before 820ns;after 615
                while (((unsigned int)(stimer_get_tick() - t) < delayTick)) {         //113 is about 4.7us, T_GD = 10us
                }
            #endif
        #endif

    /******************************* step 2: send Tone(80us) *******************************/
    ///////send tone/////////////////
    //Since tx_on is finished, tx_en is still 1, tx_on will not be set 1 when the following tone is sent.
    //Can be confirmed by spectrum analyzer.
    ble_rf_set_tx_modulation_index(RF_MI_P0p00);
#if (MCU_CORE_TYPE == MCU_CORE_TL721X) || (MCU_CORE_TYPE == MCU_CORE_TL322X)
    systimer_set_irq_capture(clock_time() + 76 * SYSTEM_TIMER_TICK_1US); //80us, 2us irq process
#else
    systimer_set_irq_capture(clock_time() + 78 * SYSTEM_TIMER_TICK_1US); //80us, 2us irq process
#endif

    //only for trigger TX_PA_PWR_OW; use rf_cs_set_power_off_singletone() to restore in mode0 step post.
    //20250225 use same power with CS sync.
    rf_cs_set_power_level_singletone(gCsMng.cs_tx_power);

        //here RF is sending......
        blms_state = BLMS_STATE_CS_REFL_STEP_TX_S; // avoid enter cmd_done, make sure mode0 tone send complete.
    #endif
}


    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ static void
    blt_cs_refl_mode1Mode2StxProc(u32 stepExpectTick, u8 extSlotRefl, u8 mode)
{
    u16 tx_early_us = (gCsMng.blt_pCsCfg->CS_SYNC_PHY == BLE_1M_PHY) ? CS_tx_early_1M_phy[mode] : CS_tx_early_2M_phy[mode];;
    u32 tx_tick     = 0;


    u8                  tx_settle_us = gCsMng.blt_pCsCfg->phaseContinue_cal_flag ? CS_COMMON_TX_SETTLE_US : TX_STL_TIFS_REAL_COMMON;
    slip_window_step_t *slipRef      = (slip_window_step_t *)(&gCsMng.blt_pCsCfg->slip_window_step[gCsMng.blt_pCsCfg->slip_stepReadIdx % SLIP_WINDOW_STEP_NUM]);


    if (mode == STEP_MODE_2) {
        //mode 2: //5us(1M PHY TXLLDLY + TXPATHDLY)
        tx_tick     = stepExpectTick + (gCsMng.blt_pCsCfg->mode2TxIntvalUs + gCsMng.blt_pCsCfg->T_SW_Us - tx_early_us - tx_settle_us) * SYSTEM_TIMER_TICK_1US;
        u32 t_pm_us = (extSlotRefl == 1) ? gCsMng.blt_pCsCfg->mode2ToneUs : gCsMng.blt_pCsCfg->mode2ToneUs_noExtslot;
        t_pm_us -= gCsMng.blt_pCsCfg->T_SW_Us;

        u8 rf_packet_len = CAL_LL_CS_TONE_TX_SIZE(t_pm_us); //(T_PM_us + tone extension_slot)/8
        pkt_CS.dma_len   = rf_tx_packet_dma_len(rf_packet_len);

        ble_rf_set_tx_modulation_index(RF_MI_P0p00);
    } else if (mode == STEP_MODE_1) {
        //8us(1 byte preamble) + 5us(1M PHY TXLLDLY + TXPATHDLY)
        //only 1M phy, todo for 2M
        tx_tick = stepExpectTick + (gCsMng.blt_pCsCfg->mode1TxIntvalUs - tx_early_us - tx_settle_us) * SYSTEM_TIMER_TICK_1US;

        reg_rf_ll_irq_list_h = BIT(0); //clear tr_turnaround_irq
        if(gCsMng.blt_pCsCfg->CS_SYNC_PHY == BLE_1M_PHY){
            blt_cs_packetSyncPDU_mode1_cali(&pkt_CS_m1, slipRef->step_reflAA, slipRef, CS_REFLECTOR_ROLE);
        }
        else{
            blt_cs_packetSyncPDU(&pkt_CS, slipRef->step_reflAA, slipRef, CS_REFLECTOR_ROLE);
        }
    }
    rf_ble_set_tx_settle(tx_settle_us-1);
    if (mode == STEP_MODE_1 && gCsMng.blt_pCsCfg->CS_SYNC_PHY == BLE_1M_PHY) {
        rf_start_fsm(FSM_STX, (void *)&pkt_CS_m1, tx_tick);
    } else {
        rf_start_fsm(FSM_STX, (void *)&pkt_CS, tx_tick);
    }
    /*add by jiapeng 2024.08.23 base on SHA-1: 2c0b5574a5618ccf1bc867d24afb7594b7f42dec.
     * calculate idle time to reduce T_IP1 & T_IP2.
     * Mode 1: remained 42.25us(from this time to tx_on, 48M, T_IP1 = 145)
     * Mode 2: remained 58us(from this time to tx_on, 48M, T_IP2 = 145)
     * Conclusion : Mode 1 T_IP1 & Mode 2 T_IP2 couldn't cut to 80us.
     */
    if (blc_rf_pa_cb) {
        blc_rf_pa_cb(PA_TYPE_TX_ON);
    }
}

    /*
 * mode 0 require system timer to trigger
 * mode 1 will be triggered by the RF tx done
 * mode 2 is triggered by the RF tx done.
 */
    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ void
    blt_cs_refl_m0SysIrq_m1m2RfTxIrq_txDone(void)
{
    u8  abort        = 0;
    u32 tick_tx_done = clock_time();

    slip_window_step_t *pStepRefl      = blt_cs_getSlipWindow();
    u8                  mode           = pStepRefl->step_modeType;
    u8                  antPathPermIdx = pStepRefl->step_antPathPermIdx; //Antenna Path Permutation Index
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

    blms_state          = BLMS_STATE_CS_REFL_STEP_E;
    systick_irq_trigger = SYS_IRQ_TRIG_CS_REFL_RX_START;

    u8  se_end         = pStepRefl->subeventEndFlag;
    u8  proc_end       = pStepRefl->proceStopFlag;
    u16 dma_len = cs_rx_fifo.pCsRxAddr[0] | (cs_rx_fifo.pCsRxAddr[1]<<8);
    u8  rx_early_us      = CS_RFRXEN_MODE_1M_EARLY_US;

    if(gCsMng.blt_pCsCfg->CS_SYNC_PHY == BLE_2M_PHY){
        rx_early_us      = CS_RFRXEN_MODE_2M_EARLY_US;
    }
    ////////// for CS Subevent Result Event start //////////
    switch(mode){
        case STEP_MODE_0:
        {
            slip_window_step_t  *pNextStep = blt_cs_getNextSlipWindow();
            if((dma_len != MODE0_REFLETOR_DMA_LEN))
            {
                cs_rx_fifo.pCsRxAddr[0] = 49;
                cs_rx_fifo.pCsRxAddr[1] = 0;
                pRxFlag->flag.rx_valid = 0;
            }

            cs_rx_para = (cs_rx_para_t *)(cs_rx_fifo.pCsRxAddr + DMA_CS_RFRX_OFFSET_TIME_STAMP(cs_rx_fifo.pCsRxAddr));
            if(pNextStep->step_modeType != STEP_MODE_0)
            {
                if (gCsMng.blt_pCsCfg->mode0_rx_flag)
                {
                    rf_agc_disable();
                    cs_rx_agc_gain = rf_get_gain_lat_value();
                }else
                {
                    se_end                        = 1;
                    slip_window_step_t *pStepLast = blt_cs_getLastSlipWindow();
                    if (pStepLast->proceStopFlag) {
                        proc_end = 1;
                    }
                    abort = (SUBEVT_ABORT_NO_MODE0_RECEIVED << 4) & 0xF0;
                }
            }
            rf_cs_set_power_off_singletone();
            ble_rf_set_tx_modulation_index(RF_MI_P0p50);
            break;
        }
        case STEP_MODE_1:
        {
           u32 iq_sample_len = (rx_early_us + gCsMng.blt_pCsCfg->none_mode_sync_us + CS_RF_RX_1M_WINDOW_EXTEND_US)*20 + CS_RX_HD_EXT_LEN;
           if((dma_len != iq_sample_len)){
               cs_rx_fifo.pCsRxAddr[0] = 49;
               cs_rx_fifo.pCsRxAddr[1] = 0;
               pRxFlag->flag.rx_valid = 0;
           }

            while ((!(reg_rf_ll_irq_list_h & BIT(0))) && (!tick1_exceed_tick2(clock_time(), tick_tx_done + 15 * SYSTEM_TIMER_TICK_1US))) {
            }
            reg_rf_ll_irq_list_h = BIT(0);                //clear tr_turnaround_irq
            cs_tick_tx_on        = rf_cs_get_timestamp(); //now this time obtained is tx_on

            cs_rx_para = (cs_rx_para_t *)(cs_rx_fifo.pCsRxAddr + DMA_CS_RFRX_OFFSET_TIME_STAMP(cs_rx_fifo.pCsRxAddr));

            cs_rx_para->tx_on_tstamp      = cs_tick_tx_on;
            cs_rx_para->rx_access_address = pStepRefl->step_initAA;
            #if (MODE1_FINE_RTT &&((CHIP_TYPE == CHIP_TYPE_TL721X)||(CHIP_TYPE == CHIP_TYPE_TL322X)))
            slip_window_step_t *pStepCur = blt_cs_getSlipWindow();
            smemcpy(&cs_rx_para->step_RttSeq[0],&pStepCur->step_initRttSeq[0],16);
            cs_rx_para->step_RttSSPos[0] = pStepCur->step_initRttSSPos[0];
            cs_rx_para->step_RttSSPos[1] = pStepCur->step_initRttSSPos[1];
            #endif
            break;
        }
        case STEP_MODE_2:
        {
            u32 t_pm_us = (pStepRefl->extSlot.step_extSlotInit ? gCsMng.blt_pCsCfg->mode2ToneUs :
                           gCsMng.blt_pCsCfg->mode2ToneUs_noExtslot) - gCsMng.blt_pCsCfg->T_SW_Us;
            u32 iq_sample_len = (rx_early_us + t_pm_us )*20 + CS_RX_HD_EXT_LEN;

            if(dma_len != iq_sample_len)
            {
                cs_rx_fifo.pCsRxAddr[0] = 49;
                cs_rx_fifo.pCsRxAddr[1] = 0;
                pRxFlag->flag.rx_valid = 0;
            }

            cs_rx_para = (cs_rx_para_t *)(cs_rx_fifo.pCsRxAddr + DMA_CS_RFRX_OFFSET_TIME_STAMP(cs_rx_fifo.pCsRxAddr));
            #if ((CHIP_TYPE == CHIP_TYPE_TL721X) || (CHIP_TYPE == CHIP_TYPE_TL322X))
                cs_rx_para->last_tx_pos_tstamp = rf_cs_get_tx_frac_pos_timestamp();
            #else
                cs_rx_para->last_tx_pos_tstamp = rf_cs_get_tx_pos_timestamp();
            #endif

            pRxFlag->flag.ext_slot = pStepRefl->extSlot.step_extSlotInit ? 1 : 0;
            cs_rx_para->tick_cs_proc_start = gCsMng.blt_pCsCfg->tick_proc_start;
            cs_rx_para->ant_path_perm_idx = antPathPermIdx;

            ble_rf_set_tx_modulation_index(RF_MI_P0p50);
            break;
        }
        default:
        {
            break;
        }
    }
    pRxFlag->flag.mode =  mode;
    pRxFlag->flag.role = CHANNEL_SOUNDING_ROLE_REFLECTOR;
    cs_rx_fifo.pCsRxAddr[3] = pStepRefl->step_chnIdx;;
    cs_rx_para->start_acl_conn_event  = gCsMng.blt_pCsCfg->cs_inst_acl;
    cs_rx_para->procedure_counter     = gCsMng.gGlobal_pCsCfg->csProcCount;
    cs_rx_para->procedure_done_status = ((proc_end) ? 0 : 1);
    cs_rx_para->subevent_done_status  = abort ? abort : (se_end ? 0 : 1);
    cs_rx_para->rx_agc_gain           = cs_rx_agc_gain;
    cs_rx_para->config_struct_addr    = gCsMng.blt_pCsCfg;
    cs_rx_fifo.wptr++;
    gCsMng.blt_pCsCfg->slip_stepReadIdx++; //important, point to next step

    #if (DBG_CS_DATA_EN && LL_CS_SNIFFER_MODE_ENABLE)
    if ((u8)(cs_rx_fifo.wptr - cs_rx_fifo.rptr) > cs_rx_fifo.num) {
        tlkapi_send_string_u8s(DBG_CS_DATA_EN, "[STK][CS] reflector cs_rx_fifo overflow", mode, cs_rx_fifo.wptr, cs_rx_fifo.rptr, cs_rx_fifo.num);
        write_dbg32(DBG_SRAM_ADDR + 4, mode);
        write_dbg32(DBG_SRAM_ADDR + 8, cs_rx_fifo.wptr);
        write_dbg32(DBG_SRAM_ADDR + 12, cs_rx_fifo.rptr);
        BLMS_ERR_DEBUG(DBG_CS_DATA_EN, 0x55550003);
    }
    #endif

    DBG_CS_CHN2_LOW;
    if (se_end)
    {
        if (proc_end)
        {
            DBG_CS_CHN3_TOGGLE;
            gCsMng.blt_pCsCfg->stopSch = 1;
            blt_sche_addUpdate(SLOT_UPDT_CHANNEL_SOUNDING_STATE_CHANGE);
        }
        blt_cs_subevent_post(proc_end);
    }
    else
    {
        u16 modeDurUs = (mode == STEP_MODE_0) ? gCsMng.blt_pCsCfg->mode0Step_durUs :
                ((mode == STEP_MODE_1) ? gCsMng.blt_pCsCfg->mode1Step_durUs : gCsMng.blt_pCsCfg->mode2Step_durUs);
        gCsMng.blt_pCsCfg->step_expect_tick += modeDurUs * SYSTEM_TIMER_TICK_1US;
        blt_cs_refl_stepSrx();
    }

    #if (SL01_cs_refl_step_start_post)
        log_task_end_irq(DBG_CS_LOG_REFL_TIM_VCD_EN, SL01_cs_refl_step_start_post);
    #endif

    u32 r = core_interrupt_disable();
    timer_set_init_tick(TIMER0, 0);
    timer_set_cap_tick(TIMER0, 3 * sys_clk.pclk);
    reg_tmr_ctrl0 |= FLD_TMR0_EN;
    core_restore_interrupt(r);

    #if OS_SUP_EN
        if (blt_os_semCountIncrementIrq_cb) {
            blt_os_semCountIncrementIrq_cb();
        }
    #endif
}

#endif
