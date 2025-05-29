/********************************************************************************************************
 * @file    svc_ras.c
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
#include "svc_spp.h"

#define SPP_START_HDL SERVICE_TELINK_SPP_HDL

static const u8 serviceSppUuid[16] = {TELINK_SPP_UUID_SERVICE};

_attribute_ble_data_retention_ u8  sppInData[100];
_attribute_ble_data_retention_ u16 sppInDataLen = 0;

static const u8 sppInUuid[16] = {TELINK_SPP_DATA_CLIENT2SERVER};

_attribute_ble_data_retention_ u8  sppOutData[256];
_attribute_ble_data_retention_ u16 sppOutDataLen = 0;

static const atts_attribute_t sppList[] =
    {
        ATTS_PRIMARY_SERVICE_128(serviceSppUuid),
        ATTS_CHARACTERISTIC_DECLARATIONS(charPropReadWriteWithoutNotify),
        {ATT_PERMISSIONS_RDWR, ATT_128_UUID_LEN, (u8 *)(size_t)&sppInUuid[0], (u16 *)(size_t)&sppOutData, sizeof(sppOutData), (u8 *)(size_t)&sppOutData, ATTS_SET_WRITE_CBACK},
        ATTS_COMMON_CCC_DEFINE,
};

int telinkSppWrite(u16 connHandle, u8 opcode, u16 attrHandle, u8 *writeValue, u16 valueLen)
{
    memcpy(sppOutData, writeValue, valueLen);
    sppOutDataLen = valueLen;
    return 0;
}

/* GAP group structure */
_attribute_ble_data_retention_ static atts_group_t svcSppGroup =
    {
        NULL,
        sppList,
        NULL,
        NULL,
        SPP_START_HDL,
        0};

void blc_svc_addSppGroup(void)
{
    svcSppGroup.endHandle  = svcSppGroup.startHandle + ARRAY_SIZE(sppList) - 1;
    svcSppGroup.writeCback = telinkSppWrite;
    blc_gatts_addAttributeServiceGroup(&svcSppGroup);
}

void blc_svc_removeSppGroup(void)
{
    blc_gatts_removeAttributeServiceGroup(SPP_START_HDL);
}
