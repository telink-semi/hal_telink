/********************************************************************************************************
 * @file    tbs_client.c
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

#define BLT_GTBS_LOG BLT_TBS_LOG

static int  blt_ccp_disconnect(u16 connHandle);
static void blt_gtbsc_readAttrValCb(u16 connHandle, u8 err, gattc_read_cfg_t *pRdCfg);
static void blt_gtbsc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);

static const blc_gapc_discList_t discCcp;
#define BLC_GTBS_START_SDP(connHandle) blc_gapc_registerDiscoveryService(connHandle, &discCcp)

static const blc_gapc_reconnList_t reconnCcp;
#define BLC_GTBS_START_RECONN(connHandle) blc_gapc_registerReconnectService(connHandle, &reconnCcp)

_attribute_ble_data_retention_
    blc_ccp_client_ctrl_t ccp_client_ctrl = {
        .process = {
                    .pNext       = NULL,
                    .id          = AUDIO_GTBS_CLIENT,
                    .usedAclRole = 0,
                    .init        = blt_ccp_init,
                    .connect     = blt_ccp_connect,
                    .discov      = blt_ccp_discovery,
                    .loop        = NULL,
                    .store       = blt_ccp_nv_store,
                    },
};

void blc_audio_registerCallControlClient(const blc_ccpc_regParam_t *param)
{
    blc_prf_registerServiceModule(BLT_GTBS_PTS_BQB_EN ? PRF_GAP_ACL_UNSPECIF : PRF_GAP_ACL_PERIPHERAL, (blc_prf_proc_t *)&ccp_client_ctrl, param);
}

blc_ccp_client_t *blt_ccp_getClientInst(u16 connHandle)
{
    int ret = blt_prf_getAclRole(connHandle);
    if (ret < 0 || ((!BLT_GTBS_PTS_BQB_EN) && ret == ACL_ROLE_CENTRAL)) {
        BLT_TBS_LOG("ERR: ACL role, unlikely: 0x%x", ret);

        if (ret >= 0) {
            /* CAP_v1.0.pdf, page14: CCP Call Control Client GAP Peripheral */
            /* Call Control Client : GAP TODO:Central and GAP Peripheral */
            blt_prf_sendSvrGapRoleErrEvt(connHandle, AUDIO_GTBS_CLIENT, ret);
        }

        return NULL;
    }

    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle
    return ccp_client_ctrl.pCcpClient[idx];
}

blc_gtbs_client_t *blt_gtbsc_getClientInst(u16 connHandle)
{
    blc_ccp_client_t *client = blt_ccp_getClientInst(connHandle);
    if (client == NULL) {
        return NULL;
    }

    return &client->gtbs;
}

int blt_ccp_init(u8 initType, const void *param)
{
#if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_ccp_client_t)), blc_ccp_client_t);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_gtbs_client_t)), blc_gtbs_client_t);
#endif
    (void)param;
    if (initType == PRF_PROC_INIT) {
        for (int i = 0; i < gAppAudioAclMaxNum; i++) {
            ccp_client_ctrl.pCcpClient[i] = blt_ccp_getClientControlBuffer(i);
            /* Clear CCP Client parameters  */
            memset(ccp_client_ctrl.pCcpClient[i], 0, sizeof(blc_ccp_client_t));
            /* Initialize Pointer buffer */
        }
    } else if (initType == PRF_PROC_DEINIT) {
    }
    return 0;
}

int blt_ccp_connect(u16 connHandle, prf_acl_state_enum connState)
{
    if (connState == PRF_ACL_STATE_DISCONN) {
        blt_ccp_disconnect(connHandle);
        BLT_TBS_LOG("Disconnect:0x%x", connHandle);
    } else {
        BLT_TBS_LOG("Connect:0x%x", connHandle);
    }
    return 0;
}

int blt_ccp_discovery(u16 connHandle)
{
    BLC_COMMON_SDP_DISCOVERY(connHandle, GTBS, gtbs);
}

int blt_ccp_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param)
{
    BLC_COMMON_NV_STORE(connHandle, GTBS, gtbs, callFriendlyNameHdl);
    return 0;
}

static int blt_ccp_disconnect(u16 connHandle)
{
    if (blt_ll_isAclHandleOutOfRange(connHandle) != BLE_SUCCESS) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    blc_ccp_client_t *ccp = blt_ccp_getClientInst(connHandle);

    for (int i = 0; i < ccp->tbsClientCount; i++) {
        memset(ccp->tbs[i], 0, sizeof(blc_tbs_client_t));
    }

    memset(ccp, 0, sizeof(blc_ccp_client_t));

    return BLE_SUCCESS;
}

/*************************************************************************
    AUDIO_EVT_GTBSC_START = AUDIO_EVT_TYPE_GTBSC,
    AUDIO_EVT_GTBS_BEARER_PROVIDER_NAME,
    AUDIO_EVT_GTBS_BEARER_TECHNOLOGY,
    AUDIO_EVT_GTBS_BEARER_URI_SCHEMES_SUPP_LIST,
    AUDIO_EVT_GTBS_BEARER_SIGNAL_STRENGTH,
    AUDIO_EVT_GTBS_BEARER_LIST_CURRENT_CALL, //refer to 'blc_gtbsc_listCurrentCallsEvt_t'
    AUDIO_EVT_GTBS_CCID,
    AUDIO_EVT_GTBS_STATUS_FLAGS,             //refer to 'blc_gtbsc_statusFlagsEvt_t'
    AUDIO_EVT_GTBS_INCOMING_CALL_TGT_URI,    //refer to 'blc_gtbsc_incomingCallTgtUriEvt_t'
    AUDIO_EVT_GTBS_CALL_STATE,               //refer to 'blc_gtbsc_listCallStateEvt_t'
    AUDIO_EVT_GTBS_TERM_REASON,              //refer to 'blc_gtbsc_termRsnEvt_t'
    AUDIO_EVT_GTBS_INCOMING_CALL,            //refer to 'blc_gtbsc_incomingCallEvt_t'
    AUDIO_EVT_GTBS_CALL_FRIENDLY_NAME,       //refer to 'blc_gtbsc_friendlyNameEvt_t'
    AUDIO_EVT_GTBS_CCP_NTF_RESULT_CODE,      //refer to 'blc_gtbsc_ccpNtfResultCodesEvt_t'
 *************************************************************************/

static void blt_gtbsc_sendBearerProviderNameEvt(blc_gtbs_client_t *pGtbsClt)
{
    blc_gtbsc_bearerProviderName_t evt;
    memcpy(evt.providerName, pGtbsClt->providerName, pGtbsClt->providerNameLen);
    evt.nameLen = pGtbsClt->providerNameLen;
    blt_prf_sendEvent(pGtbsClt->connHandle, AUDIO_EVT_GTBS_BEARER_PROVIDER_NAME, (u8 *)&evt, sizeof(blc_gtbsc_bearerProviderName_t));
}

static void blt_gtbsc_sendTechnologyEvt(blc_gtbs_client_t *pGtbsClt)
{
    blc_gtbsc_technology_t evt;
    evt.technology = pGtbsClt->technology;
    blt_prf_sendEvent(pGtbsClt->connHandle, AUDIO_EVT_GTBS_BEARER_TECHNOLOGY, (u8 *)&evt, sizeof(blc_gtbsc_technology_t));
}

static void blt_gtbsc_sendUriSchemeSuppListEvt(blc_gtbs_client_t *pGtbsClt)
{
    blc_gtbsc_uriSchemeSuppList_t evt;
    evt.suppLen = pGtbsClt->URISchemesSupportedListLen;
    memcpy(evt.uriSchemeSuppList, pGtbsClt->URISchemesSupportedList, pGtbsClt->URISchemesSupportedListLen);
    blt_prf_sendEvent(pGtbsClt->connHandle, AUDIO_EVT_GTBS_BEARER_URI_SCHEMES_SUPP_LIST, (u8 *)&evt, sizeof(blc_gtbsc_uriSchemeSuppList_t));
}

static void blt_gtbsc_sendSignalStrengthEvt(blc_gtbs_client_t *pGtbsClt)
{
    blc_gtbsc_signalStrength_t evt;
    evt.signalStrength = pGtbsClt->signalStrength;
    blt_prf_sendEvent(pGtbsClt->connHandle, AUDIO_EVT_GTBS_BEARER_SIGNAL_STRENGTH, (u8 *)&evt, sizeof(blc_gtbsc_signalStrength_t));
}

static void blt_gtbsc_sendStatusFlagsEvt(blc_gtbs_client_t *pGtbsClt)
{
    blc_gtbsc_statusFlagsEvt_t evt;
    evt.statusFlags = pGtbsClt->statusFlags.statusFlags;
    blt_prf_sendEvent(pGtbsClt->connHandle, AUDIO_EVT_GTBS_STATUS_FLAGS, (u8 *)&evt, sizeof(blc_gtbsc_statusFlagsEvt_t));
}

static void blt_gtbsc_sendListCurrentCallsEvt(blc_gtbs_client_t *pGtbsClt)
{
    blc_gtbsc_listCurrentCallsEvt_t pEvt;
    pEvt.listLen = pGtbsClt->listCurrCallsLen;
    memcpy(pEvt.currentListCall, pGtbsClt->listCurrCalls, pGtbsClt->listCurrCallsLen);

    blt_prf_sendEvent(pGtbsClt->connHandle, AUDIO_EVT_GTBS_BEARER_LIST_CURRENT_CALL, (u8 *)&pEvt, sizeof(blc_gtbsc_listCurrentCallsEvt_t));
}

