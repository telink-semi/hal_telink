#include "../hadm.h"
#include "tl_common.h"
#include "stack/ble/ble.h"


#define MIN_VAL(m1, m2)         ((m1) < (m2) ? (m1) : (m2))
#define MAX_VAL(m1, m2)         ((m1) > (m2) ? (m1) : (m2))
#define REDEF_LOG_EN            (0)
#define MODE2_QUALITY_THREAD    (2)
/*
 * extracting arithmetic progression
 * input array is an incremental array
 * ans is equal, take the smaller d. If both ans and d agree, it doesn't matter.
 */
int extrArithSeq(u8* input, int in_total_len, u8* output)
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

u8 reflectStepCtrl[2048] = {0};
u8 initiaStepCtrl[2048] = {0};
#if 1
void csStepMode2Antenna(cs_step_ctrl_t *initiaDes, cs_step_ctrl_t *refDes,
                        int j, u8 channel, u16 algDataCnt, int *initiaAlgdata, int *refAlgdata)
{
    static u8 iniToneQuality = MODE2_QUALITY_THREAD;
    static u8 refToneQuality = MODE2_QUALITY_THREAD;
    static u16 iniQualityIndex = 0;
    static u16 refQualityIndex = 0;
    static u16 stepIndex = 0;

    cs_step_mode2_t *initiaData = (cs_step_mode2_t *)(initiaDes->dataCtrl[j].stepStart+3);
    cs_step_mode2_t *refData    = (cs_step_mode2_t *)(refDes->dataCtrl[j].stepStart+3);

    //tlkapi_printf(APP_CS_DIST_EN, "start toneQuality %d, %d, %d, %d", toneQuality, firstFlag, modeIndex, qualityIndex);

    //Find the corresponding valid channel in all mode2.
    if((channel==initiaDes->dataCtrl[j].stepChannel) && (channel==refDes->dataCtrl[j].stepChannel))
    {
        /* After the csStepQualityScreenMode2(), all the tones here must have a
         * quality less than 2.
         * */
        u8 initQualityBetter = 0;

        //tlkapi_printf(REDEF_LOG_EN, "mode2 target channel %d", channel);
        for(int k=0; k<(initiaDes->dataCtrl[j].antennaPaths+1); k++)    //Each antenna loop judgment includes expanding the tone.
        {
            //tlkapi_send_string_data(APP_CS_DIST_EN, "data: %s", &mode2Data->Tone[k], 4);
            u8 origQual = initiaData->Tone[k].Tone_Quality_Indicator & 0x0F;
            u8 extSlot = (initiaData->Tone[k].Tone_Quality_Indicator & 0xF0) >> 4;

            /*
             * Tone_Quality_Initiator:
             *
             * quality(bit0-3):
             *   - 0x00  Tone quality is good
             *   - 0x01  Tone quality is medium
             *   - 0x02  Tone quality is low
             *   - 0x03  Tone quality is unavailable
             *   - All other values  Reserved for future use
             *
             * extension(bit4-7):
             *   - 0x00 = Not tone extension slot
             *   - 0x01 = Tone extension slot; tone not expected to be present
             *   - 0x02 = Tone extension slot; tone expected to be present
             *   - All other values = Reserved for future use
             **/
            if((0==extSlot) || (2==extSlot))
            {
                if(origQual < iniToneQuality)
                {
                    iniToneQuality = origQual;
                    //stepIndex = j;
                    iniQualityIndex = k;
                    initQualityBetter = 1;
                }
            }
        }

        for(int n=0; n<(refDes->dataCtrl[j].antennaPaths+1); n++)   //Each antenna loop judgment includes expanding the tone.
        {
            //tlkapi_send_string_data(APP_CS_DIST_EN, "data: %s", &mode2Data->Tone[k], 4);
            u8 origQual = refData->Tone[n].Tone_Quality_Indicator & 0x0F;
            u8 extSlot = (refData->Tone[n].Tone_Quality_Indicator & 0xF0) >> 4;

            if((0==extSlot) || (2==extSlot))
            {
                if(origQual < refToneQuality)
                {
                    if(initQualityBetter == 1)
                    {
                        refToneQuality = origQual;
                        stepIndex = j;
                        refQualityIndex = n;
                    }
                }
            }
        }
    }

    //tlkapi_printf(APP_CS_DIST_EN, "end toneQuality %d, firstFlag %d, %d, %d, %d, %d", toneQuality, firstFlag, modeIndex, qualityIndex, j, k);
    if(j == (initiaDes->stepNums-1))    //Extract I-sample and Q-sample after all mode2 have been traversed completely.
    {
        cs_step_mode2_t *modeData = (cs_step_mode2_t *)(initiaDes->dataCtrl[stepIndex].stepStart+3);
        u32 tonePCTData = 0;
        BYTE_TO_UINT24(tonePCTData ,&modeData->Tone[iniQualityIndex].Tone_PCT[0]);
        int iSample = ((tonePCTData & BIT_RNG(0, 10)) | ((tonePCTData & BIT(11)) ? BIT_RNG(11,31) : 0));
        int qSample = (((tonePCTData & BIT_RNG(12, 22))>>12) | (((tonePCTData & BIT(23))>>12) ? BIT_RNG(11,31) : 0));
        initiaAlgdata[algDataCnt] = iSample;
        initiaAlgdata[algDataCnt+1] = qSample;

        modeData = (cs_step_mode2_t *)(refDes->dataCtrl[stepIndex].stepStart+3);
        BYTE_TO_UINT24(tonePCTData ,&modeData->Tone[refQualityIndex].Tone_PCT[0]);
        iSample = ((tonePCTData & BIT_RNG(0, 10)) | ((tonePCTData & BIT(11)) ? BIT_RNG(11,31) : 0));
        qSample = (((tonePCTData & BIT_RNG(12, 22))>>12) | (((tonePCTData & BIT(23))>>12) ? BIT_RNG(11,31) : 0));
        refAlgdata[algDataCnt] = iSample;
        refAlgdata[algDataCnt+1] = qSample;

        //tlkapi_printf(REDEF_LOG_EN, "extract data: channel=%d, stepIndex=%d, iniQualityIndex=%d, refQualityIndex=%d, algDataCnt=%d",
                                                   //channel,    stepIndex,    iniQualityIndex,    refQualityIndex,    algDataCnt);
        //tlkapi_printf(APP_CS_DIST_EN, "Data: iSample = %+.d, qSample = %+.d", iSample, qSample);

        iniToneQuality = MODE2_QUALITY_THREAD;
        refToneQuality = MODE2_QUALITY_THREAD;
        iniQualityIndex = 0;
        refQualityIndex = 0;
        stepIndex = 0;
    }
}
#else
void csStepMode2Antenna(cs_step_ctrl_t *modeDes, cs_step_mode2_t *mode2Data, int j, u8 channel, u16 algDataCnt, int *alg_data)
{
    static u8 toneQuality = MODE2_QUALITY_THREAD;
    static u16 qualityIndex = 0;
    static u16 stepIndex = 0;

    //tlkapi_printf(APP_CS_DIST_EN, "start toneQuality %d, %d, %d, %d", toneQuality, firstFlag, modeIndex, qualityIndex);

    if(channel == modeDes->dataCtrl[j].stepChannel) //Find the corresponding valid channel in all mode2.
    {
        /* After the csStepQualityScreenMode2(), all the tones here must have a
         * quality less than 2.
         * */

        //tlkapi_printf(APP_CS_DIST_EN, "mode2 target channel %d", channel);
        for(int k=0; k<(modeDes->dataCtrl[j].antennaPaths+1); k++)  //Each antenna loop judgment includes expanding the tone.
        {
            //tlkapi_send_string_data(APP_CS_DIST_EN, "data: %s", &mode2Data->Tone[k], 4);
            u8 origQual = mode2Data->Tone[k].Tone_Quality_Indicator & 0x0F;
            u8 extSlot = (mode2Data->Tone[k].Tone_Quality_Indicator & 0xF0) >> 4;

            /*
             * Tone_Quality_Initiator:
             *
             * quality(bit0-3):
             *   - 0x00  Tone quality is good
             *   - 0x01  Tone quality is medium
             *   - 0x02  Tone quality is low
             *   - 0x03  Tone quality is unavailable
             *   - All other values  Reserved for future use
             *
             * extension(bit4-7):
             *   - 0x00 = Not tone extension slot
             *   - 0x01 = Tone extension slot; tone not expected to be present
             *   - 0x02 = Tone extension slot; tone expected to be present
             *   - All other values = Reserved for future use
             **/
            if((0==extSlot) || (2==extSlot))
            {
                if(origQual < toneQuality)
                {
                    toneQuality = origQual;
                    stepIndex = j;
                    qualityIndex = k;
                }
            }
        }
    }

    //tlkapi_printf(APP_CS_DIST_EN, "end toneQuality %d, firstFlag %d, %d, %d, %d, %d", toneQuality, firstFlag, modeIndex, qualityIndex, j, k);

    if(j == (modeDes->stepNums-1))  //Extract I-sample and Q-sample after all mode2 have been traversed completely.
    {
        cs_step_mode2_t *modeData = (cs_step_mode2_t *)(modeDes->dataCtrl[stepIndex].stepStart+3);
        u32 tonePCTData = 0;
        BYTE_TO_UINT24(tonePCTData ,&modeData->Tone[qualityIndex].Tone_PCT[0]);

        int iSample = ((tonePCTData & BIT_RNG(0, 10)) | ((tonePCTData & BIT(11)) ? BIT_RNG(11,31) : 0));
        int qSample = (((tonePCTData & BIT_RNG(12, 22))>>12) | (((tonePCTData & BIT(23))>>12) ? BIT_RNG(11,31) : 0));
        alg_data[algDataCnt] = iSample;
        alg_data[algDataCnt+1] = qSample;
        //tlkapi_printf(tlkapi_printf, "extract data: stepIndex = %d, qualityIndex = %d, tonePCTData = %x, I = %d, Q = %d", stepIndex, qualityIndex, tonePCTData, iSample, qSample);
        //tlkapi_printf(APP_CS_DIST_EN, "Data: iSample = %+.d, qSample = %+.d", iSample, qSample);

        toneQuality = MODE2_QUALITY_THREAD;
        qualityIndex = 0;
        stepIndex = 0;
    }
}
#endif

