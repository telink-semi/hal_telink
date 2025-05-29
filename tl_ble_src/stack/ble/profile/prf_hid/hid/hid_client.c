/********************************************************************************************************
 * @file    hid_client.c
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

#include "hid_internal.h"
#include "hid_client_buf.h"

#include "stack/ble/host/gatt/tlk_malloc_stack.h"


static int  blt_hidc_init(u8 initType, const void *param);
static int  blt_hidc_connect(u16 connHandle, prf_acl_state_enum connState);
static int  blt_hidc_discovery(u16 connHandle);
static int  blt_hidc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param);
static void blt_hidc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);

static const blc_gapc_discList_t discHid;
#define BLC_HID_START_SDP(connHandle) blc_gapc_registerDiscoveryService(connHandle, &discHid)

static const blc_gapc_reconnList_t reconnHid;
#define BLC_HID_START_RECONN(connHandle) blc_gapc_registerReconnectService(connHandle, &reconnHid)
#if ((!defined(HOST_V2_ENABLE)))
_attribute_ble_data_retention_ struct blc_hid_client_ctrl hid_client_ctrl = {
    .process = {
                .pNext       = NULL,
                .id          = HID_CLIENT,
                .usedAclRole = 0,
                .init        = blt_hidc_init,
                .connect     = blt_hidc_connect,
                .discov      = blt_hidc_discovery,
                .loop        = NULL,
                .store       = blt_hidc_nv_store,
                },
};
#else
static const struct blc_prf_process_params s_hid_client_process_params = {
    .id = HID_CLIENT,
    .usedAclRole = PRF_GAP_ACL_CENTRAL,
    .init = blt_hidc_init,
    .connect = blt_hidc_connect,
    .discovery = blt_hidc_discovery,
    .store = blt_hidc_nv_store,
};

_attribute_ble_data_retention_ struct blc_hid_client_ctrl hid_client_ctrl = {
    .process = {
                .next = SLIST_HEAD_INITIALIZER(),
                .prf_params = &s_hid_client_process_params,
                },
};
#endif
void blc_hid_registerHIDControlClient(const struct blc_hidc_regParam *param)
{
#if ((!defined(HOST_V2_ENABLE)))
    blc_prf_registerServiceModule(PRF_GAP_ACL_CENTRAL, (blc_prf_proc_t *)&hid_client_ctrl, param);
#else
    blc_prf_registerServiceModule((struct blc_prf_process *) &hid_client_ctrl, param);
#endif
}

static int blt_hidc_init(u8 initType, const void *param)
{
    (void)param;
#if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(struct blc_hid_client)), blc_hid_client);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(struct blc_hid_client_ctrl)), blc_hid_client_ctrl);
#endif

    if (initType == PRF_PROC_INIT) {
        BLT_HID_LOG("Client init");
    }
    //  else if (initType == PRF_PROC_DEINIT) {
    //      BLT_HID_LOG("Client deinit");
    //  }
    return 0;
}

static struct blc_hid_client *blt_hidc_getClientInst(u16 connHandle)
{
    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle
    return hid_client_ctrl.pHidClient[idx];
}

static int blt_hidc_connect(u16 connHandle, prf_acl_state_enum connState)
{
    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle

    if (connState == PRF_ACL_STATE_DISCONN) {
        BLT_HID_LOG("Disconnect:0x%x", connHandle);
        HID_FREE(hid_client_ctrl.pHidClient[idx]);
        hid_client_ctrl.pHidClient[idx] = NULL;
    } else {
        BLT_HID_LOG("Connect:0x%x", connHandle);
        hid_client_ctrl.pHidClient[idx] = HID_MALLOC(sizeof(struct blc_hid_client));
        memset(hid_client_ctrl.pHidClient[idx], 0, sizeof(struct blc_hid_client));
    }

    return 0;
}

static int blt_hidc_discovery(u16 connHandle)
{
    BLC_BASIC_SDP_DISCOVERY(connHandle, HID, hid);
}

static int blt_hidc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param)
{
    BLC_BASIC_NV_STORE(connHandle, HID, hid, reportMapHdl);

    if (nvState == PRF_NV_STATE_STORE) {
        memcpy(&pNvInfo->protocolModeProp, &client->protocolModeProp, 7);
        for (int i = 0; i < client->reportCount; i++) {
            pNvInfo->reportCharInfo[i].attrHandle            = AUD_PARAM_ATT_STORE(client->reportCharInfo[i].attrHandle, client->ntfInput.startHdl);
            pNvInfo->reportCharInfo[i].cccHandle             = AUD_PARAM_ATT_STORE(client->reportCharInfo[i].cccHandle, client->ntfInput.startHdl);
            pNvInfo->reportCharInfo[i].reportReferenceHandle = AUD_PARAM_ATT_STORE(client->reportCharInfo[i].reportReferenceHandle, client->ntfInput.startHdl);
            pNvInfo->reportCharInfo[i].properties            = client->reportCharInfo[i].properties;
            pNvInfo->reportCharInfo[i].reportReferenceValue  = client->reportCharInfo[i].reportReferenceValue;
        }
        pNvInfo->reportCount = client->reportCount;
    } else if (nvState == PRF_NV_STATE_LOAD) {
        memcpy(&client->protocolModeProp, &pNvInfo->protocolModeProp, 7);
        for (int i = 0; i < pNvInfo->reportCount; i++) {
            client->reportCharInfo[i].attrHandle            = AUD_PARAM_ATT_RESTORE(pNvInfo->reportCharInfo[i].attrHandle, client->ntfInput.startHdl);
            client->reportCharInfo[i].cccHandle             = AUD_PARAM_ATT_RESTORE(pNvInfo->reportCharInfo[i].cccHandle, client->ntfInput.startHdl);
            client->reportCharInfo[i].reportReferenceHandle = AUD_PARAM_ATT_RESTORE(pNvInfo->reportCharInfo[i].reportReferenceHandle, client->ntfInput.startHdl);
            client->reportCharInfo[i].properties            = pNvInfo->reportCharInfo[i].properties;
            client->reportCharInfo[i].reportReferenceValue  = pNvInfo->reportCharInfo[i].reportReferenceValue;
        }
        client->reportCount = pNvInfo->reportCount;
    }
    return 0;
}

static void blt_hidc_recvBootKeyboardInputReport(u16 connHandle, struct blc_hid_client *hidc, u8 *val, u16 valLen)
{
    memcpy(&hidc->bootKeyboardInputReport, val, min(valLen, sizeof(hidc->bootKeyboardInputReport)));
    struct blc_hidc_recvBootKeyboardInputReportEvt evt = {
        .value = val,
        .len   = valLen};
    blt_prf_sendEvent(connHandle, HIDC_EVT_RECV_BOOT_KEYBOARD_INPUT_REPORT, &evt, sizeof(struct blc_hidc_recvBootKeyboardInputReportEvt));
}

static void blt_hidc_recvBootMouseInputReport(u16 connHandle, struct blc_hid_client *hidc, u8 *val, u16 valLen)
{
    memcpy(&hidc->bootMouseInputReport, val, min(valLen, sizeof(hidc->bootMouseInputReport)));
    struct blc_hidc_recvBootMouseInputReportEvt evt = {
        .value = val,
        .len   = valLen};
    blt_prf_sendEvent(connHandle, HIDC_EVT_RECV_BOOT_MOUSE_INPUT_REPORT, &evt, sizeof(struct blc_hidc_recvBootMouseInputReportEvt));
}

static void blt_hidc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    BLT_HID_LOG("receive data, connHandle:0x%x, attHdl:0x%x, value:%s", connHandle, attHdl, hex_to_str(val, valLen));

    struct blc_hid_client *hidc = blt_hidc_getClientInst(connHandle);

    if (hidc == NULL) {
        return;
    }

    if (hidc->bootKeyboardInputReportHdl == attHdl) {
        blt_hidc_recvBootKeyboardInputReport(connHandle, hidc, val, valLen);
    } else if (hidc->bootMouseInputReportHdl == attHdl) {
        blt_hidc_recvBootMouseInputReport(connHandle, hidc, val, valLen);
    }

    for (int i = 0; i < HID_SUPPORT_REPORT_HANDLE_MAX; i++) {
        if (hidc->reportCharInfo[i].attrHandle == attHdl) {
            memcpy(&hidc->reportCharInfo[i].report[0], val, min(valLen, sizeof(hidc->reportCharInfo[i].report)));
            struct blc_hidc_recvInputReportData evt = {
                .reportId = hidc->reportCharInfo[i].reportReferenceValue.reportId,
                .value    = val,
                .len      = valLen};
            blt_prf_sendEvent(connHandle, HIDC_EVT_RECV_INPUT_REPORT_DATA, &evt, sizeof(struct blc_hidc_recvInputReportData));
            break;
        }
    }
}

/***************************DIS sdp discovery*******************************/
static void blt_hidc_displayInfo(u16 connHandle, struct blc_hid_client *client)
{
    BLT_HID_LOG("HID sdp over connHandle[0x%x]", connHandle);
    if (client->protocolModeHdl) {
        BLT_HID_LOG("Protocol Mode:[Handle:0x%x, Properties:0x%x, %s]", client->protocolModeHdl, client->protocolModeProp, client->protocolMode == BOOT_PROTOCOL_MODE ? "Boot Protocol Mode" : client->protocolMode == REPORT_PROTOCOL_MODE ? "Report Protocol Mode" :
                                                                                                                                                                                                                                              "Reserved for Future Use");
    }
    if (client->bootKeyboardInputReportHdl) {
        BLT_HID_LOG("Boot Keyboard Input Report:[Handle:0x%x, Properties:0x%x, value:%s]",
                    client->bootKeyboardInputReportHdl,
                    client->bootKeyboardInputReportProp,
                    hex_to_str(&client->bootKeyboardInputReport, 1));
    }
    if (client->bootKeyboardOutputReportHdl) {
        BLT_HID_LOG("Boot Keyboard Output Report:[Handle:0x%x, Properties:0x%x, value:%s]",
                    client->bootKeyboardOutputReportHdl,
                    client->bootKeyboardOutputReportProp,
                    hex_to_str(&client->bootKeyboardOutputReport, 1));
    }
    if (client->bootMouseInputReportHdl) {
        BLT_HID_LOG("Boot Mouse Input Report:[Handle:0x%x, Properties:0x%x, value:%s]",
                    client->bootMouseInputReportHdl,
                    client->bootMouseInputReportProp,
                    hex_to_str(&client->bootMouseInputReport, 1));
    }
    if (client->HIDInformationHdl) {
        BLT_HID_LOG("HID Information:[Handle:0x%x], Properties:0x%x, HID Version: %x, Country Count: %d, Remote Wake:%s, Normally Connectable :%s",
                    client->HIDInformationHdl,
                    client->HIDInformationProp,
                    client->HIDInformation.bcdHID,
                    client->HIDInformation.bCountCode,
                    client->HIDInformation.remoteWake ? "Supp" : "Nosupp",
                    client->HIDInformation.normallyConnectable ? "Supp" : "Nosupp");
    }

    if (client->HIDControlPointHdl) {
        BLT_HID_LOG("HID Control Point:[Handle:0x%x, Properties:0x%x]", client->HIDControlPointHdl, client->HIDControlPointProp);
    }

    if (client->reportMapHdl) {
        BLT_HID_LOG("Report Map:[Handle:0x%x, Properties:0x%x]", client->reportMapHdl, client->reportMapProp);
        BLT_HID_LOG("Map is %s", hex_to_str(client->reportMap, client->reportMapLen));
    }

    for (int i = 0; i < client->reportCount; i++) {
        BLT_HID_LOG("index:%d, Handle:0x%x, Properties:0x%x, CCC Handle:0x%x, Reference Handle:0x%x", i, client->reportCharInfo[i].attrHandle, client->reportCharInfo[i].properties, client->reportCharInfo[i].cccHandle, client->reportCharInfo[i].reportReferenceHandle);

        u8 type = client->reportCharInfo[i].reportReferenceValue.reportType;
        BLT_HID_LOG("report ID is %d, type is %s", client->reportCharInfo[i].reportReferenceValue.reportId, type == HID_REPORT_TYPE_INPUT ? "Input" : type == HID_REPORT_TYPE_OUTPUT ? "Output" :
                                                                                                                                                  type == HID_REPORT_TYPE_FEATURE    ? "Feature" :
                                                                                                                                                                                       "RFU");
    }
}

