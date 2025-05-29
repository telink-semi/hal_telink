/********************************************************************************************************
 * @file    attc.c
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

#include "attc.h"

/////////////////////// ATT Client process att_rx_packet /////////////////////////////

/*
 * pAttrConCb->attReq.attcReqPending check current NOT concern.
 */
u16 blt_attc_proc_errorRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    return blt_gattc_handle_rsp(pAttrConCb->connHandle, attr, attrLen);
}

u16 blt_attc_proc_exchangeMtuRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    (void)attrLen;                                     //unused, remove warning
    if (pAttrConCb->scid != L2CAP_CID_ATTR_PROTOCOL) { //For EATT channels
        return 0;                                      //EATT MTU response not ack any ATT value.
    }
    blt_attr_exchangeMtuRsp_t *pRsp        = (blt_attr_exchangeMtuRsp_t *)attr;
    u16                        serverRxMtu = max(pRsp->serverRxMtu, ATT_MTU_SIZE);
    u16                        mtu         = blt_gattc_exchangeMtu_rsp(pAttrConCb->connHandle, serverRxMtu);

    if (gap_eventMask & GAP_EVT_MASK_ATT_EXCHANGE_MTU) {
        u8                             param_evt[8];
        gap_gatt_mtuSizeExchangeEvt_t *pEvt = (gap_gatt_mtuSizeExchangeEvt_t *)param_evt;
        pEvt->connHandle                    = pAttrConCb->connHandle;
        pEvt->peer_MTU                      = serverRxMtu;
        pEvt->effective_MTU                 = mtu;

        blc_gap_send_event(GAP_EVT_ATT_EXCHANGE_MTU, param_evt, sizeof(gap_gatt_mtuSizeExchangeEvt_t));
    }

    return 0;
}

u16 blt_attc_proc_findInfoRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    return blt_gattc_handle_rsp(pAttrConCb->connHandle, attr, attrLen);
}

u16 blt_attc_proc_findByTypeValueRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    return blt_gattc_handle_rsp(pAttrConCb->connHandle, attr, attrLen);
}

u16 blt_attc_proc_readByTypeRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    return blt_gattc_handle_rsp(pAttrConCb->connHandle, attr, attrLen);
}

u16 blt_attc_proc_readRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    return blt_gattc_handle_rsp(pAttrConCb->connHandle, attr, attrLen);
}

u16 blt_attc_proc_readBlobRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    return blt_gattc_handle_rsp(pAttrConCb->connHandle, attr, attrLen);
}

u16 blt_attc_proc_readMultiRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    return blt_gattc_handle_rsp(pAttrConCb->connHandle, attr, attrLen);
}

u16 blt_attc_proc_readMultiVarRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    return blt_gattc_handle_rsp(pAttrConCb->connHandle, attr, attrLen);
}

u16 blt_attc_proc_readByGroupTypeRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    return blt_gattc_handle_rsp(pAttrConCb->connHandle, attr, attrLen);
}

u16 blt_attc_proc_writeRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    return blt_gattc_handle_rsp(pAttrConCb->connHandle, attr, attrLen);
}

u16 blt_attc_proc_prepareWriteRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    return blt_gattc_handle_rsp(pAttrConCb->connHandle, attr, attrLen);
}

u16 blt_attc_proc_executeWriteRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    return blt_gattc_handle_rsp(pAttrConCb->connHandle, attr, attrLen);
}

u16 blt_attc_proc_handleValueNtf(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    /* ATT notify process */
    blt_gattc_notification(pAttrConCb->connHandle, attr, attrLen);

    return 0;
}

u16 blt_attc_proc_handleValueInd(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    /* ATT indicate process */
    blt_gattc_notification(pAttrConCb->connHandle, attr, attrLen);

    /* prepare to send indicate-confirm packet */
    return blc_att_prepareConfirm(pAttrConCb->attTxBuff);
}

u16 blt_attc_proc_multiHandleValueNtf(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    /* ATT multiple notify process */
    blt_gattc_multiNotification(pAttrConCb->connHandle, attr, attrLen);

    return 0;
}

////////////////////// ATT Client Send packet concerned /////////////////////////////
ble_sts_t blc_attc_sendMtuSizeExchangeRequest(u16 connHandle, u16 mtuSize)
{
    if (((connHandle & BLM_CONN_HANDLE) && (!blc_att_setCentralRxMtuSize(mtuSize))) ||
        ((connHandle & BLS_CONN_HANDLE) && (!blc_att_setPeripheralRxMtuSize(mtuSize)))) {
        //PrepareMtuSizeExchangeRequest format
        u8 format[3];
        format[0] = ATT_OP_EXCHANGE_MTU_REQ;
        format[1] = U16_LO(mtuSize);
        format[2] = U16_HI(mtuSize);

        return blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 3, NULL, 0);
    } else {
        return GATT_ERR_INVALID_PARAMETER;
    }
}

