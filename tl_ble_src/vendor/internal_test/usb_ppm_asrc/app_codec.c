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
#include "app_codec.h"
#include "app.h"
#include "application/usbstd/usb.h"
#include "drivers/B91/audio.h"

#if (INTER_TEST_MODE == TEST_USB_PPM_ASRC)


signed short app_usb_iso_in_buffer[APP_AUDIO_OUTPUT_BUFFER_SIZE+8];//48k,10ms,
signed short app_usb_iso_out_buffer[APP_AUDIO_INPUT_BUFFER_SIZE+8];//48k,10ms,


volatile signed short app_dmic_in_pingpong[DMIC_IN_PINGPONG_SIZE];//48K,16bit_mono,10ms = 48*2*10=960B,pingpong *2= unsigned short
volatile signed short app_usb_out_pingpong[USB_OUT_PINGPONG_SIZE];//48K,16bit_stereo,10ms


volatile u8* dmic_in_buffer_start_addr = NULL;


extern audio_i2s_codec_config_t audio_i2s_codec_config;
extern audio_i2s_invert_config_t audio_i2s_invert_config;

void app_audio_config(audio_flow_mode_e flow_mode,audio_sample_rate_e rate,audio_in_mode_e in_mode,audio_out_mode_e out_mode){

    audio_reset();
    codec_reset();
    //1. set mic and speaker sample resolution and channel num
    audio_i2s_codec_config.audio_in_mode=in_mode;//BIT_16_MONO;//mic mono--ISO_IN
    audio_i2s_codec_config.audio_out_mode=out_mode;//BIT_16_STEREO_FIFO0;//speak stereo---ISO_OUT
    audio_i2s_codec_config.i2s_data_select=I2S_BIT_16_DATA;
    audio_i2s_codec_config.codec_data_select=CODEC_BIT_16_DATA;

    //2.
    audio_set_codec_clk(1,16);//from ppl 192/16=12M
    audio_mux_config(CODEC_I2S,audio_i2s_codec_config.audio_in_mode,audio_i2s_codec_config.audio_in_mode,audio_i2s_codec_config.audio_out_mode);
    audio_i2s_config(I2S_I2S_MODE,audio_i2s_codec_config.i2s_data_select,audio_i2s_codec_config.i2s_codec_m_s_mode,&audio_i2s_invert_config);
    audio_set_i2s_clock(rate,AUDIO_RATE_EQUAL,0);
    audio_clk_en(1,1);
    reg_audio_codec_vic_ctr=FLD_AUDIO_CODEC_SLEEP_ANALOG;//active analog sleep mode
    while(!(reg_audio_codec_stat_ctr&FLD_AUDIO_CODEC_PON_ACK));//wait codec can be configured
    if(flow_mode<BUF_TO_LINE_OUT)
    {
        audio_codec_adc_config(audio_i2s_codec_config.i2s_codec_m_s_mode,(flow_mode%3),rate,audio_i2s_codec_config.codec_data_select,MCU_WREG);
    }

    if(flow_mode>LINE_IN_TO_BUF)
    {
        audio_codec_dac_config(audio_i2s_codec_config.i2s_codec_m_s_mode,rate,audio_i2s_codec_config.codec_data_select,MCU_WREG);
    }
    audio_data_fifo0_path_sel(I2S_DATA_IN_FIFO,I2S_OUT);
}
void app_codec_init()
{
    audio_set_dmic_pin(DMIC_GROUPD_D4_DAT_D5_D6_CLK);
    app_audio_config(DMIC_IN_TO_BUF_TO_LINE_OUT, APP_AUDIO_SAMPLE_RATE,APP_MIC_IN_MODE,APP_SPK_OUT_MODE);//audio_init(DMIC_IN_TO_BUF_TO_LINE_OUT, AUDIO_48K, STEREO_BIT_16);//MIC_SAMPLE_RATE

    audio_rx_dma_chain_init(BLC_CODEC_MIC_DMA,(unsigned short*)app_dmic_in_pingpong, DMIC_IN_PINGPONG_SIZE<<1);
    audio_tx_dma_chain_init(BLC_CODEC_SPK_DMA,(unsigned short*)app_usb_out_pingpong, USB_OUT_PINGPONG_SIZE<<1);

    //tlk_buffer_init((s8*)app_usb_iso_in_buffer, APP_AUDIO_OUTPUT_BUFFER_SIZE<<1, MIC_BUFF_IDX);
    //tlk_buffer_init((u8*)app_usb_iso_out_buffer, 2*APP_AUDIO_INPUT_BUFFER_SIZE, SPK_BUFF_IDX);
    //tlk_buffer_clear(MIC_BUFF_IDX);

    dmic_in_buffer_start_addr = app_dmic_in_pingpong;
}


