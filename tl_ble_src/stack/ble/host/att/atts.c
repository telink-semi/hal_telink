/********************************************************************************************************
 * @file    atts.c
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

#include "atts.h"

#define ATTS_ERR_RSP_NOT_SUPPORTED blt_att_packageErrorRsp(attr->opcode, ATT_HANDLE_NONE, ATT_ERR_REQ_NOT_SUPPORTED, pAttConCb->attTxBuff)

/*
 * All data processing, using STREAM_TO_U16, U16_TO_STREAM macro definition
 * Compatible with both big-endian and small-endian
 */
/////////////////////// ATT Server process att_rx_packet /////////////////////////////
u16 blt_atts_proc_exchangeMtuReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    (void)attrLen;
    if (pAttrConCb->scid != L2CAP_CID_ATTR_PROTOCOL) { //For EATT channels
        return blt_att_packageErrorRsp(ATT_OP_EXCHANGE_MTU_REQ, ATT_HANDLE_NONE, ATT_ERR_REQ_NOT_SUPPORTED, pAttrConCb->attTxBuff);
    }

    u8 *pAttrData   = attr->data;
    u16 clientRxMtu = ATT_MTU_SIZE;
    STREAM_TO_U16(clientRxMtu, pAttrData);
    u16 connHandle = pAttrConCb->connHandle;

    u16 mtu = blt_gap_recvRemoteMtu(connHandle, clientRxMtu);

    if (gap_eventMask & GAP_EVT_MASK_ATT_EXCHANGE_MTU) {
        u8                             param_evt[8];
        gap_gatt_mtuSizeExchangeEvt_t *pEvt = (gap_gatt_mtuSizeExchangeEvt_t *)param_evt;
        pEvt->connHandle                    = connHandle;
        pEvt->peer_MTU                      = clientRxMtu;
        pEvt->effective_MTU                 = mtu;

        blc_gap_send_event(GAP_EVT_ATT_EXCHANGE_MTU, param_evt, sizeof(gap_gatt_mtuSizeExchangeEvt_t));
    }
    return blt_att_packageExchangeMtuRsp(blt_gap_getInitMtu(connHandle), pAttrConCb->attTxBuff);
}

u16 blt_atts_proc_findInfoReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    (void)attrLen;
    u8                     *pAttrData = attr->data;
    const atts_attribute_t *pAttr     = NULL;
    u16                     startHandle, endHandle;
    u8                      err        = ATT_SUCCESS;
    u16                     connHandle = pAttrConCb->connHandle;
    attr_pkt_t             *txBuf      = pAttrConCb->attTxBuff;
    u8                     *rsp        = (u8 *)txBuf->data;

    u16 mtu = blt_gap_getScidMtu(connHandle, pAttrConCb->scid);

    if (mtu < ATT_MTU_SIZE || (!blt_atts_initServiceDiscoverTick(connHandle))) {
        err = ATT_ERR_ATTR_NOT_FOUND;
    }

    STREAM_TO_U16(startHandle, pAttrData);
    STREAM_TO_U16(endHandle, pAttrData);

    atts_group_t *pAttrGroup = blc_gatts_getAttributeServiceGroup(connHandle);
    if (pAttrGroup == NULL) {
        err = ATT_ERR_ATTR_NOT_FOUND;
    } else if ((startHandle == 0) || (startHandle > endHandle)) {
        err = ATT_ERR_INVALID_HANDLE;
    }

    u16 handle = blt_atts_findInRange(connHandle, startHandle, endHandle, &pAttr);
    if (handle == ATT_HANDLE_NONE) {
        err = ATT_ERR_ATTR_NOT_FOUND;
    }

    if (!err) {
        u16 uuidLen = pAttr->uuidLen;
        U8_TO_STREAM(rsp, uuidLen == ATT_16_UUID_LEN ? ATT_FIND_HANDLE_16_UUID : ATT_FIND_HANDLE_128_UUID);

        u8 maxListCnt = (mtu - ATT_FIND_INFO_RSP_LEN) / (uuidLen + 2);

        U16_TO_STREAM(rsp, handle);
        STR_TO_STREAM(rsp, pAttr->uuid, uuidLen);
        maxListCnt--;

        handle++;
        while (handle <= endHandle && maxListCnt) {
            handle = blt_atts_findInRange(connHandle, handle, endHandle, &pAttr);
            if (handle == ATT_HANDLE_NONE) {
                break;
            }

            if (pAttr->uuidLen != uuidLen) {
                break;
            }

            U16_TO_STREAM(rsp, handle);
            STR_TO_STREAM(rsp, pAttr->uuid, uuidLen);
            maxListCnt--;

            if (handle == ATT_HANDLE_MAX) {
                break;
            }

            if (++handle > endHandle) {
                break;
            }
        }
    }

    if (err) {
        return blt_att_packageErrorRsp(ATT_OP_FIND_INFO_REQ, startHandle, err, txBuf);
    }
    txBuf->opcode = ATT_OP_FIND_INFO_RSP;
    return rsp - &txBuf->opcode;
}

u16 blt_atts_proc_findByTypeValueReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    u8                     *pAttrData = attr->data;
    const atts_attribute_t *pAttr     = NULL;
    atts_group_t           *pGroup    = NULL;
    u16                     startHandle, endHandle;
    u8                      err        = ATT_SUCCESS;
    u16                     connHandle = pAttrConCb->connHandle;
    attr_pkt_t             *txBuf      = pAttrConCb->attTxBuff;
    u8                     *rsp        = (u8 *)txBuf->data;

    u16 mtu = blt_gap_getScidMtu(connHandle, pAttrConCb->scid);

    if (mtu < ATT_MTU_SIZE || (!blt_atts_initServiceDiscoverTick(connHandle))) {
        err = ATT_ERR_ATTR_NOT_FOUND;
    }

    STREAM_TO_U16(startHandle, pAttrData);
    STREAM_TO_U16(endHandle, pAttrData);
    u8 *attrType = pAttrData;
    pAttrData += 2;

    atts_group_t *pAttrGroup = blc_gatts_getAttributeServiceGroup(connHandle);
    if (pAttrGroup == NULL) {
        err = ATT_ERR_ATTR_NOT_FOUND;
    } else if ((startHandle == ATT_HANDLE_NONE) || (startHandle > endHandle)) {
        err = ATT_ERR_INVALID_HANDLE;
    }

    u8 len = attrLen - ATT_FIND_TYPE_REQ_LEN;

    if (!err) {
        u16 handle = startHandle;
        u16 nextHandle;
        u8  maxListCnt = (mtu - ATT_FIND_TYPE_RSP_LEN) >> 2;
        while (((handle = blt_atts_findUuidInRange(connHandle, handle, endHandle, ATT_16_UUID_LEN, attrType, &pAttr, &pGroup)) != ATT_HANDLE_NONE) && maxListCnt) {
            if ((pAttr->perm & ATT_PERMISSIONS_READ) &&
                ((len == 0) || ((len == *pAttr->attrValueLen) && (memcmp(pAttrData, pAttr->attrValue, len) == 0)))) {
                if (blt_uuid_cmp16or128(declarationsPrimaryServiceUuid, ATT_16_UUID_LEN, attrType)) {
                    nextHandle = blt_atts_findServiceGroupEnd(connHandle, handle);
                } else {
                    nextHandle = handle;
                }
                U16_TO_STREAM(rsp, handle);
                U16_TO_STREAM(rsp, nextHandle);
                maxListCnt--;
            } else {
                nextHandle = handle;
            }

            if ((nextHandle >= endHandle) || (nextHandle == ATT_HANDLE_MAX)) {
                break;
            }

            handle = nextHandle + 1;
        }

        if ((u8 *)txBuf->data == rsp) {
            err = ATT_ERR_ATTR_NOT_FOUND;
        }
    }

    if (err) {
        return blt_att_packageErrorRsp(ATT_OP_FIND_BY_TYPE_VALUE_REQ, startHandle, err, txBuf);
    }
    txBuf->opcode = ATT_OP_FIND_BY_TYPE_VALUE_RSP;
    return rsp - &txBuf->opcode;
}