static void blt_hidc_readReferenceInfo(u16 connHandle, struct blc_hid_client *client);

static void blt_hidc_readReferenceInfoCb(u16 connHandle, u8 err, gattc_read_cfg_t *pRdCfg)
{
    (void)err;
    (void)pRdCfg;
    blt_hidc_readReferenceInfo(connHandle, blt_hidc_getClientInst(connHandle));
}

static void blt_hidc_readReferenceInfo(u16 connHandle, struct blc_hid_client *client)
{
    client->reportFoundDescCount++;
    for (; client->reportFoundDescCount < client->reportCount; client->reportFoundDescCount++) {
        if (client->reportCharInfo[client->reportFoundDescCount].reportReferenceHandle) {
            gapc_read_cfg_t gapReCfg = {
                .handle   = client->reportCharInfo[client->reportFoundDescCount].reportReferenceHandle,
                .wBuff    = &client->reportCharInfo[client->reportFoundDescCount].reportReferenceValue.reportId,
                .wBuffLen = NULL,
                .maxLen   = sizeof(client->reportCharInfo[client->reportFoundDescCount].reportReferenceValue),
                .func     = blt_hidc_readReferenceInfoCb,
            };
            if (blc_gapc_readAttributeValue(connHandle, &gapReCfg) == BLE_SUCCESS) {
                break;
            }
        }
    }

    if (client->reportFoundDescCount == client->reportCount) {
        blt_hidc_displayInfo(connHandle, client);
        blc_prf_setDiscoveryStatusFinish(connHandle);
    }
}

