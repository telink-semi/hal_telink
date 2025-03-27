/********************************************************************************************************
 * @file    svc_scps.c
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

#ifndef SCPS_SCAN_REFRESH
#define SCPS_SCAN_REFRESH                   1
#endif

#define SCPS_START_HDL                      SERVICE_SCAN_PARAMETERS_HDL

/*
 * @brief the structure for default ScPS service List.
 */
static const atts_attribute_t scpsList[] =
{
    ATTS_PRIMARY_SERVICE(serviceScanParametersUuid),

    //Scan Interval Window
    ATTS_CHAR_UUID_WRITE_NULL(charPropWriteWithout, characteristicScanIntervalWindowUuid),

#if SCPS_SCAN_REFRESH
    //Scan Refresh
    ATTS_CHAR_UUID_NOTIF_ONLY(characteristicScanRefreshUuid),
    ATTS_COMMON_CCC_DEFINE,
#endif
};

/*
 * @brief the structure for default ScPS service group.
 */
_attribute_ble_data_retention_
static atts_group_t svcScpsGroup =
{
    NULL,
    scpsList,
    NULL,
    NULL,
    SCPS_START_HDL,
    0,
};

/**
 * @brief      for user add default ScPS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addScpsGroup(void)
{
    svcScpsGroup.endHandle = svcScpsGroup.startHandle+ARRAY_SIZE(scpsList)-1;
    blc_gatts_addAttributeServiceGroup(&svcScpsGroup);
}

/**
 * @brief      for user remove default ScPS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeScpsGroup(void)
{
    blc_gatts_removeAttributeServiceGroup(SCPS_START_HDL);
}

/**
 * @brief      for user register read or write attribute value callback function in ScPS service.
 * @param[in]  writeCback: write attribute value callback function pointer.
 * @return     none.
 */
void blc_svc_scpsCbackRegister(atts_w_cb_t writeCback)
{
    svcScpsGroup.writeCback = writeCback;
}
