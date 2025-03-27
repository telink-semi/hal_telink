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

#include "app_config.h"
#include "app_audio.h"
#include "app_codec.h"
#include "app_audio_ctrl.h"
#include "vendor/common/tlk_api/tlk_tone.h"
#include "vendor/common/tlk_api/tlk_codec.h"
#if (UNICAST_SERVER_SELECT == UNICAST_SERVER_HEADSET)

/**
 *  @brief  app control block parameter
 */
app_audio_ctrl_t appCtrl;

static int  app_audio_prfEvtCb(u16 aclHandle, int evtID, u8 *pData, u16 dataLen);
static void app_event_acl_connect(u16 aclHandle, blc_prf_aclConnEvt_t *pEvt);
static void app_event_acl_disconnect(u16 aclHandle, blc_prf_aclDisconnEvt_t *pEvt);
static void app_event_cis_connect(u16 aclHandle, blc_audio_cisConnEvt_t *pEvt);
static void app_event_cis_disconnect(u16 aclHandle, blc_audio_cisDisconnEvt_t *pEvt);
static void app_event_cis_request(u16 aclHandle, blc_audio_cisReqEvt_t *pEvt);
static void app_event_security_done(u16 aclHandle, blc_prf_securityDoneEvt_t *pEvt);
static void app_event_audio_sdp_found(u16 aclHandle, blc_prf_sdpFoundEvt_t *pEvt);
static void app_event_audio_sdp_not_found(u16 aclHandle, blc_prf_sdpFailEvt_t *pEvt);
static void app_event_audio_sdp_over(u16 aclHandle, blc_prf_sdpOverEvt_t *pEvt);
static void app_ep_codec_configured(u16 aclHandle, blc_bapus_codecConfiguredEvt_t *pEvt);
static void app_ep_qos_configured(u16 aclHandle, blc_bapus_qosConfiguredEvt_t *pEvt);
static void app_ep_enabling(u16 aclHandle, blc_bapus_enablingEvt_t *pEvt);
static void app_ep_receive_streaming(u16 aclHandle, blc_bapus_receiveStreamingEvt_t *pEvt);
static void app_ep_send_streaming(u16 aclHandle, blc_bapus_sendStreamingEvt_t *pEvt);
static void app_ep_disabling(u16 aclHandle, blc_bapus_disablingEvt_t *pEvt);
static void app_ep_releasing(u16 aclHandle, blc_bapus_releasingEvt_t *pEvt);


/**
 * @brief      Audio module init.
 * @param[in]  none.
 * @return     none.
 */
void app_audio_init(void)
{
    /* Audio profile event register */
    blc_audio_initialModule(app_audio_prfEvtCb);

    /* Audio CAP acceptor init */
    app_audio_acceptor_init();

#if (ACL_PERIPHR_SMP_ENABLE || ACL_CENTRAL_SMP_ENABLE)
    /* Audio bonding initial */
    blc_prf_initPairingInfoStoreModule();
#endif
    /* Codec init */
    app_codec_init();

    tlkapi_printf(APP_LOG_EN,"unicast server init");
}

/**
 * @brief       Unicast Server register profile event callback.
 */
static const app_audio_evtCb_t unicastCb[] = {
    /* Event for controller or Host */
    {PRF_EVTID_ACL_CONNECT              , (void*)app_event_acl_connect},
    {PRF_EVTID_ACL_DISCONNECT           , (void*)app_event_acl_disconnect},
    {AUDIO_EVT_CIS_CONNECT              , (void*)app_event_cis_connect},
    {AUDIO_EVT_CIS_DISCONNECT           , (void*)app_event_cis_disconnect},
    {AUDIO_EVT_CIS_REQUEST              , (void*)app_event_cis_request},
    {PRF_EVTID_SMP_SECURITY_DONE        , (void*)app_event_security_done},
    /* Event for Client SDP */
    {PRF_EVTID_CLIENT_SDP_FOUND         , (void*)app_event_audio_sdp_found},
    {PRF_EVTID_CLIENT_SDP_FAIL          , (void*)app_event_audio_sdp_not_found},
    {PRF_EVTID_CLIENT_ALL_SDP_OVER      , (void*)app_event_audio_sdp_over},
    /* Event for BAP Unicast Server */
    {AUDIO_EVT_BAPUS_CODEC_CONFIGURED   , (void*)app_ep_codec_configured},
    {AUDIO_EVT_BAPUS_QOS_CONFIGURED     , (void*)app_ep_qos_configured},
    {AUDIO_EVT_BAPUS_ENABLING           , (void*)app_ep_enabling},
    {AUDIO_EVT_BAPUS_RECEIVE_STREAMING  , (void*)app_ep_receive_streaming},
    {AUDIO_EVT_BAPUS_SEND_STREAMING     , (void*)app_ep_send_streaming},
    {AUDIO_EVT_BAPUS_DISABLING          , (void*)app_ep_disabling},
    {AUDIO_EVT_BAPUS_RELEASING          , (void*)app_ep_releasing},
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
            unicastCb[i].evtCb(aclHandle, (void*)pData);
            return 0;
        }
    }

    /* audio call EVT */
    app_call_event_callback(aclHandle, evtID, pData, dataLen);
    /* audio media EVT */
    app_media_event_callback(aclHandle, evtID, pData, dataLen);

    return 0;
}

