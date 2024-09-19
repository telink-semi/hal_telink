/********************************************************************************************************
 * @file    ble_stack.h
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
#ifndef BLE_STACK_H_
#define BLE_STACK_H_

#include "stack/ble/TL321X/ble_config.h"


/**
 *  @brief  Definition for Link Layer Feature Support
 */

#define LL_FEATURE_MASK_LL_ENCRYPTION                                   BIT(0) //core_4.0
#define LL_FEATURE_MASK_CONNECTION_PARA_REQUEST_PROCEDURE               BIT(1) //core_4.1
#define LL_FEATURE_MASK_EXTENDED_REJECT_INDICATION                      BIT(2) //core_4.1
#define LL_FEATURE_MASK_SLAVE_INITIATED_FEATURES_EXCHANGE               BIT(3) //core_4.1
#define LL_FEATURE_MASK_LE_PING                                         BIT(4) //core_4.1
#define LL_FEATURE_MASK_LE_DATA_LENGTH_EXTENSION                        BIT(5) //core_4.2
#define LL_FEATURE_MASK_LL_PRIVACY                                      BIT(6) //core_4.2
#define LL_FEATURE_MASK_EXTENDED_SCANNER_FILTER_POLICIES                BIT(7) //core_4.2

#define LL_FEATURE_MASK_LE_2M_PHY                                       BIT(8)  //core_5.0
#define LL_FEATURE_MASK_STABLE_MODULATION_INDEX_TX                      BIT(9)  //core_5.0
#define LL_FEATURE_MASK_STABLE_MODULATION_INDEX_RX                      BIT(10) //core_5.0
#define LL_FEATURE_MASK_LE_CODED_PHY                                    BIT(11) //core_5.0
#define LL_FEATURE_MASK_LE_EXTENDED_ADVERTISING                         BIT(12) //core_5.0
#define LL_FEATURE_MASK_LE_PERIODIC_ADVERTISING                         BIT(13) //core_5.0
#define LL_FEATURE_MASK_CHANNEL_SELECTION_ALGORITHM2                    BIT(14) //core_5.0
#define LL_FEATURE_MASK_LE_POWER_CLASS_1                                BIT(15) //core_5.0
#define LL_FEATURE_MASK_MIN_USED_OF_USED_CHANNELS                       BIT(16) //core_5.0

#define LL_FEATURE_MASK_CONNECTION_CTE_REQUEST                          BIT(17) //core_5.1
#define LL_FEATURE_MASK_CONNECTION_CTE_RESPONSE                         BIT(18) //core_5.1
#define LL_FEATURE_MASK_CONNECTIONLESS_CTE_TRANSMITTER                  BIT(19) //core_5.1
#define LL_FEATURE_MASK_CONNECTIONLESS_CTE_RECEIVER                     BIT(20) //core_5.1
#define LL_FEATURE_MASK_ANTENNA_SWITCHING_DURING_CTE_TRANSMISSION       BIT(21) //core_5.1
#define LL_FEATURE_MASK_ANTENNA_SWITCHING_DURING_CTE_RECEPTION          BIT(22) //core_5.1
#define LL_FEATURE_MASK_RECEIVING_CONSTANT_TONE_EXTENSIONS              BIT(23) //core_5.1
#define LL_FEATURE_MASK_PERIODIC_ADVERTISING_SYNC_TRANSFER_SENDER       BIT(24) //core_5.1
#define LL_FEATURE_MASK_PERIODIC_ADVERTISING_SYNC_TRANSFER_RECIPIENT    BIT(25) //core_5.1
#define LL_FEATURE_MASK_SLEEP_CLOCK_ACCURACY_UPDATES                    BIT(26) //core_5.1
#define LL_FEATURE_MASK_REMOTE_PUBLIC_KEY_VALIDATION                    BIT(27) //core_5.1

#define LL_FEATURE_MASK_CONNECTED_ISOCHRONOUS_STREAM_MASTER             BIT(28) //core_5.2
#define LL_FEATURE_MASK_CONNECTED_ISOCHRONOUS_STREAM_SLAVE              BIT(29) //core_5.2
#define LL_FEATURE_MASK_ISOCHRONOUS_BROADCASTER                         BIT(30) //core_5.2
#define LL_FEATURE_MASK_SYNCHRONIZED_RECEIVER                           BIT(31) //core_5.2
#define LL_FEATURE_MASK_ISOCHRONOUS_CHANNELS                            BIT(0)  //core_5.2
#define LL_FEATURE_MASK_LE_POWER_CTRL_REQUEST                           BIT(1)  //core_5.2
#define LL_FEATURE_MASK_LE_POWER_CHANGE_INDICATION                      BIT(2)  //core_5.2
#define LL_FEATURE_MASK_LE_PATH_LOSS_MONITORING                         BIT(3)  //core_5.2


#define LL_FEATURE_MASK_PERIODIC_ADV_ADI_SUPPORT                        BIT(4)  //bit(36)   core_5.3
#define LL_FEATURE_MASK_CONNECTION_SUBRATING                            BIT(5)  //bit(37)   core_5.3
#define LL_FEATURE_MASK_CONNECTION_SUBRATING_HOST                       BIT(6)  //bit(38)   core_5.3
#define LL_FEATURE_MASK_CHANNEL_CLASSIFICATION                          BIT(7)  //bit(39)   core_5.3


#define LL_FEATURE_MASK_ADVERTISING_CODING_SELECTION                    BIT(8)  //bit(40)   core_5.4
#define LL_FEATURE_MASK_ADVERTISING_CODING_SELECT_HOST_SUPPORT          BIT(9)  //bit(41)   core_5.4
#define LL_FEATURE_MASK_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER  BIT(11) //bit(43)   core_5.4
#define LL_FEATURE_MASK_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER     BIT(12) //bit(44)   core_5.4



#define LL_FEATURE_MASK_CHANNEL_SOUNDING                                BIT(14) //bit(46)   core_5.X   channel sounding
#define LL_FEATURE_MASK_CHANNEL_SOUNDING_HOST                           BIT(15) //bit(47)
#define LL_FEATURE_MASK_CHANNEL_SOUNDING_PCT_QUALITY_INDICATION         BIT(16) //bit(48)
/////////////////////////////////////////////////////////////////////////////

#define         VENDOR_ID                       0x0211
#define         VENDOR_ID_HI_B                  U16_HI(VENDOR_ID)
#define         VENDOR_ID_LO_B                  U16_LO(VENDOR_ID)

#define         BLUETOOTH_VER_4_0               6
#define         BLUETOOTH_VER_4_1               7
#define         BLUETOOTH_VER_4_2               8
#define         BLUETOOTH_VER_5_0               9
#define         BLUETOOTH_VER_5_1               10
#define         BLUETOOTH_VER_5_2               11
#define         BLUETOOTH_VER_5_3               12
#define         BLUETOOTH_VER_5_4               13
#define         BLUETOOTH_VER_5_X               14


#ifndef         BLUETOOTH_VER
#define         BLUETOOTH_VER                   BLUETOOTH_VER_5_3
#endif


#if (BLUETOOTH_VER == BLUETOOTH_VER_4_2)
    #define         BLUETOOTH_VER_SUBVER            0x22BB
#elif (BLUETOOTH_VER == BLUETOOTH_VER_5_0)
    #define         BLUETOOTH_VER_SUBVER            0x1C1C
#else
    #define         BLUETOOTH_VER_SUBVER            0x4103
#endif







#if (BLUETOOTH_VER == BLUETOOTH_VER_4_0)
    #define LL_CMD_MAX                                                  LL_REJECT_IND
#elif (BLUETOOTH_VER == BLUETOOTH_VER_4_1)
    #define LL_CMD_MAX                                                  LL_PING_RSP
#elif (BLUETOOTH_VER == BLUETOOTH_VER_4_2)
    #define LL_CMD_MAX                                                  LL_LENGTH_RSP
#elif (BLUETOOTH_VER == BLUETOOTH_VER_5_0)
    #define LL_CMD_MAX                                                  LL_MIN_USED_CHN_IND
#elif (BLUETOOTH_VER == BLUETOOTH_VER_5_1)
    #define LL_CMD_MAX                                                  LL_CLOCK_ACCURACY_RSP
#elif (BLUETOOTH_VER == BLUETOOTH_VER_5_2)
    #define LL_CMD_MAX                                                  LL_POWER_CHANGE_IND
#elif (BLUETOOTH_VER == BLUETOOTH_VER_5_3)
    #define LL_CMD_MAX                                                  LL_CS_SEC_REQ//LL_CHANNEL_STATUS_IND
#elif (BLUETOOTH_VER == BLUETOOTH_VER_5_4)
    #define LL_CMD_MAX                                                  LL_PERIODIC_SYNC_WR_IND
#elif (BLUETOOTH_VER == BLUETOOTH_VER_5_X)
    #define LL_CMD_MAX                                                  LL_CS_SEC_REQ
#endif





#if (BLUETOOTH_VER >= BLUETOOTH_VER_4_0)
	#define LL_FEATURE_ENABLE_LE_ENCRYPTION								LL_FEATURE_SUPPORT_LE_ENCRYPTION
#endif


