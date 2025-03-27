/********************************************************************************************************
 * @file    chacha20_poly1305.c
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
#include "lib/include/chacha20_poly1305/chacha20_poly1305.h"
#include "lib/include/chacha20_poly1305/chacha20_poly1305_portable.h"
#include "lib/include/crypto_common/utility.h"


/**
 * @brief       input message.
 * @param[in]   crypto             - encrypting or decrypting.
 * @param[in]   key                - key in bytes.
 * @param[in]   constant           - constant of nonce.
 * @param[in]   iv                 - iv of nonce.
 * @param[in]   stage              - data interleave stage.
 * @param[in]   dma_en             - for DMA mode(not 0) or not(0).
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. this function is common for CPU/DMA.
  @endverbatim
 */
unsigned int chacha20_poly1305_init_internal(CHACHA20_POLY1305_CRYPTO crypto, unsigned char key[32], unsigned int constant, 
    unsigned char iv[8], CHACHA20_POLY1305_SEC_STAGE stage, unsigned int dma_en)
{
    if(NULL == key || NULL == iv)
    {
        return CHACHA20_POLY1305_BUFFER_NULL;
    }
    else if(CHACHA20_POLY1305_STAGE_LAST < stage)
    {
        return CHACHA20_POLY1305_INPUT_INVALID;
    }
    else if(CHACHA20_POLY1305_CRYPTO_DECRYPT < crypto)
    {
        return CHACHA20_POLY1305_INPUT_INVALID;
    }
    else
    {;}

    chacha20_poly1305_wait_core_idle();

    if(CHACHA20_POLY1305_DMA_ENABLE == dma_en)
    {
        chacha20_poly1305_set_dma_mode();
    }
    else
    {
        chacha20_poly1305_set_cpu_mode();
    }
    
    chacha20_poly1305_set_crypto(crypto);
    chacha20_poly1305_set_sec_stage(stage);
    chacha20_poly1305_set_key(key);
    chacha20_poly1305_set_const(constant);
    chacha20_poly1305_set_iv(iv);
    
    return CHACHA20_POLY1305_SUCCESS;
}

/**
 * @brief       hacha20_poly1305 init (CPU style).
 * @param[in]   ctx             - CHACHA20_POLY1305_CTX context pointer.
 * @param[in]   crypto          - encrypting or decrypting.
 * @param[in]   key             - key in bytes.
 * @param[in]   constant        - constant of nonce.
 * @param[in]   iv              - iv of nonce.
 * @param[in]   aad             - all aad.
 * @param[in]   aad_bytes       - byte length of aad.
 * @return      0:success     other:error
 */
