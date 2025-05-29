/********************************************************************************************************
 * @file    hash_basic.c
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
#include <string.h>
#include "lib/include/hash/hash_basic.h"
#include "lib/include/hash/hash_portable.h"
#include "lib/include/crypto_common/utility.h"
#include "driver.h"



#if 0
//hash register pointer
volatile static HASH_REG * const g_hash_reg = (HASH_REG *)HASH_BASE_ADDR;
#endif

 /**
  * @brief          get HFE IP version.
  * @return         HFE IP version
  */
unsigned int hash_get_version(void)
{
    return rHASH_VERSION;
}


/**
 * @brief       set hash to be CPU mode.
 * @return      none
 */
void hash_set_cpu_mode(void)
{
    volatile unsigned int mask = ~(((unsigned int)1)<<HASH_DMA_OFFSET);

    rHASH_CFG &= mask;
}


/**
 * @brief       set hash to be DMA mode.
 * @return      none
 */
void hash_set_dma_mode(void)
{
    volatile unsigned int flag = (((unsigned int)1)<<HASH_DMA_OFFSET);

    rHASH_CFG |= flag;
}


/**
 * @brief       set the specific hash algorithm.
 * @param[in]   hash_alg   - specific hash algorithm.
 * @return      none
 * @note
  @verbatim
      -# 1. please make sure hash_alg is valid.
  @endverbatim
 */
void hash_set_alg(HASH_ALG hash_alg)
{
    volatile unsigned int mask = (~0x0000000F);

    rHASH_CFG &= mask;
    rHASH_CFG |= hash_alg;
}


/**
 * @brief       enable hash interruption in CPU mode or DMA mode.
 * @return      none
 */
void hash_enable_interruption(void)
{
    volatile unsigned int flag = (((unsigned int)1)<<HASH_INTERRUPTION_OFFSET);

    rHASH_CFG |= flag;
}


/**
 * @brief       disable hash interruption in CPU mode or DMA mode.
 * @return      none
 */
void hash_disable_interruption(void)
{
    volatile unsigned int mask = ~(((unsigned int)1)<<HASH_INTERRUPTION_OFFSET);

    rHASH_CFG &= mask;
}


/**
 * @brief       set the tag whether current block is the last message block or not.
 * @param[in]   tag  - 0(no), other(yes).
 * @return      none
 * @note
  @verbatim
      -# 1. if it is the last block, please config hash_reg->HASH_MSG_LEN,
 *        then the hardware will do the padding and post-processing.
  @endverbatim
 */
void hash_set_last_block(unsigned int tag)
{
    volatile unsigned int flag = (((unsigned int)1) << HASH_LAST_OFFSET);
    volatile unsigned int mask = (~(((unsigned int)1) << HASH_LAST_OFFSET));

    if(tag)     //current block is the last one of the message
    {
        rHASH_CFG |= flag;
    }
    else        //current block is not the last one of the message
    {
        rHASH_CFG &= mask;
    }
}


/**
 * @brief       get current HASH iterator value.
 * @param[in]   iterator  - current hash iterator.
 * @param[in]   hash_iterator_words - word length.
 * @return      none
 */
void hash_get_iterator(unsigned char *iterator, unsigned int hash_iterator_words)
{
    unsigned int temp;
    unsigned int i;

    if(((unsigned int)iterator) & 3) //for the case that iterator is not aligned by word
    {
        for (i = 0; i < hash_iterator_words; i++)
        {
            temp = rHASH_OUT(i);
            memcpy_(iterator+(i<<2), &temp, 4);
        }
    }
    else
    {
        for (i = 0; i < hash_iterator_words; i++)
        {
            ((unsigned int *)iterator)[i] = rHASH_OUT(i);
        }
    }
}


/**
 * @brief       input current iterator value.
 * @param[in]   iterator             - hash iterator value.
 * @param[in]   hash_iterator_words  - iterator word length.
 * @return      none
 * @note
  @verbatim
      -# 1. iterator must be word aligned.
  @endverbatim
 */
void hash_set_iterator(const unsigned int *iterator, unsigned int hash_iterator_words)
{
    unsigned int i;

    for (i = 0; i < hash_iterator_words; i++)
    {
        rHASH_IN(i) = iterator[i];
    }
}


/**
 * @brief       clear rHASH_PCR_LEN.
 * @return      none
 */
