/* ***************************************************************************
 *
 * Copyright (c) 2021 Samsung Electronics Co., Ltd All Rights Reserved.
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * This software is the confidential and proprietary information of Samsung
 * Electronics Co., Ltd. ("Confidential Information").
 * You shall not disclose such Confidential Information and shall use it
 * only in accordance with the terms of the license agreement you entered
 * into with Samsung Electronics Co., Ltd. ("SAMSUNG")
 * SAMSUNG MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
 * SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SAMSUNG SHALL NOT BE
 * LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING,
 * MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 *
 ****************************************************************************/

#ifndef TAGSDK_INC_TAGUTIL_H_
#define TAGSDK_INC_TAGUTIL_H_

#include <TagCore.h>
#include "PortOs.h"
/*
 * Macros
 */
#define CONVERT_SEC_TO_MS(x)    ((x) * 1000)
#define CONVERT_SEC_TO_TICKS( xTimeInS ) ( ( PortTickType_t ) ( ( ( PortTickType_t ) ( xTimeInS ) * ( PortTickType_t ) PORT_TICK_RATE_HZ ) ) )

/**
 * @brief Dump memory area
 *
 * @details This function dump a memory area
 * @param[in] tag a string to know the memory attribute
 * @param[in] buf a pointer to a buffer to dump
 * @param[in] len the size of buffer pointed by buf in bytes
 *
 */
#ifdef TAG_CONFIG_DEBUG_PRINT_MEMORY_DUMP
TagError_t TagUtilDumpMem(char *tag, uint8_t *buf, uint16_t len);
#endif

/**
* @brief  Compute CRC kermit over a data chunk.
*
* @param[in] pData        pointer to the data chunk
* @param[in] lenData       the length of the data chunk
* @param[in] crcValueOld  current CRC value
*
* @return  computed CRC.
*
*/
uint16_t TagUtilCrc16(uint8_t *pData, uint16_t lenData, uint16_t crcValueOld);

/**
 * @brief convert hex value to string
 *
 * @param[in] dst       the pointer to store the converted string
 * @param[in] dstSize   the max size of "dst" pointer
 * @param[in] src       the pointer having the value to be converted
 * @param[in] srcSize   the size of "src" pointer's data
 * @param[in] outlen    the pointer to store the length of the converted string
 *
 * @retval      TAG_ERROR_NONE for Success
 * @retval      TAG_ERROR_INVALID_ARG Invalid arguments passed
 */
TagError_t TagUtilConvertHexToString(unsigned char *dst, size_t dstSize,
        unsigned char *src, size_t srcSize, size_t *outlen);

/*
 * in-line functions
 */

static inline size_t writeLittleEndian(uint8_t *dest, size_t offset, uint64_t src, size_t length)
{
    int i;
    for(i = 0; i < length; i++)
    {
        dest[offset + i] = src & 0xff;
        src >>= 8;
    }

    return length;
}

static inline uint64_t readUint64LittleEndian(const uint8_t *src)
{
    uint64_t result = 0;
    int i;
    for(i = 0; i < 8; i++)
    {
        result <<= 8;
        result += src[7 - i];
    }
    return result;
}

static inline uint32_t readUint32LittleEndian(const uint8_t *src)
{
    uint32_t result = 0;
    int i;
    for(i = 0; i < 4; i++)
    {
        result <<= 8;
        result += src[3 - i];
    }
    return result;
}

static inline uint16_t readUint16LittleEndian(const uint8_t *src)
{
    uint32_t result = 0;
    int i;
    for(i = 0; i < 2; i++)
    {
        result <<= 8;
        result += src[1 - i];
    }
    return result;
}


int TagUtilConvertStringToInt(const char *src, int len);

char *TagUtilConvertIntToString(int val, char *dst, int len);

void TagUtilSystemReset(TagBootReason reason, const char *debug_info);

void TagUtilDelayBusy(uint32_t ms);

void TagUtilBubbleSort(int* list, int n);

#endif //TAGSDK_INC_TAGUTIL_H_
