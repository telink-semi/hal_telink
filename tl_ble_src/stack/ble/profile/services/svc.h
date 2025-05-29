/********************************************************************************************************
 * @file    svc.h
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
#pragma once

#if ((!defined(HOST_V2_ENABLE)))
#define SERVICE_GATT_START_HANDLE          0x0001

#define SERVICE_LE_AUDIO_START_HDL         0x0200

#define SERVICE_CHANNEL_SOUNDING_START_HDL 0x0800

#define SERVICE_ELECTRONIC_SHELF_LABEL_HDL 0x0820

#define SERVICE_HID_START_HDL              0x0880

//Telink private Service all 128 uuid
#define SERVICE_TELINK_PRIVATE_START_HDL 0x8000


#include "svc_gatt/svc_gatt.h"
#include "svc_cs/svc_cs.h"
#include "svc_telink/svc_telink.h"
#include "svc_hid/svc_hid.h"
#include "svc_esl/svc_esl.h"

#include "svc_uuid.h"
#else
#include "stack/ble/host_v1/services/svc_gatt/bas/svc_battery.h"
#include "stack/ble/host_v1/services/svc_gatt/core/svc_core.h"
#include "stack/ble/host_v1/services/svc_gatt/dis/svc_dis.h"
#include "stack/ble/host_v1/services/svc_gatt/scps/svc_scps.h"

#include "stack/ble/host_v1/services/svc_telink/ota/svc_ota.h"

#include "stack/ble/host_v1/services/svc_hid/svc_keyboard/svc_keyboard.h"
#include "stack/ble/host_v1/services/svc_hid/svc_mouse/svc_mouse.h"
#include "stack/ble/host_v1/services/svc_hid/svc_km/svc_km.h"
#include "stack/ble/host_v1/services/svc_hid/hid/svc_hid.h"
#include "stack/ble/host_v1/services/svc_hid/ull_hid/svc_ull_hid.h"

#include "stack/ble/host_v1/services/svc_audio/svc_audio.h"
#include "stack/ble/host_v1/services/svc_audio/aics/svc_aics.h"
#include "stack/ble/host_v1/services/svc_audio/bap/svc_ascs.h"
#include "stack/ble/host_v1/services/svc_audio/bap/svc_bass.h"
#include "stack/ble/host_v1/services/svc_audio/bap/svc_pacs.h"
#include "stack/ble/host_v1/services/svc_audio/cas/svc_cas.h"

#include "stack/ble/host_v1/services/svc_audio/csis/svc_csis.h"
#include "stack/ble/host_v1/services/svc_audio/has/svc_has.h"
#include "stack/ble/host_v1/services/svc_audio/mcs/svc_gmcs.h"
#include "stack/ble/host_v1/services/svc_audio/mcs/svc_mcs.h"
#include "stack/ble/host_v1/services/svc_audio/mcs/svc_ots.h"
#include "stack/ble/host_v1/services/svc_audio/micp/svc_micp.h"
#include "stack/ble/host_v1/services/svc_audio/micp/svc_mics.h"
#include "stack/ble/host_v1/services/svc_audio/tbs/svc_gtbs.h"
#include "stack/ble/host_v1/services/svc_audio/tbs/svc_tbs.h"
#include "stack/ble/host_v1/services/svc_audio/tmas/svc_tmas.h"
#include "stack/ble/host_v1/services/svc_audio/vcp/svc_vcp.h"
#include "stack/ble/host_v1/services/svc_audio/vcp/svc_vcs.h"
#include "stack/ble/host_v1/services/svc_audio/vcp/svc_vocs.h"

#include "stack/ble/host_v1/services/svc_cs/svc_cs.h"
#endif
