/********************************************************************************************************
 * @file    svc_aics.c
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

#define AICS_START_HDL           SERVICE_AUDIO_INPUT_CONTROL_HDL

#define AICS_INPUT_DESC_MAX_SIZE 50

const u16 gAicsInDescMaxSize = AICS_INPUT_DESC_MAX_SIZE;

#define AICS_AUDIO_IN_STATE_FIX_LEN      4
#define AICS_GAIN_SET_PROPERTIES_FIX_LEN 3
#define AICS_AUDIO_IN_TYPE_FIX_LEN       1
#define AICS_AUDIO_IN_STATUS_FIX_LEN     1

#if APP_AUDIO_AICS_SERVER_MAX_INSTANCE_NUM > 0
static const u16 aicsAudioInputStateValueLen       = AICS_AUDIO_IN_STATE_FIX_LEN;
static const u16 aicsGainSettingPropertiesValueLen = AICS_GAIN_SET_PROPERTIES_FIX_LEN;
static const u16 aicsAudioInputTypeValueLen        = AICS_AUDIO_IN_TYPE_FIX_LEN;
static const u16 aicsAudioInputStatusValueLen      = AICS_AUDIO_IN_STATUS_FIX_LEN;
#endif

#if APP_AUDIO_AICS_SERVER_MAX_INSTANCE_NUM > 0
_attribute_ble_data_retention_
    u8 aicsAudioInputStateValue[APP_AUDIO_AICS_SERVER_MAX_INSTANCE_NUM][AICS_AUDIO_IN_STATE_FIX_LEN];
_attribute_ble_data_retention_
    u8 aicsGainSettingPropertiesValue[APP_AUDIO_AICS_SERVER_MAX_INSTANCE_NUM][AICS_GAIN_SET_PROPERTIES_FIX_LEN];
_attribute_ble_data_retention_
    u8 aicsAudioInputTypeValue[APP_AUDIO_AICS_SERVER_MAX_INSTANCE_NUM];
_attribute_ble_data_retention_
    u8 aicsAudioInputStatusValue[APP_AUDIO_AICS_SERVER_MAX_INSTANCE_NUM];
_attribute_ble_data_retention_
    u8 aicsAudioInputDescriptionValue[APP_AUDIO_AICS_SERVER_MAX_INSTANCE_NUM][AICS_INPUT_DESC_MAX_SIZE];
_attribute_ble_data_retention_
    u16 aicsAudioInputDescriptionValueLen[APP_AUDIO_AICS_SERVER_MAX_INSTANCE_NUM];
#endif

#define ATTS_CHAR_AUDIO_INPUT_STATE(value)                \
    ATTS_CHARACTERISTIC_DECLARATIONS(charPropReadNotfiy), \
        ATTS_CHAR_UUID_DEFINE(ATT_PERMISSIONS_ENCRYPT_READ, characteristicAudioInputStateUuid, aicsAudioInputStateValueLen, AICS_AUDIO_IN_STATE_FIX_LEN, value, 0)

#define ATTS_CHAR_GAIN_SETTING(value)               \
    ATTS_CHARACTERISTIC_DECLARATIONS(charPropRead), \
        ATTS_CHAR_UUID_DEFINE(ATT_PERMISSIONS_ENCRYPT_READ, characteristicGainSettingsAttributeUuid, aicsGainSettingPropertiesValueLen, AICS_GAIN_SET_PROPERTIES_FIX_LEN, value, 0)

#define ATTS_CHAR_AUDIO_INPUT_TYPE(value)           \
    ATTS_CHARACTERISTIC_DECLARATIONS(charPropRead), \
        ATTS_CHAR_UUID_DEFINE(ATT_PERMISSIONS_ENCRYPT_READ, characteristicAudioInputTypeUuid, aicsAudioInputTypeValueLen, AICS_AUDIO_IN_TYPE_FIX_LEN, value, 0)

#define ATTS_CHAR_AUDIO_INPUT_STATUS(value)               \
    ATTS_CHARACTERISTIC_DECLARATIONS(charPropReadNotfiy), \
        ATTS_CHAR_UUID_DEFINE(ATT_PERMISSIONS_ENCRYPT_READ, characteristicAudioInputStatusUuid, aicsAudioInputStatusValueLen, AICS_AUDIO_IN_STATUS_FIX_LEN, value, 0)

#define ATTS_CHAR_AUDIO_INPUT_CTRL_POINT()                                                                                                          \
    ATTS_CHARACTERISTIC_DECLARATIONS(charPropWrite),                                                                                                \
    {                                                                                                                                               \
        ATT_PERMISSIONS_ENCRYPT_WRITE, ATT_16_UUID_LEN, (u8 *)(size_t)characteristicAudioInputControlPointUuid, NULL, 0, NULL, ATTS_SET_WRITE_CBACK \
    }

#define ATTS_CHAR_AUDIO_INPUT_DESC(value, len)                        \
    ATTS_CHARACTERISTIC_DECLARATIONS(charPropReadWriteWithoutNotify), \
        ATTS_CHAR_UUID_DEFINE(ATT_PERMISSIONS_ENCRYPT_RDWR, characteristicAudioInputDescriptionUuid, len, AICS_INPUT_DESC_MAX_SIZE, value, ATTS_SET_WRITE_CBACK | ATTS_SET_ALLOW_WRITE | ATTS_SET_VARIABLE_LEN)

#if APP_AUDIO_AICS_SERVER_MAX_INSTANCE_NUM > 0
/*
 * @brief the structure for default AICS service List.
 */
