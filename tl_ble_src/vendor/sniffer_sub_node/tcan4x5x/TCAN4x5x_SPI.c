/*
 * TCAN4x5x_SPI.c
 * Description: This file is responsible for abstracting the lower-level microcontroller SPI read and write functions
 *
 *
 *
 * Copyright (c) 2019 Texas Instruments Incorporated.  All rights reserved.
 * Software License Agreement
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of Texas Instruments Incorporated nor the names of
 * its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "TCAN4x5x_SPI.h"


__attribute__((aligned(4))) uint8_t regWrite[8] = {0};

#if (SPI_MODE == SPI_NDMA_MODE)
  #if (BOARD_SELECT==BOARD_9528A_EVK_C1T266A20_V1_3)
    gspi_pin_config_t gspi_pin_config =
    {
        .spi_csn_pin        = TCAN4550_GPIO_CS_N,
        .spi_clk_pin        = TCAN4550_GPIO_SCLK,
        .spi_mosi_io0_pin   = TCAN4550_GPIO_DO,
        .spi_miso_io1_pin   = TCAN4550_GPIO_DI,
        .spi_io2_pin        = 0,
        .spi_io3_pin        = 0,
    };
  #elif (BOARD_SELECT==BOARD_9223A_EVK_C1T289A67_V1_0)
    lspi_pin_config_t lspi_pin_config =
    {
        .spi_csn_pin        = TCAN4550_GPIO_CS_N,
        .spi_clk_pin        = TCAN4550_GPIO_SCLK,
        .spi_mosi_io0_pin   = TCAN4550_GPIO_DO,
        .spi_miso_io1_pin   = TCAN4550_GPIO_DI,
        .spi_io2_pin        = 0,
        .spi_io3_pin        = 0,
    };
  #elif (BOARD_SELECT==BOARD_9223B_EVK_C1T325A67_V1_0)
    lspi_pin_config_t lspi_pin_config =
    {
        .spi_csn_pin        = TCAN4550_GPIO_CS_N,
        .spi_clk_pin        = TCAN4550_GPIO_SCLK,
        .spi_mosi_io0_pin   = TCAN4550_GPIO_DO,
        .spi_miso_io1_pin   = TCAN4550_GPIO_DI,
        .spi_io2_pin        = 0,
        .spi_io3_pin        = 0,
    };
  #endif
#elif (SPI_MODE == SPI_DMA_MODE)
    #define SPI_TX_DMA_CHN  DMA4
    #define SPI_RX_DMA_CHN  DMA5

    _attribute_ble_data_retention_ volatile u32 end_irq_flag = 1; /* default write complete */
    _attribute_ble_data_retention_ volatile u32 rx_dma_flag = 1;  /* default read complete */

    lspi_pin_config_t lspi_pin_config =
    {
        .spi_csn_pin        = TCAN4550_GPIO_CS_N,
        .spi_clk_pin        = TCAN4550_GPIO_SCLK,
        .spi_mosi_io0_pin   = TCAN4550_GPIO_DO,
        .spi_miso_io1_pin   = TCAN4550_GPIO_DI,
        .spi_io2_pin        = 0,
        .spi_io3_pin        = 0,
    };
    spi_wr_rd_config_t spi_b91m_slave_protocol_config =
    {
        .spi_io_mode        = SPI_SINGLE_MODE,/*IO mode set to SPI_3_LINE_MODE when SPI_3LINE_SLAVE.*/
        .spi_dummy_cnt      = 0,//B92 supports up to 32 clk cycle dummy, and TL751X, TL721X supports up to 256 clk cycle dummy.
        .spi_cmd_en         = 0,
        .spi_addr_en        = 0,
        .spi_addr_len       = 0,//when spi_addr_en = 0,invalid set.
        .spi_cmd_fmt_en     = 0,//when spi_cmd_en = 0,invalid set.
        .spi_addr_fmt_en    = 0,//when spi_addr_en = 0,invalid set.
    };
#endif