u16 blt_atts_proc_readByTypeReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    u8                     *pAttrData = attr->data;
    const atts_attribute_t *pAttr     = NULL;
    atts_group_t           *pGroup    = NULL;
    u16                     startHandle, endHandle;
    int                     err        = ATT_SUCCESS;
    u16                     connHandle = pAttrConCb->connHandle;
    attr_pkt_t             *txBuf      = pAttrConCb->attTxBuff;
    u8                     *rsp        = (u8 *)txBuf->data;

    u16 mtu = blt_gap_getScidMtu(connHandle, pAttrConCb->scid);

    if (mtu < ATT_MTU_SIZE || (!blt_atts_initServiceDiscoverTick(connHandle))) {
        err = ATT_ERR_ATTR_NOT_FOUND;
    }

    STREAM_TO_U16(startHandle, pAttrData);
    STREAM_TO_U16(endHandle, pAttrData);

    u8 uuid_len = attrLen - ATT_READ_TYPE_REQ_LEN;

    atts_group_t *pAttrGroup = blc_gatts_getAttributeServiceGroup(connHandle);
    if (pAttrGroup == NULL) {
        err = ATT_ERR_ATTR_NOT_FOUND;
    } else if (!((uuid_len == ATT_16_UUID_LEN) || (uuid_len == ATT_128_UUID_LEN))) {
        err = ATT_ERR_INVALID_PDU;
    } else if ((startHandle == ATT_HANDLE_NONE) || (startHandle > endHandle)) {
        err = ATT_ERR_INVALID_HANDLE;
    }

    if (!err) {
        u16 handle  = blt_atts_findUuidInRange(connHandle, startHandle, endHandle, uuid_len, pAttrData, &pAttr, &pGroup);
        startHandle = handle;

        if (handle == ATT_HANDLE_NONE) {
            err = ATT_ERR_ATTR_NOT_FOUND;
        } else if ((err = blt_atts_permissions(connHandle, ATT_PERMISSIONS_READ, handle, pAttr->perm)) != ATT_SUCCESS) {
        } else {
            u16 attLen   = pAttr->attrValueLen ? *pAttr->attrValueLen : 0;
            u8 *attValue = pAttr->attrValue;

            if ((pAttr->settings & ATTS_SET_READ_CBACK) && (pGroup->readCback != NULL)) {
                u8 *outValue    = NULL;
                u16 outValueLen = 0;
                //read callback return is ATT_SUCCESS
                err = pGroup->readCback(connHandle, ATT_OP_READ_BY_TYPE_REQ, startHandle, &outValue, &outValueLen);

                //if return error code out, stack cannot reply.
                if (err < ATT_SUCCESS || err >= 0x100) {
                    return 0;
                }
                if (err != ATT_SUCCESS) {
                    goto attsPushReadByTypeRsp;
                }
                if (outValueLen) {
                    attLen   = outValueLen;
                    attValue = outValue;
                }
            }

            if (pAttr->settings & ATTS_SET_ATTR_VALUE_PROPERTIES) {
                const atts_attribute_t *pNextAttr = pAttr + 1;
                attLen                            = pNextAttr->uuidLen + 3;
                U8_TO_STREAM(rsp, attLen + 2);                           //pair Length
                U16_TO_STREAM(rsp, handle);                              //Attribute Handle
                U8_TO_STREAM(rsp, *pAttr->attrValue);                    //Characteristic Properties
                U16_TO_STREAM(rsp, handle + 1);                          //Characteristic Value Handle
                STR_TO_STREAM(rsp, pNextAttr->uuid, pNextAttr->uuidLen); //Characteristic UUID

            } else {
                U8_TO_STREAM(rsp, attLen + 2);
                U16_TO_STREAM(rsp, handle);
                STR_TO_STREAM(rsp, attValue, attLen);
            }

            attLen        = min(attLen, mtu - ATT_READ_TYPE_RSP_LEN - 2); //attrValueLen > MTU-2-2, attLen use min value.
            u8 maxListCnt = (mtu - ATT_READ_TYPE_RSP_LEN) / (attLen + 2);

            maxListCnt--;

            handle++;
            while (((handle = blt_atts_findUuidInRange(connHandle, handle, endHandle, uuid_len, pAttrData, &pAttr, &pGroup)) != ATT_HANDLE_NONE) && maxListCnt) {
                if (blt_atts_permissions(connHandle, ATT_PERMISSIONS_READ, handle, pAttr->perm) != ATT_SUCCESS) {
                    break;
                }
                u16 newAttLen = pAttr->attrValueLen ? *pAttr->attrValueLen : 0;
                ;
                attValue = pAttr->attrValue;
                if ((pAttr->settings & ATTS_SET_READ_CBACK) && (pGroup->readCback != NULL)) {
                    u8 *outValue    = NULL;
                    u16 outValueLen = 0;
                    //read callback return is ATT_SUCCESS
                    int err2 = pGroup->readCback(connHandle, ATT_OP_READ_BY_TYPE_REQ, startHandle, &outValue, &outValueLen);

                    //if return error code out, stack cannot reply.
                    if (err2 != ATT_SUCCESS) {
                        break;
                    }

                    if (outValueLen) {
                        newAttLen = outValueLen;
                        attValue  = outValue;
                    }
                }

                if (pAttr->settings & ATTS_SET_ATTR_VALUE_PROPERTIES) {
                    const atts_attribute_t *pNextAttr = pAttr + 1;
                    newAttLen                         = pNextAttr->uuidLen + 3;

                    if (newAttLen != attLen) {
                        break;
                    }
                    U16_TO_STREAM(rsp, handle);                              //Attribute Handle
                    U8_TO_STREAM(rsp, *pAttr->attrValue);                    //Characteristic Properties
                    U16_TO_STREAM(rsp, handle + 1);                          //Characteristic Value Handle
                    STR_TO_STREAM(rsp, pNextAttr->uuid, pNextAttr->uuidLen); //Characteristic UUID
                    maxListCnt--;
                } else if (newAttLen == attLen) {
                    U16_TO_STREAM(rsp, handle);
                    STR_TO_STREAM(rsp, attValue, newAttLen);
                    maxListCnt--;
                } else {
                    break;
                }

                if (handle == ATT_HANDLE_MAX) {
                    break;
                }

                if (++handle > endHandle) {
                    break;
                }
            }
        }
    }

attsPushReadByTypeRsp:
    if (err) {
        return blt_att_packageErrorRsp(ATT_OP_READ_BY_TYPE_REQ, startHandle, err, txBuf);
    }
    txBuf->opcode = ATT_OP_READ_BY_TYPE_RSP;
    return rsp - &txBuf->opcode;
}

u16 blt_atts_proc_read(attConCb_t *pAttrConCb, u16 attrHandle, u16 valueOffset, u8 opcode)
{
    const atts_attribute_t *pAttr       = NULL;
    atts_group_t           *pGroup      = NULL;
    int                     err         = ATT_SUCCESS;
    u16                     connHandle  = pAttrConCb->connHandle;
    attr_pkt_t             *txBuf       = pAttrConCb->attTxBuff;
    u8                     *rsp         = (u8 *)txBuf->data;
    u8                     *outValue    = NULL;
    u16                     outValueLen = 0x0000;

    u16 mtu = blt_gap_getScidMtu(connHandle, pAttrConCb->scid);

    pAttr = blt_atts_findByHandle(connHandle, attrHandle, &pGroup);

    if (pAttr != NULL) {
        if ((err = blt_atts_permissions(connHandle, ATT_PERMISSIONS_READ, attrHandle, pAttr->perm)) == ATT_SUCCESS) {
            if ((pAttr->settings & ATTS_SET_READ_CBACK) &&
                (pGroup->readCback != NULL)) {
                //read callback return is ATT_SUCCESS
                err = pGroup->readCback(connHandle, valueOffset ? ATT_OP_READ_BLOB_REQ : ATT_OP_READ_REQ, attrHandle, &outValue, &outValueLen);
                //if return error code out, stack cannot reply.
                if (err < ATT_SUCCESS || err >= 0x100) {
                    return 0;
                }
                if (err != ATT_SUCCESS) {
                    goto attsPushReadRsp;
                }
            }

            if (err == ATT_SUCCESS && outValueLen == 0x0000) {
                outValue    = pAttr->attrValue;
                outValueLen = pAttr->attrValueLen ? *pAttr->attrValueLen : 0;
            }

            if (valueOffset > outValueLen) {
                err = ATT_ERR_INVALID_OFFSET;
            }

            outValueLen = min(outValueLen - valueOffset, mtu - 1);
            outValue += valueOffset;
        }
    } else {
        err = ATT_ERR_INVALID_HANDLE;
    }

attsPushReadRsp:
    if (err) {
        return blt_att_packageErrorRsp(opcode, attrHandle, err, txBuf);
    }

    txBuf->opcode = opcode == ATT_OP_READ_REQ ? ATT_OP_READ_RSP : ATT_OP_READ_BLOB_RSP;
    STR_TO_STREAM(rsp, outValue, outValueLen);
    return rsp - &txBuf->opcode;
}

