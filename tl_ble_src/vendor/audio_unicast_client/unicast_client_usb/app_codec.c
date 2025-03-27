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
#include "application/usbstd/usb.h"
#include "vendor/common/tlk_api/tlk_mem.h"
#include "vendor/common/tlk_api/tlk_buffer.h"
#include "algorithm/audio_alg/lc3/lc3.h"

#if (UNICAST_CLIENT_SELECT == UNICAST_CLIENT_USB)
extern app_ctrl_t appCtrl;
app_usb_param_t usbI;
app_usb_param_t usbO;
struct list_node_t pStart;//header node
unsigned short app_audio_output_buffer[APP_AUDIO_OUTPUT_BUFFER_SIZE];//48k,10ms,
unsigned short app_audio_input_buffer[APP_AUDIO_INPUT_BUFFER_SIZE];//48k,10ms,

#if(APP_AUDIO_SCENE == APP_SCENE_TWS)
static void app_list_add(audio_pkt_t *pData,u8 channel);
#elif(APP_AUDIO_SCENE == APP_SCENE_HEADSET)
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
void app_codec_init()
{
    ble_audio_timer_init(TIMER0);
    pStart.next = NULL;
#if(APP_AUDIO_SCENE == APP_SCENE_TWS)
    blc_audio_usb_init();
    reg_usb_ep7_buf_addr = 0x00;
    tlk_buffer_init((u8*)app_audio_input_buffer,2*APP_AUDIO_INPUT_BUFFER_SIZE,TLK_BUFFER_1);
    tlk_buffer_init((u8*)app_audio_output_buffer,2*APP_AUDIO_OUTPUT_BUFFER_SIZE,TLK_BUFFER_2);
    usbI.cC = 2;
    usbI.fSample = gLc3Index[codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].frequencyValue][codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].durationValue].frameSample;
    usbI.fOctets = 2*usbI.fSample;
    usbI.frameUs = gLc3Index[codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].frequencyValue][codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].durationValue].duraUs;

    usbO.cC = 2;
    usbO.fSample = gLc3Index[codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].frequencyValue][codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].durationValue].frameSample;
    usbO.fOctets = 2*usbO.fSample;
    usbO.frameUs = gLc3Index[codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].frequencyValue][codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].durationValue].duraUs;
    tlkapi_printf(APP_LOG_EN,"usbI.fSample %d\n",usbI.fSample);
    tlkapi_printf(APP_LOG_EN,"usbI.fOctets %d\n",usbI.fOctets);
    tlkapi_printf(APP_LOG_EN,"usbI.frameUs %d\n",usbI.frameUs);

    tlkapi_printf(APP_LOG_EN,"usbO.fSample %d\n",usbO.fSample);
    tlkapi_printf(APP_LOG_EN,"usbO.fOctets %d\n",usbO.fOctets);
    tlkapi_printf(APP_LOG_EN,"usbO.frameUs %d\n",usbO.frameUs);
    tlk_mem_pool_desc_t poolDesc[] =
    {
        { 660,  8},//receive data use,16k,2channel,10ms,640byte each frame.
    };

