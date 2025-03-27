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
#include "app_call.h"
#include "app_media.h"
#include "app_att.h"

#if (INTER_TEST_MODE == TEST_CIS_AUDIO_SERVER)

app_audio_ctrl_t appCtrl;

static int  app_audio_prfEvtCb(u16 connHandle, int evtID, u8 *pData, u16 dataLen);
static void app_ep_config_codec_process(u16 connHandle,u8 *pData, u16 dataLen);
static void app_ep_config_qos_process(u16 connHandle,u8 *pData, u16 dataLen);
static void app_ep_enable_process(u16 connHandle,u8 *pData, u16 dataLen);
static void app_ep_sink_start_process(u16 connHandle,u8 *pData, u16 dataLen);
static void app_ep_source_start_process(u16 connHandle,u8 *pData, u16 dataLen);
static void app_ep_disable_process(u16 connHandle,u8 *pData, u16 dataLen);
static void app_ep_release_process(u16 connHandle,u8 *pData, u16 dataLen);
static void app_ep_handler(void);


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

    app_codec_init();

    BLT_APP_LOG("audio unicast server init");
}

static void app_ep_handler(void)
{
    for(u8 i=0;i<APP_AUDIO_MAX_SINK_EP;i++)
    {
        if(appCtrl.sink[i].epOp&APP_EP_RECEIVE_START)
        {
            if(tlk_codec_getState(TLK_CODEC_OUTPUT) != TLK_CODEC_STATE_IDLE)
            {
                int ret = blc_bapus_Start(appCtrl.aclHandle,appCtrl.sink[i].epId);
                if(ret==AUDIO_ESUCC)
                {
                    appCtrl.sink[i].epOp &= ~APP_EP_RECEIVE_START;
                }
                else if(ret==AUDIO_ENOREADY)
                {
                    //wait until cis established.
                }
                else
                {
                    BLT_APP_LOG("sink start fail-reason: %d", ret);
                }
            }
        }
        if(appCtrl.sink[i].epOp&APP_EP_RELEASE)
        {
            int ret = blc_bapus_aseReleasedByCache(appCtrl.aclHandle,appCtrl.sink[i].epId,0);
            if(ret!=AUDIO_ESUCC)
            {
                BLT_APP_LOG("error status: %d", ret);
            }
            else
            {
                appCtrl.sink[i].epOp &= ~APP_EP_RELEASE;
            }
        }
    }
    for(u8 i=0;i<APP_AUDIO_MAX_SOURCE_EP;i++)
    {
        if(appCtrl.source[i].epOp&APP_EP_RECEIVE_STOP)
        {
            int ret = blc_bapus_Stop(appCtrl.aclHandle,appCtrl.source[i].epId);
            if(ret==AUDIO_ESUCC)
            {
                appCtrl.source[i].epOp &= ~APP_EP_RECEIVE_STOP;
            }
            else if(ret==AUDIO_ESTATUS)
            {
                BLT_APP_LOG("error state");
            }
            else if(ret==AUDIO_EDIR)
            {
                BLT_APP_LOG("error dir ep : %d", appCtrl.source[i].epOp);
            }
        }
        if(appCtrl.source[i].epOp&APP_EP_RELEASE)
        {
            int ret = blc_bapus_aseReleasedByCache(appCtrl.aclHandle,appCtrl.source[i].epId,0);
            if(ret!=AUDIO_ESUCC)
            {
                BLT_APP_LOG("error status: %d", ret);
            }
            else
            {
                appCtrl.source[i].epOp &= ~APP_EP_RELEASE;
            }
        }
    }
}

static int app_audio_ep_event_callback(u16 connHandle, int evtID, u8 *pData, u16 dataLen)
{
    switch(evtID)
    {
        case AUDIO_EVT_BAPUS_CODEC_CONFIGURED:
        {
            app_ep_config_codec_process(connHandle,pData,dataLen);
        }
        break;
        case AUDIO_EVT_BAPUS_QOS_CONFIGURED:
        {
            app_ep_config_qos_process(connHandle,pData,dataLen);
        }
        break;
        case AUDIO_EVT_BAPUS_ENABLING:
        {
            app_ep_enable_process(connHandle,pData,dataLen);

        }
        break;
        case AUDIO_EVT_BAPUS_RECEIVE_STREAMING:
        {
            app_ep_sink_start_process(connHandle,pData,dataLen);

        }
        break;
        case AUDIO_EVT_BAPUS_SEND_STREAMING:
        {
            app_ep_source_start_process(connHandle,pData,dataLen);
        }
        break;
        case AUDIO_EVT_BAPUS_DISABLING:
        {
            app_ep_disable_process(connHandle,pData,dataLen);
        }
        break;
        case AUDIO_EVT_BAPUS_RELEASING:
        {
            app_ep_release_process(connHandle,pData,dataLen);
        }
        break;

        default:
        break;
    }
    return 0;
}

