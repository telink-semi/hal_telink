/********************************************************************************************************
 * @file    app_audio.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "app.h"
#include "app_config.h"
#include "app_audio.h"
#include "app_codec.h"
#include "app_audio_ui.h"
#include "app_parse_char.h"
#include "tlk_api/tlk_codec.h"

#if (INTER_TEST_MODE == TEST_LE_AUDIO_SWTICH_CLIENT)

/**
 *  @brief  app control block parameter
 */
app_ctrl_t appCtrl;
extern s8   cig_creat_flag;

static int  app_audio_prfEvtCb(u16 aclHandle, int evtID, u8 *pData, u16 dataLen);
static void app_event_acl_connect(u16 aclHandle, blc_prf_aclConnEvt_t *pEvt, u16 dataLen);
static void app_event_cis_connect(u16 aclHandle, blc_audio_cisConnEvt_t *pEvt, u16 dataLen);
static void app_event_cis_disconnect(u16 aclHandle, blc_audio_cisDisconnEvt_t *pEvt, u16 dataLen);
static void app_event_acl_disconnect(u16 aclHandle, blc_prf_aclDisconnEvt_t *pEvt, u16 dataLen);
static void app_event_audio_sdp_over(u16 aclHandle, blc_prf_sdpOverEvt_t *pEvt, u16 dataLen);
static void app_event_audio_sdp_found(u16 aclHandle, blc_prf_sdpFoundEvt_t *pEvt, u16 dataLen);
static void app_event_audio_sdp_not_found(u16 aclHandle, blc_prf_sdpFailEvt_t *pEvt, u16 dataLen);
static void app_event_codec_configured(u16 aclHandle, blc_bapuc_codecConfiguredEvt_t *pEvt, u16 dataLen);
static void app_event_qos_configured(u16 aclHandle, blc_bapuc_qosConfiguredEvt_t *pEvt, u16 dataLen);
static void app_event_enabling(u16 aclHandle, blc_bapuc_enablingEvt_t *pEvt, u16 dataLen);
static void app_event_disabling(u16 aclHandle, blc_bapuc_disablingEvt_t *pEvt, u16 dataLen);
static void app_event_releasing(u16 aclHandle, blc_bapuc_releasingEvt_t *pEvt, u16 dataLen);
static void app_event_receive_streaming(u16 aclHandle, blc_audio_streamingEvt_t *pEvt, u16 dataLen);
static void app_event_send_streaming(u16 aclHandle, blc_audio_streamingEvt_t *pEvt, u16 dataLen);

app_connect_info_t appConnInfo;
static connect_info_t *app_audio_allocateConnBuff(u16 connHandle, u8 addrType, u8 address[6]);
static int app_client_sdpEnd(u16 connHandle, u8 *pData, u16 dataLen);
static int app_bapba_foundSink(u16 connHandle, u8 *pData, u16 dataLen);
static int app_bapba_startedSyncPA(u16 connHandle, u8 *pData, u16 dataLen);
static int app_bapba_foundSourceInfo(u16 connHandle, u8 *pData, u16 dataLen);
static int app_bapba_sourceEncState(u16 connHandle, u8 *pData, u16 dataLen);
static int app_bapba_PastStartedReady(u16 connHandle, u8 *pData, u16 dataLen);
static int app_bassc_recvSinkState(u16 connHandle, u8 *pData, u16 dataLen);
static int app_bassc_bcstCodeReq(u16 connHandle, u8 *pData, u16 dataLen);
static int app_bassc_badBcstCode(u16 connHandle, u8 *pData, u16 dataLen);
static int app_vcsc_changedVolState(u16 connHandle, u8 *pData, u16 dataLen);
static int app_vocsc_changedVolOffset(u16 connHandle, u8 *pData, u16 dataLen);


/**
 * @brief      audio module init.
 * @param[in]  none.
 * @return     none.
 */
void app_audio_init(void)
{
    /* Audio profile event register */
    blc_audio_initialModule(app_audio_prfEvtCb);

    /* Audio CAP initiator init */
    blc_cap_initUnicastInitiator();

    /* CAP Broadcast Commander initial */
    blc_cap_initBcstCommander();

#if (ACL_PERIPHR_SMP_ENABLE || ACL_CENTRAL_SMP_ENABLE)
    /* Audio bonding initial */
    blc_prf_initPairingInfoStoreModule();
#endif

    appCtrl.acl_max_num = ACL_CENTRAL_MAX_NUM;

    /* Codec init */
    app_codec_init();

    /* Cis uart/usb-cdc UI initial */
    app_audio_ui_init();

    app_parse_printf("Broadcast Assistant initial.\r\n");

#if APP_AUDIO_ASRC_EN
    /* ASRC init */
    my_asrc_init_stereo(0);
    tlkapi_printf(APP_LOG_EN,"Init_ASRC");
#endif
}

/**
 * @brief      audio loop handler process.
 * @param[in]  none.
 * @return     none.
 */
void app_audio_handler(void)
{
    /* audio profile loop entry */
    blc_prf_main_loop();
    /* audio codec loop entry */
    app_codec_handler();
}

/**
 * @brief       Unicast Client register profile event callback.
 */
static const app_audio_evtCb_t unicastCb[] = {
    /* Event for controller or Host */
    {PRF_EVTID_ACL_CONNECT              , (void*)app_event_acl_connect},
    {PRF_EVTID_ACL_DISCONNECT           , (void*)app_event_acl_disconnect},
    {AUDIO_EVT_CIS_CONNECT              , (void*)app_event_cis_connect},
    {AUDIO_EVT_CIS_DISCONNECT           , (void*)app_event_cis_disconnect},
    /* Event for Client SDP */
    {PRF_EVTID_CLIENT_SDP_FOUND         , (void*)app_event_audio_sdp_found},
    {PRF_EVTID_CLIENT_SDP_FAIL          , (void*)app_event_audio_sdp_not_found},
    {PRF_EVTID_CLIENT_SDP_END           , app_client_sdpEnd},
    {PRF_EVTID_CLIENT_ALL_SDP_OVER      , (void*)app_event_audio_sdp_over},
    /* Event for BAP Unicast Client */
    {AUDIO_EVT_BAPUC_CODEC_CONFIGURED   , (void*)app_event_codec_configured},
    {AUDIO_EVT_BAPUC_QOS_CONFIGURED     , (void*)app_event_qos_configured},
    {AUDIO_EVT_BAPUC_ENABLING           , (void*)app_event_enabling},
    {AUDIO_EVT_BAPUC_DISABLING          , (void*)app_event_disabling},
    {AUDIO_EVT_BAPUC_RELEASING          , (void*)app_event_releasing},
    {AUDIO_EVT_BAPUC_RECEIVE_STREAMING  , (void*)app_event_receive_streaming}, //Client as Sink, receive audio
    {AUDIO_EVT_BAPUC_SEND_STREAMING     , (void*)app_event_send_streaming},    //Client as Source, send audio

    /* Event for BAP Broadcast Assistant */
    {AUDIO_EVT_BAPBA_FOUND_SINK             , app_bapba_foundSink},
    {AUDIO_EVT_BAPBA_START_SYNC_PA          , app_bapba_startedSyncPA},
    {AUDIO_EVT_BAPBA_FOUND_SOURCE_INFO      , app_bapba_foundSourceInfo},
    {AUDIO_EVT_BAPBA_SOURCE_ENC_STATE       , app_bapba_sourceEncState},
    {AUDIO_EVT_BAPBA_PAST_STARTED_READY     , app_bapba_PastStartedReady},
    /* Event for BASS Client */
    {AUDIO_EVT_BASSC_RECV_SINK_STATE        , app_bassc_recvSinkState},
    {AUDIO_EVT_BASSC_BROADCAST_CODE_REQ     , app_bassc_bcstCodeReq},
    {AUDIO_EVT_BASSC_BAD_BROADCAST_CODE     , app_bassc_badBcstCode},
    /* Event for VCS Client */
    {AUDIO_EVT_VCSC_CHANGED_VOLUME_STATE    , app_vcsc_changedVolState},
    {AUDIO_EVT_VOCSC_CHANGED_VOLUME_OFFSET  , app_vocsc_changedVolOffset},
};

