/********************************************************************************************************
 * @file    ske_ccm.c
 *
 * @brief   This is the source file for TL751X
 *
 * @author  Driver Group
 * @date    2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "lib/include/ske/ske_ccm.h"




#ifdef SUPPORT_SKE_MODE_CCM



//this function is common part for ske_lp_ccm_init() and ske_lp_dma_ccm_init()
unsigned int ske_lp_ccm_pre_init(SKE_CCM_CTX *ctx, SKE_CRYPTO crypto, unsigned char *nonce, unsigned char M, unsigned char L,
        unsigned int aad_bytes, unsigned int c_bytes)
{
    unsigned int tmp, len;

    if(NULL == ctx || NULL == nonce)
    {
        return SKE_BUFFER_NULL;
    }
    else if((0 == aad_bytes) && (0 == c_bytes))
    {
        return SKE_INPUT_INVALID;
    }
    else
    {;}

    //check M(the valid candidates are 4,6,8,10,12,14,16)
    if(M & 1)
    {
        return SKE_INPUT_INVALID;
    }
    else if((M < 4) || (M > 16))
    {
        return SKE_INPUT_INVALID;
    }
    else
    {;}

    //check L(the valid candidates are 2,3,4,5,6,7,8)
    if((L < 2) || (L > 8))
    {
        return SKE_INPUT_INVALID;
    }
    else
    {;}

    //check c_bytes
    tmp = c_bytes;
    len = 0;
    while(tmp)
    {
        len++;
        tmp >>= 8;
    }

    if(len > L)
    {
        return SKE_INPUT_INVALID;
    }
    else
    {;}

    /***** init the ctx fields *****/
    ctx->M = M;
    ctx->L = L;

    //A0
    ctx->buf[0] = (ctx->L)-1;
    memcpy_(ctx->buf + 1, nonce, 15-(ctx->L));
    memset_(ctx->buf + 16 - ctx->L, 0, ctx->L);

    ctx->aad_bytes = aad_bytes;
    //ske_lp_set_gcm_aad_len_uint32(aad_bytes);   //for CPU and DMA mode, this action is different.

    ctx->c_bytes = c_bytes;
    ske_lp_set_c_len_uint32(c_bytes);

    ctx->current_bytes = 0;
    ctx->crypto = crypto;

    return SKE_SUCCESS;
}

/**
 * @brief       to get B0 block, please make sure all parameters are valid.
 * @param[in]   nonce           - pointer to the nonce value.
 * @param[in]   M               - number of message authentication bits (tag length).
 * @param[in]   L               - length of the length field in bytes.
 * @param[in]   aad_bytes       - number of bytes in the Additional Authenticated Data (AAD).
 * @param[in]   c_bytes         - number of bytes in the ciphertext.
 * @param[out]  out             - pointer to the output B0 block
 * @return      0:success     other:error
 */
void ske_lp_ccm_get_B0(unsigned char *nonce, unsigned char M, unsigned char L, unsigned int aad_bytes, unsigned int c_bytes, unsigned char out[16])
{
    unsigned char tmp[4];

    //B0 flag
    out[0] = 0;
    out[0] |= (M-2)/2;
    out[0] <<= 3;
    out[0] |= L-1;

    if(aad_bytes)
    {
        out[0] |= 0x40;    //with aad flag
    }
    else
    {;}

    //B0 nonce
    if(nonce != out+1)    //namely, if out is not ctx->buf
    {
        memcpy_(out+1, nonce, 15-L);
        memset_(out+1+15-L, 0, L);
    }
    else
    {;}

    //B0 message byte length
#ifdef SKE_LP_CPU_BIG_ENDIAN
    memcpy_(tmp, &c_bytes, 4);
#else
    reverse_byte_array((unsigned char *)(&c_bytes), tmp, 4);
#endif

    if(L <= 4)
    {
        memcpy_(out+16-L, tmp+4-L, L);
    }
    else
    {
        memcpy_(out+16-4, tmp, 4);
    }
}


/**
 * @brief       to get B1 block(if aad exists), please make sure all parameters are valid.
 * @param[in]   aad              -
 * @param[in]   aad_bytes        -
 * @param[in]   aad_offset       -
 * @param[in]   out          -
 * @return      none
 * @note
  @verbatim
      -# 1. *aad_offset will be the necessary byte length of add head, to build B1 block.
      -# 2.this function is common part for ske_lp_ccm_init() and ske_lp_dma_ccm_init().
  @endverbatim
 */
void ske_lp_ccm_get_B1(unsigned char *aad, unsigned int aad_bytes, unsigned int *aad_offset, unsigned char out[16])
{
    unsigned char tmp[4];
    unsigned int current_bytes, left_bytes;

#ifdef SKE_LP_CPU_BIG_ENDIAN
    memcpy_(tmp, &aad_bytes, 4);
#else
    reverse_byte_array((unsigned char *)(&aad_bytes), tmp, 4);
#endif

    if(aad_bytes < ((1<<16)-(1<<8)))
    {
        memcpy_(out, tmp + 2, 2);
        current_bytes = 2;
        left_bytes = 16-2;
    }
    else
    {
        out[0] = 0xFF;
        out[1] = 0xFE;
        memcpy_(out + 2, tmp, 4);
        current_bytes = 6;
        left_bytes = 16-6;
    }

    if(aad_bytes <= left_bytes)
    {
        memcpy_(out + current_bytes, aad, aad_bytes);
        memset_(out + current_bytes + aad_bytes, 0, left_bytes - aad_bytes);
        *aad_offset = aad_bytes;
    }
    else
    {
        memcpy_(out + current_bytes, aad, left_bytes);
        *aad_offset = left_bytes;
    }
}


