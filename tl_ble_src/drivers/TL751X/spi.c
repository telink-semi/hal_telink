/********************************************************************************************************
 * @file    spi.c
 *
 * @brief   This is the source file for TL751X
 *
 * @author  Driver Group
 * @date    2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#include "spi.h"
#include "clock.h"

static unsigned char s_gspi_tx_dma_chn;
static unsigned char s_gspi_master_rx_dma_chn;
static unsigned char s_gspi_slave_rx_dma_chn;
static unsigned char s_lspi_tx_dma_chn;
static unsigned char s_lspi_master_rx_dma_chn;
static unsigned char s_lspi_slave_rx_dma_chn;
dma_chain_config_t g_spi_rx_dma_list_cfg;
dma_chain_config_t g_spi_tx_dma_list_cfg;
dma_config_t gspi_tx_dma_config = {
    .dst_req_sel    = DMA_REQ_GSPI_TX,//tx req
    .src_req_sel    = 0,
    .dst_addr_ctrl  = DMA_ADDR_FIX,
    .src_addr_ctrl  = DMA_ADDR_INCREMENT,//increment
    .dstmode        = DMA_HANDSHAKE_MODE,//handshake
    .srcmode        = DMA_NORMAL_MODE,
    .dstwidth       = DMA_CTR_WORD_WIDTH,//must word
    .srcwidth       = DMA_CTR_WORD_WIDTH,//must word
    .src_burst_size = 0,//master tx dma support burst4/2/1(4/2/1 word).
    .read_num_en    = 0,
    .priority       = 0,
    .write_num_en   = 0,
    .auto_en        = 0,//must 0
};
dma_config_t gspi_master_rx_dma_config = {
    .dst_req_sel   = 0,
    .src_req_sel    = DMA_REQ_GSPI_RX,//rx req
    .dst_addr_ctrl  = DMA_ADDR_INCREMENT,
    .src_addr_ctrl  = DMA_ADDR_FIX,
    .dstmode        = DMA_NORMAL_MODE,
    .srcmode        = DMA_HANDSHAKE_MODE,
    .dstwidth       = DMA_CTR_WORD_WIDTH,//must word
    .srcwidth       = DMA_CTR_WORD_WIDTH,////must word
    .src_burst_size = 0,//master rx dma support burst2/1(2/1 word).
    .read_num_en    = 0,
    .priority       = 0,
    .write_num_en   = 0,
    .auto_en        = 0,//must 0
};
dma_config_t gspi_slave_rx_dma_config = {
    .dst_req_sel   = 0,
    .src_req_sel    = DMA_REQ_GSPI_RX,//rx req
    .dst_addr_ctrl  = DMA_ADDR_INCREMENT,
    .src_addr_ctrl  = DMA_ADDR_FIX,
    .dstmode        = DMA_NORMAL_MODE,
    .srcmode        = DMA_HANDSHAKE_MODE,
    .dstwidth       = DMA_CTR_WORD_WIDTH,//must word
    .srcwidth       = DMA_CTR_WORD_WIDTH,////must word
    .src_burst_size = 0,//slave not support burst.
    .read_num_en    = 0,
    .priority       = 0,
    .write_num_en   = 1,//When write_num_en is set to 1, dma will write back the length information at the address 4 bytes before the destination address.
    .auto_en        = 0,//must 0
};
dma_config_t lspi_tx_dma_config = {
    .dst_req_sel    = DMA_REQ_LSPI_TX,//tx req
    .src_req_sel    = 0,
    .dst_addr_ctrl  = DMA_ADDR_FIX,
    .src_addr_ctrl  = DMA_ADDR_INCREMENT,//increment
    .dstmode        = DMA_HANDSHAKE_MODE,//handshake
    .srcmode        = DMA_NORMAL_MODE,
    .dstwidth       = DMA_CTR_WORD_WIDTH,//must word
    .srcwidth       = DMA_CTR_WORD_WIDTH,//must word
    .src_burst_size = 0,//master tx dma support burst4/2/1(4/2/1 word).
    .read_num_en    = 0,
    .priority       = 0,
    .write_num_en   = 0,
    .auto_en        = 0,//must 0
};

dma_config_t lspi_master_rx_dma_config = {
    .dst_req_sel    = 0,
    .src_req_sel    = DMA_REQ_LSPI_RX,//rx req
    .dst_addr_ctrl  = DMA_ADDR_INCREMENT,
    .src_addr_ctrl  = DMA_ADDR_FIX,
    .dstmode        = DMA_NORMAL_MODE,
    .srcmode        = DMA_HANDSHAKE_MODE,
    .dstwidth       = DMA_CTR_WORD_WIDTH,//must word
    .srcwidth       = DMA_CTR_WORD_WIDTH,////must word
    .src_burst_size = 0,//master rx dma support burst2/1(2/1 word).
    .read_num_en    = 0,
    .priority       = 0,
    .write_num_en   = 0,
    .auto_en        = 0,//must 0
};
dma_config_t lspi_slave_rx_dma_config = {
    .dst_req_sel    = 0,
    .src_req_sel    = DMA_REQ_LSPI_RX,//rx req
    .dst_addr_ctrl  = DMA_ADDR_INCREMENT,
    .src_addr_ctrl  = DMA_ADDR_FIX,
    .dstmode        = DMA_NORMAL_MODE,
    .srcmode        = DMA_HANDSHAKE_MODE,
    .dstwidth       = DMA_CTR_WORD_WIDTH,//must word
    .srcwidth       = DMA_CTR_WORD_WIDTH,////must word
    .src_burst_size = 0,//slave not support burst.
    .read_num_en    = 0,
    .priority       = 0,
    .write_num_en   = 1,//When write_num_en is set to 1, dma will write back the length information at the address 4 bytes before the destination address
    .auto_en        = 0,//must 0
};

/**
 * @brief      This function clock and working mode for SPI interface.
 * @param[in]  spi_sel          - the spi module.
 * @param[in]  src              - the spi clock source.
 * @param[in]  div_clock        - the division factor for SPI module.
 * @return     none
 */
void spi_clock_clk_config(spi_sel_e spi_sel,sys_clock_src_e src,unsigned short div_clock)
{
    switch (spi_sel)
     {
       case LSPI_MODULE:
           reg_rst0 |= FLD_RST0_LSPI;
           reg_clk_en0 |= FLD_CLK0_LSPI_EN;
           reg_lspi_clk_set = (reg_lspi_clk_set & (~FLD_LSPI_CLK_MOD))| src | div_clock;
           break;
       case GSPI_MODULE:
           reg_rst1 |= FLD_RST1_GSPI;
           reg_clk_en1 |= FLD_CLK1_GSPI_EN;
           reg_gspi_clk_set = (reg_gspi_clk_set & (~FLD_GSPI_CLK_MOD))| ((src>>4)<<8 )| div_clock;
           break;
       default:break;
    }
}

/**
 * @brief      This function selects  pin  for gspi master or slave mode.
 * @param[in]  pin  - the selected pin.
 * @return     none
 */
