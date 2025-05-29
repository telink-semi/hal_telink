/********************************************************************************************************
 * @file    dis_client.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "stack/ble/host/gatt/tlk_malloc_stack.h"

#include "dis_internal.h"
#include "dis_client_buf.h"


static int  blt_disc_init(u8 initType, const void *param);
static int  blt_disc_connect(u16 connHandle, prf_acl_state_enum connState);
static int  blt_disc_discovery(u16 connHandle);
static int  blt_disc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param);
static void blt_disc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);

static const blc_gapc_discList_t discDis;
#define BLC_DIS_START_SDP(connHandle) blc_gapc_registerDiscoveryService(connHandle, &discDis)

static const blc_gapc_reconnList_t reconnDis;
#define BLC_DIS_START_RECONN(connHandle) blc_gapc_registerReconnectService(connHandle, &reconnDis)
#if ((!defined(HOST_V2_ENABLE)))
_attribute_ble_data_retention_ struct blc_dis_client_ctrl dis_client_ctrl = {
    .process = {
                .pNext       = NULL,
                .id          = DIS_CLIENT,
                .usedAclRole = 0,
                .init        = blt_disc_init,
                .connect     = blt_disc_connect,
                .discov      = blt_disc_discovery,
                .loop        = NULL,
                .store       = blt_disc_nv_store,
                },
};
#else
static const struct blc_prf_process_params s_dis_client_process_params = {
    .id = DIS_CLIENT,
    .usedAclRole = PRF_GAP_ACL_UNSPECIF,
    .init = blt_disc_init,
    .connect = blt_disc_connect,
    .discovery = blt_disc_discovery,
    .store = blt_disc_nv_store,
};

_attribute_ble_data_retention_ struct blc_dis_client_ctrl dis_client_ctrl = {
    .process = {
                .next = SLIST_HEAD_INITIALIZER(),
                .prf_params = &s_dis_client_process_params,
                },
};
#endif
void blc_basic_registerDISControlClient(const struct blc_disc_regParam *param)
{
#if ((!defined(HOST_V2_ENABLE)))
    blc_prf_registerServiceModule(PRF_GAP_ACL_UNSPECIF, (blc_prf_proc_t *)&dis_client_ctrl, param);
#else
    blc_prf_registerServiceModule((struct blc_prf_process *) &dis_client_ctrl, param);
#endif
}

static int blt_disc_init(u8 initType, const void *param)
{
#if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(struct blc_dis_client)), blc_dis_client);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(struct blc_dis_client_ctrl)), blc_dis_client_ctrl);
#endif

    (void)param;

    if (initType == PRF_PROC_INIT) {
        BLT_DIS_LOG("Client init");
    }
    //  else if (initType == PRF_PROC_DEINIT) {
    //      BLT_DIS_LOG("Client deinit");
    //  }
    return 0;
}

static struct blc_dis_client *blt_disc_getClientInst(u16 connHandle)
{
    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle
    return dis_client_ctrl.pDisClient[idx];
}

static int blt_disc_connect(u16 connHandle, prf_acl_state_enum connState)
{
    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle

    if (connState == PRF_ACL_STATE_DISCONN) {
        BLT_DIS_LOG("Disconnect:0x%x", connHandle);
        DIS_FREE(dis_client_ctrl.pDisClient[idx]);
        dis_client_ctrl.pDisClient[idx] = NULL;
    } else {
        BLT_DIS_LOG("Connect:0x%x", connHandle);
        dis_client_ctrl.pDisClient[idx] = DIS_MALLOC(sizeof(struct blc_dis_client));
        memset(dis_client_ctrl.pDisClient[idx], 0, sizeof(struct blc_dis_client));
    }

    return 0;
}

static int blt_disc_discovery(u16 connHandle)
{
    BLC_BASIC_SDP_DISCOVERY(connHandle, DIS, dis);
}

static int blt_disc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param)
{
    BLC_BASIC_NV_STORE(connHandle, DIS, dis, udiForMedicalDevicesHdl);
    return 0;
}

//For code alignment, no real functionality
static void blt_disc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    (void)connHandle;
    (void)attHdl;
    (void)val;
    (void)valLen;
}

/***************************DIS sdp discovery*******************************/

