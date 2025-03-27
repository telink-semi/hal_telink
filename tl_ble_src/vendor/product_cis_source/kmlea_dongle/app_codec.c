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
#include "app.h"
#include "application/usbstd/usb.h"
#include "algorithm/audio_alg/lc3/lc3.h"
#include "vendor/common/tlk_api/tlk_buffer.h"

#if (PRODUCT_CIS_SOURCE_SELECT == PRODUCT_KMLEA_DONGLE)

extern app_ctrl_t appCtrl;
unsigned short app_audio_output_buffer[APP_AUDIO_OUTPUT_BUFFER_SIZE];//48k,10ms,
unsigned short app_audio_input_buffer[APP_AUDIO_INPUT_BUFFER_SIZE];//48k,10ms,

static void app_audio_receive_process(void);
static void app_audio_send_process(void);
static void app_codec_process(void);


void app_codec_init()
{
    blc_audio_usb_init();
    tlk_buffer_init((u8*)app_audio_input_buffer,2*APP_AUDIO_INPUT_BUFFER_SIZE,TLK_BUFFER_1);
    tlk_buffer_init((u8*)app_audio_output_buffer,2*APP_AUDIO_OUTPUT_BUFFER_SIZE,TLK_BUFFER_2);

    appCtrl.codecI.cC = 2;
    appCtrl.codecI.fSample = gLc3Index[codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].frequencyValue][codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].durationValue].frameSample;
    appCtrl.codecI.fOctets = 2*appCtrl.codecI.fSample;
    appCtrl.codecI.frameUs = gLc3Index[codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].frequencyValue][codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].durationValue].duraUs;

    appCtrl.codecO.cC = 1;
    appCtrl.codecO.fSample = gLc3Index[codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].frequencyValue][codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].durationValue].frameSample;
    appCtrl.codecO.fOctets = 2*appCtrl.codecO.fSample;
    appCtrl.codecO.frameUs = gLc3Index[codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].frequencyValue][codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].durationValue].duraUs;
    tlkapi_printf(APP_LOG_EN,"appCtrl.codecI.fSample %d",appCtrl.codecI.fSample);
    tlkapi_printf(APP_LOG_EN,"appCtrl.codecI.fOctets %d",appCtrl.codecI.fOctets);
    tlkapi_printf(APP_LOG_EN,"appCtrl.codecI.frameUs %d",appCtrl.codecI.frameUs);

    tlkapi_printf(APP_LOG_EN,"appCtrl.codecO.fSample %d",appCtrl.codecO.fSample);
    tlkapi_printf(APP_LOG_EN,"appCtrl.codecO.fOctets %d",appCtrl.codecO.fOctets);
    tlkapi_printf(APP_LOG_EN,"appCtrl.codecO.frameUs %d",appCtrl.codecO.frameUs);
}

void app_codec_handler()
{
    app_audio_receive_process();
    app_audio_send_process();
    app_codec_process();
    usb_handle_irq();
}

