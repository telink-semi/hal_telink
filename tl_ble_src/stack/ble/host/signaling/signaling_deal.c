/********************************************************************************************************
 * @file    signaling_deal.c
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

u16 blt_signal_proc_cmdRejectRsp(signalConCb_t *pSignalConCb, signal_pkt_t* signal, u16 signalLen)
{
    (void)signalLen;
#if L2CAP_CREDIT_BASED_FLOW_CONTROL_MODE_EN
    u16 connHandle = pSignalConCb->connHandle;

    blt_signal_cmdRejectRsp_t* pRsp = (blt_signal_cmdRejectRsp_t*)signal;

    if(blt_l2cap_getCreateCoc(connHandle))
    {
        blt_l2cap_reportCocCreateConnectFinishEvent(connHandle,  signal->code, pRsp->reason);
    }
#endif
    return 0;
}

u16 blt_signal_proc_connParamUpdateReq(signalConCb_t *pSignalConCb, signal_pkt_t* signal, u16 signalLen)
{
    (void)signalLen;
    u16 connHandle = pSignalConCb->connHandle;
    if(connHandle & BLM_CONN_HANDLE)
    {
        blt_signal_connParamUpdateReq_t* pReq = (blt_signal_connParamUpdateReq_t*)signal;

        u32 interval_us = pReq->intervalMin*1250;  //1.25ms unit
        u32 timeout_us = pReq->timeout*10000; //10ms unit
        u32 long_suspend_us = interval_us * (pReq->latency+1);
        signal_pkt_connParamUpdateRsp_t pRsp;

        //interval <= 320ms;  long suspend < 3S;  interval * (latency +1)*2 <= timeout
        if(pReq->intervalMin >= CONN_INTERVAL_320MS || long_suspend_us >= 3000000 || (long_suspend_us*2>timeout_us)){
            pRsp.result = CONN_PARAM_UPDATE_REJECT;
            return blt_signal_packageConnParamUpdateRsp(pSignalConCb->signalTxBuff, signal->identifier, &pRsp);
        }

        if(gap_eventMask & GAP_EVT_MASK_L2CAP_CONN_PARAM_UPDATE_REQ)
        {
            gap_l2cap_connParamUpdateReqEvt_t param_evt = {
                .connHandle = connHandle,
                .id = pReq->identifier,
                .min_interval = pReq->intervalMin,
                .max_interval = pReq->intervalMax,
                .latency = pReq->latency,
                .timeout = pReq->timeout,
            };

            blc_gap_send_event(GAP_EVT_L2CAP_CONN_PARAM_UPDATE_REQ, (u8*)&param_evt, sizeof(gap_l2cap_connParamUpdateReqEvt_t));
            return 0;
        }
        else
        {
            blm_l2cap_processConnParamUpdatePending(connHandle, pReq->intervalMin, pReq->intervalMax, pReq->latency, pReq->timeout);
            pRsp.result = CONN_PARAM_UPDATE_ACCEPT;
            return blt_signal_packageConnParamUpdateRsp(pSignalConCb->signalTxBuff, signal->identifier, &pRsp);
        }
    }
    return blt_signal_packageCmdRejectRspUnderstood(pSignalConCb->signalTxBuff, signal->identifier);
}

u16 blt_signal_proc_connParamUpdateRsp(signalConCb_t *pSignalConCb, signal_pkt_t* signal, u16 signalLen)
{
    (void)signalLen;
    u16 connHandle = pSignalConCb->connHandle;
    if( connHandle & BLM_CONN_HANDLE ){ ///slave send req and receive rsp
        return 0;
    }

    blt_signal_connParamUpdateRsp_t* pRsp = (blt_signal_connParamUpdateRsp_t*)signal;

    if(gap_eventMask & GAP_EVT_MASK_L2CAP_CONN_PARAM_UPDATE_RSP)
    {
        gap_l2cap_connParamUpdateRspEvt_t param_evt = {
            .connHandle = connHandle,
            .id = pRsp->identifier,
            .result = pRsp->result
        };
        blc_gap_send_event ( GAP_EVT_L2CAP_CONN_PARAM_UPDATE_RSP, (u8*)&param_evt, sizeof(gap_l2cap_connParamUpdateRspEvt_t) );
    }

    return 0;
}

#if L2CAP_CREDIT_BASED_FLOW_CONTROL_MODE_EN

u16 blt_signal_proc_disconnReq(signalConCb_t *pSignalConCb, signal_pkt_t* signal, u16 signalLen)
{
    (void)signalLen;
    u16 connHandle = pSignalConCb->connHandle;

    blt_signal_disconnReq_t* pReq = (blt_signal_disconnReq_t*)signal;

    l2cap_coc_cid_t* pCid = blt_l2cap_cocGetDstCID(connHandle, pReq->dstCID);

    if(pCid == NULL)
    {
        l2cap_cmdRejectCidData_t invalidCID = {
            .srcCID = pReq->srcCID,
            .dstCID = pReq->dstCID,
        };
        return blt_signal_packageCmdRejectRspInvalidCid(pSignalConCb->signalTxBuff, signal->identifier, &invalidCID);
    }

    blt_l2cap_reportCocDisconnectEvent(pCid);

    signal_pkt_disconnRsp_t pRsp;
    pRsp.dstCID = pReq->dstCID;
    pRsp.srcCID = pReq->srcCID;

    return blt_signal_packageDisconnRsp(pSignalConCb->signalTxBuff, signal->identifier, &pRsp);
}

u16 blt_signal_proc_disconnRsp(signalConCb_t *pSignalConCb, signal_pkt_t* signal, u16 signalLen)
{
    (void)signalLen;
    u16 connHandle = pSignalConCb->connHandle;

    blt_signal_disconnRsp_t* pRsp = (blt_signal_disconnRsp_t*)signal;

    l2cap_coc_cid_t* pCid = blt_l2cap_cocGetSrcCID(connHandle, pRsp->dstCID);

    if(pCid)
    {
        blt_l2cap_reportCocDisconnectEvent(pCid);
    }

    return 0;
}

u16 blt_signal_proc_leCreditBasedConnReq(signalConCb_t *pSignalConCb, signal_pkt_t* signal, u16 signalLen)
{
    (void)signalLen;
    u16 connHandle = pSignalConCb->connHandle;

    blt_signal_leCreditBasedConnReq_t* pReq = (blt_signal_leCreditBasedConnReq_t*)signal;

    if(pReq->dataLength != sizeof(signal_pkt_leCreditBasedConnReq_t))
        return blt_signal_packageCmdRejectRspUnderstood(pSignalConCb->signalTxBuff, signal->identifier);

    signal_pkt_leCreditBasedConnRsp_t pRsp;
    memset(&pRsp, 0, sizeof(signal_pkt_leCreditBasedConnRsp_t));
    pRsp.MTU = blt_l2cap_cocGetRecvMtu();
    pRsp.MPS = blt_l2cap_cocGetRecvMps();
    pRsp.initialCredits = SIGNAL_DEFAULT_INITIAL_CREDITS;

    //L2CAP implementations shall support a minimum MTU size of 23 octets.
    //L2CAP implementations shall support a minimum MPS of 23 octets and may support an MPS up to 65533 octets.
    if(pReq->MTU < SIGNAL_MINIMUM_MTU || pReq->MPS < SIGNAL_MINIMUM_MPS || pReq->MPS > SIGNAL_MAXIMUM_MPS || pReq->MTU < pReq->MPS)
    {
        pRsp.result = CONN_REFUSED_UNACCEPTABLE_PARAMETERS;
        return blt_signal_packageLeCreditBasedConnRsp(pSignalConCb->signalTxBuff, signal->identifier, &pRsp);
    }

    if(SIGNAL_CHECK_DYNAMIC_SPSM(pReq->SPSM))
    {
        if(blt_l2cap_cocGetRecvSpsm() != pReq->SPSM)
        {
            pRsp.result = CONN_REFUSED_SPSM_NOT_SUPPORT;
            return blt_signal_packageLeCreditBasedConnRsp(pSignalConCb->signalTxBuff, signal->identifier, &pRsp);
        }
    }
    else if(!SIGNAL_CHECK_SIG_ASSIGNED_SPSM(pReq->SPSM))
    {
        pRsp.result = CONN_REFUSED_SPSM_NOT_SUPPORT;
        return blt_signal_packageLeCreditBasedConnRsp(pSignalConCb->signalTxBuff, signal->identifier, &pRsp);
    }

    if(!SIGNAL_CHECK_DYNAMIC_CID(pReq->srcCID))
    {
        pRsp.result = CONN_REFUSED_INVALID_SOURCE_CID;
        return blt_signal_packageLeCreditBasedConnRsp(pSignalConCb->signalTxBuff, signal->identifier, &pRsp);
    }

    if(blt_l2cap_cocGetDstCID(connHandle, pReq->srcCID))
    {
        pRsp.result = CONN_REFUSED_SOURCE_CID_ALREADY_ALLOCATED;
        return blt_signal_packageLeCreditBasedConnRsp(pSignalConCb->signalTxBuff, signal->identifier, &pRsp);
    }

    if(blt_l2cap_cocCheckNotAvailableResources(pReq->SPSM))
    {
        pRsp.result = CONN_REFUSED_NO_RESOURCES_AVAILABLE;
        return blt_signal_packageLeCreditBasedConnRsp(pSignalConCb->signalTxBuff, signal->identifier, &pRsp);
    }

    l2cap_coc_cid_t* pCid = blt_l2cap_cocCreateNewCID(connHandle, pReq->SPSM, pReq->srcCID);

    if(pCid == NULL)
    {
        pRsp.result = CONN_REFUSED_NO_RESOURCES_AVAILABLE;
        return blt_signal_packageLeCreditBasedConnRsp(pSignalConCb->signalTxBuff, signal->identifier, &pRsp);
    }

    pCid->mps = pReq->MPS;
    pCid->mtu = pReq->MTU;
    pCid->recvCredits = SIGNAL_DEFAULT_INITIAL_CREDITS;
    pCid->sendCredits = pReq->initialCredits;

    pRsp.dstCID = pCid->dstCID;
    pRsp.result = CONN_SUCCESSFUL;

    blt_l2cap_reportCocConnectEvent(pCid);

    return blt_signal_packageLeCreditBasedConnRsp(pSignalConCb->signalTxBuff, signal->identifier, &pRsp);
}

u16 blt_signal_proc_leCreditBasedConnRsp(signalConCb_t *pSignalConCb, signal_pkt_t* signal, u16 signalLen)
{
    (void)signalLen;
    u16 connHandle = pSignalConCb->connHandle;

    l2cap_coc_acl_t* pAcl = blt_l2cap_getCreateCoc(connHandle);

    if(pAcl == NULL)
    {
        return blt_signal_packageCmdRejectRspUnderstood(pSignalConCb->signalTxBuff, signal->identifier);
    }

    blt_signal_leCreditBasedConnRsp_t* pRsp = (blt_signal_leCreditBasedConnRsp_t*)signal;

    if(pRsp->result != CONN_SUCCESSFUL)
    {
        blt_l2cap_reportCocCreateConnectFinishEvent(connHandle, pRsp->code, pRsp->result);
        return 0;
    }


    if(pRsp->MTU < SIGNAL_MINIMUM_MTU || pRsp->MPS < SIGNAL_MINIMUM_MPS || pRsp->MPS > SIGNAL_MAXIMUM_MPS || pRsp->MTU < pRsp->MPS)
    {
        //recv error code, need send Disconnect Req to disconnect.
        l2cap_coc_cid_t cid = {
            .connHandle = connHandle,
            .SPSM = pAcl->SPSM,
            .mtu = pRsp->MTU,
            .srcCID = pRsp->dstCID,
            .dstCID = pAcl->dstCID[0],
        };
        blt_l2cap_reportCocConnectEvent(&cid);
        blt_signal_sendDisconnReq(&cid);
        blt_l2cap_reportCocCreateConnectFinishEvent(connHandle, pRsp->code, CONN_REFUSED_UNACCEPTABLE_PARAMETERS);
        return 0;
    }

    if(blt_l2cap_cocGetSrcCID(connHandle, pRsp->dstCID))
    {
        blt_l2cap_reportCocCreateConnectFinishEvent(connHandle, pRsp->code, CONN_SUCCESSFUL);
        return 0;
    }

    l2cap_coc_cid_t* pCid = blt_l2cap_cocCreateNewCID(connHandle, pAcl->SPSM, pRsp->dstCID);

    if(pCid == NULL)
    {
        l2cap_coc_cid_t cid = {
            .connHandle = connHandle,
            .SPSM = pAcl->SPSM,
            .mtu = pRsp->MTU,
            .srcCID = pRsp->dstCID,
            .dstCID = pAcl->dstCID[0],
        };
        blt_l2cap_reportCocConnectEvent(&cid);
        blt_signal_sendDisconnReq(&cid);
        blt_l2cap_reportCocCreateConnectFinishEvent(connHandle, pRsp->code, CONN_REFUSED_NO_RESOURCES_AVAILABLE);

        return 0;
    }

    pCid->dstCID = pAcl->dstCID[0];
    pCid->mtu = pRsp->MTU;
    pCid->mps = pRsp->MPS;
    pCid->sendCredits = pRsp->initialCredits;
    pCid->recvCredits = SIGNAL_DEFAULT_INITIAL_CREDITS;

    blt_l2cap_reportCocConnectEvent(pCid);
    blt_l2cap_reportCocCreateConnectFinishEvent(connHandle, pRsp->code, CONN_SUCCESSFUL);
    return 0;
}

u16 blt_signal_proc_flowCtrlCreditInd(signalConCb_t *pSignalConCb, signal_pkt_t* signal, u16 signalLen)
{
    (void)signalLen;
    u16 connHandle = pSignalConCb->connHandle;

    blt_signal_flowCtrlCreditInd_t* pInd = (blt_signal_flowCtrlCreditInd_t*)signal;

    l2cap_coc_cid_t* pCid = blt_l2cap_cocGetSrcCID(connHandle, pInd->CID);
    if(pCid == NULL)
    {
        return 0;
    }

    if(pCid->sendCredits + pInd->credits > 65535)
    {
        blt_signal_sendDisconnReq(pCid);
    }
    else
    {
        pCid->sendCredits += pInd->credits;
    }

    return 0;
}


u16 blt_signal_proc_creditBasedConnReq(signalConCb_t *pSignalConCb, signal_pkt_t* signal, u16 signalLen)
{
    (void)signalLen;
    u16 connHandle = pSignalConCb->connHandle;

    blt_signal_creditBasedConnReq_t* pReq = (blt_signal_creditBasedConnReq_t*)signal;

    u8 srcCidNum = (pReq->dataLength - 8)>>1;

    signal_pkt_creditBasedConnRsp_t pRsp;
    memset(&pRsp, 0, sizeof(signal_pkt_creditBasedConnRsp_t));

    pRsp.MTU = blt_l2cap_cocGetRecvMtu();
    pRsp.MPS = blt_l2cap_cocGetRecvMps();
    pRsp.initialCredits = SIGNAL_DEFAULT_INITIAL_CREDITS;
    pRsp.result = ALL_CONN_SUCCESSFUL;

    l2cap_coc_acl_t* pAcl = blt_l2cap_getCreateCoc(connHandle);

    if(pAcl)
    {
        pRsp.result = SOME_CONN_REFUSED_INSUFFICIENT_RESOURCES_AVA;
        return blt_signal_packageCreditBasedConnRsp(pSignalConCb->signalTxBuff, signal->identifier, srcCidNum, &pRsp);
    }

    //L2CAP implementations shall support a minimum MTU size of 64 octets.
    //L2CAP implementations shall support a minimum MPS of 64 octets and may support an MPS up to 65533 octets.
    if(pReq->MTU < SIGNAL_CREDIT_MINIMUM_MTU || pReq->MPS < SIGNAL_CREDIT_MINIMUM_MPS || pReq->MPS > SIGNAL_CREDIT_MAXIMUM_MPS || pReq->MTU < pReq->MPS || pReq->initialCredits == 0)
    {
        pRsp.result = ALL_CONN_REFUSED_INVALID_PARAMETERS;
        return blt_signal_packageCreditBasedConnRsp(pSignalConCb->signalTxBuff, signal->identifier, srcCidNum, &pRsp);
    }

    if(SIGNAL_CHECK_DYNAMIC_SPSM(pReq->SPSM))
    {
        if(blt_l2cap_cocGetRecvSpsm() != pReq->SPSM)
        {
            pRsp.result = ALL_CONN_REFUSED_SPSM_NOT_SUPPORTED;
            return blt_signal_packageCreditBasedConnRsp(pSignalConCb->signalTxBuff, signal->identifier, srcCidNum, &pRsp);
        }
    }
    else if(!SIGNAL_CHECK_SIG_ASSIGNED_SPSM(pReq->SPSM))
    {
        pRsp.result = ALL_CONN_REFUSED_SPSM_NOT_SUPPORTED;
        return blt_signal_packageCreditBasedConnRsp(pSignalConCb->signalTxBuff, signal->identifier, srcCidNum, &pRsp);
    }

    for(int i=0; i<srcCidNum; i++)
    {
        if(!SIGNAL_CHECK_DYNAMIC_CID(pReq->srcCID[i]))
        {
            pRsp.result = SOME_CONN_REFUSED_INVALID_SOURCE_CID;
            continue;   //Some connections refused �C invalid Source CID
        }

        if(blt_l2cap_cocGetSrcCID(connHandle, pReq->srcCID[i]))
        {
            pRsp.result = SOME_CONN_REFUSED_SOURCE_CID_ALREADY_ALLOCATED;
            continue;   //Some connections refused �C Source CID already allocated
        }

        l2cap_coc_cid_t* pCid = blt_l2cap_cocCreateNewCID(connHandle, pReq->SPSM, pReq->srcCID[i]);

        if(pCid == NULL)
        {
            pRsp.result = SOME_CONN_REFUSED_INSUFFICIENT_RESOURCES_AVA;
            continue;   //Some connections refused �C insufficient resources available
        }

        pRsp.dstCID[i] = pCid->dstCID;
        pCid->mps = pReq->MPS;
        pCid->mtu = pReq->MTU;
        pCid->SPSM = pReq->SPSM;
        pCid->recvCredits = SIGNAL_DEFAULT_INITIAL_CREDITS;
        pCid->sendCredits = pReq->initialCredits;
        blt_l2cap_reportCocConnectEvent(pCid);
    }

    return blt_signal_packageCreditBasedConnRsp(pSignalConCb->signalTxBuff, signal->identifier, srcCidNum, &pRsp);
}

u16 blt_signal_proc_creditBasedConnRsp(signalConCb_t *pSignalConCb, signal_pkt_t* signal, u16 signalLen)
{
    u16 connHandle = pSignalConCb->connHandle;

    l2cap_coc_acl_t* pAcl = blt_l2cap_getCreateCoc(connHandle);

    if(pAcl == NULL)
    {
        return blt_signal_packageCmdRejectRspUnderstood(pSignalConCb->signalTxBuff, signal->identifier);
    }

    blt_signal_creditBasedConnRsp_t* pRsp = (blt_signal_creditBasedConnRsp_t*)signal;

    if(!(pRsp->result == ALL_CONN_SUCCESSFUL ||
            pRsp->result == SOME_CONN_REFUSED_INSUFFICIENT_RESOURCES_AVA ||
            pRsp->result == SOME_CONN_REFUSED_INVALID_SOURCE_CID ||
            pRsp->result == SOME_CONN_REFUSED_SOURCE_CID_ALREADY_ALLOCATED
    ))
    {
        //clear coc value
        blt_l2cap_reportCocCreateConnectFinishEvent(connHandle, pRsp->code, pRsp->result);
        return 0;
    }

    for(size_t i=0; i<((signalLen-sizeof(blt_signal_creditBasedConnRsp_t))>>1) + 1; i++)
    {
        if(pRsp->dstCID[i] == 0)
        {
            continue;
        }

        if(blt_l2cap_cocGetSrcCID(connHandle, pRsp->dstCID[i]))
        {
            continue;
        }

        l2cap_coc_cid_t* pCid = blt_l2cap_cocCreateNewCID(connHandle, pAcl->SPSM, pRsp->dstCID[i]);

        if(pCid == NULL)
        {
            l2cap_coc_cid_t cid = {
                .connHandle = connHandle,
                .SPSM = pAcl->SPSM,
                .mtu = pRsp->MTU,
                .srcCID = pRsp->dstCID[i],
                .dstCID = pAcl->dstCID[i],
            };
            blt_l2cap_reportCocConnectEvent(&cid);

            blt_signal_sendDisconnReq(&cid);
            continue;
        }

        pCid->dstCID = pAcl->dstCID[i];
        pCid->mtu = pRsp->MTU;
        pCid->mps = pRsp->MPS;
        pCid->sendCredits = pRsp->initialCredits;
        pCid->recvCredits = SIGNAL_DEFAULT_INITIAL_CREDITS;
        blt_l2cap_reportCocConnectEvent(pCid);
    }

    blt_l2cap_reportCocCreateConnectFinishEvent(connHandle, pRsp->code, pRsp->result);
    return 0;
}

#define blt_signal_pktCreditRecfgRsp(result)        blt_signal_packageCreditBasedRecfgRsp(pSignalConCb->signalTxBuff, signal->identifier, result)
u16 blt_signal_proc_creditBasedRecfgReq(signalConCb_t *pSignalConCb, signal_pkt_t* signal, u16 signalLen)
{
    u16 connHandle = pSignalConCb->connHandle;

    blt_signal_creditBasedRecfgReq_t* pReq = (blt_signal_creditBasedRecfgReq_t*)signal;

    u16 dstCidCnt = ((signalLen - sizeof(blt_signal_creditBasedRecfgReq_t))>>1) + 1;

    if(dstCidCnt > 5 || dstCidCnt == 0 || pReq->MTU<SIGNAL_CREDIT_MINIMUM_MTU || pReq->MPS<SIGNAL_CREDIT_MINIMUM_MPS)
    {
        return blt_signal_pktCreditRecfgRsp(RECONFIGURATION_FAILED_OTHER_UNACCEPTABLE_PARAMS);
    }


    u16 minMtu = 0x0000, minMps = 0x0000;

    for(int i=0; i<dstCidCnt ;i++)
    {
        l2cap_coc_cid_t* pCid = blt_l2cap_cocGetSrcCID(connHandle, pReq->dstCID[i]);
        if(pCid == NULL)
        {
            return blt_signal_pktCreditRecfgRsp(RECONFIGURATION_FAILED_DST_CIDS_INVALID);
        }
        minMtu = max(minMtu, pCid->mtu);
        minMps = max(minMps, pCid->mps);
    }

    if(pReq->MTU < minMtu)
    {
        return blt_signal_pktCreditRecfgRsp(RECONFIGURATION_FAILED_MTU_NOT_ALLOWED);
    }

    if(pReq->MPS < minMps && dstCidCnt > 1)
    {
        return blt_signal_pktCreditRecfgRsp(RECONFIGURATION_FAILED_MPS_NOT_ALLOWED);
    }

    for(int i=0; i<dstCidCnt ;i++)
    {
        l2cap_coc_cid_t* pCid = blt_l2cap_cocGetSrcCID(connHandle, pReq->dstCID[i]);
        if(pCid == NULL)
        {
            return blt_signal_pktCreditRecfgRsp(RECONFIGURATION_FAILED_DST_CIDS_INVALID);
        }
        pCid->mps = pReq->MPS;
        pCid->mtu = pReq->MTU;
        blt_l2cap_reportCocReconnfigureEvent(pCid);
    }

    return blt_signal_pktCreditRecfgRsp(RECONFIGURATION_SUCCESSFUL);
}

u16 blt_signal_proc_creditBasedRecfgRsp(signalConCb_t *pSignalConCb, signal_pkt_t* signal, u16 signalLen)
{
    (void)pSignalConCb;
    (void)signal;
    (void)signalLen;
    return 0;
}

#endif
