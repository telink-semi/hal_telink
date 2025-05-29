/********************************************************************************************************
 * @file    unicast_server.c
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

#include "bap_internal.h"

void blt_audio_unicastSvrProcCisConn(u16 cisHandle)
{
    int connHandle = blt_audio_getAclHdlByCisHdl(cisHandle);
    if (connHandle < 0) {
        return;
    }
    blc_ascs_server_t *ascss = blt_ascss_getCtrl((u16)connHandle);
    for (u8 i = 0; i < ascss->aseCnt; i++) {
        if (ascss->aseState[i]->cisHandle == cisHandle) {
            ascss->aseState[i]->cisEstablish = true;
            BLT_BAP_LOG("Unicast Server: CIS established-ASE: %d", ascss->aseState[i]->aseID);
            if (!ascss->aseState[i]->dataPathSetup) {
                blt_audio_unicastDataPathSetup(ascss->aseState[i], 0, 0);
            }
        }
    }
}

void blt_audio_unicastSvrProcCisDisConn(u16 cisHandle)
{
    int aclHandle = blt_audio_getAclHdlByCisHdl(cisHandle);
    if (aclHandle < 0) {
        return;
    }
    blc_ascs_server_t *ascss = blt_ascss_getCtrl(aclHandle);
    for (u8 i = 0; i < ascss->aseCnt; i++) {
        if (ascss->aseState[i]->cisHandle == cisHandle) {
            if (ascss->aseState[i]->state == BLT_ASCS_ASE_STATE_QOS_CFG) {
                ascss->aseState[i]->cisHandle    = 0;
                ascss->aseState[i]->cisEstablish = 0;
            } else {
                ascss->aseState[i]->state         = BLT_ASCS_ASE_STATE_IDLE;
                ascss->aseState[i]->cisHandle     = 0;
                ascss->aseState[i]->cisEstablish  = 0;
                ascss->aseState[i]->dataPathSetup = 0;
                ascss->aseState[i]->notifFlag     = 0x01;
                blt_ascss_ntfAllAseState(aclHandle);
            }
        }
    }
    BLT_BAP_LOG("Unicast Server: CIS Disconn:0x%x", aclHandle);
}

void blt_audio_unicastSvrProcAclDisConn(u16 aclHandle)
{
    blc_ascs_server_t *ascss = blt_ascss_getCtrl(aclHandle);
    for (u8 i = 0; i < ascss->aseCnt; i++) {
        ascss->aseState[i]->state         = BLT_ASCS_ASE_STATE_IDLE;
        ascss->aseState[i]->cisHandle     = 0;
        ascss->aseState[i]->cisEstablish  = 0;
        ascss->aseState[i]->dataPathSetup = 0;
    }
    BLT_BAP_LOG("Unicast Server: ACL Disconn:0x%x", aclHandle);
}

void blt_audio_unicastSvrProcCisReq(u16 aclHandle, u16 cisHandle, u8 cigId, u8 cisId)
{
    blc_ascs_server_t *ascss        = blt_ascss_getCtrl(aclHandle);
    u8                 cisCreatFlag = 0;
    for (u8 i = 0; i < ascss->aseCnt; i++) {
        if (ascss->aseState[i]->QosState.cigID == cigId && ascss->aseState[i]->QosState.cisID == cisId) {
            if (ascss->aseState[i]->state == BLT_ASCS_ASE_STATE_QOS_CFG || ascss->aseState[i]->state == BLT_ASCS_ASE_STATE_ENABLING) {
                cisCreatFlag                  = 1;
                ascss->aseState[i]->cisHandle = cisHandle;
            }
        }
    }
    if (cisCreatFlag) {
        if (blc_ll_acceptCisRequest(cisHandle) == BLE_SUCCESS) {
            BLT_BAP_LOG("Unicast Server:accept cis req");
        } else {
            BLT_BAP_LOG("Unicast Server:cmd-accept cis req fail: 0x%x");
        }
    }
}

static void blt_bap_leStackEvtForUnicastSvr(u32 h, u8 *p, int len)
{
    (void)len;
    if (h & HCI_FLAG_EVENT_BT_STD) //Controller HCI event
    {
        u8 evtCode     = h & 0xff;
        u8 subEvt_code = p[0];
        //------------ HCI event: LE CIS disconnect -------------------------------------
        if (evtCode == HCI_EVT_DISCONNECTION_COMPLETE) { //connection terminate
            hci_disconnectionCompleteEvt_t *pDisConn   = (hci_disconnectionCompleteEvt_t *)p;
            u16                             connHandle = pDisConn->connHandle;
            if (connHandle & BLT_CIS_HANDLE) {
                BLT_BAP_LOG("Unicast Server: CIS disconnect: 0x%x", connHandle);
                /* app event callback */
                blt_audio_sendCisDisconnEvt(pDisConn);
                /* profile layer process cis disconnect event. */
                blt_audio_unicastSvrProcCisDisConn(pDisConn->connHandle);
            } else if (connHandle & BLS_CONN_HANDLE) {
                blt_audio_unicastSvrProcAclDisConn(pDisConn->connHandle);
            }
        } else if (evtCode == HCI_EVT_LE_META) {
            //------HCI LE event: LE CIS established event -------------------------------
            if (subEvt_code == HCI_SUB_EVT_LE_CIS_ESTABLISHED) {
                hci_le_cisEstablishedEvt_t *pCisEstbEvt = (hci_le_cisEstablishedEvt_t *)p;
                BLT_BAP_LOG("Unicast Server: CIS established: 0x%x", pCisEstbEvt->cisHandle);
                /* app event callback */
                if (pCisEstbEvt->status == BLE_SUCCESS) {
                    /* profile layer process cis established event. */
                    blt_audio_unicastSvrProcCisConn(pCisEstbEvt->cisHandle);

                    blt_audio_sendCisConnEvt(pCisEstbEvt);
                } else //connect fail,treat it as a cis-disconnect.
                {
                }
            }
            //------HCI LE event: LE CIS Request event -------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_CIS_REQUEST) {
                BLT_ASCS_LOG("Unicast Server: CIS request");
                hci_le_cisReqEvt_t *pCisReqEvt = (hci_le_cisReqEvt_t *)p;
                /* app event callback */
                blt_audio_sendCisReqEvt(pCisReqEvt);
                /* profile layer process cis request event.*/
                blt_audio_unicastSvrProcCisReq(pCisReqEvt->aclHandle, pCisReqEvt->cisHandle, pCisReqEvt->cigId, pCisReqEvt->cisId);
            }
        }
    }
}

