/********************************************************************************************************
 * @file    chn_reflector.c
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
#if(LL_FEATURE_ENABLE_CHANNEL_SOUNDING_REFLECTOR)

#define DBG_CS_LOG_REFL_TIM_VCD_EN      0


#define STEP_MODE0_STX_MARGIN_US        (40)
#define STEP_MODE1_STX_MARGIN_US        (40)
#define STEP_MODE2_STX_MARGIN_US        (40)

static _attribute_ram_code_ void blt_cs_refl_mode0ManualTxProc(u32 stepExpectTick);
static _attribute_ram_code_ void blt_cs_refl_mode1Mode2StxProc(u32 stepExpectTick, u8 extSlotRefl, u8 mode);
_attribute_ram_code_  void blt_cs_refl_m0SysIrq_m1m2RfTxIrq_txDone(void);


#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_
void blt_cs_refl_stepSrx(void)
{
    DBG_CS_CHN3_HIGH;

#if (SL01_cs_refl_step_start_post)
    log_task_begin_irq(DBG_CS_LOG_REFL_TIM_VCD_EN, SL01_cs_refl_step_start_post );
#endif

    u8 slipIdx = blt_pCsCfg->slip_stepReadIdx&SLIP_WINDOW_STEP_MSK;
    slip_window_step_t * pslip_window_step = &blt_pCsCfg->slip_window_step[slipIdx];
    u8  chnIdx = pslip_window_step->step_chnIdx;
    u8  mode   = pslip_window_step->step_modeType;

    ble_rf_set_cs_channel(chnIdx);
    #if (HADM_PHASE_CONTINUITY)
        //After setting the frequency point, enable manual fcal to ensure phase continuity.
        u32 tick_fcal_start = 0;
        if(cs_phase_continuity_flag){
            ble_rf_manual_fcal_start();//6us 96M
            tick_fcal_start = clock_time();
        }
    #endif

    blms_state = BLMS_STATE_CS_REFL_STEP_S;
    systick_irq_trigger = SYS_IRQ_TRIG_CS_REFL_TX_START;

    /////////////////////////////////////////////////////////////////

    u32 initAA = pslip_window_step->step_initAA; //first receive, need set peer's access code.
    u32 reflAA = pslip_window_step->step_reflAA;
    u8  extSlotInit = pslip_window_step->extSlot.step_extSlotInit;
    u8  extSlotRefl = pslip_window_step->extSlot.step_extSlotRefl;
//  u8  extSlotAll  = pslip_window_step->extSlot.step_extSlotFlag;


#if(SL16_step_count)
    log_b16_irq(DBG_CS_LOG_REFL_TIM_VCD_EN, SL16_step_count, stepCnt);
#endif

    static u32 stepCnt = 0;
    u32 chnModeIeRe = (chnIdx<<24)|(mode<<16)|(extSlotInit<<8)|(extSlotRefl);
    tlkapi_send_string_u32s(0, "srxCntiAArAA_IRextChnM", stepCnt,initAA, reflAA, chnModeIeRe);
    stepCnt++;

#if(SL08_step_chnIdx)
    log_b8_irq(DBG_CS_LOG_REFL_TIM_VCD_EN, SL08_step_chnIdx, chnIdx);
#endif

    /////////////////////////////////////////////////////////////////////
#if (0)
    if(mode == STEP_MODE_0)
    {
        rf_set_ble_access_code((u8*)&initAA);
    }
    else if(mode == STEP_MODE_1){
        rf_set_ble_access_code((u8*)&initAA);//must
        ble_rf_agc_disable();
    }
    else if(mode == STEP_MODE_2){
        ble_rf_agc_disable();
    }
#endif

    if((mode == STEP_MODE_0) || (mode == STEP_MODE_1))
    {
        rf_set_ble_access_code((u8*)&initAA);//must
    }

    #if (HADM_PHASE_CONTINUITY)
        u8 rx_settle_us = cs_phase_continuity_flag ? CS_RF_RX_SETTLE_US : RX_SETTLE_US;
        u8 tx_settle_us = TX_STL_TIFS_REAL_COMMON;
    #else
        u8 rx_settle_us = RX_SETTLE_US;
        u8 tx_settle_us = TX_STL_TIFS_REAL_COMMON;
    #endif

    rf_ble_set_rx_settle(rx_settle_us);


    pCsRxAddr = cs_rx_fifo.p_base + (cs_rx_fifo.wptr & cs_rx_fifo.mask) * cs_rx_fifo.size;
    ble_rf_set_rx_dma(pCsRxAddr, cs_rx_fifo.size_div_16);

    u8 rx_early_us = 0;

    if(mode == STEP_MODE_0){

        rx_early_us = CS_MODE0_RX_EARLY_US_1M;//20
        /**iq_sample_number: 0.25us unit (4MHz) ---> 0.25us * 1 = 0.25us
         * iq_start_point: sync mode, sampling starts at (start_point + 1) * 0.125us after sync
         *      (1 + 1) * 0.125us = 0.25us
         */
        ble_rf_channel_sounding_iq_sample_config(1, 1, RF_HADM_IQ_SAMPLE_SYNC_MODE);//iq_sample_number cannot set to 0

        //1st rx timeout: rx_early_us + RX_SETTLE_US + T_SY + T_RD + margin_15us
        //                rx_early_us + RX_SETTLE_US + 44   + 5    + 15         = rx_early_us + RX_SETTLE_US + 64
        rf_set_1st_rx_timeout(rx_early_us + rx_settle_us + 64); //20+45+64=129
    }
    else if(mode == STEP_MODE_2 || mode == STEP_MODE_1){
        /**iq_sample_number: 0.25us unit (4MHz) ---> 0.25us * 1 = 0.25us
         * iq_start_point: rx_en mode, sampling starts at 0.25us+start_point*0.125us after settle
         * mode 2:  0.25us + 6*0.125us = 1us
         * mode 1:  0.25us + 1*0.125us = 0.375us
         */
        rx_early_us = (mode == STEP_MODE_2) ? CS_MODE2_RX_EARLY_US_1M: (CS_MODE1_RX_EARLY_US_1M + CS_RF_RX_1M_EXTRA_PREAMBLE_US);
        u8 rx_extend_us = CS_RF_RX_1M_WINDOW_EXTEND_US;// ensure correct Sync

    #if(MULTIPLE_ANTENNA_EN)
        u16 iq_sample_number = 0;//1us == 4sample
        int sampleTime = 0;

        if(mode == STEP_MODE_2){
            u32 t_pm_us = (extSlotInit==1) ? (g_T_SW_us+g_t_pm_us)*(g_antennaPathNum+1): (g_T_SW_us+g_t_pm_us)*g_antennaPathNum;
            t_pm_us -= g_T_SW_us; //because RF machine rx will start to receive after T_SW.
            iq_sample_number = (rx_early_us + t_pm_us) << 2;

            sampleTime = (g_T_SW_us+g_t_pm_us)*(g_antennaPathNum+1);//used to set 1st rx timeout.
        }
        else{//MODE_1
            iq_sample_number = (rx_early_us + CS_1M_PACKET_AA_ONLY_US + rx_extend_us) << 2;
            sampleTime = CS_1M_PACKET_AA_ONLY_US + rx_extend_us;
        }
    #else
        //u8 t_rx_PM = g_T_PM_us * (1 + CS_DRBG);
        //u8 t_rx_PM = T_PM_US[blt_pCsCfg->T_PM]; //*(1 + CS_DRBG) need to confirm with lijing.
        u16 t_pm_us = (extSlotInit==1) ? (g_t_pm_us*2) : g_t_pm_us;
        u16 iq_sample_number;//1us == 4sample
        if(mode == STEP_MODE_2){
            iq_sample_number = (rx_early_us + t_pm_us) << 2;
        }
        else{//MODE_1
            iq_sample_number = (rx_early_us + CS_1M_PACKET_AA_ONLY_US + rx_extend_us) << 2;
        }
    #endif
        u8 start_point = (mode == STEP_MODE_2)? 6 : 1;
        ble_rf_channel_sounding_iq_sample_config(iq_sample_number, start_point, RF_HADM_IQ_SAMPLE_RXEN_MODE);//iq_sample_number cannot set to 0

        /*mode 2:
         * 1st rx timeout: RX_SETTLE_US + rx_early_us + T_PM + extension_slot + margin_25us
         *                 RX_SETTLE_US + rx_early_us + T_PM + T_PM          + 25           = RX_SETTLE_US + rx_early_us + (T_PM_us << 1) + 25
         */

        /*mode 1:
         * 1st rx timeout: rx_settle_us + rx_early_us + CS_1M_PACKET_AA_ONLY_US + rx_extend_us + margin_25us
         *          =     rx_settle_us + rx_early_us + CS_1M_PACKET_AA_ONLY_US + rx_extend_us + 25
         */
    #if(!MULTIPLE_ANTENNA_EN)
        int sampleTime = (mode == STEP_MODE_2)?(g_t_pm_us*2):(CS_1M_PACKET_AA_ONLY_US + rx_extend_us);
    #endif

        rf_set_1st_rx_timeout(rx_settle_us + rx_early_us + sampleTime + 25);
    }

