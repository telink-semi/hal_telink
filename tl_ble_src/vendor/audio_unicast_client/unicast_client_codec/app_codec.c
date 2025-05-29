/********************************************************************************************************
 * @file    app_codec.c
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

#include "app_audio.h"
#include "app_config.h"
#include "app_codec.h"

#include "vendor/common/tlk_api/tlk_mem.h"
#include "vendor/common/tlk_api/tlk_codec.h"
#include "algorithm/audio_alg/lc3/lc3.h"

#if (UNICAST_CLIENT_SELECT == UNICAST_CLIENT_CODEC)
extern app_ctrl_t  appCtrl;
struct list_node_t pStart;                                                //header node
unsigned short     app_audio_output_buffer[APP_AUDIO_OUTPUT_BUFFER_SIZE]; //48k,10ms,
unsigned short     app_audio_input_buffer[APP_AUDIO_INPUT_BUFFER_SIZE];   //48k,10ms,
app_codec_desc_t   codecI;
app_codec_desc_t   codecO;
    #if (APP_AUDIO_SCENE == APP_SCENE_TWS)
static void app_list_add(audio_pkt_t *pData, u8 channel);
    #elif (APP_AUDIO_SCENE == APP_SCENE_HEADSET)
static void app_list_add(audio_pkt_t *pData);
    #endif
static bool app_list_delete_node(struct list_node_t *pData);
static void app_audio_receive_process(void);
static void app_audio_send_process(void);
static void app_codec_process(void);

/**
 * @brief      Codec init function.
 * @param[in]  none.
 * @return     none.
 */
void app_codec_init(void)
{
    ble_audio_timer_init(TIMER0);
    ble_audio_timer_init(TIMER1);
    pStart.next = NULL;


    #if (APP_AUDIO_SCENE == APP_SCENE_TWS)
    tlk_codec_init();
    tlk_codec_config(TLK_CODEC_OUTPUT, TLK_CODEC_FREQ_16000, TLK_CODEC_2_CHANNEL, TLK_CODEC_LINE, (u8 *)app_audio_output_buffer, 2 * APP_AUDIO_OUTPUT_BUFFER_SIZE);
    tlk_codec_config(TLK_CODEC_INPUT, TLK_CODEC_FREQ_16000, TLK_CODEC_2_CHANNEL, TLK_CODEC_MIC, (u8 *)app_audio_input_buffer, 2 * APP_AUDIO_INPUT_BUFFER_SIZE);
    codecI.cC      = 2;
    codecI.fSample = gLc3Index[codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].frequencyValue][codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].durationValue].frameSample;
    codecI.fOctets = 2 * codecI.fSample;
    codecI.frameUs = gLc3Index[codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].frequencyValue][codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].durationValue].duraUs;

    codecO.cC      = 2;
    codecO.fSample = gLc3Index[codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].frequencyValue][codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].durationValue].frameSample;
    codecO.fOctets = 2 * codecO.fSample;
    codecO.frameUs = gLc3Index[codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].frequencyValue][codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].durationValue].duraUs;
    tlkapi_printf(APP_LOG_EN, "codecI.fSample %d", codecI.fSample);
    tlkapi_printf(APP_LOG_EN, "codecI.fOctets %d", codecI.fOctets);
    tlkapi_printf(APP_LOG_EN, "codecI.frameUs %d", codecI.frameUs);

    tlkapi_printf(APP_LOG_EN, "codecO.fSample %d", codecO.fSample);
    tlkapi_printf(APP_LOG_EN, "codecO.fOctets %d", codecO.fOctets);
    tlkapi_printf(APP_LOG_EN, "codecO.frameUs %d", codecO.frameUs);
    tlk_mem_pool_desc_t poolDesc[] =
        {
            {660, 10}, //receive data use,16k,2channel,10ms,640byte each frame.
    };
    #elif (APP_AUDIO_SCENE == APP_SCENE_HEADSET)
    tlk_codec_init();
    tlk_codec_config(TLK_CODEC_INPUT, TLK_CODEC_FREQ_48000, TLK_CODEC_2_CHANNEL, TLK_CODEC_MIC, (u8 *)app_audio_input_buffer, 2 * APP_AUDIO_INPUT_BUFFER_SIZE);
    tlk_codec_config(TLK_CODEC_OUTPUT, TLK_CODEC_FREQ_16000, TLK_CODEC_1_CHANNEL, TLK_CODEC_LINE, (u8 *)app_audio_output_buffer, 2 * APP_AUDIO_OUTPUT_BUFFER_SIZE);
    codecI.cC      = 2;
    codecI.fSample = gLc3Index[codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].frequencyValue][codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].durationValue].frameSample;
    codecI.fOctets = 2 * codecI.fSample;
    codecI.frameUs = gLc3Index[codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].frequencyValue][codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].durationValue].duraUs;

    codecO.cC      = 1;
    codecO.fSample = gLc3Index[codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].frequencyValue][codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].durationValue].frameSample;
    codecO.fOctets = 2 * codecO.fSample;
    codecO.frameUs = gLc3Index[codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].frequencyValue][codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].durationValue].duraUs;
    tlkapi_printf(APP_LOG_EN, "codecI.fSample %d", codecI.fSample);
    tlkapi_printf(APP_LOG_EN, "codecI.fOctets %d", codecI.fOctets);
    tlkapi_printf(APP_LOG_EN, "codecI.frameUs %d", codecI.frameUs);

    tlkapi_printf(APP_LOG_EN, "codecO.fSample %d", codecO.fSample);
    tlkapi_printf(APP_LOG_EN, "codecO.fOctets %d", codecO.fOctets);
    tlkapi_printf(APP_LOG_EN, "codecO.frameUs %d", codecO.frameUs);
    tlk_mem_pool_desc_t poolDesc[] =
        {
            {340, 8}, //receive data use,16k,1channel,10ms,320 byte each frame.
    };
    #endif
    const u8 numPools = sizeof(poolDesc) / sizeof(poolDesc[0]);
    //blocks malloc
    if (tlk_mempool_init(numPools, poolDesc) != 0) {
        tlkapi_printf(APP_LOG_EN, "error-mempool init failed!");
    }
}