/**
 * @brief       audio profile event callback function.
 * @param[in]   aclHandle: ACL connect handle.
 * @param[in]   evtID: audio event ID, refer audio_event_enum.
 * @param[in]   pData: Data carried by the event.
 * @param[in]   dataLen: data length.
 * @return      0/1.
 */
static int app_audio_prfEvtCb(u16 aclHandle, int evtID, u8 *pData, u16 dataLen)
{
    for(int i=0; i < ARRAY_SIZE(unicastCb); i++)
    {
        if(unicastCb[i].id == evtID){
            unicastCb[i].evtCb(aclHandle, (void*)pData, dataLen);
        }
    }
    return 0;
}

s8 app_audio_getHandleIndex(u16 connHandle)
{
    tlkapi_printf(APP_LOG_EN,"acl_max_num:0x%x\n", appCtrl.acl_max_num);
    tlkapi_printf(APP_LOG_EN,"acl_handle[0]:0x%x\n", appCtrl.aclParam[0].acl_handle);
    tlkapi_printf(APP_LOG_EN,"acl_handle[1]:0x%x\n", appCtrl.aclParam[1].acl_handle);
    for(int i=0;i<appCtrl.acl_max_num;i++)
    {
        if(appCtrl.aclParam[i].acl_handle == connHandle)
        {
            return i;
        }
    }
    return -1;
}

static void app_event_acl_connect(u16 aclHandle, blc_prf_aclConnEvt_t *pEvt, u16 dataLen)
{
    tlkapi_printf(APP_LOG_EN,"app event-acl connect\n");

    for(int i=0;i<appCtrl.acl_max_num;i++)
    {
        if(appCtrl.aclParam[i].acl_handle == 0)
        {
            appCtrl.aclParam[i].acl_handle = pEvt->aclHandle;
            tlkapi_printf(APP_LOG_EN,"acl handle match-index:0x%x\n", i);
            tlkapi_printf(APP_LOG_EN,"acl handle match-handle:0x%x\n", appCtrl.aclParam[i].acl_handle);
            break;
        }
    }
    appCtrl.acl_count++;
//  if(appCtrl.acl_count >= appCtrl.acl_max_num)
//  {
//      blc_ll_setExtScanEnable( BLC_SCAN_DISABLE, DUP_FILTER_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
//      tlkapi_printf(APP_LOG_EN,"acl connect cnt-stop scan:0x%x\n", appCtrl.acl_count);
//  }
//  else
//  {
//      blc_ll_setExtScanEnable( BLC_SCAN_ENABLE, DUP_FILTER_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
//      tlkapi_printf(APP_LOG_EN,"acl connect cnt-start scan:0x%x\n", appCtrl.acl_count);
//  }

    blc_prf_aclConnEvt_t* aclConn = (blc_prf_aclConnEvt_t*)pEvt;

    blc_hci_le_getRemoteSupportedFeatures(aclHandle);

    if(app_audio_allocateConnBuff(aclConn->aclHandle, aclConn->PeerAddrType, aclConn->PeerAddr)){
        appConnInfo.connCnt++;
        tlkapi_printf(APP_LOG_EN, "[I] ACL connection complete:0x%x\n", aclConn->aclHandle);
    }
}
static void app_event_cis_connect(u16 aclHandle, blc_audio_cisConnEvt_t *pEvt, u16 dataLen)
{
    tlkapi_printf(APP_LOG_EN,"app event-cis connect\n");

    cig_creat_flag = 1;

    s8 acl_index = app_audio_getHandleIndex(aclHandle);
    if(acl_index<0)
    {
        tlkapi_printf(APP_LOG_EN,"error-get acl handle:0x%x\n", aclHandle);
    }

    #if (UI_LED_ENABLE)
        gpio_write(GPIO_LED_GREEN, 1);
    #endif

    tlkapi_printf(APP_LOG_EN,"nse:0x%x\n", pEvt->nse);
    tlkapi_printf(APP_LOG_EN,"ft m2s:0x%x\n", pEvt->ft_m2s);
    tlkapi_printf(APP_LOG_EN,"ft s2m:0x%x\n", pEvt->ft_s2m);
}

