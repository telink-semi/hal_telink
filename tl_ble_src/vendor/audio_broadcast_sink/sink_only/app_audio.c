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
#include "../sink_config.h"
#if (SINK_VERSION == SINK_ONLY_VERSION)

#include "app_config.h"

#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "app_buffer.h"
#include "app_audio.h"
#include "app.h"
#include "../app_ctrl_audio.h"

u16 aclConnHandle = 0x0000;

u16 app_audio_get_acl_conn_handle(void)
{
    return aclConnHandle;
}

appSinkVcpState_t appSinkVcpState = {
    .volume = VOLUME_INITIAL_VALUE,
    .mute = MUTE_INITIAL_VALUE,
    .volOffset[0] = LEFT_VOL_OFFSET_INITIAL_VALUE,
    .volOffset[1] = RIGHT_VOL_OFFSET_INITIAL_VALUE,
};

/**
 * @brief       change volume state.
 * @param[in]   none
 * @return      none
 */
static void app_changeVolState(void)
{
#if (MCU_CORE_TYPE == MCU_CORE_B91)
    if(appSinkVcpState.mute) {
        audio_set_codec_dac_mute();
    }
    else{
        audio_set_codec_dac_unmute();
    }
    audio_set_codec_dac_gain(appSinkVcpState.volume);
#endif
}

/**
 * @brief       set volume and mute state.
 * @param[in]   volume: volume value.
 * @param[in]   mute: mute state.
 * @return      none
 */
void app_setVolState(u8 volume, bool mute)
{
    appSinkVcpState.volume = volume;
    appSinkVcpState.mute = mute;
    app_changeVolState();
}

/**
 * @brief       set volume up and send volume state to remote device.
 * @param[in]   none
 * @return      none
 */
void app_send_volUp(void)
{
    if(appSinkVcpState.volume == 255)       return ;

    if(appSinkVcpState.volume + VLOUME_STEP_INITIAL_VALUE >= 255)
    {
        appSinkVcpState.volume = 255;
    }
    else
    {
        appSinkVcpState.volume += VLOUME_STEP_INITIAL_VALUE;
    }

    blc_vcss_updateVolSetting(app_audio_get_acl_conn_handle(), appSinkVcpState.volume);
    app_changeVolState();
}

/**
 * @brief       set volume down and send volume state to remote device.
 * @param[in]   none
 * @return      none
 */
void app_send_volDown(void)
{
    if(appSinkVcpState.volume == 0)     return ;

    if(appSinkVcpState.volume - VLOUME_STEP_INITIAL_VALUE <= 0)
    {
        appSinkVcpState.volume = 0;
    }
    else
    {
        appSinkVcpState.volume -= VLOUME_STEP_INITIAL_VALUE;
    }

    blc_vcss_updateVolSetting(app_audio_get_acl_conn_handle(), appSinkVcpState.volume);
    app_changeVolState();
}

/**
 * @brief       send mute state to remote device.
 * @param[in]   none
 * @return      none
 */
void app_send_mute(void)
{
    if(appSinkVcpState.mute)    return ;

    appSinkVcpState.mute = true;
    blc_vcss_updateMuteState(app_audio_get_acl_conn_handle(), appSinkVcpState.mute);
    app_changeVolState();
}

/**
 * @brief       send unmute state to remote device.
 * @param[in]   none
 * @return      none
 */
void app_send_unmute(void)
{
    if(!appSinkVcpState.mute)   return ;

    appSinkVcpState.mute = false;
    blc_vcss_updateMuteState(app_audio_get_acl_conn_handle(), appSinkVcpState.mute);
    app_changeVolState();
}

/**
 * @brief       change mute state and send mute/unmute state to remote device.
 * @param[in]   none
 * @return      none
 */
void app_send_changeMuteState(void)
{
    if(appSinkVcpState.mute)
    {
        appSinkVcpState.mute = false;
    }
    else
    {
        appSinkVcpState.mute = true;
    }

    blc_vcss_updateMuteState(app_audio_get_acl_conn_handle(), appSinkVcpState.mute);
    app_changeVolState();
}


/**
 * @brief       controller acl connect event.
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   pData: Data carried by the event.
 * @param[in]   dataLen: data length.
 * @return      0.
 */
static int app_common_aclConnectCb(u16 connHandle, u8 *pData, u16 dataLen)
{
    gpio_write(GPIO_LED_BLUE, 1);
#if(UI_9517C)
    gpio_write(GPIO_LED_BLUE_9517C, 1);
#endif
    aclConnHandle = connHandle;
    blc_ll_setExtAdvEnable(BLC_ADV_DISABLE, ADV_HANDLE0, 0, 0);//connect success, disable the adv.
    tlkapi_printf(APP_PRF_EVT_LOG_EN, "bis sink acl connect:0x%x\n", connHandle);
    return 0;
}

