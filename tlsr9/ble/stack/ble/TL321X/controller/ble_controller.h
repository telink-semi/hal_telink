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
#ifndef BLE_CONTROLLER_H_
#define BLE_CONTROLLER_H_



#include "stack/ble/TL321X/ble_common.h"
#include "stack/ble/TL321X/ble_format.h"


#include "stack/ble/TL321X/hci/hci.h"
#include "stack/ble/TL321X/hci/hci_const.h"
#include "stack/ble/TL321X/hci/hci_cmd.h"
#include "stack/ble/TL321X/hci/hci_event.h"

#include "stack/ble/TL321X/controller/ll/ll.h"
#include "stack/ble/TL321X/controller/ll/ll_pm.h"

#include "stack/ble/TL321X/controller/ll/acl_conn/acl_conn.h"
#include "stack/ble/TL321X/controller/ll/acl_conn/acl_peripheral.h"
#include "stack/ble/TL321X/controller/ll/acl_conn/acl_central.h"


#include "stack/ble/TL321X/controller/ll/adv/adv.h"
#include "stack/ble/TL321X/controller/ll/adv/leg_adv.h"
#include "stack/ble/TL321X/controller/ll/adv/ext_adv.h"

#include "stack/ble/TL321X/controller/ll/scan/scan.h"
#include "stack/ble/TL321X/controller/ll/scan/leg_scan.h"
#include "stack/ble/TL321X/controller/ll/scan/ext_scan.h"


#include "stack/ble/TL321X/controller/ll/init/init.h"
#include "stack/ble/TL321X/controller/ll/init/leg_init.h"
#include "stack/ble/TL321X/controller/ll/init/ext_init.h"


#include "stack/ble/TL321X/controller/ll/prdadv/pda.h"
#include "stack/ble/TL321X/controller/ll/prdadv/prd_adv.h"
#include "stack/ble/TL321X/controller/ll/prdadv/pda_sync.h"




#include "stack/ble/TL321X/controller/whitelist/whitelist.h"
#include "stack/ble/TL321X/controller/whitelist/resolvlist.h"

#include "stack/ble/TL321X/controller/csa/csa.h"

#include "stack/ble/TL321X/controller/phy/phy.h"
#include "stack/ble/TL321X/controller/phy/phy_test.h"



#include "stack/ble/TL321X/controller/contr_comp.h"

#include "algorithm/algorithm.h"

/*********************************************************/
//Remove when file merge to SDK //
#include "stack/ble/TL321X/ble_config.h"

#if (BQB_LOWER_TESTER_ENABLE)
	#include "stack/ble/TL321X/bqb/bqb.h"
	#include "stack/ble/TL321X/bqb/bqb_ll.h"
#endif
/*********************************************************/

#endif /* BLE_H_ */