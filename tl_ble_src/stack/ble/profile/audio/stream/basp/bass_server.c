/********************************************************************************************************
 * @file    bass_server.c
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
#include "../bap_internal.h"

#define DEFAULT_PAST_TIMER 1000

static int  blt_basss_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8 *writeValue, u16 valueLen);
static void blt_basss_serviceInit(blc_bass_server_t *server);
static u8  *blt_basss_findBcstRecvStateBySrcID(u16 connHandle, u8 sourceID, u16 **len);

_attribute_ble_data_retention_
    blc_bass_server_ctrl_t bass_server_ctrl = {
        .process = {
                    .pNext       = NULL,
                    .id          = AUDIO_BASS_SERVER,
                    .usedAclRole = 0,
                    .init        = blt_basss_init,
                    .connect     = blt_basss_connect,
                    .discov      = NULL,
                    .loop        = blt_basss_loop,
                    },
};

void blc_audio_registerBASSControServer(const blc_basss_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_PERIPHERAL, (blc_prf_proc_t *)&bass_server_ctrl, param);
}

static blc_bass_server_t *blt_basss_getCtrl(u16 connHandle)
{
#if (0)
    int ret = blt_prf_getAclRole(connHandle);
    if (ret < 0 || ret == ACL_ROLE_CENTRAL) {
        BLT_BASS_LOG("ERR: ACL role, unlikely: 0x%x", ret);

        if (ret >= 0) {
            blt_prf_sendSvrGapRoleErrEvt(connHandle, AUDIO_BASS_SERVER, ret);
        }

        return NULL;
    }
#else
    (void)connHandle;
#endif

    return &bass_server_ctrl.bassServer;
}

int blt_basss_init(u8 initType, const void *param)
{
#if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_bass_server_t)), blc_bass_server_t);
#endif

    if (initType == PRF_PROC_INIT) {
        BLT_BASS_LOG("Server init");
        blc_svc_addBassGroup();
        blc_svc_bassCbackRegister(NULL, blt_basss_writeCback);
        blc_bass_server_t *basss = blt_basss_getCtrl(0xFFFF);
        if (param) {
            const blc_basss_regParam_t *regParam = (const blc_basss_regParam_t *)param;
            basss->pastTimer                     = regParam->pastTimer;
        } else {
            basss->pastTimer = DEFAULT_PAST_TIMER;
        }

        blt_basss_serviceInit(basss);
    }
    //  else if (initType == PRF_PROC_DEINIT) {
    //      blc_svc_removeBassGroup();
    //      BLT_BASS_LOG("blt_basss_deinit");
    //  }
    return 0;
}

int blt_basss_connect(u16 connHandle, prf_acl_state_enum connState)
{
    if (connState == PRF_ACL_STATE_DISCONN) {
        BLT_BASS_LOG("Disconnect:0x%x", connHandle);

        blc_bass_server_t *basss = blt_basss_getCtrl(connHandle);
        if (basss->pastConnHandle == connHandle) {
            blt_audio_broadcastSinkRecvEvt(basss->pastConnHandle, AUDIO_EVT_BASSS_NO_PAST, NULL, 0);
            blt_basss_pastFinish(connHandle);
            basss->sourceId = 0;
        }
    } else {
        BLT_BASS_LOG("Connect:0x%x", connHandle);

        /* connected mark */
    }
    return 0;
}

int blt_basss_loop(u16 connHandle)
{
    blc_bass_server_t *basss = blt_basss_getCtrl(connHandle);
    if (basss->pastStartTimer && clock_time_exceed(basss->pastStartTimer, basss->pastTimer * 1000)) {
        blt_audio_broadcastSinkRecvEvt(basss->pastConnHandle, AUDIO_EVT_BASSS_NO_PAST, NULL, 0);
        blt_basss_pastFinish(connHandle);

        blt_bass_recvState_t *pRcvState = (blt_bass_recvState_t *)blt_basss_findBcstRecvStateBySrcID(basss->pastConnHandle, basss->sourceId, NULL);

        if (!pRcvState) {
            return false;
        }

        if (pRcvState->paSyncState == BASS_PA_STATE_SYNCINFO_REQUEST) {
            pRcvState->paSyncState = BASS_PA_STATE_NO_PAST;
            blc_basss_notifyRecvState(basss->pastConnHandle, basss->sourceId);
        }
    }
    return 0;
}

void blt_basss_pastFinish(u16 connHandle)
{
    blc_bass_server_t *basss = blt_basss_getCtrl(connHandle);

    basss->pastStartTimer = 0;
    basss->pastConnHandle = 0;
}

