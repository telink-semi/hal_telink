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
#include "vendor/common/tlk_api/tlk_codec.h"
#if (UNICAST_CLIENT_SELECT == UNICAST_CLIENT_CODEC)

/**
 *  @brief  app control block parameter
 */
app_ctrl_t appCtrl;

static int  app_audio_prfEvtCb(u16 aclHandle, int evtID, u8 *pData, u16 dataLen);
static void app_event_acl_connect(u16 aclHandle, blc_prf_aclConnEvt_t *pEvt);
static void app_event_cis_connect(u16 aclHandle, blc_audio_cisConnEvt_t *pEvt);
static void app_event_cis_disconnect(u16 aclHandle, blc_audio_cisDisconnEvt_t *pEvt);
static void app_event_acl_disconnect(u16 aclHandle, blc_prf_aclDisconnEvt_t *pEvt);
static void app_event_audio_sdp_over(u16 aclHandle, blc_prf_sdpOverEvt_t *pEvt);
static void app_event_audio_sdp_found(u16 aclHandle, blc_prf_sdpFoundEvt_t *pEvt);
static void app_event_audio_sdp_not_found(u16 aclHandle, blc_prf_sdpFailEvt_t *pEvt);
static void app_event_codec_configured(u16 aclHandle, blc_bapuc_codecConfiguredEvt_t *pEvt);
static void app_event_qos_configured(u16 aclHandle, blc_bapuc_qosConfiguredEvt_t *pEvt);
static void app_event_enabling(u16 aclHandle, blc_bapuc_enablingEvt_t *pEvt);
static void app_event_disabling(u16 aclHandle, blc_bapuc_disablingEvt_t *pEvt);
static void app_event_releasing(u16 aclHandle, blc_bapuc_releasingEvt_t *pEvt);
static void app_event_receive_streaming(u16 aclHandle, blc_audio_streamingEvt_t *pEvt);
static void app_event_send_streaming(u16 aclHandle, blc_audio_streamingEvt_t *pEvt);
static s8   app_audio_getHandleIndex(u16 connHandle);

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

    #if (ACL_PERIPHR_SMP_ENABLE || ACL_CENTRAL_SMP_ENABLE)
    /* Audio bonding initial */
    blc_prf_initPairingInfoStoreModule();
    #endif

    appCtrl.acl_max_num = ACL_CENTRAL_MAX_NUM;

    /* Codec init */
    app_codec_init();

    #if APP_AUDIO_ASRC_EN
    /* ASRC init */
    my_asrc_init_stereo(0);
    tlkapi_printf(APP_LOG_EN, "Init_ASRC");
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
    {PRF_EVTID_ACL_CONNECT,             (void *)app_event_acl_connect        },
    {PRF_EVTID_ACL_DISCONNECT,          (void *)app_event_acl_disconnect     },
    {AUDIO_EVT_CIS_CONNECT,             (void *)app_event_cis_connect        },
    {AUDIO_EVT_CIS_DISCONNECT,          (void *)app_event_cis_disconnect     },
    /* Event for Client SDP */
    {PRF_EVTID_CLIENT_SDP_FOUND,        (void *)app_event_audio_sdp_found    },
    {PRF_EVTID_CLIENT_SDP_FAIL,         (void *)app_event_audio_sdp_not_found},
    {PRF_EVTID_CLIENT_ALL_SDP_OVER,     (void *)app_event_audio_sdp_over     },
    /* Event for BAP Unicast Client */
    {AUDIO_EVT_BAPUC_CODEC_CONFIGURED,  (void *)app_event_codec_configured   },
    {AUDIO_EVT_BAPUC_QOS_CONFIGURED,    (void *)app_event_qos_configured     },
    {AUDIO_EVT_BAPUC_ENABLING,          (void *)app_event_enabling           },
    {AUDIO_EVT_BAPUC_DISABLING,         (void *)app_event_disabling          },
    {AUDIO_EVT_BAPUC_RELEASING,         (void *)app_event_releasing          },
    {AUDIO_EVT_BAPUC_RECEIVE_STREAMING, (void *)app_event_receive_streaming  }, //Client as Sink, receive audio
    {AUDIO_EVT_BAPUC_SEND_STREAMING,    (void *)app_event_send_streaming     }, //Client as Source, send audio
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
    for (int i = 0; i < ARRAY_SIZE(unicastCb); i++) {
        if (unicastCb[i].id == evtID) {
            unicastCb[i].evtCb(aclHandle, (void *)pData);
        }
    }
    return 0;
}

