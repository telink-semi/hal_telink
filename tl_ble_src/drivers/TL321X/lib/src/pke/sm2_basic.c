/********************************************************************************************************
 * @file    sm2_basic.c
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
    #include "lib/include/trng/trng.h"
    #include "lib/include/crypto_common/utility.h"
    #ifdef PKE_SEC
        #include "lib/include/crypto_common/utility_sec.h"
    #endif


    #define SM2_DEFAULT_ID_BYTE_LEN (16u)
unsigned char g_sm2_default_id[16] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38};


//SM2 algorithm parameters
unsigned int sm2p256v1_p[8]   = {0xFFFFFFFFu, 0xFFFFFFFFu, 0x00000000u, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFEu};
unsigned int sm2p256v1_p_h[8] = {0x00000003u, 0x00000002u, 0xFFFFFFFFu, 0x00000002u, 0x00000001u, 0x00000001u, 0x00000002u, 0x00000004u};
    #if (defined(PKE_LP) || defined(PKE_SECURE))
unsigned int sm2p256v1_p_n0[1] = {
    1u,
};
    #endif
unsigned int sm2p256v1_a[8]   = {0xFFFFFFFCu, 0xFFFFFFFFu, 0x00000000u, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFEu};
unsigned int sm2p256v1_b[8]   = {0x4D940E93u, 0xDDBCBD41u, 0x15AB8F92u, 0xF39789F5u, 0xCF6509A7u, 0x4D5A9E4Bu, 0x9D9F5E34u, 0x28E9FA9Eu};
unsigned int sm2p256v1_Gx[8]  = {0x334C74C7u, 0x715A4589u, 0xF2660BE1u, 0x8FE30BBFu, 0x6A39C994u, 0x5F990446u, 0x1F198119u, 0x32C4AE2Cu};
unsigned int sm2p256v1_Gy[8]  = {0x2139F0A0u, 0x02DF32E5u, 0xC62A4740u, 0xD0A9877Cu, 0x6B692153u, 0x59BDCEE3u, 0xF4F6779Cu, 0xBC3736A2u};
unsigned int sm2p256v1_n[8]   = {0x39D54123u, 0x53BBF409u, 0x21C6052Bu, 0x7203DF6Bu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFEu};
unsigned int sm2p256v1_n_h[8] = {0x7C114F20u, 0x901192AFu, 0xDE6FA2FAu, 0x3464504Au, 0x3AFFE0D4u, 0x620FC84Cu, 0xA22B3D3Bu, 0x1EB5E412u};
    #if (defined(PKE_LP) || defined(PKE_SECURE))
unsigned int sm2p256v1_n_n0[1] = {
    0x72350975u,
};
    #endif

//SM2 para (n-1), for private key checking
unsigned int sm2p256v1_n_1[8] = {0x72350975u, 0x327F9E88u, 0xFC8319A5u, 0xDF1E8D34u, 0xB08941D4u, 0x2B0068D3u, 0x82E4C7BCu, 0x6F39132Fu};

    //[2^128]G, for [k]G of high speed
    #if !(defined(PKE_LP) || defined(PKE_SECURE))
static unsigned int sm2p256v1_2_128_G_x[8] = {0xD13A42EDu, 0xEAE3D9A9u, 0x484E1B38u, 0x2B2308F6u, 0x88C21F3Au, 0x3DB7B248u, 0x74D55DA9u, 0xB692E5B5u};
static unsigned int sm2p256v1_2_128_G_y[8] = {0xE295E5ABu, 0xD186469Du, 0x73438E6Du, 0xDB61AC17u, 0x544926F9u, 0x5A924F85u, 0x0F3FB613u, 0xA175051Bu};
    #endif

eccp_curve_t sm2_curve[1] = {
    {
     SM2_BIT_LEN,
     SM2_BIT_LEN,
     sm2p256v1_p,
     sm2p256v1_p_h,
    #if (defined(PKE_LP) || defined(PKE_SECURE))
     sm2p256v1_p_n0,
    #endif
     sm2p256v1_a,
     sm2p256v1_b,
     sm2p256v1_Gx,
     sm2p256v1_Gy,
     sm2p256v1_n,
     sm2p256v1_n_h,
    #if (defined(PKE_LP) || defined(PKE_SECURE))
     sm2p256v1_n_n0,
    #else
        sm2p256v1_2_128_G_x,
        sm2p256v1_2_128_G_y,
    #endif
     }
};


/**
 * @brief       get SM2 Z value = SM3(bitLenofID||ID||a||b||Gx||Gy||Px||Py).
 * @param[in]   ID           - User ID
 * @param[in]   byteLenofID  - byte length of ID, must be less than 2^13
 * @param[in]   pubKey       - public key(0x04 + x + y), 65 bytes, big-endian
 * @param[out]  Z            - Z value, SM3 digest, 32 bytes.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.bit length of ID must be less than 2^16, thus byte length must be less than 2^13
      -# 2.if ID is NULL, then replace it with sm2 default ID
      -# 3.please make sure the pubKey is valid
  @endverbatim
 */