static void app_codec_process(void)//config lc3
{
    if(appCtrl.aclParam.sink.codecOp == APP_CONFIG_CODEC)
    {
        APP_DBG_CHN_14_HIGH;
        u8 paramIndex = appCtrl.aclParam.sink.codecParam;
        if(appCtrl.aclParam.sink.blocks>1)
        {
            for(u8 j=0;j<appCtrl.aclParam.sink.blocks;j++)
            {
                int lc3Ret = lc3dec_decode_init_bap(j,codecSettings[paramIndex].frequencyValue,\
                                                       codecSettings[paramIndex].durationValue,
                                                       codecSettings[paramIndex].frameOctets);
                if(lc3Ret != LC3DEC_OK)
                {
                    tlkapi_printf(APP_LOG_EN,"lc3 decode init fail:0x%x", lc3Ret);
                    return;
                }
                tlkapi_printf(APP_LOG_EN,"j %d", j);
                tlkapi_printf(APP_LOG_EN,"lc3 decode init success:0x%x", j);
                tlkapi_printf(APP_LOG_EN,"codecSettings[paramIndex].frequencyValue %d", codecSettings[paramIndex].frequencyValue);
                tlkapi_printf(APP_LOG_EN,"codecSettings[paramIndex].durationValue  %d", codecSettings[paramIndex].durationValue);
                tlkapi_printf(APP_LOG_EN,"codecSettings[paramIndex].frameOctets    %d", codecSettings[paramIndex].frameOctets);
            }
        }
        else
        {
            int lc3Ret = lc3dec_decode_init_bap(0,codecSettings[paramIndex].frequencyValue,\
                                                   codecSettings[paramIndex].durationValue,
                                                   codecSettings[paramIndex].frameOctets);
            if(lc3Ret != LC3DEC_OK)
            {
                tlkapi_printf(APP_LOG_EN,"lc3 decode init fail:0x%x", lc3Ret);
                return;
            }
            tlkapi_printf(APP_LOG_EN,"codecSettings[paramIndex].frequencyValue %d", codecSettings[paramIndex].frequencyValue);
            tlkapi_printf(APP_LOG_EN,"codecSettings[paramIndex].durationValue  %d", codecSettings[paramIndex].durationValue);
            tlkapi_printf(APP_LOG_EN,"codecSettings[paramIndex].frameOctets    %d", codecSettings[paramIndex].frameOctets);
        }
        appCtrl.aclParam.sink.codecOp = APP_CODEC_IDLE;
        APP_DBG_CHN_14_LOW;
    }
    if(appCtrl.aclParam.source.codecOp == APP_CONFIG_CODEC)
    {
        APP_DBG_CHN_15_HIGH;
        u8 paramIndex = appCtrl.aclParam.source.codecParam;
        if(appCtrl.aclParam.source.blocks>1)
        {
            for(u8 j=0;j<appCtrl.aclParam.source.blocks;j++)
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
            int lc3Ret = lc3enc_encode_init_bap(0,codecSettings[paramIndex].frequencyValue,
                                                   codecSettings[paramIndex].durationValue,
                                                   codecSettings[paramIndex].frameOctets);
            if(lc3Ret != LC3DEC_OK)
            {
                tlkapi_printf(APP_LOG_EN,"lc3  encode init fail:0x%x", lc3Ret);
                return;
            }
            tlkapi_printf(APP_LOG_EN,"codecSettings[paramIndex].frequencyValue %d", codecSettings[paramIndex].frequencyValue);
            tlkapi_printf(APP_LOG_EN,"codecSettings[paramIndex].durationValue  %d", codecSettings[paramIndex].durationValue);
            tlkapi_printf(APP_LOG_EN,"codecSettings[paramIndex].frameOctets    %d", codecSettings[paramIndex].frameOctets);
        }
        appCtrl.aclParam.source.codecOp = APP_CODEC_IDLE;
        APP_DBG_CHN_15_LOW;
    }
}

