/*
 * TCAN4x5x_Data_Structs.h
 * Description: This file contains the structures and enumerations that the TCAN4550 library will use
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


#ifndef TCAN4X5X_DATA_STRUCTS_H_
#define TCAN4X5X_DATA_STRUCTS_H_

// ~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*
//                   Starting with the MCAN Data Structures
// ~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*



/**
 * @brief Used to setup the data timing parameters of the MCAN module
 * This is a simplified struct, requiring only the prescaler value (1:x), number of time quanta before and after the sample point.
 */
typedef struct
{
    //! @brief Prescaler value, interpreted as 1:x
    //! \n Valid range is: 1 to 32
    uint8_t DataBitRatePrescaler : 6;

    //! @brief DTQBSP: Number of time quanta before sample point
    //! \n Valid values are: 2 to 33
    uint8_t DataTqBeforeSamplePoint : 6;

    //! @brief DTQASP: Number of time quanta after sample point
    //! \n Valid values are: 1 to 16
    uint8_t DataTqAfterSamplePoint : 5;

} TCAN4x5x_MCAN_Data_Timing_Simple;


/**
 * @brief Used to setup the timing parameters of the MCAN module
 * This is the raw MCAN form of the struct which takes in the same values as the actual Bosch MCAN core
 */
typedef struct
{
    //! @brief DBRP: The prescaler value from the MCAN system clock. Interpreted by MCAN as the value is this field + 1
    //! \n Valid range is: 0 to 31
    uint8_t DataBitRatePrescaler : 5;

    //! @brief DTSEG1: Data time segment 1 + prop segment value. Interpreted by MCAN as the value in this field + 1
    //! \n Valid values are: 0 to 31
    uint8_t DataTimeSeg1andProp : 5;

    //! @brief DTSEG2: Data time segment 2. Interpreted by MCAN as the value is this field + 1
    //! \n Valid values are: 0 to 15
    uint8_t DataTimeSeg2 : 4;

    //! @brief DSJW: Data Resynchronization jump width. Interpreted by MCAN as the value is this field + 1
    //! \n Valid values are: 0 to 15
    uint8_t DataSyncJumpWidth : 4;

    //! @brief TDCO: Transmitter delay compensation offset
    //! \n Valid values are 0 to 127 mtq
    uint8_t TDCOffset : 7;

    //! @brief TDCFilter: Transmitter delay compensation Filter Window Length
    //! \n Valid values are 0 to 127 mtq
    uint8_t TDCFilter : 7;

} TCAN4x5x_MCAN_Data_Timing_Raw;


/**
 * @brief Used to setup the nominal timing parameters of the MCAN module
 * This is a simplified struct, requiring only the prescaler value (1:x), number of time quanta before and after the sample point.
 */
typedef struct
{
    //! @brief NBRP: The prescaler value from the MCAN system clock. Value interpreted as 1:x
    //! \n Valid range is: 1 to 512
    uint16_t NominalBitRatePrescaler : 10;

    //! @brief NTQBSP: The total number of time quanta prior to sample point
    //! \n Valid values are: 2 to 257
    uint16_t NominalTqBeforeSamplePoint : 9;

    //! @brief NTQASP: The total number of time quanta after the sample point
    //!\n Valid values are: 2 to 128
    uint8_t NominalTqAfterSamplePoint : 8;

} TCAN4x5x_MCAN_Nominal_Timing_Simple;


/**
 * @brief Used to setup the nominal timing parameters of the MCAN module
 * This is the raw MCAN form of the struct which takes in the same values as the actual Bosch MCAN core
 */
typedef struct
{
    //! @brief NBRP: The prescaler value from the MCAN system clock. Interpreted by MCAN as the value is this field + 1
    //! \n Valid range is: 0 to 511
    uint16_t NominalBitRatePrescaler : 9;

    //! @brief NTSEG1: Data time segment 1 + prop segment value. Interpreted by MCAN as the value is this field + 1
    //! \n Valid values are: 0 to 255
    uint8_t NominalTimeSeg1andProp : 8;

    //! @brief NTSEG2: Data time segment 2. Interpreted by MCAN as the value is this field + 1
    //! \n Valid values are: 0 to 127
    uint8_t NominalTimeSeg2 : 7;

    //! @brief NSJW: Nominal time Resynchronization jump width. Interpreted by MCAN as the value is this field + 1
    //! \n Valid values are: 0 to 127
    uint8_t NominalSyncJumpWidth : 7;
} TCAN4x5x_MCAN_Nominal_Timing_Raw;


/**
 * @brief Data payload defines for the different MRAM sections, used by the @c TCAN4x5x_MRAM_Config struct
 */
typedef enum
{
    //! 8 bytes of data payload
    MRAM_8_Byte_Data = 0,

    //! 12 bytes of data payload
    MRAM_12_Byte_Data = 0x1,

    //! 16 bytes of data payload
    MRAM_16_Byte_Data = 0x2,

    //! 20 bytes of data payload
    MRAM_20_Byte_Data = 0x3,

    //! 24 bytes of data payload
    MRAM_24_Byte_Data = 0x4,

    //! 32 bytes of data payload
    MRAM_32_Byte_Data = 0x5,

    //! 48 bytes of data payload
    MRAM_48_Byte_Data = 0x6,

    //! 64 bytes of data payload
    MRAM_64_Byte_Data = 0x7
} TCAN4x5x_MRAM_Element_Data_Size;

