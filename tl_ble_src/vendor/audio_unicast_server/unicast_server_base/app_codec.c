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
#include "ext_driver/ext_audio.h"
#include "stack/ble/ble.h"

#include "app_config.h"
#include "app_audio.h"
#include "app_codec.h"

#include "vendor/common/tlk_api/tlk_tone.h"
#include "vendor/common/tlk_api/tlk_codec.h"
#include "vendor/common/tlk_api/tlk_mem.h"
#include "vendor/common/tlk_api/tlk_eq.h"
#include "algorithm/audio_alg/lc3/lc3.h"
#include "algorithm/audio_alg/hybrid/gcc7/tlka_hybrid_alg_api.h"
#include "../../../algorithm/audio_alg/alg_audio_cfg.h"
#include "../../../algorithm/audio_alg/audio_alg_debug.h"
#if (UNICAST_SERVER_SELECT == UNICAST_SERVER_BASE)
struct list_node_t pStart;
unsigned short app_audio_output_buffer[APP_AUDIO_OUTPUT_BUFFER_SIZE];
unsigned short app_audio_input_buffer[APP_AUDIO_INPUT_BUFFER_SIZE];
app_codec_desc_t    codecI;
app_codec_desc_t    codecO;
extern app_audio_ctrl_t appCtrl;

#define ALG_TIMING_DEBUG_EN         0
#if ALG_AUDIO_EN
    #if AUDIO_DEBUG_DATA_EN
        #define ALG_BUFF_SIZE       1024*34 //0x6be8 + debug
    #else
        #define ALG_BUFF_SIZE       1024*28 //0x6be8
    #endif
unsigned char alg_buff[ALG_BUFF_SIZE] = {0};
#endif
#if ALG_HYBRID_ALG_EN
    void *g_hybrid_scratch_buff     = NULL;
    void *g_st_hybrid   = NULL;
#endif

/**
 * @brief      Codec init function.
 * @param[in]  none.
 * @return     none.
 */
void app_codec_init(void)
{
    tlk_codec_init();
    ble_audio_timer_init(TIMER0);
    pStart.next = NULL;
    #if ALG_HYBRID_ALG_EN
        int hybrid_version = tlka_hybrid_alg_get_version();
        BLT_APP_LOG("HYBRID init version inf:0x%x",hybrid_version);

        extern u8 lc3Scratch[];
        // audio algrithm init
        int ns = 0;
        void* buff_tp = alg_buff;
        GSC_Param gsc_para = {
            .frame_size = 80,
            .sampleRate = 16000,
            .exchange_mic = 0
        };

        AEC_Param aec_para = {
            .frame_size = 80,
            .sampleRate = 16000,
            .en_aec_post = 1
        };

        W_NS_CFG_PARAM ns_para = {
            .frame_size = 80,
            .sampleRate = 16000,
            .target_level = k21dB
        };

        HYBRID_ALG_Param hybrid_para;
        hybrid_para.frame_size = 80;
        hybrid_para.gsc_para = &gsc_para;
        hybrid_para.aec_para = &aec_para;
        hybrid_para.ns_para = &ns_para;

        int hybrid_scratch_ram = tlka_hybrid_alg_get_scratch_size();
        // multiplex lc3Scratch
        g_hybrid_scratch_buff = lc3Scratch;

        int hybridSize = tlka_hybrid_alg_get_size();
        g_st_hybrid = buff_tp + ns;
        ns += hybridSize;
        BLT_APP_LOG ("hybrid init end scratch:0x%x,0x%x", hybrid_scratch_ram, hybridSize);
        tlka_hybrid_alg_init((void *)g_st_hybrid, &hybrid_para, g_hybrid_scratch_buff, hybridSize);
    #endif

    #if AUDIO_DEBUG_DATA_EN
        void* p_buf = buff_tp + ns;
        int sbc_size = add_init(p_buf);
        ns += sbc_size;
        BLT_APP_LOG("SBC init end:0x%x",sbc_size);
    #endif

#if (APP_SCENE == APP_SCENE_TWS)
    tlk_mem_pool_desc_t poolDesc[] =
    {
        { 980,  8 },
    };
#elif(APP_SCENE == APP_SCENE_HEADSET_EP1_MULTIPLEXING)
    tlk_mem_pool_desc_t poolDesc[] =
    {
        { 1960,  6 },
    };
#endif

    u8 numPools = sizeof(poolDesc) / sizeof(poolDesc[0]);
    u32 ret = tlk_mempool_init(numPools, poolDesc);
    if(ret!=TLK_MEM_SUCCESS)
    {
        tlkapi_printf(APP_LOG_EN,"error-mempool init failed! %d",ret);
    }
    else
    {
        tlkapi_printf(APP_LOG_EN,"mempool init success!");
    }
}

