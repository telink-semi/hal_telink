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

//Bluetooth Version is the key of BQB Certify. So define this at the top.
#define BLUETOOTH_VER BLUETOOTH_VER_6_0

//TODO: Should define these per features or version, instead of Engineer's name.
#define BQB_DEFAULT 0
#define BQB_GAOQIU  1
#define BQB_QIUWEI  2
#define BQB_YAFEI   3
#define BQB_QINGHUA 4
#define BQB_LIJING  5
#define BQB_SIHUI   6
#define BQB_CS      7
#define BQB_6_0     8

#define BQB_SELECT  BQB_CS     //BQB_DEFAULT when and after release. During Certify period, change it to common define for fast scrum


#if (BQB_SELECT == BQB_GAOQIU)
    /*
 *
 * boundary protect, do not write code here, in case of GIT push conflict
 *
 */
    #define APP_WORKAROUND_TX_FIFO_4K_LIMITATION_EN ACL_TXFIFO_4K_LIMITATION_WORKAROUND

    #define APP_LE_EXTENDED_SCAN_EN                 1
    #define APP_LE_EXTENDED_ADV_EN                  1 //Run #222 - /HCI/DDI/BI-08-C [Reject Invalid Enable Command (Extended Advertising, Random Address)]
    #define APP_LL_SUBRATE_EN                       1 //Run #198 - /HCI/CCO/BI-39-C [Invalid Write Authenticated Payload Timeout Parameters]
    #define APP_LE_EXTENDED_INIT_EN                 1


/*
 *
 * boundary protect, do not write code here, in case of GIT push conflict
 *
 */
#elif (BQB_SELECT == BQB_QIUWEI)
    /*
 *
 * boundary protect, do not write code here, in case of GIT push conflict
 *
 */
    #define APP_LE_EXTENDED_ADV_EN                            1
    #define APP_LE_PERIODIC_ADV_EN                            1
    #define APP_LE_EXTENDED_SCAN_EN                           1
    #define APP_LE_EXTENDED_INIT_EN                           1


    #define APP_SYNCHRONIZED_RECEIVER_EN                      1
    #define APP_ISOCHRONOUS_BROADCASTER_EN                    1
    #define APP_ISOCHRONOUS_BROADCASTER_SYNC_EN               1 //add about 30K ramcode

    #define APP_WORKAROUND_TX_FIFO_4K_LIMITATION_EN           0

    #define SUB_INTERVAL_VARIATION_WORKAROUND_MLP_CODE_IN_RAM 0 //1792B
    #define NEED_MORE_TEST_TO_CONFIRM                         0 //tested on 9.20 and the results are good. later will delete it.


    #define EXTADV_DATA_CHANGE_MANUAL_DATA_BUFFER             1 //must enable for BQB privacy, for ext_adv data address change


    #define APP_PAST_EN                                       1 //just for HCI/GEV/BV-02-C

    #define TASK_VERY_CLOSE_DROP_EN                           0
    #define PDA_SCAN_PENDING_FIX_EN                           0
    #define EXT_ADV_EN_MORE_STRATEGY                          0

    #define DEBUG_QIUWEI_GPIO_ENABLE                          1
    //debug
    #if (0) //SiHui use
        #define IUT_HCI_LOG_EN         1

        #define DBG_EXTSCAN_LOGIC      1
        #define DBG_EXTSCAN_REPORT     0
        #define DBG_EXTSCAN_ERR_PKT_EN 1
        #define DBG_PRVC_LEGSCAN_EN    1
        #define DBG_PRVC_EXTADV_EN     1
        #define DBG_PRVC_EXTSCAN_EN    1
        #define DBG_PRVC_INIT_EN       1
        #define DBG_PRVC_RL_EN         1

        #define SIHUI_FILTER_RSSI      1
    #endif

///////////// All HCI case setting & All LL/DDI/ADV case///////////////////
//when test EBQ HCI case, the macro need to set 800 for checking data_too_long.
//"HCI/DDI/BI-50-C" and "HCI/DDI/BI-51-C"
//#define APP_MAX_LENGTH_ADV_DATA                                   800

//HCI/BIS/BV-01-C , need to set same as IXIT. now IXIT's setting is 1 and 1
//#define       APP_BIS_NUM_IN_PER_BIG_BCST                         1 //default 2
//#define       LL_BIS_IN_PER_BIG_BCST_NUM_MAX                      1 //default 4


