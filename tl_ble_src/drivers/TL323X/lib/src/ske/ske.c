/********************************************************************************************************
 * @file    ske.c
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
#include <stdio.h>
#include "lib/include/crypto_common/utility.h"
#include "lib/include/ske/ske.h"

static ske_ctx_t g_ske_ctx[1];

/**
 * @brief           since hardware does not support 3DES
 */
#if (defined(SUPPORT_SKE_TDES_128))
/**
 * @brief           hold current ske algorithm(for 3DES)
 */
static ske_alg_e g_ske_alg;
/**
 * @brief           hold input mode for 3DES
 */
static ske_mode_e g_ske_mode;
/**
 * @brief           hold current block cipher crypto for 3DES
 */
static ske_crypto_e g_ske_crypto_action;
/**
 * @brief           hold key for 3DES
 */
static unsigned int g_ske_key_buf[6];
/**
 * @brief           hold current IV for 3DES
 */
static unsigned int g_ske_iv_buf[4];
#endif

#if (defined(SUPPORT_SKE_TDES_128))
/**
 * @brief           Performs big-endian addition on an array of bytes
 * @param[in,out]   a                    - Pointer to the byte array (big-endian)
 * @param[in]       a_bytes              - Length of the array in bytes
 * @param[in]       b                    - Value to add
 * @return          None
 * @note            Used for CTR counter addition (big-endian)
 */
void ske_big_endian_add_uint8(unsigned char *a, unsigned int a_bytes, unsigned char b)
{
    int i;

    for (i = a_bytes; i > 0;)
    {
        a[--i] += b;
        if (a[i] < b)
        {
            b = 1;
        }
        else
        {
#if 1
            b = 0; // for security
#else
            break;
#endif
        }
    }
}
#endif

/**
 * @brief           Checks if the SKE algorithm is valid
 * @param[in]       ske_alg              - SKE algorithm to check
 * @return          SKE_SUCCESS if valid, otherwise error code
 */
unsigned char ske_lp_check_alg(ske_alg_e ske_alg)
{
    unsigned char ret;

    switch (ske_alg)
    {
#ifdef SUPPORT_SKE_DES
    case SKE_ALG_DES:
#endif

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

#ifdef SUPPORT_SKE_AES_128
    case SKE_ALG_AES_128:
#endif

#ifdef SUPPORT_SKE_AES_192
    case SKE_ALG_AES_192:
#endif

#ifdef SUPPORT_SKE_AES_256
    case SKE_ALG_AES_256:
#endif

#ifdef SUPPORT_SKE_SM4
    case SKE_ALG_SM4:
#endif
        ret = SKE_SUCCESS;
        break;

    default:
        ret = SKE_INPUT_INVALID;
        break;
    }

    return ret;
}

/**
 * @brief           Checks if the SKE algorithm mode is valid
 * @param[in]       ske_alg              - SKE algorithm
 * @param[in]       ske_mode             - SKE mode to check
 * @return          SKE_SUCCESS if valid, otherwise error code
 */
