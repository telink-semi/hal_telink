/********************************************************************************************************
 * @file    ascs_client.c
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

static const blc_gapc_discList_t discAscs;
#define BLC_ASCS_START_SDP(connHandle)      blc_gapc_registerDiscoveryService(connHandle, &discAscs)

static const blc_gapc_reconnList_t reconnAscs;
#define BLC_ASCS_START_RECONN(connHandle)       blc_gapc_registerReconnectService(connHandle, &reconnAscs)

static void blt_ascsc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);
bool blt_ascsc_rcvAseStateNtfDeal(blc_ascs_client_t *pAscsClt, u16 ntfHandle, u8 *pNtfVal, u16 ntfValLen);

_attribute_ble_data_retention_
blc_ascs_client_ctrl_t ascs_client_ctrl = {
    .process = {
        .pNext = NULL,
        .id = AUDIO_ASCS_CLIENT,
        .usedAclRole = 0,
        .init = blt_ascsc_init,
        .connect = blt_ascsc_connect,
        .discov = blt_ascsc_discovery,
        .loop = blt_ascsc_loop,
        .store = blt_ascsc_nv_store,
    },
};


u8 cig_pack_format = 1;//default PACK_INTERLEAVED

void blc_ascss_setCigPackingType(packing_type_t type)
{
    cig_pack_format = type;
}

void blc_audio_registerASCSControlClient(const blc_ascsc_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_CENTRAL, (blc_prf_proc_t*)&ascs_client_ctrl, param);
}

int blt_ascsc_init(u8 initType, const void* param)
{
#if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_ascs_client_ctrl_t)), gAscscCtrl);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blt_ascsc_ase_t)), gAse);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_ascs_client_t)), blc_ascs_client_t);
#endif
    (void)param;

    if(initType == PRF_PROC_INIT) {

        for (int i = 0; i < gAppAudioAclCentralNum; i++) {
            ascs_client_ctrl.pAscsClient[i] = blt_ascsc_getClientBuf(i);
            /* Clear ASCS Client parameters  */
            memset(ascs_client_ctrl.pAscsClient[i], 0, sizeof(blc_ascs_client_t));
            /* Initialize Pointer buffer */
            for (int j = 0; j < gAppAscsCltSinkAseNum; j++) {
                ascs_client_ctrl.pAscsClient[i]->pSinkAse[j] = blt_ascsc_getSinkAseBuf(i, j);
                ascs_client_ctrl.pAscsClient[i]->pSinkAse[j]->dir = AUDIO_DIR_SINK;
                ascs_client_ctrl.pAscsClient[i]->pSinkAse[j]->pMetadata = blt_ascsc_getMetadataBuf(i, j);
            }
            for (int k = 0; k < gAppAscsCltSrcAseNum; k++) {
                ascs_client_ctrl.pAscsClient[i]->pSrcAse[k] = blt_ascsc_getSrcAseBuf(i, k);
                ascs_client_ctrl.pAscsClient[i]->pSrcAse[k]->dir = AUDIO_DIR_SOURCE;
                ascs_client_ctrl.pAscsClient[i]->pSrcAse[k]->pMetadata = blt_ascsc_getMetadataBuf(i, gAppAscsCltSinkAseNum+k);
            }
        }
    } else if (initType == PRF_PROC_DEINIT) {
    }
    return 0;
}


int blt_ascsc_connect(u16 connHandle, prf_acl_state_enum connState)
{
    blc_ascs_client_t *client = blt_ascsc_getClientInst(connHandle);

    if(connState == PRF_ACL_STATE_DISCONN)
    {
        BLT_ASCS_LOG("Disconnect: 0x%x", connHandle);
        //TODO: clear pending variable
        /* Clear PACS Client parameters  */
        client->connHandle = 0;
        client->sinkAseIdx = 0;
        client->srcAseIdx = 0;

        for(u8 i=0;i<client->sinkAseNum;i++)
        {
            client->pSinkAse[i]->flags = 0;
        }
        for(u8 i=0;i<client->srcAseNum;i++)
        {
            client->pSrcAse[i]->flags = 0;
        }
    }
    else
    {
        BLT_ASCS_LOG("Connect: 0x%x", connHandle);
        /* connected mark */
        client->connHandle = connHandle;
        //Clear
        client->aseFsmBusy = 0;
        client->sinkAseNum = 0;
        client->srcAseNum = 0;
        client->aseCount = 0;
    }

    return 0;
}

int blt_ascsc_discovery(u16 connHandle)
{
    BLC_COMMON_SDP_DISCOVERY(connHandle, ASCS, ascs);
}

int blt_ascsc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t* param)
{
    BLC_COMMON_NV_STORE(connHandle, ASCS, ascs, srcAseHdl[STACK_AUDIO_ASCS_ASE_SNK_NUM-1]);

    if(nvState == PRF_NV_STATE_STORE)
    {
        pNvInfo->sinkAseCnt = client->sinkAseNum;
        pNvInfo->srcAseCnt = client->srcAseNum;
    }
    else if(nvState == PRF_NV_STATE_LOAD)
    {
        client->sinkAseNum = pNvInfo->sinkAseCnt;
        client->srcAseNum = pNvInfo->srcAseCnt;
        client->aseCount = client->sinkAseNum + client->srcAseNum;
    }

    return 0;
}

int blt_ascsc_loop(u16 connHandle)
{
    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(connHandle);

    if(!pAscsClt->aseFsmBusy) return 0;

    /////////////////////  old process /////////////////////////////
    extern void blt_audio_ascpProcess(blc_ascs_client_t *pAscsClt);
    blt_audio_ascpProcess(pAscsClt);

    return 0;
}

blc_ascs_client_t *blt_ascsc_getClientInst(u16 connHandle)
{
    int ret = blt_prf_getAclRole(connHandle);
    if(ret < 0 || ret == ACL_ROLE_PERIPHERAL){
        BLT_ASCS_LOG("ERR: ACL role, unlikely: 0x%x", ret);

        if(ret >= 0){
            /* BAP Unicast Client GAP Central */
            blt_prf_sendSvrGapRoleErrEvt(connHandle, AUDIO_ASCS_CLIENT, ret);
        }

        return NULL;
    }

    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle
    return ascs_client_ctrl.pAscsClient[idx];
}


#if (0)
static void blt_ascsc_aseSetStatus(blt_ascsc_ase_t *pAse, blt_ascsc_aseState_t *pAseStatus, u16 aseStateLen)
{
    if(!pAse || !pAseStatus) return;

    u16 totalLen;
    u8 lastAseState = pAse->state;
    u8 currAseState = pAseStatus->aseState;


    BLT_ASCS_LOG(" ASE ID: %d",  pAse->aseID);
    BLT_ASCS_LOG(" ASE state: %d", pAse->state);

    #if(DBG_ASCS_LOG)
    char dbgAseStateStr[7][10] = {
        "IDLE     ", "CODEC_CFG", "QOS_CFG  ", "ENABLING ", "STREAMING", "DISABLING", "RELEASING",
    };
    BLT_ASCS_LOG(dbgAseStateStr[currAseState]);
    #endif

    switch (currAseState) {
        case BLT_ASCS_ASE_STATE_IDLE:
            /* Idle state: Additional_ASE_Parameters: Empty (zero length) */
            totalLen = sizeof(blt_ascsc_aseState_t);
            if (totalLen != aseStateLen) {
                BLT_ASCS_LOG("Invalid ASE state length");
                return;
            }
            /* TODO: streaming release, Notify Upper Layer */
            pAse->aseID = pAseStatus->aseID;
            pAse->state = pAseStatus->aseState;
            pAse->flags = 0;
            BLT_ASCS_LOG("Server's ASE state => IDLE");
        break;
        case BLT_ASCS_ASE_STATE_CODEC_CFG:
            /* ASE send process ended */
//          pAse->flags &= ~BLT_AUDIOC_ASE_FLAG_SEND_WAIT;

            switch (lastAseState) {
                case BLT_ASCS_ASE_STATE_IDLE:
                case BLT_ASCS_ASE_STATE_CODEC_CFG:
                case BLT_ASCS_ASE_STATE_QOS_CFG:
                case BLT_ASCS_ASE_STATE_RELEASING:
                    break;
                default:
                    BLT_ASCS_LOG("Invalid ASE state machine transitions");
                    return;
            }

            /* ASE Read OR ASE Notify process common */
            blt_ascsc_aseStateCodecCfg_t *pAseStateCodecCfg = (blt_ascsc_aseStateCodecCfg_t*)pAseStatus;
            totalLen = pAseStateCodecCfg->codecSpecCfgLen + OFFSETOF(blt_ascsc_aseStateCodecCfg_t, codecSpecCfg);
            if (totalLen != aseStateLen) {
                BLT_ASCS_LOG("Invalid ASE state length");
                return;
            }

            BLT_ASCS_LOG("Server's ASE state => CODEC_CFG: %d",  lastAseState);

            if (memcmp(&pAse->codecId, &pAseStateCodecCfg->codecId, sizeof(blc_audio_codec_id_t))) {
                BLT_ASCS_LOG("Codec configuration mismatched");
                return;
            }

            blc_audio_codecSpecCfgParsed_t codecSpecParam = {0};
            u8 specCfgSts = blt_audio_getCodecSpecCfgParam(&pAseStateCodecCfg->codecSpecCfgLen, &codecSpecParam);
            if (specCfgSts != AUDIO_ESUCC) {
                BLT_ASCS_LOG("Get codec cfg Error: %d", specCfgSts);
                return;
            }

            if(codecSpecParam.frequency != pAse->frequency || codecSpecParam.duration != pAse->duration || \
               codecSpecParam.allocation != pAse->location|| codecSpecParam.frameOcts != pAse->frameOcts|| \
               codecSpecParam.codecFrameBlksPerSDU!=pAse->codecFrmBlksPerSDU) {
                BLT_ASCS_LOG("codec cfg changed");
                return;
            }

//          pAse->qosPref.unframedSupp = pAseStateCodecCfg->framing == 0; /*<! Unframed support flag      */
//          pAse->qosPref.phy = pAseStateCodecCfg->prefPHY;  /*<! Preferred PHY              */
//          pAse->qosPref.rtn = pAseStateCodecCfg->prefRetransmitNum; /*<! Range: 0x00 - 0xFF         */
//          pAse->qosPref.latency = pAseStateCodecCfg->maxTransportLatency; /*<! Unit: ms, Range: 0x0005-0x0FA0 */
//          pAse->qosPref.pd_min = bstream_to_u24_le(pAseStateCodecCfg->presentationDelayMin); /** @brief Minimum Presentation Delay */
//          pAse->qosPref.pd_max = bstream_to_u24_le(pAseStateCodecCfg->presentationDelayMax);/** @brief Maximum Presentation Delay */
//          pAse->qosPref.pref_pd_min = bstream_to_u24_le(pAseStateCodecCfg->prefPresentationDelayMin);/** @brief Preferred minimum Presentation Delay */
//          pAse->qosPref.pref_pd_max = bstream_to_u24_le(pAseStateCodecCfg->prefPresentationDelayMax);/** @brief Preferred maximum Presentation Delay  */

            //pAse->flags = BLT_AUDIOC_ASE_FLAG_SEND_QOS;
//          pAse->ready |= BLT_AUDIO_ASE_CODEC_READY;
//          pAse->state = pAseStatus->aseState;

            break;
        case BLT_ASCS_ASE_STATE_QOS_CFG:
            /* ASE send process ended */
//          pAse->flags &= ~BLT_AUDIOC_ASE_FLAG_SEND_WAIT;

            if (pAse->dir == AUDIO_DIR_SOURCE) {
                switch (lastAseState) {
                    case BLT_ASCS_ASE_STATE_CODEC_CFG:
                    case BLT_ASCS_ASE_STATE_QOS_CFG:
                    case BLT_ASCS_ASE_STATE_DISABLING:
                        break;
                    default:
                        BLT_ASCS_LOG("Invalid ASE state machine transitions");
                        return;
                }
            } else {
                switch (lastAseState) {
                    case BLT_ASCS_ASE_STATE_CODEC_CFG:
                    case BLT_ASCS_ASE_STATE_QOS_CFG:
                    case BLT_ASCS_ASE_STATE_ENABLING:
                    case BLT_ASCS_ASE_STATE_STREAMING:
                        break;
                    default:
                        BLT_ASCS_LOG("Invalid ASE state machine transitions");
                        return;
                }
            }

            /* ASE Read OR ASE Notify process common */
            blt_ascsc_aseStateQosCfg_t *pAseStateQosCfg = (blt_ascsc_aseStateQosCfg_t*)pAseStatus;
            totalLen = sizeof(blt_ascsc_aseStateQosCfg_t);
            if (totalLen != aseStateLen) {
                BLT_ASCS_LOG("Invalid ASE state length");
                return;
            }

            (void)pAseStateQosCfg;

            BLT_ASCS_LOG("Server's ASE state => QOS_CFG: %d", lastAseState);

            pAse->state = pAseStatus->aseState;

            break;
        case BLT_ASCS_ASE_STATE_ENABLING:
            /* ASE send process ended */
//          pAse->flags &= ~BLT_AUDIOC_ASE_FLAG_SEND_WAIT;

            switch (lastAseState) {
                case BLT_ASCS_ASE_STATE_QOS_CFG:
                case BLT_ASCS_ASE_STATE_ENABLING:
                    break;
                default:
                    BLT_ASCS_LOG("Invalid ASE state machine transitions");
                    return;
            }

            /* ASE Read OR ASE Notify process common */
            blt_ascsc_aseStateEnable_t *pAseStateEnable = (blt_ascsc_aseStateEnable_t*)pAseStatus;
            totalLen = pAseStateEnable->metaDataLen + OFFSETOF(blt_ascsc_aseStateEnable_t, pMetaData);
            if (totalLen != aseStateLen) {
                BLT_ASCS_LOG("Invalid ASE state length");
                return;
            }

            pAse->aseID = pAseStatus->aseID;
            pAse->state = pAseStatus->aseState;
            BLT_ASCS_LOG("Server's ASE state => ENABLING: %d", lastAseState);
            break;
        case BLT_ASCS_ASE_STATE_STREAMING:
            switch (lastAseState) {
                case BLT_ASCS_ASE_STATE_ENABLING:
                case BLT_ASCS_ASE_STATE_STREAMING:
                    break;
                default:
                    BLT_ASCS_LOG("Invalid ASE state machine transitions");
                    return;
            }

            /* ASE Read OR ASE Notify process common */
            blt_ascsc_aseStateStream_t *pAseStateStream = (blt_ascsc_aseStateStream_t*)pAseStatus;
            totalLen = pAseStateStream->metaDataLen + OFFSETOF(blt_ascsc_aseStateStream_t, pMetaData);
            if (totalLen != aseStateLen) {
                BLT_ASCS_LOG("Invalid ASE state length");
                return;
            }

            pAse->aseID = pAseStatus->aseID;
            pAse->state = pAseStatus->aseState;
            BLT_ASCS_LOG("Server's ASE state => STREAMING: %d", lastAseState);
            break;
        case BLT_ASCS_ASE_STATE_DISABLING:
            if (pAse->dir == AUDIO_DIR_SOURCE) {
                switch (lastAseState) {
                    case BLT_ASCS_ASE_STATE_ENABLING:
                    case BLT_ASCS_ASE_STATE_STREAMING:
                        break;
                    default:
                        BLT_ASCS_LOG("Invalid ASE state machine transitions");
                        return;
                }
            } else {
                BLT_ASCS_LOG("Invalid ASE state machine transitions");
                return;
            }


            /* ASE Read OR ASE Notify process common */
            blt_ascsc_aseStateDisable_t *pAseStateDisable = (blt_ascsc_aseStateDisable_t*)pAseStatus;
            totalLen = pAseStateDisable->metaDataLen + OFFSETOF(blt_ascsc_aseStateDisable_t, pMetaData);
            if (totalLen != aseStateLen) {
                BLT_ASCS_LOG("Invalid ASE state length");
                return;
            }

            pAse->aseID = pAseStatus->aseID;
            pAse->state = pAseStatus->aseState;
            BLT_ASCS_LOG("Server's ASE state => DISABLING: %d", lastAseState);
            break;
        case BLT_ASCS_ASE_STATE_RELEASING:
            switch (lastAseState) {
                case BLT_ASCS_ASE_STATE_CODEC_CFG:
                case BLT_ASCS_ASE_STATE_QOS_CFG:
                case BLT_ASCS_ASE_STATE_ENABLING:
                case BLT_ASCS_ASE_STATE_STREAMING:
                    break;
                case BLT_ASCS_ASE_STATE_DISABLING:
                    if (pAse->dir == AUDIO_DIR_SOURCE) {
                        break;
                    } /* else fail for sink */
                default:
                    BLT_ASCS_LOG("Invalid ASE state machine transitions");
                    return;
            }

            /* Releasing state: Additional_ASE_Parameters: Empty (zero length) */
            totalLen = sizeof(blt_ascsc_aseState_t);
            if (totalLen != aseStateLen) {
                BLT_ASCS_LOG("Invalid ASE state length");
                return;
            }

            /* The Unicast Client shall terminate any CIS established for that ASE
             * by following the Connected Isochronous Stream Terminate procedure
             * defined in Volume 3, Part C, Section 9.3.15 in when the Unicast
             * Client has determined that the ASE is in the Releasing state.
             */

            pAse->aseID = pAseStatus->aseID;
            pAse->state = pAseStatus->aseState;
            BLT_ASCS_LOG("Server's ASE state => RELEASING: %d", lastAseState);
            break;
    }
}
#endif


/*************************************************************************
 *  GATTC Write Characteristics
 *  - CHARACTERISTIC_UUID_ASE_CONTROL_POINT
 *************************************************************************/