/**
 * @brief      Codec process in loop function,include audio data send,audio data receive and other codec process.
 * @param[in]  none.
 * @return     none.
 */
void app_codec_handler(void)
{
    app_audio_receive_process();
    app_audio_send_process();
    app_codec_process();
}

static void app_codec_process(void) //config lc3
{
    for (u8 i = 0; i < appCtrl.acl_max_num; i++) {
        if (appCtrl.aclParam[i].sink.codecOp == APP_CONFIG_CODEC) {
            u8 paramIndex = appCtrl.aclParam[i].sink.codecParam;
            if (appCtrl.aclParam[i].sink.blocks > 1) {
                for (u8 j = 0; j < appCtrl.aclParam[i].sink.blocks; j++) {
                    int lc3Ret = lc3dec_decode_init_bap(j, codecSettings[paramIndex].frequencyValue, codecSettings[paramIndex].durationValue, codecSettings[paramIndex].frameOctets);
                    if (lc3Ret != LC3DEC_OK) {
                        tlkapi_printf(APP_LOG_EN, "lc3 decode init fail:0x%x\n", lc3Ret);
                        return;
                    }
                    tlkapi_printf(APP_LOG_EN, "j %d\n", j);
                    tlkapi_printf(APP_LOG_EN, "lc3 decode init success:0x%x\n", j);
                    tlkapi_printf(APP_LOG_EN, "codecSettings[paramIndex].frequencyValue %d\n", codecSettings[paramIndex].frequencyValue);
                    tlkapi_printf(APP_LOG_EN, "codecSettings[paramIndex].durationValue  %d\n", codecSettings[paramIndex].durationValue);
                    tlkapi_printf(APP_LOG_EN, "codecSettings[paramIndex].frameOctets    %d\n", codecSettings[paramIndex].frameOctets);
                }
            } else {
                int lc3Ret = lc3dec_decode_init_bap(i, codecSettings[paramIndex].frequencyValue, codecSettings[paramIndex].durationValue, codecSettings[paramIndex].frameOctets);
                if (lc3Ret != LC3DEC_OK) {
                    tlkapi_printf(APP_LOG_EN, "lc3 decode init fail:0x%x", lc3Ret);
                    return;
                }
                tlkapi_printf(APP_LOG_EN, "i %d", i);
                tlkapi_printf(APP_LOG_EN, "lc3 decode init success:0x%x", i);
                tlkapi_printf(APP_LOG_EN, "codecSettings[paramIndex].frequencyValue %d", codecSettings[paramIndex].frequencyValue);
                tlkapi_printf(APP_LOG_EN, "codecSettings[paramIndex].durationValue  %d", codecSettings[paramIndex].durationValue);
                tlkapi_printf(APP_LOG_EN, "codecSettings[paramIndex].frameOctets    %d", codecSettings[paramIndex].frameOctets);
            }
            appCtrl.aclParam[i].sink.codecOp = APP_CODEC_IDLE;
        }
        if (appCtrl.aclParam[i].source.codecOp == APP_CONFIG_CODEC) {
            u8 paramIndex = appCtrl.aclParam[i].source.codecParam;
            if (appCtrl.aclParam[i].source.blocks > 1) {
                for (u8 j = 0; j < appCtrl.aclParam[i].source.blocks; j++) {
                    int lc3Ret = lc3enc_encode_init_bap(j, codecSettings[paramIndex].frequencyValue, codecSettings[paramIndex].durationValue, codecSettings[paramIndex].frameOctets);
                    if (lc3Ret != LC3DEC_OK) {
                        tlkapi_printf(APP_LOG_EN, "lc3  encode init fail:0x%x", lc3Ret);
                        return;
                    }
                    tlkapi_printf(APP_LOG_EN, "j %d", j);
                    tlkapi_printf(APP_LOG_EN, "lc3 encode init success:0x%x", j);
                    tlkapi_printf(APP_LOG_EN, "codecSettings[paramIndex].frequencyValue %d", codecSettings[paramIndex].frequencyValue);
                    tlkapi_printf(APP_LOG_EN, "codecSettings[paramIndex].durationValue  %d", codecSettings[paramIndex].durationValue);
                    tlkapi_printf(APP_LOG_EN, "codecSettings[paramIndex].frameOctets    %d", codecSettings[paramIndex].frameOctets);
                }
            } else {
                int lc3Ret = lc3enc_encode_init_bap(i, codecSettings[paramIndex].frequencyValue, codecSettings[paramIndex].durationValue, codecSettings[paramIndex].frameOctets);
                if (lc3Ret != LC3DEC_OK) {
                    tlkapi_printf(APP_LOG_EN, "lc3  encode init fail:0x%x", lc3Ret);
                    return;
                }
                tlkapi_printf(APP_LOG_EN, "i %d", i);
                tlkapi_printf(APP_LOG_EN, "lc3 encode init success:0x%x", i);
                tlkapi_printf(APP_LOG_EN, "codecSettings[paramIndex].frequencyValue %d", codecSettings[paramIndex].frequencyValue);
                tlkapi_printf(APP_LOG_EN, "codecSettings[paramIndex].durationValue  %d", codecSettings[paramIndex].durationValue);
                tlkapi_printf(APP_LOG_EN, "codecSettings[paramIndex].frameOctets    %d", codecSettings[paramIndex].frameOctets);
            }
            appCtrl.mic_reset                  = 1;
            appCtrl.aclParam[i].source.codecOp = APP_CODEC_IDLE;
        }
    }
}

