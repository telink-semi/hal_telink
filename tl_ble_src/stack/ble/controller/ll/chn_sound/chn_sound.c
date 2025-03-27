/********************************************************************************************************
 * @file    chn_sound.c
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


#if(LL_FEATURE_ENABLE_CHANNEL_SOUNDING)

#if OS_SUP_EN
#include "stack/ble/os_sup/os_sup.h"
#include "stack/ble/os_sup/os_sup_stack.h"
#endif

#define     DBG_TIM_LOG_EN                  1

#define QIUWEI_STEP_MODE_PROC                       1 //later will delete

#define     MAX_CONSECUTIVE_PROC_SUPPORT    0xFFFF
#define     NUM_ANT_SUPPORT                 0x02
#define     MAX_ANT_PATHS_SUPPORT           0X02
//role _support can set by API init

#define     CS_SCH_SET_EARLY_US             (2000)

/*
 * refer to: RTT_Type_coarse/RTT_Type_32bit_ss/RTT_Type_96bit_ss/RTT_Type_32bit_rs/RTT_Type_64bit_rs/RTT_Type_96bit_rs/RTT_Type_128bit_rs
 */
const u8 RTT_Type_SeqNum[7] = {0,  32, 96, 32, 64, 96, 128}; //bit number
const u8 T_PM_US[3]         = {10, 20, 40}; //unit us


//need to make sure not used in irq.
const u8 T_IP_US[8]     = {10, 20, 30, 40, 50, 60, 80,145}; //unit us
const u8 T_FCS_US[10]    = {15, 20, 30, 40, 50, 60, 80, 100, 120,150}; //unit us
const u8 ACI_to_N_AP[8] = {1, 2, 3, 4, 2, 3, 4, 4};

_attribute_data_retention_ u8 g_T_IP1_us = 145; // if there are more than one config ID, g_T_IP1_us is not right. later process. todo
_attribute_data_retention_ u8 g_T_IP2_us = 145; // if there are more than one config ID, g_T_IP2_us is not right. later process. todo
_attribute_data_retention_ u8 g_T_FCS_us = 150; // if there are more than one config ID, g_T_FCS_us is not right. later process. todo
_attribute_data_retention_ u8 g_t_pm_us = 40;
_attribute_data_retention_ u8 g_T_SW_us = 0;
_attribute_data_retention_ u8 g_antennaPathNum = 1;

cs_mng_t gCsMng;
cs_config_t  *blt_pCsCfg;
rf_packet_cs_t  pkt_CS;


cs_step_IQ_param_t csStepIQ_param;
_attribute_ble_data_retention_ cs_rx_fifo_t cs_rx_fifo;

_attribute_data_retention_ u8 *pCsRxAddr = NULL;
_attribute_data_retention_ u8 cs_subeventResultReportType = CS_SUBEVENT_RESULT_EVENT_FIRST;

u32 cs_tick_tx_on;
u8 cs_rx_agc_gain;

float cs_cfo;
float cs_angleStep = 0;//2 * PI * cs_cfo / SAMPLERATE;
float cs_compArr[LL_CS_STEP_IQ_NUM_MAX] = {0.0};
float cs_if_adjustment79[LL_CS_CHANNEL_NUM_MAX] = {-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500};
//(SIGNAL + NOISE) / NOISE
float cs_thresGood = 17.9;//286.0;//SNR Good threshold
float cs_thresBad = 4.1;//2.25;//SNR Bad threshold

//first for initiator, second for reflector
float cs_adThr[2] = {0.15, 0.8};
float cs_adStep[2] = {-0.01, 0.01};
#if (HADM_PHASE_CONTINUITY)//phase continuity affect distance accuracy of mode 1.need to solve this issue.But now there is not enough time to debug.todo
    _attribute_data_retention_ int cs_internalDelay[2] = {319492, 74155};//20231119
#else
    _attribute_data_retention_ int cs_internalDelay[2] = {319512, 74155};//{318956, 73508};//{325441, 80000};
#endif

//calcPesNadm use nadm adtype = [1,2];//1 based on normalized cross correlation, 2 based on phase minimum square error
int cs_nadm_adtype = 2;//base on phase

cs_config_t *gGlobal_pCsCfg = NULL;
chn_sound_capbilities_t bltCsLocalSupportCap = {

        .Num_Config_Supported                   = 1,//range 1-4
        .max_consecutive_procedures_supported   = MAX_CONSECUTIVE_PROC_SUPPORT,
        .Num_Antennas_Supported                 = NUM_ANT_SUPPORT,
        .Max_Antenna_Paths_Supported            = MAX_ANT_PATHS_SUPPORT,

        .Roles_Supported                        = CS_INIT_REFL_ROLE,
        .Mode_Types                             = 0,  //mandatory mode1 and mode 2
        .RTT_Capability                         = 0,//150ns
        .RTT_AA_Only_N                          = 240,//todo
        .RTT_Sounding_N                         = 0,
        .RTT_Random_Payload_N                   = 0,

        .Optional_NADM_Sounding_Capability = 0,
        .Optional_NADM_Random_Capability =0,
        .Optional_CS_SYNC_PHYs_Supported = 0,   //just mandatory 1M PHY
        .Optional_Subfeatures_Supported = 0,
        .Optional_T_IP1_Times_Supported = 0,    //CS_T_TP_145US is mandatory so shall not be set in optional capbility
        .Optional_T_IP2_Times_Supported = 0,    //CS_T_TP_145US,//145us
        .Optional_T_FCS_Times_Supported = 0,    //CS_T_FCS_150US, //150us
        .Optional_T_PM_Times_Supported  = 0,    //CS_T_PM_40US, //40us M
        .T_SW_Time_Supported = 10, //10us
};





u8 blt_ll_getNewCsConfig(void){

    for(u8 i = 0 ; i<gCsMng.max_num_cofig; i++)
    {
        cs_config_t *pCfg = gGlobal_pCsCfg + i;

        if(pCfg->occupy==0)
        {
//          smemset(pCfg,0,sizeof(cs_config_t));//todo
            return i;
        }
    }
    return 0xff;
}

u8 blt_ll_getCsConfigById(u16 connHandle, u8 config_id)
{
    for(u8 i = 0 ; i<gCsMng.max_num_cofig; i++)
    {
        cs_config_t *pCfg = gGlobal_pCsCfg + i;
        if((pCfg->occupy) && (pCfg->Config_ID==config_id) && (connHandle==pCfg->aclHandle))
        {
            return i;
        }
    }

    return 0xff;
}

u8 blt_ll_getCsConfigByRole(u16 connHandle, cs_config_role_t role)
{
    for(u8 i = 0 ; i<gCsMng.max_num_cofig; i++)
    {
        cs_config_t *pCfg = gGlobal_pCsCfg + i;
        if((pCfg->occupy) && (pCfg->Role==role) && (connHandle==pCfg->aclHandle))
        {
            return 1;
        }
    }

    return 0;
}

u8 blt_ll_getCsConfigByConnHandle(u16 connHandle)
{
    for(int i = 0 ; i<gCsMng.max_num_cofig; i++)
    {
        cs_config_t *pCfg = gGlobal_pCsCfg + i;
        if((pCfg->occupy)  && (connHandle==pCfg->aclHandle))
        {
            return i;
        }
    }

    return 0xff;
}
/*
 * A_ant_num:Number of Device A Antennas
 * B_ant_num:Number of Device B Antennas
 */
u8 blt_ll_calculateCsAci(u8 A_ant_num,u8 B_ant_num){
    u8 aci = 0;
    if((B_ant_num == 1) && (A_ant_num <=4)){
        aci = A_ant_num -1;
    }
    else if((A_ant_num == 1)&&(B_ant_num <=4)){
        aci = B_ant_num + 2;
    }
    else if((A_ant_num == 2) && (B_ant_num == 2)){
        aci = 7;
    }
    return aci;
}

u8 blt_ll_checkCsAci(u8 local_aci,u8 remote_aci){
    u8 ret = 0xff;
    if((remote_aci == 0) ||  (remote_aci == local_aci)){
        return remote_aci;
    }
    else if(remote_aci > local_aci){
        return ret;
    }
    else{
        if(local_aci == 7){
            if(remote_aci < 2){
                return remote_aci;
            }
        }
        else if(local_aci >3) {
            if(remote_aci > 3){
                return remote_aci;
            }
        }
        else if(local_aci >0) {
            if(remote_aci > 0){
                return remote_aci;
            }
        }
    }

    return ret;
}

u8 blt_ll_getNAPByAci(u8 aci){// The number of antenna paths is designated N_AP
    u8 ret = 0;
    if(aci ==7){
        ret = 4;
    }
    else if(aci > 3){
        ret = aci - 2;
    }
    else{
        ret = aci +1;
    }

    return ret;
}



#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_
int blt_ll_acl_post_checkCsTask(st_ll_conn_t *pc)
{
    u32 acl_expect_tick;
    if(pc->aclRole==ACL_ROLE_PERIPHERAL){
        acl_expect_tick = bls_pconn->connExpectTime - blms_pconn->conn_intvl_tick;
    }
    else{
        acl_expect_tick =  pc->ap_tick_mark;
    }

    if(pc->cs_pending){

        u8 cs_cfg_idx = pc->cs_pending & CS_IDX_MSK ;
        cs_config_t *pCsCfg = gGlobal_pCsCfg + cs_cfg_idx ;

        if(pCsCfg->connEventCount == pc->conn_inst_mark){
            pc->cs_pending = 0;
            pCsCfg->csProcCount = 0;
            pCsCfg->seqNum_mark_csSubEvent = 0;
            pCsCfg->inst_start_proc = pc->conn_inst_mark; //must
            pCsCfg->tick_proc_start = acl_expect_tick + pCsCfg->csOft_us * SYSTEM_TIMER_TICK_1US;

        #if(QIUWEI_STEP_MODE_PROC)
            pCsCfg->nonMode0_chnReadIdx = 0;//CHANNEL_MAP_ALL_USED_REFRESH;
            pCsCfg->mode0_chnReadIdx= 0;//CHANNEL_MAP_ALL_USED_REFRESH;
            pCsCfg->slip_stepReadIdx = pCsCfg->slip_stepWriteIdx = 0;
            pCsCfg->cs_procdure_1st_flag = 1;

            if(pCsCfg->Sub_Mode == SUBMODE_TYPE_MODE_UNUSED){
                pCsCfg->mainNum_noSubMode = (pCsCfg->Subevent_Len - pCsCfg->Mode_0_Steps*pCsCfg->mode0Step_durUs)/pCsCfg->mainModeStep_durUs;
            }
        #endif


            pc->csTaskEnableMask |= BIT(cs_cfg_idx);
            blt_sche_addTaskMask(TSKMSK_CS_0<<cs_cfg_idx);
            DBG_CHN6_TOGGLE;DBG_CHN6_TOGGLE;
        }
        else{//todo if past
        }
    }

    for(int i = 0 ; i<gCsMng.max_num_cofig; i++){
        if(pc->csTaskEnableMask &(BIT(i))){
            u16 floor = 0;
            cs_config_t *pCsCfg = gGlobal_pCsCfg + i;
            if((!pCsCfg->occupy)){continue;}

            if(pCsCfg->Procedure_Interval){
                floor = ((u16)(pc->conn_inst_mark - pCsCfg->inst_start_proc)) / pCsCfg->Procedure_Interval;
            }

            if( floor <= 1 ){ //within procedure

                u8 insert_flag = 0;
                if(floor==1 ){ //procedure interval complete.such as procedure interval = 12
                    insert_flag = 1;
                    blt_pCsCfg->proc_end_flag = 0;
                    pCsCfg->seqNum_mark_csSubEvent = 0;
                    pCsCfg->inst_start_proc = pc->conn_inst_mark; //must
                    pCsCfg->csProcCount ++;
                    pCsCfg->tick_proc_start = acl_expect_tick + pCsCfg->csOft_us * SYSTEM_TIMER_TICK_1US;

                    if((pCsCfg->chn_update_pend)){
                        s16 diff  = pc->conn_inst_mark - pCsCfg->chn_update_inst;
                        if(diff>=0){

                            DBG_CS_CHN2_TOGGLE;DBG_CS_CHN2_TOGGLE;
                            pCsCfg->chn_update_pend = 0;
                            pCsCfg->Chn_en_num  = blt_cs_extractEnableChnMap(pCsCfg->Channel_Map, pCsCfg->filteredChnArray);
                        }
                    }

                #if 1///when procedure cnt ++, need to reset drbg
                    drbg_backtracking_resistance(kdrbg_global, vdrbg_global);
                    cs_drbg_init();

                    pCsCfg->nonMode0_chnReadIdx = 0;//CHANNEL_MAP_ALL_USED_REFRESH;
                    pCsCfg->mode0_chnReadIdx= 0;//CHANNEL_MAP_ALL_USED_REFRESH;
                    pCsCfg->slip_stepReadIdx = pCsCfg->slip_stepWriteIdx = 0;
                    pCsCfg->cs_procdure_1st_flag = 1;
                #endif

                    DBG_CS_CHN9_TOGGLE;DBG_CS_CHN9_TOGGLE;
                    #if(SL16_cs_proCnt)
                        log_b16_irq(SL_STACK_CS_TIME_EN, SL16_cs_proCnt, pCsCfg->csProcCount);
                    #endif

                    if((pCsCfg->procMaxCount) && (pCsCfg->csProcCount >= pCsCfg->procMaxCount)){ //remove cs event post
                        //todo
                        insert_flag = 0;
                        pCsCfg->cs_procedure_en = 0;
                        pc->csTaskEnableMask &= ~BIT(i);
                        blt_sche_removeTaskMask(TSKMSK_CS_0<<i);
                        pCsCfg->cs_procedure_measurement_en = 0;
                        tlkapi_send_string_u32s(DBG_CS_LOG_SCH_MASK_EN, "[CS][SCH] remove cs task", pCsCfg->csProcCount, pCsCfg->procMaxCount);
                    }

                }else if((((u16)(pc->conn_inst_mark - pCsCfg->inst_start_proc)) % pCsCfg->Event_Interval) ==0){ //cs event
                    //cs event start, insert subevent, update cs event start point
                    insert_flag = 1;
                }

                if(insert_flag){//ACL followed by channel sounding event.
                    pCsCfg->tick_expect_csSubevent = acl_expect_tick + pCsCfg->csOft_us * SYSTEM_TIMER_TICK_1US;
                    pCsCfg->cs_inst_acl = pc->conn_inst_mark;
                    pCsCfg->cs_sub_event_oft = -1;


                    #if (SLEV_CS_event_insert)
                        log_event_irq(SL_STACK_CS_TIME_EN, SLEV_CS_event_insert);
                    #endif
                    DBG_CS_CHN9_TOGGLE;DBG_CS_CHN9_TOGGLE;

                    /*
                     * The Procedure_Interval field shall be set to indicate the time in units of connection intervals between the
                     * start of consecutive CS procedures. The Procedure_Interval field shall be set to a value from 0 to 65535.
                     * This value shall be set to 0 if the procedure is only to be run once
                     */
                    if( pCsCfg->Procedure_Interval){
                        u16 inst_next_proc = pCsCfg->inst_start_proc + pCsCfg->Procedure_Interval;
                        if((u16)(inst_next_proc - pCsCfg->cs_inst_acl) <=  pCsCfg->Event_Interval){
                            pCsCfg->flag_endEvtInProc = 1;
                        }
                        else{
                            pCsCfg->flag_endEvtInProc = 0;
                        }
                    }
                    else{
//                      u32 pCsCfg->tick_proc_start + pCsCfg->Max_Procedure_Len*SYSTEM_TIMER_TICK_625US;
                    }

                    tlkapi_send_string_u32s(DBG_CS_LOG_SCH_MASK_EN, "[CS][SCH] CS event", pCsCfg->csProcCount,  pc->conn_inst_mark, pCsCfg->flag_endEvtInProc);

                    u32 sSlot_task_start = TICKS_ABS_2_SSLOT_ABS(pCsCfg->tick_expect_csSubevent - CS_SCH_SET_EARLY_US*SYSTEM_TIMER_TICK_1US);
                    //mark last subevent to build timelines when rebuilt
                    pCsCfg->sSlot_mark_csSubevent = sSlot_task_start - pCsCfg->sSlot_csSubIntvl;
                    pCsCfg->bSlot_mark_csSubevent = SSLOT_ABS_2_BSLOT_ABS(pCsCfg->sSlot_mark_csSubevent); //There is a probability of risk

                    blt_sche_addUpdate(SLOT_UPDT_CHANNEL_SOUNDING_STATE_CHANGE);
                }

            }
            else{ //>1  out procedure
                //update cs event & cs procedure start point
                u16 fmod = ((u16)(pc->conn_inst_mark - pCsCfg->inst_start_proc)) % pCsCfg->Procedure_Interval;
                pCsCfg->inst_start_proc = pc->conn_inst_mark -  fmod;
                pCsCfg->csProcCount += floor;

            #if(SL16_cs_proCnt)
                log_b16_irq(SL_STACK_CS_TIME_EN, SL16_cs_proCnt, pCsCfg->csProcCount);
            #endif


                if((pCsCfg->procMaxCount)&&(pCsCfg->csProcCount > pCsCfg->procMaxCount)){
                    //todo
                    pc->csTaskEnableMask &= ~BIT(i);
                    blt_sche_removeTaskMask(TSKMSK_CS_0<<i);
                }

                u16 diff_eventcnt = ((u16)(pc->conn_inst_mark - pCsCfg->inst_start_proc))/pCsCfg->Event_Interval;
                pCsCfg->seqNum_mark_csSubEvent = diff_eventcnt * pCsCfg->Subevents_Per_Event; // next +1
                tlkapi_send_string_u32s(DBG_CS_LOG_SCH_MASK_EN, "proc jump", pCsCfg->csProcCount, pCsCfg->seqNum_mark_csSubEvent, pc->conn_inst_mark,pCsCfg->inst_start_proc);


                u32 csOfset_us = (pCsCfg->csOft_us);
                u32 sSlot_mark = TICKS_ABS_2_SSLOT_ABS(acl_expect_tick + csOfset_us *SYSTEM_TIMER_TICK_1US );
                pCsCfg->bSlot_start_proc = SSLOT_ABS_2_BSLOT_ABS(sSlot_mark) - pc->bSlot_interval * fmod;// todo

            }
        }
    }
    return 0;
}

#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_
int blt_ll_insertCsSchedulerLinklist(cs_config_t *pCsCfg){

    s32 sSlot_start_cs;
    int new_task_cnt = 0;
    int int_jump_num;
    u32 csevent_start=0;
    u8  csEvtOft = 0;

    pCsCfg->csTsk_wptr = pCsCfg->csTsk_rptr = 0;
    if(bltSche.sSlot_idx_reset == 1 && (bltSche.build_index == 0)){
        pCsCfg->sSlot_mark_csSubevent -= bltSche.sSlot_idx_past;
    }
    if(pCsCfg->cs_sub_event_oft >= (pCsCfg->Subevents_Per_Event-1)){
        return 0;
    }

    if( pCsCfg->sSlot_mark_csSubevent >= bltSche.sSlot_idx_next){//may be happen in the first CS create
        int_jump_num = 0;
        sSlot_start_cs = pCsCfg->sSlot_mark_csSubevent + pCsCfg->sSlot_csSubIntvl;
    }
    else{

        /*
         * todo fanqh : the first subevent task locate here unexpected and jump = 1 when -3 replace with -1
         */
        int_jump_num = (bltSche.sSlot_idx_next - 5 - pCsCfg->sSlot_mark_csSubevent)/pCsCfg->sSlot_csSubIntvl;
        sSlot_start_cs = pCsCfg->sSlot_mark_csSubevent + (int_jump_num + 1)*pCsCfg->sSlot_csSubIntvl;
    }

    csEvtOft = pCsCfg->cs_sub_event_oft + int_jump_num + 1;
    csevent_start = pCsCfg->seqNum_mark_csSubEvent + int_jump_num;
    if(csEvtOft >= pCsCfg->Subevents_Per_Event){
        return 0;
    }
    tlkapi_send_string_u32s(DBG_CS_LOG_SCH_MASK_EN, "[CS][SCH] tsk jump",  int_jump_num, bltSche.sSlot_idx_next, pCsCfg->sSlot_mark_csSubevent);




    for(int j=0; j<CS_SCH_FIFONUM; j++){

        sch_task_t  *pCur_schTask = (sch_task_t *)&pCsCfg->csTskFifo[j];
        //todo Subevent_Interval should add software consumption
        pCur_schTask->begin = sSlot_start_cs + j*pCsCfg->sSlot_csSubIntvl;
        pCur_schTask->end = pCur_schTask->begin + pCsCfg->sSlotCsDuration - 1;
        pCur_schTask->cs_subevent_seqNum  = csevent_start + j + 1;
        pCur_schTask->cs_procCnt = pCsCfg->csProcCount;
        pCur_schTask->cs_oft = csEvtOft + j;



        /*
         * The number of CS subevents executed is equal to N_MAX_SUBEVENTS_PER_PROCEDURE
         * A CS procedure is considered complete and closed
         */
        if(pCur_schTask->cs_subevent_seqNum > 32){
            DBG_CS_CHN6_TOGGLE;DBG_CS_CHN6_TOGGLE;
            break;
        }


        u32 proc_extend_end = pCsCfg->tick_proc_start + pCsCfg->Max_Procedure_Len *SYSTEM_TIMER_TICK_625US;
        u32 tick_subevent_end = SSLOT_ABS_2_TICKS_ABS(pCur_schTask->end);

        if(tick1_exceed_tick2(tick_subevent_end, proc_extend_end)){
            break;
        }
        /*
         * The Subevent_Len field shall be set to indicate the maximum duration of each CS subevent in
         * microseconds and shall be greater than or equal to 1250 microseconds and less than 4 seconds
         */
//      if(pCur_schTask->end > (pCsCfg->bSlot_start_proc + pCsCfg->procMaxCount)){ //todo
//          break;
//      }

        tlkapi_send_string_u32s(DBG_CS_LOG_SCH_MASK_EN&0, "sch", pCur_schTask->cs_procCnt, csevent_start,pCur_schTask->cs_subevent_seqNum,0);
//      tlkapi_send_string_u32s(DBG_CS_LOGIC_EN&0, "poll sch", j, pCur_schTask, pCur_schTask->begin, pCur_schTask->end);

        if(pCur_schTask->cs_oft >= pCsCfg->Subevents_Per_Event){
            break;
        }

        if( pCur_schTask->begin >=  bltSche.sSlot_endIdx_dft){  //new task beyond correct range, finish
            break;
        }
        else if(pCur_schTask->end < bltSche.sSlot_endIdx_dft){ //new task in correct range
            pCsCfg->csTsk_wptr = j;
            new_task_cnt ++;
        }
        else{ //new task across "sSlot_endIdx_dft"
            u32 cur_task_offset = TSKMSK_CS_0 + pCsCfg->idx;
            //for those task across end_idx, find the task with highest priority, to guarantee that task not missed
            if(bltPri.pri_cal[cur_task_offset] > bltPri.priMax_value){
                bltPri.priMax_value = bltPri.pri_cal[cur_task_offset];
                bltPri.priMax_index = cur_task_offset;
                bltSche.sSlot_endIdx_maxPri = pCur_schTask->begin;
            }

            break;
        }
    }


    if(new_task_cnt){
        int ret = blt_ll_addTask2ExistLinklist(&pCsCfg->csTskFifo[0],pCsCfg->csTsk_wptr + 1);
        tlkapi_send_string_u8s(0, "cs insert task", new_task_cnt, ret, pCsCfg->csTsk_wptr + 1);
    }

    return 0;
}

#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_
int blt_ll_rebuildCsSchedulerLinklist(void){
    int i = 0;

    cs_config_t *pCsCfg = NULL;
    for(i=0; i<gCsMng.max_num_cofig; i++)
    {
        if( bltSche.task_mask & (TSKMSK_CS_0<<i ) )
        {
            tlkapi_send_string_u8s(DBG_CS_LOG_SCH_MASK_EN&0, "rebuildCsSch", i);
            pCsCfg = (cs_config_t *)(gGlobal_pCsCfg + i);
            blt_ll_insertCsSchedulerLinklist(pCsCfg);
        }
    }

    return 0;
}



#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_
int blt_cs_step_rx(void){

    if(blt_pCsCfg->Role == CHANNEL_SOUNDING_ROLE_INITIATOR){
        ll_cs_initiator_irq_task_cb(FLAG_CS_STEP_RX); //blt_cs_initiator_irq_task  blt_cs_step_rx
    }
    else if(blt_pCsCfg->Role == CHANNEL_SOUNDING_ROLE_REFLECTOR){

    }
 return 0;
}



#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_
int blt_cs_subevent_post(unsigned char phase_en){
    blms_state = BLMS_STATE_CS_SUBEVENT_E;

    blt_cs_subevent_rf_deinit(phase_en);

    #if (HADM_PHASE_CONTINUITY)
        if(!phase_en){
            blt_pCsCfg->phaseContin_config_flag = 0;
        }
    #endif

    blt_ll_calculate_sSlot_next(clock_time() + SLOT_PROCESS_MAX_TICK);

#if (SL01_cs_subevent_0)
    log_task_end_irq(SL_STACK_CS_TIME_EN, SL01_cs_subevent_0);
#endif
    DBG_CS_CHN2_LOW;

    return 0;
}


_attribute_ram_code_ void blt_cs_subevent_rf_init(void)
{
    //CS Subevent start
    DBG_CHN5_HIGH;//test configuration consumption time start
    ble_rf_channel_sounding_init();
    ble_rf_tx_channel_sounding_mode_en();
    ble_rf_rx_channel_sounding_mode_en(1, IQ_20_BIT_MODE);//sample rate 4MHz, IQ 20 bit
    //set RF Power in subevent_start
    rf_set_power_level_index(RF_POWER_INDEX_P4p61dBm);//20231116, RF_POWER_INDEX_P9p90dBm ---> RF_POWER_INDEX_P4p61dBm, later it is necessary to distinguish VOLTAGE_1V8 or VOLTAGE_3V3
    ble_rf_set_power_level_singletone(RF_POWER_P4p61dBm);//20231116, RF_POWER_P9p90dBm ---> RF_POWER_P4p61dBm, later it is necessary to distinguish the VOLTAGE_1V8 or VOLTAGE_3V3

    /* Different process for different MCU: ******************************************/
    ble_rf_set_tx_dma(0, 17);
     //  This register exists in Kite/Vulture/826x, riscv architecture chips such as
     //  Eagle do not have this register, but DMA fifo can be turned off by forcibly
     //  setting DMA TX rptr = DMA TX wptr to use DMA default tx fifo.If the hardware
     //  TX rptr of DMA TX fifo == hardware TX wptr, then send DMA Tx default fifo,
     //  otherwise send DMA Tx fifo non-default area.
    HAL_REG_RF_DMA_FIFO_TX_RPTR = FLD_DMA_RPTR_CLR;
    /**********************************************************************************/
    DBG_CHN5_LOW;//test configuration consumption time post
}

#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_ void blt_cs_subevent_rf_deinit(unsigned char phase_en)
{
    //CS Subevent stop
    DBG_CHN5_HIGH;//test configuration consumption time start
    #if (HADM_PHASE_CONTINUITY)
        if(cs_phase_continuity_flag){
            ble_rf_cs_phase_continuity_dis(phase_en);
        }
    #endif
    ble_rf_tx_channel_sounding_mode_dis();
    ble_rf_rx_channel_sounding_mode_dis();
    ble_rf_channel_sounding_deinit();
    DBG_CHN5_LOW;//test configuration consumption time post
}


#if (QIUWEI_STEP_MODE_PROC)
//_attribute_ble_data_retention_ u8   cs_chnMap_keepBuf[72];

static _always_inline void blt_cs_mode0_chn_init(cs_config_t * pCsCfg)
{

    ///calculate mode-0 channel map and keep them in relevant chnMapIdx buffer.
    chn_sel_3a(pCsCfg->Chn_en_num, pCsCfg->filteredChnArray, pCsCfg->mode0ShuffledChnArray);
    pCsCfg->csChnAvailNum = pCsCfg->Chn_en_num*(pCsCfg->Channel_Map_Repetition);
}


