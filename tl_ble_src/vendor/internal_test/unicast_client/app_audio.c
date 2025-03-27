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

#include "app.h"
#include "app_config.h"
#include "app_audio.h"
#include "application/usbstd/usb.h"

#if (INTER_TEST_MODE == TEST_CIS_AUDIO_CLIENT)

app_common_ctrl_t app_ctrl;


static int  app_audio_prfEvtCb(u16 connHandle, int evtID, u8 *pData, u16 dataLen);
static void app_event_acl_connect(blc_prf_aclConnEvt_t *pEvt);
static void app_event_cis_connect(u16 aclHandle,blc_audio_cisConnEvt_t *pEvt);
static void app_event_cis_disconnect(u16 aclHandle,blc_audio_cisDisconnEvt_t *pEvt);
static void app_event_acl_disconnect(blc_prf_aclDisconnEvt_t *pEvt);
static void app_event_audio_sdp_found(blc_prf_sdpFoundEvt_t *pEvt);
static void app_event_audio_sdp_not_found(blc_prf_sdpFailEvt_t *pEvt);
static void app_event_audio_sdp_over(blc_prf_sdpOverEvt_t *pEvt);
static void app_event_codec_configured(blc_bapuc_codecConfiguredEvt_t *pEvt);
static void app_event_qos_configured(blc_bapuc_qosConfiguredEvt_t *pEvt);
static void app_event_sink_stream_start(blc_audio_streamingEvt_t *pEvt);
static void app_event_source_stream_start(blc_audio_streamingEvt_t *pEvt);

static void app_codec_handler(void);
static void app_audio_receive_handler(void);
static void app_audio_send_handler(void);

static void app_list_add_node(audio_pkt_t *pData,u8 channel);
static bool app_list_delete_node(struct list_node_t *pData);
static void app_list_free(void);
static s8   app_audio_getHandleIndex(u16 connHandle);

u32 appTimerState = 0;
struct list_node_t pStart;//header node
unsigned short app_audio_spk_buffer[APP_AUDIO_SPK_BUFFER_SIZE];//48k,10ms,
unsigned short app_audio_mic_buffer[APP_AUDIO_MIC_BUFFER_SIZE];//48k,10ms,

#if(MCU_CORE_TYPE == MCU_CORE_B92)
audio_codec_stream0_input_t audio_codec_input =
{
    .input_src = DMIC_STREAM0_STEREO,
    .sample_rate = APP_AUDIO_CODEC_FREQUENCY,
    .fifo_num = FIFO0,
    .data_width = APP_AUDIO_CHANNEL,
    .dma_num = BLC_CODEC_MIC_DMA,
    .data_buf = app_audio_mic_buffer,
    .data_buf_size = sizeof(app_audio_mic_buffer),
};

audio_codec_output_t audio_codec_output =
{
    .output_src = CODEC_DAC_STEREO,
    .sample_rate = APP_AUDIO_CODEC_FREQUENCY,
    .fifo_num = FIFO0,
    .data_width = APP_AUDIO_CHANNEL,
    .dma_num = BLC_CODEC_SPK_DMA,
    .mode = HP_MODE,
    .data_buf = app_audio_spk_buffer,
    .data_buf_size = sizeof(app_audio_spk_buffer),
};
#endif

int app_audio_init(void)
{
    /* Audio profile event register */
    blc_audio_initialModule(app_audio_prfEvtCb);

    /* Audio CAP initiator init */
    blc_cap_initUnicastInitiator();

#if (ACL_PERIPHR_SMP_ENABLE || ACL_CENTRAL_SMP_ENABLE)
    /* Audio bonding initial */
    blc_prf_initPairingInfoStoreModule();
#endif

    pStart.next = NULL;
#if(APP_AUDIO_SCENE == APP_AUDIO_SCENE_TEL)
    #if(MCU_CORE_TYPE == MCU_CORE_B91)
        ble_audio_codec_init(BLC_CODEC_SUBDEV_MIC_SPK,APP_AUDIO_CODEC_FREQUENCY,APP_AUDIO_CHANNEL);
    #elif(MCU_CORE_TYPE == MCU_CORE_B92)
        ble_audio_codec_init(BLC_CODEC_SUBDEV_MIC_SPK,&audio_codec_input,&audio_codec_output);
    #endif
#elif(APP_AUDIO_SCENE == APP_AUDIO_SCENE_MUSIC)

    #if(APP_AUDIO_MUSIC_IN == APP_AUDIO_MUSIC_USB_IN)
        blc_audio_usb_init(BLC_CODEC_SUBDEV_SPK);
    #elif(APP_AUDIO_MUSIC_IN == APP_AUDIO_MUSIC_LINE_IN)
        ble_audio_codec_init(BLC_CODEC_SUBDEV_MIC, APP_AUDIO_CODEC_FREQUENCY, APP_AUDIO_CHANNEL);
    #elif(APP_AUDIO_MUSIC_IN == APP_AUDIO_MUSIC_IIS_IN)
        extern int  gAudioIisConfig;
        gAudioIisConfig = I2S_S_CODEC_M;
        gpio_input_en(I2S_BCK_PC3|I2S_DAC_LR_PC6|I2S_DAC_DAT_PC7);
        ble_audio_codec_init(BLC_CODEC_SUBDEV_IIS,APP_AUDIO_CODEC_FREQUENCY,APP_AUDIO_CHANNEL);
    #endif

#endif
    /* init asrc */
#if APP_AUDIO_ASRC_EN
    my_asrc_init_stereo(0);
    BLT_APP_LOG("Init_ASRC");
#endif

    appTimerState = APP_STATE_NONE;
    ble_audio_timer_init(TIMER0);
    app_ctrl.acl_max_num = ACL_CENTRAL_MAX_NUM;
    app_ctrl.acl_param[0].audioSink.streamTick = 0;
    app_ctrl.acl_param[1].audioSink.streamTick = 0;
    tlk_mem_pool_desc_t poolDesc[] =
    {
        { 660,  15 },//receive data use,16k,2channel,10ms,640byte each frame.
    };
    const u8 numPools = sizeof(poolDesc) / sizeof(poolDesc[0]);
    //blocks malloc
    if(tlk_mempool_init(numPools, poolDesc)!=0)
    {
        BLT_APP_LOG("error-mempool init failed!");
    }
    return 0;
}
void app_audio_handler(void)
{
    blc_prf_main_loop();
    app_codec_handler();
#if(APP_AUDIO_SCENE == APP_AUDIO_SCENE_MUSIC)

#if(APP_AUDIO_MUSIC_IN == APP_AUDIO_MUSIC_USB_IN)
    usb_handle_irq();
#endif

#endif
    app_audio_receive_handler();
    app_audio_send_handler();
}

