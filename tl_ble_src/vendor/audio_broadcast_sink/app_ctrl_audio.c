/********************************************************************************************************
 * @file    app_ctrl_audio.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "app_ctrl_audio.h"
#include "vendor/common/tlk_api/tlk_codec.h"
#include "algorithm/audio_alg/lc3/lc3.h"

///////////////////////audio control LC3 packet and push to DMA///////////////////////
int recvWptr = 0;
int recvRptr = 0;
u32 currentTimestamp = 0;
appSinkRecvSpeak_t recvAudioBuff[APP_SINK_RECV_SPEAK_FRAME_COUNT];

appSinkInfo_t appSinkInfo = {
    .bisSyncNum = 0,
    .bigSyncState = BIG_LOST,
};

u16 codecSpeakBuff[APP_AUDIO_FRAME_BYTES*2];

/**
 * @brief       codec initial function
 * @param[in]   none.
 * return       none.
 */
void app_codec_init(void)
{
    ble_audio_timer_init(TIMER0);
    tlk_codec_init();
    memset(recvAudioBuff, 0, sizeof(recvAudioBuff));
}

/**
 * @brief       set BIG sync state into codec model.
 * @param[in]   syncState: refer to blc_audio_bigSyncState_enum.
 * @param[in]   numBis: the number of sync BIS.
 * @param[in]   bisHandles: sync BIS handles.
 * @return      none
 */
void app_codec_setBigSyncState(u8 syncState, u8 numBis, u16 bisHandles[0])
{
    appSinkInfo.bigSyncState = syncState;
    if(syncState == BIG_SYNCED)
    {
        for(int i=0; i<numBis; i++)
        {
            appSinkInfo.bisInfo[i].bisHandle = bisHandles[i];
        }
    }
}

/**
 * @brief       set BIG information into control model.
 * @param[in]   codecEvt: broadcast sink initial codec event.
 * @return      none
 */
void app_codec_setBigInformation(blc_bapbs_bisSinkInitCodecEvt_t* codecEvt)
{
    u8 lc3Index = 0;
    appSinkInfo.syncLocation = 0x00000000;
    for(int i=0; i<codecEvt->bisNum; i++)
    {
        bisSyncInfo_t* bisInfo = &codecEvt->bisInfo[i];
        tlkapi_printf(APP_PRF_EVT_LOG_EN, "CodecID is %s Freq:%d Duration:%d FrameOcts:%d\n",
                hex_to_str(&bisInfo->CodecId.id, 5), bisInfo->codecCfg.frequency,
                bisInfo->codecCfg.duration, bisInfo->codecCfg.frameOcts);

        appSinkInfo.bisInfo[i].codecFrameSize = bisInfo->codecCfg.frameOcts;
        appSinkInfo.bisInfo[i].allocation = bisInfo->codecCfg.allocation&(BLC_AUDIO_LOCATION_FLAG_FL | BLC_AUDIO_LOCATION_FLAG_FR);
        u32 validAllocation = bisInfo->codecCfg.allocation&BROADCAST_SINK_LOCATION;
        appSinkInfo.syncLocation |= validAllocation;
        u8 lc3Count = 0;
        if(validAllocation == (BLC_AUDIO_LOCATION_FLAG_FL|BLC_AUDIO_LOCATION_FLAG_FR))
        {
            lc3Count = 2;
        }
        else if(validAllocation == BLC_AUDIO_LOCATION_FLAG_FL || validAllocation == BLC_AUDIO_LOCATION_FLAG_FR )
        {
            lc3Count = 1;
        }

        appSinkInfo.bisInfo[i].lc3Count = lc3Count;
        for(int j=0; j<lc3Count; j++)
        {
            lc3dec_decode_init_bap(lc3Index, bisInfo->codecCfg.frequency, bisInfo->codecCfg.duration, bisInfo->codecCfg.frameOcts);
            appSinkInfo.bisInfo[i].lc3Index[j] = lc3Index++;
        }
    }
    appSinkInfo.presentationDelay = codecEvt->presentationDelay;
    appSinkInfo.bisSyncNum = codecEvt->bisNum;
    switch(codecEvt->bisInfo[0].codecCfg.frequency)
    {
    case BLC_AUDIO_FREQ_CFG_8000:
        appSinkInfo.frameDataLen = 160;
        break;
    case BLC_AUDIO_FREQ_CFG_16000:
        appSinkInfo.frameDataLen = 320;
        break;
    case BLC_AUDIO_FREQ_CFG_24000:
        appSinkInfo.frameDataLen = 480;
        break;
    case BLC_AUDIO_FREQ_CFG_32000:
        appSinkInfo.frameDataLen = 640;
        break;
//  case BLC_AUDIO_FREQ_CFG_44100:
//      rate = AUDIO_44EP1K;
//      break;
    case BLC_AUDIO_FREQ_CFG_48000:
        appSinkInfo.frameDataLen = 960;
        break;
    default:
        appSinkInfo.frameDataLen = 320;
        break;
    }

    #if (APP_AUDIO_OUTPUT_TYPE == APP_AUDIO_IIS_OUT)

    #elif(APP_AUDIO_OUTPUT_TYPE == APP_AUDIO_LINE_OUT)
        tlk_codec_config(TLK_CODEC_OUTPUT, codecEvt->bisInfo[0].codecCfg.frequency, TLK_CODEC_2_CHANNEL, TLK_CODEC_LINE, (u8*)codecSpeakBuff, sizeof(codecSpeakBuff));
        tlk_codec_start(TLK_CODEC_OUTPUT);
    #endif

}