unsigned char ske_lp_check_mode(ske_alg_e ske_alg, ske_mode_e ske_mode)
{
    unsigned char ret;

    switch (ske_mode)
    {
#ifdef SUPPORT_SKE_MODE_ECB
    case SKE_MODE_ECB:
#endif

#ifdef SUPPORT_SKE_MODE_CBC
    case SKE_MODE_CBC:
#endif

#ifdef SUPPORT_SKE_MODE_CFB
    case SKE_MODE_CFB:
#endif

#ifdef SUPPORT_SKE_MODE_OFB
    case SKE_MODE_OFB:
#endif

#ifdef SUPPORT_SKE_MODE_CTR
    case SKE_MODE_CTR:
#endif

#ifdef SUPPORT_SKE_MODE_CBC_MAC
    case SKE_MODE_CBC_MAC:
#endif
        ret = SKE_SUCCESS;
        break;

        // for DES/3DES, CMAC is not supported at present
#ifdef SUPPORT_SKE_MODE_CMAC
    case SKE_MODE_CMAC:
        switch (ske_alg)
        {
#ifdef SUPPORT_SKE_AES_128
        case SKE_ALG_AES_128:
#endif

#ifdef SUPPORT_SKE_AES_192
        case SKE_ALG_AES_192:
#endif

#ifdef SUPPORT_SKE_AES_256
        case SKE_ALG_AES_256:
#endif

#ifdef SUPPORT_SKE_SM4
        case SKE_ALG_SM4:
#endif

#if (defined(SUPPORT_SKE_AES_128) || defined(SUPPORT_SKE_AES_192) || defined(SUPPORT_SKE_AES_256) || defined(SUPPORT_SKE_SM4))
            ret = SKE_SUCCESS;
            break;
#endif

        default:
            ret = SKE_INPUT_INVALID;
        }
        break;
#endif

        // for DES/3DES, XTS, CCM and GCM mode are not supported due to the definition or standard
#ifdef SUPPORT_SKE_MODE_XTS
    case SKE_MODE_XTS:
#endif

#ifdef SUPPORT_SKE_MODE_CCM
    case SKE_MODE_CCM:
#endif

#ifdef SUPPORT_SKE_MODE_GCM
    case SKE_MODE_GCM:
#endif

#if (defined(SUPPORT_SKE_MODE_XTS) || defined(SUPPORT_SKE_MODE_CCM) || defined(SUPPORT_SKE_MODE_GCM))
        switch (ske_alg)
        {
#ifdef SUPPORT_SKE_AES_128
        case SKE_ALG_AES_128:
#endif

#ifdef SUPPORT_SKE_AES_192
        case SKE_ALG_AES_192:
#endif

#ifdef SUPPORT_SKE_AES_256
        case SKE_ALG_AES_256:
#endif

#ifdef SUPPORT_SKE_SM4
        case SKE_ALG_SM4:
#endif

#if (defined(SUPPORT_SKE_AES_128) || defined(SUPPORT_SKE_AES_192) || defined(SUPPORT_SKE_AES_256) || defined(SUPPORT_SKE_SM4))
            ret = SKE_SUCCESS;
            break;
#endif

        default:
            ret = SKE_INPUT_INVALID;
            break;
        }

        break;
#endif

    default:
        ret = SKE_INPUT_INVALID;
        break;
    }

    return ret;
}

/**
 * @brief           Gets the block byte length for a specific SKE algorithm
 * @param[in]       ske_alg              - SKE algorithm
 * @return          Block byte length for the algorithm
 */
unsigned char ske_lp_get_block_byte_len(ske_alg_e ske_alg)
{
    unsigned char byteLen;

    switch (ske_alg)
    {
#ifdef SUPPORT_SKE_DES
    case SKE_ALG_DES:
#endif

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

#if (defined(SUPPORT_SKE_DES) || defined(SUPPORT_SKE_TDES_128) || defined(SUPPORT_SKE_TDES_192) || defined(SUPPORT_SKE_TDES_EEE_128) || defined(SUPPORT_SKE_TDES_EEE_192))
        byteLen = 8;
        break;
#endif

#ifdef SUPPORT_SKE_AES_128
    case SKE_ALG_AES_128:
#endif

#ifdef SUPPORT_SKE_AES_192
    case SKE_ALG_AES_192:
#endif

#ifdef SUPPORT_SKE_AES_256
    case SKE_ALG_AES_256:
#endif

#ifdef SUPPORT_SKE_SM4
    case SKE_ALG_SM4:
#endif

#if (defined(SUPPORT_SKE_AES_128) || defined(SUPPORT_SKE_AES_192) || defined(SUPPORT_SKE_AES_256) || defined(SUPPORT_SKE_SM4))
        byteLen = 16;
        break;
#endif

    default:
        byteLen = 16; // default alg SM4
    }

    return byteLen;
}

/**
 * @brief           Gets the key byte length for a specific SKE algorithm
 * @param[in]       ske_alg              - SKE algorithm
 * @return          Key byte length for the algorithm
 */