#if (BLUETOOTH_VER >= BLUETOOTH_VER_4_1)
    #define LL_FEATURE_ENABLE_EXTENDED_REJECT_INDICATION                1
    #define LL_FEATURE_ENABLE_SLAVE_INITIATED_FEATURES_EXCHANGE         1
	#define	LL_FEATURE_ENABLE_LE_PING									LL_FEATURE_SUPPORT_LE_PING
#endif


#if (BLUETOOTH_VER >= BLUETOOTH_VER_4_2)
    #define LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION                  LL_FEATURE_SUPPORT_LE_DATA_LENGTH_EXTENSION

    /* support privacy & RPA from BLE 4.2, do not consider previous version */
    #define LL_FEATURE_ENABLE_PRIVACY                                       LL_FEATURE_SUPPORT_PRIVACY
#endif


#if (BLUETOOTH_VER >= BLUETOOTH_VER_5_0)
    #define LL_FEATURE_ENABLE_EXTENDED_SCANNER_FILTER_POLICIES          LL_FEATURE_SUPPORT_EXTENDED_SCANNER_FILTER_POLICIES
    #define LL_FEATURE_ENABLE_LE_2M_PHY                                 LL_FEATURE_SUPPORT_LE_2M_PHY
    #define LL_FEATURE_ENABLE_LE_CODED_PHY                              LL_FEATURE_SUPPORT_LE_CODED_PHY
    #define LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING                   LL_FEATURE_SUPPORT_LE_EXTENDED_ADVERTISING
    #define LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING                   LL_FEATURE_SUPPORT_LE_PERIODIC_ADVERTISING
    #define LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2              LL_FEATURE_SUPPORT_CHANNEL_SELECTION_ALGORITHM2
    #define LL_FEATURE_ENABLE_MIN_USED_OF_USED_CHANNELS                 LL_FEATURE_SUPPORT_MIN_USED_OF_USED_CHANNELS

    #define LL_FEATURE_ENABLE_LE_EXTENDED_SCAN                          LL_FEATURE_SUPPORT_LE_EXTENDED_SCANNING  //Vendor define
    #define LL_FEATURE_ENABLE_LE_EXTENDED_INITIATE                      LL_FEATURE_SUPPORT_LE_EXTENDED_INITIATE  //Vendor define
    #define LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC              LL_FEATURE_SUPPORT_LE_PERIODIC_ADVERTISING_SYNC
#endif


#if (BLUETOOTH_VER >= BLUETOOTH_VER_5_1)
    #define LL_FEATURE_ENABLE_LE_AOA_AOD                                LL_FEATURE_SUPPORT_LE_AOA_AOD
    #define LL_FEATURE_ENABLE_CONNECTION_CTE_REQUEST                    LL_FEATURE_SUPPORT_CONNECTION_CTE_REQUEST
    #define LL_FEATURE_ENABLE_CONNECTION_CTE_RESPONSE                   LL_FEATURE_SUPPORT_CONNECTION_CTE_RESPONSE
    #define LL_FEATURE_ENABLE_CONNECTIONLESS_CTE_TRANSMITTER            LL_FEATURE_SUPPORT_CONNECTIONLESS_CTE_TRANSMITTER
    #define LL_FEATURE_ENABLE_CONNECTIONLESS_CTE_RECEIVER               LL_FEATURE_SUPPORT_CONNECTIONLESS_CTE_RECEIVER
    #define LL_FEATURE_ENABLE_ANTENNA_SWITCHING_CTE_TRANSMISSION_AOD    LL_FEATURE_SUPPORT_ANTENNA_SWITCHING_CTE_TRANSMISSION_AOD
    #define LL_FEATURE_ENABLE_ANTENNA_SWITCHING_CTE_RECEPTION_AOA       LL_FEATURE_SUPPORT_ANTENNA_SWITCHING_CTE_RECEPTION_AOA
    #define LL_FEATURE_ENABLE_RECEIVING_CONSTANT_TONE_EXTENSIONS        LL_FEATURE_SUPPORT_RECEIVING_CONSTANT_TONE_EXTENSIONS
    #define LL_FEATURE_ENABLE_LE_PAST_SENDER                            LL_FEATURE_SUPPORT_LE_PAST_SENDER
    #define LL_FEATURE_ENABLE_LE_PAST_RECIPIENT                         LL_FEATURE_SUPPORT_LE_PAST_RECIPIENT

    #define LL_FEATURE_ENABLE_PAST                                      (     LL_FEATURE_SUPPORT_LE_PAST_SENDER \
                                                                            | LL_FEATURE_SUPPORT_LE_PAST_RECIPIENT )
#endif


#if (BLUETOOTH_VER >= BLUETOOTH_VER_5_2)
    #define LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER       LL_FEATURE_SUPPORT_CONNECTED_ISOCHRONOUS_STREAM_MASTER
    #define LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE        LL_FEATURE_SUPPORT_CONNECTED_ISOCHRONOUS_STREAM_SLAVE
    #define LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER                   LL_FEATURE_SUPPORT_ISOCHRONOUS_BROADCASTER
    #define LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER                     LL_FEATURE_SUPPORT_SYNCHRONIZED_RECEIVER
    #define LL_FEATURE_ENABLE_ISOCHRONOUS_CHANNELS                      LL_FEATURE_SUPPORT_ISOCHRONOUS_CHANNELS
    #define LL_FEATURE_ENABLE_POWER_CONTROL_REQUEST                     LL_FEATURE_SUPPORT_POWER_CONTROL_REQUEST
    #define LL_FEATURE_ENABLE_POWER_LOSS_MONITORING                     LL_FEATURE_SUPPORT_POWER_LOSS_MONITORING



    #define LL_FEATURE_ENABLE_POWER_CONTROL                             (     LL_FEATURE_SUPPORT_POWER_CONTROL_REQUEST \
                                                                            | LL_FEATURE_SUPPORT_POWER_LOSS_MONITORING )

    #define LL_FEATURE_ENABLE_CONNECTED_ISO                             (     LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER \
                                                                            | LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE )

    #define LL_FEATURE_ENABLE_CONNECTIONLESS_ISO                        (     LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER             \
                                                                            | LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER )

    #define LL_FEATURE_ENABLE_ISO                                       (     LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER \
                                                                            | LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE \
                                                                            | LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER             \
                                                                            | LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER )

    #define LL_FEATURE_ENABLE_ISOCHRONOUS_TEST_MODE                     LL_FEATURE_SUPPORT_ISOCHRONOUS_TEST_MODE //Vendor define
#endif


#if (BLUETOOTH_VER >= BLUETOOTH_VER_5_3)
    #define LL_FEATURE_ENABLE_SLEEP_CLK_ACCURACY_UPDATE                 LL_FEATURE_SUPPORT_SLEEP_CLK_ACCURACY_UPDATE
    #define LL_FEATURE_ENABLE_REMOTE_PUBLIC_KEY_VALIDATION              LL_FEATURE_SUPPORT_REMOTE_PUBLIC_KEY_VALIDATION

    #define LL_FEATURE_ENABLE_PERIODIC_ADV_ADI_SUPPORT                  LL_FEATURE_SUPPORT_PERIODIC_ADV_ADI_SUPPORT
    #define LL_FEATURE_ENABLE_CONNECTION_SUBRATING                      LL_FEATURE_SUPPORT_CONNECTION_SUBRATING
    #define LL_FEATURE_ENABLE_CONNECTION_SUBRATING_HOST                 LL_FEATURE_SUPPORT_CONNECTION_SUBRATING_HOST
    #define LL_FEATURE_ENABLE_CHANNEL_CLASSIFICATION                    LL_FEATURE_SUPPORT_CHANNEL_CLASSIFICATION
    #define LL_FEATURE_ENABLE_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE       LL_FEATURE_SUPPORT_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE



    //special: Google project need future feature CS, but we only support core _5.3
    #define LL_FEATURE_ENABLE_CHANNEL_SOUNDING_INITIATOR                                    LL_FEATURE_SUPPORT_CHANNEL_SOUNDING_INITIATOR
    #define LL_FEATURE_ENABLE_CHANNEL_SOUNDING_REFLECTOR                                    LL_FEATURE_SUPPORT_CHANNEL_SOUNDING_REFLECTOR

    #define LL_FEATURE_ENABLE_CHANNEL_SOUNDING                                              (LL_FEATURE_SUPPORT_CHANNEL_SOUNDING_REFLECTOR ||\
                                                                                                LL_FEATURE_ENABLE_CHANNEL_SOUNDING_INITIATOR)

#endif



#if (BLUETOOTH_VER >= BLUETOOTH_VER_5_4)
    #define LL_FEATURE_ENABLE_ADVERTISING_CODING_SELECTION                      LL_FEATURE_SUPPORT_ADVERTISING_CODING_SELECTION
    #define LL_FEATURE_ENABLE_ADVERTISING_CODING_SELECT_HOST_SUPPORT            LL_FEATURE_SUPPORT_ADVERTISING_CODING_SELECT_HOST_SUPPORT
    #define LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER    LL_FEATURE_SUPPORT_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER
    #define LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER       LL_FEATURE_SUPPORT_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER
