/********************************************************************************************************
 * @file    sm3.c
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
#include "lib/include/hash/sm3.h"


#ifdef SUPPORT_HASH_SM3


/**
 * @brief       init sm3
 * @param[in]   ctx         - SM3_CTX context pointer.
 * @return      0:success     other:error
 */
unsigned int sm3_init(SM3_CTX *ctx)
{
    return hash_init(ctx, HASH_SM3);
}

/**
 * @brief       sm3 update message
 * @param[in]   ctx            - SM3_CTX context pointer.
 * @param[in]   msg            - message.
 * @param[in]   msg_bytes      - byte length of the input message.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# please make sure the three parameters are valid, and ctx is initialized.
  @endverbatim
 */
unsigned int sm3_update(SM3_CTX *ctx, unsigned char *msg, unsigned int msg_bytes)
{
    return hash_update(ctx, msg, msg_bytes);
}

/**
 * @brief       message update done, get the sm3 digest
 * @param[in]   ctx               - SM3_CTX context pointer.
 * @param[out]   digest            - sm3 digest, 32 bytes.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the digest buffer is sufficient.
  @endverbatim
 */
unsigned int sm3_final(SM3_CTX *ctx, unsigned char *digest)
{
    return hash_final(ctx, digest);
}

/**
 * @brief       input whole message and get its sm3 digest
 * @param[in]   msg            - message.
 * @param[in]   msg_bytes      - byte length of the input message, it could be 0.
 * @param[out]   digest         - sm3 digest, 32 bytes.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the digest buffer is sufficient.
  @endverbatim
 */
unsigned int sm3(unsigned char *msg, unsigned int msg_bytes, unsigned char *digest)
{
    return hash(HASH_SM3, msg, msg_bytes, digest);
}


    #ifdef SUPPORT_HASH_NODE
/**
 * @brief        input whole message and get its sm3 digest(node style)
 * @param[in]    node        - input, message node pointer
 * @param[in]    node_num    - input, number of hash nodes, i.e. number of message segments.
 * @param[out]   digest      - output, sm3 digest, 32 bytes
 * @return       0: HASH_SUCCESS(success), other(error)
 * @note
  @verbatim
 *    -# 1. please make sure the digest buffer is sufficient
 *    -# 2. if the whole message consists of some segments, every segment is a node, a node includes
 *        address and byte length.
  @endverbatim
 */
unsigned int sm3_node_steps(HASH_NODE *node, unsigned int node_num, unsigned char *digest)
{
    return hash_node_steps(HASH_SM3, node, node_num, digest);
}
    #endif


    #ifdef HASH_DMA_FUNCTION
/**
 * @brief       init dma sm3
 * @param[in]   ctx           - SM3_DMA_CTX context pointer.
 * @param[in]   callback      - callback function pointer.
 * @return      0:success     other:error
 */
unsigned int sm3_dma_init(SM3_DMA_CTX *ctx, HASH_CALLBACK callback)
{
    return hash_dma_init(ctx, HASH_SM3, callback);
}

/**
 * @brief       dma sm3 update some message blocks
 * @param[in]   ctx         - SM3_DMA_CTX context pointer.
 * @param[in]   msg         - message blocks.
 * @param[in]   msg_bytes   - byte length of the input message, must be a multiple of sm3
 *                            block byte length(64).
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the four parameters are valid, and ctx is initialized.
  @endverbatim
 */
unsigned int sm3_dma_update_blocks(SM3_DMA_CTX *ctx, unsigned int *msg, unsigned int msg_bytes)
{
    return hash_dma_update_blocks(ctx, msg, msg_bytes);
}

/**
 * @brief       dma sm3 final(input the remainder message and get the digest)
 * @param[in]   ctx               - SM3_DMA_CTX context pointer.
 * @param[in]   remainder_msg     - remainder message.
 * @param[in]   remainder_bytes   - byte length of the remainder message.
 *@param[out]   digest            - sm3 digest, 32 bytes.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the four parameters are valid, and ctx is initialized.
  @endverbatim
  */
unsigned int sm3_dma_final(SM3_DMA_CTX *ctx, unsigned int *remainder_msg, unsigned int remainder_bytes, unsigned int *digest)
{
    return hash_dma_final(ctx, remainder_msg, remainder_bytes, digest);
}

/**
 * @brief       dma sm3 digest calculate
 * @param[in]   msg           - message.
 * @param[in]   msg_bytes     - byte length of the message, it could be 0.
 * @param[out]   digest        - sm3 digest, 32 bytes.
 * @param[in]   callback      - callback function pointer.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the four parameters are valid.
  @endverbatim
 */
unsigned int sm3_dma(unsigned int *msg, unsigned int msg_bytes, unsigned int *digest, HASH_CALLBACK callback)
{
    return hash_dma(HASH_SM3, msg, msg_bytes, digest, callback);
}

        #ifdef SUPPORT_HASH_DMA_NODE
/**
 * @brief        input whole message and get its sm3 digest(dma node style)
 * @param[in]    node        - input, message node pointer
 * @param[in]    node_num    - input, number of hash nodes, i.e. number of message segments.
 * @param[out]   digest      - output, sm3 digest, 32 bytes
 * @param[in]    callback    - callback function pointer
 * @return       0: HASH_SUCCESS(success), other(error)
 * @note
  @verbatim
 *     -# 1. please make sure the digest buffer is sufficient
 *     -# 2. if the whole message consists of some segments, every segment is a node, a node includes
 *        address and byte length.
 *     -# 3. for every node or segment except the last, its message length must be a multiple of block length.
  @endverbatim
 */
unsigned int sm3_dma_node_steps(HASH_DMA_NODE *node, unsigned int node_num, unsigned int *digest, HASH_CALLBACK callback)
{
    return hash_dma_node_steps(HASH_SM3, node, node_num, digest, callback);
}
        #endif

    #endif


#endif
