/*! @file hash_kdf.c */
#include "lib/include/hash/hash_kdf.h"
#include "lib/include/crypto_common/utility.h"

#ifdef SUPPORT_PBKDF2

#ifdef PBKDF2_HIGH_SPEED
/**
 * @brief           pbkdf2 backup hmac ctx
 * @param[out]      ctx_bak              - hmac ctx
 * @param[in]       ctx                  - hmac ctx to be backup
 * @return          HASH_SUCCESS(success), other(error)
 */
void pbkdf2_hmac_backup(hmac_ctx_t *ctx_bak, hmac_ctx_t *ctx)
{
    memcpy_((unsigned char *)ctx_bak, (unsigned char *)ctx, sizeof(hmac_ctx_t));

#ifndef CONFIG_HASH_SUPPORT_MUL_THREAD
    hash_get_iterator((unsigned char *)(ctx_bak->hash_ctx->hash_buffer), CAST2UINT32(ctx_bak->hash_ctx->iterator_word_len));
#endif
}
#endif

#ifdef PBKDF2_HIGH_SPEED
/**
 * @brief           pbkdf2 recover hmac ctx
 * @param[out]      ctx                  - hmac ctx to be recover
 * @param[in]       ctx_bak              - hmac ctx
 * @return          HASH_SUCCESS(success), other(error)
 */
void pbkdf2_hmac_recover(hmac_ctx_t *ctx, hmac_ctx_t *ctx_bak)
{
    memcpy_((unsigned char *)ctx, (unsigned char *)ctx_bak, sizeof(hmac_ctx_t));

#ifndef CONFIG_HASH_SUPPORT_MUL_THREAD
    hash_set_iterator((unsigned int *)(ctx_bak->hash_ctx->hash_buffer), CAST2UINT32(ctx_bak->hash_ctx->iterator_word_len));
#endif
}
#endif

/**
 * @brief           pbkdf2 function(using hmac as PRF)
 * @param[in]       alg                  - specific hash algorithm
 * @param[in]       pwd                  - password, as the key of hmac
 * @param[in]       sp_key_idx           - index of secure port key(password)
 * @param[in]       pwd_bytes            - byte length of password, it could be 0
 * @param[in]       salt                 - salt
 * @param[in]       salt_bytes           - byte length of salt, it could be 0
 * @param[in]       iter                 - iteration times
 * @param[out]      out                  - derived key
 * @param[in]       out_bytes            - byte length of derived key
 * @return          HASH_SUCCESS(success), other(error)
 */