static s8 app_audio_getHandleIndex(u16 connHandle)
{
    tlkapi_printf(APP_LOG_EN, "acl_max_num:0x%x\n", appCtrl.acl_max_num);
    tlkapi_printf(APP_LOG_EN, "acl_handle[0]:0x%x\n", appCtrl.aclParam[0].acl_handle);
    tlkapi_printf(APP_LOG_EN, "acl_handle[1]:0x%x\n", appCtrl.aclParam[1].acl_handle);
    for (int i = 0; i < appCtrl.acl_max_num; i++) {
        if (appCtrl.aclParam[i].acl_handle == connHandle) {
            return i;
        }
    }
    return -1;
}

static void app_event_acl_connect(u16 aclHandle, blc_prf_aclConnEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN, "app event-acl connect\n");

    for (int i = 0; i < appCtrl.acl_max_num; i++) {
        if (appCtrl.aclParam[i].acl_handle == 0) {
            appCtrl.aclParam[i].acl_handle = pEvt->aclHandle;
            tlkapi_printf(APP_LOG_EN, "acl handle match-index:0x%x\n", i);
            tlkapi_printf(APP_LOG_EN, "acl handle match-handle:0x%x\n", appCtrl.aclParam[i].acl_handle);
            break;
        }
    }
    appCtrl.acl_count++;
    if (appCtrl.acl_count >= appCtrl.acl_max_num) {
        blc_ll_setExtScanEnable(BLC_SCAN_DISABLE, DUP_FILTER_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
        tlkapi_printf(APP_LOG_EN, "acl connect cnt-stop scan:0x%x\n", appCtrl.acl_count);
    } else {
        blc_ll_setExtScanEnable(BLC_SCAN_ENABLE, DUP_FILTER_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
        tlkapi_printf(APP_LOG_EN, "acl connect cnt-start scan:0x%x\n", appCtrl.acl_count);
    }
}

static void app_event_cis_connect(u16 aclHandle, blc_audio_cisConnEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN, "app event-cis connect\n");

    s8 acl_index = app_audio_getHandleIndex(aclHandle);
    if (acl_index < 0) {
        tlkapi_printf(APP_LOG_EN, "error-get acl handle:0x%x\n", aclHandle);
    }
    tlkapi_printf(APP_LOG_EN, "nse:0x%x\n", pEvt->nse);
    tlkapi_printf(APP_LOG_EN, "ft m2s:0x%x\n", pEvt->ft_m2s);
    tlkapi_printf(APP_LOG_EN, "ft s2m:0x%x\n", pEvt->ft_s2m);
}

static void app_event_cis_disconnect(u16 aclHandle, blc_audio_cisDisconnEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN, "app event-cis disconnect %d\n", aclHandle);

    s8 acl_index = app_audio_getHandleIndex(aclHandle);
    if (acl_index < 0) {
        tlkapi_printf(APP_LOG_EN, "error-get acl handle:0x%x\n", aclHandle);
        return;
    }
    appCtrl.aclParam[acl_index].sink.sS   = false;
    appCtrl.aclParam[acl_index].source.sS = false;
    tlkapi_printf(APP_LOG_EN, "reason:0x%x\n", pEvt->reason);

    if (blc_ll_disconnect(aclHandle, HCI_ERR_REMOTE_USER_TERM_CONN) == BLE_SUCCESS) {
        tlkapi_printf(APP_LOG_EN, "cis disconnect,disconnect acl actively\n");
    }
}

