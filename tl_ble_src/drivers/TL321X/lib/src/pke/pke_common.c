/********************************************************************************************************
 * @file    pke_common.c
 *
 * @brief   This is the source file for TL321X
 *
 * @author  Driver Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#include "lib/include/pke/pke_common.h"

/**
 * @brief       load input operand to baseaddr
 * @param[out]  baseaddr     - destination data
 * @param[in]   data         - source data
 * @param[in]   wordLen      - word length of data
 * @return      0:success     other:error
 */
void pke_load_operand(unsigned int *baseaddr, unsigned int *data, unsigned int wordLen)
{
    unsigned int i;

    if (baseaddr != data) {
        for (i = 0; i < wordLen; i++) {
            *((volatile unsigned int *)(baseaddr + i)) = data[i];
        }
    } else {
        ;
    }
}

/**
 * @brief       get result operand from baseaddr
 * @param[out]  baseaddr     - source data
 * @param[in]   data         - destination data
 * @param[in]   wordLen      - word length of data
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.operands are both U32 little-endian
  @endverbatim
 */
void pke_read_operand(unsigned int *baseaddr, unsigned int *data, unsigned int wordLen)
{
    unsigned int i;

    if (baseaddr != data) {
        for (i = 0; i < wordLen; i++) {
            data[i] = *((volatile unsigned int *)(baseaddr + i));
        }
    } else {
        ;
    }
}

/**
 * @brief       load input operand(U8 big-endian) to baseaddr
 * @param[out]  baseaddr     - destination data
 * @param[in]   data         - source data, U8 big-endian
 * @param[in]   byteLen      - byte length of data
 * @return      0:success     other:error
 */
void pke_load_operand_U8(unsigned int *baseaddr, unsigned char *data, unsigned int byteLen)
{
    unsigned int t, i;

    if ((unsigned char *)baseaddr != data) {
        for (data += (byteLen - 1); byteLen > 3; byteLen -= 4) {
            t = (unsigned int)(*data);
            t |= ((unsigned int)(*(data - 1))) << 8;
            t |= ((unsigned int)(*(data - 2))) << 16;
            t |= ((unsigned int)(*(data - 3))) << 24;

            *((volatile unsigned int *)(baseaddr++)) = t;

            data -= 4;
        }

        if (byteLen) {
            t = 0;
            for (i = 0; i < byteLen; i++) {
                t |= ((unsigned int)(*data)) << (i << 3);
                data--;
            }

            *((volatile unsigned int *)(baseaddr)) = t;
        } else {
            ;
        }
    } else {
        ;
    }
}

/**
 * @brief       get result operand(U8 big-endian) from baseaddr
 * @param[in]   baseaddr     - source data
 * @param[out]  data         - destination data, U8 big-endian
 * @param[in]   byteLen      - byte length of data
 * @return      0:success     other:error
 */
void pke_read_operand_U8(unsigned int *baseaddr, unsigned char *data, unsigned int byteLen)
{
    unsigned int t, i;

    if (baseaddr != (unsigned int *)data) {
        for (data += (byteLen - 1); byteLen > 3; byteLen -= 4) {
            t = *((volatile unsigned int *)(baseaddr++));

            *data       = (t) & 0xFF;
            *(data - 1) = (t >> 8) & 0xFF;
            *(data - 2) = (t >> 16) & 0xFF;
            *(data - 3) = (t >> 24) & 0xFF;

            data -= 4;
        }

        if (byteLen) {
            t = *((volatile unsigned int *)(baseaddr));

            for (i = 0; i < byteLen; i++) {
                *data = (t >> (i << 3)) & 0xFF;
                data--;
            }
        }
    } else {
        ;
    }
}

/**
 * @brief       set operand with an unsigned int value
 * @param[out]  baseaddr      - operand
 * @param[in]   wordLen       - word length of operand
 * @param[in]   b             - unsigned int value b
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.wordLen can not be 0
  @endverbatim
 */
void pke_set_operand_uint32_value(unsigned int *baseaddr, unsigned int wordLen, unsigned int b)
{
    unsigned int i = wordLen;

    while (i > 1) {
        *((volatile unsigned int *)(baseaddr + (--i))) = 0; //baseaddr[--i] = 0;
    }

    *((volatile unsigned int *)(baseaddr)) = b;             //baseaddr[0] = b;
}