static void app_audio_receive_process(void)
{
    if (tlk_codec_getState(TLK_CODEC_OUTPUT) != TLK_CODEC_STATE_STREAMING) {
        return;
    }
    #if (APP_AUDIO_SCENE == APP_SCENE_TWS)
    for (u8 i = 0; i < appCtrl.acl_max_num; i++) {
        if (appCtrl.aclParam[i].sink.sS) {
            sdu_packet_t *pPkt = blc_bapuc_sduPacketPop(appCtrl.aclParam[i].acl_handle, 0);
            if (pPkt != NULL) {
                unsigned int detect = 0;
                if (pPkt->iso_sdu_len != codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].frameOctets) {
                    if (!appCtrl.aclParam[i].sink.sT) {
                        APP_DBG_CHN_6_HIGH;
                        APP_DBG_CHN_6_LOW;
                        return;
                    } else {
                        APP_DBG_CHN_7_HIGH;
                        APP_DBG_CHN_7_LOW;
                        detect = 1;
                    }
                } else {
                    APP_DBG_CHN_8_HIGH;
                    APP_DBG_CHN_8_LOW;
                    appCtrl.aclParam[i].sink.sT = clock_time() | 1;
                }
                LC3DEC_Error ret_lc3 = lc3dec_set_parameter(i, LC3_PARA_BEC_DETECT, &detect);
                if (ret_lc3 != LC3DEC_OK) {
                    tlkapi_printf(APP_LOG_EN, "lc3 decode set parameter error:0x%x", ret_lc3);
                    return;
                }
                APP_DBG_CHN_3_HIGH;
                audio_pkt_t pRaw = {0};
                ret_lc3          = lc3dec_decode_pkt(i, pPkt->data, pPkt->iso_sdu_len, (u8 *)pRaw.buffer);
                if (ret_lc3 != LC3DEC_OK) {
                    tlkapi_printf(APP_LOG_EN, "lc3 decode error:0x%x", ret_lc3);
                    return;
                } else {
                    pRaw.renderPoint = pPkt->timestamp + appCtrl.aclParam[i].sink.pD * SYSTEM_TIMER_TICK_1US + AUDIO_UNICAST_CLIENT_MAX_TRANSPORT_LATENCY * SYSTEM_TIMER_TICK_1MS; //tick
                    app_list_add(&pRaw, i);
                }
                APP_DBG_CHN_3_LOW;
            }
        }
    }
    #elif (APP_AUDIO_SCENE == APP_SCENE_HEADSET)
    for (u8 i = 0; i < appCtrl.acl_max_num; i++) {
        if (appCtrl.aclParam[i].sink.sS) {
            sdu_packet_t *pPkt = blc_bapuc_sduPacketPop(appCtrl.aclParam[i].acl_handle, 0);
            if (pPkt != NULL) {
                unsigned int detect = 0;
                if (pPkt->iso_sdu_len != codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].frameOctets) {
                    if (!appCtrl.aclParam[i].sink.sT) {
                        APP_DBG_CHN_6_HIGH;
                        APP_DBG_CHN_6_LOW;
                        return;
                    } else {
                        APP_DBG_CHN_7_HIGH;
                        APP_DBG_CHN_7_LOW;
                        detect = 1;
                    }
                } else {
                    APP_DBG_CHN_8_HIGH;
                    APP_DBG_CHN_8_LOW;
                    appCtrl.aclParam[i].sink.sT = clock_time() | 1;
                }
                LC3DEC_Error ret_lc3 = lc3dec_set_parameter(i, LC3_PARA_BEC_DETECT, &detect);
                if (ret_lc3 != LC3DEC_OK) {
                    tlkapi_printf(APP_LOG_EN, "lc3 decode set parameter error:0x%x", ret_lc3);
                    return;
                }
                APP_DBG_CHN_3_HIGH;
                audio_pkt_t pRaw = {0};
                ret_lc3          = lc3dec_decode_pkt(i, pPkt->data, pPkt->iso_sdu_len, (u8 *)pRaw.buffer);
                if (ret_lc3 != LC3DEC_OK) {
                    tlkapi_printf(APP_LOG_EN, "lc3 decode error:0x%x", ret_lc3);
                    return;
                } else {
                    pRaw.renderPoint = pPkt->timestamp + appCtrl.aclParam[i].sink.pD * SYSTEM_TIMER_TICK_1US + AUDIO_UNICAST_CLIENT_MAX_TRANSPORT_LATENCY * SYSTEM_TIMER_TICK_1MS; //tick
                    app_list_add(&pRaw);
                }
                APP_DBG_CHN_3_LOW;
            }
        }
    }
    #endif
}

