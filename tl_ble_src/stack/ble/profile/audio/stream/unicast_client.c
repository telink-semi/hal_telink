/********************************************************************************************************
 * @file    unicast_client.c
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

static void blt_bap_leStackEvtForUnicastClt(u32 h, u8 *p, int len)
{
    (void)len;
    //Controller HCI event
    if (h & HCI_FLAG_EVENT_BT_STD) {
        u8 evtCode     = h & 0xff;
        u8 subEvt_code = p[0];
        //------------ HCI event: LE CIS disconnect -------------------------------------
        if (evtCode == HCI_EVT_DISCONNECTION_COMPLETE) {
            hci_disconnectionCompleteEvt_t *pDisConn   = (hci_disconnectionCompleteEvt_t *)p;
            u16                             connHandle = pDisConn->connHandle;
            if (connHandle & BLT_CIS_HANDLE) {
#if (0)
                int aclHandle = blt_audio_getAclHdlByCisHdl(connHandle);
                BLT_BAP_LOG("Unicast Client: CIS disconnect:cisHdl[0x%X] aclHdl[0x%X]", connHandle, aclHandle);
#endif

                if (blt_audio_cap_ctrl.kmaMark) {
                    u8 cur_aclCentral_idx = blc_ll_getAclCentralIndex_arrangedTaskTiming_diffMode(pDisConn->connHandle);
                    if (cur_aclCentral_idx != blt_audio_cap_ctrl.aclIdx1 && cur_aclCentral_idx != blt_audio_cap_ctrl.aclIdx2 && cur_aclCentral_idx != blt_audio_cap_ctrl.aclIdx3 && cur_aclCentral_idx != blt_audio_cap_ctrl.aclIdx4) {
                        return;
                    }
                }
                /* app event callback */
                blt_audio_sendCisDisconnEvt(pDisConn);

                /* profile layer process cis disconnect event. */
                audio_error_enum ret = blt_ascsc_cisDisconnEvt(connHandle, p);
                BLT_BAP_LOG("blt_audio_sendCisDisconnEvt:0x%x", ret);
            }
        }
        //------HCI LE event: LE CIS established event -------------------------------
        else if (evtCode == HCI_EVT_LE_META && subEvt_code == HCI_SUB_EVT_LE_CIS_ESTABLISHED) {
            hci_le_cisEstablishedEvt_t *pCisEstbEvt = (hci_le_cisEstablishedEvt_t *)p;
            BLT_BAP_LOG("Unicast Client: CIS established: 0x%x", pCisEstbEvt->cisHandle);
            /* profile layer process cis established event. */
            blt_ascsc_cisConnectEvt(pCisEstbEvt->cisHandle, p);
            /* app event callback */
            blt_audio_sendCisConnEvt(pCisEstbEvt);
        }
    }
}

void blt_audio_unicastCltSetCigParamsEvt(u16 connHandle)
{
    blt_prf_sendEvent(connHandle, AUDIO_EVT_BAPUC_SET_CIG_PARAMS, NULL, 0);
}

void blt_audio_unicastCltEnablingEvt(u16 connHandle, blt_ascsc_ase_t *pAse)
{
    blc_bapuc_enablingEvt_t enableEvt;
    enableEvt.aseID     = pAse->aseID;
    enableEvt.aclHandle = connHandle;
    enableEvt.aseDir    = pAse->dir;
    blt_audio_getMetadataParams(pAse->metadataLen, pAse->pMetadata, &enableEvt.metaParam);

    blt_prf_sendEvent(connHandle, AUDIO_EVT_BAPUC_ENABLING, (u8 *)&enableEvt, sizeof(blc_bapuc_enablingEvt_t));
}

void blt_audio_unicastCltUpdateEvt(u16 connHandle, blt_ascsc_ase_t *pAse)
{
    blc_bapuc_updateMetadataEvt_t updateEvt;
    updateEvt.aseID     = pAse->aseID;
    updateEvt.aclHandle = connHandle;
    updateEvt.aseDir    = pAse->dir;
    blt_audio_getMetadataParams(pAse->metadataLen, pAse->pMetadata, &updateEvt.metaParam);

    blt_prf_sendEvent(connHandle, AUDIO_EVT_BAPUC_UPDATE_METADATA, (u8 *)&updateEvt, sizeof(blc_bapuc_updateMetadataEvt_t));
}

