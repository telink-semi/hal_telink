/******************************************************************************
 * Copyright (c) 2022 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/
#ifndef CIS_SLAVE_H_
#define CIS_SLAVE_H_

//#if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE)




#define		CIG_SLV_PARAM_LEN		(416)   //Note: user can't modify this value,and this value must 4 byte aligned




/**
 * @brief      for user to initialize CIS slave module.
 * @param	   none
 * @return     none
 */
void 		blc_ll_initCisSlave_module(void);


/**
 * @brief
 * @param
 * @param
 * @return     ble_sts_t
 */
ble_sts_t 	blc_ll_initCisSlaveParameters( u8 *pCisSlavePara, int cis_slv_num);


/**
 * @brief
 * @param
 * @param
 * @return     ble_sts_t
 */
ble_sts_t 	blc_hci_le_acceptCisReq(u16 cisHandle);


/**
 * @brief
 * @param
 * @param
 * @return     ble_sts_t
 */
ble_sts_t 	blc_hci_le_rejectCisReq(u16 cisHandle, u8 reason, u8* pRetParam);



#endif


//#endif /* CIS_MASTER_H_ */