#endif


#if (BLUETOOTH_VER >= BLUETOOTH_VER_5_X)
    #define LL_FEATURE_ENABLE_CHANNEL_SOUNDING_INITIATOR                                    LL_FEATURE_SUPPORT_CHANNEL_SOUNDING_INITIATOR
    #define LL_FEATURE_ENABLE_CHANNEL_SOUNDING_REFLECTOR                                    LL_FEATURE_SUPPORT_CHANNEL_SOUNDING_REFLECTOR


    #define LL_FEATURE_ENABLE_CHANNEL_SOUNDING                                              (LL_FEATURE_SUPPORT_CHANNEL_SOUNDING_REFLECTOR ||\
                                                                                                LL_FEATURE_ENABLE_CHANNEL_SOUNDING_INITIATOR)

#endif



#ifndef      LL_FEATURE_ENABLE_LE_ENCRYPTION
#define      LL_FEATURE_ENABLE_LE_ENCRYPTION                            0
#endif

#ifndef      LL_FEATURE_ENABLE_CONNECTION_PARA_REQUEST_PROCEDURE
#define      LL_FEATURE_ENABLE_CONNECTION_PARA_REQUEST_PROCEDURE        0
#endif

#ifndef      LL_FEATURE_ENABLE_EXTENDED_REJECT_INDICATION
#define      LL_FEATURE_ENABLE_EXTENDED_REJECT_INDICATION               0
#endif

#ifndef      LL_FEATURE_ENABLE_SLAVE_INITIATED_FEATURES_EXCHANGE
#define      LL_FEATURE_ENABLE_SLAVE_INITIATED_FEATURES_EXCHANGE        0
#endif

#ifndef      LL_FEATURE_ENABLE_LE_PING
#define      LL_FEATURE_ENABLE_LE_PING                                  0
#endif

#ifndef      LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION
#define      LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION                 0
#endif

#ifndef      LL_FEATURE_ENABLE_LL_PRIVACY
#define      LL_FEATURE_ENABLE_LL_PRIVACY                               0
#endif

#ifndef      LL_FEATURE_ENABLE_EXTENDED_SCANNER_FILTER_POLICIES
#define      LL_FEATURE_ENABLE_EXTENDED_SCANNER_FILTER_POLICIES         0
#endif

#ifndef      LL_FEATURE_ENABLE_LE_2M_PHY
#define      LL_FEATURE_ENABLE_LE_2M_PHY                                0
#endif

#ifndef      LL_FEATURE_ENABLE_STABLE_MODULATION_INDEX_TX
#define      LL_FEATURE_ENABLE_STABLE_MODULATION_INDEX_TX               0
#endif

#ifndef      LL_FEATURE_ENABLE_STABLE_MODULATION_INDEX_RX
#define      LL_FEATURE_ENABLE_STABLE_MODULATION_INDEX_RX               0
#endif

#ifndef      LL_FEATURE_ENABLE_LE_CODED_PHY
#define      LL_FEATURE_ENABLE_LE_CODED_PHY                             0
#endif

#ifndef      LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING
#define      LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING                  0
#endif

#ifndef      LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING
#define      LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING                  0
#endif

#ifndef      LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2
#define      LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2             0
#endif

#ifndef      LL_FEATURE_ENABLE_MIN_USED_OF_USED_CHANNELS
#define      LL_FEATURE_ENABLE_MIN_USED_OF_USED_CHANNELS                0
#endif

#ifndef      LL_FEATURE_ENABLE_LE_AOA_AOD
#define      LL_FEATURE_ENABLE_LE_AOA_AOD                               0
#endif

#ifndef      LL_FEATURE_ENABLE_CONNECTION_CTE_REQUEST
#define      LL_FEATURE_ENABLE_CONNECTION_CTE_REQUEST                   0
#endif

#ifndef      LL_FEATURE_ENABLE_CONNECTION_CTE_RESPONSE
#define      LL_FEATURE_ENABLE_CONNECTION_CTE_RESPONSE                  0
#endif

#ifndef      LL_FEATURE_ENABLE_CONNECTIONLESS_CTE_TRANSMITTER
#define      LL_FEATURE_ENABLE_CONNECTIONLESS_CTE_TRANSMITTER           0
#endif

#ifndef      LL_FEATURE_ENABLE_CONNECTIONLESS_CTE_RECEIVER
#define      LL_FEATURE_ENABLE_CONNECTIONLESS_CTE_RECEIVER              0
#endif

#ifndef      LL_FEATURE_ENABLE_ANTENNA_SWITCHING_CTE_TRANSMISSION_AOD
#define      LL_FEATURE_ENABLE_ANTENNA_SWITCHING_CTE_TRANSMISSION_AOD   0
#endif

#ifndef      LL_FEATURE_ENABLE_ANTENNA_SWITCHING_CTE_RECEPTION_AOA
#define      LL_FEATURE_ENABLE_ANTENNA_SWITCHING_CTE_RECEPTION_AOA      0
#endif

#ifndef      LL_FEATURE_ENABLE_RECEIVING_CONSTANT_TONE_EXTENSIONS
#define      LL_FEATURE_ENABLE_RECEIVING_CONSTANT_TONE_EXTENSIONS       0
#endif

#ifndef      LL_FEATURE_ENABLE_LE_PAST_SENDER
#define      LL_FEATURE_ENABLE_LE_PAST_SENDER                           0
#endif

#ifndef      LL_FEATURE_ENABLE_LE_PAST_RECIPIENT
#define      LL_FEATURE_ENABLE_LE_PAST_RECIPIENT                        0
#endif

#ifndef     LL_FEATURE_ENABLE_SLEEP_CLK_ACCURACY_UPDATE
#define     LL_FEATURE_ENABLE_SLEEP_CLK_ACCURACY_UPDATE                 0
#endif

#ifndef     LL_FEATURE_ENABLE_REMOTE_PUBLIC_KEY_VALIDATION
#define     LL_FEATURE_ENABLE_REMOTE_PUBLIC_KEY_VALIDATION              0
#endif

#ifndef      LL_FEATURE_ENABLE_LE_POWER_CLASS_1
#define      LL_FEATURE_ENABLE_LE_POWER_CLASS_1                         0
#endif

#ifndef      LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER
#define      LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER      0
#endif

#ifndef      LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE
#define      LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE       0
#endif

#ifndef      LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER
#define      LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER                  0
#endif

#ifndef      LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER
#define      LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER                    0
#endif

#ifndef      LL_FEATURE_ENABLE_ISOCHRONOUS_CHANNELS
#define      LL_FEATURE_ENABLE_ISOCHRONOUS_CHANNELS                     0
#endif

#ifndef      LL_FEATURE_ENABLE_POWER_CONTROL_REQUEST
#define      LL_FEATURE_ENABLE_POWER_CONTROL_REQUEST                    0
#endif

#ifndef      LL_FEATURE_ENABLE_POWER_LOSS_MONITORING
#define      LL_FEATURE_ENABLE_POWER_LOSS_MONITORING                    0
#endif

#ifndef      LL_FEATURE_ENABLE_PAST
#define      LL_FEATURE_ENABLE_PAST                                     0
#endif

#ifndef      LL_FEATURE_ENABLE_POWER_CONTROL
#define      LL_FEATURE_ENABLE_POWER_CONTROL                            0
#endif

#ifndef      LL_FEATURE_ENABLE_CONNECTED_ISO
#define      LL_FEATURE_ENABLE_CONNECTED_ISO                            0
#endif

#ifndef      LL_FEATURE_ENABLE_CONNECTIONLESS_ISO
#define      LL_FEATURE_ENABLE_CONNECTIONLESS_ISO                       0
#endif

#ifndef      LL_FEATURE_ENABLE_ISO
#define      LL_FEATURE_ENABLE_ISO                                      0
#endif

#ifndef      LL_FEATURE_ENABLE_LE_EXTENDED_SCAN
#define      LL_FEATURE_ENABLE_LE_EXTENDED_SCAN                         0
#endif

#ifndef      LL_FEATURE_ENABLE_LE_EXTENDED_INITIATE
#define      LL_FEATURE_ENABLE_LE_EXTENDED_INITIATE                     0
#endif

#ifndef      LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC
#define      LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC             0
#endif

#ifndef      LL_FEATURE_ENABLE_ISOCHRONOUS_TEST_MODE
#define      LL_FEATURE_ENABLE_ISOCHRONOUS_TEST_MODE                    0
#endif

#ifndef      LL_FEATURE_ENABLE_PERIODIC_ADV_ADI_SUPPORT
#define      LL_FEATURE_ENABLE_PERIODIC_ADV_ADI_SUPPORT                 0
#endif

#ifndef      LL_FEATURE_ENABLE_CONNECTION_SUBRATING
#define      LL_FEATURE_ENABLE_CONNECTION_SUBRATING                     0
#endif

#ifndef      LL_FEATURE_ENABLE_CONNECTION_SUBRATING_HOST
#define      LL_FEATURE_ENABLE_CONNECTION_SUBRATING_HOST                0
#endif

#ifndef      LL_FEATURE_ENABLE_CHANNEL_CLASSIFICATION
#define      LL_FEATURE_ENABLE_CHANNEL_CLASSIFICATION                   0
#endif




