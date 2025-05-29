/********************************************************************************************************
 * @file    sha512_224.c
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
#include "lib/include/hash/sha512_224.h"

#ifdef SUPPORT_HASH_SHA512_224

/**
 * @brief           init sha512_224
 * @param[in]       ctx                  - sha512_224_ctx_t context pointer
 * @return          HASH_SUCCESS(success), other(error)
 */
unsigned int sha512_224_init(sha512_224_ctx_t *ctx)
{
    return hash_init(ctx, HASH_SHA512_224);
}

/**
 * @brief           sha512_224 update message
 * @param[in]       ctx                  - sha512_224_ctx_t context pointer
 * @param[in]       msg                  - message
 * @param[in]       msg_len            - byte length of the input message
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the three parameters are valid, and ctx is initialize
 */
unsigned int sha512_224_update(sha512_224_ctx_t *ctx, const unsigned char *msg, unsigned int msg_len)
{
    return hash_update(ctx, msg, msg_len);
}

/**
 * @brief           message update done, get the sha512_224 digest
 * @param[out]      digest               - sha512_224 digest, 28 bytes
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the digest buffer is sufficient
 */
unsigned int sha512_224_final(sha512_224_ctx_t *ctx, unsigned char *digest)
{
    return hash_final(ctx, digest);
}

/**
 * @brief           input whole message and get its sha512_224 digest
 * @param[in]       msg                  - message
 * @param[in]       msg_len            - byte length of the input message, it could be 0
 * @param[out]      digest               - sha512_224 digest, 28 bytes
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the digest buffer is sufficient
 */
unsigned int sha512_224(const unsigned char *msg, unsigned int msg_len, unsigned char *digest)
{
    return hash(HASH_SHA512_224, msg, msg_len, digest);
}

#ifdef SUPPORT_HASH_NODE
/**
 * @brief           input whole message and get its sha512_224 digest(node style)
 * @param[in]       node                 - message node pointer
 * @param[in]       node_num             - number of hash nodes, i.e. number of message segments.
 * @param[out]      digest               - sha512_224 digest, 28 bytes
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the digest buffer is sufficient
 *        2. if the whole message consists of some segments, every segment is a node, a node includes
 *           address and byte length
 */
unsigned int sha512_224_node_steps(const hash_node_t *node, unsigned int node_num, unsigned char *digest)
{
    return hash_node_steps(HASH_SHA512_224, node, node_num, digest);
}
#endif

#ifdef HASH_DMA_FUNCTION
/**
 * @brief           init dma sha512_224
 * @param[in]       ctx                  - sha512_224_ctx_dma_t context pointer
 * @param[in]       callback             - callback function pointer
 * @return          HASH_SUCCESS(success), other(error)
 */
unsigned int sha512_224_dma_init(sha512_224_ctx_dma_t *ctx, hash_callback callback)
{
    return hash_dma_init(ctx, HASH_SHA512_224, callback);
}

/**
 * @brief           dma sha512_224 update some message blocks
 * @param[in]       ctx                  - sha512_224_ctx_dma_t context pointer
 * @param[in]       msg                  - message blocks
 * @param[in]       msg_len            - byte length of the input message, must be a multiple of sha512_224
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the four parameters are valid, and ctx is initialize
 */
unsigned int sha512_224_dma_update_blocks(sha512_224_ctx_dma_t *ctx, unsigned int *msg, unsigned int msg_len)
{
    return hash_dma_update_blocks(ctx, msg, msg_len);
}

/**
 * @brief           dma sha512_224 final(input the remainder message and get the digest)
 * @param[in]       ctx                  - sha512_224_ctx_dma_t context pointer
 * @param[in]       remainder_msg        - remainder message
 * @param[in]       remainder_bytes      - byte length of the remainder message
 * @param[out]      digest               - sha512_224 digest, 28 bytes
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the four parameters are valid, and ctx is initialize
 */
unsigned int sha512_224_dma_final(sha512_224_ctx_dma_t *ctx, unsigned int *remainder_msg, unsigned int remainder_bytes, unsigned int *digest)
{
    return hash_dma_final(ctx, remainder_msg, remainder_bytes, digest);
}

/**
 * @brief           dma sha512_224 digest calculate
 * @param[in]       msg                  - message
 * @param[in]       msg_len            - byte length of the message, it could be 0
 * @param[out]      digest               - sha512_224 digest, 28 bytes
 * @param[in]       callback             - callback function pointer
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the four parameters are valid
 */
unsigned int sha512_224_dma(unsigned int *msg, unsigned int msg_len, unsigned int *digest, hash_callback callback)
{
    return hash_dma(HASH_SHA512_224, msg, msg_len, digest, callback);
}

#ifdef SUPPORT_HASH_DMA_NODE
/**
 * @brief           input whole message and get its sha512_224 digest(dma node style)
 * @param[in]       node                 - message node pointer
 * @param[in]       node_num             - number of hash nodes, i.e. number of message segments.
 * @param[out]      digest               - sha512_224 digest, 28 bytes
 * @param[in]       callback             - callback function pointer
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the digest buffer is sufficient
 *        2. if the whole message consists of some segments, every segment is a node, a node includes
 *           address and byte length.
 *        3. for every node or segment except the last, its message length must be a multiple of block length
 */
unsigned int sha512_224_dma_node_steps(const hash_dma_node_t *node, unsigned int node_num, unsigned int *digest, hash_callback callback)
{
    return hash_dma_node_steps(HASH_SHA512_224, node, node_num, digest, callback);
}
#endif

#endif

#endif