void blt_audio_unicastCltRcvStreamEvt(u16 connHandle, blt_ascsc_ase_t *pAse)
{
    blc_bapuc_receiveStreamingEvt_t startEvt;
    startEvt.aseID     = pAse->aseID;
    startEvt.aclHandle = connHandle;
    blt_prf_sendEvent(connHandle, AUDIO_EVT_BAPUC_RECEIVE_STREAMING, (u8 *)&startEvt, sizeof(blc_bapuc_receiveStreamingEvt_t));
}

void blt_audio_unicastCltSendStreamEvt(u16 connHandle, blt_ascsc_ase_t *pAse)
{
    blc_bapuc_sendStreamingEvt_t startEvt;

    startEvt.aseID     = pAse->aseID;
    startEvt.aclHandle = connHandle;
    blt_prf_sendEvent(connHandle, AUDIO_EVT_BAPUC_SEND_STREAMING, (u8 *)&startEvt, sizeof(blc_bapuc_sendStreamingEvt_t));
}

void blt_audio_unicastCltDisablingEvt(u16 connHandle, blt_ascsc_ase_t *pAse)
{
    blc_bapuc_disablingEvt_t disableEvt;

    disableEvt.aclHandle = connHandle;
    disableEvt.aseID     = pAse->aseID;
    disableEvt.aseDir    = pAse->dir;
    blt_prf_sendEvent(connHandle, AUDIO_EVT_BAPUC_DISABLING, (u8 *)&disableEvt, sizeof(blc_bapuc_disablingEvt_t));
}

void blt_audio_unicastCltReleasingEvt(u16 connHandle, blt_ascsc_ase_t *pAse)
{
    blc_bapuc_releasingEvt_t releaseEvt;

    releaseEvt.aclHandle = connHandle;
    releaseEvt.aseID     = pAse->aseID;
    releaseEvt.aseDir    = pAse->dir;
    blt_prf_sendEvent(connHandle, AUDIO_EVT_BAPUC_RELEASING, (u8 *)&releaseEvt, sizeof(blc_bapuc_releasingEvt_t));
}

void blt_audio_unicastCltCodecCfgEvt(u16 connHandle, blt_ascsc_ase_t *pAse, blt_ascsc_aseStateCodecCfg_t *codecParam)
{
    blc_bapuc_codecConfiguredEvt_t qosReqEvt;
    qosReqEvt.aclHandle              = connHandle;
    qosReqEvt.aseDir                 = pAse->dir;
    qosReqEvt.aseID                  = pAse->aseID;
    qosReqEvt.framing                = codecParam->framing;
    qosReqEvt.PreferredRetransmitNum = codecParam->prefRetransmitNum;
    qosReqEvt.maxTransportLatency    = codecParam->maxTransportLatency;
    blt_prf_sendEvent(connHandle, AUDIO_EVT_BAPUC_CODEC_CONFIGURED, (u8 *)&qosReqEvt, sizeof(blc_bapuc_codecConfiguredEvt_t));
}

void blt_audio_unicastCltQosCfgEvt(u16 connHandle, blt_ascsc_ase_t *pAse)
{
    blc_bapuc_qosConfiguredEvt_t qosCfgEvt;
    qosCfgEvt.aclHandle         = connHandle;
    qosCfgEvt.aseDir            = pAse->dir;
    qosCfgEvt.aseID             = pAse->aseID;
    qosCfgEvt.PHY               = pAse->PHY;
    qosCfgEvt.retransNum        = pAse->RTN;
    qosCfgEvt.maxSdu            = pAse->maxSDU;
    qosCfgEvt.maxTransLatency   = pAse->maxTransLatency;
    qosCfgEvt.sduInterval       = pAse->sduInterval;
    qosCfgEvt.presentationDelay = pAse->PresentationDly;
    blt_prf_sendEvent(connHandle, AUDIO_EVT_BAPUC_QOS_CONFIGURED, (u8 *)&qosCfgEvt, sizeof(blc_bapuc_qosConfiguredEvt_t));
}

void blc_audio_registerBapUnicastClient(const blc_bapuc_regParam_t *param)
{
    blc_audio_registerPACSControlClient(param->pPacsParam);
    blc_audio_registerASCSControlClient(param->pAscsParam);

    /* LE stack event callback for BAP Unicast Client role */
    bap_unicast_clt_cb = blt_bap_leStackEvtForUnicastClt;
}

