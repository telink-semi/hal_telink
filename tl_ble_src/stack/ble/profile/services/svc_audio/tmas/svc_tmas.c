/********************************************************************************************************
 * @file    svc_tmas.c
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

#define TMAS_START_HDL SERVICE_TELEPHONE_AND_MEDIA_AUDIO_HFL

_attribute_ble_data_retention_ static u16 tmasTmapRoleValue    = DEFAULT_TMAP_ROLE;
static const u16                          tmasTmapRoleValueLen = sizeof(tmasTmapRoleValue);

/*
 * @brief the structure for default TMAS service List.
 */
static const atts_attribute_t tmasList[] =
    {
        ATTS_PRIMARY_SERVICE(serviceTelephonyAndMediaAudioUuid),

        //TMAP Role
        ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropRead, characteristicTmapRoleUuid, tmasTmapRoleValue),
};

/*
 * @brief the structure for default TMAS service group.
 */
_attribute_ble_data_retention_ static atts_group_t svcTmasGroup =
    {
        NULL,
        tmasList,
        NULL,
        NULL,
        TMAS_START_HDL,
        0,
};

/**
 * @brief      for user add default TMAS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addTmasGroup(void)
{
    svcTmasGroup.endHandle = svcTmasGroup.startHandle + ARRAY_SIZE(tmasList) - 1;
    blc_gatts_addAttributeServiceGroup(&svcTmasGroup);
}

/**
 * @brief      for user remove default TMAS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeTmasGroup(void)
{
    blc_gatts_removeAttributeServiceGroup(TMAS_START_HDL);
}