#elif(APP_AUDIO_SCENE == APP_SCENE_HEADSET)
    blc_audio_usb_init();
    tlk_buffer_init((u8*)app_audio_input_buffer,2*APP_AUDIO_INPUT_BUFFER_SIZE,TLK_BUFFER_1);
    tlk_buffer_init((u8*)app_audio_output_buffer,2*APP_AUDIO_OUTPUT_BUFFER_SIZE,TLK_BUFFER_2);

    usbI.cC = 2;
    usbI.fSample = gLc3Index[codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].frequencyValue][codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].durationValue].frameSample;
    usbI.fOctets = 2*usbI.fSample;
    usbI.frameUs = gLc3Index[codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].frequencyValue][codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].durationValue].duraUs;

    usbO.cC = 1;
    usbO.fSample = gLc3Index[codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].frequencyValue][codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].durationValue].frameSample;
    usbO.fOctets = 2*usbO.fSample;
    usbO.frameUs = gLc3Index[codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].frequencyValue][codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].durationValue].duraUs;

    tlkapi_printf(APP_LOG_EN,"usbI.fSample %d\n",usbI.fSample);
    tlkapi_printf(APP_LOG_EN,"usbI.fOctets %d\n",usbI.fOctets);
    tlkapi_printf(APP_LOG_EN,"usbI.frameUs %d\n",usbI.frameUs);

    tlkapi_printf(APP_LOG_EN,"usbO.fSample %d\n",usbO.fSample);
    tlkapi_printf(APP_LOG_EN,"usbO.fOctets %d\n",usbO.fOctets);
    tlkapi_printf(APP_LOG_EN,"usbO.frameUs %d\n",usbO.frameUs);
    tlk_mem_pool_desc_t poolDesc[] =
    {
        { 340,  8},//receive data use,16k,1channel,10ms,320 byte each frame.
    };
#endif
    const u8 numPools = sizeof(poolDesc) / sizeof(poolDesc[0]);
    //blocks malloc
    if(tlk_mempool_init(numPools, poolDesc)!=0)
    {
        tlkapi_printf(APP_LOG_EN,"error-mempool init failed!\n");
    }
}

/**
 * @brief      Codec process in loop function,include audio data send,audio data receive and other codec process.
 * @param[in]  none.
 * @return     none.
 */
