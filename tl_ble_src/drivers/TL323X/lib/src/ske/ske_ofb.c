/********************************************************************************************************
 * @file    ske_ofb.c
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
#include "lib/include/ske/ske_ofb.h"

#ifdef SUPPORT_SKE_MODE_XTS

/**
 * @brief           Initializes SKE XTS mode configuration
 * @param[in]       ctx                  - ske_xts_ctx_t context pointer
 * @param[in]       alg                  - SKE algorithm
 * @param[in]       crypto               - Encrypt or decrypt operation
 * @param[in]       key                  - Key in bytes, key = key1||key2
 * @param[in]       sp_key_idx           - Index of secure port key
 * @param[in]       i                    - I value, same length as block length
 * @param[in]       c_bytes              - Byte length of plaintext/ciphertext
 * @return          SKE_SUCCESS if successful, otherwise error code
 * @note
 *        1. If key is from user input, ensure key is not NULL
 *        2. Key consists of key1 and key2
 *        3. c_bytes cannot be less than block byte length
 */
unsigned int ske_lp_xts_init(ske_xts_ctx_t *ctx, ske_alg_e alg, ske_crypto_e crypto, const unsigned char *key, unsigned short sp_key_idx, unsigned char *i, unsigned int c_bytes)
{
    unsigned int ret;

    if (NULL == ctx)
    {
        return SKE_BUFFER_NULL;
    }
    else if (ske_lp_get_block_byte_len(alg) != 16)
    {
        return SKE_INPUT_INVALID;
    }
    else if (c_bytes < 16)
    {
        return SKE_INPUT_INVALID;
    }
    else
    {
        ;
    }

    ctx->crypto = crypto;
    ctx->c_bytes = c_bytes;
    ctx->current_bytes = 0;

    ske_lp_set_cpu_mode();

    ret = ske_lp_init_internal(ctx->ske_xts_ctx, alg, SKE_MODE_ECB, SKE_CRYPTO_ENCRYPT, key + ske_lp_get_key_byte_len(alg), sp_key_idx, NULL, SKE_LP_DMA_DISABLE);
    if (SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    ret = ske_lp_update_blocks_internal(ctx->ske_xts_ctx, i, ctx->t, 16);
    if (SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    ske_lp_set_cpu_mode();

    return ske_lp_init_internal(ctx->ske_xts_ctx, alg, SKE_MODE_ECB, crypto, key, sp_key_idx, NULL, SKE_LP_DMA_DISABLE);
}

/**
 * @brief           Updates SKE XTS mode T value
 * @param[in]       ctx                  - ske_xts_ctx_t context pointer
 * @return          None
 * @note            Must be called after calling ske_lp_xts_init()
 */
void ske_lp_xts_update_t(ske_xts_ctx_t *ctx)
{
    unsigned int i;
    unsigned char flag;

    flag = 0;
    if (ctx->t[15] & 0x80)
    {
        flag = 1;
    }
    else
    {
        ;
    }

    for (i = 15; i > 0; i--)
    {
        ctx->t[i] <<= 1;
        ctx->t[i] |= ctx->t[i - 1] >> 7;
    }
    ctx->t[0] <<= 1;

    if (flag)
    {
        ctx->t[0] ^= 0x87;
    }
    else
    {
        ;
    }
}

/**
 * @brief           Updates one block in SKE XTS mode
 * @param[in]       ctx                  - ske_xts_ctx_t context pointer
 * @param[in]       in                   - Input data block
 * @param[out]      out                  - Output data block
 * @return          SKE_SUCCESS if successful, otherwise error code
 * @note            Must be called after calling ske_lp_xts_init()
 */
unsigned int ske_lp_xts_update_one_block(ske_xts_ctx_t *ctx, unsigned char in[16], unsigned char out[16])
{
    unsigned char tmp[16];
    unsigned int i, ret;

    for (i = 0; i < 16; i++)
    {
        tmp[i] = in[i] ^ ctx->t[i];
    }

    ret = ske_lp_update_blocks_internal(ctx->ske_xts_ctx, tmp, out, 16);
    if (SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    for (i = 0; i < 16; i++)
    {
        out[i] = out[i] ^ ctx->t[i];
    }

    return SKE_SUCCESS;
}

/**
 * @brief           Updates multiple blocks in SKE XTS mode
 * @param[in]       ctx                  - ske_xts_ctx_t context pointer
 * @param[in]       in                   - Input data
 * @param[out]      out                  - Output data
 * @param[in]       bytes                - Byte length of input/output data
 * @return          SKE_SUCCESS if successful, otherwise error code
 * @note
 *        1. in and out can be the same buffer
 *        2. bytes must be a multiple of block byte length
 *        3. For partial blocks, use ske_lp_xts_update_including_last_2_blocks()
 */
unsigned int ske_lp_xts_update_blocks(ske_xts_ctx_t *ctx, unsigned char *in, unsigned char *out, unsigned int bytes)
{
    unsigned int i, ret;

    if ((NULL == ctx) || (NULL == in) || (NULL == out))
    {
        return SKE_BUFFER_NULL;
    }
    else if (bytes & (ctx->ske_xts_ctx->block_bytes - 1))
    {
        return SKE_INPUT_INVALID;
    }
    else if (ctx->current_bytes & (ctx->ske_xts_ctx->block_bytes - 1))
    {
        return SKE_INPUT_INVALID;
    }
    else
    {
        ;
    }

    if (ctx->c_bytes & 0x0F)
    {
        if (ctx->c_bytes - 16 - (ctx->c_bytes & 0x0F) < ctx->current_bytes + bytes)
        {
            return SKE_INPUT_INVALID;
        }
        else
        {
            ;
        }
    }
    else
    {
        if (ctx->c_bytes < ctx->current_bytes + bytes)
        {
            return SKE_INPUT_INVALID;
        }
        else
        {
            ;
        }
    }

    for (i = 0; i < bytes; i += 16)
    {
        ret = ske_lp_xts_update_one_block(ctx, in, out);
        if (SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            ;
        }

        in += 16;
        out += 16;
        ske_lp_xts_update_t(ctx);
    }

    ctx->current_bytes += bytes;

    return SKE_SUCCESS;
}

/**
 * @brief           Updates including last 2 blocks in SKE XTS mode
 * @param[in]       ctx                  - ske_xts_ctx_t context pointer
 * @param[in]       in                   - Input data
 * @param[out]      out                  - Output data
 * @param[in]       bytes                - Byte length of input/output data
 * @return          SKE_SUCCESS if successful, otherwise error code
 * @note
 *        1. in and out can be the same buffer
 *        2. Handles cases where ctx->c_bytes % 16 is not 0
 */
unsigned int ske_lp_xts_update_including_last_2_blocks(ske_xts_ctx_t *ctx, unsigned char *in, unsigned char *out, unsigned int bytes)
{
    unsigned char tmp[16], tmp2[16], tmp_t[16];
    unsigned int blocks_bytes;
    unsigned int ret;

    if (NULL == ctx || NULL == in || NULL == out)
    {
        return SKE_BUFFER_NULL;
    }
    else if (bytes <= ctx->ske_xts_ctx->block_bytes || !(bytes & 0x0F))
    {
        return SKE_INPUT_INVALID;
    }
    else if (ctx->current_bytes & (ctx->ske_xts_ctx->block_bytes - 1) || ctx->current_bytes + bytes != ctx->c_bytes)
    {
        return SKE_INPUT_INVALID;
    }
    else
    {
        ;
    }

    // process blocks
    blocks_bytes = (bytes & (~0x0F)) - ctx->ske_xts_ctx->block_bytes;
    if (blocks_bytes)
    {
        ret = ske_lp_xts_update_blocks(ctx, in, out, blocks_bytes);
        if (SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            ;
        }

        in += blocks_bytes;
        out += blocks_bytes;
        bytes -= blocks_bytes;
    }
    else
    {
        ;
    }

    // process remainder 2 blocks
    bytes &= 15;
    if (SKE_CRYPTO_ENCRYPT == ctx->crypto)
    {
        ret = ske_lp_xts_update_one_block(ctx, in, tmp2);
        if (SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            ;
        }

        ske_lp_xts_update_t(ctx);

        memcpy_(tmp, in + 16, bytes);
        memcpy_(out + 16, tmp2, bytes);
        memcpy_(tmp + bytes, tmp2 + bytes, 16 - bytes);

        ret = ske_lp_xts_update_one_block(ctx, tmp, out);
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
        memcpy_(tmp_t, ctx->t, 16);
        ske_lp_xts_update_t(ctx);

        ret = ske_lp_xts_update_one_block(ctx, in, tmp);
        if (SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            ;
        }

        memcpy_(tmp2, in + 16, bytes);
        memcpy_(out + 16, tmp, bytes);
        memcpy_(tmp2 + bytes, tmp + bytes, 16 - bytes);

        memcpy_(ctx->t, tmp_t, 16);
        ret = ske_lp_xts_update_one_block(ctx, tmp2, out);
        if (SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            ;
        }
    }

    return SKE_SUCCESS;
}

/**
 * @brief           Finalizes SKE XTS mode operation
 * @param[in]       ctx                  - ske_xts_ctx_t context pointer
 * @return          SKE_SUCCESS if successful, otherwise error code
 * @note            This is the last step of XTS calling, and it is optional
 */
unsigned int ske_lp_xts_final(ske_xts_ctx_t *ctx)
{
    memset_(ctx, 0, sizeof(ske_xts_ctx_t));

    return SKE_SUCCESS;
}

/**
 * @brief           Performs SKE XTS mode encryption or decryption
 * @param[in]       alg                  - SKE algorithm
 * @param[in]       crypto               - Encrypt or decrypt operation
 * @param[in]       key                  - Key in bytes, key = key1||key2
 * @param[in]       sp_key_idx           - Index of secure port key
 * @param[in]       i                    - I value, same length as block length
 * @param[in]       in                   - Input data
 * @param[out]      out                  - Output data
 * @param[in]       c_bytes              - Byte length of input/output data
 * @return          SKE_SUCCESS if successful, otherwise error code
 * @note
 *        1. If key is from user input, ensure key is not NULL
 *        2. Key consists of key1 and key2
 *        3. c_bytes cannot be less than block byte length
 */
unsigned int ske_lp_xts_crypto(ske_alg_e alg, ske_crypto_e crypto, const unsigned char *key, unsigned short sp_key_idx, unsigned char *i, unsigned char *in, unsigned char *out,
                               unsigned int c_bytes)
{
    ske_xts_ctx_t ctx[1];
    unsigned int ret;

    ret = ske_lp_xts_init(ctx, alg, crypto, key, sp_key_idx, i, c_bytes);
    if (SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    if (c_bytes & 0x0F)
    {
        ret = ske_lp_xts_update_including_last_2_blocks(ctx, in, out, c_bytes);
    }
    else
    {
        ret = ske_lp_xts_update_blocks(ctx, in, out, c_bytes);
    }

    if (SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    return ske_lp_xts_final(ctx);
}

#endif
