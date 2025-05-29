/********************************************************************************************************
 * @file    ske_cbc_mac.c
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
#include "lib/include/crypto_common/utility.h"
#include "lib/include/ske/ske_cbc_mac.h"


#ifdef SUPPORT_SKE_MODE_CBC_MAC


/**
 * @brief       ske_lp cbc mac internal init config
 *
 * @param[in]   ctx              - Pointer to the SKE_CBC_MAC_CTX context.
 * @param[in]   alg              - The SKE LP algorithm to be used.
 * @param[in]   key              - The key in bytes.
 * @param[in]   sp_key_idx       - Index of the secure port key.
 *                                  (sp_key_idx & 0x7FFF) must be within the range [1, SKE_MAX_KEY_IDX].
 *                                  If the MSB of sp_key_idx is 1, it indicates the use of the lower 128 bits of a 256-bit key.
 * @param[in]   mac_bytes        - MAC byte length. Must be greater than 1 and not exceed the block length.
 * @param[in]   dma_en           - DMA mode flag. Non-zero for DMA mode, zero for normal mode.
 * @return      SKE_SUCCESS(success), other(error)
 *
 * @verbatim
 *              -# 1.If key is from user input, make sure key is not NULL (in which case sp_key_idx is ignored),otherwise, if key is from secure port,(sp_key_idx & 0x7FFF)must be within [1, SKE_MAX_KEY_IDX].
 * @endverbatim
 */
unsigned int ske_lp_cbc_mac_init_internal(SKE_CBC_MAC_CTX *ctx, SKE_ALG alg, unsigned char *key, unsigned short sp_key_idx, unsigned char mac_bytes, unsigned int dma_en)
{
    unsigned int iv[4];

    //check and keep the mac length
    if (0 == mac_bytes || mac_bytes > ske_lp_get_block_byte_len(alg)) {
        return SKE_INPUT_INVALID; //SKE_ERROR;
    } else {
        ctx->mac_bytes = mac_bytes;
    }

    //set iv zero
    uint32_clear(iv, 4);

    if (dma_en) {
    #ifdef SKE_LP_DMA_FUNCTION
        return ske_lp_dma_init(alg, SKE_MODE_CBC, SKE_CRYPTO_ENCRYPT, key, sp_key_idx, (unsigned char *)iv);
    #endif
        return 0;
    } else {
        return ske_lp_init_internal(ctx->ske_cbc_mac_ctx, alg, SKE_MODE_CBC, SKE_CRYPTO_ENCRYPT, key, sp_key_idx, (unsigned char *)iv, SKE_LP_DMA_DISABLE);
    }
}

/**
 * @brief       ske_lp cbc mac init(CPU style)
 *
 * @param[in]   ctx              - Pointer to the SKE_CBC_MAC_CTX context.
 * @param[in]   alg              - The SKE LP algorithm to be used.
 * @param[in]   mac_action       - The MAC action to perform, must be SKE_GENERATE_MAC or SKE_VERIFY_MAC.
 * @param[in]   padding          - The SKE LP CBC MAC padding scheme, such as SKE_NO_PADDING or SKE_ZERO_PADDING.
 * @param[in]   key              - The key in bytes.
 * @param[in]   sp_key_idx       - Index of the secure port key.
 *                                  (sp_key_idx & 0x7FFF) must be within the range [1, SKE_MAX_KEY_IDX].
 *                                  If the MSB of sp_key_idx is 1, it indicates the use of the lower 128 bits of a 256-bit key.
 * @param[in]   mac_bytes        - MAC byte length. Must be greater than 1 and not exceed the block length.
 * @return      SKE_SUCCESS (success), other (error)
 *
 * @verbatim
 *             -# 1.If key is from user input, make sure key is not NULL (in which case sp_key_idx is ignored),
 *                otherwise, if key is from secure port, (sp_key_idx & 0x7FFF) must be within [1, SKE_MAX_KEY_IDX].
 * @endverbatim
 */