void gspi_set_pin_mux(gspi_pin_def_e pin)
{
    if (pin != GSPI_NONE_PIN)
    {
        /**
         * When configuring the mux pin of SPI, pull up the CSN configuration.
         * The chip SPI defaults to the master, and when the slave configures the pin as CSN, since the CSN is floating and in the input state,
         * an end interrupt may be generated by external influences.
         * Added by minghai
         */
        if((pin == GSPI_CSN0_PA0_PIN) | (pin == GSPI_CSN0_PB7_PIN) | (pin == GSPI_CSN0_PG1_PIN) | (pin == GSPI_CSN0_PJ2_PIN) |
                (pin == GSPI_CN1_PB3_PIN) | (pin == GSPI_CN1_PF5_PIN) |
                (pin == GSPI_CN2_PB4_PIN) | (pin == GSPI_CN2_PF6_PIN) |
                (pin == GSPI_CN3_PB6_PIN) | (pin == GSPI_CN3_PG0_PIN)){
            gpio_set_up_down_res((gpio_pin_e)pin, GPIO_PIN_PULLUP_10K);
        }
        gpio_input_en((gpio_pin_e)pin);
        reg_gpio_func_mux(pin) = 3;
        gpio_function_dis((gpio_pin_e)pin);
    }
}
/**
 * @brief       This function enable gspi csn pin.
 * @param[in]   pin - the csn pin.
 * @return      none
 */
void gspi_cs_pin_en(gspi_pin_def_e pin)
{
    gspi_set_pin_mux(pin);
}

/**
 * @brief       This function disable gspi csn pin.
 * @param[in]   pin - the csn pin.
 * @return      none
 */
void gspi_cs_pin_dis(gspi_pin_def_e pin)
{
    gpio_output_en((gpio_pin_e)pin);
    gpio_input_dis((gpio_pin_e)pin);
    gpio_set_high_level((gpio_pin_e)pin);
    gpio_function_en((gpio_pin_e)pin);//gpio_function_en must be set at the end
}

/**
 * @brief       This function change gspi csn pin.
 * @param[in]   current_csn_pin - the current csn pin.
 * @param[in]   next_csn_pin - the next csn pin.
 * @return      none.
 */
void gspi_change_csn_pin(gspi_pin_def_e current_csn_pin,gspi_pin_def_e next_csn_pin)
{
    gspi_cs_pin_dis(current_csn_pin);
    gspi_cs_pin_en(next_csn_pin);
}

/**
 * @brief       This function servers to set gspi pin.
 * @param[in]   gspi_pin_config - the pointer of pin config struct.
 * @return      none
 */
void gspi_set_pin(gspi_pin_config_t *spi_pin_config)
{
    gspi_set_pin_mux(spi_pin_config->spi_clk_pin);
    gspi_set_pin_mux(spi_pin_config->spi_csn_pin);
    gspi_set_pin_mux(spi_pin_config->spi_mosi_io0_pin);
    gspi_set_pin_mux(spi_pin_config->spi_miso_io1_pin);
    gspi_set_pin_mux(spi_pin_config->spi_io2_pin);
    gspi_set_pin_mux(spi_pin_config->spi_io3_pin);
    gspi_set_pin_mux(spi_pin_config->spi_io4_pin);
    gspi_set_pin_mux(spi_pin_config->spi_io5_pin);
    gspi_set_pin_mux(spi_pin_config->spi_io6_pin);
    gspi_set_pin_mux(spi_pin_config->spi_io7_pin);
    gspi_set_pin_mux(spi_pin_config->spi_dm_pin);
}

/**
 * @brief      This function selects  pin  for lspi master or slave mode.
 * @param[in]  pin  - the selected pin.
 * @return     none
 */
void lspi_set_pin_mux(lspi_pin_def_e pin)
{
    if (pin != 0)
    {
        unsigned char val = 0;
        /**
         * When configuring the mux pin of SPI, pull up the CSN configuration.
         * The chip SPI defaults to the master, and when the slave configures the pin as CSN, since the CSN is floating and in the input state,
         * an end interrupt may be generated by external influences.
         * Added by minghai
         */
        if((pin == LSPI_CSN_PA3_PIN) | (pin == LSPI_CSN_PB0_PIN) | (pin == LSPI_CSN_PB4_PIN) | (pin == LSPI_CSN_PD2_PIN) | (pin == LSPI_CSN_PD6_PIN) |
          (pin == LSPI_CSN_PF4_PIN) | (pin == LSPI_CSN_PG0_PIN) | (pin == LSPI_CSN_PG4_PIN) | (pin == LSPI_CSN_PH4_PIN) | (pin == LSPI_CSN_PJ5_PIN))
        {
            gpio_set_up_down_res((gpio_pin_e)pin, GPIO_PIN_PULLUP_10K);
            val = 16;
        }
        gpio_input_en((gpio_pin_e)pin);
        reg_gpio_func_mux(pin) = val;
        gpio_function_dis((gpio_pin_e)pin);
    }
}

/**
 * @brief       This function servers to set lspi pin.
 * @param[in]   spi_pin_config - the pointer of pin config struct.
 * @return      none
 */
void lspi_set_pin(lspi_pin_config_t *spi_pin_config)
{
    lspi_set_pin_mux(spi_pin_config->spi_clk_pin);
    lspi_set_pin_mux(spi_pin_config->spi_csn_pin);
    lspi_set_pin_mux(spi_pin_config->spi_mosi_io0_pin);
    lspi_set_pin_mux(spi_pin_config->spi_miso_io1_pin);
    lspi_set_pin_mux(spi_pin_config->spi_io2_pin);
    lspi_set_pin_mux(spi_pin_config->spi_io3_pin);
    lspi_set_pin_mux(spi_pin_config->spi_io4_pin);
    lspi_set_pin_mux(spi_pin_config->spi_io5_pin);
    lspi_set_pin_mux(spi_pin_config->spi_io6_pin);
    lspi_set_pin_mux(spi_pin_config->spi_io7_pin);
    lspi_set_pin_mux(spi_pin_config->spi_dm_pin);
}

/**
 * @brief      This function selects  pin  for sspi .
 * @param[in]  pin  - the selected pin.
 * @return     none
 */
void sspi_set_pin_mux(sspi_pin_def_e pin)
{
    if (pin != 0)
    {
        if((pin == SSPI_CSN_PA0_PIN) | (pin == SSPI_CSN_PA4_PIN) | (pin == SSPI_CSN_PB1_PIN) |
           (pin == SSPI_CSN_PB5_PIN) | (pin == SSPI_CSN_PD3_PIN) | (pin == SSPI_CSN_PD7_PIN) |
           (pin == SSPI_CSN_PF5_PIN) | (pin == SSPI_CSN_PG1_PIN) | (pin == SSPI_CSN_PG5_PIN) |
           (pin == SSPI_CSN_PH5_PIN) | (pin == SSPI_CSN_PJ2_PIN))
        {
            gpio_set_up_down_res((gpio_pin_e)pin, GPIO_PIN_PULLUP_10K);
        }
        gpio_input_en((gpio_pin_e)pin);
        reg_gpio_func_mux(pin) = 14;
        gpio_function_dis((gpio_pin_e)pin);
    }
}

/**
 * @brief   This function servers to set sspi pin.
 * @param[in]   spi_pin_config - the pointer of pin config struct.
 * @return  none
 */
void spi_slave_set_pin(sspi_pin_config_t *spi_pin_config)
{
    sspi_set_pin_mux(spi_pin_config->spi_clk_pin);
    sspi_set_pin_mux(spi_pin_config->spi_csn_pin);
    sspi_set_pin_mux(spi_pin_config->spi_mosi_io0_pin);
    sspi_set_pin_mux(spi_pin_config->spi_miso_io1_pin);
}

/**
 * @brief       This function configures the clock and working mode for SPI interface.
 * @param[in]   spi_sel     - the spi module.
 * @param[in]   div_clock   - the division factor for SPI module.
 *              spi_clock_out = pll_clk / div_clock
 * @param[in]    mode       - the selected working mode of SPI module.
 *               bit2: CPHA-SPI_CLK Phase,bit3: CPOL-SPI_CLK Polarity
 *              MODE0:  CPOL = 0 , CPHA = 0;
 *              MODE1:  CPOL = 0 , CPHA = 1;
 *              MODE2:  CPOL = 1 , CPHA = 0;
 *              MODE3:  CPOL = 1 , CPHA = 1;
 * @return      none
 */