static void blt_gtbsc_sendCallStateEvt(blc_gtbs_client_t *pGtbsClt)
{
    blc_gtbsc_listCallStateEvt_t pEvt;
    pEvt.stateLen = pGtbsClt->callStateLen;
    memcpy((u8 *)&pEvt.state[0].callIndex, (u8 *)&pGtbsClt->callState[0].callIndex, pGtbsClt->callStateLen);
    blt_prf_sendEvent(pGtbsClt->connHandle, AUDIO_EVT_GTBS_CALL_STATE, (u8 *)&pEvt, sizeof(blc_gtbsc_listCallStateEvt_t));
}

static void blt_gtbsc_sendTermReasonEvt(blc_gtbs_client_t *pGtbsClt)
{
    blc_gtbsc_termRsnEvt_t pEvt;
    pEvt.callIndex = pGtbsClt->termRsn.callIndex;
    pEvt.termRsn   = pGtbsClt->termRsn.reasonCode;
    blt_prf_sendEvent(pGtbsClt->connHandle, AUDIO_EVT_GTBS_TERM_REASON, (u8 *)&pEvt, sizeof(blc_gtbsc_termRsnEvt_t));
}

static void blt_gtbsc_sendIncomingCallTgtUriEvt(blc_gtbs_client_t *pGtbsClt)
{
    blc_gtbsc_incomingCallTgtUriEvt_t pEvt;
    pEvt.uriLen = pGtbsClt->uriLen - 1;
    memcpy(&pEvt.uri, &pGtbsClt->uri, pGtbsClt->uriLen);
    blt_prf_sendEvent(pGtbsClt->connHandle, AUDIO_EVT_GTBS_INCOMING_CALL_TGT_URI, (u8 *)&pEvt, sizeof(blc_gtbsc_incomingCallTgtUriEvt_t));
}

static void blt_gtbsc_sendIncomingCallEvt(blc_gtbs_client_t *pGtbsClt)
{
    blc_gtbsc_incomingCallEvt_t pEvt;
    pEvt.callLen = pGtbsClt->incomingCallLen - 1;
    memcpy(&pEvt.call, &pGtbsClt->incomingCall, pGtbsClt->incomingCallLen);
    blt_prf_sendEvent(pGtbsClt->connHandle, AUDIO_EVT_GTBS_INCOMING_CALL, (u8 *)&pEvt, sizeof(blc_gtbsc_incomingCallEvt_t));
}

static void blt_gtbsc_sendFriendlyNameEvt(blc_gtbs_client_t *pGtbsClt)
{
    blc_gtbsc_friendlyNameEvt_t pEvt;
    pEvt.nameLen = pGtbsClt->callFriendlyNameLen - 1;
    memcpy(&pEvt.name, &pGtbsClt->callFriendlyName, pGtbsClt->callFriendlyNameLen);
    blt_prf_sendEvent(pGtbsClt->connHandle, AUDIO_EVT_GTBS_CALL_FRIENDLY_NAME, (u8 *)&pEvt, sizeof(blc_gtbsc_friendlyNameEvt_t));
}

static void blt_gtbsc_sendCcpNtfResultCodeEvt(blc_gtbs_client_t *pGtbsClt, u8 reqOpcode, u8 callIndex, u8 resultCode)
{
    u8                                buf[sizeof(blc_gtbsc_ccpNtfResultCodesEvt_t)];
    blc_gtbsc_ccpNtfResultCodesEvt_t *pEvt = (blc_gtbsc_ccpNtfResultCodesEvt_t *)buf;
    pEvt->reqOpcode                        = reqOpcode;
    pEvt->callIndex                        = callIndex;
    pEvt->resultCode                       = resultCode;

    blt_prf_sendEvent(pGtbsClt->connHandle, AUDIO_EVT_GTBS_CCP_NTF_RESULT_CODE, buf, sizeof(blc_gtbsc_ccpNtfResultCodesEvt_t));
}

void blt_gtbsc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);

    if (attHdl == client->bearerProviderNameHdl) //e.g.: "CMCC"
    {
        valLen                  = min(valLen, sizeof(client->providerName));
        client->providerNameLen = valLen;
        memcpy(client->providerName, val, valLen);
        BLT_TBS_LOG("NTF INFO: ATT_HDL[0x%x] Bearer Provider Name[%.*s]", attHdl, valLen, val);
        blt_gtbsc_sendBearerProviderNameEvt(client);
    } else if (attHdl == client->bearerTechnologyHdl) {
        if (valLen != 1) {
            return;
        }

        BLT_TBS_LOG("NTF INFO: ATT_HDL[0x%x] Bearer Technology[%d]", attHdl, val[0]);
        client->technology = val[0];
        blt_gtbsc_sendTechnologyEvt(client);
    } else if (attHdl == client->bearerURISchemesSuppListHdl) //e.g.: tel,sip,skype
    {
        valLen                             = min(valLen, sizeof(client->URISchemesSupportedList));
        client->URISchemesSupportedListLen = valLen;
        memcpy(client->URISchemesSupportedList, val, valLen);
        BLT_TBS_LOG("NTF INFO: ATT_HDL[0x%x] Bearer URI Schemes Supported List[%.*s]", attHdl, valLen, val);
        blt_gtbsc_sendUriSchemeSuppListEvt(client);
    } else if (attHdl == client->bearerSignalStrengthHdl) {
        BLT_TBS_LOG("NTF INFO: ATT_HDL[0x%x] Bearer Signal Strength[%s]", attHdl, val[0]);
        if (valLen != 1) {
            return;
        }

        client->signalStrength = val[0];
        blt_gtbsc_sendSignalStrengthEvt(client);
    } else if (attHdl == client->bearerListCurrentCallsHdl) {
        BLT_TBS_LOG("NTF INFO: ATT_HDL[0x%x] Bearer List Current Calls[%.*s]", attHdl, valLen, val);

        valLen                   = min(valLen, sizeof(client->listCurrCalls));
        client->listCurrCallsLen = valLen;
        memcpy(client->listCurrCalls, val, valLen);
        if (valLen >= 1) {
            /* Send event to the upper layer: Call_State && Call_FLags && Call_URI: e.g.: Alerting, Outgoing, URI="tel:10086" */
            blt_gtbsc_sendListCurrentCallsEvt(client);
        }
    } else if (attHdl == client->statusFlagsHdl) {
        u16 stsFlg = bstream_to_u16_le(val);
        BLT_TBS_LOG("NTF INFO: ATT_HDL[0x%x] Status Flags[%s]", attHdl, blc_gtbs_getStatusFlagsDescription(stsFlg));
        if (valLen != sizeof(blc_tbs_status_flags_t)) {
            return;
        }

        client->statusFlags.statusFlags = stsFlg;

        /* Send event to the upper layer: Inband ringtone && Silent mode */
        blt_gtbsc_sendStatusFlagsEvt(client);
    } else if (attHdl == client->incomingCallTargetBearerURIHdl) {
        valLen                = min(valLen, sizeof(client->uri));
        client->uriLen        = valLen;
        client->uri.callIndex = val[0];
        memcpy(&client->uri.info, val + 1, valLen - 1);

        BLT_TBS_LOG("NTF INFO: ATT_HDL[0x%x] Incoming Call Target Bearer URI[CallIndex:%d][%.*s]",
                    attHdl,
                    client->uri.callIndex,
                    client->uriLen - 1,
                    client->uri.info);

        if (valLen >= 1) {
            /* Send event to the upper layer: Call_Index= 1, URI="tel:+15025551212" */
            blt_gtbsc_sendIncomingCallTgtUriEvt(client);
        }
    } else if (attHdl == client->callStateHdl) {
        BLT_TBS_LOG("NTF INFO: ATT_HDL[0x%x] Call State", attHdl);

        valLen               = min(valLen, sizeof(client->callState));
        client->callStateLen = valLen;
        memcpy(&client->callState[0].callIndex, val, valLen);

        if (valLen >= sizeof(blc_tbs_call_state_t)) {
            /* Send event to the upper layer: Call State: e.g.: Alerting */
            blt_gtbsc_sendCallStateEvt(client);
        }
    } else if (attHdl == client->terminationReasonHdl) {
        if (valLen != sizeof(client->termRsn)) {
            return;
        }
        u8 *pVal = val;
        u8  termRsn, callIndex;
        STREAM_TO_U8(callIndex, pVal);
        STREAM_TO_U8(termRsn, pVal);
        const char *termName = blt_gtbs_getTerminationReasonName(termRsn);
        BLT_TBS_LOG("NTF INFO: ATT_HDL[0x%x] Termination CallIndex[0x%x] Reason[%s]", attHdl, callIndex, termName);
        client->termRsn.termRsn = bstream_to_u16_le(val);
        /* Send event to the upper layer: Termination Reason: e.g.: Call_Index= 1, client terminated the call */
        blt_gtbsc_sendTermReasonEvt(client);
    } else if (attHdl == client->incomingCallHdl) {
        valLen                  = min(valLen, sizeof(client->incomingCall));
        client->incomingCallLen = valLen;

        client->incomingCall.callIndex = val[0];
        memcpy(client->incomingCall.info, val + 1, valLen - 1);

        BLT_TBS_LOG("NTF INFO: ATT_HDL[0x%x] Incoming Call CallIndex[0x%x] URI[%.*s]",
                    attHdl,
                    client->incomingCall.callIndex,
                    client->incomingCallLen - 1,
                    client->incomingCall.info);

        if (valLen >= 1) {
            /* Send event to the upper layer: Incoming Call: e.g.: Call_Index= 1, skype:xyz */
            blt_gtbsc_sendIncomingCallEvt(client);
        }
    } else if (attHdl == client->callFriendlyNameHdl) {
        valLen                      = min(valLen, sizeof(client->callFriendlyName));
        client->callFriendlyNameLen = valLen;

        client->callFriendlyName.callIndex = val[0];
        memcpy(client->callFriendlyName.info, val + 1, valLen - 1);
        BLT_TBS_LOG("NTF INFO: ATT_HDL[0x%x] Call Friendly Name [CallIndex:%d][%.*s]",
                    attHdl,
                    client->callFriendlyName.callIndex,
                    client->callFriendlyNameLen - 1,
                    client->callFriendlyName.info);

        if (valLen >= 1) {
            /* Send event to the upper layer: Friendly Name: e.g.: Call_Index = 1, FriendlyName:el */
            blt_gtbsc_sendFriendlyNameEvt(client);
        }
    }
    /*  */
    else if (attHdl == client->callControlPointHdl) {
        blt_gtbs_ccp_ntf_t ccpNtf;
        u8                *pVal = val;
        STREAM_TO_U8(ccpNtf.resultOpcode, pVal);
        STREAM_TO_U8(ccpNtf.callIndex, pVal);
        STREAM_TO_U8(ccpNtf.resultCode, pVal);
        BLT_TBS_LOG("NTF INFO: ATT_HDL[0x%x] Call Control Point Requested Opcode[%s] Call Index[0x%x] Result Code[%s]", attHdl, blc_gtbs_getCallControlPointOpcodeName(ccpNtf.resultOpcode), ccpNtf.callIndex, blt_ccp_ntfStatusFlagsStr(ccpNtf.resultCode));
        /* Send event to the upper layer:  Call Control Point Notification : e.g.: Requested Opcode=Originate, Call_Index = 1, Result Code=OPCODE NOT SUPPORTED */
        blt_gtbsc_sendCcpNtfResultCodeEvt(client, ccpNtf.resultOpcode, ccpNtf.callIndex, ccpNtf.resultCode);
    }
}