unsigned int ske_lp_cbc_mac_init(SKE_CBC_MAC_CTX *ctx, SKE_ALG alg, SKE_MAC mac_action, SKE_PADDING padding, unsigned char *key, unsigned short sp_key_idx, unsigned char mac_bytes)
{
    //check and keep the padding scheme and ctx->left_bytes = 0
    if (NULL == ctx) {
        return SKE_BUFFER_NULL;
    } else if (padding > SKE_ZERO_PADDING) {
        return SKE_INPUT_INVALID;
    } else if (mac_action > SKE_VERIFY_MAC) {
        return SKE_INPUT_INVALID;
    } else {
        ;
    }

    ctx->is_updated = 0;
    ctx->padding    = padding;
    ctx->left_bytes = 0;
    ctx->mac_action = mac_action;

    ske_lp_set_cpu_mode();

    return ske_lp_cbc_mac_init_internal(ctx, alg, key, sp_key_idx, mac_bytes, SKE_LP_DMA_DISABLE);
}

/**
 * @brief       ske_lp cbc_mac update message(CPU style)
 *
 * @param[in]   ctx              - Pointer to the SKE_CBC_MAC_CTX context.
 * @param[in]   msg              - The message.
 * @param[in]   msg_bytes        - Byte length of the message.
 * @return      SKE_SUCCESS (success), other (error)
 *
 * @verbatim
 *             -# msg_bytes could be any value.
 * @endverbatim
 */

