/*! @file jpake.c */
#include <stdio.h>

#include "lib/include/crypto_common/utility.h"
#include "lib/include/pke/jpake.h"
#include "lib/include/trng/trng.h"

extern unsigned int jpake_hash_length(hash_ctx_t *ctx, unsigned int msg_len);
extern unsigned int jpake_hash_string(hash_ctx_t *ctx, unsigned char *s, unsigned int byteLen);
extern unsigned int jpake_get_rand_xa_less_than_q(unsigned int *q, unsigned int *xa, unsigned int wlen, unsigned int remainder_bits, unsigned int could_be_zero);

/**
 * @brief           J-PAKE hash big number
 * @param[in]       ctx                  - hash_ctx_t struct pointer
 * @param[in]       a                    - big number to be hashed
 * @param[in]       t                    - temporary buffer with same length as a
 * @param[in]       byteLen              - byte length of a or t
 * @return          0:success     other:error
 */
unsigned int jpake_hash_big_number(hash_ctx_t *ctx, unsigned int *a, unsigned int *t, unsigned int byteLen)
{
    unsigned int ret;

    ret = jpake_hash_length(ctx, byteLen);
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    reverse_byte_array((unsigned char *)a, (unsigned char *)t, byteLen);

    return hash_update(ctx, (const unsigned char *)t, byteLen);
}

/**
 * @brief           J-PAKE get c = h mod q for ZKP(a), here h=hash(g,V,A,ID,other_info)
 * @param[in]       jpake_para           - jpake_para_t struct pointer
 * @param[in]       g                    - parameter g of ZKP(a), a is the private key of ZKP(a) owner.
 * @param[in]       V                    - g^v mod p, p is modulus, v is the random secret of ZKP(a)
 * @param[out]      A                    - g^a mod p, public key of ZKP(a) owner.
 * @param[in]       ID                   - ID of ZKP(a) owner.
 * @param[in]       ID_bytes             - ID byte length.
 * @param[in]       other_info           - other information of ZKP(a) owner.
 * @param[in]       other_info_bytes     - other_info byte length.
 * @param[in]       c                    - c = h mod q.
 * @return          0:success     other:error
 * @note
 *        1.c is a big number with the same length as the parameter q of ZKP(a).
 *        2.g is not necessarily the parameter g of J-PAKE.
 */
unsigned int jpake_get_zkp_c(jpake_para_t *jpake_para, unsigned int *g, unsigned int *V, unsigned int *A, unsigned char *ID, unsigned int ID_bytes, unsigned char *other_info,
                             unsigned int other_info_bytes, unsigned int *c)
{
    unsigned int p_len = get_byte_len(jpake_para->p_bitlen);
    unsigned int q_wlen = get_word_len(jpake_para->qBitLen);
    unsigned int digest_bytes;
    unsigned int t[JPAKE_MAX_WORD_LEN];
    hash_ctx_t hash_ctx[1];
    unsigned int ret;

    digest_bytes = hash_get_digest_word_len(jpake_para->hash_alg) << 2;

    ret = hash_init(hash_ctx, jpake_para->hash_alg);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // hash input g
    jpake_hash_big_number(hash_ctx, g, t, p_len);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // hash input V
    jpake_hash_big_number(hash_ctx, V, t, p_len);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // hash input A
    jpake_hash_big_number(hash_ctx, A, t, p_len);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // hash input ID
    ret = jpake_hash_string(hash_ctx, ID, ID_bytes);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // hash input other info
    if (other_info)
    {
        ret = jpake_hash_string(hash_ctx, other_info, other_info_bytes);
        if (HASH_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }
    }

    t[((digest_bytes + 3) / 4) - 1] = 0;
    ret = hash_final(hash_ctx, (unsigned char *)t);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    if ((jpake_para->qBitLen) > (digest_bytes << 3))
    {
        reverse_byte_array((unsigned char *)t, (unsigned char *)c, digest_bytes);
        memset_(((unsigned char *)c) + digest_bytes, 0, (q_wlen << 2) - digest_bytes);
    }
    else
    {
        reverse_byte_array((unsigned char *)t, (unsigned char *)t, digest_bytes);

        ret = pke_mod(t, get_valid_words(t, digest_bytes / 4), jpake_para->q, jpake_para->q_h, jpake_para->q_n0, q_wlen, c);
        if (PKE_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }
    }

    ret = JPAKE_SUCCESS;

END:

    return ret;
}