static void app_event_acl_disconnect(u16 aclHandle, blc_prf_aclDisconnEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN, "app event-acl disconnect\n");

    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if (acl_index < 0) {
        tlkapi_printf(APP_LOG_EN, "error-get acl handle:0x%x\n", pEvt->aclHandle);
        return;
    }
    memset((u8 *)&appCtrl.aclParam[acl_index].acl_handle, 0, sizeof(app_acl_param_t));
    appCtrl.acl_count--;
    if (appCtrl.acl_count == 0) {
        appCtrl.acl_csis_exist = 0;
        appCtrl.acl_csis_size  = 0;
        memset(appCtrl.acl_csis_sirk, 0, 16);
        tlk_codec_stop(TLK_CODEC_INPUT);
        tlk_codec_stop(TLK_CODEC_OUTPUT);
    }
    int scan_ret = blc_ll_setExtScanEnable(BLC_SCAN_ENABLE, DUP_FILTER_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
    if (scan_ret != BLE_SUCCESS) {
        tlkapi_printf(APP_LOG_EN, "start scan error:0x%x\n", scan_ret);
    } else {
        tlkapi_printf(APP_LOG_EN, "start scan success:0x%x\n", scan_ret);
    }
}

static void app_event_audio_sdp_found(u16 aclHandle, blc_prf_sdpFoundEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN, "app event-audio sdp found\n");

    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if (acl_index < 0) {
        tlkapi_printf(APP_LOG_EN, "error-get acl handle:0x%x\n", pEvt->aclHandle);
        return;
    }

    if (pEvt->svcId == AUDIO_CSIS_CLIENT) {
        if (appCtrl.acl_count == 1) {
            appCtrl.acl_csis_exist = 1;
            appCtrl.acl_csis_size  = 0; //clear. update latter by clientSDP
            tlkapi_printf(APP_LOG_EN, "acceptor with CSIS\n");
        }
    }
}

static void app_event_audio_sdp_not_found(u16 aclHandle, blc_prf_sdpFailEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN, "app event-audio sdp not found\n");

    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if (acl_index < 0) {
        tlkapi_printf(APP_LOG_EN, "error-get acl handle:0x%x\n", pEvt->aclHandle);
        return;
    }

    if (pEvt->svcId == AUDIO_CSIS_CLIENT) {
        if (appCtrl.acl_count == 1) {
            appCtrl.acl_csis_exist = 0;
            appCtrl.acl_csis_size  = 1;
            tlkapi_printf(APP_LOG_EN, "acceptor without CSIS, client support 1 acceptor");
        }
    }
}

