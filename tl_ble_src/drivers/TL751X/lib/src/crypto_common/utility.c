/********************************************************************************************************
 * @file    utility.c
 *
 * @brief   This is the source file for TL751X
 *
 * @author  Driver Group
 * @date    2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "stimer.h"

#ifdef PKE_PRINT_BUF

/**
 * @brief       prints the contents of an array of unsigned characters, output in hexadecimal form.
 * @param[in]   buf              - an array of unsigned characters.
 * @param[in]   byteLen          - array length in bytes.
 * @param[in]   name             - the name or description of the array.
 * @return      none
 */
void print_buf_U8(unsigned char buf[], unsigned int byteLen, char name[])
{
    unsigned int i;

    printf("\r\n %s: ",name); fflush(stdout);
    for(i=0; i<byteLen; i++)
    {
        printf("%02x", buf[i]);
    }

    printf("\r\n");
}

/**
 * @brief       prints the contents of an array of unsigned characters, output in hexadecimal form.
 * @param[in]   buf              - an array of unsigned characters.
 * @param[in]   byteLen          - array length (in words).
 * @param[in]   name             - the name or description of the array.
 * @return      none
 */
void print_buf_U32(unsigned int buf[], unsigned int wordLen, char name[])
{
    unsigned int i;

    printf("\r\n %s: %08x\r\n",name, (unsigned int)buf);fflush(stdout);
    for(i=0; i<wordLen; i++)
    {
        printf("%08x", buf[i]);fflush(stdout);
    }

    printf("\r\n");fflush(stdout);
}

/**
 * @brief       prints the contents of an unsigned array of integers, output in hexadecimal form. Print in reverse order.
 * @param[in]   buf              - an array of unsigned characters.
 * @param[in]   byteLen          - array length in bytes.
 * @param[in]   name             - the name or description of the array.
 * @return      none
 */
void print_BN_buf_U32(unsigned int buf[], unsigned int wordLen, char name[])
{
    unsigned int i;

    printf("\r\n %08x %s: ", (unsigned int)buf, name);fflush(stdout);
    for(i=0; i<wordLen; i++)
    {
        printf("%08x", buf[wordLen-1-i]);
    }
    printf("\r\n");fflush(stdout);
}
#endif

static unsigned long start_tick = 0;
static unsigned long end_tick = 0;

/**
 * @brief       gets the current system clock period and stores it in the global variable start_tick.
 * @return      indicates the current system clock period
 */
unsigned int startP(void)
{
    start_tick = stimer_get_tick();

    return start_tick;
}

/**
 * @brief       calculate performance indicators at the end of the data transfer and output the results.
 * @param[in]   mode                 - data transfer mode (0 for CPU mode, non-0 for DMA mode).
 * @param[in]   once_bytes           - array length in bytes.
 * @param[in]   round                - the name or description of the array.
 * @return      none
 */
unsigned int endP(unsigned char mode, unsigned int once_bytes, unsigned int round)
{
    unsigned long total_bytes = 0;
    double delta_s = 0.0;
    double speed = 0.0;

    end_tick = stimer_get_tick();
    delta_s = (end_tick - start_tick) / (1000.0 * 1000 * SYSTEM_TIMER_TICK_1US) /* s */;

    total_bytes = once_bytes * round;

    speed = (total_bytes / delta_s) / (1024 * 1024); /* Mbyte/s */
    if (0 == mode)
        printf("\r\n CPU mode speed %f Mbyte/s\r\n", speed);
    else
        printf("\r\n DMA mode speed %f Mbyte/s\r\n", speed);

    printf("finished\r\n");
    return end_tick;
}

#if 0
/**
 * @brief       copies a block of memory from the source address to the destination address.
 * @param[out]  dst              - destination address where the data will be copied to.
 * @param[in]   src              - source address where the data will be copied from.
 * @param[in]   size             - number of bytes to be copied.
 * @return      none
 */
