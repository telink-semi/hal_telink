/********************************************************************************************************
 * @file    ske_aes_xcbc_mac_96.c
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
#include "lib/include/ske/ske.h"
#include "lib/include/crypto_common/utility.h"
#include "lib/include/ske/ske_aes_xcbc_mac_96.h"


#ifdef SUPPORT_SKE_AES_XCBC_MAC_96

/**
 * @brief       ske_lp get aes_xcbc_mac_96 k1, k2 and k3.
 * @param[in]   ctx              - SKE_CCM_CTX context pointer.
 * @param[in]   key              - AES128 key in bytes.
 * @param[in]   sp_key_idx       - index of secure port key, (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX],
 *                                 if the MSB(sp_key_idx) is 1, that means using low 128bit of the 256bit key.
 * @param[out]  k1               - aes_xcbc_mac_96 k1.
 * @param[out]  k2               - aes_xcbc_mac_96 k2.
 * @param[out]  k3               - aes_xcbc_mac_96 k3.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. if key is from user input, please make sure key is not NULL(now sp_key_idx is useless),
 *        otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX]
  @endverbatim
 */
unsigned int ske_lp_aes_xcbc_mac_96_get_k1_k2_k3(SKE_CTX *ctx, unsigned char *key, unsigned short sp_key_idx, unsigned char k1[16], unsigned char k2[16], unsigned char k3[16])
{
    unsigned int ret;

    ret = ske_lp_init_internal(ctx, SKE_ALG_AES_128, SKE_MODE_ECB, SKE_CRYPTO_ENCRYPT, key, sp_key_idx, NULL, SKE_LP_DMA_DISABLE);
    if (SKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    uint32_set((unsigned int *)k1, 0x02020202, 4);
    ret |= ske_lp_update_blocks_internal(ctx, (unsigned char *)k1, k2, 0x10);
    uint32_set((unsigned int *)k1, 0x03030303, 4);
    ret |= ske_lp_update_blocks_internal(ctx, (unsigned char *)k1, k3, 0x10);
    uint32_set((unsigned int *)k1, 0x01010101, 4);
    ret |= ske_lp_update_blocks_internal(ctx, (unsigned char *)k1, (unsigned char *)k1, 0x10);
    if (SKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    return SKE_SUCCESS;
}

/**
 * @brief       Initialize AES-XCBC-MAC-96 for CPU style (ske_lp).
 * @param[in]   ctx              - Pointer to the SKE_AES_XCBC_MAC_96_CTX context.
 * @param[in]   mac_action       - The action to be performed:
 *                                 must be either SKE_GENERATE_MAC or SKE_VERIFY_MAC.
 * @param[in]   key              - AES key in bytes.
 * @param[in]   sp_key_idx       - Index of the secure port key.
 *                                 (sp_key_idx & 0x7FFF) must be in the range [1, SKE_MAX_KEY_IDX].
 *                                 If the MSB of sp_key_idx is 1, the low 128 bits of the 256-bit key are used.
 * @return      SKE_SUCCESS(success), other(error)
 * @verbatim
 *      -# 1. If key is from user input, please make sure key is not NULL (now sp_key_idx is useless),
 *         otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in the range [1, SKE_MAX_KEY_IDX].
 * @endverbatim
 */
unsigned int ske_lp_aes_xcbc_mac_96_init(SKE_AES_XCBC_MAC_96_CTX *ctx, SKE_MAC mac_action, unsigned char *key, unsigned short sp_key_idx)
{
    unsigned int k1[4];
    unsigned int ret;

    //check and keep ctx->left_bytes = 0
    if (NULL == ctx) {
        return SKE_BUFFER_NULL;
    } else if (mac_action > SKE_VERIFY_MAC) {
        return SKE_INPUT_INVALID;
    } else {
        ;
    }

    ctx->left_bytes = 0;
    ctx->mac_action = mac_action;

    ske_lp_set_cpu_mode();

    ret = ske_lp_aes_xcbc_mac_96_get_k1_k2_k3(ctx->ske_xcbc_mac_ctx, key, sp_key_idx, (unsigned char *)k1, ctx->k2, ctx->k3);
    if (SKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //set iv zero
    uint32_clear((unsigned int *)ctx->block_buf, 4);

    ske_lp_set_cpu_mode(); //since use k1

    return ske_lp_init_internal(ctx->ske_xcbc_mac_ctx, SKE_ALG_AES_128, SKE_MODE_CBC, SKE_CRYPTO_ENCRYPT, (unsigned char *)k1, sp_key_idx, (unsigned char *)ctx->block_buf, SKE_LP_DMA_DISABLE);
}

/**
 * @brief       Update the message for AES-XCBC-MAC-96 (CPU style) operation.
 * @param[in]   ctx              - Pointer to the SKE_AES_XCBC_MAC_96_CTX context.
 * @param[in]   msg              - Input message.
 * @param[in]   msg_bytes        - The byte length of the message.
 * @return      SKE_SUCCESS(success), other(error)
 * @verbatim
 *      -# 1. msg_bytes can be any value.
 * @endverbatim
 */
unsigned int ske_lp_aes_xcbc_mac_96_update(SKE_AES_XCBC_MAC_96_CTX *ctx, unsigned char *msg, unsigned int msg_bytes)
{
    unsigned int  blocks_bytes;
    unsigned int  ret;
    unsigned char fill_bytes, remainder;

    if (NULL == ctx) {
        return SKE_BUFFER_NULL;
    } else if ((NULL == msg) || (0 == msg_bytes)) {
        return SKE_SUCCESS;
    } else {
        ;
    }

    //if one block left, process it
    if (ctx->ske_xcbc_mac_ctx->block_bytes == ctx->left_bytes) {
        ret = ske_lp_update_blocks_no_output_(ctx->ske_xcbc_mac_ctx, ctx->block_buf, ctx->ske_xcbc_mac_ctx->block_bytes);
        if (SKE_SUCCESS != ret) {
            return ret;
        } else {
            ctx->left_bytes = 0;
        }
    } else {
        ;
    }

    //padding
    if (ctx->left_bytes) {
        fill_bytes = ctx->ske_xcbc_mac_ctx->block_bytes - ctx->left_bytes;
        if (msg_bytes <= fill_bytes) {
            memcpy_(ctx->block_buf + ctx->left_bytes, msg, msg_bytes);
            ctx->left_bytes += msg_bytes;
            return SKE_SUCCESS;
        } else {
            memcpy_(ctx->block_buf + ctx->left_bytes, msg, fill_bytes);
            ret = ske_lp_update_blocks_no_output_(ctx->ske_xcbc_mac_ctx, ctx->block_buf, ctx->ske_xcbc_mac_ctx->block_bytes);
            if (SKE_SUCCESS != ret) {
                return ret;
            } else {
                ctx->left_bytes = 0;
                msg += fill_bytes;
                msg_bytes -= fill_bytes;
            }
        }
    } else {
        ;
    }

    //process some blocks
    blocks_bytes = (msg_bytes / ctx->ske_xcbc_mac_ctx->block_bytes) * ctx->ske_xcbc_mac_ctx->block_bytes;
    remainder    = msg_bytes % ctx->ske_xcbc_mac_ctx->block_bytes;

    //process remainder
    if (remainder) {
        ret = ske_lp_update_blocks_no_output_(ctx->ske_xcbc_mac_ctx, msg, blocks_bytes); //print_buf_U8(msg, blocks_bytes, "blocks_bytes");
        if (SKE_SUCCESS != ret) {
            return ret;
        } else {
            memcpy_(ctx->block_buf, msg + blocks_bytes, remainder);
            ctx->left_bytes = remainder;
        }
    } else {
        blocks_bytes -= ctx->ske_xcbc_mac_ctx->block_bytes;
        ret = ske_lp_update_blocks_no_output_(ctx->ske_xcbc_mac_ctx, msg, blocks_bytes);
        if (SKE_SUCCESS != ret) {
            return ret;
        } else {
            memcpy_(ctx->block_buf, msg + blocks_bytes, ctx->ske_xcbc_mac_ctx->block_bytes);
            ctx->left_bytes = ctx->ske_xcbc_mac_ctx->block_bytes;
        }
    }

    return SKE_SUCCESS;
}

/**
 * @brief       Finish the AES-XCBC-MAC-96 operation and get or verify the MAC (CPU style).
 * @param[in]   ctx            - Pointer to the SKE_AES_XCBC_MAC_96_CTX context.
 * @param[in]   mac            - Input for generating MAC, output for verifying MAC.
 * @return      SKE_SUCCESS(success), other(error)
 * @verbatim
 *      -# 1. If ctx->mac_action is SKE_GENERATE_MAC, mac is the output. If ctx->mac_action is SKE_VERIFY_MAC, mac is the input. A return value of SKE_SUCCESS means the MAC is valid; otherwise, the MAC is invalid.
 * @endverbatim
 */
unsigned int ske_lp_aes_xcbc_mac_96_final(SKE_AES_XCBC_MAC_96_CTX *ctx, unsigned char mac[12])
{
    unsigned int tmp[4];
    unsigned int ret;

    if ((NULL == ctx) || (NULL == mac)) {
        return SKE_BUFFER_NULL;
    } else {
        ;
    }

    if (ctx->ske_xcbc_mac_ctx->block_bytes == ctx->left_bytes) {
        uint32_XOR((unsigned int *)ctx->block_buf, (unsigned int *)ctx->k2, (unsigned int *)ctx->block_buf, ctx->ske_xcbc_mac_ctx->block_words);
        ret = ske_lp_update_blocks_internal_(ctx->ske_xcbc_mac_ctx, ctx->block_buf, (unsigned char *)tmp, ctx->ske_xcbc_mac_ctx->block_bytes);
        if (SKE_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }
    } else {
        ctx->block_buf[ctx->left_bytes] = 0x80;
        memset_(ctx->block_buf + ctx->left_bytes + 1, 0, ctx->ske_xcbc_mac_ctx->block_bytes - 1 - ctx->left_bytes);
        uint32_XOR((unsigned int *)ctx->block_buf, (unsigned int *)ctx->k3, (unsigned int *)ctx->block_buf, ctx->ske_xcbc_mac_ctx->block_words);
        ret = ske_lp_update_blocks_internal_(ctx->ske_xcbc_mac_ctx, ctx->block_buf, (unsigned char *)tmp, ctx->ske_xcbc_mac_ctx->block_bytes);
        if (SKE_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }
    }

    if (SKE_GENERATE_MAC == ctx->mac_action) {
        memcpy_(mac, tmp, 0x0C);
        ret = SKE_SUCCESS;
    } else {
        ret = memcmp_(mac, tmp, 0x0C);
    }

    return ret;
}

/**
 * @brief       Perform the AES-XCBC-MAC-96 operation (CPU style or one-off style).
 * @param[in]   mac_action       - The action to perform: must be either SKE_GENERATE_MAC or SKE_VERIFY_MAC.
 * @param[in]   key              - The key in bytes (user-provided or from secure port).
 * @param[in]   sp_key_idx       - Index of the secure port key. The value (sp_key_idx & 0x7FFF) must be in the range [1, SKE_MAX_KEY_IDX].
 *                                 If the MSB of sp_key_idx is set, the low 128 bits of a 256-bit key are used.
 * @param[in]   msg              - The message to process.
 * @param[in]   msg_bytes        - The byte length of the message.
 * @param[in]   mac              - Input for generating the MAC, output for verifying the MAC.
 * @return      SKE_SUCCESS(success), other(error)
 * @verbatim
 *      -# 1. If the key is from user input, ensure the key is not NULL. If the key is from the secure port,
 *         the value (sp_key_idx & 0x7FFF) must be in the range [1, SKE_MAX_KEY_IDX].
 *      -# 2. msg_bytes can be any value.
 *      -# 3. If mac_action is SKE_GENERATE_MAC, mac is the output. If mac_action is SKE_VERIFY_MAC,
 *         mac is the input. A return value of SKE_SUCCESS means the MAC is valid; otherwise, the MAC is invalid.
 * @endverbatim
 */
unsigned int ske_lp_aes_xcbc_mac_96(SKE_MAC mac_action, unsigned char *key, unsigned short sp_key_idx, unsigned char *msg, unsigned int msg_bytes, unsigned char mac[12])
{
    SKE_AES_XCBC_MAC_96_CTX ctx[1];
    unsigned int            ret;

    ret = ske_lp_aes_xcbc_mac_96_init(ctx, mac_action, key, sp_key_idx);
    if (SKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = ske_lp_aes_xcbc_mac_96_update(ctx, msg, msg_bytes);
    if (SKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    return ske_lp_aes_xcbc_mac_96_final(ctx, mac);
}


    #ifdef SKE_LP_DMA_FUNCTION
/**
 * @brief       Initialize the AES-XCBC-MAC-96 operation in DMA style.
 * @param[in]   ctx              - Pointer to the SKE_AES_XCBC_MAC_96_DMA_CTX context.
 * @param[in]   key              - The key in bytes (user-provided or from secure port).
 * @param[in]   sp_key_idx       - Index of the secure port key. The value (sp_key_idx & 0x7FFF) must be in the range [1, SKE_MAX_KEY_IDX].
 *                                 If the MSB of sp_key_idx is set, the low 128 bits of a 256-bit key are used.
 * @return      SKE_SUCCESS(success), other(error)
 * @verbatim
 *      -# 1. If the key is from user input, ensure the key is not NULL. If the key is from the secure port,
 *         the value (sp_key_idx & 0x7FFF) must be in the range [1, SKE_MAX_KEY_IDX].
 * @endverbatim
 */
unsigned int ske_lp_dma_aes_xcbc_mac_96_init(SKE_AES_XCBC_MAC_96_DMA_CTX *ctx, unsigned char *key, unsigned short sp_key_idx)
{
    unsigned int k1[4];
    unsigned int iv[4];
    unsigned int ret;

    if (NULL == ctx) {
        return SKE_BUFFER_NULL;
    } else {
        ;
    }

    ske_lp_set_cpu_mode();

    ret = ske_lp_aes_xcbc_mac_96_get_k1_k2_k3(ctx->ske_xcbc_mac_ctx, key, sp_key_idx, (unsigned char *)k1, ctx->k2, ctx->k3);
    if (SKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //set iv zero
    uint32_clear(iv, 4);

    return ske_lp_dma_init(SKE_ALG_AES_128, SKE_MODE_CBC, SKE_CRYPTO_ENCRYPT, (unsigned char *)k1, sp_key_idx, (unsigned char *)iv);
}

/**
 * @brief       Update the AES-XCBC-MAC-96 operation in DMA style with message blocks (excluding the last block or message tail).
 * @param[in]   ctx              - Pointer to the SKE_AES_XCBC_MAC_96_DMA_CTX context.
 * @param[in]   msg              - The message blocks to update, excluding the last block (or message tail).
 * @param[in]   msg_words        - The word length of msg, must be a multiple of the block word length.
 * @param[out]  tmp_out          - Temporary output ciphertext, occupies the same block structure as the input message msg.
 * @param[in]   callback         - Pointer to the callback function. Can be NULL, in which case no action is performed.
 * @return      SKE_SUCCESS(success), other(error)
 * @verbatim
 *      -# 1. The input message msg must consist of full blocks and exclude the last block (or message tail).
 * @endverbatim
 */
unsigned int ske_lp_dma_aes_xcbc_mac_96_update_blocks_excluding_last_block(SKE_AES_XCBC_MAC_96_DMA_CTX *ctx, unsigned int *msg, unsigned int msg_words, unsigned int *tmp_out, SKE_CALLBACK callback)
{
    if ((NULL == msg) || (0 == msg_words)) {
        return SKE_SUCCESS;
    } else if ((NULL == ctx) || (NULL == tmp_out)) {
        return SKE_BUFFER_NULL;
    } else if (msg_words & (ctx->ske_xcbc_mac_ctx->block_words - 1)) {
        return SKE_INPUT_INVALID;
    } else {
        ;
    }

    return ske_lp_dma_update_blocks(msg, tmp_out, msg_words, callback);
}

/**
 * @brief       Update the AES-XCBC-MAC-96 operation in DMA style, including the last block (or message tail), and generate the MAC.
 * @param[in]   ctx              - Pointer to the SKE_AES_XCBC_MAC_96_DMA_CTX context.
 * @param[in]   msg              - The message, including the last block (or message tail).
 * @param[in]   msg_bytes        - The byte length of msg, could be 0.
 * @param[out]  tmp_out          - Temporary output ciphertext, occupies the same block structure as the input msg.
 *                                 If the message length is 0, tmp_out must be one block.
 * @param[out]  mac              - The AES-XCBC-MAC-96 result, occupies 12 bytes.
 * @param[in]   callback         - Pointer to the callback function. Can be NULL, in which case no action is performed.
 * @return      SKE_SUCCESS(success), other(error)
 * @verbatim
 *      -# 1. If the whole message length is 0, the message must occupy one block, and msg_bytes should be set to 0.
 * @endverbatim
 */
unsigned int ske_lp_dma_aes_xcbc_mac_96_update_including_last_block(SKE_AES_XCBC_MAC_96_DMA_CTX *ctx, unsigned int *msg, unsigned int msg_bytes, unsigned int *tmp_out, unsigned int *mac, SKE_CALLBACK callback)
{
    unsigned int remainder_bytes;
    unsigned int block_num;
    unsigned int ret;

    if ((NULL == ctx) || (NULL == msg) || (NULL == tmp_out) || (NULL == mac)) {
        return SKE_BUFFER_NULL;
    } else {
        ;
    }

    remainder_bytes = msg_bytes % ctx->ske_xcbc_mac_ctx->block_bytes;
    block_num       = msg_bytes / ctx->ske_xcbc_mac_ctx->block_bytes;

    if ((0 == remainder_bytes) && (0 != msg_bytes)) {
        ret = ske_lp_dma_update_blocks(msg, tmp_out, (block_num - 1) * ctx->ske_xcbc_mac_ctx->block_words, callback);
        if (SKE_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }

        //last block
        uint32_XOR(msg + (block_num - 1) * ctx->ske_xcbc_mac_ctx->block_words, (unsigned int *)ctx->k2, tmp_out, ctx->ske_xcbc_mac_ctx->block_words);
        ret = ske_lp_dma_update_blocks(tmp_out, tmp_out, ctx->ske_xcbc_mac_ctx->block_words, callback);
        if (SKE_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }
    } else {
        ret = ske_lp_dma_update_blocks(msg, tmp_out, block_num * ctx->ske_xcbc_mac_ctx->block_words, callback);
        if (SKE_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }

        //last block
        memset_((unsigned char *)msg + msg_bytes, 0x80, 1);
        memset_((unsigned char *)msg + msg_bytes + 1, 0, ctx->ske_xcbc_mac_ctx->block_bytes - 1 - remainder_bytes);

        uint32_XOR(msg + block_num * ctx->ske_xcbc_mac_ctx->block_words, (unsigned int *)ctx->k3, tmp_out, ctx->ske_xcbc_mac_ctx->block_words);
        ret = ske_lp_dma_update_blocks(tmp_out, tmp_out, ctx->ske_xcbc_mac_ctx->block_words, callback);
        if (SKE_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }
    }

    ske_lp_dma_final();
    memcpy_(mac, (unsigned char *)tmp_out, 0x0C);

    return SKE_SUCCESS;
}

/**
 * @brief       Perform AES-XCBC-MAC-96 operation in both DMA style and one-off style.
 * @param[in]   key              - Input key in byte buffer style.
 * @param[in]   sp_key_idx       - Index of the secure port key. (sp_key_idx & 0x7FFF) must be in the range [1, SKE_MAX_KEY_IDX].
 *                                 If the MSB of sp_key_idx is 1, the low 128 bits of the 256-bit key will be used.
 * @param[in]   msg              - The message to be processed.
 * @param[in]   msg_bytes        - The byte length of the message.
 * @param[out]  tmp_out          - Temporary output ciphertext, which occupies the same block structure as the input message.
 *                                 If the message length is 0, tmp_out must occupy one block.
 * @param[out]  mac              - The generated MAC result.
 * @param[in]   callback         - Pointer to the callback function. Can be NULL, in which case no action is performed.
 * @return      SKE_SUCCESS(success), other(error)
 * @verbatim
 *      -# 1. If the key is provided by the user, ensure that the key is not NULL. In this case, sp_key_idx is ignored.
 *         Otherwise, the key is retrieved from the secure port, and (sp_key_idx & 0x7FFF) must be in the range [1, SKE_MAX_KEY_IDX].
 *      -# 2. msg_bytes represents the actual byte length of the message, which can be any value (including 0).
 *         (1). If msg_bytes is not 0, msg must consist of (msg_bytes + 15) / 16 blocks.
 *         (2). If msg_bytes is 0, msg occupies one block.
 * @endverbatim
 */
unsigned int ske_lp_dma_aes_xcbc_mac_96(unsigned char *key, unsigned short sp_key_idx, unsigned int *msg, unsigned int msg_bytes, unsigned int *tmp_out, unsigned int *mac, SKE_CALLBACK callback)
{
    SKE_AES_XCBC_MAC_96_DMA_CTX ctx[1];
    unsigned int                ret;

    ret = ske_lp_dma_aes_xcbc_mac_96_init(ctx, key, sp_key_idx);
    if (SKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    return ske_lp_dma_aes_xcbc_mac_96_update_including_last_block(ctx, msg, msg_bytes, tmp_out, mac, callback);
}
    #endif

#endif
