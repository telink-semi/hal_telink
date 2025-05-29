/********************************************************************************************************
 * @file    cs_ctrl_pdu.c
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

    #define ACI_TO_MAX_PATH(n)  ((n < 4) ? (n + 1) : ((n < 7) ? (n - 2) : (n - 5)))
    #define COUNT_BIT_1(n)      (((n) & 1) + (((n) >> 1) & 1) + (((n) >> 2) & 1) + (((n) >> 3) & 1))

    #define CS_SCH_SET_EARLY_US (2000)  // Rx early time before Rx/Tx timestamp of cs subevent
    #define CS_SUBEVT_OFFSET    (10000) // time offset before acl anchor point


antenna_control_func_t ant_ctrl_cfg;

ble_sts_t blt_ll_calcStepDuration(cs_config_t *pCsCfg);


//need to make sure not used in irq.
const u8 T_IP_US[8]         = {10, 20, 30, 40, 50, 60, 80, 145};           //unit us
const u8 T_FCS_US[10]       = {15, 20, 30, 40, 50, 60, 80, 100, 120, 150}; //unit us
const u8 ACI_to_N_AP[8]     = {1, 2, 3, 4, 2, 3, 4, 4};
const u8 RTT_Type_SeqNum[7] = {0, 4, 12, 4, 8, 12, 16};                    //byte number, //bit number{0,  32, 96, 32, 64, 96, 128}
const u8 T_PM_US[3]         = {10, 20, 40};                                //unit us


chn_sound_capabilities_t bltCsLocalSupportCap = {

    .Num_Config_Supported                 = 1, //range 1-4
    .max_consecutive_procedures_supported = 0,
    .Num_Antennas_Supported               = NUM_ANT_SUPPORT,
    .Max_Antenna_Paths_Supported          = MAX_ANT_PATHS_SUPPORT,

    .Roles_Supported                   = CS_ROLE_DISABLE,
    .Mode_Types                        = 0,   //mandatory mode1 and mode 2
    .RTT_Capability                    = 0,   //150ns
    .RTT_AA_Only_N                     = 240, //todo
    .RTT_Sounding_N                    = 240,
    .RTT_Random_Payload_N              = 240,
    .Optional_NADM_Sounding_Capability = 0,
    .Optional_NADM_Random_Capability   = 0,
    .Optional_CS_SYNC_PHYs_Supported   = 0,        //just mandatory 1M PHY
    .Optional_Subfeatures_Supported    = 0,
    .Optional_T_IP1_Times_Supported    = 0,        //CS_T_TP_145US is mandatory so shall not be set in optional capability
    .Optional_T_IP2_Times_Supported    = 0,        //CS_T_TP_145US,//145us
    .Optional_T_FCS_Times_Supported    = 0,        //CS_T_FCS_150US, //150us

    .Optional_T_PM_Times_Supported = CS_T_PM_20US, //CS_T_PM_40US, //40us M

    #if (HCI_CCO_BI_79_C)
    .T_SW_Time_Supported = 0,                      //0us
    #else
    .T_SW_Time_Supported = 10, //10us
    #endif
    .Optional_TX_SNR_Capability = 0xff,            // SNR = 18dB
};

u8 csChnNum = 0;

void blt_cs_chnMapAndOperate(u8 *chn_out, u8 *chnM, u8 *originChnMap)
{
    for (u32 i = 0; i < 10; i++) {
        chn_out[i] = originChnMap[i] & chnM[i];
    }
}

void blt_ll_cs_buffCombize(u8 *input_l, u8 *input_h, u8 *output, u8 len)
{
    for (int i = 0; i < (len); i++) {
        output[i]       = input_l[i];
        output[i + len] = input_h[i];
    }
}

u8 blt_ll_getNewCsConfig(void)
{
    for (u32 i = 0; i < gCsMng.max_num_cofig; i++) {
        cs_config_t *pCfg = gCsMng.gGlobal_pCsCfg + i;

        if (pCfg->occupy == 0) {
            //          smemset(pCfg,0,sizeof(cs_config_t));//todo
            return i;
        }
    }
    return 0xff;
}

u8 blt_ll_getCsConfigById(u16 connHandle, u8 config_id)
{
    for (u32 i = 0; i < gCsMng.max_num_cofig; i++) {
        cs_config_t *pCfg = gCsMng.gGlobal_pCsCfg + i;
        if ((pCfg->occupy) && (pCfg->Config_ID == config_id) && (connHandle == pCfg->aclHandle)) {
            return i;
        }
    }

    return 0xff;
}

u8 blt_ll_getCsConfigByRole(u16 connHandle, cs_config_role_t role)
{
    for (u32 i = 0; i < gCsMng.max_num_cofig; i++) {
        cs_config_t *pCfg = gCsMng.gGlobal_pCsCfg + i;
        if ((pCfg->occupy) && (pCfg->Role == role) && (connHandle == pCfg->aclHandle)) {
            return 1;
        }
    }

    return 0;
}

u8 blt_ll_getCsConfigByConnHandle(u16 connHandle)
{
    for (int i = 0; i < gCsMng.max_num_cofig; i++) {
        cs_config_t *pCfg = gCsMng.gGlobal_pCsCfg + i;
        if ((pCfg->occupy) && (connHandle == pCfg->aclHandle)) {
            return i;
        }
    }

    return 0xff;
}

u8 blt_ll_checkCsAci(u8 local_aci, u8 remote_aci)
{
    u8 ret = 0xff;
    if ((remote_aci == 0) || (remote_aci == local_aci)) {
        return remote_aci;
    } else if (remote_aci > local_aci) {
        return ret;
    } else {
        if (local_aci == 7) {
            if (remote_aci < 2) {
                return remote_aci;
            }
        } else if (local_aci > 3) {
            if (remote_aci > 3) {
                return remote_aci;
            }
        } else if (local_aci > 0) {
            if (remote_aci > 0) {
                return remote_aci;
            }
        }
    }

    return ret;
}



    #if CS_SUBEVENT_LEN_EVALUATE
u32 blt_ll_cs_subevent_schedule_early_cal(cs_config_t *pCsCfg, u32 maxSubLen, u32 *pSubLen, u32 schedule_early_max)
{
        #define SUBEVENT_START_MARGIN 600 //96Mhz
    u8 first_cal_mode0_chn     = 1;
    u8 first_cal_non_mode0_chn = 1;
    u8 cur_step_mode           = 0;
    u8 next_step_mode          = 0;
    u8 chn_lll                 = 0;
    u8 main_mode_repeat_cnt    = 0; // needn't consider main mode repeat when evaluate schedule early time.

    u8                 chnMRepeCnt                 = 0;
    u8                 step_cnt                    = 0;
    u8                 submode_insertion           = 0;
    u16                mode0_chnReadIdx            = 0;
    u16                nonMode0_chnReadIdx         = 0;
    u32                noneMode0ShuffledChannelNum = 0;
    u32                subevent_len                = 0;
    u8                 mode0ShuffledChnArray[72];
    u8                 nonmode0ShuffledChnArray[404];
    slip_window_step_t slip_window_step;

    u8 break_flag = 0;


    u32 clock_tick = clock_time();

    drbg->stepCnt = 0;

    for (int j = 0; subevent_len < maxSubLen; j++) {
        u8 seqbit_len             = 0;
        u8 sounding_sequence_task = 0;
        u8 random_sequence_task   = 0;
        u8 access_code_task       = 0;
        u8 tone_task              = 0;
        u8 non_mode0_chn_flush    = 0;

        slip_window_step_t *pslip_window_step = &slip_window_step;

        //step 1: calculate current mode
        //mode0
        if (step_cnt < pCsCfg->Mode_0_Steps) {
            cur_step_mode = STEP_MODE_0;
        }
        //main mode + sub mode
        else {
            cur_step_mode = pCsCfg->Main_Mode; //include mainmode repeat
            if (main_mode_repeat_cnt == 0) {   //no main mode need repeat
                if (pCsCfg->Sub_Mode != SUBMODE_TYPE_MODE_UNUSED) {
                    //cal sub mode insertion
                    if (!submode_insertion) {
                        submode_insertion = cs_sub_mode_insertion(pCsCfg->Main_Mode_Max_Steps, pCsCfg->Main_Mode_Min_Steps) + 1;
                    }
                    if (submode_insertion == 1) { //sub mode
                        cur_step_mode = pCsCfg->Sub_Mode;
                    }
                    submode_insertion--;
                }
            }
        }

        //step 2: calculate next step mode
        if ((step_cnt + 1) < pCsCfg->Mode_0_Steps) {
            next_step_mode = STEP_MODE_0;
        } else {
            if (submode_insertion == 1) {
                next_step_mode = pCsCfg->Sub_Mode;
            } else {
                next_step_mode = pCsCfg->Main_Mode;
            }
        }

        //step 3: create drbg task
        if (cur_step_mode == STEP_MODE_0) {
            ////mode0 chn function start////
            if (first_cal_mode0_chn || (mode0_chnReadIdx >= pCsCfg->Chn_en_num)) {
                first_cal_mode0_chn = 0;
                chn_sel_3a(pCsCfg->Chn_en_num, pCsCfg->filteredChnArray, mode0ShuffledChnArray);

                mode0_chnReadIdx = 0; //restart
            }

            u8 tChnIdx                     = mode0_chnReadIdx % pCsCfg->Chn_en_num;
            pslip_window_step->step_chnIdx = mode0ShuffledChnArray[tChnIdx];
            mode0_chnReadIdx++;
            ////mode0 chn function end////
            //access code
            access_code_task = 1;
        } else {                        //non mode0
            if (main_mode_repeat_cnt) { // repeat main mode
                main_mode_repeat_cnt--;
                //needn't consider
            } else {
                if (cur_step_mode == pCsCfg->Main_Mode) {
                    ////non mode0 chn function start////
                    if (first_cal_non_mode0_chn || ((pCsCfg->ChSel == CSA_3B) && (nonMode0_chnReadIdx >= pCsCfg->Chn_en_num))) {
                        first_cal_non_mode0_chn = 0;
                        if (pCsCfg->ChSel) { // channel select #3c
                            drbg->stepCnt -= pCsCfg->Mode_0_Steps;
                            noneMode0ShuffledChannelNum = chn_sel_3c_cb(pCsCfg->Channel_Map, pCsCfg->Ch3c_Shape, pCsCfg->Ch3c_Jump, pCsCfg->Channel_Map_Repetition, nonmode0ShuffledChnArray);
                            drbg->stepCnt += pCsCfg->Mode_0_Steps;

                        } else { // channel select #3b
                            chn_sel_3b(pCsCfg->Chn_en_num, pCsCfg->filteredChnArray, nonmode0ShuffledChnArray);
                        }
                        nonMode0_chnReadIdx = 0; //restart
                    }
                    non_mode0_chn_flush = 1;
                    u8 tChnIdx          = 0;
                    if (pCsCfg->ChSel == CSA_3C) {
                        tChnIdx = nonMode0_chnReadIdx;
                    } else {
                        tChnIdx = nonMode0_chnReadIdx % pCsCfg->Chn_en_num;
                    }
                    pslip_window_step->step_chnIdx = nonmode0ShuffledChnArray[tChnIdx];
                    nonMode0_chnReadIdx++;
                    ////non mode0 chn function end////

                    chn_lll = pslip_window_step->step_chnIdx;
                } else {                                      //sub mode
                    pslip_window_step->step_chnIdx = chn_lll; //gCsMng.blt_pCsCfg->mainmode_repeat_chn[(gCsMng.blt_pCsCfg->mainmode_repeat_wptr + 4 - 1)&0x03];
                }
            }


            if ((cur_step_mode == STEP_MODE_1) || (cur_step_mode == STEP_MODE_3)) {
                //access code
                access_code_task = 1;
                //sounding sequence marker position random & signal select
                if (pCsCfg->RTT_Type == RTT_Type_32bit_ss) {
                    sounding_sequence_task = 1;
                    seqbit_len             = 32;
                } else if (pCsCfg->RTT_Type == RTT_Type_96bit_ss) {
                    sounding_sequence_task = 1;
                    seqbit_len             = 96;
                }
                //random sequence generation
                else if (pCsCfg->RTT_Type == RTT_Type_32bit_rs || pCsCfg->RTT_Type == RTT_Type_64bit_rs ||
                         pCsCfg->RTT_Type == RTT_Type_96bit_rs || pCsCfg->RTT_Type == RTT_Type_128bit_rs) {
                    random_sequence_task = 1;
                    seqbit_len           = 32 * (pCsCfg->RTT_Type - 2);
                }
            }

            if ((cur_step_mode == STEP_MODE_2) || (cur_step_mode == STEP_MODE_3)) {
                tone_task = 1;
            }
        }

        if (access_code_task) {
            //access code
            cs_access_addr((u8 *)&pslip_window_step->step_reflAA, (u8 *)&pslip_window_step->step_initAA);
            tlkapi_send_string_u32s(0, "m0AA", pslip_window_step->step_reflAA, pslip_window_step->step_initAA, step_cnt, pslip_window_step->step_chnIdx);
        }

        if (tone_task) {
            //Antenna path permutation index selection
            if (pCsCfg->aci != 0) { //only when exist multiple antenna path then calculate the API.
                pslip_window_step->step_antPathPermIdx = cs_antenna_path_perm(pCsCfg->antennaPathNum);
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

        //step 4: prepare next step
        drbg->stepCnt++;
        step_cnt++;
        pslip_window_step->step_modeType   = cur_step_mode;
        pslip_window_step->subeventEndFlag = 0;
        pslip_window_step->proceStopFlag   = 0;

        //step 5: check subevent whether is done.
        u32 next_step_len = 0;

        u16 *pDurUs = (u16 *)&pCsCfg->mode0Step_durUs; //mode0Step_durUs ~ mode3Step_durUs address are continuously
        subevent_len += pDurUs[cur_step_mode & 0x03];
        next_step_len += pDurUs[next_step_mode & 0x03];

        if ((non_mode0_chn_flush == 1) && (nonMode0_chnReadIdx >= pCsCfg->Chn_en_num)) { //need check mode0 chnmap repeat
            chnMRepeCnt++;
        }


        //subevent done
        if (((subevent_len + next_step_len) > maxSubLen) || (step_cnt == 160)) {
            pslip_window_step->subeventEndFlag = 1;
        }
        //procedure done
        if ((pCsCfg->ChSel == CSA_3B) && (chnMRepeCnt >= pCsCfg->Channel_Map_Repetition)) {
            pslip_window_step->subeventEndFlag = 1;
        }
        if ((pCsCfg->ChSel == CSA_3C) && noneMode0ShuffledChannelNum && (nonMode0_chnReadIdx >= noneMode0ShuffledChannelNum)) {
            pslip_window_step->subeventEndFlag = 1;
        }
        if (drbg->stepCnt >= 256) {
            pslip_window_step->subeventEndFlag = 1;
        }


        if (pslip_window_step->subeventEndFlag) {
            break;
        }
        u32 t_us = ((u32)(clock_time() - clock_tick)) / SYSTEM_TIMER_TICK_1US;
        //200us is evaluated for next step.
        if ((t_us + SUBEVENT_START_MARGIN + 200) > schedule_early_max) {
            break_flag = 1;
            break;
        }
    }
    pSubLen[0] = break_flag ? subevent_len : maxSubLen;
    clock_tick = ((u32)(clock_time() - clock_tick)) / SYSTEM_TIMER_TICK_1US;
    clock_tick = SUBEVENT_START_MARGIN + clock_tick * 2;
    cs_drbg_init();
    return clock_tick;
}

void blt_ll_cs_subevent_len_cal(st_ll_conn_t *pAclConn, u8 config_id)
{
    u8 cfgIdx = blt_ll_getCsConfigById(pAclConn->acl_conHandle, config_id);

    cs_config_t *pCsCfg = gCsMng.gGlobal_pCsCfg + cfgIdx;

    u32 subevent_len_t = 0;

    u32 max_offset = min((pAclConn->conn_intvl_n_1m25 * 1250 - 1), 4000000);

    u16 max_sch_early_us = blt_ll_cs_subevent_schedule_early_cal(pCsCfg, pCsCfg->Max_Subevent_Len, &subevent_len_t, max_offset);

    tlkapi_send_string_u32s(0, "cs subevent len cal", pCsCfg->Max_Subevent_Len, subevent_len_t, max_sch_early_us, pAclConn->conn_intvl_n_1m25 * 1250);

    u32 max_subevent_len = min(subevent_len_t, pCsCfg->Max_Subevent_Len);

    max_sch_early_us = max(max_sch_early_us, TLK_T_MES);

    pCsCfg->sch_early_us = max_sch_early_us;

    u32 max_subevent_interval = (max_subevent_len + max_sch_early_us + CS_SUBEVNET_RESULT_REPORT_DURATION_US + (625 - 1)) / 625;

    pCsCfg->Subevent_Len = max_subevent_len;

    u16 min_cs_offset_us;
    #if (LL_CS_SNIFFER_MODE_ENABLE)
        /*
         * timing(uS)
         * first packet(rf_len=0) + T_IFS + second packet(rf_len=255) + margin + cs_sch_early_us
         *          80              150             2120                  400           2100        = 4850
         */
        min_cs_offset_us = 4850;
    #else
        min_cs_offset_us = 500;
    #endif

    pCsCfg->offset_min = max(max_sch_early_us + pAclConn->sSlot_duration * SSLOT_US_NUM, min_cs_offset_us);

    pCsCfg->offset_max = max((pAclConn->conn_intvl_n_1m25 * 1250 - 1), pCsCfg->offset_min);

    u32 max_procedure_len = min(pCsCfg->Max_Procedure_Len * 625, (pCsCfg->Max_Procedure_Interval * (pAclConn->conn_intvl_n_1m25 * 1250) - pCsCfg->offset_min));

    u8 subevent_per_event = ((max_procedure_len + max_subevent_interval * 625 - max_subevent_len + (max_subevent_interval * 625 - 1)) / (max_subevent_interval * 625));

    pCsCfg->Subevents_Per_Event = min(subevent_per_event, 32);

    if (pCsCfg->Subevents_Per_Event <= 1) {
        pCsCfg->Subevents_Per_Event = 1;
        pCsCfg->subEvtIntvl_625us   = 0;
    } else {
        pCsCfg->subEvtIntvl_625us = max_subevent_interval;
    }

    u32 procedure_interval = max(((CS_HADM_DURATION_US + pCsCfg->Max_Procedure_Len * 625 + pCsCfg->offset_max + (pAclConn->conn_intvl_n_1m25 * 1250 - 1)) / (pAclConn->conn_intvl_n_1m25 * 1250)), 1);


    pCsCfg->Procedure_Interval = max(procedure_interval, pCsCfg->Min_Procedure_Interval);
    if (pCsCfg->Max_Procedure_Interval < pCsCfg->Procedure_Interval) {
        tlkapi_send_string_u32s(0, "max procedure interval not enough", pCsCfg->Max_Procedure_Interval, procedure_interval);
    }
    pCsCfg->Procedure_Interval = min(pCsCfg->Procedure_Interval, pCsCfg->Max_Procedure_Interval);

    pCsCfg->Event_Interval = pCsCfg->Procedure_Interval ? pCsCfg->Procedure_Interval : 1;

    if (pCsCfg->procMaxCountInstant == 1) {
        pCsCfg->Procedure_Interval = 0;
    }

    //Event_Interval: (1--65535)

    tlkapi_send_string_u32s(0, "cs subevent len result", pCsCfg->offset_min, pCsCfg->Procedure_Interval, pCsCfg->Subevent_Len, pCsCfg->sch_early_us);
}
    #endif

void blt_ll_cs_exchangeCapProc(st_ll_conn_t *pAclConn)
{
    cs_param_t *pCsParam = &pAclConn->csParam;
    if (pCsParam->cs_cap_req == PROC_CS_CAP_EVT_PENDING) {
        pCsParam->cs_cap_exchange = 1;
        pCsParam->cs_cap_req      = 0;
        hci_le_readRemoteSupCapComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8 *)&pAclConn->csRemoteSupCap);
    } else //send req/rsp
    {
        chn_sound_capabilities_t  *local                                   = &bltCsLocalSupportCap;
        u8                         buff[sizeof(rf_packet_ll_cs_cap_req_t)] = {0};
        rf_packet_ll_cs_cap_req_t *cap_req                                 = (rf_packet_ll_cs_cap_req_t *)buff;

        cap_req->type                            = LLID_CONTROL;
        cap_req->rf_len                          = sizeof(rf_packet_ll_cs_cap_req_t) - 2;
        cap_req->Mode_Types                      = local->Mode_Types;
        cap_req->RTT_Capability                  = local->RTT_Capability;
        cap_req->RTT_AA_Only_N                   = local->RTT_AA_Only_N;
        cap_req->RTT_Sounding_N                  = local->RTT_Sounding_N;
        cap_req->RTT_Random_Sequence_N           = local->RTT_Random_Payload_N;
        cap_req->NADM_Sounding_Capability        = local->Optional_NADM_Sounding_Capability;
        cap_req->NADM_Random_Sequence_Capability = local->Optional_NADM_Random_Capability;
        cap_req->CS_SYNC_PHY_Capability          = local->Optional_CS_SYNC_PHYs_Supported;
        cap_req->Num_Ant                         = local->Num_Antennas_Supported;
        cap_req->Max_Ant_Path                    = local->Max_Antenna_Paths_Supported;
        cap_req->Role                            = local->Roles_Supported;
        cap_req->Companion_Signal                = (local->Optional_Subfeatures_Supported & CS_COMPANION_SIGNAL_SUPPORT) ? 1 : 0;
        cap_req->No_FAE                          = (local->Optional_Subfeatures_Supported & CS_No_FAE_SUPPORT) ? 1 : 0;
        cap_req->chn_sel_3c                      = (local->Optional_Subfeatures_Supported & CS_CSA_3C_SUPPORT) ? 1 : 0;
        cap_req->Sounding_PCT_Estimate           = (local->Optional_Subfeatures_Supported & CS_SOUNDING_PCT_ESTIMATE_SUPPORT) ? 1 : 0;
        cap_req->Num_Configs                     = local->Num_Config_Supported;
        cap_req->Max_Procedures_Supported        = local->max_consecutive_procedures_supported;
        cap_req->T_SW                            = local->T_SW_Time_Supported;
        cap_req->T_IP1_Capability                = local->Optional_T_IP1_Times_Supported;
        cap_req->T_IP2_Capability                = local->Optional_T_IP2_Times_Supported;
        cap_req->T_FCS_Capability                = local->Optional_T_FCS_Times_Supported;
        cap_req->T_PM_Capability                 = local->Optional_T_PM_Times_Supported;
        cap_req->SNR                             = local->Optional_TX_SNR_Capability;


        if (pCsParam->cs_cap_req & PROC_CS_CAP_SEND_RSP) {
            cap_req->opcode = LL_CS_CAPABILITIES_RSP;
            blt_ll_debug_print_capabilities(local, "[SEND_CAP_RSP]");
        } else if (pCsParam->cs_cap_req & PROC_CS_CAP_SEND_REQ) { //EXCHANGE_SEND_REQ
            //tlkapi_send_string_data(DBG_CS_LL_LOG_MASK_EN,"[CS][LL][SEND_CAP_REQ]",(u8*)buff,sizeof(rf_packet_ll_cs_cap_req_t));
            cap_req->opcode = LL_CS_CAPABILITIES_REQ;
            blt_ll_debug_print_capabilities(local, "[SEND_CAP_REQ]");
        }

        if (ll_push_tx_fifo_handler(pAclConn->acl_conHandle | HANDLE_STK_FLAG, (u8 *)cap_req)) {
            if (pCsParam->cs_cap_req & PROC_CS_CAP_SEND_RSP) {
                pCsParam->cs_cap_req          = 0;
                pCsParam->cs_cap_exchange     = 1;
                pAclConn->ll_rsp_timeout_tick = 0;
                hci_le_readRemoteSupCapComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8 *)&pAclConn->csRemoteSupCap);
            } else {
                pCsParam->cs_cap_req &= ~PROC_CS_CAP_SEND_REQ;
                pCsParam->cs_cap_req |= PROC_CS_CAP_WAIT_RSP;
                pAclConn->ll_rsp_timeout_tick = clock_time() | 1;
            }
        }
    }
}