void memcpy_(void *dst, void *src, unsigned int size)
{
    unsigned char *a = (unsigned char *)dst;
    unsigned char *b = (unsigned char *)src;
#if 0
    while(size--)
    {
        *a++ = *b++;
    }
#else
    unsigned int *aa = (unsigned int *)dst;
    unsigned int *bb = (unsigned int *)src;
    unsigned int i, count, tmp;

    if((((unsigned int)dst) & 3) || (((unsigned int)src) & 3))
    {
        while(size--)
        {
            *a++ = *b++;
        }
    }
    else
    {
        count = size/4;
        for(i=0; i<count; i++)
        {
            *aa++ = *bb++;
        }

        tmp = size&3;
        if(tmp)
        {
            a+=(size&(~0x03));
            b+=(size&(~0x03));
            while(tmp--)
            {
                *a++ = *b++;
            }
        }
        else
        {;}
    }
#endif
}

/**
 * @brief Sets a block of memory to a specified value.
 * @param dst The starting address of the destination memory block.
 * @param value The value to be set.
 * @param size The size of the memory block (in bytes).
 * @return      none
 */
void memset_(void *dst, unsigned char value, unsigned int size)
{
    volatile unsigned char *a = (unsigned char *)dst;
#if 0
    while(size--)
    {
        *a++ = value;
    }
#else
    unsigned int i, count, tmp;

    tmp = ((unsigned int)dst) & 3;
    if(tmp)
    {
        if(size > 4-tmp)
        {
            for(i=0; i<4-tmp; i++)
            {
                *a++ = value;
            }
            size -= (4-tmp);
        }
        else
        {
            for(i=0; i<size; i++)
            {
                *a++ = value;
            }
            return;
        }
    }
    else
    {;}

    count = size/4;
    if(count)
    {
        tmp = value;
        tmp = (tmp<<8)|value;
        tmp = (tmp<<8)|value;
        tmp = (tmp<<8)|value;
        for(i=0; i<count; i++)
        {
            *((volatile unsigned int *)a) = tmp;
            a+=4;
        }
    }
    else
    {;}

    tmp = size&3;
    if(tmp)
    {
        for(i=0; i<tmp; i++)
        {
            *a++ = value;
        }
    }
    else
    {;}
#endif
}

/**
 * @brief       compares two blocks of memory byte by byte..
 * @param[in]   m1               - pointer to the first memory block.
 * @param[in]   m2               - pointer to the second memory block.
 * @param[in]   size             - number of bytes to be compared.
 * @return      Returns an integer value indicating the result of the comparison:
 *         - Negative value if the first differing byte in m1 is less than the corresponding byte in m2
 *         - Zero if all bytes in both blocks are equal
 *         - Positive value if the first differing byte in m1 is greater than the corresponding byte in m2
 */
char memcmp_(void *m1, void *m2, unsigned int size)
{
    char *a = (char *)m1;
    char *b = (char *)m2;
    char c;

    while(size--)
    {
        c = (*a++ - *b++);
        if(c)
        {
            return c;
        }
        else
        {;}
    }

    return 0;
}
#endif

/**
 * @brief       set uint32 buffer.
 * @param[out]  a                - output word buffer.
 * @param[in]   value            - input word value.
 * @param[in]   wordLen          - word length of buffer a.
 * @return      none
 */
void uint32_set(volatile unsigned int *a, unsigned int value, unsigned int wordLen)
{
    while(wordLen)
    {
        a[--wordLen] = value;
    }
}

/**
 * @brief       copy uint32 buffer.
 * @param[out]  dst          - output word buffer.
 * @param[in]   src          - input word buffer.
 * @param[in]   wordLen      - word length of buffer dst or src.
 * @return      none
 */
void uint32_copy(volatile unsigned int *dst, volatile const unsigned int *src, unsigned int wordLen)
{
    unsigned int i;

    if(dst != src)
    {
        for(i=0; i<wordLen; i++)
        {
            dst[i] = src[i];
        }
    }
    else
    {;}
}