//////////// All other case setting ///////////////////
//LL/BIS/BRD/BV-23-C  //LL/BIS/BRD/BV-24-C
//the following macro need to keep same as IXIT---LE ISO Max HCI Data Packet Length.
//#define       BIS_SDU_IN_OCTETS_MAX           512 //before IXIT setting is 251. change IXIT value to 512
/*
 *
 * boundary protect, do not write code here, in case of GIT push conflict
 *
 */
#elif (BQB_SELECT == BQB_YAFEI)
    /*
 *
 * boundary protect, do not write code here, in case of GIT push conflict
 *
 */
    #define APP_LE_EXTENDED_ADV_EN             1
    #define APP_LE_PERIODIC_ADV_EN             1
    #define APP_LE_EXTENDED_SCAN_EN            1
    #define APP_LE_EXTENDED_INIT_EN            1
    #define APP_SYNCHRONIZED_RECEIVER_EN       1
    #define APP_LE_CIS_CENTRAL                 1
    #define APP_LE_CIS_PERIPHR                 1
    #define APP_PAST_EN                        1
    #define APP_POWER_CONTROL                  1
    #define APP_CHN_CLASS_EN                   1

    #define LL_CON_PER_BV88C                   1
    #define LL_CON_PER_BV98C_AND_CON_CEN_BV94C 1

    //usb debug log default closed
    #define IUT_HCI_LOG_EN                     0
    #define DBG_LL_PAST_EN                     0
    #define DBG_LL_PCL_EN                      0
    #define DBG_LL_CC_EN                       0
    #define DBG_PRVC_EXTADV_EN                 0
    #define DBG_PRVC_EXTSCAN_EN                0
    #define DBG_PRVC_INIT_EN                   0
    #if (0) //SiHui use
        #define DBG_PRVC_RL_EN                                         1
        #define DBG_PRVC_LEGADV_EN                                     1
        #define DBG_PRVC_LEGSCAN_EN                                    1
        #define DBG_PRVC_INIT_EN                                       1
        #define DBG_PRVC_CONN_EN                                       1
        #define CIS_FLOW_LOG_EN                                        1
        #define LL_CTRL_LOG_EN                                         1
        #define DBG_CIS_1ST_AP_TIMING_EN                               1
        #define DBG_NUM_COM_PKT                                        0
        #define DBG_CIS_PARAM                                          1
        #define DBG_CIS_CENTRAL_PARAM                                  1
        #define DBG_CIS_TX_DATA                                        0
        #define DBG_CIS_RX_DATA                                        0
        #define DBG_SET_CIG_PARAMS                                     1
        #define DBG_HCI_CIS_TEST                                       1
        #define SIHUI_FILTER_RSSI                                      0
        #define NETWORK_PRIVACY_IGNORE_IDA_CHECK                       1
        #define LL_FEATURE_SUPPORT_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE 1
        #define EXTADV_DATA_CHANGE_MANUAL_DATA_BUFFER                  1
    #endif

    #define DEBUG_YAFEI_GPIO_ENABLE 1

/*
 *
 * boundary protect, do not write code here, in case of GIT push conflict
 *
 */
#elif (BQB_SELECT == BQB_QINGHUA)
    /*
 *
 * boundary protect, do not write code here, in case of GIT push conflict
 *
 */

    #define DEBUG_FANQH_GPIO_ENABLE 1

    #if (1)
        //for Subrate Test
        #define APP_LL_SUBRATE_EN                   1


        //for BIS_Sync Test
        #define APP_LE_EXTENDED_SCAN_EN             1
        #define APP_ISOCHRONOUS_BROADCASTER_SYNC_EN 1


        #define LL_BIS_SNC_BV18C_BN6                0


        /*
 *1.  LL/BIS/SNC/BV-18-C
 *    LL/IST/SNC/BV-01-C
 * should open this macro
 * #define   LL_BIS_SNC_BV18C_BN6           1
 *
 */

        /*
 *
 *  IAL/BIS/FRA/SNC/BV-20-C
 *  IAL/BIS/FRA/BRD/BV-18-C
 *  should opend this macro
 *  NEED_MORE_TEST_TO_CONFIRM
 *
 *
 */


        #define APP_LE_EXTENDED_ADV_EN              1
        #define APP_LE_PERIODIC_ADV_EN              1
        #define APP_LE_EXTENDED_SCAN_EN             1
        #define APP_LE_EXTENDED_INIT_EN             1
        #define APP_ISOCHRONOUS_BROADCASTER_EN      1
        #define MAX_CONFLICT_NUM                    8

        #define SL_STACK_BIS_RX_DATA_EN             0
        #define SL_STACK_BIS_RX_DATA_EN             0
        #define DEB_BIG_SYNC_EN                     0

    #else

        //cis IAL
        #define APP_LE_CIS_CENTRAL        1
        #define APP_LE_CIS_PERIPHR        1
        #define SCHE_PRE_ALLOCATE_MAX_LEN SCHE_PRE_ALLOCATE_LEN_120MS


        #define DBG_CIS_PARAM             1
        #define DBG_CIS_TX_DATA           1
        #define DBG_CIS_RX_DATA           1
        #define DBG_CIS_RX_DATA_FLOW_EN   1
        #define DBG_CIS_TX_DATA_FLOW_EN   1
    #endif

    #define DEBUG_PAYLOAD_NUM       1
    #define DBG_IAL_EN              1
    #define DEB_BIG_SYNC_LL_DATA_EN 0
