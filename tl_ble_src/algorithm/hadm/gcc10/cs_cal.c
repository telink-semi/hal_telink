/********************************************************************************************************
 * @file    cs_cal.c
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
#include "tlk_algo1/include/libcs_tlk1.h"
#include "tlk_algo2/include/libcs_tlk2.h"
#include "tlk_algo3/include/libcs_tlk3.h"
#include "cs_cal.h"

_attribute_ble_data_retention_  u32 algorithmMask = BLC_RANGING_ALGORITHM_1;

#define DISTANCE_MIN_CHN                    3//minimal channel num for calculating distance

#define MIN_VAL(m1, m2)         ((m1) < (m2) ? (m1) : (m2))
#define MAX_VAL(m1, m2)         ((m1) > (m2) ? (m1) : (m2))
#define REDEF_LOG_EN            (0)
#define MODE2_QUALITY_THREAD    (1)

#ifndef APP_CS_DIST_EN
#define APP_CS_DIST_EN                          0
#endif

#ifndef CS_ALG2_LOG_EN
#define CS_ALG2_LOG_EN                          1
#endif

#ifndef PARSE_TLK_ALGO2_JSON_LOG
#define PARSE_TLK_ALGO2_JSON_LOG                0
#endif

typedef struct __attribute__((packed))  {
    int chn_num;
    float freq_step;
    float likeness[2];
    float dist[2];
}hadm_log_t;

typedef struct __attribute__((packed))  {
    unsigned char  path_count;
    unsigned char  mode;
    hadm_log_t log[0];
}hadm_log_print_t;

_attribute_iram_bss_ u8 dist_params_set[(MAX_ANT_PATHS_SUPPORT+1)*2*sizeof(hadm_log_t)] = {0};

#define CHANNUM (255)

#define STEP_HEAD_LEN           (3)

u8 gProcedureBuf[PROCEDURE_DATA_LEN*2];

static void blt_cs_useFixedDistance(float *dist, blc_ranging_algorithm_enum mask);

#if(CONSOLE_OUTPUT_VIA_DEBUG == 1)
    #define app_parse_printf(fmt, ...)              tlk_printf(fmt, ##__VA_ARGS__)
#else
    extern void app_parse_printf(const char *format, ...);
#endif

extern u8 blc_ras_getStepLength(u8 mode, u8 role, u8 rtt_type, u8 numAntennaPaths);
/*
 * extracting arithmetic progression
 * input array is an incremental array
 * ans is equal, take the smaller d. If both ans and d agree, it doesn't matter.
 */
int extrArithSeq(u8* input, int in_total_len, u8* output, u8 min_len_arith)
{
    if(in_total_len < 1){
        return 0;
    }

    u8 in_len = 0;

    for(int i = in_total_len; i > 0; i--)
    {
        if(input[i-1] != 0)
        {
            in_len = i;
            break;
        }
    }

    u8 in_max = input[in_len - 1];
    u8 in_min = input[0];

    u8 df = in_max - in_min;
    u8 ans = 1;
    int ki = 0;
    int kd = 0;
    for (int d = 0; d <= df; ++d) {
        u8 p[80];                       //max length < 80
        memset(p, 0, 80);
        for (int i = 0; i < in_len; i++) {
            u8 d0 = input[i];
            int d1 = input[i] - d;
            if (d1 >= in_min && d1 <= in_max && p[d1] != 0) {
                p[d0] = MAX_VAL(p[d0], p[d1] + 1);
                if(ans < p[d0]){
                    ans = p[d0];
                    ki = i;
                    kd = d;
                }
            }
            p[d0] = MAX_VAL(p[d0], 1);
            // sync with libiao, used for calculate the arithmetic sequence with minimum step length -- yuexin
            if(min_len_arith && (ans >= DISTANCE_MIN_CHN) && (i == (in_len-1))){
                for(int k = 0;k < ans;k++){
                    output[k] = input[ki] - (ans-1-k)*kd;
                }
                tlkapi_send_string_u32s(CS_ALGO_DEBUG_LOG_EN, "ArithSeq:i,d,len",ki,kd,ans);
                //my_dump_str_data(APP_CS_DIST_DBG, "output d3",&output[0],ans);
                return ans;
            }
        }
    }
    for(int k = 0;k < ans;k++){
        output[k] = input[ki] - (ans-1-k)*kd;
    }

    tlkapi_printf(APP_CS_DIST_EN, "ArithSeq:i = %d, d = %d, len = %d\n",ki,kd,ans);
    return ans;
}

int csStepChannelMask(u8 *chn_mask, u8 *data)
{
    u8 dataCnt = 0;
    for(int i = 0; i < 80; i++)
    {
        if(chn_mask[i/8] & BIT(i%8))
        {
            data[dataCnt] = i;
            dataCnt++;
        }
    }

    return dataCnt;
}

/*
 * intr_buf:initiator buffer
 * refl_buf: reflector buffer
 * qfl:quality filter level
 */
void csStepDataQualityFilter(u8* intr_buf,u8* refl_buf,u8 qfl)
{
    (void)intr_buf;
    (void)refl_buf;
    (void)qfl;
//  u8 chn_mask[10] = {0};

}




void csStepDataMode2(u8 *chn_mask, int chn_len, int *ipm_data, int *pct_data,int* rpl_ipm,int * rpl_pct,u8* initiator_filter_data , u8* reflector_filter_data)
{
    u16 algDataCnt = 0;
    for(int i = 0; i < chn_len; i++) //The total number of all valid channels.
    {
        u32 tonePCTData = 0;
        BYTE_TO_UINT24(tonePCTData ,&initiator_filter_data[chn_mask[i]*5+1]);
        int iSample = ((tonePCTData & BIT_RNG(0, 10)) | ((tonePCTData & BIT(11)) ? BIT_RNG(11,31) : 0));
        int qSample = (((tonePCTData & BIT_RNG(12, 22))>>12) | (((tonePCTData & BIT(23))>>12) ? BIT_RNG(11,31) : 0));
        rpl_ipm[algDataCnt>>1] = (int)initiator_filter_data[chn_mask[i]*5];
        ipm_data[algDataCnt] = iSample;
        ipm_data[algDataCnt+1] = qSample;


        BYTE_TO_UINT24(tonePCTData ,&reflector_filter_data[chn_mask[i]*5+1]);
        iSample = ((tonePCTData & BIT_RNG(0, 10)) | ((tonePCTData & BIT(11)) ? BIT_RNG(11,31) : 0));
        qSample = (((tonePCTData & BIT_RNG(12, 22))>>12) | (((tonePCTData & BIT(23))>>12) ? BIT_RNG(11,31) : 0));
        rpl_pct[algDataCnt>>1] = (int)reflector_filter_data[chn_mask[i]*5];
        pct_data[algDataCnt] = iSample;
        pct_data[algDataCnt+1] = qSample;

        algDataCnt += 2;
    }
}

int csStepQualityScreenMode1(u8 *initiaBuf, u8 *rflBuf, short *initiaAlgData, short *rflAlgData)
{
    cs_step_ctrl_t *rflDes = (cs_step_ctrl_t *)rflBuf;
    cs_step_ctrl_t *initiaDes = (cs_step_ctrl_t *)initiaBuf;

    u16 availCnt = 0;
    //tlkapi_printf(REDEF_LOG_EN, "initia stepNums = %d, modelen = %d", initiaDes->stepNums, initiaDes->modeLen);
    //tlkapi_printf(REDEF_LOG_EN, "reflect stepNums = %d, modelen = %d", rflDes->stepNums, rflDes->modeLen);

    cs_step_mode1_t *rflData = NULL;
    cs_step_mode1_t *initiaData = NULL;

    for(int i = 0; i < rflDes->stepNums; i++)   //Traverse all mode1.
    {
        rflData = (cs_step_mode1_t *)(rflDes->dataCtrl[i].stepStart+STEP_HEAD_LEN);
        initiaData = (cs_step_mode1_t *)(initiaDes->dataCtrl[i].stepStart+STEP_HEAD_LEN);

        tlkapi_printf(APP_CS_DIST_EN, "Packet_Quality= %d", rflData->Packet_Quality);

        tlkapi_printf(APP_CS_DIST_EN, "Channel = %d, %d", rflDes->dataCtrl[i].stepChannel, initiaDes->dataCtrl[i].stepChannel);
        if(rflDes->dataCtrl[i].stepChannel != initiaDes->dataCtrl[i].stepChannel)
        {
            //TODO: cxh
            tlkapi_printf(APP_CS_DIST_EN, "different channel = %d, %d", initiaDes->dataCtrl[i].stepChannel, rflDes->dataCtrl[i].stepChannel);
            break;
        }

        if((0 == rflData->Packet_Quality) && (0 == initiaData->Packet_Quality))
        {
            rflAlgData[availCnt] = rflData->ToA_ToD[0] | (rflData->ToA_ToD[1] << 8);
            initiaAlgData[availCnt] = initiaData->ToA_ToD[0] | (initiaData->ToA_ToD[1] << 8);
            availCnt++;
        }
    }
    return availCnt;
}

void csStepQualityScreenMode2(cs_step_ctrl_t *initiaDes,cs_step_ctrl_t *refDes,u8 *initiaBuf, u8 *rflBuf, u8 *chnMask,u8 antenna_path_num,u8* rets,u8 appi)
{

        u32 ant_permutation_index[24] = {0x03020100,0x03020001,0x03010200,0x03010002,0x03000102,0x03000201,
                                        0x02030100,0x02030001,0x02010300,0x02010003,0x02000103,0x02000301,
                                        0x01020300,0x01020003,0x01030200,0x01030002,0x01000302,0x01000203,
                                        0x00020103,0x00020301,0x00010203,0x00010302,0x00030102,0x00030201};
        int i, j;


        /* Traverse all mode2. */
        for(i=0; i<initiaDes->stepNums; i++)
        {
            cs_step_mode2_t *initiaData = (cs_step_mode2_t *)(initiaDes->dataCtrl[i].stepStart+3);
            cs_step_mode2_t *refData = (cs_step_mode2_t *)(refDes->dataCtrl[i].stepStart+3);
            u8 antenna_index = initiaData->Antenna_Permutation_Index;
            /* 1. different channel, return */
            if(refDes->dataCtrl[i].stepChannel != initiaDes->dataCtrl[i].stepChannel)
            {
                //tlkapi_printf(REDEF_LOG_EN, "different channel: step=%d, initiator=%d, reflector=%d", i, initiaDes->dataCtrl[i].stepChannel, refDes->dataCtrl[i].stepChannel);
                break;
            }

            /* 2. initiator step quality. */
            /* 3. reflector step quality. */

            for(j=0; j<antenna_path_num; j++)
            {
                u8 OrigQual = 0;
                u8 ExtSlot = 0;
                u8 refToneValid = 0;
                u8 initiaToneValid = 0;
                u8 ant_path_index = (u8)((ant_permutation_index[antenna_index]>>(j*8))&0x03);

                if(ant_path_index == appi){
                    OrigQual = initiaData->Tone[j].Tone_Quality_Indicator & 0x0F;
                    ExtSlot = (initiaData->Tone[j].Tone_Quality_Indicator & 0xF0) >> 4;

                    if((j == (antenna_path_num - 1)) && (((initiaData->Tone[j+1].Tone_Quality_Indicator & 0xF0) >> 4)  == 2))
                    {
                        if((initiaData->Tone[j+1].Tone_Quality_Indicator & 0x0F) < OrigQual)
                        {
                            smemcpy(&initiaData->Tone[j].Tone_PCT[0],&initiaData->Tone[j+1].Tone_PCT[0],4);
                            OrigQual = initiaData->Tone[j].Tone_Quality_Indicator & 0x0F;
                        }
                    }

                    if(0==ExtSlot)
                    {

                        if(OrigQual < MODE2_QUALITY_THREAD)
                        {
                            initiaToneValid++;
                        }
                        else
                        {
                            //tlkapi_printf(REDEF_LOG_EN, "step=%d, initiator step quality = %d, antennaPath=%d, ExtSlot=%d",i, OrigQual, initiaDes->dataCtrl[i].antennaPaths, ExtSlot);
                        }
                    }

                    OrigQual = refData->Tone[j].Tone_Quality_Indicator & 0x0F;
                    ExtSlot = (refData->Tone[j].Tone_Quality_Indicator & 0xF0) >> 4;

                    if((j == (antenna_path_num - 1)) && (((refData->Tone[j+1].Tone_Quality_Indicator & 0xF0) >> 4)  == 2))
                    {
                        if((refData->Tone[j+1].Tone_Quality_Indicator & 0x0F) < OrigQual)
                        {
                            smemcpy(&refData->Tone[j].Tone_PCT[0],&refData->Tone[j+1].Tone_PCT[0],4);
                            OrigQual = refData->Tone[j].Tone_Quality_Indicator & 0x0F;
                        }
                    }

                    if(0==ExtSlot)
                    {
                        if(OrigQual < MODE2_QUALITY_THREAD)
                        {
                            refToneValid++;
                        }
                        else
                        {
                            //tlkapi_printf(REDEF_LOG_EN, "step=%d, reflector step quality = %d, antennaPath=%d, ExtSlot=%d", i, OrigQual, refDes->dataCtrl[i].antennaPaths, ExtSlot);
                        }
                    }

                    if(initiaToneValid && refToneValid)
                    {
                        u8 *p = (u8*)&initiaBuf[5*initiaDes->dataCtrl[i].stepChannel];

                        if((initiaData->Tone[j].Tone_Quality_Indicator & 0x0f) <= (0x0f & p[4]))
                        {
                            p[0] = initiaDes->dataCtrl[i].refPowerLvl;
                            smemcpy(&p[1],&initiaData->Tone[j].Tone_PCT[0],4);
                        }

                        p = (u8*)&rflBuf[5*refDes->dataCtrl[i].stepChannel];

                        if((refData->Tone[j].Tone_Quality_Indicator & 0x0f) <= (0x0f & p[4]))
                        {
                            p[0] = refDes->dataCtrl[i].refPowerLvl;
                            smemcpy(&p[1],&refData->Tone[j].Tone_PCT[0],4);
                        }

                        chnMask[(initiaDes->dataCtrl[i].stepChannel/8)] |= BIT(initiaDes->dataCtrl[i].stepChannel%8);
                        rets[0]++;
                    }
                }
            }
        }
}

