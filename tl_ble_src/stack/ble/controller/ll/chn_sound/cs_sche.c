/********************************************************************************************************
 * @file    cs_sche.c
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

#include <math.h>

#if (LL_FEATURE_ENABLE_CHANNEL_SOUNDING)

    #if OS_SUP_EN
        #include "stack/ble/os_sup/os_sup.h"
        #include "stack/ble/os_sup/os_sup_stack.h"
    #endif


extern void      blt_ll_cs_exchangeCapProc(st_ll_conn_t *pAclConn);
extern void      blt_ll_cs_exchangeSecurityStartProc(st_ll_conn_t *pAclConn);
extern void      blt_ll_cs_exchangeFaeTableProc(st_ll_conn_t *pAclConn);
extern ble_sts_t blt_ll_cs_exchangeConfigReq(st_ll_conn_t *pAclConn, cs_config_t *pCsCfg);
extern ble_sts_t blt_ll_cs_exchangeCsStartProc(st_ll_conn_t *pAclConn, cs_config_t *pCsCfg);
extern ble_sts_t blt_ll_cs_exchangeCsProcedureRepeatTerminateProc(st_ll_conn_t *pAclConn, cs_config_t *pCsCfg);
extern int       blt_ll_cs_ctrl_pdu_proc(st_ll_conn_t *pAclConn, u8 opcode, u8 *raw);
//role _support can set by API init


/***********global variables need only one copy in multi config/ reflector and initiator******************/
cs_mng_t gCsMng;
/***********************************************************************************/
extern s8 chip_intlDly_calVal[79 * 2];
extern s8 fcal_cali_table[79];
extern s8 cs_mode1_phy1M_internalDelay[79];
    #define HCI_RX_FIFO_NUM 2
_attribute_data_retention_ u8 hciCsSubeventRxFifo[CS_SUBEVENT_BUFF_LEN_MAX * HCI_RX_FIFO_NUM];


/*********************EBQ TEST ************************/
chn_sound_ll_flow_ctrl_t csFlowCtrl = {
    .csRspCheckErr   = 0,
    .csConfigExchErr = 0,
    .csStartErr      = 0,
    .csTermiFlag     = 0,
    .csPowerCtrl     = 0,
};


/******************************************************/


/******************************IRQ handle*******************************************/
int blt_ll_acl_post_checkCsTask(st_ll_conn_t *pc);
int blt_ll_insertCsSchedulerLinklist(cs_config_t *pCsCfg);
/***********************************************************************************/

/******************************Main loop  handle*******************************************/

/***********************************************************************************/
#if(Google_SRS)
static void blt_cs_AFH_2_CS_chanMap_v1(u8 *le_chan_map, u8 *cs_chan_map)
{
    u8 cs_chan_idx_array[72];
    u8 idx = 0;

    // convert LE AFH Channel map to cs channel map
    for(int i = 0; i < 5; i++){
        u8 bytes = le_chan_map[i];
        for(int j = 0; j < 8; j++){
            u8 enable = bytes & BIT(j);
            if(enable){
                u8 idx_cs;
                u8 idx_le = i * 8 + j;
                idx_cs = (idx_le < 11) ? (idx_le * 2 + 2) : (idx_le * 2 + 4);
                for(int k = idx_cs; k <= idx_cs + 1; k++){
                    // convert idx_cs to channel map bit
                    if(k!=0 && k!=1 && k!=23 && k!=24 && k!=25 && k!=77 && k!=78){
                        cs_chan_map[k/8] |= BIT(k%8);
                    }
                }
            }
        }
    }
}
#endif

    #if (CS_SLEEP_CLOCK_ACCURACY)
        /* This api is used to calculate window wide size when receive the first mode0 sync pkt when we are reflector.
 * The formula is: transmitterAllowance = (txCA ÷ 1000000) × (receiveWindowEnd - timeOfLastSync) + J(µs)
 * Note:txCA is ppm, receiveWindowEnd - timeOfLastSync is sleep duration ,J is 2 when active clock, 16 when sleep clock.
 */
        #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
        #endif
    _attribute_ram_code_ unsigned short
    blt_cs_calcWindowWideUs_of_sleepClockAccuracy(u8 ppm_div_10, unsigned int sleep_dur_us)
{
    u16 winWideUs = ((ppm_div_10 * 10 * sleep_dur_us) / 1000000) + 16;
    //  CS_EBQ_LOG("ppm(%d), sleep_dur(%d), winWideUs(%d)",ppm_div_10, sleep_dur_us, winWideUs);
    return winWideUs;
}
    #endif


    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ int
    blt_ll_cs_startProcedure(st_ll_conn_t *pc, cs_config_t *pCsCfg, u32 acl_ap_tick)
{
    pCsCfg->stopSch     = 0;
    pCsCfg->chnMRepeCnt = 0;


    pCsCfg->seqNum_mark_csSubEvent = 0;
    pCsCfg->inst_start_proc        = pc->conn_inst_mark; //must
    #if (MCU_CORE_TYPE == MCU_CORE_TL721X) || (MCU_CORE_TYPE == MCU_CORE_TL322X)
    pCsCfg->tick_proc_start = (acl_ap_tick + pCsCfg->csOft_us * SYSTEM_TIMER_TICK_1US - clock_time()) / 3 + reg_bb_timer_tick;
    #else
    pCsCfg->tick_proc_start = acl_ap_tick + pCsCfg->csOft_us * SYSTEM_TIMER_TICK_1US;
    #endif
    pCsCfg->mode0_chnReadIdx    = 0; //CHANNEL_MAP_ALL_USED_REFRESH;
    pCsCfg->nonMode0_chnReadIdx = 0; //CHANNEL_MAP_ALL_USED_REFRESH;
    pCsCfg->slip_stepReadIdx = pCsCfg->slip_stepWriteIdx = 0;
    pCsCfg->cs_procdure_1st_flag                         = 1;

    return 0;
}

    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ int
    blt_ll_cs_startEvent(st_ll_conn_t *pc, cs_config_t *pCsCfg, u16 muli)
{
    u32 acl_expect_tick;
    if (pc->aclRole == ACL_ROLE_PERIPHERAL) {
        if(bls_pconn->tick_1st_rx){
            DBG_CS_CHN5_TOGGLE;
            if(bltPHYs.cur_llPhy == BLE_PHY_2M){
                pc->csParam.ap_tick_mark   = bls_pconn->tick_1st_rx + bltPHYs.prmb_ac_us * SYSTEM_TIMER_TICK_1US - (24 + CS_HW_DELAY_2M) * SYSTEM_TIMER_TICK_1US;
            }
            else if(bltPHYs.cur_llPhy == BLE_PHY_1M){
                DBG_CS_CHN6_TOGGLE;
                pc->csParam.ap_tick_mark   = bls_pconn->tick_1st_rx + bltPHYs.prmb_ac_us * SYSTEM_TIMER_TICK_1US - (40 + CS_HW_DELAY_1M) * SYSTEM_TIMER_TICK_1US;
            }
            else{
                pc->csParam.ap_tick_mark =    bls_pconn->connExpectTime - pc->conn_intvl_tick ;
                //todo
            }
        }
        else{
            pc->csParam.ap_tick_mark =    bls_pconn->connExpectTime - pc->conn_intvl_tick ;
        }
    }
    #if (LL_CS_SNIFFER_MODE_ENABLE)
        else {
            #if (LL_RSSI_SNIFFER_MASTER_ENABLE && LL_ACL_CEN_EN)
                bool snif_used = ll_acl_sniffer_mst_irq_task_cb ? TRUE : FALSE;
                if (snif_used) {
                    if(blm_pconn->tick_1st_rx) {
                        DBG_CS_CHN5_TOGGLE;
                        if(bltPHYs.cur_llPhy == BLE_PHY_2M) {
                            pc->csParam.ap_tick_mark = blm_pconn->tick_1st_rx + bltPHYs.prmb_ac_us * SYSTEM_TIMER_TICK_1US - (24 + CS_HW_DELAY_2M) * SYSTEM_TIMER_TICK_1US;
                        }
                        else if(bltPHYs.cur_llPhy == BLE_PHY_1M) {
                            DBG_CS_CHN6_TOGGLE;
                            pc->csParam.ap_tick_mark = blm_pconn->tick_1st_rx + bltPHYs.prmb_ac_us * SYSTEM_TIMER_TICK_1US - (40 + CS_HW_DELAY_1M) * SYSTEM_TIMER_TICK_1US;
                        }
                        else{
                            //TODO, need to confirm CS_HW_DELAY_CODED
                            pc->csParam.ap_tick_mark = blm_pconn->connExpectTime - pc->conn_intvl_tick ;
                        }
                    }
                    else{
                        pc->csParam.ap_tick_mark = blm_pconn->connExpectTime - pc->conn_intvl_tick ;
                    }
                }
            #endif
        }
    #endif

    acl_expect_tick = pc->csParam.ap_tick_mark;

    pCsCfg->tick_expect_csSubevent = acl_expect_tick + pCsCfg->csOft_us * SYSTEM_TIMER_TICK_1US;
    pCsCfg->cs_inst_acl            = pc->conn_inst_mark;
    pCsCfg->cs_sub_event_oft       = -1;
    u32 sSlot_task_start           = TICKS_ABS_2_SSLOT_ABS(pCsCfg->tick_expect_csSubevent - pCsCfg->sch_early_us * SYSTEM_TIMER_TICK_1US);
    pCsCfg->sSlot_mark_csSubevent  = sSlot_task_start - pCsCfg->sSlot_csSubIntvl;
    pCsCfg->bSlot_mark_csSubevent  = SSLOT_ABS_2_BSLOT_ABS(pCsCfg->sSlot_mark_csSubevent); //There is a probability of risk
    blt_sche_addUpdate(SLOT_UPDT_CHANNEL_SOUNDING_STATE_CHANGE);

    u16 inst_next_proc = pCsCfg->inst_start_proc + pCsCfg->Procedure_Interval * (muli + 1);
    if ((u16)(inst_next_proc - pCsCfg->cs_inst_acl) <= pCsCfg->Event_Interval) {
        pCsCfg->flag_endEvtInProc = 1;
    } else {
        pCsCfg->flag_endEvtInProc = 0;
    }
    tlkapi_send_string_u32s(CS_SCHE_DEBUG_LOG_EN, "[CS][SCH] CS event", pCsCfg->csProcCount, pc->conn_inst_mark, pCsCfg->flag_endEvtInProc);
    DBG_CS_CHN9_TOGGLE;
    DBG_CS_CHN9_TOGGLE;
    return 0;
}