static void blt_hidc_foundService(u16 connHandle, u8 count, u16 startHandle, u16 endHandle)
{
    struct blc_hid_client *client = blt_hidc_getClientInst(connHandle);

    if (count == 0xFF) {
        blc_prf_sendServiceDiscoveryFailEvent(connHandle, HID_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        BLT_HID_LOG("ERR:not found HID");
        return;
    }

    if (count == 0) {
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, HID_CLIENT);
        if (client) {
            blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
            if (client->reportCount) {
                client->reportFoundDescCount = -1;
                blt_hidc_readReferenceInfo(connHandle, client);
            } else {
                blt_hidc_displayInfo(connHandle, client);
                blc_prf_setDiscoveryStatusFinish(connHandle);
            }
        } else {
            blc_prf_setDiscoveryStatusFinish(connHandle);
        }
        return;
    }

    if (client) {
        client->ntfInput.startHdl     = startHandle;
        client->ntfInput.endHdl       = endHandle;
        client->ntfInput.ntfOrIndFunc = blt_hidc_dataInput;
    }

    BLT_HID_LOG("   INFO: HID connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x", connHandle, startHandle, endHandle);
    blc_prf_sendServiceDiscoveryFoundEvent(connHandle, HID_CLIENT, startHandle, endHandle);
}

