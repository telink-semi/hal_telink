/********************************************************************************************************
 * @file    ske_basic.c
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
#include "lib/include/ske/ske_basic.h"
#include "lib/include/ske/ske_portable.h"
#include "lib/include/crypto_common/utility.h"
#include "lib/include/ske/ske_portable.h"
#include "lib/include/trng/trng.h"
#include "driver.h"

/**
 * @brief           ske_lp register pointer
 */
static volatile ske_lp_reg_t *const g_ske_lp_reg = (ske_lp_reg_t *)(SKE_LP_BASE_ADDR);

/**
 * @brief           Get SKE IP version
 * @return          SKE IP version
 */
unsigned int ske_lp_get_version(void)
{
    return g_ske_lp_reg->ske_version;
}

/**
 * @brief           Set SKE to CPU mode
 */
void ske_lp_set_cpu_mode(void)
{
    volatile unsigned int flag = (((unsigned int)1) << SKE_LP_UP_CFG_OFFSET);
    volatile unsigned int mask = ~(((unsigned int)1) << SKE_LP_DMA_OFFSET);

    g_ske_lp_reg->cfg &= mask;
    g_ske_lp_reg->cfg |= flag;
}

/**
 * @brief           Set SKE to DMA mode
 * @return          none
 */
void ske_lp_set_dma_mode(void)
{
    volatile unsigned int flag = (((unsigned int)1) << SKE_LP_DMA_OFFSET);
    //  volatile unsigned int mask = ~(((unsigned int)1)<<SKE_LP_UP_CFG_OFFSET);

    g_ske_lp_reg->cfg |= flag;
    //  g_ske_lp_reg->cfg &= mask;
}

/**
 * @brief           set the ske_lp endian
 * @return          none
 * @note
 *        1. actually, this config works for only CPU mode no
 */
void ske_lp_set_endian_uint32(void)
{
    volatile unsigned int mask = ~(((unsigned int)3) << SKE_LP_REVERSE_BYTE_ORDER_IN_WORD_OFFSET);
#ifdef SKE_LP_REVERSE_BYTE_ORDER_IN_WORD
    volatile unsigned int flag = (((unsigned int)2) << SKE_LP_REVERSE_BYTE_ORDER_IN_WORD_OFFSET);
#endif

    g_ske_lp_reg->cfg &= mask; // clear bit[25:24], and now requires CPU is big-endian
#ifdef SKE_LP_REVERSE_BYTE_ORDER_IN_WORD
    g_ske_lp_reg->cfg |= flag; // requires CPU is little-endian, input and output reversed by hardware----ske IP
#endif
}

/**
 * @brief           set ske_lp encrypting or decrypting
 * @param[in]       crypto               - SKE_CRYPTO_ENCRYPT or SKE_CRYPTO_DECRYPT
 * @return          none
 * @note
 *        1. please make sure crypto is valid
 */
void ske_lp_set_crypto(ske_crypto_e crypto)
{
    volatile unsigned int mask = ~(((unsigned int)1) << SKE_LP_CRYPTO_OFFSET);

    g_ske_lp_reg->cfg &= mask;
    g_ske_lp_reg->cfg |= (((unsigned int)crypto) << SKE_LP_CRYPTO_OFFSET);
}

/**
 * @brief           set ske_lp alg
 * @param[in]       ske_alg              - ske_lp algorithm
 * @return          none
 * @note
 *        1. please make sure ske_alg is valid
 */