unsigned char ske_lp_get_key_byte_len(ske_alg_e ske_alg)
{
    unsigned char byte_len;

    switch (ske_alg)
    {
#ifdef SUPPORT_SKE_DES
    case SKE_ALG_DES:
        byte_len = 8;
        break;
#endif

#ifdef SUPPORT_SKE_TDES_128
    case SKE_ALG_TDES_128:
#endif

#ifdef SUPPORT_SKE_TDES_EEE_128
    case SKE_ALG_TDES_EEE_128:
#endif

#ifdef SUPPORT_SKE_AES_128
    case SKE_ALG_AES_128:
#endif

#ifdef SUPPORT_SKE_SM4
    case SKE_ALG_SM4:
#endif

#if (defined(SUPPORT_SKE_TDES_128) || defined(SUPPORT_SKE_TDES_EEE_128) || defined(SUPPORT_SKE_AES_128) || defined(SUPPORT_SKE_SM4))
        byte_len = 16;
        break;
#endif

#ifdef SUPPORT_SKE_TDES_192
    case SKE_ALG_TDES_192:
#endif

#ifdef SUPPORT_SKE_TDES_EEE_192
    case SKE_ALG_TDES_EEE_192:
#endif

#ifdef SUPPORT_SKE_AES_192
    case SKE_ALG_AES_192:
#endif

#if (defined(SUPPORT_SKE_TDES_192) || defined(SUPPORT_SKE_TDES_EEE_192) || defined(SUPPORT_SKE_AES_192))
        byte_len = 24;
        break;
#endif

#ifdef SUPPORT_SKE_AES_256
    case SKE_ALG_AES_256:
        byte_len = 32;
        break;
#endif

    default:
        byte_len = 16; // default alg SM4
    }

    return byte_len;
}

/**
 * @brief           Sets the Initialization Vector (IV) for SKE
 * @param[in]       iv                   - Pointer to the IV data
 * @param[in]       block_bytes          - Byte length of the IV
 * @return          None
 */
void ske_lp_set_iv(const unsigned char *iv, unsigned int block_bytes)
{
    unsigned int tmp[4];

    if (((unsigned int)iv) & 3)
    {
        memcpy_(tmp, iv, block_bytes);
        ske_lp_set_iv_uint32(tmp, block_bytes / 4);
    }
    else
    {
        ske_lp_set_iv_uint32((const unsigned int *)iv, block_bytes / 4);
    }
}

/**
 * @brief           Sets the key for SKE
 * @param[in]       alg                  - SKE algorithm
 * @param[in]       key                  - Pointer to the key data
 * @param[in]       key_bytes            - Byte length of the key
 * @param[in]       key_idx              - Key index (1 or 2)
 * @return          None
 */
void ske_lp_set_key(ske_alg_e alg, const unsigned char *key, unsigned short key_bytes, unsigned short key_idx)
{
    unsigned int tmp[8];

    memcpy_(tmp, key, key_bytes);

    // for 3DES-2key, set key3=key1
    switch (alg)
    {
#ifdef SUPPORT_SKE_TDES_128
    case SKE_ALG_TDES_128:
#endif

#ifdef SUPPORT_SKE_TDES_EEE_128
    case SKE_ALG_TDES_EEE_128:
#endif

#if (defined(SUPPORT_SKE_TDES_128) || defined(SUPPORT_SKE_TDES_EEE_128))
        memcpy_(tmp + 4, key, 8);
        key_bytes += 8;
        break;
#endif

    default:
        break;
    }

    ske_lp_set_key_uint32(tmp, key_idx, key_bytes / 4);
}

/**
 * @brief           Initializes SKE configuration
 * @param[in]       ctx                  - Pointer to SKE context
 * @param[in]       alg                  - SKE algorithm
 * @param[in]       mode                 - SKE operation mode
 * @param[in]       crypto               - Encrypt or decrypt operation
 * @param[in]       key                  - Pointer to the key data
 * @param[in]       sp_key_idx           - Secure port key index
 * @param[in]       iv                   - Pointer to the IV data
 * @param[in]       dma_en               - DMA mode enable flag
 * @return          SKE_SUCCESS if successful, otherwise error code
 */