static void blt_gtbsc_displayInfo(u16 connHandle, blc_gtbs_client_t *client)
{
    BLT_GTBS_LOG("sdp over connHandle[0x%x]", connHandle);
    BLT_GTBS_LOG("  INFO: Provider Name is %s Universal call Identifier is %s", client->providerName, hex_to_str(client->UCI, client->UCILen));
    BLT_GTBS_LOG("  INFO: Technology is %x URI Schemes Supported List is %s", client->technology, client->URISchemesSupportedList);
    BLT_GTBS_LOG("  INFO: Signal Strength is %d, Interval is %d sec, List current calls size is %d", client->signalStrength, client->signalStrengthReportingInterval, client->listCurrCallsLen);
    BLT_GTBS_LOG("  INFO: CCID is %d status flags, inBand Rigtone[%d], silent Mode[%d]", client->ccid, client->statusFlags.inbandRingtone, client->statusFlags.silentMode);
    BLT_GTBS_LOG("  INFO: Incoming call target bearer URI, Index is %x URI is %s", client->uri.callIndex, client->uri.info);
    BLT_GTBS_LOG("  INFO: call State: Index[0x%x], state[0x%x], incoming/outgoing[%d], information withheld by Server[%d]/Network[%d]",
                 client->callState[0].callIndex,
                 client->callState[0].state,
                 client->callState[0].incomingOutgoing,
                 client->callState[0].infoWithheldByServer,
                 client->callState[0].infoWithheldByNetwork);
    BLT_GTBS_LOG("  INFO: Incoming call, Index is %x URI is %s", client->incomingCall.callIndex, client->incomingCall.info);
    BLT_GTBS_LOG("  INFO: call Friend Name, Index is %x URI is %s", client->callFriendlyName.callIndex, client->callFriendlyName.info);
    BLT_GTBS_LOG("  INFO: call Control Point Optional Opcodes is %x", client->callCtrlPointOptionalOp);
}

static void blt_gtbsc_foundService(u16 connHandle, u8 count, u16 startHandle, u16 endHandle)
{
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);

    if (count == 0xFF) {
        blc_prf_sendServiceDiscoveryFailEvent(connHandle, AUDIO_GTBS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        BLT_GTBS_LOG("ERR:not found GTBS");
        return;
    }

    if (count == 0) {
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, AUDIO_GTBS_CLIENT);
        blt_gtbsc_displayInfo(connHandle, client);
        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return;
    }

    client->ntfInput.startHdl     = startHandle;
    client->ntfInput.endHdl       = endHandle;
    client->ntfInput.ntfOrIndFunc = blt_gtbsc_dataInput;
    BLT_GTBS_LOG("  INFO: GTBS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, startHandle, endHandle);

    blc_prf_sendServiceDiscoveryFoundEvent(connHandle, AUDIO_GTBS_CLIENT, startHandle, endHandle);
}

static void blt_gtbsc_foundProviderNameChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gtbs_client_t *client     = blt_gtbsc_getClientInst(connHandle);
    client->bearerProviderNameHdl = valueHandle;
    BLT_GTBS_LOG("Bearer provider name ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gtbsc_providerNameStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->providerName[0];
    *readLen                  = &client->providerNameLen;
    *readMaxSize              = sizeof(client->providerName);
    *rdCbFunc                 = blt_gtbsc_readAttrValCb;
}

static void blt_gtbsc_foundCallerIdentifierChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    client->bearerUCIHdl      = valueHandle;
    BLT_GTBS_LOG("Bearer caller identifier ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gtbsc_callerIdentifierStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->UCI[0];
    *readLen                  = &client->UCILen;
    *readMaxSize              = sizeof(client->UCI);
    *rdCbFunc                 = blt_gtbsc_readAttrValCb;
}

static void blt_gtbsc_foundTechnologyChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gtbs_client_t *client   = blt_gtbsc_getClientInst(connHandle);
    client->bearerTechnologyHdl = valueHandle;
    BLT_GTBS_LOG("bearer technology ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gtbsc_technologyStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->technology;
    *readLen                  = NULL;
    *readMaxSize              = 1;
    *rdCbFunc                 = blt_gtbsc_readAttrValCb;
}

static void blt_gtbsc_foundUriListChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gtbs_client_t *client           = blt_gtbsc_getClientInst(connHandle);
    client->bearerURISchemesSuppListHdl = valueHandle;
    BLT_GTBS_LOG("Bearer URI Schemes Supported List ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gtbsc_uriListStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->URISchemesSupportedList[0];
    *readLen                  = &client->URISchemesSupportedListLen;
    *readMaxSize              = sizeof(client->URISchemesSupportedList);
    *rdCbFunc                 = blt_gtbsc_readAttrValCb;
}

static void blt_gtbsc_foundSignalStrengthChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gtbs_client_t *client       = blt_gtbsc_getClientInst(connHandle);
    client->bearerSignalStrengthHdl = valueHandle;
    BLT_GTBS_LOG("Bearer Signal Strength ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gtbsc_signalStrengthStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->signalStrength;
    *readLen                  = NULL;
    *readMaxSize              = 1;
    *rdCbFunc                 = blt_gtbsc_readAttrValCb;
}

static void blt_gtbsc_foundSignalReportIntervalChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gtbs_client_t *client    = blt_gtbsc_getClientInst(connHandle);
    client->reportingIntervalHdl = valueHandle;
    BLT_GTBS_LOG("Bearer Signal Strength Reporting Interval ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gtbsc_signalReportIntervalStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->signalStrengthReportingInterval;
    *readLen                  = NULL;
    *readMaxSize              = 1;
    *rdCbFunc                 = blt_gtbsc_readAttrValCb;
}

static void blt_gtbsc_foundCurrentCallListChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gtbs_client_t *client         = blt_gtbsc_getClientInst(connHandle);
    client->bearerListCurrentCallsHdl = valueHandle;
    BLT_GTBS_LOG("Bearer List Current Calls ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gtbsc_currentCallListStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->listCurrCalls[0];
    *readLen                  = &client->listCurrCallsLen;
    *readMaxSize              = sizeof(client->listCurrCalls);
    *rdCbFunc                 = blt_gtbsc_readAttrValCb;
}

static void blt_gtbsc_foundCcidChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    client->ccidHdl           = valueHandle;
    BLT_GTBS_LOG("Content Control ID (CCID) ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gtbsc_ccidStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->ccid;
    *readLen                  = NULL;
    *readMaxSize              = 1;
    *rdCbFunc                 = blt_gtbsc_readAttrValCb;
}

static void blt_gtbsc_foundStatusFlagsChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    client->statusFlagsHdl    = valueHandle;
    BLT_GTBS_LOG("status flags ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gtbsc_statusFlagsStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->statusFlags;
    *readLen                  = NULL;
    *readMaxSize              = sizeof(blc_tbs_status_flags_t);
    *rdCbFunc                 = blt_gtbsc_readAttrValCb;
}

static void blt_gtbsc_foundIncomingCallUriChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gtbs_client_t *client              = blt_gtbsc_getClientInst(connHandle);
    client->incomingCallTargetBearerURIHdl = valueHandle;
    BLT_GTBS_LOG("Incoming Call Target Bearer URI ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gtbsc_incomingCallUriStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->uri;
    *readLen                  = &client->uriLen;
    *readMaxSize              = sizeof(blc_tbs_incoming_call_target_bearer_uri_t);
    *rdCbFunc                 = blt_gtbsc_readAttrValCb;
}

