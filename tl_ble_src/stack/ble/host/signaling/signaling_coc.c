/********************************************************************************************************
 * @file    signaling_coc.c
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

#if L2CAP_CREDIT_BASED_FLOW_CONTROL_MODE_EN

//////////////////CoC parameter//////////////
_attribute_ble_data_retention_
static l2cap_coc_control_t sL2capCocCtrl = {0};

u16 blt_l2cap_cocGetRecvSpsm(void)
{
    return sL2capCocCtrl.SPSM;
}

u16 blt_l2cap_cocGetRecvMtu(void)
{
    return sL2capCocCtrl.MTU;
}

u16 blt_l2cap_cocGetRecvMps(void)
{
    return sL2capCocCtrl.MPS;
}

//////////////////CoC parameter//////////////

//this function to check source CID, allocated.
bool blt_l2cap_cocCheckCidAllocated(u16 connHandle, u16 srcCID)
{
    l2cap_coc_control_t *pCtrl = &sL2capCocCtrl;

    for(int i=0; i<pCtrl->cocCidCnt; i++)
    {
        if(pCtrl->pCocCid[i].connHandle == connHandle && pCtrl->pCocCid[i].srcCID == srcCID)
        {
            return true;
        }
    }

#if L2CAP_SERVER_FEATURE_SUPPORTED_EATT
    for(int i=0; i<pCtrl->eattCidCnt; i++)
    {
        if(pCtrl->pEattCid[i].connHandle == connHandle && pCtrl->pEattCid[i].srcCID == srcCID)
        {
            return true;
        }
    }
#endif

    return false;
}

bool blt_l2cap_cocCheckNotAvailableResources(u16 SPSM)
{
    l2cap_coc_control_t *pCtrl = &sL2capCocCtrl;
#if L2CAP_SERVER_FEATURE_SUPPORTED_EATT
    u16 cidCnt = SPSM == L2CAP_COC_SPSM_EATT? pCtrl->eattCidCnt: pCtrl->cocCidCnt;
    l2cap_coc_cid_t* pCid = SPSM == L2CAP_COC_SPSM_EATT? pCtrl->pEattCid: pCtrl->pCocCid;
#else
    (void)SPSM;
    u16 cidCnt = pCtrl->cocCidCnt;
    l2cap_coc_cid_t* pCid = pCtrl->pCocCid;
#endif

    for(int i=0; i<cidCnt; i++)
    {
        if(!pCid[i].connHandle)
        {
            return false;
        }
    }

    return true;
}

//return value:BIT31-BIT16: EATT CID empty count, BIT15-BIT0: CoC CID empty count.
static int blt_l2cap_cocGetDstUsedMap(u16 connHandle, u8 cidMap[8])
{
    l2cap_coc_control_t *pCtrl = &sL2capCocCtrl;
    memset(cidMap, 0, 8);

    int emptyCnt = 0;

    //check cid count empty
    for(int i=0; i<pCtrl->cocCidCnt; i++)
    {
        l2cap_coc_cid_t *pCid = &pCtrl->pCocCid[i];

        if(pCid->connHandle == connHandle)
        {
            u16 srcCID = pCid->dstCID - L2CAP_COC_CID_START;
            cidMap[(srcCID>>3)] |= BIT(srcCID&0x07);
        }

        if(!pCid->connHandle)
        {
            emptyCnt += BIT(0);
        }
    }

#if L2CAP_SERVER_FEATURE_SUPPORTED_EATT
    //check cid count empty
    for(int i=0; i<pCtrl->eattCidCnt; i++)
    {
        l2cap_coc_cid_t *pCid = &pCtrl->pEattCid[i];

        if(pCid->connHandle == connHandle)
        {
            u16 srcCID = pCid->dstCID - L2CAP_COC_CID_START;
            cidMap[(srcCID>>3)] |= BIT(srcCID&0x07);
        }

        if(!pCid->connHandle)
        {
            emptyCnt += BIT(16);
        }
    }
#endif

    return emptyCnt;
}

static u16 blt_l2cap_getCocDstUsedMap(u16 connHandle, u8 cidMap[8])
{
    return blt_l2cap_cocGetDstUsedMap(connHandle, cidMap) & 0x0000FFFF;
}

static u16 blt_l2cap_cocGetEmptyDstCidNum(u16 connHandle, u16 srcCID)
{
    u8 cidMap[8];
    blt_l2cap_cocGetDstUsedMap(connHandle, cidMap);
    u16 dstCID = srcCID - L2CAP_COC_CID_START;
    if(cidMap[dstCID>>3] & BIT(dstCID&0x07))
    {
        for(int i=0; i<L2CAP_CHANNEL_MAX_COUNT; i++)
        {
            if(cidMap[(i>>3)] & BIT(i&0x07))
            {
                continue;
            }
            dstCID = i;
            break;
        }
    }
    return dstCID+L2CAP_COC_CID_START;
}

l2cap_coc_cid_t* blt_l2cap_cocCreateNewCID(u16 connHandle, u16 SPSM, u16 srcCID)
{
    l2cap_coc_control_t *pCtrl = &sL2capCocCtrl;

#if L2CAP_SERVER_FEATURE_SUPPORTED_EATT
    u16 cidCnt = SPSM == L2CAP_COC_SPSM_EATT? pCtrl->eattCidCnt: pCtrl->cocCidCnt;
    l2cap_coc_cid_t* pCid = SPSM == L2CAP_COC_SPSM_EATT? pCtrl->pEattCid: pCtrl->pCocCid;
#else
    u16 cidCnt = pCtrl->cocCidCnt;
    l2cap_coc_cid_t* pCid = pCtrl->pCocCid;
#endif
    for(int i=0; i<cidCnt; i++)
    {
        if(!pCid[i].connHandle)
        {
            memset(&pCid[i], 0, OFFSETOF(l2cap_coc_cid_t, pRxSdu));
            pCid[i].connHandle = connHandle;
            pCid[i].srcCID = srcCID;
            pCid[i].dstCID = blt_l2cap_cocGetEmptyDstCidNum(connHandle, srcCID);
            pCid[i].SPSM = SPSM;
            return &pCid[i];
        }
    }

    return NULL;
}

l2cap_coc_cid_t* blt_l2cap_cocGetDstCID(u16 connHandle, u16 dstCID)
{
    l2cap_coc_control_t *pCtrl = &sL2capCocCtrl;

    for(int i=0; i<pCtrl->cocCidCnt; i++)
    {
        if(pCtrl->pCocCid[i].connHandle == connHandle && pCtrl->pCocCid[i].dstCID == dstCID)
        {
            return &pCtrl->pCocCid[i];
        }
    }

#if L2CAP_SERVER_FEATURE_SUPPORTED_EATT
    for(int i=0; i<pCtrl->eattCidCnt; i++)
    {
        if(pCtrl->pEattCid[i].connHandle == connHandle && pCtrl->pEattCid[i].dstCID == dstCID)
        {
            return &pCtrl->pEattCid[i];
        }
    }
#endif

    return NULL;
}

l2cap_coc_cid_t* blt_l2cap_cocGetSrcCID(u16 connHandle, u16 srcCID)
{
    l2cap_coc_control_t *pCtrl = &sL2capCocCtrl;

    for(int i=0; i<pCtrl->cocCidCnt; i++)
    {
        if(pCtrl->pCocCid[i].connHandle == connHandle && pCtrl->pCocCid[i].srcCID == srcCID)
        {
            return &pCtrl->pCocCid[i];
        }
    }

#if L2CAP_SERVER_FEATURE_SUPPORTED_EATT
    for(int i=0; i<pCtrl->eattCidCnt; i++)
    {
        if(pCtrl->pEattCid[i].connHandle == connHandle && pCtrl->pEattCid[i].srcCID == srcCID)
        {
            return &pCtrl->pEattCid[i];
        }
    }
#endif

    return NULL;
}

void blt_l2cap_cocDisconnect(u16 connHandle)
{
    l2cap_coc_control_t *pCtrl = &sL2capCocCtrl;

    for(int i=0; i<pCtrl->cocCidCnt; i++)
    {
        if(pCtrl->pCocCid[i].connHandle == connHandle)
        {
            blt_l2cap_reportCocDisconnectEvent(&pCtrl->pCocCid[i]);
        }
    }

#if L2CAP_SERVER_FEATURE_SUPPORTED_EATT
    for(int i=0; i<pCtrl->eattCidCnt; i++)
    {
        if(pCtrl->pEattCid[i].connHandle == connHandle)
        {
            blt_l2cap_reportCocDisconnectEvent(&pCtrl->pEattCid[i]);
        }
    }
#endif

    for(int i=0; i<pCtrl->createConnCnt; i++)
    {
        if(pCtrl->pCreateConn[i].connHandle == connHandle)
        {
            blt_l2cap_reportCocCreateConnectFinishEvent(connHandle, 0xFF, 0x00);
        }
    }
}

void blt_l2cap_cocDataControl(u16 connHandle, l2cap_pkt_t* pkt)
{
    l2cap_coc_cid_t* pCid = blt_l2cap_cocGetDstCID(connHandle, pkt->cid);

    if(pCid == NULL)
    {
        return ;
    }

    if(pCid->recvLen)
    {
        if(pCid->recvLen + pkt->pduLen > pCid->sduLen || pkt->pduLen > blt_l2cap_cocGetRecvMps())
        {
            blt_signal_sendDisconnReq(pCid);
            return ;
        }

        memcpy(pCid->pRxSdu + pCid->recvLen, pkt->payload.coc.cont.info, pkt->pduLen);
        pCid->recvLen += pkt->pduLen;       //PDU Length
    }
    else
    {
        if(pkt->payload.coc.start.sduLen > blt_l2cap_cocGetRecvMtu() || pkt->pduLen-2 > blt_l2cap_cocGetRecvMps())
        {
            blt_signal_sendDisconnReq(pCid);
            return ;
        }
        pCid->sduLen = pkt->payload.coc.start.sduLen;   //SDU Length
        pCid->recvLen = pkt->pduLen - 2;                //PDU Length

        memcpy(pCid->pRxSdu, pkt->payload.coc.start.info, pCid->recvLen);
    }

    if(pCid->recvCredits == 0)
    {
        blt_signal_sendDisconnReq(pCid);
        return ;
    }

    pCid->recvCredits -- ;

    if(pCid->recvCredits <= (SIGNAL_DEFAULT_INITIAL_CREDITS>>1))
    {
        signal_pkt_flowCtrlCreditInd_t ind = {
            .CID = pkt->cid,
            .credits = SIGNAL_DEFAULT_INITIAL_CREDITS - pCid->recvCredits,
        };
        if(BLE_SUCCESS == blt_signal_sendFlowCtrlCreditInd(connHandle, &ind))
        {
            pCid->recvCredits += ind.credits;
        }
    }

    if(pCid->recvLen == pCid->sduLen)
    {
#if L2CAP_SERVER_FEATURE_SUPPORTED_EATT
        if(pCid->SPSM == L2CAP_COC_SPSM_EATT)
        {
            blt_eatt_l2capEattRxHandle(pCid);
        }
        else
#endif
        {
            blt_l2cap_reportCocReceiveDataEvent(pCid);
        }

        pCid->recvLen = 0;
        pCid->sduLen = 0;
    }
}

static bool blt_l2cap_cocCheckTxFifo(u16 connHandle, u16 dataLen)
{
    u16 unitMaxLen = blt_llms_get_connEffectiveMaxTxOctets_by_connIdx(connHandle & CONN_IDX_MASK);

    int pktNum = (dataLen+4)/unitMaxLen + (((dataLen+4)%unitMaxLen)? 1: 0);

    u8 max_fifo_num = blt_llms_get_tx_fifo_max_num(connHandle);
    return (blc_ll_getTxFifoNumber(connHandle) + pktNum) > (max_fifo_num-BLMS_STACK_USED_TX_FIFO_NUM);
}

//sduLen = 0, mean coc continue packet
static void blt_l2cap_sendCocPkt(u16 connHandle, u16 srcCID, u16 sduLen, u8* data, u16 dataLen)
{
    u16 unitMaxLen = blt_llms_get_connEffectiveMaxTxOctets_by_connIdx(connHandle & CONN_IDX_MASK);

    /////////////// step 2, push data to TX fifo ////////////
    u8 l2capBuff[257]; //DLE max 251, rf_max=255, header=2, 257 byte is enough.
    u8* pPtr = l2capBuff;

    u16 l2capLen, sendLen;

    u16 dataIdx = 0;
    u16 sendDataLen;

    if(dataLen + (sduLen?0:2) > unitMaxLen -6)
    {
        sendDataLen = unitMaxLen-4 - (sduLen?2:0);
    }
    else
    {
        sendDataLen = dataLen;
    }

    l2capLen = dataLen + (sduLen?2:0);
    sendLen = sendDataLen + (sduLen?2:0) + 4;

    U8_TO_STREAM(pPtr, LLID_DATA_START);    //first data packet
    U8_TO_STREAM(pPtr, sendLen);        //rf_len
    U16_TO_STREAM(pPtr, l2capLen);
    U16_TO_STREAM(pPtr, srcCID);
    if(sduLen)  U16_TO_STREAM(pPtr, sduLen);

    STR_TO_STREAM(pPtr, data, sendDataLen);
    ll_push_tx_fifo_handler (connHandle, l2capBuff);

    dataIdx = sendDataLen;

    while(dataIdx < dataLen)
    {
        pPtr = l2capBuff;
        if(dataLen-dataIdx > unitMaxLen)
        {
            sendDataLen = unitMaxLen;
        }
        else
        {
            sendDataLen = dataLen-dataIdx;
        }

        U8_TO_STREAM(pPtr, LLID_DATA_CONTINUE); //first data packet
        U8_TO_STREAM(pPtr, sendDataLen);        //rf_len
        STR_TO_STREAM(pPtr, data + dataIdx, sendDataLen);
        ll_push_tx_fifo_handler (connHandle, l2capBuff);
        dataIdx += sendDataLen;
    }
}

static u16 blt_l2cap_cocGetMpsSize(u16 connHandle, u16 MPS, u16 headLen)
{
    u16 unitMaxLen = blt_llms_get_connEffectiveMaxTxOctets_by_connIdx(connHandle & CONN_IDX_MASK);

    u8 max_fifo_num = blt_llms_get_tx_fifo_max_num(connHandle);

    u16 ll_max_mps = unitMaxLen*(max_fifo_num-1-BLMS_STACK_USED_TX_FIFO_NUM) -4-headLen;    //1 empty packet

    return min(ll_max_mps, MPS);
}

static int blt_l2cap_cocSendData(l2cap_coc_cid_t* pCid)
{
    if(!pCid->connHandle || !pCid->sendTotalLen)    return 0;

    if(!pCid->sendCredits)  return 1;

    u16 sendLen;

    if(pCid->sendOffsetLen)
    {
        u16 mps = blt_l2cap_cocGetMpsSize(pCid->connHandle, pCid->mps, 0);
        sendLen = min(mps, pCid->sendTotalLen - pCid->sendOffsetLen);
    }
    else
    {
        u16 mps = blt_l2cap_cocGetMpsSize(pCid->connHandle, pCid->mps, 2);
        sendLen = min(mps, pCid->sendTotalLen);
    }


    if(blt_l2cap_cocCheckTxFifo(pCid->connHandle, sendLen + (pCid->sendOffsetLen?0:2)))
    {
        return 1;
    }

    blt_l2cap_sendCocPkt(pCid->connHandle, pCid->srcCID, pCid->sendOffsetLen?0:pCid->sendTotalLen, pCid->pTxSdu+pCid->sendOffsetLen, sendLen);
    pCid->sendCredits--;
    pCid->sendOffsetLen += sendLen;

    if(pCid->sendOffsetLen == pCid->sendTotalLen)
    {
        pCid->sendTotalLen = 0;
        pCid->sendOffsetLen = 0;
        blt_l2cap_reportCocSendDataFinishEvent(pCid);
        return 0;
    }

    return 1;
}

static void blt_l2cap_cocClearAcl(u16 connHandle)
{
    l2cap_coc_control_t *pCtrl = &sL2capCocCtrl;

    for(int i=0; i<pCtrl->createConnCnt; i++)
    {
        if(pCtrl->pCreateConn[i].connHandle == connHandle)
        {
            memset(&pCtrl->pCreateConn[i], 0, sizeof(l2cap_coc_acl_t));
        }
    }
}

l2cap_coc_acl_t* blt_l2cap_getCreateCoc(u16 connHandle)
{
    l2cap_coc_control_t *pCtrl = &sL2capCocCtrl;

    for(int i=0; i<pCtrl->createConnCnt; i++)
    {
        if(pCtrl->pCreateConn[i].connHandle == connHandle)
        {
            return &pCtrl->pCreateConn[i];
        }
    }

    return NULL;
}

////////////////l2cap CoC event////////////
void blt_l2cap_reportCocConnectEvent(l2cap_coc_cid_t* pCid)
{
    if(gap_eventMask & GAP_EVT_MASK_L2CAP_COC_CONNECT){
        gap_l2cap_cocConnectEvt_t evt = {
            .connHandle = pCid->connHandle,
            .spsm = pCid->SPSM,
            .mtu = pCid->mtu,
            .srcCid = pCid->srcCID,
            .dstCid = pCid->dstCID,
        };
        blc_gap_send_event(GAP_EVT_L2CAP_COC_CONNECT, (u8*)&evt, sizeof(gap_l2cap_cocConnectEvt_t));
    }
}

void blt_l2cap_reportCocDisconnectEvent(l2cap_coc_cid_t* pCid)
{
    if(gap_eventMask & GAP_EVT_MASK_L2CAP_COC_DISCONNECT){
        gap_l2cap_cocDisconnectEvt_t evt = {
            .connHandle = pCid->connHandle,
            .srcCid = pCid->srcCID,
            .dstCid = pCid->dstCID,
        };
        blc_gap_send_event(GAP_EVT_L2CAP_COC_DISCONNECT, (u8*)&evt, sizeof(gap_l2cap_cocDisconnectEvt_t));
    }
    memset(pCid, 0, OFFSETOF(l2cap_coc_cid_t, pRxSdu));
}

void blt_l2cap_reportCocReconnfigureEvent(l2cap_coc_cid_t* pCid)
{
    if(gap_eventMask & GAP_EVT_MASK_L2CAP_COC_RECONFIGURE){
        gap_l2cap_cocReconfigureEvt_t evt = {
            .connHandle = pCid->connHandle,
            .srcCid = pCid->srcCID,
            .mtu = pCid->mtu,
        };
        blc_gap_send_event(GAP_EVT_L2CAP_COC_RECONFIGURE, (u8*)&evt, sizeof(gap_l2cap_cocReconfigureEvt_t));
    }
}

void blt_l2cap_reportCocReceiveDataEvent(l2cap_coc_cid_t* pCid)
{
    if(gap_eventMask & GAP_EVT_MASK_L2CAP_COC_RECV_DATA){
        gap_l2cap_cocRecvDataEvt_t evt = {
            .connHandle = pCid->connHandle,
            .dstCid = pCid->dstCID,
            .length = pCid->sduLen,
            .data = pCid->pRxSdu,
        };
        blc_gap_send_event(GAP_EVT_L2CAP_COC_RECV_DATA, (u8*)&evt, sizeof(gap_l2cap_cocRecvDataEvt_t));
    }
}

void blt_l2cap_reportCocSendDataFinishEvent(l2cap_coc_cid_t* pCid)
{
    if(gap_eventMask & GAP_EVT_MASK_L2CAP_COC_SEND_DATA_FINISH){
        gap_l2cap_cocSendDataFinishEvt_t evt = {
            .connHandle = pCid->connHandle,
            .srcCid = pCid->srcCID,
        };
        blc_gap_send_event(GAP_EVT_L2CAP_COC_SEND_DATA_FINISH, (u8*)&evt, sizeof(gap_l2cap_cocSendDataFinishEvt_t));
    }
}

void blt_l2cap_reportCocCreateConnectFinishEvent(u16 connHandle, u8 code, u16 result)
{
    blt_l2cap_cocClearAcl(connHandle);
    if(gap_eventMask & GAP_EVT_MASK_L2CAP_COC_CREATE_CONNECT_FINISH){
        gap_l2cap_cocCreateConnectFinishEvt_t evt = {
            .connHandle = connHandle,
            .code = code,
            .result = result,
        };
        blc_gap_send_event(GAP_EVT_L2CAP_COC_CREATE_CONNECT_FINISH, (u8*)&evt, sizeof(gap_l2cap_cocCreateConnectFinishEvt_t));
    }
}

////////////////l2cap CoC event end////////////

///////////////L2cap CoC public APIs//////////
int blc_l2cap_registerCocModule(blc_coc_initParam_t* param, u8 *pBuffer, u16 buffLen)
{
#if L2CAP_SERVER_FEATURE_SUPPORTED_EATT
    if(buffLen < COC_MODULE_BUFFER_SIZE(param->createConnCnt, param->cocCidCnt, param->eattCidCnt, param->MTU))
#else
    if(buffLen < COC_MODULE_BUFFER_SIZE(param->createConnCnt, param->cocCidCnt, 0, param->MTU))
#endif
    {
        return -1;
    }

    if(!(SIGNAL_CHECK_DYNAMIC_SPSM(param->SPSM) || SIGNAL_CHECK_SIG_ASSIGNED_SPSM(param->SPSM)))
    {
        return -2;
    }

    if(param->MTU < SIGNAL_MINIMUM_MTU)
    {
        return -4;
    }

    //@brief    8 = header(2)+l2cap_len(2)+CID(2)+SDU_Length(2)
    u16 MPS = blt_l2cap_getAclRxBufferSize() - 8;

    if(MPS < SIGNAL_MINIMUM_MPS || MPS > SIGNAL_MAXIMUM_MPS)
    {
        return -5;
    }

#if L2CAP_SERVER_FEATURE_SUPPORTED_EATT
    if((param->cocCidCnt + param->eattCidCnt) == 0)
#else
    if(param->cocCidCnt == 0)
#endif
    {
        return -6;
    }

    l2cap_coc_control_t *pCtrl = &sL2capCocCtrl;
    memset(pCtrl, 0, sizeof(l2cap_coc_control_t));
    memset(pBuffer, 0, buffLen);

    pCtrl->MTU = param->MTU;
    pCtrl->MPS = min(MPS, pCtrl->MTU);
    pCtrl->SPSM = param->SPSM;
    pCtrl->createConnCnt = param->createConnCnt;
    pCtrl->cocCidCnt = param->cocCidCnt;
#if L2CAP_SERVER_FEATURE_SUPPORTED_EATT
    pCtrl->eattCidCnt = param->eattCidCnt;
#endif

    pCtrl->pCreateConn = (l2cap_coc_acl_t*)pBuffer;
    pBuffer += param->createConnCnt*sizeof(l2cap_coc_acl_t);

    pCtrl->pCocCid = (l2cap_coc_cid_t*)pBuffer;
    pBuffer += param->cocCidCnt*sizeof(l2cap_coc_cid_t);
    for(int i=0; i<pCtrl->cocCidCnt; i++)
    {
        pCtrl->pCocCid[i].pRxSdu = pBuffer;
        pBuffer += param->MTU;
    }

#if L2CAP_SERVER_FEATURE_SUPPORTED_EATT
    pCtrl->pEattCid = (l2cap_coc_cid_t*)pBuffer;
    pBuffer += param->eattCidCnt*sizeof(l2cap_coc_cid_t);
    for(int i=0; i<pCtrl->eattCidCnt; i++)
    {
        pCtrl->pEattCid[i].pRxSdu = pBuffer;
        pBuffer += param->MTU;
        pCtrl->pEattCid[i].pTxSdu = pBuffer;
        pBuffer += param->MTU;
    }
#endif

    coc_data_handler = blt_l2cap_cocDataControl;
    signal_rx_handlers = (signal_rx_proc_t*)(size_t)signal_CoC_handlers;
    coc_disconnect_handler = blt_l2cap_cocDisconnect;
    coc_main_loop_cb = blc_l2cap_cocMainLoop;
    return 0;
}

ble_sts_t blc_l2cap_disconnectCocChannel(u16 connHandle, u16 srcCID)
{
    if(BLE_SUCCESS != blt_ll_isAclhdlInvalid(connHandle))
    {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    l2cap_coc_cid_t* pCid = blt_l2cap_cocGetSrcCID(connHandle, srcCID);

    if(pCid == NULL)    return L2CAP_ERR_INVALID_PARAMETER;

    return blt_signal_sendDisconnReq(pCid);

}

ble_sts_t blc_l2cap_createLeCreditBasedConnect(u16 connHandle)
{
    if(BLE_SUCCESS != blt_ll_isAclhdlInvalid(connHandle))
    {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    l2cap_coc_control_t *pCtrl = &sL2capCocCtrl;

    if(pCtrl->cocCidCnt == 0)
    {
        return L2CAP_ERR_INVALID_PARAMETER;
    }

    u8 cidMap[8];

    if(blt_l2cap_getCocDstUsedMap(connHandle, cidMap) == 0)
    {
        return L2CAP_ERR_NO_CID_AVAILABLE;
    }
    l2cap_coc_acl_t* pAcl = NULL;

    for(int i=0; i<pCtrl->createConnCnt; i++)
    {
        if(pCtrl->pCreateConn[i].connHandle == connHandle)
        {
            return L2CAP_ERR_COC_CREATING;
        }
        if(!pCtrl->pCreateConn[i].connHandle)
        {
            pAcl = &pCtrl->pCreateConn[i];
            break;
        }
    }

    if(!pAcl)       return L2CAP_ERR_NO_CREATE_COC_HANDLER;

    u16 srcCID = 0;
    for(int i=0; i<L2CAP_CHANNEL_MAX_COUNT; i++)
    {
        if(cidMap[(i>>3)] & BIT(i&0x07))
        {
            continue;
        }
        srcCID = L2CAP_COC_CID_START + i;
        break;
    }

    if(srcCID == 0)     return L2CAP_ERR_ALL_CID_ALLOCATED;

    ble_sts_t state = blt_signal_sendLeCreditBasedConnReq(connHandle, pCtrl->SPSM, srcCID);

    if(state == BLE_SUCCESS)
    {
        pAcl->connHandle = connHandle;
        pAcl->SPSM = pCtrl->SPSM;
        pAcl->dstCIDNum = 1;
        pAcl->dstCID[0] = srcCID;
    }
    return state;
}

ble_sts_t blc_l2cap_createCreditBasedConnect(u16 connHandle, u8 srcCnt)
{
    if(BLE_SUCCESS != blt_ll_isAclhdlInvalid(connHandle))
    {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    l2cap_coc_control_t *pCtrl = &sL2capCocCtrl;

    if(pCtrl->cocCidCnt == 0)
    {
        return L2CAP_ERR_INVALID_PARAMETER;
    }

    u8 cidMap[8];
    int emptyCidCnt = blt_l2cap_getCocDstUsedMap(connHandle, cidMap);

    if(!emptyCidCnt)        return L2CAP_ERR_NO_CID_AVAILABLE;

    srcCnt = min3(SIGNAL_CREADIT_CID_MAX_NUM, srcCnt, emptyCidCnt);

    l2cap_coc_acl_t* pAcl = NULL;

    for(int i=0; i<pCtrl->createConnCnt; i++)
    {
        if(pCtrl->pCreateConn[i].connHandle == connHandle)
        {
            return L2CAP_ERR_COC_CREATING;
        }
        if(!pCtrl->pCreateConn[i].connHandle)
        {
            pAcl = &pCtrl->pCreateConn[i];
            break;
        }
    }

    if(!pAcl)       return L2CAP_ERR_NO_CREATE_COC_HANDLER;

    u16 srcCID[5];
    int j=0;

    for(int i=0; i<L2CAP_CHANNEL_MAX_COUNT; i++)
    {
        if(cidMap[(i>>3)] & BIT(i&0x07))
        {
            continue;
        }
        srcCID[j++] = L2CAP_COC_CID_START + i;
        if(j>=srcCnt) break;
    }

    ble_sts_t state = blt_signal_sendCreditBasedConnReq(connHandle, pCtrl->SPSM, srcCID, srcCnt);

    if(state == BLE_SUCCESS)
    {
        pAcl->connHandle = connHandle;
        pAcl->SPSM = pCtrl->SPSM;
        memcpy(pAcl->dstCID, srcCID, sizeof(srcCID));
        pAcl->dstCIDNum = srcCnt;
    }

    return state;
}

ble_sts_t blc_l2cap_sendCocData(u16 connHandle, u16 srcCID, u8* data, u16 dataLen)
{
    if(BLE_SUCCESS != blt_ll_isAclhdlInvalid(connHandle))
    {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    l2cap_coc_cid_t* pCid = blt_l2cap_cocGetSrcCID(connHandle, srcCID);

    if(pCid == NULL || data == NULL || dataLen == 0 || dataLen > pCid->mtu)
    {
        return L2CAP_ERR_INVALID_PARAMETER;
    }

    if(pCid->sendTotalLen)
    {
        return L2CAP_ERR_COC_DATA_STILL_SENT;
    }

    pCid->sendTotalLen = dataLen;
    pCid->sendOffsetLen = 0;
    pCid->pTxSdu = data;
    sL2capCocCtrl.modelBusy = 1;
    return BLE_SUCCESS;
}

void blc_l2cap_cocMainLoop(void)
{
    if(sL2capCocCtrl.modelBusy)
    {
        sL2capCocCtrl.modelBusy = 0;
        for(int i=0; i<sL2capCocCtrl.cocCidCnt; i++)
        {
            if(blt_l2cap_cocSendData(&sL2capCocCtrl.pCocCid[i]))
            {
                sL2capCocCtrl.modelBusy = 1;
            }
        }
#if L2CAP_SERVER_FEATURE_SUPPORTED_EATT
        for(int i=0; i<sL2capCocCtrl.eattCidCnt; i++)
        {
            if(blt_l2cap_cocSendData(&sL2capCocCtrl.pEattCid[i]))
            {
                sL2capCocCtrl.modelBusy = 1;
            }
        }
#endif
    }
}

#endif