static void app_event_acl_connect(u16 aclHandle, blc_prf_aclConnEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN,"event-acl connect, handle: 0x%x",pEvt->aclHandle);
    ble_sts_t advRet = blc_ll_setExtAdvEnable(BLC_ADV_DISABLE, ADV_HANDLE0, 0, 0);//connect success, disable the adv.
    if(advRet!=BLE_SUCCESS)
    {
        tlkapi_printf(APP_LOG_EN,"close adv fail:0x%x", pEvt->aclHandle);
    }
    appCtrl.aclHandle = pEvt->aclHandle;
}
static void app_event_acl_disconnect(u16 aclHandle, blc_prf_aclDisconnEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN,"event-acl disconnect,handle: 0x%x",pEvt->aclHandle);
    tlkapi_printf(APP_LOG_EN,"reason:%x", pEvt->reason);
    tlk_codec_stop(TLK_CODEC_INPUT);
    tlk_codec_stop(TLK_CODEC_OUTPUT);
    appCtrl.aclHandle = 0;
    memset((u8*)&appCtrl.configCodecIdx,0,sizeof(app_audio_ctrl_t)-4);
    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE0, 0, 0);
}
static void app_event_cis_connect(u16 aclHandle, blc_audio_cisConnEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN,"event-cis connect,handle:0x%x",pEvt->cisHandle);

    audio_error_enum ret = blc_bapus_aseReceiverStartReady(aclHandle, appCtrl.sink[0].epId);
    if(ret!=AUDIO_ESUCC)
    {
        tlkapi_printf(APP_LOG_EN,"receive start fail - ret[%d]",ret);
    }