static void app_codec_handler(void)//config lc3
{
    for(u8 i=0;i<app_ctrl.acl_max_num;i++)
    {
        if(app_ctrl.acl_param[i].audioSink.codecOperation == APP_CONFIG_CODEC)
        {
            u8 paramIndex = app_ctrl.acl_param[i].audioSink.audioParam;
            int lc3Ret = lc3dec_decode_init_bap(i,codecSettings[paramIndex].frequencyValue,\
                                                  codecSettings[paramIndex].durationValue,
                                                  codecSettings[paramIndex].frameOctets);
            if(lc3Ret != LC3DEC_OK)
            {
                BLT_APP_LOG("lc3 decode init fail:0x%x", lc3Ret);
            }
            else
            {
                BLT_APP_LOG("lc3 decode init success:0x%x", i);
            }
            app_ctrl.acl_param[i].audioSink.codecOperation = APP_CODEC_IDLE;

        }
        else if(app_ctrl.acl_param[i].audioSink.codecOperation == APP_RELEASE_CODEC)
        {
            lc3dec_free_init(i);
            app_ctrl.acl_param[i].audioSink.codecOperation = APP_CODEC_IDLE;
        }
        if(app_ctrl.acl_param[i].audioSource.codecOperation == APP_CONFIG_CODEC)
        {
            u8 paramIndex = app_ctrl.acl_param[i].audioSource.audioParam;
            int lc3Ret = lc3enc_encode_init_bap(i,codecSettings[paramIndex].frequencyValue,
                                                  codecSettings[paramIndex].durationValue,
                                                  codecSettings[paramIndex].frameOctets);
            if(lc3Ret != LC3DEC_OK)
            {
                BLT_APP_LOG("lc3  encode init fail:0x%x", lc3Ret);
            }
            else
            {
                BLT_APP_LOG("lc3 encode init success:0x%x", i);
            }
            app_ctrl.acl_param[i].audioSource.codecOperation = APP_CODEC_IDLE;
        }
        else if(app_ctrl.acl_param[i].audioSource.codecOperation == APP_RELEASE_CODEC)
        {
            lc3enc_free_init(i);
            app_ctrl.acl_param[i].audioSource.codecOperation = APP_CODEC_IDLE;
        }
    }
}

