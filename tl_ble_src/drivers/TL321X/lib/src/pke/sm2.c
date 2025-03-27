/********************************************************************************************************
 * @file    sm2.c
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

#include "lib/include/pke/pke_config.h"


#ifdef SUPPORT_SM2

#include "lib/include/pke/sm2.h"
#include "lib/include/hash/hash_kdf.h"
#include "lib/include/trng/trng.h"
#include "lib/include/crypto_common/utility.h"


extern  unsigned int sm2p256v1_Gx[8];
extern  unsigned int sm2p256v1_Gy[8];
extern  unsigned int sm2p256v1_n[8];
extern  unsigned int sm2p256v1_n_h[8];
#if (defined(PKE_LP) || defined(PKE_SECURE))
extern  unsigned int sm2p256v1_n_n0[1];
#endif
extern  unsigned int sm2p256v1_n_1[8];

//extern  eccp_curve_t sm2_curve[1];




/**
 * @brief       Generate SM2 Signature r and s with rand k
 * @param[in]   e[8]         - e value, 8 words, little-endian
 * @param[in]   k[8]         - random number k, 8 words, little-endian
 * @param[in]   dA[8]        - private key, 8 words, little-endian
 * @param[out]  r[8]         - Signature r, 8 words, little-endian
 * @param[in]   s[8]         - Signature s, 8 words, little-endian
 * @return      SM2_SUCCESS(success)     other:error
 * @note
  @verbatim
      -# 1.e and dA can not be modified
      -# 2.e must be less than n(order of the SM2 curve)
      -# 3.dA must be in [1, n-2]
  @endverbatim
 */