void blc_audio_registerBapUnicastServer(const blc_bapus_regParam_t *param)
{
    if (param == NULL) { //use default parameters
        blc_audio_registerASCSControlServer(NULL);
        blc_audio_registerPACSControlServer(&defaultPacsParam);
    } else {
        blc_audio_registerASCSControlServer(param->pAscsParam);
        blc_audio_registerPACSControlServer(param->pPacsParam);
    }

    /* LE stack event callback for BAP Unicast Server role */
    bap_unicast_svr_cb = blt_bap_leStackEvtForUnicastSvr;
}

void blt_audio_unicastSvrCodecCfgEvt(u16 connHandle, blt_ascss_ase_state_t *pAse, blc_audio_codecSpecCfgParsed_t *pCodecCfg)
{
    blc_bapus_codecConfiguredEvt_t configureCodecEvt;

    configureCodecEvt.dir       = pAse->dir;
    configureCodecEvt.audioEpId = pAse->aseID;
    memcpy((u8 *)&configureCodecEvt.codecid, (u8 *)&pAse->codecState.codecId, sizeof(blc_audio_codec_id_t));
    configureCodecEvt.duration           = pCodecCfg->duration;
    configureCodecEvt.frequency          = pCodecCfg->frequency;
    configureCodecEvt.frameOcts          = pCodecCfg->frameOcts;
    configureCodecEvt.location           = pCodecCfg->allocation;
    configureCodecEvt.codecFrmBlksPerSDU = pCodecCfg->codecFrameBlksPerSDU;
    blt_prf_sendEvent(connHandle, AUDIO_EVT_BAPUS_CODEC_CONFIGURED, (u8 *)&configureCodecEvt, sizeof(blc_bapus_codecConfiguredEvt_t));
}

