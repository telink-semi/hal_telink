/********************************************************************************************************
 * @file    ascs_server.c
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

static void blt_ascss_serviceInit(blc_ascs_server_t *server);
static int  blt_ascss_readCback(u16 connHandle, u8 opcode, u16 attrHandle, u8 **outValue, u16 *outValueLen);
static int  blt_ascss_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8 *writeValue, u16 valueLen);


_attribute_ble_data_retention_
    blc_ascs_server_ctrl_t ascs_server_ctrl = {
        .process = {
                    .pNext       = NULL,
                    .id          = AUDIO_ASCS_SERVER,
                    .usedAclRole = 0,
                    .init        = blt_ascss_init,
                    .connect     = blt_ascss_connect,
                    .discov      = NULL,
                    .loop        = NULL,
                    },
};

void blc_audio_registerASCSControlServer(const blc_ascss_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_PERIPHERAL, (blc_prf_proc_t *)&ascs_server_ctrl, param);
}

blc_ascs_server_t *blt_ascss_getCtrl(u16 connHandle)
{
    int ret = blt_prf_getAclRole(connHandle);
    if (ret < 0 || ret == ACL_ROLE_CENTRAL) {
        BLT_ASCS_LOG("ERR: ACL role, unlikely: 0x%x", ret);

        if (ret >= 0) {
            /* BAP Unicast Server GAP Peripheral */
            blt_prf_sendSvrGapRoleErrEvt(connHandle, AUDIO_ASCS_SERVER, ret);
        }

        return NULL;
    }

    /* Notice: The buffer defined in the protocol layer only considers the slave or master,
     * and does not consider the situation together. blmsParam.max_master_num is subtracted
     * here for security.  */
    int idx = blc_prf_getAclConnectIndex(connHandle) - blmsParam.max_master_num; //already checked aclHandle
    return ascs_server_ctrl.pAcscServer[idx];
}

#define ASCSS_ASE_CTRL_HANDLE(connHandle) (blt_ascss_getCtrl(connHandle)->aseCtrlHandle)

int blt_ascss_init(u8 initType, const void *param)
{
#if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_ascs_server_t)), blc_ascs_server_t);
#endif
    (void)param;

    if (initType == PRF_PROC_INIT) {
        blc_svc_addAscsGroup();
        blc_svc_ascsCbackRegister(blt_ascss_readCback, blt_ascss_writeCback);
        BLT_ASCS_LOG("blt_ascss_init");
    } else if (initType == PRF_PROC_DEINIT) {
        blc_svc_removeAscsGroup();
        BLT_ASCS_LOG("blt_ascss_deinit");
    }

    for (int i = 0; i < gAppAudioAclPeripheralNum; i++) {
        ascs_server_ctrl.pAcscServer[i] = blc_ascss_getAscssInfo(i);
        blc_ascs_server_t *server       = ascs_server_ctrl.pAcscServer[i];
        memset((u8 *)server, 0, sizeof(blc_ascs_server_t));
        for (int j = 0; j < gAscssSinkAseCnt + gAscssSrcAseCnt; j++) {
            server->aseState[j] = blc_ascss_getAseStateInfo(i * (gAscssSinkAseCnt + gAscssSrcAseCnt) + j);
            memset((u8 *)server->aseState[j], 0, sizeof(blt_ascss_ase_state_t));
        }
        if (initType == PRF_PROC_INIT) {
            blt_ascss_serviceInit(server);
        }
    }
    return 0;
}

int blt_ascss_connect(u16 connHandle, prf_acl_state_enum connState)
{
    if (connState == PRF_ACL_STATE_DISCONN) {
        BLT_ASCS_LOG("blt_ascss_disconnect: 0x%x", connHandle);
        blc_ascs_server_t *ascss = blt_ascss_getCtrl(connHandle);
        for (int i = 0; i < ascss->aseCnt; i++) {
            memset((u8 *)&ascss->aseState[i]->state, 0, sizeof(blt_ascss_ase_state_t) - OFFSETOF(blt_ascss_ase_state_t, state));
            BLT_ASCS_LOG("  i[%d] ase_state[%d]", i, ascss->aseState[i]->state);
        }
    } else {
        BLT_ASCS_LOG("blt_ascss_connect: 0x%x", connHandle);
    }

    return 0;
}

static int blt_ascss_readCback(u16 connHandle, u8 opcode, u16 attrHandle, u8 **outValue, u16 *outValueLen)
{
    (void)opcode;

    blc_ascs_server_t *ascss = blt_ascss_getCtrl(connHandle);
    for (int i = 0; i < ascss->aseCnt; i++) {
        if (attrHandle == ascss->aseState[i]->aseHandle) {
            if (ascss->aseState[i]->state == BLT_ASCS_ASE_STATE_IDLE || ascss->aseState[i]->state == BLT_ASCS_ASE_STATE_RELEASING) {
                *outValue    = &ascss->aseState[i]->aseID;
                *outValueLen = 2;
            } else if (ascss->aseState[i]->state == BLT_ASCS_ASE_STATE_CODEC_CFG) {
                ascss->aseState[i]->codecState.aseID    = ascss->aseState[i]->aseID;
                ascss->aseState[i]->codecState.aseState = ascss->aseState[i]->state;
                blc_ascss_initAseParam(ascss->aseState[i]);
                *outValue    = &ascss->aseState[i]->codecState.aseID;
                *outValueLen = 25 + ascss->aseState[i]->codecState.codecSpecCfgLen;
            } else if (ascss->aseState[i]->state == BLT_ASCS_ASE_STATE_QOS_CFG) {
                ascss->aseState[i]->QosState.aseID    = ascss->aseState[i]->aseID;
                ascss->aseState[i]->QosState.aseState = ascss->aseState[i]->state;
                *outValue                             = &ascss->aseState[i]->QosState.aseID;
                *outValueLen                          = sizeof(blt_ascss_aseStateQosCfg_t);
            } else {
                ascss->aseState[i]->otherState.aseID    = ascss->aseState[i]->aseID;
                ascss->aseState[i]->otherState.aseState = ascss->aseState[i]->state;
                ascss->aseState[i]->otherState.cigID    = ascss->aseState[i]->QosState.cigID;
                ascss->aseState[i]->otherState.cisID    = ascss->aseState[i]->QosState.cisID;
                *outValue                               = &ascss->aseState[i]->otherState.aseID;
                *outValueLen                            = 5 + ascss->aseState[i]->otherState.metadataLen;
            }
        }
    }
    return ATT_SUCCESS;
}

