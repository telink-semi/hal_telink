/*
 * TCAN4550.c
 * Description: This file contains TCAN4550 functions, and relies on the TCAN4x5x_SPI abstraction functions
 * Additional Feature Sets of TCAN4550 vs TCAN4x5x:
 *  - Watchdog Timer Functions
 *
 *  Version: 1.2.2
 *  Date: 6/8/2019
 *
 *  Change Log
 *
 *  1.2.2(6/8/2020)
 *      - Corrected incorrect mask value for REG_BITS_DEVICE_IE_MASK (bit 31 is now 0)
 *  1.2.1 (9/19/2019)
 *      - Added a missing AHB_BURST_READ_END() to the TCAN4x5x_MCAN_ReadXIDFilter function, which caused the next read or write to fail
 *
 *  1.2.0 (5/1/2019)
 *      - Added the MCAN_ConfigureGlobalFilter function for changing default packet behavior
 *      - Added a define to allow caching of MCAN configuration registers to reduce the number of SPI reads
 *      - Added a FIFO fill level checker to the MCAN_ReadNextFIFO method to exit if there is no new element to read
 *      - Added a read function for SID and XID filters
 *      - Added a SPIERR clear function
 *      
 *  1.1.1 (6/12/2018)
 *      - Minor typo correction for the ConfigureNominalTiming_Simple() function
 *
 *  1.1 (6/6/2018)
 *      - Updates to code for readability, and consistency
 *      - Some function names updated for consistency
 *      - Added functionality and abstraction for interrupts
 *      - Bit fields updated for final silicon
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
#include "vendor/common/sniffer_common/sniffer_common.h"
#include "TCAN4550.h"
#include "TCAN4x5x_Data_Structs.h"
#include "../node_config.h"


#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
// If caching is enabled, define the necessary variables
uint32_t TCAN4x5x_MCAN_CACHE[9];
#endif

#define CAN_RX_BUFFER_LEN_MAX 64
u8 can_rx_buffer[CAN_RX_BUFFER_LEN_MAX] = {0};
u8 can_tx_buffer[CAN_RX_BUFFER_LEN_MAX] = {0};

_attribute_ble_data_retention_ volatile u32 tcan_reset_flag = 0;

#if 0
// Initialize to 0 or you'll get garbage
TCAN4x5x_MCAN_RX_Header MsgHeader = {0};
u8 numBytes = 0;
u8 dataPayload[64] = {0};
uint8_t  msg_rx_Data[8] = {0, 0, 0, 0, 0, 0, 0, 0};
uint8_t  msg_rx_DLC;
uint32_t msg_rx_ID;
#endif

#define OPCODE_WRITE     0x61
#define OPCODE_READ      0x41

#define RX0_NUM_ELEMENTS 16
#define TX_FOIFO_NUM     8
#define FILTER_ID_NUMBER 1

const u16 fSID[FILTER_ID_NUMBER] =
    {
        ECU_TO_MAIN_NODE_REQ_SID,
#if FILTER_ID_NUMBER != 1
        SUB_NODE_TO_MAIN_NODE_DATA_SID_0,
        SUB_NODE_TO_MAIN_NODE_DATA_SID_1,
        SUB_NODE_TO_MAIN_NODE_DATA_SID_2,
        SUB_NODE_TO_MAIN_NODE_DATA_SID_3,
        SUB_NODE_TO_MAIN_NODE_RSP_SID
#endif
};
const u32 fXID[FILTER_ID_NUMBER] =
    {
        ECU_TO_MAIN_NODE_REQ_SID,
#if FILTER_ID_NUMBER != 1
        SUB_NODE_TO_MAIN_NODE_DATA_SID_0,
        SUB_NODE_TO_MAIN_NODE_DATA_SID_1,
        SUB_NODE_TO_MAIN_NODE_DATA_SID_2,
        SUB_NODE_TO_MAIN_NODE_DATA_SID_3,
        SUB_NODE_TO_MAIN_NODE_RSP_SID
#endif
};

_attribute_ram_code_sec_noinline_ void gpio_irq_handler(void)
{
#if (APP_CAN_PM_ENABLE)
    if (gpio_read(TCAN4550_GPIO_WKREQ_N) == 0) {
        extern unsigned char can_wake_up_flag;
        can_wake_up_flag = 1;
    }
#endif
    gpio_clr_irq_status(FLD_GPIO_IRQ_CLR);
}
PLIC_ISR_REGISTER(gpio_irq_handler, IRQ_GPIO)

void tcan4550_gpio_init(void)
{
    /* RST: The RST pin is a device reset pin. It has a weak internal pull-down
     * resistor for normal operation. If communication has stopped with the
     * TCAN4550-Q1, the RST pin can be pulsed high and then back low for greater
     * than t PULSE_WIDTH(30us) to perform a power on reset to the device.
     */
    gpio_function_en(TCAN4550_GPIO_RST); //enable gpio
    gpio_output_en(TCAN4550_GPIO_RST);   //enable output
    gpio_input_dis(TCAN4550_GPIO_RST);   //disable input
    gpio_write(TCAN4550_GPIO_RST, 0);

    //nWKRQ, input floating
    gpio_function_en(TCAN4550_GPIO_WKREQ_N);
    gpio_set_output_en(TCAN4550_GPIO_WKREQ_N, 0);
    gpio_set_input_en(TCAN4550_GPIO_WKREQ_N, 1);
    gpio_setup_up_down_resistor(TCAN4550_GPIO_WKREQ_N, PM_PIN_UP_DOWN_FLOAT);

#if (APP_CAN_PM_ENABLE)
    /* RISING_EDGE      low-high -> sleep
     * FALLING_EDGE     high-low -> wake-up
     * */
    gpio_set_irq(TCAN4550_GPIO_WKREQ_N, INTR_FALLING_EDGE);
    plic_interrupt_enable(IRQ_GPIO);
#endif

#if TCAN4550_RX_INTERRUPT_ENABLE
    //nINT, nput pullup
    gpio_set_func(TCAN4550_GPIO_INT_N, AS_GPIO);
    gpio_set_output_en(TCAN4550_GPIO_INT_N, 0);
    gpio_set_input_en(TCAN4550_GPIO_INT_N, 1);
    gpio_setup_up_down_resistor(TCAN4550_GPIO_INT_N, PM_PIN_PULLUP_10K);
    gpio_set_interrupt(TCAN4550_GPIO_INT_N, POL_FALLING);
#else
    gpio_function_en(TCAN4550_GPIO_INT_N);
    gpio_set_output_en(TCAN4550_GPIO_INT_N, 0);
    gpio_set_input_en(TCAN4550_GPIO_INT_N, 1);
    gpio_setup_up_down_resistor(TCAN4550_GPIO_INT_N, PM_PIN_PULLUP_10K);
#endif

#if 0
    //INH I
    gpio_function_en(TCAN4550_GPIO_INH );
    gpio_set_output_en(TCAN4550_GPIO_INH, 0);
    gpio_set_input_en(TCAN4550_GPIO_INH ,1);
#endif

    //WAKE, output high level
    //WAKE, input floating
    gpio_function_en(TCAN4550_GPIO_WAKE);
    gpio_set_output_en(TCAN4550_GPIO_WAKE, 0);
    gpio_set_input_en(TCAN4550_GPIO_WAKE, 1);
    gpio_setup_up_down_resistor(TCAN4550_GPIO_WAKE, PM_PIN_PULLUP_10K);
    //gpio_write(TCAN4550_GPIO_RST, 1);

#if (BOARD_SELECT == BOARD_9223A_EVK_C1T289A67_V1_0)
    //GPIO1, input floating
    gpio_function_en(TCAN4550_GPIO_GPIO1);
    gpio_set_output_en(TCAN4550_GPIO_GPIO1, 0);
    gpio_set_input_en(TCAN4550_GPIO_GPIO1, 1);
    gpio_setup_up_down_resistor(TCAN4550_GPIO_GPIO1, PM_PIN_UP_DOWN_FLOAT);
#endif

#if (BOARD_SELECT == BOARD_9223B_EVK_C1T325A20_V1_0)

#elif (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A102)

#else
    //GPIO2, input floating
    gpio_function_en(TCAN4550_GPIO_GPIO2);
    gpio_set_output_en(TCAN4550_GPIO_GPIO2, 0);
    gpio_set_input_en(TCAN4550_GPIO_GPIO2, 1);
    gpio_setup_up_down_resistor(TCAN4550_GPIO_GPIO2, PM_PIN_UP_DOWN_FLOAT);
#endif
}

void tcan4550_reset_hw(void)
{
    gpio_function_en(TCAN4550_GPIO_RST); //enable gpio
    gpio_output_en(TCAN4550_GPIO_RST);   //enable output
    gpio_input_dis(TCAN4550_GPIO_RST);   //disable input

    gpio_write(TCAN4550_GPIO_RST, 1);
    sleep_us(50);                        // must greater than 30us
    gpio_write(TCAN4550_GPIO_RST, 0);
    sleep_ms(1);                         // must greater than 700us

    gpio_function_en(TCAN4550_GPIO_RST); //enable gpio
    gpio_output_dis(TCAN4550_GPIO_RST);  //disable output
    gpio_input_en(TCAN4550_GPIO_RST);    //enable input
    gpio_setup_up_down_resistor(TCAN4550_GPIO_RST, PM_PIN_PULLDOWN_100K);
    gpio_write(TCAN4550_GPIO_RST, 0);
}

void tcan4550_read_id(void)
{
    __attribute__((aligned(4))) u8 ptr[8] = {0}; //SUCCESS: 'NACT0554'
    __attribute__((aligned(4))) u8 reg[4] = {AHB_READ_OPCODE, REG_SPI_DEVICE_ID0 >> 8, REG_SPI_DEVICE_ID0 & 0xFF, 2};

    can_spi_write_read(reg, 4, ptr, 8);
    tlkapi_printf(APP_CAN_LOG_EN, "[APP][CAN] TCAN4550_ID:%c%c%c%c%c%c%c%c\n", ptr[3], ptr[2], ptr[1], ptr[0], ptr[7], ptr[6], ptr[5], ptr[4]);
}

void tcan4550_readReg_t(void)
{
    u32 reg = 0;

    reg = AHB_READ_32(REG_DEV_IR);
    tlkapi_printf(APP_CAN_LOG_EN, "dev_ir  0820 %08X\n", reg);

    reg = AHB_READ_32(REG_MCAN_IR);
    tlkapi_printf(APP_CAN_LOG_EN, "mcan_ir 1050 %08X\n", reg);

    reg = AHB_READ_32(REG_MCAN_ECR);
    tlkapi_printf(APP_CAN_LOG_EN, "ecr     1040 %08X\n", reg);

    reg = AHB_READ_32(REG_MCAN_PSR);
    tlkapi_printf(APP_CAN_LOG_EN, "psr     1044 %08X\n", reg);
}

void tcan4550_readREG0800(void)
{
    u32 reg = AHB_READ_32(REG_DEV_MODES_AND_PINS);
    tlkapi_printf(APP_CAN_LOG_EN, "REG_DEV_MODES_AND_PINS = %08X\n", reg);
}

void tcan4550_isr(void)
{
    TCAN4x5x_Device_Interrupts dev_ir;  // Define a new Device IR object for device (non-CAN) interrupt checking
    TCAN4x5x_MCAN_Interrupts   mcan_ir; // Setup a new MCAN IR object for easy interrupt checking
    memset((u8 *)&dev_ir, 0, sizeof(TCAN4x5x_Device_Interrupts));
    memset((u8 *)&mcan_ir, 0, sizeof(TCAN4x5x_MCAN_Interrupts));

    TCAN4x5x_Device_ReadInterrupts(&dev_ir); // Read the device interrupt register
    TCAN4x5x_MCAN_ReadInterrupts(&mcan_ir);  // Read the interrupt register

    //tlkapi_printf(APP_CAN_LOG_EN,"int: %08X, %08X\n",dev_ir.word, mcan_ir.word);
#if 0
    TCAN4x5x_Device_ClearInterrupts(&dev_ir); // 0820 0830
    TCAN4x5x_MCAN_ClearInterrupts(&mcan_ir);  // 1050 0824
#else
    TCAN4x5x_Device_ClearInterruptsAll();
    TCAN4x5x_MCAN_ClearInterruptsAll();
#endif

#if 1
    if (dev_ir.SPIERR)                 // If the SPIERR flag is set
    {
        TCAN4x5x_Device_ClearSPIERR(); // Clear the SPIERR flag
    }
#endif

#if 1
    if (mcan_ir.BO) // reset tcan4550 when BUS_OFF.
    {
        tcan_reset_flag = 1;
        return;
    }
#endif

#if 1
    /* If a new message in RX FIFO 0
     *   - RX FIFO 0 new message interrupt enable.
     *   - Rx FIFO 0 watermark reached.
     *   - Rx FIFO 0 full interrupt enable.
     *   - Bus_off status changed.
     *   - Rx FIFO 0 message lost.
     *   - High priority message.
     * */
    u32 int_flag = mcan_ir.RF0N || mcan_ir.RF0W || mcan_ir.RF0F ||
                   /*mcan_ir.BO||*/ mcan_ir.RF0L || mcan_ir.HPM;
#else
    u32 int_flag = mcan_ir.RF0N || mcan_ir.RF0F || mcan_ir.RF0L || mcan_ir.HPM ||
                   mcan_ir.MRAF || mcan_ir.BEC || mcan_ir.BEU || mcan_ir.ELO ||
                   mcan_ir.EP || mcan_ir.EW || mcan_ir.BO || mcan_ir.WDI ||
                   mcan_ir.PEA || mcan_ir.PED;
#endif

    if (int_flag) {
        DBG_SNIF_CHN15_HIGH;
        u16                     frameNum        = 0;
        u8                      numBytes        = 0;   // Used since the ReadNextFIFO function will return how many bytes of data were read
        u8                      dataPayload[64] = {0}; // Used to store the received data
        TCAN4x5x_MCAN_RX_Header MsgHeader       = {0}; // Initialize to 0 or you'll get garbage


        frameNum = AHB_READ_32(REG_MCAN_RXF0S) & 0x0000007F;
        //tlkapi_printf(APP_CAN_LOG_EN, "framNum = %d\n", framNum);
        if (frameNum == 0) {
            return;
        }

        for (int i = 0; i < frameNum; i++) {
            MsgHeader.ID = 0;

            /* This will read the next element in the RX FIFO 0 */
            numBytes = TCAN4x5x_MCAN_ReadNextFIFO(RXFIFO0, &MsgHeader, dataPayload);

            /* numBytes will have the number of bytes it transferred in it. Or you can decode the DLC value in MsgHeader.DLC
             * The data is now in dataPayload[], and message specific information is in the MsgHeader struct.
             * Example of how you can do an action based off a received address
             */
            if (MsgHeader.ID == MAIN_NODE_TO_SUB_NODE_SYNC_SID) {
                extern void canfd_rxdata_handle(u8 * data, u8 len);
                canfd_rxdata_handle((u8 *)dataPayload, numBytes);
            }
        }
        DBG_SNIF_CHN15_LOW;
    }
}

