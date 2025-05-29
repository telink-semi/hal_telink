/*! @file ske_aes_xcbc_mac_96.c */
#include <stdio.h>

#include "lib/include/crypto_common/utility.h"
#include "lib/include/ske/ske_aes_xcbc_mac_96.h"

#ifdef SUPPORT_SKE_AES_XCBC_MAC_96

unsigned int ske_lp_update_blocks_internal_(ske_ctx_t *ctx, const unsigned char *in, unsigned char *out, unsigned int bytes);
unsigned int ske_lp_update_blocks_no_output_(ske_ctx_t *ctx, const unsigned char *in, unsigned int bytes);

/**
 * @brief           ske_lp get aes_xcbc_mac_96 k1, k2 and k3
 * @param[in]       ctx                  - ske_ctx_t context pointer
 * @param[in]       key                  - AES128 key in bytes
 * @param[in]       sp_key_idx           - index of secure port key, (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX],
 * @param[out]      k1                   - aes_xcbc_mac_96 k1
 * @param[out]      k2                   - aes_xcbc_mac_96 k2
 * @param[out]      k3                   - aes_xcbc_mac_96 k3
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. if key is from user input, please make sure key is not NULL(now sp_key_idx is useless),
 *           otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX
 */
unsigned int ske_lp_aes_xcbc_mac_96_get_k1_k2_k3(ske_ctx_t *ctx, const unsigned char *key, unsigned short sp_key_idx, unsigned char k1[16], unsigned char k2[16],
                                                 unsigned char k3[16])
{
    unsigned int ret;

    ret = ske_lp_init_internal(ctx, SKE_ALG_AES_128, SKE_MODE_ECB, SKE_CRYPTO_ENCRYPT, key, sp_key_idx, NULL, SKE_LP_DMA_DISABLE);
    if (SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    uint32_set((unsigned int *)k1, 0x02020202, 4);
    ret |= ske_lp_update_blocks_internal(ctx, (unsigned char *)k1, k2, 0x10);
    uint32_set((unsigned int *)k1, 0x03030303, 4);
    ret |= ske_lp_update_blocks_internal(ctx, (unsigned char *)k1, k3, 0x10);
    uint32_set((unsigned int *)k1, 0x01010101, 4);
    ret |= ske_lp_update_blocks_internal(ctx, (unsigned char *)k1, (unsigned char *)k1, 0x10);
    if (SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    return SKE_SUCCESS;
}

/**
 * @brief           ske_lp aes_xcbc_mac_96 init(CPU style)
 * @param[in]       ctx                  - ske_aes_xcbc_mac_96_ctx_t context pointer
 * @param[in]       mac_action           - must be SKE_GENERATE_MAC or SKE_VERIFY_MAC
 * @param[in]       key                  - key in bytes
 * @param[in]       sp_key_idx           - index of secure port key, (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX],
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. if key is from user input, please make sure key is not NULL(now sp_key_idx is useless),
 *           otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX
 */
unsigned int ske_lp_aes_xcbc_mac_96_init(ske_aes_xcbc_mac_96_ctx_t *ctx, ske_mac_e mac_action, const unsigned char *key, unsigned short sp_key_idx)
{
    unsigned int k1[4];
    unsigned int ret;

    // check and keep ctx->left_bytes = 0
    if (NULL == ctx)
    {
        return SKE_BUFFER_NULL;
    }
    else if (mac_action > SKE_VERIFY_MAC)
    {
        return SKE_INPUT_INVALID;
    }
    else
    {
        ;
    }

    ctx->left_bytes = 0;
    ctx->mac_action = mac_action;

    ske_lp_set_cpu_mode();

    ret = ske_lp_aes_xcbc_mac_96_get_k1_k2_k3(ctx->ske_xcbc_mac_ctx, key, sp_key_idx, (unsigned char *)k1, ctx->k2, ctx->k3);
    if (SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    // set iv zero
    uint32_clear((unsigned int *)ctx->block_buf, 4);

    ske_lp_set_cpu_mode(); // since use k1

    return ske_lp_init_internal(ctx->ske_xcbc_mac_ctx, SKE_ALG_AES_128, SKE_MODE_CBC, SKE_CRYPTO_ENCRYPT, (unsigned char *)k1, sp_key_idx, (unsigned char *)ctx->block_buf,
                                SKE_LP_DMA_DISABLE);
}

/**
 * @brief           ske_lp aes_xcbc_mac_96 update message(CPU style)
 * @param[in]       ctx                  - ske_aes_xcbc_mac_96_ctx_t context pointer
 * @param[in]       msg                  - message
 * @param[in]       msg_bytes            - byte length of message.
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. msg_bytes could be any value
 */
unsigned int ske_lp_aes_xcbc_mac_96_update(ske_aes_xcbc_mac_96_ctx_t *ctx, unsigned char *msg, unsigned int msg_bytes)
{
    unsigned int blocks_bytes;
    unsigned int ret;
    unsigned char fill_bytes, remainder;

    if (NULL == ctx)
    {
        return SKE_BUFFER_NULL;
    }
    else if ((NULL == msg) || (0 == msg_bytes))
    {
        return SKE_SUCCESS;
    }
    else
    {
        ;
    }

    // if one block left, process it
    if (ctx->ske_xcbc_mac_ctx->block_bytes == ctx->left_bytes)
    {
        ret = ske_lp_update_blocks_no_output_(ctx->ske_xcbc_mac_ctx, ctx->block_buf, ctx->ske_xcbc_mac_ctx->block_bytes);
        if (SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            ctx->left_bytes = 0;
        }
    }
    else
    {
        ;
    }

    // padding
    if (ctx->left_bytes)
    {
        fill_bytes = ctx->ske_xcbc_mac_ctx->block_bytes - ctx->left_bytes;
        if (msg_bytes <= fill_bytes)
        {
            memcpy_(ctx->block_buf + ctx->left_bytes, msg, msg_bytes);
            ctx->left_bytes += msg_bytes;
            return SKE_SUCCESS;
        }
        else
        {
            memcpy_(ctx->block_buf + ctx->left_bytes, msg, fill_bytes);
            ret = ske_lp_update_blocks_no_output_(ctx->ske_xcbc_mac_ctx, ctx->block_buf, ctx->ske_xcbc_mac_ctx->block_bytes);
            if (SKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {
                ctx->left_bytes = 0;
                msg += fill_bytes;
                msg_bytes -= fill_bytes;
            }
        }
    }
    else
    {
        ;
    }

    // process some blocks
    blocks_bytes = (msg_bytes / ctx->ske_xcbc_mac_ctx->block_bytes) * ctx->ske_xcbc_mac_ctx->block_bytes;
    remainder = msg_bytes % ctx->ske_xcbc_mac_ctx->block_bytes;

    // process remainder
    if (remainder)
    {
        ret = ske_lp_update_blocks_no_output_(ctx->ske_xcbc_mac_ctx, msg, blocks_bytes); // print_buf_u8(msg, blocks_bytes, "blocks_bytes");
        if (SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            memcpy_(ctx->block_buf, msg + blocks_bytes, remainder);
            ctx->left_bytes = remainder;
        }
    }
    else
    {
        blocks_bytes -= ctx->ske_xcbc_mac_ctx->block_bytes;
        ret = ske_lp_update_blocks_no_output_(ctx->ske_xcbc_mac_ctx, msg, blocks_bytes);
        if (SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            memcpy_(ctx->block_buf, msg + blocks_bytes, ctx->ske_xcbc_mac_ctx->block_bytes);
            ctx->left_bytes = ctx->ske_xcbc_mac_ctx->block_bytes;
        }
    }

    return SKE_SUCCESS;
}

/**
 * @brief           ske_lp aes_xcbc_mac_96 finish, and get the mac or verify the mac(CPU style)
 * @param[in]       ctx                  - ske_aes_xcbc_mac_96_ctx_t context pointer
 * @param[in]       mac                  - input(for generating mac), output(for verifying mac)
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. if ctx->mac_action is SKE_GENERATE_MAC, mac is output. and if ctx->mac_action is SKE_VERIFY_MAC,
 *           mac is input, return value SKE_SUCCESS means the mac is valid, otherwise mac is invalid
 */
unsigned int ske_lp_aes_xcbc_mac_96_final(ske_aes_xcbc_mac_96_ctx_t *ctx, unsigned char mac[12])
{
    unsigned int tmp[4];
    unsigned int ret;

    if ((NULL == ctx) || (NULL == mac))
    {
        return SKE_BUFFER_NULL;
    }
    else
    {
        ;
    }

    if (ctx->ske_xcbc_mac_ctx->block_bytes == ctx->left_bytes)
    {
        uint32_xor((unsigned int *)ctx->block_buf, (unsigned int *)ctx->k2, (unsigned int *)ctx->block_buf, ctx->ske_xcbc_mac_ctx->block_words);
        ret = ske_lp_update_blocks_internal_(ctx->ske_xcbc_mac_ctx, ctx->block_buf, (unsigned char *)tmp, ctx->ske_xcbc_mac_ctx->block_bytes);
        if (SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            ;
        }
    }
    else
    {
        ctx->block_buf[ctx->left_bytes] = 0x80;
        memset_(ctx->block_buf + ctx->left_bytes + 1, 0, ctx->ske_xcbc_mac_ctx->block_bytes - 1 - ctx->left_bytes);
        uint32_xor((unsigned int *)ctx->block_buf, (unsigned int *)ctx->k3, (unsigned int *)ctx->block_buf, ctx->ske_xcbc_mac_ctx->block_words);
        ret = ske_lp_update_blocks_internal_(ctx->ske_xcbc_mac_ctx, ctx->block_buf, (unsigned char *)tmp, ctx->ske_xcbc_mac_ctx->block_bytes);
        if (SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            ;
        }
    }

    if (SKE_GENERATE_MAC == ctx->mac_action)
    {
        memcpy_(mac, tmp, 0x0C);
        ret = SKE_SUCCESS;
    }
    else
    {
        ret = memcmp_(mac, tmp, 0x0C);
    }

    return ret;
}

/**
 * @brief           ske_lp xcbc_mac_96(CPU style, one-off style)
 * @param[in]       mac_action           - must be SKE_GENERATE_MAC or SKE_VERIFY_MAC
 * @param[in]       key                  - key in bytes
 * @param[in]       sp_key_idx           - index of secure port key, (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX],
 * @param[in]       msg                  - message
 * @param[in]       msg_bytes            - byte length of message.
 * @param[in]       mac                  - input(for generating mac), output(for verifying mac)
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. if key is from user input, please make sure key is not NULL(now sp_key_idx is useless),
 *           otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX]
 *        2. msg_bytes could be any value.
 *        3. if mac_action is SKE_GENERATE_MAC, mac is output. and if mac_action is SKE_VERIFY_MAC,
 *           mac is input, return value SKE_SUCCESS means the mac is valid, otherwise mac is invalid
 */
unsigned int ske_lp_aes_xcbc_mac_96(ske_mac_e mac_action, const unsigned char *key, unsigned short sp_key_idx, unsigned char *msg, unsigned int msg_bytes, unsigned char mac[12])
{
    ske_aes_xcbc_mac_96_ctx_t ctx[1];
    unsigned int ret;

    ret = ske_lp_aes_xcbc_mac_96_init(ctx, mac_action, key, sp_key_idx);
    if (SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    ret = ske_lp_aes_xcbc_mac_96_update(ctx, msg, msg_bytes);
    if (SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    return ske_lp_aes_xcbc_mac_96_final(ctx, mac);
}

#ifdef SKE_LP_DMA_FUNCTION

/**
 * @brief           ske_lp aes-xcbc-mac-96 dma style init
 * @param[in]       ctx                  - ske_aes_xcbc_mac_96_dma_ctx_t context pointer
 * @param[in]       key                  - key in bytes
 * @param[in]       sp_key_idx           - index of secure port key, (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX],
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. if key is from user input, please make sure key is not NULL(now sp_key_idx is useless),
 *           otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX
 */
unsigned int ske_lp_dma_aes_xcbc_mac_96_init(ske_aes_xcbc_mac_96_dma_ctx_t *ctx, const unsigned char *key, unsigned short sp_key_idx)
{
    unsigned int k1[4];
    unsigned int iv[4];
    unsigned int ret;

    if (NULL == ctx)
    {
        return SKE_BUFFER_NULL;
    }
    else
    {
        ;
    }

    ske_lp_set_cpu_mode();

    ret = ske_lp_aes_xcbc_mac_96_get_k1_k2_k3(ctx->ske_xcbc_mac_ctx, key, sp_key_idx, (unsigned char *)k1, ctx->k2, ctx->k3);
    if (SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    // set iv zero
    uint32_clear(iv, 4);

    return ske_lp_dma_init(SKE_ALG_AES_128, SKE_MODE_CBC, SKE_CRYPTO_ENCRYPT, (unsigned char *)k1, sp_key_idx, (const unsigned char *)iv);
}

/**
 * @brief           ske_lp aes-xcbc-mac-96 dma style update update message blocks(excluding the last block, or the message tail)
 * @param[in]       ctx                  - ske_aes_xcbc_mac_96_dma_ctx_t context pointer
 * @param[in]       msg                  - message of some blocks, excluding last block(or message tail)
 * @param[in]       msg_words            - word length of msg, must be a multiple of block word length
 * @param[out]      tmp_out              - temporary output ciphertext, ske_lp need to output, it occupies the same blocks as the
 * @param[in]       callback             - callback function pointer, this could be NULL, means doing nothing
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. the input msg must be some blocks, and excludes the last block(or message tail
 */
unsigned int ske_lp_dma_aes_xcbc_mac_96_update_blocks_excluding_last_block(ske_aes_xcbc_mac_96_dma_ctx_t *ctx, unsigned int *msg, unsigned int msg_words, unsigned int *tmp_out,
                                                                           SKE_CALLBACK callback)
{
    if ((NULL == msg) || (0 == msg_words))
    {
        return SKE_SUCCESS;
    }
    else if ((NULL == ctx) || (NULL == tmp_out))
    {
        return SKE_BUFFER_NULL;
    }
    else if (msg_words & (ctx->ske_xcbc_mac_ctx->block_words - 1))
    {
        return SKE_INPUT_INVALID;
    }
    else
    {
        ;
    }

    return ske_lp_dma_update_blocks(msg, tmp_out, msg_words, callback);
}

/**
 * @brief           ske_lp aes-xcbc-mac-96 dma style update message including the last block(or message tail), and get the mac
 * @param[in]       ctx                  - ske_aes_xcbc_mac_96_dma_ctx_t context pointer
 * @param[in]       msg                  - message including the last block(or message tail)
 * @param[in]       msg_bytes            - byte length of msg, could be 0
 * @param[out]      tmp_out              - temporary output ciphertext, ske_lp need to output, it occupies the same blocks as the
 * @param[out]      mac                  - aes-xcbc-mac-96, occupies 12 bytes
 * @param[in]       callback             - callback function pointer, this could be NULL, means doing nothing
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. if the whole message length is 0, this case is supported. in this case, msg occupies a block, and
 *           please set msg_bytes to 0
 */
unsigned int ske_lp_dma_aes_xcbc_mac_96_update_including_last_block(ske_aes_xcbc_mac_96_dma_ctx_t *ctx, unsigned int *msg, unsigned int msg_bytes, unsigned int *tmp_out,
                                                                    unsigned int *mac, SKE_CALLBACK callback)
{
    unsigned int remainder_bytes;
    unsigned int block_num;
    unsigned int ret;

    if ((NULL == ctx) || (NULL == msg) || (NULL == tmp_out) || (NULL == mac))
    {
        return SKE_BUFFER_NULL;
    }
    else
    {
        ;
    }

    remainder_bytes = msg_bytes % ctx->ske_xcbc_mac_ctx->block_bytes;
    block_num = msg_bytes / ctx->ske_xcbc_mac_ctx->block_bytes;

    if ((0 == remainder_bytes) && (0 != msg_bytes))
    {
        ret = ske_lp_dma_update_blocks(msg, tmp_out, (block_num - 1) * ctx->ske_xcbc_mac_ctx->block_words, callback);
        if (SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            ;
        }

        // last block
        uint32_xor(msg + (block_num - 1) * ctx->ske_xcbc_mac_ctx->block_words, (unsigned int *)ctx->k2, tmp_out, ctx->ske_xcbc_mac_ctx->block_words);
        ret = ske_lp_dma_update_blocks(tmp_out, tmp_out, ctx->ske_xcbc_mac_ctx->block_words, callback);
        if (SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            ;
        }
    }
    else
    {
        ret = ske_lp_dma_update_blocks(msg, tmp_out, block_num * ctx->ske_xcbc_mac_ctx->block_words, callback);
        if (SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            ;
        }

        // last block
        memset_((unsigned char *)msg + msg_bytes, 0x80, 1);
        memset_((unsigned char *)msg + msg_bytes + 1, 0, ctx->ske_xcbc_mac_ctx->block_bytes - 1 - remainder_bytes);

        uint32_xor(msg + block_num * ctx->ske_xcbc_mac_ctx->block_words, (unsigned int *)ctx->k3, tmp_out, ctx->ske_xcbc_mac_ctx->block_words);
        ret = ske_lp_dma_update_blocks(tmp_out, tmp_out, ctx->ske_xcbc_mac_ctx->block_words, callback);
        if (SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            ;
        }
    }

    ske_lp_dma_final();
    memcpy_(mac, (unsigned char *)tmp_out, 0x0C);

    return SKE_SUCCESS;
}

/**
 * @brief           ske_lp dma_aes_xcbc_mac_96(DMA style, one-off style)
 * @param[in]       key                  - key in byte buffer style
 * @param[in]       sp_key_idx           - index of secure port key, (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX],
 * @param[in]       msg                  - message
 * @param[in]       msg_bytes            - byte length of message.
 * @param[out]      tmp_out              - temporary output ciphertext, ske_lp need to output, it occupies the same blocks as the
 * @param[out]      mac                  - mac
 * @param[in]       callback             - callback function pointer, this could be NULL, means doing nothing
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. if key is from user input, please make sure key is not NULL(now sp_key_idx is useless),
 *           otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX]
 *        2. msg_bytes is actual byte length of message, it could be any value(including 0).
 *           (1). if msg_bytes is not 0, msg must have (msg_bytes+15)/16 blocks.
 *           (2). if msg_bytes is 0, msg occupies a block
 */
unsigned int ske_lp_dma_aes_xcbc_mac_96(const unsigned char *key, unsigned short sp_key_idx, unsigned int *msg, unsigned int msg_bytes, unsigned int *tmp_out, unsigned int *mac,
                                        SKE_CALLBACK callback)
{
    ske_aes_xcbc_mac_96_dma_ctx_t ctx[1];
    unsigned int ret;

    ret = ske_lp_dma_aes_xcbc_mac_96_init(ctx, key, sp_key_idx);
    if (SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    return ske_lp_dma_aes_xcbc_mac_96_update_including_last_block(ctx, msg, msg_bytes, tmp_out, mac, callback);
}
#endif

#endif
