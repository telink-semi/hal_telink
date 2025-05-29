/********************************************************************************************************
 * @file    rsa.c
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
#include "lib/include/pke/pke.h"
#include "lib/include/pke/pke_prime.h"
#include "lib/include/trng/trng.h"
#include "lib/include/pke/rsa.h"
#include "lib/include/crypto_common/utility.h"

#ifdef SUPPORT_RSA

/**
 * @brief           out = a^e mod n.
 * @param[in]       a                    - unsigned int big integer a, base number, make sure a < n.
 * @param[in]       e                    - unsigned int big integer e, exponent, make sure e < n.
 * @param[in]       n                    - unsigned int big integer n, modulus, make sure n is odd.
 * @param[out]      out                  - out = a^e mod n.
 * @param[in]       eBitLen              - real bit length of unsigned int big integer e.
 * @param[in]       nBitLen              - real bit length of unsigned int big integer n.
 * @return          0:success     other:error
 * @note
 *        1.a, n, and out have the same word length:((nBitLen+31)>>5); and e word length is (eBitLen+31)>>5.
 */
unsigned int RSA_ModExp(const unsigned int *a, const unsigned int *e, const unsigned int *n, unsigned int *out, unsigned int eBitLen, unsigned int nBitLen)
{
    unsigned int e_wlen = get_word_len(eBitLen);
    unsigned int n_wlen = get_word_len(nBitLen);
    unsigned int ret;

    if ((NULL == a) || (NULL == e) || (NULL == n) || (NULL == out))
    {
        return RSA_BUFFER_NULL;
    }
    else if ((nBitLen > RSA_MAX_BIT_LEN) || (eBitLen > nBitLen))
    {
        return RSA_INPUT_TOO_LONG;
    }
    else if ((0u == nBitLen) || (0u == (n[0] & 1u)))
    {
        return RSA_INPUT_INVALID;
    }
    else
    {
        // handle other
    }

    ret = pke_modexp_check_input((const unsigned int *)n, (const unsigned int *)e, (const unsigned int *)a, out, n_wlen, e_wlen);
    if (PKE_FINISHED == ret)
    {
        return RSA_SUCCESS;
    }
    else if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        // handle other
    }

    ret = pke_pre_calc_mont_for_modexp(n, nBitLen, NULL, NULL);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    return pke_modexp(n, e, a, out, n_wlen, e_wlen);
}

/**
 * @brief           out = a^d mod n, here d represents RSA CRT private key (p,q,dp,dq,u).
 * @param[in]       a                    - unsigned int big integer a, base number, make sure a < n=pq.
 * @param[in]       p                    - unsigned int big integer p, prime number, one part of private key (p,q,dp,dq,u).
 * @param[in]       q                    - unsigned int big integer q, prime number, one part of private key (p,q,dp,dq,u).
 * @param[in]       dp                   - unsigned int big integer dp = e^(-1) mod (p-1), one part of private key (p,q,dp,dq,u).
 * @param[in]       dq                   - unsigned int big integer dq = e^(-1) mod (q-1), one part of private key (p,q,dp,dq,u).
 * @param[in]       u                    - unsigned int big integer u = q^(-1) mod p, one part of private key (p,q,dp,dq,u).
 * @param[in]       out                  - out = a^d mod n.
 * @param[out]      nBitLen              - real bit length of unsigned int big integer n=pq.
 * @return          0:success     other:error
 * @note
 *        1.a and out have the same word length:((nBitLen+31)>>5); and p,p_h,q,q_h,dp,dq,u have the same word length:((nBitLen/2+31)>>5).
 */