static void blt_disc_displayInfo(u16 connHandle, struct blc_dis_client *client)
{
    BLT_DIS_LOG("DIS sdp over connHandle[0x%x]", connHandle);

    if (client->manufacturerNameHdl) {
        BLT_DIS_LOG("Manufacturer Name:[handle:0x%x value:%.*s]",
                    client->manufacturerNameHdl,
                    client->manufacturerNameLen,
                    client->manufacturerName);
    }

    if (client->modelNumberHdl) {
        BLT_DIS_LOG("Model Number:[handle:0x%x value:%.*s]",
                    client->modelNumberHdl,
                    client->modelNumberLen,
                    client->modelNumber);
    }

    if (client->serialNumberHdl) {
        BLT_DIS_LOG("Serial Number:[handle:0x%x value:%.*s]",
                    client->serialNumberHdl,
                    client->serialNumberLen,
                    client->serialNumber);
    }

    if (client->hardwareRevisionHdl) {
        BLT_DIS_LOG("Hardware Revision:[handle:0x%x value:%.*s]",
                    client->hardwareRevisionHdl,
                    client->hardwareRevisionLen,
                    client->hardwareRevision);
    }

    if (client->firmwareRevisionHdl) {
        BLT_DIS_LOG("Firmware Revision:[handle:0x%x value:%.*s]",
                    client->firmwareRevisionHdl,
                    client->firmwareRevisionLen,
                    client->firmwareRevision);
    }

    if (client->softwareRevisionHdl) {
        BLT_DIS_LOG("Software Revision:[handle:0x%x value:%.*s]",
                    client->softwareRevisionHdl,
                    client->softwareRevisionLen,
                    client->softwareRevision);
    }

    if (client->systemIdHdl) {
        BLT_DIS_LOG("System ID:[handle:0x%x, manufacturer:0x%02x%02x%02x%02x%02x, OUI:0x%02x%02x%02x]",
                    client->systemIdHdl,
                    client->systemId.manufacturer[4],
                    client->systemId.manufacturer[3],
                    client->systemId.manufacturer[2],
                    client->systemId.manufacturer[1],
                    client->systemId.manufacturer[0],
                    client->systemId.oui[2],
                    client->systemId.oui[1],
                    client->systemId.oui[0]);
    }

    if (client->IEEEDataListHdl) {
        BLT_DIS_LOG("IEEE 11073-20601 Regulatory Certification Data List:[handle:0x%x, value is %s]",
                    client->IEEEDataListHdl,
                    hex_to_str(client->IEEEDataList, client->IEEEDataListLen));
    }

    if (client->PnPIDHdl) {
        BLT_DIS_LOG("PnP ID:[handle:0x%x, %s:%d(0x%x) Product Id:%d, Product Version:%d]",
                    client->PnPIDHdl,
                    client->PnPID.vidSrc == 0x01 ? "Bluetooth SIG Company ID" :
                                                   (client->PnPID.vidSrc == 0x02 ? "USB Implementer��s Forum assigned Vendor ID " : "Reserved for future use"),
                    client->PnPID.vid,
                    client->PnPID.vid,
                    client->PnPID.pid,
                    client->PnPID.ver);
    }

    if (client->udiForMedicalDevicesHdl) {
        BLT_DIS_LOG("UDI for Medical Devices:[handle:0x%x, value is %s]",
                    client->udiForMedicalDevicesHdl,
                    hex_to_str(client->udiForMedicalDevices, client->udiForMedicalDevicesLen));
    }
}


BLT_BASIC_SDP_DISCOVERY_SERVICE(dis, DIS)
BLT_DEFINE_DIS_DISCOVERY_FOUND_CHAR(manufacturerName)
BLT_DEFINE_DIS_DISCOVERY_START_READ(manufacturerName)
BLT_DEFINE_DIS_DISCOVERY_FOUND_CHAR(modelNumber)
BLT_DEFINE_DIS_DISCOVERY_START_READ(modelNumber)
BLT_DEFINE_DIS_DISCOVERY_FOUND_CHAR(serialNumber)
BLT_DEFINE_DIS_DISCOVERY_START_READ(serialNumber)
BLT_DEFINE_DIS_DISCOVERY_FOUND_CHAR(hardwareRevision)
BLT_DEFINE_DIS_DISCOVERY_START_READ(hardwareRevision)
BLT_DEFINE_DIS_DISCOVERY_FOUND_CHAR(firmwareRevision)
BLT_DEFINE_DIS_DISCOVERY_START_READ(firmwareRevision)
BLT_DEFINE_DIS_DISCOVERY_FOUND_CHAR(softwareRevision)
BLT_DEFINE_DIS_DISCOVERY_START_READ(softwareRevision)
BLT_DEFINE_DIS_DISCOVERY_FOUND_CHAR(systemId)
BLT_DEFINE_DIS_DISCOVERY_START_READ_FIX_LEN(systemId)
BLT_DEFINE_DIS_DISCOVERY_FOUND_CHAR(IEEEDataList)
BLT_DEFINE_DIS_DISCOVERY_START_READ(IEEEDataList)
BLT_DEFINE_DIS_DISCOVERY_FOUND_CHAR(PnPID)
BLT_DEFINE_DIS_DISCOVERY_START_READ_FIX_LEN(PnPID)
BLT_DEFINE_DIS_DISCOVERY_FOUND_CHAR(udiForMedicalDevices)
BLT_DEFINE_DIS_DISCOVERY_START_READ(udiForMedicalDevices)

