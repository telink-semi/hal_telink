/********************************************************************************************************
 * @file    hash_kdf.c
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
#include "lib/include/hash/hash_kdf.h"
#include "lib/include/crypto_common/utility.h"


#ifdef SUPPORT_PBKDF2


    #ifdef PBKDF2_HIGH_SPEED
/**
 * @brief       dma md5 digest calculate
 * @param[out]  ctx_bak            - hmac ctx.
 * @param[in]   ctx                - hmac ctx to be backup.
 * @return      0:success     other:error
 */
void pbkdf2_hmac_backup(HMAC_CTX *ctx_bak, HMAC_CTX *ctx)
{
    memcpy_((unsigned char *)ctx_bak, (unsigned char *)ctx, sizeof(HMAC_CTX));

        #ifndef CONFIG_HASH_SUPPORT_MUL_THREAD
    /**
 * @brief       get current HASH iterator value.
 * @param[in]   iterator  - current hash iterator.
 * @param[in]   hash_iterator_words iterator word length.
 * @return      none
 */
    hash_get_iterator((unsigned char *)(ctx_bak->hash_ctx->hash_buffer), CAST2UINT32(ctx_bak->hash_ctx->iterator_word_len));
        #endif
}
    #endif


    #ifdef PBKDF2_HIGH_SPEED
/* function: pbkdf2 recover hmac ctx
 * parameters:
 *     ctx ------------------------ output, hmac ctx to be recover
 *     ctx_bak -------------------- input, hmac ctx
 * return: HASH_SUCCESS(success), other(error)
 * caution:
 *     1.
 */
void pbkdf2_hmac_recover(HMAC_CTX *ctx, HMAC_CTX *ctx_bak)
{
    memcpy_((unsigned char *)ctx, (unsigned char *)ctx_bak, sizeof(HMAC_CTX));

        #ifndef CONFIG_HASH_SUPPORT_MUL_THREAD
    hash_set_iterator((unsigned int *)(ctx_bak->hash_ctx->hash_buffer), CAST2UINT32(ctx_bak->hash_ctx->iterator_word_len));
        #endif
}
    #endif


/* function: pbkdf2 function(using hmac as PRF)
 * parameters:
 *     hash_alg ------------------- input, specific hash algorithm
 *     pwd ------------------------ input, password, as the key of hmac
 *     sp_key_idx ----------------- input, index of secure port key(password)
 *     pwd_bytes ------------------ input, byte length of password, it could be 0
 *     salt ----------------------- input, salt
 *     salt_bytes ----------------- input, byte length of salt, it could be 0
 *     iter ----------------------- input, iteration times
 *     out ------------------------ output, derived key
 *     out_bytes ------------------ input, byte length of derived key
 * return: HASH_SUCCESS(success), other(error)
 * caution:
 *     1.
 */
