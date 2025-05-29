/********************************************************************************************************
 * @file    tlk_codec.c
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
#include "tlk_codec.h"

tlk_codec_desc_t tlkCodecI;
tlk_codec_desc_t tlkCodecO;

void tlk_codec_init(void)
{
    codec_base_init();
}

void tlk_codec_close(void)
{
    codec_close();
}

tlk_codec_sts_e tlk_codec_config(tlk_codec_e codec, tlk_codec_frequency_e freq, tlk_codec_channel_e chanC, tlk_codec_mode_e mode, u8 *buffer, u16 bufferLen)
{
    if (freq < TLK_CODEC_FREQ_16000 || freq > TLK_CODEC_FREQ_48000) {
        return TLK_CODEC_NON_SUPPORT;
    }
    if (chanC >= TLK_CODEC_CHANNEL_MAX || chanC < TLK_CODEC_1_CHANNEL) {
        return TLK_CODEC_NON_SUPPORT;
    }
    if (buffer == NULL || bufferLen < 512) {
        return TLK_CODEC_NON_SUPPORT;
    }
    if (codec == TLK_CODEC_INPUT) {
        tlkCodecI.pBuffer    = buffer;
        tlkCodecI.pBufferLen = bufferLen;
        tlkCodecI.cC         = chanC;
        tlkCodecI.freq       = freq;
        tlkCodecI.mode       = mode;
        codec_config_input(tlkCodecI.freq, tlkCodecI.cC, tlkCodecI.mode);
        tlkCodecI.state = TLK_CODEC_STATE_CONFIGURED;
    } else if (codec == TLK_CODEC_OUTPUT) {
        if (mode == TLK_CODEC_MIC) {
            return TLK_CODEC_NON_SUPPORT;
        }
        tlkCodecO.pBuffer    = buffer;
        tlkCodecO.pBufferLen = bufferLen;
        tlkCodecO.cC         = chanC;
        tlkCodecO.freq       = freq;
        tlkCodecO.mode       = mode;
        codec_config_output(tlkCodecO.freq, tlkCodecO.cC, tlkCodecO.mode);
        tlkCodecO.state = TLK_CODEC_STATE_CONFIGURED;
    } else {
        return TLK_CODEC_EMPTY_CODEC;
    }
    return TLK_CODEC_SUCCESS;
}

tlk_codec_state_e tlk_codec_getState(tlk_codec_e codec)
{
    if (codec == TLK_CODEC_INPUT) {
        return tlkCodecI.state;
    } else if (codec == TLK_CODEC_OUTPUT) {
        return tlkCodecO.state;
    }
    return TLK_CODEC_SUCCESS;
}

tlk_codec_sts_e tlk_codec_start(tlk_codec_e codec)
{
    if (codec == TLK_CODEC_INPUT) {
        if (tlkCodecI.state == TLK_CODEC_STATE_IDLE) {
            return TLK_CODEC_STATE_ERROR;
        } else if (tlkCodecI.state == TLK_CODEC_STATE_STREAMING) {
            return TLK_CODEC_OPERATION_REPEAT;
        }
        codec_input_enable(tlkCodecI.pBuffer, tlkCodecI.pBufferLen);
        tlkCodecI.state = TLK_CODEC_STATE_STREAMING;
    } else if (codec == TLK_CODEC_OUTPUT) {
        if (tlkCodecO.state == TLK_CODEC_STATE_IDLE) {
            return TLK_CODEC_STATE_ERROR;
        } else if (tlkCodecO.state == TLK_CODEC_STATE_STREAMING) {
            return TLK_CODEC_OPERATION_REPEAT;
        }
        codec_output_enable(tlkCodecO.pBuffer, tlkCodecO.pBufferLen);
        tlkCodecO.state = TLK_CODEC_STATE_STREAMING;
    }
    return TLK_CODEC_SUCCESS;
}

tlk_codec_sts_e tlk_codec_stop(tlk_codec_e codec)
{
    if (codec == TLK_CODEC_INPUT) {
        if (tlkCodecI.state != TLK_CODEC_STATE_STREAMING) {
            return TLK_CODEC_STATE_ERROR;
        }
        codec_input_disable();
        tlkCodecI.state = TLK_CODEC_STATE_CONFIGURED;
    } else if (codec == TLK_CODEC_OUTPUT) {
        if (tlkCodecO.state != TLK_CODEC_STATE_STREAMING) {
            return TLK_CODEC_STATE_ERROR;
        }
        codec_output_disable();
        tlkCodecO.state = TLK_CODEC_STATE_CONFIGURED;
    }
    return TLK_CODEC_SUCCESS;
}

/****************************************input*****************************************/
tlk_codec_sts_e tlk_codec_input_dataPop(u8 *pData, u16 pDataLen)
{
    if (tlkCodecI.state != TLK_CODEC_STATE_STREAMING) {
        return TLK_CODEC_STATE_ERROR;
    }
    if (pData == NULL || pDataLen == 0) {
        return TLK_CODEC_PARAMETER_ERROR;
    }
    if (codec_input_getDataLen() < pDataLen) {
        return TLK_CODEC_DATA_INSUFFICIENT;
    }
    codec_input_readData(pData, pDataLen);
    return TLK_CODEC_SUCCESS;
}

/****************************************output*****************************************/
_attribute_ram_code_ tlk_codec_sts_e tlk_codec_output_dataPush(u8 *pData, u16 pDataLen)
{
    if (tlkCodecO.state != TLK_CODEC_STATE_STREAMING) {
        return TLK_CODEC_STATE_ERROR;
    }
    if (pData == NULL || pDataLen == 0 || pDataLen > tlkCodecO.pBufferLen || (pDataLen & 0x01) != 0) {
        return TLK_CODEC_PARAMETER_ERROR;
    }
    codec_output_writeData(pData, pDataLen);
    return TLK_CODEC_SUCCESS;
}

_attribute_ram_code_ tlk_codec_sts_e tlk_codec_output_getOffset(u32 *writeOffset, u32 *readOffset)
{
    if (tlkCodecO.state != TLK_CODEC_STATE_STREAMING) {
        return TLK_CODEC_STATE_ERROR;
    }
    if (writeOffset == NULL || readOffset == NULL) {
        return TLK_CODEC_PARAMETER_ERROR;
    }
    *writeOffset = codec_output_getWriteOffset();
    *readOffset  = codec_output_getReadOffset();
    return TLK_CODEC_SUCCESS;
}

_attribute_ram_code_ tlk_codec_sts_e tlk_codec_output_setWriteOffset(u32 offset)
{
    if (tlkCodecO.state != TLK_CODEC_STATE_STREAMING) {
        return TLK_CODEC_STATE_ERROR;
    }
    codec_output_setWriteOffset(offset);
    return TLK_CODEC_SUCCESS;
}

tlk_codec_sts_e tlk_codec_output_mute(void)
{
    if (tlkCodecO.state == TLK_CODEC_STATE_IDLE) {
        return TLK_CODEC_STATE_ERROR;
    }
    memset(tlkCodecO.pBuffer, 0, tlkCodecO.pBufferLen);
    return TLK_CODEC_SUCCESS;
}