u16 blt_atts_proc_readReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    (void)attrLen;
    u8 *pAttrData  = attr->data;
    u16 attrHandle = ATT_HANDLE_NONE;
    STREAM_TO_U16(attrHandle, pAttrData);
    return blt_atts_proc_read(pAttrConCb, attrHandle, 0x0000, ATT_OP_READ_REQ);
}

u16 blt_atts_proc_readBlobReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    (void)attrLen;
    u8 *pAttrData   = attr->data;
    u16 attrHandle  = ATT_HANDLE_NONE;
    u16 valueOffset = 0x0000;
    STREAM_TO_U16(attrHandle, pAttrData);
    STREAM_TO_U16(valueOffset, pAttrData);
    return blt_atts_proc_read(pAttrConCb, attrHandle, valueOffset, ATT_OP_READ_BLOB_REQ);
}

u16 blt_atts_proc_readMultiReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    const atts_attribute_t *pAttr      = NULL;
    atts_group_t           *pGroup     = NULL;
    int                     err        = ATT_SUCCESS;
    u16                     connHandle = pAttrConCb->connHandle;
    attr_pkt_t             *txBuf      = pAttrConCb->attTxBuff;
    u8                     *rsp        = (u8 *)txBuf->data;
    u16                     attHandle  = 0;
    u8                     *pAttVal    = attr->data;
    u8                      setOfHdls  = (attrLen - 1) >> 1;
    u16                     mtu        = blt_gap_getScidMtu(connHandle, pAttrConCb->scid);
    if (mtu < ATT_MTU_SIZE || (!blt_atts_initServiceDiscoverTick(connHandle))) {
        err = ATT_ERR_ATTR_NOT_FOUND;
    } else if (setOfHdls == 0 || attrLen > mtu) {
        setOfHdls = 0;
        err       = ATT_ERR_INVALID_HANDLE;
    }

    int maxRspValLen = mtu - 1;

    while (setOfHdls--) {
        STREAM_TO_U16(attHandle, pAttVal);
        pAttr = blt_atts_findByHandle(connHandle, attHandle, &pGroup);
        if (pAttr == NULL) {
            err = ATT_ERR_INVALID_HANDLE;
            break;
        }

        if ((err = blt_atts_permissions(connHandle, ATT_PERMISSIONS_READ, attHandle, pAttr->perm)) != ATT_SUCCESS) {
            break;
        }

        u8 *outValue    = pAttr->attrValue;
        u16 outValueLen = pAttr->attrValueLen ? *pAttr->attrValueLen : 0;
        ;
        if ((pAttr->settings & ATTS_SET_READ_CBACK) && (pGroup->readCback != NULL)) {
            //read callback return is ATT_SUCCESS
            err = pGroup->readCback(connHandle, ATT_OP_READ_MULTIPLE_REQ, attHandle, &outValue, &outValueLen);
            if (err != ATT_SUCCESS) {
                break;
            }

            if (outValueLen == 0x0000) {
                outValue    = pAttr->attrValue;
                outValueLen = pAttr->attrValueLen ? *pAttr->attrValueLen : 0;
                ;
            }
        }

        /* If the Set Of Values parameter is longer than (ATT_MTU-1) then only the first (ATT_MTU-1) octets shall be included in this response. */
        maxRspValLen -= outValueLen;
        if (maxRspValLen < 0) {
            outValueLen += maxRspValLen;
            STR_TO_STREAM(rsp, outValue, outValueLen);
            break;
        }
        STR_TO_STREAM(rsp, outValue, outValueLen);
    }

    if (err) {
        return blt_att_packageErrorRsp(ATT_OP_READ_MULTIPLE_REQ, attHandle, err, txBuf);
    }

    txBuf->opcode = ATT_OP_READ_MULTIPLE_RSP;

    return min(mtu, rsp - &txBuf->opcode);
}

u16 blt_atts_proc_readByGroupTypeReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    u8                     *pAttrData = attr->data;
    const atts_attribute_t *pAttr     = NULL;
    atts_group_t           *pGroup    = NULL;
    u16                     startHandle, endHandle;
    u8                      err        = ATT_SUCCESS;
    u16                     connHandle = pAttrConCb->connHandle;
    attr_pkt_t             *txBuf      = pAttrConCb->attTxBuff;
    u8                     *rsp        = (u8 *)txBuf->data;

    u16 mtu = blt_gap_getScidMtu(connHandle, pAttrConCb->scid);

    if (mtu < ATT_MTU_SIZE || (!blt_atts_initServiceDiscoverTick(connHandle))) {
        err = ATT_ERR_ATTR_NOT_FOUND;
    }

    STREAM_TO_U16(startHandle, pAttrData);
    STREAM_TO_U16(endHandle, pAttrData);

    u8            uuid_len   = attrLen - ATT_READ_GROUP_TYPE_REQ_LEN;
    atts_group_t *pAttrGroup = blc_gatts_getAttributeServiceGroup(connHandle);
    if (pAttrGroup == NULL) {
        err = ATT_ERR_ATTR_NOT_FOUND;
    } else if (!((uuid_len == ATT_16_UUID_LEN) || (uuid_len == ATT_128_UUID_LEN))) {
        err = ATT_ERR_INVALID_PDU;
    } else if ((startHandle == ATT_HANDLE_NONE) || (startHandle > endHandle)) {
        err = ATT_ERR_INVALID_HANDLE;
    } else if (!blt_uuid_cmp16or128(declarationsPrimaryServiceUuid, uuid_len, pAttrData)) {
        err = ATT_ERR_UNSUPPORTED_GRP_TYPE;
    }

    if (!err) {
        u16 handle = blt_atts_findUuidInRange(connHandle, startHandle, endHandle, uuid_len, pAttrData, &pAttr, &pGroup);
        u16 attLen = *pAttr->attrValueLen;
        if (!((attLen == ATT_16_UUID_LEN) || (attLen == ATT_128_UUID_LEN))) {
            err = ATT_ERR_ATTR_NOT_FOUND;
        } else if (handle == ATT_HANDLE_NONE) {
            err = ATT_ERR_ATTR_NOT_FOUND;
        } else if ((err = blt_atts_permissions(connHandle, ATT_PERMISSIONS_READ, handle, pAttr->perm)) != ATT_SUCCESS) {
            startHandle = handle;
        } else {
            U8_TO_STREAM(rsp, attLen + 4); //4: Attribute Handle and End Group Handle

            u8 maxListCnt = (mtu - ATT_READ_GROUP_TYPE_RSP_LEN) / (attLen + 4);
            U16_TO_STREAM(rsp, handle);
            handle = blt_atts_findServiceGroupEnd(connHandle, handle);
            U16_TO_STREAM(rsp, handle);
            STR_TO_STREAM(rsp, pAttr->attrValue, attLen);
            maxListCnt--;

            while (maxListCnt) {
                if (handle == ATT_HANDLE_MAX) {
                    break;
                }
                if (++handle > endHandle) {
                    break;
                }
                if ((handle = blt_atts_findUuidInRange(connHandle, handle, endHandle, uuid_len, pAttrData, &pAttr, &pGroup)) == ATT_HANDLE_NONE) {
                    break;
                }
                if ((*pAttr->attrValueLen == attLen) &&
                    (blt_atts_permissions(connHandle, ATT_PERMISSIONS_READ, handle, pAttr->perm) == ATT_SUCCESS)) {
                    U16_TO_STREAM(rsp, handle);
                    handle = blt_atts_findServiceGroupEnd(connHandle, handle);
                    U16_TO_STREAM(rsp, handle);
                    STR_TO_STREAM(rsp, pAttr->attrValue, attLen);
                    maxListCnt--;
                } else {
                    break;
                }
            }
        }
    }

    if (err) {
        return blt_att_packageErrorRsp(ATT_OP_READ_BY_GROUP_TYPE_REQ, startHandle, err, txBuf);
    }
    txBuf->opcode = ATT_OP_READ_BY_GROUP_TYPE_RSP;
    return rsp - &txBuf->opcode;
}

