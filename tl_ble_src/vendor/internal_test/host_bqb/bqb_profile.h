/********************************************************************************************************
 * @file    bqb_profile.h
 *
 * @brief   This is the source file for BLE SDK
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
#ifndef BQB_LE_H_
#define BQB_LE_H_

#include "app_config.h"

#if (INTER_TEST_MODE == TEST_HOST_BQB)


    #define BQB_LE_ACTION_START       0x0000
    #define BQB_LE_HOST_START         0x1000
    #define BQB_LE_PROFILE_START      0x2000

    #define BQB_LE_GAP_START          (BQB_LE_HOST_START + 0x0000)
    #define BQB_LE_GATT_START         (BQB_LE_HOST_START + 0x0100)
    #define BQB_LE_L2CAP_START        (BQB_LE_HOST_START + 0x0200)
    #define BQB_LE_SM_START           (BQB_LE_HOST_START + 0x0300)

    #define BQB_UI_OPERATION_ENABLE   1
    #define BQB_GAP_TESTCASE_ENABLE   1
    #define BQB_GATT_TESTCASE_ENABLE  1
    #define BQB_L2CAP_TESTCASE_ENABLE 1
    #define BQB_SM_TESTCASE_ENABLE    1

    //#define GATT_SR_GAS_BV_05_c                   1

    //UI
    #if (BQB_UI_OPERATION_ENABLE)
        #define BQB_LE_START_PAIR    (BQB_LE_ACTION_START + 0x0000)
        #define BQB_LE_START_UNPAIR  (BQB_LE_ACTION_START + 0x0001)
        #define BQB_LE_REBOOT_DEV    (BQB_LE_ACTION_START + 0x0002)
        #define BQB_LE_ENTER_PINCODE (BQB_LE_ACTION_START + 0x0003)


    #endif //#if (BQB_UI_OPERATION_ENABLE)

    //GAP
    #if (BQB_GAP_TESTCASE_ENABLE)
        #define GAP_BROB_BCST_BV_01_C (BQB_LE_GAP_START + 0x01)


    #endif //#if (BQB_GAP_TESTCASE_ENABLE)


    //GATT
    #if (BQB_GATT_TESTCASE_ENABLE)
        #define GATT_CL_GAC_BV_01_C      (BQB_LE_GATT_START + 0x01)

        #define GATT_SR_GAR_BI_03_C      (BQB_LE_GATT_START + 0x02)
        #define GATT_SR_GAN_BV_01_C      (BQB_LE_GATT_START + 0x03)
        #define GATT_SR_GAN_BV_02_C      (BQB_LE_GATT_START + 0x04)
        #define GATT_SR_GAI_BV_01_C      (BQB_LE_GATT_START + 0x05)
        #define GATT_SR_GAS_BV_01_C      (BQB_LE_GATT_START + 0x06)

        #define GATT_CL_WRITE_ATTR_VALUE (BQB_LE_GATT_START + 0x07)
        #define GATT_CL_READ_ATTR_VALUE  (BQB_LE_GATT_START + 0x08)
        #define GATT_CL_DISCOVERY        (BQB_LE_GATT_START + 0x09)
    #endif //#if (BQB_GATT_TESTCASE_ENABLE)


    //L2CAP
    #if (BQB_L2CAP_TESTCASE_ENABLE)
        #define L2CAP_COS_CFC_BV_01_C (BQB_LE_L2CAP_START + 0x01)

    #endif //#if (BQB_L2CAP_TESTCASE_ENABLE)


    //SM
    #if (BQB_SM_TESTCASE_ENABLE)
        #define SM_CEN_PROT_BV_01_C (BQB_LE_SM_START + 0x01)
        #define SM_CEN_JW_BV_05_C   (BQB_LE_SM_START + 0x02)
        #define SM_CEN_JW_BI_04_C   (BQB_LE_SM_START + 0x03)
        #define SM_CEN_JW_BI_01_C   (BQB_LE_SM_START + 0x04)

        #define SM_CEN_PKE_BV_01_C  (SM_CEN_JW_BI_01_C + 0x01)
        #define SM_CEN_PKE_BV_04_C  (SM_CEN_JW_BI_01_C + 0x02)
        #define SM_CEN_PKE_BI_01_C  (SM_CEN_JW_BI_01_C + 0x03)
        #define SM_CEN_PKE_BI_02_C  (SM_CEN_JW_BI_01_C + 0x04)
        #define SM_CEN_OOB_BV_05_C  (SM_CEN_JW_BI_01_C + 0x05)
        #define SM_CEN_OOB_BV_07_C  (SM_CEN_JW_BI_01_C + 0x06)
        #define SM_CEN_EKS_BV_01_C  (SM_CEN_JW_BI_01_C + 0x07)
        #define SM_CEN_EKS_BI_01_C  (SM_CEN_JW_BI_01_C + 0x08)

        #define SM_CEN_KDU_BI_01_C  (SM_CEN_JW_BI_01_C + 0x09)
        #define SM_CEN_KDU_BI_02_C  (SM_CEN_JW_BI_01_C + 0x0A)
        #define SM_CEN_KDU_BI_03_C  (SM_CEN_JW_BI_01_C + 0x0B)
        #define SM_CEN_KDU_BV_05_C  (SM_CEN_JW_BI_01_C + 0x0C)
        #define SM_CEN_KDU_BV_06_C  (SM_CEN_JW_BI_01_C + 0x0D)
        #define SM_CEN_KDU_BV_10_C  (SM_CEN_JW_BI_01_C + 0x0E)

        #define SM_CEN_SIP_BV_02_C  (SM_CEN_JW_BI_01_C + 0x0F)
        #define SM_CEN_SCJW_BV_01_C (SM_CEN_JW_BI_01_C + 0x10)
        #define SM_CEN_SCJW_BI_01_C (SM_CEN_JW_BI_01_C + 0x11)

        #define SM_CEN_SCPK_BV_01_C (SM_CEN_JW_BI_01_C + 0x12)
        #define SM_CEN_SCPK_BV_04_C (SM_CEN_JW_BI_01_C + 0x13)
        #define SM_CEN_SCPK_BI_01_C (SM_CEN_JW_BI_01_C + 0x14)
        #define SM_CEN_SCPK_BI_02_C (SM_CEN_JW_BI_01_C + 0x15)

        #define SM_CEN_OOB_BV_01_C  (SM_CEN_SCPK_BI_02_C + 0x01)
        #define SM_CEN_OOB_BV_03_C  (SM_CEN_SCPK_BI_02_C + 0x02)
        #define SM_CEN_OOB_BV_09_C  (SM_CEN_SCPK_BI_02_C + 0x03)
        #define SM_CEN_OOB_BI_01_C  (SM_CEN_SCPK_BI_02_C + 0x04)
        #define SM_CEN_SCOB_BV_01_C (SM_CEN_SCPK_BI_02_C + 0x05)
        #define SM_CEN_SCOB_BV_04_C (SM_CEN_SCPK_BI_02_C + 0x06)
        #define SM_CEN_SCOB_BI_01_C (SM_CEN_SCPK_BI_02_C + 0x07)
        #define SM_CEN_SCOB_BI_04_C (SM_CEN_SCPK_BI_02_C + 0x08)

        //peripheral
        #define SM_PER_PROT_BV_02_C (SM_CEN_SCOB_BI_04_C + 0x01)
        #define SM_PER_JW_BV_02_C   (SM_CEN_SCOB_BI_04_C + 0x02)
        #define SM_PER_JW_BI_03_C   (SM_CEN_SCOB_BI_04_C + 0x03)
        #define SM_PER_JW_BI_02_C   (SM_CEN_SCOB_BI_04_C + 0x04)

        #define SM_PER_PKE_BV_02_C  (SM_CEN_SCOB_BI_04_C + 0x05)
        #define SM_PER_PKE_BV_05_C  (SM_CEN_SCOB_BI_04_C + 0x06)
        #define SM_PER_PKE_BI_03_C  (SM_CEN_SCOB_BI_04_C + 0x07)
        #define SM_PER_EKS_BV_02_C  (SM_CEN_SCOB_BI_04_C + 0x08)
        #define SM_PER_EKS_BI_02_C  (SM_CEN_SCOB_BI_04_C + 0x09)

        #define SM_PER_OOB_BV_02_C  (SM_CEN_SCOB_BI_04_C + 0x0A)
        #define SM_PER_OOB_BV_04_C  (SM_CEN_SCOB_BI_04_C + 0x0B)
        #define SM_PER_OOB_BV_10_C  (SM_CEN_SCOB_BI_04_C + 0x0C)
        #define SM_PER_OOB_BI_02_C  (SM_CEN_SCOB_BI_04_C + 0x0D)

        #define SM_PER_KDU_BV_01_C  (SM_CEN_SCOB_BI_04_C + 0x0E)
        #define SM_PER_KDU_BV_02_C  (SM_CEN_SCOB_BI_04_C + 0x0F)
        #define SM_PER_KDU_BV_07_C  (SM_CEN_SCOB_BI_04_C + 0x10)
        #define SM_PER_KDU_BV_08_C  (SM_CEN_SCOB_BI_04_C + 0x11)
        #define SM_PER_KDU_BI_01_C  (SM_CEN_SCOB_BI_04_C + 0x12)
        #define SM_PER_KDU_BI_02_C  (SM_CEN_SCOB_BI_04_C + 0x13)
        #define SM_PER_KDU_BI_03_C  (SM_CEN_SCOB_BI_04_C + 0x14)
        #define SM_PER_SCJW_BV_03_C (SM_CEN_SCOB_BI_04_C + 0x15)
        #define SM_PER_SCJW_BI_02_C (SM_CEN_SCOB_BI_04_C + 0x16)

        #define SM_PER_SIP_BV_01_C  (SM_CEN_SCOB_BI_04_C + 0x17)
        #define SM_PER_SIE_BV_01_C  (SM_CEN_SCOB_BI_04_C + 0x18)

        #define SM_PER_SCJW_BV_02_C (SM_CEN_SCOB_BI_04_C + 0x19)

        #define SM_PER_SCPK_BV_02_C (SM_CEN_SCOB_BI_04_C + 0x1A)
        #define SM_PER_SCPK_BV_03_C (SM_CEN_SCOB_BI_04_C + 0x1B)
        #define SM_PER_SCPK_BI_03_C (SM_CEN_SCOB_BI_04_C + 0x1C)
        #define SM_PER_SCPK_BI_04_C (SM_CEN_SCOB_BI_04_C + 0x1D)

        #define SM_PER_OOB_BV_6_C   (SM_CEN_SCOB_BI_04_C + 0x1E)
        #define SM_PER_OOB_BV_8_C   (SM_CEN_SCOB_BI_04_C + 0x1F)


    #endif //#if (BQB_SM_TESTCASE_ENABLE)

void blc_svc_addBqbGattGroup(void);
void app_bqb_init(void);
void app_bqb_connect(u16 aclHandle);
void app_bqb_disconn(u16 aclHandle);
void app_bqb_handler(void);

#endif //#if (INTER_TEST_MODE == TEST_HOST_BQB)
#endif