static void app_audio_receive_handler(void)
{
    for(u8 i=0;i<app_ctrl.acl_max_num;i++)
    {
        if(app_ctrl.acl_param[i].audioSink.streamStart)
        {
            sdu_packet_t* pPkt = blc_bapuc_sduPacketPop(app_ctrl.acl_param[i].acl_handle, 0);
            if(pPkt != NULL)
            {
                unsigned int detect = 0;
                if(pPkt->iso_sdu_len!=codecSettings[APP_AUDIO_CODEC_PARAMETER_PREFER].frameOctets)
                {
                    if(!app_ctrl.acl_param[i].audioSink.streamTick)
                    {
                        APP_DBG_CHN_10_HIGH;
                        APP_DBG_CHN_10_LOW;
                        return;
                    }
                    else
                    {
                        APP_DBG_CHN_11_HIGH;
                        APP_DBG_CHN_11_LOW;
                        detect = 1;
                    }
                }
                else
                {
                    APP_DBG_CHN_12_HIGH;
                    APP_DBG_CHN_12_LOW;
                    app_ctrl.acl_param[i].audioSink.streamTick = clock_time()|1;
                }
                LC3DEC_Error ret_lc3 = lc3dec_set_parameter(i, LC3_PARA_BEC_DETECT, &detect);
                if(ret_lc3!=LC3DEC_OK)
                {
                    BLT_APP_LOG("lc3 decode set parameter error:0x%x", ret_lc3);
                    return;
                }
                APP_DBG_CHN_3_HIGH;
                audio_pkt_t pRaw = {0};
                ret_lc3 = lc3dec_decode_pkt(i,pPkt->data,pPkt->iso_sdu_len,(u8*)pRaw.buffer);
                if(ret_lc3!=LC3DEC_OK)
                {
                    BLT_APP_LOG("lc3 decode error:0x%x", ret_lc3);
                    return;
                }
                else
                {
                    pRaw.renderPoint = pPkt->timestamp+app_ctrl.acl_param[i].audioSink.presentationDelay*SYSTEM_TIMER_TICK_1US\
                    +unicastQosSettings[APP_AUDIO_QOS_PARAMETER_PREFER][APP_AUDIO_CODEC_PARAMETER_PREFER].maxTransportLatency*SYSTEM_TIMER_TICK_1MS;//tick
                    app_list_add_node(&pRaw,i);
                }
                APP_DBG_CHN_3_LOW;
            }
        }
    }
}
u32 app_audio_send_tick = 0;
static void app_audio_send_handler(void)//ok
{
    if(!(app_ctrl.acl_param[0].audioSource.streamStart || app_ctrl.acl_param[1].audioSource.streamStart))
    {
        return;
    }
    if(clock_time_exceed(app_audio_send_tick, 30*1000)&&app_audio_send_tick)
    {
        app_audio_send_tick = 0;
        BLT_APP_LOG("no data need send");
    }
#if(APP_AUDIO_SCENE == APP_AUDIO_SCENE_TEL)
    u16 lenPcm  = blc_codec_getMicDataLen();
#elif(APP_AUDIO_SCENE == APP_AUDIO_SCENE_MUSIC)

    #if(APP_AUDIO_MUSIC_IN == APP_AUDIO_MUSIC_USB_IN)
        u16 lenPcm  = blc_audio_getDataLen();
    #else
        u16 lenPcm  = blc_codec_getMicDataLen();
    #endif

#endif
    if(lenPcm<2*APP_AUDIO_FRAME_SAMPLE_BYTES)
    {
        return;
    }

    u16 audioEncLen = codecSettings[APP_AUDIO_CODEC_PARAMETER_PREFER].frameOctets;
    u16 audio_pcm[APP_AUDIO_FRAME_SAMPLE]={0};
    u8  audio_enc[ACL_CENTRAL_MAX_NUM][APP_AUDIO_SUPPORT_MAX_ENCODE_BYTES]={0};
    u16 pcmData[ACL_CENTRAL_MAX_NUM*APP_AUDIO_FRAME_SAMPLE]={0};

#if(APP_AUDIO_SCENE == APP_AUDIO_SCENE_TEL)
    u8 codecRet = blc_codec_readMicBuff((u8*)pcmData,2*APP_AUDIO_FRAME_SAMPLE_BYTES);
#elif(APP_AUDIO_SCENE == APP_AUDIO_SCENE_MUSIC)

    #if(APP_AUDIO_MUSIC_IN == APP_AUDIO_MUSIC_USB_IN)
        if(app_audio_send_tick == 0)
        {
            extern void blc_cis_get_tx_point(u8 role);
            blc_cis_get_tx_point(1);
            blc_audio_clearBuffer();
            app_audio_send_tick = clock_time();
            //if the audio stream is interrupted,lc3 should be reitialized.
            app_ctrl.acl_param[0].audioSource.streamStart == true?app_ctrl.acl_param[0].audioSource.codecOperation = APP_CONFIG_CODEC:0;
            app_ctrl.acl_param[1].audioSource.streamStart == true?app_ctrl.acl_param[1].audioSource.codecOperation = APP_CONFIG_CODEC:0;
            return;
        }
        u8 codecRet = blc_audio_readBuffer((u8*)pcmData,2*APP_AUDIO_FRAME_SAMPLE_BYTES);
        blc_usb_adjust_volume((u16*)pcmData,2*APP_AUDIO_FRAME_SAMPLE);
    #else
        u8 codecRet = blc_codec_readMicBuff((u8*)pcmData,2*APP_AUDIO_FRAME_SAMPLE_BYTES);
    #endif

#endif

    if(codecRet!=true)
    {
        BLT_APP_LOG("get mic data error");
    }

#if APP_AUDIO_ASRC_EN
    my_asrc_data_stereo((short *)pcmData, 960,(short *)pcmData);
#endif

    for(u8 i=0;i<app_ctrl.acl_max_num;i++)
    {
        APP_DBG_CHN_1_HIGH;
        if(app_ctrl.acl_param[i].audioSource.streamStart)
        {
            for(u16 j=0;j<APP_AUDIO_FRAME_SAMPLE;j++)
            {
                audio_pcm[j] = pcmData[ACL_CENTRAL_MAX_NUM*j+i];
            }
            LC3ENC_Error ret_lc3 = lc3enc_encode_pkt(i,(u8*)audio_pcm,audio_enc[i]);
            if(ret_lc3!=LC3ENC_OK)
            {
                BLT_APP_LOG("lc3 encode error:0x%x", ret_lc3);
            }
        }
        APP_DBG_CHN_1_LOW;
    }
    for(u8 i=0;i<app_ctrl.acl_max_num;i++)
    {
        if(app_ctrl.acl_param[i].audioSource.streamStart)
        {
            APP_DBG_CHN_2_HIGH;
            app_audio_send_tick = clock_time();
            int ret = blc_bapuc_sduPacketPush(app_ctrl.acl_param[i].acl_handle, 0, audio_enc[i], audioEncLen);
            if(ret != AUDIO_ESUCC)
            {
                BLT_APP_LOG("sdu send fail-ret:0x%x", ret);
            }
            APP_DBG_CHN_2_LOW;
        }
    }

}