#define BASSS_RECV_STATE_HANDLE(connHandle, index) blt_basss_getCtrl(connHandle)->recvStateHandle[index]

int blc_basss_notifyRecvState(u16 connHandle, u8 sourceID)
{
    return blc_gatts_notifyAttr(connHandle, BASSS_RECV_STATE_HANDLE(connHandle, sourceID - 1));
}

static int blt_basss_checkWriteParam(u8 *pParam, u8 paramLen)
{
    u8  i;
    u8  opcode = pParam[0];
    int err    = ATT_SUCCESS;

    if ((opcode == BASS_OPCODE_REMOTE_SCAN_STOPPED) || (opcode == BASS_OPCODE_REMOTE_SCAN_STARTED)) {
        err = paramLen == 1 ? ATT_SUCCESS : ATT_ERR_WRITE_REQUEST_REJECT;
    } else if (opcode == BASS_OPCODE_ADD_SOURCE) {
        blt_bass_AddSrc_t *pAddSrc = (blt_bass_AddSrc_t *)pParam;

        if (paramLen < sizeof(blt_bass_AddSrc_t)) {
            return ATT_ERR_WRITE_REQUEST_REJECT;
        }

        paramLen -= sizeof(blt_bass_AddSrc_t);

        u8  bisSyncNoPreferenceCnt = 0x00;
        u8 *ptr                    = (u8 *)&pAddSrc->parameter.subGrps[0];

        for (i = 0; i < pAddSrc->parameter.numSubGrps; i++) {
            bass_subGrp_t *pSubGrp = (bass_subGrp_t *)(ptr);
            if (paramLen < pSubGrp->metadataLen + sizeof(bass_subGrp_t)) {
                err = ATT_ERR_INVALID_PDU;
                break;
            }
            paramLen -= pSubGrp->metadataLen + sizeof(bass_subGrp_t);
            ptr += pSubGrp->metadataLen + sizeof(bass_subGrp_t);

            /* <BASS_v1.0>, Page14
             * If the server detects that a BIS_Sync parameter value written by a client is not 0xFFFFFFFF for a
             * subgroup, and if the server detects that a BIS_index value written by a client is set to a value of 0b1 in
             * more than one subgroup, the server shall ignore the operation.
             */
            if (pSubGrp->bisSync == 0xFFFFFFFF) {
                bisSyncNoPreferenceCnt++;
            }
        }
        if (paramLen || bisSyncNoPreferenceCnt > 1) {
            err = ATT_ERR_WRITE_REQUEST_REJECT;
        }
    } else if (opcode == BASS_OPCODE_MODIFY_SOURCE) {
        blt_bass_modifySrc_t *pModifySrc = (blt_bass_modifySrc_t *)pParam;

        if (paramLen < sizeof(blt_bass_modifySrc_t)) {
            return ATT_ERR_WRITE_REQUEST_REJECT;
        }
        paramLen -= sizeof(blt_bass_modifySrc_t);

        u8  bisSyncNoPreferenceCnt = 0x00;
        u8 *ptr                    = (u8 *)&pModifySrc->parameter.subGrps[0];

        for (i = 0; i < pModifySrc->parameter.numSubGrps; i++) {
            bass_subGrp_t *pSubGrp = (bass_subGrp_t *)(ptr);
            if (paramLen < pSubGrp->metadataLen + sizeof(bass_subGrp_t)) {
                err = ATT_ERR_INVALID_PDU;
                break;
            }
            paramLen -= pSubGrp->metadataLen + sizeof(bass_subGrp_t);
            ptr += pSubGrp->metadataLen + sizeof(bass_subGrp_t);

            /* <BASS_v1.0>, Page14
             * If the server detects that a BIS_Sync parameter value written by a client is not 0xFFFFFFFF for a
             * subgroup, and if the server detects that a BIS_index value written by a client is set to a value of 0b1 in
             * more than one subgroup, the server shall ignore the operation.
             */
            if (pSubGrp->bisSync == 0xFFFFFFFF) {
                bisSyncNoPreferenceCnt++;
            }
        }
        if (paramLen || bisSyncNoPreferenceCnt > 1) {
            err = ATT_ERR_WRITE_REQUEST_REJECT;
        }
    } else if (opcode == BASS_OPCODE_SET_BROADCAST_CODE) {
        err = paramLen == sizeof(blt_bass_setBcstCode_t) ? ATT_SUCCESS : ATT_ERR_WRITE_REQUEST_REJECT;
    } else if (opcode == BASS_OPCODE_REMOVE_SOURCE) {
        err = paramLen == sizeof(blt_bass_rmvSrc_t) ? ATT_SUCCESS : ATT_ERR_WRITE_REQUEST_REJECT;
    } else {
        err = BASS_APP_ERR_OPCODE_NOT_SUPP;
    }

    return err;
}