static void blt_gtbsc_foundCallStateChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    client->callStateHdl      = valueHandle;
    BLT_GTBS_LOG("call state ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gtbsc_callStateStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->callState[0];
    *readLen                  = &client->callStateLen;
    *readMaxSize              = sizeof(client->callState);
    *rdCbFunc                 = blt_gtbsc_readAttrValCb;
}

static void blt_gtbsc_foundCallControlPointChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gtbs_client_t *client   = blt_gtbsc_getClientInst(connHandle);
    client->callControlPointHdl = valueHandle;
    BLT_GTBS_LOG("call control point ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gtbsc_foundCcpOptionalOpcodesChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gtbs_client_t *client             = blt_gtbsc_getClientInst(connHandle);
    client->callControlPointOptionalOpHdl = valueHandle;
    BLT_GTBS_LOG("Call Control Point Optional Opcodes ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gtbsc_ccpOptionalOpcodesStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->callCtrlPointOptionalOp;
    *readLen                  = NULL;
    *readMaxSize              = 2;
    *rdCbFunc                 = blt_gtbsc_readAttrValCb;
}

static void blt_gtbsc_foundTerminationReasonChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gtbs_client_t *client    = blt_gtbsc_getClientInst(connHandle);
    client->terminationReasonHdl = valueHandle;
    BLT_GTBS_LOG("Termination Reason ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gtbsc_foundIncomingCallChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    client->incomingCallHdl   = valueHandle;
    BLT_GTBS_LOG("incoming call ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gtbsc_incomingCallStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->incomingCall;
    *readLen                  = &client->incomingCallLen;
    *readMaxSize              = sizeof(blc_tbs_incoming_call_t);
    *rdCbFunc                 = blt_gtbsc_readAttrValCb;
}

static void blt_gtbsc_foundCallFriendlyNameChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gtbs_client_t *client   = blt_gtbsc_getClientInst(connHandle);
    client->callFriendlyNameHdl = valueHandle;
    BLT_GTBS_LOG("call friendly name ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gtbsc_callFriendlyNameStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->callFriendlyName;
    *readLen                  = &client->callFriendlyNameLen;
    *readMaxSize              = sizeof(blc_tbs_call_friendly_name_t);
    *rdCbFunc                 = blt_gtbsc_readAttrValCb;
}

static const blc_gapc_discService_t gtbsService = {
    .uuid = UUID16_INIT(SERVICE_UUID_GENERIC_TELEPHONE_BEARER),
    .sfun = blt_gtbsc_foundService,
};

static const blc_gapc_discChar_t gtbsChar[] = {
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_BEARER_PROVIDER_NAME),
     .cfun         = blt_gtbsc_foundProviderNameChar,
     .rfun         = blt_gtbsc_providerNameStartRead,
     },
    {
     .readValue = true,
     .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_BEARER_UCI),
     .cfun      = blt_gtbsc_foundCallerIdentifierChar,
     .rfun      = blt_gtbsc_callerIdentifierStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_BEARER_TECHNOLOGY),
     .cfun         = blt_gtbsc_foundTechnologyChar,
     .rfun         = blt_gtbsc_technologyStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_BEARER_URI_SCHEMES_SUPPRTED_LIST),
     .cfun         = blt_gtbsc_foundUriListChar,
     .rfun         = blt_gtbsc_uriListStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_BEARER_SS),
     .cfun         = blt_gtbsc_foundSignalStrengthChar,
     .rfun         = blt_gtbsc_signalStrengthStartRead,
     },
    {
     .readValue = true,
     .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_BEARER_SS_REPORTING_INTERVAL),
     .cfun      = blt_gtbsc_foundSignalReportIntervalChar,
     .rfun      = blt_gtbsc_signalReportIntervalStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_BEARER_LIST_CURRENT_CALLS),
     .cfun         = blt_gtbsc_foundCurrentCallListChar,
     .rfun         = blt_gtbsc_currentCallListStartRead,
     },
    {
     .readValue = true,
     .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_CONTENT_CONTROL_ID),
     .cfun      = blt_gtbsc_foundCcidChar,
     .rfun      = blt_gtbsc_ccidStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_STATUS_FLAGS),
     .cfun         = blt_gtbsc_foundStatusFlagsChar,
     .rfun         = blt_gtbsc_statusFlagsStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_INCOMING_CALL_TARGET_BEARER_URI),
     .cfun         = blt_gtbsc_foundIncomingCallUriChar,
     .rfun         = blt_gtbsc_incomingCallUriStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_CALL_STATE),
     .cfun         = blt_gtbsc_foundCallStateChar,
     .rfun         = blt_gtbsc_callStateStartRead,
     },
    {
     .subscribeNtf = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_CALL_CTRL_POINT),
     .cfun         = blt_gtbsc_foundCallControlPointChar,
     },
    {
     .readValue = true,
     .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_CALL_CTRL_POINT_OPTIONAL_OPCODES),
     .cfun      = blt_gtbsc_foundCcpOptionalOpcodesChar,
     .rfun      = blt_gtbsc_ccpOptionalOpcodesStartRead,
     },
    {
     .subscribeNtf = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_TERMINATION_REASON),
     .cfun         = blt_gtbsc_foundTerminationReasonChar,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_INCOMING_CALL),
     .cfun         = blt_gtbsc_foundIncomingCallChar,
     .rfun         = blt_gtbsc_incomingCallStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_CALL_FRIENDLY_NAME),
     .cfun         = blt_gtbsc_foundCallFriendlyNameChar,
     .rfun         = blt_gtbsc_callFriendlyNameStartRead,
     },
};

static const blc_gapc_discList_t discCcp = {
    .maxServiceCount = 1,
    .service         = &gtbsService,
    .includeTable    = {
                        .size = 0,
                        },
    .characteristicTable = {
                        .size           = ARRAY_SIZE(gtbsChar),
                        .characteristic = gtbsChar,
                        },
};

/**********reconnect function start*********/
static bool blt_gtbsc_reconnService(u16 connHandle, int count)
{
    if (count == 0) {
        blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
        blt_gtbsc_displayInfo(connHandle, client);
        BLT_TBS_LOG("  INFO: GTBS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, client->ntfInput.startHdl, client->ntfInput.endHdl);
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, AUDIO_GTBS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return true;
    }

    if (count > 1) {
        return false;
    }
    return true;
}

static int blt_gtbsc_providerNameGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->bearerProviderNameHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gtbsc_callerIdentifierGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ;
    charInfo->valueHandle = client->bearerUCIHdl;

    return 1;
}

static int blt_gtbsc_technologyGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->bearerTechnologyHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gtbsc_uriListGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->bearerURISchemesSuppListHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gtbsc_signalStrengthGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->bearerSignalStrengthHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gtbsc_signalReportIntervalGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ;
    charInfo->valueHandle = client->reportingIntervalHdl;

    return 1;
}

static int blt_gtbsc_currentCallListGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->bearerListCurrentCallsHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gtbsc_ccidGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ;
    charInfo->valueHandle = client->ccidHdl;

    return 1;
}

static int blt_gtbsc_statusFlagsGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->statusFlagsHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gtbsc_incomingCallUriGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->incomingCallTargetBearerURIHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gtbsc_callStateGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->callStateHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gtbsc_callControlPointGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    (void)connHandle;
    charInfo->properties = CHAR_PROP_NOTIFY;
    charInfo->cccHandle  = 0;
    return 1;
}

static int blt_gtbsc_ccpOptionalOpcodesGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ;
    charInfo->valueHandle = client->callControlPointOptionalOpHdl;

    return 1;
}

static int blt_gtbsc_terminationReasonGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    (void)connHandle;
    charInfo->properties = CHAR_PROP_NOTIFY;
    charInfo->cccHandle  = 0;

    return 1;
}

static int blt_gtbsc_incomingCallGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->incomingCallHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gtbsc_callFriendlyNameGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->callFriendlyNameHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static const blc_gapc_reconnChar_t reGtbsChar[] = {

    {
     .ifun = blt_gtbsc_providerNameGetInfo,
     .rfun = blt_gtbsc_providerNameStartRead,
     },

    {
     .ifun = blt_gtbsc_callerIdentifierGetInfo,
     .rfun = blt_gtbsc_callerIdentifierStartRead,
     },

    {
     .ifun = blt_gtbsc_technologyGetInfo,
     .rfun = blt_gtbsc_technologyStartRead,
     },

    {
     .ifun = blt_gtbsc_uriListGetInfo,
     .rfun = blt_gtbsc_uriListStartRead,
     },

    {
     .ifun = blt_gtbsc_signalStrengthGetInfo,
     .rfun = blt_gtbsc_signalStrengthStartRead,
     },

    {
     .ifun = blt_gtbsc_signalReportIntervalGetInfo,
     .rfun = blt_gtbsc_signalReportIntervalStartRead,
     },

    {
     .ifun = blt_gtbsc_currentCallListGetInfo,
     .rfun = blt_gtbsc_currentCallListStartRead,
     },

    {
     .ifun = blt_gtbsc_ccidGetInfo,
     .rfun = blt_gtbsc_ccidStartRead,
     },

    {
     .ifun = blt_gtbsc_statusFlagsGetInfo,
     .rfun = blt_gtbsc_statusFlagsStartRead,
     },

    {
     .ifun = blt_gtbsc_incomingCallUriGetInfo,
     .rfun = blt_gtbsc_incomingCallUriStartRead,
     },

    {
     .ifun = blt_gtbsc_callStateGetInfo,
     .rfun = blt_gtbsc_callStateStartRead,
     },

    {.ifun = blt_gtbsc_callControlPointGetInfo                                            },

    {
     .ifun = blt_gtbsc_ccpOptionalOpcodesGetInfo,
     .rfun = blt_gtbsc_ccpOptionalOpcodesStartRead,
     },

    {.ifun = blt_gtbsc_terminationReasonGetInfo                                            },

    {
     .ifun = blt_gtbsc_incomingCallGetInfo,
     .rfun = blt_gtbsc_incomingCallStartRead,
     },

    {
     .ifun = blt_gtbsc_callFriendlyNameGetInfo,
     .rfun = blt_gtbsc_callFriendlyNameStartRead,
     },
};