/**
 * @brief Defines the number of MRAM elements and the size of the elements
 */
typedef struct
{
    /************************
     *    Filter Elements   *
     ************************/

    //! @brief Standard ID Number of Filter Elements: The number of 11-bit filters the user would like
    //! \n Valid range is: 0 to 128
    uint8_t SIDNumElements : 8;

    //! @brief Extended ID Number of Filter Elements: The number of 29-bit filters the user would like
    //! \n Valid range is: 0 to 64
    uint8_t XIDNumElements : 7;


    /************************
     *  RX FIFO Elements    *
     ************************/

    //! @brief RX FIFO 0 number of elements: The number of elements for the RX FIFO 0
    //! \n Valid range is: 0 to 64
    uint8_t Rx0NumElements : 7;

    //! @brief RX FIFO 0 element size: The number of bytes for the RX 0 FIFO (data payload)
    TCAN4x5x_MRAM_Element_Data_Size Rx0ElementSize : 3;

    //! @brief RX FIFO 1 number of elements: The number of elements for the RX FIFO 1
    //!\n Valid range is: 0 to 64
    uint8_t Rx1NumElements : 7;

    //! @brief RX FIFO 1 element size: The number of bytes for the RX 1 FIFO (data payload)
    TCAN4x5x_MRAM_Element_Data_Size Rx1ElementSize : 3;

    //! @brief RX Buffers number of elements: The number of elements for the RX Buffers (Not the FIFO)
    //! \n Valid range is: 0 to 64
    uint8_t RxBufNumElements : 7;

    //! @brief RX Buffers element size: The number of bytes for the RX Buffers (data payload), not the FIFO
    TCAN4x5x_MRAM_Element_Data_Size RxBufElementSize : 3;

    /************************
     *  TX Buffer Elements  *
     ************************/

    //! @brief TX Event FIFO number of elements: The number of elements for the TX Event FIFO
    //! \n Valid range is: 0 to 32
    uint8_t TxEventFIFONumElements : 6;

    //! @brief TX Buffers number of elements: The number of elements for the TX Buffers
    //! \n Valid range is: 0 to 32
    uint8_t TxBufferNumElements : 6;

    //! @brief TX Buffers element size: The number of bytes for the TX Buffers (data payload)
    TCAN4x5x_MRAM_Element_Data_Size TxBufferElementSize : 3;

} TCAN4x5x_MRAM_Config;


/**
 * @brief struct containing the bit fields of the MCAN CCCR register
 */
typedef struct
{
    union
    {
        //! @brief Full register as single 32-bit word
        uint32_t word;

        struct
        {
            //! @brief Reserved (0)
            uint8_t reserved : 2;

            //! @brief ASM: Restricted Operation Mode. The device can only listen to CAN traffic and acknowledge, but not send anything.
            uint8_t ASM : 1;

            //! @brief Reserved (0)
            uint8_t reserved2 : 1;

            //! @brief CSR: Clock stop request
            uint8_t CSR : 1;

            //! @brief MON: Bus monitoring mode. The device may only listen to CAN traffic, and is not allowed to acknowledge or send error frames.
            uint8_t MON : 1;

            //! @brief DAR: Disable automatic retransmission. If a transmission errors, gets a NACK, or loses arbitration, the MCAN controller will NOT try to transmit again
            uint8_t DAR : 1;

            //! @brief TEST: MCAN Test mode enable
            uint8_t TEST : 1;

            //! @brief FDOE: Can FD mode enabled, master enable for CAN FD support
            uint8_t FDOE : 1;

            //! @brief BRSE: Bit rate switch enabled for can FD. Master enable for bit rate switching support
            uint8_t BRSE : 1;

            //! @brief Reserved (0)
            uint8_t reserved3 : 2;

            //! @brief PXHD: Protocol exception handling disable
            //! \n 0 = Protocol exception handling enabled [default]
            //! \n 1 = protocol exception handling disabled
            uint8_t PXHD : 1;

            //! @brief EFBI: Edge filtering during bus integration. 0 Disables this [default]
            uint8_t EFBI : 1;

            //! @brief TXP: Transmit Pause Enable: Pause for 2 can bit times before next transmission
            uint8_t TXP : 1;

            //! @brief NSIO: Non Iso Operation
            //! \n 0: CAN FD frame format according to ISO 11898-1:2015 [default]
            //! \n 1: CAN FD frame format according to Bosch CAN FD Spec v1
            uint8_t NISO : 1;
        };
    };
} TCAN4x5x_MCAN_CCCR_Config;



/**
 * @brief Struct containing the MCAN interrupt bit field
 */