void blt_ll_cs_exchangeSecurityStartProc(st_ll_conn_t *pAclConn)
{
    cs_param_t *pCsParam = &pAclConn->csParam;
    if (pCsParam->cs_security_enable == PROC_CS_SEC_EVT_PENDING) {
        pCsParam->cs_security_exchange = 1;
        pCsParam->cs_security_enable   = 0;
        hci_le_csSecurityEnableComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle);
    } else //send req/rsp
    {
        u8 buff[sizeof(rf_pkt_ll_cs_sec_req_t)];

        rf_pkt_ll_cs_sec_req_t *sec_req = (rf_pkt_ll_cs_sec_req_t *)buff;
        sec_req->type                   = LLID_CONTROL;
        if (pCsParam->cs_security_enable & PROC_CS_SEC_SEND_RSP) {
            rf_pkt_ll_cs_sec_rsp_t *sec_rsp = (rf_pkt_ll_cs_sec_rsp_t *)buff;
            sec_rsp->opcode                 = LL_CS_SEC_RSP;
            sec_rsp->rf_len                 = sizeof(rf_pkt_ll_cs_sec_rsp_t) - 2;

    #if 1 //temporary use default value. later delete. qiuwei
            generateRandomNum(8, &pCsParam->CS_IV_P[0]);
            generateRandomNum(4, &pCsParam->CS_IN_P[0]);
            generateRandomNum(8, &pCsParam->CS_PV_P[0]);
    #else
            u8 tCS_IV_P[8] = {0xe9, 0xdf, 0xfd, 0x0b, 0x8a, 0xc2, 0x0b, 0xe1};
            u8 tCS_IN_P[4] = {0xc1, 0x77, 0xf4, 0x9f};
            u8 tCS_PV_P[8] = {0x44, 0xed, 0x82, 0x98, 0xdf, 0xde, 0x80, 0xc9};

            smemcpy(pCsParam->CS_IV_P, tCS_IV_P, 8);
            smemcpy(pCsParam->CS_IN_P, tCS_IN_P, 4);
            smemcpy(pCsParam->CS_PV_P, tCS_PV_P, 8);
    #endif
            smemcpy(&sec_rsp->CS_IV_P[0], &pCsParam->CS_IV_P[0], 8);
            smemcpy(&sec_rsp->CS_IN_P[0], &pCsParam->CS_IN_P[0], 4);
            smemcpy(&sec_rsp->CS_PV_P[0], &pCsParam->CS_PV_P[0], 8);

            blt_ll_cs_buffCombize(&pCsParam->CS_IV_C[0], &pCsParam->CS_IV_P[0], &pCsParam->CS_IV[0], 8);
            blt_ll_cs_buffCombize(&pCsParam->CS_IN_C[0], &pCsParam->CS_IN_P[0], &pCsParam->CS_IN[0], 4);
            blt_ll_cs_buffCombize(&pCsParam->CS_PV_C[0], &pCsParam->CS_PV_P[0], &pCsParam->CS_PV[0], 8);

            drbg = (drbg_param_t *)&pCsParam->drbg_data[0];
            drbg_instantiation_func_h9(pCsParam->CS_IV, pCsParam->CS_IN, pCsParam->CS_PV, &drbg->kdrbg[0], &drbg->vdrbg[0]); ///////////
            cs_drbg_init();
            //CS_LL_LOG("[SEND_SEC_RSP]:%s",hex_to_str(buff,sizeof(rf_pkt_ll_cs_sec_rsp_t)));
            blt_ll_debug_print_security(buff, "[SEND_SEC_RSP]");
        } else if (pCsParam->cs_security_enable & PROC_CS_SEC_SEND_REQ) { //EXCHANGE_SEND_REQ

            sec_req->opcode = LL_CS_SEC_REQ;
            sec_req->rf_len = sizeof(rf_pkt_ll_cs_sec_req_t) - 2;
    #if 1 //temporary use default value. later delete. qiuwei
            generateRandomNum(8, &pCsParam->CS_IV_C[0]);
            generateRandomNum(4, &pCsParam->CS_IN_C[0]);
            generateRandomNum(8, &pCsParam->CS_PV_C[0]);
    #else
            u8 tCS_IV_C[8] = {0x3b, 0x0b, 0xca, 0xe0, 0x86, 0x51, 0x7f, 0x3e};
            u8 tCS_IN_C[4] = {0x0d, 0x84, 0x73, 0x86};
            u8 tCS_PV_C[8] = {0x43, 0xf1, 0x68, 0x78, 0x96, 0x74, 0xa6, 0x64};
            smemcpy(pCsParam->CS_IV_C, tCS_IV_C, 8);
            smemcpy(pCsParam->CS_IN_C, tCS_IN_C, 4);
            smemcpy(pCsParam->CS_PV_C, tCS_PV_C, 8);
    #endif
            smemcpy(&sec_req->CS_IV_C[0], &pCsParam->CS_IV_C[0], 8);
            smemcpy(&sec_req->CS_IN_C[0], &pCsParam->CS_IN_C[0], 4);
            smemcpy(&sec_req->CS_PV_C[0], &pCsParam->CS_PV_C[0], 8);
            //CS_LL_LOG("[SEND_SEC_REQ]:%s",hex_to_str(buff,sizeof(rf_pkt_ll_cs_sec_req_t)));
            blt_ll_debug_print_security(buff, "[SEND_SEC_REQ]");
        }

        if (ll_push_tx_fifo_handler(pAclConn->acl_conHandle | HANDLE_STK_FLAG, (u8 *)sec_req)) {
            if (pCsParam->cs_security_enable & PROC_CS_SEC_SEND_RSP) {
                pCsParam->cs_security_enable = 0;
    #if (LL_CS_CEN_REF_BV_01_C)
                pCsParam->cs_security_exchange = 0;
    #else
                pCsParam->cs_security_exchange = 1;
    #endif
                pAclConn->ll_rsp_timeout_tick = 0;
                hci_le_csSecurityEnableComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle);
            } else {
                pCsParam->cs_security_enable &= ~PROC_CS_SEC_SEND_REQ;
                pCsParam->cs_security_enable |= PROC_CS_SEC_WAIT_RSP;
                pAclConn->ll_rsp_timeout_tick = clock_time() | 1;
            }
        }
    }
}

void blt_ll_cs_exchangeFaeTableProc(st_ll_conn_t *pAclConn)
{
    cs_param_t *pCsParam = &pAclConn->csParam;
    if (pCsParam->cs_fae_req == PROC_CS_FAE_EVT_PENDING) {
        pCsParam->cs_fae_req      = 0;
        pCsParam->cs_fae_exchange = 1;
        u8 reason                 = BLE_SUCCESS;
        if (cs_fae_cmplt_reason) {
            reason              = cs_fae_cmplt_reason;
            cs_fae_cmplt_reason = 0;
        }
        hci_le_readRemoteFAETableComplete_evt(reason, pAclConn->acl_conHandle, (u8 *)&pCsParam->fae_table[0]);
    } else //send req/rsp
    {
        u8                      buff[sizeof(rf_pkt_ll_cs_fae_rsp_t)] = {0};
        rf_pkt_ll_cs_fae_rsp_t *pfae_table                           = (rf_pkt_ll_cs_fae_rsp_t *)buff;
        //from 2 to 22 and 26 to 76
        s8 local_fae_table[72] = {-1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0};
        pfae_table->type       = LLID_CONTROL;
        if (pCsParam->cs_fae_req & PROC_CS_FAE_SEND_REQ) {
            pfae_table->opcode = LL_CS_FAE_REQ;
            pfae_table->rf_len = 1;
            CS_LL_LOG("[SEND_FAE_REQ]");
        } else if (pCsParam->cs_fae_req & PROC_CS_FAE_SEND_RSP) {
            pfae_table->opcode = LL_CS_FAE_RSP;
            pfae_table->rf_len = (u8)(sizeof(rf_pkt_ll_cs_fae_rsp_t) - 2);
            smemcpy(pfae_table->fae_table, local_fae_table, 72);
            tlkapi_send_string_data(DBG_CS_LL_LOG_MASK_EN, "[CS][LL][SEND_FAE_RSP]", (u8 *)buff, sizeof(rf_pkt_ll_cs_fae_rsp_t));
        }

        if (ll_push_tx_fifo_handler(pAclConn->acl_conHandle | HANDLE_STK_FLAG, (u8 *)pfae_table)) {
            if (pCsParam->cs_fae_req & PROC_CS_FAE_SEND_RSP) {
                pCsParam->cs_fae_req          = 0;
                pCsParam->cs_fae_exchange     = 1;
                pAclConn->ll_rsp_timeout_tick = 0;
                hci_le_readRemoteFAETableComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8 *)&pfae_table->fae_table[0]);
            } else {
                pCsParam->cs_fae_req &= ~PROC_CS_FAE_SEND_REQ;
                pCsParam->cs_fae_req |= PROC_CS_FAE_WAIT_RSP;
                pAclConn->ll_rsp_timeout_tick = clock_time() | 1;
            }
        }
    }
}

ble_sts_t blt_ll_cs_exchangeConfigReq(st_ll_conn_t *pAclConn, cs_config_t *pCsCfg)
{
    cs_param_t *pCsParam = &pAclConn->csParam;

    if (pCsParam->cs_config_req == PROC_CS_CONFIG_EVT_PENDING) {
        pCsParam->cs_config_req = 0;
    #if LL_CS_CEN_INI_BI_04_C
        if (csFlowCtrl.csConfigExchErr) {
            if (csFlowCtrl.configCollision) {
                CS_HCI_LOG("report config complete evt file lines:%s ,%d", __FILENAME__, __LINE__);
                csFlowCtrl.configCollision = 0;
                hci_le_csConfigComplete_evt(HCI_ERR_LMP_ERR_TRANSACTION_COLLISION, pAclConn->acl_conHandle, (u8 *)pCsCfg);
            } else {
                CS_HCI_LOG("report config complete evt file lines:%s ,%d", __FILENAME__, __LINE__);
                hci_le_csConfigComplete_evt(HCI_ERR_UNSUPPORTED_LMP_PARAM_VAL, pAclConn->acl_conHandle, (u8 *)pCsCfg);
            }
        } else {
            CS_HCI_LOG("report config complete evt file lines:%s ,%d", __FILENAME__, __LINE__);
            hci_le_csConfigComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8 *)pCsCfg);
        }
    #else
        hci_le_csConfigComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8 *)pCsCfg);
    #endif
    } else {
        u8                            buff[sizeof(rf_packet_ll_cs_config_req_t)] = {0};
        rf_packet_ll_cs_config_req_t *req                                        = (rf_packet_ll_cs_config_req_t *)buff;

        req->type = LLID_CONTROL;

        if (pCsParam->cs_config_req & PROC_CS_CONFIG_SEND_RSP) {
            req->opcode    = LL_CS_CONFIG_RSP;
            req->Config_ID = pCsCfg->Config_ID;
            req->rf_len    = sizeof(rf_packet_ll_cs_config_rsp_t) - 2;
            CS_LL_LOG("[SEND_CONFIG_RSP] Config State: ", pCsCfg->state);
        } else if (pCsParam->cs_config_req & PROC_CS_CONFIG_SEND_REQ) {
            req->rf_len    = sizeof(rf_packet_ll_cs_config_req_t) - 2;
            req->opcode    = LL_CS_CONFIG_REQ;
            req->Config_ID = pCsCfg->Config_ID;
            req->State     = pCsCfg->state; //enable config
            smemcpy(req->ChM, pCsCfg->Channel_Map, 10);
            smemcpy(pCsCfg->Origin_Chn_Map, pCsCfg->Channel_Map, 10);
            req->ChM_Repetition       = pCsCfg->Channel_Map_Repetition;
            req->Main_Mode            = pCsCfg->Main_Mode;
            req->Sub_Mode             = pCsCfg->Sub_Mode;
            req->Main_Mode_Min_Steps  = pCsCfg->Main_Mode_Min_Steps;
            req->Main_Mode_Max_Steps  = pCsCfg->Main_Mode_Max_Steps;
            req->Main_Mode_Repetition = pCsCfg->Main_Mode_Repetition;
            req->Mode_0_Steps         = pCsCfg->Mode_0_Steps;
            req->CS_SYNC_PHY          = pCsCfg->CS_SYNC_PHY;
            req->RTT_Type             = pCsCfg->RTT_Type & 0x0f;
            req->Role                 = pCsCfg->Role & 0x01;
            req->Companion_Signal     = pCsCfg->Companion_Signal_Enable ? 1 : 0;
            req->ChSel                = pCsCfg->ChSel ? 1 : 0;
            req->Ch3cShape            = pCsCfg->Ch3c_Shape;
            req->Ch3cJump             = pCsCfg->Ch3c_Jump;

            req->T_IP1 = pCsCfg->T_IP1;
            req->T_IP2 = pCsCfg->T_IP2;
            req->T_FCS = pCsCfg->T_FCS;
            req->T_PM  = pCsCfg->T_PM;


            pCsCfg->T_IP1_Us       = T_IP_US[pCsCfg->T_IP1];
            pCsCfg->T_IP2_Us       = T_IP_US[pCsCfg->T_IP2];
            pCsCfg->T_FCS_Us       = T_FCS_US[pCsCfg->T_FCS];
            pCsCfg->T_PM_Us        = T_PM_US[pCsCfg->T_PM];
            pCsCfg->T_SW_Us        = 0; //will change in CS start procedure
            pCsCfg->antennaPathNum = 1; //will change in CS start procedure

            //tlkapi_send_string_data(DBG_CS_LL_LOG_MASK_EN,"[CS][LL][SEND_CFG_REQ]",buff,sizeof(rf_packet_ll_cs_config_req_t));
            blt_ll_debug_print_config_reqeust(req, "[SEND_CFG_REQ]");

            // /Pre-Release/LL/CS/PER/INI/BV-28-C   [Channel Sounding Configuration Procedure Collision, Peripheral, Initiator]
            // /Pre-Release/LL/CS/PER/REF/BV-28-C   [Channel Sounding Configuration Procedure Collision, Peripheral, Reflector]
            // if acl role is peripheral, It's not expected to send config req, will config collision
            csFlowCtrl.configCollision = (pAclConn->aclRole == ACL_ROLE_PERIPHERAL) ? 1 : 0;
            CS_EBQ_LOG("config collision, peripheral not expected to send config req")
        }


        if (ll_push_tx_fifo_handler(pAclConn->acl_conHandle | HANDLE_STK_FLAG, (u8 *)req)) {
            if (pCsParam->cs_config_req & PROC_CS_CONFIG_SEND_RSP) {
                pCsParam->cs_config_req       = 0;
                pAclConn->ll_rsp_timeout_tick = 0;
                hci_le_csConfigComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8 *)pCsCfg);
            } else {
                pCsParam->cs_config_req &= ~PROC_CS_CONFIG_SEND_REQ;
                pCsParam->cs_config_req |= PROC_CS_CONFIG_WAIT_RSP;
                pAclConn->ll_rsp_timeout_tick = clock_time() | 1;
            }
            pAclConn->csMapMask |= BIT(pCsCfg->idx);
        }
    }

    return BLE_SUCCESS;
}