static const blc_gapc_reconnList_t reconnCcp = {
    .resfun = blt_gtbsc_reconnService,
    .charTb = {
               .size           = ARRAY_SIZE(reGtbsChar),
               .characteristic = reGtbsChar,
               },
    .inclSize = 0,
};

/**********reconnect function ending********/


/*************************************************************************
 *  GATTC Read Characteristics
 *  Read Bearer Provider Name 4.4.1 O
 *  Read Bearer UCI 4.4.2 O
 *  Read Bearer Technology 4.4.3 O
 *  Read Bearer URI Schemes Supported List 4.4.4 O
 *  Read Bearer Signal Strength 4.4.5 O
 *  Read Bearer Signal Strength Reporting Interval 4.4.6 O
 *  Read Bearer List Current Calls 4.4.8 O
 *  Read Content Control ID 4.4.9 O
 *  Read Incoming Call Target Bearer URI 4.4.10 O
 *  Read Status Flags 4.4.11 O
 *  Read Call State 4.4.12 M
 *  Read Call Control Point Optional Opcodes 4.4.14 O
 *  Read Incoming Call 4.4.15 O
 *  Read Call Friendly Name 4.4.16 O
 *************************************************************************/
static void blt_gtbsc_readAttrValCb(u16 connHandle, u8 err, gattc_read_cfg_t *pRdCfg)
{
    assert(blt_ll_isAclhdlInvalid(connHandle) == BLE_SUCCESS);

    if (err == GATT_ERR_DATA_LENGTH_EXCEED_MEM_RESTRICTION) {
        BLT_TBS_LOG("RD_CB INFO: ERR: Can not save all read values due to memory restrictions");
    } else if (err) {
        BLT_TBS_LOG("RD_CB INFO: ERR: read handle:[0x%x] err:[0x%x]", pRdCfg->single.handle, err);
        return;
    }

    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);


    u16 attHandle      = pRdCfg->single.handle;
    u8 *pAttVal        = pRdCfg->single.wBuff;
    u16 attValLen      = pRdCfg->single.wBuffLen == NULL ? pRdCfg->single.maxLen : *pRdCfg->single.wBuffLen;
    u16 validAttValLen = min(pRdCfg->single.maxLen, attValLen);
    //BLT_TBS_LOG("RD_CB INFO:  [%d %d %d]", attValLen, pRdCfg->single.maxLen, validAttValLen);

    if (attHandle == client->bearerProviderNameHdl) {
        BLT_TBS_LOG("RD_CB INFO: ATT_HDL[0x%x] Bearer Provider Name[%.*s]", attHandle, validAttValLen, pAttVal);
    } else if (attHandle == client->bearerUCIHdl) {
        BLT_TBS_LOG("RD_CB INFO: ATT_HDL[0x%x] Uniform Caller Identifier[%.*s]", attHandle, validAttValLen, pAttVal);
    } else if (attHandle == client->bearerTechnologyHdl) {
        const char *name = blc_gtbs_getBearerTechnologyName(pAttVal[0]);
        BLT_TBS_LOG("RD_CB INFO: ATT_HDL[0x%x] Bearer Technology[%s]", attHandle, name);
    } else if (attHandle == client->bearerURISchemesSuppListHdl) {
        BLT_TBS_LOG("NTF INFO: ATT_HDL[0x%x] Bearer URI Schemes Supported List[%.*s]", attHandle, validAttValLen, pAttVal);
    } else if (attHandle == client->bearerSignalStrengthHdl) {
        BLT_TBS_LOG("RD_CB INFO: ATT_HDL[0x%x] Bearer Signal Strength[%d]", attHandle, pAttVal[0]);
    } else if (attHandle == client->reportingIntervalHdl) {
        BLT_TBS_LOG("RD_CB INFO: ATT_HDL[0x%x] Bearer Signal Strength Reporting Interval[%d sec]", attHandle, pAttVal[0]);
    } else if (attHandle == client->bearerListCurrentCallsHdl) {
        BLT_TBS_LOG("RD_CB INFO: ATT_HDL[0x%x] Bearer List Current Calls", attHandle);

        u8                         callUri[STACK_AUDIO_CALL_MEMBERS_MAX_NUM][40]; //40
        blc_gtbsc_list_curr_call_t calls[STACK_AUDIO_CALL_MEMBERS_MAX_NUM];

        u8  listItemLen, i = 0;
        u16 len  = validAttValLen, currCallUriLen;
        u8 *pVal = pAttVal;

        while (len) {
            calls[i].pCallUri = &callUri[i][0];
            STREAM_TO_U8(listItemLen, pVal);
            STREAM_TO_U8(calls[i].callIndex, pVal);
            STREAM_TO_U8(calls[i].state, pVal);
            STREAM_TO_U8(calls[i].callFlags, pVal);
            len -= listItemLen + 1;

            currCallUriLen = listItemLen - 3;
            STREAM_TO_STR(calls[i].pCallUri, pVal, min(40, currCallUriLen));

            BLT_TBS_LOG("RD_CB INFO: Call State [%d] Call_Index[0x%x] Call_State[0x%x] Call_Flags[0x%x] Call_URI[%.*s]",
                        i,
                        calls[i].callIndex,
                        calls[i].state,
                        calls[i].callFlags,
                        min(40, currCallUriLen),
                        calls[i].pCallUri);
            i++;

            if (i == STACK_AUDIO_CALL_MEMBERS_MAX_NUM) {
                BLT_TBS_LOG("RD_CB WRN: Could not parse all calls due to memory restrictions");
                break;
            }
        }
        if (validAttValLen >= 1) {
            /* Send event to the upper layer: Call_State && Call_FLags && Call_URI: e.g.: Alerting, Outgoing, URI="tel:10086" */
            blt_gtbsc_sendListCurrentCallsEvt(client);
        }
    } else if (attHandle == client->ccidHdl) {
        BLT_TBS_LOG("RD_CB INFO: ATT_HDL[0x%x] Content Control ID[%d]", attHandle, pAttVal[0]);
    } else if (attHandle == client->statusFlagsHdl) {
        u16 stsFlg = bstream_to_u16_le(pAttVal);
        BLT_TBS_LOG("RD_CB INFO: ATT_HDL[0x%x] Status Flags[%s]", attHandle, blc_gtbs_getStatusFlagsDescription(stsFlg));

        /* Send event to the upper layer: Inband ringtone && Server Silent mode */
        blt_gtbsc_sendStatusFlagsEvt(client);
    } else if (attHandle == client->incomingCallTargetBearerURIHdl) {
        BLT_TBS_LOG("RD_CB INFO: ATT_HDL[0x%x] Incoming Call Target Bearer URI[callIndex:%d][%.*s]", attHandle, pAttVal[0], validAttValLen - 1, pAttVal + 1);
        if (validAttValLen >= 1) {
            /* Send event to the upper layer: Call_Index= 1, URI="tel:+15025551212" */
            blt_gtbsc_sendIncomingCallTgtUriEvt(client);
        }
    } else if (attHandle == client->callStateHdl) {
        BLT_TBS_LOG("RD_CB INFO: ATT_HDL[0x%x] Call State", attHandle);

        blc_gtbs_call_state_t callState[STACK_AUDIO_CALL_MEMBERS_MAX_NUM];
        u8                    i   = 0;
        u16                   len = validAttValLen;
        while (len) {
            len -= sizeof(blc_tbs_call_state_t);
            STREAM_TO_U8(callState[i].callIndex, pAttVal);
            STREAM_TO_U8(callState[i].state, pAttVal);
            STREAM_TO_U8(callState[i].callFlags, pAttVal);
            const char *callStateName = blc_gtbs_getCallStateName(callState[i].state);
            const char *callflagsName = blc_gtbs_getCallFlagsDescription(callState[i].callFlags);
            BLT_TBS_LOG("RD_CB INFO: [%d] Call_Index[%d] Call_State[%s] Call_Flags[%s]", i, callState[i].callIndex, callStateName, callflagsName);
            i++;

            if (i == STACK_AUDIO_CALL_MEMBERS_MAX_NUM) {
                BLT_TBS_LOG("RD_CB WRN: Could not parse all calls due to memory restrictions");
                break;
            }
        }
        if (validAttValLen >= sizeof(blc_tbs_call_state_t)) {
            /* Send event to the upper layer: Call State: e.g.: Alerting  */
            blt_gtbsc_sendCallStateEvt(client);
        }
    } else if (attHandle == client->callControlPointOptionalOpHdl) {
        BLT_TBS_LOG("RD_CB INFO: ATT_HDL[0x%x] Call Control Point Optional Opcodes[%s]", attHandle, pAttVal[0]);
    } else if (attHandle == client->incomingCallHdl) {
        BLT_TBS_LOG("RD_CB INFO: ATT_HDL[0x%x] Incoming Call URI[callIndex:%d][%.*s]", attHandle, pAttVal[0], attValLen - 1, pAttVal + 1);

        if (attValLen >= 1) {
            /* Send event to the upper layer: Incoming Call: e.g.: Call_Index= 1, skype:xyz */
            blt_gtbsc_sendIncomingCallEvt(client);
        }
    } else if (attHandle == client->callFriendlyNameHdl) {
        BLT_TBS_LOG("RD_CB INFO: ATT_HDL[0x%x] Call Friendly Name [CallIndex:%d][%.*s]", attHandle, pAttVal[0], attValLen - 1, pAttVal + 1);
        if (attValLen >= 1) {
            /* Send event to the upper layer: Friendly Name: e.g.: Call_Index = 1, FriendlyName:el */
            blt_gtbsc_sendFriendlyNameEvt(client);
        }
    }
}