//core_5.4 begin
#ifndef LL_FEATURE_ENABLE_ADVERTISING_CODING_SELECTION
#define LL_FEATURE_ENABLE_ADVERTISING_CODING_SELECTION                      0
#endif

#ifndef LL_FEATURE_ENABLE_ADVERTISING_CODING_SELECT_HOST_SUPPORT
#define LL_FEATURE_ENABLE_ADVERTISING_CODING_SELECT_HOST_SUPPORT            0
#endif

#ifndef LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER
#define LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER    0
#endif

#ifndef LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER
#define LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER       0
#endif
//core_5.4 end

//core 5.x
#ifndef LL_FEATURE_ENABLE_CHANNEL_SOUNDING_INITIATOR
#define LL_FEATURE_ENABLE_CHANNEL_SOUNDING_INITIATOR                                    0
#endif

#ifndef LL_FEATURE_ENABLE_CHANNEL_SOUNDING_REFLECTOR
#define LL_FEATURE_ENABLE_CHANNEL_SOUNDING_REFLECTOR                                    0
#endif

#ifndef LL_FEATURE_ENABLE_CHANNEL_SOUNDING
#define LL_FEATURE_ENABLE_CHANNEL_SOUNDING                                              (LL_FEATURE_ENABLE_CHANNEL_SOUNDING_REFLECTOR ||\
                                                                                                LL_FEATURE_ENABLE_CHANNEL_SOUNDING_INITIATOR)
#endif

//BIT<0:31>
// feature below is conFiged by application or HCI command, corresponding bit must be 0
// <8>  : LL_FEATURE_ENABLE_LE_2M_PHY
// <11> : LL_FEATURE_ENABLE_LE_CODED_PHY
// <12> : LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING
// <13> : LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING
// <14> : LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2
// <19> : LL_FEATURE_ENABLE_CONNECTIONLESS_CTE_TRANSMITTER
// <20> : LL_FEATURE_ENABLE_CONNECTIONLESS_CTE_RECEIVER
// <21> : LL_FEATURE_ENABLE_ANTENNA_SWITCHING_CTE_TRANSMISSION_AOD
// <22> : LL_FEATURE_ENABLE_ANTENNA_SWITCHING_CTE_RECEPTION_AOA
// <23> : LL_FEATURE_ENABLE_RECEIVING_CONSTANT_TONE_EXTENSIONS
// <24> : LL_FEATURE_ENABLE_LE_PAST_SENDER
// <25> : LL_FEATURE_ENABLE_LE_PAST_RECIPIENT
// <28> : LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER
// <29> : LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE
// <30> : LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER
// <31> : LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER

#define LL_FEATURE_MASK_BASE0                                       (   LL_FEATURE_ENABLE_LE_ENCRYPTION                     <<0     |  \
                                                                        LL_FEATURE_ENABLE_CONNECTION_PARA_REQUEST_PROCEDURE <<1     |  \
                                                                        LL_FEATURE_ENABLE_EXTENDED_REJECT_INDICATION        <<2     |  \
                                                                        LL_FEATURE_ENABLE_SLAVE_INITIATED_FEATURES_EXCHANGE <<3     |  \
                                                                        LL_FEATURE_ENABLE_LE_PING                           <<4     |  \
                                                                        LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION          <<5     |  \
                                                                        LL_FEATURE_ENABLE_PRIVACY                           <<6     |  \
                                                                        LL_FEATURE_ENABLE_EXTENDED_SCANNER_FILTER_POLICIES  <<7     |  \
                                                                        0                                                   <<8     |  \
                                                                        LL_FEATURE_ENABLE_STABLE_MODULATION_INDEX_TX        <<9     |  \
                                                                        LL_FEATURE_ENABLE_STABLE_MODULATION_INDEX_RX        <<10    |  \
                                                                        0                                                   <<11    |  \
                                                                        0                                                   <<12    |  \
                                                                        0                                                   <<13    |  \
                                                                        0                                                   <<14    |  \
                                                                        LL_FEATURE_ENABLE_LE_POWER_CLASS_1                  <<15    |  \
                                                                        LL_FEATURE_ENABLE_MIN_USED_OF_USED_CHANNELS         <<16    |  \
                                                                        LL_FEATURE_ENABLE_CONNECTION_CTE_REQUEST            <<17    |  \
                                                                        LL_FEATURE_ENABLE_CONNECTION_CTE_RESPONSE           <<18    |  \
                                                                        0                                                   <<19    |  \
                                                                        0                                                   <<20    |  \
                                                                        0                                                   <<21    |  \
                                                                        0                                                   <<22    |  \
                                                                        0                                                   <<23    |  \
                                                                        0                                                   <<24    |  \
                                                                        0                                                   <<25    |  \
                                                                        LL_FEATURE_ENABLE_SLEEP_CLK_ACCURACY_UPDATE         <<26    |  \
                                                                        LL_FEATURE_ENABLE_REMOTE_PUBLIC_KEY_VALIDATION      <<27    |  \
                                                                        0                                                   <<28    |  \
                                                                        0                                                   <<29    |  \
                                                                        0                                                   <<30    |  \
                                                                        0                                                   <<31    )
//BIT<32:63>
// feature below is conFiged by application or HCI command, corresponding bit must be 0
// <32> :   LL_FEATURE_ENABLE_ISOCHRONOUS_CHANNELS  //Attention: Set by Host, should clear when HCI_reset !!!
// <33> :   LL_FEATURE_ENABLE_POWER_CONTROL_REQUEST
// <34> :   LL_FEATURE_ENABLE_POWER_CONTROL_REQUEST
// <35> :   LL_FEATURE_ENABLE_POWER_LOSS_MONITORING
// <37> :   LL_FEATURE_ENABLE_CONNECTION_SUBRATING
// <39> :   LL_FEATURE_ENABLE_CHANNEL_CLASSIFICATION
#define LL_FEATURE_MASK_BASE1                                       (   0                                                    <<0 | \
                                                                        0                                                    <<1 | \
                                                                        0                                                    <<2 | \
                                                                        0                                                    <<3 | \
                                                                        LL_FEATURE_ENABLE_PERIODIC_ADV_ADI_SUPPORT           <<4 | \
                                                                        0                                                    <<5 | \
                                                                        0                                                    <<7 | \
                                                                        LL_FEATURE_ENABLE_ADVERTISING_CODING_SELECTION       <<8 | \
                                                                        LL_FEATURE_ENABLE_ADVERTISING_CODING_SELECT_HOST_SUPPORT         <<9 | \
                                                                        LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER <<11 | \
                                                                        LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER    <<12)


extern u32 LL_FEATURE_MASK_0;
extern u32 LL_FEATURE_MASK_1;



#define LL_FEATURE_BYTE_0                                               U32_BYTE0(LL_FEATURE_MASK_0)
#define LL_FEATURE_BYTE_1                                               U32_BYTE1(LL_FEATURE_MASK_0)
#define LL_FEATURE_BYTE_2                                               U32_BYTE2(LL_FEATURE_MASK_0)
#define LL_FEATURE_BYTE_3                                               U32_BYTE3(LL_FEATURE_MASK_0)
#define LL_FEATURE_BYTE_4                                               U32_BYTE0(LL_FEATURE_MASK_1)
#define LL_FEATURE_BYTE_5                                               U32_BYTE1(LL_FEATURE_MASK_1)
#define LL_FEATURE_BYTE_6                                               U32_BYTE2(LL_FEATURE_MASK_1)
#define LL_FEATURE_BYTE_7                                               U32_BYTE3(LL_FEATURE_MASK_1)


















#if (LL_FEATURE_ENABLE_PRIVACY)
    #define LL_FEATURE_ENABLE_LOCAL_RPA                                 LL_FEATURE_SUPPORT_LOCAL_RPA
#endif


#ifndef      LL_FEATURE_ENABLE_LOCAL_RPA
#define      LL_FEATURE_ENABLE_LOCAL_RPA                                0
#endif


#ifndef      LL_FEATURE_ENABLE_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE
#define      LL_FEATURE_ENABLE_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE      0
#endif







/******************************************** Link Layer **************************************************************/
#define         BLE_T_IFS                                       150 //unit: uS
#define         MIN_T_MSS                                       150 //unit: uS  minimum value of T_MSS
#define         BLE_T_MAFS                                      300 //unit: uS


#define         SCA_MASTER_SLAVE_251_500_PPM                    0
#define         SCA_MASTER_SLAVE_151_250_PPM                    1
#define         SCA_MASTER_SLAVE_101_150_PPM                    2
#define         SCA_MASTER_SLAVE_76_100_PPM                     3
#define         SCA_MASTER_SLAVE_51_75_PPM                      4
#define         SCA_MASTER_SLAVE_31_50_PPM                      5
#define         SCA_MASTER_SLAVE_21_30_PPM                      6
#define         SCA_MASTER_SLAVE_0_20_PPM                       7


/**
 *  @brief  Definition for LL Control PDU Opcode
 */                                                                     // rf_len without MIC
