/********************************************************************************************************
 * @file    broadcast_sink.c
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

void blt_bap_leStackEvtForBcstSink(u32 h, u8 *p, int n);

void blc_audio_registerBapBroadcastSink(const blc_bapbs_regParam_t *param)
{
    blc_audio_registerPACSControlServer(param->pPacsParam);
    blc_audio_registerBASSControServer(param->pBassParam);

    /* LE stack event callback for BAP Broadcast Sink role */
    bap_bcst_sink_cb = blt_bap_leStackEvtForBcstSink;
}

enum{
    ACL_CONN_STATE_DISCONNECT,
    ACL_CONN_STATE_CONN,
};

enum{
    BIG_SYNC_STATE_NONE,
    BIG_SYNC_STATE_START,
    BIG_SYNC_STATE_SYNCING,
    BIG_SYNC_STATE_FAILED,
    BIG_SYNC_STATE_SUCCESS,
};

enum{
    BIG_ENC_STATE_NO,
    BIG_ENC_STATE_CODE_REQ,
    BIG_ENC_STATE_HAD_CODE,
};

typedef struct{
    u8  autoSyncPaEn;

    u8  sourceID;
    u8  srcAdvType;
    u8  srcAdvAddr[6];
    u8  srcAdvSID;
    u8  broadcastId[3];

    u8  paSyncState;
    u16 syncHandle;
    u8  audioLocation;

    u16 aclHandle;
    u8  aclConnState;

    u8  broadcastCodeValid;
    u8  broadcastCode[16];

    u32 bisSyncInfo;
    u32 bisSyncState;
    u8  bigSyncState;
    u8  bigHandle;
    u8  bigEncEn;
    u8  numBis;
    u16 bisHandle[2];

    u8 BASELen;
    u8 BASE[252];   //exclude ADType and Basic Audio Announcement Service
} blt_broadcastSinkState_t;

blt_broadcastSinkState_t sinkState = {
    .autoSyncPaEn = false,
    .aclConnState = ACL_CONN_STATE_DISCONNECT,
    .paSyncState  = PA_SYNC_STATE_NONE,
    .bigSyncState = BIG_SYNC_STATE_NONE,
};

