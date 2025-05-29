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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "default_att.h"


#include "ble_att_uuid.h"
#include "ble_att_service.h"

#include "uuid16bit.h"

#include "svc.h"
#include "svc_format.h"

#if(1)


///** GATT Characteristic Properties bit field */
//#define CHAR_PROP_BROADCAST         BIT(0)
//#define CHAR_PROP_READ              BIT(1)
//#define CHAR_PROP_WRITE_WITHOUT_RSP BIT(2)
//#define CHAR_PROP_WRITE             BIT(3)
//#define CHAR_PROP_NOTIFY            BIT(4)
//#define CHAR_PROP_INDICATE          BIT(5)
//#define CHAR_PROP_AUTHEN            BIT(6)
//#define CHAR_PROP_EXTENDED          BIT(7)
//uint8_t clientCharacteristicConfiguration[2] = { 0x00, 0x00 };
//const uint16_t clientCharacteristicConfigurationLen = sizeof(clientCharacteristicConfiguration);
//#define ATT_16_UUID_LEN  2  /*!< \brief Length in bytes of a 16 bit UUID */
//#define ATT_128_UUID_LEN 16 /*!< \brief Length in bytes of a 128 bit UUID */
//
//const uint8_t charPropReadWriteNotify = CHAR_PROP_READ | CHAR_PROP_WRITE | CHAR_PROP_NOTIFY;
//#define ATTS_CHAR_UUID128_DEFINE(perm, uuid, valueLen, maxValueLen, value, settings)    {perm, ATT_128_UUID_LEN, (u8*)(size_t)uuid, (u16*)(size_t)&valueLen, maxValueLen, (u8*)(size_t)value, settings}
//#define ATTS_CHAR_UUID128_DEFINE_VALUE_POINTER(properties, perm, uuid, maxValueLen, value, settings)    \
//                                                                                       ATTS_CHARACTERISTIC_DECLARATIONS(properties),       \
//                                                                                       ATTS_CHAR_UUID128_DEFINE(perm, uuid, value##Len, maxValueLen, value, settings)
//
//
//#define ATTS_CHAR_UUID128_RDWR_POINT(properties, uuid, maxValueLen, value, settings)            ATTS_CHAR_UUID128_DEFINE_VALUE_POINTER(properties, ATT_PERMISSIONS_RDWR, uuid, maxValueLen, value, settings)
//
//#define ATTS_CHAR_UUID128_RDWR_POINT_RWCB(properties, uuid, maxValueLen, value)                 ATTS_CHAR_UUID128_RDWR_POINT(properties, uuid, maxValueLen, value, ATTS_SET_WRITE_CBACK | ATTS_SET_READ_CBACK)
//
//#define ATTS_CCC_DEFINE_COMMON(ccc, cccLen)     {ATT_PERMISSIONS_RDWR,ATT_16_UUID_LEN,(uint8_t*)(size_t)descriptorClientCharacteristicConfigurationUuid,(uint16_t*)(size_t)&cccLen,sizeof(ccc),(uint8_t*)(size_t)ccc,0}
//#define ATTS_CCC_DEFINE(ccc)                    ATTS_CCC_DEFINE_COMMON(ccc, ccc##Len)
//#define ATTS_COMMON_CCC_DEFINE                  ATTS_CCC_DEFINE(clientCharacteristicConfiguration)


 const static u8 serviceSppUuid[16]  = {TELINK_SPP_UUID_SERVICE};

_attribute_ble_data_retention_ u8 sppInData[100];
_attribute_ble_data_retention_ u16 sppInDataLen = 0;

 const static u8 sppInUuid[16]       = {TELINK_SPP_DATA_CLIENT2SERVER};

_attribute_ble_data_retention_ u8 sppOutData[256];
_attribute_ble_data_retention_ u16 sppOutDataLen = 0;

static const atts_attribute_t sppList[] =
{
    ATTS_PRIMARY_SERVICE_128((u8*)serviceSppUuid),
    ATTS_CHARACTERISTIC_DECLARATIONS(charPropReadWriteNotify),
    {ATT_PERMISSIONS_RDWR, ATT_128_UUID_LEN, (u8 *)(size_t)&sppInUuid[0], (u16 *)(size_t)&sppOutDataLen, sizeof(sppOutData), (u8 *)(size_t)&sppOutData, ATTS_SET_WRITE_CBACK|ATTS_SET_READ_CBACK},
    ATTS_COMMON_CCC_DEFINE,
};




int fp_write_cb(u16 connHandle,
                u8 opcode,
                u16 attrHandle,
                u8* in,
                u16 inLen) {
    tlkapi_printf(1, "attrHandle: %x, len: %d", attrHandle, inLen);
    tlkapi_send_string_data(1, "received", in, inLen);
    memcpy(sppOutData, in, inLen);
    return 0;
}
/* GAP group structure */
_attribute_ble_data_retention_ static atts_group_t svcSppGroup =
{
    NULL,
    (atts_attribute_t *) sppList,
    NULL,
    &fp_write_cb,
    SPP_START_HDL,
    SPP_END_HDL
};

void blc_svc_addSppGroup(void)
{
    assert(SPP_MAX_HDL-SPP_START_HDL > TELINK_SPP_MAX_HDL_NUM);
    blc_gatts_addAttributeServiceGroup(&svcSppGroup);
}
#endif