/*
 *  TODO: The cs sche currently consider the first cs procedure and not the first separately, using paramter 'pc->cs_pending'
 *  which be set to true when receive or send ll_cs_ind.It will cause the code hard to understand and need consider more.
 *  We can sche cs based on if procedure count is 0 or not, It's will be more clear. -- yuexin,qinghua
 */
    #if (CS_SCH_OPTIMIZE)
        #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
        #endif
    _attribute_ram_code_ int
    blt_ll_acl_post_checkCsTask(st_ll_conn_t *pc)
{
    u32 acl_expect_tick;
    u16 multi = 0;
    if (pc->aclRole == ACL_ROLE_PERIPHERAL) {
        acl_expect_tick = bls_pconn->connExpectTime - blms_pconn->conn_intvl_tick;
    } else {
        acl_expect_tick = pc->ap_tick_mark;
    }
    if (pc->cs_pending) { // scheduler the first cs procedure
        u8           cs_cfg_idx = pc->cs_pending & CS_IDX_MSK;
        cs_config_t *pCsCfg     = gCsMng.gGlobal_pCsCfg + cs_cfg_idx;
        drbg                    = (drbg_param_t *)&pc->csParam.drbg_data[0];
        u16 remainder = pCsCfg->Procedure_Interval;
        u8 start_first_proc = 0;
        if(pCsCfg->Procedure_Interval == 0) {
            if((u16)(pc->conn_inst_mark - pCsCfg->inst_start_proc) < BIT(14) && pc->conn_inst_mark == pCsCfg->inst_start_proc) {
                start_first_proc = 1;
                // if procedure interval is 0, just need start one time, then set enable to 0
                pCsCfg->cs_procedure_en             = 0;
                pCsCfg->cs_procedure_measurement_en = 0;
            }
        }
        else {
            if ((u16)(pc->conn_inst_mark - pCsCfg->inst_start_proc) < BIT(14)) // conn_inst_mark in the further of connEventCount
            {
                remainder = (u16)(pc->conn_inst_mark - pCsCfg->inst_start_proc) % pCsCfg->Procedure_Interval;
                multi     = (u16)(pc->conn_inst_mark - pCsCfg->inst_start_proc) / pCsCfg->Procedure_Interval;
            }
            // check multi to make sure how many procedure interval we have passed, update drbg to avoid mismatch
            for(int j = 0; j < multi; j++) {
                pCsCfg->csProcCount++;
                cs_drbg_init();
                drbg_backtracking_resistance(&drbg->kdrbg[0], &drbg->vdrbg[0]);
                if ((pCsCfg->procMaxCountInstant) && (pCsCfg->csProcCount >= pCsCfg->procMaxCount)) {
                    pCsCfg->cs_procedure_en             = 0;
                    pCsCfg->cs_procedure_measurement_en = 0;
                    pc->cs_pending = 0;
                    return 0;
                }
            }
            if(remainder == 0) {
                start_first_proc = 1;
            }
        }
        // note: in first procedure, we don't csProcCnt++, because ras ranging counter will be wrong(start from 1 not 0)
        if(start_first_proc) {
            blt_ll_cs_startProcedure(pc, pCsCfg, acl_expect_tick);
            blt_ll_cs_startEvent(pc, pCsCfg, multi);
            pc->csTaskEnableMask |= BIT(cs_cfg_idx);
            blt_sche_addTaskMask(TSKMSK_CS_0 << cs_cfg_idx);
            pc->cs_pending = 0;
        }
        #if (CS_PROC_REPEAT_TERMINATE) // confirm with yuexin
            if (pCsCfg->csProcCount > 0 && csFlowCtrl.csTermiFlag == Wait_Next_CS_Proc_Start) {
                csFlowCtrl.csTermiFlag = No_CS_Terminate_Proc;
            }
        #endif
        return 0;
    }


    for (int i = 0; i < gCsMng.max_num_cofig; i++) {
        u8 csProcedureFlag = 0;
        u8 csEventFlag     = 0;

        if (pc->csTaskEnableMask & (BIT(i))) {
            cs_config_t *pCsCfg = gCsMng.gGlobal_pCsCfg + i;
            if ((!pCsCfg->occupy)) {
                continue;
            }
            //When pCsCfg->Procedure_Interval is 0, remainder can be 0. When pCsCfg->Procedure_Interval is not 0, remainder can't be 0, so set remainder = pCsCfg->Procedure_Interval.
            u16 remainder = pCsCfg->Procedure_Interval;


            if (pCsCfg->Procedure_Interval != 0) {
                if ((u16)(pc->conn_inst_mark - pCsCfg->inst_start_proc) < BIT(14)) // conn_inst_mark in the further of connEventCount
                {
                    // check if cs start acl event
                    remainder = (u16)(pc->conn_inst_mark - pCsCfg->inst_start_proc) % pCsCfg->Procedure_Interval;
                    // check how many procedure has been skipped.
                    multi     = (u16)(pc->conn_inst_mark - pCsCfg->inst_start_proc) / pCsCfg->Procedure_Interval;
                }
            }
            if (remainder == 0) {
                csProcedureFlag = 1;
                u8 cs_end_flag = 0;
                if(pCsCfg->Procedure_Interval == 0){ // procedure interval==0 means max procedure count is 1, cs sche has already be called,shouldn't called here.
                    cs_end_flag = 1;
                }
                for (int j = 0; j < multi; j++) {
                    pCsCfg->csProcCount++;
                    cs_drbg_init();
                    drbg_backtracking_resistance(&drbg->kdrbg[0], &drbg->vdrbg[0]);
                    if ((pCsCfg->procMaxCountInstant) && (pCsCfg->csProcCount >= pCsCfg->procMaxCount)) {
                        cs_end_flag = 1;
                        break;
                    }
                }
                if(cs_end_flag){
                    pCsCfg->cs_procedure_en             = 0;
                    pCsCfg->cs_procedure_measurement_en = 0;
                    pc->csTaskEnableMask &= ~BIT(i);
                    blt_sche_removeTaskMask(TSKMSK_CS_0 << i);
                    return 0;
                }
                blt_ll_cs_startProcedure(pc, pCsCfg, acl_expect_tick);
            } else {
                csEventFlag = (((pc->conn_inst_mark - pCsCfg->inst_start_proc) % pCsCfg->Event_Interval) == 0) ? 1 : 0;
            }
            if (csProcedureFlag || (csEventFlag)) {
                blt_ll_cs_startEvent(pc, pCsCfg, multi);
            }
            if ((pCsCfg->chn_update_pend)) {
                s16 diff = pc->conn_inst_mark - pCsCfg->chn_update_inst;
                if (diff >= 0) {
                    pCsCfg->chn_update_pend = 0;
                    blt_cs_chnMapAndOperate(pCsCfg->Channel_Map, pCsCfg->Chm_Ind_Map, pCsCfg->Origin_Chn_Map);
                    blt_cs_extractEnableChnMap(pCsCfg->Channel_Map, pCsCfg->filteredChnArray, &pCsCfg->Chn_en_num);
                }
            }
        }
    }
    return 0;
}
    #else
        #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
        #endif
    _attribute_ram_code_ int
    blt_ll_acl_post_checkCsTask(st_ll_conn_t *pc)
{
    u32 acl_expect_tick;
    if (pc->aclRole == ACL_ROLE_PERIPHERAL) {
        acl_expect_tick = bls_pconn->connExpectTime - blms_pconn->conn_intvl_tick;
    } else {
        acl_expect_tick = pc->ap_tick_mark;
    }

    if (pc->cs_pending) {
        u8           cs_cfg_idx = pc->cs_pending & CS_IDX_MSK;
        cs_config_t *pCsCfg     = gCsMng.gGlobal_pCsCfg + cs_cfg_idx;

        if (pCsCfg->connEventCount == pc->conn_inst_mark) {
            pc->cs_pending = 0;
        #if (CS_PROC_REPEAT_TERMINATE)
            /* DRBG update when procedure cnt++, notice two situations!!!!
                 * situation 1: when cs procedure stop due to exceed max procedure count, when restart a cs procedure
                 * DRBG shouldn't update.
                 * situation 2: when cs procedure stop due to cs terminate, when restart a cs procedure,
                 * DRBG should update  -- test according to EBQ , add by yuexin
                 */
            if (pCsCfg->csProcCount > 0 && csFlowCtrl.csTermiFlag == Wait_Next_CS_Proc_Start) {
                csFlowCtrl.csTermiFlag = No_CS_Terminate_Proc;
            }
        #endif
            blt_ll_cs_startProcedure(pc, pCsCfg, acl_expect_tick);
            pc->csTaskEnableMask |= BIT(cs_cfg_idx);
            blt_sche_addTaskMask(TSKMSK_CS_0 << cs_cfg_idx);
        } else { //todo if past
        }
    }

    for (int i = 0; i < gCsMng.max_num_cofig; i++) {
        if (pc->csTaskEnableMask & (BIT(i))) {
            u16          floor  = 0;
            cs_config_t *pCsCfg = gCsMng.gGlobal_pCsCfg + i;
            if ((!pCsCfg->occupy)) {
                continue;
            }

            if (pCsCfg->Procedure_Interval) {
                floor = ((u16)(pc->conn_inst_mark - pCsCfg->inst_start_proc)) / pCsCfg->Procedure_Interval;
            }

            if (floor <= 1) { //within procedure

                u8 insert_flag = 0;

                // procedure interval == 0,  max_Procedure_cnt == 1
                if (floor == 0) {
                    if (pCsCfg->chn_update_pend && (pCsCfg->chn_update_inst <= pCsCfg->connEventCount)) { // LL/CS/CEN/INI/BV-23-C,if channel map update before cs procedure start,update here
                        pCsCfg->chn_update_pend = 0;
                        blt_cs_chnMapAndOperate(pCsCfg->Channel_Map, pCsCfg->Chm_Ind_Map, pCsCfg->Origin_Chn_Map);
                        tlkapi_send_string_data(CS_SCHE_DEBUG_LOG_EN, "channel update before cs star", pCsCfg->Channel_Map, 10);
                        blt_cs_extractEnableChnMap(pCsCfg->Channel_Map, pCsCfg->filteredChnArray, &pCsCfg->Chn_en_num);
                    }
                    if ((pCsCfg->procMaxCountInstant) && (pCsCfg->csProcCount >= pCsCfg->procMaxCount)) {
                        insert_flag             = 0;
                        pCsCfg->cs_procedure_en = 0;
                        pc->csTaskEnableMask &= ~BIT(i);
                        blt_sche_removeTaskMask(TSKMSK_CS_0 << i);
                        pCsCfg->cs_procedure_measurement_en = 0;
                        tlkapi_send_string_u32s(CS_SCHE_DEBUG_LOG_EN, "[CS][SCH] remove cs task situation1", pCsCfg->csProcCount, pCsCfg->procMaxCount);
                    }
                }

                if (floor == 1) { //procedure interval complete.such as procedure interval = 12
                    insert_flag = 1;

        #if (ACL_LOST_PKT_ERR)

                    extern u32   acl_evt_cnt;
                    extern u32   rx_cnt;
                    extern float error_rate;

                    error_rate = 100 - rx_cnt * 100 / acl_evt_cnt;
                    tlkapi_send_string_u32s(CS_SCHE_DEBUG_LOG_EN, "ACL Packet lost rate", blt_debug_hex_2_dec_display(error_rate), acl_evt_cnt, rx_cnt);
                    rx_cnt      = 0;
                    acl_evt_cnt = 0;
        #endif

                    //                  pCsCfg->csProcCount ++; // must do it before blt_ll_cs_startProcedure
                    blt_ll_cs_startProcedure(pc, pCsCfg, acl_expect_tick);


                    if ((pCsCfg->chn_update_pend)) {
                        s16 diff = pc->conn_inst_mark - pCsCfg->chn_update_inst;
                        if (diff >= 0) {
                            //                            DBG_CS_CHN6_TOGGLE;DBG_CS_CHN6_TOGGLE;
                            pCsCfg->chn_update_pend = 0;
                            blt_cs_chnMapAndOperate(pCsCfg->Channel_Map, pCsCfg->Chm_Ind_Map, pCsCfg->Origin_Chn_Map);
                            blt_cs_extractEnableChnMap(pCsCfg->Channel_Map, pCsCfg->filteredChnArray, &pCsCfg->Chn_en_num);
                        }
                    }

                    DBG_CS_CHN9_TOGGLE;
                    DBG_CS_CHN9_TOGGLE;
        #if (SL16_cs_proCnt)
                    log_b16_irq(SL_STACK_CS_TIME_EN, SL16_cs_proCnt, pCsCfg->csProcCount);
        #endif

                    if ((pCsCfg->procMaxCountInstant) && (pCsCfg->csProcCount >= pCsCfg->procMaxCount)) { //remove cs event post
                        //todo
                        insert_flag             = 0;
                        pCsCfg->cs_procedure_en = 0;
                        pc->csTaskEnableMask &= ~BIT(i);
                        blt_sche_removeTaskMask(TSKMSK_CS_0 << i);
                        pCsCfg->cs_procedure_measurement_en = 0;
                        tlkapi_send_string_u32s(CS_SCHE_DEBUG_LOG_EN, "[CS][SCH] remove cs task situation2", pCsCfg->csProcCount, pCsCfg->procMaxCount);
                    }

                } else if ((((u16)(pc->conn_inst_mark - pCsCfg->inst_start_proc)) % pCsCfg->Event_Interval) == 0) { //cs event
                    //cs event start, insert subevent, update cs event start point
                    insert_flag = 1;
                }

                if (insert_flag) { //ACL followed by channel sounding event.
                    pCsCfg->tick_expect_csSubevent = acl_expect_tick + pCsCfg->csOft_us * SYSTEM_TIMER_TICK_1US;
                    pCsCfg->cs_inst_acl            = pc->conn_inst_mark;
                    pCsCfg->cs_sub_event_oft       = -1;


        #if (SLEV_CS_event_insert)
                    log_event_irq(SL_STACK_CS_TIME_EN, SLEV_CS_event_insert);
        #endif
                    DBG_CS_CHN9_TOGGLE;
                    DBG_CS_CHN9_TOGGLE;

                    /*
                     * The Procedure_Interval field shall be set to indicate the time in units of connection intervals between the
                     * start of consecutive CS procedures. The Procedure_Interval field shall be set to a value from 0 to 65535.
                     * This value shall be set to 0 if the procedure is only to be run once
                     */
                    if (pCsCfg->Procedure_Interval) {
                        u16 inst_next_proc = pCsCfg->inst_start_proc + pCsCfg->Procedure_Interval;
                        if ((u16)(inst_next_proc - pCsCfg->cs_inst_acl) <= pCsCfg->Event_Interval) {
                            pCsCfg->flag_endEvtInProc = 1;
                        } else {
                            pCsCfg->flag_endEvtInProc = 0;
                        }
                    } else {
                        //                      u32 pCsCfg->tick_proc_start + pCsCfg->Max_Procedure_Len*SYSTEM_TIMER_TICK_625US;
                    }

                    tlkapi_send_string_u32s(CS_SCHE_DEBUG_LOG_EN, "[CS][SCH] CS event", pCsCfg->csProcCount, pc->conn_inst_mark, pCsCfg->flag_endEvtInProc);

                    u32 sSlot_task_start = TICKS_ABS_2_SSLOT_ABS(pCsCfg->tick_expect_csSubevent - pCsCfg->sch_early_us * SYSTEM_TIMER_TICK_1US);
                    /*** mark last subevent to build timelines when rebuilt*/
                    pCsCfg->sSlot_mark_csSubevent = sSlot_task_start - pCsCfg->sSlot_csSubIntvl;
                    pCsCfg->bSlot_mark_csSubevent = SSLOT_ABS_2_BSLOT_ABS(pCsCfg->sSlot_mark_csSubevent); //There is a probability of risk

                    blt_sche_addUpdate(SLOT_UPDT_CHANNEL_SOUNDING_STATE_CHANGE);
                }

            } else { //>1  out procedure
                //update cs event & cs procedure start point
                u16 fmod                = ((u16)(pc->conn_inst_mark - pCsCfg->inst_start_proc)) % pCsCfg->Procedure_Interval;
                pCsCfg->inst_start_proc = pc->conn_inst_mark - fmod;
                pCsCfg->csProcCount += floor;

        #if (SL16_cs_proCnt)
                log_b16_irq(SL_STACK_CS_TIME_EN, SL16_cs_proCnt, pCsCfg->csProcCount);
        #endif


                if ((pCsCfg->procMaxCount) && (pCsCfg->csProcCount >= pCsCfg->procMaxCount)) {
                    //todo
                    pc->csTaskEnableMask &= ~BIT(i);
                    blt_sche_removeTaskMask(TSKMSK_CS_0 << i);
                }

                u16 diff_eventcnt              = ((u16)(pc->conn_inst_mark - pCsCfg->inst_start_proc)) / pCsCfg->Event_Interval;
                pCsCfg->seqNum_mark_csSubEvent = diff_eventcnt * pCsCfg->Subevents_Per_Event; // next +1
                tlkapi_send_string_u32s(CS_SCHE_DEBUG_LOG_EN, "proc jump", pCsCfg->csProcCount, pCsCfg->seqNum_mark_csSubEvent, pc->conn_inst_mark, pCsCfg->inst_start_proc);


                u32 csOfset_us           = (pCsCfg->csOft_us);
                u32 sSlot_mark           = TICKS_ABS_2_SSLOT_ABS(acl_expect_tick + csOfset_us * SYSTEM_TIMER_TICK_1US);
                pCsCfg->bSlot_start_proc = SSLOT_ABS_2_BSLOT_ABS(sSlot_mark) - pc->bSlot_interval * fmod; // todo
            }
        }
    }
    return 0;
}

    #endif
    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ int
    blt_ll_insertCsSchedulerLinklist(cs_config_t *pCsCfg)
{
    s32 sSlot_start_cs;
    int new_task_cnt = 0;
    int int_jump_num;
    u32 csevent_start = 0;
    u8  csEvtOft      = 0;

    pCsCfg->csTsk_wptr = pCsCfg->csTsk_rptr = 0;
    if (bltSche.sSlot_idx_reset == 1 && (bltSche.build_index == 0)) {
        pCsCfg->sSlot_mark_csSubevent -= bltSche.sSlot_idx_past;
    }
    if (pCsCfg->cs_sub_event_oft >= (pCsCfg->Subevents_Per_Event - 1)) {
        return 0;
    }

    if (pCsCfg->stopSch) {
        return 0;
    }

    s32 sSlot_current_csSubevent = pCsCfg->sSlot_mark_csSubevent + pCsCfg->sSlot_csSubIntvl;
    /*** Is current subevent in the further? */
    if (sSlot_current_csSubevent >= bltSche.sSlot_idx_next) {
        int_jump_num   = 0;
        sSlot_start_cs = sSlot_current_csSubevent;
    }
    //when sslot index > Bit(17), will reset sslot, sslot next = 0; sslot past = ((sslot index +31)/32)*32 , make sure sslot past align with bslot.
    //cs task will before sslot past (sSlot_current_csSubevent < 0), at this case continue to run this task.
    else if ((bltSche.sSlot_idx_reset == 1) && (sSlot_current_csSubevent > -32)) {
        int_jump_num   = 0;
        sSlot_start_cs = sSlot_current_csSubevent;
    } else {
    #if (LL_CS_SNIFFER_MODE_ENABLE)
        /* ensure that cs event can be scheduled on, will refer to the handling of the macro 'CS_SCH_OPTIMIZE' */
        if (pCsCfg->sSlot_csSubIntvl == 0) {
            int_jump_num   = 0;
            sSlot_start_cs = bltSche.sSlot_idx_next + 2;
        } else {
            int_jump_num   = ((bltSche.sSlot_idx_next - sSlot_current_csSubevent - 1) / pCsCfg->sSlot_csSubIntvl) + 1;
            sSlot_start_cs = sSlot_current_csSubevent + (int_jump_num)*pCsCfg->sSlot_csSubIntvl;
        }
    #else
        if (pCsCfg->sSlot_csSubIntvl == 0) {
            return 0;
        }

        int_jump_num   = ((bltSche.sSlot_idx_next - sSlot_current_csSubevent - 1) / pCsCfg->sSlot_csSubIntvl) + 1;
        sSlot_start_cs = sSlot_current_csSubevent + (int_jump_num)*pCsCfg->sSlot_csSubIntvl;
    #endif
    }
    tlkapi_send_string_u32s(CS_SCHE_DEBUG_LOG_EN, "[CS][SCH] insert0", int_jump_num, sSlot_start_cs, sSlot_current_csSubevent, pCsCfg->sSlot_mark_csSubevent);
    tlkapi_send_string_u32s(CS_SCHE_DEBUG_LOG_EN, "[CS][SCH] insert1", bltSche.sSlot_idx_next, bltSche.sSlot_idx_reset, bltSche.sSlot_idx_past, pCsCfg->sSlot_csSubIntvl);


    csEvtOft      = pCsCfg->cs_sub_event_oft + int_jump_num + 1;
    csevent_start = pCsCfg->seqNum_mark_csSubEvent + int_jump_num;
    if (csEvtOft >= pCsCfg->Subevents_Per_Event) {
        return 0;
    }

    for (int j = 0; j < CS_SCH_FIFONUM; j++) {
        cs_sch_task_t *pCur_schTask = (cs_sch_task_t *)&pCsCfg->csTskFifo[j];
        //todo Subevent_Interval should add software consumption
        pCur_schTask->task.begin         = sSlot_start_cs + j * pCsCfg->sSlot_csSubIntvl;
        pCur_schTask->task.end           = pCur_schTask->task.begin + pCsCfg->sSlotCsDuration - 1;
        pCur_schTask->cs_subevent_seqNum = csevent_start + j + 1; //start from 1
        pCur_schTask->cs_procCnt         = pCsCfg->csProcCount;
        pCur_schTask->cs_oft             = csEvtOft + j;          //start from 0

        DBG_CS_CHN11_TOGGLE;

        /*
         * The number of CS subevents executed is equal to N_MAX_SUBEVENTS_PER_PROCEDURE
         * A CS procedure is considered complete and closed
         */
        if (pCur_schTask->cs_subevent_seqNum > CS_SUBEVENT_PER_PROCEDURE_MAX) {
            //          DBG_CS_CHN5_TOGGLE;DBG_CS_CHN5_TOGGLE;
            break;
        }

        /*
         * The Subevent_Len field shall be set to indicate the maximum duration of each CS subevent in
         * microseconds and shall be greater than or equal to 1250 microseconds and less than 4 seconds
         */
        //      if(pCur_schTask->end > (pCsCfg->bSlot_start_proc + pCsCfg->procMaxCount)){ //todo
        //          break;
        //      }

        tlkapi_send_string_u32s(DBG_CS_LOG_SCH_MASK_EN, "sch", pCur_schTask->cs_procCnt, csevent_start, pCur_schTask->cs_subevent_seqNum);

        if (pCur_schTask->cs_oft >= pCsCfg->Subevents_Per_Event) {
            break;
        }
        if (pCur_schTask->task.begin >= bltSche.sSlot_endIdx_dft) {     //new task beyond correct range, finish
            break;
        } else if (pCur_schTask->task.end < bltSche.sSlot_endIdx_dft) { //new task in correct range
            pCsCfg->csTsk_wptr = j;
            new_task_cnt++;
        } else {                                                        //new task across "sSlot_endIdx_dft"
            u32 cur_task_offset = TSKMSK_CS_0 + pCsCfg->idx;
            //for those task across end_idx, find the task with highest priority, to guarantee that task not missed
            if (bltPri.pri_cal[cur_task_offset] > bltPri.priMax_value) {
                bltPri.priMax_value         = bltPri.pri_cal[cur_task_offset];
                bltPri.priMax_index         = cur_task_offset;
                bltSche.sSlot_endIdx_maxPri = pCur_schTask->task.begin;
            }

            break;
        }
    }


    if (new_task_cnt) {
        int ret = blt_ll_addTask2ExistLinklistWithTaskSize((sch_task_t *)&pCsCfg->csTskFifo[0], sizeof(cs_sch_task_t), pCsCfg->csTsk_wptr + 1);
        tlkapi_send_string_u8s(CS_SCHE_DEBUG_LOG_EN, "cs insert task", new_task_cnt, ret, pCsCfg->csTsk_wptr + 1);
    }

    return 0;
}

    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ int
    blt_ll_rebuildCsSchedulerLinklist(void)
{
    int i = 0;

    cs_config_t *pCsCfg = NULL;
    for (i = 0; i < gCsMng.max_num_cofig; i++) {
        if (bltSche.task_mask & (TSKMSK_CS_0 << i)) {
            tlkapi_send_string_u8s(DBG_CS_LOG_SCH_MASK_EN & 0, "rebuildCsSch", i);
            pCsCfg = (cs_config_t *)(gCsMng.gGlobal_pCsCfg + i);
            blt_ll_insertCsSchedulerLinklist(pCsCfg);
        }
    }

    return 0;
}


    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ int
    blt_cs_subevent_post(unsigned char procedure_end_flag)
{ // complete procedure  phase_en = 0; else 1
    blms_state = BLMS_STATE_CS_SUBEVENT_E;

    u8 phase_conti_en = procedure_end_flag?0:1;

    if (gCsMng.blt_pCsCfg->phaseContinue_cal_flag) {
        ble_rf_cs_phase_continuity_dis(phase_conti_en);
        gCsMng.blt_pCsCfg->phaseContinue_cal_flag = 0;
    }

    if(gCsMng.blt_pCsCfg->CS_SYNC_PHY == BLE_2M_PHY){
        CS_LL_LOG("Subevent post, change to rf 1M ",gCsMng.blt_pCsCfg->CS_SYNC_PHY );
        rf_ble_switch_phy(BLE_1M_PHY,0); // must be called before blt_cs_subevent_rf_init, because it will change some channel sounding rf setting
    }

    ble_rf_channel_sounding_deinit();
    blt_ll_cs_tx_power_deinit();

    ble_rf_set_accessCodeThreshold(gCsMng.blt_pCsCfg->acl_ac_threshold);

    if (procedure_end_flag) {
        blmsParam.cs_procedure_busy = 0;
        gCsMng.blt_pCsCfg->phaseContinue_cal_flag = 0; // procedure complete

    /* ll cs terminate req/rsp was done, next procedure marker should be cleaned*/
        #if (CS_PROC_REPEAT_TERMINATE)
            if (csFlowCtrl.csTermiFlag == Receive_Send_CS_Terminate_RSP) {
                for (int i = 0; i < gCsMng.max_num_cofig; i++) {
                    if (blms_pconn->csTaskEnableMask & (BIT(i))) {
                        blt_sche_removeTaskMask(TSKMSK_CS_0 << i);
                    }
                }
                blms_pconn->csTaskEnableMask = 0;
                blms_pconn->cs_pending       = 0;
                csFlowCtrl.csTermiFlag       = Remove_CS_Task_with_Terminate;
            }
        #endif

    }

    if(ant_ctrl_cfg.set_ant_permu_func){
        ant_ctrl_cfg.set_ant_permu_func(ant_ctrl_cfg.ant_ctl_default);
    }

    if(gCsMng.isCSsubeventBusy)
    {
        gCsMng.isCSsubeventBusy --;
    }

    blt_ll_calculate_sSlot_next(clock_time() + SLOT_PROCESS_MAX_TICK);

    #if (SL01_cs_subevent_0)
    log_task_end_irq(SL_STACK_CS_TIME_EN, SL01_cs_subevent_0);
    #endif
    DBG_CS_CHN1_LOW;

    return 0;
}