ble_sts_t blt_ll_cs_exchangeCsStartProc(st_ll_conn_t *pAclConn, cs_config_t *pCsCfg)
{
    cs_param_t *pCsParam = &pAclConn->csParam;
    if (pCsParam->cs_req == PROC_CS_EVT_PENDING) {
        pCsParam->cs_req = 0;
    #if (LL_CS_CEN_INI_BI_01_C && LL_CS_CEN_INI_BV_20_C)
        if (csFlowCtrl.csRspCheckErr) {
            CS_EBQ_LOG("report proc complete evt in file %s line %d", __FILENAME__, __LINE__);
            hci_le_csProcedureEnableComplete_evt(HCI_ERR_INVALID_LMP_PARAMS, pAclConn->acl_conHandle, (u8 *)pCsCfg);
        } else if (csFlowCtrl.csStartErr) {
            pCsCfg->cs_procedure_en = 0; // cs start proc disable
            CS_EBQ_LOG("report proc complete evt in file %s line %d", __FILENAME__, __LINE__);
            hci_le_csProcedureEnableComplete_evt(HCI_ERR_INVALID_LMP_PARAMS, pAclConn->acl_conHandle, (u8 *)pCsCfg);
        } else {
            pCsCfg->cs_procedure_measurement_en = 1;
            CS_EBQ_LOG("report proc complete evt in file %s line %d", __FILENAME__, __LINE__);
            hci_le_csProcedureEnableComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8 *)pCsCfg);
        }
    #else
        pCsCfg->cs_procedure_measurement_en = 1;
        CS_EBQ_LOG("report proc complete evt in file %s line %d", __FILENAME__, __LINE__);
        hci_le_csProcedureEnableComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8 *)pCsCfg);
    #endif
    } else {
        u32 ll_cs_req_pkt_len = sizeof(rf_packet_ll_cs_req_t);
        u32 ll_cs_rsp_pkt_len = sizeof(rf_packet_ll_cs_rsp_t);
        u32 ll_cs_ind_pkt_len = sizeof(rf_packet_ll_cs_ind_t);

        if ((ll_cs_rsp_pkt_len > ll_cs_req_pkt_len) || (ll_cs_ind_pkt_len > ll_cs_req_pkt_len)) {
            CS_LL_LOG("[START] buff len abnormal:0x%x,0x%x,0x%x", ll_cs_req_pkt_len, ll_cs_rsp_pkt_len, ll_cs_ind_pkt_len);
        }
        u8 buff[sizeof(rf_packet_ll_cs_req_t)] = {0};

        if (pCsParam->cs_req & PROC_CS_SEND_REQ) {
            rf_packet_ll_cs_req_t *req = (rf_packet_ll_cs_req_t *)buff;

            /* IRQ protect to make sure 2 values are a pair, in case IRQ coming when calculating which lead to timing error
             * use other variable to get and store, to decrease IRQ disabling time */
            u32 r                 = irq_disable();
            u32 acl_ap_markTick   = pAclConn->ap_tick_mark;
            u16 acl_conn_markInst = pAclConn->conn_inst_mark;
            irq_restore(r);

            /* attention that "ap tick mark" maybe a future tick in about 100~200uS, because
             * "ap_tick_mark = bltSche.sSlot_tick_irq + IRQ_BTX_SEND_DELAY_US*SYSTEM_TIMER_TICK_1US " is executed in btx start IRQ, main_loop code
             * may run in 100uS */
            u32 tick_now = clock_time();
            int aclEvent_past;
            if ((u32)(acl_ap_markTick - tick_now) < IRQ_BTX_SEND_DELAY_US * SYSTEM_TIMER_TICK_1US) {
                aclEvent_past = 0;
            } else {
                aclEvent_past = (tick_now - acl_ap_markTick) / pAclConn->conn_intvl_tick;
            }


            if (aclEvent_past > 100) {
                write_dbg32(0x0018, aclEvent_past);
                BLMS_ERR_DEBUG(DBG_CS_FLOW_EN, 0x999A0000);
            }

            u8 delta_inter = 40;
            if (pAclConn->conn_intvl_n_1m25 > CONN_INTERVAL_30MS) {
                delta_inter = 10;
            } else if (pAclConn->conn_intvl_n_1m25 > CONN_INTERVAL_20MS) {
                delta_inter = 15;
            } else if (pAclConn->conn_intvl_n_1m25 > CONN_INTERVAL_15MS) {
                delta_inter = 20;
            } else if (pAclConn->conn_intvl_n_1m25 > CONN_INTERVAL_10MS) {
                delta_inter = 30;
            }

    #if (DBG_CREATE_FF0D_ERROR)
            int inst_inc         = 2;
            u16 future_aclEvtCnt = acl_conn_markInst + inst_inc;
    #else
            int inst_inc           = aclEvent_past + delta_inter;
            pCsCfg->connEventCount = acl_conn_markInst + inst_inc;
    #endif
    #if (!CS_SUBEVENT_LEN_EVALUATE)
            /*
             * The Offset_Min value shall be greater than or equal to 500 us and less than 4 seconds.
             */
            pCsCfg->offset_min = max2(CS_SUBEVT_OFFSET, (pAclConn->sSlot_duration * SSLOT_US_NUM + pCsCfg->sch_early_us));
            pCsCfg->offset_max = min2((pAclConn->conn_intvl_n_1m25 * 1250 - pCsCfg->offset_min), 4000000);
    #endif

    #if (DBG_CS_SCH_INIT)
            if (pCsCfg->offset_min > pCsCfg->offset_max) {
                write_dbg32(0x0018, pCsCfg->offset_min);
                write_dbg32(0x001C, pCsCfg->offset_max);
                BLMS_ERR_DEBUG(DBG_CS_SCH_INIT, 0xBBff0300);
            }
    #endif

            u8 min_ant          = (bltCsLocalSupportCap.Num_Antennas_Supported > pAclConn->csRemoteSupCap.Num_Antennas_Supported) ? pAclConn->csRemoteSupCap.Num_Antennas_Supported : bltCsLocalSupportCap.Num_Antennas_Supported;
            u8 min_max_ant_path = (bltCsLocalSupportCap.Max_Antenna_Paths_Supported > pAclConn->csRemoteSupCap.Max_Antenna_Paths_Supported) ? pAclConn->csRemoteSupCap.Max_Antenna_Paths_Supported : bltCsLocalSupportCap.Max_Antenna_Paths_Supported;
            u8 aci              = 0;
            if ((min_max_ant_path == 4) && (min_ant >= 2)) {
                aci = 7; //2:2
            } else {
                u8 min_ant_path1 = (min_max_ant_path > pAclConn->csRemoteSupCap.Num_Antennas_Supported) ? pAclConn->csRemoteSupCap.Num_Antennas_Supported : min_max_ant_path;
                u8 min_ant_path2 = (min_max_ant_path > bltCsLocalSupportCap.Num_Antennas_Supported) ? bltCsLocalSupportCap.Num_Antennas_Supported : min_max_ant_path;
                if (pCsCfg->Role == CHANNEL_SOUNDING_ROLE_INITIATOR) {
                    if (bltCsLocalSupportCap.Num_Antennas_Supported < pAclConn->csRemoteSupCap.Num_Antennas_Supported) {
                        aci = min_ant_path1 + 2; //1:min_ant_path1
                    } else {
                        aci = min_ant_path2 - 1; //min_ant_path2 : 1
                    }
                } else if (pCsCfg->Role == CHANNEL_SOUNDING_ROLE_REFLECTOR) {
                    if (bltCsLocalSupportCap.Num_Antennas_Supported <= pAclConn->csRemoteSupCap.Num_Antennas_Supported) {
                        aci = min_ant_path2 - 1; //min_ant_path2 : 1
                    } else {
                        aci = min_ant_path1 + 2; //1:min_ant_path1
                    }
                }
            }
            pCsCfg->aci = aci;
            // [PBLE-169]Broadcom 20240130 session15:if no cs cap exchange before,csRemote ant and ant path will equal = 0,
            // we should consider this situation if no cap exchange , just set aci = 0 (A1_B1)
            if (pAclConn->csRemoteSupCap.Num_Antennas_Supported == 0 && pAclConn->csRemoteSupCap.Max_Antenna_Paths_Supported == 0) {
                CS_LL_LOG("[CS REQ/RSP] No cs cap exchange before");
                pCsCfg->aci = 0;
            }

    #if CS_SUBEVENT_LEN_EVALUATE
            blt_ll_calcStepDuration(pCsCfg); //250us@96Mhz
            blt_ll_cs_subevent_len_cal(pAclConn, pCsCfg->Config_ID);
    #endif
            //          pCsCfg->Preferred_Peer_Ant = BIT(0); //todo by biao 2023.1211

            req->type                = LLID_CONTROL;
            req->rf_len              = (u8)(ll_cs_req_pkt_len - 2);
            req->opcode              = LL_CS_REQ;
            req->Config_ID           = pCsCfg->Config_ID;
            req->connEventCount      = pCsCfg->connEventCount;
            req->Offset_Min[0]       = (u8)(pCsCfg->offset_min & 0xff);
            req->Offset_Min[1]       = (u8)((pCsCfg->offset_min >> 8) & 0xff);
            req->Offset_Min[2]       = (u8)((pCsCfg->offset_min >> 16) & 0xff);
            req->Offset_Max[0]       = (u8)(pCsCfg->offset_max & 0xff);
            req->Offset_Max[1]       = (u8)((pCsCfg->offset_max >> 8) & 0xff);
            req->Offset_Max[2]       = (u8)((pCsCfg->offset_max >> 16) & 0xff);
            req->Max_Procedure_Len   = pCsCfg->Max_Procedure_Len;
            req->Event_Interval      = pCsCfg->Event_Interval;
            req->Subevents_Per_Event = pCsCfg->Subevents_Per_Event;
            req->Subevent_Interval   = pCsCfg->subEvtIntvl_625us;
            req->Subevent_Len[0]     = (u8)(pCsCfg->Subevent_Len & 0xff);
            req->Subevent_Len[1]     = (u8)((pCsCfg->Subevent_Len >> 8) & 0xff);
            req->Subevent_Len[2]     = (u8)((pCsCfg->Subevent_Len >> 16) & 0xff);
            req->Procedure_Interval  = pCsCfg->Procedure_Interval;
            req->Procedure_Count     = pCsCfg->procMaxCountInstant;
            req->ACI                 = pCsCfg->aci;
            req->Preferred_Peer_Ant  = pCsCfg->Preferred_Peer_Ant;
            req->PHY                 = pCsCfg->PHY;
            req->Pwr_Delta           = pCsCfg->Tx_Pwr_Delta;

            //tlkapi_send_string_data(DBG_CS_LL_LOG_MASK_EN,"[CS][LL][CS_SEND_REQ]",buff,sizeof(rf_packet_ll_cs_req_t));
            blt_ll_debug_print_cs_reqeust(req, "[CS_SEND_REQ]");
        } else if (pCsParam->cs_req & PROC_CS_SEND_RSP) {
            rf_packet_ll_cs_rsp_t *rsp = (rf_packet_ll_cs_rsp_t *)buff;
            rsp->type                  = LLID_CONTROL;
            rsp->rf_len                = ll_cs_rsp_pkt_len - 2;
            rsp->opcode                = LL_CS_RSP;
            rsp->Config_ID             = pCsCfg->Config_ID;
            rsp->connEventCount        = pCsCfg->connEventCount;
            rsp->Offset_Min[0]         = (u8)(pCsCfg->offset_min & 0xff);
            rsp->Offset_Min[1]         = (u8)((pCsCfg->offset_min >> 8) & 0xff);
            rsp->Offset_Min[2]         = (u8)((pCsCfg->offset_min >> 16) & 0xff);
            rsp->Offset_Max[0]         = (u8)(pCsCfg->offset_max & 0xff);
            rsp->Offset_Max[1]         = (u8)((pCsCfg->offset_max >> 8) & 0xff);
            rsp->Offset_Max[2]         = (u8)((pCsCfg->offset_max >> 16) & 0xff);
            rsp->Event_Interval        = pCsCfg->Event_Interval;
            rsp->Subevents_Per_Event   = pCsCfg->Subevents_Per_Event;
            rsp->Subevent_Interval     = pCsCfg->subEvtIntvl_625us;
            rsp->Subevent_Len[0]       = (u8)(pCsCfg->Subevent_Len & 0xff);
            rsp->Subevent_Len[1]       = (u8)((pCsCfg->Subevent_Len >> 8) & 0xff);
            rsp->Subevent_Len[2]       = (u8)((pCsCfg->Subevent_Len >> 16) & 0xff);
            rsp->ACI                   = pCsCfg->aci;
            rsp->PHY                   = pCsCfg->PHY;
            rsp->Pwr_Delta             = pCsCfg->Tx_Pwr_Delta;

            //tlkapi_send_string_data(DBG_CS_LL_LOG_MASK_EN,"[CS][LL][CS_SEND_RSP]",buff,sizeof(rf_packet_ll_cs_rsp_t));
            blt_ll_debug_print_cs_response(rsp, "[CS_SEND_RSP]");
        } else if (pCsParam->cs_req & PROC_CS_SEND_IND) {
            /*
             *  The connEventCoun value supplied in either the LL_CS_RSP or
             *  LL_CS_IND PDU shall be no sooner in time than the value received
             *  in the LL_CS_REQ or LL_CS_RSP PDU that is being responded to
             *  todo  Note: Not considering the case of numeric wrap  fanqh
             */
            pCsCfg->connEventCount  = max2(pAclConn->conn_inst_mark + 10, pCsCfg->connEventCount);
            pCsCfg->inst_start_proc = pCsCfg->connEventCount;


            rf_packet_ll_cs_ind_t *ind = (rf_packet_ll_cs_ind_t *)buff;
            ind->type                  = LLID_CONTROL;
            ind->rf_len                = ll_cs_ind_pkt_len - 2;
            ind->opcode                = LL_CS_IND;
            ind->Config_ID             = pCsCfg->Config_ID;
            ind->connEventCount        = pCsCfg->connEventCount;
            ind->Offset[0]             = (u8)(pCsCfg->csOft_us & 0xff);
            ind->Offset[1]             = (u8)((pCsCfg->csOft_us >> 8) & 0xff);
            ind->Offset[2]             = (u8)((pCsCfg->csOft_us >> 16) & 0xff);
            ind->Event_Interval        = pCsCfg->Event_Interval;
            ind->Subevents_Per_Event   = pCsCfg->Subevents_Per_Event;
            ind->Subevent_Interval     = pCsCfg->subEvtIntvl_625us;
            ind->Subevent_Len[0]       = (u8)(pCsCfg->Subevent_Len & 0xff);
            ind->Subevent_Len[1]       = (u8)((pCsCfg->Subevent_Len >> 8) & 0xff);
            ind->Subevent_Len[2]       = (u8)((pCsCfg->Subevent_Len >> 16) & 0xff);
            ind->ACI                   = pCsCfg->aci;
            ind->PHY                   = pCsCfg->PHY;
            ind->Pwr_Delta             = pCsCfg->Tx_Pwr_Delta;

            pCsCfg->sSlotCsDuration  = (pCsCfg->Subevent_Len + SLOT_PROCESS_MAX_US + pCsCfg->sch_early_us) * SSLOT_US_REVERSE + 1;
            pCsCfg->sSlot_csSubIntvl = BSLOT_DUR_2_SSLOT_DUR(pCsCfg->subEvtIntvl_625us);

            //tlkapi_send_string_data(DBG_CS_LL_LOG_MASK_EN,"[CS][LL][CS_SEND_IND]",buff,sizeof(rf_packet_ll_cs_ind_t));
            blt_ll_debug_print_cs_ind(ind, "[CS_SEND_IND]");
        }


        if (ll_push_tx_fifo_handler(pAclConn->acl_conHandle | HANDLE_STK_FLAG, (u8 *)buff)) //blt_llms_pushTxfifo
        {
            if (pCsParam->cs_req & PROC_CS_SEND_REQ) {
                pCsParam->cs_req &= ~PROC_CS_SEND_REQ;
                if (pAclConn->aclRole == ACL_ROLE_PERIPHERAL) {
                    pCsParam->cs_req |= PROC_CS_WAIT_IND;
                } else { //ACL_ROLE_CENTRAL
                    pCsParam->cs_req |= PROC_CS_WAIT_RSP;
                }
                pAclConn->ll_rsp_timeout_tick = clock_time() | 1;
            } else if (pCsParam->cs_req & PROC_CS_SEND_RSP) {
                pCsParam->cs_req &= ~PROC_CS_SEND_RSP;
                pCsParam->cs_req |= PROC_CS_WAIT_IND;
                pAclConn->ll_rsp_timeout_tick = clock_time() | 1;
            } else if (pCsParam->cs_req & PROC_CS_SEND_IND) {
    #if (CS_EBQ_TEST && APP_POWER_CONTROL)
                if (pCsParam->cs_req & PROC_CS_PWL_PENDING) {
                    CS_EBQ_LOG("[TX]after send cs ind, Send PWL req");
                    blc_ll_readRemoteTxPwrLvl(pAclConn->acl_conHandle, 0, 1);
                }
    #endif
                pCsParam->cs_req                    = 0;
                pCsCfg->cs_procedure_measurement_en = 1;
                pAclConn->ll_rsp_timeout_tick       = 0;
                pAclConn->cs_pending |= (pCsCfg->idx | CS_IDX_FLG);
                blt_ll_calcStepDuration(pCsCfg);
    /*
                 * EBQ case LL/CS/CEN/REF/BV-01-C:after ll_cs_ind push to tx fifo,mark the tx_wptr.set pending flag
                 * check markTXFifoWptr in tx_post, when it comes, set csReportEvtFlag and report event.
                 */
    #if (LL_CS_CEN_REF_BV_01_C)
                pAclConn->indFlagPending     = 1;
                pAclConn->connMarkTxFifoWptr = pAclConn->tx_wptr;
    #else
                CS_EBQ_LOG("report proc complete evt in file %s line %d", __FILENAME__, __LINE__);
                hci_le_csProcedureEnableComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8 *)pCsCfg);
    #endif
            }
        }
    }

    return BLE_SUCCESS;
}

ble_sts_t blt_ll_cs_exchangeCsProcedureRepeatTerminateProc(st_ll_conn_t *pAclConn, cs_config_t *pCsCfg)
{
    cs_param_t *pCsParam = &pAclConn->csParam;

    if (csFlowCtrl.csTermiFlag == Remove_CS_Task_with_Terminate && pCsParam->cs_terminate_ind == PROC_CS_TERMINATE_EVT_PENDING) {
        // Run #3879 - /Pre-Release/LL/CS/CEN/INI/BV-26-C   [CS Procedure Receive Repeat Termination, Central, Initiator]
        // Run #3880 - /Pre-Release/LL/CS/CEN/INI/BV-33-C   [CS Procedure Sends Repeat Termination, Central, Initiator]
        // if cs procedure enable now, report evt after this instance done. Otherwise, report immediately
        pCsParam->cs_terminate_ind = 0;
        if (pAclConn->csTaskEnableMask & BIT(pCsCfg->idx)) { // check if cs procedure
            pCsCfg->csReportTermiEvt = 1;                    // wait current cs procedure complete
        } else {
            pCsCfg->cs_procedure_measurement_en = 0;
            pCsCfg->cs_procedure_en             = 0;
            pCsParam->cs_terminate_ind          = 0;
            CS_EBQ_LOG("report proc complete evt(disable) in file %s line %d", __FILENAME__, __LINE__);
            hci_le_csProcedureEnableComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8 *)pCsCfg);
        }
        // after a cs terminate procedure done,should enable cs state.this is to insure can start a new cs procedure
        pCsCfg->state = 1;
    } else {
        u8                            buff[sizeof(rf_pkt_ll_cs_terminate_req_t)] = {0};
        rf_pkt_ll_cs_terminate_req_t *ind                                        = (rf_pkt_ll_cs_terminate_req_t *)buff;
        if (pCsParam->cs_terminate_ind & PROC_CS_TERMINATE_SEND_REQ) {
            ind->type      = LLID_CONTROL;
            ind->rf_len    = sizeof(rf_pkt_ll_cs_terminate_req_t) - 2;
            ind->opcode    = LL_CS_TERMINATE_REQ;
            ind->Config_ID = pCsCfg->Config_ID;
            ind->ProcCount = pCsCfg->csProcCount;
            ind->ErrorCode = pCsParam->cs_terminate_error_code; //todo yuexin
            tlkapi_send_string_data((stkLog_mask & STK_LOG_LL_CS), "[send]terminate req", &ind->opcode, 5);
            CS_LL_LOG("[TERMIN][TX] terminate req CID and errorCode:0x%x,0x%x", ind->Config_ID, ind->ErrorCode);
        }

        if (pCsParam->cs_terminate_ind & PROC_CS_TERMINATE_SEND_RSP) {
            ind->type      = LLID_CONTROL;
            ind->rf_len    = sizeof(rf_pkt_ll_cs_terminate_req_t) - 2;
            ind->opcode    = LL_CS_TERMINATE_RSP;
            ind->Config_ID = pCsCfg->Config_ID;
            ind->ProcCount = pCsCfg->csProcCount;
            ind->ErrorCode = pCsParam->cs_terminate_error_code; //todo yuexin

            /* if send or receive terminate rsp, next cs procedure should be terminate, flag it,
             * then use flag cs_pending to stop cs procedure while insert cs task*/
            csFlowCtrl.csTermiFlag = Receive_Send_CS_Terminate_RSP;
            CS_LL_LOG("[TERMIN][TX] terminate rsp CID and errorCode:0x%x,0x%x", ind->Config_ID, ind->ErrorCode);
        }
        if (pCsParam->cs_terminate_ind & (PROC_CS_TERMINATE_SEND_REQ | PROC_CS_TERMINATE_SEND_RSP)) {
            if (ll_push_tx_fifo_handler(pAclConn->acl_conHandle | HANDLE_STK_FLAG, (u8 *)buff)) //blt_llms_pushTxfifo
            {
                if (pCsParam->cs_terminate_ind & PROC_CS_TERMINATE_SEND_REQ) {
                    pCsParam->cs_terminate_ind &= ~PROC_CS_TERMINATE_SEND_REQ;
                } else if (pCsParam->cs_terminate_ind & PROC_CS_TERMINATE_SEND_RSP) {
                    pCsParam->cs_terminate_ind &= ~PROC_CS_TERMINATE_SEND_RSP;
                    pCsParam->cs_terminate_ind = PROC_CS_TERMINATE_EVT_PENDING;
                }
            }
        }
    }

    return BLE_SUCCESS;
}

ble_sts_t blt_ll_cs_chnMapUpdateProce(void)
{
    ble_sts_t                   ret = BLE_SUCCESS;
    u8                          buff[sizeof(rf_pkt_ll_cs_chn_map_ind_t)];
    rf_pkt_ll_cs_chn_map_ind_t *chm_ind = (rf_pkt_ll_cs_chn_map_ind_t *)buff;

    chm_ind->type   = LLID_CONTROL;
    chm_ind->rf_len = sizeof(rf_pkt_ll_cs_chn_map_ind_t) - 2;

    memcpy(chm_ind->ChM, gCsMng.chn_map, 10);
    chm_ind->opcode = LL_CS_CHANNEL_MAP_IND;
    CS_LL_LOG("[CHNL][TX] chnl map ind");


    for (u32 i = 0; i < gCsMng.max_num_cofig; i++) {
        cs_config_t *pCfg = gCsMng.gGlobal_pCsCfg + i;
        if (pCfg->state) {
            st_ll_conn_t *pAclConn = (st_ll_conn_t *)(u32)&blms[pCfg->aclHandle & CONN_IDX_MASK];

            smemcpy(pCfg->Chm_Ind_Map, gCsMng.chn_map, 10);

            pCfg->chn_update_pend = 1;
            if (pCfg->cs_procedure_measurement_en) {
                pCfg->chn_update_inst = pCfg->inst_start_proc + pCfg->Procedure_Interval;
                chm_ind->instant      = pCfg->chn_update_inst;

                tlkapi_send_string_u32s(DBG_CS_LL_LOG_MASK_EN, "chm updat meas en", pCfg->chn_update_inst, pCfg->inst_start_proc, pCfg->Procedure_Interval);

            } else {
                pCfg->chn_update_inst = pAclConn->conn_inst + 6;
                chm_ind->instant      = pCfg->chn_update_inst;

                tlkapi_send_string_u32s(DBG_CS_LL_LOG_MASK_EN, "chm updat meas disable", pCfg->chn_update_inst, pAclConn->conn_inst, pCfg->Procedure_Interval);
            }
            CS_LL_LOG("send cs chn map update:%d", pCfg->chn_update_inst);
            gCsMng.chn_map_upt_tick = 0;
            ll_push_tx_fifo_handler(pAclConn->acl_conHandle | HANDLE_STK_FLAG, (u8 *)chm_ind);
        }
    }
    return ret;
}

