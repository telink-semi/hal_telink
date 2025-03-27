/********************************************************************************************************
 * @file    sha224.c
 *
 * @brief   This is the source file for TL721X
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
#include "lib/include/hash/sha224.h"





#ifdef SUPPORT_HASH_SHA224
/**
 * @brief       init sha224
 * @param[in]   ctx         - SHA224_CTX context pointer.
 * @return      0:success     other:error
 */
unsigned int sha224_init(SHA224_CTX *ctx)
{
    return hash_init(ctx, HASH_SHA224);
}

/**
 * @brief       sha224 update message
 * @param[in]   ctx            - SHA224_CTX context pointer.
 * @param[in]   msg            - message.
 * @param[in]   msg_bytes      - byte length of the input message.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the three parameters are valid, and ctx is initialized.
  @endverbatim
 */
unsigned int sha224_update(SHA224_CTX *ctx, const unsigned char *msg, unsigned int msg_bytes)
{
    return hash_update(ctx, msg, msg_bytes);
}

/**
 * @brief       message update done, get the sha224 digest
 * @param[in]   ctx               - SHA224_CTX context pointer.
 * @param[out]  digest            - sha224 digest, 28 bytes.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the digest buffer is sufficient.
  @endverbatim
 */
unsigned int sha224_final(SHA224_CTX *ctx, unsigned char *digest)
{
    return hash_final(ctx, digest);
}


/**
 * @brief       input whole message and get its sha224 digest
 * @param[in]   msg            - message.
 * @param[in]   msg_bytes      - byte length of the input message, it could be 0.
 * @param[in]   digest         - sha224 digest, 28 bytes.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the digest buffer is sufficient.
  @endverbatim
 */
unsigned int sha224(unsigned char *msg, unsigned int msg_bytes, unsigned char *digest)
{
    return hash(HASH_SHA224, msg, msg_bytes, digest);
}


#ifdef HASH_DMA_FUNCTION
/**
 * @brief       init dma sha224
 * @param[in]   ctx           - SHA224_DMA_CTX context pointer.
 * @param[in]   callback      - callback function pointer.
 * @return      0:success     other:error
 */
unsigned int sha224_dma_init(SHA224_DMA_CTX *ctx, HASH_CALLBACK callback)
{
    return hash_dma_init(ctx, HASH_SHA224, callback);
}

/**
 * @brief       dma sha224 update some message blocks
 * @param[in]   ctx         - SHA224_DMA_CTX context pointer.
 * @param[in]   msg         - message blocks.
 * @param[in]   msg_words   - word length of the input message, must be a multiple of sha224
 *                            block word length(16).
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the four parameters are valid, and ctx is initialized.
  @endverbatim
 */
unsigned int sha224_dma_update_blocks(SHA224_DMA_CTX *ctx, unsigned int *msg, unsigned int msg_words)
{
    return hash_dma_update_blocks(ctx, msg, msg_words);
}

/**
 * @brief       dma sha224 final(input the remainder message and get the digest)
 * @param[in]   ctx               - SHA224_DMA_CTX context pointer.
 * @param[in]   remainder_msg     - remainder message.
 * @param[in]   remainder_bytes   - byte length of the remainder message, must be in [0, BLOCK_BYTE_LEN-1],
 *                                  here BLOCK_BYTE_LEN is block byte length of sha224, it is 64.
 * @param[in]   digest            - hash digest.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the four parameters are valid, and ctx is initialized.
  @endverbatim
 */
unsigned int sha224_dma_final(SHA224_DMA_CTX *ctx, unsigned int *remainder_msg, unsigned int remainder_bytes, unsigned int *digest)
{
    return hash_dma_final(ctx, remainder_msg, remainder_bytes, digest);
}

/**
 * @brief       dma sha224 digest calculate
 * @param[in]   msg           - message.
 * @param[in]   msg_bytes     - byte length of the message, it could be 0.
 * @param[out]  digest        - sha224 digest, 28 bytes.
 * @param[in]   callback      - callback function pointer.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. please make sure the four parameters are valid.
  @endverbatim
 */
unsigned int sha224_dma(unsigned int *msg, unsigned int msg_bytes, unsigned int *digest, HASH_CALLBACK callback)
{
    return hash_dma(HASH_SHA224, msg, msg_bytes, digest, callback);
}
#endif

#endif
