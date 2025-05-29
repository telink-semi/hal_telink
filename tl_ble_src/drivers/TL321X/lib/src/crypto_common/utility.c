/********************************************************************************************************
 * @file    utility.c
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
#include "lib/include/crypto_common/utility.h"


#if 0
/**
 * @brief       memory copy, like memcpy()
 * @param[in]   dst         - output, output buffer.
 * @param[in]   src         - input, input buffer.
 * @param[in]   size        - input, bytes of src or dst buffer.
 * @return      none
 * @note
   @verbatim
      -# 1. please make sure neither of dst,src is NULL.
      -# 2. 2. please make sure dst buffer and src buffer do not have common part.
   @endverbatim
 */
void memcpy_(void *dst, const void *src, unsigned int size)
{
    #if 0
    while(size--)
    {
        *(dst++) = *(src++);
    }
    #else
    unsigned int *a_u32;
    const unsigned int *b_u32;
    unsigned char *a_u8 = (unsigned char *)dst;
    const unsigned char *b_u8 = (const unsigned char *)src;
    unsigned int i, count, tmp;

        #ifdef SUPPORT_STATIC_ANALYSIS
    if((NULL != dst) && (NULL != src))
    {
        #endif
        if((0U != (((unsigned int)dst) & 3U)) || (0U != (((unsigned int)src) & 3U)))
        {
            for(i = 0U; i<size; i++)
            {
                a_u8[i] = b_u8[i];
            }
        }
        else
        {
            a_u32 = (unsigned int *)dst;
            b_u32 = (const unsigned int *)src;
            count = size>>2;
            for(i=0U; i<count; i++)
            {
                a_u32[i] = b_u32[i];
            }

            tmp = size&3U;
            if(0U != tmp)
            {
                a_u8 = &(a_u8[size&(~0x03U)]);
                b_u8 = &(b_u8[size&(~0x03U)]);
                for(i=0U; i<tmp; i++)
                {
                    a_u8[i] = b_u8[i];
                }
            }
            else
            {}
        }
        #ifdef SUPPORT_STATIC_ANALYSIS
    }
    else
    {}
        #endif
    #endif
}


/**
 * @brief       memory set, like memset()
 * @param[in]   dst         - output, output buffer.
 * @param[in]   value       - input, unsigned char value.
 * @param[in]   size        - input, bytes of dst buffer.
 * @return      none
 * @note
   @verbatim
      -# 1. please make sure dst is not NULL.
   @endverbatim
 */
void memset_(void *dst, unsigned char value, unsigned int size)
{
    #if 0
    unsigned int i = 0u;

    for(; i<size; i++)
    {
        ((unsigned char *)dst)[i] = value;
    }
    #else
    unsigned int i, count, tmp;
    unsigned int is_over = 0U;
    unsigned int bytes = size;
    unsigned char *a_u8 = (unsigned char *)dst;
    unsigned int *a_u32;

        #ifdef SUPPORT_STATIC_ANALYSIS
    if(NULL != dst)
    {
        #endif
        tmp = ((unsigned int)dst) & 3U;
        if(0U != tmp)
        {
            tmp = 4U - tmp;
            if(bytes > tmp)
            {
                for(i=0U; i<tmp; i++)
                {
                    a_u8[i] = value;
                }
                a_u8 = &a_u8[tmp];
                bytes -= tmp;
            }
            else
            {
                for(i=0U; i<bytes; i++)
                {
                    a_u8[i] = value;
                }
                is_over = 1U;
            }
        }
        else
        {}

        if(0U == is_over)
        {
            a_u32 = (unsigned int *)a_u8;
            count = bytes>>2;
            if(0U != count)
            {
                tmp = (unsigned int)value;
                tmp = (tmp<<8)|((unsigned int)value);
                tmp = (tmp<<8)|((unsigned int)value);
                tmp = (tmp<<8)|((unsigned int)value);
                uint32_set(a_u32, tmp, count);
                a_u32 = &(a_u32[count]);
            }
            else
            {}

            tmp = bytes&3U;
            if(0U != tmp)
            {
                a_u8 = (unsigned char *)a_u32;
                for(i=0; i<tmp; i++)
                {
                    a_u8[i] = value;
                }
            }
            else
            {}
        }
        else
        {}
        #ifdef SUPPORT_STATIC_ANALYSIS
    }
    else
    {}
        #endif
    #endif
}


/**
 * @brief       memory set, like memset()
 * @param[in]   m1         - input, unsigned char buffer m1.
 * @param[in]   m2         - input, unsigned char buffer m2
 * @param[in]   size       - input, bytes of buffer m1 or m2.
 * @return      0(m1 = m2), other(m1 != m2)
 * @note
   @verbatim
      -# 1. please make sure neither of m1,m2 is NULL.
   @endverbatim
 */
