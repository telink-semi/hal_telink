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

#include "app_config.h"
#include "app_audio.h"
#include "app_codec.h"

#if (INTER_TEST_MODE == TEST_CIS_AUDIO_SERVER)


u32 appTimerState = 0;

    #if (APP_SCENE == APP_SCENE_TWS)
struct list_node_mono_t pStart;
    #elif (APP_SCENE == APP_SCENE_HEADSET_EP1_MULTIPLEXING)
struct list_node_stereo_t pStart;
    #elif (APP_SCENE == APP_SCENE_HEADSET_EP2)
struct list_node_stereo_t pStart;
    #endif

unsigned short app_audio_output_buffer[APP_AUDIO_OUTPUT_BUFFER_SIZE];
unsigned short app_audio_input_buffer[APP_AUDIO_INPUT_BUFFER_SIZE];

extern app_audio_ctrl_t appCtrl;

void app_codec_init(void)
{
    tlk_codec_init();
    appTimerState = APP_STATE_NONE;
    ble_audio_timer_init(TIMER0);
    tlk_mem_pool_desc_t poolDesc[] =
        {
            {980, 12},
    };
    u8  numPools = sizeof(poolDesc) / sizeof(poolDesc[0]);
    u32 ret      = tlk_mempool_init(numPools, poolDesc);
    if (ret != TLK_MEM_SUCCESS) {
        BLT_APP_LOG("error-mempool init failed! %d", ret);
    } else {
        BLT_APP_LOG("mempool init success!");
    }
}

