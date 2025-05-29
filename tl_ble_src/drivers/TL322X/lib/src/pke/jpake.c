/********************************************************************************************************
 * @file    jpake.c
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
#include <stdio.h>
#include "lib/include/pke/ed25519.h"
#include "lib/include/pke/jpake.h"
#include "lib/include/crypto_common/utility.h"
#include "lib/include/trng/trng.h"


/**
 * @brief       J-PAKE hash item (byte) length
 * @param[in]   ctx                  - HASH_CTX struct pointer
 * @param[in]   msg_bytes            - message or item byte length
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.ctx must be initialized
  @endverbatim
 */
extern unsigned int jpake_hash_length(HASH_CTX *ctx, unsigned int msg_bytes);

/**
 * @brief       J-PAKE hash string
 * @param[in]   ctx                  - HASH_CTX struct pointer
 * @param[in]   s                    - byte string
 * @param[in]   byteLen              - byte length of s
 * @return      0:success     other:error
 */
extern unsigned int jpake_hash_string(HASH_CTX *ctx, unsigned char *s, unsigned int byteLen);

/**
 * @brief       J-PAKE get rand xa less than q
 * @param[in]   q                        - big number q
 * @param[out]  xa                       - random big number less than q
 * @param[in]   wordLen                  - word length of q and xa
 * @param[in]   remainder_bits           - real bit length of q mod 32.
 * @param[in]   could_be_zero            - could xa be zero, 0(xa can not be zero), other(xa can be zero).
 * @return      0:success     other:error
 */
extern unsigned int jpake_get_rand_xa_less_than_q(unsigned int *q, unsigned int *xa, unsigned int wordLen, unsigned int remainder_bits, unsigned int could_be_zero);

/**
 * @brief       J-PAKE hash big number
 * @param[in]   ctx             - HASH_CTX struct pointer
 * @param[in]   a               - big number to be hashed
 * @param[in]   t               - temporary buffer with same length as a
 * @param[in]   byteLen         - byte length of a or t
 * @return      0:success     other:error
 */
unsigned int jpake_hash_big_number(HASH_CTX *ctx, unsigned int *a, unsigned int *t, unsigned int byteLen)
{
    unsigned int ret;

    ret = jpake_hash_length(ctx, byteLen);
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    reverse_byte_array((unsigned char *)a, (unsigned char *)t, byteLen);

    return hash_update(ctx, (unsigned char *)t, byteLen);
}

/**
 * @brief       J-PAKE get c = h mod q for ZKP(a), here h=hash(g,V,A,ID,other_info)
 * @param[in]   jpake_para       - JPAKE_PARA struct pointer
 * @param[in]   g                - parameter g of ZKP(a), a is the private key of ZKP(a) owner.
 * @param[in]   V                - g^v mod p, p is modulus, v is the random secret of ZKP(a)
 * @param[out]  A                - g^a mod p, public key of ZKP(a) owner.
 * @param[in]   ID               - ID of ZKP(a) owner.
 * @param[in]   ID_bytes         - ID byte length.
 * @param[in]   other_info       - other information of ZKP(a) owner.
 * @param[in]   other_info_bytes - other_info byte length.
 * @param[in]   c                - c = h mod q.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.c is a big number with the same length as the parameter q of ZKP(a).
      -# 2.g is not necessarily the parameter g of J-PAKE.
  @endverbatim
 */