static void app_event_cis_disconnect(u16 aclHandle, blc_audio_cisDisconnEvt_t *pEvt, u16 dataLen)
{
    tlkapi_printf(APP_LOG_EN,"app event-cis disconnect %x\n",aclHandle);

    cig_creat_flag = 0;

    s8 acl_index = app_audio_getHandleIndex(aclHandle);
    if(acl_index<0)
    {
        tlkapi_printf(APP_LOG_EN,"error-get acl handle:0x%x\n", aclHandle);
        return;
    }

    appCtrl.aclParam[acl_index].sink.sS = false;
    appCtrl.aclParam[acl_index].source.sS = false;
    tlkapi_printf(APP_LOG_EN,"reason:0x%x\n", pEvt->reason);

    appCtrl.aclParam[acl_index].sink.sT = false;
    appCtrl.aclParam[acl_index].source.sT = false;

    appCtrl.acl_csis_exist = 0;
    appCtrl.acl_csis_size = 0;
    memset(appCtrl.acl_csis_sirk,0,16);
    tlk_codec_stop(TLK_CODEC_INPUT);
    tlk_codec_stop(TLK_CODEC_OUTPUT);

    blc_isoal_clearCisSdu(pEvt->cisHandle);

    #if (UI_LED_ENABLE)
        gpio_write(GPIO_LED_GREEN, 0);
    #endif

}
static void app_event_acl_disconnect(u16 aclHandle, blc_prf_aclDisconnEvt_t *pEvt, u16 dataLen)
{
    tlkapi_printf(APP_LOG_EN,"app event-acl disconnect\n");

    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if(acl_index<0)
    {
        tlkapi_printf(APP_LOG_EN,"error-get acl handle:0x%x\n", pEvt->aclHandle);
        return;
    }
    memset((u8*)&appCtrl.aclParam[acl_index].acl_handle,0,sizeof(app_acl_param_t));
    appCtrl.acl_count--;
    if(appCtrl.acl_count == 0)
    {
        appCtrl.acl_csis_exist = 0;
        appCtrl.acl_csis_size = 0;
        memset(appCtrl.acl_csis_sirk,0,16);
        tlk_codec_stop(TLK_CODEC_INPUT);
        tlk_codec_stop(TLK_CODEC_OUTPUT);
    }
    int scan_ret = blc_ll_setExtScanEnable( BLC_SCAN_ENABLE, DUP_FILTER_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
    if(scan_ret!=BLE_SUCCESS)
    {
        tlkapi_printf(APP_LOG_EN,"start scan error:0x%x\n", scan_ret);
    }
    else
    {
        tlkapi_printf(APP_LOG_EN,"start scan success:0x%x\n", scan_ret);
    }

    for(int i=0; i<MAX_LINK_NUM; i++)
    {
        connect_info_t *pConn = &appConnInfo.conn[i];
        if(pConn->isUsed && aclHandle == pConn->connHandle){
            app_parse_printf("acl disconnected Handle:%x, Addr %s\r\n", aclHandle, addr_to_str(pConn->address));
            memset(pConn, 0, sizeof(connect_info_t));
        }
    }

    if(appConnInfo.connCnt){
        appConnInfo.connCnt--;
    }

    tlkapi_printf(APP_LOG_EN, "[I] ACL disconnection complete:0x%x\n", aclHandle);
}
static void app_event_audio_sdp_found(u16 aclHandle, blc_prf_sdpFoundEvt_t *pEvt, u16 dataLen)
{
    tlkapi_printf(APP_LOG_EN,"app event-audio sdp found\n");

    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if(acl_index<0)
    {
        tlkapi_printf(APP_LOG_EN,"error-get acl handle:0x%x\n", pEvt->aclHandle);
        return;
    }

    if(pEvt->svcId == AUDIO_CSIS_CLIENT){
        if(appCtrl.acl_count == 1){
            appCtrl.acl_csis_exist = 1;
            appCtrl.acl_csis_size = 0; //clear. update latter by clientSDP
            tlkapi_printf(APP_LOG_EN,"acceptor with CSIS\n");
        }
    }

    extern blc_vcp_client_t *blt_vcp_getClientInst(u16 connHandle);
    blc_vcp_client_t *vcp = blt_vcp_getClientInst(aclHandle);
    extern void blt_vcp_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);
    vcp->vcsClient.ntfInput.ntfOrIndFunc = blt_vcp_dataInput;
    blc_gattc_addSubscribeCCCNode(aclHandle, &vcp->vcsClient.ntfInput);

    blc_prf_sdpFoundEvt_t* sdpFound = (blc_prf_sdpFoundEvt_t*)pEvt;

    if(sdpFound->svcId == AUDIO_PACS_CLIENT) {
        app_parse_printf("ConnHandle:%x, PACS Found Start.\r\n", aclHandle);
    }
    else if(sdpFound->svcId == AUDIO_BASS_CLIENT) {
        app_parse_printf("ConnHandle:%x, BASS Found Start.\r\n", aclHandle);
    }
    else if(sdpFound->svcId == AUDIO_VCP_CLIENT) {
        app_parse_printf("ConnHandle:%x, VCP Volume Controller Found Start.\r\n", aclHandle);
    }

    tlkapi_printf(APP_LOG_EN, "sdp Found, connHandle:0x%x ID:0x%x startHandle:0x%x endHandle:0x%x\n",
            sdpFound->aclHandle, sdpFound->svcId, sdpFound->startHdl, sdpFound->endHdl);

}
static void app_event_audio_sdp_not_found(u16 aclHandle, blc_prf_sdpFailEvt_t *pEvt, u16 dataLen)
{
    tlkapi_printf(APP_LOG_EN,"app event-audio sdp not found\n");

    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if(acl_index<0)
    {
        tlkapi_printf(APP_LOG_EN,"error-get acl handle:0x%x\n", pEvt->aclHandle);
        return;
    }

    if(pEvt->svcId == AUDIO_CSIS_CLIENT){
        if(appCtrl.acl_count == 1){
            appCtrl.acl_csis_exist = 0;
            appCtrl.acl_csis_size = 1;
            tlkapi_printf(APP_LOG_EN,"acceptor without CSIS, client support 1 acceptor");
        }
    }

    blc_prf_sdpFailEvt_t* sdpFail = (blc_prf_sdpFailEvt_t*)pEvt;
    if(sdpFail->svcId == AUDIO_PACS_CLIENT) {
        app_parse_printf("ConnHandle:%x, PACS Found Failed.\r\n", aclHandle);
    }
    else if(sdpFail->svcId == AUDIO_BASS_CLIENT) {
        app_parse_printf("ConnHandle:%x, BASS Found Failed.\r\n", aclHandle);
    }
    else if(sdpFail->svcId == AUDIO_VCP_CLIENT) {
        app_parse_printf("ConnHandle:%x, VCP Volume Controller Found Failed.\r\n", aclHandle);
    }

    tlkapi_printf(APP_LOG_EN, "sdp fail, connHandle:0x%x ID:0x%x\n", sdpFail->aclHandle, sdpFail->svcId);

}
static void app_event_audio_sdp_over(u16 aclHandle, blc_prf_sdpOverEvt_t *pEvt, u16 dataLen)
{
    tlkapi_printf(APP_LOG_EN,"app event-audio sdp over\n");

    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if(acl_index<0)
    {
        tlkapi_printf(APP_LOG_EN,"error-get acl handle:0x%x\n", pEvt->aclHandle);
        return;
    }

    if(appCtrl.acl_count == 1 && appCtrl.acl_csis_exist){
        int r = blc_csisc_getSetIdentityResolvingKey(pEvt->aclHandle, appCtrl.acl_csis_sirk);
        if(AUDIO_ESUCC != r){
            tlkapi_printf(APP_LOG_EN,"error-get SIRK:0x%x\n", r);
            return;
        }else{
            tlkapi_printf(APP_LOG_EN,"CSISC: get SIRK:%s\n", hex_to_str(appCtrl.acl_csis_sirk, 16));
        }
        r = blc_csisc_getCoordinatedSetSize(pEvt->aclHandle, &appCtrl.acl_csis_size);
        if(AUDIO_ESUCC != r){
            tlkapi_printf(APP_LOG_EN,"error-get SetSize:0x%x\n", r);
            return;
        }else{
            tlkapi_printf(APP_LOG_EN,"CSISC: SetSize:0x%x\n", appCtrl.acl_csis_size);
        }
    }

    connect_info_t* pConn = app_audio_getConn(aclHandle);

    app_parse_printf("ConnHandle:%x, SDP over\r\n", aclHandle);
    tlkapi_printf(APP_LOG_EN, "broadcast assistant sdk over\n");

    pConn->sdp_over = 1;

    if(pConn->sinkState == CONNECT_SINK_HAD_SOURCE) {
        app_parse_printf("Sink Had Sync BIS\r\n");
    }

    blc_ll_updateConnection(aclHandle, CONN_INTERVAL_100MS, CONN_INTERVAL_200MS, 0, CONN_TIMEOUT_3S, 0, 0xFFFF );


//  blc_audio_ase_cfg_info_t audChnInfo;
//  int audioRet = blc_bapuc_checkAudioConfigures(pEvt->aclHandle, APP_AUDIO_CONFIGURATION_PREFER, &audChnInfo);
//  if(audioRet!= AUDIO_ESUCC)
//  {
//      tlkapi_printf(APP_LOG_EN,"error-audio configurations:0x%x\n", audioRet);
//      return;
//  }
//
//  ////////////////////// SVR: SINK; CLT: SOURCE //////////////////////////////////////
//  for(int i = 0; i<audChnInfo.sinkASEsPerSvr; i++)
//  {
//      audioRet = blc_bapuc_setAseConfigCodec(pEvt->aclHandle,audChnInfo.sinkASEId[i], APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER, &audChnInfo);
//      if(audioRet!= AUDIO_ESUCC)
//      {
//          tlkapi_printf(APP_LOG_EN,"error-unicast config audio:0x%x\n", audioRet);
//      }
//      else
//      {
//          appCtrl.aclParam[acl_index].source.codecParam = APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER;
//          appCtrl.aclParam[acl_index].source.blocks = audChnInfo.sinkCodecFrameBlksPerSDU;
//          appCtrl.aclParam[acl_index].source.codecOp = APP_CONFIG_CODEC;
//          tlkapi_printf(APP_LOG_EN,"source config codec-blocks %d\n",appCtrl.aclParam[acl_index].source.blocks);
//      }
//  }
//
//  ////////////////////// SVR: SOURCE;  CLT: SINK //////////////////////////////////////
//  for(int i = 0; i<audChnInfo.srcASEsPerSvr; i++)
//  {
//      audioRet = blc_bapuc_setAseConfigCodec(pEvt->aclHandle, audChnInfo.srcASEId[i], APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER, &audChnInfo);
//      if(audioRet!= AUDIO_ESUCC)
//      {
//          tlkapi_printf(APP_LOG_EN,"error-unicast config audio:0x%x\n", audioRet);
//      }
//      else
//      {
//          appCtrl.aclParam[acl_index].sink.codecParam = APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER;
//          appCtrl.aclParam[acl_index].sink.blocks = audChnInfo.srcCodecFrameBlksPerSDU;
//          appCtrl.aclParam[acl_index].sink.codecOp = APP_CONFIG_CODEC;
//          tlkapi_printf(APP_LOG_EN,"sink config codec-blocks %d\n",appCtrl.aclParam[acl_index].sink.blocks);
//      }
//  }
}
static void app_event_codec_configured(u16 aclHandle, blc_bapuc_codecConfiguredEvt_t *pEvt, u16 dataLen)
{
    tlkapi_printf(APP_LOG_EN,"app event-codec configured\n");

    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if(acl_index<0)
    {
        tlkapi_printf(APP_LOG_EN,"error-get acl handle:0x%x\n", pEvt->aclHandle);
        return;
    }
    if(pEvt->aseDir == AUDIO_DIR_SOURCE)
    {
        tlkapi_printf(APP_LOG_EN,"source max latency %d\n",pEvt->maxTransportLatency);
        blc_bapuc_setAseConfigQos(pEvt->aclHandle, pEvt->aseID, APP_AUDIO_QOS_OUTPUT_PARAMETER_PREFER);
    }
    else
    {
        tlkapi_printf(APP_LOG_EN,"sink max latency %d\n",pEvt->maxTransportLatency);
        blc_bapuc_setAseConfigQos(pEvt->aclHandle, pEvt->aseID, APP_AUDIO_QOS_OUTPUT_PARAMETER_PREFER);
    }
}
static void app_event_qos_configured(u16 aclHandle, blc_bapuc_qosConfiguredEvt_t*pEvt, u16 dataLen)
{
    tlkapi_printf(APP_LOG_EN,"app event-qos configured\n");

    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if(acl_index<0)
    {
        tlkapi_printf(APP_LOG_EN,"error-get acl handle:0x%x\n", pEvt->aclHandle);
        return;
    }
    if(pEvt->aseDir == AUDIO_DIR_SOURCE)
    {
        appCtrl.aclParam[acl_index].sink.pD = pEvt->presentationDelay;
        tlkapi_printf(APP_LOG_EN,"sink presentation delay:0x%x\n",  appCtrl.aclParam[acl_index].sink.pD);
    }
    else
    {
        tlkapi_printf(APP_LOG_EN,"source presentation delay:0x%x\n", pEvt->presentationDelay);
    }
}
static void app_event_receive_streaming(u16 aclHandle, blc_audio_streamingEvt_t *pEvt, u16 dataLen)
{
    tlkapi_printf(APP_LOG_EN,"app event-receive streaming\n");

    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if(acl_index<0)
    {
        tlkapi_printf(APP_LOG_EN,"error-get acl handle:0x%x\n", pEvt->aclHandle);
        return;
    }
    tlk_codec_sts_e codecRet = tlk_codec_start(TLK_CODEC_OUTPUT);
    if(codecRet == TLK_CODEC_STATE_ERROR)
    {
        tlkapi_printf(APP_LOG_EN,"error codec state %d",codecRet);
    }
    else if(codecRet == TLK_CODEC_OPERATION_REPEAT)
    {
        tlkapi_printf(APP_LOG_EN,"codec already enable %d",codecRet);
    }
    else
    {
        tlkapi_printf(APP_LOG_EN,"codec-output enable success");
    }
    appCtrl.aclParam[acl_index].sink.sS = true;
}
static void app_event_send_streaming(u16 aclHandle, blc_audio_streamingEvt_t *pEvt, u16 dataLen)
{
    tlkapi_printf(APP_LOG_EN,"app event-send streaming\n");

    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if(acl_index<0)
    {
        tlkapi_printf(APP_LOG_EN,"error-get acl handle:0x%x\n", pEvt->aclHandle);
        return;
    }
    tlk_codec_sts_e codecRet = tlk_codec_start(TLK_CODEC_INPUT);
    if(codecRet == TLK_CODEC_STATE_ERROR)
    {
        tlkapi_printf(APP_LOG_EN,"error codec state %d",codecRet);
    }
    else if(codecRet == TLK_CODEC_OPERATION_REPEAT)
    {
        tlkapi_printf(APP_LOG_EN,"codec already enable %d",codecRet);
    }
    else
    {
        tlkapi_printf(APP_LOG_EN,"codec-input enable success");
    }
    appCtrl.aclParam[acl_index].source.sS = true;
}
static void app_event_enabling(u16 aclHandle, blc_bapuc_enablingEvt_t *pEvt, u16 dataLen)
{
    tlkapi_printf(APP_LOG_EN,"app event-enabling\n");

    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if(acl_index<0)
    {
        tlkapi_printf(APP_LOG_EN,"error-get acl handle:0x%x\n", pEvt->aclHandle);
        return;
    }
    if(pEvt->aseDir == AUDIO_DIR_SOURCE)//Client as audio sink, if ready to receive data,execute 'receive start ready' operation to inform Server
    {
        blc_bapuc_setAseReceiverStartReady(pEvt->aclHandle,pEvt->aseID);
    }
    else
    {
        //Client as audio source,if server ready to receive data,will execute 'receive start ready' automatically.
    }
}
static void app_event_disabling(u16 aclHandle, blc_bapuc_disablingEvt_t *pEvt, u16 dataLen)
{
    tlkapi_printf(APP_LOG_EN,"app event-disabling\n");

    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if(acl_index<0)
    {
        tlkapi_printf(APP_LOG_EN,"error-get acl handle:0x%x\n", pEvt->aclHandle);
        return;
    }
    if(pEvt->aseDir == AUDIO_DIR_SOURCE)//Client as audio sink, if ready to stop receive data,execute 'receive stop ready' operation to inform Server
    {
        blc_bapuc_setAseReceiverStopReady(pEvt->aclHandle,pEvt->aseID);
    }
}
static void app_event_releasing(u16 aclHandle, blc_bapuc_releasingEvt_t *pEvt, u16 dataLen)
{
    tlkapi_printf(APP_LOG_EN,"app event-releasing\n");
    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if(acl_index<0)
    {
        tlkapi_printf(APP_LOG_EN,"error-get acl handle:0x%x\n", pEvt->aclHandle);
        return;
    }
}