BLT_DEFINE_HID_DISCOVERY_FOUND_CHAR(protocolMode)
BLT_DEFINE_HID_DISCOVERY_START_READ_FIX_LEN(protocolMode)
BLT_DEFINE_HID_DISCOVERY_FOUND_CHAR(bootKeyboardInputReport)
BLT_DEFINE_HID_DISCOVERY_START_READ_FIX_LEN(bootKeyboardInputReport)
BLT_DEFINE_HID_DISCOVERY_FOUND_CHAR(bootKeyboardOutputReport)
BLT_DEFINE_HID_DISCOVERY_START_READ_FIX_LEN(bootKeyboardOutputReport)
BLT_DEFINE_HID_DISCOVERY_FOUND_CHAR(bootMouseInputReport)
BLT_DEFINE_HID_DISCOVERY_START_READ_FIX_LEN(bootMouseInputReport)
BLT_DEFINE_HID_DISCOVERY_FOUND_CHAR(HIDInformation)
BLT_DEFINE_HID_DISCOVERY_START_READ_FIX_LEN(HIDInformation)
BLT_DEFINE_HID_DISCOVERY_FOUND_CHAR(HIDControlPoint)
BLT_DEFINE_HID_DISCOVERY_FOUND_CHAR(reportMap)
BLT_DEFINE_HID_DISCOVERY_START_READ(reportMap)

static void blt_hidc_reportFoundChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    struct blc_hid_client *client = blt_hidc_getClientInst(connHandle);

    if (client == NULL) {
        BLT_HID_LOG("ERR: HID client control module is NULL. connHandle[0x%x]", connHandle);
        return;
    }

    if (client->reportCount >= HID_SUPPORT_REPORT_HANDLE_MAX) {
        BLT_HID_LOG("ERR: HID Report Count too many, greater than HID_SUPPORT_REPORT_HANDLE_MAX")
    }

    client->reportCharInfo[client->reportCount].attrHandle = valueHandle;
    client->reportCharInfo[client->reportCount].properties = properties;

    client->reportCount++;
    BLT_HID_LOG("report connHandle:0x%x properties:0x%x handle:0x%x ", connHandle, properties, valueHandle);
}

static void blt_hidc_reportStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)rdCbFunc; /*!< make compiler happy */

    struct blc_hid_client *client = blt_hidc_getClientInst(connHandle);

    if (client == NULL) {
        return;
    }

    for (int i = 0; i < HID_SUPPORT_REPORT_HANDLE_MAX; i++) {
        if (attrHandle == client->reportCharInfo[i].attrHandle) {
            *read        = client->reportCharInfo[i].report;
            *readLen     = &client->reportCharInfo[i].reportLen;
            *readMaxSize = sizeof(client->reportCharInfo[i].report);
            break;
        }
    }

    BLT_HID_LOG("report read info, attrHandle is 0x%x", attrHandle);
}

void blt_hidc_reportFoundDesc(u16 connHandle, uuid_t *uuid, u16 attrHandle)
{
    struct blc_hid_client *client = blt_hidc_getClientInst(connHandle);

    if (client == NULL || uuid == NULL || attrHandle == 0) {
        client->reportFoundDescCount++;
        return;
    }

    if (!blc_uuid_cmp(uuid, UUID16_DEF(DESCRIPTOR_UUID_CLIENT_CHARACTERISTIC_CONFIGURATION))) {
        client->reportCharInfo[client->reportFoundDescCount].cccHandle = attrHandle;
    }

    if (!blc_uuid_cmp(uuid, UUID16_DEF(DESCRIPTOR_UUID_REPORT_REFERENCE))) {
        client->reportCharInfo[client->reportFoundDescCount].reportReferenceHandle = attrHandle;
    }
}

static const blc_gapc_discService_t hidService = {
    .uuid = UUID16_INIT(SERVICE_UUID_HUMAN_INTERFACE_DEVICE),
    .sfun = blt_hidc_foundService,
};

static const blc_gapc_discChar_t hidChar[] = {
    BLT_HID_DISCOVERY_READ_CHAR(CHARACTERISTIC_UUID_PROTOCOL_MODE, protocolMode),
    BLT_HID_DISCOVERY_READ_NOTIFY_CHAR(CHARACTERISTIC_UUID_BOOT_KEYBOARD_INPUT_REPORT, bootKeyboardInputReport),
    BLT_HID_DISCOVERY_READ_CHAR(CHARACTERISTIC_UUID_BOOT_KEYBOARD_OUTPUT_REPORT, bootKeyboardOutputReport),
    BLT_HID_DISCOVERY_READ_NOTIFY_CHAR(CHARACTERISTIC_UUID_BOOT_MOUSE_INPUT_REPORT, bootMouseInputReport),
    BLT_HID_DISCOVERY_READ_CHAR(CHARACTERISTIC_UUID_HID_INFORMATION, HIDInformation),
    BLT_HID_DISCOVERY_WRITE_CHAR(CHARACTERISTIC_UUID_HID_CONTROL_POINT, HIDControlPoint),
    BLT_HID_DISCOVERY_READ_CHAR(CHARACTERISTIC_UUID_HID_REPORT_MAP, reportMap),
    {
                                                     .subscribeNtf = true,
                                                     .findDecs     = true,
                                                     .readValue    = true,
                                                     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_HID_REPORT),
                                                     .cfun         = blt_hidc_reportFoundChar,
                                                     .dfun         = blt_hidc_reportFoundDesc,
                                                     .rfun         = blt_hidc_reportStartRead,
                                                     },
};

