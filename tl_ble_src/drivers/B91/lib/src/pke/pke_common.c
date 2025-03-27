/********************************************************************************************************
 * @file    pke_common.c
 *
 * @brief   This is the source file for B91
 *
 * @author  Driver Group
 * @date    2019
 *
 * @par     Copyright (c) 2019, Telink Semiconductor (Shanghai) Co., Ltd.
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
/********* pke version:1.0 *********/
#include "lib/include/pke/pke_common.h"


/**
 * @brief       copy uint32 buffer.
 * @param[out]  dst     - output word buffer.
 * @param[in]   src     - input word value.
 * @param[in]   wordLen - word length of buffer dst or src.
 * @return      none.
 */
void uint32_copy(unsigned int *dst, unsigned int *src, unsigned int wordLen)
{
    unsigned int i;

    if(dst != src)
    {
        for(i=0; i<wordLen; i++)
        {
            dst[i] = src[i];
        }
    }
}

/**
 * @brief           clear uint32 buffer.
 * @param[in&out]   a        - word buffer a.
 * @param[in]       aWordLen - word length of buffer a.
 * @return          none.
 */
void uint32_clear(unsigned int *a, unsigned int wordLen)
{
    while(wordLen)
    {
        a[--wordLen] = 0;
    }
}

/**
 * @brief       reverse byte order in every unsigned int word.
 * @param[in]   in      - input byte buffer.
 * @param[out]  out     - output word buffer.
 * @param[in]   byteLen - byte length of buffer in or out.
 * @return      none.
 * @caution     byteLen must be multiple of 4.
 */
void reverse_word(unsigned char *in, unsigned char *out, unsigned int bytelen)
{
    unsigned int i, len;
    unsigned char tmp;
    unsigned char *p = in;

    if(in == out)
    {
        while(bytelen>0)
        {
            tmp=*p;
            *p=*(p+3);
            *(p+3)=tmp;
            p+=1;
            tmp=*p;
            *p=*(p+1);
            *(p+1)=tmp;
            bytelen-=4;
            p+=3;
        }
    }
    else
    {
        for (i = 0; i < bytelen; i++)
        {
            len = i >> 2;
            len = len << 3;
            out[i] = p[len + 3 - i];
        }
    }
}