_attribute_ram_code_ void blt_cs_select_mode0ChnIdx(cs_config_t *pCsCfg)
{
    u8 calChnNum = 0;
    (void)calChnNum;//clean warning: unused variable 'calChnNum' [-Wunused-variable] by SunWei
    if(pCsCfg->mode0_chnReadIdx >= pCsCfg->Chn_en_num){
        chn_sel_3a(pCsCfg->Chn_en_num, pCsCfg->filteredChnArray, pCsCfg->mode0ShuffledChnArray);
        pCsCfg->mode0_chnReadIdx = 0; //restart
    }

    u8 tChnIdx = pCsCfg->mode0_chnReadIdx%pCsCfg->Chn_en_num;
    pCsCfg->slip_window_step[pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_chnIdx = pCsCfg->mode0ShuffledChnArray[tChnIdx];
    pCsCfg->mode0_chnReadIdx++;
}


_attribute_ram_code_ void blt_cs_nonModeChn_cal(cs_config_t *pCsCfg){

    chn_sel_3b(pCsCfg->Chn_en_num, pCsCfg->filteredChnArray, pCsCfg->nonmode0ShuffledChnArray);
    pCsCfg->nonMode0_chnReadIdx = 0; //restart
}

_attribute_ram_code_ void blt_cs_select_nonMode0ChnIdx(cs_config_t *pCsCfg)
{
    if( pCsCfg->nonMode0_chnReadIdx >= pCsCfg->Chn_en_num){
        blt_cs_nonModeChn_cal(pCsCfg);
    }

    u8 tChnIdx = pCsCfg->nonMode0_chnReadIdx % pCsCfg->Chn_en_num;

    pCsCfg->slip_window_step[pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_chnIdx = pCsCfg->nonmode0ShuffledChnArray[tChnIdx];

    pCsCfg->nonMode0_chnReadIdx++;
}

/*
 * cs_ss_marker_sig_sel(u8* sig_initiator, u8* sig_reflector);
 * cs_ss_marker_position(u8 seqbit_len, u8* pos_initiator, u8* pos_reflector);
 * cs_random_seq(u8* seq, u8 seqbit_len)
 * cs_tpm_ext(u8* tpm_ext);
 */
_attribute_ram_code_ void blt_cs_stepDRBG_proc(u8 step_mode_type){

    if(step_mode_type == STEP_MODE_0){
        tlkapi_send_string_data(0, "DRBG callback mode error", 0, 0);
        return ;
    }

    u8 tmpSlipWriteIdx = blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK;
    slip_window_step_t* pCurSlipWindowStep = (slip_window_step_t*)&blt_pCsCfg->slip_window_step[tmpSlipWriteIdx];

    switch(step_mode_type){
        case STEP_MODE_1: //RTT
        {
            if(blt_pCsCfg->RTT_Type == RTT_Type_32bit_ss){
                cs_ss_marker_sig_sel(pCurSlipWindowStep->step_initRttSeq, pCurSlipWindowStep->step_reflRttSeq);

                cs_ss_marker_position(32, pCurSlipWindowStep->step_initRttSSPos, pCurSlipWindowStep->step_reflRttSSPos);
            }
            else if(blt_pCsCfg->RTT_Type == RTT_Type_96bit_ss){
                cs_ss_marker_sig_sel(pCurSlipWindowStep->step_initRttSeq, pCurSlipWindowStep->step_reflRttSeq);

                cs_ss_marker_position(32, pCurSlipWindowStep->step_initRttSSPos, pCurSlipWindowStep->step_reflRttSSPos);
            }
            else if(blt_pCsCfg->RTT_Type == RTT_Type_32bit_rs || blt_pCsCfg->RTT_Type == RTT_Type_64bit_rs ||\
                    blt_pCsCfg->RTT_Type == RTT_Type_96bit_rs || blt_pCsCfg->RTT_Type == RTT_Type_128bit_rs)
            {
                u8 seqbit_len = 32*(blt_pCsCfg->RTT_Type - 2);

                cs_random_seq(pCurSlipWindowStep->step_initRttSeq, seqbit_len);//here is not sure. initiator first.
                cs_random_seq(pCurSlipWindowStep->step_reflRttSeq, seqbit_len);//here is not sure. then reflector.
            }
        }
        break;

        case STEP_MODE_2:
        {
            //cs_antenna_path_perm(); //todo
            cs_tpm_ext((u8*)&pCurSlipWindowStep->extSlot.step_extSlotFlag);
        }
        break;

        case STEP_MODE_3:
        {
            if(blt_pCsCfg->RTT_Type == RTT_Type_32bit_ss){
                cs_ss_marker_sig_sel(pCurSlipWindowStep->step_initRttSeq, pCurSlipWindowStep->step_reflRttSeq);

                cs_ss_marker_position(32, pCurSlipWindowStep->step_initRttSSPos,pCurSlipWindowStep->step_reflRttSSPos);
            }
            else if(blt_pCsCfg->RTT_Type == RTT_Type_96bit_ss){
                cs_ss_marker_sig_sel(pCurSlipWindowStep->step_initRttSeq, pCurSlipWindowStep->step_reflRttSeq);

                cs_ss_marker_position(32, pCurSlipWindowStep->step_initRttSSPos, pCurSlipWindowStep->step_reflRttSSPos);
            }
            else if(blt_pCsCfg->RTT_Type == RTT_Type_32bit_rs || blt_pCsCfg->RTT_Type == RTT_Type_64bit_rs ||\
                    blt_pCsCfg->RTT_Type == RTT_Type_96bit_rs || blt_pCsCfg->RTT_Type == RTT_Type_128bit_rs)
            {
                u8 seqbit_len = 32*(blt_pCsCfg->RTT_Type - 2);

                cs_random_seq(pCurSlipWindowStep->step_initRttSeq, seqbit_len);//here is not sure
                cs_random_seq(pCurSlipWindowStep->step_reflRttSeq, seqbit_len);//here is not sure
            }

            //cs_antenna_path_perm(); //todo

            ///calculate extension tone flag
            cs_tpm_ext((u8*)&pCurSlipWindowStep->extSlot.step_extSlotFlag);
        }
        break;

        default:
            //can not run here.
            tlkapi_send_string_data(0, "DRBG callback process error", 0, 0);
        break;
    }
}

#if (NOW_NOT_IMPLEMENT_FUNCTION)
static _always_inline u16 blt_cs_slipWindowstepDurCal(u8 slipWind_idx){

    u16 stepDurUs = 0;

    slipWind_idx = blt_pCsCfg->slip_stepReadIdx + slipWind_idx;

    switch (blt_pCsCfg->slip_window_step[slipWind_idx&SLIP_WINDOW_STEP_MSK].step_modeType){
        case STEP_MODE_0:
        {
            stepDurUs = blt_pCsCfg->mode0Step_durUs;
        }
            break;
        case STEP_MODE_1:
        {
            stepDurUs = blt_pCsCfg->mode1Step_durUs;
        }
            break;
        case STEP_MODE_2:
        {
            stepDurUs = blt_pCsCfg->mode2Step_durUs;
        }
            break;
        case STEP_MODE_3:
        {
            stepDurUs = blt_pCsCfg->mode3Step_durUs;
        }
            break;
        default:
            break;
    }

    return stepDurUs;
}


/**
 * please note: slip_window_step[] order is:
 *  mode-0...->repetition...->last main_mode...->last sub_mode->...
 */
static inline void blt_cs_curSub_notLeave_proc(u8 curSubevtArngMainModeNum, u8 subevtJumpNum)
{
    /**
     * here situation: before must run one subevent at least.
     */

    /*
     * please note: slip_window_step[] order is: mode-0...->repetition...->last main_mode...->last sub_mode->...
     */
    if(subevtJumpNum == 0){
        tlkapi_send_string_data(1, "can not run here, error 1", 0, 0); //just for debug
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    //step 1: arrange current subevent's main mode steps.
    for(int i=0; i<curSubevtArngMainModeNum; i++){
        blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_modeType = blt_pCsCfg->Main_Mode;
        //blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_chnIdx   = ;//todo
        blt_cs_stepDRBG_proc(blt_pCsCfg->Main_Mode);
        blt_pCsCfg->slip_stepWriteIdx++;
        cs_step_add();
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    //step 2: arrange current subevent's sub_mode step.
    blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_modeType = blt_pCsCfg->Sub_Mode;
    //blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_chnIdx   = ;//todo
    blt_cs_stepDRBG_proc(blt_pCsCfg->Sub_Mode);
    blt_pCsCfg->slip_stepWriteIdx++;
    cs_step_add();

    //////////////////////////////////////////////////////////////////////////////////////////////
    //blt_pCsCfg->slip_stepReadIdx will always point to the step to run immediately.
    if(subevtJumpNum > 1){
        blt_pCsCfg->slip_stepReadIdx = blt_pCsCfg->slip_stepWriteIdx;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    //step 3: arrange next subevent's mode-0.
    for(int i=0; i<blt_pCsCfg->Mode_0_Steps; i++){
        blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_modeType = STEP_MODE_0;
        //blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_chnIdx   = ;//todo

        if(subevtJumpNum == 1){
            cs_access_addr((u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_reflAA, (u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_initAA);
        }

        blt_pCsCfg->slip_stepWriteIdx++;
        cs_step_add();
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    //step 4: arrange current subevent's repetition step to next subevent.
    for(int i=0; i<blt_pCsCfg->Main_Mode_Repetition; i++){
        blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_modeType = blt_pCsCfg->Main_Mode;
        //blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_chnIdx   = ;//todo
        blt_cs_stepDRBG_proc(blt_pCsCfg->Main_Mode);

        if(subevtJumpNum == 1){
            cs_access_addr((u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_reflAA, (u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_initAA);
        }

        blt_pCsCfg->slip_stepWriteIdx++;
        cs_step_add();
    }


    //////////////////////////////////////////////////////////////////////////////////////////////
    blt_pCsCfg->slip_stepWriteIdx ++;
}



/**
 * please note: slip_window_step[] order is:
 *  mode-0...->repetition...->last main_mode...->last sub_mode->...
 */
static inline void blt_cs_curSub_leaveMode_proc(u8 curSubevtArngMainModeNum, u8 leaveMainNum, u8 subevtJumpNum)
{
    /**
     * here situation: before must run one subevent at least.
     */
    /*
     * please note: slip_window_step[] order is: mode-0...->repetition...->last main_mode...->last sub_mode->...
     */
    if(subevtJumpNum == 0){
        tlkapi_send_string_data(1, "can not run here, error 3", 0, 0); //just for debug
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    //step 1: arrange current subevent's main mode steps.
    for(int i=0; i < curSubevtArngMainModeNum; i++){
        blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_modeType = blt_pCsCfg->Main_Mode;
        //blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_chnIdx   = ;//todo
        blt_cs_stepDRBG_proc(blt_pCsCfg->Main_Mode);
        blt_pCsCfg->slip_stepWriteIdx++;
        cs_step_add();
    }

    //blt_pCsCfg->slip_stepReadIdx will always point to the step to run immediately.
    if(subevtJumpNum > 1){
        blt_pCsCfg->slip_stepReadIdx = blt_pCsCfg->slip_stepWriteIdx;
    }
    //////////////////////////////////////////////////////////////////////////////////////////////
    //step 2: arrange next subevent's mode-0 step.
    for(int i=0; i<blt_pCsCfg->Mode_0_Steps; i++){
        blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_modeType = STEP_MODE_0;
        //blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_chnIdx   = ;//todo

        if(subevtJumpNum == 1){
            cs_access_addr((u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_reflAA, (u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_initAA);
        }

        blt_pCsCfg->slip_stepWriteIdx++;
        cs_step_add();
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    //step 3: arrange current subevent's repetition step to next subevent.
    for(int i=0; i<blt_pCsCfg->Main_Mode_Repetition; i++){
        blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_modeType = blt_pCsCfg->Main_Mode;
        //blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_chnIdx   = ;//todo
        blt_cs_stepDRBG_proc(blt_pCsCfg->Main_Mode);

        if(subevtJumpNum == 1){
            cs_access_addr((u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_reflAA, (u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_initAA);
        }

        blt_pCsCfg->slip_stepWriteIdx++;
        cs_step_add();
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    //step 4: arrange current subevent's main_mode step to next subevent.
    for(int i=0; i<leaveMainNum; i++){
        blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_modeType = blt_pCsCfg->Main_Mode;
        //blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_chnIdx   = ;//todo
        blt_cs_stepDRBG_proc(blt_pCsCfg->Main_Mode);

        if(subevtJumpNum == 1){
            cs_access_addr((u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_reflAA, (u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_initAA);
        }

        blt_pCsCfg->slip_stepWriteIdx++;
        cs_step_add();
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    //step 5: arrange current subevent's sub_mode step to next subevent.
    blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_modeType = blt_pCsCfg->Sub_Mode;
    //blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_chnIdx   = ;//todo
    blt_cs_stepDRBG_proc(blt_pCsCfg->Sub_Mode);

    if(subevtJumpNum == 1){
        cs_access_addr((u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_reflAA, (u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_initAA);
    }

    cs_step_add();
    //////////////////////////////////////////////////////////////////////////////////////////////


    //////////////////////////////////////////////////////////////////////////////////////////////
    blt_pCsCfg->slip_stepWriteIdx++;
}


static inline void blt_cs_1stStepProcedure_curSub_notLeave_proc(u8 curSubevtArngMainModeNum, u8 subevtJumpNum)
{
    /**
     * here situation: before not run any step of this CS procedure. this is the first running.
     */
    /*
     * please note: slip_window_step[] order is: mode-0...->main_mode...->sub_mode->...->next subevent mode-0...->last repetition...
     */
    if(subevtJumpNum == 0){
        tlkapi_send_string_data(1, "can not run here, error 4", 0, 0); //just for debug
    }
    //////////////////////////////////////////////////////////////////////////////////////////////
    blt_pCsCfg->slip_stepReadIdx = blt_pCsCfg->slip_stepWriteIdx;

    //step 1: arrange current subevent's mode-0 step.
    for(int i=0; i<blt_pCsCfg->Mode_0_Steps; i++){
        blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_modeType = STEP_MODE_0;
        //blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_chnIdx   = ;//todo
        blt_pCsCfg->slip_stepWriteIdx++;
        cs_step_add();
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    //step 2: arrange current subevent's main_mode steps.
    for(int i=0; i < curSubevtArngMainModeNum; i++){
        blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_modeType = blt_pCsCfg->Main_Mode;
        //blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_chnIdx   = ;//todo
        blt_cs_stepDRBG_proc(blt_pCsCfg->Main_Mode);
        blt_pCsCfg->slip_stepWriteIdx++;
        cs_step_add();
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    //step 3: arrange current subevent's sub_mode step.
    blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_modeType = blt_pCsCfg->Sub_Mode;
    //blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_chnIdx   = ;//todo
    blt_cs_stepDRBG_proc(blt_pCsCfg->Sub_Mode);
    cs_step_add();

    //////////////////////////////////////////////////////////////////////////////////////////////
    //blt_pCsCfg->slip_stepReadIdx will always point to the step to run immediately.
    if(subevtJumpNum > 1){
        blt_pCsCfg->slip_stepReadIdx = blt_pCsCfg->slip_stepWriteIdx;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    //step 4: arrange next subevent's mode-0.
    for(int i=0; i<blt_pCsCfg->Mode_0_Steps; i++){
        blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_modeType = STEP_MODE_0;
        //blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_chnIdx   = ;//todo

        if(subevtJumpNum == 1){
            cs_access_addr((u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_reflAA, (u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_initAA);
        }

        blt_pCsCfg->slip_stepWriteIdx++;
        cs_step_add();
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    //step 5: arrange current subevent's repetition step to next subevent.
    for(int i=0; i<blt_pCsCfg->Main_Mode_Repetition; i++){
        blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_modeType = blt_pCsCfg->Main_Mode;
        //blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_chnIdx   = ;//todo

        blt_cs_stepDRBG_proc(blt_pCsCfg->Main_Mode);

        if(subevtJumpNum == 1){
            cs_access_addr((u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_reflAA, (u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_initAA);
        }

        blt_pCsCfg->slip_stepWriteIdx++;
        cs_step_add();
    }


    //////////////////////////////////////////////////////////////////////////////////////////////
    blt_pCsCfg->slip_stepWriteIdx++;
}

static inline void blt_cs_1stStepProcdure_curSub_LeaveMode_proc(u8 curSubevtArngMainModeNum, u8 leaveMainNum, u8 subevtJumpNum)
{
    /**
     * here situation: before not run any step of this CS procedure. this is the first running.
     */
    /*
     * please note: slip_window_step[] order is: mode-0...->main_mode...->next subevent mode-0...-> repetition...-> last main mode...->sub_mode->...
     */
    if(subevtJumpNum == 0){
        tlkapi_send_string_data(1, "can not run here, error 5", 0, 0); //just for debug
    }
    //////////////////////////////////////////////////////////////////////////////////////////////
    blt_pCsCfg->slip_stepReadIdx = blt_pCsCfg->slip_stepWriteIdx;

    //step 1: arrange current subevent's mode-0 step.
    for(int i=0; i<blt_pCsCfg->Mode_0_Steps; i++){
        blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_modeType = STEP_MODE_0;
        //blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_chnIdx   = ;//todo
        blt_pCsCfg->slip_stepWriteIdx++;
        cs_step_add();
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    //step 2: arrange current subevent's main_mode steps.
    for(int i=0; i < curSubevtArngMainModeNum; i++){
        blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_modeType = blt_pCsCfg->Main_Mode;
        //blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_chnIdx   = ;//todo
        blt_cs_stepDRBG_proc(blt_pCsCfg->Main_Mode);
        blt_pCsCfg->slip_stepWriteIdx++;
        cs_step_add();
    }

    //blt_pCsCfg->slip_stepReadIdx will always point to the step to run immediately.
    if(subevtJumpNum > 1){
        blt_pCsCfg->slip_stepReadIdx = blt_pCsCfg->slip_stepWriteIdx;
    }


    //////////////////////////////////////////////////////////////////////////////////////////////
    //step 3: arrange next subevent's mode-0 step.
    for(int i=0; i<blt_pCsCfg->Mode_0_Steps; i++){
        blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_modeType = STEP_MODE_0;
        //blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_chnIdx   = ;//todo

        if(subevtJumpNum == 1){
            cs_access_addr((u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_reflAA, (u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_initAA);
        }

        blt_pCsCfg->slip_stepWriteIdx++;
        cs_step_add();
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    //step 4: arrange current subevent's repetition step to next subevent.
    for(int i=0; i<blt_pCsCfg->Main_Mode_Repetition; i++){
        blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_modeType = blt_pCsCfg->Main_Mode;
        //blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_chnIdx   = ;//todo
        blt_cs_stepDRBG_proc(blt_pCsCfg->Main_Mode);

        if(subevtJumpNum == 1){
            cs_access_addr((u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_reflAA, (u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_initAA);
        }

        blt_pCsCfg->slip_stepWriteIdx++;
        cs_step_add();
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    //step 5: arrange current subevent's main_mode step to next subevent.
    for(int i=0; i< leaveMainNum; i++){
        blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_modeType = blt_pCsCfg->Main_Mode;
        //blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_chnIdx   = ;//todo
        blt_cs_stepDRBG_proc(blt_pCsCfg->Main_Mode);

        if(subevtJumpNum == 1){
            cs_access_addr((u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_reflAA, (u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_initAA);
        }

        blt_pCsCfg->slip_stepWriteIdx++;
        cs_step_add();
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    //step 6: arrange current subevent's sub_mode step to next subevent.
    blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_modeType = blt_pCsCfg->Sub_Mode;
    //blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_chnIdx   = ;//todo
    blt_cs_stepDRBG_proc(blt_pCsCfg->Sub_Mode);

    if(subevtJumpNum == 1){
        cs_access_addr((u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_reflAA, (u8*)&blt_pCsCfg->slip_window_step[blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK].step_initAA);
    }

    cs_step_add();
    //////////////////////////////////////////////////////////////////////////////////////////////


    //////////////////////////////////////////////////////////////////////////////////////////////
    blt_pCsCfg->slip_stepWriteIdx++;
}
#endif

_attribute_ram_code_ void blt_cs_arrange_mainMode_proc(void)
{
    blt_cs_stepDRBG_proc(blt_pCsCfg->Main_Mode);
    cs_step_add();
}

_attribute_ram_code_
void blt_cs_subevent_stepConfig_proc(sch_task_t *pCsSch)
{
    if(blt_pCsCfg->cs_procdure_1st_flag){
        //72 channel cost 160us in 96M
        //mode0 must exist. other mode just may exist. so here only calculate mode0 map
        blt_cs_mode0_chn_init(blt_pCsCfg); //calculate mode-0 and non-mode-0 channel map
    }

    //the sequence is: subevent start(i.e. first step) --> step2--> step3--> step4......--> subevent post
    //must leave sufficient time to handle some DRBG calculation before (first step).

    u16 tJumpStepNum = 0;
    (void)tJumpStepNum;//clean warning: unused variable 'tJumpStepNum' [-Wunused-variable] by SunWei
    u32 tSubevtJumpNum = (u32)(pCsSch->cs_subevent_seqNum - blt_pCsCfg->seqNum_mark_csSubEvent);//note here whether to -1, need to according to actual situation.

    if(tSubevtJumpNum == 0){
        tSubevtJumpNum = 1;
    }

    if(blt_pCsCfg->Sub_Mode != SUBMODE_TYPE_MODE_UNUSED){
    #if (NOW_NOT_IMPLEMENT_FUNCTION)
        u16 currSubevtStepNum = 0;
        u32 jumpStepSum = 0;

        do{ //for(int i=0; i < tSubevtJumpNum; i++)
            ///DRBG calculate the number of main mode.
            u8 drbg_mainModeNum = cs_sub_mode_insertion(blt_pCsCfg->Main_Mode_Max_Steps,blt_pCsCfg->Main_Mode_Min_Steps);


            ///if lastLeaveStepNum != 0, indicate run one subevent at least and the content is mode-0.
            u8 lastLeaveStepNum = (u8)(blt_pCsCfg->slip_stepWriteIdx - blt_pCsCfg->slip_stepReadIdx);

            ////////////////////////////above need to process/////////////////////////////////////////

            ///if existed leaved steps, calculate the leaved steps duration in unit of us(microsecond).
            u32 lastLeaveStepUs = 0;
            for(int i=0; i<lastLeaveStepNum; i++){
                ///if lastLeaveStepNum = 0, here not run.
                lastLeaveStepUs += blt_cs_slipWindowstepDurCal(i);//base on blt_pCsCfg->slip_stepReadIdx
            }

            ///calculate the available time for current subevent non-mode-0's step to use.
            u32 curAvailStepUs = blt_pCsCfg->Subevent_Len - lastLeaveStepUs; /// lastLeaveStepUs has include mode0...+ repetition + submode...

            ///calculate the time of current subevent's main_mode
            u32 curReqdMainModeUs = drbg_mainModeNum*blt_pCsCfg->mainModeStep_durUs;

            u8 statusFlag = 0;
            (void)statusFlag;//clean warning: variable 'statusFlag' set but not used [-Wunused-but-set-variable],by SunWei
            u32 t1stStepLeaveTime4MainSub_Us = 0;

            u8 tCurrSubevtArngMainNum = 0;

            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            ///according to the real situation to arrange step in slip_window_step[]. include mode and channel.
            ///if lastLeaveStepNum != 0, indicate before run one subevent at least and the content is mode-0 at least.
            if(lastLeaveStepNum){
                if(curAvailStepUs>=curReqdMainModeUs){

                    u32 forSubmodeUs = curAvailStepUs - curReqdMainModeUs;

                    //case 1: current subevent can run all steps include previous leave and current.
                    if(forSubmodeUs >= blt_pCsCfg->subModeStep_durUs){
                        statusFlag = CUR_SUBEVENT_NOT_LEAVE;

                        tCurrSubevtArngMainNum = drbg_mainModeNum;

                        //+1 is for sub_mode
                        currSubevtStepNum = blt_pCsCfg->slip_stepWriteIdx - blt_pCsCfg->slip_stepReadIdx + tCurrSubevtArngMainNum + 1;//must place before blt_cs_curSub_notLeave_proc();

                        blt_cs_curSub_notLeave_proc(tCurrSubevtArngMainNum, tSubevtJumpNum);
                    }
                    //case 2: current subevent's SUB_MODE will be leaved to the following subevent.
                    else{
                        statusFlag = CUR_SUBEVENT_ONLY_LEAVE_SUB_MODE;

                        tCurrSubevtArngMainNum = drbg_mainModeNum;

                        currSubevtStepNum = blt_pCsCfg->slip_stepWriteIdx - blt_pCsCfg->slip_stepReadIdx + tCurrSubevtArngMainNum;//must place before blt_cs_curSub_leaveMode_proc();

                        blt_cs_curSub_leaveMode_proc(tCurrSubevtArngMainNum, 0, tSubevtJumpNum); //leaveMainModeNum = 0;
                    }
                }
                //case 3: current subevent's MAIN_MDOE and SUB_MODE will be leaved to the following subevent.
                else{
                    statusFlag = CUR_SUBEVENT_LEAVE_MAIN_SUB_MODE;

                    u8 leaveMainModeNum = curAvailStepUs/blt_pCsCfg->mainModeStep_durUs - drbg_mainModeNum;
                    tCurrSubevtArngMainNum = drbg_mainModeNum - leaveMainModeNum;

                    currSubevtStepNum = blt_pCsCfg->slip_stepWriteIdx - blt_pCsCfg->slip_stepReadIdx + tCurrSubevtArngMainNum;//must place before blt_cs_curSub_leaveMode_proc();

                    blt_cs_curSub_leaveMode_proc(tCurrSubevtArngMainNum, leaveMainModeNum, tSubevtJumpNum);

                }
            }else{
                t1stStepLeaveTime4MainSub_Us = (curAvailStepUs - blt_pCsCfg->allMode0Step_durUs);

                if(t1stStepLeaveTime4MainSub_Us >= curReqdMainModeUs){
                    u32 forSubmodeUs_1 = t1stStepLeaveTime4MainSub_Us - curReqdMainModeUs;

                    //case 1: current subevent can run all steps include previous leave and current.
                    if(forSubmodeUs_1 >= blt_pCsCfg->subModeStep_durUs){
                        statusFlag = CUR_SUBEVENT_NOT_LEAVE;

                        tCurrSubevtArngMainNum = drbg_mainModeNum;
                        blt_cs_1stStepProcedure_curSub_notLeave_proc(tCurrSubevtArngMainNum, tSubevtJumpNum);

                        currSubevtStepNum = blt_pCsCfg->Mode_0_Steps + tCurrSubevtArngMainNum + 1; //+1 is for sub_mode
                    }
                    //case 2: current subevent's SUB_MODE will be leaved to the following subevent.
                    else{
                        statusFlag = CUR_SUBEVENT_ONLY_LEAVE_SUB_MODE;

                        tCurrSubevtArngMainNum = drbg_mainModeNum;
                        blt_cs_1stStepProcdure_curSub_LeaveMode_proc(tCurrSubevtArngMainNum, 0, tSubevtJumpNum);

                        currSubevtStepNum = blt_pCsCfg->Mode_0_Steps + tCurrSubevtArngMainNum;
                    }
                }else{
                    //case 3: current subevent's MAIN_MDOE and SUB_MODE will be leaved to the following subevent.
                    statusFlag = CUR_SUBEVENT_LEAVE_MAIN_SUB_MODE;

                    u8 leaveMainModeNum = t1stStepLeaveTime4MainSub_Us/blt_pCsCfg->mainModeStep_durUs - drbg_mainModeNum;
                    tCurrSubevtArngMainNum = drbg_mainModeNum - leaveMainModeNum;
                    blt_cs_1stStepProcdure_curSub_LeaveMode_proc(tCurrSubevtArngMainNum, leaveMainModeNum, tSubevtJumpNum);

                    currSubevtStepNum = blt_pCsCfg->Mode_0_Steps + tCurrSubevtArngMainNum;
                }
            }
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            jumpStepSum += currSubevtStepNum;

            ////////it is used for cs_sub_mode_insertion()////////
            /*
             * Here run all required DRBG algorithm. then set the relevant value to slip_window_step[] to be run.
             */
            if(tSubevtJumpNum == 1){ /// tSubevtJumpNum = 1, indicate the current subevent to be run.

                jumpStepSum -= currSubevtStepNum; //the last currSubevtStepNum value not jump, it is the step to run immediately.
            }

            tSubevtJumpNum--;

        }while(tSubevtJumpNum > 0);
        #endif
    }else{
        ///timing is : mode0...->repetition(1st not exist)->main mode...->
        int i = 0;

        do{//tSubevtJumpNum is 1 at least

            blt_pCsCfg->slip_stepReadIdx = blt_pCsCfg->slip_stepWriteIdx;

            u8 writeSlipIdx = blt_pCsCfg->slip_stepWriteIdx&SLIP_WINDOW_STEP_MSK;
            slip_window_step_t * pslip_window_step = &blt_pCsCfg->slip_window_step[writeSlipIdx];

            /////////////////////////////////////
            //step 1: arrange mode-0 steps.
            for(int j=0; j < blt_pCsCfg->Mode_0_Steps; j++){
                pslip_window_step->step_modeType = STEP_MODE_0;
                pslip_window_step->subeventEndFlag = 0; //clear

                blt_cs_select_mode0ChnIdx(blt_pCsCfg);

                if(tSubevtJumpNum == 1){
                    ///32M: run one time cost 183us
                    cs_access_addr((u8*)&pslip_window_step->step_reflAA, (u8*)&pslip_window_step->step_initAA);
                    tlkapi_send_string_u32s(0, "m0AA", pslip_window_step->step_reflAA, pslip_window_step->step_initAA, 0,0);
                }

                cs_step_add();
                writeSlipIdx = ++blt_pCsCfg->slip_stepWriteIdx;
                pslip_window_step = &blt_pCsCfg->slip_window_step[writeSlipIdx&SLIP_WINDOW_STEP_MSK];

                tlkapi_send_string_u32s(0, "slip IDX", writeSlipIdx, blt_pCsCfg->slip_stepWriteIdx, 0, 0);
            }

            //step 2: if 1st, no repetition step and only arrange main mode step. if has been run, need to arrange repetition firstly here.
            if( !blt_pCsCfg->cs_procdure_1st_flag ){

                u8 repetCnt = 0;
                for(i=0; i<blt_pCsCfg->Main_Mode_Repetition; i++){
                    pslip_window_step->step_modeType = blt_pCsCfg->Main_Mode;
                    pslip_window_step->subeventEndFlag = 0; //clear

                    blt_cs_stepDRBG_proc(blt_pCsCfg->Main_Mode); //other DRBG calculate

                    ////////////////////////////////////////////////
                    //blt_cs_select_repetMainChnIdx(); //channel

                    u16 repetIdx = (u16)(blt_pCsCfg->nonMode0_chnReadIdx - blt_pCsCfg->Main_Mode_Repetition + repetCnt);
                    repetIdx = repetIdx%blt_pCsCfg->Chn_en_num;

                    pslip_window_step->step_chnIdx = blt_pCsCfg->nonmode0ShuffledChnArray[repetIdx];

                    repetCnt++;
                    ////////////////////////////////////////////////

                    if(tSubevtJumpNum == 1 && blt_pCsCfg->Main_Mode != STEP_MODE_2 ){
                        cs_access_addr((u8*)&pslip_window_step->step_reflAA, (u8*)&pslip_window_step->step_initAA);
                    }

                    cs_step_add();
                    writeSlipIdx = ++blt_pCsCfg->slip_stepWriteIdx;
                    pslip_window_step = &blt_pCsCfg->slip_window_step[writeSlipIdx&SLIP_WINDOW_STEP_MSK];
                }

                for(i=0; i < (blt_pCsCfg->mainNum_noSubMode-blt_pCsCfg->Main_Mode_Repetition); i++){
                    pslip_window_step->step_modeType = blt_pCsCfg->Main_Mode;
                    pslip_window_step->subeventEndFlag = 0; //clear

                    blt_cs_stepDRBG_proc(blt_pCsCfg->Main_Mode);
                    blt_cs_select_nonMode0ChnIdx(blt_pCsCfg);

                    if(tSubevtJumpNum == 1 && blt_pCsCfg->Main_Mode != STEP_MODE_2){
                        cs_access_addr((u8*)&pslip_window_step->step_reflAA, (u8*)&pslip_window_step->step_initAA);
                        tlkapi_send_string_u32s(0, "no1stMainAA", pslip_window_step->step_reflAA, pslip_window_step->step_initAA, 0,0);

                    }

                    cs_step_add();
                    writeSlipIdx = ++blt_pCsCfg->slip_stepWriteIdx;
                    pslip_window_step = &blt_pCsCfg->slip_window_step[writeSlipIdx&SLIP_WINDOW_STEP_MSK];
                }
            }else{

                blt_pCsCfg->cs_procdure_1st_flag = 0;

                blt_cs_nonModeChn_cal(blt_pCsCfg);

                for(i=0; i<blt_pCsCfg->mainNum_noSubMode; i++){
                    pslip_window_step->step_modeType = blt_pCsCfg->Main_Mode;
                    pslip_window_step->subeventEndFlag = 0; //clear

                    blt_cs_stepDRBG_proc(blt_pCsCfg->Main_Mode);
                    blt_cs_select_nonMode0ChnIdx(blt_pCsCfg);

                    if(tSubevtJumpNum == 1 && blt_pCsCfg->Main_Mode != STEP_MODE_2){
                        cs_access_addr((u8*)&pslip_window_step->step_reflAA, (u8*)&pslip_window_step->step_initAA);
                        tlkapi_send_string_u32s(0, "1stMainAA", pslip_window_step->step_reflAA, pslip_window_step->step_initAA, 0,0);

                    }

                    cs_step_add();
                    writeSlipIdx = ++blt_pCsCfg->slip_stepWriteIdx;
                    pslip_window_step = &blt_pCsCfg->slip_window_step[writeSlipIdx&SLIP_WINDOW_STEP_MSK];
                }
            }
        }while(--tSubevtJumpNum);

        u8 endStepIdx = (blt_pCsCfg->slip_stepWriteIdx-1)&SLIP_WINDOW_STEP_MSK;
        blt_pCsCfg->slip_window_step[endStepIdx].subeventEndFlag = 1;
    }
    //////////////////////////////////////////////////////////////////////////////////////////////
}
#endif


#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_
int blt_cs_subevent_start(int cs_cfg_idx, void *p)
{
    sch_task_t *pCsSch = (sch_task_t*)p;
    blt_pCsCfg = (cs_config_t *)(gGlobal_pCsCfg + cs_cfg_idx);

    DBG_CHN9_HIGH;
    DBG_CS_CHN2_HIGH;

#if (SL01_cs_subevent_0)
    log_task_begin_irq(SL_STACK_CS_TIME_EN, SL01_cs_subevent_0);
#endif

    blt_pCsCfg->sSlot_mark_csSubevent =  bltSche.sSlot_idx_irq_real;
    blt_pCsCfg->bSlot_mark_csSubevent =  bltSche.bSlot_idx_irq_real;


    int cs_subevent_jump = 0;
    if(blt_pCsCfg->csProcCount != pCsSch->cs_procCnt){
        cs_subevent_jump = pCsSch->cs_subevent_seqNum;
        blt_pCsCfg->csProcCount = pCsSch->cs_procCnt;

        tlkapi_send_string_u8s(DBG_CS_LOG_SCH_MASK_EN, "[CS] procedure start", blt_pCsCfg->procMaxCount);
        //todo New procedure start, main mode, repetition all need to start again
    }
    else{
        cs_subevent_jump = (u32)(pCsSch->cs_subevent_seqNum - blt_pCsCfg->seqNum_mark_csSubEvent);
    }
    blt_pCsCfg->cs_sub_event_oft = pCsSch->cs_oft;


    if((blt_pCsCfg->flag_endEvtInProc  &&  (blt_pCsCfg->cs_sub_event_oft == (blt_pCsCfg->Subevents_Per_Event - 1)))\
             || (pCsSch->cs_subevent_seqNum==32)){
        blt_pCsCfg->proc_end_flag = 1;

        DBG_CS_CHN7_TOGGLE;
    }


    if(cs_subevent_jump){

        cs_subevent_jump -= 1;
        //todo There are subevent skips, and you need to calculate how many stepCNTs you skipped
    }
    tlkapi_send_string_data(0, "subevent jump", &cs_subevent_jump, 4);

    blt_cs_subevent_rf_init();

    blt_pCsCfg->mode0_rx_flag = 0; //clear per subevent.

    tlkapi_send_string_u32s(DBG_CS_LOG_SCH_MASK_EN, "[CS] subevent start", blt_pCsCfg->seqNum_mark_csSubEvent, cs_subevent_jump, blt_pCsCfg->cs_sub_event_oft);
    blt_pCsCfg->step_expect_tick = blt_pCsCfg->tick_expect_csSubevent + pCsSch->cs_oft * blt_pCsCfg->subEvtIntvl_625us * SYSTEM_TIMER_TICK_625US;
    rf_ble_set_tx_settle(TX_STL_TIFS_REAL_COMMON);
    rf_ble_set_rx_settle(RX_SETTLE_US);

    #if (HADM_PHASE_CONTINUITY)
        if(!cs_phase_continuity_flag && blt_pCsCfg->phaseContin_config_flag){
            ble_rf_cs_phase_continuity_en();//50us
        }
    #endif

#if (QIUWEI_STEP_MODE_PROC)
    blt_cs_subevent_stepConfig_proc(pCsSch);
#endif
    if((blt_pCsCfg->Role == CHANNEL_SOUNDING_ROLE_INITIATOR)){

        blt_pCsCfg->step_expect_tick += (g_T_FCS_us + 4 + pCsSch->cs_oft*5) *SYSTEM_TIMER_TICK_1US;
        ll_cs_initiator_irq_task_cb(FLAG_CS_STEP_INIT_STX_START);  // blt_cs_initiator_irq_task     blt_cs_init_step_stx_start
    }else{


        blt_pCsCfg->step_expect_tick += (4 + pCsSch->cs_oft*5) *SYSTEM_TIMER_TICK_1US;
        ll_cs_reflector_irq_task_cb(FLAG_CS_STEP_REFL_SRX_START);    //blt_cs_reflector_irq_task      blt_cs_refl_stepSrx
    }



    blt_pCsCfg->seqNum_mark_csSubEvent = pCsSch->cs_subevent_seqNum; //move here. above need to use mark and current subevent number.

    return 0;
}


#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_
int blt_cs_interrupt_task (int flag, void *p){

    if(flag == FLAG_CS_ACL_CB_FLAG)
    {

        st_ll_conn_t *pAcl = (st_ll_conn_t*) p;
        //ACL conn establish set channle sounding param
        //Role_Enable: all disable,   CS_SYNC_Antenna_Selection is set to 0x01
        cs_param_t *pCsParam = &pAcl->csParam;

        pCsParam->role_enable = 0;
        pCsParam->CS_SYNC_AntSel = 1;
        pCsParam->cs_cap_req = 0;
        pCsParam->cs_cap_exchange = 0;
        pCsParam->cs_fae_exchange = 0;
        pCsParam->cs_config_req = 0;
        pCsParam->cs_security_enable = 0;
        pCsParam->cs_security_exchange = 0;
        pCsParam->cs_req = 0;
        pCsParam->cs_fae_req = 0;
        pAcl->csMapMask = 0;

        for(int i = 0 ; i<gCsMng.max_num_cofig; i++)
        {
            cs_config_t *pCfg = gGlobal_pCsCfg + i;
            if((pCfg->occupy)  && (pAcl->acl_conHandle ==pCfg->aclHandle))
            {
                pCfg->occupy = 0;
                pCfg->state  = 0;
                pCfg->chn_update_pend = 0;
            }
        }
    }
    else if(flag & FLAG_SCHEDULE_BUILD){
        blt_ll_rebuildCsSchedulerLinklist();
    }
    else if(flag & FLAG_SCHEDULE_POLL){
        blt_ll_acl_post_checkCsTask((st_ll_conn_t*) p);
    }
    else if(flag & FLAG_CS_ACL_DISCONN_CB){
        st_ll_conn_t *pc = (st_ll_conn_t*)p;
        for(int i = 0 ; i<gCsMng.max_num_cofig; i++)
        {
            if(pc->csTaskEnableMask &(BIT(i)))
            {
                blt_sche_removeTaskMask(TSKMSK_CS_0<<i);
            }
        }
        pc->csTaskEnableMask = 0;
        pc->cs_pending = 0;
    }
    else if(flag & FLAG_CS_SUBEVENT_START){
        blt_cs_subevent_start(flag&FLAG_SCHEDULE_TASK_IDX_MASK, p);
    }
    else if(flag & FLAG_CS_STEP_RX){
        blt_cs_step_rx();
    }
    else if(flag & FLAG_INSERT_SCHTSK_CONFLICT){
        return 1;
    }

    return 0;
}

int blc_cs_resetByHandle(u16 connHandle){

    st_ll_conn_t* pAcl = (st_ll_conn_t*)blt_ll_getAclConnPtr(connHandle);
    cs_param_t *pCsParam = &pAcl->csParam;
    pCsParam->role_enable = 0;
    pCsParam->CS_SYNC_AntSel = 1;
    pCsParam->cs_cap_req = 0;
    pCsParam->cs_cap_exchange = 0;
    pCsParam->cs_fae_exchange = 0;
    pCsParam->cs_config_req = 0;

    pCsParam->cs_security_enable = 0;
    pCsParam->cs_security_exchange = 0;

    pCsParam->cs_req = 0;

    pCsParam->cs_fae_req = 0;
    pAcl->csMapMask = 0;


    for(int i = 0 ; i<gCsMng.max_num_cofig; i++)
    {
        cs_config_t *pCfg = gGlobal_pCsCfg + i;
        if((pCfg->occupy)  && (connHandle==pCfg->aclHandle))
        {
            pCfg->occupy = 0;
            pCfg->state  = 0;
            pCfg->chn_update_pend = 0;
        }
    }



    return 0;
}


static u32 blt_ll_getStepDuration_us(cs_config_t *pCsCfg, u8 mode, u8 t_fcs){

        u32 duration_us = 0;

        u8  oneByteUs  = (pCsCfg->CS_SYNC_PHY == BLE_1M_PHY) ? 8 : 4;
        u8 configId_T_SW_us = 0;
        u8 num_ap = ACI_to_N_AP[pCsCfg->aci];//aci only can be know after LL_CS_IND.

        g_antennaPathNum = num_ap;
        st_ll_conn_t * pAclConn = (st_ll_conn_t *)(u32)&blms[pCsCfg->aclHandle & CONN_IDX_MASK];
        /*
         * 1. There shall not be antenna switching activity in the 1:1 configuration.
         * 2. In this configuration, with N_AP antennas in the initiator, the initiator shall be the only device performing
         *    antenna switching. The antenna switch duration selected shall be the T_SW value of the initiator
         */
        if(num_ap==1){
            configId_T_SW_us = 0;
        }
        else if((pCsCfg->aci >=1) &&  ((pCsCfg->aci <=3)))//1, 2, 3
        {
            configId_T_SW_us = (pCsCfg->Role==CS_CONFIG_INITIATOR_ROLE)? bltCsLocalSupportCap.T_SW_Time_Supported: pAclConn->csRemoteSupCap.T_SW_Time_Supported;
        }
        else if((pCsCfg->aci >=4) &&  ((pCsCfg->aci <=6))){
            configId_T_SW_us = (pCsCfg->Role==CS_CONFIG_INITIATOR_ROLE)?pAclConn->csRemoteSupCap.T_SW_Time_Supported:bltCsLocalSupportCap.T_SW_Time_Supported;
        }
        else{
            configId_T_SW_us = max(bltCsLocalSupportCap.T_SW_Time_Supported, pAclConn->csRemoteSupCap.T_SW_Time_Supported);//unit us
        }
        g_T_SW_us = configId_T_SW_us;

        switch(mode)
        {
            case MAINMODE_TYPE_MODE_1:
            {
                u16 mode1_T_SY_noPayload = (pCsCfg->CS_SYNC_PHY == BLE_1M_PHY) ? MODE_1_T_SY_1M_US_WITHOUT_SS_RS : MODE_1_T_SY_2M_US_WITHOUT_SS_RS;
                /*** mode_1: T_FCS + T_SY + T_RD + T_IP1 + T_SY + T_FM + T_RD***/
                duration_us = t_fcs + 2*(mode1_T_SY_noPayload + (RTT_Type_SeqNum[pCsCfg->RTT_Type]/8)*oneByteUs) + 2*T_RD_US + g_T_IP1_us;
                pCsCfg->mode1Step_durUs = duration_us;
            }
            break;
            case MAINMODE_TYPE_MODE_2:
            {
                /***T_FCS + (T_SW+T_PM)*(N_AP+1) + T_RD + T_IP2 + (T_SW+T_PM)*(N_AP+1) + T_RD; Note: + 1 is for extended slot. */

                duration_us = t_fcs + 2*(configId_T_SW_us+T_PM_US[pCsCfg->T_PM])*(num_ap+1) + 2*T_RD_US + g_T_IP2_us;
                pCsCfg->mode2Step_durUs = duration_us;
            }
            break;
            case MAINMODE_TYPE_MODE_3:
            {
                u16 mode3_T_SY_noPayload = (pCsCfg->CS_SYNC_PHY == BLE_1M_PHY) ? MODE_3_T_SY_1M_US_WITHOUT_SS_RS : MODE_3_T_SY_2M_US_WITHOUT_SS_RS;
                /***T_FCS + T_SY + T_GD + (T_SW+T_PM)*(N_AP+1) + T_RD + T_IP2 + (T_SW+T_PM)*(N_AP+1) + T_GD + T_SY + T_RD***/
                u16 tmp_T_SY = 2*(mode3_T_SY_noPayload + (RTT_Type_SeqNum[pCsCfg->RTT_Type]/8)*oneByteUs);
                duration_us = t_fcs + 2*(tmp_T_SY + T_GD_US + (configId_T_SW_us+T_PM_US[pCsCfg->T_PM])*(num_ap +1 ) + T_RD_US) + g_T_IP2_us;
                pCsCfg->mode3Step_durUs = duration_us;
            }
            break;
            default:
                break;

        }

        return duration_us;
}

u32 switchBitMask2Index(u32 bitMsk){
    for(int i=0; i<32;i++){
        if( bitMsk & BIT(0)){
            return i;
        }
    }
    return 0xFF;
}

ble_sts_t   blt_ll_calcStepDuration(cs_config_t *pCsCfg){

    g_T_IP1_us = T_IP_US[pCsCfg->T_IP1];
    g_T_IP2_us = T_IP_US[pCsCfg->T_IP2];
    g_T_FCS_us = T_FCS_US[pCsCfg->T_FCS];
    g_t_pm_us = T_PM_US[pCsCfg->T_PM];

    pCsCfg->t_synu_us = (pCsCfg->CS_SYNC_PHY == BLE_1M_PHY) ? MODE_0_T_SY_1M_US : MODE_0_T_SY_2M_US;

    /*** mode_0: T_FCS + T_SY + T_RD +  T_IP1 + T_SY + T_GD + T_FM + T_RD***/
    /*** this variable mode0_durUs may be used to calculate the main mode number in subevent ***/
    pCsCfg->allMode0Step_durUs = pCsCfg->Mode_0_Steps*(g_T_FCS_us + 2*pCsCfg->t_synu_us + 2*T_RD_US + g_T_IP1_us  + T_GD_US + MODE_0_T_FM_US);
    pCsCfg->mode0Step_durUs = g_T_FCS_us + 2*pCsCfg->t_synu_us + 2*T_RD_US + g_T_IP1_us  + T_GD_US + MODE_0_T_FM_US;
    pCsCfg->mainModeStep_durUs = blt_ll_getStepDuration_us(pCsCfg, pCsCfg->Main_Mode, g_T_FCS_us);


    if(pCsCfg->Sub_Mode == SUBMODE_TYPE_MODE_UNUSED){
        ///calculate the number of the main mode if there is no sub_mode
        pCsCfg->mainMode_num = (pCsCfg->Subevent_Len - pCsCfg->allMode0Step_durUs)/pCsCfg->mainModeStep_durUs;
        pCsCfg->subModeStep_durUs = 0;
    }
    else{
        pCsCfg->subModeStep_durUs = blt_ll_getStepDuration_us(pCsCfg, pCsCfg->Sub_Mode, g_T_FCS_us);
    }

    return BLE_SUCCESS;
}


void blt_ll_cs_exchangeCapProc(st_ll_conn_t* pAclConn){

    cs_param_t *pCsParam = &pAclConn->csParam;
    if(pCsParam->cs_cap_req == PROC_EVT_PENDING){
        pCsParam->cs_cap_exchange = 1;
        pCsParam->cs_cap_req = 0;
        hci_le_readRemoteSupCapComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8*)&pAclConn->csRemoteSupCap);
    }
    else //send req/rsp
    {
        chn_sound_capbilities_t *local = &bltCsLocalSupportCap;
        u8 buff[sizeof(rf_packet_ll_cs_cap_req_t)] = {0};
        rf_packet_ll_cs_cap_req_t *cap_req = (rf_packet_ll_cs_cap_req_t*) buff;

        cap_req->type                               = LLID_CONTROL;
        cap_req->rf_len                             = sizeof(rf_packet_ll_cs_cap_req_t) - 2;
        cap_req->Mode_Types                         = local->Mode_Types;
        cap_req->RTT_Capability                     = local->RTT_Capability;
        cap_req->RTT_AA_Only_N                      = local->RTT_AA_Only_N;
        cap_req->RTT_Sounding_N                     = local->RTT_Sounding_N;
        cap_req->RTT_Random_Sequence_N              = local->RTT_Random_Payload_N;
        cap_req->NADM_Sounding_Capability           = local->Optional_NADM_Sounding_Capability;
        cap_req->NADM_Random_Sequence_Capability    = local->Optional_NADM_Random_Capability;
        cap_req->CS_SYNC_PHY_Capability             = local->Optional_CS_SYNC_PHYs_Supported;
        cap_req->Num_Ant                            = local->Num_Antennas_Supported;
        cap_req->Max_Ant_Path                       = local->Max_Antenna_Paths_Supported;
        cap_req->Role                               = local->Roles_Supported;
        cap_req->Companion_Signal                   = (local->Optional_Subfeatures_Supported & CS_COMPANION_SIGNAL_SUPPORT) ?1:0;
        cap_req->No_FAE                             = (local->Optional_Subfeatures_Supported & CS_No_FAE_SUPPORT)?1:0;
        cap_req->chn_sel_3c                         = (local->Optional_Subfeatures_Supported & CS_CSA_3C_SUPPORT) ?1:0;
        cap_req->Sounding_PCT_Estimate              = (local->Optional_Subfeatures_Supported & CS_SOUNDING_PCT_ESTIMATE_SUPPORT) ?1:0;
        cap_req->Num_Configs                        = local->Num_Config_Supported;
        cap_req->Max_Procedures_Supported           = local->max_consecutive_procedures_supported;
        cap_req->T_SW                               = local->T_SW_Time_Supported;
        cap_req->T_IP1_Capability                   = local->Optional_T_IP1_Times_Supported;
        cap_req->T_IP2_Capability                   = local->Optional_T_IP2_Times_Supported;
        cap_req->T_FCS_Capability                   = local->Optional_T_FCS_Times_Supported;
        cap_req->T_PM_Capability                    = local->Optional_T_PM_Times_Supported;


        if(pCsParam->cs_cap_req & PROC_SEND_RSP){
            CS_HCI_LOG("[CAP][TX] rsp");

            cap_req->Num_Ant = 1; //fanqh reflect need 1 ant in google LR20
            cap_req->opcode = LL_CS_CAPABILITIES_RSP;
        }
        else if(pCsParam->cs_cap_req & PROC_SEND_REQ){//EXCHANGE_SEND_REQ
            CS_HCI_LOG("[CAP][TX] req:0x%x", pCsParam->cs_cap_req);
            cap_req->opcode = LL_CS_CAPABILITIES_REQ;
        }

        if(ll_push_tx_fifo_handler (pAclConn->acl_conHandle | HANDLE_STK_FLAG, (u8 *)cap_req))
        {
            if(pCsParam->cs_cap_req & PROC_SEND_RSP){
                pCsParam->cs_cap_req = 0;
                pCsParam->cs_cap_exchange = 1;
                pAclConn->ll_rsp_timeout_tick = 0;
                hci_le_readRemoteSupCapComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8*)&pAclConn->csRemoteSupCap);
            }
            else{
                pCsParam->cs_cap_req &= ~PROC_SEND_REQ;
                pCsParam->cs_cap_req |= PROC_WAIT_RSP;
                pAclConn->ll_rsp_timeout_tick = clock_time() | 1;
            }

        }
    }

}

void blt_ll_cs_buffCombize(u8* input_l,u8* input_h,u8* output,u8 len)
{
    for(int i = 0;i<(len);i++){
        output[i] = input_l[i];
        output[i+len] = input_h[i];
    }
}

void blt_ll_cs_exchangeSecurityStartProc(st_ll_conn_t* pAclConn){

    cs_param_t *pCsParam = &pAclConn->csParam;
    if(pCsParam->cs_security_enable == PROC_EVT_PENDING){
        pCsParam->cs_security_exchange = 1;
        pCsParam->cs_security_enable = 0;
        hci_le_csSecurityEnableComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle);
    }
    else //send req/rsp
    {
        u8 buff[sizeof(rf_pkt_ll_cs_sec_req_t)];

        rf_pkt_ll_cs_sec_req_t *sec_req = (rf_pkt_ll_cs_sec_req_t*) buff;
        sec_req->type   = LLID_CONTROL;
        if(pCsParam->cs_security_enable & PROC_SEND_RSP){
            rf_pkt_ll_cs_sec_rsp_t *sec_rsp = (rf_pkt_ll_cs_sec_rsp_t*) buff;
            sec_rsp->opcode = LL_CS_SEC_RSP;
            sec_rsp->rf_len = sizeof(rf_pkt_ll_cs_sec_rsp_t) -2;

#if 1 //temporary use default value. later delete. qiuwei
            generateRandomNum(8,&pCsParam->CS_IV_P[0]);
            generateRandomNum(4,&pCsParam->CS_IN_P[0]);
            generateRandomNum(8,&pCsParam->CS_PV_P[0]);
#else
            u8 tCS_IV_P[8] = {0xe9 ,0xdf ,0xfd ,0x0b ,0x8a ,0xc2 ,0x0b ,0xe1};
            u8 tCS_IN_P[4] = {0xc1 ,0x77 ,0xf4 ,0x9f};
            u8 tCS_PV_P[8] = {0x44 ,0xed ,0x82 ,0x98 ,0xdf ,0xde ,0x80 ,0xc9};

            smemcpy(pCsParam->CS_IV_P, tCS_IV_P, 8);
            smemcpy(pCsParam->CS_IN_P, tCS_IN_P, 4);
            smemcpy(pCsParam->CS_PV_P, tCS_PV_P, 8);
#endif
            smemcpy(&sec_rsp->CS_IV_P[0],&pCsParam->CS_IV_P[0],8);
            smemcpy(&sec_rsp->CS_IN_P[0],&pCsParam->CS_IN_P[0],4);
            smemcpy(&sec_rsp->CS_PV_P[0],&pCsParam->CS_PV_P[0],8);

            blt_ll_cs_buffCombize(&pCsParam->CS_IV_C[0],&pCsParam->CS_IV_P[0],&pCsParam->CS_IV[0],8);
            blt_ll_cs_buffCombize(&pCsParam->CS_IN_C[0],&pCsParam->CS_IN_P[0],&pCsParam->CS_IN[0],4);
            blt_ll_cs_buffCombize(&pCsParam->CS_PV_C[0],&pCsParam->CS_PV_P[0],&pCsParam->CS_PV[0],8);

            drbg_instantiation_func_h9(pCsParam->CS_IV, pCsParam->CS_IN,pCsParam->CS_PV, kdrbg_global, vdrbg_global);///////////
            cs_drbg_init();
            CS_HCI_LOG("[SEC][TX] rsp");
        }
        else if(pCsParam->cs_security_enable & PROC_SEND_REQ){//EXCHANGE_SEND_REQ

            sec_req->opcode = LL_CS_SEC_REQ;
            sec_req->rf_len = sizeof(rf_pkt_ll_cs_sec_req_t) -2;
#if 1 //temporary use default value. later delete. qiuwei
            generateRandomNum(8,&pCsParam->CS_IV_C[0]);
            generateRandomNum(4,&pCsParam->CS_IN_C[0]);
            generateRandomNum(8,&pCsParam->CS_PV_C[0]);
#else
            u8 tCS_IV_C[8] = {0x3b ,0x0b ,0xca ,0xe0 ,0x86 ,0x51 ,0x7f ,0x3e};
            u8 tCS_IN_C[4] = {0x0d ,0x84 ,0x73 ,0x86};
            u8 tCS_PV_C[8] = {0x43 ,0xf1 ,0x68 ,0x78 ,0x96 ,0x74 ,0xa6 ,0x64};
            smemcpy(pCsParam->CS_IV_C, tCS_IV_C, 8);
            smemcpy(pCsParam->CS_IN_C, tCS_IN_C, 4);
            smemcpy(pCsParam->CS_PV_C, tCS_PV_C, 8);
#endif
            smemcpy(&sec_req->CS_IV_C[0],&pCsParam->CS_IV_C[0],8);
            smemcpy(&sec_req->CS_IN_C[0],&pCsParam->CS_IN_C[0],4);
            smemcpy(&sec_req->CS_PV_C[0],&pCsParam->CS_PV_C[0],8);
            CS_HCI_LOG("[SEC][TX] req");
        }

        if(ll_push_tx_fifo_handler (pAclConn->acl_conHandle | HANDLE_STK_FLAG, (u8 *)sec_req))
        {

            if(pCsParam->cs_security_enable & PROC_SEND_RSP){
                pCsParam->cs_security_enable = 0;
                pCsParam->cs_security_exchange = 1;
                pAclConn->ll_rsp_timeout_tick = 0;
                hci_le_csSecurityEnableComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle);
            }
            else{
                pCsParam->cs_security_enable &= ~PROC_SEND_REQ;
                pCsParam->cs_security_enable |= PROC_WAIT_RSP;
                pAclConn->ll_rsp_timeout_tick = clock_time() | 1;
            }

        }
    }

}
void blt_ll_cs_exchangeFaeTableProc(st_ll_conn_t* pAclConn){

    cs_param_t *pCsParam = &pAclConn->csParam;
    if(pCsParam->cs_fae_req == PROC_EVT_PENDING){
        pCsParam->cs_fae_req = 0;
        pCsParam->cs_fae_exchange = 1;
        u8 reason = BLE_SUCCESS;
        if(cs_fae_cmplt_reason){
            reason = cs_fae_cmplt_reason;
            cs_fae_cmplt_reason = 0;
        }
        hci_le_readRemoteFAETableComplete_evt(reason, pAclConn->acl_conHandle, (u8*)&pCsParam->fae_table[0]);
    }
    else //send req/rsp
    {
        u8 buff[sizeof(rf_pkt_ll_cs_fae_rsp_t)] = {0};
        rf_pkt_ll_cs_fae_rsp_t *pfae_table = (rf_pkt_ll_cs_fae_rsp_t*) buff;
        //from 2 to 22 and 26 to 76
        s8 local_fae_table[72] = {-1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0};
        pfae_table->type    = LLID_CONTROL;
        if(pCsParam->cs_fae_req & PROC_SEND_REQ){
            pfae_table->opcode = LL_CS_FAE_REQ;
            pfae_table->rf_len = 1;
            CS_HCI_LOG("[FAE][TX] req");
        }
        else if(pCsParam->cs_fae_req & PROC_SEND_RSP){
            pfae_table->opcode = LL_CS_FAE_RSP;
            pfae_table->rf_len = (u8)(sizeof(rf_pkt_ll_cs_fae_rsp_t) - 2);
            smemcpy(pfae_table->fae_table,local_fae_table,72);
            CS_HCI_LOG("[FAE][TX] rsp");
        }

        if(ll_push_tx_fifo_handler (pAclConn->acl_conHandle | HANDLE_STK_FLAG, (u8 *)pfae_table))
        {
            if(pCsParam->cs_fae_req & PROC_SEND_RSP){
                pCsParam->cs_fae_req = 0;
                pCsParam->cs_fae_exchange = 1;
                pAclConn->ll_rsp_timeout_tick = 0;
                hci_le_readRemoteFAETableComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8*)&pfae_table->fae_table[0]);
            }
            else{
                pCsParam->cs_fae_req &= ~PROC_SEND_REQ;
                pCsParam->cs_fae_req |= PROC_WAIT_RSP;
                pAclConn->ll_rsp_timeout_tick = clock_time() | 1;
            }

        }
    }

}

ble_sts_t blt_ll_cs_exchangeConfigReq(st_ll_conn_t* pAclConn, cs_config_t *pCsCfg){

    cs_param_t *pCsParam = &pAclConn->csParam;

    if(pCsParam->cs_config_req == PROC_EVT_PENDING){
        pCsParam->cs_config_req = 0;
        hci_le_csConfigComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8*)pCsCfg);
    }
    else
    {
        u8 buff[sizeof(rf_packet_ll_cs_config_req_t)] = {0};
        rf_packet_ll_cs_config_req_t *req = (rf_packet_ll_cs_config_req_t*)buff;

        req->type = LLID_CONTROL;

        if(pCsParam->cs_config_req & PROC_SEND_RSP){
            req->opcode     = LL_CS_CONFIG_RSP;
            req->Config_ID  = pCsCfg->Config_ID;
            req->rf_len     = sizeof(rf_packet_ll_cs_config_rsp_t) - 2;
            CS_HCI_LOG("[CFG][TX] rsp:0x%x", pCsCfg->state);
        }
        else if(pCsParam->cs_config_req & PROC_SEND_REQ)
        {
            req->rf_len                 = sizeof(rf_packet_ll_cs_config_req_t) - 2;
            req->opcode                 = LL_CS_CONFIG_REQ;
            req->Config_ID              = pCsCfg->Config_ID;
            req->State                  = pCsCfg->state; //enable config
            smemcpy(req->ChM, pCsCfg->Channel_Map, 10);
            req->ChM_Repetition         = pCsCfg->Channel_Map_Repetition;
            req->Main_Mode              = pCsCfg->Main_Mode;
            req->Sub_Mode               = pCsCfg->Sub_Mode;
            req->Main_Mode_Min_Steps    = pCsCfg->Main_Mode_Min_Steps;
            req->Main_Mode_Max_Steps    = pCsCfg->Main_Mode_Max_Steps;
            req->Main_Mode_Repetition   = pCsCfg->Main_Mode_Repetition;
            req->Mode_0_Steps           = pCsCfg->Mode_0_Steps;
            req->CS_SYNC_PHY            = pCsCfg->CS_SYNC_PHY;
            req->RTT_Type               = pCsCfg->RTT_Type & 0x0f;
            req->Role                   = pCsCfg->Role&0x01;
            req->Companion_Signal       = pCsCfg->Companion_Signal_Enable ? 1:0;
            req->ChSel                  = pCsCfg->ChSel?1:0;
            req->Ch3cShape              = pCsCfg->Ch3c_Shape;
            req->Ch3cJump               = pCsCfg->Ch3c_Jump;

            req->T_IP1                  = pCsCfg->T_IP1;
            req->T_IP2                  = pCsCfg->T_IP2;
            req->T_FCS                  = pCsCfg->T_FCS;
            req->T_PM                   = pCsCfg->T_PM;

            //CS_HCI_CMD_CHECK_LOG("[CFG][TX] req:0x%x, %d,%d,%d,%d", pCsCfg->state, req->T_IP1,req->T_IP2,req->T_FCS,req->T_PM );
        }


        if(ll_push_tx_fifo_handler (pAclConn->acl_conHandle | HANDLE_STK_FLAG, (u8 *)req))
        {
            if(pCsParam->cs_config_req & PROC_SEND_RSP)
            {
                pCsParam->cs_config_req = 0;
                pAclConn->ll_rsp_timeout_tick = 0;
                hci_le_csConfigComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8*)pCsCfg);
            }
            else
            {
                pCsParam->cs_config_req &= ~PROC_SEND_REQ;
                pCsParam->cs_config_req |= PROC_WAIT_RSP;
                pAclConn->ll_rsp_timeout_tick = clock_time() | 1;
            }
            pAclConn->csMapMask |=  pCsCfg->idx;
        }
    }

    return BLE_SUCCESS;
}
ble_sts_t blt_ll_cs_exchangeCsStartProc(st_ll_conn_t* pAclConn, cs_config_t *pCsCfg){

    cs_param_t *pCsParam = &pAclConn->csParam;

    if(pCsParam->cs_req == PROC_EVT_PENDING){
        pCsParam->cs_req = 0;
        pCsCfg->cs_procedure_measurement_en = 1;
        hci_le_csProcedureEnableComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle,(u8*) pCsCfg);
    }
    else
    {
        u32 ll_cs_req_pkt_len = sizeof(rf_packet_ll_cs_req_t);
        u32 ll_cs_rsp_pkt_len = sizeof(rf_packet_ll_cs_rsp_t);
        u32 ll_cs_ind_pkt_len = sizeof(rf_packet_ll_cs_ind_t);

        if((ll_cs_rsp_pkt_len > ll_cs_req_pkt_len ) || (ll_cs_ind_pkt_len > ll_cs_req_pkt_len ))
        {
            CS_HCI_LOG("[START] buff len abnormal:0x%x,0x%x,0x%x",ll_cs_req_pkt_len,ll_cs_rsp_pkt_len,ll_cs_ind_pkt_len);
        }
        u8 buff[sizeof(rf_packet_ll_cs_req_t)] = {0};

        if(pCsParam->cs_req & PROC_SEND_REQ)
        {
            rf_packet_ll_cs_req_t *req = (rf_packet_ll_cs_req_t*)buff;

            /* IRQ protect to make sure 2 values are a pair, in case IRQ coming when calculating which lead to timing error
             * use other variable to get and store, to decrease IRQ disabling time */
            u32 r = irq_disable();
            u32 acl_ap_markTick = pAclConn->ap_tick_mark;
            u16 acl_conn_markInst = pAclConn->conn_inst_mark;
            irq_restore(r);

            /* attention that "ap tick mark" maybe a future tick in about 100~200uS, because
             * "ap_tick_mark = bltSche.sSlot_tick_irq + IRQ_BTX_SEND_DELAY_US*SYSTEM_TIMER_TICK_1US " is executed in btx start IRQ, main_loop code
             * may run in 100uS */
            u32 tick_now = clock_time();
            int aclEvent_past;
            if((u32)(acl_ap_markTick - tick_now) < IRQ_BTX_SEND_DELAY_US*SYSTEM_TIMER_TICK_1US){
                aclEvent_past = 0;
            }
            else{
                aclEvent_past = (tick_now - acl_ap_markTick)/pAclConn->conn_intvl_tick;
            }


            if(aclEvent_past > 100){
                write_dbg32(0x0018, aclEvent_past);
                BLMS_ERR_DEBUG(DBG_CS_FLOW_EN, 0x999A0000);
            }

            u8 delta_inter = 40;
            if(pAclConn->conn_intvl_n_1m25 > CONN_INTERVAL_30MS){
                delta_inter = 10;
            }
            else if(pAclConn->conn_intvl_n_1m25 > CONN_INTERVAL_20MS){
                delta_inter = 15;
            }
            else if(pAclConn->conn_intvl_n_1m25 > CONN_INTERVAL_15MS){
                delta_inter = 20;
            }
            else if(pAclConn->conn_intvl_n_1m25 > CONN_INTERVAL_10MS){
                delta_inter = 30;
            }

        #if (DBG_CREATE_FF0D_ERROR)
            int inst_inc = 2;
            u16 future_aclEvtCnt = acl_conn_markInst + inst_inc;
        #else
            int inst_inc = aclEvent_past + delta_inter;
            pCsCfg->connEventCount = acl_conn_markInst + inst_inc;
        #endif

            /*
             * The Offset_Min value shall be greater than or equal to 500 us and less than 4 seconds.
             */
            pCsCfg->offset_min = max2(500, (pAclConn->sSlot_duration*SSLOT_US_NUM + CS_SCH_SET_EARLY_US));
            pCsCfg->offset_max = min2((pAclConn->conn_intvl_n_1m25 *1250 - pCsCfg->offset_min), 4000000);

        #if(DBG_CS_SCH_INIT)
            if(pCsCfg->offset_min > pCsCfg->offset_max){
                write_dbg32(0x0018, pCsCfg->offset_min);
                write_dbg32(0x001C, pCsCfg->offset_max);
                BLMS_ERR_DEBUG(DBG_CS_SCH_INIT, 0xBBff0300);
            }
        #endif

#if 0
            u8 min_ant = (bltCsLocalSupportCap.Num_Antennas_Supported > pAclConn->csRemoteSupCap.Num_Antennas_Supported )? pAclConn->csRemoteSupCap.Num_Antennas_Supported:bltCsLocalSupportCap.Num_Antennas_Supported;
            u8 min_max_ant_path = (bltCsLocalSupportCap.Max_Antenna_Paths_Supported > pAclConn->csRemoteSupCap.Max_Antenna_Paths_Supported )? pAclConn->csRemoteSupCap.Max_Antenna_Paths_Supported:bltCsLocalSupportCap.Max_Antenna_Paths_Supported;
            u8 AntNumA = 1;
            u8 AntNumB = 1;
            if((min_max_ant_path == 4) && (min_ant >=2)){
                AntNumA = 2;
                AntNumB = 2;
            }
            else{
                AntNumA = 1;
                AntNumB = (min_max_ant_path > pAclConn->csRemoteSupCap.Num_Antennas_Supported)?pAclConn->csRemoteSupCap.Num_Antennas_Supported:min_max_ant_path;
            }


            pCsCfg->aci = blt_ll_calculateCsAci(AntNumA,AntNumB);
            pCsCfg->Preferred_Peer_Ant = BIT(0);
#endif
            req->type                   = LLID_CONTROL;
            req->rf_len                 = (u8)(ll_cs_req_pkt_len - 2);
            req->opcode                 = LL_CS_REQ;
            req->Config_ID              = pCsCfg->Config_ID;
            req->connEventCount         = pCsCfg->connEventCount;
            req->Offset_Min[0]          = (u8)(pCsCfg->offset_min & 0xff);
            req->Offset_Min[1]          = (u8)((pCsCfg->offset_min >>8) & 0xff);
            req->Offset_Min[2]          = (u8)((pCsCfg->offset_min >>16) & 0xff);
            req->Offset_Max[0]          = (u8)(pCsCfg->offset_max & 0xff);
            req->Offset_Max[1]          = (u8)((pCsCfg->offset_max >>8) & 0xff);
            req->Offset_Max[2]          = (u8)((pCsCfg->offset_max >>16) & 0xff);
            req->Max_Procedure_Len      = pCsCfg->Max_Procedure_Len;
            req->Event_Interval         = pCsCfg->Event_Interval;
            req->Subevents_Per_Event    = pCsCfg->Subevents_Per_Event;
            req->Subevent_Interval      = pCsCfg->subEvtIntvl_625us;
            req->Subevent_Len[0]        = (u8)(pCsCfg->Subevent_Len & 0xff);
            req->Subevent_Len[1]        = (u8)((pCsCfg->Subevent_Len >>8) & 0xff);
            req->Subevent_Len[2]        = (u8)((pCsCfg->Subevent_Len >>16) & 0xff);
            req->Procedure_Interval     = pCsCfg->Procedure_Interval;
            req->Procedure_Count        = pCsCfg->procMaxCount;
            req->ACI                    = pCsCfg->aci;
            req->Preferred_Peer_Ant     = pCsCfg->Preferred_Peer_Ant;
            req->PHY                    = pCsCfg->PHY;
            req->Pwr_Delta              = pCsCfg->Tx_Pwr_Delta;
        }
        else if(pCsParam->cs_req & PROC_SEND_RSP){
            rf_packet_ll_cs_rsp_t *rsp  = (rf_packet_ll_cs_rsp_t*)buff;
            rsp->type                   = LLID_CONTROL;
            rsp->rf_len                 = ll_cs_rsp_pkt_len - 2;
            rsp->opcode                 = LL_CS_RSP;
            rsp->Config_ID              = pCsCfg->Config_ID;
            rsp->connEventCount         = pCsCfg->connEventCount;
            rsp->Offset_Min[0]          = (u8)(pCsCfg->offset_min & 0xff);
            rsp->Offset_Min[1]          = (u8)((pCsCfg->offset_min >>8) & 0xff);
            rsp->Offset_Min[2]          = (u8)((pCsCfg->offset_min >>16) & 0xff);
            rsp->Offset_Max[0]          = (u8)(pCsCfg->offset_max & 0xff);
            rsp->Offset_Max[1]          = (u8)((pCsCfg->offset_max >>8) & 0xff);
            rsp->Offset_Max[2]          = (u8)((pCsCfg->offset_max >>16) & 0xff);
            rsp->Event_Interval         = pCsCfg->Event_Interval;
            rsp->Subevents_Per_Event    = pCsCfg->Subevents_Per_Event;
            rsp->Subevent_Interval      = pCsCfg->subEvtIntvl_625us;
            rsp->Subevent_Len[0]        = (u8)(pCsCfg->Subevent_Len & 0xff);
            rsp->Subevent_Len[1]        = (u8)((pCsCfg->Subevent_Len >>8) & 0xff);
            rsp->Subevent_Len[2]        = (u8)((pCsCfg->Subevent_Len >>16) & 0xff);
            rsp->ACI                    = pCsCfg->aci;
            rsp->PHY                    = pCsCfg->PHY;
            rsp->Pwr_Delta              = pCsCfg->Tx_Pwr_Delta;
            CS_HCI_LOG("[START][TX] rsp");

        }
        else if(pCsParam->cs_req & PROC_SEND_IND){

            pCsCfg->connEventCount = pAclConn->conn_inst_mark+10; // todo fanqh



            rf_packet_ll_cs_ind_t *ind  = (rf_packet_ll_cs_ind_t*)buff;
            ind->type                   = LLID_CONTROL;
            ind->rf_len                 = ll_cs_ind_pkt_len - 2;
            ind->opcode                 = LL_CS_IND;
            ind->Config_ID              = pCsCfg->Config_ID;
            ind->connEventCount         = pCsCfg->connEventCount;
            ind->Offset[0]              = (u8)(pCsCfg->csOft_us & 0xff);;
            ind->Offset[1]              = (u8)((pCsCfg->csOft_us >>8) & 0xff);
            ind->Offset[2]              = (u8)((pCsCfg->csOft_us >>16) & 0xff);
            ind->Event_Interval         = pCsCfg->Event_Interval;
            ind->Subevents_Per_Event    = pCsCfg->Subevents_Per_Event;
            ind->Subevent_Interval      = pCsCfg->subEvtIntvl_625us;
            ind->Subevent_Len[0]        = (u8)(pCsCfg->Subevent_Len & 0xff);
            ind->Subevent_Len[1]        = (u8)((pCsCfg->Subevent_Len >>8) & 0xff);
            ind->Subevent_Len[2]        = (u8)((pCsCfg->Subevent_Len >>16) & 0xff);
            ind->ACI                    = pCsCfg->aci;
            ind->PHY                    = pCsCfg->PHY;
            ind->Pwr_Delta              = pCsCfg->Tx_Pwr_Delta;

            pCsCfg->sSlotCsDuration = (pCsCfg->Subevent_Len + SLOT_PROCESS_MAX_US)*SSLOT_US_REVERSE + 1;
            pCsCfg->sSlot_csSubIntvl =  BSLOT_DUR_2_SSLOT_DUR(pCsCfg->subEvtIntvl_625us);

            CS_HCI_LOG("[START][TX] ind:0x%x,0x%x",pCsCfg->connEventCount, pCsCfg->csOft_us);
        }



        if(ll_push_tx_fifo_handler (pAclConn->acl_conHandle | HANDLE_STK_FLAG, (u8 *)buff))//blt_llms_pushTxfifo
        {

            if(pCsParam->cs_req & PROC_SEND_REQ){
                pCsParam->cs_req &= ~PROC_SEND_REQ;
                if(pAclConn->aclRole == ACL_ROLE_PERIPHERAL)
                {
                    pCsParam->cs_req |= PROC_WAIT_IND;
                }
                else{//ACL_ROLE_CENTRAL
                    pCsParam->cs_req |= PROC_WAIT_RSP;
                }
                pAclConn->ll_rsp_timeout_tick = clock_time() | 1;
            }
            else if(pCsParam->cs_req & PROC_SEND_RSP)
            {
                pCsParam->cs_req &= ~PROC_SEND_RSP;
                pCsParam->cs_req |= PROC_WAIT_IND;
                pAclConn->ll_rsp_timeout_tick = clock_time() | 1;
            }
            else if(pCsParam->cs_req & PROC_SEND_IND){
                pCsParam->cs_req = 0;
                pCsCfg->cs_procedure_measurement_en = 1;
                pAclConn->ll_rsp_timeout_tick = 0;
                pAclConn->cs_pending |= (pCsCfg->idx | CS_IDX_FLG);

                blt_ll_calcStepDuration(pCsCfg);

                hci_le_csProcedureEnableComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8*)pCsCfg);
            }
        }
    }

    return BLE_SUCCESS;
}
ble_sts_t blt_ll_cs_exchangeCsProcedureRepeatTerminateProc(st_ll_conn_t* pAclConn, cs_config_t *pCsCfg){

    cs_param_t *pCsParam = &pAclConn->csParam;

    if(pCsParam->cs_terminate_ind == PROC_EVT_PENDING){
        pCsCfg->cs_procedure_measurement_en = 0;
        pCsCfg->cs_procedure_en = 0;
        pCsParam->cs_terminate_ind = 0;
        hci_le_csProcedureEnableComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle,(u8*) pCsCfg);
    }
    else
    {
        u8 buff[sizeof(rf_pkt_ll_cs_terminate_ind_t)] = {0};
        rf_pkt_ll_cs_terminate_ind_t *ind   = (rf_pkt_ll_cs_terminate_ind_t*)buff;
        if(pCsParam->cs_terminate_ind & PROC_SEND_IND)
        {
            ind->type                   = LLID_CONTROL;
            ind->rf_len                 = sizeof(rf_pkt_ll_cs_terminate_ind_t) - 2;
            ind->opcode                 = LL_CS_TERMINATE_IND;
            ind->Config_ID              = pCsCfg->Config_ID;
            ind->ErrorCode              = 0x0c;//todo TBD
            CS_HCI_LOG("[TERMIN][TX] terminate ind:0x%x",ind->Config_ID);
        }

        if(ll_push_tx_fifo_handler (pAclConn->acl_conHandle | HANDLE_STK_FLAG, (u8 *)buff))//blt_llms_pushTxfifo
        {
            pCsParam->cs_terminate_ind = 0;
            pCsCfg->cs_procedure_measurement_en = 0;
            pCsCfg->cs_procedure_en = 0;
            //pAclConn->cs_pending |= (pCsCfg->idx | CS_IDX_FLG); todo
            hci_le_csProcedureEnableComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8*)pCsCfg);

        }
    }

    return BLE_SUCCESS;
}

void   blt_cs_chnMapAndOperate(u8 *chn_out, u8 *chnM){

    for(u8 i = 0; i< 10; i++){
        chn_out[i] &= chnM[i];
    }

}

ble_sts_t blt_ll_cs_chnMapUpdateProce(void){

    ble_sts_t ret = BLE_SUCCESS;
    u8 buff[sizeof(rf_pkt_ll_cs_chn_map_ind_t)];
    rf_pkt_ll_cs_chn_map_ind_t *chm_ind = (rf_pkt_ll_cs_chn_map_ind_t*) buff;

    chm_ind->type                               = LLID_CONTROL;
    chm_ind->rf_len                             = sizeof(rf_pkt_ll_cs_chn_map_ind_t) - 2;

    memcpy(chm_ind->ChM, gCsMng.chn_map, 10);
    chm_ind->opcode = LL_CS_CHANNEL_MAP_IND;
    CS_HCI_LOG("[CHNL][TX] chnl map ind");



    for(u8 i = 0 ; i<gCsMng.max_num_cofig; i++)
    {
        cs_config_t *pCfg = gGlobal_pCsCfg + i;
        if(pCfg->state)
        {
            st_ll_conn_t *pAclConn = (st_ll_conn_t*)(u32)&blms[pCfg->aclHandle & CONN_IDX_MASK];

            blt_cs_chnMapAndOperate(pCfg->Channel_Map, gCsMng.chn_map);

            pCfg->chn_update_pend = 1;
            if(pCfg->cs_procedure_measurement_en)
            {
                pCfg->chn_update_inst = pCfg->connEventCount + pCfg->Procedure_Interval;
                chm_ind->instant = pCfg->chn_update_inst;

            }
            else
            {
//              pCfg->chn_update_pend = 1;
                pCfg->chn_update_inst = pAclConn->conn_inst + pCfg->Procedure_Interval;
                chm_ind->instant = pCfg->chn_update_inst;
//              pCfg->Chn_en_num  = blt_cs_extractEnableChnMap(pCfg->Channel_Map, pCfg->filteredChnArray);
            }
            gCsMng.chn_map_upt_tick = 0;
            ll_push_tx_fifo_handler (pAclConn->acl_conHandle | HANDLE_STK_FLAG, (u8 *)chm_ind);
        }
    }
    return ret;

}


//refer Channel sounding CRr10 P72
int blt_ll_cs_checkConfigParam(st_ll_conn_t* pAclConn, u8 opcode, u8 *raw){
    (void)pAclConn;//clean warning: unused variable 'pAclConn' [-Wunused-variable] by SunWei
    (void)opcode;//clean warning: unused variable 'opcode' [-Wunused-variable] by SunWei
    rf_packet_ll_cs_config_req_t *pConfigReq = (rf_packet_ll_cs_config_req_t*)raw;
    // remote or local don't support mode3

    if((pConfigReq->Config_ID>3) || (pConfigReq->Main_Mode>3) || (pConfigReq->Main_Mode == 0)
            || ((pConfigReq->Sub_Mode>3) && (pConfigReq->Sub_Mode!=0xff))|| (pConfigReq->Sub_Mode==0))
    {
        CS_HCI_LOG("[CHK_CFG_P] Config_ID abnormal:0x%x,0x%x,0x%x",pConfigReq->Config_ID,pConfigReq->Main_Mode,pConfigReq->Sub_Mode);
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    chn_sound_capbilities_t *csLocalCap = &bltCsLocalSupportCap;

    if(!(csLocalCap->Mode_Types & BIT(0)))//local not support mode3
    {
        if((pConfigReq->Main_Mode ==0x03)||  (pConfigReq->Sub_Mode==0x03))
        {
            CS_HCI_LOG("[CHK_CFG_P] local not support mode3:0x%x,0x%x",pConfigReq->Main_Mode,pConfigReq->Sub_Mode);
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
    }

    if((pConfigReq->Main_Mode_Max_Steps == 0) || (pConfigReq->Main_Mode_Min_Steps == 0) ){
        CS_HCI_LOG("[CHK_CFG_P] Main_Mode step abnormal:0x%x,0x%x",pConfigReq->Main_Mode_Max_Steps,pConfigReq->Main_Mode_Min_Steps);
//      return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    if((pConfigReq->Main_Mode_Repetition>3) || (pConfigReq->Mode_0_Steps == 0) || (pConfigReq->Mode_0_Steps>3)
                    || (pConfigReq->Role>1) || (pConfigReq->RTT_Type>6) ||(pConfigReq->CS_SYNC_PHY == 0)
                    || (pConfigReq->CS_SYNC_PHY>2) || (pConfigReq->ChSel>1) || (pConfigReq->Ch3cShape>1)){
        CS_HCI_LOG("[CHK_CFG_P] role abnormal:0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x",pConfigReq->Main_Mode_Repetition,pConfigReq->Mode_0_Steps,pConfigReq->Role,\
                pConfigReq->RTT_Type,pConfigReq->CS_SYNC_PHY,pConfigReq->ChSel,pConfigReq->Ch3cShape);
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    if((pConfigReq->Ch3cJump <2 || pConfigReq->Ch3cJump>8) || (pConfigReq->Companion_Signal>1)){
        CS_HCI_LOG("[CHK_CFG_P] Ch3cJump abnormal:0x%x,0x%x",pConfigReq->Ch3cJump,pConfigReq->Companion_Signal);
//      return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }
    if(pConfigReq->ChM_Repetition == 0) {
        CS_HCI_LOG("[CHK_CFG_P] chn num abnormal:%d",pConfigReq->ChM_Repetition);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    return 0;
}

int blt_ll_cs_checkCsreqParam(st_ll_conn_t* pAclConn, u8 opcode, u8 *raw,cs_config_t *pCsCfg ){
    (void)opcode;//clean warning: unused variable 'opcode' [-Wunused-variable] by SunWei
    rf_packet_ll_cs_req_t *pConfigReq = (rf_packet_ll_cs_req_t*)raw;

    u32 off_min = pConfigReq->Offset_Min[0]|(pConfigReq->Offset_Min[1]<<8)|(pConfigReq->Offset_Min[2]<<16);
    u32 off_max = pConfigReq->Offset_Max[0]|(pConfigReq->Offset_Max[1]<<8)|(pConfigReq->Offset_Max[2]<<16);
    if((500 > off_min) || (off_min >= 4000*1000)){//500<=offset_min <4000*1000 us
        CS_HCI_LOG("[CHK_CS_REQ]offset abnormal:0x%x",off_min);
        return 0xff;
    }

    if((off_max < off_min) || (off_max >= (pAclConn->conn_intvl_n_1m25 * 1250))){//offset_min<= offset_max < le connection interval
        CS_HCI_LOG("[CHK_CS_REQ]conn_interval abnormal:0x%x,0x%x,0x%x",off_min,off_max,pAclConn->conn_intvl_n_1m25);
        return 0xfe;
    }

    if(pConfigReq->Event_Interval <1){//1<= event_interval <= 65535
        CS_HCI_LOG("[CHK_CS_REQ]Event_Interval abnormal:0x%x",pConfigReq->Event_Interval);
        return 0xfd;
    }

    if(pConfigReq->Subevents_Per_Event <1){//subevent_per_event >=1
        CS_HCI_LOG("[CHK_CS_REQ]Per_Event abnormal:0x%x",pConfigReq->Subevents_Per_Event);
        return 0xfc;
    }

    if(pConfigReq->Subevent_Interval <=1){//subevent interval if(subevent_per_event >1){valid}else{ be set to 0}
        pConfigReq->Subevent_Interval = 0;
    }

    u32 subevent_len = (pConfigReq->Subevent_Len[0] | (pConfigReq->Subevent_Len[1]<<8) | (pConfigReq->Subevent_Len[2]<<16));
    if((1250 > subevent_len) || (subevent_len >= 4000*1000)){// 1250<=subevent len < 4000*1000
        CS_HCI_LOG("[CHK_CS_REQ]subevent_len abnormal:0x%x",subevent_len);
        return 0xfb;
    }

    if(pConfigReq->Preferred_Peer_Ant & 0xf0){//Preferred_Peer_Ant  bit0--3 valid
        CS_HCI_LOG("[CHK_CS_REQ]Preferred_Peer_Ant abnormal:0x%x",pConfigReq->Preferred_Peer_Ant);
        return 0xfa;
    }

    if(pConfigReq->PHY != pCsCfg->CS_SYNC_PHY){
        CS_HCI_LOG("[CHK_CS_REQ]PHY abnormal:0x%x,0x%x",pConfigReq->PHY,pCsCfg->CS_SYNC_PHY);
        return 0xf9;
    }

    if(pConfigReq->ACI & 0xf0){//0-7
        CS_HCI_LOG("[CHK_CS_REQ]ACI abnormal:0x%x",pConfigReq->ACI);
        return 0xf8;
    }
    return 0;

}


int blt_ll_cs_ctrl_pdu_proc(st_ll_conn_t* pAclConn, u8 opcode, u8 *raw){

    cs_param_t *pCsParam = &pAclConn->csParam;
#if OS_SUP_EN
    if(blt_os_giveSem_cb)
    {
        blt_os_giveSem_cb();
    }
#endif
    if(opcode == LL_REJECT_IND_EXT){
        rf_packet_ll_reject_ext_ind_t* pRejectExtInd = (rf_packet_ll_reject_ext_ind_t*)raw;
        CS_HCI_LOG("[REJECT][RX]opcode:0x%x,errCode:0x%x",pRejectExtInd->rejectOpcode,pRejectExtInd->errCode);
        if((pRejectExtInd->rejectOpcode == LL_CS_SEC_REQ) || (pRejectExtInd->rejectOpcode == LL_CS_SEC_RSP)){
            pCsParam->cs_security_enable = 0;
            CS_HCI_LOG("[REJECT][RX]sec exch fail");
        }
        else if((pRejectExtInd->rejectOpcode == LL_CS_CAPABILITIES_REQ) || (pRejectExtInd->rejectOpcode == LL_CS_CAPABILITIES_RSP)){
            pCsParam->cs_cap_req = 0;
            CS_HCI_LOG("[REJECT][RX]cap exch fail");
        }
        else if((pRejectExtInd->rejectOpcode == LL_CS_CONFIG_REQ) || (pRejectExtInd->rejectOpcode == LL_CS_CONFIG_RSP)){
            pCsParam->cs_config_req = 0;
            //pCsCfg->state = 0; todo
            CS_HCI_LOG("[REJECT][RX]cfg exch fail");
        }
        else if((pRejectExtInd->rejectOpcode == LL_CS_REQ) || (pRejectExtInd->rejectOpcode == LL_CS_RSP)|| (pRejectExtInd->rejectOpcode == LL_CS_IND)){
            pCsParam->cs_req = 0;
            CS_HCI_LOG("[REJECT][RX]cs start exch fail");
        }
        else if((pRejectExtInd->rejectOpcode == LL_CS_FAE_REQ) || (pRejectExtInd->rejectOpcode == LL_CS_FAE_RSP)){
            pCsParam->cs_fae_req = 0;
            CS_HCI_LOG("[REJECT][RX]fae exch fail");
        }
        else if(pRejectExtInd->rejectOpcode == LL_CS_TERMINATE_IND){
            pCsParam->cs_terminate_ind = 0;
            CS_HCI_LOG("[REJECT][RX]terminate fail");
        }

    }
    else if(opcode == LL_CS_SEC_REQ)
    {
        rf_pkt_ll_cs_sec_req_t *pSecReq = (rf_pkt_ll_cs_sec_req_t*)raw;

        CS_HCI_LOG("[SEC][RX] req");
        /*
         * If the remote Link Layer sends an LL_CS_SEC_REQ PDU
         * when the Channel Sounding (Host Support) feature bit is not set in the local Link Layer, the local Link
         * Layer shall send an LL_REJECT_EXT_IND PDU with the error code Unsupported Remote Feature /
         * Unsupported LMP Feature (0x1A)
         * */
        if(!(LL_FEATURE_MASK_1 & LL_FEATURE_MASK_CHANNEL_SOUNDING_HOST)){
            CS_HCI_LOG("[SEC] host not support");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_SEC_REQ, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }
        /*
         *  If the remote Link Layer sends an
         *  LL_CS_SEC_REQ PDU without the Encryption Start procedure having successfully completed, the local
         *  Link Layer shall send an LL_REJECT_EXT_IND PDU with the error code Insufficient Security (0x2F).
         */
        if(0 == pAclConn->crypt.enable){
            CS_HCI_LOG("[SEC] enc not complete");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_SEC_REQ, HCI_ERR_INSUFFICIENT_SECURITY, 1);
        }

        smemcpy(&pCsParam->CS_IV_C[0],&pSecReq->CS_IV_C[0],8);
        smemcpy(&pCsParam->CS_IN_C[0],&pSecReq->CS_IN_C[0],4);
        smemcpy(&pCsParam->CS_PV_C[0],&pSecReq->CS_PV_C[0],8);

        pCsParam->cs_security_enable |= (PROC_SEND_RSP | PROC_EVT_PENDING);


    }
    else if(opcode == LL_CS_SEC_RSP)
    {
        rf_pkt_ll_cs_sec_rsp_t *pSecRsp = (rf_pkt_ll_cs_sec_rsp_t*)raw;
        CS_HCI_LOG("[SEC][RX] rsp");
        if(!(pCsParam->cs_security_enable & PROC_WAIT_RSP)){
            pCsParam->cs_security_enable = 0;
            CS_HCI_LOG("[SEC] proc abnormal:0x%x",pCsParam->cs_security_enable);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_FAE_RSP, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }

        smemcpy(&pCsParam->CS_IV_P[0],&pSecRsp->CS_IV_P[0],8);
        smemcpy(&pCsParam->CS_IN_P[0],&pSecRsp->CS_IN_P[0],4);
        smemcpy(&pCsParam->CS_PV_P[0],&pSecRsp->CS_PV_P[0],8);

        blt_ll_cs_buffCombize(&pCsParam->CS_IV_C[0],&pCsParam->CS_IV_P[0],&pCsParam->CS_IV[0],8);
        blt_ll_cs_buffCombize(&pCsParam->CS_IN_C[0],&pCsParam->CS_IN_P[0],&pCsParam->CS_IN[0],4);
        blt_ll_cs_buffCombize(&pCsParam->CS_PV_C[0],&pCsParam->CS_PV_P[0],&pCsParam->CS_PV[0],8);

        if(pCsParam->cs_security_enable & PROC_EVT_PENDING){
            hci_le_csSecurityEnableComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle);
        }

        pCsParam->cs_security_exchange = 1;
        pCsParam->cs_security_enable = 0;
        pAclConn->ll_rsp_timeout_tick = 0;

        drbg_instantiation_func_h9(pCsParam->CS_IV, pCsParam->CS_IN,pCsParam->CS_PV, kdrbg_global, vdrbg_global);///////////
        cs_drbg_init();
    }
    else if(opcode == LL_CS_CAPABILITIES_REQ){
        CS_HCI_LOG("[CAP][RX] req");
        rf_packet_ll_cs_cap_req_t *pReq = (rf_packet_ll_cs_cap_req_t*)raw;
        chn_sound_capbilities_t *pCap = &pAclConn->csRemoteSupCap;

        if(!(LL_FEATURE_MASK_1 & LL_FEATURE_MASK_CHANNEL_SOUNDING_HOST)){
            CS_HCI_LOG("[CAP] host not support");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_CAPABILITIES_REQ, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }

        pCap->Num_Config_Supported                  = pReq->Num_Configs;
        pCap->max_consecutive_procedures_supported  = pReq->Max_Procedures_Supported;
        pCap->Num_Antennas_Supported                = pReq->Num_Ant;
        pCap->Max_Antenna_Paths_Supported           = pReq->Max_Ant_Path;
        pCap->Roles_Supported                       = pReq->Role;
        pCap->Mode_Types                            = pReq->Mode_Types;
        pCap->RTT_Capability                        = pReq->RTT_Capability;
        pCap->RTT_AA_Only_N                         = pReq->RTT_AA_Only_N;
        pCap->RTT_Sounding_N                        = pReq->RTT_Sounding_N;
        pCap->RTT_Random_Payload_N                  = pReq->RTT_Random_Sequence_N;
        pCap->Optional_NADM_Sounding_Capability     = pReq->NADM_Sounding_Capability;
        pCap->Optional_NADM_Random_Capability       = pReq->NADM_Random_Sequence_Capability;
        pCap->Optional_Subfeatures_Supported        = (pReq->Companion_Signal) | (pReq->No_FAE<<1) \
                                                        | (pReq->chn_sel_3c<<2) | (pReq->Sounding_PCT_Estimate<<3);
        pCap->Optional_T_IP1_Times_Supported        = pReq->T_IP1_Capability;
        pCap->Optional_T_IP2_Times_Supported        = pReq->T_IP2_Capability;
        pCap->Optional_T_FCS_Times_Supported        = pReq->T_FCS_Capability;
        pCap->Optional_T_FCS_Times_Supported        = pReq->T_FCS_Capability;
        pCap->Optional_T_PM_Times_Supported         = pReq->T_PM_Capability;
        pCap->T_SW_Time_Supported                   = pReq->T_SW;

        pCsParam->cs_cap_req                       |= (PROC_SEND_RSP | PROC_EVT_PENDING);


    }
    else if(opcode == LL_CS_CAPABILITIES_RSP)
    {
        CS_HCI_LOG("[CAP][RX] rsp");
        if(!(pCsParam->cs_cap_req & PROC_WAIT_RSP)){
            pCsParam->cs_cap_req = 0;
            CS_HCI_LOG("[CAP] proc abnormal:0x%x",pCsParam->cs_cap_req);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_CAPABILITIES_RSP, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }

        rf_packet_ll_cs_cap_req_t *pReq             = (rf_packet_ll_cs_cap_req_t*)raw;
        chn_sound_capbilities_t *pCap               = &pAclConn->csRemoteSupCap;

        pCap->Num_Config_Supported                  = pReq->Num_Configs;
        pCap->max_consecutive_procedures_supported  = pReq->Max_Procedures_Supported;
        pCap->Num_Antennas_Supported                = pReq->Num_Ant;
        pCap->Max_Antenna_Paths_Supported           = pReq->Max_Ant_Path;
        pCap->Roles_Supported                       = pReq->Role;
        pCap->Mode_Types                            = pReq->Mode_Types;
        pCap->RTT_Capability                        = pReq->RTT_Capability;
        pCap->RTT_AA_Only_N                         = pReq->RTT_AA_Only_N;
        pCap->RTT_Sounding_N                        = pReq->RTT_Sounding_N;
        pCap->RTT_Random_Payload_N                  = pReq->RTT_Random_Sequence_N;
        pCap->Optional_NADM_Sounding_Capability     = pReq->NADM_Sounding_Capability;
        pCap->Optional_NADM_Random_Capability       = pReq->NADM_Random_Sequence_Capability;
        pCap->Optional_Subfeatures_Supported        = (pReq->Companion_Signal) | (pReq->No_FAE<<1) \
                                                        | (pReq->chn_sel_3c<<2) | (pReq->Sounding_PCT_Estimate<<3);
        pCap->Optional_T_IP1_Times_Supported        = pReq->T_IP1_Capability;
        pCap->Optional_T_IP2_Times_Supported        = pReq->T_IP2_Capability;
        pCap->Optional_T_FCS_Times_Supported        = pReq->T_FCS_Capability;
        pCap->Optional_T_PM_Times_Supported         = pReq->T_PM_Capability;
        pCap->T_SW_Time_Supported                   = pReq->T_SW;

        if(pCsParam->cs_cap_req & PROC_EVT_PENDING)
        {
            hci_le_readRemoteSupCapComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle,(u8*)pCap);
        }
        pCsParam->cs_cap_exchange = 1;
        pAclConn->ll_rsp_timeout_tick = 0;
        pCsParam->cs_cap_req = 0;
    }

    else if(opcode == LL_CS_CONFIG_REQ){
        rf_packet_ll_cs_config_req_t *pConfigReq = (rf_packet_ll_cs_config_req_t*)raw;

        CS_HCI_LOG("[CFG][RX] req");
        /*
         * 1. If the remote Link Layer sends an LL_CS_CONFIG_REQ PDU when the Channel Sounding
         * (Host Support) feature bit is not set in the local Link Layer, then the local Link
         * Layer shall send an LL_REJECT_EXT_IND PDU
         */
        if(!(LL_FEATURE_MASK_1 & LL_FEATURE_MASK_CHANNEL_SOUNDING_HOST)){
            CS_HCI_LOG("[CFG] host not support");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_CONFIG_REQ, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }

        /*      1. save configReq
         *      2. If the parameters received in an LL_CS_CONFIG_REQ PDU are not acceptable to that Link Layer,
         *      then it shall immediately reject the configuration parameter set with an LL_REJECT_EXT_IND PDU
         *      with the error code Unsupported LL Parameter Value (0x20)
         */
        u8 ret = blt_ll_cs_checkConfigParam(pAclConn,opcode,raw);
        if(ret != BLE_SUCCESS){
            CS_HCI_LOG("[CFG] param check fail:0x%x",ret);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_CONFIG_REQ, HCI_ERR_UNSUPPORTED_LMP_PARAM_VAL, 1);
        }

        u8 cfgIdx = blt_ll_getCsConfigById(pAclConn->acl_conHandle,pConfigReq->Config_ID);
        if( cfgIdx == 0xff){
            cfgIdx = blt_ll_getNewCsConfig();
            if(cfgIdx == 0xff){
                CS_HCI_LOG("[CFG] create cfg fail:0x%x",cfgIdx);
                return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_CONFIG_REQ, HCI_ERR_CONN_REJ_LIMITED_RESOURCES, 1);
            }
            CS_HCI_LOG("[CFG] create cfg id:0x%x",cfgIdx);
        }
        else{
            CS_HCI_LOG("[CFG] cfg id is exist:0x%x",cfgIdx);
        }

        cs_config_t *pCsCfg             = gGlobal_pCsCfg + cfgIdx;
        pCsCfg->Chn_en_num = blt_cs_extractEnableChnMap(pConfigReq->ChM, pCsCfg->filteredChnArray);
        if((pCsCfg->Chn_en_num < 15)){
            CS_HCI_LOG("[CFG] chnMap Num Error:0x%x",pCsCfg->Chn_en_num);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_CONFIG_REQ, HCI_ERR_UNSUPPORTED_LMP_PARAM_VAL, 1);
        }




        pCsCfg->Config_ID               = pConfigReq->Config_ID;
        pCsCfg->state                   = pConfigReq->State;
        smemcpy(pCsCfg->Channel_Map, pConfigReq->ChM, 10);
        pCsCfg->Channel_Map_Repetition  =pConfigReq->ChM_Repetition;
        pCsCfg->Main_Mode               = pConfigReq->Main_Mode;
        pCsCfg->Sub_Mode                = pConfigReq->Sub_Mode;
        pCsCfg->Main_Mode_Min_Steps     = pConfigReq->Main_Mode_Min_Steps;
        pCsCfg->Main_Mode_Max_Steps     = pConfigReq->Main_Mode_Max_Steps;
        pCsCfg->Main_Mode_Repetition    = pConfigReq->Main_Mode_Repetition;
        pCsCfg->Mode_0_Steps            = pConfigReq->Mode_0_Steps;
        pCsCfg->CS_SYNC_PHY             = pConfigReq->CS_SYNC_PHY;
        pCsCfg->RTT_Type                = pConfigReq->RTT_Type;
        pCsCfg->Role                    = !pConfigReq->Role;
        pCsCfg->Companion_Signal_Enable = pConfigReq->Companion_Signal;
        pCsCfg->ChSel                   = pConfigReq->ChSel;
        pCsCfg->Ch3c_Shape              = pConfigReq->Ch3cShape;
        pCsCfg->Ch3c_Jump               = pConfigReq->Ch3cJump;
        pCsCfg->T_IP1                   = pConfigReq->T_IP1;
        pCsCfg->T_IP2                   = pConfigReq->T_IP2;
        pCsCfg->T_FCS                   = pConfigReq->T_FCS;
        pCsCfg->T_PM                    = pConfigReq->T_PM;

        pCsCfg->occupy                  = pCsCfg->state;
//      pCsCfg->idx                     = cfgIdx;
        pCsCfg->aclHandle               = pAclConn->acl_conHandle;
        pAclConn->csMapMask |=  BIT(pCsCfg->idx);

//      tlkapi_send_string_u32s(1, "rec_cs_config_req", pAclConn->csMapMask);


#if(CS_IOP_EN)
        if(pCsCfg->Sub_Mode!=0xff){
            tlkapi_send_string_data(1,"submode non-supported", &pCsCfg->Sub_Mode, 1);
            BLMS_ERR_DEBUG(1, 0xBBff1100);
        }
//      if(pCsCfg->Main_Mode_Repetition != 0){
//          tlkapi_send_string_data(1,"Main_Mode_Repetition non-supported", &pCsCfg->Main_Mode_Repetition, 1);
//          BLMS_ERR_DEBUG(1, 0xBBff1200);
//      }

#endif

        pCsParam->cs_config_req         |= (PROC_SEND_RSP | PROC_EVT_PENDING);
        pCsParam->cs_config_pend_idx    = pCsCfg->idx;

        pCsCfg->PHY                     = pCsCfg->CS_SYNC_PHY;//todo by biao & qinghua


    }
    else if(opcode==LL_CS_CONFIG_RSP){
        CS_HCI_LOG("[CFG][RX] rsp");
        rf_packet_ll_cs_config_rsp_t *pConfigRsp = (rf_packet_ll_cs_config_rsp_t*)raw;
        cs_config_t *pCsCfg = gGlobal_pCsCfg + pAclConn->csParam.cs_config_pend_idx;

        /*
         * 1. The value of the Config_ID parameter returned in the LL_CS_CONFIG_RSP PDU shall be the same as
         *    the value received in the LL_CS_CONFIG_REQ PDU
         */
        if(pCsCfg->Config_ID != pConfigRsp->Config_ID){
            pCsCfg->state = 0;
            pCsParam->cs_config_req = 0;
            CS_HCI_LOG("[CFG] cfg id is not consist:0x%x",pConfigRsp->Config_ID);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_CONFIG_RSP, HCI_ERR_UNSUPPORTED_LMP_PARAM_VAL, 1);
        }

        if(!(pCsParam->cs_config_req & PROC_WAIT_RSP)){
            pCsCfg->state = 0;
            pCsParam->cs_config_req = 0;
            CS_HCI_LOG("[CFG] proc abnormal:0x%x",pCsParam->cs_config_req);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_CONFIG_RSP, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }


        if(pCsParam->cs_config_req & PROC_EVT_PENDING)
        {
            hci_le_csConfigComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8*)pCsCfg);
        }

        pCsParam->cs_config_req  = 0;
        pAclConn->ll_rsp_timeout_tick = 0;
        pCsCfg->occupy = pCsCfg->state;
    }
    else if(opcode==LL_CS_REQ){
        CS_HCI_LOG("[START][RX] req");
        rf_packet_ll_cs_req_t *pCsReq = (rf_packet_ll_cs_req_t*)raw;
        /* If the remote Link Layer sends an
        LL_CS_REQ PDU when the Channel Sounding (Host Support) feature bit is not set in the local Link
        Layer, the local Link Layer shall send an LL_REJECT_EXT_IND PDU with the error code Unsupported
        Remote Feature / Unsupported LMP Feature (0x1A).*/
        if(!(LL_FEATURE_MASK_1 & LL_FEATURE_MASK_CHANNEL_SOUNDING_HOST)){
            CS_HCI_LOG("[START] host not support");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_REQ, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }
        /* If the CS configuration ID received during the CS Start procedure does not exist or is
        otherwise not properly enabled, then the receiving Link Layer shall immediately respond with an
        LL_REJECT_EXT_IND PDU with the error code Invalid LL Parameters (0x1E).*/

        u8 cfgIdx = blt_ll_getCsConfigById(pAclConn->acl_conHandle, pCsReq->Config_ID);
        if(cfgIdx == 0xff){
            CS_HCI_LOG("[START] config not exist:0x%x,0x%x",pAclConn->acl_conHandle,pCsReq->Config_ID);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_REQ, HCI_ERR_CONN_REJ_LIMITED_RESOURCES, 1);
        }
        cs_config_t *pCsCfg = gGlobal_pCsCfg + cfgIdx;
        if(pCsCfg->state == 0){//todo  not properly enabled
            CS_HCI_LOG("[START] config not enable");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_REQ, HCI_ERR_CONN_REJ_LIMITED_RESOURCES, 1);
        }

        /* If the parameters received in an LL_CS_REQ PDU are not acceptable to the receiving Link Layer
         (Central or Peripheral), then it shall immediately reject the procedure sending a LL_REJECT_EXT_IND
         PDU with the appropriate error code.*/
        int ret = blt_ll_cs_checkCsreqParam(pAclConn,opcode,raw,pCsCfg);
        if(ret){
            CS_HCI_LOG("[START] param check fail:0x%x",ret);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_REQ, HCI_ERR_UNSUPPORTED_LMP_PARAM_VAL, 1);
        }

        pCsCfg->aci = pCsReq->ACI;

        if(pAclConn->aclRole == ACL_ROLE_CENTRAL){
            pCsParam->cs_req = (PROC_SEND_IND | PROC_EVT_PENDING);
            //todo ind packet calculate
        }
        else{
            pCsParam->cs_req = (PROC_SEND_RSP | PROC_EVT_PENDING);

            pCsCfg->connEventCount = pCsReq->connEventCount;



            pCsCfg->Max_Procedure_Len = pCsReq->Max_Procedure_Len;
            u32 subevent_len = pCsReq->Subevent_Len[0] | (pCsReq->Subevent_Len[1]<<8) | (pCsReq->Subevent_Len[2]<<16);

//           if(subevent_len >3800){//3.8ms, case1:(m0=2, m2=6) case2:(m0=1, m2=7)
//               subevent_len = 3800;
//           }
             pCsCfg->Subevent_Len = min2(subevent_len, pAclConn->conn_intvl_n_1m25*1250/3 * 2);;
             pCsCfg->subEvtIntvl_625us = max(pCsReq->Subevent_Interval, (pCsCfg->Subevent_Len + TLK_T_MES )/625 + 5);
            /*
             * 1. The Subevent_Interval shall be greater than or equal to the sum of the Subevent_Len selected plus
             *    T_MES. A Controller shall be capable of supporting a minimum Subevent_Len of 2.5 ms.
             * 2. The value of Subevent_Len supplied in either the LL_CS_RSP or LL_CS_IND PDU shall be less than or equal to the
             * value received in the LL_CS_REQ or LL_CS_RSP PDU that is being responded to
             */
            pCsCfg->Event_Interval = pCsReq->Event_Interval;
            pCsCfg->Subevents_Per_Event = min(pCsReq->Subevents_Per_Event, (pAclConn->conn_intvl_n_1m25*1250/3 * 2)/(pCsCfg->subEvtIntvl_625us*625) -1);

            /*
             * The Offset_Min value shall be greater than or equal to 500 us and less than 4 seconds.
             */
            u32 ofst_min = max2(500, (pAclConn->sSlot_duration*SSLOT_US_NUM + CS_SCH_SET_EARLY_US));
            u32 ofst_max = pAclConn->conn_intvl_n_1m25*1250 - pCsCfg->Subevent_Len - CS_SCH_SET_EARLY_US;

            pCsCfg->offset_min = max2(ofst_min, (u32)(pCsReq->Offset_Min[0] | (pCsReq->Offset_Min[1]<<8) | (pCsReq->Offset_Min[2]<<16)));
            pCsCfg->offset_max = min2(ofst_max, (u32)(pCsReq->Offset_Max[0] | (pCsReq->Offset_Max[1]<<8) | (pCsReq->Offset_Max[2]<<16)));

            pCsCfg->Procedure_Interval = pCsReq->Procedure_Interval;
            pCsCfg->procMaxCount = pCsReq->Procedure_Count;
        }

        pCsCfg->cs_procedure_en = 1;

    }
    else if(opcode==LL_CS_RSP){
        rf_packet_ll_cs_rsp_t *pCsRsp = (rf_packet_ll_cs_rsp_t*)raw;
        cs_config_t *pCsCfg = gGlobal_pCsCfg + pCsParam->cs_pend_idx;
        if(!(pCsParam->cs_req & PROC_WAIT_RSP)){
            pCsCfg->cs_procedure_en = 0;
            pCsParam->cs_req = 0;
            CS_HCI_LOG("[START] proc abnormal:0x%x",pCsParam->cs_req);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_RSP, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }

        u8 cfgIdx = blt_ll_getCsConfigById(pAclConn->acl_conHandle, pCsRsp->Config_ID);
        if(cfgIdx == 0xff){
            pCsCfg->cs_procedure_en = 0;
            pCsParam->cs_req = 0;
            CS_HCI_LOG("[START] config not exist:0x%x,0x%x",pAclConn->acl_conHandle, pCsRsp->Config_ID);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_RSP, HCI_ERR_CONN_REJ_LIMITED_RESOURCES, 1);
        }

        u32 min = pCsRsp->Offset_Min[0] | (pCsRsp->Offset_Min[1]<<8) | (pCsRsp->Offset_Min[2]<<16);
        u32 max = pCsRsp->Offset_Max[0] | (pCsRsp->Offset_Max[1]<<8) | (pCsRsp->Offset_Max[2]<<16);

        if((min < pCsCfg->offset_min) || (max > pCsCfg->offset_max)){
            pCsCfg->cs_procedure_en = 0;
            CS_HCI_LOG("[START] offset invalid:0x%x,0x%x,0x%x,0x%x",  pCsCfg->offset_min,pCsCfg->offset_max,max,min);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_RSP, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }

        u8 aci = blt_ll_checkCsAci(pCsCfg->aci,pCsRsp->ACI);
        if(aci == 0xff){
            pCsCfg->cs_procedure_en = 0;
            pCsParam->cs_req = 0;
            CS_HCI_LOG("[START] aci invalid:0x%x,0x%x,0x%x", pAclConn->acl_conHandle,pCsCfg->aci,pCsRsp->ACI);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_RSP, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }
        else{
            pCsCfg->aci = pCsRsp->ACI;
        }


        pCsCfg->connEventCount = pCsRsp->connEventCount;

        pCsCfg->csOft_us = min;
        pCsCfg->Subevent_Len = pCsRsp->Subevent_Len[0] | ( pCsRsp->Subevent_Len[1]<<8) | ( pCsRsp->Subevent_Len[2]<<16);

        pCsParam->cs_req &=~PROC_WAIT_RSP;
        pCsParam->cs_req |= PROC_SEND_IND ;
        //todo ind packet calculate
    }
    else if(opcode==LL_CS_IND){
        rf_packet_ll_cs_ind_t *pCsInd = (rf_packet_ll_cs_ind_t*)raw;
        cs_config_t *pCsCfg = gGlobal_pCsCfg + pCsParam->cs_pend_idx;
        if(!(pCsParam->cs_req & PROC_WAIT_IND)){
            pCsCfg->cs_procedure_en = 0;
            pCsParam->cs_req = 0;
            CS_HCI_LOG("[START] proc abnormal:0x%x",pCsParam->cs_req);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_IND, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }
        if(pCsParam->cs_req & PROC_EVT_PENDING)
        {
            hci_le_csProcedureEnableComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle,(u8*) pCsCfg);
        }
        pAclConn->ll_rsp_timeout_tick = 0;
        pCsParam->cs_req = 0;
        pCsCfg->cs_procedure_measurement_en = 1;
        pAclConn->cs_pending |= (pCsCfg->idx | CS_IDX_FLG);

        pCsCfg->connEventCount = pCsInd->connEventCount;
        pCsCfg->csOft_us = pCsInd->Offset[0] | pCsInd->Offset[1]<<8 | pCsInd->Offset[2]<<16;
        pCsCfg-> Event_Interval = pCsInd->Event_Interval;

        pCsCfg->subEvtIntvl_625us = pCsInd->Subevent_Interval;
        pCsCfg->sSlot_csSubIntvl =  BSLOT_DUR_2_SSLOT_DUR(pCsCfg->subEvtIntvl_625us);
        pCsCfg->Subevents_Per_Event = pCsInd->Subevents_Per_Event;

        pCsCfg->Subevent_Len =  pCsInd->Subevent_Len[0] | (pCsInd->Subevent_Len[1]<<8) | (pCsInd->Subevent_Len[2]<<16);
        pCsCfg->aci = pCsInd->ACI;
        pCsCfg->PHY = pCsInd->PHY;
        pCsCfg->Tx_Pwr_Delta = pCsInd->Pwr_Delta;

        pCsCfg->sSlotCsDuration = (pCsCfg->Subevent_Len + SLOT_PROCESS_MAX_US)*SSLOT_US_REVERSE + 1;

        blt_ll_calcStepDuration(pCsCfg);

        CS_HCI_LOG("[START][RX] ind:0x%x,0x%x,0x%x", pCsCfg->idx, pCsCfg->connEventCount, pCsCfg->csOft_us);

    }
    else if(opcode == LL_CS_TERMINATE_IND)
    {
        CS_HCI_LOG("[TERMIN][RX] ind");
        rf_pkt_ll_cs_terminate_ind_t *pCsInd = (rf_pkt_ll_cs_terminate_ind_t*)raw;

        u8 cfgIdx = blt_ll_getCsConfigById(pAclConn->acl_conHandle, pCsInd->Config_ID);
        if(cfgIdx == 0xff){
            pCsParam->cs_terminate_ind = 0;
            CS_HCI_LOG("[TERMIN] config not exist:0x%x",pAclConn->acl_conHandle);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_TERMINATE_IND, HCI_ERR_CONN_REJ_LIMITED_RESOURCES, 1);
        }

        cs_config_t *pCsCfg = gGlobal_pCsCfg + pCsParam->cs_pend_idx;

        if(pCsParam->cs_terminate_ind & PROC_EVT_PENDING)
        {
            hci_le_csProcedureEnableComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle,(u8*) pCsCfg);
        }

        pCsParam->cs_terminate_ind = 0;
        pCsCfg->cs_procedure_measurement_en = 0;
        pCsCfg->cs_procedure_en = 0;
    }
    else if(opcode == LL_CS_FAE_REQ){
        CS_HCI_LOG("[FAE][RX] req");

        if(!(LL_FEATURE_MASK_1 & LL_FEATURE_MASK_CHANNEL_SOUNDING_HOST)){
            CS_HCI_LOG("[FAE] host not support");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_FAE_REQ, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }

        if((pCsParam->role_enable & CS_REFLECTOR_ROLE) == 0)//todo double check
        {
            CS_HCI_LOG("[FAE] reflector role not en:0x%x",pCsParam->role_enable);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_FAE_REQ, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }
        /*A Link Layer receiving an LL_CS_FAE_REQ PDU after having set the No_FAE bit in its CS capabilities
        shall immediately respond with an LL_REJECT_EXT_IND PDU with the error code Unsupported Feature
        or Parameter Value (0x11).
        */
        if(bltCsLocalSupportCap.Optional_Subfeatures_Supported & CS_No_FAE_SUPPORT){
            CS_HCI_LOG("[FAE] CS_No_FAE_SUPPORT:0x%x",bltCsLocalSupportCap.Optional_Subfeatures_Supported);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_FAE_REQ, HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE, 1);
        }

        pCsParam->cs_fae_req    |= (PROC_SEND_RSP | PROC_EVT_PENDING);


    }
    else if(opcode == LL_CS_FAE_RSP)
    {
        CS_HCI_LOG("[FAE][RX] rsp");
        if(!(pCsParam->cs_fae_req & PROC_WAIT_RSP)){
            pCsParam->cs_fae_req = 0;
            CS_HCI_LOG("[FAE] proc abnormal:0x%x",pCsParam->cs_fae_req);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_FAE_RSP, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }

        rf_pkt_ll_cs_fae_rsp_t *pRsp                = (rf_pkt_ll_cs_fae_rsp_t*)raw;
        smemcpy(pCsParam->fae_table,pRsp->fae_table,72);

        if(pCsParam->cs_fae_req & PROC_EVT_PENDING)
        {
            hci_le_readRemoteFAETableComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle,(u8*)&pRsp->fae_table[0]);
        }
        pCsParam->cs_fae_exchange = 1;
        pAclConn->ll_rsp_timeout_tick = 0;
        pCsParam->cs_fae_req = 0;
    }
    else if(opcode == LL_CS_CHANNEL_MAP_IND)
    {
        CS_HCI_LOG("[CHNL][RX] ind");
        rf_pkt_ll_cs_chn_map_ind_t *chm_ind = (rf_pkt_ll_cs_chn_map_ind_t*) raw;

        for(u8 i = 0 ; i<MAX_NUM_CS_CONFIG; i++)
        {
            if(pAclConn->csMapMask & BIT(i))
            {
                cs_config_t *pCfg = gGlobal_pCsCfg + i;
                if(pCfg->state)
                {
                    blt_cs_chnMapAndOperate(pCfg->Channel_Map, chm_ind->ChM);

//                  if(pCfg->cs_procedure_measurement_en)
                    {
                        pCfg->chn_update_inst = chm_ind->instant;
                        chm_ind->instant = pCfg->chn_update_inst;
                        pCfg->chn_update_pend = 1;

                    }
//                  else
//                  {
//                      chm_ind->instant = pAclConn->conn_inst + pCfg->Procedure_Interval;
//                      pCfg->Chn_en_num  = blt_cs_extractEnableChnMap(pCfg->Channel_Map, pCfg->filteredChnArray);
//                  }
                }
            }
        }

    }
    else{
        return LL_ERR_UNKNOWN_OPCODE;
    }

    return BLE_SUCCESS;

}

ble_sts_t   blc_ll_initCsRxFifo(u8 *pRxBuf, int fifo_size, int fifo_num)
{

    /* number must be 2^n */
    if( IS_POWER_OF_2(fifo_num) && fifo_num > 3){
        cs_rx_fifo.num = fifo_num;
        cs_rx_fifo.mask = fifo_num - 1;
    }
    else{
        return LL_ERR_INVALID_PARAMETER;
    }


    /* size must be 16*n */
    if( (fifo_size & 15) == 0){
        cs_rx_fifo.size = fifo_size;
        cs_rx_fifo.size_div_16 = fifo_size>>4;
    }
    else{
        return LL_ERR_INVALID_PARAMETER;
    }

    cs_rx_fifo.p_base = pRxBuf;
    cs_rx_fifo.rptr = cs_rx_fifo.wptr;

    return BLE_SUCCESS;
}

#ifndef     DBG_CS_MODE_STEP_NUM_EN
    #define     DBG_CS_MODE_STEP_NUM_EN                 1
#endif

#if (DBG_CS_MODE_STEP_NUM_EN)
    volatile u8 cs_subevent_num = 0;
    volatile u8 cs_mode0_step_num = 0;
    volatile u16 cs_nonMode0_step_num = 0;
#endif

void blt_ll_cs_data_loop(void){

    if(cs_rx_fifo.rptr != cs_rx_fifo.wptr){
        while (cs_rx_fifo.rptr != cs_rx_fifo.wptr)
        {
            u8 *raw_pkt = (u8 *)(cs_rx_fifo.p_base + (cs_rx_fifo.rptr & cs_rx_fifo.mask) * cs_rx_fifo.size);

            if(raw_pkt[2])
            {
                u8 csChannel = raw_pkt[3] & BLT_CS_STEP_CHANNEL_MASK;

                u8 role;
                s32 initial_IQData[LL_CS_STEP_IQ_NUM_MAX];

                u8 packetSyncFlag = 0;
                u8 packetQuality = CS_STEP_RECEIVE_PACKET_QUALITY_LOW;
                u8 packetNADM = CS_STEP_RECEIVE_PACKET_NADM_UNKNOWN;
                s8 packetRSSI = 0x7F;
                u8 toneQualityIndicator = CS_STEP_RECEIVE_TONE_QUALITY_UNAVAILABLE;
                u8 toneExtQualityIndicator = CS_STEP_RECEIVE_TONE_QUALITY_UNAVAILABLE;
                #if (CS_ANTENNA_SWITCHING_DATA_EN)
                    u8 toneAntQualityIndicator = CS_STEP_RECEIVE_TONE_QUALITY_UNAVAILABLE;
                #endif

                if(raw_pkt[2] & BLT_CS_MODE_RX_FLAG){
                    packetSyncFlag = (raw_pkt[DMA_CS_RFRX_OFFSET_SYNC_FLAG(raw_pkt)] & BIT(3)) >> 3;

                    if(packetSyncFlag){
                        packetQuality = blt_ll_cs_getPktMatchSyncQuality(raw_pkt);
                        packetRSSI = raw_pkt[DMA_CS_RFRX_OFFSET_RSSI(raw_pkt)] - 110;
                    }
                }

                u32 tick_cs_proc_start;
                u32 tx_turnaround_time_pos;//start point of tx turnaround
                u32 tx_turnaround_time_neg;//end point of tx turnaround
                u32 tx_on_start_tstamp;//start point of tx on
                u32 rx_iq_start_tstamp;
                u32 rx_pkt_iq_sync_tstamp;
                u32 cs_rx_accessAddr;
                #if (DBG_CS_DATA_PRINT_EN || DBG_CS_DATA_USB_PRINT_EN)
                    u8 cs_rx_agcGain = raw_pkt[DMA_CS_RFRX_OFFSET_RX_AGC_GAIN(raw_pkt)];
                #endif

                int pct_initiator[2];
                int pct_ext_initiator[2];
                int pct_reflector[2];
                int pct_ext_reflector[2];
                float toneQuality_raw;
                float toneExtQuality_raw;
                #if (CS_ANTENNA_SWITCHING_DATA_EN)
                    int pct_ant_initiator[2];
                    int pct_ant_reflector[2];
                    float toneAntQuality_raw;
                #endif

                short cte_initiator = 0x8000;//Time difference is not available
                short cte_reflector = 0x8000;//Time difference is not available

                u8 eventResult[255];//length < MAX HCIevent size
                u16 result_len = 0;
                hci_le_csSubeventResultEvt_t *pEvt = (hci_le_csSubeventResultEvt_t *)eventResult;
                hci_le_csSubeventResultContinueEvt_t *pEvtConti = (hci_le_csSubeventResultContinueEvt_t *)eventResult;

                if(cs_subeventResultReportType == CS_SUBEVENT_RESULT_EVENT_FIRST){
                    result_len = sizeof(hci_le_csSubeventResultEvt_t) + 3;

                    pEvt->Subevent_Code = HCI_SUB_EVT_LE_CS_SUBEVENT_RESULT;
                    pEvt->Connection_Handle = raw_pkt[DMA_CS_RFRX_OFFSET_CONN_HANDLE(raw_pkt)];
                    pEvt->Config_ID = raw_pkt[DMA_CS_RFRX_OFFSET_CONFIG_ID_LOW4BIT(raw_pkt)] & 0xF;
                    BYTE_TO_UINT16(pEvt->Start_ACL_Conn_Event, &raw_pkt[DMA_CS_RFRX_OFFSET_START_ACL_CONN_EVENT_2BYTE(raw_pkt)]);
                    BYTE_TO_UINT16(pEvt->Procedure_Counter, &raw_pkt[DMA_CS_RFRX_OFFSET_PROCEDURE_COUNTER_2BYTE(raw_pkt)]);
                    pEvt->Frequency_Compensation = 0xC000;//Frequency compensation value is not available, or the role is not initiator
                    pEvt->Reference_Power_Level = 10;//10 dBm   //TODO
                    pEvt->Procedure_Done_Status = raw_pkt[DMA_CS_RFRX_OFFSET_PROCEDURE_DONE_STATUS(raw_pkt)] & 0xF;
                    pEvt->Subevent_Done_Status = raw_pkt[DMA_CS_RFRX_OFFSET_SUBEVENT_DONE_STATUS(raw_pkt)] & 0xF;
                    pEvt->Abort_Reason = ((raw_pkt[DMA_CS_RFRX_OFFSET_PROCEDURE_DONE_STATUS(raw_pkt)] & 0xF0) >> 4) | (raw_pkt[DMA_CS_RFRX_OFFSET_SUBEVENT_DONE_STATUS(raw_pkt)] & 0xF0);
                    pEvt->Num_Antenna_Paths = ((raw_pkt[2] & BLT_CS_MODE_2_FLAG)?((raw_pkt[DMA_CS_RFRX_OFFSET_NUM_ANTENNA_PATHS_HIGH4BIT(raw_pkt)] & 0xF0) >> 4):1);
                    pEvt->Num_Steps_Reported = 1;

                    #if (DBG_CS_MODE_STEP_NUM_EN)
                        if(raw_pkt[2] & BLT_CS_MODE_0_FLAG){
                            cs_subevent_num++;
                            cs_mode0_step_num++;

                            //the temporary solution is used to reduce data loss caused by packet loss of reflector returned data
                            //pEvt->Connection_Handle |= cs_subevent_num << 8;//20231120 remove, GATT is now used

                            //tlkapi_printf(DBG_CS_MODE_STEP_NUM_EN, "CS subevent_num=%d\n", cs_subevent_num);
                            #if (DBG_CS_DATA_PRINT_EN)
                                //printf("CS subevent_num=%d\n", cs_subevent_num);
                            #endif
                        }
                    #endif
                }
                else if(cs_subeventResultReportType == CS_SUBEVENT_RESULT_EVENT_CONTINUE){
                    result_len = sizeof(hci_le_csSubeventResultContinueEvt_t) + 3;

                    pEvtConti->Subevent_Code = HCI_SUB_EVT_LE_CS_SUBEVENT_RESULT_CONTINUE;
                    pEvtConti->Connection_Handle = raw_pkt[DMA_CS_RFRX_OFFSET_CONN_HANDLE(raw_pkt)];
                    pEvtConti->Config_ID = raw_pkt[DMA_CS_RFRX_OFFSET_CONFIG_ID_LOW4BIT(raw_pkt)] & 0xF;
                    pEvtConti->Procedure_Done_Status = raw_pkt[DMA_CS_RFRX_OFFSET_PROCEDURE_DONE_STATUS(raw_pkt)] & 0xF;
                    pEvtConti->Subevent_Done_Status = raw_pkt[DMA_CS_RFRX_OFFSET_SUBEVENT_DONE_STATUS(raw_pkt)] & 0xF;
                    pEvtConti->Abort_Reason = ((raw_pkt[DMA_CS_RFRX_OFFSET_PROCEDURE_DONE_STATUS(raw_pkt)] & 0xF0) >> 4) | (raw_pkt[DMA_CS_RFRX_OFFSET_SUBEVENT_DONE_STATUS(raw_pkt)] & 0xF0);
                    pEvtConti->Num_Antenna_Paths = ((raw_pkt[2] & BLT_CS_MODE_2_FLAG)?((raw_pkt[DMA_CS_RFRX_OFFSET_NUM_ANTENNA_PATHS_HIGH4BIT(raw_pkt)] & 0xF0) >> 4):1);
                    pEvtConti->Num_Steps_Reported = 1;

                    #if (DBG_CS_MODE_STEP_NUM_EN)
                        if(raw_pkt[2] & BLT_CS_MODE_0_FLAG){
                            cs_mode0_step_num++;
                        }

                        if((raw_pkt[2] & BLT_CS_MODE_1_FLAG) || (raw_pkt[2] & BLT_CS_MODE_2_FLAG) || (raw_pkt[2] & BLT_CS_MODE_3_FLAG)){
                            cs_nonMode0_step_num++;
                        }

                        if((pEvtConti->Procedure_Done_Status == 0) && (pEvtConti->Subevent_Done_Status == 0)){
                            u16 csProcedureCounter;
                            BYTE_TO_UINT16(csProcedureCounter, &raw_pkt[DMA_CS_RFRX_OFFSET_PROCEDURE_COUNTER_2BYTE(raw_pkt)]);
                            (void)csProcedureCounter;//clean warning: variable 'csProcedureCounter' set but not used [-Wunused-but-set-variable],by SunWei

                            #if (DBG_CS_DATA_USB_PRINT_EN)
                                tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "CS Procedure %d done,subevent_num=%d,mode0_step_num=%d,nonMode0_step_num=%d\n", csProcedureCounter, cs_subevent_num, cs_mode0_step_num, cs_nonMode0_step_num);
                            #endif
                            #if (DBG_CS_DATA_PRINT_EN || DBG_CS_DISTANCE_STRING_PRINT_EN)
                                printf("CS Procedure %d end,subevent_num=%d,mode0_step_num=%d,nonMode0_step_num=%d\n", csProcedureCounter, cs_subevent_num, cs_mode0_step_num, cs_nonMode0_step_num);
                            #endif

                            cs_subevent_num = 0;
                            cs_mode0_step_num = 0;
                            cs_nonMode0_step_num = 0;
                        }
                    #endif
                }

                if(raw_pkt[2] & BLT_CS_INITIATOR_FLAG){
                    role = BLT_CS_INITIATOR_FLAG;

                    if(raw_pkt[2] & BLT_CS_MODE_0_FLAG){
                        //tlkapi_send_string_data(DBG_CS_DATA_EN, "initiator_mode_0_mlp_data", raw_pkt, 16);

                        cs_step_IQ_param_t *stepIQ_param = (cs_step_IQ_param_t *)&csStepIQ_param;
                        blt_ll_cs_getStepIQParam(role, STEP_MODE_0, raw_pkt, stepIQ_param);

                        DBG_CHN7_HIGH;DBG_CS_CHN8_HIGH;//test hadm algorithm time start
                        s32 rx_freq_offset;
                        float cfoCoarse;
                        float mfo = 0.0;
                        s16 cs_mfo = 0;
                        if(packetSyncFlag){
                            blt_ll_cs_Convert20BitIQ2int(&raw_pkt[csStepIQ_param.startIQIdx], &initial_IQData[0], csStepIQ_param.lenSample);
                            float IQData_float[LL_CS_STEP_IQ_LEN_MAX];
                            for (u16 i = 0; i < csStepIQ_param.lenIQ; i++){
                                IQData_float[i] = (float)initial_IQData[i];
                            }
                            rx_freq_offset = blt_ll_cs_getStepRxFreqOffset(BLE_1M_PHY, raw_pkt);
                            cfoCoarse = (float)rx_freq_offset;
                            cs_cfo = calcFreq(&IQData_float[0], csStepIQ_param.lenSample, cfoCoarse, SAMPLERATE);
                            cs_angleStep = 2 * PI * cs_cfo / SAMPLERATE;
                            calcCompensate(cs_compArr, csStepIQ_param.lenSample, -cs_angleStep);

                            mfo = (float)((2402 + csChannel) * 1000); //kHz
                            mfo = (cs_cfo / 1000) / mfo;
                            mfo = mfo * (1e6);//Units: 1 ppm
                            cs_mfo = (s16)(mfo * 100);//Units: 0.01 ppm
                        }
                        DBG_CHN7_LOW;DBG_CS_CHN8_LOW;//test hadm algorithm time post

                        u8 mode0Result[CS_STEP_DATA_LENGTH_MODE0_INITIATOR];
                        cs_step_mode0_t *pMode0 = (cs_step_mode0_t *)mode0Result;
                        pMode0->Packet_Quality = packetQuality;
                        pMode0->Packet_RSSI = packetRSSI;
                        pMode0->Packet_Antenna = 1;
                        pMode0->Measured_Freq_Offset = cs_mfo;
                        if(cs_subeventResultReportType == CS_SUBEVENT_RESULT_EVENT_FIRST){
                            pEvt->Step_Mode->mode = STEP_MODE_0;
                            pEvt->Step_Mode->channel = csChannel;
                            pEvt->Step_Mode->len = CS_STEP_DATA_LENGTH_MODE0_INITIATOR;
                            smemcpy(pEvt->Step_Mode->data, mode0Result, CS_STEP_DATA_LENGTH_MODE0_INITIATOR);
                        }
                        else if(cs_subeventResultReportType == CS_SUBEVENT_RESULT_EVENT_CONTINUE){
                            pEvtConti->Step_Mode->mode = STEP_MODE_0;
                            pEvtConti->Step_Mode->channel = csChannel;
                            pEvtConti->Step_Mode->len = CS_STEP_DATA_LENGTH_MODE0_INITIATOR;
                            smemcpy(pEvtConti->Step_Mode->data, mode0Result, CS_STEP_DATA_LENGTH_MODE0_INITIATOR);
                        }
                        result_len += CS_STEP_DATA_LENGTH_MODE0_INITIATOR;

                        //int debug_cfo = (int)cs_cfo;
                        //tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", csStepIQ_param.lenIQ, packetSyncFlag, packetQuality, debug_cfo);
                        #if (DBG_CS_DATA_PRINT_EN)
                            printf("init_mode0_data,cfo=%f,freq_off=%d\n", cs_cfo, rx_freq_offset);
                            printf("  lenIQ=%d,SyncFlag=%d,Quality=%d,agc_gain=%d,RSSI=%d\n", csStepIQ_param.lenIQ, packetSyncFlag, packetQuality, cs_rx_agcGain, packetRSSI);
                        #endif
                        #if (DBG_CS_DATA_USB_PRINT_EN)
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "init_mode0_data,cfo=%f,freq_off=%d\n", cs_cfo, rx_freq_offset);
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  lenIQ=%d,SyncFlag=%d,Quality=%d,agc_gain=%d,RSSI=%d\n", csStepIQ_param.lenIQ, packetSyncFlag, packetQuality, cs_rx_agcGain, packetRSSI);
                        #endif
                    }
                    else if(raw_pkt[2] & BLT_CS_MODE_1_FLAG){
                        //tlkapi_send_string_data(DBG_CS_DATA_EN, "initiator_mode_1_mlp_data", raw_pkt, 16);

                        cs_step_IQ_param_t *stepIQ_param = (cs_step_IQ_param_t *)&csStepIQ_param;
                        blt_ll_cs_getStepIQParam(role, STEP_MODE_1, raw_pkt, stepIQ_param);

                        BYTE_TO_UINT32(tx_on_start_tstamp, &raw_pkt[DMA_CS_RFRX_OFFSET_TX_ON_TSTAMP_4BYTE(raw_pkt)]);
                        BYTE_TO_UINT32(cs_rx_accessAddr, &raw_pkt[DMA_CS_RFRX_OFFSET_RX_ACCESS_ADDRESS_4BYTE(raw_pkt)]);
                        BYTE_TO_UINT32(rx_pkt_iq_sync_tstamp, &raw_pkt[DMA_CS_RFRX_OFFSET_TIME_STAMP(raw_pkt)]);

                        //BYTE_TO_UINT32(rx_iq_start_tstamp, &raw_pkt[DMA_CS_RFRX_OFFSET_IQ_START_TSTAMP(raw_pkt)]);//for test

                        DBG_CHN7_HIGH;DBG_CS_CHN8_HIGH;//test hadm algorithm time start
                        if(packetSyncFlag){
                            if(tx_on_start_tstamp != rx_pkt_iq_sync_tstamp){
                                int cs_aaCode[CS_ACCESS_ADDRESS_BIT_SIZE];
                                for (u16 i = 0; i < CS_ACCESS_ADDRESS_BIT_SIZE; i++){
                                    cs_aaCode[i] = (cs_rx_accessAddr >> i) & 1;
                                }
                                //tlkapi_printf(DBG_CS_DATA_EN, "cs_rx_accessAddr,%s", hex_to_str(cs_aaCode, CS_ACCESS_ADDRESS_BIT_SIZE));

                                int dataRate = 1e6; // for 1M case now
                                /**
                                 *  First param: number of step
                                 *  Second param: role, INITIATOR---0,REFLECTOR---1
                                 */
                                parameterPesCollectDataSDK paraPesSDK = pesCollectDataInitSDK(1, 0, dataRate, cs_aaCode, CS_ACCESS_ADDRESS_BIT_SIZE, cs_internalDelay, cs_adThr, cs_adStep);
                                u32 t_sy_center_delta_init = 194 * 2 * 1000;//(44+5+145)*1e-6/0.5e-9
                                calcPesInfoSDK((int*)&tx_on_start_tstamp, (int*)&rx_pkt_iq_sync_tstamp, t_sy_center_delta_init, &cte_initiator, paraPesSDK);
                                #if (0)
                                    blt_ll_cs_Convert20BitIQ2int(&raw_pkt[csStepIQ_param.startIQIdx], &initial_IQData[0], csStepIQ_param.lenSample);

                                    float rdm = 0.0;// Detector Metrics
                                    //currently consume 2.3ms under CCLK_96M
                                    packetNADM = calcPesNadm(initial_IQData, csStepIQ_param.lenSample, -cs_cfo, cs_nadm_adtype, &rdm, paraPesSDK);
                                #endif
                            }
                            else{
                                packetQuality = CS_STEP_RECEIVE_PACKET_QUALITY_LOW;
                            }
                        }
                        DBG_CHN7_LOW;DBG_CS_CHN8_LOW;//test hadm algorithm time post

                        u8 mode1Result[CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY];
                        cs_step_mode1_t *pMode1 = (cs_step_mode1_t *)mode1Result;

                        pMode1->Packet_Quality = packetQuality;
                        pMode1->Packet_NADM = packetNADM;
                        pMode1->Packet_RSSI = packetRSSI;
                        pMode1->ToA_ToD[0] = U16_LO(cte_initiator);
                        pMode1->ToA_ToD[1] = U16_HI(cte_initiator);
                        pMode1->Packet_Antenna = 1;
                        if(cs_subeventResultReportType == CS_SUBEVENT_RESULT_EVENT_FIRST){
                            pEvt->Step_Mode->mode = STEP_MODE_1;
                            pEvt->Step_Mode->channel = csChannel;
                            pEvt->Step_Mode->len = CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY;
                            smemcpy(pEvt->Step_Mode->data, mode1Result, CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY);
                        }
                        else if(cs_subeventResultReportType == CS_SUBEVENT_RESULT_EVENT_CONTINUE){
                            pEvtConti->Step_Mode->mode = STEP_MODE_1;
                            pEvtConti->Step_Mode->channel = csChannel;
                            pEvtConti->Step_Mode->len = CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY;
                            smemcpy(pEvtConti->Step_Mode->data, mode1Result, CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY);
                        }
                        result_len += CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY;

                        //tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", csStepIQ_param.lenIQ, csStepIQ_param.lenSample, csStepIQ_param.startIQIdx, 0);
                        //tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", cs_rx_accessAddr, tx_on_start_tstamp / SYSTEM_TIMER_TICK_1US, rx_iq_start_tstamp / SYSTEM_TIMER_TICK_1US, rx_pkt_iq_sync_tstamp / SYSTEM_TIMER_TICK_1US);
                        //tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", csStepIQ_param.lenIQ, rx_pkt_iq_sync_tstamp - tx_on_start_tstamp, (rx_pkt_iq_sync_tstamp - rx_iq_start_tstamp) / SYSTEM_TIMER_TICK_1US, (rx_pkt_iq_sync_tstamp - tx_on_start_tstamp) / SYSTEM_TIMER_TICK_1US);
                        //tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", packetSyncFlag, packetQuality, packetRSSI, cte_initiator);
                        #if (DBG_CS_DATA_PRINT_EN)
                            printf("init_mode_1_data,sync-tx_on=%d,ToA_ToD=%hd\n", rx_pkt_iq_sync_tstamp - tx_on_start_tstamp, cte_initiator);
                            printf("  lenIQ=%d,SyncFlag=%d,Quality=%d,NADM=%d,agc_gain=%d,RSSI=%d\n", csStepIQ_param.lenIQ, packetSyncFlag, packetQuality, packetNADM, cs_rx_agcGain, packetRSSI);
                        #endif
                        #if (DBG_CS_DATA_USB_PRINT_EN)
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "init_mode_1_data,sync-tx_on=%d,ToA_ToD=%hd\n", rx_pkt_iq_sync_tstamp - tx_on_start_tstamp, cte_initiator);
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  lenIQ=%d,SyncFlag=%d,Quality=%d,NADM=%d,agc_gain=%d,RSSI=%d\n", csStepIQ_param.lenIQ, packetSyncFlag, packetQuality, packetNADM, cs_rx_agcGain, packetRSSI);
                        #endif
                    }
                    else if(raw_pkt[2] & BLT_CS_MODE_2_FLAG){
                        //tlkapi_send_string_data(DBG_CS_DATA_EN, "initiator_mode_2_mlp_data", raw_pkt, 16);

                        cs_step_IQ_param_t *stepIQ_param = (cs_step_IQ_param_t *)&csStepIQ_param;
                        blt_ll_cs_getStepIQParam(role, STEP_MODE_2, raw_pkt, stepIQ_param);

                        BYTE_TO_UINT32(tick_cs_proc_start, &raw_pkt[DMA_CS_RFRX_OFFSET_TICK_CS_PROC_START_4BYTE(raw_pkt)]);
                        BYTE_TO_UINT32(tx_turnaround_time_neg, &raw_pkt[DMA_CS_RFRX_OFFSET_PKT_TX_NEG_TSTAMP(raw_pkt)]);
                        BYTE_TO_UINT32(rx_iq_start_tstamp, &raw_pkt[DMA_CS_RFRX_OFFSET_IQ_START_TSTAMP(raw_pkt)]);

                        cs_cfo = 0;//from Haili 20231030, the cfo at both ends should be opposite, reflector_cfo fixed to zero

                        DBG_CHN7_HIGH;DBG_CS_CHN8_HIGH;//test hadm algorithm time start
                        u16 valid_lenSample;
                        u16 valid_startIQIdx;

                        valid_lenSample = csStepIQ_param.lenSample - ((CS_RX_1M_TONE_HALF_EXCLUSION_PERIOD_US * 2 + csStepIQ_param.d_T_SW) << 2);
                        valid_startIQIdx = csStepIQ_param.startIQIdx + ((CS_RX_1M_TONE_HALF_EXCLUSION_PERIOD_US * (IQ_20_BIT_MODE >> 8)) << 2);

                        #if (CS_ANTENNA_SWITCHING_DATA_EN)
                            if(csStepIQ_param.d_N_AP == 2){
                                blt_ll_cs_Convert20BitIQ2int(&raw_pkt[valid_startIQIdx], &initial_IQData[0], valid_lenSample);
                                toneAntQuality_raw = calcTesInfo(&initial_IQData[0], cs_compArr, valid_lenSample, (u32)(rx_iq_start_tstamp - tick_cs_proc_start), (u32)(tx_turnaround_time_neg - tick_cs_proc_start), -cs_cfo, cs_if_adjustment79[csChannel], &(pct_ant_initiator[0]), cs_thresGood, cs_thresBad);
                                toneAntQualityIndicator = blt_ll_cs_getToneQualityIndicator(toneAntQuality_raw);
                                compressTesInfo(pct_ant_initiator, 2, 12);

                                valid_startIQIdx += csStepIQ_param.lenIQ;
                            }
                        #endif

                        blt_ll_cs_Convert20BitIQ2int(&raw_pkt[valid_startIQIdx], &initial_IQData[0], valid_lenSample);
                        toneQuality_raw = calcTesInfo(&initial_IQData[0], cs_compArr, valid_lenSample, (u32)(rx_iq_start_tstamp - tick_cs_proc_start), (u32)(tx_turnaround_time_neg - tick_cs_proc_start), -cs_cfo, cs_if_adjustment79[csChannel], &(pct_initiator[0]), cs_thresGood, cs_thresBad);
                        toneQualityIndicator = blt_ll_cs_getToneQualityIndicator(toneQuality_raw);
                        compressTesInfo(pct_initiator, 2, 12);

                        if(csStepIQ_param.tone_ext){
                            blt_ll_cs_Convert20BitIQ2int(&raw_pkt[valid_startIQIdx + csStepIQ_param.lenIQ], &initial_IQData[0], valid_lenSample);
                            toneExtQuality_raw = calcTesInfo(&initial_IQData[0], cs_compArr, valid_lenSample, (u32)(rx_iq_start_tstamp - tick_cs_proc_start), (u32)(tx_turnaround_time_neg - tick_cs_proc_start), -cs_cfo, cs_if_adjustment79[csChannel], &(pct_ext_initiator[0]), cs_thresGood, cs_thresBad);
                            toneExtQualityIndicator = blt_ll_cs_getToneQualityIndicator(toneExtQuality_raw);
                            compressTesInfo(pct_ext_initiator, 2, 12);
                        }
                        DBG_CHN7_LOW;DBG_CS_CHN8_LOW;//test hadm algorithm time post
                        //tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", csStepIQ_param.lenSample, toneQualityIndicator, csStepIQ_param.tone_ext, toneExtQualityIndicator);

                        #if (CS_ANTENNA_SWITCHING_DATA_EN)
                            u8 mode2Result[CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_2];
                            cs_step_mode2_t *pMode2 = (cs_step_mode2_t *)mode2Result;
                            u8 k = 0;
                            u8 data_len = CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1;

                            pMode2->Antenna_Permutation_Index = csStepIQ_param.d_ACI;
                            if(csStepIQ_param.d_N_AP == 2){
                                pMode2->Tone[k].Tone_PCT[0] = U16_LO(pct_ant_initiator[0]);
                                pMode2->Tone[k].Tone_PCT[1] = ((pct_ant_initiator[0] & 0xF00) >> 8) | ((pct_ant_initiator[1] & 0xF) << 4);
                                pMode2->Tone[k].Tone_PCT[2] = U16_LO((pct_ant_initiator[1] >> 4));
                                pMode2->Tone[k].Tone_Quality_Indicator = toneAntQualityIndicator;
                                k++;
                                data_len += CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_2 - CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1;
                            }
                            pMode2->Tone[k].Tone_PCT[0] = U16_LO(pct_initiator[0]);
                            pMode2->Tone[k].Tone_PCT[1] = ((pct_initiator[0] & 0xF00) >> 8) | ((pct_initiator[1] & 0xF) << 4);
                            pMode2->Tone[k].Tone_PCT[2] = U16_LO((pct_initiator[1] >> 4));
                            pMode2->Tone[k].Tone_Quality_Indicator = toneQualityIndicator;
                            k++;
                            pMode2->Tone[k].Tone_PCT[0] = U16_LO(pct_ext_initiator[0]);
                            pMode2->Tone[k].Tone_PCT[1] = ((pct_ext_initiator[0] & 0xF00) >> 8) | ((pct_ext_initiator[1] & 0xF) << 4);
                            pMode2->Tone[k].Tone_PCT[2] = U16_LO((pct_ext_initiator[1] >> 4));
                            if(raw_pkt[2] & BLT_CS_STEP_TONE_EXTENSION_SLOT_FLAG){
                                pMode2->Tone[k].Tone_Quality_Indicator = toneExtQualityIndicator | BIT(5);
                            }
                            else{
                                pMode2->Tone[k].Tone_Quality_Indicator = toneExtQualityIndicator | BIT(4);
                            }
                            if(cs_subeventResultReportType == CS_SUBEVENT_RESULT_EVENT_FIRST){
                                pEvt->Step_Mode->mode = STEP_MODE_2;
                                pEvt->Step_Mode->channel = csChannel;
                                pEvt->Step_Mode->len = data_len;
                                smemcpy(pEvt->Step_Mode->data, mode2Result, data_len);
                            }
                            else if(cs_subeventResultReportType == CS_SUBEVENT_RESULT_EVENT_CONTINUE){
                                pEvtConti->Step_Mode->mode = STEP_MODE_2;
                                pEvtConti->Step_Mode->channel = csChannel;
                                pEvtConti->Step_Mode->len = data_len;
                                smemcpy(pEvtConti->Step_Mode->data, mode2Result, data_len);
                            }
                            result_len += data_len;
                        #else
                            u8 mode2Result[CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1];
                            cs_step_mode2_t *pMode2 = (cs_step_mode2_t *)mode2Result;

                            pMode2->Antenna_Permutation_Index = 0;
                            pMode2->Tone[0].Tone_PCT[0] = U16_LO(pct_initiator[0]);
                            pMode2->Tone[0].Tone_PCT[1] = ((pct_initiator[0] & 0xF00) >> 8) | ((pct_initiator[1] & 0xF) << 4);
                            pMode2->Tone[0].Tone_PCT[2] = U16_LO((pct_initiator[1] >> 4));
                            pMode2->Tone[0].Tone_Quality_Indicator = toneQualityIndicator;
                            pMode2->Tone[1].Tone_PCT[0] = U16_LO(pct_ext_initiator[0]);
                            pMode2->Tone[1].Tone_PCT[1] = ((pct_ext_initiator[0] & 0xF00) >> 8) | ((pct_ext_initiator[1] & 0xF) << 4);
                            pMode2->Tone[1].Tone_PCT[2] = U16_LO((pct_ext_initiator[1] >> 4));
                            if(raw_pkt[2] & BLT_CS_STEP_TONE_EXTENSION_SLOT_FLAG){
                                pMode2->Tone[1].Tone_Quality_Indicator = toneExtQualityIndicator | BIT(5);
                            }
                            else{
                                pMode2->Tone[1].Tone_Quality_Indicator = toneExtQualityIndicator | BIT(4);
                            }
                            if(cs_subeventResultReportType == CS_SUBEVENT_RESULT_EVENT_FIRST){
                                pEvt->Step_Mode->mode = STEP_MODE_2;
                                pEvt->Step_Mode->channel = csChannel;
                                pEvt->Step_Mode->len = CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1;
                                smemcpy(pEvt->Step_Mode->data, mode2Result, CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1);
                            }
                            else if(cs_subeventResultReportType == CS_SUBEVENT_RESULT_EVENT_CONTINUE){
                                pEvtConti->Step_Mode->mode = STEP_MODE_2;
                                pEvtConti->Step_Mode->channel = csChannel;
                                pEvtConti->Step_Mode->len = CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1;
                                smemcpy(pEvtConti->Step_Mode->data, mode2Result, CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1);
                            }
                            result_len += CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1;
                        #endif

                        #if (DBG_CS_DATA_PRINT_EN)
                            //printf("init_mode_2_data,agc_gain=%d,toneQI=%d,pct_I=%hd,pct_Q=%hd,pct_I=0x%hx,pct_Q=0x%hx,toneQ=%f\n", cs_rx_agcGain, toneQualityIndicator, pct_initiator[0], pct_initiator[1], pct_initiator[0], pct_initiator[1], toneQuality_raw);
                            //printf("  lenIQ=%d,lenSample=%d,startIQIdx=%d,valid_lenSample=%d,valid_startIQIdx=%d\n", csStepIQ_param.lenIQ, csStepIQ_param.lenSample, csStepIQ_param.startIQIdx, valid_lenSample, valid_startIQIdx);
                            //printf("  channel=%d,iq_start=%d,tx_off=%d,cs_start=%d,iq_start-tx_off=%d\n", csChannel, rx_iq_start_tstamp/SYSTEM_TIMER_TICK_1US, tx_turnaround_time_neg/SYSTEM_TIMER_TICK_1US, tick_cs_proc_start/SYSTEM_TIMER_TICK_1US, (rx_iq_start_tstamp-tx_turnaround_time_neg)/SYSTEM_TIMER_TICK_1US);
                            //if(csStepIQ_param.tone_ext){
                            //  printf("  tone_ext=1,toneQI=%d,pct_I=%hd,pct_Q=%hd,pct_I=0x%hx,pct_Q=0x%hx,toneQ=%f\n", toneExtQualityIndicator, pct_ext_initiator[0], pct_ext_initiator[1], pct_ext_initiator[0], pct_ext_initiator[1], toneExtQuality_raw);
                            //}
                            printf("init_mode_2_data,agc_gain=%d,toneQI=%d,pct_I=%hd,pct_Q=%hd,toneQ=%f\n", cs_rx_agcGain, toneQualityIndicator, pct_initiator[0], pct_initiator[1], toneQuality_raw);
                            printf("  channel=%d,iq_start-tx_off=%d\n", csChannel, (rx_iq_start_tstamp-tx_turnaround_time_neg)/SYSTEM_TIMER_TICK_1US);
                            #if (CS_ANTENNA_SWITCHING_DATA_EN)
                                if(csStepIQ_param.d_N_AP == 2){
                                    printf("  N_AP=%d,T_SW=%d,ACI=%d,toneQI=%d,pct_I=%hd,pct_Q=%hd,toneQ=%f\n", csStepIQ_param.d_N_AP, csStepIQ_param.d_T_SW, csStepIQ_param.d_ACI, toneAntQualityIndicator, pct_ant_initiator[0], pct_ant_initiator[1], toneAntQuality_raw);
                                }
                            #endif
                            if(csStepIQ_param.tone_ext){
                                printf("  tone_ext=1,toneQI=%d,pct_I=%hd,pct_Q=%hd,toneQ=%f\n", toneExtQualityIndicator, pct_ext_initiator[0], pct_ext_initiator[1], toneExtQuality_raw);
                            }
                        #endif
                        #if (DBG_CS_DATA_USB_PRINT_EN)
                            //tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "init_mode_2_data,agc_gain=%d,toneQI=%d,pct_I=%hd,pct_Q=%hd,pct_I=0x%hx,pct_Q=0x%hx,toneQ=%f\n", cs_rx_agcGain, toneQualityIndicator, pct_initiator[0], pct_initiator[1], pct_initiator[0], pct_initiator[1], toneQuality_raw);
                            //tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  lenIQ=%d,lenSample=%d,startIQIdx=%d,valid_lenSample=%d,valid_startIQIdx=%d\n", csStepIQ_param.lenIQ, csStepIQ_param.lenSample, csStepIQ_param.startIQIdx, valid_lenSample, valid_startIQIdx);
                            //tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  channel=%d,iq_start=%d,tx_off=%d,cs_start=%d,iq_start-tx_off=%d\n", csChannel, rx_iq_start_tstamp/SYSTEM_TIMER_TICK_1US, tx_turnaround_time_neg/SYSTEM_TIMER_TICK_1US, tick_cs_proc_start/SYSTEM_TIMER_TICK_1US, (rx_iq_start_tstamp-tx_turnaround_time_neg)/SYSTEM_TIMER_TICK_1US);
                            //if(csStepIQ_param.tone_ext){
                            //  tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  tone_ext=1,toneQI=%d,pct_I=%hd,pct_Q=%hd,pct_I=0x%hx,pct_Q=0x%hx,toneQ=%f\n", toneExtQualityIndicator, pct_ext_initiator[0], pct_ext_initiator[1], pct_ext_initiator[0], pct_ext_initiator[1], toneExtQuality_raw);
                            //}
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "init_mode_2_data,agc_gain=%d,toneQI=%d,pct_I=%hd,pct_Q=%hd,toneQ=%f\n", cs_rx_agcGain, toneQualityIndicator, pct_initiator[0], pct_initiator[1], toneQuality_raw);
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  channel=%d,iq_start-tx_off=%d\n", csChannel, (rx_iq_start_tstamp-tx_turnaround_time_neg)/SYSTEM_TIMER_TICK_1US);
                            #if (CS_ANTENNA_SWITCHING_DATA_EN)
                                if(csStepIQ_param.d_N_AP == 2){
                                    tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  N_AP=%d,T_SW=%d,ACI=%d,toneQI=%d,pct_I=%hd,pct_Q=%hd,toneQ=%f\n", csStepIQ_param.d_N_AP, csStepIQ_param.d_T_SW, csStepIQ_param.d_ACI, toneAntQualityIndicator, pct_ant_initiator[0], pct_ant_initiator[1], toneAntQuality_raw);
                                }
                            #endif
                            if(csStepIQ_param.tone_ext){
                                tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  tone_ext=1,toneQI=%d,pct_I=%hd,pct_Q=%hd,toneQ=%f\n", toneExtQualityIndicator, pct_ext_initiator[0], pct_ext_initiator[1], toneExtQuality_raw);
                            }
                        #endif
                    }
                    else if(raw_pkt[2] & BLT_CS_MODE_3_FLAG){
                        //tlkapi_send_string_data(DBG_CS_DATA_EN, "initiator_mode_3_mlp_data", raw_pkt, 16);
                        #if (DBG_CS_DATA_PRINT_EN)
                            printf("init_mode_3_data\n");
                        #endif
                        #if (DBG_CS_DATA_USB_PRINT_EN)
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "init_mode_3_data\n");
                        #endif
                    }
                }
                else if(raw_pkt[2] & BLT_CS_REFLECTOR_FLAG){
                    role = BLT_CS_REFLECTOR_FLAG;

                    if(raw_pkt[2] & BLT_CS_MODE_0_FLAG){
                        //tlkapi_send_string_data(DBG_CS_DATA_EN, "reflector_mode_0_mlp_data", raw_pkt, 16);

                        DBG_CHN7_HIGH;DBG_CS_CHN8_HIGH;//test hadm algorithm time start
                        cs_cfo = cs_angleStep = 0;
                        calcCompensate(cs_compArr, LL_CS_STEP_IQ_NUM_MAX / 2, -cs_angleStep);

                        s32 rx_freq_offset;
                        if(packetSyncFlag){
                            rx_freq_offset = blt_ll_cs_getStepRxFreqOffset(BLE_1M_PHY, raw_pkt);
                        }
                        (void)rx_freq_offset;//clean warning: variable 'rx_freq_offset' set but not used [-Wunused-but-set-variable],by SunWei
                        DBG_CHN7_LOW;DBG_CS_CHN8_LOW;//test hadm algorithm time post

                        u8 mode0Result[CS_STEP_DATA_LENGTH_MODE0_REFLECTOR];
                        cs_step_mode0_t *pMode0 = (cs_step_mode0_t *)mode0Result;
                        pMode0->Packet_Quality = packetQuality;
                        pMode0->Packet_RSSI = packetRSSI;
                        pMode0->Packet_Antenna = 1;
                        if(cs_subeventResultReportType == CS_SUBEVENT_RESULT_EVENT_FIRST){
                            pEvt->Step_Mode->mode = STEP_MODE_0;
                            pEvt->Step_Mode->channel = csChannel;
                            pEvt->Step_Mode->len = CS_STEP_DATA_LENGTH_MODE0_REFLECTOR;
                            smemcpy(pEvt->Step_Mode->data, mode0Result, CS_STEP_DATA_LENGTH_MODE0_REFLECTOR);
                        }
                        else if(cs_subeventResultReportType == CS_SUBEVENT_RESULT_EVENT_CONTINUE){
                            pEvtConti->Step_Mode->mode = STEP_MODE_0;
                            pEvtConti->Step_Mode->channel = csChannel;
                            pEvtConti->Step_Mode->len = CS_STEP_DATA_LENGTH_MODE0_REFLECTOR;
                            smemcpy(pEvtConti->Step_Mode->data, mode0Result, CS_STEP_DATA_LENGTH_MODE0_REFLECTOR);
                        }
                        result_len += CS_STEP_DATA_LENGTH_MODE0_REFLECTOR;

                        #if (DBG_CS_DATA_PRINT_EN)
                            printf("refl_mode_0_data,freq_off=%d\n", rx_freq_offset);
                            printf("  lenIQ=%d,SyncFlag=%d,Quality=%d,agc_gain=%d,RSSI=%d\n", csStepIQ_param.lenIQ, packetSyncFlag, packetQuality, cs_rx_agcGain, packetRSSI);
                        #endif
                        #if (DBG_CS_DATA_USB_PRINT_EN)
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "refl_mode_0_data,freq_off=%d\n", rx_freq_offset);
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  lenIQ=%d,SyncFlag=%d,Quality=%d,agc_gain=%d,RSSI=%d\n", csStepIQ_param.lenIQ, packetSyncFlag, packetQuality, cs_rx_agcGain, packetRSSI);
                        #endif
                    }
                    else if(raw_pkt[2] & BLT_CS_MODE_1_FLAG){
                        //tlkapi_send_string_data(DBG_CS_DATA_EN, "reflector_mode_1_mlp_data", raw_pkt, 16);

                        cs_step_IQ_param_t *stepIQ_param = (cs_step_IQ_param_t *)&csStepIQ_param;
                        blt_ll_cs_getStepIQParam(role, STEP_MODE_1, raw_pkt, stepIQ_param);
                        //tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", csStepIQ_param.lenIQ, csStepIQ_param.lenSample, csStepIQ_param.startIQIdx, 0);

                        BYTE_TO_UINT32(tx_on_start_tstamp, &raw_pkt[DMA_CS_RFRX_OFFSET_TX_ON_TSTAMP_4BYTE(raw_pkt)]);
                        BYTE_TO_UINT32(cs_rx_accessAddr, &raw_pkt[DMA_CS_RFRX_OFFSET_RX_ACCESS_ADDRESS_4BYTE(raw_pkt)]);
                        BYTE_TO_UINT32(rx_pkt_iq_sync_tstamp, &raw_pkt[DMA_CS_RFRX_OFFSET_TIME_STAMP(raw_pkt)]);

                        //BYTE_TO_UINT32(rx_iq_start_tstamp, &raw_pkt[DMA_CS_RFRX_OFFSET_IQ_START_TSTAMP(raw_pkt)]);//for test

                        DBG_CHN7_HIGH;DBG_CS_CHN8_HIGH;//test hadm algorithm time start
                        if(packetSyncFlag){
                            if(tx_on_start_tstamp != rx_pkt_iq_sync_tstamp){
                                int cs_aaCode[CS_ACCESS_ADDRESS_BIT_SIZE];
                                for (u16 i = 0; i < CS_ACCESS_ADDRESS_BIT_SIZE; i++){
                                    cs_aaCode[i] = (cs_rx_accessAddr >> i) & 1;
                                }
                                //tlkapi_printf(DBG_CS_DATA_EN, "cs_rx_accessAddr,%s", hex_to_str(cs_aaCode, CS_ACCESS_ADDRESS_BIT_SIZE));

                                int dataRate = 1e6; // for 1M case now
                                /**
                                 *  First param: number of step
                                 *  Second param: role, INITIATOR---0,REFLECTOR---1
                                 */
                                parameterPesCollectDataSDK paraPesSDK = pesCollectDataInitSDK(1, 1, dataRate, cs_aaCode, CS_ACCESS_ADDRESS_BIT_SIZE, cs_internalDelay, cs_adThr, cs_adStep);
                                u32 t_sy_center_delta_init = 194 * 2 * 1000;//(44+5+145)*1e-6/0.5e-9
                                calcPesInfoSDK((int*)&tx_on_start_tstamp, (int*)&rx_pkt_iq_sync_tstamp, t_sy_center_delta_init, &cte_reflector, paraPesSDK);

                                #if (0)
                                    blt_ll_cs_Convert20BitIQ2int(&raw_pkt[csStepIQ_param.startIQIdx], &initial_IQData[0], csStepIQ_param.lenSample);

                                    float rdm = 0.0;// Detector Metrics
                                    //currently consume 2.3ms under CCLK_96M
                                    packetNADM = calcPesNadm(initial_IQData, csStepIQ_param.lenSample, -cs_cfo, cs_nadm_adtype, &rdm, paraPesSDK);
                                #endif
                            }
                            else{
                                packetQuality = CS_STEP_RECEIVE_PACKET_QUALITY_LOW;
                            }
                        }
                        DBG_CHN7_LOW;DBG_CS_CHN8_LOW;//test hadm algorithm time post

                        u8 mode1Result[CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY];
                        cs_step_mode1_t *pMode1 = (cs_step_mode1_t *)mode1Result;
                        pMode1->Packet_Quality = packetQuality;
                        pMode1->Packet_NADM = packetNADM;
                        pMode1->Packet_RSSI = packetRSSI;
                        pMode1->ToA_ToD[0] = U16_LO(cte_reflector);
                        pMode1->ToA_ToD[1] = U16_HI(cte_reflector);
                        pMode1->Packet_Antenna = 1;
                        if(cs_subeventResultReportType == CS_SUBEVENT_RESULT_EVENT_FIRST){
                            pEvt->Step_Mode->mode = STEP_MODE_1;
                            pEvt->Step_Mode->channel = csChannel;
                            pEvt->Step_Mode->len = CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY;
                            smemcpy(pEvt->Step_Mode->data, mode1Result, CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY);
                        }
                        else if(cs_subeventResultReportType == CS_SUBEVENT_RESULT_EVENT_CONTINUE){
                            pEvtConti->Step_Mode->mode = STEP_MODE_1;
                            pEvtConti->Step_Mode->channel = csChannel;
                            pEvtConti->Step_Mode->len = CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY;
                            smemcpy(pEvtConti->Step_Mode->data, mode1Result, CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY);
                        }
                        result_len += CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY;

                        //tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", csStepIQ_param.lenIQ, csStepIQ_param.lenSample, csStepIQ_param.startIQIdx, 0);
                        //tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", cs_rx_accessAddr, rx_iq_start_tstamp / SYSTEM_TIMER_TICK_1US, rx_pkt_iq_sync_tstamp / SYSTEM_TIMER_TICK_1US, tx_on_start_tstamp / SYSTEM_TIMER_TICK_1US);
                        //tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", csStepIQ_param.lenIQ, tx_on_start_tstamp - rx_pkt_iq_sync_tstamp, (rx_pkt_iq_sync_tstamp - rx_iq_start_tstamp) / SYSTEM_TIMER_TICK_1US, (tx_on_start_tstamp - rx_pkt_iq_sync_tstamp) / SYSTEM_TIMER_TICK_1US);
                        //tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", packetSyncFlag, packetQuality, packetRSSI, cte_reflector);
                        #if (DBG_CS_DATA_PRINT_EN)
                            printf("refl_mode_1_data,tx_on-sync=%d,ToD_ToA=%hd\n", tx_on_start_tstamp - rx_pkt_iq_sync_tstamp, cte_reflector);
                            printf("  lenIQ=%d,SyncFlag=%d,Quality=%d,NADM=%d,agc_gain=%d,RSSI=%d\n", csStepIQ_param.lenIQ, packetSyncFlag, packetQuality, packetNADM, cs_rx_agcGain, packetRSSI);
                        #endif
                        #if (DBG_CS_DATA_USB_PRINT_EN)
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "refl_mode_1_data,tx_on-sync=%d,ToD_ToA=%hd\n", tx_on_start_tstamp - rx_pkt_iq_sync_tstamp, cte_reflector);
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  lenIQ=%d,SyncFlag=%d,Quality=%d,NADM=%d,agc_gain=%d,RSSI=%d\n", csStepIQ_param.lenIQ, packetSyncFlag, packetQuality, packetNADM, cs_rx_agcGain, packetRSSI);
                        #endif
                    }
                    else if(raw_pkt[2] & BLT_CS_MODE_2_FLAG){
                        //tlkapi_send_string_data(DBG_CS_DATA_EN, "reflector_mode_2_mlp_data", raw_pkt, 16);

                        cs_step_IQ_param_t *stepIQ_param = (cs_step_IQ_param_t *)&csStepIQ_param;
                        blt_ll_cs_getStepIQParam(role, STEP_MODE_2, raw_pkt, stepIQ_param);

                        BYTE_TO_UINT32(tick_cs_proc_start, &raw_pkt[DMA_CS_RFRX_OFFSET_TICK_CS_PROC_START_4BYTE(raw_pkt)]);
                        BYTE_TO_UINT32(rx_iq_start_tstamp, &raw_pkt[DMA_CS_RFRX_OFFSET_IQ_START_TSTAMP(raw_pkt)]);
                        BYTE_TO_UINT32(tx_turnaround_time_pos, &raw_pkt[DMA_CS_RFRX_OFFSET_LAST_TX_POS_TSTAMP_4BYTE(raw_pkt)]);

                        DBG_CHN7_HIGH;DBG_CS_CHN8_HIGH;//test hadm algorithm time start
                        u16 valid_lenSample;
                        u16 valid_startIQIdx;

                        valid_lenSample = csStepIQ_param.lenSample - ((CS_RX_1M_TONE_HALF_EXCLUSION_PERIOD_US * 2 + csStepIQ_param.d_T_SW) << 2);
                        valid_startIQIdx = csStepIQ_param.startIQIdx + ((CS_RX_1M_TONE_HALF_EXCLUSION_PERIOD_US * (IQ_20_BIT_MODE >> 8)) << 2);

                        #if (CS_ANTENNA_SWITCHING_DATA_EN)
                            if(csStepIQ_param.d_N_AP == 2){
                                blt_ll_cs_Convert20BitIQ2int(&raw_pkt[valid_startIQIdx], &initial_IQData[0], valid_lenSample);
                                toneAntQuality_raw = calcTesInfo(&initial_IQData[0], cs_compArr, valid_lenSample, (u32)(rx_iq_start_tstamp - tick_cs_proc_start), (u32)(tx_turnaround_time_pos - tick_cs_proc_start), -cs_cfo, cs_if_adjustment79[csChannel], &(pct_ant_reflector[0]), cs_thresGood, cs_thresBad);
                                toneAntQualityIndicator = blt_ll_cs_getToneQualityIndicator(toneAntQuality_raw);
                                compressTesInfo(pct_ant_reflector, 2, 12);

                                valid_startIQIdx += csStepIQ_param.lenIQ;
                            }
                        #endif

                        blt_ll_cs_Convert20BitIQ2int(&raw_pkt[valid_startIQIdx], &initial_IQData[0], valid_lenSample);
                        toneQuality_raw = calcTesInfo(&initial_IQData[0], cs_compArr, valid_lenSample, (u32)(rx_iq_start_tstamp - tick_cs_proc_start), (u32)(tx_turnaround_time_pos - tick_cs_proc_start), -cs_cfo, cs_if_adjustment79[csChannel], &(pct_reflector[0]), cs_thresGood, cs_thresBad);
                        toneQualityIndicator = blt_ll_cs_getToneQualityIndicator(toneQuality_raw);
                        compressTesInfo(pct_reflector, 2, 12);

                        if(csStepIQ_param.tone_ext){
                            blt_ll_cs_Convert20BitIQ2int(&raw_pkt[valid_startIQIdx + csStepIQ_param.lenIQ], &initial_IQData[0], valid_lenSample);
                            toneExtQuality_raw = calcTesInfo(&initial_IQData[0], cs_compArr, valid_lenSample, (u32)(rx_iq_start_tstamp - tick_cs_proc_start), (u32)(tx_turnaround_time_pos - tick_cs_proc_start), -cs_cfo, cs_if_adjustment79[csChannel], &(pct_ext_reflector[0]), cs_thresGood, cs_thresBad);
                            toneExtQualityIndicator = blt_ll_cs_getToneQualityIndicator(toneExtQuality_raw);
                            compressTesInfo(pct_ext_reflector, 2, 12);
                        }
                        DBG_CHN7_LOW;DBG_CS_CHN8_LOW;//test hadm algorithm time post
                        //tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", csStepIQ_param.lenSample, toneQualityIndicator, csStepIQ_param.tone_ext, toneExtQualityIndicator);

                        #if (CS_ANTENNA_SWITCHING_DATA_EN)
                            u8 mode2Result[CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_2];
                            cs_step_mode2_t *pMode2 = (cs_step_mode2_t *)mode2Result;
                            u8 k = 0;
                            u8 data_len = CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1;

                            pMode2->Antenna_Permutation_Index = csStepIQ_param.d_ACI;
                            if(csStepIQ_param.d_N_AP == 2){
                                pMode2->Tone[k].Tone_PCT[0] = U16_LO(pct_ant_reflector[0]);
                                pMode2->Tone[k].Tone_PCT[1] = ((pct_ant_reflector[0] & 0xF00) >> 8) | ((pct_ant_reflector[1] & 0xF) << 4);
                                pMode2->Tone[k].Tone_PCT[2] = U16_LO((pct_ant_reflector[1] >> 4));
                                pMode2->Tone[k].Tone_Quality_Indicator = toneAntQualityIndicator;
                                k++;
                                data_len += CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_2 - CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1;
                            }
                            pMode2->Tone[k].Tone_PCT[0] = U16_LO(pct_reflector[0]);
                            pMode2->Tone[k].Tone_PCT[1] = ((pct_reflector[0] & 0xF00) >> 8) | ((pct_reflector[1] & 0xF) << 4);
                            pMode2->Tone[k].Tone_PCT[2] = U16_LO((pct_reflector[1] >> 4));
                            pMode2->Tone[k].Tone_Quality_Indicator = toneQualityIndicator;
                            k++;
                            pMode2->Tone[k].Tone_PCT[0] = U16_LO(pct_ext_reflector[0]);
                            pMode2->Tone[k].Tone_PCT[1] = ((pct_ext_reflector[0] & 0xF00) >> 8) | ((pct_ext_reflector[1] & 0xF) << 4);
                            pMode2->Tone[k].Tone_PCT[2] = U16_LO((pct_ext_reflector[1] >> 4));
                            if(raw_pkt[2] & BLT_CS_STEP_TONE_EXTENSION_SLOT_FLAG){
                                pMode2->Tone[k].Tone_Quality_Indicator = toneExtQualityIndicator | BIT(5);
                            }
                            else{
                                pMode2->Tone[k].Tone_Quality_Indicator = toneExtQualityIndicator | BIT(4);
                            }
                            if(cs_subeventResultReportType == CS_SUBEVENT_RESULT_EVENT_FIRST){
                                pEvt->Step_Mode->mode = STEP_MODE_2;
                                pEvt->Step_Mode->channel = csChannel;
                                pEvt->Step_Mode->len = data_len;
                                smemcpy(pEvt->Step_Mode->data, mode2Result, data_len);
                            }
                            else if(cs_subeventResultReportType == CS_SUBEVENT_RESULT_EVENT_CONTINUE){
                                pEvtConti->Step_Mode->mode = STEP_MODE_2;
                                pEvtConti->Step_Mode->channel = csChannel;
                                pEvtConti->Step_Mode->len = data_len;
                                smemcpy(pEvtConti->Step_Mode->data, mode2Result, data_len);
                            }
                            result_len += data_len;
                        #else
                            u8 mode2Result[CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1];
                            cs_step_mode2_t *pMode2 = (cs_step_mode2_t *)mode2Result;
                            pMode2->Antenna_Permutation_Index = 0;
                            pMode2->Tone[0].Tone_PCT[0] = U16_LO(pct_reflector[0]);
                            pMode2->Tone[0].Tone_PCT[1] = ((pct_reflector[0] & 0xF00) >> 8) | ((pct_reflector[1] & 0xF) << 4);
                            pMode2->Tone[0].Tone_PCT[2] = U16_LO((pct_reflector[1] >> 4));
                            pMode2->Tone[0].Tone_Quality_Indicator = toneQualityIndicator;
                            pMode2->Tone[1].Tone_PCT[0] = U16_LO(pct_ext_reflector[0]);
                            pMode2->Tone[1].Tone_PCT[1] = ((pct_ext_reflector[0] & 0xF00) >> 8) | ((pct_ext_reflector[1] & 0xF) << 4);
                            pMode2->Tone[1].Tone_PCT[2] = U16_LO((pct_ext_reflector[1] >> 4));
                            if(raw_pkt[2] & BLT_CS_STEP_TONE_EXTENSION_SLOT_FLAG){
                                pMode2->Tone[1].Tone_Quality_Indicator = toneExtQualityIndicator | BIT(5);
                            }
                            else{
                                pMode2->Tone[1].Tone_Quality_Indicator = toneExtQualityIndicator | BIT(4);
                            }
                            if(cs_subeventResultReportType == CS_SUBEVENT_RESULT_EVENT_FIRST){
                                pEvt->Step_Mode->mode = STEP_MODE_2;
                                pEvt->Step_Mode->channel = csChannel;
                                pEvt->Step_Mode->len = CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1;
                                smemcpy(pEvt->Step_Mode->data, mode2Result, CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1);
                            }
                            else if(cs_subeventResultReportType == CS_SUBEVENT_RESULT_EVENT_CONTINUE){
                                pEvtConti->Step_Mode->mode = STEP_MODE_2;
                                pEvtConti->Step_Mode->channel = csChannel;
                                pEvtConti->Step_Mode->len = CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1;
                                smemcpy(pEvtConti->Step_Mode->data, mode2Result, CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1);
                            }
                            result_len += CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1;
                        #endif

                        #if (DBG_CS_DATA_PRINT_EN)
                            //printf("refl_mode_2_data,agc_gain=%d,toneQI=%d,pct_I=%hd,pct_Q=%hd,pct_I=0x%hx,pct_Q=0x%hx,toneQ=%f\n", cs_rx_agcGain, toneQualityIndicator, pct_reflector[0], pct_reflector[1], pct_reflector[0], pct_reflector[1], toneQuality_raw);
                            //printf("  lenIQ=%d,lenSample=%d,startIQIdx=%d,valid_lenSample=%d,valid_startIQIdx=%d\n", csStepIQ_param.lenIQ, csStepIQ_param.lenSample, csStepIQ_param.startIQIdx, valid_lenSample, valid_startIQIdx);
                            //printf("  channel=%d,iq_start=%d,tx_en=%d,cs_start=%d,tx_en-iq_start=%d\n", csChannel, rx_iq_start_tstamp/SYSTEM_TIMER_TICK_1US, tx_turnaround_time_pos/SYSTEM_TIMER_TICK_1US, tick_cs_proc_start/SYSTEM_TIMER_TICK_1US, (tx_turnaround_time_pos-rx_iq_start_tstamp)/SYSTEM_TIMER_TICK_1US);
                            //if(csStepIQ_param.tone_ext){
                            //  printf("  tone_ext=1,toneQI=%d,pct_I=%hd,pct_Q=%hd,pct_I=0x%hx,pct_Q=0x%hx,toneQ=%f\n", toneExtQualityIndicator, pct_ext_reflector[0], pct_ext_reflector[1], pct_ext_reflector[0], pct_ext_reflector[1], toneExtQuality_raw);
                            //}
                            printf("refl_mode_2_data,agc_gain=%d,toneQI=%d,pct_I=%hd,pct_Q=%hd,toneQ=%f\n", cs_rx_agcGain, toneQualityIndicator, pct_reflector[0], pct_reflector[1], toneQuality_raw);
                            printf("  channel=%d,tx_en-iq_start=%d\n", csChannel, (tx_turnaround_time_pos-rx_iq_start_tstamp)/SYSTEM_TIMER_TICK_1US);
                            #if (CS_ANTENNA_SWITCHING_DATA_EN)
                                if(csStepIQ_param.d_N_AP == 2){
                                    printf("  N_AP=%d,T_SW=%d,ACI=%d,toneQI=%d,pct_I=%hd,pct_Q=%hd,toneQ=%f\n", csStepIQ_param.d_N_AP, csStepIQ_param.d_T_SW, csStepIQ_param.d_ACI, toneAntQualityIndicator, pct_ant_reflector[0], pct_ant_reflector[1], toneAntQuality_raw);
                                }
                            #endif
                            if(csStepIQ_param.tone_ext){
                                printf("  tone_ext=1,toneQI=%d,pct_I=%hd,pct_Q=%hd,toneQ=%f\n", toneExtQualityIndicator, pct_ext_reflector[0], pct_ext_reflector[1], toneExtQuality_raw);
                            }
                        #endif
                        #if (DBG_CS_DATA_USB_PRINT_EN)
                            //tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "refl_mode_2_data,agc_gain=%d,toneQI=%d,pct_I=%hd,pct_Q=%hd,pct_I=0x%hx,pct_Q=0x%hx,toneQ=%f\n", cs_rx_agcGain, toneQualityIndicator, pct_reflector[0], pct_reflector[1], pct_reflector[0], pct_reflector[1], toneQuality_raw);
                            //tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  lenIQ=%d,lenSample=%d,startIQIdx=%d,valid_lenSample=%d,valid_startIQIdx=%d\n", csStepIQ_param.lenIQ, csStepIQ_param.lenSample, csStepIQ_param.startIQIdx, valid_lenSample, valid_startIQIdx);
                            //tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  channel=%d,iq_start=%d,tx_en=%d,cs_start=%d,tx_en-iq_start=%d\n", csChannel, rx_iq_start_tstamp/SYSTEM_TIMER_TICK_1US, tx_turnaround_time_pos/SYSTEM_TIMER_TICK_1US, tick_cs_proc_start/SYSTEM_TIMER_TICK_1US, (tx_turnaround_time_pos-rx_iq_start_tstamp)/SYSTEM_TIMER_TICK_1US);
                            //if(csStepIQ_param.tone_ext){
                            //  tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  tone_ext=1,toneQI=%d,pct_I=%hd,pct_Q=%hd,pct_I=0x%hx,pct_Q=0x%hx,toneQ=%f\n", toneExtQualityIndicator, pct_ext_reflector[0], pct_ext_reflector[1], pct_ext_reflector[0], pct_ext_reflector[1], toneExtQuality_raw);
                            //}
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "refl_mode_2_data,agc_gain=%d,toneQI=%d,pct_I=%hd,pct_Q=%hd,toneQ=%f\n", cs_rx_agcGain, toneQualityIndicator, pct_reflector[0], pct_reflector[1], toneQuality_raw);
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  channel=%d,tx_en-iq_start=%d\n", csChannel, (tx_turnaround_time_pos-rx_iq_start_tstamp)/SYSTEM_TIMER_TICK_1US);
                            #if (CS_ANTENNA_SWITCHING_DATA_EN)
                                if(csStepIQ_param.d_N_AP == 2){
                                    tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  N_AP=%d,T_SW=%d,ACI=%d,toneQI=%d,pct_I=%hd,pct_Q=%hd,toneQ=%f\n", csStepIQ_param.d_N_AP, csStepIQ_param.d_T_SW, csStepIQ_param.d_ACI, toneAntQualityIndicator, pct_ant_reflector[0], pct_ant_reflector[1], toneAntQuality_raw);
                                }
                            #endif
                            if(csStepIQ_param.tone_ext){
                                tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  tone_ext=1,toneQI=%d,pct_I=%hd,pct_Q=%hd,toneQ=%f\n", toneExtQualityIndicator, pct_ext_reflector[0], pct_ext_reflector[1], toneExtQuality_raw);
                            }
                        #endif
                    }
                    else if(raw_pkt[2] & BLT_CS_MODE_3_FLAG){
                        //tlkapi_send_string_data(DBG_CS_DATA_EN, "reflector_mode_3_mlp_data", raw_pkt, 16);
                        #if (DBG_CS_DATA_PRINT_EN)
                            printf("refl_mode_3_data\n");
                        #endif
                        #if (DBG_CS_DATA_USB_PRINT_EN)
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "refl_mode_3_data\n");
                        #endif
                    }
                }

                if(cs_subeventResultReportType == CS_SUBEVENT_RESULT_EVENT_FIRST){
                    if(hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_CS_SUBEVENT_RESULT)
                    {
                        hci_le_csSubeventResult_evt(0, 0, &pEvt->Subevent_Code, result_len);
                    }

                    if((pEvt->Subevent_Done_Status & 0xF) == CS_SUBEVENT_DONE_STATUS_PARTIAL){
                        cs_subeventResultReportType = CS_SUBEVENT_RESULT_EVENT_CONTINUE;
                    }
                }
                else if(cs_subeventResultReportType == CS_SUBEVENT_RESULT_EVENT_CONTINUE){
                    if(hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_CS_SUBEVENT_RESULT_CONTINUE)
                    {
                        hci_le_csSubeventResultContinue_evt(0, 0, &pEvtConti->Subevent_Code, result_len);
                    }

                    if((pEvtConti->Subevent_Done_Status & 0xF) != CS_SUBEVENT_DONE_STATUS_PARTIAL){
                        cs_subeventResultReportType = CS_SUBEVENT_RESULT_EVENT_FIRST;
                    }
                }
            }

            //raw_pkt[2] = 0;
            cs_rx_fifo.rptr++;
        }
    }
}

void blt_ll_cs_mainloop(void){
    for(int conn_idx=ACL_CONN_IDX_CEN0; conn_idx<LL_MAX_ACL_CONN_NUM; conn_idx++){
        st_ll_conn_t *pc = (st_ll_conn_t*)(u32)&blms[conn_idx];
        cs_param_t *pCsParam = &pc->csParam;

        if(!pc->connState){continue;}

        if((pc->llcp_flag.bit.ll_feat_exg_flag) && ((pCsParam->cs_cap_req & (PROC_SEND_REQ | PROC_SEND_RSP)) || (pCsParam->cs_cap_req == PROC_EVT_PENDING))){
            blt_ll_cs_exchangeCapProc(pc);
        }
        if((pCsParam->cs_security_enable & (PROC_SEND_REQ | PROC_SEND_RSP)) || (pCsParam->cs_security_enable == PROC_EVT_PENDING)){
            blt_ll_cs_exchangeSecurityStartProc(pc);
        }
        if((pCsParam->cs_fae_req & (PROC_SEND_REQ | PROC_SEND_RSP)) || (pCsParam->cs_fae_req == PROC_EVT_PENDING)){
            blt_ll_cs_exchangeFaeTableProc(pc);
        }
        if((pCsParam->cs_config_req & (PROC_SEND_REQ | PROC_SEND_RSP)) || (pCsParam->cs_config_req == PROC_EVT_PENDING)){
            cs_config_t *pCsCfg = gGlobal_pCsCfg + pCsParam->cs_config_pend_idx;
            blt_ll_cs_exchangeConfigReq(pc, pCsCfg);
        }

        if((pCsParam->cs_req & (PROC_SEND_REQ | PROC_SEND_RSP | PROC_SEND_IND)) || (pCsParam->cs_req == PROC_EVT_PENDING)){
            cs_config_t *pCsCfg = gGlobal_pCsCfg + pCsParam->cs_pend_idx;
            blt_ll_cs_exchangeCsStartProc(pc, pCsCfg);
        }

        if((pCsParam->cs_terminate_ind & PROC_SEND_IND ) || (pCsParam->cs_terminate_ind == PROC_EVT_PENDING)){
            cs_config_t *pCsCfg = gGlobal_pCsCfg + pCsParam->cs_pend_idx;
            blt_ll_cs_exchangeCsProcedureRepeatTerminateProc(pc, pCsCfg);
        }

    }
    if(gCsMng.chn_map_upt_tick && clock_time_exceed(gCsMng.chn_map_upt_tick,1000*1000)){
        gCsMng.chn_map_upt_tick = 0;
    }

    blt_ll_cs_data_loop();
}

void blt_le_cs_reset(void){
    for(int conn_idx=ACL_CONN_IDX_CEN0; conn_idx<LL_MAX_ACL_CONN_NUM; conn_idx++){
        st_ll_conn_t *pc = (st_ll_conn_t*)(u32)&blms[conn_idx];
        blc_cs_resetByHandle(pc->acl_conHandle);
    }

    cs_rx_fifo.rptr = cs_rx_fifo.wptr;
    cs_subeventResultReportType = CS_SUBEVENT_RESULT_EVENT_FIRST;

    blt_cs_subevent_rf_deinit(0);
    ble_rf_set_tx_modulation_index(RF_MI_P0p50);

    CS_HCI_LOG("reset hci success");
}

_attribute_no_inline_
int blt_cs_mainloop_task (int flag, void *p){
    (void)p;//clean warning: unused variable 'p' [-Wunused-variable] by SunWei
    if(flag == (int)FLAG_MODULE_MAINLOOP){
        blt_ll_cs_mainloop();
    }
    else if(flag == (int)FLAG_CHECK_INIT){
    }
    else if(flag == (int)FLAG_MODULE_RESET){
        blt_le_cs_reset();
    }
    return 0;
}


ble_sts_t   blc_ll_initCsModule_initConfigParametersBuffer(u8 *pParamBuf, int cs_config_num)
{

    STATIC_ASSERT_FILE(CS_PARAM_LENGTH == sizeof(cs_config_t), chn_sound);

    #if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
        STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(cs_config_t)), chn_sound);
        STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(chn_sound_capbilities_t)), chn_sound);
        STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(cs_param_t)), chn_sound);
    #endif

    LL_FEATURE_MASK_1 |= LL_FEATURE_MASK_CHANNEL_SOUNDING|LL_FEATURE_MASK_CHANNEL_SOUNDING_HOST|LL_FEATURE_MASK_CHANNEL_SOUNDING_PCT_QUALITY_INDICATION;

    ll_chn_sounding_ctrl_handler = blt_ll_cs_ctrl_pdu_proc;
    ll_chn_sounding_irq_task_cb = blt_cs_interrupt_task;
    ll_chn_sounding_mlp_task_cb = blt_cs_mainloop_task;
    gCsMng.max_num_cofig = cs_config_num;

    for(int i = 0 ; i<gCsMng.max_num_cofig; i++)
    {
        cs_config_t *pConfig = ((cs_config_t*)pParamBuf) + i;
        pConfig->idx = i;
        pConfig->occupy = 0;
        pConfig->state = 0;

        for(int j = 0; j <CS_SCH_FIFONUM; j++)
        {
            pConfig->csTskFifo[j].scheTask_oft = TSKOFT_CS + i;
            pConfig->csTskFifo[j].scheTask_idx = i;
            pConfig->csTskFifo[j].scheTask_flg = TSKFLG_CS;
        }
    }
    gGlobal_pCsCfg =(cs_config_t*) pParamBuf;

    return BLE_SUCCESS;
}