static int app_audio_prfEvtCb(u16 connHandle, int evtID, u8 *pData, u16 dataLen)
{
    switch(evtID){
    case PRF_EVTID_ACL_CONNECT:
    {
        BLT_APP_LOG("app event-acl connect");
        app_event_acl_connect((blc_prf_aclConnEvt_t*)pData);
    }
    break;

    case PRF_EVTID_ACL_DISCONNECT:
    {
        BLT_APP_LOG("app event-acl disconnect");
        app_event_acl_disconnect((blc_prf_aclDisconnEvt_t*)pData);
    }
    break;

    case AUDIO_EVT_CIS_CONNECT:
    {
        BLT_APP_LOG("app event-cis connect");
        app_event_cis_connect(connHandle,(blc_audio_cisConnEvt_t *)pData);
    }
    break;

    case AUDIO_EVT_CIS_DISCONNECT:
    {
        BLT_APP_LOG("app event-cis disconnect");
        app_event_cis_disconnect(connHandle,(blc_audio_cisDisconnEvt_t *)pData);
    }
    break;

    case AUDIO_EVT_BAPUC_CODEC_CONFIGURED:
    {
        BLT_APP_LOG("app event-codec configured");
        app_event_codec_configured((blc_bapuc_codecConfiguredEvt_t *)pData);
    }
    break;

    case AUDIO_EVT_BAPUC_QOS_CONFIGURED:
    {
        BLT_APP_LOG("app event-qos configured");
        app_event_qos_configured((blc_bapuc_qosConfiguredEvt_t *)pData);
    }
    break;

    case AUDIO_EVT_BAPUC_RECEIVE_STREAMING: /* Client as Audio Sink, start to receive audio */
    {
        BLT_APP_LOG("app event-sink stream start");
        app_event_sink_stream_start((blc_audio_streamingEvt_t *)pData);
    }
    break;

    case AUDIO_EVT_BAPUC_SEND_STREAMING: /* Client as Audio Source, start to send audio */
    {
        BLT_APP_LOG("app event-source stream start");
        app_event_source_stream_start((blc_audio_streamingEvt_t *)pData);
    }
    break;

    case PRF_EVTID_CLIENT_SDP_FOUND:
    {
        BLT_APP_LOG("app event-audio sdp found");
        app_event_audio_sdp_found((blc_prf_sdpFoundEvt_t *)pData);
    }
    break;

    case PRF_EVTID_CLIENT_SDP_FAIL:
    {
        BLT_APP_LOG("app event-audio sdp not found");
        app_event_audio_sdp_not_found((blc_prf_sdpFailEvt_t *)pData);
    }
    break;

    case PRF_EVTID_CLIENT_ALL_SDP_OVER:
    {
        BLT_APP_LOG("app event-audio sdp over");
        app_event_audio_sdp_over((blc_prf_sdpOverEvt_t *)pData);
    }
    break;

    default:
        BLT_APP_LOG("unprocessed audio event:0x%x", evtID);
        break;
    }
    return 0;
}

