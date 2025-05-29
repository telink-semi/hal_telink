/********************************************************************************************************
 * @file    rram.c
 *
 * @brief   This is the source file for tl322x
 *
 * @author  Driver Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#include "rram.h"
#include "lib/include/clock.h"


dma_config_t rram_tx_dma_config = {
    .dst_req_sel    = DMA_REQ_RRAM_TX,    // tx req
    .src_req_sel    = 0,
    .dst_addr_ctrl  = DMA_ADDR_FIX,
    .src_addr_ctrl  = DMA_ADDR_INCREMENT, // increment
    .dstmode        = DMA_HANDSHAKE_MODE, // handshake
    .srcmode        = DMA_NORMAL_MODE,
    .dstwidth       = DMA_CTR_WORD_WIDTH, // must be word
    .srcwidth       = DMA_CTR_WORD_WIDTH, // must be word
    .src_burst_size = 2,                  // 4 word
    .read_num_en    = 0,
    .priority       = 0,
    .write_num_en   = 0,
    .auto_en        = 0, // must be 0
};
dma_config_t rram_rx_dma_config = {
    .dst_req_sel    = 0,
    .src_req_sel    = DMA_REQ_RRAM_RX,
    .dst_addr_ctrl  = DMA_ADDR_INCREMENT,
    .src_addr_ctrl  = DMA_ADDR_FIX,
    .dstmode        = DMA_NORMAL_MODE,
    .srcmode        = DMA_HANDSHAKE_MODE,
    .dstwidth       = DMA_CTR_WORD_WIDTH, // must be word
    .srcwidth       = DMA_CTR_WORD_WIDTH, // must be word
    .src_burst_size = 2,                  // 4 word
    .read_num_en    = 0,
    .priority       = 0,
    .write_num_en   = 0,
    .auto_en        = 0, // must be 0
};

/**
 * @brief      This setting serves to set the configuration of stimer input capture DMA.
 */
dma_config_t rram_dma_config = {
    .dst_req_sel    = 0,
    .src_req_sel    = DMA_REQ_STIMER_RX,
    .dst_addr_ctrl  = DMA_ADDR_INCREMENT,
    .src_addr_ctrl  = DMA_ADDR_FIX,
    .dstmode        = DMA_NORMAL_MODE,
    .srcmode        = DMA_HANDSHAKE_MODE,
    .dstwidth       = DMA_CTR_WORD_WIDTH,
    .srcwidth       = DMA_CTR_WORD_WIDTH,
    .src_burst_size = DMA_BURST_1_WORD,
    .vacant_bit     = 0,
    .read_num_en    = 0,
    .priority       = 0,
    .write_num_en   = 0,
    .auto_en        = 0,
};

void rram_enter_otp(void)
{
    rram_reg_set_cmd(RRAM_ENTER_OTP_CMD);
    while (reg_rram_state & FLD_RRAM_CTRL_BUSY)
        ;
}

void rram_enter_mtp(void)
{
    rram_reg_set_cmd(RRAM_ENTER_MTP_CMD);
    while (reg_rram_state & FLD_RRAM_CTRL_BUSY)
        ;
}

void rram_write_enable(void)
{
    rram_reg_set_cmd(RRAM_WRITE_ENABLE_CMD);
    while (reg_rram_state & FLD_RRAM_CTRL_BUSY)
        ;
}

void rram_write_disable(void)
{
    rram_reg_set_cmd(RRAM_WRITE_DISENB_CMD);
    while (reg_rram_state & FLD_RRAM_CTRL_BUSY)
        ;
}

void rram_ahbread_word(unsigned int addr, unsigned int len, unsigned int *data)
{
    for (unsigned int i = 0; i < (len >> 2); i++) {
        data[i] = reg_rram_ahb_rd_data_word(addr + i * 4);
    }
}

void rram_ahbread_byte(unsigned int addr, unsigned int len, unsigned char *data)
{
    for (unsigned int i = 0; i < len; i++) {
        data[i] = reg_rram_ahb_rd_data_byte(addr + i);
    }
}

void rram_regread(unsigned int addr, unsigned int len, unsigned int *data)
{
    unsigned int len_dword = len >> 4;

    rram_reg_set_addr(addr >> 4);
    rram_reg_set_len(len_dword);
    rram_reg_set_cmd(RRAM_READ_DATA_CMD);
    for (unsigned int i = 0; i < len_dword; i++) {
        while ((reg_rram_state & FLD_RRAM_RX_READY) != FLD_RRAM_RX_READY)
            ;
        data[i * 4 + 0] = rram_reg_read_data();
        data[i * 4 + 1] = rram_reg_read_data();
        data[i * 4 + 2] = rram_reg_read_data();
        data[i * 4 + 3] = rram_reg_read_data();
    }
}

