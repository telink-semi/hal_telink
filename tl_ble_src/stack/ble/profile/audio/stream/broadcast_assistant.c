/********************************************************************************************************
 * @file    broadcast_assistant.c
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

#include "stack/ble/host/gatt/tlk_malloc_stack.h"

#define BAPBA_MALLOC(len)       malloc_nonreten(len)
#define BAPBA_REALLOC(ptr, len) realloc_nonreten(ptr, len)
#define BAPBA_FREE(ptr)         free_nonreten(ptr)

typedef struct {
    u16 connHandle;
    union{
        u8 scanEn;
        u8 localPAST;
    };
    union{
        u16 syncHandle;
        u16 advHandle;
    };
    u8  paSyncState;
    u8  advAddrType;
    u8  advAddr[6];
    u8  advSID;
    u8  BcstId[3];
} blt_bcst_assistant_scan_t;

blt_bcst_assistant_scan_t bapbaScanSourceState;

blt_bcst_assistant_scan_t bapbaSyncPASTState;

static void blt_prf_hciEventCb(u32 h, u8 *p, int len);

void blc_audio_registerBroadcastAssistant(const blc_bapba_regParam_t *param)
{
    blc_audio_registerPACSControlClient(param->pPacsParam);
    blc_audio_registerBASSControlClient(param->pBassParam);

    /* LE stack event callback for BAP Broadcast Assistant role */
    bap_bcst_assistant_cb = blt_prf_hciEventCb;
}

static int blt_audio_assistantStartSyncPA(blc_bapba_startSyncPaEvt_t *evt)
{
    return blt_prf_sendEvent(0xFFFF, AUDIO_EVT_BAPBA_START_SYNC_PA, (u8*)evt, sizeof(blc_bapba_startSyncPaEvt_t));
}

static int blt_audio_assistantFoundSink(blc_bapba_foundSinkEvt_t *evt)
{
    return blt_prf_sendEvent(0xFFFF, AUDIO_EVT_BAPBA_FOUND_SINK, (u8*)evt, sizeof(blc_bapba_foundSinkEvt_t));
}

static int blt_audio_assistantPastStartedReady(void)
{
    return blt_prf_sendEvent(bapbaSyncPASTState.connHandle, AUDIO_EVT_BAPBA_PAST_STARTED_READY, NULL, 0);
}

static u8* blt_audio_getAdvTypeInfo(u8 *pAdvDat, u32 len, data_type_t advType, u8* outLen)
{
    u8 adLen = 0;
    u8 *p = pAdvDat;

    while(len)
    {
        adLen = p[0];
        if(p[1] == advType)
        {
            *outLen = adLen-1;
            return p+2;
        }

        if(len > (u32)(adLen + 1)){
            len -= (adLen + 1);
            p   += (adLen + 1);
        }else{
            len = 0;
        }
    }

    *outLen = 0;
    return NULL;
}

static u8* blt_audio_getCompleteNameInfo(u8 *pAdvDat, u32 len, u8* outLen)
{
    return blt_audio_getAdvTypeInfo(pAdvDat, len, DT_COMPLETE_LOCAL_NAME, outLen);
}

static u8* blt_audio_getBroadcastNameInfo(u8 *pAdvDat, u32 len, u8* outLen)
{
    return blt_audio_getAdvTypeInfo(pAdvDat, len, DT_BROADCAST_NAME, outLen);
}

static bool blt_audio_findSolicitationReq(u8 *pAdvDat, u32 len)
{
    u8 adLen = 0;
    u8 *p = pAdvDat;

    while(len)
    {
        adLen = p[0];
        if(p[1] == DT_SERVICE_DATA_16BIT_UUID && bstream_to_u16_le(&p[2]) == SERVICE_UUID_BROADCAST_AUDIO_SCAN)
        {
            return true;
        }

        if(len > (u32)(adLen + 1)){
            len -= (adLen + 1);
            p   += (adLen + 1);
        }else{
            len = 0;
        }
    }
    return false;
}

