/********************************************************************************************************
 * @file    chacha20_poly1305_basic.c
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
#include "lib/include/chacha20_poly1305/chacha20_poly1305_basic.h"
#include "lib/include/chacha20_poly1305/chacha20_poly1305_portable.h"
#include "lib/include/crypto_common/utility.h"
#include "driver.h"

#if 0
//chacha20_poly1305 register pointer
volatile static chacha20_poly1305_reg_t * const g_chacha20_poly1305_reg = (chacha20_poly1305_reg_t *)(CHACHA20_POLY1305_BASE_ADDR);
#endif

/**
 * @brief       get chacha20_poly1305 IP version
 * @return      chacha20_poly1305 IP version
 */
unsigned int chacha20_poly1305_get_version(void)
{
    return rCHACHA20_POLY1305_VERSION;
}

/**
 * @brief       set chacha20_poly1305 to be CPU mode
 * @return      none
 */
void chacha20_poly1305_set_cpu_mode(void)
{
    volatile unsigned int mask = ~(((unsigned int)1)<<CHACHA20_POLY1305_DMA_OFFSET);

    rCHACHA20_POLY1305_CFG &= mask;
}

/**
 * @brief       set chacha20_poly1305 to be DMA mode
 * @return      none
 */
void chacha20_poly1305_set_dma_mode(void)
{
    volatile unsigned int flag = (((unsigned int)1)<<CHACHA20_POLY1305_DMA_OFFSET);

    rCHACHA20_POLY1305_CFG |= flag;
}

/**
 * @brief       chacha20_poly1305 soft reset
 * @return      none
 */
void chacha20_poly1305_soft_reset(void)
{
    volatile unsigned int flag = (((unsigned int)1)<<CHACHA20_POLY1305_SOFT_RESET_OFFSET);

    rCHACHA20_POLY1305_SOFT_RST_N |= flag;
}

/**
 * @brief       chacha20_poly1305 set crypto mode
 * @param[in]   crypto               - dec or enc.
 * @return      none
 */
void chacha20_poly1305_set_crypto(CHACHA20_POLY1305_CRYPTO crypto)
{
    volatile unsigned int mask = ~(((unsigned int)1) << CHACHA20_POLY1305_CRYPTO_OFFSET);

    rCHACHA20_POLY1305_CFG &= mask;
    rCHACHA20_POLY1305_CFG |= (((unsigned int)crypto) << CHACHA20_POLY1305_CRYPTO_OFFSET);
}

/**
 * @brief       chacha20_poly1305 set sec_stage
 * @param[in]   stage               - data interleave stage.
 * @return      error code
 */
void chacha20_poly1305_set_sec_stage(CHACHA20_POLY1305_SEC_STAGE stage)
{
    volatile unsigned int mask = ~(0x00000003U << CHACHA20_POLY1305_SEC_STAGE_OFFSET);

    rCHACHA20_POLY1305_CFG &= mask;                                                              //clear bit [0:1]
    rCHACHA20_POLY1305_CFG |= (((unsigned int)stage) << CHACHA20_POLY1305_SEC_STAGE_OFFSET);         //set stage
}


/**
 * @brief       chacha20_poly1305 start
 * @return      none
 */
void chacha20_poly1305_start(void)
{
    volatile unsigned int start_flag = 1;

    rCHACHA20_POLY1305_CTRL |= start_flag;
}


/**
 * @brief       chacha20_poly1305 start
 * @return      none
 */
void chacha20_poly1305_wait_till_done(void)
{
    volatile unsigned int finish_flag = 1;
    volatile unsigned int clear_flag = 0;

    while(!(rCHACHA20_POLY1305_RISR & finish_flag))
    {;}

    rCHACHA20_POLY1305_RISR = clear_flag;  //write 0 to clear
}


/**
 * @brief       chacha20_poly1305 set key
 * @param[in]   key               - key in word buffer.
 * @return      none
 */
void chacha20_poly1305_set_key(unsigned char key[32])
{
    unsigned char i = 8;
    unsigned int tmp[8];
    unsigned int *key_ptr = (unsigned int *)key;

    if(((unsigned int)key)&3)
    {
        memcpy_((unsigned char *)tmp, key, 32);
        key_ptr = tmp;
    }
    
    while (i--)
    {
        rCHACHA20_POLY1305_KEY(i) = key_ptr[i];
    }
}


/**
 * @brief       chacha20_poly1305 set key
 * @param[in]   constant            - constant of nonce.
 * @return      none
 */
void chacha20_poly1305_set_const(unsigned int constant)
{
    rCHACHA20_POLY1305_CONSTANT = constant;
}


/**
 * @brief       chacha20_poly1305 set nonce
 * @param[in]   iv            - iv in word buffer.
 * @return      none
 */