static void blt_ascss_pushAseCtrlNtf(u16 connHandle, blt_ascs_aseCtrlPointCharNtf_t *pRsp)
{
    u8 numOfAses = pRsp->numOfAses == 0xFF ? 1 : pRsp->numOfAses;
    u8 totalLen  = sizeof(blt_ascs_aseCtrlPointCharNtf_t) + numOfAses * sizeof(aseCtrlNtfPayload_t);

    blc_ascs_server_t *ascss = blt_ascss_getCtrl(connHandle);
    ascss->aseCtrlNtfLen     = totalLen;
    memcpy(ascss->aseCtrlNtf, (u8 *)pRsp, totalLen);
    blc_gatts_notifyValue(connHandle, ASCSS_ASE_CTRL_HANDLE(connHandle), ascss->aseCtrlNtf, ascss->aseCtrlNtfLen);
}

static void blt_ascss_pushAseCtrlNtfErr(u16 connHandle, u8 opcode, u8 resCode, u8 reason)
{
    u8                              buf[5];
    blt_ascs_aseCtrlPointCharNtf_t *aseState = (blt_ascs_aseCtrlPointCharNtf_t *)&buf[0];
    aseState->opcode                         = opcode;
    aseState->numOfAses                      = 0xFF;    /* If the Response_Code value is 0x01 or 0x02, Number_of_ASEs shall be set to 0xFF */
    aseState->payload[0].aseID               = 0x00;
    aseState->payload[0].responseCode        = resCode; /* If the Response_Code value is 0x01 or 0x02, [i] shall be set to 0. */
    aseState->payload[0].reason              = reason;  /* If the Response_Code value is 0x01 or 0x02, [i] shall be set to 0. */

    blt_ascss_pushAseCtrlNtf(connHandle, aseState);
}

static blt_ascss_ase_state_t *blt_ascss_findAseState(u16 connHandle, u8 aseID)
{
    blc_ascs_server_t *ascss = blt_ascss_getCtrl(connHandle);
    for (int i = 0; i < ascss->aseCnt; i++) {
        if (ascss->aseState[i]->aseID == aseID) {
            return ascss->aseState[i];
        }
    }
    return NULL;
}

//TODO,
//Target latency and targetPHY should be taken into account and calculate a matched latency when notified codec state,
//note 2023.1.9 by tianxiang.
static int blt_ascss_dealCfgCodec(u16 connHandle, blt_ascss_ase_state_t *pAse, u8 *pData, aseCtrlNtfPayload_t *rsp)
{
    blt_ascs_cfgCodec_t *cfgCodec = (blt_ascs_cfgCodec_t *)pData;

    blc_audio_codecSpecCfgParsed_t specCfgParam = {0};
    BLT_ASCS_LOG("DBG ASCS: config Codec");

    if (blt_audio_getCodecSpecCfgParam(&cfgCodec->codecSpecCfgLen, &specCfgParam) != AUDIO_ESUCC) {
        rsp->responseCode = BLT_ASCS_RSP_CODE_UNSUPP_CONFIG_PARAM;
        rsp->reason       = BLT_ASCS_REASON_CODEC_SEPC_CONFIG;
        BLT_ASCS_LOG("get codec cfg param error");
        return AUDIO_ERR_PARAM_INVALID;
    }
    u8 type = pAse->dir == AUDIO_DIR_SINK ? BLT_PAC_SINK : BLT_PAC_SOURCE;
    if (blt_pacss_checkCodecCfgParam(connHandle, type, &specCfgParam) != AUDIO_ESUCC) {
        rsp->responseCode = BLT_ASCS_RSP_CODE_UNSUPP_CONFIG_PARAM;
        rsp->reason       = BLT_ASCS_REASON_CODEC_SEPC_CONFIG;
        BLT_ASCS_LOG("codec cfg param error");
        return AUDIO_ERR_PARAM_INVALID;
    }
    memcpy(&pAse->codecState.codecSpecCfgLen, &cfgCodec->codecSpecCfgLen, cfgCodec->codecSpecCfgLen + 1);
    memcpy(&pAse->codecState.codecId.id, &cfgCodec->codecID.id, sizeof(blc_audio_codec_id_t));
    return AUDIO_ESUCC;
}