unsigned int sm2_getZ(unsigned char *ID, unsigned int byteLenofID, unsigned char pubKey[65], unsigned char Z[32])
{
    unsigned int  tmp[SM2_WORD_LEN];
    unsigned int  tmp2[SM2_WORD_LEN];
    HASH_CTX      ctx[1];
    unsigned int  ret;
    unsigned char tmp_u8;

    if ((NULL == pubKey) || (NULL == Z)) {
        return SM2_BUFFER_NULL;
    } else if (POINT_UNCOMPRESSED != pubKey[0]) {
        return SM2_INPUT_INVALID;
    } else if (byteLenofID > SM2_MAX_ID_BYTE_LEN) {
        return SM2_INPUT_INVALID;
    } else if ((NULL == ID) || (0u == byteLenofID)) {
        ID          = g_sm2_default_id;
        byteLenofID = SM2_DEFAULT_ID_BYTE_LEN;
    } else {
        ;
    }

    #ifdef PKE_BIG_ENDIAN
    reverse_word_array(pubKey + 1u, tmp, SM2_WORD_LEN);
    reverse_word_array(pubKey + 1u + SM2_BYTE_LEN, tmp2, SM2_WORD_LEN);
    #else
    reverse_byte_array(pubKey + 1u, (unsigned char *)(tmp), SM2_BYTE_LEN);
    reverse_byte_array(pubKey + 1u + SM2_BYTE_LEN, (unsigned char *)(tmp2), SM2_BYTE_LEN);
    #endif

    ret = eccp_pointVerify(sm2_curve, tmp, tmp2);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = hash_init(ctx, HASH_SM3);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    byteLenofID <<= 3;
    tmp_u8 = (unsigned char)((byteLenofID >> 8u) & 0xFFu);
    ret    = hash_update(ctx, (unsigned char *)&tmp_u8, 1);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    tmp_u8 = (unsigned char)(byteLenofID & 0xFFu);
    ret    = hash_update(ctx, (unsigned char *)&tmp_u8, 1);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    byteLenofID >>= 3;
    ret = hash_update(ctx, ID, byteLenofID);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    #ifdef PKE_BIG_ENDIAN
    reverse_word_array((unsigned char *)sm2p256v1_a, tmp, SM2_WORD_LEN);
    #else
    reverse_byte_array((unsigned char *)sm2p256v1_a, (unsigned char *)tmp, SM2_BYTE_LEN);
    #endif

    ret = hash_update(ctx, (unsigned char *)tmp, SM2_BYTE_LEN);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    #ifdef PKE_BIG_ENDIAN
    reverse_word_array((unsigned char *)sm2p256v1_b, tmp, SM2_WORD_LEN);
    #else
    reverse_byte_array((unsigned char *)sm2p256v1_b, (unsigned char *)tmp, SM2_BYTE_LEN);
    #endif

    ret = hash_update(ctx, (unsigned char *)tmp, SM2_BYTE_LEN);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    #ifdef PKE_BIG_ENDIAN
    reverse_word_array((unsigned char *)sm2p256v1_Gx, tmp, SM2_WORD_LEN);
    #else
    reverse_byte_array((unsigned char *)sm2p256v1_Gx, (unsigned char *)tmp, SM2_BYTE_LEN);
    #endif

    ret = hash_update(ctx, (unsigned char *)tmp, SM2_BYTE_LEN);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    #ifdef PKE_BIG_ENDIAN
    reverse_word_array((unsigned char *)sm2p256v1_Gy, tmp, SM2_WORD_LEN);
    #else
    reverse_byte_array((unsigned char *)sm2p256v1_Gy, (unsigned char *)tmp, SM2_BYTE_LEN);
    #endif

    ret = hash_update(ctx, (unsigned char *)tmp, SM2_BYTE_LEN);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = hash_update(ctx, pubKey + 1u, SM2_BYTE_LEN << 1);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = hash_final(ctx, Z);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = SM2_SUCCESS;

END:

    return ret;
}

