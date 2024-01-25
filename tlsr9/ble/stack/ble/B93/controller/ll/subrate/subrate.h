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

#ifndef STACK_BLE_CONTROLLER_LL_ACL_CONN_ACL_SUBRATE_H_
#define STACK_BLE_CONTROLLER_LL_ACL_CONN_ACL_SUBRATE_H_

ble_sts_t blc_hci_le_subrate_request(hci_le_subrateRequestCmdParams_t *pCmdPara);
void		blc_ll_initConnSubrate_feature(void);
ble_sts_t	blc_setHostFeatureConnSubrate_en(u8 en);
ble_sts_t   blc_hci_le_set_default_subrate(hci_le_setDefaultSubrateCmdParams_t *pCmdPara);

#endif /* STACK_BLE_CONTROLLER_LL_ACL_CONN_ACL_SUBRATE_H_ */
