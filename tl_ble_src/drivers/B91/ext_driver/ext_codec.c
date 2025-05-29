/********************************************************************************************************
 * @file    ext_codec.c
 *
 * @brief   This is the header file for BLE SDK
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
#include "../driver.h"
#include "ext_codec.h"
#include "../../../algorithm/audio_alg/alg_audio_cfg.h"

#define TLK_CODEC_OUTPUT_DMA                DMA3
#define TLK_CODEC_INPUT_DMA                 DMA2

u16 gInputBufferLen    = 0;
u8* gInputBuffer       = NULL;
u32 gInputReadOffset   = 0;

u16 gOutputBufferLen   = 0;
u8* gOutputBuffer      = NULL;
u32 gOutputWriteOffset = 0;

extern audio_i2s_codec_config_t audio_i2s_codec_config;
extern audio_i2s_invert_config_t audio_i2s_invert_config;
const u8 tlk_codec_index[] =
{
        0,
        8,
        11.25,
        16,
        22.05,
        24,
        32,
        44.1,
        48,
        88.2,
        96,
        176.4,
        192
};

void codec_base_init(void)//le audio support 16bit default
{
    reg_rst2&=(~FLD_RST2_AUD);
    reg_rst2 |=FLD_RST2_AUD;//reset audio

    reg_rst3&=(~FLD_RST3_CODEC);
    reg_rst3 |=FLD_RST3_CODEC;//reset codec

    audio_set_codec_clk(1,16);//from ppl 192/16=12M
    audio_i2s_codec_config.i2s_data_select=I2S_BIT_16_DATA;
    audio_i2s_codec_config.codec_data_select=CODEC_BIT_16_DATA;
    reg_audio_ctrl |= CODEC_I2S;
    audio_i2s_config(I2S_I2S_MODE,audio_i2s_codec_config.i2s_data_select,audio_i2s_codec_config.i2s_codec_m_s_mode,&audio_i2s_invert_config);

    audio_set_i2s_clk(2,125);
    audio_set_i2s_bclk(0);
    audio_clk_en(1,1);
    reg_audio_codec_vic_ctr=FLD_AUDIO_CODEC_SLEEP_ANALOG;//active analog sleep mode//pass
    while(!(reg_audio_codec_stat_ctr&FLD_AUDIO_CODEC_PON_ACK));//wait codec can be configured
}


void codec_close(void)
{
    audio_rx_dma_dis();
    audio_tx_dma_dis();
    audio_codec_dac_power_down();
    audio_codec_adc_power_down();
    gOutputBufferLen  = 0;
    gOutputBuffer     = NULL;
    gOutputWriteOffset= 0;

    gInputBufferLen   = 0;
    gInputBuffer      = NULL;
    gInputReadOffset  = 0;
}

void iis_base_init(u8 *Ibuffer,u16 IbufferLen,u8 *Obuffer,u16 ObufferLen)//le audio support 16bit default
{
    gpio_input_en(I2S_BCK_PC3|I2S_ADC_LR_PC6|I2S_ADC_DAT_PC7|I2S_DAC_LR_PC4|I2S_DAC_DAT_PC5);
    reg_rst2&=(~FLD_RST2_AUD);
    reg_rst2 |=FLD_RST2_AUD;//reset audio

    reg_rst3&=(~FLD_RST3_CODEC);
    reg_rst3 |=FLD_RST3_CODEC;//reset codec

    audio_tx_dma_dis();

    audio_rx_dma_dis();

    audio_set_codec_supply(CODEC_2P8V);
    gInputBufferLen = IbufferLen;
    gInputBuffer    = Ibuffer;
    gOutputBufferLen = ObufferLen;
    gOutputBuffer    = Obuffer;
    audio_tx_dma_chain_init (DMA3, (u16*)Obuffer, ObufferLen);

    audio_rx_dma_chain_init(DMA2,(u16*)Ibuffer, IbufferLen);


    extern void audio_i2s_set_pin(void);
    audio_i2s_set_pin();
    extern void audio_set_chn_wl(audio_channel_wl_mode_e chn_wl);
    audio_set_chn_wl(STEREO_BIT_16);
    audio_mux_config(IO_I2S,audio_i2s_codec_config.audio_in_mode,audio_i2s_codec_config.audio_in_mode,audio_i2s_codec_config.audio_out_mode);

    audio_i2s_codec_config.i2s_data_select=I2S_BIT_16_DATA;
    audio_i2s_codec_config.codec_data_select=CODEC_BIT_16_DATA;
    audio_i2s_codec_config.i2s_codec_m_s_mode = I2S_S_CODEC_M;
    audio_i2s_config(I2S_I2S_MODE,audio_i2s_codec_config.i2s_data_select,audio_i2s_codec_config.i2s_codec_m_s_mode,&audio_i2s_invert_config);

    audio_set_lrclk(64,64);
    audio_set_i2s_clk(2,125);
    audio_set_i2s_bclk(0);
    audio_clk_en(1,1);

    audio_data_fifo0_path_sel(I2S_DATA_IN_FIFO,I2S_OUT);

}

void codec_config_input(tlk_codec_frequency_e freq,tlk_codec_channel_e chanC,tlk_codec_mode_e mode)
{
    if(mode == TLK_CODEC_I2S)
    {

    }
    else
    {
        if(chanC == TLK_CODEC_1_CHANNEL)
        {
            audio_i2s_codec_config.audio_in_mode=BIT_16_MONO;
        }
        else if(chanC == TLK_CODEC_2_CHANNEL)
        {
            audio_i2s_codec_config.audio_in_mode=BIT_16_STEREO;
        }

        reg_audio_tune = ((reg_audio_tune&(~(FLD_AUDIO_I2S_I2S_AIN0_COME|FLD_AUDIO_I2S_I2S_AIN1_COME)))|MASK_VAL(FLD_AUDIO_I2S_I2S_AIN0_COME,audio_i2s_codec_config.audio_in_mode,\
                                                                                                                FLD_AUDIO_I2S_I2S_AIN1_COME,audio_i2s_codec_config.audio_in_mode));

        reg_int_pcm_num=((3072/tlk_codec_index[freq])-1);//reg_int_pcm_num=(adc_div-1);

        audio_codec_adc_config(audio_i2s_codec_config.i2s_codec_m_s_mode,mode,freq,audio_i2s_codec_config.codec_data_select,MCU_WREG);

        reg_audio_sel = (reg_audio_sel&=~FLD_AUDIO_AIN0_SEL)|MASK_VAL( FLD_AUDIO_AIN0_SEL, I2S_DATA_IN_FIFO);
    }
}


void codec_config_output(tlk_codec_frequency_e freq,tlk_codec_channel_e chanC,tlk_codec_mode_e mode)
{
    if(mode == TLK_CODEC_I2S)
    {

    }
    else
    {
        if(chanC == TLK_CODEC_1_CHANNEL)
        {
            audio_i2s_codec_config.audio_out_mode=BIT_16_MONO_FIFO0;
        }
        else if(chanC == TLK_CODEC_2_CHANNEL)
        {
            audio_i2s_codec_config.audio_out_mode=BIT_16_STEREO_FIFO0;
        }

        reg_audio_tune = (reg_audio_tune&=~FLD_AUDIO_I2S_I2S_AOUT_COME)|MASK_VAL(FLD_AUDIO_I2S_I2S_AOUT_COME,audio_i2s_codec_config.audio_out_mode);


        reg_dec_pcm_num=((3072/tlk_codec_index[freq])-1);// reg_dec_pcm_num=(dac_div-1);

        audio_codec_dac_config(audio_i2s_codec_config.i2s_codec_m_s_mode,freq,audio_i2s_codec_config.codec_data_select,MCU_WREG);

        reg_audio_sel = (reg_audio_sel&=~FLD_AUDIO_AOUT0_SEL)|MASK_VAL(FLD_AUDIO_AOUT0_SEL, I2S_OUT);
    }
}

/*****************************************codec input******************************************/
void codec_input_enable(u8* pBuffer,u16 bufferLen)
{
    gInputBufferLen = bufferLen;
    gInputBuffer    = pBuffer;
    dma_chn_dis(TLK_CODEC_INPUT_DMA);
    audio_rx_dma_chain_init(TLK_CODEC_INPUT_DMA,(u16*)gInputBuffer,gInputBufferLen);
}