/**
 * @brief       get SM2 E value = SM3(Z||M) (one-off style)
 * @param[in]   M             - Message
 * @param[in]   byteLen       - byte length of M
 * @param[in]   Z             - Z value, 32 bytes
 * @param[out]  E             - E value, 32 bytes
 * @return      0:success     other:error
 */
unsigned int sm2_getE(unsigned char *M, unsigned int byteLen, unsigned char Z[32], unsigned char E[32])
{
    HASH_CTX     ctx[1];
    unsigned int ret;

    if ((NULL == M) || (NULL == Z) || (NULL == E)) {
        return SM2_BUFFER_NULL;
    } else if (0u == byteLen) {
        return SM2_INPUT_INVALID;
    } else {
        ;
    }

    ret = hash_init(ctx, HASH_SM3);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = hash_update(ctx, Z, 32u);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = hash_update(ctx, M, byteLen);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = hash_final(ctx, E);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = SM2_SUCCESS;

END:

    return ret;
}


    #ifdef SM2_GETE_BY_STEPS
/**
 * @brief       step 1 of getting SM2 E value(stepwise style), init
 * @param[in]   ctx     - input, HASH_CTX context pointer
 * @param[in]   Z       - input, Z value, 32 bytes
 * @return      SM2_SUCCESS(success), other(error)
 */
unsigned int sm2_getE_init(HASH_CTX *ctx, unsigned char Z[32])
{
    unsigned int ret;

    ret = hash_init(ctx, HASH_SM3);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = hash_update(ctx, Z, 32u);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = SM2_SUCCESS;

END:

    return ret;
}

/**
 * @brief       step 2 of getting SM2 E value(stepwise style), update message
 * @param[in]   ctx           - input, HASH_CTX context pointer
 * @param[in]   msg           - input, message
 * @param[in]   msg_bytes     - input, byte length of the input message
 * @return      SM2_SUCCESS(success), other(error)
 *  @note
  @verbatim
      -# 1. please make sure the three parameters are valid, and ctx is initialized.
  @endverbatim
 */
unsigned int sm2_getE_update(HASH_CTX *ctx, unsigned char *msg, unsigned int msg_bytes)
{
    unsigned int ret;

    ret = hash_update(ctx, msg, msg_bytes);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = SM2_SUCCESS;

END:

    return ret;
}

/**
 * @brief       step 3 of getting SM2 E value(stepwise style), message update done, get the digest(SM2 E value)
 * @param[in]   ctx           - input, HASH_CTX context pointer
 * @param[in]   E             - input, message
 * @return      SM2_SUCCESS(success), other(error)
 *  @note
  @verbatim
      -# 1. please make sure the ctx is valid and initialized.
      -# 2. please make sure the digest buffer E is sufficient.
  @endverbatim
 */