#if(0)
void blc_ll_test_CsSpecialSetting(u16 connHandle)
{
    st_ll_conn_t* pAcl                      = (st_ll_conn_t*)blt_ll_getAclConnPtr(connHandle);
    cs_param_t *pCsParam                    = &pAcl->csParam;
    pCsParam->cs_cap_exchange               = 1;
    pCsParam->cs_security_exchange          = 1;
    pCsParam->cs_fae_exchange               = 1;
    u8 st2_h9_cs_iv[16]={0x3b ,0x0b ,0xca ,0xe0 ,0x86 ,0x51 ,0x7f ,0x3e ,\
                                    0xe9 ,0xdf ,0xfd ,0x0b ,0x8a ,0xc2 ,0x0b ,0xe1 };
    u8 st2_h9_cs_in[8]={0x0d ,0x84 ,0x73 ,0x86 ,0xc1 ,0x77 ,0xf4 ,0x9f };
    u8 st2_h9_cs_pv[16]={0x43 ,0xf1 ,0x68 ,0x78 ,0x96 ,0x74 ,0xa6 ,0x64 ,\
                                    0x44 ,0xed ,0x82 ,0x98 ,0xdf ,0xde ,0x80 ,0xc9 };
    smemcpy(&pCsParam->CS_IV[0],st2_h9_cs_iv,16);
    smemcpy(&pCsParam->CS_IN[0],st2_h9_cs_in,8);
    smemcpy(&pCsParam->CS_PV[0],st2_h9_cs_pv,16);
}
#endif