void hash_clear_msg_len(void)
{
    volatile unsigned int flag = 0;

    rHASH_PCR_LEN(0) = flag;
    rHASH_PCR_LEN(1) = flag;
    rHASH_PCR_LEN(2) = flag;
    rHASH_PCR_LEN(3) = flag;
}



/**
 * @brief       set the total byte length of the whole message.
 * @param[in]   msg_total_bytes             - total byte length of the whole message.
 * @param[in]   words                       - word length of array msg_total_bytes.
 * @return      none
 */
void hash_set_msg_total_byte_len(unsigned int *msg_total_bytes, unsigned int words)
{
    while(words--)
    {
        rHASH_PCR_LEN(words) = msg_total_bytes[words];
    }
}


/**
 * @brief       set dma output bytes length.
 * @param[in]   bytes             - byte length of the written data for hash hardware.
 * @return      none
 */
void hash_set_dma_output_len(unsigned int bytes)
{
    rHASH_DMA_WLEN = bytes;
}


/**
 * @brief       start HASH iteration calc.
 * @return      none
 */
void hash_start(void)
{
    volatile unsigned int flag = 1;

    //while((rHASH_SR1 & flag) == 1)
    //{;}

    rHASH_SR2 |= flag;
    rHASH_CTRL |= flag;

}


/**
 * @brief       wait till done.
 * @return      none
 */
void hash_wait_till_done(void)
{
    volatile unsigned int flag = 1;

    while((rHASH_SR2 & flag) == 0)
    {;}

    rHASH_SR2 = flag;
}


/**
 * @brief       DMA wait till done.
 * @param[in]   callback             - callback function pointer.
 * @return      none
 */
void hash_dma_wait_till_done(HASH_CALLBACK callback)
{
    volatile unsigned int flag = 1;

    while((rHASH_SR2 & flag) == 0)
    {
        if(callback)
        {
            callback();
        }
        else
        {;}
    }

    rHASH_SR2 = flag;
}


/**
 * @brief       input message.
 * @param[in]   msg             - message.
 * @param[in]   msg_words       - word length of msg.
 * @return      none
 * @note
  @verbatim
      -# 1. if msg does not contain the last block, please make sure msg_words is a multiple of the
        hash block word length.
  @endverbatim
 */
void hash_input_msg(const unsigned char *msg, unsigned int msg_words)
{
    unsigned int tmp;
    unsigned int i;

    if(((unsigned int)msg) & 3)
    {
        for(i = 0; i < msg_words; i++)
        {
            memcpy_((unsigned char *)&tmp, msg, 4);
            rHASH_M_DIN(i) = tmp;
            msg += 4;
        }
    }
    else
    {
        for(i = 0; i < msg_words; i++)
        {
            rHASH_M_DIN(i) = *((const unsigned int *)msg);
            msg += 4;
        }
    }
}


#ifdef HASH_DMA_FUNCTION
/**
 * @brief       input message.
 * @param[in]   in             - message of some blocks, or message including the last byte(last block).
 * @param[out]  out            - hash digest or hmac.
 * @param[in]   inByteLen      - actual byte length of input msg.
 * @param[in]   callback       - callback function pointer.
 * @return      none
 * @note
  @verbatim
      -# 1. for DMA operation, the unit of input and output is 4 words, so, please make sure the buffer
        out is sufficient.
      -# 2. if just to input message, not to get digest or hmac, please set para out to be NULL and WLEN to be 0.
        if to get the digest or hmac, para out can not be NULL, and please set WLEN to be digest length.
  @endverbatim
 */
void hash_dma_operate(unsigned int *in, unsigned int *out, unsigned int inByteLen, HASH_CALLBACK callback)
{
    //src addr
    rHASH_DMA_SA = ((unsigned int)in)&0xFFFFFFFF;
    hash_tx_dma(hash_get_tx_dma_channel(), (unsigned int)in, inByteLen);
    //dst addr
    if(out)
    {
        rHASH_DMA_DA = ((unsigned int)out)&0xFFFFFFFF;
        hash_rx_dma(hash_get_rx_dma_channel(), (unsigned int)out, rHASH_DMA_WLEN);
    }
    else
    {;}

    //data byte length
    rHASH_DMA_RLEN = inByteLen;

    hash_start();

    hash_dma_wait_till_done(callback);
}

#endif