void rram_regwrite(unsigned int addr, unsigned int len, unsigned int *data)
{
    unsigned int len_dword = len >> 4;

    rram_reg_set_addr(addr >> 4);
    rram_reg_set_len(len_dword);
    rram_reg_set_cmd(RRAM_WRITE_DATA_CMD);
    for (unsigned int i = 0; i < len_dword; i++) {
        while ((reg_rram_state & FLD_RRAM_TX_READY) != FLD_RRAM_TX_READY)
            ;
        rram_reg_write_data(data[i * 4 + 0]);
        rram_reg_write_data(data[i * 4 + 1]);
        rram_reg_write_data(data[i * 4 + 2]);
        rram_reg_write_data(data[i * 4 + 3]);
    }
    while (reg_rram_state & FLD_RRAM_CTRL_BUSY)
        ;
}

void rram_otp_regwrite(unsigned int addr, unsigned int len, unsigned int *data)
{
    unsigned int len_dword = len >> 4;

    rram_reg_set_addr(addr >> 4);
    rram_reg_set_len(len_dword);
    rram_reg_set_cmd(RRAM_WRITE_DATA_CMD);
    for (unsigned int i = 0; i < len_dword; i++) {
        while ((reg_rram_state & FLD_RRAM_TX_READY) != FLD_RRAM_TX_READY)
            ;
        rram_reg_write_data(data[i * 4 + 0]);
        rram_reg_write_data(data[i * 4 + 1]);
        rram_reg_write_data(data[i * 4 + 2]);
        rram_reg_write_data(data[i * 4 + 3]);
    }
    while (reg_rram_state & FLD_RRAM_CTRL_BUSY)
        ;
    while (reg_rram_state & FLD_RRAM_RRAM_BUSY)
        ;
}

void rram_regwrite_abort_test(unsigned int addr, unsigned int len, unsigned int *data)
{
    unsigned int len_dword     = len >> 4;
    unsigned int len_dword_err = len_dword - 1;

    rram_reg_set_addr(addr >> 4);
    rram_reg_set_len(len_dword);
    rram_reg_set_cmd(RRAM_WRITE_DATA_CMD);
    for (unsigned int i = 0; i < len_dword_err; i++) {
        while ((reg_rram_state & FLD_RRAM_TX_READY) != FLD_RRAM_TX_READY)
            ;
        rram_reg_write_data(data[i * 4 + 0]);
        rram_reg_write_data(data[i * 4 + 1]);
        rram_reg_write_data(data[i * 4 + 2]);
        rram_reg_write_data(data[i * 4 + 3]);
    }
    while (reg_rram_state & FLD_RRAM_RRAM_BUSY)
        ;
}

unsigned short rram_read_reg(unsigned char cmd)
{
    unsigned short rdata_reg = 0;

    rram_reg_set_cmd(cmd);
    while ((reg_rram_state & FLD_RRAM_READREG_READY) != FLD_RRAM_READREG_READY)
        ;
    rdata_reg = rram_reg_read_reg();

    return rdata_reg;
}

void rram_write_reg(unsigned char cmd, unsigned short data)
{
    rram_reg_write_reg(data);
    rram_reg_set_cmd(cmd);
    while (reg_rram_state & FLD_RRAM_CTRL_BUSY)
        ;
    while (reg_rram_state & FLD_RRAM_RRAM_BUSY)
        ;
}

unsigned char rram_read_state_reg(void)
{
    unsigned char rdata_reg = 0;

    rram_reg_set_cmd(RRAM_READ_SREG_CMD);
    while ((reg_rram_state & FLD_RRAM_READREG_READY) != FLD_RRAM_READREG_READY)
        ;
    rdata_reg = rram_reg_read_reg0();

    return rdata_reg;
}

void rram_write_state_reg(unsigned char data)
{
    rram_reg_write_reg0(data);
    rram_reg_set_cmd(RRAM_WRITE_SREG_CMD);
    while (reg_rram_state & FLD_RRAM_CTRL_BUSY)
        ;
}

unsigned char rram_read_creg(unsigned char addr)
{
    unsigned char rdata_reg = 0;

    rram_reg_set_addr0(addr);
    rram_reg_set_cmd(RRAM_READ_CREG_CMD);
    while ((reg_rram_state & FLD_RRAM_READREG_READY) != FLD_RRAM_READREG_READY)
        ;
    rdata_reg = rram_reg_read_reg0();

    return rdata_reg;
}

void rram_write_creg(unsigned char addr, unsigned char data)
{
    rram_reg_set_addr0(addr);
    rram_reg_write_reg0(data);
    rram_reg_set_cmd(RRAM_WRITE_CREG_CMD);
    while (reg_rram_state & FLD_RRAM_CTRL_BUSY)
        ;
}