u32 capture1_tick_stimer = 0;
u32 mic_wptr1            = 0;

static void app_audio_send_process(void)
{
    if (tlk_codec_getState(TLK_CODEC_INPUT) != TLK_CODEC_STATE_STREAMING) {
        return;
    }

    if (appCtrl.mic_reset) {
        APP_DBG_CHN_6_HIGH;
        capture1_tick_stimer = clock_time();
        u32 mic_wptr         = codec_get_InputWriteOffset();
        u32 rptr             = (2 * APP_AUDIO_INPUT_BUFFER_SIZE + mic_wptr - ((codecI.cC * codecI.fOctets * 2) >> 2)) % (APP_AUDIO_INPUT_BUFFER_SIZE * 2);
        codec_set_InputReadOffset(rptr);
        appCtrl.mic_reset = 0;
        tlkapi_printf(APP_LOG_EN, "mic_reset:");
        tlkapi_printf(APP_LOG_EN, "mic_reset111:%d , %d", mic_wptr, rptr);
        //      ble_audio_timer_init(TIMER1);
        capture1_tick_stimer    = 6000 * SYSTEM_TIMER_TICK_1US + capture1_tick_stimer;
        u32 capture1_tick_timer = ((u32)(capture1_tick_stimer - clock_time()) * sys_clk.pclk) / SYSTEM_TIMER_TICK_1US;
        ble_audio_timer_set_capture(TIMER1, 0, capture1_tick_timer);
        mic_wptr1 = 0xffffffff;
        APP_DBG_CHN_6_LOW;
    }
    u16 pcmData[2 * APP_AUDIO_INPUT_FRAME_SAMPLE_MAX] = {0};
    #if (APP_AUDIO_SCENE == APP_SCENE_TWS)
    if (mic_wptr1 == 0xffffffff) {
        return;
    }
    if (codec_input_readData1((u8 *)pcmData, codecI.cC * codecI.fOctets, mic_wptr1) == 1)
    //    if(tlk_codec_input_dataPop((u8*)pcmData,codecI.cC*codecI.fOctets) == TLK_CODEC_SUCCESS)
    {
        mic_wptr1 = 0xffffffff;
        //      u32 rptr0 = codec_get_InputReadOffset();
        //      u32 wptr0 = codec_get_InputWriteOffset();
        //      tlkapi_printf(APP_LOG_EN,"offset:0x%x,0x%x", rptr0,wptr0);
        u16 audio_pcm[APP_AUDIO_INPUT_FRAME_SAMPLE_MAX]          = {0};
        u8  audio_enc[2][APP_AUDIO_INPUT_FRAME_ENCODE_BYTES_MAX] = {0};

        //        static u8 wptr = 0;
        //        wptr &=7;
        //        wptr++;
        //        for(u8 i=0;i<wptr;i++)
        //        {
        //          APP_DBG_CHN_5_HIGH;
        //          APP_DBG_CHN_5_LOW;
        //        }

        for (u8 i = 0; i < appCtrl.acl_max_num; i++) {
            APP_DBG_CHN_1_HIGH;
            if (appCtrl.aclParam[i].source.sS) {
                for (u16 j = 0; j < codecI.fSample; j++) {
                    audio_pcm[j] = pcmData[2 * j + i];
                }
                LC3ENC_Error ret_lc3 = lc3enc_encode_pkt(i, (u8 *)audio_pcm, audio_enc[i]);
                if (ret_lc3 != LC3ENC_OK) {
                    tlkapi_printf(APP_LOG_EN, "lc3 encode error:0x%x", ret_lc3);
                }
            }
            APP_DBG_CHN_1_LOW;
        }
        //      audio_enc[0][0] = wptr;
        //      audio_enc[1][0] = wptr;
        for (u8 i = 0; i < appCtrl.acl_max_num; i++) {
            if (appCtrl.aclParam[i].source.sS) {
                APP_DBG_CHN_2_HIGH;
                int ret = blc_bapuc_sduPacketPush(appCtrl.aclParam[i].acl_handle, 0, audio_enc[i], codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].frameOctets);
                if (ret != AUDIO_ESUCC) {
                    tlkapi_printf(APP_LOG_EN, "sdu send fail-ret:0x%x", ret);
                }
                APP_DBG_CHN_2_LOW;
            }
        }
    }
    #elif (APP_AUDIO_SCENE == APP_SCENE_HEADSET)
    u16 pcmData[2 * APP_AUDIO_INPUT_FRAME_SAMPLE_MAX] = {0};
    if (tlk_codec_input_dataPop((u8 *)pcmData, codecI.cC * codecI.fOctets) == TLK_CODEC_SUCCESS) {
        u16 audio_pcm[APP_AUDIO_INPUT_FRAME_SAMPLE_MAX]           = {0};
        u8  audio_enc[2 * APP_AUDIO_INPUT_FRAME_ENCODE_BYTES_MAX] = {0};
        for (u8 i = 0; i < appCtrl.acl_max_num; i++) {
            APP_DBG_CHN_1_HIGH;
            if (appCtrl.aclParam[i].source.sS) {
                for (u8 j = 0; j < appCtrl.aclParam[i].source.blocks; j++) {
                    for (u16 t = 0; t < codecI.fSample; t++) {
                        audio_pcm[t] = pcmData[2 * t + j];
                    }
                    LC3ENC_Error ret_lc3 = lc3enc_encode_pkt(j, (u8 *)audio_pcm, audio_enc + j * codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].frameOctets);
                    if (ret_lc3 != LC3ENC_OK) {
                        tlkapi_printf(APP_LOG_EN, "lc3 encode error:0x%x", ret_lc3);
                    }
                }
            }
            APP_DBG_CHN_1_LOW;
        }
        for (u8 i = 0; i < appCtrl.acl_max_num; i++) {
            if (appCtrl.aclParam[i].source.sS) {
                APP_DBG_CHN_2_HIGH;
                int ret = blc_bapuc_sduPacketPush(appCtrl.aclParam[i].acl_handle, 0, audio_enc, appCtrl.aclParam[i].source.blocks * codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].frameOctets);
                if (ret != AUDIO_ESUCC) {
                    tlkapi_printf(APP_LOG_EN, "sdu send fail-ret:0x%x", ret);
                }
                APP_DBG_CHN_2_LOW;
            }
        }
    }
    #endif
}

