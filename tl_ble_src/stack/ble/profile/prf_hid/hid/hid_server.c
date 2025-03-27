/********************************************************************************************************
 * @file    hid_server.c
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

#include "hid_internal.h"
#include "hid_server_buf.h"

static int blt_hids_init(u8 initType, const void* param);
static int blt_hids_connect(u16 connHandle, prf_acl_state_enum connState);
static void blt_hids_serviceInit(const struct blc_hids_regParam* param);
static int blt_hids_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8* writeValue, u16 valueLen);
static void blt_hids_setProtocolMode(u8 protocolMode);

_attribute_ble_data_retention_
struct blc_hid_server_ctrl hid_server_ctrl = {
    .process = {
        .pNext = NULL,
        .id = HID_SERVER,
        .usedAclRole = 0,
        .init = blt_hids_init,
        .connect = blt_hids_connect,
        .discov = NULL,
        .loop = NULL,
    },
};

void blc_hid_registerHIDControlServer(const struct blc_hids_regParam *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_PERIPHERAL, (blc_prf_proc_t*)&hid_server_ctrl, param);
}

static int blt_hids_init(u8 initType, const void* param)
{
#if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(struct blc_hid_server_ctrl)), blc_hid_server_ctrl);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(struct blc_hid_server)), blc_hid_server);
#endif

    if(initType == PRF_PROC_INIT) {
        BLT_HID_LOG("Server init");
        blc_svc_addHidGroup();
        blt_hids_serviceInit(param);
        blc_svc_hidCbackRegister(NULL, blt_hids_writeCback);
    }
//  else if (initType == PRF_PROC_DEINIT) {
//      blc_svc_removeHidGroup();
//      BLT_HID_LOG("Server deinit");
//  }
    return 0;
}

static int blt_hids_connect(u16 connHandle, prf_acl_state_enum connState)
{
    (void)connHandle;
    if(connState == PRF_ACL_STATE_DISCONN) {
        blt_hids_setProtocolMode(HID_PROTOCOL_MODE_REPORT);
    }

    return 0;
}

static struct blc_hid_server* blt_hids_getCtrl(u16 connHandle)
{
    (void)connHandle;
    return &hid_server_ctrl.hidServer;
}

#define HIDS_PROTOCOL_MODE_HANDLE(connHandle)               (blt_hids_getCtrl(connHandle)->protocolModeHdl)
#define HIDS_BOOT_KEYBOARD_INPUT_REPORT_HANDLE(connHandle)  (blt_hids_getCtrl(connHandle)->bootKeyboardInputReportHdl)
#define HIDS_BOOT_KEYBOARD_OUTPUT_REPORT_HANDLE(connHandle) (blt_hids_getCtrl(connHandle)->bootKeyboardOutputReportHdl)
#define HIDS_BOOT_MOUSE_INPUT_REPORT_HANDLE(connHandle)     (blt_hids_getCtrl(connHandle)->bootMouseInputReportHdl)
#define HIDS_HID_INFORMATION_HANDLE(connHandle)             (blt_hids_getCtrl(connHandle)->HIDInformationHdl)
#define HIDS_HID_CONTROL_POINT_HANDLE(connHandle)           (blt_hids_getCtrl(connHandle)->HIDControlPointHdl)
#define HIDS_REPORT_MAP_HANDLE(connHandle)                  (blt_hids_getCtrl(connHandle)->reportMapHdl)


BLT_HID_SERVER_INIT_HANDLE(protocolMode)
BLT_HID_SERVER_INIT_HANDLE(bootKeyboardInputReport)
BLT_HID_SERVER_INIT_HANDLE(bootKeyboardOutputReport)
BLT_HID_SERVER_INIT_HANDLE(bootMouseInputReport)
BLT_HID_SERVER_INIT_HANDLE(HIDInformation)
BLT_HID_SERVER_INIT_HANDLE(HIDControlPoint)
BLT_HID_SERVER_INIT_HANDLE(reportMap)

static void blt_hids_reportInitChar(atts_foundCharParam_t * p, void *input)
{
    struct blc_hid_server *server = (struct blc_hid_server*)input;
    if(p->num > HID_SUPPORT_REPORT_HANDLE_MAX)
    {
        BLT_HID_LOG("ERR: report char too many");
        return ;
    }
    server->reportCharInfo[p->num].attrHandle = p->charHandle;
    struct hid_reportReferenceValue *reference = (struct hid_reportReferenceValue*)blc_gatts_getReportReferenceValue(PRF_RFU_CONN_HANDLE, p->charHandle);

    if(reference)
    {
        server->reportCharInfo[p->num].reportReferenceValue.reportId = reference->reportId;
        server->reportCharInfo[p->num].reportReferenceValue.reportType = reference->reportType;
    }
}

static const atts_findCharList_t hidsChar[] = {
    BLT_HID_SERVER_FIND_CHAR(protocolMode, characteristicProtocolModeUuid),
    BLT_HID_SERVER_FIND_CHAR(bootKeyboardInputReport, characteristicBootKeyboardInputReportUuid),
    BLT_HID_SERVER_FIND_CHAR(bootKeyboardOutputReport, characteristicBootKeyboardOutputReportUuid),
    BLT_HID_SERVER_FIND_CHAR(bootMouseInputReport, characteristicBootMouseInputReportUuid),
    BLT_HID_SERVER_FIND_CHAR(HIDInformation, characteristicHidInformationUuid),
    BLT_HID_SERVER_FIND_CHAR(HIDControlPoint, characteristicHidControlPointUuid),
    BLT_HID_SERVER_FIND_CHAR(reportMap, characteristicReportMapUuid),
    BLT_HID_SERVER_FIND_CHAR(report, characteristicReportUuid),
};

static void blt_hids_serviceInit(const struct blc_hids_regParam* param)
{
    (void)param;
    struct blc_hid_server *server = blt_hids_getCtrl(PRF_RFU_CONN_HANDLE);
    blc_atts_findCharacteristicByServiceUuid(serviceHumanInterfaceDeviceUuid, ATT_16_UUID_LEN, hidsChar, ARRAY_SIZE(hidsChar), server);

    BLT_HID_LOG("Handle information, Protocol Mode:0x%x, report Map:0x%x, HID Information:0x%x, HID Control Point:0x%x",
            server->protocolModeHdl, server->reportMapHdl, server->HIDInformationHdl, server->HIDControlPointHdl);
    BLT_HID_LOG("Boot Keyboard Input Report:0x%x, Boot Keyboard Output Report:0x%x, Boot Mouse Input Report:0x%x",
            server->bootKeyboardInputReportHdl, server->bootKeyboardOutputReportHdl, server->bootMouseInputReportHdl);

}

static u8* blt_hids_getProtocolMode(u16 connHandle)
{
    return blc_gatts_getAttributeValueByHandle(connHandle, HIDS_PROTOCOL_MODE_HANDLE(connHandle));
}

static void blt_hids_setProtocolMode(u8 protocolMode)
{
    u8 *pProtocolMode = blt_hids_getProtocolMode(PRF_RFU_CONN_HANDLE);
    if(!pProtocolMode)      return ;

    *pProtocolMode = protocolMode;
}

static hid_bootKeyboardInputValue_t* blt_hids_getBootKeyboardInput(u16 connHandle)
{
    return (hid_bootKeyboardInputValue_t*)blc_gatts_getAttributeValueByHandle(connHandle, HIDS_BOOT_KEYBOARD_INPUT_REPORT_HANDLE(connHandle));
}

void blc_hids_setBootKeyboardInput(hid_bootKeyboardInputValue_t* value)
{
    hid_bootKeyboardInputValue_t *pValue = blt_hids_getBootKeyboardInput(PRF_RFU_CONN_HANDLE);
    if(!pValue)     return ;

    memcpy(pValue, value, sizeof(hid_bootKeyboardInputValue_t));
}

int blc_hids_notifyBootKeyboardInput(u16 connHandle, hid_bootKeyboardInputValue_t* value)
{
    blc_hids_setBootKeyboardInput(value);
    return blc_gatts_notifyAttr(connHandle, HIDS_BOOT_KEYBOARD_INPUT_REPORT_HANDLE(connHandle));
}

static hid_bootMouseInputValue_t* blt_hids_getBootMouseInput(u16 connHandle)
{
    return (hid_bootMouseInputValue_t*)blc_gatts_getAttributeValueByHandle(connHandle, HIDS_BOOT_MOUSE_INPUT_REPORT_HANDLE(connHandle));
}

void blc_hids_setBootMouseInput(hid_bootMouseInputValue_t* value)
{
    hid_bootMouseInputValue_t *pValue = blt_hids_getBootMouseInput(PRF_RFU_CONN_HANDLE);
    if(!pValue)     return ;

    memcpy(pValue, value, sizeof(hid_bootMouseInputValue_t));
}

int blc_hids_notifyBootMouseInput(u16 connHandle, hid_bootMouseInputValue_t* value)
{
    blc_hids_setBootMouseInput(value);
    return blc_gatts_notifyAttr(connHandle, HIDS_BOOT_MOUSE_INPUT_REPORT_HANDLE(connHandle));
}

int blc_hids_notifyInputReport(u16 connHandle, u8 reportID, u8* value, u16 valueLen)
{
    if(value == NULL || valueLen == 0)
    {
        return PRF_ERR_INVALID_PARAMETER;
    }

    struct blc_hid_server *hids = blt_hids_getCtrl(connHandle);

    for(int i=0; i<HID_SUPPORT_REPORT_HANDLE_MAX; i++)
    {
        if(hids->reportCharInfo[i].attrHandle && hids->reportCharInfo[i].reportReferenceValue.reportId == reportID &&
                hids->reportCharInfo[i].reportReferenceValue.reportType == HID_REPORT_TYPE_INPUT)
        {
            return blc_gatts_notifyValue(connHandle, hids->reportCharInfo[i].attrHandle, value, valueLen);
        }
    }

    return PRF_ERR_INVALID_ATTR_HANDLE;
}

static int blt_hids_writeProtocolMode(u16 connHandle, u8* writeValue, u16 valueLen)
{
    if(valueLen != 1)
    {
        return ATT_ERR_INVALID_PDU;
    }

    blt_hids_setProtocolMode(*writeValue);

    struct blc_hids_protocolModeChangeEvt evt = {
        .protocolMode = *writeValue,
    };
    blt_prf_sendEvent(connHandle, HIDS_EVT_PROTOCOL_MODE_CHANGE, &evt, sizeof(struct blc_hids_protocolModeChangeEvt));
    return ATT_SUCCESS;
}

static int blt_hids_writeBootKeyboardInput(u16 connHandle, u8* writeValue, u16 valueLen)
{
    struct blc_hids_recvBootKeyboardInputReportEvt evt = {
        .value = writeValue,
        .len = valueLen
    };
    blt_prf_sendEvent(connHandle, HIDS_EVT_RECV_BOOT_KEYBOARD_INPUT_REPORT, &evt, sizeof(struct blc_hids_recvBootKeyboardInputReportEvt));
    return ATT_SUCCESS;
}

static int blt_hids_writeBootKeyboardOutput(u16 connHandle, u8* writeValue, u16 valueLen)
{
    struct blc_hids_recvBootKeyboardOutputReportEvt evt = {
        .value = writeValue,
        .len = valueLen
    };
    blt_prf_sendEvent(connHandle, HIDS_EVT_RECV_BOOT_KEYBOARD_OUTPUT_REPORT, &evt, sizeof(struct blc_hids_recvBootKeyboardOutputReportEvt));
    return ATT_SUCCESS;
}

static int blt_hids_writeBootMouseInput(u16 connHandle, u8* writeValue, u16 valueLen)
{
    struct blc_hids_recvBootMouseInputReportEvt evt = {
        .value = writeValue,
        .len = valueLen
    };
    blt_prf_sendEvent(connHandle, HIDS_EVT_RECV_BOOT_MOUSE_INPUT_REPORT, &evt, sizeof(struct blc_hids_recvBootMouseInputReportEvt));
    return ATT_SUCCESS;
}

static int blt_hids_writeHIDControlPoint(u16 connHandle, u8* writeValue, u16 valueLen)
{
    if(valueLen != 1)
    {
        return ATT_ERR_INVALID_PDU;
    }

    if(*writeValue == HID_CONTROL_POINT_ENTER_SUSPEND)
    {
        blt_prf_sendEvent(connHandle, HIDS_EVT_ENTER_SUSPEND_STATE, NULL, 0);
    }
    else if(*writeValue == HID_CONTROL_POINT_EXIT_SUSPEND)
    {
        blt_prf_sendEvent(connHandle, HIDS_EVT_EXIT_SUSPEND_STATE, NULL, 0);
    }

    return ATT_SUCCESS;
}

static int blt_hids_writeReport(u16 connHandle, struct hid_reportReferenceValue* reference, u8* writeValue, u16 valueLen)
{
    struct blc_hids_recvReportEvt evt = {
        .reportId = reference->reportId,
        .reportType = reference->reportType,
        .value = writeValue,
        .len = valueLen
    };
    blt_prf_sendEvent(connHandle, HIDS_EVT_RECV_REPORT, &evt, sizeof(struct blc_hids_recvReportEvt));
    return ATT_SUCCESS;
}

static int blt_hids_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8* writeValue, u16 valueLen)
{
    (void)opcode;

    BLT_HID_LOG("Write ConnHandle:0x%x, attrHandle:0x%x, value is %s", connHandle, attrHandle, hex_to_str(writeValue, valueLen));

    struct blc_hid_server *hids = blt_hids_getCtrl(connHandle);

    if(hids->protocolModeHdl == attrHandle)
    {
        return blt_hids_writeProtocolMode(connHandle, writeValue, valueLen);
    }
    else if(hids->bootKeyboardInputReportHdl == attrHandle)
    {
        return blt_hids_writeBootKeyboardInput(connHandle, writeValue, valueLen);
    }
    else if(hids->bootKeyboardOutputReportHdl == attrHandle)
    {
        return blt_hids_writeBootKeyboardOutput(connHandle, writeValue, valueLen);
    }
    else if(hids->bootMouseInputReportHdl == attrHandle)
    {
        return blt_hids_writeBootMouseInput(connHandle, writeValue, valueLen);
    }
    else if(hids->HIDControlPointHdl == attrHandle)
    {
        return blt_hids_writeHIDControlPoint(connHandle, writeValue, valueLen);
    }

    for(int i=0; i<HID_SUPPORT_REPORT_HANDLE_MAX; i++)
    {
        if(hids->reportCharInfo[i].attrHandle == attrHandle)
        {
            return blt_hids_writeReport(connHandle, &hids->reportCharInfo[i].reportReferenceValue, writeValue, valueLen);
        }
    }

    return ATT_SUCCESS;
}