unsigned int ske_lp_init_internal(ske_ctx_t *ctx, ske_alg_e alg, ske_mode_e mode, ske_crypto_e crypto, const unsigned char *key, unsigned short sp_key_idx, const unsigned char *iv,
                                  unsigned int dma_en)
{
    unsigned int key_bytes;
    (void)sp_key_idx;

    if (NULL == ctx)
    {
        return SKE_BUFFER_NULL;
    }
    else if (SKE_SUCCESS != ske_lp_check_alg(alg))
    {
        return SKE_INPUT_INVALID;
    }
    else if (SKE_SUCCESS != ske_lp_check_mode(alg, mode))
    {
        return SKE_INPUT_INVALID;
    }
    else if (crypto > SKE_CRYPTO_DECRYPT)
    {
        return SKE_INPUT_INVALID;
    }
    else if (NULL == key) // secure port
    {
#ifdef SKE_SECURE_PORT_FUNCTION
        /*  TODO. to add some checking actions about secure port, this depends on user
        if((SKE_ALG_SM4 != alg) || (sp_key_idx > 3))
        {
            return SKE_ERROR;//SKE_BUFFER_NULL;
        }*/
#else
        return SKE_INPUT_INVALID;
#endif
    }
    else
    {
        ;
    }

    if (SKE_MODE_ECB == mode)
    {
        iv = NULL;
    }
    else if (NULL == iv)
    {
        return SKE_BUFFER_NULL; // SKE_ERROR;//
    }
    else
    {
        ;
    }

#if (defined(SUPPORT_SKE_TDES_128))
    /******************* because hw does not support TDES ******************/

    g_ske_alg = alg; // very important!!! to distinguish 3DES and other algorithm, if the MACRO is available

    if ((SKE_ALG_TDES_128 == alg) || (SKE_ALG_TDES_192 == alg) || (SKE_ALG_TDES_EEE_128 == alg) || (SKE_ALG_TDES_EEE_192 == alg))
    {
        if ((SKE_ALG_TDES_128 == alg) || (SKE_ALG_TDES_EEE_128 == alg))
        {
            memcpy_(g_ske_key_buf, key, 16);
            memcpy_(g_ske_key_buf + 16 / 4, key, 8);
        }
        else
        {
            memcpy_(g_ske_key_buf, key, 24);
        }

        g_ske_ctx->block_bytes = ske_lp_get_block_byte_len(alg);
        g_ske_ctx->block_words = g_ske_ctx->block_bytes >> 2;

        g_ske_mode = mode;
        g_ske_crypto_action = crypto;
        alg = SKE_ALG_DES;

        if (SKE_MODE_ECB != mode)
        {
            memcpy_(g_ske_iv_buf, iv, 8);
            mode = SKE_MODE_ECB;
        }
        else
        {
            ;
        }
    }
    else
    {
        ;
    }
    /***********************************************************************/
#endif

    ctx->block_bytes = ske_lp_get_block_byte_len(alg);
    ctx->block_words = ctx->block_bytes >> 2;

    ske_lp_set_endian_uint32();
    ske_lp_set_alg(alg);
    ske_lp_set_mode(mode);
    ske_lp_set_crypto(crypto);
    ske_lp_set_last_block(0);

    // set iv or nonce
    if (NULL != iv)
    {
        ske_lp_set_iv(iv, ctx->block_bytes);
    }
    else
    {
        ;
    }

    if (NULL != key) // key is from user input
    {
        // key1
        key_bytes = ske_lp_get_key_byte_len(alg);
        ske_lp_set_key(alg, key, key_bytes, 1);
    }
    else // key is from secure port
    {
        // TODO. set secure port.
    }

    return ske_lp_expand_key(dma_en);
}

/**
 * @brief           Initializes SKE configuration (CPU style)
 * @param[in]       alg                  - SKE algorithm
 * @param[in]       mode                 - SKE operation mode
 * @param[in]       crypto               - Encrypt or decrypt operation
 * @param[in]       key                  - Pointer to the key data
 * @param[in]       sp_key_idx           - Secure port key index
 * @param[in]       iv                   - Pointer to the IV data
 * @return          SKE_SUCCESS if successful, otherwise error code
 */