volatile u8 can_txfifo_index = 0;

int can_fd_data_send(u16 sid, u8 *pData, u32 len)
{
    u32 reg = AHB_READ_32(REG_MCAN_TXFQS);
    if (reg & BIT(21)) {
        /* Tx FIFO/Queue full */
        return -1;
    }

    /* Define the CAN message we want to send*/
    TCAN4x5x_MCAN_TX_Header header = {0}; // Remember to initialize to 0, or you'll get random garbage!

    memset((u8 *)can_tx_buffer, 0, sizeof(can_tx_buffer));
    blc_app_memory_copy((u8 *)can_tx_buffer, pData, len, sizeof(can_tx_buffer), 0x22240000 | __LINE__);
    /* Set the DLC to be equal to or less than the data payload (it is ok to pass
     * a 64 byte data array into the WriteTXFIFO function if your DLC is 8 bytes,
     * only the first 8 bytes will be read)
     */
    if (len <= 8) {
        header.DLC = TCAN4x5x_MCAN_DLCtoBytes(len);
    } else if (len <= 12) {
        header.DLC = MCAN_DLC_12B;
    } else if (len <= 16) {
        header.DLC = MCAN_DLC_16B;
    } else if (len <= 20) {
        header.DLC = MCAN_DLC_20B;
    } else if (len <= 24) {
        header.DLC = MCAN_DLC_24B;
    } else if (len <= 32) {
        header.DLC = MCAN_DLC_32B;
    } else if (len <= 48) {
        header.DLC = MCAN_DLC_48B;
    } else {
        header.DLC = MCAN_DLC_64B;
    }

    //header.DLC = TCAN4x5x_MCAN_DLCtoBytes(len);
    header.ID  = sid;
    header.FDF = 1; // CAN FD frame enabled
#if DATA_FIELD_RATE == RATE_500K
    header.BRS = 0;
#else
    header.BRS = 1; // Bit rate switch enabled
#endif
    header.EFC = 0;
    header.MM  = 0;
    header.RTR = 0;
    header.XTD = 0; // We are not using an extended ID in this example
    header.ESI = 0; // Error state indicator

    u32 ret = 0;
    /* This claims to return "number of bytes read" but I think that's a copy-paste mistake
    * At time of writing (using TCAN45xx driver rev B), the return value is:
    * 0 on failure,
    * 1 << bufIndex (second parameter) on success
    */

    // This line writes the data and header to TX FIFO
    ret = TCAN4x5x_MCAN_WriteTXBuffer(can_txfifo_index, &header, (u8 *)can_tx_buffer);
    if (ret == 0) {
        tlkapi_printf(APP_CAN_LOG_EN, "failed to write TX buffer\n");
        return -2;
    }
    // Request that TX Buffer  be transmitted
    ret = TCAN4x5x_MCAN_TransmitBufferContents(can_txfifo_index);

    if (ret == 0) {
        tlkapi_printf(APP_CAN_LOG_EN, "failed to transmit TX buffer contents");
        return -3;
    }
    can_txfifo_index++;
    if (can_txfifo_index >= TX_FOIFO_NUM) {
        can_txfifo_index = 0;
    }
    return 0;
}

void can_test(void)
{
    static u8 datalen = 1;
    uint8_t   data[64];

    for (int i = 0; i < 64; i++) {
        data[i] = i;
    }

    can_fd_data_send(0x777, data, datalen);
    datalen++;
    if (datalen > 64) {
        datalen = 1;
    }
}

void tcan4550_init(void)
{
    tcan4550_spi_init();

    tcan4550_gpio_init();

    tcan4550_reset_hw();

    tcan4550_read_id();

    Init_CAN();
}

/*
 * Configure the TCAN4550
 * Everything at this point is for TCAN4550
 * Run the main MCAN configuration sequence. The bulk of the configuration is in this!
 */
void Init_CAN(void)
{
    TCAN4x5x_Device_ClearSPIERR(); // Clear any SPI ERR flags that might be set as a result of our pin mux changing during MCU startup

    /* Step one attempt to clear all interrupts */
    TCAN4x5x_Device_Interrupt_Enable dev_ie;                           // Initialize to 0 to all bits are set to 0.
    memset((u8 *)&dev_ie, 0, sizeof(TCAN4x5x_Device_Interrupt_Enable));
    bool retValue = TCAN4x5x_Device_ConfigureInterruptEnable(&dev_ie); // Disable all non-MCAN related interrupts for simplicity
    if (retValue == FALSE) {
        /* 830: interrupt enable/disable */
        tlkapi_printf(APP_CAN_LOG_EN, "[APP][CAN] Clear all interrupts error, %s, %d\n", __FUNCTION__, __LINE__);
        return;
    }

    TCAN4x5x_Device_Interrupts dev_ir;       // Setup a new MCAN IR object for easy interrupt checking
    memset((u8 *)&dev_ir, 0, sizeof(TCAN4x5x_Device_Interrupts));
    TCAN4x5x_Device_ReadInterrupts(&dev_ir); // Request that the struct be updated with current DEVICE (not MCAN) interrupt values
    /* 820: interrupt state */
    //tlkapi_printf(APP_CAN_LOG_EN, "[APP][CAN] Read device interrupts: %08X, %s, %d\n", dev_ir.word, __FUNCTION__, __LINE__);
    if (dev_ir.PWRON) {                           // If the Power On interrupt flag is set
        TCAN4x5x_Device_ClearInterrupts(&dev_ir); // Clear it because if it's not cleared within ~4 minutes, it goes to sleep
    }

    /* Configure the CAN bus speeds */
    TCAN4x5x_MCAN_Nominal_Timing_Simple TCANNomTiming;     // 500k arbitration with a 40 MHz crystal ((40E6 / 2) / (32 + 8) = 500E3)
    memset((u8 *)&TCANNomTiming, 0, sizeof(TCAN4x5x_MCAN_Nominal_Timing_Simple));
    TCANNomTiming.NominalBitRatePrescaler    = 2;
    TCANNomTiming.NominalTqBeforeSamplePoint = 32;         // SS + PTS + PBS1
    TCANNomTiming.NominalTqAfterSamplePoint  = 8;          // PBS2
#if DATA_FIELD_RATE == RATE_4M
    TCAN4x5x_MCAN_Data_Timing_Simple TCANDataTiming = {0}; // 4 Mbps CAN FD with a 40 MHz crystal (40E6 / (8 + 2) = 4E6)
    TCANDataTiming.DataBitRatePrescaler             = 1;
    TCANDataTiming.DataTqBeforeSamplePoint          = 8;
    TCANDataTiming.DataTqAfterSamplePoint           = 2;
#elif DATA_FIELD_RATE == RATE_5M
    TCAN4x5x_MCAN_Data_Timing_Simple TCANDataTiming = {0}; // 5 Mbps CAN FD with a 40 MHz crystal (40E6 / (6 + 2) = 5E6)
    TCANDataTiming.DataBitRatePrescaler             = 1;
    TCANDataTiming.DataTqBeforeSamplePoint          = 6;
    TCANDataTiming.DataTqAfterSamplePoint           = 2;
#elif DATA_FIELD_RATE == RATE_2M
    TCAN4x5x_MCAN_Data_Timing_Simple TCANDataTiming = {0}; // 2 Mbps CAN FD with a 40 MHz crystal (40E6 / (15 + 5) = 2E6)
    TCANDataTiming.DataBitRatePrescaler             = 1;
    TCANDataTiming.DataTqBeforeSamplePoint          = 16;
    TCANDataTiming.DataTqAfterSamplePoint           = 4;
#elif DATA_FIELD_RATE == RATE_1M
    TCAN4x5x_MCAN_Data_Timing_Simple TCANDataTiming = {0}; // 1 Mbps CAN FD with a 40 MHz crystal (40E6 / 2 / (15 + 5) = 1E6)
    TCANDataTiming.DataBitRatePrescaler             = 2;
    TCANDataTiming.DataTqBeforeSamplePoint          = 15;
    TCANDataTiming.DataTqAfterSamplePoint           = 5;
#elif DATA_FIELD_RATE == RATE_500K
    TCAN4x5x_MCAN_Data_Timing_Simple TCANDataTiming = {0}; // 500 Kbps CAN FD with a 40 MHz crystal (40E6 / 4 / (15 + 5) = 5E3)
    TCANDataTiming.DataBitRatePrescaler             = 4;
    TCANDataTiming.DataTqBeforeSamplePoint          = 15;
    TCANDataTiming.DataTqAfterSamplePoint           = 5;
#endif

    /* Configure the MCAN core settings */
    TCAN4x5x_MCAN_CCCR_Config cccrConfig; // Remember to initialize to 0, or you'll get random garbage!
    memset((u8 *)&cccrConfig, 0, sizeof(TCAN4x5x_MCAN_CCCR_Config));
    cccrConfig.FDOE = 1;                  // CAN FD mode enable
#if DATA_FIELD_RATE == RATE_500K
    cccrConfig.BRSE = 0;
#else
    cccrConfig.BRSE = 1; // CAN FD Bit rate switch enable
#endif

    /* Configure the default CAN packet filtering settings */
    TCAN4x5x_MCAN_Global_Filter_Configuration gfc;
    memset((u8 *)&gfc, 0, sizeof(TCAN4x5x_MCAN_Global_Filter_Configuration));
    gfc.RRFE = 1;                   // Reject remote frames (TCAN4x5x doesn't support this)
    gfc.RRFS = 1;                   // Reject remote frames (TCAN4x5x doesn't support this)
    gfc.ANFE = TCAN4x5x_GFC_REJECT; // Default behavior if incoming message doesn't match a filter is to accept into RXFIO0 for extended ID messages (29 bit IDs)
    gfc.ANFS = TCAN4x5x_GFC_REJECT; // Default behavior if incoming message doesn't match a filter is to accept into RXFIO0 for standard ID messages (11 bit IDs)

    /* ************************************************************************
     * In the next configuration block, we will set the MCAN core up to have:
     *   - 1 SID filter element
     *   - 1 XID Filter element
     *   - 5 RX FIFO 0 elements
     *   - RX FIFO 0 supports data payloads up to 64 bytes
     *   - RX FIFO 1 and RX Buffer will not have any elements, but we still set their data payload sizes, even though it's not required
     *   - No TX Event FIFOs
     *   - 2 Transmit buffers supporting up to 64 bytes of data payload
     */
    TCAN4x5x_MRAM_Config MRAMConfiguration = {0};
    MRAMConfiguration.SIDNumElements       = FILTER_ID_NUMBER;    // Standard ID number of elements, you MUST have a filter written to MRAM for each element defined
    MRAMConfiguration.XIDNumElements       = FILTER_ID_NUMBER;    // Extended ID number of elements, you MUST have a filter written to MRAM for each element defined

    MRAMConfiguration.Rx0NumElements = RX0_NUM_ELEMENTS;          // RX0 Number of elements
    MRAMConfiguration.Rx0ElementSize = MRAM_64_Byte_Data;         // RX0 data payload size
    MRAMConfiguration.Rx1NumElements = 0;                         // RX1 number of elements
    MRAMConfiguration.Rx1ElementSize = MRAM_64_Byte_Data;         // RX1 data payload size

    MRAMConfiguration.RxBufNumElements = 0;                       // RX buffer number of elements
    MRAMConfiguration.RxBufElementSize = MRAM_64_Byte_Data;       // RX buffer data payload size

    MRAMConfiguration.TxEventFIFONumElements = 0;                 // TX Event FIFO number of elements
    MRAMConfiguration.TxBufferNumElements    = TX_FOIFO_NUM;      // TX buffer number of elements
    MRAMConfiguration.TxBufferElementSize    = MRAM_64_Byte_Data; // TX buffer data payload size

    /* Configure the MCAN core with the settings above, the changes in this block are write protected registers,      *
     * so it makes the most sense to do them all at once, so we only unlock and lock once                             */

    TCAN4x5x_MCAN_EnableProtectedRegisters();                    // Start by making protected registers accessible
    TCAN4x5x_MCAN_ConfigureCCCRRegister(&cccrConfig);            // Enable FD mode and Bit rate switching
    TCAN4x5x_MCAN_ConfigureGlobalFilter(&gfc);                   // Configure the global filter configuration (Default CAN message behavior)
    TCAN4x5x_MCAN_ConfigureNominalTiming_Simple(&TCANNomTiming); // Setup nominal/arbitration bit timing
    TCAN4x5x_MCAN_ConfigureDataTiming_Simple(&TCANDataTiming);   // Setup CAN FD timing
    TCAN4x5x_MRAM_Clear();                                       // Clear all of MRAM (Writes 0's to all of it)
    TCAN4x5x_MRAM_Configure(&MRAMConfiguration);                 // Set up the applicable registers related to MRAM configuration
    TCAN4x5x_MCAN_DisableProtectedRegisters();                   // Disable protected write and take device out of INIT mode

#if 1
    /* Set the interrupts we want to enable for MCAN */
    TCAN4x5x_MCAN_Interrupt_Enable mcan_ie;           // Remember to initialize to 0, or you'll get random garbage!
    memset((u8 *)&mcan_ie, 0, sizeof(TCAN4x5x_MCAN_Interrupt_Enable));
    mcan_ie.RF0NE = 1;                                // RX FIFO 0 new message interrupt enable
    mcan_ie.RF0WE = 1;                                // Rx FIFO 0 watermark reached
    mcan_ie.RF0FE = 1;                                // Rx FIFO 0 full interrupt enable
    mcan_ie.BOE   = 1;                                // Bus_off status changed
    mcan_ie.RF0LE = 1;                                // Rx FIFO 0 message lost
    mcan_ie.HPME  = 1;                                // High priority message
    #if 0
    mcan_ie.MRAFE = 1;                                          // Message RAM access failure
    mcan_ie.BECE  = 1;                                          // MRAM Bit error corrected
    mcan_ie.BEUE  = 1;                                          // MRAM Bit error uncorrected
    mcan_ie.ELOE  = 1;                                          // Error logging overflow
    mcan_ie.EPE   = 1;                                          // Error_passive status changed
    mcan_ie.EWE   = 1;                                          // Error_warning status changed
    mcan_ie.BOE   = 1;                                          // Bus_off status changed
    mcan_ie.WDIE  = 1;                                          // MRAM Watchdog Interrupt
    mcan_ie.PEAE  = 1;                                          // Protocol Error in arbitration phase (nominal bit time used)
    mcan_ie.PEDE  = 1;                                          // Protocol error in data phase (data bit time is used)
    #endif
    TCAN4x5x_MCAN_ConfigureInterruptEnable(&mcan_ie); // Enable the appropriate registers
#endif

    /* Setup filters, this filter will mark any message with ID 0x055 as a priority message */
    TCAN4x5x_MCAN_SID_Filter SID_ID;
    memset((u8 *)&SID_ID, 0, sizeof(TCAN4x5x_MCAN_SID_Filter));
    SID_ID.SFT  = TCAN4x5x_SID_SFT_CLASSIC;           // SFT: Standard filter type. Configured as a classic filter
    SID_ID.SFEC = TCAN4x5x_SID_SFEC_PRIORITYSTORERX0; // Standard filter element configuration, store it in RX fifo 0 as a priority message

#if 0                                                 /* Sniffer main node */
    #if FILTER_ID_NUMBER != 1
    for(int i=0; i<FILTER_ID_NUMBER; i++){
        SID_ID.SFID1 = fSID[i];
        SID_ID.SFID2 = 0x7FF;
        TCAN4x5x_MCAN_WriteSIDFilter(i, &SID_ID);
    }
    #else
    SID_ID.SFID1 = SUB_NODE_TO_MAIN_NODE_DATA_SID_1;            // SFID1 (Classic mode Filter)
    SID_ID.SFID2 = 0x788;
    TCAN4x5x_MCAN_WriteSIDFilter(0, &SID_ID);                   // Write to the MRAM
    #endif
#else                                         /* Sniffer sub node */
    SID_ID.SFID1 = MAIN_NODE_TO_SUB_NODE_SYNC_SID;
    SID_ID.SFID2 = 0x7FF;                     // SFID2 (Classic mode Mask)
    TCAN4x5x_MCAN_WriteSIDFilter(0, &SID_ID); // Write to the MRAM
#endif

    /* Store ID 0x12345678 as a priority message */
    TCAN4x5x_MCAN_XID_Filter XID_ID = {0};
    XID_ID.EFT                      = TCAN4x5x_XID_EFT_CLASSIC;           // EFT
    XID_ID.EFEC                     = TCAN4x5x_XID_EFEC_PRIORITYSTORERX0; // EFEC
#if FILTER_ID_NUMBER != 1
    for (int i = 0; i < FILTER_ID_NUMBER; i++) {
        XID_ID.EFID1 = fXID[i];                                           // EFID1 (Classic mode filter)
        XID_ID.EFID2 = 0x1FFFFFFF;                                        // EFID2 (Classic mode mask)
        TCAN4x5x_MCAN_WriteXIDFilter(i, &XID_ID);                         // Write to the MRAM
    }
#else
    XID_ID.EFID1 = 0x12345678;                // EFID1 (Classic mode filter)
    XID_ID.EFID2 = 0x1FFFFFFF;                // EFID2 (Classic mode mask)
    TCAN4x5x_MCAN_WriteXIDFilter(0, &XID_ID); // Write to the MRAM
#endif

    /* Configure the TCAN4550 Non-CAN-related functions */
    TCAN4x5x_DEV_CONFIG devConfig;                                     // Remember to initialize to 0, or you'll get random garbage!
    memset((u8 *)&devConfig, 0, sizeof(TCAN4x5x_DEV_CONFIG));
    devConfig.SWE_DIS          = 0;                                    // Keep Sleep Wake Error Enabled (it's a disable bit, not an enable)
    devConfig.DEVICE_RESET     = 0;                                    // Not requesting a software reset
    devConfig.WD_EN            = 0;                                    // Watchdog disabled
    devConfig.nWKRQ_CONFIG     = 0;                                    // Mirror INH function (default)
    devConfig.INH_DIS          = 0;                                    // INH enabled (default)
    devConfig.GPIO1_GPO_CONFIG = TCAN4x5x_DEV_CONFIG_GPO1_MCAN_INT1;   // MCAN nINT 1 (default)
    devConfig.FAIL_SAFE_EN     = 0;                                    // Failsafe disabled (default)
    devConfig.GPIO1_CONFIG     = TCAN4x5x_DEV_CONFIG_GPIO1_CONFIG_GPO; // GPIO set as GPO (Default)
    devConfig.WD_ACTION        = TCAN4x5x_DEV_CONFIG_WDT_ACTION_nINT;  // Watchdog set an interrupt (default)
    devConfig.WD_BIT_RESET     = 0;                                    // Don't reset the watchdog
    devConfig.nWKRQ_VOLTAGE    = 0;                                    // Set nWKRQ to internal voltage rail (default)
    devConfig.GPO2_CONFIG      = TCAN4x5x_DEV_CONFIG_GPO2_NO_ACTION;   // GPO2 has no behavior (default)
    devConfig.CLK_REF          = 1;                                    // Input crystal is a 40 MHz crystal (default)
    devConfig.WAKE_CONFIG      = TCAN4x5x_DEV_CONFIG_WAKE_BOTH_EDGES;  // Wake pin can be triggered by either edge (default)
    TCAN4x5x_Device_Configure(&devConfig);                             // Configure the device with the above configuration

    TCAN4x5x_Device_SetMode(TCAN4x5x_DEVICE_MODE_NORMAL);              // Set to normal mode, since configuration is done. This line turns on the transceiver

    TCAN4x5x_MCAN_ClearInterruptsAll();                                // Resets all MCAN interrupts (does NOT include any SPIERR interrupts)
}