static void blt_bapba_scanSolicitationRequest(extAdvEvt_info_t* pExtAdv)
{
    if(blt_audio_findSolicitationReq(pExtAdv->data, pExtAdv->data_length))
    {
        blc_bapba_foundSinkEvt_t foundSinkEvt;
        foundSinkEvt.addrType = pExtAdv->address_type;
        memcpy(foundSinkEvt.address, pExtAdv->address, 6);
        u8* completeName = blt_audio_getCompleteNameInfo(pExtAdv->data, pExtAdv->data_length, &foundSinkEvt.completeNameLen);
        foundSinkEvt.completeName = BAPBA_MALLOC(foundSinkEvt.completeNameLen + 1);
        if(foundSinkEvt.completeName == NULL)   return ;

        memcpy(foundSinkEvt.completeName, completeName, foundSinkEvt.completeNameLen);
        foundSinkEvt.completeName[foundSinkEvt.completeNameLen] = '\0';
//          if(blt_audio_findSolicitationReq(pExtAdv->data, pExtAdv->data_length))
        blt_audio_assistantFoundSink(&foundSinkEvt);
//          if(memcmp(foundSinkEvt.completeName, "PTS-", 4) == 0)
//              blt_audio_assistantFoundSink(&foundSinkEvt);
        BAPBA_FREE(foundSinkEvt.completeName);
    }
}

static void blt_bapba_scanBroadcastSource(extAdvEvt_info_t* pExtAdv)
{
    if(!bapbaScanSourceState.scanEn || bapbaScanSourceState.paSyncState != PA_SYNC_STATE_NONE)
        return ;

    u8 *pAdvData = pExtAdv->data;
    u8 broadcastId[3];

    //parse broadcastId from adv data
    if((pExtAdv->perd_adv_inter == PERIODIC_ADV_INTER_NO_PERIODIC_ADV) ||
            (!blt_audio_sinkGetBroadcastId(broadcastId, pAdvData, pExtAdv->data_length))){
        return;
    }

    blc_bapba_startSyncPaEvt_t paSyncEvt = {
        .sid = pExtAdv->advertising_sid,
        .addrType = pExtAdv->address_type,
    };
    memcpy(paSyncEvt.address, pExtAdv->address, 6);
    memcpy(paSyncEvt.broadcastId, broadcastId, 3);

    u8* name = blt_audio_getCompleteNameInfo(pAdvData, pExtAdv->data_length, &paSyncEvt.completeNameLen);
    paSyncEvt.completeName = BAPBA_MALLOC(paSyncEvt.completeNameLen + 1);
    if(paSyncEvt.completeName == NULL)  return ;
    memcpy(paSyncEvt.completeName, name, paSyncEvt.completeNameLen);
    paSyncEvt.completeName[paSyncEvt.completeNameLen] = '\0';

    name = blt_audio_getBroadcastNameInfo(pAdvData, pExtAdv->data_length, &paSyncEvt.broadcastNameLen);
    paSyncEvt.broadcastName = BAPBA_MALLOC(paSyncEvt.broadcastNameLen + 1);

    if(paSyncEvt.broadcastName == NULL)
    {
        BAPBA_FREE(paSyncEvt.completeName);
        return ;
    }
    memcpy(paSyncEvt.broadcastName, name, paSyncEvt.broadcastNameLen);
    paSyncEvt.broadcastName[paSyncEvt.broadcastNameLen] = '\0';

    if(blt_audio_assistantStartSyncPA(&paSyncEvt)) {
        BAPBA_FREE(paSyncEvt.completeName);
        BAPBA_FREE(paSyncEvt.broadcastName);
        return;
    }

    BAPBA_FREE(paSyncEvt.completeName);
    BAPBA_FREE(paSyncEvt.broadcastName);

    BLT_BIS_ASSISTANT_LOG("extend Adv found, addr is %s", addr_to_str(pExtAdv->address));

    u8 status = blc_ll_periodicAdvertisingCreateSync(SYNC_ADV_SPECIFY | REPORTING_INITIALLY_EN,
                                                     pExtAdv->advertising_sid,
                                                     pExtAdv->address_type,
                                                     pExtAdv->address,
                                                     0, SYNC_TIMEOUT_2S, 0);
    if(status != BLE_SUCCESS){
        BLT_BIS_ASSISTANT_LOG("PA sync create start -- Failed(status is %d)", status);
        memset(&bapbaScanSourceState, 0, sizeof(blt_bcst_assistant_scan_t));
    }else{
        bapbaScanSourceState.paSyncState = PA_SYNC_STATE_START;
        bapbaScanSourceState.advSID = pExtAdv->advertising_sid;
        bapbaScanSourceState.advAddrType = pExtAdv->address_type;
        memcpy(bapbaScanSourceState.advAddr, pExtAdv->address, 6);
        memcpy(bapbaScanSourceState.BcstId, broadcastId, 3);
        BLT_BIS_ASSISTANT_LOG("PA sync create start -- OK");
    }

}