void codec_input_disable(void)
{
    dma_chn_dis(TLK_CODEC_INPUT_DMA);
    gInputBufferLen    = 0;
    gInputBuffer       = NULL;
    gInputReadOffset   = 0;
}

int codec_input_getDataLen(void)
{
    u16 used;
    u32 wptr;
    u32 rptr;

    rptr = gInputReadOffset;
    wptr = (audio_get_rx_dma_wptr(TLK_CODEC_INPUT_DMA))-((u32)gInputBuffer);

    if(wptr >= rptr)
    {
        used = wptr-rptr;
    }
    else
    {
        used = gInputBufferLen+wptr-rptr;
    }
    return used;
}
_attribute_ram_code_
u16 codec_get_InputBuffMaxlen(void)
{
    return gInputBufferLen;
}
_attribute_ram_code_
u32 codec_get_InputWriteOffset(void)
{
    u32 wptr = (u16)((audio_get_rx_dma_wptr(TLK_CODEC_INPUT_DMA))-((u32)gInputBuffer));
    return wptr;
}
_attribute_ram_code_
u32 codec_get_InputReadOffset(void)
{
    return gInputReadOffset;
}
_attribute_ram_code_
void codec_set_InputReadOffset(u32 rptr)
{
    if(rptr>gInputBufferLen)
    {
        gInputReadOffset = rptr - gInputBufferLen;
    }
    else
    {
        gInputReadOffset = rptr;
    }
}
_attribute_ram_code_
int codec_input_readData1(u8* pData,u16 pDataLen,u32 wptr)
{
    u32 rptr;
    u32 micDataLen;
    u32 offset;

    rptr = gInputReadOffset;

    if(wptr >= rptr)
    {
        micDataLen = wptr-rptr;
    }
    else
    {
        micDataLen = gInputBufferLen+wptr-rptr;
    }

    if(micDataLen<pDataLen)
    {
        return false;//mic data not enough
    }

    if(pDataLen+rptr>=gInputBufferLen)
    {
        offset = gInputBufferLen-rptr;
        gInputReadOffset = pDataLen+rptr-gInputBufferLen;
    }
    else
    {
        offset = pDataLen;
        gInputReadOffset += pDataLen;
    }
    memcpy(pData, gInputBuffer+rptr, offset);
    if(offset < pDataLen)
    {
        memcpy(pData+offset, gInputBuffer, pDataLen-offset);
    }

    return true;
}
_attribute_ram_code_
int codec_input_readData(u8* pData,u16 pDataLen)
{
    u32 wptr;
    u32 rptr;
    u32 micDataLen;
    u32 offset;

    rptr = gInputReadOffset;
    wptr = (audio_get_rx_dma_wptr(TLK_CODEC_INPUT_DMA))-((u32)gInputBuffer);

    if(wptr >= rptr)
    {
        micDataLen = wptr-rptr;
    }
    else
    {
        micDataLen = gInputBufferLen+wptr-rptr;
    }

    if(micDataLen<pDataLen)
    {
        return false;//mic data not enough
    }

    if(pDataLen+rptr>=gInputBufferLen)
    {
        offset = gInputBufferLen-rptr;
        gInputReadOffset = pDataLen+rptr-gInputBufferLen;
    }
    else
    {
        offset = pDataLen;
        gInputReadOffset += pDataLen;
    }
    memcpy(pData, gInputBuffer+rptr, offset);
    if(offset < pDataLen)
    {
        memcpy(pData+offset, gInputBuffer, pDataLen-offset);
    }

    return true;
}