static const atts_attribute_t aicsList[] =
    {
        ATTS_SECONDARY_SERVICE(serviceAudioInputControlUuid),

        //Audio Input State
        ATTS_CHAR_AUDIO_INPUT_STATE(&aicsAudioInputStateValue[0][0]),
        ATTS_COMMON_CCC_DEFINE,

        //Gain Setting Properties
        ATTS_CHAR_GAIN_SETTING(&aicsGainSettingPropertiesValue[0][0]),

        //Audio Input Type
        ATTS_CHAR_AUDIO_INPUT_TYPE(&aicsAudioInputTypeValue[0]),

        //Audio Input Status
        ATTS_CHAR_AUDIO_INPUT_STATUS(&aicsAudioInputStatusValue[0]),
        ATTS_COMMON_CCC_DEFINE,

        //Audio Input Control Point
        ATTS_CHAR_AUDIO_INPUT_CTRL_POINT(),

        //Audio Input Description
        ATTS_CHAR_AUDIO_INPUT_DESC(&aicsAudioInputDescriptionValue[0][0], aicsAudioInputDescriptionValueLen[0]),
        ATTS_COMMON_CCC_DEFINE,

    #if APP_AUDIO_AICS_SERVER_MAX_INSTANCE_NUM > 1
        ATTS_SECONDARY_SERVICE(serviceAudioInputControlUuid),

        //Audio Input State
        ATTS_CHAR_AUDIO_INPUT_STATE(&aicsAudioInputStateValue[1][0]),
        ATTS_COMMON_CCC_DEFINE,

        //Gain Setting Properties
        ATTS_CHAR_GAIN_SETTING(&aicsGainSettingPropertiesValue[1][0]),

        //Audio Input Type
        ATTS_CHAR_AUDIO_INPUT_TYPE(&aicsAudioInputTypeValue[1]),

        //Audio Input Status
        ATTS_CHAR_AUDIO_INPUT_STATUS(&aicsAudioInputStatusValue[1]),
        ATTS_COMMON_CCC_DEFINE,

        //Audio Input Control Point
        ATTS_CHAR_AUDIO_INPUT_CTRL_POINT(),

        //Audio Input Description
        ATTS_CHAR_AUDIO_INPUT_DESC(&aicsAudioInputDescriptionValue[1][0], aicsAudioInputDescriptionValueLen[1]),
        ATTS_COMMON_CCC_DEFINE,
    #endif

    #if APP_AUDIO_AICS_SERVER_MAX_INSTANCE_NUM > 2
        ATTS_SECONDARY_SERVICE(serviceAudioInputControlUuid),

        //Audio Input State
        ATTS_CHAR_AUDIO_INPUT_STATE(&aicsAudioInputStateValue[2][0]),
        ATTS_COMMON_CCC_DEFINE,

        //Gain Setting Properties
        ATTS_CHAR_GAIN_SETTING(&aicsGainSettingPropertiesValue[2][0]),

        //Audio Input Type
        ATTS_CHAR_AUDIO_INPUT_TYPE(&aicsAudioInputTypeValue[2]),

        //Audio Input Status
        ATTS_CHAR_AUDIO_INPUT_STATUS(&aicsAudioInputStatusValue[2]),
        ATTS_COMMON_CCC_DEFINE,

        //Audio Input Control Point
        ATTS_CHAR_AUDIO_INPUT_CTRL_POINT(),

        //Audio Input Description
        ATTS_CHAR_AUDIO_INPUT_DESC(&aicsAudioInputDescriptionValue[2][0], aicsAudioInputDescriptionValueLen[2]),
        ATTS_COMMON_CCC_DEFINE,
    #endif

    #if APP_AUDIO_AICS_SERVER_MAX_INSTANCE_NUM > 3
        ATTS_SECONDARY_SERVICE(serviceAudioInputControlUuid),

        //Audio Input State
        ATTS_CHAR_AUDIO_INPUT_STATE(&aicsAudioInputStateValue[3][0]),
        ATTS_COMMON_CCC_DEFINE,

        //Gain Setting Properties
        ATTS_CHAR_GAIN_SETTING(&aicsGainSettingPropertiesValue[3][0]),

        //Audio Input Type
        ATTS_CHAR_AUDIO_INPUT_TYPE(&aicsAudioInputTypeValue[3]),

        //Audio Input Status
        ATTS_CHAR_AUDIO_INPUT_STATUS(&aicsAudioInputStatusValue[3]),
        ATTS_COMMON_CCC_DEFINE,

        //Audio Input Control Point
        ATTS_CHAR_AUDIO_INPUT_CTRL_POINT(),

        //Audio Input Description
        ATTS_CHAR_AUDIO_INPUT_DESC(&aicsAudioInputDescriptionValue[3][0], aicsAudioInputDescriptionValueLen[3]),
        ATTS_COMMON_CCC_DEFINE,
    #endif
};
#endif