static int blt_ascsc_write(u16 connHandle, blt_ascsc_write_ase_t *pAseCfg, u16 aseCfgLen)
{
    assert(blt_ll_isAclhdlInvalid(connHandle) == BLE_SUCCESS);
    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(connHandle);
    BLT_ASCS_LOG("blt_ascc_writeCtrlPoint: %d", pAseCfg->opcode);

    if (!pAseCfg || !aseCfgLen || pAseCfg->opcode > BLT_ASCS_OPCODE_CONFIG_RELEASE) {
        BLT_ASCS_LOG("  ERR: ATT write opcode invalid: %d", pAseCfg->opcode);
        return AUDIO_ERR_INVALID_PARAMETER;
    } else if (!pAscsClt->aseCtrlPntHdl) {
        BLT_ASCS_LOG("  ERR: ATT write handle not set");
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    gattc_write_cfg_t wrCfg;
    gattc_write_cfg_t *pWrCfg = &wrCfg;

    u8 writeBuff[255+20+1];
    memcpy(writeBuff, (u8*)pAseCfg, aseCfgLen);

    pWrCfg->func = NULL;
    pWrCfg->handle = pAscsClt->aseCtrlPntHdl;
    pWrCfg->offset = 0;
    pWrCfg->data = writeBuff;
    pWrCfg->length = aseCfgLen;
    pWrCfg->withoutRsp = TRUE; /* write command (recommend, if write value length > ATT_MTU-1, use write long procedure ) */

    return blc_gattc_writeAttributeValue(connHandle, pWrCfg);
}

int blc_ascsc_writeConfigCodec(u16 connHandle, blt_ascsc_ase_t *pAseList[], u8 aseListCnt)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(connHandle);
    BLT_ASCS_LOG("blc_ascsc_writeConfigCodec");

    if (!pAscsClt->aseCtrlPntHdl) {
        BLT_ASCS_LOG("  ERR: ATT write handle not set");
        return AUDIO_ERR_DISCOVERY_FAILED;
    } else if (!pAseList || !aseListCnt) {
        return AUDIO_ERR_INVALID_PARAMETER;
    } else {
        for (int i = 0; i < aseListCnt; i++) {
            u8 state = pAseList[i]->state;
            switch (state) {
                /* the Config Codec operation for an ASE is valid only if the value of the ASE_State
                 * field is 0x00 (Idle), 0x01 (Codec Configured), or 0x02 (QoS Configured) */
                case BLT_ASCS_ASE_STATE_IDLE:
                case BLT_ASCS_ASE_STATE_CODEC_CFG:
                case BLT_ASCS_ASE_STATE_QOS_CFG:
                    break;
                default:
                    BLT_ASCS_LOG("  ERR: Invalid state: %d", state);
                    return AUDIO_ERR_STATUS;
            }
        }
    }

    u8 buf[sizeof(blt_ascsc_write_ase_t) + aseListCnt*sizeof(blt_ascsc_cfg_codec_t)];
    blt_ascsc_write_ase_t *pWrAse = (blt_ascsc_write_ase_t *)buf;
    /* <<ASCS_v1.0.pdf>> Page 29, Table 5.2: Config Codec operation format */
    pWrAse->opcode = BLT_ASCS_OPCODE_CONFIG_CODEC;
    pWrAse->numOfAses = aseListCnt;

    for (int i = 0; i < aseListCnt; i++) {
        blt_ascsc_cfg_codec_t *pCfgCodec = (blt_ascsc_cfg_codec_t*)(pWrAse->aseParam + i * sizeof(blt_ascsc_cfg_codec_t));
        pCfgCodec->aseID = pAseList[i]->aseID;
        pCfgCodec->tgtLatency = pAseList[i]->tgtLatency = 2; //TODO:
        pCfgCodec->tgtPhy = pAseList[i]->tgtPHY = 2;         //TODO:
        pCfgCodec->codecId = pAseList[i]->codecId;

        u8 *pCodecSpecCfg = pCfgCodec->codecSpecCfg;
        /* Sampling_Frequency: Selected codec sampling frequency */
        U8_TO_STREAM(pCodecSpecCfg, 2);
        U8_TO_STREAM(pCodecSpecCfg, BLC_AUDIO_CAPTYPE_CFG_SAMPLE_FREQUENCY);
        U8_TO_STREAM(pCodecSpecCfg, pAseList[i]->frequency);
        /* Frame_Duration: Use 7.5 ms OR 10 ms codec frames  */
        U8_TO_STREAM(pCodecSpecCfg, 2);
        U8_TO_STREAM(pCodecSpecCfg, BLC_AUDIO_CAPTYPE_CFG_FRAME_DURATION);
        U8_TO_STREAM(pCodecSpecCfg, pAseList[i]->duration);
        if(pAseList[i]->location){ //Optional
            pCfgCodec->codecSpecCfgLen = 19;
            /* Audio_Channel_Allocation: 4-octet bitfield of Audio Location values */
            U8_TO_STREAM(pCodecSpecCfg, 5);
            U8_TO_STREAM(pCodecSpecCfg, BLC_AUDIO_CAPTYPE_CFG_CHANNELS_ALLOCATION);
            U32_TO_STREAM(pCodecSpecCfg, pAseList[i]->location);
        }else{
            pCfgCodec->codecSpecCfgLen = 13;
        }
        /* Octets_Per_Codec_Frame: Number of octets used per codec frame */
        U8_TO_STREAM(pCodecSpecCfg, 3);
        U8_TO_STREAM(pCodecSpecCfg, BLC_AUDIO_CAPTYPE_CFG_OCTETS_PER_CODEC_FRAME);
        U16_TO_STREAM(pCodecSpecCfg, pAseList[i]->frameOcts);
        /* Codec_Frame_Blocks_Per_SDU: Number of blocks of codec frames per SDU */
        U8_TO_STREAM(pCodecSpecCfg, 2);
        U8_TO_STREAM(pCodecSpecCfg, BLC_AUDIO_CAPTYPE_CFG_CODEC_FRAME_BLCKS_PER_SDU);
        U8_TO_STREAM(pCodecSpecCfg, pAseList[i]->codecFrmBlksPerSDU);
    }

    return blt_ascsc_write(connHandle, pWrAse, sizeof(buf));
}

int blc_ascsc_writeQosConfig(u16 connHandle, blt_ascsc_ase_t *pAseList[], u8 aseListCnt)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(connHandle);
    BLT_ASCS_LOG("blc_ascsc_writeQosConfig");

    if (!pAscsClt->aseCtrlPntHdl) {
        BLT_ASCS_LOG("  ERR: ATT write handle not set");
        return AUDIO_ERR_DISCOVERY_FAILED;
    } else if (!pAseList || !aseListCnt) {
        return AUDIO_ERR_INVALID_PARAMETER;
    } else {
        for (int i = 0; i < aseListCnt; i++) {
            u8 state = pAseList[i]->state;
            switch (state) {
                /* the Config QoS operation for an ASE is valid only if the value of the ASE_State
                 * field is 0x01 (Codec Configured) or 0x02 (QoS Configured). */
                case BLT_ASCS_ASE_STATE_CODEC_CFG:
                case BLT_ASCS_ASE_STATE_QOS_CFG:
                    break;
                default:
                    BLT_ASCS_LOG("  ERR: Invalid state: %d", state);
                    return AUDIO_ERR_STATUS;
            }
        }
    }

    /* */
    u8 buf[sizeof(blt_ascsc_write_ase_t) + aseListCnt*sizeof(blt_ascsc_config_qos_t)];
    blt_ascsc_write_ase_t *pWrAse = (blt_ascsc_write_ase_t *)buf;
    /* <<ASCS_v1.0.pdf>> Page 31, Table 5.3: Config QoS operation format */
    pWrAse->opcode = BLT_ASCS_OPCODE_CONFIG_QOS;
    pWrAse->numOfAses = aseListCnt;

    for (int i = 0; i < aseListCnt; i++) {
        blt_ascsc_config_qos_t *pQosCfg = (blt_ascsc_config_qos_t*)(pWrAse->aseParam + i * sizeof(blt_ascsc_config_qos_t));
        pQosCfg->aseID = pAseList[i]->aseID;
        pQosCfg->cigID = pAseList[i]->cigID;
        pQosCfg->cisID = pAseList[i]->cisID;
        u24_to_bstream_le(pAseList[i]->sduInterval, pQosCfg->sduInterval);
        pQosCfg->framing = pAseList[i]->framing;
        pQosCfg->PHY = pAseList[i]->PHY;
        pQosCfg->maxSDU = pAseList[i]->maxSDU;
        pQosCfg->RTN = pAseList[i]->RTN;
        pQosCfg->maxTranLatency = pAseList[i]->maxTransLatency;
        u24_to_bstream_le(pAseList[i]->PresentationDly, pQosCfg->presentationDelay);
    }

    return blt_ascsc_write(connHandle, pWrAse, sizeof(buf));
}

int blc_ascsc_writeEnable(u16 connHandle, blt_ascsc_ase_t *pAseList[], u8 aseListCnt)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(connHandle);
    BLT_ASCS_LOG("blc_ascsc_writeEnable");
    u16 totalMetadataLen = 0;

    if (!pAscsClt->aseCtrlPntHdl) {
        BLT_ASCS_LOG("  ERR: ATT write handle not set");
        return AUDIO_ERR_DISCOVERY_FAILED;
    } else if (!pAseList || !aseListCnt) {
        return AUDIO_ERR_INVALID_PARAMETER;
    } else {
        for (int i = 0; i < aseListCnt; i++) {
            /* the Enable operation is valid for an ASE only if the value of the ASE_State field is
             * 0x02 (QoS Configured). */
            u8 state = pAseList[i]->state;
            if(state != BLT_ASCS_ASE_STATE_QOS_CFG) {
                BLT_ASCS_LOG("  ERR: Invalid state: %d", state);
                return AUDIO_ERR_STATUS;
            }
            totalMetadataLen += pAseList[i]->metadataLen;
        }
    }

    u8 buf[sizeof(blt_ascsc_write_ase_t) + totalMetadataLen + aseListCnt * sizeof(blt_ascsc_ase_enable_t)];
    blt_ascsc_write_ase_t *pWrAse = (blt_ascsc_write_ase_t *)buf;

    /* <<ASCS_v1.0.pdf>> Page 32, Table 5.4: Enable operation format */
    pWrAse->opcode = BLT_ASCS_OPCODE_CONFIG_ENABLE;
    pWrAse->numOfAses = aseListCnt;

    for (int i = 0; i < aseListCnt; i++) {
        blt_ascsc_ase_enable_t *pEnable = (blt_ascsc_ase_enable_t*)(pWrAse->aseParam + i * (pAseList[i]->metadataLen + sizeof(blt_ascsc_ase_enable_t)));
        pEnable->aseID = pAseList[i]->aseID;
        pEnable->metadataLen = pAseList[i]->metadataLen;
        memcpy(pEnable->metadataCfg, pAseList[i]->pMetadata, pAseList[i]->metadataLen);
    }

    return blt_ascsc_write(connHandle, pWrAse, sizeof(buf));
}

int blc_ascsc_writeRcvStartRdy(u16 connHandle, blt_ascsc_ase_t *pAseList[], u8 aseListCnt)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(connHandle);
    BLT_ASCS_LOG("blc_ascsc_writeRcvStartRdy");

    if (!pAscsClt->aseCtrlPntHdl) {
        BLT_ASCS_LOG("  ERR: ATT write handle not set");
        return AUDIO_ERR_DISCOVERY_FAILED;
    } else if (!pAseList || !aseListCnt) {
        return AUDIO_ERR_INVALID_PARAMETER;
    } else {
        for (int i = 0; i < aseListCnt; i++) {
            u8 state = pAseList[i]->state;
            if (state != BLT_ASCS_ASE_STATE_ENABLING || pAseList[i]->dir != AUDIO_DIR_SOURCE) {
                /* the Receiver Start Ready operation is valid for an ASE only if the value of the
                 * ASE_State field is 0x03 (Enabling) and if the device initiating the Receiver Start
                 * Ready operation is acting as Audio Sink for that ASE. */
                BLT_ASCS_LOG("  ERR: [RcvStartRdy]Invalid state OR dir: %d", state);
                return AUDIO_ERR_INVALID_PARAMETER;
            }
        }
    }

    u8 buf[sizeof(blt_ascsc_write_ase_t) + aseListCnt * sizeof(blt_ascsc_rcv_start_rdy_t)];
    blt_ascsc_write_ase_t *pWrAse = (blt_ascsc_write_ase_t *)buf;

    /* <<ASCS_v1.0.pdf>> Page 33, Table 5.5: Receiver Start Ready operation format */
    pWrAse->opcode = BLT_ASCS_OPCODE_CONFIG_RECV_START;
    pWrAse->numOfAses = aseListCnt;

    for (int i = 0; i < aseListCnt; i++) {
        blt_ascsc_rcv_start_rdy_t *pRcdStartRdy = (blt_ascsc_rcv_start_rdy_t*)(pWrAse->aseParam + i * sizeof(blt_ascsc_rcv_start_rdy_t));
        pRcdStartRdy->aseID = pAseList[i]->aseID;
    }

    return blt_ascsc_write(connHandle, pWrAse, sizeof(buf));
}

int blc_ascsc_writeDisable(u16 connHandle, blt_ascsc_ase_t *pAseList[], u8 aseListCnt)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(connHandle);
    BLT_ASCS_LOG("blc_ascsc_writeDisable");

    if (!pAscsClt->aseCtrlPntHdl) {
        BLT_ASCS_LOG("  ERR: ATT write handle not set");
        return AUDIO_ERR_DISCOVERY_FAILED;
    } else if (!pAseList || !aseListCnt) {
        return AUDIO_ERR_INVALID_PARAMETER;
    } else {
        for (int i = 0; i < aseListCnt; i++) {
            u8 state = pAseList[i]->state;
            switch (state) {
                /* the Disable operation is valid for an ASE only if the value of the ASE_State field
                 * is 0x03 (Enabling) or 0x04 (Streaming). */
                case BLT_ASCS_ASE_STATE_ENABLING:
                case BLT_ASCS_ASE_STATE_STREAMING:
                    break;
                default:
                    BLT_ASCS_LOG("  ERR: Invalid state: %d", state);
                    return AUDIO_ERR_STATUS;
            }
        }
    }

    u8 buf[sizeof(blt_ascsc_write_ase_t) + aseListCnt * sizeof(blt_ascsc_ase_disable_t)];
    blt_ascsc_write_ase_t *pWrAse = (blt_ascsc_write_ase_t *)buf;

    /* <<ASCS_v1.0.pdf>> Page 33, Table 5.6: Disable operation parameters */
    pWrAse->opcode = BLT_ASCS_OPCODE_CONFIG_DISABLE;
    pWrAse->numOfAses = aseListCnt;

    for (int i = 0; i < aseListCnt; i++) {
        blt_ascsc_ase_disable_t *pRcdStartRdy = (blt_ascsc_ase_disable_t*)(pWrAse->aseParam + i * sizeof(blt_ascsc_ase_disable_t));
        pRcdStartRdy->aseID = pAseList[i]->aseID;
    }

    return blt_ascsc_write(connHandle, pWrAse, sizeof(buf));
}

int blc_ascsc_writeRcvStopRdy(u16 connHandle, blt_ascsc_ase_t *pAseList[], u8 aseListCnt)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(connHandle);
    BLT_ASCS_LOG("blc_ascsc_writeRcvStopRdy");

    if (!pAscsClt->aseCtrlPntHdl) {
        BLT_ASCS_LOG("  ERR: ATT write handle not set");
        return AUDIO_ERR_DISCOVERY_FAILED;
    } else if (!pAseList || !aseListCnt) {
        return AUDIO_ERR_INVALID_PARAMETER;
    } else {
        for (int i = 0; i < aseListCnt; i++) {
            u8 state = pAseList[i]->state;
            if (state != BLT_ASCS_ASE_STATE_DISABLING || pAseList[i]->dir != AUDIO_DIR_SOURCE) {
                /* the Receiver Stop Ready operation is valid only for a Source ASE and only if the
                 * value of the ASE_State field is 0x05 (Disabling). */
                BLT_ASCS_LOG("  ERR: [RcvStopRdy]Invalid state OR dir: %d", state);
                return AUDIO_ERR_INVALID_PARAMETER;
            }
        }
    }

    u8 buf[sizeof(blt_ascsc_write_ase_t) + aseListCnt * sizeof(blt_ascsc_rcv_stop_rdy_t)];
    blt_ascsc_write_ase_t *pWrAse = (blt_ascsc_write_ase_t *)buf;

    /* <<ASCS_v1.0.pdf>> Page 34, Table 5.7: Receiver Stop Ready operation. */
    pWrAse->opcode = BLT_ASCS_OPCODE_CONFIG_RECV_STOP;
    pWrAse->numOfAses = aseListCnt;

    for (int i = 0; i < aseListCnt; i++) {
        blt_ascsc_rcv_stop_rdy_t *pRcdStopRdy = (blt_ascsc_rcv_stop_rdy_t*)(pWrAse->aseParam + i * sizeof(blt_ascsc_rcv_stop_rdy_t));
        pRcdStopRdy->aseID = pAseList[i]->aseID;
    }

    return blt_ascsc_write(connHandle, pWrAse, sizeof(buf));
}