typedef struct
{
    union
    {
        //! @brief Full register as single 32-bit word
        uint32_t word;
        struct
        {
            //! @brief IR[0] RF0N: Rx FIFO 0 new message
            uint8_t RF0N : 1;

            //! @brief IR[1] RF0W: Rx FIFO 0 watermark reached
            uint8_t RF0W : 1;

            //! @brief IR[2] RF0F: Rx FIFO 0 full
            uint8_t RF0F : 1;

            //! @brief IR[3] RF0L: Rx FIFO 0 message lost
            uint8_t RF0L : 1;

            //! @brief IR[4] RF1N: Rx FIFO 1 new message
            uint8_t RF1N : 1;

            //! @brief IR[5]  RF1W: RX FIFO 1 watermark reached
            uint8_t RF1W : 1;

            //! @brief IR[6] RF1F: Rx FIFO 1 full
            uint8_t RF1F : 1;

            //! @brief IR[7] RF1L: Rx FIFO 1 message lost
            uint8_t RF1L : 1;

            //! @brief IR[8] HPM: High priority message
            uint8_t HPM : 1;

            //! @brief IR[9] TC: Transmission completed
            uint8_t TC : 1;

            //! @brief IR[10] TCF: Transmission cancellation finished
            uint8_t TCF : 1;

            //! @brief IR[11] TFE: Tx FIFO Empty
            uint8_t TFE : 1;

            //! @brief IR[12] TEFN: Tx Event FIFO new entry
            uint8_t TEFN : 1;

            //! @brief IR[13] TEFW: Tx Event FIFO water mark reached
            uint8_t TEFW : 1;

            //! @brief IR[14] TEFF: Tx Event FIFO full
            uint8_t TEFF : 1;

            //! @brief IR[15] TEFL: Tx Event FIFO element lost
            uint8_t TEFL : 1;

            //! @brief IR[16] TSW: Timestamp wrapped around
            uint8_t TSW : 1;

            //! @brief IR[17] MRAF: Message RAM access failure
            uint8_t MRAF : 1;

            //! @brief IR[18] TOO: Time out occurred
            uint8_t TOO : 1;

            //! @brief IR[19] DRX: Message stored to dedicated RX buffer
            uint8_t DRX : 1;

            //! @brief IR[20] BEC: MRAM Bit error corrected
            uint8_t BEC : 1;

            //! @brief IR[21] BEU: MRAM Bit error uncorrected
            uint8_t BEU : 1;

            //! @brief IR[22] ELO: Error logging overflow
            uint8_t ELO : 1;

            //! @brief IR[23] EP: Error_passive status changed
            uint8_t EP : 1;

            //! @brief IR[24] EW: Error_warning status changed
            uint8_t EW : 1;

            //! @brief IR[25] BO: Bus_off status changed
            uint8_t BO : 1;

            //! @brief IR[26] WDI: MRAM Watchdog Interrupt
            uint8_t WDI : 1;

            //! @brief IR[27] PEA Protocol Error in arbitration phase (nominal bit time used)
            uint8_t PEA : 1;

            //! @brief IR[28] PED: Protocol error in data phase (data bit time is used)
            uint8_t PED : 1;

            //! @brief IR[29] ARA: Access to reserved address
            uint8_t ARA : 1;

            //! @brief IR[30:31] Reserved, not writable
            uint8_t reserved : 2;
        };
    };
} TCAN4x5x_MCAN_Interrupts;


/**
 * @brief Struct containing the MCAN interrupt enable bit field
 */
typedef struct
{
    union
    {
        //! Full register as single 32-bit word
        uint32_t word;
        struct
        {
            //! @brief IE[0] RF0NE: Rx FIFO 0 new message
            uint8_t RF0NE : 1;

            //! @brief IE[1] RF0WE: Rx FIFO 0 watermark reached
            uint8_t RF0WE : 1;

            //! @brief IE[2] RF0FE: Rx FIFO 0 full
            uint8_t RF0FE : 1;

            //! @brief IE[3] RF0LE: Rx FIFO 0 message lost
            uint8_t RF0LE : 1;

            //! @brief IE[4] RF1NE: Rx FIFO 1 new message
            uint8_t RF1NE : 1;

            //! @brief IE[5]  RF1WE: RX FIFO 1 watermark reached
            uint8_t RF1WE : 1;

            //! @brief IE[6] RF1FE: Rx FIFO 1 full
            uint8_t RF1FE : 1;

            //! @brief IE[7] RF1LE: Rx FIFO 1 message lost
            uint8_t RF1LE : 1;

            //! @brief IE[8] HPME: High priority message
            uint8_t HPME : 1;

            //! @brief IE[9] TCE: Transmission completed
            uint8_t TCE : 1;

            //! @brief IE[10] TCFE: Transmission cancellation finished
            uint8_t TCFE : 1;

            //! @brief IE[11] TFEE: Tx FIFO Empty
            uint8_t TFEE : 1;

            //! @brief IE[12] TEFNE: Tx Event FIFO new entry
            uint8_t TEFNE : 1;

            //! @brief IE[13] TEFWE: Tx Event FIFO watermark reached
            uint8_t TEFWE : 1;

            //! @brief IE[14] TEFFE: Tx Event FIFO full
            uint8_t TEFFE : 1;

            //! @brief IE[15] TEFLE: Tx Event FIFO element lost
            uint8_t TEFLE : 1;

            //! @brief IE[16] TSWE: Timestamp wraparound
            uint8_t TSWE : 1;

            //! @brief IE[17] MRAFE: Message RAM access failure
            uint8_t MRAFE : 1;

            //! @brief IE[18] TOOE: Time out occurred
            uint8_t TOOE : 1;

            //! @brief IE[19] DRXE: Message stored to dedicated RX buffer
            uint8_t DRXE : 1;

            //! @brief IE[20] BECE: MRAM Bit error corrected
            uint8_t BECE : 1;

            //! @brief IE[21] BEUE: MRAM Bit error uncorrected
            uint8_t BEUE : 1;

            //! @brief IE[22] ELOE: Error logging overflow
            uint8_t ELOE : 1;

            //! @brief IE[23] EPE: Error_passive status changed
            uint8_t EPE : 1;

            //! @brief IE[24] EWE: Error_warning status changed
            uint8_t EWE : 1;

            //! @brief IE[25] BOE: Bus_off status changed
            uint8_t BOE : 1;

            //! @brief IE[26] WDIE: MRAM Watchdog Interrupt
            uint8_t WDIE : 1;

            //! @brief IE[27] PEAE Protocol Error in arbitration phase (nominal bit time used)
            uint8_t PEAE : 1;

            //! @brief IE[28] PEDE: Protocol error in data phase (data bit time is used)
            uint8_t PEDE : 1;

            //! @brief IE[29] ARAE: Access to reserved address
            uint8_t ARAE : 1;

            //! @brief IE[30:31] Reserved, not writable
            uint8_t reserved : 2;
        };
    };
} TCAN4x5x_MCAN_Interrupt_Enable;