int blc_bapuc_checkAudioConfigures(u16 aclHandle, std_unicast_aud_cfg_enum audCfgIdx, blc_audio_ase_cfg_info_t *outChnInfo)
{
    if (audCfgIdx >= BLC_AUDIO_STD_AUDIO_CONFIGURATIONS_E_MAX) {
        BLT_BAP_LOG("Invalid input: audio configurations");
        return AUDIO_EPARAM;
    }

    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(aclHandle);
    if (pAscsClt == NULL) {
        BLT_BAP_LOG("Invalid ASCS not found");
        return AUDIO_EHANDLE;
    }

    blc_pacs_client_t *pPacsClt = blt_pacsc_getClientInst(aclHandle);
    if (pPacsClt == NULL) {
        BLT_BAP_LOG("Invalid PACS not found");
        return AUDIO_EHANDLE;
    }

    std_unicast_aud_cfg_t const *pUsedAudCfg = &unicastAudioConfigurations[audCfgIdx];

    if (pUsedAudCfg->numOfSvr > gAppAudioAclCentralNum) {
        BLT_BAP_LOG("Invalid Server number not support");
        return AUDIO_ERR_PARAM_INVALID;
    }

    if (pUsedAudCfg->numOfSvr >= 3) {
        BLT_BAP_LOG("ERR:audio server num >= 3");
        return AUDIO_ERR_PARAM_INVALID;
    }

    int                           i, counts = 0;
    u8                            sinkASEsPerSvr = 0, srcASEsPerSvr = 0;
    blt_audio_codecSpecCapParam_t codecSpecCap;

    if (pUsedAudCfg->sinkASEs) {
        if (!pAscsClt->sinkAseNum || !pPacsClt->sinkPacRcdNum) {
            BLT_BAP_LOG("audio cap err:sinkASEs[%d] sinkPacNum[%d] ", pAscsClt->sinkAseNum, pPacsClt->sinkPacRcdNum);
            return AUDIO_ERR_PARAM_INVALID;
        }
        sinkASEsPerSvr = pUsedAudCfg->sinkASEs / pUsedAudCfg->numOfSvr;

        if (pAscsClt->sinkAseNum < sinkASEsPerSvr) {
            BLT_BAP_LOG("ERR:audio sink ASEs num Not enough");
            return AUDIO_ERR_PARAM_INVALID;
        }

        //Min Sink Audio Locations per Server
        if (pUsedAudCfg->minSinkAudLocPerSvr >= 2) {
            counts = blt_calBit1Number(pPacsClt->sinkAudioLca); //Sink Audio Locations
            if (pUsedAudCfg->minSinkAudLocPerSvr > counts) {
                BLT_BAP_LOG("audio sink location param err:%d", counts);
                BLT_BAP_LOG("pUsedAudCfg->minSinkAudLocPerSvr:%d", pUsedAudCfg->minSinkAudLocPerSvr);
                return AUDIO_ERR_PARAM_INVALID;
            }
        }

        /* The Audio_Channel_Allocation LTV structure defined in Bluetooth Assigned Numbers [2] may be present
         * in the Codec_Specific_Configuration field. The absence of the Audio_Channel_Allocation LTV structure
         * shall be interpreted as a single channel with no specified Audio Location. */
        //TODO: Audio_Channel_Allocation

        //Audio_Channel_Counts
        if (pUsedAudCfg->audChnsPerSinkASE >= 1) {
            for (i = 0; i < pPacsClt->sinkPacRcdNum; i++) {
                //TODO: CodecID offset check
                int ret = blt_audio_getCodecSpecCapParam(pPacsClt->pSinkPacRcd[i]->pac + 6, &codecSpecCap);
                if (ret != AUDIO_ESUCC) {
                    BLT_BAP_LOG("Get codec sepc cap param err:%d", ret);
                    return AUDIO_ERR_PARAM_INVALID;
                }

                if (codecSpecCap.fieldExistFlg & BLC_AUDIO_AUDIO_CHANNEL_MASK) {
                    counts = codecSpecCap.counts;
                } else {
                    counts = 1; //a single channel with no specified Audio Location
                }
                break;
                if (counts >= pUsedAudCfg->audChnsPerSinkASE) {
                    break;
                }
            }
            if (i == pPacsClt->sinkPacRcdNum) {
                BLT_BAP_LOG("audio sink channels param err:%d", counts);
                return AUDIO_ERR_PARAM_INVALID;
            }
        }
    }

    if (pUsedAudCfg->srcASEs) {
        if (!pAscsClt->srcAseNum || !pPacsClt->srcPacRcdNum) {
            BLT_BAP_LOG("audio cap err:srcASEs[%d] srcPacNum[%d] ", pAscsClt->srcAseNum, pPacsClt->srcPacRcdNum);
            return AUDIO_ERR_PARAM_INVALID;
        }

        srcASEsPerSvr = pUsedAudCfg->srcASEs / pUsedAudCfg->numOfSvr;

        if (pAscsClt->srcAseNum < srcASEsPerSvr) {
            BLT_BAP_LOG("ERR:audio src ASEs num Not enough");
            return AUDIO_ERR_PARAM_INVALID;
        }

        //Min Src Audio Locations per Server
        if (pUsedAudCfg->minSrcAudLocPerSvr >= 2) {
            counts = blt_calBit1Number(pPacsClt->srcAudioLca); //Src Audio Locations
            if (pUsedAudCfg->minSrcAudLocPerSvr > counts) {
                BLT_BAP_LOG("audio src location param err:%d", counts);
                return AUDIO_ERR_PARAM_INVALID;
            }
        }

        /* The Audio_Channel_Allocation LTV structure defined in Bluetooth Assigned Numbers [2] may be present
         * in the Codec_Specific_Configuration field. The absence of the Audio_Channel_Allocation LTV structure
         * shall be interpreted as a single channel with no specified Audio Location. */
        //TODO: Audio_Channel_Allocation

        //Audio_Channel_Counts
        if (pUsedAudCfg->audChnsPerSrcASE >= 1) {
            for (i = 0; i < pPacsClt->srcPacRcdNum; i++) {
                //TODO: CodecID offset check
                int ret = blt_audio_getCodecSpecCapParam(pPacsClt->pSrcPacRcd[i]->pac + 6, &codecSpecCap);
                if (ret != AUDIO_ESUCC) {
                    BLT_BAP_LOG("Get codec sepc cap param err:%d", ret);
                    return AUDIO_ERR_PARAM_INVALID;
                }

                if (codecSpecCap.fieldExistFlg & BLC_AUDIO_AUDIO_CHANNEL_MASK) {
                    counts = blt_calBit1Number(codecSpecCap.counts);
                } else {
                    counts = 1; //a single channel with no specified Audio Location
                }
                if (counts >= pUsedAudCfg->audChnsPerSrcASE) {
                    break;
                }
            }
            if (i == pPacsClt->srcPacRcdNum) {
                BLT_BAP_LOG("audio src channels param err:%d", counts);
                return AUDIO_ERR_PARAM_INVALID;
            }
        }
    }

    //Keep
    pAscsClt->svrNums = pUsedAudCfg->numOfSvr;
    pAscsClt->cisNums = pUsedAudCfg->CISes;
    //  pAscsClt->audCfgIdx = audCfgIdx;

    if (outChnInfo) {
        for (i = 0; i < pAscsClt->sinkAseNum; i++) {
            outChnInfo->sinkASEId[i] = pAscsClt->pSinkAse[i]->aseID;
        }
        for (i = 0; i < pAscsClt->srcAseNum; i++) {
            outChnInfo->srcASEId[i] = pAscsClt->pSrcAse[i]->aseID;
        }

        sinkASEsPerSvr = pUsedAudCfg->sinkASEs / pUsedAudCfg->numOfSvr;
        srcASEsPerSvr  = pUsedAudCfg->srcASEs / pUsedAudCfg->numOfSvr;

        u32 sinkAudLoc = pPacsClt->sinkAudioLcaHdl ? pPacsClt->sinkAudioLca : 0;
        u32 srcAudLoc  = pPacsClt->srcAudioLcaHdl ? pPacsClt->srcAudioLca : 0;

        BLT_BAP_LOG("sinkASEsPerSvr[%d],srcASEsPerSvr[%d]", sinkASEsPerSvr, srcASEsPerSvr);
        BLT_BAP_LOG("sinkAudLoc[%d],srcAudLoc[%d]", sinkAudLoc, srcAudLoc);
        u8 off, sinkAudChnAllocCnt = 0, srcAudChnAllocCnt = 0;
        if (pUsedAudCfg->minSinkAudLocPerSvr) {
            sinkAudChnAllocCnt = pUsedAudCfg->minSinkAudLocPerSvr / sinkASEsPerSvr;
        } else {
            sinkAudChnAllocCnt = 1;
        }
        if (pUsedAudCfg->minSrcAudLocPerSvr) {
            srcAudChnAllocCnt = pUsedAudCfg->minSrcAudLocPerSvr / srcASEsPerSvr;
        } else {
            srcAudChnAllocCnt = 1;
        }
        BLT_BAP_LOG("sinkAudChnAllocCnt[%d],srcAudChnAllocCnt[%d]", sinkAudChnAllocCnt, srcAudChnAllocCnt);
        outChnInfo->sinkCodecFrameBlksPerSDU = sinkAudChnAllocCnt;
        outChnInfo->srcCodecFrameBlksPerSDU  = srcAudChnAllocCnt;


        u32 sinkAudLocAlloc[2] = {0};
        u32 srcAudLocAlloc[2]  = {0};
        u32 sinkAudLocTemp     = sinkAudLoc;
        for (i = 0; i < sinkASEsPerSvr; i++) {
            BLT_BAP_LOG("[sinkASEsPerSvr] %d", sinkASEsPerSvr);
            for (int j = 0; j < sinkAudChnAllocCnt; j++) {
                BLT_BAP_LOG("[sinkAudChnAllocCnt] %d", sinkAudChnAllocCnt);
                off = __builtin_ffs(sinkAudLocTemp);
                sinkAudLocAlloc[i] |= BIT(off - 1);
                sinkAudLocTemp &= ~BIT(off - 1);
                BLT_BAP_LOG("[Sink audio Loc Alloc] aLoc:0X%X  off %d", sinkAudLocAlloc[i], off);
            }
            outChnInfo->sinkAudLocAlloc[i] = sinkAudLocAlloc[i];
            BLT_BAP_LOG("sink location %d", outChnInfo->sinkAudLocAlloc[i]);
        }

        u32 srcAudLocTemp = srcAudLoc;
        for (i = 0; i < srcASEsPerSvr; i++) {
            for (int j = 0; j < srcAudChnAllocCnt; j++) {
                BLT_BAP_LOG("[srcAudChnAllocCnt] %d", srcAudChnAllocCnt);
                off = __builtin_ffs(srcAudLocTemp);
                srcAudLocAlloc[i] |= BIT(off - 1);
                srcAudLocTemp &= ~BIT(off - 1);
                BLT_BAP_LOG("[SRC audio Loc Alloc] aLoc:0X%X  off %d", srcAudLocAlloc[i], off);
            }
            outChnInfo->srcAudLocAlloc[i] = srcAudLocAlloc[i];
            BLT_BAP_LOG("source location %d", outChnInfo->srcAudLocAlloc[i]);
        }

        outChnInfo->sinkASEsPerSvr = sinkASEsPerSvr;
        outChnInfo->srcASEsPerSvr  = srcASEsPerSvr;
        outChnInfo->sinkAudLoc     = sinkAudLoc;
        outChnInfo->srcAudLoc      = srcAudLoc;
    }

    return AUDIO_ESUCC;
}