static int app_audio_prfEvtCb(u16 connHandle, int evtID, u8 *pData, u16 dataLen)
{
    switch(evtID)
    {
        case PRF_EVTID_ACL_CONNECT:
        {
            blc_prf_aclConnEvt_t *pEvt = (blc_prf_aclConnEvt_t*)pData;
            BLT_APP_LOG("event-acl connect, handle: 0x%x",pEvt->aclHandle);
            ble_sts_t advRet = blc_ll_setExtAdvEnable(BLC_ADV_DISABLE, ADV_HANDLE0, 0, 0);//connect success, disable the adv.
            if(advRet!=BLE_SUCCESS)
            {
                BLT_APP_LOG("close adv fail:0x%x", pEvt->aclHandle);
            }
            appCtrl.aclHandle = pEvt->aclHandle;
        }
        break;
        case PRF_EVTID_ACL_DISCONNECT:
        {

            blc_prf_aclDisconnEvt_t *pEvt = (blc_prf_aclDisconnEvt_t*)pData;
            BLT_APP_LOG("event-acl disconnect,handle: 0x%x",pEvt->aclHandle);
            BLT_APP_LOG("reason:%x", pEvt->reason);
//          tlk_codec_close(TLK_CODEC_INPUT);
//          tlk_codec_close(TLK_CODEC_OUTPUT);
            app_list_free();
            appCtrl.aclHandle = 0;
            memset((u8*)&appCtrl.codecI,0,sizeof(app_audio_ctrl_t)-8);
            blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE0, 0, 0);
        }
        break;
        case AUDIO_EVT_CIS_CONNECT:
        {
            blc_audio_cisConnEvt_t *pEvt = (blc_audio_cisConnEvt_t*)pData;
            BLT_APP_LOG("event-cis connect,handle:0x%x",pEvt->cisHandle);
//          BLT_APP_LOG("pEvt->nse: %d", pEvt->nse);
//          BLT_APP_LOG("pEvt->ft_m2s: %d",Evt->ft_m2s);
//          BLT_APP_LOG("pEvt->ft_s2m: %d",pEvt->ft_s2m);
//          BLT_APP_LOG("pEvt->maxPDU_m2s: %d",pEvt->maxPDU_m2s);
//          BLT_APP_LOG("pEvt->maxPDU_s2m: %d",pEvt->maxPDU_s2m);
//          BLT_APP_LOG("pEvt->isoIntvl: %d",pEvt->isoIntvl);
        }
        break;
        case AUDIO_EVT_CIS_DISCONNECT://todo
        {
            blc_audio_cisDisconnEvt_t *pEvt = (blc_audio_cisDisconnEvt_t*)pData;
            BLT_APP_LOG("event-cis disconnect,handle:0x%x", pEvt->cisHandle);
            BLT_APP_LOG("reason:%x", pEvt->reason);
        }
        break;
        case AUDIO_EVT_CIS_REQUEST:
        {
            blc_audio_cisReqEvt_t *pEvt = (blc_audio_cisReqEvt_t*)pData;
            BLT_APP_LOG("event-cis request,handle: 0x%x",pEvt->cisHandle);
        }
        break;
        case PRF_EVTID_SMP_SECURITY_DONE:
        {
            blc_prf_securityDoneEvt_t *pEvt = (blc_prf_securityDoneEvt_t*)pData;
            BLT_APP_LOG("event-smp security done,handle:0x%x",pEvt->aclHandle);
        }
        break;

        case PRF_EVTID_CLIENT_ALL_SDP_OVER:
        {
            blc_prf_sdpOverEvt_t *pEvt = (blc_prf_sdpOverEvt_t*)pData;
            BLT_APP_LOG("event-sdk over,handle:0x%x",pEvt->aclHandle);
        }
        break;

        default:
        {
            /* audio end-point EVT */
            app_audio_ep_event_callback(connHandle, evtID, pData, dataLen);
            /* audio call EVT */
            app_call_event_callback(connHandle, evtID, pData, dataLen);
            /* audio media EVT */
            app_media_event_callback(connHandle, evtID, pData, dataLen);
        }
        break;
    }
    return 0;
}