void app_codec_handler()
{
    app_audio_receive_process();
    app_audio_send_process();
    app_codec_process();
    usb_handle_irq();
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

static void app_codec_process(void)//config lc3
{
    for(u8 i=0;i<appCtrl.acl_max_num;i++)
    {
        if(appCtrl.aclParam[i].sink.codecOp == APP_CONFIG_CODEC)
        {
            u8 paramIndex = appCtrl.aclParam[i].sink.codecParam;
            u16 sinkChannelNum = app_get_one_in_data(appCtrl.aclParam[i].sink.location);
            if(sinkChannelNum>1)
            {
                for(u8 j=0;j<sinkChannelNum;j++)
                {
                    int lc3Ret = lc3dec_decode_init_bap(j,codecSettings[paramIndex].frequencyValue,\
                                                           codecSettings[paramIndex].durationValue,
                                                           codecSettings[paramIndex].frameOctets);
                    if(lc3Ret != LC3DEC_OK)
                    {
                        tlkapi_printf(APP_LOG_EN,"lc3 decode init fail:0x%x\n", lc3Ret);
                        return;
                    }
                    tlkapi_printf(APP_LOG_EN,"j %d\n", j);
                    tlkapi_printf(APP_LOG_EN,"lc3 decode init success:0x%x\n", j);
                    tlkapi_printf(APP_LOG_EN,"codecSettings[paramIndex].frequencyValue %d\n", codecSettings[paramIndex].frequencyValue);
                    tlkapi_printf(APP_LOG_EN,"codecSettings[paramIndex].durationValue  %d\n", codecSettings[paramIndex].durationValue);
                    tlkapi_printf(APP_LOG_EN,"codecSettings[paramIndex].frameOctets    %d\n", codecSettings[paramIndex].frameOctets);
                }
            }
            else
            {
                int lc3Ret = lc3dec_decode_init_bap(i,codecSettings[paramIndex].frequencyValue,\
                                                       codecSettings[paramIndex].durationValue,
                                                       codecSettings[paramIndex].frameOctets);
                if(lc3Ret != LC3DEC_OK)
                {
                    tlkapi_printf(APP_LOG_EN,"lc3 decode init fail:0x%x", lc3Ret);
                    return;
                }
                tlkapi_printf(APP_LOG_EN,"i %d", i);
                tlkapi_printf(APP_LOG_EN,"lc3 decode init success:0x%x", i);
                tlkapi_printf(APP_LOG_EN,"codecSettings[paramIndex].frequencyValue %d", codecSettings[paramIndex].frequencyValue);
                tlkapi_printf(APP_LOG_EN,"codecSettings[paramIndex].durationValue  %d", codecSettings[paramIndex].durationValue);
                tlkapi_printf(APP_LOG_EN,"codecSettings[paramIndex].frameOctets    %d", codecSettings[paramIndex].frameOctets);
            }
            appCtrl.aclParam[i].sink.codecOp = APP_CODEC_IDLE;
        }
        if(appCtrl.aclParam[i].source.codecOp == APP_CONFIG_CODEC)
        {
            u8 paramIndex = appCtrl.aclParam[i].source.codecParam;
            u16 sourceChannelNum = app_get_one_in_data(appCtrl.aclParam[i].source.location);
            if(sourceChannelNum>1)
            {
                for(u8 j=0;j<sourceChannelNum;j++)
                {
                    int lc3Ret = lc3enc_encode_init_bap(j,codecSettings[paramIndex].frequencyValue,
                                                           codecSettings[paramIndex].durationValue,
                                                           codecSettings[paramIndex].frameOctets);
                    if(lc3Ret != LC3DEC_OK)
                    {
                        tlkapi_printf(APP_LOG_EN,"lc3  encode init fail:0x%x", lc3Ret);
                        return;
                    }
                    tlkapi_printf(APP_LOG_EN,"j %d", j);
                    tlkapi_printf(APP_LOG_EN,"lc3 encode init success:0x%x", j);
                    tlkapi_printf(APP_LOG_EN,"codecSettings[paramIndex].frequencyValue %d", codecSettings[paramIndex].frequencyValue);
                    tlkapi_printf(APP_LOG_EN,"codecSettings[paramIndex].durationValue  %d", codecSettings[paramIndex].durationValue);
                    tlkapi_printf(APP_LOG_EN,"codecSettings[paramIndex].frameOctets    %d", codecSettings[paramIndex].frameOctets);
                }
            }
            else
            {
                int lc3Ret = lc3enc_encode_init_bap(i,codecSettings[paramIndex].frequencyValue,
                                                       codecSettings[paramIndex].durationValue,
                                                       codecSettings[paramIndex].frameOctets);
                if(lc3Ret != LC3DEC_OK)
                {
                    tlkapi_printf(APP_LOG_EN,"lc3  encode init fail:0x%x", lc3Ret);
                    return;
                }
                tlkapi_printf(APP_LOG_EN,"i %d", i);
                tlkapi_printf(APP_LOG_EN,"lc3 encode init success:0x%x", i);
                tlkapi_printf(APP_LOG_EN,"codecSettings[paramIndex].frequencyValue %d", codecSettings[paramIndex].frequencyValue);
                tlkapi_printf(APP_LOG_EN,"codecSettings[paramIndex].durationValue  %d", codecSettings[paramIndex].durationValue);
                tlkapi_printf(APP_LOG_EN,"codecSettings[paramIndex].frameOctets    %d", codecSettings[paramIndex].frameOctets);
            }
            appCtrl.aclParam[i].source.codecOp = APP_CODEC_IDLE;
        }
    }
}
static void app_audio_receive_process(void)
{
#if(APP_AUDIO_SCENE == APP_SCENE_TWS)
    for(u8 i=0;i<appCtrl.acl_max_num;i++)
    {
        if(appCtrl.aclParam[i].sink.sS)
        {
            sdu_packet_t* pPkt = blc_bapuc_sduPacketPop(appCtrl.aclParam[i].acl_handle, 0);
            if(pPkt != NULL)
            {
                unsigned int detect = 0;
                if(pPkt->iso_sdu_len!=codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].frameOctets)
                {
                    if(!appCtrl.aclParam[i].sink.sT)
                    {
                        APP_DBG_CHN_13_HIGH;
                        APP_DBG_CHN_13_LOW;
                        return;
                    }
                    else
                    {
                        APP_DBG_CHN_14_HIGH;
                        APP_DBG_CHN_14_LOW;
                        detect = 1;
                    }
                }
                else
                {
                    APP_DBG_CHN_15_HIGH;
                    APP_DBG_CHN_15_LOW;
                    appCtrl.aclParam[i].sink.sT = clock_time()|1;
                }
                LC3DEC_Error ret_lc3 = lc3dec_set_parameter(i, LC3_PARA_BEC_DETECT, &detect);
                if(ret_lc3!=LC3DEC_OK)
                {
                    tlkapi_printf(APP_LOG_EN,"lc3 decode set parameter error:0x%x", ret_lc3);
                    return;
                }
                APP_DBG_CHN_3_HIGH;
                audio_pkt_t pRaw = {0};
                ret_lc3 = lc3dec_decode_pkt(i,pPkt->data,pPkt->iso_sdu_len,(u8*)pRaw.buffer);
                if(ret_lc3!=LC3DEC_OK)
                {
                    tlkapi_printf(APP_LOG_EN,"lc3 decode error:0x%x", ret_lc3);
                    return;
                }
                else
                {
                    pRaw.renderPoint = pPkt->timestamp+appCtrl.aclParam[i].sink.pD*SYSTEM_TIMER_TICK_1US\
                    +AUDIO_UNICAST_CLIENT_MAX_TRANSPORT_LATENCY*SYSTEM_TIMER_TICK_1MS;//tick
                    app_list_add(&pRaw,i);
                }
                APP_DBG_CHN_3_LOW;
            }
        }
    }
#elif(APP_AUDIO_SCENE == APP_SCENE_HEADSET)
    if(appCtrl.aclParam[0].sink.sS)
    {
        sdu_packet_t* pPkt = blc_bapuc_sduPacketPop(appCtrl.aclParam[0].acl_handle, 0);
        if(pPkt != NULL)
        {
            unsigned int detect = 0;
            if(pPkt->iso_sdu_len!=codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].frameOctets)
            {
                if(!appCtrl.aclParam[0].sink.sT)
                {
                    APP_DBG_CHN_13_HIGH;
                    APP_DBG_CHN_13_LOW;
                    return;
                }
                else
                {
                    APP_DBG_CHN_14_HIGH;
                    APP_DBG_CHN_14_LOW;
                    detect = 1;
                }
            }
            else
            {
                APP_DBG_CHN_15_HIGH;
                APP_DBG_CHN_15_LOW;
                appCtrl.aclParam[0].sink.sT = clock_time()|1;
            }
            LC3DEC_Error ret_lc3 = lc3dec_set_parameter(0, LC3_PARA_BEC_DETECT, &detect);
            if(ret_lc3!=LC3DEC_OK)
            {
                tlkapi_printf(APP_LOG_EN,"lc3 decode set parameter error:0x%x", ret_lc3);
                return;
            }
            APP_DBG_CHN_3_HIGH;
            audio_pkt_t pRaw = {0};
            ret_lc3 = lc3dec_decode_pkt(0,pPkt->data,pPkt->iso_sdu_len,(u8*)pRaw.buffer);
            if(ret_lc3!=LC3DEC_OK)
            {
                tlkapi_printf(APP_LOG_EN,"lc3 decode error:0x%x", ret_lc3);
                return;
            }
            else
            {
                pRaw.renderPoint = pPkt->timestamp+appCtrl.aclParam[0].sink.pD*SYSTEM_TIMER_TICK_1US\
                +AUDIO_UNICAST_CLIENT_MAX_TRANSPORT_LATENCY*SYSTEM_TIMER_TICK_1MS;//tick
                app_list_add(&pRaw);
            }
            APP_DBG_CHN_3_LOW;
        }
    }