_attribute_ram_code_ void app_timer1_irq_proc(void)
{
    //  APP_DBG_CHN_5_HIGH;
    timer_stop(TIMER1);
    mic_wptr1              = codec_get_InputWriteOffset();
    capture1_tick_stimer   = 10000 * SYSTEM_TIMER_TICK_1US + capture1_tick_stimer;
    u32 capture_tick_timer = ((u32)(capture1_tick_stimer - clock_time()) * sys_clk.pclk) / SYSTEM_TIMER_TICK_1US;
    ble_audio_timer_set_capture(TIMER1, 0, capture_tick_timer);
    //   APP_DBG_CHN_5_LOW;
}

/**
 * @brief      Timer irq process,used to playback audio data at a specific tick.
 * @param[in]  none.
 * @return     none.
 */
_attribute_ram_code_ void app_timer_irq_proc(void)
{
    timer_stop(TIMER0);
    APP_DBG_CHN_4_HIGH;
    u32 readOffset  = 0;
    u32 writeOffset = 0;
    u32 offset      = 0;
    if (tlk_codec_output_getOffset(&writeOffset, &readOffset) != TLK_CODEC_SUCCESS) {
        app_list_free();
        //        tlkapi_printf(APP_LOG_EN,"get offset error");
        return;
    }

    if (writeOffset >= readOffset) {
        offset = writeOffset - readOffset;

    } else {
        offset = 2 * APP_AUDIO_OUTPUT_BUFFER_SIZE + writeOffset - readOffset;
    }
    if (offset < 12 || offset > 48) {
        APP_DBG_CHN_5_HIGH;
        //        tlkapi_printf(APP_LOG_EN,"set offset");
        if (tlk_codec_output_setWriteOffset(readOffset + 32) != TLK_CODEC_SUCCESS) {
            app_list_free();
            //            tlkapi_printf(APP_LOG_EN,"set offset error");
            return;
        }
        APP_DBG_CHN_5_LOW;
    }

    if (tlk_codec_output_dataPush((u8 *)pStart.next->buffer, codecO.cC * codecO.fOctets) != TLK_CODEC_SUCCESS) {
        app_list_free();
        //        tlkapi_printf(APP_LOG_EN,"data push error");
        return;
    }
    if (app_list_delete_node(pStart.next) == 0) {
        //        tlkapi_printf(APP_LOG_EN,"memory free failed-start");
    }
    if (pStart.next != NULL) {
        u32 capture_tick_stimer = pStart.next->renderPoint - clock_time();
        u32 capture_tick_timer  = (capture_tick_stimer * sys_clk.pclk) / SYSTEM_TIMER_TICK_1US;
        ble_audio_timer_set_capture(TIMER0, 0, capture_tick_timer);
    }
    APP_DBG_CHN_4_LOW;
}