int blc_ll_getCsCapExchStatus(u16 connHandle)
{
    st_ll_conn_t* pAcl                              = (st_ll_conn_t*)blt_ll_getAclConnPtr(connHandle);
    cs_param_t *pCsParam                            = &pAcl->csParam;
    return pCsParam->cs_cap_exchange ;
}
int blc_ll_getCsFaeExchStatus(u16 connHandle)
{
    st_ll_conn_t* pAcl                              = (st_ll_conn_t*)blt_ll_getAclConnPtr(connHandle);
    cs_param_t *pCsParam                            = &pAcl->csParam;
    return pCsParam->cs_fae_exchange ;

}
int blc_ll_getCsSecExchStatus(u16 connHandle)
{
    st_ll_conn_t* pAcl                              = (st_ll_conn_t*)blt_ll_getAclConnPtr(connHandle);
    cs_param_t *pCsParam                            = &pAcl->csParam;
    return pCsParam->cs_security_exchange ;
}



int blc_ll_test_CsFaeExchCtrl(u16 connHandle)
{
    st_ll_conn_t* pAcl                              = (st_ll_conn_t*)blt_ll_getAclConnPtr(connHandle);
    cs_param_t *pCsParam                            = &pAcl->csParam;
    chn_sound_capbilities_t *pcsRemoteSupCap        = &pAcl->csRemoteSupCap;

    if(pCsParam->cs_fae_exchange ){
        return 0;
    }

    if(pCsParam->role_enable & CS_INITIATOR_ROLE){
        if(pcsRemoteSupCap->Optional_Subfeatures_Supported & CS_No_FAE_SUPPORT){//needn't send cs fae req
            if(pCsParam->role_enable & CS_REFLECTOR_ROLE){
                if(bltCsLocalSupportCap.Optional_Subfeatures_Supported & CS_No_FAE_SUPPORT){//needn't peer dev send cs fae req
                    pCsParam->cs_fae_exchange = 1;
                }
                else{
                    return 1;
                    //wait peer dev initiator cs fae excahnge
                }
            }
            else{
                pCsParam->cs_fae_exchange = 1;
            }
        }
        else{
            return 2;//blc_hci_le_cs_readRemoteFAE_table(connHandle);
        }
    }
    else if(pCsParam->role_enable & CS_REFLECTOR_ROLE){
        if(bltCsLocalSupportCap.Optional_Subfeatures_Supported & CS_No_FAE_SUPPORT){//needn't peer dev send cs fae req
            pCsParam->cs_fae_exchange = 1;
        }
        else{
            return 1;
            //wait peer dev initiator cs fae excahnge
        }
    }
    return 0;
}

