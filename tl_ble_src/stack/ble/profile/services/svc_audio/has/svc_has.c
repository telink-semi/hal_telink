/********************************************************************************************************
 * @file    svc_has.c
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

#ifndef LE_AUDIO_HAS_HEARING_AID_PRESET_CONTROL_POINT
    #define LE_AUDIO_HAS_HEARING_AID_PRESET_CONTROL_POINT 1
#endif

#define HAS_START_HDL SERVICE_HEARING_ACCESS_HDL

_attribute_ble_data_retention_
    u8           hasHearingAidFeaturesValue    = 0x00;
static const u16 hasHearingAidFeaturesValueLen = 1;

#if LE_AUDIO_HAS_HEARING_AID_PRESET_CONTROL_POINT
_attribute_ble_data_retention_
    u8           hasActivePresetIndexValue    = 0x00;
static const u16 hasActivePresetIndexValueLen = 1;
#endif

/*
 * @brief the structure for default HAS service List.
 */
static const atts_attribute_t hasList[] =
    {
        ATTS_PRIMARY_SERVICE(serviceHearingAccessUuid),

        //Hearing Aid Features
        ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropReadNotfiy, characteristicHearingAidFeaturesUuid, hasHearingAidFeaturesValue),
        ATTS_COMMON_CCC_DEFINE,

#if LE_AUDIO_HAS_HEARING_AID_PRESET_CONTROL_POINT
        //Hearing Aid Preset Control Point
        ATTS_CHAR_UUID_ENCR_WRITE_NULL(charPropWriteIndicate, characteristicHearingAidPresetControlPointUuid),
        ATTS_COMMON_CCC_DEFINE,

        //Active Preset Index
        ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropReadNotfiy, characteristicActivePresetIndexUuid, hasActivePresetIndexValue),
        ATTS_COMMON_CCC_DEFINE,
#endif
};

/*
 * @brief the structure for default HAS service group.
 */
_attribute_ble_data_retention_ static atts_group_t svcHasGroup =
    {
        NULL,
        hasList,
        NULL,
        NULL,
        HAS_START_HDL,
        0,
};

/**
 * @brief      for user add default HAS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addHasGroup(void)
{
    svcHasGroup.endHandle = svcHasGroup.startHandle + ARRAY_SIZE(hasList) - 1;
    blc_gatts_addAttributeServiceGroup(&svcHasGroup);
}

/**
 * @brief      for user remove default HAS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeHasGroup(void)
{
    blc_gatts_removeAttributeServiceGroup(HAS_START_HDL);
}

/**
 * @brief      for user register read or write attribute value callback function in HAS service.
 * @param[in]  readCback: read attribute value callback function pointer.
 * @param[in]  writeCback: write attribute value callback function pointer.
 * @return     none.
 */
void blc_svc_hasCbackRegister(atts_r_cb_t readCback, atts_w_cb_t writeCback)
{
    svcHasGroup.readCback  = readCback;
    svcHasGroup.writeCback = writeCback;
}