/**
 * @brief Enable Protected MCAN Registers
 *
 * Attempts to enable CCCR.CCE and CCCR.INIT to allow writes to protected registers, needed for MCAN configuration
 *
 * @return @c TRUE if successfully enabled, otherwise return @c FALSE
 */
bool TCAN4x5x_MCAN_EnableProtectedRegisters(void)
{
    uint8_t  i;
    uint32_t readValue, firstRead;

    firstRead = AHB_READ_32(REG_MCAN_CCCR);
    if ((firstRead & (REG_BITS_MCAN_CCCR_CCE | REG_BITS_MCAN_CCCR_INIT)) == (REG_BITS_MCAN_CCCR_CCE | REG_BITS_MCAN_CCCR_INIT)) {
        return TRUE;
    }

    // Unset the CSA and CSR bits since those will be set if we're in standby mode. Writing a 1 to these bits will force a clock stop event and prevent the return to normal mode
    firstRead &= ~(REG_BITS_MCAN_CCCR_CSA | REG_BITS_MCAN_CCCR_CSR | REG_BITS_MCAN_CCCR_CCE | REG_BITS_MCAN_CCCR_INIT);
    // Try up to 5 times to set the CCCR register, if not, then fail config, since we need these bits set to configure the device.
    for (i = 5; i > 0; i--) {
        AHB_WRITE_32(REG_MCAN_CCCR, firstRead | REG_BITS_MCAN_CCCR_CCE | REG_BITS_MCAN_CCCR_INIT);
        readValue = AHB_READ_32(REG_MCAN_CCCR);

        if ((readValue & (REG_BITS_MCAN_CCCR_CCE | REG_BITS_MCAN_CCCR_INIT)) == (REG_BITS_MCAN_CCCR_CCE | REG_BITS_MCAN_CCCR_INIT)) {
            return TRUE;
        } else if (i == 1) { // Ran out of tries, give up
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * @brief Disable Protected MCAN Registers
 *
 * Attempts to disable CCCR.CCE and CCCR.INIT to disallow writes to protected registers
 *
 * @return @c TRUE if successfully enabled, otherwise return @c FALSE
 */
bool TCAN4x5x_MCAN_DisableProtectedRegisters(void)
{
    uint8_t  i;
    uint32_t readValue;

    readValue = AHB_READ_32(REG_MCAN_CCCR);
    if ((readValue & REG_BITS_MCAN_CCCR_CCE) == 0) {
        return TRUE;
    }

    // Try up to 5 times to unset the CCCR register, if not, then fail config, since we need these bits set to configure the device.
    for (i = 5; i > 0; i--) {
        AHB_WRITE_32(REG_MCAN_CCCR, (readValue & ~(REG_BITS_MCAN_CCCR_CSA | REG_BITS_MCAN_CCCR_CSR | REG_BITS_MCAN_CCCR_CCE | REG_BITS_MCAN_CCCR_INIT))); // Unset these bits
        readValue = AHB_READ_32(REG_MCAN_CCCR);

        if ((readValue & REG_BITS_MCAN_CCCR_CCE) == 0) {
            return TRUE;
        } else if (i == 1) {
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * @brief Configure the MCAN CCCR Register
 *
 * Configures the bits of the CCCR register to match the CCCR config struct
 *
 * @warning This function writes to protected MCAN registers
 * @note Requires that protected registers have been unlocked using @c TCAN4x5x_MCAN_EnableProtectedRegisters() and @c TCAN4x5x_MCAN_DisableProtectedRegisters() be used to lock the registers after configuration
 *
 * @param *cccrConfig is a pointer to a @c TCAN4x5x_MCAN_CCCR_Config struct containing the configuration bits
 *
 * @return @c TRUE if successfully enabled, otherwise return @c FALSE
 */
bool TCAN4x5x_MCAN_ConfigureCCCRRegister(TCAN4x5x_MCAN_CCCR_Config *cccrConfig)
{
    uint32_t value, readValue;


    value = cccrConfig->word;
    value &= ~(REG_BITS_MCAN_CCCR_RESERVED_MASK | REG_BITS_MCAN_CCCR_CSA | REG_BITS_MCAN_CCCR_CCE | REG_BITS_MCAN_CCCR_INIT); // Bitwise AND to get the valid bits (ignore reserved bits and the CCE and INIT)

    // If we made it here, we can update the value so that our protected write stays enabled
    value |= (REG_BITS_MCAN_CCCR_INIT | REG_BITS_MCAN_CCCR_CCE);


    AHB_WRITE_32(REG_MCAN_CCCR, value);
#ifdef TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES
    readValue = AHB_READ_32(REG_MCAN_CCCR);

    // Need to do these bitwise ANDs to make this work for clock stop requests and not trigger a FALSE failure when comparing read back value
    if ((readValue & ~(REG_BITS_MCAN_CCCR_RESERVED_MASK | REG_BITS_MCAN_CCCR_CSA | REG_BITS_MCAN_CCCR_CSR | REG_BITS_MCAN_CCCR_CCE | REG_BITS_MCAN_CCCR_INIT)) != (value & ~(REG_BITS_MCAN_CCCR_RESERVED_MASK | REG_BITS_MCAN_CCCR_CSA | REG_BITS_MCAN_CCCR_CSR | REG_BITS_MCAN_CCCR_CCE | REG_BITS_MCAN_CCCR_INIT))) {
        // If our written value and read back value aren't the same, then we return a failure.
        return FALSE;
    }

    // Check to see if the CSR bits are not as expected, since this can be set by the hardware.
    if ((readValue & REG_BITS_MCAN_CCCR_CSR) != cccrConfig->CSR) {
        // Then there's a difference in the CSR bits, which may not be a failure.
        if (TCAN4x5x_Device_ReadMode() == TCAN4x5x_DEVICE_MODE_STANDBY) {
            // CSR bit is set due to being in standby mode. Not a failure.
            return TRUE;
        } else {
            // It's not matching for some other reason, we've got a real failure.
            return FALSE;
        }
    }
#endif
    return TRUE;
}

/**
 * @brief Read the MCAN CCCR configuration register
 *
 * Reads the MCAN CCCR configuration register and updates the passed @c TCAN4x5x_MCAN_CCCR_Config struct
 *
 * @param *cccrConfig is a pointer to a @c TCAN4x5x_MCAN_CCCR_Config struct containing the CCCR bit fields that will be updated
 */
void TCAN4x5x_MCAN_ReadCCCRRegister(TCAN4x5x_MCAN_CCCR_Config *cccrConfig)
{
    cccrConfig->word = AHB_READ_32(REG_MCAN_CCCR);
}

/**
 * @brief Reads the MCAN data time settings, using the simple struct
 *
 * Reads the MCAN data timing registers and updates the @c *dataTiming struct
 *
 * @warning This function writes to protected MCAN registers
 * @note Requires that protected registers have been unlocked using @c TCAN4x5x_MCAN_EnableProtectedRegisters() and @c TCAN4x5x_MCAN_DisableProtectedRegisters() be used to lock the registers after configuration
 *
 * @param *dataTiming is a pointer of a @c TCAN4x5x_MCAN_Data_Timing_Simple struct containing the simplified data timing information
 */
void TCAN4x5x_MCAN_ReadDataTimingFD_Simple(TCAN4x5x_MCAN_Data_Timing_Simple *dataTiming)
{
    uint32_t regData;

    // Read the data timing register
    regData = AHB_READ_32(REG_MCAN_DBTP);

    // These registers are only writable if CCE and INIT are both set. Sets the nominal bit timing and prescaler information
    dataTiming->DataBitRatePrescaler    = ((regData >> 16) & 0x1F) + 1;
    dataTiming->DataTqBeforeSamplePoint = ((regData >> 8) & 0x1F) + 2;
    dataTiming->DataTqAfterSamplePoint  = ((regData >> 4) & 0xF) + 1;
}

/**
 * @brief Reads the MCAN data time settings, using the raw MCAN struct
 *
 * Reads the MCAN data timing registers and updates the @c *dataTiming struct
 *
 * @param *dataTiming is a pointer of a @c TCAN4x5x_MCAN_Data_Timing_Simple struct containing the raw data timing information
 */
void TCAN4x5x_MCAN_ReadDataTimingFD_Raw(TCAN4x5x_MCAN_Data_Timing_Raw *dataTiming)
{
    uint32_t regData;

    // Read the data timing register
    regData = AHB_READ_32(REG_MCAN_DBTP);

    // These registers are only writable if CCE and INIT are both set. Sets the nominal bit timing and prescaler information
    dataTiming->DataBitRatePrescaler = ((regData >> 16) & 0x1F);
    dataTiming->DataTimeSeg1andProp  = ((regData >> 8) & 0x1F);
    dataTiming->DataTimeSeg2         = ((regData >> 4) & 0xF);
    dataTiming->DataSyncJumpWidth    = (regData & 0xF);

    if (regData & REG_BITS_MCAN_DBTP_TDC_EN) {
        // If TDC is set, then read the TDC register
        regData               = AHB_READ_32(REG_MCAN_TDCR);
        dataTiming->TDCOffset = ((regData >> 8) & 0x7F);
        dataTiming->TDCFilter = (regData & 0x7F);
    } else {
        dataTiming->TDCOffset = 0;
        dataTiming->TDCFilter = 0;
    }
}

/**
 * @brief Writes the MCAN data time settings, using the simple data timing struct
 *
 * Writes the data timing information to MCAN using the input from the @c *dataTiming pointer
 *
 * @warning This function writes to protected MCAN registers
 * @note Requires that protected registers have been unlocked using @c TCAN4x5x_MCAN_EnableProtectedRegisters() and @c TCAN4x5x_MCAN_DisableProtectedRegisters() be used to lock the registers after configuration
 *
 * @param *dataTiming is a pointer of a @c TCAN4x5x_MCAN_Data_Timing_Simple struct containing the simplified data timing information
 * @return @c TRUE if successfully enabled, otherwise return @c FALSE
 */
bool TCAN4x5x_MCAN_ConfigureDataTiming_Simple(TCAN4x5x_MCAN_Data_Timing_Simple *dataTiming)
{
    uint32_t writeValue, TDCOWriteValue;
    uint32_t tempValue;

    // These registers are only writable if CCE and INIT are both set. Sets the nominal bit timing and prescaler information
    // Check to make sure prescaler is in range 1-32
    tempValue = dataTiming->DataBitRatePrescaler;
    if (tempValue > 32) {
        tempValue = 32;
    } else if (tempValue == 0) {
        tempValue = 1;
    }

    writeValue = ((uint32_t)(tempValue - 1)) << 16; // Subtract 1 because MCAN expects 1 less than actual value

    // Check Tq before sample point is within valid range of 2-33
    tempValue = dataTiming->DataTqBeforeSamplePoint;
    if (tempValue > 33) {
        tempValue = 33;
    } else if (tempValue < 2) {
        tempValue = 2;
    }

    writeValue |= ((uint32_t)(tempValue - 2)) << 8;  // Subtract 2 for the Sync bit and because MCAN expects 1 less than actual
    TDCOWriteValue = (uint32_t)(tempValue - 1) << 8; // Subtract 1 to make secondary sample point match primary. We take the sync bit out. See below note as to why
    // Check Tq after the sample point is within valid range of 1-16
    tempValue = dataTiming->DataTqAfterSamplePoint;
    if (tempValue > 16) {
        tempValue = 16;
    } else if (tempValue == 0) {
        tempValue = 1;
    }

    writeValue |= ((uint32_t)(tempValue - 1)) << 4; // Subtract 1 because MCAN expects 1 less than actual value

    //Copy SJW from tq after sample point in most cases
    writeValue |= ((uint32_t)(tempValue - 1)); // Subtract 1 because MCAN expects 1 less than actual value

    // NOTE: In most cases, you want to enable Transceiver Delay Compensation Offset and set it to 1 more than what's in the DTSEG1 register in MCAN.
    // Doing this ensures that the secondary sample point is the same as the primary sample point
    writeValue |= REG_BITS_MCAN_DBTP_TDC_EN;
    AHB_WRITE_32(REG_MCAN_DBTP, writeValue); // Write the value to the DBTP register

#ifdef TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES
    // Check to see if the write was successful.
    tempValue = AHB_READ_32(REG_MCAN_DBTP);
    if (tempValue != writeValue) {
        return FALSE;
    }
#endif

    AHB_WRITE_32(REG_MCAN_TDCR, TDCOWriteValue);
#ifdef TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES
    // Check to see if the write was successful.
    tempValue = AHB_READ_32(REG_MCAN_TDCR);
    if (tempValue != TDCOWriteValue) {
        return FALSE;
    }
#endif

    // Configure the Timestamp counter to use an external time stamp value. This is required to use time stamps with CAN FD
    AHB_WRITE_32(REG_MCAN_TSCC, REG_BITS_MCAN_TSCC_COUNTER_EXTERNAL);
    return TRUE;
}

/**
 * @brief Writes the MCAN data time settings, using the raw MCAN data timing struct
 *
 * Writes the data timing information to MCAN using the input from the @c *dataTiming pointer
 *
 * @warning This function writes to protected MCAN registers
 * @note Requires that protected registers have been unlocked using @c TCAN4x5x_MCAN_EnableProtectedRegisters() and @c TCAN4x5x_MCAN_DisableProtectedRegisters() be used to lock the registers after configuration
 *
 * @param *dataTiming is a pointer of a @c TCAN4x5x_MCAN_Data_Timing_Raw struct containing the raw data timing information
 *
 * @return @c TRUE if successfully enabled, otherwise return @c FALSE
 */
bool TCAN4x5x_MCAN_ConfigureDataTiming_Raw(TCAN4x5x_MCAN_Data_Timing_Raw *dataTiming)
{
    uint32_t writeValue;
#ifdef TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES
    uint32_t tempValue;
#endif

    // These registers are only writable if CCE and INIT are both set. Sets the nominal bit timing and prescaler information
    writeValue = ((uint32_t)(dataTiming->DataBitRatePrescaler & 0x1F)) << 16;
    writeValue |= ((uint32_t)(dataTiming->DataTimeSeg1andProp & 0x1F)) << 8;
    writeValue |= ((uint32_t)(dataTiming->DataTimeSeg2 & 0x0F)) << 4;
    writeValue |= ((uint32_t)(dataTiming->DataSyncJumpWidth & 0x0F));
    if ((dataTiming->TDCOffset > 0) || (dataTiming->TDCFilter > 0)) {
        // If either of these are set, then enable transmitter delay compensation
        writeValue |= REG_BITS_MCAN_DBTP_TDC_EN;
        AHB_WRITE_32(REG_MCAN_DBTP, writeValue);
#ifdef TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES
        // Check to see if the write was successful.
        tempValue = AHB_READ_32(REG_MCAN_DBTP);
        if (tempValue != writeValue) {
            return FALSE;
        }
#endif

        writeValue = (uint32_t)(dataTiming->TDCOffset & 0x7F) << 8;
        writeValue |= (uint32_t)(dataTiming->TDCFilter & 0x7F);
        AHB_WRITE_32(REG_MCAN_TDCR, writeValue);
#ifdef TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES
        // Check to see if the write was successful.
        tempValue = AHB_READ_32(REG_MCAN_TDCR);
        if (tempValue != writeValue) {
            return FALSE;
        }
#endif
    } else {
        AHB_WRITE_32(REG_MCAN_DBTP, writeValue);
#ifdef TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES
        // Check to see if the write was successful.
        tempValue = AHB_READ_32(REG_MCAN_DBTP);
        if (tempValue != writeValue) {
            return FALSE;
        }
#endif
    }

    // Configure the Timestamp counter to use an external time stamp value. This is required to use time stamps with CAN FD
    AHB_WRITE_32(REG_MCAN_TSCC, REG_BITS_MCAN_TSCC_COUNTER_EXTERNAL);

    return TRUE;
}

/**
 * @brief Reads the MCAN nominal/arbitration time settings, using the simple timing struct
 *
 * Reads the MCAN nominal timing registers and updates the @c *nomTiming struct
 *
 * @param *nomTiming is a pointer of a @c TCAN4x5x_MCAN_Nominal_Timing_Simple struct containing the simplified nominal timing information
 */
void TCAN4x5x_MCAN_ReadNominalTiming_Simple(TCAN4x5x_MCAN_Nominal_Timing_Simple *nomTiming)
{
    uint32_t readValue;

    readValue = AHB_READ_32(REG_MCAN_NBTP);

    // These registers are only writable if CCE and INIT are both set. Sets the nominal bit timing and prescaler information
    nomTiming->NominalBitRatePrescaler    = ((readValue >> 16) & 0x1FF) + 1;
    nomTiming->NominalTqBeforeSamplePoint = ((readValue >> 8) & 0xFF) + 2;
    nomTiming->NominalTqAfterSamplePoint  = (readValue & 0x7F) + 1;
}

/**
 * @brief Reads the MCAN nominal/arbitration time settings, using the raw MCAN timing struct
 *
 * Reads the MCAN nominal timing registers and updates the @c *nomTiming struct
 *
 * @param *nomTiming is a pointer of a @c TCAN4x5x_MCAN_Nominal_Timing_Raw struct containing the raw MCAN nominal timing information
 */
void TCAN4x5x_MCAN_ReadNominalTiming_Raw(TCAN4x5x_MCAN_Nominal_Timing_Raw *nomTiming)
{
    uint32_t readValue;

    readValue = AHB_READ_32(REG_MCAN_NBTP);

    // These registers are only writable if CCE and INIT are both set. Sets the nominal bit timing and prescaler information
    nomTiming->NominalSyncJumpWidth    = ((readValue >> 25) & 0x7F);
    nomTiming->NominalBitRatePrescaler = ((readValue >> 16) & 0x1FF);
    nomTiming->NominalTimeSeg1andProp  = ((readValue >> 8) & 0xFF);
    nomTiming->NominalTimeSeg2         = (readValue & 0x7F);
}

/**
 * @brief Writes the MCAN nominal timing settings, using the simple nominal timing struct
 *
 * Writes the data timing information to MCAN using the input from the @c *nomTiming pointer
 *
 * @warning This function writes to protected MCAN registers
 * @note Requires that protected registers have been unlocked using @c TCAN4x5x_MCAN_EnableProtectedRegisters() and @c TCAN4x5x_MCAN_DisableProtectedRegisters() be used to lock the registers after configuration
 *
 * @param *nomTiming is a pointer of a @c TCAN4x5x_MCAN_Nominal_Timing_Simple struct containing the simplified nominal timing information
 * @return @c TRUE if successfully enabled, otherwise return @c FALSE
 */
bool TCAN4x5x_MCAN_ConfigureNominalTiming_Simple(TCAN4x5x_MCAN_Nominal_Timing_Simple *nomTiming)
{
    uint32_t writeValue, tempValue;


    // These registers are only writable if CCE and INIT are both set. Sets the nominal bit timing and prescaler information
    // Check that prescaler is in valid range of 1-512
    tempValue = nomTiming->NominalBitRatePrescaler;
    if (tempValue > 512) {
        tempValue = 512;
    } else if (tempValue == 0) {
        tempValue = 1;
    }
    writeValue = ((uint32_t)(tempValue - 1)) << 16; // Subtract 1 because MCAN expects 1 less than actual value


    // Check that prescaler is in valid range of 2-257
    tempValue = nomTiming->NominalTqBeforeSamplePoint;
    if (tempValue > 257) {
        tempValue = 257;
    } else if (tempValue < 2) {
        tempValue = 2;
    }
    writeValue |= ((uint32_t)(tempValue - 2)) << 8; // Subtract 2, 1 for sync, and 1 because MCAN expects 1 less than actual value

    // Check that prescaler is in valid range of 2-257
    tempValue = nomTiming->NominalTqAfterSamplePoint;
    if (tempValue > 128) {
        tempValue = 128;
    } else if (tempValue < 2) {
        tempValue = 2;
    }
    writeValue |= ((uint32_t)(tempValue - 1));       // Subtract 1 because MCAN expects 1 less than actual value
    writeValue |= ((uint32_t)(tempValue - 1)) << 25; // NSJW is made to match the MCAN after bit time value

    // Write value to the NBTP register
    AHB_WRITE_32(REG_MCAN_NBTP, writeValue);
#ifdef TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES
    // Check the write was successful
    tempValue = AHB_READ_32(REG_MCAN_NBTP);
    if (tempValue != writeValue) {
        return FALSE;
    }
#endif

    return TRUE;
}

/**
 * @brief Writes the MCAN nominal timing settings, using the raw MCAN nominal timing struct
 *
 * Writes the data timing information to MCAN using the input from the @c *nomTiming pointer
 *
 * @warning This function writes to protected MCAN registers
 * @note Requires that protected registers have been unlocked using @c TCAN4x5x_MCAN_EnableProtectedRegisters() and @c TCAN4x5x_MCAN_DisableProtectedRegisters() be used to lock the registers after configuration
 *
 * @param *nomTiming is a pointer of a @c TCAN4x5x_MCAN_Nominal_Timing_Raw struct containing the raw MCAN nominal timing information
 * @return @c TRUE if successfully enabled, otherwise return @c FALSE
 */
bool TCAN4x5x_MCAN_ConfigureNominalTiming_Raw(TCAN4x5x_MCAN_Nominal_Timing_Raw *nomTiming)
{
    uint32_t writeValue;
#ifdef TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES
    uint32_t tempValue;
#endif
    // These registers are only writable if CCE and INIT are both set. Sets the nominal bit timing and prescaler information
    writeValue = ((uint32_t)(nomTiming->NominalSyncJumpWidth & 0x7F)) << 25;
    writeValue |= ((uint32_t)(nomTiming->NominalBitRatePrescaler & 0x1FF)) << 16;
    writeValue |= ((uint32_t)(nomTiming->NominalTimeSeg1andProp)) << 8;
    writeValue |= ((uint32_t)(nomTiming->NominalTimeSeg2 & 0x7F));
    AHB_WRITE_32(REG_MCAN_NBTP, writeValue);
#ifdef TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES
    // Check that the write was successful
    tempValue = AHB_READ_32(REG_MCAN_NBTP);
    if (tempValue != writeValue) {
        return FALSE;
    }
#endif
    return TRUE;
}

/**
 * @brief Configures the MCAN global filter configuration register, using the passed Global Filter Configuration struct.
 *
 * Configures the default behavior of the MCAN controller when receiving messages. This can include accepting or rejecting CAN messages by default.
 *
 * @warning This function writes to protected MCAN registers
 * @note Requires that protected registers have been unlocked using @c TCAN4x5x_MCAN_EnableProtectedRegisters() and @c TCAN4x5x_MCAN_DisableProtectedRegisters() be used to lock the registers after configuration
 *
 * @param *gfc is a pointer of a @c TCAN4x5x_MCAN_Global_Filter_Configuration struct containing the register values
 * @return @c TRUE if successfully enabled, otherwise return @c FALSE
 */
bool TCAN4x5x_MCAN_ConfigureGlobalFilter(TCAN4x5x_MCAN_Global_Filter_Configuration *gfc)
{
    uint32_t writeValue, readValue;


    writeValue = (gfc->word & REG_BITS_MCAN_GFC_MASK);
    AHB_WRITE_32(REG_MCAN_GFC, writeValue);

#ifdef TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES
    readValue = AHB_READ_32(REG_MCAN_GFC);

    // Need to do these bitwise ANDs to make this work for clock stop requests and not trigger a FALSE failure when comparing read back value
    if (readValue != writeValue) {
        // If our written value and read back value aren't the same, then we return a failure.
        return FALSE;
    }
#endif
    return TRUE;
}

/**
 * @brief Configures the MRAM registers
 *
 * Uses the @c *MRAMConfig pointer to set up the various sections of the MRAM memory space.
 * There are several different elements that may be configured in the MRAM, including their number of elements, as well as size of elements.
 * This function will automatically generate the start addresses for each of the appropriate MRAM sections, attempting to place them immediately back-to-back.
 * This function will check for over allocated memory conditions, and return @c FALSE if this is found to be the case.
 *
 * @warning This function writes to protected MCAN registers
 * @note Requires that protected registers have been unlocked using @c TCAN4x5x_MCAN_EnableProtectedRegisters() and @c TCAN4x5x_MCAN_DisableProtectedRegisters() be used to lock the registers after configuration
 *
 * @param *MRAMConfig is a pointer of a @c TCAN4x5x_MRAM_Config struct containing the desired MRAM configuration
 * @return @c TRUE if successful, otherwise return @c FALSE
 */
bool TCAN4x5x_MRAM_Configure(TCAN4x5x_MRAM_Config *MRAMConfig)
{
    uint16_t startAddress  = 0x0000; // Used to hold the start and end addresses for each section as we write them into the appropriate registers
    uint32_t registerValue = 0;      // Used to create the 32-bit word to write to each register
    uint32_t readValue     = 0;
    uint8_t  MRAMValue;


    // First the 11-bit filter section can be setup.
    MRAMValue = MRAMConfig->SIDNumElements;
    if (MRAMValue > 128) {
        MRAMValue = 128;
    }

    registerValue = 0;
    if (MRAMValue > 0) {
        registerValue = ((uint32_t)(MRAMValue) << 16) | ((uint32_t)startAddress);
    }
    startAddress += (4 * (uint16_t)MRAMValue);
    AHB_WRITE_32(REG_MCAN_SIDFC, registerValue);
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
    TCAN4x5x_MCAN_CACHE[TCAN4x5x_MCAN_CACHE_SIDFC] = registerValue;
#endif
#ifdef TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES
    // Verify content of register
    readValue = AHB_READ_32(REG_MCAN_SIDFC);
    if (readValue != registerValue) {
        return FALSE;
    }
#endif


    // The 29-bit extended filter section
    MRAMValue = MRAMConfig->XIDNumElements;
    if (MRAMValue > 64) {
        MRAMValue = 64;
    }

    registerValue = 0;
    if (MRAMValue > 0) {
        registerValue = ((uint32_t)(MRAMValue) << 16) | ((uint32_t)startAddress);
    }
    startAddress += (8 * (uint16_t)MRAMValue);
    AHB_WRITE_32(REG_MCAN_XIDFC, registerValue);
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
    TCAN4x5x_MCAN_CACHE[TCAN4x5x_MCAN_CACHE_XIDFC] = registerValue;
#endif
#ifdef TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES
    // Verify content of register
    readValue = AHB_READ_32(REG_MCAN_XIDFC);
    if (readValue != registerValue) {
        return FALSE;
    }
#endif


    // RX FIFO 0
    MRAMValue = MRAMConfig->Rx0NumElements;
    if (MRAMValue > 64) {
        MRAMValue = 64;
    }

    registerValue = 0;
    if (MRAMValue > 0) {
        registerValue = ((uint32_t)(MRAMValue) << 16) | ((uint32_t)startAddress); // Write start address and the number of elements
        registerValue |= REG_BITS_MCAN_RXF0C_F0OM_OVERWRITE;                      // Also enable overwrite mode when FIFO is full
    }
    startAddress += (((uint32_t)TCAN4x5x_MCAN_TXRXESC_DataByteValue((uint8_t)MRAMConfig->Rx0ElementSize) + 8) * (uint16_t)MRAMValue);
    AHB_WRITE_32(REG_MCAN_RXF0C, registerValue);
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
    TCAN4x5x_MCAN_CACHE[TCAN4x5x_MCAN_CACHE_RXF0C] = registerValue;
#endif
#ifdef TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES
    // Verify content of register
    readValue = AHB_READ_32(REG_MCAN_RXF0C);
    if (readValue != registerValue) {
        return FALSE;
    }
#endif

    // RX FIFO 1
    MRAMValue = MRAMConfig->Rx1NumElements;
    if (MRAMValue > 64) {
        MRAMValue = 64;
    }

    registerValue = 0;
    if (MRAMValue > 0) {
        registerValue = ((uint32_t)(MRAMValue) << 16) | ((uint32_t)startAddress);
    }
    startAddress += (((uint32_t)TCAN4x5x_MCAN_TXRXESC_DataByteValue((uint8_t)MRAMConfig->Rx1ElementSize) + 8) * (uint16_t)MRAMValue);
    AHB_WRITE_32(REG_MCAN_RXF1C, registerValue);
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
    TCAN4x5x_MCAN_CACHE[TCAN4x5x_MCAN_CACHE_RXF1C] = registerValue;
#endif
#ifdef TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES
    // Verify content of register
    readValue = AHB_READ_32(REG_MCAN_RXF1C);
    if (readValue != registerValue) {
        return FALSE;
    }
#endif

    // RX Buffers
    // Since RXBuffers are a little weird, you don't actually tell MCAN how many elements you have. Instead, you tell it indirectly through filters.
    // For example, you would have to setup a filter to tell it which value to go to
    MRAMValue = MRAMConfig->RxBufNumElements;
    if (MRAMValue > 64) {
        MRAMValue = 64;
    }

    registerValue = 0;
    if (MRAMValue > 0) {
        registerValue = ((uint32_t)startAddress);
    }
    startAddress += (((uint32_t)TCAN4x5x_MCAN_TXRXESC_DataByteValue((uint8_t)MRAMConfig->RxBufElementSize) + 8) * (uint16_t)MRAMValue);
    AHB_WRITE_32(REG_MCAN_RXBC, registerValue);
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
    TCAN4x5x_MCAN_CACHE[TCAN4x5x_MCAN_CACHE_RXBC] = registerValue;
#endif
#ifdef TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES
    // Verify content of register
    readValue = AHB_READ_32(REG_MCAN_RXBC);

    if (readValue != registerValue) {
        return FALSE;
    }
#endif

    // TX Event FIFO
    MRAMValue = MRAMConfig->TxEventFIFONumElements;
    if (MRAMValue > 32) {
        MRAMValue = 32;
    }

    registerValue = 0;
    if (MRAMValue > 0) {
        registerValue = ((uint32_t)(MRAMValue) << 16) | ((uint32_t)startAddress);
    }
    startAddress += (8 * (uint16_t)MRAMValue);
    AHB_WRITE_32(REG_MCAN_TXEFC, registerValue);
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
    TCAN4x5x_MCAN_CACHE[TCAN4x5x_MCAN_CACHE_TXEFC] = registerValue;
#endif
#ifdef TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES
    // Verify content of register
    readValue = AHB_READ_32(REG_MCAN_TXEFC);
    if (readValue != registerValue) {
        return FALSE;
    }
#endif

    // TX Buffer
    MRAMValue = MRAMConfig->TxBufferNumElements;
    if (MRAMValue > 32) {
        MRAMValue = 32;
    }


    registerValue = 0;
    if (MRAMValue > 0) {
        registerValue = ((uint32_t)(MRAMValue) << 24) | ((uint32_t)startAddress);
        //registerValue |= REG_BITS_MCAN_TXBC_TFQM;               // Sets TFQM to 1 (Queue mode), and sets all registers to be generic non-dedicated buffers.
    }
    startAddress += (((uint32_t)TCAN4x5x_MCAN_TXRXESC_DataByteValue((uint8_t)MRAMConfig->TxBufferElementSize) + 8) * (uint16_t)MRAMValue);
    AHB_WRITE_32(REG_MCAN_TXBC, registerValue);
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
    TCAN4x5x_MCAN_CACHE[TCAN4x5x_MCAN_CACHE_TXBC] = registerValue;
#endif
#ifdef TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES
    // Verify content of register
    readValue = AHB_READ_32(REG_MCAN_TXBC);
    if (readValue != registerValue) {
        return FALSE;
    }
#endif


    // Check and make sure we did not go out of memory bounds. If it is, return fail
    if ((startAddress - 1) > (MRAM_SIZE + REG_MRAM)) {
        return FALSE;
    }

    // Set the RX Element Size Register
    registerValue = ((uint32_t)(MRAMConfig->RxBufElementSize) << 8) | ((uint32_t)(MRAMConfig->Rx1ElementSize) << 4) | (uint32_t)(MRAMConfig->Rx0ElementSize);
    AHB_WRITE_32(REG_MCAN_RXESC, registerValue);
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
    TCAN4x5x_MCAN_CACHE[TCAN4x5x_MCAN_CACHE_RXESC] = registerValue;
#endif
#ifdef TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES
    // Verify content of register
    readValue = AHB_READ_32(REG_MCAN_RXESC);
    if (readValue != registerValue) {
        return FALSE;
    }
#endif


    // Set the TX Element Size Register
    registerValue = (uint32_t)(MRAMConfig->TxBufferElementSize);
    AHB_WRITE_32(REG_MCAN_TXESC, registerValue);
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
    TCAN4x5x_MCAN_CACHE[TCAN4x5x_MCAN_CACHE_TXESC] = registerValue;
#endif
#ifdef TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES
    // Verify content of register
    readValue = AHB_READ_32(REG_MCAN_TXESC);
    if (readValue != registerValue) {
        return FALSE;
    }
#endif

    return TRUE;
}

/**
 * @brief Clear (Zero-fill) the contents of MRAM
 *
 * Write 0s to every address in MRAM. Useful for initializing the MRAM to known values during initial configuration so that accidental ECC errors do not happen
 */
void TCAN4x5x_MRAM_Clear(void)
{
    uint16_t       curAddr;
    const uint16_t endAddr = REG_MRAM + MRAM_SIZE;

    // Need to write 0's to the entire MRAM
    curAddr = REG_MRAM;

    while (curAddr < endAddr) {
        AHB_WRITE_32(curAddr, 0);
        curAddr += 4;
    }
}

/**
 * @brief Read the next MCAN FIFO element
 *
 * This function will read the next MCAN FIFO element specified and return the corresponding header information and data payload.
 * The start address of the elment is automatically calculated by looking at the MCAN's register that says where the next element to read exists.
 *
 * @param FIFODefine is an @c TCAN4x5x_MCAN_FIFO_Enum enum corresponding to either RXFIFO0 or RXFIFO1
 * @param *header is a pointer to a @c TCAN4x5x_MCAN_RX_Header struct containing the CAN-specific header information
 * @param dataPayload[] is a byte array that will be updated with the read data
 *
 * @warning @c dataPayload[] must be at least as big as the largest possible data payload, otherwise writing to out of bounds memory may occur
 *
 * @return the number of bytes that were read from the TCAN4x5x and stored into @c dataPayload[]
 */
__attribute__((aligned(4))) uint8_t can_rxBuffer[100] = {0};

uint8_t
    TCAN4x5x_MCAN_ReadNextFIFO(TCAN4x5x_MCAN_FIFO_Enum FIFODefine, TCAN4x5x_MCAN_RX_Header *header, uint8_t dataPayload[])
{
    uint32_t readData;
    uint16_t startAddress;
    uint8_t  i = 0, getIndex, elementSize;

    // Get the get buffer location and size, depending on the source type
    switch (FIFODefine) {
    default: // RXFIFO0 is default
    {
        readData = AHB_READ_32(REG_MCAN_RXF0S);
        if ((readData & 0x0000007F) == 0) {
            return 0;
        }
        getIndex = (uint8_t)((readData & 0x3F00) >> 8);
        // Get the RX 0 Start location and size...
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
        readData = TCAN4x5x_MCAN_CACHE[TCAN4x5x_MCAN_CACHE_RXF0C];
#else
        readData = AHB_READ_32(REG_MCAN_RXF0C);
#endif
        startAddress = (uint16_t)(readData & 0x0000FFFF) + REG_MRAM;
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
        readData = TCAN4x5x_MCAN_CACHE[TCAN4x5x_MCAN_CACHE_RXESC];
#else
        readData = AHB_READ_32(REG_MCAN_RXESC);
#endif
        readData &= 0x07;
        elementSize = TCAN4x5x_MCAN_TXRXESC_DataByteValue(readData); // Maximum theoretical data payload supported by this MCAN configuration
        // Calculate the actual start address for the latest index
        startAddress += (((uint32_t)elementSize + 8) * getIndex);
        break;
    }

    case RXFIFO1:
    {
        readData = AHB_READ_32(REG_MCAN_RXF1S);
        if ((readData & 0x0000007F) == 0) {
            return 0;
        }
        getIndex = (uint8_t)((readData & 0x3F00) >> 8);
        // Get the RX 1 Start location and size...
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
        readData = TCAN4x5x_MCAN_CACHE[TCAN4x5x_MCAN_CACHE_RXF1C];
#else
        readData = AHB_READ_32(REG_MCAN_RXF1C);
#endif
        startAddress = (uint16_t)(readData & 0x0000FFFF) + REG_MRAM;
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
        readData = TCAN4x5x_MCAN_CACHE[TCAN4x5x_MCAN_CACHE_RXESC];
#else
        readData = AHB_READ_32(REG_MCAN_RXESC);
#endif
        readData    = (readData & 0x70) >> 4;
        elementSize = TCAN4x5x_MCAN_TXRXESC_DataByteValue(readData); // Maximum theoretical data payload supported by this MCAN configuration
        // Calculate the actual start address for the latest index
        startAddress += (((uint32_t)elementSize + 8) * getIndex);
        break;
    }
    }

#if 0
    // Read the data, start with a burst read
    AHB_READ_BURST_START(startAddress, 2);
    readData = AHB_READ_BURST_READ(); // First header
    header->ESI = (readData & 0x80000000) >> 31;
    header->XTD = (readData & 0x40000000) >> 30;
    header->RTR = (readData & 0x20000000) >> 29;

    if (header->XTD)
        header->ID  = (readData & 0x1FFFFFFF);
    else
        header->ID  = (readData & 0x1FFC0000) >> 18;

    readData = AHB_READ_BURST_READ();   // Second header
    AHB_READ_BURST_END();               // Terminate the burst read
    header->RXTS    = (readData & 0x0000FFFF);
    header->DLC     = (readData & 0x000F0000) >> 16;
    header->BRS     = (readData & 0x00100000) >> 20;
    header->FDF     = (readData & 0x00200000) >> 21;
    header->FIDX    = (readData & 0x7F000000) >> 24;
    header->ANMF    = (readData & 0x80000000) >> 31;

    // Get the actual data
    // If the data payload size of the header is smaller than the maximum we can store, then update the new element size to read only what we need to (prevents accidental overflow reading)
    if (TCAN4x5x_MCAN_DLCtoBytes(header->DLC) < elementSize )
        elementSize = TCAN4x5x_MCAN_DLCtoBytes(header->DLC); // Returns the number of data bytes

    // Start a burst read for the number of data bytes we require at the data payload area of the MRAM
    // The equation below ensures that we will always read the correct number of words since the divide truncates any remainders, and we need a ceil()-like function
    if (elementSize > 0) {
        AHB_READ_BURST_START(startAddress + 8, (elementSize + 3) >> 2);
        i = 0;  // Used to count the number of bytes we have read.
        while (i < elementSize) {
            if ((i % 4) == 0) {
                readData = AHB_READ_BURST_READ();
            }

            dataPayload[i] = (uint8_t)((readData >> ((i % 4) * 8)) & 0xFF);
            i++;
            if (i > elementSize)
                i = elementSize;
        }
        AHB_READ_BURST_END(); // Terminate the burst read
    }
#else
    __attribute__((aligned(4))) u8 reg1[4] = {AHB_READ_OPCODE, startAddress >> 8, startAddress & 0xFF, 2};
    __attribute__((aligned(4))) u8 data[8] = {0};

    can_spi_write_read(reg1, 4, data, 8);

    readData = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];

    header->ESI = (readData & 0x80000000) >> 31;
    header->XTD = (readData & 0x40000000) >> 30;
    header->RTR = (readData & 0x20000000) >> 29;

    if (header->XTD) {
        header->ID = (readData & 0x1FFFFFFF);
    } else {
        header->ID = (readData & 0x1FFC0000) >> 18;
    }

    readData = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];

    header->RXTS = (readData & 0x0000FFFF);
    header->DLC  = (readData & 0x000F0000) >> 16;
    header->BRS  = (readData & 0x00100000) >> 20;
    header->FDF  = (readData & 0x00200000) >> 21;
    header->FIDX = (readData & 0x7F000000) >> 24;
    header->ANMF = (readData & 0x80000000) >> 31;

    // Get the actual data
    // If the data payload size of the header is smaller than the maximum we can store, then update the new element size to read only what we need to (prevents accidental overflow reading)
    if (TCAN4x5x_MCAN_DLCtoBytes(header->DLC) < elementSize) {
        elementSize = TCAN4x5x_MCAN_DLCtoBytes(header->DLC); // Returns the number of data bytes
    }

    // Start a burst read for the number of data bytes we require at the data payload area of the MRAM
    // The equation below ensures that we will always read the correct number of words since the divide truncates any remainders, and we need a ceil()-like function
    if (elementSize > 0) {
        __attribute__((aligned(4))) u8 reg2[4] = {AHB_READ_OPCODE, (startAddress + 8) >> 8, (startAddress + 8) & 0xFF, (elementSize + 3) >> 2};

        can_spi_write_read(reg2, 4, can_rxBuffer+4, elementSize);

        u32 *p   = (u32 *)(&can_rxBuffer[4]);
        u8   num = (elementSize + 3) >> 2;
        for (u8 j = 0; j < num; j++) {
            dataPayload[j * 4]     = p[j] >> 24;
            dataPayload[j * 4 + 1] = p[j] >> 16;
            dataPayload[j * 4 + 2] = p[j] >> 8;
            dataPayload[j * 4 + 3] = p[j] & 0xFF;
        }

        i = elementSize;
    }
#endif

    // Acknowledge the FIFO read
    switch (FIFODefine) {
    default: // RXFIFO0
        AHB_WRITE_32(REG_MCAN_RXF0A, getIndex);
        break;

    case RXFIFO1:
        AHB_WRITE_32(REG_MCAN_RXF1A, getIndex);
        break;
    }


    return i; // Return the number of bytes retrieved
}

/**
 * @brief Read the specified RX buffer element
 *
 * This function will read the specified MCAN buffer element and return the corresponding header information and data payload.
 * The start address of the element is automatically calculated.
 *
 * @param bufIndex is the RX buffer index to read from (starts at 0)
 * @param *header is a pointer to a @c TCAN4x5x_MCAN_RX_Header struct containing the CAN-specific header information
 * @param dataPayload[] is a byte array that will be updated with the read data
 *
 * @warning @c dataPayload[] must be at least as big as the largest possible data payload, otherwise writing to out of bounds memory may occur
 *
 * @return the number of bytes that were read from the TCAN4x5x and stored into @c dataPayload[]
 */
uint8_t
    TCAN4x5x_MCAN_ReadRXBuffer(uint8_t bufIndex, TCAN4x5x_MCAN_RX_Header *header, uint8_t dataPayload[])
{
    uint32_t readData;
    uint16_t startAddress;
    uint8_t  i = 0, getIndex, elementSize;

    // Get the get buffer location and size
    getIndex = bufIndex;
    if (getIndex > 64) {
        getIndex = 64;
    }

    // Get the RX Buffer Start location and size...
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
    readData = TCAN4x5x_MCAN_CACHE[TCAN4x5x_MCAN_CACHE_RXBC];
#else
    readData = AHB_READ_32(REG_MCAN_RXBC);
#endif
    startAddress = (uint16_t)(readData & 0x0000FFFF) + REG_MRAM;
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
    readData = TCAN4x5x_MCAN_CACHE[TCAN4x5x_MCAN_CACHE_RXESC];
#else
    readData = AHB_READ_32(REG_MCAN_RXESC);
#endif
    readData    = (readData & 0x0700) >> 8;
    elementSize = TCAN4x5x_MCAN_TXRXESC_DataByteValue(readData); // Maximum theoretical data payload supported by this MCAN configuration
    // Calculate the actual start address for the latest index
    startAddress += (((uint32_t)elementSize + 8) * getIndex);


    // Read the data, start with a burst read
    AHB_READ_BURST_START(startAddress, 2);
    readData    = AHB_READ_BURST_READ(); // First header
    header->ESI = (readData & 0x80000000) >> 31;
    header->XTD = (readData & 0x40000000) >> 30;
    header->RTR = (readData & 0x20000000) >> 29;

    if (header->XTD) {
        header->ID = (readData & 0x1FFFFFFF);
    } else {
        header->ID = (readData & 0x1FFC0000) >> 18;
    }

    readData = AHB_READ_BURST_READ(); // Second header
    AHB_READ_BURST_END();             // Terminate the burst read
    header->RXTS = (readData & 0x0000FFFF);
    header->DLC  = (readData & 0x000F0000) >> 16;
    header->BRS  = (readData & 0x00100000) >> 20;
    header->FDF  = (readData & 0x00200000) >> 21;
    header->FIDX = (readData & 0x7F000000) >> 24;
    header->ANMF = (readData & 0x80000000) >> 31;

    // Get the actual data
    // If the data payload size of the header is smaller than the maximum we can store, then update the new element size to read only what we need to (prevents accidentical overflow reading)
    if (TCAN4x5x_MCAN_DLCtoBytes(header->DLC) < elementSize) {
        elementSize = TCAN4x5x_MCAN_DLCtoBytes(header->DLC); // Returns the number of data bytes
    }

    // Start a burst read for the number of data bytes we require at the data payload area of the MRAM
    // The equation below ensures that we will always read the correct number of words since the divide truncates any remainders, and we need a ceil()-like function
    if (elementSize > 0) {
        AHB_READ_BURST_START(startAddress + 8, (elementSize + 3) >> 2);
        i = 0; // Used to count the number of bytes we have read.
        while (i < elementSize) {
            if ((i % 4) == 0) {
                readData = AHB_READ_BURST_READ();
            }

            dataPayload[i] = (uint8_t)((readData >> ((i % 4) * 8)) & 0xFF);
            i++;
            if (i > elementSize) {
                i = elementSize;
            }
        }
        AHB_READ_BURST_END(); // Terminate the burst read
    }
    // Acknowledge the FIFO read
    if (getIndex < 32) {
        AHB_WRITE_32(REG_MCAN_NDAT1, 1 << getIndex);
    } else {
        AHB_WRITE_32(REG_MCAN_NDAT2, 1 << (getIndex - 32));
    }


    return i; // Return the number of bytes retrieved
}

/**
 * @brief Write CAN message to the specified TX buffer
 *
 * This function will write a CAN message to a specified TX buffer that can be transmitted at a later time with the @c TCAN4x5x_MCAN_TransmitBufferContents() function
 *
 * @param bufIndex is the TX buffer index to write to (starts at 0)
 * @param *header is a pointer to a @c TCAN4x5x_MCAN_TX_Header struct containing the CAN-specific header information
 * @param dataPayload[] is a byte array that contains the data payload
 *
 * @warning @c dataPayload[] must be at least as big as the specified DLC size inside the @c *header struct
 *
 * @return the number of bytes that were read from the TCAN4x5x and stored into @c dataPayload[]
 */
__attribute__((aligned(4))) u8 can_TxBuffer[100] = {0};

uint32_t
    TCAN4x5x_MCAN_WriteTXBuffer(uint8_t bufIndex, TCAN4x5x_MCAN_TX_Header *header, uint8_t dataPayload[])
{
    // Step 1: Get the start address of the
    uint32_t SPIData;
    uint16_t startAddress;
    uint8_t  i, elementSize, temp;

    //tlkapi_printf(APP_CAN_LOG_EN, "step1\n");
    // Get the TX Start location and size...
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
    SPIData = TCAN4x5x_MCAN_CACHE[TCAN4x5x_MCAN_CACHE_TXBC];
#else
    SPIData = AHB_READ_32(REG_MCAN_TXBC);
#endif
    startAddress = (uint16_t)(SPIData & 0x0000FFFF) + 0x8000;
    // Transmit FIFO and queue numbers
    temp        = (uint8_t)((SPIData >> 24) & 0x3F);
    elementSize = temp > 32 ? 32 : temp;
    // Dedicated transmit buffers
    temp = (uint8_t)((SPIData >> 16) & 0x3F);
    elementSize += temp > 32 ? 32 : temp;

    if (bufIndex > (elementSize - 1)) {
        return 0;
    }
    //tlkapi_printf(APP_CAN_LOG_EN, "step2\n");
    // Get the actual element size of each TX element
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
    SPIData = TCAN4x5x_MCAN_CACHE[TCAN4x5x_MCAN_CACHE_TXESC];
#else
    SPIData = AHB_READ_32(REG_MCAN_TXESC);
#endif
    elementSize = TCAN4x5x_MCAN_TXRXESC_DataByteValue(SPIData & 0x07) + 8;

    // Calculate the actual start address for the latest index
    startAddress += ((uint32_t)elementSize * bufIndex);

    // Now we need to actually check how much data we are writing (because we don't need to fill a 64-byte FIFO if we are sending an 8 byte can packet)
    elementSize = (TCAN4x5x_MCAN_DLCtoBytes(header->DLC & 0x0F) + 8) >> 2; // Convert it to words for easier reading by dividing by 4, and only look at data payload
    if (TCAN4x5x_MCAN_DLCtoBytes(header->DLC & 0x0F) % 4) {                // If we don't have a whole word worth of data... We need to round up to the nearest word (by default it truncates). Can be done by simply adding another word.
        elementSize += 1;
    }
#if 0
    // Read the data, start with a burst read
    AHB_WRITE_BURST_START(startAddress, elementSize);
    SPIData = 0;

    SPIData         |= ((uint32_t)header->ESI & 0x01) << 31;
    SPIData         |= ((uint32_t)header->XTD & 0x01) << 30;
    SPIData         |= ((uint32_t)header->RTR & 0x01) << 29;

    if (header->XTD)
        SPIData     |= ((uint32_t)header->ID & 0x1FFFFFFF);
    else
        SPIData     |= ((uint32_t)header->ID & 0x07FF) << 18;

    AHB_WRITE_BURST_WRITE(SPIData);

    SPIData = 0;
    SPIData         |= ((uint32_t)header->DLC & 0x0F) << 16;
    SPIData         |= ((uint32_t)header->BRS & 0x01) << 20;
    SPIData         |= ((uint32_t)header->FDF & 0x01) << 21;
    SPIData         |= ((uint32_t)header->EFC & 0x01) << 23;
    SPIData         |= ((uint32_t)header->MM & 0xFF) << 24;
    AHB_WRITE_BURST_WRITE(SPIData);

    // Get the actual data
    elementSize = TCAN4x5x_MCAN_DLCtoBytes(header->DLC & 0x0F); // Returns the number of data bytes
    i = 0;  // Used to count the number of bytes we have read.
    while (i < elementSize) {
        SPIData = 0;
        // If elementSize - i < 4, then this means we are on our last word, with a word that is less than 4 bytes long
        if ((elementSize - i) < 4) {
            while (i < elementSize)
            {
                SPIData |= ((uint32_t)dataPayload[i] << ((i % 4) * 8));
                i++;
            }

            AHB_WRITE_BURST_WRITE(SPIData);
        } else {
            SPIData |= ((uint32_t)dataPayload[i++]);
            SPIData |= ((uint32_t)dataPayload[i++]) << 8;
            SPIData |= ((uint32_t)dataPayload[i++]) << 16;
            SPIData |= ((uint32_t)dataPayload[i++]) << 24;

            AHB_WRITE_BURST_WRITE(SPIData);
        }

        if (i > elementSize)
            i = elementSize;
    }
    AHB_WRITE_BURST_END();              // Terminate the burst read
#else
    can_TxBuffer[0] = AHB_WRITE_OPCODE;
    can_TxBuffer[1] = startAddress >> 8;
    can_TxBuffer[2] = startAddress & 0xFF;
    can_TxBuffer[3] = elementSize;
    SPIData         = 0;
    SPIData |= ((uint32_t)header->ESI & 0x01) << 31;
    SPIData |= ((uint32_t)header->XTD & 0x01) << 30;
    SPIData |= ((uint32_t)header->RTR & 0x01) << 29;

    if (header->XTD) {
        SPIData |= ((uint32_t)header->ID & 0x1FFFFFFF);
    } else {
        SPIData |= ((uint32_t)header->ID & 0x07FF) << 18;
    }
    can_TxBuffer[4] = SPIData >> 24;
    can_TxBuffer[5] = SPIData >> 16;
    can_TxBuffer[6] = SPIData >> 8;
    can_TxBuffer[7] = SPIData;

    SPIData = 0;
    SPIData |= ((uint32_t)header->DLC & 0x0F) << 16;
    SPIData |= ((uint32_t)header->BRS & 0x01) << 20;
    SPIData |= ((uint32_t)header->FDF & 0x01) << 21;
    SPIData |= ((uint32_t)header->EFC & 0x01) << 23;
    SPIData |= ((uint32_t)header->MM & 0xFF) << 24;

    can_TxBuffer[8]  = SPIData >> 24;
    can_TxBuffer[9]  = SPIData >> 16;
    can_TxBuffer[10] = SPIData >> 8;
    can_TxBuffer[11] = SPIData;
    u8 index         = 12;
    //spi_master_write(SPI_MODULE_SEL,can_TxBuffer, 12);

    // Get the actual data
    elementSize = TCAN4x5x_MCAN_DLCtoBytes(header->DLC & 0x0F); // Returns the number of data bytes
    i           = 0;                                            // Used to count the number of bytes we have read.
    while (i < elementSize) {
        SPIData = 0;
        // If elementSize - i < 4, then this means we are on our last word, with a word that is less than 4 bytes long
        if ((elementSize - i) < 4) {
            while (i < elementSize) {
                SPIData |= ((uint32_t)dataPayload[i] << ((i % 4) * 8));
                i++;
            }

            can_TxBuffer[index++] = SPIData >> 24;
            can_TxBuffer[index++] = SPIData >> 16;
            can_TxBuffer[index++] = SPIData >> 8;
            can_TxBuffer[index++] = SPIData & 0xFF;
        } else {
            SPIData |= ((uint32_t)dataPayload[i++]);
            SPIData |= ((uint32_t)dataPayload[i++]) << 8;
            SPIData |= ((uint32_t)dataPayload[i++]) << 16;
            SPIData |= ((uint32_t)dataPayload[i++]) << 24;

            can_TxBuffer[index++] = SPIData >> 24;
            can_TxBuffer[index++] = SPIData >> 16;
            can_TxBuffer[index++] = SPIData >> 8;
            can_TxBuffer[index++] = SPIData & 0xFF;
        }

        if (i > elementSize) {
            i = elementSize;
        }
    }
    //tlkapi_printf(APP_CAN_LOG_EN, "step3\n");
    can_spi_write(can_TxBuffer, i + 12);
    //AHB_WRITE_BURST_END();              // Terminate the burst read
    //tlkapi_printf(APP_CAN_LOG_EN, "step4\n");
#endif
    return (uint32_t)1 << bufIndex; // Return the number of bytes retrieved
}

/**
 * @brief Transmit TX buffer contents of the specified tx buffer
 *
 * Writes the specified buffer index bit value into the TXBAR register to request a message to send
 *
 * @param bufIndex is the TX buffer index to write to (starts at 0)
 *
 * @warning Function does NOT check if the buffer contents are valid
 *
 * @return @c TRUE if the request was queued, @c FALSE if the buffer value was invalid (out of range)
 */
bool TCAN4x5x_MCAN_TransmitBufferContents(uint8_t bufIndex)
{
    uint32_t writeValue;
    uint8_t  requestedBuf = bufIndex;

    if (requestedBuf > 31) {
        return FALSE;
    }

    writeValue = 1 << requestedBuf;

    AHB_WRITE_32(REG_MCAN_TXBAR, writeValue);
    return TRUE;
}

/**
 * @brief Write MCAN Standard ID filter into MRAM
 *
 * This function will write a standard ID MCAN filter to a specified filter element
 *
 * @param filterIndex is the SID filter index in MRAM to write to (starts at 0)
 * @param *filter is a pointer to a @c TCAN4x5x_MCAN_SID_Filter struct containing the MCAN filter information
 *
 * @return @c TRUE if write was successful, @c FALSE if not
 */
bool TCAN4x5x_MCAN_WriteSIDFilter(uint8_t filterIndex, TCAN4x5x_MCAN_SID_Filter *filter)
{
    uint32_t readData;
    uint16_t startAddress;
    uint8_t  getIndex;
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
    readData = TCAN4x5x_MCAN_CACHE[TCAN4x5x_MCAN_CACHE_SIDFC];
#else
    readData = AHB_READ_32(REG_MCAN_SIDFC);
#endif
    getIndex = (readData & 0x00FF0000) >> 16;
    if (filterIndex > getIndex) { // Check if the fifo number is valid and within range. If not, then fail
        return FALSE;
    } else {
        getIndex = filterIndex;
    }

    startAddress = (uint16_t)(readData & 0x0000FFFF) + REG_MRAM;
    // Calculate the actual start address for the latest index
    startAddress += (getIndex << 2);          // Multiply by 4 and add to start address

    AHB_WRITE_32(startAddress, filter->word); // Write the value to the register
#ifdef TCAN4x5x_DEVICE_VERIFY_CONFIGURATION_WRITES
    // Verify that write was successful
    readData = AHB_READ_32(startAddress);
    if (readData != filter->word) {
        return FALSE;
    }
#endif
    return TRUE;
}

/**
 * @brief Read a MCAN Standard ID filter from MRAM
 *
 * This function will read a standard ID MCAN filter from a specified filter element
 *
 * @param filterIndex is the SID filter index in MRAM to read from (starts at 0)
 * @param *filter is a pointer to a @c TCAN4x5x_MCAN_SID_Filter struct that will be updated with the read MCAN filter
 *
 * @return @c TRUE if read was successful, @c FALSE if not
 */
bool TCAN4x5x_MCAN_ReadSIDFilter(uint8_t filterIndex, TCAN4x5x_MCAN_SID_Filter *filter)
{
    uint32_t readData;
    uint16_t startAddress;
    uint8_t  getIndex;
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
    readData = TCAN4x5x_MCAN_CACHE[TCAN4x5x_MCAN_CACHE_SIDFC];
#else
    readData = AHB_READ_32(REG_MCAN_SIDFC);
#endif
    getIndex = (readData & 0x00FF0000) >> 16;
    if (filterIndex > getIndex) { // Check if the fifo number is valid and within range. If not, then fail
        return FALSE;
    } else {
        getIndex = filterIndex;
    }

    startAddress = (uint16_t)(readData & 0x0000FFFF) + REG_MRAM;
    // Calculate the actual start address for the latest index
    startAddress += (getIndex << 2);          // Multiply by 4 and add to start address

    filter->word = AHB_READ_32(startAddress); // Read the value from the MRAM
    return TRUE;
}

/**
 * @brief Write MCAN Extended ID filter into MRAM
 *
 * This function will write an extended ID MCAN filter to a specified filter element
 *
 * @param filterIndex is the XID filter index in MRAM to write to (starts at 0)
 * @param *filter is a pointer to a @c TCAN4x5x_MCAN_XID_Filter struct containing the MCAN filter information
 *
 * @return @c TRUE if write was successful, @c FALSE if not
 */
bool TCAN4x5x_MCAN_WriteXIDFilter(uint8_t filterIndex, TCAN4x5x_MCAN_XID_Filter *filter)
{
    uint32_t readData, writeData;
    uint16_t startAddress;
    uint8_t  getIndex;
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
    readData = TCAN4x5x_MCAN_CACHE[TCAN4x5x_MCAN_CACHE_XIDFC];
#else
    readData = AHB_READ_32(REG_MCAN_XIDFC);
#endif
    getIndex = (readData & 0x00FF0000) >> 16;
    if (filterIndex > getIndex) { // Check if the fifo number is valid and within range. If not, then fail
        return FALSE;
    } else {
        getIndex = filterIndex;
    }

    startAddress = (uint16_t)(readData & 0x0000FFFF) + REG_MRAM;
    // Calculate the actual start address for the latest index
    startAddress += (getIndex << 3); // Multiply by 4 and add to start address

    // Write the 2 words to memory
    writeData = (uint32_t)(filter->EFEC) << 29;
    writeData |= (uint32_t)(filter->EFID1);
    AHB_WRITE_32(startAddress, writeData); // Write the value to the register
#ifdef TCAN4x5x_DEVICE_VERIFY_CONFIGURATION_WRITES
    readData = AHB_READ_32(startAddress);
    if (readData != writeData) {
        return FALSE;
    }
#endif

    startAddress += 4;
    writeData = (uint32_t)(filter->EFT) << 30;
    writeData |= (uint32_t)(filter->EFID2);
    AHB_WRITE_32(startAddress, writeData); // Write the value to the register
#ifdef TCAN4x5x_DEVICE_VERIFY_CONFIGURATION_WRITES
    readData = AHB_READ_32(startAddress);
    if (readData != writeData) {
        return FALSE;
    }
#endif

    return TRUE;
}

/**
 * @brief Read MCAN Extended ID filter from MRAM
 *
 * This function will read an extended ID MCAN filter from a specified filter element
 *
 * @param filterIndex is the XID filter index in MRAM to read from (starts at 0)
 * @param *filter is a pointer to a @c TCAN4x5x_MCAN_XID_Filter struct that will be updated with information from MRAM
 *
 * @return @c TRUE if read was successful, @c FALSE if not
 */
bool TCAN4x5x_MCAN_ReadXIDFilter(uint8_t filterIndex, TCAN4x5x_MCAN_XID_Filter *filter)
{
    uint32_t readData;
    uint16_t startAddress;
    uint8_t  getIndex;
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
    readData = TCAN4x5x_MCAN_CACHE[TCAN4x5x_MCAN_CACHE_XIDFC];
#else
    readData = AHB_READ_32(REG_MCAN_XIDFC);
#endif
    getIndex = (readData & 0x00FF0000) >> 16;
    if (filterIndex > getIndex) { // Check if the fifo number is valid and within range. If not, then fail
        return FALSE;
    } else {
        getIndex = filterIndex;
    }

    startAddress = (uint16_t)(readData & 0x0000FFFF) + REG_MRAM;
    // Calculate the actual start address for the latest index
    startAddress += (getIndex << 3);       // Multiply by 4 and add to start address

    AHB_READ_BURST_START(startAddress, 2); // Send SPI header for a burst SPI read of 2 words
    readData = AHB_READ_BURST_READ();      // Read first word from MRAM

    filter->EFEC  = (TCAN4x5x_XID_EFEC_Values)((readData >> 29) & 0x07);
    filter->EFID1 = readData & 0x1FFFFFFF;

    readData = AHB_READ_BURST_READ(); // Read second word from MRAM
    AHB_READ_BURST_END();             // Terminate the SPI transaction
    filter->EFT   = (TCAN4x5x_XID_EFT_Values)((readData >> 30) & 0x03);
    filter->EFID2 = readData & 0x1FFFFFFF;

    return TRUE;
}

/**
 * @brief Read the MCAN interrupts
 *
 * Reads the MCAN interrupts and updates a @c TCAN4x5x_MCAN_Interrupts struct that is passed to the function
 *
 * @param *ir is a pointer to a @c TCAN4x5x_MCAN_Interrupts struct containing the interrupt bit fields that will be updated
 */
void TCAN4x5x_MCAN_ReadInterrupts(TCAN4x5x_MCAN_Interrupts *ir)
{
    ir->word = AHB_READ_32(REG_MCAN_IR);
}

/**
 * @brief Clear the MCAN interrupts
 *
 * Will attempt to clear any interrupts that are marked as a '1' in the passed @c TCAN4x5x_MCAN_Interrupts struct
 *
 * @param *ir is a pointer to a @c TCAN4x5x_MCAN_Interrupts struct containing the interrupt bit fields that will be updated
 */
void TCAN4x5x_MCAN_ClearInterrupts(TCAN4x5x_MCAN_Interrupts *ir)
{
    AHB_WRITE_32(REG_MCAN_IR, ir->word);
}

/**
 * @brief Clear all MCAN interrupts
 *
 * Clears all MCAN interrupts
 */
void TCAN4x5x_MCAN_ClearInterruptsAll(void)
{
    AHB_WRITE_32(REG_MCAN_IR, 0xFFFFFFFF);
}

/**
 * @brief Read the MCAN interrupt enable register
 *
 * Reads the MCAN interrupt enable register and updates the passed @c TCAN4x5x_MCAN_Interrupt_Enable struct
 *
 * @param *ie is a pointer to a @c TCAN4x5x_MCAN_Interrupt_Enable struct containing the interrupt bit fields that will be updated
 */
void TCAN4x5x_MCAN_ReadInterruptEnable(TCAN4x5x_MCAN_Interrupt_Enable *ie)
{
    ie->word = AHB_READ_32(REG_MCAN_IE);
}

/**
 * @brief Configures the MCAN interrupt enable register
 *
 * Configures the MCAN interrupt enable register based on the passed @c TCAN4x5x_MCAN_Interrupt_Enable struct
 * Also enables MCAN interrupts out to the INT1 pin.
 *
 * @param *ie is a pointer to a @c TCAN4x5x_MCAN_Interrupt_Enable struct containing the desired enabled interrupt bits
 */
void TCAN4x5x_MCAN_ConfigureInterruptEnable(TCAN4x5x_MCAN_Interrupt_Enable *ie)
{
    AHB_WRITE_32(REG_MCAN_IE, ie->word);
    AHB_WRITE_32(REG_MCAN_ILE, REG_BITS_MCAN_ILE_EINT0); // This is necessary to enable the MCAN Int mux to the output nINT pin
}

/**
 * @brief Converts the CAN message DLC hex value to the number of bytes it corresponds to
 *
 * @param inputDLC is the DLC value from/to a CAN message struct
 * @return The number of bytes of data (0-64 bytes)
 */
uint8_t
    TCAN4x5x_MCAN_DLCtoBytes(uint8_t inputDLC)
{
    static const uint8_t lookup[7] = {12, 16, 20, 24, 32, 48, 64};

    if (inputDLC < 9) {
        return inputDLC;
    }

    if (inputDLC < 16) {
        return lookup[(unsigned int)(inputDLC - 9)];
    }

    return 0;
}

/**
 * @brief Converts the MCAN ESC (Element Size) value to number of bytes that it corresponds to
 *
 * @param inputESCValue is the value from an element size configuration register
 * @return The number of bytes of data (8-64 bytes)
 */
uint8_t
    TCAN4x5x_MCAN_TXRXESC_DataByteValue(uint8_t inputESCValue)
{
    static const uint8_t lookup[8] = {8, 12, 16, 20, 24, 32, 48, 64};
    return lookup[(unsigned int)(inputESCValue & 0x07)];
}

/* ************************************** *
 *  Start of Device (Non-MCAN) Functions  *
 * ************************************** */

/**
 * @brief Read the TCAN4x5x device version register
 *
 * @return The register value for the device version register
 */
uint16_t
    TCAN4x5x_Device_ReadDeviceVersion(void)
{
    uint32_t readValue;

    readValue = AHB_READ_32(REG_SPI_REVISION);

    return (uint16_t)(readValue & 0xFFFF);
}

/**
 * @brief Configures the device mode and pin register
 *
 * Configures the device mode and pin register based on the passed @c TCAN4x5x_DEV_CONFIG struct, but will mask out the reserved bits on a write
 *
 * @param *devCfg is a pointer to a @c TCAN4x5x_DEV_CONFIG struct containing the desired device mode and pin register values
 *
 * @return @c TRUE if configuration successfully done, @c FALSE if not
 */
bool TCAN4x5x_Device_Configure(TCAN4x5x_DEV_CONFIG *devCfg)
{
    // First we must read the register
    uint32_t readDevice = AHB_READ_32(REG_DEV_MODES_AND_PINS);

    // Then mask the bits that will be set by the struct
    readDevice &= ~(REG_BITS_DEVICE_MODE_SWE_MASK | REG_BITS_DEVICE_MODE_DEVICE_RESET | REG_BITS_DEVICE_MODE_WDT_MASK |
                    REG_BITS_DEVICE_MODE_NWKRQ_CONFIG_MASK | REG_BITS_DEVICE_MODE_INH_MASK | REG_BITS_DEVICE_MODE_GPO1_FUNC_MASK |
                    REG_BITS_DEVICE_MODE_FAIL_SAFE_MASK | REG_BITS_DEVICE_MODE_GPO1_MODE_MASK | REG_BITS_DEVICE_MODE_WDT_ACTION_MASK |
                    REG_BITS_DEVICE_MODE_WDT_RESET_BIT | REG_BITS_DEVICE_MODE_NWKRQ_VOLT_MASK | REG_BITS_DEVICE_MODE_TESTMODE_ENMASK |
                    REG_BITS_DEVICE_MODE_GPO2_MASK | REG_BITS_DEVICE_MODE_WD_CLK_MASK | REG_BITS_DEVICE_MODE_WAKE_PIN_MASK);

    // Copy to a temporary location in memory, so we don't modify the incoming struct
    TCAN4x5x_DEV_CONFIG tempCfg;
    tempCfg.word = devCfg->word;

    // Clear the reserved flags.
    tempCfg.RESERVED0 = 0;
    tempCfg.RESERVED1 = 0;
    tempCfg.RESERVED2 = 0;
    tempCfg.RESERVED3 = 0;
    tempCfg.RESERVED4 = 0;
    tempCfg.RESERVED5 = 0;


    // Set the bits according to the incoming struct
    readDevice |= (REG_BITS_DEVICE_MODE_FORCED_SET_BITS | tempCfg.word);

    AHB_WRITE_32(REG_DEV_MODES_AND_PINS, readDevice);

#ifdef TCAN4x5x_DEVICE_VERIFY_CONFIGURATION_WRITES
    // Check to see if the write was successful.
    uint32_t readValue = AHB_READ_32(REG_DEV_MODES_AND_PINS); // Read value
    if (readValue != readDevice) {
        return FALSE;
    }
#endif
    return TRUE;
}

/**
 * @brief Reads the device mode and pin register
 *
 * Reads the device mode and pin register and updates the passed @c TCAN4x5x_DEV_CONFIG struct
 *
 * @param *devCfg is a pointer to a @c TCAN4x5x_DEV_CONFIG struct to be updated with the current mode and pin register values
 */
void TCAN4x5x_Device_ReadConfig(TCAN4x5x_DEV_CONFIG *devCfg)
{
    devCfg->word = AHB_READ_32(REG_DEV_MODES_AND_PINS);
}

/**
 * @brief Read the device interrupts
 *
 * Reads the device interrupts and updates a @c TCAN4x5x_Device_Interrupts struct that is passed to the function
 *
 * @param *ir is a pointer to a @c TCAN4x5x_Device_Interrupts struct containing the interrupt bit fields that will be updated
 */
void TCAN4x5x_Device_ReadInterrupts(TCAN4x5x_Device_Interrupts *ir)
{
    ir->word = AHB_READ_32(REG_DEV_IR);
}

/**
 * @brief Clear the device interrupts
 *
 * Will attempt to clear any interrupts that are marked as a '1' in the passed @c TCAN4x5x_Device_Interrupts struct
 *
 * @param *ir is a pointer to a @c TCAN4x5x_Device_Interrupts struct containing the interrupt bit fields that will be updated
 */
void TCAN4x5x_Device_ClearInterrupts(TCAN4x5x_Device_Interrupts *ir)
{
    AHB_WRITE_32(REG_DEV_IR, ir->word);
}

/**
 * @brief Clear all device interrupts
 *
 * Clears all device interrupts
 */
void TCAN4x5x_Device_ClearInterruptsAll(void)
{
    AHB_WRITE_32(REG_DEV_IR, 0xFFFFFFFF);
}

/**
 * @brief Clears a SPIERR flag that may be set
 */
void TCAN4x5x_Device_ClearSPIERR(void)
{
    AHB_WRITE_32(REG_SPI_STATUS, 0xFFFFFFFF); // Simply write all 1s to attempt to clear a SPIERR that was set
}

/**
 * @brief Read the device interrupt enable register
 *
 * Reads the device interrupt enable register and updates the passed @c TCAN4x5x_Device_Interrupt_Enable struct
 *
 * @param *ie is a pointer to a @c TCAN4x5x_Device_Interrupt_Enable struct containing the interrupt bit fields that will be updated
 */
void TCAN4x5x_Device_ReadInterruptEnable(TCAN4x5x_Device_Interrupt_Enable *ie)
{
    ie->word = AHB_READ_32(REG_DEV_IE);
}

/**
 * @brief Configures the device interrupt enable register
 *
 * Configures the device interrupt enable register based on the passed @c TCAN4x5x_Device_Interrupt_Enable struct
 *
 * @param *ie is a pointer to a @c TCAN4x5x_Device_Interrupt_Enable struct containing the desired enabled interrupt bits
 *
 * @return @c TRUE if configuration successfully done, @c FALSE if not
 */
bool TCAN4x5x_Device_ConfigureInterruptEnable(TCAN4x5x_Device_Interrupt_Enable *ie)
{
    AHB_WRITE_32(REG_DEV_IE, ie->word);
#ifdef TCAN4x5x_DEVICE_VERIFY_CONFIGURATION_WRITES
    // Check to see if the write was successful.
    uint32_t readValue = AHB_READ_32(REG_DEV_IE); // Read value
    //tlkapi_printf(APP_CAN_LOG_EN, "readValue = %08X\r\n",readValue);
    readValue &= REG_BITS_DEVICE_IE_MASK; // Apply mask to ignore reserved
    if (readValue != (ie->word & REG_BITS_DEVICE_IE_MASK)) {
        return FALSE;
    }
#endif
    return TRUE;
}

/**
 * @brief Sets the TCAN4x5x device mode
 *
 * Sets the TCAN4x5x device mode based on the input @c modeDefine enum
 *
 * @param modeDefine is an @c TCAN4x5x_Device_Mode_Enum enum
 *
 * @return @c TRUE if configuration successfully done, @c FALSE if not
 */
bool TCAN4x5x_Device_SetMode(TCAN4x5x_Device_Mode_Enum modeDefine)
{
    uint32_t writeValue = (AHB_READ_32(REG_DEV_MODES_AND_PINS) & ~REG_BITS_DEVICE_MODE_DEVICEMODE_MASK);
    switch (modeDefine) {
    case TCAN4x5x_DEVICE_MODE_NORMAL:
        writeValue |= REG_BITS_DEVICE_MODE_DEVICEMODE_NORMAL;
        break;

    case TCAN4x5x_DEVICE_MODE_SLEEP:
        writeValue |= REG_BITS_DEVICE_MODE_DEVICEMODE_SLEEP;
        break;

    case TCAN4x5x_DEVICE_MODE_STANDBY:
        writeValue |= REG_BITS_DEVICE_MODE_DEVICEMODE_STANDBY;
        break;

    default:
        return FALSE;
    }

    AHB_WRITE_32(REG_DEV_MODES_AND_PINS, writeValue);

#ifdef TCAN4x5x_DEVICE_VERIFY_CONFIGURATION_WRITES
    // Check to see if the write was successful.
    writeValue &= REG_BITS_DEVICE_MODE_DEVICEMODE_MASK; // Mask out the part we care about verifying

    if ((AHB_READ_32(REG_DEV_MODES_AND_PINS) & REG_BITS_DEVICE_MODE_DEVICEMODE_MASK) != writeValue) {
        return FALSE;
    }
#endif
    return TRUE;
}

/**
 * @brief Reads the TCAN4x5x device mode
 *
 * Reads the TCAN4x5x device mode and returns a @c modeDefine enum
 *
 * @return A @c TCAN4x5x_Device_Mode_Enum enum of the current state
 */
TCAN4x5x_Device_Mode_Enum
    TCAN4x5x_Device_ReadMode(void)
{
    uint32_t readValue = (AHB_READ_32(REG_DEV_MODES_AND_PINS) & REG_BITS_DEVICE_MODE_DEVICEMODE_MASK);

    switch (readValue) {
    case REG_BITS_DEVICE_MODE_DEVICEMODE_NORMAL:
        return TCAN4x5x_DEVICE_MODE_NORMAL;

    case REG_BITS_DEVICE_MODE_DEVICEMODE_SLEEP:
        return TCAN4x5x_DEVICE_MODE_SLEEP;

    case REG_BITS_DEVICE_MODE_DEVICEMODE_STANDBY:
        return TCAN4x5x_DEVICE_MODE_STANDBY;

    default:
        return TCAN4x5x_DEVICE_MODE_STANDBY;
    }
}

/**
 * @brief Sets the TCAN4x5x device test mode
 *
 * Sets the TCAN4x5x device test mode based on the input @c modeDefine enum
 *
 * @param modeDefine is an @c TCAN4x5x_Device_Test_Mode_Enum enum
 *
 * @return @c TRUE if configuration successfully done, @c FALSE if not
 */
bool TCAN4x5x_Device_EnableTestMode(TCAN4x5x_Device_Test_Mode_Enum modeDefine)
{
    uint32_t readWriteValue = AHB_READ_32(REG_DEV_MODES_AND_PINS);
    readWriteValue &= ~REG_BITS_DEVICE_MODE_TESTMODE_MASK; // Clear the bits that we are setting

    // Set the appropriate bits depending on the passed in value
    switch (modeDefine) {
    case TCAN4x5x_DEVICE_TEST_MODE_NORMAL:
        TCAN4x5x_Device_DisableTestMode();
        break;

    case TCAN4x5x_DEVICE_TEST_MODE_CONTROLLER:
        readWriteValue |= REG_BITS_DEVICE_MODE_TESTMODE_CONTROLLER | REG_BITS_DEVICE_MODE_TESTMODE_EN;
        break;

    case TCAN4x5x_DEVICE_TEST_MODE_PHY:
        readWriteValue |= REG_BITS_DEVICE_MODE_TESTMODE_PHY | REG_BITS_DEVICE_MODE_TESTMODE_EN;
        break;

    default:
        return FALSE;                                     // If an invalid value was passed, then we will return fail
    }
    AHB_WRITE_32(REG_DEV_MODES_AND_PINS, readWriteValue); // Write the updated values

#ifdef TCAN4x5x_DEVICE_VERIFY_CONFIGURATION_WRITES
    // Check to see if the write was successful.
    if (AHB_READ_32(REG_DEV_MODES_AND_PINS) != readWriteValue) {
        return FALSE;
    }
#endif
    return TRUE;
}

/**
 * @brief Disables the TCAN4x5x device test mode
 *
 * @return @c TRUE if disabling test mode was successful, @c FALSE if not
 */
bool TCAN4x5x_Device_DisableTestMode(void)
{
    uint32_t readWriteValue = AHB_READ_32(REG_DEV_MODES_AND_PINS);
    readWriteValue &= ~(REG_BITS_DEVICE_MODE_TESTMODE_MASK | REG_BITS_DEVICE_MODE_TESTMODE_ENMASK); // Clear the bits
    AHB_WRITE_32(REG_DEV_MODES_AND_PINS, readWriteValue);                                           // Write the updated values

#ifdef TCAN4x5x_DEVICE_VERIFY_CONFIGURATION_WRITES
    // Check to see if the write was successful.
    if (AHB_READ_32(REG_DEV_MODES_AND_PINS) != readWriteValue) {
        return FALSE;
    }
#endif
    return TRUE;
}

u8 tcan4550_enter_sleep(void)
{
    return TCAN4x5x_Device_SetMode(TCAN4x5x_DEVICE_MODE_SLEEP);
}

/**
 * @brief Reads the TCAN4x5x device test mode
 *
 * @return an @c TCAN4x5x_Device_Test_Mode_Enum of the current device test mode
 */
TCAN4x5x_Device_Test_Mode_Enum
    TCAN4x5x_Device_ReadTestMode(void)
{
    uint32_t readValue = AHB_READ_32(REG_DEV_MODES_AND_PINS);

    // If Test mode is enabled...
    if (readValue & REG_BITS_DEVICE_MODE_TESTMODE_ENMASK) {
        if (readValue & REG_BITS_DEVICE_MODE_TESTMODE_CONTROLLER) {
            return TCAN4x5x_DEVICE_TEST_MODE_CONTROLLER;
        } else {
            return TCAN4x5x_DEVICE_TEST_MODE_PHY;
        }
    }
    return TCAN4x5x_DEVICE_TEST_MODE_NORMAL;
}

/**
 * @brief Configure the watchdog
 *
 * @param WDTtimeout is an @c TCAN4x5x_WDT_Timer_Enum enum of different times for the watch dog window
 *
 * @return @c TRUE if successfully configured, or @c FALSE otherwise
 */
bool TCAN4x5x_WDT_Configure(TCAN4x5x_WDT_Timer_Enum WDTtimeout)
{
    uint32_t readWriteValue = AHB_READ_32(REG_DEV_MODES_AND_PINS);
    readWriteValue &= ~REG_BITS_DEVICE_MODE_WD_TIMER_MASK; // Clear the bits that we are setting

    // Set the appropriate bits depending on the passed in value
    switch (WDTtimeout) {
    case TCAN4x5x_WDT_60MS:
        readWriteValue |= REG_BITS_DEVICE_MODE_WD_TIMER_60MS;
        break;

    case TCAN4x5x_WDT_600MS:
        readWriteValue |= REG_BITS_DEVICE_MODE_WD_TIMER_600MS;
        break;

    case TCAN4x5x_WDT_3S:
        readWriteValue |= REG_BITS_DEVICE_MODE_WD_TIMER_3S;
        break;

    case TCAN4x5x_WDT_6S:
        readWriteValue |= REG_BITS_DEVICE_MODE_WD_TIMER_6S;
        break;

    default:
        return FALSE;                                     // If an invalid value was passed, then we will return fail
    }
    AHB_WRITE_32(REG_DEV_MODES_AND_PINS, readWriteValue); // Write the updated values

#ifdef TCAN4x5x_DEVICE_VERIFY_CONFIGURATION_WRITES
    // Check to see if the write was successful.
    if (AHB_READ_32(REG_DEV_MODES_AND_PINS) != readWriteValue) {
        return FALSE;
    }
#endif
    return TRUE;
}

/**
 * @brief Read the watchdog configuration
 *
 * @return an @c TCAN4x5x_WDT_Timer_Enum enum of the currently configured time window
 */
TCAN4x5x_WDT_Timer_Enum
    TCAN4x5x_WDT_Read(void)
{
    uint32_t readValue = AHB_READ_32(REG_DEV_MODES_AND_PINS);
    readValue &= REG_BITS_DEVICE_MODE_WD_TIMER_MASK;

    switch (readValue) {
    case REG_BITS_DEVICE_MODE_WD_TIMER_60MS:
        return TCAN4x5x_WDT_60MS;

    case REG_BITS_DEVICE_MODE_WD_TIMER_600MS:
        return TCAN4x5x_WDT_600MS;

    case REG_BITS_DEVICE_MODE_WD_TIMER_3S:
        return TCAN4x5x_WDT_3S;

    case REG_BITS_DEVICE_MODE_WD_TIMER_6S:
        return TCAN4x5x_WDT_6S;

    default:
        return TCAN4x5x_WDT_60MS; // If an invalid value was passed, then we will return the POR default
    }
}

/**
 * @brief Enable the watchdog timer
 *
 * @return @c TRUE if successfully enabled, or @c FALSE otherwise
 */
bool TCAN4x5x_WDT_Enable(void)
{
    uint32_t readWriteValue = AHB_READ_32(REG_DEV_MODES_AND_PINS) | REG_BITS_DEVICE_MODE_WDT_EN;
    AHB_WRITE_32(REG_DEV_MODES_AND_PINS, readWriteValue); // Enable the watch dog timer

#ifdef TCAN4x5x_DEVICE_VERIFY_CONFIGURATION_WRITES
    // Check to see if the write was successful.
    if (AHB_READ_32(REG_DEV_MODES_AND_PINS) != readWriteValue) {
        return FALSE;
    }
#endif

    return TRUE;
}

/**
 * @brief Disable the watchdog timer
 *
 * @return @c TRUE if successfully disabled, or @c FALSE otherwise
 */
bool TCAN4x5x_WDT_Disable(void)
{
    uint32_t writeValue = AHB_READ_32(REG_DEV_MODES_AND_PINS);
    writeValue &= ~REG_BITS_DEVICE_MODE_WDT_EN;       // Clear the EN bit
    AHB_WRITE_32(REG_DEV_MODES_AND_PINS, writeValue); // Disable the watch dog timer

#ifdef TCAN4x5x_DEVICE_VERIFY_CONFIGURATION_WRITES
    // Check to see if the write was successful.
    if (AHB_READ_32(REG_DEV_MODES_AND_PINS) != writeValue) {
        return FALSE;
    }
#endif
    return TRUE;
}

/**
 * @brief Reset the watchdog timer
 */
void TCAN4x5x_WDT_Reset(void)
{
    uint32_t writeValue = AHB_READ_32(REG_DEV_MODES_AND_PINS);
    writeValue |= REG_BITS_DEVICE_MODE_WDT_RESET_BIT;
    AHB_WRITE_32(REG_DEV_MODES_AND_PINS, writeValue); // Reset the watch dog timer
}
