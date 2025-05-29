/********************************************************************************************************
 * @file    hmac.c
 *
 * @brief   This is the source file for TL721X
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
#include "lib/include/crypto_common/utility.h"
#include "lib/include/hash/hash.h"
#include "lib/include/hash/hmac.h"


#ifdef HMAC_SECURE_PORT_FUNCTION
/**
 * @brief       ENABLE HMAC
 * @param[in]   sp_key_idx    - index of secure port key.
 * @return      none
 */
void hash_hmac_enable_secure_port(unsigned short sp_key_idx)
{
    sp_key_idx++; //avoid unused warning

    return;
}
#endif

/**
 * @brief       init HMAC
 * @param[in]   ctx            - HMAC_CTX context pointer.
 * @param[in]   hash_alg       - specific hash algorithm.
 * @param[in]   key            - key.
 * @param[in]   sp_key_idx     - index of secure port key.
 * @param[in]   key_bytes      - byte length of key, it could be 0.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure hash_alg is valid.
  @endverbatim
 */
unsigned int hmac_init(HMAC_CTX *ctx, HASH_ALG hash_alg, const unsigned char *key, unsigned short sp_key_idx, unsigned int key_bytes)
{
    unsigned int block_byte_len, digest_byte_len;
    unsigned int i, ret;

    if (NULL == ctx) {
        return HASH_BUFFER_NULL;
    } else if (HASH_SUCCESS != check_hash_alg(hash_alg)) {
        return HASH_INPUT_INVALID;
    } else if (NULL == key) {
#ifdef HMAC_SECURE_PORT_FUNCTION
        //TODO
#else
        key_bytes = 0;
#endif
    } else {
        ;
    }

    if (key) //key is from user input
    {
        //hash_hmac_disable_secure_port();
    } else //key is from secure port
    {
#ifdef HMAC_SECURE_PORT_FUNCTION
        //open micro, avoid unused key_idx warning
        hash_hmac_enable_secure_port(sp_key_idx); //this function design by user
#endif
    }

    block_byte_len  = hash_get_block_word_len(hash_alg) << 2;
    digest_byte_len = hash_get_digest_word_len(hash_alg) << 2;

    ctx->hash_alg = hash_alg;

    //get K0
    if (key_bytes <= block_byte_len) {
        memcpy_((unsigned char *)ctx->K0, key, key_bytes);
        memset_(((unsigned char *)(ctx->K0)) + key_bytes, 0, block_byte_len - key_bytes);
    } else {
        //K0 = hash(key)||000..00
        ret = hash_init(ctx->hash_ctx, hash_alg);
        if (HASH_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        ret = hash_update(ctx->hash_ctx, key, key_bytes);
        if (HASH_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        ret = hash_final(ctx->hash_ctx, (unsigned char *)(ctx->K0));
        if (HASH_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        memset_(((unsigned char *)(ctx->K0)) + digest_byte_len, 0, block_byte_len - digest_byte_len);
    }

    //get K0 ^ ipad
    digest_byte_len = block_byte_len / 4;
    for (i = 0; i < digest_byte_len; i++) {
        ctx->K0[i] ^= HMAC_IPAD;
    }

    ret = hash_init(ctx->hash_ctx, hash_alg);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ret = hash_update(ctx->hash_ctx, (unsigned char *)(ctx->K0), block_byte_len);
    }

END:
    if (HASH_SUCCESS != ret) {
        memset_(ctx, 0, sizeof(HMAC_CTX));
    } else {
        ;
    }

    return ret;
}

/**
 * @brief       hmac update message
 * @param[in]   ctx            - HMAC_CTX context pointer.
 * @param[in]   msg            - message.
 * @param[in]   msg_bytes      - byte length of the input message.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the three parameters are valid, and ctx is initialized.
  @endverbatim
 */
unsigned int hmac_update(HMAC_CTX *ctx, const unsigned char *msg, unsigned int msg_bytes)
{
    if (NULL == ctx) {
        return HASH_BUFFER_NULL;
    } else {
        return hash_update(ctx->hash_ctx, msg, msg_bytes);
    }
}

/**
 * @brief       message update done, get the hmac
 * @param[in]   ctx            - HMAC_CTX context pointer.
 * @param[out]  mac            - message.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the ctx is valid and initialized.
      -# 2. please make sure the mac buffer is sufficient.
  @endverbatim
 */
unsigned int hmac_final(HMAC_CTX *ctx, unsigned char *mac)
{
    unsigned int block_word_len;
    unsigned int i, ret;

    if (NULL == ctx || NULL == mac) {
        return HASH_BUFFER_NULL;
    } else {
        ;
    }

    //set mac as hash((K0^ipad)||message)
    ret = hash_final(ctx->hash_ctx, mac);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    //get K0 ^ opad
    block_word_len = hash_get_block_word_len(ctx->hash_alg);
    for (i = 0; i < block_word_len; i++) {
        ctx->K0[i] ^= HMAC_IPAD_XOR_OPAD;
    }

    ret = hash_init(ctx->hash_ctx, ctx->hash_alg);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = hash_update(ctx->hash_ctx, (unsigned char *)(ctx->K0), ctx->hash_ctx->block_byte_len);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = hash_update(ctx->hash_ctx, mac, ctx->hash_ctx->digest_byte_len);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = hash_final(ctx->hash_ctx, mac);

END:
    if (HASH_SUCCESS != ret) {
        memset_(mac, 0, hash_get_digest_word_len(ctx->hash_alg) << 2);
    } else {
        ;
    }

    memset_(ctx, 0, sizeof(HMAC_CTX));

    return ret;
}

/**
 * @brief       input key and whole message, get the hmac
 * @param[in]   hash_alg       - specific hash algorithm.
 * @param[in]   key            - key.
 * @param[in]   sp_key_idx     - index of secure port key.
 * @param[in]   key_bytes      - byte length of the key.
 * @param[in]   msg            - message.
 * @param[in]   msg_bytes      - byte length of the input message.
 * @param[out]  mac            - hmac.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the mac buffer is sufficient.
  @endverbatim
 */
unsigned int hmac(HASH_ALG hash_alg, unsigned char *key, unsigned short sp_key_idx, unsigned int key_bytes, unsigned char *msg, unsigned int msg_bytes, unsigned char *mac)
{
    HMAC_CTX     ctx[1];
    unsigned int ret;

    if (NULL == mac) {
        return HASH_BUFFER_NULL;
    } else {
        ;
    }

    ret = hmac_init(ctx, hash_alg, key, sp_key_idx, key_bytes);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = hash_update(ctx->hash_ctx, (unsigned char *)msg, msg_bytes);
    if (HASH_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    ret = hmac_final(ctx, mac);

END:
    memset_(ctx, 0, sizeof(HMAC_CTX));

    return ret;
}


#ifdef HASH_DMA_FUNCTION
/**
 * @brief       input key and whole message, get the hmac
 * @param[in]   ctx            - HMAC_DMA_CTX context pointer.
 * @param[in]   hash_alg       - specific hash algorithm.
 * @param[in]   key            - key.
 * @param[in]   sp_key_idx     - index of secure port key.
 * @param[in]   key_bytes      - key byte length.
 * @param[in]   callback       - callback function pointer.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure hash_alg is valid.
      -# 1. here hmac is not for SHA3.
  @endverbatim
 */
unsigned int hmac_dma_init(HMAC_DMA_CTX *ctx, HASH_ALG hash_alg, const unsigned char *key, unsigned short sp_key_idx, unsigned int key_bytes, HASH_CALLBACK callback)
{
    unsigned int ret;

    if (NULL == ctx) {
        return HASH_BUFFER_NULL;
    } else if (HASH_SUCCESS != check_hash_alg(hash_alg)) {
        return HASH_INPUT_INVALID;
    } else if (NULL == key) {
        key_bytes = 0;
    } else {
        ;
    }

    ret = hmac_init(ctx->hmac_ctx, hash_alg, key, sp_key_idx, key_bytes);
    if (HASH_SUCCESS == ret) {
        ctx->hash_dma_ctx->hash_alg       = hash_alg;
        ctx->hash_dma_ctx->block_word_len = (ctx->hmac_ctx->hash_ctx->block_byte_len) / 4;
        uint32_copy(ctx->hash_dma_ctx->total, ctx->hmac_ctx->hash_ctx->total, (ctx->hash_dma_ctx->block_word_len) / 8);
        ctx->hash_dma_ctx->callback = callback;

        hash_set_dma_mode();
        hash_set_dma_output_len(0);
    } else {
        ;
    }

    return ret;
}

/**
 * @brief       dma hmac update message
 * @param[in]   ctx            - HMAC_DMA_CTX context pointer.
 * @param[in]   msg            - message.
 * @param[in]   msg_words      - word length of the input message, must be a multiple of block word length of HASH.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the four parameters are valid, and ctx is initialized.
  @endverbatim
 */
unsigned int hmac_dma_update_blocks(HMAC_DMA_CTX *ctx, unsigned int *msg, unsigned int msg_words)
{
    if (NULL == ctx) {
        return HASH_BUFFER_NULL;
    } else {
        return hash_dma_update_blocks(ctx->hash_dma_ctx, msg, msg_words);
    }
}

/**
 * @brief       dma hmac message update done, get the hmac
 * @param[in]   ctx                - HMAC_DMA_CTX context pointer.
 * @param[in]   remainder_msg      - message.
 * @param[in]   remainder_bytes    - byte length of the last message, must be in [0, BLOCK_BYTE_LEN-1],
 *                                   here BLOCK_BYTE_LEN is block byte length of HASH.
 * @param[out]  mac                - hmac
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the three parameters are valid, and ctx is initialized.
  @endverbatim
 */
unsigned int hmac_dma_final(HMAC_DMA_CTX *ctx, unsigned int *remainder_msg, unsigned int remainder_bytes, unsigned int *mac)
{
    unsigned int i;
    unsigned int ret;

    if ((NULL == ctx) || (NULL == mac)) {
        return HASH_BUFFER_NULL;
    } else {
        ;
    }

    ret = hash_dma_final(ctx->hash_dma_ctx, remainder_msg, remainder_bytes, mac); //print_buf_U8(mac, 32, "mac---------");
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //get K0 ^ opad
    for (i = 0; i < ctx->hash_dma_ctx->block_word_len; i++) {
        ctx->hmac_ctx->K0[i] ^= HMAC_IPAD_XOR_OPAD;
    }

    ret = hash_init(ctx->hmac_ctx->hash_ctx, ctx->hmac_ctx->hash_alg);
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = hash_update(ctx->hmac_ctx->hash_ctx, (unsigned char *)(ctx->hmac_ctx->K0), ctx->hmac_ctx->hash_ctx->block_byte_len);
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //tmp_iterator may not be accessed by bytes
    uint32_copy(ctx->hmac_ctx->K0, mac, ctx->hmac_ctx->hash_ctx->digest_byte_len / 4);
    ret = hash_update(ctx->hmac_ctx->hash_ctx, (unsigned char *)(ctx->hmac_ctx->K0), ctx->hmac_ctx->hash_ctx->digest_byte_len);
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        return hash_final(ctx->hmac_ctx->hash_ctx, (unsigned char *)mac);
    }
}

/**
 * @brief       dma hmac input key and message, get the hmac
 * @param[in]   hash_alg                - specific hash algorithm.
 * @param[in]   key                     - key.
 * @param[in]   sp_key_idx              - index of secure port key.
 * @param[in]   key_bytes               - key byte length.
 * @param[in]   msg                     - message.
 * @param[in]   msg_bytes               - byte length of the input message.
 * @param[out]  mac                     - hmac.
 * @param[in]   callback                - callback function pointer.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure hash_alg is valid.
  @endverbatim
 */
unsigned int hmac_dma(HASH_ALG hash_alg, unsigned char *key, unsigned short sp_key_idx, unsigned int key_bytes, unsigned int *msg, unsigned int msg_bytes, unsigned int *mac, HASH_CALLBACK callback)
{
    unsigned int blocks_words, remainder_bytes;
    unsigned int ret;
    HMAC_DMA_CTX ctx[1];

    if (NULL == mac) {
        return HASH_BUFFER_NULL;
    } else {
        ;
    }

    if (NULL == key) {
        key_bytes = 0;
    } else {
        ;
    }

    if (NULL == msg) {
        msg_bytes = 0;
    } else {
        ;
    }

    ret = hmac_dma_init(ctx, hash_alg, key, sp_key_idx, key_bytes, callback);
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    remainder_bytes = msg_bytes % ctx->hmac_ctx->hash_ctx->block_byte_len;
    blocks_words    = (msg_bytes - remainder_bytes) / 4;
    ret             = hash_dma_update_blocks(ctx->hash_dma_ctx, msg, blocks_words);
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        return hmac_dma_final(ctx, (unsigned int *)(msg + blocks_words), remainder_bytes, mac);
    }
}
#endif