void tcan4550_spi_init(void)
{
#if (SPI_MODE == SPI_NDMA_MODE)
    spi_master_init(SPI_MODULE_SEL, sys_clk.pll_clk * 1000000/SPI_CLK, SPI_MODE0);
  #if (BOARD_SELECT==BOARD_9528A_EVK_C1T266A20_V1_3)
    gspi_set_pin(&gspi_pin_config);
  #elif (BOARD_SELECT==BOARD_9223A_EVK_C1T289A67_V1_0)
    lspi_set_pin(&lspi_pin_config);
  #elif (BOARD_SELECT==BOARD_9223B_EVK_C1T325A67_V1_0)
    lspi_set_pin(&lspi_pin_config);
  #endif
    spi_master_config(SPI_MODULE_SEL, SPI_NORMAL);
#elif (SPI_MODE == SPI_DMA_MODE)
    /* clear */
    spi_clr_irq_status(SPI_MODULE_SEL, SPI_END_INT);
    /* endint_en, cs high trigger */
    spi_set_irq_mask(SPI_MODULE_SEL, SPI_END_INT_EN);

    spi_master_init(SPI_MODULE_SEL, sys_clk.pll_clk * 1000000/SPI_CLK, SPI_MODE0);

    spi_set_tx_dma_config(SPI_MODULE_SEL,SPI_TX_DMA_CHN);
    spi_set_master_rx_dma_config(SPI_MODULE_SEL,SPI_RX_DMA_CHN);
    /* Only master supports burst mode.
     * GSPI RX DMA supports DMA_BURST_2_WORD/DMA_BURST_1_WORD, TX DMA supports DMA_BURST_4_WORD/DMA_BURST_2_WORD/DMA_BURST_1_WORD.
     * LSPI RX DMA supports DMA_BURST_2_WORD/DMA_BURST_1_WORD, TX DMA supports DMA_BURST_4_WORD/DMA_BURST_2_WORD/DMA_BURST_1_WORD.*/
    dma_set_spi_burst_size(SPI_TX_DMA_CHN,DMA_BURST_1_WORD);
    dma_set_spi_burst_size(SPI_RX_DMA_CHN,DMA_BURST_1_WORD);

    lspi_set_pin(&lspi_pin_config);
    plic_interrupt_enable(IRQ_LSPI);
    spi_master_config_plus(SPI_MODULE_SEL, &spi_b91m_slave_protocol_config);
    dma_set_irq_mask(SPI_RX_DMA_CHN,TC_MASK);
    plic_interrupt_enable(IRQ_DMA);
    core_interrupt_enable();
#endif
}

#if (SPI_MODE == SPI_DMA_MODE)
_attribute_ram_code_sec_noinline_ void lspi_irq_handler(void)
{
    if(spi_get_irq_status(SPI_MODULE_SEL,SPI_END_INT))
    {
        spi_clr_irq_status(SPI_MODULE_SEL, SPI_END_INT);
        end_irq_flag = 1;
    }
}
PLIC_ISR_REGISTER(lspi_irq_handler, IRQ_LSPI)

_attribute_ram_code_sec_noinline_ void dma_irq_handler(void)
{
    if(dma_get_tc_irq_status(BIT(SPI_RX_DMA_CHN)))
    {
        dma_clr_tc_irq_status(BIT(SPI_RX_DMA_CHN));
        rx_dma_flag = 1;
    }
}
PLIC_ISR_REGISTER(dma_irq_handler, IRQ_DMA)
#endif

uint32_t can_spi_write(uint8_t *pWrite, uint32_t writeLen)
{
#if(SPI_MODE==SPI_NDMA_MODE)
    spi_master_write(SPI_MODULE_SEL, pWrite, writeLen);
#else
    /* Waiting for the complete of the previous SPI write, the interrupt will set
     * this flag to 1
     */
    uint32_t tickNow = clock_time();
    while(0==end_irq_flag)
    //while (spi_is_busy(SPI_MODULE_SEL))
    {
        /* clk=4M, 15 bytes about 30.5us. */
        if(clock_time_exceed(tickNow, 10*1000))
        {
            tlkapi_printf(APP_CAN_LOG_EN, "[APP][CAN-FD] spi write error! %s, %d\r\n", __FUNCTION__, __LINE__);
            end_irq_flag = 1;
            return 1;
        }
    }
    end_irq_flag = 0;
    spi_master_write_dma_plus(SPI_MODULE_SEL,   SPI_WRITE_DATA_SINGLE_CMD, (unsigned int)NULL, pWrite, writeLen, SPI_MODE_WR_WRITE_ONLY);
#endif

    return 0;
}