#ifdef SKE_LP_CCM_CPU_UPDATE_AAD_BY_STEP
//to prepare B1 block without input aad(if aad exists), please make sure all parameters are valid
void ske_lp_ccm_pre_B1(SKE_CCM_CTX *ctx)
{
    unsigned char tmp[4];
    unsigned int left_bytes;

#ifdef SKE_LP_CPU_BIG_ENDIAN
    memcpy_(tmp, &ctx->aad_bytes, 4);
#else
    reverse_byte_array((unsigned char *)(&ctx->aad_bytes), tmp, 4);
#endif

    if(ctx->aad_bytes < ((1<<16)-(1<<8)))
    {
        memcpy_(ctx->buf, tmp + 2, 2);
        ctx->current_bytes = 2;
        left_bytes = 16-2;
    }
    else
    {
        ctx->buf[0] = 0xFF;
        ctx->buf[1] = 0xFE;
        memcpy_(ctx->buf + 2, tmp, 4);
        ctx->current_bytes = 6;
        left_bytes = 16-6;
    }

    ctx->b1_aad_start_offset = ctx->current_bytes;
    if(ctx->aad_bytes <= left_bytes)
    {
        ctx->b1_aad_end_offset = ctx->b1_aad_start_offset + ctx->aad_bytes;
        memset_(ctx->buf + ctx->b1_aad_end_offset, 0, 16 - ctx->b1_aad_end_offset);
    }
    else
    {
        ctx->b1_aad_end_offset = 16;
    }
}
#endif


/**
 * @brief       ske_lp ccm mode init config.
 * @param[in]   ctx              - SKE_CCM_CTX context pointer.
 * @param[in]   alg              - ske_lp algorithm.
 * @param[in]   crypto           - encrypting or decrypting.
 * @param[in]   key              - TDES key, 24 bytes.
 * @param[in]   sp_key_idx       - key in bytes, key of AES(128/192/256) or SM4.
 * @param[in]   nonce            - index of secure port key, (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX].
 * @param[in]   M                - nonce in bytes, its byte length is 15-L.
 * @param[in]   L                - bytes of length field(message byte length is less than 256^L).
 * @param[in]   aad_bytes        - byte length of aad, it could be any value, including 0.
 * @param[in]   c_bytes          - byte length of plaintext/ciphertext, it could be any value, including 0.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.this function is for CPU style.
      -# 2.only AES(128/192/256) and SM4 are supported for CCM mode.
      -# 3.if key is from user input, please make sure key is not NULL(now sp_key_idx is useless),
       otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX].
      -# 4.only AES(128/192/256) and SM4 are supported for CCM mode.
      -# 5.aad_bytes and c_bytes could not be zero at the same time due to hardware implementation.
  @endverbatim
 */