void csStepDataMode2(u8 *chn_mask, int chn_len, int *ipm_data, int *pct_data)
{
    cs_step_ctrl_t *initiaDes = (cs_step_ctrl_t *)initiaStepCtrl;
    cs_step_ctrl_t *rflDes = (cs_step_ctrl_t *)reflectStepCtrl;

#if 1
    u16 algDataCnt = 0;
    for(int i = 0; i < chn_len; i++) //The total number of all valid channels.
    {
        for(int j = 0; j < initiaDes->stepNums; j++) //The extraction of high-quality total numbers in mode 2.
        {
            csStepMode2Antenna(initiaDes, rflDes, j, chn_mask[i], algDataCnt, ipm_data, pct_data);
        }
        algDataCnt += 2;
    }
#else
    tlkapi_printf(APP_CS_DIST_EN, "initiaDes---------------------------------------");
    u16 algDataCnt = 0;
    for(int i = 0; i < chn_len; i++)    //The total number of all valid channels.
    {
        for(int j = 0; j < initiaDes->stepNums; j++)    //The extraction of high-quality total numbers in mode 2.
        {
            cs_step_mode2_t *initiaData = (cs_step_mode2_t *)(initiaDes->dataCtrl[j].stepStart+3);
            csStepMode2Antenna(initiaDes, initiaData, j, chn_mask[i], algDataCnt, ipm_data);
        }
        algDataCnt += 2;
    }

    tlkapi_printf(APP_CS_DIST_EN, "rflDes---------------------------------------");
    algDataCnt = 0;
    for(int i = 0; i < chn_len; i++)    //The total number of all valid channels.
    {
        for(int j = 0; j < rflDes->stepNums; j++)   //The extraction of high-quality total numbers in mode 2.
        {
            cs_step_mode2_t *rflData = (cs_step_mode2_t *)(rflDes->dataCtrl[j].stepStart+3);
            csStepMode2Antenna(rflDes, rflData, j, chn_mask[i], algDataCnt, pct_data);
        }
        algDataCnt += 2;
    }
#endif
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

        //tlkapi_printf(APP_CS_DIST_EN, "Packet_Quality= %d", rflData->Packet_Quality);

        //tlkapi_printf(APP_CS_DIST_EN, "Channel = %d, %d", rflDes->dataCtrl[i].stepChannel, initiaDes->dataCtrl[i].stepChannel);
        if(rflDes->dataCtrl[i].stepChannel != initiaDes->dataCtrl[i].stepChannel)
        {
            //TODO: cxh
            //tlkapi_printf(APP_CS_DIST_EN, "different channel = %d, %d", initiaDes->dataCtrl[i].stepChannel, rflDes->dataCtrl[i].stepChannel);
            break;
        }

        if((0 == rflData->Packet_Quality) && (0 == initiaData->Packet_Quality))
        {
            memcpy((u8 *)&rflDes->dataCtrl[availCnt], (u8 *)&rflDes->dataCtrl[i], sizeof(cs_step_mode_t));
            memcpy((u8 *)&initiaDes->dataCtrl[availCnt], (u8 *)&initiaDes->dataCtrl[i], sizeof(cs_step_mode_t));

            rflAlgData[availCnt] = rflData->ToA_ToD[0] | (rflData->ToA_ToD[1] << 8);
            initiaAlgData[availCnt] = initiaData->ToA_ToD[0] | (initiaData->ToA_ToD[1] << 8);
            availCnt++;
        }
    }
    u8 memSetLen = sizeof(cs_step_mode_t)*(rflDes->stepNums - availCnt);
    rflDes->stepNums = availCnt;
    initiaDes->stepNums = availCnt;
    //tlkapi_printf(REDEF_LOG_EN, "initia stepNums = %d", initiaDes->stepNums);
    //tlkapi_printf(REDEF_LOG_EN, "reflect stepNums = %d", rflDes->stepNums);

    memset((u8 *)&rflDes->dataCtrl[availCnt], 0, memSetLen);
    memset((u8 *)&initiaDes->dataCtrl[availCnt], 0, memSetLen);
    return availCnt;
}
#if STEP_INDEX_ENABLE
int csStepQualityScreenMode2(u8 *initiaBuf, u8 *rflBuf, u8 *chnMask)
{
    cs_step_ctrl_t *refDes = (cs_step_ctrl_t *)rflBuf;
    cs_step_ctrl_t *initiaDes = (cs_step_ctrl_t *)initiaBuf;

    u16 availCnt = 0;
    //tlkapi_printf(REDEF_LOG_EN, "initia stepNums = %d, modelen = %d", initiaDes->stepNums, initiaDes->modeLen);
    //tlkapi_printf(REDEF_LOG_EN, "reflect stepNums = %d, modelen = %d", refDes->stepNums, refDes->modeLen);
#if 0
    u8 iniIdex[200] = {0};
    u8 refIdex[200] = {0};
    for(int y=0;y<initiaDes->stepNums;y++)
    {
        iniIdex[y]=initiaDes->dataCtrl[y].stepIndex;
    }
    for(int y=0;y<refDes->stepNums;y++)
    {
        refIdex[y]=refDes->dataCtrl[y].stepIndex;
    }
    tlkapi_send_string_data(REDEF_LOG_EN, "ini idx:", iniIdex, 128);
    tlkapi_send_string_data(REDEF_LOG_EN, "ref idx:", refIdex, 128);

    memset(iniIdex, 0, 128);
    memset(refIdex, 0, 128);
    for(int y=0;y<initiaDes->stepNums;y++)
    {
        iniIdex[y]=initiaDes->dataCtrl[y].stepChannel;
    }
    for(int y=0;y<refDes->stepNums;y++)
    {
        refIdex[y]=refDes->dataCtrl[y].stepChannel;
    }
    tlkapi_send_string_data(REDEF_LOG_EN, "ini channel:", iniIdex, 128);
    tlkapi_send_string_data(REDEF_LOG_EN, "ref channel:", refIdex, 128);
#endif
    int i, j;
    u8 OrigQual = 0;
    u8 ExtSlot = 0;
    u8 refToneValid = 0;
    u8 initiaToneValid = 0;
    int preK = 0;

    /* Traverse all mode2. */
    for(i=0; i<initiaDes->stepNums; i++)
    {
        cs_step_mode2_t *initiaData = (cs_step_mode2_t *)(initiaDes->dataCtrl[i].stepStart+STEP_HEAD_LEN);

        for(int k=preK; k<refDes->stepNums; k++)
        {
            cs_step_mode2_t *refData = (cs_step_mode2_t *)(refDes->dataCtrl[k].stepStart+STEP_HEAD_LEN);

            if(k > i)
            {
                break;
            }

            if(initiaDes->dataCtrl[i].stepIndex == refDes->dataCtrl[k].stepIndex)
            {
                /* 1. different channel, return */
                if(initiaDes->dataCtrl[i].stepChannel == refDes->dataCtrl[k].stepChannel)
                {
                    preK = k;

                    /* 2. initiator step quality. */
                    //tlkapi_printf(REDEF_LOG_EN, "inittiator anrenaPaths = %d",initiaDes->dataCtrl[i].antennaPaths);
                    /* Traverse the number of tones in the data. */
                    initiaToneValid = 0;
                    for(j=0; j<(initiaDes->dataCtrl[i].antennaPaths+1); j++)
                    {
                        OrigQual = initiaData->Tone[j].Tone_Quality_Indicator & 0x0F;
                        ExtSlot = (initiaData->Tone[j].Tone_Quality_Indicator & 0xF0) >> 4;

                        /*
                         * Tone_Quality_Initiator:
                         * quality(bit0-3):
                         *   - 0x00  Tone quality is good
                         *   - 0x01  Tone quality is medium
                         *   - 0x02  Tone quality is low
                         *   - 0x03  Tone quality is unavailable
                         *   - All other values  Reserved for future use
                         *
                         * extension(bit4-7):
                         *   - 0x00 = Not tone extension slot
                         *   - 0x01 = Tone extension slot; tone not expected to be present
                         *   - 0x02 = Tone extension slot; tone expected to be present
                         *   - All other values = Reserved for future use
                         **/
                        if((0==ExtSlot) || (2==ExtSlot))
                        {

                            //if(2==ExtSlot)
                            //{
                                //tlkapi_printf(REDEF_LOG_EN, "step=%d, initiator ExtSlot=%d", i, ExtSlot);
                            //}
                            if(OrigQual < MODE2_QUALITY_THREAD)
                            {
                                initiaToneValid++;
                                break;
                            }
                            else
                            {
                                //tlkapi_printf(REDEF_LOG_EN, "step=%d, initiator step quality = %d, antennaPath=%d, ExtSlot=%d",i, OrigQual, initiaDes->dataCtrl[i].antennaPaths, ExtSlot);
                            }
                        }
                    }

                    /* 3. reflector step quality. */
                    //tlkapi_printf(REDEF_LOG_EN, "reflector anrenaPaths = %d",refDes->dataCtrl[i].antennaPaths);
                    /* Traverse the number of tones in the data. */
                    refToneValid = 0;
                    for(j=0; j<(refDes->dataCtrl[k].antennaPaths+1); j++)
                    {
                        OrigQual = refData->Tone[j].Tone_Quality_Indicator & 0x0F;
                        ExtSlot = (refData->Tone[j].Tone_Quality_Indicator & 0xF0) >> 4;

                        if((0==ExtSlot) || (2==ExtSlot))
                        {
                            //if(2==ExtSlot)
                            //{
                                //tlkapi_printf(REDEF_LOG_EN, "step=%d, reflector ExtSlot=%d", i, ExtSlot);
                            //}

                            if(OrigQual < MODE2_QUALITY_THREAD)
                            {
                                refToneValid++;
                                break;
                            }
                            else
                            {
                                //tlkapi_printf(REDEF_LOG_EN, "step=%d, reflector step quality = %d, antennaPath=%d, ExtSlot=%d", i, OrigQual, refDes->dataCtrl[i].antennaPaths, ExtSlot);
                            }
                        }
                    }

                    if(initiaToneValid && refToneValid)
                    {
                        memcpy((u8 *)&initiaDes->dataCtrl[availCnt], (u8 *)&initiaDes->dataCtrl[i], sizeof(cs_step_mode_t));
                        memcpy((u8 *)&refDes->dataCtrl[availCnt], (u8 *)&refDes->dataCtrl[k], sizeof(cs_step_mode_t));
                        //tlkapi_printf(REDEF_LOG_EN, "chl = %d\n", rflDes->dataCtrl[availCnt].stepChannel);
                        chnMask[initiaDes->dataCtrl[availCnt].stepChannel/8] |= BIT(initiaDes->dataCtrl[availCnt].stepChannel%8);
                        availCnt++;
                        //break;
                    }
                    break;
                } // if(initiaDes->dataCtrl[i].stepChannel == refDes->dataCtrl[k].stepChannel)
                else
                {
                    //tlkapi_printf(REDEF_LOG_EN, "different channel: , initiator=%d, %d, reflector=%d,%d", i, initiaDes->dataCtrl[i].stepChannel, k,refDes->dataCtrl[i].stepChannel);
                    //tlkapi_printf(REDEF_LOG_EN, "different channel: , initiator: step:%d, Index:%d, channel:%d, "
                                                                                    // "reflector: step:%d, Index:%d, channel:%d, ",
                                                                                     // i, initiaDes->dataCtrl[i].stepIndex, initiaDes->dataCtrl[i].stepChannel,
                                                                                     // k, refDes->dataCtrl[k].stepIndex, refDes->dataCtrl[k].stepChannel);
                }
                break;
            } // if(initiaDes->dataCtrl[i].stepIndex == refDes->dataCtrl[k].stepIndex)
            else
            {
                //tlkapi_printf(REDEF_LOG_EN, "different channel: , initiator: step:%d, Index:%d, reflector: step:%d, Index:%d",
                                           //  i, initiaDes->dataCtrl[i].stepIndex, k, refDes->dataCtrl[i].stepIndex);
            }
        } // for(int k=preK; k<refDes->stepNums; k++)
    } //for(i=0; i<initiaDes->stepNums; i++)

    initiaDes->stepNums = availCnt;
    refDes->stepNums    = availCnt;

    //tlkapi_printf(REDEF_LOG_EN, "valid initia stepNums = %d", initiaDes->stepNums);
    //tlkapi_printf(REDEF_LOG_EN, "valid reflect stepNums = %d", refDes->stepNums);
    if(availCnt != 0)
    {
        memset((u8 *)&initiaDes->dataCtrl[availCnt], 0, sizeof(cs_step_mode_t) * (initiaDes->stepNums-availCnt));
        memset((u8 *)&refDes->dataCtrl[availCnt], 0, sizeof(cs_step_mode_t) * (refDes->stepNums-availCnt));
    }
    if(availCnt  <  20)
    {
        tlkapi_printf(REDEF_LOG_EN, "mode2: phaseDiffDistance = valid step number not enough., musicDistance = valid step number not enough.\n");
        availCnt = 0;
    }
    return availCnt;
}
#else
int csStepQualityScreenMode2(u8 *initiaBuf, u8 *rflBuf, u8 *chnMask)
{
    cs_step_ctrl_t *refDes = (cs_step_ctrl_t *)rflBuf;
    cs_step_ctrl_t *initiaDes = (cs_step_ctrl_t *)initiaBuf;

    u16 availCnt = 0;
    //tlkapi_printf(REDEF_LOG_EN, "initia stepNums = %d, modelen = %d", initiaDes->stepNums, initiaDes->modeLen);
    //tlkapi_printf(REDEF_LOG_EN, "reflect stepNums = %d, modelen = %d", refDes->stepNums, refDes->modeLen);

    int i, j;
    u8 OrigQual = 0;
    u8 ExtSlot = 0;
    u8 refToneValid = 0;
    u8 initiaToneValid = 0;

    /* Traverse all mode2. */
    for(i=0; i<initiaDes->stepNums; i++)
    {
        cs_step_mode2_t *initiaData = (cs_step_mode2_t *)(initiaDes->dataCtrl[i].stepStart+3);
        cs_step_mode2_t *refData = (cs_step_mode2_t *)(refDes->dataCtrl[i].stepStart+3);

        /* 1. different channel, return */
        if(refDes->dataCtrl[i].stepChannel != initiaDes->dataCtrl[i].stepChannel)
        {
            //tlkapi_printf(REDEF_LOG_EN, "different channel: step=%d, initiator=%d, reflector=%d", i, initiaDes->dataCtrl[i].stepChannel, refDes->dataCtrl[i].stepChannel);
            break;
        }

        /* 2. initiator step quality. */
        //tlkapi_printf(REDEF_LOG_EN, "inittiator anrenaPaths = %d",initiaDes->dataCtrl[i].antennaPaths);
        /* Traverse the number of tones in the data. */
        initiaToneValid = 0;
        for(j=0; j<(initiaDes->dataCtrl[i].antennaPaths+1); j++)
        {
            OrigQual = initiaData->Tone[j].Tone_Quality_Indicator & 0x0F;
            ExtSlot = (initiaData->Tone[j].Tone_Quality_Indicator & 0xF0) >> 4;

            /*
             * Tone_Quality_Initiator:
             * quality(bit0-3):
             *   - 0x00  Tone quality is good
             *   - 0x01  Tone quality is medium
             *   - 0x02  Tone quality is low
             *   - 0x03  Tone quality is unavailable
             *   - All other values  Reserved for future use
             *
             * extension(bit4-7):
             *   - 0x00 = Not tone extension slot
             *   - 0x01 = Tone extension slot; tone not expected to be present
             *   - 0x02 = Tone extension slot; tone expected to be present
             *   - All other values = Reserved for future use
             **/
            if((0==ExtSlot) || (2==ExtSlot))
            {

                //if(2==ExtSlot)
                //{
                    //tlkapi_printf(REDEF_LOG_EN, "step=%d, initiator ExtSlot=%d", i, ExtSlot);
                //}
                if(OrigQual < MODE2_QUALITY_THREAD)
                {
                    initiaToneValid++;
                    break;
                }
                else
                {
                    //tlkapi_printf(REDEF_LOG_EN, "step=%d, initiator step quality = %d, antennaPath=%d, ExtSlot=%d",i, OrigQual, initiaDes->dataCtrl[i].antennaPaths, ExtSlot);
                }
            }
        }

        /* 3. reflector step quality. */
        //tlkapi_printf(REDEF_LOG_EN, "reflector anrenaPaths = %d",refDes->dataCtrl[i].antennaPaths);
        /* Traverse the number of tones in the data. */
        refToneValid = 0;
        for(j=0; j<(refDes->dataCtrl[i].antennaPaths+1); j++)
        {
            OrigQual = refData->Tone[j].Tone_Quality_Indicator & 0x0F;
            ExtSlot = (refData->Tone[j].Tone_Quality_Indicator & 0xF0) >> 4;

            if((0==ExtSlot) || (2==ExtSlot))
            {
                //if(2==ExtSlot)
                //{
                    //tlkapi_printf(REDEF_LOG_EN, "step=%d, reflector ExtSlot=%d", i, ExtSlot);
                //}

                if(OrigQual < MODE2_QUALITY_THREAD)
                {
                    refToneValid++;
                    break;
                }
                else
                {
                    //tlkapi_printf(REDEF_LOG_EN, "step=%d, reflector step quality = %d, antennaPath=%d, ExtSlot=%d", i, OrigQual, refDes->dataCtrl[i].antennaPaths, ExtSlot);
                }
            }
        }

        if(initiaToneValid && refToneValid)
        {
            memcpy((u8 *)&initiaDes->dataCtrl[availCnt], (u8 *)&initiaDes->dataCtrl[i], sizeof(cs_step_mode_t));
            memcpy((u8 *)&refDes->dataCtrl[availCnt], (u8 *)&refDes->dataCtrl[i], sizeof(cs_step_mode_t));
            //tlkapi_printf(REDEF_LOG_EN, "chl = %d\n", rflDes->dataCtrl[availCnt].stepChannel);
            chnMask[initiaDes->dataCtrl[availCnt].stepChannel/8] |= BIT(initiaDes->dataCtrl[availCnt].stepChannel%8);
            availCnt++;
            //break;
        }
    } //for(i=0; i<initiaDes->stepNums; i++)

    initiaDes->stepNums = availCnt;
    refDes->stepNums    = availCnt;

    //tlkapi_printf(REDEF_LOG_EN, "valid initia stepNums = %d", initiaDes->stepNums);
    //tlkapi_printf(REDEF_LOG_EN, "valid reflect stepNums = %d", refDes->stepNums);
    if(availCnt != 0)
    {
        u8 memSetLen = sizeof(cs_step_mode_t) * (refDes->stepNums-availCnt);
        memset((u8 *)&initiaDes->dataCtrl[availCnt], 0, memSetLen);
        memset((u8 *)&refDes->dataCtrl[availCnt], 0, memSetLen);
    }
    if(availCnt  <  20)
    {
        tlkapi_printf(REDEF_LOG_EN, "mode2: phaseDiffDistance = valid step number not enough., musicDistance = valid step number not enough.\n");
        availCnt = 0;
    }
    return availCnt;
}
#endif