u16 blt_atts_proc_readMultiVarReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    const atts_attribute_t *pAttr      = NULL;
    atts_group_t           *pGroup     = NULL;
    int                     err        = ATT_SUCCESS;
    u16                     connHandle = pAttrConCb->connHandle;
    attr_pkt_t             *txBuf      = pAttrConCb->attTxBuff;
    u8                     *rsp        = (u8 *)txBuf->data;
    u16                     attHandle  = 0;
    u8                     *pAttVal    = attr->data;
    u8                      setOfHdls  = (attrLen - 1) >> 1;
    u16                     mtu        = blt_gap_getScidMtu(connHandle, pAttrConCb->scid);
    if (mtu < ATT_MTU_SIZE || (!blt_atts_initServiceDiscoverTick(connHandle))) {
        err = ATT_ERR_ATTR_NOT_FOUND;
    } else if (setOfHdls == 0 || attrLen > mtu) {
        setOfHdls = 0;
        err       = ATT_ERR_INVALID_HANDLE;
    }

    int maxRspValLen = mtu - 1;

    while (setOfHdls--) {
        STREAM_TO_U16(attHandle, pAttVal);
        pAttr = blt_atts_findByHandle(connHandle, attHandle, &pGroup);
        if (pAttr == NULL) {
            err = ATT_ERR_INVALID_HANDLE;
            break;
        }

        if ((err = blt_atts_permissions(connHandle, ATT_PERMISSIONS_READ, attHandle, pAttr->perm)) != ATT_SUCCESS) {
            break;
        }

        u8 *outValue    = pAttr->attrValue;
        u16 outValueLen = pAttr->attrValueLen ? *pAttr->attrValueLen : 0;
        ;
        if ((pAttr->settings & ATTS_SET_READ_CBACK) && (pGroup->readCback != NULL)) {
            //read callback return is ATT_SUCCESS
            err = pGroup->readCback(connHandle, ATT_OP_READ_MULTIPLE_VARIABLE_REQ, attHandle, &outValue, &outValueLen);
            if (err != ATT_SUCCESS) {
                break;
            }

            if (outValueLen == 0x0000) {
                outValue    = pAttr->attrValue;
                outValueLen = pAttr->attrValueLen ? *pAttr->attrValueLen : 0;
                ;
            }
        }

        /* If the Set Of Values parameter is longer than (ATT_MTU-1) then only the first (ATT_MTU-1) octets shall be included in this response. */
        maxRspValLen -= (outValueLen + 2);
        if (maxRspValLen < 0) {
            outValueLen += maxRspValLen;
            U16_TO_STREAM(rsp, *pAttr->attrValueLen);
            STR_TO_STREAM(rsp, outValue, outValueLen);
            break;
        }

        U16_TO_STREAM(rsp, *pAttr->attrValueLen);
        STR_TO_STREAM(rsp, pAttr->attrValue, *pAttr->attrValueLen);
    }

    if (err) {
        return blt_att_packageErrorRsp(ATT_OP_READ_MULTIPLE_VARIABLE_REQ, attHandle, err, txBuf);
    }

    txBuf->opcode = ATT_OP_READ_MULTIPLE_VARIABLE_RSP;

    /* If the Set Of Values parameter is longer than (ATT_MTU-1) then only the first (ATT_MTU-1) octets shall be included in this response. */
    return min(mtu, rsp - &txBuf->opcode);
}

u16 blt_atts_proc_write(attConCb_t *pAttrConCb, u8 opcode, u8 *pAttrData, u16 attrLen)
{
    u16 attrHandle = 0x0000;
    STREAM_TO_U16(attrHandle, pAttrData);
    attrLen -= 2; //attribute handle;
    const atts_attribute_t *pAttr;
    atts_group_t           *pGroup;
    u16                     connHandle = pAttrConCb->connHandle;
    int                     err        = ATT_SUCCESS;
    attr_pkt_t             *txBuf      = pAttrConCb->attTxBuff;

    pAttr = blt_atts_findByHandle(connHandle, attrHandle, &pGroup);

    if (pAttr == NULL) {
        err = ATT_ERR_INVALID_HANDLE;
    }

    if (!err && ((err = blt_atts_permissions(connHandle, ATT_PERMISSIONS_WRITE, attrHandle, pAttr->perm)) == ATT_SUCCESS)) {
        if ((pAttr->settings & ATTS_SET_WRITE_CBACK) && (pGroup->writeCback != NULL)) {
            err = pGroup->writeCback(connHandle, opcode, attrHandle, pAttrData, attrLen);
            if (err < ATT_SUCCESS || err >= 0x100) {
                return 0;
            }
            if (err != ATT_SUCCESS) {
                goto attsPushWriteRsp;
            }
        }

        if ((pAttr->settings & ATTS_SET_ALLOW_WRITE) != 0) {
            if (((pAttr->settings & ATTS_SET_VARIABLE_LEN) == 0) && (attrLen != pAttr->maxAttrLen)) {
                err = ATT_ERR_INVALID_ATTR_VALUE_LEN;
            } else if (((pAttr->settings & ATTS_SET_VARIABLE_LEN) != 0) && (attrLen > pAttr->maxAttrLen)) {
                err = ATT_ERR_INVALID_ATTR_VALUE_LEN;
            } else {
                memcpy(pAttr->attrValue, pAttrData, attrLen);
                if (*(pAttr->attrValueLen) != attrLen) {
                    *(pAttr->attrValueLen) = attrLen;
                }
            }
        }
    }

attsPushWriteRsp:

    if (err) {
        return blt_att_packageErrorRsp(opcode, attrHandle, err, txBuf);
    }

    txBuf->opcode = ATT_OP_WRITE_RSP;
    return 1;
}

u16 blt_atts_proc_writeReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    return blt_atts_proc_write(pAttrConCb, attr->opcode, attr->data, attrLen - ATT_HDR_LEN);
}

#if PREPARE_WRITE_QUEUE_MODE

typedef struct
{
    u16 packLen;
    u8  data[23]; //TODO: MTU
} atts_queue_t;

typedef struct
{
    u16           connHandle;
    u8            queueIndex;
    u8            queueSize;
    atts_queue_t *queuePtr;
} atts_pre_write_queue_t;

atts_queue_t attPreWriteBuff[20]; //TODO: MTU;

atts_pre_write_queue_t attsPreWriteQueue = {
    .queueIndex = 0,
    .queueSize  = 20,
    .queuePtr   = &attPreWriteBuff[0],
};

u16 blt_atts_proc_prepareWriteReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    u8 *pAttrData  = attr->data;
    u16 attrHandle = 0x0000;
    STREAM_TO_U16(attrHandle, pAttrData);

    const atts_attribute_t *pAttr      = NULL;
    atts_group_t           *pGroup     = NULL;
    u8                      err        = ATT_SUCCESS;
    u16                     connHandle = pAttrConCb->connHandle;
    attr_pkt_t             *txBuf      = pAttrConCb->attTxBuff;

    pAttr = blt_atts_findByHandle(connHandle, attrHandle, &pGroup);
    if (pAttr == NULL) {
        err = ATT_ERR_INVALID_HANDLE;
    } else if ((err = blt_atts_permissions(connHandle, ATT_PERMISSIONS_WRITE, attrHandle, pAttr->perm)) == ATT_SUCCESS) {
    }

    if (!err) {
        if ((attsPreWriteQueue.connHandle && attsPreWriteQueue.connHandle != connHandle) ||
            attsPreWriteQueue.queueIndex >= attsPreWriteQueue.queueSize) {
            err = ATT_ERR_PREPARE_QUEUE_FULL;
            goto attsPushPrepareWriteRsp;
        }

        attsPreWriteQueue.connHandle = connHandle;

        atts_queue_t *ptr = attsPreWriteQueue.queuePtr + attsPreWriteQueue.queueIndex; //TODO: MTU;
        memcpy((u8 *)ptr->data, (u8 *)attr, attrLen);
        ptr->packLen = attrLen;
        attsPreWriteQueue.queueIndex++;
        memcpy((u8 *)txBuf, (u8 *)attr, attrLen);
    }