#if(MULTIPLE_ANTENNA_EN)
    u8 exist_T_SW = (mode == STEP_MODE_2) ? g_T_SW_us : 0;
    u32 rx_tick = blt_pCsCfg->step_expect_tick + (g_T_FCS_us + exist_T_SW - rx_early_us - rx_settle_us) * SYSTEM_TIMER_TICK_1US;//g_T_FCS_us
#else
    u32 rx_tick = blt_pCsCfg->step_expect_tick + (g_T_FCS_us - rx_early_us - rx_settle_us) * SYSTEM_TIMER_TICK_1US;//g_T_FCS_us
#endif

    #if (HADM_PHASE_CONTINUITY)
        if(cs_phase_continuity_flag){
            while(!tick1_exceed_tick2(clock_time(), tick_fcal_start + RF_FCAL_MANUAL_START2DONE_TIME_US * SYSTEM_TIMER_TICK_1US)){}//RF_FCAL_MANUAL_START2DONE_TIME_US
            ble_rf_manual_fcal_done();//4us 96M
        }
    #endif

#if 0
    u32 tick_now = clock_time();
    if(tick1_exceed_tick2(tick_now, rx_tick)){
        write_dbg32(0x0018, tick_now);
        write_dbg32(0x001C, rx_tick);
        u32 diff = (u32)(tick_now - rx_tick)/SYSTEM_TIMER_TICK_1US;

        tlkapi_send_string_u32s(1, "mode0Srx", tick_now, rx_tick, diff, 0);
        BLMS_ERR_DEBUG(DBG_CS_SCH_REFL, 0xDDff0000);
    }
#endif
    rf_start_fsm(FSM_SRX, NULL, rx_tick);

    blt_pCsCfg->mode0_tick_rx = 0;
    blt_pCsCfg->step_rx_flag = 0;


    //////////////////////////////////////////////////////////////
    u8 T_RD_us = 5;
    u8 CS_SYNC_us = 0;
    u32 step_tx_startTick = 0;
    //here set the post capture, but only for RF error, not generate cmd done/first rx timeout/rf rx irq etc.
    if(mode == STEP_MODE_0){
        //Regardless of whether an RF packet is received or not, set the beginning tick of TX
        //if receive relevant packet, calibrate the step_expect_tick using timestamp tick in the receive API.
        //if not receive, not change the step_expect_tick.
        CS_SYNC_us = 44; // 1M phy. other phy now not support.
        step_tx_startTick = (g_T_FCS_us + CS_SYNC_us + T_RD_us + g_T_IP1_us-tx_settle_us-CS_MODE0_TX_EARLY_US_1M - STEP_MODE0_STX_MARGIN_US )*SYSTEM_TIMER_TICK_1US;
        systimer_set_irq_capture(blt_pCsCfg->step_expect_tick + step_tx_startTick);
    }
    else if(mode == STEP_MODE_1){
        switch(blt_pCsCfg->RTT_Type){
            case RTT_Type_coarse:
            {
                CS_SYNC_us = 44;
            }
                break;

            default:
            {
                tlkapi_send_string_data(0, "other type not support", 0, 0);
            }
                break;//CS_1M_PACKET_AA_ONLY_US
        }
        step_tx_startTick = (g_T_FCS_us + CS_SYNC_us + T_RD_us + g_T_IP1_us-tx_settle_us-CS_RF_TX_1M_PACKET_EARLY_US - STEP_MODE1_STX_MARGIN_US )*SYSTEM_TIMER_TICK_1US;
        systimer_set_irq_capture(blt_pCsCfg->step_expect_tick + step_tx_startTick);
    }
    else if(mode == STEP_MODE_2){
    #if(MULTIPLE_ANTENNA_EN)
        step_tx_startTick = ( ((blt_pCsCfg->mode2Step_durUs+g_T_FCS_us+g_T_IP2_us)>>1) + g_T_SW_us -tx_settle_us-CS_MODE2_TX_EARLY_US_1M- STEP_MODE2_STX_MARGIN_US)*SYSTEM_TIMER_TICK_1US;
        systimer_set_irq_capture(blt_pCsCfg->step_expect_tick + step_tx_startTick); //TODO
    #else
        //the beginning of TX
        systimer_set_irq_capture(blt_pCsCfg->step_expect_tick + (g_T_FCS_us + (g_T_SW_us + g_t_pm_us)*2 + T_RD_us + g_T_IP2_us -tx_settle_us-CS_MODE2_TX_EARLY_US_1M- STEP_MODE2_STX_MARGIN_US)*SYSTEM_TIMER_TICK_1US); //TODO
    #endif
    }
    else if(mode == STEP_MODE_3){
        //TODO
    }
}


#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_ void blt_cs_refl_stepRev(void)
{

#if (SL01_cs_refl_rev_ok)
    static u32 toggleCnt = 0;
    toggleCnt++;
    log_task_irq(DBG_CS_LOG_REFL_TIM_VCD_EN, SL01_cs_refl_rev_ok, toggleCnt&0x01);
#endif


    HAL_CLEAR_RF_RX_IRQ;

    blt_pCsCfg->step_rx_flag = 1;

    u8 slipIdx = blt_pCsCfg->slip_stepReadIdx&SLIP_WINDOW_STEP_MSK;
    u8  mode   = blt_pCsCfg->slip_window_step[slipIdx].step_modeType;

    if((mode == STEP_MODE_0)){

        u32 tick_rx_timestamp = hal_rf_get_rx_timestamp();
        blt_pCsCfg->mode0_tick_rx = tick_rx_timestamp;

        if(!blt_pCsCfg->mode0_rx_flag){
            blt_pCsCfg->mode0_rx_flag = 1;

            #if (HADM_PHASE_CONTINUITY)
                if(!cs_phase_continuity_flag && !blt_pCsCfg->phaseContin_config_flag){
                    ble_rf_cs_get_rx_cali_value(&rx_cs_cali);
                }
            #endif
            ble_rf_agc_disable();
        }

        #if (HADM_PHASE_CONTINUITY)
            u8 tx_settle_us = TX_STL_TIFS_REAL_COMMON;
        #else
            u8 tx_settle_us = TX_STL_TIFS_REAL_COMMON;
        #endif

        /* receive packet,calibrate the step expect tick.
         * RXPATHDLY:   12 SYNC MODE
         * Access code: 32bit
         * preamble :   8bit
         */
        blt_pCsCfg->step_expect_tick = blt_pCsCfg->mode0_tick_rx - (12 + 32 +8 + g_T_FCS_us)*SYSTEM_TIMER_TICK_1US;

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
         */
        systimer_set_irq_capture(tick_rx_timestamp + (g_T_IP1_us -3 -STEP_MODE0_STX_MARGIN_US-tx_settle_us-CS_MODE0_TX_EARLY_US_1M)*SYSTEM_TIMER_TICK_1US); //5 is T_RD
        systick_irq_trigger = SYS_IRQ_TRIG_CS_REFL_TX_START;
    }

    cs_rx_agc_gain = rf_get_gain_lat_value();
}


#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_
void blt_cs_refl_stepStx(void)
{
    DBG_CS_CHN10_HIGH;

#if (SL01_cs_refl_step_stx_start_post)
    log_task_begin_irq(DBG_CS_LOG_REFL_TIM_VCD_EN, SL01_cs_refl_step_stx_start_post );
#endif

    systick_irq_trigger = SYS_IRQ_TRIG_CS_REFL_TX_POST;

    u8 slipIdx = blt_pCsCfg->slip_stepReadIdx&SLIP_WINDOW_STEP_MSK;
    slip_window_step_t * pslip_window_step = &blt_pCsCfg->slip_window_step[slipIdx];

    u8  chnIdx = pslip_window_step->step_chnIdx;
    u8  mode   = pslip_window_step->step_modeType;
    u32 reflAA = pslip_window_step->step_reflAA; //first receive, need set peer's access code.
    u32 initAA = pslip_window_step->step_initAA; //first receive, need set peer's access code.
    u8  extSlotRefl = pslip_window_step->extSlot.step_extSlotRefl;

    tlkapi_send_string_u32s(0, "postChnMAAext", initAA, reflAA, chnIdx, mode);

//  simulateUart_send_str_data (&chnIdx, 1);

    switch(mode){
        case STEP_MODE_0:
        {
            blt_cs_refl_mode0ManualTxProc(blt_pCsCfg->step_expect_tick);//here RF is sending tone.
        }
        break;
        case STEP_MODE_1:
        case STEP_MODE_2:
        {
            blt_cs_refl_mode1Mode2StxProc(blt_pCsCfg->step_expect_tick, extSlotRefl, mode);
        }
        break;
        case STEP_MODE_3:
        {

        }
        break;
    }

    DBG_CS_CHN10_LOW;

#if (SL01_cs_refl_step_stx_start_post)
    log_task_end_irq(DBG_CS_LOG_REFL_TIM_VCD_EN, SL01_cs_refl_step_stx_start_post );
#endif

}


