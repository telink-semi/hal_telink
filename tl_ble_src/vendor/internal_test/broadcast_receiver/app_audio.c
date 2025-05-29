/********************************************************************************************************
 * @file    app_audio.c
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

#include "app_buffer.h"
#include "app_audio.h"

#if (INTER_TEST_MODE == TEST_BIS_AUDIO_RECEIVER)

bool gAppAudioIsRecv     = false;
u32  gAppAudioDelayTimer = 0;
bool gAppAudioIsPlay     = false;


appSinkInfo_t appSinkInfo = {
    .aclHandle  = 0,
    .bisHandle  = 0,
    .decodeSize = 0,
    .recvPckCnt = 0,
    .spkState   = 0,
};

u8 codecSpeakBuff[APP_AUDIO_FRAME_BYTES * APP_SINK_RECV_SPEAK_FRAME_COUNT];

    #if (MCU_CORE_TYPE == MCU_CORE_B92)
audio_codec_stream0_input_t audio_codec_input =
    {
        .input_src     = DMIC_STREAM0_MONO_L,
        .sample_rate   = AUDIO_16K,
        .fifo_num      = FIFO0,
        .data_width    = CODEC_BIT_16_DATA,
        .dma_num       = DMA2,
        .data_buf      = codecSpeakBuff,
        .data_buf_size = sizeof(codecSpeakBuff),
};
audio_codec_output_t audio_codec_output =
    {
        .output_src    = DMIC_STREAM0_MONO_L,
        .sample_rate   = AUDIO_16K,
        .fifo_num      = FIFO0,
        .data_width    = CODEC_BIT_16_DATA,
        .dma_num       = DMA3,
        .mode          = HP_MODE,
        .data_buf      = codecSpeakBuff,
        .data_buf_size = sizeof(codecSpeakBuff),
};
    #endif


extern sdu_packet_t *blc_ll_popBisSyncRxSduData(u16 bis_connHandle);

unsigned short IisDataMix[960] = {0};

enum
{
    SPEAK_ST_MUTE,
    SPEAK_ST_OPEN,
};

void app_audio_handler(void)
{
    if (appSinkInfo.bisHandle) {
        sdu_packet_t *pPkt = NULL;

        /*
         * from qihang, now two bis have problem. so now only process one bis data. later process multiple bis data.
         */
        pPkt = blc_ll_popBisSyncRxSduData(appSinkInfo.bisHandle);
        if (pPkt != NULL) {
            if (pPkt->iso_sdu_len == 0 && pPkt->pkt_st == HCI_ISO_VALID_DATA) {
                return;
            }

            if (pPkt->pkt_st == HCI_ISO_LOST_DATA) {
                BLT_APP_LOG("mainloop packet loss:0x%x", pPkt->iso_sdu_len);
            }
            //BLT_APP_LOG("bis handle:0x%x", appSinkInfo.bisHandle);
            //BLT_APP_LOG("pkt st:0x%x", pPkt->pkt_st);
            u8 *audioBuffer = (appSinkInfo.recvPckCnt & (APP_SINK_RECV_SPEAK_FRAME_COUNT - 1)) * appSinkInfo.frameDataLen + appSinkInfo.recvSpeakBuff;
            //BLT_APP_STR_LOG("[APP]***** Get SDU *****", ((u8*)pPkt), 9+4+pPkt->iso_sdu_len);

            unsigned int detect = 0;

            if (pPkt->iso_sdu_len == appSinkInfo.decodeSize) {
                BLT_APP_LOG("lc3 decode1:0x%x", pPkt->iso_sdu_len);
            } else {
                detect = 1;
                BLT_APP_LOG("lc3 decode2:0x%x", pPkt->iso_sdu_len);
            }
            LC3DEC_Error ret_lc3 = lc3dec_set_parameter(0, LC3_PARA_BEC_DETECT, &detect);

            if (ret_lc3 != LC3DEC_OK) {
                BLT_APP_LOG("lc3 decode set parameter error:0x%x", ret_lc3);
                return;
            }

            ret_lc3 = lc3dec_decode_pkt(0, (u8 *)pPkt->data, pPkt->iso_sdu_len, audioBuffer);
            if (ret_lc3 != LC3DEC_OK) {
                BLT_APP_LOG("lc3 decode error:0x%x", ret_lc3);
                return;
            }
            //BLT_APP_STR_LOG("[APP]lc3 decode audio data is ", audioBuffer, appSinkInfo.frameDataLen);

            appSinkInfo.recvPckCnt++;

            if (appSinkInfo.spkState == SPEAK_ST_OPEN) //when the first 4 packets have been processed, then process the following packet one by one.
            {
    #if (APP_AUDIO_OUTPUT_TYPE == APP_AUDIO_LINE_OUT)
                blc_codec_WriteSpkBuff(appSinkInfo.recvSpeakBuff, appSinkInfo.frameDataLen);
    #elif (APP_AUDIO_OUTPUT_TYPE == APP_AUDIO_IIS_OUT)
                for (int i = 0; i < 480; i++) {
                    u16 temp_pcm = audioBuffer[2 * i];
                    temp_pcm += audioBuffer[2 * i + 1] << 8;
                    IisDataMix[2 * i] = temp_pcm;
                }
                blc_codec_WriteSpkBuff((u8 *)IisDataMix, appSinkInfo.frameDataLen * 2);
    #endif
                appSinkInfo.recvPckCnt--;
            } else if (appSinkInfo.spkState == SPEAK_ST_MUTE) //mute the first 1 second packet to avoid pop voice. from qihang.
            {
                ble_codec_spkOpen();

    #if (APP_AUDIO_OUTPUT_TYPE == APP_AUDIO_LINE_OUT)
                audio_set_codec_dac_mute(); //mute the speaker
                blc_codec_WriteSpkBuff(appSinkInfo.recvSpeakBuff, appSinkInfo.frameDataLen);
    #elif (APP_AUDIO_OUTPUT_TYPE == APP_AUDIO_IIS_OUT)
                for (int i = 0; i < 480; i++) {
                    u16 temp_pcm = audioBuffer[2 * i];
                    temp_pcm += audioBuffer[2 * i + 1] << 8;
                    IisDataMix[2 * i] = temp_pcm;
                }
                blc_codec_WriteSpkBuff((u8 *)IisDataMix, appSinkInfo.frameDataLen * appSinkInfo.recvPckCnt * 2);
    #endif

                if (appSinkInfo.recvPckCnt >= 100) { //from qihang's advice, in order to avoid pop voice. 1s data.
                    audio_set_codec_dac_unmute();
                    appSinkInfo.spkState   = SPEAK_ST_OPEN;
                    appSinkInfo.recvPckCnt = 0;
                }
            }
        }
    } else {
        if (appSinkInfo.spkState) {
            ble_audio_codec_close();
            appSinkInfo.spkState = SPEAK_ST_MUTE;
        }
    }
}


#endif /* INTER_TEST_MODE */