attsPushPrepareWriteRsp:
    if (err) {
        return blt_att_packageErrorRsp(ATT_OP_PREPARE_WRITE_REQ, attrHandle, err, txBuf);
    }
    txBuf->opcode = ATT_OP_PREPARE_WRITE_RSP;
    return attrLen;
}

static int blt_atts_prepareWriteReq(u16 connHandle, u16 handle, u8 *data, u16 length)
{
    const atts_attribute_t *pAttr  = NULL;
    atts_group_t           *pGroup = NULL;

    pAttr = blt_atts_findByHandle(connHandle, handle, &pGroup);
    if (pAttr == 0) {
        return ATT_ERR_INVALID_HANDLE;
    }

    if ((pAttr->settings & ATTS_SET_WRITE_CBACK) && (pGroup->writeCback != NULL)) {
        int err = pGroup->writeCback(connHandle, ATT_OP_PREPARE_WRITE_REQ, handle, data, length);
        if (err < ATT_SUCCESS || err >= 0x100) {
            return -1;
        }

        if (err != ATT_SUCCESS) {
            return err;
        }

        if ((pAttr->settings & ATTS_SET_ALLOW_WRITE) != 0) {
            if (((pAttr->settings & ATTS_SET_VARIABLE_LEN) == 0) && (length != pAttr->maxAttrLen)) {
                return ATT_ERR_INVALID_ATTR_VALUE_LEN;
            } else if (((pAttr->settings & ATTS_SET_VARIABLE_LEN) != 0) && (length > pAttr->maxAttrLen)) {
                return ATT_ERR_INVALID_ATTR_VALUE_LEN;
            } else {
                memcpy(pAttr->attrValue, data, length);
                if (*(pAttr->attrValueLen) != length) {
                    *(pAttr->attrValueLen) = length;
                }
            }
        }
    }

    return ATT_SUCCESS;
}

u16 blt_atts_proc_executeWriteReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    u8 *pAttrData = attr->data;
    u8  flags     = 0x0000;
    STREAM_TO_U8(flags, pAttrData);

    int         err        = ATT_SUCCESS;
    u16         connHandle = pAttrConCb->connHandle;
    attr_pkt_t *txBuf      = pAttrConCb->attTxBuff;

    u16 handle = 0;

    if (flags == ATT_EXEC_WRITE_ALL) {
        u8  valueList[2][100];
        u16 handleList[2] = {0x0000, 0x0000};
        u16 offsetList[2] = {0x0000, 0x0000};

        u8  *valuePtr  = NULL;
        u16  offset    = 0;
        u16 *offsetPtr = NULL;

        for (int i = 0; i < attsPreWriteQueue.queueIndex; i++) {
            atts_queue_t *ptr = attsPreWriteQueue.queuePtr + i; //TODO: MTU
            u8           *req = ptr->data + ATT_HDR_LEN;
            STREAM_TO_U16(handle, req);
            STREAM_TO_U16(offset, req);
            if (handle == handleList[0] || handleList[0] == 0x0000) {
                handleList[0] = handle;
                valuePtr      = &valueList[0][0] + offsetList[0];
                offsetPtr     = &offsetList[0];
            } else {
                handleList[1] = handle;
                valuePtr      = &valueList[1][0] + offsetList[1];
                offsetPtr     = &offsetList[1];
            }

            if (offset != *offsetPtr) {
                err = ATT_ERR_INVALID_OFFSET;
                goto attsPushExecuteWriteRsp;
            }
            memcpy(valuePtr, req, ptr->packLen - 5);
            *offsetPtr += ptr->packLen - 5;
        }

        if (handleList[0]) {
            err = blt_atts_prepareWriteReq(connHandle, handleList[0], &valueList[0][0], offsetList[0]);

            if (err < ATT_SUCCESS || err >= 0x100) {
                return 0;
            }

            if (err) {
                goto attsPushExecuteWriteRsp;
            }
        }

        if (handleList[1]) {
            err = blt_atts_prepareWriteReq(connHandle, handleList[1], &valueList[1][0], offsetList[1]);

            if (err < ATT_SUCCESS || err >= 0x100) {
                return 0;
            }

            if (err) {
                goto attsPushExecuteWriteRsp;
            }
        }
    } else if (flags == ATT_EXEC_WRITE_CANCEL) {
    } else {
        err = ATT_ERR_INVALID_PDU;
    }

attsPushExecuteWriteRsp:
    attsPreWriteQueue.connHandle = 0x0000;
    attsPreWriteQueue.queueIndex = 0;

    if (err) {
        return blt_att_packageErrorRsp(ATT_OP_EXECUTE_WRITE_REQ, handle, err, txBuf);
    }
    txBuf->opcode = ATT_OP_EXECUTE_WRITE_RSP;
    return ATT_HDR_LEN;
}

#else
extern att_pre_write_buff_t att_pre_write;

u16 blt_atts_proc_prepareWriteReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    u8 *pAttrData   = attr->data;
    u16 attrHandle  = 0x0000;
    u16 valueOffset = 0x0000;
    STREAM_TO_U16(attrHandle, pAttrData);
    STREAM_TO_U16(valueOffset, pAttrData);

    const atts_attribute_t *pAttr      = NULL;
    atts_group_t           *pGroup     = NULL;
    u8                      err        = ATT_SUCCESS;
    u16                     connHandle = pAttrConCb->connHandle;
    attr_pkt_t             *txBuf      = pAttrConCb->attTxBuff;

    pAttr = blt_atts_findByHandle(connHandle, attrHandle, &pGroup);
    if (pAttr == NULL) {
        err = ATT_ERR_INVALID_HANDLE;
    } else if ((err = blt_atts_permissions(connHandle, ATT_PERMISSIONS_WRITE, attrHandle, pAttr->perm)) == ATT_SUCCESS) {
    }

    if (!err) {
        if (valueOffset == 0x00) {
            att_pre_write.handle = connHandle;
            memcpy(att_pre_write.buff, &attr->opcode, 3);
        } else if (att_pre_write.offset != valueOffset) {
            att_pre_write.handle = 0xFFFE;
        }

        u16 prepare_pkt_len = valueOffset + attrLen - ATT_PREP_WRITE_REQ_LEN;

        if (prepare_pkt_len > att_pre_write.buffMaxLen) {
            err = ATT_ERR_PREPARE_QUEUE_FULL;
            goto attsPushPrepareWriteRsp;
        }

        att_pre_write.offset = prepare_pkt_len;
        memcpy(att_pre_write.buff + 3 + valueOffset, pAttrData, attrLen - ATT_PREP_WRITE_REQ_LEN);
        memcpy((u8 *)txBuf, (u8 *)attr, attrLen);
    }

attsPushPrepareWriteRsp:
    if (err) {
        return blt_att_packageErrorRsp(ATT_OP_PREPARE_WRITE_REQ, attrHandle, err, txBuf);
    }
    txBuf->opcode = ATT_OP_PREPARE_WRITE_RSP;
    return attrLen;
}