/************************************** BIS Assistant Start ****************************************/
const char locationStr[32][30] = {
    {"Front Left"}, {"Front Right"}, {"Front Center"}, {"Low Frequency Effects 1"},
    {"Back Left"}, {"Back Right"}, {"Front Left of Center"}, {"Front Right of Center"},
    {"Back Center"}, {"Low Frequency Effects 2"}, {"Side Left"}, {"Side Right"},
    {"Top Front Left"}, {"Top Front Right"}, {"Top Front Center"}, {"Top Center"},
    {"Top Back Left"}, {"Top Back Right"}, {"Top Side Left"}, {"Top Side Right"},
    {"Top Back Center"}, {"Bottom Front Center"}, {"Bottom Front Left"}, {"Bottom Front Right"},
    {"Front Left Wide"}, {"Front Right Wide"}, {"Left Surround"}, {"Right Surround"},
    {"RFU"}, {"RFU"}, {"RFU"}, {"RFU"},
};

bool muteFlag = 1;

source_info_t sourceInfoTable[MAX_SOURCE_INFO_NUM];
int    sourceCnt = 0;

sink_info_t sinkInfoTable[MAX_SINK_INFO_NUM];
int sinkCnt = 0;
int sinkFlag = 1;

/******************************************************************************/
/* Broadcast Assistant                                                        */
/******************************************************************************/