static u8 *blt_basss_getAvaBcstRecvState(u16 connHandle, u8 *outputIdx, u16 **len)
{
    blc_bass_server_t *basss = blt_basss_getCtrl(connHandle);
    if (!basss->bcstRcvStateCnt) {
        return NULL;
    }

    for (int index = 0; index < basss->bcstRcvStateCnt; index++) {
        u8 *value = NULL;
        blc_gatts_getAttributeInformationByHandle(connHandle, basss->recvStateHandle[index], &value, len);
        if (**len == 0) {
            if (outputIdx != NULL) {
                *outputIdx = index;
            }
            return value;
        }
    }

    BLT_BASS_LOG("ERR: BcstRcvState not available");
    return NULL; /* bcstRcvStateChar not found */
}

static u8 *blt_basss_findBcstRecvStateBySrcID(u16 connHandle, u8 sourceID, u16 **len)
{
    blc_bass_server_t *basss = blt_basss_getCtrl(connHandle);
    if (!basss->bcstRcvStateCnt || basss->bcstRcvStateCnt > sourceID) {
        BLT_BASS_LOG("ERR:BcstRcvState not found, foundSourceID is %d, maxCount is %d", sourceID, basss->bcstRcvStateCnt);
        return NULL;
    }

    u8  *value    = NULL;
    u16 *valueLen = NULL;
    blc_gatts_getAttributeInformationByHandle(connHandle, basss->recvStateHandle[sourceID - 1], &value, &valueLen);
    if (*valueLen == 0) {
        return NULL;
    }
    if (len) {
        *len = valueLen;
    }
    return value;
}

static blt_bass_recvState_t *blt_audio_rmvBcstRcvStateChr(u16 connHandle, blt_bass_recvState_t *pBcstRcvState, u16 *len)
{
    (void)len;
    blt_audio_broadcastSinkRecvEvt(connHandle, AUDIO_EVT_BASSS_REMOVE_SOURCE, (u8 *)pBcstRcvState, sizeof(blt_bass_syncPaEvt_t));

    return NULL;
}

static u8 *blt_basss_clearBcstRecvState(u16 connHandle, u8 index)
{
    blc_bass_server_t *basss = blt_basss_getCtrl(connHandle);
    if (index >= basss->bcstRcvStateCnt) {
        return NULL;
    }

    u8  *value    = NULL;
    u16 *valueLen = NULL;
    blc_gatts_getAttributeInformationByHandle(connHandle, basss->recvStateHandle[index], &value, &valueLen);

    blt_audio_rmvBcstRcvStateChr(connHandle, (blt_bass_recvState_t *)value, valueLen);

    return value;
}

static void blt_basss_sendSyncToBisEvt(u16 connHandle, u8 sourceID, u8 numSubGrps, u8 indexSubGroup, u32 BISSync, blc_audio_metadata_parsed_t *metadata)
{
    blt_bass_syncBisEvt_t syncBisEvt;

    syncBisEvt.sourceID      = sourceID;
    syncBisEvt.numSubGroup   = numSubGrps;
    syncBisEvt.indexSubGroup = indexSubGroup;
    syncBisEvt.BISSync       = BISSync;
    syncBisEvt.metaData      = metadata;
    BLT_BASS_LOG("send Sync BIS index = %d bisSync = 0x%08x", indexSubGroup, BISSync);

    blt_audio_broadcastSinkRecvEvt(connHandle, AUDIO_EVT_BASSS_SYNC_TO_BIS, (u8 *)&syncBisEvt, sizeof(blt_bass_syncBisEvt_t));
}

static void blt_basss_sendRecvSetBroadcastCode(u16 connHandle, u8 sourceID, u8 *broadcastCode)
{
    blt_basss_recvBroadcastCodeEvt_t recvBroadcastCode;

    recvBroadcastCode.sourceID = sourceID;
    memcpy(recvBroadcastCode.broadcastCode, broadcastCode, 16);

    blt_audio_broadcastSinkRecvEvt(connHandle, AUDIO_EVT_BASSS_RECV_SET_BROADCAST_CODE, (u8 *)&recvBroadcastCode, sizeof(blt_basss_recvBroadcastCodeEvt_t));
}

static void blt_basss_sendNotSyncPaEvt(u16 connHandle)
{
    blt_audio_broadcastSinkRecvEvt(connHandle, AUDIO_EVT_BASSS_DONOT_SYNC_TO_PA, NULL, 0);
}