u16 blt_atts_proc_executeWriteReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    (void)attrLen;
    u8 *pAttrData = attr->data;
    u8  flags     = 0x0000;
    STREAM_TO_U8(flags, pAttrData);

    const atts_attribute_t *pAttr      = NULL;
    atts_group_t           *pGroup     = NULL;
    int                     err        = ATT_SUCCESS;
    u16                     connHandle = pAttrConCb->connHandle;
    attr_pkt_t             *txBuf      = pAttrConCb->attTxBuff;
    u16                     handle     = 0;
    u8                     *req        = att_pre_write.buff + ATT_HDR_LEN;

    if (att_pre_write.handle == 0xFFFE) {
        err = ATT_ERR_INVALID_OFFSET;
        goto attsPushExecuteWriteRsp;
    }

    STREAM_TO_U16(handle, req);
    pAttr = blt_atts_findByHandle(connHandle, handle, &pGroup);
    if (pAttr == NULL) {
        err    = ATT_ERR_INVALID_HANDLE;
        handle = 0;
    }
    if (att_pre_write.handle != 0xffff && !err && ((err = blt_atts_permissions(connHandle, ATT_PERMISSIONS_WRITE, handle, pAttr->perm)) == ATT_SUCCESS)) {
        if (flags == ATT_EXEC_WRITE_ALL) {
            if ((pAttr->settings & ATTS_SET_WRITE_CBACK) && (pGroup->writeCback != NULL)) {
                err = pGroup->writeCback(connHandle, ATT_OP_PREPARE_WRITE_REQ, handle, req, att_pre_write.offset);
                if (err < ATT_SUCCESS || err >= 0x100) {
                    return 0;
                }
                if (err != ATT_SUCCESS) {
                    goto attsPushExecuteWriteRsp;
                }
            }

            u16 writeLen = att_pre_write.offset;

            if ((pAttr->settings & ATTS_SET_ALLOW_WRITE) != 0) {
                if (((pAttr->settings & ATTS_SET_VARIABLE_LEN) == 0) && (writeLen != pAttr->maxAttrLen)) {
                    err = ATT_ERR_INVALID_ATTR_VALUE_LEN;
                } else if (((pAttr->settings & ATTS_SET_VARIABLE_LEN) != 0) && (writeLen > pAttr->maxAttrLen)) {
                    err = ATT_ERR_INVALID_ATTR_VALUE_LEN;
                } else {
                    memcpy(pAttr->attrValue, req, writeLen);
                    if (*(pAttr->attrValueLen) != writeLen) {
                        *(pAttr->attrValueLen) = writeLen;
                    }
                }
            }
        } else if (flags == ATT_EXEC_WRITE_CANCEL) {
        } else {
            err = ATT_ERR_INVALID_PDU;
        }
    }

attsPushExecuteWriteRsp:
    att_pre_write.offset = 0x0000;
    att_pre_write.handle = 0xffff;

    if (err) {
        return blt_att_packageErrorRsp(ATT_OP_EXECUTE_WRITE_REQ, handle, err, txBuf);
    }
    txBuf->opcode = ATT_OP_EXECUTE_WRITE_RSP;
    return ATT_HDR_LEN;
}

#endif

u16 blt_atts_proc_writeCmd(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    blt_atts_proc_write(pAttrConCb, attr->opcode, attr->data, attrLen - ATT_HDR_LEN);
    return 0; //command not need response
}

u16 blt_atts_proc_signedWriteCmd(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    (void)pAttrConCb;
    (void)attr;
    (void)attrLen;
    return 0; //command not need response
}

u16 blt_atts_proc_handleValueCfm(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen)
{
    (void)attr;
    (void)attrLen;
    gap_ms_para_t *pGap_ms_para = blc_gap_getMasterSlavePara(pAttrConCb->connHandle);

    if (gap_eventMask & GAP_EVT_MASK_GATT_HANDLE_VALUE_CONFIRM) {
#if (CUSTOM_DARWIN_FMN_ENABLE)
        if (custom_darwin_fmn.darwin_fmn_enable) {
            u16 report_handle[2];
            report_handle[0] = pGap_ms_para->indicate_handle;
            report_handle[1] = pAttrConCb->connHandle;
            blc_gap_send_event(GAP_EVT_GATT_HANDLE_VALUE_CONFIRM, (u8 *)&report_handle ,sizeof(report_handle)); //app_host_event_callback
        } else
#endif
        {
            u16 report_handle[2];
            report_handle[0] = pAttrConCb->connHandle;
            report_handle[1] = pGap_ms_para->indicate_handle;
            blc_gap_send_event ( GAP_EVT_GATT_HANDLE_VALUE_CONFIRM, (u8 *)&report_handle ,sizeof(report_handle));
        }
    }

    pGap_ms_para->indicate_handle = 0;

    blt_gatts_recvIndCfm(pAttrConCb->connHandle, pAttrConCb->scid);
    return 0;
}

////////////////////// ATT Server Send packet concerned /////////////////////////////
ble_sts_t blc_atts_sendErrResponse(u16 connHandle, u8 reqOpcode, u16 attHdlInErr, u8 ErrorCode)
{
    if (blt_ll_isAclhdlInvalid(connHandle)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

#if (0)
    attr_pkt_t *pAttTxPkt = (attr_pkt_t *)blt_l2cap_get_tx_buff(connHandle);
    blt_att_packageErrorRsp(reqOpcode, attHdlInErr, ErrorCode, pAttTxPkt);

    return blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, &pAttTxPkt->opcode, 1, pAttTxPkt->data, pAttTxPkt->dataLen);
#else
    u8 format[1];
    format[0] = ATT_OP_ERROR_RSP;

    u8 temp[4];
    temp[0] = reqOpcode;
    temp[1] = U16_LO(attHdlInErr);
    temp[2] = U16_HI(attHdlInErr);
    temp[3] = ErrorCode;
    return blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 1, (u8 *)temp, 4);
#endif
}

ble_sts_t blc_atts_sendHandleValueNotify(u16 connHandle, u16 attHandle, u8 *p, int len)
{
    if (blt_ll_isAclhdlInvalid(connHandle)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

#if (0) //Do not use l2cap_tx_buff
    attr_pkt_t *pAttTxPkt = blt_l2cap_get_tx_buff(connHandle);
    blc_att_prepareNotify(attHandle, len, p, pAttTxPkt);

    return blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, &pAttTxPkt->opcode, 1, pAttTxPkt->data, pAttTxPkt->dataLen);
#else
    //HandleValueNotify format
    u8 format[4];
    format[0] = ATT_OP_HANDLE_VALUE_NOTI;
    format[1] = U16_LO(attHandle);
    format[2] = U16_HI(attHandle);

    return blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 3, p, len);
#endif
}

///indicate need to wait confirm
ble_sts_t blc_atts_sendHandleValueIndicate(u16 connHandle, u16 attHandle, u8 *p, int len)
{
    if (p == NULL || len == 0) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    if (blt_ll_isAclhdlInvalid(connHandle)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    //HandleValueIndicate format
    u8 format[4];
    format[0] = ATT_OP_HANDLE_VALUE_IND;
    format[1] = U16_LO(attHandle);
    format[2] = U16_HI(attHandle);

    u8 api_status = (u8)blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 3, p, len);
    if (api_status == BLE_SUCCESS) {
        gap_ms_para_t *pGap_ms_para   = blc_gap_getMasterSlavePara(connHandle);
        pGap_ms_para->indicate_handle = attHandle;
    }

    return api_status;
}

ble_sts_t blc_atts_sendMultHandleValueNtf(u16 connHandle, atts_multHandleNtf_t *ntf, int count)
{
    if (ntf == NULL || count < 2) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    if (blt_ll_isAclhdlInvalid(connHandle)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    //HandleValueIndicate format
    u8 format[1];
    format[0] = ATT_OP_MULTIPLE_HANDLE_VALUE_NTF;

    u8  buff[512];
    u16 len = 0;

    u8 *p = buff;
    for (int i = 0; i < count; i++) {
        U16_TO_STREAM(p, ntf->handle);
        U16_TO_STREAM(p, ntf->length);
        STR_TO_STREAM(p, ntf->value, ntf->length);
        len += ntf->length + 4;
        ntf++;
    }

    return blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 1, buff, len);
}