/**
 * Check if the CS (Channel Sounding) subevent is busy.
 *
 * This function evaluates whether the CS subevent is currently occupied by checking 
 * the `isCSsubeventBusy` flag in the global CS management structure (`gCsMng`).
 *
 * @return bool Returns TRUE if the CS subevent is busy, otherwise FALSE.
 */
bool blc_cs_isSubeventBusy(void){
    return gCsMng.isCSsubeventBusy? TRUE:FALSE;
}

    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
_attribute_ram_code_
void blt_cs_subevent_rf_init(void)
{
    //CS Subevent start
    DBG_CHN5_HIGH;                                         //test configuration consumption time start
    ble_rf_channel_sounding_init();
    //set RF Power in subevent_start
    // when set cs sync/tone power, don't need rf_cs_set_power_level_singletone here, sync with driver -- yuexin20240924
    blt_ll_cs_tx_power_init();
    /* Different process for different MCU: ******************************************/
    ble_rf_set_tx_dma(0, 3); //48/16=3
                             //  This register exists in Kite/Vulture/826x, riscv architecture chips such as
                             //  Eagle do not have this register, but DMA fifo can be turned off by forcibly
                             //  setting DMA TX rptr = DMA TX wptr to use DMA default tx fifo.If the hardware
                             //  TX rptr of DMA TX fifo == hardware TX wptr, then send DMA Tx default fifo,
                             //  otherwise send DMA Tx fifo non-default area.
    HAL_REG_RF_DMA_FIFO_TX_RPTR = FLD_DMA_RPTR_CLR;
    /**********************************************************************************/
    DBG_CHN5_LOW; //test configuration consumption time post
}

    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ void
    blt_cs_subevent_rf_deinit(unsigned char phase_en)
{
    //CS Subevent stop
    DBG_CHN5_HIGH; //test configuration consumption time start
    ble_rf_cs_phase_continuity_dis(phase_en);

    ble_rf_channel_sounding_deinit();

    blt_ll_cs_tx_power_deinit();

    DBG_CHN5_LOW; //test configuration consumption time post
}

    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif

    void
    blt_cs_subevent_stepConfig_proc(cs_sch_task_t *pCsSch)
{
    u32 tSubevtJumpNum = (u32)(pCsSch->cs_subevent_seqNum - gCsMng.blt_pCsCfg->seqNum_mark_csSubEvent); //note here whether to -1, need to according to actual situation.
    if (tSubevtJumpNum == 0) {
        tSubevtJumpNum = 1;
    }


    do {
        gCsMng.blt_pCsCfg->slip_stepReadIdx = gCsMng.blt_pCsCfg->slip_stepWriteIdx;


        u8        first_cal_mode0_chn     = 0;
        u8        first_cal_non_mode0_chn = 0;
        u8        cur_step_mode           = 0;
        u8        next_step_mode          = 0;
        static u8 chn_lll                 = 0;
        u8        main_mode_repeat_cnt    = gCsMng.blt_pCsCfg->Main_Mode_Repetition;

        u32 subevent_len = 0;
        u8  step_cnt     = 0;

        if (gCsMng.blt_pCsCfg->cs_procdure_1st_flag) {
            drbg->stepCnt                        = 0;
            main_mode_repeat_cnt                 = 0; // needn't consider main mode repeat when procedure 1st
            gCsMng.blt_pCsCfg->submode_insertion = 0;
            first_cal_mode0_chn                  = 1;
            first_cal_non_mode0_chn              = 1;
        }
        gCsMng.blt_pCsCfg->mainmode_repeat_rptr = (gCsMng.blt_pCsCfg->mainmode_repeat_wptr + CHN_REPEAT_BUFF_LEN - main_mode_repeat_cnt) % CHN_REPEAT_BUFF_LEN;

        for (int j = 0; subevent_len < gCsMng.blt_pCsCfg->Subevent_Len; j++) {
            u8 seqbit_len             = 0;
            u8 sounding_sequence_task = 0;
            u8 random_sequence_task   = 0;
            u8 access_code_task       = 0;
            u8 tone_task              = 0;
            u8 non_mode0_chn_flush    = 0;

            u8                  writeSlipIdx      = gCsMng.blt_pCsCfg->slip_stepWriteIdx % SLIP_WINDOW_STEP_NUM;
            slip_window_step_t *pslip_window_step = &gCsMng.blt_pCsCfg->slip_window_step[writeSlipIdx];

            //step 1: calculate current mode
            //mode0
            if (step_cnt < gCsMng.blt_pCsCfg->Mode_0_Steps) {
                cur_step_mode = STEP_MODE_0;
            }
            //main mode + sub mode
            else {
                cur_step_mode = gCsMng.blt_pCsCfg->Main_Mode; //include mainmode repeat
                if (main_mode_repeat_cnt == 0) {              //no main mode need repeat
                    if (gCsMng.blt_pCsCfg->Sub_Mode != SUBMODE_TYPE_MODE_UNUSED) {
                        //cal sub mode insertion
                        if (!gCsMng.blt_pCsCfg->submode_insertion) {
                            gCsMng.blt_pCsCfg->submode_insertion = cs_sub_mode_insertion(gCsMng.blt_pCsCfg->Main_Mode_Max_Steps, gCsMng.blt_pCsCfg->Main_Mode_Min_Steps) + 1;
                        }

                        if (gCsMng.blt_pCsCfg->submode_insertion == 1) { //sub mode
                            cur_step_mode = gCsMng.blt_pCsCfg->Sub_Mode;
                        }
                        gCsMng.blt_pCsCfg->submode_insertion--;
                    }
                }
            }

            //step 2: calculate next step mode
            if ((step_cnt + 1) < gCsMng.blt_pCsCfg->Mode_0_Steps) {
                next_step_mode = STEP_MODE_0;
            } else {
                if (gCsMng.blt_pCsCfg->submode_insertion == 1) {
                    next_step_mode = gCsMng.blt_pCsCfg->Sub_Mode;
                } else {
                    next_step_mode = gCsMng.blt_pCsCfg->Main_Mode;
                }
            }

            //step 3: create drbg task
            if (cur_step_mode == STEP_MODE_0) {
#if(Google_SRS)
                // if mode0 channel map enable, use special mode0 map(convert from LE_AFH map)
                if(gCsMng.gGlobal_pCsCfg->srs_chn_en) {
                    if(first_cal_mode0_chn || (gCsMng.blt_pCsCfg->mode0_chnReadIdx >= gCsMng.blt_pCsCfg->srs_chn_en_num)) {
                        first_cal_mode0_chn = 0;
                        chn_sel_3a(gCsMng.blt_pCsCfg->srs_chn_en_num, gCsMng.blt_pCsCfg->srs_filteredChnArray, gCsMng.blt_pCsCfg->mode0ShuffledChnArray);
                        gCsMng.blt_pCsCfg->mode0_chnReadIdx = 0;
                    }
                    u8 tChnIdx                     = gCsMng.blt_pCsCfg->mode0_chnReadIdx % gCsMng.blt_pCsCfg->srs_chn_en_num;
                    pslip_window_step->step_chnIdx         = gCsMng.blt_pCsCfg->mode0ShuffledChnArray[tChnIdx];
                }
#else
                if(0){}
#endif
                else {
                    if (first_cal_mode0_chn || (gCsMng.blt_pCsCfg->mode0_chnReadIdx >= gCsMng.blt_pCsCfg->Chn_en_num)) {
                        first_cal_mode0_chn = 0;
                        chn_sel_3a(gCsMng.blt_pCsCfg->Chn_en_num, gCsMng.blt_pCsCfg->filteredChnArray, gCsMng.blt_pCsCfg->mode0ShuffledChnArray);

                        gCsMng.blt_pCsCfg->mode0_chnReadIdx = 0; //restart
                    }

                    u8 tChnIdx                     = gCsMng.blt_pCsCfg->mode0_chnReadIdx % gCsMng.blt_pCsCfg->Chn_en_num;
                    pslip_window_step->step_chnIdx = gCsMng.blt_pCsCfg->mode0ShuffledChnArray[tChnIdx];
                }

                gCsMng.blt_pCsCfg->mode0_chnReadIdx++;
                ////mode0 chn function end////
                //access code
                access_code_task = 1;
            } else {                        //non mode0
                if (main_mode_repeat_cnt) { // repeat main mode
                    main_mode_repeat_cnt--;
                    pslip_window_step->step_chnIdx = gCsMng.blt_pCsCfg->mainmode_repeat_chn[gCsMng.blt_pCsCfg->mainmode_repeat_rptr];
                    gCsMng.blt_pCsCfg->mainmode_repeat_rptr++;
                    gCsMng.blt_pCsCfg->mainmode_repeat_rptr = gCsMng.blt_pCsCfg->mainmode_repeat_rptr % CHN_REPEAT_BUFF_LEN;
                } else {
                    if (cur_step_mode == gCsMng.blt_pCsCfg->Main_Mode) {
                        ////non mode0 chn function start////
                        if (first_cal_non_mode0_chn || ((gCsMng.blt_pCsCfg->ChSel == CSA_3B) && (gCsMng.blt_pCsCfg->nonMode0_chnReadIdx >= gCsMng.blt_pCsCfg->Chn_en_num))) {
                            first_cal_non_mode0_chn = 0;
                            if (gCsMng.blt_pCsCfg->ChSel) { // channel select #3c
                                drbg->stepCnt -= gCsMng.blt_pCsCfg->Mode_0_Steps;
                                gCsMng.blt_pCsCfg->noneMode0ShuffledChannelNum = chn_sel_3c_cb(gCsMng.gGlobal_pCsCfg->Channel_Map, gCsMng.blt_pCsCfg->Ch3c_Shape, gCsMng.blt_pCsCfg->Ch3c_Jump, gCsMng.blt_pCsCfg->Channel_Map_Repetition, gCsMng.blt_pCsCfg->nonmode0ShuffledChnArray);
                                drbg->stepCnt += gCsMng.blt_pCsCfg->Mode_0_Steps;

                            } else { // channel select #3b
                                chn_sel_3b(gCsMng.blt_pCsCfg->Chn_en_num, gCsMng.blt_pCsCfg->filteredChnArray, gCsMng.blt_pCsCfg->nonmode0ShuffledChnArray);
                            }
                            gCsMng.blt_pCsCfg->nonMode0_chnReadIdx = 0; //restart
                        }
                        non_mode0_chn_flush = 1;
                        u8 tChnIdx          = 0;
                        if (gCsMng.blt_pCsCfg->ChSel == CSA_3C) {
                            tChnIdx = gCsMng.blt_pCsCfg->nonMode0_chnReadIdx;
                        } else {
                            tChnIdx = gCsMng.blt_pCsCfg->nonMode0_chnReadIdx % gCsMng.blt_pCsCfg->Chn_en_num;
                        }
                        pslip_window_step->step_chnIdx = gCsMng.blt_pCsCfg->nonmode0ShuffledChnArray[tChnIdx];
                        gCsMng.blt_pCsCfg->nonMode0_chnReadIdx++;
                        ////non mode0 chn function end////

                        //record main mode chn for next subevent repeat
                        gCsMng.blt_pCsCfg->mainmode_repeat_chn[gCsMng.blt_pCsCfg->mainmode_repeat_wptr % CHN_REPEAT_BUFF_LEN] = pslip_window_step->step_chnIdx;
                        gCsMng.blt_pCsCfg->mainmode_repeat_wptr++;
                        gCsMng.blt_pCsCfg->mainmode_repeat_wptr = gCsMng.blt_pCsCfg->mainmode_repeat_wptr % CHN_REPEAT_BUFF_LEN;
                        chn_lll                                 = pslip_window_step->step_chnIdx;
                    } else {                                      //sub mode
                        pslip_window_step->step_chnIdx = chn_lll; //gCsMng.blt_pCsCfg->mainmode_repeat_chn[(gCsMng.blt_pCsCfg->mainmode_repeat_wptr + 4 - 1)&0x03];
                    }
                }


                if ((cur_step_mode == STEP_MODE_1) || (cur_step_mode == STEP_MODE_3)) {
                    //access code
                    access_code_task = 1;
                    //sounding sequence marker position random & signal select
                    if (gCsMng.blt_pCsCfg->RTT_Type == RTT_Type_32bit_ss) {
                        sounding_sequence_task = 1;
                        seqbit_len             = 32;
                    } else if (gCsMng.blt_pCsCfg->RTT_Type == RTT_Type_96bit_ss) {
                        sounding_sequence_task = 1;
                        seqbit_len             = 96;
                    }
                    //random sequence generation
                    else if (gCsMng.blt_pCsCfg->RTT_Type == RTT_Type_32bit_rs || gCsMng.blt_pCsCfg->RTT_Type == RTT_Type_64bit_rs ||
                             gCsMng.blt_pCsCfg->RTT_Type == RTT_Type_96bit_rs || gCsMng.blt_pCsCfg->RTT_Type == RTT_Type_128bit_rs) {
                        random_sequence_task = 1;
                        seqbit_len           = 32 * (gCsMng.blt_pCsCfg->RTT_Type - 2);
                    }
                }

                if ((cur_step_mode == STEP_MODE_2) || (cur_step_mode == STEP_MODE_3)) {
                    tone_task = 1;
                }
            }

            if (access_code_task) {
                //access code
                if (tSubevtJumpNum == 1) { //subevent maybe skip over,tSubevtJumpNum > 1 mean this subevent be skip.
                    cs_access_addr((u8 *)&pslip_window_step->step_reflAA, (u8 *)&pslip_window_step->step_initAA);
                    tlkapi_send_string_u32s(CS_SCHE_DEBUG_LOG_EN, "m0AA", pslip_window_step->step_reflAA, pslip_window_step->step_initAA, step_cnt, pslip_window_step->step_chnIdx);
                }
            }

            if (tone_task) {
                //Antenna path permutation index selection
                if (gCsMng.blt_pCsCfg->aci != 0) { //only when exist multiple antenna path then calculate the API.
                    pslip_window_step->step_antPathPermIdx = cs_antenna_path_perm(gCsMng.blt_pCsCfg->antennaPathNum);
                } else {
                    pslip_window_step->step_antPathPermIdx = 0;
                }
                //T_PM CS tone extension slot transmission presence
                cs_tpm_ext((u8 *)&pslip_window_step->extSlot.step_extSlotFlag);
            }
            pslip_window_step->seqMode = 0;
            if (sounding_sequence_task) {
                //sounding sequence marker position random & signal select
                pslip_window_step->step_initRttSSPos[0] = 0xff;
                pslip_window_step->step_initRttSSPos[1] = 0xff;
                pslip_window_step->step_reflRttSSPos[0] = 0xff;
                pslip_window_step->step_reflRttSSPos[1] = 0xff;
                cs_ss_marker(seqbit_len, pslip_window_step->step_initRttSSPos, pslip_window_step->step_reflRttSSPos, pslip_window_step->step_initRttSeq, pslip_window_step->step_reflRttSeq);
                //todo optimize by role
                u8 select[4] = {0};
                select[0]    = pslip_window_step->step_initRttSeq[0] & BIT(0);
                select[1]    = pslip_window_step->step_initRttSeq[1] & BIT(0);
                select[2]    = pslip_window_step->step_reflRttSeq[0] & BIT(0);
                select[3]    = pslip_window_step->step_reflRttSeq[1] & BIT(0);

                u8 position[4] = {0};
                position[0]    = pslip_window_step->step_initRttSSPos[0];
                position[1]    = pslip_window_step->step_initRttSSPos[1];
                position[2]    = pslip_window_step->step_reflRttSSPos[0];
                position[3]    = pslip_window_step->step_reflRttSSPos[1];

                for (int i = 0; i < (seqbit_len >> 3); i++) {
                    pslip_window_step->step_initRttSeq[i] = 0xaa;
                    pslip_window_step->step_reflRttSeq[i] = 0xaa;
                }
                for (int k = 0; k < 4; k++) {
                    u8 *pRTT = NULL;
                    if (k < 2) {
                        pRTT = (u8 *)(&pslip_window_step->step_initRttSeq[0]);
                    } else {
                        pRTT = (u8 *)(&pslip_window_step->step_reflRttSeq[0]);
                    }

                    if (position[k] != 0xff) {
                        if (select[k]) { //0011
                            pRTT[position[k] / 8] &= ~BIT(position[k] % 8);
                            position[k]++;
                            pRTT[position[k] / 8] &= ~BIT(position[k] % 8);
                            position[k]++;
                            pRTT[position[k] / 8] |= BIT(position[k] % 8);
                            position[k]++;
                            pRTT[position[k] / 8] |= BIT(position[k] % 8);
                        } else { //1100
                            pRTT[position[k] / 8] |= BIT(position[k] % 8);
                            position[k]++;
                            pRTT[position[k] / 8] |= BIT(position[k] % 8);
                            position[k]++;
                            pRTT[position[k] / 8] &= ~BIT(position[k] % 8);
                            position[k]++;
                            pRTT[position[k] / 8] &= ~BIT(position[k] % 8);
                        }
                    }
                }

                pslip_window_step->seqMode = 1; //sounding sequence
            }

            if (random_sequence_task) {
                //random sequence generation
                cs_random_seq(pslip_window_step->step_initRttSeq, pslip_window_step->step_reflRttSeq, seqbit_len);
    #if (0) // LL/CS/CEN/INI/BV-12-C no need reverse random sequence -- yuexin
                for (int i = 0; i < (seqbit_len >> 4); i++) {
                    u8 data                                                       = pslip_window_step->step_initRttSeq[i];
                    pslip_window_step->step_initRttSeq[i]                         = pslip_window_step->step_initRttSeq[(seqbit_len >> 3) - 1 - i];
                    pslip_window_step->step_initRttSeq[(seqbit_len >> 3) - 1 - i] = data;

                    data                                                          = pslip_window_step->step_reflRttSeq[i];
                    pslip_window_step->step_reflRttSeq[i]                         = pslip_window_step->step_reflRttSeq[(seqbit_len >> 3) - 1 - i];
                    pslip_window_step->step_reflRttSeq[(seqbit_len >> 3) - 1 - i] = data;
                }
    #endif
                pslip_window_step->seqMode = 2;            //random sequence
            }
            pslip_window_step->seqLen = (seqbit_len >> 3); //byte
    #if (0)                                                // add some debug info
            tlkapi_printf(CS_SCHE_DEBUG_LOG_EN, "cnt,non,chn:%d %d %d", step_cnt, gCsMng.blt_pCsCfg->nonMode0_chnReadIdx, pslip_window_step->step_chnIdx);
            tlkapi_send_string_data(CS_SCHE_DEBUG_LOG_EN, "AA info", (u8 *)&pslip_window_step->step_initAA, 8);

            if (sounding_sequence_task || random_sequence_task) {
                tlkapi_send_string_data(CS_SCHE_DEBUG_LOG_EN, "SS info", (u8 *)&pslip_window_step->step_initRttSeq[0], 32);
            }
    #endif
            //step 4: prepare next step
            drbg->stepCnt++;
            step_cnt++;
            gCsMng.blt_pCsCfg->slip_stepWriteIdx++;
            pslip_window_step->step_modeType   = cur_step_mode;
            pslip_window_step->subeventEndFlag = 0;
            pslip_window_step->proceStopFlag   = 0;

            //step 5: check subevent whether is done.
            u32  next_step_len = 0;
            u16 *pDurUs        = (u16 *)&gCsMng.blt_pCsCfg->mode0Step_durUs; //mode0Step_durUs ~ mode3Step_durUs address are continuously
            subevent_len += pDurUs[cur_step_mode & 0x03];
            next_step_len += pDurUs[next_step_mode & 0x03];

            if ((non_mode0_chn_flush == 1) && (gCsMng.blt_pCsCfg->nonMode0_chnReadIdx >= gCsMng.blt_pCsCfg->Chn_en_num)) { //need check mode0 chnmap repeat
                gCsMng.blt_pCsCfg->chnMRepeCnt++;
            }

            // calculate if exceed the latest subEvtLen
            if (gCsMng.gGlobal_pCsCfg->calcSubEvtMargin == 1 && (subevent_len + next_step_len) > gCsMng.gGlobal_pCsCfg->subEvtMargin) {
                pslip_window_step->subeventEndFlag = 1;
                gCsMng.blt_pCsCfg->procStopRsn     = PROC_DONE_MAX_PROC_LEN;
            }


            //subevent done
            if (((subevent_len + next_step_len) > gCsMng.blt_pCsCfg->Subevent_Len) || (step_cnt == 160)) {
                pslip_window_step->subeventEndFlag = 1;

                if ((pCsSch->cs_subevent_seqNum == CS_SUBEVENT_PER_PROCEDURE_MAX) ||
                    ((gCsMng.blt_pCsCfg->flag_endEvtInProc) && (gCsMng.blt_pCsCfg->cs_sub_event_oft == (gCsMng.blt_pCsCfg->Subevents_Per_Event - 1))))

                {
                    gCsMng.blt_pCsCfg->procStopRsn = PROC_DONE_LAST_EVENT;
                   DBG_CS_CHN6_TOGGLE;
                   DBG_CS_CHN6_TOGGLE;
                }

                /* when cs subevent done, check if exceed max_procedure_len,two situation to be considered
                 * situation 1: no step to be continued, cs procedure stop
                 * situation 2: still some cs step to be continued
                 */
                gCsMng.blt_pCsCfg->subEvtCnt++;
                if (gCsMng.blt_pCsCfg->subEvtCnt >= gCsMng.blt_pCsCfg->max_subEvtCnt) {
                    if (gCsMng.gGlobal_pCsCfg->subEvtContinue == 1) // not the latest subevent
                    {
                        gCsMng.gGlobal_pCsCfg->calcSubEvtMargin = 1;
                        gCsMng.gGlobal_pCsCfg->subEvtContinue   = 0;
                    } else {
                        gCsMng.blt_pCsCfg->procStopRsn = PROC_DONE_MAX_PROC_LEN;
                    }
                }
            }


            //procedure done
            if ((gCsMng.blt_pCsCfg->ChSel == CSA_3B) && (gCsMng.blt_pCsCfg->chnMRepeCnt >= gCsMng.blt_pCsCfg->Channel_Map_Repetition)) {
                gCsMng.blt_pCsCfg->procStopRsn     = PROC_DONE_ALL_3B_CHN_USED;
                pslip_window_step->subeventEndFlag = 1;
            }
            if ((gCsMng.blt_pCsCfg->ChSel == CSA_3C) && gCsMng.blt_pCsCfg->noneMode0ShuffledChannelNum && (gCsMng.blt_pCsCfg->nonMode0_chnReadIdx >= gCsMng.blt_pCsCfg->noneMode0ShuffledChannelNum)) {
                gCsMng.blt_pCsCfg->procStopRsn     = PROC_DONE_ALL_3C_CHN_USED;
                pslip_window_step->subeventEndFlag = 1;
            }
            if (drbg->stepCnt >= 256) {
                gCsMng.blt_pCsCfg->procStopRsn     = PROC_DONE_MAX_STEP_CNT;
                pslip_window_step->subeventEndFlag = 1;
            }

            if (gCsMng.blt_pCsCfg->procStopRsn & (PROC_DONE_MAX_STEP_CNT | PROC_DONE_ALL_3B_CHN_USED | PROC_DONE_ALL_3C_CHN_USED | PROC_DONE_LAST_EVENT | PROC_DONE_MAX_PROC_LEN)) {
                //              tlkapi_printf(1,"procedure close condition: %d",gCsMng.blt_pCsCfg->procStopRsn);
                pslip_window_step->proceStopFlag = 1;
                // reset, not effect next procedure
                gCsMng.blt_pCsCfg->subEvtCnt   = 0;
                gCsMng.blt_pCsCfg->procStopRsn = 0;
                if (gCsMng.gGlobal_pCsCfg->calcSubEvtMargin) {
                    gCsMng.gGlobal_pCsCfg->calcSubEvtMargin = 0;
                    gCsMng.gGlobal_pCsCfg->subEvtContinue   = 1;
                }
            }

            if (pslip_window_step->subeventEndFlag) {
                break;
            }
        }
        gCsMng.blt_pCsCfg->cs_procdure_1st_flag = 0;

    } while (--tSubevtJumpNum);
}


    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ int
    blt_cs_subevent_start(int cs_cfg_idx, void *p)
{
    cs_sch_task_t *pCsSch = (cs_sch_task_t *)p;
    gCsMng.blt_pCsCfg     = (cs_config_t *)(gCsMng.gGlobal_pCsCfg + cs_cfg_idx);
    st_ll_conn_t *pAcl    = (st_ll_conn_t *)blt_ll_getAclConnPtr(gCsMng.blt_pCsCfg->aclHandle);
    drbg                  = (drbg_param_t *)&pAcl->csParam.drbg_data[0];
    DBG_CS_CHN1_HIGH;

    gCsMng.blt_pCsCfg->mode0SyncMark = gCsMng.blt_pCsCfg->Mode_0_Steps;
#if(Google_SRS)
    if(gCsMng.gGlobal_pCsCfg->srs_chn_en) {
        smemset(gl_mode0_chan_map,0,10);
        blt_cs_AFH_2_CS_chanMap_v1(pAcl->acl_chnParam.chmTbl,gl_mode0_chan_map);
        tlkapi_send_string_data(1,"LE AFH MAP",pAcl->acl_chnParam.chmTbl,5);
        tlkapi_send_string_data(1,"Mode0 MAP",gl_mode0_chan_map,10);
        blt_cs_extractEnableChnMap(gl_mode0_chan_map,gCsMng.blt_pCsCfg->srs_filteredChnArray,&gCsMng.blt_pCsCfg->srs_chn_en_num);
    }
#endif
    #if (CS_SLEEP_CLOCK_ACCURACY)
    if (gCsMng.blt_pCsCfg->Role == CHANNEL_SOUNDING_ROLE_REFLECTOR) {
        // get last rx tick and then calculate next subevent start tick, only with a very big offset(400ms), calc this value.
        if (gCsMng.blt_pCsCfg->csOft_us >= 400000) {
            // cause current code,we dindn't handle with LL_CLOCK_ACCURACY_REQ/RSP, So use 500ppm as default when offset > 400ms
            gCsMng.blt_pCsCfg->winWideUs = blt_cs_calcWindowWideUs_of_sleepClockAccuracy(50, gCsMng.blt_pCsCfg->csOft_us); // pAcl->ppm_div_10
            gCsMng.blt_pCsCfg->winWideUs += 50;                                                                            // add more margin to pass EBQ.
            CS_EBQ_LOG("ppm is %d, offset is %d(ms),winWide is %d(us)", pAcl->ppm_div_10 * 10, gCsMng.blt_pCsCfg->csOft_us / 1000, gCsMng.blt_pCsCfg->winWideUs);
        } else {
            gCsMng.blt_pCsCfg->winWideUs = 0;
        }
    }
    #else
    gCsMng.blt_pCsCfg->winWideUs = 0; // only use window widening when test EBQ, in daily test, don't use.
    #endif


    #if (SL01_cs_subevent_0)
    log_task_begin_irq(SL_STACK_CS_TIME_EN, SL01_cs_subevent_0);
    #endif

    gCsMng.blt_pCsCfg->sSlot_mark_csSubevent = bltSche.sSlot_idx_irq_real;
    gCsMng.blt_pCsCfg->bSlot_mark_csSubevent = bltSche.bSlot_idx_irq_real;


    u16 stop_procedure   = 0;
    int cs_subevent_jump = 0;
    if (gCsMng.blt_pCsCfg->csProcCount != pCsSch->cs_procCnt) {
        cs_subevent_jump               = pCsSch->cs_subevent_seqNum;
        gCsMng.blt_pCsCfg->csProcCount = pCsSch->cs_procCnt;

        tlkapi_send_string_u8s(CS_SCHE_DEBUG_LOG_EN, "[CS] procedure start", gCsMng.blt_pCsCfg->procMaxCount);

        stop_procedure = 1;
        //todo New procedure start, main mode, repetition all need to start again
    } else {
        cs_subevent_jump = (u32)(pCsSch->cs_subevent_seqNum - gCsMng.blt_pCsCfg->seqNum_mark_csSubEvent);

        /*
         * 1 - none subevent jump
         * 2 - 1 subevent jump
         */
        if (cs_subevent_jump > 2) {
            stop_procedure = 1;
            //tlkapi_send_string_u32s((stkLog_mask & STK_LOG_LL_CS), "error subevent jump", cs_subevent_jump);
        }
    }


    if (stop_procedure) {
        gCsMng.blt_pCsCfg->stopSch = 1;
        blt_sche_addUpdate(SLOT_UPDT_CHANNEL_SOUNDING_STATE_CHANGE);
        blms_state = BLMS_STATE_CS_SUBEVENT_E;
        return 0;
    }
    //    DBG_CS_CHN4_HIGH;
    gCsMng.blt_pCsCfg->cs_sub_event_oft = pCsSch->cs_oft;


    if(gCsMng.blt_pCsCfg->CS_SYNC_PHY == BLE_2M_PHY){
        CS_LL_LOG("sync phy 2M: %d, rf switch to 2M ",gCsMng.blt_pCsCfg->CS_SYNC_PHY );
        rf_ble_switch_phy(BLE_2M_PHY,0); // must be called before blt_cs_subevent_rf_init, because it will change some channel sounding rf setting
    }

    /* if mode2 or mode3 should enable phase continue
     *  tx_cali and rx cali just get once is ok
     *  2024.08.21  fanqh & jiapeng
     */
    if (!gCsMng.cs_get_rf_cali_flag &&
        ((gCsMng.blt_pCsCfg->Sub_Mode == SUBMODE_TYPE_MODE_UNUSED) && (gCsMng.blt_pCsCfg->Main_Mode != STEP_MODE_1))) {
        extern void ble_rf_cs_settle_cali_init(void);
        ble_rf_cs_settle_cali_init(); //@48mhz 345us,@96mhz 339us
        gCsMng.cs_get_rf_cali_flag = 1;
    }

    if (gCsMng.cs_get_rf_cali_flag) {
        blmsParam.cs_procedure_busy  = 1;
        gCsMng.blt_pCsCfg->phaseContinue_cal_flag = 1;
    }

    blt_cs_subevent_rf_init();

    gCsMng.blt_pCsCfg->acl_ac_threshold = reg_rf_modem_sync_thres_ble; //save acl ac_threshold, cs use threshold as 32
    ble_rf_set_accessCodeThreshold(CS_ACCESSCODE_THRESHOLD);

    gCsMng.blt_pCsCfg->mode0_rx_flag = 0;                              //clear per subevent.

    gCsMng.blt_pCsCfg->step_expect_tick = gCsMng.blt_pCsCfg->tick_expect_csSubevent + pCsSch->cs_oft * gCsMng.blt_pCsCfg->subEvtIntvl_625us * SYSTEM_TIMER_TICK_625US;

    if (gCsMng.blt_pCsCfg->phaseContinue_cal_flag) {
        ble_rf_cs_phase_continuity_en(); //@48mhz 66us,@96mhz 53us
    }

    blt_cs_subevent_stepConfig_proc(pCsSch);

    gCsMng.blt_pCsCfg->sync_pky_cnt = 0;
    cs_param_t *pCsParam = &pAcl->csParam;

    if(ant_ctrl_cfg.set_ant_param_func){
        ant_ctrl_cfg.set_ant_param_func(gCsMng.blt_pCsCfg, pCsParam->CS_SYNC_AntSel);
    }

    #if (LL_CS_SNIFFER_MODE_ENABLE)
    if (csSniffer_param.totalNodeNum > 1) {
        csSniffer_param.curProcedureCountIdx = (gCsMng.blt_pCsCfg->csProcCount & CS_COUNTER_CONVERT_SUB_NODE_INDEX_MASK) % csSniffer_param.totalNodeNum;
        tlkapi_send_string_u32s(DBG_CS_DATA_EN, "[STK][CS] csProcCount", gCsMng.blt_pCsCfg->csProcCount, csSniffer_param.curProcedureCountIdx);

        if (csSniffer_param.curProcedureCountIdx != csSniffer_param.curNodeIdx) {
            gCsMng.blt_pCsCfg->subEvtCnt   = 0;
            gCsMng.blt_pCsCfg->procStopRsn = 0;
            #if (!CS_SCH_OPTIMIZE)
                gCsMng.blt_pCsCfg->csProcCount++;
            #endif
            gCsMng.blt_pCsCfg->stopSch = 1;
            blt_sche_addUpdate(SLOT_UPDT_CHANNEL_SOUNDING_STATE_CHANGE);
            blt_cs_subevent_post(1);
            gCsMng.blt_pCsCfg->seqNum_mark_csSubEvent = pCsSch->cs_subevent_seqNum;

            DBG_CS_CHN1_LOW;

            return 0;
        }
    }
    else{
        tlkapi_send_string_u32s(DBG_CS_DATA_EN, "[STK][CS] csProcCount", gCsMng.blt_pCsCfg->csProcCount);
    }
    #endif


    gCsMng.isCSsubeventBusy ++;

    if ((gCsMng.blt_pCsCfg->Role == CHANNEL_SOUNDING_ROLE_INITIATOR)) {
    #if CAP_CALIB_EN

        if (gCsMng.capCalibValue != gCsMng.capCalibValuePre) {
#if(MCU_CORE_TYPE == MCU_CORE_TL322X)
            analog_write(0x10a, (analog_read(0x10a) & 0xc0) | gCsMng.capCalibValue);
#else
            analog_write(0x8a, (analog_read(0x8a) & 0xc0) | gCsMng.capCalibValue);
#endif
            gCsMng.capCalibValuePre = gCsMng.capCalibValue;
        }
    #endif
        gCsMng.blt_pCsCfg->step_expect_tick += (gCsMng.blt_pCsCfg->T_FCS_Us + 4 + pCsSch->cs_oft * 5) * SYSTEM_TIMER_TICK_1US;
        ll_cs_initiator_irq_task_cb(FLAG_CS_STEP_INIT_STX_START); // blt_cs_initiator_irq_task     blt_cs_init_step_stx_start
    } else {
        gCsMng.blt_pCsCfg->step_expect_tick += (gCsMng.blt_pCsCfg->T_FCS_Us + 4 + pCsSch->cs_oft * 5) * SYSTEM_TIMER_TICK_1US;
        ll_cs_reflector_irq_task_cb(FLAG_CS_STEP_REFL_SRX_START); //blt_cs_reflector_irq_task      blt_cs_refl_stepSrx
    }

                                                                  //    DBG_CS_CHN4_LOW;

    gCsMng.blt_pCsCfg->seqNum_mark_csSubEvent = pCsSch->cs_subevent_seqNum; //move here. above need to use mark and current subevent number.

    return 0;
}


    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ int
    blt_cs_interrupt_task(int flag, void *p)
{
    if (flag == FLAG_CS_ACL_CB_FLAG) {
        st_ll_conn_t *pAcl = (st_ll_conn_t *)p;
        //ACL conn establish set channle sounding param
        //Role_Enable: all disable,   CS_SYNC_Antenna_Selection is set to 0x01
        cs_param_t *pCsParam = &pAcl->csParam;

        pCsParam->role_enable          = 0;
        pCsParam->CS_SYNC_AntSel       = 1;
        pCsParam->cs_cap_req           = 0;
        pCsParam->cs_cap_exchange      = 0;
        pCsParam->cs_fae_exchange      = 0;
        pCsParam->cs_config_req        = 0;
        pCsParam->cs_security_enable   = 0;
        pCsParam->cs_security_exchange = 0;
        pCsParam->cs_req               = 0;
        pCsParam->cs_fae_req           = 0;
        pAcl->csMapMask                = 0;

        for (int i = 0; i < gCsMng.max_num_cofig; i++) {
            cs_config_t *pCfg = gCsMng.gGlobal_pCsCfg + i;
            if ((pCfg->occupy) && (pAcl->acl_conHandle == pCfg->aclHandle)) {
                pCfg->occupy          = 0;
                pCfg->state           = 0;
                pCfg->chn_update_pend = 0;
            }
        }
    } else if (flag & FLAG_SCHEDULE_BUILD) {
        blt_ll_rebuildCsSchedulerLinklist();
    } else if (flag & FLAG_SCHEDULE_POLL) {
        blt_ll_acl_post_checkCsTask((st_ll_conn_t *)p);
    } else if (flag & FLAG_CS_ACL_DISCONN_CB) {
        st_ll_conn_t *pc = (st_ll_conn_t *)p;
        for (int i = 0; i < gCsMng.max_num_cofig; i++) {
            if (pc->csTaskEnableMask & (BIT(i))) {
                blt_sche_removeTaskMask(TSKMSK_CS_0 << i);
            }
        }
        pc->csTaskEnableMask = 0;
        pc->cs_pending       = 0;
    } else if (flag & FLAG_CS_SUBEVENT_START) {
        blt_cs_subevent_start(flag & FLAG_SCHEDULE_TASK_IDX_MASK, p);
    } else if (flag & FLAG_INSERT_SCHTSK_CONFLICT) {
        return 1;
    }

    return 0;
}