/**
 * @brief CAN message header
 */
typedef struct
{
    //! @brief CAN ID received
    uint32_t ID : 29;

    //! @brief Remote Transmission Request flag
    uint8_t RTR : 1;

    //! @brief Extended Identifier flag
    uint8_t XTD : 1;

    //! @brief Error state indicator flag
    uint8_t ESI : 1;

    //! @brief Receive time stamp
    uint16_t RXTS : 16;

    //! @brief Data length code
    uint8_t DLC : 4;

    //! @brief Bit rate switch used flag
    uint8_t BRS : 1;

    //! @brief CAN FD Format flag
    uint8_t FDF : 1;

    //! @brief Reserved (0)
    uint8_t reserved : 2;

    //! @brief Filter index that this message matched
    uint8_t FIDX : 7;

    //! @brief Accepted non matching frame flag
    uint8_t ANMF : 1;

} TCAN4x5x_MCAN_RX_Header ;


/**
 * @brief CAN message header for transmitted messages
 */
typedef struct
{
   //! @brief CAN ID to send
    uint32_t ID : 29;

   //! @brief Remote Transmission Request flag
    uint8_t RTR : 1;

   //! @brief Extended Identifier flag
    uint8_t XTD : 1;

   //! @brief Error state indicator flag
    uint8_t ESI : 1;

   //! @brief Data length code
    uint8_t DLC : 4;

   //! @brief Bit rate switch used flag
    uint8_t BRS : 1;

   //! @brief CAN FD Format flag
    uint8_t FDF : 1;

   //! @brief Reserved
    uint8_t reserved : 1;

   //! @brief Event FIFO Control flag, to store tx events or not
    uint8_t EFC : 1;

   //! @brief Message Marker, used if @c EFC is set to 1
    uint8_t MM : 8;

} TCAN4x5x_MCAN_TX_Header;


typedef enum
{
    //! Disabled filter. This filter will do nothing if it matches a packet
    TCAN4x5x_SID_SFEC_DISABLED          = 0x0,

    //! Store in RX FIFO 0 if the filter matches the incoming message
    TCAN4x5x_SID_SFEC_STORERX0          = 0x1,

    //! Store in RX FIFO 1 if the filter matches the incoming message
    TCAN4x5x_SID_SFEC_STORERX1          = 0x2,

    //! Reject the packet (do not store, do not notify MCU) if the filter matches the incoming message
    TCAN4x5x_SID_SFEC_REJECTMATCH       = 0x3,

    //! Store in default location but set a high priority message interrupt if the filter matches the incoming message
    TCAN4x5x_SID_SFEC_PRIORITY          = 0x4,

    //! Store in RX FIFO 0 and set a high priority message interrupt if the filter matches the incoming message
    TCAN4x5x_SID_SFEC_PRIORITYSTORERX0  = 0x5,

    //! Store in RX FIFO 1 and set a high priority message interrupt if the filter matches the incoming message
    TCAN4x5x_SID_SFEC_PRIORITYSTORERX1  = 0x6,

    //! Store in RX Buffer for debug if the filter matches the incoming message. SFT is ignored if this is selected.
    TCAN4x5x_SID_SFEC_STORERXBUFORDEBUG = 0x7
} TCAN4x5x_SID_SFEC_Values;

typedef enum
{
    //! Disabled filter. This filter will match nothing
    TCAN4x5x_SID_SFT_DISABLED           = 0x3,

    //! Classic filter with SFID1 as the ID to match, and SFID2 as the bit mask that applies to SFID1
    TCAN4x5x_SID_SFT_CLASSIC            = 0x2,

    //! Dual ID filter, where both SFID1 and SFID2 hold IDs that can match (must match exactly)
    TCAN4x5x_SID_SFT_DUALID             = 0x1,

    //! Range Filter. SFID1 holds the start address, and SFID2 holds the end address. Any address in between will match
    TCAN4x5x_SID_SFT_RANGE              = 0x0
} TCAN4x5x_SID_SFT_Values;


/**
 * @brief Standard ID filter struct
 */
typedef struct
{
    union
    {
        //! full register as single 32-bit word
        uint32_t word;

        struct
        {
            //! @brief SFID2[10:0]
            uint16_t SFID2 : 11;

            //! @brief Reserved
            uint8_t reserved : 5;

            //! @brief SFID1[10:0]
            uint16_t SFID1 : 11;

            //! @brief SFEC[2:0]   Standard filter element configuration
            TCAN4x5x_SID_SFEC_Values SFEC : 3;

            //! @brief SFT Standard Filter Type
            TCAN4x5x_SID_SFT_Values SFT : 2;
        };
    };
} TCAN4x5x_MCAN_SID_Filter;



