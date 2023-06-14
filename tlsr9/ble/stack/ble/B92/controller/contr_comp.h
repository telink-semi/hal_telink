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

#ifndef STACK_BLE_CONTROLLER_CONTR_API_COMP_H_
#define STACK_BLE_CONTROLLER_CONTR_API_COMP_H_



#define PM_SLEEP_ACL_SLAVE							PM_SLEEP_ACL_PERIPHR
#define	PM_SLEEP_ACL_MASTER							PM_SLEEP_ACL_CENTRAL


/* for BLE controller old API name compatible with new SDK */

#define	blc_ll_initInitiating_module				blc_ll_initLegacyInitiating_module


#define blc_ll_initAclSlaveRole_module				blc_ll_initAclPeriphrRole_module
#define blc_ll_initAclConnSlaveTxFifo				blc_ll_initAclPeriphrTxFifo



#define blc_ll_initAclMasterRole_module				blc_ll_initAclCentralRole_module
#define blc_ll_initAclConnMasterTxFifo				blc_ll_initAclCentralTxFifo
#define	blc_ll_setAclMasterConnectionInterval		blc_ll_setAclCentralBaseConnectionInterval


#endif /* STACK_BLE_CONTROLLER_CONTR_API_COMP_H_ */
