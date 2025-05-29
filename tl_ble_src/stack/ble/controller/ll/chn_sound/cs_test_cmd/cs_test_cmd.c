/********************************************************************************************************
 * @file    cs_test_cmd.c
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


void debugwait(void);

void debugwait(void)
{
#if (CS_EBQ_TEST && TLKAPI_DEBUG_ENABLE)
    while (tlkapi_debug_isBusy()) {
        tlkapi_debug_handler();
    }
#endif
}

#if (CHANNEL_SOUNDING_TEST_MODE_ENABLE)

u8 testMode_drbg_data[216];

ble_sts_t blt_cs_testMode_setOverConfigPointer(hci_le_cs_test_cmdParam_t *pTestCmd);

/*********************  cs test command info calculate start *****************/

overConfig_bit0_is_set_t  *pOverConfig0 = NULL;
overConfig_bit0_not_set_t *pOverConfig1 = NULL; // point to same as pOverConfig0
overConfig_bit2_is_set_t  *pOverConfig2 = NULL;
overConfig_bit3_is_set_t  *pOverConfig3 = NULL;
overConfig_bit4_is_set_t  *pOverConfig4 = NULL;
overConfig_bit5_is_set_t  *pOverConfig5 = NULL;
overConfig_bit6_is_set_t  *pOverConfig6 = NULL;
overConfig_bit7_is_set_t  *pOverConfig7 = NULL;
overConfig_bit8_is_set_t  *pOverConfig8 = NULL;

const u8 Sync_Payload_Pattern[8] = {0x00, 0xf0, 0xaa, 0x00, 0xff, 0x00, 0x0f, 0x55};

// set cs test mode over config parameter
ble_sts_t blt_cs_testMode_setOverConfigPointer(hci_le_cs_test_cmdParam_t *pTestCmd)
{
    pOverConfig0 = NULL;
    pOverConfig1 = NULL;
    pOverConfig2 = NULL;
    pOverConfig3 = NULL;
    pOverConfig4 = NULL;
    pOverConfig5 = NULL;
    pOverConfig6 = NULL;
    pOverConfig7 = NULL;
    pOverConfig8 = NULL;
    u16 overConfig = pTestCmd->Override_Config;

    u8 overParaOffset = 0;

    if (overConfig & BIT(0)) {
        pOverConfig0 = (overConfig_bit0_is_set_t *)&pTestCmd->Override_Parameters_Data[0];
        overParaOffset += pOverConfig0->channel_length + 1;
        CS_TEST_LOG("ORC bit0 set,chn without drbg");
    } else {
        pOverConfig1 = (overConfig_bit0_not_set_t *)&pTestCmd->Override_Parameters_Data[0];
        smemcpy(gCsMng.blt_pCsCfg->Channel_Map, &pTestCmd->Override_Parameters_Data[0], 10);
        gCsMng.blt_pCsCfg->ChSel      = pTestCmd->Override_Parameters_Data[10];
        gCsMng.blt_pCsCfg->Ch3c_Shape = pTestCmd->Override_Parameters_Data[11];
        gCsMng.blt_pCsCfg->Ch3c_Jump  = pTestCmd->Override_Parameters_Data[12];
        overParaOffset += sizeof(overConfig_bit0_not_set_t);
        CS_TEST_LOG("ORC bit0 not set,chn with drbg");
        CS_TEST_LOG("type,shape,jump:%d %d %d", gCsMng.blt_pCsCfg->ChSel, gCsMng.blt_pCsCfg->Ch3c_Shape, gCsMng.blt_pCsCfg->Ch3c_Jump);
    }

    if (overConfig & BIT(2)) {
        pOverConfig2 = (overConfig_bit2_is_set_t *)&pTestCmd->Override_Parameters_Data[overParaOffset];
        overParaOffset += sizeof(overConfig_bit2_is_set_t);
        CS_TEST_LOG("ORC bit2 set,include Main_Mode_Steps:%d", pOverConfig2->main_mode_steps);
    }

    if (overConfig & BIT(3)) {
        pOverConfig3 = (overConfig_bit3_is_set_t *)&pTestCmd->Override_Parameters_Data[overParaOffset];
        overParaOffset += sizeof(overConfig_bit3_is_set_t);
        CS_TEST_LOG("ORC bit3 set,include T_PM_Tone_Ext");
    }

    if (overConfig & BIT(4)) {
        pOverConfig4 = (overConfig_bit4_is_set_t *)&pTestCmd->Override_Parameters_Data[overParaOffset];
        overParaOffset += sizeof(overConfig_bit4_is_set_t);
        CS_TEST_LOG("ORC bit4 set,include Tone_API");
    }

    if (overConfig & BIT(5)) {
        pOverConfig5 = (overConfig_bit5_is_set_t *)&pTestCmd->Override_Parameters_Data[overParaOffset];
        overParaOffset += sizeof(overConfig_bit5_is_set_t);
        CS_TEST_LOG("ORC bit5 set,include SYNC_AA");
    }

    if (overConfig & BIT(6)) {
        pOverConfig6 = (overConfig_bit6_is_set_t *)&pTestCmd->Override_Parameters_Data[overParaOffset];
        overParaOffset += sizeof(overConfig_bit6_is_set_t);
        CS_TEST_LOG("ORC bit6 set,include SS_Marker_Pos");
    }

    if (overConfig & BIT(7)) {
        pOverConfig7 = (overConfig_bit7_is_set_t *)&pTestCmd->Override_Parameters_Data[overParaOffset];
        overParaOffset += sizeof(overConfig_bit7_is_set_t);
        CS_TEST_LOG("ORC bit7 set,include SS_Marker_Val");
    }

    if (overConfig & BIT(8)) {
        pOverConfig8 = (overConfig_bit8_is_set_t *)&pTestCmd->Override_Parameters_Data[overParaOffset];
        overParaOffset += sizeof(overConfig_bit8_is_set_t);
        CS_TEST_LOG("ORC bit8 set,include Sync_Payload");
    }

    if (overConfig & BIT(10)) {
        gCsMng.blt_pCsCfg->stable_phase_en = 1;
        CS_TEST_LOG("ORC bit10 set,stable phase test");
    }

    if (overParaOffset != pTestCmd->Override_Parameters_Length) {
        CS_TEST_LOG("OverConfig param len error,realLen & expectLen:%d & %d", overParaOffset, pTestCmd->Override_Parameters_Length);
        return HCI_ERR_INVALID_LMP_PARAMS;
    }

    return BLE_SUCCESS;
}