s32 blt_ll_cs_getStepRxFreqOffset(u8 phy, u8* raw_data)
{
    /**
     * rx freq offset = pkt_fdc x bit_rate x 4000 / 256 / 2 / 3.14159
     * 1M PHY, 1M_bit_rate = 1000, 1M_FO = pkt_fdc x 2487
     * 2M PHY, 2M_bit_rate = 2000, 2M_FO = pkt_fdc x 4974
     */
    s32 freq_offset;

    freq_offset = ((raw_data[DMA_CS_RFRX_OFFSET_FREQ_OFFSET(raw_data) + 1] & 0x07) << 8) | raw_data[DMA_CS_RFRX_OFFSET_FREQ_OFFSET(raw_data)];
    freq_offset = ((freq_offset > 0x3ff) ? (freq_offset - 0x800) : freq_offset);
    if(phy == BLE_2M_PHY){
        freq_offset *= 4974;//Hz
    }
    else{
        freq_offset *= 2487;//Hz
    }

    return freq_offset;
}

void blt_ll_cs_getStepIQParam(u8 role, u8 step_mode, u8* raw_data, cs_step_IQ_param_t *step_param)
{
    step_param->startIQIdx = DMA_CS_RFRX_OFFSET_IQ_DATA;
    step_param->tone_ext = 0;

    if(step_mode == STEP_MODE_0){
        if(role == BLT_CS_INITIATOR_FLAG){
            step_param->lenIQ = DMA_CS_RFRX_IQ_DATA_LEN(raw_data);
        }
        else{
            step_param->lenIQ = 0;
            step_param->startIQIdx += (IQ_20_BIT_MODE >> 8);
        }
    }
    else if(step_mode == STEP_MODE_1){
        step_param->lenIQ = DMA_CS_RFRX_IQ_DATA_LEN(raw_data) - (((CS_RFRXEN_MODE_EARLY_US + CS_RF_RX_1M_EXTRA_PREAMBLE_US + (CS_RF_RX_1M_WINDOW_EXTEND_US * 3 / 4)) * (IQ_20_BIT_MODE >> 8)) << 2);
        step_param->startIQIdx += (((CS_RFRXEN_MODE_EARLY_US + CS_RF_RX_1M_EXTRA_PREAMBLE_US) * (IQ_20_BIT_MODE >> 8)) << 2);
    }
    else if(step_mode == STEP_MODE_2){
        if(raw_data[2] & BLT_CS_STEP_TONE_EXTENSION_SLOT_FLAG){
            step_param->tone_ext = 1;
        }

        step_param->d_T_SW = raw_data[DMA_CS_RFRX_OFFSET_T_SW_LOW4BIT(raw_data)] & 0xF;

        #if (CS_ANTENNA_SWITCHING_DATA_EN)
            step_param->d_N_AP = (raw_data[DMA_CS_RFRX_OFFSET_NUM_ANTENNA_PATHS_HIGH4BIT(raw_data)] & 0xF0) >> 4;
            step_param->d_ACI = (raw_data[DMA_CS_RFRX_OFFSET_ACI_HIGH4BIT(raw_data)] & 0xF0) >> 4;

            /* Tx and Rx skip the first T_SW
             *      ANT1                        ANT2                ext_slot
             *  T_SW + T_PM         +       T_SW + T_PM     +       T_SW + T_PM
             *  Tx/Rx  |<---Start                                      End--->|
             *  lenIQ  |<---                                                   ---T_SW--->|
             */
            step_param->lenIQ = DMA_CS_RFRX_IQ_DATA_LEN(raw_data) - (((CS_RFRXEN_MODE_EARLY_US - step_param->d_T_SW) * (IQ_20_BIT_MODE >> 8)) << 2);

            if(step_param->d_N_AP == 1){
                if(step_param->tone_ext){
                    step_param->lenIQ /= 2;
                }
            }
            else if(step_param->d_N_AP == 2){
                if(step_param->tone_ext){
                    step_param->lenIQ /= 3;
                }
                else{
                    step_param->lenIQ /= 2;
                }
            }
            else{
                #if (DBG_CS_DATA_EN)
                    tlkapi_send_string_u32s(DBG_CS_DATA_EN, "cs_getStepIQParam d_N_AP error", role, step_mode, step_param->lenIQ, step_param->d_N_AP);
                    BLMS_ERR_DEBUG(DBG_CS_DATA_EN, 0x55550004);
                #endif
            }

            step_param->startIQIdx += ((CS_RFRXEN_MODE_EARLY_US * (IQ_20_BIT_MODE >> 8)) << 2);
        #else
            step_param->lenIQ = DMA_CS_RFRX_IQ_DATA_LEN(raw_data) - ((CS_RFRXEN_MODE_EARLY_US * (IQ_20_BIT_MODE >> 8)) << 2);

            if(step_param->tone_ext){
                step_param->lenIQ /= 2;
            }

            step_param->startIQIdx += ((CS_RFRXEN_MODE_EARLY_US * (IQ_20_BIT_MODE >> 8)) << 2);
        #endif
    }
    else if(step_mode == STEP_MODE_3){

    }

    step_param->lenSample = step_param->lenIQ / 5;

    #if (DBG_CS_DATA_EN)
        if(step_param->lenIQ > LL_CS_STEP_IQ_LEN_MAX){
            tlkapi_send_string_u32s(DBG_CS_DATA_EN, "cs_getStepIQParam lenIQ error", role, step_mode, step_param->lenIQ, step_param->lenSample);
            BLMS_ERR_DEBUG(DBG_CS_DATA_EN, 0x55550001);
        }
    #endif
}