void app_config_codec(void)
{
    /*********************************config codec********************************/
    u8 sinkCnt       = 0;
    u8 sinkFrequency = 0;
    u8 sinkDuration  = 0;
    u8 sinkBlocks    = 0;

    u8 sourceCnt       = 0;
    u8 sourceFrequency = 0;
    u8 sourceDuration  = 0;
    u8 sourceBlocks    = 0;

    for (u8 i = 0; i < APP_AUDIO_MAX_SINK_EP; i++) //ep sink-codec output
    {
        if (appCtrl.sink[i].cP.paramReady) {
            sinkCnt++;
            if ((sinkCnt > 1 && appCtrl.sink[i].cP.blocks > 1) || appCtrl.sink[i].cP.blocks > 2) {
                appCtrl.sink[i].epOp |= APP_EP_RELEASE; //not support more than 2 channel
            }
            if (sinkCnt > 1) {
                if (sinkFrequency != appCtrl.sink[i].cP.frequency || sinkDuration != appCtrl.sink[i].cP.duration || sinkBlocks != appCtrl.sink[i].cP.blocks) {
                    appCtrl.sink[i].epOp |= APP_EP_RELEASE; //not support asymmetric configure.
                }
            }
            sinkFrequency = appCtrl.sink[i].cP.frequency;
            sinkDuration  = appCtrl.sink[i].cP.duration;
            sinkBlocks    = appCtrl.sink[i].cP.blocks;
            if (sinkBlocks > 1) {
                for (u8 t = 0; t < sinkBlocks; t++) {
                    int lc3Ret = lc3dec_decode_init_bap(t, appCtrl.sink[i].cP.frequency, appCtrl.sink[i].cP.duration, appCtrl.sink[i].cP.frameOcts);
                    BLT_APP_LOG("lc3 dec config:frequency[%d],duration[%d],frameOcts[%d]", appCtrl.sink[i].cP.frequency, appCtrl.sink[i].cP.duration, appCtrl.sink[i].cP.frameOcts);
                    if (lc3Ret != LC3DEC_OK) {
                        BLT_APP_LOG("error-lc3 encode fail ep_ID,ret:%d,%d", lc3Ret, appCtrl.sink[i].epId);
                        appCtrl.sink[i].epOp |= APP_EP_RELEASE; //not support asymmetric configure.
                        return;
                    }
                }
            } else {
                int lc3Ret = lc3dec_decode_init_bap(i, appCtrl.sink[i].cP.frequency, appCtrl.sink[i].cP.duration, appCtrl.sink[i].cP.frameOcts);
                BLT_APP_LOG("lc3 dec config:frequency[%d],duration[%d],frameOcts[%d]", appCtrl.sink[i].cP.frequency, appCtrl.sink[i].cP.duration, appCtrl.sink[i].cP.frameOcts);
                if (lc3Ret != LC3DEC_OK) {
                    BLT_APP_LOG("error-lc3 encode fail ep_ID,ret:%d,%d", lc3Ret, appCtrl.sink[i].epId);
                    appCtrl.sink[i].epOp |= APP_EP_RELEASE; //not support asymmetric configure.
                    return;
                }
            }
        }
    }
    if (sinkCnt) {
        if (sinkBlocks == 2 || sinkCnt == 2) {
            appCtrl.codecO.cC = TLK_CODEC_2_CHANNEL;
        } else {
            appCtrl.codecO.cC = TLK_CODEC_1_CHANNEL;
        }
        tlk_codec_sts_e codecRet = tlk_codec_config(TLK_CODEC_OUTPUT, sinkFrequency, appCtrl.codecO.cC, (u8 *)app_audio_output_buffer, 2 * APP_AUDIO_OUTPUT_BUFFER_SIZE);
        if (codecRet != TLK_CODEC_SUCCESS) {
            BLT_APP_LOG("error-codec config fail ret:%d", codecRet);
            for (u8 i = 0; i < APP_AUDIO_MAX_SINK_EP; i++) {
                appCtrl.sink[i].epOp |= APP_EP_RELEASE;
            }
        }
        BLT_APP_LOG("codec output config:frequency[%d],channel[%d]", sinkFrequency, appCtrl.codecO.cC);
        appCtrl.codecO.fSample = appCtrl.codecO.cC * gLc3Index[sinkFrequency][sinkDuration].frameSample;
        appCtrl.codecO.fOctets = appCtrl.codecO.cC * gLc3Index[sinkFrequency][sinkDuration].frameOctets;
        appCtrl.codecO.frameUs = gLc3Index[sinkFrequency][sinkDuration].duraUs;
        BLT_APP_LOG("output:frame sample %d,frame octets %d,frame duration %d", appCtrl.codecO.fSample, appCtrl.codecO.fOctets, appCtrl.codecO.frameUs);
    }
    for (u8 i = 0; i < APP_AUDIO_MAX_SOURCE_EP; i++) //ep source-codec input
    {
        if (appCtrl.source[i].cP.paramReady) {
            sourceCnt++;
            if ((sourceCnt > 1 && appCtrl.source[i].cP.blocks > 1) || appCtrl.source[i].cP.blocks > 2) {
                appCtrl.source[i].epOp |= APP_EP_RELEASE; //not support more than 2 channel
            }
            if (sourceCnt > 1) {
                if (sinkFrequency != appCtrl.source[i].cP.frequency || sinkDuration != appCtrl.source[i].cP.duration || sourceBlocks != appCtrl.source[i].cP.blocks) {
                    appCtrl.source[i].epOp |= APP_EP_RELEASE; //not support asymmetric configure.
                }
            }
            sourceFrequency = appCtrl.source[i].cP.frequency;
            sourceDuration  = appCtrl.source[i].cP.duration;
            sourceBlocks    = appCtrl.source[i].cP.blocks;
            if (sourceBlocks > 1) {
                for (u8 t = 0; t < sourceBlocks; t++) {
                    int lc3Ret = lc3enc_encode_init_bap(t, appCtrl.source[i].cP.frequency, appCtrl.source[i].cP.duration, appCtrl.source[i].cP.frameOcts);
                    BLT_APP_LOG("lc3 enc config:frequency[%d],duration[%d],frameOcts[%d]", appCtrl.source[i].cP.frequency, appCtrl.source[i].cP.duration, appCtrl.source[i].cP.frameOcts);
                    if (lc3Ret != LC3DEC_OK) {
                        BLT_APP_LOG("error-lc3 encode fail ep_ID,ret:%d,%d", lc3Ret, appCtrl.sink[i].epId);
                        appCtrl.source[i].epOp |= APP_EP_RELEASE; //not support asymmetric configure.
                        return;
                    }
                }
            } else {
                int lc3Ret = lc3enc_encode_init_bap(i, appCtrl.source[i].cP.frequency, appCtrl.source[i].cP.duration, appCtrl.source[i].cP.frameOcts);
                BLT_APP_LOG("lc3 enc config:frequency[%d],duration[%d],frameOcts[%d]", appCtrl.source[i].cP.frequency, appCtrl.source[i].cP.duration, appCtrl.source[i].cP.frameOcts);
                if (lc3Ret != LC3DEC_OK) {
                    BLT_APP_LOG("error-lc3 encode fail ep_ID,ret:%d,%d", lc3Ret, appCtrl.sink[i].epId);
                    appCtrl.source[i].epOp |= APP_EP_RELEASE; //not support asymmetric configure.
                    return;
                }
            }
        }
    }
    if (sourceCnt) {
        if (sourceBlocks == 2 || sourceCnt == 2) {
            appCtrl.codecI.cC = TLK_CODEC_2_CHANNEL;
        } else {
            appCtrl.codecI.cC = TLK_CODEC_1_CHANNEL;
        }
        tlk_codec_sts_e codecRet = tlk_codec_config(TLK_CODEC_INPUT, sourceFrequency, appCtrl.codecI.cC, (u8 *)app_audio_input_buffer, 2 * APP_AUDIO_INPUT_BUFFER_SIZE);
        if (codecRet != TLK_CODEC_SUCCESS) {
            BLT_APP_LOG("error-codec config fail ret:%d", codecRet);
            for (u8 i = 0; i < APP_AUDIO_MAX_SOURCE_EP; i++) {
                appCtrl.source[i].epOp |= APP_EP_RELEASE;
            }
        }
        BLT_APP_LOG("codec input config:frequency[%d],channel[%d]", sourceFrequency, appCtrl.codecI.cC);
        appCtrl.codecI.fSample = appCtrl.codecI.cC * gLc3Index[sourceFrequency][sourceDuration].frameSample;
        appCtrl.codecI.fOctets = appCtrl.codecI.cC * gLc3Index[sourceFrequency][sourceDuration].frameOctets;
        appCtrl.codecI.frameUs = gLc3Index[sourceFrequency][sourceDuration].duraUs;
        BLT_APP_LOG("input:frame sample %d,frame octets %d,frame duration %d", appCtrl.codecI.fSample, appCtrl.codecI.fOctets, appCtrl.codecI.frameUs);
    }
}


    #if (APP_SCENE == APP_SCENE_TWS)