void spi_master_init(spi_sel_e spi_sel,unsigned short div_clock, spi_mode_type_e mode)
{

    spi_clock_clk_config(spi_sel,XTAL_48M,div_clock);
    reg_spi_ctrl3(spi_sel)  |= (FLD_SPI_MASTER_MODE|FLD_SPI_DMATX_SOF_CLRTXF_EN|FLD_SPI_DMARX_EOF_CLRRXF_EN|FLD_SPI_AUTO_HREADY_EN);//master
    reg_spi_ctrl3(spi_sel) = ((reg_spi_ctrl3(spi_sel) & (~FLD_SPI_WORK_MODE)) | (mode << 2));// select SPI mode, support four modes
    spi_rx_tx_irq_trig_cnt(spi_sel);
    spi_xip_dis(spi_sel);
}

/**
 * @brief       This function configures the clock and working mode for SPI interface.
 * @param[in]   spi_sel - the spi module.
 * @param[in]   mode    - the selected working mode of SPI module.
 *               bit2: CPHA-SPI_CLK Phase,bit3: CPOL-SPI_CLK Polarity
 *              MODE0:  CPOL = 0 , CPHA = 0;
 *              MODE1:  CPOL = 0 , CPHA = 1;
 *              MODE2:  CPOL = 1 , CPHA = 0;
 *              MODE3:  CPOL = 1 , CPHA = 1;
 * @return      none
 * @note        spi_clock_in  <= slave'hclk/6
 */
void spi_slave_init(spi_sel_e spi_sel,spi_mode_type_e mode)
{
    spi_clock_clk_config(spi_sel,XTAL_48M,1);
    reg_spi_ctrl3(spi_sel)  &= (~FLD_SPI_MASTER_MODE);//slave
    reg_spi_ctrl3(spi_sel)  |= (FLD_SPI_DMATX_SOF_CLRTXF_EN|FLD_SPI_DMARX_EOF_CLRRXF_EN|FLD_SPI_AUTO_HREADY_EN);//slave
    reg_spi_ctrl3(spi_sel) = ((reg_spi_ctrl3(spi_sel) & (~FLD_SPI_WORK_MODE)) | (mode << 2));// select SPI mode, support four modes
    spi_rx_irq_trig_cnt(spi_sel,4);
    spi_tx_irq_trig_cnt(spi_sel,4);
    spi_xip_dis(spi_sel);
}

/**
 * @brief       This function servers to set the number of bytes to triggered the receive and transmit interrupt.
 * @param[in]   spi_sel - the spi module.
 * @return      none
 */
void spi_rx_tx_irq_trig_cnt(spi_sel_e spi_sel)
{
    //lspi and gspi with rxfifo deepth = 16 bytes,rx dma support burst 2/1.
    spi_rx_irq_trig_cnt(spi_sel,12);
    //lspi and gspi with txfifo deepth = 32 bytes,tx dma support burst 4/2/1.
    spi_tx_irq_trig_cnt(spi_sel,4);
}

/**
 * @brief       This function servers to set dummy cycle cnt.
 * @param[in]   spi_sel     - the spi module.
 * @param[in]   dummy_cnt   - the cnt of dummy clock.
 * @return      none
 */
void spi_set_dummy_cnt(spi_sel_e spi_sel, unsigned short dummy_cnt)
{
    reg_spi_ctrl2(spi_sel) = ((reg_spi_ctrl2(spi_sel) & (~FLD_SPI_DUMMY_CNT)) | ((dummy_cnt - 1) & FLD_SPI_DUMMY_CNT));
    reg_spi_ctrl4(spi_sel) = ((reg_spi_ctrl4(spi_sel) & (~FLD_SPI_DUMMY_CNT_ADD)) | ((dummy_cnt - 1) & FLD_SPI_DUMMY_CNT_ADD));
}

/**
 * @brief       This function servers to set spi transfer mode.
 * @param[in]   spi_sel - the spi module.
 * @param[in]   mode    - transfer mode.
 * @return      none
 */
void spi_set_transmode(spi_sel_e spi_sel, spi_tans_mode_e mode)
{
    reg_spi_ctrl2(spi_sel) = ((reg_spi_ctrl2(spi_sel) & (~FLD_SPI_TRANSMODE)) | ((mode & 0xf) << 4));
}

/**
 * @brief       This function servers to set spi io  mode.
 * @param[in]   spi_sel - the spi module.
 * @param[in]   mode    - single/dual/quad /3line.
 * @return      none
  */
void spi_set_io_mode(spi_sel_e spi_sel, spi_io_mode_e mode)
{
    if(mode == SPI_3_LINE_MODE)
    {
        /*The io mode must set to single mode*/
        spi_3line_mode_en(spi_sel);
        reg_spi_ctrl1(spi_sel) = ((reg_spi_ctrl1(spi_sel) & (~FLD_SPI_DATA_LANE)));
    }
    else
    {
        /*must disable 3line mode*/
        reg_spi_ctrl1(spi_sel) = ((reg_spi_ctrl1(spi_sel) & (~FLD_SPI_DATA_LANE))  | (mode & 0x3));
        spi_3line_mode_dis(spi_sel);
    }
}

/**
 * @brief       This function servers to config normal mode.
 * @param[in]   spi_sel - the spi module.
 * @param[in]   mode    - nomal ,mode 3line.
 * @return      none
 */
void spi_master_config(spi_sel_e spi_sel, spi_normal_3line_mode_e mode)
{
    spi_cmd_dis(spi_sel);
    spi_addr_dis(spi_sel);
    spi_set_io_mode(spi_sel, (spi_io_mode_e)mode);
}

/**
 * @brief       This function servers to config spi special mode.
 * @param[in]   spi_sel     - the spi module.
 * @param[in]   config  - the pointer of pin special config struct.
 * @return      none
 */
void spi_master_config_plus(spi_sel_e spi_sel,spi_wr_rd_config_t *config)
{
    spi_set_io_mode(spi_sel, config->spi_io_mode);
    spi_set_addr_len(spi_sel, config->spi_addr_len);
    spi_set_dummy_cnt(spi_sel, config->spi_dummy_cnt);
    spi_set_token_val(spi_sel, config->spi_token_val_sel);

    if (1 == config->spi_cmd_en)
    {
        spi_cmd_en(spi_sel);
    }
    else if (0 == config->spi_cmd_en)
    {
        spi_cmd_dis(spi_sel);
    }

    if (1 == config->spi_cmd_fmt_en)
    {
        spi_cmd_fmt_en(spi_sel);
    }
    else if (0 == config->spi_cmd_fmt_en)
    {
       spi_cmd_fmt_dis(spi_sel);
    }

    if (1 == config->spi_addr_en)
    {
        spi_addr_en(spi_sel);
    }
    else if (0 == config->spi_addr_en)
    {
        spi_addr_dis(spi_sel);
    }

    if (1 == config->spi_addr_fmt_en)
    {
        spi_addr_fmt_en(spi_sel);
    }
    else if (0 == config->spi_addr_fmt_en)
    {
        spi_addr_fmt_dis(spi_sel);
    }

    if (1 == config->spi_cmd1_en)
    {
        spi_cmd1_en(spi_sel);
    }
    else if (0 == config->spi_cmd1_en)
    {
        spi_cmd1_dis(spi_sel);
    }

    if (1 == config->spi_token_en)
    {
        spi_token_en(spi_sel);
    }
    else if (0 == config->spi_token_en)
    {
        spi_token_dis(spi_sel);
    }
    if (1 == config->spi_ddr_mode)
    {
        spi_ddr_en(spi_sel);
    }
    else if (0 == config->spi_ddr_mode)
    {
        spi_ddr_dis(spi_sel);
    }
}