typedef enum
{
    //! Disabled filter. This filter will do nothing if it matches a packet
    TCAN4x5x_XID_EFEC_DISABLED          = 0x0,

    //! Store in RX FIFO 0 if the filter matches the incoming message
    TCAN4x5x_XID_EFEC_STORERX0          = 0x1,

    //! Store in RX FIFO 1 if the filter matches the incoming message
    TCAN4x5x_XID_EFEC_STORERX1          = 0x2,

    //! Reject the packet (do not store, do not notify MCU) if the filter matches the incoming message
    TCAN4x5x_XID_EFEC_REJECTMATCH       = 0x3,

    //! Store in default location but set a high priority message interrupt if the filter matches the incoming message
    TCAN4x5x_XID_EFEC_PRIORITY          = 0x4,

    //! Store in RX FIFO 0 and set a high priority message interrupt if the filter matches the incoming message
    TCAN4x5x_XID_EFEC_PRIORITYSTORERX0  = 0x5,

    //! Store in RX FIFO 1 and set a high priority message interrupt if the filter matches the incoming message
    TCAN4x5x_XID_EFEC_PRIORITYSTORERX1  = 0x6,

    //! Store in RX Buffer for debug if the filter matches the incoming message.
    TCAN4x5x_XID_EFEC_STORERXBUFORDEBUG = 0x7
} TCAN4x5x_XID_EFEC_Values;

typedef enum
{
    //! Range filter from EFID1 to EFID2, The XIDAM mask is not applied
    TCAN4x5x_XID_EFT_RANGENOMASK        = 0x3,

    //! Classic Filter, EFID1 is the ID/filter, and EFID2 is the mask
    TCAN4x5x_XID_EFT_CLASSIC            = 0x2,

    //! Dual ID filter matches if the incoming ID matches EFID1 or EFID2
    TCAN4x5x_XID_EFT_DUALID             = 0x1,

    //! Range filter from EFID1 to EFID2
    TCAN4x5x_XID_EFT_RANGE              = 0x0
} TCAN4x5x_XID_EFT_Values;


/**
 * @brief Extended ID filter struct
 */
typedef struct
{

    //! @brief EFID2[28:0]
    uint32_t EFID2 : 29;

    //! @brief Reserved
    uint8_t reserved : 1;

    //! @brief EFT[1:0]
    TCAN4x5x_XID_EFT_Values EFT : 2;

    //! EFID1[28:0]
    uint32_t EFID1 : 29;

    //! @brief SFT Standard Filter Type
    TCAN4x5x_XID_EFEC_Values EFEC : 3;
} TCAN4x5x_MCAN_XID_Filter;


typedef enum {
    TCAN4x5x_GFC_ACCEPT_INTO_RXFIFO0 = 0,
    TCAN4x5x_GFC_ACCEPT_INTO_RXFIFO1 = 1,
    TCAN4x5x_GFC_REJECT = 2
} TCAN4x5x_GFC_NO_MATCH_BEHAVIOR;

/**
 * @brief Struct containing the register values for the Global Filter Configuration Register
 */
typedef struct
{
    union
    {
        //! Full word of register
        uint32_t word;

        struct {
            //! @brief GFC[0] :  Reject Remote Frames for Extended IDs
            uint8_t RRFE : 1;

            //! @brief GFC[1] :  Reject Remote Frames for Standard IDs
            uint8_t RRFS : 1;

            //! @brief GFC[3:2] :  Accept Non-matching Frames Extended
            //! Valid values:
            //! TCAN4x5x_GFC_ACCEPT_INTO_RXFIFO0 : Accept into RXFIFO0
            //! TCAN4x5x_GFC_ACCEPT_INTO_RXFIFO1 : Accept into RXFIFO1
            //! TCAN4x5x_GFC_REJECT              : Reject
            TCAN4x5x_GFC_NO_MATCH_BEHAVIOR ANFE : 2;

            //! @brief GFC[5:4] :  Accept Non-matching Frames Standard
            //! Valid values:
            //! TCAN4x5x_GFC_ACCEPT_INTO_RXFIFO0 : Accept into RXFIFO0
            //! TCAN4x5x_GFC_ACCEPT_INTO_RXFIFO1 : Accept into RXFIFO1
            //! TCAN4x5x_GFC_REJECT              : Reject
            TCAN4x5x_GFC_NO_MATCH_BEHAVIOR ANFS : 2;

            //! @brief Reserved
            uint32_t reserved : 26;
        };
    };
} TCAN4x5x_MCAN_Global_Filter_Configuration;



// ~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*
//                        TCAN4x5x Device Structures
// ~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*


/**
 * @brief Struct containing the device interrupt bit field
 */