/* validation of LL/CS/CEN/BI-02-C and LL/CS/PER/BI-02-C */
ble_sts_t blt_ll_cs_checkCapabilitiesParam(st_ll_conn_t *pAclConn, u8 opcode, u8 *raw)
{
    (void)opcode; //clean warning: unused variable 'opcode' [-Wunused-variable] by SunWei
    (void)pAclConn;
    rf_packet_ll_cs_cap_req_t *pReq = (rf_packet_ll_cs_cap_req_t *)raw;

    if ((1 > pReq->Num_Configs) || (pReq->Num_Configs > 4)) { //valid num configs is between <1 ; 4>
        CS_LL_LOG("[CHK_CAP]num configs abnormal:0x%x", pReq->Num_Configs);
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    if ((1 > pReq->Num_Ant) || (pReq->Num_Ant > 4)) { //valid num ant is between <1 ; 4>
        CS_LL_LOG("[CHK_CAP]num ant abnormal:0x%x", pReq->Num_Ant);
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    /* The Max_Ant_Path field shall be set to the maximum number of antenna paths that are supported by the
     * device for CS tone exchanges. The field shall be set to a value between 1 and 4 and shall be greater than
     * or equal to the value set for the Num_Ant field. */
    if ((1 > pReq->Max_Ant_Path) || (pReq->Max_Ant_Path > 4) || (pReq->Max_Ant_Path < pReq->Num_Ant)) { //valid max ant path is between <1 ; 4> and >= Num_Ant
        CS_LL_LOG("[CHK_CAP]max ant path abnormal:0x%x, 0x%x", pReq->Max_Ant_Path, pReq->Num_Ant);
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    /* At least one of the roles needs to be available */
    /* Use CS_INIT_REFL_ROLE as a 0b00000011 bitmask  */
    if ((pReq->Role & CS_INIT_REFL_ROLE) == 0) { //no valid role
        CS_LL_LOG("[CHK_CAP]role abnormal:0x%x", pReq->Role);
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    return 0;
}

/* LL/CS/CEN/INI/BI-03-C, LL/CS/CEN/REF/BI-04-C, LL/CS/PER/INI/BI-04-C, LL/CS/PER/REF/BI-05-C */
/* ConfigID, MainMode, SubMode, CS_SYNC_PHY, Role, ChMRep, MainModeRep, Mode0Steps, Ch3cJump, Ch3cShape, Companion_Signal, ChSel, RTT_Type checks */
/* LL/CS/PER/INI/BI-06-C, LL/CS/CEN/INI/BI-05-C, LL/CS/PER/REF/BI-07-C, LL/CS/CEN/REF/BI-06-C */
/* T_IP1, T_IP2, T_FCS, Main_Mode, Sub_Mode, RTT_Types, ChSel (we don't limit channels within capabilities) within capabilities */
//refer Channel sounding CRr10 P72
ble_sts_t blt_ll_cs_checkConfigParam(st_ll_conn_t *pAclConn, u8 opcode, u8 *raw)
{
    (void)pAclConn; //clean warning: unused variable 'pAclConn' [-Wunused-variable] by SunWei
    (void)opcode;   //clean warning: unused variable 'opcode' [-Wunused-variable] by SunWei
    rf_packet_ll_cs_config_req_t *pConfigReq = (rf_packet_ll_cs_config_req_t *)raw;
    // remote or local don't support mode3

    if ((pConfigReq->Config_ID > 3) || (pConfigReq->State > 1) || (pConfigReq->Main_Mode > 3) || (pConfigReq->Main_Mode == 0) || ((pConfigReq->Sub_Mode > 3) && (pConfigReq->Sub_Mode != 0xff)) || (pConfigReq->Sub_Mode == 0)) {
        CS_LL_LOG("[CHK_CFG_P] Config_ID abnormal:0x%x,0x%x,0x%x", pConfigReq->Config_ID, pConfigReq->Main_Mode, pConfigReq->Sub_Mode);
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    /* LL/CS/PER/INI/BI-06-C, LL/CS/CEN/INI/BI-05-C, LL/CS/PER/REF/BI-07-C, LL/CS/CEN/REF/BI-06-C */
    /* T_IP1, T_IP2, T_FCS, Main_Mode, Sub_Mode */
    chn_sound_capabilities_t *csLocalCap = &bltCsLocalSupportCap;

    if (!(csLocalCap->Mode_Types & BIT(0))) //local not support mode3
    {
        if ((pConfigReq->Main_Mode == 0x03) || (pConfigReq->Sub_Mode == 0x03)) {
            CS_LL_LOG("[CHK_CFG_P] local not support mode3:0x%x,0x%x", pConfigReq->Main_Mode, pConfigReq->Sub_Mode);
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
    }

    if ((1 << pConfigReq->T_IP1) != CS_T_IP_145US) { //only check further if not of mandatory value
        if (!(csLocalCap->Optional_T_IP1_Times_Supported & pConfigReq->T_IP1)) {
            CS_LL_LOG("[CHK_CFG_P] T_IP1 value not supported :0x%x,0x%x", pConfigReq->T_IP1, csLocalCap->Optional_T_IP1_Times_Supported);
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
    }

    if ((1 << pConfigReq->T_IP2) != CS_T_IP_145US) { //only check further if not of mandatory value
        if (!(csLocalCap->Optional_T_IP2_Times_Supported & pConfigReq->T_IP2)) {
            CS_LL_LOG("[CHK_CFG_P] T_IP2 value not supported :0x%x,0x%x", pConfigReq->T_IP2, csLocalCap->Optional_T_IP2_Times_Supported);
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
    }

    if ((1 << pConfigReq->T_FCS) != CS_T_FCS_150US) { //only check further if not of mandatory value
        if (!(csLocalCap->Optional_T_FCS_Times_Supported & pConfigReq->T_FCS)) {
            CS_LL_LOG("[CHK_CFG_P] T_FCS value not supported :0x%x,0x%x", pConfigReq->T_FCS, csLocalCap->Optional_T_FCS_Times_Supported);
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
    }

    //Submode != 0xFF checks
    if (pConfigReq->Sub_Mode != 0xFF) {
        //TBD: Confirm expected behaviour with BT SIG, maybe it makes more sense to change to a MainMode
        //only in this case instead of making this an issue
        if (pConfigReq->Main_Mode == pConfigReq->Sub_Mode) {
            CS_LL_LOG("[CHK_CFG_P] Main_Mode same as Sub_Mode:0x%x, 0x%x", pConfigReq->Main_Mode, pConfigReq->Sub_Mode);
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
        // PBLE-186:mainmode + submode combination check,we now only support mainmode mode2 + submode mode1
        if (pConfigReq->Main_Mode == CS_MODE2 && pConfigReq->Sub_Mode == CS_MODE1) {
            CS_LL_LOG("[CHK_CFG_P]mainmode mode1 + submode mode2");
        } else {
            CS_LL_LOG("[CHK_CFG_P]mainmode&submode combination not support/invalid value");
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }


        //validation of Min/Max steps settings
        /* If the Sub_Mode field has been set to None, then these two fields shall be reserved for future use.
         * Otherwise, Main_Mode_Min_Steps shall be greater than 1 and less than or equal to N_STEPS_MAX - 1.
         * Main_Mode_Max_Steps shall be greater than or equal to Main_Mode_Min_Steps and less than or equal to N_STEPS_MAX - 1.
         * Spec errata: https://bluetooth.atlassian.net/browse/ES-23402
         * LL_CS_CONFIG_REQ requirement for Main_Mode_Min_Steps should be >= 1 instead of >1
         * CS_STEPS_PER_PROCEDURE_MAX is outside of the type - test spec errata created to clarify ES-24812
         */

        if ((pConfigReq->Main_Mode_Min_Steps < 1) || (pConfigReq->Main_Mode_Min_Steps > pConfigReq->Main_Mode_Max_Steps)) { //min steps greater than max
                                                                                                                            //          || (pConfigReq->Main_Mode_Min_Steps >= CS_STEPS_PER_PROCEDURE_MAX) //min steps > N_STEPS_MAX-1
                                                                                                                            //          || (pConfigReq->Main_Mode_Max_Steps >= CS_STEPS_PER_PROCEDURE_MAX)) //max steps > N_STEPS_MAX-1
            CS_LL_LOG("[CHK_CFG_P] Main_Mode step abnormal:0x%x,0x%x", pConfigReq->Main_Mode_Min_Steps, pConfigReq->Main_Mode_Max_Steps);
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
    }

    if ((pConfigReq->Main_Mode_Repetition > 3) || (pConfigReq->Mode_0_Steps == 0) || (pConfigReq->Mode_0_Steps > 3) || (pConfigReq->Role > 1) || (pConfigReq->RTT_Type > 6) || (pConfigReq->CS_SYNC_PHY == 0) || (pConfigReq->CS_SYNC_PHY > 2) || (pConfigReq->ChSel > 1) || (pConfigReq->Ch3cShape > 1)) {
        CS_LL_LOG("[CHK_CFG_P] role abnormal:0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x", pConfigReq->Main_Mode_Repetition, pConfigReq->Mode_0_Steps, pConfigReq->Role, pConfigReq->RTT_Type, pConfigReq->CS_SYNC_PHY, pConfigReq->ChSel, pConfigReq->Ch3cShape);
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    /* LL/CS/PER/INI/BI-06-C, LL/CS/CEN/INI/BI-05-C, LL/CS/PER/REF/BI-07-C, LL/CS/CEN/REF/BI-06-C */
    /* RTT_Types within capabilities */
    // >6 condition is already checked above
    if (pConfigReq->RTT_Type >= RTT_Type_32bit_rs) {
        if (csLocalCap->RTT_Random_Payload_N == 0) {
            CS_LL_LOG("[CHK_CFG_P] RTT_Type random sequence not supported:0x%x, 0x%x", pConfigReq->RTT_Type, csLocalCap->RTT_Random_Payload_N);
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
    }
    // >2 conditions already checked above
    if (pConfigReq->RTT_Type >= RTT_Type_32bit_ss) {
        if (csLocalCap->RTT_Sounding_N == 0) {
            CS_LL_LOG("[CHK_CFG_P] RTT_Type sounding sequence not supported:0x%x, 0x%x", pConfigReq->RTT_Type, csLocalCap->RTT_Sounding_N);
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
    }
    // >0 conditions already checked above
    if (pConfigReq->RTT_Type >= RTT_Type_AA_Only) {
        if (csLocalCap->RTT_AA_Only_N == 0) {
            CS_LL_LOG("[CHK_CFG_P] RTT_Type AA only not supported:0x%x, 0x%x", pConfigReq->RTT_Type, csLocalCap->RTT_AA_Only_N);
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
    }
    /* LL/CS/CEN/INI/BV-05-C: only chsel equal to ch3c, ch3cjump will be judged*/
    if (((pConfigReq->ChSel == 1) && (pConfigReq->Ch3cJump < 2 || pConfigReq->Ch3cJump > 8)) || (pConfigReq->Companion_Signal > 1)) {
        CS_LL_LOG("[CHK_CFG_P] Ch3cJump abnormal:0x%x,0x%x,0x%x", pConfigReq->ChSel, pConfigReq->Ch3cJump, pConfigReq->Companion_Signal);
    #if (LL_CS_CEN_INI_BI_05_C)
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    #endif
    }
    if (pConfigReq->ChM_Repetition == 0) {
        CS_LL_LOG("[CHK_CFG_P] chn num abnormal:%d", pConfigReq->ChM_Repetition);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    return BLE_SUCCESS;
}

/* (LL_CS_REQ) LL/CS/CEN/INI/BI-06-C, LL/CS/CEN/REF/BI-07-C, LL/CS/PER/INI/BI-07-C, LL/CS/PER/REF/BI-08-C */
/* Offset_Min, Offset_Max, Offset_Max>Offset_Min, Event_interval, Subevents_Per_Event, Subevent_Len */
ble_sts_t blt_ll_cs_checkCsreqParam(st_ll_conn_t *pAclConn, u8 opcode, u8 *raw, cs_config_t *pCsCfg)
{
    (void)opcode; //clean warning: unused variable 'opcode' [-Wunused-variable] by SunWei
    rf_packet_ll_cs_req_t *pConfigReq = (rf_packet_ll_cs_req_t *)raw;

    #if (LL_FEATURE_ENABLE_POWER_CONTROL)
    if ((pAclConn->ll_remoteFeature1 & LL_FEATURE_MASK_LE_PATH_LOSS_MONITORING) && !csFlowCtrl.csPowerCtrl) {
        CS_EBQ_LOG("[CHK_CS_REQ]Power Enable but not monitor");
        return HCI_ERR_UNSUPPORTED_LMP_PARAM_VAL;
    }
    #endif
    /*LL/CS/CEN/INI/BI-06-C [Reject a CS Start Procedure With Invalid Parameters , Central, Initiator]
      LL/CS/CEN/REF/BI-07-C [Reject a CS Start Procedure With Invalid Parameters , Central, Reflector]
      LL/CS/PER/INI/BI-07-C [Reject a CS Start Procedure With Invalid Parameters , Peripheral, Initiator]
      LL/CS/PER/REF/BI-08-C [Reject a CS Start Procedure With Invalid Parameters , Peripheral, Reflector]*/
    if (pConfigReq->Subevents_Per_Event > 32) {
        CS_LL_LOG("[CHK_CS_REQ]Subevents_Per_Event can't larger than 32");
    }
    #if (LL_CS_CEN_INI_BI_08_C)
    if (csChnNum > 0 && csChnNum < 15) {
        CS_EBQ_LOG("[CHK_CS_REQ]num channel fewr than 15:%d", csChnNum);
        return HCI_ERR_INSUFFICIENT_CHANNELS;
    }
    #endif
    unsigned int Subevent_len = pConfigReq->Subevent_Len[0] | (pConfigReq->Subevent_Len[1] << 8) | (pConfigReq->Subevent_Len[2] << 16);
    if (pConfigReq->Subevent_Interval && pConfigReq->Subevent_Interval * 625 < Subevent_len + TLK_T_MES) {
        CS_LL_LOG("[CHK_CS_REQ]SubIntvl should larger than SubLen+T_MES");
        return HCI_ERR_INVALID_LMP_PARAMS;
    }
    if (pConfigReq->Subevents_Per_Event > 1 && pConfigReq->Subevent_Interval == 0) {
        CS_LL_LOG("[CHK_CS_REQ]SubIntvl wrong");
        return HCI_ERR_INVALID_LMP_PARAMS;
    }

    u32 off_min = pConfigReq->Offset_Min[0] | (pConfigReq->Offset_Min[1] << 8) | (pConfigReq->Offset_Min[2] << 16);
    u32 off_max = pConfigReq->Offset_Max[0] | (pConfigReq->Offset_Max[1] << 8) | (pConfigReq->Offset_Max[2] << 16);
    if ((CS_EVENT_OFFSET_MIN > off_min) || (off_min >= CS_EVENT_OFFSET_MAX)) { //500<=offset_min <4000*1000 us
        CS_LL_LOG("[CHK_CS_REQ]offset abnormal:0x%x", off_min);
        return HCI_ERR_INVALID_LMP_PARAMS;
    }

    if ((off_max < off_min) || (off_max >= (pAclConn->conn_intvl_n_1m25 * 1250))) { //offset_min<= offset_max < le connection interval
        CS_LL_LOG("[CHK_CS_REQ]conn_interval abnormal:0x%x,0x%x,0x%x", off_min, off_max, pAclConn->conn_intvl_n_1m25);
        return HCI_ERR_INVALID_LMP_PARAMS;
    }

    if (pConfigReq->Event_Interval < 1) { //1<= event_interval <= 65535
        CS_LL_LOG("[CHK_CS_REQ]Event_Interval abnormal:0x%x", pConfigReq->Event_Interval);
        return HCI_ERR_INVALID_LMP_PARAMS;
    }

    if (pConfigReq->Subevents_Per_Event < 1) { //subevent_per_event >=1
        CS_LL_LOG("[CHK_CS_REQ]Per_Event abnormal:0x%x", pConfigReq->Subevents_Per_Event);
        return HCI_ERR_INVALID_LMP_PARAMS;
    }

    if (pConfigReq->Subevents_Per_Event <= 1) { //subevent interval if(subevent_per_event >1){valid}else{ be set to 0}
        pConfigReq->Subevent_Interval = 0;
    }

    u32 subevent_len = (pConfigReq->Subevent_Len[0] | (pConfigReq->Subevent_Len[1] << 8) | (pConfigReq->Subevent_Len[2] << 16));
    if ((CS_SUBEVENT_LEN_MIN > subevent_len) || (subevent_len >= CS_SUBEVENT_LEN_MAX)) { //    1250<=subevent len < 4000*1000
        CS_LL_LOG("[CHK_CS_REQ]subevent_len abnormal:0x%x", subevent_len);
        return HCI_ERR_INVALID_LMP_PARAMS;
    }

    if (pConfigReq->Preferred_Peer_Ant & 0xf0) { //Preferred_Peer_Ant  bit0--3 valid
        CS_LL_LOG("[CHK_CS_REQ]Preferred_Peer_Ant abnormal:0x%x", pConfigReq->Preferred_Peer_Ant);
        return HCI_ERR_INVALID_LMP_PARAMS;
    }
    // LL/CS/CEN/REF/BV-33-C,this case used 2M phy as default
    // CS SYNC 2M PHY Now Support
#if(0)
    if (!(pAclConn->ll_remoteFeature1 & LL_FEATURE_MASK_LE_PATH_LOSS_MONITORING) && (pConfigReq->PHY != pCsCfg->CS_SYNC_PHY)) {
        CS_LL_LOG("[CHK_CS_REQ]PHY abnormal:0x%x,0x%x", pConfigReq->PHY, pCsCfg->CS_SYNC_PHY);
        return HCI_ERR_UNSUPPORTED_LMP_PARAM_VAL;
    }
#endif

    if (pConfigReq->ACI & 0xf0) { //0-7
        CS_LL_LOG("[CHK_CS_REQ]ACI abnormal:0x%x", pConfigReq->ACI);
        return HCI_ERR_INVALID_LMP_PARAMS;
    }

    u8 maxAntPathTwoDevice = max(bltCsLocalSupportCap.Max_Antenna_Paths_Supported, pAclConn->csRemoteSupCap.Max_Antenna_Paths_Supported);
    if (ACI_TO_MAX_PATH(pConfigReq->ACI) > maxAntPathTwoDevice) {
        CS_LL_LOG("[CHK_CS_REQ]Error: ACI Max Path(%d) > Max ANT Path(%d)", ACI_TO_MAX_PATH(pConfigReq->ACI), maxAntPathTwoDevice);
        return HCI_ERR_INVALID_LMP_PARAMS;
    }

    if (COUNT_BIT_1(pConfigReq->Preferred_Peer_Ant) > ACI_TO_MAX_PATH(pConfigReq->ACI)) {
        CS_LL_LOG("[CHK_CS_REQ]Error: Prefer peer Ant(%d) > Max ANT Path(%d)", COUNT_BIT_1(pConfigReq->Preferred_Peer_Ant), ACI_TO_MAX_PATH(pConfigReq->ACI));
        return HCI_ERR_INVALID_LMP_PARAMS;
    }

    if (COUNT_BIT_1(pConfigReq->Preferred_Peer_Ant) == 0) {
        CS_LL_LOG("[CHK_CS_REQ]Error: Prefer peer Ant(%d) invalid, maxAntPath(%d)", COUNT_BIT_1(pConfigReq->Preferred_Peer_Ant), ACI_TO_MAX_PATH(pConfigReq->ACI));
        return HCI_ERR_INVALID_LMP_PARAMS;
    }

    return BLE_SUCCESS;
}

/* (LL_CS_RSP) LL/CS/CEN/INI/BI-01-C and LL/CS/CEN/REF/BI-01-C expect ErrorCode set to 0x1E (Invalid LL Parameters). */
/* Offset_Min, Offset_Max, Offset_Max>Offset_Min, Subevent_Len */
/* connEventCount, Offset_Min - RSP < REQ */
ble_sts_t blt_ll_cs_checkCsrspParam(st_ll_conn_t *pAclConn, u8 opcode, u8 *raw, cs_config_t *pCsCfg)
{
    (void)opcode; //clean warning: unused variable 'opcode' [-Wunused-variable] by SunWei
    rf_packet_ll_cs_rsp_t *pConfigRsp = (rf_packet_ll_cs_rsp_t *)raw;

    u32 off_min = pConfigRsp->Offset_Min[0] | (pConfigRsp->Offset_Min[1] << 8) | (pConfigRsp->Offset_Min[2] << 16);
    u32 off_max = pConfigRsp->Offset_Max[0] | (pConfigRsp->Offset_Max[1] << 8) | (pConfigRsp->Offset_Max[2] << 16);
    if ((CS_EVENT_OFFSET_MIN > off_min) || (off_min >= CS_EVENT_OFFSET_MAX)) { //500<=offset_min <4000*1000 us
        CS_LL_LOG("[CHK_CS_RSP]offset abnormal:0x%x", off_min);
        return HCI_ERR_INVALID_LMP_PARAMS;
    }

    if ((off_max < off_min) || (off_max >= (pAclConn->conn_intvl_n_1m25 * 1250))) { //offset_min<= offset_max < le connection interval
        CS_LL_LOG("[CHK_CS_RSP]conn_interval abnormal:0x%x,0x%x,0x%x", off_min, off_max, pAclConn->conn_intvl_n_1m25);
        return HCI_ERR_INVALID_LMP_PARAMS;
    }

    u32 subevent_len = (pConfigRsp->Subevent_Len[0] | (pConfigRsp->Subevent_Len[1] << 8) | (pConfigRsp->Subevent_Len[2] << 16));
    //subevent_len RSP > REQ //Requirement violated: Subevent_Len (LL_CS_RSP) <= Subevent_Len (LL_CS_REQ)
    if ((CS_SUBEVENT_LEN_MIN > subevent_len) || (subevent_len >= CS_SUBEVENT_LEN_MAX) || (subevent_len > pCsCfg->Subevent_Len)) { //   1250 <= subevent len < 4000*1000
        CS_LL_LOG("[CHK_CS_RSP]subevent_len abnormal:0x%x", subevent_len);
        return HCI_ERR_INVALID_LMP_PARAMS;
    }

    if (pConfigRsp->Subevents_Per_Event <= 1) { //subevent interval if(subevent_per_event >1){valid}else{ be set to 0}
        pConfigRsp->Subevent_Interval = 0;
    } else {
        //check only valid if pConfigRsp->Subevents_Per_Event > 1
        //subevent_interval RSP < REQ //Requirement violated: Subevent_Interval (LL_CS_RSP) >= Subevent_Interval (LL_CS_REQ)
        if (pConfigRsp->Subevent_Interval < pCsCfg->subEvtIntvl_625us) {
            CS_LL_LOG("[CHK_CS_RSP]subevent intvl abnormal:0x%x, 0x%x", pConfigRsp->Subevent_Interval, pCsCfg->subEvtIntvl_625us);
            return HCI_ERR_INVALID_LMP_PARAMS;
        }
    }

    //RSP < REQ //Requirement violated: connEventCount (LL_CS_RSP) >= connEventCount (LL_CS_REQ)
    if (pConfigRsp->connEventCount < pCsCfg->connEventCount) {
        CS_LL_LOG("[CHK_CS_RSP]conn evt abnormal:0x%x, 0x%x", pConfigRsp->connEventCount, pCsCfg->connEventCount);
        return HCI_ERR_INVALID_LMP_PARAMS;
    }

    //RSP < REQ //Requirement violated: Offset_Min (LL_CS_RSP) >= Offset_Min (LL_CS_REQ)
    if (off_min < pCsCfg->offset_min) {
        CS_LL_LOG("[CHK_CS_RSP]conn evt abnormal:0x%x, 0x%x", off_min, pCsCfg->offset_min);
        return HCI_ERR_INVALID_LMP_PARAMS;
    }

    if (pConfigRsp->Event_Interval == 0) {
        CS_LL_LOG("[CHK-CS_RSP]event intvl abnormal:0x%x", pConfigRsp->Event_Interval);
        return HCI_ERR_INVALID_LMP_PARAMS;
    }

    return BLE_SUCCESS;
}

ble_sts_t blt_ll_cs_checkCsindParam(st_ll_conn_t *pAclConn, u8 opcode, u8 *raw, cs_config_t *pCsCfg)
{
    (void)opcode; //clean warning: unused variable 'opcode' [-Wunused-variable] by SunWei
    (void)pAclConn;
    rf_packet_ll_cs_ind_t *pConfigInd = (rf_packet_ll_cs_ind_t *)raw;

    u32 offset = pConfigInd->Offset[0] | pConfigInd->Offset[1] << 8 | pConfigInd->Offset[2] << 16;

    if ((pCsCfg->offset_min > offset) || (offset > pCsCfg->offset_max)) {
        CS_LL_LOG("[CHK_CS_IND]offset abnormal:0x%x, min:0x%x, max:0x%x", offset, pCsCfg->offset_min, pCsCfg->offset_max);
        return HCI_ERR_INVALID_LMP_PARAMS;
    }

    if (pConfigInd->Event_Interval < 1) { //1<= event_interval <= 65535
        CS_LL_LOG("[CHK_CS_IND]Event_Interval abnormal:0x%x", pConfigInd->Event_Interval);
        return HCI_ERR_INVALID_LMP_PARAMS;
    }

    if (pConfigInd->Subevents_Per_Event < 1) { //subevent_per_event >=1
        CS_LL_LOG("[CHK_CS_IND]Per_Event abnormal:0x%x", pConfigInd->Subevents_Per_Event);
        return HCI_ERR_INVALID_LMP_PARAMS;
    }

    u32 subevent_len = pConfigInd->Subevent_Len[0] | (pConfigInd->Subevent_Len[1] << 8) | (pConfigInd->Subevent_Len[2] << 16);
    //subevent_len IND > RSP //Requirement violated: Subevent_Len (LL_CS_IND) <= Subevent_Len (LL_CS_RSP)
    if (subevent_len > pCsCfg->Subevent_Len) {
        CS_LL_LOG("[CHK_CS_IND]subevent len abnormal:0x%x, 0x%x", pConfigInd->Subevent_Interval, pCsCfg->subEvtIntvl_625us);
        return HCI_ERR_INVALID_LMP_PARAMS;
    }

    if (pConfigInd->Subevents_Per_Event <= 1) { //subevent interval if(subevent_per_event >1){valid}else{ be set to 0}
        pConfigInd->Subevent_Interval = 0;
    } else {
        //check only valid if pConfigInd->Subevents_Per_Event > 1
        //IND < RSP //Requirement violated: Subevent_Interval (LL_CS_IND) >= Subevent_Interval (LL_CS_RSP)
        if (pConfigInd->Subevent_Interval < pCsCfg->subEvtIntvl_625us) {
            CS_LL_LOG("[CHK_CS_IND]subevent intvl abnormal:0x%x, 0x%x", pConfigInd->Subevent_Interval, pCsCfg->subEvtIntvl_625us);
            return HCI_ERR_INVALID_LMP_PARAMS;
        }
    }

    //IND < RSP //Requirement violated: connEventCount (LL_CS_IND) >= connEventCount (LL_CS_RSP)
    if (pConfigInd->connEventCount < pCsCfg->connEventCount) {
        CS_LL_LOG("[CHK_CS_IND]conn evt abnormal:0x%x, 0x%x", pConfigInd->connEventCount, pCsCfg->connEventCount);
        return HCI_ERR_INVALID_LMP_PARAMS;
    }
    // CS SYNC PHY is not relevant with ACL PHY
#if(0)
    if (pConfigInd->PHY != pCsCfg->CS_SYNC_PHY) {
        CS_LL_LOG("[CHK_CS_IND]PHY abnormal:0x%x,0x%x", pConfigInd->PHY, pCsCfg->CS_SYNC_PHY);
        return HCI_ERR_INVALID_LMP_PARAMS;
    }
#endif
    if (pConfigInd->ACI & 0xf0) { //0-7
        CS_LL_LOG("[CHK_CS_IND]ACI abnormal:0x%x", pConfigInd->ACI);
        return HCI_ERR_INVALID_LMP_PARAMS;
    }
    return BLE_SUCCESS;
}

int blt_ll_cs_ctrl_pdu_proc(st_ll_conn_t *pAclConn, u8 opcode, u8 *raw)
{
    cs_param_t *pCsParam = &pAclConn->csParam;
    #if OS_SUP_EN
    if (blt_os_giveSem_cb) {
        blt_os_giveSem_cb();
    }
    #endif
    if (opcode == LL_REJECT_IND_EXT) {
        rf_packet_ll_reject_ext_ind_t *pRejectExtInd = (rf_packet_ll_reject_ext_ind_t *)raw;
        CS_LL_LOG("[REJECT][RX]opcode:0x%x,errCode:0x%x", pRejectExtInd->rejectOpcode, pRejectExtInd->errCode);
        if ((pRejectExtInd->rejectOpcode == LL_CS_SEC_REQ) || (pRejectExtInd->rejectOpcode == LL_CS_SEC_RSP)) {
            pCsParam->cs_security_enable = 0;
            CS_LL_LOG("[REJECT][RX]sec exch fail");
        } else if ((pRejectExtInd->rejectOpcode == LL_CS_CAPABILITIES_REQ) || (pRejectExtInd->rejectOpcode == LL_CS_CAPABILITIES_RSP)) {
            pCsParam->cs_cap_req = 0;
            CS_LL_LOG("[REJECT][RX]cap exch fail");
        } else if ((pRejectExtInd->rejectOpcode == LL_CS_CONFIG_REQ) || (pRejectExtInd->rejectOpcode == LL_CS_CONFIG_RSP)) {
            pCsParam->cs_config_req = 0;
    //pCsCfg->state = 0; todo
    // config exchange error , send cs_config_complete_evt
    #if (LL_CS_CEN_INI_BI_04_C)
            csFlowCtrl.csConfigExchErr = 1;
            pCsParam->cs_config_req    = PROC_CS_CONFIG_EVT_PENDING;
    #endif
            CS_LL_LOG("[REJECT][RX]cfg exch fail");
        } else if ((pRejectExtInd->rejectOpcode == LL_CS_REQ) || (pRejectExtInd->rejectOpcode == LL_CS_RSP) || (pRejectExtInd->rejectOpcode == LL_CS_IND)) {
    #if (LL_CS_CEN_INI_BV_20_C)
            /* ebq case LL/CS/CEN/INI/BV-20-C,if cs start proc was reject,cs proc enable complete evt
                should be report */
            pCsParam->cs_req      = PROC_CS_EVT_PENDING;
            csFlowCtrl.csStartErr = 1;
    #else
            pCsParam->cs_req = 0;
    #endif
            CS_LL_LOG("[REJECT][RX]cs start exch fail");
        } else if ((pRejectExtInd->rejectOpcode == LL_CS_FAE_REQ) || (pRejectExtInd->rejectOpcode == LL_CS_FAE_RSP)) {
            pCsParam->cs_fae_req = 0;
            CS_LL_LOG("[REJECT][RX]fae exch fail");
        } else if (pRejectExtInd->rejectOpcode == LL_CS_TERMINATE_REQ) {
            pCsParam->cs_terminate_ind = 0;
            CS_LL_LOG("[REJECT][RX]terminate fail");
        }

    } else if (opcode == LL_CS_SEC_REQ) {
        rf_pkt_ll_cs_sec_req_t *pSecReq = (rf_pkt_ll_cs_sec_req_t *)raw;

        //CS_LL_LOG("[RECV_SEC_REQ]:%s",hex_to_str(raw,sizeof(rf_pkt_ll_cs_sec_req_t)));
        blt_ll_debug_print_security(raw, "[RECV_SEC_REQ]");
        /*
         * If the remote Link Layer sends an LL_CS_SEC_REQ PDU
         * when the Channel Sounding (Host Support) feature bit is not set in the local Link Layer, the local Link
         * Layer shall send an LL_REJECT_EXT_IND PDU with the error code Unsupported Remote Feature /
         * Unsupported LMP Feature (0x1A)
         * */
        if (!(LL_FEATURE_MASK_1 & LL_FEATURE_MASK_CHANNEL_SOUNDING_HOST)) {
            CS_LL_LOG("[SEC] host not support");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_SEC_REQ, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }
        /*
         *  If the remote Link Layer sends an
         *  LL_CS_SEC_REQ PDU without the Encryption Start procedure having successfully completed, the local
         *  Link Layer shall send an LL_REJECT_EXT_IND PDU with the error code Insufficient Security (0x2F).
         */
        /* HCI/CCO/BI-82-C */
        if (0 == pAclConn->crypt.enable) {
            CS_LL_LOG("[SEC REQ] enc not complete");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_SEC_REQ, HCI_ERR_INSUFFICIENT_SECURITY, 1);
        }

        smemcpy(&pCsParam->CS_IV_C[0], &pSecReq->CS_IV_C[0], 8);
        smemcpy(&pCsParam->CS_IN_C[0], &pSecReq->CS_IN_C[0], 4);
        smemcpy(&pCsParam->CS_PV_C[0], &pSecReq->CS_PV_C[0], 8);

        pCsParam->cs_security_enable |= (PROC_CS_SEC_SEND_RSP | PROC_CS_SEC_EVT_PENDING);


    } else if (opcode == LL_CS_SEC_RSP) {
        rf_pkt_ll_cs_sec_rsp_t *pSecRsp = (rf_pkt_ll_cs_sec_rsp_t *)raw;
        //CS_LL_LOG("[RECV_SEC_RSP]:%s",hex_to_str(raw,sizeof(rf_pkt_ll_cs_sec_rsp_t)));
        blt_ll_debug_print_security(raw, "[RECV_SEC_RSP]");

        if (0 == pAclConn->crypt.enable) {
            CS_LL_LOG("[SEC RSP] enc not complete");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_SEC_RSP, HCI_ERR_INSUFFICIENT_SECURITY, 1);
        }

        if (!(pCsParam->cs_security_enable & PROC_CS_SEC_WAIT_RSP)) {
            pCsParam->cs_security_enable = 0;
            CS_LL_LOG("[SEC] proc abnormal:0x%x", pCsParam->cs_security_enable);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_FAE_RSP, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }

        smemcpy(&pCsParam->CS_IV_P[0], &pSecRsp->CS_IV_P[0], 8);
        smemcpy(&pCsParam->CS_IN_P[0], &pSecRsp->CS_IN_P[0], 4);
        smemcpy(&pCsParam->CS_PV_P[0], &pSecRsp->CS_PV_P[0], 8);

        blt_ll_cs_buffCombize(&pCsParam->CS_IV_C[0], &pCsParam->CS_IV_P[0], &pCsParam->CS_IV[0], 8);
        blt_ll_cs_buffCombize(&pCsParam->CS_IN_C[0], &pCsParam->CS_IN_P[0], &pCsParam->CS_IN[0], 4);
        blt_ll_cs_buffCombize(&pCsParam->CS_PV_C[0], &pCsParam->CS_PV_P[0], &pCsParam->CS_PV[0], 8);

        if (pCsParam->cs_security_enable & PROC_CS_SEC_EVT_PENDING) {
            hci_le_csSecurityEnableComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle);
        }
    #if (LL_CS_CEN_REF_BV_01_C)
        pCsParam->cs_security_exchange = 0;
    #else
        pCsParam->cs_security_exchange = 1;
    #endif
        pCsParam->cs_security_enable  = 0;
        pAclConn->ll_rsp_timeout_tick = 0;
        drbg                          = (drbg_param_t *)&pCsParam->drbg_data[0];
        drbg_instantiation_func_h9(pCsParam->CS_IV, pCsParam->CS_IN, pCsParam->CS_PV, &drbg->kdrbg[0], &drbg->vdrbg[0]); ///////////
        cs_drbg_init();
    } else if (opcode == LL_CS_CAPABILITIES_REQ) {
        rf_packet_ll_cs_cap_req_t *pReq = (rf_packet_ll_cs_cap_req_t *)raw;
        chn_sound_capabilities_t  *pCap = &pAclConn->csRemoteSupCap;

        //tlkapi_send_string_data(DBG_CS_LL_LOG_MASK_EN,"[CAP][RX][RECV_CAP_REQ]",(u8*)raw, sizeof(rf_packet_ll_cs_cap_req_t));

        if (!(LL_FEATURE_MASK_1 & LL_FEATURE_MASK_CHANNEL_SOUNDING_HOST)) {
            CS_LL_LOG("[CAP] host not support");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_CAPABILITIES_REQ, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }

        if (0 == pAclConn->crypt.enable) {
            CS_LL_LOG("[CAP REQ] enc not complete");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_CAPABILITIES_REQ, HCI_ERR_INSUFFICIENT_SECURITY, 1);
        }

        pCap->Num_Config_Supported                 = pReq->Num_Configs;
        pCap->max_consecutive_procedures_supported = pReq->Max_Procedures_Supported;
        pCap->Num_Antennas_Supported               = pReq->Num_Ant;
        pCap->Max_Antenna_Paths_Supported          = pReq->Max_Ant_Path;
        pCap->Roles_Supported                      = pReq->Role;
        pCap->Mode_Types                           = pReq->Mode_Types;
        pCap->RTT_Capability                       = pReq->RTT_Capability;
        pCap->RTT_AA_Only_N                        = pReq->RTT_AA_Only_N;
        pCap->RTT_Sounding_N                       = pReq->RTT_Sounding_N;
        pCap->RTT_Random_Payload_N                 = pReq->RTT_Random_Sequence_N;
        pCap->Optional_NADM_Sounding_Capability    = pReq->NADM_Sounding_Capability;
        pCap->Optional_NADM_Random_Capability      = pReq->NADM_Random_Sequence_Capability;
        pCap->Optional_CS_SYNC_PHYs_Supported      = pReq->CS_SYNC_PHY_Capability;
        pCap->Optional_Subfeatures_Supported       = (pReq->Companion_Signal) | (pReq->No_FAE << 1) | (pReq->chn_sel_3c << 2) | (pReq->Sounding_PCT_Estimate << 3);
        pCap->Optional_T_IP1_Times_Supported       = pReq->T_IP1_Capability;
        pCap->Optional_T_IP2_Times_Supported       = pReq->T_IP2_Capability;
        pCap->Optional_T_FCS_Times_Supported       = pReq->T_FCS_Capability;
        pCap->Optional_T_FCS_Times_Supported       = pReq->T_FCS_Capability;
        pCap->Optional_T_PM_Times_Supported        = pReq->T_PM_Capability;
        pCap->T_SW_Time_Supported                  = pReq->T_SW;
        pCap->Optional_TX_SNR_Capability           = pReq->SNR;

        pCsParam->cs_cap_req |= (PROC_CS_CAP_SEND_RSP | PROC_CS_CAP_EVT_PENDING);

        //Print remote capabilities
        blt_ll_debug_print_capabilities(pCap, "[RECV_CAP_REQ]");

    } else if (opcode == LL_CS_CAPABILITIES_RSP) {
        if (0 == pAclConn->crypt.enable) {
            CS_HCI_LOG("[CAP RSP] enc not complete");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_CAPABILITIES_RSP, HCI_ERR_INSUFFICIENT_SECURITY, 1);
        }

        if (!(pCsParam->cs_cap_req & PROC_CS_CAP_WAIT_RSP)) {
            pCsParam->cs_cap_req = 0;
            CS_LL_LOG("[CAP] proc abnormal:0x%x", pCsParam->cs_cap_req);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_CAPABILITIES_RSP, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }

        rf_packet_ll_cs_cap_req_t *pReq = (rf_packet_ll_cs_cap_req_t *)raw;
        chn_sound_capabilities_t  *pCap = &pAclConn->csRemoteSupCap;

        //tlkapi_send_string_data(DBG_CS_LL_LOG_MASK_EN,"[CS][LL][RECV_CAP_RSP]",(u8*)raw,sizeof(rf_packet_ll_cs_cap_req_t));

        pCap->Num_Config_Supported                 = pReq->Num_Configs;
        pCap->max_consecutive_procedures_supported = pReq->Max_Procedures_Supported;
        pCap->Num_Antennas_Supported               = pReq->Num_Ant;
        pCap->Max_Antenna_Paths_Supported          = pReq->Max_Ant_Path;
        pCap->Roles_Supported                      = pReq->Role;
        pCap->Mode_Types                           = pReq->Mode_Types;
        pCap->RTT_Capability                       = pReq->RTT_Capability;
        pCap->RTT_AA_Only_N                        = pReq->RTT_AA_Only_N;
        pCap->RTT_Sounding_N                       = pReq->RTT_Sounding_N;
        pCap->RTT_Random_Payload_N                 = pReq->RTT_Random_Sequence_N;
        pCap->Optional_NADM_Sounding_Capability    = pReq->NADM_Sounding_Capability;
        pCap->Optional_NADM_Random_Capability      = pReq->NADM_Random_Sequence_Capability;
        pCap->Optional_CS_SYNC_PHYs_Supported      = pReq->CS_SYNC_PHY_Capability;
        pCap->Optional_Subfeatures_Supported       = (pReq->Companion_Signal) | (pReq->No_FAE << 1) | (pReq->chn_sel_3c << 2) | (pReq->Sounding_PCT_Estimate << 3);
        pCap->Optional_T_IP1_Times_Supported       = pReq->T_IP1_Capability;
        pCap->Optional_T_IP2_Times_Supported       = pReq->T_IP2_Capability;
        pCap->Optional_T_FCS_Times_Supported       = pReq->T_FCS_Capability;
        pCap->Optional_T_PM_Times_Supported        = pReq->T_PM_Capability;
        pCap->T_SW_Time_Supported                  = pReq->T_SW;
        pCap->Optional_TX_SNR_Capability           = pReq->SNR;

        //Print remote capabilities
        blt_ll_debug_print_capabilities(pCap, "[RECV_CAP_RSP]");

        /* validation of LL/CS/CEN/BI-02-C and LL/CS/PER/BI-02-C */
        u8 ret = blt_ll_cs_checkCapabilitiesParam(pAclConn, opcode, raw);

        if (pCsParam->cs_cap_req & PROC_CS_CAP_EVT_PENDING) {
            if (ret != BLE_SUCCESS) {
                hci_le_readRemoteSupCapComplete_evt(HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE, pAclConn->acl_conHandle, (u8 *)pCap);
            } else {
                hci_le_readRemoteSupCapComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8 *)pCap);
            }
        }
        pCsParam->cs_cap_exchange     = 1;
        pAclConn->ll_rsp_timeout_tick = 0;
        pCsParam->cs_cap_req          = 0;
    } else if (opcode == LL_CS_CONFIG_REQ) {
        rf_packet_ll_cs_config_req_t *pConfigReq = (rf_packet_ll_cs_config_req_t *)raw;

        //tlkapi_send_string_data(DBG_CS_LL_LOG_MASK_EN,"[CS][LL][RECV_CONFIG_REQ]",(u8*)raw,sizeof(rf_packet_ll_cs_config_req_t));

        // Run #3418 - /Pre-Release/LL/CS/CEN/INI/BV-29-C   [Channel Sounding Configuration Procedure Collision, Central, Initiator]
        // Run #3422 - /Pre-Release/LL/CS/CEN/REF/BV-29-C   [Channel Sounding Configuration Procedure Collision, Central, Reflector]
        if ((pAclConn->aclRole == ACL_ROLE_CENTRAL) && (pCsParam->cs_config_req & PROC_CS_CONFIG_WAIT_RSP)) {
            CS_LL_LOG("[CFG][RX] config procedure collision,should receive config rsp");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_CONFIG_REQ, HCI_ERR_LMP_ERR_TRANSACTION_COLLISION, 1);
        }

        /*
         * 1. If the remote Link Layer sends an LL_CS_CONFIG_REQ PDU when the Channel Sounding
         * (Host Support) feature bit is not set in the local Link Layer, then the local Link
         * Layer shall send an LL_REJECT_EXT_IND PDU
         */
        if (!(LL_FEATURE_MASK_1 & LL_FEATURE_MASK_CHANNEL_SOUNDING_HOST)) {
            CS_LL_LOG("[CFG] host not support");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_CONFIG_REQ, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }

        if (0 == pAclConn->crypt.enable) {
            CS_LL_LOG("[CONFIG REQ] enc not complete");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_CONFIG_REQ, HCI_ERR_INSUFFICIENT_SECURITY, 1);
        }

        /*      1. save configReq
         *      2. If the parameters received in an LL_CS_CONFIG_REQ PDU are not acceptable to that Link Layer,
         *      then it shall immediately reject the configuration parameter set with an LL_REJECT_EXT_IND PDU
         *      with the error code Unsupported LL Parameter Value (0x20)
         */
        /* LL/CS/CEN/INI/BI-03-C, LL/CS/CEN/REF/BI-04-C, LL/CS/PER/INI/BI-04-C, LL/CS/PER/REF/BI-05-C */
        /* ConfigID, MainMode, SubMode, CS_SYNC_PHY, Role, ChMRep, MainModeRep, Mode0Steps, Ch3cJump, Ch3cShape, Companion_Signal, ChSel, RTT_Type checks */
        /* LL/CS/PER/INI/BI-06-C, LL/CS/CEN/INI/BI-05-C, LL/CS/PER/REF/BI-07-C, LL/CS/CEN/REF/BI-06-C */
        /* T_IP1, T_IP2, T_FCS, Main_Mode, Sub_Mode, RTT_Types, ChSel within capabilities */
        u8 ret = blt_ll_cs_checkConfigParam(pAclConn, opcode, raw);
        if (ret != BLE_SUCCESS) {
            CS_LL_LOG("[CFG] param check fail:0x%x", ret);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_CONFIG_REQ, ret, 1);
        }

        u8 cfgIdx = blt_ll_getCsConfigById(pAclConn->acl_conHandle, pConfigReq->Config_ID);
        if (cfgIdx == 0xff) {
            cfgIdx = blt_ll_getNewCsConfig();
            if (cfgIdx == 0xff) {
                CS_LL_LOG("[CFG] create cfg fail:0x%x", cfgIdx);
                return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_CONFIG_REQ, HCI_ERR_CONN_REJ_LIMITED_RESOURCES, 1);
            }
            CS_LL_LOG("[CFG] create cfg id:0x%x", cfgIdx);
        } else {
            CS_LL_LOG("[CFG] cfg id is exist:0x%x", cfgIdx);
        }

        cs_config_t *pCsCfg = gCsMng.gGlobal_pCsCfg + cfgIdx;
        /* LL/CS/CEN/INI/BI-03-C, LL/CS/CEN/REF/BI-04-C, LL/CS/PER/INI/BI-04-C, LL/CS/PER/REF/BI-05-C */
        /* ChM checks, we ignore invalid channels. If the count after excluding invalid ch. is < 15 - reject */
        ret = blt_cs_extractEnableChnMap(pConfigReq->ChM, pCsCfg->filteredChnArray, &pCsCfg->Chn_en_num);
        if ((ret != BLE_SUCCESS) || (pCsCfg->Chn_en_num < 15)) {
            CS_LL_LOG("[CFG] chnNum eroro:0x%x, 0x%x", ret, pCsCfg->Chn_en_num);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_CONFIG_REQ, HCI_ERR_UNSUPPORTED_LMP_PARAM_VAL, 1);
        }

        pCsCfg->Config_ID = pConfigReq->Config_ID;
        pCsCfg->state     = pConfigReq->State;
        smemcpy(pCsCfg->Channel_Map, pConfigReq->ChM, 10);
        smemcpy(pCsCfg->Origin_Chn_Map, pConfigReq->ChM, 10);
        pCsCfg->Channel_Map_Repetition  = pConfigReq->ChM_Repetition;
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

        pCsCfg->occupy = pCsCfg->state;
        //      pCsCfg->idx                     = cfgIdx;
        pCsCfg->aclHandle   = pAclConn->acl_conHandle;
        pAclConn->csMapMask = BIT(pCsCfg->idx);

        pCsCfg->T_IP1_Us       = T_IP_US[pCsCfg->T_IP1];
        pCsCfg->T_IP2_Us       = T_IP_US[pCsCfg->T_IP2];
        pCsCfg->T_FCS_Us       = T_FCS_US[pCsCfg->T_FCS];
        pCsCfg->T_PM_Us        = T_PM_US[pCsCfg->T_PM];
        pCsCfg->T_SW_Us        = 0; //will change in CS start procedure
        pCsCfg->antennaPathNum = 1; //will change in CS start procedure

        pCsParam->cs_config_req |= (PROC_CS_CONFIG_SEND_RSP | PROC_CS_CONFIG_EVT_PENDING);
        pCsParam->cs_config_pend_idx = pCsCfg->idx;

        pCsCfg->PHY = pCsCfg->CS_SYNC_PHY; //todo by biao & qinghua

        blt_ll_debug_print_config_reqeust(pConfigReq, "[RECV_CONFIG_REQ]");
    #if (LL_CS_CEN_INI_BV_05_C)
        /* CS EBQ case LL/CS/CEN/INI/BV-05-C, test send repeat config.
         * if without this part, first cs LL procedure is ok,but since second config, LL procedure will be wrong,
         * because the flag like ll_cs_senc is not clean, and the second round ll_cs_senc_req will not be sent.
         * It's a quick fix for this case,but further,once a new ll_cs_config_req is receive,all ll cs flag
         * should be cleaned --yuexin 20240321 TODO
         */
        pCsParam->cs_security_exchange = 0;
    #endif
    } else if (opcode == LL_CS_CONFIG_RSP) {
        rf_packet_ll_cs_config_rsp_t *pConfigRsp = (rf_packet_ll_cs_config_rsp_t *)raw;
        cs_config_t                  *pCsCfg     = gCsMng.gGlobal_pCsCfg + pAclConn->csParam.cs_config_pend_idx;

        tlkapi_send_string_data(DBG_CS_LL_LOG_MASK_EN, "[CS][LL][RECV_CONFIG_RSP]", (u8 *)raw, sizeof(rf_packet_ll_cs_config_rsp_t));

        if (0 == pAclConn->crypt.enable) {
            CS_LL_LOG("[CONFIG RSP] enc not complete");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_CONFIG_RSP, HCI_ERR_INSUFFICIENT_SECURITY, 1);
        }

        /*
         * 1. The value of the Config_ID parameter returned in the LL_CS_CONFIG_RSP PDU shall be the same as
         *    the value received in the LL_CS_CONFIG_REQ PDU
         */
        if (pCsCfg->Config_ID != pConfigRsp->Config_ID) {
            pCsCfg->state           = 0;
            pCsParam->cs_config_req = 0;
            CS_LL_LOG("[CFG] cfg id is not consist:0x%x", pConfigRsp->Config_ID);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_CONFIG_RSP, HCI_ERR_UNSUPPORTED_LMP_PARAM_VAL, 1);
        }

        if (!(pCsParam->cs_config_req & PROC_CS_CONFIG_WAIT_RSP)) {
            pCsCfg->state           = 0;
            pCsParam->cs_config_req = 0;
            CS_LL_LOG("[CFG] proc abnormal:0x%x", pCsParam->cs_config_req);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_CONFIG_RSP, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }


        if (pCsParam->cs_config_req & PROC_CS_CONFIG_EVT_PENDING) {
            hci_le_csConfigComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8 *)pCsCfg);
        }

        pCsParam->cs_config_req       = 0;
        pAclConn->ll_rsp_timeout_tick = 0;
        pCsCfg->occupy                = pCsCfg->state;
    } else if (opcode == LL_CS_REQ) {
        rf_packet_ll_cs_req_t *pCsReq = (rf_packet_ll_cs_req_t *)raw;

        //CS_LL_LOG("[RECV_CS_REQ]:%s",hex_to_str(raw, sizeof(rf_packet_ll_cs_req_t)));
        blt_ll_debug_print_cs_reqeust(pCsReq, "[RECV_CS_REQ]");

        if (0 == pAclConn->crypt.enable) {
            CS_LL_LOG("[CS REQ]ACL is Unencrypted");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_REQ, HCI_ERR_INSUFFICIENT_SECURITY, 1);
        }

    #if (LL_CS_CEN_INI_BI_04_C)
        if (csFlowCtrl.csConfigExchErr) {
            CS_EBQ_LOG("[fail] cs config exchange error,send ll reject");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_REQ, HCI_ERR_INVALID_LMP_PARAMS, 1);
        }
    #endif
        /* If the remote Link Layer sends an
        LL_CS_REQ PDU when the Channel Sounding (Host Support) feature bit is not set in the local Link
        Layer, the local Link Layer shall send an LL_REJECT_EXT_IND PDU with the error code Unsupported
        Remote Feature / Unsupported LMP Feature (0x1A).*/
        if (!(LL_FEATURE_MASK_1 & LL_FEATURE_MASK_CHANNEL_SOUNDING_HOST)) {
            CS_LL_LOG("[START] host not support");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_REQ, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }
        /* If the CS configuration ID received during the CS Start procedure does not exist or is
        otherwise not properly enabled, then the receiving Link Layer shall immediately respond with an
        LL_REJECT_EXT_IND PDU with the error code Invalid LL Parameters (0x1E).*/

        u8 cfgIdx = blt_ll_getCsConfigById(pAclConn->acl_conHandle, pCsReq->Config_ID);
        if (cfgIdx == 0xff) {
            CS_LL_LOG("[START] config not exist:0x%x,0x%x", pAclConn->acl_conHandle, pCsReq->Config_ID);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_REQ, HCI_ERR_CONN_REJ_LIMITED_RESOURCES, 1);
        }
        cs_config_t *pCsCfg = gCsMng.gGlobal_pCsCfg + cfgIdx;
        if (pCsCfg->state == 0) { //todo  not properly enabled
            CS_LL_LOG("[START] config not enable");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_REQ, HCI_ERR_CONN_REJ_LIMITED_RESOURCES, 1);
        }

        /* If the parameters received in an LL_CS_REQ PDU are not acceptable to the receiving Link Layer
         (Central or Peripheral), then it shall immediately reject the procedure sending a LL_REJECT_EXT_IND
         PDU with the appropriate error code.*/
        /* (LL_CS_REQ) LL/CS/CEN/INI/BI-06-C, LL/CS/CEN/REF/BI-07-C, LL/CS/PER/INI/BI-07-C, LL/CS/PER/REF/BI-08-C */
        /* Offset_Min, Offset_Max, Offset_Max>Offset_Min, Event_interval, Subevents_Per_Event, Subevent_Len */
        ble_sts_t ret = blt_ll_cs_checkCsreqParam(pAclConn, opcode, raw, pCsCfg);
        if (ret) {
            CS_LL_LOG("[START] req param check fail:0x%x", ret);
            csFlowCtrl.csStartErr = 1;
            pCsParam->cs_req      = PROC_CS_EVT_PENDING;
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_REQ, ret, 1);
        }


        u8 aci     = pCsReq->ACI;
        u8 antA    = 1; //initiator
        u8 antB    = 1; //reflector
        u8 antPath = 1;
        u8 check   = 0;
        if (aci == 7) {
            antA    = 2;
            antB    = 2;
            antPath = 4;
        } else if (aci > 3) {
            antA    = 1;
            antB    = aci - 2;
            antPath = antB;
        } else if (aci > 0) {
            antA    = aci + 1;
            antB    = 1;
            antPath = antA;
        } else if (aci == 0) {
            antA    = 1;
            antB    = 1;
            antPath = antA;
        } else {
            check = 1;
        }

        if (check == 0) {
            if (pCsCfg->Role == CHANNEL_SOUNDING_ROLE_INITIATOR) {
                if ((antB > pAclConn->csRemoteSupCap.Num_Antennas_Supported) || (antA > bltCsLocalSupportCap.Num_Antennas_Supported) || (antPath > bltCsLocalSupportCap.Max_Antenna_Paths_Supported) || (antPath > pAclConn->csRemoteSupCap.Max_Antenna_Paths_Supported)) {
                    check = 2;
                }
            } else {
                if ((antA > pAclConn->csRemoteSupCap.Num_Antennas_Supported) || (antB > bltCsLocalSupportCap.Num_Antennas_Supported) || (antPath > bltCsLocalSupportCap.Max_Antenna_Paths_Supported) || (antPath > pAclConn->csRemoteSupCap.Max_Antenna_Paths_Supported)) {
                    check = 3;
                }
            }
        }


        if (check == 0) {
            pCsCfg->aci = pCsReq->ACI;
        } else {
            u8 min_ant          = (bltCsLocalSupportCap.Num_Antennas_Supported > pAclConn->csRemoteSupCap.Num_Antennas_Supported) ? pAclConn->csRemoteSupCap.Num_Antennas_Supported : bltCsLocalSupportCap.Num_Antennas_Supported;
            u8 min_max_ant_path = (bltCsLocalSupportCap.Max_Antenna_Paths_Supported > pAclConn->csRemoteSupCap.Max_Antenna_Paths_Supported) ? pAclConn->csRemoteSupCap.Max_Antenna_Paths_Supported : bltCsLocalSupportCap.Max_Antenna_Paths_Supported;
            aci                 = 0;
            if ((min_max_ant_path == 4) && (min_ant >= 2)) {
                aci = 7; //2:2
            } else {
                u8 min_ant_path1 = (min_max_ant_path > bltCsLocalSupportCap.Num_Antennas_Supported) ? bltCsLocalSupportCap.Num_Antennas_Supported : min_max_ant_path;
                u8 min_ant_path2 = (min_max_ant_path > pAclConn->csRemoteSupCap.Num_Antennas_Supported) ? pAclConn->csRemoteSupCap.Num_Antennas_Supported : min_max_ant_path;
                if (pCsCfg->Role == CHANNEL_SOUNDING_ROLE_INITIATOR) {
                    if (bltCsLocalSupportCap.Num_Antennas_Supported < pAclConn->csRemoteSupCap.Num_Antennas_Supported) {
                        aci = min_ant_path1 + 2; // 1 : min_ant_path1
                    } else {
                        aci = min_ant_path2 - 1; // min_ant_path2 : 1
                    }
                } else if (pCsCfg->Role == CHANNEL_SOUNDING_ROLE_REFLECTOR) {
                    if (bltCsLocalSupportCap.Num_Antennas_Supported <= pAclConn->csRemoteSupCap.Num_Antennas_Supported) {
                        aci = min_ant_path2 - 1; // min_ant_path2 : 1
                    } else {
                        aci = min_ant_path1 + 2; // 1 : min_ant_path1
                    }
                }
            }

            // [PBLE-169]Broadcom 20240130 session15:if no cs cap exchange before,csRemote ant and ant path will equal = 0,
            // we should consider this situation if no cap exchange , if we get LL_CS_REQ, just set the aci = req->aci. -- yuexin 20240525
            if (pAclConn->csRemoteSupCap.Num_Antennas_Supported == 0 && pAclConn->csRemoteSupCap.Max_Antenna_Paths_Supported == 0) {
                CS_LL_LOG("[CS REQ/RSP] No cs cap exchange before");
                aci = pCsReq->ACI;
            }

            pCsCfg->aci = aci;
            CS_LL_LOG("[START] ACI:0x%x,check:0x%x,0x%x,0x%x,0x%x", pCsCfg->aci, check);
        }

        if (pAclConn->aclRole == ACL_ROLE_CENTRAL) {
            pCsParam->cs_req = (PROC_CS_SEND_IND | PROC_CS_EVT_PENDING);
            /* Test with EBQ case calculate LL/CS/CEN/INI/BV-18-C, LL/CS/CEN/INI/BV-19-C, LL/CS/CEN/INI/BV-31-C
             * when we are initiator, reflector send ll_cs_req, we should proc req data
             */
    #if (LL_CS_CEN_INI_BV_18_C)
            pCsCfg->connEventCount     = pCsReq->connEventCount;
            pCsCfg->Max_Procedure_Len  = pCsReq->Max_Procedure_Len;
            pCsCfg->Event_Interval     = pCsReq->Event_Interval;
            pCsCfg->Procedure_Interval = pCsReq->Procedure_Interval;
            pCsCfg->procMaxCount += pCsReq->Procedure_Count;
            pCsCfg->procMaxCountInstant = pCsReq->Procedure_Count;
            pCsCfg->Subevents_Per_Event = pCsReq->Subevents_Per_Event;
            u32 req_subevent_len        = pCsReq->Subevent_Len[0] | (pCsReq->Subevent_Len[1] << 8) | (pCsReq->Subevent_Len[2] << 16);
            pCsCfg->Subevent_Len        = min3(req_subevent_len, TLK_CS_SUBEVENT_MAX_LEN, pAclConn->conn_intvl_n_1m25 * 1250 / 3 * 2);
            //make sure we are within global criteria - we should always be
            pCsCfg->Subevent_Len = min(pCsCfg->Subevent_Len, CS_SUBEVENT_LEN_MAX);
            pCsCfg->Subevent_Len = max(pCsCfg->Subevent_Len, CS_SUBEVENT_LEN_MIN);
            /*
             * 1. The Subevent_Interval shall be greater than or equal to the sum of the Subevent_Len selected plus
             *    T_MES. A Controller shall be capable of supporting a minimum Subevent_Len of 2.5 ms.
             * 2. The value of Subevent_Interval supplied in either LL_CS_RSP PDU or the LL_CS_IND_PDU shall be greater than or
             *    equal to the value received in the LL_CS_REQ or LL_CS_RSP PDU that is being responded to
             */

            if (pCsCfg->Subevents_Per_Event == 1) {
                pCsCfg->subEvtIntvl_625us = 0;
            } else {
                pCsCfg->subEvtIntvl_625us = max(pCsReq->Subevent_Interval, (pCsCfg->Subevent_Len + TLK_T_MES) / 625 + 5);
            }

            /* calculate cs procedure offset */
            u32 ofst_min = max2(CS_SUBEVT_OFFSET, (pAclConn->sSlot_duration * SSLOT_US_NUM + pCsCfg->sch_early_us));
            u32 ofst_max = pAclConn->conn_intvl_n_1m25 * 1250 - pCsCfg->Subevent_Len - pCsCfg->sch_early_us;

            u32 csOft_req = pCsReq->Offset_Min[0] | (pCsReq->Offset_Min[1] << 8) | (pCsReq->Offset_Min[2] << 16);
            if (csOft_req < ofst_min) {
                pCsCfg->csOft_us = ofst_min;
            } else if (csOft_req > ofst_max) {
                pCsCfg->csOft_us = ofst_max;
            } else {
                pCsCfg->csOft_us = csOft_req;
            }

            pCsCfg->PHY = pCsReq->PHY;
    #endif
        } else {
            pCsParam->cs_req = (PROC_CS_SEND_RSP | PROC_CS_EVT_PENDING);

            pCsCfg->connEventCount     = pCsReq->connEventCount;
            pCsCfg->Max_Procedure_Len  = pCsReq->Max_Procedure_Len;
            pCsCfg->Event_Interval     = pCsReq->Event_Interval;
            pCsCfg->Procedure_Interval = pCsReq->Procedure_Interval;
            // if it's the first start procedure instance or the last started procedure count is 0, peripheral side should clean the csProcCount
            if (!pCsCfg->procMaxCountInstant) {
                pCsCfg->csProcCount  = 0;
                pCsCfg->procMaxCount = 0;
                CS_LL_LOG("Clean cs procedure count and max procedure count on peripheral side");
            }
            pCsCfg->procMaxCount += pCsReq->Procedure_Count;
            pCsCfg->procMaxCountInstant = pCsReq->Procedure_Count;

    #if CS_SUBEVENT_LEN_EVALUATE
            blt_ll_calcStepDuration(pCsCfg);
            u32 subevent_len_t   = 0;
            u32 req_subevent_len = pCsReq->Subevent_Len[0] | (pCsReq->Subevent_Len[1] << 8) | (pCsReq->Subevent_Len[2] << 16);
            req_subevent_len     = min2(req_subevent_len, TLK_CS_SUBEVENT_MAX_LEN);
            u32 offset_max       = pCsReq->Offset_Max[0] | (pCsReq->Offset_Max[1] << 8) | (pCsReq->Offset_Max[2] << 16);
            u32 offset_min       = pCsReq->Offset_Min[0] | (pCsReq->Offset_Min[1] << 8) | (pCsReq->Offset_Min[2] << 16);
            tlkapi_send_string_u32s(0, "cs subevent len cal rsp", offset_min, offset_max, req_subevent_len, req_subevent_len);
            u32 max_sch_early_us = blt_ll_cs_subevent_schedule_early_cal(pCsCfg, req_subevent_len, &subevent_len_t, offset_max);

            max_sch_early_us = max(max_sch_early_us, TLK_T_MES);

            if ((subevent_len_t == req_subevent_len) && ((max_sch_early_us + pAclConn->sSlot_duration * SSLOT_US_NUM) <= offset_max)) {
                pCsCfg->Subevent_Len = req_subevent_len;
            } else {
                pCsCfg->Subevent_Len = subevent_len_t;
            }

            u16 subevent_interval = (pCsCfg->Subevent_Len + max_sch_early_us + CS_SUBEVNET_RESULT_REPORT_DURATION_US + (625 - 1)) / 625;

            pCsCfg->subEvtIntvl_625us = max(subevent_interval, pCsReq->Subevent_Interval);

            pCsCfg->offset_min   = max(max_sch_early_us + pAclConn->sSlot_duration * SSLOT_US_NUM, 500);
            pCsCfg->offset_min   = max(pCsCfg->offset_min, offset_min);
            pCsCfg->offset_min   = min(pCsCfg->offset_min, (pAclConn->conn_intvl_n_1m25 * 1250) - 1);
            pCsCfg->offset_min   = min(pCsCfg->offset_min, offset_max);
            pCsCfg->offset_max   = max(offset_max, offset_min);
            pCsCfg->offset_max   = min(pCsCfg->offset_max, (pAclConn->conn_intvl_n_1m25 * 1250) - 1);
            pCsCfg->sch_early_us = max_sch_early_us;

            u8 subevent_per_event = (pCsCfg->Event_Interval * (pAclConn->conn_intvl_n_1m25 * 1250) - pCsCfg->offset_max) / (pCsCfg->subEvtIntvl_625us * 625); //must be integer.

            pCsCfg->Subevents_Per_Event = min(subevent_per_event, 32);

            if (pCsCfg->Subevents_Per_Event <= 1) {
                pCsCfg->Subevents_Per_Event = 1;
                pCsCfg->subEvtIntvl_625us   = 0;
            }


            tlkapi_send_string_u32s(0, "cs subevent len cal", req_subevent_len, subevent_len_t, max_sch_early_us, pCsCfg->offset_min);

    #else
            /*** Decision - we use the proposed subevent count, we can only reduce this number ***/
            pCsCfg->Subevents_Per_Event = pCsReq->Subevents_Per_Event;
            // s16 subevent_cnt = (pAclConn->conn_intvl_n_1m25*1250/3 * 2)/(pCsCfg->subEvtIntvl_625us*625) -1; //relevant?
            // pCsCfg->Subevents_Per_Event = min(pCsReq->Subevents_Per_Event, subevent_cnt); //relevant?

            u32 req_subevent_len = pCsReq->Subevent_Len[0] | (pCsReq->Subevent_Len[1] << 8) | (pCsReq->Subevent_Len[2] << 16);

        #if SUBEVENTLEN_ALG
            u32 min_subevent_len = blt_ll_calcMinSubeventLen(pCsCfg);

            CS_HCI_LOG("[START] reqSubeventLen: 0x%x minSubeventLen: 0x%x", req_subevent_len, min_subevent_len);

            /*
                * 1. The value of Subevent_Len supplied in either the LL_CS_RSP or LL_CS_IND PDU shall be less than or equal to the
                * value received in the LL_CS_REQ or LL_CS_RSP PDU that is being responded to
                */
            /*** Assertion - if proposed requested subevent length < min_subevent_len - invalid parameters ***/
            /*** no subevent len valid, not possible to change other relevant parameters ***/
            if (req_subevent_len < min_subevent_len) {
                pCsCfg->cs_procedure_en = 0;
                pCsParam->cs_req        = 0;
                CS_HCI_LOG("[START] req subevent len too short:0x%x,req: 0x%x,min: 0x%x ", pAclConn->acl_conHandle, req_subevent_len, min_subevent_len);
                return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_REQ, HCI_ERR_INVALID_LMP_PARAMS, 1);
            }

            /*** Decision: (this is an algorithm we can decide on) ***/
            /*** If the Subevent Len in REQ is <= CS_SUBEVENT_LEN_THRESHOLD then we just use it ***/
            /*** If difference between requested and min subevent length <= 200, then just take ***/
            /*** the REQ value to avoid handling fractions ***/
            /*** Otherwise - we use the subevent len, that is in the middle of what gets proposed in REQ ***/
            /*** and what we calculate as being the minimum, rounded down to the nearest full 100 us ***/

            if ((req_subevent_len <= CS_SUBEVENT_LEN_THRESHOLD_LOW) || ((req_subevent_len - min_subevent_len) <= 200)) {
                pCsCfg->Subevent_Len = req_subevent_len;
            } else {
                pCsCfg->Subevent_Len = (((req_subevent_len + min_subevent_len) / 2) / 100) * 100;
            }

            /*** Limit the upper subevent length, as long as it doesn't violate min_subevent_len ***/
            if ((min_subevent_len <= CS_SUBEVENT_LEN_THRESHOLD_HIGH) && (pCsCfg->Subevent_Len > CS_SUBEVENT_LEN_THRESHOLD_HIGH)) {
                pCsCfg->Subevent_Len = CS_SUBEVENT_LEN_THRESHOLD_HIGH;
            }
        #else
            if (req_subevent_len > TLK_CS_SUBEVENT_MAX_LEN) { //3.8ms, case1:(m0=2, m2=6) case2:(m0=1, m2=7)
                req_subevent_len = TLK_CS_SUBEVENT_MAX_LEN;
            }

            pCsCfg->Subevent_Len = min2(req_subevent_len, pAclConn->conn_intvl_n_1m25 * 1250 / 3 * 2);
        #endif

            //subevent len in RSP needs to be <= than in REQ. Make sure it is so.
            pCsCfg->Subevent_Len = min2(pCsCfg->Subevent_Len, req_subevent_len);
        #if (DBG_CS_ONE_SUBEVENT_72CHN)
            pCsCfg->Subevent_Len = TLK_CS_SUBEVENT_MAX_LEN;
        #endif
            //make sure we are within global criteria - we should always be
            pCsCfg->Subevent_Len = min(pCsCfg->Subevent_Len, CS_SUBEVENT_LEN_MAX);
            pCsCfg->Subevent_Len = max(pCsCfg->Subevent_Len, CS_SUBEVENT_LEN_MIN);

            //pCsCfg->Subevent_Len = min2(subevent_len, pAclConn->conn_intvl_n_1m25*1250/3 * 2); //relevant?

            /*
             * 1. The Subevent_Interval shall be greater than or equal to the sum of the Subevent_Len selected plus
             *    T_MES. A Controller shall be capable of supporting a minimum Subevent_Len of 2.5 ms.
             * 2. The value of Subevent_Interval supplied in either LL_CS_RSP PDU or the LL_CS_IND_PDU shall be greater than or
             *    equal to the value received in the LL_CS_REQ or LL_CS_RSP PDU that is being responded to
             */
            pCsCfg->subEvtIntvl_625us = max(pCsReq->Subevent_Interval, (pCsCfg->Subevent_Len + TLK_T_MES) / 625 + 5);

            if (pCsCfg->Subevents_Per_Event == 1) {
                pCsCfg->subEvtIntvl_625us = 0;
            }

            /*
             * The Offset_Min value shall be greater than or equal to 500 us and less than 4 seconds.
             */
            u32 ofst_min = max2(CS_SUBEVT_OFFSET, (pAclConn->sSlot_duration * SSLOT_US_NUM + pCsCfg->sch_early_us));
            u32 ofst_max = pAclConn->conn_intvl_n_1m25 * 1250 - pCsCfg->Subevent_Len - pCsCfg->sch_early_us;

            pCsCfg->offset_min = max2(ofst_min, (u32)(pCsReq->Offset_Min[0] | (pCsReq->Offset_Min[1] << 8) | (pCsReq->Offset_Min[2] << 16)));
            pCsCfg->offset_max = min2(ofst_max, (u32)(pCsReq->Offset_Max[0] | (pCsReq->Offset_Max[1] << 8) | (pCsReq->Offset_Max[2] << 16)));

            if (pCsCfg->offset_max <= pCsCfg->offset_min) {
                pCsCfg->offset_max = pCsCfg->offset_min + 1250;
            } //todo by fqh 2023.12.12
    #endif
        }

        pCsCfg->cs_procedure_en = 1;
        pCsCfg->csRspProcRole   = 1;
    } else if (opcode == LL_CS_RSP) {
        rf_packet_ll_cs_rsp_t *pCsRsp = (rf_packet_ll_cs_rsp_t *)raw;
        cs_config_t           *pCsCfg = gCsMng.gGlobal_pCsCfg + pCsParam->cs_pend_idx;

        //CS_LL_LOG("[RECV_CS_RSP]:%s",hex_to_str(raw, sizeof(rf_packet_ll_cs_rsp_t)));
        blt_ll_debug_print_cs_response(pCsRsp, "[RECV_CS_RSP]");

        if (0 == pAclConn->crypt.enable) {
            CS_LL_LOG("[CS RSP]ACL is Unencrypted");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_RSP, HCI_ERR_INSUFFICIENT_SECURITY, 1);
        }

        if (!(pCsParam->cs_req & PROC_CS_WAIT_RSP)) {
            pCsCfg->cs_procedure_en = 0;
            pCsParam->cs_req        = 0;
            CS_LL_LOG("[START RSP] proc abnormal:0x%x", pCsParam->cs_req);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_RSP, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }

        u8 cfgIdx = blt_ll_getCsConfigById(pAclConn->acl_conHandle, pCsRsp->Config_ID);
        if (cfgIdx == 0xff) {
            pCsCfg->cs_procedure_en = 0;
            pCsParam->cs_req        = 0;
            CS_LL_LOG("[START RSP] config not exist:0x%x,0x%x", pAclConn->acl_conHandle, pCsRsp->Config_ID);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_RSP, HCI_ERR_CONN_REJ_LIMITED_RESOURCES, 1);
        }

        /* (LL_CS_RSP) LL/CS/CEN/INI/BI-01-C and LL/CS/CEN/REF/BI-01-C expect ErrorCode set to 0x1E (Invalid LL Parameters). */
        ble_sts_t ret = blt_ll_cs_checkCsrspParam(pAclConn, opcode, raw, pCsCfg);
        if (ret) {
            csFlowCtrl.csRspCheckErr = 1;
            pCsCfg->cs_procedure_en  = 0;
            pCsParam->cs_req         = 0;
            CS_LL_LOG("[START RSP] rsp param check fail:0x%x", ret);
            /* LL/CS/CEN/INI/BI-01-C if rsp para error, should send procedure enable complete evt
             * rather than indication packet -- yuexin
             */
    #if (LL_CS_CEN_INI_BI_01_C)
            pCsParam->cs_req |= PROC_CS_EVT_PENDING;
    #endif
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_RSP, HCI_ERR_INVALID_LMP_PARAMS, 1);
        }

        u8 aci = blt_ll_checkCsAci(pCsCfg->aci, pCsRsp->ACI); //local: initiator role
        if (aci == 0xff) {
            pCsCfg->cs_procedure_en = 0;
            pCsParam->cs_req        = 0;
            CS_LL_LOG("[START] aci invalid:0x%x,0x%x,0x%x", pAclConn->acl_conHandle, pCsCfg->aci, pCsRsp->ACI);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_RSP, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        } else {
            pCsCfg->aci = pCsRsp->ACI;
        }
    #if CS_SUBEVENT_LEN_EVALUATE
        pCsCfg->Event_Interval = pCsRsp->Event_Interval;

        u32 subevent_len = pCsRsp->Subevent_Len[0] | (pCsRsp->Subevent_Len[1] << 8) | (pCsRsp->Subevent_Len[2] << 16);

        pCsCfg->Subevent_Len = min(subevent_len, pCsCfg->Subevent_Len);
        pCsCfg->Subevent_Len = min2(TLK_CS_SUBEVENT_MAX_LEN, pCsCfg->Subevent_Len);

        pCsCfg->subEvtIntvl_625us = max(pCsCfg->subEvtIntvl_625us, pCsRsp->Subevent_Interval);

        pCsCfg->Subevents_Per_Event = pCsRsp->Subevents_Per_Event;

        pCsCfg->connEventCount = pCsRsp->connEventCount;

        u32 min            = pCsRsp->Offset_Min[0] | (pCsRsp->Offset_Min[1] << 8) | (pCsRsp->Offset_Min[2] << 16);
        u32 max            = pCsRsp->Offset_Max[0] | (pCsRsp->Offset_Max[1] << 8) | (pCsRsp->Offset_Max[2] << 16);
        pCsCfg->offset_min = min;
        pCsCfg->offset_max = max;
        pCsCfg->csOft_us   = min;
    #else
        // Run #3236 - /Pre-Release/LL/CS/CEN/INI/BI-09-C   [Reject a Config Request During an Active CS Procedure With the Same Config_ID , Central, Initiator]
        // if we act as central, config event interval should set same as event interval in config response
        pCsCfg->Event_Interval = pCsRsp->Event_Interval;

        u32 subevent_len = pCsRsp->Subevent_Len[0] | (pCsRsp->Subevent_Len[1] << 8) | (pCsRsp->Subevent_Len[2] << 16);

        pCsCfg->Subevent_Len = min(subevent_len, TLK_CS_SUBEVENT_MAX_LEN);

        //      //subevent interval needs to be >= than the one in RSP
        pCsCfg->subEvtIntvl_625us = max(pCsCfg->subEvtIntvl_625us, pCsRsp->Subevent_Interval);

        pCsCfg->connEventCount = pCsRsp->connEventCount;

        u32 min = pCsRsp->Offset_Min[0] | (pCsRsp->Offset_Min[1] << 8) | (pCsRsp->Offset_Min[2] << 16);
        //Decision: - we take and use the minimum offset
        pCsCfg->csOft_us = min;
    #endif
        pCsParam->cs_req &= ~PROC_CS_WAIT_RSP;
    #if (LL_CS_CEN_INI_BI_01_C)
        if (!csFlowCtrl.csRspCheckErr) {
            pCsParam->cs_req |= PROC_CS_SEND_IND;
        }
    #else
        pCsParam->cs_req |= PROC_CS_SEND_IND;
    #endif
        //todo ind packet calculate
    } else if (opcode == LL_CS_IND) {
        rf_packet_ll_cs_ind_t *pCsInd = (rf_packet_ll_cs_ind_t *)raw;
        cs_config_t           *pCsCfg = gCsMng.gGlobal_pCsCfg + pCsParam->cs_pend_idx;

        //CS_LL_LOG("[RECV_CS_IND]:%s",hex_to_str(raw, sizeof(rf_packet_ll_cs_ind_t)));
        blt_ll_debug_print_cs_ind(pCsInd, "[RECV_CS_IND]");

        if (!(pCsParam->cs_req & PROC_CS_WAIT_IND)) {
            pCsCfg->cs_procedure_en = 0;
            pCsParam->cs_req        = 0;
            CS_LL_LOG("[START] proc abnormal:0x%x", pCsParam->cs_req);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_IND, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }
        /* LL/CS/PER/REF/BI-03-C */
        int ret = blt_ll_cs_checkCsindParam(pAclConn, opcode, raw, pCsCfg);
        if (ret) {
            CS_LL_LOG("[START] ind param check fail:0x%x", ret);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_IND, ret, 1);
        }

        pAclConn->ll_rsp_timeout_tick       = 0;
        pCsCfg->cs_procedure_measurement_en = 1;
        pAclConn->cs_pending |= (pCsCfg->idx | CS_IDX_FLG);

        pCsCfg->connEventCount  = pCsInd->connEventCount;
        pCsCfg->inst_start_proc = pCsCfg->connEventCount;

        pCsCfg->csOft_us       = pCsInd->Offset[0] | pCsInd->Offset[1] << 8 | pCsInd->Offset[2] << 16;
        pCsCfg->Event_Interval = pCsInd->Event_Interval;

        pCsCfg->subEvtIntvl_625us   = pCsInd->Subevent_Interval;
        pCsCfg->sSlot_csSubIntvl    = BSLOT_DUR_2_SSLOT_DUR(pCsCfg->subEvtIntvl_625us);
        pCsCfg->Subevents_Per_Event = pCsInd->Subevents_Per_Event;

        pCsCfg->Subevent_Len = pCsInd->Subevent_Len[0] | (pCsInd->Subevent_Len[1] << 8) | (pCsInd->Subevent_Len[2] << 16);
        pCsCfg->aci          = pCsInd->ACI;
        pCsCfg->PHY          = pCsInd->PHY;
        pCsCfg->Tx_Pwr_Delta = pCsInd->Pwr_Delta;

        pCsCfg->sSlotCsDuration = (pCsCfg->Subevent_Len + SLOT_PROCESS_MAX_US + pCsCfg->sch_early_us) * SSLOT_US_REVERSE + 1;

        if (pCsParam->cs_req & PROC_CS_EVT_PENDING) {
            CS_EBQ_LOG("report proc complete evt in file %s line %d", __FILENAME__, __LINE__);
            hci_le_csProcedureEnableComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8 *)pCsCfg);
        }
    #if (CS_EBQ_TEST && APP_POWER_CONTROL)
        if (pCsParam->cs_req & PROC_CS_PWL_PENDING) {
            CS_EBQ_LOG("[TX]after receive cs ind, Send PWL req");
            blc_ll_readRemoteTxPwrLvl(pAclConn->acl_conHandle, 0, 1);
        }
    #endif
        pCsParam->cs_req = 0;

        blt_ll_calcStepDuration(pCsCfg);

        CS_LL_LOG("[START][RX] ind:0x%x,0x%x,0x%x", pCsCfg->idx, pCsCfg->connEventCount, pCsCfg->csOft_us);

    } else if (opcode == LL_CS_TERMINATE_REQ) {
        CS_LL_LOG("[TERMINATE][RX] req");
        rf_pkt_ll_cs_terminate_req_t *pCsReq = (rf_pkt_ll_cs_terminate_req_t *)raw;
        cs_config_t                  *pCsCfg = gCsMng.gGlobal_pCsCfg + pCsParam->cs_pend_idx;
    /*  StartCSProcCount is defined as the starting CSProcCount value used for the first
            instance of the CS procedure series that is being terminated. */
    #if (0)
        pCsCfg->startCsProcCount = pCsCfg->csProcCount + 1;
        pCsCfg->endCsProcCount   = pCsCfg->startCsProcCount + pCsCfg->procMaxCount;
        if (pCsCfg->csProcCount < pCsCfg->startCsProcCount || pCsCfg->csProcCount > pCsCfg->endCsProcCount) {
            CS_LL_LOG("[TERMINATE]proc count error:0x%x", pCsCfg->csProcCount);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_TERMINATE_REQ, HCI_ERR_CMD_DISALLOWED, 1);
        }
    #endif

        /* Pre-Release/LL/CS/CEN/INI/BI-12-C    [Reject CS Request PDUs if ACL is Unencrypted, Central, Initiator] 
           Pre-Release/LL/CS/PER/INI/BI-12-C    [Reject CS Request PDUs if ACL is Unencrypted, Peripheral, Initiator]*/
        if (0 == pAclConn->crypt.enable) {
            CS_LL_LOG("[TERMINATE REQ] enc not complete");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_TERMINATE_REQ, HCI_ERR_INSUFFICIENT_SECURITY, 1);
        }
        pCsParam->cs_terminate_error_code = pCsReq->ErrorCode;
        pCsCfg->csProcCount               = pCsReq->ProcCount;

    //PBLE-214:When cs terminate procedure was triggered,Config state should not be set to 0;
    //because if next procedure re-start,will reject cs req if state = 0,and need re-config exchange,it shouldn't
    //be so, so disable next line code -- yuexin 20240527
    #if (0)
        pCsCfg->state = 0;
    #endif
        pCsCfg->Config_ID          = pCsReq->Config_ID;
        pCsParam->cs_terminate_ind = PROC_CS_TERMINATE_SEND_RSP;

    } else if (opcode == LL_CS_TERMINATE_RSP) {
        CS_LL_LOG("[TERMINATE][RX] req");
        /* if send or receive terminate rsp, next cs procedure should be terminate, flag it,
         * then use flag cs_pending to stop cs procedure while insert cs task*/
        csFlowCtrl.csTermiFlag     = Receive_Send_CS_Terminate_RSP;
        pCsParam->cs_terminate_ind = PROC_CS_TERMINATE_EVT_PENDING;
        CS_LL_LOG("CS Terminate Flag now is 1,we receive terminate rsp");
    } else if (opcode == LL_CS_FAE_REQ) {
        CS_LL_LOG("[RECV_FAE_REQ]");

        if (!(LL_FEATURE_MASK_1 & LL_FEATURE_MASK_CHANNEL_SOUNDING_HOST)) {
            CS_LL_LOG("[FAE] host not support");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_FAE_REQ, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }

        if (0 == pAclConn->crypt.enable) {
            CS_LL_LOG("[FAE REQ]ACL is Unencrypted");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_FAE_REQ, HCI_ERR_INSUFFICIENT_SECURITY, 1);
        }

        if ((pCsParam->role_enable & CS_REFLECTOR_ROLE) == 0) //todo double check
        {
            CS_LL_LOG("[FAE] reflector role not en:0x%x", pCsParam->role_enable);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_FAE_REQ, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }
        /*A Link Layer receiving an LL_CS_FAE_REQ PDU after having set the No_FAE bit in its CS capabilities
        shall immediately respond with an LL_REJECT_EXT_IND PDU with the error code Unsupported Feature
        or Parameter Value (0x11).
        */
        if (bltCsLocalSupportCap.Optional_Subfeatures_Supported & CS_No_FAE_SUPPORT) {
            CS_LL_LOG("[FAE] CS_No_FAE_SUPPORT:0x%x", bltCsLocalSupportCap.Optional_Subfeatures_Supported);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_FAE_REQ, HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE, 1);
        }

        pCsParam->cs_fae_req |= (PROC_CS_FAE_SEND_RSP | PROC_CS_FAE_EVT_PENDING);


    } else if (opcode == LL_CS_FAE_RSP) {
        if (0 == pAclConn->crypt.enable) {
            CS_LL_LOG("[FAE RSP]ACL is Unencrypted");
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_FAE_RSP, HCI_ERR_INSUFFICIENT_SECURITY, 1);
        }

        if (!(pCsParam->cs_fae_req & PROC_CS_FAE_WAIT_RSP)) {
            pCsParam->cs_fae_req = 0;
            CS_LL_LOG("[FAE] proc abnormal:0x%x", pCsParam->cs_fae_req);
            return blt_llms_rejectInd(pAclConn->acl_conHandle, LL_CS_FAE_RSP, HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, 1);
        }

        rf_pkt_ll_cs_fae_rsp_t *pRsp = (rf_pkt_ll_cs_fae_rsp_t *)raw;
        smemcpy(pCsParam->fae_table, pRsp->fae_table, 72);

        tlkapi_send_string_data((stkLog_mask & STK_LOG_LL_CS), "[CS][LL][RECV_FAE_RSP]", (u8 *)raw, sizeof(rf_pkt_ll_cs_fae_rsp_t));

        if (pCsParam->cs_fae_req & PROC_CS_FAE_EVT_PENDING) {
            hci_le_readRemoteFAETableComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8 *)&pRsp->fae_table[0]);
        }
        pCsParam->cs_fae_exchange     = 1;
        pAclConn->ll_rsp_timeout_tick = 0;
        pCsParam->cs_fae_req          = 0;
    } else if (opcode == LL_CS_CHANNEL_MAP_IND) {
        CS_LL_LOG("[CHNL][RX] ind");
        rf_pkt_ll_cs_chn_map_ind_t *chm_ind = (rf_pkt_ll_cs_chn_map_ind_t *)raw;
    #if (LL_CS_CEN_INI_BI_08_C)
        extern u8 blt_cs_getEnableChmNum(u8 * chm);
        csChnNum = blt_cs_getEnableChmNum(chm_ind->ChM);
    #endif
        for (u32 i = 0; i < gCsMng.max_num_cofig; i++) {
            if (pAclConn->csMapMask & BIT(i)) {
                cs_config_t *pCfg = gCsMng.gGlobal_pCsCfg + i;
                if (pCfg->state) {
                    smemcpy(pCfg->Chm_Ind_Map, chm_ind->ChM, 10);

                    pCfg->chn_update_inst = chm_ind->instant;
                    chm_ind->instant      = pCfg->chn_update_inst;
                    pCfg->chn_update_pend = 1;
                    if ((pCfg->chn_update_inst - pAclConn->conn_inst) % 65535 >= 32767) {
                        pCfg->chn_update_pend = 2; // update instant in the past, need check if have procedure repeat
                        CS_LL_LOG("upt past,conn evt is %d, upt inst is %d", pAclConn->conn_inst, pCfg->chn_update_inst);
                    }
                    CS_LL_LOG("receive cs chn map update:%d", pCfg->chn_update_inst);
                }
            }
        }

    } else {
        return LL_ERR_UNKNOWN_OPCODE;
    }

    return BLE_SUCCESS;
}