/**
 * @brief      Free all node in list.
 * @param[in]  none.
 * @return     none.
 */
_attribute_ram_code_ void app_list_free(void)
{
    while (pStart.next != NULL) {
        app_list_delete_node(pStart.next);
    }
}

_attribute_ram_code_ static bool app_list_delete_node(struct list_node_t *pData)
{
    struct list_node_t *pTemp = &pStart;
    while (pTemp->next != NULL) {
        APP_DBG_CHN_12_HIGH;
        APP_DBG_CHN_12_LOW;
        if (pTemp->next == pData) {
            if (pTemp->next->next != NULL) {
                struct list_node_t *pDel = pTemp->next;
                pTemp->next              = pTemp->next->next;
                memset((u8 *)&pDel->renderPoint, 0, sizeof(struct list_node_t));
                TLK_MEM_STATE_T memRet = tlk_mem_free(pDel);
                if (memRet != TLK_MEM_SUCCESS) {
                    tlkapi_printf(APP_LOG_EN, "mempool free failed,pos1!");
                }
                return 1;
            } else {
                struct list_node_t *pDel = pTemp->next;
                pTemp->next              = NULL;
                memset((u8 *)&pDel->renderPoint, 0, sizeof(struct list_node_t));
                TLK_MEM_STATE_T memRet = tlk_mem_free(pDel);
                if (memRet != TLK_MEM_SUCCESS) {
                    tlkapi_printf(APP_LOG_EN, "mempool free failed,pos2!");
                }
                return 1;
            }
        }
        pTemp = pTemp->next;
    }
    return 0;
}

    #if (APP_AUDIO_SCENE == APP_SCENE_TWS)