unsigned int sm2_sign_with_k(unsigned int e[8], unsigned int k[8], unsigned int dA[8], unsigned int r[8], unsigned int s[8])
{
    unsigned int tmp1[SM2_WORD_LEN], tmp2[SM2_WORD_LEN];
    unsigned int ret;

    if((NULL == e) || (NULL == k) || (NULL == dA) || (NULL == r) || (NULL == s))
    {
        return SM2_BUFFER_NULL;
    }
    else
    {;}

    //make sure k in [1, n-1]
    ret = uint32_integer_check(k, sm2p256v1_n, SM2_WORD_LEN, SM2_ZERO_ALL, SM2_INTEGER_TOO_BIG,
            SM2_SUCCESS);
    if(SM2_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

#ifdef SM2_HIGH_SPEED
    ret = eccp_pointMul_base((eccp_curve_t *)sm2_curve, k, tmp1, NULL);
#else
    ret = eccp_pointMul(sm2_curve, k, sm2_curve->eccp_Gx, sm2_curve->eccp_Gy, tmp1, NULL);
#endif
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //tmp1 = x1 mod n
    if(uint32_BigNumCmp(tmp1, SM2_WORD_LEN, sm2p256v1_n, SM2_WORD_LEN) >= 0)
    {
        ret = pke_sub(tmp1, sm2p256v1_n, tmp1, SM2_WORD_LEN);
        if(PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}
    }
    else
    {;}

    //r = e + x1 mod n
    ret = pke_modadd(sm2p256v1_n, e, tmp1, r, SM2_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //make sure r is not zero
    if(0u != uint32_BigNum_Check_Zero(r, SM2_WORD_LEN))
    {
        return SM2_ZERO_ALL;
    }
    else
    {;}

    //tmp1 = r + k mod n
    ret = pke_modadd(sm2p256v1_n, r, k, tmp1, SM2_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else if(0u != uint32_BigNum_Check_Zero(tmp1, SM2_WORD_LEN))   //make sure r+k is not n
    {
        return SM2_ZERO_ALL;
    }
    else
    {;}

#if (defined(PKE_LP) || defined(PKE_SECURE))
    ret = pke_load_modulus_and_pre_monts(sm2p256v1_n, sm2p256v1_n_h, sm2p256v1_n_n0, SM2_BIT_LEN);
#else
    ret = pke_load_modulus_and_pre_monts((unsigned int *)sm2p256v1_n, (unsigned int *)sm2p256v1_n_h, SM2_BIT_LEN);
#endif
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //tmp1 =  r*dA mod n
#if (defined(PKE_LP) || defined(PKE_SECURE))
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
#endif
    ret = pke_modmul_internal(r, dA, tmp1, SM2_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //tmp1 =  (k - r*dA) mod n
    ret = pke_modsub(sm2p256v1_n, k, tmp1, tmp1, SM2_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //tmp2 = 1+dA
    uint32_copy(tmp2, dA, SM2_WORD_LEN);
    ret = uint32_big_num_little_endian_add_little(tmp2, SM2_WORD_LEN, 1u, (unsigned char)1);
    if(0u != ret)
    {
        return PKE_INTEGER_TOO_BIG;
    }
    else
    {
        ;
    }

    //tmp2 = (1+dA)^(-1) mod n
    ret = pke_modinv(sm2p256v1_n, tmp2, tmp2, SM2_WORD_LEN, SM2_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

#if 0
#if (defined(PKE_LP) || defined(PKE_SECURE))
    ret = pke_load_modulus_and_pre_monts((unsigned int *)sm2p256v1_n, (unsigned int *)sm2p256v1_n_h, (unsigned int *)sm2p256v1_n_n0, SM2_BIT_LEN);
#else
    ret = pke_load_modulus_and_pre_monts((unsigned int *)sm2p256v1_n, (unsigned int *)sm2p256v1_n_h, SM2_BIT_LEN);
#endif
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }
#endif

    //s = ((1+dA)^(-1))*(k - r*dA) mod n
    ret = pke_modmul_internal(tmp1, tmp2, s, SM2_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //make sure s is not zero
    if(0u != uint32_BigNum_Check_Zero(s, SM2_WORD_LEN))
    {
        return SM2_ZERO_ALL;
    }
    else
    {
        return SM2_SUCCESS;
    }
}


/**
 * @brief       Generate SM2 Signature
 * @param[in]   E[32]          - input, E value, 32 bytes, big-endian
 * @param[in]   rand_k[32]     - input, random big integer k in signing, 32 bytes, big-endian,
 *                               if you do not have this integer, please set this parameter to be NULL,
 *                               it will be generated inside.
 * @param[in]   priKey[32]     - private key, 32 bytes, big-endian
 * @param[out]  signature[64]  - Signature r and s, 64 bytes, big-endian
 * @return      SM2_SUCCESS(success)     other:error
 * @note
  @verbatim
      -# 1.if you do not have rand_k, please set the parameter to be NULL, it will be generated inside.
  @endverbatim
 */
unsigned int sm2_sign(unsigned char E[32], unsigned char rand_k[32], unsigned char priKey[32], unsigned char signature[64])
{
    unsigned int e[SM2_WORD_LEN], k[SM2_WORD_LEN], dA[SM2_WORD_LEN], r[SM2_WORD_LEN], s[SM2_WORD_LEN];
    unsigned int ret;

    if((NULL == E) || (NULL == priKey) || (NULL == signature))
    {
        return SM2_BUFFER_NULL;
    }
    else
    {;}

    //e = e mod n
#ifdef PKE_BIG_ENDIAN
    reverse_word_array(E, e, SM2_WORD_LEN);
#else
    reverse_byte_array(E, (unsigned char *)e, SM2_BYTE_LEN);
#endif
    if(uint32_BigNumCmp(e, SM2_WORD_LEN, sm2p256v1_n, SM2_WORD_LEN) >= 0)
    {
        ret = pke_sub(e, sm2p256v1_n, e, SM2_WORD_LEN);
        if(PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}
    }
    else
    {;}

    //make sure priKey in [1, n-2]
#ifdef PKE_BIG_ENDIAN
    reverse_word_array(priKey, dA, SM2_WORD_LEN);
#else
    reverse_byte_array(priKey, (unsigned char *)dA, SM2_BYTE_LEN);
#endif
    ret = uint32_integer_check(dA, sm2p256v1_n_1, SM2_WORD_LEN, SM2_ZERO_ALL, SM2_INTEGER_TOO_BIG,
            SM2_SUCCESS);
    if(SM2_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    if(NULL == rand_k)
    {
        do {
            ret = get_rand((unsigned char *)k, SM2_BYTE_LEN);
            if(TRNG_SUCCESS != ret)
            {
                break;
            }
            else
            {;}

            ret = sm2_sign_with_k(e, k, dA, r, s);
        } while((SM2_ZERO_ALL == ret) || (SM2_INTEGER_TOO_BIG == ret));
    }
    else
    {
        reverse_byte_array(rand_k, (unsigned char *)k, SM2_BYTE_LEN);

        ret = sm2_sign_with_k(e, k, dA, r, s);
    }

    if(SM2_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
#ifdef PKE_BIG_ENDIAN
        if(0u != (((unsigned int)(signature)) & 3u))
        {
            reverse_word_array((unsigned char *)r, r, SM2_WORD_LEN);
            reverse_word_array((unsigned char *)s, s, SM2_WORD_LEN);
            memcpy_(signature, r, SM2_BYTE_LEN);
            memcpy_(signature+SM2_BYTE_LEN, s, SM2_BYTE_LEN);
        }
        else
        {
            reverse_word_array((unsigned char *)r, (unsigned int *)signature, SM2_WORD_LEN);
            reverse_word_array((unsigned char *)s, (unsigned int *)(signature+SM2_BYTE_LEN), SM2_WORD_LEN);
        }
#else
        reverse_byte_array((unsigned char *)r, signature, SM2_BYTE_LEN);
        reverse_byte_array((unsigned char *)s, signature+SM2_BYTE_LEN, SM2_BYTE_LEN);
#endif

        return SM2_SUCCESS;
    }
}


/**
 * @brief       Verify SM2 Signature
 * @param[in]   E[32]          - E value, 32 bytes, big-endian
 * @param[in]   pubKey[65]     - public key(0x04 + x + y), 65 bytes, big-endian
 * @param[in]   signature[64]  - Signature r and s, 64 bytes, big-endian
 * @return      SM2_SUCCESS(success)     other:error
 */
unsigned int sm2_verify(unsigned char E[32], unsigned char pubKey[65], unsigned char signature[64])
{
    unsigned int e[SM2_WORD_LEN], r[SM2_WORD_LEN], s[SM2_WORD_LEN], tmp[SM2_WORD_LEN<<2];
    unsigned int *t = e;
    unsigned int ret;

    if((NULL == E) || (NULL == pubKey) || (NULL == signature))
    {
        return SM2_BUFFER_NULL;
    }
    else if(POINT_UNCOMPRESSED != pubKey[0])    //make sure pubKey[0] is POINT_UNCOMPRESSED
    {
        return SM2_INPUT_INVALID;
    }
    else
    {;}

    //get PA and check PA
#ifdef PKE_BIG_ENDIAN
    reverse_word_array(pubKey+1u, tmp+2u*SM2_WORD_LEN, SM2_WORD_LEN);
    reverse_word_array(pubKey+1u+SM2_BYTE_LEN, tmp+(3u*SM2_WORD_LEN), SM2_WORD_LEN);
#else
    reverse_byte_array(pubKey+1u, (unsigned char *)(tmp+(2u*SM2_WORD_LEN)), SM2_BYTE_LEN);
    reverse_byte_array(pubKey+1u+SM2_BYTE_LEN, (unsigned char *)(tmp+(3u*SM2_WORD_LEN)), SM2_BYTE_LEN);
#endif
    ret = eccp_pointVerify(sm2_curve, (tmp+(2u*SM2_WORD_LEN)), (tmp+(3u*SM2_WORD_LEN)));
    if(PKE_SUCCESS != ret)
    {
        return SM2_NOT_ON_CURVE;
    }
    else
    {;}

    //make sure r in [1, n-1]
#ifdef PKE_BIG_ENDIAN
    reverse_word_array(signature, r, SM2_WORD_LEN);
#else
    reverse_byte_array(signature, (unsigned char *)r, SM2_BYTE_LEN);
#endif
    ret = uint32_integer_check(r, sm2p256v1_n, SM2_WORD_LEN, SM2_ZERO_ALL, SM2_INTEGER_TOO_BIG,
            SM2_SUCCESS);
    if(SM2_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

    //make sure s in [1, n-1]
#ifdef PKE_BIG_ENDIAN
    reverse_word_array(signature+SM2_BYTE_LEN, s, SM2_WORD_LEN);
#else
    reverse_byte_array(signature+SM2_BYTE_LEN, (unsigned char *)s, SM2_BYTE_LEN);
#endif
    ret = uint32_integer_check(s, sm2p256v1_n, SM2_WORD_LEN, SM2_ZERO_ALL, SM2_INTEGER_TOO_BIG,
            SM2_SUCCESS);
    if(SM2_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

    //t = (r+s) mod n
    ret = pke_modadd(sm2p256v1_n, r, s, t, SM2_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

    //if t is 0, refuse the signature
    if(0u != uint32_BigNum_Check_Zero(t, SM2_WORD_LEN))
    {
        ret = SM2_ZERO_ALL;
        goto END;
    }
    else
    {;}

#ifdef SM2_HIGH_SPEED
    ret = eccp_pointMul_Shamir_safe((eccp_curve_t *)sm2_curve,
                                    s, (unsigned int *)sm2p256v1_Gx, (unsigned int *)sm2p256v1_Gy,
                                    t, tmp+(2u*SM2_WORD_LEN), tmp+(3u*SM2_WORD_LEN),
                                    tmp, NULL);
#else
    //[s]G
    ret = eccp_pointMul(sm2_curve, s, sm2_curve->eccp_Gx, sm2_curve->eccp_Gy, tmp, tmp+SM2_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

    //[t]PA
    ret = eccp_pointMul(sm2_curve, t, tmp+2u*SM2_WORD_LEN, tmp+3u*SM2_WORD_LEN, tmp+2u*SM2_WORD_LEN,
                        tmp+3u*SM2_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

    //[s]G + [t]PA
    ret = eccp_pointAdd_safe(sm2_curve, tmp, tmp+SM2_WORD_LEN, tmp+2u*SM2_WORD_LEN, tmp+3u*SM2_WORD_LEN,
                        tmp, NULL);
#endif
    if(PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

    //e = e mod n
#ifdef PKE_BIG_ENDIAN
    reverse_word_array(E, e, SM2_WORD_LEN);
#else
    reverse_byte_array(E, (unsigned char *)e, SM2_BYTE_LEN);
#endif
    if(uint32_BigNumCmp(e, SM2_WORD_LEN, sm2p256v1_n, SM2_WORD_LEN) >= 0)
    {
        ret = pke_sub(e, sm2p256v1_n, e, SM2_WORD_LEN);
        if(PKE_SUCCESS != ret)
        {
            goto END;
        }
        else
        {;}
    }
    else
    {;}

    //tmp = x1 mod n
    if(uint32_BigNumCmp(tmp, SM2_WORD_LEN, sm2p256v1_n, SM2_WORD_LEN) >= 0)
    {
        ret = pke_sub(tmp, sm2p256v1_n, tmp, SM2_WORD_LEN);
        if(PKE_SUCCESS != ret)
        {
            goto END;
        }
        else
        {;}
    }
    else
    {;}

    //tmp = e + x1 mod n
    ret = pke_modadd(sm2p256v1_n, e, tmp, tmp, SM2_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

    //cmp
    if(0 != uint32_BigNumCmp(tmp, SM2_WORD_LEN, r, SM2_WORD_LEN))
    {
        ret = SM2_VERIFY_FAILED;
        goto END;
    }
    else
    {;}

    //success
    ret = SM2_SUCCESS;

END:

    return ret;
}


/**
 * @brief       SM2 Encryption with rand k
 * @param[in]   M             - plaintext, MByteLen bytes, big-endian
 * @param[in]   MByteLen      - byte length of M
 * @param[in]   k[8]          - random number k, 8 words, little-endian
 * @param[in]   pubkey_x      - x coordinate of public key point, 8 words, little-endian
 * @param[in]   pubkey_y      - y coordinate of public key point, 8 words, little-endian
 * @param[in]   order         - either SM2_C1C3C2 or SM2_C1C2C3
 * @param[out]  C             - ciphertext, CByteLen bytes, big-endian   
 * @param[out]  CByteLen      - byte length of C, should be MByteLen+97 if success
 * @return      SM2_SUCCESS(success)     other:error
 * @note
  @verbatim
      -# 1.M and C can be the same buffer.
      -# 2.please make sure pubkey_x and pubkey_y are valid.
  @endverbatim
 */
unsigned int sm2_encrypt_with_k(unsigned char *M, unsigned int MByteLen, unsigned int *k,
                            unsigned int *pubkey_x, unsigned int *pubkey_y,
                            sm2_cipher_order_e order,
                            unsigned char *C, unsigned int *CByteLen)
{
    unsigned char counter[4] = {0,0,0,1};
    unsigned int xy[SM2_WORD_LEN<<1];
    unsigned char *C2, *C3;
    int32_t i;
    unsigned int ret;

    HASH_NODE hash_node[3];  //since M and C may point the same address, please do not initialize hash_node here.

    if((NULL == M) || (NULL == k) || (NULL == pubkey_x) || (NULL == pubkey_y) || (NULL == C) || (NULL == CByteLen))
    {
        return SM2_BUFFER_NULL;
    }
    else if(0u == MByteLen)
    {
        return SM2_INPUT_INVALID;
    }
    else if(order > SM2_C1C2C3)
    {
        return SM2_INPUT_INVALID;
    }
    else
    {;}

    C2 = C+1u+(2u*SM2_BYTE_LEN) + ((SM2_C1C2C3 == order)?0u:SM2_BYTE_LEN);
    C3 = C+1u+(2u*SM2_BYTE_LEN) + ((SM2_C1C2C3 == order)?MByteLen:0u);

    //not support M and C crossing, but support M = C
    if(M > C)
    {
        if((C + MByteLen+1u+(3u*SM2_BYTE_LEN)) > M)
        {
            return SM2_INPUT_INVALID;
        }
        else
        {;}
    }
    else if(M < C)
    {
        if((M + MByteLen) > C)
        {
            return SM2_INPUT_INVALID;
        }
        else
        {;}
    }
    else  //M = C
    {
        //move M to C2, and now M = C2
        for(i=(int32_t)(MByteLen-1u); i>=0; i--)
        {
            C2[i] = M[i];
        }

        M = C2;
    }

    //make sure k in [1, n-1]
    ret = uint32_integer_check(k, sm2p256v1_n, SM2_WORD_LEN, SM2_ZERO_ALL, SM2_INTEGER_TOO_BIG,
            SM2_SUCCESS);
    if(SM2_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //get [k]G
#ifdef SM2_HIGH_SPEED
    ret = eccp_pointMul_base((eccp_curve_t *)sm2_curve, k, xy, xy+SM2_WORD_LEN);
#else
    ret = eccp_pointMul(sm2_curve, k, sm2_curve->eccp_Gx, sm2_curve->eccp_Gy, xy, xy+SM2_WORD_LEN);
#endif
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //output C1
    C[0] = POINT_UNCOMPRESSED;
#ifdef PKE_BIG_ENDIAN
    reverse_word_array((unsigned char *)xy, xy, SM2_WORD_LEN);
    reverse_word_array((unsigned char *)(xy+SM2_WORD_LEN), xy+SM2_WORD_LEN, SM2_WORD_LEN);
    memcpy_(C+1u, xy, SM2_BYTE_LEN);
    memcpy_(C+1u+SM2_BYTE_LEN, xy+SM2_WORD_LEN, SM2_BYTE_LEN);
#else
    reverse_byte_array((unsigned char *)xy, C+1u, SM2_BYTE_LEN);
    reverse_byte_array((unsigned char *)(xy+SM2_WORD_LEN), C+1u+SM2_BYTE_LEN, SM2_BYTE_LEN);
#endif

    //get [k]PB
    ret = eccp_pointMul(sm2_curve, k, pubkey_x, pubkey_y, xy, xy+SM2_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //get x2||y2
#ifdef PKE_BIG_ENDIAN
    reverse_word_array((unsigned char *)xy, xy, SM2_WORD_LEN);
    reverse_word_array((unsigned char *)(xy+SM2_WORD_LEN), xy+SM2_WORD_LEN, SM2_WORD_LEN);
#else
    reverse_byte_array((unsigned char *)xy, (unsigned char *)xy, SM2_BYTE_LEN);
    reverse_byte_array((unsigned char *)(xy+SM2_WORD_LEN), (unsigned char *)(xy+SM2_WORD_LEN), SM2_BYTE_LEN);
#endif

    //get C3
    hash_node[0].msg_addr  = (unsigned char *)xy;
    hash_node[0].msg_bytes = SM2_BYTE_LEN;
    hash_node[1].msg_addr  = (unsigned char *)M;
    hash_node[1].msg_bytes = MByteLen;
    hash_node[2].msg_addr  = (unsigned char *)(xy+SM2_WORD_LEN);
    hash_node[2].msg_bytes = SM2_BYTE_LEN;
    ret = hash_node_steps(HASH_SM3, hash_node, 3u, C3);
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //get C2
    //hash_node[0].msg_addr  = (unsigned char *)xy;
    hash_node[0].msg_bytes = SM2_BYTE_LEN<<1;
    hash_node[1].msg_addr  = (unsigned char *)counter;
    hash_node[1].msg_bytes = 4u;
    ret = ansi_x9_63_kdf_node_with_xor_in(HASH_SM3, hash_node, 2u, counter, M, C2, MByteLen, 1u);
    if(HASH_SUCCESS == ret)
    {
        CByteLen[0] = MByteLen+1u+(3u*SM2_BYTE_LEN);

        return SM2_SUCCESS;
    }
    else if(HASH_OUTPUT_ZERO_ALL == ret)
    {
        return SM2_ZERO_ALL;
    }
    else
    {
        return ret;
    }
}


/**
 * @brief       SM2 Encryption
 * @param[in]   M              - plaintext, MByteLen bytes, big-endian
 * @param[in]   MByteLen       - byte length of M
 * @param[in]   rand_k[32]     - input, random big integer k in encrypting, 32 bytes, big-endian,
 *                               if you do not have this integer, please set this parameter to be NULL,
 *                               it will be generated inside.
 * @param[in]   pubKey[65]     - public key, 65 bytes, big-endian
 * @param[in]   order          - either SM2_C1C3C2 or SM2_C1C2C3
 * @param[out]   C              - ciphertext, CByteLen bytes, big-endian
 * @param[out]  CByteLen       - byte length of C, should be MByteLen+97 if success
 * @return      SM2_SUCCESS(success)     other:error
 * @note
  @verbatim
      -# 1.M and C can be the same buffer.
      -# 2.if you do not have rand_k, please set the parameter to be NULL, it will be generated inside.
      -# 3.please make sure pubKey is valid
  @endverbatim
 */
unsigned int sm2_encrypt(unsigned char *M, unsigned int MByteLen, unsigned char rand_k[32], unsigned char pubKey[65],
        sm2_cipher_order_e order, unsigned char *C, unsigned int *CByteLen)
{
    unsigned int k[SM2_WORD_LEN];
    unsigned int pubkey_x[SM2_WORD_LEN],pubkey_y[SM2_WORD_LEN];
    unsigned int ret;

    if(NULL == pubKey)
    {
        return SM2_BUFFER_NULL;
    }
    else if(POINT_UNCOMPRESSED != pubKey[0])
    {
        return SM2_INPUT_INVALID;
    }
    else
    {;}

#ifdef PKE_BIG_ENDIAN
    reverse_word_array(pubKey+1u, pubkey_x, SM2_WORD_LEN);
    reverse_word_array(pubKey+1u+SM2_BYTE_LEN, pubkey_y, SM2_WORD_LEN);
#else
    reverse_byte_array(pubKey+1u, (unsigned char *)pubkey_x, SM2_BYTE_LEN);
    reverse_byte_array(pubKey+1u+SM2_BYTE_LEN, (unsigned char *)pubkey_y, SM2_BYTE_LEN);
#endif
    ret = eccp_pointVerify(sm2_curve, pubkey_x, pubkey_y);
    if(PKE_SUCCESS != ret)
    {
        return SM2_NOT_ON_CURVE;
    }
    else
    {;}

    if(NULL == rand_k)
    {
        do {
            ret = get_rand((unsigned char *)k, SM2_BYTE_LEN);
            if(TRNG_SUCCESS != ret)
            {
                break;
            }
            else
            {;}

            ret = sm2_encrypt_with_k(M, MByteLen, k, pubkey_x, pubkey_y, order, C, CByteLen);
        } while((SM2_ZERO_ALL == ret) || (SM2_INTEGER_TOO_BIG == ret));
    }
    else
    {
        reverse_byte_array(rand_k, (unsigned char *)k, SM2_BYTE_LEN);

        ret = sm2_encrypt_with_k(M, MByteLen, k, pubkey_x, pubkey_y, order, C, CByteLen);
    }

    return ret;
}


/**
 * @brief       SM2 Decryption
 * @param[in]   C             - ciphertext, CByteLen bytes, big-endian
 * @param[in]   CByteLen      - byte length of C, make sure MByteLen>97
 * @param[in]   priKey[32]    - private key, 32 bytes, big-endian
 * @param[out]  M             - plaintext, MByteLen bytes, big-endian
 * @param[out]  MByteLen      - byte length of M, should be CByteLen-97 if success
 * @return      SM2_SUCCESS(success)     other:error
 * @note
  @verbatim
      -# 1.M and C can be the same buffer.
  @endverbatim
 */
unsigned int sm2_decrypt(unsigned char *C, unsigned int CByteLen, unsigned char priKey[32],
        sm2_cipher_order_e order, unsigned char *M, unsigned int *MByteLen)
{
    unsigned char counter[4] = {0,0,0,1};
    unsigned int i, temLen;
    unsigned int dA[SM2_WORD_LEN], xy[SM2_WORD_LEN<<1];
    unsigned char digest[SM2_BYTE_LEN];
    unsigned char C3_buf[SM2_BYTE_LEN];
    unsigned char *C2, *C3;
    unsigned int ret;

    HASH_NODE hash_node[3];

    if((NULL == C) || (NULL == priKey) || (NULL == M) || (NULL == MByteLen))
    {
        return SM2_BUFFER_NULL;
    }
    else if(CByteLen <= (1u+(3u*SM2_BYTE_LEN)))                                        //97 = 1+3*ECCP_BYTELEN
    {
        return SM2_INPUT_INVALID;
    }
    else if(order > SM2_C1C2C3)
    {
        return SM2_INPUT_INVALID;
    }
    else
    {;}

    hash_node[0].msg_addr  = (unsigned char *)xy;
    hash_node[0].msg_bytes = SM2_BYTE_LEN<<1;
    hash_node[1].msg_addr  = counter;
    hash_node[1].msg_bytes = 4U;
    hash_node[2].msg_addr  = (unsigned char *)(xy+SM2_WORD_LEN);
    hash_node[2].msg_bytes = SM2_BYTE_LEN;

    temLen = CByteLen-1u-(3u*SM2_BYTE_LEN);

    C2 = C+1u+(2u*SM2_BYTE_LEN) +((SM2_C1C2C3 == order)?0u:SM2_BYTE_LEN);
    C3 = C+1u+(2u*SM2_BYTE_LEN) +((SM2_C1C2C3 == order)?temLen:0u);

    //not support M and C crossing, but support M = C
    if(M > C)
    {
        if((C + CByteLen) > M)
        {
            return SM2_INPUT_INVALID;
        }
        else
        {;}
    }
    else if(M < C)
    {
        if((M + temLen) > C)
        {
            return SM2_INPUT_INVALID;
        }
        else
        {;}
    }
    else  //M = C
    {;}

    //make sure C1 is on the SM2 curve
#ifdef PKE_BIG_ENDIAN
    reverse_word_array(C+1u, xy, SM2_WORD_LEN);
    reverse_word_array(C+1u+SM2_BYTE_LEN, xy+SM2_WORD_LEN, SM2_WORD_LEN);
#else
    reverse_byte_array(C+1u, (unsigned char *)xy, SM2_BYTE_LEN);
    reverse_byte_array(C+1u+SM2_BYTE_LEN, (unsigned char *)(xy+SM2_WORD_LEN), SM2_BYTE_LEN);
#endif
    ret = eccp_pointVerify(sm2_curve, xy, xy+SM2_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        return SM2_NOT_ON_CURVE;
    }
    else
    {;}

    if(M == C)  //M = C
    {
        //keep C3
        memcpy_(C3_buf, C3, SM2_BYTE_LEN);
        C3 = C3_buf;

        //move C2 to M, and now M = C2
        for(i=0; i<temLen; i++)
        {
            M[i] = C2[i];
        }

        C2 = M;
    }
    else
    {;}

    //make sure priKey in [1, n-2]
#ifdef PKE_BIG_ENDIAN
    reverse_word_array(priKey, dA, SM2_WORD_LEN);
#else
    reverse_byte_array(priKey, (unsigned char *)dA, SM2_BYTE_LEN);
#endif
    ret = uint32_integer_check(dA, sm2p256v1_n_1, SM2_WORD_LEN, SM2_ZERO_ALL, SM2_INTEGER_TOO_BIG,
            SM2_SUCCESS);
    if(SM2_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //[dA]C1
    ret = eccp_pointMul(sm2_curve, dA, xy, xy+SM2_WORD_LEN, xy, xy+SM2_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

#ifdef PKE_BIG_ENDIAN
    reverse_word_array((unsigned char *)xy, xy, SM2_WORD_LEN);
    reverse_word_array((unsigned char *)(xy+SM2_WORD_LEN), xy+SM2_WORD_LEN, SM2_WORD_LEN);
#else
    reverse_byte_array((unsigned char *)xy, (unsigned char *)xy, SM2_BYTE_LEN);
    reverse_byte_array((unsigned char *)(xy+SM2_WORD_LEN), (unsigned char *)(xy+SM2_WORD_LEN), SM2_BYTE_LEN);
#endif

    ret = ansi_x9_63_kdf_node_with_xor_in(HASH_SM3, hash_node, 2u, counter, C2, M, temLen, 1u);
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //hash_node[0].msg_addr  = (unsigned char *)xy;
    hash_node[0].msg_bytes = SM2_BYTE_LEN;
    hash_node[1].msg_addr  = (unsigned char *)M;
    hash_node[1].msg_bytes = temLen;
    //hash_node[2].msg_addr  = (unsigned char *)(xy+SM2_WORD_LEN);
    //hash_node[2].msg_bytes = SM2_BYTE_LEN;
    ret = hash_node_steps(HASH_SM3, hash_node, 3u, digest);
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    if(((unsigned char)0) != memcmp_(C3, digest, SM2_BYTE_LEN))
    {
        return SM2_DECRYPT_VERIFY_FAILED;
    }
    else
    {
        *MByteLen = temLen;
        return SM2_SUCCESS;
    }
}


/**
 * @brief       SM2 Key Exchange
 * @param[in]   role              - SM2_Role_Sponsor - sponsor, SM2_Role_Responsor - responsor
 * @param[in]   dA[32]            - local's permanent private key7
 * @param[in]   PB[65]            - peer's permanent public key
 * @param[in]   rA[32]            - local's temporary private key
 * @param[in]   RA[65]            - local's temporary public key
 * @param[in]   RB[65]            - peer's temporary public key
 * @param[in]   ZA[32]            - local's Z value
 * @param[in]   ZB[32]            - peer's Z value
 * @param[in]   kByteLen          - byte length of output key, should be less than (2^32 - 1)bit
 * @param[out]  KA[kByteLen]      - output key
 * @param[out]  S1[32]            - sponsor's S1, or responsor's S2, this is optional
 * @param[out]  SA[32]            - sponsor's SA, or responsor's SB, this is optional
 * @return      SM2_SUCCESS(success)     other:error
 * @note
  @verbatim
      -# 1.please make sure the inputs are valid
      -# 2.S1 and SA are optional, if you don't need, please set S1 and SA as NULL
      -# 3.in case that S1(S2) and SA(SB) exist, if S1=SB,S2=SA, then exchange success.
  @endverbatim
 */
unsigned int sm2_exchangekey(sm2_exchange_role_e role,
                        unsigned char *dA, unsigned char *PB,
                        unsigned char *rA, unsigned char *RA,
                        unsigned char *RB,
                        unsigned char *ZA, unsigned char *ZB,
                        unsigned int kByteLen,
                        unsigned char *KA, unsigned char *S1, unsigned char *SA)
{
    unsigned char counter[4] = {0,0,0,1};
    unsigned int x1[SM2_WORD_LEN], t1[SM2_WORD_LEN], tmp[SM2_WORD_LEN<<2];
    HASH_NODE hash_node[5];
    unsigned int ret;

    if((NULL == dA) || (NULL == PB) || (NULL == rA) || (NULL == RA) || (NULL == RB))
    {
        return SM2_BUFFER_NULL;
    }
    else if((NULL == ZA) || (NULL == ZB) || (NULL == KA))
    {
        return SM2_BUFFER_NULL;
    }
    else if(role > SM2_Role_Responsor)
    {
        return SM2_EXCHANGE_ROLE_INVALID;
    }
    else if(0u == kByteLen)
    {
        return SM2_INPUT_INVALID;
    }
    else if((POINT_UNCOMPRESSED != PB[0]) || (POINT_UNCOMPRESSED != RA[0]) || (POINT_UNCOMPRESSED != RB[0]))
    {
        return SM2_INPUT_INVALID;
    }
    else
    {;}

#ifdef PKE_BIG_ENDIAN
    reverse_word_array(RA+1u, x1, SM2_WORD_LEN);
    reverse_word_array(RA+1u+SM2_BYTE_LEN, t1, SM2_WORD_LEN);
#else
    reverse_byte_array(RA+1u, (unsigned char *)x1, SM2_BYTE_LEN);
    reverse_byte_array(RA+1u+SM2_BYTE_LEN, (unsigned char *)t1, SM2_BYTE_LEN);
#endif
    if(PKE_SUCCESS != eccp_pointVerify(sm2_curve, x1, t1))
    {
        ret = SM2_NOT_ON_CURVE;
        goto END;
    }
    else
    {;}

    //get x1
    uint32_clear(x1+(SM2_WORD_LEN>>1), SM2_WORD_LEN>>1);
    x1[(SM2_WORD_LEN>>1)-1u] |= 0x80000000u;

    //make sure rA in [1, n-2]
#ifdef PKE_BIG_ENDIAN
    reverse_word_array(rA, t1, SM2_WORD_LEN);
#else
    reverse_byte_array(rA, (unsigned char *)t1, SM2_BYTE_LEN);
#endif
    ret = uint32_integer_check(t1, sm2p256v1_n_1, SM2_WORD_LEN, SM2_ZERO_ALL, SM2_INTEGER_TOO_BIG,
            SM2_SUCCESS);
    if(SM2_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

#if (defined(PKE_LP) || defined(PKE_SECURE))
    ret = pke_load_modulus_and_pre_monts(sm2p256v1_n, sm2p256v1_n_h, sm2p256v1_n_n0, SM2_BIT_LEN);
#else
    ret = pke_load_modulus_and_pre_monts((unsigned int *)sm2p256v1_n, (unsigned int *)sm2p256v1_n_h, SM2_BIT_LEN);
#endif
    if(PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

    //t1 = x1*rA mod n
#if (defined(PKE_LP) || defined(PKE_SECURE))
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
#endif
    ret = pke_modmul_internal(x1, t1, t1, SM2_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

    //make sure dA in [1, n-2]
#ifdef PKE_BIG_ENDIAN
    reverse_word_array(dA, x1, SM2_WORD_LEN);
#else
    reverse_byte_array(dA, (unsigned char *)x1, SM2_BYTE_LEN);
#endif
    ret = uint32_integer_check(x1, sm2p256v1_n_1, SM2_WORD_LEN, SM2_ZERO_ALL, SM2_INTEGER_TOO_BIG,
            SM2_SUCCESS);
    if(SM2_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

    //t1 = (dA + x1*rA) mod n, and it must not be 0
    ret = pke_modadd(sm2p256v1_n, t1, x1, t1, SM2_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        goto END;
    }
    else if(0u != uint32_BigNum_Check_Zero(t1, SM2_WORD_LEN))
    {
        ret = SM2_ZERO_ALL;
        goto END;
    }
    else
    {;}

    //make sure RB on the SM2 curve
#ifdef PKE_BIG_ENDIAN
    reverse_word_array(RB+1u, tmp, SM2_WORD_LEN);
    reverse_word_array(RB+1u+SM2_BYTE_LEN, tmp+SM2_WORD_LEN, SM2_WORD_LEN);
#else
    reverse_byte_array(RB+1u, (unsigned char *)tmp, SM2_BYTE_LEN);
    reverse_byte_array(RB+1u+SM2_BYTE_LEN, (unsigned char *)(tmp+SM2_WORD_LEN), SM2_BYTE_LEN);
#endif
    ret = eccp_pointVerify(sm2_curve, tmp, tmp+SM2_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        return SM2_NOT_ON_CURVE;
    }
    else
    {;}

    uint32_copy(x1, tmp, SM2_WORD_LEN>>1);
    uint32_clear(x1+(SM2_WORD_LEN>>1), SM2_WORD_LEN>>1);
    x1[(SM2_WORD_LEN>>1)-1u] |= 0x80000000u;

#ifdef SM2_HIGH_SPEED
    ret = pke_set_modulus_and_pre_monts((unsigned int *)sm2p256v1_n, (unsigned int *)sm2p256v1_n_h, SM2_BIT_LEN);
    if(PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

    //x1 = tA*x2 mod n
    ret = pke_modmul_internal(t1, x1, x1, SM2_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

    //get PB point and verify
#ifdef PKE_BIG_ENDIAN
    reverse_word_array(PB+1u, tmp+(2u*SM2_WORD_LEN), SM2_WORD_LEN);
    reverse_word_array(PB+1u+SM2_BYTE_LEN, tmp+(3u*SM2_WORD_LEN), SM2_WORD_LEN);
#else
    reverse_byte_array(PB+1u, (unsigned char *)(tmp+(2u*SM2_WORD_LEN)), SM2_BYTE_LEN);
    reverse_byte_array(PB+1u+SM2_BYTE_LEN, (unsigned char *)(tmp+(3u*SM2_WORD_LEN)), SM2_BYTE_LEN);
#endif
    ret = eccp_pointVerify((eccp_curve_t *)sm2_curve, tmp+(2u*SM2_WORD_LEN), tmp+(3u*SM2_WORD_LEN));
    if(PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

    //[tA]PB +[tA*x2 mod n]RB
    ret = eccp_pointMul_Shamir_safe((eccp_curve_t *)sm2_curve,
                                    t1, tmp+(2u*SM2_WORD_LEN), tmp+(3u*SM2_WORD_LEN),
                                    x1, tmp, tmp+SM2_WORD_LEN,
                                    tmp, tmp+SM2_WORD_LEN);
#else
    ret = eccp_pointMul(sm2_curve, x1, tmp, tmp+SM2_WORD_LEN, tmp, tmp+SM2_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

    //get PB point(caution: do not delete this)
#ifdef PKE_BIG_ENDIAN
    reverse_word_array(PB+1u, tmp+2u*SM2_WORD_LEN, SM2_WORD_LEN);
    reverse_word_array(PB+1u+SM2_BYTE_LEN, tmp+3u*SM2_WORD_LEN, SM2_WORD_LEN);
#else
    reverse_byte_array(PB+1u, (unsigned char *)(tmp+2u*SM2_WORD_LEN), SM2_BYTE_LEN);
    reverse_byte_array(PB+1u+SM2_BYTE_LEN, (unsigned char *)(tmp+3u*SM2_WORD_LEN), SM2_BYTE_LEN);
#endif
    ret = eccp_pointVerify(sm2_curve, tmp+2u*SM2_WORD_LEN, tmp+3u*SM2_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

    ret = eccp_pointAdd_safe(sm2_curve, tmp, tmp+SM2_WORD_LEN, tmp+2u*SM2_WORD_LEN, tmp+3u*SM2_WORD_LEN,
                        tmp, tmp+SM2_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

    ret = eccp_pointMul(sm2_curve, t1, tmp, tmp+SM2_WORD_LEN, tmp, tmp+SM2_WORD_LEN);
#endif
    if(PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

    //xU||yU
#ifdef PKE_BIG_ENDIAN
    reverse_word_array((unsigned char *)tmp, tmp, SM2_WORD_LEN);
    reverse_word_array((unsigned char *)(tmp+SM2_WORD_LEN), tmp+SM2_WORD_LEN, SM2_WORD_LEN);
#else
    reverse_byte_array((unsigned char *)tmp, (unsigned char *)tmp, SM2_BYTE_LEN);
    reverse_byte_array((unsigned char *)(tmp+SM2_WORD_LEN), (unsigned char *)(tmp+SM2_WORD_LEN), SM2_BYTE_LEN);
#endif

    hash_node[0].msg_addr  = (unsigned char *)tmp;
    hash_node[0].msg_bytes = SM2_BYTE_LEN<<1;
    if(SM2_Role_Sponsor == role)
    {
        hash_node[1].msg_addr  = (unsigned char *)ZA;
        hash_node[2].msg_addr  = (unsigned char *)ZB;
    }
    else
    {
        hash_node[1].msg_addr  = (unsigned char *)ZB;
        hash_node[2].msg_addr  = (unsigned char *)ZA;
    }
    hash_node[1].msg_bytes = SM2_BYTE_LEN;
    hash_node[2].msg_bytes = SM2_BYTE_LEN;
    hash_node[3].msg_addr  = (unsigned char *)counter;
    hash_node[3].msg_bytes = 4u;

    //KA
    ret = ansi_x9_63_kdf_node(HASH_SM3, hash_node, 4u, counter, KA, kByteLen, NULL, 0u);
    if(HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    //check value is optional
    if((NULL != S1) && (NULL != SA))
    {
        //t1 = hash(xu||ZA||ZB||x1||y1||x2||y2)
        hash_node[0].msg_addr  = (unsigned char *)tmp;
        hash_node[0].msg_bytes = SM2_BYTE_LEN;

        if(SM2_Role_Sponsor == role)
        {
            hash_node[1].msg_addr  = ZA;
            hash_node[2].msg_addr  = ZB;
            hash_node[3].msg_addr  = RA+1u;
            hash_node[4].msg_addr  = RB+1u;
        }
        else
        {
            hash_node[1].msg_addr  = ZB;
            hash_node[2].msg_addr  = ZA;
            hash_node[3].msg_addr  = RB+1u;
            hash_node[4].msg_addr  = RA+1u;
        }

        hash_node[1].msg_bytes = SM2_BYTE_LEN;
        hash_node[2].msg_bytes = SM2_BYTE_LEN;
        hash_node[3].msg_bytes = SM2_BYTE_LEN<<1;
        hash_node[4].msg_bytes = SM2_BYTE_LEN<<1;

        ret = hash_node_steps(HASH_SM3, hash_node, 5u, (unsigned char *)t1);
        if(HASH_SUCCESS != ret)
        {
            goto END;
        }
        else
        {;}

        //get SA = hash(0x03||yu||t1)
        ((unsigned char *)(tmp))[SM2_BYTE_LEN-1u] = (unsigned char)0x03;
        hash_node[0].msg_addr  = &(((unsigned char *)(tmp))[SM2_BYTE_LEN-1u]);
        hash_node[0].msg_bytes = SM2_BYTE_LEN+1u;
        hash_node[1].msg_addr  = (unsigned char *)t1;
        hash_node[1].msg_bytes = SM2_BYTE_LEN;
        if(SM2_Role_Sponsor == role)
        {
            ret = hash_node_steps(HASH_SM3, hash_node, 2u, (unsigned char *)SA);
        }
        else
        {
            ret = hash_node_steps(HASH_SM3, hash_node, 2u, (unsigned char *)S1);
        }

        if(HASH_SUCCESS != ret)
        {
            goto END;
        }
        else
        {;}

        //get S1 = hash(0x02||yu||t1)
        ((unsigned char *)(tmp))[SM2_BYTE_LEN-1u] = (unsigned char)0x02;
        if(SM2_Role_Sponsor == role)
        {
            ret = hash_node_steps(HASH_SM3, hash_node, 2u, (unsigned char *)S1);
        }
        else
        {
            ret = hash_node_steps(HASH_SM3, hash_node, 2u, (unsigned char *)SA);
        }

        if(HASH_SUCCESS != ret)
        {
            goto END;
        }
        else
        {;}
    }
    else
    {
        ;
    }

    ret = SM2_SUCCESS;

END:

    return ret;
}

#endif