/* Server concerned functions, process C->S */
static blt_ascs_reason_enum blt_ascs_checkQosCfgParam(blt_ascs_cfgQos_t *pCfgQosParam)
{
    u32 presentationDelay = bstream_to_u24_le(pCfgQosParam->presentationDelay);
    if ((presentationDelay < AUDIO_UNICAST_SERVER_PRESENTATION_DELAY_MIN) || (presentationDelay > AUDIO_UNICAST_SERVER_PRESENTATION_DELAY_MAX)) {
        return BLT_ASCS_REASON_PRESENT_DELAY;
    }
    if (pCfgQosParam->maxTranLatency > AUDIO_UNICAST_SERVER_MAX_TRANSPORT_LATENCY) {
        BLT_ASCS_LOG("latency %d", pCfgQosParam->maxTranLatency);
        return BLT_ASCS_REASON_MAX_LATENCY;
    }
    if (pCfgQosParam->cigID > 0xEF) {
        return BLT_ASCS_REASON_INVALID_AES_CIS_MAPPING;
    }
    if (pCfgQosParam->cisID > 0xEF) {
        return BLT_ASCS_REASON_INVALID_AES_CIS_MAPPING;
    }
    if (pCfgQosParam->framing > 1) {
        return BLT_ASCS_REASON_FRAMING; /*  0 OR 1 */
    }
    return BLT_ASCS_REASON_SUCCESS;
}

static bool blt_ascss_checkCisMap(u16 connHandle, blt_ascss_ase_state_t *pAse, u8 cigID, u8 cisID)
{
    blc_ascs_server_t *ascss = blt_ascss_getCtrl(connHandle);

    for (int i = 0; i < ascss->aseCnt; i++) {
        blt_ascss_ase_state_t *ase = ascss->aseState[i];
        if (ase->dir != pAse->dir || ase->aseID == pAse->aseID || ase->state == BLT_ASCS_ASE_STATE_IDLE || ase->state == BLT_ASCS_ASE_STATE_CODEC_CFG) {
            continue;
        }
        if (cigID == ase->QosState.cigID && cisID == ase->QosState.cisID) {
            return false;
        }
    }
    return true;
}

static int blt_ascss_dealCfgQos(u16 connHandle, blt_ascss_ase_state_t *pAse, u8 *pData, aseCtrlNtfPayload_t *rsp)
{
    //parameter check, eg:cigID,PHY ...
    blt_ascs_cfgQos_t *configQos = (blt_ascs_cfgQos_t *)pData;

    BLT_ASCS_LOG("DBG ASCS: config qos");
    blt_ascs_reason_enum reason = blt_ascs_checkQosCfgParam(configQos);
    if (reason) {
        rsp->responseCode = BLT_ASCS_RSP_CODE_INVALID_CONFIG_PARAM;
        rsp->reason       = reason;
        BLT_ASCS_LOG("Error: qos param invalid");
        return AUDIO_ERR_PARAM_INVALID;
    }

    /* If a client requests a Config QoS operation for an ASE that would result in more than one Sink ASE
        having identical CIG_ID and CIS_ID parameter values for that client, or that would result in more than one
        Source ASE having identical CIG_ID and CIS_ID parameter values for that client, the server shall not
        accept the Config QoS operation for that ASE. The server shall send a notification of the ASE Control
        Point characteristic to the client, the server shall set the Response_Code value for that ASE to 0x09
        (Invalid Parameter Value), and the server shall set the Reason value for that ASE to 0x0A
        (Invalid_ASE_CIS_Mapping). */
    if (!blt_ascss_checkCisMap(connHandle, pAse, configQos->cigID, configQos->cisID)) {
        rsp->responseCode = BLT_ASCS_RSP_CODE_INVALID_CONFIG_PARAM;
        rsp->reason       = BLT_ASCS_REASON_INVALID_AES_CIS_MAPPING;
        BLT_ASCS_LOG("Error: cis map invalid");
        return AUDIO_ERR_PARAM_INVALID;
    }
    BLT_ASCS_LOG("DBG ASCS: config qos success-ase ID: %d", pAse->aseID);
    memcpy(&pAse->QosState.cigID, &configQos->cigID, sizeof(blt_ascs_cfgQos_t) - 1);
    return AUDIO_ESUCC;
}