_attribute_ram_code_ void app_timer_irq_proc0(void)
{
    timer_stop(TIMER0);
}

volatile u32 iso_in_irq_stick = 0;
volatile u8 iso_in_permiteFlag = 0;
/**
 * @brief     This function servers to set USB Input.
 * @param[in] none
 * @return    none.
 */
//APP_DBG_CHN_6_HIGH--chn2  ///APP_DBG_CHN_7_HIGH--chn3 ///APP_DBG_CHN_8_HIGH -- chn1
//APP_DBG_CHN_9_HIGH--chn4  ///APP_DBG_CHN_10_HIGH--chn5 ///APP_DBG_CHN_11_TOGGLE---chn6
_attribute_ram_code_ void  app_usb_irq_proc (void)
{
    APP_DBG_CHN_8_HIGH;

    ///////////////////ISO OUT ///////////////////
    if (usbhw_get_eps_irq()&FLD_USB_EDP6_IRQ)
    {
        APP_DBG_CHN_6_HIGH; //chn2
        static u32 IsoOutOffset = 0;
        ///////////// output to audio fifo out ////////////////
        u8 usbData[192];//256
        unsigned char len = reg_usb_ep6_ptr;

        usbhw_reset_ep_ptr(USB_EDP_SPEAKER);
        for (unsigned int i=0; i<len; i++)
        {
            usbData[i] = reg_usb_ep6_dat;
        }

        tlk_mem_cpy(app_usb_out_pingpong + IsoOutOffset, usbData, len);
        IsoOutOffset += (len>>1);
        IsoOutOffset %= USB_OUT_PINGPONG_SIZE;

        usbhw_data_ep_ack(USB_EDP_SPEAKER);
        usbhw_clr_eps_irq(FLD_USB_EDP6_IRQ);
        APP_DBG_CHN_6_LOW;
    }

    ///////////////////ISO IN ///////////////////

    if ( (usbhw_get_eps_irq()& FLD_USB_EDP7_IRQ) )
    {
         APP_DBG_CHN_7_HIGH;
         usbhw_clr_eps_irq(FLD_USB_EDP7_IRQ);

         iso_in_irq_stick = clock_time()|0x01;

         if(micDataRdyFlg){
             iso_in_permiteFlag = 1;
         }

         if(iso_in_permiteFlag){
             /////// get MIC input data ///////////////////////////////
             usbhw_reset_ep_ptr(USB_EDP_MIC);
             static u32 IsoInOffset = 0;
             s16* ps = (s16*)(app_dmic_in_pingpong+IsoInOffset);

             IsoInOffset += MIC_CHANNEL_COUNT*(MIC_SAMPLE_RATE/1000);

             IsoInOffset %= DMIC_IN_PINGPONG_SIZE;

             for(u16 i=0;i<(MIC_SAMPLE_RATE/1000);i++)
             {
                 reg_usb_ep7_dat = *ps;
                 reg_usb_ep7_dat = *ps++>>8;
            #if (MIC_CHANNEL_COUNT == 2)
                 reg_usb_ep7_dat = *ps;
                 reg_usb_ep7_dat = *ps++>>8;
            #endif

             }

             usbhw_data_ep_ack(USB_EDP_MIC);
         }

         APP_DBG_CHN_7_LOW;
    }

    APP_DBG_CHN_8_LOW;
}

#endif
