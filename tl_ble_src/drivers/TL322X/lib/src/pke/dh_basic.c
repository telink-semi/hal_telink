/********************************************************************************************************
 * @file    dh_basic.c
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


#ifdef SUPPORT_DH

    #include "lib/include/pke/dh.h"
    #include "lib/include/trng/trng.h"
    #include "lib/include/crypto_common/utility.h"

/**
 * @brief       DH parameters pointer init, set pointers of (p, q, g)
 * @param[in]   dh_para      - DH_PARA struct pointer.
 * @param[in]   p_buf        - a U32 buffer holds p, the prime defining the GF(p).
 * @param[in]   p_bits       - bit length of p.
 * @param[in]   p_h_buf      - a U32 buffer holds pre-calculated mont parameters H(R^2 mod p).
 * @param[in]   P_n0_buf     - a U32 buffer holds pre-calculated mont parameters n0'(-modoulus^(-1) mod 2^w).
 * @param[in]   q_buf        - a U32 buffer holds q, a prime factor of p-1, aka order of g..
 * @param[in]   q_bits       - bit length of q.
 * @param[in]   g_buf        - a U32 buffer holds g, a generator of the q-order subgroup of GF(p)*.
 * @param[in]   g_bits       - bit length of g.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.p_h_buf holds H(R^2 mod p), to accelerate DH calculation, it occupies the same size 
          memory as well as p_buf. if you do not have this, please set p_h_buf to NULL.
      -# 2.p_n0_buf holds n0'(-modoulus^(-1) mod 2^w), to accelerate DH calculation, it occupies
          (w+7)/8 bytes, here w is actually 32. if you do not have this, please set p_n0_buf to NULL.
  @endverbatim
 */
unsigned int dh_param_pointer_init(DH_PARA *dh_para, unsigned int *p_buf, unsigned int p_bits, unsigned int *p_h_buf, unsigned int *p_n0_buf, unsigned int *q_buf, unsigned int q_bits, unsigned int *g_buf, unsigned int g_bits)
{
    if ((NULL == dh_para) || (NULL == p_buf) || (NULL == q_buf) || (NULL == g_buf)) {
        return DH_POINTER_NULL;
    } else if ((0 == p_bits) || (0 == q_bits) || (0 == g_bits)) {
        return DH_INVALID_INPUT;
    } else if ((q_bits > p_bits) || (g_bits > p_bits)) {
        return DH_INVALID_INPUT;
    } else if (p_bits > DH_MAX_BIT_LEN) {
        return DH_INVALID_INPUT;
    } else {
        ;
    }

    dh_para->p      = p_buf;
    dh_para->p_bits = p_bits;
    dh_para->p_h    = p_h_buf;
    dh_para->p_n0   = p_n0_buf;
    dh_para->q      = q_buf;
    dh_para->q_bits = q_bits;
    dh_para->g      = g_buf;
    dh_para->g_bits = g_bits;

    return DH_SUCCESS;
}

/**
 * @brief       DH parameters value init, set pointers of (p, q, g)
 * @param[in]   dh_para      - DH_PARA struct pointer.
 * @param[in]   p            - a prime defining the GF(p).
 * @param[in]   p_h          - the pre-calculated mont parameter (R^2 mod p).
 * @param[it]   p_n0         - the pre-calculated mont parameter (-modoulus^(-1) mod 2^w).
 * @param[in]   q            - a prime factor of p-1, aka order of g.
 * @param[in]   g            - a generator of the q-order subgroup of GF(p)*
 * @return      0:success     other:error
 * @note
  @verbatim
      -#1. please call dh_param_pointer_init() before calling this function.
      -#2. the input p occupies (dh_para->p_bits+7)/8 bytes;
           the input p_h occupies (dh_para->p_bits+7)/8 bytes, if you have this;
           the input p_n0 occupies (w+7)/8 bytes, if you have this, w is actually 32 here;
           the input q occupies (dh_para->q_bits+7)/8 bytes;
           the input g occupies (dh_para->g_bits+7)/8 bytes;
       -#3.if you do not have p_h, please set p_h to NULL, 
           if you do not have p_n0, please set p_n0 to NULL.       
  @endverbatim
 */