static int blt_ascss_dealMetadataPacket(u16 connHandle, blt_ascss_ase_state_t *pAse, u8 *pData, aseCtrlNtfPayload_t *rsp)
{
    blt_ascs_metadata_t        *aseEnable = (blt_ascs_metadata_t *)pData;
    blc_audio_metadata_parsed_t metaParam = {.ignoreUnsuppMetadataFlag = 0};
    if (blt_audio_getMetadataParams(aseEnable->metadataLen, aseEnable->metadataCfg, &metaParam)) {
        rsp->responseCode = metaParam.rspCode;
        rsp->reason       = metaParam.rsnMark;
        return AUDIO_ERR_PARAM_INVALID;
    }
    u8 *pMeta = pAse->otherState.metadata;
    if (metaParam.fieldExistFlg & BLC_AUDIO_METATYPE_STREAMING_CONTEXTS_MASK) {
        u16 context = blt_pacss_getAvailableContext(connHandle, pAse->dir);
        if ((context & metaParam.streamingCtx) != metaParam.streamingCtx) {
            rsp->responseCode = BLT_ASCS_RSP_CODE_REJECTED_CONFIG_PARAM;
            rsp->reason       = 0x00;
            return AUDIO_ERR_PARAM_INVALID;
        }
        U8_TO_STREAM(pMeta, 0x03);
        U8_TO_STREAM(pMeta, BLC_AUDIO_METATYPE_STREAMING_CONTEXTS);
        U16_TO_STREAM(pMeta, metaParam.streamingCtx);
    }
    if (metaParam.fieldExistFlg & BLC_AUDIO_METATYPE_PROGRAM_INFO_MASK) {
        U8_TO_STREAM(pMeta, metaParam.programInfoLen + 1);
        U8_TO_STREAM(pMeta, BLC_AUDIO_METATYPE_PROGRAM_INFO);
        STR_TO_STREAM(pMeta, metaParam.pProgramInfo, metaParam.programInfoLen);
    }
    if (metaParam.fieldExistFlg & BLC_AUDIO_METATYPE_LANGUAGE_MASK) {
        U8_TO_STREAM(pMeta, 0x04);
        U8_TO_STREAM(pMeta, BLC_AUDIO_METATYPE_LANGUAGE);
        U24_TO_STREAM(pMeta, metaParam.language);
    }
    if (metaParam.fieldExistFlg & BLC_AUDIO_METATYPE_CCID_LIST_MASK) {
        U8_TO_STREAM(pMeta, metaParam.ccidListLen + 1);
        U8_TO_STREAM(pMeta, BLC_AUDIO_METATYPE_CCID_LIST);
        STR_TO_STREAM(pMeta, metaParam.pCcidList, metaParam.ccidListLen);
    }
    if (metaParam.fieldExistFlg & BLC_AUDIO_METATYPE_PARENTAL_RATING_MASK) {
        U8_TO_STREAM(pMeta, 0x02);
        U8_TO_STREAM(pMeta, BLC_AUDIO_METATYPE_PARENTAL_RATING);
        U8_TO_STREAM(pMeta, metaParam.parentalRating);
    }
    if (metaParam.fieldExistFlg & BLC_AUDIO_METATYPE_PROGRAM_INFO_URI_MASK) {
        U8_TO_STREAM(pMeta, metaParam.programInfoURILen + 1);
        U8_TO_STREAM(pMeta, BLC_AUDIO_METATYPE_PROGRAM_INFO_URI);
        STR_TO_STREAM(pMeta, metaParam.pProgramInfoURI, metaParam.programInfoURILen);
    }
    if (metaParam.fieldExistFlg & BLC_AUDIO_METATYPE_EXTENDED_METADATA_MASK) {
        U8_TO_STREAM(pMeta, metaParam.extMetadataLen + 1);
        U8_TO_STREAM(pMeta, BLC_AUDIO_METATYPE_EXTENDED_METADATA);
        STR_TO_STREAM(pMeta, metaParam.pExtMetadata, metaParam.extMetadataLen);
    }
    if (metaParam.fieldExistFlg & BLC_AUDIO_METATYPE_VENDOR_SPECIFIC_MASK) {
        U8_TO_STREAM(pMeta, metaParam.vsMetadataLen + 1);
        U8_TO_STREAM(pMeta, BLC_AUDIO_METATYPE_VENDOR_SPECIFIC);
        STR_TO_STREAM(pMeta, metaParam.pVendorSpecMetadata, metaParam.vsMetadataLen);
    }
    pAse->otherState.metadataLen = pMeta - pAse->otherState.metadata;
    return AUDIO_ESUCC;
}

static int blt_ascss_dealEnable(u16 connHandle, blt_ascss_ase_state_t *pAse, u8 *pData, aseCtrlNtfPayload_t *rsp)
{
    audio_error_enum err = blt_ascss_dealMetadataPacket(connHandle, pAse, pData, rsp);
    return err;
}

static int blt_ascss_dealRecvStartReady(u16 connHandle, blt_ascss_ase_state_t *pAse, u8 *pData, aseCtrlNtfPayload_t *rsp)
{
    (void)connHandle;
    (void)pData;
    if (pAse->dir == AUDIO_DIR_SINK) {
        rsp->responseCode = BLT_ASCS_RSP_CODE_INVALID_ASE_DIRECTION;
        rsp->reason       = BLT_ASCS_REASON_SUCCESS;
        return AUDIO_ERR_PARAM_INVALID;
    }
    return AUDIO_ESUCC;
}

static int blt_ascss_dealDisable(u16 connHandle, blt_ascss_ase_state_t *pAse, u8 *pData, aseCtrlNtfPayload_t *rsp)
{
    /*  If a Source ASE is in the Disabling state, and/or if a Sink ASE is in the QoS Configured state, the Unicast
        Client or the Unicast Server may terminate a CIS established for that ASE */
    (void)connHandle;
    (void)pAse;
    (void)pData;
    (void)rsp;
    return AUDIO_ESUCC;
}

static int blt_ascss_dealRecvStopReady(u16 connHandle, blt_ascss_ase_state_t *pAse, u8 *pData, aseCtrlNtfPayload_t *rsp)
{
    (void)connHandle;
    (void)pAse;
    (void)pData;
    (void)rsp;
    return AUDIO_ESUCC;
}

static int blt_ascss_dealUpdateMetadata(u16 connHandle, blt_ascss_ase_state_t *pAse, u8 *pData, aseCtrlNtfPayload_t *rsp)
{
    audio_error_enum err = blt_ascss_dealMetadataPacket(connHandle, pAse, pData, rsp);
    if (err == AUDIO_ESUCC) {
        blt_audio_unicastSvrUpdateEvt(connHandle, pAse);
    }
    return err;
}

static int blt_ascss_dealRelease(u16 connHandle, blt_ascss_ase_state_t *pAse, u8 *pData, aseCtrlNtfPayload_t *rsp)
{
    (void)connHandle;
    (void)pAse;
    (void)pData;
    (void)rsp;
    return AUDIO_ESUCC;
}

typedef int (*evtActFun_t)(u16 connHandle, blt_ascss_ase_state_t *pAse, u8 *pData, aseCtrlNtfPayload_t *rsp);

typedef struct
{
    u8          aseType;
    u8          curState;
    u8          event;
    u8          nextState;
    evtActFun_t cb;
} ascsFsmTable_t;