static void blt_audio_assistantAdvReportEvt(u8 *p, int len)
{
    (void)len;
    hci_le_extAdvReportEvt_t *pExtAdvRpt = (hci_le_extAdvReportEvt_t *)p;

    int offset = 0;

    extAdvEvt_info_t *pExtAdv = NULL;
    for(int i=0; i<pExtAdvRpt->num_reports ; i++)
    {
        pExtAdv = (extAdvEvt_info_t *)(pExtAdvRpt->advEvtInfo + offset);
        offset += (EXTADV_INFO_LENGTH + pExtAdv->data_length);

        //This function to scan BAP role Scan Delegator, with solicitation request.
        blt_bapba_scanSolicitationRequest(pExtAdv);

        //This function to scan BAP role Broadcast Source, with Broadcast Audio Announcements.
        blt_bapba_scanBroadcastSource(pExtAdv);
    }
}

static void blt_bapba_periodicAdvSyncSourceScan(hci_le_periodicAdvSyncEstablishedEvt_t *pEvt)
{
    if(pEvt->status == BLE_SUCCESS)
    {
        bapbaScanSourceState.paSyncState = PA_SYNC_STATE_SUCCESS;
    }
    else
    {
        memset(&bapbaScanSourceState, 0, sizeof(blt_bcst_assistant_scan_t));
    }
}

static void blt_bapba_periodicAdvSyncPAST(hci_le_periodicAdvSyncEstablishedEvt_t *pEvt)
{
    if(pEvt->status == BLE_SUCCESS)
    {
        bapbaSyncPASTState.paSyncState = PA_SYNC_STATE_SUCCESS;
        blt_audio_assistantPastStartedReady();
    }
    else
    {
        memset(&bapbaSyncPASTState, 0, sizeof(blt_bcst_assistant_scan_t));
    }
}

static bool blt_bapba_checkSyncIfo(blt_bcst_assistant_scan_t* scan, u8 advAddrType, u8 advSID, u8 advAddr[6])
{
    if(scan->paSyncState != PA_SYNC_STATE_START ||
            advAddrType != scan->advAddrType ||
            advSID != scan->advSID ||
            memcmp(advAddr, scan->advAddr, 6)) {
        return false;
    }

    return true;
}

static void blt_audio_assistantPeriodicAdvSync(u8 *p, int len)
{
    (void)len;
    hci_le_periodicAdvSyncEstablishedEvt_t *pEvt = (hci_le_periodicAdvSyncEstablishedEvt_t*)p;

    BLT_BIS_ASSISTANT_LOG("PDA sync(sync Handle 0x%x) status is 0x%x", pEvt->syncHandle, pEvt->status);

    if(blt_bapba_checkSyncIfo(&bapbaScanSourceState, pEvt->advAddrType, pEvt->advSID, pEvt->advAddr)) {
        bapbaScanSourceState.syncHandle = pEvt->syncHandle;
        blt_bapba_periodicAdvSyncSourceScan(pEvt);
    }
    if(blt_bapba_checkSyncIfo(&bapbaSyncPASTState, pEvt->advAddrType, pEvt->advSID, pEvt->advAddr)) {
        bapbaSyncPASTState.syncHandle = pEvt->syncHandle;
        blt_bapba_periodicAdvSyncPAST(pEvt);
    }

}

static int blt_audio_assistantCheckCodecConfig(blc_audio_codec_id_t* codec, blc_audio_codecSpecCfgParsed_t* codecCfg, blc_audio_metadata_parsed_t* metadata)
{
    return blc_pacsc_checkSinkPAC(bapbaScanSourceState.connHandle, codec, codecCfg, metadata);
}

static void blt_audio_assistantSendFoundSourceInfo(blc_bapba_foundSourceInfoEvt_t *pEvt)
{
    blt_prf_sendEvent(bapbaScanSourceState.connHandle, AUDIO_EVT_BAPBA_FOUND_SOURCE_INFO, (u8*)pEvt, sizeof(blc_bapba_foundSourceInfoEvt_t));
    pEvt->presentationDelay = 0xFFFFFFFF;           //Used to indicate that is a continuous event.
}