int blc_ascsc_writeRelease(u16 connHandle, blt_ascsc_ase_t *pAseList[], u8 aseListCnt)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(connHandle);
    BLT_ASCS_LOG("blc_ascsc_writeRelease");

    if (!pAscsClt->aseCtrlPntHdl) {
        BLT_ASCS_LOG("  ERR: ATT write handle not set");
        return AUDIO_ERR_DISCOVERY_FAILED;
    } else if (!pAseList || !aseListCnt) {
        return AUDIO_ERR_INVALID_PARAMETER;
    } else {
        for (int i = 0; i < aseListCnt; i++) {
            u8 state = pAseList[i]->state;
            switch (state) {
            /* the Release operation is valid for an ASE only if the value of the ASE_State field
             * is 0x01 (Codec Configured), 0x02 (QoS Configured), 0x03 (Enabling), 0x04 (Streaming), or 0x05
             * (Disabling). */
                case BLT_ASCS_ASE_STATE_CODEC_CFG:
                case BLT_ASCS_ASE_STATE_QOS_CFG:
                case BLT_ASCS_ASE_STATE_ENABLING:
                case BLT_ASCS_ASE_STATE_STREAMING:
                case BLT_ASCS_ASE_STATE_DISABLING:
                    break;
                default:
                    BLT_ASCS_LOG("  ERR: Invalid state: %d", state);
                    return AUDIO_ERR_STATUS;
            }
        }
    }

    u8 buf[sizeof(blt_ascsc_write_ase_t) + aseListCnt * sizeof(blt_ascsc_release_t)];
    blt_ascsc_write_ase_t *pWrAse = (blt_ascsc_write_ase_t *)buf;

    /* <<ASCS_v1.0.pdf>> Page 36, Table 5.9: Release operation format */
    pWrAse->opcode = BLT_ASCS_OPCODE_CONFIG_RELEASE;
    pWrAse->numOfAses = aseListCnt;

    for (int i = 0; i < aseListCnt; i++) {
        blt_ascsc_release_t *pRelease = (blt_ascsc_release_t*)(pWrAse->aseParam + i * sizeof(blt_ascsc_release_t));
        pRelease->aseID = pAseList[i]->aseID;
    }

    return blt_ascsc_write(connHandle, pWrAse, sizeof(buf));
}

int blc_ascsc_writeUptMetadata(u16 connHandle, blt_ascsc_ase_t *pAseList[], u8 aseListCnt)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(connHandle);
    BLT_ASCS_LOG("blc_ascsc_writeUptMetadata");
    u16 totalMetadataLen = 0;

    if (!pAscsClt->aseCtrlPntHdl) {
        BLT_ASCS_LOG("  ERR: ATT write handle not set");
        return AUDIO_ERR_DISCOVERY_FAILED;
    } else if (!pAseList || !aseListCnt) {
        return AUDIO_ERR_INVALID_PARAMETER;
    } else {
        for (int i = 0; i < aseListCnt; i++) {
            u8 state = pAseList[i]->state;
            switch (state) {
                /* the Update Metadata operation is valid for an ASE only if the value of the
                 * ASE_State field is 0x03 (Enabling) or 0x04 (Streaming). */
                case BLT_ASCS_ASE_STATE_ENABLING:
                case BLT_ASCS_ASE_STATE_STREAMING:
                    break;
                default:
                    BLT_ASCS_LOG("  ERR: Invalid state: %d", state);
                    return AUDIO_ERR_STATUS;
            }
            totalMetadataLen += pAseList[i]->metadataLen;
        }
    }

    u8 buf[sizeof(blt_ascsc_write_ase_t) + totalMetadataLen + aseListCnt * sizeof(blt_ascsc_update_metadata_t)];
    blt_ascsc_write_ase_t *pWrAse = (blt_ascsc_write_ase_t *)buf;

    /* <<ASCS_v1.0.pdf>> Page 35, Table 5.8: Update Metadata operation format */
    pWrAse->opcode = BLT_ASCS_OPCODE_CONFIG_UPDATE_METADATA;
    pWrAse->numOfAses = aseListCnt;

    for (int i = 0; i < aseListCnt; i++) {
        blt_ascsc_update_metadata_t *pUptMetadata = (blt_ascsc_update_metadata_t*)(pWrAse->aseParam + i * (pAseList[i]->metadataLen + sizeof(blt_ascsc_update_metadata_t)));
        pUptMetadata->aseID = pAseList[i]->aseID;
        pUptMetadata->metadataLen = pAseList[i]->metadataLen;
        memcpy(pUptMetadata->metadataCfg, pAseList[i]->pMetadata, pAseList[i]->metadataLen);
    }

    return blt_ascsc_write(connHandle, pWrAse, sizeof(buf));
}

#if (0)//currently not used, removed latter
u16 blc_ascsc_getConnHdlByCisHdl(u16 cisHandle)
{
    blt_ascsc_ase_t *pAse = NULL;
    blc_ascs_client_t *pAscsClt = NULL;

    for (int i = 0; i < gAppAudioAclCentralNum; i++) {
        pAscsClt = ascs_client_ctrl.pAscsClient[i];
        if (!pAscsClt->connHandle) return 0;
        u8 aseCnt = pAscsClt->sinkAseNum + pAscsClt->srcAseNum;
        for(int j = 0; j < aseCnt; j++){
            if(j < pAscsClt->sinkAseNum){
                pAse = pAscsClt->pSinkAse[j];
            } else{
                pAse = pAscsClt->pSrcAse[j - pAscsClt->sinkAseNum];
            }

            if (pAse->cisHdl == cisHandle) {
                return pAscsClt->connHandle;
            }
        }
    }

    return 0;
}
#endif

blt_ascsc_ase_t *blt_ascsc_getAsePtrByAseId(u16 connHandle, u8 aseID)
{
#if (1)
    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(connHandle);

    if (pAscsClt == NULL || !pAscsClt->connHandle) return NULL;

    blt_ascsc_ase_t *pAse = NULL;
    u8 aseCnt = pAscsClt->sinkAseNum + pAscsClt->srcAseNum;
    for (int j = 0; j < aseCnt; j++) {
        if (j < pAscsClt->sinkAseNum) {
            pAse = pAscsClt->pSinkAse[j];
        } else {
            pAse = pAscsClt->pSrcAse[j - pAscsClt->sinkAseNum];
        }

        if (pAse->aseID == aseID) {
            return pAse;
        }
    }

    return NULL;
#else
    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(connHandle);
    if(pAscsClt == NULL)
    {
        return NULL;
    }
    blt_ascsc_ase_t *pAse = blt_ascsc_findAseByID(pAscsClt,aseID);
    if(pAse != NULL)
    {
        return pAse;
    }
    else
    {
        return NULL;
    }
#endif
}

#if (0) //remove latter
int blc_ascsc_codecCfg(u16 connHandle, u8 aseID, blc_ascsc_aseConfig_t *pCfg)
{
    if(aseID == 0 || pCfg == NULL) {
        return AUDIO_EPARAM;
    }

    int central_role = blt_prf_getAclRole(connHandle);
    if (central_role != ACL_ROLE_CENTRAL) {
        BLT_ASCS_LOG("Invalid conn role, should be central: %d", central_role);
        return AUDIO_ERR_PARAM_INVALID;
    }

    blt_ascsc_ase_t *pAse = blt_ascsc_getAsePtrByAseId(connHandle, aseID);
    if (pAse == NULL) {
        BLT_ASCS_LOG("Invalid AseID not found: %d", aseID);
        return AUDIO_EHANDLE;
    }

    switch (pAse->state) {
        /* the Config Codec operation for an ASE is valid only if the value of the ASE_State
         * field is 0x00 (Idle), 0x01 (Codec Configured), or 0x02 (QoS Configured) */
        case BLT_ASCS_ASE_STATE_IDLE:
        case BLT_ASCS_ASE_STATE_CODEC_CFG:
        case BLT_ASCS_ASE_STATE_QOS_CFG:
            break;
        default:
            BLT_ASCS_LOG("Invalid state: %d", pAse->state);
            return AUDIO_ERR_STATUS;
    }

    if (pCfg->frequency == 0 || pCfg->frequency > BLC_AUDIO_FREQ_CFG_48000) {
        BLT_ASCS_LOG("frequency unavailable: %d", pCfg->frequency);
        return AUDIO_EPARAM;
    } else {
        pAse->frequency = pCfg->frequency;
    }
    if(pCfg->duration != BLC_AUDIO_DURATION_CFG_7_5 && pCfg->duration != BLC_AUDIO_DURATION_CFG_10) {
        BLT_ASCS_LOG("duration unavailable: %d", pCfg->duration);
        return AUDIO_EPARAM;
    } else {
        pAse->duration = pCfg->duration;
    }

    if(pCfg->frameOcts == 0) {
        BLT_ASCS_LOG("frameOcts unavailable: %d", pCfg->frameOcts);
        return AUDIO_EPARAM;
    } else {
        pAse->frameOcts = pCfg->frameOcts;
    }
    if(pCfg->location == 0) {
        BLT_ASCS_LOG("audio location unavailable: %d", pCfg->location);
        return AUDIO_EPARAM;
    } else {
        pAse->location = pCfg->location;
    }
    if(pCfg->codecFrameBlksPerSDU == 0) {
        BLT_ASCS_LOG("codecFrmBlksPerSDU unavailable: %d", pCfg->codecFrameBlksPerSDU);
        return AUDIO_EPARAM;
    } else {
        pAse->codecFrmBlksPerSDU = pCfg->codecFrameBlksPerSDU;
    }

    pAse->cigID = pCfg->cigID;
    pAse->cisID = pCfg->cisID;
    pAse->tgtLatency = 0x02; /* TODO: Select target latency based on additional input */
    pAse->tgtPHY = 0x02; /* TODO: Select target PHY based on additional input */
    pAse->codecId = pCfg->codecId;

//  pAse->ready |= BLT_AUDIO_ASE_PARAM_READY;
//  pAse->flags = BLT_AUDIOC_ASE_FLAG_SEND_CODEC;


    BLT_ASCS_LOG("ase config success", pCfg, sizeof(blc_ascsc_aseConfig_t));
    return AUDIO_ESUCC;

}
#endif

/***********************************************************************
 *    Deal Notify from server
 ***********************************************************************/
static void blt_ascsc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    blc_ascs_client_t* client = blt_ascsc_getClientInst(connHandle);
    /* ASE Control Point */
    if (attHdl == client->aseCtrlPntHdl) {
        BLT_ASCS_LOG("ASE Control Point Handle: %d", attHdl);
        BLT_ASCS_LOG("ASE Control Point notify, %s", hex_to_str(val, valLen));

        blt_ascsc_ase_cp_ntf_t *pRsp = (blt_ascsc_ase_cp_ntf_t*)val;
        if (valLen != sizeof(blt_ascsc_ase_cp_ntf_t) + pRsp->numOfAses*sizeof(blt_ascsc_ase_cp_rsp_t)) {
            return;
        }

        /////////////////////  old process /////////////////////////////
        extern bool blt_ascsc_rcvAseCtrlPntNtfDeal(blc_ascs_client_t *pAscsClt, u16 ntfHandle, u8 *pNtfVal, u16 ntfValLen);
        blt_ascsc_rcvAseCtrlPntNtfDeal(client, attHdl, val, valLen);
    } else {
        blt_ascsc_aseState_t *pAseState = (blt_ascsc_aseState_t*)val;

        blt_ascsc_ase_t *pAse = NULL;
        u16 aseHdl;
        for(int i = 0; i<client->aseCount; i++){
            if(i < client->sinkAseNum) {
                pAse = client->pSinkAse[i];
                aseHdl = client->sinkAseHdl[i];
                BLT_ASCS_LOG("Sink ASE Handle: %d", aseHdl);
            } else {
                pAse = client->pSrcAse[i - client->sinkAseNum];
                aseHdl = client->srcAseHdl[i - client->sinkAseNum];
                BLT_ASCS_LOG("Source ASE Handle: %d", aseHdl);
            }
            if (attHdl == aseHdl && pAseState->aseID == pAse->aseID) {
                BLT_ASCS_LOG("ASE State notify, %s", hex_to_str(val, valLen));

                /////////////////////  old process /////////////////////////////
                #if (1)
                    blt_ascsc_rcvAseStateNtfDeal(client, attHdl, val, valLen);
                #else
                    /* Update the ASE status value maintained by the client according to the notify ASE status. */
                    blt_ascsc_aseSetStatus(pSinkAse, pAseState, valLen);
                #endif
                break;
            }
        }
    }
}

static void blt_ascsc_displayInfo(u16 connHandle, blc_ascs_client_t* client)
{
    BLT_ASCS_LOG(" sdp over connHandle[0x%x]", connHandle);
    BLT_ASCS_LOG("    INFO:ASE control Point Handle[0x%x]", client->aseCtrlPntHdl);
    BLT_ASCS_LOG("    INFO:Sink ASE characteristic num is %d", client->sinkAseNum);

    for(int i=0; i<client->sinkAseNum; i++)
    {
        blt_ascsc_ase_t* pAse = client->pSinkAse[i];
        BLT_ASCS_LOG("        SinkASE:index[%d] Handle[0x%x] ID[0x%x] state[%d]", i, client->sinkAseHdl[i], pAse->aseID, pAse->state);
    }
    BLT_ASCS_LOG("    INFO:Source ASE characteristic num is %d", client->srcAseNum);
    for(int i=0; i<client->srcAseNum; i++)
    {
        blt_ascsc_ase_t* pAse = client->pSrcAse[i];
        BLT_ASCS_LOG("        SourceASE:index[%d] Handle[0x%x] ID[0x%x] state[%d]", i, client->srcAseHdl[i], pAse->aseID, pAse->state);
    }
}

static void blt_ascsc_foundService(u16 connHandle, u8 count, u16 startHandle, u16 endHandle)
{
    blc_ascs_client_t* client = blt_ascsc_getClientInst(connHandle);

    if(count == 0xFF)
    {
        blc_prf_sendServiceDiscoveryFailEvent(connHandle, AUDIO_ASCS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        BLT_ASCS_LOG("ERR:not found ASCS");
        return ;
    }

    if(count == 0)
    {
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, AUDIO_ASCS_CLIENT);
        blt_ascsc_displayInfo(connHandle, client);
        client->aseCount = client->sinkAseNum + client->srcAseNum;
        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return ;
    }

    client->ntfInput.startHdl = startHandle;
    client->ntfInput.endHdl = endHandle;
    client->ntfInput.ntfOrIndFunc = blt_ascsc_dataInput;
    BLT_ASCS_LOG("  INFO: ASCS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, startHandle, endHandle);
    blc_prf_sendServiceDiscoveryFoundEvent(connHandle, AUDIO_ASCS_CLIENT, startHandle, endHandle);
}

static void blt_ascsc_foundSinkAseChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_ascs_client_t* client = blt_ascsc_getClientInst(connHandle);
    if(client->sinkAseNum>=STACK_AUDIO_ASCS_ASE_SNK_NUM) {
        BLT_ASCS_LOG("ERR sink ase characteristic[0x%x] too many", valueHandle);
        return ;
    }
    client->sinkAseHdl[client->sinkAseNum] = valueHandle;
    client->sinkAseNum++;
    BLT_ASCS_LOG(" sink ase ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_ascsc_sinkAseStartRead(u16 connHandle, u16 attrHandle, u8** read, u16** readLen, u16* readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_ascs_client_t* client = blt_ascsc_getClientInst(connHandle);
    if(client->sinkAseIdx>=STACK_AUDIO_ASCS_ASE_SNK_NUM) {

        BLT_ASCS_LOG("ERR sink ase characteristic[0x%x] too many", attrHandle);
        return ;
    }
    blt_ascsc_ase_t *pAse = client->pSinkAse[client->sinkAseIdx];
    client->sinkAseIdx ++;
    //TODO: only read aseID and state
    *read = (u8*)&pAse->aseID;
    *readLen = NULL;
    *readMaxSize = 2;
    *rdCbFunc = NULL;
}

static void blt_ascsc_foundSourceAseChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_ascs_client_t* client = blt_ascsc_getClientInst(connHandle);
    if(client->srcAseNum>=STACK_AUDIO_ASCS_ASE_SNK_NUM) {
        BLT_ASCS_LOG("ERR source ase characteristic[0x%x] too many", valueHandle);
        return ;
    }
    client->srcAseHdl[client->srcAseNum] = valueHandle;
    client->srcAseNum++;
    BLT_ASCS_LOG(" source ase ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_ascsc_sourceAseStartRead(u16 connHandle, u16 attrHandle, u8** read, u16** readLen, u16* readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_ascs_client_t* client = blt_ascsc_getClientInst(connHandle);
    if(client->srcAseIdx>=STACK_AUDIO_ASCS_ASE_SNK_NUM) {

        BLT_ASCS_LOG("ERR sink ase characteristic[0x%x] too many", attrHandle);
        return ;
    }
    blt_ascsc_ase_t *pAse = client->pSrcAse[client->srcAseIdx];
    client->srcAseIdx ++;
    //TODO: only read aseID and state
    *read = (u8*)&pAse->aseID;
    *readLen = NULL;
    *readMaxSize = 2;
    *rdCbFunc = NULL;
}

static void blt_ascsc_foundAseControlPpointChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_ascs_client_t* client = blt_ascsc_getClientInst(connHandle);
    client->aseCtrlPntHdl = valueHandle;
    BLT_ASCS_LOG("ase control ppoint ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}


static const blc_gapc_discService_t ascsService = {
    .uuid = UUID16_INIT(SERVICE_UUID_AUDIO_STREAM_CONTROL),
    .sfun = blt_ascsc_foundService,
};

static const blc_gapc_discChar_t ascsChar[] = {
    {
        .subscribeNtf = true,
        .readValue = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_SINK_ASE),
        .cfun = blt_ascsc_foundSinkAseChar,
        .rfun = blt_ascsc_sinkAseStartRead,
    },
    {
        .subscribeNtf = true,
        .readValue = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_SOURCE_ASE),
        .cfun = blt_ascsc_foundSourceAseChar,
        .rfun = blt_ascsc_sourceAseStartRead,
    },
    {
        .subscribeNtf = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_ASE_CONTROL_POINT),
        .cfun = blt_ascsc_foundAseControlPpointChar,
    },
};