void blc_audio_registerBapBroadcastSinkSourceLocal(u8 addrType, u8* addr, u8 sid, u8* broadcastId, u32 bisSync, char *broadcast_code)
{
    sinkState.bigSyncState = BIG_SYNC_STATE_NONE;
    sinkState.sourceID = 0x00;
    sinkState.aclHandle = 0x00;

    sinkState.srcAdvType = addrType;
    memcpy(sinkState.srcAdvAddr,  addr, 6);
    sinkState.srcAdvSID  = sid;
    memcpy(sinkState.broadcastId,  broadcastId, 3);

    sinkState.broadcastCodeValid = BIG_ENC_STATE_NO;

    ll_whiteList_reset();
    ll_whiteList_add(addrType, addr);

    sinkState.autoSyncPaEn = true;
    sinkState.paSyncState = PA_SYNC_STATE_NONE;
    blc_ll_setExtScanParam( OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_WL,  SCAN_PHY_1M,
                            SCAN_TYPE_PASSIVE,  SCAN_INTERVAL_100MS,    SCAN_WINDOW_100MS,
                            SCAN_TYPE_PASSIVE,  SCAN_INTERVAL_90MS,     SCAN_WINDOW_90MS);

    blc_ll_setExtScanEnable( BLC_SCAN_ENABLE, DUPE_FLTR_DISABLE,
                             SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
    BLT_BIS_SINK_LOG("syn to PA by PDA sync");
    BLT_BIS_SINK_LOG("addrType: %d SID: %d ", sinkState.srcAdvType, sinkState.srcAdvSID);
    BLT_BIS_SINK_LOG("address:", addr_to_str(sinkState.srcAdvAddr));
    BLT_BIS_SINK_LOG("broadcast ID:", hex_to_str(sinkState.broadcastId, 3));

    sinkState.broadcastCodeValid = BIG_ENC_STATE_HAD_CODE;
    memcpy(sinkState.broadcastCode, broadcast_code, 16);
    BLT_BIS_SINK_LOG("broadcast code is %s", hex_to_str(broadcast_code, 16));

    sinkState.bisSyncInfo = bisSync;
    sinkState.bigSyncState = BIG_SYNC_STATE_START;

    bap_bcst_assistant_cb = NULL;
    bap_bcst_sink_cb = blt_bap_leStackEvtForBcstSink;
}

static void blt_audio_notSyncPA(void)
{
    //PDA Sync check state
    if(sinkState.paSyncState == PA_SYNC_STATE_SUCCESS)
    {
        ble_sts_t status = blc_ll_periodicAdvertisingTerminateSync(sinkState.syncHandle);
        gPdaPkt.dataOffset = 0;
        BLT_BIS_SINK_LOG("Terminate PDA Sync status is %d", status);
        blc_basss_updatePASyncState(sinkState.aclHandle,
                                    sinkState.sourceID, BASS_PA_STATE_NOT_SYNC_TO_PA);
    }
    sinkState.paSyncState = PA_SYNC_STATE_NONE;
    sinkState.autoSyncPaEn = false;
    blc_ll_setExtScanEnable( BLC_SCAN_DISABLE, DUPE_FLTR_DISABLE,
                             SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
}

void blt_audio_broadcastSinkRecvEvt(u16 connHandle, int evtID, u8 *pData, u16 dataLen)
{
    switch(evtID)
    {
        case AUDIO_EVT_BASSS_REMOTE_SCAN_STOPPED:
            blt_prf_sendEvent(connHandle, AUDIO_EVT_BAPBS_REMOTE_SCAN_STOPPED, pData, dataLen);
            break;
        case AUDIO_EVT_BASSS_REMOTE_SCAN_STARTED:
            blt_prf_sendEvent(connHandle, AUDIO_EVT_BAPBS_REMOTE_SCAN_STARTED, pData, dataLen);
            break;
        case AUDIO_EVT_BASSS_NO_PAST:
        {
            BLT_BIS_SINK_LOG("    No PAST");
            sinkState.paSyncState = PA_SYNC_STATE_NONE;
            sinkState.autoSyncPaEn = false;
            sinkState.bisSyncInfo = 0;
            blc_ll_setExtScanEnable( BLC_SCAN_DISABLE, DUPE_FLTR_DISABLE,
                                     SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
        }break;
        case AUDIO_EVT_BASSS_REMOVE_SOURCE:
        {
            blt_bass_syncPaEvt_t *pEvt = (blt_bass_syncPaEvt_t *)pData;
            if(sinkState.sourceID == pEvt->sourceID && sinkState.broadcastCodeValid == BIG_ENC_STATE_HAD_CODE)
            {
                sinkState.broadcastCodeValid = BIG_ENC_STATE_NO;
                memset(sinkState.broadcastCode, 0, 16);
            }
            if(sinkState.bigSyncState == BIG_SYNC_STATE_SUCCESS)
            {
                blc_ll_bigTerminateSync(sinkState.bigHandle);
                BLT_BIS_SINK_LOG("sink had sync BIS");
            }
            sinkState.bigSyncState = BIG_SYNC_STATE_NONE;
            sinkState.bisSyncInfo = 0x00;
            sinkState.bisSyncState = 0x00;
            sinkState.BASELen = 0;
            blt_audio_notSyncPA();
        }break;
        case AUDIO_EVT_BASSS_DONOT_SYNC_TO_PA:
        {
            blt_audio_notSyncPA();
        }break;
        case AUDIO_EVT_BASSS_SYNC_TO_PA:
        {
            blt_bass_syncPaEvt_t *pEvt = (blt_bass_syncPaEvt_t *)pData;
            BLT_BIS_SINK_LOG("source id = %d pa state is %d sink PA sync state is %d", pEvt->sourceID, pEvt->paSyncState, sinkState.paSyncState);

            if(sinkState.paSyncState != PA_SYNC_STATE_NONE)
            {
                break;
            }

            sinkState.bigSyncState = BIG_SYNC_STATE_NONE;
            sinkState.sourceID = pEvt->sourceID;
            sinkState.aclHandle = connHandle;

            sinkState.srcAdvType = pEvt->advAddrType;
            memcpy(sinkState.srcAdvAddr,  pEvt->advAddr, 6);
            sinkState.srcAdvSID = pEvt->advSID;
            memcpy(sinkState.broadcastId, pEvt->broadcastId, 3);

            sinkState.broadcastCodeValid = BIG_ENC_STATE_NO;

            /* Use PAST to sync PA */
            if(pEvt->paSyncState == BASS_PA_STATE_SYNCINFO_REQUEST)
            {
                sinkState.paSyncState = PA_SYNC_STATE_START;
                BLT_BIS_SINK_LOG("Sync to PA by PAST");
                break;
            }

            ll_whiteList_reset();
            ll_whiteList_add(pEvt->advAddrType, pEvt->advAddr);

            sinkState.autoSyncPaEn = true;

            blc_ll_setExtScanParam( OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_WL,  SCAN_PHY_1M,
                                    SCAN_TYPE_PASSIVE,  SCAN_INTERVAL_100MS,    SCAN_WINDOW_100MS,
                                    SCAN_TYPE_PASSIVE,  SCAN_INTERVAL_90MS,     SCAN_WINDOW_90MS);

            blc_ll_setExtScanEnable( BLC_SCAN_ENABLE, DUPE_FLTR_DISABLE,
                                     SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
            BLT_BIS_SINK_LOG("Sync to PA by PDA sync, addrType: %d, SID: %d", sinkState.srcAdvType, sinkState.srcAdvSID);
            BLT_BIS_SINK_LOG("    Address is %s Broadcast ID is 0x%06x", hex_to_str(sinkState.srcAdvAddr, 6), sinkState.broadcastId);
        }break;

        case AUDIO_EVT_BASSS_SYNC_TO_BIS:
        {
            blt_bass_syncBisEvt_t *pEvt = (blt_bass_syncBisEvt_t *)pData;
            BLT_BIS_SINK_LOG("want to sync bis %d %d %d %d", pEvt->sourceID, sinkState.sourceID, sinkState.bisSyncInfo, pEvt->BISSync);
            if(pEvt->sourceID != sinkState.sourceID)
                break;

            if(sinkState.bisSyncInfo == pEvt->BISSync)
                break;

            BLT_BIS_SINK_LOG("BIS sync info is %08x", pEvt->BISSync);
            sinkState.bisSyncInfo = pEvt->BISSync;
            if(sinkState.bisSyncInfo != 0){
                sinkState.bigSyncState = BIG_SYNC_STATE_START;
            }
            else{
                if(sinkState.bigSyncState == BIG_SYNC_STATE_SUCCESS)
                {
                    sinkState.bigSyncState = BIG_SYNC_STATE_NONE;
                    blc_ll_bigTerminateSync(sinkState.bigHandle);
                    BLT_BIS_SINK_LOG("sink had sync BIS");

                }
                else
                {
                    sinkState.bigSyncState = BIG_SYNC_STATE_NONE;
                    BLT_BIS_SINK_LOG("The client has no recommended BIS sync value");
                }
            }
        }break;

        case AUDIO_EVT_BASSS_RECV_SET_BROADCAST_CODE:
        {
            blt_basss_recvBroadcastCodeEvt_t *pEvt = (blt_basss_recvBroadcastCodeEvt_t *)pData;
            BLT_BIS_SINK_LOG("source id is %d %d, valid is %d, broadcast code is 0x%s",
                    sinkState.sourceID, pEvt->sourceID, sinkState.broadcastCodeValid,
                    hex_to_str(pEvt->broadcastCode, 16));
            if(sinkState.sourceID == pEvt->sourceID && sinkState.broadcastCodeValid == BIG_ENC_STATE_CODE_REQ)
            {
                sinkState.broadcastCodeValid = BIG_ENC_STATE_HAD_CODE;
                memcpy(sinkState.broadcastCode, pEvt->broadcastCode, 16);
            }
        }break;

        default:
            break;
    }
}

bool blt_audio_sinkGetBroadcastId(u8 bcstId[3], u8 *pAdvDat, u32 len)
{
    u8 adLen = 0;
    u8 *p = pAdvDat;

    while(len)
    {
        adLen = p[0];
        if(p[1] == DT_SERVICE_DATA_16BIT_UUID && bstream_to_u16_le(&p[2]) == SERVICE_UUID_BROADCAST_AUDIO_ANNOUNCEMENT && adLen >= 6)
        {
            memcpy(bcstId, &p[4], 3);
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

static int blt_audio_sinkAdvReportEvt(u8 *p, int len)
{
    (void)len;
    hci_le_extAdvReportEvt_t *pExtAdvRpt = (hci_le_extAdvReportEvt_t *)p;

    int offset = 0;

    extAdvEvt_info_t *pExtAdv = NULL;

    for(int i=0; i<pExtAdvRpt->num_reports ; i++)
    {
        pExtAdv = (extAdvEvt_info_t *)(pExtAdvRpt->advEvtInfo + offset);
        offset += (EXTADV_INFO_LENGTH + pExtAdv->data_length);

        if(!sinkState.autoSyncPaEn || sinkState.paSyncState != PA_SYNC_STATE_NONE)
        {
            continue;
        }

        if(sinkState.srcAdvSID  == pExtAdv->advertising_sid &&
            sinkState.srcAdvType == pExtAdv->address_type    &&
           !memcmp((u8*)&sinkState.srcAdvAddr[0], (u8*)&pExtAdv->address[0], 6)) //TODO: RPA
        {
            u8 *pAdvData = pExtAdv->data;
            u8 broadcastId[3];

            //parse broadcastId from adv data
            if(!blt_audio_sinkGetBroadcastId(broadcastId, pAdvData, pExtAdv->data_length)){
                continue;
            }

            BLT_BIS_SINK_LOG("    extend advertise found");
            if(!memcmp(sinkState.broadcastId, broadcastId, 3))
            {
                u8 status = blc_ll_periodicAdvertisingCreateSync(SYNC_ADV_SPECIFY | REPORTING_INITIALLY_EN,
                                                                 pExtAdv->advertising_sid,
                                                                 pExtAdv->address_type,
                                                                 pExtAdv->address,
                                                                 0, SYNC_TIMEOUT_2S, 0);
                if(status != BLE_SUCCESS){
                    BLT_BIS_SINK_LOG("ERR: PA sync create start -- Failed(status is %d)", status);
                    break;
                }else{
                    sinkState.paSyncState  = PA_SYNC_STATE_START;
                    BLT_BIS_SINK_LOG("PA sync create start -- OK");
                    blc_ll_setExtScanEnable(BLC_SCAN_DISABLE, DUPE_FLTR_DISABLE,
                                             SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
                }
            }
        }
        else
        {
            BLT_BIS_SINK_LOG("advertise type is invalid");
        }

    }
    return 0;
}

static int blt_audio_sinkPeriodicAdcSyncTransferRecv(u8 *p, int len)
{
    (void)len;
    hci_le_periodicAdvSyncTransferRcvdEvt_t *pEvt = (hci_le_periodicAdvSyncTransferRcvdEvt_t*)p;
    //BLT_BIS_SINK_LOG("pda synced by PAST, status is 0x%x", pEvt->status);
    if(pEvt->status == BLE_SUCCESS) {
        if(sinkState.srcAdvSID == pEvt->advSID && sinkState.srcAdvType == pEvt->advAddrType &&
                (memcmp(sinkState.srcAdvAddr, pEvt->advAddr, 6) == 0)) {
            blc_basss_updatePASyncState(sinkState.aclHandle,
                                    sinkState.sourceID, BASS_PA_STATE_SYNC_TO_PA);
            sinkState.syncHandle = pEvt->syncHandle;
            sinkState.paSyncState = PA_SYNC_STATE_SUCCESS;
        }

    }
    else { //sync pda failed
        blc_basss_updatePASyncState(sinkState.aclHandle,
                sinkState.sourceID, BASS_PA_STATE_FAILED_TO_SYNC_TO_PA);
    }
    blc_basss_notifyRecvState(sinkState.aclHandle, sinkState.sourceID);

    blt_basss_pastFinish(sinkState.aclHandle);

    return 0;
}

static int blt_audio_sinkPeriodicAdvReport(u8 *p, int len)
{
    if(blt_pda_recombination_handler(p) != PDA_SYNC_REPORT_DATA_COMPLETE)
        return -1;

    /* Check if LC3 has been configured. */
    if(sinkState.BASELen)
        return -1;

    p = gPdaPkt.data;
    len = gPdaPkt.dataLength;

    while(len)
    {
        u8 adLen = p[0];
        if(p[1] == DT_SERVICE_DATA && bstream_to_u16_le(&p[2]) == SERVICE_UUID_BASIC_AUDIO_ANNOUNCEMENT)
        {
            sinkState.BASELen = adLen -3;   //skip ADType and service Data UUID: exclude Basic Audio Announcement Service
            memcpy(sinkState.BASE, &p[4], sinkState.BASELen);
            return 0;
        }

        if(len > (adLen+1)){
            len -= adLen + 1;
            p += adLen + 1;
        }else{
            len = 0;
        }
    }

    return 0;
}

static int blt_audio_sendPdaSyncState(u8 status, u8* param, u8 paramLen)
{
    blc_bapbs_pdaSyncStateEvt_t evt;

    memcpy(&evt.syncHandle, param, paramLen);
    evt.state = status;
    BLT_BIS_SINK_LOG("send PDA sync state[%d], param is %s", status, hex_to_str(param, paramLen));
    return blt_prf_sendEvent(sinkState.aclHandle, AUDIO_EVT_BAPBS_PDA_SYNC_STATE, (u8*)&evt, sizeof(evt));
}

static int blt_audio_sinkPeriodicAdvSync(u8 *p, int len)
{
    (void)len;
    hci_le_periodicAdvSyncEstablishedEvt_t *pEvt = (hci_le_periodicAdvSyncEstablishedEvt_t*)p;
    //BLT_BIS_SINK_LOG("status is %d, ps sync state is 0x%x", pEvt->status, sinkState.paSyncState);
    if(pEvt->status == BLE_SUCCESS && sinkState.paSyncState == PA_SYNC_STATE_START){
        sinkState.paSyncState = PA_SYNC_STATE_SUCCESS;
        sinkState.syncHandle  = pEvt->syncHandle;

        //update PA SYNC STATE to BASP
        blc_basss_updatePASyncState(sinkState.aclHandle,
                sinkState.sourceID, BASS_PA_STATE_SYNC_TO_PA);

        sinkState.BASELen = 0;
        blt_audio_sendPdaSyncState(PDA_SYNCED, (u8*)&pEvt->syncHandle, sizeof(hci_le_periodicAdvSyncEstablishedEvt_t) - 2);
        BLT_BIS_SINK_LOG("sink PA sync established, BIS info is 0x%08x", sinkState.bisSyncInfo);
    }
    else{
        sinkState.paSyncState = PA_SYNC_STATE_NONE;

        blc_basss_updatePASyncState(sinkState.aclHandle,
                sinkState.sourceID, BASS_PA_STATE_FAILED_TO_SYNC_TO_PA);

        sinkState.BASELen = 0;
        blt_audio_sendPdaSyncState(PDA_SYNCED_FAILED, (u8*)&pEvt->syncHandle, sizeof(hci_le_periodicAdvSyncEstablishedEvt_t) - 2);
        BLT_BIS_SINK_LOG("ERR PA sync failed, reason is %d", pEvt->status);

    }

    blc_basss_notifyRecvState(sinkState.aclHandle, sinkState.sourceID);
    return 0;
}

//bit0: 1 mean want to sync
static int blt_audio_parseCodecConfig(u32 bisSync, u8* bis)
{
    u8 bisSyncInfoBuf[sizeof(blc_bapbs_bisSinkInitCodecEvt_t) + sizeof(bisSyncInfo_t)*32];

    blc_bapbs_bisSinkInitCodecEvt_t *evt = (blc_bapbs_bisSinkInitCodecEvt_t *)bisSyncInfoBuf;

    u8* ptr = sinkState.BASE;

    STREAM_TO_U24(evt->presentationDelay, ptr);

    evt->bisNum  = 0;

    u8 numSubGroup = 0;
    STREAM_TO_U8(numSubGroup, ptr);

    u8 numBis = 0;
    bisSyncInfo_t* bisInfo = evt->bisInfo;
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
            bisInfo->CodecId = codecId;
            bisInfo->metadata = metadata;
            bisInfo->codecCfg = codecCfg;
//          BLT_BIS_SINK_LOG("  BIS index is %d, found BIS index is %d", bisIndex, ptr[0]);
            if(bisSync & BIT(ptr[0] - 1)) {
                evt->bisNum++;
                *bis++ = ptr[0];
                blt_audio_getCodecSpecCfgParam(ptr+1, &bisInfo->codecCfg);
                bisInfo ++;
            }
            ptr += ptr[1] + 2;
        }
    }

    if(evt->bisNum != blt_calBit1Number(bisSync))
    {
        return 1;
    }

    return blt_prf_sendEvent(sinkState.aclHandle, AUDIO_EVT_BAPBS_BIS_SINK_INIT_CODEC, (u8*)evt, sizeof(blc_bapbs_bisSinkInitCodecEvt_t) + sizeof(bisSyncInfo_t)*evt->bisNum);;
}

static int blt_audio_sinkBiginfoAdvReport(u8 *p, int len)
{
    (void)len;
    if(!sinkState.BASELen)      //must receive complete BASE Value.
        return 1;

    hci_le_bigInfoAdvReportEvt_t *pEvt = (hci_le_bigInfoAdvReportEvt_t *)p;

    //BLT_BIS_SINK_LOG("big sync state is 0x%x, info is %d, broadcode is %d", sinkState.bigSyncState, sinkState.bisSyncInfo, sinkState.broadcastCodeValid);
    if(sinkState.bigSyncState == BIG_SYNC_STATE_START)
    {
        if(sinkState.bisSyncInfo == 0)
        {
            sinkState.bigSyncState = BIG_SYNC_STATE_NONE;
            return 0;
        }

        //TODO: ENC BIS
        if(pEvt->enc){
            //Write BASCP BIG_Encryption field to 0x01 to request Client to send broadcast code
            if(sinkState.broadcastCodeValid == BIG_ENC_STATE_NO)
            {
                sinkState.broadcastCodeValid = BIG_ENC_STATE_CODE_REQ;
                blc_basss_updateBigEncState(sinkState.aclHandle, sinkState.sourceID, BASS_BIG_BCSTCODE_REQUIRED, NULL);
                blc_basss_notifyRecvState(sinkState.aclHandle, sinkState.sourceID);
                BLT_BIS_SINK_LOG("receive ENC BIS");
                return 0;
            }
            else if(sinkState.broadcastCodeValid == BIG_ENC_STATE_CODE_REQ)
            {
                return 0;
            }
        }

        u8 bigSyncParamBuf[sizeof(hci_le_bigCreateSyncParams_t) + 32] = {0};

        hci_le_bigCreateSyncParams_t *pBigCreateSyncParam = (hci_le_bigCreateSyncParams_t*)bigSyncParamBuf;

        pBigCreateSyncParam->big_handle       = BIG_HANDLE_0;                /* Used to identify the BIG */
        pBigCreateSyncParam->sync_handle      = pEvt->syncHandle;            /* Identifier of the periodic advertising train */
        pBigCreateSyncParam->enc              = pEvt->enc;                   /* Encryption flag */

        if(sinkState.broadcastCodeValid){
            memcpy(pBigCreateSyncParam->broadcast_code, sinkState.broadcastCode, 16);/* TK: all zeros, just like JustWorks TODO: LE security mode 3, here use LE security mode 3 level2 */
            BLT_BIS_SINK_LOG("    BIG encryption");
        }
        else{
            memset(pBigCreateSyncParam->broadcast_code, 0, 16);
            BLT_BIS_SINK_LOG("    BIG no encryption");
        }

        sinkState.bigEncEn = pEvt->enc;

        pBigCreateSyncParam->mse              = pEvt->nse;                   /* The Controller can schedule reception of any number of subevents up to NSE */
        pBigCreateSyncParam->big_sync_timeout = 10*pEvt->IsoItvl*1250/10000; /* Synchronization timeout for the BIG */
        BLT_BIS_SINK_LOG("    BIS NSE[%d] numberOfBis[%d]", pEvt->nse, pEvt->numBis);

        /* Only one BIS connection is created by default. Expand later as needed */
        if(sinkState.bisSyncInfo == 0xffffffff)
        {
            //TODO: How to select the correct bis synchronization.
            pBigCreateSyncParam->bis[0] = 1;
            pBigCreateSyncParam->num_bis = 1;
            sinkState.bisSyncState = 1;
        }
        else
        {
            if(blt_audio_parseCodecConfig(sinkState.bisSyncInfo, &pBigCreateSyncParam->bis[0]))
            {
                BLT_BIS_SINK_LOG("    APP Not support codec Configuration");
                sinkState.bisSyncState = 0;
                sinkState.bigSyncState = BIG_SYNC_STATE_FAILED;
                return 0;
            }
            sinkState.bisSyncState = sinkState.bisSyncInfo;
            pBigCreateSyncParam->num_bis = blt_calBit1Number(sinkState.bisSyncInfo);
        }

        BLT_BIS_SINK_LOG(" create sync bis number is %d, bis index %d %d", pBigCreateSyncParam->num_bis, pBigCreateSyncParam->bis[0], pBigCreateSyncParam->bis[1]);

        ble_sts_t status = blc_hci_le_bigCreateSync(pBigCreateSyncParam);
        if(status != BLE_SUCCESS){
            sinkState.bisSyncState = 0;
            BLT_BIS_SINK_LOG("ERR: BIG create sync error 0x%x", status);
            sinkState.bigSyncState = BIG_SYNC_STATE_FAILED;
            return 0;
        }
        sinkState.bigSyncState = BIG_SYNC_STATE_SYNCING;

        BLT_BIS_SINK_LOG("    BIG sync start ");
    }
    return 0;
}

static int blt_audio_sendBigSyncState(blc_audio_bigSyncState_enum state, u8* param)
{
    u8 evt[50];
    blc_bapbs_BisSinkSyncBigEvt_t* pEvt = (blc_bapbs_BisSinkSyncBigEvt_t*)evt;
    if(state == BIG_SYNCED || state == BIG_SYNCED_FAILED)
    {
        hci_le_bigSyncEstablishedEvt_t *pHciEvt = (hci_le_bigSyncEstablishedEvt_t*)param;
        pEvt->numBis = pHciEvt->numBis;
        pEvt->bigHandle = pHciEvt->bigHandle;
        pEvt->isoInterval = pHciEvt->isoIntvl;
        for(int i=0; i<pEvt->numBis; i++)
        {
            pEvt->bisHandles[i] = pHciEvt->bisHandles[i];
        }
    }
    else
    {
        hci_le_bigSyncLostEvt_t *pHciEvt = (hci_le_bigSyncLostEvt_t*)param;
        pEvt->lostReason = pHciEvt->reason;
        pEvt->bigHandle = pHciEvt->bigHandle;
    }
    pEvt->state = state;
    return blt_prf_sendEvent(sinkState.aclHandle, AUDIO_EVT_BAPBS_BIS_SINK_SYNC_BIG, (u8*)evt, sizeof(pEvt));
}

static int blt_audio_sinkBigSync(u8 *p, int len)
{
    (void)len;
    hci_le_bigSyncEstablishedEvt_t *pEvt = (hci_le_bigSyncEstablishedEvt_t*)p;

    BLT_BIS_SINK_LOG("BIG Sync, reason is 0x%x", pEvt->status);

    if(pEvt->status == BLE_SUCCESS && sinkState.bigSyncState == BIG_SYNC_STATE_SYNCING)
    {
        sinkState.bigSyncState = BIG_SYNC_STATE_SUCCESS;

        sinkState.bigHandle = pEvt->bigHandle;

        //BLT_BIS_SINK_LOG("Big handle is 0x%x, bis num is %d", pEvt->bigHandle, pEvt->numBis);
        for(int i=0; i<pEvt->numBis; i++)
        {
            sinkState.bisHandle[i] = pEvt->bisHandles[i];
            sinkState.numBis++;
            //BLT_BIS_SINK_LOG("set data path handle is 0x%x, index = %d", pEvt->bisHandles[i], i);
            blc_ll_setupIsoDataPath(
                    pEvt->bisHandles[i],
                    Data_Dir_Output,
                    Data_Path_HCI,
                    0, 0, 0, 0, 0,
                    0, 0, 0, 0
                    );

        }

        //Write SYNC state to BASP and notify client
        blc_basss_updateBisSyncState(sinkState.aclHandle, sinkState.sourceID, 0, sinkState.bisSyncState);

        //Write BIGEncryption state to BASP and notify client
        u8 bigEncryption = BASS_BIG_NOT_ENCRYPTED;
        if(sinkState.bigEncEn){
            bigEncryption = BASS_BIG_DECRYPTING;
        }

        blc_basss_updateBigEncState(sinkState.aclHandle, sinkState.sourceID, bigEncryption, NULL);

        blc_basss_notifyRecvState(sinkState.aclHandle, sinkState.sourceID);
        blt_audio_sendBigSyncState(BIG_SYNCED, p);

        //TODO: Debug
#if 0       //bugfix PDA, ACL, BIS all sync, PDA will lost
        ble_sts_t status = blc_ll_periodicAdvertisingTerminateSync(sinkState.syncHandle);
        gPdaPkt.dataOffset = 0;
        BLT_BIS_SINK_LOG("terminate PDA Sync status is 0x%x", status);
#endif

        BLT_BIS_SINK_LOG("    BIG sync successfully");
    }
    else if(pEvt->status == HCI_ERR_OP_CANCELLED_BY_HOST || pEvt->status == HCI_ERR_CONN_TERM_BY_LOCAL_HOST){
        blt_audio_sendBigSyncState(BIG_SYNCED_FAILED, p);
        sinkState.paSyncState = PA_SYNC_STATE_NONE;
        BLT_BIS_SINK_LOG("    BIG Terminate Sync: 0x%x", pEvt->status);
    }
    else{
        if(pEvt->status != BLE_SUCCESS){
            sinkState.bigSyncState = 0x00000000;
            blc_basss_updateBisSyncState(sinkState.aclHandle, sinkState.sourceID, 0, sinkState.bisSyncState);
            blc_basss_notifyRecvState(sinkState.aclHandle, sinkState.sourceID);
        }
        blt_audio_sendBigSyncState(BIG_SYNCED_FAILED, p);
        sinkState.paSyncState = PA_SYNC_STATE_NONE;

        BLT_BIS_SINK_LOG("BIG sync failed, status is 0x%x", pEvt->status);
    }
    return 0;
}

static int blt_audio_sinkPeriodicAdvLost(u8 *p, int len)
{
    (void)len;
    (void)p;

    gPdaPkt.dataOffset = 0;

    sinkState.syncHandle = 0;
    sinkState.paSyncState = PA_SYNC_STATE_NONE;

    sinkState.BASELen = 0;

    //update PA SYNC STATE to BASP
    blc_basss_updatePASyncState(sinkState.aclHandle,
            sinkState.sourceID, BASS_PA_STATE_NOT_SYNC_TO_PA);

    blc_ll_setExtScanEnable( BLC_SCAN_DISABLE, DUPE_FLTR_DISABLE,
                             SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);

    blt_audio_sendPdaSyncState(PDA_LOST, NULL, 0);
    BLT_BIS_SINK_LOG("PDA Sync Lost");

    blc_basss_notifyRecvState(sinkState.aclHandle, sinkState.sourceID);
    return 0;
}

static int blt_audio_sinkBigSyncLost(u8 *p, int len)
{
    (void)len;
    hci_le_bigSyncLostEvt_t *pEvt = (hci_le_bigSyncLostEvt_t*)p;
    sinkState.bigSyncState = BIG_SYNC_STATE_NONE;
    sinkState.bisSyncState = 0;
    sinkState.bisSyncInfo = 0;
    sinkState.bigEncEn = 0;

    //Write BIGEncryption state to BASP and notify client
    if(pEvt->reason == HCI_ERR_CONN_TERM_MIC_FAILURE){
        blc_basss_updateBigEncState(sinkState.aclHandle, sinkState.sourceID, BASS_BIG_BAD_CODE, sinkState.broadcastCode);
    }
    else{
        blc_basss_updateBigEncState(sinkState.aclHandle, sinkState.sourceID, BASS_BIG_NOT_ENCRYPTED, NULL);
    }
    //Write SYNC LOST state to BASP
    blc_basss_updateBisSyncState(sinkState.aclHandle, sinkState.sourceID, 0, sinkState.bisSyncState);
    blc_basss_notifyRecvState(sinkState.aclHandle, sinkState.sourceID);
    blt_audio_sendBigSyncState(BIG_LOST, p);
    sinkState.broadcastCodeValid = BIG_ENC_STATE_NO;
    BLT_BIS_SINK_LOG("BIG Sync Lost, reason is 0x%x", pEvt->reason);
    return 0;
}

static int blt_audio_sinkTerminateBigComplete(u8 *p, int len)
{
    (void)len;
    hci_le_terminateBigCompleteEvt_t *pEvt = (hci_le_terminateBigCompleteEvt_t *)p;

    sinkState.bigSyncState = BIG_SYNC_STATE_NONE;
    sinkState.bisSyncState = 0;
    sinkState.bigEncEn = 0;

    //Write SYNC LOST state to BASP
    blc_basss_updateBisSyncState(sinkState.aclHandle, sinkState.sourceID, 0, sinkState.bisSyncState);

    //Write BIGEncryption state to BASP and notify client
    if(pEvt->reason == HCI_ERR_CONN_TERM_MIC_FAILURE){
        blc_basss_updateBigEncState(sinkState.aclHandle, sinkState.sourceID, BASS_BIG_BAD_CODE, sinkState.broadcastCode);
    }
    else{
        blc_basss_updateBigEncState(sinkState.aclHandle, sinkState.sourceID, BASS_BIG_NOT_ENCRYPTED, NULL);
    }

    BLT_BIS_SINK_LOG("BIG terminate complete, reason is 0x%x", pEvt->reason);
    sinkState.broadcastCodeValid = BIG_ENC_STATE_NO;
    blc_basss_notifyRecvState(sinkState.aclHandle, sinkState.sourceID);
    blt_audio_sendBigSyncState(BIG_LOST, p);
    return 0;
}

void blt_bap_leStackEvtForBcstSink(u32 h, u8 *p, int n)
{
    if (h &HCI_FLAG_EVENT_BT_STD)       //Controller HCI event
    {
        u8 evtCode = h & 0xff;
        if(evtCode == HCI_EVT_LE_META)
        {
            u8 subEvt_code = p[0];
            //BLT_BIS_SINK_LOG("evtCode is %d subEvt_code is 0x%x", evtCode, subEvt_code);
            if(subEvt_code == HCI_SUB_EVT_LE_EXTENDED_ADVERTISING_REPORT) // Report Ext ADV packet
            {
                //Obtain AUX_SYNC_IND PDU, Step1: HCI_LE_Periodic_Advertising_Create_Sync
                blt_audio_sinkAdvReportEvt(p, n);
            }
            else if(subEvt_code == HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_ESTABLISHED)
            {
                //Step2: After receiving HCI_LE_Periodic_Advertising_Sync_Established Event(With Sync_Handle)
                blt_audio_sinkPeriodicAdvSync(p, n);
            }
            else if(subEvt_code == HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_TRANSFER_RECEIVED)  //PDA sync transfer received
            {
                //PAST Step1: PAST receive PDA sync.
                blt_audio_sinkPeriodicAdcSyncTransferRecv(p, n);
            }
            else if(subEvt_code == HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_REPORT)  //PDA report
            {
                //Step3: After receiving HCI_LE_Periodic_Advertising_Report Event
                blt_audio_sinkPeriodicAdvReport(p, n);
            }
            else if(subEvt_code == HCI_SUB_EVT_LE_BIGINFO_ADVERTISING_REPORT)
            {
                //Setp4:
                blt_audio_sinkBiginfoAdvReport(p, n);
            }
            else if (subEvt_code == HCI_SUB_EVT_LE_BIG_SYNC_ESTABLISHED)    // create BIG complete
            {
                //Step5:big sync established successful
                blt_audio_sinkBigSync(p, n);
            }
            else if(subEvt_code == HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_LOST)
            {
                blt_audio_sinkPeriodicAdvLost(p, n);
            }
            else if (subEvt_code == HCI_SUB_EVT_LE_BIG_SYNC_LOST)
            {
                blt_audio_sinkBigSyncLost(p, n);
            }
            else if(subEvt_code == HCI_SUB_EVT_LE_TERMINATE_BIG_COMPLETE)
            {
                blt_audio_sinkTerminateBigComplete(p, n);
            }
        }
    }

}