u8 blt_ll_cs_getPktMatchSyncQuality(u8* raw_data)
{
    u8 pktSyncQuality = raw_data[DMA_CS_RFRX_OFFSET_PKT_MATCH_SYNC(raw_data)];

    pktSyncQuality = CS_ACCESS_ADDRESS_BIT_SIZE - pktSyncQuality;

    return  (pktSyncQuality > 2 ? 2 : pktSyncQuality);
}

_attribute_ram_code_ void blt_ll_cs_Convert20BitIQ2int(u8 *data_src, s32 *data_dest, u16 len_sample)
{
    u32 i;

    for(i = 0; i < len_sample; i++)
    {
        data_dest[i*2] = data_src[i*5] + (data_src[i*5+1] << 8) + ((data_src[i*5+2] & 0x0F) << 16);
        data_dest[i*2 + 1] = ((data_src[i*5+2] & 0xF0) >> 4) + (data_src[i*5+3] << 4) + ((data_src[i*5+4]) << 12);
    }
    for(i = 0;i < 2*len_sample;i++)
    {
        if(data_dest[i] > (s32)BIT(19))
        {
            data_dest[i] = data_dest[i] - BIT(20);
        }
    }
}

s32 blc_ll_cs_getMode1InternalCircuitDelay(cs_param_role_t role)
{
    s32 internalCircuitDelay;
    if(role == (cs_param_role_t)CS_CONFIG_INITIATOR_ROLE){
        internalCircuitDelay = cs_internalDelay[0];
    }
    else{
        internalCircuitDelay = cs_internalDelay[1];
    }

    return internalCircuitDelay;
}