_attribute_ram_code_ void app_list_add_mono_ep1(audio_pkt_mono_t *pData) //audio data list add
{
    struct list_node_mono_t *pTemp = &pStart;
    if (pTemp->next == NULL) {
        u32 capture_tick_stimer = pData->renderPoint - clock_time();
        u32 capture_tick_timer  = (capture_tick_stimer * sys_clk.pclk) / SYSTEM_TIMER_TICK_1US;
        ble_audio_timer_set_capture(TIMER0, 0, capture_tick_timer);
        appTimerState = APP_STATE_RENDER_START;
    }
    while (pTemp->next != NULL) {
        pTemp = pTemp->next;
    }

    struct list_node_mono_t *pNew = (struct list_node_mono_t *)tlk_mem_malloc(sizeof(struct list_node_mono_t));
    if (pNew == NULL) {
        BLT_APP_LOG("mempool malloc failed!");
        while (1) {
        #if (TLKAPI_DEBUG_ENABLE)
            tlkapi_debug_handler();
        #endif
        }
    }
    pNew->renderPoint = pData->renderPoint;
    memcpy((u8 *)pNew->buffer, (u8 *)pData->buffer, appCtrl.codecO.fOctets);
    pTemp->next = pNew;
    pNew->next  = NULL;
}

_attribute_ram_code_ bool app_list_delete(struct list_node_mono_t *pData) //audio data playback over, delete node.
{
    struct list_node_mono_t *pTemp = &pStart;
    while (pTemp->next != NULL) {
        if (pTemp->next == pData) {
            if (pTemp->next->next != NULL) {
                struct list_node_mono_t *pDel = pTemp->next;
                pTemp->next                   = pTemp->next->next;
                memset((u8 *)pDel->buffer, 0, APP_AUDIO_MAX_SINK_EP * APP_AUDIO_SUPPORT_MAX_FRAME_SAMPLE);
                TLK_MEM_STATE_T memRet = tlk_mem_free(pDel);
                if (memRet != TLK_MEM_SUCCESS) {
                    BLT_APP_LOG("mempool free failed,pos1!");
                }
                return true;
            } else {
                struct list_node_mono_t *pDel = pTemp->next;
                pTemp->next                   = NULL;
                memset((u8 *)pDel->buffer, 0, APP_AUDIO_MAX_SINK_EP * APP_AUDIO_SUPPORT_MAX_FRAME_SAMPLE);
                TLK_MEM_STATE_T memRet = tlk_mem_free(pDel);
                if (memRet != TLK_MEM_SUCCESS) {
                    BLT_APP_LOG("mempool free failed,pos2!");
                }
                return true;
            }
        }
        pTemp = pTemp->next;
    }
    return false;
}
    #else

