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
#include "app_audio_ctrl.h"


#if (INTER_TEST_MODE == TEST_LE_AUDIO_SWTICH_SERVER)
#if (TLK_TONE_ENABLE)
    #include "tlk_api/tlk_tone.h"
#endif
#include "tlk_api/tlk_codec.h"
#include "tlk_api/tlk_mem.h"
#include "algorithm/audio_alg/lc3/lc3.h"
#include "ext_driver/ext_audio.h"
struct __attribute__((packed)) list_node_t pStart;
unsigned short                             app_audio_output_buffer[APP_AUDIO_OUTPUT_BUFFER_SIZE];
unsigned short                             app_audio_input_buffer[APP_AUDIO_INPUT_BUFFER_SIZE];
app_codec_desc_t                           codecI;
app_codec_desc_t                           codecO;
extern app_audio_ctrl_t                    appCtrl;

    #define ALG_TIMING_DEBUG_EN 0
    #define ALG_BUFF_SIZE       1024 * 36
unsigned char alg_buff[ALG_BUFF_SIZE] = {0};


extern int                recvWptr;
extern int                recvRptr;
extern appSinkRecvSpeak_t recvAudioBuff[APP_SINK_RECV_SPEAK_FRAME_COUNT];
extern u16                codecSpeakBuff[APP_AUDIO_FRAME_BYTES * 2];

/**
 * @brief      Codec init function.
 * @param[in]  none.
 * @return     none.
 */
void app_codec_init(void)
{
    tlk_codec_init();
    ble_audio_timer_init(TIMER0);

    memset(recvAudioBuff, 0, sizeof(recvAudioBuff));

    pStart.next = NULL;
    #if ALG_GSC_EN | ALG_AEC_EN | ALG_S_NS
    int bf_aec_ns_version = tlka_aec_ns_get_version();
    BLT_APP_LOG("GSC init version inf:0x%x", bf_aec_ns_version);
    #endif

    #if ALG_AECM_EN
    int aecm_version = //tlka_aec_ns_get_version();
        BLT_APP_LOG("AECM init version inf:0x%x", aecm_version);
    #endif

    #if ALG_W_NS
    int w_ns_version = tlka_w_ns_get_version();
    BLT_APP_LOG("W_NS init version inf:0x%x", w_ns_version);
    #endif

    #if ALG_AGC_EN
    int agc_version = tlka_agc_get_version();
    BLT_APP_LOG("AGC init version inf:0x%x", agc_version);
    #endif

    #if ALG_GSC_EN
    // audio algrithm init
    int   ns         = 0;
    void *buff_tp    = alg_buff;
    int   frame_size = 80;
    int   bf_ram     = gsc_get_size(); //0x1470
    g_gsc_st_p       = buff_tp + ns;
    ns += bf_ram;

    int exchange_mic = 0;
    gsc_state_init(g_gsc_st_p, frame_size, exchange_mic);
    BLT_APP_LOG("GSC init end:0x%x,0x%x,0x%x", bf_ram, frame_size, exchange_mic);
    #endif

    #if ALG_AEC_EN | ALG_S_NS
    g_aec_st    = buff_tp + ns;
    int aec_ram = tlka_aec_get_size(); //0x1aa8
    ns += aec_ram;

    g_s_ns_den = buff_tp + ns;
    int ns_ram = tlka_ns_get_size(); //0x16c8
    ns += ns_ram;

    scratch_buff        = (void *)(buff_tp + ns);
    int ns_scratch_ram  = tlka_ns_get_scratch_size();
    int aec_scratch_ram = tlka_aec_get_scratch_size();
    int scratch_ram     = (ns_scratch_ram > aec_scratch_ram) ? ns_scratch_ram : aec_scratch_ram; //0xffc
    ns += scratch_ram;

    tlka_aec_init(g_aec_st, &aecParas, scratch_buff);
    tlka_ns_init(g_s_ns_den, &nsParas, scratch_buff);

    tlka_ns_set_parameter(g_s_ns_den, SPEEX_PREPROCESS_SET_ECHO_STATE, g_aec_st);

    tlk_aec_init(16000, 80);
    BLT_APP_LOG("AEC and S_NS init end:0x%x,0x%x,0x%x", aec_ram, ns_ram, scratch_ram);
    #endif

    #if ALG_W_NS
    w_ns_st           = buff_tp + ns;         //
    int webrtc_ns_ram = tlka_w_ns_get_size(); //0x1fa0
    ns += webrtc_ns_ram;

    w_ns_scratch_st           = buff_tp + ns;
    int webrtc_ns_scratch_ram = tlka_w_ns_get_scratch_size(); //0x1380
    ns += webrtc_ns_scratch_ram;

    W_nsParas.frame_size = 80;
    W_nsParas.sampleRate = 16000;

    tlka_w_ns_init(w_ns_st, W_nsParas, w_ns_scratch_st);
    BLT_APP_LOG("WebRtc_SYS_state scratch:0x%x,0x%x", webrtc_ns_ram, webrtc_ns_scratch_ram);
    #endif

    #if ALG_AGC_EN
    g_agc_st     = (buff_tp + ns);
    int agc_size = tlka_agc_get_size(); //0x1a8
    ns += agc_size;
    tlka_agc_init(g_agc_st, agc_param);
    BLT_APP_LOG("AGC init end:0x%x", agc_size);
    #endif


    #if AUDIO_DEBUG_DATA_EN
    void *p_buf    = buff_tp + ns;
    int   sbc_size = add_init(p_buf);
    ns += sbc_size;
    BLT_APP_LOG("SBC init end:0x%x", sbc_size);
    #endif

    #if (APP_SCENE == APP_SCENE_TWS)
    tlk_mem_pool_desc_t poolDesc[] =
        {
            {980, 8},
    };
    #elif (APP_SCENE == APP_SCENE_HEADSET_EP1_MULTIPLEXING)
    tlk_mem_pool_desc_t poolDesc[] =
        {
            {1960, 10},
    };
    #endif

    u8  numPools = sizeof(poolDesc) / sizeof(poolDesc[0]);
    u32 ret      = tlk_mempool_init(numPools, poolDesc);
    if (ret != TLK_MEM_SUCCESS) {
        tlkapi_printf(APP_LOG_EN, "error-mempool init failed! %d", ret);
    } else {
        tlkapi_printf(APP_LOG_EN, "mempool init success!");
    }
}