////////////////////// ATT Server UUID compare ////////////////////////////////
bool blt_atts_uuidCmp(const atts_attribute_t *pAttr, u8 uuidLen, const u8 *pUuid)
{
    if (pAttr->uuidLen == uuidLen) {
        return (memcmp(pAttr->uuid, pUuid, uuidLen) == 0);
    } else if ((pAttr->uuidLen == ATT_128_UUID_LEN) && uuidLen == ATT_16_UUID_LEN) {
        return blt_uuid_cmp16to128(pUuid, pAttr->uuid);
    } else if ((pAttr->uuidLen == ATT_16_UUID_LEN) && uuidLen == ATT_128_UUID_LEN) {
        return blt_uuid_cmp16to128(pAttr->uuid, pUuid);
    } else {
        return false;
    }
}

bool blt_atts_attrCmp(const atts_attribute_t *pAttr, u8 uuidLen, const u8 *pUuid)
{
    if (*pAttr->attrValueLen == uuidLen) {
        return (memcmp(pAttr->attrValue, pUuid, uuidLen) == 0);
    } else if ((*pAttr->attrValueLen == ATT_128_UUID_LEN) && uuidLen == ATT_16_UUID_LEN) {
        return blt_uuid_cmp16to128(pUuid, pAttr->attrValue);
    } else if ((*pAttr->attrValueLen == ATT_16_UUID_LEN) && uuidLen == ATT_128_UUID_LEN) {
        return blt_uuid_cmp16to128(pAttr->attrValue, pUuid);
    } else {
        return false;
    }
}

bool blt_uuid_compare(u8 *pUuid1, u8 uuidLen1, u8 *pUuid2, u8 uuidLen2)
{
    if ((uuidLen1 != ATT_16_UUID_LEN && uuidLen1 != ATT_128_UUID_LEN) ||
        (uuidLen2 != ATT_16_UUID_LEN && uuidLen2 != ATT_128_UUID_LEN)) {
        return false;
    }

    if (uuidLen1 == uuidLen2) {
        return (memcmp(pUuid1, pUuid2, uuidLen1) == 0);
    } else if (uuidLen1 == ATT_16_UUID_LEN) {
        return blt_uuid_cmp16to128(pUuid1, pUuid2);
    } else if (uuidLen1 == ATT_128_UUID_LEN) {
        return blt_uuid_cmp16to128(pUuid2, pUuid1);
    }
    return false;
}

////////////////////////////////////////
///ATT permissions
////////////////////////////////////////
u8 blt_atts_permissions(u16 connHandle, u8 permit, u8 handle, u8 permissions)
{
#if DUAL_CORE_MODE_ENABLED
    return ATT_SUCCESS;
#endif

    (void)handle;
    if (!(permit & permissions)) {
        return (permit & ATT_PERMISSIONS_READ) ? ATT_ERR_READ_NOT_PERMITTED : ATT_ERR_WRITE_NOT_PERMITTED;
    }

    if (permissions & ATT_PERMISSIONS_SECURITY) {
        return blt_gatt_requestServiceAccess(connHandle, permissions);
    }

    return ATT_SUCCESS;
}

////////////////////////////////////////
///ATT attribute function
////////////////////////////////////////
static u16 blt_atts_findServiceUuid(u16 connHandle, u8 uuidLen, const u8 *pUuid, atts_group_t **pAttrGroup)
{
    if (uuidLen != ATT_16_UUID_LEN && uuidLen != ATT_128_UUID_LEN) {
        my_dump_str_data(STACK_DUMP_EN, "err: uuid length", &uuidLen, 1);
        *pAttrGroup = NULL;
        return ATT_HANDLE_NONE;
    }


    atts_group_t *pGroup = blc_gatts_getAttributeServiceGroup(connHandle);

    if (pGroup == NULL) {
        my_dump_str_data(STACK_DUMP_EN, "err: no attribute table", 0, 0);
        *pAttrGroup = NULL;
        return ATT_HANDLE_NONE;
    }
    const atts_attribute_t *pAttr = NULL;
    for (; pGroup != NULL; pGroup = pGroup->pNext) {
        for (int i = 0; i <= pGroup->endHandle - pGroup->startHandle; i++) {
            pAttr = &pGroup->pAttr[i];
            if (blt_atts_uuidCmp(pAttr, ATT_16_UUID_LEN, declarationsPrimaryServiceUuid) || blt_atts_uuidCmp(pAttr, ATT_16_UUID_LEN, declarationsSecondaryServiceUuid)) {
                if (blt_atts_attrCmp(pAttr, uuidLen, pUuid)) {
                    *pAttrGroup = pGroup;
                    return i + pGroup->startHandle;
                }
            }
        }
    }

    *pAttrGroup = NULL;
    return ATT_HANDLE_NONE;
}

static u8 blt_atts_findChar(const atts_attribute_t *pAttr, u16 attrHdl, int attrLen, const atts_findCharList_t *charList, u16 charListLen, void *p)
{
#define FIND_CHAR_MAX_NUM 50
    if (FIND_CHAR_MAX_NUM < charListLen || attrLen <= 1) {
        my_dump_str_data(STACK_DUMP_EN, "err: char list too long", &charListLen, 2);
        return false;
    }

    atts_foundCharParam_t  charParam;
    atts_foundCharParam_t *pParam = &charParam;
    u8                     charNum[FIND_CHAR_MAX_NUM];
    memset(charNum, 0, FIND_CHAR_MAX_NUM);

    do {
        if (attrLen > 1 && blt_atts_uuidCmp(pAttr, ATT_16_UUID_LEN, declarationsCharacteristicUuid)) {
            pAttr++;
            attrLen--;
            attrHdl++;

            for (int i = 0; i < charListLen; i++) {
                if (blt_atts_uuidCmp(pAttr, charList[i].charUuidLen, charList[i].charUuid) && charList[i].foundCback != NULL) {
                    pParam->charHandle  = attrHdl;
                    pParam->num         = charNum[i];
                    pParam->charData    = pAttr->attrValue;
                    pParam->charDataLen = pAttr->attrValueLen;
                    pAttr++;
                    attrHdl++;
                    attrLen--;
                    if (attrLen > 0 && blt_atts_uuidCmp(pAttr, ATT_16_UUID_LEN, descriptorClientCharacteristicConfigurationUuid)) {
                        pParam->CCC       = pAttr->attrValue;
                        pParam->cccHandle = attrHdl;
                        pAttr++;
                        attrHdl++;
                        attrLen--;
                    } else if (attrLen > 1 && blt_atts_uuidCmp(pAttr + 1, ATT_16_UUID_LEN, descriptorClientCharacteristicConfigurationUuid)) {
                        pParam->CCC       = (pAttr + 1)->attrValue;
                        pParam->cccHandle = attrHdl + 1;
                        pAttr += 2;
                        attrHdl += 2;
                        attrLen -= 2;
                    } else {
                        pParam->CCC = NULL;
                    }

                    if (charList[i].foundCback) {
                        charList[i].foundCback(pParam, p);
                    }
                    charNum[i]++;
                    break;
                }
            }
        } else {
            pAttr++;
            attrHdl++;
            attrLen--;
        }

    } while (attrLen > 0);

    return true;
}

static u8 blt_atts_findServiceInfo(u16 connHandle, u16 startHdl, u16 endHdl, u8 uuidLen, const u8 *pUuid, atts_group_t **pAttrGroup)
{
    (void)endHdl;
    if (uuidLen != ATT_16_UUID_LEN && uuidLen != ATT_128_UUID_LEN) {
        my_dump_str_data(STACK_DUMP_EN, "err: uuid length", &uuidLen, 1);
        *pAttrGroup = NULL;
        return ATT_HANDLE_NONE;
    }


    atts_group_t *pGroup = blc_gatts_getAttributeServiceGroup(connHandle);

    if (pGroup == NULL) {
        my_dump_str_data(STACK_DUMP_EN, "err: no attribute table", 0, 0);
        *pAttrGroup = NULL;
        return ATT_HANDLE_NONE;
    }

    const atts_attribute_t *pAttr = NULL;
    for (; pGroup != NULL; pGroup = pGroup->pNext) {
        for (int i = 0; i <= pGroup->endHandle - pGroup->startHandle; i++) {
            pAttr = &pGroup->pAttr[i];
            if (blt_atts_uuidCmp(pAttr, ATT_16_UUID_LEN, declarationsPrimaryServiceUuid) || blt_atts_uuidCmp(pAttr, ATT_16_UUID_LEN, declarationsSecondaryServiceUuid)) {
                if (blt_atts_attrCmp(pAttr, uuidLen, pUuid) && startHdl == i + pGroup->startHandle) {
                    *pAttrGroup = pGroup;
                    return i + pGroup->startHandle;
                }
            }
        }
    }

    *pAttrGroup = NULL;
    return ATT_HANDLE_NONE;
}