#endif
}
static void app_audio_send_process(void)
{
#if(APP_AUDIO_SCENE == APP_SCENE_TWS)
    u16 pcmData[2*APP_AUDIO_INPUT_FRAME_SAMPLE_MAX]={0};
    if(tlk_buffer_read((u8*)pcmData,usbI.cC*usbI.fOctets,TLK_BUFFER_1) == TLK_BUFFER_SUCCESS)
    {
        blc_usb_adjust_volume(pcmData,usbI.cC*usbI.fSample);
        u16 audio_pcm[APP_AUDIO_INPUT_FRAME_SAMPLE_MAX]={0};
        u8 audio_enc[2][APP_AUDIO_INPUT_FRAME_ENCODE_BYTES_MAX]={0};
        for(u8 i=0;i<appCtrl.acl_max_num;i++)
        {
            APP_DBG_CHN_1_HIGH;
            if(appCtrl.aclParam[i].source.sS)
            {
                for(u16 j=0;j<usbI.fSample;j++)
                {
                    audio_pcm[j] = pcmData[2*j+i];
                }
                LC3ENC_Error ret_lc3 = lc3enc_encode_pkt(i,(u8*)audio_pcm,audio_enc[i]);
                if(ret_lc3!=LC3ENC_OK)
                {
                    tlkapi_printf(APP_LOG_EN,"lc3 encode error:0x%x", ret_lc3);
                }
            }
            APP_DBG_CHN_1_LOW;
        }
        for(u8 i=0;i<appCtrl.acl_max_num;i++)
        {
            if(appCtrl.aclParam[i].source.sS)
            {
                APP_DBG_CHN_2_HIGH;
                int ret = blc_bapuc_sduPacketPush(appCtrl.aclParam[i].acl_handle, 0, audio_enc[i], codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].frameOctets);
                if(ret != AUDIO_ESUCC)
                {
                    tlkapi_printf(APP_LOG_EN,"sdu send fail-ret:0x%x", ret);
                }
                APP_DBG_CHN_2_LOW;
            }
        }
    }