unsigned int jpake_get_zkp_c(JPAKE_PARA *jpake_para, unsigned int *g, unsigned int *V, unsigned int *A, unsigned char *ID, unsigned int ID_bytes, unsigned char *other_info, unsigned int other_info_bytes, unsigned int *c)
{
    unsigned int pByteLen = GET_BYTE_LEN(jpake_para->pBitLen);
    unsigned int qWordLen = GET_WORD_LEN(jpake_para->qBitLen);
    unsigned int digest_bytes;
    unsigned int t[JPAKE_MAX_WORD_LEN];
    HASH_CTX     hash_ctx[1];
    unsigned int ret;

    digest_bytes = hash_get_digest_word_len(jpake_para->hash_alg) << 2;

    ret = hash_init(hash_ctx, jpake_para->hash_alg);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    //hash input g
    jpake_hash_big_number(hash_ctx, g, t, pByteLen);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    //hash input V
    jpake_hash_big_number(hash_ctx, V, t, pByteLen);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    //hash input A
    jpake_hash_big_number(hash_ctx, A, t, pByteLen);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    //hash input ID
    ret = jpake_hash_string(hash_ctx, ID, ID_bytes);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    //hash input other info
    if (other_info) {
        ret = jpake_hash_string(hash_ctx, other_info, other_info_bytes);
        if (HASH_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }
    }

    t[((digest_bytes + 3) / 4) - 1] = 0;
    ret                             = hash_final(hash_ctx, (unsigned char *)t);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    if ((jpake_para->qBitLen) > (digest_bytes << 3)) {
        reverse_byte_array((unsigned char *)t, (unsigned char *)c, digest_bytes);
        memset_(((unsigned char *)c) + digest_bytes, 0, (qWordLen << 2) - digest_bytes);
    } else {
        reverse_byte_array((unsigned char *)t, (unsigned char *)t, digest_bytes);

        ret = pke_mod(t, get_valid_words(t, digest_bytes / 4), jpake_para->q, jpake_para->q_h, jpake_para->q_n0, qWordLen, c);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }
    }

    ret = JPAKE_SUCCESS;

END:

    return ret;
}

/**
 * @brief       J-PAKE get xa, xb, g^xa, g^xb
 * @param[in]   jpake_para   - JPAKE_PARA struct pointer
 * @param[in]   xa           - random big number less than q, private key
 * @param[in]   xb           - random big number less than q, private key
 * @param[out]  gxa          - random big number g^xa, public key
 * @param[in]   gxb          - random big number g^xb, public key
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.xa is in [0,q-1], xb is in [1,q-1].
  @endverbatim
 */
