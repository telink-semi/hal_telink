/********************************************************************************************************
 * @file    svc_cas.c
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

/*
 * There are no characteristics in CAS
 * There shall be no more than one CAS instance on a server.
 * The CAS shall include no more than one instance of CSIS
 */
#define CAS_START_HDL SERVICE_COMMON_AUDIO_HDL

extern const u16 csisIncludeValue[3];
/*
 * @brief the structure for default CAS service List.
 */
static const atts_attribute_t casList[] =
    {
        ATTS_PRIMARY_SERVICE(serviceCommonAudioUuid),
        ATTS_INCLUDE_DEFINE(csisIncludeValue),
};

/*
 * @brief the structure for default CAS service group.
 */
_attribute_ble_data_retention_ static atts_group_t svcCasGroup =
    {
        NULL,
        casList,
        NULL,
        NULL,
        CAS_START_HDL,
        0,
};

/**
 * @brief      for user add default CAS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addCasGroup(void)
{
    svcCasGroup.endHandle = svcCasGroup.startHandle + ARRAY_SIZE(casList) - 1;
    blc_gatts_addAttributeServiceGroup(&svcCasGroup);
}

/**
 * @brief      for user remove default CAS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeCasGroup(void)
{
    blc_gatts_removeAttributeServiceGroup(CAS_START_HDL);
}
