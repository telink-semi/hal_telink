/********************************************************************************************************
 * @file    ras_server_data.h
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
#pragma once



/**
 * @brief     Start procedure.
 * @param[in] *procedureHead: the pointer of procedure parameter.
 * @return    CS_RAS_SUCCESS - success
 *            other          - error
 */
int blc_rass_procedureEnComplete(hci_le_csProcedureEnableCompleteEvt_t *procedureHead);

/**
 * @brief     Handle subevent result event from HCI.
 * @param[in] *resultEvt: the pointer of subevent result event data.
 * @return    CS_RAS_SUCCESS - success
 *            other          - error
 */
int blc_rass_subeventResultData(hci_le_csSubeventResultEvt_t *resultEvt);

/**
 * @brief     Handle subevent result continue event from HCI.
 * @param[in] *resultEvt: the pointer of subevent result continue event data.
 * @return    CS_RAS_SUCCESS - success
 *            other          - error
 */
int blc_rass_subeventResultContinueData(hci_le_csSubeventResultContinueEvt_t *continueEvt);

/**
 * @brief          for calculate the ranging data of one procedure.
 * @param[in]      connHandle: ACL handle..
 * @param[in]      *rangData: the pointer of ranging data.
 * @param[in]      length: the data length of ranging data.
 * @param[in][out] *distance1: if it is mode1, then this parameter is the distance result of mode1;
 *                             if it is mode2, then this parameter is the phase calculation result of mode2.
 * @param[in][out] *distance2: if it is mode1, then this parameter is the distance result of mode1, and this parameter is the same as distance1.
 *                             if it is mode2, then this parameter is the music calculation result of mode2.
 * @return         0     - the result of distance is valid.
 *                 other - the result of distance is invalid.
 */
u32 blc_rass_calcRangData(u16 connHandle, u8 *rangData, u32 length, float *distance1, float *distance2);