static const blc_gapc_discService_t disService = {
    .uuid = UUID16_INIT(SERVICE_UUID_DEVICE_INFORMATION),
    .sfun = blt_disc_foundService,
};

static const blc_gapc_discChar_t disChar[] = {
    BLT_DIS_DISCOVERY_READ_CHAR(CHARACTERISTIC_UUID_MANUFACTURER_NAME_STRING, manufacturerName),
    BLT_DIS_DISCOVERY_READ_CHAR(CHARACTERISTIC_UUID_MODEL_NUMBER_STRING, modelNumber),
    BLT_DIS_DISCOVERY_READ_CHAR(CHARACTERISTIC_UUID_SERIAL_NUMBER_STRING, serialNumber),
    BLT_DIS_DISCOVERY_READ_CHAR(CHARACTERISTIC_UUID_HARDWARE_REVISION_STRING, hardwareRevision),
    BLT_DIS_DISCOVERY_READ_CHAR(CHARACTERISTIC_UUID_FIRMWARE_REVISION_STRING, firmwareRevision),
    BLT_DIS_DISCOVERY_READ_CHAR(CHARACTERISTIC_UUID_SOFTWARE_REVISION_STRING, softwareRevision),
    BLT_DIS_DISCOVERY_READ_CHAR(CHARACTERISTIC_UUID_SYSTEM_ID, systemId),
    BLT_DIS_DISCOVERY_READ_CHAR(CHARACTERISTIC_UUID_IEEE11073_20601_DATA_LIST, IEEEDataList),
    BLT_DIS_DISCOVERY_READ_CHAR(CHARACTERISTIC_UUID_PNP_ID, PnPID),
    BLT_DIS_DISCOVERY_READ_CHAR(CHARACTERISTIC_UUID_UDI_FOR_MEDICAL_DEVICES, udiForMedicalDevices),
};

static const blc_gapc_discList_t discDis = {
    .maxServiceCount = 1,
    .service         = &disService,
    .includeTable    = {
                        .size = 0,
                        },
    .characteristicTable = {
                        .size           = ARRAY_SIZE(disChar),
                        .characteristic = disChar,
                        },
};

/***************************DIS sdp discovery end*******************************/

/**********reconnect function********/
BLT_BASIC_RECONNECT_SERVICE(dis, DIS)
BLT_DIS_RECONNECT_GET_INFO_READ(manufacturerName)
BLT_DIS_RECONNECT_GET_INFO_READ(modelNumber)
BLT_DIS_RECONNECT_GET_INFO_READ(serialNumber)
BLT_DIS_RECONNECT_GET_INFO_READ(hardwareRevision)
BLT_DIS_RECONNECT_GET_INFO_READ(firmwareRevision)
BLT_DIS_RECONNECT_GET_INFO_READ(softwareRevision)
BLT_DIS_RECONNECT_GET_INFO_READ(systemId)
BLT_DIS_RECONNECT_GET_INFO_READ(IEEEDataList)
BLT_DIS_RECONNECT_GET_INFO_READ(PnPID)
BLT_DIS_RECONNECT_GET_INFO_READ(udiForMedicalDevices)

static const blc_gapc_reconnChar_t reDisChar[] = {
    BLT_DIS_RECONNECT_CHAR(manufacturerName),
    BLT_DIS_RECONNECT_CHAR(modelNumber),
    BLT_DIS_RECONNECT_CHAR(serialNumber),
    BLT_DIS_RECONNECT_CHAR(hardwareRevision),
    BLT_DIS_RECONNECT_CHAR(firmwareRevision),
    BLT_DIS_RECONNECT_CHAR(softwareRevision),
    BLT_DIS_RECONNECT_CHAR(systemId),
    BLT_DIS_RECONNECT_CHAR(IEEEDataList),
    BLT_DIS_RECONNECT_CHAR(PnPID),
    BLT_DIS_RECONNECT_CHAR(udiForMedicalDevices),
};

static const blc_gapc_reconnList_t reconnDis = {
    .resfun = blt_disc_recService,
    .charTb = {
               .size           = ARRAY_SIZE(reDisChar),
               .characteristic = reDisChar,
               },
    .inclSize = 0,
};

/**********reconnect function ending********/