/**
 * @brief       codec model pop all SDU data from BIS .
 * @param[in]   none.
 * @return      true: pop finish.
 *              false: pop not finish.
 */
static bool app_codec_popAllSduData(void)
{
    if(appSinkInfo.bisSyncNum == 1) //Only sync 1 BIS
    {
        appSinkInfo.bisInfo[0].popSdu = blc_ll_popBisSyncRxSduData(appSinkInfo.bisInfo[0].bisHandle);
        if(appSinkInfo.bisInfo[0].popSdu)   return true;
    }
    else    //Sync 2 BIS
    {
        if(appSinkInfo.bisInfo[0].popSdu == NULL)
        {
            appSinkInfo.bisInfo[0].popSdu = blc_ll_popBisSyncRxSduData(appSinkInfo.bisInfo[0].bisHandle);
        }
        else
        {
            appSinkInfo.bisInfo[1].popSdu = blc_ll_popBisSyncRxSduData(appSinkInfo.bisInfo[1].bisHandle);
            if(appSinkInfo.bisInfo[1].popSdu)   return true;
        }
    }
    return false;
}

static void app_audio_decodeSdu(void)
{
    u8 leftBuff[APP_AUDIO_FRAME_BYTES];
    u8 rightBuff[APP_AUDIO_FRAME_BYTES];
    sdu_packet_t* pPkt = NULL;

    appSinkRecvSpeak_t* recvAudioTemp = &recvAudioBuff[recvWptr];;

    for(int i=0; i<appSinkInfo.bisSyncNum; i++)
    {
        appSinkSyncBisInfo_t* bisInfo = &appSinkInfo.bisInfo[i];
        pPkt = bisInfo->popSdu;
        unsigned int detect = 0;
        if(pPkt->pkt_st == HCI_ISO_VALID_DATA){

        }else{
            detect = 1;
            tlkapi_printf(APP_LOG_EN, "lc3 lost:pktSeqNum:0x%x ISO handle: 0x%x", pPkt->pkt_seq_num, pPkt->isoHandle);
        }

        for(int j=0; j<bisInfo->lc3Count; j++)
        {
            lc3dec_set_parameter(bisInfo->lc3Index[j], LC3_PARA_BEC_DETECT, &detect);
        }

        int offsetIndex = 0, lc3Index = 0;
        for(int j=0; j<2; j++)      //TODO: only supported FL/FR
        {
            if(bisInfo->allocation & BIT(j))
            {
                if(BROADCAST_SINK_LOCATION & BIT(j))
                {
                    u8* inputData = pPkt->data + bisInfo->codecFrameSize * offsetIndex;
                    u8* outputData = j==0? leftBuff: rightBuff;
                    lc3dec_decode_pkt(bisInfo->lc3Index[lc3Index], inputData, bisInfo->codecFrameSize, outputData);
                    lc3Index++;
                }
                offsetIndex++;
            }
        }
    }

    recvAudioTemp->seqNum = pPkt->pkt_seq_num;
    recvAudioTemp->renderPoint = pPkt->timestamp + appSinkInfo.presentationDelay*SYSTEM_TIMER_TICK_1US; //TODO: BIG timestamp use 1us, CIS timestamp use tick.

    u16* leftData;
    u16* rightData;
    u16* data = recvAudioTemp->rxBuff;
    if(appSinkInfo.syncLocation == BLC_AUDIO_LOCATION_FLAG_FL) {
        leftData = (u16*)leftBuff;
        rightData = (u16*)leftBuff;
    }
    else if(appSinkInfo.syncLocation == BLC_AUDIO_LOCATION_FLAG_FR)
    {
        leftData = (u16*)rightBuff;
        rightData = (u16*)rightBuff;
    }
    else
    {
        leftData = (u16*)leftBuff;
        rightData = (u16*)rightBuff;
    }

    for(int i=0; i<(appSinkInfo.frameDataLen>>1); i++)
    {
        *data++ = *leftData++;
        *data++ = *rightData++;
    }

    u32 r = irq_disable();
    u8 empty = recvRptr == recvWptr;
    recvWptr++;
    if(recvWptr == APP_SINK_RECV_SPEAK_FRAME_COUNT)
    {
        recvWptr = 0;
    }
    irq_restore(r);


    if(empty)
    {
        u32 capture_tick_stimer = recvAudioTemp->renderPoint-clock_time();
        u32 capture_tick_timer  = (capture_tick_stimer*sys_clk.pclk)/SYSTEM_TIMER_TICK_1US ;
        ble_audio_timer_set_capture(TIMER0, 0, capture_tick_timer);
    }

}

