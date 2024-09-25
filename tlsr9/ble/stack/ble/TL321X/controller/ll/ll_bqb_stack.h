/********************************************************************************************************
 * @file    ll_bqb_stack.h
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
#ifndef __LL_BQB_STACK_H_
#define __LL_BQB_STACK_H_


#ifndef BQB_TEST_EN
#define BQB_TEST_EN                                             0
#endif

//TODO: clear about each item later
#ifndef EBQ_TEST_EN
#define EBQ_TEST_EN                                             0 //change to 0 after BQB finish
#endif




#if EBQ_TEST_EN
    #define BIS_ADV_EBQ                                         1
    #define LL_DDI_ADV_BV61C                                    1
    #define PDA_SYNC_EBQ                                        1 //LL/DDI/SCN/BV-21-C, need qiuwei check
    #define EXTENDED_ADV_RPT_MANUAL_EN                          1 //LL_DDI_SCN_BV_37_C
    #define LONG_CTRL_PDUS_AUTO_FEATURE_REQ_DIS                 1 //only for EBQ test, if use normally, need to delete.
    #define LL_CON_PER_BV105C                                   1 //LL/CON/PER/BV-105-C
    #define ONLY_FOR_EBQ_TEST_LATER_REMOVE                      1 //only for EBQ test, if use normally, need to delete.

    #define BQB_HOST_SEND_EMPTY_ACL_DATA_HANDLE_EN              1
    #define BQB_HCI_LOCAL_SUP_CMD                               1


    #define LL_FEATURE_SUPPORT_MIN_USED_OF_USED_CHANNELS        1
    #define LL_FEATURE_SUPPORT_EXTENDED_SCANNER_FILTER_POLICIES 1
    #define LL_FEATURE_SUPPORT_PERIODIC_ADV_ADI_SUPPORT         1
    #define LE_AUTHENTICATED_PAYLOAD_TIMEOUT_SUPPORT_EN         1

    #ifndef LL_BIS_SYNC_TEST
    #define LL_BIS_SYNC_TEST                                    1
    #endif

    #ifndef LL_CON_PER_BV88C
    #define LL_CON_PER_BV88C                                    0   //LL/CON/PER/BV-88-C
    #endif

    #ifndef LL_CON_PER_BV98C_AND_CON_CEN_BV94C //Fix EBQ's case bug, remove latter
    #define LL_CON_PER_BV98C_AND_CON_CEN_BV94C                  0   //LL/CON/CEN/BV-94-C, LL/CON/PER/BV-98-C
    #endif

    #ifndef LL_BIS_SNC_BV18C_BN6
    #define LL_BIS_SNC_BV18C_BN6                                0   //LL/BIS/SNC/BV-18-C
    #endif

    #define HCI_SEND_NUM_OF_CMP_AFT_ACK                         1   //for EBQ test
#else
    #define BIS_ADV_EBQ                                         0
    #define LL_DDI_ADV_BV61C                                    0
    #define PDA_SYNC_EBQ                                        0
    #define EXTENDED_ADV_RPT_MANUAL_EN                          0
    #define LONG_CTRL_PDUS_AUTO_FEATURE_REQ_DIS                 0
    #define LL_CON_PER_BV88C                                    0
    #define LL_CON_PER_BV98C_AND_CON_CEN_BV94C                  0
    #define LL_CON_PER_BV105C                                   0
    #define ONLY_FOR_EBQ_TEST_LATER_REMOVE                      0

    #define BQB_HOST_SEND_EMPTY_ACL_DATA_HANDLE_EN              0
    #define BQB_HCI_LOCAL_SUP_CMD                               0
#endif



#ifndef  WALKAROUND_ISO_TIMESTAMP_EN
#define  WALKAROUND_ISO_TIMESTAMP_EN            0
#endif

#ifndef   NEED_MORE_TEST_TO_CONFIRM
#define   NEED_MORE_TEST_TO_CONFIRM             1
#endif










/* TSWR = TestCase Work Around */
#if (BQB_TEST_EN)

#define BQB_TSWR_LL_CON_INI_BV_27_C                                  1

#endif







/* TSWR = TestCase Work Around */



/* LL/CON/INI/BV-27-C [Connection Initiation with Valid Access Address]
 * For each of the 32 bit positions in the access addresses in the CONNECT_IND PDU, each bit is set
 * to 0 in at least three access addresses and set to 1 in at least three access addresses.
 * SiHui: It's reasonable requirements, because access code need to be randomly generated according to Core SPEC,
 * So BQB check if any one of 32 bit is random enough.
 *
 * EBQ test 18 rounds, detect the least bit of access code, need at least three "0" and three "1"
 * the algorithm of "blt_ll_connCalcAccessAddr_v2" do note consider this, some bit are constant
 */
#ifndef BQB_TSWR_LL_CON_INI_BV_27_C
#define BQB_TSWR_LL_CON_INI_BV_27_C                             0
#endif




#endif /* __LL_BQB_STACK_H_ */