/**
 * @brief       This function servers to send command by spi.
 * @param[in]   spi_sel - the spi module.
 * @param[in]   cmd - command.
 * @return      none
 */
void spi_master_send_cmd(spi_sel_e spi_sel, unsigned short cmd)
{
    spi_tx_fifo_clr(spi_sel);
    spi_set_transmode(spi_sel,SPI_MODE_NONE_DATA);//nodata
    //It is valid only when cmd1_en is enabled.
    spi_set_cmd1(spi_sel, ((cmd >> 8) & 0xff));//The cmd1 must be configured first.
    spi_set_cmd(spi_sel, (cmd & 0xff));
    while (spi_is_busy(spi_sel));
}

/**
 * @brief       This function servers to write spi fifo.
 * tx_fifo_depth are fixed sizes.
 * lspi with txfifo deepth = 20 bytes
 * gspi with txfifo deepth = 8 bytes
 * @param[in]   spi_sel - the spi module.
 * @param[in]   data    - the pointer to the data for write.
 * @param[in]   len     - write length.
 * @return      none
 */
void spi_write(spi_sel_e spi_sel, unsigned char *data, unsigned int len)
{
    unsigned char word_len = len >> 2;
    unsigned char single_len = len & 3;
    unsigned char tx_fifo_depth = 0;
    switch (spi_sel)
    {
        case LSPI_MODULE:
            tx_fifo_depth = 20;
            break;
        case GSPI_MODULE:
            tx_fifo_depth = 8;
            break;
    }
    //When the remaining size in tx_fifo is not less than 4 bytes, the MCU moves the data according to the word length.
    for (unsigned int i = 0; i < word_len; i++)
    {
        while (tx_fifo_depth - (reg_spi_txfifo_status(spi_sel) & FLD_SPI_TXF_ENTRIES) < 4);
        reg_spi_wr_rd_data_word(spi_sel) = ((unsigned int *)data)[i];
    }
    //When the remaining size in tx_fifo is less than 4 bytes, the MCU moves the data according to the word length.
    for (unsigned int i = 0; i < single_len; i++)
    {
        while (reg_spi_txfifo_status(spi_sel) & FLD_SPI_TXF_FULL);
        reg_spi_wr_rd_data(spi_sel,i % 4) = data[(word_len*4) + i];
    }
}

/**
 * @brief       This function servers to read spi fifo.
 * rx_fifo_depth are fixed sizes.
 * lspi with rxfifo deepth = 12 bytes
 * gspi with rxfifo deepth = 8 bytes
 * @param[in]   spi_sel - the spi module.
 * @param[in]   data    - the pointer to the data for read.
 * @param[in]   len     - write length.
 * @return      none
 */
void spi_read(spi_sel_e spi_sel, unsigned char *data, unsigned int len)
{
    unsigned char word_len = len >> 2;
    unsigned char single_len = len & 3;
    //When the data size in rx_fifo is not less than 4 bytes, the MCU moves the data according to the word length
    for (unsigned int i = 0; i < word_len; i++)
    {
        while ((reg_spi_rxfifo_status(spi_sel)  & FLD_SPI_RXF_ENTRIES) < 4);
        ((unsigned int *)data)[i] = reg_spi_wr_rd_data_word(spi_sel) ;
    }
    //When the data size in rx_fifo is less than 4 bytes, the MCU moves the data according to the word length
    for (unsigned char i = 0; i < single_len; i++)
    {
        while (reg_spi_rxfifo_status(spi_sel)  & FLD_SPI_RXF_EMPTY);
        data[(word_len*4) + i] = reg_spi_wr_rd_data((spi_sel), i % 4);
    }

}

/**
 * @brief       This function serves to normal write data in normal.
 * @param[in]   spi_sel - the spi module.
 * @param[in]   data    - the pointer to the data for write.
 * @param[in]   len     - write length.
 * @return      none
 */
void spi_master_write(spi_sel_e spi_sel, unsigned char *data, unsigned int len)
{
    spi_tx_dma_dis(spi_sel);
    spi_tx_fifo_clr(spi_sel);
    spi_tx_cnt(spi_sel,len);
    spi_set_transmode(spi_sel,SPI_MODE_WRITE_ONLY);
    spi_set_cmd(spi_sel,0x00);//when  cmd  disable that  will not sent cmd,just trigger spi send .
    spi_write(spi_sel,(unsigned char *)data, len);
    while (spi_is_busy(spi_sel));
}

/**
 * @brief       This function serves to normal write and read data.
 * This interface cannot be used for full duplex.
 * rd_len shouldn't set to 0. Must write first, then read.
 * @param[in]   spi_sel - the spi module.
 * @param[in]   wr_data - the pointer to the data for write.
 * @param[in]   wr_len  - write length.
 * @param[in]   rd_data - the pointer to the data for read.
 * @param[in]   rd_len  - read length.
 * @return      none
 */
void spi_master_write_read(spi_sel_e spi_sel, unsigned char *wr_data, unsigned int wr_len, unsigned char *rd_data, unsigned int rd_len)
{
    spi_tx_dma_dis(spi_sel);
    spi_rx_dma_dis(spi_sel);
    spi_tx_fifo_clr(spi_sel);
    spi_rx_fifo_clr(spi_sel);
    spi_tx_cnt(spi_sel,wr_len);
    spi_rx_cnt(spi_sel,rd_len);
    spi_set_transmode(spi_sel,SPI_MODE_WRITE_READ);
    spi_set_cmd(spi_sel,0x00);//when  cmd  disable that  will not sent cmd,just trigger spi send .
    spi_write(spi_sel,(unsigned char *)wr_data, wr_len);
    spi_read(spi_sel, (unsigned char *)rd_data, rd_len);
    while (spi_is_busy(spi_sel));
}

/**
 * @brief       This function serves to single/dual/quad write to the SPI slave.
 * @param[in]   spi_sel     - the spi module.
 * @param[in]   cmd         - cmd one byte will first write.
 * @param[in]   addr        - the address of slave.
 * @param[in]   data        -  pointer to the data need to write.
 * @param[in]   data_len    - length in byte of the data need to write.
 * @param[in]   wr_mode     - write mode.dummy or not dummy.
 * @return      none
 */
void spi_master_write_plus(spi_sel_e spi_sel, unsigned short cmd, unsigned int addr, unsigned char *data, unsigned int data_len, spi_wr_tans_mode_e wr_mode)
{
    spi_tx_dma_dis(spi_sel);
    spi_tx_fifo_clr(spi_sel);   
    spi_set_address(spi_sel, addr);
    spi_set_transmode(spi_sel, (spi_tans_mode_e)wr_mode);
    spi_tx_cnt(spi_sel, data_len);
    //It is valid only when cmd1_en is enabled.
    spi_set_cmd1(spi_sel, ((cmd >> 8) & 0xff));//The cmd1 must be configured first.
    spi_set_cmd(spi_sel, (cmd & 0xff));
    spi_write(spi_sel, (unsigned char *)data, data_len);
    while (spi_is_busy(spi_sel));
}
/**
 * @brief       This function serves to normal write data repeatedly.
 * @param[in]   spi_sel     - the spi module.
 * @param[in]   data        - the pointer to the data for write.
 * @param[in]   len         - write length.
 * @param[in]   repeat_time - number of times to write data repeatedly.
 * @return      none
 */