static const blc_gapc_discList_t discHid = {
    .maxServiceCount = 1,
    .service         = &hidService,
    .includeTable    = {
                        .size = 0,
                        },
    .characteristicTable = {
                        .size           = ARRAY_SIZE(hidChar),
                        .characteristic = hidChar,
                        },
};

/***************************DIS sdp discovery end*******************************/

/**********reconnect function********/
BLT_BASIC_RECONNECT_SERVICE(hid, HID)
BLT_HID_RECONNECT_GET_INFO_READ(protocolMode)
BLT_HID_RECONNECT_GET_INFO_READ(bootKeyboardInputReport)
BLT_HID_RECONNECT_GET_INFO_READ(bootKeyboardOutputReport)
BLT_HID_RECONNECT_GET_INFO_READ(bootMouseInputReport)
BLT_HID_RECONNECT_GET_INFO_READ(HIDInformation)
BLT_HID_RECONNECT_GET_INFO_READ(reportMap)

static int blt_hidc_reportGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    struct blc_hid_client *client = blt_hidc_getClientInst(connHandle);

    if (client == NULL) {
        return 0;
    }

    for (int i = 0; i < client->reportCount; i++) {
        charInfo->properties  = client->reportCharInfo[i].properties;
        charInfo->valueHandle = client->reportCharInfo[i].attrHandle;
        charInfo->cccHandle   = client->reportCharInfo[i].cccHandle;
        charInfo++;
    }

    return client->reportCount;
}

static const blc_gapc_reconnChar_t reHidChar[] = {
    BLT_HID_RECONNECT_CHAR(protocolMode),
    BLT_HID_RECONNECT_CHAR(bootKeyboardInputReport),
    BLT_HID_RECONNECT_CHAR(bootKeyboardOutputReport),
    BLT_HID_RECONNECT_CHAR(bootMouseInputReport),
    BLT_HID_RECONNECT_CHAR(HIDInformation),
    BLT_HID_RECONNECT_CHAR(reportMap),
    BLT_HID_RECONNECT_CHAR(report),
};

static const blc_gapc_reconnList_t reconnHid = {
    .resfun = blt_hidc_recService,
    .charTb = {
               .size           = ARRAY_SIZE(reHidChar),
               .characteristic = reHidChar,
               },
    .inclSize = 0,
};

/**********reconnect function ending********/

/**********Write Characteristic Attribute Value*********/
int blc_hidc_writeProtocolModeWithout(u16 connHandle, u8 protocolMode)
{
    BLT_HID_WRITE_ATTR_VALUE_WITHOUT_RSP_FIX_LEN(protocolMode);
}

int blc_hidc_writeBootProtocolMode(u16 connHandle)
{
    return blc_hidc_writeProtocolMode(connHandle, BOOT_PROTOCOL_MODE);
}

int blc_hidc_writeReportProtocolMode(u16 connHandle)
{
    return blc_hidc_writeProtocolMode(connHandle, REPORT_PROTOCOL_MODE);
}
#if ((!defined(HOST_V2_ENABLE)))
int blc_hidc_writeBootKeyboardInput(u16 connHandle, hid_bootKeyboardInputValue_t *pBootKeyboardInputReport, prf_write_cb_t writeCb)
{
    hid_bootKeyboardInputValue_t bootKeyboardInputReport;
    memcpy(&bootKeyboardInputReport, pBootKeyboardInputReport, sizeof(hid_bootKeyboardInputValue_t));
    BLT_HID_WRITE_ATTR_VALUE_FIX_LEN(bootKeyboardInputReport);
}
#else
int blc_hidc_writeBootKeyboardInput(u16 connHandle, struct hid_bootKeyboardInputValue *pBootKeyboardInputReport, prf_write_cb_t writeCb)
{
    struct hid_bootKeyboardInputValue bootKeyboardInputReport;
    memcpy(&bootKeyboardInputReport, pBootKeyboardInputReport, sizeof(struct hid_bootKeyboardInputValue));
    BLT_HID_WRITE_ATTR_VALUE_FIX_LEN(bootKeyboardInputReport);
}
#endif
int blc_hidc_writeBootKeyboardIOutut(u16 connHandle, u16 bootKeyboardOutputReport, prf_write_cb_t writeCb)
{
    BLT_HID_WRITE_ATTR_VALUE_FIX_LEN(bootKeyboardOutputReport);
}

int blc_hidc_writeBootKeyboardIOututWithout(u16 connHandle, u16 bootKeyboardOutputReport)
{
    BLT_HID_WRITE_ATTR_VALUE_WITHOUT_RSP_FIX_LEN(bootKeyboardOutputReport);
}
#if ((!defined(HOST_V2_ENABLE)))
int blc_hidc_writeBootMouseInput(u16 connHandle, hid_bootMouseInputValue_t *pBootMouseInputReport, prf_write_cb_t writeCb)
{
    hid_bootMouseInputValue_t bootMouseInputReport;
    memcpy(&bootMouseInputReport, pBootMouseInputReport, sizeof(hid_bootMouseInputValue_t));
    BLT_HID_WRITE_ATTR_VALUE_FIX_LEN(bootMouseInputReport);
}
#else
int blc_hidc_writeBootMouseInput(u16 connHandle, struct hid_bootMouseInputValue *pBootMouseInputReport, prf_write_cb_t writeCb)
{
    struct hid_bootMouseInputValue bootMouseInputReport;
    memcpy(&bootMouseInputReport, pBootMouseInputReport, sizeof(struct hid_bootMouseInputValue));
    BLT_HID_WRITE_ATTR_VALUE_FIX_LEN(bootMouseInputReport);
}
#endif
int blc_hidc_writeHIDControlPointWithout(u16 connHandle, u8 HIDControlPoint)
{
    BLT_HID_WRITE_ATTR_VALUE_WITHOUT_RSP_FIX_LEN(HIDControlPoint);
}

