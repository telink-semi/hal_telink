/*! @file dh_basic.c */
#include "lib/include/pke/pke_config.h"

#ifdef SUPPORT_DH

#include "lib/include/crypto_common/utility.h"
#include "lib/include/pke/dh.h"
#include "lib/include/trng/trng.h"

/**
 * @brief           DH parameters pointer init, set pointers of (p, q, g)
 * @param[in]       dh_para              - DH_PARA struct pointer.
 * @param[in]       p_buf                - a unsigned int buffer holds p, the prime defining the GF(p).
 * @param[in]       p_bits               - bit length of p.
 * @param[in]       p_h_buf              - a unsigned int buffer holds pre-calculated mont parameters H(R^2 mod p).
 * @param[in]       p_n0_buf             - a unsigned int buffer holds pre-calculated mont parameters n0'(-modulus^(-1) mod 2^w).
 * @param[in]       q_buf                - a unsigned int buffer holds q, a prime factor of p-1, aka order of g..
 * @param[in]       q_bits               - bit length of q.
 * @param[in]       g_buf                - a unsigned int buffer holds g, a generator of the q-order subgroup of GF(p)*.
 * @param[in]       g_bits               - bit length of g.
 * @return          0:success     other:error
 * @note
 *        1.p_h_buf holds H(R^2 mod p), to accelerate DH calculation, it occupies
 *           the same size memory as well as p_buf. if you do not have this, please set
 *           p_h_buf to NULL.
 *        2.p_n0_buf holds n0'(-modulus^(-1) mod 2^w), to accelerate DH
 *           calculation, it occupies (w+7)/8 bytes, here w is actually 32. if you do not
 *           have this, please set p_n0_buf to NULL.
 */
unsigned int dh_param_pointer_init(dh_para_t *dh_para, unsigned int *p_buf, unsigned int p_bits, unsigned int *p_h_buf, unsigned int *p_n0_buf, unsigned int *q_buf,
                                   unsigned int q_bits, unsigned int *g_buf, unsigned int g_bits)
{
    if ((NULL == dh_para) || (NULL == p_buf) || (NULL == q_buf) || (NULL == g_buf))
    {
        return DH_POINTER_NULL;
    }
    else if ((0 == p_bits) || (0 == q_bits) || (0 == g_bits))
    {
        return DH_INVALID_INPUT;
    }
    else if ((q_bits > p_bits) || (g_bits > p_bits))
    {
        return DH_INVALID_INPUT;
    }
    else if (p_bits > DH_MAX_BIT_LEN)
    {
        return DH_INVALID_INPUT;
    }
    else
    {
        ;
    }

    dh_para->p = p_buf;
    dh_para->p_bits = p_bits;
    dh_para->p_h = p_h_buf;
    dh_para->p_n0 = p_n0_buf;
    dh_para->q = q_buf;
    dh_para->q_bits = q_bits;
    dh_para->g = g_buf;
    dh_para->g_bits = g_bits;

    return DH_SUCCESS;
}

/**
 * @brief           DH parameters value init, set pointers of (p, q, g)
 * @param[in]       dh_para              - Pointer to the DH_PARA struct.
 * @param[in]       p                    - A prime defining the GF(p).
 * @param[in]       p_h                  - The pre-calculated mont parameter (R^2 mod p).
 * @param[in]       p_n0                 - The pre-calculated mont parameter (-modulus^(-1) mod 2^w).
 * @param[in]       q                    - A prime factor of p-1, aka order of g.
 * @param[in]       g                    - A generator of the q-order subgroup of GF(p)*.
 * @return          0: success    other: error
 * @note
 *        1. Please call dh_param_pointer_init() before calling this function.
 *        2. The input p occupies (dh_para->p_bits+7)/8 bytes;
 *           the input p_h occupies (dh_para->p_bits+7)/8 bytes, if you have this;
 *           the input p_n0 occupies (w+7)/8 bytes, if you have this, w is actually 32 here;
 *           the input q occupies (dh_para->q_bits+7)/8 bytes;
 *           the input g occupies (dh_para->g_bits+7)/8 bytes.
 *        3. If you do not have p_h, please set p_h to NULL,
 *           if you do not have p_n0, please set p_n0 to NULL.
 */
