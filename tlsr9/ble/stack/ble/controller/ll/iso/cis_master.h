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
#ifndef CIS_MASTER_H_
#define CIS_MASTER_H_




#define 		CIG_MST_PARAM_LEN				(436)   //Note: user can't modify this value,and this value must 4 byte aligned
#define 		CIG_ID_0						0
#define 		CIG_ID_1		      			1
#define 		CIG_ID_2		              	2
#define 		CIG_ID_3		              	3
#define 		CIG_ID_INVALID		           	0xFF




/**
 * @brief      for user to initialize CIS master module.
 * @param	   none
 * @return     none
 */
void 		blc_ll_initCisMaster_module(void);


/**
 * @brief
 * @param	   none
 * @return     none
 */
void 		blc_ll_initCisMasterParameters( u8 *pCigPara, int cig_mst_num);


/**
 * @brief
 * @param
 * @param
 * @return     ble_sts_t
 */
ble_sts_t 	blc_hci_le_setCigParams	   (hci_le_setCigParam_cmdParam_t* pCmdParam, 		hci_le_setCigParam_retParam_t* pRetParam);


/**
 * @brief
 * @param
 * @param
 * @return     ble_sts_t
 */
ble_sts_t 	blc_hci_le_setCigParamsTest(hci_le_setCigParamTest_cmdParam_t* pCmdParam, 	hci_le_setCigParam_retParam_t* pRetParam);


/**
 * @brief
 * @param
 * @param
 * @return     ble_sts_t
 */
ble_sts_t 	blc_hci_le_removeCig(u8 cigId, u8* pRetParamm);


/**
 * @brief
 * @param
 * @param
 * @return     ble_sts_t
 */
ble_sts_t 	blc_hci_le_createCis(hci_le_CreateCisParams_t* pCisPara);



#endif
