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
#ifndef BIS_H_
#define BIS_H_


//Note: user can't modify this value,and this value must 4 byte aligned
#define		BIS_PARAM_LENGTH		(192)



u32 		blc_ll_getAvailBisNum(u8 role);
u32 		blt_ll_bis_getAccessCode(u32 seedAccessCode, u8 bisSeq);
u32 		blt_ll_bis_getSeedAccessAddr(void);

ble_sts_t	blc_ll_InitBisConnectionlessParameters(u8 *pBisPara, u8 bis_bcst_num, u8 bis_sync_num);


#endif