static int blt_gtbsc_readAttrVal(u16 connHandle, blt_gtbs_read_enum rdType)
{
    BLT_TBS_LOG("blt_gtbsc_readAttrVal:%d", rdType);
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        BLT_TBS_LOG("ERR: ACL handle invalid");
        return HCI_ERR_UNKNOWN_CONN_ID;
    } else if (rdType >= GTBS_READ_MAX) {
        BLT_TBS_LOG("ERR: Invalid read type %d", rdType);
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);

    gapc_read_cfg_t pGapReCfg;
    pGapReCfg.handle = 0;
    pGapReCfg.func   = blt_gtbsc_readAttrValCb;

    if (rdType == GTBS_READ_BEARER_PROVIDER_NAME) {
        pGapReCfg.handle   = client->bearerProviderNameHdl;
        pGapReCfg.wBuff    = (u8 *)&client->providerName[0];
        pGapReCfg.wBuffLen = &client->providerNameLen;
        pGapReCfg.maxLen   = sizeof(client->providerName);
    } else if (rdType == GTBS_READ_BEARER_UCI) {
        pGapReCfg.handle   = client->bearerUCIHdl;
        pGapReCfg.wBuff    = (u8 *)&client->UCI[0];
        pGapReCfg.wBuffLen = &client->UCILen;
        pGapReCfg.maxLen   = sizeof(client->UCI);
    } else if (rdType == GTBS_READ_BEARER_TECHNOLOGY) {
        pGapReCfg.handle   = client->bearerTechnologyHdl;
        pGapReCfg.wBuff    = (u8 *)&client->technology;
        pGapReCfg.wBuffLen = NULL;
        pGapReCfg.maxLen   = 1;
    } else if (rdType == GTBS_READ_BEARER_URI_SCHEMES_SUPP_LIST) {
        pGapReCfg.handle   = client->bearerURISchemesSuppListHdl;
        pGapReCfg.wBuff    = (u8 *)&client->URISchemesSupportedList[0];
        pGapReCfg.wBuffLen = &client->URISchemesSupportedListLen;
        pGapReCfg.maxLen   = sizeof(client->URISchemesSupportedList);
    } else if (rdType == GTBS_READ_BEARER_SIGNAL_STRENGTH) {
        pGapReCfg.handle   = client->bearerSignalStrengthHdl;
        pGapReCfg.wBuff    = (u8 *)&client->signalStrength;
        pGapReCfg.wBuffLen = NULL;
        pGapReCfg.maxLen   = 1;
    } else if (rdType == GTBS_READ_BEARER_SIGNAL_STRENGTH_RPT_ITVL) {
        pGapReCfg.handle   = client->reportingIntervalHdl;
        pGapReCfg.wBuff    = (u8 *)&client->signalStrengthReportingInterval;
        pGapReCfg.wBuffLen = NULL;
        pGapReCfg.maxLen   = 1;
    } else if (rdType == GTBS_READ_BEARER_LIST_CURRENT_CALLS) {
        pGapReCfg.handle   = client->bearerListCurrentCallsHdl;
        pGapReCfg.wBuff    = (u8 *)&client->listCurrCalls[0];
        pGapReCfg.wBuffLen = &client->listCurrCallsLen;
        pGapReCfg.maxLen   = sizeof(client->listCurrCalls);
    } else if (rdType == GTBS_READ_CONTENT_CONTROL_ID) {
        pGapReCfg.handle   = client->ccidHdl;
        pGapReCfg.wBuff    = (u8 *)&client->ccid;
        pGapReCfg.wBuffLen = NULL;
        pGapReCfg.maxLen   = 1;
    } else if (rdType == GTBS_READ_INCOMING_CALL_TARGET_BEARER_URI) {
        pGapReCfg.handle   = client->incomingCallTargetBearerURIHdl;
        pGapReCfg.wBuff    = (u8 *)&client->uri.callIndex;
        pGapReCfg.wBuffLen = &client->uriLen;
        pGapReCfg.maxLen   = sizeof(client->uri);
    } else if (rdType == GTBS_READ_STATUS_FLAGS) {
        pGapReCfg.handle   = client->statusFlagsHdl;
        pGapReCfg.wBuff    = (u8 *)&client->statusFlags.statusFlags;
        pGapReCfg.wBuffLen = NULL;
        pGapReCfg.maxLen   = sizeof(client->statusFlags);
    } else if (rdType == GTBS_READ_CALL_STATE) {
        pGapReCfg.handle   = client->callStateHdl;
        pGapReCfg.wBuff    = (u8 *)&client->callState[0].callIndex;
        pGapReCfg.wBuffLen = &client->callStateLen;
        pGapReCfg.maxLen   = sizeof(client->callState);
    } else if (rdType == GTBS_READ_CALL_CONTROL_POINT_OPT_OPCODES) {
        pGapReCfg.handle   = client->callControlPointOptionalOpHdl;
        pGapReCfg.wBuff    = (u8 *)&client->callCtrlPointOptionalOp;
        pGapReCfg.wBuffLen = NULL;
        pGapReCfg.maxLen   = 2;
    } else if (rdType == GTBS_READ_INCOMING_CALL) {
        pGapReCfg.handle   = client->incomingCallHdl;
        pGapReCfg.wBuff    = (u8 *)&client->incomingCall.callIndex;
        pGapReCfg.wBuffLen = &client->incomingCallLen;
        pGapReCfg.maxLen   = sizeof(client->incomingCall);
    } else if (rdType == GTBS_READ_CALL_FRIENDLY_NAME) {
        pGapReCfg.handle   = client->callFriendlyNameHdl;
        pGapReCfg.wBuff    = (u8 *)&client->callFriendlyName.callIndex;
        pGapReCfg.wBuffLen = &client->callFriendlyNameLen;
        pGapReCfg.maxLen   = sizeof(client->callFriendlyName);
    }

    if (pGapReCfg.handle == 0) {
        BLT_TBS_LOG("ERR: Handle not set");
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    return blc_gapc_readAttributeValue(connHandle, &pGapReCfg);
}

int blc_gtbsc_readBearerProviderName(u16 connHandle, prf_read_cb_t readCb)
{
    (void)readCb;
    return blt_gtbsc_readAttrVal(connHandle, GTBS_READ_BEARER_PROVIDER_NAME);
}

int blc_gtbsc_readBearerUCI(u16 connHandle, prf_read_cb_t readCb)
{
    (void)readCb;
    return blt_gtbsc_readAttrVal(connHandle, GTBS_READ_BEARER_UCI);
}

int blc_gtbsc_readBearerTechnology(u16 connHandle, prf_read_cb_t readCb)
{
    (void)readCb;
    return blt_gtbsc_readAttrVal(connHandle, GTBS_READ_BEARER_TECHNOLOGY);
}

int blc_gtbsc_readBearerURISchemesSuppList(u16 connHandle, prf_read_cb_t readCb)
{
    (void)readCb;
    return blt_gtbsc_readAttrVal(connHandle, GTBS_READ_BEARER_URI_SCHEMES_SUPP_LIST);
}

int blc_gtbsc_readBearerSignalStrength(u16 connHandle, prf_read_cb_t readCb)
{
    (void)readCb;
    return blt_gtbsc_readAttrVal(connHandle, GTBS_READ_BEARER_SIGNAL_STRENGTH);
}

int blc_gtbsc_readBearerSignalStrengthRptItvl(u16 connHandle, prf_read_cb_t readCb)
{
    (void)readCb;
    return blt_gtbsc_readAttrVal(connHandle, GTBS_READ_BEARER_SIGNAL_STRENGTH_RPT_ITVL);
}

int blc_gtbsc_readBearerListCurrentCalls(u16 connHandle, prf_read_cb_t readCb)
{
    (void)readCb;
    return blt_gtbsc_readAttrVal(connHandle, GTBS_READ_BEARER_LIST_CURRENT_CALLS);
}

int blc_gtbsc_readContentControlID(u16 connHandle, prf_read_cb_t readCb)
{
    (void)readCb;
    return blt_gtbsc_readAttrVal(connHandle, GTBS_READ_CONTENT_CONTROL_ID);
}

int blc_gtbsc_readIncomingCallTargetBearerURI(u16 connHandle, prf_read_cb_t readCb)
{
    (void)readCb;
    return blt_gtbsc_readAttrVal(connHandle, GTBS_READ_INCOMING_CALL_TARGET_BEARER_URI);
}

int blc_gtbsc_readStatusFlags(u16 connHandle, prf_read_cb_t readCb)
{
    (void)readCb;
    return blt_gtbsc_readAttrVal(connHandle, GTBS_READ_STATUS_FLAGS);
}