#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_
int blt_cs_reflector_irq_task (int step_flag)
{

    if(step_flag == FLAG_CS_STEP_REFL_SRX_START){
        blt_cs_refl_stepSrx();
    }
    else if(step_flag == FLAG_CS_STEP_REFL_STX_START){
        blt_cs_refl_stepStx();
    }
    else if(step_flag == FLAG_CS_STEP_RX){
        blt_cs_refl_stepRev();
    }
    else if(step_flag == FLAG_CS_STEP_REFL_STX_POST){
        blt_cs_refl_m0SysIrq_m1m2RfTxIrq_txDone();
    }

    return 0;
}

ble_sts_t   blc_ll_initCsReflectorModule(void)
{
    ll_cs_reflector_irq_task_cb = blt_cs_reflector_irq_task;

    return 0;
}


/*
 * why do CS_SYNC+tone together send? cause if use "RF TX done" to trigger Tone's sending?
 * because T_GD(10us) is not enough to process. only the time between hardware generation and software irq irq's coming require about 20us.
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_
static  void blt_cs_refl_mode0ManualTxProc(u32 stepExpectTick)
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
    #if (HADM_PHASE_CONTINUITY)
        u8 tx_settle_us = TX_STL_TIFS_REAL_COMMON;
    #else
        u8 tx_settle_us = TX_STL_TIFS_REAL_COMMON;
    #endif

    u32 tx_tick = 0;
    u32 tick_now = clock_time();
    u8 tx_early_us = CS_MODE0_TX_EARLY_US_1M;//8us(1 byte preamble) + 5us(1M PHY TXLLDLY + TXPATHDLY).
    tx_tick = stepExpectTick + (g_T_FCS_us + 49 + g_T_IP1_us - tx_early_us - tx_settle_us) * SYSTEM_TIMER_TICK_1US;//SYNC MODE

    if(tick1_exceed_tick2(tick_now, tx_tick)){
        write_dbg32(0x0018, tick_now);
        write_dbg32(0x001C, tx_tick);
        u32 diff = (u32)(tick_now - tx_tick)/SYSTEM_TIMER_TICK_1US;

        tlkapi_send_string_u32s(0, "mode0Stx", diff, blt_pCsCfg->step_expect_tick, blt_pCsCfg->tick_expect_csSubevent, 0);
        BLMS_ERR_DEBUG(DBG_CS_SCH_REFL, 0xCCff0000);
    }



    rf_ble_set_tx_settle(tx_settle_us);

    //1M PHY, 1 byte for preamble extra length
    u8 rf_packet_len = 2+4+1; //2(preamble) + 4(access address) + 1(4 bit trail), PRMBL_LENGTH_1M
    pkt_CS.dma_len = rf_tx_packet_dma_len(rf_packet_len);

    u32 reflector_accessAddr = blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepReadIdx&SLIP_WINDOW_STEP_MSK].step_reflAA;
    smemcpy((u8 *)&pkt_CS.accessAddress, (u8 *)&reflector_accessAddr, 4);

    pkt_CS.preamble[0] = pkt_CS.preamble[1] = BIT_IS_SET(pkt_CS.accessAddress, 0) ? 0x55 : 0xAA;
    pkt_CS.trailer = BIT_IS_SET(pkt_CS.accessAddress, 31) ? 0xA : 0x5;
    pkt_CS.shift_sequence = 0;

    ble_rf_set_manual_tx_mode();

    rf_start_fsm(FSM_STX, (void *)&pkt_CS, tx_tick);

    while(!HAL_GET_RF_TX_IRQ){
        if(usr_irq_handler_cb){usr_irq_handler_cb();}
    }

    #if (HADM_PHASE_CONTINUITY)
        if(!cs_phase_continuity_flag && !blt_pCsCfg->phaseContin_config_flag){
            if(HAL_GET_RF_TX_IRQ){
                ble_rf_cs_get_tx_cali_value(&tx_cs_cali);
            }
        }
    #endif

    HAL_CLEAR_RF_TX_IRQ;

    u32 t = clock_time();

    u8 delayTick = 0;
#if(SYSTICK_NUM_PER_US == 24 )
    delayTick = cs_phase_continuity_flag ? 110:72;//62-before 820ns;after 615
#else
    #error "cal delay tick for other MCU, qiuwei !!!"
#endif
    while(((unsigned int)(stimer_get_tick() - t) < delayTick)){ //113 is about 4.7us, T_RD = 5us

    }


    systimer_set_irq_capture(clock_time() + 78*SYSTEM_TIMER_TICK_1US);//80us, 2us irq process
    /******************************* step 2: send Tone(80us) *******************************/
    ///////send tone/////////////////
    //Since tx_on is finished, tx_en is still 1, tx_on will not be set 1 when the following tone is sent.
    //Can be confirmed by spectrum analyzer.
    ble_rf_set_tx_modulation_index(RF_MI_P0p00);
    //20231116, RF_POWER_P9p90dBm ---> RF_POWER_P4p61dBm, later it is necessary to distinguish the VOLTAGE_1V8 or VOLTAGE_3V3
    ble_rf_set_power_level_singletone(RF_POWER_P4p61dBm);//only for trigger TX_PA_PWR_OW; need to restore after send completely. todo now
    //here RF is sending......

}



#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_
 static void blt_cs_refl_mode1Mode2StxProc(u32 stepExpectTick, u8 extSlotRefl, u8 mode)
{
    u8 T_RD = 5;
    u16 rfMiIdx = 0;
    u16 tx_early_us = 0;
    u32 tx_tick = 0;

    u8 rf_packet_len = 0;

    #if (HADM_PHASE_CONTINUITY)
        u8 tx_settle_us = TX_STL_TIFS_REAL_COMMON;
    #else
        u8 tx_settle_us = TX_STL_TIFS_REAL_COMMON;
    #endif

    if(mode == STEP_MODE_2){
        rfMiIdx = RF_MI_P0p00;
        //mode 2: //5us(1M PHY TXLLDLY + TXPATHDLY)
        tx_early_us = CS_MODE2_TX_EARLY_US_1M;

    #if(MULTIPLE_ANTENNA_EN)
        u32 step_tx_startUs = (blt_pCsCfg->mode2Step_durUs+g_T_FCS_us+g_T_IP2_us)>>1;
        tx_tick = stepExpectTick + (step_tx_startUs + g_T_SW_us- tx_early_us - tx_settle_us)* SYSTEM_TIMER_TICK_1US;

        u32 t_pm_us = (extSlotRefl==1) ? (g_T_SW_us+g_t_pm_us)*(g_antennaPathNum+1): (g_T_SW_us+g_t_pm_us)*g_antennaPathNum;
        t_pm_us -= g_T_SW_us;
    #else
        tx_tick = stepExpectTick + (g_T_FCS_us + ((g_T_SW_us+g_t_pm_us)*2) + T_RD + g_T_IP2_us + g_T_SW_us- tx_early_us - tx_settle_us) * SYSTEM_TIMER_TICK_1US;

        u16 t_pm_us = (extSlotRefl==1) ? (g_t_pm_us*2) : g_t_pm_us;
    #endif


        rf_packet_len = CAL_LL_CS_TONE_TX_SIZE(t_pm_us);//(T_PM_us + tone extension_slot)/8

        ble_rf_set_tx_modulation_index(rfMiIdx);
    }
    else if(mode == STEP_MODE_1){
        //8us(1 byte preamble) + 5us(1M PHY TXLLDLY + TXPATHDLY)
        tx_early_us = CS_RF_TX_1M_PACKET_EARLY_US;

        tx_tick = stepExpectTick + (g_T_FCS_us + CS_1M_PACKET_AA_ONLY_US + T_RD + g_T_IP1_us - tx_early_us - tx_settle_us) * SYSTEM_TIMER_TICK_1US;

        rf_packet_len = 2 + 4 + 1;//2(preamble) + 4(access address) + 1(4 bit trail), PRMBL_LENGTH_1M
    }

//  ble_rf_set_power_level_singletone(RF_POWER_P4p61dBm);//no need, set RF Power in subevent_start already

    rf_ble_set_tx_settle(tx_settle_us);

    if(mode == STEP_MODE_1){
        reg_rf_ll_irq_list_h = BIT(0);//clear tr_turnaround_irq

        u32 reflector_accessAddr = blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepReadIdx&SLIP_WINDOW_STEP_MSK].step_reflAA;
        smemcpy((u8 *)&pkt_CS.accessAddress, (u8 *)&reflector_accessAddr, 4);
        pkt_CS.preamble[0] = pkt_CS.preamble[1] = BIT_IS_SET(pkt_CS.accessAddress, 0) ? 0x55 : 0xAA;
        pkt_CS.trailer = BIT_IS_SET(pkt_CS.accessAddress, 31) ? 0xA : 0x5;
        pkt_CS.shift_sequence = 0;
    }

    pkt_CS.dma_len = rf_tx_packet_dma_len(rf_packet_len);

    rf_start_fsm(FSM_STX, (void *)&pkt_CS, tx_tick);
}