static void blt_basss_sendSyncToPaEvt(u16 connHandle, u8 *state)
{
    blt_audio_broadcastSinkRecvEvt(connHandle, AUDIO_EVT_BASSS_SYNC_TO_PA, (u8 *)state, sizeof(blt_bass_syncPaEvt_t));
}

typedef att_err_t (*basss_dealOpcode)(u16 connHandle, u8 *value, u16 valueLen);

static att_err_t blt_basss_dealRemoteScanStopped(u16 connHandle, u8 *value, u16 valueLen)
{
    (void)value;
    (void)valueLen;
    BLT_BASS_LOG("receive Remote Scan Stopped");
    blt_audio_broadcastSinkRecvEvt(connHandle, AUDIO_EVT_BASSS_REMOTE_SCAN_STOPPED, NULL, 0);
    return ATT_SUCCESS;
}

static att_err_t blt_basss_dealRemoteScanStarted(u16 connHandle, u8 *value, u16 valueLen)
{
    (void)value;
    (void)valueLen;
    BLT_BASS_LOG("receive Remote Scan Started");
    blt_audio_broadcastSinkRecvEvt(connHandle, AUDIO_EVT_BASSS_REMOTE_SCAN_STARTED, NULL, 0);
    return ATT_SUCCESS;
}

static att_err_t blt_basss_dealAddSource(u16 connHandle, u8 *value, u16 valueLen)
{
    (void)valueLen;
    BLT_BASS_LOG("receive Add Source");

    blt_bass_AddSrc_t *pAddSrc = (blt_bass_AddSrc_t *)value;

    u8   stateIndex   = 0;
    u16 *pRcvStateLen = NULL;
    u8  *pRcvState    = blt_basss_getAvaBcstRecvState(connHandle, &stateIndex, &pRcvStateLen);

    bool ntfFlag = false;

    if (!pRcvState) {
        /* if the server has no empty Broadcast Receive State characteristics, the server
           shall first delete all fields in a selected Broadcast Receive State characteristic,<BASS_v1.0>, Page15 */
        pRcvState = blt_basss_clearBcstRecvState(connHandle, 0);
    }

    u8 *pRcvStateTemp = pRcvState;

    if (!pRcvState) {
        /* DO Nothing */
        BLT_BASS_LOG("ERR: Not found Ava BcstRcvState ");
        return (att_err_t)BASS_APP_ERR_INVALID_SOURCE_ID;
    }

    if ((pAddSrc->parameter.advAddrType > 0x01) ||
        (pAddSrc->parameter.advSID > 0x0F) ||
        (pAddSrc->parameter.paSync > BASS_SYNC_TO_PA_PAST_NAVA)) {
        return ATT_ERR_WRITE_REQUEST_REJECT;
    }

    //Source ID
    U8_TO_STREAM(pRcvState, stateIndex + 1);

    U8_TO_STREAM(pRcvState, pAddSrc->parameter.advAddrType);
    STR_TO_STREAM(pRcvState, pAddSrc->parameter.advAddr, 6);
    U8_TO_STREAM(pRcvState, pAddSrc->parameter.advSID);
    STR_TO_STREAM(pRcvState, pAddSrc->parameter.broadcastId, 3);

    if (pAddSrc->parameter.paSync == BASS_NOT_SYNC_TO_PA) {
        U8_TO_STREAM(pRcvState, BASS_PA_STATE_NOT_SYNC_TO_PA);
        ntfFlag = true; //need ntf client add source successful
        // Server shall not attempt to synchronize to the PA
        blt_basss_sendNotSyncPaEvt(connHandle);
    } else if (pAddSrc->parameter.paSync == BASS_SYNC_TO_PA_PAST_AVA ||
               pAddSrc->parameter.paSync == BASS_SYNC_TO_PA_PAST_NAVA) {
        //pAvaBcstRcvState->paSyncState = pAddSrc->parameter.paSync;
        //Two strategies: 1. Proactively establish a synchronization relationship with the PA;
        //                2. Request the Client to send SyncInfo (If the Server supports PAST procedures, you must use strategy 2).
        if (blmsParam.past_en && pAddSrc->parameter.paSync == BASS_SYNC_TO_PA_PAST_AVA) { //use PAST establish a synchronization with PA
            U8_TO_STREAM(pRcvState, BASS_PA_STATE_SYNCINFO_REQUEST);

            BLT_BASS_LOG("Add source: sync to PA use PAST");
            //When the state changes, the client needs to be notified
            ntfFlag                  = true; //need ntf client syncinfo request
            blc_bass_server_t *basss = blt_basss_getCtrl(connHandle);
            basss->pastStartTimer    = clock_time() | 1;
            basss->pastConnHandle    = connHandle;
            basss->sourceId          = stateIndex + 1;

        } else {
            U8_TO_STREAM(pRcvState, BASS_PA_STATE_NOT_SYNC_TO_PA);
            BLT_BASS_LOG("Add source: sync to PA");
        }
        blt_basss_sendSyncToPaEvt(connHandle, pRcvStateTemp);
    }

    U8_TO_STREAM(pRcvState, BASS_BIG_NOT_ENCRYPTED);

    u8 numSubGrps = min(pAddSrc->parameter.numSubGrps, BASS_SUPP_MAX_BIG_GROUPS);
    U8_TO_STREAM(pRcvState, numSubGrps);

    bass_subGrp_t *pAddPtr      = &pAddSrc->parameter.subGrps[0];
    bass_subGrp_t *pRcvStateSub = (bass_subGrp_t *)pRcvState;
    for (int index = 0; index < numSubGrps; index++) {
        BLT_BASS_LOG("Add bis index = %d Sync = 0x%08x metaData is 0x%s", index, pAddPtr->bisSync, hex_to_str(pAddPtr->metadata, pAddPtr->metadataLen));

        pRcvStateSub->bisSync                = 0x00;
        blc_audio_metadata_parsed_t metaData = {.ignoreUnsuppMetadataFlag = true};
        blt_audio_getMetadataParams(pAddPtr->metadataLen, pAddPtr->metadata, &metaData);

        blt_basss_sendSyncToBisEvt(connHandle, stateIndex + 1, numSubGrps, index, pAddPtr->bisSync, &metaData);
        pRcvStateSub->metadataLen = blt_audio_setMetadata(&metaData, pRcvStateSub->metadata);

        pAddPtr      = (bass_subGrp_t *)((u8 *)pAddPtr + sizeof(bass_subGrp_t) + pAddPtr->metadataLen);
        pRcvStateSub = (bass_subGrp_t *)((u8 *)pRcvStateSub + sizeof(bass_subGrp_t) + pRcvStateSub->metadataLen);
    }

    *pRcvStateLen = (u32)pRcvStateSub - (u32)pRcvStateTemp;

    if (ntfFlag) {
        blc_basss_notifyRecvState(connHandle, stateIndex + 1);
    }

    return ATT_SUCCESS;
}