#define                 LL_CONNECTION_UPDATE_REQ    0x00                            // 12           12
#define                 LL_CHANNEL_MAP_REQ          0x01                            //  8           8
#define                 LL_TERMINATE_IND            0x02                            //  2

#define                 LL_ENC_REQ                  0x03    // encryption           // 23
#define                 LL_ENC_RSP                  0x04    // encryption           // 13
#define                 LL_START_ENC_REQ            0x05    // encryption           //  1
#define                 LL_START_ENC_RSP            0x06    // encryption           //  1

#define                 LL_UNKNOWN_RSP              0x07                            //  2
#define                 LL_FEATURE_REQ              0x08                            //  9
#define                 LL_FEATURE_RSP              0x09                            //  9

#define                 LL_PAUSE_ENC_REQ            0x0A    // encryption           //  1
#define                 LL_PAUSE_ENC_RSP            0x0B    // encryption           //  1

#define                 LL_VERSION_IND              0x0C                            //  6
#define                 LL_REJECT_IND               0x0D                            //  2
#define                 LL_SLAVE_FEATURE_REQ        0x0E    //core_4.1              //  9
#define                 LL_CONNECTION_PARAM_REQ     0x0F    //core_4.1              // 24
#define                 LL_CONNECTION_PARAM_RSP     0x10    //core_4.1              // 24
#define                 LL_REJECT_IND_EXT           0x11    //core_4.1              //  3
#define                 LL_PING_REQ                 0x12    //core_4.1              //  1
#define                 LL_PING_RSP                 0x13    //core_4.1              //  1
#define                 LL_LENGTH_REQ               0x14    //core_4.2              //  9
#define                 LL_LENGTH_RSP               0x15    //core_4.2              //  9
#define                 LL_PHY_REQ                  0x16    //core_5.0              //  3
#define                 LL_PHY_RSP                  0x17    //core_5.0              //  3
#define                 LL_PHY_UPDATE_IND           0x18    //core_5.0              //  5           5
#define                 LL_MIN_USED_CHN_IND         0x19    //core_5.0              //  3

#define                 LL_CTE_REQ                  0x1A    //core_5.1              //  2
#define                 LL_CTE_RSP                  0x1B    //core_5.1              //  2
#define                 LL_PERIODIC_SYNC_IND        0x1C    //core_5.1              // 35
#define                 LL_CLOCK_ACCURACY_REQ       0x1D    //core_5.1              //  2
#define                 LL_CLOCK_ACCURACY_RSP       0x1E    //core_5.1              //  2


#define                 LL_CIS_REQ                  0x1F    //core_5.2              //  36      bigger than 27 !!!
#define                 LL_CIS_RSP                  0x20    //core_5.2              //  9
#define                 LL_CIS_IND                  0x21    //core_5.2              //  16
#define                 LL_CIS_TERMINATE_IND        0x22    //core_5.2              //  4
#define                 LL_POWER_CONTROL_REQ        0x23    //core_5.2              //  4
#define                 LL_POWER_CONTROL_RSP        0x24    //core_5.2              //  5           5
#define                 LL_POWER_CHANGE_IND         0x25    //core_5.2              //  5           5

#define                 LL_SUBRATE_REQ              0x26    //core_5.3              //  11
#define                 LL_SUBRATE_IND              0x27    //core_5.3              //  11
#define                 LL_CHANNEL_REPORTING_IND    0x28    //core_5.3              //  4
#define                 LL_CHANNEL_STATUS_IND       0x29    //core_5.3              //  11

#define                 LL_PERIODIC_SYNC_WR_IND     0x2A    //core_5.4              //  43      bigger than 27 !!!

//core 5.4+  opcode todo

#define                 LL_CS_SEC_REQ                   0x39 //core_6.0
#define                 LL_CS_SEC_RSP                   0x2D //core_6.0
#define                 LL_CS_CAPABILITIES_REQ          0x2E //core_6.0
#define                 LL_CS_CAPABILITIES_RSP          0x2F //core_6.0
#define                 LL_CS_CONFIG_REQ                0x30 //core_6.0
#define                 LL_CS_CONFIG_RSP                0x31 //core_6.0
#define                 LL_CS_REQ                       0x32 //core_6.0
#define                 LL_CS_RSP                       0x33 //core_6.0
#define                 LL_CS_IND                       0x34 //core_6.0
#define                 LL_CS_TERMINATE_IND             0x35 //core_6.0
#define                 LL_CS_FAE_REQ                   0x36 //core_6.0
#define                 LL_CS_FAE_RSP                   0x37 //core_6.0
#define                 LL_CS_CHANNEL_MAP_IND           0x38 //core_6.0

#define                 LL_FRAME_SPACE_REQ              0x39 //core_6.0
#define                 LL_FRAME_SPACE_RSP              0x3A //core_6.0





/**
 *  @brief  Definition for LLID of Data Physical Channel PDU header field
 */
#define                 LLID_RESERVED               0x00
#define                 LLID_DATA_CONTINUE          0x01
#define                 LLID_DATA_START             0x02
#define                 LLID_CONTROL                0x03




//Extended Header BIT
#define         EXTHD_BIT_ADVA                                  BIT(0)
#define         EXTHD_BIT_TARGETA                               BIT(1)
#define         EXTHD_BIT_CTE_INFO                              BIT(2)
#define         EXTHD_BIT_ADI                                   BIT(3)
#define         EXTHD_BIT_AUX_PTR                               BIT(4)
#define         EXTHD_BIT_SYNC_INFO                             BIT(5)
#define         EXTHD_BIT_TX_POWER                              BIT(6)


//Extended Header Length
#define         EXTHD_LEN_6_ADVA                                6
#define         EXTHD_LEN_6_TARGETA                             6
#define         EXTHD_LEN_1_CTE                                 1
#define         EXTHD_LEN_2_ADI                                 2
#define         EXTHD_LEN_3_AUX_PTR                             3
#define         EXTHD_LEN_18_SYNC_INFO                          18
#define         EXTHD_LEN_1_TX_POWER                            1


#define         LL_EXTADV_MODE_NON_CONN_NON_SCAN                (0x00)
#define         LL_EXTADV_MODE_CONN                             (0x01)      //connectable, none_scannable
#define         LL_EXTADV_MODE_SCAN                             (0x02)      //scannable,   none_connectable
#define         LL_EXTADV_MODE_RFU                              (0x03)

#define         EXT_ADV_PDU_AUXPTR_OFFSET_UNITS_30_US           0
#define         EXT_ADV_PDU_AUXPTR_OFFSET_UNITS_300_US          1

#define         EXT_ADV_PDU_AUXPTR_CA_51_500_PPM                0
#define         EXT_ADV_PDU_AUXPTR_CA_0_50_PPM                  1

#define         EXT_ADV_PDU_SYNC_OFFSET_UNITS_30_US         0
#define         EXT_ADV_PDU_SYNC_OFFSET_UNITS_300_US            1


#define         BIG_PDU_BIG_OFFSET_UNITS_30_US                  0
#define         BIG_PDU_BIG_OFFSET_UNITS_300_US                 1

/**
 *  @brief  The Aux PHY field: the PHY used to transmit the auxiliary packet
 */
typedef enum{
    PHY_USED_AUXPTR_LE_1M       = 0x00,
    PHY_USED_AUXPTR_LE_2M       = 0x01,
    PHY_USED_AUXPTR_LE_CODED    = 0x02,
    PHY_USED_AUXPTR_RFU_BEGIN   = 0x03,  //0b011~0b111 reserved for future use
}aux_phy_field_t;




// Advertise channel PDU Type
typedef enum advChannelPDUType_e {
    LL_TYPE_ADV_IND          = 0x00,
    LL_TYPE_ADV_DIRECT_IND   = 0x01,
    LL_TYPE_ADV_NONCONN_IND  = 0x02,

    LL_TYPE_SCAN_REQ         = 0x03,
    LL_TYPE_AUX_SCAN_REQ     = 0x03,

    LL_TYPE_SCAN_RSP         = 0x04,

    LL_TYPE_CONNECT_REQ      = 0x05,
    LL_TYPE_AUX_CONNECT_REQ = 0x05,

    LL_TYPE_ADV_SCAN_IND     = 0x06,

    LL_TYPE_ADV_EXT_IND                 = 0x07, //core_5.0
    LL_TYPE_AUX_ADV_IND                 = 0x07, //core_5.0
    LL_TYPE_AUX_SCAN_RSP                = 0x07, //core_5.0
    LL_TYPE_AUX_SYNC_IND                = 0x07, //core_5.0
    LL_TYPE_AUX_CHAIN_IND               = 0x07, //core_5.0
    LL_TYPE_AUX_SYNC_SUBEVENT_IND       = 0x07, //core_5.4
    LL_TYPE_AUX_SYNC_SUBEVENT_RSP       = 0x07, //core_5.4

    LL_TYPE_AUX_CONNECT_RSP = 0x08, //core_5.0
} advChannelPDUType_t;