void spi_master_write_repeat(spi_sel_e spi_sel, unsigned char *data, unsigned int len, unsigned int repeat_time)
{
    unsigned int i = 0, j = 0, k = 0;
    spi_tx_dma_dis(spi_sel);
    spi_tx_fifo_clr(spi_sel);
    spi_tx_cnt(spi_sel, len*repeat_time);
    spi_set_transmode(spi_sel, SPI_MODE_WRITE_ONLY);
    spi_set_cmd(spi_sel, 0x00);//when  cmd  disable that  will not sent cmd,just trigger spi send .
    for (i = 0; i < repeat_time; i++)
    {
        for (j = 0; j < len; j++,k++)
        {
            while (reg_spi_txfifo_status(spi_sel) & FLD_SPI_TXF_FULL);
            reg_spi_wr_rd_data(spi_sel, k % 4) = data[j];
        }
    }
    while (spi_is_busy(spi_sel));
}
/**
 * @brief       This function serves to single/dual/quad write data to the SPI slave repeatedly.
 * @param[in]   spi_sel     - the spi module.
 * @param[in]   cmd         - cmd one byte will first write.
 * @param[in]   addr        - the address of slave.
 * @param[in]   data        - pointer to the data need to write.
 * @param[in]   data_len    - length in byte of the data need to write.
 * @param[in]   wr_mode     - write mode.dummy or not dummy.
 * @param[in]   repeat_time - number of times to write data repeatedly.
 * @return      none
 * @attention   Only data would be written repeatedly. the typical sending order is cmd + address + data * repeat_time,
 *              cmd and address would not be repeated.
 */
void spi_master_write_repeat_plus(spi_sel_e spi_sel, unsigned short cmd, unsigned int addr, unsigned char *data, unsigned int data_len, unsigned int repeat_time, spi_wr_tans_mode_e wr_mode)
{
    unsigned int i = 0, j = 0, k = 0;
    spi_tx_dma_dis(spi_sel);
    spi_tx_fifo_clr(spi_sel);
    spi_set_address(spi_sel, addr);
    spi_tx_cnt(spi_sel, data_len*repeat_time);
    spi_set_transmode(spi_sel, (spi_tans_mode_e)wr_mode);
    //It is valid only when cmd1_en is enabled.
    spi_set_cmd1(spi_sel, ((cmd >> 8) & 0xff));//The cmd1 must be configured first.
    spi_set_cmd(spi_sel, (cmd & 0xff));
    for (i = 0; i < repeat_time; i++)
    {
        for (j = 0; j < data_len; j++,k++)
        {
            while (reg_spi_txfifo_status(spi_sel) & FLD_SPI_TXF_FULL);
            reg_spi_wr_rd_data(spi_sel, k % 4) = data[j];
        }
    }
    while (spi_is_busy(spi_sel));
}
/**
 * @brief       This function serves to single/dual/quad  read from the SPI slave.
 * @param[in]   spi_sel     - the spi module.
 * @param[in]   cmd         - cmd one byte will first write.
 * @param[in]   addr        - the address of slave.
 * @param[in]   data        - pointer to the data need to read.
 * @param[in]   data_len    - the length of data.
 * @param[in]   rd_mode     - read mode.dummy or not dummy.
 * @return      none
 */
void spi_master_read_plus(spi_sel_e spi_sel, unsigned short cmd, unsigned int addr, unsigned char *data, unsigned int data_len, spi_rd_tans_mode_e rd_mode)
{
    spi_rx_dma_dis(spi_sel);
    spi_rx_fifo_clr(spi_sel);
    spi_set_address(spi_sel, addr);
    spi_set_transmode(spi_sel, (spi_tans_mode_e)rd_mode);
    spi_rx_cnt(spi_sel, data_len);
    //It is valid only when cmd1_en is enabled.
    spi_set_cmd1(spi_sel, ((cmd >> 8) & 0xff));//The cmd1 must be configured first.
    spi_set_cmd(spi_sel, (cmd & 0xff));
    spi_read(spi_sel, (unsigned char *)data, data_len);
    while (spi_is_busy(spi_sel));
}

/**
 * @brief       This function serves to write address, then read data from the SPI slave.
 * This interface cannot be used for full duplex.
 * @param[in]   spi_sel     - the spi module.
 * @param[in]   cmd         - cmd one byte will first write.
 * @param[in]   addrs       - pointer to the address of slave.
 * @param[in]   addr_len    - the length of address.
 * @param[in]   data        - the pointer to the data for read.
 * @param[in]   data_len    - read length.
 * @param[in]   wr_mode     - write mode.dummy or not dummy.
 * @return      none
 */
void spi_master_write_read_plus(spi_sel_e spi_sel, unsigned short cmd, unsigned char *addrs, unsigned int addr_len, unsigned char *data, unsigned int data_len, spi_rd_tans_mode_e wr_mode)
{
    spi_tx_dma_dis(spi_sel);
    spi_rx_dma_dis(spi_sel);
    spi_tx_fifo_clr(spi_sel);
    spi_rx_fifo_clr(spi_sel);
    spi_tx_cnt(spi_sel, addr_len);
    spi_rx_cnt(spi_sel, data_len);
    spi_set_transmode(spi_sel, (spi_tans_mode_e)wr_mode);
    //It is valid only when cmd1_en is enabled.
    spi_set_cmd1(spi_sel, ((cmd >> 8) & 0xff));//The cmd1 must be configured first.
    spi_set_cmd(spi_sel, (cmd & 0xff));
    spi_write(spi_sel, (unsigned char *)addrs, addr_len);
    spi_read(spi_sel, (unsigned char *)data, data_len);
    while (spi_is_busy(spi_sel));
}

/**
 * @brief       This function serves to set rx_dma channel and config dma rx default for spi master.
 * @param[in]   spi_sel - the spi module.
 * @param[in]   chn     - dma channel.
 * @return      none
 */
void spi_set_master_rx_dma_config(spi_sel_e spi_sel,dma_chn_e chn)
{
    if (GSPI_MODULE == spi_sel)
    {
        s_gspi_master_rx_dma_chn = chn;
        dma_config(chn, &gspi_master_rx_dma_config);
    }
    else if(LSPI_MODULE == spi_sel)
    {
        s_lspi_master_rx_dma_chn = chn;
        dma_config(chn, &lspi_master_rx_dma_config);
    }
}

/**
 * @brief       This function serves to set rx_dma channel and config dma rx default for spi slave.
 * @param[in]   spi_sel - the spi module.
 * @param[in]   chn     - dma channel.
 * @return      none
 */
void spi_set_slave_rx_dma_config(spi_sel_e spi_sel,dma_chn_e chn)
{
    if (GSPI_MODULE == spi_sel)
    {
        s_gspi_slave_rx_dma_chn = chn;
        dma_config(chn, &gspi_slave_rx_dma_config);
    }
    else if(LSPI_MODULE == spi_sel)
    {
        s_lspi_slave_rx_dma_chn = chn;
        dma_config(chn, &lspi_slave_rx_dma_config);
    }
}
/**
 * @brief       This function serves to set tx_dma channel and config dma tx default.
 * @param[in]   spi_sel - the spi module.
 * @param[in]   chn     - dma channel.
 * @return      none
 */
void spi_set_tx_dma_config(spi_sel_e spi_sel,dma_chn_e chn)
{
    if (GSPI_MODULE == spi_sel)
    {
        s_gspi_tx_dma_chn = chn;
        dma_config(chn, &gspi_tx_dma_config);
    }
    else if(LSPI_MODULE == spi_sel)
    {
        s_lspi_tx_dma_chn = chn;
        dma_config(chn, &lspi_tx_dma_config);
    }
}

/**
 * @brief       this  function set spi dma channel.
 * @param[in]   spi_dma_chn - dma channel.
 * @param[in]   src_addr    - the address of source.
 * @param[in]   dst_addr    - the address of destination.
 * @param[in]   len         - the length of data.
 * */