int blc_bapuc_setAseConfigCodec(u16 aclHandle, u8 aseID, blc_audio_std_codec_settings_enum codecCfgIdx, blc_audio_ase_cfg_info_t *pAseCfgInfo)
{
    if (codecCfgIdx >= BLC_AUDIO_STD_CODEC_SETTINGS_E_MAX) {
        BLT_BAP_LOG("Invalid input: codec setting");
        return AUDIO_EPARAM;
    }

    if (pAseCfgInfo == NULL) {
        return AUDIO_EPARAM;
    }

    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(aclHandle);
    if (pAscsClt == NULL) {
        BLT_BAP_LOG("Invalid ACL handle: 0x%x", aclHandle);
        return AUDIO_EHANDLE;
    }

    u8               i                    = 0;
    u8               codecFrameBlksPerSDU = 0;
    u32              location             = 0;
    blt_ascsc_ase_t *pAse                 = NULL;

    for (i = 0; i < pAscsClt->sinkAseNum; i++) {
        if (aseID == pAscsClt->pSinkAse[i]->aseID) {
            pAse                 = pAscsClt->pSinkAse[i];
            codecFrameBlksPerSDU = pAseCfgInfo->sinkCodecFrameBlksPerSDU;
            location             = pAseCfgInfo->sinkAudLocAlloc[i];
        }
    }

    for (i = 0; i < pAscsClt->srcAseNum; i++) {
        if (aseID == pAscsClt->pSrcAse[i]->aseID) {
            pAse                 = pAscsClt->pSrcAse[i];
            codecFrameBlksPerSDU = pAseCfgInfo->srcCodecFrameBlksPerSDU;
            location             = pAseCfgInfo->srcAudLocAlloc[i];
        }
    }

    if (pAse == NULL) {
        return AUDIO_EMPTY;
    }

    blc_pacs_client_t *pPacsClt = blt_pacsc_getClientInst(aclHandle);
    if (pPacsClt == NULL) {
        BLT_BAP_LOG("Invalid PACS not found: %d", aseID);
        return AUDIO_EHANDLE;
    }
    u8                            pacRcdNum  = 0;
    blt_audio_pac_record_param_t *pPacRecord = NULL;
    if (pAse->dir == AUDIO_DIR_SINK) {
        pacRcdNum  = pPacsClt->sinkPacRcdNum;
        pPacRecord = pPacsClt->pSinkPacRcd[0];
        //BLT_BAP_LOG("Sink PAC num:%d", pacRcdNum);
    } else {
        pacRcdNum  = pPacsClt->srcPacRcdNum;
        pPacRecord = pPacsClt->pSrcPacRcd[0];
        //BLT_BAP_LOG("Source PAC num:%d", pacRcdNum);
    }

    for (i = 0; i < pacRcdNum; i++) {
        u8  pacNum    = pPacRecord[i].pac[0];
        u16 pacOffset = 1;
        for (u8 j = 0; j < pacNum; j++) {
            blt_audio_codecSpecCapParam_t codecSpecCap;
            int                           ret = blt_audio_getCodecSpecCapParam(pPacRecord[i].pac + pacOffset + 5, &codecSpecCap);
            if (ret != AUDIO_ESUCC) {
                BLT_BAP_LOG("Get codec sepc cap param err:%d", ret);
                break;
            }
            u8 codecSpecLen = pPacRecord[i].pac[pacOffset + 5];
            u8 metadataLen  = pPacRecord[i].pac[pacOffset + 6 + codecSpecLen];
            pacOffset       = pacOffset + 5 + 1 + codecSpecLen + 1 + metadataLen;
            BLT_BAP_LOG("codecSpecLen:%d", codecSpecLen);
            BLT_BAP_LOG("metadataLen:%d", metadataLen);
            //BLT_BAP_LOG("PAC:", &codecSpecCap, sizeof(blt_audio_codecSpecCapParam_t));

            if ((codecSettings[codecCfgIdx].frequencyBitField & codecSpecCap.frequency) &&
                (codecSettings[codecCfgIdx].durationBitField & codecSpecCap.duration) &&
                codecSpecCap.minOctets <= codecSettings[codecCfgIdx].frameOctets &&
                codecSettings[codecCfgIdx].frameOctets <= codecSpecCap.maxOctets) {
                blc_ascsc_aseConfig_t aseCfg;
                aseCfg.cigID                = 0x00;
                aseCfg.cisID                = aclHandle & 0x01;
                aseCfg.codecFrameBlksPerSDU = codecFrameBlksPerSDU;
                aseCfg.codecId.id           = BLC_AUDIO_CODING_FORMAT_LC3;
                aseCfg.codecId.companyID    = 0;
                aseCfg.codecId.vendorID     = 0;
                aseCfg.frequency            = codecSettings[codecCfgIdx].frequencyValue;
                aseCfg.duration             = codecSettings[codecCfgIdx].durationValue;
                aseCfg.frameOcts            = codecSettings[codecCfgIdx].frameOctets;
                aseCfg.location             = location;
                BLT_BAP_LOG("aclHandle: %x", aclHandle);
                BLT_BAP_LOG("cisID: %d", aseCfg.cisID);
                BLT_BAP_LOG("frequency: %d", aseCfg.frequency);
                BLT_BAP_LOG("duration: %d", aseCfg.duration);
                BLT_BAP_LOG("frameOcts: %d", aseCfg.frameOcts);
                BLT_BAP_LOG("location: %d", location);
                ret = blt_ascsc_setAseCfg(aclHandle, aseID, &aseCfg);
                if (ret != AUDIO_ESUCC) {
                    BLT_BAP_LOG("sink ase config fail - ret: %d", ret);
                    return ret;
                }
                ret = blc_bapuc_setASEOperationEnable(aclHandle, aseID);
                if (ret != AUDIO_ESUCC) {
                    BLT_BAP_LOG("ase enable fail: %d", ret);
                    return ret;
                }
                pAse->stdCodecSettingsIdx = codecCfgIdx;

                return AUDIO_ESUCC;
            }
        }
    }
    BLT_BAP_LOG("un-match: Codec parameters found");
    return AUDIO_EPARAM;
}

