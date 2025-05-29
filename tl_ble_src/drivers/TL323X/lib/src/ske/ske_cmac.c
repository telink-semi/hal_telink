/*! @file ske_cmac.c */
#include <stdio.h>
#include "lib/include/crypto_common/utility.h"
#include "lib/include/ske/ske_cmac.h"

#ifdef SUPPORT_SKE_MODE_CMAC

unsigned int ske_lp_update_blocks_internal_(ske_ctx_t *ctx, const unsigned char *in, unsigned char *out, unsigned int bytes);
unsigned int ske_lp_update_blocks_no_output_(ske_ctx_t *ctx, const unsigned char *in, unsigned int bytes);

/**
 * @brief           block a = a<<1
 * @param[in]       a                    - input/output, a block
 * @param[in]       a_len                - block byte length
 * @return          none
 */
void block_left_shift_1_bit(unsigned char *a, unsigned int a_len)
{
    unsigned int i;

    for (i = 0; i < a_len - 1; i++)
    {
        a[i] <<= 1;
        a[i] |= (a[i + 1] >> 7);
    }
    a[i] <<= 1;
}

/**
 * @brief           ske_lp get cmac k1 and k2
 * @param[in]       ctx                  - ske_ctx_t context pointer
 * @param[in]       alg                  - ske_lp algorithm
 * @param[in]       key                  - key in bytes
 * @param[in]       sp_key_idx           - index of secure port key, (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX],
 * @param[out]      k1                   - cmac k1
 * @param[out]      k2                   - cmac k2
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. if key is from user input, please make sure key is not NULL(now sp_key_idx is useless),
 *           otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX
 */