void spi_set_dma(dma_chn_e spi_dma_chn, unsigned int src_addr, unsigned int dst_addr, unsigned int len)
{
    dma_set_address(spi_dma_chn, src_addr, dst_addr);
    dma_set_size(spi_dma_chn, len, DMA_WORD_WIDTH);
    dma_chn_en(spi_dma_chn);
}
/**
 * @brief       this  function set spi tx dma channel.
 * @param[in]   spi_sel     - the spi module.
 * @param[in]   src_addr    - the address of source.
 * @param[in]   len         - the length of data.
 * @note        src_addr : must be aligned by word (4 bytes), otherwise the program will enter an exception
 *
 * */
void spi_set_tx_dma(spi_sel_e spi_sel, unsigned char* src_addr,unsigned int len)
{
    unsigned char tx_dma_chn;
    if (GSPI_MODULE == spi_sel)
    {
        tx_dma_chn = s_gspi_tx_dma_chn;
    }
    else
    {
        tx_dma_chn = s_lspi_tx_dma_chn;
    }
    spi_tx_dma_en(spi_sel);
    dma_set_address(tx_dma_chn, (unsigned int)src_addr, reg_spi_data_buf_adr(spi_sel));
    dma_set_size(tx_dma_chn, len, DMA_WORD_WIDTH);
    dma_chn_en(tx_dma_chn);
}

/**
 * @brief       this  function set spi rx dma channel.
 * @param[in]   spi_sel     - the spi module.
 * @param[in]   dst_addr    - the address of destination.
 * @param[in]   len         - the length of data.
 * @note
               -# If write_num is enabled, the length of data received by DMA will be written into the first four bytes of the address.
               -# A length greater than XX_len itself and a multiple of 4 is denoted as CEILING(XX_len,4). For example: XX_len=3 ,CEILING(XX_len,4)=4; XX_len=21 ,CEILING(Tx_length, 4)=24.
                  The actual length sent by master  is denoted as Tx_len, The length (param[in]-len) of the interface configuration is denoted as Rx_len.
                  when CEILING(Tx_len,4) > CEILING(Rx_len,4), When the length of the DMA carry reaches Rx_len, the DMA will not stop working and the excess data will not be carried into the buff.
                  for example:Tx_len=21,Rx_len=20,When the DMA stops working the buff is written with a length of 21 and only 20 bytes of data are stored.It is recommended to configure the appropriate Rx_len to avoid this situation.
                -# After DMA transfer completion, the interface needs to be invoked again to read the next batch of data.
 * */
void spi_set_rx_dma(spi_sel_e spi_sel, unsigned char* dst_addr, unsigned int len)
{
    unsigned char rx_dma_chn;
    if (GSPI_MODULE == spi_sel)
    {
        rx_dma_chn = s_gspi_slave_rx_dma_chn;
    }
    else
    {
        rx_dma_chn = s_lspi_slave_rx_dma_chn;
    }
    dma_chn_dis(rx_dma_chn);
    spi_rx_dma_en(spi_sel);
    dma_set_address(rx_dma_chn, reg_spi_data_buf_adr(spi_sel), (unsigned int)(dst_addr));
    dma_set_size(rx_dma_chn,len,DMA_WORD_WIDTH);
    dma_chn_en(rx_dma_chn);
}
/**
 * @brief       This function serves to normal write data by dma.
 * @param[in]   spi_sel     - the spi module.
 * @param[in]   src_addr    - the pointer to the data for write.
 * @param[in]   len         - write length.
 * @return      none
 * @note        src_addr : must be aligned by word (4 bytes), otherwise the program will enter an exception
 */
void spi_master_write_dma(spi_sel_e spi_sel, unsigned char *src_addr, unsigned int len)
{
    unsigned char tx_dma_chn;
    spi_tx_fifo_clr(spi_sel);
    spi_tx_dma_en(spi_sel);
    spi_tx_cnt(spi_sel, len);
    spi_set_transmode(spi_sel, SPI_MODE_WRITE_ONLY);
    if (GSPI_MODULE == spi_sel)
    {
        tx_dma_chn = s_gspi_tx_dma_chn;
    }
    else
    {
        tx_dma_chn = s_lspi_tx_dma_chn;
    }

    spi_set_dma(tx_dma_chn,(unsigned int)src_addr, reg_spi_data_buf_adr(spi_sel), len);
    spi_set_cmd(spi_sel, 0x00);
}

/**
 * @brief       This function serves to normal write cmd and address, then read data by dma.
 * This interface could be used for full duplex.
 * When this interface is used for full-duplex communication, it can only be used on the master side,
 * and the spi_master_config() interface must be called first to disable hardware cmd and hardware address, addr_len is equal to data_len.
 * @param[in]   spi_sel     - the spi module.
 * @param[in]   addr        - the pointer to the cmd and address for write.
 * @param[in]   addr_len    - write length.
 * @param[in]   data        - the pointer to the data for read.
 * @param[in]   data_len    - read length.
 * @note        addr/data : must be aligned by word (4 bytes), otherwise the program will enter an exception.
 * @return      none
 */
void spi_master_write_read_dma(spi_sel_e spi_sel, unsigned char *addr, unsigned int addr_len, unsigned char *data, unsigned int data_len)
{
    unsigned char tx_dma_chn, rx_dma_chn;
    spi_tx_fifo_clr(spi_sel);
    spi_rx_fifo_clr(spi_sel);
    spi_tx_dma_en(spi_sel);
    spi_rx_dma_en(spi_sel);
    spi_tx_cnt(spi_sel, addr_len);
    spi_rx_cnt(spi_sel, data_len);
    spi_set_transmode(spi_sel, SPI_MODE_WRITE_READ);
    if (GSPI_MODULE == spi_sel)
    {
        tx_dma_chn = s_gspi_tx_dma_chn;
        rx_dma_chn = s_gspi_master_rx_dma_chn;
    }
    else
    {
        tx_dma_chn = s_lspi_tx_dma_chn;
        rx_dma_chn = s_lspi_master_rx_dma_chn;
    }
    spi_set_dma(tx_dma_chn, (unsigned int)(addr), reg_spi_data_buf_adr(spi_sel), addr_len);
    spi_set_dma(rx_dma_chn, reg_spi_data_buf_adr(spi_sel), (unsigned int)(data), data_len);
    spi_set_cmd(spi_sel, 0x00);//when  cmd  disable that  will not sent cmd,just trigger spi send .
}

/**
 * @brief       This function serves to single/dual/quad  write to the SPI slave by dma.
 * @param[in]   spi_sel     - the spi module.
 * @param[in]   cmd         - cmd one byte will first write.
 * @param[in]   addr        - the address of slave.
 * @param[in]   data        - pointer to the data need to write.
 * @param[in]   data_len    - length in byte of the data need to write.
 * @param[in]   wr_mode     - write mode.dummy or not dummy.
 * @note        data : must be aligned by word (4 bytes), otherwise the program will enter an exception.
 * @return      none
 */
void spi_master_write_dma_plus(spi_sel_e spi_sel, unsigned short cmd, unsigned int addr, unsigned char *data, unsigned int data_len, spi_wr_tans_mode_e wr_mode)
{
    unsigned char tx_dma_chn;
    spi_tx_fifo_clr(spi_sel);
    spi_tx_dma_en(spi_sel);
    spi_tx_cnt(spi_sel, data_len);
    spi_set_transmode(spi_sel, (spi_tans_mode_e)wr_mode);
    spi_set_address(spi_sel,addr);
    if (GSPI_MODULE == spi_sel)
    {
        tx_dma_chn = s_gspi_tx_dma_chn;
    }
    else
    {
        tx_dma_chn = s_lspi_tx_dma_chn;
    }

    spi_set_dma(tx_dma_chn, (unsigned int)data, reg_spi_data_buf_adr(spi_sel), data_len);
    //It is valid only when cmd1_en is enabled.
    spi_set_cmd1(spi_sel, ((cmd >> 8) & 0xff));//The cmd1 must be configured first.
    spi_set_cmd(spi_sel, (cmd & 0xff));
}