unsigned int dh_param_value_init(DH_PARA *dh_para, unsigned char *p, unsigned char *p_h, unsigned char *p_n0, unsigned char *q, unsigned char *g)
{
    unsigned int pByteLen, qByteLen, gByteLen;

    if ((NULL == dh_para) || (NULL == p) || (NULL == q) || (NULL == g)) {
        return DH_POINTER_NULL;
    } else {
        ;
    }

    pByteLen = GET_BYTE_LEN(dh_para->p_bits);
    qByteLen = GET_BYTE_LEN(dh_para->q_bits);
    gByteLen = GET_BYTE_LEN(dh_para->g_bits);

    if (NULL != p_h) {
        if (NULL == dh_para->p_h) {
            return DH_POINTER_NULL;
        } else {
            reverse_byte_array(p_h, (unsigned char *)dh_para->p_h, pByteLen);
        }
    } else {
        ;
    }

    if (NULL != p_n0) {
        if (NULL == dh_para->p_n0) {
            return DH_POINTER_NULL;
        } else {
            reverse_byte_array(p_n0, (unsigned char *)dh_para->p_n0, 1 << 2);
        }
    } else {
        ;
    }

    reverse_byte_array(p, (unsigned char *)dh_para->p, pByteLen);
    reverse_byte_array(q, (unsigned char *)dh_para->q, qByteLen);
    reverse_byte_array(g, (unsigned char *)dh_para->g, gByteLen);

    //p and q can not be even.
    if ((0u == (dh_para->p[0] & 1u)) || (0u == (dh_para->q[0] & 1u))) {
        return DH_INVALID_INPUT;
    } else {
        ;
    }

    //to support p == dh_para->p
    if (0u != (pByteLen & 3u)) {
        memset_(((unsigned char *)dh_para->p) + pByteLen, 0, 4u - (pByteLen & 3u));
    } else {
        ;
    }

    //to support q == dh_para->q
    if (0u != (qByteLen & 3u)) {
        memset_(((unsigned char *)dh_para->q) + qByteLen, 0, 4u - (qByteLen & 3u));
    } else {
        ;
    }

    //to support g == dh_para->g
    if (0u != (gByteLen & 3u)) {
        memset_(((unsigned char *)dh_para->g) + gByteLen, 0, 4u - (gByteLen & 3u));
    } else {
        ;
    }

    return DH_SUCCESS;
}

/*
unsigned int dh_parameter_check(DH_PARA *dh_para)
{
    //TODO

    return DH_SUCCESS;
}*/


/**
 * @brief       DH check public key, it must be in [2, p-2], and pubkey^q = 1 mod p
 * @param[in]   dh_para      - DH_PARA struct pointer.
 * @param[in]   p_minus_1    - p-1.
 * @param[in]   pubkey       - public key.
 * @return      0:success     other:error
 * @note
  @verbatim
      -#1. please call dh_param_value_init() before calling this function.
      -#2. the input p_minus_1 and pubkey both occupy (dh_para->p_bits+31)/32 words;      
  @endverbatim
 */