unsigned int ske_lp_init(ske_alg_e alg, ske_mode_e mode, ske_crypto_e crypto, const unsigned char *key, unsigned short sp_key_idx, unsigned char *iv)
{
    ske_lp_set_cpu_mode();
#if defined(SUPPORT_SKE_MODE_XTS)
    ske_lp_set_c_len_uint32(0); // just for XTS mode
#endif

    return ske_lp_init_internal(g_ske_ctx, alg, mode, crypto, key, sp_key_idx, iv, SKE_LP_DMA_DISABLE);
}

#if (defined(SUPPORT_SKE_TDES_128))
/**
 * @brief           Performs 3DES encryption or decryption (CPU style)
 * @param[in]       in                   - Pointer to input data
 * @param[out]      out                  - Pointer to output data
 * @param[in]       bytes                - Byte length of input/output data
 * @return          SKE_SUCCESS if successful, otherwise error code
 */
unsigned int ske_tdes_update_blocks(const unsigned char *in, unsigned char *out, unsigned int bytes)
{
    unsigned int tmp_in[2], tmp_out[2];
    unsigned int tmp_iv[2];
    unsigned char *p_out = out;
    unsigned int is_EEE;
    unsigned int i;
    unsigned int ret;

    if (NULL == in)
    {
        return SKE_BUFFER_NULL;
    }
    else if (bytes & (g_ske_ctx->block_bytes - 1))
    {
        return SKE_INPUT_INVALID;
    }
    else if (0 == bytes)
    {
        return SKE_SUCCESS;
    }
    else
    {
        ;
    }

    if ((SKE_ALG_TDES_128 == g_ske_alg) || (SKE_ALG_TDES_192 == g_ske_alg))
    {
        is_EEE = 0;
    }
    else
    {
        is_EEE = 1;
    }

    // input one block ---> calculating ---> output one block
    for (i = 0; i < bytes; i += g_ske_ctx->block_bytes)
    {
        /************* first **************/
        memcpy_(tmp_in, in, g_ske_ctx->block_bytes);

        if ((SKE_MODE_CFB == g_ske_mode) || (SKE_MODE_OFB == g_ske_mode) || (SKE_MODE_CTR == g_ske_mode))
        {
            ret = tdes_ecb_update_one_block(is_EEE, g_ske_key_buf, SKE_CRYPTO_ENCRYPT, g_ske_iv_buf, tmp_iv);
            if (SKE_SUCCESS != ret)
            {
                ret = SKE_ERROR;
                goto END;
            }
            else
            {
                ;
            }

            tmp_out[0] = tmp_iv[0] ^ tmp_in[0];
            tmp_out[1] = tmp_iv[1] ^ tmp_in[1];
        }
        else // ECB or CBC
        {
            if ((SKE_MODE_CBC == g_ske_mode) && (SKE_CRYPTO_ENCRYPT == g_ske_crypto_action)) // CBC encrypt
            {
                tmp_in[0] ^= g_ske_iv_buf[0];
                tmp_in[1] ^= g_ske_iv_buf[1];
            }
            else
            {
                ;
            }

            ret = tdes_ecb_update_one_block(is_EEE, g_ske_key_buf, g_ske_crypto_action, tmp_in, tmp_out);
            if (SKE_SUCCESS != ret)
            {
                ret = SKE_ERROR;
                goto END;
            }
            else
            {
                ;
            }

            if ((SKE_MODE_CBC == g_ske_mode) && (SKE_CRYPTO_DECRYPT == g_ske_crypto_action)) // CBC decrypt
            {
                tmp_out[0] ^= g_ske_iv_buf[0];
                tmp_out[1] ^= g_ske_iv_buf[1];
            }
            else
            {
                ;
            }
        }

        // update iv
        if (SKE_MODE_CTR == g_ske_mode)
        {
            ske_big_endian_add_uint8((unsigned char *)g_ske_iv_buf, g_ske_ctx->block_bytes, 1);
        }
        else if (SKE_MODE_OFB == g_ske_mode)
        {
            uint32_copy(g_ske_iv_buf, tmp_iv, g_ske_ctx->block_words);
        }
        else if ((SKE_MODE_CBC == g_ske_mode) || (SKE_MODE_CFB == g_ske_mode))
        {
            if (SKE_CRYPTO_ENCRYPT == g_ske_crypto_action)
            {
                uint32_copy(g_ske_iv_buf, tmp_out, g_ske_ctx->block_words);
            }
            else
            {
                uint32_copy(g_ske_iv_buf, tmp_in, g_ske_ctx->block_words);
            }
        }
        else
        {
            ;
        }

        // output
        if (out)
        {
            memcpy_(out, tmp_out, g_ske_ctx->block_bytes);
            out += g_ske_ctx->block_bytes;
        }
        else
        {
            ;
        }

        in += g_ske_ctx->block_bytes;
    }

END:

    if (SKE_ERROR == ret)
    {
        uint32_clear(g_ske_key_buf, sizeof(g_ske_key_buf) / 4);
        uint32_clear(g_ske_iv_buf, sizeof(g_ske_iv_buf) / 4);
        if (p_out)
        {
            memset_(p_out, 0, bytes);
        }
        else
        {
            ;
        }
    }
    else
    {
        ;
    }

    return ret;
}
#endif