int blc_hidc_writeEnterSuspend(u16 connHandle)
{
    return blc_hidc_writeHIDControlPointWithout(connHandle, HID_CONTROL_POINT_ENTER_SUSPEND);
}

int blc_hidc_writeExitSuspend(u16 connHandle)
{
    return blc_hidc_writeHIDControlPointWithout(connHandle, HID_CONTROL_POINT_EXIT_SUSPEND);
}

static int blc_hidc_writeReportValue(u16 connHandle, struct hid_reportReferenceValue *reference, u8 *value, u16 valueLen, prf_write_cb_t writeCb, bool withoutRsp)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        BLT_HID_LOG("ERR: ACL handle invalid"
                    "\n");
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    struct blc_hid_client *client = blt_hidc_getClientInst(connHandle);

    for (int i = 0; i < HID_SUPPORT_REPORT_HANDLE_MAX; i++) {
        if (client->reportCharInfo[i].attrHandle && client->reportCharInfo[i].reportReferenceHandle &&
            client->reportCharInfo[i].reportReferenceValue.reportId == reference->reportId &&
            client->reportCharInfo[i].reportReferenceValue.reportType == reference->reportType) {
            if ((withoutRsp && (client->reportCharInfo[i].properties & CHAR_PROP_WRITE_WITHOUT_RSP)) ||
                (!withoutRsp && (client->reportCharInfo[i].properties & CHAR_PROP_WRITE))) {
                return PRF_ERR_INVALID_PARAMETER;
            }
            gapc_write_cfg_t pGapWrCfg;
            pGapWrCfg.func       = blc_prf_writeAttributeValueDefaultCallback;
            pGapWrCfg.handle     = client->protocolModeHdl;
            pGapWrCfg.data       = value;
            pGapWrCfg.length     = valueLen;
            pGapWrCfg.withoutRsp = withoutRsp;
            pGapWrCfg.cbData     = NULL;
            return blc_prf_writeAttributeValue(connHandle, &pGapWrCfg, writeCb);
        }
    }

    BLT_HID_LOG("ERR: handle not set"
                "\n");
    return PRF_ERR_INVALID_ATTR_HANDLE;
}

int blc_hidc_writeReport(u16 connHandle, struct hid_reportReferenceValue *reference, u8 *value, u16 valueLen, prf_write_cb_t writeCb)
{
    return blc_hidc_writeReportValue(connHandle, reference, value, valueLen, writeCb, false);
}

int blc_hidc_writeReportWithout(u16 connHandle, struct hid_reportReferenceValue *reference, u8 *value, u16 valueLen)
{
    return blc_hidc_writeReportValue(connHandle, reference, value, valueLen, NULL, true);
}

int blc_hidc_writeReportInput(u16 connHandle, u8 reportId, u8 *value, u16 valueLen, prf_write_cb_t writeCb)
{
    struct hid_reportReferenceValue reference = {
        .reportId   = reportId,
        .reportType = HID_REPORT_TYPE_INPUT,
    };
    return blc_hidc_writeReport(connHandle, &reference, value, valueLen, writeCb);
}

int blc_hidc_writeReportOutput(u16 connHandle, u8 reportId, u8 *value, u16 valueLen, prf_write_cb_t writeCb)
{
    struct hid_reportReferenceValue reference = {
        .reportId   = reportId,
        .reportType = HID_REPORT_TYPE_OUTPUT,
    };
    return blc_hidc_writeReport(connHandle, &reference, value, valueLen, writeCb);
}

int blc_hidc_writeReportOutputWithout(u16 connHandle, u8 reportId, u8 *value, u16 valueLen)
{
    struct hid_reportReferenceValue reference = {
        .reportId   = reportId,
        .reportType = HID_REPORT_TYPE_OUTPUT,
    };
    return blc_hidc_writeReport(connHandle, &reference, value, valueLen, NULL);
}

int blc_hidc_writeReportFeature(u16 connHandle, u8 reportId, u8 *value, u16 valueLen, prf_write_cb_t writeCb)
{
    struct hid_reportReferenceValue reference = {
        .reportId   = reportId,
        .reportType = HID_REPORT_TYPE_FEATURE,
    };
    return blc_hidc_writeReport(connHandle, &reference, value, valueLen, writeCb);
}

/**********Write Characteristic Attribute Value End*********/

/**********Read Characteristic Attribute Value*********/
int blc_hidc_readProtocolMode(u16 connHandle, prf_read_cb_t readCb)
{
    BLT_HID_READ_ATTR_VALUE_FIX_LEN(protocolMode);
}

int blc_hidc_readReportMap(u16 connHandle, prf_read_cb_t readCb)
{
    BLT_HID_READ_ATTR_VALUE(reportMap);
}

