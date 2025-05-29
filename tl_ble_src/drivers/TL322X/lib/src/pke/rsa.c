/********************************************************************************************************
 * @file    rsa.c
 *
 * @brief   This is the source file for tl322x
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

#include "lib/include/pke/pke_config.h"


#ifdef SUPPORT_RSA

    #include "lib/include/pke/rsa.h"
    #include "lib/include/pke/pke_prime.h"
    #include "lib/include/trng/trng.h"
    #include "lib/include/crypto_common/utility.h"

/**
 * @brief       out = a^e mod n.
 * @param[in]   a            - unsigned int big integer a, base number, make sure a < n.
 * @param[in]   e            - unsigned int big integer e, exponent, make sure e < n.
 * @param[in]   n            - unsigned int big integer n, modulus, make sure n is odd.
 * @param[out]  out          - out = a^e mod n.
 * @param[in]   eBitLen      - real bit length of unsigned int big integer e.
 * @param[in]   nBitLen      - real bit length of unsigned int big integer n.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.a, n, and out have the same word length:((nBitLen+31)>>5); and e word length is (eBitLen+31)>>5.
  @endverbatim
 */
unsigned int RSA_ModExp(unsigned int *a, unsigned int *e, unsigned int *n, unsigned int *out, unsigned int eBitLen, unsigned int nBitLen)
{
    unsigned int eWordLen = GET_WORD_LEN(eBitLen);
    unsigned int nWordLen = GET_WORD_LEN(nBitLen);
    unsigned int ret;

    if ((NULL == a) || (NULL == e) || (NULL == n) || (NULL == out)) {
        return RSA_BUFFER_NULL;
    } else if ((nBitLen > RSA_MAX_BIT_LEN) || (eBitLen > nBitLen)) {
        return RSA_INPUT_TOO_LONG;
    } else if ((0u == nBitLen) || (0u == (n[0] & 1u))) {
        return RSA_INPUT_INVALID;
    } else {
        ;
    }

    ret = pke_modexp_check_input((unsigned int *)n, (unsigned int *)e, (unsigned int *)a, out, nWordLen, eWordLen);
    if (PKE_FINISHED == ret) {
        return RSA_SUCCESS;
    } else if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = pke_pre_calc_mont_for_modexp(n, nBitLen, NULL, NULL);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    return pke_modexp(n, e, a, out, nWordLen, eWordLen);
}

/**
 * @brief        out = a^d mod n, here d represents RSA CRT private key (p,q,dp,dq,u).
 * @param[in]   a            - unsigned int big integer a, base number, make sure a < n=pq.
 * @param[in]   p            - unsigned int big integer p, prime number, one part of private key (p,q,dp,dq,u).
 * @param[in]   q            - unsigned int big integer q, prime number, one part of private key (p,q,dp,dq,u).
 * @param[in]   dp           - unsigned int big integer dp = e^(-1) mod (p-1), one part of private key (p,q,dp,dq,u).
 * @param[in]   dq           - unsigned int big integer dq = e^(-1) mod (q-1), one part of private key (p,q,dp,dq,u).
 * @param[in]   u            - unsigned int big integer u = q^(-1) mod p, one part of private key (p,q,dp,dq,u).
 * @param[in]   out          - out = a^d mod n.
 * @param[out]  nBitLen      - real bit length of unsigned int big integer n=pq.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# a and out have the same word length:((nBitLen+31)>>5); and p,p_h,q,q_h,dp,dq,u
         have the same word length:((nBitLen/2+31)>>5).
  @endverbatim
 */