/**
 * @brief           Performs SKE encryption or decryption (CPU style)
 * @param[in]       in                   - Pointer to input data
 * @param[out]      out                  - Pointer to output data
 * @param[in]       bytes                - Byte length of input/output data
 * @return          SKE_SUCCESS if successful, otherwise error code
 */
unsigned int ske_lp_update_blocks(unsigned char *in, unsigned char *out, unsigned int bytes)
{
    if ((NULL == in) || (NULL == out))
    {
        return SKE_BUFFER_NULL;
    }
    else if (bytes & (g_ske_ctx->block_bytes - 1))
    {
        return SKE_INPUT_INVALID;
    }
    else if (0 == bytes)
    {
        return SKE_SUCCESS;
    }
    else
    {
        ;
    }

#if (defined(SUPPORT_SKE_TDES_128))
    if ((SKE_ALG_TDES_128 == g_ske_alg) || (SKE_ALG_TDES_192 == g_ske_alg) || (SKE_ALG_TDES_EEE_128 == g_ske_alg) || (SKE_ALG_TDES_EEE_192 == g_ske_alg))
    {
        return ske_tdes_update_blocks(in, out, bytes);
    }
    else
    {
        ;
    }
#endif

    return ske_lp_update_blocks_internal(g_ske_ctx, in, out, bytes);
}

/**
 * @brief           Finalizes SKE operation
 * @return          SKE_SUCCESS if successful, otherwise error code
 */
unsigned int ske_lp_final(void)
{
    memset_(g_ske_ctx, 0, sizeof(ske_ctx_t));

#if (defined(SUPPORT_SKE_TDES_128))
    g_ske_alg = SKE_ALG_DES;
    g_ske_crypto_action = SKE_CRYPTO_ENCRYPT;
    g_ske_mode = SKE_MODE_ECB;
    uint32_clear(g_ske_key_buf, sizeof(g_ske_key_buf) / 4);
    uint32_clear(g_ske_iv_buf, sizeof(g_ske_iv_buf) / 4);
#endif

    return SKE_SUCCESS;
}

/**
 * @brief           Performs SKE encryption or decryption (CPU style, one-off)
 * @param[in]       alg                  - SKE algorithm
 * @param[in]       mode                 - SKE operation mode (ECB, CBC, OFB, etc.)
 * @param[in]       crypto               - Encrypt or decrypt operation
 * @param[in]       key                  - Key in bytes
 * @param[in]       sp_key_idx           - Secure port key index
 * @param[in]       iv                   - IV in bytes (must be a block)
 * @param[in]       in                   - Pointer to input data (plaintext or ciphertext)
 * @param[out]      out                  - Pointer to output data (ciphertext or plaintext)
 * @param[in]       bytes                - Byte length of input/output data
 * @return          SKE_SUCCESS if successful, otherwise error code
 * @note
 *        1. If mode is ECB, then there is no IV (iv can be NULL)
 *        2. Designed for ECB/CBC/CFB/OFB/CTR/XTS modes (input/output unit is a block)
 *        3. If key is from user input, ensure key is not NULL (sp_key_idx is unused)
 *        4. in and out can be the same buffer (output will overwrite input)
 *        5. bytes must be a multiple of block byte length
 */
