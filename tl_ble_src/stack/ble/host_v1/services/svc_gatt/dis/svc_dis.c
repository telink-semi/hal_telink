/********************************************************************************************************
 * @file    svc_dis.c
 *
 * @brief   This is the source file for TLSR/TL
 *
 * @author  Bluetooth Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#include <stddef.h>

#include "common/types.h"
#include "common/utility.h"
#include "common/compiler.h"

#include "../../../l2cap/att/inc/ble_att_uuid.h"
#include "../../../l2cap/att/inc/ble_att_service.h"

#include "../../../l2cap/att/inc/uuid16bit.h"

#include "../../inc/svc.h"
#include "../../inc/svc_format.h"

#include "../svc_gatt.h"

#include "svc_dis.h"

#ifndef DIS_SUPP_MANUFACTURER_NAME_STRING
#define DIS_SUPP_MANUFACTURER_NAME_STRING       0
#endif

#ifndef DIS_SUPP_MODEL_NUMBER_STRING
#define DIS_SUPP_MODEL_NUMBER_STRING            0
#endif

#ifndef DIS_SUPP_SERIAL_NUMBER_STRING
#define DIS_SUPP_SERIAL_NUMBER_STRING           0
#endif

#ifndef DIS_SUPP_HARDWARE_REVISION_STRING
#define DIS_SUPP_HARDWARE_REVISION_STRING       0
#endif

#ifndef DIS_SUPP_FIRMWARE_REVISION_STRING
#define DIS_SUPP_FIRMWARE_REVISION_STRING       0
#endif

#ifndef DIS_SUPP_SOFTWARE_REVISION_STRING
#define DIS_SUPP_SOFTWARE_REVISION_STRING       0
#endif

#ifndef DIS_SUPP_SYSTEM_ID
#define DIS_SUPP_SYSTEM_ID                      0
#endif

#ifndef DIS_SUPP_IEEE_11073_20601
#define DIS_SUPP_IEEE_11073_20601               0
#endif

#ifndef DIS_SUPP_PNP_ID
#define DIS_SUPP_PNP_ID                         1
#endif

#ifndef DIS_SUPP_UDI_FOR_MEDICAL_DEVICES
#define DIS_SUPP_UDI_FOR_MEDICAL_DEVICES        0
#endif

#define DIS_START_HDL                           SERVICE_DEVICE_INFORMATION_HDL

#if DIS_SUPP_MANUFACTURER_NAME_STRING
static const char manufacturerName[] = "Telink-semi";
static const uint16_t  manufacturerNameLen = sizeof(manufacturerName);
#endif

#if DIS_SUPP_MODEL_NUMBER_STRING
static const char modelNumber[] = "mult-conn-sdk";
static const uint16_t  modelNumberLen = sizeof(modelNumber);
#endif

#if DIS_SUPP_SERIAL_NUMBER_STRING
static const char serialNumber[] = "0000-0000-0000";
static const uint16_t  serialNumberLen = sizeof(serialNumber);
#endif

#if DIS_SUPP_HARDWARE_REVISION_STRING
static const char hardwareRevision[] = "0.0.0";
static const uint16_t  hardwareRevisionLen = sizeof(hardwareRevision);
#endif

#if DIS_SUPP_FIRMWARE_REVISION_STRING
static const char firmwareRevision[] = "BLE-5.4";
static const uint16_t  firmwareRevisionLen = sizeof(firmwareRevision);
#endif

#if DIS_SUPP_SOFTWARE_REVISION_STRING
static const char softwareRevision[] = "4.1.0";
static const uint16_t  softwareRevisionLen = sizeof(softwareRevision);
#endif

#if DIS_SUPP_SYSTEM_ID
//Organizationally Unique Identifier(OUI)
static const struct dis_system_id systemId;
static const uint16_t systemIdLen = sizeof(systemId);
#endif

#if DIS_SUPP_IEEE_11073_20601
static const uint8_t  IEEE_DataList[] = { 0x01 };
static const uint16_t IEEE_DataListLen = sizeof(IEEE_DataList);
#endif

#if DIS_SUPP_PNP_ID
static const struct dis_pnp PnPID = {
    .vidSrc = 0x02,
    .vid = 0x248a,
    .pid = 0x8266,
    .ver = 0x0001,
};
static const uint16_t PnPIDLen = sizeof(struct dis_pnp);
#endif

#if DIS_SUPP_UDI_FOR_MEDICAL_DEVICES
//Unique Device Identifier(UDI) for Medical Devices,
static const uint8_t  udiForMedicalDevices[] = { 0x01 };
static const uint16_t udiForMedicalDevicesLen = sizeof(udiForMedicalDevices);
#endif

/*
 * @brief the structure for default DIS service List.
 */
static const struct atts_attribute disList[] =
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
    //Firmware Revision String
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
_attribute_ble_data_retention_
static struct atts_group svcDisGroup =
{
    NULL,
    disList,
    NULL,
    NULL,
    DIS_START_HDL,
    0
};

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