typedef struct
{
    union
    {
        //! Full register as single 32-bit word
        uint32_t word;

        struct
        {
            //! @brief DEV_IR[0] VTWD: Global Voltage, Temp, or Watchdog (if equipped) Interrupt
            uint8_t VTWD : 1;

            //! @brief DEV_IR[1] M_CAN_INT: There are MCAN interrupts pending
            uint8_t M_CAN_INT : 1;

            //! @brief DEV_IR[2] : Selective Wake Error (If equipped)
            uint8_t SWERR : 1;

            //! @brief DEV_IR[3] : SPI Error
            uint8_t SPIERR : 1;

            //! @brief DEV_IR[4] : CBF, CAN Bus Fault
            uint8_t CBF : 1;

            //! @brief DEV_IR[5] : CANERR, CAN Error
            uint8_t CANERR : 1;

            //! @brief DEV_IR[6] : WKRQ, Wake Request
            uint8_t WKRQ : 1;

            //! @brief DEV_IR[7] : GLOBALERR, Global Error. Is the OR output of all interrupts
            uint8_t GLOBALERR : 1;

            //! @brief DEV_IR[8] : CANDOM, Can bus stuck dominant
            uint8_t CANDOM : 1;

            //! @brief DEV_IR[9] : RESERVED
            uint8_t RESERVED : 1;

            //! @brief DEV_IR[10] : CANTO, CAN Timeout
            uint8_t CANTO : 1;

            //! @brief DEV_IR[11] : RESERVED
            uint8_t RESERVED2 : 1;

            //! @brief DEV_IR[12] : FRAME_OVF, Frame Error Overflow (If Selective Wake is equipped)
            uint8_t FRAME_OVF : 1;

            //! @brief DEV_IR[13] : WKERR, Wake Error
            uint8_t WKERR : 1;

            //! @brief DEV_IR[14] : LWU, Local Wake Up
            uint8_t LWU : 1;

            //! @brief DEV_IR[15] : CANINT, CAN Bus Wake Up Interrupt
            uint8_t CANINT : 1;

            //! @brief DEV_IR[16] : ECCERR, MRAM ECC Error
            uint8_t ECCERR : 1;

            //! @brief DEV_IR[17] : Reserved
            uint8_t RESERVED3 : 1;

            //! @brief DEV_IR[18] : WDTO, Watchdog Time Out
            uint8_t WDTO : 1;

            //! @brief DEV_IR[19] : TSD, Thermal Shut Down
            uint8_t TSD : 1;

            //! @brief DEV_IR[20] : PWRON, Power On Interrupt
            uint8_t PWRON : 1;

            //! @brief DEV_IR[21] : UVIO, Undervoltage on UVIO
            uint8_t UVIO : 1;

            //! @brief DEV_IR[22] : UVSUP, Undervoltage on VSUP and VCCOUT
            uint8_t UVSUP : 1;

            //! @brief DEV_IR[23] : SMS, Sleep Mode Status Flag. Set when sleep mode is entered due to WKERR, UVIO, or TSD faults
            uint8_t SMS : 1;

            //! @brief DEV_IR[24] : CANBUSBAT, CAN Shorted to VBAT
            uint8_t CANBUSBAT : 1;

            //! @brief DEV_IR[25] : CANBUSGND, CAN Shorted to GND
            uint8_t CANBUSGND : 1;

            //! @brief DEV_IR[26] : CANBUSOPEN, CAN Open fault
            uint8_t CANBUSOPEN : 1;

            //! @brief DEV_IR[27] : CANLGND, CANL GND
            uint8_t CANLGND : 1;

            //! @brief DEV_IR[28] : CANHBAT, CANH to VBAT
            uint8_t CANHBAT : 1;

            //! @brief DEV_IR[29] : CANHCANL, CANH and CANL shorted
            uint8_t CANHCANL : 1;

            //! @brief DEV_IR[30] : CANBUSTERMOPEN, CAN Bus has termination point open
            uint8_t CANBUSTERMOPEN : 1;

            //! @brief DEV_IR[31] : CANBUSNOM, CAN Bus is normal flag
            uint8_t CANBUSNORM : 1;
        };
    };
} TCAN4x5x_Device_Interrupts;

/**
 * @brief Struct containing the device interrupt enable bit field
 */