uint32_t can_spi_read(uint8_t *pRead, uint32_t readLen)
{
#if(SPI_MODE==SPI_NDMA_MODE)
    spi_master_read(SPI_MODULE_SEL, pRead, readLen);
#else
    /* Waiting for the complete of the previous SPI write, the interrupt will set
     * this flag to 1
     */
    uint32_t tickNow = clock_time();
    while(0==end_irq_flag)
    //while (spi_is_busy(SPI_MODULE_SEL))
    {
        /* clk=4M, 15 bytes about 30.5us. */
        if(clock_time_exceed(tickNow, 10*1000))
        {
            tlkapi_printf(APP_CAN_LOG_EN, "[APP][CAN-FD] spi write error! %s, %d\r\n", __FUNCTION__, __LINE__);
            end_irq_flag = 1;
            break;
        }
    }

    spi_clr_irq_status(SPI_MODULE_SEL, SPI_END_INT);
    rx_dma_flag = 0;
    spi_master_read_dma_plus(SPI_MODULE_SEL, SPI_READ_DATA_SINGLE_CMD, (unsigned int)NULL, pRead, readLen, SPI_MODE_RD_READ_ONLY);
    /* Waiting for the complete of the SPI read, the dma interrupt will set this
     * flag to 1
     */
    tickNow = clock_time();
    while(0==rx_dma_flag)
    //while (spi_is_busy(SPI_MODULE_SEL))
    {
        /* clk=4M, 15 bytes about 30.5us. */
        if(clock_time_exceed(tickNow, 10*1000))
        {
            tlkapi_printf(APP_CAN_LOG_EN, "[APP][CAN-FD] spi read error! %s, %d\r\n", __FUNCTION__, __LINE__);
            rx_dma_flag = 1;
            spi_set_irq_mask(SPI_MODULE_SEL, SPI_END_INT_EN);
            return 1;
        }
    }
    spi_set_irq_mask(SPI_MODULE_SEL, SPI_END_INT_EN);
#endif
    return 0;
}

uint32_t can_spi_write_read(uint8_t *pWrite, uint32_t writeLen, uint8_t *pRead, uint32_t readLen)
{
#if(SPI_MODE==SPI_NDMA_MODE)
    spi_master_write_read(SPI_MODULE_SEL, pWrite, writeLen, pRead, readLen);
#else
  #if 0
    spi_master_write_read_dma(SPI_MODULE_SEL, pWrite, writeLen, pRead, readLen);
    while(!rx_dma_flag);
    rx_dma_flag = 0;
  #else
    /* Waiting for the complete of the previous SPI write, the interrupt will set
     * this flag to 1
     */
    uint32_t tickNow = clock_time();
    while(0==end_irq_flag)
    //while (spi_is_busy(SPI_MODULE_SEL))
    {
        /* clk=4M, 15 bytes about 30.5us. */
        if(clock_time_exceed(tickNow, 10*1000))
        {
            tlkapi_printf(APP_CAN_LOG_EN, "[APP][CAN-FD] spi write error! %s, %d\r\n", __FUNCTION__, __LINE__);
            end_irq_flag = 1;
            return 1;
        }
    }

    spi_clr_irq_status(SPI_MODULE_SEL, SPI_END_INT);
    rx_dma_flag = 0;
    spi_master_write_read_dma_plus(SPI_MODULE_SEL, (unsigned int)NULL, pWrite, writeLen, pRead, readLen, SPI_MODE_WRITE_READ);
    /* Waiting for the complete of the SPI read, the dma interrupt will set this
     * flag to 1
     */
    tickNow = clock_time();
    while(0==rx_dma_flag)
    //while (spi_is_busy(SPI_MODULE_SEL))
    {
        /* clk=4M, 15 bytes about 30.5us. */
        if(clock_time_exceed(tickNow, 10*1000))
        {
            tlkapi_printf(APP_CAN_LOG_EN, "[APP][CAN-FD] spi write_read error! %s, %d\r\n", __FUNCTION__, __LINE__);
            rx_dma_flag = 1;
            spi_set_irq_mask(SPI_MODULE_SEL, SPI_END_INT_EN);
            return 1;
        }
    }
    spi_set_irq_mask(SPI_MODULE_SEL, SPI_END_INT_EN);
  #endif
#endif

    return 0;
}
/*
 * @brief Single word write
 *
 * @param address A 16-bit address of the destination register
 * @param data A 32-bit word of data to write to the destination register
 */
void
AHB_WRITE_32(uint16_t address, uint32_t data)
{
#if 1
    /* must be aligned by word (4 bytes), otherwise the program will enter an exception. */
    //__attribute__((aligned(4))) uint8_t reg[8]={AHB_WRITE_OPCODE, address>>8, address&0xFF, 1, data>>24, data>>16, data>>8, data&0xFF};

    regWrite[0] = AHB_WRITE_OPCODE;
    regWrite[1] = address>>8;
    regWrite[2] = address&0xFF,
    regWrite[3] = 1;
    regWrite[4] = data>>24;
    regWrite[5] = data>>16;
    regWrite[6] = data>>8;
    regWrite[7] = data&0xFF;

    can_spi_write(regWrite, 8);
#else
    u8 reg[8];

    reg[0] = AHB_WRITE_OPCODE;
    reg[1] = address>>8;
    reg[2] = address&0xFF;
    reg[3] = 1;

    reg[4] = (data>>24)&0xFF;
    reg[5] = (data>>16)&0xFF;
    reg[6] = (data>>8)&0xFF;
    reg[7] = (data)&0xFF;

    spi_write(reg, 4, &reg[4], 4, TCAN4550_GPIO_CS_N);
#endif
}