pda_recombination_t gPdaPkt={
    .dataOffset = 0
};

typedef struct {
    u16 syncHandle;
    u16 length;
    u8* data;
}paRecombination_t;

static paRecombination_t gPaData[TSKNUM_PDA_SYNC] = {
    {0x0000, 0, NULL},
    {0x0000, 0, NULL},
};

void blt_clearPeriodicAdvertisingDataRecombination(u16 syncHandle)
{
    for(size_t i=0; i<ARRAY_SIZE(gPaData); i++) {
        if(gPaData[i].syncHandle == syncHandle) {
            BAPBA_FREE(gPaData[i].data);
            memset(&gPaData[i], 0, sizeof(paRecombination_t));
        }
    }
}

paRecombination_t* blt_periodicAdvertisingDataRecombination(hci_le_periodicAdvReportEvt_t *pEvt)
{
    paRecombination_t* pPaData = NULL;

    for(size_t i=0; i<ARRAY_SIZE(gPaData); i++) {
        if(gPaData[i].syncHandle == pEvt->syncHandle) {
            pPaData = &gPaData[i];
            break;
        }
        if(gPaData[i].syncHandle == 0x0000) {
            pPaData = &gPaData[i];
        }
    }

    if(pPaData == NULL)     return NULL;

    if(pEvt->dataStatus == PDA_SYNC_REPORT_DATA_COMPLETE || pEvt->dataStatus == PDA_SYNC_REPORT_DATA_INCOMPLETE) {
        pPaData->data = BAPBA_REALLOC(pPaData->data, gPaData->length + pEvt->dataLength);
        if(pPaData->data == NULL)   return NULL;

        memcpy(pPaData->data + pPaData->length, pEvt->data, pEvt->dataLength);
        pPaData->length += pEvt->dataLength;

        pPaData->syncHandle = pEvt->syncHandle;

        if(pEvt->dataStatus == PDA_SYNC_REPORT_DATA_COMPLETE)   return pPaData;
    }
    else if(pEvt->dataStatus == PDA_SYNC_REPORT_DATA_TRUNCATED) {
        BAPBA_FREE(pPaData->data);
        pPaData->length = 0;
    }

    return NULL;
}

int blt_pda_recombination_handler(u8 *p)
{
    hci_le_periodicAdvReportEvt_t *pEvt = (hci_le_periodicAdvReportEvt_t *)p;
    u8 syncHdl = pEvt->syncHandle&0x03;

    if(syncHdl >= TSKNUM_PDA_SYNC){ //0/1
        return PDA_SYNC_REPORT_DATA_TRUNCATED;
    }

    if(pEvt->dataStatus == PDA_SYNC_REPORT_DATA_INCOMPLETE || pEvt->dataStatus == PDA_SYNC_REPORT_DATA_COMPLETE){

        memcpy((u8*)(gPdaPkt.data + gPdaPkt.dataOffset), pEvt->data, pEvt->dataLength);

        gPdaPkt.dataOffset += pEvt->dataLength;

        if(pEvt->dataStatus == PDA_SYNC_REPORT_DATA_COMPLETE){
            gPdaPkt.dataLength = gPdaPkt.dataOffset;
            gPdaPkt.dataOffset = 0;
            return PDA_SYNC_REPORT_DATA_COMPLETE; //complete
        }
    }
    else if(pEvt->dataStatus == PDA_SYNC_REPORT_DATA_TRUNCATED){
        gPdaPkt.dataOffset = 0;
        return PDA_SYNC_REPORT_DATA_TRUNCATED; //truncated
    }

    return PDA_SYNC_REPORT_DATA_INCOMPLETE; ///incomplete
}