unsigned char memcmp_(const void *m1, const void *m2, unsigned int size)
{
    const unsigned char *p1 = (const unsigned char *)m1;
    const unsigned char *p2 = (const unsigned char *)m2;
    unsigned int bytes = size;
    unsigned char c = (unsigned char)0;

    #ifdef SUPPORT_STATIC_ANALYSIS
    if((NULL != m1) && (NULL != m2))
    {
    #endif
        while(0U != bytes)
        {
            c = p1[0] - p2[0];
            if((unsigned char)0 != c)
            {
                break;
            }
            else
            {}

            p1 = &p1[1];
            p2 = &p2[1];
            bytes--;
        }
    #ifdef SUPPORT_STATIC_ANALYSIS
    }
    else
    {}
    #endif

    return c;
}

#endif

/**
 * @brief       set uint32 buffer
 * @param[in]   a             - output, output word buffer.
 * @param[in]   value         - input, input word value.
 * @param[in]   wordLen       - input, word length of buffer a.
 * @return      none
 * @note
   @verbatim
      -# 1. please make sure a is not NULL.
   @endverbatim
 */
void uint32_set(unsigned int *a, unsigned int value, unsigned int wordLen)
{
    unsigned int i = wordLen;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
#endif
        while (0U != i) {
            --i;
            a[i] = value;
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif
}

/**
 * @brief       copy uint32 buffer
 * @param[in]   dst             - output, output word buffer.
 * @param[in]   src             - input, input word buffer.
 * @param[in]   wordLen         - input, word length of buffer dst or src.
 * @return      none
 * @note
   @verbatim
      -# 1. please make sure neither of dst,src is NULL.
   @endverbatim
 */
void uint32_copy(unsigned int *dst, const unsigned int *src, unsigned int wordLen)
{
    unsigned int i;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != dst) && (NULL != src)) {
#endif
        if (dst != src) {
            for (i = 0U; i < wordLen; i++) {
                dst[i] = src[i];
            }
        } else {
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif
}

/**
 * @brief       copy uint32 buffer of 8 words
 * @param[in]   dst             - output, output word buffer.
 * @param[in]   src             - input, input word buffer.
 * @return      none
 * @note
   @verbatim
      -#  please make sure neither of dst,src is NULL.
   @endverbatim
 */
void uint32_copy_8_words(unsigned int *dst, const unsigned int *src)
{
#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != dst) && (NULL != src)) {
#endif
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
        dst[4] = src[4];
        dst[5] = src[5];
        dst[6] = src[6];
        dst[7] = src[7];
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif
}

/**
 * @brief       clear uint32 buffer
 * @param[in]   a             - input&output, word buffer a.
 * @param[in]   wordLen      - input, word length of buffer a.
 * @return      none
 * @note
   @verbatim
      -# 1. please make sure a is not NULL.
   @endverbatim
 */
void uint32_clear(unsigned int *a, unsigned int wordLen)
{
    volatile unsigned int i;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
#endif
#if 1
        i = wordLen;
        while (0U != i) {
            i -= 1U;
            a[i] = 0U;
        }
#else
    for (i = 0U; i < wordLen; i++) {
        a[i] = 0U;
    }
#endif
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif
}

/**
 * @brief       clear uint32 buffer of 8 words
 * @param[in]   a             - input&output, word buffer a.
 * @return      none
 * @note
   @verbatim
      -# 1. please make sure a is not NULL.
   @endverbatim
 */
void uint32_clear_8_words(unsigned int *a)
{
#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
#endif
        a[0] = 0U;
        a[1] = 0U;
        a[2] = 0U;
        a[3] = 0U;
        a[4] = 0U;
        a[5] = 0U;
        a[6] = 0U;
        a[7] = 0U;
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif
}

/**
 * @brief       sleep for a while
 * @param[in]   count             - input, counter for sleeping.
 * @return      a unsigned int value, actually no use, ignore this
 */
static unsigned int uint32_sleep1(unsigned int count)
{
    MEM_VOLATILE unsigned int a = 0U, b = 0U;
    MEM_VOLATILE unsigned int i;
    volatile unsigned int     result = 0U;

    for (i = 0U; i < count; i++) {
        result |= (a - (b + i));
        a &= result;
    }

    return result;
}

/**
 * @brief       sleep for a while
 * @param[in]   count             - input, counter for sleeping.
 * @return      a unsigned int value, actually no use, ignore this
 */
static unsigned int uint32_sleep2(unsigned int count)
{
    MEM_VOLATILE unsigned int a = 0U, b = 0U;
    MEM_VOLATILE unsigned int i;
    volatile unsigned int     result = 0U;

    for (i = 0U; i < count; i++) {
        result |= ((a + i) ^ b);
        b ^= result;
    }

    return result;
}

/**
 * @brief       sleep for a while
 * @param[in]   count             - input, count.
 * @param[in]   rand_bit          - input, random bit, only the LSB works.
 * @return      none
 */
void uint32_sleep(unsigned int count, unsigned char rand_bit)
{
    if (0U == (((unsigned int)rand_bit) & 0x01U)) {
        (void)uint32_sleep1(count);
    } else {
        (void)uint32_sleep2(count);
    }
}


#if 0
/**
 * @brief       convert 0x1122334455667788 to 0x4433221188776655
 * @param[in]   in             - source address.
 * @param[out]  out            - destination address.
 * @param[in]   wordLen        - word length of in/out.
 * @return      none
 */
void uint32_endian_reverse(unsigned char *in, unsigned char *out, unsigned int wordLen)
{
    unsigned char tmp;

    if(in == out)
    {
        while(wordLen>0U)
        {
            tmp=*in;
            in[0]=in[3];
            in[3]=tmp;
            in=&(in[1]);
            tmp=*in;
            in[0]=in[1];
            in[1]=tmp;
            wordLen--;
            in=&(in[3]);
        }
    }
    else
    {
        while(wordLen>0U)
        {
            out[0] = in[3];
            out[1] = in[2];
            out[2] = in[1];
            out[3] = in[0];
            wordLen--;
            in = &(in[4]);
            out = &(out[4]);
        }
    }
}


/**
 * @brief       reverse word array
 * @param[in]   in             - input, input buffer.
 * @param[out]  out            - output, output buffer.
 * @param[in]   wordLen        - input, word length of in or out.
 * @return      none
 * @note
   @verbatim
      -# 1. in and out could point the same buffer.
   @endverbatim
 */
void reverse_word_array(unsigned char *in, unsigned int *out, unsigned int wordLen)
{
    unsigned int idx, round = wordLen >> 1;
    unsigned int tmp;
    unsigned int *p_in;

    if(0U != (((unsigned int)(in))&3U))
    {
        memcpy_(out, in, wordLen<<2);
        p_in = out;
    }
    else
    {
        p_in = (unsigned int *)in;
    }

    for (idx = 0U; idx < round; idx++)
    {
        tmp = p_in[idx];
        out[idx] = p_in[wordLen - 1U - idx];
        out[wordLen - 1U - idx] = tmp;
    }

    if ((0U != (wordLen & 0x1U)) && (p_in != out))
    {
        out[round] = p_in[round];
    }
    else
    {}
}
#endif


/**
 * @brief       reverse byte array
 * @param[in]   in             - input, input buffer.
 * @param[out]  out            - output, output buffer.
 * @param[in]   wordLen        - input, word length of in or out.
 * @return      none
 * @note
   @verbatim
      -# 1. please make sure neither of in,out is NULL.
      -# 2. in and out could point the same buffer
   @endverbatim
 */
void reverse_byte_array(const unsigned char *in, unsigned char *out, unsigned int byteLen)
{
    unsigned int  idx, round_num = byteLen >> 1;
    unsigned char tmp;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != in) && (NULL != out)) {
#endif
        for (idx = 0U; idx < round_num; idx++) {
            tmp                     = in[idx];
            out[idx]                = in[byteLen - 1U - idx];
            out[byteLen - 1U - idx] = tmp;
        }

        if ((0U != (byteLen & 0x1U)) && (in != out)) {
            out[round_num] = in[round_num];
        } else {
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif
}


#if 0
/**
 * @brief      reverse byte order in every unsigned int word
 * @param[in]   in             - input, input byte buffer.
 * @param[out]  out            - output, output word buffer.
 * @param[in]   bytelen        - input, byte length of buffer in or out.
 * @return      none
 * @note
   @verbatim
      -# 1. byteLen must be a multiple of 4.
   @endverbatim
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
            *p=*(p+3U);
            *(p+3U)=tmp;
            p+=1U;
            tmp=*p;
            *p=*(p+1U);
            *(p+1U)=tmp;
            bytelen-=4U;
            p+=3U;
        }
    }
    else
    {
        for (i = 0U; i < bytelen; i++)
        {
            len = i >> 2;
            len = len << 3;
            out[i] = p[len + 3U - i];
        }
    }
}
#endif


#if 0
/**
 * @brief     reverse word order
 * @param[in]   in             - input, input word buffer.
 * @param[out]  out            - output, output word buffer.
 * @param[in]   bytelen        - input, word length of buffer in or out.
 * @param[in]   reverse_word   - input, whether to reverse byte order in every word, 0:no, other:yes.
 * @return      none
 * @note
   @verbatim
      -# 1. in DAM mode, the memory may be accessed by words, not by bytes, this function is designed for the case.
   @endverbatim
 */
void dma_reverse_word_array(unsigned int *in, unsigned int *out, unsigned int wordLen, unsigned int reverse_word)
{
    unsigned int i, j;
    unsigned int tmp;
    unsigned int *p=out;

    if(in == out)
    {
        for(i=0U; i<wordLen; i+=4U)
        {
            for (j = 0U; j < 2U; j++)
            {
                tmp = p[j];
                p[j] = p[4U - 1U - j];
                p[4U - 1U - j] = tmp;
            }
            p+=4U;
        }
    }
    else
    {
        for(i=0U; i<wordLen; i+=4U)
        {
            p[0] = in[3];
            p[1] = in[2];
            p[2] = in[1];
            p[3] = in[0];
            p+=4U;
            in+=4U;
        }
    }

    if(0U != reverse_word)
    {
        for (i = 0U; i < wordLen; i++)
        {
            tmp = *out;
            *out = tmp&0xFFU;
            *out <<= 8;
            *out |= (tmp>>8)&0xFFU;
            *out <<= 8;
            *out |= (tmp>>16)&0xFFU;
            *out <<= 8;
            *out |= (tmp>>24)&0xFFU;

            out++;
        }
    }
    else
    {}
}
#endif


/**
 * @brief     reverse byte array
 * @param[in]   in             - input, input buffer, 32 bytes.
 * @param[out]  out            - output, output buffer, 8 words.
 * @return      none
 * @note
   @verbatim
      -# 1. please make sure neither of in,out is NULL.
      -# 2. in and out can not point the same buffer.
      -# 3. this is for big number of 256 bits in SM2, SM9, etc.
   @endverbatim
 */
void u8big_to_u32little_256bits(const unsigned char *in, unsigned int *out)
{
#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != in) && (NULL != out)) {
#endif
        out[7] = ((unsigned int)in[3]) | (((unsigned int)in[2]) << 8u) | (((unsigned int)in[1]) << 16u) | (((unsigned int)in[0]) << 24u);
        out[6] = ((unsigned int)in[7]) | (((unsigned int)in[6]) << 8u) | (((unsigned int)in[5]) << 16u) | (((unsigned int)in[4]) << 24u);
        out[5] = ((unsigned int)in[11]) | (((unsigned int)in[10]) << 8u) | (((unsigned int)in[9]) << 16u) | (((unsigned int)in[8]) << 24u);
        out[4] = ((unsigned int)in[15]) | (((unsigned int)in[14]) << 8u) | (((unsigned int)in[13]) << 16u) | (((unsigned int)in[12]) << 24u);
        out[3] = ((unsigned int)in[19]) | (((unsigned int)in[18]) << 8u) | (((unsigned int)in[17]) << 16u) | (((unsigned int)in[16]) << 24u);
        out[2] = ((unsigned int)in[23]) | (((unsigned int)in[22]) << 8u) | (((unsigned int)in[21]) << 16u) | (((unsigned int)in[20]) << 24u);
        out[1] = ((unsigned int)in[27]) | (((unsigned int)in[26]) << 8u) | (((unsigned int)in[25]) << 16u) | (((unsigned int)in[24]) << 24u);
        out[0] = ((unsigned int)in[31]) | (((unsigned int)in[30]) << 8u) | (((unsigned int)in[29]) << 16u) | (((unsigned int)in[28]) << 24u);
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif
}

/**
 * @brief     reverse byte array
 * @param[in&out]   a             - input&output, 8 words.
 * @return      none
 * @note
   @verbatim
      -# 1. please make sure a is not NULL.
      -# 2. this is for big number of 256 bits in SM2, SM9, etc.
   @endverbatim
 */
void u8big_to_u32little_256bits_self(unsigned int *a)
{
    unsigned int tmp;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
#endif
        tmp  = a[7];
        a[7] = (((unsigned int)a[0]) >> 24) | ((((unsigned int)a[0]) >> 8) & 0xFF00u) | ((((unsigned int)a[0]) << 8) & 0xFF0000u) | (((unsigned int)a[0]) << 24);
        a[0] = (tmp >> 24) | ((tmp >> 8) & 0xFF00u) | ((tmp << 8) & 0xFF0000u) | (tmp << 24);
        tmp  = a[6];
        a[6] = (((unsigned int)a[1]) >> 24) | ((((unsigned int)a[1]) >> 8) & 0xFF00u) | ((((unsigned int)a[1]) << 8) & 0xFF0000u) | (((unsigned int)a[1]) << 24);
        a[1] = (tmp >> 24) | ((tmp >> 8) & 0xFF00u) | ((tmp << 8) & 0xFF0000u) | (tmp << 24);
        tmp  = a[5];
        a[5] = (((unsigned int)a[2]) >> 24) | ((((unsigned int)a[2]) >> 8) & 0xFF00u) | ((((unsigned int)a[2]) << 8) & 0xFF0000u) | (((unsigned int)a[2]) << 24);
        a[2] = (tmp >> 24) | ((tmp >> 8) & 0xFF00u) | ((tmp << 8) & 0xFF0000u) | (tmp << 24);
        tmp  = a[4];
        a[4] = (((unsigned int)a[3]) >> 24) | ((((unsigned int)a[3]) >> 8) & 0xFF00u) | ((((unsigned int)a[3]) << 8) & 0xFF0000u) | (((unsigned int)a[3]) << 24);
        a[3] = (tmp >> 24) | ((tmp >> 8) & 0xFF00u) | ((tmp << 8) & 0xFF0000u) | (tmp << 24);
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif
}

/**
 * @brief     reverse byte array
 * @param[in]   a             - input, input buffer, 8 words.
 * @param[out]  output        - output, output buffer, 32 bytes.
 * @return      none
 * @note
   @verbatim
      -# 1. please make sure neither of in,out is NULL.
      -# 2. in and out can not point the same buffer.
      -# 3. this is for big number of 256 bits in SM2, SM9, etc.
   @endverbatim
 */
void u32little_to_u8big_256bits(const unsigned int *in, unsigned char *out)
{
#if 0
    unsigned int i, j;
    unsigned int t;

    if(out == (unsigned char *)in)
    {
        for(i = 0u; i < 4u; i++)
        {
            t = in[7u-i];
            j = 28u - (i << 2);
            out[j]      = (unsigned char)((in[i] >> 24) & 0xffu);
            out[j + 1u] = (unsigned char)((in[i] >> 16) & 0xffu);
            out[j + 2u] = (unsigned char)((in[i] >> 8) & 0xffu);
            out[j + 3u] = (unsigned char)((in[i]) & 0xffu);
            j = i<<2;
            out[j]      = (unsigned char)((t >> 24) & 0xffu);
            out[j + 1u] = (unsigned char)((t >> 16) & 0xffu);
            out[j + 2u] = (unsigned char)((t >> 8) & 0xffu);
            out[j + 3u] = (unsigned char)(t & 0xffu);
        }
    }
    else
    {
        for(i = 0u; i < 8u; i++)
        {
            j = 28u - (i << 2);
            out[j]      = (unsigned char)((in[i] >> 24) & 0xffu);
            out[j + 1u] = (unsigned char)((in[i] >> 16) & 0xffu);
            out[j + 2u] = (unsigned char)((in[i] >> 8) & 0xffu);
            out[j + 3u] = (unsigned char)((in[i]) & 0xffu);
        }
    }
#elif 0
    unsigned int i, j;

    for (i = 0u; i < 8u; i++) {
        j           = 28u - (i << 2);
        out[j]      = (unsigned char)((in[i] >> 24) & 0xffu);
        out[j + 1u] = (unsigned char)((in[i] >> 16) & 0xffu);
        out[j + 2u] = (unsigned char)((in[i] >> 8) & 0xffu);
        out[j + 3u] = (unsigned char)((in[i]) & 0xffu);
    }
#else
    #ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != in) && (NULL != out)) {
    #endif
        out[28] = ((unsigned char)(in[0] >> 24));
        out[29] = ((unsigned char)(in[0] >> 16));
        out[30] = ((unsigned char)(in[0] >> 8));
        out[31] = ((unsigned char)in[0]);
        out[24] = ((unsigned char)(in[1] >> 24));
        out[25] = ((unsigned char)(in[1] >> 16));
        out[26] = ((unsigned char)(in[1] >> 8));
        out[27] = ((unsigned char)in[1]);
        out[20] = ((unsigned char)(in[2] >> 24));
        out[21] = ((unsigned char)(in[2] >> 16));
        out[22] = ((unsigned char)(in[2] >> 8));
        out[23] = ((unsigned char)in[2]);
        out[16] = ((unsigned char)(in[3] >> 24));
        out[17] = ((unsigned char)(in[3] >> 16));
        out[18] = ((unsigned char)(in[3] >> 8));
        out[19] = ((unsigned char)in[3]);
        out[12] = ((unsigned char)(in[4] >> 24));
        out[13] = ((unsigned char)(in[4] >> 16));
        out[14] = ((unsigned char)(in[4] >> 8));
        out[15] = ((unsigned char)in[4]);
        out[8]  = ((unsigned char)(in[5] >> 24));
        out[9]  = ((unsigned char)(in[5] >> 16));
        out[10] = ((unsigned char)(in[5] >> 8));
        out[11] = ((unsigned char)in[5]);
        out[4]  = ((unsigned char)(in[6] >> 24));
        out[5]  = ((unsigned char)(in[6] >> 16));
        out[6]  = ((unsigned char)(in[6] >> 8));
        out[7]  = ((unsigned char)in[6]);
        out[0]  = ((unsigned char)(in[7] >> 24));
        out[1]  = ((unsigned char)(in[7] >> 16));
        out[2]  = ((unsigned char)(in[7] >> 8));
        out[3]  = ((unsigned char)in[7]);
    #ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
    #endif
#endif
}

/**
 * @brief    C = A XOR B
 * @param[in]   A             - input, byte buffer a.
 * @param[in]   B             - input, byte buffer b.
 * @param[in]   C             - output, C = A XOR B.
 * @param[in]   byteLen       - input, byte length of A,B,C.
 * @return      none
 * @note
   @verbatim
      -# 1. please make sure none of A,B,C is NULL.
   @endverbatim
 */
void uint8_XOR(const unsigned char *A, const unsigned char *B, unsigned char *C, unsigned int byteLen)
{
    unsigned int i;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != A) && (NULL != B) && (NULL != C)) {
#endif
        for (i = 0U; i < byteLen; i++) {
            C[i] = A[i] ^ B[i];
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif
}

/**
 * @brief    C = A XOR B
 * @param[in]   A             - input, word buffer a.
 * @param[in]   B             - input, word buffer b.
 * @param[in]   C             - output, C = A XOR B.
 * @param[in]   byteLen       - input, word length of A,B,C.
 * @return      none
 * @note
   @verbatim
      -# 1. please make sure none of A,B,C is NULL.
   @endverbatim
 */
void uint32_XOR(const unsigned int *A, const unsigned int *B, unsigned int *C, unsigned int wordLen)
{
    unsigned int i;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != A) && (NULL != B) && (NULL != C)) {
#endif
        for (i = 0U; i < wordLen; i++) {
            C[i] = A[i] ^ B[i];
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif
}

/**
 * @brief   get aimed bit value of big integer a
 * @param[in]   a             - input, big integer a.
 * @param[in]   bit_index     - input, aimed bit location.
 * @return      bit value of aimed bit
 * @note
   @verbatim
      -# 1. please make sure a is not NULL.
      -# 2. for the LSB, bit index is 0.
   @endverbatim
 */
unsigned int get_bit_value_by_index(const unsigned int *a, unsigned int bit_index)
{
    unsigned int ret = 0u;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
#endif
        if (0u != (a[(bit_index) >> 5u] & ((unsigned int)1u << (bit_index & 31u)))) {
            ret = 1u;
        } else {
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif

    return ret;
}

/**
 * @brief   get real bit length of big number a of wordLen words
 * @param[in]   a             - input, big integer a.
 * @param[in]   wordLen       - input, word length of a.
 * @return      real bit length of big number a
 * @note
   @verbatim
      -# 1. please make sure a is not NULL.
   @endverbatim
 */
unsigned int get_valid_bits(const unsigned int *a, unsigned int wordLen)
{
    unsigned int i = 0U;
    unsigned int j;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
#endif
        for (i = wordLen; i > 0U; i--) {
            if (0U != a[i - 1U]) {
                break;
            } else {
            }
        }

        if (0U != i) {
            for (j = 32U; j > 0U; j--) {
                if (0U != (a[i - 1U] & (((unsigned int)0x1) << (j - 1U)))) {
                    break;
                } else {
                }
            }

            i = ((i - 1U) << 5U) + j;
        } else {
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif

    return i;
}

/**
 * @brief   get real word length of big number a of max_words words
 * @param[in]   a             - input, big integer a.
 * @param[in]   max_words     - input, max word length of a.
 * @return     real word length of big number a.
 * @note
   @verbatim
      -# 1. please make sure a is not NULL.
   @endverbatim
 */
unsigned int get_valid_words(const unsigned int *a, unsigned int max_words)
{
    unsigned int ret = 0;
    unsigned int i;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
#endif
        i = max_words;
        while (i > 0U) {
            if (0U != a[i - 1U]) {
                ret = i;
                break;
            } else {
            }

            i--;
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif

    return ret;
}

/**
 * @brief   check whether big number or unsigned char buffer a is all zero or not
 * @param[in]   a             - input, byte buffer a.
 * @param[in]   aByteLen      - input, byte length of a.
 * @return    0(a is not zero),1(a is all zero)
 * @note
   @verbatim
      -# 1. please make sure a is not NULL.
   @endverbatim
 */
unsigned int uint8_BigNum_Check_Zero(const unsigned char *a, unsigned int aByteLen)
{
    unsigned int i;
    unsigned int ret = 0U;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
#endif
        ret = 1U;
        for (i = 0U; i < aByteLen; i++) {
            if (0U != a[i]) {
                ret = 0U;
                break;
            } else {
            }
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif

    return ret;
}

/**
 * @brief   check whether big number or unsigned int buffer a is all zero or not
 * @param[in]   a             - input, big integer or word buffer a.
 * @param[in]   aByteLen      - input, word length of a.
 * @return     0(a is not zero), 1(a is all zero)
 * @note
   @verbatim
      -# 1. please make sure a is not NULL.
   @endverbatim
 */
unsigned int uint32_BigNum_Check_Zero(const unsigned int *a, unsigned int aWordLen)
{
    unsigned int i;
    unsigned int ret = 0U;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
#endif
        ret = 1U;
        for (i = 0U; i < aWordLen; i++) {
            if (0U != a[i]) {
                ret = 0U;
                break;
            } else {
            }
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif

    return ret;
}

/**
 * @brief   a = a + b
 * @param[in]   a             - input, big number a, unsigned char big-endian.
 * @param[in]   a_bytes       - input, byte length of a.
 * @param[in]   b             - input, unsigned char integer b.
 * @param[in]   is_secure     - input, is secure implementation, 0(not), other(yes).
 * @return     0(not overflow),1(overflow)
 * @note
   @verbatim
      -# 1. please make sure a is not NULL.
      -# 2. this is mainly used for counter++ in SKE, KDF, etc.
   @endverbatim
 */
unsigned int uint8_big_num_big_endian_add_little(unsigned char *a, unsigned int a_bytes, unsigned char b, unsigned char is_secure)
{
    unsigned int  ret = 0U;
    unsigned int  i;
    unsigned char carry = b;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
#endif
        if ((unsigned char)0 != is_secure) {
            i = a_bytes;
            while (0U != i) {
                i--;
                a[i] += carry;
                if (a[i] < carry) {
                    carry = (unsigned char)1;
                } else {
                    carry = (unsigned char)0;
                }
            }

            ret = (unsigned int)carry;
        } else {
            i = a_bytes;
            while (0U != i) {
                i--;
                a[i] += carry;
                if (a[i] < carry) {
                    carry = (unsigned char)1;
                } else {
                    carry = (unsigned char)0;
                    break;
                }
            }

            ret = (unsigned int)carry;
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif

    return ret;
}

/**
 * @brief   a = a + b
 * @param[in]   a             - input, big number a, unsigned int little-endian.
 * @param[in]   a_words       - input, word length of a.
 * @param[in]   b             - input, unsigned int integer b.
 * @param[in]   is_secure     - input, is secure implementation, 0(not), other(yes).
 * @return     0(not overflow),1(overflow)
 * @note
   @verbatim
      -# 1. please make sure a is not NULL.
      -# this is mainly used for public key algorithm implementation.
   @endverbatim
 */
unsigned int uint32_big_num_little_endian_add_little(unsigned int *a, unsigned int a_words, unsigned int b, unsigned char is_secure)
{
    unsigned int ret = 0U;
    unsigned int i;
    unsigned int carry = b;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
#endif
        if ((unsigned char)0 != is_secure) {
            for (i = 0U; i < a_words; i++) {
                a[i] += carry;
#if 0
                carry = (unsigned int)(a[i] < carry);
#else
                if (a[i] < carry) {
                    carry = 1U;
                } else {
                    carry = 0U;
                }
#endif
            }

            ret = carry;
        } else {
            for (i = 0U; i < a_words; i++) {
                a[i] += carry;
                if (a[i] < carry) {
                    carry = 1U;
                } else {
                    carry = 0U;
                    break;
                }
            }

            ret = carry;
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif

    return ret;
}

/**
 * @brief      compare big integer a and b(same word length)
 * @param[in]  a  input, big integer a
 * @param[in]  b  input, big integer b
 * @param[in]  words input, real word length of a,b
 * @return     0:a=b,   1:a>b,   -1: a<b
 * @note:
 *     1. please make sure neither of a,b is NULL.
 */
static int32_t uint32_BigNumCmp_internal(const unsigned int *a, const unsigned int *b, unsigned int words)
{
    int32_t      ret = 0;
    unsigned int i   = words;

    while (0U != i) {
        i--;
        if (a[i] > b[i]) {
            ret = 1;
        } else if (a[i] < b[i]) {
            ret = -1;
        } else {
            //nothing to do, just for static analysis.
        }

        if (0 != ret) {
            break;
        } else {
        }
    }

    return ret;
}

/**
 * @brief   compare big integer a and b
 * @param[in]   a             - input, big integer a.
 * @param[in]   aWordLen      - input, word length of a.
 * @param[in]   b             - input, big integer b.
 * @param[in]   bWordLen      - input, word length of b.
 * @return     0:a=b,   1:a>b,   -1: a<b
 * @note
   @verbatim
      -# 1. please make sure neither of a,b is NULL.
   @endverbatim
 */
int32_t uint32_BigNumCmp(const unsigned int *a, unsigned int aWordLen, const unsigned int *b, unsigned int bWordLen)
{
    unsigned int a_words, b_words;
    int32_t      ret = 0;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != a) && (NULL != b)) {
#endif
        a_words = get_valid_words(a, aWordLen);
        b_words = get_valid_words(b, bWordLen);

        if (a_words > b_words) {
            ret = 1;
        } else if (a_words < b_words) {
            ret = -1;
        } else {
            ret = uint32_BigNumCmp_internal(a, b, a_words);
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif

    return ret;
}

/**
 * @brief   for a = b*2^t, b is odd, get t
 * @param[in]   a             - big integer a.
 * @return     number of multiple by 2, for a
 * @note
   @verbatim
      -# 1. please make sure a is not NULL.
      -# 2. make sure a != 0.
   @endverbatim
 */
unsigned int Get_Multiple2_Number(const unsigned int *a)
{
    unsigned int t, i = 0U, j = 0U;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
#endif
        while (0U == (a[i])) {
            i++;
        }

        t = a[i];
        for (; j < 32U; j++) {
            if (0U != (t & (((unsigned int)1U) << j))) {
                break;
            } else {
            }
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif

    return (i << 5) | j;
}

/**
 * @brief   a = a/(2^n), here n<32
 * @param[in]   a             - big integer a.
 * @param[in]   a_words       - word length of a.
 * @param[in]   n             - exponent of 2^n, n<32.
 * @return     word length of a = a/(2^n)
 * @note
   @verbatim
      -# 1. please make sure a is not NULL.
      -# 2. make sure a_words is real word length of a and a_words is not 0.
   @endverbatim
 */
FLAG_STATIC unsigned int Big_Div2n_n_less_than_32(unsigned int *a, unsigned int a_words, unsigned int n)
{
    unsigned int ret;
    unsigned int i;

    for (i = 0U; i < (a_words - 1U); i++) {
        a[i] >>= n;
        a[i] |= (a[i + 1U] << (32U - n));
    }
    a[i] >>= n;

    if (0U == a[i]) {
        ret = i;
    } else {
        ret = a_words;
    }

    return ret;
}

/**
 * @brief   a = a/(2^n), here n>=32
 * @param[in]   a             - big integer a.
 * @param[in]   a_words       - word length of a.
 * @param[in]   n             - exponent of 2^n, n>=32.
 * @return     word length of a = a/(2^n),
 * @note
   @verbatim
      -# 1. please make sure a is not NULL.
      -# 2. make sure a_words is real word length of a and a_words is not 0.
      -# 3. actually n could be any value.
   @endverbatim
 */
FLAG_STATIC unsigned int Big_Div2n_n_not_less_than_32(unsigned int *a, unsigned int a_words, unsigned int n)
{
    unsigned int ret;
    unsigned int i, j, bits;

#if 0
    j    = n/32;
    bits = n%32;
#else
    j    = n >> 5U;
    bits = n & 31U;
#endif

    if (j < a_words) {
        for (i = 0; i < (a_words - j); i++) {
            a[i] = a[i + j];
        }
        uint32_clear(&a[a_words - j], j);

        if (0U != bits) //bits is in [1, 31]
        {
            ret = Big_Div2n_n_less_than_32(a, a_words - j, bits);
        } else          //bits is 0
        {
            ret = a_words - j;
        }
    } else {
        uint32_clear(a, a_words);
        ret = 0U;
    }

    return ret;
}

/**
 * @brief   a = a/(2^n)
 * @param[in]   a             - big integer a.
 * @param[in]   aWordLen      - word length of a.
 * @param[in]   n             - exponent of 2^n.
 * @return     word length of a = a/(2^n),
 * @note
   @verbatim
      -# 1. please make sure a is not NULL.
      -# 2. make sure aWordLen is real word length of a.
      -# 3. please make sure aWordLen*32 is not less than n.
   @endverbatim
 */
unsigned int Big_Div2n(unsigned int *a, unsigned int aWordLen, unsigned int n)
{
    unsigned int ret = 0U;
    unsigned int a_words;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
#endif
        a_words = get_valid_words(a, aWordLen);

        if ((0U == n) || (0U == a_words)) {
            ret = a_words;
        } else if (n < 32U) //now a is not zero(a_words is not zero), and n is not zero either.
        {
            ret = Big_Div2n_n_less_than_32(a, a_words, n);
        } else              //now a is not zero(a_words is not zero), and n is greater than 31
        {
            ret = Big_Div2n_n_not_less_than_32(a, a_words, n);
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif

    return ret;
}

/**
 * @brief   check whether a is equal to 1 or not
 * @param[in]   a             - pointer to unsigned int big integer a.
 * @param[in]   aWordLen      - word length of big integer a.
 * @return     1(a is 1), 0(a is not 1)
 * @note
   @verbatim
      -# 1. please make sure a is not NULL.
   @endverbatim
 */
unsigned int Bigint_Check_1(const unsigned int *a, unsigned int aWordLen)
{
    unsigned int i;
    unsigned int ret = 0U;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
#endif
        if ((0U == aWordLen) || (a[0] != 1U)) {
            ret = 0U;
        } else {
            ret = 1U;
            for (i = 1U; i < aWordLen; i++) {
                if (0U != a[i]) {
                    ret = 0U;
                    break;
                } else {
                }
            }
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif

    return ret;
}

/**
 * @brief   check whether a is equal to p-1 or not
 * @param[in]   a             - pointer to unsigned int big integer a.
 * @param[in]   p             - pointer to unsigned int big integer p, p must be odd.
 * @param[in]   wordLen       - word length of a and p.
 * @return     1(a is p-1), 0(a is not p-1)
 * @note
   @verbatim
      -# 1. please make sure neither of a,p is NULL.
      -# 2. please make sure p is odd.
   @endverbatim
 */
unsigned int Bigint_Check_p_1(const unsigned int *a, const unsigned int *p, unsigned int wordLen)
{
    unsigned int i;
    unsigned int ret = 0U;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != a) && (NULL != p)) {
#endif
        if ((0U == wordLen) || (a[0] != (p[0] - 1U))) {
            ret = 0U;
        } else {
            ret = 1U;
            for (i = 1U; i < wordLen; i++) {
                if (a[i] != p[i]) {
                    ret = 0U;
                    break;
                } else {
                }
            }
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif

    return ret;
}

/* function: check whether integer k is in [1, n-1]
 * parameters:
 *     k -------------------------- input, big number k
 *     n -------------------------- input, big number n
 *     wordLen -------------------- input, word length of k and n
 * return:
 *     ret_zero ------------------- k is zero
 *     ret_big -------------------- k is greater/bigger than or equal to n
 *     ret_success ---------------- k is in [1, n-1]
 * caution:
 *     1.
 */

/**
 * @brief   check whether integer k is in [1, n-1]
 * @param[in]   k             - input, big number k.
 * @param[in]   n             - input, big number n.
 * @param[in]   wordLen       - input, word length of k and n.
 * @return     ret_zero      k is zero
 *             ret_big       k is greater/bigger than or equal to n
 *             ret_success   k is in [1, n-1]
 */
unsigned int uint32_integer_check(const unsigned int *k, const unsigned int *n, unsigned int wordLen, unsigned int ret_zero, unsigned int ret_big, unsigned int ret_success)
{
    unsigned int ret;

    if (0U != uint32_BigNum_Check_Zero(k, wordLen)) {
        ret = ret_zero;
    } else if (uint32_BigNumCmp(k, wordLen, n, wordLen) >= 0) {
        ret = ret_big;
    } else {
        ret = ret_success;
    }

    return ret;
}
