/********************************************************************************************************
 * @file    svc_dis.c
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

#ifndef DIS_SUPP_MANUFACTURER_NAME_STRING
    #define DIS_SUPP_MANUFACTURER_NAME_STRING 0
#endif

#ifndef DIS_SUPP_MODEL_NUMBER_STRING
    #define DIS_SUPP_MODEL_NUMBER_STRING 0
#endif

#ifndef DIS_SUPP_SERIAL_NUMBER_STRING
    #define DIS_SUPP_SERIAL_NUMBER_STRING 0
#endif

#ifndef DIS_SUPP_HARDWARE_REVISION_STRING
    #define DIS_SUPP_HARDWARE_REVISION_STRING 0
#endif

#ifndef DIS_SUPP_FIRMWARE_REVISION_STRING
    #define DIS_SUPP_FIRMWARE_REVISION_STRING 0
#endif

#ifndef DIS_SUPP_SOFTWARE_REVISION_STRING
    #define DIS_SUPP_SOFTWARE_REVISION_STRING 0
#endif

#ifndef DIS_SUPP_SYSTEM_ID
    #define DIS_SUPP_SYSTEM_ID 0
#endif

#ifndef DIS_SUPP_IEEE_11073_20601
    #define DIS_SUPP_IEEE_11073_20601 0
#endif

#ifndef DIS_SUPP_PNP_ID
    #define DIS_SUPP_PNP_ID 1
#endif

#ifndef DIS_SUPP_UDI_FOR_MEDICAL_DEVICES
    #define DIS_SUPP_UDI_FOR_MEDICAL_DEVICES 0
#endif

#define DIS_START_HDL SERVICE_DEVICE_INFORMATION_HDL

#if DIS_SUPP_MANUFACTURER_NAME_STRING
static const char manufacturerName[]  = "Telink-semi";
static const u16  manufacturerNameLen = sizeof(manufacturerName);
#endif

#if DIS_SUPP_MODEL_NUMBER_STRING
static const char modelNumber[]  = "mult-conn-sdk";
static const u16  modelNumberLen = sizeof(modelNumber);
#endif

#if DIS_SUPP_SERIAL_NUMBER_STRING
static const char serialNumber[]  = "0000-0000-0000";
static const u16  serialNumberLen = sizeof(serialNumber);
#endif

#if DIS_SUPP_HARDWARE_REVISION_STRING
static const char hardwareRevision[]  = "0.0.0";
static const u16  hardwareRevisionLen = sizeof(hardwareRevision);
#endif

#if DIS_SUPP_FIRMWARE_REVISION_STRING
static const char firmwareRevision[]  = "BLE-5.4";
static const u16  firmwareRevisionLen = sizeof(firmwareRevision);
#endif

#if DIS_SUPP_SOFTWARE_REVISION_STRING
static const char softwareRevision[]  = "4.1.0";
static const u16  softwareRevisionLen = sizeof(softwareRevision);
#endif

#if DIS_SUPP_SYSTEM_ID
//Organizationally Unique Identifier(OUI)
static const dis_system_id_t systemId;
;
static const u16 systemIdLen = sizeof(systemId);
#endif

#if DIS_SUPP_IEEE_11073_20601
static const u8  IEEE_DataList[]  = {0x01};
static const u16 IEEE_DataListLen = sizeof(IEEE_DataList);
#endif

#if DIS_SUPP_PNP_ID
static const dis_pnp_t PnPID = {
    .vidSrc = 0x02,
    .vid    = 0x248a,
    .pid    = 0x8266,
    .ver    = 0x0001,
};
static const u16 PnPIDLen = sizeof(PnPID);
#endif

#if DIS_SUPP_UDI_FOR_MEDICAL_DEVICES
//Unique Device Identifier(UDI) for Medical Devices,
static const u8  udiForMedicalDevices[]  = {0x01};
static const u16 udiForMedicalDevicesLen = sizeof(udiForMedicalDevices);
#endif

/*
 * @brief the structure for default DIS service List.
 */
static const atts_attribute_t disList[] =
    {
        ATTS_PRIMARY_SERVICE(serviceDeviceInformationUuid),

#if DIS_SUPP_MANUFACTURER_NAME_STRING
        //Manufacturer Name String
        ATTS_CHAR_UUID_READ_POINT_NOCB(charPropRead, characteristicManufacturerNameStringUuid, manufacturerName),
#endif

#if DIS_SUPP_MODEL_NUMBER_STRING
        //Model Number String
        ATTS_CHAR_UUID_READ_POINT_NOCB(charPropRead, characteristicModelNumberStringUuid, modelNumber),
#endif

#if DIS_SUPP_SERIAL_NUMBER_STRING
        //Serial Number String
        ATTS_CHAR_UUID_READ_POINT_NOCB(charPropRead, characteristicSerialNumberStringUuid, serialNumber),
#endif

#if DIS_SUPP_HARDWARE_REVISION_STRING
        //Hardware Revision String
        ATTS_CHAR_UUID_READ_POINT_NOCB(charPropRead, characteristicHardwareRevisionStringUuid, hardwareRevision),
#endif

#if DIS_SUPP_FIRMWARE_REVISION_STRING
        //Frimware Revision String
        ATTS_CHAR_UUID_READ_POINT_NOCB(charPropRead, characteristicFirmwareRevisionStringUuid, firmwareRevision),
#endif

#if DIS_SUPP_SOFTWARE_REVISION_STRING
        //Software Revision String
        ATTS_CHAR_UUID_READ_POINT_NOCB(charPropRead, characteristicSoftwareRevisionStringUuid, softwareRevision),
#endif

#if DIS_SUPP_SYSTEM_ID
        //System ID
        ATTS_CHAR_UUID_READ_ENTITY_NOCB(charPropRead, characteristicSystemIdUuid, systemId),
#endif

#if DIS_SUPP_IEEE_11073_20601
        //IEEE 11073-20601 Regulatory Certification Data List
        ATTS_CHAR_UUID_READ_POINT_NOCB(charPropRead, characteristicIEEE_11073_20601DataListUuid, IEEE_DataList),
#endif

#if DIS_SUPP_PNP_ID
        //PNP ID
        ATTS_CHAR_UUID_READ_ENTITY_NOCB(charPropRead, characteristicPnpIdUuid, PnPID),
#endif

#if DIS_SUPP_UDI_FOR_MEDICAL_DEVICES
        //UDI for Medical Devices
        ATTS_CHAR_UUID_READ_POINT_NOCB(charPropRead, characteristicUdiForMedicalDevicesUuid, udiForMedicalDevices),
#endif

};

/*
 * @brief the structure for default DIS service group.
 */
_attribute_ble_data_retention_ static atts_group_t svcDisGroup =
    {
        NULL,
        disList,
        NULL,
        NULL,
        DIS_START_HDL,
        0};

/**
 * @brief      for user add default DIS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addDisGroup(void)
{
    svcDisGroup.endHandle = svcDisGroup.startHandle + ARRAY_SIZE(disList) - 1;
    blc_gatts_addAttributeServiceGroup(&svcDisGroup);
}

/**
 * @brief      for user remove default DIS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeDisGroup(void)
{
    blc_gatts_removeAttributeServiceGroup(DIS_START_HDL);
}