static void app_ep_config_codec_process(u16 connHandle,u8 *pData, u16 dataLen)
{
    blc_bapus_codecConfiguredEvt_t *pEvt = (blc_bapus_codecConfiguredEvt_t *)pData;
    BLT_APP_LOG("event-configure codec,epID:0x%x", pEvt->audioEpId);
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
                BLT_APP_LOG("sink[%d]", i);
                break;
            }
        }
        BLT_APP_LOG("frequency:%d", pEvt->frequency);
        BLT_APP_LOG("duration:%d", pEvt->duration);
        BLT_APP_LOG("frameOcts:%d", pEvt->frameOcts);
        BLT_APP_LOG("location:%d", pEvt->location);
        BLT_APP_LOG("blocks:%d", pEvt->codecFrmBlksPerSDU);
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
                BLT_APP_LOG("source[%d]", i);
                break;
            }
        }
        BLT_APP_LOG("frequency:%d", pEvt->frequency);
        BLT_APP_LOG("duration:%d", pEvt->duration);
        BLT_APP_LOG("frameOcts:%d", pEvt->frameOcts);
        BLT_APP_LOG("location:%d", pEvt->location);
        BLT_APP_LOG("blocks:%d", pEvt->codecFrmBlksPerSDU);
    }
}

static void  app_ep_config_qos_process(u16 connHandle,u8 *pData, u16 dataLen)
{
    blc_bapus_qosConfiguredEvt_t *pEvt = (blc_bapus_qosConfiguredEvt_t *)pData;
    BLT_APP_LOG("event-config qos,epID 0x%x",pEvt->audioEpId);
    if(pEvt->dir == AUDIO_DIR_SINK)
    {
        for(u8 i=0;i<APP_AUDIO_MAX_SINK_EP;i++)
        {
            if(appCtrl.sink[i].epId == pEvt->audioEpId)
            {
                appCtrl.sink[i].pD = pEvt->presentationDelay;//us
                BLT_APP_LOG("Sink[%d]-PD %d", i,pEvt->presentationDelay);
                BLT_APP_LOG("Sink[%d]-maxTransLatency %d", i,pEvt->maxTransLatency);
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
                BLT_APP_LOG("Source[%d]-PD %d", i,pEvt->presentationDelay);
            }
        }
    }
    appCtrl.configCodecIdx = true;
}

static void  app_ep_enable_process(u16 connHandle,u8 *pData, u16 dataLen)
{
    blc_bapus_enablingEvt_t *pEvt = (blc_bapus_enablingEvt_t *)pData;
    BLT_APP_LOG("event-enable,epID 0x%x", pEvt->audioEpId);
    if(pEvt->dir == AUDIO_DIR_SINK)
    {
        for(u8 i=0;i<APP_AUDIO_MAX_SINK_EP;i++)
        {
            if(appCtrl.sink[i].epId == pEvt->audioEpId)
            {
                BLT_APP_LOG("sink[%d]",i);
                appCtrl.sink[i].epOp |= APP_EP_RECEIVE_START;//app layer prepare to receive packet
            }
        }
    }
    else if(pEvt->dir == AUDIO_DIR_SOURCE)
    {
        for(u8 i=0;i<APP_AUDIO_MAX_SINK_EP;i++)
        {
            if(appCtrl.sink[i].epId == pEvt->audioEpId)
            {
                BLT_APP_LOG("source[%d]",i);
            }
        }
    }
}