unsigned int dh_param_value_init(const dh_para_t *dh_para, const unsigned char *p, const unsigned char *p_h, const unsigned char *p_n0, const unsigned char *q,
                                 const unsigned char *g)
{
    unsigned int p_len, q_len, g_len;

    if ((NULL == dh_para) || (NULL == p) || (NULL == q) || (NULL == g))
    {
        return DH_POINTER_NULL;
    }
    else
    {
        ;
    }

    p_len = get_byte_len(dh_para->p_bits);
    q_len = get_byte_len(dh_para->q_bits);
    g_len = get_byte_len(dh_para->g_bits);

    if (NULL != p_h)
    {
        if (NULL == dh_para->p_h)
        {
            return DH_POINTER_NULL;
        }
        else
        {
            reverse_byte_array(p_h, (unsigned char *)dh_para->p_h, p_len);
        }
    }
    else
    {
        ;
    }

    if (NULL != p_n0)
    {
        if (NULL == dh_para->p_n0)
        {
            return DH_POINTER_NULL;
        }
        else
        {
            reverse_byte_array(p_n0, (unsigned char *)dh_para->p_n0, 1 << 2);
        }
    }
    else
    {
        ;
    }

    reverse_byte_array(p, (unsigned char *)dh_para->p, p_len);
    reverse_byte_array(q, (unsigned char *)dh_para->q, q_len);
    reverse_byte_array(g, (unsigned char *)dh_para->g, g_len);

    // p and q can not be even.
    if ((0u == (dh_para->p[0] & 1u)) || (0u == (dh_para->q[0] & 1u)))
    {
        return DH_INVALID_INPUT;
    }
    else
    {
        ;
    }

    // to support p == dh_para->p
    if (0u != (p_len & 3u))
    {
        memset_(((unsigned char *)dh_para->p) + p_len, 0, 4u - (p_len & 3u));
    }
    else
    {
        ;
    }

    // to support q == dh_para->q
    if (0u != (q_len & 3u))
    {
        memset_(((unsigned char *)dh_para->q) + q_len, 0, 4u - (q_len & 3u));
    }
    else
    {
        ;
    }

    // to support g == dh_para->g
    if (0u != (g_len & 3u))
    {
        memset_(((unsigned char *)dh_para->g) + g_len, 0, 4u - (g_len & 3u));
    }
    else
    {
        ;
    }

    return DH_SUCCESS;
}

/**
 * @brief           DH check public key, it must be in [2, p-2], and pubkey^q = 1 mod p
 * @param[in]       dh_para              - Pointer to the DH_PARA struct.
 * @param[in]       p_minus_1            - p-1.
 * @param[in]       pubkey               - Public key.
 * @return          0: success    other: error
 * @note
 *        1. Please call dh_param_value_init() before calling this function.
 *        2. The input p_minus_1 and pubkey both occupy (dh_para->p_bits+31)/32 words;
 */