static void blt_bapba_periodicAdvReportSourceScan(u8* BASE, u16 len)
{

    BLT_BIS_ASSISTANT_LOG("BASE is %s", hex_to_str(BASE, len));
    blc_bapba_foundSourceInfoEvt_t sourceInfoEvt;

    sourceInfoEvt.sid = bapbaScanSourceState.advSID;
    sourceInfoEvt.addrType = bapbaScanSourceState.advAddrType;
    memcpy(sourceInfoEvt.address, bapbaScanSourceState.advAddr, 6);

    while(len)
    {
        u8 adLen = BASE[0];
        if(BASE[1] == DT_SERVICE_DATA_16BIT_UUID && bstream_to_u16_le(&BASE[2]) == SERVICE_UUID_BASIC_AUDIO_ANNOUNCEMENT)
        {
            u8* ptr = &BASE[4];
            STREAM_TO_U24(sourceInfoEvt.presentationDelay, ptr);

            u8 numSubGroup = 0;
            STREAM_TO_U8(numSubGroup, ptr);
            u8 numBis = 0;
            for(int i=0; i<numSubGroup; i++)
            {
                STREAM_TO_U8(numBis, ptr);
                blc_audio_codec_id_t codecId;
                STREAM_TO_STR(&codecId.id, ptr, sizeof(blc_audio_codec_id_t));
                blc_audio_codecSpecCfgParsed_t codecCfg;
                codecCfg.fieldExistFlg = 0;
                blt_audio_getCodecSpecCfgParam(ptr, &codecCfg);
                ptr += ptr[0] + 1;  //skip codec specific configuration
                u8* metadata = ptr;
                ptr += ptr[0] + 1;  //skip metadata & metadata Len
                for(int j=0; j<numBis; j++)
                {
                    sourceInfoEvt.bisInfo[0].CodecId = codecId;
                    sourceInfoEvt.bisInfo[0].metadata = metadata;
                    sourceInfoEvt.bisInfo[0].codecCfg = codecCfg;

                    blt_audio_getCodecSpecCfgParam(ptr+1, &sourceInfoEvt.bisInfo[0].codecCfg);
                    sourceInfoEvt.bisIndex = ptr[0];
                    ptr += ptr[1] + 2;

                    if(blt_audio_assistantCheckCodecConfig(&sourceInfoEvt.bisInfo[0].CodecId, &sourceInfoEvt.bisInfo[0].codecCfg, NULL))
                        continue;

                    blt_audio_assistantSendFoundSourceInfo(&sourceInfoEvt);
                    BLT_BIS_ASSISTANT_LOG("FoundSourceInfo");
                }
            }
        }

        if(len > (adLen+1)){
            len -= adLen + 1;
            BASE += adLen + 1;
        }else{
            len = 0;
        }
    }
}

static void blt_audio_assistantPeriodicAdvReport(u8 *p, int len)
{
    (void)len;
    //BLT_BIS_ASSISTANT_LOG("PDA report is %s", hex_to_str(p, len));
    hci_le_periodicAdvReportEvt_t *pEvt = (hci_le_periodicAdvReportEvt_t *)p;

    paRecombination_t* pPaData = blt_periodicAdvertisingDataRecombination(pEvt);

    if(pPaData == NULL)
    {
        return ;
    }

    if(pEvt->syncHandle == bapbaScanSourceState.syncHandle && bapbaScanSourceState.paSyncState == PA_SYNC_STATE_SUCCESS) {
        blt_bapba_periodicAdvReportSourceScan(pPaData->data, pPaData->length);
    }

    blt_clearPeriodicAdvertisingDataRecombination(pEvt->syncHandle);

}

static void blt_audio_assistantBiginfoAdvReport(u8 *p, int len)
{
    (void)len;
    hci_le_bigInfoAdvReportEvt_t *pEvt = (hci_le_bigInfoAdvReportEvt_t *)p;

    if(pEvt->syncHandle == bapbaScanSourceState.syncHandle)
    {
        ble_sts_t status = blc_ll_periodicAdvertisingTerminateSync(bapbaScanSourceState.syncHandle);
        BLT_BIS_ASSISTANT_LOG("terminate PDA Sync status is %d", status);
        blt_clearPeriodicAdvertisingDataRecombination(pEvt->syncHandle);
        blc_bapba_sourceEncStateEvt_t sourceEncEvt = {.enc = pEvt->enc};
        blt_prf_sendEvent(bapbaScanSourceState.connHandle, AUDIO_EVT_BAPBA_SOURCE_ENC_STATE, (u8*)&sourceEncEvt, sizeof(blc_bapba_sourceEncStateEvt_t));
        memset(&bapbaScanSourceState, 0, sizeof(blt_bcst_assistant_scan_t));
    }
}

