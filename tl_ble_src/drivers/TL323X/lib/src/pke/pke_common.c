/*! @file pke_common.c */
#include "lib/include/pke/pke_common.h"

#include "lib/include/crypto_common/utility.h"

/**
 * @brief           Load input operand to baseaddr
 * @param[out]      baseaddr             - destination data
 * @param[in]       data                 - source data
 * @param[in]       wlen                 - word length of data
 * @return          None
 * @note
 *        1. Operands are both unsigned int little-endian.
 */
void pke_load_operand(unsigned int *baseaddr, const unsigned int *data, unsigned int wlen)
{
    unsigned int i;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != baseaddr) && (NULL != data))
    {
#endif
        if (baseaddr != data)
        {
            for (i = 0; i < wlen; i++)
            {
                ((volatile unsigned int *)baseaddr)[i] = data[i];
            }
        }
        else
        {
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    }
    else
    {
    }
#endif
}

/**
 * @brief           Load input operand of 256 bits to baseaddr
 * @param[out]      baseaddr             - destination data
 * @param[in]       data                 - source data
 * @return          None
 * @note
 *        1. Operands are both unsigned int little-endian.
 *        2. Operands are both of 256 bits for SM2, SM9, etc.
 */
void pke_load_operand_256bits(unsigned int *baseaddr, const unsigned int *data)
{
#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != baseaddr) && (NULL != data))
    {
#endif
        *((volatile unsigned int *)(&baseaddr[0])) = data[0];
        *((volatile unsigned int *)(&baseaddr[1])) = data[1];
        *((volatile unsigned int *)(&baseaddr[2])) = data[2];
        *((volatile unsigned int *)(&baseaddr[3])) = data[3];
        *((volatile unsigned int *)(&baseaddr[4])) = data[4];
        *((volatile unsigned int *)(&baseaddr[5])) = data[5];
        *((volatile unsigned int *)(&baseaddr[6])) = data[6];
        *((volatile unsigned int *)(&baseaddr[7])) = data[7];
#ifdef SUPPORT_STATIC_ANALYSIS
    }
    else
    {
    }
#endif
}

/**
 * @brief           Get result operand from baseaddr
 * @param[in]       baseaddr             - source data
 * @param[out]      data                 - destination data
 * @param[in]       wlen                 - word length of data
 * @return          None
 * @note
 *        1. Operands are both unsigned int little-endian.
 */
void pke_read_operand(const unsigned int *baseaddr, unsigned int *data, unsigned int wlen)
{
    unsigned int i;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != baseaddr) && (NULL != data))
    {
#endif
        if (baseaddr != data)
        {
            for (i = 0; i < wlen; i++)
            {
                data[i] = baseaddr[i];
            }
        }
        else
        {
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    }
    else
    {
    }
#endif
}

/**
 * @brief           Get result operand of 256 bits from baseaddr
 * @param[in]       baseaddr             - source data
 * @param[out]      data                 - destination data
 * @return          None
 * @note
 *        1. Operands are both unsigned int little-endian.
 *        2. Operands are both of 256 bits for SM2, SM9, etc.
 */
void pke_read_operand_256bits(const unsigned int *baseaddr, unsigned int *data)
{
#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != baseaddr) && (NULL != data))
    {
#endif
        data[0] = baseaddr[0];
        data[1] = baseaddr[1];
        data[2] = baseaddr[2];
        data[3] = baseaddr[3];
        data[4] = baseaddr[4];
        data[5] = baseaddr[5];
        data[6] = baseaddr[6];
        data[7] = baseaddr[7];
#ifdef SUPPORT_STATIC_ANALYSIS
    }
    else
    {
    }
#endif
}

/**
 * @brief           Load input operand (unsigned char big-endian) to baseaddr
 * @param[in]       baseaddr             - destination data
 * @param[out]      data                 - source data, unsigned char big-endian
 * @param[in]       byteLen              - byteLen Input, byte length of data
 * @return          None
 */
void pke_load_operand_U8(unsigned int *baseaddr, const unsigned char *data, unsigned int byteLen)
{
    unsigned int *dst;
    unsigned int bytes;
    unsigned int t, i;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != data)
    {
#endif
        if ((unsigned char *)baseaddr != data)
        {
            dst = baseaddr;
            bytes = byteLen;

            for (; bytes > 3u; bytes -= 4u)
            {
                t = (unsigned int)(data[bytes - 1u]);
                t |= ((unsigned int)(data[bytes - 2u])) << 8;
                t |= ((unsigned int)(data[bytes - 3u])) << 16;
                t |= ((unsigned int)(data[bytes - 4u])) << 24;

                *((volatile unsigned int *)(dst)) = t;

                dst = &dst[1];
            }

            if (0U != bytes)
            {
                t = 0U;
                for (i = 0U; i < bytes; i++)
                {
                    t |= ((unsigned int)(data[bytes - 1u - i])) << (i << 3);
                }

                *((volatile unsigned int *)(dst)) = t;
            }
            else
            {
            }
        }
        else
        {
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    }
    else
    {
    }
#endif
}

