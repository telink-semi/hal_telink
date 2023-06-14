/******************************************************************************
 * Copyright (c) 2023 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef BIS_H_
#define BIS_H_


#define			BIS_PARAM_LENGTH								sizeof(ll_bis_t)	//280	todo SDK release should modify it to 288 user can't modify this value !!!





/*
 * @brief      for user to allocate bis parameters buffer. both broadcast and Synchronize use the API.
 * @param[in]  pBisPara - start address of BIS parameters buffer.
 * @param[in]  bis_bcst_num - the bis broadcast number application layer may use.
 * @param[in]  bis_sync_num - the bis Synchronized number application layer may use.
 * @return     status, 0x00:  succeed
 * 			           other: failed
 */
ble_sts_t	blc_ll_InitBisParametersBuffer(u8 *pBisPara, u8 bis_bcst_num, u8 bis_sync_num);




#endif