//  tlkapi_printf(APP_LOG_EN,"pEvt->nse: %d", pEvt->nse);
//  tlkapi_printf(APP_LOG_EN,"pEvt->ft_m2s: %d",Evt->ft_m2s);
//  tlkapi_printf(APP_LOG_EN,"pEvt->ft_s2m: %d",pEvt->ft_s2m);
//  tlkapi_printf(APP_LOG_EN,"pEvt->maxPDU_m2s: %d",pEvt->maxPDU_m2s);
//  tlkapi_printf(APP_LOG_EN,"pEvt->maxPDU_s2m: %d",pEvt->maxPDU_s2m);
//  tlkapi_printf(APP_LOG_EN,"pEvt->isoIntvl: %d",pEvt->isoIntvl);
}
static void app_event_cis_disconnect(u16 aclHandle, blc_audio_cisDisconnEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN,"event-cis disconnect,handle:0x%x", pEvt->cisHandle);
    tlkapi_printf(APP_LOG_EN,"reason:%x", pEvt->reason);
    tlk_codec_stop(TLK_CODEC_INPUT);
    tlk_codec_stop(TLK_CODEC_OUTPUT);
}
static void app_event_cis_request(u16 aclHandle, blc_audio_cisReqEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN,"event-cis request,handle: 0x%x",pEvt->cisHandle);
}
static void app_event_security_done(u16 aclHandle, blc_prf_securityDoneEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN,"event-smp security done,handle:0x%x",pEvt->aclHandle);
}
static void app_event_audio_sdp_found(u16 aclHandle, blc_prf_sdpFoundEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN,"event-client sdp found,handle:0x%x, serviceID:%d, startHandle:0x%x ~ endHandle:0x%x",pEvt->aclHandle, pEvt->svcId, pEvt->startHdl, pEvt->endHdl);
}
static void app_event_audio_sdp_not_found(u16 aclHandle, blc_prf_sdpFailEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN,"event-client sdp fail,handle:0x%x, serviceID:%d",pEvt->aclHandle, pEvt->svcId);
}
static void app_event_audio_sdp_over(u16 aclHandle, blc_prf_sdpOverEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN,"event-client all sdp over,handle:0x%x",pEvt->aclHandle);
}
static void app_ep_codec_configured(u16 aclHandle, blc_bapus_codecConfiguredEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN,"event-configure codec,epID:0x%x", pEvt->audioEpId);
    if(pEvt->dir == AUDIO_DIR_SINK)
    {
        for(u8 i=0;i<APP_AUDIO_MAX_SINK_EP;i++)
        {
            if(!appCtrl.sink[i].epId || appCtrl.sink[i].epId == pEvt->audioEpId)
            {
                appCtrl.sink[i].epId = pEvt->audioEpId;
                memcpy((u8*)&appCtrl.sink[i].cP.codecId,(u8*)&pEvt->codecid.id,5);
                appCtrl.sink[i].cP.frequency = pEvt->frequency;
                appCtrl.sink[i].cP.duration = pEvt->duration;
                appCtrl.sink[i].cP.frameOcts = pEvt->frameOcts;
                appCtrl.sink[i].cP.location = pEvt->location;
                appCtrl.sink[i].cP.blocks = pEvt->codecFrmBlksPerSDU;
                appCtrl.sink[i].cP.paramReady = true;
                tlkapi_printf(APP_LOG_EN,"sink[%d]", i);
                break;
            }
        }
        tlkapi_printf(APP_LOG_EN,"frequency:%d", pEvt->frequency);
        tlkapi_printf(APP_LOG_EN,"duration:%d", pEvt->duration);
        tlkapi_printf(APP_LOG_EN,"frameOcts:%d", pEvt->frameOcts);
        tlkapi_printf(APP_LOG_EN,"location:%d", pEvt->location);
        tlkapi_printf(APP_LOG_EN,"blocks:%d", pEvt->codecFrmBlksPerSDU);
    }
    else if(pEvt->dir == AUDIO_DIR_SOURCE)
    {
        for(u8 i=0;i<APP_AUDIO_MAX_SOURCE_EP;i++)
        {
            if(!appCtrl.source[i].epId || appCtrl.source[i].epId == pEvt->audioEpId)
            {
                appCtrl.source[i].epId = pEvt->audioEpId;
                memcpy((u8*)&appCtrl.source[i].cP.codecId,(u8*)&pEvt->codecid,5);
                appCtrl.source[i].cP.frequency = pEvt->frequency;
                appCtrl.source[i].cP.duration = pEvt->duration;
                appCtrl.source[i].cP.frameOcts = pEvt->frameOcts;
                appCtrl.source[i].cP.location = pEvt->location;
                appCtrl.source[i].cP.blocks = pEvt->codecFrmBlksPerSDU;
                appCtrl.source[i].cP.paramReady = true;
                tlkapi_printf(APP_LOG_EN,"source[%d]", i);
                break;
            }
        }
        tlkapi_printf(APP_LOG_EN,"frequency:%d", pEvt->frequency);
        tlkapi_printf(APP_LOG_EN,"duration:%d", pEvt->duration);
        tlkapi_printf(APP_LOG_EN,"frameOcts:%d", pEvt->frameOcts);
        tlkapi_printf(APP_LOG_EN,"location:%d", pEvt->location);
        tlkapi_printf(APP_LOG_EN,"blocks:%d", pEvt->codecFrmBlksPerSDU);
    }
}
static void app_ep_qos_configured(u16 aclHandle, blc_bapus_qosConfiguredEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN,"event-config qos,epID 0x%x",pEvt->audioEpId);
    if(pEvt->dir == AUDIO_DIR_SINK)
    {
        for(u8 i=0;i<APP_AUDIO_MAX_SINK_EP;i++)
        {
            if(appCtrl.sink[i].epId == pEvt->audioEpId)
            {
                appCtrl.sink[i].pD = pEvt->presentationDelay;//us
                tlkapi_printf(APP_LOG_EN,"Sink[%d]-PD %d", i,pEvt->presentationDelay);
                tlkapi_printf(APP_LOG_EN,"Sink[%d]-maxTransLatency %d", i,pEvt->maxTransLatency);
            }
        }
    }
    else if(pEvt->dir == AUDIO_DIR_SOURCE)
    {
        for(u8 i=0;i<APP_AUDIO_MAX_SOURCE_EP;i++)
        {
            if(appCtrl.source[i].epId == pEvt->audioEpId)
            {
                appCtrl.source[i].pD = pEvt->presentationDelay;//us
                tlkapi_printf(APP_LOG_EN,"Source[%d]-PD %d", i,pEvt->presentationDelay);
            }
        }
    }
    appCtrl.configCodecIdx = true;
}
static void app_ep_enabling(u16 aclHandle, blc_bapus_enablingEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN,"event-enabling,epID 0x%x", pEvt->audioEpId);
    if(pEvt->dir == AUDIO_DIR_SINK)
    {
        for(u8 i=0;i<APP_AUDIO_MAX_SINK_EP;i++)
        {
            if(appCtrl.sink[i].epId == pEvt->audioEpId)
            {
                tlkapi_printf(APP_LOG_EN,"sink[%d]",i);
                if(tlk_codec_getState(TLK_CODEC_OUTPUT)!=TLK_CODEC_STATE_IDLE)
                {
                    audio_error_enum ret = blc_bapus_aseReceiverStartReady(aclHandle, pEvt->audioEpId);
                    if(ret!=AUDIO_ESUCC)
                    {
                        tlkapi_printf(APP_LOG_EN,"receive start fail - ret[%d]",ret);
                    }
                    return;
                }
            }
        }
    }
    else if(pEvt->dir == AUDIO_DIR_SOURCE)
    {
        for(u8 i=0;i<APP_AUDIO_MAX_SINK_EP;i++)
        {
            if(appCtrl.sink[i].epId == pEvt->audioEpId)
            {
                tlkapi_printf(APP_LOG_EN,"source[%d]",i);
            }
        }
    }
}
static void app_ep_receive_streaming(u16 aclHandle, blc_bapus_receiveStreamingEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN,"event-sink stream start,epID 0x%x",pEvt->audioEpId);
    for(u8 i=0;i<APP_AUDIO_MAX_SINK_EP;i++)
    {
        if(appCtrl.sink[i].epId == pEvt->audioEpId)
        {
            appCtrl.sink[i].sS = true;
            tlkapi_printf(APP_LOG_EN,"sink[%d]",i);
            tlk_codec_sts_e codecRet = tlk_codec_start(TLK_CODEC_OUTPUT);
            if(codecRet == TLK_CODEC_STATE_ERROR)
            {
                tlkapi_printf(APP_LOG_EN,"error codec state");
            }
            else if(codecRet == TLK_CODEC_OPERATION_REPEAT)
            {
                tlkapi_printf(APP_LOG_EN,"codec already enable");
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"codec-output enable success");
            }
        }
    }
}
static void app_ep_send_streaming(u16 aclHandle, blc_bapus_sendStreamingEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN,"event-source stream start,epID 0x%x",pEvt->audioEpId);

    for(u8 i=0;i<APP_AUDIO_MAX_SOURCE_EP;i++)
    {
        if(appCtrl.source[i].epId == pEvt->audioEpId)
        {
            appCtrl.source[i].sS = true;
            tlkapi_printf(APP_LOG_EN,"source[%d]",i);
            tlk_codec_sts_e codecRet = tlk_codec_start(TLK_CODEC_INPUT);
            if(codecRet == TLK_CODEC_STATE_ERROR)
            {
                tlkapi_printf(APP_LOG_EN,"error codec state");
            }
            else if(codecRet == TLK_CODEC_OPERATION_REPEAT)
            {
                tlkapi_printf(APP_LOG_EN,"codec already enable");
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"codec-input enable success");
            }
        }
    }
}
static void app_ep_disabling(u16 aclHandle, blc_bapus_disablingEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN,"event-disable,epID 0x%x",pEvt->audioEpId);
    for(u8 i=0;i<APP_AUDIO_MAX_SOURCE_EP;i++)
    {
        if(appCtrl.source[i].epId == pEvt->audioEpId)
        {
            appCtrl.source[i].sS = false;
            tlkapi_printf(APP_LOG_EN,"source[%d]",i);
        }
    }
    for(u8 i=0;i<APP_AUDIO_MAX_SINK_EP;i++)
    {
        if(appCtrl.sink[i].epId == pEvt->audioEpId)
        {
            appCtrl.sink[i].sS = false;
            tlkapi_printf(APP_LOG_EN,"sink[%d]",i);
        }
    }
}
static void app_ep_releasing(u16 aclHandle, blc_bapus_releasingEvt_t *pEvt)
{
    tlkapi_printf(APP_LOG_EN,"event-release,epID 0x%x",pEvt->audioEpId);
    for(u8 i=0;i<APP_AUDIO_MAX_SOURCE_EP;i++)
    {
        if(appCtrl.source[i].epId == pEvt->audioEpId)
        {
            appCtrl.source[i].sS = false;
            tlkapi_printf(APP_LOG_EN,"source[%d]",i);
        }
    }
    bool inputCloseIdx = true;
    for(u8 i=0;i<APP_AUDIO_MAX_SOURCE_EP;i++)
    {
        if(appCtrl.source[i].sS)
        {
            inputCloseIdx = false;
        }
    }
    if(inputCloseIdx)
    {
//      tlk_codec_close(TLK_CODEC_INPUT);//TODO
    }
    for(u8 i=0;i<APP_AUDIO_MAX_SINK_EP;i++)
    {
        if(appCtrl.sink[i].epId == pEvt->audioEpId)
        {
            appCtrl.sink[i].sS = false;
            tlkapi_printf(APP_LOG_EN,"sink[%d]",i);
        }
    }

    bool outputCloseIdx = true;
    for(u8 i=0;i<APP_AUDIO_MAX_SINK_EP;i++)
    {
        if(appCtrl.sink[i].sS)
        {
            outputCloseIdx = false;
        }
    }
    if(outputCloseIdx)
    {
//      tlk_codec_close(TLK_CODEC_OUTPUT);//TODO
    }
}