/**
 * @brief       copy uint32 buffer.
 * @param[in/out]   a            - word buffer a.
 * @param[in]   aWordLen         - word length of buffer a.
 * @return      none
 */
void uint32_clear(volatile unsigned int *a, unsigned int wordLen)
{
#if 1
    volatile unsigned int i = wordLen;

    while(i)
    {
        a[--i] = 0;
    }
#else
    volatile unsigned int i = 0;
    for(i=0;i<wordLen;i++)
    {
        a[i] = 0;
    }
#endif
}

/**
 * @brief       delay execution for a specified number of iterations.
 * @return      none
 */
static void uint32_sleep1(unsigned int count)
{
    volatile unsigned int a=0;
    volatile unsigned int b=0;
    volatile unsigned int result=0;
    volatile unsigned int i;

    for(i=0;i<count;i++)
    {
        result |= ((a+i) - (b+i));
    }
}

/**
 * @brief       delay execution for a specified number of iterations using bitwise XOR operation.
 * @return      none
 */
static void uint32_sleep2(unsigned int count)
{
    volatile unsigned int a=0;
    volatile unsigned int b=0;
    volatile unsigned int result=0;
    volatile unsigned int i;

    for(i=0;i<count;i++)
    {
        result |= ((a+i) ^ (b+i));
    }
}

/**
 * @brief       sleep for a while.
 * @param[in]   count         - count.
 * @return      none
 */
void uint32_sleep(unsigned int count, unsigned char rand)
{
    unsigned char rand1 = rand & 0x01;

    if(0 == rand1)
    {
        uint32_sleep1(count);
    }
    else
    {
        uint32_sleep2(count);
    }
}

#if 0
/**
 * @brief       convert 0x1122334455667788 to 0x4433221188776655.
 * @param[in]   in         - source address.
 * @param[in]   out        - destination address.
 * @param[in]   wordLen    - word length of in/out.
 * @return      none
 */
void uint32_endian_reverse(unsigned char *in, unsigned char *out, unsigned int wordLen)
{
    unsigned char tmp;

    if(in == out)
    {
        while(wordLen>0)
        {
            tmp=*in;
            *in=*(in+3);
            *(in+3)=tmp;
            in+=1;
            tmp=*in;
            *in=*(in+1);
            *(in+1)=tmp;
            wordLen--;
            in+=3;
        }
    }
    else
    {
        while(wordLen>0)
        {
            *(out)   = *(in+3);
            *(out+1) = *(in+2);
            *(out+2) = *(in+1);
            *(out+3) = *(in);
            wordLen--;
            in += 4;
            out += 4;
        }
    }
}

/**
 * @brief       reverse word array.
 * @param[in]   in         - input buffer.
 * @param[in]   out        - output buffer.
 * @param[in]   wordLen    - word length of in or out.
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

    if(((unsigned int)(in))&3)
    {
        memcpy_(out, in, wordLen<<2);
        p_in = out;
    }
    else
    {
        p_in = (unsigned int *)in;
    }

    for (idx = 0; idx < round; idx++)
    {
        tmp = p_in[idx];
        out[idx] = p_in[wordLen - 1 - idx];
        out[wordLen - 1 - idx] = tmp;
    }

    if ((wordLen & 0x1) && (p_in != out))
    {
        out[round] = p_in[round];
    }
    else
    {;}
}

#endif

/**
 * @brief       reverse word array.
 * @param[in]   in         - input buffer.
 * @param[in]   out        - output buffer.
 * @param[in]   wordLen    - word length of in or out.
 * @return      none
 * @note
  @verbatim
      -# 1. in and out could point the same buffer.
  @endverbatim
  */
void reverse_byte_array(volatile const unsigned char *in, unsigned char *out, unsigned int byteLen)
{
    unsigned int idx, round = byteLen >> 1;
    unsigned char tmp;

    for (idx = 0; idx < round; idx++)
    {
        tmp = in[idx];
        out[idx] = in[byteLen - 1 - idx];
        out[byteLen - 1 - idx] = tmp;
    }

    if ((byteLen & 0x1) && (in != out))
    {
        out[round] = in[round];
    }
    else
    {;}
}

