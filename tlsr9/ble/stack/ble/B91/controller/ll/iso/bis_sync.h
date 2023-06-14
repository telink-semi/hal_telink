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
#ifndef BIS_SYNC_H_
#define BIS_SYNC_H_


#define 		BIG_SYNC_PARAM_LENGTH		(696) // Note: user can't modify this value,and this value must 4 byte aligned



/**
 * @brief      This function is used to initialize BIG Synchronize module.
 * @param	   none
 * @return     none
 */
void 		blc_ll_initBisSynchronize_module(void);

/**
 * @brief      This function is used to initialize BIG Synchronize parameters.
 * @param	   pointer to BIG Synchronize parameters buffer
 * @return     Number of BIG SYNC supported
 */
ble_sts_t	blc_ll_initBigSyncParameters(u8 *pBigSyncPara, u8 bigSyncNum);


/**
 * @brief
 * @param	   none
 * @return     none
 */
ble_sts_t 	blc_hci_le_bigCreateSync(hci_le_bigCreateSyncParams_t* pCmdParam);


/**
 * @brief
 * @param	   none
 * @return     none
 */
ble_sts_t 	blc_hci_le_bigTerminateSync(u8 bigHandle, u8* pRetParam);


/**
 * @brief      Used to enable private LegScan to get BIGINFO for BIG SYNC
 * @param	   none
 * @return     none
 */
void		blc_ll_enPrivLegScanForBigBync(void);


/**
 * @brief      Used to enable scan to get BIGINFO for BIG SYNC
 * @param	   none
 * @return     none
 */
void		blc_ll_enScanForBigBync(void);

/**
 * @brief
 * @param	   none
 * @return     none
 */
ble_sts_t 	blc_ll_bigCreateSync(u8 big_handle, u16 sync_handle, u8 enc, u8 broadcast_code[16],
								 u8 mse, u16 big_sync_timeout, u8  num_bis, u8 *bis);

/**
 * @brief
 * @param	   none
 * @return     none
 */
ble_sts_t 	blc_ll_bigTerminateSync(u8 bigHandle);


#endif /* BIS_SYNC_H_ */
