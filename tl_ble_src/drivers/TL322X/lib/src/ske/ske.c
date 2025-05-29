/********************************************************************************************************
 * @file    ske.c
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
#include "lib/include/ske/ske.h"


static SKE_CTX g_ske_ctx[1];


//since hardware does not support 3DES
#if (defined(SUPPORT_SKE_TDES_128))
static SKE_ALG      g_ske_alg;           //hold current ske algorithm(for 3DES)
static SKE_MODE     g_ske_mode;          //hold input mode for 3DES
static SKE_CRYPTO   g_ske_crypto_action; //hold current block cipher crypto for 3DES
static unsigned int g_ske_key_buf[6];    //hold key for 3DES
static unsigned int g_ske_iv_buf[4];     //hold current IV for 3DES
#endif


#if (defined(SUPPORT_SKE_TDES_128))
/**
 * @brief       a=a+1.
 * @param[in]   a            - big integer a in bytes, big-endian.
 * @param[in]   bytes        - byte length of a.
 * @return      none
 * @note
  @verbatim
      -# 1.for CTR/CCM counter addition(big-endian).
  @endverbatim
 */
void ske_big_endian_add_uint8(unsigned char *a, unsigned int a_bytes, unsigned char b)
{
    int i;

    for (i = a_bytes; i > 0;) {
        a[--i] += b;
        if (a[i] < b) {
            b = 1;
        } else {
    #if 1
            b = 0; //for security
    #else
            break;
    #endif
        }
    }
}
#endif

/**
 * @brief       check whether the ske algorithm is valid or not.
 * @param[in]   ske_alg              - specific ske algorithm.
 * @return      0:success(valid)     other:error(invalid)
 */