u16 app_get_one_in_data(u32 data)
{
    uint16_t n = 0;
    while (data > 0) {
        if (data & 0x01) {
            n++;
        }
        data >>= 1;
    }
    return n;
}

/**
 * @brief      Configure hardware codec and lc3.
 * @param[in]  none.
 * @return     none.
 */
void app_config_codec(void)
{
    /*********************************config codec********************************/
    u8 sinkCnt        = 0;
    u8 sinkFrequency  = 0;
    u8 sinkDuration   = 0;
    u8 sinkChannelNum = 0;

    u8 sourceCnt        = 0;
    u8 sourceFrequency  = 0;
    u8 sourceDuration   = 0;
    u8 sourceChannelNum = 0;

    for (u8 i = 0; i < APP_AUDIO_MAX_SINK_EP; i++) //ep sink-codec output
    {
        if (appCtrl.sink[i].cP.paramReady) {
            sinkCnt++;
            if ((sinkCnt > 1 && app_get_one_in_data(appCtrl.sink[i].cP.location) > 1) || app_get_one_in_data(appCtrl.sink[i].cP.location) > 2) {
                blc_bapus_aseRelease(appCtrl.aclHandle, appCtrl.sink[i].epId); //not support more than 2 channel
            }
            if (sinkCnt > 1) {
                if (sinkFrequency != appCtrl.sink[i].cP.frequency || sinkDuration != appCtrl.sink[i].cP.duration || sinkChannelNum != app_get_one_in_data(appCtrl.sink[i].cP.location)) {
                    blc_bapus_aseRelease(appCtrl.aclHandle, appCtrl.sink[i].epId); //not support asymmetric configure.
                }
            }
            sinkFrequency  = appCtrl.sink[i].cP.frequency;
            sinkDuration   = appCtrl.sink[i].cP.duration;
            sinkChannelNum = app_get_one_in_data(appCtrl.sink[i].cP.location);

            if (sinkChannelNum > 1) {
                for (u8 t = 0; t < sinkChannelNum; t++) {
                    int lc3Ret = lc3dec_decode_init_bap(t, appCtrl.sink[i].cP.frequency, appCtrl.sink[i].cP.duration, appCtrl.sink[i].cP.frameOcts);
                    tlkapi_printf(APP_LOG_EN, "lc3 dec channel[%d]:frequency[%d],duration[%d],frameOcts[%d]", t, appCtrl.sink[i].cP.frequency, appCtrl.sink[i].cP.duration, appCtrl.sink[i].cP.frameOcts);
                    if (lc3Ret != LC3DEC_OK) {
                        tlkapi_printf(APP_LOG_EN, "error-lc3 encode fail ep_ID,ret:%x", lc3Ret);
                        blc_bapus_aseRelease(appCtrl.aclHandle, appCtrl.sink[i].epId); //not support asymmetric configure.
                        return;
                    }
                }
            } else {
                int lc3Ret = lc3dec_decode_init_bap(i, appCtrl.sink[i].cP.frequency, appCtrl.sink[i].cP.duration, appCtrl.sink[i].cP.frameOcts);
                tlkapi_printf(APP_LOG_EN, "lc3 dec channel[%d]:frequency[%d],duration[%d],frameOcts[%d]", i, appCtrl.sink[i].cP.frequency, appCtrl.sink[i].cP.duration, appCtrl.sink[i].cP.frameOcts);
                if (lc3Ret != LC3DEC_OK) {
                    tlkapi_printf(APP_LOG_EN, "error-lc3 encode fail ep_ID,ret:%d,%d", lc3Ret, appCtrl.sink[i].epId);
                    blc_bapus_aseRelease(appCtrl.aclHandle, appCtrl.sink[i].epId); //not support asymmetric configure.
                    return;
                }
            }
        }
    }
    if (sinkCnt) {
        if (sinkChannelNum == 2 || sinkCnt == 2) {
            codecO.cC = TLK_CODEC_2_CHANNEL;
        } else {
            codecO.cC = TLK_CODEC_1_CHANNEL;
        }
        tlk_codec_sts_e codecRet = tlk_codec_config(TLK_CODEC_OUTPUT, sinkFrequency, codecO.cC, TLK_CODEC_LINE, (u8 *)app_audio_output_buffer, 2 * APP_AUDIO_OUTPUT_BUFFER_SIZE);
        if (codecRet != TLK_CODEC_SUCCESS) {
            tlkapi_printf(APP_LOG_EN, "error-codec config fail ret:%d", codecRet);
            for (u8 i = 0; i < APP_AUDIO_MAX_SINK_EP; i++) {
                blc_bapus_aseRelease(appCtrl.aclHandle, appCtrl.sink[i].epId);
            }
        }
        tlkapi_printf(APP_LOG_EN, "codec output:frequency[%d],channel[%d]", sinkFrequency, codecO.cC);
        codecO.fSample = codecO.cC * gLc3Index[sinkFrequency][sinkDuration].frameSample;
        codecO.fOctets = codecO.cC * gLc3Index[sinkFrequency][sinkDuration].frameOctets;
        codecO.frameUs = gLc3Index[sinkFrequency][sinkDuration].duraUs;
        tlkapi_printf(APP_LOG_EN, "output:frame sample %d,frame octets %d,frame duration %d", codecO.fSample, codecO.fOctets, codecO.frameUs);
    }
    for (u8 i = 0; i < APP_AUDIO_MAX_SOURCE_EP; i++) //ep source-codec input
    {
        if (appCtrl.source[i].cP.paramReady) {
            sourceCnt++;
            if ((sourceCnt > 1 && app_get_one_in_data(appCtrl.source[i].cP.location) > 1) || app_get_one_in_data(appCtrl.source[i].cP.location) > 2) {
                blc_bapus_aseRelease(appCtrl.aclHandle, appCtrl.source[i].epId); //not support more than 2 channel
            }
            if (sourceCnt > 1) {
                if (sinkFrequency != appCtrl.source[i].cP.frequency || sinkDuration != appCtrl.source[i].cP.duration || sourceChannelNum != app_get_one_in_data(appCtrl.source[i].cP.location)) {
                    blc_bapus_aseRelease(appCtrl.aclHandle, appCtrl.source[i].epId); //not support asymmetric configure.
                }
            }
            sourceFrequency  = appCtrl.source[i].cP.frequency;
            sourceDuration   = appCtrl.source[i].cP.duration;
            sourceChannelNum = app_get_one_in_data(appCtrl.source[i].cP.location);
            if (sourceChannelNum > 1) {
                for (u8 t = 0; t < sourceChannelNum; t++) {
                    int lc3Ret = lc3enc_encode_init_bap(t, appCtrl.source[i].cP.frequency, appCtrl.source[i].cP.duration, appCtrl.source[i].cP.frameOcts);
                    tlkapi_printf(APP_LOG_EN, "lc3 enc channel[%d]:frequency[%d],duration[%d],frameOcts[%d]", t, appCtrl.source[i].cP.frequency, appCtrl.source[i].cP.duration, appCtrl.source[i].cP.frameOcts);
                    if (lc3Ret != LC3DEC_OK) {
                        tlkapi_printf(APP_LOG_EN, "error-lc3 encode fail ep_ID,ret:%d,%d", lc3Ret, appCtrl.sink[i].epId);
                        blc_bapus_aseRelease(appCtrl.aclHandle, appCtrl.source[i].epId); //not support asymmetric configure.
                        return;
                    }
                }
            } else {
                int lc3Ret = lc3enc_encode_init_bap(i, appCtrl.source[i].cP.frequency, appCtrl.source[i].cP.duration, appCtrl.source[i].cP.frameOcts);
                tlkapi_printf(APP_LOG_EN, "lc3 enc channel[%d]:frequency[%d],duration[%d],frameOcts[%d]", i, appCtrl.source[i].cP.frequency, appCtrl.source[i].cP.duration, appCtrl.source[i].cP.frameOcts);
                if (lc3Ret != LC3DEC_OK) {
                    tlkapi_printf(APP_LOG_EN, "error-lc3 encode fail ep_ID,ret:%d,%d", lc3Ret, appCtrl.sink[i].epId);
                    blc_bapus_aseRelease(appCtrl.aclHandle, appCtrl.source[i].epId); //not support asymmetric configure.
                    return;
                }
            }
        }
    }
    if (sourceCnt) {
        if (sourceChannelNum == 2 || sourceCnt == 2) {
            codecI.cC = TLK_CODEC_2_CHANNEL;
        } else {
            codecI.cC = TLK_CODEC_1_CHANNEL;
        }

        tlk_codec_sts_e codecRet = tlk_codec_config(TLK_CODEC_INPUT, sourceFrequency, codecI.cC, TLK_CODEC_LINE, (u8 *)app_audio_input_buffer, 2 * APP_AUDIO_INPUT_BUFFER_SIZE);
        if (codecRet != TLK_CODEC_SUCCESS) {
            tlkapi_printf(APP_LOG_EN, "error-codec config fail ret:%d", codecRet);
            for (u8 i = 0; i < APP_AUDIO_MAX_SOURCE_EP; i++) {
                blc_bapus_aseRelease(appCtrl.aclHandle, appCtrl.source[i].epId);
            }
        }
        tlkapi_printf(APP_LOG_EN, "codec input:frequency[%d],channel[%d]", sourceFrequency, codecI.cC);
        codecI.fSample = codecI.cC * gLc3Index[sourceFrequency][sourceDuration].frameSample;
    #if ALG_GSC_EN
        codecI.fOctets = codecI.cC * gLc3Index[sourceFrequency][sourceDuration].frameOctets * 2;
    #else
        codecI.fOctets = codecI.cC * gLc3Index[sourceFrequency][sourceDuration].frameOctets;
    #endif
        codecI.frameUs = gLc3Index[sourceFrequency][sourceDuration].duraUs;
        tlkapi_printf(APP_LOG_EN, "input:frame sample %d,frame octets %d,frame duration %d", codecI.fSample, codecI.fOctets, codecI.frameUs);
    }
}


    #if (APP_SCENE == APP_SCENE_TWS)