void blt_ll_cs_mainloop(void)
{
    for (int conn_idx = ACL_CONN_IDX_CEN0; conn_idx < LL_MAX_ACL_CONN_NUM; conn_idx++) {
        st_ll_conn_t *pc       = (st_ll_conn_t *)(u32)&blms[conn_idx];
        cs_param_t   *pCsParam = &pc->csParam;
        cs_config_t  *pCsCfg   = NULL;
        if (!pc->connState) {
            continue;
        }
    #if (HCI_CCO_BI_109_C)
        if (((pCsParam->cs_cap_req & (PROC_CS_CAP_SEND_REQ | PROC_CS_CAP_SEND_RSP)) || (pCsParam->cs_cap_req == PROC_CS_CAP_EVT_PENDING))) {
    #else
        if ((pc->llcp_flag.bit.ll_feat_exg_flag) && ((pCsParam->cs_cap_req & (PROC_CS_CAP_SEND_REQ | PROC_CS_CAP_SEND_RSP)) || (pCsParam->cs_cap_req == PROC_CS_CAP_EVT_PENDING))) {
    #endif
            blt_ll_cs_exchangeCapProc(pc);
        }
        if ((pCsParam->cs_security_enable & (PROC_CS_SEC_SEND_REQ | PROC_CS_SEC_SEND_RSP)) || (pCsParam->cs_security_enable == PROC_CS_SEC_EVT_PENDING)) {
            blt_ll_cs_exchangeSecurityStartProc(pc);
        }
        if ((pCsParam->cs_fae_req & (PROC_CS_FAE_SEND_REQ | PROC_CS_FAE_SEND_RSP)) || (pCsParam->cs_fae_req == PROC_CS_FAE_EVT_PENDING)) {
            blt_ll_cs_exchangeFaeTableProc(pc);
        }
        if ((pCsParam->cs_config_req & (PROC_CS_CONFIG_SEND_REQ | PROC_CS_CONFIG_SEND_RSP)) || (pCsParam->cs_config_req == PROC_CS_CONFIG_EVT_PENDING)) {
            pCsCfg = gCsMng.gGlobal_pCsCfg + pCsParam->cs_config_pend_idx;
            blt_ll_cs_exchangeConfigReq(pc, pCsCfg);
        }
        //      pCsCfg = gCsMng.gGlobal_pCsCfg + pCsParam->cs_config_pend_idx;
    #if (CS_EBQ_TEST & 0)
        /* EBQ case:LL/CS/CEN/INI/BV-04-C, after ll_cs_config,if no cs config rsp in timeout tick(now is 3s)
         * cs config complete evt should be reported with error code HCI_ERR_CMD_DISALLOWED*/
        if (pCsParam->cs_config_req && clock_time_exceed(pc->ll_rsp_timeout_tick, 3000 * 1000)) {
            hci_le_csConfigComplete_evt(HCI_ERR_CMD_DISALLOWED, pc->acl_conHandle, (u8 *)pCsCfg);
            pCsParam->cs_config_req = 0;
            pc->ll_rsp_timeout_tick = 0;
            CS_HCI_LOG("[Config] wait config rsp timeout!");
        }
    #endif
        if ((pCsParam->cs_req & (PROC_CS_SEND_REQ | PROC_CS_SEND_RSP | PROC_CS_SEND_IND)) || (pCsParam->cs_req == PROC_CS_EVT_PENDING)) {
            pCsCfg = gCsMng.gGlobal_pCsCfg + pCsParam->cs_pend_idx;
    #if (CS_EBQ_TEST && APP_POWER_CONTROL)
            if (!pc->pPclCb->pc_sendReq)
    #endif
            {
                blt_ll_cs_exchangeCsStartProc(pc, pCsCfg);
            }
        }
    #if (CS_EBQ_TEST & 0)
        /* EBQ case:HCI/CCO/BI-113-C, if cs config not complete, IUT send cs_req with wrong parameter,
         * and EBQ will not send cs_rsp, here we set a timeout tick, if long time no cs_rsp, report hci evt*/
        if (pCsParam->cs_req && clock_time_exceed(pc->ll_rsp_timeout_tick, 6000 * 1000)) { // set rsp timeout according to ebq testing
            hci_le_csProcedureEnableComplete_evt(HCI_ERR_CMD_DISALLOWED, pc->acl_conHandle, (u8 *)pCsCfg);
            pCsParam->cs_req        = 0;
            pc->ll_rsp_timeout_tick = 0;
            pCsCfg->cs_procedure_en = 0;
            CS_HCI_LOG("[LL_CS] wait cs rsp timeout!");
        }
    #endif
    #if (LL_CS_CEN_REF_BV_01_C)
        /* EBQ case LL/CS/CEN/REF/BV-01-C: hci event should be report after ll_cs_ind sent */
        if (pc->csReportEvtFlag) {
            pCsCfg = gCsMng.gGlobal_pCsCfg + pCsParam->cs_pend_idx;
            hci_le_csProcedureEnableComplete_evt(BLE_SUCCESS, pc->acl_conHandle, (u8 *)pCsCfg);
            pc->csReportEvtFlag = 0;
            pc->indFlagPending  = 0;
        }
    #endif

        if ((pCsParam->cs_terminate_ind & (PROC_CS_TERMINATE_SEND_REQ | PROC_CS_TERMINATE_SEND_RSP)) || (pCsParam->cs_terminate_ind == PROC_CS_TERMINATE_EVT_PENDING)) {
            pCsCfg = gCsMng.gGlobal_pCsCfg + pCsParam->cs_pend_idx;
            blt_ll_cs_exchangeCsProcedureRepeatTerminateProc(pc, pCsCfg);
        }
        if (csFlowCtrl.csTermiFlag == Remove_CS_Task_with_Terminate) {
            /* cs procedure has been terminated,now clean some relevant flag*/
            pCsCfg                              = gCsMng.gGlobal_pCsCfg + pCsParam->cs_pend_idx;
            pCsCfg->cs_procedure_en             = 0;
            pCsCfg->cs_procedure_measurement_en = 0;
            csFlowCtrl.csTermiFlag              = Wait_Next_CS_Proc_Start;
        }
    }
    if (gCsMng.chn_map_upt_tick && clock_time_exceed(gCsMng.chn_map_upt_tick, 1000 * 1000)) {
        gCsMng.chn_map_upt_tick = 0;
    }

    //    blt_ll_cs_data_loop();
    if (ll_cs_hci_subevent_report_cb == NULL) {
        blt_ll_cs_loop_hci_subevent();
    }
}

int blc_cs_resetByHandle(u16 connHandle)
{
    st_ll_conn_t *pAcl     = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    cs_param_t   *pCsParam = &pAcl->csParam;
    drbg                   = (drbg_param_t *)&pCsParam->drbg_data[0];
    cs_drbg_init(); // drbg reset
    // clean cs paramter
    smemset(pCsParam,0,sizeof(cs_param_t));
    for(int i = 0; i < gCsMng.max_num_cofig; i++){
        cs_config_t *pCfg = gCsMng.gGlobal_pCsCfg + i;
        smemset(pCfg,0,sizeof(cs_config_t));
        pCfg->idx = i;
        for(int j = 0; j <CS_SCH_FIFONUM; j++)
        {
            pCfg->csTskFifo[j].task.scheTask_oft = TSKOFT_CS + i;
            pCfg->csTskFifo[j].task.scheTask_idx = i;
            pCfg->csTskFifo[j].task.scheTask_flg = TSKFLG_CS;
        }
    }
    // clean global paramter gCsMng
    smemset(gCsMng.chn_map,0,10);
    gCsMng.cs_get_rf_cali_flag = 0;
    gCsMng.rpl_factor = 0;
    gCsMng.chn_map_upt_tick = 0;
    // clean csFlowCtrl
    smemset(&csFlowCtrl,0,sizeof(chn_sound_ll_flow_ctrl_t));
    return 0;
}

void blt_le_cs_reset(void)
{
    for (int conn_idx = ACL_CONN_IDX_CEN0; conn_idx < LL_MAX_ACL_CONN_NUM; conn_idx++) {
        st_ll_conn_t *pc = (st_ll_conn_t *)(u32)&blms[conn_idx];
        blc_cs_resetByHandle(pc->acl_conHandle);
    }

    cs_rx_fifo.rptr    = cs_rx_fifo.wptr;
    gCsMng.hciFifoRptr = gCsMng.hciFifoWptr;
    blt_cs_subevent_rf_deinit(0);
    ble_rf_set_tx_modulation_index(RF_MI_P0p50);

    CS_HCI_LOG("reset hci success");
}

_attribute_no_inline_ int blt_cs_mainloop_task(int flag, void *p)
{
    (void)p; //clean warning: unused variable 'p' [-Wunused-variable] by SunWei
    if (flag == (int)FLAG_MODULE_MAINLOOP) {
        blt_ll_cs_mainloop();
    } else if (flag == (int)FLAG_CHECK_INIT) {
    } else if (flag == (int)FLAG_MODULE_RESET) {
        blt_le_cs_reset();
    }
    return 0;
}

/**
 * Initialize Channel Sounding Configuration Parameters
 *
 * @param pParamBuf Pointer to the buffer containing channel sounding configuration parameters.
 * @param cs_config_num Number of channel sounding configurations.
 *
 * @return ble_sts_t Returns BLE_SUCCESS on success, otherwise an error code.
 *
 * This function initializes the channel sounding (CS) configuration parameters and performs the following operations:
 * 1. Ensures parameter length and structure size consistency using static assertions.
 * 2. Checks if structures are 4-byte aligned when BLT_STRUCT_4B_ALIGN_CHECK_EN is enabled.
 * 3. Sets LL feature masks to enable channel sounding functionality.
 * 4. Registers channel sounding control, interrupt, and main loop processing callbacks.
 * 5. Initializes the channel sounding HCI receive FIFO.
 * 6. Sets the maximum number of configurations and calibration values (if enabled).
 * 7. Configures the timer and interrupt priority.
 * 8. Initializes and populates the channel sounding configuration structures.
 */
ble_sts_t blc_ll_initCsConfigParam(u8 *pParamBuf, int cs_config_num)
{
    // Ensure parameter length and cs_config_t structure size consistency
    STATIC_ASSERT_FILE(CS_PARAM_LENGTH == sizeof(cs_config_t), chn_sound);

    // Check if structures are 4-byte aligned when BLT_STRUCT_4B_ALIGN_CHECK_EN is enabled
    #if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(cs_config_t)), chn_sound);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(chn_sound_capabilities_t)), chn_sound);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(cs_param_t)), chn_sound);
    #endif

    // Set LL feature masks to enable channel sounding functionality
    LL_FEATURE_MASK_1 |= LL_FEATURE_MASK_CHANNEL_SOUNDING | LL_FEATURE_MASK_CHANNEL_SOUNDING_HOST;

    // Register channel sounding control, interrupt, and main loop processing callbacks
    ll_chn_sounding_ctrl_handler = blt_ll_cs_ctrl_pdu_proc;
    ll_chn_sounding_irq_task_cb  = blt_cs_interrupt_task;
    ll_chn_sounding_mlp_task_cb  = blt_cs_mainloop_task;
    ll_cs_rawData_process_cb     = blt_ll_cs_data_loop;

    // Initialize the channel sounding HCI receive FIFO
    blc_cs_initCsHciRxFifo(hciCsSubeventRxFifo, CS_SUBEVENT_BUFF_LEN_MAX, HCI_RX_FIFO_NUM);

    // Set the maximum number of configurations and calibration values (if enabled)
    gCsMng.max_num_cofig = cs_config_num;
    #if CAP_CALIB_EN
        #if (MCU_CORE_TYPE == MCU_CORE_TL721X)
            gCsMng.capCalibValue = 0x0c;
        #else
            gCsMng.capCalibValue = 0x20;
        #endif
        gCsMng.capCalibValuePre = 0x80;
    #endif

    // Configure the timer and interrupt priority
    #if (MCU_CORE_TYPE == MCU_CORE_TL721X)|| (MCU_CORE_TYPE == MCU_CORE_TL322X)
        timer_set_irq_mask(FLD_TMR0_MODE_IRQ);
    #endif

    #ifdef MCU_CORE_N22_ENABLE
        clic_set_priority(IRQ_TIMER0, 1);
        clic_interrupt_enable(IRQ_TIMER0);
    #else
        plic_set_priority(IRQ_TIMER0, 1);
        plic_interrupt_enable(IRQ_TIMER0);
    #endif

    timer_set_mode(TIMER0, TIMER_MODE_SYSCLK);

    // Initialize and populate the channel sounding configuration structures
    for (int i = 0; i < gCsMng.max_num_cofig; i++) {
        cs_config_t *pConfig = ((cs_config_t *)pParamBuf) + i;
        pConfig->idx         = i;
        memset(pConfig, 0, sizeof(cs_config_t)); 

        for (int j = 0; j < CS_SCH_FIFONUM; j++) {
            pConfig->csTskFifo[j].task.scheTask_oft = TSKOFT_CS + i;
            pConfig->csTskFifo[j].task.scheTask_idx = i;
            pConfig->csTskFifo[j].task.scheTask_flg = TSKFLG_CS;
        }
    }
    gCsMng.gGlobal_pCsCfg = (cs_config_t *)pParamBuf;

    return BLE_SUCCESS;
}

void blc_loadCsCali_table(u32 flash_addr)
{
    /**    check sum     |   check flag   |  tone cali table    |  fcal cali table   |  1M cali table    |   2M cali table
     *  --------------------------------------------------------------------------------------------------------------------
     *      2 Bytes      |     2Bytes     |  158Bytes(79chn*2)  |    79Bytes(79chn)  |   79Bytes(79chn)  |   79Bytes(79chn)
     *
     *   ### check flag struct ###
     *   tone flag  |  fcal flag  |  1M flag  |  2M flag  |  length
     *   -------------------------------------------------------------------
     *      bit(15) |    bit(14)  |  bit(13)  |   bit(12) | bit(0)~bit(11)
     *  check sum: The sum of all significant data of cali table in byte.
     *  tone flag: Include tone cali value or not in cali table, 1 have, 0 not have.
     *  fcal flag: Include fcal cali value or not in cali table, 1 have, 0 not have.
     *  1M flag:   Include mode1 1M cali value or not in cali table, 1 have, 0 not have.
     *  2M flag:   Include mode1 2M cali value or not in cali table, 1 have, 0 not have.
     *  length:    The length of significant data length in cali table.
     */

    #define CALI_CHAN_NUM 79
    #define CAL_VAL_OFT   4

    u8  cali_data[CAL_VAL_OFT + CALI_CHAN_NUM * 5] = {0};
    u16 check_sum                                  = 0;
    u8  tone_flag                                  = 0;
    u8  fcal_flag                                  = 0;

    cs_flash_cali_table_t *p_cali = (cs_flash_cali_table_t *)cali_data;

    flash_read_page((unsigned long)(flash_addr), (unsigned long)CAL_VAL_OFT, (unsigned char *)p_cali);

    if ((p_cali->cali_tone_flag || p_cali->cali_fcal_flag || p_cali->cali_1M_flag || p_cali->cali_2M_flag) && (p_cali->len <= (CALI_CHAN_NUM * 5)) && (p_cali->len % CALI_CHAN_NUM == 0)) {
        flash_read_page((unsigned long)flash_addr + CAL_VAL_OFT, (unsigned long)(CALI_CHAN_NUM * 5), (unsigned char *)p_cali + CAL_VAL_OFT);
        if (p_cali->cali_tone_flag) {
            for (int i = 0; i < CALI_CHAN_NUM * 2; i++) {
                check_sum += p_cali->cali_table[i];
            }
        }
        if(p_cali->cali_fcal_flag) {
            for (int i = 0; i < CALI_CHAN_NUM; i++) {
                check_sum += p_cali->cali_table[CALI_CHAN_NUM * 2 + i];
            }
        }
        if (p_cali->cali_1M_flag) {
            for (int i = 0; i < CALI_CHAN_NUM; i++) {
                check_sum += p_cali->cali_table[CALI_CHAN_NUM * 3 + i];
            }
        }
        if (p_cali->cali_2M_flag) {
            for (int i = 0; i < CALI_CHAN_NUM; i++) {
                check_sum += p_cali->cali_table[CALI_CHAN_NUM * 4 + i];
            }
        }

        if (check_sum == p_cali->check_sum) {
            if (p_cali->cali_tone_flag) {
                for (int i = 0; i < CALI_CHAN_NUM; i++) {
                    chip_intlDly_calVal[2 * i]     = p_cali->cali_table[2 * i];
                    chip_intlDly_calVal[2 * i + 1] = p_cali->cali_table[2 * i + 1];
                }
                tone_flag = 1;

                tlkapi_send_string_data((stkLog_mask & STK_LOG_LL_CS), "[LL][CS] cali_tone", chip_intlDly_calVal, sizeof(chip_intlDly_calVal));
            }
            if (p_cali->cali_fcal_flag) {
                // todo, fcal cali operate
                for (int i = 0; i < CALI_CHAN_NUM; i++) {
                    fcal_cali_table[i] = p_cali->cali_table[i + CALI_CHAN_NUM * 2];
                }
                fcal_flag = 1;

                tlkapi_send_string_data((stkLog_mask & STK_LOG_LL_CS), "[LL][CS] cali_fcal", fcal_cali_table, sizeof(fcal_cali_table));
            }
            if (p_cali->cali_1M_flag) {
                for (int i = 0; i < CALI_CHAN_NUM; i++) {
                    cs_mode1_phy1M_internalDelay[i] = p_cali->cali_table[i + CALI_CHAN_NUM * 3];
                }

                tlkapi_send_string_data((stkLog_mask & STK_LOG_LL_CS), "[LL][CS] cali_1M", cs_mode1_phy1M_internalDelay, sizeof(cs_mode1_phy1M_internalDelay));
            }
            //for packet 2M cali table
    #if (0)
            if (p_cali->cali_2M_flag) {
                for (int i = 0; i < CALI_CHAN_NUM; i++) {
                    cs_mode1_phy2M_internalDelay[i] = p_cali->cali_table[i + CALI_CHAN_NUM * 4];
                }

                tlkapi_send_string_data((stkLog_mask & STK_LOG_LL_CS), "[LL][CS] cali_2M", cs_mode1_phy2M_internalDelay, sizeof(cs_mode1_phy2M_internalDelay));
            }
    #endif
        }

        if (tone_flag == 0) {
            for (int m = 0; m < CALI_CHAN_NUM; m++) {
                chip_intlDly_calVal[2 * m]     = 63;
                chip_intlDly_calVal[2 * m + 1] = 0;
            }
        }
        if(fcal_flag == 0) {
            // todo set fcal default value
        }
    }
}

    #if 0 //see sizeof(cs_config_t) in warning information
    char checker(int);
    char checkSizeOfInt[sizeof(cs_config_t)]={checker(&checkSizeOfInt)};
    #endif


#endif