void csSubevntModeExtra(u8 *procStart, u16 procDataLen, u8 *StepCtrl, u8 modeType)
{
    cs_step_ctrl_t *stepDescribe = (cs_step_ctrl_t *)StepCtrl;

    u8 *subEvtStart = procStart+PROCEDURE_HEAD_LEN; /* offset procedure header 4byte */
    s16 procLen = procDataLen-PROCEDURE_HEAD_LEN;
    u16 stepIndex = 0;
    u8 numAntennaPath = ((blc_rass_data_t *)procStart)->numAntennaPaths;


    stepDescribe->stepMode = modeType;

    while(procLen > 0)  //Filter the mode of the procedure regardless of quality.
    {
        blc_rass_data_body_t *subEvtPtr = (blc_rass_data_body_t *)subEvtStart;
        u8 *subDataPtr = (u8 *)subEvtPtr->pSubeventRangingData;
        u16 subEvtLen = 0;
    #if STEP_INDEX_ENABLE
        //tlkapi_printf(REDEF_LOG_EN, "subEvtPtr->subeventIndex = %d", subEvtPtr->subeventIndex);
    #endif

        for(int i = 0; i < subEvtPtr->numStepsReported; i++)
        {
            u32 stepData = 0;
            u8 stepDataLen = 0;
            u8 stepMode = 0;
            BYTE_TO_UINT24(stepData ,subDataPtr);
            stepDataLen = U32_BYTE2(stepData);
            stepMode = U32_BYTE0(stepData);

            if((stepMode&0xf) == modeType)
            {
            #if STEP_INDEX_ENABLE
                if(modeType==2)
                {
                    stepDescribe->dataCtrl[stepIndex].stepIndex  = (subEvtPtr->subeventIndex * 5) + i;
                }
            #endif
                stepDescribe->dataCtrl[stepIndex].antennaPaths = numAntennaPath;
                stepDescribe->dataCtrl[stepIndex].stepChannel = U32_BYTE1(stepData);
                stepDescribe->dataCtrl[stepIndex].stepStart = subDataPtr;
                //tlkapi_send_string_data(APP_CS_DIST_EN, "stepStart %s", subDataPtr-3, 6);
                stepIndex++;

                stepDescribe->stepNums = stepIndex;
                stepDescribe->modeLen = stepDataLen;
                /*tlkapi_printf(APP_CS_DIST_EN, "stepMode = %x, stepNums = %x, modeLen = %x\n",
                        stepDescribe->stepMode,stepDescribe->stepNums, stepDescribe->modeLen);*/
            }

            subDataPtr += (stepDataLen+STEP_HEAD_LEN);  //next step start
            subEvtLen  += (stepDataLen+STEP_HEAD_LEN);  //subevent length count
        }

        subEvtStart += (subEvtLen+SUBEVENT_HEAD_LEN);       //next subevent index start
        procLen     -= (subEvtLen+SUBEVENT_HEAD_LEN);       //procedure total length calculation
    }
}

