/********************************************************************************************************
 * @file    ske_cbc.c
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
#include "lib/include/ske/ske_cbc.h"

#ifdef SUPPORT_SKE_MODE_CBC

/**
 * @brief           Initializes CBC mode configuration
 * @param[in]       alg                  - SKE algorithm
 * @param[in]       crypto               - Encrypt or decrypt operation
 * @param[in]       key                  - Key data
 * @param[in]       sp_key_idx           - Secure port key index
 * @param[in]       iv                   - Initialization vector
 * @return          SKE_SUCCESS if successful, otherwise error code
 */
unsigned int ske_lp_cbc_init(ske_alg_e alg, ske_crypto_e crypto, const unsigned char *key, unsigned short sp_key_idx, unsigned char *iv)
{
    return ske_lp_init(alg, SKE_MODE_CBC, crypto, key, sp_key_idx, iv);
}

/**
 * @brief           Updates multiple blocks in CBC mode
 * @param[in]       in                   - Input data
 * @param[out]      out                  - Output data
 * @param[in]       bytes                - Data length in bytes
 * @return          SKE_SUCCESS if successful, otherwise error code
 */
unsigned int ske_lp_cbc_update_blocks(unsigned char *in, unsigned char *out, unsigned int bytes)
{
    return ske_lp_update_blocks(in, out, bytes);
}

/**
 * @brief           Finalizes CBC mode operation
 * @return          SKE_SUCCESS if successful, otherwise error code
 */
unsigned int ske_lp_cbc_final(void)
{
    return ske_lp_final();
}

/**
 * @brief           Performs one-shot encryption/decryption in CBC mode
 * @param[in]       alg                  - SKE algorithm
 * @param[in]       crypto               - Encrypt or decrypt operation
 * @param[in]       key                  - Key data
 * @param[in]       sp_key_idx           - Secure port key index
 * @param[in]       iv                   - Initialization vector
 * @param[in]       in                   - Input data
 * @param[out]      out                  - Output data
 * @param[in]       bytes                - Data length in bytes
 * @return          SKE_SUCCESS if successful, otherwise error code
 */
unsigned int ske_lp_cbc_crypto(ske_alg_e alg, ske_crypto_e crypto, const unsigned char *key, unsigned short sp_key_idx, unsigned char *iv, unsigned char *in, unsigned char *out,
                               unsigned int bytes)
{
    return ske_lp_crypto(alg, SKE_MODE_CBC, crypto, key, sp_key_idx, iv, in, out, bytes);
}

#ifdef SKE_LP_DMA_FUNCTION
/**
 * @brief           Initializes CBC mode configuration for DMA
 * @param[in]       alg                  - SKE algorithm
 * @param[in]       crypto               - Encrypt or decrypt operation
 * @param[in]       key                  - Key data
 * @param[in]       sp_key_idx           - Secure port key index
 * @param[in]       iv                   - Initialization vector
 * @return          SKE_SUCCESS if successful, otherwise error code
 */
unsigned int ske_lp_dma_cbc_init(ske_alg_e alg, ske_crypto_e crypto, const unsigned char *key, unsigned short sp_key_idx, unsigned char *iv)
{
    return ske_lp_dma_init(alg, SKE_MODE_CBC, crypto, key, sp_key_idx, iv);
}

/**
 * @brief           Updates multiple blocks in CBC mode using DMA
 * @param[in]       in                   - Input data
 * @param[out]      out                  - Output data
 * @param[in]       words                - Data length in words
 * @param[in]       callback             - Callback function pointer
 * @return          SKE_SUCCESS if successful, otherwise error code
 */
unsigned int ske_lp_dma_cbc_update_blocks(unsigned int *in, unsigned int *out, unsigned int words, SKE_CALLBACK callback)
{
    return ske_lp_dma_update_blocks(in, out, words, callback);
}

/**
 * @brief           Finalizes CBC mode operation for DMA
 * @return          SKE_SUCCESS if successful, otherwise error code
 */
unsigned int ske_lp_dma_cbc_final(void)
{
    return ske_lp_dma_final();
}

/**
 * @brief           Performs one-shot encryption/decryption in CBC mode using DMA
 * @param[in]       alg                  - SKE algorithm
 * @param[in]       crypto               - Encrypt or decrypt operation
 * @param[in]       key                  - Key data
 * @param[in]       sp_key_idx           - Secure port key index
 * @param[in]       iv                   - Initialization vector
 * @param[in]       in                   - Input data
 * @param[out]      out                  - Output data
 * @param[in]       words                - Data length in words
 * @param[in]       callback             - Callback function pointer
 * @return          SKE_SUCCESS if successful, otherwise error code
 */
unsigned int ske_lp_dma_cbc_crypto(ske_alg_e alg, ske_crypto_e crypto, const unsigned char *key, unsigned short sp_key_idx, unsigned char *iv, unsigned int *in, unsigned int *out,
                                   unsigned int words, SKE_CALLBACK callback)
{
    return ske_lp_dma_crypto(alg, SKE_MODE_CBC, crypto, key, sp_key_idx, iv, in, out, words, callback);
}
#endif

#endif