int blc_hidc_readBootKeyboardInputReport(u16 connHandle, prf_read_cb_t readCb)
{
    BLT_HID_READ_ATTR_VALUE_FIX_LEN(bootKeyboardInputReport);
}

int blc_hidc_readBootKeyboardOutputReport(u16 connHandle, prf_read_cb_t readCb)
{
    BLT_HID_READ_ATTR_VALUE_FIX_LEN(bootKeyboardOutputReport);
}

int blc_hidc_readBootMouseInputReport(u16 connHandle, prf_read_cb_t readCb)
{
    BLT_HID_READ_ATTR_VALUE_FIX_LEN(bootMouseInputReport);
}

int blc_hidc_readHIDInformation(u16 connHandle, prf_read_cb_t readCb)
{
    BLT_HID_READ_ATTR_VALUE_FIX_LEN(HIDInformation);
}

int blc_hidc_readReport(u16 connHandle, struct hid_reportReferenceValue *reference, prf_read_cb_t readCb)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        BLT_HID_LOG("ERR: ACL handle invalid"
                    "\n");
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    struct blc_hid_client *client = blt_hidc_getClientInst(connHandle);

    for (int i = 0; i < HID_SUPPORT_REPORT_HANDLE_MAX; i++) {
        if (client->reportCharInfo[i].attrHandle && client->reportCharInfo[i].reportReferenceHandle &&
            client->reportCharInfo[i].reportReferenceValue.reportId == reference->reportId &&
            client->reportCharInfo[i].reportReferenceValue.reportType == reference->reportType) {
            gapc_read_cfg_t pGapReCfg;
            pGapReCfg.func     = blc_prf_readAttributeValueDefaultCallback;
            pGapReCfg.handle   = client->reportCharInfo[i].attrHandle;
            pGapReCfg.wBuff    = (u8 *)&client->reportCharInfo[i].report[0];
            pGapReCfg.wBuffLen = &client->reportCharInfo[i].reportLen;
            pGapReCfg.maxLen   = sizeof(client->reportCharInfo[i].report);

            return blc_prf_readAttributeValue(connHandle, &pGapReCfg, readCb);
        }
    }

    BLT_HID_LOG("ERR: handle not set"
                "\n");
    return PRF_ERR_INVALID_ATTR_HANDLE;
}

int blc_hidc_readReportInput(u16 connHandle, u8 reportId, prf_read_cb_t readCb)
{
    struct hid_reportReferenceValue reference = {
        .reportId   = reportId,
        .reportType = HID_REPORT_TYPE_INPUT,
    };
    return blc_hidc_readReport(connHandle, &reference, readCb);
}

int blc_hidc_readReportOutput(u16 connHandle, u8 reportId, prf_read_cb_t readCb)
{
    struct hid_reportReferenceValue reference = {
        .reportId   = reportId,
        .reportType = HID_REPORT_TYPE_OUTPUT,
    };
    return blc_hidc_readReport(connHandle, &reference, readCb);
}

int blc_hidc_readReportFeature(u16 connHandle, u8 reportId, prf_read_cb_t readCb)
{
    struct hid_reportReferenceValue reference = {
        .reportId   = reportId,
        .reportType = HID_REPORT_TYPE_FEATURE,
    };
    return blc_hidc_readReport(connHandle, &reference, readCb);
}

/**********Read Characteristic Attribute Value End*********/

/**********Get Characteristic Attribute Value*********/
int blc_hidc_getProtocolMode(u16 connHandle, u8 *protocolMode)
{
    BLT_HID_GET_ATTR_VALUE_FIX_LEN(protocolMode);
}

int blc_hidc_getReportMap(u16 connHandle, u8 *reportMap, u16 *reportMapLen)
{
    BLT_HID_GET_ATTR_VALUE(reportMap);
}

#if ((!defined(HOST_V2_ENABLE)))
int blc_hidc_getBootKeyboardInputReport(u16 connHandle, hid_bootKeyboardInputValue_t *bootKeyboardInputReport)
{
    BLT_HID_GET_ATTR_VALUE_FIX_LEN(bootKeyboardInputReport);
}
#else
int blc_hidc_getBootKeyboardInputReport(u16 connHandle, struct hid_bootKeyboardInputValue *bootKeyboardInputReport)
{
    BLT_HID_GET_ATTR_VALUE_FIX_LEN(bootKeyboardInputReport);
}
#endif

int blc_hidc_getBootKeyboardOutputReport(u16 connHandle, u16 *bootKeyboardOutputReport)
{
    BLT_HID_GET_ATTR_VALUE_FIX_LEN(bootKeyboardOutputReport);
}

#if ((!defined(HOST_V2_ENABLE)))
int blc_hidc_getBootMouseInputReport(u16 connHandle, hid_bootMouseInputValue_t *bootMouseInputReport)
{
    BLT_HID_GET_ATTR_VALUE_FIX_LEN(bootMouseInputReport);
}

int blc_hidc_getHIDInformation(u16 connHandle, hid_hidInformationVale_t *HIDInformation)
{
    BLT_HID_GET_ATTR_VALUE_FIX_LEN(HIDInformation);
}
#else
int blc_hidc_getBootMouseInputReport(u16 connHandle, struct hid_bootMouseInputValue *bootMouseInputReport)
{
    BLT_HID_GET_ATTR_VALUE_FIX_LEN(bootMouseInputReport);
}

