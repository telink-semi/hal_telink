/********************************************************************************************************
 * @file    ble_profile.h
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
#ifndef BLE_PROFILE_H_
#define BLE_PROFILE_H_

#include "stack/ble/profile/prf_debug.h"

#include "stack/ble/profile/prf_common/storage/prf_storage.h"

//Profile Common
#include "stack/ble/profile/prf_common/prf_common.h"

//Audio Services
#include "stack/ble/profile/services/svc.h"
#include "stack/ble/profile/services/svc_adv.h"

//////////////////////////Basic Profile///////////////////////////////////
#include "stack/ble/profile/basic_profile/prf_basic.h"

////////////////////// Channel Sounding Ranging  //////////////////////
#include "stack/ble/profile/channel_sounding/prf_cs.h"

///////////////////// Human Interface Device  /////////////////////////
#include "stack/ble/profile/prf_hid/prf_hid.h"

/*********************************************************/
//Remove when file merge to SDK //
#include "stack/ble/profile/channel_sounding/rasp/ras_internal.h"

#include "stack/ble/profile/prf_common/prf_internal.h"

#include "stack/ble/profile/prf_common/storage/store_internal.h"

/*********************************************************/

#include "stack/ble/profile/audio/prf_audio.h"

#endif /* BLE_PROFILE_H_ */
