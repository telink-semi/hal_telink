/********************************************************************************************************
 * @file    svc_ull_hid.c
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

#define ULL_HID_START_HDL                  SERVICE_ULTRA_LOW_LATENCY_HID_HDL

#define ULL_HID_PROPERTIES_FORMAT_MAX_SIZE 21

_attribute_ble_data_retention_ static u8  ullhidProperties[ULL_HID_PROPERTIES_FORMAT_MAX_SIZE];
_attribute_ble_data_retention_ static u16 ullhidPropertiesLen = 0;

/*
 * @brief the structure for default ULL-HID service List.
 */
static const atts_attribute_t ullhidList[] =
    {
        ATTS_PRIMARY_SERVICE(serviceUllhidUuid),

        //ULL HID Properties
        ATTS_CHAR_UUID_READ_POINT_NOCB(charPropRead, characteristicUllHidPropertiesUuid, ullhidProperties),

        //LE HID Operation
        ATTS_CHAR_UUID_WRITE_NULL(charPropWriteIndicate, characteristicLeHidOperationModeUuid),
        ATTS_COMMON_CCC_DEFINE,
};

/*
 * @brief the structure for default ULL-HID service group.
 */
_attribute_ble_data_retention_ static atts_group_t svcUllhidGroup =
    {
        NULL,
        ullhidList,
        NULL,
        NULL,
        ULL_HID_START_HDL,
        0,
};

/**
 * @brief      for user add default ULL-HID service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addUllhidGroup(void)
{
    svcUllhidGroup.endHandle = svcUllhidGroup.startHandle + ARRAY_SIZE(ullhidList) - 1;
    blc_gatts_addAttributeServiceGroup(&svcUllhidGroup);
}

/**
 * @brief      for user remove default ULL-HID service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeUllhidGroup(void)
{
    blc_gatts_removeAttributeServiceGroup(ULL_HID_START_HDL);
}

/**
 * @brief      for user register read or write attribute value callback function in ULL-HID service.
 * @param[in]  writeCback: write attribute value callback function pointer.
 * @return     none.
 */
void blc_svc_ullhidCbackRegister(atts_w_cb_t writeCback)
{
    svcUllhidGroup.writeCback = writeCback;
}