unsigned int chacha20_poly1305_init(CHACHA20_POLY1305_CTX *ctx, CHACHA20_POLY1305_CRYPTO crypto, unsigned char key[32], unsigned int constant, 
    unsigned char iv[8], unsigned char *aad , unsigned long long aad_bytes)
{
    unsigned int ret;
    unsigned long long round = (aad_bytes+3)>>2;
    unsigned int remainder = aad_bytes & 3;
    unsigned int tmp[1] = {0};

    ctx->cur_cnt = 1;
    ctx->payload_bytes = 0;
    ctx->aad_bytes = aad_bytes;
    memset_(ctx->cur_tag, 0, 17);

    if((NULL == aad) && (0 != aad_bytes))
    {
        return CHACHA20_POLY1305_BUFFER_NULL;
    }
    else
    {;}

    if(0 == remainder)
    {
        remainder = 4;
    }
    else
    {;}
    
    ret = chacha20_poly1305_init_internal(crypto, key, constant, iv, CHACHA20_POLY1305_STAGE_INIT, CHACHA20_POLY1305_DMA_DISABLE);
    if(CHACHA20_POLY1305_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

#ifdef CONFIG_CHACHA20_POLY1305_SUPPORT_MUL_THREAD
    ctx->crypto = crypto;
    memcpy_(ctx->key, key, 32);
    memcpy_(ctx->iv, iv, 8);
    ctx->constant = constant;
#else
    ctx->crypto = crypto;
#endif

    if(0 != aad_bytes)
    {
        chacha20_poly1305_set_length(aad_bytes, 0);
        chacha20_poly1305_start();
        chacha20_poly1305_set_last_word(0);
        if(round > 1)
        {
            chacha20_poly1305_set_input_word((unsigned int *)aad, round - 1);
        }
        else
        {;}
        

        //last aad word
        chacha20_poly1305_set_last_word(1);
        if((NULL != aad) && (0 != aad_bytes))
        {
            memcpy_((unsigned char *)tmp, aad + ((round-1)<<2), remainder);

        }
        else
        {
            tmp[0] = 0;
        }
        chacha20_poly1305_set_input_word(tmp, 1);

        chacha20_poly1305_wait_core_idle();
        ret = chacha20_poly1305_get_error();
        if(CHACHA20_POLY1305_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}
        
        chacha20_poly1305_get_current_tag(ctx->cur_tag);
    }
    else
    {;}

    return CHACHA20_POLY1305_SUCCESS;
}

/**
 * @brief       chacha20_poly1305 update message excluding the last data (CPU mode).
 * @param[in]   ctx                - CHACHA20_POLY1305_CTX context pointer.
 * @param[in]   payload_in         - payload of some blocks, excluding last block(or message tail).
 * @param[out]  payload_out        - ciphertext or plaintext.
 * @param[in]   payload_bytes      - byte length of payload, must be a multiple of 64, could be 0.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. if the whole payload length is 0, this case is supported. in this case, payload_in and
        payload_out could set NULL.
  @endverbatim
 */
unsigned int chacha20_poly1305_update_excluding_last_data(CHACHA20_POLY1305_CTX *ctx, unsigned char *payload_in, 
    unsigned char *payload_out, unsigned long long payload_bytes)
{
    unsigned int ret = 0;
    unsigned long long round = (payload_bytes+63)/64;
    unsigned int remainder = payload_bytes & 63;

    if(payload_bytes & 63)
    {
        return CHACHA20_POLY1305_INPUT_INVALID;
    }
    else if(NULL == ctx || ((NULL == payload_out) && (0 != payload_bytes)) || ((NULL == payload_out) && (0 != payload_bytes)))
    {
        return CHACHA20_POLY1305_BUFFER_NULL;
    }
    else if(0 == payload_bytes)
    {
        return CHACHA20_POLY1305_SUCCESS;
    }
    {;}

    ctx->payload_bytes += payload_bytes;

#ifdef CONFIG_CHACHA20_POLY1305_SUPPORT_MUL_THREAD  
    ret = chacha20_poly1305_init_internal(ctx->crypto, ctx->key, ctx->constant, ctx->iv, CHACHA20_POLY1305_STAGE_MIDDLE, CHACHA20_POLY1305_DMA_DISABLE);
    if(CHACHA20_POLY1305_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}
#else
    chacha20_poly1305_set_sec_stage(CHACHA20_POLY1305_STAGE_MIDDLE);
#endif

    chacha20_poly1305_set_length_clean_aad_last(ctx->aad_bytes, payload_bytes);
    chacha20_poly1305_set_cnt(ctx->cur_cnt);
    chacha20_poly1305_set_tag_in(ctx->cur_tag);

    chacha20_poly1305_start();
    chacha20_poly1305_set_last_word(0);
    
    if(round > 1)
    {
        ret = chacha20_poly1305_update_blocks(payload_in, payload_out, payload_bytes - 64);
        if(CHACHA20_POLY1305_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        payload_in += payload_bytes - 64;
        payload_out += payload_bytes - 64;
    }
    else
    {;}

    if(0 == remainder)
    {
        remainder = 64;
    }
    else
    {;}

    ret = chacha20_poly1305_update_last_block(payload_in, payload_out, remainder);  
    if(CHACHA20_POLY1305_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ctx->cur_cnt = chacha20_poly1305_get_current_cnt(); 
    chacha20_poly1305_get_current_tag(ctx->cur_tag);

    return CHACHA20_POLY1305_SUCCESS;
}

/**
 * @brief       chacha20_poly1305 update message including the last data (CPU mode), and get the tag.
 * @param[in]   ctx                - CHACHA20_POLY1305_CTX context pointer.
 * @param[in]   payload_in         - payload, including last block(or message tail).
 * @param[in]   payload_out        - ciphertext or plaintext.
 * @param[in]   payload_bytes      - byte length of payload.
 * @param[out]  tag                - chacha20_poly1305 tag, occupies 16 bytes.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. if the all payload length is 0, this case is supported. in this case, payload_in and
 *        payload_out could set NULL and payload_bytes should be 0.
  @endverbatim
 */
unsigned int chacha20_poly1305_update_including_last_data(CHACHA20_POLY1305_CTX *ctx, unsigned char *payload_in, 
    unsigned char *payload_out, unsigned long long payload_bytes, unsigned char tag[16])
{   
    unsigned int ret;
    unsigned int remainder = payload_bytes & 63;
    unsigned long long round = (payload_bytes+63)/64;

    if(NULL == ctx || ((NULL == payload_out) && (0 != payload_bytes)) || ((NULL == payload_out) && (0 != payload_bytes)))
    {
        return CHACHA20_POLY1305_BUFFER_NULL;
    }
    else
    {;}

    if((0 == remainder) && (0 != payload_bytes))
    {
        remainder = 64;
    }
    else
    {;}

    ctx->payload_bytes += payload_bytes;

#ifdef CONFIG_CHACHA20_POLY1305_SUPPORT_MUL_THREAD
    ret = chacha20_poly1305_init_internal(ctx->crypto, ctx->key, ctx->constant, ctx->iv, CHACHA20_POLY1305_STAGE_LAST, CHACHA20_POLY1305_DMA_DISABLE);
    if(CHACHA20_POLY1305_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}
#else
    chacha20_poly1305_set_sec_stage(CHACHA20_POLY1305_STAGE_LAST);
#endif

    chacha20_poly1305_set_cnt(ctx->cur_cnt);
    chacha20_poly1305_set_tag_in(ctx->cur_tag);
    chacha20_poly1305_set_length_clean_aad_last(ctx->aad_bytes, ctx->payload_bytes);

    chacha20_poly1305_start();
    if(0 != payload_bytes)
    {
        chacha20_poly1305_set_last_word(0);
        ret = chacha20_poly1305_update_blocks(payload_in, payload_out, payload_bytes - remainder);
        if(CHACHA20_POLY1305_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        payload_in += (round-1)<<6;
        payload_out += (round-1)<<6;

        ret = chacha20_poly1305_update_last_block(payload_in, payload_out, remainder);
        if(CHACHA20_POLY1305_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}
    }
    else
    {
        chacha20_poly1305_wait_core_idle();
    }

    chacha20_poly1305_get_current_tag(ctx->cur_tag);

    if(CHACHA20_POLY1305_CRYPTO_ENCRYPT == ctx->crypto)
    {
    memcpy_(tag, ctx->cur_tag, 16);
    }
    else
    {
        ret = memcmp_(tag, ctx->cur_tag, 16);
        if(0 != ret)
        {
            return CHACHA20_POLY1305_VERIFY_FAIL;
        }
        else
        {;}
    }

    return CHACHA20_POLY1305_SUCCESS;
}


/**
 * @brief       chacha20_poly1305 (CPU style, one-off style).
 * @param[in]   crypto             - encrypting or decrypting.
 * @param[in]   key                - key in bytes.
 * @param[in]   constant           - constant of nonce.
 * @param[in]   iv                 - iv of nonce.
 * @param[in]   aad                - key in bytes.
 * @param[in]   aad_bytes          - byte length of aad.
 * @param[in]   payload_in         - payload of some blocks, excluding last block(or message tail).
 * @param[out]  payload_out        - ciphertext or plaintext.
 * @param[in]   payload_bytes      - byte length of payload, could be 0.
 * @param[out]  tag                - chacha20_poly1305 tag, occupies 16 bytes.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. aad_bytes and payload_bytes could be any value. if length bytes is 0, could set pointer is NULL.
  @endverbatim
 */
unsigned int chacha20_poly1305(CHACHA20_POLY1305_CRYPTO crypto, unsigned char key[32], unsigned int constant, 
    unsigned char iv[8], unsigned char *aad , unsigned long long aad_bytes, unsigned char *payload_in, unsigned char *payload_out, unsigned long long payload_bytes, unsigned char tag[16])
{
    unsigned int ret;
    CHACHA20_POLY1305_CTX ctx[1];
    
    ret = chacha20_poly1305_init(ctx, crypto, key, constant, iv, aad, aad_bytes);
    if(CHACHA20_POLY1305_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = chacha20_poly1305_update_including_last_data(ctx, payload_in, payload_out, payload_bytes, tag);
    
    return ret;
}



#ifdef CHACHA20_POLY1305_DMA_FUNCTION
#include "driver.h"

/**
 * @brief       chacha20_poly1305 init (DMA style).
 * @param[in]   ctx                - CHACHA20_POLY1305_DMA_CTX context pointer.
 * @param[in]   crypto             - encrypting or decrypting.
 * @param[in]   key                - key in bytes.
 * @param[in]   constant           - constant of nonce.
 * @param[in]   iv                 - iv of nonce.
 * @param[in]   aad                - key in bytes.
 * @param[in]   aad_bytes          - byte length of aad.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. in dma mode, the byte length of aad occupy 32 bit.
  @endverbatim
 */
unsigned int chacha20_poly1305_dma_init(CHACHA20_POLY1305_DMA_CTX *ctx, CHACHA20_POLY1305_CRYPTO crypto, unsigned char key[32], 
    unsigned int constant, unsigned char iv[8], unsigned int *aad , unsigned int aad_bytes)
{
    unsigned int ret;

    ctx->cur_cnt = 1;
    ctx->aad_bytes = aad_bytes;
    ctx->payload_bytes = 0;
    memset_(ctx->cur_tag, 0, 17);

    if((NULL == aad) && (0 != aad_bytes))
    {
        return CHACHA20_POLY1305_BUFFER_NULL;
    }
    else
    {;}

    ret = chacha20_poly1305_init_internal(crypto, key, constant, iv, CHACHA20_POLY1305_STAGE_INIT, CHACHA20_POLY1305_DMA_ENABLE);
    if(CHACHA20_POLY1305_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

#ifdef CONFIG_CHACHA20_POLY1305_SUPPORT_MUL_THREAD
    ctx->crypto = crypto;
    memcpy_(ctx->key, key, 32);
    memcpy_(ctx->iv, iv, 8);
    ctx->constant = constant;
#else
    ctx->crypto = crypto;
#endif

    if(0 != aad_bytes)
    {
        chacha20_poly1305_set_length(aad_bytes, 0);
        chacha20_poly1305_dma_set_aad(aad, aad_bytes);
        chacha20_poly1305_dma_set_payload(NULL, 0);
        
        chacha20_poly1305_tx_dma(DMA0, (unsigned int)aad, aad_bytes);

        chacha20_poly1305_start();
        chacha20_poly1305_wait_core_idle();
        ret = chacha20_poly1305_get_error();
        if(CHACHA20_POLY1305_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}
        
        chacha20_poly1305_get_current_tag(ctx->cur_tag);
    }
    else
    {;}
    
    return CHACHA20_POLY1305_SUCCESS;
}

/**
 * @brief       chacha20_poly1305 update message excluding the last data (DMA mode).
 * @param[in]   ctx                - CHACHA20_POLY1305_DMA_CTX context pointer.
 * @param[in]   payload_in         - payload of some blocks, excluding last block(or message tail).
 * @param[out]  payload_out        - ciphertext or plaintext.
 * @param[in]   payload_bytes      - byte words of payload, must be a multiple of 16, could be 0.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. if the whole payload length is 0, this case is supported. in this case, payload_in and
        payload_out could set NULL.
  @endverbatim
 */
unsigned int chacha20_poly1305_dma_update_excluding_last_data(CHACHA20_POLY1305_DMA_CTX *ctx, unsigned int *payload_in, unsigned int *payload_out, 
    unsigned int payload_bytes)
{
    unsigned int ret;

    if(payload_bytes & 63)
    {
        return CHACHA20_POLY1305_INPUT_INVALID;
    }
    else if(NULL == ctx || ((NULL == payload_out) && (0 != payload_bytes)) || ((NULL == payload_out) && (0 != payload_bytes)))
    {
        return CHACHA20_POLY1305_BUFFER_NULL;
    }
    else if(0 == payload_bytes)
    {
        return CHACHA20_POLY1305_SUCCESS;
    }
    {;}
    
    ctx->payload_bytes += payload_bytes;

#ifdef CONFIG_CHACHA20_POLY1305_SUPPORT_MUL_THREAD
    ret = chacha20_poly1305_init_internal(ctx->crypto, ctx->key, ctx->constant, ctx->iv, CHACHA20_POLY1305_STAGE_MIDDLE, CHACHA20_POLY1305_DMA_ENABLE);
    if(CHACHA20_POLY1305_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}
#else
    chacha20_poly1305_set_sec_stage(CHACHA20_POLY1305_STAGE_MIDDLE);
#endif

    chacha20_poly1305_set_length_clean_aad_last(ctx->aad_bytes, payload_bytes);
    chacha20_poly1305_set_tag_in(ctx->cur_tag);
    chacha20_poly1305_set_cnt(ctx->cur_cnt);
    
    ret = chacha20_poly1305_dma_operate(payload_in, payload_out, payload_bytes);
    if(CHACHA20_POLY1305_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ctx->cur_cnt = chacha20_poly1305_get_current_cnt();
    chacha20_poly1305_get_current_tag(ctx->cur_tag);
    
    return CHACHA20_POLY1305_SUCCESS;
}

/**
 * @brief       chacha20_poly1305 update message including the last data (DMA mode), and get the tag.
 * @param[in]   ctx                - CHACHA20_POLY1305_DMA_CTX context pointer.
 * @param[in]   payload_in         - payload, including last block(or message tail).
 * @param[out]  payload_out        - ciphertext or plaintext.
 * @param[in]   payload_bytes      - byte length of payload.
 * @param[out]  tag                - chacha20_poly1305 tag, occupies 16 bytes.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1. if the all payload length is 0, this case is supported. in this case, payload_in and
        payload_out could set NULL and payload_bytes should be 0.
  @endverbatim
 */
unsigned int chacha20_poly1305_dma_update_including_last_data(CHACHA20_POLY1305_DMA_CTX *ctx, unsigned int *payload_in,
    unsigned int *payload_out, unsigned int payload_bytes, unsigned char tag[16])
{
    unsigned int ret;

    if(NULL == ctx || ((NULL == payload_out) && (0 != payload_bytes)) || ((NULL == payload_out) && (0 != payload_bytes)))
    {
        return CHACHA20_POLY1305_BUFFER_NULL;
    }
    else
    {;}

    ctx->payload_bytes += payload_bytes;

#ifdef CONFIG_CHACHA20_POLY1305_SUPPORT_MUL_THREAD
    ret = chacha20_poly1305_init_internal(ctx->crypto, ctx->key, ctx->constant, ctx->iv, CHACHA20_POLY1305_STAGE_LAST, CHACHA20_POLY1305_DMA_ENABLE);
    if(CHACHA20_POLY1305_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}
#else
    chacha20_poly1305_set_sec_stage(CHACHA20_POLY1305_STAGE_LAST);
#endif

    chacha20_poly1305_set_length_clean_aad_last(ctx->aad_bytes, ctx->payload_bytes);
    chacha20_poly1305_set_tag_in(ctx->cur_tag);
    chacha20_poly1305_set_cnt(ctx->cur_cnt);

    ret = chacha20_poly1305_dma_operate(payload_in, payload_out, payload_bytes);
    if(CHACHA20_POLY1305_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ctx->cur_cnt = chacha20_poly1305_get_current_cnt();
    chacha20_poly1305_get_current_tag(ctx->cur_tag);
    if(CHACHA20_POLY1305_CRYPTO_ENCRYPT == ctx->crypto)
    {
    memcpy_(tag, ctx->cur_tag, 16);
    }
    else
    {
        ret = memcmp_(tag, ctx->cur_tag, 16);
        if(0 != ret)
        {
            return CHACHA20_POLY1305_VERIFY_FAIL;
        }
        else
        {;}
    }
    return CHACHA20_POLY1305_SUCCESS;
}

/**
 * @brief       chacha20_poly1305 (DMA style, one-off style).
 * @param[in]   crypto                - encrypting or decrypting.
 * @param[in]   key                   - key in bytes.
 * @param[in]   constant              - iv of nonce.
 * @param[in]   iv                    - byte length of payload.
 * @param[in]   aad                   - key in bytes.
 * @param[in]   aad_bytes             - byte length of aad.
 * @param[in]   payload_in            - payload of some blocks, excluding last block(or message tail).
 * @param[out]  payload_out           - ciphertext or plaintext.
 * @param[in]   payload_bytes         - byte length of payload, could be 0.
 * @param[out]  tag                   - chacha20_poly1305 tag, occupies 4 words.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.aad_bytes and payload_bytes could be any value. if length bytes is 0, could set pointer is NULL.
      -# 2.in dma mode, the byte length of aad or payload occupy 32 bit.
  @endverbatim
 */
unsigned int chacha20_poly1305_dma(CHACHA20_POLY1305_CRYPTO crypto, unsigned char key[32], unsigned int constant, 
    unsigned char iv[8], unsigned int *aad , unsigned int aad_bytes, unsigned int *payload_in, unsigned int *payload_out, unsigned int payload_bytes, unsigned char tag[16])
{
    unsigned int ret;
    CHACHA20_POLY1305_DMA_CTX ctx[1];

    ret = chacha20_poly1305_dma_init(ctx, crypto, key, constant, iv, aad, aad_bytes);
    if(CHACHA20_POLY1305_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = chacha20_poly1305_dma_update_including_last_data(ctx, payload_in, payload_out, payload_bytes, tag);

    return ret;
}
#endif