int blc_hidc_getHIDInformation(u16 connHandle, struct hid_hidInformationVale *HIDInformation)
{
    BLT_HID_GET_ATTR_VALUE_FIX_LEN(HIDInformation);
}
#endif

int blc_hidc_getReport(u16 connHandle, struct hid_reportReferenceValue *reference, u8 *report, u16 *reportLen)
{
    if (report == NULL || reportLen == NULL) {
        return PRF_COMMON_ERR_INPUT_NULL;
    }

    if (blt_ll_isAclHandleOutOfRange(connHandle)) {
        return PRF_HCI_ERROR_FLAG + HCI_ERR_UNKNOWN_CONN_ID;
    }

    struct blc_hid_client *client = blt_hidc_getClientInst(connHandle);

    if (client == NULL) {
        return PRF_COMMON_ERR_CTRL_MODULE_NOT_FOUND;
    }

    for (int i = 0; i < HID_SUPPORT_REPORT_HANDLE_MAX; i++) {
        if (client->reportCharInfo[i].attrHandle && client->reportCharInfo[i].reportReferenceHandle &&
            client->reportCharInfo[i].reportReferenceValue.reportId == reference->reportId &&
            client->reportCharInfo[i].reportReferenceValue.reportType == reference->reportType) {
            memcpy(report, &client->reportCharInfo[i].report, client->reportCharInfo[i].reportLen);
            *reportLen = client->reportCharInfo[i].reportLen;
            return PRF_COMMON_SUCC;
        }
    }

    return PRF_COMMON_ERR_ATTR_HANDLE_NOT_FOUND;
}

int blc_hidc_getReportInput(u16 connHandle, u8 reportId, u8 *report, u16 *reportLen)
{
    struct hid_reportReferenceValue reference = {
        .reportId   = reportId,
        .reportType = HID_REPORT_TYPE_INPUT,
    };
    return blc_hidc_getReport(connHandle, &reference, report, reportLen);
}

int blc_hidc_getReportOutput(u16 connHandle, u8 reportId, u8 *report, u16 *reportLen)
{
    struct hid_reportReferenceValue reference = {
        .reportId   = reportId,
        .reportType = HID_REPORT_TYPE_OUTPUT,
    };
    return blc_hidc_getReport(connHandle, &reference, report, reportLen);
}

int blc_hidc_getReportFeature(u16 connHandle, u8 reportId, u8 *report, u16 *reportLen)
{
    struct hid_reportReferenceValue reference = {
        .reportId   = reportId,
        .reportType = HID_REPORT_TYPE_FEATURE,
    };
    return blc_hidc_getReport(connHandle, &reference, report, reportLen);
}

/**********Get Characteristic Attribute Value*********/

int blc_usb_setProtocol(u16 connHandle, u8 protocolMode)
{
    return blc_hidc_writeProtocolMode(connHandle, protocolMode);
}

int blc_usb_setBootProtocolMode(u16 connHandle)
{
    return blc_hidc_writeProtocolMode(connHandle, BOOT_PROTOCOL_MODE);
}

int blc_usb_setReportProtocolMode(u16 connHandle)
{
    return blc_hidc_writeProtocolMode(connHandle, REPORT_PROTOCOL_MODE);
}

int blc_usb_getProtocol(u16 connHandle, prf_read_cb_t readCb)
{
    return blc_hidc_readProtocolMode(connHandle, readCb);
}

int blc_usb_getReport(u16 connHandle, struct hid_reportReferenceValue *reference, prf_read_cb_t readCb)
{
    return blc_hidc_readReport(connHandle, reference, readCb);
}

int blc_usb_getReportInput(u16 connHandle, u8 reportId, prf_read_cb_t readCb)
{
    return blc_hidc_readReportInput(connHandle, reportId, readCb);
}

int blc_usb_getReportOutput(u16 connHandle, u8 reportId, prf_read_cb_t readCb)
{
    return blc_hidc_readReportOutput(connHandle, reportId, readCb);
}

int blc_usb_getReportFeature(u16 connHandle, u8 reportId, prf_read_cb_t readCb)
{
    return blc_hidc_readReportFeature(connHandle, reportId, readCb);
}

int blc_usb_setReport(u16 connHandle, struct hid_reportReferenceValue *reference, u8 *value, u16 valueLen, prf_write_cb_t writeCb)
{
    return blc_hidc_writeReport(connHandle, reference, value, valueLen, writeCb);
}

int blc_usb_setReportOutput(u16 connHandle, u8 reportId, u8 *value, u16 valueLen, prf_write_cb_t writeCb)
{
    return blc_hidc_writeReportOutput(connHandle, reportId, value, valueLen, writeCb);
}

int blc_usb_dataOut(u16 connHandle, u8 reportId, u8 *value, u16 valueLen)
{
    return blc_hidc_writeReportOutputWithout(connHandle, reportId, value, valueLen);
}

int blc_usb_setReportFeature(u16 connHandle, u8 reportId, u8 *value, u16 valueLen, prf_write_cb_t writeCb)
{
    return blc_hidc_writeReportFeature(connHandle, reportId, value, valueLen, writeCb);
}