_attribute_ram_code_ void app_list_add_stereo_ep1(audio_pkt_stereo_t *pData) //audio data list add
{
    struct list_node_stereo_t *pTemp = &pStart;
    if (pTemp->next == NULL) {
        u32 capture_tick_stimer = pData->renderPoint - clock_time();
        u32 capture_tick_timer  = (capture_tick_stimer * sys_clk.pclk) / SYSTEM_TIMER_TICK_1US;
        ble_audio_timer_set_capture(TIMER0, 0, capture_tick_timer);
        appTimerState = APP_STATE_RENDER_START;
    }
    while (pTemp->next != NULL) {
        pTemp = pTemp->next;
    }
    struct list_node_stereo_t *pNew = (struct list_node_stereo_t *)tlk_mem_malloc(sizeof(struct list_node_stereo_t));
    if (pNew == NULL) {
        BLT_APP_LOG("mempool malloc failed!");
        while (1) {
        #if (TLKAPI_DEBUG_ENABLE)
            tlkapi_debug_handler();
        #endif
        }
    }
    pNew->renderPoint = pData->renderPoint;
    memcpy((u8 *)pNew->buffer, (u8 *)pData->buffer, appCtrl.codecI.fOctets);
    pTemp->next = pNew;
    pNew->next  = NULL;
}

_attribute_ram_code_ bool app_list_delete(struct list_node_stereo_t *pData) //audio data playback over, delete node.
{
    struct list_node_stereo_t *pTemp = &pStart;
    while (pTemp->next != NULL) {
        if (pTemp->next == pData) {
            if (pTemp->next->next != NULL) {
                struct list_node_stereo_t *pDel = pTemp->next;
                pTemp->next                     = pTemp->next->next;
                memset((u8 *)pDel->buffer, 0, APP_AUDIO_MAX_SINK_EP * APP_AUDIO_SUPPORT_MAX_FRAME_SAMPLE);
                TLK_MEM_STATE_T memRet = tlk_mem_free(pDel);
                if (memRet != TLK_MEM_SUCCESS) {
                    BLT_APP_LOG("mempool free failed,pos1!");
                }
                return true;
            } else {
                struct list_node_stereo_t *pDel = pTemp->next;
                pTemp->next                     = NULL;
                memset((u8 *)pDel->buffer, 0, APP_AUDIO_MAX_SINK_EP * APP_AUDIO_SUPPORT_MAX_FRAME_SAMPLE);
                TLK_MEM_STATE_T memRet = tlk_mem_free(pDel);
                if (memRet != TLK_MEM_SUCCESS) {
                    BLT_APP_LOG("mempool free failed,pos2!");
                }
                return true;
            }
        }
        pTemp = pTemp->next;
    }
    return false;
}


    #endif