static void blt_audio_assistantPeriodicAdvLost(u8 *p, int len)
{
    (void)len;
    hci_le_periodicAdvSyncLostEvt_t *pEvt = (hci_le_periodicAdvSyncLostEvt_t*)p;
    BLT_BIS_ASSISTANT_LOG("PDA Sync lost");

    if(pEvt->syncHandle == bapbaScanSourceState.syncHandle) {

    }

    blt_clearPeriodicAdvertisingDataRecombination(pEvt->syncHandle);
}

static prf_hciLeMetaEvtCb_t assistantHciEvt[] = {
    {HCI_SUB_EVT_LE_EXTENDED_ADVERTISING_REPORT,    blt_audio_assistantAdvReportEvt},
    {HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_ESTABLISHED, blt_audio_assistantPeriodicAdvSync},
    {HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_REPORT,    blt_audio_assistantPeriodicAdvReport},
    {HCI_SUB_EVT_LE_BIGINFO_ADVERTISING_REPORT,     blt_audio_assistantBiginfoAdvReport},
    {HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_LOST, blt_audio_assistantPeriodicAdvLost},
};

PRF_HCI_EVT_CALLBACK(assistantHciEvt);

int blt_audio_bcstAssistantRecvEvt(u16 connHandle, int evtID, u8 *pData, u16 dataLen)
{
    switch(evtID)
    {
        case AUDIO_EVT_BASSC_RECV_SYNCINFO_REQ: {
            if(bapbaSyncPASTState.localPAST) {
                hci_le_paSetInfoTransferCmdParams_t cmdPara = {
                    .connHandle = connHandle,
                    .serviceData = 0x0100,      //TODO: by qihang.mou, BAP define SourceID+Adv type
                    .advHandle = bapbaSyncPASTState.advHandle
                };
                hci_le_paSetInfoTransferRetParams_t retPara;
                ble_sts_t status = blc_hci_le_periodicAdvSetInfoTransfer(&cmdPara, &retPara);
                BLT_BIS_ASSISTANT_LOG("recv syncInfo req, 0x%x, status is %d", connHandle, status);
            }
            else{
                hci_le_pastCmdParams_t cmdPara = {
                    .connHandle = connHandle,
                    .serviceData = 0x0100,      //TODO: by qihang.mou, BAP define SourceID+Adv type
                    .syncHandle = bapbaSyncPASTState.syncHandle
                };
                hci_le_pastRetParams_t retPara;
                ble_sts_t status = blc_hci_le_periodicAdvSyncTransfer(&cmdPara, &retPara);
                BLT_BIS_ASSISTANT_LOG("recv syncInfo req, 0x%x, status is %d", connHandle, status);
            }
        }break;
        default:
            break;
    }

    return blt_prf_sendEvent(connHandle, evtID, pData, dataLen);
}

////////////////////////Broadcast Assistant command API////////////////////////////
void blc_bapba_writeRemoteScanStarted(u16 connHandle)
{
    blc_bassc_writeRemoteScanStarted(connHandle);
    bapbaScanSourceState.scanEn = true;
    bapbaScanSourceState.connHandle = connHandle;
}

void blc_bapba_writeRemoteScanStopped(u16 connHandle)
{
    blc_bassc_writeRemoteScanStopped(connHandle);
    bapbaScanSourceState.scanEn = false;
    bapbaScanSourceState.connHandle = 0x00;
}

void blc_bapba_setLocalSourceInfo(u16 advHandle, blc_audio_source_head_t *head)
{
    bapbaSyncPASTState.localPAST = 1;
    memcpy(&bapbaSyncPASTState.advAddrType, head, sizeof(blc_audio_source_head_t));
    bapbaSyncPASTState.advHandle = advHandle;
}

bool blc_bapba_startPAST(u16 connHandle, blc_audio_source_head_t *head)
{
    if(bapbaSyncPASTState.paSyncState != PA_SYNC_STATE_NONE) {
        return false;
    }

    bapbaSyncPASTState.localPAST = 0;
    bapbaSyncPASTState.connHandle = connHandle;
    memcpy(&bapbaSyncPASTState.advAddrType, head, sizeof(blc_audio_source_head_t));

    u8 status = blc_ll_periodicAdvertisingCreateSync(SYNC_ADV_SPECIFY | REPORTING_INITIALLY_EN,
                                                     head->sid, head->addrType, head->addr,
                                                     0, SYNC_TIMEOUT_2S, 0);
    if(status != BLE_SUCCESS){
        BLT_BIS_ASSISTANT_LOG("PA sync create start -- Failed(status is %d)", status);
        memset(&bapbaSyncPASTState, 0, sizeof(blt_bcst_assistant_scan_t));
    }else{
        bapbaSyncPASTState.paSyncState = PA_SYNC_STATE_START;
        BLT_BIS_ASSISTANT_LOG("PA sync create start -- OK");
    }

    return status == BLE_SUCCESS;
}