unsigned int dh_check_public_key(DH_PARA *dh_para, unsigned int *p_minus_1, unsigned int *pubkey)
{
    //unsigned int tmp[DH_MAX_WORD_LEN];
    unsigned int *tmp;
    unsigned int  step_bytes, pWordLen, qWordLen;
    unsigned int  ret;

    if ((NULL == dh_para) || (NULL == p_minus_1) || (NULL == pubkey)) {
        return DH_POINTER_NULL;
    } else {
        ;
    }

    pWordLen = GET_WORD_LEN(dh_para->p_bits);
    qWordLen = GET_WORD_LEN(dh_para->q_bits);

    //make sure pubkey is in [2, p-2]
    if (get_valid_bits(pubkey, pWordLen) <= 1) {
        return DH_INVALID_INPUT;
    } else if (uint32_BigNumCmp(pubkey, pWordLen, p_minus_1, pWordLen) >= 0) {
        return DH_INVALID_INPUT;
    } else {
        ;
    }

    if ((NULL == dh_para->p_h) || (NULL == dh_para->p_n0)) {
        ret = pke_pre_calc_mont_for_modexp(dh_para->p, dh_para->p_bits, NULL, NULL);
    } else {
        ret = pke_load_modulus_and_pre_monts(dh_para->p, dh_para->p_h, dh_para->p_n0, dh_para->p_bits);
    }
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    step_bytes = pke_get_operand_bytes();
    tmp        = (unsigned int *)(rPKE_A(0u, step_bytes));
    ret        = pke_modexp((unsigned int *)(rPKE_B(3u, step_bytes)), dh_para->q, pubkey, tmp, pWordLen, qWordLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else if (1u != Bigint_Check_1(tmp, pWordLen)) {
        return DH_INVALID_INPUT;
    } else {
    }

    return DH_SUCCESS;
}

/**
 * @brief       DH generate public key from private key.
 * @param[in]   dh_para      - DH_PARA struct pointer.
 * @param[in]   prikey       - private key.
 * @param[in]   pubkey       - public key.
 * @return      0:success     other:error
 * @note
  @verbatim
      -#1. please call dh_param_value_init() before calling this function.
      -#2. the input prikey occupies (dh_para->q_bits+7)/8 bytes;
           the output pubkey occupies (dh_para->p_bits+7)/8 bytes;      
  @endverbatim
 */
unsigned int dh_generate_pubkey_from_prikey(DH_PARA *dh_para, unsigned char *prikey, unsigned char *pubkey)
{
    unsigned int tmp[DH_MAX_WORD_LEN];
    unsigned int x[DH_MAX_WORD_LEN];
    //unsigned int y[DH_MAX_WORD_LEN];
    unsigned int *g;
    unsigned int *y;
    unsigned int  pWordLen, qWordLen, gWordLen;
    unsigned int  pByteLen, qByteLen;
    unsigned int  tmpLen, ret;

    if ((NULL == dh_para) || (NULL == prikey) || (NULL == pubkey)) {
        return DH_POINTER_NULL;
    } else {
        ;
    }

    pWordLen = GET_WORD_LEN(dh_para->p_bits);
    qWordLen = GET_WORD_LEN(dh_para->q_bits);
    gWordLen = GET_WORD_LEN(dh_para->g_bits);
    pByteLen = GET_BYTE_LEN(dh_para->p_bits);
    qByteLen = GET_BYTE_LEN(dh_para->q_bits);

    //get tmp = q-1
    uint32_copy(tmp, dh_para->q, qWordLen);
    tmp[0u] -= 1u;

    x[qWordLen - 1u] = 0u;
    reverse_byte_array((unsigned char *)prikey, (unsigned char *)x, qByteLen);

    // x should be in [2, q-2]
    tmpLen = get_valid_bits(x, qWordLen);
    if (0u == tmpLen) {
        return DH_ZERO_ALL;
    } else if (1u == tmpLen) {
        return DH_VALUE_ONE;
    } else if (uint32_BigNumCmp(x, qWordLen, tmp, qWordLen) >= 0) {
        return DH_INTEGER_TOO_BIG;
    } else {
        ;
    }

    if ((NULL == dh_para->p_h) || (NULL == dh_para->p_n0)) {
        ret = pke_pre_calc_mont_for_modexp(dh_para->p, dh_para->p_bits, NULL, NULL);
    } else {
        ret = pke_load_modulus_and_pre_monts(dh_para->p, dh_para->p_h, dh_para->p_n0, dh_para->p_bits);
    }
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    tmpLen = pke_get_operand_bytes();
    y      = (unsigned int *)(rPKE_A(0u, tmpLen));

    if (gWordLen < pWordLen) {
        g = (unsigned int *)(rPKE_B(0u, tmpLen));
        uint32_copy(g, dh_para->g, gWordLen);
        uint32_clear(g + gWordLen, pWordLen - gWordLen);
    } else {
        g = dh_para->g;
    }

    ret = pke_modexp((unsigned int *)(rPKE_B(3u, tmpLen)), x, g, y, pWordLen, qWordLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    reverse_byte_array((unsigned char *)y, (unsigned char *)pubkey, pByteLen);

    return DH_SUCCESS;
}

/**
 * @brief       DH generate key pair.
 * @param[in]   dh_para      - DH_PARA struct pointer.
 * @param[in]   prikey       - private key.
 * @param[in]   pubkey       - public key.
 * @return      0:success     other:error
 * @note
  @verbatim
      -#1. please call dh_param_value_init() before calling this function.
      -#2. the input prikey occupies (dh_para->q_bits+7)/8 bytes;
           the output pubkey occupies (dh_para->p_bits+7)/8 bytes;      
  @endverbatim
 */
unsigned int dh_generate_key(DH_PARA *dh_para, unsigned char *prikey, unsigned char *pubkey)
{
    unsigned int qByteLen;
    unsigned int tmpBitLen, ret;

    if ((NULL == dh_para) || (NULL == prikey) || (NULL == pubkey)) {
        return DH_POINTER_NULL;
    } else {
        ;
    }

    qByteLen = GET_BYTE_LEN(dh_para->q_bits);

    do {
        ret = get_rand((unsigned char *)prikey, qByteLen);
        if (TRNG_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }

        //make sure prikey has the same bit length as q
        tmpBitLen = (dh_para->q_bits) & 7u;
        if (0u != tmpBitLen) {
            prikey[0u] &= (1u << (tmpBitLen)) - 1u;
        } else {
            ;
        }

        ret = dh_generate_pubkey_from_prikey(dh_para, prikey, pubkey);

        // prikey should be in [2, q-2]
    } while ((DH_ZERO_ALL == ret) || (DH_VALUE_ONE == ret) || (DH_INTEGER_TOO_BIG == ret));

    return ret;
}

#endif