unsigned int ske_lp_crypto(ske_alg_e alg, ske_mode_e mode, ske_crypto_e crypto, const unsigned char *key, unsigned short sp_key_idx, unsigned char *iv, unsigned char *in,
                           unsigned char *out, unsigned int bytes)
{
    unsigned int ret;

    ske_lp_set_cpu_mode();
#if defined(SUPPORT_SKE_MODE_XTS)
    ske_lp_set_c_len_uint32(0); // just for XTS mode
#endif

    ret = ske_lp_init_internal(g_ske_ctx, alg, mode, crypto, key, sp_key_idx, iv, SKE_LP_DMA_DISABLE);
    if (SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        return ske_lp_update_blocks(in, out, bytes);
    }
}

/**
 * @brief           Performs SKE encryption or decryption without output (internal helper function)
 * @param[in]       ctx                  - Pointer to SKE context
 * @param[in]       in                   - Pointer to input data
 * @param[in]       bytes                - Byte length of input data
 * @return          SKE_SUCCESS if successful, otherwise error code
 */
unsigned int ske_lp_update_blocks_no_output_(ske_ctx_t *ctx, const unsigned char *in, unsigned int bytes)
{
#if (defined(SUPPORT_SKE_TDES_128))
    if ((SKE_ALG_TDES_128 == g_ske_alg) || (SKE_ALG_TDES_192 == g_ske_alg) || (SKE_ALG_TDES_EEE_128 == g_ske_alg) || (SKE_ALG_TDES_EEE_192 == g_ske_alg))
    {
        return ske_tdes_update_blocks(in, NULL, bytes);
    }
    else
    {
        ;
    }
#endif

    return ske_lp_update_blocks_no_output(ctx, in, bytes);
}

/**
 * @brief           Internal helper function for SKE block updates
 * @param[in]       ctx                  - Pointer to SKE context
 * @param[in]       in                   - Pointer to input data
 * @param[out]      out                  - Pointer to output data
 * @param[in]       bytes                - Byte length of input/output data
 * @return          SKE_SUCCESS if successful, otherwise error code
 */
unsigned int ske_lp_update_blocks_internal_(ske_ctx_t *ctx, unsigned char *in, unsigned char *out, unsigned int bytes)
{
#if (defined(SUPPORT_SKE_TDES_128))
    if ((SKE_ALG_TDES_128 == g_ske_alg) || (SKE_ALG_TDES_192 == g_ske_alg) || (SKE_ALG_TDES_EEE_128 == g_ske_alg) || (SKE_ALG_TDES_EEE_192 == g_ske_alg))
    {
        return ske_tdes_update_blocks(in, out, bytes);
    }
    else
    {
        ;
    }
#endif

    return ske_lp_update_blocks_internal(ctx, in, out, bytes);
}

#ifdef SKE_LP_DMA_FUNCTION
/**
 * @brief           Initializes SKE configuration (DMA style)
 * @param[in]       alg                  - ske_lp algorithm
 * @param[in]       mode                 - ske_lp algorithm operation mode, like ECB,CBC,OFB,etc.
 * @param[in]       crypto               - encrypting or decrypting
 * @param[in]       key                  - key in bytes, must be a block
 * @param[in]       sp_key_idx           - index of secure port key, (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX],
 *                                         if the MSB(sp_key_idx) is 1, that means using low 128bit of the 256bit key
 * @param[in]       iv                   - iv in bytes
 * @return          SKE_SUCCESS if successful, otherwise error code
 * @note
 *        1. if mode is ECB, then there is no iv, in this case iv could be NULL
 *        2. this function is designed for ECB/CBC/CFB/OFB/CTR/XTS modes, and input/output unit is a block
 *        3. if key is from user input, please make sure key is not NULL(now sp_key_idx is useless),
 *           otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX]
 */