static void app_event_audio_sdp_over(u16 aclHandle, blc_prf_sdpOverEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN, "app event-audio sdp over\n");

    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if (acl_index < 0) {
        tlkapi_printf(APP_LOG_EN, "error-get acl handle:0x%x\n", pEvt->aclHandle);
        return;
    }

    if (appCtrl.acl_count == 1 && appCtrl.acl_csis_exist) {
        int r = blc_csisc_getSetIdentityResolvingKey(pEvt->aclHandle, appCtrl.acl_csis_sirk);
        if (AUDIO_ESUCC != r) {
            tlkapi_printf(APP_LOG_EN, "error-get SIRK:0x%x\n", r);
            return;
        } else {
            tlkapi_printf(APP_LOG_EN, "CSISC: get SIRK:%s\n", hex_to_str(appCtrl.acl_csis_sirk, 16));
        }
        r = blc_csisc_getCoordinatedSetSize(pEvt->aclHandle, &appCtrl.acl_csis_size);
        if (AUDIO_ESUCC != r) {
            tlkapi_printf(APP_LOG_EN, "error-get SetSize:0x%x\n", r);
            return;
        } else {
            tlkapi_printf(APP_LOG_EN, "CSISC: SetSize:0x%x\n", appCtrl.acl_csis_size);
        }
    }

    blc_audio_ase_cfg_info_t audChnInfo;
    int                      audioRet = blc_bapuc_checkAudioConfigures(pEvt->aclHandle, APP_AUDIO_CONFIGURATION_PREFER, &audChnInfo);
    if (audioRet != AUDIO_ESUCC) {
        tlkapi_printf(APP_LOG_EN, "error-audio configurations:0x%x\n", audioRet);
        return;
    }

    ////////////////////// SVR: SINK; CLT: SOURCE //////////////////////////////////////
    for (int i = 0; i < audChnInfo.sinkASEsPerSvr; i++) {
        audioRet = blc_bapuc_setAseConfigCodec(pEvt->aclHandle, audChnInfo.sinkASEId[i], APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER, &audChnInfo);
        if (audioRet != AUDIO_ESUCC) {
            tlkapi_printf(APP_LOG_EN, "error-unicast config audio:0x%x\n", audioRet);
        } else {
            appCtrl.aclParam[acl_index].source.codecParam = APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER;
            appCtrl.aclParam[acl_index].source.blocks     = audChnInfo.sinkCodecFrameBlksPerSDU;
            appCtrl.aclParam[acl_index].source.codecOp    = APP_CONFIG_CODEC;
            tlkapi_printf(APP_LOG_EN, "source config codec-blocks %d\n", appCtrl.aclParam[acl_index].source.blocks);
        }
    }

    ////////////////////// SVR: SOURCE;  CLT: SINK //////////////////////////////////////
    for (int i = 0; i < audChnInfo.srcASEsPerSvr; i++) {
        audioRet = blc_bapuc_setAseConfigCodec(pEvt->aclHandle, audChnInfo.srcASEId[i], APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER, &audChnInfo);
        if (audioRet != AUDIO_ESUCC) {
            tlkapi_printf(APP_LOG_EN, "error-unicast config audio:0x%x\n", audioRet);
        } else {
            appCtrl.aclParam[acl_index].sink.codecParam = APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER;
            appCtrl.aclParam[acl_index].sink.blocks     = audChnInfo.srcCodecFrameBlksPerSDU;
            appCtrl.aclParam[acl_index].sink.codecOp    = APP_CONFIG_CODEC;
            tlkapi_printf(APP_LOG_EN, "sink config codec-blocks %d\n", appCtrl.aclParam[acl_index].sink.blocks);
        }
    }
}

static void app_event_codec_configured(u16 aclHandle, blc_bapuc_codecConfiguredEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN, "app event-codec configured\n");

    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if (acl_index < 0) {
        tlkapi_printf(APP_LOG_EN, "error-get acl handle:0x%x\n", pEvt->aclHandle);
        return;
    }
    if (pEvt->aseDir == AUDIO_DIR_SOURCE) {
        tlkapi_printf(APP_LOG_EN, "source max latency %d\n", pEvt->maxTransportLatency);
        blc_bapuc_setAseConfigQos(pEvt->aclHandle, pEvt->aseID, APP_AUDIO_QOS_OUTPUT_PARAMETER_PREFER);
    } else {
        tlkapi_printf(APP_LOG_EN, "sink max latency %d\n", pEvt->maxTransportLatency);
        blc_bapuc_setAseConfigQos(pEvt->aclHandle, pEvt->aseID, APP_AUDIO_QOS_OUTPUT_PARAMETER_PREFER);
    }
}

