/********************************************************************************************************
 * @file    tlk_buffer.h
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
#ifndef VENDOR_COMMON_TLK_BUFFER_H_
#define VENDOR_COMMON_TLK_BUFFER_H_
#include "common/bit.h"
#include "types.h"
#include "compiler.h"
#include <string.h>

//buffer error code
typedef enum
{
    TLK_BUFFER_SUCCESS           = 0,
    TLK_BUFFER_PARAMETER_ERROR   = 1,
    TLK_BUFFER_NO_INIT           = 2,
    TLK_BUFFER_DATA_INSUFFICIENT = 3,
} tlk_buffer_sts_e;

//buffer index,default 2,user can extend more
typedef enum
{
    TLK_BUFFER_1   = 0,
    TLK_BUFFER_2   = 1,
    TLK_BUFFER_MAX = 2,
} tlk_buffer_e;

//buffer control struct
typedef struct
{
    u8 *buffer;
    u32 bufferLen;
    u32 bufferROffset;
    u32 bufferWOffset;
} tlk_buffer_t;

/**
 * @brief      This function serves to init the buffer.
 * @param[in]  pBuffer - the buffer need to init.
 * @param[in]  buffLen - buffer length.
 * @param[in]  index   - buffer index.
 * @return     Succeed or failed,search 'tlk_buffer_sts_e' for detail.
 */
tlk_buffer_sts_e tlk_buffer_init(u8 *pBuffer, u16 buffLen, tlk_buffer_e index);

/**
 * @brief      This function serves to clear the buffer,which means all of the buffer will be set to '0'.
 * @param[in]  index  - buffer index.
 * @return     Succeed or failed,search 'tlk_buffer_sts_e' for detail.
 */
tlk_buffer_sts_e tlk_buffer_clear(tlk_buffer_e index);

/**
 * @brief      This function serves to write data to the buffer,user needn't consider buffer edge.
 * @param[in]  pData   - the data need to write.
 * @param[in]  dataLen - data length.
 * @param[in]  index   - buffer index.
 * @return     Succeed or failed,search 'tlk_buffer_sts_e' for detail.
 */
tlk_buffer_sts_e tlk_buffer_write(u8 *pData, u16 dataLen, tlk_buffer_e index);

/**
 * @brief      This function serves to read data from the buffer,user needn't consider buffer edge.
 * @param[in]  pData   - read data will store in.
 * @param[in]  dataLen - data length.
 * @param[in]  index   - buffer index.
 * @return     Succeed or failed,search 'tlk_buffer_sts_e' for detail.
 */
tlk_buffer_sts_e tlk_buffer_read(u8 *pData, u16 dataLen, tlk_buffer_e index);
#endif /* VENDOR_COMMON_TLK_BUFFER_H_ */