cs_distance_error_code_t csStepQualityScreenMode2_algo3(cs_step_ctrl_t *initiaDes,cs_step_ctrl_t *refDes,u8 *initiaBuf, u8 *rflBuf, u8 *chnMask,u8 antenna_path_num,u8* rets,u8 appi)
{
    u8 pct_check_error_flag = 0;
    for(int ii = 0; ii < initiaDes->stepNums; ii++)
    {
        cs_step_mode2_t *initiaData = (cs_step_mode2_t *)(initiaDes->dataCtrl[ii].stepStart + 3);
        cs_step_mode2_t *refData = (cs_step_mode2_t *)(refDes->dataCtrl[ii].stepStart + 3);
        u8 antenna_index = initiaData->Antenna_Permutation_Index;
        /* 1. different channel, return */
        if(refDes->dataCtrl[ii].stepChannel != initiaDes->dataCtrl[ii].stepChannel)
        {
            //tlkapi_printf(REDEF_LOG_EN, "different channel: step=%d, initiator=%d, reflector=%d", i, initiaDes->dataCtrl[i].stepChannel, refDes->dataCtrl[i].stepChannel);
            break;
        }

        /* 2. initiator step quality. */
        /* 3. reflector step quality. */

        for(int jj = 0; jj < antenna_path_num; jj++)
        {
            u8 OrigQual = 0;
            u8 ExtSlot = 0;
            u8 refToneValid = 0;
            u8 initiaToneValid = 0;
            //u8 ant_path_index = (u8)((ant_permutation_index[antenna_index]>>(j*8))&0x03);
            u8 ant_path_index = 0;
            float initPCT;
            float reflPCT;

            if(ant_path_index == appi) //appi:  antenna index
            {
                initiaToneValid = 1;
                refToneValid = 1;

                if(initiaToneValid && refToneValid)
                {
                    u8 *p = (u8*)&initiaBuf[5 * initiaDes->dataCtrl[ii].stepChannel];
                    smemcpy(&p[0], &initiaData->Tone[jj].Tone_PCT[0], 4);

                    p = (u8*)&rflBuf[5 * refDes->dataCtrl[ii].stepChannel];
                    smemcpy(&p[0], &refData->Tone[jj].Tone_PCT[0], 4);
                    // check init and refl pct data, if pct is 0, arctan will cause the final distance result be 0.Yuexin
                    initPCT = (initiaData->Tone[jj].Tone_PCT[0] | \
                                    (initiaData->Tone[jj].Tone_PCT[1] << 8) | \
                                    (initiaData->Tone[jj].Tone_PCT[2] << 16));
                    reflPCT = (refData->Tone[jj].Tone_PCT[0] | \
                                    (refData->Tone[jj].Tone_PCT[1] << 8) | \
                                    (refData->Tone[jj].Tone_PCT[2] << 16));
                    if(initPCT == 0 || reflPCT == 0){
                        pct_check_error_flag = 1;
                    }

                    chnMask[(initiaDes->dataCtrl[ii].stepChannel/8)] |= BIT(initiaDes->dataCtrl[ii].stepChannel%8);
                    rets[0]++;
                }
            }
        }
    }
    if(pct_check_error_flag) {
        return CS_DIST_ERR_STEP_CHECK_INVALID;
    } else {
        return CS_DIST_SUCCESS;
    }
}

cs_distance_error_code_t csStepQualityScreenMode2_algo3_mant(cs_step_ctrl_t *initiaDes,cs_step_ctrl_t *refDes, u8 *initiaBuf, u8 *rflBuf, u8 *chnMask,u8 antenna_path_num,
                                    u8* rets)
{
    u8 pct_check_error_flag = 0;
    int buf_step_len = 1 + 4 * antenna_path_num;

    u8* init_ptr = NULL;
    u8* refl_ptr = NULL;
    for(int ii = 0; ii < initiaDes->stepNums; ii++)
    {
        init_ptr = (u8* )(initiaDes->dataCtrl[ii].stepStart + 3);
        refl_ptr = (u8* )(refDes->dataCtrl[ii].stepStart + 3);

        // check if pct data is 0, if so, return error.
        for(int k = 0; k < antenna_path_num; k++) {
            int pct_i = *(init_ptr + 4 * k + 1)
                        | (*(init_ptr + 4 * k + 2) << 8)
                        | (*(init_ptr + 4 * k + 3) << 16);
            int pct_r = *(refl_ptr + 4 * k + 1)
                        | (*(refl_ptr + 4 * k + 2) << 8)
                        | (*(refl_ptr + 4 * k + 3) << 16);
            if(pct_i == 0 || pct_r == 0) {
                pct_check_error_flag = 1;
            }
        }

        u8 *p = (u8*)&initiaBuf[buf_step_len * initiaDes->dataCtrl[ii].stepChannel];
        smemcpy(&p[0], init_ptr, buf_step_len);

        p     = (u8*)&rflBuf[buf_step_len * refDes->dataCtrl[ii].stepChannel];
        smemcpy(&p[0], refl_ptr, buf_step_len);

        chnMask[(initiaDes->dataCtrl[ii].stepChannel/8)] |= BIT(initiaDes->dataCtrl[ii].stepChannel%8);
        rets[0]++;
    }
    if(pct_check_error_flag) {
        return CS_DIST_ERR_STEP_CHECK_INVALID;
    } else {
        return CS_DIST_SUCCESS;
    }
}

void csSubevntModeExtra(u8 *procStart, u16 procDataLen, u8 *StepCtrl, u8 modeType)
{
    cs_step_ctrl_t *stepDescribe = (cs_step_ctrl_t *)StepCtrl;

    u8 *subEvtStart = procStart+PROCEDURE_HEAD_LEN; /* offset procedure header 4byte */
    s16 procLen = procDataLen-PROCEDURE_HEAD_LEN;
    u16 stepIndex = 0;
    u8 numAntennaPath = ((blc_rass_prot_head_t *)procStart)->data.numAntennaPaths;


    stepDescribe->stepMode = modeType;
    stepDescribe->stepNums = 0;
    while(procLen > 0)  //Filter the mode of the procedure regardless of quality.
    {
        blc_rass_data_body_t *subEvtPtr = (blc_rass_data_body_t *)subEvtStart;
        u8 *subDataPtr = (u8 *)subEvtPtr->pSubeventRangingData;
        u16 subEvtLen = 0;


        for(int i = 0; i < subEvtPtr->numStepsReported; i++)
        {
            u32 stepData = 0;
            u8 stepDataLen = 0;
            u8 stepMode = 0;
            BYTE_TO_UINT24(stepData ,subDataPtr);
            stepDataLen = U32_BYTE2(stepData);
            stepMode = U32_BYTE0(stepData);

//          if(stepIndex<160 && (stepMode&0xf) == modeType)
            if((stepMode&0xf) == modeType)
            {
                stepDescribe->dataCtrl[stepIndex].refPowerLvl = subEvtPtr->referencePowerLevel;//arrange RPL to every step for hadm algorithm
                stepDescribe->dataCtrl[stepIndex].antennaPaths = numAntennaPath;
                stepDescribe->dataCtrl[stepIndex].stepChannel = U32_BYTE1(stepData);
                stepDescribe->dataCtrl[stepIndex].stepStart = subDataPtr;
                tlkapi_send_string_data(APP_CS_DIST_EN, "stepStart %s", subDataPtr-3, 6);
                stepIndex++;

                stepDescribe->stepNums = stepIndex;
                stepDescribe->modeLen = stepDataLen;
                tlkapi_printf(APP_CS_DIST_EN, "stepMode = %x, stepNums = %x, modeLen = %x\n",
                        stepDescribe->stepMode,stepDescribe->stepNums, stepDescribe->modeLen);
            }

            subDataPtr += (stepDataLen+STEP_HEAD_LEN);  //next step start
            subEvtLen  += (stepDataLen+STEP_HEAD_LEN);  //subevent length count
        }

        subEvtStart += (subEvtLen+SUBEVENT_HEAD_LEN);       //next subevent index start
        procLen     -= (subEvtLen+SUBEVENT_HEAD_LEN);       //procedure total length calculation
    }
}

u32 csStepExtraMode1(u16 connHandle, blt_ras_proc_ctrl_t *procCtrlInitiator, blt_ras_proc_ctrl_t *procCtrlReflector, float *distance)
{
    (void) connHandle;
    u8 reflectStepCtrl[sizeof(cs_step_ctrl_t) + CS_STEPS_PER_PROCEDURE_MAX*sizeof(cs_step_mode_t)] = {0};
    u8 initiaStepCtrl[sizeof(cs_step_ctrl_t) + CS_STEPS_PER_PROCEDURE_MAX*sizeof(cs_step_mode_t)] = {0};

    cs_step_ctrl_t *initiaDes = (cs_step_ctrl_t *)initiaStepCtrl;
    cs_step_ctrl_t *reflectDes = (cs_step_ctrl_t *)reflectStepCtrl;
    blt_ras_proc_ctrl_t *initiaProcCtrl = procCtrlInitiator; //(blc_rass_data_ctrl_t *)rasCompleteDataset->local.procedure_ctrl_buf;

    blt_ras_proc_ctrl_t *reflectProcCtrl = procCtrlReflector; //(blc_rass_data_ctrl_t *)rasCompleteDataset->remote.procedure_ctrl_buf;

    /* extract data of the same mode in the procedure */
    csSubevntModeExtra(initiaProcCtrl->proc.pData, initiaProcCtrl->proc.dataLen, initiaStepCtrl, STEP_MODE_1);
    if(initiaDes->stepNums == 0)
    {
        return CS_DIST_ERR_STEPS_NUMS_ZEROS;
    }
    csSubevntModeExtra(reflectProcCtrl->proc.pData, reflectProcCtrl->proc.dataLen, reflectStepCtrl, STEP_MODE_1);
    if(reflectDes->stepNums == 0)
    {
        return CS_DIST_ERR_STEPS_NUMS_ZEROS;
    }
    short cte_sync1[CHANNUM] = {0};
    short cte_sync2[CHANNUM] = {0};
    int nAverage = csStepQualityScreenMode1(initiaStepCtrl, reflectStepCtrl, cte_sync1, cte_sync2);

    DBG_CS_CHN7_HIGH;//test hadm algorithm time start    for lijing
    DBG_CS_CHN7_TOGGLE;
    DBG_CS_CHN7_TOGGLE;
    DBG_CS_CHN7_TOGGLE;
    DBG_CS_CHN7_TOGGLE;

    parameterPesCalcDistanceSDK paraPesSDK = pesCalcDistanceInitSDK(nAverage);

    int sync2_flag[CHANNUM] = {0};
    int sync1_flag[CHANNUM] = {0};
    int sync_flag[CHANNUM] = {0};
    for (int i = 0; i < CHANNUM; i++)
    {
        sync1_flag[i] = 1;
        sync2_flag[i] = 1;
        sync_flag[i] = sync1_flag[i] * sync2_flag[i];
    }

    float distPesSync1[CHANNUM] = {0};
    float distPesSync = pesCalcDistSDK(cte_sync1,cte_sync2,sync_flag,distPesSync1,paraPesSDK);

    DBG_CS_CHN7_LOW;//test hadm algorithm time post   for lijing

    tlkapi_printf(CS_ALGO_DEBUG_LOG_EN, "mode1: Distance  = %f", distPesSync);

    *distance = distPesSync;

    return CS_DIST_SUCCESS;
}