unsigned char ske_lp_check_alg(SKE_ALG ske_alg)
{
    unsigned char ret;

    switch (ske_alg) {
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
 * @brief       check whether the ske algorithm mode is valid or not.
 * @param[in]   ske_alg              - specific ske algorithm.
 * @param[in]   ske_mode             - specific ske algorithm mode.
 * @return      0:success(valid)     other:error(invalid)
 */
unsigned char ske_lp_check_mode(SKE_ALG ske_alg, SKE_MODE ske_mode)
{
    unsigned char ret;

    switch (ske_mode) {
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

        //for DES/3DES, CAMC is not supported at present
#ifdef SUPPORT_SKE_MODE_CMAC
    case SKE_MODE_CMAC:
        switch (ske_alg) {
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

        //for DES/3DES, XTS, CCM and GCM mode are not supported due to the definition or standard
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
        switch (ske_alg) {
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
 * @brief       get block byte length for specific ske_lp alg.
 * @param[in]   ske_alg              - ske_lp algorithm.
 * @return      block byte length for ske_lp alg
 * @note
  @verbatim
      -# 1. please make sure ske_alg is valid.
  @endverbatim
 */
unsigned char ske_lp_get_block_byte_len(SKE_ALG ske_alg)
{
    unsigned char byteLen;

    switch (ske_alg) {
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
        byteLen = 16; //default alg SM4
    }

    return byteLen;
}

/**
 * @brief       get key byte length for specific ske_lp alg.
 * @param[in]   ske_alg              - ske_lp algorithm.
 * @return      key byte length for ske_lp alg
 * * @note
  @verbatim
      -# 1. please make sure the inputs are valid.
  @endverbatim
 */
unsigned char ske_lp_get_key_byte_len(SKE_ALG ske_alg)
{
    unsigned char byte_len;

    switch (ske_alg) {
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
        byte_len = 16; //default alg SM4
    }

    return byte_len;
}

/**
 * @brief       set ske_lp iv.
 * @param[in]   iv               - initial vector.
 * @param[in]   block_bytes      - byte length of current ske_lp block.
 * @return      none
 * @note
  @verbatim
      -# 1. please make sure ske_alg is valid.
  @endverbatim
 */
void ske_lp_set_iv(unsigned char *iv, unsigned int block_bytes)
{
    unsigned int tmp[4];

    if (((unsigned int)iv) & 3) {
        memcpy_(tmp, iv, block_bytes);
        ske_lp_set_iv_uint32(tmp, block_bytes / 4);
    } else {
        ske_lp_set_iv_uint32((unsigned int *)iv, block_bytes / 4);
    }
}

/**
 * @brief       ske_lp setting key
 * @param[in]   alg              - ske_lp algorithm.
 * @param[in]   key              - key in word buffer.
 * @param[in]   key_bytes        - key bytes.
 * @param[in]   key_idx          - key id.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.this function is common for CPU/DMA/DMA-LL.
      -# 2.if mode is ECB, then there is no iv, in this case iv could be NULL.
      -# 3.if mode is CMAC/CBC-MAC, the iv must be a block of all zero.
      -# 4.if key is from user input, please make sure the argument key is not NULL(now sp_key_idx is useless),
        otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX].
  @endverbatim
 */
void ske_lp_set_key(SKE_ALG alg, const unsigned char *key, unsigned short key_bytes, unsigned short key_idx)
{
    unsigned int tmp[8];

    memcpy_(tmp, key, key_bytes);

    //for 3DES-2key, set key3=key1
    switch (alg) {
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
 * @brief       ske_lp init config
 * @param[in]   ctx              - SKE_CTX context pointer.
 * @param[in]   alg              - ske_lp algorithm.
 * @param[in]   mode             - ske_lp algorithm operation mode, like ECB,CBC,OFB,etc.
 * @param[in]   crypto           - encrypting or decrypting.
 * @param[in]   key              - key in bytes.
 * @param[in]   sp_key_idx       - index of secure port key, (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX],
 *                                 if the MSB(sp_key_idx) is 1, that means using low 128bit of the 256bit key.
 * @param[in]   iv               - iv in bytes, must be a block.
 * @param[in]   dma_en           - for DMA mode(not 0) or not(0).
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.this function is common for CPU/DMA/DMA-LL.
      -# 2.if mode is ECB, then there is no iv, in this case iv could be NULL.
      -# 3.if mode is CMAC/CBC-MAC, the iv must be a block of all zero.
      -# 4.if key is from user input, please make sure the argument key is not NULL(now sp_key_idx is useless),
        otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX].
  @endverbatim
 */
unsigned int ske_lp_init_internal(SKE_CTX *ctx, SKE_ALG alg, SKE_MODE mode, SKE_CRYPTO crypto, const unsigned char *key, unsigned short sp_key_idx, unsigned char *iv, unsigned int dma_en)
{
    unsigned int key_bytes;
    (void)sp_key_idx;

    if (NULL == ctx) {
        return SKE_BUFFER_NULL;
    } else if (SKE_SUCCESS != ske_lp_check_alg(alg)) {
        return SKE_INPUT_INVALID;
    } else if (SKE_SUCCESS != ske_lp_check_mode(alg, mode)) {
        return SKE_INPUT_INVALID;
    } else if (crypto > SKE_CRYPTO_DECRYPT) {
        return SKE_INPUT_INVALID;
    } else if (NULL == key) //secure port
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
    } else {
        ;
    }

    if (SKE_MODE_ECB == mode) {
        iv = NULL;
    } else if (NULL == iv) {
        return SKE_BUFFER_NULL; //SKE_ERROR;//
    } else {
        ;
    }

#if (defined(SUPPORT_SKE_TDES_128))
    /******************* because hw does not support TDES ******************/

    g_ske_alg = alg; //very important!!! to distinguish 3DES and other algorithm, if the MACRO is available

    if ((SKE_ALG_TDES_128 == alg) || (SKE_ALG_TDES_192 == alg) || (SKE_ALG_TDES_EEE_128 == alg) ||
        (SKE_ALG_TDES_EEE_192 == alg)) {
        if ((SKE_ALG_TDES_128 == alg) || (SKE_ALG_TDES_EEE_128 == alg)) {
            memcpy_(g_ske_key_buf, key, 16);
            memcpy_(g_ske_key_buf + 16 / 4, key, 8);
        } else {
            memcpy_(g_ske_key_buf, key, 24);
        }

        g_ske_ctx->block_bytes = ske_lp_get_block_byte_len(alg);
        g_ske_ctx->block_words = g_ske_ctx->block_bytes >> 2;

        g_ske_mode          = mode;
        g_ske_crypto_action = crypto;
        alg                 = SKE_ALG_DES;

        if (SKE_MODE_ECB != mode) {
            memcpy_(g_ske_iv_buf, iv, 8);
            mode = SKE_MODE_ECB;
        } else {
            ;
        }
    } else {
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

    //set iv or nonce
    if (NULL != iv) {
        ske_lp_set_iv(iv, ctx->block_bytes);
    } else {
        ;
    }

    if (NULL != key) //key is from user input
    {
        // key1
        key_bytes = ske_lp_get_key_byte_len(alg);
        ske_lp_set_key(alg, key, key_bytes, 1);
    } else //key is from secure port
    {
        //TODO. set secure port.
    }

    return ske_lp_expand_key(dma_en);
}

/**
 * @brief       ske_lp init config(CPU style)
 * @param[in]   alg              - ske_lp algorithm.
 * @param[in]   mode             - ske_lp algorithm operation mode, like ECB,CBC,OFB,etc.
 * @param[in]   crypto           - encrypting or decrypting.
 * @param[in]   key              - key in bytes.
 * @param[in]   sp_key_idx       - index of secure port key, (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX],
 *                                 if the MSB(sp_key_idx) is 1, that means using low 128bit of the 256bit key.
 * @param[in]   iv               - iv in bytes, must be a block.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.if mode is ECB, then there is no iv, in this case iv could be NULL.
      -# 2.this function is designed for ECB/CBC/CFB/OFB/CTR/XTS modes, and input/output unit must be a block.
      -# 3.if key is from user input, please make sure the argument key is not NULL(now sp_key_idx is useless),
        otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX].
  @endverbatim
 */
unsigned int ske_lp_init(SKE_ALG alg, SKE_MODE mode, SKE_CRYPTO crypto, unsigned char *key, unsigned short sp_key_idx, unsigned char *iv)
{
    ske_lp_set_cpu_mode();
#if defined(SUPPORT_SKE_MODE_XTS)
    ske_lp_set_c_len_uint32(0); //just for XTS mode
#endif

    return ske_lp_init_internal(g_ske_ctx, alg, mode, crypto, key, sp_key_idx, iv, SKE_LP_DMA_DISABLE);
}


#if (defined(SUPPORT_SKE_TDES_128))
/**
 * @brief       ske 3des encryption or decryption(CPU style)
 * @param[in]   in               - plaintext or ciphertext.
 * @param[out]  out              - ciphertext or plaintext.
 * @param[in]   bytes            - byte length of input or output.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.this function is designed for ECB/CBC/CFB/OFB/CTR modes, and input/output unit must be a block.
      -# 2.to save memory, in and out could be the same buffer, in this case, the output will
        cover the input.
      -# 3.bytes must be a multiple of block byte length.
  @endverbatim
 */
unsigned int ske_tdes_update_blocks(unsigned char *in, unsigned char *out, unsigned int bytes)
{
    unsigned int   tmp_in[2], tmp_out[2];
    unsigned int   tmp_iv[2];
    unsigned char *p_out = out;
    unsigned int   is_EEE;
    unsigned int   i;
    unsigned int   ret;

    if (NULL == in) {
        return SKE_BUFFER_NULL;
    } else if (bytes & (g_ske_ctx->block_bytes - 1)) {
        return SKE_INPUT_INVALID;
    } else if (0 == bytes) {
        return SKE_SUCCESS;
    } else {
        ;
    }

    if ((SKE_ALG_TDES_128 == g_ske_alg) || (SKE_ALG_TDES_192 == g_ske_alg)) {
        is_EEE = 0;
    } else {
        is_EEE = 1;
    }

    //input one block ---> calculating ---> output one block
    for (i = 0; i < bytes; i += g_ske_ctx->block_bytes) {
        /************* first **************/
        memcpy_(tmp_in, in, g_ske_ctx->block_bytes);

        if ((SKE_MODE_CFB == g_ske_mode) || (SKE_MODE_OFB == g_ske_mode) || (SKE_MODE_CTR == g_ske_mode)) {
            ret = tdes_ecb_update_one_block(is_EEE, g_ske_key_buf, SKE_CRYPTO_ENCRYPT, g_ske_iv_buf, tmp_iv);
            if (SKE_SUCCESS != ret) {
                ret = SKE_ERROR;
                goto END;
            } else {
                ;
            }

            tmp_out[0] = tmp_iv[0] ^ tmp_in[0];
            tmp_out[1] = tmp_iv[1] ^ tmp_in[1];
        } else                                                                               //ECB or CBC
        {
            if ((SKE_MODE_CBC == g_ske_mode) && (SKE_CRYPTO_ENCRYPT == g_ske_crypto_action)) //CBC encrypt
            {
                tmp_in[0] ^= g_ske_iv_buf[0];
                tmp_in[1] ^= g_ske_iv_buf[1];
            } else {
                ;
            }

            ret = tdes_ecb_update_one_block(is_EEE, g_ske_key_buf, g_ske_crypto_action, tmp_in, tmp_out);
            if (SKE_SUCCESS != ret) {
                ret = SKE_ERROR;
                goto END;
            } else {
                ;
            }

            if ((SKE_MODE_CBC == g_ske_mode) && (SKE_CRYPTO_DECRYPT == g_ske_crypto_action)) //CBC decrypt
            {
                tmp_out[0] ^= g_ske_iv_buf[0];
                tmp_out[1] ^= g_ske_iv_buf[1];
            } else {
                ;
            }
        }

        //update iv
        if (SKE_MODE_CTR == g_ske_mode) {
            ske_big_endian_add_uint8((unsigned char *)g_ske_iv_buf, g_ske_ctx->block_bytes, 1);
        } else if (SKE_MODE_OFB == g_ske_mode) {
            uint32_copy(g_ske_iv_buf, tmp_iv, g_ske_ctx->block_words);
        } else if ((SKE_MODE_CBC == g_ske_mode) || (SKE_MODE_CFB == g_ske_mode)) {
            if (SKE_CRYPTO_ENCRYPT == g_ske_crypto_action) {
                uint32_copy(g_ske_iv_buf, tmp_out, g_ske_ctx->block_words);
            } else {
                uint32_copy(g_ske_iv_buf, tmp_in, g_ske_ctx->block_words);
            }
        } else {
            ;
        }

        //output
        if (out) {
            memcpy_(out, tmp_out, g_ske_ctx->block_bytes);
            out += g_ske_ctx->block_bytes;
        } else {
            ;
        }

        in += g_ske_ctx->block_bytes;
    }

END:

    if (SKE_ERROR == ret) {
        uint32_clear(g_ske_key_buf, sizeof(g_ske_key_buf) / 4);
        uint32_clear(g_ske_iv_buf, sizeof(g_ske_iv_buf) / 4);
        if (p_out) {
            memset_(p_out, 0, bytes);
        } else {
            ;
        }
    } else {
        ;
    }

    return ret;
}
#endif

/**
 * @brief       ske 3des encryption or decryption(CPU style)
 * @param[in]   in               - plaintext or ciphertext.
 * @param[out]  out              - ciphertext or plaintext.
 * @param[in]   bytes            - byte length of input or output.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.this function is designed for ECB/CBC/CFB/OFB/CTR modes, and input/output unit must be a block.
      -# 2.to save memory, in and out could be the same buffer, in this case, the output will
        cover the input.
      -# 3.bytes must be a multiple of block byte length.
  @endverbatim
 */
unsigned int ske_lp_update_blocks(unsigned char *in, unsigned char *out, unsigned int bytes)
{
    if ((NULL == in) || (NULL == out)) {
        return SKE_BUFFER_NULL;
    } else if (bytes & (g_ske_ctx->block_bytes - 1)) {
        return SKE_INPUT_INVALID;
    } else if (0 == bytes) {
        return SKE_SUCCESS;
    } else {
        ;
    }

#if (defined(SUPPORT_SKE_TDES_128))
    if ((SKE_ALG_TDES_128 == g_ske_alg) || (SKE_ALG_TDES_192 == g_ske_alg) ||
        (SKE_ALG_TDES_EEE_128 == g_ske_alg) || (SKE_ALG_TDES_EEE_192 == g_ske_alg)) {
        return ske_tdes_update_blocks(in, out, bytes);
    } else {
        ;
    }
#endif

    return ske_lp_update_blocks_internal(g_ske_ctx, in, out, bytes);
}

/**
 * @brief       ske_lp finish.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.if encryption or decryption is done, please call this(optional).
  @endverbatim
 */
unsigned int ske_lp_final(void)
{
    memset_(g_ske_ctx, 0, sizeof(SKE_CTX));

#if (defined(SUPPORT_SKE_TDES_128))
    g_ske_alg           = SKE_ALG_DES;
    g_ske_crypto_action = SKE_CRYPTO_ENCRYPT;
    g_ske_mode          = SKE_MODE_ECB;
    uint32_clear(g_ske_key_buf, sizeof(g_ske_key_buf) / 4);
    uint32_clear(g_ske_iv_buf, sizeof(g_ske_iv_buf) / 4);
#endif

    return SKE_SUCCESS;
}

/**
 * @brief       ske_lp encrypting or decrypting(CPU style, one-off style)
 * @param[in]   alg              - ske_lp algorithm.
 * @param[in]   mode             - ske_lp algorithm operation mode, like ECB,CBC,OFB,etc.
 * @param[in]   crypto           - encrypting or decrypting.
 * @param[in]   key              - key in bytes.
 * @param[in]   sp_key_idx       - index of secure port key, (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX],
 *                                 if the MSB(sp_key_idx) is 1, that means using low 128bit of the 256bit key.
 * @param[in]   iv               - iv in bytes, must be a block.
 * @param[in]   in               - plaintext or ciphertext.
 * @param[in]   out              - ciphertext or plaintext.
 * @param[in]   bytes            - byte length of input or output.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.if mode is ECB, then there is no iv, in this case iv could be NULL.
      -# 2.this function is designed for ECB/CBC/CFB/OFB/CTR/XTS modes, and input/output unit is a block.
      -# 3.if key is from user input, please make sure the argument key is not NULL(now sp_key_idx is useless),
        otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX].
      -# 4.ito save memory, in and out could be the same buffer, in this case, the output will
        cover the input.
      -# 5.bytes must be a multiple of block byte length.
  @endverbatim
 */
unsigned int ske_lp_crypto(SKE_ALG alg, SKE_MODE mode, SKE_CRYPTO crypto, unsigned char *key, unsigned short sp_key_idx, unsigned char *iv, unsigned char *in, unsigned char *out, unsigned int bytes)
{
    unsigned int ret;

    ske_lp_set_cpu_mode();
#if defined(SUPPORT_SKE_MODE_XTS)
    ske_lp_set_c_len_uint32(0); //just for XTS mode
#endif

    ret = ske_lp_init_internal(g_ske_ctx, alg, mode, crypto, key, sp_key_idx, iv, SKE_LP_DMA_DISABLE);
    if (SKE_SUCCESS != ret) {
        return ret;
    } else {
        return ske_lp_update_blocks(in, out, bytes);
    }
}

unsigned int ske_lp_update_blocks_no_output_(SKE_CTX *ctx, unsigned char *in, unsigned int bytes)
{
#if (defined(SUPPORT_SKE_TDES_128))
    if ((SKE_ALG_TDES_128 == g_ske_alg) || (SKE_ALG_TDES_192 == g_ske_alg) ||
        (SKE_ALG_TDES_EEE_128 == g_ske_alg) || (SKE_ALG_TDES_EEE_192 == g_ske_alg)) {
        return ske_tdes_update_blocks(in, NULL, bytes);
    } else {
        ;
    }
#endif

    return ske_lp_update_blocks_no_output(ctx, in, bytes);
}

unsigned int ske_lp_update_blocks_internal_(SKE_CTX *ctx, unsigned char *in, unsigned char *out, unsigned int bytes)
{
#if (defined(SUPPORT_SKE_TDES_128))
    if ((SKE_ALG_TDES_128 == g_ske_alg) || (SKE_ALG_TDES_192 == g_ske_alg) ||
        (SKE_ALG_TDES_EEE_128 == g_ske_alg) || (SKE_ALG_TDES_EEE_192 == g_ske_alg)) {
        return ske_tdes_update_blocks(in, out, bytes);
    } else {
        ;
    }
#endif

    return ske_lp_update_blocks_internal(ctx, in, out, bytes);
}


#ifdef SKE_LP_DMA_FUNCTION
/**
 * @brief       ske_lp init config(DMA style)
 * @param[in]   alg              - ske_lp algorithm.
 * @param[in]   mode             - ske_lp algorithm operation mode, like ECB,CBC,OFB,etc.
 * @param[in]   crypto           - encrypting or decrypting.
 * @param[in]   key              - key in bytes.
 * @param[in]   sp_key_idx       - index of secure port key, (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX],
 *                                 if the MSB(sp_key_idx) is 1, that means using low 128bit of the 256bit key.
 * @param[in]   iv               - iv in bytes, must be a block.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.if mode is ECB, then there is no iv, in this case iv could be NULL.
      -# 2.this function is designed for ECB/CBC/CFB/OFB/CTR/XTS modes, and input/output unit is a block.
      -# 3.if key is from user input, please make sure the argument key is not NULL(now sp_key_idx is useless),
        otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX].
  @endverbatim
 */
unsigned int ske_lp_dma_init(SKE_ALG alg, SKE_MODE mode, SKE_CRYPTO crypto, unsigned char *key, unsigned short sp_key_idx, unsigned char *iv)
{
    ske_lp_set_cpu_mode();
    #if defined(SUPPORT_SKE_MODE_XTS)
    ske_lp_set_c_len_uint32(0); //just for XTS mode
    #endif

    return ske_lp_init_internal(g_ske_ctx, alg, mode, crypto, key, sp_key_idx, iv, SKE_LP_DMA_ENABLE);
}

/**
 * @brief       ske_lp encryption or decryption(DMA style)
 * @param[in]   in               - plaintext or ciphertext.
 * @param[out]  out              - ciphertext or plaintext.
 * @param[in]   words            - word length of input or output, must be multiples of block length.
 * @param[in]   callback         - callback function pointer, this could be NULL, means doing nothing.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.this function is designed for ECB/CBC/CFB/OFB/CTR/XTS modes, and input/output unit is a block.
      -# 2.to save memory, in and out could be the same buffer, in this case, the output will
        cover the input.
      -# 3.words must be a multiple of block word length.
  @endverbatim
 */
unsigned int ske_lp_dma_update_blocks(unsigned int *in, unsigned int *out, unsigned int words, SKE_CALLBACK callback)
{
    if (0 == words) {
        return SKE_SUCCESS;
    } else if (words & (g_ske_ctx->block_words - 1)) {
        return SKE_INPUT_INVALID;
    } else {
        return ske_lp_dma_operate(g_ske_ctx, in, out, words, words, callback);
    }
}

/**
 * @brief       ske_lp finish(DMA style)
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.if encryption or decryption is done, please call this(optional).
  @endverbatim
 */
unsigned int ske_lp_dma_final(void)
{
    memset_(g_ske_ctx, 0, sizeof(SKE_CTX));

    return SKE_SUCCESS;
}

/**
 * @brief       ske_lp encryption or decryption(DMA style)
 * @param[in]   alg              - ske_lp algorithm.
 * @param[in]   mode             - ske_lp algorithm operation mode, like ECB,CBC,OFB,etc.
 * @param[in]   crypto           - encrypting or decrypting.
 * @param[in]   key              - key in bytes.
 * @param[in]   sp_key_idx       - index of secure port key, (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX],
 *                                 if the MSB(sp_key_idx) is 1, that means using low 128bit of the 256bit key.
 * @param[in]   iv               - iv in bytes, must be a block.
 * @param[in]   in               - plaintext or ciphertext.
 * @param[out]  out              - ciphertext or plaintext.
 * @param[in]   words            - word length of input or output.
 * @param[in]   callback         - callback function pointer, this could be NULL, means do nothing.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.if mode is ECB, then there is no iv, in this case iv could be NULL.
      -# 2.this function is designed for ECB/CBC/CFB/OFB/CTR/XTS modes, and input/output unit is a block.
      -# 3.if key is from user input, please make sure key is not NULL(now sp_key_idx is useless),
        otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX].
      -# 4.to save memory, in and out could be the same buffer, in this case, the output will
        cover the input.
      -# 5.words must be a multiple of block word length.
  @endverbatim
 */
unsigned int ske_lp_dma_crypto(SKE_ALG alg, SKE_MODE mode, SKE_CRYPTO crypto, unsigned char *key, unsigned short sp_key_idx, unsigned char *iv, unsigned int *in, unsigned int *out, unsigned int words, SKE_CALLBACK callback)
{
    unsigned int ret;

    ret = ske_lp_dma_init(alg, mode, crypto, key, sp_key_idx, iv);
    if (SKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    return ske_lp_dma_update_blocks(in, out, words, callback);
}
#endif
