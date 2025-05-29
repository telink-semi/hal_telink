/********************************************************************************************************
 * @file    utility.c
 *
 * @brief   This is the source file for TL323X
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
#include <stdio.h>
#include "lib/include/crypto_common/utility.h"
#include "lib/include/stimer.h"

// Maintenance API
/**
 * @brief           Compare big integer a and b
 * @param[in]       a                    - big integer a
 * @param[in]       a_wlen               - Word length of a
 * @param[in]       b                    - big integer b
 * @param[in]       b_wlen               - Word length of b
 * @return          0 (a = b), 1 (a > b), -1 (a < b)
 * @note            Ensure that neither of a or b is NULL.
 */
int big_integer_compare(const unsigned int *a, unsigned int a_wlen, const unsigned int *b, unsigned int b_wlen)
{
    return uint32_big_num_cmp(a, a_wlen, b, b_wlen);
}

/**
 * @brief           a = a / (2^n)
 * @param[in,out]   a                    - Big integer a, will be modified in place
 * @param[in]       a_wlen               - Word length of a
 * @param[in]       n                    - Exponent of 2^n
 * @return          Word length of a after division by 2^n
 * @note
 *        1. Ensure that a is not NULL.
 *        2. Ensure that a_wlen is the real word length of a.
 *        3. Ensure that a_wlen * 32 is not less than n. a_wlen
 */
unsigned int div2n_u32(unsigned int *a, unsigned int a_wlen, unsigned int n)
{
    return big_div_2n(a, a_wlen, n);
}

/**
 * @brief           Get the maximum value between two inputs
 * @param[in]       a                    - First value
 * @param[in]       b                    - Second value
 * @return          Maximum value between a and b
 */
unsigned int get_max_len(unsigned int a, unsigned int b)
{
    return (a > b) ? a : b;
}

/**
 * @brief           Get the word length from bit length
 * @param[in]       bit_len              - Bit length
 * @return          Word length corresponding to the given bit length
 * @note
 *        1. The function assumes that a word is 32 bits.
 *        2. Caution: Ensure that bit_len is non-negative.
 */
unsigned int get_word_len(unsigned int bit_len)
{
#ifdef SUPPORT_STATIC_ANALYSIS
    unsigned int words, carry;

    words = bit_len >> 5;
    carry = ((bit_len & 31u) + 31u) >> 5;
    words += carry;
    if (words < carry) {}

    return words;
#elif 0
    return (bit_len >> 5) + (((bit_len & 31u) + 31u) >> 5); // to avoid overflow risk
#else
    return (bit_len + 31u) >> 5;
#endif
}

/**
 * @brief           Get the byte length from bit length
 * @param[in]       bit_len              - Bit length
 * @return          Byte length corresponding to the given bit length
 */
unsigned int get_byte_len(unsigned int bit_len)
{
#ifdef SUPPORT_STATIC_ANALYSIS
    unsigned int bytes, carry;

    bytes = bit_len >> 3;
    carry = ((bit_len & 7u) + 7u) >> 3;
    bytes += carry;
    if (bytes < carry) {}

    return bytes;
#elif 0
    return (bit_len >> 3) + (((bit_len & 7u) + 7u) >> 3); // to avoid overflow
                                                          // risk
#else
    return (bit_len + 7u) >> 3;
#endif
}

/**
 * @brief           Memory copy, like memcpy()
 * @param[out]      dst                  - buffer
 * @param[in]       src                  - buffer
 * @param[in]       size                 - Number of bytes to copy (size of src or dst buffer)
 * @return          None
 * @note
 *        1. Ensure that neither of dst nor src is NULL.
 *        2. Ensure that the dst and src buffers do not overlap.
 */