typedef enum {
    TYPE_MASK_ADV_IND           =   BIT(0),
    TYPE_MASK_ADV_DIRECT_IND    =   BIT(1),
    TYPE_MASK_ADV_NONCONN_IND   =   BIT(2),
    TYPE_MASK_SCAN_REQ          =   BIT(3),
    TYPE_MASK_SCAN_RSP          =   BIT(4),
    TYPE_MASK_CONNECT_REQ       =   BIT(5),
    TYPE_MASK_ADV_SCAN_IND      =   BIT(6),
    TYPE_MASK_EXT_ADV           =   BIT(7),
} advTypeMask_t;








/**
 *  @brief  Definition for LLID of Connected Isochronous PDU header field
 */
typedef enum{
    ISO_LLID_UNFRAMED_PDU_END_FRAGMENT_SDU          = 0x00, //Unframed CIS Data PDU; end fragment of an SDU or a complete SDU
    ISO_LLID_UNFRAMED_PDU_START_CONTI_FRAGMENT_SDU  = 0x01, ISO_LLID_UNFRAMED_PDU_PADDING_SDU = 0x01,//Unframed CIS Data PDU; start or continuation fragment of an SDU
    ISO_LLID_FRAMED_PDU_SEGMENT_SDU                 = 0x02, //Framed CIS Data PDU; one or more segments of an SDU
    ISO_LLID_RESERVED                               = 0x03,
}ISO_llid_type;




/**
 *  @brief  Definition for BIG Control PDU Opcode
 */
typedef enum{
    BIG_CHANNEL_MAP_IND = 0x00,
    BIG_TERMINATE_IND   = 0x01,
}big_ctrl_op;





/******************************************** L2CAP **************************************************************/

// l2cap pb flag type
#define L2CAP_FRIST_PKT_H2C              0x00
#define L2CAP_CONTINUING_PKT             0x01
#define L2CAP_FIRST_PKT_C2H              0x02


/**
 *  @brief  Definition for L2CAP CID name space for the LE-U
 */
typedef enum{
    L2CAP_CID_NULL              = 0x0000,
    L2CAP_CID_ATTR_PROTOCOL     = 0x0004,
    L2CAP_CID_SIG_CHANNEL       = 0x0005,
    L2CAP_CID_SMP               = 0x0006,
}l2cap_cid_type;


/**
 *  @brief  Definition for L2CAP signal packet formats
 */
typedef enum{
    L2CAP_COMMAND_REJECT_RSP                = 0x01, //BR & LE
    L2CAP_CONNECTION_REQ                    = 0x02, //BR
    L2CAP_CONNECTION_RSP                    = 0x03, //BR
    L2CAP_CONFIGURATION_REQ                 = 0x04, //BR
    L2CAP_CONFIGURATION_RSP                 = 0x05, //BR
    L2CAP_DISCONNECTION_REQ                 = 0x06, //BR & LE
    L2CAP_DISCONNECTION_RSP                 = 0x07, //BR & LE
    L2CAP_ECHO_REQ                          = 0x08, //BR
    L2CAP_ECHO_RSP                          = 0x09, //BR
    L2CAP_INFORMATION_REQ                   = 0x0A, //BR
    L2CAP_INFORMATION_RSP                   = 0x0B, //BR
    L2CAP_CREATE_CHANNEL_REQ                = 0x0C, //no used, core_5.3
    L2CAP_CREATE_CHANNEL_RSP                = 0x0D, //no used, core_5.3
    L2CAP_MOVE_CHANNEL_REQ                  = 0x0E, //no used, core_5.3
    L2CAP_MOVE_CHANNEL_RSP                  = 0x0F, //no used, core_5.3
    L2CAP_MOVE_CHANNEL_CONFIRMATION_REQ     = 0x10, //no used, core_5.3
    L2CAP_MOVE_CHANNEL_CONFIRMATION_RSP     = 0x11, //no used, core_5.3
    L2CAP_CONN_PARAM_UPDATE_REQ             = 0x12, //LE
    L2CAP_CONN_PARAM_UPDATE_RSP             = 0x13, //LE
    L2CAP_LE_CREDIT_BASED_CONNECTION_REQ    = 0x14, //LE
    L2CAP_LE_CREDIT_BASED_CONNECTION_RSP    = 0x15, //LE
    L2CAP_FLOW_CONTROL_CREDIT_IND           = 0x16, //BR & LE
    L2CAP_CREDIT_BASED_CONNECTION_REQ       = 0x17, //BR & LE
    L2CAP_CREDIT_BASED_CONNECTION_RSP       = 0x18, //BR & LE
    L2CAP_CREDIT_BASED_RECONFIGURE_REQ      = 0x19, //BR & LE
    L2CAP_CREDIT_BASED_RECONFIGURE_RSP      = 0x1A, //BR & LE
}l2cap_sig_pkt_format;

/**
 *  @brief  Definition for Error Response of signal packet
 *  See the Core_v5.0(Vol 3/Part A/4.1) for more information.
 */
typedef enum{
    SIG_CMD_NOT_UNDERSTAND  = 0,
    SIG_MTU_EXCEEDED        = 1,
    SIG_INVALID_CID_REQUEST = 2,
}l2cap_sig_cmd_reject_reason;

//Result values for the L2CAP_LE_CREDIT_BASED_CONNECTION_RSP packet.
enum{
    CONN_SUCCESSFUL                             = 0x0000,
    CONN_REFUSED_SPSM_NOT_SUPPORT               = 0x0002,
    CONN_REFUSED_NO_RESOURCES_AVAILABLE         = 0x0004,
    CONN_REFUSED_INSUFFICIENT_AUTHENTICATION    = 0x0005,   //not used
    CONN_REFUSED_INSUFFICIENT_AUTHORIZATION     = 0x0006,   //not used
    CONN_REFUSED_ENCRYPTION_KEY_SIZE_TOO_SHORT  = 0x0007,   //not used
    CONN_REFUSED_INSUFFICIENT_ENCRYPTION        = 0x0008,   //not used
    CONN_REFUSED_INVALID_SOURCE_CID             = 0x0009,
    CONN_REFUSED_SOURCE_CID_ALREADY_ALLOCATED   = 0x000A,
    CONN_REFUSED_UNACCEPTABLE_PARAMETERS        = 0x000B,
};

//Result values for the L2CAP_CREDIT_BASED_CONNECTION_RSP packet.
enum{
    ALL_CONN_SUCCESSFUL                             = 0x0000,
    ALL_CONN_REFUSED_SPSM_NOT_SUPPORTED             = 0x0002,
    SOME_CONN_REFUSED_INSUFFICIENT_RESOURCES_AVA    = 0x0004,
    ALL_CONN_REFUSED_INSUFFICIENT_AUTHENTICATION    = 0x0005,
    ALL_CONN_REFUSED_INSUFFICIENT_AUTHORIZATION     = 0x0006,
    ALL_CONN_REFUSED_ENCRYPTION_KEY_SIZE_TOO_SHORT  = 0x0007,
    ALL_CONN_REFUSED_INSUFFICIENT_ENCRYPTION        = 0x0008,
    SOME_CONN_REFUSED_INVALID_SOURCE_CID            = 0x0009,
    SOME_CONN_REFUSED_SOURCE_CID_ALREADY_ALLOCATED  = 0x000A,
    ALL_CONN_REFUSED_UNACCEPTABLE_PARAMETERS        = 0x000B,
    ALL_CONN_REFUSED_INVALID_PARAMETERS             = 0x000C,
    ALL_CONN_REFUSED_NO_FURTHER_INFO_AVA            = 0x000D,
    ALL_CONN_REFUSED_AUTHENTICATION_PENDING         = 0x000E,
    ALL_CONN_REFUSED_AUTHORIZATION_PENDING          = 0x000F,
};

//Result values for the L2CAP_CREDIT_BASED_RECONFIGURE_RSP packet.
enum{
    RECONFIGURATION_SUCCESSFUL                      = 0x0000,
    RECONFIGURATION_FAILED_MTU_NOT_ALLOWED          = 0x0001,
    RECONFIGURATION_FAILED_MPS_NOT_ALLOWED          = 0x0002,
    RECONFIGURATION_FAILED_DST_CIDS_INVALID         = 0x0003,
    RECONFIGURATION_FAILED_OTHER_UNACCEPTABLE_PARAMS= 0x0004,
};


/**
 *  @brief  AuxPtr field
 */
typedef struct __attribute__((packed)) {
    u8  chn_index    :6;
    u8  ca          :1;
    u8  offset_unit :1;
    u16 aux_offset  :13;
    u16 aux_phy     :3;
} aux_ptr_t;


/**
 *  @brief  SYncInfo field
 */
typedef struct __attribute__((packed)) {
    u16 syncPktOffset: 13;
    u16 offsetUnit:    1;
    u16 offsetAdjust:  1;
    u16 rfu:           1;

    u16 itvl;

    u8  chm[5];//[0:36]chm, : [37:39]sca
    u32 AA;
    u8 crcInit[3];

    u16 evtCounter;
} sync_info_t;

typedef struct {
    u32 rsp_AA;

    u8  num_subevent;   //also be used to judge whether is PAwR sync. 0 indicate periodic advertising without response. not zero indicate PAwR
    u8  subevent_intvl; //in unit of 1.25ms. range from 7.5ms to 318.75ms
    u8  rsp_slot_delay; //in unit of 1.25ms, range from 1.25ms to 317.5ms
    u8  rsp_slot_spacing; //in unit of 0.125ms, from 0.25ms to 31.875ms
} pawr_acad_t; //PAwR ACAD content used




