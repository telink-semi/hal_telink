/********************************************************************************************************
 * @file    signaling_pkt.c
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

//////////////////signaling create new package to tx buffer//////////////////////
u16 blt_signal_packageCmdRejectRsp(signal_pkt_t* txBuf, u8 identifier, l2cap_sig_cmd_reject_reason reason, u32 reasonData)
{
    if(!txBuf)
        return 0;

    u8 *buffer = &txBuf->code;

    U8_TO_STREAM(buffer, L2CAP_COMMAND_REJECT_RSP);
    U8_TO_STREAM(buffer, identifier);

    switch(reason)
    {
        case SIG_CMD_NOT_UNDERSTAND:
            U16_TO_STREAM(buffer, 0x02);
            U16_TO_STREAM(buffer, reason);
            break;

        case SIG_MTU_EXCEEDED:      //Not used this reason.
            U16_TO_STREAM(buffer, 0x04);
            U16_TO_STREAM(buffer, reason);
            U16_TO_STREAM(buffer, reasonData);
            break;

        case SIG_INVALID_CID_REQUEST:
            U16_TO_STREAM(buffer, 0x06);
            U16_TO_STREAM(buffer, reason);
            U32_TO_STREAM(buffer, reasonData);
        break;

        default:
            return 0;
    }
    return buffer - &txBuf->code;
}

u16 blt_signal_packageCmdRejectRspUnderstood(signal_pkt_t* txBuf, u8 identifier)
{
    return blt_signal_packageCmdRejectRsp(txBuf, identifier, SIG_CMD_NOT_UNDERSTAND, 0);
}

u16 blt_signal_packageCmdRejectRspInvalidCid(signal_pkt_t* txBuf, u8 identifier, l2cap_cmdRejectCidData_t* invalidCID)
{
    return blt_signal_packageCmdRejectRsp(txBuf, identifier, SIG_INVALID_CID_REQUEST, *(u32*)invalidCID);
}

u16 blt_signal_packageConnParamUpdateReq(signal_pkt_t* txBuf, u8 identifier, signal_pkt_connParamUpdateReq_t* req)
{
    if(!txBuf)
        return 0;

    u8 *buffer = &txBuf->code; //skip dataLen field

    U8_TO_STREAM(buffer, L2CAP_CONN_PARAM_UPDATE_REQ);
    U8_TO_STREAM(buffer, identifier);
    U16_TO_STREAM(buffer, sizeof(signal_pkt_connParamUpdateReq_t));

    U16_TO_STREAM(buffer, req->intervalMin);
    U16_TO_STREAM(buffer, req->intervalMax);
    U16_TO_STREAM(buffer, req->latency);
    U16_TO_STREAM(buffer, req->timeout);

    return buffer - &txBuf->code;
}

u16 blt_signal_packageConnParamUpdateRsp(signal_pkt_t* txBuf, u8 identifier, signal_pkt_connParamUpdateRsp_t* rsp)
{
    if(!txBuf)
        return 0;

    u8 *buffer = &txBuf->code; //skip dataLen field

    U8_TO_STREAM(buffer, L2CAP_CONN_PARAM_UPDATE_RSP);
    U8_TO_STREAM(buffer, identifier);
    U16_TO_STREAM(buffer, sizeof(signal_pkt_connParamUpdateRsp_t));

    U16_TO_STREAM(buffer, rsp->result);

    return buffer - &txBuf->code;
}

#if L2CAP_CREDIT_BASED_FLOW_CONTROL_MODE_EN

u16 blt_signal_packageDisconnReq(signal_pkt_t* txBuf, u8 identifier, signal_pkt_disconnReq_t* req)
{
    if(!txBuf)
        return 0;

    u8 *buffer = &txBuf->code; //skip dataLen field

    U8_TO_STREAM(buffer, L2CAP_DISCONNECTION_REQ);
    U8_TO_STREAM(buffer, identifier);
    U16_TO_STREAM(buffer, sizeof(signal_pkt_disconnReq_t));

    U16_TO_STREAM(buffer, req->dstCID);
    U16_TO_STREAM(buffer, req->srcCID);

    return buffer - &txBuf->code;
}

u16 blt_signal_packageDisconnRsp(signal_pkt_t* txBuf, u8 identifier, signal_pkt_disconnRsp_t* rsp)
{
    if(!txBuf)
        return 0;

    u8 *buffer = &txBuf->code; //skip dataLen field

    U8_TO_STREAM(buffer, L2CAP_DISCONNECTION_RSP);
    U8_TO_STREAM(buffer, identifier);
    U16_TO_STREAM(buffer, sizeof(signal_pkt_disconnReq_t));

    U16_TO_STREAM(buffer, rsp->dstCID);
    U16_TO_STREAM(buffer, rsp->srcCID);

    return buffer - &txBuf->code;
}

u16 blt_signal_packageLeCreditBasedConnReq(signal_pkt_t* txBuf, u8 identifier, signal_pkt_leCreditBasedConnReq_t* req)
{
    if(!txBuf)
        return 0;

    u8 *buffer = &txBuf->code; //skip dataLen field

    U8_TO_STREAM(buffer, L2CAP_LE_CREDIT_BASED_CONNECTION_REQ);
    U8_TO_STREAM(buffer, identifier);
    U16_TO_STREAM(buffer, sizeof(signal_pkt_leCreditBasedConnReq_t));

    U16_TO_STREAM(buffer, req->SPSM);
    U16_TO_STREAM(buffer, req->srcCID);
    U16_TO_STREAM(buffer, req->MTU);
    U16_TO_STREAM(buffer, req->MPS);
    U16_TO_STREAM(buffer, req->initialCredits);

    return buffer - &txBuf->code;
}

u16 blt_signal_packageLeCreditBasedConnRsp(signal_pkt_t* txBuf, u8 identifier, signal_pkt_leCreditBasedConnRsp_t* rsp)
{
    if(!txBuf)
        return 0;

    u8 *buffer = &txBuf->code; //skip dataLen field

    U8_TO_STREAM(buffer, L2CAP_LE_CREDIT_BASED_CONNECTION_RSP);
    U8_TO_STREAM(buffer, identifier);
    U16_TO_STREAM(buffer, sizeof(signal_pkt_leCreditBasedConnRsp_t));

    U16_TO_STREAM(buffer, rsp->dstCID);
    U16_TO_STREAM(buffer, rsp->MTU);
    U16_TO_STREAM(buffer, rsp->MPS);
    U16_TO_STREAM(buffer, rsp->initialCredits);
    U16_TO_STREAM(buffer, rsp->result);

    return buffer - &txBuf->code;
}

u16 blt_signal_packageFlowCtrlCreditInd(signal_pkt_t* txBuf, u8 identifier, signal_pkt_flowCtrlCreditInd_t* ind)
{
    if(!txBuf)
        return 0;

    u8 *buffer = &txBuf->code; //skip dataLen field

    U8_TO_STREAM(buffer, L2CAP_FLOW_CONTROL_CREDIT_IND);
    U8_TO_STREAM(buffer, identifier);
    U16_TO_STREAM(buffer, sizeof(signal_pkt_flowCtrlCreditInd_t));

    U16_TO_STREAM(buffer, ind->CID);
    U16_TO_STREAM(buffer, ind->credits);

    return buffer - &txBuf->code;
}

u16 blt_signal_packageCreditBasedConnReq(signal_pkt_t* txBuf, u8 identifier, u8 srcCIDNum, signal_pkt_creditBasedConnReq_t* req)
{
    if(!txBuf)
        return 0;

    u8 *buffer = &txBuf->code; //skip dataLen field

    U8_TO_STREAM(buffer, L2CAP_CREDIT_BASED_CONNECTION_REQ);
    U8_TO_STREAM(buffer, identifier);
    U16_TO_STREAM(buffer, sizeof(signal_pkt_creditBasedConnReq_t) - (SIGNAL_CREADIT_CID_MAX_NUM-srcCIDNum)*sizeof(u16));

    U16_TO_STREAM(buffer, req->SPSM);
    U16_TO_STREAM(buffer, req->MTU);
    U16_TO_STREAM(buffer, req->MPS);
    U16_TO_STREAM(buffer, req->initialCredits);
    for(int i=0; i<srcCIDNum; i++)
    {
        U16_TO_STREAM(buffer, req->srcCID[i]);
    }

    return buffer - &txBuf->code;
}

u16 blt_signal_packageCreditBasedConnRsp(signal_pkt_t* txBuf, u8 identifier, u8 dstCIDNum, signal_pkt_creditBasedConnRsp_t* rsp)
{
    if(!txBuf)
        return 0;

    u8 *buffer = &txBuf->code; //skip dataLen field

    U8_TO_STREAM(buffer, L2CAP_CREDIT_BASED_CONNECTION_RSP);
    U8_TO_STREAM(buffer, identifier);
    U16_TO_STREAM(buffer, sizeof(signal_pkt_creditBasedConnRsp_t) - (SIGNAL_CREADIT_CID_MAX_NUM-dstCIDNum)*sizeof(u16));

    U16_TO_STREAM(buffer, rsp->MTU);
    U16_TO_STREAM(buffer, rsp->MPS);
    U16_TO_STREAM(buffer, rsp->initialCredits);
    U16_TO_STREAM(buffer, rsp->result);
    for(int i=0; i<dstCIDNum; i++)
    {
        U16_TO_STREAM(buffer, rsp->dstCID[i]);
    }

    return buffer - &txBuf->code;
}

u16 blt_signal_packageCreditBasedRecfgReq(signal_pkt_t* txBuf, u8 identifier, u8 dstCIDNum, signal_pkt_creditBasedCfgReq_t* req)
{
    if(!txBuf)
        return 0;

    u8 *buffer = &txBuf->code; //skip dataLen field

    U8_TO_STREAM(buffer, L2CAP_CREDIT_BASED_RECONFIGURE_REQ);
    U8_TO_STREAM(buffer, identifier);
    U16_TO_STREAM(buffer, sizeof(signal_pkt_creditBasedCfgReq_t) - (SIGNAL_CREADIT_CID_MAX_NUM-dstCIDNum)*sizeof(u16));

    U16_TO_STREAM(buffer, req->MTU);
    U16_TO_STREAM(buffer, req->MPS);
    for(int i=0; i<dstCIDNum; i++)
    {
        U16_TO_STREAM(buffer, req->dstCID[i]);
    }

    return buffer - &txBuf->code;
}

u16 blt_signal_packageCreditBasedRecfgRsp(signal_pkt_t* txBuf, u8 identifier, u16 result)
{
    if(!txBuf)
        return 0;

    u8 *buffer = &txBuf->code; //skip dataLen field

    U8_TO_STREAM(buffer, L2CAP_CREDIT_BASED_RECONFIGURE_RSP);
    U8_TO_STREAM(buffer, identifier);
    U16_TO_STREAM(buffer, sizeof(signal_pkt_creditBasedCfgRsp_t));

    U16_TO_STREAM(buffer, result);

    return buffer - &txBuf->code;
}

#endif

///////////////////all LE signaling package/////////////////////

//////////////////push LE signaling package to link layer/////////////
#define SIGNAL_SEND_PREPARE_PKT(connHandle)         u8 signalTxBuff[23];    \
                                                    signal_pkt_t * signal_pkt = (signal_pkt_t *)signalTxBuff;   \
                                                    u8 identifier = blt_signal_getIdentifier()
#define SIGNAL_SEND_PUSH_PKT(connHandle)            return blt_l2cap_pushData_2_controller (connHandle, L2CAP_CID_SIG_CHANNEL, \
                                                    &signal_pkt->code, sizeof(signal_pkt_t), signal_pkt->data, signal_pkt->dataLen)


ble_sts_t blc_signal_sendConnectParameterUpdateReq(u16 connHandle, u16 min_interval, u16 max_interval, u16 latency, u16 timeout)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS || (connHandle & BLM_CONN_HANDLE)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    signal_pkt_connParamUpdateReq_t req = {
        .intervalMin = min_interval,
        .intervalMax = max_interval,
        .latency = latency,
        .timeout = timeout
    };

    SIGNAL_SEND_PREPARE_PKT(connHandle);

    blt_signal_packageConnParamUpdateReq(signal_pkt, identifier, &req);
    SIGNAL_SEND_PUSH_PKT(connHandle);
}

ble_sts_t blc_signal_sendConnectParameterUpdateRsp(u16 connHandle, u8 identifier, conn_para_up_rsp result)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS || (connHandle & BLS_CONN_HANDLE)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    signal_pkt_connParamUpdateRsp_t rsp = {
        .result = result
    };

    u8 signalTxBuff[23];
    signal_pkt_t * signal_pkt = (signal_pkt_t *)signalTxBuff;

    blt_signal_packageConnParamUpdateRsp(signal_pkt, identifier, &rsp);
    SIGNAL_SEND_PUSH_PKT(connHandle);
}

#if L2CAP_CREDIT_BASED_FLOW_CONTROL_MODE_EN
ble_sts_t blt_signal_sendDisconnReqByCid(u16 connHandle, u16 dstCID, u16 srcCID)
{
    SIGNAL_SEND_PREPARE_PKT(connHandle);

    signal_pkt_disconnReq_t req = {
        .dstCID = dstCID,
        .srcCID = srcCID,
    };
    blt_signal_packageDisconnReq(signal_pkt, identifier, &req);
    SIGNAL_SEND_PUSH_PKT(connHandle);
}

ble_sts_t blt_signal_sendDisconnReq(l2cap_coc_cid_t* pCid)
{
    if(pCid->SPSM == 0)     return BLE_SUCCESS;
    ble_sts_t state = blt_signal_sendDisconnReqByCid(pCid->connHandle, pCid->srcCID, pCid->dstCID);
    if(state == BLE_SUCCESS)
        pCid->SPSM = 0;
    return state;
}

ble_sts_t blt_signal_sendLeCreditBasedConnReq(u16 connHandle, u16 SPSM, u16 srcCID)
{
    SIGNAL_SEND_PREPARE_PKT(connHandle);
    signal_pkt_leCreditBasedConnReq_t req = {
        .SPSM = SPSM,
        .srcCID = srcCID,
        .MTU = blt_l2cap_cocGetRecvMtu(),
        .MPS = blt_l2cap_cocGetRecvMps(),
        .initialCredits = SIGNAL_DEFAULT_INITIAL_CREDITS,
    };
    blt_signal_packageLeCreditBasedConnReq(signal_pkt, identifier, &req);
    SIGNAL_SEND_PUSH_PKT(connHandle);
}

ble_sts_t blt_signal_sendFlowCtrlCreditInd(u16 connHandle, signal_pkt_flowCtrlCreditInd_t* ind)
{
    SIGNAL_SEND_PREPARE_PKT(connHandle);
    blt_signal_packageFlowCtrlCreditInd(signal_pkt, identifier, ind);
    SIGNAL_SEND_PUSH_PKT(connHandle);
}

ble_sts_t blt_signal_sendCreditBasedConnReq(u16 connHandle, u16 SPSM, u16* srcCID, u8 srcCIDCnt)
{
    SIGNAL_SEND_PREPARE_PKT(connHandle);
    signal_pkt_creditBasedConnReq_t req = {
        .SPSM = SPSM,
        .MTU = blt_l2cap_cocGetRecvMtu(),
        .MPS = blt_l2cap_cocGetRecvMps(),
        .initialCredits = SIGNAL_DEFAULT_INITIAL_CREDITS,
    };

    for(int i=0; i<srcCIDCnt; i++)
    {
        req.srcCID[i] = srcCID[i];
    }

    blt_signal_packageCreditBasedConnReq(signal_pkt, identifier, srcCIDCnt, &req);
    SIGNAL_SEND_PUSH_PKT(connHandle);
}

ble_sts_t blt_signal_sendCreditReconfigureReq(u16 connHandle, u16 MTU, u16 MPS, u16* dstCID, u8 dstCIDCnt)
{
    SIGNAL_SEND_PREPARE_PKT(connHandle);
    signal_pkt_creditBasedCfgReq_t req = {
        .MTU = MTU,
        .MPS = MPS,
    };

    for(int i=0; i<dstCIDCnt; i++)
    {
        req.dstCID[i] = dstCID[i];
    }

    blt_signal_packageCreditBasedRecfgReq(signal_pkt, identifier, dstCIDCnt, &req);
    SIGNAL_SEND_PUSH_PKT(connHandle);
}

#endif