int blc_ll_getCsCapExchStatus(u16 connHandle)
{
    st_ll_conn_t *pAcl     = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    cs_param_t   *pCsParam = &pAcl->csParam;
    return pCsParam->cs_cap_exchange;
}

int blc_ll_getCsFaeExchStatus(u16 connHandle)
{
    st_ll_conn_t *pAcl     = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    cs_param_t   *pCsParam = &pAcl->csParam;
    return pCsParam->cs_fae_exchange;
}

int blc_ll_getCsSecExchStatus(u16 connHandle)
{
    st_ll_conn_t *pAcl     = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    cs_param_t   *pCsParam = &pAcl->csParam;
    return pCsParam->cs_security_exchange;
}

int blc_ll_CsFaeExchCtrl(u16 connHandle)
{
    st_ll_conn_t             *pAcl            = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    cs_param_t               *pCsParam        = &pAcl->csParam;
    chn_sound_capabilities_t *pcsRemoteSupCap = &pAcl->csRemoteSupCap;

    if (pCsParam->cs_fae_exchange) {
        return 0;
    }

    if (pCsParam->role_enable & CS_INITIATOR_ROLE) {
        if (pcsRemoteSupCap->Optional_Subfeatures_Supported & CS_No_FAE_SUPPORT) {             //needn't send cs fae req
            if (pCsParam->role_enable & CS_REFLECTOR_ROLE) {
                if (bltCsLocalSupportCap.Optional_Subfeatures_Supported & CS_No_FAE_SUPPORT) { //needn't peer dev send cs fae req
                    pCsParam->cs_fae_exchange = 1;
                } else {
                    return 1;
                    //wait peer dev initiator cs fae excahnge
                }
            } else {
                pCsParam->cs_fae_exchange = 1;
            }
        } else {
            return 2;                                                                  //blc_hci_le_cs_readRemoteFAE_table(connHandle);
        }
    } else if (pCsParam->role_enable & CS_REFLECTOR_ROLE) {
        if (bltCsLocalSupportCap.Optional_Subfeatures_Supported & CS_No_FAE_SUPPORT) { //needn't peer dev send cs fae req
            pCsParam->cs_fae_exchange = 1;
        } else {
            return 1;
            //wait peer dev initiator cs fae excahnge
        }
    }
    return 0;
}


    #if SUBEVENTLEN_ALG