static const blc_gapc_discList_t discAscs = {
    .maxServiceCount = 1,
    .service = &ascsService,
    .includeTable = {
        .size = 0,
    },
    .characteristicTable = {
        .size = ARRAY_SIZE(ascsChar),
        .characteristic = ascsChar,
    },
};

/**********reconnect function start*********/
static bool blt_ascsc_reconnService(u16 connHandle, int count)
{
    if(count == 0)
    {
        blc_ascs_client_t *client = blt_ascsc_getClientInst(connHandle);
        blt_ascsc_displayInfo(connHandle, client);
        BLT_ASCS_LOG("  INFO: ASCS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, client->ntfInput.startHdl, client->ntfInput.endHdl);
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, AUDIO_ASCS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return true;
    }

    if(count > 1)
        return false;
    return true;
}

static int blt_ascsc_sinkAseGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    blc_ascs_client_t* client = blt_ascsc_getClientInst(connHandle);
    BLT_ASCS_LOG("sink ASE count is %d, address is 0x%x, client address is %x, connHandle is %x", client->sinkAseNum, &client->sinkAseNum, client, connHandle);
    for(int i=0; i<client->sinkAseNum; i++)
    {
        charInfo->properties = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
        charInfo->valueHandle = client->sinkAseHdl[i];
        charInfo->cccHandle = 0;
        BLT_ASCS_LOG("sink ASE test, %x", charInfo->valueHandle);
        charInfo++;
    }

    return client->sinkAseNum;
}

static int blt_ascsc_sourceAseGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    blc_ascs_client_t* client = blt_ascsc_getClientInst(connHandle);
    BLT_ASCS_LOG("source ASE count is %d", client->srcAseNum);
    for(int i=0; i<client->srcAseNum; i++)
    {
        charInfo->properties = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
        charInfo->valueHandle = client->srcAseHdl[i];
        charInfo->cccHandle = 0;
        BLT_ASCS_LOG("source ASE test, %x", charInfo->valueHandle);
        charInfo++;
    }

    return client->srcAseNum;
}

static int blt_ascsc_aseControlPpointGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    (void)connHandle;
    charInfo->properties = CHAR_PROP_NOTIFY;
    charInfo->cccHandle = 0;

    return 1;
}
static const blc_gapc_reconnChar_t reAscsChar[] = {

    {
        .ifun = blt_ascsc_sinkAseGetInfo,
        .rfun = blt_ascsc_sinkAseStartRead,
    },

    {
        .ifun = blt_ascsc_sourceAseGetInfo,
        .rfun = blt_ascsc_sourceAseStartRead,
    },

    {
        .ifun = blt_ascsc_aseControlPpointGetInfo
    },
};

static const blc_gapc_reconnList_t reconnAscs = {
    .resfun = blt_ascsc_reconnService,
    .charTb = {
        .size = ARRAY_SIZE(reAscsChar),
        .characteristic = reAscsChar,
    },
    .inclSize = 0,
};

/**********reconnect function ending********/


//////////////////////////////////// old ascs_client2.c ///////////////////////////////////
blt_ascsc_cisBondingParam_t cisBondingParam[LL_MAX_ACL_CEN_NUM];

/* Central ISO event concerned */
static int blt_ascsc_audioDataPathSetup(blt_ascsc_ase_t *pAse, u8 dataPathType, audio_role_enum role)
{
    /* type 0: Codec inside controller, others: Codec inside host */

    if(pAse == NULL || !pAse->cisEstablished)
    {
        return -AUDIO_EHANDLE;
    }

    if(!pAse->dataPathSetup)
    { /* Audio data path setup */
        hci_le_setupIsoDataPath_cmdParam_t isoDataPath;
        hci_le_setupIsoDataPath_retParam_t isoDataPathRet;
        isoDataPath.conn_handle = pAse->cisHdl;
        isoDataPath.data_path_dir = pAse->dir == AUDIO_DIR_SINK ? 1 : 0;

        if(role == AUDIO_SERVER)//role 1:server
        {
            isoDataPath.data_path_dir = pAse->dir == AUDIO_DIR_SINK ? Data_Dir_Output : Data_Dir_Input;
        }
        else if(role == AUDIO_CLIENT)//role 0:client
        {
            isoDataPath.data_path_dir = pAse->dir == AUDIO_DIR_SINK ? Data_Dir_Input : Data_Dir_Output;
        }

        isoDataPath.data_path_id = 0x00; /* When set to 0x00, the data path shall be over the HCI transport. When set to 0xFF the path shall be disabled. */
        memcpy(&isoDataPath.codec_id_assignNum, &pAse->codecId, 5);

        isoDataPath.control_delay[0] = U32_BYTE0(pAse->PresentationDly);
        isoDataPath.control_delay[1] = U32_BYTE1(pAse->PresentationDly);
        isoDataPath.control_delay[2] = U32_BYTE2(pAse->PresentationDly);

        if(dataPathType)
        { /* if the codec in use resides in the Bluetooth Controller  */
            isoDataPath.codec_config_len = 16;
            isoDataPath.codec_config[0] = 0x02;
            isoDataPath.codec_config[1] = BLC_AUDIO_CAPTYPE_CFG_SAMPLE_FREQUENCY;
            isoDataPath.codec_config[2] = pAse->frequency;
            isoDataPath.codec_config[3] = 0x02;
            isoDataPath.codec_config[4] = BLC_AUDIO_CAPTYPE_CFG_FRAME_DURATION;
            isoDataPath.codec_config[5] = pAse->duration;
            isoDataPath.codec_config[6] = 0x05;
            isoDataPath.codec_config[7] = BLC_AUDIO_CAPTYPE_CFG_CHANNELS_ALLOCATION;
            isoDataPath.codec_config[8] = (pAse->location) & 0xFF;
            isoDataPath.codec_config[9] = ((pAse->location) & 0xFF00)>>8;
            isoDataPath.codec_config[10] = ((pAse->location) & 0xFF0000)>>16;
            isoDataPath.codec_config[11] = ((pAse->location) & 0xFF000000)>>24;
            isoDataPath.codec_config[12] = 0x03;
            isoDataPath.codec_config[13] = BLC_AUDIO_CAPTYPE_CFG_OCTETS_PER_CODEC_FRAME;
            isoDataPath.codec_config[14] = (pAse->frameOcts) & 0xFF;
            isoDataPath.codec_config[15] = ((pAse->frameOcts) & 0xFF00)>>8;
        }
        else
        { /* if the codec in use resides in the Bluetooth Host  */
            isoDataPath.codec_config_len = 0;
            isoDataPath.codec_id_assignNum = BLC_AUDIO_CODING_FORMAT_TRANSPARENT;
        }

        if(blc_hci_le_setupIsoDataPath(&isoDataPath,&isoDataPathRet) == BLE_SUCCESS)
        {
            pAse->dataPathSetup = 1;
            BLT_ASCS_LOG("Audio data path setup");
            return AUDIO_ESUCC;
        }
        else
        {
            BLT_ASCS_LOG("Audio data path setup failed: %d", isoDataPathRet.status);
        }
    }
    else
    {
        BLT_ASCS_LOG("Audio data path already setup");
        return AUDIO_ESUCC;
    }
    return -AUDIO_EHANDLE;
}
static int blt_ascsc_audioDataPathRemove(blt_ascsc_ase_t *pAse, audio_role_enum role)
{
    if(pAse == NULL || !pAse->cisEstablished)
    {
        return AUDIO_EHANDLE;
    }

    if(pAse->dataPathSetup)
    { /* Audio data path setup */
        pAse->dataPathSetup = 0;
        u8 data_path_dir = 0;

        if(role == AUDIO_SERVER)//role 1:server
        {
            data_path_dir = pAse->dir == AUDIO_DIR_SINK ? DP_OUTPUT_MASK : DP_INPUT_MASK;
        }
        else if(role == AUDIO_CLIENT)//role 0:client
        {
            data_path_dir = pAse->dir == AUDIO_DIR_SINK ? DP_INPUT_MASK : DP_OUTPUT_MASK;
        }

        ble_sts_t ret = blc_ll_removeCisDataPath(pAse->cisHdl, data_path_dir);
        BLT_ASCS_LOG("Audio data path remove: %d", ret);
        if(ret != BLE_SUCCESS){
            return AUDIO_EFAIL;
        }

        return AUDIO_ESUCC;
    }
    else
    {
        BLT_ASCS_LOG("Audio data path already removed");
        return AUDIO_ESUCC;
    }
    return AUDIO_EHANDLE;
}
int blt_ascsc_cisConnectEvt(u16 cisHandle, u8 *pPkt)
{
    int connHandle = blt_audio_getAclHdlByCisHdl(cisHandle);
    if(connHandle < 0) return -AUDIO_EPARAM;

    u8 index;
    blt_ascsc_ase_t *pAse;
    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(connHandle);
    if(pAscsClt == NULL || !cisHandle)
    {
        return AUDIO_EHANDLE;
    }
    hci_le_cisEstablishedEvt_t *pCisEstbEvent = (hci_le_cisEstablishedEvt_t *)pPkt;

    for(index=0; index<pAscsClt->aseCount; index++)
    {
        if(index < pAscsClt->sinkAseNum)
        {
            pAse = pAscsClt->pSinkAse[index];
        }
        else
        {
            pAse = pAscsClt->pSrcAse[index - pAscsClt->sinkAseNum];
        }
        if(pAse->cisHdl == cisHandle)
        {
            if(pCisEstbEvent->status == BLE_SUCCESS)
            {
                BLT_ASCS_LOG("cis establish success - ASE ID: %d", pAse->aseID);
                pAse->cisEstablished = 1;
                if(pAse->dataPathSetup || !blt_ascsc_audioDataPathSetup(pAse, 1, AUDIO_CLIENT))
                { /* codec inside host */
                    BLT_ASCS_LOG("enable ase-Ase ID: %d", pAse->aseID);
                    //1.Client audio Source, server audio Sink: Client do nothing
                    //2.Client audio Sink, server audio Source: Write CP Receive start ready cmd(if enabling):If a Source ASE is in the Enabling state, the Unicast Client shall initiate the Receiver Start Ready
                    //operation for that ASE when the Unicast Client is ready to consume audio data transmitted from that ASE by the Unicast Server.
                    pAse->flags |= BLT_ASCSC_ASE_FLAG_SEND_ENABLE;
                }
            }
            else
            {
                BLT_ASCS_LOG("cis establish fail: %d", pCisEstbEvent->status);
                blt_ascsc_audioDataPathRemove(pAse, AUDIO_CLIENT);
                if(pAse->flags & BLT_ASCSC_ASE_FLAG_SEND_ENABLE)
                {
                    pAse->flags &= ~BLT_ASCSC_ASE_FLAG_SEND_ENABLE;
                }

                if(pAse->cisCreateRetryNum++ < 1)
                {
                    pAse->flags |= BLT_ASCSC_ASE_FLAG_CREATE_CIS;
                    pAse->flags &= ~BLT_ASCSC_ASE_FLAG_SEND_WAIT;

                    //must,  because the flag of pAse is none, pAscsClt's busy will be cleared after the Qos configuration is sent
                    pAscsClt->aseFsmBusy = 1;
                    BLT_ASCS_LOG("retry to establish cis-Ase ID: %d", pAse->aseID);
                    BLT_ASCS_LOG("retry num: %d", pAse->cisCreateRetryNum);
                }
            }
            pAse->flags &= ~BLT_ASCSC_ASE_FLAG_CREATE_CIS_WAIT;

        }
    }
    return AUDIO_ESUCC;
}
int blt_ascsc_cisDisconnEvt(u16 cisHandle, u8 *pPkt)
{
    (void)pPkt;
    int connHandle = blt_audio_getAclHdlByCisHdl(cisHandle);
    if(connHandle < 0) return AUDIO_EPARAM;

    u8 index;
    blt_ascsc_ase_t *pAse;
    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(connHandle);
    if(pAscsClt == NULL)
    {
        return AUDIO_EHANDLE;
    }

    for(index=0; index<pAscsClt->aseCount; index++)
    {
        if(index < pAscsClt->sinkAseNum)
        {
            pAse = pAscsClt->pSinkAse[index];
        }
        else
        {
            pAse = pAscsClt->pSrcAse[index - pAscsClt->sinkAseNum];
        }

        BLT_ASCS_LOG("cis disconnect,pAse->cisHdl:0x%X", pAse->cisHdl);

        if(pAse->cisHdl == cisHandle)
        {
            /*  BAP v1.0.1 page 105
             *  when the Unicast Server detects loss of a CIS for an ASE in the Streaming or
             *  the Disabling state, the Unicast Server transitions the ASE to the QoS Configured state.*/
            BLT_ASCS_LOG("cis disconnect,ase match cis - ASE ID': %d", pAse->aseID);

            //Remove ISO data path
            audio_error_enum ret = blt_ascsc_audioDataPathRemove(pAse, AUDIO_CLIENT);
            if(ret != AUDIO_ESUCC){
                BLT_ASCS_LOG("Audio data path remove failed");
            }
            pAse->cisHdl = 0; /* clear */
            pAse->cigID = 0xFF;
            pAse->cisID = 0xFF;
            pAse->cisEstablished = 0; /* clear */
            pAse->flags &= ~BLT_ASCSC_ASE_FLAG_DESTROY_CIS_WAIT;
            pAse->flags &= ~BLT_ASCSC_ASE_FLAG_DESTROY_CIS;
            // codec qos need re-trigger
            pAse->ready = BLT_ASCSC_ASE_READY_NONE;

            if(pAse->state == BLT_ASCS_ASE_STATE_STREAMING || pAse->state == BLT_ASCS_ASE_STATE_DISABLING)
            {
                BLT_ASCS_LOG("ase state convert to QOS Configured': %d", pAse->aseID);
                pAse->state = BLT_ASCS_ASE_STATE_IDLE;
                pAse->flags = BLT_ASCSC_ASE_FLAG_NONE;
                pAscsClt->aseFsmBusy = 1;
            }
        }
    }
    return AUDIO_ESUCC;
}




