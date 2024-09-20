/********************************************************************************************************
 * @file    ble_controller.h
 *
 * @brief   This is the header file for BLE SDK
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

// #include "stack/ble/controller/ll/past/past.h"

// #include "stack/ble/controller/ll/pcl/pcl.h"

// #include "stack/ble/controller/ll/chn_class/chn_class.h"

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
// #include "stack/ble/controller/ll/prdadv/PAwR_adv.h"
#include "stack/ble/TL321X/controller/ll/prdadv/pda_sync.h"
// #include "stack/ble/controller/ll/prdadv/PAwR_sync.h"

// #include "stack/ble/controller/ial/ial.h"
// #include "stack/ble/controller/ll/iso/iso.h"

// #include "stack/ble/controller/ll/iso/bis.h"
// #include "stack/ble/controller/ll/iso/bis_bcst.h"
// #include "stack/ble/controller/ll/iso/bis_sync.h"
// #include "stack/ble/controller/ll/iso/cis.h"
// #include "stack/ble/controller/ll/iso/cis_central.h"
// #include "stack/ble/controller/ll/iso/cis_peripheral.h"
// #include "stack/ble/controller/ll/aoa_aod/aoa_aod.h"
// #include "stack/ble/controller/ll/chn_sound/chn_sound.h"


#include "stack/ble/TL321X/controller/whitelist/whitelist.h"
#include "stack/ble/TL321X/controller/whitelist/resolvelist.h"

#include "stack/ble/TL321X/controller/csa/csa.h"
#include "stack/ble/TL321X/controller/phy/phy.h"
#include "stack/ble/TL321X/controller/phy/phy_test.h"


// #include "stack/ble/controller/ll/subrate/subrate.h"

// #include "stack/ble/controller/contr_comp.h"

// #include "stack/ble/device/multi_device.h"

#include "stack/ble/TL321X/controller/contr_comp.h"

// #include "stack/ble/debug/debug.h"

#include "algorithm/algorithm.h"

/*********************************************************/
//Remove when file merge to SDK //
#include "stack/ble/TL321X/ble_config.h"
// #include "stack/ble/ble_stack.h"

// #include "stack/ble/controller/ll/ll_stack.h"
// #include "stack/ble/controller/ll/acl_conn/acl_stack.h"
// #include "stack/ble/controller/ll/adv/adv_stack.h"
// #include "stack/ble/controller/ll/scan/scan_stack.h"
// #include "stack/ble/controller/ll/init/init_stack.h"
// #include "stack/ble/controller/ll/prdadv/pda_stack.h"
// #include "stack/ble/controller/ll/aoa_aod/aoa_stack.h"
// #include "stack/ble/controller/ial/ial_stack.h"
// #include "stack/ble/controller/ll/past/past_stack.h"
// #include "stack/ble/controller/ll/iso/iso_stack.h"
// #include "stack/ble/controller/ll/iso/bis_stack.h"
// #include "stack/ble/controller/ll/iso/cis_stack.h"
// #include "stack/ble/controller/whitelist/whitelist_stack.h"
// #include "stack/ble/controller/csa/csa_stack.h"
// #include "stack/ble/controller/phy/phy_stack.h"
// #include "stack/ble/hci/hci_stack.h"
// #include "stack/ble/controller/ll/subrate/subrate_stack.h"
// #include "stack/ble/controller/ll/chn_sound/chn_sound_stack.h"
// #include "stack/ble/controller/ll/acl_conn/acl_sniffer/acl_sniffer_stack.h"
// #include "stack/ble/hal/hal_internal.h"

// #include "stack/ble/controller/cs_drbg/drbg_stack.h"

// #include "stack/ble/device/device_stack.h"
// #include "stack/ble/darwin/darwin_stack.h"

/*********************************************************/

#endif /* BLE_H_ */
