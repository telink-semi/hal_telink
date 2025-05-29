/********************************************************************************************************
 * @file    hmac.c
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
#include <string.h>
#include "lib/include/crypto_common/lib_extension.h"
#include "lib/include/crypto_common/utility.h"
#include "lib/include/hash/hash.h"
#include "lib/include/hash/hmac.h"

/**
 * @brief           init HMAC
 * @param[in]       ctx                  - context pointer
 * @param[in]       alg                  - specific hash algorithm
 * @param[in]       key                  - key
 * @param[in]       sp_key_idx           - index of secure port key
 * @param[in]       key_bytes            - byte length of key, it could be 0
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure alg is valid
 */
unsigned int hmac_init(hmac_ctx_t *ctx, hash_alg_e alg, const unsigned char *key, unsigned short sp_key_idx, unsigned int key_bytes)
{
    unsigned int block_byte_len, digest_byte_len;
    unsigned int i, ret;

    (void)sp_key_idx;

    if (NULL == ctx)
    {
        return HASH_BUFFER_NULL;
    }
    else if (HASH_SUCCESS != check_hash_alg(alg))
    {
        return HASH_INPUT_INVALID;
    }
    else if (NULL == key)
    {
#ifdef HMAC_SECURE_PORT_FUNCTION
        // TODO
#else
        return HASH_BUFFER_NULL; // key_bytes = 0;
#endif
    }
    else
    {
        // handle other;
    }

#ifdef HMAC_SECURE_PORT_FUNCTION
    if (NULL != key) // key is from user input
    {
        // hash_hmac_disable_secure_port();
    }
    else // key is from secure port
    {
        // hash_hmac_enable_secure_port(sp_key_idx);
        // hash_hmac_enable_secure_port(sp_key_idx+1);
    }
#endif

    block_byte_len = CAST2UINT32(hash_get_block_word_len(alg)) << 2;
    digest_byte_len = CAST2UINT32(hash_get_digest_word_len(alg)) << 2;

    // get K0
    if (key_bytes <= block_byte_len)
    {
        memcpy_((unsigned char *)(ctx->K0), key, key_bytes);
        memset_(((unsigned char *)(ctx->K0)) + key_bytes, 0, block_byte_len - key_bytes);
    }
    else
    {
        // K0 = hash(key)||000..00
        ret = hash_init(ctx->hash_ctx, alg);
        if (HASH_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
        }

        ret = hash_update(ctx->hash_ctx, key, key_bytes);
        if (HASH_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
        }

        ret = hash_final(ctx->hash_ctx, (unsigned char *)(ctx->K0));
        if (HASH_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
        }

        memset_(((unsigned char *)(ctx->K0)) + digest_byte_len, 0, block_byte_len - digest_byte_len);
    }

    // get K0 ^ ipad
    digest_byte_len = block_byte_len / 4U;
    for (i = 0; i < digest_byte_len; i++)
    {
        ctx->K0[i] ^= HMAC_IPAD;
    }

    ret = hash_init(ctx->hash_ctx, alg);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ret = hash_update(ctx->hash_ctx, (unsigned char *)(ctx->K0), block_byte_len);
    }