/**********Read Characteristic Attribute Value*********/
int blc_disc_readManufacturerName(u16 connHandle, prf_read_cb_t readCb)
{
    BLT_DIS_READ_ATTR_VALUE(manufacturerName);
}

int blc_disc_readModelNumber(u16 connHandle, prf_read_cb_t readCb)
{
    BLT_DIS_READ_ATTR_VALUE(modelNumber);
}

int blc_disc_readSerialNumber(u16 connHandle, prf_read_cb_t readCb)
{
    BLT_DIS_READ_ATTR_VALUE(serialNumber);
}

int blc_disc_readHardwareRevision(u16 connHandle, prf_read_cb_t readCb)
{
    BLT_DIS_READ_ATTR_VALUE(hardwareRevision);
}

int blc_disc_readFirmwareRevision(u16 connHandle, prf_read_cb_t readCb)
{
    BLT_DIS_READ_ATTR_VALUE(firmwareRevision);
}

int blc_disc_readSoftwareRevision(u16 connHandle, prf_read_cb_t readCb)
{
    BLT_DIS_READ_ATTR_VALUE(softwareRevision);
}

int blc_disc_readSystemId(u16 connHandle, prf_read_cb_t readCb)
{
    BLT_DIS_READ_ATTR_VALUE_FIX_LEN(systemId);
}

int blc_disc_readIEEEDataList(u16 connHandle, prf_read_cb_t readCb)
{
    BLT_DIS_READ_ATTR_VALUE(IEEEDataList);
}

int blc_disc_readPnPID(u16 connHandle, prf_read_cb_t readCb)
{
    BLT_DIS_READ_ATTR_VALUE_FIX_LEN(PnPID);
}

int blc_disc_readUdiForMedicalDevices(u16 connHandle, prf_read_cb_t readCb)
{
    BLT_DIS_READ_ATTR_VALUE(udiForMedicalDevices);
}

/**********Read Characteristic Attribute Value End*********/

/**********Get Characteristic Attribute Value*********/

int blc_disc_getManufacturerName(u16 connHandle, u8 *manufacturerName, u16 *manufacturerNameLen)
{
    BLT_DIS_GET_ATTR_VALUE(manufacturerName);
}

int blc_disc_getModelNumber(u16 connHandle, u8 *modelNumber, u16 *modelNumberLen)
{
    BLT_DIS_GET_ATTR_VALUE(modelNumber);
}

int blc_disc_getSerialNumber(u16 connHandle, u8 *serialNumber, u16 *serialNumberLen)
{
    BLT_DIS_GET_ATTR_VALUE(serialNumber);
}

int blc_disc_getHardwareRevision(u16 connHandle, u8 *hardwareRevision, u16 *hardwareRevisionLen)
{
    BLT_DIS_GET_ATTR_VALUE(hardwareRevision);
}

int blc_disc_getFirmwareRevision(u16 connHandle, u8 *firmwareRevision, u16 *firmwareRevisionLen)
{
    BLT_DIS_GET_ATTR_VALUE(firmwareRevision);
}

int blc_disc_getSoftwareRevision(u16 connHandle, u8 *softwareRevision, u16 *softwareRevisionLen)
{
    BLT_DIS_GET_ATTR_VALUE(softwareRevision);
}

#if ((!defined(HOST_V2_ENABLE)))
int blc_disc_getSystemId(u16 connHandle, dis_system_id_t *systemId)
{
    BLT_DIS_GET_ATTR_VALUE_FIX_LEN(systemId);
}

int blc_disc_getPnPID(u16 connHandle, dis_pnp_t *PnPID)
{
    BLT_DIS_GET_ATTR_VALUE_FIX_LEN(PnPID);
}
#else
//int blc_disc_getSystemId(u16 connHandle, struct dis_system_id *systemId)
//{
//    BLT_DIS_GET_ATTR_VALUE_FIX_LEN(systemId);
//}
//
//int blc_disc_getPnPID(u16 connHandle, struct dis_pnp *PnPID)
//{
//    BLT_DIS_GET_ATTR_VALUE_FIX_LEN(PnPID);
//}
#endif

int blc_disc_getIEEEDataList(u16 connHandle, u8 *IEEEDataList, u16 *IEEEDataListLen)
{
    BLT_DIS_GET_ATTR_VALUE(IEEEDataList);
}

int blc_disc_getUdiForMedicalDevices(u16 connHandle, u8 *udiForMedicalDevices, u16 *udiForMedicalDevicesLen)
{
    BLT_DIS_GET_ATTR_VALUE(udiForMedicalDevices);
}

/**********Get Characteristic Attribute Value End*********/