static void  app_ep_sink_start_process(u16 connHandle,u8 *pData, u16 dataLen)
{
    blc_bapus_receiveStreamingEvt_t *pEvt = (blc_bapus_receiveStreamingEvt_t *)pData;
    BLT_APP_LOG("event-sink stream start,epID 0x%x",pEvt->audioEpId);
    for(u8 i=0;i<APP_AUDIO_MAX_SINK_EP;i++)
    {
        if(appCtrl.sink[i].epId == pEvt->audioEpId)
        {
            appCtrl.sink[i].sS = true;
            BLT_APP_LOG("sink[%d]",i);
            tlk_codec_sts_e codecRet = tlk_codec_start(TLK_CODEC_OUTPUT);
            if(codecRet == TLK_CODEC_STATE_ERROR)
            {
                BLT_APP_LOG("error codec state");
            }
            else if(codecRet == TLK_CODEC_OPERATION_REPEAT)
            {
                BLT_APP_LOG("codec already enable");
            }
            else
            {
                BLT_APP_LOG("codec-input enable success");
            }
        }
    }
}
static void  app_ep_source_start_process(u16 connHandle,u8 *pData, u16 dataLen)
{
    blc_bapus_sendStreamingEvt_t *pEvt = (blc_bapus_sendStreamingEvt_t *)pData;
    BLT_APP_LOG("event-source stream start,epID 0x%x",pEvt->audioEpId);

    for(u8 i=0;i<APP_AUDIO_MAX_SOURCE_EP;i++)
    {
        if(appCtrl.source[i].epId == pEvt->audioEpId)
        {
            appCtrl.source[i].sS = true;
            BLT_APP_LOG("source[%d]",i);
            tlk_codec_sts_e codecRet = tlk_codec_start(TLK_CODEC_INPUT);
            if(codecRet == TLK_CODEC_STATE_ERROR)
            {
                BLT_APP_LOG("error codec state");
            }
            else if(codecRet == TLK_CODEC_OPERATION_REPEAT)
            {
                BLT_APP_LOG("codec already enable");
            }
            else
            {
                BLT_APP_LOG("codec-input enable success");
            }
        }
    }
}
static void  app_ep_disable_process(u16 connHandle,u8 *pData, u16 dataLen)
{
    blc_bapus_disablingEvt_t *pEvt = (blc_bapus_disablingEvt_t *)pData;
    BLT_APP_LOG("event-disable,epID 0x%x",pEvt->audioEpId);
    for(u8 i=0;i<APP_AUDIO_MAX_SOURCE_EP;i++)
    {
        if(appCtrl.source[i].epId == pEvt->audioEpId)
        {
            appCtrl.source[i].epOp |= APP_EP_RECEIVE_STOP;//app layer prepare to stop send packet
            appCtrl.source[i].sS = false;
            BLT_APP_LOG("source[%d]",i);
        }
    }
    for(u8 i=0;i<APP_AUDIO_MAX_SINK_EP;i++)
    {
        if(appCtrl.sink[i].epId == pEvt->audioEpId)
        {
            appCtrl.sink[i].sS = false;
            BLT_APP_LOG("sink[%d]",i);
        }
    }
}
static void  app_ep_release_process(u16 connHandle,u8 *pData, u16 dataLen)
{
    blc_bapus_releasingEvt_t *pEvt = (blc_bapus_releasingEvt_t *)pData;
    BLT_APP_LOG("event-release,epID 0x%x",pEvt->audioEpId);
    for(u8 i=0;i<APP_AUDIO_MAX_SOURCE_EP;i++)
    {
        if(appCtrl.source[i].epId == pEvt->audioEpId)
        {
            appCtrl.source[i].sS = false;
            appCtrl.source[i].epOp |= APP_EP_RELEASE;//app layer prepare to stop send packet
            BLT_APP_LOG("source[%d]",i);
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
//      tlk_codec_close(TLK_CODEC_INPUT);
    }
    for(u8 i=0;i<APP_AUDIO_MAX_SINK_EP;i++)
    {
        if(appCtrl.sink[i].epId == pEvt->audioEpId)
        {
            appCtrl.sink[i].sS = false;
            appCtrl.sink[i].epOp |= APP_EP_RELEASE;//app layer prepare to stop send packet
            BLT_APP_LOG("sink[%d]",i);
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
//      tlk_codec_close(TLK_CODEC_OUTPUT);
    }
}

void app_audio_handler(void)
{
    blc_prf_main_loop();
    app_ep_handler();
    app_codec_handler();
}


#endif /* INTER_TEST_MODE */
