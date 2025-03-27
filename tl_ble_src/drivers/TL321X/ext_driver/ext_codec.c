/********************************************************************************************************
 * @file    ext_codec.c
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
#include"ext_codec.h"
#define TLK_CODEC_OUTPUT_DMA                DMA3
#define TLK_CODEC_INPUT_DMA                 DMA2

u32 gInputBufferLen    = 0;
u8* gInputBuffer       = NULL;
u32 gInputReadOffset   = 0;

u32 gOutputBufferLen   = 0;
u8* gOutputBuffer      = NULL;
u32 gOutputWriteOffset = 0;

#include "../../../algorithm/audio_alg/alg_audio_cfg.h"

audio_codec_stream0_input_t audio_codec_input =
{
    .input_src = AMIC_STREAM0_STEREO,
    .sample_rate = AUDIO_16K,
    .fifo_num = FIFO0,
    .data_width = CODEC_BIT_16_DATA,
    .dma_num = TLK_CODEC_INPUT_DMA,
    .data_buf = NULL,
    .data_buf_size = 0,
};

audio_codec_output_t audio_codec_output =
{
    .output_src = CODEC_DAC_STEREO,
    .sample_rate = AUDIO_48K,
    .fifo_num = FIFO0,
    .data_width = CODEC_BIT_16_DATA,
    .dma_num = TLK_CODEC_OUTPUT_DMA,
    .mode = HP_MODE,
    .data_buf = NULL,
    .data_buf_size = 0,
};
void codec_base_init(void)//le audio support 16bit default
{
    audio_codec_init();
}
void codec_close(void)
{
    audio_rx_dma_dis(audio_codec_input.dma_num);
    audio_tx_dma_dis(audio_codec_output.dma_num);
    audio_codec_dac_power_down();
    audio_codec_adc_power_down();
    gOutputBufferLen  = 0;
    gOutputBuffer     = NULL;
    gOutputWriteOffset= 0;

    gInputBufferLen   = 0;
    gInputBuffer      = NULL;
    gInputReadOffset  = 0;
}

void codec_config_input(tlk_codec_frequency_e freq,tlk_codec_channel_e chanC,tlk_codec_mode_e mode)
{
     if(mode == TLK_CODEC_MIC)
     {
         if(chanC == TLK_CODEC_1_CHANNEL)
         {
             audio_codec_input.input_src = AMIC_STREAM0_MONO_L;
         }
         else if(chanC == TLK_CODEC_2_CHANNEL)
         {
             audio_codec_input.input_src = AMIC_STREAM0_STEREO;
         }
     }
     else if(mode == TLK_CODEC_LINE)
     {
         if(chanC == TLK_CODEC_1_CHANNEL)
         {
             audio_codec_input.input_src = LINE_STREAM0_MONO_L;
         }
         else if(chanC == TLK_CODEC_2_CHANNEL)
         {
             audio_codec_input.input_src = LINE_STREAM0_STEREO;
         }
     }
     else if(mode == TLK_CODEC_I2S)
     {

     }
     audio_codec_input.sample_rate = freq+1;
     audio_codec_stream0_input_init(&audio_codec_input);
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
             audio_codec_output.output_src = CODEC_DAC_MONO_L;
         }
         else if(chanC == TLK_CODEC_2_CHANNEL)
         {
             audio_codec_output.output_src = CODEC_DAC_STEREO;
         }
         audio_codec_output.sample_rate = freq+1;
         audio_codec_stream_output_init(&audio_codec_output);
     }
}
void codec_input_enable(u8* pBuffer,u16 bufferLen)
{
    gInputBufferLen = bufferLen;
    gInputBuffer    = pBuffer;
    audio_codec_input.data_buf = pBuffer;
    audio_codec_input.data_buf_size = bufferLen;
    audio_rx_dma_chain_init(audio_codec_input.fifo_num,audio_codec_input.dma_num,(unsigned short*)audio_codec_input.data_buf,audio_codec_input.data_buf_size);
    audio_rx_dma_en(audio_codec_input.dma_num);
}
void codec_input_disable(void)
{
    audio_rx_dma_dis(audio_codec_input.dma_num);
    audio_codec_adc_power_down();
    gInputBufferLen    = 0;
    gInputBuffer       = NULL;
    gInputReadOffset   = 0;
}

void codec_output_enable(u8* pBuffer,u16 bufferLen)
{
    gOutputBufferLen = bufferLen;
    gOutputBuffer    = pBuffer;
    audio_codec_output.data_buf = pBuffer;
    audio_codec_output.data_buf_size = bufferLen;
    audio_tx_dma_chain_init(audio_codec_output.fifo_num,audio_codec_output.dma_num,(unsigned short*)audio_codec_output.data_buf,audio_codec_output.data_buf_size);
    audio_tx_dma_en(audio_codec_output.dma_num);
}


void codec_output_disable(void)
{
    memset(gOutputBuffer,0,gOutputBufferLen);
    audio_tx_dma_dis(audio_codec_output.dma_num);
}






/*****************************************codec input******************************************/
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
u16 codec_get_InputBuffMaxlen(void)
{
    return gInputBufferLen;
}
u32 codec_get_InputWriteOffset(void)
{
    u32 wptr = (u16)((audio_get_rx_dma_wptr(TLK_CODEC_INPUT_DMA))-((u32)gInputBuffer));
    return wptr;
}
u32 codec_get_InputReadOffset(void)
{
    return gInputReadOffset;
}
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
u16 codec_get_OutputBufferLen(void)
{
    return gOutputBufferLen;
}
int codec_output_getReadOffset(void)
{
    u32 readOffset = (audio_get_tx_dma_rptr(TLK_CODEC_OUTPUT_DMA))-((u32)gOutputBuffer);
    return readOffset;
}
int codec_output_getWriteOffset(void)
{
    return gOutputWriteOffset;
}

void codec_output_setWriteOffset(u32 offset)
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
int codec_output_writeData(u8* pData,u16 pDataLen)
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