int blc_bapuc_setAseConfigQos(u16 aclHandle, u8 aseID, blc_audio_std_qos_settings_enum qosCfgIdx)
{
    if (qosCfgIdx >= BLC_AUDIO_STD_QOS_SETTINGS_E_MAX) {
        BLT_BAP_LOG("Invalid input: qos setting");
        return AUDIO_EPARAM;
    }

    //blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(connHandle);
    blt_ascsc_ase_t *pAse = blt_ascsc_getAsePtrByAseId(aclHandle, aseID);
    if (pAse == NULL) {
        BLT_BAP_LOG("Invalid AseID not found: %d", aseID);
        return AUDIO_EHANDLE;
    }

    std_qos_settings_t qosSettings = unicastQosSettings[qosCfgIdx][pAse->stdCodecSettingsIdx];
    if (pAse->unframedNotSupp) {
        if (qosSettings.framing == CIS_UNFRAMED) {
            BLT_BAP_LOG("Invalid input unframed not support: %d", aseID);
            return AUDIO_ERR_PARAM_INVALID;
        }
    }
    pAse->framing         = qosSettings.framing;
    pAse->sduInterval     = qosSettings.sduInterval;
    pAse->RTN             = qosSettings.retransmitNum;
    pAse->maxTransLatency = qosSettings.maxTransportLatency;
    pAse->maxSDU          = qosSettings.maxSduSize * pAse->codecFrmBlksPerSDU;

    BLT_BAP_LOG("pAse->framing: %d", pAse->framing);
    BLT_BAP_LOG("pAse->sduInterval: %d", pAse->sduInterval);
    BLT_BAP_LOG("pAse->RTN: %d", pAse->RTN);
    BLT_BAP_LOG("pAse->maxTransLatency: %d", pAse->maxTransLatency);
    BLT_BAP_LOG("pAse->maxSDU: %d", pAse->maxSDU);
    return AUDIO_ESUCC;
}

