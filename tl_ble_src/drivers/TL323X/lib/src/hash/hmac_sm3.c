/********************************************************************************************************
 * @file    hmac_sm3.c
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
#include "lib/include/hash/hmac_sm3.h"

#ifdef SUPPORT_HASH_SM3

/**
 * @brief           init hmac-sm3
 * @param[in]       ctx                  - hmac_sm3_ctx_t context pointer
 * @param[in]       key                  - key
 * @param[in]       sp_key_idx           - index of secure port key
 * @param[in]       key_bytes            - byte length of key, it could be 0
 * @return          HASH_SUCCESS(success), other(error)
 */
unsigned int hmac_sm3_init(hmac_sm3_ctx_t *ctx, const unsigned char *key, unsigned short sp_key_idx, unsigned int key_bytes)
{
    return hmac_init(ctx, HASH_SM3, key, sp_key_idx, key_bytes);
}

/**
 * @brief           hmac-sm3 update message
 * @param[in]       ctx                  - hmac_sm3_ctx_t context pointer
 * @param[in]       msg                  - message
 * @param[in]       msg_len            - byte length of the input message
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the three parameters are valid, and ctx is initialize
 */
unsigned int hmac_sm3_update(hmac_sm3_ctx_t *ctx, const unsigned char *msg, unsigned int msg_len)
{
    return hmac_update(ctx, msg, msg_len);
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
unsigned int hmac_sm3_final(hmac_sm3_ctx_t *ctx, unsigned char *mac)
{
    return hmac_final(ctx, mac);
}

/**
 * @brief           input key and whole message, get the hmac
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
unsigned int hmac_sm3(const unsigned char *key, unsigned short sp_key_idx, unsigned int key_bytes, const unsigned char *msg, unsigned int msg_len, unsigned char *mac)
{
    return hmac(HASH_SM3, key, sp_key_idx, key_bytes, msg, msg_len, mac);
}

#ifdef SUPPORT_HASH_NODE
/**
 * @brief           input key and whole message, get the hmac(node style)
 * @param[in]       key                  - key
 * @param[in]       sp_key_idx           - index of secure port key
 * @param[in]       key_bytes            - byte length of the key
 * @param[in]       node                 - message node pointer
 * @param[in]       node_num             - number of hash nodes, i.e. number of message segments.
 * @param[out]      mac                  - hmac
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the mac buffer is sufficient
 *        2. if the whole message consists of some segments, every segment is a node, a node includes
 *           address and byte length
 */
unsigned int hmac_sm3_node_steps(const unsigned char *key, unsigned short sp_key_idx, unsigned int key_bytes, const hash_node_t *node, unsigned int node_num, unsigned char *mac)
{
    return hmac_node_steps(HASH_SM3, key, sp_key_idx, key_bytes, node, node_num, mac);
}
#endif

#ifdef HASH_DMA_FUNCTION
/**
 * @brief           init dma hmac-sm3
 * @param[in]       ctx                  - hmac_sm3_dma_ctx_t context pointer
 * @param[in]       key                  - key
 * @param[in]       sp_key_idx           - index of secure port key
 * @param[in]       key_bytes            - key byte length
 * @param[in]       callback             - callback function pointer
 * @return          HASH_SUCCESS(success), other(error)
 */
unsigned int hmac_sm3_dma_init(hmac_sm3_dma_ctx_t *ctx, const unsigned char *key, unsigned short sp_key_idx, unsigned int key_bytes, hash_callback callback)
{
    return hmac_dma_init(ctx, HASH_SM3, key, sp_key_idx, key_bytes, callback);
}

/**
 * @brief           dma hmac-sm3 update message
 * @param[in]       ctx                  - hmac_sm3_dma_ctx_t context pointer
 * @param[in]       msg                  - message
 * @param[in]       msg_len            - byte length of the input message, must be a multiple of block byte length
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the four parameters are valid, and ctx is initialize
 */
unsigned int hmac_sm3_dma_update_blocks(hmac_sm3_dma_ctx_t *ctx, unsigned int *msg, unsigned int msg_len)
{
    return hmac_dma_update_blocks(ctx, msg, msg_len);
}

/**
 * @brief           dma hmac-sm3 message update done, get the hmac
 * @param[in]       ctx                  - hmac_sm3_dma_ctx_t context pointer
 * @param[in]       remainder_msg        - message
 * @param[in]       remainder_bytes      - byte length of the remainder message
 * @param[out]      mac                  - hmac
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the three parameters are valid, and ctx is initialize
 */
unsigned int hmac_sm3_dma_final(hmac_sm3_dma_ctx_t *ctx, unsigned int *remainder_msg, unsigned int remainder_bytes, unsigned int *mac)
{
    return hmac_dma_final(ctx, remainder_msg, remainder_bytes, mac);
}

/**
 * @brief           dma hmac-sm3 input key and message, get the hmac
 * @param[in]       key                  - key
 * @param[in]       sp_key_idx           - index of secure port key
 * @param[in]       key_bytes            - key byte length
 * @param[in]       msg                  - message
 * @param[in]       msg_len            - byte length of the input message
 * @param[out]      mac                  - hmac
 * @param[in]       callback             - callback function pointer
 * @return          HASH_SUCCESS(success), other(error)
 */
unsigned int hmac_sm3_dma(const unsigned char *key, unsigned short sp_key_idx, unsigned int key_bytes, unsigned int *msg, unsigned int msg_len, unsigned int *mac,
                          hash_callback callback)
{
    return hmac_dma(HASH_SM3, key, sp_key_idx, key_bytes, msg, msg_len, mac, callback);
}

#ifdef SUPPORT_HASH_DMA_NODE
/**
 * @brief           dma hmac input key and message, get the hmac(node style)
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
 *        2. if the whole message consists of some segments, every segment is a node, a node includes
 *           address and byte length.
 *        3. for every node or segment except the last, its message length must be a multiple of block length
 */
unsigned int hmac_sm3_dma_node_steps(const unsigned char *key, unsigned short sp_key_idx, unsigned int key_bytes, const hash_dma_node_t *node, unsigned int node_num,
                                     unsigned int *mac, hash_callback callback)
{
    return hmac_dma_node_steps(HASH_SM3, key, sp_key_idx, key_bytes, node, node_num, mac, callback);
}
#endif

#endif

#endif
