/********************************************************************************************************
 * @file    ecdsa.c
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


#ifdef SUPPORT_ECDSA

    #include "lib/include/pke/ecdsa.h"
    #include "lib/include/crypto_common/utility.h"
    #include "lib/include/trng/trng.h"

/**
 * @brief       Generate ECDSA Signature in U32 little-endian big integer style
 * @param[in]   curve         - ecc curve struct pointer, please make sure it is valid.
 * @param[in]   e             - derived from hash value.
 * @param[in]   k             - internal random integer k.
 * @param[in]   dA            - private key.
 * @param[out]  r             - signature r.
 * @param[out]  s             - signature s.
 * @return      0:success     other:error
 * @note
   @verbatim
      -# 1.please make sure e is in [0,n-1], dA is in [1,n-1]
   @endverbatim
 */
unsigned int ecdsa_sign_uint32(eccp_curve_t *curve, unsigned int *e, unsigned int *k, unsigned int *dA, unsigned int *r, unsigned int *s)
{
    unsigned int nWordLen;
    unsigned int pWordLen;
    unsigned int tmp1[ECCP_MAX_WORD_LEN];
    unsigned int ret;

    if ((NULL == curve) || (NULL == e) || (NULL == k) || (NULL == dA) || (NULL == r) || (NULL == s)) {
        return ECDSA_POINTOR_NULL;
    } else if (curve->eccp_p_bitLen > ECCP_MAX_BIT_LEN) {
        return ECDSA_INVALID_INPUT;
    } else {
        ;
    }

    nWordLen = GET_WORD_LEN(curve->eccp_n_bitLen);
    pWordLen = GET_WORD_LEN(curve->eccp_p_bitLen);

    //make sure k in [1, n-1]
    ret = uint32_integer_check(k, curve->eccp_n, nWordLen, ECDSA_ZERO_ALL, ECDSA_INTEGER_TOO_BIG, ECDSA_SUCCESS);
    if (ECDSA_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //get x1
    #if !(defined(PKE_LP) || defined(PKE_SECURE))
    if ((NULL != curve->eccp_half_Gx) && (NULL != curve->eccp_half_Gy)) {
        ret = eccp_pointMul_base(curve, k, tmp1, NULL);
    } else {
    #endif
        ret = eccp_pointMul(curve, k, curve->eccp_Gx, curve->eccp_Gy, tmp1, NULL); //y coordinate is not needed
    #if !(defined(PKE_LP) || defined(PKE_SECURE))
    }
    #endif
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //r = x1 mod n
    #if (defined(PKE_LP) || defined(PKE_SECURE))
    ret = pke_mod(tmp1, pWordLen, curve->eccp_n, curve->eccp_n_h, curve->eccp_n_n0, nWordLen, r);
    #else
    ret = pke_mod(tmp1, pWordLen, curve->eccp_n, curve->eccp_n_h, nWordLen, r);
    #endif
    if (PKE_SUCCESS != ret) {
        return ret;
    } else if (0u != uint32_BigNum_Check_Zero(r, nWordLen)) //make sure r is not zero
    {
        return ECDSA_ZERO_ALL;
    } else {
        ;
    }

    #if (defined(PKE_LP) || defined(PKE_SECURE))
    ret = pke_set_modulus_and_pre_monts(curve->eccp_n, curve->eccp_n_h, curve->eccp_n_n0, curve->eccp_n_bitLen);
    #else
    ret = pke_set_modulus_and_pre_monts(curve->eccp_n, curve->eccp_n_h, curve->eccp_n_bitLen);
    #endif
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //tmp1 =  r*dA mod n
    #if (defined(PKE_LP) || defined(PKE_SECURE))
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
    #endif
    ret = pke_modmul_internal(r, dA, tmp1, nWordLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //tmp1 = e + r*dA mod n
    ret = pke_modadd(curve->eccp_n, e, tmp1, tmp1, nWordLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //s = k^(-1) mod n
    ret = pke_modinv(curve->eccp_n, k, s, nWordLen, nWordLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //s = (k^(-1))*(e + r*dA) mod n
    ret = pke_modmul_internal(s, tmp1, s, nWordLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //make sure s is not zero
    if (0u != uint32_BigNum_Check_Zero(s, nWordLen)) {
        return ECDSA_ZERO_ALL;
    } else {
        return ECDSA_SUCCESS;
    }
}

/**
 * @brief       Generate ECDSA Signature in byte string style
 * @param[in]   curve         - ecc curve struct pointer, please make sure it is valid.
 * @param[in]   E             - derived from hash value.
 * @param[in]   EByteLen      - byte length of E.
 * @param[in]   rand_k        - random big integer k in signing, big-endian.
 * @param[out]  priKey        - private key, big-endian.
 * @param[out]  signature     - ignature r and s, big-endian.
 * @return      0:success     other:error
 */
unsigned int ecdsa_sign(eccp_curve_t *curve, unsigned char *E, unsigned int EByteLen, unsigned char *rand_k, unsigned char *priKey, unsigned char *signature)
{
    unsigned int tmpLen;
    unsigned int nByteLen;
    unsigned int nWordLen;
    unsigned int e[ECCP_MAX_WORD_LEN], k[ECCP_MAX_WORD_LEN], dA[ECCP_MAX_WORD_LEN];
    unsigned int r[ECCP_MAX_WORD_LEN], s[ECCP_MAX_WORD_LEN];
    unsigned int ret;

    if ((NULL == curve) || (NULL == priKey) || (NULL == signature)) {
        return ECDSA_POINTOR_NULL;
    } else if (curve->eccp_p_bitLen > ECCP_MAX_BIT_LEN) {
        return ECDSA_INVALID_INPUT;
    } else {
        ;
    }

    //E could be zero
    if (NULL == E) {
        EByteLen = 0;
    } else {
        ;
    }

    nByteLen = GET_BYTE_LEN(curve->eccp_n_bitLen);
    nWordLen = GET_WORD_LEN(curve->eccp_n_bitLen);

    //get integer e from hash value E(according to SEC1-V2 2009)
    uint32_clear(e, nWordLen);
    if (curve->eccp_n_bitLen >= (EByteLen << 3)) //in this case, make E as e directly
    {
        reverse_byte_array((unsigned char *)E, (unsigned char *)e, EByteLen);
    } else                                       //in this case, make left eccp_n_bitLen bits of E as e
    {
        reverse_byte_array((unsigned char *)E, (unsigned char *)e, nByteLen);
        tmpLen = (curve->eccp_n_bitLen) & 7u;
        if (0u != tmpLen) {
            Big_Div2n(e, nWordLen, 8u - tmpLen);
        } else {
            ;
        }
    }

    //get e = e mod n, i.e., make sure e in [0, n-1]
    if (uint32_BigNumCmp(e, nWordLen, curve->eccp_n, nWordLen) >= 0) {
        ret = pke_sub(e, curve->eccp_n, e, nWordLen);
        if (PKE_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }
    } else {
        ;
    }

    //make sure priKey in [1, n-1]
    dA[nWordLen - 1u] = 0u;
    reverse_byte_array((unsigned char *)priKey, (unsigned char *)dA, nByteLen);
    ret = uint32_integer_check(dA, curve->eccp_n, nWordLen, ECDSA_ZERO_ALL, ECDSA_INTEGER_TOO_BIG, ECDSA_SUCCESS);
    if (ECDSA_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //get k
    if (NULL != rand_k) {
        k[nWordLen - 1u] = 0u;
        reverse_byte_array(rand_k, (unsigned char *)k, nByteLen);
    } else {
ECDSA_SIGN_LOOP:

        ret = get_rand((unsigned char *)k, nByteLen);
        if (TRNG_SUCCESS != ret) {
            return ret;
        } else {
            //make sure k has the same bit length as n
            tmpLen = (curve->eccp_n_bitLen) & 0x1Fu;
            if (0u != tmpLen) {
                k[nWordLen - 1u] &= (((unsigned int)1) << (tmpLen)) - 1u;
            } else {
                ;
            }
        }
    }

    //sign
    ret = ecdsa_sign_uint32(curve, e, k, dA, r, s);
    if (((ECDSA_ZERO_ALL == ret) || (ECDSA_INTEGER_TOO_BIG == ret)) && (NULL == rand_k)) {
        goto ECDSA_SIGN_LOOP;
    } else {
        ;
    }

    if (ECDSA_SUCCESS != ret) {
        return ret;
    } else {
        reverse_byte_array((unsigned char *)r, signature, nByteLen);
        reverse_byte_array((unsigned char *)s, signature + nByteLen, nByteLen);

        return ECDSA_SUCCESS;
    }
}

/**
 * @brief       Verify ECDSA Signature in byte string style
 * @param[in]   curve               - ecc curve struct pointer, please make sure it is valid.
 * @param[in]   E                   - hash value, big-endian.
 * @param[in]   EByteLen            - byte length of E.
 * @param[in]   pubKey              - private key, big-endian.
 * @param[out]  signature           - signature r and s, big-endian.
 * @return      ECDSA_SUCCESS(success)     other:error
 */
unsigned int ecdsa_verify(eccp_curve_t *curve, unsigned char *E, unsigned int EByteLen, unsigned char *pubKey, unsigned char *signature)
{
    unsigned int tmpLen;
    unsigned int nByteLen;
    unsigned int nWordLen;
    unsigned int pByteLen;
    unsigned int pWordLen;
    unsigned int e[ECCP_MAX_WORD_LEN], r[ECCP_MAX_WORD_LEN], s[ECCP_MAX_WORD_LEN];
    unsigned int tmp[ECCP_MAX_WORD_LEN], x[ECCP_MAX_WORD_LEN];
    unsigned int ret;

    if ((NULL == curve) || (NULL == pubKey) || (NULL == signature)) {
        return ECDSA_POINTOR_NULL;
    } else if (curve->eccp_p_bitLen > ECCP_MAX_BIT_LEN) {
        return ECDSA_INVALID_INPUT;
    } else {
        ;
    }

    //E could be zero
    if (NULL == E) {
        EByteLen = 0;
    } else {
        ;
    }

    nByteLen = GET_BYTE_LEN(curve->eccp_n_bitLen);
    nWordLen = GET_WORD_LEN(curve->eccp_n_bitLen);
    pByteLen = GET_BYTE_LEN(curve->eccp_p_bitLen);
    pWordLen = GET_WORD_LEN(curve->eccp_p_bitLen);

    //make sure r in [1, n-1]
    r[nWordLen - 1u] = 0u;
    reverse_byte_array(signature, (unsigned char *)r, nByteLen);
    ret = uint32_integer_check(r, curve->eccp_n, nWordLen, ECDSA_ZERO_ALL, ECDSA_INTEGER_TOO_BIG, ECDSA_SUCCESS);
    if (ECDSA_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //make sure s in [1, n-1]
    s[nWordLen - 1u] = 0u;
    reverse_byte_array(signature + nByteLen, (unsigned char *)s, nByteLen);
    ret = uint32_integer_check(s, curve->eccp_n, nWordLen, ECDSA_ZERO_ALL, ECDSA_INTEGER_TOO_BIG, ECDSA_SUCCESS);
    if (ECDSA_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //tmp = s^(-1) mod n
    ret = pke_modinv(curve->eccp_n, s, tmp, nWordLen, nWordLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //get integer e from hash value E(according to SEC1-V2 2009)
    uint32_clear(e, nWordLen);
    if (curve->eccp_n_bitLen >= (EByteLen << 3)) //in this case, make E as e directly
    {
        reverse_byte_array((unsigned char *)E, (unsigned char *)e, EByteLen);
    } else                                       //in this case, make left eccp_n_bitLen bits of E as e
    {
        memcpy_((unsigned char *)e, E, nByteLen);
        reverse_byte_array((unsigned char *)E, (unsigned char *)e, nByteLen);
        tmpLen = (curve->eccp_n_bitLen) & 7u;
        if (0u != tmpLen) {
            Big_Div2n(e, nWordLen, 8u - tmpLen);
        } else {
            ;
        }
    }

    //get e = e mod n, i.e., make sure e in [0, n-1]
    if (uint32_BigNumCmp(e, nWordLen, curve->eccp_n, nWordLen) >= 0) {
        ret = pke_sub(e, curve->eccp_n, e, nWordLen);
        if (PKE_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }
    } else {
        ;
    }

    #if (defined(PKE_LP) || defined(PKE_SECURE))
    ret = pke_set_modulus_and_pre_monts(curve->eccp_n, curve->eccp_n_h, curve->eccp_n_n0, curve->eccp_n_bitLen);
    #else
    ret = pke_set_modulus_and_pre_monts(curve->eccp_n, curve->eccp_n_h, curve->eccp_n_bitLen);
    #endif
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //x =  e*(s^(-1)) mod n
    #if (defined(PKE_LP) || defined(PKE_SECURE))
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
    #endif
    ret = pke_modmul_internal(e, tmp, x, nWordLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //tmp =  r*(s^(-1)) mod n
    ret = pke_modmul_internal(r, tmp, tmp, nWordLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //check public key
    e[pWordLen - 1u] = 0u;
    s[pWordLen - 1u] = 0u;
    reverse_byte_array(pubKey, (unsigned char *)e, pByteLen);
    reverse_byte_array(pubKey + pByteLen, (unsigned char *)s, pByteLen);
    ret = eccp_pointVerify(curve, e, s);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    #if (defined(PKE_HP) || defined(PKE_UHP))
    ret = eccp_pointMul_Shamir_safe(curve,
                                    tmp,
                                    e,
                                    s,
                                    x,
                                    curve->eccp_Gx,
                                    curve->eccp_Gy,
                                    e,
                                    s);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }
    #else
    ret = eccp_pointMul(curve, tmp, e, s, e, s);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    if (0u == uint32_BigNum_Check_Zero(x, nWordLen)) {
        ret = eccp_pointMul(curve, x, curve->eccp_Gx, curve->eccp_Gy, x, tmp);
        if (PKE_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }

        ret = eccp_pointAdd_safe(curve, e, s, x, tmp, e, s);
        if (PKE_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }
    } else {
        ;
    }
    #endif

    //x = x1 mod n
    #if (defined(PKE_LP) || defined(PKE_SECURE))
    ret = pke_mod(e, pWordLen, curve->eccp_n, curve->eccp_n_h, curve->eccp_n_n0, nWordLen, tmp);
    #else
    ret = pke_mod(e, pWordLen, curve->eccp_n, curve->eccp_n_h, nWordLen, tmp);
    #endif
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    if (0 != uint32_BigNumCmp(tmp, nWordLen, r, nWordLen)) {
        return ECDSA_VERIFY_FAILED;
    } else {
        return ECDSA_SUCCESS;
    }
}

#endif
