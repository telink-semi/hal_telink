/********************************************************************************************************
 * @file    svc_vocs.c
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

#define VOCS_START_HDL            SERVICE_VOCS_IN_VCS_HDL

#define VOCS_OUTPUT_DESC_MAX_SIZE 50

const u16 gVocsOutDescMaxSize = VOCS_OUTPUT_DESC_MAX_SIZE;

#define VOCS_VOL_OFFSET_STATE_FIX_LEN 3
#define VOCS_AUDIO_LOCATION_FIX_LEN   4

#if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 0
_attribute_ble_data_retention_ static u8 vocsVolumeOffsetStateValue[APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM][VOCS_VOL_OFFSET_STATE_FIX_LEN] = {
    {0x00, 0x00, 0x00},
    #if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 1
    {0x00, 0x00, 0x00},
    #endif
    #if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 2
    {0x00, 0x00, 0x00},
    #endif
    #if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 3
    {0x00, 0x00, 0x00},
    #endif
};
static const u16 vocsVolumeOffsetStateValueLen = VOCS_VOL_OFFSET_STATE_FIX_LEN;

_attribute_ble_data_retention_ static u32 vocsAudioLocationValue[APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM] = {
    0x00,
    #if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 1
    0x00,
    #endif
    #if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 2
    0x00,
    #endif
    #if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 3
    0x00,
    #endif
};
static const u16 vocsAudioLocationValueLen = VOCS_AUDIO_LOCATION_FIX_LEN;

_attribute_ble_data_retention_ static u8 vocsAudioOutputDescriptionValue[APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM][VOCS_OUTPUT_DESC_MAX_SIZE];

_attribute_ble_data_retention_ static u16 vocsAudioOutputDescriptionValueLen[APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM];
#endif

#define ATTS_CHAR_VOL_OFFSET_STATE(value)                 \
    ATTS_CHARACTERISTIC_DECLARATIONS(charPropReadNotfiy), \
        ATTS_CHAR_UUID_DEFINE(ATT_PERMISSIONS_ENCRYPT_READ, characteristicVolumeOffsetStateUuid, vocsVolumeOffsetStateValueLen, VOCS_VOL_OFFSET_STATE_FIX_LEN, value, 0)

#define ATTS_CHAR_AUDIO_LOCATION(value)                               \
    ATTS_CHARACTERISTIC_DECLARATIONS(charPropReadWriteWithoutNotify), \
        ATTS_CHAR_UUID_DEFINE(ATT_PERMISSIONS_ENCRYPT_RDWR, characteristicAudioLocationUuid, vocsAudioLocationValueLen, VOCS_AUDIO_LOCATION_FIX_LEN, value, ATTS_SET_WRITE_CBACK | ATTS_SET_ALLOW_WRITE)

#define ATTS_CHAR_VOL_OFFSET_CTRL_POINT()                                                                                                             \
    ATTS_CHARACTERISTIC_DECLARATIONS(charPropWrite),                                                                                                  \
    {                                                                                                                                                 \
        ATT_PERMISSIONS_ENCRYPT_WRITE, ATT_16_UUID_LEN, (u8 *)(size_t)characteristicVolumeOffsetControlPointUuid, NULL, 0, NULL, ATTS_SET_WRITE_CBACK \
    }

#define ATTS_CHAR_AUDIO_OUTPUT_DESC(value, len)                       \
    ATTS_CHARACTERISTIC_DECLARATIONS(charPropReadWriteWithoutNotify), \
        ATTS_CHAR_UUID_DEFINE(ATT_PERMISSIONS_ENCRYPT_RDWR, characteristicAudioOutputDescriptionUuid, len, VOCS_OUTPUT_DESC_MAX_SIZE, value, ATTS_SET_WRITE_CBACK | ATTS_SET_ALLOW_WRITE | ATTS_SET_VARIABLE_LEN)

#if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 0
/*
 * @brief the structure for default VOCS service List.
 */
