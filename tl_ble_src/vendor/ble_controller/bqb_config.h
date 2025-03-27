/********************************************************************************************************
 * @file    bqb_config.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef VENDOR_B91_CONTROLLER_BQB_CONFIG_H_
#define VENDOR_B91_CONTROLLER_BQB_CONFIG_H_

/**
 * @defgroup      bqb_app_config
 * @brief         bqb macro configurations.
 * @note          only APP related macro is defined here, associated macros
 *                will be added later.
 * @{
 */

/**
 * @brief         enable the extended advertisement module.
 * @note          Run #222 - /HCI/DDI/BI-08-C.
 */
#define APP_LE_EXTENDED_ADV_EN                                      0
#if APP_LE_EXTENDED_ADV_EN == 0
/**
 * @brief         enable the 2M and coded phy.
 * @note          default to enable.
 */
#define APP_LE_2M_CODED_PHY_EN                                      1

/**
 * @brief         enable the channel selection algorithm 2.
 * @note          default to enable.
 */
#define APP_LE_CHANNEL_SELECTION_ALGORITHM_2_EN                     1
#endif // APP_LE_EXTENDED_ADV_EN == 0

/**
 * @brief         enable the periodic advertisement module.
 */
#define APP_LE_PERIODIC_ADV_EN                                      0

/**
 * @brief         enable the extended scan module.
 */
#define APP_LE_EXTENDED_SCAN_EN                                     0

/**
 * @brief         enable the extended initiating module.
 */
#define APP_LE_EXTENDED_INIT_EN                                     0

/**
 * @brief         enable the AoA/AoD Constant Tone Extension module.
 */
#define APP_LE_AOA_AOD_EN                                           0
#if APP_LE_AOA_AOD_EN == 1
#define LL_FEATURE_SUPPORT_LE_AOA_AOD       1
#define LL_FEATURE_SUPPORT_CONNECTION_CTE_REQUEST                   1
#define LL_FEATURE_SUPPORT_CONNECTION_CTE_RESPONSE                  1
#define LL_FEATURE_SUPPORT_CONNECTIONLESS_CTE_TRANSMITTER           1
#define LL_FEATURE_SUPPORT_CONNECTIONLESS_CTE_RECEIVER              1
#define LL_FEATURE_SUPPORT_ANTENNA_SWITCHING_CTE_TRANSMISSION_AOD   1
#define LL_FEATURE_SUPPORT_ANTENNA_SWITCHING_CTE_RECEPTION_AOA      1
#define LL_FEATURE_SUPPORT_RECEIVING_CONSTANT_TONE_EXTENSIONS       1
#define LL_UNREQUESTED_CONSTANT_TONE_EXTENSION_RECEIVING_ENABLE     1
#endif

/**
 * @brief         enable the BIG broadcast module.
 */
#define APP_ISOCHRONOUS_BROADCASTER_EN                              0

/**
 * @brief         enable the periodic advertising synchronization module and
 *                the BIG broadcast module.
 * @note          add about 30K ramcode.
 */
#define APP_ISOCHRONOUS_BROADCASTER_SYNC_EN                         0

/**
 * @brief         enable the periodic advertising synchronization module.
 */
#define APP_SYNCHRONIZED_RECEIVER_EN                                0

/**
 * @brief         enable the PAST module.
 * @note          just for HCI/GEV/BV-02-C.
 */
#define APP_PAST_EN                                                 0
#if APP_PAST_EN == 1
#define LL_FEATURE_SUPPORT_LE_PAST_SENDER                           1
#define LL_FEATURE_SUPPORT_LE_PAST_RECIPIENT                        1
#endif

/**
 * @brief         enable the LL Power control module.
 */
#define APP_POWER_CONTROL                                           0

/**
 * @brief         enable the CIS module and the related modules.
 */
#define APP_LE_CIS_CENTRAL                                          0
#define APP_LE_CIS_PERIPHR                                          0

/**
 * @brief         enable the PHY test module.
 */
#define APP_LE_PHY_TEST_EN                                          0

/**
 * @brief         enable the ChnClassification feature.
 */
#define APP_CHN_CLASS_EN                                            0

/**
 * @brief         enable the subrate feature.
 * @note          Run #198 - /HCI/CCO/BI-39-C.
 */
#define APP_LL_SUBRATE_EN                                           0
#if APP_LL_SUBRATE_EN == 1
#define  LL_FEATURE_SUPPORT_CONNECTION_SUBRATING                    1
#endif