ble_sts_t blc_attc_sendPrepareWriteRequest(u16 connHandle, u16 attHandle, u16 offset, u8 *p, u16 len)
{
    if (p == NULL || len == 0) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    //PrepareWriteRequest format
    u8 format[5];
    format[0] = ATT_OP_PREPARE_WRITE_REQ;
    format[1] = U16_LO(attHandle);
    format[2] = U16_HI(attHandle);
    format[3] = U16_LO(offset);
    format[4] = U16_HI(offset);

    return blt_att_sendData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 5, p, len);
}

ble_sts_t blc_attc_sendExecuteWriteRequest(u16 connHandle, u16 attHandle, u8 flags)
{
    (void)attHandle; //unused, remove warning
    if (flags != ATT_EXEC_WRITE_CANCEL && flags != ATT_EXEC_WRITE_ALL) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    //ExecuteWriteRequest format
    u8 format[2];
    format[0] = ATT_OP_EXECUTE_WRITE_REQ;
    format[1] = flags;

    return blt_att_sendData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 2, NULL, 0);
}

ble_sts_t blc_attc_sendWriteRequest(u16 connHandle, u16 attHandle, u8 *p, u16 len)
{
    if (p == NULL || len == 0) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    //WriteRequest format
    u8 format[3];
    format[0] = ATT_OP_WRITE_REQ;
    format[1] = U16_LO(attHandle);
    format[2] = U16_HI(attHandle);

    return blt_att_sendData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 3, p, len);
}

ble_sts_t blc_attc_sendWriteCommand(u16 connHandle, u16 attHandle, u8 *p, u16 len)
{
    if (p == NULL || len == 0) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    //WriteCommand format
    u8 format[3];
    format[0] = ATT_OP_WRITE_CMD;
    format[1] = U16_LO(attHandle);
    format[2] = U16_HI(attHandle);

    return blt_att_sendData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 3, p, len);
}

ble_sts_t blc_attc_sendFindInfoRequest(u16 connHandle, u16 start_attHandle, u16 end_attHandle)
{
    //FindInfoReq format
    u8 format[5];
    format[0] = ATT_OP_FIND_INFO_REQ;
    format[1] = U16_LO(start_attHandle);
    format[2] = U16_HI(start_attHandle);
    format[3] = U16_LO(end_attHandle);
    format[4] = U16_HI(end_attHandle);

    return blt_att_sendData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 5, NULL, 0);
}

ble_sts_t blc_attc_sendFindByTypeValueRequest(u16 connHandle, u16 start_attHandle, u16 end_attHandle, u16 uuid, u8 *attr_value, u16 attr_value_len)
{
    if (attr_value == NULL && attr_value_len > 0) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    u16 mtu = blt_gap_getScidMtu(connHandle, L2CAP_CID_ATTR_PROTOCOL); /* att layer call gap layer's API TODO */

    if (mtu == 0 || attr_value_len > (mtu - 7)) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    /*
     * The size of the array depends on the size of the effectMTU, so it is impossible to estimate the size of the array.
     * Considering few applications to use larger effectMTU, the array size is set to 517B.
     * (Refer to <<Core5.3>> | Vol 3, Part F page 1416: The maximum length of an attribute value shall be 512 octets.)
     * It is best to use dynamic allocation here,so that set array according to the size of the actual data size.
     */
#if (0)
    u8 format[ATT_MAX_MTU]; //TODO: normal stack need larger
#else                       /* GCC C99 */
    u8 format[mtu];
#endif

    format[0] = ATT_OP_FIND_BY_TYPE_VALUE_REQ;
    format[1] = U16_LO(start_attHandle);
    format[2] = U16_HI(start_attHandle);
    format[3] = U16_LO(end_attHandle);
    format[4] = U16_HI(end_attHandle);
    format[5] = U16_LO(uuid);
    format[6] = U16_HI(uuid);
    memcpy(&format[7], attr_value, attr_value_len);

    return blt_att_sendData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 7 + attr_value_len, NULL, 0);
}

ble_sts_t blc_attc_sendReadByTypeRequest(u16 connHandle, u16 start_attHandle, u16 end_attHandle, u8 *uuid, u8 uuid_len)
{
    if (uuid == NULL || (uuid_len != 2 && uuid_len != 16)) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    u8 format[5 + 16];
    format[0] = ATT_OP_READ_BY_TYPE_REQ;
    format[1] = U16_LO(start_attHandle);
    format[2] = U16_HI(start_attHandle);
    format[3] = U16_LO(end_attHandle);
    format[4] = U16_HI(end_attHandle);
    memcpy(&format[5], uuid, uuid_len);

    return blt_att_sendData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 5 + uuid_len, NULL, 0);
}

