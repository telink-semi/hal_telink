/********************************************************************************************************
 * @file    svc_vcs.c
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

#define VCS_START_HDL                   SERVICE_VOLUME_CONTROL_HDL

#define VCS_VOL_STATE_FIX_LEN                           3
#define VCS_VOL_FLAGS_FIX_LEN                           1

_attribute_ble_data_retention_
u8 vcsVolumeStateValue[VCS_VOL_STATE_FIX_LEN];
static const u16 vcsVolumeStateValueLen = VCS_VOL_STATE_FIX_LEN;

_attribute_ble_data_retention_
u8 vcsVolumeFlagsValue;
static const u16 vcsVolumeFlagsValueLen = VCS_VOL_FLAGS_FIX_LEN;

extern const u16 aicsIncludeValue[APP_AUDIO_AICS_SERVER_MAX_INSTANCE_NUM][3];
extern const u16 vocsIncludeValue[APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM][3];

/*
 * @brief the structure for default VCS service List.
 */
static const atts_attribute_t vcsList[] =
{
    ATTS_PRIMARY_SERVICE(serviceVolumeControlUuid),

#if APP_AUDIO_VCS_INCLUDE_AICS_INSTANCE_NUM > 0
    ATTS_INCLUDE_DEFINE(&aicsIncludeValue[0][0]),
#endif
#if APP_AUDIO_VCS_INCLUDE_AICS_INSTANCE_NUM > 1
    ATTS_INCLUDE_DEFINE(&aicsIncludeValue[1][0]),
#endif
#if APP_AUDIO_VCS_INCLUDE_AICS_INSTANCE_NUM > 2
    ATTS_INCLUDE_DEFINE(&aicsIncludeValue[2][0]),
#endif
#if APP_AUDIO_VCS_INCLUDE_AICS_INSTANCE_NUM > 3
    ATTS_INCLUDE_DEFINE(&aicsIncludeValue[3][0]),
#endif

#if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 0
    ATTS_INCLUDE_DEFINE(&vocsIncludeValue[0][0]),
#endif
#if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 1
    ATTS_INCLUDE_DEFINE(&vocsIncludeValue[1][0]),
#endif
#if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 2
    ATTS_INCLUDE_DEFINE(&vocsIncludeValue[2][0]),
#endif
#if APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM > 3
    ATTS_INCLUDE_DEFINE(&vocsIncludeValue[3][0]),
#endif

    //Volume State
    ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropReadNotfiy, characteristicVolumeStateUuid, vcsVolumeStateValue),
    ATTS_COMMON_CCC_DEFINE,

    //Volume Control Point
    ATTS_CHAR_UUID_ENCR_WRITE_NULL(charPropWrite, characteristicVolumeControlPointUuid),

    //Volume Flags
    ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropRead, characteristicVolumeFlagsUuid, vcsVolumeFlagsValue),
    ATTS_COMMON_CCC_DEFINE,
};

/*
 * @brief the structure for default VCS service group.
 */
_attribute_ble_data_retention_
static atts_group_t svcVcsGroup =
{
    NULL,
    vcsList,
    NULL,
    NULL,
    VCS_START_HDL,
    0,
};

/**
 * @brief      for user add default VCS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addVcsGroup(void)
{
    svcVcsGroup.endHandle = svcVcsGroup.startHandle+ARRAY_SIZE(vcsList)-1;
    blc_gatts_addAttributeServiceGroup(&svcVcsGroup);
}

/**
 * @brief      for user remove default VCS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeVcsGroup(void)
{
    blc_gatts_removeAttributeServiceGroup(VCS_START_HDL);
}

/**
 * @brief      for user register read or write attribute value callback function in VCS service.
 * @param[in]  readCback: read attribute value callback function pointer.
 * @param[in]  writeCback: write attribute value callback function pointer.
 * @return     none.
 */
void blc_svc_vcsCbackRegister(atts_r_cb_t readCback, atts_w_cb_t writeCback)
{
    svcVcsGroup.readCback = readCback;
    svcVcsGroup.writeCback = writeCback;
}