unsigned int RSA_CRTModExp(unsigned int *a, unsigned int *p, unsigned int *q, unsigned int *dp, unsigned int *dq, unsigned int *u, unsigned int *out, unsigned int nBitLen)
{
    unsigned int  buf[RSA_MAX_WORD_LEN];
    unsigned int *m1 = buf;
    unsigned int *m2 = buf + (RSA_MAX_WORD_LEN >> 1);
    unsigned int *tmp_out;
    unsigned int  tmp_step;
    unsigned int  nWordLen = GET_WORD_LEN(nBitLen);
    unsigned int  pBitLen  = nBitLen >> 1;
    unsigned int  pWordLen = GET_WORD_LEN(pBitLen);
    int32_t       flag;
    unsigned int  ret;

    if ((NULL == a) || (NULL == p) || (NULL == q) || (NULL == dp) || (NULL == dq) || (NULL == u) || (NULL == out)) {
        return RSA_BUFFER_NULL;
    } else if (nBitLen > RSA_MAX_BIT_LEN) {
        return RSA_INPUT_TOO_LONG;
    } else if ((0u == nBitLen) || (0u != (nBitLen & 1u))) {
        return RSA_INPUT_INVALID;
    } else {
        ;
    }

    //get n = p*q
    ret = pke_mul(p, q, buf, pWordLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //a should be in [0,n]
    flag = uint32_BigNumCmp(a, nWordLen, buf, nWordLen);
    if (flag > 0) {
        return RSA_INPUT_INVALID;
    } else if ((0 == flag) || (1u == uint32_BigNum_Check_Zero(a, nWordLen))) //if a is 0 or n, the output is 0
    {
        uint32_clear(out, nWordLen);
        return RSA_SUCCESS;
    } else {
        ;
    }

    //do pke_pre_calc_mont() first, because a may be less than p or q, then pke_mod() will not
    //call pke_pre_calc_mont() inside, but pke_modexp() needs the output of pke_pre_calc_mont().

    //m2 = (a) mod q
    ret = pke_pre_calc_mont_for_modexp(q, pBitLen, NULL, NULL);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //get the pBitLen step
    tmp_step = pke_get_operand_bytes();

    #if (defined(PKE_LP) || defined(PKE_SECURE))
    ret = pke_mod(a, nWordLen, q, (unsigned int *)(rPKE_A(3u, tmp_step)), (unsigned int *)(rPKE_B(4u, tmp_step)), pWordLen, m2);
    #else
    ret = pke_mod(a, nWordLen, q, (unsigned int *)(rPKE_B(0u, tmp_step)), pWordLen, m2);
    #endif
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //m2 = (a)^dq mod q
    ret = pke_modexp(q, dq, m2, m2, pWordLen, pWordLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //m1 = (a) mod p
    ret = pke_pre_calc_mont_for_modexp(p, pBitLen, NULL, NULL);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    #if (defined(PKE_LP) || defined(PKE_SECURE))
    ret = pke_mod(a, nWordLen, p, (unsigned int *)(rPKE_A(3u, tmp_step)), (unsigned int *)(rPKE_B(4u, tmp_step)), pWordLen, m1);
    #else
    ret = pke_mod(a, nWordLen, p, (unsigned int *)(rPKE_B(0u, tmp_step)), pWordLen, m1);
    #endif
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //m1 = (a)^dp mod p
    ret = pke_modexp(p, dp, m1, m1, pWordLen, pWordLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    #if (defined(PKE_LP) || defined(PKE_SECURE))
    tmp_out = (unsigned int *)(rPKE_B(0u, tmp_step));
    #else
    tmp_out = (unsigned int *)(rPKE_B(1u, tmp_step));
    #endif

    //m1 = (m1-m2) mod p
    if (uint32_BigNumCmp(m2, pWordLen, p, pWordLen) >= 0) {
        //if m2 >= p, get tmp_out = m2 mod p
        ret = pke_sub(m2, p, tmp_out, pWordLen);
        if (PKE_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }

        ret = pke_modsub(p, m1, tmp_out, m1, pWordLen);
    } else {
        ret = pke_modsub(p, m1, m2, m1, pWordLen);
    }

    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //m1 = h = u*(m1-m2) mod p
    #if 1
        #if (defined(PKE_LP) || defined(PKE_SECURE))
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
        #endif
    ret = pke_modmul_internal(m1, u, m1, pWordLen);
    #else
    ret = pke_modmul(p, m1, u, m1, pWordLen);
    #endif
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //store the nBitLen step
    tmp_step = pke_set_operand_width(nBitLen);

    //A1 = hq
    ret = pke_mul(m1, q, (unsigned int *)(rPKE_A(1u, tmp_step)), pWordLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //out = m2+hq
    uint32_copy((unsigned int *)(rPKE_B(1u, tmp_step)), m2, pWordLen);
    uint32_clear((unsigned int *)(rPKE_B(1u, tmp_step)) + pWordLen, nWordLen - pWordLen);
    return pke_add((unsigned int *)(rPKE_A(1u, tmp_step)), (unsigned int *)(rPKE_B(1u, tmp_step)), out, nWordLen);
}

/**
 * @brief       get big odd integer e of eBitLen
 * @param[in]   e            - unsigned int big odd integer e.
 * @param[in]   eBitLen      - bit length of unsigned int big odd integer e.
 * @return      0:success     1:error
 * @note
  @verbatim
      -# 1.eBitLen must be big than 1.
  @endverbatim
 */
unsigned int RSA_Get_E1(unsigned int e[], unsigned int eBitLen)
{
    unsigned int eWordLen = (eBitLen + 0x1Fu) >> 5;
    unsigned int ret;

    if (eBitLen < 2u) {
        return RSA_INPUT_INVALID;
    } else {
        ;
    }

    ret = get_rand((unsigned char *)e, eWordLen << 2);
    if (TRNG_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    eBitLen &= 31u;

    if (0u != eBitLen) {
    #if 0
        e[eWordLen - 1u] <<= (32u - eBitLen);
        e[eWordLen - 1u] |= 0x80000000u;
        e[eWordLen - 1u] >>= (32u - eBitLen);
    #else
        e[eWordLen - 1u] &= (1u << (eBitLen)) - 1u;
        e[eWordLen - 1u] |= 1u << (eBitLen - 1u);
    #endif
    } else {
        e[eWordLen - 1u] |= 0x80000000u;
    }

    e[0] |= 0x01u; //make e odd

    return 0u;
}

/**
 * @brief       get big odd integer e of eBitLen, satisfies e < fai_n of bitLen.
 * @param[in]   e                - unsigned int big odd integer e.
 * @param[in]   fai_n            - unsigned int big even integer fai_n.
 * @param[in]   bitLen           - bit length of unsigned int big odd integer e and n.
 * @return      0:success     1:error(error: bitLen<66), 2(error, n is 1000000000...000000)
 * @note
  @verbatim
      -# 1.eBitLen must be big than 65.
      -# 2.n can not be 1000000000...000000.
  @endverbatim
 */
unsigned int RSA_Get_E2(unsigned int e[], unsigned int fai_n[], unsigned int bitLen)
{
    int32_t       i;
    unsigned char j;

    if (bitLen < 66u) {
        return 1u;
    } else {
        ;
    }

    RSA_Get_E1(e, bitLen);
    bitLen--;
    i = ((bitLen + 0x1Fu) >> 5) - 1; //i is the word index of the word where the second highest bit is located
    j = bitLen & 31u;                //j is the bit length up to the second highest bit in the targeted word
    if (j == (unsigned char)0) {
        j = 32;
    } else {
        ;
    }

    while (i >= 0) {
        e[i] &= (unsigned int)(~(1u << (j - ((unsigned char)1))));
        if (uint32_BigNumCmp(e, i + 1, fai_n, i + 1) < 0) //if e < n
        {
            return 0u;
        } else {
            ;
        }

        j--;
        if (((unsigned char)0) == j) //j is 0, switch to the next word
        {
            i--;
            j = 32;
        } else {
            ;
        }
    }

    return 2u; //fail, because fai_n is 1000000000...000000
}

/**
 * @brief       judge whether big integer a is equal to 0x5a5a5a5a5a...5a or not.
 * @param[in]   a                - unsigned int big odd integer e.
 * @param[in]   aBitLen          - real bit length of a.
 * @return      0:(a==0x5a5a5a5a5a...5a)     1:(a!=0x5a5a5a5a5a...5a)
 * @note
  @verbatim
      -# 1.aBitLen can not be 0.
      -# 2.if aBitLen%32 != 0, then the highest word of a should be 0.
  @endverbatim
 */
unsigned int CheckValue_0x5a5a5a5a(unsigned int a[], unsigned int aBitLen)
{
    unsigned int i, wordLen = aBitLen >> 5;

    if (0u != (aBitLen & 0x1Fu)) {
        if (a[wordLen] != 0u) {
            return 1;
        } else {
            ;
        }
    } else {
        ;
    }

    for (i = 0; i < wordLen; i++) {
        if (a[i] != 0x5a5a5a5au) {
            return 1;
        } else {
            ;
        }
    }

    return 0;
}

/**
 * @brief       generate RSA key (e,d,n).
 * @param[out]  e                - unsigned int big integer, RSA public key e.
 * @param[out]  d                - unsigned int big integer, RSA private key d.
 * @param[out]  n                - unsigned int big integer, RSA public module n.
 * @param[in]   eBitLen          - real bit length of e.
 * @param[in]   nBitLen          - real bit length of n.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.nBitLen can not be even.
      -# 2.eBitLen must be larger than 1, and less than or equal to nBitLen.
  @endverbatim
 */
unsigned int RSA_GetKey(unsigned int *e, unsigned int *d, unsigned int *n, unsigned int eBitLen, unsigned int nBitLen)
{
    unsigned int  buf[RSA_MAX_WORD_LEN];
    unsigned int *p, *q, *in, *out;
    unsigned int  pBitLen, pWordLen, eWordLen, nWordLen, tmp_step;
    unsigned int  count, ret;

    if ((NULL == e) || (NULL == d) || (NULL == n)) {
        return RSA_BUFFER_NULL;
    } else if ((0u != (nBitLen & 1u)) || (nBitLen < RSA_MIN_BIT_LEN) || (nBitLen > RSA_MAX_BIT_LEN)) //nBitLen can not be odd
    {
        return RSA_INPUT_INVALID;
    } else if ((eBitLen < 2u) || (eBitLen > nBitLen)) {
        return RSA_INPUT_INVALID;
    } else {
        ;
    }

    p = buf;
    q = buf + RSA_MAX_WORD_LEN / 2;

    tmp_step = pke_set_operand_width(nBitLen);

    in = (unsigned int *)(rPKE_B(1u, tmp_step));
    #if (defined(PKE_LP) || defined(PKE_SECURE))
    out = (unsigned int *)(rPKE_A(2u, tmp_step));
    #else
    out = (unsigned int *)(rPKE_A(1u, tmp_step));
    #endif

    eWordLen = GET_WORD_LEN(eBitLen);
    nWordLen = GET_WORD_LEN(nBitLen);
    pBitLen  = nBitLen >> 1;
    pWordLen = GET_WORD_LEN(pBitLen);

GET_PQ:

    ret = get_prime(p, pBitLen);
    if (0u != ret) {
        return ret;
    } else {
        ;
    }

    ret = get_prime(q, pBitLen);
    if (0u != ret) {
        return ret;
    } else {
        ;
    }

    p[0]--;                           // p=p-1
    q[0]--;                           // q=q-1
    ret = pke_mul(p, q, n, pWordLen); // get fai(n)=(p-1)(q-1)
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    count = 0u;
GET_E:
    count++;
    if (count == 7u) {
        goto GET_PQ;
    } else {
        ;
    }

    switch (eBitLen) {
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
        if (eBitLen == nBitLen) {
            ret = RSA_Get_E2(e, n, eBitLen);
            if (0u != ret) {
                return ret;
            } else {
                ;
            }
        } else {
            ret = RSA_Get_E1(e, eBitLen);
            if (0u != ret) {
                return ret;
            } else {
                ;
            }
        }
        break;
    }
    }

    //get d = e^(-1) mod n
    ret = pke_modinv(n, e, d, nWordLen, eWordLen);
    if (PKE_NO_MODINV == ret)                                       //if d doesn't exist
    {
        if ((eBitLen == 2u) || (eBitLen == 5u) || (eBitLen == 17u)) //if e is prime, and e divide fai(n)
        {
            goto GET_PQ;
        } else                                                      //1. e is prime, and e divide fai(n) 2.e is not prime, and
        {                                                           //e, fai(n) have common divisor.
            goto GET_E;
        }
    } else if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //get n = pq
    p[0]++;
    q[0]++;
    ret = pke_mul(p, q, n, pWordLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = pke_pre_calc_mont_for_modexp(n, nBitLen, NULL, NULL);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //Encryption test
    if (0u != (nBitLen & 0x1Fu)) {
        in[nWordLen - 1u] = 0u;
    } else {
        ;
    }

    uint32_set(in, 0x5a5a5a5au, nBitLen >> 5);

    ret = pke_modexp(n, e, in, out, nWordLen, eWordLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = pke_modexp(n, d, out, out, nWordLen, nWordLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    if (0u != CheckValue_0x5a5a5a5a(out, nBitLen)) {
        goto GET_PQ;
    } else {
        return RSA_SUCCESS;
    }
}

/**
 * @brief       generate RSA-CRT key (e,p,q,dp,dq,u,n).
 * @param[out]  e                - unsigned int big integer, RSA public key e.
 * @param[out]  p                - unsigned int big integer, RSA private key p.
 * @param[out]  q                - unsigned int big integer, RSA private key q.
 * @param[out]  dp               - unsigned int big integer, RSA private key dp.
 * @param[out]  dq               - unsigned int big integer, RSA private key dq.
 * @param[out]  u                - unsigned int big integer, RSA private key u = q^(-1) mod p.
 * @param[out]  n                - unsigned int big integer, RSA public module n.
 * @param[in]   eBitLen          - real bit length of e.
 * @param[in]   nBitLen          - real bit length of n.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.nBitLen can not be even.
      -# 2.eBitLen must be larger than 1, and less than or equal to nBitLen.
  @endverbatim
 */
unsigned int RSA_GetCRTKey(unsigned int *e, unsigned int *p, unsigned int *q, unsigned int *dp, unsigned int *dq, unsigned int *u, unsigned int *n, unsigned int eBitLen, unsigned int nBitLen)
{
    unsigned int buf[RSA_MAX_WORD_LEN];
    unsigned int pBitLen, pWordLen, eWordLen, nWordLen, i, wordLen;
    int32_t      count;
    unsigned int ret;

    if ((NULL == e) || (NULL == p) || (NULL == q) || (NULL == dp) || (NULL == dq) || (NULL == u) || (NULL == n)) {
        return RSA_BUFFER_NULL;
    } else if ((0u != (nBitLen & 1u)) || (nBitLen < RSA_MIN_BIT_LEN) || (nBitLen > RSA_MAX_BIT_LEN)) //nBitLen can not be odd
    {
        return RSA_INPUT_INVALID;
    } else if ((eBitLen < 2u) || (eBitLen > nBitLen)) {
        return RSA_INPUT_INVALID;
    } else {
        ;
    }

    eWordLen = GET_WORD_LEN(eBitLen);
    nWordLen = GET_WORD_LEN(nBitLen);
    pBitLen  = nBitLen >> 1;
    pWordLen = GET_WORD_LEN(pBitLen);

GET_PQ:

    ret = get_prime(p, pBitLen);
    if (0u != ret) {
        return ret;
    } else {
        ;
    }

    ret = get_prime(q, pBitLen);
    if (0u != ret) {
        return ret;
    } else {
        ;
    }

    count = uint32_BigNumCmp(p, pWordLen, q, pWordLen); // make p > q, for get u = q^(-1) mod p convenient
    if (count == -1) {
        for (i = 0; i < pWordLen; i++) {
            wordLen = p[i];
            p[i]    = q[i];
            q[i]    = wordLen;
        }
    } else if (count == 0) {
        goto GET_PQ;
    } else {
        ;
    }

    p[0]--;                               // p=p-1
    q[0]--;                               // q=q-1
    if (eBitLen == nBitLen) {
        ret = pke_mul(p, q, n, pWordLen); // get fai(n)=(p-1)(q-1)
        if (PKE_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }
    } else {
        ;
    }

    count = 0;
GET_E:
    count++;
    if (count == 7) {
        goto GET_PQ;
    } else {
        ;
    }

    switch (eBitLen) {
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
        if (eBitLen == nBitLen) {
            ret = RSA_Get_E2(e, n, eBitLen);
            if (0u != ret) {
                return ret;
            } else {
                ;
            }
        } else {
            ret = RSA_Get_E1(e, eBitLen);
            if (0u != ret) {
                return ret;
            } else {
                ;
            }
        }
        break;
    }
    }

    // dp = e^(-1) mod (p-1)
    if (uint32_BigNumCmp(e, eWordLen, p, pWordLen) > 0) {
    #if (defined(PKE_LP) || defined(PKE_SECURE))
        ret = pke_mod(e, eWordLen, p, NULL, NULL, pWordLen, u);
    #else
        ret = pke_mod(e, eWordLen, p, NULL, pWordLen, u);
    #endif
        if (PKE_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }

        wordLen = pWordLen;
    } else {
        uint32_copy(u, e, eWordLen);
        wordLen = eWordLen;
    }

    ret = pke_modinv(p, u, dp, pWordLen, wordLen);
    if (PKE_NO_MODINV == ret) {
        if ((eBitLen == 2u) || (eBitLen == 5u) || (eBitLen == 17u)) //if e is prime, and e divide fai(n)
        {
            goto GET_PQ;
        } else                                                      //1. e is prime, and e divide fai(n) 2.e is not prime, and
        {                                                           //e, fai(n) have common divisor.
            goto GET_E;
        }
    } else if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    // dq = e^(-1) mod (q-1)
    if (uint32_BigNumCmp(e, eWordLen, q, pWordLen) > 0) {
    #if (defined(PKE_LP) || defined(PKE_SECURE))
        ret = pke_mod(e, eWordLen, q, NULL, NULL, pWordLen, u);
    #else
        ret = pke_mod(e, eWordLen, q, NULL, pWordLen, u);
    #endif
        if (PKE_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }

        wordLen = pWordLen;
    } else {
        uint32_copy(u, e, eWordLen);
        wordLen = eWordLen;
    }

    ret = pke_modinv(q, u, dq, pWordLen, wordLen);
    if (PKE_NO_MODINV == ret) {
        if ((eBitLen == 2u) || (eBitLen == 5u) || (eBitLen == 17u)) //if e is prime, and e divide fai(n)
        {
            goto GET_PQ;
        } else                                                      //1. e is prime, and e divide fai(n) 2.e is not prime, and
        {                                                           //e, fai(n) have common divisor.
            goto GET_E;
        }
    } else if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    p[0]++;
    q[0]++;

    // u = q^(-1) mod p
    ret = pke_modinv(p, q, u, pWordLen, pWordLen);
    if (PKE_NO_MODINV == ret) {
        goto GET_PQ;
    } else if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    // get n
    ret = pke_mul(p, q, n, pWordLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //Encryption test
    if (0u != (nBitLen & 0x1Fu)) {
        buf[nWordLen - 1u] = 0u;
    } else {
        ;
    }

    wordLen = nBitLen >> 5;
    uint32_set(buf, 0x5a5a5a5au, wordLen);

    ret = pke_pre_calc_mont_for_modexp(n, nBitLen, NULL, NULL);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = pke_modexp(n, e, buf, buf, nWordLen, eWordLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    if (0u == CheckValue_0x5a5a5a5a(buf, nBitLen)) {
        goto GET_PQ;
    } else {
        ;
    }

    ret = RSA_CRTModExp(buf, p, q, dp, dq, u, buf, nBitLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    if (0u != CheckValue_0x5a5a5a5a(buf, nBitLen)) {
        goto GET_PQ;
    } else {
        return RSA_SUCCESS;
    }
}

#endif