void blt_audio_unicastSvrQosCfgEvt(u16 connHandle, blt_ascss_ase_state_t *pAse)
{
    blc_bapus_qosConfiguredEvt_t configureQosEvt;

    configureQosEvt.dir             = pAse->dir;
    configureQosEvt.audioEpId       = pAse->aseID;
    configureQosEvt.cigID           = pAse->QosState.cigID;
    configureQosEvt.cisID           = pAse->QosState.cisID;
    configureQosEvt.framing         = pAse->QosState.framing;
    configureQosEvt.retransNum      = pAse->QosState.retranNum;
    configureQosEvt.maxSdu          = pAse->QosState.maxSdu;
    configureQosEvt.maxTransLatency = pAse->QosState.maxTranLatency;
    BYTE_TO_UINT24(configureQosEvt.sduInterval, pAse->QosState.sduInterval);
    BYTE_TO_UINT24(configureQosEvt.presentationDelay, pAse->QosState.presentationDelay);
    blt_prf_sendEvent(connHandle, AUDIO_EVT_BAPUS_QOS_CONFIGURED, (u8 *)&configureQosEvt, sizeof(blc_bapus_qosConfiguredEvt_t));
}

void blt_audio_unicastSvrEnablingEvt(u16 connHandle, blt_ascss_ase_state_t *pAse)
{
    blc_bapus_enablingEvt_t enableEvt;
    enableEvt.audioEpId = pAse->aseID;
    enableEvt.dir       = pAse->dir;
    enableEvt.metaLen   = pAse->otherState.metadataLen;
    memcpy(enableEvt.meta, pAse->otherState.metadata, enableEvt.metaLen);
    blt_prf_sendEvent(connHandle, AUDIO_EVT_BAPUS_ENABLING, (u8 *)&enableEvt, sizeof(blc_bapus_enablingEvt_t));
}

void blt_audio_unicastSvrUpdateEvt(u16 connHandle, blt_ascss_ase_state_t *pAse)
{
    blc_bapus_updateMetadataEvt_t updateEvt;
    updateEvt.audioEpId = pAse->aseID;
    updateEvt.dir       = pAse->dir;
    updateEvt.metaLen   = pAse->otherState.metadataLen;
    memcpy(updateEvt.meta, pAse->otherState.metadata, updateEvt.metaLen);
    blt_prf_sendEvent(connHandle, AUDIO_EVT_BAPUS_UPDATE_METADATA, (u8 *)&updateEvt, sizeof(blc_bapus_updateMetadataEvt_t));
}

void blt_audio_unicastSvrDisablingEvt(u16 connHandle, blt_ascss_ase_state_t *pAse)
{
    blc_bapus_disablingEvt_t disableEvt;
    disableEvt.audioEpId = pAse->aseID;
    disableEvt.dir       = pAse->dir;
    blt_prf_sendEvent(connHandle, AUDIO_EVT_BAPUS_DISABLING, (u8 *)&disableEvt, sizeof(blc_bapus_disablingEvt_t));
}

void blt_audio_unicastSvrReleasingEvt(u16 connHandle, blt_ascss_ase_state_t *pAse)
{
    blc_bapus_releasingEvt_t releaseEvt;
    releaseEvt.audioEpId = pAse->aseID;
    releaseEvt.dir       = pAse->dir;
    blt_prf_sendEvent(connHandle, AUDIO_EVT_BAPUS_RELEASING, (u8 *)&releaseEvt, sizeof(blc_bapus_releasingEvt_t));
}

void blt_audio_unicastSvrSendStreamEvt(u16 connHandle, blt_ascss_ase_state_t *pAse)
{
    blc_bapus_sendStreamingEvt_t sourceStartEvt;
    sourceStartEvt.audioEpId = pAse->aseID;
    sourceStartEvt.dir       = pAse->dir;
    blt_prf_sendEvent(connHandle, AUDIO_EVT_BAPUS_SEND_STREAMING, (u8 *)&sourceStartEvt, sizeof(blc_bapus_sendStreamingEvt_t));
}

void blt_audio_unicastSvrRcvStreamEvt(u16 connHandle, blt_ascss_ase_state_t *pAse)
{
    blc_bapus_receiveStreamingEvt_t sinkStartEvt;
    sinkStartEvt.audioEpId = pAse->aseID;
    sinkStartEvt.dir       = pAse->dir;
    blt_prf_sendEvent(connHandle, AUDIO_EVT_BAPUS_RECEIVE_STREAMING, (u8 *)&sinkStartEvt, sizeof(blc_bapus_receiveStreamingEvt_t));
}

int blc_bapus_aseConfigCodec(u16 aclHandle, u8 epId)
{
    (void)aclHandle;
    (void)epId;
    return AUDIO_ESUCC;
}