/**
 * @brief       controller acl disconnect event.
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   pData: Data carried by the event.
 * @param[in]   dataLen: data length.
 * @return      0.
 */
static int app_common_aclDisconnectCb(u16 connHandle, u8 *pData, u16 dataLen)
{
    gpio_write(GPIO_LED_BLUE, 0);
#if(UI_9517C)
    gpio_write(GPIO_LED_BLUE_9517C, 0);
#endif
    aclConnHandle = 0x0000;
    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE0, 0, 0);//[!!!] TODO: must start
    tlkapi_printf(APP_PRF_EVT_LOG_EN, "bis sink acl disconnect:0x%x\n", connHandle);
    return 0;
}

/**
 * @brief       BAP Broadcast Sink receive remote scan stopped event.
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   pData: Data carried by the event.
 * @param[in]   dataLen: data length.
 * @return      0.
 */
static int app_bapbs_remoteScanStoppedCb(u16 connHandle, u8 *pData, u16 dataLen)
{
    tlkapi_printf(APP_PRF_EVT_LOG_EN, "BIS sink remote scan stop:0x%x\n", connHandle);
    return 0;
}

/**
 * @brief       BAP Broadcast Sink receive remote scan started event.
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   pData: Data carried by the event.
 * @param[in]   dataLen: data length.
 * @return      0.
 */
static int app_bapbs_remoteScanStartedCb(u16 connHandle, u8 *pData, u16 dataLen)
{
    tlkapi_printf(APP_PRF_EVT_LOG_EN, "BIS sink remote scan start:0x%x\n", connHandle);
    return 0;
}

/**
 * @brief       BAP Broadcast Sink initial codec device event.
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   pData: Data carried by the event.
 * @param[in]   dataLen: data length.
 * @return      0.
 */
static int app_bapbs_initCodec(u16 connHandle, u8 *pData, u16 dataLen)
{
    blc_bapbs_bisSinkInitCodecEvt_t* initCodecEvt = (blc_bapbs_bisSinkInitCodecEvt_t*)pData;
    tlkapi_printf(APP_PRF_EVT_LOG_EN, "Bis sink init codec, PresentationDelay:%dus bisNumber is %d\n", initCodecEvt->presentationDelay, initCodecEvt->bisNum);

    app_codec_setBigInformation((blc_bapbs_bisSinkInitCodecEvt_t*)pData);

    return 0;
}

/**
 * @brief       BAP Broadcast Sink sync BIG state change event.
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   pData: Data carried by the event.
 * @param[in]   dataLen: data length.
 * @return      0.
 */