#elif(APP_AUDIO_SCENE == APP_SCENE_HEADSET)
    u16 pcmData[2*APP_AUDIO_INPUT_FRAME_SAMPLE_MAX]={0};
    if(tlk_buffer_read((u8*)pcmData,usbI.cC*usbI.fOctets,TLK_BUFFER_1) == TLK_BUFFER_SUCCESS)
    {
        if(clock_time_exceed(appCtrl.aclParam[0].source.sT, 30 * 1000))//To make audio data consistent,if 30ms not read data,reinitialize lc3 encode.
        {
            tlk_buffer_clear(TLK_BUFFER_1);
            appCtrl.aclParam[0].source.codecOp  = APP_CONFIG_CODEC;
            appCtrl.aclParam[0].source.sT = clock_time()|1;
            tlkapi_printf(APP_LOG_EN,"source time exceed,clear buffer and init lc3");
            return;
        }
        appCtrl.aclParam[0].source.sT = clock_time()|1;
        u16 audio_pcm[APP_AUDIO_INPUT_FRAME_SAMPLE_MAX]={0};
        u8 audio_enc[2*APP_AUDIO_INPUT_FRAME_ENCODE_BYTES_MAX]={0};

        blc_usb_adjust_volume(pcmData,usbI.cC*usbI.fSample);
        u8 sourceChannelNum = app_get_one_in_data(appCtrl.aclParam[0].source.location);
        if(appCtrl.aclParam[0].source.sS)
        {
            for(u8 j=0;j<sourceChannelNum;j++)
            {
                APP_DBG_CHN_1_HIGH;
                for(u16 t=0;t<usbI.fSample;t++)
                {
                    audio_pcm[t] = pcmData[2*t+j];
                }
                LC3ENC_Error ret_lc3 = lc3enc_encode_pkt(j,(u8*)audio_pcm,audio_enc+j*codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].frameOctets);
                if(ret_lc3!=LC3ENC_OK)
                {
                    tlkapi_printf(APP_LOG_EN,"lc3 encode error:0x%x", ret_lc3);
                }
                APP_DBG_CHN_1_LOW;
            }
        }

        if(appCtrl.aclParam[0].source.sS)
        {
            APP_DBG_CHN_2_HIGH;
            int ret = blc_bapuc_sduPacketPush(appCtrl.aclParam[0].acl_handle, 0, audio_enc, sourceChannelNum*codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].frameOctets);
            if(ret != AUDIO_ESUCC)
            {
                tlkapi_printf(APP_LOG_EN,"sdu send fail-ret:0x%x", ret);
            }
            APP_DBG_CHN_2_LOW;
        }
    }