#define ASCS_FSM_LIST(aseType, state, event, nextState, cb)  {aseType, state, event, nextState, cb}

#define ASCSS_FSM_LIST(aseType, state, event, nextState, cb) {aseType, BLT_ASCS_ASE_STATE_##state, BLT_ASCS_OPCODE_CONFIG_##event, BLT_ASCS_ASE_STATE_##nextState, blt_ascss_deal##cb}
#define ASCSS_SINK_FSM(state, event, nextState, cb)          ASCSS_FSM_LIST(BLT_ASE_DIRECTION_SINK, state, event, nextState, cb)
#define ASCSS_SRC_FSM(state, event, nextState, cb)           ASCSS_FSM_LIST(BLT_ASE_DIRECTION_SRC, state, event, nextState, cb)
#define ASCSS_ALL_FSM(state, event, nextState, cb)           ASCSS_FSM_LIST(BLT_ASE_DIRECTION_BIDIR, state, event, nextState, cb)


static const ascsFsmTable_t ascssFsmTb[] = {
#if 0
    ASCSS_ALL_FSM(IDLE, CODEC, CODEC_CFG, CfgCodec),

    ASCSS_ALL_FSM(CODEC_CFG, CODEC, CODEC_CFG, CfgCodec),
    ASCSS_ALL_FSM(CODEC_CFG, RELEASE, RELEASING, Release),
    ASCSS_ALL_FSM(CODEC_CFG, QOS, QOS_CFG, CfgQos),

    ASCSS_ALL_FSM(QOS_CFG, CODEC, CODEC_CFG, CfgCodec),
    ASCSS_ALL_FSM(QOS_CFG, QOS, QOS_CFG, CfgQos),
    ASCSS_ALL_FSM(QOS_CFG, RELEASE, RELEASING, Release),
    ASCSS_ALL_FSM(QOS_CFG, ENABLE, ENABLING, Enable),

    ASCSS_ALL_FSM(ENABLING, RELEASE, RELEASING, Release),
    ASCSS_ALL_FSM(ENABLING, UPDATE_METADATA, ENABLING, UpdateMetadata),
    ASCSS_SRC_FSM(ENABLING, DISABLE, DISABLING, Disable),
    ASCSS_SINK_FSM(ENABLING, DISABLE, QOS_CFG, Disable),
    ASCSS_ALL_FSM(ENABLING, RECV_START, STREAMING, RecvStartReady),

    ASCSS_ALL_FSM(STREAMING, UPDATE_METADATA, STREAMING, UpdateMetadata),
    ASCSS_SRC_FSM(STREAMING, DISABLE, DISABLING, Disable),
    ASCSS_SINK_FSM(STREAMING, DISABLE, QOS_CFG, Disable),
    ASCSS_ALL_FSM(STREAMING, RELEASE, RELEASING, Release),

    ASCSS_SRC_FSM(DISABLING, RECV_STOP, QOS_CFG, RecvStopReady),
    ASCSS_SRC_FSM(DISABLING, RELEASE, RELEASING, Release),

#else
    {BLT_ASE_DIRECTION_BIDIR, BLT_ASCS_ASE_STATE_IDLE,      BLT_ASCS_OPCODE_CONFIG_CODEC,           BLT_ASCS_ASE_STATE_CODEC_CFG, blt_ascss_dealCfgCodec      },

    {BLT_ASE_DIRECTION_BIDIR, BLT_ASCS_ASE_STATE_CODEC_CFG, BLT_ASCS_OPCODE_CONFIG_CODEC,           BLT_ASCS_ASE_STATE_CODEC_CFG, blt_ascss_dealCfgCodec      },
    {BLT_ASE_DIRECTION_BIDIR, BLT_ASCS_ASE_STATE_CODEC_CFG, BLT_ASCS_OPCODE_CONFIG_RELEASE,         BLT_ASCS_ASE_STATE_RELEASING, blt_ascss_dealRelease       },
    {BLT_ASE_DIRECTION_BIDIR, BLT_ASCS_ASE_STATE_CODEC_CFG, BLT_ASCS_OPCODE_CONFIG_QOS,             BLT_ASCS_ASE_STATE_QOS_CFG,   blt_ascss_dealCfgQos        },

    {BLT_ASE_DIRECTION_BIDIR, BLT_ASCS_ASE_STATE_QOS_CFG,   BLT_ASCS_OPCODE_CONFIG_CODEC,           BLT_ASCS_ASE_STATE_CODEC_CFG, blt_ascss_dealCfgCodec      },
    {BLT_ASE_DIRECTION_BIDIR, BLT_ASCS_ASE_STATE_QOS_CFG,   BLT_ASCS_OPCODE_CONFIG_QOS,             BLT_ASCS_ASE_STATE_QOS_CFG,   blt_ascss_dealCfgQos        },
    {BLT_ASE_DIRECTION_BIDIR, BLT_ASCS_ASE_STATE_QOS_CFG,   BLT_ASCS_OPCODE_CONFIG_RELEASE,         BLT_ASCS_ASE_STATE_RELEASING, blt_ascss_dealRelease       },
    {BLT_ASE_DIRECTION_BIDIR, BLT_ASCS_ASE_STATE_QOS_CFG,   BLT_ASCS_OPCODE_CONFIG_ENABLE,          BLT_ASCS_ASE_STATE_ENABLING,  blt_ascss_dealEnable        },

    {BLT_ASE_DIRECTION_BIDIR, BLT_ASCS_ASE_STATE_ENABLING,  BLT_ASCS_OPCODE_CONFIG_RELEASE,         BLT_ASCS_ASE_STATE_RELEASING, blt_ascss_dealRelease       },
    {BLT_ASE_DIRECTION_BIDIR, BLT_ASCS_ASE_STATE_ENABLING,  BLT_ASCS_OPCODE_CONFIG_UPDATE_METADATA, BLT_ASCS_ASE_STATE_ENABLING,  blt_ascss_dealUpdateMetadata},
    {BLT_ASE_DIRECTION_SRC,   BLT_ASCS_ASE_STATE_ENABLING,  BLT_ASCS_OPCODE_CONFIG_DISABLE,         BLT_ASCS_ASE_STATE_DISABLING, blt_ascss_dealDisable       },
    {BLT_ASE_DIRECTION_SINK,  BLT_ASCS_ASE_STATE_ENABLING,  BLT_ASCS_OPCODE_CONFIG_DISABLE,         BLT_ASCS_ASE_STATE_QOS_CFG,   blt_ascss_dealDisable       },
    {BLT_ASE_DIRECTION_BIDIR, BLT_ASCS_ASE_STATE_ENABLING,  BLT_ASCS_OPCODE_CONFIG_RECV_START,      BLT_ASCS_ASE_STATE_STREAMING, blt_ascss_dealRecvStartReady},

    {BLT_ASE_DIRECTION_BIDIR, BLT_ASCS_ASE_STATE_STREAMING, BLT_ASCS_OPCODE_CONFIG_UPDATE_METADATA, BLT_ASCS_ASE_STATE_STREAMING, blt_ascss_dealUpdateMetadata},
    {BLT_ASE_DIRECTION_SRC,   BLT_ASCS_ASE_STATE_STREAMING, BLT_ASCS_OPCODE_CONFIG_DISABLE,         BLT_ASCS_ASE_STATE_DISABLING, blt_ascss_dealDisable       },
    {BLT_ASE_DIRECTION_SINK,  BLT_ASCS_ASE_STATE_STREAMING, BLT_ASCS_OPCODE_CONFIG_DISABLE,         BLT_ASCS_ASE_STATE_QOS_CFG,   blt_ascss_dealDisable       },
    {BLT_ASE_DIRECTION_BIDIR, BLT_ASCS_ASE_STATE_STREAMING, BLT_ASCS_OPCODE_CONFIG_RELEASE,         BLT_ASCS_ASE_STATE_RELEASING, blt_ascss_dealRelease       },

    {BLT_ASE_DIRECTION_SRC,   BLT_ASCS_ASE_STATE_DISABLING, BLT_ASCS_OPCODE_CONFIG_RECV_STOP,       BLT_ASCS_ASE_STATE_QOS_CFG,   blt_ascss_dealRecvStopReady },
    {BLT_ASE_DIRECTION_SRC,   BLT_ASCS_ASE_STATE_DISABLING, BLT_ASCS_OPCODE_CONFIG_RELEASE,         BLT_ASCS_ASE_STATE_RELEASING, blt_ascss_dealRelease       },

#endif
};