u16 app_get_one_in_data(u32 data)
{
    uint16_t n = 0;
    while (data > 0)
    {
        if (data & 0x01)
            n++;
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
    u8 sinkCnt=0;
    u8 sinkFrequency = 0;
    u8 sinkDuration  = 0;
    u8 sinkChannelNum = 0;

    u8 sourceCnt=0;
    u8 sourceFrequency = 0;
    u8 sourceDuration  = 0;
    u8 sourceChannelNum = 0;

    for(u8 i=0;i<APP_AUDIO_MAX_SINK_EP;i++)//ep sink-codec output
    {
        if(appCtrl.sink[i].cP.paramReady)
        {
            sinkCnt++;
            if((sinkCnt>1 && app_get_one_in_data(appCtrl.sink[i].cP.location)>1)||app_get_one_in_data(appCtrl.sink[i].cP.location)>2)
            {

                blc_bapus_aseRelease(appCtrl.aclHandle,appCtrl.sink[i].epId);//not support more than 2 channel
            }
            if(sinkCnt>1)
            {
                if(sinkFrequency != appCtrl.sink[i].cP.frequency || sinkDuration != appCtrl.sink[i].cP.duration\
                || sinkChannelNum != app_get_one_in_data(appCtrl.sink[i].cP.location))
                {
                    blc_bapus_aseRelease(appCtrl.aclHandle,appCtrl.sink[i].epId);//not support asymmetric configure.
                }
            }
            sinkFrequency = appCtrl.sink[i].cP.frequency;
            sinkDuration  = appCtrl.sink[i].cP.duration;
            sinkChannelNum = app_get_one_in_data(appCtrl.sink[i].cP.location);

            if(sinkChannelNum>1)
            {
                for(u8 t=0;t<sinkChannelNum;t++)
                {
                    int lc3Ret = lc3dec_decode_init_bap(t,appCtrl.sink[i].cP.frequency,\
                                                           appCtrl.sink[i].cP.duration,\
                                                           appCtrl.sink[i].cP.frameOcts);
                    tlkapi_printf(APP_LOG_EN,"lc3 dec channel[%d]:frequency[%d],duration[%d],frameOcts[%d]",t,appCtrl.sink[i].cP.frequency,appCtrl.sink[i].cP.duration,appCtrl.sink[i].cP.frameOcts);
                    if(lc3Ret != LC3DEC_OK)
                    {
                        tlkapi_printf(APP_LOG_EN,"error-lc3 encode fail ep_ID,ret:%x", lc3Ret);
                        blc_bapus_aseRelease(appCtrl.aclHandle,appCtrl.sink[i].epId);//not support asymmetric configure.
                        return;
                    }
                }
            }
            else
            {
                int lc3Ret = lc3dec_decode_init_bap(i,appCtrl.sink[i].cP.frequency,\
                                                       appCtrl.sink[i].cP.duration,\
                                                       appCtrl.sink[i].cP.frameOcts);
                tlkapi_printf(APP_LOG_EN,"lc3 dec channel[%d]:frequency[%d],duration[%d],frameOcts[%d]",i,appCtrl.sink[i].cP.frequency,appCtrl.sink[i].cP.duration,appCtrl.sink[i].cP.frameOcts);
                if(lc3Ret != LC3DEC_OK)
                {
                    tlkapi_printf(APP_LOG_EN,"error-lc3 encode fail ep_ID,ret:%d,%d", lc3Ret,appCtrl.sink[i].epId);
                    blc_bapus_aseRelease(appCtrl.aclHandle,appCtrl.sink[i].epId);//not support asymmetric configure.
                    return;
                }
            }
        }
    }
    if(sinkCnt)
    {
        if(sinkChannelNum == 2 || sinkCnt == 2)
        {
            codecO.cC = TLK_CODEC_2_CHANNEL;
        }
        else
        {
            codecO.cC = TLK_CODEC_1_CHANNEL;
        }
        tlk_codec_sts_e codecRet = tlk_codec_config(TLK_CODEC_OUTPUT,sinkFrequency,codecO.cC,TLK_CODEC_LINE,(u8*)app_audio_output_buffer,2*APP_AUDIO_OUTPUT_BUFFER_SIZE);
#if AUDIO_CLOCK_CALIB2_ALGORITHM_EN
        blc_audio_clock_calib_init();
#endif
        if(codecRet != TLK_CODEC_SUCCESS)
        {
            tlkapi_printf(APP_LOG_EN,"error-codec config fail ret:%d", codecRet);
            for(u8 i=0;i<APP_AUDIO_MAX_SINK_EP;i++)
            {
                blc_bapus_aseRelease(appCtrl.aclHandle,appCtrl.sink[i].epId);
            }
        }
        tlkapi_printf(APP_LOG_EN,"codec output:frequency[%d],channel[%d]",sinkFrequency,codecO.cC);
        codecO.fSample =codecO.cC*gLc3Index[sinkFrequency][sinkDuration].frameSample;
        codecO.fOctets =codecO.cC*gLc3Index[sinkFrequency][sinkDuration].frameOctets;
        codecO.frameUs =gLc3Index[sinkFrequency][sinkDuration].duraUs;
        tlkapi_printf(APP_LOG_EN,"output:frame sample %d,frame octets %d,frame duration %d",codecO.fSample,codecO.fOctets,codecO.frameUs);
    }
    for(u8 i=0;i<APP_AUDIO_MAX_SOURCE_EP;i++)//ep source-codec input
    {
        if(appCtrl.source[i].cP.paramReady)
        {
            sourceCnt++;
            if((sourceCnt>1 && app_get_one_in_data(appCtrl.source[i].cP.location)>1)||app_get_one_in_data(appCtrl.source[i].cP.location)>2)
            {
                blc_bapus_aseRelease(appCtrl.aclHandle,appCtrl.source[i].epId);//not support more than 2 channel
            }
            if(sourceCnt>1)
            {
                if(sinkFrequency != appCtrl.source[i].cP.frequency || sinkDuration != appCtrl.source[i].cP.duration\
                || sourceChannelNum !=app_get_one_in_data(appCtrl.source[i].cP.location))
                {
                    blc_bapus_aseRelease(appCtrl.aclHandle,appCtrl.source[i].epId);//not support asymmetric configure.
                }
            }
            sourceFrequency = appCtrl.source[i].cP.frequency;
            sourceDuration  = appCtrl.source[i].cP.duration;
            sourceChannelNum = app_get_one_in_data(appCtrl.source[i].cP.location);
            if(sourceChannelNum>1)
            {
                for( u8 t=0;t<sourceChannelNum;t++)
                {
                    int lc3Ret = lc3enc_encode_init_bap(t,appCtrl.source[i].cP.frequency,\
                                                           appCtrl.source[i].cP.duration,\
                                                           appCtrl.source[i].cP.frameOcts);
                    tlkapi_printf(APP_LOG_EN,"lc3 enc channel[%d]:frequency[%d],duration[%d],frameOcts[%d]",t,appCtrl.source[i].cP.frequency,appCtrl.source[i].cP.duration,appCtrl.source[i].cP.frameOcts);
                    if(lc3Ret != LC3DEC_OK)
                    {
                        tlkapi_printf(APP_LOG_EN,"error-lc3 encode fail ep_ID,ret:%d,%d", lc3Ret,appCtrl.sink[i].epId);
                        blc_bapus_aseRelease(appCtrl.aclHandle,appCtrl.source[i].epId);//not support asymmetric configure.
                        return;
                    }
                }
            }
            else
            {
                int lc3Ret = lc3enc_encode_init_bap(i,appCtrl.source[i].cP.frequency,\
                                                       appCtrl.source[i].cP.duration,\
                                                       appCtrl.source[i].cP.frameOcts);
                tlkapi_printf(APP_LOG_EN,"lc3 enc channel[%d]:frequency[%d],duration[%d],frameOcts[%d]",i,appCtrl.source[i].cP.frequency,appCtrl.source[i].cP.duration,appCtrl.source[i].cP.frameOcts);
                if(lc3Ret != LC3DEC_OK)
                {
                    tlkapi_printf(APP_LOG_EN,"error-lc3 encode fail ep_ID,ret:%d,%d", lc3Ret,appCtrl.sink[i].epId);
                    blc_bapus_aseRelease(appCtrl.aclHandle,appCtrl.source[i].epId);//not support asymmetric configure.
                    return;
                }
            }
        }
    }
    if(sourceCnt)
    {
        if(sourceChannelNum == 2 || sourceCnt == 2)
        {
            codecI.cC = TLK_CODEC_2_CHANNEL;
        }
        else
        {
            codecI.cC = TLK_CODEC_1_CHANNEL;
        }
        #if ALG_HYBRID_ALG_EN
            codecI.cC = TLK_CODEC_2_CHANNEL;
        #endif
        tlk_codec_sts_e codecRet = tlk_codec_config(TLK_CODEC_INPUT,sourceFrequency,codecI.cC,TLK_CODEC_MIC,(u8*)app_audio_input_buffer,2*APP_AUDIO_INPUT_BUFFER_SIZE);
        if(codecRet != TLK_CODEC_SUCCESS)
        {
            tlkapi_printf(APP_LOG_EN,"error-codec config fail ret:%d", codecRet);
            for(u8 i=0;i<APP_AUDIO_MAX_SOURCE_EP;i++)
            {
                blc_bapus_aseRelease(appCtrl.aclHandle,appCtrl.source[i].epId);
            }
        }
        tlkapi_printf(APP_LOG_EN,"codec input:frequency[%d],channel[%d]",sourceFrequency,codecI.cC);
        codecI.fSample =codecI.cC*gLc3Index[sourceFrequency][sourceDuration].frameSample;
        codecI.fOctets =codecI.cC*gLc3Index[sourceFrequency][sourceDuration].frameOctets;
        codecI.frameUs =gLc3Index[sourceFrequency][sourceDuration].duraUs;
        tlkapi_printf(APP_LOG_EN,"input:frame sample %d,frame octets %d,frame duration %d",codecI.fSample,codecI.fOctets,codecI.frameUs);
    }
}



#if (APP_SCENE == APP_SCENE_TWS)
_attribute_ram_code_ void app_list_add_node(audio_pkt_t *pData)//audio data list add
{

    if(((unsigned int)(pData->renderPoint - clock_time()))> 200*SYSTEM_TIMER_TICK_1MS)
    {
        APP_DBG_CHN_11_HIGH;
        APP_DBG_CHN_11_LOW;
        return;
    }
    struct list_node_t *pTemp = &pStart;
    APP_DBG_CHN_12_HIGH;
    u32 irq = irq_disable();
    if(pTemp->next==NULL)
    {
        u32 capture_tick_stimer = pData->renderPoint-clock_time();
        u32 capture_tick_timer  = (capture_tick_stimer*sys_clk.pclk+8)/SYSTEM_TIMER_TICK_1US;
        ble_audio_timer_set_capture(TIMER0,0,capture_tick_timer);
        if(appCtrl.spk_reset){
            ble_audio_reset();
        }
    }
    while(pTemp->next!=NULL)
    {
        APP_DBG_CHN_10_HIGH;
        APP_DBG_CHN_10_LOW;
        pTemp = pTemp->next;
    }
    struct list_node_t *pNew = (struct list_node_t*)tlk_mem_malloc(sizeof(struct list_node_t));
    if(pNew == NULL)
    {
        APP_DBG_CHN_13_HIGH;
        APP_DBG_CHN_13_LOW;
        tlkapi_printf(APP_LOG_EN,"mempool malloc failed!");
        while(1)
        {
            #if (TLKAPI_DEBUG_ENABLE)
                tlkapi_debug_handler();
            #endif
        }
    }
    pNew->renderPoint = pData->renderPoint;
    pNew->pkt_seq_num = pData->pkt_seq_num;
    memcpy((u8*)pNew->buffer,(u8*)pData->buffer,codecO.fOctets);
    pTemp->next = pNew;
    pNew->next = NULL;
    irq_restore(irq);
    APP_DBG_CHN_12_LOW;
}
_attribute_ram_code_ bool app_list_delete_node(struct list_node_t *pData)//audio data playback over, delete node.
{
    struct list_node_t *pTemp = &pStart;
    while(pTemp->next!=NULL)
    {
//      APP_DBG_CHN_9_HIGH;
//      APP_DBG_CHN_9_LOW;
        if(pTemp->next == pData)
        {
            if(pTemp->next->next != NULL)
            {
                struct list_node_t *pDel = pTemp->next;
                pTemp->next = pTemp->next->next;
                memset((u8*)&pDel->renderPoint,0,sizeof(struct list_node_t));
                TLK_MEM_STATE_T memRet = tlk_mem_free(pDel);
                if(memRet!=TLK_MEM_SUCCESS)
                {
                    tlkapi_printf(APP_LOG_EN,"mempool free failed,pos1!");
                    break;
                }
                return true;
            }
            else
            {
                struct list_node_t *pDel = pTemp->next;
                pTemp->next=NULL;
                memset((u8*)&pDel->renderPoint,0,sizeof(struct list_node_t));
                TLK_MEM_STATE_T memRet = tlk_mem_free(pDel);
                if(memRet!=TLK_MEM_SUCCESS)
                {
                    tlkapi_printf(APP_LOG_EN,"mempool free failed,pos2!");
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

_attribute_ram_code_ void app_list_add_node(audio_pkt_t *pData)//audio data list add
{

    if(((unsigned int)(pData->renderPoint - clock_time()))> 200*SYSTEM_TIMER_TICK_1MS)
    {
        return;
    }
    struct list_node_t *pTemp = &pStart;
    u32 irq = irq_disable();
    if(pTemp->next==NULL)
    {
        u32 capture_tick_stimer = pData->renderPoint-clock_time();
        u32 capture_tick_timer  = (capture_tick_stimer*sys_clk.pclk)/SYSTEM_TIMER_TICK_1US ;
        ble_audio_timer_set_capture(TIMER0,0,capture_tick_timer);
    }
    while(pTemp->next!=NULL)
    {
        pTemp = pTemp->next;
    }
    struct list_node_t *pNew = (struct list_node_t*)tlk_mem_malloc(sizeof(struct list_node_t));
    if(pNew == NULL)
    {
        tlkapi_printf(APP_LOG_EN,"mempool malloc failed!");
        while(1)
        {
            #if (TLKAPI_DEBUG_ENABLE)
                tlkapi_debug_handler();
            #endif
        }
    }
    pNew->renderPoint = pData->renderPoint;
    memcpy((u8*)pNew->buffer,(u8*)pData->buffer,codecO.fOctets);
    pTemp->next = pNew;
    pNew->next = NULL;
    irq_restore(irq);
}

_attribute_ram_code_ bool app_list_delete_node(struct list_node_t *pData)//audio data playback over, delete node.
{
    struct list_node_t *pTemp = &pStart;
    while(pTemp->next!=NULL)
    {
        if(pTemp->next == pData)
        {
            if(pTemp->next->next != NULL)
            {
                struct list_node_t *pDel = pTemp->next;
                pTemp->next = pTemp->next->next;
                memset((u8*)&pDel->renderPoint,0,sizeof(struct list_node_t));
                TLK_MEM_STATE_T memRet = tlk_mem_free(pDel);
                if(memRet!=TLK_MEM_SUCCESS)
                {
                    tlkapi_printf(APP_LOG_EN,"mempool free failed,pos1!");
                    break;
                }
                return true;
            }
            else
            {
                struct list_node_t *pDel = pTemp->next;
                pTemp->next=NULL;
                memset((u8*)&pDel->renderPoint,0,sizeof(struct list_node_t));
                TLK_MEM_STATE_T memRet = tlk_mem_free(pDel);
                if(memRet!=TLK_MEM_SUCCESS)
                {
                    tlkapi_printf(APP_LOG_EN,"mempool free failed,pos2!");
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
    while(pStart.next!=NULL)
    {
        app_list_delete_node(pStart.next);
    }
}
volatile unsigned short alg_spk_rptr = 0;
void app_codec_send_process(void)
{
    if(tlk_codec_getState(TLK_CODEC_INPUT)!=TLK_CODEC_STATE_STREAMING)
    {
        return;
    }

#if (APP_SCENE == APP_SCENE_TWS || APP_SCENE == APP_SCENE_HEADSET_EP1_MULTIPLEXING)
    u16 pcmData[APP_AUDIO_SUPPORT_MAX_FRAME_SAMPLE]={0};
    if(appCtrl.mic_reset){
        u32 mic_wptr = codec_get_InputWriteOffset();
        u32 rptr = (2*APP_AUDIO_INPUT_BUFFER_SIZE + mic_wptr - codecI.fOctets )%(APP_AUDIO_INPUT_BUFFER_SIZE*2);
        codec_set_InputReadOffset(rptr);

        u32 spk_rptr = codec_output_getReadOffset();
        alg_spk_rptr = ((spk_rptr/2) + APP_AUDIO_OUTPUT_BUFFER_SIZE - 308)%APP_AUDIO_OUTPUT_BUFFER_SIZE;//ALG_AEC_DELAY

        appCtrl.mic_reset = 0;
    }
    if(tlk_codec_input_dataPop((u8*)pcmData,codecI.fOctets) == TLK_CODEC_SUCCESS)
    {
        u8 audio_enc[APP_AUDIO_SUPPORT_MAX_ENCODE_FRAME_BYTES]={0};
        for(u8 i=0;i<APP_AUDIO_MAX_SOURCE_EP;i++)
        {
            if(appCtrl.source[i].sS)
            {
                log_task_begin_irq(ALG_TIMING_DEBUG_EN,SL01_app2);
                APP_DBG_CHN_1_HIGH;
                short ref_buf[160];
                short *ptr_out = NULL;
                signed short spk_buf[160] ={0};

                ptr_out   = ref_buf;

                signed short spk_dbg[160] ={0};
            #if ALG_HYBRID_ALG_EN//1.2ms,stereo
                for(int i=0;i<160;i++)
                {
                    u16 rptr = (alg_spk_rptr + i)% APP_AUDIO_OUTPUT_BUFFER_SIZE;
                    *(spk_buf+i) = *(app_audio_output_buffer + rptr);
                    *(spk_dbg+i) = *(spk_buf+i);
                }
                alg_spk_rptr = (alg_spk_rptr+160) % APP_AUDIO_OUTPUT_BUFFER_SIZE;

                //2~3ms
                short mic_in_buf[160];
                for(int i=0; i<80; i++) {
                    mic_in_buf[i] = pcmData[2*i];
                    mic_in_buf[i + 80] = pcmData[2*i + 1];
                }
                tlka_hybrid_alg_process_frame(g_st_hybrid, 7, mic_in_buf, spk_buf, ptr_out);

                for(int i=0; i<80; i++) {
                    mic_in_buf[i] = pcmData[2*i + 160];
                    mic_in_buf[i + 80] = pcmData[2*i + 1 + 160];
                }
                tlka_hybrid_alg_process_frame(g_st_hybrid, 7, mic_in_buf, spk_buf+80, ptr_out+80);
            #else//mono
                memcpy(ptr_out,pcmData,codecI.fOctets);//160 sample
            #endif

            #if AUDIO_DEBUG_DATA_EN
                memcpy(ref_buf,ptr_out,320);//160 sample
            #endif

            #if EQ_AUDIO_EN
                g_eq_para.eq_type        = EQ_TYPE_VOICE_MIC;
                g_eq_para.eq_sample_rate = EQ_SAMPLE_RATE_16K;
                g_eq_para.eq_channel = EQ_CHANNEL_LEFT;
                g_eq_para.eq_nstage = instance_eq_voice_mic_left.nstage;
                eq_proc(g_eq_para, (signed short *)ptr_out, (signed short *)ptr_out, codecI.fSample);
            #endif

                LC3ENC_Error ret_lc3 = lc3enc_encode_pkt(i,(u8*)ptr_out,audio_enc);
                if(ret_lc3!=LC3ENC_OK)
                {
                    tlkapi_printf(APP_LOG_EN,"lc3 encode error:0x%x", ret_lc3);
                }
                int ret = blc_bapus_sduPacketPush(appCtrl.aclHandle,appCtrl.source[i].epId,audio_enc, appCtrl.source[i].cP.frameOcts);
                if(ret != AUDIO_ESUCC)
                {
                    log_event_irq(ALG_TIMING_DEBUG_EN,SLET_app1);
                    tlkapi_printf(APP_LOG_EN,"cis send fail-ret: %d", ret);
                }
                APP_DBG_CHN_1_LOW;
                log_task_end_irq(ALG_TIMING_DEBUG_EN,SL01_app2);

                #if AUDIO_DEBUG_DATA_EN
                    add_enc_stereo(ref_buf,spk_dbg,160);//5ms
                    add_enc_stereo(ref_buf+80,spk_dbg+80,160);//5ms
                    for(int j = 0;j<2;j++){//printf
                        tlkapi_send_string_data(APP_LOG_EN,"debug data print", add_buff+add_buff_rptr*128,128);
                        add_buff_rptr++;
                        add_buff_rptr = add_buff_rptr % ADD_BUFF_BLOCK_NUM;
                    }
//                  tlkapi_send_string_data(APP_LOG_EN,"debug data print", spk_dbg,160);
//                  tlkapi_send_string_data(APP_LOG_EN,"debug data print", spk_dbg+80,160);
                #endif

            }
        }
    }
    else
    {
        return;
    }
#endif
}
_attribute_ram_code_
void app_codec_receive_process(void)
{
    if(tlk_codec_getState(TLK_CODEC_OUTPUT)!=TLK_CODEC_STATE_STREAMING)
    {
        return;
    }
#if (APP_SCENE == APP_SCENE_TWS)
    for(u8 i=0;i<APP_AUDIO_MAX_SINK_EP;i++)
    {
        if(appCtrl.sink[i].sS)
        {
            sdu_packet_t* pPkt = blc_bapus_sduPacketPop(appCtrl.aclHandle, appCtrl.sink[i].epId);
#if (TLK_TONE_ENABLE)
            if (tlk_tone_is_playing()) {
                continue;
            }
#endif
            if(pPkt != NULL)
            {
                log_task_begin_irq(ALG_TIMING_DEBUG_EN,SL01_app1);
                u32 detect = 0;
                if(pPkt->iso_sdu_len!=appCtrl.sink[i].cP.frameOcts)
                {
                    if(!appCtrl.sink[i].sT)
                    {
                        APP_DBG_CHN_5_HIGH;
                        APP_DBG_CHN_5_LOW;
                        return;
                    }
                    else
                    {
                        APP_DBG_CHN_6_HIGH;
                        APP_DBG_CHN_6_LOW;
                        detect = 1;
                        log_event_irq(ALG_TIMING_DEBUG_EN,SLEV_app0);
                    }
                }
                else
                {
                    APP_DBG_CHN_7_HIGH;
                    APP_DBG_CHN_7_LOW;
                    appCtrl.sink[i].sT = clock_time()|1;
                }
                LC3DEC_Error ret_lc3 = lc3dec_set_parameter(i, LC3_PARA_BEC_DETECT, &detect);
                if(ret_lc3!=LC3DEC_OK)
                {
                    tlkapi_printf(APP_LOG_EN,"lc3 decode set parameter error: %d", ret_lc3);
                    return;
                }
                APP_DBG_CHN_2_HIGH;
                audio_pkt_t pRaw = {0};
//              u8 wptr = pPkt->data[0] & 0x0f;
//              for(u8 i=0;i<wptr;i++)
//              {
//                  delay_us(1);
//                  APP_DBG_CHN_9_HIGH;
//                  APP_DBG_CHN_9_LOW;
//                  delay_us(1);
//              }
//              return;

                ret_lc3 = lc3dec_decode_pkt(i,pPkt->data,pPkt->iso_sdu_len,(u8*)pRaw.buffer);
                if(ret_lc3!=LC3DEC_OK)
                {
                    tlkapi_printf(APP_LOG_EN,"lc3 decode error-ret: %d", ret_lc3);
                    return;
                }
                pRaw.renderPoint = pPkt->timestamp + appCtrl.sink[i].pD*SYSTEM_TIMER_TICK_1US;//render point,tick count.
                pRaw.pkt_seq_num = (u32)(pPkt->pkt_seq_num);

                for(u8 i=0;i<(pRaw.pkt_seq_num & 0x07);i++)
                {
                    APP_DBG_CHN_5_HIGH;
                    APP_DBG_CHN_5_LOW;
                }

                APP_DBG_CHN_9_HIGH;

                app_audio_control_volume((signed short *)pRaw.buffer, codecO.fSample);
#if EQ_AUDIO_EN
                g_eq_para.eq_type        = EQ_TYPE_MUSIC;
                g_eq_para.eq_sample_rate = EQ_SAMPLE_RATE_48K;
                g_eq_para.eq_channel = EQ_CHANNEL_LEFT;
                g_eq_para.eq_nstage = instance_eq_music_left.nstage;
                eq_proc(g_eq_para, (signed short *)pRaw.buffer, (signed short *)pRaw.buffer, codecO.fSample);
#endif
                APP_DBG_CHN_9_LOW;
//              log_b16_irq(ALG_TIMING_DEBUG_EN,SL16_dbug0,pPkt->pkt_seq_num);
                app_list_add_node(&pRaw);
                APP_DBG_CHN_2_LOW;
                log_task_end_irq(ALG_TIMING_DEBUG_EN,SL01_app1);
            }
        }
    }
#elif(APP_SCENE == APP_SCENE_HEADSET_EP1_MULTIPLEXING)
    for(u8 i=0;i<APP_AUDIO_MAX_SINK_EP;i++)
    {
        if(appCtrl.sink[i].sS)
        {
            sdu_packet_t* pPkt = blc_bapus_sduPacketPop(appCtrl.aclHandle, appCtrl.sink[i].epId);
#if (TLK_TONE_ENABLE)
            if (tlk_tone_is_playing()) {
                continue;
            }
#endif
            if(pPkt != NULL)
            {
                u32 detect = 0;
                u8 sinkChannelNum = app_get_one_in_data(appCtrl.sink[i].cP.location);
                if(pPkt->iso_sdu_len!=sinkChannelNum*appCtrl.sink[i].cP.frameOcts)
                {
                    if(!appCtrl.sink[i].sT)
                    {
                        return;
                    }
                    else
                    {
                        detect = 1;
                    }
                }
                else
                {
                    appCtrl.sink[i].sT = clock_time()|1;
                }

                audio_pkt_t pRaw={0};
                u16 audio_pcm[APP_AUDIO_SUPPORT_MAX_FRAME_SAMPLE]={0};
                for(u8 j=0;j<sinkChannelNum;j++)
                {
                    LC3DEC_Error ret_lc3 = lc3dec_set_parameter(j, LC3_PARA_BEC_DETECT, &detect);
                    if(ret_lc3!=LC3DEC_OK)
                    {
                        tlkapi_printf(APP_LOG_EN,"lc3 decode set parameter error: %d", ret_lc3);
                        return;
                    }
                    APP_DBG_CHN_2_HIGH;

                    ret_lc3 = lc3dec_decode_pkt(j,pPkt->data+j*appCtrl.sink[i].cP.frameOcts,appCtrl.sink[i].cP.frameOcts,(u8*)audio_pcm);
                    if(ret_lc3!=LC3DEC_OK)
                    {
                        tlkapi_printf(APP_LOG_EN,"lc3 decode error-ret: %d", ret_lc3);
                        return;
                    }
                    for(u16 t=0;t<codecO.fSample/sinkChannelNum;t++)
                    {
                        pRaw.buffer[sinkChannelNum*t+j] = audio_pcm[t];
                    }
                    APP_DBG_CHN_2_LOW;
                }
                pRaw.renderPoint = pPkt->timestamp + appCtrl.sink[i].pD*SYSTEM_TIMER_TICK_1US;//render point,tick count.
                app_list_add_node(&pRaw);
            }
        }
    }
#endif
}


_attribute_ram_code_ void app_timer_irq_proc(void)
{
    if(appCtrl.spk_reset){
#if (MCU_CORE_TYPE == MCU_CORE_B91)
        while(!clock_time_exceed(pStart.next->renderPoint,6));
        reg_audio_en |= FLD_AUDIO_I2S_CLK_EN;//enable i2s en
        reg_audio_codec_dac_itf_ctr= MASK_VAL( FLD_AUDIO_CODEC_FORMAT, CODEC_I2S_MODE,\
                FLD_AUDIO_CODEC_DAC_ITF_SB, CODEC_ITF_AC, \
                FLD_AUDIO_CODEC_SLAVE, I2S_M_CODEC_S, \
                FLD_AUDIO_CODEC_WL, CODEC_BIT_16_DATA);
#else
#endif
        appCtrl.spk_reset = 0;
    }
    log_task_begin_irq(ALG_TIMING_DEBUG_EN,SL01_app0);
      timer_stop(TIMER0);
      APP_DBG_CHN_3_HIGH;
      u32 readOffset = 0;
      u32 writeOffset = 0;
      u32 offset = 0;
#if (TLK_TONE_ENABLE)
      if (tlk_tone_is_playing())
      {
          app_audio_tone_handle_task();
          APP_DBG_CHN_3_LOW;
          return;
      }
#endif
      if(tlk_codec_output_getOffset(&writeOffset,&readOffset)!=TLK_CODEC_SUCCESS)
      {
          app_list_free();
//        tlkapi_printf(APP_LOG_EN,"get offset error");
          return;
      }

      if(writeOffset >= readOffset)
      {
          offset = writeOffset - readOffset;

      }
      else
      {
          offset = 2*APP_AUDIO_OUTPUT_BUFFER_SIZE + writeOffset - readOffset;
      }

      if(offset<8 || offset>24)
      {
          tlkapi_printf(1,"get offset error:0x%x,0x%x,0x%x",writeOffset,readOffset,offset);
//        APP_DBG_CHN_4_HIGH;

          if(tlk_codec_output_setWriteOffset(readOffset+16)!=TLK_CODEC_SUCCESS)
          {
              app_list_free();
//            tlkapi_printf(APP_LOG_EN,"set offset error");
              return;
          }
//        alg_spk_rptr = ((readOffset + 16)/2 + APP_AUDIO_OUTPUT_BUFFER_SIZE - 0)%APP_AUDIO_OUTPUT_BUFFER_SIZE;//ALG_AEC_DELAY
//        APP_DBG_CHN_4_LOW;
      }
      APP_DBG_CHN_4_HIGH;
      if(tlk_codec_output_dataPush((u8*)pStart.next->buffer,codecO.fOctets)!=TLK_CODEC_SUCCESS)
      {
          app_list_free();
//        tlkapi_printf(APP_LOG_EN,"data push error");
          return;
      }
      APP_DBG_CHN_4_LOW;
#if AUDIO_CLOCK_CALIB2_ALGORITHM_EN
      blc_audio_clock_calib2(pStart.next->renderPoint,pStart.next->pkt_seq_num,offset);
#else
      blc_audio_clock_calib(pStart.next->renderPoint,codecO.frameUs*SYSTEM_TIMER_TICK_1US);
#endif
      if(app_list_delete_node(pStart.next)==0)
      {
          APP_DBG_CHN_8_HIGH;
//        tlkapi_printf(APP_LOG_EN,"memory free failed-start");
          APP_DBG_CHN_8_LOW;
      }

      if(pStart.next!=NULL)
      {
          u32 capture_tick_stimer = pStart.next->renderPoint-clock_time();
          u32 capture_tick_timer  = (capture_tick_stimer*sys_clk.pclk)/SYSTEM_TIMER_TICK_1US ;
          ble_audio_timer_set_capture(TIMER0,0,capture_tick_timer);
      }
      APP_DBG_CHN_3_LOW;
      log_task_end_irq(ALG_TIMING_DEBUG_EN, SL01_app0);
}
_attribute_ram_code_
void app_codec_handler(void)
{
    app_codec_receive_process();
    app_codec_send_process();
    if(appCtrl.configCodecIdx)
    {
        app_config_codec();
        appCtrl.mic_reset = 1;
        appCtrl.spk_reset = 1;
        appCtrl.configCodecIdx = false;
    }
}
#endif
