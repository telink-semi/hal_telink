/********************************************************************************************************
 * @file    sha1.c
 *
 * @brief   This is the source file for TL321X
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
#include "lib/include/hash/sha1.h"





#ifdef SUPPORT_HASH_SHA1

/**
 * @brief       init sha1
 * @param[in]   ctx         - SHA1_CTX context pointer.
 * @return      0:success     other:error
 */
unsigned int sha1_init(SHA1_CTX *ctx)
{
    return hash_init(ctx, HASH_SHA1);
}

/**
 * @brief       sha1 update message
 * @param[in]   ctx            - SHA1_CTX context pointer.
 * @param[in]   msg            - message.
 * @param[in]   msg_bytes      - byte length of the input message.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the three parameters are valid, and ctx is initialized.
  @endverbatim
 */
unsigned int sha1_update(SHA1_CTX *ctx, const unsigned char *msg, unsigned int msg_bytes)
{
    return hash_update(ctx, msg, msg_bytes);
}

/**
 * @brief       message update done, get the sha1 digest
 * @param[out]  digest         - sha1 digest, 20 bytes.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the digest buffer is sufficient.
  @endverbatim
 */
unsigned int sha1_final(SHA1_CTX *ctx, unsigned char *digest)
{
    return hash_final(ctx, digest);
}

/**
 * @brief       input whole message and get its sha1 digest
 * @param[in]   msg            - message.
 * @param[in]   msg_bytes      - byte length of the input message, it could be 0.
 * @param[out]  digest         - sha1 digest, 20 bytes.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the digest buffer is sufficient.
  @endverbatim
 */
unsigned int sha1(unsigned char *msg, unsigned int msg_bytes, unsigned char *digest)
{
    return hash(HASH_SHA1, msg, msg_bytes, digest);
}


#ifdef HASH_DMA_FUNCTION

/**
 * @brief       init dma sha1
 * @param[in]   ctx            - SHA1_DMA_CTX context pointer.
 * @param[in]   callback       - callback function pointer.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the digest buffer is sufficient.
  @endverbatim
 */
unsigned int sha1_dma_init(SHA1_DMA_CTX *ctx, HASH_CALLBACK callback)
{
    return hash_dma_init(ctx, HASH_SHA1, callback);
}

/**
 * @brief       dma sha1 update some message blocks
 * @param[in]   ctx            - SHA1_DMA_CTX context pointer.
 * @param[in]   msg            - message blocks.
 * @param[in]   msg_words      - word length of the input message, must be a multiple of sha1
 *                               block word length(16).
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the four parameters are valid, and ctx is initialized.
  @endverbatim
 */
unsigned int sha1_dma_update_blocks(SHA1_DMA_CTX *ctx, unsigned int *msg, unsigned int msg_words)
{
    return hash_dma_update_blocks(ctx, msg, msg_words);
}

/**
 * @brief       dma sha1 final(input the remainder message and get the digest)
 * @param[in]   ctx               - SHA1_DMA_CTX context pointer.
 * @param[in]   remainder_msg     - remainder message.
 * @param[in]   remainder_bytes   - byte length of the remainder message, must be in [0, BLOCK_BYTE_LEN-1],
 *                                  here BLOCK_BYTE_LEN is block byte length of sha1, it is 64.
 * @param[out]  digest            - sha1 digest, 20 bytes.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the four parameters are valid, and ctx is initialized.
  @endverbatim
 */
unsigned int sha1_dma_final(SHA1_DMA_CTX *ctx, unsigned int *remainder_msg, unsigned int remainder_bytes, unsigned int *digest)
{
    return hash_dma_final(ctx, remainder_msg, remainder_bytes, digest);
}

/**
 * @brief       dma sha1 digest calculate
 * @param[in]   msg               - message.
 * @param[in]   msg_bytes         - byte length of the message, it could be 0.
 * @param[out]  digest            - sha1 digest, 20 bytes.
 * @param[in]   callback          - callback function pointer.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the four parameters are valid, and ctx is initialized.
  @endverbatim
 */
unsigned int sha1_dma(unsigned int *msg, unsigned int msg_bytes, unsigned int *digest, HASH_CALLBACK callback)
{
    return hash_dma(HASH_SHA1, msg, msg_bytes, digest, callback);
}
#endif

#endif