static u32 blt_ll_getStepDuration_us(cs_config_t *pCsCfg, u8 mode, u8 t_fcs)
{
    u32 duration_us = 0;

    u8 oneByteUs        = (pCsCfg->CS_SYNC_PHY == BLE_1M_PHY) ? 8 : 4;
    u8 configId_T_SW_us = 0;
    u8 num_ap           = ACI_to_N_AP[pCsCfg->aci]; //aci only can be know after LL_CS_IND.

    pCsCfg->antennaPathNum = num_ap;
    st_ll_conn_t *pAclConn = (st_ll_conn_t *)(u32)&blms[pCsCfg->aclHandle & CONN_IDX_MASK];
    /*
         * 1. There shall not be antenna switching activity in the 1:1 configuration.
         * 2. In this configuration, with N_AP antennas in the initiator, the initiator shall be the only device performing
         *    antenna switching. The antenna switch duration selected shall be the T_SW value of the initiator
         */
    if (num_ap == 1) {
        configId_T_SW_us = 0;
    } else if ((pCsCfg->aci >= 1) && ((pCsCfg->aci <= 3))) //1, 2, 3
    {
        configId_T_SW_us = (pCsCfg->Role == CS_CONFIG_INITIATOR_ROLE) ? bltCsLocalSupportCap.T_SW_Time_Supported : pAclConn->csRemoteSupCap.T_SW_Time_Supported;
    } else if ((pCsCfg->aci >= 4) && ((pCsCfg->aci <= 6))) {
        configId_T_SW_us = (pCsCfg->Role == CS_CONFIG_INITIATOR_ROLE) ? pAclConn->csRemoteSupCap.T_SW_Time_Supported : bltCsLocalSupportCap.T_SW_Time_Supported;
    } else {
        configId_T_SW_us = max(bltCsLocalSupportCap.T_SW_Time_Supported, pAclConn->csRemoteSupCap.T_SW_Time_Supported); //unit us
    }
    pCsCfg->T_SW_Us = configId_T_SW_us;

    switch (mode) {
    case MAINMODE_TYPE_MODE_1:
    {
        u16 mode1_T_SY_noPayload = (pCsCfg->CS_SYNC_PHY == BLE_1M_PHY) ? MODE_1_T_SY_1M_US_WITHOUT_SS_RS : MODE_1_T_SY_2M_US_WITHOUT_SS_RS;
        /*** mode_1: T_FCS + T_SY + T_RD + T_IP1 + T_SY + T_FM + T_RD***/
        duration_us             = t_fcs + 2 * (mode1_T_SY_noPayload + (RTT_Type_SeqNum[pCsCfg->RTT_Type] / 8) * oneByteUs) + 2 * T_RD_US + pCsCfg->T_IP1_Us;
        pCsCfg->mode1Step_durUs = duration_us;
    } break;
    case MAINMODE_TYPE_MODE_2:
    {
        /***T_FCS + (T_SW+T_PM)*(N_AP+1) + T_RD + T_IP2 + (T_SW+T_PM)*(N_AP+1) + T_RD; Note: + 1 is for extended slot. */

        duration_us             = t_fcs + 2 * (configId_T_SW_us + T_PM_US[pCsCfg->T_PM]) * (num_ap + 1) + 2 * T_RD_US + pCsCfg->T_IP2_Us;
        pCsCfg->mode2Step_durUs = duration_us;
    } break;
    case MAINMODE_TYPE_MODE_3:
    {
        u16 mode3_T_SY_noPayload = (pCsCfg->CS_SYNC_PHY == BLE_1M_PHY) ? MODE_3_T_SY_1M_US_WITHOUT_SS_RS : MODE_3_T_SY_2M_US_WITHOUT_SS_RS;
        /***T_FCS + T_SY + T_GD + (T_SW+T_PM)*(N_AP+1) + T_RD + T_IP2 + (T_SW+T_PM)*(N_AP+1) + T_GD + T_SY + T_RD***/
        u16 tmp_T_SY            = 2 * (mode3_T_SY_noPayload + (RTT_Type_SeqNum[pCsCfg->RTT_Type] / 8) * oneByteUs);
        duration_us             = t_fcs + 2 * (tmp_T_SY + T_GD_US + (configId_T_SW_us + T_PM_US[pCsCfg->T_PM]) * (num_ap + 1) + T_RD_US) + pCsCfg->T_IP2_Us;
        pCsCfg->mode3Step_durUs = duration_us;
    } break;
    default:
        break;
    }

    return duration_us;
}