u32 csStepExtraMode2(u16 connHandle, blt_ras_proc_ctrl_t *procCtrlInitiator, blt_ras_proc_ctrl_t *procCtrlReflector, float *distance)
{
    (void) connHandle;
    hadm_log_print_t *p_prt = (hadm_log_print_t *)&dist_params_set[0];

    u8 reflectStepCtrl[sizeof(cs_step_ctrl_t) + CS_STEPS_PER_PROCEDURE_MAX*sizeof(cs_step_mode_t)] = {0};
    u8 initiaStepCtrl[sizeof(cs_step_ctrl_t) + CS_STEPS_PER_PROCEDURE_MAX*sizeof(cs_step_mode_t)] = {0};

    smemset(p_prt, 0, sizeof(dist_params_set));

    cs_step_ctrl_t *initiaDes = (cs_step_ctrl_t *)initiaStepCtrl;
    cs_step_ctrl_t *reflectDes = (cs_step_ctrl_t *)reflectStepCtrl;
    blt_ras_proc_ctrl_t *initiaProcCtrl = procCtrlInitiator; //(blc_rass_proc_ctrl_t *)rasCompleteDataset->local.procedure_ctrl_buf;
    blt_ras_proc_ctrl_t *reflectProcCtrl = procCtrlReflector; //(blc_rass_proc_ctrl_t *)rasCompleteDataset->remote.procedure_ctrl_buf;

    /* extract data of the same mode in the procedure */
    csSubevntModeExtra(initiaProcCtrl->proc.pData, initiaProcCtrl->proc.dataLen, initiaStepCtrl, STEP_MODE_2);// offset procedureCounter 4byte
    if(initiaDes->stepNums == 0)
    {
        return CS_DIST_ERR_STEPS_NUMS_ZEROS;
    }

    csSubevntModeExtra(reflectProcCtrl->proc.pData, reflectProcCtrl->proc.dataLen, reflectStepCtrl, STEP_MODE_2);

    if(reflectDes->stepNums == 0)
    {
        return CS_DIST_ERR_STEPS_NUMS_ZEROS;
    }


    u8 ret_flag = 0;
    u8 antenna_path_num = initiaDes->dataCtrl[0].antennaPaths;


    for(int m = 0; m <antenna_path_num; m++ ){
        u8 initiator_filter_data[5*80] = {0};//1 byte rpl, 3byte tone ,1byte tone quality
        u8 reflector_filter_data[5*80] = {0};
        u8 chn_mask[10] = {0};
        u8 res = 0;
        csStepQualityScreenMode2((cs_step_ctrl_t *)initiaStepCtrl,(cs_step_ctrl_t *)reflectStepCtrl,initiator_filter_data, reflector_filter_data, chn_mask,antenna_path_num,&res,m);
        tlkapi_send_string_u8s(CS_ALGO_DEBUG_LOG_EN | 1, "path Num & status", m,res);//REDEF_LOG_EN
        if(res == 0)
        {
            tlkapi_send_string_u8s(CS_ALGO_DEBUG_LOG_EN | 1, "CS_DIST_ERR_STEPS_NUM_NOT_ENOUGH: ant path", m);//REDEF_LOG_EN
            continue;
        }
        else{
            ret_flag = 1;
        }

        /*
         * When update the hadm lib to version 44, the longest phase distance reduce to 14m(past version 39 will reach to about 30m).
         * finally find that if the freq step is 2MHz, distance will not correct with distance > 14m, but for freq step 1MHz, It's okay.
         * Discuss with haili and update the hadm lib to version 55, need 2 step to calculate phase distance.
         * step1: use the arithmetic sequence with minimum step length to calculate phase distance.
         * step2: use the the longest arithmetic sequence to calculate distance again.
         * step3: phase distance combine             -- yuexin 2024.08.22
         */
        u8 chn_data[80] = {0};// channel num max is 80
        u8 data_out[2][80] = {0};

        u8 data_len = csStepChannelMask(chn_mask, chn_data);

        int chnLen1,chnLen2;
        float stepSize = 1.0/1024.0;

        int ipm_1[80*2] = {0};
        int pct_1[80*2] = {0};
        complex ipmpct_1[80];
        float T2WR_1[79];
        float likelinessP1,T2WRDiffMean_1;

        int ipm_2[80*2] = {0};
        int pct_2[80*2] = {0};
        complex ipmpct_2[80];
        float T2WR_2[79];
        float likelinessP2,T2WRDiffMean_2;

        float fstep_1 = 0.0f;
        float fstep_2 = 0.0f;
        float distPhaseDiff_1 = 0.0f;
        float distPhaseDiff_2 = 0.0f;
        float distPhaseDiff_final = 0.0f;
        int rpl_ipm[80] = {0};
        int rpl_pct[80] = {0};

        DBG_CS_CHN7_HIGH;//test hadm algorithm time start    for lijing
        DBG_CS_CHN7_TOGGLE;
        DBG_CS_CHN7_TOGGLE;
        DBG_CS_CHN7_TOGGLE;
        DBG_CS_CHN7_TOGGLE;

        // step1: find minimum frequency step jump sequence
        chnLen1 = extrArithSeq(chn_data, data_len, &data_out[1][0],1);//Minimum arithmetic
        chnLen2 = extrArithSeq(chn_data, data_len, &data_out[0][0],0);//Maximum length

        if(chnLen2 < 3)
        {
            return CS_DIST_ERR_STEPS_NUM_NOT_ENOUGH;
        }
        // Note: when use distance combine in step3 below, must be sure that freqJumpMHz_1 < freqJumpMHz_2
        //       so, if chnLen1 == chnLen2, then operate chnLen2 sequence to make sure that.
        u8 combine_optimize = 0;
        if(chnLen1 == chnLen2)
        {
            combine_optimize = 1;
            int len = chnLen2;
            //chnLen2 seq step size > chnLen1 seq step size
            int n = 0;
            for(n = 0; n< (len+1)/2; n++)
            {
                data_out[0][n] = data_out[0][n*2];
            }
            chnLen2 = n+1;
        }

        csStepDataMode2(&data_out[1][0], chnLen1, ipm_1, pct_1,rpl_ipm,rpl_pct,initiator_filter_data,reflector_filter_data);

        u8 freqJumpMHz_1 = data_out[1][1]-data_out[1][0];
        fstep_1 = 1e6 * freqJumpMHz_1;
        parameterConstTes para_1 = tesInit(chnLen1, fstep_1, stepSize);
        calcIpmPct(ipm_1, rpl_ipm, pct_1, rpl_pct, ipmpct_1, para_1);
        distPhaseDiff_1 = tesPhase(ipmpct_1, T2WR_1, &(T2WRDiffMean_1), &likelinessP1, para_1); // calculate the dis1 and offset

        // step2: use the longest seq ,freq step size and offset to calculate dis2

        csStepDataMode2(&data_out[0][0], chnLen2, ipm_2, pct_2,rpl_ipm,rpl_pct,initiator_filter_data,reflector_filter_data);

        u8 freqJumpMHz_2 = data_out[0][1]-data_out[0][0];
        if((freqJumpMHz_2 != freqJumpMHz_1) && (combine_optimize == 0)) {
            tlkapi_send_string_u8s(CS_ALGO_DEBUG_LOG_EN | 1,"step size not same",freqJumpMHz_1,freqJumpMHz_2);
            fstep_2 = 1e6 * freqJumpMHz_2;
            parameterConstTes para_2 = tesInit(chnLen2, fstep_2, stepSize);
            calcIpmPct(ipm_2, rpl_ipm, pct_2, rpl_pct, ipmpct_2, para_2);
            distPhaseDiff_2 = tesPhase(ipmpct_2, T2WR_2, &(T2WRDiffMean_2), &likelinessP2, para_2);

            // step3: distance combine, This step not must, but it's safer
            distPhaseDiff_final = distCombine(distPhaseDiff_1,freqJumpMHz_1,1,distPhaseDiff_2,freqJumpMHz_2,1);
        }
        else {
            // if second round step size is same, don't need calculate again
            tlkapi_send_string_u8s(CS_ALGO_DEBUG_LOG_EN | 1,"step size same",freqJumpMHz_1,freqJumpMHz_2);
            distPhaseDiff_final = distPhaseDiff_1;
        }

        DBG_CS_CHN7_LOW;//test hadm algorithm time post   for lijing

    // debug info, add TLKAPI_DEBUG_FIFO_SIZE if log is incomplete.
#if(1)

        if(distPhaseDiff_final<0 || distPhaseDiff_final>150){
            distPhaseDiff_final = 0;
        }

        tlkapi_printf(CS_ALGO_DEBUG_LOG_EN | 1,"freq step1: %f, freq step2: %f, likelinessP1: %f, likelinessP2: %f, chnLen1: %d, chnLen2: %d\r\n",
                            fstep_1, fstep_2, likelinessP1, likelinessP2, chnLen1, chnLen2);
        tlkapi_printf(CS_ALGO_DEBUG_LOG_EN | 1,"phase dis1: %f, phase dis2: %f, phase final: %f\r\n",
                            distPhaseDiff_1,distPhaseDiff_2,distPhaseDiff_final);
        tlkapi_send_string_data(CS_ALGO_DEBUG_LOG_EN,"phase minimum step chn seq",(u8*)&data_out[1][0],chnLen1);
        tlkapi_send_string_data(CS_ALGO_DEBUG_LOG_EN,"phase longest step chn seq",(u8*)&data_out[0][0],chnLen2);

        p_prt->log[p_prt->path_count].chn_num =  chnLen2;
        p_prt->log[p_prt->path_count].freq_step =  fstep_2;
        p_prt->log[p_prt->path_count].likeness[0] =  likelinessP1;
        p_prt->log[p_prt->path_count++].dist[0] =  distPhaseDiff_final;
#endif


    }
    //todo:Temporary solution--> single path ok, if there are multi-paths, need to combine, 12.24 add by jiapeng.
    *distance = p_prt->log[0].dist[0];

    if(ret_flag == 0)
    {
        return CS_DIST_ERR_STEPS_NUM_NOT_ENOUGH;
    }
    return CS_DIST_SUCCESS;
}

///////////////////////////////// ALGO3 Start ////////////////////////////////////////

tlka_cs_t  tlk_algo3_cs_st;

void blc_cs_setAlgo3Thd(float cs_thd_s, float cs_thd_m)
{
    tlk_algo3_cs_st.cs_thd_s = cs_thd_s;
    tlk_algo3_cs_st.cs_thd_m = cs_thd_m;
}