/**
 * @brief       assistant set source information.
 * @param[in]   info: source information pointer.
 * @return      initial source count.
 */
static int app_audio_setSourceInfo(blc_audio_source_head_t* info)
{
    source_info_t *source = &sourceInfoTable[sourceCnt++];

    source->advSID = info->sid;
    source->advAddrType = info->addrType;
    memcpy(source->advAddr, info->addr, 6);
    memcpy(source->broadcastId, info->broadcastId, 3);
    source->usedFlag = 0;
    return sourceCnt;
}

/**
 * @brief       assistant initial/clean source information buffer.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_initSourceInfoBuf(void)
{
    sourceCnt = 0;
    memset(&sourceInfoTable[0], 0, sizeof(source_info_t)*MAX_SOURCE_INFO_NUM);
}

/**
 * @brief       assistant find source information by advSId/addrType/Addr.
 * @param[in]   advSID: advertise SID.
 * @param[in]   advAddrType: advertise address type.
 * @param[in]   advAddr: advertise address
 * @return      source information point or NULL.
 */
static source_info_t *app_audio_findSourceInfo(u8 advSID, u8 advAddrType, u8 advAddr[6])
{
    tlkapi_printf(0, "app event-assistant find source information");

    if(sourceCnt >= MAX_SOURCE_INFO_NUM)
    {
        return &sourceInfoTable[0];
    }

    for(int i=0; i<sourceCnt; i++)
    {
        source_info_t *source = &sourceInfoTable[i];

        if(source->advSID == advSID && source->advAddrType == advAddrType && !memcmp(source->advAddr,advAddr,6))
        {
            return source;
        }
    }
    return NULL;
}

/**
 * @brief       assistant calculate source information index.
 * @param[in]   sourceInfo: source information pointer.
 * @return      source information index.
 */
static int app_audio_getSourceInfoIndex(source_info_t* sourceInfo)
{
    int index = 0;
    for(int i=0; i<sourceCnt; i++)
    {
        source_info_t *source = &sourceInfoTable[i];
        if(source->usedFlag)    index++;
        if(source == sourceInfo)    break;
    }
    return index;
}

/**
 * @brief       assistant get source information by index.
 * @param[in]   index: source information index.
 * @return      source information pointer/NULL.
 */
source_info_t *app_audio_getSourceInfo(u8 index)
{
    if(sourceCnt < index)
    {
        return NULL;
    }

    int validIndex = 0;
    for(int i=0; i<sourceCnt; i++)
    {
        source_info_t *source = &sourceInfoTable[i];
        if(source->usedFlag)
        {
            validIndex++;
            if(validIndex == index)
                return &sourceInfoTable[i];
        }
    }

    return NULL;
}

/**
 * @brief       app display source information to uart/usb-cdc.
 * @param[in]   source: source information pointer.
 * @return      none.
 */
