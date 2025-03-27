/********************************************************************************************************
 * @file    cis_peripheral.h
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
#ifndef CIS_PERIPHERAL_H_
#define CIS_PERIPHERAL_H_



#define         CIS_SLV_PARAM_LEN                           616 //user can't modify this value !!!




/**
 * @brief      for user to initialize CIS peripheral module and allocate CIS peripheral parameters buffer.
 * @param[in]  pCisPerParamBuf - start address of CIS peripheral parameters buffer
 * @param[in]  cis_per_num - CIS peripheral number application layer may use
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_initCisPeriphrModule_initCisPeriphrParametersBuffer(u8 *pCisPerParamBuf, int cis_per_num);


/**
 * @brief      this function is used by the Peripheral's Host to inform the Controller to accept the request for the CIS
 *             that is identified by the Connection_Handle
 * @param[in]  cisHandle - Connection handle of the CIS
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_acceptCisRequest(u16 cisHandle);


/**
 * @brief      this function  is used by the Peripheral's Host to inform the Controller to reject the request for the CIS
 *             that is identified by the Connection_Handle.
 * @param[in]  cisHandle - Connection handle of the CIS to be rejected
 * @param[in]  reason - Reason the CIS request was rejected
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_rejectCisReq(u16 cisHandle, u8 reason);



#endif /* CIS_PERIPHERAL_H_ */