typedef struct {
    u8  type;
    u8  rf_len;
    u8  opcode;
    u8  winSize;
    u16 winOffset;
    u16 interval;
    u16 latency;
    u16 timeout;
    u16 instant;
} rf_packet_connect_upd_req_t;

typedef struct {
    u8  type;
    u8  rf_len;
    u8  opcode;
    u8  chm[5];
    u16 instant;
} rf_packet_chm_upd_req_t;

typedef struct __attribute__((packed)) {
    u8  type;
    u8  rf_len;
    u8  opcode;
    u8  rand[8];
    u16 ediv;
    u8  skdm[8];
    u8  ivm[4];
} rf_packet_ll_enc_req_t;

typedef struct {
    u8  type;
    u8  rf_len;
    u8  opcode;
    u8  skds[8];
    u8  ivs[4];
} rf_packet_ll_enc_rsp_t;

typedef struct {
    u8  type;
    u8  rf_len;
    u8  opcode;
    u8  unknownType;
} rf_packet_ll_unknown_rsp_t;

typedef struct {
    u8  type;
    u8  rf_len;
    u8  opcode;
    u8  featureSet[8];
} rf_packet_ll_feature_exg_t;

typedef struct{
    u8  type;
    u8  rf_len;
    u8  opcode;
    u8  versNr;
    u16 compId;
    u16 subVersNr;
}rf_packet_version_ind_t;

typedef struct{
    u8  type;
    u8  rf_len;
    u8  opcode;
    u8  errCode;
}rf_packet_ll_reject_ind_t;

typedef struct{
    u8  type;
    u8  rf_len;
    u8  opcode;
    u8  rejectOpcode;
    u8  errCode;
}rf_packet_ll_reject_ext_ind_t;

typedef struct{
    u8  type;
    u8  rf_len;
    u8  opcode;
    u8  reason;
}rf_packet_ll_terminate_t;



typedef struct {
    u8  type;
    u8  rf_len;

    u8  opcode;             //
    u8  dat[1];             //
}rf_packet_ll_control_t;








typedef struct {
    u8 llid   :2;
    u8 nesn   :1;
    u8 sn     :1;
    u8 md     :1;
    u8 cp     :1;
    u8 rfu1   :2;
    u8 rf_len;
    u8 cte_info;
}rf_cte_data_header_t;




typedef struct {
    u8 llid   :2;
    u8 nesn   :1;
    u8 sn     :1;
    u8 md     :1;
    u8 rfu1   :3;
    u8 rf_len;
}rf_acl_data_head_t;



typedef struct {
    u8 llid   :2;
    u8 nesn   :1;
    u8 sn     :1;
    u8 cie    :1;
    u8 rfu0   :1;
    u8 npi    :1;
    u8 rfu1   :1;
    u8 rf_len;
}rf_cis_data_hdr_t;



typedef struct {
    u8 llid   :2;
    u8 cssn   :3;
    u8 cstf   :1;
    u8 rfu0   :2;
    u8 rf_len;
}rf_bis_data_hdr_t;



typedef struct __attribute__((packed)) {
    u8  type;
    u8  rf_len;
    u8  opcode;
    u8  cigId;  // decided by host
    u8  cisId;  // decided by host
    u8  phyM2S; // decided by host // le_phy_prefer_type_t: BIT(0) BIT(1) BIT(2)
    u8  phyS2M;                    // le_phy_prefer_type_t: BIT(0) BIT(1) BIT(2)

    u32 maxSduM2S :12;  // decided by host
    u32 rfu0      :3;
    u32 framed    :1;   // decided by host
    u32 maxSduS2M :12;  // decided by host
    u32 rfu1      :4;

    u8 sduIntvlM2S[3]; // decided by host  SDU_Interval_M_To_S(20 bits) + RFU(4 bits)
    u8 sduIntvlS2M[3]; // decided by host  SDU_Interval_S_To_M(20 bits) + RFU(4 bits)

    u16 maxPduM2S;   // host assign in "Set CIG PARAM Test Command", but should decided by controller in "Set CIG PARAM Command"
    u16 maxPduS2M;   // host assign in "Set CIG PARAM Test Command", but should decided by controller in "Set CIG PARAM Command"
    u8  nse;         // host assign in "Set CIG PARAM Test Command", but should decided by controller in "Set CIG PARAM Command"
    u8  subIntvl[3]; // unit: uS  host assign in "Set CIG PARAM Test Command", but should decided by controller in "Set CIG PARAM Command"

    u8  bnM2S:4;    // host assign in "Set CIG PARAM Test Command", but should decided by controller in "Set CIG PARAM Command"
    u8  bnS2M:4;    // host assign in "Set CIG PARAM Test Command", but should decided by controller in "Set CIG PARAM Command"
    u8  ftM2S;      // host assign in "Set CIG PARAM Test Command", but should decided by controller in "Set CIG PARAM Command"
    u8  ftS2M;      // host assign in "Set CIG PARAM Test Command", but should decided by controller in "Set CIG PARAM Command"
    u16 isoIntvl;   //unit: 1.25 mS host assign in "Set CIG PARAM Test Command", but should decided by controller in "Set CIG PARAM Command"

    u8  cisOffsetMin[3];
    u8  cisOffsetMax[3];
    u16 connEventCnt;

}rf_packet_ll_cis_req_t;

typedef struct __attribute__((packed)) {
    u8  type;               //RA(1)_TA(1)_RFU(2)_TYPE(4)
    u8  rf_len;             //LEN(6)_RFU(2)
    u8  opcode;
    u8  cisOffsetMin[3];
    u8  cisOffsetMax[3];
    u16 connEventCnt;
}rf_packet_ll_cis_rsp_t;

typedef struct __attribute__((packed)) {
    u8  type;               //RA(1)_TA(1)_RFU(2)_TYPE(4)
    u8  rf_len;             //LEN(6)_RFU(2)
    u8  opcode;
    u32 cisAccessAddr;      //Access Address of the CIS
    u8  cisOffset[3];
    u8  cigSyncDly[3];
    u8  cisSyncDly[3];
    u16 connEventCnt;
}rf_packet_ll_cis_ind_t;

typedef struct{
    u8  type;
    u8  rf_len;
    u8  opcode;
    u8  cig_id;
    u8  cis_id;
    u8  errorCode;
}rf_packet_ll_cis_terminate_t;



typedef struct __attribute__((packed)) {
    union{
        rf_bis_data_hdr_t  bisPduHdr;
        rf_cis_data_hdr_t  cisPduHdr;
        rf_acl_data_head_t aclPduHdr;
        struct __attribute__((packed)) {
            u8 type;
            u8 rf_len;
        }pduHdr;
    }llPduHdr;        /* LL PDU Header: 2 */
    u8  llPayload[1]; /* Max LL Payload length: 251 */
}llPhysChnPdu_t;

typedef struct __attribute__((packed)) {
    u32 dma_len;
    llPhysChnPdu_t llPhysChnPdu;
}rf_packet_ll_data_t;




typedef struct{
    u8  type;
    u8  rf_len;
    u8  opcode;
    u8  enable;
    u8  min_spacing;
    u8  max_delay;
}rf_pkt_ll_chn_reporting_ind_t;


typedef struct __attribute__((packed)) {
    u8  type;
    u8  rf_len;
    u8  opcode;

    u8  Mode_Types;
    u8  RTT_Capability;
    u8  RTT_AA_Only_N;
    u8  RTT_Sounding_N;
    u8  RTT_Random_Sequence_N;
    u16  NADM_Sounding_Capability;
    u16 NADM_Random_Sequence_Capability;
    u8 CS_SYNC_PHY_Capability; //BIT(1) 2M

    u8 Num_Ant      :4;
    u8 Max_Ant_Path :4;//Max_Ant_Path >= Num_Ant

    u8 Role                 :2;
    u8 Companion_Signal     :1;
    u8 No_FAE               :1;
    u8 chn_sel_3c           :1;
    u8 Sounding_PCT_Estimate:1;
    u8 RFU1                 :2;

    u8 Num_Configs;//0< , <= 4
    u16 Max_Procedures_Supported;// N_PROCEDURE_COUNT max value,0 indicates the capability to run CS procedures repetitions until terminated.
    u8 T_SW;
    u16 T_IP1_Capability;
    u16 T_IP2_Capability;
    u16 T_FCS_Capability;
    u16 T_PM_Capability;
    u8  RFU2;


}rf_packet_ll_cs_cap_req_t;

typedef struct{
    u8  type;
    u8  rf_len;
    u8  opcode;

    u8 CS_IV_C[8];
    u8 CS_IN_C[4];
    u8 CS_PV_C[8];
}rf_pkt_ll_cs_sec_req_t;

typedef struct{
    u8  type;
    u8  rf_len;
    u8  opcode;

    u8 CS_IV_P[8];
    u8 CS_IN_P[4];
    u8 CS_PV_P[8];
}rf_pkt_ll_cs_sec_rsp_t;


typedef struct{
    u8  type;
    u8  rf_len;
    u8  opcode;
}rf_pkt_ll_cs_fae_req_t;

