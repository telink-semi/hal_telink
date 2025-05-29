/********************************************************************************************************
 * @file    hmac_sha256.c
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
#include "lib/include/hash/hmac_sha256.h"


#ifdef SUPPORT_HASH_SHA256
/**
 * @brief       init hmac-sha256
 * @param[in]   ctx                    - HMAC_SHA256_CTX context pointer.
 * @param[in]   key                    - key.
 * @param[in]   sp_key_idx             - index of secure port key.
 * @param[in]   key_bytes              - byte length of key, it could be 0.
 * @return      0:success     other:error
 */
unsigned int hmac_sha256_init(HMAC_SHA256_CTX *ctx, unsigned char *key, unsigned short sp_key_idx, unsigned int key_bytes)
{
    return hmac_init(ctx, HASH_SHA256, key, sp_key_idx, key_bytes);
}

/**
 * @brief       hmac-sha256 update message
 * @param[in]   ctx                    - HMAC_SHA256_CTX context pointer.
 * @param[in]   msg                    - message.
 * @param[in]   msg_bytes              - byte length of the input message.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the three parameters are valid, and ctx is initialized.
  @endverbatim
 */
unsigned int hmac_sha256_update(HMAC_SHA256_CTX *ctx, unsigned char *msg, unsigned int msg_bytes)
{
    return hmac_update(ctx, msg, msg_bytes);
}

/**
 * @brief       message update done, get the hmac
 * @param[in]   ctx                    - HMAC_CTX context pointer.
 * @param[in]   mac                    - hmac.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the ctx is valid and initialized.
      -# 2. please make sure the mac buffer is sufficient.
  @endverbatim
 */
unsigned int hmac_sha256_final(HMAC_SHA256_CTX *ctx, unsigned char *mac)
{
    return hmac_final(ctx, mac);
}

/**
 * @brief       input key and whole message, get the hmac
 *
 * @param[in]   key                    - key in word buffer.
 * @param[in]   sp_key_idx             - index of secure port key.
 * @param[in]   key_bytes              - byte length of the key.
 * @param[in]   msg                    - message.
 * @param[in]   msg_bytes              - byte length of the message.
 * @param[out]   mac                    - hmac.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the mac buffer is sufficient.
  @endverbatim
 */
unsigned int hmac_sha256(unsigned char *key, unsigned short sp_key_idx, unsigned int key_bytes, unsigned char *msg, unsigned int msg_bytes, unsigned char *mac)
{
    return hmac(HASH_SHA256, key, sp_key_idx, key_bytes, msg, msg_bytes, mac);
}


    #ifdef SUPPORT_HASH_NODE
/**
 * @brief       input key and whole message, get the hmac(node style).
 * @param[in]   key               - key.
 * @param[in]   sp_key_idx        - index of secure port key.
 * @param[in]   key_bytes         - key byte length.
 * @param[in]   node              - message node pointer
 * @param[in]   node_num          - number of hash nodes, i.e. number of message segments.
 * @param[out]   mac               - hmac.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the mac buffer is sufficient.
      -# 2. if the whole message consists of some segments, every segment is a node, a node includes
            address and byte length.
  @endverbatim
 */
unsigned int hmac_sha256_node_steps(unsigned char *key, unsigned short sp_key_idx, unsigned int key_bytes, HASH_NODE *node, unsigned int node_num, unsigned char *mac)
{
    return hmac_node_steps(HASH_SHA256, key, sp_key_idx, key_bytes, node, node_num, mac);
}
    #endif


    #ifdef HASH_DMA_FUNCTION
/**
 * @brief       init dma hmac-sha256
 * @param[in]   ctx                    - HMAC_SHA256_DMA_CTX context pointer.
 * @param[in]   key                    - key.
 * @param[in]   sp_key_idx             - index of secure port key.
 * @param[in]   key_bytes              - key byte length.
 * @param[in]   callback               - callback function pointer.
 * @return      0:success     other:error
 */
unsigned int hmac_sha256_dma_init(HMAC_SHA256_DMA_CTX *ctx, unsigned char *key, unsigned short sp_key_idx, unsigned int key_bytes, HASH_CALLBACK callback)
{
    return hmac_dma_init(ctx, HASH_SHA256, key, sp_key_idx, key_bytes, callback);
}

/**
 * @brief       dma hmac-sha256 update message
 * @param[in]   ctx                    - HMAC_CTX context pointer.
 * @param[in]   msg                    - message.
 * @param[in]   msg_bytes              - word length of the input message, must be a multiple of block word length
 *                                       of SHA256(16).
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the four parameters are valid, and ctx is initialized.
  @endverbatim
 */
unsigned int hmac_sha256_dma_update_blocks(HMAC_SHA256_DMA_CTX *ctx, unsigned int *msg, unsigned int msg_bytes)
{
    return hmac_dma_update_blocks(ctx, msg, msg_bytes);
}

/**
 * @brief       dma hmac-sha256 message update done, get the hmac
 * @param[in]   ctx                    - HMAC_SHA256_DMA_CTX context pointer.
 * @param[in]   remainder_msg          - message.
 * @param[in]   remainder_bytes        - byte length of the last message, must be in [0, BLOCK_BYTE_LEN-1],
 *                                       here BLOCK_BYTE_LEN is block byte length of SHA256(64).
 * @param[out]   mac                    - hmac
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the three parameters are valid, and ctx is initialized.
  @endverbatim
 */
unsigned int hmac_sha256_dma_final(HMAC_SHA256_DMA_CTX *ctx, unsigned int *remainder_msg, unsigned int remainder_bytes, unsigned int *mac)
{
    return hmac_dma_final(ctx, remainder_msg, remainder_bytes, mac);
}

/**
 * @brief       dma hmac-sha256 input key and message, get the hmac
 * @param[in]   key                    - key.
 * @param[in]   sp_key_idx             - index of secure port key.
 * @param[in]   key_bytes              - key byte length.
 * @param[in]   msg                    - message.
 * @param[in]   msg_bytes              - byte length of the input message.
 * @param[out]   mac                    - hmac.
 * @param[in]   callback               - callback function pointer.
 * @return      0:success     other:error
 */
unsigned int hmac_sha256_dma(unsigned char *key, unsigned short sp_key_idx, unsigned int key_bytes, unsigned int *msg, unsigned int msg_bytes, unsigned int *mac, HASH_CALLBACK callback)
{
    return hmac_dma(HASH_SHA256, key, sp_key_idx, key_bytes, msg, msg_bytes, mac, callback);
}


        #ifdef SUPPORT_HASH_DMA_NODE
/**
 * @brief       dma hmac input key and message, get the hmac(node style).
 * @param[in]   key               - key.
 * @param[in]   sp_key_idx        - index of secure port key.
 * @param[in]   key_bytes         - key byte length.
 * @param[in]   node              - message node pointer
 * @param[in]   node_num          - number of hash nodes, i.e. number of message segments.
 * @param[out]   mac               - hmac.
 * @param[in]   callback          - callback function pointer
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the mac buffer is sufficient.
      -# 2. if the whole message consists of some segments, every segment is a node, a node includes
            address and byte length.
      -# 3. for every node or segment except the last, its message length must be a multiple of block length.
  @endverbatim
 */
unsigned int hmac_sha256_dma_node_steps(unsigned char *key, unsigned short sp_key_idx, unsigned int key_bytes, HASH_DMA_NODE *node, unsigned int node_num, unsigned int *mac, HASH_CALLBACK callback)
{
    return hmac_dma_node_steps(HASH_SHA256, key, sp_key_idx, key_bytes, node, node_num, mac, callback);
}
        #endif

    #endif


#endif