_attribute_ram_code_ void app_timer_irq_proc(void)
{
    timer_stop(TIMER0);
    if(appTimerState == APP_STATE_RENDER_START)
    {
          int spkReadOffset  = ble_codec_getSpkReadOffset();
          if(spkReadOffset<0)
          {
              BLT_APP_LOG("audio state error-read offset:0x%x", spkReadOffset);
              return;
          }
          APP_DBG_CHN_4_HIGH;
          u32 spkSetOffset = spkReadOffset+32;//8 sample buffer
          u32 setOffsetRet = ble_codec_setSpkWriteOffset(spkSetOffset);
          if(setOffsetRet!=BLC_CODEC_SUCCESS)
          {
              BLT_APP_LOG("audio state error-set offset:0x%x", setOffsetRet);
              return;
          }
          u32 setBufferRet = blc_codec_WriteSpkBuff((u8*)pStart.next->buffer,2*APP_AUDIO_FRAME_SAMPLE_BYTES);
          if(setBufferRet!=true)
          {
              BLT_APP_LOG("audio state error-fill buffer");
              return;
          }
          if(app_list_delete_node(pStart.next)==0)
          {
              BLT_APP_LOG("memory free failed-start");
          }
          if(pStart.next!=NULL)
          {
              u32 capture_tick_stimer = pStart.next->renderPoint-clock_time();
              u32 capture_tick_timer  = (capture_tick_stimer*sys_clk.pclk)/SYSTEM_TIMER_TICK_1US ;
              ble_audio_timer_set_capture(TIMER0,0,capture_tick_timer);
              appTimerState = APP_STATE_RENDER_CONTINUE;
          }
          else
          {
              appTimerState = APP_STATE_NONE;
          }
          APP_DBG_CHN_4_LOW;
    }
    else if(appTimerState == APP_STATE_RENDER_CONTINUE)
    {
        int spkWriteOffset = ble_codec_getSpkWriteOffset();
        int spkReadOffset  = ble_codec_getSpkReadOffset();
        if(spkWriteOffset<0 || spkReadOffset<0)
        {
            BLT_APP_LOG("audio state error-write offset:0x%x", spkWriteOffset);
            BLT_APP_LOG("audio state error-read offset:0x%x", spkReadOffset);
            return;
        }
        u32 spkSetOffset = spkReadOffset;
        if(spkReadOffset>spkWriteOffset)
        {
            if(spkReadOffset>APP_AUDIO_SPK_BUFFER_SIZE && spkWriteOffset<APP_AUDIO_SPK_BUFFER_SIZE)
            {
                //nothing to do
            }
            else
            {
                spkSetOffset = spkReadOffset+16;//4 sample buffer
                u32 setOffsetRet = ble_codec_setSpkWriteOffset(spkSetOffset);
                if(setOffsetRet!=BLC_CODEC_SUCCESS)
                {
                    BLT_APP_LOG("audio state error-set offset:0x%x", setOffsetRet);
                    return;
                }
                BLT_APP_LOG("jump 1 sample - spkReadOffset:0x%x", spkReadOffset);
                BLT_APP_LOG("jump 1 sample - spkWriteOffset:0x%x", spkWriteOffset);
            }
        }
        APP_DBG_CHN_5_HIGH;
        u32 setBufferRet = blc_codec_WriteSpkBuff((u8*)pStart.next->buffer,2*APP_AUDIO_FRAME_SAMPLE_BYTES);
        if(setBufferRet!=true)
        {
            BLT_APP_LOG("audio state error-fill buffer");
            return;
        }
        if(app_list_delete_node(pStart.next)==0)
        {
            BLT_APP_LOG("memory free failed-continue");
        }
        if(pStart.next!=NULL)
        {
            u32 capture_tick_stimer = pStart.next->renderPoint-clock_time();
            u32 capture_tick_timer  = (capture_tick_stimer*sys_clk.pclk)/SYSTEM_TIMER_TICK_1US ;
            ble_audio_timer_set_capture(TIMER0,0,capture_tick_timer);
            appTimerState = APP_STATE_RENDER_CONTINUE;
        }
        else
        {
            appTimerState = APP_STATE_NONE;
        }
        APP_DBG_CHN_5_LOW;
    }
    else
    {

    }
}

/**
 * @brief     This function servers to set USB Input.
 * @param[in] none
 * @return    none.
 */
_attribute_ram_code_ void  app_usb_irq_proc ()
{
    u8 usbData[256]={0};
    unsigned char len = reg_usb_ep6_ptr;
    usbhw_reset_ep_ptr(USB_EDP_SPEAKER);
    APP_DBG_CHN_6_HIGH;
    for (unsigned int i=0; i<len; i++)
    {
        usbData[i] = reg_usb_ep6_dat;
    }
    blc_audio_writeBuffer(usbData,len);
    usbhw_data_ep_ack(USB_EDP_SPEAKER);
    APP_DBG_CHN_6_LOW;
}

_attribute_ram_code_ static void app_list_add_node(audio_pkt_t *pData,u8 channel)
{
    if(pStart.next==NULL)
    {
        struct list_node_t *pNew = (struct list_node_t*)tlk_mem_malloc(sizeof(struct list_node_t));
        if(pNew == NULL)
        {
            BLT_APP_LOG("mempool malloc failed,pos1!");
            while(1)
            {
                ////////////////////////////////////// Debug entry /////////////////////////////////
                #if (TLKAPI_DEBUG_ENABLE)
                    tlkapi_debug_handler();
                #endif
            }
        }
        pNew->renderPoint = pData->renderPoint;
        for(int i=0;i<APP_AUDIO_FRAME_SAMPLE;i++)
        {
            pNew->buffer[ACL_CENTRAL_MAX_NUM*i+channel] = pData->buffer[i];
        }
        pStart.next = pNew;
        pNew->next = NULL;
        u32 capture_tick_stimer = pData->renderPoint-clock_time();
        u32 capture_tick_timer  = (capture_tick_stimer*sys_clk.pclk)/SYSTEM_TIMER_TICK_1US ;
        ble_audio_timer_set_capture(TIMER0,0,capture_tick_timer);
        appTimerState = APP_STATE_RENDER_START;
        return;
    }
    else
    {
        struct list_node_t *pTemp = &pStart;
        while(pTemp->next!=NULL)
        {
            pTemp = pTemp->next;
            if(pData->renderPoint == pTemp->renderPoint)
            {
                APP_DBG_CHN_8_HIGH;
                for(int i=0;i<APP_AUDIO_FRAME_SAMPLE;i++)
                {
                    pTemp->buffer[ACL_CENTRAL_MAX_NUM*i+channel] = pData->buffer[i];
                }
                APP_DBG_CHN_8_LOW;
                return;
            }
        }
        APP_DBG_CHN_9_HIGH;
        struct list_node_t *pNew = (struct list_node_t*)tlk_mem_malloc(sizeof(struct list_node_t));
        if(pNew == NULL)
        {
            BLT_APP_LOG("mempool malloc failed,pos2!");
            while(1)
            {
                ////////////////////////////////////// Debug entry /////////////////////////////////
                #if (TLKAPI_DEBUG_ENABLE)
                    tlkapi_debug_handler();
                #endif
            }
        }
        pNew->renderPoint = pData->renderPoint;
        for(int i=0;i<APP_AUDIO_FRAME_SAMPLE;i++)
        {
            pNew->buffer[ACL_CENTRAL_MAX_NUM*i+channel] = pData->buffer[i];
        }
        pTemp->next = pNew;
        pNew->next = NULL;
        APP_DBG_CHN_9_LOW;
    }
}

