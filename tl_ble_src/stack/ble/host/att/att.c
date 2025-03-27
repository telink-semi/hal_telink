/********************************************************************************************************
 * @file    att.c
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
#include "stack/ble/ble.h"

#include "att.h"



/** Dispatch table for incoming ATT requests.  Sorted by op code. */
typedef struct {
    u8 op;
    u8 type;
    u16 expect_len;
    u16 (*att_rx_fn) (attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
}att_rx_proc_t;

#define ATTS_PROC_FUN(op)           blt_atts_proc_##op
#define ATTS_RX_STR(op, op2, type)  {ATT_OP_##op, type, sizeof(blt_attr_##op2##_t), ATTS_PROC_FUN(op2)}
#define ATTS_REQ_RX_STR(op, op2)    ATTS_RX_STR(op##_REQ, op2##Req, ATT_REQUEST)
#define ATTS_CMD_RX_STR(op, op2)    ATTS_RX_STR(op##_CMD, op2##Cmd, ATT_COMMAND)
#define ATTS_CFM_RX_STR(op, op2)    ATTS_RX_STR(op##_CFM, op2##Cfm, ATT_CONFIRMATION)

#define ATTC_PROC_FUN(op)           blt_attc_proc_##op
#define ATTC_RX_STR(op, op2, type)  {ATT_OP_##op, type, sizeof(blt_attr_##op2##_t), ATTC_PROC_FUN(op2)}
#define ATTC_RSP_RX_STR(op, op2)    ATTC_RX_STR(op##_RSP, op2##Rsp, ATT_RESPONSE)
#define ATTC_IND_RX_STR(op, op2)    ATTC_RX_STR(op##_IND, op2##Ind, ATT_INDICATION)
#define ATTC_NTF_RX_STR(op, op2)    ATTC_RX_STR(op##_NTF, op2##Ntf, ATT_NOTIFICATION)

/** Dispatch table for incoming ATT commands.  Must be ordered by op code. */
static const att_rx_proc_t attr_rx_handlers[] =
{
    /* ATT Server concerned */
#if (1)
    ATTS_REQ_RX_STR(EXCHANGE_MTU, exchangeMtu),
    ATTS_REQ_RX_STR(FIND_INFO, findInfo),
    ATTS_REQ_RX_STR(FIND_BY_TYPE_VALUE, findByTypeValue),
    ATTS_REQ_RX_STR(READ_BY_TYPE, readByType),
    ATTS_REQ_RX_STR(READ, read),
    ATTS_REQ_RX_STR(READ_BLOB, readBlob),
    ATTS_REQ_RX_STR(READ_MULTI, readMulti),
    ATTS_REQ_RX_STR(READ_BY_GROUP_TYPE, readByGroupType),
    ATTS_REQ_RX_STR(READ_MULTIPLE_VARIABLE, readMultiVar),
    ATTS_REQ_RX_STR(WRITE, write),
    ATTS_REQ_RX_STR(PREPARE_WRITE, prepareWrite),
    ATTS_REQ_RX_STR(EXECUTE_WRITE, executeWrite),
    ATTS_CMD_RX_STR(WRITE, write),
    ATTS_CMD_RX_STR(SIGNED_WRITE, signedWrite),
    ATTS_CFM_RX_STR(HANDLE_VALUE, handleValue),
#else /* The code is easier to view */
    {   ATT_OP_EXCHANGE_MTU_REQ,            ATT_REQUEST,        sizeof(blt_attr_exchangeMtuReq_t),      blt_atts_proc_exchangeMtuReq        },
    {   ATT_OP_FIND_INFO_REQ,               ATT_REQUEST,        sizeof(blt_attr_findInfoReq_t),         blt_atts_proc_findInfoReq           },
    {   ATT_OP_FIND_BY_TYPE_VALUE_REQ,      ATT_REQUEST,        sizeof(blt_attr_findByTypeValueReq_t),  blt_atts_proc_findByTypeValueReq    },
    {   ATT_OP_READ_BY_TYPE_REQ,            ATT_REQUEST,        sizeof(blt_attr_readByTypeReq_t),       blt_atts_proc_readByTypeReq         },
    {   ATT_OP_READ_REQ,                    ATT_REQUEST,        sizeof(blt_attr_readReq_t),             blt_atts_proc_readReq               },
    {   ATT_OP_READ_BLOB_REQ,               ATT_REQUEST,        sizeof(blt_attr_readBlobReq_t),         blt_atts_proc_readBlobReq           },
    {   ATT_OP_READ_MULTI_REQ,              ATT_REQUEST,        sizeof(blt_attr_readMultiReq_t),        blt_atts_proc_readMultiReq          },
    {   ATT_OP_READ_BY_GROUP_TYPE_REQ,      ATT_REQUEST,        sizeof(blt_attr_readByGroupTypeReq_t),  blt_atts_proc_readByGroupTypeReq    },
    {   ATT_OP_READ_MULTIPLE_VARIABLE_REQ,  ATT_REQUEST,        sizeof(blt_attr_readMultiVarReq_t),     blt_atts_proc_readMultiVarReq       },
    {   ATT_OP_WRITE_REQ,                   ATT_REQUEST,        sizeof(blt_attr_writeReq_t),            blt_atts_proc_writeReq              },
    {   ATT_OP_PREPARE_WRITE_REQ,           ATT_REQUEST,        sizeof(blt_attr_prepareWriteReq_t),     blt_atts_proc_prepareWriteReq       },
    {   ATT_OP_EXECUTE_WRITE_REQ,           ATT_REQUEST,        sizeof(blt_attr_executeWriteReq_t),     blt_atts_proc_executeWriteReq       },
    {   ATT_OP_WRITE_CMD,                   ATT_COMMAND,        sizeof(blt_attr_writeCmd_t),            blt_atts_proc_writeCmd              },
    {   ATT_OP_SIGNED_WRITE_CMD,            ATT_COMMAND,        sizeof(blt_attr_signedWriteCmd_t),      blt_atts_proc_signedWriteCmd        },
    {   ATT_OP_HANDLE_VALUE_CFM,            ATT_CONFIRMATION,   sizeof(blt_attr_handleValueCfm_t),      blt_atts_proc_handleValueCfm        },
#endif



    /* ATT Client concerned */
#if (1)
    ATTC_RSP_RX_STR(ERROR, error),
    ATTC_RSP_RX_STR(EXCHANGE_MTU, exchangeMtu),
    ATTC_RSP_RX_STR(FIND_INFO, findInfo),
    ATTC_RSP_RX_STR(FIND_BY_TYPE_VALUE, findByTypeValue),
    ATTC_RSP_RX_STR(READ_BY_TYPE, readByType),
    ATTC_RSP_RX_STR(READ, read),
    ATTC_RSP_RX_STR(READ_BLOB, readBlob),
    ATTC_RSP_RX_STR(READ_MULTIPLE, readMulti),
    ATTC_RSP_RX_STR(READ_BY_GROUP_TYPE, readByGroupType),
    ATTC_RSP_RX_STR(READ_MULTIPLE_VARIABLE, readMultiVar),
    ATTC_RSP_RX_STR(WRITE, write),
    ATTC_RSP_RX_STR(PREPARE_WRITE, prepareWrite),
    ATTC_RSP_RX_STR(EXECUTE_WRITE, executeWrite),
    ATTC_IND_RX_STR(HANDLE_VALUE, handleValue),
    ATTC_NTF_RX_STR(HANDLE_VALUE, handleValue),
    ATTC_NTF_RX_STR(MULTIPLE_HANDLE_VALUE, multiHandleValue),
#else /* The code is easier to view */
    {   ATT_OP_ERROR_RSP,                   ATT_RESPONSE,       sizeof(blt_attr_errorRsp_t),            blt_attc_proc_errorRsp              },
    {   ATT_OP_EXCHANGE_MTU_RSP,            ATT_RESPONSE,       sizeof(blt_attr_exchangeMtuRsp_t),      blt_attc_proc_exchangeMtuRsp        },
    {   ATT_OP_FIND_INFO_RSP,               ATT_RESPONSE,       sizeof(blt_attr_findInfoRsp_t),         blt_attc_proc_findInfoRsp,          },
    {   ATT_OP_FIND_BY_TYPE_VALUE_RSP,      ATT_RESPONSE,       sizeof(blt_attr_findByTypeValueRsp_t),  blt_attc_proc_findByTypeValueRsp    },
    {   ATT_OP_READ_BY_TYPE_RSP,            ATT_RESPONSE,       sizeof(blt_attr_readByTypeRsp_t),       blt_attc_proc_readByTypeRsp         },
    {   ATT_OP_READ_RSP,                    ATT_RESPONSE,       sizeof(blt_attr_readRsp_t),             blt_attc_proc_readRsp               },
    {   ATT_OP_READ_BLOB_RSP,               ATT_RESPONSE,       sizeof(blt_attr_readBlobRsp_t),         blt_attc_proc_readBlobRsp           },
    {   ATT_OP_READ_MULTIPLE_RSP,           ATT_RESPONSE,       sizeof(blt_attr_readMultiRsp_t),        blt_attc_proc_readMultiRsp          },
    {   ATT_OP_READ_BY_GROUP_TYPE_RSP,      ATT_RESPONSE,       sizeof(blt_attr_readByGroupTypeRsp_t),  blt_attc_proc_readByGroupTypeRsp    },
    {   ATT_OP_WRITE_RSP,                   ATT_RESPONSE,       sizeof(blt_attr_writeRsp_t),            blt_attc_proc_writeRsp              },
    {   ATT_OP_PREPARE_WRITE_RSP,           ATT_RESPONSE,       sizeof(blt_attr_prepareWriteRsp_t),     blt_attc_proc_prepareWriteRsp       },
    {   ATT_OP_EXECUTE_WRITE_RSP,           ATT_RESPONSE,       sizeof(blt_attr_executeWriteRsp_t),     blt_attc_proc_executeWriteRsp       },
    {   ATT_OP_HANDLE_VALUE_NTF,            ATT_NOTIFICATION,   sizeof(blt_attr_handleValueNtf_t),      blt_attc_proc_handleValueNtf        },
    {   ATT_OP_HANDLE_VALUE_IND,            ATT_INDICATION,     sizeof(blt_attr_handleValueInd_t),      blt_attc_proc_handleValueInd        },
    {   ATT_OP_READ_MULTIPLE_VARIABLE_RSP,  ATT_RESPONSE,       sizeof(blt_attr_readMultiVarRsp_t),     blt_attc_proc_readMultiVarRsp       },
    {   ATT_OP_MULTIPLE_HANDLE_VALUE_NTF,   ATT_NOTIFICATION,   sizeof(blt_attr_multiHandleValueNtf_t), blt_attc_proc_multiHandleValueNtf   },
#endif
};


int blt_att_sendData_2_controller(u16 aclHandle, u16 scid, u8 *pHead, u8 headLen, u8 *pData, u16 dataLen)
{
    if(scid == L2CAP_CID_ATTR_PROTOCOL){ //For ATT channel
        return blt_l2cap_pushData_2_controller(aclHandle, L2CAP_CID_ATTR_PROTOCOL, pHead, headLen, pData, dataLen);
    }
    return 0;   //TODO: EATT client function.
}

u16 blt_att_procAttrRxPkt(attConCb_t *pAttrConCb, attr_pkt_t* attr, u16 attrLen)
{
    u8 op = attr->opcode;
    attr_pkt_t *pAttTx = pAttrConCb->attTxBuff;

    //TODO: slot_id :  L2CAP: CID == ATT and CID == EATT are all use the common process: att_rx_handlers
    for (u32 i = 0; i < ARRAY_SIZE(attr_rx_handlers); i++) {
        if (op == attr_rx_handlers[i].op) {
            if (attrLen < attr_rx_handlers[i].expect_len) {
                my_dump_str_data(0, "att rx length check err", 0, 0); //debug
                return blt_att_packageErrorRsp(op, ATT_HANDLE_NONE, ATT_ERR_INVALID_PDU, pAttTx);
            } else {
                my_dump_str_data(0, "att_rx_fn", &op, 1); //debug
                if((attr_rx_handlers[i].type & ATT_NED_ACK) && pAttTx == NULL)
                    return 0;
                /* ATT error response process in <<attr_ rx_ Holders>> internal */
                return attr_rx_handlers[i].att_rx_fn(pAttrConCb, attr, attrLen);    //attr_rx_handlers
            }
        }
    }

    if(op & ATT_PDU_MASK_COMMAND)
    {
        return 0;   //Command Flag, don't response.
    }

    my_dump_str_data(0, "att rx req not support", 0, 0); //debug
    return blt_att_packageErrorRsp(op, ATT_HANDLE_NONE, ATT_ERR_REQ_NOT_SUPPORTED, pAttTx);
}


////////////////////////////////////////////////////////////////////////////
//                  att_rx_process (client + server)
////////////////////////////////////////////////////////////////////////////
int blt_att_l2capAttRxHandler (u16 connHandle, l2cap_pkt_t *ptrAttr)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        return 0;
    }

    attConCb_t attConCb = {
        .connHandle = connHandle,
        .scid = L2CAP_CID_ATTR_PROTOCOL,
    };

    gap_ms_para_t *pGap_ms_para = blc_gap_getMasterSlavePara(connHandle);
    l2cap_pkt_t * l2cap_pkt = (l2cap_pkt_t *)blt_l2cap_get_tx_buff(connHandle);

    /*
     * core_v5.3 Vol 3, Part F, 3.3.2 Sequential protocol
     * Once a client sends a request to a server, that client shall send no other
     * request to the same server on the same ATT bearer until a response PDU has
     * been received
     */
    if (pGap_ms_para->pPendingPkt) {
        my_dump_str_data(0, "TX ATT pending to send", 0, 0); //debug
        attConCb.attTxBuff = NULL;
    } else {

        attConCb.attTxBuff = l2cap_pkt ? &l2cap_pkt->payload.att : NULL;
    }

    u16 pduLen = blt_att_procAttrRxPkt(&attConCb, (attr_pkt_t*)&ptrAttr->payload, ptrAttr->pduLen);

    if (pduLen){
        l2cap_pkt->pduLen = pduLen;
        l2cap_pkt->cid = L2CAP_CID_ATTR_PROTOCOL;
        my_dump_str_data(0, "TX ATT", (u8*)&l2cap_pkt->payload, l2cap_pkt->pduLen); //debug

        if(BLE_SUCCESS != blt_l2cap_pushData_2_controller (connHandle, l2cap_pkt->cid, &l2cap_pkt->payload.att.opcode, 1, (u8*)l2cap_pkt->payload.att.data, l2cap_pkt->pduLen-1))
        {
            pGap_ms_para->pPendingPkt = l2cap_pkt;
        }
    }

    return 0;
}