/*
 * mode 0 require system timer to trigger
 * mode 1 will be triggered by the RF tx done
 * mode 2 is triggered by the RF tx done.
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_
void blt_cs_refl_m0SysIrq_m1m2RfTxIrq_txDone(void)
{
    DBG_CS_CHN11_TOGGLE;

    u32 tick_tx_done = clock_time();

    u8 slipIdx = blt_pCsCfg->slip_stepReadIdx&SLIP_WINDOW_STEP_MSK;
    slip_window_step_t * pslip_window_step = &blt_pCsCfg->slip_window_step[slipIdx];
    u8 mode = pslip_window_step->step_modeType;

    ///////////////////////////////////////
    if(mode != STEP_MODE_1){
        //first close previous RF status
        ble_rf_set_power_off_singletone();
        ble_rf_set_tx_modulation_index(RF_MI_P0p50);
    }

    rf_set_tx_rx_off();
    STOP_RF_STATE_MACHINE;
    reg_rf_irq_status = FLD_RF_IRQ_TX | FLD_RF_IRQ_RX | FLD_RF_IRQ_CMD_DONE | FLD_RF_IRQ_FIRST_TIMEOUT;
    //ble_rf_agc_enable();

    if(mode == STEP_MODE_1){
        while((!(reg_rf_ll_irq_list_h & BIT(0))) && (!tick1_exceed_tick2(clock_time(), tick_tx_done + 15 * SYSTEM_TIMER_TICK_1US))){
        }
        reg_rf_ll_irq_list_h = BIT(0);//clear tr_turnaround_irq
        cs_tick_tx_on = rf_hadm_get_timestamp();//now this time obtained is tx_on
    }
#if (HADM_PHASE_CONTINUITY)
    else if(mode == STEP_MODE_0){
        ///phase continuity affect distance accuracy of mode 1.need to solve this issue.But now there is not enough time to debug.todo
        ///So mode 1 not use phase continuity setting temporally and thus can keep same accuracy as the 9.30 SDK.
        if( (blt_pCsCfg->Sub_Mode == SUBMODE_TYPE_MODE_UNUSED) && (blt_pCsCfg->Main_Mode != STEP_MODE_1 )){
            if(!cs_phase_continuity_flag && blt_pCsCfg->mode0_rx_flag && !blt_pCsCfg->phaseContin_config_flag){
                ble_rf_cs_phase_continuity_en();
                blt_pCsCfg->phaseContin_config_flag = 1;
            }
        }
    }
#endif

    ////////////////////////////////////////

    blms_state = BLMS_STATE_CS_REFL_STEP_E;
    systick_irq_trigger = SYS_IRQ_TRIG_CS_REFL_RX_START;

    ////////////////////////////////////////

    u8 rx_ext_slot_en = pslip_window_step->extSlot.step_extSlotInit;
    u8 chn = pslip_window_step->step_chnIdx;
    u8 se_end = pslip_window_step->subeventEndFlag;
    u32 initAA = pslip_window_step->step_initAA;

    ////////// for CS Subevent Result Event start //////////
    pCsRxAddr[2] = BLT_CS_REFLECTOR_FLAG;
    if(mode == STEP_MODE_0){
        pCsRxAddr[2] |= BLT_CS_MODE_0_FLAG;

        if(!blt_pCsCfg->step_rx_flag){
            //set to the default value
            pCsRxAddr[0] = 0x31;
            pCsRxAddr[1] = 0;
        }
    }
    else if(mode == STEP_MODE_1){
        pCsRxAddr[2] |= BLT_CS_MODE_1_FLAG;

        u8 cs_tx_on_tick[4] = {U32_TO_BYTES(cs_tick_tx_on)};
        smemcpy(pCsRxAddr + DMA_CS_RFRX_OFFSET_TX_ON_TSTAMP_4BYTE(pCsRxAddr), cs_tx_on_tick, 4);

        u8 cs_rx_AA[4] = {U32_TO_BYTES(initAA)};
        smemcpy(pCsRxAddr + DMA_CS_RFRX_OFFSET_RX_ACCESS_ADDRESS_4BYTE(pCsRxAddr), cs_rx_AA, 4);
    }
    else if(mode == STEP_MODE_2){
        pCsRxAddr[2] |= BLT_CS_MODE_2_FLAG;

        u32 last_tx_pos_timestamp = rf_hadm_get_tx_pos_timestamp();
        u8 tx_start_point[4] = {U32_TO_BYTES(last_tx_pos_timestamp)};
        smemcpy(pCsRxAddr + DMA_CS_RFRX_OFFSET_LAST_TX_POS_TSTAMP_4BYTE(pCsRxAddr), tx_start_point, 4);

        u8 cs_proc_start_tick[4] = {U32_TO_BYTES(blt_pCsCfg->tick_proc_start)};
        smemcpy(pCsRxAddr + DMA_CS_RFRX_OFFSET_TICK_CS_PROC_START_4BYTE(pCsRxAddr), cs_proc_start_tick, 4);

        pCsRxAddr[DMA_CS_RFRX_OFFSET_T_SW_LOW4BIT(pCsRxAddr)] = g_T_SW_us;
        pCsRxAddr[DMA_CS_RFRX_OFFSET_ACI_HIGH4BIT(pCsRxAddr)] = (pCsRxAddr[DMA_CS_RFRX_OFFSET_T_SW_LOW4BIT(pCsRxAddr)] | (blt_pCsCfg->aci << 4));
    }
    else if(mode == STEP_MODE_3){
        pCsRxAddr[2] |= BLT_CS_MODE_3_FLAG;
    }
    if(blt_pCsCfg->step_rx_flag){
        pCsRxAddr[2] |= BLT_CS_MODE_RX_FLAG;
    }
    if(rx_ext_slot_en){
        pCsRxAddr[2] |= BLT_CS_STEP_TONE_EXTENSION_SLOT_FLAG;
    }

    pCsRxAddr[3] = chn;

    DBG_CS_CHN3_LOW;

    pCsRxAddr[DMA_CS_RFRX_OFFSET_CONN_HANDLE(pCsRxAddr)] = U16_LO(blt_pCsCfg->aclHandle);
    pCsRxAddr[DMA_CS_RFRX_OFFSET_CONFIG_ID_LOW4BIT(pCsRxAddr)] = blt_pCsCfg->Config_ID;
    pCsRxAddr[DMA_CS_RFRX_OFFSET_NUM_ANTENNA_PATHS_HIGH4BIT(pCsRxAddr)] = (pCsRxAddr[DMA_CS_RFRX_OFFSET_CONFIG_ID_LOW4BIT(pCsRxAddr)] | (g_antennaPathNum << 4));//N_AP = 1~4
    pCsRxAddr[DMA_CS_RFRX_OFFSET_START_ACL_CONN_EVENT_2BYTE(pCsRxAddr)] = U16_LO(blt_pCsCfg->connEventCount);
    pCsRxAddr[DMA_CS_RFRX_OFFSET_START_ACL_CONN_EVENT_2BYTE(pCsRxAddr) + 1] = U16_HI(blt_pCsCfg->connEventCount);
    pCsRxAddr[DMA_CS_RFRX_OFFSET_PROCEDURE_COUNTER_2BYTE(pCsRxAddr)] = U16_LO(blt_pCsCfg->csProcCount);
    pCsRxAddr[DMA_CS_RFRX_OFFSET_PROCEDURE_COUNTER_2BYTE(pCsRxAddr) + 1] = U16_HI(blt_pCsCfg->csProcCount);
    pCsRxAddr[DMA_CS_RFRX_OFFSET_PROCEDURE_DONE_STATUS(pCsRxAddr)] = ((se_end && blt_pCsCfg->proc_end_flag) ? 0 : 1);
    pCsRxAddr[DMA_CS_RFRX_OFFSET_SUBEVENT_DONE_STATUS(pCsRxAddr)] = (se_end ? 0 : 1);
    pCsRxAddr[DMA_CS_RFRX_OFFSET_RX_AGC_GAIN(pCsRxAddr)] = cs_rx_agc_gain;

    cs_rx_fifo.wptr++;

    #if OS_SUP_EN
        if(blt_os_semCountIncrementIrq_cb)
        {
            blt_os_semCountIncrementIrq_cb();
        }
    #endif

    #if (DBG_CS_DATA_EN)
        if((u8)(cs_rx_fifo.wptr - cs_rx_fifo.rptr) > cs_rx_fifo.num){
            tlkapi_send_string_u8s(DBG_CS_DATA_EN, "cs_rx_fifo overflow", mode, cs_rx_fifo.wptr, cs_rx_fifo.rptr, cs_rx_fifo.num);
            BLMS_ERR_DEBUG(DBG_CS_DATA_EN, 0x55550003);
        }
    #endif
    ////////// for CS Subevent Result Event end //////////

    blt_pCsCfg->slip_stepReadIdx++; //important, point to next step

    u16 modeDurUs = (mode == STEP_MODE_0) ? blt_pCsCfg->mode0Step_durUs: ((mode == STEP_MODE_1) ? blt_pCsCfg->mode1Step_durUs:blt_pCsCfg->mode2Step_durUs);

    if(!pslip_window_step->subeventEndFlag){
        blt_pCsCfg->step_expect_tick += modeDurUs*SYSTEM_TIMER_TICK_1US;
        blt_cs_refl_stepSrx();
    }else{

        //mode 0 can not be the end step of one subevent.
        if( (mode == STEP_MODE_0) && pslip_window_step->subeventEndFlag){//judge current step
            //can not run here.
            tlkapi_send_string_data(0, "mode0 ending", 0, 0);
            BLMS_ERR_DEBUG(DBG_CS_SCH_REFL, 0xFFAABB00);
        }

        blt_cs_subevent_post(blt_pCsCfg->proc_end_flag ? 0 : 1);
    }

    #if (SL01_cs_refl_step_start_post)
        log_task_end_irq(DBG_CS_LOG_REFL_TIM_VCD_EN, SL01_cs_refl_step_start_post );
    #endif
}

#endif



#if (DBG_CS_SUBEVENT_ENABLE)
_attribute_ram_code_ u32 ble_cs_reflector_mode0_test(u32 tick_step_start, u32 csAccessAddr, u8 csChannel, u8 csPHY, u8 T_FCS_us, u8 T_IP1_us)
{
    DBG_CHN7_HIGH;// Phase 1 start

    DBG_CHN5_HIGH;//test configuration consumption time start
    ble_rf_set_cs_channel(csChannel);
    #if (HADM_PHASE_CONTINUITY)
        //After setting the frequency point, enable manual fcal to ensure phase continuity.
        u32 tick_fcal_start = 0;
        if(cs_phase_continuity_flag){
            ble_rf_manual_fcal_start();
            tick_fcal_start = clock_time();
        }
    #endif
    rf_set_ble_access_code((u8*)&csAccessAddr);

    ble_rf_set_rx_dma(cs_rx_buff, DMA_CS_RFRX_MAX_DMA_LEN>>4);

    /**iq_sample_number: 0.25us unit (4MHz) ---> 0.25us * 1 = 0.25us
     * iq_start_point: sync mode, sampling starts at (start_point + 1) * 0.125us after sync
     *      (1 + 1) * 0.125us = 0.25us
     */
    ble_rf_channel_sounding_iq_sample_config(1, 1, RF_HADM_IQ_SAMPLE_SYNC_MODE);//iq_sample_number cannot set to 0

    #if (HADM_PHASE_CONTINUITY)
        u8 rx_settle_us;
        if(cs_phase_continuity_flag){
            rx_settle_us = CS_RF_RX_SETTLE_US;
        }
        else{
            rx_settle_us = RX_SETTLE_US;
        }
    #else
        u8 rx_settle_us = RX_SETTLE_US;
    #endif
    rf_ble_set_rx_settle(rx_settle_us);
    DBG_CHN5_LOW;//test configuration consumption time post


    u8 rx_early_us = 20;
    u32 rx_tick = tick_step_start + (T_FCS_us - rx_early_us - rx_settle_us) * SYSTEM_TIMER_TICK_1US;

    #if (HADM_PHASE_CONTINUITY)
        if(cs_phase_continuity_flag){
            while(!tick1_exceed_tick2(clock_time(), tick_fcal_start + RF_FCAL_MANUAL_START2DONE_TIME_US * SYSTEM_TIMER_TICK_1US)){}
            ble_rf_manual_fcal_done();
        }
    #endif
    rf_start_fsm(FSM_SRX, NULL, rx_tick);
    //1st rx timeout: rx_early_us + rx_settle_us + T_SY + T_RD + margin_15us
    //                rx_early_us + rx_settle_us + 44   + 5    + 15         = rx_early_us + rx_settle_us + 64
    rf_set_1st_rx_timeout(rx_early_us + rx_settle_us + 64);

    u8 ret = 0;
    while(!tick1_exceed_tick2(clock_time(), tick_step_start + 1000 * SYSTEM_TIMER_TICK_1US)){
        if(HAL_GET_RF_RX_IRQ){
            ret = 1;
        }
        if((reg_rf_irq_status & FLD_RF_IRQ_RX_TIMEOUT)){
            ret = 2;
        }
        if((reg_rf_irq_status & FLD_RF_IRQ_RX_FIFO_FULL)){
            ret = 3;
        }
        if((reg_rf_irq_status & FLD_RF_IRQ_RX_CRC_2)){
            ret = 4;
        }
        if((reg_rf_irq_status & FLD_RF_IRQ_CMD_DONE)){
            ret = 5;
        }
        if((reg_rf_irq_status & FLD_RF_IRQ_FSM_TIMEOUT)){
            ret = 6;
        }
        if((reg_rf_irq_status & FLD_RF_IRQ_RX_DR)){
            ret = 7;
        }
        if((reg_rf_irq_status & FLD_RF_IRQ_FIRST_TIMEOUT)){
            ret = 8;
        }
        if(ret){
            break;
        }
        if(usr_irq_handler_cb){usr_irq_handler_cb();}
    }
    u32 tick_rx_done = clock_time();

    if(HAL_GET_RF_RX_IRQ){
        if(!cs_mode0_rx_flag){
            cs_mode0_rx_flag = 1;

            #if (HADM_PHASE_CONTINUITY)
                if(!cs_phase_continuity_flag){
                    ble_rf_cs_get_rx_cali_value(&rx_cs_cali);
                }
            #endif

            ble_rf_agc_disable();
        }
    }
    cs_rx_agc_gain = rf_get_gain_lat_value();
    DBG_CHN7_LOW;// Phase 1 end

    u32 tick_rx_timestamp = hal_rf_get_rx_timestamp();

    if(ret){
        tlkapi_send_string_u32s(DBG_CS_REFLECTOR_TIMING, "reflector_mode0_break", ret , tick_rx_timestamp / SYSTEM_TIMER_TICK_1US, tick_rx_done / SYSTEM_TIMER_TICK_1US, (tick_rx_done - tick_rx_timestamp) / SYSTEM_TIMER_TICK_1US);
    }

    DBG_CHN7_HIGH;// Phase 2 start
    u8 tx_early_us = CS_RF_TX_1M_PACKET_EARLY_US;//8us(1 byte preamble) + 5us(1M PHY TXLLDLY + TXPATHDLY)
    #if (HADM_PHASE_CONTINUITY)
        u8 tx_settle_us;
        if(cs_phase_continuity_flag){
            tx_settle_us = CS_RF_TX_SETTLE_US;
        }
        else{
            tx_settle_us = TX_STL_TIFS_REAL_COMMON;
        }
    #else
        u8 tx_settle_us = TX_STL_TIFS_REAL_COMMON;
    #endif
    u32 tx_tick;
    if((ret == 1) || (ret == 5)){
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
         */
        tx_tick = tick_rx_timestamp + (T_IP1_us - 3 - tx_early_us - tx_settle_us) * SYSTEM_TIMER_TICK_1US;//SYNC MODE
    }
    else{
        /**1M PHY, receive lose Air Timing
         *      T_FCS +     T_SY + T_RD +   T_IP1
         *      T_FCS       44      5       T_IP1
         *
         *  Theoretical tx point = tick_step_start + T_FCS + 44 + 5 + T_IP1 = tick_step_start + T_FCS + T_IP1 + 49
         */
        tx_tick = tick_step_start + (T_FCS_us + T_IP1_us + 49 - tx_early_us - tx_settle_us) * SYSTEM_TIMER_TICK_1US;
    }

    DBG_CHN5_HIGH;//test configuration consumption time start
    rf_ble_set_tx_settle(tx_settle_us);

    //1M PHY, 1 byte for preamble extra length
    u8 rf_packet_len = 2 + 4 + 1;//2(preamble) + 4(access address) + 1(4 bit trail), PRMBL_LENGTH_1M
    pkt_CS.dma_len = rf_tx_packet_dma_len(rf_packet_len);

    smemcpy((u8 *)&pkt_CS.accessAddress, (u8 *)&blms_pconn->aclAccessAddr, 4);

    pkt_CS.preamble[0] = pkt_CS.preamble[1] = BIT_IS_SET(pkt_CS.accessAddress, 0) ? 0x55 : 0xAA;
    pkt_CS.trailer = BIT_IS_SET(pkt_CS.accessAddress, 31) ? 0xA : 0x5;
    pkt_CS.shift_sequence = 0;

    ble_rf_set_manual_tx_mode();
    DBG_CHN5_LOW;//test configuration consumption time post

    rf_start_fsm(FSM_STX, (void *)&pkt_CS, tx_tick);

    while(!HAL_GET_RF_TX_IRQ){
        if(usr_irq_handler_cb){usr_irq_handler_cb();}
    }
    #if (HADM_PHASE_CONTINUITY)
        if(!cs_phase_continuity_flag){
            if(HAL_GET_RF_TX_IRQ){
                ble_rf_cs_get_tx_cali_value(&tx_cs_cali);
            }
        }
    #endif
    HAL_CLEAR_RF_TX_IRQ;

