/********************************************************************************************************
 * @file    ecdsa.c
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
#include "lib/include/pke/pke.h"
#include "lib/include/trng/trng.h"
#include "lib/include/pke/ecdsa.h"
#include "lib/include/crypto_common/eccp_curve.h"
#include "lib/include/trng/trng_basic.h"

#ifdef SUPPORT_ECDSA

/**
 * @brief           Generate ECDSA Signature in unsigned int little-endian big integer style
 * @param[in]       curve                - ecc curve struct pointer, please make sure it is valid.
 * @param[in]       e                    - derived from hash value.
 * @param[in]       k                    - internal random integer k.
 * @param[in]       dA                   - private key.
 * @param[out]      r                    - signature r.
 * @param[out]      s                    - signature s.
 * @return          0:success     other:error
 * @note
 *        1.please make sure e is in [0,n-1], dA is in [1,n-1]
 */
FLAG_STATIC unsigned int ecdsa_sign_uint32(const eccp_curve_t *curve, const unsigned int *e, const unsigned int *k, const unsigned int *dA, unsigned int *r, unsigned int *s)
{
    unsigned int n_wlen;
    unsigned int p_wlen;
    unsigned int tmp1[ECCP_MAX_WORD_LEN];
    unsigned int ret;

    if ((NULL == curve) || (NULL == e) || (NULL == k) || (NULL == dA) || (NULL == r) || (NULL == s))
    {
        return ECDSA_POINTER_NULL;
    }
    else if (curve->eccp_p_bitLen > ECCP_MAX_BIT_LEN)
    {
        return ECDSA_INVALID_INPUT;
    }
    else
    {
        // handle other
    }

    n_wlen = get_word_len(curve->eccp_n_bitLen);
    p_wlen = get_word_len(curve->eccp_p_bitLen);

    // make sure k in [1, n-1]
    ret = uint32_integer_check(k, curve->eccp_n, n_wlen, ECDSA_ZERO_ALL, ECDSA_INTEGER_TOO_BIG, ECDSA_SUCCESS);
    if (ECDSA_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

// get x1
#if !(defined(PKE_LP) || defined(PKE_SECURE))
    if ((NULL != curve->eccp_half_Gx) && (NULL != curve->eccp_half_Gy))
    {
        ret = eccp_pointMul_base(curve, k, tmp1, NULL);
    }
    else
    {
#endif
        ret = eccp_pointmul(curve, k, curve->eccp_Gx, curve->eccp_Gy, tmp1,
                            NULL); // y coordinate is not needed
#if !(defined(PKE_LP) || defined(PKE_SECURE))
    }
#endif
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

// r = x1 mod n
#if (defined(PKE_LP) || defined(PKE_SECURE))
    ret = pke_mod(tmp1, p_wlen, curve->eccp_n, curve->eccp_n_h, curve->eccp_n_n0, n_wlen, r);
#else
    ret = pke_mod(tmp1, p_wlen, curve->eccp_n, curve->eccp_n_h, n_wlen, r);
#endif
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else if (0u != uint32_bignum_check_zero(r, n_wlen)) // make sure r is not zero
    {
        return ECDSA_ZERO_ALL;
    }
    else
    {
        // handle other
    }

#if (defined(PKE_LP) || defined(PKE_SECURE))
    ret = pke_set_modulus_and_pre_monts(curve->eccp_n, curve->eccp_n_h, curve->eccp_n_n0, curve->eccp_n_bitLen);
#else
    ret = pke_set_modulus_and_pre_monts(curve->eccp_n, curve->eccp_n_h, curve->eccp_n_bitLen);
#endif
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

// tmp1 =  r*dA mod n
#if (defined(PKE_LP) || defined(PKE_SECURE))
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
#endif
    ret = pke_modmul_internal(r, dA, tmp1, n_wlen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // tmp1 = e + r*dA mod n
    ret = pke_modadd(curve->eccp_n, e, tmp1, tmp1, n_wlen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // s = k^(-1) mod n
    ret = pke_modinv(curve->eccp_n, k, s, n_wlen, n_wlen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // s = (k^(-1))*(e + r*dA) mod n
    ret = pke_modmul_internal(s, tmp1, s, n_wlen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // make sure s is not zero
    if (0u != uint32_bignum_check_zero(s, n_wlen))
    {
        return ECDSA_ZERO_ALL;
    }
    else
    {
        return ECDSA_SUCCESS;
    }
}

/**
 * @brief           Generate ECDSA Signature in byte string style
 * @param[in]       curve                - ecc curve struct pointer, please make sure it is valid.
 * @param[in]       E                    - derived from hash value.
 * @param[in]       e_len                - byte length of E.
 * @param[in]       rand_k               - random big integer k in signing, big-endian.
 * @param[out]      priKey               - private key, big-endian.
 * @param[out]      signature            - signature r and s, big-endian.
 * @return          0:success     other:error
 */
unsigned int ecdsa_sign(const eccp_curve_t *curve, const unsigned char *E, unsigned int e_len, const unsigned char *rand_k, const unsigned char *priKey, unsigned char *signature)
{
    unsigned int tmp_len;
    unsigned int n_len;
    unsigned int n_wlen;
    unsigned int e[ECCP_MAX_WORD_LEN], k[ECCP_MAX_WORD_LEN], dA[ECCP_MAX_WORD_LEN];
    unsigned int r[ECCP_MAX_WORD_LEN], s[ECCP_MAX_WORD_LEN];
    unsigned int ret;
    unsigned int e_bytelen = e_len;

    if ((NULL == curve) || (NULL == priKey) || (NULL == signature))
    {
        return ECDSA_POINTER_NULL;
    }
    else if (curve->eccp_p_bitLen > ECCP_MAX_BIT_LEN)
    {
        return ECDSA_INVALID_INPUT;
    }
    else
    {
        // handle other
    }

    // E could be zero
    if (NULL == E)
    {
        e_bytelen = 0;
    }
    else
    {
    }

    n_len = get_byte_len(curve->eccp_n_bitLen);
    n_wlen = get_word_len(curve->eccp_n_bitLen);

    // get integer e from hash value E(according to SEC1-V2 2009)
    uint32_clear(e, n_wlen);
    if (curve->eccp_n_bitLen >= (e_bytelen << 3)) // in this case, make E as e directly
    {
        reverse_byte_array(E, (unsigned char *)e, e_bytelen);
    }
    else // in this case, make left eccp_n_bitLen bits of E as e
    {
        reverse_byte_array(E, (unsigned char *)e, n_len);
        tmp_len = (curve->eccp_n_bitLen) & 7u;
        if (0u != tmp_len)
        {
            (void)big_div_2n(e, n_wlen, 8u - tmp_len);
        }
        else
        {
        }
    }

    // get e = e mod n, i.e., make sure e in [0, n-1]
    if (uint32_big_num_cmp(e, n_wlen, curve->eccp_n, n_wlen) >= 0)
    {
        ret = pke_sub(e, curve->eccp_n, e, n_wlen);
        if (PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
        }
    }
    else
    {
    }

    // make sure priKey in [1, n-1]
    dA[n_wlen - 1u] = 0u;
    reverse_byte_array(priKey, (unsigned char *)dA, n_len);
    ret = uint32_integer_check(dA, curve->eccp_n, n_wlen, ECDSA_ZERO_ALL, ECDSA_INTEGER_TOO_BIG, ECDSA_SUCCESS);
    if (ECDSA_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // get k
    if (NULL != rand_k)
    {
        k[n_wlen - 1u] = 0u;
        reverse_byte_array(rand_k, (unsigned char *)k, n_len);
    }
    else
    {
    ECDSA_SIGN_LOOP:

        ret = get_rand((unsigned char *)k, n_len);
        if (TRNG_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            // make sure k has the same bit length as n
            tmp_len = (curve->eccp_n_bitLen) & 0x1Fu;
            if (0u != tmp_len)
            {
                k[n_wlen - 1u] &= (((unsigned int)1) << (tmp_len)) - 1u;
            }
            else
            {
            }
        }
    }

    // sign
    ret = ecdsa_sign_uint32(curve, e, k, dA, r, s);
    if (((ECDSA_ZERO_ALL == ret) || (ECDSA_INTEGER_TOO_BIG == ret)) && (NULL == rand_k))
    {
        goto ECDSA_SIGN_LOOP;
    }
    else
    {
    }

    if (ECDSA_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        reverse_byte_array((unsigned char *)r, signature, n_len);
        reverse_byte_array((unsigned char *)s, &signature[n_len], n_len);

        return ECDSA_SUCCESS;
    }
}

/**
 * @brief           Verify ECDSA Signature in byte string style
 * @param[in]       curve                - ecc curve struct pointer, please make sure it is valid.
 * @param[in]       E                    - hash value, big-endian.
 * @param[in]       e_len                - byte length of E.
 * @param[in]       pubKey               - private key, big-endian.
 * @param[out]      signature            - signature r and s, big-endian.
 * @return          ECDSA_SUCCESS(success)     other:error
 */
unsigned int ecdsa_verify(const eccp_curve_t *curve, const unsigned char *E, unsigned int e_len, const unsigned char *pubKey, const unsigned char *signature)
{
    unsigned int tmp_len;
    unsigned int n_len;
    unsigned int n_wlen;
    unsigned int p_len;
    unsigned int p_wlen;
    unsigned int e[ECCP_MAX_WORD_LEN], r[ECCP_MAX_WORD_LEN], s[ECCP_MAX_WORD_LEN];
    unsigned int tmp[ECCP_MAX_WORD_LEN], x[ECCP_MAX_WORD_LEN];
    unsigned int ret;
    unsigned int e_bytelen = e_len;

    if ((NULL == curve) || (NULL == pubKey) || (NULL == signature))
    {
        return ECDSA_POINTER_NULL;
    }
    else if (curve->eccp_p_bitLen > ECCP_MAX_BIT_LEN)
    {
        return ECDSA_INVALID_INPUT;
    }
    else
    {
        // handle other
    }

    // E could be zero
    if (NULL == E)
    {
        e_bytelen = 0;
    }
    else
    {
    }

    n_len = get_byte_len(curve->eccp_n_bitLen);
    n_wlen = get_word_len(curve->eccp_n_bitLen);
    p_len = get_byte_len(curve->eccp_p_bitLen);
    p_wlen = get_word_len(curve->eccp_p_bitLen);

    // make sure r in [1, n-1]
    r[n_wlen - 1u] = 0u;
    reverse_byte_array(signature, (unsigned char *)r, n_len);
    ret = uint32_integer_check(r, curve->eccp_n, n_wlen, ECDSA_ZERO_ALL, ECDSA_INTEGER_TOO_BIG, ECDSA_SUCCESS);
    if (ECDSA_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // make sure s in [1, n-1]
    s[n_wlen - 1u] = 0u;
    reverse_byte_array(&signature[n_len], (unsigned char *)s, n_len);
    ret = uint32_integer_check(s, curve->eccp_n, n_wlen, ECDSA_ZERO_ALL, ECDSA_INTEGER_TOO_BIG, ECDSA_SUCCESS);
    if (ECDSA_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // tmp = s^(-1) mod n
    ret = pke_modinv(curve->eccp_n, s, tmp, n_wlen, n_wlen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // get integer e from hash value E(according to SEC1-V2 2009)
    uint32_clear(e, n_wlen);
    if (curve->eccp_n_bitLen >= (e_bytelen << 3)) // in this case, make E as e directly
    {
        reverse_byte_array(E, (unsigned char *)e, e_bytelen);
    }
    else // in this case, make left eccp_n_bitLen bits of E as e
    {
        memcpy_((unsigned char *)e, E, n_len);
        reverse_byte_array(E, (unsigned char *)e, n_len);
        tmp_len = (curve->eccp_n_bitLen) & 7u;
        if (0u != tmp_len)
        {
            (void)big_div_2n(e, n_wlen, 8u - tmp_len);
        }
        else
        {
        }
    }

    // get e = e mod n, i.e., make sure e in [0, n-1]
    if (uint32_big_num_cmp(e, n_wlen, curve->eccp_n, n_wlen) >= 0)
    {
        ret = pke_sub(e, curve->eccp_n, e, n_wlen);
        if (PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
        }
    }
    else
    {
    }

#if (defined(PKE_LP) || defined(PKE_SECURE))
    ret = pke_set_modulus_and_pre_monts(curve->eccp_n, curve->eccp_n_h, curve->eccp_n_n0, curve->eccp_n_bitLen);
#else
    ret = pke_set_modulus_and_pre_monts(curve->eccp_n, curve->eccp_n_h, curve->eccp_n_bitLen);
#endif
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

// x =  e*(s^(-1)) mod n
#if (defined(PKE_LP) || defined(PKE_SECURE))
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
#endif
    ret = pke_modmul_internal(e, tmp, x, n_wlen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // tmp =  r*(s^(-1)) mod n
    ret = pke_modmul_internal(r, tmp, tmp, n_wlen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // check public key
    e[p_wlen - 1u] = 0u;
    s[p_wlen - 1u] = 0u;
    reverse_byte_array(pubKey, (unsigned char *)e, p_len);
    reverse_byte_array(&pubKey[p_len], (unsigned char *)s, p_len);
    ret = eccp_pointverify(curve, e, s);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

#if (defined(PKE_HP) || defined(PKE_UHP))
    ret = eccp_pointMul_Shamir_safe(curve, tmp, e, s, x, curve->eccp_Gx, curve->eccp_Gy, e, s);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }
#else
    ret = eccp_pointmul(curve, tmp, e, s, e, s);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    if (0u == uint32_bignum_check_zero(x, n_wlen))
    {
        ret = eccp_pointmul(curve, x, curve->eccp_Gx, curve->eccp_Gy, x, tmp);
        if (PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
        }

        ret = eccp_pointadd_safe(curve, e, s, x, tmp, e, s);
        if (PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
        }
    }
    else
    {
    }
#endif

// x = x1 mod n
#if (defined(PKE_LP) || defined(PKE_SECURE))
    ret = pke_mod(e, p_wlen, curve->eccp_n, curve->eccp_n_h, curve->eccp_n_n0, n_wlen, tmp);
#else
    ret = pke_mod(e, p_wlen, curve->eccp_n, curve->eccp_n_h, n_wlen, tmp);
#endif
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    if (0 != uint32_big_num_cmp(tmp, n_wlen, r, n_wlen))
    {
        return ECDSA_VERIFY_FAILED;
    }
    else
    {
        return ECDSA_SUCCESS;
    }
}

#endif