static void app_audio_displaySourceInfo(source_info_t *source)
{
    int i = app_audio_getSourceInfoIndex(source);

    app_parse_printf("Found Source Info[%x]\r\n\t%s Address is %s\r\n", i, source->advAddrType?"random":"public", addr_to_str(source->advAddr));
    app_parse_printf("\tDevice Name:%s\r\n\tBroadcast Name:%s\r\n", source->completeName, source->broadcastName);

    char samplingFreqStr[14][9] = {
        {""},
        {"8kHz"}, {"11025Hz"}, {"16kHz"}, {"22050Hz"}, {"24kHz"},
        {"32kHz"}, {"44.1kHz"}, {"48kHz"}, {"88.2Hz"}, {"96kHz"},
        {"176.4kHz"}, {"192kHz"}, {"384kHz"}
    };

    for(int i=0; i<source->bisCnt; i++)
    {
        app_parse_printf("\t\tBIS Index: %d\r\n\t\tCodec ID: %s\r\n\t\tSampling Frequency: %s\r\n",
                source->bisIndex[i], hex_to_str(&source->bisInfo[i].CodecId.id, 5),
                &samplingFreqStr[source->bisInfo[i].codecCfg.frequency][0]);

        for(int j=0; j<32; j++)
        {
            if(source->bisInfo[i].codecCfg.allocation & BIT(j))
            {
                app_parse_printf("\t\t%s: Supported\r\n", &locationStr[j][0]);
            }
        }
    }

    if(source->enc) {
        app_parse_printf("\tSource is Encrypted!!!\r\n");
    }
    else {
        app_parse_printf("\tSource is Unencrypted\r\n");
    }

}

/**
 * @brief       app display all source information to uart/usb-cdc.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_showSourceInfo(void)
{
    int index = 0;
    for(int i=0; i<sourceCnt; i++)
    {
        source_info_t *source = &sourceInfoTable[i];
        if(source->usedFlag){
            index++;
            app_audio_displaySourceInfo(source);
        }
    }

    if(!index)          app_parse_printf("No Source Scanned\r\n");

}


/******************************************************************************/
/* ACL connection  handle                                                     */
/******************************************************************************/

/**
 * @brief       allocate connect information buffer.
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   addrType: remote device address type.
 * @param[in]   address: remote device address.
 * @return      connect information point/NULL.
 */
static connect_info_t *app_audio_allocateConnBuff(u16 connHandle, u8 addrType, u8 address[6])
{
    char* deviceName = NULL;
    for(int i=0; i<MAX_SINK_INFO_NUM; i++)
    {
        if(memcmp(sinkInfoTable[i].address, address, 6) == 0 && sinkInfoTable[i].addrType == addrType)
        {
            deviceName = sinkInfoTable[i].deviceName;
        }
    }

    for(int i=0; i<MAX_LINK_NUM; i++)
    {
        connect_info_t *pConn = &appConnInfo.conn[i];
        if(!pConn->isUsed){
            pConn->isUsed = true;
            pConn->connHandle = connHandle;
            pConn->addrType = addrType;
            memcpy(pConn->address, address, 6);
            strncpy(pConn->deviceName, deviceName, sizeof(pConn->deviceName));
            return pConn;
        }
    }
    return NULL;
}

/**
 * @brief       get connect information buffer by connHandle.
 * @param[in]   connHandle: ACL connect handle.
 * @return      connect information point/NULL.
 */
connect_info_t * app_audio_getConn(u16 connHandle)
{
    for(int i=0; i<MAX_LINK_NUM; i++)
    {
        connect_info_t *pConn = &appConnInfo.conn[i];
        if(pConn->isUsed && connHandle == pConn->connHandle){
            return pConn;
        }
    }
    return NULL;
}

/**
 * @brief       check acl connect count full.
 * @param[in]   none.
 * @return      true:ACL connect full.
 */
bool app_audio_aclConnFull(void)
{
    return appConnInfo.connCnt >= MAX_LINK_NUM;
}

/**
 * @brief       get connHandle by index.
 * @param[in]   index: ACL connect index.
 * @return      connHandle.
 */
u16 app_audio_getConnHandle(u8 index)
{
    if(appConnInfo.connCnt < index)
        return 0x0000;

    connect_info_t *pConn = &appConnInfo.conn[index-1];
    return pConn->isUsed? pConn->connHandle: 0x0000;
}

/**
 * @brief       app display all connect information to uart/usb-cdc.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_showConnInfo(void)
{
    if(!appConnInfo.connCnt)
    {
        app_parse_printf("No Connection information\r\n");
        return ;
    }

    for(int i=0; i<appConnInfo.connCnt; i++)
    {
        u8*address = appConnInfo.conn[i].address;

        app_parse_printf("[%d]Connect:%s %s name:%s\r\n", i+1, appConnInfo.conn[i].addrType?"random":"public", addr_to_str(address), appConnInfo.conn[i].deviceName);
    }
}

SourceInfo_t foundInfoTemp;

/**
 * @brief       assistant initial/clean connect information buffer.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_initSinkInfoBuf(void)
{
    sinkCnt = 0;
    sinkFlag = 1;
    memset(&sinkInfoTable[0], 0, sizeof(sink_info_t)*MAX_SINK_INFO_NUM);
}

/**
 * @brief       open scan sink switch.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_openScanSink(void)
{
    sinkFlag = 0;
}

/**
 * @brief       close scan sink switch.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_closeScanSink(void)
{
    sinkFlag = 1;
}

/**
 * @brief       find sink information and store to buffer.
 * @param[in]   addrType: sink address type.
 * @param[in]   address: sink address.
 * @param[in]   deviceName: sink device name.
 * @return      0: sink Already exist/ Insufficient buffer.
 *              other: sink index.
 */
static u8 app_audio_findSinkInfo(u8 addrType, u8 address[6], char* deviceName)
{
    if(sinkCnt == MAX_SINK_INFO_NUM)
        return 0;

    for(int i=0; i<sinkCnt; i++)
    {
        sink_info_t *sinkInfo = &sinkInfoTable[i];

        if(sinkInfo->addrType == addrType && !memcmp(sinkInfo->address, address, 6))
        {
            return 0;
        }
    }
    sinkInfoTable[sinkCnt].addrType = addrType;
    memcpy(sinkInfoTable[sinkCnt].address, address, 6);
    strncpy(sinkInfoTable[sinkCnt].deviceName, deviceName, sizeof(sinkInfoTable[sinkCnt].deviceName));
    sinkCnt++;
    return sinkCnt;
}

/**
 * @brief       app display all scan sink information to uart/usb-cdc.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_showSinkInfo(void)
{
    if(sinkFlag)    app_parse_printf("Now Stop Scan new Sink\r\n");

    if(!sinkCnt)    app_parse_printf("No Scan information\r\n");

    for(int i=0; i<sinkCnt; i++)
    {
        sink_info_t *sinkInfo = &sinkInfoTable[i];
        app_parse_printf("[%d] %s %s name:%s\r\n", i+1, sinkInfo->addrType?"random":"public", addr_to_str(sinkInfo->address), sinkInfo->deviceName);
    }
}

/**
 * @brief       create sink ACL connect by index.
 * @param[in]   index: scan sink index.
 * @return      0: index error.
 *              1: index true.
 */