/**
 * @brief      Audio loop handler process.
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

#if (TLK_TONE_ENABLE)
extern app_codec_desc_t codecO;
extern unsigned short   app_audio_output_buffer[APP_AUDIO_OUTPUT_BUFFER_SIZE];
_attribute_ram_code_ void app_audio_tone_handle_task(void)
{
    u32 td;

    if(!appCtrl.is_tone_codec_cfg)
    {
        // tone codec flag
        appCtrl.is_tone_codec_cfg = 1;
        tlkapi_printf(APP_LOG_EN,"is_tone_codec_cfg %d", appCtrl.is_tone_codec_cfg);
        // first audio buffer is mute
        appCtrl.tone_len = 240 * sizeof(tcodec_type);
        memset(appCtrl.tone_buff, 0, appCtrl.tone_len);
        // stop cis codec
        app_list_free();
        tlk_codec_stop(TLK_CODEC_OUTPUT);
        // start tone codec
#if TLK_TONE_MONO_MODE
        tlk_codec_config(TLK_CODEC_OUTPUT,TLK_CODEC_FREQ_48000,TLK_CODEC_1_CHANNEL,TLK_CODEC_LINE,(u8*)app_audio_output_buffer,2*APP_AUDIO_OUTPUT_BUFFER_SIZE);
#else
        tlk_codec_config(TLK_CODEC_OUTPUT,TLK_CODEC_FREQ_48000,TLK_CODEC_2_CHANNEL,TLK_CODEC_LINE,(u8*)app_audio_output_buffer,2*APP_AUDIO_OUTPUT_BUFFER_SIZE);
#endif
        tlk_codec_start(TLK_CODEC_OUTPUT);
        tlk_codec_output_setWriteOffset(200);
    }

    if(appCtrl.tone_len > 0)
    {
        codec_output_writeData((u8*)appCtrl.tone_buff,appCtrl.tone_len);
        appCtrl.tone_len = 0;
    }

    appCtrl.tone_len = tlk_tone_get_sample((int16_t *)appCtrl.tone_buff, 240 * sizeof(tcodec_type), 48000);

    if(appCtrl.tone_len == 0)
    {
        appCtrl.is_tone_codec_cfg = 0;
        tlkapi_printf(APP_LOG_EN,"is_tone_codec_cfg %d", appCtrl.is_tone_codec_cfg);
        if(tlk_tone_is_playing())
        {
            tlkapi_printf(APP_LOG_EN,"tone_handle_next");
        }
        else
        {
            tlk_codec_stop(TLK_CODEC_OUTPUT);
            tlk_codec_config(TLK_CODEC_OUTPUT,appCtrl.sink[0].cP.frequency,codecO.cC,TLK_CODEC_LINE,(u8*)app_audio_output_buffer,2*APP_AUDIO_OUTPUT_BUFFER_SIZE);
            tlk_codec_start(TLK_CODEC_OUTPUT);
            tlkapi_printf(APP_LOG_EN,"tone_handle_exit");
            return;
        }
    }

    td = (clock_time() - g_tone_cfg.last_tone_tick) / SYSTEM_TIMER_TICK_1US;
    td = 5000 - td;
    if(td > 5000) {
        td = 5000;
        g_tone_cfg.last_tone_tick = clock_time();
    }
    g_tone_cfg.last_tone_tick += 5000 * SYSTEM_TIMER_TICK_1US;
    // set next timer
    ble_audio_timer_set_capture(TIMER0, 0,  td * sys_clk.pclk);
}
#endif
#endif