#if 0
/**
 * @brief       reverse word array.
 * @param[in]   in         - input byte buffer.
 * @param[in]   out        - output word buffer.
 * @param[in]   bytelen    - byte length of buffer in or out.
 * @return      none
 * @note
   @verbatim
      -# 1. byteLen must be multiple of 4.
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

/**
 * @brief       reverse word order.
 * @param[in]   in              - input word buffer.
 * @param[out]  out             - output word buffer.
 * @param[in]   wordLen         - word length of buffer in or out.
 * @param[in]   reverse_word    - whether to reverse byte order in every word, 0:no, other:yes.
 * @return      none
 * @note
   @verbatim
      -# 1. in DAM mode, the memory may be accessed by words, not by bytes, this function is designed
        for the case.
   @endverbatim
*/
void dma_reverse_word_array(unsigned int *in, unsigned int *out, unsigned int wordLen, unsigned int reverse_word)
{
    unsigned int i, j;
    unsigned int tmp;
    unsigned int *p=out;

    if(in == out)
    {
        for(i=0; i<wordLen; i+=4)
        {
            for (j = 0; j < 2; j++)
            {
                tmp = p[j];
                p[j] = p[4 - 1 - j];
                p[4 - 1 - j] = tmp;
            }
            p+=4;
        }
    }
    else
    {
        for(i=0; i<wordLen; i+=4)
        {
            p[0] = in[3];
            p[1] = in[2];
            p[2] = in[1];
            p[3] = in[0];
            p+=4;
            in+=4;
        }
    }

    if(reverse_word)
    {
        for (i = 0; i < wordLen; i++)
        {
            tmp = *out;
            *out = tmp&0xFF;
            *out <<= 8;
            *out |= (tmp>>8)&0xFF;
            *out <<= 8;
            *out |= (tmp>>16)&0xFF;
            *out <<= 8;
            *out |= (tmp>>24)&0xFF;

            out++;
        }
    }
    else
    {;}
}
#endif

/**
 * @brief       C = A XOR B.
 * @param[in]   A         - byte buffer a.
 * @param[in]   B         - byte buffer b.
 * @param[out]  C         - C = A XOR B.
 * @return      none
 */
void uint8_XOR(unsigned char *A, unsigned char *B, unsigned char *C, unsigned int byteLen)
{
    unsigned int i;

    for(i=0; i<byteLen; i++)
    {
        C[i] = A[i] ^ B[i];
    }
}

/**
 * @brief       C = A XOR B.
 * @param[in]   A         - word buffer a.
 * @param[in]   B         - word buffer b.
 * @param[out]  C         - C = A XOR B.
 * @param[in]   byteLen   - word length of A,B,C.
 * @return      none
 */
void uint32_XOR(unsigned int *A, unsigned int *B, unsigned int *C, unsigned int wordLen)
{
    unsigned int i;

    for(i=0; i<wordLen; i++)
    {
        C[i] = A[i] ^ B[i];
    }
}

/**
 * @brief       get real bit length of big number a of wordLen words.
 * @param[in]   a         - pointer to the array of unsigned integers.
 * @param[in]   wordLen   - length of the array in words (unsigned integers).
 * @return      the number of valid bits in the array, or 0 if the array is empty or all elements are zero
 */
unsigned int get_valid_bits(const unsigned int *a, unsigned int wordLen)
{
    unsigned int i = 0;
    unsigned int j = 0;

    if(0 == wordLen)
    {
        return 0;
    }
    else
    {;}

    for (i = wordLen; i > 0; i--)
    {
        if (a[i - 1])
        {
            break;
        }
        else
        {;}
    }

    if(0 == i)
    {
        return 0;
    }
    else
    {;}

    for (j = 32; j > 0; j--)
    {
        if (a[i - 1] & (((unsigned int)0x1) << (j - 1)))
        {
            break;
        }
        else
        {;}
    }

    return ((i - 1) << 5) + j;
}