static void blt_ascss_dealSingleOpcode(u16 connHandle, u8 opcode, u8 *pData, aseCtrlNtfPayload_t *rsp)
{
    u8                     aseID = pData[0];
    blt_ascss_ase_state_t *pAse  = blt_ascss_findAseState(connHandle, aseID);
    rsp->aseID                   = aseID;
    if (pAse == NULL) {
        rsp->responseCode = BLT_ASCS_RSP_CODE_INVALID_ASE_ID;
        rsp->reason       = 0x00;
        return;
    }


    for (size_t i = 0; i < ARRAY_SIZE(ascssFsmTb); i++) {
        if (opcode == ascssFsmTb[i].event && pAse->state == ascssFsmTb[i].curState && (pAse->dir & ascssFsmTb[i].aseType)) {
            if (ascssFsmTb[i].cb(connHandle, pAse, pData, rsp) == AUDIO_ESUCC) {
                pAse->state       = ascssFsmTb[i].nextState;
                rsp->responseCode = BLT_ASCS_RSP_CODE_SUCCESS;
                rsp->reason       = 0x00;
                pAse->notifFlag   = 0x01;
            }
            return;
        }

        if (opcode == ascssFsmTb[i].event) {
            BLT_ASCS_LOG("INF: ASCS write CP Deal[op%d]: aseState[%d] fsmState[%d] aseDir[%d] fsmAseType[%d]", opcode, pAse->state, ascssFsmTb[i].curState, pAse->dir, ascssFsmTb[i].aseType);
        }
    }
    rsp->responseCode = BLT_ASCS_RSP_CODE_INVALID_ASE_STATE;
    rsp->reason       = 0x00;
}

static const aseCtrlLega_t aseCtrlLega[] = {
    {BLT_ASCS_OPCODE_CONFIG_CODEC,           8,  1},
    {BLT_ASCS_OPCODE_CONFIG_QOS,             16, 0},
    {BLT_ASCS_OPCODE_CONFIG_ENABLE,          1,  1},
    {BLT_ASCS_OPCODE_CONFIG_RECV_START,      1,  0},
    {BLT_ASCS_OPCODE_CONFIG_DISABLE,         1,  0},
    {BLT_ASCS_OPCODE_CONFIG_RECV_STOP,       1,  0},
    {BLT_ASCS_OPCODE_CONFIG_UPDATE_METADATA, 1,  1},
    {BLT_ASCS_OPCODE_CONFIG_RELEASE,         1,  0},
};