void ske_lp_set_alg(ske_alg_e ske_alg)
{
    volatile unsigned int mask = ~(0x0000000F);
    unsigned int cfg;

    switch (ske_alg)
    {
#ifdef SUPPORT_SKE_DES
    case SKE_ALG_DES:
        cfg = 3;
        break;
#endif

#ifdef SUPPORT_SKE_AES_128
    case SKE_ALG_AES_128:
        cfg = 1;
        break;
#endif

#ifdef SUPPORT_SKE_AES_192
    case SKE_ALG_AES_192:
        cfg = 4;
        break;
#endif

#ifdef SUPPORT_SKE_AES_256
    case SKE_ALG_AES_256:
        cfg = 5;
        break;
#endif

#ifdef SUPPORT_SKE_SM4
    case SKE_ALG_SM4:
        cfg = 2;
        break;
#endif

    default:
        cfg = 2; // default alg SM4
    }

    g_ske_lp_reg->cfg &= mask; // clear bit[3:0]
    g_ske_lp_reg->cfg |= cfg;  // set ske_lp alg cfg
}

/**
 * @brief           set ske_lp alg operation mode
 * @param[in]       mode                 - operation mode
 * @return          none
 * @note
 *        1. please make sure mode is valid
 */
void ske_lp_set_mode(ske_mode_e mode)
{
    volatile unsigned int mask = ~(0x0000000FU << SKE_LP_MODE_OFFSET);

    g_ske_lp_reg->cfg &= mask;                                         // clear bit [31:28]
    g_ske_lp_reg->cfg |= (((unsigned int)mode) << SKE_LP_MODE_OFFSET); // set mode
}

/**
 * @brief           set whether ske_lp current input data is the last data or not
 * @param[in]       is_last_block        - 0:no, other:yes
 * @return          none
 * @note
 *        1. just for CMAC/CCM/GCM/XTS mod
 */
void ske_lp_set_last_block(unsigned int is_last_block)
{
    volatile unsigned int flag = (((unsigned int)1) << SKE_LP_LAST_DATA_OFFSET);
    volatile unsigned int mask = ~(((unsigned int)1) << SKE_LP_LAST_DATA_OFFSET);

    if (is_last_block)
    {
        g_ske_lp_reg->m_din_cr |= flag;
    }
    else
    {
        g_ske_lp_reg->m_din_cr &= mask;
    }
}

/**
 * @brief           ske_lp start to expand key or calc
 * @return          none
 */
void ske_lp_start(void)
{
    volatile unsigned int start_flag = 1;

    while (1U == (g_ske_lp_reg->sr1 & start_flag))
        ;

    g_ske_lp_reg->ctrl |= start_flag;
}

/**
 * @brief           wait till ske_lp calculating is done
 * @return          none
 */
unsigned int ske_lp_wait_till_done(void)
{
    volatile unsigned int finish_flag = 1;
    volatile unsigned int clear_flag = 0;

#if 0
    while((g_ske_lp_reg->sr2 & finish_flag) == 0)
#else
    while (((g_ske_lp_reg->sr2 & finish_flag) == 0) || ((g_ske_lp_reg->sr1 & finish_flag) == 1))
#endif
    {
        ;
    }

    g_ske_lp_reg->sr2 = clear_flag; // write 0 to clear

#if 0
    while((g_ske_lp_reg->ctrl & finish_flag) == 1)
    {;}
#endif

    return SKE_SUCCESS;
}

/**
 * @brief           set ske_lp key
 * @param[in]       key                  - key in word buffer
 * @param[in]       idx                  - key index, only 1 and 2 are valid
 * @param[in]       key_words            - word length of key
 * @return          none
 * @note
 *        1. if idx is 1, set key1 register, else if idx is 2, set key2 register, please
 *           make sure idx is valid
 */
void ske_lp_set_key_uint32(unsigned int *key, unsigned int idx, unsigned int key_words)
{
    (void)idx;

    volatile unsigned int *key_reg = g_ske_lp_reg->key1;
    unsigned int i;

    for (i = 0; i < key_words; i++)
    {
        key_reg[i] = key[i]; // printf("\r\nkey %08x", key_reg[i]);
    }
}

/**
 * @brief           set ske_lp iv
 * @param[in]       iv                   - iv in word buffer
 * @param[in]       block_words          - word length of ske_lp block
 * @return          none
 * @note
 *        1. please make sure the three parameters are valid
 */
