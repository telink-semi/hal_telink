/********************************************************************************************************
 * @file    bqb_profile.h
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
#ifndef BQB_LE_H_
#define BQB_LE_H_

#include "app_config.h"

#if (FEATURE_TEST_MODE == TEST_GATTC_SDP)
#if (1 || BQB_PROFILE_ENABLE)

#define BQB_LE_START            0x3000
#define BQB_LE_AICS_START       (BQB_LE_START+0)
#define BQB_LE_CCP_START        (BQB_LE_START+0x0100)
#define BQB_LE_CSIP_START       (BQB_LE_START+0x0200)
#define BQB_LE_CSIS_START       (BQB_LE_START+0x0300)
#define BQB_LE_GMCS_START       (BQB_LE_START+0x0400)
#define BQB_LE_GTBS_START       (BQB_LE_START+0x0500)
#define BQB_LE_IOPT_START       (BQB_LE_START+0x0600)
#define BQB_LE_MCP_START        (BQB_LE_START+0x0700)
#define BQB_LE_MCS_START        (BQB_LE_START+0x0800)
#define BQB_LE_MICP_START       (BQB_LE_START+0x0900)
#define BQB_LE_MICS_START       (BQB_LE_START+0x0A00)
#define BQB_LE_OTS_START        (BQB_LE_START+0x0B00)
#define BQB_LE_TBS_START        (BQB_LE_START+0x0C00)
#define BQB_LE_VCP_START        (BQB_LE_START+0x0D00)
#define BQB_LE_VCS_START        (BQB_LE_START+0x0E00)
#define BQB_LE_VOCS_START       (BQB_LE_START+0x0F00)
#define BQB_LE_ASCS_START       (BQB_LE_START+0x1000)
#define BQB_LE_BAP_START        (BQB_LE_START+0x1100)
#define BQB_LE_BASS_START       (BQB_LE_START+0x1200)
#define BQB_LE_PACS_START       (BQB_LE_START+0x1300)

//ASCP
#define BQB_ASCP_BAP_CL_CGGIT_CHA_BV_02_C                (BQB_LE_AICS_START+0x01)
#define BQB_ASCP_BAP_CL_CGGIT_CHA_BV_04_C                (BQB_LE_AICS_START+0x02)
#define BQB_ASCP_BAP_UCL_CGGIT_CHA_BV_03_C               (BQB_LE_AICS_START+0x03)
#define BQB_ASCP_BAP_UCL_SCC_BV_004_038_070_072_C        (BQB_LE_AICS_START+0x04)
#define BQB_ASCP_BAP_UCL_SCC_BV_020_022_054_086_C        (BQB_LE_AICS_START+0x05)
#define BQB_ASCP_BAP_UCL_PD_BV_03_C                      (BQB_LE_AICS_START+0x06)
#define BQB_ASCP_BAP_UCL_STR_BV_039_103_235_251_300_316_318_C    (BQB_LE_AICS_START+0x07)
#define BQB_ASCP_BAP_UCL_STR_BV_007_071_365_381_C                (BQB_LE_AICS_START+0x08)
#define BQB_ASCP_BAP_UCL_PD_BV_03_04_C                   (BQB_LE_AICS_START+0x09)
#define BQB_ASCP_BAP_UCL_STR_BV_142_190_C                (BQB_LE_AICS_START+0x0A)
#define BQB_ASCP_BAP_UCL_STR_BV_267_C                    (BQB_LE_AICS_START+0x0B)
#define BQB_ASCP_BAP_UCL_STR_TEST                        (BQB_LE_AICS_START+0x0C)
#define BQB_ASCP_BAP_UCL_STR_TEST1                        (BQB_LE_AICS_START+0x0D)
#define BQB_ASCP_BAP_UCL_STR_TEST2                        (BQB_LE_AICS_START+0x0E)
#define BQB_ASCP_BAP_UCL_SCC_BV_103_C                   (BQB_LE_AICS_START+0x0F)
#define BQB_ASCP_BAP_UCL_SCC_BV_106_C                   (BQB_LE_AICS_START+0x10)
#define BQB_ASCP_BAP_UCL_STR_BV_397_531_C               (BQB_LE_AICS_START+0x11)
#define BQB_ASCP_BAP_UCL_STR_BV_461_533_C               (BQB_LE_AICS_START+0x12)
#define BQB_ASCP_BAP_UCL_SCC_BV_115_C                   (BQB_LE_AICS_START+0x13)

//CSIP -- 22
#define BQB_CSIP_CL_CGGIT_SER_BV_01_C     (BQB_LE_CSIP_START+0x01)
#define BQB_CSIP_CL_CGGIT_CHA_BV_01_C     (BQB_LE_CSIP_START+0x11)
#define BQB_CSIP_CL_CGGIT_CHA_BV_02_C     (BQB_LE_CSIP_START+0x12)
#define BQB_CSIP_CL_CGGIT_CHA_BV_03_C     (BQB_LE_CSIP_START+0x13)
#define BQB_CSIP_CL_CGGIT_CHA_BV_04_C     (BQB_LE_CSIP_START+0x14)
#define BQB_CSIP_CL_SP_BV_01_C            (BQB_LE_CSIP_START+0x21)
#define BQB_CSIP_CL_SP_BV_03_C            (BQB_LE_CSIP_START+0x23)
#define BQB_CSIP_CL_SP_BV_04_C            (BQB_LE_CSIP_START+0x24)
#define BQB_CSIP_CL_SP_BV_06_C            (BQB_LE_CSIP_START+0x26)
#define BQB_CSIP_CL_SP_BV_07_C            (BQB_LE_CSIP_START+0x27)
#define BQB_CSIP_CL_SPE_BI_01_C           (BQB_LE_CSIP_START+0x31)
#define BQB_CSIP_CL_SPE_BI_02_C           (BQB_LE_CSIP_START+0x32)
#define BQB_CSIP_CL_SPE_BI_03_C           (BQB_LE_CSIP_START+0x33)
#define BQB_CSIP_CL_SPE_BI_04_C           (BQB_LE_CSIP_START+0x34)
#define BQB_CSIP_WRITE                    (BQB_LE_CSIP_START+0xFE)
#define BQB_CSIP_READ                     (BQB_LE_CSIP_START+0xFF)

//VCP -- 49
#define BQB_VCP_VC_CGGIT_SER_BV_01_C     (BQB_LE_VCP_START+0x01)  //
#define BQB_VCP_VC_CGGIT_SER_BV_02_C     (BQB_LE_VCP_START+0x02)  //
#define BQB_VCP_VC_CGGIT_SER_BV_03_C     (BQB_LE_VCP_START+0x03)  //
#define BQB_VCP_VC_CGGIT_CHA_BV_01_C     (BQB_LE_VCP_START+0x04) //
#define BQB_VCP_VC_CGGIT_CHA_BV_02_C     (BQB_LE_VCP_START+0x05)
#define BQB_VCP_VC_CGGIT_CHA_BV_03_C     (BQB_LE_VCP_START+0x06)
#define BQB_VCP_VC_CGGIT_CHA_BV_04_C     (BQB_LE_VCP_START+0x07)
#define BQB_VCP_VC_CGGIT_CHA_BV_05_C     (BQB_LE_VCP_START+0x08)
#define BQB_VCP_VC_CGGIT_CHA_BV_06_C     (BQB_LE_VCP_START+0x09)
#define BQB_VCP_VC_CGGIT_CHA_BV_07_C     (BQB_LE_VCP_START+0x0a)
#define BQB_VCP_VC_CGGIT_CHA_BV_08_C     (BQB_LE_VCP_START+0x0b)
#define BQB_VCP_VC_CGGIT_CHA_BV_09_C     (BQB_LE_VCP_START+0x0c)
#define BQB_VCP_VC_CGGIT_CHA_BV_10_C     (BQB_LE_VCP_START+0x0d)
#define BQB_VCP_VC_CGGIT_CHA_BV_11_C     (BQB_LE_VCP_START+0x0e)
#define BQB_VCP_VC_CGGIT_CHA_BV_12_C     (BQB_LE_VCP_START+0x0f)
#define BQB_VCP_VC_CGGIT_CHA_BV_13_C     (BQB_LE_VCP_START+0x10)
#define BQB_VCP_VC_SPE_BI_01_C           (BQB_LE_VCP_START+0x11)
#define BQB_VCP_VC_SPE_BI_02_C           (BQB_LE_VCP_START+0x12)
#define BQB_VCP_VC_SPE_BI_03_C           (BQB_LE_VCP_START+0x13)
#define BQB_VCP_VC_SPE_BI_04_C           (BQB_LE_VCP_START+0x14)
#define BQB_VCP_VC_SPE_BI_05_C           (BQB_LE_VCP_START+0x15)
#define BQB_VCP_VC_SPE_BI_06_C           (BQB_LE_VCP_START+0x16)
#define BQB_VCP_VC_SPE_BI_07_C           (BQB_LE_VCP_START+0x17)
#define BQB_VCP_VC_SPE_BI_08_C           (BQB_LE_VCP_START+0x18)
#define BQB_VCP_VC_SPE_BI_09_C           (BQB_LE_VCP_START+0x19)
#define BQB_VCP_VC_SPE_BI_10_C           (BQB_LE_VCP_START+0x1a)
#define BQB_VCP_VC_SPE_BI_11_C           (BQB_LE_VCP_START+0x1b)
#define BQB_VCP_VC_SPE_BI_12_C           (BQB_LE_VCP_START+0x1c)
#define BQB_VCP_VC_SPE_BI_13_C           (BQB_LE_VCP_START+0x1d)
#define BQB_VCP_VC_SPE_BI_14_C           (BQB_LE_VCP_START+0x1e)
#define BQB_VCP_VC_SPE_BI_15_C           (BQB_LE_VCP_START+0x1f)
#define BQB_VCP_VC_SPE_BI_16_C           (BQB_LE_VCP_START+0x20)
#define BQB_VCP_VC_SPE_BI_17_C           (BQB_LE_VCP_START+0x21)
#define BQB_VCP_VC_SPE_BI_18_C           (BQB_LE_VCP_START+0x22)
#define BQB_VCP_VC_SPE_BI_19_C           (BQB_LE_VCP_START+0x23)
#define BQB_VCP_VC_SPE_BI_20_C           (BQB_LE_VCP_START+0x24)
#define BQB_VCP_VC_VCCP_BV_01_C          (BQB_LE_VCP_START+0x25)
#define BQB_VCP_VC_VCCP_BV_02_C          (BQB_LE_VCP_START+0x26)
#define BQB_VCP_VC_VCCP_BV_03_C          (BQB_LE_VCP_START+0x27)
#define BQB_VCP_VC_VCCP_BV_04_C          (BQB_LE_VCP_START+0x28)
#define BQB_VCP_VC_VCCP_BV_05_C          (BQB_LE_VCP_START+0x29)
#define BQB_VCP_VC_VCCP_BV_06_C          (BQB_LE_VCP_START+0x2a)
#define BQB_VCP_VC_VCCP_BV_07_C          (BQB_LE_VCP_START+0x2b)
#define BQB_VCP_VC_AICP_BV_01_C          (BQB_LE_VCP_START+0x2c)
#define BQB_VCP_VC_AICP_BV_02_C          (BQB_LE_VCP_START+0x2d)
#define BQB_VCP_VC_AICP_BV_03_C          (BQB_LE_VCP_START+0x2e)
#define BQB_VCP_VC_AICP_BV_04_C          (BQB_LE_VCP_START+0x2f)
#define BQB_VCP_VC_AICP_BV_05_C          (BQB_LE_VCP_START+0x30)
#define BQB_VCP_VC_VOCP_BV_01_C          (BQB_LE_VCP_START+0x31)

#define BQB_VCP_VC_READ                  (BQB_LE_VCP_START+0xFE)
#define BQB_VCP_VC_WRITE                 (BQB_LE_VCP_START+0xFF)



void app_bqb_init(void);

void app_bqb_connect(u16 aclHandle);
void app_bqb_disconn(u16 aclHandle);

void app_bqb_handler(void);


#endif //BQB_PROFILE_ENABLE
#endif //#if (FEATURE_TEST_MODE == TEST_GATTC_SDP)


#endif //BQB_LE_H_