_attribute_ram_code_ static bool app_list_delete_node(struct list_node_t *pData)
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
                memset((u8*)pDel->buffer,0,4*APP_AUDIO_FRAME_SAMPLE);
                TLK_MEM_STATE_T memRet = tlk_mem_free(pDel);
                if(memRet!=TLK_MEM_SUCCESS)
                {
                    BLT_APP_LOG("mempool free failed,pos1!");
                }
                return 1;
            }
            else
            {
                struct list_node_t *pDel = pTemp->next;
                pTemp->next=NULL;
                memset((u8*)pDel->buffer,0,4*APP_AUDIO_FRAME_SAMPLE);
                TLK_MEM_STATE_T memRet = tlk_mem_free(pDel);
                if(memRet!=TLK_MEM_SUCCESS)
                {
                    BLT_APP_LOG("mempool free failed,pos2!");
                }
                return 1;
            }
        }
        pTemp = pTemp->next;
    }
    return 0;
}
_attribute_ram_code_ void app_list_free(void)
{
    while(pStart.next!=NULL)
    {
        app_list_delete_node(pStart.next);
    }
}

static s8 app_audio_getHandleIndex(u16 connHandle)
{
    for(int i=0;i<app_ctrl.acl_max_num;i++)
    {
        if(app_ctrl.acl_param[i].acl_handle == connHandle)
        {
            return i;
        }
    }
    return -1;
}