unsigned int jpake_get_xa_xb_gxa_gxb(JPAKE_PARA *jpake_para, unsigned int *xa, unsigned int *xb, unsigned int *gxa, unsigned int *gxb)
{
    unsigned int remainder_bits = jpake_para->qBitLen & 31;
    unsigned int pWordLen       = GET_WORD_LEN(jpake_para->pBitLen);
    unsigned int qWordLen       = GET_WORD_LEN(jpake_para->qBitLen);
    unsigned int ret;

    ret = jpake_get_rand_xa_less_than_q(jpake_para->q, xa, qWordLen, remainder_bits, 1);
    if (ret) {
        goto END;
    } else {
        ;
    }

    ret = jpake_get_rand_xa_less_than_q(jpake_para->q, xb, qWordLen, remainder_bits, 0);
    if (ret) {
        goto END;
    } else {
        ;
    }

    //set mod p
    ret = pke_set_modulus_and_pre_monts(jpake_para->p, jpake_para->p_h, jpake_para->p_n0, jpake_para->pBitLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = pke_modexp((unsigned int *)jpake_para->p, (unsigned int *)xa, (unsigned int *)jpake_para->g, gxa, pWordLen, qWordLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = pke_modexp((unsigned int *)jpake_para->p, (unsigned int *)xb, (unsigned int *)jpake_para->g, gxb, pWordLen, qWordLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = JPAKE_SUCCESS;

END:

    return ret;
}

/**
 * @brief       J-PAKE get ZKP(a) internal API
 * @param[in]   jpake_para       - JPAKE_PARA struct pointer
 * @param[in]   zkp_owner_info   - ID and other info of ZKP(a) owner
 * @param[in]   c                - c = hash(g, gv, ga, ID, info) mod q
 * @param[out]  a                - private key of ZKP(a) owner
 * @param[in]   v                - random secret value for ZKP(a), must be in [0, q-1]
 * @param[in]   g                - parameter g of ZKP(a)
 * @param[in]   zkp              - ZKP(a)
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.one of zkp_owner_info and c must be NULL, and another must be not NULL
      -# 2.v could be NULL, in this case, v will be generated inside
      -# 3.g is not necessarily the parameter g of J-PAKE.
  @endverbatim
 */
unsigned int jpake_generate_zkp_internal(JPAKE_PARA *jpake_para, JPAKE_USER_INFO *zkp_owner_info, unsigned int *c, unsigned int *a, unsigned int *v, unsigned int *g, JPAKE_ZKP *zkp)
{
    unsigned int v_buf[JPAKE_MAX_WORD_LEN];
    unsigned int c_buf[JPAKE_MAX_WORD_LEN];
    unsigned int remainder_bits = jpake_para->qBitLen & 31;
    unsigned int pWordLen       = GET_WORD_LEN(jpake_para->pBitLen);
    unsigned int qWordLen       = GET_WORD_LEN(jpake_para->qBitLen);
    unsigned int ret;

    if (NULL == v) {
        //get random v in [0, q-1]
        v   = v_buf;
        ret = jpake_get_rand_xa_less_than_q(jpake_para->q, v, qWordLen, remainder_bits, 1);
        if (ret) {
            goto END;
        } else {
            ;
        }
    } else {
        ;
    }

    //set mod p
    ret = pke_set_modulus_and_pre_monts(jpake_para->p, jpake_para->p_h, jpake_para->p_n0, jpake_para->pBitLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    //gv = g^v mod p
    ret = pke_modexp((unsigned int *)jpake_para->p, (unsigned int *)v, (unsigned int *)g, zkp->gv, pWordLen, qWordLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    if ((NULL != zkp_owner_info) && (NULL == c)) {
        //c = hash(g, gv, ga, ID, info) mod q
        c   = c_buf;
        ret = jpake_get_zkp_c(jpake_para, g, zkp->gv, zkp->ga, zkp_owner_info->ID, zkp_owner_info->ID_bytes, zkp_owner_info->other_info, zkp_owner_info->other_info_bytes, c);
        if (JPAKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }
    } else if ((NULL == zkp_owner_info) && (NULL != c)) {
        ;
    } else {
        ret = JPAKE_INVALID_INPUT;
        goto END;
    }

    //make sure c < q
    if (uint32_BigNumCmp(c, qWordLen, jpake_para->q, qWordLen) >= 0) {
        return JPAKE_INTEGER_TOO_BIG;
    } else {
        ;
    }

    //set mod q
    ret = pke_set_modulus_and_pre_monts(jpake_para->q, jpake_para->q_h, jpake_para->q_n0, jpake_para->qBitLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    //r = a*c mod q
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
    ret = pke_modmul_internal((unsigned int *)a, (unsigned int *)c, zkp->r, qWordLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    //r = v - a*c mod q
    ret = pke_modsub((unsigned int *)jpake_para->q, (unsigned int *)v, (unsigned int *)zkp->r, zkp->r, qWordLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = JPAKE_SUCCESS;

END:

    return ret;
}

/**
 * @brief       J-PAKE get ZKP(a)
 * @param[in]   jpake_para       - JPAKE_PARA struct pointer
 * @param[in]   zkp_owner_info   - ID and other info of ZKP(a) owner
 * @param[in]   a                - private key of ZKP(a) owner
 * @param[in]   g                - parameter g of ZKP(a)
 * @param[out]  zkp              - ZKP(a)
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.g is not necessarily the parameter g of J-PAKE.
  @endverbatim
 */
unsigned int jpake_generate_zkp(JPAKE_PARA *jpake_para, JPAKE_USER_INFO *zkp_owner_info, unsigned int *a, unsigned int *g, JPAKE_ZKP *zkp)
{
    return jpake_generate_zkp_internal(jpake_para, zkp_owner_info, NULL, a, NULL, g, zkp);
}

/**
 * @brief       J-PAKE verify ZKP(a) internal API, here a is the private key of ZKP owner, not disclosed
 * @param[in]   jpake_para       - JPAKE_PARA struct pointer
 * @param[in]   zkp_owner_info   - ID and other info of ZKP(a) owner
 * @param[in]   c                - c = hash(g, gv, ga, ID, info) mod q
 * @param[in]   g                - parameter g of ZKP(a)
 * @param[out]  zkp              - ZKP(a)
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.one of zkp_owner_info and c must be NULL, and another must be not NULL.
      -# 2.g is not necessarily the parameter g of J-PAKE.
  @endverbatim
 */
unsigned int jpake_verify_zkp_internal(JPAKE_PARA *jpake_para, JPAKE_USER_INFO *zkp_owner_info, unsigned int *c, unsigned int *g, JPAKE_ZKP *zkp)
{
    unsigned int t[JPAKE_MAX_WORD_LEN];
    unsigned int c_buf[JPAKE_MAX_WORD_LEN];

    unsigned int pWordLen = GET_WORD_LEN(jpake_para->pBitLen);
    unsigned int qWordLen = GET_WORD_LEN(jpake_para->qBitLen);
    unsigned int ret;

    //make sure ga in [1,p-1]
    if (uint32_BigNum_Check_Zero(zkp->ga, pWordLen)) {
        return JPAKE_ZERO_ALL;
    } else if (uint32_BigNumCmp(zkp->ga, pWordLen, jpake_para->p, pWordLen) >= 0) {
        return JPAKE_INTEGER_TOO_BIG;
    } else {
        ;
    }

    //set mod p
    ret = pke_set_modulus_and_pre_monts(jpake_para->p, jpake_para->p_h, jpake_para->p_n0, jpake_para->pBitLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    //ga^q should be 1 mod p
    ret = pke_modexp((unsigned int *)jpake_para->p, (unsigned int *)jpake_para->q, (unsigned int *)zkp->ga, t, pWordLen, qWordLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else if (1u != Bigint_Check_1(t, pWordLen)) {
        ret = JPAKE_INVALID_INPUT;
        goto END;
    } else {
        ;
    }

    //make sure r less than q
    if (uint32_BigNumCmp(zkp->r, qWordLen, jpake_para->q, qWordLen) >= 0) {
        ret = JPAKE_INVALID_INPUT;
        goto END;
    } else {
        ;
    }

    if ((NULL != zkp_owner_info) && (NULL == c)) {
        //c = hash(g, gv, ga, ID, info) mod q
        c   = c_buf;
        ret = jpake_get_zkp_c(jpake_para, g, zkp->gv, zkp->ga, zkp_owner_info->ID, zkp_owner_info->ID_bytes, zkp_owner_info->other_info, zkp_owner_info->other_info_bytes, c);
        if (JPAKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }
    } else if ((NULL == zkp_owner_info) && (NULL != c)) {
        ;
    } else {
        ret = JPAKE_INVALID_INPUT;
        goto END;
    }

    //make sure c < q
    if (uint32_BigNumCmp(c, qWordLen, jpake_para->q, qWordLen) >= 0) {
        return JPAKE_INTEGER_TOO_BIG;
    } else {
        ;
    }

    //set mod p
    ret = pke_set_modulus_and_pre_monts(jpake_para->p, jpake_para->p_h, jpake_para->p_n0, jpake_para->pBitLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    if (uint32_BigNum_Check_Zero(zkp->r, qWordLen) && uint32_BigNum_Check_Zero(c, qWordLen)) {
        pke_set_operand_uint32_value(t, qWordLen, 1);

        goto FINAL;
    } else if (uint32_BigNum_Check_Zero(zkp->r, qWordLen)) {
        //t = ga^c mod p
        ret = pke_modexp((unsigned int *)jpake_para->p, (unsigned int *)c, (unsigned int *)zkp->ga, t, pWordLen, qWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        goto FINAL;
    } else if (uint32_BigNum_Check_Zero(c, qWordLen)) {
        //t = g^r mod p
        ret = pke_modexp((unsigned int *)jpake_para->p, (unsigned int *)zkp->r, (unsigned int *)g, t, pWordLen, qWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        goto FINAL;
    } else {
        //t = g^r mod p
        ret = pke_modexp((unsigned int *)jpake_para->p, (unsigned int *)zkp->r, (unsigned int *)g, t, pWordLen, qWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        //c = ga^c mod p
        ret = pke_modexp((unsigned int *)jpake_para->p, (unsigned int *)c, (unsigned int *)zkp->ga, c, pWordLen, qWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        //t = (g^r)*(ga^c) mod p
        pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
        ret = pke_modmul_internal((unsigned int *)t, (unsigned int *)c, (unsigned int *)t, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }
    }

FINAL:

    if (uint32_BigNumCmp(zkp->gv, pWordLen, t, pWordLen) != 0) {
        return JPAKE_VERIFY_ZKP_FAILURE;
    } else {
        ;
    }

    ret = JPAKE_SUCCESS;

END:

    return ret;
}

/**
 * @brief       J-PAKE verify ZKP(a), here a is the private key of ZKP owner, not disclosed
 * @param[in]   jpake_para       - JPAKE_PARA struct pointer
 * @param[in]   zkp_owner_info   - ID and other info of ZKP(a) owner
 * @param[in]   g                - parameter g of ZKP(a)
 * @param[in]   zkp              - ZKP(a)
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.g is not necessarily the parameter g of J-PAKE.
  @endverbatim
 */
unsigned int jpake_verify_zkp(JPAKE_PARA *jpake_para, JPAKE_USER_INFO *zkp_owner_info, unsigned int *g, JPAKE_ZKP *zkp)
{
    return jpake_verify_zkp_internal(jpake_para, zkp_owner_info, NULL, g, zkp);
}

/**
 * @brief       J-PAKE round 1, get private key xa, xb, and ZKP(xa), ZKP(xb) for local user
 * @param[in]   jpake_para               - JPAKE_PARA struct pointer
 * @param[in]   local_zkp_owner_info     - ID and other info of local user
 * @param[out]  xa                       - private key xa of local user(x1 of Alice, or x3 of Bob)
 * @param[out]  xb                       - private key xb of local user(x2 of Alice, or x4 of Bob)
 * @param[out]  xa_zkp                   - ZKP(xa)
 * @param[out]  xb_zkp                   - ZKP(xb)
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.xa is in [0,q-1], xb is in [1,q-1].
  @endverbatim
 */
unsigned int jpake_round1_generate_xa_xb_and_local_two_zkps(JPAKE_PARA *jpake_para, JPAKE_USER_INFO *local_zkp_owner_info, unsigned int *xa, unsigned int *xb, JPAKE_ZKP *xa_zkp, JPAKE_ZKP *xb_zkp)
{
    unsigned int ret;

    if ((NULL == jpake_para) || (NULL == local_zkp_owner_info) || (NULL == xa) || (NULL == xb) ||
        (NULL == xa_zkp) || (NULL == xb_zkp)) {
        return JPAKE_POINTOR_NULL;
    } else {
        ;
    }

    ret = jpake_get_xa_xb_gxa_gxb(jpake_para, xa, xb, xa_zkp->ga, xb_zkp->ga);
    if (JPAKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = jpake_generate_zkp(jpake_para, local_zkp_owner_info, xa, jpake_para->g, xa_zkp);
    if (JPAKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = jpake_generate_zkp(jpake_para, local_zkp_owner_info, xb, jpake_para->g, xb_zkp);
    if (JPAKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = JPAKE_SUCCESS;

END:

    return ret;
}

/**
 * @brief       J-PAKE round 1, verify ZKP(xc), ZKP(xd) of peer user
 * @param[in]   jpake_para               - JPAKE_PARA struct pointer
 * @param[in]   peer_zkp_owner_info      - ID and other info of peer user
 * @param[in]   xc_zkp                   - ZKP(xc) of peer user(ZKP(x1) of Alice, or ZKP(x3) of Bob)
 * @param[in]   xd_zkp                   - ZKP(xd) of peer user(ZKP(x2) of Alice, or ZKP(x4) of Bob)
 * @return      0:success     other:error
 */
unsigned int jpake_round1_verify_peer_two_zkps(JPAKE_PARA *jpake_para, JPAKE_USER_INFO *peer_zkp_owner_info, JPAKE_ZKP *xc_zkp, JPAKE_ZKP *xd_zkp)
{
    unsigned int pWordLen;
    unsigned int ret;

    if ((NULL == jpake_para) || (NULL == peer_zkp_owner_info) || (NULL == xc_zkp) || (NULL == xd_zkp)) {
        return JPAKE_POINTOR_NULL;
    } else {
        ;
    }

    pWordLen = GET_WORD_LEN(jpake_para->pBitLen);

    ret = jpake_verify_zkp(jpake_para, peer_zkp_owner_info, jpake_para->g, xc_zkp);
    if (ret != JPAKE_SUCCESS) {
        goto END;
    } else {
        ;
    }

    ret = jpake_verify_zkp(jpake_para, peer_zkp_owner_info, jpake_para->g, xd_zkp);
    if (ret != JPAKE_SUCCESS) {
        goto END;
    } else {
        ;
    }

    //gd should not be 1 mod p
    if (1u == Bigint_Check_1(xd_zkp->ga, pWordLen)) {
        ret = JPAKE_INVALID_INPUT;
        goto END;
    } else {
        ;
    }

    ret = JPAKE_SUCCESS;

END:

    return ret;
}

/**
 * @brief       J-PAKE round 2, get local ZKP(xb*s) (ZKP(x2*s) of Alice, or ZKP(x4*s) of Bob)
 * @param[in]   jpake_para               - JPAKE_PARA struct pointer
 * @param[in]   local_zkp_owner_info     - ID and other info of local user
 * @param[out]  round2_ctx               - JPAKE_ROUND2_CTX struct pointer of local user
 * @param[in]   xb                       - private xb of local user(x2 of Alice, or x4 of Bob), less than q
 * @param[in]   gxa                      - public g^xa of local user(g^x1 of Alice, or g^x3 of Bob), less than p
 * @param[in]   gxc                      - public g^xc of peer user(g^x3 of Bob, or g^x1 of Alice), less than p
 * @param[in]   gxd                      - public g^xd of peer user(g^x4 of Bob, or g^x2 of Alice), less than p
 * @param[in]   s                        - shared secret of two sides
 * @param[in]   sWordLen                 - word length of s
 * @param[out]  local_zkp_xb_s           - ZKP(xb*s) of local user(g is (g^xa)(g^xc)(g^xd) mod p)
 * @return      0:success     other:error
 */
unsigned int jpake_round2_generate_local_zkp(JPAKE_PARA *jpake_para, JPAKE_USER_INFO *local_zkp_owner_info, JPAKE_ROUND2_CTX *round2_ctx, unsigned int *xb, unsigned int *gxa, unsigned int *gxc, unsigned int *gxd, unsigned int *s, unsigned int sWordLen, JPAKE_ZKP *local_zkp_xb_s)
{
    unsigned int g[JPAKE_MAX_WORD_LEN];

    unsigned int pWordLen;
    unsigned int qWordLen;
    unsigned int ret;

    if ((NULL == jpake_para) || (NULL == local_zkp_owner_info) || (NULL == round2_ctx) || (NULL == xb) || (NULL == gxa) ||
        (NULL == gxc) || (NULL == gxd) || (NULL == s) || (NULL == local_zkp_xb_s)) {
        return JPAKE_POINTOR_NULL;
    } else if (0 == sWordLen) {
        return JPAKE_INVALID_INPUT;
    } else {
        ;
    }

    pWordLen = GET_WORD_LEN(jpake_para->pBitLen);
    qWordLen = GET_WORD_LEN(jpake_para->qBitLen);

    //set mod p
    ret = pke_set_modulus_and_pre_monts(jpake_para->p, jpake_para->p_h, jpake_para->p_n0, jpake_para->pBitLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    //(g^xa)*(g^xc) mod p
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
    ret = pke_modmul_internal((unsigned int *)gxa, (unsigned int *)gxc, (unsigned int *)round2_ctx->gxa_gxc, pWordLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    //g = (g^xa)*(g^xc)*(g^xd) mod p
    ret = pke_modmul_internal((unsigned int *)round2_ctx->gxa_gxc, (unsigned int *)gxd, (unsigned int *)g, pWordLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    //set mod q
    ret = pke_set_modulus_and_pre_monts(jpake_para->q, jpake_para->q_h, jpake_para->q_n0, jpake_para->qBitLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    //get s mod q
    ret = pke_mod(s, get_valid_words(s, sWordLen), jpake_para->q, jpake_para->q_h, jpake_para->q_n0, qWordLen, round2_ctx->xb_s);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    //xb*s mod q
    ret = pke_modmul_internal((unsigned int *)xb, (unsigned int *)round2_ctx->xb_s, (unsigned int *)round2_ctx->xb_s, qWordLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    //set mod p
    pke_set_modulus_and_pre_monts(jpake_para->p, jpake_para->p_h, jpake_para->p_n0, jpake_para->pBitLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    //((g^xa)*(g^xc)*(g^xd))^(xb*s) mod p
    ret = pke_modexp((unsigned int *)jpake_para->p, (unsigned int *)round2_ctx->xb_s, (unsigned int *)g, local_zkp_xb_s->ga, pWordLen, qWordLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    //get zkp of xb*s
    ret = jpake_generate_zkp(jpake_para, local_zkp_owner_info, round2_ctx->xb_s, g, local_zkp_xb_s);
    if (ret != JPAKE_SUCCESS) {
        goto END;
    } else {
        ;
    }

    ret = JPAKE_SUCCESS;

END:

    return ret;
}

/**
 * @brief       J-PAKE round 2, verify peer ZKP(xb*s) (ZKP(x2*s) of Alice, or ZKP(x4*s) of Bob), and compute key
 * @param[in]   jpake_para               - JPAKE_PARA struct pointer
 * @param[in]   peer_zkp_owner_info      - ID and other info of peer user
 * @param[out]  round2_ctx               - JPAKE_ROUND2_CTX struct pointer of local user
 * @param[in]   xb                       - xb of local user(x2 of Alice, or x4 of Bob)
 * @param[in]   gxb                      - g^xb of local user(g^x2 of Alice, or g^x4 of Bob)
 * @param[in]   gxd                      - g^xd of peer user(g^x4 of Bob, or g^x2 of Alice)
 * @param[in]   peer_zkp_xb_s            - ZKP(xb*s) of peer user(g is (g^xa)(g^xc)(g^xb) mod p)
 * @param[out]  key                      - output key
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.round2_ctx must be used by jpake_round2_generate_local_zkp() by local user
  @endverbatim
 */
unsigned int jpake_round2_verify_peer_zkp_and_compute_key(JPAKE_PARA *jpake_para, JPAKE_USER_INFO *peer_zkp_owner_info, JPAKE_ROUND2_CTX *round2_ctx, unsigned int *xb, unsigned int *gxb, unsigned int *gxd, JPAKE_ZKP *peer_zkp_xb_s, unsigned int *key)
{
    unsigned int t[JPAKE_MAX_WORD_LEN];
    unsigned int pWordLen;
    unsigned int qWordLen;
    unsigned int ret;

    if ((NULL == jpake_para) || (NULL == peer_zkp_owner_info) || (NULL == round2_ctx) || (NULL == xb) || (NULL == gxb) ||
        (NULL == gxd) || (NULL == peer_zkp_xb_s) || (NULL == key)) {
        return JPAKE_POINTOR_NULL;
    } else {
        ;
    }

    pWordLen = GET_WORD_LEN(jpake_para->pBitLen);
    qWordLen = GET_WORD_LEN(jpake_para->qBitLen);

    //set mod p
    ret = pke_set_modulus_and_pre_monts(jpake_para->p, jpake_para->p_h, jpake_para->p_n0, jpake_para->pBitLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    //get t = ga*gb*gc as g for peer_zkp
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
    ret = pke_modmul_internal((unsigned int *)round2_ctx->gxa_gxc, (unsigned int *)gxb, t, pWordLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    //make sure new g != 1 mod p
    if (1u == Bigint_Check_1(t, pWordLen)) {
        ret = JPAKE_INVALID_INPUT;
        goto END;
    } else {
        ;
    }

    ret = jpake_verify_zkp(jpake_para, peer_zkp_owner_info, t, peer_zkp_xb_s);
    if (ret != JPAKE_SUCCESS) {
        goto END;
    } else {
        ;
    }

    //compute_key
    ret = pke_sub((unsigned int *)jpake_para->q, (unsigned int *)round2_ctx->xb_s, t, qWordLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

#if 0
    //set mod p
    ret = pke_set_modulus_and_pre_monts(jpake_para->p, jpake_para->p_h, jpake_para->p_n0, jpake_para->pBitLen);
    if(PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}
#endif

    ret = pke_modexp((unsigned int *)jpake_para->p, (unsigned int *)t, (unsigned int *)gxd, t, pWordLen, qWordLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
    ret = pke_modmul_internal((unsigned int *)peer_zkp_xb_s->ga, (unsigned int *)t, t, pWordLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = pke_modexp((unsigned int *)jpake_para->p, (unsigned int *)xb, (unsigned int *)t, key, pWordLen, qWordLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = JPAKE_SUCCESS;

END:

    return ret;
}

/**
 * @brief       J-PAKE round 3, compute H(H(k))
 * @param[in]   jpake_para               - JPAKE_PARA struct pointer
 * @param[in]   h_key                    - digest of the big number key after round 2
 * @param[out]  h_h_key                  - H(H(k))
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.sponsor compute H(H(k)) and send it to responsor, responsor recompute and compare.
  @endverbatim
 */
unsigned int jpake_round3_hash_hash_key(JPAKE_PARA *jpake_para, unsigned char *h_key, unsigned char *h_h_key)
{
    unsigned int digest_bytes;
    unsigned int ret;

    digest_bytes = hash_get_digest_word_len(jpake_para->hash_alg) << 2;

    ret = hash(jpake_para->hash_alg, h_key, digest_bytes, h_h_key);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = JPAKE_SUCCESS;

END:

    return ret;
}

/**
 * @brief       J-PAKE round 3, compute H(k)
 * @param[in]   jpake_para               - JPAKE_PARA struct pointer
 * @param[in]   key                    - the big number key after round 2
 * @param[out]  h_key                  - H(H(k))
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.if responsor compare H(H(k)) successfully, responsor compute H(k) and send it to sponsor,
           sponsor recompute and compare.
  @endverbatim
 */
unsigned int jpake_round3_hash_key(JPAKE_PARA *jpake_para, unsigned int *key, unsigned char *h_key)
{
    unsigned int t[JPAKE_MAX_WORD_LEN];
    unsigned int pByteLen;
    HASH_CTX     hash_ctx[1];
    unsigned int ret;

    pByteLen = GET_BYTE_LEN(jpake_para->pBitLen);

    ret = hash_init(hash_ctx, jpake_para->hash_alg);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    reverse_byte_array((unsigned char *)key, (unsigned char *)t, pByteLen);
    ret = hash_update(hash_ctx, (unsigned char *)t, pByteLen);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = hash_final(hash_ctx, h_key);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = JPAKE_SUCCESS;

END:

    return ret;
}