//  DBG_CHN7_LOW;
    //Since tx_on is finished, tx_en is still 1, tx_on will not be set 1 when the following tone is sent.
    //Can be confirmed by spectrum analyzer.
    ble_rf_set_tx_modulation_index(RF_MI_P0p00);
    //20231116, RF_POWER_P9p90dBm ---> RF_POWER_P4p61dBm, later it is necessary to distinguish the VOLTAGE_1V8 or VOLTAGE_3V3
    ble_rf_set_power_level_singletone(RF_POWER_P4p61dBm);//only for trigger TX_PA_PWR_OW
    u32 tick_tone_start = clock_time();
//  DBG_CHN7_HIGH;

    //T_FM = 80us
    while ((clock_time() - tick_tone_start) < 80 * SYSTEM_TIMER_TICK_1US){
        if(usr_irq_handler_cb){usr_irq_handler_cb();}
    }

//  DBG_CHN7_LOW;
    ble_rf_set_power_off_singletone();
    ble_rf_set_tx_modulation_index(RF_MI_P0p50);
//  DBG_CHN7_HIGH;

    DBG_CHN7_LOW;// Phase 2 end

    tlkapi_send_string_data(DBG_CS_REFLECTOR_TIMING, "reflector_data", cs_rx_buff, 16);

    DBG_CHN7_HIGH;// Phase 3 start

    rf_set_tx_rx_off();
    STOP_RF_STATE_MACHINE;
    reg_rf_irq_status = FLD_RF_IRQ_TX | FLD_RF_IRQ_RX | FLD_RF_IRQ_CMD_DONE | FLD_RF_IRQ_FIRST_TIMEOUT;

    #if (HADM_PHASE_CONTINUITY)
        if(!cs_phase_continuity_flag && cs_mode0_rx_flag){
            ble_rf_cs_phase_continuity_en();
        }
    #endif

    u32 tick_next_step_start;

    if((ret == 1) || (ret == 5)){
        /**1M PHY, receive succeed Air Timing
         *      Access_Address + Trailer + T_RD + T_IP1 + T_SY + T_GD + T_FM + T_RD
         *      timestamp           4       5     T_IP1     44    10     80     5 = T_IP1_us + 148
         *
         * 1M PHY, IQ Sample Rx Path Delay Timing
         *      RXPATHDLY
         *          5   //RXEN MODE
         *          12  //SYNC MODE
         *
         *  Total time consumption = T_IP1_us + 148 - RXPATHDLY = T_IP1_us + 143    //RXEN MODE
         *  Total time consumption = T_IP1_us + 148 - RXPATHDLY = T_IP1_us + 136    //SYNC MODE
         */
        tick_next_step_start = tick_rx_timestamp + (T_IP1_us + 136) * SYSTEM_TIMER_TICK_1US;//SYNC MODE
    }
    else{
        /**1M PHY, receive lose Air Timing
         *      T_FCS +     T_SY + T_RD +   T_IP1 +     T_SY + T_GD + T_FM + T_RD
         *      T_FCS       44      5       T_IP1       44      10      80      5
         *
         *  Total time consumption = T_FCS_us + T_IP1_us + 188
         */
        tick_next_step_start = tick_step_start + (T_FCS_us + T_IP1_us + 188) * SYSTEM_TIMER_TICK_1US;
    }

    cs_rx_buff[2] = BLT_CS_REFLECTOR_FLAG;
    cs_rx_buff[2] |= BLT_CS_MODE_0_FLAG;
    if((ret == 1) || (ret == 5)){
        cs_rx_buff[2] |= BLT_CS_MODE_RX_FLAG;
    }
    cs_rx_buff[3] = csChannel;

    cs_rx_buff[DMA_CS_RFRX_OFFSET_RX_AGC_GAIN(cs_rx_buff)] = cs_rx_agc_gain;

    blt_ll_csRxFifoUpdate();

    DBG_CHN7_LOW;// Phase 3 end

    return tick_next_step_start;
}