u32 csStepExtraMode1(u16 procIndex, float *distance1, float *distance2)
{
    memset(initiaStepCtrl, 0, sizeof(initiaStepCtrl));
    memset(reflectStepCtrl, 0, sizeof(reflectStepCtrl));

    cs_step_ctrl_t *initiaDes = (cs_step_ctrl_t *)initiaStepCtrl;
    blc_rass_data_ctrl_t *initiaDataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf;

#if PROCEDURE_RECEIVE_UART
    blc_rass_data_ctrl_t *reflectDataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf_uart;
#endif

    blc_rass_data_ctrl_t *reflectDataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf_ble;

    /* extract data of the same mode in the procedure */
    csSubevntModeExtra(initiaDataCtrl->procDataDes[procIndex].procStartaddr+2, initiaDataCtrl->procDataDes[procIndex].procDataLen-2, initiaStepCtrl, 1);
    if(initiaDes->stepNums == 0)
    {
        return 1;
    }
    csSubevntModeExtra(reflectDataCtrl->procDataDes[procIndex].procStartaddr+2, reflectDataCtrl->procDataDes[procIndex].procDataLen-2, reflectStepCtrl, 1);

    short cte_sync1[CHANNUM] = {0};
    short cte_sync2[CHANNUM] = {0};
    csStepQualityScreenMode1(initiaStepCtrl, reflectStepCtrl, cte_sync1, cte_sync2);

    int nAverage = initiaDes->stepNums;
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

    tlkapi_printf(REDEF_LOG_EN, "mode1: Distance  = %f", distPesSync);

    *distance1 = distPesSync;
    *distance2 = distPesSync;

    return 0;
}