unsigned int ske_lp_cmac_get_k1_k2(ske_ctx_t *ctx, ske_alg_e alg, const unsigned char *key, unsigned short sp_key_idx, unsigned char k1[16], unsigned char k2[16])
{
    unsigned int flag = 0, ret;
    unsigned char Rb;

    ret = ske_lp_init_internal(ctx, alg, SKE_MODE_ECB, SKE_CRYPTO_ENCRYPT, key, sp_key_idx, NULL, SKE_LP_DMA_DISABLE);
    if (SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    // get Rb
    if (8 == ctx->block_bytes)
    {
        Rb = 0x1B;
    }
    else
    {
        Rb = 0x87;
    }

    uint32_clear((unsigned int *)k1, ctx->block_words);
    ret = ske_lp_update_blocks_internal_(ctx, k1, k1, ctx->block_bytes);
    if (SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    if (k1[0] & 0x80)
    {
        flag = 1;
    }
    else
    {
        ;
    }

    block_left_shift_1_bit(k1, ctx->block_bytes);
    if (flag)
    {
        k1[ctx->block_bytes - 1] ^= Rb;
    }
    else
    {
        ;
    }

    uint32_copy((unsigned int *)k2, (unsigned int *)k1, ctx->block_words);
    block_left_shift_1_bit(k2, ctx->block_bytes);
    if (k1[0] & 0x80)
    {
        k2[ctx->block_bytes - 1] ^= Rb;
    }
    else
    {
        ;
    }

    return SKE_SUCCESS;
}

/**
 * @brief           ske_lp cmac internal init config
 * @param[in]       ctx                  - ske_cmac_ctx_t context pointer
 * @param[in]       alg                  - ske_lp algorithm
 * @param[in]       key                  - key in bytes
 * @param[in]       sp_key_idx           - index of secure port key, (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX],
 * @param[in]       mac_bytes            - mac byte length, must be bigger than 1, and not bigger than block length
 * @param[in]       dma_en               - for DMA mode(not 0) or not(0)
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. if key is from user input, please make sure key is not NULL(now sp_key_idx is useless),
 *           otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX
 */
unsigned int ske_lp_cmac_init_internal(ske_cmac_ctx_t *ctx, ske_alg_e alg, const unsigned char *key, unsigned short sp_key_idx, unsigned char mac_bytes, unsigned int dma_en)
{
    unsigned int iv[4];
    unsigned int ret;

    // check and keep the mac length
    if (0 == mac_bytes || mac_bytes > ske_lp_get_block_byte_len(alg))
    {
        return SKE_INPUT_INVALID; // SKE_ERROR;
    }
    else
    {
        ctx->mac_bytes = mac_bytes;
    }

    ske_lp_set_cpu_mode();

    ret = ske_lp_cmac_get_k1_k2(ctx->ske_cmac_ctx, alg, key, sp_key_idx, ctx->k1, ctx->k2);
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

    if (dma_en)
    {
        return ske_lp_dma_init(alg, SKE_MODE_CBC, SKE_CRYPTO_ENCRYPT, key, sp_key_idx, (const unsigned char *)iv);
    }
    else
    {
        return ske_lp_init_internal(ctx->ske_cmac_ctx, alg, SKE_MODE_CBC, SKE_CRYPTO_ENCRYPT, key, sp_key_idx, (const unsigned char *)iv, SKE_LP_DMA_DISABLE);
    }
}

/**
 * @brief           ske_lp cmac init(CPU style)
 * @param[in]       ctx                  - ske_cmac_ctx_t context pointer
 * @param[in]       alg                  - ske_lp algorithm
 * @param[in]       mac_action           - must be SKE_GENERATE_MAC or SKE_VERIFY_MAC
 * @param[in]       key                  - key in bytes
 * @param[in]       sp_key_idx           - index of secure port key, (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX],
 * @param[in]       mac_bytes            - mac byte length, must be bigger than 1, and not bigger than block length
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. if key is from user input, please make sure key is not NULL(now sp_key_idx is useless),
 *           otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX
 */
unsigned int ske_lp_cmac_init(ske_cmac_ctx_t *ctx, ske_alg_e alg, ske_mac_e mac_action, const unsigned char *key, unsigned short sp_key_idx, unsigned char mac_bytes)
{
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

    return ske_lp_cmac_init_internal(ctx, alg, key, sp_key_idx, mac_bytes, SKE_LP_DMA_DISABLE);
}

/**
 * @brief           ske_lp cmac update message(CPU style)
 * @param[in]       ctx                  - ske_cmac_ctx_t context pointer
 * @param[in]       msg                  - message
 * @param[in]       msg_bytes            - byte length of message.
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. msg_bytes could be any value
 */
unsigned int ske_lp_cmac_update(ske_cmac_ctx_t *ctx, unsigned char *msg, unsigned int msg_bytes)
{
    unsigned int blocks_bytes;
    unsigned int ret;
    unsigned char fill_bytes, remainder;

    if (NULL == ctx)
    {
        return SKE_BUFFER_NULL;
    }
    else if (NULL == msg || 0 == msg_bytes)
    {
        return SKE_SUCCESS;
    }
    else
    {
        ;
    }

    // if one block left, process it
    if (ctx->ske_cmac_ctx->block_bytes == ctx->left_bytes)
    {
        ret = ske_lp_update_blocks_no_output_(ctx->ske_cmac_ctx, ctx->block_buf, ctx->ske_cmac_ctx->block_bytes);
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
        fill_bytes = ctx->ske_cmac_ctx->block_bytes - ctx->left_bytes;
        if (msg_bytes <= fill_bytes)
        {
            memcpy_(ctx->block_buf + ctx->left_bytes, msg, msg_bytes);
            ctx->left_bytes += msg_bytes;
            return SKE_SUCCESS;
        }
        else
        {
            memcpy_(ctx->block_buf + ctx->left_bytes, msg, fill_bytes);
            ret = ske_lp_update_blocks_no_output_(ctx->ske_cmac_ctx, ctx->block_buf, ctx->ske_cmac_ctx->block_bytes);
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
    blocks_bytes = (msg_bytes / ctx->ske_cmac_ctx->block_bytes) * ctx->ske_cmac_ctx->block_bytes;
    remainder = msg_bytes % ctx->ske_cmac_ctx->block_bytes;

    // process remainder
    if (remainder)
    {
        ret = ske_lp_update_blocks_no_output_(ctx->ske_cmac_ctx, msg, blocks_bytes); // print_buf_u8(msg, blocks_bytes, "blocks_bytes");
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
        blocks_bytes -= ctx->ske_cmac_ctx->block_bytes;
        ret = ske_lp_update_blocks_no_output_(ctx->ske_cmac_ctx, msg, blocks_bytes);
        if (SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            memcpy_(ctx->block_buf, msg + blocks_bytes, ctx->ske_cmac_ctx->block_bytes);
            ctx->left_bytes = ctx->ske_cmac_ctx->block_bytes;
        }
    }

    return SKE_SUCCESS;
}

/**
 * @brief           ske_lp cmac finish, and get the mac or verify the mac(CPU style)
 * @param[in]       ctx                  - ske_cmac_ctx_t context pointer
 * @param[in]       mac                  - input(for generating mac), output(for verifying mac)
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. if ctx->mac_action is SKE_GENERATE_MAC, mac is output. and if ctx->mac_action is SKE_VERIFY_MAC,
 *           mac is input, return value SKE_SUCCESS means the mac is valid, otherwise mac is invalid
 */
unsigned int ske_lp_cmac_final(ske_cmac_ctx_t *ctx, unsigned char *mac)
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

    if (ctx->ske_cmac_ctx->block_bytes == ctx->left_bytes)
    {
        uint32_xor((unsigned int *)ctx->block_buf, (unsigned int *)ctx->k1, (unsigned int *)ctx->block_buf, ctx->ske_cmac_ctx->block_words);
        ret = ske_lp_update_blocks_internal_(ctx->ske_cmac_ctx, ctx->block_buf, (unsigned char *)tmp, ctx->ske_cmac_ctx->block_bytes);
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
        memset_(ctx->block_buf + ctx->left_bytes + 1, 0, ctx->ske_cmac_ctx->block_bytes - 1 - ctx->left_bytes); // print_buf_u8(ctx->block_buf, 16, "ctx->block_buf");
        uint32_xor((unsigned int *)ctx->block_buf, (unsigned int *)ctx->k2, (unsigned int *)ctx->block_buf, ctx->ske_cmac_ctx->block_words);
        ret = ske_lp_update_blocks_internal_(ctx->ske_cmac_ctx, ctx->block_buf, (unsigned char *)tmp, ctx->ske_cmac_ctx->block_bytes);
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
        memcpy_(mac, tmp, ctx->mac_bytes);
        ret = SKE_SUCCESS;
    }
    else
    {
        ret = memcmp_(mac, tmp, ctx->mac_bytes);
    }

    return ret;
}

/**
 * @brief           ske_lp cmac(CPU style, one-off style)
 * @param[in]       alg                  - ske_lp algorithm
 * @param[in]       mac_action           - must be SKE_GENERATE_MAC or SKE_VERIFY_MAC
 * @param[in]       key                  - key in bytes
 * @param[in]       sp_key_idx           - index of secure port key, (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX],
 * @param[in]       msg                  - message
 * @param[in]       msg_bytes            - byte length of message.
 * @param[in]       mac                  - input(for generating mac), output(for verifying mac)
 * @param[in]       mac_bytes            - mac byte length, must be bigger than 1, and not bigger than block length
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. if key is from user input, please make sure key is not NULL(now sp_key_idx is useless),
 *           otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX]
 *        2. msg_bytes could be any value.
 *        3. if mac_action is SKE_GENERATE_MAC, mac is output. and if mac_action is SKE_VERIFY_MAC,
 *           mac is input, return value SKE_SUCCESS means the mac is valid, otherwise mac is invalid
 */
unsigned int ske_lp_cmac(ske_alg_e alg, ske_mac_e mac_action, const unsigned char *key, unsigned short sp_key_idx, unsigned char *msg, unsigned int msg_bytes, unsigned char *mac,
                         unsigned char mac_bytes)
{
    ske_cmac_ctx_t ctx[1];
    unsigned int ret;

    ret = ske_lp_cmac_init(ctx, alg, mac_action, key, sp_key_idx, mac_bytes);
    if (SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    ret = ske_lp_cmac_update(ctx, msg, msg_bytes);
    if (SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    return ske_lp_cmac_final(ctx, mac);
}

#ifdef SKE_LP_DMA_FUNCTION

/**
 * @brief           ske_lp cmac dma style init
 * @param[in]       ctx                  - ske_cmac_dma_ctx_t context pointer
 * @param[in]       alg                  - ske_lp algorithm
 * @param[in]       key                  - key in bytes
 * @param[in]       sp_key_idx           - index of secure port key, (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX],
 * @param[in]       mac_bytes            - mac byte length, must be bigger than 1, and not bigger than block length
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. if key is from user input, please make sure key is not NULL(now sp_key_idx is useless),
 *           otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX
 */
unsigned int ske_lp_dma_cmac_init(ske_cmac_dma_ctx_t *ctx, ske_alg_e alg, const unsigned char *key, unsigned short sp_key_idx, unsigned char mac_bytes)
{
    if (NULL == ctx)
    {
        return SKE_BUFFER_NULL;
    }
    else
    {
        ;
    }

    // CMAC DMA for 3DES is not supported.
    switch (alg)
    {
#ifdef SUPPORT_SKE_TDES_128
    case SKE_ALG_TDES_128:
#endif

#ifdef SUPPORT_SKE_TDES_192
    case SKE_ALG_TDES_192:
#endif

#ifdef SUPPORT_SKE_TDES_EEE_128
    case SKE_ALG_TDES_EEE_128:
#endif

#ifdef SUPPORT_SKE_TDES_EEE_192
    case SKE_ALG_TDES_EEE_192:
#endif

#if (defined(SUPPORT_SKE_TDES_128) || defined(SUPPORT_SKE_TDES_192) || defined(SUPPORT_SKE_TDES_EEE_128) || defined(SUPPORT_SKE_TDES_EEE_192))
        return SKE_INPUT_INVALID;
#endif

    default:
        break;
    }

    return ske_lp_cmac_init_internal((ske_cmac_ctx_t *)ctx, alg, key, sp_key_idx, mac_bytes, SKE_LP_DMA_ENABLE);
}

/**
 * @brief           ske_lp cmac dma style update update message blocks(excluding the last block, or the message tail)
 * @param[in]       ctx                  - ske_cmac_dma_ctx_t context pointer
 * @param[in]       msg                  - message of some blocks, excluding last block(or message tail)
 * @param[in]       msg_words            - word length of msg, must be a multiple of block word length
 * @param[out]      tmp_out              - temporary output ciphertext, ske_lp need to output, it occupies the same blocks as the
 * @param[in]       callback             - callback function pointer, this could be NULL, means doing nothing
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. the input msg must be some blocks, and excludes the last block(or message tail
 */
unsigned int ske_lp_dma_cmac_update_blocks_excluding_last_block(ske_cmac_dma_ctx_t *ctx, unsigned int *msg, unsigned int msg_words, unsigned int *tmp_out, SKE_CALLBACK callback)
{
    if ((NULL == msg) || (0 == msg_words))
    {
        return SKE_SUCCESS;
    }
    else if ((NULL == ctx) || (NULL == tmp_out))
    {
        return SKE_BUFFER_NULL;
    }
    else if (msg_words & (ctx->ske_cmac_ctx->block_words - 1))
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
 * @brief           ske_lp cmac dma style update message including the last block(or message tail), and get the mac
 * @param[in]       ctx                  - ske_cmac_dma_ctx_t context pointer
 * @param[in]       msg                  - message including the last block(or message tail)
 * @param[in]       msg_bytes            - byte length of msg, could be 0
 * @param[out]      tmp_out              - temporary output ciphertext, ske_lp need to output, it occupies the same blocks as the
 * @param[out]      mac                  - cmac, occupies a block
 * @param[in]       callback             - callback function pointer, this could be NULL, means doing nothing
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. if the whole message length is 0, this case is supported. in this case, msg occupies a block, and
 *           please set msg_bytes to 0
 */
unsigned int ske_lp_dma_cmac_update_including_last_block(ske_cmac_dma_ctx_t *ctx, unsigned int *msg, unsigned int msg_bytes, unsigned int *tmp_out, unsigned int *mac,
                                                         SKE_CALLBACK callback)
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

    remainder_bytes = msg_bytes % ctx->ske_cmac_ctx->block_bytes;
    block_num = msg_bytes / ctx->ske_cmac_ctx->block_bytes;
    ;

    if ((0 == remainder_bytes) && (0 != msg_bytes))
    {
        ret = ske_lp_dma_update_blocks(msg, tmp_out, (block_num - 1) * ctx->ske_cmac_ctx->block_words, callback);
        if (SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            ;
        }

        // last block
        uint32_xor(msg + (block_num - 1) * ctx->ske_cmac_ctx->block_words, (unsigned int *)ctx->k1, tmp_out, ctx->ske_cmac_ctx->block_words);
        ret = ske_lp_dma_update_blocks(tmp_out, tmp_out, ctx->ske_cmac_ctx->block_words, callback);
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
        ret = ske_lp_dma_update_blocks(msg, tmp_out, block_num * ctx->ske_cmac_ctx->block_words, callback);
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
        memset_((unsigned char *)msg + msg_bytes + 1, 0, ctx->ske_cmac_ctx->block_bytes - 1 - remainder_bytes);

        uint32_xor(msg + block_num * ctx->ske_cmac_ctx->block_words, (unsigned int *)ctx->k2, tmp_out, ctx->ske_cmac_ctx->block_words);
        ret = ske_lp_dma_update_blocks(tmp_out, tmp_out, ctx->ske_cmac_ctx->block_words, callback);
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
    memcpy_(mac, (unsigned char *)tmp_out, ctx->mac_bytes);

    return SKE_SUCCESS;
}

/**
 * @brief           ske_lp cmac(DMA style, one-off style)
 * @param[in]       alg                  - ske_lp algorithm
 * @param[in]       key                  - key in byte buffer style
 * @param[in]       sp_key_idx           - index of secure port key, (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX],
 * @param[in]       msg                  - message
 * @param[in]       msg_bytes            - byte length of message
 * @param[out]      tmp_out              - temporary output ciphertext, ske_lp need to output, it occupies the same blocks as the
 * @param[out]      mac                  - mac
 * @param[in]       mac_bytes            - byte length of mac
 * @param[in]       callback             - callback function pointer, this could be NULL, means doing nothing
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. if key is from user input, please make sure key is not NULL(now sp_key_idx is useless),
 *           otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX]
 *        2. msg_bytes is actual byte length of message, it could be any value(including 0).
 *           (1). if msg_bytes is not 0, msg must have (msg_bytes+15)/16 blocks, if the last block is not full,
 *           please pad with zero.
 *           (2). if msg_bytes is 0, msg occupies a block
 */
unsigned int ske_lp_dma_cmac(ske_alg_e alg, const unsigned char *key, unsigned short sp_key_idx, unsigned int *msg, unsigned int msg_bytes, unsigned int *tmp_out,
                             unsigned int *mac, unsigned char mac_bytes, SKE_CALLBACK callback)
{
    ske_cmac_dma_ctx_t ctx[1];
    unsigned int ret;

    ret = ske_lp_dma_cmac_init(ctx, alg, key, sp_key_idx, mac_bytes);
    if (SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    return ske_lp_dma_cmac_update_including_last_block(ctx, msg, msg_bytes, tmp_out, mac, callback);
}
#endif

#endif