_attribute_ram_code_ u32 ble_cs_reflector_mode1_test(u32 tick_step_start, u32 csAccessAddr, u8 csChannel, u8 csPHY, u8 T_FCS_us, u8 T_IP1_us)
{
    DBG_CHN7_HIGH;// Phase 1 start

    DBG_CHN5_HIGH;//test configuration consumption time start
    //ble_rf_agc_disable();

    ble_rf_set_cs_channel(csChannel);
    #if (HADM_PHASE_CONTINUITY)
        //After setting the frequency point, enable manual fcal to ensure phase continuity.
        u32 tick_fcal_start = 0;
        if(cs_phase_continuity_flag){
            ble_rf_manual_fcal_start();
            tick_fcal_start = clock_time();
        }
    #endif

    rf_set_ble_access_code((u8*)&csAccessAddr);//must

    ble_rf_set_rx_dma(cs_rx_buff, DMA_CS_RFRX_MAX_DMA_LEN>>4);

    /**iq_sample_number: 0.25us unit (4MHz) ---> 0.25us * 1 = 0.25us
     * iq_start_point: rx_en mode, sampling starts at 0.25us+start_point*0.125us after settle
     *      0.25us + 1*0.125us = 0.375us
     */
    u8 rx_early_us = CS_RFRXEN_MODE_EARLY_US + CS_RF_RX_1M_EXTRA_PREAMBLE_US;//5us(1M PHY RXPATHDLY) RXEN MODE + 1 byte for preamble extra length
    u8 rx_extend_us = CS_RF_RX_1M_WINDOW_EXTEND_US;// ensure correct Sync

    u16 iq_sample_number = (CS_1M_PACKET_AA_ONLY_US + rx_early_us + rx_extend_us) << 2;
    ble_rf_channel_sounding_iq_sample_config(iq_sample_number, 1, RF_HADM_IQ_SAMPLE_RXEN_MODE);//iq_sample_number cannot set to 0
    DBG_CHN5_LOW;//test configuration consumption time post

    #if (HADM_PHASE_CONTINUITY)
        u8 rx_settle_us;
        if(cs_phase_continuity_flag){
            rx_settle_us = CS_RF_RX_SETTLE_US;
        }
        else{
            rx_settle_us = RX_SETTLE_US;
        }
    #else
        u8 rx_settle_us = RX_SETTLE_US;
    #endif
    rf_ble_set_rx_settle(rx_settle_us);

    u32 rx_tick = tick_step_start + (T_FCS_us - rx_early_us - RX_SETTLE_US) * SYSTEM_TIMER_TICK_1US;

    #if (HADM_PHASE_CONTINUITY)
        if(cs_phase_continuity_flag){
            while(!tick1_exceed_tick2(clock_time(), tick_fcal_start + RF_FCAL_MANUAL_START2DONE_TIME_US * SYSTEM_TIMER_TICK_1US)){}
            ble_rf_manual_fcal_done();
        }
    #endif
    rf_start_fsm(FSM_SRX, NULL, rx_tick);
    //1st rx timeout: rx_settle_us + rx_early_us + CS_1M_PACKET_AA_ONLY_US + rx_extend_us + margin_25us
    //          =     rx_settle_us + rx_early_us + CS_1M_PACKET_AA_ONLY_US + rx_extend_us + 25
    rf_set_1st_rx_timeout(rx_settle_us + rx_early_us + CS_1M_PACKET_AA_ONLY_US + rx_extend_us + 25);

    u8 ret = 0;
    while(!tick1_exceed_tick2(clock_time(), tick_step_start + 1000 * SYSTEM_TIMER_TICK_1US)){
        if(HAL_GET_RF_RX_IRQ){
            ret = 1;
        }
        if((reg_rf_irq_status & FLD_RF_IRQ_RX_TIMEOUT)){
            ret = 2;
        }
        if((reg_rf_irq_status & FLD_RF_IRQ_RX_FIFO_FULL)){
            ret = 3;
        }
        if((reg_rf_irq_status & FLD_RF_IRQ_RX_CRC_2)){
            ret = 4;
        }
        if((reg_rf_irq_status & FLD_RF_IRQ_CMD_DONE)){
            ret = 5;
        }
        if((reg_rf_irq_status & FLD_RF_IRQ_FSM_TIMEOUT)){
            ret = 6;
        }
        if((reg_rf_irq_status & FLD_RF_IRQ_RX_DR)){
            ret = 7;
        }
        if((reg_rf_irq_status & FLD_RF_IRQ_FIRST_TIMEOUT)){
            ret = 8;
        }
        if(ret){
            break;
        }
        if(usr_irq_handler_cb){usr_irq_handler_cb();}
    }
    DBG_CHN7_LOW;// Phase 1 end
    u32 tick_rx_done = clock_time();
    u32 tick_rx_timestamp = hal_rf_get_rx_timestamp();
    u32 iq_start_timestamp = rf_hadm_get_iq_start_timestamp();
    cs_rx_agc_gain = rf_get_gain_lat_value();

    if(ret){
        tlkapi_send_string_u32s(DBG_CS_REFLECTOR_TIMING, "reflector_mode1_break", ret , tick_rx_timestamp / SYSTEM_TIMER_TICK_1US, tick_rx_done / SYSTEM_TIMER_TICK_1US, (tick_rx_done - tick_rx_timestamp) / SYSTEM_TIMER_TICK_1US);
        tlkapi_send_string_data(DBG_CS_REFLECTOR_TIMING, "reflector_rx_data", cs_rx_buff, 16);
    }

    DBG_CHN7_HIGH;// Phase 2 start

    DBG_CHN5_HIGH;//test configuration consumption time start
//  ble_rf_set_tx_modulation_index(RF_MI_P0p50);//no need
//  ble_rf_set_power_level_singletone(RF_POWER_P4p61dBm);//no need, set RF Power in subevent_start already

    u8 tx_early_us = CS_RF_TX_1M_PACKET_EARLY_US;//8us(1 byte preamble) + 5us(1M PHY TXLLDLY + TXPATHDLY)
    #if (HADM_PHASE_CONTINUITY)
        u8 tx_settle_us;
        if(cs_phase_continuity_flag){
            tx_settle_us = CS_RF_TX_SETTLE_US;
        }
        else{
            tx_settle_us = TX_STL_TIFS_REAL_COMMON;
        }
    #else
        u8 tx_settle_us = TX_STL_TIFS_REAL_COMMON;
    #endif
    u32 tx_tick = tick_step_start + (T_FCS_us + CS_1M_PACKET_AA_ONLY_US + 5 + T_IP1_us - tx_early_us - tx_settle_us) * SYSTEM_TIMER_TICK_1US;
    rf_ble_set_tx_settle(tx_settle_us);
    DBG_CHN5_LOW;//test configuration consumption time post

    reg_rf_ll_irq_list_h = BIT(0);//clear tr_turnaround_irq

    rf_start_fsm(FSM_STX, (void *)&pkt_CS, tx_tick);

    //1M PHY, RTT AA Only, 1 byte for preamble extra length
    u8 rf_packet_len = 2 + 4 + 1;//2(preamble) + 4(access address) + 1(4 bit trail), PRMBL_LENGTH_1M
    pkt_CS.dma_len = rf_tx_packet_dma_len(rf_packet_len);
    smemcpy((u8 *)&pkt_CS.accessAddress, (u8 *)&csAccessAddr, 4);
    pkt_CS.preamble[0] = pkt_CS.preamble[1] = BIT_IS_SET(pkt_CS.accessAddress, 0) ? 0x55 : 0xAA;
    pkt_CS.trailer = BIT_IS_SET(pkt_CS.accessAddress, 31) ? 0xA : 0x5;
    pkt_CS.shift_sequence = 0;

    while(!HAL_GET_RF_TX_IRQ){
        if(usr_irq_handler_cb){usr_irq_handler_cb();}
    }
    u32 tick_tx_done = clock_time();
    DBG_CHN7_LOW;// Phase 2 end

    while(!(reg_rf_ll_irq_list_h & BIT(0))){
    }
    reg_rf_ll_irq_list_h = BIT(0);//clear tr_turnaround_irq
    u32 tick_cs_tx_on = rf_hadm_get_timestamp();

    DBG_CHN7_HIGH;// Phase 3 start
    u32 tick_next_step_start;

    rf_set_tx_rx_off();
    STOP_RF_STATE_MACHINE;
    reg_rf_irq_status = FLD_RF_IRQ_TX | FLD_RF_IRQ_RX | FLD_RF_IRQ_CMD_DONE | FLD_RF_IRQ_FIRST_TIMEOUT;

    //ble_rf_agc_enable();

    /**
     *      T_FCS + T_SY + T_RD + T_IP1 + T_SY +  T_RD
     *      T_FCS   T_SY    5     T_IP2   T_SY      5
     *
     *  Total time consumption = T_FCS_us + T_SY*2 + T_IP1_us + 10
     */
    tick_next_step_start = tick_step_start + (T_FCS_us + (CS_1M_PACKET_AA_ONLY_US << 1) + T_IP1_us + 10) * SYSTEM_TIMER_TICK_1US;

    cs_rx_buff[2] = BLT_CS_REFLECTOR_FLAG;
    cs_rx_buff[2] |= BLT_CS_MODE_1_FLAG;
    if((ret == 1) || (ret == 5)){
        cs_rx_buff[2] |= BLT_CS_MODE_RX_FLAG;
    }
    cs_rx_buff[3] = csChannel;

    u8 cs_tx_on_tick[4] = {U32_TO_BYTES(tick_cs_tx_on)};
    smemcpy(cs_rx_buff + DMA_CS_RFRX_OFFSET_TX_ON_TSTAMP_4BYTE(cs_rx_buff), cs_tx_on_tick, 4);

    u8 cs_rx_AA[4] = {U32_TO_BYTES(csAccessAddr)};
    smemcpy(cs_rx_buff + DMA_CS_RFRX_OFFSET_RX_ACCESS_ADDRESS_4BYTE(cs_rx_buff), cs_rx_AA, 4);

    cs_rx_buff[DMA_CS_RFRX_OFFSET_RX_AGC_GAIN(cs_rx_buff)] = cs_rx_agc_gain;

    blt_ll_csRxFifoUpdate();

    if(ret){
        tlkapi_send_string_u32s(1, "reflector_mode1_break", ret, iq_start_timestamp / SYSTEM_TIMER_TICK_1US, tick_rx_timestamp / SYSTEM_TIMER_TICK_1US, tick_cs_tx_on / SYSTEM_TIMER_TICK_1US);
        tlkapi_send_string_u32s(DBG_CS_REFLECTOR_TIMING, "reflector_mode1_break", ret, iq_start_timestamp / SYSTEM_TIMER_TICK_1US, tick_rx_timestamp / SYSTEM_TIMER_TICK_1US, tick_cs_tx_on / SYSTEM_TIMER_TICK_1US);
    }

    DBG_CHN7_LOW;// Phase 3 end

    return tick_next_step_start;
}