/*
 * @brief Single word read
 *
 * @param address A 16-bit address of the source register
 *
 * @return Returns 32-bit word of data from source register
 */
uint32_t
AHB_READ_32(uint16_t address)
{
#if 1
    uint32_t returnData=0;
    __attribute__((aligned(4))) uint8_t reg[8]={AHB_READ_OPCODE, address>>8, address&0xFF, 1, 0,0,0,0};

    can_spi_write_read(reg, 4, reg+4, 4);
    returnData = (reg[4]<<24)|(reg[5]<<16)|(reg[6]<<8)|reg[7];

    return returnData;
#else
    uint32_t returnData = 0;
    u8 reg[8];

    reg[0] = AHB_READ_OPCODE;
    reg[1] = address>>8;
    reg[2] = address&0xFF;
    reg[3] = 1;

    reg[4] = 0;
    reg[5] = 0;
    reg[6] = 0;
    reg[7] = 0;
    spi_read(reg, 4, reg+4, 4, TCAN4550_GPIO_CS_N);

    returnData |= (reg[4]<<24);
    returnData |= (reg[5]<<16);
    returnData |= (reg[6]<<8);
    returnData |= (reg[7]<<0);

    return returnData;
#endif
}


/*
 * @brief Burst write start
 *
 * The SPI transaction contains 3 parts: the header (start), the payload, and the end of data (end)
 * This function is the start, where the register address and number of words are transmitted
 *
 * @param address A 16-bit address of the destination register
 * @param words The number of 4-byte words that will be transferred. 0 = 256 words
 */
void
AHB_WRITE_BURST_START(uint16_t address, uint8_t words)
{
#if 0
    //set the CS low to start the transaction
    GPIO_setOutputLowOnPin(SPI_CS_GPIO_PORT, TCAN4550_GPIO_CS_N);

    EUSCI_B_SPI_transmitData(SPI_HW_ADDR, AHB_WRITE_OPCODE);

    // Send the 16-bit address
    WAIT_FOR_TRANSMIT;
    EUSCI_B_SPI_transmitData(SPI_HW_ADDR, HWREG8(&address + 1));
    WAIT_FOR_TRANSMIT;
    EUSCI_B_SPI_transmitData(SPI_HW_ADDR, HWREG8(&address));


    WAIT_FOR_TRANSMIT;
    // Send the number of words to read
    EUSCI_B_SPI_transmitData(SPI_HW_ADDR, words);
#else
    /* must be aligned by word (4 bytes), otherwise the program will enter an exception. */
    //__attribute__((aligned(4))) uint8_t reg[4]={AHB_WRITE_OPCODE, address>>8, address&0xFF, words};

    regWrite[0] = AHB_WRITE_OPCODE;
    regWrite[1] = address>>8;
    regWrite[2] = address&0xFF;
    regWrite[3] = words;

    can_spi_write(regWrite, 4);
#endif
}


/*
 * @brief Burst write
 *
 * The SPI transaction contains 3 parts: the header (start), the payload, and the end of data (end)
 * This function writes a single word at a time
 *
 * @param data A 32-bit word of data to write to the destination register
 */
void
AHB_WRITE_BURST_WRITE(uint32_t data)
{
#if 0
    WAIT_FOR_TRANSMIT;
    EUSCI_B_SPI_transmitData(SPI_HW_ADDR, HWREG8(&data + 3));
    WAIT_FOR_TRANSMIT;
    EUSCI_B_SPI_transmitData(SPI_HW_ADDR, HWREG8(&data + 2));
    WAIT_FOR_TRANSMIT;
    EUSCI_B_SPI_transmitData(SPI_HW_ADDR, HWREG8(&data + 1));
    WAIT_FOR_TRANSMIT;
    EUSCI_B_SPI_transmitData(SPI_HW_ADDR, HWREG8(&data));
#else
    /* must be aligned by word (4 bytes), otherwise the program will enter an exception. */
    //__attribute__((aligned(4))) uint8_t reg[4]={(data>>24)&0xFF, (data>>16)&0xFF, (data>>8)&0xFF, (data)&0xFF};

    regWrite[0] = (data>>24)&0xFF;
    regWrite[1] = (data>>16)&0xFF;
    regWrite[2] = (data>>8)&0xFF;
    regWrite[3] = (data)&0xFF;

    can_spi_write(regWrite, 4);
#endif
}