unsigned int ske_lp_cbc_mac_update(SKE_CBC_MAC_CTX *ctx, unsigned char *msg, unsigned int msg_bytes)
{
    unsigned int  blocks_bytes;
    unsigned int  ret;
    unsigned char fill_bytes, remainder;

    if (NULL == ctx) {
        return SKE_BUFFER_NULL; //SKE_ERROR;
    } else {
        ;
    }

    if ((NULL == msg) || (0 == msg_bytes)) {
        return SKE_SUCCESS;
    } else {
        ctx->is_updated = 1;
    }

    if (ctx->left_bytes) {
        fill_bytes = ctx->ske_cbc_mac_ctx->block_bytes - ctx->left_bytes;
        if (msg_bytes < fill_bytes) {
            memcpy_(ctx->block_buf + ctx->left_bytes, msg, msg_bytes);
            ctx->left_bytes += msg_bytes;
            return SKE_SUCCESS;
        } else {
            memcpy_(ctx->block_buf + ctx->left_bytes, msg, fill_bytes);
            ret = ske_lp_update_blocks_no_output_(ctx->ske_cbc_mac_ctx, ctx->block_buf, ctx->ske_cbc_mac_ctx->block_bytes);
            if (SKE_SUCCESS != ret) {
                return ret;
            } else {
                ctx->left_bytes = 0;
                msg += fill_bytes;
                msg_bytes -= fill_bytes;
            }
        }
    }

    //update blocks
    blocks_bytes = (msg_bytes / ctx->ske_cbc_mac_ctx->block_bytes) * ctx->ske_cbc_mac_ctx->block_bytes;
    ret          = ske_lp_update_blocks_no_output_(ctx->ske_cbc_mac_ctx, msg, blocks_bytes);
    if (SKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //hold the remainder
    remainder = msg_bytes % ctx->ske_cbc_mac_ctx->block_bytes;
    if (remainder) {
        memcpy_(ctx->block_buf, msg + blocks_bytes, remainder);
        ctx->left_bytes = remainder;
    } else {
        ;
    }

    return SKE_SUCCESS;
}

/**
 * @brief       ske_lp cbc_mac finish, and get the mac(CPU style)
 *
 * @param[in]   ctx              - Pointer to the SKE_CBC_MAC_CTX context.
 * @param[in,out] mac            - Input (for verifying mac), output (for generating mac).
 * @return      SKE_SUCCESS (success), other (error)
 *
 * @verbatim
 *             -# 1. If ctx->mac_action is SKE_GENERATE_MAC, mac is output. If ctx->mac_action is SKE_VERIFY_MAC,
 *                mac is input, return value SKE_SUCCESS means the mac is valid, otherwise mac is invalid.
 *             -# 2. For the case that padding is SKE_NO_PADDING, if the total length of message is not a multiple of
 *                block length, it will return error.
 * @endverbatim
 */
unsigned int ske_lp_cbc_mac_final(SKE_CBC_MAC_CTX *ctx, unsigned char *mac)
{
    unsigned int tmp[4];
    unsigned int ret;

    if (NULL == ctx || NULL == mac) {
        return SKE_BUFFER_NULL;      //SKE_ERROR;
    } else if (0 == ctx->is_updated) //no input, it is not valid
    {
        return SKE_INPUT_INVALID;
    } else {
        ;
    }

    if (0 == ctx->left_bytes) {
        ske_lp_simple_get_output_block(tmp, ctx->ske_cbc_mac_ctx->block_words);
    } else {
        //ske_lp_set_last_block(1);

        if (SKE_NO_PADDING == ctx->padding) {
            return SKE_ERROR;
        } else if (SKE_ZERO_PADDING == ctx->padding) {
            memset_(ctx->block_buf + ctx->left_bytes, 0, ctx->ske_cbc_mac_ctx->block_bytes - ctx->left_bytes);
            ret = ske_lp_update_blocks_internal_(ctx->ske_cbc_mac_ctx, ctx->block_buf, (unsigned char *)tmp, ctx->ske_cbc_mac_ctx->block_bytes);
            if (SKE_SUCCESS != ret) {
                return ret;
            } else {
                ;
            }
        } else {
            return SKE_ERROR;
        }
    }

    if (SKE_GENERATE_MAC == ctx->mac_action) {
        memcpy_(mac, tmp, ctx->mac_bytes);
        ret = SKE_SUCCESS;
    } else {
        ret = memcmp_(mac, tmp, ctx->mac_bytes);
    }

    return ret;
}

/**
 * @brief       ske_lp cbc mac(CPU style, one-off style)
 *
 * @param[in]   alg              - The SKE LP algorithm to be used.
 * @param[in]   mac_action       - The MAC action to perform, must be SKE_GENERATE_MAC or SKE_VERIFY_MAC.
 * @param[in]   padding          - The SKE LP CBC MAC padding scheme, such as SKE_NO_PADDING or SKE_ZERO_PADDING.
 * @param[in]   key              - The key in bytes.
 * @param[in]   sp_key_idx       - Index of the secure port key.
 *                                  (sp_key_idx & 0x7FFF) must be within the range [1, SKE_MAX_KEY_IDX].
 *                                  If the MSB of sp_key_idx is 1, it indicates the use of the lower 128 bits of a 256-bit key.
 * @param[in]   msg              - The message.
 * @param[in]   msg_bytes        - Byte length of the message.
 * @param[in,out] mac            - Input (for verifying mac), output (for generating mac).
 * @param[in]   mac_bytes        - MAC byte length, must be greater than 1 and not exceed the block length.
 * @return      SKE_SUCCESS (success), other (error)
 *
 * @verbatim
 *             -# 1. If key is from user input, ensure key is not NULL (in which case sp_key_idx is ignored);
 *                otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1, SKE_MAX_KEY_IDX].
 *             -# 2. msg_bytes cannot be 0.
 *             -# 3. If mac_action is SKE_GENERATE_MAC, mac is output. If mac_action is SKE_VERIFY_MAC,
 *                mac is input, return value SKE_SUCCESS means the mac is valid, otherwise mac is invalid.
 *             -# 4. For the case that padding is SKE_NO_PADDING, if the total length of message is not a multiple of
 *                block length, it will return error.
 * @endverbatim
 */
unsigned int ske_lp_cbc_mac(SKE_ALG alg, SKE_MAC mac_action, SKE_PADDING padding, unsigned char *key, unsigned short sp_key_idx, unsigned char *msg, unsigned int msg_bytes, unsigned char *mac, unsigned char mac_bytes)
{
    unsigned int    ret;
    SKE_CBC_MAC_CTX ctx[1];

    ret = ske_lp_cbc_mac_init(ctx, alg, mac_action, padding, key, sp_key_idx, mac_bytes);
    if (SKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = ske_lp_cbc_mac_update(ctx, msg, msg_bytes);
    if (SKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    return ske_lp_cbc_mac_final(ctx, mac);
}


    #ifdef SKE_LP_DMA_FUNCTION

/**
 * @brief       ske_lp cbc mac dma style init
 *
 * @param[in]   ctx              - Pointer to the SKE_CBC_MAC_DMA_CTX context.
 * @param[in]   alg              - The SKE LP algorithm to be used.
 * @param[in]   key              - The key in bytes.
 * @param[in]   sp_key_idx       - Index of the secure port key.
 *                                  (sp_key_idx & 0x7FFF) must be within the range [1, SKE_MAX_KEY_IDX].
 *                                  If the MSB of sp_key_idx is 1, it indicates the use of the lower 128 bits of a 256-bit key.
 * @param[in]   mac_bytes        - MAC byte length, must be greater than 1 and not exceed the block length.
 * @return      SKE_SUCCESS (success), other (error)
 *
 * @verbatim
 *             -# 1. If key is from user input, ensure key is not NULL (in which case sp_key_idx is ignored);
 *                otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1, SKE_MAX_KEY_IDX].
 * @endverbatim
 */
unsigned int ske_lp_dma_cbc_mac_init(SKE_CBC_MAC_DMA_CTX *ctx, SKE_ALG alg, unsigned char *key, unsigned short sp_key_idx, unsigned char mac_bytes)
{
    if (NULL == ctx) {
        return SKE_BUFFER_NULL;
    } else {
        ;
    }

    //CBC_MAC DMA for 3DES is not supported.
    switch (alg) {
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

    ctx->ske_cbc_mac_ctx->block_bytes = ske_lp_get_block_byte_len(alg);
    ctx->ske_cbc_mac_ctx->block_words = ctx->ske_cbc_mac_ctx->block_bytes >> 2;

    return ske_lp_cbc_mac_init_internal((SKE_CBC_MAC_CTX *)ctx, alg, key, sp_key_idx, mac_bytes, SKE_LP_DMA_ENABLE);
}

/**
 * @brief       ske_lp cbc mac dma style update message blocks (excluding the last block, or the message tail)
 *
 * @param[in]   ctx              - Pointer to the SKE_CBC_MAC_DMA_CTX context.
 * @param[in]   msg              - Message of some blocks, excluding last block (or message tail).
 * @param[in]   msg_words        - Word length of msg, must be a multiple of block word length.
 * @param[out]  tmp_out          - Temporary output ciphertext, SKE LP needs to output, it occupies the same blocks as the input msg.
 * @param[in]   callback         - Callback function pointer, this could be NULL, meaning doing nothing.
 * @return      SKE_SUCCESS (success), other (error)
 *
 * @verbatim
 *             -# 1. The input msg must be some blocks, and excludes the last block (or message tail).
 * @endverbatim
 */
unsigned int ske_lp_dma_cbc_mac_update_blocks_excluding_last_block(SKE_CBC_MAC_DMA_CTX *ctx, unsigned int *msg, unsigned int msg_words, unsigned int *tmp_out, SKE_CALLBACK callback)
{
    if ((NULL == msg) || (0 == msg_words)) {
        return SKE_SUCCESS;
    } else if ((NULL == ctx) || (NULL == tmp_out)) {
        return SKE_BUFFER_NULL;
    } else if (msg_words & (ctx->ske_cbc_mac_ctx->block_words - 1)) {
        return SKE_INPUT_INVALID;
    } else {
        ;
    }

    return ske_lp_dma_update_blocks(msg, tmp_out, msg_words, callback);
}

/**
 * @brief       ske_lp cbc mac dma style update message including the last block (or message tail), and get the mac
 *
 * @param[in]   ctx              - Pointer to the SKE_CBC_MAC_DMA_CTX context.
 * @param[in]   msg              - Message including the last block (or message tail).
 * @param[in]   msg_bytes        - Byte length of msg, cannot be 0.
 * @param[out]  tmp_out          - Temporary output ciphertext, SKE LP needs to output, it occupies the same blocks as the input msg.
 * @param[out]  mac              - Output, CBC_MAC, occupies a block.
 * @param[in]   callback         - Callback function pointer, this could be NULL, meaning doing nothing.
 * @return      SKE_SUCCESS (success), other (error)
 *
 * @verbatim
 *             -# 1. If the actual message length msg_bytes is not a multiple of block length, please ensure the
 *                last block (or message tail) is padded with 0 already.
 * @endverbatim
 */
unsigned int ske_lp_dma_cbc_mac_update_including_last_block(SKE_CBC_MAC_DMA_CTX *ctx, unsigned int *msg, unsigned int msg_bytes, unsigned int *tmp_out, unsigned int *mac, SKE_CALLBACK callback)
{
    unsigned int msg_words;
    unsigned int ret;

    if ((NULL == ctx) || (NULL == msg) || (NULL == tmp_out) || (NULL == mac)) {
        return SKE_BUFFER_NULL;
    } else if (0 == msg_bytes) {
        return SKE_INPUT_INVALID;
    } else {
        ;
    }

    msg_words = (msg_bytes + ctx->ske_cbc_mac_ctx->block_bytes - 1) / ctx->ske_cbc_mac_ctx->block_bytes;
    msg_words *= ctx->ske_cbc_mac_ctx->block_words;

    ret = ske_lp_dma_update_blocks(msg, tmp_out, msg_words, callback);
    if (SKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    memcpy_(mac, tmp_out + msg_words - ctx->ske_cbc_mac_ctx->block_words, ctx->mac_bytes);
    ske_lp_dma_final();

    return SKE_SUCCESS;
}

/**
 * @brief       ske_lp cbc_mac (DMA style, one-off style)
 *
 * @param[in]   alg              - The SKE LP algorithm to be used.
 * @param[in]   key              - The key in byte buffer style.
 * @param[in]   sp_key_idx       - Index of the secure port key.
 *                                  (sp_key_idx & 0x7FFF) must be within the range [1, SKE_MAX_KEY_IDX].
 *                                  If the MSB of sp_key_idx is 1, it indicates the use of the lower 128 bits of a 256-bit key.
 * @param[in]   msg              - The message.
 * @param[in]   msg_bytes        - Byte length of message, cannot be 0.
 * @param[out]  tmp_out          - Temporary output ciphertext, SKE LP needs to output, it occupies the same blocks as the input msg.
 * @param[out]  mac              - Output, CBC MAC.
 * @param[in]   mac_bytes        - MAC byte length, must be greater than 1 and not exceed the block length.
 * @param[in]   callback         - Callback function pointer, this could be NULL, meaning doing nothing.
 * @return      SKE_SUCCESS (success), other (error)
 *
 * @verbatim
 *             -# 1. If key is from user input, ensure key is not NULL (in which case sp_key_idx is ignored);
 *                otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1, SKE_MAX_KEY_IDX].
 *             -# 2. msg_bytes cannot be 0.
 *             -# 3. If the actual message length msg_bytes is not a multiple of block length, please ensure the last block
 *                is padded with 0 already.
 * @endverbatim
 */
unsigned int ske_lp_dma_cbc_mac(SKE_ALG alg, unsigned char *key, unsigned short sp_key_idx, unsigned int *msg, unsigned int msg_bytes, unsigned int *tmp_out, unsigned int *mac, unsigned int mac_bytes, SKE_CALLBACK callback)
{
    SKE_CBC_MAC_DMA_CTX ctx[1];
    unsigned int        ret;

    ret = ske_lp_dma_cbc_mac_init(ctx, alg, key, sp_key_idx, mac_bytes);
    if (SKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    return ske_lp_dma_cbc_mac_update_including_last_block(ctx, msg, msg_bytes, tmp_out, mac, callback);
}

    #endif

#endif