/* Client process server's ATT notify(ASE control point & ASE state notify) */
static blt_ascsc_ase_t *blt_ascsc_findAseByID(blc_ascs_client_t *pAscsClt, u8 aseID)
{
    u8 index;

    if(aseID == 0)
    {
        return NULL;
    }

    for(index=0; index<pAscsClt->sinkAseNum; index++)
    {
        if(pAscsClt->pSinkAse[index]->aseID == aseID)
        {
            break;
        }
    }
    if(index == pAscsClt->sinkAseNum)
    {
        /* Sink ASE not found, start to find source ASE */
        for(index=0; index<pAscsClt->srcAseNum; index++)
        {
            if(pAscsClt->pSrcAse[index]->aseID == aseID)
            {
                break;
            }
        }
        if(index == pAscsClt->srcAseNum)
        { /* Sink ASE and Source are all not found */
            return NULL;
        }
        else
        {
            return pAscsClt->pSrcAse[index];
        }
    }
    else
    {
        return pAscsClt->pSinkAse[index];
    }
}
static u8 blt_acscc_getAseListWithSameCigID(blc_ascs_client_t *pAscsClt, blt_ascsc_ase_t *outAseList[STACK_AUDIO_ASCS_ASE_NUM])
{
    if(pAscsClt == NULL || outAseList == NULL)
    {
        return 0;
    }

    u8 aseListCnt = 0;
    blt_ascsc_ase_t *pAse = NULL;

    u8 cigID = CIG_ID_INVALID;
    for(u8 index=0; index<pAscsClt->aseCount; index++)
    {
        if(index < pAscsClt->sinkAseNum)
        {
            pAse = pAscsClt->pSinkAse[index];
            //BLT_ASCS_LOG("sink ASE num: %d", pAscsClt->sinkAseNum);
        }
        else
        {
            pAse = pAscsClt->pSrcAse[index - pAscsClt->sinkAseNum];
            //BLT_ASCS_LOG("source ASE num: %d", pAscsClt->srcAseNum);
        }

        if(pAse->ready & BLT_ASCSC_ASE_PARAM_READY)
        {
            if(aseListCnt == 0)
            { /* Find 1st configured ase's cigID */
                cigID = pAse->cigID;
//              BLT_ASCS_LOG("ase ID: %d", pAse->aseID);
//              BLT_ASCS_LOG("cis ID: %d", pAse->cisID);
//              BLT_ASCS_LOG("cig ID: %d", pAse->cigID);
                outAseList[aseListCnt++] = pAse;
                //BLT_ASCS_LOG("Find 1st configured ase's cigID: %d", cigID);
            }
            else
            {
                if(pAse->cigID == cigID)
                {
//                  BLT_ASCS_LOG("ase ID: %d", pAse->aseID);
//                  BLT_ASCS_LOG("cis ID: %d", pAse->cisID);
//                  BLT_ASCS_LOG("cig ID: %d", pAse->cigID);
                    outAseList[aseListCnt++] = pAse;
                }
            }
        }
    }

    //if(aseListCnt) hex_to_str("Codec Ase List, %s", hex_to_str(outAseList, aseListCnt*sizeof(blt_ascsc_ase_t)));
    return aseListCnt;
}
static u8 blt_acscc_statisticsCisInfoInCig(u8 aseListCnt, blt_ascsc_ase_t *pInAseList[], u8 *pOutCissCnt, blt_ascsc_cissInfo_t outCissInfoList[])
{
    if(!aseListCnt || outCissInfoList == NULL)
    {
        return false;
    }
    u8 sameCisIDStatCnt, cisIDListCnt,cisID;
    for(u8 i=0; i<aseListCnt; i++)
    {
        cisID = pInAseList[i]->cisID;
        if(i == 0)
        {
            cisIDListCnt = 0;
        }
        else
        {
            for(u8 j=0;j<cisIDListCnt;j++)
            {
                if(pInAseList[i]->cisID == outCissInfoList[j].cisID)
                {
                    goto jump; /* cisID equal skip */
                }
            }
        }

        sameCisIDStatCnt = 0;
        for(u8 j=0; j<aseListCnt; j++)
        {
            if(cisID == pInAseList[j]->cisID)
            {
                outCissInfoList[cisIDListCnt].pAseList[sameCisIDStatCnt++] = pInAseList[j];
            }
        }
        outCissInfoList[cisIDListCnt].cisID = cisID;
        outCissInfoList[cisIDListCnt].cisNum = sameCisIDStatCnt;
        cisIDListCnt++;
        jump:;
    }
    if(pOutCissCnt != NULL)
    {
        *pOutCissCnt = cisIDListCnt;
    }

    return true;
}
static u8 blt_audio_ascpMetadataCmp(blt_ascsc_ase_t *pAse, u8 newMetaLen, u8 *pNewMeta)
{
    blc_audio_metadata_parsed_t currMetadata = {0};
    blc_audio_metadata_parsed_t lastMetadata = {0};

    u8 metadataChkSts = blt_audio_getMetadataParams(newMetaLen, pNewMeta, &currMetadata);
    if(metadataChkSts)
    {
        return 0;
    }

    metadataChkSts = blt_audio_getMetadataParams(pAse->metadataLen, pAse->pMetadata, &lastMetadata);
    if(metadataChkSts)
    {
        return 0;
    }

    u8 metadataChangeFlag = 0;
    //consider add metadata,delete metadata,update metadata.
    if(lastMetadata.fieldExistFlg != currMetadata.fieldExistFlg)
    {
        metadataChangeFlag = 1;
    }
    else {
        //only apply server notified metadata,if ASE different,then update ASE.
        if(currMetadata.fieldExistFlg&BLC_AUDIO_METATYPE_STREAMING_CONTEXTS_MASK)
        {
            if (lastMetadata.streamingCtx != currMetadata.streamingCtx) {
                metadataChangeFlag = 1;
                goto metadata_chang;
            }
        }
        if(currMetadata.fieldExistFlg&BLC_AUDIO_METATYPE_PROGRAM_INFO_MASK)
        {
            if (lastMetadata.programInfoLen != currMetadata.programInfoLen) {
                metadataChangeFlag = 1;
                goto metadata_chang;
            }else if (memcmp(lastMetadata.pProgramInfo, currMetadata.pProgramInfo, currMetadata.programInfoLen)) {
                metadataChangeFlag = 1;
                goto metadata_chang;
            }
        }
        if(currMetadata.fieldExistFlg&BLC_AUDIO_METATYPE_LANGUAGE_MASK)
        {
            if(lastMetadata.language != currMetadata.language)//update metadata
            {
                metadataChangeFlag = 1;
                goto metadata_chang;
            }
        }
        if(currMetadata.fieldExistFlg&BLC_AUDIO_METATYPE_CCID_LIST_MASK)
        {
            if (lastMetadata.ccidListLen != currMetadata.ccidListLen) {
                metadataChangeFlag = 1;
                goto metadata_chang;
            }else if (memcmp(lastMetadata.pCcidList, currMetadata.pCcidList, currMetadata.ccidListLen)) {
                metadataChangeFlag = 1;
                goto metadata_chang;
            }
        }
        if(currMetadata.fieldExistFlg&BLC_AUDIO_METATYPE_PARENTAL_RATING_MASK)
        {
            if (lastMetadata.parentalRating != currMetadata.parentalRating) {
                metadataChangeFlag = 1;
                goto metadata_chang;
            }
        }
        if(currMetadata.fieldExistFlg&BLC_AUDIO_METATYPE_PROGRAM_INFO_URI_MASK)
        {
            if (lastMetadata.programInfoURILen != currMetadata.programInfoURILen) {
                metadataChangeFlag = 1;
                goto metadata_chang;
            }else if (memcmp(lastMetadata.pProgramInfo, currMetadata.pProgramInfo, currMetadata.programInfoURILen)) {
                metadataChangeFlag = 1;
                goto metadata_chang;
            }
        }
        if(currMetadata.fieldExistFlg&BLC_AUDIO_METATYPE_EXTENDED_METADATA_MASK)
        {
            if (lastMetadata.extMetadataLen != currMetadata.extMetadataLen) {
                metadataChangeFlag = 1;
                goto metadata_chang;
            }else if (memcmp(lastMetadata.pExtMetadata, currMetadata.pExtMetadata, currMetadata.extMetadataLen)) {
                metadataChangeFlag = 1;
                goto metadata_chang;
            }
        }
        if(currMetadata.fieldExistFlg&BLC_AUDIO_METATYPE_VENDOR_SPECIFIC_MASK)
        {
            if (lastMetadata.vsMetadataLen != currMetadata.vsMetadataLen) {
                metadataChangeFlag = 1;
                goto metadata_chang;
            }else if (memcmp(lastMetadata.pVendorSpecMetadata, currMetadata.pVendorSpecMetadata, currMetadata.vsMetadataLen)) {
                metadataChangeFlag = 1;
                goto metadata_chang;
            }
        }
    }

metadata_chang:

    if (metadataChangeFlag) {
        pAse->metadataLen = newMetaLen;
        memcpy(pAse->pMetadata, pNewMeta, newMetaLen);
        return 1;
    }

    return 0;
}
bool blt_ascsc_rcvAseStateNtfDeal(blc_ascs_client_t *pAscsClt, u16 ntfHandle, u8 *pNtfVal, u16 ntfValLen)
{
    (void)ntfHandle;
    blt_ascsc_ase_t *pAse = NULL;
    blt_ascsc_aseStateVal_t *pascpState = (blt_ascsc_aseStateVal_t*)pNtfVal;
    u8 aseState = pascpState->u.aseVal.aseState;
    u8 aseID = pascpState->u.aseVal.aseID;

    if (ntfValLen < 2)
    {
        return false; /* MIN length 2B: ASE_ID, ASE_State */;
    }

    u16 dataLen = ntfValLen;
    pAse = blt_ascsc_findAseByID(pAscsClt, aseID);
    BLT_ASCS_LOG("receive notify on state handle - Ase ID: %d", pAse->aseID);
    if(pAscsClt == NULL || pAse == NULL)
    {
        BLT_ASCS_LOG("Invalid parameter");
        return false;
    }
    /////////////////////////////////////////////////////////////////////////////////////////////////
    u8 outCissCnt = 0;
    blt_ascsc_cissInfo_t outCissInfoList[STACK_AUDIO_ASCS_ASE_NUM] = {0};
    blt_ascsc_ase_t* pAseList[STACK_AUDIO_ASCS_ASE_NUM] = {0};
    u8 aseListCnt = blt_acscc_getAseListWithSameCigID(pAscsClt, pAseList);//The number of ase in a same CIG.
    blt_acscc_statisticsCisInfoInCig(aseListCnt, pAseList, &outCissCnt, &outCissInfoList[0]);

    u16 aclHandle = pAscsClt->connHandle;
    /////////////////////////////////////////////////////////////////////////////////////////////////
    if(aseState == BLT_ASCS_ASE_STATE_CODEC_CFG)
    {
        BLT_ASCS_LOG("Ase state-codec configured");
        /**
         * The Unicast Client shall write all Config QoS operation parameter values for all ASEs being configured
         * within the same CIG with the Unicast server in a single Config QoS operation. I
         */
        if(pAse->state == BLT_ASCS_ASE_STATE_CODEC_CFG ||\
                pAse->state == BLT_ASCS_ASE_STATE_IDLE ||\
                pAse->state == BLT_ASCS_ASE_STATE_QOS_CFG)//need to prepare the value of qos
        {
            pAse->state = BLT_ASCS_ASE_STATE_CODEC_CFG;
            if(dataLen != OFFSETOF(blt_ascsc_aseStateCodecCfg_t, codecSpecCfg)+ pascpState->u.codec.codecSpecCfgLen)
            {
                return false;
            }
            u32 minPrefPreDly, maxPrefPreDly;
            BYTE_TO_UINT24(minPrefPreDly, pascpState->u.codec.prefPresentationDelayMin); //Preferred_Presentation_Delay_Min
            BYTE_TO_UINT24(maxPrefPreDly, pascpState->u.codec.prefPresentationDelayMax); //Preferred_Presentation_Delay_Max
            if(minPrefPreDly&&maxPrefPreDly)
            {
                if(minPrefPreDly > maxPrefPreDly)
                {
                    BLT_ASCS_LOG("Invalid prefMinPD: %d", minPrefPreDly);
                    BLT_ASCS_LOG("Invalid prefMaxPD: %d", maxPrefPreDly);
                    if(pAse->flags & BLT_ASCSC_ASE_FLAG_ENABLE)
                    {
                        pAse->flags &= ~BLT_ASCSC_ASE_FLAG_ENABLE_MASK;
                        pAse->ready &= ~BLT_ASCSC_ASE_CODEC_READY;
                    }
                    return false;
                }
            }
            //The server can use this parameter and Preferred_Presentation_Delay_Max to express a
            //narrower range of its supported presentation delay that the server prefers to operate in.
            pAse->prefMinPresentationDelay = minPrefPreDly;
            pAse->prefMaxPresentationDelay = maxPrefPreDly;
            u32 minPreDly, maxPreDly;
            BYTE_TO_UINT24(minPreDly, pascpState->u.codec.presentationDelayMin); //Presentation_Delay_Min
            BYTE_TO_UINT24(maxPreDly, pascpState->u.codec.presentationDelayMax); //Presentation_Delay_Max


            if(minPreDly > maxPreDly)
            {
                BLT_ASCS_LOG("Invalid minPD: %d", minPreDly);
                BLT_ASCS_LOG("Invalid maxPD: %d", maxPreDly);
                if(pAse->flags & BLT_ASCSC_ASE_FLAG_ENABLE)
                {
                    pAse->flags &= ~BLT_ASCSC_ASE_FLAG_ENABLE_MASK;
                    pAse->ready &= ~BLT_ASCSC_ASE_CODEC_READY;
                }
                return false;
            }
            //Preferred_Presentation_Delay: A value of 0x000000 indicates no preference
            if((minPrefPreDly && minPrefPreDly < minPreDly) || (maxPrefPreDly && maxPrefPreDly > maxPreDly))
            {
                BLT_ASCS_LOG("Invalid prefMinPD': %d", minPrefPreDly);
                BLT_ASCS_LOG("Invalid prefMaxPD': %d", maxPrefPreDly);
                if(pAse->flags & BLT_ASCSC_ASE_FLAG_ENABLE)
                {
                    pAse->flags &= ~BLT_ASCSC_ASE_FLAG_ENABLE_MASK;
                    pAse->ready &= ~BLT_ASCSC_ASE_CODEC_READY;
                }
                return false;
            }

            u8 prefPhy = pascpState->u.codec.prefPHY;

            //Server supported Presentation_Delay(Minimum Maximum)
            pAse->minPresentationDelay = minPreDly;
            pAse->maxPresentationDelay = maxPreDly;

            //Server support for unframed ISOAL PDUs
            pAse->unframedNotSupp = pascpState->u.codec.framing & 0b1;
            //Server preferred value for the PHY parameter (Formatted as a bitfield, BIT(0):1M,BIT(1)2M,BIT(3)Coded)
            pAse->preferredPHY = prefPhy; //pascpState->u.codec.prefPHY;
            //Server preferred value for Retransmission_Number parameter(Range:0x00��C0xFF)
            pAse->preferredRTN = pascpState->u.codec.prefRetransmitNum;
            //Server preferred value for the Max_Transport_Latency parameter(Range: 0x0005��C0x0FA0 ms)


            blc_audio_codecSpecCfgParsed_t codecSpecParam = {0};
            pAse->codecId = pascpState->u.codec.codecId;
            u8 specCfgSts = blt_audio_getCodecSpecCfgParam(&pascpState->u.codec.codecSpecCfgLen, &codecSpecParam);
            if(specCfgSts)
            {
                BLT_ASCS_LOG("Get codec cfg Error: %d", specCfgSts);
                return false;
            }
            if(codecSpecParam.frequency!=pAse->frequency ||\
               codecSpecParam.duration!=pAse->duration ||\
               codecSpecParam.allocation!=pAse->location||\
               codecSpecParam.frameOcts!=pAse->frameOcts||\
               codecSpecParam.codecFrameBlksPerSDU!=pAse->codecFrmBlksPerSDU)
            {
                BLT_ASCS_LOG("codec cfg changed");
                return false;
            }

//          pAse->sduInterval = audioDurUs;
//          pAse->cigSduInterval = audioDurUs;
            pAse->ready |= BLT_ASCSC_ASE_CODEC_READY;
            blt_audio_unicastCltCodecCfgEvt(pAscsClt->connHandle, pAse, &pascpState->u.codec);
            u16 transportLatencyTemp = min(pascpState->u.codec.maxTransportLatency, AUDIO_UNICAST_CLIENT_MAX_TRANSPORT_LATENCY);
            pAse->maxTransLatency = min(pAse->maxTransLatency,transportLatencyTemp);
            bool m2s = pAse->dir == AUDIO_DIR_SINK ? 1 : 0;
            /* BAP v1.0.1 section7.1 Page 133
             * The Unicast Client sets the same Presentation_Delay parameter value for all ASEs of the same direction
            (all Sink ASEs or all Source ASEs) in each CIG*/

            /* BAP v1.0.1 section7.1.3 Page 134
             * The Presentation_Delay_Min and Presentation_Delay field values exposed by a Unicast
               Server for a Sink ASE are independent of any Presentation_Delay_Min and Presentation_Delay_Max
               field values exposed by that Unicast Server for a Source ASE.*/
            /* BAP v1.0.1 section7.1.3 Page 135
            For all ASEs where a Unicast Server is in the Audio Sink role (all Sink ASEs), and for all ASEs where a
            Unicast Server is in the Audio Source role (all Source ASEs), the Presentation_Delay parameter values
            requested by the Unicast Client with each Unicast Server shall be:
            No lower than the greatest value of Presentation_Delay_Min that the Unicast Servers have
            respectively exposed in the Codec Configured state for that ASE.
            No greater than the lowest value of Presentation_Delay_Max that the Unicast Servers have
            respectively exposed in the Codec Configured state for that ASE.*/

            /* BAP v1.0.1 section7.2.1 Page 136
            For all ASEs where a Unicast Server is in the Audio Sink role (all Sink ASEs), the
            Max_Transport_Latency parameter value requested by the Unicast Client with each Unicast Server shall
            be no greater than the lowest value of Max_Transport_Latency that the Unicast Servers have respectively
            exposed in the Codec Configured state for those Sink ASEs.

            For all ASEs where a Unicast Server is in the Audio Source role (all Source ASEs), the
            Max_Transport_Latency parameter value requested by the Unicast Client with each Unicast Server shall
            be no greater than the lowest value of Max_Transport_Latency that the Unicast Servers have respectively
            exposed in the Codec Configured state for those Source ASEs.*/
            u8 cisNums = pAscsClt->cisNums;
            u8 svrNums = pAscsClt->svrNums;
            (void)svrNums;

            if(aseListCnt == 1)
            {
                BLT_ASCS_LOG("only one Ase");
                u8 cig_ret_buffer[sizeof(hci_le_setCigParam_retParam_t)+2*AUDIO_UNICAST_CLIENT_STREAMS];
                hci_le_setCigParam_retParam_t *pCigRetParam = (hci_le_setCigParam_retParam_t*)cig_ret_buffer;

                u8 cig_cmd_buffer[sizeof(hci_le_setCigParam_cmdParam_t)+AUDIO_UNICAST_CLIENT_STREAMS*sizeof(cigParam_cisCfg_t)];
                hci_le_setCigParam_cmdParam_t* pCigCmdParam = (hci_le_setCigParam_cmdParam_t*)cig_cmd_buffer;
                pAse->PresentationDly = minPreDly;

                if((prefPhy & 0x02) != 0 && (AUDIO_UNICAST_CLIENT_PREFERRED_PHY & 0x02) != 0)
                {
                    pAse->PHY = BLC_AUDIO_PHY_FLAG_2M;
                }
                else if((prefPhy & 0x01) != 0 && (AUDIO_UNICAST_CLIENT_PREFERRED_PHY & 0x01) != 0)
                {
                    pAse->PHY = BLC_AUDIO_PHY_FLAG_1M;
                }
                else if((prefPhy & 0x04) != 0 && (AUDIO_UNICAST_CLIENT_PREFERRED_PHY & 0x04) != 0)
                {
                    pAse->PHY = BLC_AUDIO_PHY_FLAG_CODED;
                }
                else
                {
                    pAse->PHY = BLC_AUDIO_PHY_FLAG_1M;
                }

                static u8 cig_set_index = 1;
                if(cig_set_index == 1)
                {

                    if(blt_audio_cap_ctrl.kmaMark)
                    {
                        cisBondingParam[0].aclHandle = 0x80+blt_audio_cap_ctrl.aclIdx1;
                        cisBondingParam[1].aclHandle = 0x80+blt_audio_cap_ctrl.aclIdx2;
                    }
                    else
                    {
                        for(u8 i=0;i<gAppAudioAclCentralNum;i++)
                        {
                            cisBondingParam[i].aclHandle = 0x80+i;
                        }
                    }

                    pCigCmdParam->cig_id = pAse->cigID;
                    pCigCmdParam->sca = SCA_MASTER_SLAVE_251_500_PPM;
                    pCigCmdParam->packing = cig_pack_format;
                    pCigCmdParam->framing = pAse->framing;
                    pCigCmdParam->cis_count = gAppAudioAclCentralNum;

                    pCigCmdParam->sdu_int_m2s[0] = U32_BYTE0(pAse->sduInterval);
                    pCigCmdParam->sdu_int_m2s[1] = U32_BYTE1(pAse->sduInterval);
                    pCigCmdParam->sdu_int_m2s[2] = U32_BYTE2(pAse->sduInterval);
                    pCigCmdParam->sdu_int_s2m[0] = U32_BYTE0(pAse->sduInterval);
                    pCigCmdParam->sdu_int_s2m[1] = U32_BYTE1(pAse->sduInterval);
                    pCigCmdParam->sdu_int_s2m[2] = U32_BYTE2(pAse->sduInterval);
                    pCigCmdParam->max_trans_lat_m2s = m2s ? pAse->maxTransLatency : 5;
                    pCigCmdParam->max_trans_lat_s2m = m2s ? 5 : pAse->maxTransLatency;

                    for(int t=0;t<cisNums;t++)
                    {
                        pCigCmdParam->cisCfg[t].cis_id = t;
                        pCigCmdParam->cisCfg[t].phy_m2s = pAse->PHY;//we should use symmetrical phy.
                        pCigCmdParam->cisCfg[t].phy_s2m = pAse->PHY;
                        pCigCmdParam->cisCfg[t].max_sdu_m2s = 0;
                        pCigCmdParam->cisCfg[t].max_sdu_s2m = 0;
                        pCigCmdParam->cisCfg[t].rtn_m2s = 0;
                        pCigCmdParam->cisCfg[t].rtn_s2m = 0;
                        if(m2s)
                        {
                            pCigCmdParam->cisCfg[t].max_sdu_m2s = pAse->maxSDU;
                            pCigCmdParam->cisCfg[t].rtn_m2s = pAse->RTN;
                        }
                        else
                        {
                            pCigCmdParam->cisCfg[t].max_sdu_s2m = pAse->maxSDU;
                            pCigCmdParam->cisCfg[t].rtn_s2m = pAse->RTN;
                        }
                    }

                    BLT_ASCS_LOG("CIG Param", hex_to_str(&pCigCmdParam->cig_id, sizeof(hci_le_setCigParam_cmdParam_t)+(gAppAudioAclCentralNum-1)*sizeof(cigParam_cisCfg_t)));

                    if(blt_audio_cap_ctrl.kmaMark)
                    {
                        blt_audio_unicastCltSetCigParamsEvt(pAscsClt->connHandle);
                        cig_set_index = 0;
                        BLT_ASCS_LOG("qos param ready,trigger send qos - Ase ID: %d", pAse->aseID);
                        for(int t=0;t<gAppAudioAclCentralNum;t++)
                        {
                            if(cisBondingParam[t].aclHandle == aclHandle)
                            {
                                pAse->cisHdl = cisBondingParam[t].cis_handle[0];
                                BLT_ASCS_LOG("cis handle: %d", pAse->cisHdl);
                            }
                        }
                        pAse->flags |= BLT_ASCSC_ASE_FLAG_SEND_QOS;
                        pAscsClt->aseFsmBusy = 1;
                    }
                    else
                    {
                        ble_sts_t status = blc_hci_le_setCigParams(pCigCmdParam, pCigRetParam);
                        if(status != BLE_SUCCESS)
                        {
                             BLT_ASCS_LOG("set CIG Param fail - status: %d", status);
                             return false;
                        }
                        else
                        {
                            cig_set_index = 0;
                            if(pCigRetParam->cig_id != pAse->cigID && pCigRetParam->cis_count != gAppAudioAclCentralNum)
                            {
                                BLT_ASCS_LOG("set CIG param return param error");
                                return false;
                            }

                            for(int t=0;t<gAppAudioAclCentralNum;t++)
                            {
                                cisBondingParam[t].cis_count = 1;
                                cisBondingParam[t].cis_handle[0] = pCigRetParam->cis_connHandle[t];
                                if(cisBondingParam[t].aclHandle == aclHandle)
                                {
                                    pAse->cisHdl = cisBondingParam[t].cis_handle[0];
                                    BLT_ASCS_LOG("set CIG param success - cis_handle: %d", pAse->cisHdl);
                                }
                            }

                            BLT_ASCS_LOG("qos param ready,trigger send qos - Ase ID: %d", pAse->aseID);
                            pAse->flags |= BLT_ASCSC_ASE_FLAG_SEND_QOS;
                            pAscsClt->aseFsmBusy = 1;
                        }
                    }
                }
                else
                {
                    for(int i=0;i<gAppAudioAclCentralNum;i++)
                    {
                        if(cisBondingParam[i].aclHandle == aclHandle)
                        {
                            BLT_ASCS_LOG("acl Handle: %d", cisBondingParam[i].aclHandle);
                            pAse->cisHdl = cisBondingParam[i].cis_handle[0];
                            pAse->flags |= BLT_ASCSC_ASE_FLAG_SEND_QOS;
                            BLT_ASCS_LOG("qos param ready,trigger send qos - Ase ID: %d", pAse->aseID);
                            pAscsClt->aseFsmBusy = 1;
                        }
                    }
                }
            }
            else if(aseListCnt > 1)
            {
                u8 aseCfgNum = 1;
                blt_ascsc_ase_t* pAseTmp = NULL;
                minPrefPreDly = pAseList[0]->prefMinPresentationDelay;
                maxPrefPreDly = pAseList[0]->prefMaxPresentationDelay;
                minPreDly = pAseList[0]->minPresentationDelay;
                maxPreDly = pAseList[0]->maxPresentationDelay;
                prefPhy = pAseList[0]->preferredPHY;
                u16 maxTransLatency = pAseList[0]->maxTransLatency;

                for(int i= 1; i<aseListCnt; i++)//
                {
                    pAseTmp = pAseList[i];
                    if(pAseTmp->state == BLT_ASCS_ASE_STATE_CODEC_CFG)
                    {
                        //Narrow down Preferred_Retransmission_Number
                        if(minPrefPreDly == 0)
                        {
                            minPrefPreDly = pAseTmp->prefMinPresentationDelay;
                        }
                        else if(pAseTmp->prefMinPresentationDelay)
                        {
                            minPrefPreDly = max(minPrefPreDly, pAseTmp->prefMinPresentationDelay);
                        }
                        if(maxPrefPreDly == 0)
                        {
                            maxPrefPreDly = pAseTmp->prefMaxPresentationDelay;
                        }
                        else if(pAseTmp->prefMaxPresentationDelay)
                        {
                            maxPrefPreDly = min(maxPrefPreDly, pAseTmp->prefMaxPresentationDelay);
                        }
                        //Narrow down Presentation_Delay
                        minPreDly = max(minPreDly, pAseTmp->minPresentationDelay);
                        maxPreDly = min(maxPreDly, pAseTmp->maxPresentationDelay);

                        //Narrow down preferred PHY bitfield
                        prefPhy &= pAseTmp->preferredPHY;

                        //Narrow down max Transport Latency
                        maxTransLatency = min(maxTransLatency, pAseTmp->maxTransLatency);
                        aseCfgNum++;
                    }
                }

                if(aseCfgNum == aseListCnt)
                {
                    BLT_ASCS_LOG("multiple Ase-num: %d", aseListCnt);
                    for(int i= 0; i<aseListCnt; i++)
                    {
                        pAseTmp = pAseList[i];
                        m2s = pAseTmp->dir == AUDIO_DIR_SINK ? 1 : 0;
                        if(minPrefPreDly)
                        {
                            pAse->PresentationDly = minPreDly;
                            pAseTmp->PresentationDly = minPrefPreDly;
                        }
                        else
                        {
                            pAse->PresentationDly = minPreDly;
                            pAseTmp->PresentationDly = minPreDly;
                        }

                        if((prefPhy & 0x02) != 0 && (AUDIO_UNICAST_CLIENT_PREFERRED_PHY & 0x02) != 0)
                        {
                            pAseTmp->PHY = BLC_AUDIO_PHY_FLAG_2M;
                        }
                        else if((prefPhy & 0x01) != 0 && (AUDIO_UNICAST_CLIENT_PREFERRED_PHY & 0x01) != 0)
                        {
                            pAseTmp->PHY = BLC_AUDIO_PHY_FLAG_1M;
                        }
                        else if((prefPhy & 0x04) != 0 && (AUDIO_UNICAST_CLIENT_PREFERRED_PHY & 0x04) != 0)
                        {
                            pAseTmp->PHY = BLC_AUDIO_PHY_FLAG_CODED;
                        }
                        else
                        {
                            pAseTmp->PHY = BLC_AUDIO_PHY_FLAG_1M;
                        }
                        pAseTmp->maxTransLatency = maxTransLatency;
                    }

                    static u8 cig_set_index = 1;
                    if(cig_set_index == 1)
                    {
                        if(blt_audio_cap_ctrl.kmaMark)
                        {
                            cisBondingParam[0].aclHandle = 0x80+blt_audio_cap_ctrl.aclIdx1;
                        }
                        else
                        {
                            for(u8 i=0;i<gAppAudioAclCentralNum;i++)
                            {
                                cisBondingParam[i].aclHandle = 0x80+i;
                            }
                        }

                        u8 cig_ret_buffer[sizeof(hci_le_setCigParam_retParam_t)+outCissCnt*gAppAudioAclCentralNum];
                        hci_le_setCigParam_retParam_t *pCigRetParam = (hci_le_setCigParam_retParam_t*)cig_ret_buffer;

                        u8 cig_cmd_buffer[sizeof(hci_le_setCigParam_cmdParam_t)+outCissCnt*gAppAudioAclCentralNum*sizeof(cigParam_cisCfg_t)];
                        hci_le_setCigParam_cmdParam_t* pCigCmdParam = (hci_le_setCigParam_cmdParam_t*)cig_cmd_buffer;
                        memset(pCigCmdParam,0,sizeof(hci_le_setCigParam_cmdParam_t)+outCissCnt*gAppAudioAclCentralNum*sizeof(cigParam_cisCfg_t));
                        pCigCmdParam->cig_id = pAseList[0]->cigID;
                        pCigCmdParam->packing = cig_pack_format; //refer to "packing_type_t"
                        pCigCmdParam->framing = pAseList[0]->framing; //refer to "framing_t"
                        memcpy(pCigCmdParam->sdu_int_m2s, &pAseList[0]->sduInterval,3);
                        memcpy(pCigCmdParam->sdu_int_s2m, &pAseList[0]->sduInterval,3);
                        pCigCmdParam->sca = SCA_MASTER_SLAVE_251_500_PPM;
                        pCigCmdParam->cis_count = outCissCnt * gAppAudioAclCentralNum;

                        u16 maxTransLatency_m2s = 5, maxTransLatency_s2m = 5;
                        for(int t=0;t<gAppAudioAclCentralNum;t++)
                        {
                            for(int i= 0; i<outCissCnt; i++)
                            {
                                pCigCmdParam->cisCfg[t*outCissCnt+i].rtn_m2s = 0;
                                pCigCmdParam->cisCfg[t*outCissCnt+i].rtn_s2m = 0;
                                for(int j= 0; j<outCissInfoList[i].cisNum; j++)
                                {
                                    m2s = outCissInfoList[i].pAseList[j]->dir == AUDIO_DIR_SINK ? 1 : 0;
                                    if(m2s)
                                    {
                                        BLT_ASCS_LOG("max sdu:m2s %d",outCissInfoList[i].pAseList[j]->maxSDU);
                                        BLT_ASCS_LOG("outCissInfoList[i].pAseList[j]->maxTransLatency %d",outCissInfoList[i].pAseList[j]->maxTransLatency);
                                        pCigCmdParam->cisCfg[t*outCissCnt+i].max_sdu_m2s = outCissInfoList[i].pAseList[j]->maxSDU;
                                        maxTransLatency_m2s = (max(maxTransLatency_m2s,outCissInfoList[i].pAseList[j]->maxTransLatency));
                                        pCigCmdParam->cisCfg[t*outCissCnt+i].rtn_s2m = outCissInfoList[i].pAseList[j]->RTN;
                                    }
                                    else
                                    {
                                        BLT_ASCS_LOG("max sdu:s2m %d",outCissInfoList[i].pAseList[j]->maxSDU);
                                        pCigCmdParam->cisCfg[t*outCissCnt+i].max_sdu_s2m = outCissInfoList[i].pAseList[j]->maxSDU;
                                        maxTransLatency_s2m = (max(maxTransLatency_s2m,outCissInfoList[i].pAseList[j]->maxTransLatency));
                                        pCigCmdParam->cisCfg[t*outCissCnt+i].rtn_s2m = outCissInfoList[i].pAseList[j]->RTN;
                                    }
                                }

                                pCigCmdParam->cisCfg[t*outCissCnt+i].cis_id = outCissInfoList[i].cisID+t*outCissCnt;
                                pCigCmdParam->cisCfg[t*outCissCnt+i].phy_m2s = pAse->PHY ;
                                pCigCmdParam->cisCfg[t*outCissCnt+i].phy_s2m = pAse->PHY ;
                            }
                        }
                        pCigCmdParam->max_trans_lat_m2s = maxTransLatency_m2s;
                        pCigCmdParam->max_trans_lat_s2m = maxTransLatency_s2m;

                        if(blt_audio_cap_ctrl.kmaMark)
                        {
                            blt_audio_unicastCltSetCigParamsEvt(pAscsClt->connHandle);
                            cig_set_index = 0;
                            if(cisBondingParam[0].aclHandle == aclHandle)
                            {
                                for(int j= 0; j<outCissInfoList[0].cisNum; j++)
                                {
                                    pAse = blt_ascsc_findAseByID(pAscsClt, outCissInfoList[0].pAseList[j]->aseID);
                                    pAse->cisHdl = cisBondingParam[0].cis_handle[0];
                                    pAse->flags |= BLT_ASCSC_ASE_FLAG_SEND_QOS;
                                    BLT_ASCS_LOG("qos param ready,trigger send qos - Ase ID: %d", pAse->aseID);
                                }
                                pAscsClt->aseFsmBusy = 1;
                                BLT_ASCS_LOG("cis_handle: 0x%x", cisBondingParam[0].cis_handle[0]);
                            }
                        }
                        else
                        {
                            ble_sts_t status = blc_hci_le_setCigParams(pCigCmdParam, pCigRetParam);
                            if(status != BLE_SUCCESS)
                            {
                                BLT_ASCS_LOG("CIG Param, %s", hex_to_str(&pCigCmdParam->cig_id, sizeof(hci_le_setCigParam_cmdParam_t)+(gAppAudioAclCentralNum-1)*sizeof(cigParam_cisCfg_t)));
                                BLT_ASCS_LOG("set CIG Param fail - CIG ID: %d",  status);
                                return false;
                            }
                            else
                            {
                                cig_set_index = 0;
                                if(pCigRetParam->cig_id != pAse->cigID || pCigRetParam->cis_count != outCissCnt*gAppAudioAclCentralNum)
                                {
                                    BLT_ASCS_LOG("set CIG param return param error");
                                    return false;
                                }
                                BLT_ASCS_LOG("handle0: 0x%x", pCigRetParam->cis_connHandle[0]);
                                BLT_ASCS_LOG("handle1: 0x%x", pCigRetParam->cis_connHandle[1]);
                                BLT_ASCS_LOG("set CIG Param, %s", hex_to_str(&pCigCmdParam->cig_id, sizeof(hci_le_setCigParam_cmdParam_t)+(outCissCnt*gAppAudioAclCentralNum-1)*sizeof(cigParam_cisCfg_t)));
                                for(int t=0;t<gAppAudioAclCentralNum;t++)
                                {
                                    cisBondingParam[t].cis_count = outCissCnt;
                                    for(int j=0;j<outCissCnt;j++)
                                    {
                                        cisBondingParam[t].cis_handle[j] = pCigRetParam->cis_connHandle[t*outCissCnt+j];
                                    }

                                    if(cisBondingParam[t].aclHandle == aclHandle)
                                    {
                                        for(int i= 0; i<outCissCnt; i++)
                                        {
                                            for(int j= 0; j<outCissInfoList[i].cisNum; j++)
                                            {
                                                pAse = blt_ascsc_findAseByID(pAscsClt, outCissInfoList[i].pAseList[j]->aseID);
                                                pAse->cisHdl = pCigRetParam->cis_connHandle[i];
                                                pAse->flags |= BLT_ASCSC_ASE_FLAG_SEND_QOS;
                                                BLT_ASCS_LOG("qos param ready,trigger send qos - Ase ID: %d", pAse->aseID);
                                            }
                                            BLT_ASCS_LOG("set CIG param success - cis_handle: 0x%x", pCigRetParam->cis_connHandle[i]);
                                        }
                                    }
                                }
                                pAscsClt->aseFsmBusy = 1;
                                blt_audio_unicastCltSetCigParamsEvt(pAscsClt->connHandle);
                            }
                        }
                    }
                    else
                    {
                        for(int i=0;i<gAppAudioAclCentralNum;i++)
                        {
                            if(cisBondingParam[i].aclHandle == pAscsClt->connHandle && outCissCnt == cisBondingParam[i].cis_count)
                            {
                                BLT_ASCS_LOG("acl Handle: 0x%x", cisBondingParam[i].aclHandle);
                                for(int j= 0; j<cisBondingParam[i].cis_count; j++)
                                {
                                    for(int t= 0; t<outCissInfoList[j].cisNum; t++)
                                    {
                                        pAse = blt_ascsc_findAseByID(pAscsClt, outCissInfoList[j].pAseList[t]->aseID);
                                        pAse->cisHdl = cisBondingParam[i].cis_handle[j];
                                        pAse->flags |= BLT_ASCSC_ASE_FLAG_SEND_QOS;
                                        BLT_ASCS_LOG("qos param ready,trigger send qos - Ase ID: %d", pAse->aseID);
                                    }
                                }
                                pAscsClt->aseFsmBusy = 1;
                            }
                        }
                    }
                }
            }
        }
        else//released
        {
            pAse->state = BLT_ASCS_ASE_STATE_CODEC_CFG;
        }
    }
    else if(aseState == BLT_ASCS_ASE_STATE_QOS_CFG)
    {
        BLT_ASCS_LOG("Ase state-qos configured");
        if(dataLen != sizeof(blt_ascsc_aseStateQosCfg_t))
        {
            BLT_ASCS_LOG("Invalid datalen");
            return false;
        }
        if(pAse->cigID != pascpState->u.qos.cigID &&\
           pAse->cisID != pascpState->u.qos.cisID &&\
           !memcmp(&pAse->sduInterval,pascpState->u.qos.SDUInterval,3) &&\
           pAse->framing != pascpState->u.qos.framing &&\
           pAse->PHY != pascpState->u.qos.PHY &&\
           pAse->maxSDU != pascpState->u.qos.maxSDU &&\
           pAse->RTN != pascpState->u.qos.retransmitNum &&\
           pAse->maxTransLatency != pascpState->u.qos.maxTransportLatency&&
           !memcmp(&pAse->PresentationDly,pascpState->u.qos.presentationDelay,3))
        {
            return false;
        }
        if(pAse->state == BLT_ASCS_ASE_STATE_QOS_CFG || pAse->state == BLT_ASCS_ASE_STATE_CODEC_CFG)
        {
            pAscsClt->aseFsmBusy = 1;
            pAse->state = BLT_ASCS_ASE_STATE_QOS_CFG;
            BLT_ASCS_LOG("trigger creat cis - ASE ID: %d", pAse->aseID);
            blt_audio_unicastCltQosCfgEvt(pAscsClt->connHandle, pAse);
            pAse->flags |= BLT_ASCSC_ASE_FLAG_CREATE_CIS;
            pAse->cisCreateRetryNum = 0; /* Init 0 */
        }
        else
        {
            pAse->state = BLT_ASCS_ASE_STATE_QOS_CFG;
        }
    }
    else if(aseState == BLT_ASCS_ASE_STATE_ENABLING)
    {
        BLT_ASCS_LOG("Ase state-enabling");
        bool metaChangeIndex = blt_audio_ascpMetadataCmp(pAse, pascpState->u.enable.metaDataLen, pascpState->u.enable.pMetaData);
        if(pAse->state == BLT_ASCS_ASE_STATE_ENABLING)
        {
            if(metaChangeIndex)
            {
                BLT_ASCS_LOG("update metadata in enabling-Ase ID: %d", pAse->aseID);
                blt_audio_unicastCltUpdateEvt(pAscsClt->connHandle, pAse);
            }
        }
        else if(pAse->state == BLT_ASCS_ASE_STATE_QOS_CFG)
        {
            if(pAse->cisEstablished)
            {
                /* If a Source ASE is in the Enabling state, the Unicast Client shall initiate the Receiver Start Ready
                 * operation for that ASE when the Unicast Client is ready to consume audio data transmitted from that ASE
                 * by the Unicast Server. */
//              if(pAse->dir == AUDIO_DIR_SOURCE)
//              {
//                  BLT_ASCS_LOG("Source Ase,client trigger receive start command: %d", pAse->aseID);
//                  pAse->flags |= BLT_ASCSC_ASE_FLAG_SEND_START;
//                  pAscsClt->aseFsmBusy = 1;
//              }
//              else
//              { //Sink ASE, Server will notify ASE state: Streaming
//                  BLT_ASCS_LOG("Sink Ase,server will initiate receive start operation autonomously: %d", pAse->aseID);
//                  pAse->flags &= ~BLT_ASCSC_ASE_FLAG_ENABLE_MASK;
//              }
                pAse->state = BLT_ASCS_ASE_STATE_ENABLING;
                blt_audio_unicastCltEnablingEvt(pAscsClt->connHandle, pAse);
            }
        }
    }
    else if(aseState == BLT_ASCS_ASE_STATE_STREAMING)
    {
        BLT_ASCS_LOG("Ase state-streaming");
        blc_audio_metadata_parsed_t metaParam = {0};
        u8 metadataChkSts = blt_audio_getMetadataParams(pascpState->u.stream.metaDataLen, pascpState->u.stream.pMetaData, &metaParam);
        if(metadataChkSts)
        {
            BLT_ASCS_LOG("Get Metadata param Error: %d", metadataChkSts);
            return false;
        }
        bool metaChangeIndex = blt_audio_ascpMetadataCmp(pAse, pascpState->u.stream.metaDataLen, pascpState->u.stream.pMetaData);
        if(metaChangeIndex)
        {
            BLT_ASCS_LOG("update metadata in streaming-Ase ID: %d", pAse->aseID);
            blt_audio_unicastCltUpdateEvt(pAscsClt->connHandle,pAse);
        }
        if(pAse->state == BLT_ASCS_ASE_STATE_ENABLING)
        {
            if(pAse->dir == AUDIO_DIR_SOURCE)
            {
                BLT_ASCS_LOG("server/source is ready ,client can receive data");
                /*
                 * Notice: There may be a problem here. The underlying SDU fifo is piled up,
                 * and the application layer does not get the data in time.
                 */
                //blt_audio_unicastCltRcvStreamEvt(pAscsClt->connHandle, pAse);
            }
            else
            {
                BLT_ASCS_LOG("server/sink is ready ,client can send data");
                blt_audio_unicastCltSendStreamEvt(pAscsClt->connHandle, pAse);
            }
        }
        pAse->flags &= ~BLT_ASCSC_ASE_FLAG_ENABLE_MASK;
        pAse->state = BLT_ASCS_ASE_STATE_STREAMING;
    }
    else if(aseState == BLT_ASCS_ASE_STATE_DISABLING)
    {
        BLT_ASCS_LOG("Ase state-disabling");
        if(pAse->dir != AUDIO_DIR_SOURCE)
        {
            return false;
        }
        blt_audio_unicastCltDisablingEvt(pAscsClt->connHandle, pAse);
    }
    else if(aseState == BLT_ASCS_ASE_STATE_RELEASING)
    {
        BLT_ASCS_LOG("Ase state-releasing");
        pAse->state = BLT_ASCS_ASE_STATE_RELEASING;
        blt_audio_unicastCltReleasingEvt(pAscsClt->connHandle, pAse);
    }
    else
    {
        BLT_ASCS_LOG("Ase state-idle");
        pAse->state = BLT_ASCS_ASE_STATE_IDLE;
    }
    return true;
}
bool blt_ascsc_rcvAseCtrlPntNtfDeal(blc_ascs_client_t *pAscsClt, u16 ntfHandle, u8 *pNtfVal, u16 ntfValLen)
{
    blt_ascsc_ase_t *pAse;
    u8 dataLen = ntfValLen;
    if(ntfHandle != pAscsClt->aseCtrlPntHdl || dataLen < 5)
    {
        return false;
    }

    blt_ascs_aseCtrlPointCharNtf_t *pAesCpNotify = (blt_ascs_aseCtrlPointCharNtf_t*)pNtfVal;
    u8 totalLen = sizeof(blt_ascs_aseCtrlPointCharNtf_t) + pAesCpNotify->numOfAses*sizeof(aseCtrlNtfPayload_t);
    if(totalLen != dataLen)
    {
        return false;
    }

    u8 aseID = 0;
    u8 reason = 0;
    u8 errCode = 0;

    for(u8 index=0; index<pAesCpNotify->numOfAses; index++)
    {
        aseID = pAesCpNotify->payload[index].aseID;
        reason = pAesCpNotify->payload[index].reason;
        errCode = pAesCpNotify->payload[index].responseCode;

        BLT_ASCS_LOG("receive notify on ctrl handle - Ase ID: %d", aseID);
        pAse = blt_ascsc_findAseByID(pAscsClt, aseID);
        if(pAse == NULL)
        {
            BLT_ASCS_LOG("ASE NULL: %d", aseID);
            continue;
        }
        if(reason != 0 || errCode != 0)
        {
            BLT_ASCS_LOG("error");
            if(pAse->flags & BLT_ASCSC_ASE_FLAG_ENABLE)
            {
                pAse->flags &= ~BLT_ASCSC_ASE_FLAG_ENABLE_MASK;
                pAse->state = BLT_ASCS_ASE_STATE_IDLE;
                continue;
            }
            else if(pAse->flags & BLT_ASCSC_ASE_FLAG_DISABLE)
            {
                pAse->flags &= ~BLT_ASCSC_ASE_FLAG_DISABLE_MASK;
                pAse->state = BLT_ASCS_ASE_STATE_IDLE;
                continue;
            }
        }
        if(reason == 0 && errCode == 0)
        {
            pAse->flags &= ~BLT_ASCSC_ASE_FLAG_SEND_WAIT;
            if(pAse->flags & BLT_ASCSC_ASE_FLAG_SEND_CODEC)
            {
                pAse->flags &= ~BLT_ASCSC_ASE_FLAG_SEND_CODEC;
                BLT_ASCS_LOG("clear send flag - send codec");
            }
            else if(pAse->flags & BLT_ASCSC_ASE_FLAG_SEND_QOS)
            {
                pAse->flags &= ~BLT_ASCSC_ASE_FLAG_SEND_QOS;
                BLT_ASCS_LOG("clear send flag - send qos");
            }
            else if(pAse->flags & BLT_ASCSC_ASE_FLAG_SEND_ENABLE)
            {
                pAse->flags &= ~BLT_ASCSC_ASE_FLAG_SEND_ENABLE;
                BLT_ASCS_LOG("clear send flag - send enable");
            }
            else if(pAse->flags & BLT_ASCSC_ASE_FLAG_SEND_DISABLE)
            {
                pAse->flags &= ~BLT_ASCSC_ASE_FLAG_SEND_DISABLE;
                BLT_ASCS_LOG("clear send flag - send disable");
            }
            else if(pAse->flags & BLT_ASCSC_ASE_FLAG_SEND_START)
            {
                pAse->flags &= ~BLT_ASCSC_ASE_FLAG_SEND_START;
                BLT_ASCS_LOG("clear send flag - send start");
            }
            else if(pAse->flags & BLT_ASCSC_ASE_FLAG_SEND_STOP)
            {
                pAse->flags &= ~BLT_ASCSC_ASE_FLAG_SEND_STOP;
                BLT_ASCS_LOG("clear send flag - send stop");
            }
            else if(pAse->flags & BLT_ASCSC_ASE_FLAG_SEND_RELEASE)
            {
                pAse->flags &= ~BLT_ASCSC_ASE_FLAG_SEND_RELEASE;
                BLT_ASCS_LOG("clear send flag - send release");
            }
            else if(pAse->flags & BLT_ASCSC_ASE_FLAG_SEND_UPDATE)
            {
                pAse->flags &= ~BLT_ASCSC_ASE_FLAG_SEND_UPDATE;
                BLT_ASCS_LOG("clear send flag - send update");
            }
        }
    }

    return true;
}