static int app_bapbs_syncBig(u16 connHandle, u8 *pData, u16 dataLen)
{
    blc_bapbs_BisSinkSyncBigEvt_t* bigSyncState = (blc_bapbs_BisSinkSyncBigEvt_t*)pData;
    tlkapi_printf(APP_PRF_EVT_LOG_EN, "BIG Sync state:%d, bigHandle is 0x%02x, bis number is %d, iosInterval is %.2fms\n",
            bigSyncState->state, bigSyncState->bigHandle, bigSyncState->numBis, bigSyncState->isoInterval*1.25);
    tlkapi_printf(APP_PRF_EVT_LOG_EN, "bis Handle 1 is 0x%02x, bis Handle 2 is 0x%02x\n",
            bigSyncState->bisHandles[0], bigSyncState->bisHandles[1]);
    if(bigSyncState->state == BIG_SYNCED)
    {
        gpio_write(GPIO_LED_GREEN, 1);
#if(UI_9517C)
        gpio_write(GPIO_LED_GREEN_9517C, 1);
#endif
        app_codec_setBigSyncState(BIG_SYNCED, bigSyncState->numBis, bigSyncState->bisHandles);

    }
    else
    {
        gpio_write(GPIO_LED_GREEN, 0);
#if(UI_9517C)
        gpio_write(GPIO_LED_GREEN_9517C, 0);
#endif
        app_codec_setBigSyncState(BIG_LOST, 0, NULL);
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
static int app_vcss_changedVolState(u16 connHandle, u8 *pData, u16 dataLen)
{
    blc_vcss_volumeStateChangeEvt_t* volStateChange = (blc_vcss_volumeStateChangeEvt_t*)pData;
    tlkapi_printf(APP_PRF_EVT_LOG_EN, "vol setting data is:0x%x, mute state is %s", volStateChange->volumeSetting, volStateChange->mute?"muted":"mute");

    app_setVolState(volStateChange->volumeSetting, volStateChange->mute);
    return 0;
}

/**
 * @brief       Broadcast Sink register profile event callback.
 */
static const app_audio_evtCb_t sinkCb[] = {
    /* Event for controller or Host */
    {PRF_EVTID_ACL_CONNECT              , app_common_aclConnectCb},
    {PRF_EVTID_ACL_DISCONNECT           , app_common_aclDisconnectCb},
    /* Event for BAP Broadcast Sink */
    {AUDIO_EVT_BAPBS_REMOTE_SCAN_STOPPED, app_bapbs_remoteScanStoppedCb},
    {AUDIO_EVT_BAPBS_REMOTE_SCAN_STARTED, app_bapbs_remoteScanStartedCb},
    {AUDIO_EVT_BAPBS_BIS_SINK_INIT_CODEC, app_bapbs_initCodec},
    {AUDIO_EVT_BAPBS_BIS_SINK_SYNC_BIG  , app_bapbs_syncBig},
    /* Event for VCS Server */
    {AUDIO_EVT_VCSS_CHANGED_VOLUME_STATE, app_vcss_changedVolState},

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

    for(int i=0; i < ARRAY_SIZE(sinkCb); i++)
    {
        if(sinkCb[i].id == evtID)
            return sinkCb[i].evtCb(aclHandle, pData, dataLen);
    }

    return 0;
}

//////////// Audio parameters setting  Begin /////////////////////////
//Broadcast sink(Scan delegator) parameter configure
/**
 * sink PAC parameter value, supported only LC3, 8_2, 16_2, 24_2, 32_2, 48_2, 48_4, 48_6.
 */
static const blc_audio_pacParam_t sinkPac[] = {
    {LC3_CAP_8_2 (BLC_AUDIO_CHANNEL_COUNTS_1 | BLC_AUDIO_CHANNEL_COUNTS_2, 2)},
    {LC3_CAP_16_2(BLC_AUDIO_CHANNEL_COUNTS_1 | BLC_AUDIO_CHANNEL_COUNTS_2, 2)},
    {LC3_CAP_24_2(BLC_AUDIO_CHANNEL_COUNTS_1 | BLC_AUDIO_CHANNEL_COUNTS_2, 2)},
    {LC3_CAP_32_2(BLC_AUDIO_CHANNEL_COUNTS_1 | BLC_AUDIO_CHANNEL_COUNTS_2, 2)},
    {LC3_CAP_48_2(BLC_AUDIO_CHANNEL_COUNTS_1 | BLC_AUDIO_CHANNEL_COUNTS_2, 2)},
    {LC3_CAP_48_4(BLC_AUDIO_CHANNEL_COUNTS_1 | BLC_AUDIO_CHANNEL_COUNTS_2, 2)},
    {LC3_CAP_48_6(BLC_AUDIO_CHANNEL_COUNTS_1 | BLC_AUDIO_CHANNEL_COUNTS_2, 2)},
};

/**
 * PACS server initial parameter.
 */
static const blc_pacss_regParam_t pacsParam = {
    .sinkPacNum = ARRAY_SIZE(sinkPac),
    .sinkPac = &sinkPac[0],
    .sinkAudioLocations = BROADCAST_SINK_LOCATION,
    .availableSinkContexts = BLC_AUDIO_CONTEXT_TYPE_MEDIA|BLC_AUDIO_CONTEXT_TYPE_LIVE,
    .availableSourceContexts = BLC_AUDIO_CONTEXT_TYPE_PROHIBITED,
    .supportedSinkContexts = BLC_AUDIO_CONTEXT_TYPE_MEDIA|BLC_AUDIO_CONTEXT_TYPE_LIVE,
    .supportedSourceContexts = BLC_AUDIO_CONTEXT_TYPE_PROHIBITED,
};

static const blc_basss_regParam_t bassParam = {
    .pastTimer = 4000,
};

/**
 * broadcast sink parameter, include BASS and PACS.
 */
static const blc_bapbs_regParam_t bcstSinkParam = {
    .pBassParam = &bassParam,
    .pPacsParam = &pacsParam,
};

/**
 * CSIP Set Member parameter configure
 */
static const blc_csiss_regParam_t csipSetMemberParam =
{
    .setRank = 1,
    .setSize = 2,
    .lockedTimeout = 60, //unit:s
    .SIRK_type = BLT_CSIS_PLAIN_TEXT_SIRK,
    .SIRK = {0x11, 0x22, 0x33, 0x44, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0f, 0x10},
};

static const blc_vocss_regParam_t vocsParam[] = {
    {
        .location = BLC_AUDIO_LOCATION_FLAG_FL,
        .volumeOffset = LEFT_VOL_OFFSET_INITIAL_VALUE,
        .desc = "Telink BIS Left Output",
    },
    {
        .location = BLC_AUDIO_LOCATION_FLAG_FR,
        .volumeOffset = RIGHT_VOL_OFFSET_INITIAL_VALUE,
        .desc = "Telink BIS Rigth Output",
    },
};

/**
 * VCP Volume Renderer parameter configure
 */
static const blc_vcss_regParam_t vcpRendererParam = {
    .vcsParam = {
        .step = VLOUME_STEP_INITIAL_VALUE,
        .volume = VOLUME_INITIAL_VALUE,
        .mute = MUTE_INITIAL_VALUE,
    },
    .vocsParam = vocsParam,
};

/**
 * TMAS role parameter configure
 */
static const blc_tmass_regParam_t sinkTmas = {
    .role = BLC_TMAP_ROLE_BROADCAST_MEDIA_RECEIVER,
};
//////////// Audio parameters setting  End /////////////////////////
/**
 * @brief       Broadcast sink initial profile function.
 * @param[in]   none.
 * @return      none.
 */
static void app_audio_init_prf(void)
{
    /* Register GAP/GATT/BAS/server */
    blc_svc_addCoreGroup();
    blc_svc_addBasGroup();
    blc_svc_addOtaGroup();

    /* Audio profile event register */
    blc_audio_initialModule(app_audio_prfEvtCb);

    /* Audio CAP acceptor init */
    //////////// Audio Stream Transitions  Begin /////////////////////////
    blc_audio_registerBapBroadcastSink(&bcstSinkParam);//BAP Broadcast Sink(Scan Delegator) init
    blc_audio_registerCSISControlServer(&csipSetMemberParam); //CSIP Set Member init
    //////////// Audio Stream Transitions  End /////////////////////////

    //////////// Capture and Rendering Control  Begin /////////////////////////
    blc_audio_registerVCSControlServer(&vcpRendererParam); //VCP Volume Renderer init
    //////////// Capture and Rendering Control  End /////////////////////////

    /* TMAP Server init */
    blc_audio_registerTMASControlServer(&sinkTmas);

    blc_svc_calculateDatabaseHash();
}

/**
 * @brief       Broadcast sink initial function.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_init(void)
{
    //audio initial controller
    u8 advData[255];
    blc_adv_ltv_t *adv_ltvs[] = {
            (blc_adv_ltv_t *) &advDefFlags,
            (blc_adv_ltv_t *) &advDefCompleteName,
            (blc_adv_ltv_t *) &advDefAppearance,
            (blc_adv_ltv_t *) &advDefBroadcastSink,
            (blc_adv_ltv_t *) &advDefTmapRole,
            };
    u16 advLen = blc_adv_buildAdvData(adv_ltvs, ARRAY_SIZE(adv_ltvs), advData);
    //Legacy, Connectable_Scannable, Undirected
    blc_ll_setExtAdvParam( ADV_HANDLE0,         ADV_EVT_PROP_EXTENDED_CONNECTABLE_UNDIRECTED,  ADV_INTERVAL_30MS,           ADV_INTERVAL_35MS,
                           BLT_ENABLE_ADV_ALL,  OWN_ADDRESS_PUBLIC,                                    BLE_ADDR_PUBLIC,                 NULL,
                           ADV_FP_NONE,         TX_POWER_3dBm,                                         BLE_PHY_1M,                      0,
                           BLE_PHY_1M,          ADV_SID_0,                                             0);
    blc_ll_setExtAdvData(ADV_HANDLE0,      advLen,  (u8*)&advData[0]);
    blc_ll_setExtScanRspData(ADV_HANDLE0,  advLen,  (u8*)&advData[0]);
    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE0, 0, 0);

    blc_ll_initPAST_module();
    hci_le_dftPastParamsCmdParams_t pastParam;

    pastParam.mode = PAST_MODE_RPT_ENABLED_DUP_FILTER_DIS;
    pastParam.skip = 0;
    pastParam.syncTimeout = 200;
    pastParam.cteType = PAST_CTE_TYPE_SYNC_TO_WITHOUT_CTE;

    blc_hci_le_setDftPeriodicAdvSyncTransferParams(&pastParam);

    //audio initial profile
    app_audio_init_prf();

    app_codec_init();

    extern void blc_ll_register_bigConflictACL_CB(void);
    extern void blc_ll_changeConflictExpeirdTimeUs(u32 expiredTimeUs);

    blc_ll_register_bigConflictACL_CB();        //only bugfix
    blc_ll_changeConflictExpeirdTimeUs(0);      //50ms acl event will create successful
}

/**
 * @brief       Broadcast sink audio main loop.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_handler(void)
{
    app_audio_receiveHandler();
}

#endif      //SINK_VERSION == SINK_ONLY_VERSION