_attribute_ram_code_ void app_list_add_node(audio_pkt_t *pData) //audio data list add
{
    if (((unsigned int)(pData->renderPoint - clock_time())) > 200 * SYSTEM_TIMER_TICK_1MS) {
        APP_DBG_CHN_11_HIGH;
        APP_DBG_CHN_11_LOW;
        return;
    }
    struct __attribute__((packed)) list_node_t *pTemp = &pStart;
    APP_DBG_CHN_12_HIGH;
    u32 irq = irq_disable();
    if (pTemp->next == NULL) {
        u32 capture_tick_stimer = pData->renderPoint - clock_time();
        u32 capture_tick_timer  = (capture_tick_stimer * sys_clk.pclk + 8) / SYSTEM_TIMER_TICK_1US;
        ble_audio_timer_set_capture(TIMER0, 0, capture_tick_timer);
    }
    while (pTemp->next != NULL) {
        APP_DBG_CHN_10_HIGH;
        APP_DBG_CHN_10_LOW;
        pTemp = pTemp->next;
    }
    struct __attribute__((packed)) list_node_t *pNew = (struct list_node_t *)tlk_mem_malloc(sizeof(struct list_node_t));
    if (pNew == NULL) {
        APP_DBG_CHN_13_HIGH;
        APP_DBG_CHN_13_LOW;
        tlkapi_printf(APP_LOG_EN, "mempool malloc failed!");
        while (1) {
        #if (TLKAPI_DEBUG_ENABLE)
            tlkapi_debug_handler();
        #endif
        }
    }
    pNew->renderPoint = pData->renderPoint;
    pNew->pkt_seq_num = pData->pkt_seq_num;
    memcpy((u8 *)pNew->buffer, (u8 *)pData->buffer, codecO.fOctets);
    pTemp->next = pNew;
    pNew->next  = NULL;
    irq_restore(irq);
    APP_DBG_CHN_12_LOW;
}