/**
 * @brief       get real word length of big number a of max_words words.
 * @param[in]   a           - big integer a.
 * @param[in]   max_words   - max word length of a.
 * @return      real word length of big number a
 */
unsigned int get_valid_words(volatile const unsigned int *a, unsigned int max_words)
{
    unsigned int i;

    for (i = max_words; i > 0; i--)
    {
        if (a[i - 1])
        {
            return i;
        }
        else
        {;}
    }

    return 0;
}

/**
 * @brief       check whether big number or unsigned char buffer a is all zero or not.
 * @param[in]   a           - byte buffer a.
 * @param[in]   aByteLen    - byte length of a.
 * @return      0(a is not zero),1(a is all zero)
 */
unsigned char uint8_BigNum_Check_Zero(unsigned char a[], unsigned int aByteLen)
{
    unsigned int i;

    for(i=0; i<aByteLen; i++)
    {
        if(a[i])
        {
            return 0;
        }
        else
        {;}
    }

    return 1;
}

/**
 * @brief       check whether big number or unsigned int buffer a is all zero or not.
 * @param[in]   a           - big integer or word buffer a.
 * @param[in]   aByteLen    - word length of a.
 * @return      0(a is not zero), 1(a is all zero)
 */
unsigned int uint32_BigNum_Check_Zero(unsigned int a[], unsigned int aWordLen)
{
    unsigned int i;

    for(i=0; i<aWordLen; i++)
    {
        if(a[i])
        {
            return 0;
        }
        else
        {;}
    }

    return 1;
}

/**
 * @brief       compare big integer a and b.
 * @param[in]   a           - big integer a.
 * @param[in]   aWordLen    - word length of a.
 * @param[in]   b           - big integer b.
 * @param[in]   bWordLen    - word length of b.
 * @return      0:a=b,   1:a>b,   -1: a<b
 */
int uint32_BigNumCmp(volatile unsigned int *a, unsigned int aWordLen, volatile const unsigned int *b, unsigned int bWordLen)
{
    int i;

    aWordLen = get_valid_words(a, aWordLen);
    bWordLen = get_valid_words(b, bWordLen);

    if(aWordLen > bWordLen)
    {
        return 1;
    }
    else if(aWordLen < bWordLen)
    {
        return -1;
    }
    else
    {;}

    for(i=(aWordLen-1);i>=0;i--)
    {
        if(a[i] > b[i])
        {
            return 1;
        }
        else if(a[i] < b[i])
        {
            return -1;
        }
        else
        {;}
    }

    return 0;
}

/**
 * @brief       for a = b*2^t, b is odd, get t.
 * @param[in]   a           - big integer a.
 * @return      number of multiple by 2, for a
 * @note
   @verbatim
      -# 1. make sure a != 0.
   @endverbatim
 */
unsigned int Get_Multiple2_Number(unsigned int a[])
{
    unsigned int t, i=0, j=0;

    while(0 == (a[i]))
    {
        i++;
    }

    t = a[i];
    while(!(t&1))
    {
        j++;
        t>>=1;
    }

    return (i<<5)+j;
}

/**
 * @brief       a = a/(2^n).
 * @param[in]   a                  - big integer a.
 * @param[in]   aWordLen           - word length of a.
 * @param[in]   n                  - exponent of 2^n.
 * @return      word length of a = a/(2^n)
 * @note
   @verbatim
      -# 1. make sure aWordLen is real word length of a.
      -# 2. to make sure aWordLen-1 is available, so data type of aWordLen is int, not unsigned int.
      -# 3. please make sure aWordLen*32 is not less than n.
   @endverbatim
 */
