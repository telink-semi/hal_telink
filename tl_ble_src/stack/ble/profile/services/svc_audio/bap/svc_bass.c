/********************************************************************************************************
 * @file    svc_bass.c
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

#define BASS_START_HDL                        SERVICE_BROADCAST_AUDIO_SCAN_HDL

#define BLC_AUDIO_BCST_RECV_STATE_BUFFER_SIZE 100

#if APP_AUDIO_BASS_SERVER_RECV_STATE_CNT > 0
_attribute_ble_data_retention_
    u8 bassRecvState1[BLC_AUDIO_BCST_RECV_STATE_BUFFER_SIZE];
_attribute_ble_data_retention_
    u16 bassRecvState1Len = 0;
#endif

#if APP_AUDIO_BASS_SERVER_RECV_STATE_CNT > 1
_attribute_ble_data_retention_
    u8 bassRecvState2[BLC_AUDIO_BCST_RECV_STATE_BUFFER_SIZE];
_attribute_ble_data_retention_
    u16 bassRecvState2Len = 0;
#endif

#if APP_AUDIO_BASS_SERVER_RECV_STATE_CNT > 2
_attribute_ble_data_retention_
    u8 bassRecvState3[BLC_AUDIO_BCST_RECV_STATE_BUFFER_SIZE];
_attribute_ble_data_retention_
    u16 bassRecvState3Len = 0;
#endif

#if APP_AUDIO_BASS_SERVER_RECV_STATE_CNT > 3
_attribute_ble_data_retention_
    u8 bassRecvState4[BLC_AUDIO_BCST_RECV_STATE_BUFFER_SIZE];
_attribute_ble_data_retention_
    u16 bassRecvState4Len = 0;
#endif

/*
 * @brief the structure for default BASS service List.
 */
static const atts_attribute_t bassList[] =
    {
        ATTS_PRIMARY_SERVICE(serviceBroadcastAudioScanUuid),

        //Broadcast Audio Scan Control Point
        ATTS_CHAR_UUID_ENCR_WRITE_NULL(charPropWriteWriteWithout, characteristicBasControlPointUuid),

#if APP_AUDIO_BASS_SERVER_RECV_STATE_CNT > 0
        ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropReadNotfiy, characteristicBroadcastReceiveStateUuid, bassRecvState1),
        ATTS_COMMON_CCC_DEFINE,
#endif

#if APP_AUDIO_BASS_SERVER_RECV_STATE_CNT > 1
        ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropReadNotfiy, characteristicBroadcastReceiveStateUuid, bassRecvState2),
        ATTS_COMMON_CCC_DEFINE,
#endif

#if APP_AUDIO_BASS_SERVER_RECV_STATE_CNT > 2
        ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropReadNotfiy, characteristicBroadcastReceiveStateUuid, bassRecvState3),
        ATTS_COMMON_CCC_DEFINE,
#endif

#if APP_AUDIO_BASS_SERVER_RECV_STATE_CNT > 3
        ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropReadNotfiy, characteristicBroadcastReceiveStateUuid, bassRecvState4),
        ATTS_COMMON_CCC_DEFINE,
#endif
};

/*
 * @brief the structure for default BASS service group.
 */
_attribute_ble_data_retention_ static atts_group_t svcBassGroup =
    {
        NULL,
        bassList,
        NULL,
        NULL,
        BASS_START_HDL,
        0,
};

/**
 * @brief      for user add default BASS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addBassGroup(void)
{
    svcBassGroup.endHandle = svcBassGroup.startHandle + ARRAY_SIZE(bassList) - 1;
    blc_gatts_addAttributeServiceGroup(&svcBassGroup);
}

/**
 * @brief      for user remove default BASS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeBassGroup(void)
{
    blc_gatts_removeAttributeServiceGroup(BASS_START_HDL);
}

/**
 * @brief      for user register read or write attribute value callback function in BASS service.
 * @param[in]  readCback: read attribute value callback function pointer.
 * @param[in]  writeCback: write attribute value callback function pointer.
 * @return     none.
 */
void blc_svc_bassCbackRegister(atts_r_cb_t readCback, atts_w_cb_t writeCback)
{
    svcBassGroup.readCback  = readCback;
    svcBassGroup.writeCback = writeCback;
}