u32 blt_ll_calcMinSubeventLen(cs_config_t *pCsCfg)
{
    //simulated parameters
    u16 t_sim_synu_us = (pCsCfg->CS_SYNC_PHY == BLE_1M_PHY) ? MODE_0_T_SY_1M_US : MODE_0_T_SY_2M_US;

    /*** mode_0: T_FCS + T_SY + T_RD +  T_IP1 + T_SY + T_GD + T_FM + T_RD***/
    /*** this variable is to be used to calculate minimum subevent length ***/
    // all mode0 steps duration in a single subevent
    u16 t_sim_allMode0Step_durUs = pCsCfg->Mode_0_Steps * (pCsCfg->T_FCS_Us + 2 * t_sim_synu_us + 2 * T_RD_US + pCsCfg->T_IP1_Us + T_GD_US + MODE_0_T_FM_US);

    // single main mode step duration
    u32 t_sim_mainModeStep_durUs = blt_ll_getStepDuration_us(pCsCfg, pCsCfg->Main_Mode, pCsCfg->T_FCS_Us);

    /*** maximum number of events that can fit in the procedure ***/
    u16 t_sim_maxEventCnt = pCsCfg->Procedure_Interval / pCsCfg->Event_Interval;

    /*** At the moment, just take # subevents from request TBD: preferred # of subevents ***/
    /*** # of subevents can only be smaller than the # in request - is not explicit in spec ***/
    /*** , but other vendors seem to treat it that way ***/
    /*** TBD: Make sure we can fit the calculated number of subevents in the procedure ***/
    u16 t_sim_maxSubeventCnt = t_sim_maxEventCnt * pCsCfg->Subevents_Per_Event;

    /*** less than N_MAX_SUBEVENTS_PER_PROCEDURE == 32 ***/
    t_sim_maxSubeventCnt = min(t_sim_maxSubeventCnt, CS_SUBEVENT_PER_PROCEDURE_MAX);

    u16 t_sim_mode0StepsCntInProc = t_sim_maxSubeventCnt * pCsCfg->Mode_0_Steps;

    u16 t_sim_mainModeStepsProcedureCnt = 0;
    u16 t_sim_mainModeStepsCnt          = 0;
    u32 t_sim_minSubeventLen            = 0;

    if (pCsCfg->Sub_Mode == SUBMODE_TYPE_MODE_UNUSED) {
        //*** TBD (potentially - super rare case): ***//
        //*** scenario with N_MAX_SUBEVENTS_PER_PROCEDURE == 32, where calculated procedure steps are ok to be > CS_STEPS_PER_PROCEDURE_MAX ***//
        //*** and limiting steps also with CS_STEPS_PER_PROCEDURE_MAX would be wrong in this special case ***//

        //*** Idea: ***//
        //*** - for subevent_per_event = 1 use event interval, for other cases use subevent interval ***//
        //*** - subtract the subevent related buffers - minimum amount of time between end of subevent and beginning of the next one ***//
        //*** - this would already give a valid subevent length, but a proper comparison with CS_STEPS_PER_PROCEDURE_MAX would be needed. ***//
        //*** - Calculate: -> t_sim_mainModeStepsCnt = (subevent_len - t_sim_allMode0Step_durUs) / t_sim_mainModeStep_durUs ***//
        //*** - Calculate: -> StepsInProcCnt = t_sim_mainModeStepsCnt * 32 + t_sim_mode0StepsCntInProc ***//
        //*** Compare StepsInProcCnt with CS_STEPS_PER_PROCEDURE_MAX ***///
        //*** If ">", then our niche case happens we currently return min subevent_len smaller than it should be, which may cause an issue ***//
        //*** , and we may allow a procedure to run, which could get truncated. For "<=" there is no issue ***//
        //*** I believe this risk is acceptable at the moment, as those would be very strange parameters (ChMRep = 3, Mode0 = 3, very small subevent len in REQ - ~3.6k ms) ***//

        //total number of main mode steps in the whole procedure
        t_sim_mainModeStepsProcedureCnt = pCsCfg->Chn_en_num * pCsCfg->Channel_Map_Repetition;

        //in the case of too many main mode steps calcuated - limit them having max steps per procedure in mind
        if ((t_sim_mainModeStepsProcedureCnt + t_sim_mode0StepsCntInProc) > CS_STEPS_PER_PROCEDURE_MAX) {
            t_sim_mainModeStepsProcedureCnt = CS_STEPS_PER_PROCEDURE_MAX - t_sim_mode0StepsCntInProc;
        }

        //necessary main mode steps in a single subevent
        t_sim_mainModeStepsCnt = t_sim_mainModeStepsProcedureCnt / t_sim_maxSubeventCnt;

        // subeventlen duration all mode0 steps and all main mode steps duration
        t_sim_minSubeventLen = t_sim_mainModeStepsCnt * t_sim_mainModeStep_durUs + t_sim_allMode0Step_durUs;

        // round it up to the closest full 100 us
        if (t_sim_minSubeventLen % 100 == 0) {
            t_sim_minSubeventLen += (100 - (t_sim_minSubeventLen % 100));
        }
    } else {
        //TBD submode handling
        t_sim_minSubeventLen = CS_SUBEVENT_LEN_MAX;
    }

    if (t_sim_minSubeventLen < CS_SUBEVENT_LEN_MIN) {
        t_sim_minSubeventLen = CS_SUBEVENT_LEN_MIN;
    } else if (t_sim_minSubeventLen > CS_SUBEVENT_LEN_MAX) {
        t_sim_minSubeventLen = CS_SUBEVENT_LEN_MAX;
    }

    return t_sim_minSubeventLen;
}
    #endif