unsigned int ske_lp_dma_init(ske_alg_e alg, ske_mode_e mode, ske_crypto_e crypto, const unsigned char *key, unsigned short sp_key_idx, const unsigned char *iv)
{
    ske_lp_set_cpu_mode();
#if defined(SUPPORT_SKE_MODE_XTS)
    ske_lp_set_c_len_uint32(0); // just for XTS mode
#endif

    return ske_lp_init_internal(g_ske_ctx, alg, mode, crypto, key, sp_key_idx, iv, SKE_LP_DMA_ENABLE);
}

/**
 * @brief           Performs SKE encryption or decryption (DMA style)
 * @param[in]       in                   - Pointer to input data (plaintext or ciphertext)
 * @param[out]      out                  - Pointer to output data (ciphertext or plaintext)
 * @param[in]       words                - Word length of input/output data (must be multiples of block length)
 * @param[in]       callback             - Callback function pointer (can be NULL)
 * @return          SKE_SUCCESS if successful, otherwise error code
 * @note
 *        1. Designed for ECB/CBC/CFB/OFB/CTR/XTS modes, input/output unit is a block
 *        2. in and out can be the same buffer (output will overwrite input)
 *        3. words must be a multiple of block word length
 */
unsigned int ske_lp_dma_update_blocks(const unsigned int *in, unsigned int *out, unsigned int words, SKE_CALLBACK callback)
{
    if (0 == words)
    {
        return SKE_SUCCESS;
    }
    else if (words & (g_ske_ctx->block_words - 1))
    {
        return SKE_INPUT_INVALID;
    }
    else
    {
        printf("ske_lp_dma_update_blocks\r\n");
        return ske_lp_dma_operate(g_ske_ctx, in, out, words, words, callback);
    }
}

/**
 * @brief           Finalizes SKE operation (DMA style)
 * @return          SKE_SUCCESS if successful, otherwise error code
 * @note            If encryption or decryption is done, please call this (optional)
 */
unsigned int ske_lp_dma_final(void)
{
    memset_(g_ske_ctx, 0, sizeof(ske_ctx_t));

    return SKE_SUCCESS;
}

/**
 * @brief           Performs SKE encryption or decryption (DMA style, one-off)
 * @param[in]       alg                  - SKE algorithm
 * @param[in]       mode                 - SKE operation mode (ECB, CBC, OFB, etc.)
 * @param[in]       crypto               - Encrypt or decrypt operation
 * @param[in]       key                  - Key in bytes
 * @param[in]       sp_key_idx           - Secure port key index
 * @param[in]       iv                   - IV in bytes (must be a block)
 * @param[in]       in                   - Pointer to input data (plaintext or ciphertext)
 * @param[out]      out                  - Pointer to output data (ciphertext or plaintext)
 * @param[in]       words                - Word length of input/output data
 * @param[in]       callback             - Callback function pointer (can be NULL)
 * @return          SKE_SUCCESS if successful, otherwise error code
 * @note
 *        1. If mode is ECB, then there is no IV (iv can be NULL)
 *        2. Designed for ECB/CBC/CFB/OFB/CTR/XTS modes (input/output unit is a block)
 *        3. If key is from user input, ensure key is not NULL (sp_key_idx is unused)
 *        4. in and out can be the same buffer (output will overwrite input)
 *        5. words must be a multiple of block word length
 */
unsigned int ske_lp_dma_crypto(ske_alg_e alg, ske_mode_e mode, ske_crypto_e crypto, const unsigned char *key, unsigned short sp_key_idx, const unsigned char *iv,
                               const unsigned int *in, unsigned int *out, unsigned int words, SKE_CALLBACK callback)
{
    unsigned int ret;
    printf("ske_lp_dma_crypto\r\n");
    ret = ske_lp_dma_init(alg, mode, crypto, key, sp_key_idx, iv);
    if (SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }
    printf("ske_lp_dma_init\r\n");
    return ske_lp_dma_update_blocks(in, out, words, callback);
}
#endif