int app_audio_createSinkConn(int index)
{
    if(index > sinkCnt)
        return 0;

    blc_ll_extended_createConnection(INITIATE_FP_ADV_SPECIFY, OWN_ADDRESS_PUBLIC,
                                    sinkInfoTable[index-1].addrType, sinkInfoTable[index-1].address, INIT_PHY_1M,
                                    SCAN_INTERVAL_30MS, SCAN_INTERVAL_30MS, CONN_INTERVAL_18P75MS, CONN_INTERVAL_18P75MS, CONN_TIMEOUT_4S,
                                    0,0,0,0,0,
                                    0,0,0,0,0);
    return 1;
}

/******************************************************************************/
/* Broadcast Assistant profile event callback                                 */
/******************************************************************************/

/**
 * @brief       audio profile single client sdp end event.
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   pData: Data carried by the event.
 * @param[in]   dataLen: data length.
 * @return      0.
 */
static int app_client_sdpEnd(u16 connHandle, u8 *pData, u16 dataLen)
{
    blc_prf_sdpFoundEvt_t* sdpFound = (blc_prf_sdpFoundEvt_t*)pData;

    connect_info_t* pConn = app_audio_getConn(connHandle);

    if(sdpFound->svcId == AUDIO_PACS_CLIENT) {
        pConn->PACS_server = 1;
        app_parse_printf("ConnHandle:%x, PACS Found End.\r\n", connHandle);
    }
    else if(sdpFound->svcId == AUDIO_BASS_CLIENT) {
        pConn->BASS_server = 1;
        app_parse_printf("ConnHandle:%x, BASS Found End.\r\n", connHandle);
    }
    else if(sdpFound->svcId == AUDIO_VCP_CLIENT) {
        pConn->VCS_server = 1;
        app_parse_printf("ConnHandle:%x, VCP Volume Controller Found End.\r\n", connHandle);
    }

    tlkapi_printf(APP_LOG_EN, "sdp Found End, connHandle:0x%x ID:0x%x startHandle:0x%x endHandle:0x%x\n",
            sdpFound->aclHandle, sdpFound->svcId, sdpFound->startHdl, sdpFound->endHdl);

    return 0;
}

/**
 * @brief       BAP Broadcast Assistant found sink device event.
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   pData: Data carried by the event.
 * @param[in]   dataLen: data length.
 * @return      0.
 */
static int app_bapba_foundSink(u16 connHandle, u8 *pData, u16 dataLen)
{
    if(sinkFlag)    return 0;
    app_parse_printf("app_bapba_foundSink, flag = %d", sinkFlag);
    blc_bapba_foundSinkEvt_t* foundSink = (blc_bapba_foundSinkEvt_t*)pData;
    u8 index = app_audio_findSinkInfo(foundSink->addrType, foundSink->address, (char*)foundSink->completeName);
    if(index)
    {
        app_parse_printf("[%d] %s %s name:%s\r\n", index, foundSink->addrType?"random":"public", addr_to_str(foundSink->address),
                foundSink->completeName
        );
    }
    return 0;
}

/**
 * @brief       BAP broadcast Assistant found source extend advertise and want to started sync PA event.
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   pData: Data carried by the event.
 * @param[in]   dataLen: data length.
 * @return      0.
 */
static int app_bapba_startedSyncPA(u16 connHandle, u8 *pData, u16 dataLen)
{
    blc_bapba_startSyncPaEvt_t *startSyncPA = (blc_bapba_startSyncPaEvt_t*)pData;

    if(app_audio_findSourceInfo(startSyncPA->sid, startSyncPA->addrType, startSyncPA->address))
        return 1;

    tlkapi_printf(APP_LOG_EN, "start sync pda, broadcast is %s\n", hex_to_str(startSyncPA->broadcastId, 3));
    foundInfoTemp.head.sid = startSyncPA->sid;
    foundInfoTemp.head.addrType = startSyncPA->addrType;
    memcpy(foundInfoTemp.head.addr, startSyncPA->address, 6);
    memcpy(foundInfoTemp.head.broadcastId, startSyncPA->broadcastId, 3);
    app_audio_setSourceInfo(&foundInfoTemp.head);

    foundInfoTemp.completeNameLen = min(startSyncPA->completeNameLen, sizeof(foundInfoTemp.completeName));
    strncpy(foundInfoTemp.completeName, (const char *)startSyncPA->completeName, sizeof(foundInfoTemp.completeName));
    foundInfoTemp.broadcastNameLen = min(startSyncPA->broadcastNameLen, sizeof(foundInfoTemp.broadcastName));
    strncpy(foundInfoTemp.broadcastName, (const char *)startSyncPA->broadcastName, sizeof(foundInfoTemp.broadcastName));

    return 0;
}

/**
 * @brief       BAP broadcast Assistant found source information event.
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   pData: Data carried by the event.
 * @param[in]   dataLen: data length.
 * @return      0.
 */
static int app_bapba_foundSourceInfo(u16 connHandle, u8 *pData, u16 dataLen)
{
    blc_bapba_foundSourceInfoEvt_t *sourceInfo = (blc_bapba_foundSourceInfoEvt_t*)pData;
    if(sourceInfo->presentationDelay != 0xFFFFFFFF){
        tlkapi_printf(APP_LOG_EN, "[APP][EVT]Found Source Info, address is %s presentation Delay is %dus\n",
                    addr_to_str(sourceInfo->address), sourceInfo->presentationDelay);
    }
    bisSyncInfo_t* bisInfo = &sourceInfo->bisInfo[0];
    tlkapi_printf(APP_LOG_EN, " bisIndex:0x%x codecID:0x%x companyID:0x%x vendorID:0x%x\n",
            sourceInfo->bisIndex, bisInfo->CodecId.id, bisInfo->CodecId.companyID, bisInfo->CodecId.vendorID);
    tlkapi_printf(APP_LOG_EN, " CodecCfg SampleFreq:0x%x Duration:0x%x allocation:0x%x frameOctet:0x%x\n",
            bisInfo->codecCfg.frequency, bisInfo->codecCfg.duration,
            bisInfo->codecCfg.allocation, bisInfo->codecCfg.frameOcts);
    tlkapi_printf(APP_LOG_EN, " metadata is %s\n", hex_to_str(&bisInfo->metadata[1], bisInfo->metadata[0]));

    source_info_t *source = app_audio_findSourceInfo(foundInfoTemp.head.sid, foundInfoTemp.head.addrType, foundInfoTemp.head.addr);
    source->usedFlag = 1;
    if(sourceInfo->presentationDelay != 0xFFFFFFFF){
        memcpy(source->completeName, foundInfoTemp.completeName, sizeof(foundInfoTemp.completeName));
        memcpy(source->broadcastName, foundInfoTemp.broadcastName, sizeof(foundInfoTemp.broadcastName));
        source->bisCnt = 1;
        source->bisIndex[0] = sourceInfo->bisIndex;
        memcpy(&source->bisInfo[0], sourceInfo->bisInfo, sizeof(bisSyncInfo_t));
    }
    else
    {
        source->bisIndex[source->bisCnt] = sourceInfo->bisIndex;
        memcpy(&source->bisInfo[source->bisCnt], &sourceInfo->bisInfo, sizeof(bisSyncInfo_t));
        source->bisCnt ++;
    }

    return 0;
}

