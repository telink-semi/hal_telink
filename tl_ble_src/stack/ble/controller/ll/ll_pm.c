/********************************************************************************************************
 * @file    ll_pm.c
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

#if (BLMS_PM_ENABLE)

#define BLS_USER_TIMER_WAKEUP_ENABLE    1

#if (MCU_STALL_EN)
_attribute_ble_data_retention_  volatile  u8     pm_mcuStall_allowFlag = 0;
#endif


_attribute_ble_data_retention_  ll_module_pm_callback_t  ll_module_pm_cb = NULL;

_attribute_ble_data_retention_  _attribute_aligned_(4)  st_llms_pm_t  blmsPm;

_attribute_ble_data_retention_ bool exit_latency_used = false;

extern blt_event_callback_t     blt_p_event_callback ;

void	blc_ll_setOsLowPowerExitLatencyUs(u32 suspendUs, u32 deepretUs)
{
	blmsPm.suspend_exitLatencyTick = suspendUs * SYSTEM_TIMER_TICK_1US;
	blmsPm.deepret_exitLatencyTick = deepretUs * SYSTEM_TIMER_TICK_1US;
}


void blt_pm_calculate_accumulated_error(void);


void        blc_ll_initPowerManagement_module(void)
{
    ll_module_pm_cb = blt_sleep_process;

    blmsPm.pm_inited = 1;
    blmsPm.user_latency = 0xffff;
    blmsParam.min_tolerance_us = 40;  //for every 80mS
//  blmsPm.sys_ppm_index = PPM_IDX_MAX;
    blmsPm.wkpTsk_oft = WKPTASK_INVALID;
    blmsPm.slave_idx_calib = 0xFF;

    blmsPm.deepRet_thresTick = 80 * SYSTEM_TIMER_TICK_1MS;
    blmsPm.deepRet_earlyWakeupTick = 300 * SYSTEM_TIMER_TICK_1US;

    blmsPm.deepRet_type = (u8)(DEFAULT_DEEPSLEEP_MODE_RET_SRAM_SIZE);


    blmsPm.wkpTsk_fifo.next = NULL;
    blmsPm.Wakeup_early_us = 30; //default :30us
}

/*
 * @brief:This function is mainly to solve the os mode, long sleep handler function
 * */
_attribute_ble_data_retention_  ll_module_pmOs_callback_t  ll_osWakeupTickProcess_cb = NULL;
#define FLAG_OS_WAKEUP_STEP_0   0
#define FLAG_OS_WAKEUP_STEP_1   1
#define FLAG_OS_WAKEUP_STEP_2   2

_attribute_ram_code_
void blt_pm_osWakeupTickProcess(int flag )
{
    static u32 wakeup_tick_expect = 0;
    switch(flag)
    {
    case FLAG_OS_WAKEUP_STEP_0:
        wakeup_tick_expect = 0;
        if(blmsPm.appWakeup_en){
            wakeup_tick_expect = blmsPm.appWakeup_tick;
        }

        if(bltSche.task_mask & TSKMSK_LEG_ADV)
        {
            if(!tick1_exceed_tick2(blmsPm.next_adv_tick,wakeup_tick_expect)){  // must use
                exit_latency_used = true;
                wakeup_tick_expect = blmsPm.next_adv_tick;
            }
        }
        if(blmsPm.pTask_wakeup->scheTask_flg == TSKFLG_ACL_SLAVE)
        {
            st_lls_conn_t *ps;
            u8 acl_index = blmsPm.pTask_wakeup->scheTask_idx;
            ps = (st_lls_conn_t*)&blmsSlave[acl_index - LL_MAX_ACL_CEN_NUM];
            if(ps->latency_available){
                if(!tick1_exceed_tick2(ps->latency_wakeup_tick,wakeup_tick_expect)){  //must use
                    exit_latency_used = true;
                blmsPm.appWakeup_en = 0;  //Use latency at this time
                wakeup_tick_expect = ps->latency_wakeup_tick;
                }
            }
        }
        break;

    case FLAG_OS_WAKEUP_STEP_1:
        if(blmsPm.appWakeup_en){
            blmsPm.appWakeup_en = 0;  //Use latency at this time
        }
        break;

    case FLAG_OS_WAKEUP_STEP_2:
        if(wakeup_tick_expect && tick1_exceed_tick2(blmsPm.current_wakeup_tick, wakeup_tick_expect)){
            blmsPm.current_wakeup_tick = wakeup_tick_expect;
        }
        break;
    default:
        wakeup_tick_expect = 0;
        break;
    }
}