static att_err_t blt_basss_dealModifySource(u16 connHandle, u8 *value, u16 valueLen)
{
    (void)valueLen;

    blt_bass_modifySrc_t *pModifySrc = (blt_bass_modifySrc_t *)value;

    BLT_BASS_LOG("receive Modify Source ID = %d", pModifySrc->parameter.srcId);

    u16                  *pRcvStateLen = NULL;
    blt_bass_recvState_t *pRcvState    = (blt_bass_recvState_t *)blt_basss_findBcstRecvStateBySrcID(connHandle, pModifySrc->parameter.srcId, &pRcvStateLen);
    if (!pRcvState) {
        return (att_err_t)BASS_APP_ERR_INVALID_SOURCE_ID;
    }

    if (pModifySrc->parameter.paSync > BASS_SYNC_TO_PA_PAST_NAVA) {
        return ATT_ERR_WRITE_REQUEST_REJECT;
    }

    bool ntfFlag = false;

    if (pModifySrc->parameter.paSync == BASS_NOT_SYNC_TO_PA) {
        pRcvState->paSyncState = BASS_PA_STATE_NOT_SYNC_TO_PA;

        // 1. Server shall not attempt to synchronize to the PA; 2. Server shall stop synchronization with the PA
        ntfFlag = true;
        blt_basss_sendNotSyncPaEvt(connHandle);
    } else if (pModifySrc->parameter.paSync == BASS_SYNC_TO_PA_PAST_AVA ||
               pModifySrc->parameter.paSync == BASS_SYNC_TO_PA_PAST_NAVA) {
        //Two strategies: 1. Proactively establish a synchronization relationship with the PA;
        //                2. Request the Client to send SyncInfo (If the Server supports PAST procedures, you must use strategy 2).
        if (pRcvState->paSyncState != BASS_PA_STATE_SYNC_TO_PA) {
            if (blmsParam.past_en && pModifySrc->parameter.paSync == BASS_SYNC_TO_PA_PAST_AVA) { //use PAST establish a synchronization with PA
                pRcvState->paSyncState = BASS_PA_STATE_SYNCINFO_REQUEST;

                ntfFlag = true;
                //When the state changes, the client needs to be notified
                blc_bass_server_t *basss = blt_basss_getCtrl(connHandle);
                basss->pastStartTimer    = clock_time() | 1;
                basss->pastConnHandle    = connHandle;
                basss->sourceId          = pModifySrc->parameter.srcId;
            } else {
                pRcvState->paSyncState = BASS_PA_STATE_NOT_SYNC_TO_PA;
                BLT_BASS_LOG("Modify source: sync to PA");
            }
            blt_basss_sendSyncToPaEvt(connHandle, (u8 *)pRcvState);
        } else { /* the server has synced to the PA */
            /**
             * If the server has synchronized to the PA, and the server has detected that the BIS is encrypted, and if
             * the server does not have the correct encryption key to decrypt the BIS, the server shall write a value
             * of 0x01 (Broadcast_Code required) to the BIG_Encryption field of the Broadcast Receive State
             * characteristic to request a client to provide a Broadcast_Code.
             */
            if (pRcvState->bigEncryption == BASS_BIG_BAD_CODE) {
                pRcvState->bigEncryption = BASS_BIG_BCSTCODE_REQUIRED;

                memcpy(&pRcvState->numSubGrps, &pRcvState->encNumSubGrps, *pRcvStateLen - OFFSETOF(blt_bass_recvState_t, badCode));
                *pRcvStateLen -= 16;
                //When the state changes, the client needs to be notified
                ntfFlag = true;
            }
        }
    }

    bass_subGrp_t *pModifyPtr   = &pModifySrc->parameter.subGrps[0];
    u8             numSubGrps   = min(pModifySrc->parameter.numSubGrps, BASS_SUPP_MAX_BIG_GROUPS);
    bass_subGrp_t *pRcvStateSub = NULL;

    if (pRcvState->bigEncryption == BASS_BIG_BAD_CODE) {
        pRcvState->encNumSubGrps = numSubGrps;
        pRcvStateSub             = pRcvState->encSubGrps;
    } else {
        pRcvState->numSubGrps = numSubGrps;
        pRcvStateSub          = pRcvState->subGrps;
    }

    for (int index = 0; index < numSubGrps; index++) {
        BLT_BASS_LOG("Modify BIS index = %d Sync = 0x%08x metaData is 0x%s", index, pModifyPtr->bisSync, hex_to_str(pModifyPtr->metadata, pModifyPtr->metadataLen));

        //      pRcvStateSub->bisSync = 0x00;
        blc_audio_metadata_parsed_t metaData = {.ignoreUnsuppMetadataFlag = true};
        blt_audio_getMetadataParams(pModifyPtr->metadataLen, pModifyPtr->metadata, &metaData);

        blt_basss_sendSyncToBisEvt(connHandle, pModifySrc->parameter.srcId, numSubGrps, index, pModifyPtr->bisSync, &metaData);

        pRcvStateSub->metadataLen = blt_audio_setMetadata(&metaData, pRcvStateSub->metadata);

        pModifyPtr   = (bass_subGrp_t *)((u8 *)pModifyPtr + sizeof(bass_subGrp_t) + pModifyPtr->metadataLen);
        pRcvStateSub = (bass_subGrp_t *)((u8 *)pRcvStateSub + sizeof(bass_subGrp_t) + pRcvStateSub->metadataLen);
    }

    *pRcvStateLen = (u32)pRcvStateSub - (u32)pRcvState;

    if (ntfFlag) {
        blc_basss_notifyRecvState(connHandle, pModifySrc->parameter.srcId);
    }

    return ATT_SUCCESS;
}