unsigned char rram_read_treg(unsigned char addr)
{
    unsigned char rdata_reg = 0;

    rram_reg_set_addr0(addr);
    rram_reg_set_cmd(RRAM_READ_TREG_CMD);
    while ((reg_rram_state & FLD_RRAM_READREG_READY) != FLD_RRAM_READREG_READY)
        ;
    rdata_reg = rram_reg_read_reg0();

    return rdata_reg;
}

void rram_write_treg(unsigned char addr, unsigned char data)
{
    rram_reg_set_addr0(addr);
    rram_reg_write_reg0(data);
    rram_reg_set_cmd(RRAM_WRITE_TREG_CMD);
    while (reg_rram_state & FLD_RRAM_CTRL_BUSY)
        ;
}

void rram_run_mbist(unsigned int *data)
{
    rram_reg_set_addr(0x0000);
    rram_reg_set_len(0x0001);
    rram_reg_set_cmd(RRAM_RUN_MBIST_CMD);
    rram_reg_write_data(data[0]);
    rram_reg_write_data(data[1]);
    rram_reg_write_data(data[2]);
    rram_reg_write_data(data[3]);
    while (reg_rram_state & FLD_RRAM_CTRL_BUSY)
        ;
}

void rram_spi_enter_otp(void)
{
    rram_spi_set_cfg(0x04);
    rram_spi_write_data(RRAM_ENTER_OTP_CMD);
    while (reg_rram_spi_st & BIT(1))
        ;
}

void rram_spi_enter_mtp(void)
{
    rram_spi_set_cfg(0x04);
    rram_spi_write_data(RRAM_ENTER_MTP_CMD);
    while (reg_rram_spi_st & BIT(1))
        ;
}

unsigned char rram_spi_read_state_reg(void)
{
    unsigned char rdata_reg = 0;

    rram_spi_set_cfg(0x08);
    rram_spi_write_data(RRAM_READ_SREG_CMD);
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;
    rdata_reg = reg_rram_spi_rdata;
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;
    rdata_reg = reg_rram_spi_rdata;

    return rdata_reg;
}

void rram_wait_done(void)
{
    int i;
    for (i = 0; i < 10000000; ++i) {
        if (!(rram_spi_read_state_reg() & 0x01)) {
            break;
        }
    }
}

void rram_spi_write_enable(void)
{
    rram_spi_set_cfg(0x04);
    rram_spi_write_data(RRAM_WRITE_ENABLE_CMD);
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;
    rram_wait_done();
}

void rram_spi_write_disable(void)
{
    rram_spi_set_cfg(0x04);
    rram_spi_write_data(RRAM_WRITE_DISENB_CMD);
    while (reg_rram_spi_st & BIT(1))
        ;
}

void rram_spiread(unsigned int addr, unsigned char len, unsigned char *data)
{
    //  rram_spi_set_cfg((len+4)<<2);
    //  rram_spi_write_data(RRAM_READ2_DATA_CMD);
    //  while((reg_rram_spi_cfg&BIT(7)) != BIT(7));
    //  rram_spi_write_data(addr>>12);
    //  while((reg_rram_spi_cfg&BIT(7)) != BIT(7));
    //  rram_spi_write_data(addr>>4);
    //  while((reg_rram_spi_cfg&BIT(7)) != BIT(7));
    //  rram_spi_write_data(addr<<4);
    //  while((reg_rram_spi_cfg&BIT(7)) != BIT(7));
    //  data[0] = rram_spi_read_data();
    //  for(unsigned int i = 0; i < len; i++)
    //  {
    //      while((reg_rram_spi_cfg&BIT(7)) != BIT(7));
    //      data[i] = rram_spi_read_data();
    //  }

    rram_spi_set_cfg((len + 4) << 2);
    rram_spi_write_data(RRAM_READ2_DATA_CMD);
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;
    rram_spi_write_data(addr >> 16);
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;
    rram_spi_write_data(addr >> 8);
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;
    rram_spi_write_data(addr & 0xff);
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;
    data[0] = rram_spi_read_data();
    for (unsigned int i = 0; i < len; i++) {
        while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
            ;
        data[i] = rram_spi_read_data();
    }
}