/*************************************************************************
 *  Client user interface functions
 *************************************************************************/
static int blt_audio_ascpSetAseCfg(blt_ascsc_ase_t *pAse, blc_ascsc_aseConfig_t *pCfg)
{
    pAse->cigID = pCfg->cigID;
    pAse->cisID = pCfg->cisID;
    pAse->codecId = pCfg->codecId;
    BLT_ASCS_LOG("ase config, %s", hex_to_str(pCfg, sizeof(blc_ascsc_aseConfig_t)));

    if(pCfg->frequency == 0 || pCfg->frequency > BLC_AUDIO_FREQ_CFG_48000)
    {
        return -AUDIO_EPARAM;
    }
    else
    {
        pAse->frequency = pCfg->frequency;
    }
    if(pCfg->duration != BLC_AUDIO_DURATION_CFG_7_5 && pCfg->duration != BLC_AUDIO_DURATION_CFG_10)
    {
        BLT_ASCS_LOG("duration unavailable: %d", pCfg->duration);
        return -AUDIO_EPARAM;
    }
    else
    {
        pAse->duration = pCfg->duration;
    }

    if(pCfg->frameOcts == 0)
    {
        BLT_ASCS_LOG("frameOcts unavailable: %d", pCfg->frameOcts);
        return -AUDIO_EPARAM;
    }
    else
    {
        pAse->frameOcts = pCfg->frameOcts;
    }
    if(pCfg->location == 0) //Optional,  0  means NOT present
    {
        BLT_ASCS_LOG("audio location unavailable");
    }
    else
    {
        pAse->location = pCfg->location;
    }
    if(pCfg->codecFrameBlksPerSDU == 0)
    {
        BLT_ASCS_LOG("codecFrmBlksPerSDU unavailable: %d",  pCfg->codecFrameBlksPerSDU);
        return -AUDIO_EPARAM;
    }
    else
    {
        pAse->codecFrmBlksPerSDU = pCfg->codecFrameBlksPerSDU;
    }
    return AUDIO_ESUCC;
}