int blc_gtbsc_readCallState(u16 connHandle, prf_read_cb_t readCb)
{
    (void)readCb;
    return blt_gtbsc_readAttrVal(connHandle, GTBS_READ_CALL_STATE);
}

int blc_gtbsc_readCallCtrlPntOptOpcodes(u16 connHandle, prf_read_cb_t readCb)
{
    (void)readCb;
    return blt_gtbsc_readAttrVal(connHandle, GTBS_READ_CALL_CONTROL_POINT_OPT_OPCODES);
}

int blc_gtbsc_readIncomingCall(u16 connHandle, prf_read_cb_t readCb)
{
    (void)readCb;
    return blt_gtbsc_readAttrVal(connHandle, GTBS_READ_INCOMING_CALL);
}

int blc_gtbsc_readCallFriendlyName(u16 connHandle, prf_read_cb_t readCb)
{
    (void)readCb;
    return blt_gtbsc_readAttrVal(connHandle, GTBS_READ_CALL_FRIENDLY_NAME);
}

/*************************************************************************
 *  GATTC Write Characteristics
 *  CHARACTERISTIC_UUID_GTBS_CONTROL_POINT,
 *    Call Control Point Procedures
 *      * Answer Incoming Call
 *      * Terminate Call
 *      * Move Call To Local Hold
 *      * Move Locally Held Call To Active Call
 *      * Move Locally And Remotely Held Call To Remotely Held Call
 *      * Originate Call
 *      * Join Calls
 *************************************************************************/
static void blt_gtbsc_writeCtrlPntCb(u16 connHandle, u8 err, void *data)
{
    assert(blt_ll_isAclhdlInvalid(connHandle) == BLE_SUCCESS);

    if (err) {
        BLT_TBS_LOG("WR_CB INFO: ERR: %x", err);
    } else {
        BLT_TBS_LOG("WR_CB INFO: SUCC");
    }

    (void)connHandle;
    (void)data;
}

static int blt_gtbsc_writeCtrlPnt(u16 connHandle, blt_gtbs_opcode_enum opcode, u8 *pVal, u16 valLen)
{
    BLT_TBS_LOG("blt_gtbsc_writeCtrlPnt:%d", opcode);
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        BLT_TBS_LOG("ERR: ACL handle invalid");
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);

    if (!client->callControlPointHdl) {
        BLT_TBS_LOG("ERR: handle not set");
        return AUDIO_ERR_INVALID_PARAMETER;
    } else if (opcode >= GTBS_OPCODE_MAX) {
        BLT_TBS_LOG("ERR: opcode[0x%x] invalid", opcode);
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    u16 mtu = blt_gap_getScidMtu(connHandle, L2CAP_CID_ATTR_PROTOCOL);

#if (0)
    u8 payload[ATT_MAX_MTU]; //TODO: normal stack need larger
#else                        /* GCC C99 */
    u8 payload[mtu];
#endif

    payload[0] = opcode;
    memcpy(&payload[1], pVal, valLen);

    gapc_write_cfg_t pGapWrCfg;
    pGapWrCfg.func       = blt_gtbsc_writeCtrlPntCb;
    pGapWrCfg.handle     = client->callControlPointHdl;
    pGapWrCfg.data       = payload;
    pGapWrCfg.length     = valLen + 1;
    pGapWrCfg.withoutRsp = true;
    pGapWrCfg.cbData     = NULL;
    return blc_gapc_writeAttributeValue(connHandle, &pGapWrCfg);
}

/*
 * It is used by a Call Control Client to answer an incoming call and make the call active on a Call Control Server.
 */
int blc_gtbsc_writeAcceptIncomingCall(u16 connHandle, u8 callIndex)
{
    return blt_gtbsc_writeCtrlPnt(connHandle, GTBS_OPCODE_ACCEPT, &callIndex, 1);
}

/*
 * It is used by a Call Control Client to end a call that is in any state on a Call Control Server.
 */
int blc_gtbsc_writeTerminateCall(u16 connHandle, u8 callIndex)
{
    //blc_gtbs_client_t* client = blt_gtbsc_getClientInst(connHandle);
    return blt_gtbsc_writeCtrlPnt(connHandle, GTBS_OPCODE_TERMINATE, &callIndex, 1);
}

/*
 * It is used by a Call Control Client to place an active call or an alerting call on local hold on a Call Control Server
 */
