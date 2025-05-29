/********************************************************************************************************
 * @file    hash_basic.c
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
#include <string.h>
#include "lib/include/hash/hash_basic.h"
#include "lib/include/crypto_common/utility.h"
#include "reg_include/hash_reg.h"
#include "driver.h"


#if 0
//hash register pointer
volatile static HASH_REG *  g_hash_reg = (HASH_REG *)HASH_BASE_ADDR;
#endif


static dma_chn_e hash_tx_dma_channel;
static dma_chn_e hash_rx_dma_channel;

/**
 * @brief       get hash tx dma channel.
 * @return      hash tx dma channel
 */
static dma_chn_e hash_get_tx_dma_channel(void)
{
    return hash_tx_dma_channel;
}

/**
 * @brief       get hash rx dma channel.
 * @return      hash rx dma channel
 */
static dma_chn_e hash_get_rx_dma_channel(void)
{
    return hash_rx_dma_channel;
}

/**
 * @brief       set hash tx dma channel.
 * @param[in]   chn  - hash tx dma channel.
 * @return      none
 */
void hash_set_tx_dma_channel(dma_chn_e chn)
{
    hash_tx_dma_channel = chn;
}

/**
 * @brief       set hash rx dma channel.
 * @param[in]   chn  - hash rx dma channel.
 * @return      none
 */
void hash_set_rx_dma_channel(dma_chn_e chn)
{
    hash_rx_dma_channel = chn;
}

/**
 * @brief       get HFE IP version.
 * @return      HFE IP version
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
    volatile unsigned int mask = ~(((unsigned int)1) << HASH_DMA_OFFSET);

    rHASH_CFG &= mask;
}

/**
 * @brief       set hash to be DMA mode.
 * @return      none
 */
void hash_set_dma_mode(void)
{
    volatile unsigned int flag = (((unsigned int)1) << HASH_DMA_OFFSET);

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
    volatile unsigned int flag = (((unsigned int)1) << HASH_INTERRUPTION_OFFSET);

    rHASH_CFG |= flag;
}

/**
 * @brief       disable hash interruption in CPU mode or DMA mode.
 * @return      none
 */
void hash_disable_interruption(void)
{
    volatile unsigned int mask = ~(((unsigned int)1) << HASH_INTERRUPTION_OFFSET);

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

    if (tag) //current block is the last one of the message
    {
        rHASH_CFG |= flag;
    } else   //current block is not the last one of the message
    {
        rHASH_CFG &= mask;
    }
}

/**
 * @brief       get current HASH iterator value.
 * @param[out]   iterator  - current hash iterator.
 * @param[in]   hash_iterator_words iterator word length.
 * @return      none
 */