void    blc_ll_enOsPowerManagement_module(void)
{
    ll_osWakeupTickProcess_cb  = blt_pm_osWakeupTickProcess;

    /*Because it takes some time for the rtos to officially enable interrupts
     *  after blt_sleep_process exits. So we need to advance this by 100us.
     * */
    blmsPm.Wakeup_early_us = 100;//max:0xFFFF
}

void    blc_pm_setDeepsleepRetentionEnable (deep_retn_en_t en)
{
    blmsPm.deepRt_en = en;
}

void    blc_pm_setSleepMask (sleep_mask_t mask)
{
    u32 r = irq_disable();

    blmsPm.sleep_taskMask = 0;

    if(mask & PM_SLEEP_LEG_ADV){
        blmsPm.sleep_taskMask |= TSKMSK_LEG_ADV;
    }

    if(mask & PM_SLEEP_LEG_SCAN){
        blmsPm.sleep_taskMask |= TSKMSK_PRICHN_SCAN;
    }

    if(mask & PM_SLEEP_ACL_PERIPHR){
        blmsPm.sleep_taskMask |= TSKMSK_ACL_SLAVE_ALL;
    }


    if(mask & PM_SLEEP_ACL_CENTRAL){
        blmsPm.sleep_taskMask |= TSKMSK_ACL_MASTER_ALL;
    }

    if(mask & PM_SLEEP_EXT_ADV){
        blmsPm.sleep_taskMask |= TSKMSK_EXT_ADV_ALL;
    }

    if(mask & PM_SLEEP_CIS_PERIPHR){
        blmsPm.sleep_taskMask |= TSKMSK_CIG_SLAVE_ALL;
    }

    if(mask & PM_SLEEP_CIS_CENTRAL){
        blmsPm.sleep_taskMask |= TSKMSK_CIG_MASTER_ALL;
    }

    blmsPm.sleep_mask = mask;

    irq_restore(r);
}


void        blc_pm_setDeepsleepRetentionThreshold(u32 threshold_ms)
{
    blmsPm.deepRet_thresTick = threshold_ms * SYSTEM_TIMER_TICK_1MS;
}



void blc_pm_setDeepsleepRetentionEarlyWakeupTiming(u32 earlyWakeup_us)
{
    blmsPm.deepRet_earlyWakeupTick = earlyWakeup_us * SYSTEM_TIMER_TICK_1US;
}

void blc_pm_setDeepsleepRetentionType(pm_sleep_mode_e sleep_type)
{
    blmsPm.deepRet_type = sleep_type;
}





void    blc_pm_setWakeupSource (pm_sleep_wakeup_src_e wakeup_src)
{
    blmsPm.wakeup_src = (u8)wakeup_src;
}