_attribute_ram_code_ void app_list_free(void)
{
    while (pStart.next != NULL) {
        app_list_delete(pStart.next);
    }
}

void app_codec_send_process(void)
{
    if (tlk_codec_getState(TLK_CODEC_INPUT) != TLK_CODEC_STATE_STREAMING) {
        return;
    }

    #if (APP_SCENE == APP_SCENE_TWS)
    u16 pcmData[APP_AUDIO_SUPPORT_MAX_FRAME_SAMPLE] = {0};
    if (tlk_codec_input_dataPop((u8 *)pcmData, appCtrl.codecI.fOctets) == TLK_CODEC_SUCCESS) {
        u8 audio_enc[APP_AUDIO_SUPPORT_MAX_ENCODE_FRAME_BYTES] = {0};
        for (u8 i = 0; i < APP_AUDIO_MAX_SOURCE_EP; i++) {
            if (appCtrl.source[i].sS) {
                APP_DBG_CHN_1_HIGH;
                LC3ENC_Error ret_lc3 = lc3enc_encode_pkt(i, (u8 *)pcmData, audio_enc);
                if (ret_lc3 != LC3ENC_OK) {
                    BLT_APP_LOG("lc3 encode error:0x%x", ret_lc3);
                }
                int ret = blc_bapus_sduPacketPush(appCtrl.aclHandle, appCtrl.source[i].epId, audio_enc, appCtrl.source[i].cP.frameOcts);
                if (ret != AUDIO_ESUCC) {
                    BLT_APP_LOG("cis send fail-ret: %d", ret);
                }
                APP_DBG_CHN_1_LOW;
            }
        }
    } else {
        return;
    }
    #elif (APP_SCENE == APP_SCENE_HEADSET_EP1_MULTIPLEXING)
    u16 pcmData[2 * APP_AUDIO_SUPPORT_MAX_FRAME_SAMPLE] = {0};
    if (tlk_codec_input_dataPop((u8 *)pcmData, appCtrl.codecI.fOctets) == TLK_CODEC_SUCCESS) {
        u16 audio_pcm[APP_AUDIO_SUPPORT_MAX_FRAME_SAMPLE]           = {0};
        u8  audio_enc[2 * APP_AUDIO_SUPPORT_MAX_ENCODE_FRAME_BYTES] = {0};
        for (u8 i = 0; i < APP_AUDIO_MAX_SOURCE_EP; i++) {
            if (appCtrl.source[i].sS) {
                for (u8 j = 0; j < appCtrl.source[i].cP.blocks; j++) {
                    for (u16 t = 0; t < appCtrl.codecI.fSample / 2; t++) {
                        audio_pcm[t] = pcmData[2 * t + j];
                    }
                    LC3ENC_Error ret_lc3 = lc3enc_encode_pkt(j, (u8 *)audio_pcm, audio_enc[i] + j * appCtrl.source[i].cP.frameOcts);
                    if (ret_lc3 != LC3ENC_OK) {
                        BLT_APP_LOG("lc3 encode error:0x%x", ret_lc3);
                    }
                }
                int ret = blc_bapus_sduPacketPush(appCtrl.aclHandle, appCtrl.source[i].epId, audio_enc, appCtrl.source[i].cP.blocks * appCtrl.source[i].cP.frameOcts);
                if (ret != AUDIO_ESUCC) {
                    BLT_APP_LOG("cis send fail-ret: %d", ret);
                }
            }
        }
    } else {
        return;
    }

    #elif (APP_SCENE == APP_SCENE_HEADSET_EP2)
    u16 pcmData[2 * APP_AUDIO_SUPPORT_MAX_FRAME_SAMPLE] = {0};
    if (tlk_codec_input_dataPop((u8 *)pcmData, appCtrl.codecI.fOctets) == TLK_CODEC_SUCCESS) {
        u16 audio_pcm[APP_AUDIO_SUPPORT_MAX_FRAME_SAMPLE]                                = {0};
        u8  audio_enc[APP_AUDIO_MAX_SOURCE_EP][APP_AUDIO_SUPPORT_MAX_ENCODE_FRAME_BYTES] = {0};
        for (u8 i = 0; i < APP_AUDIO_MAX_SOURCE_EP; i++) {
            if (appCtrl.source[i].sS) {
                for (u16 j = 0; j < appCtrl.codecI.fSample / 2; j++) {
                    audio_pcm[t] = pcmData[2 * i + j];
                }
                LC3ENC_Error ret_lc3 = lc3enc_encode_pkt(i, (u8 *)audio_pcm, audio_enc[i]);
                if (ret_lc3 != LC3ENC_OK) {
                    BLT_APP_LOG("lc3 encode error:0x%x", ret_lc3);
                }
            }
        }
        for (u8 i = 0; i < APP_AUDIO_MAX_SOURCE_EP; i++) {
            if (appCtrl.source[i].sS) {
                int ret = blc_bapus_sduPacketPush(appCtrl.aclHandle, appCtrl.source[i].epId, audio_enc[i], appCtrl.source[i].cP.frameOcts);
                if (ret != AUDIO_ESUCC) {
                    BLT_APP_LOG("cis send fail-ret: %d", ret);
                }
            }
        }
    } else {
        return;
    }
    #endif
}

