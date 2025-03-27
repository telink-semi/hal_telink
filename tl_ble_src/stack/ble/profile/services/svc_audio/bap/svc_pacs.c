/********************************************************************************************************
 * @file    svc_pacs.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "stack/ble/ble.h"

#ifndef LE_AUDIO_PACS_SINK_PAC
#define LE_AUDIO_PACS_SINK_PAC                      1
#endif

#ifndef LE_AUDIO_PACS_SINK_AUDIO_LOCATIONS
#define LE_AUDIO_PACS_SINK_AUDIO_LOCATIONS          1
#endif

#ifndef LE_AUDIO_PACS_SOURCE_PAC
#define LE_AUDIO_PACS_SOURCE_PAC                    1
#endif

#ifndef LE_AUDIO_PACS_SOURCE_AUDIO_LOCATIONS
#define LE_AUDIO_PACS_SOURCE_AUDIO_LOCATIONS        1
#endif

#define LE_AUDIO_PACS_PAC_MAX_SIZE                  512

#define PACS_START_HDL                          SERVICE_PUBLISHED_AUDIO_CAPABILITIES_HDL

#define AUDIO_LOCATION_FIX_SIZE                 4
#define AUDIO_CONTEXTS_FIX_SIZE                 4

const u16 gPacMaxSize = LE_AUDIO_PACS_PAC_MAX_SIZE;

#if LE_AUDIO_PACS_SINK_PAC
_attribute_ble_data_retention_
u8 pacsSinkPACValue[LE_AUDIO_PACS_PAC_MAX_SIZE];
_attribute_ble_data_retention_
u16 pacsSinkPACValueLen;
#endif

#if LE_AUDIO_PACS_SINK_AUDIO_LOCATIONS
_attribute_ble_data_retention_
u8 pacsSinkAudioLocationsValue[AUDIO_LOCATION_FIX_SIZE];
static const u16 pacsSinkAudioLocationsValueLen = AUDIO_LOCATION_FIX_SIZE;
#endif

#if LE_AUDIO_PACS_SOURCE_PAC
_attribute_ble_data_retention_
u8 pacsSourcePACValue[LE_AUDIO_PACS_PAC_MAX_SIZE];
_attribute_ble_data_retention_
u16 pacsSourcePACValueLen;
#endif

#if LE_AUDIO_PACS_SOURCE_AUDIO_LOCATIONS
_attribute_ble_data_retention_
u8 pacsSourceAudioLocationsValue[AUDIO_LOCATION_FIX_SIZE];
static const u16 pacsSourceAudioLocationsValueLen = AUDIO_LOCATION_FIX_SIZE;
#endif

_attribute_ble_data_retention_
u8 pacsAvailableAudioContextsValue[AUDIO_CONTEXTS_FIX_SIZE];
static const u16 pacsAvailableAudioContextsValueLen = AUDIO_CONTEXTS_FIX_SIZE;

_attribute_ble_data_retention_
u8 pacsSupportedAudioContextsValue[AUDIO_CONTEXTS_FIX_SIZE];
static const u16 pacsSupportedAudioContextsValueLen = AUDIO_CONTEXTS_FIX_SIZE;

/*
 * @brief the structure for default PACS service List.
 */
static const atts_attribute_t pacsList[] =
{
    ATTS_PRIMARY_SERVICE(servicePublishedAudioCapabilitiesUuid),
#if LE_AUDIO_PACS_SINK_PAC
    //Sink PAC
    ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropReadNotfiy, characteristicSinkPacUuid, pacsSinkPACValue),
    ATTS_COMMON_CCC_DEFINE,
#endif

#if LE_AUDIO_PACS_SINK_AUDIO_LOCATIONS
    //Sink Audio Locations
    ATTS_CHAR_UUID_ENCR_RDWR_POINT_WCB(charPropReadWriteNotify, characteristicSinkAudioLocationsUuid, pacsSinkAudioLocationsValue),
    ATTS_COMMON_CCC_DEFINE,
#endif

#if LE_AUDIO_PACS_SOURCE_PAC
    //Source PAC
    ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropReadNotfiy, characteristicSourcePacUuid, pacsSourcePACValue),
    ATTS_COMMON_CCC_DEFINE,
#endif

#if LE_AUDIO_PACS_SOURCE_AUDIO_LOCATIONS
    //Source Audio Locations
    ATTS_CHAR_UUID_ENCR_RDWR_POINT_WCB(charPropReadWriteNotify, characteristicSourceAudioLocationsUuid, pacsSourceAudioLocationsValue),
    ATTS_COMMON_CCC_DEFINE,
#endif

    //Available Audio Contexts
    ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropReadNotfiy, characteristicAvailableAudioContextsUuid, pacsAvailableAudioContextsValue),
    ATTS_COMMON_CCC_DEFINE,

    //Supported Audio Contexts
    ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropReadNotfiy, characteristicSupportedAudioContextsUuid, pacsSupportedAudioContextsValue),
    ATTS_COMMON_CCC_DEFINE,
};

/*
 * @brief the structure for default PACS service group.
 */
_attribute_ble_data_retention_
static atts_group_t svcPacsGroup =
{
    NULL,
    pacsList,
    NULL,
    NULL,
    PACS_START_HDL,
    0,
};

/**
 * @brief      for user add default PACS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addPacsGroup(void)
{
    svcPacsGroup.endHandle = svcPacsGroup.startHandle + ARRAY_SIZE(pacsList) - 1;
    blc_gatts_addAttributeServiceGroup(&svcPacsGroup);
}

/**
 * @brief      for user remove default PACS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removePacsGroup(void)
{
    blc_gatts_removeAttributeServiceGroup(PACS_START_HDL);
}

/**
 * @brief      for user register read or write attribute value callback function in PACS service.
 * @param[in]  readCback: read attribute value callback function pointer.
 * @param[in]  writeCback: write attribute value callback function pointer.
 * @return     none.
 */
void blc_svc_pacsCbackRegister(atts_r_cb_t readCback, atts_w_cb_t writeCback)
{
    svcPacsGroup.readCback = readCback;
    svcPacsGroup.writeCback = writeCback;
}

#if !(LE_AUDIO_PACS_SINK_PAC || LE_AUDIO_PACS_SOURCE_PAC)
#error "pacs:Mandatory to support at least one of the Sink PAC or Source PAC characteristic"
#endif

#if !LE_AUDIO_PACS_SINK_PAC && LE_AUDIO_PACS_SINK_AUDIO_LOCATIONS
#error "pacs:Optional to support if the Sink PAC characteristic is supported, otherwise Excluded"
#endif

#if !LE_AUDIO_PACS_SOURCE_PAC && LE_AUDIO_PACS_SOURCE_AUDIO_LOCATIONS
#error "pacs:Optional to support if the Source PAC characteristic is supported, otherwise Excluded"
#endif