u8 tone_ext_init[4] = {0, 1, 0, 1};
u8 tone_ext_refl[4] = {0, 0, 1, 1};

// according to core spec test mode: T_PM_Tone_Ext
void cs_cal_toneExt(u8 T_PM_Tone_Ext, slip_window_step_t *pslip_window_step, u8 step_cnt)
{
    if (T_PM_Tone_Ext < 4) {
        pslip_window_step->extSlot.step_extSlotInit = tone_ext_init[T_PM_Tone_Ext];
        pslip_window_step->extSlot.step_extSlotRefl = tone_ext_refl[T_PM_Tone_Ext];
    } else if (T_PM_Tone_Ext == 4 && step_cnt >= gCsMng.blt_pCsCfg->Mode_0_Steps) {
        unsigned int i                              = (step_cnt - gCsMng.blt_pCsCfg->Mode_0_Steps) % 4;
        pslip_window_step->extSlot.step_extSlotInit = tone_ext_init[i];
        pslip_window_step->extSlot.step_extSlotRefl = tone_ext_refl[i];
    } else {
        CS_TEST_LOG("T_PM_Tone_Ext error!");
    }
}

// calculate step config for each subevent
void blt_cs_testMode_calcStepConfig(void)
{
    CS_TEST_LOG("start calculate cs step info");
    gCsMng.blt_pCsCfg->slip_stepWriteIdx = gCsMng.blt_pCsCfg->slip_stepReadIdx = 0;
    gCsMng.blt_pCsCfg->chnMRepeCnt = 0;

    u32       subevent_len            = 0;
    u8        step_cnt                = 0;
    u8        cur_step_mode           = 0;
    u8        next_step_mode          = 0;
    u8        main_mode_repeat_cnt    = gCsMng.blt_pCsCfg->Main_Mode_Repetition;
    u8        first_cal_mode0_chn     = 1;
    u8        first_cal_non_mode0_chn = 1;
    u8        non_mode0_chn_flush     = 0;
    u8        access_code_task        = 0;
    u8        tone_task               = 0;
    u8        ss_task                 = 0;
    u8        rs_task                 = 0;
    u8        seqbit_len              = 0;
    static u8 chn_lll                 = 0;

    drbg->stepCnt = 0;

    if (gCsMng.blt_pCsCfg->cs_procdure_1st_flag) {
        drbg->stepCnt                      = 0;
        main_mode_repeat_cnt               = 0;  // needn't consider main mode repeat when procedure 1st
        gCsMng.blt_pCsCfg->submode_insertion = 0;
        first_cal_mode0_chn                = 1;
        first_cal_non_mode0_chn            = 1;
    }

        for (int i = 0; subevent_len < gCsMng.blt_pCsCfg->Subevent_Len; i++) {
        if (step_cnt >= SLIP_WINDOW_STEP_NUM) {
            break;
        }
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
                            if (pOverConfig2) {
                                gCsMng.blt_pCsCfg->Main_Mode_Max_Steps = gCsMng.blt_pCsCfg->Main_Mode_Min_Steps = pOverConfig2->main_mode_steps;
                            }
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
            access_code_task = 1;
        } else if (cur_step_mode == STEP_MODE_1) {
            access_code_task = 1;
            if (gCsMng.blt_pCsCfg->RTT_Type == RTT_Type_32bit_ss) {
                ss_task    = SS_TASK_NO_SYNC_PAYLOAD;
                seqbit_len = 32;
            } else if (gCsMng.blt_pCsCfg->RTT_Type == RTT_Type_96bit_ss) {
                ss_task    = SS_TASK_NO_SYNC_PAYLOAD;
                seqbit_len = 96;
            } else if (gCsMng.blt_pCsCfg->RTT_Type == RTT_Type_32bit_rs || gCsMng.blt_pCsCfg->RTT_Type == RTT_Type_64bit_rs ||
                       gCsMng.blt_pCsCfg->RTT_Type == RTT_Type_96bit_rs || gCsMng.blt_pCsCfg->RTT_Type == RTT_Type_128bit_rs) {
                rs_task    = RS_TASK_NO_SYNC_PAYLOAD;
                seqbit_len = 32 * (gCsMng.blt_pCsCfg->RTT_Type - 2);
            }
            if (pOverConfig8) {
                ss_task = rs_task = SS_RS_TASK_SYNC_PAYLOAD;
            }
        } else if (cur_step_mode == STEP_MODE_2) {
            tone_task = 1;
        }
        // step3 calculate cs channel
        if (pOverConfig1) // override config bit0 is not set, calc cs channel accroding to DRBG
        {
            if (cur_step_mode == STEP_MODE_0) {
                if (first_cal_mode0_chn || (gCsMng.blt_pCsCfg->mode0_chnReadIdx >= gCsMng.blt_pCsCfg->Chn_en_num)) {
                    first_cal_mode0_chn = 0;
                    chn_sel_3a(gCsMng.blt_pCsCfg->Chn_en_num, gCsMng.blt_pCsCfg->filteredChnArray, gCsMng.blt_pCsCfg->mode0ShuffledChnArray);
                    //                  CS_TEST_SEND_STRING(1,"mode0 chn",gCsMng.blt_pCsCfg->mode0ShuffledChnArray,72);debugwait();
                    gCsMng.blt_pCsCfg->mode0_chnReadIdx = 0; //restart
                }

                    u8 tChnIdx                     = gCsMng.blt_pCsCfg->mode0_chnReadIdx % gCsMng.blt_pCsCfg->Chn_en_num;
                    pslip_window_step->step_chnIdx = gCsMng.blt_pCsCfg->mode0ShuffledChnArray[tChnIdx];
                gCsMng.blt_pCsCfg->mode0_chnReadIdx++;
                ////mode0 chn function end////
                //access code
            } else {                        //non mode0
                if (main_mode_repeat_cnt) { // repeat main mode
                    main_mode_repeat_cnt--;
                    pslip_window_step->step_chnIdx = gCsMng.blt_pCsCfg->mainmode_repeat_chn[gCsMng.blt_pCsCfg->mainmode_repeat_rptr];
                    gCsMng.blt_pCsCfg->mainmode_repeat_rptr++;
                    gCsMng.blt_pCsCfg->mainmode_repeat_rptr = gCsMng.blt_pCsCfg->mainmode_repeat_rptr % CHN_REPEAT_BUFF_LEN;
                } else {
                    if (cur_step_mode == gCsMng.blt_pCsCfg->Main_Mode) {
                        ////non mode0 chn function start////
                        if (first_cal_non_mode0_chn || (gCsMng.blt_pCsCfg->nonMode0_chnReadIdx >= gCsMng.blt_pCsCfg->Chn_en_num)) {
                            first_cal_non_mode0_chn = 0;
                            if (gCsMng.blt_pCsCfg->ChSel) { // channel select #3c
                                drbg->stepCnt -= gCsMng.blt_pCsCfg->Mode_0_Steps;
                                gCsMng.blt_pCsCfg->noneMode0ShuffledChannelNum = chn_sel_3c(gCsMng.blt_pCsCfg->Channel_Map, gCsMng.blt_pCsCfg->Ch3c_Shape, gCsMng.blt_pCsCfg->Ch3c_Jump, gCsMng.blt_pCsCfg->Channel_Map_Repetition, gCsMng.blt_pCsCfg->nonmode0ShuffledChnArray);
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
            }
            if ((non_mode0_chn_flush == 1) && (gCsMng.blt_pCsCfg->nonMode0_chnReadIdx >= gCsMng.blt_pCsCfg->Chn_en_num)) { //need check mode0 chnmap repeat
                gCsMng.blt_pCsCfg->chnMRepeCnt++;
            }
        } else if (pOverConfig0) // override config bit0 is set, calc cs step channel according to override config param
        {
            if (cur_step_mode == STEP_MODE_0) {
                pslip_window_step->step_chnIdx = pOverConfig0->channel[gCsMng.blt_pCsCfg->mode0_chnReadIdx % pOverConfig0->channel_length];
                gCsMng.blt_pCsCfg->mode0_chnReadIdx++;
            }
            else
            {
                pslip_window_step->step_chnIdx = pOverConfig0->channel[gCsMng.blt_pCsCfg->nonMode0_chnReadIdx % pOverConfig0->channel_length];
                gCsMng.blt_pCsCfg->nonMode0_chnReadIdx++;
                gCsMng.blt_pCsCfg->chnMRepeCnt = gCsMng.blt_pCsCfg->nonMode0_chnReadIdx/pOverConfig0->channel_length;
            }
        }

        if (tone_task) {
            //Antenna path permutation index selection
            if (gCsMng.blt_pCsCfg->aci != 0) { //only when exist multiple antenna path then calculate the API.
                pslip_window_step->step_antPathPermIdx = cs_antenna_path_perm(gCsMng.blt_pCsCfg->antennaPathNum);
            } else {
                pslip_window_step->step_antPathPermIdx = 0;
            }
            if (pOverConfig3) {
                cs_cal_toneExt(pOverConfig3->t_pm_tone_ext, pslip_window_step, step_cnt);
            } else {
                cs_tpm_ext((u8 *)&pslip_window_step->extSlot.step_extSlotFlag); // tone extend through DRBG
            }
            tone_task = 0;
        }

        // step4 calculate access address
        if (access_code_task) {
            if (pOverConfig5) // get AA from override config param
            {
                pslip_window_step->step_initAA = pOverConfig5->cs_sync_AA_init;
                pslip_window_step->step_reflAA = pOverConfig5->cs_sync_AA_refl;
            } else // get AA by DRBG
            {
                cs_access_addr((u8 *)&pslip_window_step->step_reflAA, (u8 *)&pslip_window_step->step_initAA);
                //              CS_TEST_SEND_STRING(1,"AA",(u8*)&pslip_window_step->step_initAA,8);
            }
            access_code_task = 0;
        }
        // step5 check if sounding seq or random seq
        if (ss_task == SS_TASK_NO_SYNC_PAYLOAD) {
            //sounding sequence marker position random & signal select
            pslip_window_step->step_initRttSSPos[0] = 0xff;
            pslip_window_step->step_initRttSSPos[1] = 0xff;
            pslip_window_step->step_reflRttSSPos[0] = 0xff;
            pslip_window_step->step_reflRttSSPos[1] = 0xff;
            u8 select[4]                            = {0};
            u8 position[4]                          = {0};

            // get Sounding Sequence marker poistion
            if (pOverConfig6)                               // override bit6 set, get marker position according to override parameter
            {
                position[0] = pOverConfig6->ss_marker1_pos; // init1
                position[1] = pOverConfig6->ss_marker2_pos; // init2
                position[2] = pOverConfig6->ss_marker1_pos; // refl1
                position[3] = pOverConfig6->ss_marker2_pos; // refl2

            } else {
                cs_ss_marker_position(seqbit_len, pslip_window_step->step_initRttSSPos, pslip_window_step->step_reflRttSSPos);
                position[0] = pslip_window_step->step_initRttSSPos[0]; // init1
                position[1] = pslip_window_step->step_initRttSSPos[1]; // init2
                position[2] = pslip_window_step->step_reflRttSSPos[0]; // refl1
                position[3] = pslip_window_step->step_reflRttSSPos[1]; // refl2
            }

            // get marker signal
            if (pOverConfig7) // override bit7 set, get marker signal according to override parameter
            {
                if (pOverConfig7->ss_marker_value == SS_Partern_0011) {
                    select[0] = select[2] = 0x01;
                    if (position[1] != 0xff) {
                        select[1] = 0x01;
                    }
                    if (position[3] != 0xff) {
                        select[3] = 0x01;
                    }
                } else if (pOverConfig7->ss_marker_value == SS_Partern_1100) {
                    select[0] = select[2] = 0x00;
                    if (position[1] != 0xff) {
                        select[1] = 0x00;
                    }
                    if (position[3] != 0xff) {
                        select[3] = 0x00;
                    }
                } else if (pOverConfig7->ss_marker_value == SS_Partern_0011_1100_Loop) {
                    //select[0] = select[2] = gCsMng.blt_pCsCfg->loopMarkerSigCnt % 2 + 1;
                    if (position[1] != 0xff) {
                        select[1] = select[0] + 1;
                    }
                    if (position[3] != 0xff) {
                        select[3] = select[2] + 1;
                    }

                } else {
                    // not support ss_marker_value
                }
            } else { // get marker signal according to DRBG
                cs_ss_marker_sig_sel(pslip_window_step->step_initRttSeq, pslip_window_step->step_reflRttSeq, &position[0], &position[2]);
                    select[0]    = pslip_window_step->step_initRttSeq[0] & BIT(0);
                    select[1]    = pslip_window_step->step_initRttSeq[1] & BIT(0);
                    select[2]    = pslip_window_step->step_reflRttSeq[0] & BIT(0);
                    select[3]    = pslip_window_step->step_reflRttSeq[1] & BIT(0);
                }

                for (int j = 0; j < (seqbit_len >> 3); j++) {
                    pslip_window_step->step_initRttSeq[j] = 0xaa;
                    pslip_window_step->step_reflRttSeq[j] = 0xaa;
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

        if (rs_task == RS_TASK_NO_SYNC_PAYLOAD) {
                //random sequence generation
                cs_random_seq(pslip_window_step->step_initRttSeq, pslip_window_step->step_reflRttSeq, seqbit_len);
            pslip_window_step->seqMode = 2; //random sequence
        }

        if (ss_task == SS_RS_TASK_SYNC_PAYLOAD || rs_task == SS_RS_TASK_SYNC_PAYLOAD) {
            if (pOverConfig8) {
                u8 *pRTT = NULL;
                if (gCsMng.blt_pCsCfg->Role == CS_INITIATOR_ROLE) {
                    pRTT = (u8 *)(&pslip_window_step->step_initRttSeq[0]);
                } else {
                    pRTT = (u8 *)(&pslip_window_step->step_reflRttSeq[0]);
                }

                if (pOverConfig8->cs_sync_payload_pattern == CS_SYNC_PRBS9) {
                    phyTest_PRBS9(pRTT, seqbit_len >> 3);  // debug-yuexin,sample data check phyTest_PRBS9
                } else if (pOverConfig8->cs_sync_payload_pattern == CS_SYNC_PRBS15) {
                    phyTest_PRBS15(pRTT, seqbit_len >> 3); // debug-yuexin,sample data check phyTest_PRBS15
                } else if (pOverConfig8->cs_sync_payload_pattern == CS_SYNC_USER_PAYLOAD) {
                    smemcpy(pRTT, pOverConfig8->cs_sync_user_payload, seqbit_len >> 3);
                } else {
                    for (int k = 0; k < seqbit_len >> 3; k++) {
                        pRTT[k] = Sync_Payload_Pattern[pOverConfig8->cs_sync_payload_pattern];
                    }
                }
            }
        }

        pslip_window_step->seqLen = (seqbit_len >> 3); //byte

        // step prepare next step
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

        if (subevent_len + next_step_len > gCsMng.blt_pCsCfg->Subevent_Len || step_cnt >= SLIP_WINDOW_STEP_NUM - 1) {
            pslip_window_step->subeventEndFlag = 1;
            gCsMng.blt_pCsCfg->subEvtCnt++;
            CS_TEST_LOG("Reach max subevent len");
        }

        if (gCsMng.blt_pCsCfg->max_subEvtCnt && gCsMng.blt_pCsCfg->subEvtCnt >= gCsMng.blt_pCsCfg->max_subEvtCnt) {
            pslip_window_step->proceStopFlag = 1;
        }
        if(pOverConfig0 && step_cnt > pOverConfig0->channel_length && (gCsMng.blt_pCsCfg->chnMRepeCnt >= gCsMng.blt_pCsCfg->Channel_Map_Repetition))
        {
            pslip_window_step->subeventEndFlag = 1;
            pslip_window_step->proceStopFlag   = 1;
            CS_TEST_LOG("all Config0 chn used");
        }

        // channel select #3b all channel used
        if (gCsMng.blt_pCsCfg->ChSel == 0 && step_cnt > gCsMng.blt_pCsCfg->Mode_0_Steps && gCsMng.blt_pCsCfg->Chn_en_num && gCsMng.blt_pCsCfg->nonMode0_chnReadIdx >= gCsMng.blt_pCsCfg->Chn_en_num && (gCsMng.blt_pCsCfg->chnMRepeCnt >= gCsMng.blt_pCsCfg->Channel_Map_Repetition)) {
            pslip_window_step->subeventEndFlag = 1;
            pslip_window_step->proceStopFlag   = 1;
            CS_TEST_LOG("all 3b chn used");
        }

        // channel select #3c all channel used
        if (gCsMng.blt_pCsCfg->ChSel == 1 && gCsMng.blt_pCsCfg->noneMode0ShuffledChannelNum && gCsMng.blt_pCsCfg->nonMode0_chnReadIdx >= gCsMng.blt_pCsCfg->noneMode0ShuffledChannelNum) {
            pslip_window_step->subeventEndFlag = 1;
            pslip_window_step->proceStopFlag   = 1;
            CS_TEST_LOG("all 3c channel used:%d", gCsMng.blt_pCsCfg->noneMode0ShuffledChannelNum);
        }

    #if (1) // debug step config information
        CS_TEST_LOG("[STEP INFO]STEP CNT & MODE & CHN: %d, %d, %d", step_cnt - 1, pslip_window_step->step_modeType, pslip_window_step->step_chnIdx);
        debugwait();
        CS_TEST_SEND_STRING(1, "[CS][Test Mode][STEP INFO] AA", (u8 *)&pslip_window_step->step_initAA, 8);
        debugwait();
        CS_TEST_SEND_STRING(1, "[CS][Test Mode][STEP INFO] SEQ", (u8 *)&pslip_window_step->step_initRttSeq[0], 32);
        debugwait();
        CS_TEST_LOG("[Test Mode][Step Info]ext init:%d", pslip_window_step->extSlot.step_extSlotInit);
        CS_TEST_LOG("[Test Mode][Step Info]ext refl:%d", pslip_window_step->extSlot.step_extSlotRefl);
    #endif

        if (pslip_window_step->subeventEndFlag) {
            break;
        }

    } // end of for loop

    CS_TEST_LOG("calculate cs step info done");

    gCsMng.blt_pCsCfg->cs_procdure_1st_flag = 0;
}

ble_sts_t blt_cs_testMode_setCSParam(hci_le_cs_test_cmdParam_t *cmdParam)
{
    gCsMng.blt_pCsCfg->Main_Mode             = cmdParam->Main_Mode_Type;
    gCsMng.blt_pCsCfg->Sub_Mode              = cmdParam->Sub_Mode_Type;
    gCsMng.blt_pCsCfg->Main_Mode_Repetition  = cmdParam->Main_Mode_Repetition;
    gCsMng.blt_pCsCfg->Mode_0_Steps          = cmdParam->Mode_0_Steps;
    gCsMng.blt_pCsCfg->Role                  = cmdParam->Role;
    gCsMng.blt_pCsCfg->RTT_Type              = cmdParam->RTT_Type;
    gCsMng.blt_pCsCfg->CS_SYNC_PHY           = cmdParam->CS_SYNC_PHY;
    gCsMng.blt_pCsCfg->Subevent_Len       = cmdParam->Subevent_Len[0] |
                                       (cmdParam->Subevent_Len[1] << 8) |
                                       (cmdParam->Subevent_Len[2] << 16);
    gCsMng.blt_pCsCfg->subEvtIntvl_625us  = cmdParam->Subevent_Interval;
    gCsMng.blt_pCsCfg->max_subEvtCnt       = cmdParam->Max_Num_Subevents;
    gCsMng.blt_pCsCfg->Transmit_Power_Level   = cmdParam->Transmit_Power_Level;
    gCsMng.blt_pCsCfg->T_IP1_Us               = cmdParam->T_IP1_Time;
    gCsMng.blt_pCsCfg->T_IP2_Us               = cmdParam->T_IP2_Time;
    gCsMng.blt_pCsCfg->T_FCS_Us               = cmdParam->T_FCS_Time;
    gCsMng.blt_pCsCfg->T_PM_Us                = cmdParam->T_PM_Time;
    gCsMng.blt_pCsCfg->T_SW_Us                = cmdParam->T_SW_Time;
    gCsMng.blt_pCsCfg->drbg_nonce             = cmdParam->DRBG_Nonce;
    gCsMng.blt_pCsCfg->Channel_Map_Repetition = cmdParam->Channel_Map_Repetion;

    gCsMng.blt_pCsCfg->aci = cmdParam->Tone_Antenna_Config_Selection;

    // debug timing information
    CS_TEST_LOG("ip1, ip2, fcs, pm, sw: %d %d %d %d %d", gCsMng.blt_pCsCfg->T_IP1_Us, gCsMng.blt_pCsCfg->T_IP2_Us, gCsMng.blt_pCsCfg->T_FCS_Us, gCsMng.blt_pCsCfg->T_PM_Us, gCsMng.blt_pCsCfg->T_SW_Us);
    debugwait();

    gCsMng.blt_pCsCfg->antennaPathNum = 1; // tmp default as 1

    if (gCsMng.blt_pCsCfg->Main_Mode == CS_MODE3 || gCsMng.blt_pCsCfg->Sub_Mode == CS_MODE3) {
        CS_TEST_LOG("Mode 3 not support");
        return HCI_ERR_UNSUPPORTED_REMOTE_FEATURE;
    }
    if (gCsMng.blt_pCsCfg->CS_SYNC_PHY == BLE_2M_PHY || gCsMng.blt_pCsCfg->CS_SYNC_PHY == BLE_2M_2BT_PHY) {
        CS_TEST_LOG("PHY 2M/2M BT not support");
        return HCI_ERR_UNSUPPORTED_REMOTE_FEATURE;
    }

    // DRBG_NONCE
    drbg = (drbg_param_t *)&testMode_drbg_data[0];
    cs_drbg_init();
    smemset(drbg->vdrbg,0,16);
    smemset(drbg->kdrbg,0,16);
    gCsMng.blt_pCsCfg->drbg_nonce = U16_HI(gCsMng.blt_pCsCfg->drbg_nonce) | (U16_LO(gCsMng.blt_pCsCfg->drbg_nonce) << 8);
    smemcpy(&drbg->vdrbg[14], (u8 *)&gCsMng.blt_pCsCfg->drbg_nonce, 2);

    // calculate each step duration
    u8  oneByteUs            = 8;                                                     //1M PHY
    u16 mode1_T_SY_noPayload = MODE_1_T_SY_1M_US_WITHOUT_SS_RS;                       //1M PHY
    u16 t_sync_us            = MODE_0_T_SY_1M_US;                                     //1M PHY

    u8 num_ap = ACI_to_N_AP[gCsMng.blt_pCsCfg->aci];                                    //aci only can be know after LL_CS_IND.

    u16 t_tone_us = (gCsMng.blt_pCsCfg->T_SW_Us + gCsMng.blt_pCsCfg->T_PM_Us) * (num_ap + 1); //Note: + 1 is for extended slot.

    /*** mode_0: T_FCS + T_SY + T_RD +  T_IP1 + T_SY + T_GD + T_FM + T_RD***/
    gCsMng.blt_pCsCfg->mode0TxIntvalUs = t_sync_us + T_RD_US + gCsMng.blt_pCsCfg->T_IP1_Us;
    gCsMng.blt_pCsCfg->mode0_sync_us   = t_sync_us;
    gCsMng.blt_pCsCfg->mode0Step_durUs = gCsMng.blt_pCsCfg->T_FCS_Us + 2 * (t_sync_us + T_RD_US) + gCsMng.blt_pCsCfg->T_IP1_Us + T_GD_US + MODE_0_T_FM_US;

    /*** mode_1: T_FCS + T_SY + T_RD + T_IP1 + T_SY + T_RD***/
    t_sync_us                          = mode1_T_SY_noPayload + RTT_Type_SeqNum[gCsMng.blt_pCsCfg->RTT_Type] * oneByteUs;
    gCsMng.blt_pCsCfg->none_mode_sync_us = t_sync_us;
    gCsMng.blt_pCsCfg->mode1TxIntvalUs   = t_sync_us + T_RD_US + gCsMng.blt_pCsCfg->T_IP1_Us;
    gCsMng.blt_pCsCfg->mode1Step_durUs   = gCsMng.blt_pCsCfg->T_FCS_Us + 2 * (t_sync_us + T_RD_US) + gCsMng.blt_pCsCfg->T_IP1_Us;

    /*** mode_2: T_FCS + (T_SW+T_PM)*(N_AP+1) + T_RD + T_IP2 + (T_SW+T_PM)*(N_AP+1) + T_RD; */
    gCsMng.blt_pCsCfg->mode2TxIntvalUs       = t_tone_us + T_RD_US + gCsMng.blt_pCsCfg->T_IP2_Us;
    gCsMng.blt_pCsCfg->mode2ToneUs           = t_tone_us;
    gCsMng.blt_pCsCfg->mode2ToneUs_noExtslot = (gCsMng.blt_pCsCfg->T_SW_Us + gCsMng.blt_pCsCfg->T_PM_Us) * num_ap;
    gCsMng.blt_pCsCfg->mode2Step_durUs       = gCsMng.blt_pCsCfg->T_FCS_Us + 2 * (t_tone_us + T_RD_US) + gCsMng.blt_pCsCfg->T_IP2_Us;

    u8 cs_tone_exclude_tail_us = gCsMng.blt_pCsCfg->T_PM_Us >> 2;
    u8 cs_tone_exclude_head_us = gCsMng.blt_pCsCfg->T_PM_Us - cs_tone_exclude_tail_us - (gCsMng.blt_pCsCfg->T_PM_Us != 10 ? 8 : 5);
    gCsMng.blt_pCsCfg->mode2IQ_StartIdx      = 4 + CS_US_TO_IQ_LEN(CS_RFRXEN_MODE_1M_EARLY_US + cs_tone_exclude_head_us); // 4 DMA len + us*4 *5  unit: byte index
    gCsMng.blt_pCsCfg->mode2IQ_RxIntval      = CS_US_TO_IQ_LEN(gCsMng.blt_pCsCfg->T_PM_Us + gCsMng.blt_pCsCfg->T_SW_Us);
    gCsMng.blt_pCsCfg->mode2IQ_ValidPMLen    = CS_US_TO_IQ_SAMPLE_NUM(gCsMng.blt_pCsCfg->T_PM_Us - cs_tone_exclude_head_us - cs_tone_exclude_tail_us);
    //  pCsCfg->mode2IQ_OffsetTick = (CS_RFRXEN_MODE_EARLY_US + (pCsCfg->mode2IQ_ValidPMLen >> 2)) * SYSTEM_TIMER_TICK_1US;

    gCsMng.blt_pCsCfg->mode2IQ_OffsetTick = (CS_RFRXEN_MODE_1M_EARLY_US + (cs_tone_exclude_head_us)) * SYSTEM_TIMER_TICK_1US;

    // step timing calculation info
    CS_TEST_LOG("mode0 tx interval & sync & duration(us):%d & %d & %d", gCsMng.blt_pCsCfg->mode0TxIntvalUs, gCsMng.blt_pCsCfg->mode0_sync_us, gCsMng.blt_pCsCfg->mode0Step_durUs);

    if (gCsMng.blt_pCsCfg->Main_Mode == STEP_MODE_1) {
        CS_TEST_LOG("mode1 sync packet us & tx interval & duration(us):%d & %d & %d", gCsMng.blt_pCsCfg->none_mode_sync_us, gCsMng.blt_pCsCfg->mode1TxIntvalUs, gCsMng.blt_pCsCfg->mode1Step_durUs);
    } else {
        CS_TEST_LOG("mode2 tx interval & tone & tone no ext & duration(us):%d & %d & %d %d", gCsMng.blt_pCsCfg->mode2TxIntvalUs, gCsMng.blt_pCsCfg->mode2ToneUs, gCsMng.blt_pCsCfg->mode2ToneUs_noExtslot, gCsMng.blt_pCsCfg->mode2Step_durUs);
        debugwait();
    }

    gCsMng.blt_pCsCfg->mode0_rx_flag = 0;

    return BLE_SUCCESS;
}

/**** test mode start  ****/

ble_sts_t blc_hci_le_cs_startCsTest(hci_le_cs_test_cmdParam_t *cmdPara)
{
    CS_TEST_LOG("*******start cs test mode**********");

    gCsMng.blt_pCsCfg = (cs_config_t *)(gCsMng.gGlobal_pCsCfg);

    gCsMng.blt_pCsCfg->cs_procdure_1st_flag = 1;
    gCsMng.blt_pCsCfg->test_mode_en         = 1;

    // enable cs test mode, will trigger another rf_irq and stimer_irq process
    extern int blt_cs_initiator_irq_task(int flag);
    ll_cs_initiator_irq_task_cb     = blt_cs_initiator_irq_task;
    extern int blt_cs_reflector_irq_task(int flag);
    ll_cs_reflector_irq_task_cb     = blt_cs_reflector_irq_task;

    /* if mode2 or mode3 should enable phase continue
     *  tx_cali and rx cali just get once is ok
     *  2024.08.21  fanqh & jiapeng
     */
    if (!gCsMng.cs_get_rf_cali_flag &&
        ((cmdPara->Sub_Mode_Type == SUBMODE_TYPE_MODE_UNUSED) && (cmdPara->Main_Mode_Type != STEP_MODE_1))) {
        extern void ble_rf_cs_settle_cali_init(void);
        ble_rf_cs_settle_cali_init(); //@48mhz 345us,@96mhz 339us
        gCsMng.cs_get_rf_cali_flag = 1;
    }

    if (gCsMng.cs_get_rf_cali_flag) {
        blmsParam.cs_procedure_busy  = 1;
        gCsMng.blt_pCsCfg->phaseContinue_cal_flag = 1;
    }

    if(gCsMng.blt_pCsCfg->CS_SYNC_PHY == BLE_2M_PHY){
        CS_LL_LOG("sync phy 2M: %d, rf switch to 2M ",gCsMng.blt_pCsCfg->CS_SYNC_PHY );
        rf_ble_switch_phy(BLE_2M_PHY,0); // must be called before blt_cs_subevent_rf_init, because it will change some channel sounding rf setting
    }
    // cs rf init
    blt_cs_subevent_rf_init();
    gCsMng.blt_pCsCfg->acl_ac_threshold = reg_rf_modem_sync_thres_ble; //save acl ac_threshold, cs use threshold as 32
    ble_rf_set_accessCodeThreshold(CS_ACCESSCODE_THRESHOLD);

    if (gCsMng.blt_pCsCfg->phaseContinue_cal_flag) {
        ble_rf_cs_phase_continuity_en(); //@48mhz 66us,@96mhz 53us
    }

    hci_le_cs_test_cmdParam_t *pTestCmd = (hci_le_cs_test_cmdParam_t *)cmdPara;

    u8 status = BLE_SUCCESS;

    // set test mode override config parameter
    status = blt_cs_testMode_setOverConfigPointer(pTestCmd);
    if (status != BLE_SUCCESS) {
        return status;
    }

    status = blt_cs_testMode_setCSParam(pTestCmd);
    if (status != BLE_SUCCESS) {
        return status;
    }

    if (pOverConfig1) {
        blt_cs_extractEnableChnMap(pOverConfig1->channel_map, gCsMng.blt_pCsCfg->filteredChnArray, &gCsMng.blt_pCsCfg->Chn_en_num);
        CS_TEST_SEND_STRING(1, "channel map", pOverConfig1->channel_map, 10);
        debugwait();
        CS_TEST_SEND_STRING(1, "filtered channel array", gCsMng.blt_pCsCfg->filteredChnArray, 72);
        debugwait();
    }

    blt_cs_testMode_calcStepConfig();

    // mode2 power check
    //  rf_set_power_level_index(RF_1V8_POWER_INDEX_N12p42dBm);
    //  rf_cs_set_power_level_singletone(RF_1V8_POWER_N12p42dBm);
    rf_ble_set_tx_settle(TX_STL_TIFS_REAL_COMMON);
    rf_ble_set_rx_settle(RX_SETTLE_US);

    if (gCsMng.blt_pCsCfg->Role == CS_CONFIG_INITIATOR_ROLE) {
        DBG_CS_CHN9_LOW;
        gCsMng.blt_pCsCfg->step_expect_tick = clock_time() + 10000 * SYSTEM_TIMER_TICK_1US;
        blt_cs_init_step_stx_start();
    } else if (gCsMng.blt_pCsCfg->Role == CS_CONFIG_REFLECTOR_ROLE) {
        gCsMng.blt_pCsCfg->firstReflRx      = 1;
        gCsMng.blt_pCsCfg->step_expect_tick = clock_time() + 3000 * SYSTEM_TIMER_TICK_1US; // if reflector, open rx window ASAP
        blt_cs_refl_stepSrx();
    }

    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_cs_endCsTest(void)
{
    gCsMng.blt_pCsCfg->test_mode_en = 0;
    gCsMng.blt_pCsCfg->stable_phase_en = 0;
    blt_le_cs_reset();
    return BLE_SUCCESS;
}

#endif // #if (CHANNEL_SOUNDING_TEST_MODE_ENABLE)