/*
 *
 * boundary protect, do not write code here, in case of GIT push conflict
 *
 */
#elif (BQB_SELECT == BQB_LIJING)
    /*
 *
 * boundary protect, do not write code here, in case of GIT push conflict
 *
 */
    #define APP_LE_EXTENDED_ADV_EN                                    1
    #define APP_LE_PERIODIC_ADV_EN                                    1
    #define APP_LE_EXTENDED_SCAN_EN                                   1
    #define APP_SYNCHRONIZED_RECEIVER_EN                              1
    #define APP_LE_AOA_AOD_EN                                         1

    //attention: the CTE-related FEATURE_SUPPORT macro below must be open
    #define LL_FEATURE_SUPPORT_LE_AOA_AOD                             1
    #define LL_FEATURE_SUPPORT_CONNECTIONLESS_CTE_TRANSMITTER         1
    #define LL_FEATURE_SUPPORT_CONNECTIONLESS_CTE_RECEIVER            1
    #define LL_FEATURE_SUPPORT_ANTENNA_SWITCHING_CTE_TRANSMISSION_AOD 1
    #define LL_FEATURE_SUPPORT_ANTENNA_SWITCHING_CTE_RECEPTION_AOA    1
    #define LL_FEATURE_SUPPORT_RECEIVING_CONSTANT_TONE_EXTENSIONS     1

    #define LL_CRC_CHECK_REGISTER_EN                                  1
    #define LL_UNREQUESTED_CONSTANT_TONE_EXTENSION_RECEIVING_ENABLE   1
    #define DBG_AOA_AOD_LOGIC                                         1


/*
 *
 * boundary protect, do not write code here, in case of GIT push conflict
 *
 */
#elif (BQB_SELECT == BQB_SIHUI)
    /*
 *
 * boundary protect, do not write code here, in case of GIT push conflict
 *
 */
    #define APP_LE_CIS_CENTRAL                                     1
    #define APP_LE_CIS_PERIPHR                                     1


    #define ONE_ACL_SLAVE_MATCH_2_CIS_SLAVE_ENABLE                 1
    #define CIS_WINDOW_WIDENING_FOR_BIG_PPM                        1 //LL/CIS/PER/BV-45-C [Listening for Packet With Window Widening, CIS]
    #define ACL_CENTRAL_BASE_INTERVAL_FOLLOW_UPPER_LAYER           1


    /* CIS/PER/BV_39
 * if BN = 3, max task timing = 60mS +,   80mS map is enough
 * if BN = 4, max task timing = 90mS +,   80mS Fail, 120mS map is enough
 * if BN = 5, max task timing = 120mS +, 120mS Fail, 160mS map is enough
 * if BN = 6, max task timing = 150mS +, 160mS map is enough
 * */
    //#define   SCHE_PRE_ALLOCATE_MAX_LEN                   SCHE_PRE_ALLOCATE_LEN_120MS


    #define EBQ_8280_CHECK_PDU_EN                                  1


    #define DEBUG_SIHUI_GPIO_ENABLE                                1

    #define DBG_PRVC_RL_EN                                         1
    #define DBG_PRVC_LEGADV_EN                                     1
    #define DBG_PRVC_LEGSCAN_EN                                    1
    #define DBG_PRVC_INIT_EN                                       1
    #define DBG_PRVC_CONN_EN                                       1


    #define CIS_FLOW_LOG_EN                                        1
    #define LL_CTRL_LOG_EN                                         1
    #define DBG_CIS_1ST_AP_TIMING_EN                               1
    #define DBG_NUM_COM_PKT                                        0
    #define DBG_CIS_PARAM                                          1
    #define DBG_CIS_CENTRAL_PARAM                                  1
    #define DBG_CIS_TX_DATA                                        0
    #define DBG_CIS_RX_DATA                                        0
    #define DBG_SET_CIG_PARAMS                                     1

    #define DBG_HCI_CIS_TEST                                       1

    #define SIHUI_FILTER_RSSI                                      0


    #define NETWORK_PRIVACY_IGNORE_IDA_CHECK                       1
    #define LL_FEATURE_SUPPORT_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE 1