#endif
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
    u32 irq = irq_disable();
    u32 ret = tlk_buffer_write((u8*)pStart.next->buffer,usbO.cC*usbO.fOctets,TLK_BUFFER_2);
    irq_restore(irq);
    if(ret!=TLK_BUFFER_SUCCESS)
    {
        tlkapi_send_string_data(1, "write error", (u8*)&ret, 4);
    }

    if(app_list_delete_node(pStart.next)==0)
    {
        tlkapi_printf(APP_LOG_EN,"memory free failed-start");
    }
    if(pStart.next!=NULL)
    {
        u32 capture_tick_stimer = pStart.next->renderPoint-clock_time();
        u32 capture_tick_timer  = (capture_tick_stimer*sys_clk.pclk)/SYSTEM_TIMER_TICK_1US ;
        ble_audio_timer_set_capture(TIMER0,0,capture_tick_timer);
    }
    APP_DBG_CHN_4_LOW;
}

/**
 * @brief     This function servers to set USB Input.
 * @param[in] none
 * @return    none.
 */
_attribute_ram_code_ void  app_usb_irq_proc (void)
{
    APP_DBG_CHN_9_HIGH;
    if (usbhw_get_eps_irq()&FLD_USB_EDP6_IRQ)
    {
        APP_DBG_CHN_5_HIGH;
        ///////////// output to audio fifo out ////////////////
        u8 usbData[256];
        unsigned char len = reg_usb_ep6_ptr;
        usbhw_reset_ep_ptr(USB_EDP_SPEAKER);
        for (unsigned int i=0; i<len; i++)
        {
            usbData[i] = reg_usb_ep6_dat;
        }
        tlk_buffer_write(usbData,len,TLK_BUFFER_1);
        usbhw_data_ep_ack(USB_EDP_SPEAKER);
        usbhw_clr_eps_irq(FLD_USB_EDP6_IRQ);
        APP_DBG_CHN_5_LOW;
    }
    if (usbhw_get_eps_irq()& FLD_USB_EDP7_IRQ)
    {

         /////// get MIC input data ///////////////////////////////
         usbhw_reset_ep_ptr(USB_EDP_MIC);
         u8 usbData[64];
         for(u8 i=0;i<2*(MIC_SAMPLE_RATE/1000)*MIC_CHANNEL_COUNT;i++)
         {
             usbData[i] = 0;
         }
         if(clock_time_exceed(usbI.tick, 5 * 1000))
         {
             APP_DBG_CHN_8_HIGH;
             tlk_buffer_clear(TLK_BUFFER_2);
             APP_DBG_CHN_8_LOW;
         }
         u32 ret = tlk_buffer_read(usbData,2*(MIC_SAMPLE_RATE/1000)*MIC_CHANNEL_COUNT,TLK_BUFFER_2);
         if(ret!=TLK_BUFFER_SUCCESS)
         {
             APP_DBG_CHN_7_HIGH;
             APP_DBG_CHN_7_LOW;
         }
         APP_DBG_CHN_6_HIGH;
         for(u16 i=0;i<2*(MIC_SAMPLE_RATE/1000)*MIC_CHANNEL_COUNT;i++)
         {
             reg_usb_ep7_dat = usbData[i];
         }
         APP_DBG_CHN_6_LOW;
         usbI.tick = clock_time()|1;
         usbhw_data_ep_ack(USB_EDP_MIC);
         usbhw_clr_eps_irq(FLD_USB_EDP7_IRQ);
    }
    APP_DBG_CHN_9_LOW;
}