unsigned int ske_lp_ccm_init(SKE_CCM_CTX *ctx, SKE_ALG alg, SKE_CRYPTO crypto, unsigned char *key, unsigned short sp_key_idx,
        unsigned char *nonce, unsigned char M, unsigned char L, unsigned int aad_bytes, unsigned int c_bytes)
{
    unsigned int ret;

    ret = ske_lp_ccm_pre_init(ctx, crypto, nonce, M, L, aad_bytes, c_bytes);
    if(SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ske_lp_set_aad_len_uint32(ctx->aad_bytes);

    ske_lp_set_cpu_mode();

    //caution: iv here is A0
    ret = ske_lp_init_internal(ctx->ske_ccm_ctx, alg, SKE_MODE_CCM, crypto, key, sp_key_idx, ctx->buf, SKE_LP_DMA_DISABLE);
    if(SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //get and inplut B0
    ske_lp_ccm_get_B0(nonce, M, L, aad_bytes, c_bytes, ctx->buf);

    if(0 == ctx->aad_bytes)
    {
        ske_lp_set_last_block(1);    //last block;
    }
    else
    {;}

    ret = ske_lp_update_blocks_no_output(ctx->ske_ccm_ctx, ctx->buf, ctx->ske_ccm_ctx->block_bytes);
    if(SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    if(0 == ctx->aad_bytes)
    {
        ske_lp_set_last_block(0);    //not last block
    }
    else
    {;}

#ifdef SKE_LP_CCM_CPU_UPDATE_AAD_BY_STEP
    //prepare B1
    if(0 != aad_bytes)
    {
        ske_lp_ccm_pre_B1(ctx);
    }
    else
    {;}
#endif

    return SKE_SUCCESS;
}


#ifdef SKE_LP_CCM_CPU_UPDATE_AAD_BY_STEP
/**
 * @brief       ske_lp ccm mode input aad(multiple step style).
 * @param[in]   ctx              - SKE_GCM_CTX context pointer.
 * @param[in]   aad              - aad.
 * @param[in]   bytes            - byte length of aad.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.this function must be called after calling ske_lp_ccm_init().
      -# 2.if there is no aad, this function could be omitted.
      -# 3.if the whole aad is too long, you could divide it into some sections by byte, then call
        this function to input the sections respectively. for example, if the whole aad byte
        length is 65, it could be divided into 3 sections with byte length 10,47,8 respectively.
  @endverbatim
 */
unsigned int ske_lp_ccm_update_aad(SKE_CCM_CTX *ctx, unsigned char *aad, unsigned int bytes)
{
    unsigned int blocks_bytes, remainder_bytes;
    unsigned int total_bytes, idx, capacity_bytes;
    unsigned int ret;

    if(NULL == ctx || (NULL == aad && ctx->aad_bytes != 0))
    {
        return SKE_BUFFER_NULL;
    }
    else if((0 == ctx->aad_bytes) || (NULL == aad) || (0 == bytes))
    {
        return SKE_SUCCESS;
    }
    else
    {;}

    //now bytes is not 0

    if(ctx->current_bytes < ctx->b1_aad_end_offset)
    {
        remainder_bytes = ctx->b1_aad_end_offset - ctx->current_bytes;
        if(bytes >= remainder_bytes)
        {
            memcpy_(ctx->buf + ctx->current_bytes, aad, remainder_bytes);
            aad += remainder_bytes;
            bytes -= remainder_bytes;
            ctx->current_bytes += remainder_bytes;

            if(ctx->current_bytes == ctx->aad_bytes + ctx->b1_aad_start_offset)
            {
                ske_lp_set_last_block(1);    //last block;
            }

            ret = ske_lp_update_blocks_no_output(ctx->ske_ccm_ctx, ctx->buf, ctx->ske_ccm_ctx->block_bytes);
            if(SKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {;}

            if(ctx->current_bytes == ctx->aad_bytes + ctx->b1_aad_start_offset)
            {
                ske_lp_set_last_block(0);    //not last block;
                ctx->current_bytes = 0;
                return SKE_SUCCESS;          //aad input finished
            }
            else
            {;}
        }
        else
        {
            memcpy_(ctx->buf + ctx->current_bytes, aad, bytes);
            ctx->current_bytes += bytes;
            return SKE_SUCCESS;
        }
    }
    else
    {;}

    //now bytes is not 0
    if(0 == bytes)
    {
        return SKE_SUCCESS;
    }
    else
    {;}

    /******** input B2,B3... ********/
    idx = ctx->current_bytes & 0x0F;
    capacity_bytes = 16 - idx;

    total_bytes = ctx->current_bytes + bytes;
    if(total_bytes < bytes || total_bytes > (ctx->b1_aad_start_offset + ctx->aad_bytes))
    {
        return SKE_INPUT_INVALID;
    }
    else if(total_bytes == (ctx->b1_aad_start_offset + ctx->aad_bytes))
    {
        if(idx)
        {
            if(bytes > capacity_bytes)
            {
                memcpy_(ctx->buf + idx, aad, capacity_bytes);
                ret = ske_lp_update_blocks_no_output(ctx->ske_ccm_ctx, ctx->buf, 16);
                if(SKE_SUCCESS != ret)
                {
                    return ret;
                }
                else
                {;}

                aad += capacity_bytes;
                bytes -= capacity_bytes;
            }
            else
            {
                //the last block
                memcpy_(ctx->buf + idx, aad, bytes);
                memset_(ctx->buf + idx + bytes, 0, sizeof(ctx->buf) - (idx + bytes));
                goto LAST_BLOCK;
            }
        }

        blocks_bytes = (bytes)&(~0x0F);  //assume that ctx->ske_ccm_ctx->block_bytes is 16
        remainder_bytes = (bytes)&0x0F;
        if(0 == remainder_bytes)
        {
            blocks_bytes -= 16;
            remainder_bytes = 16;
        }
        else
        {;}

        ret = ske_lp_update_blocks_no_output(ctx->ske_ccm_ctx, aad, blocks_bytes);
        if(SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        memcpy_(ctx->buf, aad+blocks_bytes, remainder_bytes);
        memset_(ctx->buf + remainder_bytes, 0, ctx->ske_ccm_ctx->block_bytes - remainder_bytes);

LAST_BLOCK:

        ske_lp_set_last_block(1);    //last block
        ret = ske_lp_update_blocks_no_output(ctx->ske_ccm_ctx, ctx->buf, ctx->ske_ccm_ctx->block_bytes);
        ske_lp_set_last_block(0);    //not last block
        if(SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        ctx->current_bytes = 0;
    }
    else
    {
        ctx->current_bytes = total_bytes;

        if(idx)
        {
            if(bytes >= capacity_bytes)
            {
                memcpy_(ctx->buf + idx, aad, capacity_bytes);
                ret = ske_lp_update_blocks_no_output(ctx->ske_ccm_ctx, ctx->buf, 16);
                if(SKE_SUCCESS != ret)
                {
                    return ret;
                }
                else
                {;}

                aad += capacity_bytes;
                bytes -= capacity_bytes;
            }
            else
            {
                memcpy_(ctx->buf + idx, aad, bytes);
                ret = SKE_SUCCESS;
                goto END;
            }
        }

        blocks_bytes = (bytes)&(~0x0F);
        remainder_bytes = (bytes)&0x0F;

        ret = ske_lp_update_blocks_no_output(ctx->ske_ccm_ctx, aad, blocks_bytes);
        if(SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        if(remainder_bytes)
        {
            memcpy_(ctx->buf, aad+blocks_bytes, remainder_bytes);
        }
        else
        {;}
    }

END:

    return SKE_SUCCESS;
}
#endif


/**
 * @brief       ske_lp ccm mode input aad(one-off style).
 * @param[in]   ctx              - SKE_GCM_CTX context pointer.
 * @param[in]   aad              - aad, its length is ctx->aad_bytes, please make sure
                                   aad here is integral.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.this function must be called after calling ske_lp_ccm_init().
      -# 2.if there is no aad, this function could be omitted.
  @endverbatim
 */
unsigned int ske_lp_ccm_aad(SKE_CCM_CTX *ctx, unsigned char *aad)
{
#ifndef SKE_LP_CCM_CPU_UPDATE_AAD_BY_STEP
    unsigned int blocks_bytes, remainder_bytes;
    unsigned int aad_bytes, aad_offset;
    unsigned int ret;

    if(NULL == ctx || (NULL == aad && ctx->aad_bytes != 0))
    {
        return SKE_BUFFER_NULL;
    }
    else if((NULL == aad) || (0 == ctx->aad_bytes))
    {
        return SKE_SUCCESS;
    }
    else
    {;}

    //now aad is not NULL, and ctx->aad_bytes is not 0

    //input B1,B2...
    aad_bytes = ctx->aad_bytes;

    /******** get and input B1 ********/
    ske_lp_ccm_get_B1(aad, aad_bytes, &aad_offset, ctx->buf);

    aad_bytes -= aad_offset;
    aad += aad_offset;
    if(0 == aad_bytes)
    {
        ske_lp_set_last_block(1);    //last block;
    }
    else
    {;}

    ret = ske_lp_update_blocks_no_output(ctx->ske_ccm_ctx, ctx->buf, ctx->ske_ccm_ctx->block_bytes);
    if(SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    if(0 == aad_bytes)
    {
        ske_lp_set_last_block(0);    //not last block
        ctx->current_bytes = 0;
        return SKE_SUCCESS;
    }
    else
    {;}

    /******** input B2,B3... ********/
    blocks_bytes = (aad_bytes)&(~0x0F);  //assume that ctx->ske_ccm_ctx->block_bytes is 16
    remainder_bytes = (aad_bytes)&0x0F;
    if(0 == remainder_bytes)
    {
        blocks_bytes -= 16;
        remainder_bytes = 16;
    }
    else
    {;}

    ret = ske_lp_update_blocks_no_output(ctx->ske_ccm_ctx, aad, blocks_bytes);
    if(SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    memcpy_(ctx->buf, aad+blocks_bytes, remainder_bytes);
    memset_(ctx->buf + remainder_bytes, 0, ctx->ske_ccm_ctx->block_bytes - remainder_bytes);
    ske_lp_set_last_block(1);    //last block
    ret = ske_lp_update_blocks_no_output(ctx->ske_ccm_ctx, ctx->buf, ctx->ske_ccm_ctx->block_bytes);
    ske_lp_set_last_block(0);    //not last block
    if(SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ctx->current_bytes = 0;

    return SKE_SUCCESS;
#else
    if(NULL == ctx)
    {
        return SKE_BUFFER_NULL;
    }
    else
    {
        return ske_lp_ccm_update_aad(ctx, aad, ctx->aad_bytes);
    }
#endif
}


/**
 * @brief       ske_lp ccm mode input plaintext/ciphertext.
 * @param[in]   ctx              - SKE_CCM_CTX context pointer.
 * @param[in]   in               - plaintext or ciphertext.
 * @param[out]  out              - ciphertext or plaintext.
 * @param[in]   bytes            - byte length of input or output.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.this function must be called after inputting aad.
      -# 2.if there is no plaintext/ciphertext, this function could be omitted.
      -# 3.to save memory, in and out could be the same buffer, in this case, the output will
       cover the input.
      -# 4.if the whole plaintext/ciphertext is too long, you could divide it by block(16 bytes),
          and if the whole plaintext/ciphertext byte length is not a multiple of 16, please make
          sure the last section contains the tail, then call this function to input the sections
          respectively. for example, if the whole plaintext/ciphertext byte length is 65, it
          could be divided into 3 sections with byte length 32,16,17 respectively..
  @endverbatim
 */
unsigned int ske_lp_ccm_update_blocks(SKE_CCM_CTX *ctx, unsigned char *in, unsigned char *out, unsigned int bytes)
{
    unsigned int blocks_bytes, remainder_bytes;
    unsigned int total_bytes;
    unsigned int ret;

    if(NULL == ctx|| NULL == in || NULL == out)
    {
        return SKE_BUFFER_NULL;
    }
    else if(0 == bytes)
    {
        return SKE_SUCCESS;
    }
    else
    {;}

    //now bytes is not 0

    total_bytes = ctx->current_bytes + bytes;
    if(total_bytes < bytes || total_bytes > ctx->c_bytes)  // overflow
    {
        return SKE_INPUT_INVALID;
    }
    else if(total_bytes == ctx->c_bytes)
    {
        blocks_bytes = bytes & (~0x0F);
        remainder_bytes = bytes & 0x0F;
        if(0 == remainder_bytes)
        {
            blocks_bytes -= 16;
            remainder_bytes = 16;
        }
        else
        {;}

        ret = ske_lp_update_blocks_internal(ctx->ske_ccm_ctx, in, out, blocks_bytes);
        if(SKE_SUCCESS != ret)
        {
            goto END;
        }
        else
        {;}

        //the last block
        memcpy_(ctx->buf, in + blocks_bytes, remainder_bytes);
        memset_(ctx->buf + remainder_bytes, 0, sizeof(ctx->buf) - remainder_bytes);

        ske_lp_set_last_block(1);
        ret = ske_lp_update_blocks_internal(ctx->ske_ccm_ctx, ctx->buf, ctx->buf, 16);
        ske_lp_set_last_block(0);
        if(SKE_SUCCESS != ret)
        {
            goto END;
        }
        else
        {;}

        memcpy_(out+blocks_bytes, ctx->buf, remainder_bytes);
    }
    else
    {
        if(bytes & (16-1))
        {
            ret = SKE_INPUT_INVALID;
            goto END;
        }
        else
        {
            ret = ske_lp_update_blocks_internal(ctx->ske_ccm_ctx, in, out, bytes);
            if(SKE_SUCCESS != ret)
            {
                goto END;
            }
            else
            {;}
        }
    }

    ret = SKE_SUCCESS;
    ctx->current_bytes = total_bytes;

END:

    return ret;
}


/**
 * @brief       ske_lp ccm mode input plaintext/ciphertext.
 * @param[in]   ctx              - SKE_CCM_CTX context pointer.
 * @param[in]   mac              - (for decryption), output(for encryption).
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.this function must be called after calling ske_lp_ccm_update_blocks().
      -# 2.byte length of mac is ctx->M.
      -# 3.for encryption, mac is output; and for decryption, mac is input, if returns SKE_SUCCESS
 *        that means certification passed, otherwise not.
      -# 4.if the whole plaintext/ciphertext is too long, you could divide it by block(16 bytes),
          and if the whole plaintext/ciphertext byte length is not a multiple of 16, please make
          sure the last section contains the tail, then call this function to input the sections
          respectively. for example, if the whole plaintext/ciphertext byte length is 65, it
          could be divided into 3 sections with byte length 32,16,17 respectively.
  @endverbatim
 */
unsigned int ske_lp_ccm_final(SKE_CCM_CTX *ctx, unsigned char *mac)
{
    unsigned int ret;

    if(NULL == ctx || NULL == mac)
    {
        return SKE_BUFFER_NULL;
    }
    else
    {;}

    ske_lp_start();

    //get mac
    ret = ske_lp_wait_till_done();
    if(SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ske_lp_simple_get_output_block((unsigned int *)ctx->buf, ctx->ske_ccm_ctx->block_words);

    if(SKE_CRYPTO_ENCRYPT == ctx->crypto)
    {
        memcpy_(mac, ctx->buf, ctx->M);
        ret = SKE_SUCCESS;
    }
    else
    {
        ret = memcmp_(mac, ctx->buf, ctx->M);
    }

    //memset_(ctx, 0, sizeof(SKE_CCM_CTX));

    return ret;
}


/**
 * @brief       ske_lp ccm mode encrypt/decrypt(one-off style).
 * @param[in]   alg              - ske_lp algorithm.
 * @param[in]   crypto           - encrypting or decrypting.
 * @param[in]   key              - key in bytes, key of AES(128/192/256) or SM4.
 * @param[in]   sp_key_idx       - index of secure port key, (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX].
 * @param[in]   nonce            - nonce in bytes, its byte length is 15-L.
 * @param[in]   M                - bytes of authentication field(bytes of mac).
 * @param[in]   L                -  bytes of length field(message byte length is less than 256^L).
 * @param[in]   aad              - aad, please make sure aad here is integral.
 * @param[in]   aad_bytes        - byte length of aad, it could be any value, including 0.
 * @param[in]   in               - plaintext or ciphertext.
 * @param[out]  out              - ciphertext or plaintext.
 * @param[in]   c_bytes          - byte length of plaintext/ciphertext, it could be any value, including 0.
 * @param[in]   mac              - (for decryption), output(for encryption).
 *
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.this function is for CPU style.
      -# 2.only AES(128/192/256) and SM4 are supported for GCM mode.
      -# 3.if key is from user input, please make sure key is not NULL(now sp_key_idx is useless),
         otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX].
      -# 4.valid M is {4,6,8,10,12,14,16}, and valid L is {2,3,4,5,6,7,8}.
      -# 5.aad_bytes and c_bytes could not be zero at the same time due to hardware implementation.
      -# 6.to save memory, in and out could be the same buffer, in this case, the output will
        cover the input.
      -# 7.byte length of mac is M.
      -# 8.for encryption, mac is output; and for decryption, mac is input, if returns SKE_SUCCESS
 *        that means certification passed, otherwise not.
  @endverbatim
 */
unsigned int ske_lp_ccm_crypto(SKE_ALG alg, SKE_CRYPTO crypto, unsigned char *key, unsigned short sp_key_idx, unsigned char *nonce,
        unsigned char M, unsigned char L, unsigned char *aad, unsigned int aad_bytes, unsigned char *in, unsigned char *out, unsigned int c_bytes,
        unsigned char *mac)
{
    SKE_CCM_CTX ctx[1];
    unsigned int ret;

    ret = ske_lp_ccm_init(ctx, alg, crypto, key, sp_key_idx, nonce, M, L, aad_bytes, c_bytes );
    if(SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = ske_lp_ccm_aad(ctx, aad);
    if(SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = ske_lp_ccm_update_blocks(ctx, in, out, c_bytes);
    if(SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    return ske_lp_ccm_final(ctx, mac);
}




#ifdef SKE_LP_DMA_FUNCTION
/**
 * @brief       ske_lp dma ccm mode init config.
 * @param[in]   ctx              - SKE_CCM_CTX context pointer.
 * @param[in]   alg              - ske_lp algorithm.
 * @param[in]   crypto           - encrypting or decrypting.
 * @param[in]   key              - key in bytes, key of AES(128/192/256) or SM4.
 * @param[in]   sp_key_idx       - index of secure port key, (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX].
 * @param[in]   nonce            - nonce in bytes, its byte length is 15-L.
 * @param[in]   M                - bytes of authentication field(bytes of mac).
 * @param[in]   L                -  bytes of length field(message byte length is less than 256^L).
 * @param[in]   aad_bytes        - byte length of aad, it could be any value, including 0.
 * @param[in]   c_bytes          - byte length of plaintext/ciphertext, it could be any value, including 0.
 *
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.this function is for DMA style.
      -# 2.only AES(128/192/256) and SM4 are supported for CCM mode.
      -# 3.if key is from user input, please make sure key is not NULL(now sp_key_idx is useless),
         otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX].
      -# valid M is {4,6,8,10,12,14,16}, and valid L is {2,3,4,5,6,7,8}.
      -# 5.aad_bytes and c_bytes could not be zero at the same time due to hardware implementation.
  @endverbatim
 */
unsigned int ske_lp_dma_ccm_init(SKE_CCM_CTX *ctx, SKE_ALG alg, SKE_CRYPTO crypto, unsigned char *key, unsigned short sp_key_idx,
        unsigned char *nonce, unsigned char M, unsigned char L, unsigned int aad_bytes, unsigned int c_bytes)
{
    unsigned int ret;

    ret = ske_lp_ccm_pre_init(ctx, crypto, nonce, M, L, aad_bytes, c_bytes);
    if(SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //get aad start offset in B1
    if(0 == aad_bytes)
    {
        ctx->b1_aad_start_offset = 0;  //only B0
    }
    else if(aad_bytes < ((1<<16)-(1<<8)))
    {
        ctx->b1_aad_start_offset = 2;
    }
    else
    {
        ctx->b1_aad_start_offset = 6;
    }

    //set total block length except plaintext/ciphertext, this is due to the hardware requires
    aad_bytes += (16 + ctx->b1_aad_start_offset);
    aad_bytes = (aad_bytes+15)&(~0x0F);
    ske_lp_set_aad_len_uint32(aad_bytes);

    ske_lp_set_cpu_mode();

    //caution: iv here is A0
    return ske_lp_init_internal(ctx->ske_ccm_ctx, alg, SKE_MODE_CCM, crypto, key, sp_key_idx, ctx->buf, SKE_LP_DMA_ENABLE);
}


/**
 * @brief       ske_lp dma ccm mode update B0(multiple steps style).
 * @param[in]   ctx              - SKE_CCM_CTX context pointer.
 * @param[in]   B0               - ske_lp algorithm.
 * @param[in]   callback         - callback function pointer, this could be NULL, means do nothing.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.this is for multiple steps style.
      -# 2.this function must be called after calling ske_lp_dma_ccm_init().
  @endverbatim
 */
unsigned int ske_lp_dma_ccm_update_B0_block(SKE_CCM_CTX *ctx, unsigned int B0[4], SKE_CALLBACK callback)
{
    if((NULL == ctx) || (NULL == B0))
    {
        return SKE_BUFFER_NULL;
    }
    else
    {;}

    return ske_lp_dma_operate(ctx->ske_ccm_ctx, B0, NULL, 4, 0, callback);
}


/**
 * @brief       ske_lp dma ccm mode update aad(multiple steps style).
 * @param[in]   ctx              - SKE_CCM_CTX context pointer.
 * @param[in]   aad              - aad.
 * @param[in]   bytes            - byte length of aad.
 * @param[in]   callback         - callback function pointer, this could be NULL, means do nothing.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.this is for multiple steps style.
      -# 2.if there is no aad, this function could be omitted.
      -# 3.this function must be called after calling ske_lp_dma_ccm_update_B0_block().
      -# 4.the whole aad here must be with prefix(in B1), and padding the tail with 0 if necessary(to
        make it occupies a multiple of 16 bytes). and now whole aad byte length is prefix byte length
        (ctx->b1_aad_start_offset) + original aad byte length.
      -# 5.if the whole aad with prefix is too long, you could divide it by block(16 bytes), and if the
         whole aad byte length is not a multiple of 16, please make sure the last section contains the
         tail, then call this function to input the sections respectively. for example, if the whole
         aad byte length is 65, it could be divided into 3 sections with byte length 48,16,1 respectively.
  @endverbatim
 */
unsigned int ske_lp_dma_ccm_update_aad_blocks(SKE_CCM_CTX *ctx, unsigned int *aad, unsigned int bytes, SKE_CALLBACK callback)
{
    unsigned int aad_blocks_words = ((bytes+15)&(~0x0F))/4;
    unsigned int total_bytes;
    unsigned int ret;

    if(NULL == ctx)
    {
        return SKE_BUFFER_NULL;
    }
    else if((NULL == aad) || (0 == bytes))
    {
        return SKE_SUCCESS;
    }
    else
    {;}

    total_bytes = ctx->current_bytes + bytes;

    if (total_bytes < bytes || total_bytes > ctx->aad_bytes + ctx->b1_aad_start_offset)  // overflow
    {
        return SKE_INPUT_INVALID;
    }
    else if(total_bytes == ctx->aad_bytes + ctx->b1_aad_start_offset)
    {
        ret = ske_lp_dma_operate(ctx->ske_ccm_ctx, aad, NULL, aad_blocks_words, 0, callback);
        if(SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        ctx->current_bytes = 0;
    }
    else
    {
        if(bytes & (16-1))
        {
            return SKE_INPUT_INVALID;
        }
        else
        {;}

        ret = ske_lp_dma_operate(ctx->ske_ccm_ctx, aad, NULL, aad_blocks_words, 0, callback);
        if(SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        ctx->current_bytes = total_bytes;
    }

    return SKE_SUCCESS;
}


/**
 * @brief       ske_lp dma ccm mode input plaintext/ciphertext, get ciphertext/plaintext or
 *              ciphertext/plaintext+mac(multiple steps style).
 * @param[in]   ctx              - SKE_CCM_CTX context pointer.
 * @param[in]   in               - plaintext/ciphertext.
 * @param[in]   out              - ciphertext/plaintext or ciphertext/plaintext+macs.
 * @param[in]   in_bytes         - byte length of in.
 * @param[in]   callback         - callback function pointer, this could be NULL, means do nothing.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.this function must be called after calling ske_lp_dma_ccm_update_aad_blocks().
      -# 2.the whole plaintext/ciphertext must be some blocks, if not, please pad it with 0.
      -# 3.f the whole plaintext/ciphertext is too long, you could divide it by block(16 bytes), and if
        the whole plaintext/ciphertext byte length is not a multiple of 16, please make sure the last
        section contains the tail, then call this function to input the sections respectively. for
        example, if the whole plaintext/ciphertext byte length is 65, it could be divided into 3 sections
        with byte length 48,16,1 respectively.
      -# 4.the output will be some blocks too, it has the same length as the input, and with padding 0 if
         necessary.
      -# 5.if input contains the tail, then output will be ciphertext/plaintext+mac.
      -# 6.to save memory, in and out could be the same buffer, in this case, the output will
       cover the input.
  @endverbatim
 */
unsigned int ske_lp_dma_ccm_update_blocks(SKE_CCM_CTX *ctx, unsigned int *in, unsigned int *out, unsigned int in_bytes,
        SKE_CALLBACK callback)
{
    unsigned int in_blocks_words = ((in_bytes+15)&(~0x0F))/4;
    unsigned int total_bytes;
    unsigned int ret;

    if((NULL == ctx) || (NULL == in) || (NULL == out))
    {
        return SKE_BUFFER_NULL;
    }
    else if(0 == in_bytes)
    {
        return SKE_SUCCESS;
    }
    else
    {;}

    if(0 == ctx->c_bytes)
    {
        if (0 == ctx->aad_bytes)
        {
            //hardware does not support
            return SKE_INPUT_INVALID;
        }
        else
        {
            //just for the case that aad is not NULL, and c is NULL, here input aad block including tail.
            ret = ske_lp_dma_operate(ctx->ske_ccm_ctx, in, out, in_blocks_words, 4, callback);
            if(SKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {;}

            clear_block_tail(out, ctx->M);

            return SKE_SUCCESS;
        }
    }
    else
    {;}

    total_bytes = ctx->current_bytes + in_bytes;
    if (total_bytes < in_bytes || total_bytes > ctx->c_bytes)  // overflow
    {
        return SKE_INPUT_INVALID;
    }
    else if(total_bytes == ctx->c_bytes)
    {
        ret = ske_lp_dma_operate(ctx->ske_ccm_ctx, in, out, in_blocks_words, in_blocks_words + 4, callback);
        if(SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        if(ctx->c_bytes & 0x0F)
        {
            clear_block_tail(out + in_blocks_words - 4, (ctx->c_bytes & 0x0F));
        }
        else
        {;}

        clear_block_tail(out + in_blocks_words, ctx->M);
    }
    else
    {
        if(in_bytes & (16-1))
        {
            return SKE_INPUT_INVALID;
        }
        else
        {;}

        ret = ske_lp_dma_operate(ctx->ske_ccm_ctx, in, out, in_blocks_words, in_blocks_words, callback);
        if(SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}
    }

    ctx->current_bytes = total_bytes;

    return SKE_SUCCESS;
}


/**
 * @brief       ske_lp dma ccm mode finish.
 * @param[in]   ctx              - SKE_CCM_CTX context pointer.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.this function is optional.
  @endverbatim
 */
unsigned int ske_lp_dma_ccm_final(SKE_CCM_CTX *ctx)
{
    if(NULL == ctx)
    {
        return SKE_BUFFER_NULL;
    }
    else
    {;}

    memset_(ctx, 0, sizeof(SKE_CCM_CTX));

    return SKE_SUCCESS;
}


/**
 * @brief       ske_lp ccm mode encrypt/decrypt(one-off style).
 * @param[in]   alg              - ske_lp algorithm.
 * @param[in]   crypto           - encrypting or decrypting.
 * @param[in]   key              - key in bytes, key of AES(128/192/256) or SM4.
 * @param[in]   sp_key_idx       - index of secure port key, (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX].
 * @param[in]   nonce            - nonce in bytes, its byte length is 15-L.
 * @param[in]   M                - bytes of authentication field(bytes of mac).
 * @param[in]   L                -  bytes of length field(message byte length is less than 256^L)
 * @param[in]   aad_bytes        - byte length of aad, it could be any value, including 0.
 * @param[in]   in               - plaintext or ciphertext.
 * @param[out]  out              - ciphertext or plaintext.
 * @param[in]   c_bytes          - byte length of plaintext/ciphertext, it could be any value, including 0.
 * @param[in]   callback         - callback function pointer, this could be NULL, means do nothing.
 *
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.this function is for DMA style.
      -# 2.only AES(128/192/256) and SM4 are supported for GCM mode.
      -# 3.if key is from user input, please make sure key is not NULL(now sp_key_idx is useless),
         otherwise, key is from secure port, and (sp_key_idx & 0x7FFF) must be in [1,SKE_MAX_KEY_IDX].
      -# 4.valid M is {4,6,8,10,12,14,16}, and valid L is {2,3,4,5,6,7,8}.
      -# 5.aad_bytes and c_bytes could not be zero at the same time due to hardware implementation.
      -# 6.if aad exists, it must be some blocks, if not, please pad it with 0
        cover the input.
      -# 7.the output ciphertext/plaintext has the same number of blocks as the input plaintext/ciphertext,
         and followed by one block, it is mac with padding 0 if necessary, so is the second last block if
         necessary(ciphertext/plaintext).
      -# 8.for encryption, mac is output; and for decryption, mac is input, if returns SKE_SUCCESS
         that means certification passed, otherwise not.
      -# 9.please make sure B0+aad+plaintext/ciphertext is integral.
      -# 10.to save memory, in and out could be the same buffer, in this case, the output will
        cover the input.
  @endverbatim
 */
unsigned int ske_lp_dma_ccm_crypto(SKE_ALG alg, SKE_CRYPTO crypto, unsigned char *key, unsigned short sp_key_idx, unsigned char *nonce,
        unsigned char M, unsigned char L, unsigned int aad_bytes, unsigned int *in, unsigned int *out, unsigned int c_bytes, SKE_CALLBACK callback)
{
    unsigned int aad_blocks_words, c_blocks_words;
    SKE_CCM_CTX ctx[1];
    unsigned int ret;

    ret = ske_lp_dma_ccm_init(ctx, alg, crypto, key, sp_key_idx, nonce, M, L, aad_bytes, c_bytes);
    if(SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    aad_blocks_words = ((aad_bytes + ctx->b1_aad_start_offset + 15)/16)*4;
    c_blocks_words = ((c_bytes + 15)/16)*4;

#if 1
    if(NULL == in || NULL == out)
    {
        return SKE_BUFFER_NULL;
    }
    else
    {;}

    ret = ske_lp_dma_operate(ctx->ske_ccm_ctx, in, out, 4 + aad_blocks_words + c_blocks_words, c_blocks_words + 4, callback);
    if(SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //clear useless data
    if(c_bytes & 0x0F)
    {
        clear_block_tail(out + c_blocks_words - 4, (c_bytes & 0x0F));
    }
    else
    {;}

    clear_block_tail(out + c_blocks_words, M);
#else
    ret = ske_lp_dma_ccm_update_B0_block(ctx, in, callback);
    if(SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    if(aad_bytes && (0 == c_bytes))
    {
        ret = ske_lp_dma_ccm_update_blocks(ctx, in+4, out, aad_bytes, callback);
        if(SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}
    }
    else
    {
        ret = ske_lp_dma_ccm_update_aad_blocks(ctx, in+4, ctx->b1_aad_start_offset + aad_bytes, callback);
        if(SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        ret = ske_lp_dma_ccm_update_blocks(ctx, in+4+aad_blocks_words, out, c_bytes, callback);
        if(SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}
    }
#endif

    return ske_lp_dma_ccm_final(ctx);
}

#endif


#endif