static void app_audio_receive_process(void)
{
    if(appCtrl.aclParam.sink.sS)
    {
        sdu_packet_t* pPkt = blc_bapuc_sduPacketPop(appCtrl.aclParam.acl_handle, 0);
        if(pPkt != NULL)
        {
            unsigned int detect = 0;
            if(pPkt->iso_sdu_len!=codecSettings[APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER].frameOctets)
            {
                if(!appCtrl.aclParam.sink.wT)
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
                appCtrl.aclParam.sink.wT = clock_time()|1;
            }
            LC3DEC_Error ret_lc3 = lc3dec_set_parameter(0, LC3_PARA_BEC_DETECT, &detect);
            if(ret_lc3!=LC3DEC_OK)
            {
                tlkapi_printf(APP_LOG_EN,"lc3 decode set parameter error:0x%x", ret_lc3);
                return;
            }
            APP_DBG_CHN_3_HIGH;
            u8 buffer[2*APP_AUDIO_OUTPUT_FRAME_SAMPLE_MAX];
            ret_lc3 = lc3dec_decode_pkt(0,pPkt->data,pPkt->iso_sdu_len,(u8*)buffer);
            if(ret_lc3!=LC3DEC_OK)
            {
                tlkapi_printf(APP_LOG_EN,"lc3 decode error:0x%x", ret_lc3);
                return;
            }
            else
            {
                 static bool bufferClearIdx = 0;
                 if(!clock_time_exceed(appCtrl.aclParam.sink.rT, 30 * 1000))//50ms not read data,clear buffer.
                 {
                     tlk_buffer_write(buffer,2*APP_AUDIO_OUTPUT_FRAME_SAMPLE_MAX,TLK_BUFFER_2);
                     bufferClearIdx = true;
                 }
                 else
                 {
                     if(bufferClearIdx)
                     {
                         tlk_buffer_clear(TLK_BUFFER_2);
                         bufferClearIdx = false;
                     }
                 }
            }
            APP_DBG_CHN_3_LOW;
        }
    }
}
static void app_audio_send_process(void)
{
    u16 pcmData[2*APP_AUDIO_INPUT_FRAME_SAMPLE_MAX]={0};
    if(tlk_buffer_read((u8*)pcmData,appCtrl.codecI.cC*appCtrl.codecI.fOctets,TLK_BUFFER_1) == TLK_BUFFER_SUCCESS)
    {
        if(clock_time_exceed(appCtrl.aclParam.source.rT, 30 * 1000))//50ms not read data,init lc3.
        {
            tlk_buffer_clear(TLK_BUFFER_1);
            appCtrl.aclParam.source.codecOp  = APP_CONFIG_CODEC;
            appCtrl.aclParam.source.rT = clock_time()|1;
            tlkapi_printf(APP_LOG_EN,"source time exceed,clear buffer and init lc3");
            return;
        }
        appCtrl.aclParam.source.rT = clock_time()|1;
        u16 audio_pcm[APP_AUDIO_INPUT_FRAME_SAMPLE_MAX]={0};
        u8 audio_enc[2*APP_AUDIO_INPUT_FRAME_ENCODE_BYTES_MAX]={0};
        APP_DBG_CHN_1_HIGH;
        if(appCtrl.aclParam.source.sS)
        {
            for(u8 j=0;j<appCtrl.aclParam.source.blocks;j++)
            {
                for(u16 t=0;t<appCtrl.codecI.fSample;t++)
                {
                    audio_pcm[t] = pcmData[2*t+j];
                }
                LC3ENC_Error ret_lc3 = lc3enc_encode_pkt(j,(u8*)audio_pcm,audio_enc+j*codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].frameOctets);
                if(ret_lc3!=LC3ENC_OK)
                {
                    tlkapi_printf(APP_LOG_EN,"lc3 encode error:0x%x", ret_lc3);
                }
            }
        }
        APP_DBG_CHN_1_LOW;
        if(appCtrl.aclParam.source.sS)
        {
            APP_DBG_CHN_2_HIGH;
            int ret = blc_bapuc_sduPacketPush(appCtrl.aclParam.acl_handle, 0, audio_enc, appCtrl.aclParam.source.blocks*codecSettings[APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER].frameOctets);
            if(ret != AUDIO_ESUCC)
            {
                tlkapi_printf(APP_LOG_EN,"sdu send fail-ret:0x%x", ret);
            }
            APP_DBG_CHN_2_LOW;
        }
    }
}

/**
 * @brief     This function servers to set USB Input.
 * @param[in] none
 * @return    none.
 */
_attribute_ram_code_ void  app_usb_irq_proc (void)
{
    APP_DBG_CHN_8_HIGH;
    if (usbhw_get_eps_irq()&FLD_USB_EDP6_IRQ)
    {
        APP_DBG_CHN_6_HIGH;
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
        APP_DBG_CHN_6_LOW;
    }
    if (usbhw_get_eps_irq()& FLD_USB_EDP7_IRQ)
    {
         APP_DBG_CHN_7_HIGH;
         /////// get MIC input data ///////////////////////////////
         usbhw_reset_ep_ptr(USB_EDP_MIC);
         u8 usbData[32];
         for(u8 i=0;i<32;i++)
         {
             usbData[i] = 0;
         }

         tlk_buffer_read(usbData,32,TLK_BUFFER_2);
         for(u16 i=0;i<32;i++)
         {
             reg_usb_ep7_dat = usbData[i];
         }
         appCtrl.aclParam.sink.rT = clock_time()|1;
         usbhw_data_ep_ack(USB_EDP_MIC);
         usbhw_clr_eps_irq(FLD_USB_EDP7_IRQ);
         APP_DBG_CHN_7_LOW;
    }
    APP_DBG_CHN_8_LOW;
}
#endif //end of (PRODUCT_CIS_SOURCE_SELECT == ...)
