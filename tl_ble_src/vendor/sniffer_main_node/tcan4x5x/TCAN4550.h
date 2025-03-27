/*
 * TCAN4550.h
 * Description: This file contains TCAN4550 functions, and relies on the TCAN4x5x_SPI abstraction functions
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

#ifndef TCAN4550_H_
#define TCAN4550_H_
#include "TCAN4x5x_SPI.h"
#include "TCAN4x5x_Reg.h"
#include "TCAN4x5x_Data_Structs.h"
#include "tl_common.h"
#include "drivers.h"


#define TEST_GPIO_1
#define TEST_GPIO_2

#define TCAN4550_GPIO_SW3
#define TCAN4550_GPIO_SW2                           GPIO_PC5

#if (BOARD_SELECT==BOARD_9528A_EVK_C1T266A20_V1_3)
#define TCAN4550_GPIO_RST                           GPIO_PB1// ____|-------|______ reset, normal mode set low level
#define TCAN4550_GPIO_WKREQ_N                       GPIO_PA6//
#define TCAN4550_GPIO_INT_N                         GPIO_PB4//
#define TCAN4550_GPIO_INH                           GPIO_PC4// NG
#define TCAN4550_GPIO_WAKE                          GPIO_PB0//
#define TCAN4550_GPIO_GPIO1                         GPIO_PA1//
#define TCAN4550_GPIO_GPIO2                         GPIO_PA2//
#elif (BOARD_SELECT==BOARD_9223A_EVK_C1T289A67_V1_0)
#define TCAN4550_GPIO_RST                           GPIO_PA4// ____|-------|______ reset, normal mode set low level
#define TCAN4550_GPIO_WKREQ_N                       GPIO_PA3//
#define TCAN4550_GPIO_INT_N                         GPIO_PA2//
#define TCAN4550_GPIO_INH                           GPIO_PC4// NG
#define TCAN4550_GPIO_WAKE                          GPIO_PB2//
#define TCAN4550_GPIO_GPIO1                         GPIO_PA0//
#define TCAN4550_GPIO_GPIO2                         GPIO_PA1//
#elif (BOARD_SELECT==BOARD_9223B_EVK_C1T325A67_V1_0)
#define TCAN4550_GPIO_RST                           GPIO_PA4// ____|-------|______ reset, normal mode set low level
#define TCAN4550_GPIO_WKREQ_N                       GPIO_PA3//
#define TCAN4550_GPIO_INT_N                         GPIO_PA2//
#define TCAN4550_GPIO_INH                           GPIO_PC4// NG
#define TCAN4550_GPIO_WAKE                          GPIO_PB2//
#define TCAN4550_GPIO_GPIO1                                 // NG
#define TCAN4550_GPIO_GPIO2                         GPIO_PA1//
#endif

#define TCAN4550_AUTO_RESET_ENABLE              1
#define TCAN4550_RX_INTERRUPT_ENABLE            0
#define RATE_500K                               0
#define RATE_1M                                 1
#define RATE_2M                                 2
#define RATE_5M                                 3
#define DATA_FIELD_RATE                         RATE_2M


#define SLAVE_TO_ECU_SID                        0x777
#define ECU_TO_SLAVE_SID                        0x767

#define SNIFFER_TO_SLAVE_RSSI_SID_0             0x751
#define SNIFFER_TO_SLAVE_RSSI_SID_1             0x752
#define SNIFFER_TO_SLAVE_RSSI_SID_2             0x753
#define SNIFFER_TO_SLAVE_RSSI_SID_3             0x754
#define SNIFFER_TO_SLAVE_RSSI_SID_4             0x755
#define SNIFFER_TO_SLAVE_RSSI_SID_5             0x756


#define SLAVE_TO_SNIFFER_SYNC_SID               0x731
#define SNIFFER_TO_SLAVE_SYNC_SID               0x737


//! If TCAN4x5x_MCAN_CACHE_CONFIGURATION is defined, then the read and write to MRAM functions will cache certain values to reduce the number of SPI reads necessary to send or receive a packet
#define TCAN4x5x_MCAN_CACHE_CONFIGURATION

//! If TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES is defined, then each MCAN configuration write will be read and verified for correctness
#define TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES

//! If TCAN4x5x_DEVICE_VERIFY_CONFIGURATION_WRITES is defined, then each device configuration write will be read and verified for correctness
#define TCAN4x5x_DEVICE_VERIFY_CONFIGURATION_WRITES


// Defines for caching the configuration values to save on reads and writes
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
#define TCAN4x5x_MCAN_CACHE_SIDFC           0
#define TCAN4x5x_MCAN_CACHE_XIDFC           1
#define TCAN4x5x_MCAN_CACHE_RXF0C           2
#define TCAN4x5x_MCAN_CACHE_RXF1C           3
#define TCAN4x5x_MCAN_CACHE_RXBC            4
#define TCAN4x5x_MCAN_CACHE_TXEFC           5
#define TCAN4x5x_MCAN_CACHE_TXBC            6
#define TCAN4x5x_MCAN_CACHE_RXESC           7
#define TCAN4x5x_MCAN_CACHE_TXESC           8
#endif

typedef enum { RXFIFO0, RXFIFO1 } TCAN4x5x_MCAN_FIFO_Enum;
typedef enum { TCAN4x5x_WDT_60MS, TCAN4x5x_WDT_600MS, TCAN4x5x_WDT_3S, TCAN4x5x_WDT_6S } TCAN4x5x_WDT_Timer_Enum;
typedef enum { TCAN4x5x_DEVICE_TEST_MODE_NORMAL, TCAN4x5x_DEVICE_TEST_MODE_PHY, TCAN4x5x_DEVICE_TEST_MODE_CONTROLLER } TCAN4x5x_Device_Test_Mode_Enum;
typedef enum { TCAN4x5x_DEVICE_MODE_NORMAL, TCAN4x5x_DEVICE_MODE_STANDBY, TCAN4x5x_DEVICE_MODE_SLEEP } TCAN4x5x_Device_Mode_Enum;

#define CAN_CONFIGURATION_ADDR              0x60000
typedef struct __attribute__((packed))
{
    u16 deviceId;
    u16 canId;
    u16 reportInterval;
    u16 rspSyncId;
    u16 sampleSize;
    u16 u16_rsvd;
}can_fd_cfg_t;

extern can_fd_cfg_t can_fd_cfg;
extern _attribute_ble_data_retention_ volatile u32 tcan_reset_flag;

void tcan4550_readReg_t(void);
void tcan4550_readREG0800(void);
int can_fd_data_send(u16 sid, u8 *pData, u32 len);
void tcan4550_reset_hw(void);
void tcan4550_init(void);
void Init_CAN(void);
void tcan4550_isr(void);
// ~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*
//                            MCAN Device Functions
// ~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*
extern bool TCAN4x5x_MCAN_EnableProtectedRegisters(void);
extern bool TCAN4x5x_MCAN_DisableProtectedRegisters(void);
extern bool TCAN4x5x_MCAN_ConfigureCCCRRegister(TCAN4x5x_MCAN_CCCR_Config *cccr);
extern void TCAN4x5x_MCAN_ReadCCCRRegister(TCAN4x5x_MCAN_CCCR_Config *cccrConfig);
extern void TCAN4x5x_MCAN_ReadDataTimingFD_Simple(TCAN4x5x_MCAN_Data_Timing_Simple *dataTiming);
extern void TCAN4x5x_MCAN_ReadDataTimingFD_Raw(TCAN4x5x_MCAN_Data_Timing_Raw *dataTiming);
extern bool TCAN4x5x_MCAN_ConfigureDataTiming_Simple(TCAN4x5x_MCAN_Data_Timing_Simple *dataTiming);
extern bool TCAN4x5x_MCAN_ConfigureDataTiming_Raw(TCAN4x5x_MCAN_Data_Timing_Raw *dataTiming);
extern void TCAN4x5x_MCAN_ReadNominalTiming_Simple(TCAN4x5x_MCAN_Nominal_Timing_Simple *nomTiming);
extern void TCAN4x5x_MCAN_ReadNominalTiming_Raw(TCAN4x5x_MCAN_Nominal_Timing_Raw *nomTiming);
extern bool TCAN4x5x_MCAN_ConfigureNominalTiming_Simple(TCAN4x5x_MCAN_Nominal_Timing_Simple *nomTiming);
extern bool TCAN4x5x_MCAN_ConfigureNominalTiming_Raw(TCAN4x5x_MCAN_Nominal_Timing_Raw *nomTiming);
extern bool TCAN4x5x_MCAN_ConfigureGlobalFilter(TCAN4x5x_MCAN_Global_Filter_Configuration *gfc);


extern bool TCAN4x5x_MRAM_Configure(TCAN4x5x_MRAM_Config *MRAMConfig);
extern void TCAN4x5x_MRAM_Clear(void);
extern void TCAN4x5x_MCAN_ReadInterrupts(TCAN4x5x_MCAN_Interrupts *ir);
extern void TCAN4x5x_MCAN_ClearInterrupts(TCAN4x5x_MCAN_Interrupts *ir);
extern void TCAN4x5x_MCAN_ClearInterruptsAll(void);
extern void TCAN4x5x_MCAN_ReadInterruptEnable(TCAN4x5x_MCAN_Interrupt_Enable *ie);
extern void TCAN4x5x_MCAN_ConfigureInterruptEnable(TCAN4x5x_MCAN_Interrupt_Enable *ie);
extern uint8_t TCAN4x5x_MCAN_ReadNextFIFO(TCAN4x5x_MCAN_FIFO_Enum FIFODefine, TCAN4x5x_MCAN_RX_Header *header, uint8_t dataPayload[]);
extern uint8_t TCAN4x5x_MCAN_ReadRXBuffer(uint8_t bufIndex, TCAN4x5x_MCAN_RX_Header *header, uint8_t dataPayload[]);
extern uint32_t TCAN4x5x_MCAN_WriteTXBuffer(uint8_t bufIndex, TCAN4x5x_MCAN_TX_Header *header, uint8_t dataPayload[]);
extern bool TCAN4x5x_MCAN_TransmitBufferContents(uint8_t bufIndex);
extern bool TCAN4x5x_MCAN_WriteSIDFilter(uint8_t filterIndex, TCAN4x5x_MCAN_SID_Filter *filter);
extern bool TCAN4x5x_MCAN_ReadSIDFilter(uint8_t filterIndex, TCAN4x5x_MCAN_SID_Filter *filter);
extern bool TCAN4x5x_MCAN_WriteXIDFilter(uint8_t fifoIndex, TCAN4x5x_MCAN_XID_Filter *filter);
extern uint8_t TCAN4x5x_MCAN_DLCtoBytes(uint8_t inputDLC);
extern uint8_t TCAN4x5x_MCAN_TXRXESC_DataByteValue(uint8_t inputESCValue);




// ~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*
//                            Non-MCAN Device Functions
// ~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*
extern uint16_t TCAN4x5x_Device_ReadDeviceVersion(void);
extern bool TCAN4x5x_Device_Configure(TCAN4x5x_DEV_CONFIG *devCfg);
extern void TCAN4x5x_Device_ReadConfig(TCAN4x5x_DEV_CONFIG *devCfg);
extern void TCAN4x5x_Device_ReadInterrupts(TCAN4x5x_Device_Interrupts *ir);
extern void TCAN4x5x_Device_ClearInterrupts(TCAN4x5x_Device_Interrupts *ir);
extern void TCAN4x5x_Device_ClearInterruptsAll(void);
extern void TCAN4x5x_Device_ClearSPIERR(void);
extern void TCAN4x5x_Device_ReadInterruptEnable(TCAN4x5x_Device_Interrupt_Enable *ie);
extern bool TCAN4x5x_Device_ConfigureInterruptEnable(TCAN4x5x_Device_Interrupt_Enable *ie);
extern bool TCAN4x5x_Device_SetMode(TCAN4x5x_Device_Mode_Enum modeDefine);
extern TCAN4x5x_Device_Mode_Enum TCAN4x5x_Device_ReadMode(void);
extern bool TCAN4x5x_Device_EnableTestMode(TCAN4x5x_Device_Test_Mode_Enum modeDefine);
extern bool TCAN4x5x_Device_DisableTestMode(void);
extern TCAN4x5x_Device_Test_Mode_Enum TCAN4x5x_Device_ReadTestMode(void);


extern bool TCAN4x5x_WDT_Configure(TCAN4x5x_WDT_Timer_Enum WDTtimeout);
extern TCAN4x5x_WDT_Timer_Enum TCAN4x5x_WDT_Read(void);
extern bool TCAN4x5x_WDT_Enable(void);
extern bool TCAN4x5x_WDT_Disable(void);
extern void TCAN4x5x_WDT_Reset(void);



#endif