void blc_cs_algo3Init(void) {
    tlka_cs_init(&tlk_algo3_cs_st);
    int csAlgo3Version = tlka_cs_get_version();
    #define MICRO(n)    (n & 0xFF)
    #define MINOR(n)    ((n >> 8)&0xFF)
    #define MAJOR(n)    ((n >> 16)&0xFF)
    tlk_printf("[CS][ALGO]Algo3 Version: %d.%d.%d, Algo3 threshold s: %.2f, Algo3 threshold s: %.2f",
               MAJOR(csAlgo3Version),
               MINOR(csAlgo3Version),
               MICRO(csAlgo3Version),
               tlk_algo3_cs_st.cs_thd_s,
               tlk_algo3_cs_st.cs_thd_m);
}

void csSortMode2PctdatabyAPI_algo3(blt_ras_proc_ctrl_t *p){

    u32 ant_permutation_index[24] = {0x03020100,0x03020001,0x03010200,0x03010002,0x03000102,0x03000201,
                                    0x02030100,0x02030001,0x02010300,0x02010003,0x02000103,0x02000301,
                                    0x01020300,0x01020003,0x01030200,0x01030002,0x01000302,0x01000203,
                                    0x00020103,0x00020301,0x00010203,0x00010302,0x00030102,0x00030201};
    u8 N_AP = *(p->proc.pData + 3);
    u8 *procStart = p->proc.pData;
    u16 procDataLen= p->proc.dataLen;

    u8 *subEvtStart = procStart+PROCEDURE_HEAD_LEN; /* offset procedure header 4byte */
    s16 procLen = procDataLen-PROCEDURE_HEAD_LEN;
    #define SORT_LOG    0
    while(procLen > 0)  //Filter the mode of the procedure regardless of quality.
    {
        blc_rass_data_body_t *subEvtPtr = (blc_rass_data_body_t *)subEvtStart;
        u8 *subDataPtr = (u8 *)subEvtPtr->pSubeventRangingData;
        u16 subEvtLen = 0;
        for(int i = 0; i < subEvtPtr->numStepsReported; i++)
        {
            u32 stepData = 0;
            u8 stepDataLen = 0;
            u8 stepMode = 0;
            BYTE_TO_UINT24(stepData ,subDataPtr);
            stepDataLen = U32_BYTE2(stepData);
            stepMode = U32_BYTE0(stepData);
            if((stepMode&0xf) == STEP_MODE_2 && N_AP > 1){
                u8 API = *(subDataPtr+3);
                u8 *pct_ptr = (subDataPtr+4);
                u8 sort_pct[20] = {0};
                tlkapi_printf(SORT_LOG, "Mode: %d,Len:%d,N_AP:%d,API: %d", stepMode, stepDataLen, N_AP, API);
                tlkapi_send_string_data(SORT_LOG, "origin api+pct*n_ap,with out extslot", subDataPtr+3, stepDataLen-4);
                for(int j=0; j<N_AP; j++){
                    u8 offset = (u8)((ant_permutation_index[API]>>(j*8))&0x03);
                    smemcpy(sort_pct + 4*offset, pct_ptr + 4*j, 4);
                    tlkapi_printf(SORT_LOG, "offset: %d", offset);
                }
                // copy sorted pct data
                smemcpy(pct_ptr, sort_pct, 4*N_AP);
                // set API to 0(A1A2A3A4)
                *(subDataPtr+3) = 0;
                tlkapi_send_string_data(SORT_LOG, "sorted api+pct*n_ap,with out extslot", subDataPtr+3, stepDataLen-4);
            }

            subDataPtr += (stepDataLen+STEP_HEAD_LEN);  //next step start
            subEvtLen  += (stepDataLen+STEP_HEAD_LEN);  //subevent length count
        }

        subEvtStart += (subEvtLen+SUBEVENT_HEAD_LEN);       //next subevent index start
        procLen     -= (subEvtLen+SUBEVENT_HEAD_LEN);       //procedure total length calculation
    }
    #undef SORT_LOG
}

// algo3 from xiaowen, just test now
u32 csStepExtraMode2_algo3(u16 connHandle, blt_ras_proc_ctrl_t *procCtrlInitiator, blt_ras_proc_ctrl_t *procCtrlReflector, float *distance)
{
    (void) connHandle;
    hadm_log_print_t *p_prt = (hadm_log_print_t *)&dist_params_set[0];

    u8 reflectStepCtrl[sizeof(cs_step_ctrl_t) + CS_STEPS_PER_PROCEDURE_MAX*sizeof(cs_step_mode_t)] = {0};
    u8 initiaStepCtrl[sizeof(cs_step_ctrl_t) + CS_STEPS_PER_PROCEDURE_MAX*sizeof(cs_step_mode_t)] = {0};

    //smemset(p_prt, 0, sizeof(dist_params_set));
    memset(p_prt, 0, sizeof(dist_params_set));

    cs_step_ctrl_t *initiaDes = (cs_step_ctrl_t *)initiaStepCtrl;
    cs_step_ctrl_t *reflectDes = (cs_step_ctrl_t *)reflectStepCtrl;
    blt_ras_proc_ctrl_t *initiaProcCtrl = procCtrlInitiator; //(blc_rass_proc_ctrl_t *)rasCompleteDataset->local.procedure_ctrl_buf;
    blt_ras_proc_ctrl_t *reflectProcCtrl = procCtrlReflector; //(blc_rass_proc_ctrl_t *)rasCompleteDataset->remote.procedure_ctrl_buf;

#if(0)
    /* jaguar@96MHz
     * 2:2  1420 us
     * 2:1   770 us
     * added by jiapeng 2025.3.12
     */
    csSortMode2PctdatabyAPI_algo3(initiaProcCtrl);
    csSortMode2PctdatabyAPI_algo3(reflectProcCtrl);
#endif

    /* extract data of the same mode in the procedure */
    csSubevntModeExtra(initiaProcCtrl->proc.pData, initiaProcCtrl->proc.dataLen, initiaStepCtrl, STEP_MODE_2);// offset procedureCounter 4byte
    if(initiaDes->stepNums == 0)
    {
        tlkapi_send_string_u32s(stkLog_mask & STK_LOG_ALGO_CS, "initiator pct abnormal",initiaProcCtrl->rangingCounter);
    }

    csSubevntModeExtra(reflectProcCtrl->proc.pData, reflectProcCtrl->proc.dataLen, reflectStepCtrl, STEP_MODE_2);

    if(reflectDes->stepNums == 0)
    {
      tlkapi_send_string_u32s(stkLog_mask & STK_LOG_ALGO_CS, "reflector pct abnormal",reflectProcCtrl->rangingCounter);
    }


    u8 ret_flag = 0;
    u8 ret_val = 0;
    u8 antenna_path_num = initiaDes->dataCtrl[0].antennaPaths;

    u8 initiator_filter_data[17 * 80] = {0};//antperm + 4byte tone x 4 ant
    u8 reflector_filter_data[17 * 80] = {0};
    u8 chn_mask[10] = {0};
    u8 res = 0;

    if(antenna_path_num == 1) {
        ret_val = csStepQualityScreenMode2_algo3((cs_step_ctrl_t *)initiaStepCtrl,
                                                 (cs_step_ctrl_t *)reflectStepCtrl,
                                                 initiator_filter_data,
                                                 reflector_filter_data,
                                                 chn_mask,
                                                 antenna_path_num,
                                                 &res,
                                                 0);
    }
    else {
        ret_val = csStepQualityScreenMode2_algo3_mant((cs_step_ctrl_t *)initiaStepCtrl,
                                            (cs_step_ctrl_t *)reflectStepCtrl,
                                            initiator_filter_data,
                                            reflector_filter_data,
                                            chn_mask,
                                            antenna_path_num,
                                            &res);
    }
    ret_flag = (res == 0) ? 0 : 1;

    u8 chn_data[80] = {0};// channel num max is 80
    u8 data_len = csStepChannelMask(chn_mask, chn_data);

    /**
     *  Note:controller will put agc info into start_acl_event field when enable macro"CS_DEBUG_MODE",and this info
     *  is very important for ALGO3 NLOS performance.
     *  TODO:based on xiaowen,a suggestion: trying to freeze agc when start channel sounding for a while or always
     *  freeze agc.
     */
    tlk_algo3_cs_st.cs_agc_init = (procCtrlInitiator->proc.pData[4] | (procCtrlInitiator->proc.pData[5] << 8));
    tlk_algo3_cs_st.cs_agc_refl = (procCtrlReflector->proc.pData[4] | (procCtrlReflector->proc.pData[5] << 8));
    //app_parse_printf("agc gain init & refl: {%d, %d}",tlk_algo3_cs_st.cs_agc_init,tlk_algo3_cs_st.cs_agc_refl);
    float dist_est = tlka_cs_proc(&tlk_algo3_cs_st, initiator_filter_data, reflector_filter_data, data_len, chn_data, antenna_path_num);
    tlkapi_printf(stkLog_mask & STK_LOG_ALGO_CS, "Ant path: [%d]",antenna_path_num);
    tlkapi_printf(stkLog_mask & STK_LOG_ALGO_CS, "algo3 distance: [%f]",dist_est);
    *distance = dist_est;
    /**
     *  Note: There is a distance matchine in the cs algo3, relevant tlk_algo3_cs_st. So don't return before call tlka_cs_proc,
     *  This api will check if PCT data abnormal, and will return distance with -999.0, so we can judge after distance calculated
     */
    if(dist_est <= -998.0) {
        return CS_DIST_ERR_STEPS_NUMS_ZEROS;
    }
    if(ret_flag == 0)
    {
        return CS_DIST_ERR_STEPS_NUM_NOT_ENOUGH;
    }
    if(ret_val != CS_DIST_SUCCESS){
        return CS_DIST_ERR_STEP_CHECK_INVALID;
    }
    return CS_DIST_SUCCESS;
}

///////////////////////////////// ALGO3 End ////////////////////////////////////////

#if(CS_TLK_ALGO2_EN)

// pre check algo2 time enough or not
#define PRE_CHECK_ALGO2_TIME                0
// tlk algo2 will expeand max 100ms with 96M system clock  todo: need use a parameter to control this time -- yuexin
#define TLK_ALGO2_MAX_EXPEND_TIME_MS        80

