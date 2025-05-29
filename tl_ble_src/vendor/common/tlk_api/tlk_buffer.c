/********************************************************************************************************
 * @file    tlk_buffer.c
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
#include "tlk_buffer.h"

tlk_buffer_t tlkBuffer[TLK_BUFFER_MAX];

tlk_buffer_sts_e tlk_buffer_init(u8 *pBuffer, u16 buffLen, tlk_buffer_e index)
{
    if (buffLen == 0 || pBuffer == NULL) {
        return TLK_BUFFER_PARAMETER_ERROR;
    }
    tlkBuffer[index].buffer    = pBuffer;
    tlkBuffer[index].bufferLen = buffLen;
    return TLK_BUFFER_SUCCESS;
}

_attribute_ram_code_ tlk_buffer_sts_e tlk_buffer_clear(tlk_buffer_e index)
{
    if (tlkBuffer[index].bufferLen == 0 || tlkBuffer[index].buffer == NULL) {
        return TLK_BUFFER_NO_INIT;
    }
    tlkBuffer[index].bufferROffset = 0;
    tlkBuffer[index].bufferWOffset = 0;
    for (u32 i = 0; i < tlkBuffer[index].bufferLen; i++) {
        *(tlkBuffer[index].buffer + i) = 0;
    }
    return TLK_BUFFER_SUCCESS;
}

_attribute_ram_code_ tlk_buffer_sts_e tlk_buffer_write(u8 *pData, u16 dataLen, tlk_buffer_e index)
{
    u32 wptr;
    u16 offset;
    u8 *pBuffer = (u8 *)tlkBuffer[index].buffer;

    if (tlkBuffer[index].bufferLen == 0 || tlkBuffer[index].buffer == NULL) {
        return TLK_BUFFER_NO_INIT;
    }
    if (pData == NULL || dataLen == 0 || (dataLen & 0x01) != 0) //2byte a frame
    {
        return TLK_BUFFER_PARAMETER_ERROR;
    }

    wptr = tlkBuffer[index].bufferWOffset;

    if (wptr + dataLen > tlkBuffer[index].bufferLen) {
        offset = tlkBuffer[index].bufferLen - wptr;
    } else {
        offset = dataLen;
    }
    for (int i = 0; i < offset; i++) {
        *(pBuffer + wptr + i) = *(pData + i);
    }
    if (offset < dataLen) {
        for (int i = 0; i < dataLen - offset; i++) {
            *(pBuffer + i) = *(pData + offset + i);
        }
    }
    wptr += dataLen;
    if (wptr >= tlkBuffer[index].bufferLen) {
        wptr -= tlkBuffer[index].bufferLen;
    }
    tlkBuffer[index].bufferWOffset = wptr;

    return TLK_BUFFER_SUCCESS;
}

_attribute_ram_code_ tlk_buffer_sts_e tlk_buffer_read(u8 *pData, u16 dataLen, tlk_buffer_e index)
{
    u32 wptr;
    u32 rptr;
    u32 usbDataLen;
    u32 offset;

    if (tlkBuffer[index].bufferLen == 0 || tlkBuffer[index].buffer == NULL) {
        return TLK_BUFFER_NO_INIT;
    }
    if (pData == NULL || dataLen == 0 || (dataLen & 0x01) != 0) //at least 2byte a frame
    {
        return TLK_BUFFER_PARAMETER_ERROR;
    }

    rptr = tlkBuffer[index].bufferROffset;
    wptr = tlkBuffer[index].bufferWOffset;

    if (wptr >= rptr) {
        usbDataLen = wptr - rptr;
    } else {
        usbDataLen = tlkBuffer[index].bufferLen + wptr - rptr;
    }

    if (usbDataLen < dataLen) {
        return TLK_BUFFER_DATA_INSUFFICIENT; //data length not enough
    }

    if (dataLen + rptr >= tlkBuffer[index].bufferLen) {
        offset                         = tlkBuffer[index].bufferLen - rptr;
        tlkBuffer[index].bufferROffset = dataLen + rptr - tlkBuffer[index].bufferLen;
    } else {
        offset = dataLen;
        tlkBuffer[index].bufferROffset += dataLen;
    }
    for (u32 i = 0; i < offset; i++) {
        *(pData + i) = *(tlkBuffer[index].buffer + rptr + i);
    }
    if (offset < dataLen) {
        for (u32 i = 0; i < dataLen - offset; i++) {
            *(pData + offset + i) = *(tlkBuffer[index].buffer + i);
        }
    }
    return TLK_BUFFER_SUCCESS;
}