void memcpy_(void *dst, const void *src, unsigned int size)
{
#if 0
    while(size--)
    {
        *(dst++) = *(src++);
    }
#else
    unsigned int        *a_u32;
    const unsigned int  *b_u32;
    unsigned char       *a_u8 = (unsigned char *)dst;
    const unsigned char *b_u8 = (const unsigned char *)src;
    unsigned int         i, count, tmp;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != dst) && (NULL != src)) {
#endif
        if ((0U != (((unsigned int)dst) & 3U)) || (0U != (((unsigned int)src) & 3U))) {
            for (i = 0U; i < size; i++) {
                a_u8[i] = b_u8[i];
            }
        } else {
            a_u32 = (unsigned int *)dst;
            b_u32 = (const unsigned int *)src;
            count = size >> 2;
            for (i = 0U; i < count; i++) {
                a_u32[i] = b_u32[i];
            }

            tmp = size & 3U;
            if (0U != tmp) {
                a_u8 = &(a_u8[size & (~0x03U)]);
                b_u8 = &(b_u8[size & (~0x03U)]);
                for (i = 0U; i < tmp; i++) {
                    a_u8[i] = b_u8[i];
                }
            } else {
            }
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif
#endif
}

/**
 * @brief           Memory set, like memset()
 * @param[out]      dst                  - buffer
 * @param[in]       value                - Unsigned char value to set
 * @param[in]       size                 - Number of bytes to set in the dst buffer
 * @return          None
 * @note            Ensure that dst is not NULL.
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
    unsigned int   i, count, tmp;
    unsigned int   is_over = 0U;
    unsigned int   bytes   = size;
    unsigned char *a_u8    = (unsigned char *)dst;
    unsigned int  *a_u32;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != dst) {
#endif
        tmp = ((unsigned int)dst) & 3U;
        if (0U != tmp) {
            tmp = 4U - tmp;
            if (bytes > tmp) {
                for (i = 0U; i < tmp; i++) {
                    a_u8[i] = value;
                }
                a_u8 = &a_u8[tmp];
                bytes -= tmp;
            } else {
                for (i = 0U; i < bytes; i++) {
                    a_u8[i] = value;
                }
                is_over = 1U;
            }
        } else {
        }

        if (0U == is_over) {
            a_u32 = (unsigned int *)a_u8;
            count = bytes >> 2;
            if (0U != count) {
                tmp = (unsigned int)value;
                tmp = (tmp << 8) | ((unsigned int)value);
                tmp = (tmp << 8) | ((unsigned int)value);
                tmp = (tmp << 8) | ((unsigned int)value);
                uint32_set(a_u32, tmp, count);
                a_u32 = &(a_u32[count]);
            } else {
            }

            tmp = bytes & 3U;
            if (0U != tmp) {
                a_u8 = (unsigned char *)a_u32;
                for (i = 0; i < tmp; i++) {
                    a_u8[i] = value;
                }
            } else {
            }
        } else {
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif
#endif
}

/**
 * @brief           Memory compare, like memcmp()
 * @param[in]       m1                   - Unsigned char buffer m1
 * @param[in]       m2                   - Unsigned char buffer m2
 * @param[in]       size                 - Number of bytes to compare in buffers m1 or m2
 * @return          0 (m1 = m2), non-zero (m1 != m2)
 * @note
 *        1.Ensure that neither of m1 nor m2 is NULL.
 */
unsigned char memcmp_(const void *m1, const void *m2, unsigned int size)
{
    const unsigned char *p1    = (const unsigned char *)m1;
    const unsigned char *p2    = (const unsigned char *)m2;
    unsigned int         bytes = size;
    unsigned char        c     = (unsigned char)0;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != m1) && (NULL != m2)) {
#endif
        while (0U != bytes) {
            c = p1[0] - p2[0];
            if ((unsigned char)0 != c) {
                break;
            } else {
            }

            p1 = &p1[1];
            p2 = &p2[1];
            bytes--;
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif

    return c;
}

/**
 * @brief           Set uint32 buffer
 * @param[out]      a                    - word buffer
 * @param[in]       value                - word value to set
 * @param[in]       wlen                 - Word length of buffer a
 * @return          None
 * @note
 *        1.Ensure that a is not NULL.
 */
void uint32_set(unsigned int *a, unsigned int value, unsigned int wlen)
{
    unsigned int i = wlen;

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
 * @brief           Copy uint32 buffer
 * @param[out]      dst                  - word buffer
 * @param[in]       src                  - word buffer
 * @param[in]       wlen                 - Word length of buffer dst or src
 * @return          None
 * @note
 *        1. Ensure that neither of dst nor src is NULL.
 */
void uint32_copy(unsigned int *dst, const unsigned int *src, unsigned int wlen)
{
    unsigned int i;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != dst) && (NULL != src)) {
#endif
        if (dst != src) {
            for (i = 0U; i < wlen; i++) {
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
 * @brief           Copy uint32 buffer of 8 words
 * @param[out]      dst                  - word buffer
 * @param[in]       src                  - word buffer
 * @return          None
 * @note
 *        1. Ensure that neither of dst nor src is NULL.
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
 * @brief           Clear uint32 buffer
 * @param[in,out]   a                    - Word buffer a, will be cleared in place
 * @param[in]       a_wlen               - Word length of buffer a
 * @return          None
 * @note
 *        1. Ensure that a is not NULL.
 */
void uint32_clear(unsigned int *a, unsigned int a_wlen)
{
    volatile unsigned int i;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
#endif
#if 1
        i = a_wlen;
        while (0U != i) {
            i -= 1U;
            a[i] = 0U;
        }
#else
    for (i = 0U; i < wlen; i++) {
        a[i] = 0U;
    }
#endif
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif
}

/**
 * @brief           Clear uint32 buffer of 8 words
 * @param[in,out]   a                    - Word buffer a, will be cleared in place
 * @return          None
 * @note
 *        1. Ensure that a is not NULL.
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
 * @brief           Sleep for a while
 * @param[in]       count                - Counter for sleeping
 * @return          An unsigned int value (actually no use, ignore this)
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
 * @brief           Sleep for a while
 * @param[in]       count                - Counter for sleeping
 * @return          An unsigned int value (actually no use, ignore this)
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
 * @brief           Sleep for a while
 * @param[in]       count                - Count for sleeping
 * @param[in]       rand_bit             - Random bit, only the LSB works
 * @return          None
 */
void uint32_sleep(unsigned int count, unsigned char rand_bit)
{
    if (0U == (((unsigned int)rand_bit) & 0x01U)) {
        (void)uint32_sleep1(count);
    } else {
        (void)uint32_sleep2(count);
    }
}

/**
 * @brief           Reverse byte array
 * @param[in]       in                   - buffer
 * @param[out]      out                  - buffer
 * @param[in]       byteLen              - Byte length of input or output buffer
 * @return          None
 *        1. Ensure that neither of in nor out is NULL.
 *        2. in and out could point to the same buffer.
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
 * @brief           Reverse byte order in every unsigned int word
 * @param[in]       in                   - buffer (byte array)
 * @param[out]      out                  - buffer (word array)
 * @param[in]       bytelen              - Byte length of input or output buffer
 * @return          None
 * @note
 *        1. Ensure that byteLen must be a multiple of 4.
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
 * @brief           Reverse word order
 * @param[in]       in                   - buffer (word array)
 * @param[out]      out                  - buffer (word array)
 * @param[in]       wlen                 - Word length of input or output buffer
 * @param[in]       reverse_word         - Whether to reverse byte order in every word, 0: no, other: yes
 * @return          None
 * @note
 *        1. In DAM mode, the memory may be accessed by words, not by bytes. This
 *           function is designed for that case.
 */
void dma_reverse_word_array(unsigned int *in, unsigned int *out, unsigned int wlen, unsigned int reverse_word)
{
    unsigned int i, j;
    unsigned int tmp;
    unsigned int *p=out;

    if(in == out)
    {
        for(i=0U; i<wlen; i+=4U)
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
        for(i=0U; i<wlen; i+=4U)
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
        for (i = 0U; i < wlen; i++)
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
 * @brief           Reverse byte array
 * @param[in]       data_in              - buffer (32 bytes)
 * @param[out]      data_out             - buffer (8 words)
 * @return          None
 * @note
 *        1. Ensure that neither of data_in nor data_out is NULL.
 *        2. data_in and data_out cannot point to the same buffer.
 *        3. This is for big numbers of 256 bits data_in SM2, SM9, etc.
 */
void u8big_to_u32little_256bits(const unsigned char *data_in, unsigned int *data_out)
{
#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != data_in) && (NULL != data_out)) {
#endif
        data_out[7] = ((unsigned int)data_in[3]) | (((unsigned int)data_in[2]) << 8u) | (((unsigned int)data_in[1]) << 16u) | (((unsigned int)data_in[0]) << 24u);
        data_out[6] = ((unsigned int)data_in[7]) | (((unsigned int)data_in[6]) << 8u) | (((unsigned int)data_in[5]) << 16u) | (((unsigned int)data_in[4]) << 24u);
        data_out[5] = ((unsigned int)data_in[11]) | (((unsigned int)data_in[10]) << 8u) | (((unsigned int)data_in[9]) << 16u) | (((unsigned int)data_in[8]) << 24u);
        data_out[4] = ((unsigned int)data_in[15]) | (((unsigned int)data_in[14]) << 8u) | (((unsigned int)data_in[13]) << 16u) | (((unsigned int)data_in[12]) << 24u);
        data_out[3] = ((unsigned int)data_in[19]) | (((unsigned int)data_in[18]) << 8u) | (((unsigned int)data_in[17]) << 16u) | (((unsigned int)data_in[16]) << 24u);
        data_out[2] = ((unsigned int)data_in[23]) | (((unsigned int)data_in[22]) << 8u) | (((unsigned int)data_in[21]) << 16u) | (((unsigned int)data_in[20]) << 24u);
        data_out[1] = ((unsigned int)data_in[27]) | (((unsigned int)data_in[26]) << 8u) | (((unsigned int)data_in[25]) << 16u) | (((unsigned int)data_in[24]) << 24u);
        data_out[0] = ((unsigned int)data_in[31]) | (((unsigned int)data_in[30]) << 8u) | (((unsigned int)data_in[29]) << 16u) | (((unsigned int)data_in[28]) << 24u);
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif
}

/**
 * @brief           Reverse byte array
 * @param[in,out]   a                    - buffer (8 words)
 * @return          None
 * @note
 *        1. Ensure that a is not NULL.
 *        2. This is for big numbers of 256 bits in SM2, SM9, etc.
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
 * @brief           Reverse byte array
 * @param[in]       in                   - buffer (8 words)
 * @param[out]      out                  - buffer (32 bytes)
 * @return          None
 * @note
 *        1. Ensure that neither of in nor out is NULL.
 *        2. in and out cannot point to the same buffer.
 *        3. This is for big numbers of 256 bits in SM2, SM9, etc.
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
 * @brief           C = A XOR B
 * @param[in]       A                    - byte buffer a
 * @param[in]       B                    - byte buffer b
 * @param[out]      C                    - byte buffer, C = A XOR B
 * @param[in]       byteLen              - Byte length of buffers A, B, and C
 * @return          None
 * @note
 *        1. Ensure that none of A, B, or C is NULL.
 */
void uint8_xor(const unsigned char *A, const unsigned char *B, unsigned char *C, unsigned int byteLen)
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
 * @brief           C = A XOR B
 * @param[in]       A                    - word buffer a
 * @param[in]       B                    - word buffer b
 * @param[out]      C                    - word buffer, C = A XOR B
 * @param[in]       wlen                 - Word length of buffers A, B, and C
 * @return          None
 * @note
 *        1. Ensure that none of A, B, or C is NULL.
  */
void uint32_xor(const unsigned int *A, const unsigned int *B, unsigned int *C, unsigned int wlen)
{
    unsigned int i;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != A) && (NULL != B) && (NULL != C)) {
#endif
        for (i = 0U; i < wlen; i++) {
            C[i] = A[i] ^ B[i];
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif
}

/**
 * @brief           Get aimed bit value of big integer a
 * @param[in]       a                    - big integer a
 * @param[in]       bit_index            - aimed bit location
 * @return          Bit value of aimed bit (0 or 1)
 * @note
 *        1. Ensure that a is not NULL.
 *        2. For the LSB, bit index is 0.
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
 * @brief           Get real bit length of big number a of wlen words
 * @param[in]       a                    - big integer a
 * @param[in]       wlen                 - word length of a
 * @return          Real bit length of big number a
 * @note
 *        1. Ensure that a is not NULL.
  */
unsigned int get_valid_bits(const unsigned int *a, unsigned int wlen)
{
    unsigned int i = 0U;
    unsigned int j;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
#endif
        for (i = wlen; i > 0U; i--) {
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
 * @brief           Get real word length of big number a of max_words words
 * @param[in]       a                    - -big integer a
 * @param[in]       max_words            - -maximum word length of a
 * @return          Real word length of big number a
 * @note
 *        1. Ensure that a is not NULL.
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
 * @brief           Check whether big number or unsigned char buffer a is all zero or not
 * @param[in]       a                    - -byte buffer a
 * @param[in]       a_len                - -byte length of a
 * @return          0 (a is not zero), 1 (a is all zero)
 * @note
 *        1. Ensure that a is not NULL.
  */
unsigned int uint8_bignum_check_zero(const unsigned char *a, unsigned int a_len)
{
    unsigned int i;
    unsigned int ret = 0U;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
#endif
        ret = 1U;
        for (i = 0U; i < a_len; i++) {
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
 * @brief           Check whether big number or unsigned int buffer a is all zero or not
 * @param[in]       a                    - -big integer or word buffer a
 * @param[in]       a_wlen               - -word length of a
 * @return          0 (a is not zero), 1 (a is all zero)
 * @note
 *        1. Ensure that a is not NULL.
  */
unsigned int uint32_bignum_check_zero(const unsigned int *a, unsigned int a_wlen)
{
    unsigned int i;
    unsigned int ret = 0U;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
#endif
        ret = 1U;
        for (i = 0U; i < a_wlen; i++) {
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
 * @brief           a = a + b
 * @param[in,out]   a                    - big number a, unsigned char big-endian
 * @param[in]       a_bytes              - byte length of a
 * @param[in]       b                    - unsigned char integer b
 * @param[in]       is_secure            - is secure implementation, 0 (not), other (yes)
 * @return          0 (no overflow), 1 (overflow)
 * @note
 *        1. Ensure that a is not NULL.
 *        2. This is mainly used for counter++ in SKE, KDF, etc.
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
 * @brief           a = a + b
 * @param[in,out]   a                    - big number a, unsigned int little-endian
 * @param[in]       a_words              - word length of a
 * @param[in]       b                    - unsigned int integer b
 * @param[in]       is_secure            - is secure implementation, 0 (not), other (yes)
 * @return          0 (no overflow), 1 (overflow)
 * @note
 *        1. Ensure that a is not NULL.
 *        2. This is mainly used for public key algorithm implementation.
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
 * @brief           Compare big integer a and b (same word length)
 * @param[in]       a                    - big integer a
 * @param[in]       b                    - big integer b
 * @param[in]       words                - real word length of a, b
 * @return          0 (a = b), 1 (a > b), -1 (a < b)
 * @note
 *        1. Ensure that neither a nor b is NULL.
  */
FLAG_STATIC int uint32_bignumcmp_internal(const unsigned int *a, const unsigned int *b, unsigned int words)
{
    int          ret = 0;
    unsigned int i   = words;

    while (0U != i) {
        i--;
        if (a[i] > b[i]) {
            ret = 1;
        } else if (a[i] < b[i]) {
            ret = -1;
        } else {
            // nothing to do, just for static analysis.
        }

        if (0 != ret) {
            break;
        } else {
        }
    }

    return ret;
}

/**
 * @brief           Compare big integer a and b
 * @param[in]       a                    - big integer a
 * @param[in]       a_wlen               - word length of a
 * @param[in]       b                    - big integer b
 * @param[in]       b_wlen               - word length of b
 * @return          0 (a = b), 1 (a > b), -1 (a < b)
 * @note
 *        1. Ensure that neither a nor b is NULL.
  */
int uint32_big_num_cmp(const unsigned int *a, unsigned int a_wlen, const unsigned int *b, unsigned int b_wlen)
{
    unsigned int a_words, b_words;
    int          ret = 0;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != a) && (NULL != b)) {
#endif
        a_words = get_valid_words(a, a_wlen);
        b_words = get_valid_words(b, b_wlen);

        if (a_words > b_words) {
            ret = 1;
        } else if (a_words < b_words) {
            ret = -1;
        } else {
            ret = uint32_bignumcmp_internal(a, b, a_words);
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif

    return ret;
}

/**
 * @brief           For a = b*2^t, where b is odd, get t
 * @param[in]       a                    - big integer a
 * @return          Number of multiples by 2 for a
 * @note
 *        1. Ensure that a is not NULL.
 *        2. Ensure that a != 0.
  */
unsigned int get_multiple2_number(const unsigned int *a)
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
 * @brief           a = a / (2^n), where n < 32
 * @param[in]       a                    - big integer a
 * @param[in]       a_words              - Word length of a
 * @param[in]       n                    - Exponent of 2^n, n < 32
 * @return          Word length of a after division by 2^n
 * @note
 *        1. Ensure that a is not NULL.
 *        2. Ensure that a_words is the real word length of a and a_words is not 0.
  */
FLAG_STATIC unsigned int big_div_2n_n_l32(unsigned int *a, unsigned int a_words, unsigned int n)
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
 * @brief           a = a / (2^n), where n >= 32
 * @param[in]       a                    - big integer a
 * @param[in]       a_words              - Word length of a
 * @param[in]       n                    - Exponent of 2^n, n >= 32
 * @return          Word length of a after division by 2^n
 * @note
 *        1. Ensure that a is not NULL.
 *        2. Ensure that a_words is the real word length of a and a_words is not 0.
 *        3. Actually, n could be any value.
  */
FLAG_STATIC unsigned int big_div_2n_n_not_l32(unsigned int *a, unsigned int a_words, unsigned int n)
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

        if (0U != bits) // bits is in [1, 31]
        {
            ret = big_div_2n_n_l32(a, a_words - j, bits);
        } else // bits is 0
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
 * @brief           a = a / (2^n)
 * @param[in]       a                    - big integer a
 * @param[in]       a_wlen               - Word length of a
 * @param[in]       n                    - Exponent of 2^n
 * @return          Word length of a after division by 2^n
 * @note
 *        1. Ensure that a is not NULL.
 *        2. Ensure that a_wlen is the real word length of a.
 *        3. Ensure that a_wlen * 32 is not less than n.
  */
unsigned int big_div_2n(unsigned int *a, unsigned int a_wlen, unsigned int n)
{
    unsigned int ret = 0U;
    unsigned int a_words;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
#endif
        a_words = get_valid_words(a, a_wlen);

        if ((0U == n) || (0U == a_words)) {
            ret = a_words;
        } else if (n < 32U) // now a is not zero(a_words is not zero), and n is not
                            // zero either.
        {
            ret = big_div_2n_n_l32(a, a_words, n);
        } else // now a is not zero(a_words is not zero), and n is greater than 31
        {
            ret = big_div_2n_n_not_l32(a, a_words, n);
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    } else {
    }
#endif

    return ret;
}

/**
 * @brief           Check whether a is equal to 1 or not
 * @param[in]       a                    - Pointer to unsigned int big integer a
 * @param[in]       a_wlen               - Word length of big integer a
 * @return          1 (a is 1), 0 (a is not 1)
 * @note
 *        1. Ensure that a is not NULL.
  */
unsigned int bigint_check_1(const unsigned int *a, unsigned int a_wlen)
{
    unsigned int i;
    unsigned int ret = 0U;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
#endif
        if ((0U == a_wlen) || (a[0] != 1U)) {
            ret = 0U;
        } else {
            ret = 1U;
            for (i = 1U; i < a_wlen; i++) {
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
 * @brief           Check whether a is equal to p-1 or not
 * @param[in]       a                    - Pointer to unsigned int big integer a
 * @param[in]       p                    - Pointer to unsigned int big integer p, p must be odd
 * @param[in]       wlen                 - Word length of a and p
 * @return          1 (a is p-1), 0 (a is not p-1)
 * @note
 *        1. Ensure that neither a nor p is NULL.
 *        2. Ensure that p is odd.
  */
unsigned int bigint_check_p_1(const unsigned int *a, const unsigned int *p, unsigned int wlen)
{
    unsigned int i;
    unsigned int ret = 0U;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != a) && (NULL != p)) {
#endif
        if ((0U == wlen) || (a[0] != (p[0] - 1U))) {
            ret = 0U;
        } else {
            ret = 1U;
            for (i = 1U; i < wlen; i++) {
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

/**
 * @brief           Check if integer k is in range [1, n-1]
 * @param[in]       k                    - Big integer k
 * @param[in]       n                    - Big integer n
 * @param[in]       wlen                 - Word length of k and n
 * @param[in]       ret_zero             - Return value if k is zero
 * @param[in]       ret_big              - Return value if k >= n
 * @param[in]       ret_success          - Return value if k is in [1, n-1]
 * @return          One of the provided return values based on k's value
 * @note
 *        1. ret_zero: k is zero
 *        2. ret_big: k is greater than or equal to n
 *        3. ret_success: k is in [1, n-1]
 */
unsigned int uint32_integer_check(const unsigned int *k, const unsigned int *n, unsigned int wlen, unsigned int ret_zero, unsigned int ret_big, unsigned int ret_success)
{
    unsigned int ret;

    if (0U != uint32_bignum_check_zero(k, wlen)) {
        ret = ret_zero;
    } else if (uint32_big_num_cmp(k, wlen, n, wlen) >= 0) {
        ret = ret_big;
    } else {
        ret = ret_success;
    }

    return ret;
}