typedef struct
{
    union
    {
        //! Full register as single 32-bit word
        uint32_t word;

        struct
        {
            //! @brief DEV_IE[0:7] : RESERVED
            uint8_t RESERVED1 : 8;

            //! @brief DEV_IE[8] : CANDOM, Can bus stuck dominant
            uint8_t CANDOMEN : 1;

            //! @brief DEV_IE[9] : RESERVED
            uint8_t RESERVED2 : 1;

            //! @brief DEV_IE[10] : CANTO, CAN Timeout
            uint8_t CANTOEN : 1;

            //! @brief DEV_IE[11] : RESERVED
            uint8_t RESERVED3 : 1;

            //! @brief DEV_IE[12] : FRAME_OVF, Frame Error Overflow (If Selective Wake is equipped)
            uint8_t FRAME_OVFEN : 1;

            //! @brief DEV_IE[13] : WKERR, Wake Error
            uint8_t WKERREN : 1;

            //! @brief DEV_IE[14] : LWU, Local Wake Up
            uint8_t LWUEN : 1;

            //! @brief DEV_IE[15] : CANINT, CAN Bus Wake Up Interrupt
            uint8_t CANINTEN : 1;

            //! @brief DEV_IE[16] : ECCERR, MRAM ECC Error
            uint8_t ECCERREN : 1;

            //! @brief DEV_IE[17] : Reserved
            uint8_t RESERVED4 : 1;

            //! @brief DEV_IE[18] : WDTO, Watchdog Time Out
            uint8_t WDTOEN : 1;

            //! @brief DEV_IE[19] : TSD, Thermal Shut Down
            uint8_t TSDEN : 1;

            //! @brief DEV_IE[20] : PWRON, Power On Interrupt
            uint8_t PWRONEN : 1;

            //! @brief DEV_IE[21] : UVIO, Undervoltage on UVIO
            uint8_t UVIOEN : 1;

            //! @brief DEV_IE[22] : UVSUP, Undervoltage on VSUP and VCCOUT
            uint8_t UVSUPEN : 1;

            //! @brief DEV_IE[23] : SMS, Sleep Mode Status Flag. Set when sleep mode is entered due to WKERR, UVIO, or TSD faults
            uint8_t SMSEN : 1;

            //! @brief DEV_IE[24] : CANBUSBAT, CAN Shorted to VBAT
            uint8_t CANBUSBATEN : 1;

            //! @brief DEV_IE[25] : CANBUSGND, CAN Shorted to GND
            uint8_t CANBUSGNDEN : 1;

            //! @brief DEV_IE[26] : CANBUSOPEN, CAN Open fault
            uint8_t CANBUSOPENEN : 1;

            //! @brief DEV_IE[27] : CANLGND, CANL GND
            uint8_t CANLGNDEN : 1;

            //! @brief DEV_IE[28] : CANHBAT, CANH to VBAT
            uint8_t CANHBATEN : 1;

            //! @brief DEV_IE[29] : CANHCANL, CANH and CANL shorted
            uint8_t CANHCANLEN : 1;

            //! @brief DEV_IE[30] : CANBUSTERMOPEN, CAN Bus has termination point open
            uint8_t CANBUSTERMOPENEN : 1;

            //! @brief DEV_IE[31] : CANBUSNOM, CAN Bus is normal flag
            uint8_t CANBUSNORMEN : 1;
        };
    };
} TCAN4x5x_Device_Interrupt_Enable;


typedef enum
{
    TCAN4x5x_DEV_CONFIG_GPO1_SPI_FAULT_INT = 0,
    TCAN4x5x_DEV_CONFIG_GPO1_MCAN_INT1 = 1,
    TCAN4x5x_DEV_CONFIG_GPO1_UNDER_VOLTAGE_OR_THERMAL_INT = 2,
} TCAN4x5x_DEV_CONFIG_GPO1_CONFIG;

typedef enum
{
    TCAN4x5x_DEV_CONFIG_GPIO1_CONFIG_GPO = 0,
    TCAN4x5x_DEV_CONFIG_GPIO1_CONFIG_WATCHDOG_INPUT = 2
} TCAN4x5x_DEV_CONFIG_GPIO1_CONFIG;

typedef enum
{
    TCAN4x5x_DEV_CONFIG_WDT_ACTION_nINT = 0,
    TCAN4x5x_DEV_CONFIG_WDT_ACTION_PULSE_INH = 1,
    TCAN4x5x_DEV_CONFIG_WDT_ACTION_PULSE_WDT_OUTPUT = 2
} TCAN4x5x_DEV_CONFIG_WDT_ACTION;

typedef enum
{
    TCAN4x5x_DEV_CONFIG_GPO2_NO_ACTION = 0,
    TCAN4x5x_DEV_CONFIG_GPO2_MCAN_INT0 = 1,
    TCAN4x5x_DEV_CONFIG_GPO2_WATCHDOG = 2,
    TCAN4x5x_DEV_CONFIG_GPO2_MIRROR_INT = 3
} TCAN4x5x_DEV_CONFIG_GPO2_CONFIG;

typedef enum
{
    TCAN4x5x_DEV_CONFIG_WAKE_DISABLED = 0,
    TCAN4x5x_DEV_CONFIG_WAKE_RISING_EDGE = 1,
    TCAN4x5x_DEV_CONFIG_WAKE_FALLING_EDGE = 2,
    TCAN4x5x_DEV_CONFIG_WAKE_BOTH_EDGES = 3
} TCAN4x5x_DEV_CONFIG_WAKE_CONFIG;

