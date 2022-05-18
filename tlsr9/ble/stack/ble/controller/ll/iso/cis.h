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
#ifndef CIS_H_
#define CIS_H_



//Note: user can't modify this value,and this value must 4 byte aligned
#define		CIS_CONN_PARAM_LENGTH		(344)




ble_sts_t 	blc_ll_initCisConnectionParameters( u8 *pCisConnPara, u32 master_cis_num, u32 slave_cis_num);

_attribute_ram_code_ void irq_cis_conn_tx(void);

#endif /* CIS_H_ */
