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
#include "app_buffer.h"
#include "application/usbstd/usb.h"
#include "vendor/common/tlk_api/tlk_buffer.h"

#if (PRODUCT_CIS_SOURCE_SELECT == PRODUCT_KMLEA_DONGLE)

app_ctrl_t appCtrl;

static int  app_audio_prfEvtCb(u16 connHandle, int evtID, u8 *pData, u16 dataLen);
static void app_event_acl_connect(blc_prf_aclConnEvt_t *pEvt);
static void app_event_cis_connect(u16 aclHandle,blc_audio_cisConnEvt_t *pEvt);
static void app_event_cis_disconnect(u16 aclHandle,blc_audio_cisDisconnEvt_t *pEvt);
static void app_event_acl_disconnect(blc_prf_aclDisconnEvt_t *pEvt);
static void app_event_audio_sdp_over(blc_prf_sdpOverEvt_t *pEvt);
static void app_event_codec_configured(blc_bapuc_codecConfiguredEvt_t *pEvt);
static void app_event_set_cig_param(hci_le_setCigParam_cmdParam_t *pEvt);
static void app_event_qos_configured(blc_bapuc_qosConfiguredEvt_t *pEvt);
static void app_event_sink_stream_start(blc_audio_streamingEvt_t *pEvt);
static void app_event_source_stream_start(blc_audio_streamingEvt_t *pEvt);

void app_audio_init(void)
{
#if (!PEER_DEVICE_USE_CIS_PERIPHERAL_TEST)
    blc_audio_setAclCentarlIndexforCIS(ACLCEN_IDX_CIS, INVALID_ACL_IDX, INVALID_ACL_IDX, INVALID_ACL_IDX);

    /* Audio profile event register */
    blc_audio_initialModule(app_audio_prfEvtCb);
#endif
    /* Audio CAP initiator init */
    blc_cap_initUnicastInitiator();

#if (ACL_PERIPHR_SMP_ENABLE || ACL_CENTRAL_SMP_ENABLE)
    /* Audio bonding initial */
    blc_prf_initPairingInfoStoreModule();
#endif

    app_codec_init();
}
void app_audio_handler(void)
{
    blc_prf_main_loop();
    app_codec_handler();
}
static int app_audio_prfEvtCb(u16 connHandle, int evtID, u8 *pData, u16 dataLen)
{
    switch(evtID){
    case PRF_EVTID_ACL_CONNECT:
    {
        tlkapi_send_string_data(USER_DUMP_EN, "app event-acl connect", 0, 0);
        app_event_acl_connect((blc_prf_aclConnEvt_t*)pData);
    }
    break;

    case PRF_EVTID_ACL_DISCONNECT:
    {
        tlkapi_send_string_data(USER_DUMP_EN, "app event-acl disconnect", 0, 0);
        app_event_acl_disconnect((blc_prf_aclDisconnEvt_t*)pData);
    }
    break;

    case AUDIO_EVT_CIS_CONNECT:
    {
        tlkapi_send_string_data(USER_DUMP_EN, "app event-cis connect", 0, 0);
        app_event_cis_connect(connHandle,(blc_audio_cisConnEvt_t *)pData);
    }
    break;

    case AUDIO_EVT_CIS_DISCONNECT:
    {
        tlkapi_send_string_data(USER_DUMP_EN, "app event-cis disconnect", 0, 0);
        app_event_cis_disconnect(connHandle,(blc_audio_cisDisconnEvt_t *)pData);
    }
    break;

    case AUDIO_EVT_BAPUC_CODEC_CONFIGURED:
    {
        tlkapi_send_string_data(USER_DUMP_EN, "app event-codec configured", 0, 0);
        app_event_codec_configured((blc_bapuc_codecConfiguredEvt_t *)pData);
    }
    break;

    case AUDIO_EVT_BAPUC_SET_CIG_PARAMS:
    {
        tlkapi_send_string_data(USER_DUMP_EN, "app event-set cig param", 0, 0);
        app_event_set_cig_param();
    }
    break;

    case AUDIO_EVT_BAPUC_QOS_CONFIGURED:
    {
        tlkapi_send_string_data(USER_DUMP_EN, "app event-qos configured", 0, 0);
        app_event_qos_configured((blc_bapuc_qosConfiguredEvt_t *)pData);
    }
    break;

    case AUDIO_EVT_BAPUC_RECEIVE_STREAMING: /* Client as Audio Sink, start to receive audio */
    {
        tlkapi_send_string_data(USER_DUMP_EN, "app event-sink stream start", 0, 0);
        app_event_sink_stream_start((blc_audio_streamingEvt_t *)pData);
    }
    break;

    case AUDIO_EVT_BAPUC_SEND_STREAMING: /* Client as Audio Source, start to send audio */
    {
        tlkapi_send_string_data(USER_DUMP_EN, "app event-source stream start", 0, 0);
        app_event_source_stream_start((blc_audio_streamingEvt_t *)pData);
    }
    break;

    case PRF_EVTID_CLIENT_ALL_SDP_OVER:
    {
        tlkapi_send_string_data(USER_DUMP_EN, "app event-audio sdp over", 0, 0);
        app_event_audio_sdp_over((blc_prf_sdpOverEvt_t *)pData);
    }
    break;

    default:
        tlkapi_send_string_data(USER_DUMP_EN, "unprocessed audio event", &evtID, 4);
        break;
    }
    return 0;
}