void blc_bapba_stopPAST(u16 connHandle)
{
    (void)connHandle;
    if(bapbaSyncPASTState.paSyncState == PA_SYNC_STATE_START)
    {
        blc_ll_periodicAdvertisingCreateSyncCancel();
    }

    if(bapbaSyncPASTState.paSyncState == PA_SYNC_STATE_SUCCESS)
    {
        ble_sts_t status = blc_ll_periodicAdvertisingTerminateSync(bapbaSyncPASTState.syncHandle);
        BLT_BIS_ASSISTANT_LOG("PAST:terminate PDA Sync status is %d", status);
    }

    memset(&bapbaSyncPASTState, 0, sizeof(blt_bcst_assistant_scan_t));
}

static void blc_bapba_writeAddSource(u16 connHandle, blc_audio_source_head_t *head, u8 paSync, u32 bisSync)
{
    u8 addSourceCmd[20];
    u8* ptr = addSourceCmd;

    STR_TO_STREAM(ptr, (u8*)head, sizeof(blc_audio_source_head_t));
    U8_TO_STREAM(ptr, paSync);
    U16_TO_STREAM(ptr, 0xFFFF);
    U8_TO_STREAM(ptr, 1);
    U32_TO_STREAM(ptr, bisSync);
    U8_TO_STREAM(ptr, 0);

    blc_bassc_writeAddSource(connHandle, addSourceCmd, sizeof(addSourceCmd));
}

void blc_bapba_writeAddSourceNotSyncPA(u16 connHandle, blc_audio_source_head_t *head, u32 bisSync)
{
    blc_bapba_writeAddSource(connHandle, head, BASS_NOT_SYNC_TO_PA, bisSync);
}

void blc_bapba_writeAddSourcePast(u16 connHandle, blc_audio_source_head_t *head, u32 bisSync)
{
    blc_bapba_writeAddSource(connHandle, head, BASS_SYNC_TO_PA_PAST_AVA, bisSync);
}

void blc_bapba_writeAddSourceNoPast(u16 connHandle, blc_audio_source_head_t *head, u32 bisSync)
{
    blc_bapba_writeAddSource(connHandle, head, BASS_SYNC_TO_PA_PAST_NAVA, bisSync);
}

static void blc_bapba_writeModifySource(u16 connHandle, u8 sourceID, u8 paSync, u32 bisSync)
{
    u8 modifySourceCmd[10] = {sourceID, paSync, U16_TO_BYTES(0xFFFF), 1, U32_TO_BYTES(bisSync)};
    blc_bassc_writeModifySource(connHandle, modifySourceCmd, sizeof(modifySourceCmd));
}

void blc_bapba_writeModifySourceNotSyncPA(u16 connHandle, u8 sourceID, u32 bisSync)
{
    blc_bapba_writeModifySource(connHandle, sourceID, BASS_NOT_SYNC_TO_PA, bisSync);
}

void blc_bapba_writeModifySourcePast(u16 connHandle, u8 sourceID, u32 bisSync)
{
    blc_bapba_writeModifySource(connHandle, sourceID, BASS_SYNC_TO_PA_PAST_AVA, bisSync);
}

void blc_bapba_writeModifySourceNoPast(u16 connHandle, u8 sourceID, u32 bisSync)
{
    blc_bapba_writeModifySource(connHandle, sourceID, BASS_SYNC_TO_PA_PAST_NAVA, bisSync);
}

void blc_bapba_writeSetBroadcastCode(u16 connHandle, u8 sourceID, u8 bcstCode[16])
{
    blc_bassc_writeSetBroadcastCode(connHandle, sourceID, bcstCode);
}

void blc_bapba_writeRemoveSource(u16 connHandle, u8 sourceID)
{
    blc_bassc_writeRemoveSource(connHandle, sourceID);
}