/**
 * @brief       This function serves to single/dual/quad  read from the SPI slave by dma.
 * @param[in]   spi_sel     - the spi module.
 * @param[in]   cmd         - cmd one byte will first write.
 * @param[in]   addr        - the address of slave.
 * @param[in]   dst_addr    - pointer to the buffer that will cache the reading out data.
 * @param[in]   data_len    - length in byte of the data need to read.
 * @param[in]   rd_mode     - read mode.dummy or not dummy.
 * @note        dst_addr : must be aligned by word (4 bytes), otherwise the program will enter an exception.
 * @return      none
 */
void spi_master_read_dma_plus(spi_sel_e spi_sel, unsigned short cmd, unsigned int addr, unsigned char *dst_addr, unsigned int data_len, spi_rd_tans_mode_e rd_mode)
{
    unsigned char rx_dma_chn;
    spi_rx_fifo_clr(spi_sel);
    spi_rx_dma_en(spi_sel);
    spi_set_transmode(spi_sel, (spi_tans_mode_e)rd_mode);
    spi_rx_cnt(spi_sel, data_len);
    spi_set_address(spi_sel,addr);
    if (GSPI_MODULE == spi_sel)
    {
        rx_dma_chn = s_gspi_master_rx_dma_chn;
    }
    else
    {
        rx_dma_chn = s_lspi_master_rx_dma_chn;
    }
    spi_set_dma(rx_dma_chn, reg_spi_data_buf_adr(spi_sel), (unsigned int)(dst_addr), data_len);
    //It is valid only when cmd1_en is enabled.
    spi_set_cmd1(spi_sel, ((cmd >> 8) & 0xff));//The cmd1 must be configured first.
    spi_set_cmd(spi_sel, (cmd & 0xff));
}

/**
 * @brief       This function serves to single/dual/quad write address and read from the SPI slave by dma.
 * This interface could be used for full duplex.
 * When this interface is used for full-duplex communication, it can only be used on the master side.
 * 1.the spi_master_config() interface must be called first to disable hardware cmd and hardware address
 * 2.must cmd is 0,addr_len is equal to rd_len,rd_mode is SPI_MODE_WRITE_AND_READ.
 * @param[in]   spi_sel     - the spi module.
 * @param[in]   cmd         - cmd one byte will first write.
 * @param[in]   addr        - the address of slave.
 * @param[in]   addr_len    - the length of address.
 * @param[in]   rd_data     - pointer to the buffer that will cache the reading out data.
 * @param[in]   rd_len      - length in byte of the data need to read.
 * @param[in]   rd_mode     - read mode.dummy or not dummy.
 * @return      none
 * @note        addr/rd_data : must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
void spi_master_write_read_dma_plus(spi_sel_e spi_sel, unsigned short cmd, unsigned char *addr, unsigned int addr_len, unsigned char *dst_addr, unsigned int rd_len, spi_rd_tans_mode_e rd_mode)
{
    unsigned char tx_dma_chn, rx_dma_chn;
    spi_tx_fifo_clr(spi_sel);
    spi_rx_fifo_clr(spi_sel);
    spi_tx_dma_en(spi_sel);
    spi_rx_dma_en(spi_sel);
    spi_tx_cnt(spi_sel,addr_len);
    spi_rx_cnt(spi_sel, rd_len);
    spi_set_transmode(spi_sel, (spi_tans_mode_e)rd_mode);
    if (GSPI_MODULE == spi_sel)
    {
        tx_dma_chn = s_gspi_tx_dma_chn;
        rx_dma_chn = s_gspi_master_rx_dma_chn;
    }
    else
    {
        tx_dma_chn = s_lspi_tx_dma_chn;
        rx_dma_chn = s_lspi_master_rx_dma_chn;
    }
    spi_set_dma(tx_dma_chn, (unsigned int)(addr), reg_spi_data_buf_adr(spi_sel), addr_len);
    spi_set_dma(rx_dma_chn, reg_spi_data_buf_adr(spi_sel), (unsigned int)(dst_addr), rd_len);
    //It is valid only when cmd1_en is enabled.
    spi_set_cmd1(spi_sel, ((cmd >> 8) & 0xff));//The cmd1 must be configured first.
    spi_set_cmd(spi_sel, (cmd & 0xff));//when  cmd  disable that  will not sent cmd,just trigger spi send .
}
/**
 * @brief       This function serves to write and read data simultaneously.
   @verbatim
   -# This interface can only be used by the master.
   -#  When initializing the master, call the spi_master_config() interface to disable the hardware cmd and hardware address,
       and then start sending and receiving data.
   -# The default chunk size sent and received by this interface is 8byte.
   @endverbatim
 * @param[in]   spi_sel     - the spi module.
 * @param[in]   write_data  - write data pointer.
 * @param[in]   read_data   - read data pointer.
 * @param[in]   len         - write/read length.
 * @return      none
 */
void spi_master_write_read_full_duplex(spi_sel_e spi_sel,unsigned char *write_data, unsigned char *read_data, unsigned int len)
{
    unsigned int spi_wr_rd_size = 8;
    unsigned int chunk_size = spi_wr_rd_size;
    spi_tx_dma_dis(spi_sel);
    spi_rx_dma_dis(spi_sel);
    spi_set_transmode(spi_sel, SPI_MODE_WRITE_AND_READ);
    spi_set_cmd(spi_sel, 0);
    spi_tx_fifo_clr(spi_sel);
    spi_rx_fifo_clr(spi_sel);
    spi_tx_cnt(spi_sel, len);
    spi_rx_cnt(spi_sel, len);

    for(unsigned int i = 0; i<len; i = i +chunk_size){
        if(chunk_size > (len - i)){
            chunk_size = len - i;
        }
        spi_write(spi_sel, write_data + i, chunk_size);
        if(len < spi_wr_rd_size){
            spi_read(spi_sel, read_data, chunk_size);
        }
        else if(i == 0){
            spi_read(spi_sel, read_data, chunk_size - 1);
        }else if((len - i) > spi_wr_rd_size){
            spi_read(spi_sel, read_data + i -1, chunk_size);
        }else{
            spi_read(spi_sel, read_data + i -1, chunk_size + 1);
        }
        spi_rx_fifo_clr(spi_sel);
        spi_tx_fifo_clr(spi_sel);
    }
}
/**
 * @brief       This function serves to read data in normal.
 * @param[in]   spi_sel     - the spi module.
 * @param[in]   data        - read data pointer.
 * @param[in]   len         - read length.
 * @return      none
 */
void spi_master_read(spi_sel_e spi_sel, unsigned char *data, unsigned int len)
{
    spi_rx_dma_dis(spi_sel);
    spi_rx_fifo_clr(spi_sel);
    spi_rx_cnt(spi_sel, len);
    spi_set_transmode(spi_sel, SPI_MODE_READ_ONLY);
    spi_set_cmd(spi_sel, 0x00);//when  cmd  disable that  will not sent cmd,just trigger spi send .
    spi_read(spi_sel, (unsigned char *)data, len);
}