static att_err_t blt_basss_dealSetBroadcastCode(u16 connHandle, u8 *value, u16 valueLen)
{
    (void)valueLen;
    blt_bass_setBcstCode_t *pSetBcstCode = (blt_bass_setBcstCode_t *)value;

    BLT_BASS_LOG("receive Set Broadcast Code is %s", hex_to_str(pSetBcstCode->parameter.BcstCode, 16));

    u16                  *pRcvStateLen = NULL;
    blt_bass_recvState_t *pRcvState    = (blt_bass_recvState_t *)blt_basss_findBcstRecvStateBySrcID(connHandle, pSetBcstCode->parameter.srcId, &pRcvStateLen);

    if (!pRcvState) {
        return (att_err_t)BASS_APP_ERR_INVALID_SOURCE_ID;
    }

    blt_basss_sendRecvSetBroadcastCode(connHandle, pSetBcstCode->parameter.srcId, pSetBcstCode->parameter.BcstCode);

    return ATT_SUCCESS;
}

static att_err_t blt_basss_dealRemoveSource(u16 connHandle, u8 *value, u16 valueLen)
{
    (void)valueLen;
    blt_bass_rmvSrc_t *pRmvSrc = (blt_bass_rmvSrc_t *)value;

    BLT_BASS_LOG("receive Remove Source ID = %d", pRmvSrc->srcId);

    u16                  *pRcvStateLen = NULL;
    blt_bass_recvState_t *pRcvState    = (blt_bass_recvState_t *)blt_basss_findBcstRecvStateBySrcID(connHandle, pRmvSrc->srcId, &pRcvStateLen);

    if (!pRcvState) {
        return (att_err_t)BASS_APP_ERR_INVALID_SOURCE_ID;
    }

    if (pRcvState->paSyncState == BASS_PA_STATE_SYNC_TO_PA) {
        BLT_BASS_LOG("WRN: current pa sync state is SYNCED TO PA");
        return ATT_SUCCESS;
    }

    u8             numSubGrps = 0;
    bass_subGrp_t *subGrps    = NULL;
    if (pRcvState->bigEncryption == BASS_BIG_BAD_CODE) {
        numSubGrps = pRcvState->encNumSubGrps;
        subGrps    = pRcvState->encSubGrps;
    } else {
        numSubGrps = pRcvState->numSubGrps;
        subGrps    = pRcvState->subGrps;
    }

    for (int index = 0; index < numSubGrps; index++) {
        if (subGrps->bisSync != 0xFFFFFFFF && subGrps->bisSync) {
            BLT_BASS_LOG("WRN:BIS sync is 0x%08x", subGrps->bisSync);
            return ATT_SUCCESS;
        }
        subGrps = (bass_subGrp_t *)((u8 *)subGrps + sizeof(bass_subGrp_t) + subGrps->metadataLen);
    }

    //Release BcstRcvStateChar resource
    blt_audio_rmvBcstRcvStateChr(connHandle, pRcvState, pRcvStateLen);

    *pRcvStateLen = 0;

    blc_basss_notifyRecvState(connHandle, pRmvSrc->srcId);

    return ATT_SUCCESS;
}