unsigned int Big_Div2n(unsigned int a[], int aWordLen, unsigned int n)
{
    int i;
    unsigned int j;

    aWordLen = get_valid_words(a, aWordLen);

    if(0 == n)
    {
        return aWordLen;
    }
    else if(!aWordLen)
    {
        return 0;
    }
    else
    {;}

    //now a is not zero(aWordLen is not zero), and n is not zero either.

    if(n<32)
    {
        for(i=0; i<aWordLen-1; i++)
        {
            a[i] >>= n;
            a[i] |= (a[i+1]<<(32-n));
        }
        a[i] >>= n;

        if(!a[i])
        {
            return i;
        }
        else
        {
            return aWordLen;
        }
    }
    else
    {;}

    j=n>>5; //j=n/32;
    n&=31;  //n=n%32;

    if((int)j<aWordLen)
    {
        if(n)   //n is in [1, 31]
        {
            for(i=0; i<aWordLen-(int)j-1; i++)
            {
                a[i] = a[i+j]>>n;
                a[i] |= (a[i+j+1]<<(32-n));
            }
            a[i] = a[i+j]>>n;
            uint32_clear(a+aWordLen-j, j);

            if(!a[i])
            {
                return i;
            }
            else
            {
                return aWordLen-j;
            }
        }
        else    //n is 0
        {
            for(i=0; i<aWordLen-(int)j; i++)
            {
                a[i] = a[i+j];
            }
            uint32_clear(a+aWordLen-j, j);

            return aWordLen-j;
        }
    }
    else
    {
        uint32_clear(a, aWordLen);
        return 0;
    }
}

/**
 * @brief       check whether a is equal to 1 or not.
 * @param[in]   a                  - pointer to unsigned int big integer a.
 * @param[in]   aWordLen           - word length of big integer a.
 * @return      1(a is 1), 0(a is not 1)
 * @note
   @verbatim
      -# 1. make sure aWordLen is real word length of a.
      -# 2. to make sure aWordLen-1 is available, so data type of aWordLen is int, not unsigned int.
      -# 3. please make sure aWordLen*32 is not less than n.
   @endverbatim
 */
unsigned char Bigint_Check_1(volatile unsigned int a[], unsigned int aWordLen)
{
    unsigned int i;

    if(!aWordLen)
    {
        return 0;
    }
    else if(a[0] != 1)
    {
        return 0;
    }
    else
    {;}

    for(i=1; i<aWordLen; i++)
    {
        if(a[i])
        {
            return 0;
        }
        else
        {;}
    }

    return 1;
}

/**
 * @brief       check whether a is equal to p-1 or not.
 * @param[in]   a                  - pointer to unsigned int big integer a.
 * @param[in]   p                  - pointer to unsigned int big integer p, p must be odd.
 * @param[in]   aWordLen           - word length of a and p.
 * @return      1(a is 1), 0(a is not 1)
 * @note
   @verbatim
      -# 1. make sure p is odd.
   @endverbatim
 */
unsigned char Bigint_Check_p_1(unsigned int a[], unsigned int p[], unsigned int wordLen)
{
    unsigned int i;

    if(!wordLen)
    {
        return 0;
    }
    else if(a[0] != p[0] - 1)
    {
        return 0;
    }
    else
    {;}

    for(i=1; i<wordLen; i++)
    {
        if(a[i] != p[i])
        {
            return 0;
        }
        else
        {;}
    }

    return 1;
}

/**
 * @brief       check whether integer k is in [1, n-1].
 * @param[in]   k                  - big number k.
 * @param[in]   n                  - big number n.
 * @param[in]   wordLen            - word length of k and n.
 * @return      ret_zero(k is zero)   ret_big(k is greater/bigger than or equal to n)  ret_success(k is in [1, n-1])
 */
unsigned int uint32_integer_check(unsigned int *k, const unsigned int *n, unsigned int wordLen, unsigned int ret_zero, unsigned int ret_big,
        unsigned int ret_success)
{
    if(uint32_BigNum_Check_Zero(k, wordLen))
    {
        return ret_zero;
    }
    else if(uint32_BigNumCmp(k, wordLen, n, wordLen) >= 0)
    {
        return ret_big;
    }
    else
    {;}

    return ret_success;
}