typedef struct{
    u8  type;
    u8  rf_len;
    u8  opcode;
    u8  fae_table[72];
}rf_pkt_ll_cs_fae_rsp_t;

typedef struct __attribute__((packed)) {
    u8  type;
    u8  rf_len;
    u8  opcode;
    u8  ChM[10];
    u16 instant;
}rf_pkt_ll_cs_chn_map_ind_t;

typedef struct{
    u8  type;
    u8  rf_len;
    u8  opcode;

    u8  Config_ID   :6;
    u8  RFU         :2;

    u8  ErrorCode;
}rf_pkt_ll_cs_terminate_ind_t;

enum{
    CS_MODE1 = 1,
    CS_MODE2 = 2,
    CS_MODE3 = 3,
};

typedef struct __attribute__((packed)) {
    u8  type;
    u8  rf_len;
    u8  opcode;

    u8  Config_ID:6;
    u8  State:2;

    u8  ChM[10];
    u8  ChM_Repetition;
    u8  Main_Mode;
    u8  Sub_Mode;

    u8  Main_Mode_Min_Steps;
    u8  Main_Mode_Max_Steps;

    u8 Main_Mode_Repetition;
    u8 Mode_0_Steps;
    u8 CS_SYNC_PHY;
    u8 RTT_Type :4;
    u8 Role     :2;

    u8 Companion_Signal :1;
    u8 RFU1             :1;
    u8 ChSel            :4;
    u8 Ch3cShape        :4;
    u8 Ch3cJump;
    u8 T_IP1;
    u8 T_IP2;
    u8 T_FCS;
    u8 T_PM;
    u8 RFU2;

}rf_packet_ll_cs_config_req_t;


typedef struct{
    u8  type;
    u8  rf_len;
    u8  opcode;

    u8  Config_ID:6;
    u8  RFU:2;


}rf_packet_ll_cs_config_rsp_t;


typedef struct __attribute__((packed)) {
    u8  type;
    u8  rf_len;
    u8  opcode;
    u8  Config_ID:6;
    u8  RFU:2;

    u16 connEventCount;
    u8  Offset_Min[3];
    u8  Offset_Max[3];

    u16 Max_Procedure_Len;
    u16 Event_Interval;

    u8  Subevents_Per_Event;
    u16 Subevent_Interval;
    u8  Subevent_Len[3];
    u16 Procedure_Interval;

    u16 Procedure_Count;
    u8  ACI;
    u8  Preferred_Peer_Ant;

    u8  PHY;
    s8  Pwr_Delta;
    u8  RFU2;

}rf_packet_ll_cs_req_t;

typedef struct __attribute__((packed)) {
    u8  type;
    u8  rf_len;
    u8  opcode;

    u8  Config_ID:6;
    u8  RFU:2;

    u16 connEventCount;
    u8  Offset_Min[3];
    u8  Offset_Max[3];

    u16 Event_Interval;
    u8  Subevents_Per_Event;
    u16 Subevent_Interval;
    u8  Subevent_Len[3];
    u8  ACI;
    u8  PHY;
    s8  Pwr_Delta;
    u8  RFU2;

}rf_packet_ll_cs_rsp_t;

typedef struct __attribute__((packed)) {
    u8  type;
    u8  rf_len;
    u8  opcode;

    u8  Config_ID:6;
    u8  RFU:2;

    u16 connEventCount;
    u8  Offset[3];

    u16 Event_Interval;
    u8  Subevents_Per_Event;
    u16 Subevent_Interval;
    u8  Subevent_Len[3];
    u8  ACI;
    u8  PHY;
    s8  Pwr_Delta;
    u8  RFU2;

}rf_packet_ll_cs_ind_t;




typedef struct __attribute__((packed)) {
    u8    type;
    u8    rf_len;
    u8    opcode;
    u16   fs_min;
    u16   fs_max;
    u8    phys_mask;
    u16   spacing_types;
}rf_packet_ll_frame_space_req_t;



typedef struct __attribute__((packed)) {
    u8    type;
    u8    rf_len;
    u8    opcode;
    u16   fs;
    u16   phys_mask;
    u16   spacing_types;
}rf_packet_ll_frame_space_rsp_t;






typedef struct __attribute__((packed)) {
    u8 llid   :2;
    u8 nesn   :1;
    u8 sn     :1;
    u8 md     :1;
    u8 rfu1   :3;

    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u8 data[1];
}rf_packet_l2cap_t;



typedef struct{
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u8 data[1];
}rf_packet_l2cap_req_t;




typedef struct{
    u8  llid;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u8  id;
    u16 data_len;
    u16 min_interval;
    u16 max_interval;
    u16 latency;
    u16 timeout;
}rf_packet_l2cap_connParaUpReq_t;



typedef struct{
    u8  llid;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u8  id;
    u16 data_len;
    u16 result;
}rf_packet_l2cap_connParaUpRsp_t;





typedef struct{
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  code;
    u8  id;
    u16 dataLen;
    u16  result;
}rf_pkt_l2cap_sig_connParaUpRsp_t;

typedef struct{
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  code;
    u8  id;
    u16 length;
    u16 psm;
    u16 mtu;
    u16 mps;
    u16 init_credits;
    u16 scid[5];
}rf_pkt_l2cap_credit_based_connection_req_t;

typedef struct{
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  code;
    u8  id;
    u16 length;
    u16 mtu;
    u16 mps;
    u16 init_credits;
    u16 result;
    u16 dcid[5];
}rf_pkt_l2cap_credit_based_connection_rsp_t;


typedef struct{
    u8  llid;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u8  id;
    u16 length;
    u16 spsm;
    u16 mtu;
    u16 mps;
    u16 init_credits;
    u16 scid[5];
}rf_packet_l2cap_credit_based_connection_req_t;

typedef struct{
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  code;
    u8  id;
    u16 length;
    u16 mtu;
    u16 mps;
    u16 dcid[5];
}rf_pkt_l2cap_credit_based_reconfigure_req_t;

typedef struct{
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  code;
    u8  id;
    u16 length;
    u16 result;
}rf_pkt_l2cap_credit_based_reconfigure_rsp_t;


typedef struct __attribute__((packed)) {
    u8  type;
    u8  rf_len;

    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u16 startingHandle;
    u16 endingHandle;
}rf_packet_att_findInfo_t;

typedef struct __attribute__((packed)) {
    u8  type;
    u8  rf_len;

    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u16 startingHandle;
    u16 endingHandle;
    u8  attType[2];             //
}rf_packet_att_readByType_t;

typedef struct __attribute__((packed)) {
    u8  type;
    u8  rf_len;

    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u16 startingHandle;
    u16 endingHandle;
    u8  attType[2];
    u8  attValue[2];
}rf_packet_att_findByTypeReq_t;

typedef struct __attribute__((packed)) {
    u8  type;
    u8  rf_len;

    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u16 data[1];
}rf_packet_att_findByTypeRsp_t;

typedef struct __attribute__((packed)) {
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u16 handle;
}rf_packet_att_read_t;

typedef struct __attribute__((packed)) {
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u16 handle;
    u16 offset;
}rf_packet_att_readBlob_t;

typedef struct __attribute__((packed)) {
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u16 startingHandle;
    u16 endingHandle;
    u8  attGroupType[2];                //
}rf_packet_att_readByGroupType_t;

typedef struct{
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u8  value[22];
}rf_packet_att_readRsp_t;

typedef struct{
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u8  datalen;
    u8  data[3];
}rf_packet_att_readByGroupTypeRsp_t;

typedef struct{
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u8  format;
    u8  data[1];            // character_handle / property / value_handle / value
}rf_packet_att_findInfoRsp_t;

typedef struct{
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u8 flags;
}rf_packet_att_executeWriteReq_t;

typedef struct __attribute__((packed)) {
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u16 handle;
    u8 value;
}rf_packet_att_write_t;

typedef struct{
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u8 mtu[2];
}rf_packet_att_mtu_t;

typedef struct{
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u8 mtu[2];
}rf_packet_att_mtu_exchange_t;

typedef struct{
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
}rf_packet_att_writeRsp_t;

typedef struct{
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u8  datalen;
    u8  data[1];            // character_handle / property / value_handle / value
}att_readByTypeRsp_t;


typedef struct{
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u8  value[22];
}att_readRsp_t;







void tlk_mem_set(void *dest, int val, unsigned int len);
void tlk_mem_cpy(void *pd, const void *ps, unsigned int len);
int tlk_mem_cmp(const void *m1, const void *m2, unsigned int len);
void tlk_mem_set4(void *dest, int val, unsigned int len);
void tlk_mem_cpy4(void *pd, const void *ps, unsigned int len);
int tlk_mem_cmp4(const void *m1, const void *m2, unsigned int len);
//int tlk_strlen(const char *str);


#define smemset     tlk_mem_set
#define smemcpy     tlk_mem_cpy
#define smemcmp     tlk_mem_cmp
#define smemset4    tlk_mem_set4
#define smemcpy4    tlk_mem_cpy4
#define smemcmp4    tlk_mem_cmp4




/////////////////cs debug end//////////////////
#endif /* BLE_STACK_H_ */