unsigned int pbkdf2_hmac(hash_alg_e alg, const unsigned char *pwd, unsigned short sp_key_idx, unsigned int pwd_bytes, const unsigned char *salt, unsigned int salt_bytes,
                         unsigned int iter, unsigned char *out, unsigned int out_bytes)
{
    unsigned int i;
    signed int j;
    unsigned int digest_words, digest_bytes, tmp_bytes;
    unsigned char counter[4] = {0, 0, 0, 1};
    unsigned int tmp[HASH_DIGEST_MAX_WORD_LEN];
    unsigned int mac[HASH_DIGEST_MAX_WORD_LEN];
    unsigned int ret;

    hmac_ctx_t ctx[1];

#ifdef PBKDF2_HIGH_SPEED
    hmac_ctx_t ctx_bak[1];
#endif

    if (HASH_SUCCESS != check_hash_alg(alg))
    {
        return HASH_INPUT_INVALID;
    }
    else if (NULL == out)
    {
        return HASH_BUFFER_NULL;
    }
    else
    {
        // handle other;
    }

    if (NULL == pwd)
    {
#ifndef HMAC_SECURE_PORT_FUNCTION
        return HASH_BUFFER_NULL;
#else
        pwd_bytes = 0;
#endif
    }
    else
    {
        ;
    }

    if (NULL == salt)
    {
        salt_bytes = 0;
    }
    else
    {
        ;
    }

    digest_words = hash_get_digest_word_len(alg);
    digest_bytes = 4U * digest_words;

#ifdef PBKDF2_HIGH_SPEED
    ret = hmac_init(ctx, alg, pwd, sp_key_idx, pwd_bytes);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    pbkdf2_hmac_backup(ctx_bak, ctx);
#endif

    while (0U != out_bytes)
    {
        if (out_bytes > digest_bytes)
        {
            tmp_bytes = digest_bytes;
        }
        else
        {
            tmp_bytes = out_bytes;
        }

        // get U1
#ifdef PBKDF2_HIGH_SPEED
        pbkdf2_hmac_recover(ctx, ctx_bak);
#else
        ret = hmac_init(ctx, alg, pwd, sp_key_idx, pwd_bytes);
        if (HASH_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }
#endif
        ret = hash_update(ctx->hash_ctx, salt, salt_bytes); // hmac_update(ctx, salt, salt_bytes);
        if (HASH_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }

        ret = hash_update(ctx->hash_ctx, counter, 4); // hmac_update(ctx, counter, 4);
        if (HASH_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }

        ret = hmac_final(ctx, (unsigned char *)mac);
        if (HASH_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }

        uint32_copy(tmp, mac, digest_words);

        for (i = 1U; i < iter; i++)
        {
#ifdef PBKDF2_HIGH_SPEED
            pbkdf2_hmac_recover(ctx, ctx_bak);
#else
            ret = hmac_init(ctx, alg, pwd, sp_key_idx, pwd_bytes);
            if (HASH_SUCCESS != ret)
            {
                goto END;
            }
            else
            {
                ;
            }
#endif
            ret = hash_update(ctx->hash_ctx, (unsigned char *)tmp, digest_bytes);
            if (HASH_SUCCESS != ret)
            {
                goto END;
            }
            else
            {
                ;
            }

            ret = hmac_final(ctx, (unsigned char *)tmp);
            if (HASH_SUCCESS != ret)
            {
                goto END;
            }
            else
            {
                ;
            }

            for (j = 0; j < (signed int)digest_words; j++)
            {
                mac[j] ^= tmp[j];
            }
        }

        // add 1
        (void)uint8_big_num_big_endian_add_little(counter, 4, 1, 1);

        memcpy_((unsigned char *)out, (unsigned char *)mac, tmp_bytes);

        out = &(out[tmp_bytes]);
        out_bytes -= tmp_bytes;
    }

    ret = HASH_SUCCESS;

END:

    return ret;
}
#endif

#ifdef SUPPORT_ANSI_X9_63_KDF
#ifdef SUPPORT_HASH_NODE
/**
 * @brief           out = ansi_x9_63_kdf(msg1||msg2||...||counter||... , out_bytes),        ---- if in is NULL
 *                  out = ansi_x9_63_kdf(msg1||msg2||...||counter||... , out_bytes) XOR in, ---- if in is not NULL
 * @param[in]       alg                  - specific hash algorithm
 * @param[in]       node                 - hash_node_t struct pointer
 * @param[in]       node_num             - number of hash_node_t, or number of message pieces
 * @param[in]       counter              - counter of 4 bytes in node array
 * @param[in]       in                   - input message, if no input message, please set this para as NULL
 * @param[out]      out                  - if in is NULL, out = kdf(...), otherwise out = kdf(...) XOR in.
 * @param[in]       out_bytes            - byte length of out, if in is not NULL, this is also byte length of in
 * @param[in]       check_whether_zero   - 0(not check whether kdf output is zero or not),
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure alg is valid
 *        2. node[counter_idx] holds counter of 4 bytes.
 *        3. if in is NULL, out = kdf(...), otherwise out = kdf(...) XOR in.
 *        4. for ansi x9.63, initial counter is 1; for PKCS#1 RSA MGF, initial counter is 0
 */