/*
 * @brief Burst write end
 *
 * The SPI transaction contains 3 parts: the header (start), the payload, and the end of data (end)
 * This function ends the burst transaction by pulling nCS high
 */
void
AHB_WRITE_BURST_END(void)
{
#if 0
    WAIT_FOR_IDLE;
    GPIO_setOutputHighOnPin(SPI_CS_GPIO_PORT, TCAN4550_GPIO_CS_N);
#else
    //CS level is high
    gpio_write(TCAN4550_GPIO_CS_N,1);
#endif
}


/*
 * @brief Burst read start
 *
 * The SPI transaction contains 3 parts: the header (start), the payload, and the end of data (end)
 * This function is the start, where the register address and number of words are transmitted
 *
 * @param address A 16-bit start address to begin the burst read
 * @param words The number of 4-byte words that will be transferred. 0 = 256 words
 */
void
AHB_READ_BURST_START(uint16_t address, uint8_t words)
{
#if 0
    // Set the CS low to start the transaction
    GPIO_setOutputLowOnPin(SPI_CS_GPIO_PORT, TCAN4550_GPIO_CS_N);
    EUSCI_B_SPI_transmitData(SPI_HW_ADDR, AHB_READ_OPCODE);

    // Send the 16-bit address
    WAIT_FOR_TRANSMIT;
    EUSCI_B_SPI_transmitData(SPI_HW_ADDR, HWREG8(&address + 1));
    WAIT_FOR_TRANSMIT;
    EUSCI_B_SPI_transmitData(SPI_HW_ADDR, HWREG8(&address));

    // Send the number of words to read
    WAIT_FOR_TRANSMIT;
    EUSCI_B_SPI_transmitData(SPI_HW_ADDR, words);
#else
    /* must be aligned by word (4 bytes), otherwise the program will enter an exception. */
    //__attribute__((aligned(4))) uint8_t reg[4]={AHB_READ_OPCODE, address>>8, address&0xFF, words};

    regWrite[0] = AHB_READ_OPCODE;
    regWrite[1] = address>>8;
    regWrite[2] = address&0xFF;
    regWrite[3] = words;

    can_spi_write(regWrite, 4);
#endif
}


/*
 * @brief Burst read start
 *
 * The SPI transaction contains 3 parts: the header (start), the payload, and the end of data (end)
 * This function where each word of data is read from the TCAN4x5x
 *
 * @return A 32-bit single data word that is read at a time
 */
uint32_t
AHB_READ_BURST_READ(void)
{
#if 0
    uint8_t readData;
    uint8_t readData1;
    uint8_t readData2;
    uint8_t readData3;
    uint32_t returnData;

    WAIT_FOR_IDLE;
    EUSCI_B_SPI_transmitData(SPI_HW_ADDR, 0x00); // pause after this
    WAIT_FOR_IDLE;

    readData = HWREG8(SPI_HW_ADDR + OFS_UCBxRXBUF);
    EUSCI_B_SPI_transmitData(SPI_HW_ADDR, 0x00);


    WAIT_FOR_IDLE;
    readData1 = HWREG8(SPI_HW_ADDR + OFS_UCBxRXBUF);
    EUSCI_B_SPI_transmitData(SPI_HW_ADDR, 0x00);

    WAIT_FOR_IDLE;
    readData2 = HWREG8(SPI_HW_ADDR + OFS_UCBxRXBUF);
    EUSCI_B_SPI_transmitData(SPI_HW_ADDR, 0x00);

    WAIT_FOR_IDLE;
    readData3 = HWREG8(SPI_HW_ADDR + OFS_UCBxRXBUF);


    returnData = (((uint32_t)readData) << 24) | (((uint32_t)readData1 << 16)) | (((uint32_t)readData2) << 8) | readData3;
    return returnData;
#else
    __attribute__((aligned(4))) uint32_t returnData = 0;

    can_spi_read((u8 *)&returnData, 4);

    return returnData;
#endif
}


/*
 * @brief Burst write end
 *
 * The SPI transaction contains 3 parts: the header (start), the payload, and the end of data (end)
 * This function ends the burst transaction by pulling nCS high
 */
void
AHB_READ_BURST_END(void)
{
#if 0
    WAIT_FOR_IDLE;
    GPIO_setOutputHighOnPin(SPI_CS_GPIO_PORT, TCAN4550_GPIO_CS_N);
#else
    //CS level is high
    gpio_write(TCAN4550_GPIO_CS_N,1);
#endif
}