int blc_bapus_aseReceiverStartReady(u16 aclHandle, u8 epId)
{
    blc_ascs_server_t *ascss = blt_ascss_getCtrl(aclHandle);
    for (u8 i = 0; i < ascss->aseCnt; i++) {
        if (epId == ascss->aseState[i]->aseID) {
            if (ascss->aseState[i]->state != BLT_ASCS_ASE_STATE_ENABLING) {
                return AUDIO_ESTATUS;
            }
            if (ascss->aseState[i]->dir != AUDIO_DIR_SINK) {
                return AUDIO_EDIR;
            }
            if (!(ascss->aseState[i]->cisEstablish && ascss->aseState[i]->dataPathSetup)) {
                return AUDIO_ENOREADY;
            }
            ascss->aseState[i]->state     = BLT_ASCS_ASE_STATE_STREAMING;
            ascss->aseState[i]->notifFlag = 0x01;
            blt_ascss_ntfAllAseState(aclHandle);
            return AUDIO_ESUCC;
        }
    }
    return AUDIO_EMPTY;
}

int blc_bapus_aseReleasedByCache(u16 aclHandle, u8 epId, bool cache)
{
    blc_ascs_server_t *ascss = blt_ascss_getCtrl(aclHandle);
    for (u8 i = 0; i < ascss->aseCnt; i++) {
        if (epId == ascss->aseState[i]->aseID) {
            if (cache) {
                ascss->aseState[i]->state = BLT_ASCS_ASE_STATE_CODEC_CFG;
            } else {
                ascss->aseState[i]->state = BLT_ASCS_ASE_STATE_IDLE;
            }
            ascss->aseState[i]->notifFlag = 0x01;
            blt_ascss_ntfAllAseState(aclHandle);
            return AUDIO_ESUCC;
        }
    }
    return AUDIO_EASEID;
}

int blc_bapus_aseRelease(u16 aclHandle, u8 epId)
{
    blc_ascs_server_t *ascss = blt_ascss_getCtrl(aclHandle);
    for (u8 i = 0; i < ascss->aseCnt; i++) {
        if (epId == ascss->aseState[i]->aseID) {
            ascss->aseState[i]->state     = BLT_ASCS_ASE_STATE_RELEASING;
            ascss->aseState[i]->notifFlag = 0x01;
            blt_ascss_ntfAllAseState(aclHandle);
            return AUDIO_ESUCC;
        }
    }
    return AUDIO_EASEID;
}

int blc_bapus_aseDisable(u16 aclHandle, u8 epId)
{
    (void)aclHandle;
    (void)epId;
    return AUDIO_ESUCC;
}

int blc_bapus_aseUpdateMetadata(u16 aclHandle, u8 epId, u8 meta[], u8 metaLen)
{
    (void)aclHandle;
    (void)epId;
    (void)meta;
    (void)metaLen;
    return AUDIO_ESUCC;
}

int blc_bapus_sduPacketPush(u16 aclHandle, u8 epId, u8 *pData, u16 len)
{
    blc_ascs_server_t *ascss = blt_ascss_getCtrl(aclHandle);
    for (u8 i = 0; i < ascss->aseCnt; i++) {
        if (epId == ascss->aseState[i]->aseID) {
            if (ascss->aseState[i]->state != BLT_ASCS_ASE_STATE_STREAMING) {
                return AUDIO_ESTATUS;
            }
            u32 sendRet = blc_iso_sendData(ascss->aseState[i]->cisHandle, pData, len);
            if (sendRet == BLE_SUCCESS) {
                return AUDIO_ESUCC;
            } else {
                BLT_BAP_LOG("cis handle: 0x%x", ascss->aseState[i]->cisHandle);
                return sendRet;
            }
        }
    }
    return AUDIO_EMPTY;
}

sdu_packet_t *blc_bapus_sduPacketPop(u16 aclHandle, u8 epId)
{
    blc_ascs_server_t *ascss = blt_ascss_getCtrl(aclHandle);
    for (u8 i = 0; i < ascss->aseCnt; i++) {
        if (epId == ascss->aseState[i]->aseID) {
            if (ascss->aseState[i]->state != BLT_ASCS_ASE_STATE_STREAMING) {
                return NULL;
            }
            sdu_packet_t *pPkt = blc_ll_popCisRxSduData(ascss->aseState[i]->cisHandle);
            return pPkt;
        }
    }
    return NULL;
}