/**
 * @brief       This function is used to send isochronous packet.
 * @param[in]   aclHandle  - The ACL connection handle.
 * @param[in]   aseID      - The ID of the ASE that need to send data .
 * @param[in]   pPkt       - Raw packet need to be sent.
 * @param[in]   pktLen     - Raw packet length.
 * @return      0          - Isochronous packet send success.
 *              Others     - Isochronous packet send fail.
 */
int blc_bapuc_sduPacketPush(u16 aclHandle, u8 idx, u8 *pPkt, u16 pktLen)
{
    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(aclHandle);
    if (pAscsClt == NULL) {
        BLT_BAP_LOG("Invalid ACL handle: 0x%x", aclHandle);
        return AUDIO_EHANDLE;
    }

    blt_ascsc_ase_t *pAse = NULL;
    if (idx > pAscsClt->sinkAseNum) {
        return AUDIO_EHANDLE;
    }

    pAse = pAscsClt->pSinkAse[idx];

    if ((!pAse->cisEstablished) || pAse->state != BLT_ASCS_ASE_STATE_STREAMING || pPkt == NULL || (!pktLen)) {
        BLT_BAP_LOG("[SduPush]Invalid: pAse:0x%x state:%d cisEstablished:%d", pAse, pAse->state, pAse->cisEstablished);
        return AUDIO_EHANDLE;
    }

    ble_sts_t status = blc_iso_sendData(pAse->cisHdl, pPkt, pktLen);
    if (status != BLE_SUCCESS) {
        BLT_BAP_LOG("[src ASE]cis send data failed: %d", pAse->aseID);
        return AUDIO_EPUSH_SDU;
    }

    return AUDIO_ESUCC;
}