void chacha20_poly1305_set_iv(unsigned char iv[8])
{
    unsigned int tmp[2];
    unsigned int *iv_ptr = (unsigned int *)iv;

    if(((unsigned int)iv)&3)
    {
        memcpy_((unsigned char *)tmp, iv, 8);
        iv_ptr = tmp;
    }

    rCHACHA20_POLY1305_IV(0) = iv_ptr[0];
    rCHACHA20_POLY1305_IV(1) = iv_ptr[1];
}

/**
 * @brief       chacha20_poly1305 set length
 * @param[in]   aad_bytes            - byte length of aad.
 * @param[in]   payload_bytes        - byte length of payload.
 * @return      none
 */
void chacha20_poly1305_set_length(unsigned long long aad_bytes, unsigned long long payload_bytes)
{
    unsigned int last_aad = aad_bytes & 3;
    unsigned int last_payload = payload_bytes & 3;

    if ((0 == last_aad) && (0 != aad_bytes))
    {
        last_aad = 4;
    }

    if ((0 == last_payload) && (0 != payload_bytes))
    {
        last_payload = 4;
    }

    chacha20_poly1305_set_length_clean_aad_last(aad_bytes, payload_bytes);
    
    rCHACHA20_POLY1305_LENGTH(4) = (last_aad << 3) | (last_payload << 0);
}

/**
 * @brief       chacha20_poly1305 set length and set aad last 0
 * @param[in]   aad_bytes            - byte length of aad.
 * @param[in]   payload_bytes        - byte length of payload.
 * @return      none
 */
void chacha20_poly1305_set_length_clean_aad_last(unsigned long long aad_bytes, unsigned long long payload_bytes)
{
    unsigned int last_payload = payload_bytes & 3;

    if ((0 == last_payload) && (0 != payload_bytes))
    {
        last_payload = 4;
    }

    rCHACHA20_POLY1305_LENGTH(0) = (aad_bytes & 0xFFFFFFFF);
    rCHACHA20_POLY1305_LENGTH(1) = aad_bytes >> 32;
    rCHACHA20_POLY1305_LENGTH(2) = (payload_bytes & 0xFFFFFFFF);
    rCHACHA20_POLY1305_LENGTH(3) = payload_bytes >> 32;
    rCHACHA20_POLY1305_LENGTH(4) = last_payload;
}

/**
 * @brief       chacha20_poly1305 set cnt
 * @param[in]   counter            - counter.
 * @return      none
 */
void chacha20_poly1305_set_cnt(unsigned int counter)
{
    rCHACHA20_POLY1305_CNT = counter;
}

/**
 * @brief       chacha20_poly1305 set tag_in
 * @param[in]   tag_in         - tag in byte buffer.
 * @return      none
 */
void chacha20_poly1305_set_tag_in(unsigned char tag_in[17])
{
    unsigned int i = 4;
    while (i--)
    {
        rCHACHA20_POLY1305_TAG_IN(i) = ((unsigned int *)tag_in)[i];
    }

    rCHACHA20_POLY1305_TAG_IN(4) = (unsigned int)tag_in[16];
}

/**
 * @brief       chacha20_poly1305 get current counter
 * @return      none
 */
unsigned int chacha20_poly1305_get_current_cnt(void)
{
    return rCHACHA20_POLY1305_COUT;
}

/**
 * @brief       chacha20_poly1305 get current tag
 * @param[out]  tag         - current tag.
 * @return      none
 */
void chacha20_poly1305_get_current_tag(unsigned char tag[17])
{
    ((unsigned int *)tag)[0] = rCHACHA20_POLY1305_TOUT(0);
    ((unsigned int *)tag)[1] = rCHACHA20_POLY1305_TOUT(1);
    ((unsigned int *)tag)[2] = rCHACHA20_POLY1305_TOUT(2);
    ((unsigned int *)tag)[3] = rCHACHA20_POLY1305_TOUT(3);
    tag[16] = rCHACHA20_POLY1305_TOUT(4) & 3;
}

/**
 * @brief       chacha20_poly1305 get err code
 * @return      error(err code)
 */
unsigned int chacha20_poly1305_get_error(void)
{
    if(0 == rCHACHA20_POLY1305_ERR_CODE)
    {
        return CHACHA20_POLY1305_SUCCESS;
    }
    else
    {
        return CHACHA20_POLY1305_CONFIG_INVALID;
    }
}

/**
 * @brief       set whether chacha20_poly1305 next input data is the last word data or not (CPU mode)
 * @param[in]   is_last_word         -last word data.
 * @return      none
 */