unsigned int sm2_getE_final(HASH_CTX *ctx, unsigned char E[32])
{
    unsigned int ret;

    ret = hash_final(ctx, E);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = SM2_SUCCESS;

END:

    return ret;
}
    #endif


/**
 * @brief       Generate SM2 public key from private key
 * @param[in]   priKey           - private key, 32 bytes, big-endian
 * @param[out]  pubKey           - public key(0x04 + x + y), 65 bytes, big-endian
 * @return      0:success     other:error
 */
unsigned int sm2_get_pubkey_from_prikey(unsigned char priKey[32], unsigned char pubKey[65])
{
    unsigned int ret;

    if ((NULL == priKey) || (NULL == pubKey)) {
        return SM2_BUFFER_NULL;
    } else {
        ;
    }

    ret = eccp_get_pubkey_from_prikey(sm2_curve, priKey, pubKey + 1);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        pubKey[0] = POINT_UNCOMPRESSED;

        return SM2_SUCCESS;
    }
}

/**
 * @brief       Generate SM2 random Key pair
 * @param[in]   priKey           - private key, 32 bytes, big-endian
 * @param[out]  pubKey           - public key(0x04 + x + y), 65 bytes, big-endian
 * @return      0:success     other:error
 */
unsigned int sm2_getkey(unsigned char priKey[32], unsigned char pubKey[65])
{
    unsigned int ret;

    #if 1
    if ((NULL == priKey) || (NULL == pubKey)) {
        return SM2_BUFFER_NULL;
    } else {
        ;
    }

    ret = eccp_getkey(sm2_curve, priKey, pubKey + 1);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        pubKey[0] = POINT_UNCOMPRESSED;

        return SM2_SUCCESS;
    }
    #else

    unsigned int k[SM2_WORD_LEN], tmp[SM2_WORD_LEN << 1];

    if (NULL == priKey || NULL == pubKey) {
        return SM2_BUFFER_NULL;
    } else {
        ;
    }

SM2_GETKEY_LOOP:

    ret = get_rand((unsigned char *)k, SM2_BYTE_LEN);
    if (TRNG_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //make sure priKey in [1, n-2]
    if (uint32_BigNum_Check_Zero(k, SM2_WORD_LEN)) {
        goto SM2_GETKEY_LOOP;
    } else if (uint32_BigNumCmp(k, SM2_WORD_LEN, (unsigned int *)sm2p256v1_n_1, SM2_WORD_LEN) >= 0) {
        goto SM2_GETKEY_LOOP;
    } else {
        ;
    }

        #ifdef SM2_HIGH_SPEED
    ret = eccp_pointMul_base((eccp_curve_t *)sm2_curve, k, tmp, tmp + SM2_WORD_LEN);
        #else
    ret = eccp_pointMul((eccp_curve_t *)sm2_curve, k, sm2_curve->eccp_Gx, sm2_curve->eccp_Gy, tmp, tmp + SM2_WORD_LEN);
        #endif
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    pubKey[0] = POINT_UNCOMPRESSED;
        #ifdef PKE_BIG_ENDIAN
    reverse_word_array((unsigned char *)k, k, SM2_WORD_LEN);
    memcpy_(priKey, k, SM2_BYTE_LEN);
    reverse_word_array((unsigned char *)tmp, k, SM2_WORD_LEN);
    memcpy_(pubKey + 1, k, SM2_BYTE_LEN);
    reverse_word_array((unsigned char *)(tmp + SM2_WORD_LEN), k, SM2_WORD_LEN);
    memcpy_(pubKey + 1 + SM2_BYTE_LEN, k, SM2_BYTE_LEN);
        #else
    reverse_byte_array((unsigned char *)k, priKey, SM2_BYTE_LEN);
    reverse_byte_array((unsigned char *)tmp, pubKey + 1u, SM2_BYTE_LEN);
    reverse_byte_array((unsigned char *)(tmp + SM2_WORD_LEN), pubKey + 1u + SM2_BYTE_LEN, SM2_BYTE_LEN);
        #endif
    return SM2_SUCCESS;
    #endif
}


#endif
