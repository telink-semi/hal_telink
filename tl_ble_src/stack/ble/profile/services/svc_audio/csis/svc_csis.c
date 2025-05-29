/********************************************************************************************************
 * @file    svc_csis.c
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

#ifndef LE_AUDIO_CSIS_COORDINATED_SET_SIZE
    #define LE_AUDIO_CSIS_COORDINATED_SET_SIZE 1
#endif

#ifndef LE_AUDIO_CSIS_SET_MEMBER_LOCK
    #define LE_AUDIO_CSIS_SET_MEMBER_LOCK 1
#endif

#ifndef LE_AUDIO_CSIS_SET_MEMBER_RANK
    #define LE_AUDIO_CSIS_SET_MEMBER_RANK 1
#endif

#define CSIS_START_HDL SERVICE_COORDINATED_SET_IDENTIFICATION_HDL

_attribute_ble_data_retention_
    svc_csis_SIRK_t csisSIRKValue;
static const u16    csisSIRKValueLen = sizeof(svc_csis_SIRK_t);

#if LE_AUDIO_CSIS_COORDINATED_SET_SIZE
_attribute_ble_data_retention_
    u8           csisCSSizeValue;
static const u16 csisCSSizeValueLen = 1;
#endif

#if LE_AUDIO_CSIS_SET_MEMBER_LOCK
_attribute_ble_data_retention_
    u8           csisSetMemberLockValue;
static const u16 csisSetMemberLockValueLen = 1;
#endif

#if LE_AUDIO_CSIS_SET_MEMBER_RANK
_attribute_ble_data_retention_
    u8           csisSetMemberRankValue;
static const u16 csisSetMemberRankValueLen = 1;
#endif

/*
 * @brief the structure for default CSIS service List.
 */
static const atts_attribute_t csisList[] =
    {
        ATTS_PRIMARY_SERVICE(serviceCoordinatedSetIdentificationUuid),

        //Set Identity Resolving Key
        ATTS_CHAR_UUID_ENCR_READ_ENTITY_CB(charPropReadNotfiy, characteristicSetIdentityResolvingKeyUuid, csisSIRKValue),
        ATTS_COMMON_CCC_DEFINE,

#if LE_AUDIO_CSIS_COORDINATED_SET_SIZE
        //Coordinated Set Size
        ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropReadNotfiy, characteristicCoordinatedSetSizeUuid, csisCSSizeValue),
        ATTS_COMMON_CCC_DEFINE,
#endif

#if LE_AUDIO_CSIS_SET_MEMBER_LOCK
        //Set Member Lock
        ATTS_CHAR_UUID_ENCR_RDWR_ENTITY_WCB(charPropReadWriteNotify, characteristicSetMemberLockUuid, csisSetMemberLockValue),
        ATTS_COMMON_CCC_DEFINE,
#endif

#if LE_AUDIO_CSIS_SET_MEMBER_RANK
        //Set Member Rank
        ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropReadNotfiy, characteristicSetMemberRankUuid, csisSetMemberRankValue),
#endif
};

/*
 * @brief the structure for default CSIS service group.
 */
_attribute_ble_data_retention_ static atts_group_t svcCsisGroup =
    {
        NULL,
        csisList,
        NULL,
        NULL,
        SERVICE_COORDINATED_SET_IDENTIFICATION_HDL,
        0,
};

const u16 csisIncludeValue[3] = {CSIS_START_HDL, CSIS_START_HDL + ARRAY_SIZE(csisList) - 1, SERVICE_UUID_COORDINATED_SET_IDENTIFICATION};

/**
 * @brief      for user add default CSIS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addCsisGroup(void)
{
    svcCsisGroup.endHandle = svcCsisGroup.startHandle + ARRAY_SIZE(csisList) - 1;
    blc_gatts_addAttributeServiceGroup(&svcCsisGroup);
}

/**
 * @brief      for user remove default CSIS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeCsisGroup(void)
{
    blc_gatts_removeAttributeServiceGroup(SERVICE_COORDINATED_SET_IDENTIFICATION_HDL);
}

/**
 * @brief      for user register read or write attribute value callback function in CSIS service.
 * @param[in]  readCback: read attribute value callback function pointer.
 * @param[in]  writeCback: write attribute value callback function pointer.
 * @return     none.
 */
void blc_svc_csisCbackRegister(atts_r_cb_t readCback, atts_w_cb_t writeCback)
{
    svcCsisGroup.readCback  = readCback;
    svcCsisGroup.writeCback = writeCback;
}

#if LE_AUDIO_CSIS_SET_MEMBER_LOCK && (!LE_AUDIO_CSIS_SET_MEMBER_RANK)
    #error "ERR:CSIS attribute table fail"
#endif