int blc_gtbsc_writeLocalHoldActiveOrImcomingCall(u16 connHandle, u8 callIndex)
{
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    if (client == NULL) {
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    if (!(client->callCtrlPointOptionalOp & GTBS_CCP_OPT_OPCODE_SUPP_LOCAL_HOLD_AND_RETRIEVE)) {
        return LE_AUDIO_SERVER_INVALID_SERVICE;
    }

    return blt_gtbsc_writeCtrlPnt(connHandle, GTBS_OPCODE_LOCAL_HOLD, &callIndex, 1);
}

/*
 * It is used by a Call Control Client to move a call that is being locally held to an active call on a Call Control Server.
 * It is used by a Call Control Client to move a call that is being locally and remotely held to a remotely held call on a Call Control Server.
 */
int blc_gtbsc_writeLocalRetrieve(u16 connHandle, u8 callIndex)
{
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    if (client == NULL) {
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    if (!(client->callCtrlPointOptionalOp & GTBS_CCP_OPT_OPCODE_SUPP_LOCAL_HOLD_AND_RETRIEVE)) {
        return LE_AUDIO_SERVER_INVALID_SERVICE;
    }

    return blt_gtbsc_writeCtrlPnt(connHandle, GTBS_OPCODE_LOCAL_RETRIEVE, &callIndex, 1);
}

/*
 * It is used by a Call Control Client to make an outgoing call on a Call Control Server.
 */
int blc_gtbsc_writeOriginate(u16 connHandle, u8 *pUriofOutgoingCall, u8 uriLen)
{
    return blt_gtbsc_writeCtrlPnt(connHandle, GTBS_OPCODE_ORIGINATE, pUriofOutgoingCall, uriLen);
}

/*
 * It is used by a Call Control Client to join multiple calls on the Call Control Server.
 */
int blc_gtbsc_writeJoinCallList(u16 connHandle, u8 *pCallList, u8 callListLen)
{
    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    if (client == NULL) {
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    if (!(client->callCtrlPointOptionalOp & GTBS_CCP_OPT_OPCODE_SUPP_JION_CALL)) {
        return LE_AUDIO_SERVER_INVALID_SERVICE;
    }

    return blt_gtbsc_writeCtrlPnt(connHandle, GTBS_OPCODE_JOIN, pCallList, callListLen);
}

int blc_gtbsc_getBearerProviderName(u16 connHandle, u8 *pOutPrName, u16 *OutPrNameLen)
{
    if (pOutPrName == NULL || OutPrNameLen == NULL) {
        return AUDIO_EPARAM;
    }
    if (blt_ll_isAclHandleOutOfRange(connHandle)) {
        return AUDIO_EPARAM;
    }

    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    if (client == NULL) {
        return AUDIO_EPARAM;
    }

    memcpy(pOutPrName, client->providerName, client->providerNameLen);
    *OutPrNameLen = client->providerNameLen;

    return AUDIO_ESUCC;
}

int blc_gtbsc_getBearerUCI(u16 connHandle, u8 *pOutUCI, u16 *outUCILen)
{
    if (pOutUCI == NULL || outUCILen == NULL) {
        return AUDIO_EPARAM;
    }
    if (blt_ll_isAclHandleOutOfRange(connHandle)) {
        return AUDIO_EPARAM;
    }

    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    if (client == NULL) {
        return AUDIO_EPARAM;
    }

    memcpy(pOutUCI, client->UCI, client->UCILen);
    *outUCILen = client->UCILen;

    return AUDIO_ESUCC;
}

int blc_gtbsc_getBearerTechnology(u16 connHandle, u8 outTechnology[1])
{
    if (outTechnology == NULL) {
        return AUDIO_EPARAM;
    }
    if (blt_ll_isAclHandleOutOfRange(connHandle)) {
        return AUDIO_EPARAM;
    }

    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    if (client == NULL) {
        return AUDIO_EPARAM;
    }

    *outTechnology = client->technology;

    return AUDIO_ESUCC;
}

int blc_gtbsc_getBearerURISchemesSuppList(u16 connHandle, u8 *outURISchemesSuppList, u16 *outURISchemesSuppListLen)
{
    (void)connHandle;
    (void)outURISchemesSuppList;
    (void)outURISchemesSuppListLen;
    return AUDIO_ESUCC;
}

int blc_gtbsc_getBearerSignalStrength(u16 connHandle, u8 outSignalStrength[1])
{
    if (outSignalStrength == NULL) {
        return AUDIO_EPARAM;
    }
    if (blt_ll_isAclHandleOutOfRange(connHandle)) {
        return AUDIO_EPARAM;
    }

    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    if (client == NULL) {
        return AUDIO_EPARAM;
    }

    *outSignalStrength = client->signalStrength;

    return AUDIO_ESUCC;
}

int blc_gtbsc_getBearerSignalStrengthItvl(u16 connHandle, u8 outSignalStrengthItvl[1])
{
    if (outSignalStrengthItvl == NULL) {
        return AUDIO_EPARAM;
    }
    if (blt_ll_isAclHandleOutOfRange(connHandle)) {
        return AUDIO_EPARAM;
    }

    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    if (client == NULL) {
        return AUDIO_EPARAM;
    }

    *outSignalStrengthItvl = client->signalStrengthReportingInterval;

    return AUDIO_ESUCC;
}

int blc_gtbsc_getCCID(u16 connHandle, u8 outCCID[1])
{
    if (outCCID == NULL) {
        return AUDIO_EPARAM;
    }
    if (blt_ll_isAclHandleOutOfRange(connHandle)) {
        return AUDIO_EPARAM;
    }

    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    if (client == NULL) {
        return AUDIO_EPARAM;
    }

    *outCCID = client->ccid;

    return AUDIO_ESUCC;
}

int blc_gtbsc_getStatusFlags(u16 connHandle, blc_tbs_status_flags_t *statusFlags)
{
    (void)connHandle;
    (void)statusFlags;
    return AUDIO_ESUCC;
}

int blc_gtbsc_getIncomingCallTargetBearerURI(u16 connHandle, blc_tbs_incoming_call_target_bearer_uri_t *uri, u16 *uriLen)
{
    (void)connHandle;
    (void)uri;
    (void)uriLen;
    return AUDIO_ESUCC;
}

int blc_gtbsc_getCallState(u16 connHandle, u16 *callMembersCnt, blc_gtbs_call_state_t *callState)
{
    if (callMembersCnt == NULL || callState == NULL) {
        return AUDIO_EPARAM;
    }
    if (blt_ll_isAclHandleOutOfRange(connHandle)) {
        return AUDIO_EPARAM;
    }

    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    if (client == NULL) {
        return AUDIO_EPARAM;
    }

    *callMembersCnt = client->callStateLen / sizeof(blc_gtbs_call_state_t);
    memcpy(callState, (u8 *)&client->callState[0], sizeof(blc_gtbs_call_state_t) * (*callMembersCnt));

    return AUDIO_ESUCC;
}

int blc_gtbsc_getCcpOptionalOp(u16 connHandle, u16 outCcpOptionalOp[1])
{
    if (outCcpOptionalOp == NULL) {
        return AUDIO_EPARAM;
    }
    if (blt_ll_isAclHandleOutOfRange(connHandle)) {
        return AUDIO_EPARAM;
    }

    blc_gtbs_client_t *client = blt_gtbsc_getClientInst(connHandle);
    if (client == NULL) {
        return AUDIO_EPARAM;
    }

    *outCcpOptionalOp = client->callCtrlPointOptionalOp;

    return AUDIO_ESUCC;
}

int blc_gtbsc_getIncomingCall(u16 connHandle, blc_tbs_incoming_call_t *incomingCall, u16 *incomingCallLen)
{
    (void)connHandle;
    (void)incomingCall;
    (void)incomingCallLen;
    return AUDIO_ESUCC;
}

int blc_gtbsc_getCallFriendlyName(u16 connHandle, blc_tbs_call_friendly_name_t *callFriendlyName, u16 *callFriendlyNameLen)
{
    (void)connHandle;
    (void)callFriendlyName;
    (void)callFriendlyNameLen;
    return AUDIO_ESUCC;
}

const char *blc_gtbs_getBearerTechnologyName(blc_gtbs_technology_enum tech)
{
    switch (tech) {
    case GTBS_TECHNOLOGY_3G:
        return "'3G'";
    case GTBS_TECHNOLOGY_4G:
        return "'4G'";
    case GTBS_TECHNOLOGY_LTE:
        return "'LTE'";
    case GTBS_TECHNOLOGY_WIFI:
        return "'WiFi'";
    case GTBS_TECHNOLOGY_5G:
        return "'5G'";
    case GTBS_TECHNOLOGY_GSM:
        return "'GSM'";
    case GTBS_TECHNOLOGY_CDMA:
        return "'CDMA'";
    case GTBS_TECHNOLOGY_2G:
        return "'2G'";
    case GTBS_TECHNOLOGY_WCDMA:
        return "'WCDMA'";
    case GTBS_TECHNOLOGY_IP:
        return "'IP'";
    default:
        return "'Reserved'";
    }
}

const char *blc_gtbs_getStatusFlagsDescription(u16 statusFlags)
{
    /*
     * 00: "inband ringtone disabled/ Server is not in silent mode"
     * 01: "inband ringtone enabled/ Server is not in silent mode"
     * 10: "inband ringtone disabled/ Server is in silent mode"
     * 11: "inband ringtone enabled/ Server is in silent mode"
     */
    switch (statusFlags & 0x11) {
    case 0b00:
        return "'inband ringtone disabled/ Server is not in silent mode'";
    case 0b01:
        return "'inband ringtone enabled/ Server is not in silent mode'";
    case 0b10:
        return "'inband ringtone disabled/ Server is in silent mode'";
    case 0b11:
        return "'inband ringtone enabled/ Server is in silent mode'";
    default:
        return "'unknown status flags'";
    }
}

const char *blc_gtbs_getCallStateName(blc_gtbs_callState_enum state)
{
    switch (state) {
    case GTBS_CALL_STATE_INCOMING:
        return "'incoming'";
    case GTBS_CALL_STATE_DIALING:
        return "'dialing'";
    case GTBS_CALL_STATE_ALERTING:
        return "'alerting'";
    case GTBS_CALL_STATE_ACTIVE:
        return "'active'";
    case GTBS_CALL_STATE_LOCALLY_HELD:
        return "'locally held'";
    case GTBS_CALL_STATE_REMOTELY_HELD:
        return "'remote held'";
    case GTBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD:
        return "'locally and remotely held'";
    default:
        return "'RFU'";
    }
}

const char *blc_gtbs_getCallFlagsDescription(u8 callFlags)
{
    /*
     * 000: "Call is an incoming call/Not withheld/Provided by network"
     * 001: "Call is an outgoing call/Not withheld/Provided by network"
     * 010: "Call is an incoming call/Withheld/Provided by network"
     * 011: "Call is an outgoing call/Withheld/Provided by network"
     * 100: "Call is an incoming call/Not withheld/Withheld by network"
     * 101: "Call is an outgoing call/Not withheld/Withheld by network"
     * 110: "Call is an incoming call/Withheld/Withheld by network"
     * 111: "Call is an outgoing call/Withheld/Withheld by network"
     */
    switch (callFlags) {
    case 0b000:
        return "'Call is an incoming call/ Not withheld/ Provided by network'";
    case 0b001:
        return "'Call is an outgoing call/ Not withheld/ Provided by network'";
    case 0b010:
        return "Call is an incoming call/ Withheld/ Provided by network'";
    case 0b011:
        return "'Call is an outgoing call/ Withheld/ Provided by network'";
    case 0b100:
        return "'Call is an incoming call/ Not withheld/ Withheld by network'";
    case 0b101:
        return "'Call is an outgoing call/ Not withheld/ Withheld by network'";
    case 0b110:
        return "'Call is an incoming call/ Withheld/ Withheld by network'";
    case 0b111:
        return "'Call is an outgoing call/ Withheld/ Withheld by network'";
    default:
        return "'unknown call flags'";
    }
}

const char *blc_gtbs_getCallControlPointOpcodeName(blt_gtbs_opcode_enum opcode)
{
    switch (opcode) {
    case GTBS_OPCODE_ACCEPT:
        return "'accept'";
    case GTBS_OPCODE_TERMINATE:
        return "'terminate'";
    case GTBS_OPCODE_LOCAL_HOLD:
        return "'local hold'";
    case GTBS_OPCODE_LOCAL_RETRIEVE:
        return "'local retrieve'";
    case GTBS_OPCODE_ORIGINATE:
        return "'originate'";
    case GTBS_OPCODE_JOIN:
        return "'join'";
    default:
        return "'unknown opcode'";
    }
}

const char *blt_ccp_ntfStatusFlagsStr(blc_gtbs_ntfResultCode_enum status)
{
    switch (status) {
    case GTBS_NTF_RESULT_CODE_SUCCESS:
        return "'success'";
    case GTBS_NTF_RESULT_CODE_OPCODE_NOT_SUPP:
        return "'opcode not supported'";
    case GTBS_NTF_RESULT_CODE_OPERATION_NOT_POSSIBLE:
        return "'operation not possible'";
    case GTBS_NTF_RESULT_CODE_INVALID_CALL_INDEX:
        return "'invalid call index'";
    case GTBS_NTF_RESULT_CODE_STATE_MISMATCH:
        return "'state mismatch'";
    case GTBS_NTF_RESULT_CODE_LACK_OF_RESOURCES:
        return "'out of resources'";
    case GTBS_NTF_RESULT_CODE_INVALID_OUTGOING_URI:
        return "'invalid URI'";
    default:
        return "'ATT err'";
    }
}

const char *blt_gtbs_getTerminationReasonName(blc_gtbs_termReason_enum status)
{
    switch (status) {
    case GTBS_TERM_REASON_URI_ERROR:
        return "'uri error'";
    case GTBS_TERM_REASON_CALL_FAILED:
        return "'call failed'";
    case GTBS_TERM_REASON_REMOTE_ENDED_CALL:
        return "'remote party ended the call'";
    case GTBS_TERM_REASON_SERVER_ENDED_CALL:
        return "'call ended from the server'";
    case GTBS_TERM_REASON_LINE_BUSY:
        return "'line was busy'";
    case GTBS_TERM_REASON_NETWORK_CONGESTION:
        return "'network congestion'";
    case GTBS_TERM_REASON_CLIENT_TERM_CALL:
        return "'client terminated the call'";
    case GTBS_TERM_REASON_NO_SERVICE:
        return "'no service'";
    case GTBS_TERM_REASON_NO_ANSWER:
        return "'no answer'";
    case GTBS_TERM_REASON_UNSPECIFIED:
        return "'unspecified'";
    default:
        return "'TERM err'";
    }
}