_attribute_ram_code_ static void app_list_add(audio_pkt_t *pData, u8 channel)
{
    if (((unsigned int)(pData->renderPoint - clock_time())) > 200 * SYSTEM_TIMER_TICK_1MS) {
        APP_DBG_CHN_14_HIGH;
        APP_DBG_CHN_14_LOW;
        return;
    }
    u32 irq = irq_disable();
    if (pStart.next == NULL) {
        APP_DBG_CHN_9_HIGH;
        struct list_node_t *pNew = (struct list_node_t *)tlk_mem_malloc(sizeof(struct list_node_t));
        if (pNew == NULL) {
            tlkapi_printf(APP_LOG_EN, "mempool malloc failed,pos1!");
            while (1) {
        ////////////////////////////////////// Debug entry /////////////////////////////////
        #if (TLKAPI_DEBUG_ENABLE)
                tlkapi_debug_handler();
        #endif
            }
        }
        pNew->renderPoint = pData->renderPoint;
        for (int i = 0; i < codecO.fSample; i++) {
            pNew->buffer[2 * i + channel] = pData->buffer[i];
        }
        pStart.next = pNew;
        pNew->next  = NULL;
        irq_restore(irq);
        u32 capture_tick_stimer = pData->renderPoint - clock_time();
        u32 capture_tick_timer  = (capture_tick_stimer * sys_clk.pclk) / SYSTEM_TIMER_TICK_1US;
        ble_audio_timer_set_capture(TIMER0, 0, capture_tick_timer);
        APP_DBG_CHN_9_LOW;
        return;
    } else {
        struct list_node_t *pTemp = &pStart;
        while (pTemp->next != NULL) {
            pTemp = pTemp->next;
            APP_DBG_CHN_13_HIGH;
            APP_DBG_CHN_13_LOW;
            if (pData->renderPoint == pTemp->renderPoint) {
                APP_DBG_CHN_11_HIGH;
                for (int i = 0; i < codecO.fSample; i++) {
                    pTemp->buffer[2 * i + channel] = pData->buffer[i];
                }
                irq_restore(irq);
                APP_DBG_CHN_11_LOW;
                return;
            }
        }
        APP_DBG_CHN_10_HIGH;
        struct list_node_t *pNew = (struct list_node_t *)tlk_mem_malloc(sizeof(struct list_node_t));
        if (pNew == NULL) {
            APP_DBG_CHN_15_HIGH;
            APP_DBG_CHN_15_LOW;
            tlkapi_printf(APP_LOG_EN, "mempool malloc failed,pos2!");
            while (1) {
        ////////////////////////////////////// Debug entry /////////////////////////////////
        #if (TLKAPI_DEBUG_ENABLE)
                tlkapi_debug_handler();
        #endif
            }
        }
        pNew->renderPoint = pData->renderPoint;
        for (int i = 0; i < codecO.fSample; i++) {
            pNew->buffer[2 * i + channel] = pData->buffer[i];
        }
        pTemp->next = pNew;
        pNew->next  = NULL;
        irq_restore(irq);
        APP_DBG_CHN_10_LOW;
    }
}


    #elif (APP_AUDIO_SCENE == APP_SCENE_HEADSET)