unsigned int pbkdf2_hmac(HASH_ALG hash_alg, unsigned char *pwd, unsigned short sp_key_idx, unsigned int pwd_bytes, unsigned char *salt, unsigned int salt_bytes, unsigned int iter, unsigned char *out, unsigned int out_bytes)
{
    unsigned int  i;
    int32_t       j;
    unsigned int  digest_words, digest_bytes, tmp_bytes;
    unsigned char counter[4] = {0, 0, 0, 1};
    unsigned int  tmp[HASH_DIGEST_MAX_WORD_LEN];
    unsigned int  mac[HASH_DIGEST_MAX_WORD_LEN];
    unsigned int  ret;

    HMAC_CTX ctx[1];

    #ifdef PBKDF2_HIGH_SPEED
    HMAC_CTX ctx_bak[1];
    #endif

    if (HASH_SUCCESS != check_hash_alg(hash_alg)) {
        return HASH_INPUT_INVALID;
    } else if (NULL == out) {
        return HASH_BUFFER_NULL;
    } else {
        //handle other;
    }

    if (NULL == pwd) {
    #ifndef HMAC_SECURE_PORT_FUNCTION
        return HASH_BUFFER_NULL;
    #else
        pwd_bytes = 0;
    #endif
    } else {
        ;
    }

    if (NULL == salt) {
        salt_bytes = 0;
    } else {
        ;
    }

    digest_words = hash_get_digest_word_len(hash_alg);
    digest_bytes = 4U * digest_words;

    #ifdef PBKDF2_HIGH_SPEED
    ret = hmac_init(ctx, hash_alg, pwd, sp_key_idx, pwd_bytes);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    pbkdf2_hmac_backup(ctx_bak, ctx);
    #endif

    while (0U != out_bytes) {
        if (out_bytes > digest_bytes) {
            tmp_bytes = digest_bytes;
        } else {
            tmp_bytes = out_bytes;
        }

        //get U1
    #ifdef PBKDF2_HIGH_SPEED
        pbkdf2_hmac_recover(ctx, ctx_bak);
    #else
        ret = hmac_init(ctx, hash_alg, pwd, sp_key_idx, pwd_bytes);
        if (HASH_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }
    #endif
        ret = hash_update(ctx->hash_ctx, salt, salt_bytes); //hmac_update(ctx, salt, salt_bytes);
        if (HASH_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        ret = hash_update(ctx->hash_ctx, counter, 4); //hmac_update(ctx, counter, 4);
        if (HASH_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        ret = hmac_final(ctx, (unsigned char *)mac);
        if (HASH_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        uint32_copy(tmp, mac, digest_words);

        for (i = 1U; i < iter; i++) {
    #ifdef PBKDF2_HIGH_SPEED
            pbkdf2_hmac_recover(ctx, ctx_bak);
    #else
            ret = hmac_init(ctx, hash_alg, pwd, sp_key_idx, pwd_bytes);
            if (HASH_SUCCESS != ret) {
                goto END;
            } else {
                ;
            }
    #endif
            ret = hash_update(ctx->hash_ctx, (unsigned char *)tmp, digest_bytes);
            if (HASH_SUCCESS != ret) {
                goto END;
            } else {
                ;
            }

            ret = hmac_final(ctx, (unsigned char *)tmp);
            if (HASH_SUCCESS != ret) {
                goto END;
            } else {
                ;
            }

            for (j = 0; j < (int32_t)digest_words; j++) {
                mac[j] ^= tmp[j];
            }
        }

        //add 1
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
/* function: out = ansi_x9_63_kdf(msg1||msg2||...||counter||... , out_bytes),        ---- if in is NULL
 *           out = ansi_x9_63_kdf(msg1||msg2||...||counter||... , out_bytes) XOR in, ---- if in is not NULL
 * parameters:
 *     hash_alg ------------------- input, specific hash algorithm
 *     hash_node ------------------ input, HASH_NODE struct pointer
 *     node_num ------------------- input, number of HASH_NODE, or number of message pieces
 *     counter_idx ---------------- input, index of the counter of 4 bytes in hash_node array
 *     in ------------------------- input, input message, if no input message, please set this para as NULL
 *     out ------------------------ output, if in is NULL, out = kdf(...), otherwise out = kdf(...) XOR in.
 *     out_bytes ------------------ input, byte length of out, if in is not NULL, this is also byte length of in
 *     check_whether_zero --------- input, 0(not check whether kdf output is zero or not),
 *                                         other(check whether kdf output is zero or not).
 * return: HASH_SUCCESS(success), other(error)
 * caution:
 *     1. please make sure hash_alg is valid
 *     2. hash_node[counter_idx] holds counter of 4 bytes.
 *     3. if in is NULL, out = kdf(...), otherwise out = kdf(...) XOR in.
 *     4. for ansi x9.63, initial counter is 1; for PKCS#1 RSA MGF, initial counter is 0.
 */
unsigned int ansi_x9_63_kdf_node_with_xor_in(HASH_ALG hash_alg, HASH_NODE *hash_node, unsigned int node_num, unsigned char *counter, unsigned char *in, unsigned char *out, unsigned int out_bytes, unsigned int check_whether_zero)
{
    unsigned char digest[HASH_DIGEST_MAX_WORD_LEN << 2];
    unsigned int  digest_bytes = (unsigned int)hash_get_digest_word_len(hash_alg) << 2;
    unsigned int  remainder_bytes;
    unsigned int  round;
    unsigned int  i, ret = HASH_SUCCESS;
    unsigned char zero_check = 0;

    if (0U != digest_bytes) {
        round = out_bytes / digest_bytes;
    } else {
        return HASH_INPUT_INVALID;
    }

    if (0U == out_bytes) {
        return HASH_SUCCESS;
    } else {
        ;
    }

    for (i = 0U; i < round; i++) {
        if (NULL == in) {
            ret = hash_node_steps(hash_alg, hash_node, node_num, &out[i * digest_bytes]);
            if (HASH_SUCCESS != ret) {
                return ret;
            } else {
                ;
            }

            if (0U != check_whether_zero) {
                if (0U != uint8_BigNum_Check_Zero(digest, digest_bytes)) {
                    zero_check |= (unsigned char)0;
                } else {
                    zero_check |= (unsigned char)1;
                }
            } else {
                ;
            }
        } else {
            ret = hash_node_steps(hash_alg, hash_node, node_num, digest);
            if (HASH_SUCCESS != ret) {
                return ret;
            } else {
                ;
            }

            if (0U != check_whether_zero) {
                if (0U != uint8_BigNum_Check_Zero(digest, digest_bytes)) {
                    zero_check |= (unsigned char)0;
                } else {
                    zero_check |= (unsigned char)1;
                }
            } else {
                ;
            }

            uint8_XOR(&in[i * digest_bytes], digest, &out[i * digest_bytes], digest_bytes);
        }

        ret = uint8_big_num_big_endian_add_little(counter, 4, 1, 1);
        if (0U != ret) {
            return HASH_ERROR;
        } else {
            ;
        }
    }

    remainder_bytes = out_bytes - round * digest_bytes;
    if (0U != remainder_bytes) {
        ret = hash_node_steps(hash_alg, hash_node, node_num, digest);
        if (HASH_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }

        if (0U != check_whether_zero) {
            if (0U != uint8_BigNum_Check_Zero(digest, remainder_bytes)) {
                zero_check |= (unsigned char)0;
            } else {
                zero_check |= (unsigned char)1;
            }
        } else {
            ;
        }

        if (NULL == in) {
            memcpy_(&out[i * digest_bytes], digest, remainder_bytes);
        } else {
            uint8_XOR(&in[i * digest_bytes], digest, &out[i * digest_bytes], remainder_bytes);
        }

        ret = uint8_big_num_big_endian_add_little(counter, 4, 1, 1);
        if (0U != ret) {
            return HASH_ERROR;
        } else {
            ;
        }
    }

    if ((0U != check_whether_zero) && (0U == zero_check)) {
        memset_(out, 0, out_bytes);
        return HASH_OUTPUT_ZERO_ALL;
    } else {
        ;
    }

    return ret;
}

/* function: key = ansi_x9_63_kdf(msg1||msg2||...||counter||..., key_bytes).
 * parameters:
 *     hash_alg ------------------- input, specific hash algorithm
 *     hash_node ------------------ input, HASH_NODE struct pointer
 *     node_num ------------------- input, number of HASH_NODE, or number of message pieces
 *     counter_idx ---------------- input, index of the counter of 4 bytes in hash_node array
 *     key ------------------------ output, key
 *     key_bytes ------------------ input, byte length of output key
 * return: HASH_SUCCESS(success), other(error)
 * caution:
 *     1. please make sure hash_alg is valid
 *     2. hash_node[counter_idx] holds counter of 4 bytes.
 *     3. here key_bytes must be a multiple of hash_alg digest byte length.
 */
unsigned int ansi_x9_63_kdf_internal(HASH_ALG hash_alg, HASH_NODE *hash_node, unsigned int node_num, unsigned char *counter, unsigned char *key, unsigned int key_bytes)
{
    unsigned int digest_bytes = (unsigned int)hash_get_digest_word_len(hash_alg) << 2;
    unsigned int round;
    unsigned int i, ret = HASH_SUCCESS;

    if (0U != digest_bytes) {
        round = key_bytes / digest_bytes;
    } else {
        return HASH_INPUT_INVALID;
    }

    if (0U == round) {
        return HASH_SUCCESS;
    } else {
        ;
    }

    for (i = 0U; i < round; i++) {
        ret = hash_node_steps(hash_alg, hash_node, node_num, &(key[(i * digest_bytes)]));
        if (HASH_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }

        (void)uint8_big_num_big_endian_add_little(counter, 4, 1, 1);
    }

    return ret;
}

/* function: k1||k2 = ansi_x9_63_kdf(msg1||msg2||...||counter||... , k1_bytes + k2_bytes).
 * parameters:
 *     hash_alg ------------------- input, specific hash algorithm
 *     hash_node ------------------ input, HASH_NODE struct pointer
 *     node_num ------------------- input, number of HASH_NODE, or number of message pieces
 *     counter_idx ---------------- input, index of the counter of 4 bytes in hash_node array
 *     k1 ------------------------- output, k1 part
 *     k1_bytes ------------------- input, byte length of k1
 *     k2 ------------------------- output, k2 part
 *     k2_bytes ------------------- input, byte length of k2
 * return: HASH_SUCCESS(success), other(error)
 * caution:
 *     1. please make sure hash_alg is valid
 *     2. hash_node[counter_idx] holds counter of 4 bytes.
 *     3. k1 can not be NULL, but k2 can.
 */
unsigned int ansi_x9_63_kdf_node(HASH_ALG hash_alg, HASH_NODE *hash_node, unsigned int node_num, unsigned char *counter, unsigned char *k1, unsigned int k1_bytes, unsigned char *k2, unsigned int k2_bytes)
{
    unsigned char digest[HASH_DIGEST_MAX_WORD_LEN << 2];
    unsigned int  digest_bytes = (unsigned int)hash_get_digest_word_len(hash_alg) << 2;
    unsigned int  blocks_bytes, remainder_bytes, fill_bytes;
    unsigned int  ret;

    if (0U != digest_bytes) {
        blocks_bytes = (k1_bytes / digest_bytes) * digest_bytes;
    } else {
        return HASH_INPUT_INVALID;
    }

    ret = ansi_x9_63_kdf_internal(hash_alg, hash_node, node_num, counter, k1, blocks_bytes);
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    remainder_bytes = k1_bytes - blocks_bytes;
    if (0U != remainder_bytes) {
        ret = ansi_x9_63_kdf_internal(hash_alg, hash_node, node_num, counter, digest, digest_bytes);
        if (HASH_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }

        memcpy_(&(k1[blocks_bytes]), digest, remainder_bytes);

        if (NULL != k2) {
            fill_bytes = digest_bytes - remainder_bytes;
            if (k2_bytes <= fill_bytes) {
                memcpy_(k2, &(digest[remainder_bytes]), k2_bytes);

                return HASH_SUCCESS;
            } else {
                memcpy_(k2, &(digest[remainder_bytes]), fill_bytes);
                k2 = &(k2[fill_bytes]);
                k2_bytes -= fill_bytes;
            }
        } else {
            ;
        }
    } else {
        ;
    }

    if ((NULL != k2) && (0U != k2_bytes)) {
        blocks_bytes = (k2_bytes / digest_bytes) * digest_bytes;
        ret          = ansi_x9_63_kdf_internal(hash_alg, hash_node, node_num, counter, k2, blocks_bytes);
        if (HASH_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }

        remainder_bytes = k2_bytes - blocks_bytes;
        if (0U != remainder_bytes) {
            ret = ansi_x9_63_kdf_internal(hash_alg, hash_node, node_num, counter, digest, digest_bytes);
            if (HASH_SUCCESS != ret) {
                return ret;
            } else {
                ;
            }

            memcpy_(&(k2[blocks_bytes]), digest, remainder_bytes);
        } else {
            ;
        }
    }

    return ret;
}

/* function: k1||k2 = ansi_x9_63_kdf(Z||counter||shared_info , k1_bytes + k2_bytes).
 * parameters:
 *     hash_alg ------------------- input, specific hash algorithm
 *     Z -------------------------- input, byte string
 *     Z_bytes -------------------- input, byte length of Z
 *     shared_info ---------------- input, shared info
 *     shared_info_bytes ---------- input, byte length of shared info
 *     k1 ------------------------- output, k1 part
 *     k1_bytes ------------------- input, byte length of k1
 *     k2 ------------------------- output, k2 part
 *     k2_bytes ------------------- input, byte length of k2
 * return: HASH_SUCCESS(success), other(error)
 * caution:
 *     1. please make sure hash_alg is valid
 *     2. k1 can not be NULL, but k2 can.
 */
unsigned int ansi_x9_63_kdf(HASH_ALG hash_alg, unsigned char *Z, unsigned int Z_bytes, unsigned char *shared_info, unsigned int shared_info_bytes, unsigned char *k1, unsigned int k1_bytes, unsigned char *k2, unsigned int k2_bytes)
{
    unsigned char  counter_buf[4] = {0x00, 0x00, 0x00, 0x01}; //init count = 1
    unsigned char *counter        = counter_buf;
    HASH_NODE      hash_node[3];

    hash_node[0].msg_addr  = Z;
    hash_node[0].msg_bytes = Z_bytes;
    hash_node[1].msg_addr  = counter;
    hash_node[1].msg_bytes = 4U;
    hash_node[2].msg_addr  = shared_info;
    hash_node[2].msg_bytes = shared_info_bytes;

    return ansi_x9_63_kdf_node(hash_alg, hash_node, 3, counter_buf, k1, k1_bytes, k2, k2_bytes);
}
    #endif
#endif