unsigned int RSA_CRTModExp(const unsigned int *a, const unsigned int *p, const unsigned int *q, const unsigned int *dp, const unsigned int *dq, const unsigned int *u,
                           unsigned int *out, unsigned int nBitLen)
{
    unsigned int buf[RSA_MAX_WORD_LEN];
    unsigned int *m1 = buf;
    unsigned int *m2 = &buf[RSA_MAX_WORD_LEN >> 1];
    unsigned int *tmp_out;
    unsigned int tmp_step;
    unsigned int n_wlen = get_word_len(nBitLen);
    unsigned int p_bitlen = nBitLen >> 1;
    unsigned int p_wlen = get_word_len(p_bitlen);
    int32_t flag;
    unsigned int ret;

    if ((NULL == a) || (NULL == p) || (NULL == q) || (NULL == dp) || (NULL == dq) || (NULL == u) || (NULL == out))
    {
        return RSA_BUFFER_NULL;
    }
    else if (nBitLen > RSA_MAX_BIT_LEN)
    {
        return RSA_INPUT_TOO_LONG;
    }
    else if ((0u == nBitLen) || (0u != (nBitLen & 1u)))
    {
        return RSA_INPUT_INVALID;
    }
    else
    {
        // handle other
    }

    // get n = p*q
    ret = pke_mul(p, q, buf, p_wlen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // a should be in [0,n]
    flag = uint32_big_num_cmp(a, n_wlen, buf, n_wlen);
    if (flag > 0)
    {
        return RSA_INPUT_INVALID;
    }
    else if ((1u == uint32_bignum_check_zero(a, n_wlen)) || (0 == flag)) // if a is 0 or n, the output is 0
    {
        uint32_clear(out, n_wlen);
        return RSA_SUCCESS;
    }
    else
    {
        // handle other
    }

    // do pke_pre_calc_mont() first, because a may be less than p or q, then
    // pke_mod() will not call pke_pre_calc_mont() inside, but pke_modexp() needs
    // the output of pke_pre_calc_mont().

    // m2 = (a) mod q
    ret = pke_pre_calc_mont_for_modexp(q, p_bitlen, NULL, NULL);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // get the p_bitlen step
    tmp_step = pke_get_operand_bytes();

#if (defined(PKE_LP) || defined(PKE_SECURE))
    ret = pke_mod(a, n_wlen, q, (unsigned int *)(rPKE_A(3u, tmp_step)), (unsigned int *)(rPKE_B(4u, tmp_step)), p_wlen, m2);
#else
    ret = pke_mod(a, n_wlen, q, (unsigned int *)(rPKE_B(0u, tmp_step)), p_wlen, m2);
#endif
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // m2 = (a)^dq mod q
    ret = pke_modexp(q, dq, m2, m2, p_wlen, p_wlen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // m1 = (a) mod p
    ret = pke_pre_calc_mont_for_modexp(p, p_bitlen, NULL, NULL);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

#if (defined(PKE_LP) || defined(PKE_SECURE))
    ret = pke_mod(a, n_wlen, p, (unsigned int *)(rPKE_A(3u, tmp_step)), (unsigned int *)(rPKE_B(4u, tmp_step)), p_wlen, m1);
#else
    ret = pke_mod(a, n_wlen, p, (unsigned int *)(rPKE_B(0u, tmp_step)), p_wlen, m1);
#endif
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // m1 = (a)^dp mod p
    ret = pke_modexp(p, dp, m1, m1, p_wlen, p_wlen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

#if (defined(PKE_LP) || defined(PKE_SECURE))
    tmp_out = (unsigned int *)(rPKE_B(0u, tmp_step));
#else
    tmp_out = (unsigned int *)(rPKE_B(1u, tmp_step));
#endif

    // m1 = (m1-m2) mod p
    if (uint32_big_num_cmp(m2, p_wlen, p, p_wlen) >= 0)
    {
        // if m2 >= p, get tmp_out = m2 mod p
        ret = pke_sub(m2, p, tmp_out, p_wlen);
        if (PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
        }

        ret = pke_modsub(p, m1, tmp_out, m1, p_wlen);
    }
    else
    {
        ret = pke_modsub(p, m1, m2, m1, p_wlen);
    }

    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

// m1 = h = u*(m1-m2) mod p
#if 1
#if (defined(PKE_LP) || defined(PKE_SECURE))
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
#endif
    ret = pke_modmul_internal(m1, u, m1, p_wlen);
#else
    ret = pke_modmul(p, m1, u, m1, p_wlen);
#endif
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // store the nBitLen step
    tmp_step = pke_set_operand_width(nBitLen);

    // A1 = hq
    ret = pke_mul(m1, q, (unsigned int *)(rPKE_A(1u, tmp_step)), p_wlen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // out = m2+hq
    uint32_copy((unsigned int *)(rPKE_B(1u, tmp_step)), m2, p_wlen);
    uint32_clear((unsigned int *)(&(rPKE_B(1u, tmp_step))[p_wlen]), n_wlen - p_wlen);
    return pke_add((unsigned int *)(rPKE_A(1u, tmp_step)), (unsigned int *)(rPKE_B(1u, tmp_step)), out, n_wlen);
}

/**
 * @brief           get big odd integer e of eBitLen
 * @param[in]       e                    - unsigned int big odd integer e.
 * @param[in]       eBitLen              - bit length of unsigned int big odd integer e.
 * @return          0:success     1:error
 * @note
 *        1.eBitLen must be big than 1.
 */
FLAG_STATIC unsigned int RSA_Get_E1(unsigned int e[], unsigned int eBitLen)
{
    unsigned int bits = 0u;
    unsigned int e_wlen = 0u;
    unsigned int ret;

    if ((eBitLen < 2u) || (eBitLen > RSA_MAX_BIT_LEN)) // just for static analysis.
    {
        ret = RSA_INPUT_INVALID;
    }
    else
    {
        e_wlen = (eBitLen + 0x1Fu) >> 5;

        ret = get_rand((unsigned char *)e, e_wlen << 2);
        if (TRNG_SUCCESS == ret)
        {
            ret = PKE_SUCCESS;
        }
        else
        {
        }
    }

    if (PKE_SUCCESS == ret)
    {
        bits = eBitLen & 31u;

        if (0u != bits)
        {
#if 0
        e[e_wlen - 1u] <<= (32u - eBitLen);
        e[e_wlen - 1u] |= 0x80000000u;
        e[e_wlen - 1u] >>= (32u - eBitLen);
#else
            e[e_wlen - 1u] &= (((unsigned int)1u) << (bits)) - 1u;
            e[e_wlen - 1u] |= ((unsigned int)1u) << (bits - 1u);
#endif
        }
        else
        {
            e[e_wlen - 1u] |= 0x80000000u;
        }

        e[0] |= 0x01u; // make e odd
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           get big odd integer e of eBitLen, satisfies e < fai_n of bitLen.
 * @param[in]       e                    - unsigned int big odd integer e.
 * @param[in]       fai_n                - unsigned int big even integer fai_n.
 * @param[in]       bitLen               - bit length of unsigned int big odd integer e and n.
 * @return          0:success     1:error(error: bitLen<66), 2(error, n is 1000000000...000000)
 * @note
 *        1.eBitLen must be big than 65.
 *        2.n can not be 1000000000...000000.
 */
FLAG_STATIC unsigned int RSA_Get_E2(unsigned int *e, const unsigned int *fai_n, unsigned int bitLen)
{
    unsigned int ret;
    unsigned int i, bits;
    unsigned char j;

    if (bitLen < 66u)
    {
        ret = RSA_INPUT_INVALID;
    }
    else
    {
        ret = RSA_Get_E1(e, bitLen);
    }

    if (PKE_SUCCESS == ret)
    {
        bits = bitLen - 1u;
        i = (bits + 0x1Fu) >> 5;         // i is 1 plus the word index of the word where the
                                         // second highest bit is located
        j = (unsigned char)(bits & 31u); // j is the bit length up to the second
                                         // highest bit in the targeted word
        if ((unsigned char)0 == j)
        {
            j = (unsigned char)32;
        }
        else
        {
        }

        while (i > 0u)
        {
            e[i - 1u] &= (unsigned int)(~(((unsigned int)1u) << (j - ((unsigned char)1))));
            if (uint32_big_num_cmp(e, i, fai_n, i) < 0) // if e < n
            {
                break;
            }
            else
            {
            }

            j--;
            if (((unsigned char)0) == j) // j is 0, switch to the next word
            {
                i--;
                j = (unsigned char)32;
            }
            else
            {
            }
        }

        // fail, because fai_n is 1000000000...000000
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           judge whether big integer a is equal to 0x5a5a5a5a5a...5a or not.
 * @param[in]       a                    - unsigned int big odd integer e.
 * @param[in]       aBitLen              - real bit length of a.
 * @return          0:(a==0x5a5a5a5a5a...5a)     1:(a!=0x5a5a5a5a5a...5a)
 * @note
 *        1.aBitLen can not be 0.
 *        2.if aBitLen%32 != 0, then the highest word of a should be 0.
 */
FLAG_STATIC unsigned int CheckValue_0x5a5a5a5a(const unsigned int *a, unsigned int aBitLen)
{
    unsigned int ret = 0u;
    unsigned int i, wlen = aBitLen >> 5;

    if (0u != (aBitLen & 0x1Fu))
    {
        if (a[wlen] != 0u)
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

    if (0u == ret)
    {
        for (i = 0; i < wlen; i++)
        {
            if (a[i] != 0x5a5a5a5au)
            {
                ret = 1u;
                break;
            }
            else
            {
            }
        }
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           generate RSA key (e,d,n).
 * @param[out]      e                    - unsigned int big integer, RSA public key e.
 * @param[out]      d                    - unsigned int big integer, RSA private key d.
 * @param[out]      n                    - unsigned int big integer, RSA public module n.
 * @param[in]       eBitLen              - real bit length of e.
 * @param[in]       nBitLen              - real bit length of n.
 * @return          0:success     other:error
 * @note
 *        1.nBitLen can not be even.
 *        2.eBitLen must be larger than 1, and less than or equal to nBitLen.
 */
unsigned int RSA_GetKey(unsigned int *e, unsigned int *d, unsigned int *n, unsigned int eBitLen, unsigned int nBitLen)
{
    unsigned int buf[RSA_MAX_WORD_LEN];
    unsigned int *p, *q, *in, *out;
    unsigned int p_bitlen, p_wlen, e_wlen, n_wlen, tmp_step;
    unsigned int count, ret;

    if ((NULL == e) || (NULL == d) || (NULL == n))
    {
        return RSA_BUFFER_NULL;
    }
    else if ((0u != (nBitLen & 1u)) || (nBitLen < RSA_MIN_BIT_LEN) || (nBitLen > RSA_MAX_BIT_LEN)) // nBitLen can not be odd
    {
        return RSA_INPUT_INVALID;
    }
    else if ((eBitLen < 2u) || (eBitLen > nBitLen))
    {
        return RSA_INPUT_INVALID;
    }
    else
    {
        // handle other
    }

    p = buf;
    q = &buf[RSA_MAX_WORD_LEN >> 1];

    tmp_step = pke_set_operand_width(nBitLen);

    in = (unsigned int *)(rPKE_B(1u, tmp_step));
#if (defined(PKE_LP) || defined(PKE_SECURE))
    out = (unsigned int *)(rPKE_A(2u, tmp_step));
#else
    out = (unsigned int *)(rPKE_A(1u, tmp_step));
#endif

    e_wlen = get_word_len(eBitLen);
    n_wlen = get_word_len(nBitLen);
    p_bitlen = nBitLen >> 1;
    p_wlen = get_word_len(p_bitlen);

GET_PQ:

    ret = get_prime(p, p_bitlen);
    if (0u != ret)
    {
        return ret;
    }
    else
    {
    }

    ret = get_prime(q, p_bitlen);
    if (0u != ret)
    {
        return ret;
    }
    else
    {
    }

    p[0]--;                         // p=p-1
    q[0]--;                         // q=q-1
    ret = pke_mul(p, q, n, p_wlen); // get fai(n)=(p-1)(q-1)
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    count = 0u;
GET_E:
    count++;
    if (count == 7u)
    {
        goto GET_PQ;
    }
    else
    {
    }

    switch (eBitLen)
    {
    case 2u:
    {
        e[0] = 3u;
        break;
    }
    case 5u:
    {
        e[0] = 17u;
        break;
    }
    case 17u:
    {
        e[0] = 65537u;
        break;
    }
    default:
    {
        if (eBitLen == nBitLen)
        {
            ret = RSA_Get_E2(e, n, eBitLen);
            if (0u != ret)
            {
                return ret;
            }
            else
            {
            }
        }
        else
        {
            ret = RSA_Get_E1(e, eBitLen);
            if (0u != ret)
            {
                return ret;
            }
            else
            {
            }
        }
        break;
    }
    }

    // get d = e^(-1) mod n
    ret = pke_modinv(n, e, d, n_wlen, e_wlen);
    if (PKE_NO_MODINV == ret) // if d doesn't exist
    {
        if ((eBitLen == 2u) || (eBitLen == 5u) || (eBitLen == 17u)) // if e is prime, and e divide fai(n)
        {
            goto GET_PQ;
        }
        else // 1. e is prime, and e divide fai(n) 2.e is not prime, and
        {    // e, fai(n) have common divisor.
            goto GET_E;
        }
    }
    else if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        // handle other
    }

    // get n = pq
    p[0]++;
    q[0]++;
    ret = pke_mul(p, q, n, p_wlen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    ret = pke_pre_calc_mont_for_modexp(n, nBitLen, NULL, NULL);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // Encryption test
    if (0u != (nBitLen & 0x1Fu))
    {
        in[n_wlen - 1u] = 0u;
    }
    else
    {
    }

    uint32_set(in, 0x5a5a5a5au, nBitLen >> 5);

    ret = pke_modexp(n, e, in, out, n_wlen, e_wlen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    ret = pke_modexp(n, d, out, out, n_wlen, n_wlen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    if (0u != CheckValue_0x5a5a5a5a(out, nBitLen))
    {
        goto GET_PQ;
    }
    else
    {
        return RSA_SUCCESS;
    }
}

/**
 * @brief           Generate primes p-1, q-1 and compute fai(n)=(p-1)(q-1) for RSA-CRT
 * @param[out]      p_minus_1            - Output buffer for prime p minus 1 (p_wlen words)
 * @param[out]      q_minus_1            - Output buffer for prime q minus 1 (p_wlen words)
 * @param[out]      fai_n                - Output buffer for fai(n)=(p-1)(q-1) (optional)
 * @param[in]       eBitLen              - Bit length of public exponent e
 * @param[in]       p_bitlen             - Bit length of primes p and q
 * @param[in]       p_wlen               - Word length of p and q ((p_bitlen+31)>>5)
 * @return          PKE_SUCCESS on success, error code otherwise
 * @note
 *        1. fai(n) is only computed when eBitLen == (p_bitlen*2)
 *        2. Returns PKE_NO_MODINV if p=q, requiring key regeneration
 *        3. p_minus_1 and q_minus_1 must be distinct primes
 *        4. All buffers must be properly allocated before calling
 */
FLAG_STATIC unsigned int rsa_crt_keygen_get_p_minus_1_q_minus_1_fai_n(unsigned int *p_minus_1, unsigned int *q_minus_1, unsigned int *fai_n, unsigned int eBitLen,
                                                                      unsigned int p_bitlen, unsigned int p_wlen)
{
    unsigned int ret;
    unsigned int i, tmp;
    int32_t flag;

    ret = get_prime(p_minus_1, p_bitlen);
    if (MAYBE_PRIME == ret)
    {
        ret = PKE_SUCCESS;
    }
    else
    {
    }

    if (PKE_SUCCESS == ret)
    {
        ret = get_prime(q_minus_1, p_bitlen);
        if (MAYBE_PRIME == ret)
        {
            ret = PKE_SUCCESS;
        }
        else
        {
        }
    }
    else
    {
    }

    if (PKE_SUCCESS == ret)
    {
        flag = uint32_big_num_cmp(p_minus_1, p_wlen, q_minus_1,
                                  p_wlen); // make p > q, to get u = q^(-1) mod p conveniently
        if (flag == -1)
        {
            for (i = 0; i < p_wlen; i++)
            {
                tmp = p_minus_1[i];
                p_minus_1[i] = q_minus_1[i];
                q_minus_1[i] = tmp;
            }
        }
        else if (flag == 0)
        {
            ret = PKE_NO_MODINV; // p=q, need to regenerate key pair
        }
        else
        {
            // nothing to do, just for static analysis.
        }
    }
    else
    {
    }

    if (PKE_SUCCESS == ret)
    {
        p_minus_1[0]--; // get p-1
        q_minus_1[0]--; // get q-1
        if (eBitLen == (p_bitlen << 1))
        {
            ret = pke_mul(p_minus_1, q_minus_1, fai_n,
                          p_wlen); // get fai(n)=(p-1)(q-1)
        }
        else
        {
        }
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           Generate RSA-CRT key (dp,dq,u) from (e,p-1,q-1)
 * @param[in]       e                    - unsigned int big integer, RSA public key e
 * @param[in]       p_minus_1            - unsigned int big integer, RSA private key p minus 1
 * @param[in]       q_minus_1            - unsigned int big integer, RSA private key q minus 1
 * @param[out]      dp                   - unsigned int big integer, RSA private key dp
 * @param[out]      dq                   - unsigned int big integer, RSA private key dq
 * @param[out]      u                    - unsigned int big integer, RSA private key u = q^(-1) mod p
 * @param[in]       e_wlen               - Real word length of e
 * @param[in]       p_wlen               - Real word length of p_minus_1,q_minus_1,dp,dq,u
 * @return          PKE_SUCCESS on success, other values indicate error
 * @note            If the returned value is PKE_SUCCESS, p_minus_1 and q_minus_1 will be p and q respectively
 */
FLAG_STATIC unsigned int rsa_crt_keygen_get_dp_dq_u_from_e_p_minus_1_q_minus_1(const unsigned int *e, unsigned int *p_minus_1, unsigned int *q_minus_1, unsigned int *dp,
                                                                               unsigned int *dq, unsigned int *u, unsigned int e_wlen, unsigned int p_wlen)
{
    unsigned int ret;

    // dp = e^(-1) mod (p-1)
    if (uint32_big_num_cmp(e, e_wlen, p_minus_1, p_wlen) > 0)
    {
#if (defined(PKE_LP) || defined(PKE_SECURE))
        ret = pke_mod(e, e_wlen, p_minus_1, NULL, NULL, p_wlen, u);
#else
        ret = pke_mod(e, e_wlen, p_minus_1, NULL, p_wlen, u);
#endif
        if (PKE_SUCCESS == ret)
        {
            ret = pke_modinv(p_minus_1, u, dp, p_wlen, p_wlen);
        }
        else
        {
        }
    }
    else
    {
        ret = pke_modinv(p_minus_1, e, dp, p_wlen, e_wlen);
    }

    if (PKE_SUCCESS == ret)
    {
        // dq = e^(-1) mod (q-1)
        if (uint32_big_num_cmp(e, e_wlen, q_minus_1, p_wlen) > 0)
        {
#if (defined(PKE_LP) || defined(PKE_SECURE))
            ret = pke_mod(e, e_wlen, q_minus_1, NULL, NULL, p_wlen, u);
#else
            ret = pke_mod(e, e_wlen, q_minus_1, NULL, p_wlen, u);
#endif
            if (PKE_SUCCESS == ret)
            {
                ret = pke_modinv(q_minus_1, u, dq, p_wlen, p_wlen);
            }
            else
            {
            }
        }
        else
        {
            ret = pke_modinv(q_minus_1, e, dq, p_wlen, e_wlen);
        }
    }
    else
    {
    }

    if (PKE_SUCCESS == ret)
    {
        p_minus_1[0]++;
        q_minus_1[0]++;

        // u = q^(-1) mod p
        ret = pke_modinv(p_minus_1, q_minus_1, u, p_wlen, p_wlen);
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           Generate e, (p,q,dp,dq,u) from p-1, q-1, and fai(n)=(p-1)(q-1) (internal API)
 * @param[in]       fai_n                - unsigned int big integer, fai(n)=(p-1)(q-1)
 * @param[out]      e                    - unsigned int big integer, RSA public key e
 * @param[in]       p_minus_1            - unsigned int big integer, (p-1)
 * @param[in]       q_minus_1            - unsigned int big integer, (q-1)
 * @param[out]      dp                   - unsigned int big integer, RSA private key dp
 * @param[out]      dq                   - unsigned int big integer, RSA private key dq
 * @param[out]      u                    - unsigned int big integer, RSA private key u
 * @param[in]       nBitLen              - Bit length of n
 * @param[in]       p_wlen               - Word length of p,dq,q,dq,u
 * @param[in]       eBitLen              - Bit length of e
 * @param[in]       e_wlen               - Word length of e
 * @return          PKE_SUCCESS on success, other values indicate error
 * @note
 *        1. If eBitLen is 2,5,17, here makes e as 3,17,65537 respectively, otherwise e is random
 *        2. fai(n) is used when (eBitLen=nBitLen), and fai(n) occupies n_wlen words
 *        3. If return value is PKE_NO_MODINV, (p,q,dp,dq,u) does not exist
 *        4. If return value is PKE_SUCCESS, p_minus_1 and q_minus_1 will be p and q
 *           respectively
 */
FLAG_STATIC unsigned int rsa_crt_keygen_get_e_p_q_dp_dq_u_internal(const unsigned int *fai_n, unsigned int *e, unsigned int *p_minus_1, unsigned int *q_minus_1, unsigned int *dp,
                                                                   unsigned int *dq, unsigned int *u, unsigned int nBitLen, unsigned int p_wlen, unsigned int eBitLen,
                                                                   unsigned int e_wlen)
{
    unsigned int ret;

    if (17u == eBitLen)
    {
        e[0] = 65537u;
        ret = PKE_SUCCESS;
    }
    else if (5u == eBitLen)
    {
        e[0] = 17u;
        ret = PKE_SUCCESS;
    }
    else if (2u == eBitLen)
    {
        e[0] = 3u;
        ret = PKE_SUCCESS;
    }
    else if (eBitLen == nBitLen)
    {
        ret = RSA_Get_E2(e, fai_n, eBitLen);
    }
    else
    {
        ret = RSA_Get_E1(e, eBitLen);
    }

    if (PKE_SUCCESS == ret)
    {
        ret = rsa_crt_keygen_get_dp_dq_u_from_e_p_minus_1_q_minus_1(e, p_minus_1, q_minus_1, dp, dq, u, e_wlen, p_wlen);
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           Generate e and (p,q,dp,dq,u) from fai(n), p-1, and q-1
 * @param[in]       fai_n                - unsigned int big integer, fai(n)=(p-1)(q-1)
 * @param[out]      e                    - unsigned int big integer, RSA public key e
 * @param[in]       p_minus_1            - unsigned int big integer, (p-1)
 * @param[in]       q_minus_1            - unsigned int big integer, (q-1)
 * @param[out]      dp                   - unsigned int big integer, RSA private key dp
 * @param[out]      dq                   - unsigned int big integer, RSA private key dq
 * @param[out]      u                    - unsigned int big integer, RSA private key u
 * @param[in]       nBitLen              - Bit length of n
 * @param[in]       p_wlen               - Word length of p,dq,q,dq,u
 * @param[in]       eBitLen              - Bit length of e
 * @param[in]       e_wlen               - Word length of e
 * @return          PKE_SUCCESS on success, other values indicate error
 * @note
 *        1. If eBitLen is 2,5,17, here makes e as 3,17,65537 respectively, otherwise e is random
 *        2. fai(n) is used when (eBitLen=nBitLen), and fai(n) occupies n_wlen words
 *        3. If return value is PKE_NO_MODINV, (p,q,dp,dq,u) does not exist
 *        4. If return value is PKE_SUCCESS, p_minus_1 and q_minus_1 will be p and q
 *           respectively
 */
FLAG_STATIC unsigned int rsa_crt_keygen_get_e_p_q_dp_dq_u(const unsigned int *fai_n, unsigned int *e, unsigned int *p_minus_1, unsigned int *q_minus_1, unsigned int *dp,
                                                          unsigned int *dq, unsigned int *u, unsigned int nBitLen, unsigned int p_wlen, unsigned int eBitLen, unsigned int e_wlen)
{
    unsigned int ret;
    unsigned int count;

    /****************************************
     * if ret is PKE_NO_MODINV, (p,q,dp,dq,u) doesn't exist, that means :
     * 1. e is prime, and e divide fai(n)
     * 2. e is not prime, and e, fai(n) have common divisor.
     *****************************************/
    if ((17u == eBitLen) || (5u == eBitLen) || (2u == eBitLen))
    {
        ret = rsa_crt_keygen_get_e_p_q_dp_dq_u_internal(fai_n, e, p_minus_1, q_minus_1, dp, dq, u, nBitLen, p_wlen, eBitLen, e_wlen);
    }
    else
    {
        for (count = 0u; count < 7u; count++)
        {
            ret = rsa_crt_keygen_get_e_p_q_dp_dq_u_internal(fai_n, e, p_minus_1, q_minus_1, dp, dq, u, nBitLen, p_wlen, eBitLen, e_wlen);
#if 1
            if (PKE_NO_MODINV != ret)
            {
                break;
            }
            else
            {
            }
#else
            if (PKE_NO_MODINV == ret)
            {
                continue;
            }
            else
            {
                break;
            }
#endif
        }
    }

    return ret;
}

/**
 * @brief           Generate n and check the RSA-CRT key pair
 * @param[in]       e                    - unsigned int big integer, RSA public key e
 * @param[in]       p                    - unsigned int big integer, prime p
 * @param[in]       q                    - unsigned int big integer, prime q
 * @param[in]       dp                   - unsigned int big integer, RSA private key dp
 * @param[in]       dq                   - unsigned int big integer, RSA private key dq
 * @param[in]       u                    - unsigned int big integer, RSA private key u = q^(-1) mod p
 * @param[out]      n                    - unsigned int big integer, RSA public module n
 * @param[in]       e_wlen               - Word length of e
 * @param[in]       p_wlen               - Word length of p,q
 * @param[in]       nBitLen              - Bit length of n
 * @param[in]       n_wlen               - Word length of n
 * @return          PKE_SUCCESS on success, other values indicate error
 * @note            If return value is PKE_NO_MODINV, this means p or q may not be prime
 */

FLAG_STATIC unsigned int rsa_crt_keygen_final(const unsigned int *e, const unsigned int *p, const unsigned int *q, const unsigned int *dp, const unsigned int *dq,
                                              const unsigned int *u, unsigned int *n, unsigned int e_wlen, unsigned int p_wlen, unsigned int nBitLen, unsigned int n_wlen)
{
    unsigned int ret;
    unsigned int buf[RSA_MAX_WORD_LEN];

    // get n = pq
    ret = pke_mul(p, q, n, p_wlen);
    if (PKE_SUCCESS == ret)
    {
#if (defined(PKE_LP) || defined(PKE_SECURE))
        ret = pke_pre_calc_mont(n, nBitLen, NULL, NULL);
#else
        ret = pke_pre_calc_mont(n, nBitLen, NULL);
#endif
    }
    else
    {
    }

    if (PKE_SUCCESS == ret)
    {
// Encryption test
#ifdef SUPPORT_STATIC_ANALYSIS
        // the last two conditional expressions are just for static analysis
        if ((0u != (nBitLen & 0x1Fu)) && (n_wlen >= 1u) && (n_wlen <= RSA_MAX_WORD_LEN))
#else
        if (0u != (nBitLen & 0x1Fu))
#endif
        {
            buf[n_wlen - 1u] = 0u;
        }
        else
        {
        }

        uint32_set(buf, 0x5a5a5a5au, nBitLen >> 5);

        ret = pke_modexp_internal(e, buf, buf, n_wlen, e_wlen);
    }
    else
    {
    }

    if (PKE_SUCCESS == ret)
    {
        if (0u == CheckValue_0x5a5a5a5a(buf, nBitLen))
        {
            ret = PKE_NO_MODINV; // to make sure buf is changed
        }
        else
        {
            ret = RSA_CRTModExp(buf, p, q, dp, dq, u, buf, nBitLen);
        }
    }
    else
    {
    }

    if (PKE_SUCCESS == ret)
    {
        if (0u != CheckValue_0x5a5a5a5a(buf, nBitLen))
        {
            ret = PKE_NO_MODINV; // this means p or q may not be prime.
        }
        else
        {
        }
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           Generate RSA-CRT key (e,p,q,dp,dq,u,n)
 * @param[out]      e                    - unsigned int big integer, RSA public key e
 * @param[out]      p                    - unsigned int big integer, RSA private key p
 * @param[out]      q                    - unsigned int big integer, RSA private key q
 * @param[out]      dp                   - unsigned int big integer, RSA private key dp
 * @param[out]      dq                   - unsigned int big integer, RSA private key dq
 * @param[out]      u                    - unsigned int big integer, RSA private key u = q^(-1) mod p
 * @param[out]      n                    - unsigned int big integer, RSA public module n
 * @param[in]       eBitLen              - Real bit length of e
 * @param[in]       nBitLen              - Real bit length of n
 * @return          RSA_SUCCESS on success, other values indicate error
 * @note
 *        1. nBitLen cannot be even
 *        2. eBitLen must be greater than 1, and less than or equal to nBitLen
 *        3. If eBitLen is 2,5,17, here makes e as 3,17,65537 respectively, otherwise e is random
 */
unsigned int RSA_GetCRTKey(unsigned int *e, unsigned int *p, unsigned int *q, unsigned int *dp, unsigned int *dq, unsigned int *u, unsigned int *n, unsigned int eBitLen,
                           unsigned int nBitLen)
{
    unsigned int ret;
    unsigned int p_bitlen, p_wlen, e_wlen, n_wlen;

    if ((NULL == e) || (NULL == p) || (NULL == q) || (NULL == dp) || (NULL == dq) || (NULL == u) || (NULL == n))
    {
        ret = RSA_BUFFER_NULL;
    }
    else if ((0u != (nBitLen & 1u)) || (nBitLen < RSA_MIN_BIT_LEN) || (nBitLen > RSA_MAX_BIT_LEN)) // nBitLen can not be odd
    {
        ret = RSA_INPUT_INVALID;
    }
    else if ((eBitLen < 2u) || (eBitLen > nBitLen))
    {
        ret = RSA_INPUT_INVALID;
    }
    else
    {
        ret = PKE_SUCCESS;
    }

    if (PKE_SUCCESS == ret)
    {
        e_wlen = get_word_len(eBitLen);
        n_wlen = get_word_len(nBitLen);
        p_bitlen = nBitLen >> 1;
        p_wlen = get_word_len(p_bitlen);

        /****************************************
         * if ret is PKE_NO_MODINV, that means :
         * 1. e, fai(n) have common divisor.
         * 2. p or q is not prime.
         * 3. p = q.
         *****************************************/
        do
        {
            ret = rsa_crt_keygen_get_p_minus_1_q_minus_1_fai_n(p, q, n, eBitLen, p_bitlen, p_wlen);
            if (PKE_SUCCESS == ret)
            {
                ret = rsa_crt_keygen_get_e_p_q_dp_dq_u(n, e, p, q, dp, dq, u, nBitLen, p_wlen, eBitLen, e_wlen);
            }
            else
            {
            }

            if (PKE_SUCCESS == ret)
            {
                ret = rsa_crt_keygen_final(e, p, q, dp, dq, u, n, e_wlen, p_wlen, nBitLen, n_wlen);
            }
            else
            {
            }
        } while (PKE_NO_MODINV == ret);

        if (PKE_SUCCESS == ret)
        {
            ret = RSA_SUCCESS;
        }
        else
        {
        }
    }

    return ret;
}

#endif