/**
  * @brief     This function serves to config master rx_dma channel llp.
  * @param[in]      spi_sel     - the spi module.
  * @param[in]      chn     - dma channel.
  * @param[in]      cmd         - cmd one byte will first write.
  * @param[in]      addr        - the address of slave.
  * @param[in]      dst_addr        - pointer to the data need to read.
  * @param[in]      data_len    - length in byte of the data need to read.
  * @param[in]      rd_mode     - read mode.dummy or not dummy.
  * @param[in]      head_of_list - the head address of dma llp.
  * @return    none
  */
 void spi_set_master_rx_dma_chain_llp(spi_sel_e spi_sel, dma_chn_e chn, unsigned short cmd, unsigned int addr, unsigned char *dst_addr, unsigned int data_len, spi_rd_tans_mode_e rd_mode,dma_chain_config_t *head_of_list)
 {
    spi_set_master_rx_dma_config(spi_sel,chn);
    spi_rx_fifo_clr(spi_sel);
    spi_set_rx_llp_sof_mode(chn);
    spi_rx_dma_en(spi_sel);
    spi_set_transmode(spi_sel, (spi_tans_mode_e)rd_mode);
    spi_rx_cnt(spi_sel, data_len);
    spi_dma_trig_spi_en(spi_sel);
    spi_set_address(spi_sel,addr);
    //It is valid only when cmd1_en is enabled.
    spi_set_cmd1(spi_sel, ((cmd >> 8) & 0xff));//The cmd1 must be configured first.
    spi_set_cmd(spi_sel, (cmd & 0xff));
    dma_set_address(chn,reg_spi_data_buf_adr(spi_sel),(unsigned int)(dst_addr));
    dma_set_size(chn, data_len, DMA_WORD_WIDTH);
    reg_dma_llp(chn)=(unsigned int)(head_of_list);
 }
/**
  * @brief     This function serves to config slave rx_dma channel llp.
  * @param[in] spi_sel  - the spi module.
  * @param[in] chn          - dma channel
  * @param[in] dst_addr     - the dma address of destination
  * @param[in] head_of_list - the head address of dma llp.
  * @return    none
  */
 void spi_set_slave_rx_dma_chain_llp(spi_sel_e spi_sel, dma_chn_e chn,unsigned char * dst_addr, unsigned int data_len,dma_chain_config_t *head_of_list)
 {
    spi_set_slave_rx_dma_config(spi_sel,chn);
    spi_rx_fifo_clr(spi_sel);
    spi_set_rx_llp_sof_mode(chn);
    spi_rx_dma_en(spi_sel);
    dma_set_address(chn,reg_spi_data_buf_adr(spi_sel),(unsigned int)(dst_addr));
    dma_set_size(chn, data_len, DMA_WORD_WIDTH);
    reg_dma_llp(chn)=(unsigned int)(head_of_list);
 }
 /**
  * @brief     This function serves to set rx dma chain transfer
  * @param[in] spi_sel  - the spi module.
  * @param[in] chn          - dma channel
  * @param[in] rx_config - the head of list of llp_pointer.
  * @param[in] llpointer - the next element of llp_pointer.
  * @param[in] dst_addr  -the dma address of destination.
  * @param[in] data_len  -the length of dma size by byte.
  * @return    none
  */
 void spi_rx_dma_add_list_element(spi_sel_e spi_sel,dma_chn_e chn,dma_chain_config_t *config_addr,dma_chain_config_t *llpointer ,unsigned char *dst_addr,unsigned int data_len)
 {
    config_addr->dma_chain_ctl=reg_dma_ctrl(chn)|BIT(0);
    config_addr->dma_chain_src_addr=reg_spi_data_buf_adr(spi_sel);
    config_addr->dma_chain_dst_addr= (unsigned int)(dst_addr);
    config_addr->dma_chain_data_len=dma_cal_size(data_len,4);
    config_addr->dma_chain_llp_ptr=(unsigned int)(llpointer);
 }
 /**
   * @brief     This function serves to config master tx_dma channel llp.
   * @param[in]     spi_sel     - the spi module.
   * @param[in]     chn     - dma channel.
   * @param[in]     cmd         - cmd one byte will first write.
   * @param[in]     addr        - the address of slave.
   * @param[in]     src_addr    - pointer to the data need to write.
   * @param[in]     data_len    - length in byte of the data need to write.
   * @param[in]     wr_mode     - write mode.dummy or not dummy.
   * @param[in]     head_of_list - the head address of dma llp.
   * @return    none
   */
 void spi_set_master_tx_dma_chain_llp(spi_sel_e spi_sel,dma_chn_e chn, unsigned short cmd, unsigned int addr, unsigned char *src_addr, unsigned int data_len, spi_wr_tans_mode_e wr_mode,dma_chain_config_t *head_of_list)
 {
    spi_set_tx_dma_config(spi_sel,chn);
    spi_tx_fifo_clr(spi_sel);
    spi_set_tx_llp_sof_mode(chn,1);
    spi_tx_dma_en(spi_sel);
    spi_tx_cnt(spi_sel, data_len);
    spi_dma_trig_spi_en(spi_sel);
    spi_set_transmode(spi_sel, (spi_tans_mode_e)wr_mode);
    spi_set_address(spi_sel,addr);
    //It is valid only when cmd1_en is enabled.
    spi_set_cmd1(spi_sel, ((cmd >> 8) & 0xff));//The cmd1 must be configured first.
    spi_set_cmd(spi_sel, (cmd & 0xff));
    dma_set_address(chn,(unsigned int)(src_addr),reg_spi_data_buf_adr(spi_sel));
    dma_set_size(chn, data_len, DMA_WORD_WIDTH);
    reg_dma_llp(chn)=(unsigned int)(head_of_list);
 }
/**
  * @brief     This function serves to config slave rx_dma channel llp.
  * @param[in] spi_sel      - the spi module.
  * @param[in] chn          - dma channel
  * @param[in] src_addr     - the dma address of source
  * @param[in] data_len     - the length of dma rx size by byte
  * @param[in] head_of_list - the head address of dma llp.
  * @return    none
*/
void spi_set_slave_tx_dma_chain_llp(spi_sel_e spi_sel, dma_chn_e chn,unsigned char *src_addr,unsigned int data_len,dma_chain_config_t *head_of_list)
{
    if (GSPI_MODULE == spi_sel)
    {
        dma_config(chn, &gspi_tx_dma_config);
    }
    else if(LSPI_MODULE == spi_sel)
    {
        dma_config(chn, &lspi_tx_dma_config);
    }
    spi_tx_fifo_clr(spi_sel);
    spi_set_tx_llp_sof_mode(chn,1);
    spi_tx_dma_en(spi_sel);
    dma_set_address(chn,(unsigned int)(src_addr),reg_spi_data_buf_adr(spi_sel));
    dma_set_size(chn, data_len, DMA_WORD_WIDTH);
    reg_dma_llp(chn)=(unsigned int)(head_of_list);
}
/**
* @brief     This function serves to set tx dma chain transfer
* @param[in] spi_sel        - the spi module.
* @param[in] chn          - dma channel
* @param[in] config_addr - the head of list of llp_pointer.
* @param[in] llpointer - the next element of llp_pointer.
* @param[in] src_addr  -the dma address of source.
* @param[in] data_len  -the length of dma size by byte.
* @return    none
*/
void spi_tx_dma_add_list_element(spi_sel_e spi_sel,dma_chn_e chn,dma_chain_config_t *config_addr,dma_chain_config_t *llpointer ,unsigned char *src_addr,unsigned int data_len)
{
   config_addr->dma_chain_ctl=reg_dma_ctrl(chn)|BIT(0);
   config_addr->dma_chain_src_addr=(unsigned int)(src_addr);
   config_addr->dma_chain_dst_addr= reg_spi_data_buf_adr(spi_sel);
   config_addr->dma_chain_data_len=dma_cal_size(data_len,4);
   config_addr->dma_chain_llp_ptr=(unsigned int)(llpointer);
}
