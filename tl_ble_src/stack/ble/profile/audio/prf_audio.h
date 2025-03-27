/********************************************************************************************************
 * @file    prf_audio.h
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


#include "stack/ble/profile/services/svc_audio/svc_audio.h"

////////////////////////// Audio Common ///////////////////////////////////
//Audio Configure
#include "stack/ble/profile/audio/common/audio_cfg.h"
#include "stack/ble/profile/audio/common/audio.h"
#include "stack/ble/profile/audio/stream/bap.h"
#include "stack/ble/profile/audio/stream/ascp/ascs.h"
#include "stack/ble/profile/audio/stream/pacp/pacs.h"
#include "stack/ble/profile/audio/stream/basp/bass.h"

///////////////////////// Content Control /////////////////////////////////
//MCP
#include "stack/ble/profile/audio/content/mcp/ots/ots.h" //MCP-OTS
#include "stack/ble/profile/audio/content/mcp/mcs.h"
//CCP
#include "stack/ble/profile/audio/content/ccp/tbs.h"

////////////////// Transition and Coordination Control ////////////////////
//CSIS
#include "stack/ble/profile/audio/trans_coord/csip/csis.h"
//CAP
#include "stack/ble/profile/audio/trans_coord/cap/cap.h"

////////////////////// Rendering and Capture Control //////////////////////
//MICP
#include "stack/ble/profile/audio/render_cap/micp/mics.h"
//AICS
#include "stack/ble/profile/audio/render_cap/aicp/aics.h"
//VOCS
#include "stack/ble/profile/audio/render_cap/vocp/vocs.h"
//VCS
#include "stack/ble/profile/audio/render_cap/vcp/vcs.h"

//Use Case Specific Profiles
#include "stack/ble/profile/audio/user_case/ucp.h"


/*********************************************************/
//Remove when file merge to SDK //
#include "stack/ble/profile/audio/common/audio_internal.h"

#include "stack/ble/profile/audio/stream/ascp/ascs_internal.h"
#include "stack/ble/profile/audio/stream/pacp/pacs_internal.h"
#include "stack/ble/profile/audio/stream/basp/bass_internal.h"

#include "stack/ble/profile/audio/trans_coord/cap/cap_internal.h"
#include "stack/ble/profile/audio/trans_coord/csip/csis_internal.h"

#include "stack/ble/profile/audio/content/ccp/tbs_internal.h"
#include "stack/ble/profile/audio/content/mcp/ots/ots_internal.h"
#include "stack/ble/profile/audio/content/mcp/mcs_internal.h"

#include "stack/ble/profile/audio/render_cap/micp/mics_internal.h"
#include "stack/ble/profile/audio/render_cap/vcp/vcs_internal.h"
#include "stack/ble/profile/audio/render_cap/vocp/vocs_internal.h"
#include "stack/ble/profile/audio/render_cap/aicp/aics_internal.h"

#include "stack/ble/profile/audio/user_case/hap/has_internal.h"
#include "stack/ble/profile/audio/user_case/tmap/tmas_internal.h"
#include "stack/ble/profile/audio/user_case/ucp_internal.h"