ble_sts_t blt_ll_calcStepDuration(cs_config_t *pCsCfg)
{
    u8  oneByteUs            = 8;                               //1M PHY
    u16 mode1_T_SY_noPayload = MODE_1_T_SY_1M_US_WITHOUT_SS_RS; //1M PHY
    u16 mode3_T_SY_noPayload = MODE_3_T_SY_1M_US_WITHOUT_SS_RS; //1M PHY
    u16 t_sync_us            = MODE_0_T_SY_1M_US;               //1M PHY

    u8            configId_T_SW_us = 0;
    u8            num_ap           = ACI_to_N_AP[pCsCfg->aci];  //aci only can be know after LL_CS_IND.
    st_ll_conn_t *pAclConn         = (st_ll_conn_t *)(u32)&blms[pCsCfg->aclHandle & CONN_IDX_MASK];
    u8  rx_early_us      = CS_RFRXEN_MODE_1M_EARLY_US;

    if (pCsCfg->CS_SYNC_PHY == BLE_2M_PHY) {                    //2M PHY
        oneByteUs            = 4;
        t_sync_us            = MODE_0_T_SY_2M_US;
        mode1_T_SY_noPayload = MODE_1_T_SY_2M_US_WITHOUT_SS_RS;
        mode3_T_SY_noPayload = MODE_3_T_SY_2M_US_WITHOUT_SS_RS;
        rx_early_us          = CS_RFRXEN_MODE_2M_EARLY_US;
    }

    pCsCfg->antennaPathNum = num_ap;

    /*
     * 1. There shall not be antenna switching activity in the 1:1 configuration.
     * 2. In this configuration, with N_AP antennas in the initiator, the initiator shall be the only device performing
     *    antenna switching. The antenna switch duration selected shall be the T_SW value of the initiator
     */
    if (num_ap == 1) {
        configId_T_SW_us = 0;
    } else if ((pCsCfg->aci >= 1) && ((pCsCfg->aci <= 3))) //1, 2, 3
    {
        configId_T_SW_us = (pCsCfg->Role == CS_CONFIG_INITIATOR_ROLE) ? bltCsLocalSupportCap.T_SW_Time_Supported : pAclConn->csRemoteSupCap.T_SW_Time_Supported;
    } else if ((pCsCfg->aci >= 4) && ((pCsCfg->aci <= 6))) {
        configId_T_SW_us = (pCsCfg->Role == CS_CONFIG_INITIATOR_ROLE) ? pAclConn->csRemoteSupCap.T_SW_Time_Supported : bltCsLocalSupportCap.T_SW_Time_Supported;
    } else {
        configId_T_SW_us = max(bltCsLocalSupportCap.T_SW_Time_Supported, pAclConn->csRemoteSupCap.T_SW_Time_Supported); //unit us
    }
    pCsCfg->T_SW_Us = configId_T_SW_us;


    u16 t_tone_us = (pCsCfg->T_SW_Us + T_PM_US[pCsCfg->T_PM]) * (num_ap + 1); //Note: + 1 is for extended slot.


    /*** mode_0: T_FCS + T_SY + T_RD +  T_IP1 + T_SY + T_GD + T_FM + T_RD***/
    pCsCfg->mode0TxIntvalUs = t_sync_us + T_RD_US + pCsCfg->T_IP1_Us;
    pCsCfg->mode0_sync_us   = t_sync_us;
    pCsCfg->mode0Step_durUs = pCsCfg->T_FCS_Us + 2 * (t_sync_us + T_RD_US) + pCsCfg->T_IP1_Us + T_GD_US + MODE_0_T_FM_US;

    /*** mode_1: T_FCS + T_SY + T_RD + T_IP1 + T_SY + T_RD***/
    t_sync_us                 = mode1_T_SY_noPayload + RTT_Type_SeqNum[pCsCfg->RTT_Type] * oneByteUs;
    pCsCfg->none_mode_sync_us = t_sync_us;
    pCsCfg->mode1TxIntvalUs   = t_sync_us + T_RD_US + pCsCfg->T_IP1_Us;
    pCsCfg->mode1Step_durUs   = pCsCfg->T_FCS_Us + 2 * (t_sync_us + T_RD_US) + pCsCfg->T_IP1_Us;

    /* Note: the internal delay contains 2 delay: (1) tx_on tick include delay 1.940us;(2) rx_sync tick include delay 14.3us, total 16.24us
     *       convert to unit 0.5ns is 32480(16.24*1000*2), test with cable change to 32184. --yuexin
     */
    /* if SW_DCOC_EN = 1, Turn on the secondary filter 1M PHY will add 2us delay time, add a comment by lijing */
    #if (SW_DCOC_EN)
        #if (CHIP_TYPE == CHIP_TYPE_TL721X)
            #define INTERNAL_DELAY_MEDIAN 26640
        #else
            #define INTERNAL_DELAY_MEDIAN 32184
        #endif
    #else
        #define INTERNAL_DELAY_MEDIAN 26712
    #endif
    #define T_OFFSET_ACESSCODE    80000 //40*1000*2; unit 0.5ns  40us = preamble(8us) + accesscode(32us)
    u32 t_sy_center_delta = 0;

    if (pCsCfg->CS_SYNC_PHY == BLE_2M_PHY) {
        t_sy_center_delta = MODE_1_T_SY_2M_US_WITHOUT_SS_RS + RTT_Type_SeqNum[pCsCfg->RTT_Type] * 4 + T_RD_US + pCsCfg->T_IP1_Us;
    } else {
        t_sy_center_delta = MODE_1_T_SY_1M_US_WITHOUT_SS_RS + RTT_Type_SeqNum[pCsCfg->RTT_Type] * 8 + T_RD_US + pCsCfg->T_IP1_Us;
    }

    t_sy_center_delta *= 2 * 1000; //from us to ns, then to 0.5ns

    /*The calculation of T_OFFSET_ACESSCODE here is to adjust the rx_pkt_iq_sync_tstamp
     *to locate the time when the SYNC preamble is received.
     * reviewer: qiuwei, qinghua, lijing, yuexin
     * Data: 2024.08.07
     */
    #if (MODE1_FINE_RTT && ((CHIP_TYPE == CHIP_TYPE_TL721X)||(CHIP_TYPE == CHIP_TYPE_TL322X)))
        if (pCsCfg->Role == CHANNEL_SOUNDING_ROLE_INITIATOR) {
            pCsCfg->t_sy_center_delta = t_sy_center_delta + T_OFFSET_ACESSCODE + INTERNAL_DELAY_MEDIAN - 52;
        } else {
            pCsCfg->t_sy_center_delta = t_sy_center_delta - T_OFFSET_ACESSCODE - INTERNAL_DELAY_MEDIAN + 52;
        }
    #else
        if (pCsCfg->Role == CHANNEL_SOUNDING_ROLE_INITIATOR) {
            pCsCfg->t_sy_center_delta = t_sy_center_delta + T_OFFSET_ACESSCODE + INTERNAL_DELAY_MEDIAN;
        } else {
            pCsCfg->t_sy_center_delta = t_sy_center_delta - T_OFFSET_ACESSCODE - INTERNAL_DELAY_MEDIAN;
        }
    #endif
    //pm = 10us,3,5,2,valid is 5us
    //pm = 20us,7,8,5,valid is 8us
    //pm = 40us,22,8,10,valid is 8us
#if (CHIP_TYPE == CHIP_TYPE_TL721X)||(CHIP_TYPE == CHIP_TYPE_TL322X)
    u8 cs_tone_exclude_tail_us = T_PM_US[pCsCfg->T_PM] >> 2;
#else
    u8 cs_tone_exclude_tail_us = 1;
#endif
    u8 cs_tone_exclude_head_us = T_PM_US[pCsCfg->T_PM] - cs_tone_exclude_tail_us - (pCsCfg->T_PM ? 8 : 5);

    /*** mode_2: T_FCS + (T_SW+T_PM)*(N_AP+1) + T_RD + T_IP2 + (T_SW+T_PM)*(N_AP+1) + T_RD; */
    pCsCfg->mode2TxIntvalUs       = t_tone_us + T_RD_US + pCsCfg->T_IP2_Us;
    pCsCfg->mode2ToneUs           = t_tone_us;
    pCsCfg->mode2ToneUs_noExtslot = (pCsCfg->T_SW_Us + T_PM_US[pCsCfg->T_PM]) * num_ap;
    pCsCfg->mode2Step_durUs       = pCsCfg->T_FCS_Us + 2 * (t_tone_us + T_RD_US) + pCsCfg->T_IP2_Us;
    pCsCfg->mode2IQ_StartIdx      = 4 + CS_US_TO_IQ_LEN(rx_early_us + cs_tone_exclude_head_us); // 4 DMA len + us*4 *5  unit: byte index
    pCsCfg->mode2IQ_RxIntval      = CS_US_TO_IQ_LEN(pCsCfg->T_PM_Us + pCsCfg->T_SW_Us);
    pCsCfg->mode2IQ_ValidPMLen    = CS_US_TO_IQ_SAMPLE_NUM(pCsCfg->T_PM_Us - cs_tone_exclude_head_us - cs_tone_exclude_tail_us);
    //  pCsCfg->mode2IQ_OffsetTick = (rx_early_us + (pCsCfg->mode2IQ_ValidPMLen >> 2)) * SYSTEM_TIMER_TICK_1US;

    pCsCfg->mode2IQ_OffsetTick = (rx_early_us + (cs_tone_exclude_head_us)) * SYSTEM_TIMER_TICK_1US;

    //  float rpl_before = -44.0981 - 20*log10(g_mode2IQ_len) - (gain - 15);
    gCsMng.rpl_factor = -44.0981 - 20 * log10(pCsCfg->mode2IQ_ValidPMLen) + 15;

    /*** mode_3: T_FCS + T_SY + T_GD + (T_SW+T_PM)*(N_AP+1) + T_RD + T_IP2 + (T_SW+T_PM)*(N_AP+1) + T_GD + T_SY + T_RD***/
    t_sync_us               = mode3_T_SY_noPayload + RTT_Type_SeqNum[pCsCfg->RTT_Type] * oneByteUs;
    pCsCfg->mode3TxIntvalUs = t_sync_us + T_GD_US + t_tone_us + T_RD_US + pCsCfg->T_IP2_Us;
    pCsCfg->mode3Step_durUs = pCsCfg->T_FCS_Us + 2 * (t_sync_us + T_GD_US + t_tone_us + T_RD_US) + pCsCfg->T_IP2_Us;
    #if (!CS_SUBEVENT_LEN_EVALUATE)
        #if CS_EBQ_TEST
    pCsCfg->sch_early_us = 6000; //In BQB TEST, some case need more time to process, as LL/CS/PER/INI/BV-05-C [Initiate CS Start Procedure, Peripheral, Initiator, Mode 1]. By SunWei
        #else
    pCsCfg->sch_early_us = 6000; // subevent len 12ms, 96M clock basic mode1/mode2, 3ms is enough, but for 48M, need more -- yuexin, for IOP todo
        #endif
    #endif
    return BLE_SUCCESS;
}

    /**
 * @brief  This function is used to set mode0 packet PDU, send 16 bits trailer,the spe stipulates that the trailer is 4bits.
 */
    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ void
    blt_cs_mode0_packetSyncPDU(rf_packet_cs_mode0_t *pPkt, u32 access_code)
{
    smemcpy((u8 *)&pPkt->accessAddress, &access_code, 4);
    pPkt->preamble[0] = pPkt->preamble[1] = BIT_IS_SET(pPkt->accessAddress, 0) ? 0x55 : 0xAA;
    pPkt->trailer                         = BIT_IS_SET(pPkt->accessAddress, 31) ? 0xAAAA : 0x5555;
    pPkt->dma_len                         = rf_tx_packet_dma_len(SYNC_PDU_1M_MODE0_LEN);
}

#endif