/**
 * @brief       BAP broadcast Assistant receive source encrypt state event.
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   pData: Data carried by the event.
 * @param[in]   dataLen: data length.
 * @return      0.
 */
static int app_bapba_sourceEncState(u16 connHandle, u8 *pData, u16 dataLen)
{
    blc_bapba_sourceEncStateEvt_t *sourceEnc = (blc_bapba_sourceEncStateEvt_t*)pData;

    source_info_t *source = app_audio_findSourceInfo(foundInfoTemp.head.sid, foundInfoTemp.head.addrType, foundInfoTemp.head.addr);
    if(source->usedFlag)
    {
        source->enc = sourceEnc->enc;
        app_audio_displaySourceInfo(source);
    }
    return 0;
}

/**
 * @brief       BAP broadcast Assistant receive source encrypt state event.
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   pData: Data carried by the event.
 * @param[in]   dataLen: data length.
 * @return      0.
 */
static int app_bapba_PastStartedReady(u16 connHandle, u8 *pData, u16 dataLen)
{
    connect_info_t* pConn = app_audio_getConn(connHandle);
    source_info_t* sourceInfo = app_audio_getSourceInfo(pConn->sourceIndex);

    if(pConn->sinkState == SINK_STATE_NO_SOURCE) {
        blc_bapba_writeAddSourcePast(pConn->connHandle, (blc_audio_source_head_t*)sourceInfo, pConn->bisSync);
    }
    return 0;
}

/**
 * @brief       bassc receive sink broadcast state event.
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   pData: Data carried by the event.
 * @param[in]   dataLen: data length.
 * @return      0.
 */
static int app_bassc_recvSinkState(u16 connHandle, u8 *pData, u16 dataLen)
{
    connect_info_t* pConn = app_audio_getConn(connHandle);
    blc_bassc_recvSinkStateEvt_t* sinkStateEvt = (blc_bassc_recvSinkStateEvt_t*)pData;
    source_info_t* sourceInfo = app_audio_getSourceInfo(pConn->sourceIndex);

    if(!dataLen){
        pConn->sinkState = SINK_STATE_NO_SOURCE;

        app_parse_printf("Sink no any Sync Source Info\r\n");
        if(pConn->sourceIndex)
        {
            if(pConn->pastFlag) {
                blc_bapba_writeAddSourcePast(connHandle, (blc_audio_source_head_t *)sourceInfo, pConn->bisSync);
            }
            else {
                blc_bapba_writeAddSourceNoPast(connHandle, (blc_audio_source_head_t *)sourceInfo, pConn->bisSync);
            }

            pConn->sourceIndex = 0;
        }
    }
    else
    {
        pConn->remoteSourceId = sinkStateEvt->sourceID;
        app_parse_printf("sink PA state is %s\r\nBIS Sync state is 0x%08x\r\n", sinkStateEvt->paState?"Sync":"Loss", sinkStateEvt->bisSyncState);

        if(sinkStateEvt->paState)
        {
            blc_bapba_stopPAST(connHandle);
        }

        if(sinkStateEvt->bisSyncState && sinkStateEvt->paState) {
            pConn->sinkState = SINK_STATE_HAD_SOURCE;
        }

        if(sinkStateEvt->bisSyncState == 0 && sinkStateEvt->paState == 0){
            pConn->sinkState = SINK_STATE_REMOVE_SOURCE;
            blc_bapba_writeRemoveSource(connHandle, pConn->remoteSourceId);
            pConn->sinkState = SINK_STATE_NO_SOURCE;
        }

    }

    tlkapi_printf(APP_LOG_EN, "[APP]receiver sink state is %s\n", hex_to_str(pData, dataLen));

    return 0;
}

/**
 * @brief       bassc receive broadcast code request event.
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   pData: Data carried by the event.
 * @param[in]   dataLen: data length.
 * @return      0.
 */
static int app_bassc_bcstCodeReq(u16 connHandle, u8 *pData, u16 dataLen)
{
    connect_info_t* pConn = app_audio_getConn(connHandle);

    blc_bapba_writeSetBroadcastCode(connHandle, pConn->remoteSourceId, (u8*)pConn->broadcastCode);

    return 0;
}

/**
 * @brief       bassc receive bad broadcast code event.
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   pData: Data carried by the event.
 * @param[in]   dataLen: data length.
 * @return      0.
 */
static int app_bassc_badBcstCode(u16 connHandle, u8 *pData, u16 dataLen)
{
    connect_info_t* pConn = app_audio_getConn(connHandle);

    blc_bassc_badBroadcastCodeEvt_t* badBcastCodeEvt = (blc_bassc_badBroadcastCodeEvt_t*)pData;

    if(badBcastCodeEvt->paState)
    {
        pConn->sourceIndex = 0;
        blc_bapba_writeModifySourceNotSyncPA(connHandle, pConn->remoteSourceId, 0);
        pConn->sinkState = SINK_STATE_BAD_CODE;
        app_parse_printf("Conn:%x, Sync BIS Failed, Error Broadcast Code is %.*s\r\n", connHandle, 16, badBcastCodeEvt->broadcastCode);
    }
    else
    {
        pConn->sinkState = SINK_STATE_REMOVE_SOURCE;
        blc_bapba_writeRemoveSource(connHandle, pConn->remoteSourceId);
        tlkapi_printf(APP_LOG_EN, "[APP] badBcstCode SINK_STATE_REMOVE_SOURCE");
    }

    return 0;
}

/**
 * @brief       vcsc changed volume state event.
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   pData: Data carried by the event.
 * @param[in]   dataLen: data length.
 * @return      0.
 */
static int app_vcsc_changedVolState(u16 connHandle, u8 *pData, u16 dataLen)
{
    blc_vcsc_volumeStateChangeEvt_t *volState = (blc_vcsc_volumeStateChangeEvt_t*)pData;
    app_parse_printf("Conn:%x, volume(min:0, max:255) is %d, muteSate:%s\r\n", connHandle, volState->volumeSetting, volState->mute?"Mute":"Unmute");

    muteFlag = volState->mute;

    return 0;
}

/**
 * @brief       vocsc changed volume offset value event.
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   pData: Data carried by the event.
 * @param[in]   dataLen: data length.
 * @return      0.
 */
static int app_vocsc_changedVolOffset(u16 connHandle, u8 *pData, u16 dataLen)
{
    blc_vocsc_volumeOffsetStateChangeEvt_t *volOffset = (blc_vocsc_volumeOffsetStateChangeEvt_t*)pData;
    app_parse_printf("Conn:%x, vocs index is %d volume offset(min:-255, max:255) is %d,\r\n", connHandle, volOffset->vocsIndex, volOffset->volumeOffset);

    return 0;
}

/************************************** BIS Assistant End ****************************************/

#endif