void ske_lp_set_iv_uint32(const unsigned int *iv, unsigned int block_words)
{
#if 1
    unsigned int i;

    for (i = 0; i < block_words; i++)
    {
        g_ske_lp_reg->iv[i] = iv[i];
    }
#else
    g_ske_lp_reg->iv[0] = iv[0];
    g_ske_lp_reg->iv[1] = iv[1];

    if (4 == block_words) // for AES/SM4
    {
        g_ske_lp_reg->iv[2] = iv[2];
        g_ske_lp_reg->iv[3] = iv[3];
    }
    else
    {
        ;
    }
#endif
}

#if (defined(SUPPORT_SKE_MODE_GCM) || defined(SUPPORT_SKE_MODE_CCM))
/**
 * @brief           set aad bits(just for ske_lp ccm/gcm mode)
 * @param[in]       aad_bytes            - byte length of aad
 * @return          none
 * @note
 *        1. this function is just for CCM/GCM mod
 */
void ske_lp_set_aad_len_uint32(unsigned int aad_bytes)
{
    g_ske_lp_reg->ske_a_len_l = ((aad_bytes) << 3) & 0xFFFFFFFF;
    g_ske_lp_reg->ske_a_len_h = aad_bytes >> (32 - 3);
}
#endif

#if (defined(SUPPORT_SKE_MODE_GCM) || defined(SUPPORT_SKE_MODE_CCM) || defined(SUPPORT_SKE_MODE_XTS))
/**
 * @brief           set plaintext/ciphertext bits(just for ske_lp ccm/gcm/xts mode)
 * @param[in]       c_bytes              - byte length of plaintext/ciphertext
 * @return          none
 * @note
 *        1. this function is just for CCM/GCM/XTS mod
 */
void ske_lp_set_c_len_uint32(unsigned int c_bytes)
{
    g_ske_lp_reg->ske_c_len_l = ((c_bytes) << 3) & 0xFFFFFFFF;
    g_ske_lp_reg->ske_c_len_h = c_bytes >> (32 - 3);
}
#endif

/**
 * @brief           input one block
 * @param[in]       in                   - plaintext or ciphertext in word buffer
 * @param[in]       block_words          - word length of ske_lp block
 * @return          none
 * @note
 *        1. in is a word buffer of only one block
 */
void ske_lp_simple_set_input_block(const unsigned int *in, unsigned int block_words)
{
#if 0
    unsigned int i;

    for(i = 0; i < block_words; i++)
    {
        g_ske_lp_reg->m_din[i] = in[i];
    }
#else
    g_ske_lp_reg->m_din[0] = in[0];
    g_ske_lp_reg->m_din[1] = in[1];

    if (4 == block_words) // for AES/SM4
    {
        g_ske_lp_reg->m_din[2] = in[2];
        g_ske_lp_reg->m_din[3] = in[3];
    }
    else
    {
        ;
    }
#endif
}

/**
 * @brief           output one block
 * @param[out]      out                  - one block output of ske_lp in word buffer
 * @param[in]       block_words          - word length of ske_lp block
 * @return          none
 */
void ske_lp_simple_get_output_block(unsigned int *out, unsigned int block_words)
{
#if 0
    unsigned int i;

    for(i = 0; i < block_words; i++)
    {
        out[i] = g_ske_lp_reg->m_dout[i];
    }
#else
    out[0] = g_ske_lp_reg->m_dout[0];
    out[1] = g_ske_lp_reg->m_dout[1];

    if (4 == block_words) // for AES/SM4
    {
        out[2] = g_ske_lp_reg->m_dout[2];
        out[3] = g_ske_lp_reg->m_dout[3];
    }
    else
    {
        ;
    }
#endif
}

/**
 * @brief           ske_lp expand key
 * @param[in]       dma_en               - for DMA mode(not 0) or not(0)
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. must be called after ske_lp_set_crypto() and ske_lp_set_alg(), and the key is set already
 */