void hash_get_iterator(unsigned char *iterator, unsigned int hash_iterator_words)
{
    unsigned int temp;
    unsigned int i;

    if (((unsigned int)iterator) & 3) //for the case that iterator is not aligned by word
    {
        for (i = 0; i < hash_iterator_words; i++) {
            temp = rHASH_OUT(i);
            memcpy_((unsigned char *)(&(iterator[(i << 2)])), (unsigned char *)(&temp), 4);
        }
    } else {
        for (i = 0; i < hash_iterator_words; i++) {
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

    for (i = 0; i < hash_iterator_words; i++) {
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

    rHASH_PCR_LEN(0U) = flag;
    rHASH_PCR_LEN(1U) = flag;
    rHASH_PCR_LEN(2U) = flag;
    rHASH_PCR_LEN(3U) = flag;
}

/**
 * @brief       set the total byte length of the whole message.
 * @param[in]   msg_total_bytes             - total byte length of the whole message.
 * @param[in]   words                       - word length of array msg_total_bytes.
 * @return      none
 */
void hash_set_msg_total_byte_len(unsigned int *msg_total_bytes, unsigned int words)
{
    while (words--) {
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
    MEM_VOLATILE unsigned int start_flag = 1U;
    MEM_VOLATILE unsigned int clean_flag = 0U;

    //while((rHASH_SR1 & flag) == 1)
    //{;}

    rHASH_SR2 |= clean_flag;
    rHASH_CTRL |= start_flag;
}

/**
 * @brief       wait till done.
 * @return      none
 */
void hash_wait_till_done(void)
{
    MEM_VOLATILE unsigned int finish_flag = 1U;
    MEM_VOLATILE unsigned int clean_flag  = 0U;

    while (0U == (rHASH_SR2 & finish_flag)) {
        ;
    }

    rHASH_SR2 = clean_flag;
}

/**
 * @brief       DMA wait till done.
 * @param[in]   callback             - callback function pointer.
 * @return      none
 */
void hash_dma_wait_till_done(HASH_CALLBACK callback)
{
    MEM_VOLATILE unsigned int finish_flag = 1U;
    MEM_VOLATILE unsigned int clean_flag  = 0U;

    while (0U == (rHASH_SR2 & finish_flag)) {
        if (NULL != callback) {
            callback();
        } else {
            ;
        }
    }

    rHASH_SR2 = clean_flag;
}

/**
 * @brief       input message(at most a block)
 * @param[in]   msg             - message.
 * @param[in]   msg_bytes       - byte length of msg, can not be greater than block bytes
 * @return      none
 * @note
  @verbatim
      -# 1. msg_bytes can not be greater than block bytes
  @endverbatim
 */
void hash_input_msg_u8(const unsigned char *msg, unsigned int msg_bytes)
{
    unsigned int msg_words       = msg_bytes / 4U;
    unsigned int remainder_bytes = msg_bytes & (3U);
    unsigned int tmp             = 0U;
    unsigned int i;

    if (0U != (((unsigned int)msg) & 3U)) {
        for (i = 0U; i < msg_words; i++) {
            memcpy_((unsigned char *)&tmp, msg, 4);
            rHASH_M_DIN(i) = tmp;
            msg            = &(msg[4]);
        }
    } else {
        for (i = 0U; i < msg_words; i++) {
            rHASH_M_DIN(i) = *((const unsigned int *)msg);
            msg            = &(msg[4]);
        }
    }

    if (0 != remainder_bytes) {
        tmp = 0U;
        memcpy_((unsigned char *)&tmp, msg, remainder_bytes);
        rHASH_M_DIN(i) = tmp;
    } else {
        ;
    }
}


#ifdef HASH_DMA_FUNCTION
/**
 * @brief       configure the destination address and data length for hash tx dma.
 * @param[in]   buf_addr         - destination address.
 * @param[in]   len              - data length.
 * @return      none
 * @note
 */
void hash_tx_dma(dma_chn_e chn, unsigned int buf_addr, unsigned int len)
{
    reg_dma_src_addr(chn) = buf_addr;
    reg_dma_dst_addr(chn) = reg_hash_fifo;
    dma_set_size(chn, len, DMA_WORD_WIDTH);
    dma_chn_en(chn);
}

/**
 * @brief       configure the receiving  address and data length for hash rx dma.
 * @param[in]   buf_addr         - receiving address.
 * @param[in]   len              - data length.
 * @return      none
 * @note
 */
void hash_rx_dma(dma_chn_e chn, unsigned int buf_addr, unsigned int len)
{
    reg_dma_src_addr(chn) = reg_hash_fifo;
    reg_dma_dst_addr(chn) = buf_addr;
    dma_set_size(chn, len, DMA_WORD_WIDTH);
    dma_chn_en(chn);
}

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
    rHASH_DMA_SA = ((unsigned int)in) & 0xFFFFFFFFU;
    hash_tx_dma(hash_get_tx_dma_channel(), (unsigned int)in, inByteLen);
    //dst addr
    if (NULL != out) {
        rHASH_DMA_DA = ((unsigned int)out) & 0xFFFFFFFFU;
        hash_rx_dma(hash_get_rx_dma_channel(), (unsigned int)out, rHASH_DMA_WLEN);
    } else {
        ;
    }

    //data byte length
    rHASH_DMA_RLEN = inByteLen;

    hash_start();

    hash_dma_wait_till_done(callback);
}

#endif