static void blt_ascss_recvAseCtrl(u16 connHandle, u8 opcode, u8 aseNum, u8 *pData, u16 dataLen)
{
    if (aseNum == 0 || aseNum > STACK_AUDIO_ASCSS_MAX_ASE_CNT) {
        blt_ascss_pushAseCtrlNtfErr(connHandle, opcode, BLT_ASCS_RSP_CODE_INVALID_LENGTH, 0);
        return;
    }
    u8     fixSize = 0, variableSize = 0;
    size_t i = 0;
    for (i = 0; i < ARRAY_SIZE(aseCtrlLega); i++) {
        if (aseCtrlLega[i].opcode == opcode) {
            fixSize      = aseCtrlLega[i].fixSize;
            variableSize = aseCtrlLega[i].variableSize;
            break;
        }
    }

    if (i == ARRAY_SIZE(aseCtrlLega)) {
        BLT_ASCS_LOG("ERR: ASCS ASE ctrl Invalid opcode, %s", hex_to_str(pData, dataLen));
        blt_ascss_pushAseCtrlNtfErr(connHandle, opcode, BLT_ASCS_RSP_CODE_UNSUPPORTED_OPCODE, 0);
        return;
    }

    u8                   rspState[2 + 3 * STACK_AUDIO_ASCSS_MAX_ASE_CNT] = {opcode, aseNum};
    aseCtrlNtfPayload_t *pRsp                                            = (aseCtrlNtfPayload_t *)&rspState[2];
    for (i = 0; i < aseNum; i++) {
        int packSize = fixSize + (variableSize ? pData[fixSize] + 1 : 0); //variable must add length.
        if (dataLen < packSize) {
            BLT_ASCS_LOG("ERR: ASCS ASE ctrl invalid package, %s", hex_to_str(pData, dataLen));
            blt_ascss_pushAseCtrlNtfErr(connHandle, opcode, BLT_ASCS_RSP_CODE_INVALID_LENGTH, 0);
            return;
        }
        BLT_ASCS_LOG("ASCS write CP, %s", hex_to_str(pData, dataLen));
        blt_ascss_dealSingleOpcode(connHandle, opcode, pData, pRsp);
        dataLen -= packSize;
        pData += packSize;
        pRsp++; //next rsp
    }

    blt_ascss_pushAseCtrlNtf(connHandle, (blt_ascs_aseCtrlPointCharNtf_t *)rspState);
    blt_ascss_ntfAllAseState(connHandle);
}

void blt_ascss_ntfAllAseState(u16 connHandle)
{
    blc_ascs_server_t *ascss = blt_ascss_getCtrl(connHandle);
    for (int i = 0; i < ascss->aseCnt; i++) {
        if (ascss->aseState[i]->notifFlag) {
            if (ascss->aseState[i]->state == BLT_ASCS_ASE_STATE_IDLE) {
                blc_gatts_notifyValue(connHandle, ascss->aseState[i]->aseHandle, &ascss->aseState[i]->aseID, 2);
            } else if (ascss->aseState[i]->state == BLT_ASCS_ASE_STATE_CODEC_CFG) {
                ascss->aseState[i]->codecState.aseID    = ascss->aseState[i]->aseID;
                ascss->aseState[i]->codecState.aseState = ascss->aseState[i]->state;
                blc_ascss_initAseParam(ascss->aseState[i]);
                blc_gatts_notifyValue(connHandle, ascss->aseState[i]->aseHandle, &ascss->aseState[i]->codecState.aseID, 25 + ascss->aseState[i]->codecState.codecSpecCfgLen);

                blc_audio_codecSpecCfgParsed_t specCfgParam = {0};
                blt_audio_getCodecSpecCfgParam(&ascss->aseState[i]->codecState.codecSpecCfgLen, &specCfgParam);
                blt_audio_unicastSvrCodecCfgEvt(connHandle, ascss->aseState[i], &specCfgParam);
            } else if (ascss->aseState[i]->state == BLT_ASCS_ASE_STATE_QOS_CFG) {
                ascss->aseState[i]->QosState.aseID    = ascss->aseState[i]->aseID;
                ascss->aseState[i]->QosState.aseState = ascss->aseState[i]->state;
                blc_gatts_notifyValue(connHandle, ascss->aseState[i]->aseHandle, &ascss->aseState[i]->QosState.aseID, sizeof(blt_ascss_aseStateQosCfg_t));
                blt_audio_unicastSvrQosCfgEvt(connHandle, ascss->aseState[i]);
            } else if (ascss->aseState[i]->state == BLT_ASCS_ASE_STATE_ENABLING) {
                ascss->aseState[i]->otherState.aseID    = ascss->aseState[i]->aseID;
                ascss->aseState[i]->otherState.aseState = ascss->aseState[i]->state;
                ascss->aseState[i]->otherState.cigID    = ascss->aseState[i]->QosState.cigID;
                ascss->aseState[i]->otherState.cisID    = ascss->aseState[i]->QosState.cisID;
                blc_gatts_notifyValue(connHandle, ascss->aseState[i]->aseHandle, &ascss->aseState[i]->otherState.aseID, 5 + ascss->aseState[i]->otherState.metadataLen);
                blt_audio_unicastSvrEnablingEvt(connHandle, ascss->aseState[i]);
            } else if (ascss->aseState[i]->state == BLT_ASCS_ASE_STATE_STREAMING) {
                ascss->aseState[i]->streamingState.aseID       = ascss->aseState[i]->aseID;
                ascss->aseState[i]->streamingState.aseState    = ascss->aseState[i]->state;
                ascss->aseState[i]->streamingState.cigID       = ascss->aseState[i]->QosState.cigID;
                ascss->aseState[i]->streamingState.cisID       = ascss->aseState[i]->QosState.cisID;
                ascss->aseState[i]->streamingState.metadataLen = ascss->aseState[i]->otherState.metadataLen;
                memcpy(ascss->aseState[i]->streamingState.metadata, ascss->aseState[i]->otherState.metadata, ascss->aseState[i]->otherState.metadataLen);
                blc_gatts_notifyValue(connHandle, ascss->aseState[i]->aseHandle, &ascss->aseState[i]->streamingState.aseID, 5 + ascss->aseState[i]->streamingState.metadataLen);
                if (ascss->aseState[i]->dir == AUDIO_DIR_SINK) {
                    blt_audio_unicastSvrRcvStreamEvt(connHandle, ascss->aseState[i]);
                } else if (ascss->aseState[i]->dir == AUDIO_DIR_SOURCE) {
                    blt_audio_unicastSvrSendStreamEvt(connHandle, ascss->aseState[i]);
                }
            } else if (ascss->aseState[i]->state == BLT_ASCS_ASE_STATE_DISABLING) {
                ascss->aseState[i]->disablingState.aseID       = ascss->aseState[i]->aseID;
                ascss->aseState[i]->disablingState.aseState    = ascss->aseState[i]->state;
                ascss->aseState[i]->disablingState.cigID       = ascss->aseState[i]->QosState.cigID;
                ascss->aseState[i]->disablingState.cisID       = ascss->aseState[i]->QosState.cisID;
                ascss->aseState[i]->disablingState.metadataLen = ascss->aseState[i]->otherState.metadataLen;
                memcpy(ascss->aseState[i]->disablingState.metadata, ascss->aseState[i]->otherState.metadata, ascss->aseState[i]->otherState.metadataLen);
                blc_gatts_notifyValue(connHandle, ascss->aseState[i]->aseHandle, &ascss->aseState[i]->disablingState.aseID, 5 + ascss->aseState[i]->disablingState.metadataLen);
                blt_audio_unicastSvrDisablingEvt(connHandle, ascss->aseState[i]);
            } else if (ascss->aseState[i]->state == BLT_ASCS_ASE_STATE_RELEASING) {
                blc_gatts_notifyValue(connHandle, ascss->aseState[i]->aseHandle, &ascss->aseState[i]->aseID, 2);
                blt_audio_unicastSvrReleasingEvt(connHandle, ascss->aseState[i]);
            }
        }
        ascss->aseState[i]->notifFlag = 0;
    }
}