//#define EXTADV_DATA_CHANGE_MANUAL_DATA_BUFFER     1
/*
 *
 * boundary protect, do not write code here, in case of GIT push conflict
 *
 */
#elif (BQB_SELECT == BQB_CS)
    /*
 *
 * boundary protect, do not write code here, in case of GIT push conflict
 *
 */

    #define APP_LE_CHANNEL_SOUNDING_CENTRAL 1
    #define APP_LE_CHANNEL_SOUNDING_PERIPHR 1
    #define DBG_CS_LOG                      1
    #define CS_EBQ_TEST                     1

    #define TLKAPI_DEBUG_FIFO_SIZE          128
    #define TLKAPI_DEBUG_FIFO_NUM           64

    #define MARKET_POSITIONING_LE_AUDIO_SUPPORT_EN 0
    //#define BLUETOOTH_VER                   BLUETOOTH_VER_6_0

/*
 *
 * boundary protect, do not write code here, in case of GIT push conflict
 *
 */
#elif (BQB_SELECT == BQB_6_0)

//Define XXX_PARAM_LENGTH for BLUETOOTH 6.0
#define ADV_SET_PARAM_LENGTH    392
#define PERD_ADV_PARAM_LENGTH   1836
#define BIG_BCST_PARAM_LENGTH   736
#define BIG_SYNC_PARAM_LENGTH   816
#define BIS_PARAM_LENGTH        272
#define CIS_SLV_PARAM_LEN       488
#define CIG_PARAM_LEN           532
#define CIS_CONN_PARAM_LENGTH   440
#define CS_PARAM_LENGTH         9768

//Define LL_FEATURE_SUPPORT_XXX for 

//Define APP_LE_XXX_EN for common app initialization.
#define APP_LE_PHY_TEST_EN              1
#define APP_LE_EXTENDED_ADV_EN          1
#define APP_LE_EXTENDED_SCAN_EN         1
#define APP_LE_EXTENDED_INIT_EN         1
#define APP_LE_PERIODIC_ADV_EN          1
#define APP_SYNCHRONIZED_RECEIVER_EN    1
#define APP_LE_MONITORING_ADVERTISERS   1


#endif //end of BQB_SELECT


#ifndef APP_LE_EXTENDED_ADV_EN
    #define APP_LE_EXTENDED_ADV_EN 0
#endif

#ifndef APP_LE_PERIODIC_ADV_EN
    #define APP_LE_PERIODIC_ADV_EN 0
#endif

#ifndef APP_LE_EXTENDED_SCAN_EN
    #define APP_LE_EXTENDED_SCAN_EN 0
#endif

#ifndef APP_LE_EXTENDED_INIT_EN
    #define APP_LE_EXTENDED_INIT_EN 0
#endif

#ifndef APP_LE_AOA_AOD_EN
    #define APP_LE_AOA_AOD_EN 0
#endif


#ifndef APP_ISOCHRONOUS_BROADCASTER_EN
    #define APP_ISOCHRONOUS_BROADCASTER_EN 0
#endif

#ifndef APP_ISOCHRONOUS_BROADCASTER_SYNC_EN
    #define APP_ISOCHRONOUS_BROADCASTER_SYNC_EN 0
#endif

#ifndef APP_SYNCHRONIZED_RECEIVER_EN
    #define APP_SYNCHRONIZED_RECEIVER_EN 0
#endif

#ifndef APP_PAST_EN
    #define APP_PAST_EN 0
#endif


#ifndef APP_POWER_CONTROL
    #define APP_POWER_CONTROL 0
#endif

#ifndef APP_LE_CIS_CENTRAL
    #define APP_LE_CIS_CENTRAL 0
#endif

#ifndef APP_LE_CIS_PERIPHR
    #define APP_LE_CIS_PERIPHR 0
#endif

#ifndef APP_LE_MONITORING_ADVERTISERS
    #define APP_LE_MONITORING_ADVERTISERS 0
#endif

#ifndef APP_LE_PHY_TEST_EN
    #define APP_LE_PHY_TEST_EN 0
#endif

#ifndef APP_CHN_CLASS_EN
    #define APP_CHN_CLASS_EN 0
#endif


#ifndef APP_LL_SUBRATE_EN
    #define APP_LL_SUBRATE_EN 0
#endif


#endif /* VENDOR_B91_CONTROLLER_BQB_CONFIG_H_ */