int blt_ascsc_disableAse(u16 aclHandle, u8 aseID)
{
    blt_ascsc_ase_t *pAse;
    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(aclHandle);
    if(pAscsClt == NULL)
    {
        return AUDIO_EHANDLE;
    }
    if(pAscsClt->aseCtrlPntHdl == 0)
    {
        return AUDIO_ENOREADY;
    }
    pAse = blt_ascsc_findAseByID(pAscsClt, aseID);
    if(pAse == NULL)
    {
        return AUDIO_EASEID;
        BLT_ASCS_LOG("Ase Null");
    }
    if(pAse->state != BLT_ASCS_ASE_STATE_STREAMING && pAse->state != BLT_ASCS_ASE_STATE_ENABLING)
    {
        BLT_ASCS_LOG("Invalid Ase State: %d", pAse->state);
        return AUDIO_ESTATUS;
    }

    BLT_ASCS_LOG("client trigger : ase disable");
    pAse->flags |= BLT_ASCSC_ASE_FLAG_SEND_DISABLE;
    pAscsClt->aseFsmBusy = 1;

    return AUDIO_ESUCC;
}

int blt_ascsc_releaseAse(u16 aclHandle, u8 aseID)
{
    blt_ascsc_ase_t *pAse;
    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(aclHandle);
    if(pAscsClt == NULL)
    {
        return AUDIO_EHANDLE;
    }
    if(pAscsClt->aseCtrlPntHdl == 0)
    {
        return AUDIO_ENOREADY;
    }
    pAse = blt_ascsc_findAseByID(pAscsClt, aseID);
    if(pAse == NULL)
    {
        return AUDIO_EASEID;
    }
    if(pAse->state != BLT_ASCS_ASE_STATE_CODEC_CFG && \
       pAse->state != BLT_ASCS_ASE_STATE_QOS_CFG && \
       pAse->state != BLT_ASCS_ASE_STATE_ENABLING &&\
       pAse->state != BLT_ASCS_ASE_STATE_STREAMING && \
       pAse->state != BLT_ASCS_ASE_STATE_DISABLING)
    {
        BLT_ASCS_LOG("Invalid Ase State: %d", pAse->state);
        return AUDIO_ESTATUS;
    }

    if(pAse->flags & BLT_ASCSC_ASE_FLAG_SEND_RELEASE)
    {
        return AUDIO_EBUSY;
    }
    BLT_ASCS_LOG("client trigger : ase release");
    pAse->flags = BLT_ASCSC_ASE_FLAG_SEND_RELEASE;
    pAse->flags |= BLT_ASCSC_ASE_FLAG_DESTROY_CIS;
    pAse->ready &= ~BLT_ASCSC_ASE_CODEC_READY;

    pAscsClt->aseFsmBusy = 1;

    return AUDIO_ESUCC;
}

int blt_ascsc_enableAse(u16 aclHandle, u8 aseID)
{
    blt_ascsc_ase_t *pAse;
    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(aclHandle);
    if(pAscsClt == NULL)
    {
        return AUDIO_EHANDLE;
    }
    if(pAscsClt->aseCtrlPntHdl == 0)
    {
        return AUDIO_ENOREADY;
    }
    pAse = blt_ascsc_findAseByID(pAscsClt, aseID);
    if(pAse == NULL)
    {
        return AUDIO_EASEID;
    }

    if((pAse->flags & BLT_ASCSC_ASE_FLAG_DESTROY_CIS) || (pAse->flags & BLT_ASCSC_ASE_FLAG_DESTROY_CIS_WAIT))
    {
        return -AUDIO_EBUSY;
    }
    if((pAse->flags & BLT_ASCSC_ASE_FLAG_CREATE_CIS) || (pAse->flags & BLT_ASCSC_ASE_FLAG_CREATE_CIS_WAIT))
    {
        return AUDIO_ESUCC;
    }

    if((pAse->ready & BLT_ASCSC_ASE_PARAM_READY) == 0)
    {
        return AUDIO_ENOREADY;
    }

    if(pAse->flags != 0)
    {
        return AUDIO_EBUSY;
    }
    if(pAse->state == BLT_ASCS_ASE_STATE_STREAMING || pAse->state == BLT_ASCS_ASE_STATE_DISABLING || pAse->state == BLT_ASCS_ASE_STATE_ENABLING)
    {
        BLT_ASCS_LOG("Invalid state: %d", pAse->aseID);
        return AUDIO_ESTATUS;
    }

    if(pAse->state == BLT_ASCS_ASE_STATE_IDLE)
    {
        pAse->flags = BLT_ASCSC_ASE_FLAG_SEND_CODEC;
        BLT_ASCS_LOG("ase - idle,trigger send codec: %d", pAse->aseID);
    }
    else if(pAse->state == BLT_ASCS_ASE_STATE_CODEC_CFG)
    {
        pAse->flags = BLT_ASCSC_ASE_FLAG_SEND_QOS;
        BLT_ASCS_LOG("ase - codec configured,trigger send qos: %d", pAse->aseID);
    }
    else if(pAse->state == BLT_ASCS_ASE_STATE_QOS_CFG)
    {
        BLT_ASCS_LOG("ase - qos configured,trigger send enable: %d", pAse->aseID);
        if(pAse->cisHdl == 0)
        {
            pAse->flags = BLT_ASCSC_ASE_FLAG_CREATE_CIS;
            BLT_ASCS_LOG("ase - cis not established,trigger creat cis");
        }
        pAse->flags |= BLT_ASCSC_ASE_FLAG_SEND_ENABLE;
    }
    else
    {
        return AUDIO_ESTATUS;
    }
    pAse->flags |= BLT_ASCSC_ASE_FLAG_ENABLE;
    pAscsClt->aseFsmBusy = 1;

    return AUDIO_ESUCC;
}