static const basss_dealOpcode basssFun[] = {
    blt_basss_dealRemoteScanStopped,
    blt_basss_dealRemoteScanStarted,
    blt_basss_dealAddSource,
    blt_basss_dealModifySource,
    blt_basss_dealSetBroadcastCode,
    blt_basss_dealRemoveSource};

static int blt_basss_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8 *writeValue, u16 valueLen)
{
    (void)opcode;
    blc_bass_server_t *basss = blt_basss_getCtrl(connHandle);

    BLT_BASS_LOG("Write Handle is %x, value is %s", attrHandle, hex_to_str(writeValue, valueLen));

    if (!basss->bcstRcvStateCnt || valueLen < 1) {
        BLT_BASS_LOG("ERR: broadcast receive state count is %d, valueLen is %d", basss->bcstRcvStateCnt, valueLen);
        return ATT_ERR_INVALID_PDU;
    }

    if (attrHandle != basss->bassCtrlHandle) {
        BLT_BASS_LOG("ERR: write attrHandle is 0x%x, correct handle is 0x%x", attrHandle, basss->bassCtrlHandle);
        return ATT_ERR_INVALID_HANDLE;
    }

    u8       *pTemp   = writeValue;
    u8        parmLen = valueLen;
    att_err_t err     = blt_basss_checkWriteParam(pTemp, parmLen);

    if (err != ATT_SUCCESS) {
        return err;
    }

    return basssFun[writeValue[0]](connHandle, writeValue, valueLen);
}

static void blt_basss_initCtrlPointChar(atts_foundCharParam_t *p, void *input)
{
    blc_bass_server_t *basss = (blc_bass_server_t *)input;
    if (p->num > 0) {
        BLT_BASS_LOG("ERR: control point char[%d] too many", p->num);
        return;
    }
    basss->bassCtrlHandle = p->charHandle;

    BLT_BASS_LOG("control point handle is 0x%x", p->charHandle);
}

static void blt_basss_initRecvStateChar(atts_foundCharParam_t *p, void *input)
{
    blc_bass_server_t *basss = (blc_bass_server_t *)input;
    if (p->num >= STACK_AUDIO_BASS_RECV_STATE_NUM) {
        BLT_BASS_LOG("ERR: receive state char max num is %d , set num is %d", STACK_AUDIO_BASS_RECV_STATE_NUM, p->num);
        return;
    }

    basss->recvStateHandle[p->num] = p->charHandle;
    basss->bcstRcvStateCnt++;
    *p->charDataLen = 0;

    BLT_BASS_LOG("receive state[%d] handle is 0x%x", p->num, p->charHandle);
}