/**
 * @brief       This function is used to pop received isochronous packet .
 * @param[in]   aclHandle  - The ACL connection handle.
 * @param[in]   aseID      - The ID of the ASE that received data .
 * @return[out] !NULL      - Isochronous packet pop success.
 *              NULL       - Isochronous packet pop failed.
 */
sdu_packet_t *blc_bapuc_sduPacketPop(u16 aclHandle, u8 idx)
{
    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(aclHandle);
    if (pAscsClt == NULL) {
        BLT_BAP_LOG("Invalid ACL handle: 0x%x", aclHandle);
        return NULL;
    }

    blt_ascsc_ase_t *pAse = NULL;
    if (idx > pAscsClt->srcAseNum) {
        return NULL;
    }
    pAse = pAscsClt->pSrcAse[idx];

    if (pAse == NULL || (!pAse->cisEstablished)) {
        BLT_BAP_LOG("[SduPop]Invalid: pAse:0x%x state:%d cisEstablished:%d", pAse, pAse->state, pAse->cisEstablished);
        return NULL;
    }

    return blc_ll_popCisRxSduData(pAse->cisHdl);
}

int blc_bapuc_setASEOperationEnable(u16 aclHandle, u8 aseID)
{
    return blt_ascsc_enableAse(aclHandle, aseID);
}