unsigned int ske_lp_expand_key(unsigned int dma_en)
{
    volatile unsigned int mask = ~(((unsigned int)1) << SKE_LP_UP_CFG_OFFSET);
    volatile unsigned int flag = (((unsigned int)1) << SKE_LP_UP_CFG_OFFSET);
    unsigned int ret;

    // update cfg
    g_ske_lp_reg->cfg |= flag;

    // expand key
    ske_lp_start();

    ret = ske_lp_wait_till_done();
    if (SKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    // not update cfg
    if (SKE_LP_DMA_ENABLE == dma_en)
    {
        ske_lp_set_dma_mode();
    }
    else
    {
        ;
    }

    g_ske_lp_reg->cfg &= mask;

    return SKE_SUCCESS;
}

/************************* DMA *************************/

#ifdef SKE_LP_DMA_FUNCTION
/**
 * @brief           wait till ske_lp dma calculating is done
 * @param[in]       callback             - callback function pointer, this could be NULL, means doing nothing
 * @return          SKE_SUCCESS(success), other(error)
 */
unsigned int ske_lp_dma_calc_wait_till_done(SKE_CALLBACK callback)
{
    volatile unsigned int finish_flag = 1;
    volatile unsigned int clear_flag = 0;

#ifdef SUPPORT_SKE_IRQ
    volatile unsigned int flag_irq = (((unsigned int)1) << SKE_LP_IRQ_OFFSET);

    if (ske_lp_reg->cfg & flag_irq)
    {
        return SKE_SUCCESS;
    }
    else
    {
        ;
    }
#endif

    while (!(g_ske_lp_reg->sr2 & finish_flag))
    {
        if (callback)
        {
            callback();
        }
        else
        {
            ;
        }
    }

    g_ske_lp_reg->sr2 = clear_flag;

    return SKE_SUCCESS;
}

/**
 * @brief           basic ske_lp DMA operation
 * @param[in]       ctx                  - ske_ctx_t context pointer
 * @param[in]       in                   - plaintext or ciphertext
 * @param[out]      out                  - ciphertext or plaintext
 * @param[in]       in_words             - word length of in, must be multiples of block word length
 * @param[in]       out_words            - word length of out, must be multiples of block word length
 * @param[in]       callback             - callback function pointer, this could be NULL, means doing nothing
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. in_words & out_words must be multiples of block words.
 *        2. it could be without output, namely, out can be NULL, out_words can be 0(for input AAD, or CBC_MAC/CMAC mode
 */
unsigned int ske_lp_dma_operate(ske_ctx_t *ctx, const unsigned int *in, unsigned int *out, unsigned int in_words, unsigned int out_words, SKE_CALLBACK callback)
{
    (void)ctx;

    if (NULL == in)
    {
        return SKE_BUFFER_NULL;
    }
    else if (0 == in_words)
    {
        return SKE_SUCCESS;
    }
    else
    {
        ;
    }

    // src & dst addr
    g_ske_lp_reg->dma_sa_l = (unsigned int)in;
    g_ske_lp_reg->dma_da_l = (unsigned int)out;

    // data word length
    g_ske_lp_reg->dma_rlen = in_words << 2;
    g_ske_lp_reg->dma_wlen = out_words << 2;

    ske_tx_dma(ske_get_tx_dma_channel(), (unsigned int)in, in_words << 2);
    ske_rx_dma(ske_get_rx_dma_channel(), (unsigned int)out, out_words << 2);
    ske_lp_start();

    return ske_lp_dma_calc_wait_till_done(callback);
}

/**
 * @brief           clear the last (16-bytes) of the block in(16 bytes)
 * @param[in]       in                   - one block buffer(128bits, for AES/SM4 GCM, CCM mode)
 * @param[in]       bytes                - real bytes of in, must be in[1,16]
 * @return          none
 * @note
 *        1. this function is for GCM,CCM mode of DMA
 */
void clear_block_tail(unsigned int in[4], unsigned int bytes)
{
    unsigned int i;

    i = bytes / 4;
    bytes &= 3;

    if (bytes)
    {
#ifdef SKE_LP_CPU_BIG_ENDIAN
        in[i] &= 0xFFFFFFFF << (32 - bytes * 8);
#else
        in[i] &= (1 << (bytes * 8)) - 1;
#endif
        i++;
    }

    while (i < 4)
    {
        in[i++] = 0;
    }
}
#endif

/**
 * @brief           update ske_lp some blocks without output
 * @param[in]       ctx                  - ske_ctx_t context pointer
 * @param[in]       in                   - some blocks
 * @param[in]       bytes                - byte length of in
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the bytes is a multiple of block byte length ctx->block_bytes
 *        2. this function is called by CCM(input aad)/GCM(input aad)/CMAC/CBC-MAC mod
 */
unsigned int ske_lp_update_blocks_no_output(ske_ctx_t *ctx, const unsigned char *in, unsigned int bytes)
{
    unsigned int in_word_align;
    unsigned int tmp_in[4];
    unsigned int i;
    unsigned int ret;

    if (((unsigned int)in) & 3)
    {
        in_word_align = 0;
    }
    else
    {
        in_word_align = 1;
    }

    // input one block ---> calculating ---> output one block
    for (i = 0; i < bytes; i += ctx->block_bytes)
    {
        if (in_word_align)
        {
            ske_lp_simple_set_input_block((const unsigned int *)in, ctx->block_words);
        }
        else
        {
            memcpy_(tmp_in, in, ctx->block_bytes);
            ske_lp_simple_set_input_block((unsigned int *)tmp_in, ctx->block_words);
        }

        ske_lp_start();

        ret = ske_lp_wait_till_done();
        if (SKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            ;
        }

        in += ctx->block_bytes;
    }

    return SKE_SUCCESS;
}

/**
 * @brief           update ske_lp some blocks and get the same number of blocks
 * @param[in]       ctx                  - ske_ctx_t context pointer
 * @param[in]       in                   - some blocks
 * @param[out]      out                  - the same number of blocks;
 * @param[in]       bytes                - byte length of in
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the bytes is a multiple of block byte length ctx->block_byte
 */
unsigned int ske_lp_update_blocks_internal(ske_ctx_t *ctx, unsigned char *in, unsigned char *out, unsigned int bytes)
{
    unsigned int in_word_align, out_word_align;
    unsigned int tmp_in[4];
    unsigned int i, round = bytes / ctx->block_bytes;
    unsigned int block_bytes = ctx->block_bytes;
    unsigned int ret;

    if (((unsigned int)in) & 3)
    {
        in_word_align = 0;
    }
    else
    {
        in_word_align = 1;
    }

    if (((unsigned int)out) & 3)
    {
        out_word_align = 0;
    }
    else
    {
        out_word_align = 1;
    }

    if (in_word_align && out_word_align)
    {
#if 1
        if (block_bytes == 16) // for AES/SM4
        {
            for (i = 0; i < round; i++)
            {
                // ske_lp_simple_set_input_block((unsigned int *)in, ctx->block_words);
                g_ske_lp_reg->m_din[0] = ((unsigned int *)in)[0];
                g_ske_lp_reg->m_din[1] = ((unsigned int *)in)[1];
                g_ske_lp_reg->m_din[2] = ((unsigned int *)in)[2];
                g_ske_lp_reg->m_din[3] = ((unsigned int *)in)[3];

                ske_lp_start();

                ret = ske_lp_wait_till_done();
                if (SKE_SUCCESS != ret)
                {
                    return ret;
                }
                else
                {
                    ;
                }

                // ske_lp_simple_get_output_block((unsigned int *)out, ctx->block_words);
                ((unsigned int *)out)[0] = g_ske_lp_reg->m_dout[0];
                ((unsigned int *)out)[1] = g_ske_lp_reg->m_dout[1];
                ((unsigned int *)out)[2] = g_ske_lp_reg->m_dout[2];
                ((unsigned int *)out)[3] = g_ske_lp_reg->m_dout[3];

                in += block_bytes;
                out += block_bytes;
            }
        }
        else // for DES/3DES
        {
            for (i = 0; i < round; i++)
            {
                // ske_lp_simple_set_input_block((unsigned int *)in, ctx->block_words);
                g_ske_lp_reg->m_din[0] = ((unsigned int *)in)[0];
                g_ske_lp_reg->m_din[1] = ((unsigned int *)in)[1];

                ske_lp_start();

                ret = ske_lp_wait_till_done();
                if (SKE_SUCCESS != ret)
                {
                    return ret;
                }
                else
                {
                    ;
                }

                // ske_lp_simple_get_output_block((unsigned int *)out, ctx->block_words);
                ((unsigned int *)out)[0] = g_ske_lp_reg->m_dout[0];
                ((unsigned int *)out)[1] = g_ske_lp_reg->m_dout[1];

                in += block_bytes;
                out += block_bytes;
            }
        }
#else
        for (i = 0; i < round; i++)
        {
            ske_lp_simple_set_input_block((unsigned int *)in, ctx->block_words); // print_buf_u32((unsigned int *)in_tmp, ctx->block_words, "in");

            ske_lp_start();

            ret = ske_lp_wait_till_done();
            if (SKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {
                ;
            }

            ske_lp_simple_get_output_block((unsigned int *)out, ctx->block_words); // print_buf_u32((unsigned int *)out_tmp, ctx->block_words, "out");

            in += block_bytes;
            out += block_bytes;
        }
#endif
    }
    else
    {
        for (i = 0; i < round; i++)
        {
            if (in_word_align)
            {
                ske_lp_simple_set_input_block((unsigned int *)in, ctx->block_words);
            }
            else
            {
                memcpy_(tmp_in, in, block_bytes);
                ske_lp_simple_set_input_block((unsigned int *)tmp_in, ctx->block_words);
            }

            ske_lp_start();

            ret = ske_lp_wait_till_done();
            if (SKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {
                ;
            }

            if (out_word_align)
            {
                ske_lp_simple_get_output_block((unsigned int *)out, ctx->block_words);
            }
            else
            {
                ske_lp_simple_get_output_block((unsigned int *)tmp_in, ctx->block_words);
                memcpy_(out, tmp_in, block_bytes);
            }

            in += block_bytes;
            out += block_bytes;
        }
    }

    return SKE_SUCCESS;
}

#ifdef SUPPORT_SKE_MODE_GMAC
/**
 * @brief           for GMAC mode to input message blocks(just for AES/SM4, block size is 16 bytes)
 * @param[in]       in                   - some blocks
 * @param[in]       bytes                - byte length of in
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the bytes is a multiple of block byte length 16
 *        2. this function is like ske_lp_update_blocks_internal(), but discard output
 */
unsigned int ske_lp_gmac_update_blocks_internal(unsigned char *in, unsigned int bytes)
{
    unsigned int tmp[4];
    unsigned int i, round = bytes / 16;
    unsigned int ret;

    if (0 == (((unsigned int)in) & 3))
    {
        for (i = 0; i < round; i++)
        {
            // ske_lp_simple_set_input_block((unsigned int *)in, ctx->block_words);
            g_ske_lp_reg->m_din[0] = ((unsigned int *)in)[0];
            g_ske_lp_reg->m_din[1] = ((unsigned int *)in)[1];
            g_ske_lp_reg->m_din[2] = ((unsigned int *)in)[2];
            g_ske_lp_reg->m_din[3] = ((unsigned int *)in)[3];

            ske_lp_start();

            ret = ske_lp_wait_till_done();
            if (SKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {
                ;
            }

            //          *tmp = g_ske_lp_reg->m_dout[0];
            //          *tmp = g_ske_lp_reg->m_dout[1];
            //          *tmp = g_ske_lp_reg->m_dout[2];
            //          *tmp = g_ske_lp_reg->m_dout[3];

            in += 16;
        }
    }
    else
    {
        // input one block ---> calculating ---> output one block
        for (i = 0; i < round; i++)
        {
            memcpy_(tmp, in, 16);
            ske_lp_simple_set_input_block((unsigned int *)tmp, 4);

            ske_lp_start();

            ret = ske_lp_wait_till_done();
            if (SKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {
                ;
            }

            //          ske_lp_simple_get_output_block((unsigned int *)tmp, 4);

            in += 16;
        }
    }

    return SKE_SUCCESS;
}
#endif

#if (defined(SUPPORT_SKE_TDES_128))
/**
 * @brief           TDES input one block and output one block
 * @param[in]       is_EEE               - is tdes SKE_ALG_TDES_EEE_128/SKE_ALG_TDES_EEE_192 or not
 * @param[in]       key                  - TDES key, 24 bytes
 * @param[in]       crypto               - SKE_CRYPTO_ENCRYPT or SKE_CRYPTO_DECRYPT
 * @param[in]       in                   - one block
 * @param[out]      out                  - one block
 * @return          SKE_SUCCESS(success), other(error)
 * @note
 *        1. hw DES must be set already
 *        2. hw ECB must be set already
 */
unsigned int tdes_ecb_update_one_block(unsigned int is_EEE, unsigned int key[6], ske_crypto_e crypto, unsigned int in[2], unsigned int out[2])
{
    unsigned int ret;

    /***************** round 1 *****************/
    ske_lp_set_cpu_mode(); // to update cfg
    ske_lp_set_crypto(crypto);

    ske_lp_set_key_uint32((SKE_CRYPTO_ENCRYPT == crypto) ? key : key + 4, 1, 2);
    ske_lp_expand_key(SKE_LP_DMA_DISABLE);

    g_ske_lp_reg->m_din[0] = in[0];
    g_ske_lp_reg->m_din[1] = in[1];

    ske_lp_start();
    ret = ske_lp_wait_till_done();
    if (SKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    /***************** round 2 *****************/
    ske_lp_set_cpu_mode(); // to update cfg
    if (!is_EEE)
    {
        ske_lp_set_crypto((SKE_CRYPTO_ENCRYPT == crypto) ? SKE_CRYPTO_DECRYPT : SKE_CRYPTO_ENCRYPT);
    }
    else
    {
        ;
    }

    ske_lp_set_key_uint32(key + 2, 1, 2);
    ske_lp_expand_key(SKE_LP_DMA_DISABLE);

    g_ske_lp_reg->m_din[0] = g_ske_lp_reg->m_dout[0];
    g_ske_lp_reg->m_din[1] = g_ske_lp_reg->m_dout[1];

    ske_lp_start();
    ret = ske_lp_wait_till_done();
    if (SKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    /***************** round 3 *****************/
    ske_lp_set_cpu_mode(); // to update cfg
    if (!is_EEE)
    {
        ske_lp_set_crypto(crypto);
    }
    else
    {
        ;
    }

    ske_lp_set_key_uint32((SKE_CRYPTO_ENCRYPT == crypto) ? key + 4 : key, 1, 2);
    ske_lp_expand_key(SKE_LP_DMA_DISABLE);

    g_ske_lp_reg->m_din[0] = g_ske_lp_reg->m_dout[0];
    g_ske_lp_reg->m_din[1] = g_ske_lp_reg->m_dout[1];

    ske_lp_start();
    ret = ske_lp_wait_till_done();
    if (SKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    out[0] = g_ske_lp_reg->m_dout[0];
    out[1] = g_ske_lp_reg->m_dout[1];

    ret = SKE_SUCCESS;

END:

    return ret;
}
#endif

/*
 * @brief       ske_lp AES128_ECB encrypting or decrypting one block(16bytes) (CPU style, one-off style)
 * @param[in]   key                  - key
 * @param[in]   in                   - one block.
 * @param[out]  out                  - one block.
 * @return      SKE_SUCCESS(success), other(error)
 * @note
 *       1.please make sure all parameter valid, include crypto is encryption or decryption
 *       2.please make sure key/iv/in/out address is word aligned.
 */
_attribute_ram_code_    /*!< BLE SDK USED */
unsigned int ske_lp_aes128_ecb_one_block(ske_crypto_e crypto, unsigned int *key, unsigned int *in, unsigned int *out)
{
    // set
    if (SKE_CRYPTO_ENCRYPT == crypto)
    {
        g_ske_lp_reg->cfg = 0x10001001U; // AES128/ECB/ENC
    }
    else
    {
        g_ske_lp_reg->cfg = 0x10001801U; // AES128/ECB/DEC
    }

    // set key
    g_ske_lp_reg->key1[0] = key[0];
    g_ske_lp_reg->key1[1] = key[1];
    g_ske_lp_reg->key1[2] = key[2];
    g_ske_lp_reg->key1[3] = key[3];

    // start
    g_ske_lp_reg->ctrl |= 1U;

    // wait done
    while (((g_ske_lp_reg->sr2 & 1U) == 0U) || ((g_ske_lp_reg->sr1 & 1U) == 1U))
    {
    }

    // clear sr
    g_ske_lp_reg->sr2 = 0U;

    // down cfg
    g_ske_lp_reg->cfg &= 0xFFFFEFFFU;

    // input 16 bytes data
    g_ske_lp_reg->m_din[0] = in[0];
    g_ske_lp_reg->m_din[1] = in[1];
    g_ske_lp_reg->m_din[2] = in[2];
    g_ske_lp_reg->m_din[3] = in[3];

    // start
    g_ske_lp_reg->ctrl |= 1;

    // wait done
    while (((g_ske_lp_reg->sr2 & 1U) == 0) || ((g_ske_lp_reg->sr1 & 1U) == 1))
    {
        ;
    }

    // clear sr
    g_ske_lp_reg->sr2 = 0U;

    // get output
    out[0] = g_ske_lp_reg->m_dout[0];
    out[1] = g_ske_lp_reg->m_dout[1];
    out[2] = g_ske_lp_reg->m_dout[2];
    out[3] = g_ske_lp_reg->m_dout[3];

    return SKE_SUCCESS;
}

/*
 * @brief       ske_lp AES128_ECB encrypting one block(16bytes) (CPU style, one-off style)
 * @param[in]   key                  - key
 * @param[in]   plaintext            - one block plaintext
 * @param[out]  encrypted_data       - one block ciphertext
 * @return      SKE_SUCCESS(success), other(error)
 */
#if 0   /*!< BLE SDK USED */
void aes_encryption_be(unsigned char *key, unsigned char *plaintext, unsigned char *encrypted_data)
{
    ske_lp_aes128_ecb_one_block(SKE_CRYPTO_ENCRYPT, (unsigned int *)key, (unsigned int *)plaintext, (unsigned int *)encrypted_data);
}
#endif  /*!< BLE SDK USED */
/*
 * @brief       ske_lp AES128_ECB encrypting one block(16bytes) (CPU style, one-off style)
 * @param[in]   key                  - key
 * @param[in]   ciphertext           - one block ciphertext
 * @param[out]  decrypted_data       - one block plaintext
 * @return      SKE_SUCCESS(success), other(error)
 */
void aes_decryption_be(unsigned char *key, unsigned char *ciphertext, unsigned char *decrypted_data)
{
    ske_lp_aes128_ecb_one_block(SKE_CRYPTO_DECRYPT, (unsigned int *)key, (unsigned int *)ciphertext, (unsigned int *)decrypted_data);
}