void rram_spiwrite(unsigned int addr, unsigned char len, unsigned char *data)
{
    //  rram_spi_set_cfg((len+4)<<2);
    //  rram_spi_write_data(RRAM_WRITE_DATA_CMD);
    //  while((reg_rram_spi_cfg&BIT(7)) != BIT(7));
    //  rram_spi_write_data(addr>>12);
    //  while((reg_rram_spi_cfg&BIT(7)) != BIT(7));
    //  rram_spi_write_data(addr>>4);
    //  while((reg_rram_spi_cfg&BIT(7)) != BIT(7));
    //  rram_spi_write_data(addr<<4);
    //  while((reg_rram_spi_cfg&BIT(7)) != BIT(7));
    //
    //  for(unsigned int i = 0; i < len; i++)
    //  {
    //      rram_spi_write_data(data[i]);
    //      while((reg_rram_spi_cfg&BIT(7)) != BIT(7));
    //  }
    //  rram_wait_done();

    rram_spi_set_cfg((len + 4) << 2);
    rram_spi_write_data(RRAM_WRITE_DATA_CMD);
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;
    rram_spi_write_data(addr >> 16);
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;
    rram_spi_write_data(addr >> 8);
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;
    rram_spi_write_data(addr);
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;

    for (unsigned int i = 0; i < len; i++) {
        rram_spi_write_data(data[i]);
        while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
            ;
    }
    rram_wait_done();
}

//creg 128byte, rram_spi_write_creg 16byte
void rram_spi_read_creg(unsigned char addr, unsigned char len, unsigned char *data)
{
    rram_spi_set_cfg((len + 2) << 2);
    rram_spi_write_data(RRAM_READ_CREG_CMD);
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;
    rram_spi_write_data(addr);
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;
    data[0] = rram_spi_read_data();
    for (unsigned int i = 0; i < len; i++) {
        while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
            ;
        data[i] = rram_spi_read_data();
    }
}

//creg 128byte, rram_spi_write_creg 16byte
void rram_spi_write_creg(unsigned char addr, unsigned char len, unsigned char *data)
{
    rram_spi_set_cfg((len + 2) << 2);
    rram_spi_write_data(RRAM_WRITE_CREG_CMD);
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;
    rram_spi_write_data(addr);
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;
    for (unsigned int i = 0; i < len; i++) {
        rram_spi_write_data(data[i]);
        while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
            ;
    }
}

//treg 256byte, rram_spi_write_treg 16byte
void rram_spi_read_treg(unsigned char addr, unsigned char len, unsigned char *data)
{
    rram_spi_set_cfg((len + 2) << 2);
    rram_spi_write_data(RRAM_READ_TREG_CMD);
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;
    rram_spi_write_data(addr);
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;
    data[0] = rram_spi_read_data();
    for (unsigned int i = 0; i < len; i++) {
        while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
            ;
        data[i] = rram_spi_read_data();
    }
}

//treg 256byte, rram_spi_write_treg 16byte
void rram_spi_write_treg(unsigned char addr, unsigned char len, unsigned char *data)
{
    rram_spi_set_cfg((len + 2) << 2);
    rram_spi_write_data(RRAM_WRITE_TREG_CMD);
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;
    rram_spi_write_data(addr);
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;
    for (unsigned int i = 0; i < len; i++) {
        rram_spi_write_data(data[i]);
        while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
            ;
    }
}

void rram_spi_run_mbist(unsigned char *data)
{
    rram_spi_set_cfg(0x50);
    rram_spi_write_data(RRAM_RUN_MBIST_CMD);
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;
    rram_spi_write_data(0x00);
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;
    rram_spi_write_data(0x00);
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;
    rram_spi_write_data(0x00);
    while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
        ;

    for (unsigned int i = 0; i < 16; i++) {
        rram_spi_write_data(data[i]);
        while ((reg_rram_spi_cfg & BIT(7)) != BIT(7))
            ;
    }
    rram_wait_done();
}

void rram_set_tx_dma_config(dma_chn_e chn)
{
    dma_config(chn, &rram_tx_dma_config);
}

void rram_set_rx_dma_config(dma_chn_e chn)
{
    dma_config(chn, &rram_rx_dma_config);
}

void rram_regwrite_dma(dma_chn_e chn, unsigned int addr, unsigned int len, unsigned int *src_data)
{
    rram_reg_set_addr(addr >> 4);
    rram_reg_set_len(len >> 4);
    rram_reg_set_cmd(RRAM_WRITE_DATA_CMD);

    dma_set_address(chn, (unsigned int)src_data, reg_rram_wr_rd_data_adr);
    dma_set_size(chn, len, DMA_WORD_WIDTH);
    dma_chn_en(chn);

    while (reg_rram_state & FLD_RRAM_CTRL_BUSY)
        ;
}

void rram_regread_dma(dma_chn_e chn, unsigned int addr, unsigned int len, unsigned int *dst_data)
{
    rram_reg_set_addr(addr >> 4);
    rram_reg_set_len(len >> 4);
    rram_reg_set_cmd(RRAM_READ_DATA_CMD);

    dma_set_address(chn, reg_rram_wr_rd_data_adr, (unsigned int)dst_data);
    dma_set_size(chn, len, DMA_WORD_WIDTH);
    dma_chn_en(chn);

    while (reg_rram_state & FLD_RRAM_CTRL_BUSY)
        ;
}