static int blt_ascss_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8 *writeValue, u16 valueLen)
{
    (void)opcode;

    if (attrHandle != ASCSS_ASE_CTRL_HANDLE(connHandle)) {
        return ATT_ERR_INVALID_HANDLE;
    }
    if (valueLen < 2) {
        return ATT_ERR_INVALID_PDU;
    }
    blt_ascss_recvAseCtrl(connHandle, writeValue[0], writeValue[1], writeValue + 2, valueLen - 2);
    return ATT_SUCCESS;
}

/******************ascs server init all characteristic handle*************************/

static void blt_ascss_initSinkAse(atts_foundCharParam_t *p, void *input)
{
    blc_ascs_server_t *ascss = (blc_ascs_server_t *)input;
    if (p->num >= gAscssSinkAseCnt) {
        BLT_ASCS_LOG("ERR: Sink ASE char too many, max num is: %d", gAscssSrcAseCnt);
    } else {
        blt_ascss_ase_state_t *ase = ascss->aseState[ascss->aseCnt];
        ascss->aseCnt++;
        ase->aseHandle = p->charHandle;
        ase->aseID     = p->charData[0];
        ase->state     = BLT_ASCS_ASE_STATE_IDLE;
        ase->dir       = BLT_ASE_DIRECTION_SINK;
    }
}

static void blt_ascss_initSrcAse(atts_foundCharParam_t *p, void *input)
{
    blc_ascs_server_t *ascss = (blc_ascs_server_t *)input;
    if (p->num >= gAscssSrcAseCnt) {
        BLT_ASCS_LOG("ERR: Source ASE char too many, max num is: %d", gAscssSrcAseCnt);
    } else {
        blt_ascss_ase_state_t *ase = ascss->aseState[ascss->aseCnt];
        ascss->aseCnt++;
        ase->aseHandle = p->charHandle;
        ase->aseID     = p->charData[0];
        ase->state     = BLT_ASCS_ASE_STATE_IDLE;
        ase->dir       = BLT_ASE_DIRECTION_SRC;
    }
}

static void blt_ascss_initAseControlPoint(atts_foundCharParam_t *p, void *input)
{
    blc_ascs_server_t *ascss = (blc_ascs_server_t *)input;
    if (p->num) {
        BLT_ASCS_LOG("ERR: ASE Control Point char too many");
        return;
    }
    ascss->aseCtrlHandle = p->charHandle;
}

static const atts_findCharList_t ascssChar[] = {
    {
     .charUuid    = characteristicSinkAseUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_ascss_initSinkAse,
     },
    {
     .charUuid    = characteristicSourceAseUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_ascss_initSrcAse,
     },
    {
     .charUuid    = characteristicAseControlPointUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_ascss_initAseControlPoint,
     },
};

static void blt_ascss_serviceInit(blc_ascs_server_t *server)
{
    blc_atts_findCharacteristicByServiceUuid(serviceAudioStreamControlUuid, ATT_16_UUID_LEN, ascssChar, ARRAY_SIZE(ascssChar), server);
}

/****************ascs server init all characteristic handle end***********************/