u32 blc_pm_getWakeupSystemTick(void)
{
    return  blmsPm.current_wakeup_tick;
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
void bls_pm_setManualLatency(u16 latency)
{
    blmsPm.user_latency = latency;
}


#if (BLS_USER_TIMER_WAKEUP_ENABLE)

    _attribute_ble_data_retention_ pm_appWakeupLowPower_callback_t  pm_appWakeupLowPowerCb = NULL;

    void blc_pm_setAppWakeupLowPower(u32 wakeup_tick, u8 enable)
    {
        blmsPm.appWakeup_tick = wakeup_tick;
        blmsPm.appWakeup_en = enable;
    }

    void blc_pm_registerAppWakeupLowPowerCb(pm_appWakeupLowPower_callback_t cb)
    {
        pm_appWakeupLowPowerCb = cb;
    }

#endif






_attribute_ram_code_
void blt_pm_calculate_accumulated_error(void)
{


    for(int conn_idx=ACL_CONN_IDX_PER0; conn_idx < LL_MAX_ACL_CONN_NUM; conn_idx++){
        if( bltSche.task_mask & (TSKMSK_ACL_CONN_0<<conn_idx) ){
            st_ll_conn_t  *pc = (st_ll_conn_t*)&blms[conn_idx];
            st_lls_conn_t *ps = (st_lls_conn_t*)&blmsSlave[conn_idx - LL_MAX_ACL_CEN_NUM];

            u32 timing_error_us = 5 + g_sleep_stimer_tick*ps->ppm_idx/(10*SYSTEM_TIMER_TICK_1MS);
            pc->pm_error_us += timing_error_us;
            ps->sleep_sys_ms += g_sleep_stimer_tick/SYSTEM_TIMER_TICK_1MS;
            ps->sleep_32k_rc += g_sleep_32k_rc_cnt;
            ps->slave_sleep_flg = 1;

        }
    }
}



#if (FAST_SETTLE)
_attribute_ram_code_ void ble_rf_fast_settle_recover(void)
{
    //TODO: You should evaluate the execution time here because it affects the deepRet wake up early
    if(fast_settle.tx_fast_en || fast_settle.rx_fast_en)
    {
        #if (!SW_DCOC_EN)
            rf_set_dcoc_cal_val(fast_settle.dcoc_cal);
        #endif

        rf_set_ldo_trim_val(fast_settle.ldo_trim);

        if(fast_settle.tx_fast_en)
        {
            rf_tx_fast_settle_init(TX_SETTLE_TIME_50US);
            rf_tx_fast_settle_en();
        }

        if(fast_settle.rx_fast_en)
        {
            rf_rx_fast_settle_init(RX_SETTLE_TIME_45US);
            rf_ble_set_rx_settle(RX_SETTLE_US);
            rf_rx_fast_settle_en();
        }
    }
}
#endif




_attribute_ram_code_sec_noinline_
void blt_pm_process_sleep_wakeup(u8 sleep_M)
{

    #if 0 //test suspend power
        if(clock_time_exceed(0, 3000000)){
            gpio_shutdown(GPIO_ALL);
            cpu_sleep_wakeup_32k_rc(SUSPEND_MODE, 0, 0);
        }
    #endif


    #if 0 //simulation
        DBG_CHN0_LOW;
        DBG_SIHUI_CHN0_LOW;
        u32 wakeup_src = 0;
        while( (u32)(blmsPm.current_wakeup_tick - clock_time()) < BIT(30) ) {
            #if (TLKAPI_DEBUG_CHANNEL == TLKAPI_DEBUG_CHANNEL_UDB)
                myudb_usb_handle_irq();
            #endif
        }
        DBG_CHN0_HIGH;
        DBG_SIHUI_CHN0_HIGH;
    #else
        // cpu_sleep_wakeup_32k_rc
        // gpio_set_high_level(GPIO_PA5);  gpio_set_high_level(GPIO_PA6);
        u32 wakeup_src = cpu_sleep_wakeup (sleep_M, PM_WAKEUP_TIMER | blmsPm.wakeup_src, blmsPm.current_wakeup_tick - blmsPm.Wakeup_early_us*SYSTEM_TIMER_TICK_1US);
        // gpio_set_low_level(GPIO_PA5);   gpio_set_low_level(GPIO_PA6);
    #endif


    #if ((MCU_CORE_TYPE == MCU_CORE_B92) || (MCU_CORE_TYPE == MCU_CORE_TL321X) || (MCU_CORE_TYPE == MCU_CORE_TL721X)) //ronglu
        // TODO: in B92, RF abnormal after enter suspend(STATUS_GPIO_ERR_NO_ENTER_PM) mode failed, root cause should be analyzed later.
        if(wakeup_src & (STATUS_ENTER_SUSPEND | STATUS_GPIO_ERR_NO_ENTER_PM))
    #else
        if(wakeup_src & STATUS_ENTER_SUSPEND)
    #endif
        {
            blt_pm_calculate_accumulated_error();

            HAL_CEVA_AES_ADDRESS_SWITCH;

            rf_drv_ble_init();
            HAL_BLE_STACK_RF_IRQ_MASK_SET;

            #if (MCU_CORE_TYPE == MCU_CORE_TL321X)
                HAL_BLE_STACK_RF_IRQ_MASK_SET;
            #endif

            #if (LL_FEATURE_ENABLE_LE_2M_PHY | LL_FEATURE_ENABLE_LE_CODED_PHY)
                if(ll_phy_switch_cb){
                    blt_ll_phy_param_reset(); //very important !!!
                }
            #endif

            #if (FAST_SETTLE)
                ble_rf_fast_settle_recover();
            #endif
        }


        /* lijing fix 20240415: Handle sleep_allowed status before STimer enable */
        if((wakeup_src & WAKEUP_STATUS_TIMER_PAD ) == WAKEUP_STATUS_PAD || wakeup_src == STATUS_GPIO_ERR_NO_ENTER_PM)  //pad, no timer
        {
            if(wakeup_src == STATUS_GPIO_ERR_NO_ENTER_PM){
                blmsPm.sleep_allowed = 0;
            }
        }
        #if (BLS_USER_TIMER_WAKEUP_ENABLE)
        else if(wakeup_src & WAKEUP_STATUS_TIMER){
            if(!blmsPm.appWakeup_flg){
                blmsPm.sleep_allowed = 0;
            }
        }
        #else
            else{
                blmsPm.sleep_allowed = 0;
            }
        #endif

    #if 1  /* 20240222 merge fix from B85m new code
              SiHui fix 20240202: set STimer enable ASAP to trigger BRX, in case that APP callback cost too many time
              SiHui & RongLu & HaoJie find this problem on B85m */

        rf_set_power_level_index(blt_extRF.txPower_index);

        #if (BLMS_DEBUG_EN)
            blmsPm.pm_entered = 1;
        #endif

        systimer_irq_enable();
    #endif


    blt_p_event_callback (BLT_EV_FLAG_SUSPEND_EXIT, (u8 *)&wakeup_src, 1);



    if ( (wakeup_src & WAKEUP_STATUS_TIMER_PAD ) == WAKEUP_STATUS_PAD || wakeup_src == STATUS_GPIO_ERR_NO_ENTER_PM)  //pad, no timer
    {
        //blmsPm.gpio_early_wkp = 1;  //hold this status, can not execute now, may cost too much timing leading to sys_tick IRQ delay
        blt_p_event_callback (BLT_EV_FLAG_GPIO_EARLY_WAKEUP, NULL, 0);

    }
    #if (BLS_USER_TIMER_WAKEUP_ENABLE)
    else if(wakeup_src & WAKEUP_STATUS_TIMER){
        if(blmsPm.appWakeup_flg){
            if(pm_appWakeupLowPowerCb){
                pm_appWakeupLowPowerCb(1);  //CALLBACK_ENTRY
            }
            //bltPm.timer_wakeup = 1;
        }
    }
    #endif



    #if (BLS_USER_TIMER_WAKEUP_ENABLE)
        blmsPm.appWakeup_flg = 0;
    #endif
    blmsPm.wakeup_src = 0;
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
int blt_sleep_process(void)
{

    if(clock_time_exceed(pm_ble_get_latest_offset_cal_time(), 20*1000000)){
//      blmsPm.sys_ppm_index = PPM_IDX_MAX;
        pm_ble_32k_rc_cal_reset();
    }

    int slave_latency_en = 0;

    if(ll_osWakeupTickProcess_cb){
        ll_osWakeupTickProcess_cb(FLAG_OS_WAKEUP_STEP_0);
    }

    /* 1. ACL slave number 1 or 2
     * 2. no other task except leg_adv & ACL slave */
#if (BLS_USER_TIMER_WAKEUP_ENABLE)
    if(!blmsPm.appWakeup_en && \
       blmsParam.cur_slave_num && blmsParam.cur_slave_num < 2 && !(bltSche.task_mask & ~(TSKMSK_LEG_ADV | TSKMSK_ACL_SLAVE_ALL)))
#else
    if(blmsParam.cur_slave_num && blmsParam.cur_slave_num < 2 && !(bltSche.task_mask & ~(TSKMSK_LEG_ADV | TSKMSK_ACL_SLAVE_ALL)))
#endif
    {
        /* 3.1. no legacy ADV exist
         * 3.2. if legacy ADV exist, only when interval >= 200mS, consider slave latency */
        if(!(bltSche.task_mask & TSKMSK_LEG_ADV) || bltLegAdv.advInt_min > ADV_INTERVAL_195MS ){

            if(blmsPm.pTask_wakeup->scheTask_flg == TSKFLG_ACL_SLAVE){


                st_ll_conn_t *pc1, *pc2, *pc_sel;
                st_lls_conn_t*  ps1, *ps2, *ps_sel;
                u8 acl_index = blmsPm.pTask_wakeup->scheTask_idx;
                pc1 = (st_ll_conn_t *)&blms[acl_index];
                ps1 = (st_lls_conn_t*)&blmsSlave[acl_index - LL_MAX_ACL_CEN_NUM];
                if(ps1->latency_available){



                    if( tick1_exceed_tick2(ps1->latency_wakeup_tick, blmsPm.next_task_tick + 5*SYSTEM_TIMER_TICK_1MS)){



                        ps_sel = ps1;
                        pc_sel = pc1;
                        slave_latency_en = 1;

                        if(blmsParam.cur_slave_num > 1){
                            for(int i=0;i<2; i++){
                                if( i != acl_index && (bltSche.task_mask & (TSKMSK_ACL_SLAVE_0<<i)) )
                                {
                                    pc2 = (st_ll_conn_t *)&blms[i + LL_MAX_ACL_CEN_NUM];
                                    ps2 = (st_lls_conn_t*)&blmsSlave[i];

                                    if(!ps2->latency_available || !tick1_exceed_tick2(ps2->latency_wakeup_tick, blmsPm.next_task_tick + 6*SYSTEM_TIMER_TICK_1MS)){
                                        slave_latency_en = 0;
                                    }
                                    else if(tick1_exceed_tick2(ps1->latency_wakeup_tick, ps2->latency_wakeup_tick)){
                                        ps_sel = ps2;
                                        pc_sel = pc2;
                                    }
                                }
                            }
                        }



                        if(slave_latency_en){
                            int adv_early_wakeup = 0;
                            if(bltSche.task_mask & TSKMSK_LEG_ADV){

                                #if (DBG_PM_TIMING)
                                    if(tick1_exceed_tick2(clock_time(), blmsPm.next_adv_tick + 500*SYSTEM_TIMER_TICK_1MS)){
                                        BLMS_ERR_DEBUG(DBG_PM_TIMING, 0xFF200000);
                                    }
                                #endif

                                /*3500: MAX_SSLOT_DURATION_ADV */
                                if(blmsPm.next_adv_tick && \
                                   tick1_exceed_tick2(blmsPm.next_adv_tick, clock_time() + 20*SYSTEM_TIMER_TICK_1US) && \
                                   tick1_exceed_tick2(ps_sel->latency_wakeup_tick, blmsPm.next_adv_tick + 3500*SYSTEM_TIMER_TICK_1US)){
                                    adv_early_wakeup = 1;
                                }
                            }

                            if(adv_early_wakeup){
                                bltLegAdv.fifoIdx_mark = 0;

                                blmsPm.wkpTsk_fifo.scheTask_oft = TSKOFT_LEG_ADV;
                                blmsPm.wkpTsk_fifo.scheTask_idx = 0;
                                blmsPm.wkpTsk_fifo.scheTask_flg = TSKFLG_LEG_ADV;
                                blmsPm.wkpTsk_fifo.taskFifo_idx = 0;

                                blmsPm.wkpTsk_oft = TSKOFT_LEG_ADV;
                                blmsPm.wkpTsk_tick = blmsPm.next_adv_tick;
                                blmsPm.current_wakeup_tick = blmsPm.next_adv_tick;
                            }
                            else{


                                blmsPm.wkpTsk_fifo.scheTask_oft = TSKOFT_ACL_SLAVE + ps_sel->acl_slv_Index;
                                blmsPm.wkpTsk_fifo.scheTask_idx = LL_MAX_ACL_CEN_NUM + ps_sel->acl_slv_Index;
                                blmsPm.wkpTsk_fifo.scheTask_flg = TSKFLG_ACL_SLAVE;
                                //blmsPm.wkpTsk_fifo.taskFifo_idx = 0;  //not used now

                                u32 sleep_duration = ps_sel->latency_wakeup_tick - clock_time();
                                ps_sel->conn_tolerance_us = sleep_duration*ps_sel->ppm_idx/(10*SYSTEM_TIMER_TICK_1MS);
                                if(sleep_duration < 200*SYSTEM_TIMER_TICK_1MS){
                                    ps_sel->conn_tolerance_us += pc_sel->pm_error_us;
                                }
                                if(ps_sel->conn_tolerance_us > ps_sel->tolerance_max_us){
                                    ps_sel->conn_tolerance_us = ps_sel->tolerance_max_us;
                                }

                                u32 acl_start_time = ps_sel->latency_wakeup_tick - BRX_LEFT_EARLY_TICK - ps_sel->conn_tolerance_us*SYSTEM_TIMER_TICK_1US;
                                // tor*2 /sSlot_unit = tor*2 /(625/32) = tor*64/625
                                #if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
                                pc_sel->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + SLOT_PROCESS_MAX_SSLOT_NUM + pdu_27b_tifs_27b_sslot[pc_sel->connPhyCtrl.conn_cur_phy - 1][pc_sel->crypt.enable] + ps_sel->conn_tolerance_us*64/625;
                                #else
                                pc_sel->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + SLOT_PROCESS_MAX_SSLOT_NUM + pdu_27b_tifs_27b_sslot[0][pc_sel->crypt.enable] + ps_sel->conn_tolerance_us*64/625;
                                #endif
                                ps_sel->latency_wakeup_flg = 1;
                                //ps_sel->slave_sleep_flg = 1;   //move to suspend exit & deep_retention back
                                blmsPm.wkpTsk_oft = TSKOFT_ACL_SLAVE + ps_sel->acl_slv_Index;
                                blmsPm.wkpTsk_tick = ps_sel->latency_wakeup_tick;
                                blmsPm.current_wakeup_tick = acl_start_time;
                            }


                            int n_sSlot = (blmsPm.current_wakeup_tick - bltSche.sSlot_tick_irq_real)*SSLOT_TICK_REVERSE;
                            blmsPm.wkpTsk_fifo.begin = bltSche.sSlot_idx_irq_real + n_sSlot;

                            #if 0 //SiHui think "end" no use when design PM code, but bring bug
                                //blmsPm.wkpTsk_fifo.end = 0;  //no need set end
                            #else
                                /* ACL slave update use: slot_diff =  bltSche.pTask_cur->end + 1 - ps->conn_update_pre_sSlotIndex;
                                 * if "end" not set, will error */
                                u16 sSlot_drtn = adv_early_wakeup ? bltLegAdv.sSlotDuration_adv : pc_sel->sSlot_allocNum;
                                blmsPm.wkpTsk_fifo.end = blmsPm.wkpTsk_fifo.begin + sSlot_drtn - 1;
                            #endif

                            bltSche.pTask_next = &blmsPm.wkpTsk_fifo;

                            #if 0  //bug, sSlot not aligned, accumulate timing error little by little
                                bltSche.sSlot_tick_irq = blmsPm.current_wakeup_tick;
                            #else
                                bltSche.sSlot_tick_irq = bltSche.sSlot_tick_start + blmsPm.wkpTsk_fifo.begin*SSLOT_TICK_NUM;
                            #endif
                            systick_irq_trigger = SYS_IRQ_TRIG_NEW_TASK;
                            systimer_set_irq_capture(bltSche.sSlot_tick_irq);
                        }
                    }
                }
            }
        }
    }


    #if (BLS_USER_TIMER_WAKEUP_ENABLE)
        blmsPm.appWakeup_flg = 0;
    #endif
    if(ll_osWakeupTickProcess_cb){
        ll_osWakeupTickProcess_cb(FLAG_OS_WAKEUP_STEP_1);
    }
    if(!slave_latency_en){
        #if (BLS_USER_TIMER_WAKEUP_ENABLE)

            if(blmsPm.appWakeup_en){
                if(tick1_exceed_tick2(blmsPm.next_task_tick, blmsPm.appWakeup_tick)){
                    exit_latency_used = false;
                    blmsPm.appWakeup_flg = 1;
                    blmsPm.current_wakeup_tick = blmsPm.appWakeup_tick;
                }
            }

            if(!blmsPm.appWakeup_flg){
                blmsPm.current_wakeup_tick = blmsPm.next_task_tick;
                exit_latency_used = true;
                if(blmsPm.pTask_wakeup->scheTask_flg == TSKFLG_ACL_SLAVE){
                    u8 acl_index = blmsPm.pTask_wakeup->scheTask_idx;
                    blmsSlave[acl_index - LL_MAX_ACL_CEN_NUM].slave_sleep_flg = 1;
                }
            }
        #else
            blmsPm.current_wakeup_tick = blmsPm.next_task_tick;
        #endif
    }

    if(ll_osWakeupTickProcess_cb){
        ll_osWakeupTickProcess_cb(FLAG_OS_WAKEUP_STEP_2);
    }

    if(!tick1_exceed_tick2(blmsPm.current_wakeup_tick, clock_time() + PM_MIN_SLEEP_US*SYSTEM_TIMER_TICK_1US)){
        exit_latency_used = false;
        return 1;
    }


    blt_p_event_callback (BLT_EV_FLAG_SLEEP_ENTER, NULL, 0);

    pm_sleep_mode_e  sleep_M = SUSPEND_MODE;
    if( blmsPm.deepRt_en && (u32)(blmsPm.current_wakeup_tick - clock_time() - blmsPm.deepRet_thresTick) < BIT(30) ){
        blmsPm.current_wakeup_tick -= blmsPm.deepRet_earlyWakeupTick;
        sleep_M = (pm_sleep_mode_e)blmsPm.deepRet_type;
    }

	if(exit_latency_used == true)
	{
		u32 exit_latency_tick = blmsPm.suspend_exitLatencyTick;
		if( blmsPm.deepRt_en && (u32)(blmsPm.current_wakeup_tick - \
				blmsPm.deepret_exitLatencyTick - clock_time() - blmsPm.deepRet_thresTick) < BIT(30) ){
			exit_latency_tick = blmsPm.deepret_exitLatencyTick;
		}

		if(systick_irq_trigger == SYS_IRQ_TRIG_NEW_TASK){
			systimer_set_irq_capture(systimer_get_irq_capture() - exit_latency_tick);
		}
		
		blmsPm.current_wakeup_tick -= exit_latency_tick;

		exit_latency_used = false;
	}


    blmsPm.sleep_enter_flag = 1;
    /* running timing for wake_up process is very critical, timing difference between wake_up and later STIMER IRQ enable
     * will be calculated to BRX window left margin. So it's better to make sure that these code are in RamCode */
    blt_pm_process_sleep_wakeup(sleep_M);



    return 0;

}





_attribute_ram_code_
void blc_ll_recoverDeepRetention(void)
{
    blt_pm_calculate_accumulated_error();

    #if (LL_FEATURE_ENABLE_LE_2M_PHY | LL_FEATURE_ENABLE_LE_CODED_PHY)
        if(ll_phy_switch_cb){
            blt_ll_phy_param_reset(); //very important !!!
        }
    #endif

    #if (BLS_USER_TIMER_WAKEUP_ENABLE)
        if( !pm_is_deepPadWakeup() && !blmsPm.appWakeup_flg){
            blmsPm.sleep_allowed = 0;
        }
    #else
        if( !pm_is_deepPadWakeup() ){
            blmsPm.sleep_allowed = 0;
        }
    #endif


    rf_set_power_level_index(blt_extRF.txPower_index);


    systimer_set_irq_capture(bltSche.sSlot_tick_irq);

    #if (BLS_USER_TIMER_WAKEUP_ENABLE)
        if(g_pm_status_info.wakeup_src & WAKEUP_STATUS_TIMER){
            if(blmsPm.appWakeup_flg){
                if(pm_appWakeupLowPowerCb){
                    if(bltSche.task_mask != 0){
                        systimer_irq_enable();
                    }
                    irq_enable();

                    pm_appWakeupLowPowerCb(1);  //CALLBACK_ENTRY
                }
            }
            else{
                blmsPm.sleep_allowed = 0;
            }
        }
        else if( pm_is_deepPadWakeup() ){
            blt_p_event_callback (BLT_EV_FLAG_GPIO_EARLY_WAKEUP, NULL, 0);
        }


        blmsPm.appWakeup_flg = 0;
    #else
        if( pm_is_deepPadWakeup() ){
            blt_p_event_callback (BLT_EV_FLAG_GPIO_EARLY_WAKEUP, NULL, 0);
        }
        else{
            blmsPm.sleep_allowed = 0;
        }
    #endif

    #if (FAST_SETTLE)
        ble_rf_fast_settle_recover();
    #endif

    blmsPm.wakeup_src = 0;
    blmsPm.pm_entered = 1;
    blmsParam.sdk_mainloop_flg = 0;

    if(bltSche.task_mask != 0){
        systimer_irq_enable();
    }
#if OS_SUP_EN
    if(blt_os_semCountIncrement_cb)
    {
        blt_os_semCountIncrement_cb();  // When you wake up, turn mainloop
    }
#endif
//  systimer_set_irq_capture(bltSche.sSlot_tick_irq);// moved before pm_appWakeupLowPowerCb()
}

#if OS_SUP_EN
_attribute_ram_code_ int blc_pm_OShandler(uint32_t expect_time)
{
    blmsPm.current_wakeup_tick = clock_time() + expect_time;
    if(!tick1_exceed_tick2(blmsPm.current_wakeup_tick, clock_time() + PM_MIN_SLEEP_US*SYSTEM_TIMER_TICK_1US)){
        return 1;
    }
    blt_p_event_callback (BLT_EV_FLAG_SLEEP_ENTER, NULL, 0);
    pm_sleep_mode_e  sleep_M = SUSPEND_MODE;
    if( blmsPm.deepRt_en && (u32)(blmsPm.current_wakeup_tick - clock_time() - blmsPm.deepRet_thresTick) < BIT(30) ){
        blmsPm.current_wakeup_tick -= blmsPm.deepRet_earlyWakeupTick;
        sleep_M = (pm_sleep_mode_e)blmsPm.deepRet_type;
    }
    // cpu_sleep_wakeup_32k_rc
    u32 wakeup_src = cpu_sleep_wakeup (sleep_M, PM_WAKEUP_TIMER | blmsPm.wakeup_src, blmsPm.current_wakeup_tick - 30*SYSTEM_TIMER_TICK_1US);

    HAL_CEVA_AES_ADDRESS_SWITCH;
    rf_drv_ble_init();//need check
    bltPHYs.cur_llPhy = BLE_PHY_1M;
    #if (FAST_SETTLE)
        ble_rf_fast_settle_recover();
    #endif
    blt_p_event_callback (BLT_EV_FLAG_SUSPEND_EXIT, (u8 *)&wakeup_src, 1);

    if ( (wakeup_src & WAKEUP_STATUS_TIMER_PAD ) == WAKEUP_STATUS_PAD || wakeup_src == STATUS_GPIO_ERR_NO_ENTER_PM)  //pad, no timer
    {
            //blmsPm.gpio_early_wkp = 1;  //hold this status, can not execute now, may cost too much timing leading to sys_tick IRQ delay
        blt_p_event_callback (BLT_EV_FLAG_GPIO_EARLY_WAKEUP, NULL, 0);
    }
    blmsPm.wakeup_src = 0;
    return 0;
}
#endif



#if (MCU_STALL_EN)
void blc_ll_appAllowMCUstall(u8 en){
    pm_mcuStall_allowFlag = en;
}

u8 blt_ll_getMCUstallFlagAppSet(void){
    return pm_mcuStall_allowFlag;
}
#endif




#endif  //end of BLMS_PM_ENABLE