static void app_event_qos_configured(u16 aclHandle, blc_bapuc_qosConfiguredEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN, "app event-qos configured\n");

    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if (acl_index < 0) {
        tlkapi_printf(APP_LOG_EN, "error-get acl handle:0x%x\n", pEvt->aclHandle);
        return;
    }
    if (pEvt->aseDir == AUDIO_DIR_SOURCE) {
        appCtrl.aclParam[acl_index].sink.pD = pEvt->presentationDelay;
        tlkapi_printf(APP_LOG_EN, "sink presentation delay:0x%x\n", appCtrl.aclParam[acl_index].sink.pD);
    } else {
        tlkapi_printf(APP_LOG_EN, "source presentation delay:0x%x\n", pEvt->presentationDelay);
    }
}

static void app_event_receive_streaming(u16 aclHandle, blc_audio_streamingEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN, "app event-receive streaming\n");

    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if (acl_index < 0) {
        tlkapi_printf(APP_LOG_EN, "error-get acl handle:0x%x\n", pEvt->aclHandle);
        return;
    }
    tlk_codec_sts_e codecRet = tlk_codec_start(TLK_CODEC_OUTPUT);
    if (codecRet == TLK_CODEC_STATE_ERROR) {
        tlkapi_printf(APP_LOG_EN, "error codec state %d", codecRet);
    } else if (codecRet == TLK_CODEC_OPERATION_REPEAT) {
        tlkapi_printf(APP_LOG_EN, "codec already enable %d", codecRet);
    } else {
        tlkapi_printf(APP_LOG_EN, "codec-output enable success");
    }
    appCtrl.aclParam[acl_index].sink.sS = true;
}

static void app_event_send_streaming(u16 aclHandle, blc_audio_streamingEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN, "app event-send streaming\n");

    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if (acl_index < 0) {
        tlkapi_printf(APP_LOG_EN, "error-get acl handle:0x%x\n", pEvt->aclHandle);
        return;
    }
    tlk_codec_sts_e codecRet = tlk_codec_start(TLK_CODEC_INPUT);
    if (codecRet == TLK_CODEC_STATE_ERROR) {
        tlkapi_printf(APP_LOG_EN, "error codec state %d", codecRet);
    } else if (codecRet == TLK_CODEC_OPERATION_REPEAT) {
        tlkapi_printf(APP_LOG_EN, "codec already enable %d", codecRet);
    } else {
        tlkapi_printf(APP_LOG_EN, "codec-input enable success");
    }
    appCtrl.aclParam[acl_index].source.sS = true;
}

static void app_event_enabling(u16 aclHandle, blc_bapuc_enablingEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN, "app event-enabling\n");

    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if (acl_index < 0) {
        tlkapi_printf(APP_LOG_EN, "error-get acl handle:0x%x\n", pEvt->aclHandle);
        return;
    }
    if (pEvt->aseDir == AUDIO_DIR_SOURCE) //Client as audio sink, if ready to receive data,execute 'receive start ready' operation to inform Server
    {
        blc_bapuc_setAseReceiverStartReady(pEvt->aclHandle, pEvt->aseID);
    } else {
        //Client as audio source,if server ready to receive data,will execute 'receive start ready' automatically.
    }
}

static void app_event_disabling(u16 aclHandle, blc_bapuc_disablingEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN, "app event-disabling\n");

    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if (acl_index < 0) {
        tlkapi_printf(APP_LOG_EN, "error-get acl handle:0x%x\n", pEvt->aclHandle);
        return;
    }
    if (pEvt->aseDir == AUDIO_DIR_SOURCE) //Client as audio sink, if ready to stop receive data,execute 'receive stop ready' operation to inform Server
    {
        blc_bapuc_setAseReceiverStopReady(pEvt->aclHandle, pEvt->aseID);
    }
}

static void app_event_releasing(u16 aclHandle, blc_bapuc_releasingEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN, "app event-releasing\n");

    s8 acl_index = app_audio_getHandleIndex(pEvt->aclHandle);
    if (acl_index < 0) {
        tlkapi_printf(APP_LOG_EN, "error-get acl handle:0x%x\n", pEvt->aclHandle);
        return;
    }
}

#endif