void blc_ll_cs_setMode1InternalCircuitDelay(cs_param_role_t role, s32 circuit_delay)
{
    if(role == (cs_param_role_t)CS_CONFIG_INITIATOR_ROLE){
        cs_internalDelay[0] = circuit_delay;
    }
    else{
        cs_internalDelay[1] = circuit_delay;
    }
}

u8 blt_ll_cs_getToneQualityIndicator(float toneQualityRaw)
{
    u8 toneQuality_indicator;

    if(toneQualityRaw > cs_thresGood){
        toneQuality_indicator = CS_STEP_RECEIVE_TONE_QUALITY_GOOD;
    }
    else if(toneQualityRaw < cs_thresBad){
        toneQuality_indicator = CS_STEP_RECEIVE_TONE_QUALITY_LOW;
    }
    else{
        toneQuality_indicator = CS_STEP_RECEIVE_TONE_QUALITY_MEDIUM;
    }

    return toneQuality_indicator;
}

//int blt_cs_initModule(void){
//
//  for(int conn_idx=ACL_CONN_IDX_CEN0; conn_idx<LL_MAX_ACL_CONN_NUM; conn_idx++){
//      st_ll_conn_t *pc = (st_ll_conn_t*)&blms[conn_idx];
//      for(int cs_idx = 0; cs_idx<MAX_NUM_CS_CONFIG_PER_ACL; cs_idx++){
//          cs_config_t *config = &pc->csConfig[cs_idx];
//          config->idx = cs_idx;
//      }
//  }
//
//  return 0;
//}

#endif


#if (DBG_CS_SUBEVENT_ENABLE)
_attribute_ble_data_retention_ rf_packet_cs_t pkt_CS;
_attribute_ble_data_retention_ u32 cs_procedure_start_tick;
_attribute_ble_data_retention_ u8 cs_mode0_rx_flag;


#if (DBG_CS_RX_FIFO_ENABLE)
    #define         CS_RX_FIFO_SIZE             DMA_CS_RFRX_MAX_DMA_LEN
    #define         CS_RX_FIFO_NUM              64
    _attribute_ble_data_retention_ u8           cs_rx_fifo_b[CS_RX_FIFO_SIZE * CS_RX_FIFO_NUM] = {0};
    _attribute_ble_data_retention_ cs_fifo_t    cs_rx_fifo_test = {
                                                    CS_RX_FIFO_SIZE,
                                                    CS_RX_FIFO_NUM,
                                                    0,
                                                    0,
                                                    cs_rx_fifo_b};
    _attribute_ble_data_retention_ u8*          cs_rx_buff;
#else
    _attribute_ble_data_retention_ u8           cs_rx_buff[DMA_CS_RFRX_MAX_DMA_LEN];
#endif


ble_sts_t   blc_ll_initCsRxFifo_test(void)
{
    cs_rx_fifo_test.wptr = cs_rx_fifo_test.rptr = 0;
    cs_rx_buff = cs_rx_fifo_test.p_base + (cs_rx_fifo_test.wptr * cs_rx_fifo_test.size);

    return BLE_SUCCESS;
}

_attribute_ram_code_ void blt_ll_csRxFifoUpdate(void)
{
    cs_rx_fifo_test.wptr == (cs_rx_fifo_test.num - 1) ? cs_rx_fifo_test.wptr = 0 : cs_rx_fifo_test.wptr++;
    cs_rx_buff = cs_rx_fifo_test.p_base + (cs_rx_fifo_test.wptr * cs_rx_fifo_test.size);
}


///////////////////////////////////////////////////////////////////////////////
#if (0)
    float compArr[160 * 2] = {0.0};//40us * 4 = 160
    int Sample_IQdata[1284] = {0};//
    int Init_IQData[240 * 2 * 40];
    u32 init_tr_turnaround_time_pos[80]  __attribute__ ((aligned (4)));
    u32 init_tr_turnaround_time_neg[80]  __attribute__ ((aligned (4)));
    u32 initiator_iq_start_tstamp[80]  __attribute__ ((aligned (4)));
    u8 qualityIndicatorInit[80];

    float angleStep = 0;//2 * PI * cfo /  SAMPLERATE;

    //need to scan CHANNUM both at reflector and initiator side
    int pct_refl[CHANNUM*2];
    int pct_init[CHANNUM*2];
#endif
///////////////////////////////////////////////////////////////////////////////