#if APP_AUDIO_AICS_SERVER_MAX_INSTANCE_NUM > 0
/*
 * @brief the structure for default AICS service group.
 */
_attribute_ble_data_retention_ static atts_group_t svcAicsGroup =
    {
        NULL,
        aicsList,
        NULL,
        NULL,
        AICS_START_HDL,
        0,
};
#endif

#define AICS_SVC_HDL_COUNT   ARRAY_SIZE(aicsList) / APP_AUDIO_AICS_SERVER_MAX_INSTANCE_NUM //15

#define AICS_START_HANDLE(n) (AICS_START_HDL + (AICS_SVC_HDL_COUNT) * (n))
#define AICS_END_HANDLE(n)   (AICS_START_HANDLE(n) + AICS_SVC_HDL_COUNT - 1)

const u16 aicsIncludeValue[APP_AUDIO_AICS_SERVER_MAX_INSTANCE_NUM][3] = {
#if APP_AUDIO_AICS_SERVER_MAX_INSTANCE_NUM > 0
    {AICS_START_HANDLE(0), AICS_END_HANDLE(0), SERVICE_UUID_AUDIO_INPUT_CONTROL},
#endif
#if APP_AUDIO_AICS_SERVER_MAX_INSTANCE_NUM > 1
    {AICS_START_HANDLE(1), AICS_END_HANDLE(1), SERVICE_UUID_AUDIO_INPUT_CONTROL},
#endif
#if APP_AUDIO_AICS_SERVER_MAX_INSTANCE_NUM > 2
    {AICS_START_HANDLE(2), AICS_END_HANDLE(2), SERVICE_UUID_AUDIO_INPUT_CONTROL},
#endif
#if APP_AUDIO_AICS_SERVER_MAX_INSTANCE_NUM > 3
    {AICS_START_HANDLE(3), AICS_END_HANDLE(3), SERVICE_UUID_AUDIO_INPUT_CONTROL},
#endif
};


/**
 * @brief      for user add default AICS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addAicsGroup(void)
{
#if APP_AUDIO_VCS_INCLUDE_AICS_INSTANCE_NUM > 0
    svcAicsGroup.endHandle = svcAicsGroup.startHandle + ARRAY_SIZE(aicsList) - 1;
    blc_gatts_addAttributeServiceGroup(&svcAicsGroup);
#endif
}

/**
 * @brief      for user remove default AICS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeGroup(void)
{
#if APP_AUDIO_VCS_INCLUDE_AICS_INSTANCE_NUM > 0
    blc_gatts_removeAttributeServiceGroup(AICS_START_HDL);
#endif
}

/**
 * @brief      for user register read or write attribute value callback function in AICS service.
 * @param[in]  readCback: read attribute value callback function pointer.
 * @param[in]  writeCback: write attribute value callback function pointer.
 * @return     none.
 */
void blc_svc_aicsCbackRegister(atts_r_cb_t readCback, atts_w_cb_t writeCback)
{
#if APP_AUDIO_VCS_INCLUDE_AICS_INSTANCE_NUM > 0
    svcAicsGroup.readCback  = readCback;
    svcAicsGroup.writeCback = writeCback;
#else
    (void)readCback;
    (void)writeCback;
#endif
}
