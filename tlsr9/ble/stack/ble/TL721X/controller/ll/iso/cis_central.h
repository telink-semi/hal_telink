/********************************************************************************************************
 * @file    cis_central.h
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
#ifndef CIS_CENTRAL_H_
#define CIS_CENTRAL_H_


#define         CIG_PARAM_LEN                                   660 //user can't modify this value !!!



/**
 * @brief      for user to initialize CIS central module and allocate CIG parameters buffer.
 * @param[in]  pCigParamBuf - start address of CIG parameters buffer
 * @param[in]  cig_num - CIG number application layer may use
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_initCisCentralModule_initCigParametersBuffer(u8 *pCigParamBuf, int cig_num);







/**
 * @brief      set CIG anchor point offset from ACL Central anchor point
 *             attention: this is special timing customization !!!
 * @param[in]  acl_cen_index - ACL central index
 * @param[in]  offset_custom_us - customized offset uS
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_setCigTimingOffsetOfAclCentral(u8 acl_cen_index, u16 offset_custom_us);





#endif /* CIS_CENTRAL_H_ */
