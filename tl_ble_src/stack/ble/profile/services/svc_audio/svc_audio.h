/********************************************************************************************************
 * @file    svc_audio.h
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

#include "../svc.h"

#ifdef __BOOT_SWITCH_APP1__
//Bluetooth LE Audio
//CSIP-CSIS
#define SERVICE_COORDINATED_SET_IDENTIFICATION_HDL      25//SERVICE_LE_AUDIO_START_HDL
#define CSIS_MAX_HDL_NUM                                12//0x10
//AICS
#define SERVICE_AUDIO_INPUT_CONTROL_HDL                 SERVICE_COORDINATED_SET_IDENTIFICATION_HDL + CSIS_MAX_HDL_NUM
#define AICS_MAX_HDL_NUM                                0//0x50
//VCS
#define SERVICE_VOLUME_CONTROL_HDL                      SERVICE_AUDIO_INPUT_CONTROL_HDL + AICS_MAX_HDL_NUM
#define VCS_MAX_HDL_NUM                                 9//0x20
#define SERVICE_VOCS_IN_VCS_HDL                         SERVICE_VOLUME_CONTROL_HDL + VCS_MAX_HDL_NUM
#define VOCS_IN_VCS_MAX_HDL_NUM                         0//0x40
//MCP-MCS
#define SERVICE_MEDIA_CONTROL_HDL                       SERVICE_VOCS_IN_VCS_HDL + VOCS_IN_VCS_MAX_HDL_NUM
#define MCS_MAX_HDL_NUM                                 0//0x40
//GMCS
#define SERVICE_GENERIC_MEDIA_CONTROL_HDL               SERVICE_MEDIA_CONTROL_HDL + MCS_MAX_HDL_NUM
#define GMCS_MAX_HDL_NUM                                0//0x40
//OTS
#define SERVICE_OBJECT_TRANSFER_HDL                     SERVICE_GENERIC_MEDIA_CONTROL_HDL + GMCS_MAX_HDL_NUM
#define OTS_MAX_HDL_NUM                                 0//0x20
//CCP-TBS
#define SERVICE_TELEPHONE_BEARER_HDL                    SERVICE_OBJECT_TRANSFER_HDL + OTS_MAX_HDL_NUM
#define TBS_MAX_HDL_NUM                                 0//0x40
//MICP-MICS
#define SERVICE_MICROPHONE_CONTROL_HDL                  SERVICE_TELEPHONE_BEARER_HDL + TBS_MAX_HDL_NUM
#define MICS_MAX_HDL_NUM                                4//0x10
//ASCS
#define SERVICE_AUDIO_STREAM_CONTROL_HDL                SERVICE_MICROPHONE_CONTROL_HDL + MICS_MAX_HDL_NUM
#define ASCS_MAX_HDL_NUM                                10//0x40
//BASS
#define SERVICE_BROADCAST_AUDIO_SCAN_HDL                SERVICE_AUDIO_STREAM_CONTROL_HDL + ASCS_MAX_HDL_NUM
#define BASS_MAX_HDL_NUM                                0//0x20
//PACS
#define SERVICE_PUBLISHED_AUDIO_CAPABILITIES_HDL        SERVICE_BROADCAST_AUDIO_SCAN_HDL + BASS_MAX_HDL_NUM
#define PACS_MAX_HDL_NUM                                19//0x20
//CAP-CAS
#define SERVICE_COMMON_AUDIO_HDL                        SERVICE_PUBLISHED_AUDIO_CAPABILITIES_HDL + PACS_MAX_HDL_NUM
#define CAS_MAX_HDL_NUM                                 2//0x10
//HAP-HAS
#define SERVICE_HEARING_ACCESS_HDL                      SERVICE_COMMON_AUDIO_HDL + CAS_MAX_HDL_NUM
#define HAS_MAX_HDL_NUM                                 0//0x10
//TMAP-TMAS
#define SERVICE_TELEPHONE_AND_MEDIA_AUDIO_HFL           SERVICE_HEARING_ACCESS_HDL + HAS_MAX_HDL_NUM
#define TAMS_MAX_HDL_NUM                                3//0x10

//Constant Tone Extension
#define SERVICE_CONSTANT_TONE_EXTENSION_HDL             SERVICE_TELEPHONE_AND_MEDIA_AUDIO_HFL + TAMS_MAX_HDL_NUM
#define CTES_MAX_HDL_NUM                                0x00
//CCP-GTBS
#define SERVICE_GENERIC_TELEPHONE_BEARER_HDL            SERVICE_CONSTANT_TONE_EXTENSION_HDL + CTES_MAX_HDL_NUM
#define GTBS_MAX_HDL_NUM                                0x40
//Device time
#define SERVICE_DEVICE_TIME_HDL                         SERVICE_GENERIC_TELEPHONE_BEARER_HDL + GTBS_MAX_HDL_NUM
#define DTS_MAX_HDL_NUM                                 0x00
#else
//Bluetooth LE Audio
//CSIP-CSIS
#define SERVICE_COORDINATED_SET_IDENTIFICATION_HDL      SERVICE_LE_AUDIO_START_HDL
#define CSIS_MAX_HDL_NUM                                0x10
//AICS
#define SERVICE_AUDIO_INPUT_CONTROL_HDL                 SERVICE_COORDINATED_SET_IDENTIFICATION_HDL + CSIS_MAX_HDL_NUM
#define AICS_MAX_HDL_NUM                                0x50
//VCS
#define SERVICE_VOLUME_CONTROL_HDL                      SERVICE_AUDIO_INPUT_CONTROL_HDL + AICS_MAX_HDL_NUM
#define VCS_MAX_HDL_NUM                                 0x20
#define SERVICE_VOCS_IN_VCS_HDL                         SERVICE_VOLUME_CONTROL_HDL + VCS_MAX_HDL_NUM
#define VOCS_IN_VCS_MAX_HDL_NUM                         0x40
//MCP-MCS
#define SERVICE_MEDIA_CONTROL_HDL                       SERVICE_VOCS_IN_VCS_HDL + VOCS_IN_VCS_MAX_HDL_NUM
#define MCS_MAX_HDL_NUM                                 0x40
//GMCS
#define SERVICE_GENERIC_MEDIA_CONTROL_HDL               SERVICE_MEDIA_CONTROL_HDL + MCS_MAX_HDL_NUM
#define GMCS_MAX_HDL_NUM                                0x40
//OTS
#define SERVICE_OBJECT_TRANSFER_HDL                     SERVICE_GENERIC_MEDIA_CONTROL_HDL + GMCS_MAX_HDL_NUM
#define OTS_MAX_HDL_NUM                                 0x20
//CCP-TBS
#define SERVICE_TELEPHONE_BEARER_HDL                    SERVICE_OBJECT_TRANSFER_HDL + OTS_MAX_HDL_NUM
#define TBS_MAX_HDL_NUM                                 0x40
//MICP-MICS
#define SERVICE_MICROPHONE_CONTROL_HDL                  SERVICE_TELEPHONE_BEARER_HDL + TBS_MAX_HDL_NUM
#define MICS_MAX_HDL_NUM                                0x10
//ASCS
#define SERVICE_AUDIO_STREAM_CONTROL_HDL                SERVICE_MICROPHONE_CONTROL_HDL + MICS_MAX_HDL_NUM
#define ASCS_MAX_HDL_NUM                                0x40
//BASS
#define SERVICE_BROADCAST_AUDIO_SCAN_HDL                SERVICE_AUDIO_STREAM_CONTROL_HDL + ASCS_MAX_HDL_NUM
#define BASS_MAX_HDL_NUM                                0x20
//PACS
#define SERVICE_PUBLISHED_AUDIO_CAPABILITIES_HDL        SERVICE_BROADCAST_AUDIO_SCAN_HDL + BASS_MAX_HDL_NUM
#define PACS_MAX_HDL_NUM                                0x20
//CAP-CAS
#define SERVICE_COMMON_AUDIO_HDL                        SERVICE_PUBLISHED_AUDIO_CAPABILITIES_HDL + PACS_MAX_HDL_NUM
#define CAS_MAX_HDL_NUM                                 0x10
//HAP-HAS
#define SERVICE_HEARING_ACCESS_HDL                      SERVICE_COMMON_AUDIO_HDL + CAS_MAX_HDL_NUM
#define HAS_MAX_HDL_NUM                                 0x10
//TMAP-TMAS
#define SERVICE_TELEPHONE_AND_MEDIA_AUDIO_HFL           SERVICE_HEARING_ACCESS_HDL + HAS_MAX_HDL_NUM
#define TAMS_MAX_HDL_NUM                                0x10

//Constant Tone Extension
#define SERVICE_CONSTANT_TONE_EXTENSION_HDL             SERVICE_TELEPHONE_AND_MEDIA_AUDIO_HFL + TAMS_MAX_HDL_NUM
#define CTES_MAX_HDL_NUM                                0x00
//CCP-GTBS
#define SERVICE_GENERIC_TELEPHONE_BEARER_HDL            SERVICE_CONSTANT_TONE_EXTENSION_HDL + CTES_MAX_HDL_NUM
#define GTBS_MAX_HDL_NUM                                0x40
//Device time
#define SERVICE_DEVICE_TIME_HDL                         SERVICE_GENERIC_TELEPHONE_BEARER_HDL + GTBS_MAX_HDL_NUM
#define DTS_MAX_HDL_NUM                                 0x00
#endif

#include "aics/svc_aics.h"
#include "bap/svc_ascs.h"
#include "bap/svc_bass.h"
#include "bap/svc_pacs.h"
#include "cas/svc_cas.h"
#include "csis/svc_csis.h"
#include "has/svc_has.h"
#include "mcs/svc_gmcs.h"
#include "mcs/svc_mcs.h"
#include "mcs/svc_ots.h"
#include "micp/svc_micp.h"
#include "micp/svc_mics.h"
#include "tbs/svc_gtbs.h"
#include "tbs/svc_tbs.h"
#include "tmas/svc_tmas.h"
#include "vcp/svc_vcp.h"
#include "vcp/svc_vcs.h"
#include "vcp/svc_vocs.h"