static void app_event_acl_connect(blc_prf_aclConnEvt_t *pEvt)
{
    for(int i=0;i<app_ctrl.acl_max_num;i++)
    {
        if(app_ctrl.acl_param[i].acl_handle == 0)
        {
            app_ctrl.acl_param[i].acl_handle = pEvt->aclHandle;
            app_ctrl.acl_param[i].cis_handle = 0;
            BLT_APP_LOG("acl handle match-index:0x%x", i);
            BLT_APP_LOG("acl handle match-handle:0x%x", app_ctrl.acl_param[i].acl_handle);
            break;
        }
    }
    app_ctrl.acl_count++;
    if(app_ctrl.acl_count >= app_ctrl.acl_max_num)
    {
        blc_ll_setExtScanEnable( BLC_SCAN_DISABLE, DUP_FILTER_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
        BLT_APP_LOG("acl connect cnt-stop scan:0x%x", app_ctrl.acl_count);
    }
    else
    {
        blc_ll_setExtScanEnable( BLC_SCAN_ENABLE, DUP_FILTER_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
        BLT_APP_LOG("acl connect cnt-start scan:0x%x", app_ctrl.acl_count);
    }
}
static void app_event_cis_connect(u16 aclHandle,blc_audio_cisConnEvt_t *pEvt)
{
    s8 acl_index = app_audio_getHandleIndex(aclHandle);
    if(acl_index<0)
    {
        BLT_APP_LOG("error-get acl handle:0x%x", aclHandle);
    }
    BLT_APP_LOG("nse:0x%x", pEvt->nse);
    BLT_APP_LOG("ft m2s:0x%x", pEvt->ft_m2s);
    BLT_APP_LOG("ft s2m:0x%x", pEvt->ft_s2m);
    app_ctrl.acl_param[acl_index].cis_handle = pEvt->cisHandle;
}

static void app_event_cis_disconnect(u16 aclHandle,blc_audio_cisDisconnEvt_t *pEvt)
{
    s8 acl_index = app_audio_getHandleIndex(aclHandle);
    if(acl_index<0)
    {
        BLT_APP_LOG("error-get acl handle:0x%x", aclHandle);
        return;
    }
    app_ctrl.acl_param[acl_index].cis_handle = 0;
    app_ctrl.acl_param[acl_index].audioSink.streamStart = false;
    app_ctrl.acl_param[acl_index].audioSource.streamStart = false;
    BLT_APP_LOG("reason:0x%x", pEvt->reason);
}
static void app_event_acl_disconnect(blc_prf_aclDisconnEvt_t *pEvt)
{
    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if(acl_index<0)
    {
        BLT_APP_LOG("error-get acl handle:0x%x", pEvt->aclHandle);
        return;
    }
    app_ctrl.acl_count--;
    if(app_ctrl.acl_count == 0)
    {
#if(APP_AUDIO_SCENE == APP_AUDIO_SCENE_TEL)
        ble_audio_codec_close();
        ble_audio_codec_init(BLC_CODEC_SUBDEV_MIC_SPK,APP_AUDIO_CODEC_FREQUENCY,APP_AUDIO_CHANNEL);
#endif
    }
    memset((u8*)&app_ctrl.acl_param[acl_index],0,sizeof(app_acl_param_t));
    int scan_ret = blc_ll_setExtScanEnable( BLC_SCAN_ENABLE, DUP_FILTER_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
    if(scan_ret!=BLE_SUCCESS)
    {
        BLT_APP_LOG("start scan error:0x%x", scan_ret);
    }
    else
    {
        BLT_APP_LOG("start scan success:0x%x", scan_ret);
    }
}
static void app_event_audio_sdp_found(blc_prf_sdpFoundEvt_t *pEvt)
{
    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if(acl_index<0)
    {
        BLT_APP_LOG("error-get acl handle:0x%x", pEvt->aclHandle);
        return;
    }

    if(pEvt->svcId == AUDIO_CSIS_CLIENT){
        if(app_ctrl.acl_count == 1){
            app_ctrl.acl_csis_exist = 1;
            app_ctrl.acl_csis_size = 0; //clear. update latter by clientSDP
            BLT_APP_LOG("acceptor with CSIS");
        }
    }
}
static void app_event_audio_sdp_not_found(blc_prf_sdpFailEvt_t *pEvt)
{
    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if(acl_index<0)
    {
        BLT_APP_LOG("error-get acl handle:0x%x", pEvt->aclHandle);
        return;
    }

    if(pEvt->svcId == AUDIO_CSIS_CLIENT){
        if(app_ctrl.acl_count == 1){
            app_ctrl.acl_csis_exist = 0;
            app_ctrl.acl_csis_size = 1;
            BLT_APP_LOG("acceptor without CSIS, client support 1 acceptor");
        }
    }
}
static void app_event_audio_sdp_over(blc_prf_sdpOverEvt_t *pEvt)
{
    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if(acl_index<0)
    {
        BLT_APP_LOG("error-get acl handle:0x%x", pEvt->aclHandle);
        return;
    }

    if(app_ctrl.acl_count == 1 && app_ctrl.acl_csis_exist){
        int r = blc_csisc_getSetIdentityResolvingKey(pEvt->aclHandle, app_ctrl.acl_csis_sirk);
        if(AUDIO_ESUCC != r){
            BLT_APP_LOG("error-get SIRK:0x%x", r);
            return;
        }else{
            BLT_APP_LOG("CSISC: get SIRK:%s", hex_to_str(app_ctrl.acl_csis_sirk, 16));
        }
        r = blc_csisc_getCoordinatedSetSize(pEvt->aclHandle, &app_ctrl.acl_csis_size);
        if(AUDIO_ESUCC != r){
            BLT_APP_LOG("error-get SetSize:0x%x", r);
            return;
        }else{
            BLT_APP_LOG("CSISC: SetSize:0x%x", app_ctrl.acl_csis_size);
        }
    }

    blc_audio_ase_cfg_info_t audChnInfo;
    int audioRet = blc_bapuc_checkAudioConfigures(pEvt->aclHandle, APP_AUDIO_CONFIGURATION_PREFER, &audChnInfo);
    if(audioRet!= AUDIO_ESUCC)
    {
        BLT_APP_LOG("error-audio configurations:0x%x", audioRet);
        return;
    }


    //Attention TODO:  delete smp bonding cause Crash, TODO:
    //return;


#if (0) //Current audio configuration can fit this
    if(audChnInfo.sinkASEsPerSvr && audChnInfo.sinkASEsPerSvr != 1){
        BLT_APP_LOG("error-audio configurations: sinkASEsPerSvr:0x%x", audChnInfo.sinkASEsPerSvr);
        return;
    }

    if(audChnInfo.srcASEsPerSvr && audChnInfo.srcASEsPerSvr != 1){
        BLT_APP_LOG("error-audio configurations: srcASEsPerSvr:0x%x", audChnInfo.srcASEsPerSvr);
        return;
    }
#endif

    ////////////////////// SVR: SINK; CLT: SOURCE //////////////////////////////////////
    for(int i = 0; i<audChnInfo.sinkASEsPerSvr; i++){

        audioRet = blc_bapuc_setAseConfigCodec(pEvt->aclHandle, AUDIO_DIR_SINK, i, APP_AUDIO_CODEC_PARAMETER_PREFER, &audChnInfo);
        if(audioRet!= AUDIO_ESUCC)
        {
            BLT_APP_LOG("error-unicast config audio:0x%x", audioRet);
        }
        else
        {
            app_ctrl.acl_param[acl_index].audioSource.audioParam = APP_AUDIO_CODEC_PARAMETER_PREFER;
            app_ctrl.acl_param[acl_index].audioSource.location = audChnInfo.sinkAudLocAlloc[i];
            app_ctrl.acl_param[acl_index].audioSource.codecOperation = APP_CONFIG_CODEC;
        }
    }

    ////////////////////// SVR: SOURCE;  CLT: SINK //////////////////////////////////////
    for(int i = 0; i<audChnInfo.srcASEsPerSvr; i++){

        audioRet = blc_bapuc_setAseConfigCodec(pEvt->aclHandle, AUDIO_DIR_SOURCE, i, APP_AUDIO_CODEC_PARAMETER_PREFER, &audChnInfo);
        if(audioRet!= AUDIO_ESUCC)
        {
            BLT_APP_LOG("error-unicast config audio:0x%x", audioRet);
        }
        else
        {
            app_ctrl.acl_param[acl_index].audioSink.audioParam = APP_AUDIO_CODEC_PARAMETER_PREFER;
            app_ctrl.acl_param[acl_index].audioSink.location = audChnInfo.srcAudLocAlloc[i];
            app_ctrl.acl_param[acl_index].audioSink.codecOperation = APP_CONFIG_CODEC;
        }
    }
}
static void app_event_codec_configured(blc_bapuc_codecConfiguredEvt_t *pEvt)
{
    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if(acl_index<0)
    {
        BLT_APP_LOG("error-get acl handle:0x%x", pEvt->aclHandle);
        return;
    }
    if(pEvt->maxTransportLatency >= unicastQosSettings[APP_AUDIO_QOS_PARAMETER_PREFER][APP_AUDIO_CODEC_PARAMETER_PREFER].maxTransportLatency)
    {
        blc_bapuc_setAseConfigQos(pEvt->aclHandle, pEvt->aseID, APP_AUDIO_QOS_PARAMETER_PREFER);
    }
}
static void app_event_qos_configured(blc_bapuc_qosConfiguredEvt_t*pEvt)
{
    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if(acl_index<0)
    {
        BLT_APP_LOG("error-get acl handle:0x%x", pEvt->aclHandle);
        return;
    }
    if(pEvt->aseDir == AUDIO_DIR_SOURCE)
    {
        app_ctrl.acl_param[acl_index].audioSink.presentationDelay = pEvt->presentationDelay;
        BLT_APP_LOG("sink presentation delay:0x%x",  app_ctrl.acl_param[acl_index].audioSink.presentationDelay);
    }
    else{
        BLT_APP_LOG("source presentation delay:0x%x", pEvt->presentationDelay);
    }
}
static void app_event_sink_stream_start(blc_audio_streamingEvt_t *pEvt)
{
    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if(acl_index<0)
    {
        BLT_APP_LOG("error-get acl handle:0x%x", pEvt->aclHandle);
        return;
    }
    ble_codec_setSpkBuffer((u8*)app_audio_spk_buffer,2*APP_AUDIO_SPK_BUFFER_SIZE);
    u32 codecRet = ble_codec_spkOpen();
    if(codecRet==BLC_CODEC_REPEAT)
    {
        BLT_APP_LOG("codec spk already open");
    }
    else if(codecRet==BLC_CODEC_NOINIT)
    {
        BLT_APP_LOG("codec spk no init");
    }
    else
    {
        BLT_APP_LOG("codec spk open success");
    }
    app_ctrl.acl_param[acl_index].audioSink.streamStart = true;
}
static void app_event_source_stream_start(blc_audio_streamingEvt_t *pEvt)
{

    extern void blc_cis_get_tx_point(u8 role);
    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if(acl_index<0)
    {
        BLT_APP_LOG("error-get acl handle:0x%x", pEvt->aclHandle);
        return;
    }
#if(APP_AUDIO_SCENE == APP_AUDIO_SCENE_TEL)

    if(!(app_ctrl.acl_param[0].audioSource.streamStart || app_ctrl.acl_param[1].audioSource.streamStart))
    {
        blc_cis_get_tx_point(1);
    }
    ble_codec_setMicBuffer((u8*)app_audio_mic_buffer,2*APP_AUDIO_MIC_BUFFER_SIZE);
    u32 codecRet = ble_codec_micOpen();

    if(codecRet==BLC_CODEC_REPEAT)
    {
        BLT_APP_LOG("codec mic already open");
    }
    else if(codecRet==BLC_CODEC_NOINIT)
    {
        BLT_APP_LOG("codec mic no init");
    }
    else
    {
        BLT_APP_LOG("codec mic open success");
    }
#elif(APP_AUDIO_SCENE == APP_AUDIO_SCENE_MUSIC)

#if(APP_AUDIO_MUSIC_IN == APP_AUDIO_MUSIC_USB_IN)
    blc_audio_setBuffer((u8*)app_audio_mic_buffer,2*APP_AUDIO_MIC_BUFFER_SIZE);
#else
    if(!(app_ctrl.acl_param[0].audioSource.streamStart || app_ctrl.acl_param[1].audioSource.streamStart))
    {
        blc_cis_get_tx_point(1);
    }
    ble_codec_setMicBuffer((u8*)app_audio_mic_buffer,2*APP_AUDIO_MIC_BUFFER_SIZE);
    u32 codecRet = ble_codec_micOpen();

    if(codecRet==BLC_CODEC_REPEAT)
    {
        BLT_APP_LOG("codec mic already open");
    }
    else if(codecRet==BLC_CODEC_NOINIT)
    {
        BLT_APP_LOG("codec mic no init");
    }
    else
    {
        BLT_APP_LOG("codec mic open success");
    }
#endif

#endif
    app_ctrl.acl_param[acl_index].audioSource.streamStart = true;
}


#endif /* INTER_TEST_MODE */