void blt_ll_cs_main_loop_test(void)
{
    if(cs_rx_fifo_test.rptr != cs_rx_fifo_test.wptr){
        while (cs_rx_fifo_test.rptr != cs_rx_fifo_test.wptr)
        {
            u8 *raw_pkt = (u8 *)(cs_rx_fifo_test.p_base + cs_rx_fifo_test.rptr * cs_rx_fifo_test.size);

            if(raw_pkt[2])
            {
                u8 csChannel = raw_pkt[3] & BLT_CS_STEP_CHANNEL_MASK;

                u8 role;
                s32 initial_IQData[LL_CS_STEP_IQ_NUM_MAX];

                u8 packetSyncFlag = 0;
                u8 packetQuality = CS_STEP_RECEIVE_PACKET_QUALITY_LOW;
                u8 packetNADM = CS_STEP_RECEIVE_PACKET_NADM_UNKNOWN;
                s8 packetRSSI = 0x7F;
                u8 toneQualityIndicator = CS_STEP_RECEIVE_TONE_QUALITY_UNAVAILABLE;
                u8 toneExtQualityIndicator = CS_STEP_RECEIVE_TONE_QUALITY_UNAVAILABLE;

                if(raw_pkt[2] & BLT_CS_MODE_RX_FLAG){
                    packetSyncFlag = (raw_pkt[DMA_CS_RFRX_OFFSET_SYNC_FLAG(raw_pkt)] & BIT(3)) >> 3;

                    if(packetSyncFlag){
                        packetQuality = blt_ll_cs_getPktMatchSyncQuality(raw_pkt);
                        packetRSSI = raw_pkt[DMA_CS_RFRX_OFFSET_RSSI(raw_pkt)] - 110;
                    }
                }

                u32 tx_turnaround_time_pos;//start point of tx turnaround
                u32 tx_turnaround_time_neg;//end point of tx turnaround
                u32 tx_on_start_tstamp;//start point of tx on
                u32 rx_iq_start_tstamp;
                u32 rx_pkt_iq_sync_tstamp;
                u32 cs_rx_accessAddr;
                #if (DBG_CS_DATA_PRINT_EN || DBG_CS_DATA_USB_PRINT_EN)
                    u8 cs_rx_agcGain = raw_pkt[DMA_CS_RFRX_OFFSET_RX_AGC_GAIN(raw_pkt)];
                #endif

                int pct_initiator[2];
                int pct_ext_initiator[2];
                int pct_reflector[2];
                int pct_ext_reflector[2];
                float toneQuality_raw;
                float toneExtQuality_raw;

                short cte_initiator = 0x8000;//Time difference is not available
                short cte_reflector = 0x8000;//Time difference is not available

                u8 eventResult[255];//length < MAX HCIevent size
                u16 result_len = sizeof(hci_le_csSubeventResultEvt_t) + 3;
                hci_le_csSubeventResultEvt_t *pEvt = (hci_le_csSubeventResultEvt_t *)eventResult;

                //for test
                pEvt->Subevent_Code = HCI_SUB_EVT_LE_CS_SUBEVENT_RESULT;
                pEvt->Connection_Handle = 0xC0B0;
                pEvt->Config_ID = 0;
                pEvt->Start_ACL_Conn_Event = 0x3412;
                pEvt->Procedure_Counter = 0x7856;
                pEvt->Frequency_Compensation = 0;//0 ppm
                pEvt->Reference_Power_Level = 10;//10 dBm
                pEvt->Procedure_Done_Status = 0;
                pEvt->Subevent_Done_Status = 0;
                pEvt->Abort_Reason = 0;
                pEvt->Num_Antenna_Paths = 1;
                pEvt->Num_Steps_Reported = 1;

                if(raw_pkt[2] & BLT_CS_INITIATOR_FLAG){
                    role = BLT_CS_INITIATOR_FLAG;

                    if(raw_pkt[2] & BLT_CS_MODE_0_FLAG){
                        tlkapi_send_string_data(DBG_CS_DATA_EN, "initiator_mode_0_mlp_data", raw_pkt, 16);

                        cs_step_IQ_param_t *stepIQ_param = (cs_step_IQ_param_t *)&csStepIQ_param;
                        blt_ll_cs_getStepIQParam(role, STEP_MODE_0, raw_pkt, stepIQ_param);

                        DBG_CHN5_HIGH;//test hadm algorithm time start
                        s32 rx_freq_offset;
                        float cfoCoarse;
                        float mfo = 0.0;
                        s16 cs_mfo = 0;
                        if(packetSyncFlag){
                            blt_ll_cs_Convert20BitIQ2int(&raw_pkt[csStepIQ_param.startIQIdx], &initial_IQData[0], csStepIQ_param.lenSample);
                            float IQData_float[LL_CS_STEP_IQ_LEN_MAX];
                            for (u16 i = 0; i < csStepIQ_param.lenIQ; i++){
                                IQData_float[i] = (float)initial_IQData[i];
                            }
                            rx_freq_offset = blt_ll_cs_getStepRxFreqOffset(BLE_1M_PHY, raw_pkt);
                            cfoCoarse = (float)rx_freq_offset;
                            cs_cfo = calcFreq(&IQData_float[0], csStepIQ_param.lenSample, cfoCoarse, SAMPLERATE);
                            cs_angleStep = 2 * PI * cs_cfo / SAMPLERATE;
                            calcCompensate(cs_compArr, csStepIQ_param.lenSample, -cs_angleStep);

                            mfo = (float)((2402 + csChannel) * 1000); //kHz
                            mfo = (cs_cfo / 1000) / mfo;
                            mfo = mfo * (1e6);//Units: 1 ppm
                            cs_mfo = (s16)(mfo * 100);//Units: 0.01 ppm
                        }
                        DBG_CHN5_LOW;//test hadm algorithm time post

                        pEvt->Step_Mode->mode = STEP_MODE_0;
                        pEvt->Step_Mode->channel = csChannel;
                        pEvt->Step_Mode->len = CS_STEP_DATA_LENGTH_MODE0_INITIATOR;

                        u8 mode0Result[CS_STEP_DATA_LENGTH_MODE0_INITIATOR];
                        cs_step_mode0_t *pMode0 = (cs_step_mode0_t *)mode0Result;

                        pMode0->Packet_Quality = packetQuality;
                        pMode0->Packet_RSSI = packetRSSI;
                        pMode0->Packet_Antenna = 1;
                        pMode0->Measured_Freq_Offset = cs_mfo;
                        smemcpy(pEvt->Step_Mode->data, mode0Result, CS_STEP_DATA_LENGTH_MODE0_INITIATOR);
                        result_len += CS_STEP_DATA_LENGTH_MODE0_INITIATOR;

                        int debug_cfo = (int)cs_cfo;
                        tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", csStepIQ_param.lenIQ, packetSyncFlag, packetQuality, debug_cfo);
                        #if (DBG_CS_DATA_PRINT_EN)
                            printf("init_mode0_data,cfo=%f,freq_off=%d,mfo=%f\n", cs_cfo, rx_freq_offset, mfo);
                            printf("  lenIQ=%d,SyncFlag=%d,Quality=%d,agc_gain=%d,RSSI=%d\n", csStepIQ_param.lenIQ, packetSyncFlag, packetQuality, cs_rx_agcGain, packetRSSI);
                            //printf("  lenIQ=%d,lenSample=%d,startIQIdx=%d\n", csStepIQ_param.lenIQ, csStepIQ_param.lenSample, csStepIQ_param.startIQIdx);
                        #endif
                        #if (DBG_CS_DATA_USB_PRINT_EN)
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "init_mode0_data,cfo=%f,freq_off=%d,mfo=%f\n", cs_cfo, rx_freq_offset, mfo);
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  lenIQ=%d,SyncFlag=%d,Quality=%d,agc_gain=%d,RSSI=%d\n", csStepIQ_param.lenIQ, packetSyncFlag, packetQuality, cs_rx_agcGain, packetRSSI);
                            //tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  lenIQ=%d,lenSample=%d,startIQIdx=%d\n", csStepIQ_param.lenIQ, csStepIQ_param.lenSample, csStepIQ_param.startIQIdx);
                        #endif
                    }
                    else if(raw_pkt[2] & BLT_CS_MODE_1_FLAG){
                        tlkapi_send_string_data(DBG_CS_DATA_EN, "initiator_mode_1_mlp_data", raw_pkt, 16);

                        cs_step_IQ_param_t *stepIQ_param = (cs_step_IQ_param_t *)&csStepIQ_param;
                        blt_ll_cs_getStepIQParam(role, STEP_MODE_1, raw_pkt, stepIQ_param);

                        BYTE_TO_UINT32(tx_on_start_tstamp, &raw_pkt[DMA_CS_RFRX_OFFSET_TX_ON_TSTAMP_4BYTE(raw_pkt)]);
                        BYTE_TO_UINT32(cs_rx_accessAddr, &raw_pkt[DMA_CS_RFRX_OFFSET_RX_ACCESS_ADDRESS_4BYTE(raw_pkt)]);
                        BYTE_TO_UINT32(rx_pkt_iq_sync_tstamp, &raw_pkt[DMA_CS_RFRX_OFFSET_TIME_STAMP(raw_pkt)]);

                        BYTE_TO_UINT32(rx_iq_start_tstamp, &raw_pkt[DMA_CS_RFRX_OFFSET_IQ_START_TSTAMP(raw_pkt)]);//for test
                        u8 pktSyncBitNum = raw_pkt[DMA_CS_RFRX_OFFSET_PKT_MATCH_SYNC(raw_pkt)];//for test

                        DBG_CHN5_HIGH;//test hadm algorithm time start
                        if(packetSyncFlag){
                            if(tx_on_start_tstamp != rx_pkt_iq_sync_tstamp){
                                int cs_aaCode[CS_ACCESS_ADDRESS_BIT_SIZE];
                                for (u16 i = 0; i < CS_ACCESS_ADDRESS_BIT_SIZE; i++){
                                    cs_aaCode[i] = (cs_rx_accessAddr >> i) & 1;
                                }
                                //tlkapi_printf(DBG_CS_DATA_EN, "cs_rx_accessAddr,%s", hex_to_str(cs_aaCode, CS_ACCESS_ADDRESS_BIT_SIZE));

                                int dataRate = 1e6; // for 1M case now
                                /**
                                 *  First param: number of step
                                 *  Second param: role, INITIATOR---0,REFLECTOR---1
                                 */
                                parameterPesCollectDataSDK paraPesSDK = pesCollectDataInitSDK(1, 0, dataRate, cs_aaCode, CS_ACCESS_ADDRESS_BIT_SIZE, cs_internalDelay, cs_adThr, cs_adStep);
                                u32 t_sy_center_delta_init = 194 * 2 * 1000;//(44+5+145)*1e-6/0.5e-9
                                calcPesInfoSDK(&tx_on_start_tstamp, &rx_pkt_iq_sync_tstamp, t_sy_center_delta_init, &cte_initiator, paraPesSDK);

                                #if (0)
                                    blt_ll_cs_Convert20BitIQ2int(&raw_pkt[csStepIQ_param.startIQIdx], &initial_IQData[0], csStepIQ_param.lenSample);

                                    float rdm = 0.0;// Detector Metrics
                                    //currently consume 2.3ms under CCLK_96M
                                    packetNADM = calcPesNadm(initial_IQData, csStepIQ_param.lenSample, -cs_cfo, cs_nadm_adtype, &rdm, paraPesSDK);
                                #endif
                            }
                            else{
                                packetQuality = CS_STEP_RECEIVE_PACKET_QUALITY_LOW;
                            }
                        }
                        DBG_CHN5_LOW;//test hadm algorithm time post

                        pEvt->Step_Mode->mode = STEP_MODE_1;
                        pEvt->Step_Mode->channel = csChannel;
                        pEvt->Step_Mode->len = CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY;

                        u8 mode1Result[CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY];
                        cs_step_mode1_t *pMode1 = (cs_step_mode1_t *)mode1Result;

                        pMode1->Packet_Quality = packetQuality;
                        pMode1->Packet_NADM = packetNADM;
                        pMode1->Packet_RSSI = packetRSSI;

                        pMode1->ToA_ToD[0] = U16_LO(cte_initiator);
                        pMode1->ToA_ToD[1] = U16_HI(cte_initiator);
                        pMode1->Packet_Antenna = 1;
                        smemcpy(pEvt->Step_Mode->data, pMode1, CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY);
                        result_len += CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY;

                        //tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", csStepIQ_param.lenIQ, csStepIQ_param.lenSample, csStepIQ_param.startIQIdx, 0);
                        tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", cs_rx_accessAddr, tx_on_start_tstamp / SYSTEM_TIMER_TICK_1US, rx_iq_start_tstamp / SYSTEM_TIMER_TICK_1US, rx_pkt_iq_sync_tstamp / SYSTEM_TIMER_TICK_1US);
                        tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", csStepIQ_param.lenIQ, rx_pkt_iq_sync_tstamp - tx_on_start_tstamp, (rx_pkt_iq_sync_tstamp - rx_iq_start_tstamp) / SYSTEM_TIMER_TICK_1US, (rx_pkt_iq_sync_tstamp - tx_on_start_tstamp) / SYSTEM_TIMER_TICK_1US);
                        tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", packetSyncFlag, packetQuality, pktSyncBitNum, cte_initiator);
                        //tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", pMode1->Packet_Quality, pMode1->Packet_NADM, pMode1->Packet_RSSI, cte_initiator);

                        #if (DBG_CS_DATA_PRINT_EN)
                            printf("init_mode_1_data,sync-tx_on=%d,ToA_ToD=%hd\n", rx_pkt_iq_sync_tstamp - tx_on_start_tstamp, cte_initiator);
                            printf("  lenIQ=%d,SyncFlag=%d,Quality=%d,NADM=%d,agc_gain=%d,RSSI=%d\n", csStepIQ_param.lenIQ, packetSyncFlag, packetQuality, packetNADM, cs_rx_agcGain, packetRSSI);
                        #endif
                        #if (DBG_CS_DATA_USB_PRINT_EN)
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "init_mode_1_data,sync-tx_on=%d,ToA_ToD=%hd\n", rx_pkt_iq_sync_tstamp - tx_on_start_tstamp, cte_initiator);
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  lenIQ=%d,SyncFlag=%d,Quality=%d,NADM=%d,agc_gain=%d,RSSI=%d\n", csStepIQ_param.lenIQ, packetSyncFlag, packetQuality, packetNADM, cs_rx_agcGain, packetRSSI);
                        #endif
                    }
                    else if(raw_pkt[2] & BLT_CS_MODE_2_FLAG){
                        tlkapi_send_string_data(DBG_CS_DATA_EN, "initiator_mode_2_mlp_data", raw_pkt, 16);

                        cs_step_IQ_param_t *stepIQ_param = (cs_step_IQ_param_t *)&csStepIQ_param;
                        blt_ll_cs_getStepIQParam(role, STEP_MODE_2, raw_pkt, stepIQ_param);

                        BYTE_TO_UINT32(tx_turnaround_time_neg, &raw_pkt[DMA_CS_RFRX_OFFSET_PKT_TX_NEG_TSTAMP(raw_pkt)]);
                        BYTE_TO_UINT32(rx_iq_start_tstamp, &raw_pkt[DMA_CS_RFRX_OFFSET_IQ_START_TSTAMP(raw_pkt)]);

                        cs_cfo = 0;//from Haili 20231030, the cfo at both ends should be opposite, reflector_cfo fixed to zero

                        DBG_CHN5_HIGH;//test hadm algorithm time start
                        #if (1)
                            u16 valid_lenSample;
                            u16 valid_startIQIdx;
                            valid_lenSample = csStepIQ_param.lenSample - ((CS_RX_1M_TONE_HALF_EXCLUSION_PERIOD_US * 2) << 2);
                            valid_startIQIdx = csStepIQ_param.startIQIdx + ((CS_RX_1M_TONE_HALF_EXCLUSION_PERIOD_US * (IQ_20_BIT_MODE >> 8)) << 2);
                            blt_ll_cs_Convert20BitIQ2int(&raw_pkt[valid_startIQIdx], &initial_IQData[0], valid_lenSample);
                            toneQuality_raw = calcTesInfo(&initial_IQData[0], cs_compArr, valid_lenSample, (u32)(rx_iq_start_tstamp - cs_procedure_start_tick), (u32)(tx_turnaround_time_neg - cs_procedure_start_tick), -cs_cfo, cs_if_adjustment79[csChannel], &(pct_initiator[0]), cs_thresGood, cs_thresBad);
                            toneQualityIndicator = blt_ll_cs_getToneQualityIndicator(toneQuality_raw);
                            compressTesInfo(pct_initiator, 2, 12);
                            if(csStepIQ_param.tone_ext){
                                blt_ll_cs_Convert20BitIQ2int(&raw_pkt[valid_startIQIdx + csStepIQ_param.lenIQ], &initial_IQData[0], valid_lenSample);
                                toneExtQuality_raw = calcTesInfo(&initial_IQData[0], cs_compArr, valid_lenSample, (u32)(rx_iq_start_tstamp - cs_procedure_start_tick), (u32)(tx_turnaround_time_neg - cs_procedure_start_tick), -cs_cfo, cs_if_adjustment79[csChannel], &(pct_ext_initiator[0]), cs_thresGood, cs_thresBad);
                                toneExtQualityIndicator = blt_ll_cs_getToneQualityIndicator(toneExtQuality_raw);
                                compressTesInfo(pct_ext_initiator, 2, 12);
                            }
                        #else
                            blt_ll_cs_Convert20BitIQ2int(&raw_pkt[csStepIQ_param.startIQIdx], &initial_IQData[0], csStepIQ_param.lenSample);
                            toneQuality_raw = calcTesInfo(&initial_IQData[0], cs_compArr, csStepIQ_param.lenSample, (u32)(rx_iq_start_tstamp - cs_procedure_start_tick), (u32)(tx_turnaround_time_neg - cs_procedure_start_tick), -cs_cfo, cs_if_adjustment79[csChannel], &(pct_initiator[0]), cs_thresGood, cs_thresBad);
                            toneQualityIndicator = blt_ll_cs_getToneQualityIndicator(toneQuality_raw);
                            compressTesInfo(pct_initiator, 2, 12);
                            if(csStepIQ_param.tone_ext){
                                blt_ll_cs_Convert20BitIQ2int(&raw_pkt[csStepIQ_param.startIQIdx + csStepIQ_param.lenIQ], &initial_IQData[0], csStepIQ_param.lenSample);
                                toneExtQuality_raw = calcTesInfo(&initial_IQData[0], cs_compArr, csStepIQ_param.lenSample, (u32)(rx_iq_start_tstamp - cs_procedure_start_tick), (u32)(tx_turnaround_time_neg - cs_procedure_start_tick), -cs_cfo, cs_if_adjustment79[csChannel], &(pct_ext_initiator[0]), cs_thresGood, cs_thresBad);
                                toneExtQualityIndicator = blt_ll_cs_getToneQualityIndicator(toneExtQuality_raw);
                                compressTesInfo(pct_ext_initiator, 2, 12);
                            }
                        #endif
                        DBG_CHN5_LOW;//test hadm algorithm time post
                        tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", csStepIQ_param.lenSample, toneQualityIndicator, csStepIQ_param.tone_ext, toneExtQualityIndicator);

                        pEvt->Step_Mode->mode = STEP_MODE_2;
                        pEvt->Step_Mode->channel = csChannel;
                        pEvt->Step_Mode->len = CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1;

                        u8 mode2Result[CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1];
                        cs_step_mode2_t *pMode2 = (cs_step_mode2_t *)mode2Result;
                        pMode2->Antenna_Permutation_Index = 0;
                        pMode2->Tone[0].Tone_PCT[0] = U16_LO(pct_initiator[0]);
                        pMode2->Tone[0].Tone_PCT[1] = ((pct_initiator[0] & 0xF00) >> 8) | ((pct_initiator[1] & 0xF) << 4);
                        pMode2->Tone[0].Tone_PCT[2] = U16_LO((pct_initiator[1] >> 4));
                        pMode2->Tone[0].Tone_Quality_Indicator = toneQualityIndicator;
                        pMode2->Tone[1].Tone_PCT[0] = U16_LO(pct_ext_initiator[0]);
                        pMode2->Tone[1].Tone_PCT[1] = ((pct_ext_initiator[0] & 0xF00) >> 8) | ((pct_ext_initiator[1] & 0xF) << 4);
                        pMode2->Tone[1].Tone_PCT[2] = U16_LO((pct_ext_initiator[1] >> 4));
                        if(raw_pkt[2] & BLT_CS_STEP_TONE_EXTENSION_SLOT_FLAG){
                            pMode2->Tone[1].Tone_Quality_Indicator = toneExtQualityIndicator | BIT(5);
                        }
                        else{
                            pMode2->Tone[1].Tone_Quality_Indicator = toneExtQualityIndicator | BIT(4);
                        }
                        smemcpy(pEvt->Step_Mode->data, mode2Result, CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1);
                        result_len += CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1;

                        #if (DBG_CS_DATA_PRINT_EN)
                            printf("init_mode_2_data,agc_gain=%d,toneQI=%d,pct_I=%hd,pct_Q=%hd,pct_I=0x%hx,pct_Q=0x%hx,toneQ=%f\n", cs_rx_agcGain, toneQualityIndicator, pct_initiator[0], pct_initiator[1], pct_initiator[0], pct_initiator[1], toneQuality_raw);
                            printf("  lenIQ=%d,lenSample=%d,startIQIdx=%d,valid_lenSample=%d,valid_startIQIdx=%d\n", csStepIQ_param.lenIQ, csStepIQ_param.lenSample, csStepIQ_param.startIQIdx, valid_lenSample, valid_startIQIdx);
                            printf("  channel=%d,iq_start=%d,tx_off=%d,cs_start=%d,iq_start-tx_off=%d\n", csChannel, rx_iq_start_tstamp/SYSTEM_TIMER_TICK_1US, tx_turnaround_time_neg/SYSTEM_TIMER_TICK_1US, cs_procedure_start_tick/SYSTEM_TIMER_TICK_1US, (rx_iq_start_tstamp-tx_turnaround_time_neg)/SYSTEM_TIMER_TICK_1US);
                            if(csStepIQ_param.tone_ext){
                                printf("  tone_ext=1,toneQI=%d,pct_I=%hd,pct_Q=%hd,pct_I=0x%hx,pct_Q=0x%hx,toneQ=%f\n", toneExtQualityIndicator, pct_ext_initiator[0], pct_ext_initiator[1], pct_ext_initiator[0], pct_ext_initiator[1], toneExtQuality_raw);
                            }
                        #endif
                        #if (DBG_CS_DATA_USB_PRINT_EN)
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "init_mode_2_data,agc_gain=%d,toneQI=%d,pct_I=%hd,pct_Q=%hd,pct_I=0x%hx,pct_Q=0x%hx,toneQ=%f\n", cs_rx_agcGain, toneQualityIndicator, pct_initiator[0], pct_initiator[1], pct_initiator[0], pct_initiator[1], toneQuality_raw);
                            //tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  lenIQ=%d,lenSample=%d,startIQIdx=%d,valid_lenSample=%d,valid_startIQIdx=%d\n", csStepIQ_param.lenIQ, csStepIQ_param.lenSample, csStepIQ_param.startIQIdx, valid_lenSample, valid_startIQIdx);
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  channel=%d,iq_start=%d,tx_off=%d,cs_start=%d,iq_start-tx_off=%d\n", csChannel, rx_iq_start_tstamp/SYSTEM_TIMER_TICK_1US, tx_turnaround_time_neg/SYSTEM_TIMER_TICK_1US, cs_procedure_start_tick/SYSTEM_TIMER_TICK_1US, (rx_iq_start_tstamp-tx_turnaround_time_neg)/SYSTEM_TIMER_TICK_1US);
                            if(csStepIQ_param.tone_ext){
                                tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  tone_ext=1,toneQI=%d,pct_I=%hd,pct_Q=%hd,pct_I=0x%hx,pct_Q=0x%hx,toneQ=%f\n", toneExtQualityIndicator, pct_ext_initiator[0], pct_ext_initiator[1], pct_ext_initiator[0], pct_ext_initiator[1], toneExtQuality_raw);
                            }
                        #endif
                    }
                    else if(raw_pkt[2] & BLT_CS_MODE_3_FLAG){
                        tlkapi_send_string_data(DBG_CS_DATA_EN, "initiator_mode_3_mlp_data", raw_pkt, 16);
                        #if (DBG_CS_DATA_PRINT_EN)
                            printf("init_mode_3_data\n");
                        #endif
                        #if (DBG_CS_DATA_USB_PRINT_EN)
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "init_mode_3_data\n");
                        #endif
                    }
                }
                else if(raw_pkt[2] & BLT_CS_REFLECTOR_FLAG){
                    role = BLT_CS_REFLECTOR_FLAG;

                    if(raw_pkt[2] & BLT_CS_MODE_0_FLAG){
                        tlkapi_send_string_data(DBG_CS_DATA_EN, "reflector_mode_0_mlp_data", raw_pkt, 16);

                        cs_step_IQ_param_t *stepIQ_param = (cs_step_IQ_param_t *)&csStepIQ_param;
                        blt_ll_cs_getStepIQParam(role, STEP_MODE_0, raw_pkt, stepIQ_param);

                        DBG_CHN5_HIGH;//test hadm algorithm time start
                        cs_cfo = cs_angleStep = 0;
                        calcCompensate(cs_compArr, LL_CS_STEP_IQ_NUM_MAX / 2, -cs_angleStep);

                        s32 rx_freq_offset;
                        if(packetSyncFlag){
                            rx_freq_offset = blt_ll_cs_getStepRxFreqOffset(BLE_1M_PHY, raw_pkt);
                        }
                        DBG_CHN5_LOW;//test hadm algorithm time post

                        pEvt->Step_Mode->mode = STEP_MODE_0;
                        pEvt->Step_Mode->channel = csChannel;
                        pEvt->Step_Mode->len = CS_STEP_DATA_LENGTH_MODE0_REFLECTOR;

                        u8 mode0Result[CS_STEP_DATA_LENGTH_MODE0_REFLECTOR];
                        cs_step_mode0_t *pMode0 = (cs_step_mode0_t *)mode0Result;
                        pMode0->Packet_Quality = packetQuality;
                        pMode0->Packet_RSSI = packetRSSI;
                        pMode0->Packet_Antenna = 1;
                        smemcpy(pEvt->Step_Mode->data, mode0Result, CS_STEP_DATA_LENGTH_MODE0_REFLECTOR);
                        result_len += CS_STEP_DATA_LENGTH_MODE0_REFLECTOR;

                        #if (DBG_CS_DATA_PRINT_EN)
                            printf("refl_mode_0_data,lenIQ=%d,freq_off=%d,SyncFlag=%d,Quality=%d,agc_gain=%d,RSSI=%d\n", csStepIQ_param.lenIQ, rx_freq_offset, packetSyncFlag, packetQuality, cs_rx_agcGain, packetRSSI);
                        #endif
                        #if (DBG_CS_DATA_USB_PRINT_EN)
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "refl_mode_0_data,lenIQ=%d,freq_off=%d,SyncFlag=%d,Quality=%d,agc_gain=%d,RSSI=%d\n", csStepIQ_param.lenIQ, rx_freq_offset, packetSyncFlag, packetQuality, cs_rx_agcGain, packetRSSI);
                        #endif
                    }
                    else if(raw_pkt[2] & BLT_CS_MODE_1_FLAG){
                        tlkapi_send_string_data(DBG_CS_DATA_EN, "reflector_mode_1_mlp_data", raw_pkt, 16);

                        cs_step_IQ_param_t *stepIQ_param = (cs_step_IQ_param_t *)&csStepIQ_param;
                        blt_ll_cs_getStepIQParam(role, STEP_MODE_1, raw_pkt, stepIQ_param);

                        BYTE_TO_UINT32(tx_on_start_tstamp, &raw_pkt[DMA_CS_RFRX_OFFSET_TX_ON_TSTAMP_4BYTE(raw_pkt)]);
                        BYTE_TO_UINT32(cs_rx_accessAddr, &raw_pkt[DMA_CS_RFRX_OFFSET_RX_ACCESS_ADDRESS_4BYTE(raw_pkt)]);
                        BYTE_TO_UINT32(rx_pkt_iq_sync_tstamp, &raw_pkt[DMA_CS_RFRX_OFFSET_TIME_STAMP(raw_pkt)]);

                        BYTE_TO_UINT32(rx_iq_start_tstamp, &raw_pkt[DMA_CS_RFRX_OFFSET_IQ_START_TSTAMP(raw_pkt)]);//for test
                        u8 pktSyncBitNum = raw_pkt[DMA_CS_RFRX_OFFSET_PKT_MATCH_SYNC(raw_pkt)];//for test

                        DBG_CHN5_HIGH;//test hadm algorithm time start
                        if(packetSyncFlag){
                            if(tx_on_start_tstamp != rx_pkt_iq_sync_tstamp){
                                int cs_aaCode[CS_ACCESS_ADDRESS_BIT_SIZE];
                                for (u16 i = 0; i < CS_ACCESS_ADDRESS_BIT_SIZE; i++){
                                    cs_aaCode[i] = (cs_rx_accessAddr >> i) & 1;
                                }
                                //tlkapi_printf(DBG_CS_DATA_EN, "cs_rx_accessAddr,%s", hex_to_str(cs_aaCode, CS_ACCESS_ADDRESS_BIT_SIZE));

                                int dataRate = 1e6; // for 1M case now
                                /**
                                 *  First param: number of step
                                 *  Second param: role, INITIATOR---0,REFLECTOR---1
                                 */
                                parameterPesCollectDataSDK paraPesSDK = pesCollectDataInitSDK(1, 1, dataRate, cs_aaCode, CS_ACCESS_ADDRESS_BIT_SIZE, cs_internalDelay, cs_adThr, cs_adStep);
                                u32 t_sy_center_delta_init = 194 * 2 * 1000;//(44+5+145)*1e-6/0.5e-9
                                calcPesInfoSDK(&tx_on_start_tstamp, &rx_pkt_iq_sync_tstamp, t_sy_center_delta_init, &cte_reflector, paraPesSDK);

                                #if (0)
                                    blt_ll_cs_Convert20BitIQ2int(&raw_pkt[csStepIQ_param.startIQIdx], &initial_IQData[0], csStepIQ_param.lenSample);

                                    float rdm = 0.0;// Detector Metrics
                                    //currently consume 2.3ms under CCLK_96M
                                    packetNADM = calcPesNadm(initial_IQData, csStepIQ_param.lenSample, -cs_cfo, cs_nadm_adtype, &rdm, paraPesSDK);
                                #endif
                            }
                            else{
                                packetQuality = CS_STEP_RECEIVE_PACKET_QUALITY_LOW;
                            }
                        }
                        DBG_CHN5_LOW;//test hadm algorithm time post

                        pEvt->Step_Mode->mode = STEP_MODE_1;
                        pEvt->Step_Mode->channel = csChannel;
                        pEvt->Step_Mode->len = CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY;

                        u8 mode1Result[CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY];
                        cs_step_mode1_t *pMode1 = (cs_step_mode1_t *)mode1Result;

                        pMode1->Packet_Quality = packetQuality;
                        pMode1->Packet_NADM = packetNADM;
                        pMode1->Packet_RSSI = packetRSSI;

                        pMode1->ToA_ToD[0] = U16_LO(cte_reflector);
                        pMode1->ToA_ToD[1] = U16_HI(cte_reflector);
                        pMode1->Packet_Antenna = 1;
                        smemcpy(pEvt->Step_Mode->data, pMode1, CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY);
                        result_len += CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY;

                        //tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", csStepIQ_param.lenIQ, csStepIQ_param.lenSample, csStepIQ_param.startIQIdx, 0);
                        tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", cs_rx_accessAddr, rx_iq_start_tstamp / SYSTEM_TIMER_TICK_1US, rx_pkt_iq_sync_tstamp / SYSTEM_TIMER_TICK_1US, tx_on_start_tstamp / SYSTEM_TIMER_TICK_1US);
                        tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", csStepIQ_param.lenIQ, tx_on_start_tstamp - rx_pkt_iq_sync_tstamp, (rx_pkt_iq_sync_tstamp - rx_iq_start_tstamp) / SYSTEM_TIMER_TICK_1US, (tx_on_start_tstamp - rx_pkt_iq_sync_tstamp) / SYSTEM_TIMER_TICK_1US);
                        tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", packetSyncFlag, packetQuality, pktSyncBitNum, cte_reflector);
                        //tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", pMode1->Packet_Quality, pMode1->Packet_NADM, pMode1->Packet_RSSI, cte_reflector);

                        #if (DBG_CS_DATA_PRINT_EN)
                            printf("refl_mode_1_data,tx_on-sync=%d,ToD_ToA=%hd\n", tx_on_start_tstamp - rx_pkt_iq_sync_tstamp, cte_reflector);
                            printf("  lenIQ=%d,SyncFlag=%d,Quality=%d,NADM=%d,agc_gain=%d,RSSI=%d\n", csStepIQ_param.lenIQ, packetSyncFlag, packetQuality, packetNADM, cs_rx_agcGain, packetRSSI);
                        #endif
                        #if (DBG_CS_DATA_USB_PRINT_EN)
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "refl_mode_1_data,tx_on-sync=%d,ToD_ToA=%hd\n", tx_on_start_tstamp - rx_pkt_iq_sync_tstamp, cte_reflector);
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  lenIQ=%d,SyncFlag=%d,Quality=%d,NADM=%d,agc_gain=%d,RSSI=%d\n", csStepIQ_param.lenIQ, packetSyncFlag, packetQuality, packetNADM, cs_rx_agcGain, packetRSSI);
                        #endif
                    }
                    else if(raw_pkt[2] & BLT_CS_MODE_2_FLAG){
                        tlkapi_send_string_data(DBG_CS_DATA_EN, "reflector_mode_2_mlp_data", raw_pkt, 16);

                        cs_step_IQ_param_t *stepIQ_param = (cs_step_IQ_param_t *)&csStepIQ_param;
                        blt_ll_cs_getStepIQParam(role, STEP_MODE_2, raw_pkt, stepIQ_param);

                        BYTE_TO_UINT32(rx_iq_start_tstamp, &raw_pkt[DMA_CS_RFRX_OFFSET_IQ_START_TSTAMP(raw_pkt)]);
                        BYTE_TO_UINT32(tx_turnaround_time_pos, &raw_pkt[DMA_CS_RFRX_OFFSET_LAST_TX_POS_TSTAMP_4BYTE(raw_pkt)]);

                        DBG_CHN5_HIGH;//test hadm algorithm time start
                        #if (1)
                            u16 valid_lenSample;
                            u16 valid_startIQIdx;
                            valid_lenSample = csStepIQ_param.lenSample - ((CS_RX_1M_TONE_HALF_EXCLUSION_PERIOD_US * 2) << 2);
                            valid_startIQIdx = csStepIQ_param.startIQIdx + ((CS_RX_1M_TONE_HALF_EXCLUSION_PERIOD_US * (IQ_20_BIT_MODE >> 8)) << 2);
                            blt_ll_cs_Convert20BitIQ2int(&raw_pkt[valid_startIQIdx], &initial_IQData[0], valid_lenSample);
                            toneQuality_raw = calcTesInfo(&initial_IQData[0], cs_compArr, valid_lenSample, (u32)(rx_iq_start_tstamp - cs_procedure_start_tick), (u32)(tx_turnaround_time_pos - cs_procedure_start_tick), -cs_cfo, cs_if_adjustment79[csChannel], &(pct_reflector[0]), cs_thresGood, cs_thresBad);
                            toneQualityIndicator = blt_ll_cs_getToneQualityIndicator(toneQuality_raw);
                            compressTesInfo(pct_reflector, 2, 12);
                            if(csStepIQ_param.tone_ext){
                                blt_ll_cs_Convert20BitIQ2int(&raw_pkt[valid_startIQIdx + csStepIQ_param.lenIQ], &initial_IQData[0], valid_lenSample);
                                toneExtQuality_raw = calcTesInfo(&initial_IQData[0], cs_compArr, valid_lenSample, (u32)(rx_iq_start_tstamp - cs_procedure_start_tick), (u32)(tx_turnaround_time_pos - cs_procedure_start_tick), -cs_cfo, cs_if_adjustment79[csChannel], &(pct_ext_reflector[0]), cs_thresGood, cs_thresBad);
                                toneExtQualityIndicator = blt_ll_cs_getToneQualityIndicator(toneExtQuality_raw);
                                compressTesInfo(pct_ext_reflector, 2, 12);
                            }
                        #else
                            blt_ll_cs_Convert20BitIQ2int(&raw_pkt[csStepIQ_param.startIQIdx], &initial_IQData[0], csStepIQ_param.lenSample);
                            toneQuality_raw = calcTesInfo(&initial_IQData[0], cs_compArr, csStepIQ_param.lenSample, (u32)(rx_iq_start_tstamp - cs_procedure_start_tick), (u32)(tx_turnaround_time_pos - cs_procedure_start_tick), -cs_cfo, cs_if_adjustment79[csChannel], &(pct_reflector[0]), cs_thresGood, cs_thresBad);
                            toneQualityIndicator = blt_ll_cs_getToneQualityIndicator(toneQuality_raw);
                            compressTesInfo(pct_reflector, 2, 12);
                            if(csStepIQ_param.tone_ext){
                                blt_ll_cs_Convert20BitIQ2int(&raw_pkt[csStepIQ_param.startIQIdx + csStepIQ_param.lenIQ], &initial_IQData[0], csStepIQ_param.lenSample);
                                toneExtQuality_raw = calcTesInfo(&initial_IQData[0], cs_compArr, csStepIQ_param.lenSample, (u32)(rx_iq_start_tstamp - cs_procedure_start_tick), (u32)(tx_turnaround_time_pos - cs_procedure_start_tick), -cs_cfo, cs_if_adjustment79[csChannel], &(pct_ext_reflector[0]), cs_thresGood, cs_thresBad);
                                toneExtQualityIndicator = blt_ll_cs_getToneQualityIndicator(toneExtQuality_raw);
                                compressTesInfo(pct_ext_reflector, 2, 12);
                            }
                        #endif
                        DBG_CHN5_LOW;//test hadm algorithm time post
                        tlkapi_send_string_u32s(DBG_CS_DATA_EN, "  ", csStepIQ_param.lenSample, toneQualityIndicator, csStepIQ_param.tone_ext, toneExtQualityIndicator);

                        pEvt->Step_Mode->mode = STEP_MODE_2;
                        pEvt->Step_Mode->channel = csChannel;
                        pEvt->Step_Mode->len = CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1;

                        u8 mode2Result[CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1];
                        cs_step_mode2_t *pMode2 = (cs_step_mode2_t *)mode2Result;
                        pMode2->Antenna_Permutation_Index = 0;
                        pMode2->Tone[0].Tone_PCT[0] = U16_LO(pct_reflector[0]);
                        pMode2->Tone[0].Tone_PCT[1] = ((pct_reflector[0] & 0xF00) >> 8) | ((pct_reflector[1] & 0xF) << 4);
                        pMode2->Tone[0].Tone_PCT[2] = U16_LO((pct_reflector[1] >> 4));
                        pMode2->Tone[0].Tone_Quality_Indicator = toneQualityIndicator;
                        pMode2->Tone[1].Tone_PCT[0] = U16_LO(pct_ext_reflector[0]);
                        pMode2->Tone[1].Tone_PCT[1] = ((pct_ext_reflector[0] & 0xF00) >> 8) | ((pct_ext_reflector[1] & 0xF) << 4);
                        pMode2->Tone[1].Tone_PCT[2] = U16_LO((pct_ext_reflector[1] >> 4));
                        if(raw_pkt[2] & BLT_CS_STEP_TONE_EXTENSION_SLOT_FLAG){
                            pMode2->Tone[1].Tone_Quality_Indicator = toneExtQualityIndicator | BIT(5);
                        }
                        else{
                            pMode2->Tone[1].Tone_Quality_Indicator = toneExtQualityIndicator | BIT(4);
                        }
                        smemcpy(pEvt->Step_Mode->data, mode2Result, CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1);
                        result_len += CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1;

                        #if (DBG_CS_DATA_PRINT_EN)
                            printf("refl_mode_2_data,agc_gain=%d,toneQI=%d,pct_I=%hd,pct_Q=%hd,pct_I=0x%hx,pct_Q=0x%hx,toneQ=%f\n", cs_rx_agcGain, toneQualityIndicator, pct_reflector[0], pct_reflector[1], pct_reflector[0], pct_reflector[1], toneQuality_raw);
                            printf("  lenIQ=%d,lenSample=%d,startIQIdx=%d,valid_lenSample=%d,valid_startIQIdx=%d\n", csStepIQ_param.lenIQ, csStepIQ_param.lenSample, csStepIQ_param.startIQIdx, valid_lenSample, valid_startIQIdx);
                            printf("  channel=%d,iq_start=%d,tx_en=%d,cs_start=%d,tx_en-iq_start=%d\n", csChannel, rx_iq_start_tstamp/SYSTEM_TIMER_TICK_1US, tx_turnaround_time_pos/SYSTEM_TIMER_TICK_1US, cs_procedure_start_tick/SYSTEM_TIMER_TICK_1US, (tx_turnaround_time_pos-rx_iq_start_tstamp)/SYSTEM_TIMER_TICK_1US);
                            if(csStepIQ_param.tone_ext){
                                printf("  tone_ext=1,toneQI=%d,pct_I=%hd,pct_Q=%hd,pct_I=0x%hx,pct_Q=0x%hx,toneQ=%f\n", toneExtQualityIndicator, pct_ext_reflector[0], pct_ext_reflector[1], pct_ext_reflector[0], pct_ext_reflector[1], toneExtQuality_raw);
                            }
                        #endif
                        #if (DBG_CS_DATA_USB_PRINT_EN)
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "refl_mode_2_data,agc_gain=%d,toneQI=%d,pct_I=%hd,pct_Q=%hd,pct_I=0x%hx,pct_Q=0x%hx,toneQ=%f\n", cs_rx_agcGain, toneQualityIndicator, pct_reflector[0], pct_reflector[1], pct_reflector[0], pct_reflector[1], toneQuality_raw);
                            //tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  lenIQ=%d,lenSample=%d,startIQIdx=%d,valid_lenSample=%d,valid_startIQIdx=%d\n", csStepIQ_param.lenIQ, csStepIQ_param.lenSample, csStepIQ_param.startIQIdx, valid_lenSample, valid_startIQIdx);
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  channel=%d,iq_start=%d,tx_en=%d,cs_start=%d,tx_en-iq_start=%d\n", csChannel, rx_iq_start_tstamp/SYSTEM_TIMER_TICK_1US, tx_turnaround_time_pos/SYSTEM_TIMER_TICK_1US, cs_procedure_start_tick/SYSTEM_TIMER_TICK_1US, (tx_turnaround_time_pos-rx_iq_start_tstamp)/SYSTEM_TIMER_TICK_1US);
                            if(csStepIQ_param.tone_ext){
                                tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "  tone_ext=1,toneQI=%d,pct_I=%hd,pct_Q=%hd,pct_I=0x%hx,pct_Q=0x%hx,toneQ=%f\n", toneExtQualityIndicator, pct_ext_reflector[0], pct_ext_reflector[1], pct_ext_reflector[0], pct_ext_reflector[1], toneExtQuality_raw);
                            }
                        #endif
                    }
                    else if(raw_pkt[2] & BLT_CS_MODE_3_FLAG){
                        tlkapi_send_string_data(DBG_CS_DATA_EN, "reflector_mode_3_mlp_data", raw_pkt, 16);
                        #if (DBG_CS_DATA_PRINT_EN)
                            printf("refl_mode_3_data\n");
                        #endif
                        #if (DBG_CS_DATA_USB_PRINT_EN)
                            tlkapi_printf(DBG_CS_DATA_USB_PRINT_EN, "refl_mode_3_data\n");
                        #endif
                    }
                }

                //raw_pkt[2] = 0;

                if(hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_CS_SUBEVENT_RESULT)
                {
                    hci_le_csSubeventResult_evt(0, 0, &pEvt->Subevent_Code, result_len);
                }
            }

            cs_rx_fifo_test.rptr == (cs_rx_fifo_test.num - 1) ? cs_rx_fifo_test.rptr = 0 : cs_rx_fifo_test.rptr++;
        }
    }
}
#endif
