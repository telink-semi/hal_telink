/********************************************************************************************************
 * @file    hdt_stack.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
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
#ifndef STACK_BLE_CONTROLLER_LL_HDT_HDT_STACK_H_
#define STACK_BLE_CONTROLLER_LL_HDT_HDT_STACK_H_

typedef enum
{
    HDT2M    = BIT(0),
    HDT3M    = BIT(1),
    HDT4M    = BIT(2),
    HDT6M    = BIT(3),
    HDT7P5M  = BIT(4),
} le_hdt_rate_type_t;

#ifndef HDT_PHY_RATES_SUPPORTED
    #define HDT_PHY_RATES_SUPPORTED                     (HDT2M | HDT3M | HDT4M | HDT6M | HDT7P5M)
#endif

#ifndef MAX_TX_PAYLOAD_PER_PACKET_SUPPORTED
    #define MAX_TX_PAYLOAD_PER_PACKET_SUPPORTED         4
#endif

#ifndef MAX_RX_PAYLOAD_PER_PACKET_SUPPORTED
    #define MAX_RX_PAYLOAD_PER_PACKET_SUPPORTED         4
#endif

#ifndef MAX_BLOCKS_PER_PACKET
#define MAX_BLOCKS_PER_PACKET                           16
#endif

typedef enum
{
    HDT_FORMAT_ALL  = 0x00,
    HDT_FORMAT0     = 0x01,
    HDT_FORMAT1     = 0x02,
} le_hdt_pfi_type_t;

#ifndef HDT_PACKET_FORMAT_SUPPORTED
#define HDT_PACKET_FORMAT_SUPPORTED                    HDT_FORMAT_ALL
#endif

typedef enum
{
    HDT_MIC64  = 0x01,
    HDT_MIC128 = 0x02,
} le_hdt_mic_length_type_t;

#ifndef HDT_MIC_DFT_LENGTH
#define HDT_MIC_DFT_LENGTH                             HDT_MIC128
#endif

typedef struct __attribute__((packed))
{
    u8  preferred_mic_length;//1,2
    u8  packet_format;//0,1,2
    u8  blocks_per_packet;//1-16
    u8  phy_rates;//bit(0)~bit(5)
    u16 max_tx_payload_window_size; // 1 or 4
    u16 max_rx_payload_window_size; // 1 or 4
    u16 MaxRxOct;
    u16 MaxTxOct;
    u16 MaxTxOct_master;
    u16 MaxTxOct_slave;
}hdt_defaultParam_t;

extern hdt_defaultParam_t  hdt_dftParam;

typedef enum
{
    HDT_RATE_SHORT_FORMAT   = 0x00,
    HDT_RATE_2M             = 0x01,
    HDT_RATE_3M             = 0x02,
    HDT_RATE_4M             = 0x03,
    HDT_RATE_6M             = 0x04,
    HDT_RATE_7P5M           = 0x05,
} le_hdtTest_rate_type_t;

typedef enum
{
    HDT_PKT_FORMAT0_SHORT_FORMAT    = 0x00,
    HDT_PKT_FORMAT1                 = 0x01,
} le_hdtTest_pfi_type_t;

typedef struct __attribute__((packed))
{
    unsigned char  hdtRateInd;
    unsigned char  hdtPfi;
    unsigned short hdtHeaderlen;
    unsigned char  hdtPhyIntvl;
    unsigned char  hdtNumBlocks[4];
    unsigned short  hdtBlockSize[4];
    unsigned short  hdtFinalBlockSize[4];
}hdt_testParam_t;

extern hdt_testParam_t  hdt_testParam;

typedef struct __attribute__((packed))
{
    u8  packet_format;//0,1,2
    u8  blocks_per_packet;//1-16
    u16 max_tx_payload_window_size; // 1 or 4
    u16 max_rx_payload_window_size; // 1 or 4
    u8  rsvd[2];

}hdt_param_t;


#endif /* STACK_BLE_CONTROLLER_LL_HDT_HDT_STACK_H_ */