_attribute_ram_code_ void app_list_add(audio_pkt_t *pData) //audio data list add
{
    if (((unsigned int)(pData->renderPoint - clock_time())) > 200 * SYSTEM_TIMER_TICK_1MS) {
        APP_DBG_CHN_14_HIGH;
        APP_DBG_CHN_14_LOW;
        return;
    }
    struct list_node_t *pTemp = &pStart;
    APP_DBG_CHN_9_HIGH;
    u32 irq = irq_disable();
    if (pTemp->next == NULL) {
        u32 capture_tick_stimer = pData->renderPoint - clock_time();
        u32 capture_tick_timer  = (capture_tick_stimer * sys_clk.pclk) / SYSTEM_TIMER_TICK_1US;
        ble_audio_timer_set_capture(TIMER0, 0, capture_tick_timer);
    }
    while (pTemp->next != NULL) {
        pTemp = pTemp->next;
    }
    struct list_node_t *pNew = (struct list_node_t *)tlk_mem_malloc(sizeof(struct list_node_t));
    if (pNew == NULL) {
        APP_DBG_CHN_15_HIGH;
        APP_DBG_CHN_15_LOW;
        tlkapi_printf(APP_LOG_EN, "mempool malloc failed!");
        while (1) {
        #if (TLKAPI_DEBUG_ENABLE)
            tlkapi_debug_handler();
        #endif
        }
    }
    pNew->renderPoint = pData->renderPoint;
    memcpy((u8 *)pNew->buffer, (u8 *)pData->buffer, codecO.fOctets);
    pTemp->next = pNew;
    pNew->next  = NULL;
    irq_restore(irq);
    APP_DBG_CHN_9_LOW;
}
    #endif


#endif