static const atts_attribute_t vocsList[] =
    {
        ATTS_SECONDARY_SERVICE(serviceVolumeOffsetControlUuid),

        //Volume Offset State
        ATTS_CHAR_VOL_OFFSET_STATE(&vocsVolumeOffsetStateValue[0][0]),
        ATTS_COMMON_CCC_DEFINE,

        //Audio Location
        ATTS_CHAR_AUDIO_LOCATION(&vocsAudioLocationValue[0]),
        ATTS_COMMON_CCC_DEFINE,

        //Volume Offset Control Point
        ATTS_CHAR_VOL_OFFSET_CTRL_POINT(),

        //Audio output description
        ATTS_CHAR_AUDIO_OUTPUT_DESC(&vocsAudioOutputDescriptionValue[0][0], vocsAudioOutputDescriptionValueLen[0]),
        ATTS_COMMON_CCC_DEFINE,

    #if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 1
        ATTS_SECONDARY_SERVICE(serviceVolumeOffsetControlUuid),

        //Volume Offset State
        ATTS_CHAR_VOL_OFFSET_STATE(&vocsVolumeOffsetStateValue[1][0]),
        ATTS_COMMON_CCC_DEFINE,

        //Audio Location
        ATTS_CHAR_AUDIO_LOCATION(&vocsAudioLocationValue[1]),
        ATTS_COMMON_CCC_DEFINE,

        //Volume Offset Control Point
        ATTS_CHAR_VOL_OFFSET_CTRL_POINT(),

        //Audio output description
        ATTS_CHAR_AUDIO_OUTPUT_DESC(&vocsAudioOutputDescriptionValue[1][0], vocsAudioOutputDescriptionValueLen[1]),
        ATTS_COMMON_CCC_DEFINE,
    #endif

    #if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 2
        ATTS_SECONDARY_SERVICE(serviceVolumeOffsetControlUuid),

        //Volume Offset State
        ATTS_CHAR_VOL_OFFSET_STATE(&vocsVolumeOffsetStateValue[2][0]),
        ATTS_COMMON_CCC_DEFINE,

        //Audio Location
        ATTS_CHAR_AUDIO_LOCATION(&vocsAudioLocationValue[2]),
        ATTS_COMMON_CCC_DEFINE,

        //Volume Offset Control Point
        ATTS_CHAR_VOL_OFFSET_CTRL_POINT(),

        //Audio output description
        ATTS_CHAR_AUDIO_OUTPUT_DESC(&vocsAudioOutputDescriptionValue[2][0], vocsAudioOutputDescriptionValueLen[2]),
        ATTS_COMMON_CCC_DEFINE,
    #endif

    #if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 3
        ATTS_SECONDARY_SERVICE(serviceVolumeOffsetControlUuid),

        //Volume Offset State
        ATTS_CHAR_VOL_OFFSET_STATE(&vocsVolumeOffsetStateValue[3][0]),
        ATTS_COMMON_CCC_DEFINE,

        //Audio Location
        ATTS_CHAR_AUDIO_LOCATION(&vocsAudioLocationValue[3]),
        ATTS_COMMON_CCC_DEFINE,

        //Volume Offset Control Point
        ATTS_CHAR_VOL_OFFSET_CTRL_POINT(),

        //Audio output description
        ATTS_CHAR_AUDIO_OUTPUT_DESC(&vocsAudioOutputDescriptionValue[3][0], vocsAudioOutputDescriptionValueLen[3]),
        ATTS_COMMON_CCC_DEFINE,
    #endif
};
#endif

#if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 0
/*
 * @brief the structure for default VOCS service group.
 */
_attribute_ble_data_retention_ static atts_group_t svcVocsGroup =
    {
        NULL,
        vocsList,
        NULL,
        NULL,
        VOCS_START_HDL,
        0,
};
#endif

#define VOCS_SVC_HDL_COUNT   ARRAY_SIZE(vocsList) / APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM //15

#define VOCS_START_HANDLE(n) (VOCS_START_HDL + (VOCS_SVC_HDL_COUNT) * (n))
#define VOCS_END_HANDLE(n)   (VOCS_START_HANDLE(n) + VOCS_SVC_HDL_COUNT - 1)

const u16 vocsIncludeValue[APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM][3] = {
#if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 0
    {VOCS_START_HANDLE(0), VOCS_END_HANDLE(0), SERVICE_UUID_VOLUME_OFFSET_CONTROL},
#endif
#if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 1
    {VOCS_START_HANDLE(1), VOCS_END_HANDLE(1), SERVICE_UUID_VOLUME_OFFSET_CONTROL},
#endif
#if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 2
    {VOCS_START_HANDLE(2), VOCS_END_HANDLE(2), SERVICE_UUID_VOLUME_OFFSET_CONTROL},
#endif
#if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 3
    {VOCS_START_HANDLE(3), VOCS_END_HANDLE(3), SERVICE_UUID_VOLUME_OFFSET_CONTROL},
#endif
};

void blc_svc_addVocsGroup(void)
{
#if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 0
    svcVocsGroup.endHandle = svcVocsGroup.startHandle + ARRAY_SIZE(vocsList) - 1;
    blc_gatts_addAttributeServiceGroup(&svcVocsGroup);
#endif
}

void blc_svc_removeVocsGroup(void)
{
#if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 0
    blc_gatts_removeAttributeServiceGroup(VOCS_START_HDL);
#endif
}

void blc_svc_vocsCbackRegister(atts_r_cb_t readCback, atts_w_cb_t writeCback)
{
#if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 0
    svcVocsGroup.readCback  = readCback;
    svcVocsGroup.writeCback = writeCback;
#else
    (void)readCback;
    (void)writeCback;
#endif
}