/**
 * @brief         enable the subrate feature.
 * @note          config by ACL_TXFIFO_4K_LIMITATION_WORKAROUND.
 */
#define APP_WORKAROUND_TX_FIFO_4K_LIMITATION_EN                     0
#if APP_WORKAROUND_TX_FIFO_4K_LIMITATION_EN == 1
#define ACL_TXFIFO_4K_LIMITATION_WORKAROUND                         1
#endif

/**
 * @}
 */


/**
 * @defgroup      bqb_private_config
 * @brief         bqb private macro configurations
 * @{
 */

#if 0
#define SUB_INTERVAL_VARIATION_WORKAROUND_MLP_CODE_IN_RAM           0 //1792B

#define TASK_VERY_CLOSE_DROP_EN                                     0
#define PDA_SCAN_PENDING_FIX_EN                                     0
#define EXT_ADV_EN_MORE_STRATEGY                                    0

#define LL_CON_PER_BV88C                                            1
#define LL_CON_PER_BV98C_AND_CON_CEN_BV94C                          1
#define NETWORK_PRIVACY_IGNORE_IDA_CHECK                            1
#define LL_FEATURE_SUPPORT_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE      1

#define MAX_CONFLICT_NUM                                            8
#define SL_STACK_BIS_RX_DATA_EN                                     0
#define SL_STACK_BIS_RX_DATA_EN                                     0
#define DEB_BIG_SYNC_EN                                             0

//attention: the CTE-related FEATURE_SUPPORT macro below must be open


#define LL_CRC_CHECK_REGISTER_EN                                    1
#define ONE_ACL_SLAVE_MATCH_2_CIS_SLAVE_ENABLE                      1
#define CIS_WINDOW_WIDENING_FOR_BIG_PPM                             1   //LL/CIS/PER/BV-45-C [Listening for Packet With Window Widening, CIS]
#define ACL_CENTRAL_BASE_INTERVAL_FOLLOW_UPPER_LAYER                1
#define EBQ_8280_CHECK_PDU_EN                                       1

/**
 * @brief         must enable for BQB privacy, for ext_adv data address change.
 */
#define EXTADV_DATA_CHANGE_MANUAL_DATA_BUFFER                       1

/**
 * @brief         if BN = 3, max task timing = 60mS +,   80mS map is enough,
 *                if BN = 4, max task timing = 90mS +,   80mS Fail, 120mS map is enough,
 *                if BN = 5, max task timing = 120mS +, 120mS Fail, 160mS map is enough,
 *                if BN = 6, max task timing = 150mS +, 160mS map is enough.
 * @note          CIS/PER/BV_39
 */
#define SCHE_PRE_ALLOCATE_MAX_LEN                                   SCHE_PRE_ALLOCATE_LEN_120MS

/**
 * @brief         when test EBQ HCI case, the macro need to set 800 for checking data_too_long.
 * @note          HCI/DDI/BI-50-C, HCI/DDI/BI-51-C
 */
// #define APP_MAX_LENGTH_ADV_DATA                                   800

/**
 * @brief         need to set same as IXIT. now IXIT's setting is 1 and 1.
 * @note          HCI/BIS/BV-01-C
 */
//#define       APP_BIS_NUM_IN_PER_BIG_BCST                         1 //default 2
//#define       LL_BIS_IN_PER_BIG_BCST_NUM_MAX                      1 //default 4

/**
 * @brief         the following macro need to keep same as IXIT---LE ISO Max HCI Data Packet Length.
 * @note          LL/BIS/BRD/BV-23-C, LL/BIS/BRD/BV-24-C
 */
//#define       BIS_SDU_IN_OCTETS_MAX           512 //before IXIT setting is 251. change IXIT value to 512

/**
 * @brief         open this macro in LL/BIS/SNC/BV-18-C, LL/IST/SNC/BV-01-C.
 * @note          LL/BIS/SNC/BV-18-C, LL/IST/SNC/BV-01-C
 */
#define   LL_BIS_SNC_BV18C_BN6              0

/**
 * @brief         open this macro in IAL/BIS/FRA/SNC/BV-20-C, IAL/BIS/FRA/BRD/BV-18-C.
 * @note          tested on 9.20 and the results are good, later will delete it.
 */
#define NEED_MORE_TEST_TO_CONFIRM           0
#endif
/**
 * @}
 */

#endif /* VENDOR_B91_CONTROLLER_BQB_CONFIG_H_ */