int blt_ascsc_setMetadata(u16 aclHandle, u8 aseID, u8 *pMetadata, u8 metadataLen)
{
    blt_ascsc_ase_t *pAse;
    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(aclHandle);
    BLT_ASCS_LOG("set metadata");
    if(pAscsClt == NULL)
    {
        return AUDIO_EHANDLE;
    }
    pAse = blt_ascsc_findAseByID(pAscsClt,aseID);
    if(pAse == NULL)
    {
        return AUDIO_EASEID;
    }

    blc_audio_metadata_parsed_t newMetadata;
    u8 metadataChkSts = blt_audio_getMetadataParams(metadataLen, pMetadata, &newMetadata);
    if(metadataChkSts)
    {
        return AUDIO_EPARAM;
    }

    bool metadataChg = blt_audio_ascpMetadataCmp(pAse, metadataLen, pMetadata);
    (void)metadataChg;

    return AUDIO_ESUCC;
}
int blt_ascsc_updateMetadata(u16 aclHandle, u8 aseID, u8 *pMetadata, u8 metadataLen)
{
    blt_ascsc_ase_t *pAse;
    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(aclHandle);
    if(pAscsClt == NULL)
    {
        return AUDIO_EHANDLE;
    }
    if(pAscsClt->aseCtrlPntHdl == 0)
    {
        return AUDIO_ENOREADY;
    }
    pAse = blt_ascsc_findAseByID(pAscsClt, aseID);
    if(pAse == NULL)
    {
        return AUDIO_EASEID;
    }
    if(pAse->state != BLT_ASCS_ASE_STATE_ENABLING && pAse->state != BLT_ASCS_ASE_STATE_STREAMING)
    {
        return AUDIO_ESTATUS;
    }

    bool metadataChg = blt_audio_ascpMetadataCmp(pAse, metadataLen, pMetadata);

    if(metadataChg)
    {
        pAse->flags = BLT_ASCSC_ASE_FLAG_SEND_UPDATE;
        pAscsClt->aseFsmBusy = 1;
    }
    else
    {
        return AUDIO_EREPEAT;
    }
    return AUDIO_ESUCC;
}

int blt_ascsc_setAseCfg(u16 connHandle, u8 aseID, blc_ascsc_aseConfig_t *pCfg)
{
    u8  index;
    blt_ascsc_ase_t *pAse;
    if(aseID == 0 || pCfg == NULL)
    {
        return AUDIO_EPARAM;
    }

    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(connHandle);
    if(pAscsClt == NULL)
    {
        return AUDIO_EHANDLE;
    }

    pAse = NULL;
    for(index=0; index<pAscsClt->aseCount; index++)
    {
        if(index < pAscsClt->sinkAseNum)
        {
            pAse = pAscsClt->pSinkAse[index];
        }
        else
        {
            pAse = pAscsClt->pSrcAse[index - pAscsClt->sinkAseNum];
        }

        if(pAse->aseID == aseID)
        {
            break;
        }
    }
    if(index == pAscsClt->aseCount)
    {
        return AUDIO_EASEID;
    }

    if(pAse->state != BLT_ASCS_ASE_STATE_IDLE)
    {
        return AUDIO_ESTATUS;
    }

    if(blt_audio_ascpSetAseCfg(pAse, pCfg) == AUDIO_ESUCC)
    {
        pAse->ready |= BLT_ASCSC_ASE_PARAM_READY;
        BLT_ASCS_LOG("ase config success");
        return AUDIO_ESUCC;
    }
    return AUDIO_EFAIL;
}

int blt_ascsc_setAllAseCfg(u16 connHandle, blc_ascsc_aseConfig_t *pCfg)
{
    int ret;
    u8  index;
    blt_ascsc_ase_t *pAse;
    if(pCfg == NULL)
    {
        return AUDIO_EPARAM;
    }

    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(connHandle);
    if(pAscsClt == NULL)
    {
        return AUDIO_EHANDLE;
    }

    ret = AUDIO_ESUCC;
    for(index=0; index<pAscsClt->aseCount; index++)
    {
        if(index < pAscsClt->sinkAseNum)
        {
            pAse = pAscsClt->pSinkAse[index];
        }
        else
        {
            pAse = pAscsClt->pSrcAse[index - pAscsClt->sinkAseNum];
        }

        if(blt_audio_ascpSetAseCfg(pAse, pCfg) == AUDIO_ESUCC)
        {
            pAse->ready |= BLT_ASCSC_ASE_PARAM_READY;
        }
        else
        {
            ret = AUDIO_EFAIL;
        }
    }

    return ret;
}

int blt_ascsc_removeCIGByAse(u16 connHandle, u8 aseID)
{
    u8 index;
    ble_sts_t ret;
    hci_le_removeCig_retParam_t retParam;
    blt_ascsc_ase_t *pAse;
    blt_ascsc_ase_t *pTemp;
    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(connHandle);
    if(pAscsClt == NULL)
    {
        return AUDIO_EHANDLE;
    }

    pAse = blt_ascsc_findAseByID(pAscsClt, aseID);
    if(pAse == NULL)
    {
        return AUDIO_EASEID;
    }

    if(pAse->cisHdl == 0 || (pAse->flags & BLT_ASCSC_ASE_FLAG_DESTROY_CIS_WAIT) != 0)
    {
        return AUDIO_ESUCC;
    }
    ret = blc_hci_le_removeCig(pAse->cigID, &retParam);
    if(ret != BLE_SUCCESS)
    {
        return AUDIO_EFAIL;
    }

    for(index=0; index<pAscsClt->aseCount; index++)
    {
        if(index < pAscsClt->sinkAseNum)
        {
            pTemp = pAscsClt->pSinkAse[index];
        }
        else
        {
            pTemp = pAscsClt->pSrcAse[index - pAscsClt->sinkAseNum];
        }

        if(pTemp->cigID == pAse->cigID && pTemp->cisHdl != 0)
        {
            pTemp->flags |= BLT_ASCSC_ASE_FLAG_DESTROY_CIS_WAIT;
        }
    }

    pAse->flags |= BLT_ASCSC_ASE_FLAG_DESTROY_CIS_WAIT;

    return AUDIO_ESUCC;
}

int blt_ascsc_termCisByAse(u16 connHandle, u8 aseID, u8 reason)
{
    ble_sts_t ret = BLE_SUCCESS;
    blt_ascsc_ase_t *pAse;
    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(connHandle);
    if(pAscsClt == NULL)
    {
        return AUDIO_EHANDLE;
    }

    pAse = blt_ascsc_findAseByID(pAscsClt, aseID);
    if(pAse == NULL)
    {
        return AUDIO_EASEID;
    }

    if(pAse->cisHdl == 0 || (pAse->flags & BLT_ASCSC_ASE_FLAG_DESTROY_CIS_WAIT) != 0)
    {
        return AUDIO_ESUCC;
    }
    ret = blc_ll_cis_disconnect(pAse->cisHdl, reason);
    if(ret != BLE_SUCCESS)
    {
        return AUDIO_EFAIL;
    }
    pAse->flags |= BLT_ASCSC_ASE_FLAG_DESTROY_CIS_WAIT;
    return AUDIO_ESUCC;
}


/*************************************************************************
 *    ASCS Client mainloop process
 *************************************************************************/
static int  blt_ascsc_createCis(blc_ascs_client_t *pAscsClt, blt_ascsc_ase_t *pAse)
{
    u8 index;
    blt_ascsc_ase_t *pTemp;

    if(pAscsClt == NULL)
    {
        return -AUDIO_EHANDLE;
    }

    if(pAse->cisEstablished)
    {
        return AUDIO_ESUCC;
    }
    for(index=0; index<pAscsClt->aseCount; index++)
    {
        if(index < pAscsClt->sinkAseNum)
        {
            pTemp = pAscsClt->pSinkAse[index];
        }
        else
        {
            pTemp = pAscsClt->pSrcAse[index - pAscsClt->sinkAseNum];
        }
        if(pTemp->flags & BLT_ASCSC_ASE_FLAG_CREATE_CIS_WAIT)
        {// previous cis is creating.
            return -AUDIO_EFAIL;
        }
    }
    BLT_ASCS_LOG("start creat cis--Ase ID: %d", pAse->aseID);
    for(index=0; index<pAscsClt->aseCount; index++)
    {
        if(index < pAscsClt->sinkAseNum)
        {
            pTemp = pAscsClt->pSinkAse[index];
        }
        else
        {
            pTemp = pAscsClt->pSrcAse[index - pAscsClt->sinkAseNum];
        }

        if(!(pAse->ready & BLT_ASCSC_ASE_CODEC_READY))
        {
            continue;
        }
        if((pTemp->cigID == pAse->cigID) && (pTemp->cisID == pAse->cisID) && (pTemp->flags & BLT_ASCSC_ASE_FLAG_CREATE_CIS_WAIT))
        {
            BLT_ASCS_LOG("cis is creating - related ASE ID: %d", pTemp->aseID);
            pAse->flags |= BLT_ASCSC_ASE_FLAG_CREATE_CIS_WAIT;
            return AUDIO_ESUCC;
        }
        if((pTemp->cigID == pAse->cigID) && (pTemp->cisID == pAse->cisID) && pTemp->cisEstablished)
        {
            pAse->cisHdl = pTemp->cisHdl;
            pAse->cisEstablished = 1;
            pAse->flags |= BLT_ASCSC_ASE_FLAG_SEND_ENABLE;
            BLT_ASCS_LOG("cis already created - related ASE ID: %d", pTemp->aseID);
            return AUDIO_ESUCC;
        }
    }

    hci_le_CreateCisParams_t cisParam;

    cisParam.cis_count = 1;

    cisParam.cisConn[0].acl_handle = pAscsClt->connHandle;
    cisParam.cisConn[0].cis_handle = pAse->cisHdl;
    ble_sts_t status = blc_hci_le_createCis(&cisParam);
    if(status != BLE_SUCCESS)
    {
        BLT_ASCS_LOG("set command-Create Cis fail: %d", status);
        return -AUDIO_EFAIL;
    }
    pAse->flags |= BLT_ASCSC_ASE_FLAG_CREATE_CIS_WAIT;
    BLT_ASCS_LOG("set command-Create Cis success--Ase ID: %d", pAse->aseID);
    return AUDIO_ESUCC;
}
static void blt_ascsc_aseProcHandler(blc_ascs_client_t *pAscsClt, blt_ascsc_ase_t *pAse)
{
    if((pAse->flags & BLT_ASCSC_ASE_FLAG_SEND_WAIT)||\
       (pAse->flags & BLT_ASCSC_ASE_FLAG_CREATE_CIS_WAIT)||\
       (pAse->flags & BLT_ASCSC_ASE_FLAG_DESTROY_CIS_WAIT))
    {
        return;
    }
    if(pAse->flags & BLT_ASCSC_ASE_FLAG_CREATE_CIS)
    {
        if(blt_ascsc_createCis(pAscsClt, pAse) == AUDIO_ESUCC)
        {
            pAse->flags &= ~BLT_ASCSC_ASE_FLAG_CREATE_CIS;
        }
        else
        {
            return;
        }
    }
    else if(pAse->flags & BLT_ASCSC_ASE_FLAG_DESTROY_CIS)
    {
        if(blt_ascsc_termCisByAse(pAscsClt->connHandle,pAse->aseID,HCI_ERR_REMOTE_USER_TERM_CONN) == AUDIO_ESUCC)
        {
            pAse->flags &= ~BLT_ASCSC_ASE_FLAG_DESTROY_CIS;
        }
        else
        {
            return;
        }
    }
    else if(pAse->flags & BLT_ASCSC_ASE_FLAG_SEND_CODEC)
    {
        blt_ascsc_ase_t *pAseList[] = { pAse };
        if(blc_ascsc_writeConfigCodec(pAscsClt->connHandle, pAseList, 1) == BLE_SUCCESS)
        //if(blt_ascsc_writeAseCodec(pAscsClt, pAse) == AUDIO_ESUCC)
        {
            pAse->flags |= BLT_ASCSC_ASE_FLAG_SEND_WAIT;
        }
    }
    else if(pAse->flags & BLT_ASCSC_ASE_FLAG_SEND_QOS)
    {
        blt_ascsc_ase_t* pAseList[STACK_AUDIO_ASCS_ASE_NUM] = {0};
        u8 aseListCnt = blt_acscc_getAseListWithSameCigID(pAscsClt, pAseList);

        if(blc_ascsc_writeQosConfig(pAscsClt->connHandle, pAseList, aseListCnt) == BLE_SUCCESS)
        //if(blt_ascsc_writeAseQOS(pAscsClt, pAse) == AUDIO_ESUCC)
        {
            for(int i = 0; i < aseListCnt; i++){
                pAseList[i]->flags |= BLT_ASCSC_ASE_FLAG_SEND_WAIT;
            }
            //pAse->flags |= BLT_ASCSC_ASE_FLAG_SEND_WAIT;
        }
    }
    else if(pAse->flags & BLT_ASCSC_ASE_FLAG_SEND_ENABLE)
    {
        if((pAse->ready & BLT_ASCSC_ASE_CODEC_READY) && pAse->cisHdl != 0
            && (pAse->flags & BLT_ASCSC_ASE_FLAG_CREATE_CIS_WAIT) == 0
            && (pAse->flags & BLT_ASCSC_ASE_FLAG_DESTROY_CIS_WAIT) == 0)
        {
            blt_ascsc_ase_t *pAseList[] = { pAse };
            if(blc_ascsc_writeEnable(pAscsClt->connHandle, pAseList, 1) == BLE_SUCCESS)
            //if(blt_ascsc_writeAseEnable(pAscsClt, pAse) == AUDIO_ESUCC)
            {
                pAse->flags |= BLT_ASCSC_ASE_FLAG_SEND_WAIT;
            }
        }
    }
    else if(pAse->flags & BLT_ASCSC_ASE_FLAG_SEND_DISABLE)
    {
        blt_ascsc_ase_t *pAseList[] = { pAse };
        if(blc_ascsc_writeDisable(pAscsClt->connHandle, pAseList, 1) == BLE_SUCCESS)
        //if(blt_ascsc_writeAseDisable(pAscsClt, pAse) == AUDIO_ESUCC)
        {
            pAse->flags |= BLT_ASCSC_ASE_FLAG_SEND_WAIT;
        }
    }
    else if(pAse->flags & BLT_ASCSC_ASE_FLAG_SEND_START)
    {
        blt_ascsc_ase_t *pAseList[] = { pAse };
        if(blc_ascsc_writeRcvStartRdy(pAscsClt->connHandle, pAseList, 1) == BLE_SUCCESS)
        //if(blt_ascsc_writeAseStart(pAscsClt, pAse) == AUDIO_ESUCC)
        {
            pAse->flags |= BLT_ASCSC_ASE_FLAG_SEND_WAIT;
            /*
             * Notice: Event placed here will not cause the accumulation of underlying SDU fifo,
             * and the application layer can get the data in time.
             */
            blt_audio_unicastCltRcvStreamEvt(pAscsClt->connHandle, pAse);
        }
    }
    else if(pAse->flags & BLT_ASCSC_ASE_FLAG_SEND_STOP)
    {
        blt_ascsc_ase_t *pAseList[] = { pAse };
        if(blc_ascsc_writeRcvStopRdy(pAscsClt->connHandle, pAseList, 1) == BLE_SUCCESS)
        //if(blt_ascsc_writeAseStop(pAscsClt, pAse) == AUDIO_ESUCC)
        {
            pAse->flags |= BLT_ASCSC_ASE_FLAG_SEND_WAIT;
        }
    }
    else if(pAse->flags & BLT_ASCSC_ASE_FLAG_SEND_RELEASE)
    {
        blt_ascsc_ase_t *pAseList[] = { pAse };
        if(blc_ascsc_writeRelease(pAscsClt->connHandle, pAseList, 1) == BLE_SUCCESS)
        //if(blt_ascsc_writeAseRelease(pAscsClt, pAse) == AUDIO_ESUCC)
        {
            pAse->flags |= BLT_ASCSC_ASE_FLAG_SEND_WAIT;
        }
    }
    else if(pAse->flags & BLT_ASCSC_ASE_FLAG_SEND_UPDATE)
    {
        blt_ascsc_ase_t *pAseList[] = { pAse };
        if(blc_ascsc_writeUptMetadata(pAscsClt->connHandle, pAseList, 1) == BLE_SUCCESS)
        //if(blt_ascsc_writeAseUpdate(pAscsClt, pAse) == AUDIO_ESUCC)
        {
            pAse->flags |= BLT_ASCSC_ASE_FLAG_SEND_WAIT;
        }
    }
    else
    {
        pAse->flags = 0;
    }
}
void blt_audio_ascpProcess(blc_ascs_client_t *pAscsClt)
{
    u8 index;
    bool isBusy = false;
    blt_ascsc_ase_t *pAse = NULL;

    /* Separate ASE process */
    for(index=0; index<pAscsClt->aseCount; index++)
    {
        if(index < pAscsClt->sinkAseNum)
        {
            pAse = pAscsClt->pSinkAse[index];
        }
        else
        {
            pAse = pAscsClt->pSrcAse[index - pAscsClt->sinkAseNum];
        }

        if(pAse->flags!= 0)
        {
            blt_ascsc_aseProcHandler(pAscsClt, pAse);

            if(!isBusy && pAse->flags != 0)
            {
                isBusy = true;
            }
        }
    }

    if(!isBusy)
    {
        BLT_ASCS_LOG("clear ascp busy index");
        pAscsClt->aseFsmBusy = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////