END:
    if (HASH_SUCCESS != ret)
    {
        memset_((unsigned char *)ctx, 0, sizeof(hmac_ctx_t));
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           hmac update message
 * @param[in]       ctx                  - hmac_ctx_t context pointer
 * @param[in]       msg                  - message
 * @param[in]       msg_len            - byte length of the input message
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the three parameters are valid, and ctx is initialize
 */
unsigned int hmac_update(hmac_ctx_t *ctx, const unsigned char *msg, unsigned int msg_len)
{
    if (NULL == ctx)
    {
        return HASH_BUFFER_NULL;
    }
    else
    {
        return hash_update(ctx->hash_ctx, msg, msg_len);
    }
}

/**
 * @brief           message update done, get the hmac
 * @param[in]       ctx                  - hmac_ctx_t context pointer
 * @param[out]      mac                  - hmac
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the ctx is valid and initialized
 *        2. please make sure the mac buffer is sufficient
 */
unsigned int hmac_final(hmac_ctx_t *ctx, unsigned char *mac)
{
    hash_alg_e alg;
    unsigned int block_word_len, digest_word_len;
    unsigned int i, ret;

    if (NULL == ctx || NULL == mac)
    {
        return HASH_BUFFER_NULL;
    }
    else
    {
    }

    alg = ctx->hash_ctx->alg;
    digest_word_len = hash_get_digest_word_len(alg);

    // set mac as hash((K0^ipad)||message)
    // caution: here context will be cleaned up
    ret = hash_final(ctx->hash_ctx, mac);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

    // get K0 ^ opad
    block_word_len = hash_get_block_word_len(alg);
    for (i = 0; i < block_word_len; i++)
    {
        ctx->K0[i] ^= HMAC_IPAD_XOR_OPAD;
    }

    ret = hash_init(ctx->hash_ctx, alg);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

    ret = hash_update(ctx->hash_ctx, (unsigned char *)(ctx->K0), ctx->hash_ctx->block_byte_len);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

    ret = hash_update(ctx->hash_ctx, mac, ctx->hash_ctx->digest_byte_len);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

    ret = hash_final(ctx->hash_ctx, mac);

END:
    if (HASH_SUCCESS != ret)
    {
        memset_(mac, 0, digest_word_len << 2);
    }
    else
    {
    }

    memset_((unsigned char *)ctx, 0, sizeof(hmac_ctx_t));

    return ret;
}

/**
 * @brief           input key and whole message, get the hmac
 * @param[in]       alg                  - specific hash algorithm
 * @param[in]       key                  - key
 * @param[in]       sp_key_idx           - index of secure port key
 * @param[in]       key_bytes            - byte length of the key
 * @param[in]       msg                  - message
 * @param[in]       msg_len            - byte length of the input message
 * @param[out]      mac                  - hmac
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the mac buffer is sufficient
 */
unsigned int hmac(hash_alg_e alg, const unsigned char *key, unsigned short sp_key_idx, unsigned int key_bytes, const unsigned char *msg, unsigned int msg_len, unsigned char *mac)
{
    hmac_ctx_t ctx[1];
    unsigned int ret;

    if (NULL == mac)
    {
        return HASH_BUFFER_NULL;
    }
    else
    {
    }

    ret = hmac_init(ctx, alg, key, sp_key_idx, key_bytes);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

    ret = hash_update(ctx->hash_ctx, msg, msg_len);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

    ret = hmac_final(ctx, mac);

END:
    memset_((unsigned char *)ctx, 0, sizeof(hmac_ctx_t));

    return ret;
}

#ifdef SUPPORT_HASH_NODE
/**
 * @brief           input key and whole message, get the hmac(node style)
 * @param[in]       alg                  - specific hash algorithm
 * @param[in]       key                  - key
 * @param[in]       sp_key_idx           - index of secure port key
 * @param[in]       key_bytes            - byte length of the key
 * @param[in]       node                 - message node pointer
 * @param[in]       node_num             - number of hash nodes, i.e. number of message segments.
 * @param[out]      mac                  - hmac
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the mac buffer is sufficient
 *        2. here hmac is not for SHA3.
 *        3. if the whole message consists of some segments, every segment is a node, a node includes
 *           address and byte length
 */
unsigned int hmac_node_steps(hash_alg_e alg, const unsigned char *key, unsigned short sp_key_idx, unsigned int key_bytes, const hash_node_t *node, unsigned int node_num,
                             unsigned char *mac)
{
    hmac_ctx_t ctx[1];
    unsigned int i, ret;

    ret = hmac_init(ctx, alg, key, sp_key_idx, key_bytes);
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    for (i = 0U; i < node_num; i++)
    {
        ret = hmac_update(ctx, node[i].msg_addr, node[i].msg_len);
        if (HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
        }
    }

    return hmac_final(ctx, mac);
}
#endif

#ifdef HASH_DMA_FUNCTION
/**
 * @brief           init dma hmac
 * @param[in]       ctx                  - hmac_dma_ctx_t context pointer
 * @param[in]       alg                  - specific hash algorithm
 * @param[in]       key                  - key
 * @param[in]       sp_key_idx           - index of secure port key
 * @param[in]       key_bytes            - key byte length
 * @param[in]       callback             - callback function pointer
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure alg is valid
 *        2. here hmac is not for SHA3
 */
unsigned int hmac_dma_init(hmac_dma_ctx_t *ctx, hash_alg_e alg, const unsigned char *key, unsigned short sp_key_idx, unsigned int key_bytes, hash_callback callback)
{
    unsigned int ret;
    hmac_ctx_t tmp_ctx[1];

    if (NULL == ctx)
    {
        return HASH_BUFFER_NULL;
    }
    else if (HASH_SUCCESS != check_hash_alg(alg))
    {
        return HASH_INPUT_INVALID;
    }
    else if (NULL == key)
    {
        key_bytes = 0;
    }
    else
    {
        // handle other;
    }

    ret = hmac_init(tmp_ctx, alg, key, sp_key_idx, key_bytes);
    if (HASH_SUCCESS == ret)
    {
        ctx->hash_dma_ctx->alg = alg;
        ctx->hash_dma_ctx->block_word_len = (tmp_ctx->hash_ctx->block_byte_len) / ((unsigned char)4);
        ctx->hash_dma_ctx->digest_byte_len = (unsigned char)(hash_get_digest_word_len(alg) << 2);
        ctx->hash_dma_ctx->callback = callback;
        uint32_copy(ctx->hash_dma_ctx->total, tmp_ctx->hash_ctx->total, CAST2UINT32(ctx->hash_dma_ctx->block_word_len) >> 3);
        memcpy_((unsigned char *)(ctx->K0), (unsigned char *)(tmp_ctx->K0), CAST2UINT32(ctx->hash_dma_ctx->block_word_len) << 2);

#ifdef CONFIG_HASH_SUPPORT_MUL_THREAD
        ctx->hash_dma_ctx->first_update_flag = (unsigned char)0;
        ctx->hash_dma_ctx->iterator_word_len = hash_get_iterator_word_len(alg);
        memcpy_((unsigned char *)(ctx->hash_dma_ctx->iterator), (unsigned char *)(tmp_ctx->hash_ctx->iterator), CAST2UINT32(ctx->hash_dma_ctx->iterator_word_len) << 2);
#else
        hash_set_dma_mode();
        hash_set_dma_output_len(0);
#endif
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           dma hmac update message
 * @param[in]       ctx                  - hmac_dma_ctx_t context pointer
 * @param[in]       msg                  - message
 * @param[in]       msg_len            - byte length of the input message, must be a multiple of block byte length of HASH
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the four parameters are valid, and ctx is initialize
 */
unsigned int hmac_dma_update_blocks(hmac_dma_ctx_t *ctx, const unsigned int *msg, unsigned int msg_len)
{
    if (NULL == ctx)
    {
        return HASH_BUFFER_NULL;
    }
    else
    {
        return hash_dma_update_blocks(ctx->hash_dma_ctx, msg, msg_len);
    }
}

/**
 * @brief           dma hmac message update done, get the hmac
 * @param[in]       ctx                  - hmac_dma_ctx_t context pointer
 * @param[in]       remainder_msg        - message
 * @param[in]       remainder_bytes      - byte length of the last message
 * @param[out]      mac                  - hmac
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the three parameters are valid, and ctx is initialize
 */
unsigned int hmac_dma_final(hmac_dma_ctx_t *ctx, const unsigned int *remainder_msg, unsigned int remainder_bytes, unsigned int *mac)
{
    unsigned int i;
    unsigned int ret;
    hash_ctx_t tmp_ctx[1];
    unsigned char *mac_remap;
#ifdef CONFIG_HASH_OS
    crypto_port_st *port = ehsm_get_crypto_port(CRYPTO_PORT_TYPE_MAILBOX);
#endif

    if ((NULL == ctx) || (NULL == mac))
    {
        return HASH_BUFFER_NULL;
    }
    else
    {
    }

    ret = hash_dma_final(ctx->hash_dma_ctx, remainder_msg, remainder_bytes, mac); // print_buf_U8(mac, 32, "mac---------");
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // get K0 ^ opad
    for (i = 0; i < ctx->hash_dma_ctx->block_word_len; i++)
    {
        ctx->K0[i] ^= HMAC_IPAD_XOR_OPAD;
    }

    ret = hash_init(tmp_ctx, ctx->hash_dma_ctx->alg);
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    ret = hash_update(tmp_ctx, (unsigned char *)(ctx->K0), CAST2UINT32(ctx->hash_dma_ctx->block_word_len) << 2);
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    mac_remap = lib_addr_arch32_lock_remap(0U, (unsigned int)mac, 0U);

    // tmp_iterator may not be accessed by bytes
    memcpy_(ctx->K0, mac_remap, CAST2UINT32(ctx->hash_dma_ctx->digest_byte_len));
    ret = hash_update(tmp_ctx, (unsigned char *)(ctx->K0), ctx->hash_dma_ctx->digest_byte_len);
    if (HASH_SUCCESS == ret)
    {
        ret = hash_final(tmp_ctx, (unsigned char *)mac_remap);
    }
    else
    {
    }

    lib_addr_arch32_unlock_remap();

    return ret;
}

/**
 * @brief           dma hmac input key and message, get the hmac
 * @param[in]       alg                  - specific hash algorithm
 * @param[in]       key                  - key
 * @param[in]       sp_key_idx           - index of secure port key
 * @param[in]       key_bytes            - key byte length
 * @param[in]       msg                  - message
 * @param[in]       msg_len            - byte length of the input message
 * @param[out]      mac                  - hmac
 * @param[in]       callback             - callback function pointer
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure alg is valid
 *        2. here hmac is not for SHA3
 */
unsigned int hmac_dma(hash_alg_e alg, const unsigned char *key, unsigned short sp_key_idx, unsigned int key_bytes, unsigned int *msg, unsigned int msg_len, unsigned int *mac,
                      hash_callback callback)
{
    unsigned int ret;
    hmac_dma_ctx_t ctx[1];

    ret = hmac_dma_init(ctx, alg, key, sp_key_idx, key_bytes, callback);
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    return hmac_dma_final(ctx, msg, msg_len, mac);
}

#ifdef SUPPORT_HASH_DMA_NODE
/**
 * @brief           dma hmac input key and message, get the hmac(node style)
 * @param[in]       alg                  - specific hash algorithm
 * @param[in]       key                  - key
 * @param[in]       sp_key_idx           - index of secure port key
 * @param[in]       key_bytes            - key byte length
 * @param[in]       node                 - message node pointer
 * @param[in]       node_num             - number of hash nodes, i.e. number of message segments.
 * @param[out]      mac                  - hmac
 * @param[in]       callback             - callback function pointer
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the digest buffer is sufficient
 *        2. please make sure alg is valid
 *        3. here hmac is not for SHA3.
 *        4. if the whole message consists of some segments, every segment is a node, a node includes
 *           address and byte length.
 *        5. for every node or segment except the last, its message length must be a multiple of block length
 */
unsigned int hmac_dma_node_steps(hash_alg_e alg, const unsigned char *key, unsigned short sp_key_idx, unsigned int key_bytes, const hash_dma_node_t *node, unsigned int node_num,
                                 unsigned int *mac, hash_callback callback)
{
    unsigned int i, ret;
    hmac_dma_ctx_t ctx[1];

    ret = hmac_dma_init(ctx, alg, key, sp_key_idx, key_bytes, callback);
    if (HASH_SUCCESS == ret)
    {
        for (i = 0; i < (node_num - 1U); i++)
        {
            ret = hmac_dma_update_blocks(ctx, node[i].msg_addr, node[i].msg_len);
            if (HASH_SUCCESS != ret)
            {
                break;
            }
            else
            {
            }
        }

        if (HASH_SUCCESS == ret)
        {
            ret = hmac_dma_final(ctx, node[i].msg_addr, node[i].msg_len, mac);
        }
        else
        {
        }
    }
    else
    {
    }

    return ret;
}
#endif

#endif