void chacha20_poly1305_set_last_word(unsigned int is_last_word)
{
    volatile unsigned int flag = 1;
    volatile unsigned int mask = 0;

    if(is_last_word)
    {
        rCHACHA20_POLY1305_LAST |= flag;
    }
    else
    {
        rCHACHA20_POLY1305_LAST &= mask;
    }
}

/**
 * @brief       wait till chacha20_poly1305 can config data
 * @return      none
 */
void chacha20_poly1305_wait_ready_data_signal(void)
{
    volatile unsigned int finish_flag = 1;

    while(!(rCHACHA20_POLY1305_DIN_RDY & finish_flag))
    {;}
}

/**
 * @brief       wait till core is idle
 * @return      none
 */
void chacha20_poly1305_wait_core_idle(void)
{
    volatile unsigned int busy_flag = 1;
    volatile unsigned int clear_flag = 0;

    while(rCHACHA20_POLY1305_STATUS & busy_flag)
    {;}
        
    rCHACHA20_POLY1305_RISR = clear_flag;
}


/**
 * @brief       set one word data
 * @param[in]   in         - pointer to data.
 * @return      none
 */
void chacha20_poly1305_simple_set_input_word(unsigned int in[1])
{   
    chacha20_poly1305_wait_ready_data_signal();
    
    rCHACHA20_POLY1305_DIN = in[0];
}


/**
 * @brief       set 4 words input data as one block
 * @param[in]   in         - pointer to data.
 * @return      none
 */
void chacha20_poly1305_set_input_block(unsigned int in[4])
{   
    chacha20_poly1305_wait_ready_data_signal();
    
    rCHACHA20_POLY1305_DIN = in[0];
    rCHACHA20_POLY1305_DIN = in[1];
    rCHACHA20_POLY1305_DIN = in[2];
    rCHACHA20_POLY1305_DIN = in[3];
}

/**
 * @brief       get 4 words output data
 * @return      none
 */
void chacha20_poly1305_get_output_block(unsigned int out[4])
{
    out[0] = rCHACHA20_POLY1305_DOUT(0);
    out[1] = rCHACHA20_POLY1305_DOUT(1);
    out[2] = rCHACHA20_POLY1305_DOUT(2);
    out[3] = rCHACHA20_POLY1305_DOUT(3);
}

/**
 * @brief       update payload blocks and get the same number of blocks
 * @param[in]   in             - plaintext or ciphertext.
 * @param[out]  out            - ciphertext or plaintext.
 * @param[in]   bytes          - bytes length of input or output, must be multiples of 64.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.  the input data does not contain last word data.
  @endverbatim
 */
unsigned int chacha20_poly1305_update_blocks(unsigned char *in, unsigned char *out, unsigned int bytes)
{
    unsigned int in_word_align, out_word_align;
    unsigned int tmp[4];
    unsigned long long i;
    unsigned long long round = bytes/16;

    if(bytes & 63)
    {
        return CHACHA20_POLY1305_INPUT_INVALID;
    }
    else
    {;}
    
    if(((unsigned int)in) & 3)
    {
        in_word_align = 0;
    }
    else
    {
        in_word_align = 1;
    }

    if(((unsigned int)out) & 3)
    {
        out_word_align = 0;
    }
    else
    {
        out_word_align = 1;
    }

    if(in_word_align && out_word_align)
    {
        for(i=0; i<round; i++)
        {
            chacha20_poly1305_set_input_block((unsigned int *)in);
            chacha20_poly1305_wait_till_done();
            chacha20_poly1305_get_output_block((unsigned int *)out);
            
            out += 16;
            in += 16;
        }
    }
    else
    {
        for(i = 0; i < round; i++)
        {
            if(in_word_align)
            {
                chacha20_poly1305_set_input_block((unsigned int *)in);
            }
            else
            {
                memcpy_((unsigned char *)tmp, in, 16);
                chacha20_poly1305_set_input_block(tmp);
            }

            chacha20_poly1305_wait_till_done();

            if(out_word_align)
            {
                chacha20_poly1305_get_output_block((unsigned int *)out);
            }
            else
            {
                chacha20_poly1305_get_output_block(tmp);
                memcpy_(out, (unsigned char *)tmp, 16);
            }

            out += 16;
            in += 16;
        }
    }

    return CHACHA20_POLY1305_SUCCESS;
}

/**
 * @brief       update payload blocks and get the same number of blocks
 * @param[in]   in             - plaintext or ciphertext.
 * @param[out]  out            - ciphertext or plaintext.
 * @param[in]   remainder      - bytes length of input or output, must be multiples of 64.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.  the input data must contain last word data.
  @endverbatim
 */