static u8 blt_atts_findInclInfo(const atts_attribute_t *pAttr, u16 attrHdl, int attrLen, const atts_findServiceList_t *serviceList, void *p)
{
    int                     attrLenTemp = attrLen;
    const atts_attribute_t *pAttrTemp   = pAttr;
    do {
        if (attrLen > 1 && blt_atts_uuidCmp(pAttr, ATT_16_UUID_LEN, declarationsIncludeUuid)) {
            typedef struct
            {
                u16 startHdl;
                u16 endHdl;
                u8  uuid[0];
            } includeAttrValue_t;

            includeAttrValue_t *inclValue = (includeAttrValue_t *)pAttr->attrValue;
            u16                 uuidSize  = *pAttr->attrValueLen - 4;

            for (int i = 0; i < serviceList->inclSize; i++) {
                const atts_findInclList_t *incl = serviceList->inclList[i];
                if (!blt_uuid_compare(inclValue->uuid, uuidSize, (u8 *)(u32)incl->inclUuid, incl->inclUuidLen)) {
                    continue;
                }

                atts_group_t *pAttrGroup = NULL;
                blt_atts_findServiceInfo(0xFFFF, inclValue->startHdl, inclValue->endHdl, uuidSize, inclValue->uuid, &pAttrGroup);
                if (pAttrGroup != NULL && incl->foundCback && incl->foundCback(p)) {
                    blt_atts_findChar(&pAttrGroup->pAttr[inclValue->startHdl - pAttrGroup->startHandle],
                                      inclValue->startHdl,
                                      inclValue->endHdl - inclValue->startHdl + 1,
                                      incl->charList,
                                      incl->charSize,
                                      p);
                }
            }

            pAttr++;
            attrLen--;
        } else {
            pAttr++;
            attrLen--;
        }

    } while (attrLen > 0 && serviceList->inclSize);

    blt_atts_findChar(pAttrTemp, attrHdl, attrLenTemp, serviceList->charList, serviceList->charSize, p);

    return true;
}

int blc_atts_findCharacteristic(const atts_findServiceList_t *serviceList, void *p)
{
    atts_group_t *pAttrGroup;

    u16 startHandle = blt_atts_findServiceUuid(0xFFFF, serviceList->serviceUuidLen, serviceList->serviceUuid, &pAttrGroup);

    if (pAttrGroup == NULL) {
        return -1;
    }

    if (blt_atts_findInclInfo(&pAttrGroup->pAttr[startHandle - pAttrGroup->startHandle], startHandle, pAttrGroup->endHandle - startHandle + 1, serviceList, p)) {
        return 0;
    }

    return 0;
}

int blc_atts_findCharacteristicByServiceUuid(const u8 *serviceUuid, u8 uuidLen, const atts_findCharList_t *charList, u16 charListLen, void *p)
{
    atts_group_t *pAttrGroup;

    u16 startHandle = blt_atts_findServiceUuid(0xFFFF, uuidLen, serviceUuid, &pAttrGroup);

    if (pAttrGroup == NULL) {
        return -1;
    }

    if (blt_atts_findChar(&pAttrGroup->pAttr[startHandle - pAttrGroup->startHandle], startHandle, pAttrGroup->endHandle - startHandle + 1, charList, charListLen, p)) {
        return 0;
    }

    return -2; //
}

u16 blt_atts_findInRange(u16 connHandle, u16 startHandle, u16 endHandle, const atts_attribute_t **pAttr)
{
    for (atts_group_t *pGroup = blc_gatts_getAttributeServiceGroup(connHandle); pGroup != NULL; pGroup = pGroup->pNext) {
        if ((startHandle < pGroup->startHandle) && (endHandle >= pGroup->startHandle)) {
            startHandle = pGroup->startHandle;
        }

        if ((startHandle >= pGroup->startHandle) && (startHandle <= pGroup->endHandle)) {
            *pAttr = &pGroup->pAttr[startHandle - pGroup->startHandle];
            return startHandle;
        }
    }

    return ATT_HANDLE_NONE;
}

u16 blt_atts_findUuidInRange(u16 connHandle, u16 startHandle, u16 endHandle, u8 uuidLen, u8 *pUuid, const atts_attribute_t **pAttr, atts_group_t **pAttrGroup)
{
    atts_group_t *pGroup = blc_gatts_getAttributeServiceGroup(connHandle);

    if (pGroup == NULL) {
        return ATT_HANDLE_NONE;
    }

    for (; pGroup != NULL; pGroup = pGroup->pNext) {
        if ((startHandle < pGroup->startHandle) && (endHandle >= pGroup->startHandle)) {
            startHandle = pGroup->startHandle;
        }
        if ((startHandle >= pGroup->startHandle) && (startHandle <= pGroup->endHandle)) {
            *pAttr = &pGroup->pAttr[startHandle - pGroup->startHandle];
            while ((startHandle <= pGroup->endHandle) && (startHandle <= endHandle)) {
                if (blt_atts_uuidCmp(*pAttr, uuidLen, pUuid)) {
                    *pAttrGroup = pGroup;
                    return startHandle;
                }
                if (startHandle == ATT_HANDLE_MAX) {
                    break;
                }
                startHandle++;
                (*pAttr)++;
            }
        }
    }
    return ATT_HANDLE_NONE;
}

u16 blt_atts_findServiceGroupEnd(u16 connHandle, u16 startHandle)
{
    const atts_attribute_t *pAttr;
    uint16_t                prevHandle;

    if (startHandle == ATT_HANDLE_MAX) {
        return ATT_HANDLE_MAX;
    }

    prevHandle = startHandle;
    startHandle++;

    atts_group_t *pGroup = blc_gatts_getAttributeServiceGroup(connHandle);

    if (pGroup == NULL) {
        return ATT_HANDLE_MAX;
    }

    for (; pGroup != NULL; pGroup = pGroup->pNext) {
        if (startHandle < pGroup->startHandle) {
            startHandle = pGroup->startHandle;
        }

        if (startHandle <= pGroup->endHandle) {
            pAttr = &pGroup->pAttr[startHandle - pGroup->startHandle];
            while (startHandle <= pGroup->endHandle) {
                if (blt_atts_uuidCmp(pAttr, ATT_16_UUID_LEN, declarationsPrimaryServiceUuid) ||
                    blt_atts_uuidCmp(pAttr, ATT_16_UUID_LEN, declarationsSecondaryServiceUuid)) {
                    return prevHandle;
                }

                if (startHandle == ATT_HANDLE_MAX) {
                    return ATT_HANDLE_MAX;
                }

                prevHandle = startHandle;
                startHandle++;
                pAttr++;
            }
            if (startHandle == pGroup->endHandle + 1) {
                return prevHandle;
            }
        }
    }

    return ATT_HANDLE_MAX;
}

const atts_attribute_t *blt_atts_findByHandle(u16 connHandle, u16 handle, atts_group_t **pAttrGroup)
{
    atts_group_t *pGroup = blc_gatts_getAttributeServiceGroup(connHandle);

    if (pGroup == NULL) {
        return NULL;
    }

    for (; pGroup != NULL; pGroup = pGroup->pNext) {
        if ((handle >= pGroup->startHandle) && (handle <= pGroup->endHandle)) {
            if (pAttrGroup) {
                *pAttrGroup = pGroup;
            }
            return &pGroup->pAttr[handle - pGroup->startHandle];
        }
    }

    return NULL;
}

bool blt_atts_initServiceDiscoverTick(u16 connHandle)
{
    gap_server_para_t *pGap_server_para = blt_gap_getServerPara(connHandle);
    if (pGap_server_para == NULL) {
        return false;
    }

    pGap_server_para->att_service_discover_tick = clock_time() | 1;
    return true;
}