static const atts_findCharList_t basssChar[] = {
    {
     .charUuid    = characteristicBasControlPointUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_basss_initCtrlPointChar,
     },
    {
     .charUuid    = characteristicBroadcastReceiveStateUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_basss_initRecvStateChar,
     },
};

static void blt_basss_serviceInit(blc_bass_server_t *server)
{
    blc_atts_findCharacteristicByServiceUuid(serviceBroadcastAudioScanUuid, ATT_16_UUID_LEN, basssChar, ARRAY_SIZE(basssChar), server);
}

bool blc_basss_updatePASyncState(u16 connHandle, u8 sourceID, blt_bass_pa_sync_state_enum paSyncState)
{
    blt_bass_recvState_t *pRcvState = (blt_bass_recvState_t *)blt_basss_findBcstRecvStateBySrcID(connHandle, sourceID, NULL);

    if (!pRcvState) {
        return false;
    }

    pRcvState->paSyncState = paSyncState;
    BLT_BASS_LOG("update PA Sync state sourceId[%d], paSyncState[%d]", sourceID, paSyncState);

    return true;
}

bool blc_basss_updateBigEncState(u16 connHandle, u8 sourceID, blt_bass_big_encryption_state_enum bigEncState, u8 *badCode)
{
    u16                  *pRcvStateLen = NULL;
    blt_bass_recvState_t *pRcvState    = (blt_bass_recvState_t *)blt_basss_findBcstRecvStateBySrcID(connHandle, sourceID, &pRcvStateLen);

    if (!pRcvState) {
        return false;
    }

    if (bigEncState == BASS_BIG_BAD_CODE && pRcvState->bigEncryption != BASS_BIG_BAD_CODE) {
        u8 *src = (u8 *)pRcvState + *pRcvStateLen;
        for (size_t i = 0; i < *pRcvStateLen - OFFSETOF(blt_bass_recvState_t, bigEncryption); i++) {
            *(src + 16) = *src;
            src--;
        }
        *pRcvStateLen += 16;
    } else if (bigEncState != BASS_BIG_BAD_CODE && pRcvState->bigEncryption == BASS_BIG_BAD_CODE) {
        memcpy(&pRcvState->numSubGrps, &pRcvState->encNumSubGrps, *pRcvStateLen - OFFSETOF(blt_bass_recvState_t, badCode));
        *pRcvStateLen -= 16;
    }

    pRcvState->bigEncryption = bigEncState;
    if (bigEncState == BASS_BIG_BAD_CODE) {
        memcpy(pRcvState->badCode, badCode, 16);
    }

    BLT_BASS_LOG("update PA Sync state sourceId[%d], bigEncState[%d]", sourceID, bigEncState);

    return true;
}

static bass_subGrp_t *blc_basss_getSubGrpsPtr(blt_bass_recvState_t *state, u8 **numSubGrps)
{
    if (state->bigEncryption == BASS_BIG_BAD_CODE) {
        *numSubGrps = &state->encNumSubGrps;
        return state->encSubGrps;
    } else {
        *numSubGrps = &state->numSubGrps;
        return state->subGrps;
    }
}

static bass_subGrp_t *blc_basss_getIndexSubGrpsPtr(blt_bass_recvState_t *state, u8 num)
{
    u8            *numSubGrps = NULL;
    bass_subGrp_t *subGrp     = blc_basss_getSubGrpsPtr(state, &numSubGrps);
    if (*numSubGrps <= num) {
        return NULL;
    }
    for (int index = 0; index < num; index++) {
        subGrp = (bass_subGrp_t *)((u8 *)subGrp + sizeof(bass_subGrp_t) + subGrp->metadataLen);
    }
    return subGrp;
}

bool blc_basss_updateBisSyncState(u16 connHandle, u8 sourceID, u8 subGrpIndex, u32 bisSync)
{
    blt_bass_recvState_t *pRcvState = (blt_bass_recvState_t *)blt_basss_findBcstRecvStateBySrcID(connHandle, sourceID, NULL);

    if (!pRcvState) {
        return false;
    }

    bass_subGrp_t *subGrp = blc_basss_getIndexSubGrpsPtr(pRcvState, subGrpIndex);
    subGrp->bisSync       = bisSync;
    BLT_BASS_LOG("sourceId is %d, BIS Sync is %d", sourceID, bisSync);
    return true;
}