/*****************************************codec output******************************************/
void codec_output_enable(u8* pBuffer,u16 bufferLen)
{
    gOutputBufferLen = bufferLen;
    gOutputBuffer    = pBuffer;
    dma_chn_dis(TLK_CODEC_OUTPUT_DMA);
    audio_tx_dma_chain_init(TLK_CODEC_OUTPUT_DMA,(u16*)gOutputBuffer,gOutputBufferLen);
}

void codec_output_disable(void)
{
    memset(gOutputBuffer,0,gOutputBufferLen);
    dma_chn_dis(TLK_CODEC_OUTPUT_DMA);
}

u16 codec_get_OutputBufferLen(void)
{
    return gOutputBufferLen;
}
_attribute_ram_code_ int codec_output_getReadOffset(void)
{
    u32 readOffset = (audio_get_tx_dma_rptr(TLK_CODEC_OUTPUT_DMA))-((u32)gOutputBuffer);
    return readOffset;
}
_attribute_ram_code_ int codec_output_getWriteOffset(void)
{
    return gOutputWriteOffset;
}

_attribute_ram_code_ void codec_output_setWriteOffset(u32 offset)
{
    if(offset>gOutputBufferLen)
    {
        gOutputWriteOffset = offset - gOutputBufferLen;
    }
    else
    {
        gOutputWriteOffset = offset;
    }
}
_attribute_ram_code_ int codec_output_writeData(u8* pData,u16 pDataLen)
{
    u32 wptr;
    u16 offset;
    u8 *pBuffer = (u8*)gOutputBuffer;

    wptr = gOutputWriteOffset;

    if(wptr+pDataLen >  gOutputBufferLen)
    {
        offset = gOutputBufferLen-wptr;
    }
    else
    {
        offset = pDataLen;
    }
    memcpy(pBuffer+wptr, pData, offset);
    if(offset < pDataLen)
    {
        memcpy(pBuffer, pData+offset, pDataLen-offset);
    }

    wptr += pDataLen;
    if(wptr >= gOutputBufferLen)
    {
        wptr -= gOutputBufferLen;
    }
    gOutputWriteOffset = wptr;
    return true;
}