static void app_event_acl_connect(blc_prf_aclConnEvt_t *pEvt)
{
    appCtrl.aclParam.acl_handle = pEvt->aclHandle;
    appCtrl.acl_count++;
}
static void app_event_cis_connect(u16 aclHandle,blc_audio_cisConnEvt_t *pEvt)
{
    tlkapi_send_string_data(USER_DUMP_EN, "nse", &pEvt->nse, 1);
    tlkapi_send_string_data(USER_DUMP_EN, "ft m2s", &pEvt->ft_m2s, 1);
    tlkapi_send_string_data(USER_DUMP_EN, "ft s2m", &pEvt->ft_s2m, 1);
}
static void app_event_cis_disconnect(u16 aclHandle,blc_audio_cisDisconnEvt_t *pEvt)
{
    appCtrl.aclParam.sink.sS = false;
    appCtrl.aclParam.source.sS = false;
    tlkapi_send_string_data(USER_DUMP_EN, "reason", &pEvt->reason, 1);
}
static void app_event_acl_disconnect(blc_prf_aclDisconnEvt_t *pEvt)
{
    appCtrl.acl_count--;
    memset((u8*)&appCtrl.aclParam,0,sizeof(app_acl_param_t));
}
static void app_event_audio_sdp_over(blc_prf_sdpOverEvt_t *pEvt)
{

    blc_audio_ase_cfg_info_t audChnInfo;
    int audioRet = blc_bapuc_checkAudioConfigures(pEvt->aclHandle, APP_AUDIO_CONFIGURATION_PREFER, &audChnInfo);
    if(audioRet!= AUDIO_ESUCC)
    {
        BLT_APP_LOG("error-audio configurations:0x%x", audioRet);
        return;
    }

    ////////////////////// SVR: SINK; CLT: SOURCE //////////////////////////////////////
    for(int i = 0; i<audChnInfo.sinkASEsPerSvr; i++)
    {
        audioRet = blc_bapuc_setAseConfigCodec(pEvt->aclHandle,audChnInfo.sinkASEId[i], APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER, &audChnInfo);
        if(audioRet!= AUDIO_ESUCC)
        {
            BLT_APP_LOG("error-unicast config audio:0x%x", audioRet);
        }
        else
        {
            appCtrl.aclParam.source.codecParam = APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER;
            appCtrl.aclParam.source.blocks     = audChnInfo.sinkCodecFrameBlksPerSDU;
            appCtrl.aclParam.source.codecOp    = APP_CONFIG_CODEC;
            BLT_APP_LOG("source config codec-blocks %d",appCtrl.aclParam.source.blocks);
        }
    }
    ////////////////////// SVR: SOURCE;  CLT: SINK //////////////////////////////////////
    for(int i = 0; i<audChnInfo.srcASEsPerSvr; i++)
    {
        audioRet = blc_bapuc_setAseConfigCodec(pEvt->aclHandle, audChnInfo.srcASEId[i], APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER, &audChnInfo);
        if(audioRet!= AUDIO_ESUCC)
        {
            BLT_APP_LOG("error-unicast config audio:0x%x", audioRet);
        }
        else
        {
            appCtrl.aclParam.sink.codecParam = APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER;
            appCtrl.aclParam.sink.blocks     = audChnInfo.srcCodecFrameBlksPerSDU;
            appCtrl.aclParam.sink.codecOp    = APP_CONFIG_CODEC;
            BLT_APP_LOG("sink config codec-blocks %d",appCtrl.aclParam.sink.blocks);
        }
    }
}
static void app_event_codec_configured(blc_bapuc_codecConfiguredEvt_t *pEvt)
{
    if(pEvt->aseDir == AUDIO_DIR_SOURCE)
    {
        BLT_APP_LOG("source max latency %d",pEvt->maxTransportLatency);
        blc_bapuc_setAseConfigQos(pEvt->aclHandle, pEvt->aseID, APP_AUDIO_QOS_OUTPUT_PARAMETER_PREFER);
    }
    else
    {
        BLT_APP_LOG("sink max latency %d",pEvt->maxTransportLatency);
        blc_bapuc_setAseConfigQos(pEvt->aclHandle, pEvt->aseID, APP_AUDIO_QOS_OUTPUT_PARAMETER_PREFER);
    }
}
static void app_event_qos_configured(blc_bapuc_qosConfiguredEvt_t*pEvt)
{
    if(pEvt->aseDir == AUDIO_DIR_SOURCE)
    {
        appCtrl.aclParam.sink.pD = pEvt->presentationDelay;
        BLT_APP_LOG("sink presentation delay:0x%x",  appCtrl.aclParam.sink.pD);
    }
    else
    {
        BLT_APP_LOG("source presentation delay:0x%x", pEvt->presentationDelay);
    }
}
static void app_event_set_cig_param(void)
{
    u8 cig_ret_buffer[3 + APP_CIS_CENTRAL_NUMBER * 1];
    hci_le_setCigParam_retParam_t *pCigRetParam = (hci_le_setCigParam_retParam_t*)cig_ret_buffer;

    u8 cig_cmd_buffer[15 + APP_CIS_CENTRAL_NUMBER * sizeof(cigParamTest_cisCfg_t)];
    hci_le_setCigParamTest_cmdParam_t* pCigCmdParam = (hci_le_setCigParamTest_cmdParam_t*)cig_cmd_buffer;

    pCigCmdParam->cig_id = CIG_ID_0;
    u32 sdu_interval_m2s = 10000;
    u32 sdu_interval_s2m = 10000;
    memcpy(pCigCmdParam->sdu_int_m2s, &sdu_interval_m2s, 3);
    memcpy(pCigCmdParam->sdu_int_s2m, &sdu_interval_s2m, 3);
    pCigCmdParam->ft_m2s = 5;
    pCigCmdParam->ft_s2m = 5;
    pCigCmdParam->iso_intvl = ISO_INTERVAL_10MS;
    pCigCmdParam->sca = PPM_251_500;
    pCigCmdParam->packing = PACK_INTERLEAVED;
    pCigCmdParam->framing = CIS_UNFRAMED;
    pCigCmdParam->cis_count = APP_CIS_CENTRAL_NUMBER;

    for(int i = 0; i<pCigCmdParam->cis_count; i++)
    {
        pCigCmdParam->cisCfg[i].cis_id = i;
        pCigCmdParam->cisCfg[i].nse = 4;
        pCigCmdParam->cisCfg[i].max_sdu_m2s = 200;
        pCigCmdParam->cisCfg[i].max_sdu_s2m = 40;
        pCigCmdParam->cisCfg[i].max_pdu_m2s = 200;
        pCigCmdParam->cisCfg[i].max_pdu_s2m = 40;
        pCigCmdParam->cisCfg[i].phy_m2s = PHY_PREFER_2M;
        pCigCmdParam->cisCfg[i].phy_s2m = PHY_PREFER_2M;
        pCigCmdParam->cisCfg[i].bn_m2s = 1;
        pCigCmdParam->cisCfg[i].bn_s2m = 1;
    }

    u8 status = blc_hci_le_setCigParamsTest(pCigCmdParam, pCigRetParam);
    if(status == BLE_SUCCESS)
    {
        tlkapi_send_string_data(APP_LOG_EN, "Set CIG Param Success", 0, 0);
        extern  blt_ascsc_cisBondingParam_t cisBondingParam[];
        cisBondingParam[0].cis_count = 1;
        cisBondingParam[0].cis_handle[0] = pCigRetParam->cis_connHandle[0];
    }
    else
    {
        tlkapi_send_string_data(APP_LOG_EN, "Set CIG Param Fail", &status, 1);
    }
}

static void app_event_sink_stream_start(blc_audio_streamingEvt_t *pEvt)
{
    tlk_buffer_clear(TLK_BUFFER_2);
    appCtrl.aclParam.sink.sS = true;
}
static void app_event_source_stream_start(blc_audio_streamingEvt_t *pEvt)
{
    tlk_buffer_clear(TLK_BUFFER_1);
    appCtrl.aclParam.source.sS = true;
}


#endif //end of (PRODUCT_CIS_SOURCE_SELECT == ...)