_attribute_ram_code_ bool app_list_delete_node(struct list_node_t *pData) //audio data playback over, delete node.
{
    struct __attribute__((packed)) list_node_t *pTemp = &pStart;
    while (pTemp->next != NULL) {
        APP_DBG_CHN_9_HIGH;
        APP_DBG_CHN_9_LOW;
        if (pTemp->next == pData) {
            if (pTemp->next->next != NULL) {
                struct __attribute__((packed)) list_node_t *pDel = pTemp->next;
                pTemp->next                                      = pTemp->next->next;
                memset((u8 *)&pDel->renderPoint, 0, sizeof(struct list_node_t));
                TLK_MEM_STATE_T memRet = tlk_mem_free(pDel);
                if (memRet != TLK_MEM_SUCCESS) {
                    tlkapi_printf(APP_LOG_EN, "mempool free failed,pos1!");
                    break;
                }
                return true;
            } else {
                struct __attribute__((packed)) list_node_t *pDel = pTemp->next;
                pTemp->next                                      = NULL;
                memset((u8 *)&pDel->renderPoint, 0, sizeof(struct list_node_t));
                TLK_MEM_STATE_T memRet = tlk_mem_free(pDel);
                if (memRet != TLK_MEM_SUCCESS) {
                    tlkapi_printf(APP_LOG_EN, "mempool free failed,pos2!");
                    break;
                }
                return true;
            }
        }
        pTemp = pTemp->next;
    }
    return false;
}
    #else