u32 csStepExtraMode2(u16 procIndex, float *distance1, float *distance2)
{
    memset(initiaStepCtrl, 0, sizeof(initiaStepCtrl));
    memset(reflectStepCtrl, 0, sizeof(reflectStepCtrl));

    cs_step_ctrl_t *initiaDes = (cs_step_ctrl_t *)initiaStepCtrl;
    blc_rass_data_ctrl_t *initiaDataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf;

#if PROCEDURE_RECEIVE_UART
    blc_rass_data_ctrl_t *reflectDataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf_uart;
#endif

    blc_rass_data_ctrl_t *reflectDataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf_ble;

    /* extract data of the same mode in the procedure */
    csSubevntModeExtra(initiaDataCtrl->procDataDes[procIndex].procStartaddr+2, initiaDataCtrl->procDataDes[procIndex].procDataLen-2, initiaStepCtrl, 2);// offset recordNumber 4byte
    if(initiaDes->stepNums == 0)
    {
        return 1;
    }

#if PROCEDURE_RECEIVE_UART
    csSubevntModeExtra(reflectDataCtrl->procDataDes[procIndex].procStartaddr+2, reflectDataCtrl->procDataDes[procIndex].procDataLen-2, reflectStepCtrl, 2);
#endif

    csSubevntModeExtra(reflectDataCtrl->procDataDes[procIndex].procStartaddr+2, reflectDataCtrl->procDataDes[procIndex].procDataLen-2, reflectStepCtrl, 2);

    //tlkapi_printf(APP_CS_DIST_EN, "TEST initiaData len = %d, reflectData len = %d", initiaDataCtrl->procDataDes[procIndex].procDataLen, reflectDataCtrl->procDataDes[procIndex].procDataLen);

    //quality screening of mode
    u8 chn_mask[10] = {0};
    u8 res = csStepQualityScreenMode2(initiaStepCtrl, reflectStepCtrl, chn_mask);
    //tlkapi_send_string_data(REDEF_LOG_EN, "TEST chn_mask: %s", chn_mask, 10);
    if(res == 0)
    {
        /* if there is no valid channel, algorithm will lead to a system crash.
         * (2023.10.31)
         * */
        return 2;
    }

    //perform differential filtering on all channels
    u8 chn_data[80] = {0};
    u8 data_out[80] = {0};
    u8 data_len = csStepChannelMask(chn_mask, chn_data);
    //tlkapi_send_string_data(REDEF_LOG_EN, "TEST chn_data: %s", chn_data, 80);

    int len = extrArithSeq(chn_data, data_len, data_out);
    //tlkapi_send_string_data(REDEF_LOG_EN, "TEST data_out: %s", data_out, 80);
    //tlkapi_printf(REDEF_LOG_EN, "ArithSeq stepNums = %d", len);
    //select high-quality tone data: I-sample Q-sample
#if 0
    /* validation (data provided by Li Jing) */
    u8 data_out[80]={21, 23, 25, 27};
    int len = 4;
    //int ipm[4*2] = {-969,1261,-379 ,1544 , 849,-1338 , 705, 1405};
    //int pct[4*2] = {-1426 ,-842 , -257,-1631 ,1512 , -647, -1034, -1280};
    int ipm[4*2] = {1456,-543 ,1270 , 960, -1329, -875, 1586, 29};
    int pct[4*2] = {1542 , 468, 495, -1579, -1212, 1119, 1361, -923};
#else
    int ipm[80*2] = {0};
    int pct[80*2] = {0};
    csStepDataMode2(data_out, len, ipm, pct);
#endif

//  float distanceReal = 4.31803;
    float distPhaseDiff, distMusic, likeliness, EVDCap,T2WRDiffMean;
    int  nIterMaxEig, nIterPS, nSigCnt;
    complex ipmpct[80];
//  complex tmp;
    //result of 0m, conj(I*Q)
    int cali[80*2] = {0};
    for(int m = 0; m < 80; m++)
    {
        cali[2*m] = 1024;
        cali[2*m+1] = 0;
    }
    //int channum = len;// phaseDiffDistance = 5.806468, musicDistance = 5.745534
    float fstep = 1e6;
    fstep = fstep*(data_out[1]-data_out[0]);
    parameterConstTes para = tesInit(len,fstep);

    calcIpmPct(ipm, pct, cali, ipmpct, para);
    //tlkapi_printf(APP_CS_DIST_EN, "TEST ipmpct: %d, %d, %s",len ,data_out[1]-data_out[0], hex_to_str(ipmpct, 80));

    //phase based ranging
    float T2WR[80];
    distPhaseDiff = tesPhase(ipmpct, T2WR, &(T2WRDiffMean), para);
    distMusic= tesMusic(ipmpct, T2WR, T2WRDiffMean, para, &(likeliness), &(nIterMaxEig), &(nIterPS), &(nSigCnt), &(EVDCap));

    // >5m,  10%
    // <=5m, 0.5m

//  float realDistance = 7.18247;

    tlkapi_printf(REDEF_LOG_EN, "mode2: phaseDiffDistance = %f, musicDistance = %f\n", distPhaseDiff, distMusic);

    //tlkapi_send_string_data(REDEF_LOG_EN, "TEST ipm: %s", ipm, 80*2);
    //tlkapi_send_string_data(REDEF_LOG_EN, "TEST pct: %s", pct, 80*2);
    *distance1 = distPhaseDiff;
    *distance2 = distMusic;

    return 0;
}

u32 csCalculateDistance(u16 procIndex, float *distance1, float *distance2)
{
    u32 retval = 0xFFFFFFFF;

    retval = csStepExtraMode1(procIndex, distance1, distance2);
    if(retval == 0)
    {
        return retval;
    }

    retval = csStepExtraMode2(procIndex, distance1, distance2);

    return retval;
}






