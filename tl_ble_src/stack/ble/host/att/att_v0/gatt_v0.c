/********************************************************************************************************
 * @file    gatt_v0.c
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


_attribute_ble_data_retention_ gatt_handler_t gatt_data_handler = NULL;

static const u8 att_access_err_no_enc[4][4] = {
    {
     ATT_SUCCESS,
     ATT_SUCCESS,
     ATT_SUCCESS,
     ATT_SUCCESS,
     },
    {
     ATT_ERR_INSUFFICIENT_AUTH,
     ATT_ERR_INSUFFICIENT_ENCRYPT,
     ATT_ERR_INSUFFICIENT_ENCRYPT,
     ATT_ERR_INSUFFICIENT_ENCRYPT,
     },
    {
     ATT_ERR_INSUFFICIENT_AUTH,
     ATT_ERR_INSUFFICIENT_ENCRYPT,
     ATT_ERR_INSUFFICIENT_ENCRYPT,
     ATT_ERR_INSUFFICIENT_ENCRYPT,
     },
    {
     ATT_ERR_INSUFFICIENT_AUTH,
     ATT_ERR_INSUFFICIENT_ENCRYPT,
     ATT_ERR_INSUFFICIENT_ENCRYPT,
     ATT_ERR_INSUFFICIENT_ENCRYPT,
     },
};

static const u8 att_access_err_enc[4][4] = {
    {
     ATT_SUCCESS,
     ATT_SUCCESS,
     ATT_SUCCESS,
     ATT_SUCCESS,
     },
    {
     ATT_SUCCESS,
     ATT_SUCCESS,
     ATT_SUCCESS,
     ATT_SUCCESS,
     },
    {
     ATT_SUCCESS,
     ATT_ERR_INSUFFICIENT_AUTH,
     ATT_SUCCESS,
     ATT_SUCCESS,
     },
    {
     ATT_SUCCESS,
     ATT_ERR_INSUFFICIENT_AUTH,
     ATT_ERR_INSUFFICIENT_AUTH,
     ATT_SUCCESS,
     },
};

u8 blt_gatt_requestServiceAccess(u16 connHandle, int gatt_perm)
{
    u8 access_require = 0;
    if (gatt_perm & ATT_PERMISSIONS_SECURE_CONN) {
        access_require = 3;
    } else if (gatt_perm & ATT_PERMISSIONS_AUTHEN) {
        access_require = 2;
    } else if (gatt_perm & ATT_PERMISSIONS_ENCRYPT) {
        access_require = 1;
    } else {
        return ATT_SUCCESS;
    }

    u8 pairing_status = (u8)blt_smp_get_pairing_status(connHandle);

    if (blt_llms_isConnectionEncrypted(connHandle)) {
        return att_access_err_enc[access_require][pairing_status];
    } else {
        return att_access_err_no_enc[access_require][pairing_status];
    }
}

void blc_gatt_register_data_handler(gatt_handler_t handler)
{
    gatt_data_handler = handler;
}

ble_sts_t blc_gatt_pushHandleValueNotify(u16 connHandle, u16 attHandle, u8 *p, int len)
{
    gap_ms_para_t *pGap_ms_para = blc_gap_getMasterSlavePara(connHandle);

    if (pGap_ms_para == NULL) {
        return GAP_ERR_INVALID_PARAMETER;
    }

#if SMP_REAL_ENCRYPTION_BUSY_ENABLE
    if (!real_encryption_busy_enable && blc_smp_isPairingBusy(connHandle)) {
        return SMP_ERR_PAIRING_BUSY;
    }
#else
    if (blc_smp_isPairingBusy(connHandle)) {
        return SMP_ERR_PAIRING_BUSY;
    }
#endif
    //If the attribute value is longer than (ATT_MTU-3) octets, peer device's host can not receive whole data
    //e.g. MTU=23, HandValueNotify format_len=3(opcode, attHandle), 20 bytes max
    else if (len > pGap_ms_para->effective_MTU - 3) {
        return GATT_ERR_DATA_LENGTH_EXCEED_MTU_SIZE;
    } else {
        if (pGap_ms_para->att_service_discover_tick) {
            if (clock_time_exceed(pGap_ms_para->att_service_discover_tick, pGap_ms_para->data_pending_time * 10000)) { //300 * 1000
                pGap_ms_para->att_service_discover_tick = 0;
            } else {
                return GATT_ERR_DATA_PENDING_DUE_TO_SERVICE_DISCOVERY_BUSY;
            }
        }
    }

#if (ATT_HANDLE_VALUE_CUSTOM_EN)
    if (bltAtt.attHl_custom_en) {
        attHandle = blt_att_change_sdkAttHandle_to_customAttHandle(attHandle);
    }
#endif

    //HandleValueNotify format
    u8 format[4];
    format[0] = ATT_OP_HANDLE_VALUE_NOTI;
    format[1] = U16_LO(attHandle);
    format[2] = U16_HI(attHandle);

    return blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 3, p, len);
}

///indicate need to wait confirm
ble_sts_t blc_gatt_pushHandleValueIndicate(u16 connHandle, u16 attHandle, u8 *p, int len)
{
    gap_ms_para_t *pGap_ms_para = blc_gap_getMasterSlavePara(connHandle);

    if (pGap_ms_para == NULL) {
        return GAP_ERR_INVALID_PARAMETER;
    }

#if SMP_REAL_ENCRYPTION_BUSY_ENABLE
    if (!real_encryption_busy_enable && blc_smp_isPairingBusy(connHandle)) {
        return SMP_ERR_PAIRING_BUSY;
    }
#else
    if (blc_smp_isPairingBusy(connHandle)) {
        return SMP_ERR_PAIRING_BUSY;
    }
#endif
    //If the attribute value is longer than (ATT_MTU-3) octets, peer device's host can not receive whole data
    //e.g. MTU=23, HandValueNotify format_len=3(opcode, attHandle), 20 bytes max
    else if (len > pGap_ms_para->effective_MTU - 3) {
        return GATT_ERR_DATA_LENGTH_EXCEED_MTU_SIZE;
    } else if (pGap_ms_para->indicate_handle) { ///slave use indicate, in case of master use it. so not use variable blms_host_slave
        return GATT_ERR_PREVIOUS_INDICATE_DATA_HAS_NOT_CONFIRMED;
    } else {
        if (pGap_ms_para->att_service_discover_tick) {
            if (clock_time_exceed(pGap_ms_para->att_service_discover_tick, pGap_ms_para->data_pending_time * 10000)) { //300 * 1000
                pGap_ms_para->att_service_discover_tick = 0;
            } else {
                return GATT_ERR_DATA_PENDING_DUE_TO_SERVICE_DISCOVERY_BUSY;
            }
        }
    }


#if (ATT_HANDLE_VALUE_CUSTOM_EN)
    if (bltAtt.attHl_custom_en) {
        attHandle = blt_att_change_sdkAttHandle_to_customAttHandle(attHandle);
    }
#endif


    //HandleValueIndicate format
    u8 format[4];
    format[0] = ATT_OP_HANDLE_VALUE_IND;
    format[1] = U16_LO(attHandle);
    format[2] = U16_HI(attHandle);

    u8 api_status = (u8)blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 3, p, len);
    if (api_status == BLE_SUCCESS) {
        pGap_ms_para->indicate_handle = attHandle;
    }

    return api_status;
}

ble_sts_t blc_gatt_pushPrepareWriteRequest(u16 connHandle, u16 attHandle, u16 offset, u8 *p, int len)
{
    if (p == NULL || len == 0) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    gap_ms_para_t *pGap_ms_para = blc_gap_getMasterSlavePara(connHandle);
    if (len > (pGap_ms_para->effective_MTU - 5)) {
        return GATT_ERR_INVALID_PARAMETER;
    }

#if (ATT_HANDLE_VALUE_CUSTOM_EN)
    if (bltAtt.attHl_custom_en) {
        attHandle = blt_att_change_sdkAttHandle_to_customAttHandle(attHandle);
    }
#endif

    //PrepareWriteRequest format
    u8 format[5];
    format[0] = ATT_OP_PREPARE_WRITE_REQ;
    format[1] = U16_LO(attHandle);
    format[2] = U16_HI(attHandle);
    format[3] = U16_LO(offset);
    format[4] = U16_HI(offset);

    u8 api_status = (u8)blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 5, p, len);

    return api_status;
}

ble_sts_t blc_gatt_pushExecuteWriteRequest(u16 connHandle, u8 flags)
{
    if (flags > 1) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    //ExecuteWriteRequest format
    u8 format[2];
    format[0] = ATT_OP_EXECUTE_WRITE_REQ;
    format[1] = flags;

    u8 api_status = (u8)blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 2, NULL, 0);

    return api_status;
}

ble_sts_t blc_gatt_pushWriteRequest(u16 connHandle, u16 attHandle, u8 *p, int len)
{
    if (p == NULL || len == 0) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    gap_ms_para_t *pGap_ms_para = blc_gap_getMasterSlavePara(connHandle);
    if (len > (pGap_ms_para->effective_MTU - 3)) {
        return GATT_ERR_INVALID_PARAMETER;
    }

#if (ATT_HANDLE_VALUE_CUSTOM_EN)
    if (bltAtt.attHl_custom_en) {
        attHandle = blt_att_change_sdkAttHandle_to_customAttHandle(attHandle);
    }
#endif

    //WriteRequest format
    u8 format[4];
    format[0] = ATT_OP_WRITE_REQ;
    format[1] = U16_LO(attHandle);
    format[2] = U16_HI(attHandle);


    u8 api_status = (u8)blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 3, p, len);

    return api_status;
}

ble_sts_t blc_gatt_pushWriteCommand(u16 connHandle, u16 attHandle, u8 *p, int len)
{
    if (p == NULL || len == 0) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    gap_ms_para_t *pGap_ms_para = blc_gap_getMasterSlavePara(connHandle);
    if (len > (pGap_ms_para->effective_MTU - 3)) {
        return GATT_ERR_INVALID_PARAMETER;
    }

#if (ATT_HANDLE_VALUE_CUSTOM_EN)
    if (bltAtt.attHl_custom_en) {
        attHandle = blt_att_change_sdkAttHandle_to_customAttHandle(attHandle);
    }
#endif

    //WriteCommand format
    u8 format[4];
    format[0] = ATT_OP_WRITE_CMD;
    format[1] = U16_LO(attHandle);
    format[2] = U16_HI(attHandle);


    u8 api_status = (u8)blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 3, p, len);

    return api_status;
}

ble_sts_t blc_gatt_pushFindInformationRequest(u16 connHandle, u16 start_attHandle, u16 end_attHandle)
{
#if (ATT_HANDLE_VALUE_CUSTOM_EN)
    if (bltAtt.attHl_custom_en) {
        start_attHandle = blt_att_change_sdkAttHandle_to_customAttHandle(start_attHandle);
        end_attHandle   = blt_att_change_sdkAttHandle_to_customAttHandle(end_attHandle);
    }
#endif

    u8 format[5];
    format[0] = ATT_OP_FIND_INFO_REQ;
    format[1] = U16_LO(start_attHandle);
    format[2] = U16_HI(start_attHandle);
    format[3] = U16_LO(end_attHandle);
    format[4] = U16_HI(end_attHandle);

    return blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 5, NULL, 0);
}

ble_sts_t blc_gatt_pushFindByTypeValueRequest(u16 connHandle, u16 start_attHandle, u16 end_attHandle, u16 uuid, u8 *attr_value, int len)
{
    if (attr_value == NULL || len == 0) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    gap_ms_para_t *pGap_ms_para = blc_gap_getMasterSlavePara(connHandle);
    if (len >= (pGap_ms_para->effective_MTU - 7)) {
        return GATT_ERR_INVALID_PARAMETER;
    }

#if (ATT_HANDLE_VALUE_CUSTOM_EN)
    if (bltAtt.attHl_custom_en) {
        start_attHandle = blt_att_change_sdkAttHandle_to_customAttHandle(start_attHandle);
        end_attHandle   = blt_att_change_sdkAttHandle_to_customAttHandle(end_attHandle);
    }
#endif

    u8 format[7];
    format[0] = ATT_OP_FIND_BY_TYPE_VALUE_REQ;
    format[1] = U16_LO(start_attHandle);
    format[2] = U16_HI(start_attHandle);
    format[3] = U16_LO(end_attHandle);
    format[4] = U16_HI(end_attHandle);
    format[5] = U16_LO(uuid);
    format[6] = U16_HI(uuid);

    return blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 7, attr_value, len);
}

ble_sts_t blc_gatt_pushReadByTypeRequest(u16 connHandle, u16 start_attHandle, u16 end_attHandle, u8 *uuid, int uuid_len)
{
    if (uuid == NULL || (uuid_len != 2 && uuid_len != 16)) {
        return GATT_ERR_INVALID_PARAMETER;
    }

#if (ATT_HANDLE_VALUE_CUSTOM_EN)
    if (bltAtt.attHl_custom_en) {
        start_attHandle = blt_att_change_sdkAttHandle_to_customAttHandle(start_attHandle);
        end_attHandle   = blt_att_change_sdkAttHandle_to_customAttHandle(end_attHandle);
    }
#endif

    u8 format[5];
    format[0] = ATT_OP_READ_BY_TYPE_REQ;
    format[1] = U16_LO(start_attHandle);
    format[2] = U16_HI(start_attHandle);
    format[3] = U16_LO(end_attHandle);
    format[4] = U16_HI(end_attHandle);

    return blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 5, uuid, uuid_len);
}

ble_sts_t blc_gatt_pushReadByGroupTypeRequest(u16 connHandle, u16 start_attHandle, u16 end_attHandle, u8 *uuid, int uuid_len)
{
    if (uuid == NULL || (uuid_len != 2 && uuid_len != 16)) {
        return GATT_ERR_INVALID_PARAMETER;
    }

#if (ATT_HANDLE_VALUE_CUSTOM_EN)
    if (bltAtt.attHl_custom_en) {
        start_attHandle = blt_att_change_sdkAttHandle_to_customAttHandle(start_attHandle);
        end_attHandle   = blt_att_change_sdkAttHandle_to_customAttHandle(end_attHandle);
    }
#endif

    u8 format[5];
    format[0] = ATT_OP_READ_BY_GROUP_TYPE_REQ;
    format[1] = U16_LO(start_attHandle);
    format[2] = U16_HI(start_attHandle);
    format[3] = U16_LO(end_attHandle);
    format[4] = U16_HI(end_attHandle);

    return blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 5, uuid, uuid_len);
}

ble_sts_t blc_gatt_pushReadRequest(u16 connHandle, u16 attHandle)
{
#if (ATT_HANDLE_VALUE_CUSTOM_EN)
    if (bltAtt.attHl_custom_en) {
        attHandle = blt_att_change_sdkAttHandle_to_customAttHandle(attHandle);
    }
#endif

    u8 format[3];
    format[0] = ATT_OP_READ_REQ;
    format[1] = U16_LO(attHandle);
    format[2] = U16_HI(attHandle);

    return blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 3, NULL, 0);
}

ble_sts_t blc_gatt_pushReadBlobRequest(u16 connHandle, u16 attHandle, u16 offset)
{
#if (ATT_HANDLE_VALUE_CUSTOM_EN)
    if (bltAtt.attHl_custom_en) {
        attHandle = blt_att_change_sdkAttHandle_to_customAttHandle(attHandle);
    }
#endif

    u8 format[4];
    format[0] = ATT_OP_READ_BLOB_REQ;
    format[1] = U16_LO(attHandle);
    format[2] = U16_HI(attHandle);

    u16 tempOffset = offset;

    return blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 3, (u8 *)&tempOffset, 2);
}

ble_sts_t blc_gatt_pushErrResponse(u16 connHandle, u8 reqOpcode, u16 attHdlInErr, u8 ErrorCode)
{
#if (ATT_HANDLE_VALUE_CUSTOM_EN)
    if (bltAtt.attHl_custom_en) {
        attHdlInErr = blt_att_change_sdkAttHandle_to_customAttHandle(attHdlInErr);
    }
#endif

    u8 format[1];
    format[0] = ATT_OP_ERROR_RSP;

    u8 temp[4];
    temp[0] = reqOpcode;
    temp[1] = U16_LO(attHdlInErr);
    temp[2] = U16_HI(attHdlInErr);
    temp[3] = ErrorCode;
    return blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 1, (u8 *)temp, 4);
}

ble_sts_t blc_gatt_pushHandleValueConfirm(u16 connHandle)
{
    u8 format[1];
    format[0] = ATT_OP_HANDLE_VALUE_CFM;

    return blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 1, NULL, 0);
}

/*
 * Note : Just to be compatible with the application layer code provided to Google(B92),
 * The SDK provided to Google later still uses blc_gatt_pushConfirm.
 * The public B91m SDK will use blc_gatt_pushHandleValueConfirm.
 * added by lihaojie at 2024.7.1
 */
ble_sts_t blc_gatt_pushConfirm(u16 connHandle)
{
    u8 format[1];
    format[0] = ATT_OP_HANDLE_VALUE_CFM;

    return blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 1, NULL, 0);
}