void app_codec_receive_process(void)
{
    if (tlk_codec_getState(TLK_CODEC_OUTPUT) != TLK_CODEC_STATE_STREAMING) {
        return;
    }
    #if (APP_SCENE == APP_SCENE_TWS)
    for (u8 i = 0; i < APP_AUDIO_MAX_SINK_EP; i++) {
        if (appCtrl.sink[i].sS) {
            sdu_packet_t *pPkt = blc_bapus_sduPacketPop(appCtrl.aclHandle, appCtrl.sink[i].epId);
            if (pPkt != NULL) {
                u32 detect = 0;
                if (pPkt->iso_sdu_len != appCtrl.sink[i].cP.frameOcts) {
                    if (!appCtrl.sink[i].sT) {
                        APP_DBG_CHN_5_HIGH;
                        APP_DBG_CHN_5_LOW;
                        return;
                    } else {
                        APP_DBG_CHN_6_HIGH;
                        APP_DBG_CHN_6_LOW;
                        detect = 1;
                    }
                } else {
                    APP_DBG_CHN_7_HIGH;
                    APP_DBG_CHN_7_LOW;
                    appCtrl.sink[i].sT = clock_time() | 1;
                }
                LC3DEC_Error ret_lc3 = lc3dec_set_parameter(i, LC3_PARA_BEC_DETECT, &detect);
                if (ret_lc3 != LC3DEC_OK) {
                    BLT_APP_LOG("lc3 decode set parameter error: %d", ret_lc3);
                    return;
                }
                APP_DBG_CHN_2_HIGH;
                audio_pkt_mono_t pRaw = {0};
                ret_lc3               = lc3dec_decode_pkt(i, pPkt->data, pPkt->iso_sdu_len, (u8 *)pRaw.buffer);
                if (ret_lc3 != LC3DEC_OK) {
                    BLT_APP_LOG("lc3 decode error-ret: %d", ret_lc3);
                    return;
                }
                pRaw.renderPoint = pPkt->timestamp + appCtrl.sink[i].pD * SYSTEM_TIMER_TICK_1US; //render point,tick count.
                app_list_add_mono_ep1(&pRaw);
                APP_DBG_CHN_2_LOW;
            }
        }
    }
    #elif (APP_SCENE == APP_SCENE_HEADSET_EP1_MULTIPLEXING)
    for (u8 i = 0; i < APP_AUDIO_MAX_SINK_EP; i++) {
        if (appCtrl.sink[i].sS) {
            sdu_packet_t *pPkt = blc_bapus_sduPacketPop(appCtrl.aclHandle, appCtrl.sink[i].epId);

            if (pPkt != NULL) {
                u32 detect = 0;
                if (pPkt->iso_sdu_len != appCtrl.sink[i].cP.blocks * appCtrl.sink[i].cP.frameOcts) {
                    if (!appCtrl.sink[i].sT) {
                        return;
                    } else {
                        detect = 1;
                    }
                } else {
                    appCtrl.sink[i].sT = clock_time() | 1;
                }
                audio_pkt_stereo_t pRaw                                          = {0};
                u16                audio_pcm[APP_AUDIO_SUPPORT_MAX_FRAME_SAMPLE] = {0};
                for (u8 j = 0; j < appCtrl.sink[i].cP.blocks; j++) {
                    LC3DEC_Error ret_lc3 = lc3dec_set_parameter(j, LC3_PARA_BEC_DETECT, &detect);
                    if (ret_lc3 != LC3DEC_OK) {
                        BLT_APP_LOG("lc3 decode set parameter error: %d", ret_lc3);
                        return;
                    }
                    APP_DBG_CHN_2_HIGH;
                    ret_lc3 = lc3dec_decode_pkt(j, pPkt->data + j * appCtrl.sink[i].cP.frameOcts, appCtrl.sink[i].cP.frameOcts, (u8 *)audio_pcm);
                    if (ret_lc3 != LC3DEC_OK) {
                        BLT_APP_LOG("lc3 decode error-ret: %d", ret_lc3);
                        return;
                    }
                    for (t = 0; t < appCtrl.codecO.fSample / 2; t++) {
                        pRaw.buffer[appCtrl.sink[i].cP.blocks * t + j] = audio_pcm[t];
                    }
                    APP_DBG_CHN_2_LOW;
                }
                pRaw.renderPoint = pPkt->timestamp + appCtrl.sink[i].pD * SYSTEM_TIMER_TICK_1US; //render point,tick count.
                app_list_add_stereo_ep1(&pRaw);
            }
        }
    }

    #elif (APP_SCENE == APP_SCENE_HEADSET_EP2)
    for (u8 i = 0; i < APP_AUDIO_MAX_SINK_EP; i++) {
        if (appCtrl.sink[i].sS) {
            sdu_packet_t *pPkt = blc_bapus_sduPacketPop(appCtrl.aclHandle, appCtrl.sink[i].epId);

            if (pPkt != NULL) {
                u32 detect = 0;
                if (pPkt->iso_sdu_len != appCtrl.sink[i].cP.frameOcts) {
                    if (!appCtrl.sink[i].sT) {
                        return;
                    } else {
                        detect = 1;
                    }
                } else {
                    appCtrl.sink[i].sT = clock_time() | 1;
                }
                LC3DEC_Error ret_lc3 = lc3dec_set_parameter(i, LC3_PARA_BEC_DETECT, &detect);
                if (ret_lc3 != LC3DEC_OK) {
                    BLT_APP_LOG("lc3 decode set parameter error: %d", ret_lc3);
                    return;
                }
                APP_DBG_CHN_2_HIGH;
                audio_pkt_mono_t pRaw = {0};
                ret_lc3               = lc3dec_decode_pkt(i, pPkt->data, pPkt->iso_sdu_len, (u8 *)pRaw.buffer);
                if (ret_lc3 != LC3DEC_OK) {
                    BLT_APP_LOG("lc3 decode error-ret: %d", ret_lc3);
                    return;
                }
                pRaw.renderPoint = pPkt->timestamp + appCtrl.sink[i].pD * SYSTEM_TIMER_TICK_1US; //render point,tick count.
                app_list_add_mono_ep2(&pRaw, i);
                APP_DBG_CHN_2_LOW;
            }
        }
    }
    #endif
}