unsigned int dh_check_public_key(const dh_para_t *dh_para, const unsigned int *p_minus_1, const unsigned int *pubkey)
{
    // unsigned int tmp[DH_MAX_WORD_LEN];
    unsigned int *tmp;
    unsigned int step_bytes, p_wlen, q_wlen;
    unsigned int ret;

    if ((NULL == dh_para) || (NULL == p_minus_1) || (NULL == pubkey))
    {
        return DH_POINTER_NULL;
    }
    else
    {
        ;
    }

    p_wlen = get_word_len(dh_para->p_bits);
    q_wlen = get_word_len(dh_para->q_bits);

    // make sure pubkey is in [2, p-2]
    if (get_valid_bits(pubkey, p_wlen) <= 1)
    {
        return DH_INVALID_INPUT;
    }
    else if (uint32_big_num_cmp(pubkey, p_wlen, p_minus_1, p_wlen) >= 0)
    {
        return DH_INVALID_INPUT;
    }
    else
    {
        ;
    }

    if ((NULL == dh_para->p_h) || (NULL == dh_para->p_n0))
    {
        ret = pke_pre_calc_mont_for_modexp(dh_para->p, dh_para->p_bits, NULL, NULL);
    }
    else
    {
        ret = pke_load_modulus_and_pre_monts(dh_para->p, dh_para->p_h, dh_para->p_n0, dh_para->p_bits);
    }
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    step_bytes = pke_get_operand_bytes();
    tmp = (unsigned int *)(rPKE_A(0u, step_bytes));
    ret = pke_modexp((unsigned int *)(rPKE_B(3u, step_bytes)), dh_para->q, pubkey, tmp, p_wlen, q_wlen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else if (1u != bigint_check_1(tmp, p_wlen))
    {
        return DH_INVALID_INPUT;
    }
    else
    {
    }

    return DH_SUCCESS;
}

/**
 * @brief          DH generate public key from private key.
 * @param[in]    dh_para      - DH_PARA struct pointer.
 * @param[in]   prikey       - private key.
 * @param[in]   pubkey       - public key.
 * @return         0:success     other:error
 * @note
 *      1. please call dh_param_value_init() before calling this function.
 *      2. the input prikey occupies (dh_para->q_bits+7)/8 bytes;
           the output pubkey occupies (dh_para->p_bits+7)/8 bytes;
 */
unsigned int dh_generate_pubkey_from_prikey(const dh_para_t *dh_para, const unsigned char *prikey, unsigned char *pubkey)
{
    unsigned int tmp[DH_MAX_WORD_LEN];
    unsigned int x[DH_MAX_WORD_LEN];
    // unsigned int y[DH_MAX_WORD_LEN];
    unsigned int *g, *y;
    unsigned int p_wlen, q_wlen, g_wlen;
    unsigned int p_len, q_len;
    unsigned int tmp_len, ret;

    if ((NULL == dh_para) || (NULL == prikey) || (NULL == pubkey))
    {
        return DH_POINTER_NULL;
    }
    else
    {
        ;
    }

    p_wlen = get_word_len(dh_para->p_bits);
    q_wlen = get_word_len(dh_para->q_bits);
    g_wlen = get_word_len(dh_para->g_bits);
    p_len = get_byte_len(dh_para->p_bits);
    q_len = get_byte_len(dh_para->q_bits);

    // get tmp = q-1
    uint32_copy(tmp, dh_para->q, q_wlen);
    tmp[0u] -= 1u;

    x[q_wlen - 1u] = 0u;
    reverse_byte_array(prikey, (unsigned char *)x, q_len);

    // x should be in [2, q-2]
    tmp_len = get_valid_bits(x, q_wlen);
    if (0u == tmp_len)
    {
        return DH_ZERO_ALL;
    }
    else if (1u == tmp_len)
    {
        return DH_VALUE_ONE;
    }
    else if (uint32_big_num_cmp(x, q_wlen, tmp, q_wlen) >= 0)
    {
        return DH_INTEGER_TOO_BIG;
    }
    else
    {
        ;
    }

    if ((NULL == dh_para->p_h) || (NULL == dh_para->p_n0))
    {
        ret = pke_pre_calc_mont_for_modexp(dh_para->p, dh_para->p_bits, NULL, NULL);
    }
    else
    {
        ret = pke_load_modulus_and_pre_monts(dh_para->p, dh_para->p_h, dh_para->p_n0, dh_para->p_bits);
    }
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    tmp_len = pke_get_operand_bytes();
    y = (unsigned int *)(rPKE_A(0u, tmp_len));

    if (g_wlen < p_wlen)
    {
        g = (unsigned int *)(rPKE_B(0u, tmp_len));
        uint32_copy(g, dh_para->g, g_wlen);
        uint32_clear(g + g_wlen, p_wlen - g_wlen);
    }
    else
    {
        g = dh_para->g;
    }

    ret = pke_modexp((unsigned int *)(rPKE_B(3u, tmp_len)), x, g, y, p_wlen, q_wlen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    reverse_byte_array((unsigned char *)y, (unsigned char *)pubkey, p_len);

    return DH_SUCCESS;
}

/**
 * @brief          DH generate key pair.
 * @param[in]    dh_para      - DH_PARA struct pointer.
 * @param[in]   prikey       - private key.
 * @param[in]   pubkey       - public key.
 * @return         0:success     other:error
 * @note
 *      1. please call dh_param_value_init() before calling this function.
 *      2. the input prikey occupies (dh_para->q_bits+7)/8 bytes;
           the output pubkey occupies (dh_para->p_bits+7)/8 bytes;
 */
unsigned int dh_generate_key(const dh_para_t *dh_para, unsigned char *prikey, unsigned char *pubkey)
{
    unsigned int q_len;
    unsigned int tmpBitLen, ret;

    if ((NULL == dh_para) || (NULL == prikey) || (NULL == pubkey))
    {
        return DH_POINTER_NULL;
    }
    else
    {
        ;
    }

    q_len = get_byte_len(dh_para->q_bits);

    do
    {
        ret = get_rand((unsigned char *)prikey, q_len);
        if (TRNG_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            ;
        }

        // make sure prikey has the same bit length as q
        tmpBitLen = (dh_para->q_bits) & 7u;
        if (0u != tmpBitLen)
        {
            prikey[0u] &= (1u << (tmpBitLen)) - 1u;
        }
        else
        {
            ;
        }

        ret = dh_generate_pubkey_from_prikey(dh_para, prikey, pubkey);

        // prikey should be in [2, q-2]
    } while ((DH_ZERO_ALL == ret) || (DH_VALUE_ONE == ret) || (DH_INTEGER_TOO_BIG == ret));

    return ret;
}

#endif