_attribute_ram_code_ void app_list_add_node(audio_pkt_t *pData) //audio data list add
{
    if (((unsigned int)(pData->renderPoint - clock_time())) > 200 * SYSTEM_TIMER_TICK_1MS) {
        return;
    }
    struct __attribute__((packed)) list_node_t *pTemp = &pStart;
    u32                                         irq   = irq_disable();
    if (pTemp->next == NULL) {
        u32 capture_tick_stimer = pData->renderPoint - clock_time();
        u32 capture_tick_timer  = (capture_tick_stimer * sys_clk.pclk) / SYSTEM_TIMER_TICK_1US;
        ble_audio_timer_set_capture(TIMER0, 0, capture_tick_timer);
    }
    while (pTemp->next != NULL) {
        pTemp = pTemp->next;
    }
    struct __attribute__((packed)) list_node_t *pNew = (struct list_node_t *)tlk_mem_malloc(sizeof(struct list_node_t));
    if (pNew == NULL) {
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
}

_attribute_ram_code_ bool app_list_delete_node(struct list_node_t *pData) //audio data playback over, delete node.
{
    struct __attribute__((packed)) list_node_t *pTemp = &pStart;
    while (pTemp->next != NULL) {
        if (pTemp->next == pData) {
            if (pTemp->next->next != NULL) {
                struct __attribute__((packed)) list_node_t *pDel = pTemp->next;
                pTemp->next                                      = pTemp->next->next;
                memset((u8 *)&pDel->renderPoint, 0, sizeof(struct list_node_t));
                TLK_MEM_STATE_T memRet = tlk_mem_free(pDel);
                if (memRet != TLK_MEM_SUCCESS) {
                    tlkapi_printf(APP_LOG_EN, "mempool free failed,pos1!");
                    break;
                }
                return true;
            } else {
                struct __attribute__((packed)) list_node_t *pDel = pTemp->next;
                pTemp->next                                      = NULL;
                memset((u8 *)&pDel->renderPoint, 0, sizeof(struct list_node_t));
                TLK_MEM_STATE_T memRet = tlk_mem_free(pDel);
                if (memRet != TLK_MEM_SUCCESS) {
                    tlkapi_printf(APP_LOG_EN, "mempool free failed,pos2!");
                    break;
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
        app_list_delete_node(pStart.next);
    }
}

volatile unsigned short alg_spk_rptr = 0;

void app_codec_send_process(void)
{
    if (tlk_codec_getState(TLK_CODEC_INPUT) != TLK_CODEC_STATE_STREAMING) {
        return;
    }
    #if (APP_SCENE == APP_SCENE_TWS || APP_SCENE == APP_SCENE_HEADSET_EP1_MULTIPLEXING)
    u16 pcmData[APP_AUDIO_SUPPORT_MAX_FRAME_SAMPLE] = {0};
    if (tlk_codec_input_dataPop((u8 *)pcmData, codecI.fOctets) == TLK_CODEC_SUCCESS) {
        u8 audio_enc[APP_AUDIO_SUPPORT_MAX_ENCODE_FRAME_BYTES] = {0};
        for (u8 i = 0; i < APP_AUDIO_MAX_SOURCE_EP; i++) {
            if (appCtrl.source[i].sS) {
                log_task_begin_irq(ALG_TIMING_DEBUG_EN, SL01_app2);
                APP_DBG_CHN_1_HIGH;
                short        ref_buf[160];
                short       *ptr_out      = NULL;
                signed short spk_buf[160] = {0};

                ptr_out = ref_buf;

                signed short spk_dbg[160] = {0};
        #if ALG_GSC_EN                                                                                         //1.2ms,stereo
                memset(ref_buf, 0, sizeof(ref_buf));                                                           //codecI.fOctets should equal to 160*4
                tlk_gsc_BeamFormer((short *)(pcmData), (short *)ref_buf, ptr_out, g_gsc_st_p, 0);
                tlk_gsc_BeamFormer((short *)(pcmData + 160), (short *)ref_buf, (ptr_out + 80), g_gsc_st_p, 0); //16k sample rate
        #else                                                                                                  //mono
                memcpy(ptr_out, pcmData, codecI.fOctets); //160 sample
        #endif

        #if AUDIO_DEBUG_DATA_EN
                memcpy(ref_buf, ptr_out, 320); //160 sample
        #endif

                for (int i = 0; i < 160; i++) {
                    u16 rptr       = (alg_spk_rptr + i) % APP_AUDIO_OUTPUT_BUFFER_SIZE;
                    *(spk_buf + i) = *(app_audio_output_buffer + rptr);
                    *(spk_dbg + i) = *(spk_buf + i);
                }
                alg_spk_rptr = (alg_spk_rptr + 160) % APP_AUDIO_OUTPUT_BUFFER_SIZE;

        #if ALG_AEC_EN | ALG_S_NS //2~3ms
                tlka_aec_process_frame(g_aec_st, ptr_out, spk_buf, ptr_out);
                tlka_aec_process_frame(g_aec_st, ptr_out + 80, spk_buf + 80, ptr_out + 80);

                tlka_ns_process_frame(g_s_ns_den, ptr_out);
                tlka_ns_process_frame(g_s_ns_den, ptr_out + 80);
        #endif

                log_task_begin_irq(ALG_TIMING_DEBUG_EN, SL01_app3);
        #if ALG_W_NS //840us
                tlka_w_ns_process_frame(w_ns_st, ptr_out);
                tlka_w_ns_process_frame(w_ns_st, ptr_out + 80);
        #endif
                log_task_end_irq(ALG_TIMING_DEBUG_EN, SL01_app3);

        #if ALG_AGC_EN
                tlka_agc_process(g_agc_st, &ptr_out, 160, &ptr_out); //235us
        #endif


        #if (ALG_AUDIO_EN)
                LC3ENC_Error ret_lc3 = lc3enc_encode_pkt(i, (u8 *)ptr_out, audio_enc);
        #else
                LC3ENC_Error ret_lc3 = lc3enc_encode_pkt(i, (u8 *)pcmData, audio_enc);
        #endif
                if (ret_lc3 != LC3ENC_OK) {
                    tlkapi_printf(APP_LOG_EN, "lc3 encode error:0x%x", ret_lc3);
                }
                int ret = blc_bapus_sduPacketPush(appCtrl.aclHandle, appCtrl.source[i].epId, audio_enc, appCtrl.source[i].cP.frameOcts);
                if (ret != AUDIO_ESUCC) {
                    log_event_irq(ALG_TIMING_DEBUG_EN, SLET_app1);
                    tlkapi_printf(APP_LOG_EN, "cis send fail-ret: %d", ret);
                }
                APP_DBG_CHN_1_LOW;
                log_task_end_irq(ALG_TIMING_DEBUG_EN, SL01_app2);

        #if AUDIO_DEBUG_DATA_EN
                add_enc_stereo(ref_buf, spk_dbg, 160);
                add_enc_stereo(ref_buf + 80, spk_dbg + 80, 160);
                for (int j = 0; j < 2; j++) { //printf
                    tlkapi_send_string_data(APP_LOG_EN, "debug data print", add_buff + add_buff_rptr * 128, 128);
                    add_buff_rptr++;
                    add_buff_rptr = add_buff_rptr % ADD_BUFF_BLOCK_NUM;
                }
                        //                  tlkapi_send_string_data(APP_LOG_EN,"debug data print", spk_dbg,160);
                        //                  tlkapi_send_string_data(APP_LOG_EN,"debug data print", spk_dbg+80,160);
        #endif
            }
        }
    } else {
        return;
    }
    #endif
}

_attribute_ram_code_ void app_codec_receive_process(void)
{
    if (tlk_codec_getState(TLK_CODEC_OUTPUT) != TLK_CODEC_STATE_STREAMING) {
        return;
    }

    #if (APP_SCENE == APP_SCENE_TWS)
    for (u8 i = 0; i < APP_AUDIO_MAX_SINK_EP; i++) {
        if (appCtrl.sink[i].sS) {
            sdu_packet_t *pPkt = blc_bapus_sduPacketPop(appCtrl.aclHandle, appCtrl.sink[i].epId);
        #if (TLK_TONE_ENABLE)
            if (tlk_tone_is_playing()) {
                continue;
            }
        #endif
            if (pPkt != NULL) {
                log_task_begin_irq(ALG_TIMING_DEBUG_EN, SL01_app1);
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
                        log_event_irq(ALG_TIMING_DEBUG_EN, SLEV_app0);
                    }
                } else {
                    APP_DBG_CHN_7_HIGH;
                    APP_DBG_CHN_7_LOW;
                    appCtrl.sink[i].sT = clock_time() | 1;
                }
                LC3DEC_Error ret_lc3 = lc3dec_set_parameter(i, LC3_PARA_BEC_DETECT, &detect);
                if (ret_lc3 != LC3DEC_OK) {
                    tlkapi_printf(APP_LOG_EN, "lc3 decode set parameter error: %d", ret_lc3);
                    return;
                }
                APP_DBG_CHN_2_HIGH;
                audio_pkt_t pRaw = {0};
                ret_lc3          = lc3dec_decode_pkt(i, pPkt->data, pPkt->iso_sdu_len, (u8 *)pRaw.buffer);
                if (ret_lc3 != LC3DEC_OK) {
                    tlkapi_printf(APP_LOG_EN, "lc3 decode error-ret: %d", ret_lc3);
                    return;
                }
                pRaw.renderPoint = pPkt->timestamp + appCtrl.sink[i].pD * SYSTEM_TIMER_TICK_1US; //render point,tick count.
                pRaw.pkt_seq_num = (u32)(pPkt->pkt_seq_num);

                for (u8 i = 0; i < (pRaw.pkt_seq_num & 0x07); i++) {
                    APP_DBG_CHN_5_HIGH;
                    APP_DBG_CHN_5_LOW;
                }

                //              log_b16_irq(ALG_TIMING_DEBUG_EN,SL16_dbug0,pPkt->pkt_seq_num);
                app_list_add_node(&pRaw);
                APP_DBG_CHN_2_LOW;
                log_task_end_irq(ALG_TIMING_DEBUG_EN, SL01_app1);
            }
        }
    }
    #elif (APP_SCENE == APP_SCENE_HEADSET_EP1_MULTIPLEXING)
    for (u8 i = 0; i < APP_AUDIO_MAX_SINK_EP; i++) {
        if (appCtrl.sink[i].sS) {
            sdu_packet_t *pPkt = blc_bapus_sduPacketPop(appCtrl.aclHandle, appCtrl.sink[i].epId);
        #if (TLK_TONE_ENABLE)
            if (tlk_tone_is_playing()) {
                continue;
            }
        #endif
            if (pPkt != NULL) {
                u32 detect         = 0;
                u8  sinkChannelNum = app_get_one_in_data(appCtrl.sink[i].cP.location);
                if (pPkt->iso_sdu_len != sinkChannelNum * appCtrl.sink[i].cP.frameOcts) {
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

                audio_pkt_t pRaw                                          = {0};
                u16         audio_pcm[APP_AUDIO_SUPPORT_MAX_FRAME_SAMPLE] = {0};
                for (u8 j = 0; j < sinkChannelNum; j++) {
                    LC3DEC_Error ret_lc3 = lc3dec_set_parameter(j, LC3_PARA_BEC_DETECT, &detect);
                    if (ret_lc3 != LC3DEC_OK) {
                        tlkapi_printf(APP_LOG_EN, "lc3 decode set parameter error: %d", ret_lc3);
                        return;
                    }
                    APP_DBG_CHN_2_HIGH;

                    ret_lc3 = lc3dec_decode_pkt(j, pPkt->data + j * appCtrl.sink[i].cP.frameOcts, appCtrl.sink[i].cP.frameOcts, (u8 *)audio_pcm);
                    if (ret_lc3 != LC3DEC_OK) {
                        tlkapi_printf(APP_LOG_EN, "lc3 decode error-ret: %d", ret_lc3);
                        return;
                    }
                    for (u16 t = 0; t < codecO.fSample / sinkChannelNum; t++) {
                        pRaw.buffer[sinkChannelNum * t + j] = audio_pcm[t];
                    }
                    APP_DBG_CHN_2_LOW;
                }
                pRaw.renderPoint = pPkt->timestamp + appCtrl.sink[i].pD * SYSTEM_TIMER_TICK_1US; //render point,tick count.
                app_list_add_node(&pRaw);
            }
        }
    }
    #endif
}

extern s8 cisConnectFlag;

_attribute_ram_code_ void app_timer_irq_proc(void)
{
    if (!cisConnectFlag) {
        timer_stop(TIMER0);

        u32 readOffset  = 0;
        u32 writeOffset = 0;
        u32 offset      = 0;
        if (tlk_codec_output_getOffset(&writeOffset, &readOffset) != TLK_CODEC_SUCCESS) {
            recvRptr = recvWptr;
            return;
        }

        if (writeOffset >= readOffset) {
            offset = writeOffset - readOffset;
        } else {
            offset = sizeof(codecSpeakBuff) + writeOffset - readOffset;
        }

        if (offset < 8 || offset > 24) {
            tlkapi_printf(APP_LOG_EN, "set offset, %d %d %d", offset, writeOffset, readOffset);

            if (tlk_codec_output_setWriteOffset(readOffset + 16) != TLK_CODEC_SUCCESS) {
                recvRptr = recvWptr;
                tlkapi_printf(APP_LOG_EN, "set offset error");
                return;
            }
        }

        appSinkRecvSpeak_t *recvAudioTemp = &recvAudioBuff[recvRptr];

        if (tlk_codec_output_dataPush((u8 *)recvAudioTemp->rxBuff, appSinkInfo.frameDataLen * 2) != TLK_CODEC_SUCCESS) {
            recvRptr = recvWptr;
            tlkapi_printf(APP_LOG_EN, "data push error");
            return;
        }

        blc_audio_clock_calib(recvAudioTemp->renderPoint, 10000 * SYSTEM_TIMER_TICK_1US);

        recvRptr++;
        if (recvRptr == APP_SINK_RECV_SPEAK_FRAME_COUNT) {
            recvRptr = 0;
        }

        if (recvRptr != recvWptr) {
            recvAudioTemp           = &recvAudioBuff[recvRptr];
            u32 capture_tick_stimer = recvAudioTemp->renderPoint - clock_time();
            u32 capture_tick_timer  = (capture_tick_stimer * sys_clk.pclk) / SYSTEM_TIMER_TICK_1US;
            ble_audio_timer_set_capture(TIMER0, 0, capture_tick_timer);
        }
    } else {
    #if (MCU_CORE_TYPE == MCU_CORE_B91)
        if (appCtrl.spk_reset) {
            while (!clock_time_exceed(pStart.next->renderPoint, 6))
                ;
            reg_audio_en |= FLD_AUDIO_I2S_CLK_EN; //enable i2s en
            reg_audio_codec_dac_itf_ctr = MASK_VAL(FLD_AUDIO_CODEC_FORMAT, CODEC_I2S_MODE, FLD_AUDIO_CODEC_DAC_ITF_SB, CODEC_ITF_AC, FLD_AUDIO_CODEC_SLAVE, I2S_M_CODEC_S, FLD_AUDIO_CODEC_WL, CODEC_BIT_16_DATA);
            appCtrl.spk_reset           = 0;
        }
    #endif
        log_task_begin_irq(ALG_TIMING_DEBUG_EN, SL01_app0);
        timer_stop(TIMER0);
        APP_DBG_CHN_3_HIGH;
        u32 readOffset  = 0;
        u32 writeOffset = 0;
        u32 offset      = 0;
    #if (TLK_TONE_ENABLE)
        if (tlk_tone_is_playing()) {
            app_audio_tone_handle_task();
            APP_DBG_CHN_3_LOW;
            return;
        }
    #endif
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

        if (offset < 8 || offset > 24) {
            tlkapi_printf(1, "get offset error:0x%x,0x%x,0x%x", writeOffset, readOffset, offset);
            //        APP_DBG_CHN_4_HIGH;

            if (tlk_codec_output_setWriteOffset(readOffset + 16) != TLK_CODEC_SUCCESS) {
                app_list_free();
                //            tlkapi_printf(APP_LOG_EN,"set offset error");
                return;
            }
            //        alg_spk_rptr = ((readOffset + 16)/2 + APP_AUDIO_OUTPUT_BUFFER_SIZE - 0)%APP_AUDIO_OUTPUT_BUFFER_SIZE;//ALG_AEC_DELAY
            //        APP_DBG_CHN_4_LOW;
        }
        APP_DBG_CHN_4_HIGH;
        if (tlk_codec_output_dataPush((u8 *)pStart.next->buffer, codecO.fOctets) != TLK_CODEC_SUCCESS) {
            app_list_free();
            //        tlkapi_printf(APP_LOG_EN,"data push error");
            return;
        }
        APP_DBG_CHN_4_LOW;
    #if AUDIO_CLOCK_CALIB2_ALGORITHM_EN
        blc_audio_clock_calib2(pStart.next->renderPoint, pStart.next->pkt_seq_num, offset);
    #else
        blc_audio_clock_calib(pStart.next->renderPoint, codecO.frameUs * SYSTEM_TIMER_TICK_1US);
    #endif
        if (app_list_delete_node(pStart.next) == 0) {
            APP_DBG_CHN_8_HIGH;
            //        tlkapi_printf(APP_LOG_EN,"memory free failed-start");
            APP_DBG_CHN_8_LOW;
        }

        if (pStart.next != NULL) {
            u32 capture_tick_stimer = pStart.next->renderPoint - clock_time();
            u32 capture_tick_timer  = (capture_tick_stimer * sys_clk.pclk) / SYSTEM_TIMER_TICK_1US;
            ble_audio_timer_set_capture(TIMER0, 0, capture_tick_timer);
        }
        APP_DBG_CHN_3_LOW;
        log_task_end_irq(ALG_TIMING_DEBUG_EN, SL01_app0);
    }
}

_attribute_ram_code_ void app_codec_handler(void)
{
    app_codec_receive_process();
    app_codec_send_process();
    if (appCtrl.configCodecIdx) {
        app_config_codec();
        appCtrl.configCodecIdx = false;
    }

    app_audio_receiveHandler();
}
#endif