_attribute_ram_code_ void app_timer_irq_proc(void)
{
    timer_stop(TIMER0);
    if (appTimerState == APP_STATE_RENDER_START) {
        u32 readOffset  = 0;
        u32 writeOffset = 0;
        if (tlk_codec_output_getOffset(&writeOffset, &readOffset) != TLK_CODEC_SUCCESS) {
            app_list_free();
            //            BLT_APP_LOG("get offset error");
            return;
        }
        APP_DBG_CHN_3_HIGH;
        //        BLT_APP_LOG("start readOffset[%d] writeOffset[%d]",readOffset,writeOffset);
        if (tlk_codec_output_setWriteOffset(readOffset + 32) != TLK_CODEC_SUCCESS) {
            app_list_free();
            //            BLT_APP_LOG("set offset error");
            return;
        }
        if (tlk_codec_output_dataPush((u8 *)pStart.next->buffer, appCtrl.codecO.fOctets) != TLK_CODEC_SUCCESS) {
            app_list_free();
            //            BLT_APP_LOG("data push error");
            return;
        }
        blc_audio_clock_calib(pStart.next->renderPoint, appCtrl.codecO.frameUs * SYSTEM_TIMER_TICK_1US);
        if (app_list_delete(pStart.next) == 0) {
            //            BLT_APP_LOG("memory free failed-start");
        }
        if (pStart.next != NULL) {
            u32 capture_tick_stimer = pStart.next->renderPoint - clock_time();
            u32 capture_tick_timer  = (capture_tick_stimer * sys_clk.pclk) / SYSTEM_TIMER_TICK_1US;
            ble_audio_timer_set_capture(TIMER0, 0, capture_tick_timer);
            appTimerState = APP_STATE_RENDER_CONTINUE;
        } else {
            appTimerState = APP_STATE_NONE;
        }
        APP_DBG_CHN_3_LOW;
    } else if (appTimerState == APP_STATE_RENDER_CONTINUE) {
        u32 readOffset  = 0;
        u32 writeOffset = 0;
        APP_DBG_CHN_4_HIGH;
        if (tlk_codec_output_getOffset(&writeOffset, &readOffset) != TLK_CODEC_SUCCESS) {
            app_list_free();
            //              BLT_APP_LOG("get offset error");
            return;
        }
        //      BLT_APP_LOG("continue readOffset[%d] writeOffset[%d]",readOffset,writeOffset);
        if (readOffset > writeOffset) {
            if (readOffset > APP_AUDIO_OUTPUT_BUFFER_SIZE && writeOffset < APP_AUDIO_OUTPUT_BUFFER_SIZE) {
                //nothing to do
            } else {
                if (tlk_codec_output_setWriteOffset(readOffset + 32) != TLK_CODEC_SUCCESS) {
                    app_list_free();
                    //                      BLT_APP_LOG("set offset error");
                    return;
                }
                //                BLT_APP_LOG("jump sample - readOffset:0x%x", readOffset);
                //                BLT_APP_LOG("jump sample - writeOffset:0x%x", writeOffset);
            }
        }

        if (tlk_codec_output_dataPush((u8 *)pStart.next->buffer, appCtrl.codecO.fOctets) != TLK_CODEC_SUCCESS) {
            app_list_free();
            //              BLT_APP_LOG("data push error");
            return;
        }
        blc_audio_clock_calib(pStart.next->renderPoint, appCtrl.codecO.frameUs * SYSTEM_TIMER_TICK_1US);
        if (app_list_delete(pStart.next) == 0) {
            //          BLT_APP_LOG("memory free failed-continue");
        }
        if (pStart.next != NULL) {
            u32 capture_tick_stimer = pStart.next->renderPoint - clock_time();
            u32 capture_tick_timer  = (capture_tick_stimer * sys_clk.pclk) / SYSTEM_TIMER_TICK_1US;
            ble_audio_timer_set_capture(TIMER0, 0, capture_tick_timer);
            appTimerState = APP_STATE_RENDER_CONTINUE;
        } else {
            appTimerState = APP_STATE_NONE;
        }
        APP_DBG_CHN_4_LOW;
    } else {
    }
}

void app_codec_handler(void)
{
    app_codec_receive_process();
    app_codec_send_process();
    if (appCtrl.configCodecIdx) {
        app_config_codec();
        appCtrl.configCodecIdx = false;
    }
}


#endif /* INTER_TEST_MODE */
