/********************************************************************************************************
 * @file    svc_mics.c
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

#define MICS_START_HDL                  SERVICE_MICROPHONE_CONTROL_HDL

_attribute_ble_data_retention_
u8 micsMuteValue = 0x00;
static const u16 micsMuteValueLen = 1;

extern const u16 aicsIncludeValue[APP_AUDIO_AICS_SERVER_MAX_INSTANCE_NUM][3];

/*
 * @brief the structure for default MICS service List.
 */
static const atts_attribute_t micsList[] =
{
    ATTS_PRIMARY_SERVICE(serviceMicrophoneControlUuid),

#if APP_AUDIO_MICS_INCLUDE_AICS_INSTANCE_NUM > 0
    ATTS_INCLUDE_DEFINE(&aicsIncludeValue[0][0]),
#endif
#if APP_AUDIO_MICS_INCLUDE_AICS_INSTANCE_NUM > 1
    ATTS_INCLUDE_DEFINE(&aicsIncludeValue[1][0]),
#endif
#if APP_AUDIO_MICS_INCLUDE_AICS_INSTANCE_NUM > 2
    ATTS_INCLUDE_DEFINE(&aicsIncludeValue[2][0]),
#endif
#if APP_AUDIO_MICS_INCLUDE_AICS_INSTANCE_NUM > 3
    ATTS_INCLUDE_DEFINE(&aicsIncludeValue[3][0]),
#endif

    //Mute
    ATTS_CHAR_UUID_ENCR_RDWR_ENTITY_WCB(charPropReadWriteNotify, characteristicMuteUuid, micsMuteValue),
    ATTS_COMMON_CCC_DEFINE,
};

/*
 * @brief the structure for default MICS service group.
 */
_attribute_ble_data_retention_
static atts_group_t svcMicsGroup =
{
    NULL,
    micsList,
    NULL,
    NULL,
    MICS_START_HDL,
    0,
};

/**
 * @brief      for user add default MICS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */

void blc_svc_addMicsGroup(void)
{
    svcMicsGroup.endHandle = svcMicsGroup.startHandle+ARRAY_SIZE(micsList)-1;
    blc_gatts_addAttributeServiceGroup(&svcMicsGroup);
}

/**
 * @brief      for user remove default MICS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeMicsGroup(void)
{
    blc_gatts_removeAttributeServiceGroup(MICS_START_HDL);
}

/**
 * @brief      for user register read or write attribute value callback function in MICS service.
 * @param[in]  readCback: read attribute value callback function pointer.
 * @param[in]  writeCback: write attribute value callback function pointer.
 * @return     none.
 */
void blc_svc_micsCbackRegister(atts_r_cb_t readCback, atts_w_cb_t writeCback)
{
    svcMicsGroup.readCback = readCback;
    svcMicsGroup.writeCback = writeCback;
}