/**
 * @brief      Free all node in list.
 * @param[in]  none.
 * @return     none.
 */
_attribute_ram_code_ void app_list_free(void)
{
    while(pStart.next!=NULL)
    {
        app_list_delete_node(pStart.next);
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
                memset((u8*)&pDel->renderPoint,0,sizeof(struct list_node_t));
                TLK_MEM_STATE_T memRet = tlk_mem_free(pDel);
                if(memRet!=TLK_MEM_SUCCESS)
                {
                    tlkapi_printf(APP_LOG_EN,"mempool free failed,pos1!");
                }
                return 1;
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
                }
                return 1;
            }
        }
        pTemp = pTemp->next;
    }
    return 0;
}

#if(APP_AUDIO_SCENE == APP_SCENE_TWS)
_attribute_ram_code_ static void app_list_add(audio_pkt_t *pData,u8 channel)
{
    if(((unsigned int)(pData->renderPoint - clock_time()))> 200*SYSTEM_TIMER_TICK_1MS)
    {
        return;
    }
    u32 irq = irq_disable();
    if(pStart.next==NULL)
    {
        APP_DBG_CHN_10_HIGH;
        struct list_node_t *pNew = (struct list_node_t*)tlk_mem_malloc(sizeof(struct list_node_t));
        if(pNew == NULL)
        {
            tlkapi_printf(APP_LOG_EN,"mempool malloc failed,pos1!");
            while(1)
            {
                ////////////////////////////////////// Debug entry /////////////////////////////////
                #if (TLKAPI_DEBUG_ENABLE)
                    tlkapi_debug_handler();
                #endif
            }
        }
        pNew->renderPoint = pData->renderPoint;
        for(int i=0;i<usbO.fSample;i++)
        {
            pNew->buffer[2*i+channel] = pData->buffer[i];
        }
        pStart.next = pNew;
        pNew->next = NULL;
        irq_restore(irq);
        u32 capture_tick_stimer = pData->renderPoint-clock_time();
        u32 capture_tick_timer  = (capture_tick_stimer*sys_clk.pclk)/SYSTEM_TIMER_TICK_1US ;
        ble_audio_timer_set_capture(TIMER0,0,capture_tick_timer);
        APP_DBG_CHN_10_LOW;
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
                APP_DBG_CHN_12_HIGH;
                for(int i=0;i<usbO.fSample;i++)
                {
                    pTemp->buffer[2*i+channel] = pData->buffer[i];
                }
                irq_restore(irq);
                APP_DBG_CHN_12_LOW;
                return;
            }
        }
        APP_DBG_CHN_11_HIGH;
        struct list_node_t *pNew = (struct list_node_t*)tlk_mem_malloc(sizeof(struct list_node_t));
        if(pNew == NULL)
        {
            tlkapi_printf(APP_LOG_EN,"mempool malloc failed,pos2!");
            while(1)
            {
                ////////////////////////////////////// Debug entry /////////////////////////////////
                #if (TLKAPI_DEBUG_ENABLE)
                    tlkapi_debug_handler();
                #endif
            }
        }
        pNew->renderPoint = pData->renderPoint;
        for(int i=0;i<usbO.fSample;i++)
        {
            pNew->buffer[2*i+channel] = pData->buffer[i];
        }
        pTemp->next = pNew;
        pNew->next = NULL;
        irq_restore(irq);
        APP_DBG_CHN_11_LOW;
    }
}


#elif(APP_AUDIO_SCENE == APP_SCENE_HEADSET)

_attribute_ram_code_ void app_list_add(audio_pkt_t *pData)//audio data list add
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
    memcpy((u8*)pNew->buffer,(u8*)pData->buffer,usbO.fOctets);
    pTemp->next = pNew;
    pNew->next = NULL;
    irq_restore(irq);
}
#endif

#endif