_attribute_ram_code_ u32 ble_cs_reflector_mode2_test(u32 tick_step_start, u32 csAccessAddr, u8 csChannel, u8 csPHY, u8 T_FCS_us, u8 T_PM_us, u8 T_IP2_us, u8 CS_DRBG)
{
    DBG_CHN7_HIGH;// Phase 1 start

    DBG_CHN5_HIGH;//test configuration consumption time start
    //ble_rf_agc_disable();

    ble_rf_set_cs_channel(csChannel);
//  rf_set_ble_access_code((u8*)&csAccessAddr);//no need
    #if (HADM_PHASE_CONTINUITY)
        //After setting the frequency point, enable manual fcal to ensure phase continuity.
        u32 tick_fcal_start = 0;
        if(cs_phase_continuity_flag){
            ble_rf_manual_fcal_start();
            tick_fcal_start = clock_time();
        }
    #endif

    ble_rf_set_rx_dma(cs_rx_buff, DMA_CS_RFRX_MAX_DMA_LEN>>4);

    #if (HADM_PHASE_CONTINUITY)
        u8 rx_settle_us;
        if(cs_phase_continuity_flag){
            rx_settle_us = CS_RF_RX_SETTLE_US;
        }
        else{
            rx_settle_us = RX_SETTLE_US;
        }
    #else
        u8 rx_settle_us = RX_SETTLE_US;
    #endif
    rf_ble_set_rx_settle(rx_settle_us);

    /**iq_sample_number: 0.25us unit (4MHz) ---> 0.25us * 1 = 0.25us
     * iq_start_point: rx_en mode, sampling starts at 0.25us+start_point*0.125us after settle
     *      0.25us + 6*0.125us = 1us
     */
    u8 rx_early_us = CS_RFRXEN_MODE_EARLY_US;//5us(1M PHY RXPATHDLY)

    u8 t_rx_PM;
    if(CS_DRBG & BIT(1)){//tone extension present
        t_rx_PM = T_PM_us * 2;
    }
    else{
        t_rx_PM = T_PM_us;
    }

    u16 iq_sample_number = (t_rx_PM + rx_early_us) << 2;
    ble_rf_channel_sounding_iq_sample_config(iq_sample_number, 6, RF_HADM_IQ_SAMPLE_RXEN_MODE);//iq_sample_number cannot set to 0
    DBG_CHN5_LOW;//test configuration consumption time post

    u32 rx_tick = tick_step_start + (T_FCS_us - rx_early_us - rx_settle_us) * SYSTEM_TIMER_TICK_1US;

    #if (HADM_PHASE_CONTINUITY)
        if(cs_phase_continuity_flag){
            while(!tick1_exceed_tick2(clock_time(), tick_fcal_start + RF_FCAL_MANUAL_START2DONE_TIME_US * SYSTEM_TIMER_TICK_1US)){}
            ble_rf_manual_fcal_done();
        }
    #endif
    rf_start_fsm(FSM_SRX, NULL, rx_tick);
    //1st rx timeout: rx_settle_us + rx_early_us + T_PM + extension_slot + margin_25us
    //                rx_settle_us + rx_early_us + T_PM + T_PM           + 25           = rx_settle_us + rx_early_us + (T_PM_us << 1) + 25
    rf_set_1st_rx_timeout(rx_settle_us + rx_early_us + (T_PM_us << 1) + 25);

    u8 ret = 0;
    while(!tick1_exceed_tick2(clock_time(), tick_step_start + 1000 * SYSTEM_TIMER_TICK_1US)){
        if(HAL_GET_RF_RX_IRQ){
            ret = 1;
        }
        if((reg_rf_irq_status & FLD_RF_IRQ_RX_TIMEOUT)){
            ret = 2;
        }
        if((reg_rf_irq_status & FLD_RF_IRQ_RX_FIFO_FULL)){
            ret = 3;
        }
        if((reg_rf_irq_status & FLD_RF_IRQ_RX_CRC_2)){
            ret = 4;
        }
        if((reg_rf_irq_status & FLD_RF_IRQ_CMD_DONE)){
            ret = 5;
        }
        if((reg_rf_irq_status & FLD_RF_IRQ_FSM_TIMEOUT)){
            ret = 6;
        }
        if((reg_rf_irq_status & FLD_RF_IRQ_RX_DR)){
            ret = 7;
        }
        if((reg_rf_irq_status & FLD_RF_IRQ_FIRST_TIMEOUT)){
            ret = 8;
        }
        if(ret){
            break;
        }
        if(usr_irq_handler_cb){usr_irq_handler_cb();}
    }
    DBG_CHN7_LOW;// Phase 1 end
    u32 tick_rx_done = clock_time();
    u32 tick_rx_timestamp = hal_rf_get_rx_timestamp();//TODO
    cs_rx_agc_gain = rf_get_gain_lat_value();

    if(ret){
        tlkapi_send_string_u32s(DBG_CS_REFLECTOR_TIMING, "reflector_mode2_break", ret , tick_rx_timestamp / SYSTEM_TIMER_TICK_1US, tick_rx_done / SYSTEM_TIMER_TICK_1US, (tick_rx_done - tick_rx_timestamp) / SYSTEM_TIMER_TICK_1US);
        tlkapi_send_string_data(DBG_CS_REFLECTOR_TIMING, "reflector_rx_data", cs_rx_buff, 16);
    }

    DBG_CHN7_HIGH;// Phase 2 start

    DBG_CHN5_HIGH;//test configuration consumption time start
    ble_rf_set_tx_modulation_index(RF_MI_P0p00);
//  ble_rf_set_power_level_singletone(RF_POWER_P4p61dBm);//no need, set RF Power in subevent_start already

    u8 tx_early_us = CS_RF_TX_1M_TONE_EARLY_US;//5us(1M PHY TXLLDLY + TXPATHDLY)
    #if (HADM_PHASE_CONTINUITY)
        u8 tx_settle_us;
        if(cs_phase_continuity_flag){
            tx_settle_us = CS_RF_TX_SETTLE_US;
        }
        else{
            tx_settle_us = TX_STL_TIFS_REAL_COMMON;
        }
    #else
        u8 tx_settle_us = TX_STL_TIFS_REAL_COMMON;
    #endif
    u32 tx_tick = tick_step_start + (T_FCS_us + (T_PM_us << 1) + 5 + T_IP2_us - tx_early_us - tx_settle_us) * SYSTEM_TIMER_TICK_1US;
    rf_ble_set_tx_settle(tx_settle_us);
    DBG_CHN5_LOW;//test configuration consumption time post

    u8 rf_packet_len;
    if(CS_DRBG & BIT(0)){//tone extension present
        rf_packet_len = CAL_LL_CS_TONE_TX_SIZE(T_PM_us * 2);//(T_PM_us + tone extension_slot)/8
    }
    else{
        rf_packet_len = CAL_LL_CS_TONE_TX_SIZE(T_PM_us);//T_PM_us/8
    }
    pkt_CS.dma_len = rf_tx_packet_dma_len(rf_packet_len);

    rf_start_fsm(FSM_STX, (void *)&pkt_CS, tx_tick);

    while(!HAL_GET_RF_TX_IRQ){
        if(usr_irq_handler_cb){usr_irq_handler_cb();}
    }
    u32 tick_tx_done = clock_time();
    ble_rf_set_power_off_singletone();
    ble_rf_set_tx_modulation_index(RF_MI_P0p50);
    DBG_CHN7_LOW;// Phase 2 end


    DBG_CHN7_HIGH;// Phase 3 start
    u32 tick_next_step_start;

    rf_set_tx_rx_off();
    STOP_RF_STATE_MACHINE;
    reg_rf_irq_status = FLD_RF_IRQ_TX | FLD_RF_IRQ_RX | FLD_RF_IRQ_CMD_DONE | FLD_RF_IRQ_FIRST_TIMEOUT;

    //ble_rf_agc_enable();

    /**
     *      T_FCS + T_PM + extension_slot + T_RD +  T_IP2 + T_PM + extension_slot + T_RD
     *      T_FCS   T_PM        T_PM        5       T_IP2   T_PM        T_PM        5
     *
     *  Total time consumption = T_FCS_us + T_PM_us*4 + T_IP2_us + 10
     */
    tick_next_step_start = tick_step_start + (T_FCS_us + (T_PM_us << 2) + T_IP2_us + 10) * SYSTEM_TIMER_TICK_1US;

    cs_rx_buff[2] = BLT_CS_REFLECTOR_FLAG;
    cs_rx_buff[2] |= BLT_CS_MODE_2_FLAG;
    if((ret == 1) || (ret == 5)){
        cs_rx_buff[2] |= BLT_CS_MODE_RX_FLAG;
    }
    if(t_rx_PM == T_PM_us * 2){
        cs_rx_buff[2] |= BLT_CS_STEP_TONE_EXTENSION_SLOT_FLAG;
    }
    cs_rx_buff[3] = csChannel;

    u32 last_tx_pos_timestamp = rf_hadm_get_tx_pos_timestamp();
    u8 tx_start_point[4] = {U32_TO_BYTES(last_tx_pos_timestamp)};
    smemcpy(cs_rx_buff + DMA_CS_RFRX_OFFSET_LAST_TX_POS_TSTAMP_4BYTE(cs_rx_buff), tx_start_point, 4);

    cs_rx_buff[DMA_CS_RFRX_OFFSET_RX_AGC_GAIN(cs_rx_buff)] = cs_rx_agc_gain;

    blt_ll_csRxFifoUpdate();

    DBG_CHN7_LOW;// Phase 3 end

    return tick_next_step_start;
}
#endif