/**
 * @brief           Get result operand (unsigned char big-endian) from baseaddr
 * @param[in]       baseaddr             - source data
 * @param[out]      data                 - destination data, unsigned char big-endian
 * @param[in]       byteLen              - byte length of data
 * @return          None
 */
void pke_read_operand_U8(const unsigned int *baseaddr, unsigned char *data, unsigned int byteLen)
{
    const unsigned int *src;
    unsigned int bytes;
    unsigned int t, i;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != data)
    {
#endif
        if ((const unsigned char *)baseaddr != data)
        {
            src = baseaddr;
            bytes = byteLen;

            for (; bytes > 3u; bytes -= 4u)
            {
                t = *(src);

                data[bytes - 1u] = (unsigned char)((t) & 0xFFu);
                data[bytes - 2u] = (unsigned char)((t >> 8) & 0xFFu);
                data[bytes - 3u] = (unsigned char)((t >> 16) & 0xFFu);
                data[bytes - 4u] = (unsigned char)((t >> 24) & 0xFFu);

                src = &src[1];
            }

            if (0U != bytes)
            {
                t = *(src);

                for (i = 0U; i < bytes; i++)
                {
                    data[bytes - 1u - i] = (unsigned char)((t >> (i << 3)) & 0xFFu);
                }
            }
        }
        else
        {
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    }
    else
    {
    }
#endif
}

/**
 * @brief           Set operand with an unsigned int value
 * @param[out]      baseaddr             - operand
 * @param[in]       wlen                 - word length of operand
 * @param[in]       b                    - unsigned int value b
 * @return          None
 * @note
 *        1. wlen cannot be 0.
 */
void pke_set_operand_uint32_value(unsigned int *baseaddr, unsigned int wlen, unsigned int b)
{
    unsigned int i = wlen;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != baseaddr)
    {
#endif
        while (i > 1U)
        {
            i--;
            *((volatile unsigned int *)(&baseaddr[i])) = 0U;
        }

        *((volatile unsigned int *)(&baseaddr[0])) = b;
#ifdef SUPPORT_STATIC_ANALYSIS
    }
    else
    {
    }
#endif
}

/**
 * @brief           Set operand of 256 bits with an unsigned int value
 * @param[out]      baseaddr             - operand
 * @param[in]       b                    - unsigned int value b
 * @return          None
 * @note
 *        1. Operand is unsigned int little-endian.
 *        2. Operand is of 256 bits for SM2, SM9, etc.
 */
void pke_set_operand_uint32_value_256bits(unsigned int *baseaddr, unsigned int b)
{
#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != baseaddr)
    {
#endif
        *((volatile unsigned int *)(&baseaddr[0])) = b;
        *((volatile unsigned int *)(&baseaddr[1])) = 0u;
        *((volatile unsigned int *)(&baseaddr[2])) = 0u;
        *((volatile unsigned int *)(&baseaddr[3])) = 0u;
        *((volatile unsigned int *)(&baseaddr[4])) = 0u;
        *((volatile unsigned int *)(&baseaddr[5])) = 0u;
        *((volatile unsigned int *)(&baseaddr[6])) = 0u;
        *((volatile unsigned int *)(&baseaddr[7])) = 0u;
#ifdef SUPPORT_STATIC_ANALYSIS
    }
    else
    {
    }
#endif
}

/**
 * @brief           Check whether k is equal to (n-1), here n is odd.
 * @param[in]       k                    - big number k
 * @param[in]       n                    - big number n
 * @param[in]       words                - word length of k and n
 * @return          0 if k is n-1, other value if k is not n-1
 * @note
 *        1. n must be odd.
 */
unsigned int is_k_equal_to_n_minus_1(const unsigned int *k, const unsigned int *n, unsigned int words)
{
    unsigned int ret = 1u;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != k) && (NULL != n))
    {
#endif
        if (k[0] == (n[0] - 1u))
        {
            ret = 0u;

            if (words > 1u)
            {
                if (0 != uint32_big_num_cmp(&k[1], words - 1u, &n[1], words - 1u))
                {
                    ret = 1u;
                }
                else
                {
                }
            }
            else
            {
            }
        }
        else
        {
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    }
    else
    {
    }
#endif

    return ret;
}