ble_sts_t blc_attc_sendReadByGroupTypeRequest(u16 connHandle, u16 start_attHandle, u16 end_attHandle, u8 *uuid, u8 uuid_len)
{
    if (uuid == NULL || (uuid_len != 2 && uuid_len != 16)) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    u8 format[5 + 16];
    format[0] = ATT_OP_READ_BY_GROUP_TYPE_REQ;
    format[1] = U16_LO(start_attHandle);
    format[2] = U16_HI(start_attHandle);
    format[3] = U16_LO(end_attHandle);
    format[4] = U16_HI(end_attHandle);
    memcpy(&format[5], uuid, uuid_len);

    return blt_att_sendData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 5 + uuid_len, NULL, 0);
}

ble_sts_t blc_attc_sendReadRequest(u16 connHandle, u16 attHandle)
{
    u8 format[3];
    format[0] = ATT_OP_READ_REQ;
    format[1] = U16_LO(attHandle);
    format[2] = U16_HI(attHandle);

    return blt_att_sendData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 3, NULL, 0);
}

ble_sts_t blc_attc_sendReadBlobRequest(u16 connHandle, u16 attHandle, u16 offset)
{
    u8 format[4];
    format[0] = ATT_OP_READ_BLOB_REQ;
    format[1] = U16_LO(attHandle);
    format[2] = U16_HI(attHandle);

    u16 tempOffset = offset;

    return blt_att_sendData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 3, (u8 *)&tempOffset, 2);
}

ble_sts_t blc_attc_sendAttHdlValueCfm(u16 connHandle)
{
    u8 format[1];
    format[0] = ATT_OP_HANDLE_VALUE_CFM;

    return blt_att_sendData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 1, NULL, 0);
}

/**
 * Format of ATT_READ_MULTIPLE_REQ PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Set Of Handles                     | 4 to (ATT_MTU-1)  |
 */
ble_sts_t blc_attc_sendReadMultReq(u16 connHandle, u8 numHandles, u16 *pHandle)
{
    if (pHandle == NULL || numHandles < 2) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    u16 mtu = blt_gap_getScidMtu(connHandle, L2CAP_CID_ATTR_PROTOCOL); /* att layer call gap layer's API TODO */

    /* attr_pkt_data length < ATT_MTU -1 */
    if (mtu == 0 || (u16)(numHandles * sizeof(pHandle)) > (mtu - 1)) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    /*
     * The size of the array depends on the size of the effectMTU, so it is impossible to estimate the size of the array.
     * Considering few applications to use larger effectMTU, the array size is set to 517B.
     * (Refer to <<Core5.3>> | Vol 3, Part F page 1416: The maximum length of an attribute value shall be 512 octets.)
     * It is best to use dynamic allocation here,so that set array according to the size of the actual data size.
     */
#if (0)
    u8 format[ATT_MAX_MTU]; //TODO: normal stack need larger
#else                       /* GCC C99 */
    u8 format[mtu];
#endif
    u8 buffLen        = 0;
    format[buffLen++] = ATT_OP_READ_MULTIPLE_REQ;
    while (numHandles--) {
        format[buffLen++] = ((*pHandle) & 0x00FF);
        format[buffLen++] = ((*pHandle) & 0xFF00) >> 8;
        pHandle++;
    }

    return blt_att_sendData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, buffLen, NULL, 0);
}

/**
 * Format of ATT_READ_MULTIPLE_VARIABLE_REQ PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Set Of Handles                     | 4 to (ATT_MTU-1)  |
 */
ble_sts_t blc_attc_sendReadMultVarReq(u16 connHandle, u8 numHandles, u16 *pHandle)
{
    if (pHandle == NULL || numHandles < 2) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    u16 mtu = blt_gap_getScidMtu(connHandle, L2CAP_CID_ATTR_PROTOCOL); /* att layer call gap layer's API TODO */

    /* attr_pkt_data length < ATT_MTU -1 */
    if (mtu == 0 || (u16)(numHandles * sizeof(pHandle)) > (mtu - 1)) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    /*
     * The size of the array depends on the size of the effectMTU, so it is impossible to estimate the size of the array.
     * Considering few applications to use larger effectMTU, the array size is set to 517B.
     * (Refer to <<Core5.3>> | Vol 3, Part F page 1416: The maximum length of an attribute value shall be 512 octets.)
     * It is best to use dynamic allocation here,so that set array according to the size of the actual data size.
     */
#if (0)
    u8 format[ATT_MAX_MTU]; //TODO: normal stack need larger
#else                       /* GCC C99 */
    u8 format[mtu];
#endif
    u8 buffLen        = 0;
    format[buffLen++] = ATT_OP_READ_MULTIPLE_VARIABLE_REQ;
    while (numHandles--) {
        format[buffLen++] = ((*pHandle) & 0x00FF);
        format[buffLen++] = ((*pHandle) & 0xFF00) >> 8;
        pHandle++;
    }

    return blt_att_sendData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, buffLen, NULL, 0);
}

void blc_att_holdAttributeResponsePayloadDuringPairingPhase(u8 hold_enable)
{
    bltAtt.smpPairingHoldATT = hold_enable;
}