u32 currentTime;
/**
 * @brief       broadcast sink audio receive BIS SDU Handler.
 * @param[in]   none
 * @return      none
 */
void app_audio_receiveHandler(void)
{
    if(appSinkInfo.bigSyncState == BIG_SYNCED)
    {
        if(app_codec_popAllSduData())
        {
            appSinkInfo.spkState = 1;
            app_audio_decodeSdu();
            appSinkInfo.bisInfo[0].popSdu = NULL;
            appSinkInfo.bisInfo[1].popSdu = NULL;
        }
    }
    else
    {
        if(appSinkInfo.spkState) {
            appSinkInfo.bisInfo[0].popSdu = NULL;
            appSinkInfo.bisInfo[1].popSdu = NULL;
            timer_stop(TIMER0);
            tlk_codec_stop(TLK_CODEC_OUTPUT);
            appSinkInfo.spkState = 0;
            recvWptr = recvRptr = 0;
            memset(recvAudioBuff, 0, sizeof(recvAudioBuff));
        }
    }
}

_attribute_ram_code_ void app_timer_irq_proc(void)
{
    timer_stop(TIMER0);

    u32 readOffset = 0;
    u32 writeOffset = 0;
    u32 offset = 0;
    if(tlk_codec_output_getOffset(&writeOffset,&readOffset)!=TLK_CODEC_SUCCESS)
    {
        recvRptr = recvWptr;
        return;
    }

    if(writeOffset >= readOffset)
    {
        offset = writeOffset - readOffset;
    }
    else
    {
        offset = sizeof(codecSpeakBuff) + writeOffset - readOffset;
    }

    if(offset<8 || offset>24)
    {
        tlkapi_printf(APP_LOG_EN,"set offset, %d %d %d", offset, writeOffset, readOffset);

        if(tlk_codec_output_setWriteOffset(readOffset+16)!=TLK_CODEC_SUCCESS)
        {
            recvRptr = recvWptr;
            tlkapi_printf(APP_LOG_EN,"set offset error");
            return;
        }
    }

    appSinkRecvSpeak_t* recvAudioTemp = &recvAudioBuff[recvRptr];

    if(tlk_codec_output_dataPush((u8*)recvAudioTemp->rxBuff, appSinkInfo.frameDataLen*2)!=TLK_CODEC_SUCCESS)
    {
        recvRptr = recvWptr;
        tlkapi_printf(APP_LOG_EN, "data push error");
        return;
    }

    blc_audio_clock_calib(recvAudioTemp->renderPoint, 10000*SYSTEM_TIMER_TICK_1US);

    recvRptr++;
    if(recvRptr == APP_SINK_RECV_SPEAK_FRAME_COUNT)
    {
        recvRptr = 0;
    }

    if(recvRptr != recvWptr)
    {
        recvAudioTemp = &recvAudioBuff[recvRptr];
        u32 capture_tick_stimer = recvAudioTemp->renderPoint-clock_time();
        u32 capture_tick_timer  = (capture_tick_stimer*sys_clk.pclk)/SYSTEM_TIMER_TICK_1US ;
        ble_audio_timer_set_capture(TIMER0, 0, capture_tick_timer);
    }

}

/**
 * @brief       timer0 interrupt handler.
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void timer0_irq_handler(void)
{
    if(timer_get_irq_status(TMR_STA_TMR0))
    {
        app_timer_irq_proc();
        timer_clr_irq_status(TMR_STA_TMR0);//clear irq must come after irq process.
    }
}
PLIC_ISR_REGISTER(timer0_irq_handler, IRQ_TIMER0)