int blc_bapuc_setAseDisable(u16 aclHandle, u8 aseID)
{
    return blt_ascsc_disableAse(aclHandle, aseID);
}

int blc_bapuc_setAseRelease(u16 aclHandle, u8 aseID)
{
    return blt_ascsc_releaseAse(aclHandle, aseID);
}

int blc_bapuc_setAseMetadata(u16 aclHandle, u8 aseID, u8 *pMetadata, u8 metadataLen)
{
    return blt_ascsc_setMetadata(aclHandle, aseID, pMetadata, metadataLen);
}

int blc_bapuc_setAseUpdateMetadata(u16 aclHandle, u8 aseID, u8 *pMetadata, u8 metadataLen)
{
    return blt_ascsc_updateMetadata(aclHandle, aseID, pMetadata, metadataLen);
}

int blc_bapuc_setAseReceiverStartReady(u16 aclHandle, u8 aseID)
{
    blt_ascsc_ase_t *pAse = blt_ascsc_getAsePtrByAseId(aclHandle, aseID);
    if (pAse == NULL) {
        BLT_BAP_LOG("Invalid AseID not found: %d", aseID);
        return AUDIO_EHANDLE;
    }

    if (pAse->state != BLT_ASCS_ASE_STATE_ENABLING) {
        return AUDIO_ESTATUS;
    }
    if (pAse->dir != AUDIO_DIR_SOURCE) {
        return AUDIO_EDIR;
    }
    BLT_BAP_LOG("Receive start ready - ASE[%d]", pAse->aseID);
    pAse->flags |= BLT_ASCSC_ASE_FLAG_SEND_START;
    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(aclHandle);
    pAscsClt->aseFsmBusy        = 1;
    return AUDIO_ESUCC;
}

int blc_bapuc_setAseReceiverStopReady(u16 aclHandle, u8 aseID)
{
    blt_ascsc_ase_t *pAse = blt_ascsc_getAsePtrByAseId(aclHandle, aseID);
    if (pAse == NULL) {
        BLT_BAP_LOG("Invalid AseID not found: %d", aseID);
        return AUDIO_EHANDLE;
    }

    if (pAse->state != BLT_ASCS_ASE_STATE_DISABLING) {
        return AUDIO_ESTATUS;
    }
    if (pAse->dir != AUDIO_DIR_SOURCE) {
        return AUDIO_EDIR;
    }
    BLT_BAP_LOG("Receive stop ready - ASE[%d]", pAse->aseID);
    pAse->flags |= BLT_ASCSC_ASE_FLAG_SEND_STOP;
    blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(aclHandle);
    pAscsClt->aseFsmBusy        = 1;
    return AUDIO_ESUCC;
}