unsigned int ansi_x9_63_kdf_node_with_xor_in(hash_alg_e alg, const hash_node_t *node, unsigned int node_num, unsigned char *counter, const unsigned char *in, unsigned char *out,
                                             unsigned int out_bytes, unsigned int check_whether_zero)
{
    unsigned char digest[HASH_DIGEST_MAX_WORD_LEN << 2];
    unsigned int digest_bytes = (unsigned int)hash_get_digest_word_len(alg) << 2;
    unsigned int remainder_bytes;
    unsigned int round;
    unsigned int i, ret = HASH_SUCCESS;
    unsigned char zero_check = 0;

    if (0U != digest_bytes)
    {
        round = out_bytes / digest_bytes;
    }
    else
    {
        return HASH_INPUT_INVALID;
    }

    if (0U == out_bytes)
    {
        return HASH_SUCCESS;
    }
    else
    {
    }

    for (i = 0U; i < round; i++)
    {
        if (NULL == in)
        {
            ret = hash_node_steps(alg, node, node_num, &out[i * digest_bytes]);
            if (HASH_SUCCESS != ret)
            {
                return ret;
            }
            else
            {
            }

            if (0U != check_whether_zero)
            {
                if (0U != uint8_BigNum_Check_Zero(digest, digest_bytes))
                {
                    zero_check |= (unsigned char)0;
                }
                else
                {
                    zero_check |= (unsigned char)1;
                }
            }
            else
            {
            }
        }
        else
        {
            ret = hash_node_steps(alg, node, node_num, digest);
            if (HASH_SUCCESS != ret)
            {
                return ret;
            }
            else
            {
            }

            if (0U != check_whether_zero)
            {
                if (0U != uint8_BigNum_Check_Zero(digest, digest_bytes))
                {
                    zero_check |= (unsigned char)0;
                }
                else
                {
                    zero_check |= (unsigned char)1;
                }
            }
            else
            {
            }

            uint8_XOR(&in[i * digest_bytes], digest, &out[i * digest_bytes], digest_bytes);
        }

        ret = uint8_big_num_big_endian_add_little(counter, 4, 1, 1);
        if (0U != ret)
        {
            return HASH_ERROR;
        }
        else
        {
        }
    }

    remainder_bytes = out_bytes - round * digest_bytes;
    if (0U != remainder_bytes)
    {
        ret = hash_node_steps(alg, node, node_num, digest);
        if (HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
        }

        if (0U != check_whether_zero)
        {
            if (0U != uint8_BigNum_Check_Zero(digest, remainder_bytes))
            {
                zero_check |= (unsigned char)0;
            }
            else
            {
                zero_check |= (unsigned char)1;
            }
        }
        else
        {
        }

        if (NULL == in)
        {
            memcpy_(&out[i * digest_bytes], digest, remainder_bytes);
        }
        else
        {
            uint8_XOR(&in[i * digest_bytes], digest, &out[i * digest_bytes], remainder_bytes);
        }

        ret = uint8_big_num_big_endian_add_little(counter, 4, 1, 1);
        if (0U != ret)
        {
            return HASH_ERROR;
        }
        else
        {
        }
    }

    if ((0U != check_whether_zero) && (0U == zero_check))
    {
        memset_(out, 0, out_bytes);
        return HASH_OUTPUT_ZERO_ALL;
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           key = ansi_x9_63_kdf(msg1||msg2||...||counter||..., key_bytes).
 * @param[in]       alg                  - specific hash algorithm
 * @param[in]       node                 - hash_node_t struct pointer
 * @param[in]       node_num             - number of hash_node_t, or number of message pieces
 * @param[in]       counter              - counter of 4 bytes in node array
 * @param[out]      key                  - key
 * @param[in]       key_bytes            - byte length of output key
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure alg is valid
 *        2. node[counter_idx] holds counter of 4 bytes.
 *        3. here key_bytes must be a multiple of alg digest byte length
 */
unsigned int ansi_x9_63_kdf_internal(hash_alg_e alg, const hash_node_t *node, unsigned int node_num, unsigned char *counter, unsigned char *key, unsigned int key_bytes)
{
    unsigned int digest_bytes = (unsigned int)hash_get_digest_word_len(alg) << 2;
    unsigned int round;
    unsigned int i, ret = HASH_SUCCESS;

    if (0U != digest_bytes)
    {
        round = key_bytes / digest_bytes;
    }
    else
    {
        return HASH_INPUT_INVALID;
    }

    if (0U == round)
    {
        return HASH_SUCCESS;
    }
    else
    {
    }

    for (i = 0U; i < round; i++)
    {
        ret = hash_node_steps(alg, node, node_num, &(key[(i * digest_bytes)]));
        if (HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
        }

        (void)uint8_big_num_big_endian_add_little(counter, 4, 1, 1);
    }

    return ret;
}

/**
 * @brief           k1||k2 = ansi_x9_63_kdf(msg1||msg2||...||counter||... , k1_bytes + k2_bytes).
 * @param[in]       alg                  - specific hash algorithm
 * @param[in]       node                 - hash_node_t struct pointer
 * @param[in]       node_num             - number of hash_node_t, or number of message pieces
 * @param[in]       counter              - counter of 4 bytes in node array
 * @param[out]      k1                   - k1 part
 * @param[in]       k1_bytes             - byte length of k1
 * @param[out]      k2                   - k2 part
 * @param[in]       k2_bytes             - byte length of k2
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure alg is valid
 *        2. node[counter_idx] holds counter of 4 bytes.
 *        3. k1 can not be NULL, but k2 can
 */
unsigned int ansi_x9_63_kdf_node(hash_alg_e alg, const hash_node_t *node, unsigned int node_num, unsigned char *counter, unsigned char *k1, unsigned int k1_bytes,
                                 unsigned char *k2, unsigned int k2_bytes)
{
    unsigned char digest[HASH_DIGEST_MAX_WORD_LEN << 2];
    unsigned int digest_bytes = (unsigned int)hash_get_digest_word_len(alg) << 2;
    unsigned int blocks_bytes, remainder_bytes, fill_bytes;
    unsigned int ret;

    if (0U != digest_bytes)
    {
        blocks_bytes = (k1_bytes / digest_bytes) * digest_bytes;
    }
    else
    {
        return HASH_INPUT_INVALID;
    }

    ret = ansi_x9_63_kdf_internal(alg, node, node_num, counter, k1, blocks_bytes);
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    remainder_bytes = k1_bytes - blocks_bytes;
    if (0U != remainder_bytes)
    {
        ret = ansi_x9_63_kdf_internal(alg, node, node_num, counter, digest, digest_bytes);
        if (HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
        }

        memcpy_(&(k1[blocks_bytes]), digest, remainder_bytes);

        if (NULL != k2)
        {
            fill_bytes = digest_bytes - remainder_bytes;
            if (k2_bytes <= fill_bytes)
            {
                memcpy_(k2, &(digest[remainder_bytes]), k2_bytes);

                return HASH_SUCCESS;
            }
            else
            {
                memcpy_(k2, &(digest[remainder_bytes]), fill_bytes);
                k2 = &(k2[fill_bytes]);
                k2_bytes -= fill_bytes;
            }
        }
        else
        {
        }
    }
    else
    {
    }

    if ((NULL != k2) && (0U != k2_bytes))
    {
        blocks_bytes = (k2_bytes / digest_bytes) * digest_bytes;
        ret = ansi_x9_63_kdf_internal(alg, node, node_num, counter, k2, blocks_bytes);
        if (HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
        }

        remainder_bytes = k2_bytes - blocks_bytes;
        if (0U != remainder_bytes)
        {
            ret = ansi_x9_63_kdf_internal(alg, node, node_num, counter, digest, digest_bytes);
            if (HASH_SUCCESS != ret)
            {
                return ret;
            }
            else
            {
            }

            memcpy_(&(k2[blocks_bytes]), digest, remainder_bytes);
        }
        else
        {
        }
    }

    return ret;
}

/**
 * @brief           k1||k2 = ansi_x9_63_kdf(Z||counter||shared_info , k1_bytes + k2_bytes).
 * @param[in]       alg                  - specific hash algorithm
 * @param[in]       Z                    - byte string
 * @param[in]       Z_bytes              - byte length of Z
 * @param[in]       shared_info          - shared info
 * @param[in]       shared_info_bytes    - byte length of shared info
 * @param[out]      k1                   - k1 part
 * @param[in]       k1_bytes             - byte length of k1
 * @param[out]      k2                   - k2 part
 * @param[in]       k2_bytes             - byte length of k2
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure alg is valid
 *        2. k1 can not be NULL, but k2 can
 */
unsigned int ansi_x9_63_kdf(hash_alg_e alg, const unsigned char *Z, unsigned int Z_bytes, const unsigned char *shared_info, unsigned int shared_info_bytes, unsigned char *k1,
                            unsigned int k1_bytes, unsigned char *k2, unsigned int k2_bytes)
{
    unsigned char counter[4] = {0x00, 0x00, 0x00, 0x01}; // init count = 1
    hash_node_t node[3];

    node[0].msg_addr = Z;
    node[0].msg_len = Z_bytes;
    node[1].msg_addr = counter;
    node[1].msg_len = 4U;
    node[2].msg_addr = shared_info;
    node[2].msg_len = shared_info_bytes;

    return ansi_x9_63_kdf_node(alg, node, 3, counter, k1, k1_bytes, k2, k2_bytes);
}
#endif
#endif