u8 Misc_data[192] = {
          0xe8, 0x07, 0x09, 0x03, 0x09, 0x36, 0x15, 0x00, 0x40, 0xe2, 0x01, 0x00, 0xee, 0x02, 0x00, 0x00, 0xe5, 0x32,
          0x90, 0x37, 0xcc, 0x7b, 0x86, 0x37, 0x99, 0xd0, 0x4e, 0x00, 0x15, 0x20, 0x00, 0x00, 0x15, 0x20, 0x00, 0x00,
          0xcd, 0x00, 0xd5, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

u8 ConfigComplete_data[sizeof(l4_csConfigCompleteEvent_t)];
u8 ProcedureEnableComplete_data[sizeof(l4_csProcedureEnableCompleteEvent_t)];
l4_csSubeventResultEvent_t SubeventResultLocal_data_0[L4API_MAX_NUM_SUBEVENTS];
l4_csSubeventResultEvent_t SubeventResultRemote_data_0[L4API_MAX_NUM_SUBEVENTS];
u8 StepDataLocal_data_0[85*(3+1+5*4)];
u8 StepDataRemote_data_0[85*(3+1+5*4)];
u32 l4_localSubEvtData_Idx[CS_SUBEVENT_PER_PROCEDURE_MAX] = {0};
u32 l4_remoteSubEvtData_Idx[CS_SUBEVENT_PER_PROCEDURE_MAX] = {0};
u8 subEvtCnt = 0;

l4_csProcedure_t tlk_proc_data;

#if(PRE_CHECK_ALGO2_TIME)
u8 blt_cs_checkAlgoMargin(u16 connHandle)
{
    u8 check_margin_enough = 0;
    st_ll_conn_t *pc = (st_ll_conn_t*)(u32)&blms[connHandle & CONN_IDX_MASK];
    u16 offset_acl_intvl = pc->conn_inst_mark - gCsMng.blt_pCsCfg->inst_start_proc;
    u16 margin_acl_intvl_num = gCsMng.blt_pCsCfg->Procedure_Interval - (offset_acl_intvl%gCsMng.blt_pCsCfg->Procedure_Interval) - 1;
    tlkapi_printf(CS_ALG2_LOG_EN,"[CS][ALG2] margin acl:%d, current acl: %d, start acl: %d\r\n",margin_acl_intvl_num,pc->conn_inst_mark,gCsMng.blt_pCsCfg->inst_start_proc);
    if(margin_acl_intvl_num *  pc->conn_intvl_n_1m25 * 1.25 < TLK_ALGO2_MAX_EXPEND_TIME_MS || (pc->conn_inst_mark-gCsMng.blt_pCsCfg->inst_start_proc)%gCsMng.blt_pCsCfg->Procedure_Interval == 0){
        check_margin_enough = 0;
        tlkapi_printf(CS_ALG2_LOG_EN,"[CS][ALG2] check acl margin not enough\r\n");
    } else {
        check_margin_enough = 1;
    }
    return check_margin_enough;
}
#endif

void blc_Algo2_CopyConfigCompleteData(void* pData, u32 dataLen){
#if(CS_TLK_ALGO2_EN)
    smemcpy(ConfigComplete_data, pData, dataLen);
#endif
}

void blc_Algo2_CopyProcedureEnableCompleteData(u8 ACI, s8 tx_power, u32 subevent_len, u8 subevents_per_event,
                                               u16 event_interval, u16 procedure_interval, u16 procedure_count){
#if(CS_TLK_ALGO2_EN)
    l4_csProcedureEnableCompleteEvent_t *pl = (l4_csProcedureEnableCompleteEvent_t *)ProcedureEnableComplete_data;
    pl->Tone_Antenna_Config_Selection = ACI;
    pl->Selected_Tx_Power = tx_power;
    pl->Subevent_Len = subevent_len;
    pl->Subevents_Per_Event = subevents_per_event;
    pl->Event_Interval = event_interval;
    pl->Procedure_Interval = procedure_interval;
    pl->Procedure_Count = procedure_count;
#endif
}

void l4_xtAPI_ble_get_const_data(l4_csProcedure_t* ble)
{
    memset(ble, 0, sizeof(l4_csProcedure_t));
    ble->Misc                    = (l4_csMisc_t*)Misc_data;
    ble->ConfigComplete          = (l4_csConfigCompleteEvent_t*)ConfigComplete_data;
    ble->ProcedureEnableComplete = (l4_csProcedureEnableCompleteEvent_t*)ProcedureEnableComplete_data;
    for(int i = 0; i < subEvtCnt; i++) {
        ble->SubeventResultLocal[i]     = (l4_csSubeventResultEvent_t*)&SubeventResultLocal_data_0[i];
        ble->SubeventResultRemote[i]    = (l4_csSubeventResultEvent_t*)&SubeventResultRemote_data_0[i];
        ble->StepDataLocal[i]           = (uint8_t*)&StepDataLocal_data_0[l4_localSubEvtData_Idx[i]];
        ble->StepDataRemote[i]          = (uint8_t*)&StepDataRemote_data_0[l4_remoteSubEvtData_Idx[i]];
    }
    subEvtCnt = 0;
}

static void blc_cs_lamda_misc_data_handle(u16 rangingCounter)
{
    l4_csMisc_t *p = (l4_csMisc_t *)Misc_data;
    smemset(&p->remote_data_classifier,0,sizeof(l4_hadm_dataclassifier_t));
    smemset(&p->local_data_classifier,0,sizeof(l4_hadm_dataclassifier_t));
    p->procedure_counter = rangingCounter;
    p->t_sw = 10;
}

static s32 sub_node_cs_lamda_subEvt_data_handle(u8 mainMode, u16 local_len, u16 remote_len, u8 *localProcCtrl, u8 *remoteProcCtrl)
{
    u8 *pLocal = localProcCtrl;
    u8 *pRemote = remoteProcCtrl;
    u16 total_len_local = 4; // skip procedure header 4 bytes
    u16 total_len_remote = 4;

    u16 localIdx = 0;
    u16 remoteIdx = 0;
    u8 num_path = *(localProcCtrl + 3);
    u8 mode0_steps = ConfigComplete_data[5];

    l4_hadm_error_t error_code = L4_HADM_HIRES180_OK_1;

    for(int i = 0; i < CS_SUBEVENT_PER_PROCEDURE_MAX; i++) {
        #if (LL_CS_SNIFFER_MODE_ENABLE)
        if(i > 0)
        #endif
        {
            tlkapi_send_string_u8s(CS_ALG2_LOG_EN,"[CS][ALG2] proc subevent", i+1);
        }
        subEvtCnt++;
        l4_csSubeventResultEvent_t *p1 = (l4_csSubeventResultEvent_t *)&SubeventResultLocal_data_0[i];
        l4_csSubeventResultEvent_t *p2 = (l4_csSubeventResultEvent_t *)&SubeventResultRemote_data_0[i];

        // according to ranging header structure , subevent header structure and data structure
        p1->Frequency_Compensation = pLocal[total_len_local+2] | (pLocal[total_len_local+3] << 8);
        p1->Num_Antenna_Paths = num_path;
        p1->Num_Steps_Reported = pLocal[total_len_local+7];
        p1->Reference_Power_Level = pLocal[total_len_local+6];

        p2->Frequency_Compensation = pRemote[total_len_remote+2] | (pRemote[total_len_remote+3] << 8);
        p2->Num_Antenna_Paths = num_path;
        p2->Num_Steps_Reported = pRemote[total_len_remote+7];
        p2->Reference_Power_Level = pRemote[total_len_remote+6];

        u16 len_l = 0;
        u16 len_r = 0;
        if(mainMode == STEP_MODE_2){
            len_l = (3+pLocal[total_len_local+10])*mode0_steps + (3+1+4*(num_path+1))*(p1->Num_Steps_Reported-mode0_steps);
            len_r = (3+pRemote[total_len_remote+10])*mode0_steps + (3+1+4*(num_path+1))*(p2->Num_Steps_Reported-mode0_steps);
        } else if(mainMode == STEP_MODE_1){
            #define MODE1_STEP_LEN    9 // mode,chn,len,quality,nadm,rssi,toa_tod[0],toa_tod[1],packet_antenna
            len_l = (3+pLocal[total_len_local+10])*mode0_steps + MODE1_STEP_LEN*(p1->Num_Steps_Reported-mode0_steps);
            len_r = (3+pRemote[total_len_remote+10])*mode0_steps + MODE1_STEP_LEN*(p2->Num_Steps_Reported-mode0_steps);
        } else{
            tlkapi_printf(CS_ALG2_LOG_EN,"[CS][ALG2] main mode type error: %d\r\n", mainMode);
        }

        #if (0)
            tlkapi_printf(CS_ALG2_LOG_EN, "[CS][ALG2] L_Num_Step=%d, L_AbortReason=%02X, L_len=%d, R_Num_Step=%d, R_AbortReason=%02X, R_len=%d\r\n",\
                          p1->Num_Steps_Reported, pLocal[total_len_local+5], len_l,\
                          p2->Num_Steps_Reported, pRemote[total_len_local+5], len_r);
        #endif

        if((len_l > sizeof(StepDataLocal_data_0)) || (len_r > sizeof(StepDataRemote_data_0))){
            tlkapi_printf(CS_ALG2_LOG_EN, "[CS][ALG2] memory_copy length error [len_l:%d,sizeof local:%d,len_r:%d,sizeof remote:%d]\r\n",
                          len_l, sizeof(StepDataLocal_data_0), len_r, sizeof(StepDataRemote_data_0));
            error_code = L4_HADM_HIRES180_ASSERT_MEMORY_USER;
            return error_code;
        }
        smemcpy(&StepDataLocal_data_0[localIdx],&pLocal[total_len_local + 8],len_l);
        smemcpy(&StepDataRemote_data_0[remoteIdx],&pRemote[total_len_remote + 8],len_r);

        l4_localSubEvtData_Idx[i] = localIdx;
        l4_remoteSubEvtData_Idx[i] = remoteIdx;
        localIdx += len_l;
        remoteIdx += len_r;

        total_len_local += (len_l + 8); // 8bytes header
        total_len_remote += (len_r + 8); // 8bytes header

        if(total_len_local >= local_len || total_len_remote >= remote_len){
            break;
        }
    }
    return error_code;
}

static s32 blc_cs_lamda_subEvt_data_handle(blt_ras_proc_ctrl_t *localProcCtrl, blt_ras_proc_ctrl_t *remoteProcCtrl, u8 mainMode)
{
    u8 *pLocal = localProcCtrl->proc.pData;
    u8 *pRemote = remoteProcCtrl->proc.pData;
    u16 local_len = localProcCtrl->proc.dataLen;
    u16 remote_len = remoteProcCtrl->proc.dataLen;
    u16 total_len_local = 4; // skip procedure header 4 bytes
    u16 total_len_remote = 4;

    u16 localIdx = 0;
    u16 remoteIdx = 0;
    u8 num_path = *(localProcCtrl->proc.pData + 3);
    u8 mode0_steps = ConfigComplete_data[5];

    l4_hadm_error_t error_code = L4_HADM_HIRES180_OK_1;

    for(int i = 0; i < CS_SUBEVENT_PER_PROCEDURE_MAX; i++) {
        #if (LL_CS_SNIFFER_MODE_ENABLE)
        if(i > 0)
        #endif
        {
            tlkapi_send_string_u8s(CS_ALG2_LOG_EN,"[CS][ALG2] proc subevent", i+1);
        }
        subEvtCnt++;
        l4_csSubeventResultEvent_t *p1 = (l4_csSubeventResultEvent_t *)&SubeventResultLocal_data_0[i];
        l4_csSubeventResultEvent_t *p2 = (l4_csSubeventResultEvent_t *)&SubeventResultRemote_data_0[i];

        // according to ranging header structure , subevent header structure and data structure
        p1->Frequency_Compensation = pLocal[total_len_local+2] | (pLocal[total_len_local+3] << 8);
        p1->Num_Antenna_Paths = num_path;
        p1->Num_Steps_Reported = pLocal[total_len_local+7];
        p1->Reference_Power_Level = pLocal[total_len_local+6];

        p2->Frequency_Compensation = pRemote[total_len_remote+2] | (pRemote[total_len_remote+3] << 8);
        p2->Num_Antenna_Paths = num_path;
        p2->Num_Steps_Reported = pRemote[total_len_remote+7];
        p2->Reference_Power_Level = pRemote[total_len_remote+6];

        u16 len_l = 0;
        u16 len_r = 0;
        if(mainMode == STEP_MODE_2){
            len_l = (3+pLocal[total_len_local+10])*mode0_steps + (3+1+4*(num_path+1))*(p1->Num_Steps_Reported-mode0_steps);
            len_r = (3+pRemote[total_len_remote+10])*mode0_steps + (3+1+4*(num_path+1))*(p2->Num_Steps_Reported-mode0_steps);
        } else if(mainMode == STEP_MODE_1){
            #define MODE1_STEP_LEN    9 // mode,chn,len,quality,nadm,rssi,toa_tod[0],toa_tod[1],packet_antenna
            len_l = (3+pLocal[total_len_local+10])*mode0_steps + MODE1_STEP_LEN*(p1->Num_Steps_Reported-mode0_steps);
            len_r = (3+pRemote[total_len_remote+10])*mode0_steps + MODE1_STEP_LEN*(p2->Num_Steps_Reported-mode0_steps);
        } else{
            tlkapi_printf(CS_ALG2_LOG_EN,"[CS][ALG2] main mode type error: %d\r\n", mainMode);
        }

        #if (0)
            tlkapi_printf(CS_ALG2_LOG_EN, "[CS][ALG2] L_Num_Step=%d, L_AbortReason=%02X, L_len=%d, R_Num_Step=%d, R_AbortReason=%02X, R_len=%d\r\n",\
                          p1->Num_Steps_Reported, pLocal[total_len_local+5], len_l,\
                          p2->Num_Steps_Reported, pRemote[total_len_local+5], len_r);
        #endif

        if((len_l > sizeof(StepDataLocal_data_0)) || (len_r > sizeof(StepDataRemote_data_0))){
            tlkapi_printf(CS_ALG2_LOG_EN, "[CS][ALG2] memory_copy length error [len_l:%d,sizeof local:%d,len_r:%d,sizeof remote:%d]\r\n",
                          len_l, sizeof(StepDataLocal_data_0), len_r, sizeof(StepDataRemote_data_0));
            error_code = L4_HADM_HIRES180_ASSERT_MEMORY_USER;
            return error_code;
        }
        smemcpy(&StepDataLocal_data_0[localIdx],&pLocal[total_len_local + 8],len_l);
        smemcpy(&StepDataRemote_data_0[remoteIdx],&pRemote[total_len_remote + 8],len_r);

        l4_localSubEvtData_Idx[i] = localIdx;
        l4_remoteSubEvtData_Idx[i] = remoteIdx;
        localIdx += len_l;
        remoteIdx += len_r;

        total_len_local += (len_l + 8); // 8bytes header
        total_len_remote += (len_r + 8); // 8bytes header

        if(total_len_local >= local_len || total_len_remote >= remote_len){
            break;
        }
    }
    return error_code;
}

#if(PARSE_TLK_ALGO2_JSON_LOG)
static void xtapi_dump(const char* p, size_t len)
{
    (void)len;
    app_parse_printf("%s", p);
    app_parse_printf("\n");
}
#endif

void lib_print_callback(const char* p, size_t len)
{
#if(PARSE_TLK_ALGO2_JSON_LOG)
    (void) len;
    app_parse_printf("%s", p);
    app_parse_printf("\n");
#endif
}

enum _l4_hadm_error_status errorStatus;
static void blc_cs_lamda_json_log_handle(void)
{
    l4_csMemCalc_t   param;
    l4_xtAPI_ble_get_const_data(&tlk_proc_data);

    param.format                  = l4_json;
    param.Number_Of_Antenna_Paths = l4_xtAPI_ble_get_num_antenna_path(&tlk_proc_data);
    param.Number_Of_Mode0_Steps   = l4_xtAPI_ble_get_num_mode0_steps(&tlk_proc_data);
    param.Number_Of_Mode1_Steps   = l4_xtAPI_ble_get_num_mode1_steps(&tlk_proc_data);
    param.Number_Of_Mode2_Steps   = l4_xtAPI_ble_get_num_mode2_steps(&tlk_proc_data);
    param.Number_Of_Subevents     = l4_xtAPI_ble_get_num_subevents(&tlk_proc_data);

    tlkapi_printf(CS_ALG2_LOG_EN, "[CS][ALG2] N_AP=%d, M0_Step=%d, M1_Step=%d, M2_Step=%d, SubEvtNum=%d\r\n",\
                   param.Number_Of_Antenna_Paths,param.Number_Of_Mode0_Steps,param.Number_Of_Mode1_Steps,\
                       param.Number_Of_Mode2_Steps,param.Number_Of_Subevents);debugwait();
#if(PARSE_TLK_ALGO2_JSON_LOG)
    unsigned char*   mem_ptr = NULL;
    int32_t          mem_size;

    mem_size = l4_xtAPI_ble_calc_memory_consumption(&param);

    mem_size = 4096;

    mem_ptr  = (unsigned char*)malloc(mem_size);

    tlkapi_printf(CS_ALG2_LOG_EN, "[CS][ALG2] mem_size is: %d\r\n", mem_size);

    if (mem_ptr == NULL){
        tlkapi_printf(CS_ALG2_LOG_EN, "[CS][ALG2] malloc error\r\n");
    }

    l4_hadm_error_t errorCode = l4_xtAPI_ble_convert(&tlk_proc_data, (u32*)mem_ptr, mem_size, xtapi_dump, l4_ssmt_6bit);
    free(mem_ptr);

    const char*                errorCodeText = _l4_hadm_errors_get_textdescriptor_from_errorcode(errorCode, &errorStatus);
    const char*                errorStatusText = _l4_hadm_errors_get_textdescriptor_from_errorstatus(errorStatus);
    if(errorCode != L4_HADM_HIRES180_OK_1){
        app_parse_printf("cs data dump error: %s\r\n",errorCodeText);
    }

    tlkapi_printf(CS_ALG2_LOG_EN, "[CS][ALG2] Error code       : %d\r\n", errorCode);
    tlkapi_printf(CS_ALG2_LOG_EN, "[CS][ALG2] Error code text  : %s\r\n", errorCodeText);
    tlkapi_printf(CS_ALG2_LOG_EN, "[CS][ALG2] Error status text: %s\r\n", errorStatusText);
    if(errorCode != L4_HADM_HIRES180_OK_1){
        tlkapi_send_string_u8s(CS_ALG2_LOG_EN,"[CS][ALG2] BLE convert to json error", errorCode);
    }
#endif

}

static l4hadm_xtAPI_instance_handle_t xtAPI_handle = NULL;

static u32    statusmemory_size  = 0;
static u32    runtimememory_size = 0;

static struct _l4hadm_xtAPI_instance_cfg       xtAPI_cfg          = {0};
static struct _l4hadm_xtAPI_calculation_params xtAPI_pars         = {0};
static l4hadm_xtAPI_result_t                   xtAPI_result       = {0};

u8 L4_status_buff[1024];
_attribute_iram_bss_ u8 L4_runtime_buff[1024*64];

static int blc_cs_lamda_calc_distance(float *distance)
{
    l4_hadm_error_t error_code = L4_HADM_HIRES180_OK_1;

    u8 *status_mem_ptr = NULL;
    u8 *runtime_mem_ptr = NULL;

    // init cs lib
    if(xtAPI_handle == NULL)
    {
        error_code = l4_hadm_xtAPI_init_instance_cfg(&xtAPI_cfg);

        if(error_code != L4_HADM_HIRES180_OK_1)
        {
            tlkapi_printf(CS_ALG2_LOG_EN,"[CS][ALG2] init cs lib config error: %d\r\n", error_code);
        }

        // set user definied configuration
        xtAPI_cfg.instance_topology                    = l4hadm_instance_topology_p2p;
        xtAPI_cfg.pairs_tobehandled_count              = 1;
        xtAPI_cfg.enable_rtt                           = 1;
        xtAPI_cfg.enable_rtt_offsethandling            = 1;
        xtAPI_cfg.enable_distfilter                    = 1;
        xtAPI_cfg.distfilter_typical_delay_s           = 0.3F;
        xtAPI_cfg.max_antennapaths_tobeused            = 4;
        xtAPI_cfg.max_frequencies_tobeused             = L4_HADM_MAXFREQUENCIES_LARGE;
        xtAPI_cfg.reset_runtimememory_before_calculate = 0;
        xtAPI_cfg.dump_func                            = lib_print_callback;    // debug-yuexin, what's difference from xtapi_dump
        xtAPI_cfg.memory_model                         = l4hadm_instance_memory_model_large;

        error_code = l4_hadm_xtAPI_req_requiredsize_forinstance(&xtAPI_cfg, &statusmemory_size, &runtimememory_size);

        if((error_code != L4_HADM_HIRES180_OK_1) || (statusmemory_size == 0) || (runtimememory_size == 0))
        {
            tlkapi_printf(CS_ALG2_LOG_EN, "[CS][ALG2] Error, cannot allocate required memory for cs:Lib: %d\r\n", error_code);
        }
        else
        {
            tlkapi_printf(CS_ALG2_LOG_EN, "[CS][ALG2] Required status/runtime memory for cs:Lib is: %d, %d\r\n", statusmemory_size, runtimememory_size);
        }

//        status_mem_ptr  = (unsigned char *)malloc(statusmemory_size);
//        runtime_mem_ptr = (unsigned char *)malloc(runtimememory_size);

        status_mem_ptr = L4_status_buff;
        runtime_mem_ptr = L4_runtime_buff;

        // open API instance
        if((status_mem_ptr != NULL) && (runtime_mem_ptr != NULL))
        {
            error_code = l4_hadm_xtAPI_open_instance(&xtAPI_cfg, &xtAPI_handle, status_mem_ptr, statusmemory_size,
                                                     runtime_mem_ptr, runtimememory_size);
        }
        else
        {
            tlkapi_printf(CS_ALG2_LOG_EN, "[CS][ALG2] malloc for status/runtime memory error:%d, %d\r\n", statusmemory_size, runtimememory_size);
        }

        if(error_code != L4_HADM_HIRES180_OK_1)
        {
            tlkapi_printf(CS_ALG2_LOG_EN, "[CS][ALG2] open API_init error: %d\r\n", error_code);
        }
        else
        {
            tlkapi_printf(CS_ALG2_LOG_EN, "[CS][ALG2] open API_init success\r\n");
        }
//        free(status_mem_ptr); // free_nonreten
//        free(runtime_mem_ptr); // free_nonreten
    }

    // set configuration to default
    error_code = l4_hadm_xtAPI_init_calculation_params(&xtAPI_pars);

    if(error_code != L4_HADM_HIRES180_OK_1)
    {
        tlkapi_printf(CS_ALG2_LOG_EN, "[CS][ALG2] init calculation param error: %d\r\n", error_code);
    }

    /* check tlk_proc_data */
    /* update the new l4 lib and not support l4_xtAPI_ble_check*/
#if(0)
    if(error_code == L4_HADM_HIRES180_OK_1){
        error_code = l4_xtAPI_ble_check(&tlk_proc_data);
        if(error_code != L4_HADM_HIRES180_OK_1)
        {
            tlkapi_printf(CS_ALG2_LOG_EN, "[CS][ALG2] ble procData check error: %d\r\n", error_code);
        }
    }
#endif

    if(error_code == L4_HADM_HIRES180_OK_1)
    {
        //tlkapi_printf(CS_ALG2_LOG_EN,"[CS][ALG2] init calculate parameter success!\r\n");
        error_code = l4_hadm_xtAPI_ble_calculate(xtAPI_handle, &xtAPI_pars, &tlk_proc_data, &xtAPI_result);
        if(error_code != L4_HADM_HIRES180_OK_1){
            tlkapi_printf(CS_ALG2_LOG_EN,"[CS][ALG2] calculate distance failed: %d\r\n",error_code);
            app_parse_printf("calculate distance fail, error code is %d\r\n",error_code);
        } else {
            // print 6bit coded result
#if(PARSE_TLK_ALGO2_JSON_LOG)   // lamda dump
            l4hadm_xtAPI_result_dump(&xtAPI_result, lib_print_callback);
            printf("\n");
            // dump result in json format
            l4_convert2shortjson(&xtAPI_result, 0, lib_print_callback);
            printf("\n");
#endif
            /** ud: un-filtered distance.
             *  fd: filtered distance of Lambda filter machine, now we use our own filter machine.
             *  uv: un-filtered velocity in meter/seconds. todo:our phase algo bad with dynamic, cause we didn't consider velocity and doppler effect?
             *  qm: variance of  distance, reflect the quality of linear, similar with likeness in our own algo.
             */
            tlkapi_printf(CS_ALG2_LOG_EN, "[CS][ALG2] ud:%.2f, fd:%.2f, uv:%.3f, qm:%.3f\r\n", xtAPI_result.distance_premeter_highres,
                          xtAPI_result.filtered_dist, xtAPI_result.distance_mpros_highres, xtAPI_result.distance_varianz_highres);
            *distance = xtAPI_result.distance_premeter_highres;
        }
    }

    if(error_code != L4_HADM_HIRES180_OK_1)
    {
        /* clean xtAPI_handle, re-initialization API_init */
        xtAPI_handle = NULL;
    }

    return error_code;
}

static int blc_cs_tlk_2_lamda(blt_ras_proc_ctrl_t *localProcCtrl, blt_ras_proc_ctrl_t *remoteProcCtrl, u8 mainMode, float *distance)
{
    l4_hadm_error_t error_code = L4_HADM_HIRES180_OK_1;

    // step1: handle msic data
    blc_cs_lamda_misc_data_handle(localProcCtrl->rangingCounter);

    // step2: handle subevent data
    error_code = blc_cs_lamda_subEvt_data_handle(localProcCtrl, remoteProcCtrl, mainMode);

    if(error_code == L4_HADM_HIRES180_OK_1){
        // step3: print procedure data to json for lamda
        blc_cs_lamda_json_log_handle();

        DBG_CS_CHN7_HIGH;//test hadm algorithm time start    for lijing
        DBG_CS_CHN7_TOGGLE;
        DBG_CS_CHN7_TOGGLE;
        DBG_CS_CHN7_TOGGLE;
        DBG_CS_CHN7_TOGGLE;
        // step4: calculate distance
        error_code = blc_cs_lamda_calc_distance(distance);

        DBG_CS_CHN7_LOW;//test hadm algorithm time post   for lijing

        if(error_code == L4_HADM_HIRES180_OK_1){
            //L4_HADM_HIRES180_OK_1 is equal to 1, but our success is equal to 0.
            error_code = CS_DIST_SUCCESS;
        }
    }

    return error_code;
}
#endif

const char *json_hex_to_str(const void *buf, u8 len)
{
    static const char hex[] = "0123456789abcdef";
    static char str[301];
    const uint8_t *b = buf;
    u8 i;

    len = min(len, (sizeof(str) - 1) / 2);

    for (i = 0; i < len; i++) {
        str[i * 2]     = hex[b[i] >> 4];
        str[i * 2 + 1] = hex[b[i] & 0xf];
    }

    str[i * 2] = '\0';

    return str;
}

/**
 * @brief       sniffer sub node calculate cs distance
 * @param[in]   snifHandle
 * @param[in]   rasRangingCounter
 * @param[in]   mainMode: procedure main mode
 * @param[in]   rasLocalLen: Initiator procedure data length
 * @param[in]   rasRemoteLen: Reflector procedure data length
 * @param[in]   *procCtrlInitiator: Initiator procedure data buffer
 * @param[in]   *procCtrlReflector: Reflector procedure data buffer
 * @param[out]  pointer to distance
 * @return      errcode: error code
 */
s32 snif_sub_node_calculate_cs_distacne(u16 snifHandle, u16 rasRangingCounter, u8 mainMode, u16 rasLocalLen, u16 rasRemoteLen, u8 *procCtrlInitiator, u8 *procCtrlReflector, float *distance)
{
    s32 retval = 0xFFFFFF;
    if(algorithmMask & BLC_RANGING_ALGORITHM_1){
        //blt_cs_useFixedDistance(&distance[0], BLC_RANGING_ALGORITHM_1);
        tlkapi_printf((stkLog_mask & STK_LOG_ALGO_CS), "[CS][DIST] sub node not support CS ALG1\r\n");
        retval = CS_DIST_ERR_RAS_RANGING_DATA_WRONG;
    }
    if(algorithmMask & BLC_RANGING_ALGORITHM_2)
    {
    #if(CS_TLK_ALGO2_EN)
        // get lamda format subevent data based on localProcCtrl and remoteProcCtrl,mode2 4path 72chn 65ms,max 100ms,depend on the IQ data(96M)
        // check the remaining time of the ALGO, mode2 4path 72chn 65ms
        u8 margin_enough = 1;
        #if(PRE_CHECK_ALGO2_TIME) // not stable now, default close
            margin_enough = blt_cs_checkAlgoMargin(snifHandle);
        #endif
        if(margin_enough){
            /* The following function similar to blc_cs_tlk_2_lamda() */
            l4_hadm_error_t error_code = L4_HADM_HIRES180_OK_1;

            // step1: handle msic data
            blc_cs_lamda_misc_data_handle(rasRangingCounter);

            // step2: handle subevent data
            error_code = sub_node_cs_lamda_subEvt_data_handle(mainMode, rasLocalLen, rasRemoteLen, procCtrlInitiator, procCtrlReflector);

            if(error_code == L4_HADM_HIRES180_OK_1){
                // step3: print procedure data to json for lamda
                blc_cs_lamda_json_log_handle();

                DBG_CS_CHN7_HIGH;//test hadm algorithm time start    for lijing
                DBG_CS_CHN7_TOGGLE;
                DBG_CS_CHN7_TOGGLE;
                DBG_CS_CHN7_TOGGLE;
                DBG_CS_CHN7_TOGGLE;
                // step4: calculate distance
                error_code = blc_cs_lamda_calc_distance(&distance[1]);
                blt_cs_useFixedDistance(&distance[1], BLC_RANGING_ALGORITHM_2);

                DBG_CS_CHN7_LOW;//test hadm algorithm time post   for lijing

                if(error_code == L4_HADM_HIRES180_OK_1){
                    //L4_HADM_HIRES180_OK_1 is equal to 1, but our success is equal to 0.
                    error_code = CS_DIST_SUCCESS;
                }
            }

            retval = error_code;
        }
    #else
        tlkapi_printf((stkLog_mask & STK_LOG_ALGO_CS), "[CS][DIST] sub node not support CS ALG2\r\n");
        retval = CS_DIST_ERR_RAS_RANGING_DATA_WRONG;
    #endif
    }
    if(algorithmMask & BLC_RANGING_ALGORITHM_3){
        //blt_cs_useFixedDistance(&distance[2], BLC_RANGING_ALGORITHM_3);
        tlkapi_printf((stkLog_mask & STK_LOG_ALGO_CS), "[CS][DIST] sub node not support CS ALG3\r\n");
        retval = CS_DIST_ERR_RAS_RANGING_DATA_WRONG;
    }

    if(algorithmMask == 0){
        retval = CS_DIST_ERR_ALGO_MASK_NOT_SET;
    }

    return retval;
}

#if (CS_DEBUG_MODE)
typedef void (*blt_print_rasLog_callback_t)(blt_ras_proc_ctrl_t *procCtrlInitiator, blt_ras_proc_ctrl_t *procCtrlReflector);
blt_print_rasLog_callback_t blt_print_rasLog_cb = NULL;

void blt_print_rasLog(blt_ras_proc_ctrl_t *procCtrlInitiator, blt_ras_proc_ctrl_t *procCtrlReflector)
{
      blt_ras_proc_ctrl_t *initiaProcCtrl = procCtrlInitiator;
      blt_ras_proc_ctrl_t *reflectProcCtrl = procCtrlReflector;
      int len = 0;
      app_parse_printf("\n{\"title\":\"hci_data\",\"data_init\":\"");
      len = 0;
      while(len < initiaProcCtrl->proc.dataLen){
          u8 print_len = 0;
          if((len + 96) < initiaProcCtrl->proc.dataLen)
          {
              print_len = 96;
          }
          else
          {
              print_len = initiaProcCtrl->proc.dataLen - len;
          }
          app_parse_printf("%s",json_hex_to_str(initiaProcCtrl->proc.pData + len, print_len));

          len = print_len + len;
      }
      app_parse_printf("\",\"data_reflt\":\"");
      len = 0;
      while(len < reflectProcCtrl->proc.dataLen){
          u8 print_len = 0;
          if((len + 96) < reflectProcCtrl->proc.dataLen)
          {
              print_len = 96;
          }
          else
          {
              print_len = reflectProcCtrl->proc.dataLen - len;
          }
          app_parse_printf("%s",json_hex_to_str(reflectProcCtrl->proc.pData + len, print_len));
          len = print_len + len;
      }
      app_parse_printf("\"}\n");
}
#endif

void blc_cs_initRangingLog(void) {
    #if(CS_DEBUG_MODE)
    blt_print_rasLog_cb = blt_print_rasLog;
    #endif
}

/**
 * @brief       calculate distance
 * @param[in]   connHandle
 * @param[in]   *procCtrlInitiator: Initiator procedure data buffer.
 * @param[in]   *procCtrlReflector: Reflector procedure data buffer.
 * @param[in]   mainMode: procedure main mode
 * @param[out]  pointer to distance.
 * @return      errcode
 */
s32 csCalculateDistance(u16 connHandle, blt_ras_proc_ctrl_t *procCtrlInitiator, blt_ras_proc_ctrl_t *procCtrlReflector,
                        u8 mainMode ,float *distance)
{
    s32 retval = 0xFFFFFF;
    if(algorithmMask & BLC_RANGING_ALGORITHM_1){
        if(mainMode == STEP_MODE_1){
            retval = csStepExtraMode1(connHandle, procCtrlInitiator, procCtrlReflector, &distance[0]);
        }else if(mainMode == STEP_MODE_2){
            retval = csStepExtraMode2(connHandle, procCtrlInitiator, procCtrlReflector, &distance[0]);
        }
        blt_cs_useFixedDistance(&distance[0], BLC_RANGING_ALGORITHM_1);
    }
    if(algorithmMask & BLC_RANGING_ALGORITHM_2)
    {
    #if(CS_TLK_ALGO2_EN)
        // get lamda format subevent data based on localProcCtrl and remoteProcCtrl,mode2 4path 72chn 65ms,max 100ms,depend on the IQ data(96M)
        // check the remaining time of the ALGO, mode2 4path 72chn 65ms
        u8 margin_enough = 1;
        #if(PRE_CHECK_ALGO2_TIME) // not stable now, default close
            margin_enough = blt_cs_checkAlgoMargin(connHandle);
        #endif
        if(margin_enough){
            retval = blc_cs_tlk_2_lamda(procCtrlInitiator,procCtrlReflector,mainMode,&distance[1]);
            blt_cs_useFixedDistance(&distance[1], BLC_RANGING_ALGORITHM_2);
        }
    #endif
    }
    if(algorithmMask & BLC_RANGING_ALGORITHM_3)
    {
        retval = csStepExtraMode2_algo3(connHandle, procCtrlInitiator, procCtrlReflector, &distance[2]);
        // cs algo3 don't need fixed offset
        #if(0)
        blt_cs_useFixedDistance(&distance[2], BLC_RANGING_ALGORITHM_3);
        #endif
    }
    if(algorithmMask == 0){
        retval = CS_DIST_ERR_ALGO_MASK_NOT_SET;
    }


    #if (CS_DEBUG_MODE) // enable macro CS_DEBUG_MODE will output ras log
    if (blt_print_rasLog_cb) {
        blt_print_rasLog_cb(procCtrlInitiator, procCtrlReflector);
    }
    #endif

    return retval;
}

u32 blc_restoreProcedureData(u16 connHandle, blt_ras_proc_ctrl_t *remoteProcedureCtrl,blt_ras_proc_ctrl_t *localProcedureCtrl, u8 *pRangingData){

    blc_rasc_ranging_data_evt_t *rangingData = (blc_rasc_ranging_data_evt_t *)pRangingData;
    u16 rangingCounter = rangingData->rangingCounter;

    u8 failedCode = 0;
    if((rangingData->rangingData==NULL)||(rangingData->rangingDataLen==0)) {
        failedCode = 1;
        goto failed;
    }

    //INPUT : remote protocol data
    blt_ras_proc_ctrl_t remoteProtCtrl;
    remoteProtCtrl.proc.pData = rangingData->rangingData;
    remoteProtCtrl.proc.dataLen = rangingData->rangingDataLen;
    u16 extractedRangingCounter = blc_ras_extractRangingCounter(remoteProtCtrl.proc.pData);
    remoteProtCtrl.rangingCounter = extractedRangingCounter;

    if(remoteProtCtrl.rangingCounter != rangingCounter){
        failedCode = 2;
        goto failed;
    }

    //INPUT : local protocol data
    blt_ras_dataset_t* localRasDataset = blc_ras_getDataset(connHandle);
    blt_ras_proc_ctrl_t *localProcCtrl = blc_getLocalProcedureData(connHandle, rangingCounter);
    if(!localProcCtrl) {
        failedCode = 3;
        goto failed;
    }

    //OUTPUT : remote procedure data
    remoteProcedureCtrl->proc.pData = gProcedureBuf;
    remoteProcedureCtrl->proc.dataLen = 0;

    //OUTPUT : local procedure data
    localProcedureCtrl->proc.pData = &gProcedureBuf[PROCEDURE_DATA_LEN];
    localProcedureCtrl->proc.dataLen = 0;

    //clean buffer, can be remove
    smemset(gProcedureBuf, 0, sizeof(gProcedureBuf));


    u8 *readPtr_Remote  = (u8 *)(remoteProtCtrl.proc.pData);
    u8 *writePtr_Remote = (u8 *)(remoteProcedureCtrl->proc.pData);
    u8 *readPtr_Local   = (u8 *)(localProcCtrl->proc.pData);
    u8 *writePtr_Local  = (u8 *)(localProcedureCtrl->proc.pData);

    if((!readPtr_Remote) || (!writePtr_Remote)||(!readPtr_Local) || (!writePtr_Local)) {
        failedCode = 4;
        goto failed;
    }

    blc_rass_prot_head_t *procedureHeadLocal = (blc_rass_prot_head_t *)(readPtr_Local);

    u8 configId = procedureHeadLocal->data.proCountCfgID;
    u8 numAntennaPaths = procedureHeadLocal->data.numAntennaPaths;

    //we use this function to decode remote data and due to that
    //we reverse the role, as the remote role will be the opposite of the local one
    u8 role = (localRasDataset->config[configId].role) ? CS_CONFIG_INITIATOR_ROLE : CS_CONFIG_REFLECTOR_ROLE;
    u8 rtt_type = localRasDataset->config[configId].rttType;

    //copy remote procedure header
    STR_TO_STREAM(writePtr_Remote, readPtr_Remote, PROCEDURE_HEAD_LEN - 1);
    U8_TO_STREAM(writePtr_Remote, (numAntennaPaths & 0x3F));
    readPtr_Remote += PROCEDURE_HEAD_LEN;

    //copy local procedure header
    STR_TO_STREAM(writePtr_Local, readPtr_Local, PROCEDURE_HEAD_LEN);
    readPtr_Local += PROCEDURE_HEAD_LEN;

    u8 step_or_abort_err_flag = 0;

    //while protocol procedure data remains
    while((readPtr_Remote - remoteProtCtrl.proc.pData) < remoteProtCtrl.proc.dataLen)
    {
        //a new subevent start
        remoteProcedureCtrl->subEvtNum++;
        localProcedureCtrl->subEvtNum++;

        blc_rass_data_body_t *subeventRemoteHead = (blc_rass_data_body_t *)(readPtr_Remote);
        blc_rass_data_body_t *subeventLocalHead = (blc_rass_data_body_t *)(readPtr_Local);
        u8 stepsRemoteNum = subeventRemoteHead->numStepsReported;
        u8 stepsLocalNum = subeventLocalHead->numStepsReported;

        if((stepsRemoteNum != stepsLocalNum) || (subeventRemoteHead->subeventDoneStatus == CS_SUBEVT_ABORT)||(subeventLocalHead->subeventDoneStatus == CS_SUBEVT_ABORT)){
            readPtr_Remote += SUBEVENT_HEAD_LEN;
            readPtr_Local += SUBEVENT_HEAD_LEN;

            step_or_abort_err_flag = 1;
            tlkapi_printf((stkLog_mask & STK_LOG_ALGO_CS), "[CS][PROC][DATA] abort(0x):%x,%x,%x,%x,%x\r\n", rangingCounter, subeventRemoteHead->subeventDoneStatus, subeventLocalHead->subeventDoneStatus, subeventRemoteHead->procedureDoneStatus, subeventLocalHead->procedureDoneStatus);debugwait();
            if(subeventRemoteHead->subeventDoneStatus != CS_SUBEVT_ABORT){
                for(int i = 0; i < stepsRemoteNum; i++){
                    blc_rass_step_head_t *stepMetaDataReadPtr = (blc_rass_step_head_t *)readPtr_Remote;
                    u8 modeRemote = stepMetaDataReadPtr->data.mode;
                    readPtr_Remote += sizeof(blc_rass_step_head_t);
                    //after decompression, the length is always the full unfiltered length
                    u8 unpackedLen = blc_ras_getStepLength(modeRemote, role, rtt_type, numAntennaPaths);
                    #if (RAS_STEP_FILTER)
                        u8 unpackedStep[35];
                        u8 filteredLen = blt_rasc_unpackStepFilter(&localRasDataset->filter, modeRemote, role, rtt_type, numAntennaPaths, readPtr_Remote, unpackedStep);
                        readPtr_Remote += filteredLen;
                    #else
                        readPtr_Remote += unpackedLen;
                    #endif
                }
            }
            if(subeventLocalHead->subeventDoneStatus != CS_SUBEVT_ABORT){
                for(int i = 0; i < stepsLocalNum; i++){
                    cs_step_value_t *stepValueLocal = (cs_step_value_t *)(readPtr_Local);
                    u8 lenLocal = stepValueLocal->len;
                    readPtr_Local += sizeof(cs_step_value_t) + lenLocal;
                }
            }
        }
        else{
            STR_TO_STREAM(writePtr_Remote, readPtr_Remote, SUBEVENT_HEAD_LEN);
            readPtr_Remote += SUBEVENT_HEAD_LEN;
            STR_TO_STREAM(writePtr_Local, readPtr_Local, SUBEVENT_HEAD_LEN);
            readPtr_Local += SUBEVENT_HEAD_LEN;

            for(int i = 0; i < stepsRemoteNum; i++){
                //local
                cs_step_value_t *stepValueLocal = (cs_step_value_t *)(readPtr_Local);
                u8 channel = stepValueLocal->channel;
                u8 lenLocal = stepValueLocal->len;
                STR_TO_STREAM(writePtr_Local, readPtr_Local, sizeof(cs_step_value_t) + lenLocal);
                readPtr_Local += sizeof(cs_step_value_t) + lenLocal;

                //remote
                blc_rass_step_head_t *stepMetaDataReadPtr = (blc_rass_step_head_t *)readPtr_Remote;
                u8 modeRemote = stepMetaDataReadPtr->data.mode;
                readPtr_Remote += sizeof(blc_rass_step_head_t);
                u8 unpackedLen = blc_ras_getStepLength(modeRemote, role, rtt_type, numAntennaPaths);

                cs_step_value_t *stepValueRemote = (cs_step_value_t *) writePtr_Remote;
                stepValueRemote->mode = modeRemote;
                stepValueRemote->channel = channel;
                stepValueRemote->len = unpackedLen;
                writePtr_Remote +=sizeof(cs_step_value_t);

                #if (RAS_STEP_FILTER)
                    u8 unpackedStep[35];
                    u8 filteredLen = blt_rasc_unpackStepFilter(&localRasDataset->filter, modeRemote, role, rtt_type, numAntennaPaths, readPtr_Remote, unpackedStep);
                    STR_TO_STREAM(writePtr_Remote, unpackedStep, unpackedLen);
                    readPtr_Remote += filteredLen;
                #else
                    STR_TO_STREAM(writePtr_Remote, readPtr_Remote, unpackedLen);
                    readPtr_Remote += unpackedLen;
                #endif
            }
        }
    }
    remoteProcedureCtrl->proc.dataLen = writePtr_Remote - remoteProcedureCtrl->proc.pData;
    localProcedureCtrl->proc.dataLen = writePtr_Local - localProcedureCtrl->proc.pData;

    if ((remoteProtCtrl.proc.pData + remoteProtCtrl.proc.dataLen) != readPtr_Remote) {
        tlkapi_printf((stkLog_mask & STK_LOG_ALGO_CS), "[CS][PROC][DATA]Remote Protocol len difference! Start\r\n");debugwait();
        failedCode = 5;
        goto failed;
    }
    if ((localProcCtrl->proc.pData + localProcCtrl->proc.dataLen) != readPtr_Local) {
        tlkapi_printf((stkLog_mask & STK_LOG_ALGO_CS), "[CS][PROC][DATA]Local Protocol len difference! Start\r\n");debugwait();
        failedCode = 6;
        goto failed;
    }
    if (step_or_abort_err_flag) {
        failedCode = 7;
        goto failed;
    }

    return BLE_SUCCESS;

failed:
    tlkapi_printf((stkLog_mask & STK_LOG_ALGO_CS), "[CS][PROC][DATA] Protocol data restore abnormal,failedCode=%d\r\n",failedCode);debugwait();
    return HCI_ERR_UNKNOWN_CONN_ID;
}

/**
 * @brief       to get local procedure data,include channel
 * @param[in]   connHandle
 * @param[in]   rangingCounter
 * @return      pointer to local procedure data buffer.
 */
blt_ras_proc_ctrl_t *blc_getLocalProcedureData(u16 connHandle, u16 rangingCounter){
    blt_ras_dataset_t* localRasDataset = blc_ras_getDataset(connHandle);
    blt_rass_procedure_query_result_t res = blt_rass_procedureQuery(localRasDataset, rangingCounter);
    return res.procData;
}


/**
 * @brief       enable cs algorithm mask
 * @param[in]   algorithmMask: cs algorithm mask
 * @return      none
 */
void blc_cs_enableAlgoMask(u32 mask)
{
    algorithmMask = mask;
}
/**
 * @brief       add cs algorithm mask
 * @param[in]   algorithmMask: cs algorithm mask
 * @return      none
 */
void blc_cs_addAlgoMask(u32 mask)
{
    algorithmMask |= mask;
}

/**
 * @brief       remove cs algorithm mask
 * @param[in]   algorithmMask: cs algorithm mask
 * @return      none
 */
void blc_cs_removeAlgoMask(u32 mask)
{
    algorithmMask &= ~mask;
}

/**
 * @brief       get cs algorithm mask
 * @param[in]   none
 * @return      algorithmMask: cs algorithm mask
 */
u32 blc_cs_getAlgoMask(void)
{
    return algorithmMask;
}


#define FLASH_ADDRESS_FIXED_OFFSET                 0xB0000
typedef struct __attribute__((packed))
{
    s16 flashOffsetAlgo1;
    s16 flashOffsetAlgo2;
    s16 flashOffsetAlgo3;
    float offset[3]; // cs distance offset apply to different algorithm
} algoFixedOffset_t;
algoFixedOffset_t algoFixedOffset = {
    .flashOffsetAlgo1 = 0,
    .flashOffsetAlgo2 = 0,
    .flashOffsetAlgo3 = 0,
#if(MCU_CORE_TYPE == MCU_CORE_TL721X)
    .offset = {0.5, 2.5, 0.0},
#else
    .offset = {0.5, 0.5, 0.0},
#endif
};

static void blt_cs_loadFixedOffsetFromFlash(void)
{
    algoFixedOffset_t read_offset = {0};
    flash_read_page(FLASH_ADDRESS_FIXED_OFFSET, sizeof(s16)*3, (u8 *)&read_offset);
    tlkapi_printf(0, "[APP][CS] Read offset from flash, algo1 offset:%dcm, algo2 offset:%dcm, algo3 offset:%dcm",
                  read_offset.flashOffsetAlgo1, read_offset.flashOffsetAlgo2, read_offset.flashOffsetAlgo3);
    if(abs(read_offset.flashOffsetAlgo1) <= 500 && read_offset.flashOffsetAlgo1 != -1){
        algoFixedOffset.offset[0] = read_offset.flashOffsetAlgo1/100.0f;
    }
    if(abs(read_offset.flashOffsetAlgo2) <= 500 && read_offset.flashOffsetAlgo2 != -1){
        algoFixedOffset.offset[1] = read_offset.flashOffsetAlgo2/100.0f;
    }
    if(abs(read_offset.flashOffsetAlgo3) <= 500 && read_offset.flashOffsetAlgo3 != -1){
        algoFixedOffset.offset[2] = read_offset.flashOffsetAlgo3/100.0f;
    }
    tlkapi_printf(0, "[APP][CS] Use fixed offset algo1 offset:%f, algo2 offset:%f, algo3 offset:%f",
                  algoFixedOffset.offset[0], algoFixedOffset.offset[1], algoFixedOffset.offset[2]);
}

static void blt_cs_useFixedDistance(float *dist, blc_ranging_algorithm_enum mask){
    u8 idx = BIT_LOW_BIT(mask);
    float offset = algoFixedOffset.offset[idx];
    float random_dist = (trng_rand() % 10 + 10) / 100.0f;
    *dist = (*dist - offset < 0.0f) ? random_dist : (*dist - offset);
}

/**
 * @brief       Initialize internal delay
 * @param[in]   none
 * @return      none
 */
void blc_cs_initInternalDelay(void){
    blt_cs_loadFixedOffsetFromFlash();
}