unsigned int chacha20_poly1305_update_last_block(unsigned char *in, unsigned char *out, unsigned int remainder)
{
    unsigned long long i;
    unsigned long long round;
    unsigned int tmp[4];
    unsigned int last_block_bytes = remainder & 15;

    round = (remainder+3)>>2;
    remainder = remainder & 3;
    
    if(0 == remainder)
    {
        remainder = 4;
    }
    else
    {;}

    if(0 == last_block_bytes)
    {
        last_block_bytes = 16;
    }
    
    for(i=1; i <= round-1; i++)
    {
        chacha20_poly1305_set_input_word((unsigned int *)in, 1);
        in += 4;

        if(0 == (i & 3))
        {
            chacha20_poly1305_wait_till_done();
            
            if(((unsigned int)out)&3)
            {
                chacha20_poly1305_get_output_block(tmp);
                memcpy_(out, (unsigned char *)tmp, 16);
            }
            else
            {
                chacha20_poly1305_get_output_block((unsigned int *)out);
            }
            out += 16;
        }
    }
    
    chacha20_poly1305_set_last_word(1);
    memcpy_((unsigned char *)tmp, in, remainder);
    chacha20_poly1305_set_input_word(tmp, 1);
    
    chacha20_poly1305_wait_core_idle();
    
    chacha20_poly1305_get_output_block((unsigned int *)tmp);
    memcpy_(out, (unsigned char *)tmp, last_block_bytes);

    return chacha20_poly1305_get_error();
}

/**
 * @brief       update chacha20_poly1305 some blocks without output
 * @param[in]   in             - data.
 * @param[in]   words          - word length of in.
 * @return      none
 * @note
  @verbatim
      -# 1.  use for all aad input and get current tag.
  @endverbatim
 */
void chacha20_poly1305_set_input_word(unsigned int *in, unsigned int words)
{
    unsigned int in_word_align;
    unsigned int tmp_in[1];
    unsigned long long i;

    if(((unsigned int)in) & 3)
    {
        in_word_align = 0;
    }
    else
    {
        in_word_align = 1;
    }

    for (i = 0; i < words; i++)
    {
        if(in_word_align)
        {
            chacha20_poly1305_simple_set_input_word((unsigned int *)in);
        }
        else
        {
            memcpy_((unsigned char *)tmp_in, (unsigned char *)in, 4);
            chacha20_poly1305_simple_set_input_word((unsigned int *)tmp_in);
        }
        in++;
    }
}


#ifdef CHACHA20_POLY1305_DMA_FUNCTION

/**
 * @brief       basic chacha20_poly1305 DMA set source address and byte length of aad
 * @param[in]   aad             - source address of aad.
 * @param[out]  aad_bytes       - byte length of aad.
 * @return      none
 */
void chacha20_poly1305_dma_set_aad(unsigned int *aad, unsigned int aad_bytes)
{
    rCHACHA20_POLY1305_DMA_SADDR_A = (unsigned int)aad;
    rCHACHA20_POLY1305_DMA_RLEN_A  = aad_bytes;
}


/**
 * @brief       basic chacha20_poly1305 DMA set source address and byte length of payload
 * @param[in]   payload             - source address of payload.
 * @param[out]  payload_bytes       - byte length of payload.
 * @return      none
 */
void chacha20_poly1305_dma_set_payload(unsigned int *payload, unsigned int payload_bytes)
{
    rCHACHA20_POLY1305_DMA_SADDR_D = (unsigned int)payload;
    rCHACHA20_POLY1305_DMA_LEN_D   = payload_bytes;
}

/**
 * @brief       update payload blocks and get the same number of blocks (DMA mode)
 * @param[in]   in             - plaintext or ciphertext.
 * @param[out]  out            - ciphertext or plaintext.
 * @param[in]   bytes          - bytes length of input or output.
 * @return      0:success     other:error
 */
unsigned int chacha20_poly1305_dma_operate(unsigned int *in, unsigned int *out, unsigned int bytes)
{
    if((NULL == in && 0 != bytes) || (NULL == out && 0 != bytes))
    {
        return CHACHA20_POLY1305_BUFFER_NULL;
    }
    else
    {;}

    //src & dst addr
    rCHACHA20_POLY1305_DMA_SADDR_D = (unsigned int)in;
    rCHACHA20_POLY1305_DMA_DADDR_D = (unsigned int)out;
    
    //data byte length
    rCHACHA20_POLY1305_DMA_RLEN_A = 0;
    rCHACHA20_POLY1305_DMA_LEN_D = bytes;

    chacha20_poly1305_tx_dma(chacha20_poly1305_get_tx_dma_channel(), (unsigned int)in, bytes);

    chacha20_poly1305_rx_dma(chacha20_poly1305_get_rx_dma_channel(), (unsigned int)out, bytes);

    chacha20_poly1305_start();
    chacha20_poly1305_wait_core_idle();

    return chacha20_poly1305_get_error();
}

#endif