typedef struct
{
    union
    {
        //! Full word for register
        uint32_t word;

        struct
        {
            //! @brief DEV_MODE_PINS[0] : Test mode configuration. Reserved in this struct
            //! It is recommended to use the test mode function to enable or disable test mode rather than this struct
            uint8_t RESERVED0 : 1;

            //! @brief DEV_MODE_PINS[1] : Sleep wake error disable.
            //! Setting this to 1 will disable the 4 minute timer that puts the part back to sleep if no activity is detected
            uint8_t SWE_DIS: 1;

            //! @brief DEV_MODE_PINS[2] : Device reset. Write a 1 to perform a reset on the part
            uint8_t DEVICE_RESET : 1;

            //! @brief DEV_MODE_PINS[3] : Watchdog Enable. Use the watchdog functions to control enabling the watchdog
            uint8_t WD_EN : 1;

            //! @brief Reserved
            //! @brief DEV_MODE_PINS[7:6] : Mode Selection. Use the mode functions to change the mode
            uint8_t RESERVED1 : 4;

            //! @brief DEV_MODE_PINS[8] : nWKRQ Configuration
            //! 0: Mirrors INH function
            //! 1: Wake request interrupt
            uint8_t nWKRQ_CONFIG : 1;

            //! @brief DEV_MODE_PINS[9] : Inhibit pin disable
            uint8_t INH_DIS : 1;

            //! @brief DEV_MODE_PINS[11:10] : GPIO1 pin as a GPO function configuration
            //! Configures the behavior of GPIO1 if it is configured to be a GPO
            //! Available values are:
            //! TCAN4x5x_DEV_CONFIG_GPO1_SPI_FAULT_INT : Active low output for a SPIERR
            //! TCAN4x5x_DEV_CONFIG_GPO1_MCAN_INT1 : Active low output for MCAN_INT1 output (See MCAN ILE and ILS registers to use)
            //! TCAN4x5x_DEV_CONFIG_GPO1_UNDER_VOLTAGE_OR_THERMAL_INT : Active low output for any under voltage or over temp interrupt
            TCAN4x5x_DEV_CONFIG_GPO1_CONFIG GPIO1_GPO_CONFIG : 2;

            //! @brief Reserved
            uint8_t RESERVED2 : 1;

            //! @brief DEV_MODE_PINS[13] : Fail safe mode enable. Excludes power up fail safe
            uint8_t FAIL_SAFE_EN : 1;

            //! @brief DEV_MODE_PINS[15:14] : GPIO1 configuration
            //! Configures the mode of the GPIO1 pin as an input or output
            //! Available values are:
            //! TCAN4x5x_DEV_CONFIG_GPIO1_CONFIG_GPO : Sets GPIO1 as an output. Be sure to see GPIO1_GPO_CONFIG field to set the behavior
            //! TCAN4x5x_DEV_CONFIG_GPIO1_CONFIG_WATCHDOG_INPUT : Sets GPIO1 as an input for the watchdog timer. Watchdog will need to be enabled
            TCAN4x5x_DEV_CONFIG_GPIO1_CONFIG GPIO1_CONFIG : 2;

            //! @brief DEV_MODE_PINS[17:16] : Watchdog action. Defines the behavior of the watchdog timer when it times out
            //! Sets the behavior of the watchdog when it times out
            //! Available values are:
            //! TCAN4x5x_DEV_CONFIG_WDT_ACTION_nINT : Sets an interrupt flag and the interrupt pin will be pulled low
            //! TCAN4x5x_DEV_CONFIG_WDT_ACTION_PULSE_INH : Pulse INH low for ~300 ms then high
            //! TCAN4x5x_DEV_CONFIG_WDT_ACTION_PULSE_WDT_OUTPUT : Pulse the watchdog output pin low for ~300 ms and high
            TCAN4x5x_DEV_CONFIG_WDT_ACTION WD_ACTION : 2;


            //! @brief DEV_MODE_PINS[18] : Watchdog reset bit
            //! Write a 1 to reset the watchdog timer. It's recommended to use the other watchdog functions for this behavior
            uint8_t WD_BIT_RESET : 1;

            //! @brief DEV_MODE_PINS[19] : nWKRQ_VOLTAGE, set the voltage rail used by the nWKRQ pin
            //! Available values are:
            //! 0 [default] : Internal Voltage rail
            //! 1           : VIO Voltage rail
            uint8_t nWKRQ_VOLTAGE : 1;

            //! @brief DEV_MODE_PINS[21:20] : RESERVED. Use test mode functions to enable test modes
            uint8_t RESERVED3 : 2;

            //! @brief DEV_MODE_PINS[23:22] : nWKRQ_VOLTAGE, set the voltage rail used by the nWKRQ pin
            //! Available values are:
            //! TCAN4x5x_DEV_CONFIG_GPO2_NO_ACTION  [default] : No action for GPO2
            //! TCAN4x5x_DEV_CONFIG_GPO2_MCAN_INT0 : Used as an output for MCAN INT0
            //! TCAN4x5x_DEV_CONFIG_GPO2_WATCHDOG : Used as a watchdog output
            //! TCAN4x5x_DEV_CONFIG_GPO2_MIRROR_INT : Mirror nINT pin
            TCAN4x5x_DEV_CONFIG_GPO2_CONFIG GPO2_CONFIG : 2;

            //! @brief DEV_MODE_PINS[26:24] : RESERVED
            uint8_t RESERVED4 : 3;

            //! @brief DEV_MODE_PINS[27] : CLK_REF, used to tell the device what the input clock/crystal frequency is
            //! Available values are:
            //! 0           : 20 MHz
            //! 1 [default] : 40 MHz
            uint8_t CLK_REF : 1;

            //! @brief DEV_MODE_PINS[29:28] : RESERVED. Use watchdog functions to set watchdog parameters
            uint8_t RESERVED5 : 2;


            //! brief DEV_MODE_PINS[31:30] : WAKE_CONFIG, used to configure the direction required to wake a part up
            //! Available values are:
            //! TCAN4x5x_DEV_CONFIG_WAKE_DISABLED             : Disabled. Wake pin is not used
            //! TCAN4x5x_DEV_CONFIG_WAKE_RISING_EDGE          : Low to high transition will wake the part
            //! TCAN4x5x_DEV_CONFIG_WAKE_FALLING_EDGE         : High to low transition will wake the part
            //! TCAN4x5x_DEV_CONFIG_WAKE_BOTH_EDGES [default] : Either transition will wake the part
            TCAN4x5x_DEV_CONFIG_WAKE_CONFIG WAKE_CONFIG : 2;
        };
    };
} TCAN4x5x_DEV_CONFIG;


#endif /* TCAN4X5X_DATA_STRUCTS_H_ */