/**
 * @brief           J-PAKE get xa, xb, g^xa, g^xb
 * @param[in]       jpake_para           - jpake_para_t struct pointer
 * @param[in]       xa                   - random big number less than q, private key
 * @param[in]       xb                   - random big number less than q, private key
 * @param[out]      gxa                  - random big number g^xa, public key
 * @param[in]       gxb                  - random big number g^xb, public key
 * @return          0:success     other:error
 * @note
 *        1.xa is in [0,q-1], xb is in [1,q-1].
 */
unsigned int jpake_get_xa_xb_gxa_gxb(jpake_para_t *jpake_para, unsigned int *xa, unsigned int *xb, unsigned int *gxa, unsigned int *gxb)
{
    unsigned int remainder_bits = jpake_para->qBitLen & 31;
    unsigned int p_wlen = get_word_len(jpake_para->p_bitlen);
    unsigned int q_wlen = get_word_len(jpake_para->qBitLen);
    unsigned int ret;

    ret = jpake_get_rand_xa_less_than_q(jpake_para->q, xa, q_wlen, remainder_bits, 1);
    if (ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = jpake_get_rand_xa_less_than_q(jpake_para->q, xb, q_wlen, remainder_bits, 0);
    if (ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // set mod p
    ret = pke_set_modulus_and_pre_monts(jpake_para->p, jpake_para->p_h, jpake_para->p_n0, jpake_para->p_bitlen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = pke_modexp((const unsigned int *)jpake_para->p, (const unsigned int *)xa, (const unsigned int *)jpake_para->g, gxa, p_wlen, q_wlen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = pke_modexp((const unsigned int *)jpake_para->p, (const unsigned int *)xb, (const unsigned int *)jpake_para->g, gxb, p_wlen, q_wlen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = JPAKE_SUCCESS;

END:

    return ret;
}

/**
 * @brief           J-PAKE get ZKP(a) internal API
 * @param[in]       jpake_para           - jpake_para_t struct pointer
 * @param[in]       zkp_owner_info       - ID and other info of ZKP(a) owner
 * @param[in]       c                    - c = hash(g, gv, ga, ID, info) mod q
 * @param[out]      a                    - private key of ZKP(a) owner
 * @param[in]       v                    - random secret value for ZKP(a), must be in [0, q-1]
 * @param[in]       g                    - parameter g of ZKP(a)
 * @param[in]       zkp                  - ZKP(a)
 * @return          0:success     other:error
 * @note
 *        1.one of zkp_owner_info and c must be NULL, and another must be not NULL
 *        2.v could be NULL, in this case, v will be generated inside
 *        3.g is not necessarily the parameter g of J-PAKE.
 */
unsigned int jpake_generate_zkp_internal(jpake_para_t *jpake_para, jpake_user_info_t *zkp_owner_info, unsigned int *c, unsigned int *a, unsigned int *v, unsigned int *g,
                                         jpake_zkp_t *zkp)
{
    unsigned int v_buf[JPAKE_MAX_WORD_LEN];
    unsigned int c_buf[JPAKE_MAX_WORD_LEN];
    unsigned int remainder_bits = jpake_para->qBitLen & 31;
    unsigned int p_wlen = get_word_len(jpake_para->p_bitlen);
    unsigned int q_wlen = get_word_len(jpake_para->qBitLen);
    unsigned int ret;

    if (NULL == v)
    {
        // get random v in [0, q-1]
        v = v_buf;
        ret = jpake_get_rand_xa_less_than_q(jpake_para->q, v, q_wlen, remainder_bits, 1);
        if (ret)
        {
            goto END;
        }
        else
        {
            ;
        }
    }
    else
    {
        ;
    }

    // set mod p
    ret = pke_set_modulus_and_pre_monts(jpake_para->p, jpake_para->p_h, jpake_para->p_n0, jpake_para->p_bitlen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // gv = g^v mod p
    ret = pke_modexp((const unsigned int *)jpake_para->p, (const unsigned int *)v, (const unsigned int *)g, zkp->gv, p_wlen, q_wlen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    if ((NULL != zkp_owner_info) && (NULL == c))
    {
        // c = hash(g, gv, ga, ID, info) mod q
        c = c_buf;
        ret = jpake_get_zkp_c(jpake_para, g, zkp->gv, zkp->ga, zkp_owner_info->ID, zkp_owner_info->ID_bytes, zkp_owner_info->other_info, zkp_owner_info->other_info_bytes, c);
        if (JPAKE_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }
    }
    else if ((NULL == zkp_owner_info) && (NULL != c))
    {
        ;
    }
    else
    {
        ret = JPAKE_INVALID_INPUT;
        goto END;
    }

    // make sure c < q
    if (uint32_big_num_cmp(c, q_wlen, jpake_para->q, q_wlen) >= 0)
    {
        return JPAKE_INTEGER_TOO_BIG;
    }
    else
    {
        ;
    }

    // set mod q
    ret = pke_set_modulus_and_pre_monts(jpake_para->q, jpake_para->q_h, jpake_para->q_n0, jpake_para->qBitLen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // r = a*c mod q
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
    ret = pke_modmul_internal((const unsigned int *)a, (const unsigned int *)c, zkp->r, q_wlen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // r = v - a*c mod q
    ret = pke_modsub((const unsigned int *)jpake_para->q, (const unsigned int *)v, (const unsigned int *)zkp->r, zkp->r, q_wlen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = JPAKE_SUCCESS;

END:

    return ret;
}

/**
 * @brief           J-PAKE get ZKP(a)
 * @param[in]       jpake_para           - jpake_para_t struct pointer
 * @param[in]       zkp_owner_info       - ID and other info of ZKP(a) owner
 * @param[in]       a                    - private key of ZKP(a) owner
 * @param[in]       g                    - parameter g of ZKP(a)
 * @param[out]      zkp                  - ZKP(a)
 * @return          0:success     other:error
 * @note
 *        1.g is not necessarily the parameter g of J-PAKE.
 */
unsigned int jpake_generate_zkp(jpake_para_t *jpake_para, jpake_user_info_t *zkp_owner_info, unsigned int *a, unsigned int *g, jpake_zkp_t *zkp)
{
    return jpake_generate_zkp_internal(jpake_para, zkp_owner_info, NULL, a, NULL, g, zkp);
}

/**
 * @brief           J-PAKE verify ZKP(a) internal API, here a is the private key of ZKP owner, not disclosed
 * @param[in]       jpake_para           - jpake_para_t struct pointer
 * @param[in]       zkp_owner_info       - ID and other info of ZKP(a) owner
 * @param[in]       c                    - c = hash(g, gv, ga, ID, info) mod q
 * @param[in]       g                    - parameter g of ZKP(a)
 * @param[out]      zkp                  - ZKP(a)
 * @return          0:success     other:error
 * @note
 *        1.one of zkp_owner_info and c must be NULL, and another must be not NULL.
 *        2.g is not necessarily the parameter g of J-PAKE.
 */
unsigned int jpake_verify_zkp_internal(jpake_para_t *jpake_para, jpake_user_info_t *zkp_owner_info, unsigned int *c, unsigned int *g, jpake_zkp_t *zkp)
{
    unsigned int t[JPAKE_MAX_WORD_LEN];
    unsigned int c_buf[JPAKE_MAX_WORD_LEN];

    unsigned int p_wlen = get_word_len(jpake_para->p_bitlen);
    unsigned int q_wlen = get_word_len(jpake_para->qBitLen);
    unsigned int ret;

    // make sure ga in [1,p-1]
    if (uint32_bignum_check_zero(zkp->ga, p_wlen))
    {
        return JPAKE_ZERO_ALL;
    }
    else if (uint32_big_num_cmp(zkp->ga, p_wlen, jpake_para->p, p_wlen) >= 0)
    {
        return JPAKE_INTEGER_TOO_BIG;
    }
    else
    {
        ;
    }

    // set mod p
    ret = pke_set_modulus_and_pre_monts(jpake_para->p, jpake_para->p_h, jpake_para->p_n0, jpake_para->p_bitlen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // ga^q should be 1 mod p
    ret = pke_modexp((const unsigned int *)jpake_para->p, (const unsigned int *)jpake_para->q, (const unsigned int *)zkp->ga, t, p_wlen, q_wlen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else if (1u != bigint_check_1(t, p_wlen))
    {
        ret = JPAKE_INVALID_INPUT;
        goto END;
    }
    else
    {
        ;
    }

    // make sure r less than q
    if (uint32_big_num_cmp(zkp->r, q_wlen, jpake_para->q, q_wlen) >= 0)
    {
        ret = JPAKE_INVALID_INPUT;
        goto END;
    }
    else
    {
        ;
    }

    if ((NULL != zkp_owner_info) && (NULL == c))
    {
        // c = hash(g, gv, ga, ID, info) mod q
        c = c_buf;
        ret = jpake_get_zkp_c(jpake_para, g, zkp->gv, zkp->ga, zkp_owner_info->ID, zkp_owner_info->ID_bytes, zkp_owner_info->other_info, zkp_owner_info->other_info_bytes, c);
        if (JPAKE_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }
    }
    else if ((NULL == zkp_owner_info) && (NULL != c))
    {
        ;
    }
    else
    {
        ret = JPAKE_INVALID_INPUT;
        goto END;
    }

    // make sure c < q
    if (uint32_big_num_cmp(c, q_wlen, jpake_para->q, q_wlen) >= 0)
    {
        return JPAKE_INTEGER_TOO_BIG;
    }
    else
    {
        ;
    }

    // set mod p
    ret = pke_set_modulus_and_pre_monts(jpake_para->p, jpake_para->p_h, jpake_para->p_n0, jpake_para->p_bitlen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    if (uint32_bignum_check_zero(zkp->r, q_wlen) && uint32_bignum_check_zero(c, q_wlen))
    {
        pke_set_operand_uint32_value(t, q_wlen, 1);

        goto FINAL;
    }
    else if (uint32_bignum_check_zero(zkp->r, q_wlen))
    {
        // t = ga^c mod p
        ret = pke_modexp((const unsigned int *)jpake_para->p, (const unsigned int *)c, (const unsigned int *)zkp->ga, t, p_wlen, q_wlen);
        if (PKE_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }

        goto FINAL;
    }
    else if (uint32_bignum_check_zero(c, q_wlen))
    {
        // t = g^r mod p
        ret = pke_modexp((const unsigned int *)jpake_para->p, (const unsigned int *)zkp->r, (const unsigned int *)g, t, p_wlen, q_wlen);
        if (PKE_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }

        goto FINAL;
    }
    else
    {
        // t = g^r mod p
        ret = pke_modexp((const unsigned int *)jpake_para->p, (const unsigned int *)zkp->r, (const unsigned int *)g, t, p_wlen, q_wlen);
        if (PKE_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }

        // c = ga^c mod p
        ret = pke_modexp((const unsigned int *)jpake_para->p, (const unsigned int *)c, (const unsigned int *)zkp->ga, c, p_wlen, q_wlen);
        if (PKE_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }

        // t = (g^r)*(ga^c) mod p
        pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
        ret = pke_modmul_internal((const unsigned int *)t, (const unsigned int *)c, (unsigned int *)t, p_wlen);
        if (PKE_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }
    }

FINAL:

    if (uint32_big_num_cmp(zkp->gv, p_wlen, t, p_wlen) != 0)
    {
        return JPAKE_VERIFY_ZKP_FAILURE;
    }
    else
    {
        ;
    }

    ret = JPAKE_SUCCESS;

END:

    return ret;
}

/**
 * @brief           J-PAKE verify ZKP(a), here a is the private key of ZKP owner, not disclosed
 * @param[in]       jpake_para           - jpake_para_t struct pointer
 * @param[in]       zkp_owner_info       - ID and other info of ZKP(a) owner
 * @param[in]       g                    - parameter g of ZKP(a)
 * @param[in]       zkp                  - ZKP(a)
 * @return          0:success     other:error
 * @note
 *        1.g is not necessarily the parameter g of J-PAKE.
 */
unsigned int jpake_verify_zkp(jpake_para_t *jpake_para, jpake_user_info_t *zkp_owner_info, unsigned int *g, jpake_zkp_t *zkp)
{
    return jpake_verify_zkp_internal(jpake_para, zkp_owner_info, NULL, g, zkp);
}

/**
 * @brief           J-PAKE round 1, get private key xa, xb, and ZKP(xa), ZKP(xb) for local user
 * @param[in]       jpake_para           - jpake_para_t struct pointer
 * @param[in]       local_zkp_owner_info - ID and other info of local user
 * @param[out]      xa                   - private key xa of local user(x1 of Alice, or x3 of Bob)
 * @param[out]      xb                   - private key xb of local user(x2 of Alice, or x4 of Bob)
 * @param[out]      xa_zkp               - ZKP(xa)
 * @param[out]      xb_zkp               - ZKP(xb)
 * @return          0:success     other:error
 * @note
 *        1.xa is in [0,q-1], xb is in [1,q-1].
 */
unsigned int jpake_round1_generate_xa_xb_and_local_two_zkps(jpake_para_t *jpake_para, jpake_user_info_t *local_zkp_owner_info, unsigned int *xa, unsigned int *xb,
                                                            jpake_zkp_t *xa_zkp, jpake_zkp_t *xb_zkp)
{
    unsigned int ret;

    if ((NULL == jpake_para) || (NULL == local_zkp_owner_info) || (NULL == xa) || (NULL == xb) || (NULL == xa_zkp) || (NULL == xb_zkp))
    {
        return JPAKE_POINTER_NULL;
    }
    else
    {
        ;
    }

    ret = jpake_get_xa_xb_gxa_gxb(jpake_para, xa, xb, xa_zkp->ga, xb_zkp->ga);
    if (JPAKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = jpake_generate_zkp(jpake_para, local_zkp_owner_info, xa, jpake_para->g, xa_zkp);
    if (JPAKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = jpake_generate_zkp(jpake_para, local_zkp_owner_info, xb, jpake_para->g, xb_zkp);
    if (JPAKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = JPAKE_SUCCESS;

END:

    return ret;
}

/**
 * @brief           J-PAKE round 1, verify ZKP(xc), ZKP(xd) of peer user
 * @param[in]       jpake_para           - jpake_para_t struct pointer
 * @param[in]       peer_zkp_owner_info  - ID and other info of peer user
 * @param[in]       xc_zkp               - ZKP(xc) of peer user(ZKP(x1) of Alice, or ZKP(x3) of Bob)
 * @param[in]       xd_zkp               - ZKP(xd) of peer user(ZKP(x2) of Alice, or ZKP(x4) of Bob)
 * @return          0:success     other:error
 */
unsigned int jpake_round1_verify_peer_two_zkps(jpake_para_t *jpake_para, jpake_user_info_t *peer_zkp_owner_info, jpake_zkp_t *xc_zkp, jpake_zkp_t *xd_zkp)
{
    unsigned int p_wlen;
    unsigned int ret;

    if ((NULL == jpake_para) || (NULL == peer_zkp_owner_info) || (NULL == xc_zkp) || (NULL == xd_zkp))
    {
        return JPAKE_POINTER_NULL;
    }
    else
    {
        ;
    }

    p_wlen = get_word_len(jpake_para->p_bitlen);

    ret = jpake_verify_zkp(jpake_para, peer_zkp_owner_info, jpake_para->g, xc_zkp);
    if (ret != JPAKE_SUCCESS)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = jpake_verify_zkp(jpake_para, peer_zkp_owner_info, jpake_para->g, xd_zkp);
    if (ret != JPAKE_SUCCESS)
    {
        goto END;
    }
    else
    {
        ;
    }

    // gd should not be 1 mod p
    if (1u == bigint_check_1(xd_zkp->ga, p_wlen))
    {
        ret = JPAKE_INVALID_INPUT;
        goto END;
    }
    else
    {
        ;
    }

    ret = JPAKE_SUCCESS;

END:

    return ret;
}

/**
 * @brief           J-PAKE round 2, get local ZKP(xb*s) (ZKP(x2*s) of Alice, or ZKP(x4*s) of Bob)
 * @param[in]       jpake_para           - jpake_para_t struct pointer
 * @param[in]       local_zkp_owner_info - ID and other info of local user
 * @param[out]      round2_ctx           - jpake_round2_ctx_t struct pointer of local user
 * @param[in]       xb                   - private xb of local user(x2 of Alice, or x4 of Bob), less than q
 * @param[in]       gxa                  - public g^xa of local user(g^x1 of Alice, or g^x3 of Bob), less than p
 * @param[in]       gxc                  - public g^xc of peer user(g^x3 of Bob, or g^x1 of Alice), less than p
 * @param[in]       gxd                  - public g^xd of peer user(g^x4 of Bob, or g^x2 of Alice), less than p
 * @param[in]       s                    - shared secret of two sides
 * @param[in]       s_wlen               - word length of s
 * @param[out]      local_zkp_xb_s       - ZKP(xb*s) of local user(g is (g^xa)(g^xc)(g^xd) mod p)
 * @return          0:success     other:error
 */
unsigned int jpake_round2_generate_local_zkp(jpake_para_t *jpake_para, jpake_user_info_t *local_zkp_owner_info, jpake_round2_ctx_t *round2_ctx, unsigned int *xb, unsigned int *gxa,
                                             unsigned int *gxc, unsigned int *gxd, unsigned int *s, unsigned int s_wlen, jpake_zkp_t *local_zkp_xb_s)
{
    unsigned int g[JPAKE_MAX_WORD_LEN];

    unsigned int p_wlen;
    unsigned int q_wlen;
    unsigned int ret;

    if ((NULL == jpake_para) || (NULL == local_zkp_owner_info) || (NULL == round2_ctx) || (NULL == xb) || (NULL == gxa) || (NULL == gxc) || (NULL == gxd) || (NULL == s) ||
        (NULL == local_zkp_xb_s))
    {
        return JPAKE_POINTER_NULL;
    }
    else if (0 == s_wlen)
    {
        return JPAKE_INVALID_INPUT;
    }
    else
    {
        ;
    }

    p_wlen = get_word_len(jpake_para->p_bitlen);
    q_wlen = get_word_len(jpake_para->qBitLen);

    // set mod p
    ret = pke_set_modulus_and_pre_monts(jpake_para->p, jpake_para->p_h, jpake_para->p_n0, jpake_para->p_bitlen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    //(g^xa)*(g^xc) mod p
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
    ret = pke_modmul_internal((const unsigned int *)gxa, (const unsigned int *)gxc, (unsigned int *)round2_ctx->gxa_gxc, p_wlen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // g = (g^xa)*(g^xc)*(g^xd) mod p
    ret = pke_modmul_internal((const unsigned int *)round2_ctx->gxa_gxc, (const unsigned int *)gxd, (unsigned int *)g, p_wlen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // set mod q
    ret = pke_set_modulus_and_pre_monts(jpake_para->q, jpake_para->q_h, jpake_para->q_n0, jpake_para->qBitLen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // get s mod q
    ret = pke_mod(s, get_valid_words(s, s_wlen), jpake_para->q, jpake_para->q_h, jpake_para->q_n0, q_wlen, round2_ctx->xb_s);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // xb*s mod q
    ret = pke_modmul_internal((const unsigned int *)xb, (const unsigned int *)round2_ctx->xb_s, (unsigned int *)round2_ctx->xb_s, q_wlen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // set mod p
    pke_set_modulus_and_pre_monts(jpake_para->p, jpake_para->p_h, jpake_para->p_n0, jpake_para->p_bitlen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    //((g^xa)*(g^xc)*(g^xd))^(xb*s) mod p
    ret = pke_modexp((const unsigned int *)jpake_para->p, (const unsigned int *)round2_ctx->xb_s, (const unsigned int *)g, local_zkp_xb_s->ga, p_wlen, q_wlen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // get zkp of xb*s
    ret = jpake_generate_zkp(jpake_para, local_zkp_owner_info, round2_ctx->xb_s, g, local_zkp_xb_s);
    if (ret != JPAKE_SUCCESS)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = JPAKE_SUCCESS;

END:

    return ret;
}

/**
 * @brief           J-PAKE round 2, verify peer ZKP(xb*s) (ZKP(x2*s) of Alice, or ZKP(x4*s) of Bob), and compute key
 * @param[in]       jpake_para           - jpake_para_t struct pointer
 * @param[in]       peer_zkp_owner_info  - ID and other info of peer user
 * @param[out]      round2_ctx           - jpake_round2_ctx_t struct pointer of local user
 * @param[in]       xb                   - xb of local user(x2 of Alice, or x4 of Bob)
 * @param[in]       gxb                  - g^xb of local user(g^x2 of Alice, or g^x4 of Bob)
 * @param[in]       gxd                  - g^xd of peer user(g^x4 of Bob, or g^x2 of Alice)
 * @param[in]       peer_zkp_xb_s        - ZKP(xb*s) of peer user(g is (g^xa)(g^xc)(g^xb) mod p)
 * @param[out]      key                  - output key
 * @return          0:success     other:error
 * @note
 *        1.round2_ctx must be used by jpake_round2_generate_local_zkp() by local user
 */
unsigned int jpake_round2_verify_peer_zkp_and_compute_key(jpake_para_t *jpake_para, jpake_user_info_t *peer_zkp_owner_info, jpake_round2_ctx_t *round2_ctx, unsigned int *xb,
                                                          unsigned int *gxb, unsigned int *gxd, jpake_zkp_t *peer_zkp_xb_s, unsigned int *key)
{
    unsigned int t[JPAKE_MAX_WORD_LEN];
    unsigned int p_wlen;
    unsigned int q_wlen;
    unsigned int ret;

    if ((NULL == jpake_para) || (NULL == peer_zkp_owner_info) || (NULL == round2_ctx) || (NULL == xb) || (NULL == gxb) || (NULL == gxd) || (NULL == peer_zkp_xb_s) ||
        (NULL == key))
    {
        return JPAKE_POINTER_NULL;
    }
    else
    {
        ;
    }

    p_wlen = get_word_len(jpake_para->p_bitlen);
    q_wlen = get_word_len(jpake_para->qBitLen);

    // set mod p
    ret = pke_set_modulus_and_pre_monts(jpake_para->p, jpake_para->p_h, jpake_para->p_n0, jpake_para->p_bitlen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // get t = ga*gb*gc as g for peer_zkp
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
    ret = pke_modmul_internal((const unsigned int *)round2_ctx->gxa_gxc, (const unsigned int *)gxb, t, p_wlen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // make sure new g != 1 mod p
    if (1u == bigint_check_1(t, p_wlen))
    {
        ret = JPAKE_INVALID_INPUT;
        goto END;
    }
    else
    {
        ;
    }

    ret = jpake_verify_zkp(jpake_para, peer_zkp_owner_info, t, peer_zkp_xb_s);
    if (ret != JPAKE_SUCCESS)
    {
        goto END;
    }
    else
    {
        ;
    }

    // compute_key
    ret = pke_sub((const unsigned int *)jpake_para->q, (const unsigned int *)round2_ctx->xb_s, t, q_wlen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

#if 0
    //set mod p
    ret = pke_set_modulus_and_pre_monts(jpake_para->p, jpake_para->p_h, jpake_para->p_n0, jpake_para->p_bitlen);
    if(PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}
#endif

    ret = pke_modexp((const unsigned int *)jpake_para->p, (const unsigned int *)t, (const unsigned int *)gxd, t, p_wlen, q_wlen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
    ret = pke_modmul_internal((const unsigned int *)peer_zkp_xb_s->ga, (const unsigned int *)t, t, p_wlen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = pke_modexp((const unsigned int *)jpake_para->p, (const unsigned int *)xb, (const unsigned int *)t, key, p_wlen, q_wlen);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = JPAKE_SUCCESS;

END:

    return ret;
}

/**
 * @brief           J-PAKE round 3, compute H(H(k))
 * @param[in]       jpake_para           - jpake_para_t struct pointer
 * @param[in]       h_key                - digest of the big number key after round 2
 * @param[out]      h_h_key              - H(H(k))
 * @return          0:success     other:error
 * @note
 *        1.sponsor compute H(H(k)) and send it to responsor, responsor recompute and compare.
 */
unsigned int jpake_round3_hash_hash_key(jpake_para_t *jpake_para, unsigned char *h_key, unsigned char *h_h_key)
{
    unsigned int digest_bytes;
    unsigned int ret;

    digest_bytes = hash_get_digest_word_len(jpake_para->hash_alg) << 2;

    ret = hash(jpake_para->hash_alg, h_key, digest_bytes, h_h_key);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = JPAKE_SUCCESS;

END:

    return ret;
}

/**
 * @brief           J-PAKE round 3, compute H(k)
 * @param[in]       jpake_para           - jpake_para_t struct pointer
 * @param[in]       key                  - the big number key after round 2
 * @param[out]      h_key                - H(H(k))
 * @return          0:success     other:error
 * @note
 *        1.if responsor compare H(H(k)) successfully, responsor compute H(k) and send it to sponsor, sponsor recompute and compare.
 */
unsigned int jpake_round3_hash_key(jpake_para_t *jpake_para, unsigned int *key, unsigned char *h_key)
{
    unsigned int t[JPAKE_MAX_WORD_LEN];
    unsigned int p_len;
    hash_ctx_t hash_ctx[1];
    unsigned int ret;

    p_len = get_byte_len(jpake_para->p_bitlen);

    ret = hash_init(hash_ctx, jpake_para->hash_alg);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    reverse_byte_array((unsigned char *)key, (unsigned char *)t, p_len);
    ret = hash_update(hash_ctx, (const unsigned char *)t, p_len);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = hash_final(hash_ctx, h_key);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = JPAKE_SUCCESS;

END:

    return ret;
}
