/********************************************************************************************************
 * @file    ske_portable.c
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
#include "lib/include/ske/ske_portable.h"
#include "lib/include/ske/ske_basic.h"
#include "driver.h"

#ifdef SKE_LP_DMA_FUNCTION

static dma_chn_e ske_tx_dma_channel;
static dma_chn_e ske_rx_dma_channel;

/**
 * @brief       get ske tx dma channel.
 * @return      hash tx dma channel
 */
dma_chn_e ske_get_tx_dma_channel(void)
{
    return ske_tx_dma_channel;
}

/**
 * @brief       get ske rx dma channel.
 * @return      ske rx dma channel
 */
dma_chn_e ske_get_rx_dma_channel(void)
{
    return ske_rx_dma_channel;
}

/**
 * @brief       set ske tx dma channel.
 * @param[in]   chn  - ske tx dma channel.
 * @return      none
 */
void ske_set_tx_dma_channel(dma_chn_e chn)
{
    ske_tx_dma_channel = chn;
}

/**
 * @brief       set ske tx dma channel.
 * @param[in]   chn  - ske tx dma channel.
 * @return      none
 */
void ske_set_rx_dma_channel(dma_chn_e chn)
{
    ske_rx_dma_channel = chn;
}

/* Initialization configuration of ske tx dma channel */
static dma_config_t ske_tx_dma_config = {
    .dst_req_sel = DMA_REQ_SKE_TX, // 32
    .src_req_sel = 0,
    .dst_addr_ctrl = DMA_ADDR_FIX,       // 2
    .src_addr_ctrl = DMA_ADDR_INCREMENT, // 0
    .dstmode = DMA_HANDSHAKE_MODE,       // handshake
    .srcmode = DMA_NORMAL_MODE,
    .dstwidth = DMA_CTR_WORD_WIDTH,     // must word
    .srcwidth = DMA_CTR_WORD_WIDTH,     // must word
    .src_burst_size = DMA_BURST_1_WORD, // must 0
    .read_num_en = 0,
    .priority = 0,
    .write_num_en = 0,
    .auto_en = 0,
};

/* Initialization configuration of ske rx dma channel */
static dma_config_t ske_rx_dma_config = {
    .dst_req_sel = 0,
    .src_req_sel = DMA_REQ_SKE_RX, // rx req
    .dst_addr_ctrl = DMA_ADDR_INCREMENT,
    .src_addr_ctrl = DMA_ADDR_FIX,
    .dstmode = DMA_NORMAL_MODE,
    .srcmode = DMA_HANDSHAKE_MODE,
    .dstwidth = DMA_CTR_WORD_WIDTH,     // must word
    .srcwidth = DMA_CTR_WORD_WIDTH,     ////must word
    .src_burst_size = DMA_BURST_1_WORD, // master rx dma support burst1(0-1 word,1-2 word,2-4 word,3-8 word).
    .read_num_en = 0,
    .priority = 0,
    .write_num_en = 0,
    .auto_en = 0, // must 0
};

/**
 * @brief Sets the burst size for ske tx DMA transfers.
 * @note        Since the DMA burst length in TX can only be fixed to 1 word,
 *              the TX burst length inside SKE can only be configured to 1 word.
 * @return      none
 */
static void ske_set_tx_burst_size(void)
{
    /*(1-1 word,2-2 word,4-4 word,8-8 word)*/
    reg_ske_thres = (reg_ske_thres & (~BIT_RNG(0, 3))) | (1 << DMA_BURST_1_WORD);
}

/**
 * @brief Sets the burst size for ske rx DMA transfers.
 * @param[in]   burst_size - The burst size to be set.
 * @return      none
 */
static void ske_set_rx_burst_size(dma_burst_size_e burst_size)
{
    /*(1-1 word,2-2 word,4-4 word,8-8 word)*/
    reg_ske_thres = (reg_ske_thres & (~BIT_RNG(4, 7))) | ((1 << burst_size) << 4);
}

/**
 * @brief       configure the destination address and data length for ske tx dma.
 * @param[in]   chn              - dma channel.
 * @param[in]   buf_addr         - destination address.
 * @param[in]   len              - data length.
 * @return      none
 * @note
 */
void ske_tx_dma(dma_chn_e chn, unsigned int buf_addr, unsigned int len)
{
    reg_dma_src_addr(chn) = buf_addr;
    reg_dma_dst_addr(chn) = reg_ske_fifo;
    dma_set_size(chn, len, DMA_WORD_WIDTH);
    dma_chn_en(chn);
}

/**
 * @brief       configure the receiving  address and data length for ske rx dma.
 * @param[in]   chn              - dma channel.
 * @param[in]   buf_addr         - receiving address.
 * @param[in]   len              - data length.
 * @return      none
 * @note
 */
void ske_rx_dma(dma_chn_e chn, unsigned int buf_addr, unsigned int len)
{
    reg_dma_src_addr(chn) = reg_ske_fifo;
    reg_dma_dst_addr(chn) = buf_addr;
    dma_set_size(chn, len, DMA_WORD_WIDTH);
    dma_chn_en(chn);
}

/**
 * @brief Sets tx dma channel for ske DMA transfers.
 * @param[in]   tx_chn - The DMA channel to be used for transmit.
 * @return      none
 */
void ske_set_tx_dma_config(dma_chn_e tx_chn)
{
    dma_config(tx_chn, &ske_tx_dma_config);
    ske_set_tx_burst_size();
    ske_set_tx_dma_channel(tx_chn);
    reg_ske_dma_en |= 0x01;
}

/**
 * @brief Sets rx dma channel for ske DMA transfers.
 * @param[in]  rx_chn     - The DMA channel to be used for transmit.
 * @param[in]  burst_size - The burst size to be set.
 * @return      none
 */
void ske_set_rx_dma_config(dma_chn_e rx_chn, dma_burst_size_e burst_size)
{
    ske_rx_dma_config.src_burst_size = burst_size;
    dma_config(rx_chn, &ske_rx_dma_config);
    reg_dma_src_addr(rx_chn) = reg_ske_fifo;
    ske_set_rx_burst_size(burst_size);
    ske_set_rx_dma_channel(rx_chn);
    reg_ske_dma_en |= 0x02;
}
#endif

/**
 * @brief Initialize SKE-related generic configurations.
 * @note       Only after calling this function can other SKE related functions be called.
 *             Otherwise, other SKE function settings will not take effect.
 * @return None.
 */
void ske_dig_en(void)
{
    ske_reset();
    ske_clk_en();
}
