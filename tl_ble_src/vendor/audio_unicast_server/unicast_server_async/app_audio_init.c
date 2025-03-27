/********************************************************************************************************
 * @file    app_audio_init.c
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
#include "app_buffer.h"
#include "app_audio.h"
#include "app_audio_ctrl.h"
#if (UNICAST_SERVER_SELECT == UNICAST_SERVER_ASYNC)

extern app_audio_ctrl_t appCtrl;

app_csis_Rsi_t advCsisRsi = {
    .ltv.len = 0x07,
    .ltv.type = DT_CSIP_RSI,
    .rsi = {0},
};

static app_incompleteList_t advIncompleteList = {
    .ltv.len = 0x05,
    .ltv.type = DT_INCOMPLETE_LIST_16BIT_SERVICE_UUID,
    .list = {(SERVICE_UUID_AUDIO_STREAM_CONTROL & 0xFF), (SERVICE_UUID_AUDIO_STREAM_CONTROL & 0xFF00)>>8,\
             (SERVICE_UUID_PUBLISHED_AUDIO_CAPABILITIES & 0xFF), (SERVICE_UUID_PUBLISHED_AUDIO_CAPABILITIES & 0xFF00)>>8},
};

static blc_adv_tmapRole_t advTmapRole = {
    .ltv.len = 0x05,
    .ltv.type = DT_SERVICE_DATA,
    .tamsUuid = SERVICE_UUID_TELEPHONY_AND_MEDIA_AUDIO,
    .tmapRole = BLC_TMAP_ROLE_CALL_TERMINAL | BLC_TMAP_ROLE_UNICAST_MEDIA_RECEIVER,
};

void app_ext_adv_init(void)
{
    blc_csiss_getResolvableSetIdentifier(advCsisRsi.rsi);

    /* Extended ADV module and ADV Set Parameters buffer initialization */
    blc_ll_initExtendedAdvModule_initExtendedAdvSetParamBuffer(app_extAdvSetParam_buf, APP_EXT_ADV_SETS_NUMBER);
    blc_ll_initExtendedAdvDataBuffer(app_extAdvData_buf, APP_EXT_ADV_DATA_LENGTH);

    blc_ll_setExtAdvParam( ADV_HANDLE0,         ADV_EVT_PROP_EXTENDED_CONNECTABLE_UNDIRECTED,                   ADV_INTERVAL_30MS,          ADV_INTERVAL_30MS,
                           BLT_ENABLE_ADV_ALL,  OWN_ADDRESS_PUBLIC,                                             BLE_ADDR_PUBLIC,                NULL,
                           ADV_FP_NONE,         TX_POWER_3dBm,                                                  BLE_PHY_1M,                     0,
                           BLE_PHY_1M,          ADV_SID_0,                                                      0);

    blc_adv_ltv_t *adv_ltvs[] = {
            (blc_adv_ltv_t *) &advDefFlags,
            (blc_adv_ltv_t *) &advDefCompleteName,
            (blc_adv_ltv_t *) &advDefAppearance,
            (blc_adv_ltv_t *) &advIncompleteList,
            (blc_adv_ltv_t *) &advCsisRsi,
            (blc_adv_ltv_t *) &capTargetAnnouncement,
            (blc_adv_ltv_t *) &bapTargetDefAnnouncement,
            (blc_adv_ltv_t *) &advTmapRole,
            };
    u8 advData[255];
    u8 adv_ext_len = blc_adv_buildAdvData(adv_ltvs,ARRAY_SIZE(adv_ltvs),advData);
    blc_ll_setExtAdvData(ADV_HANDLE0, adv_ext_len, advData);
    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE0, 0, 0);
}

//////////// Audio parameters setting  Begin /////////////////////////
//CSIP Set Member parameter configure
blc_csiss_regParam_t csipSetMemberParam =
{
//  .setRank = 0,
//  .setSize = 0,
    .lockedTimeout = 60, //unit:s
    .SIRK_type = BLT_CSIS_PLAIN_TEXT_SIRK,
//  .SIRK = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0f, 0x10},
};

const blc_audio_pacParam_t sinkPac[] = {
    {
        LC3_CAP_16_2(BLC_AUDIO_CHANNEL_COUNTS_1, 1),
        METADATA_CONTEXTS(BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL|BLC_AUDIO_CONTEXT_TYPE_MEDIA),
    },
    {
        LC3_CAP_24_2(BLC_AUDIO_CHANNEL_COUNTS_1, 1),
        METADATA_CONTEXTS(BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL|BLC_AUDIO_CONTEXT_TYPE_MEDIA),
    },
    {
        LC3_CAP_32_2(BLC_AUDIO_CHANNEL_COUNTS_1, 1),
        METADATA_CONTEXTS(BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL|BLC_AUDIO_CONTEXT_TYPE_MEDIA),
    },
    {
        LC3_CAP_48_2(BLC_AUDIO_CHANNEL_COUNTS_1, 1),
        METADATA_CONTEXTS(BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL|BLC_AUDIO_CONTEXT_TYPE_MEDIA),
    },
};

const blc_audio_pacParam_t sourcePac[] = {
    {
        LC3_CAP_16_2(BLC_AUDIO_CHANNEL_COUNTS_1, 1),
        METADATA_CONTEXTS_CONVERSATIONAL,
    },
    {
        LC3_CAP_24_2(BLC_AUDIO_CHANNEL_COUNTS_1, 1),
        METADATA_CONTEXTS_CONVERSATIONAL,
    },
    {
        LC3_CAP_32_2(BLC_AUDIO_CHANNEL_COUNTS_1, 1),
        METADATA_CONTEXTS_CONVERSATIONAL,
    },
};

blc_pacss_regParam_t pacsParam = {
    .sinkPacNum = ARRAY_SIZE(sinkPac),
    .sinkPac = &sinkPac[0],
//  .sinkAudioLocations = 0,
    .sourcePacNum = ARRAY_SIZE(sourcePac),
    .sourcePac = &sourcePac[0],
//  .sourceAudioLocations = 0,
    .availableSinkContexts = AUDIO_UNICAST_SERVER_DEFAULT_CONTEXT,
    .availableSourceContexts = AUDIO_UNICAST_SERVER_DEFAULT_CONTEXT,
    .supportedSinkContexts = AUDIO_UNICAST_SERVER_DEFAULT_CONTEXT,
    .supportedSourceContexts = AUDIO_UNICAST_SERVER_DEFAULT_CONTEXT,
};
blc_bapus_regParam_t unicastSvrParam = {
    .pAscsParam = NULL,
    .pPacsParam = &pacsParam,
};

//VCP Volume Renderer parameter configure
const blc_vcss_regParam_t vcpRendererParam = {
    .vcsParam = {
        .step = 20,
        .volume = 20,
        .mute = false,
    },
};
//TMAS role parameter configure
const blc_tmass_regParam_t tmasParam = {
    .role = BLC_TMAP_ROLE_CALL_TERMINAL | BLC_TMAP_ROLE_UNICAST_MEDIA_RECEIVER,
};
//////////// Audio parameters setting  End /////////////////////////

bool blc_gatts_printfServicesInfo(u16 connHandle)
{
    atts_group_t *pGroup = blc_gatts_getAttributeServiceGroup(connHandle);

    if(pGroup == NULL)
    {
        return false;
    }

    for(; pGroup != NULL; pGroup = pGroup->pNext)
    {
        atts_attribute_t* pAttr = pGroup->pAttr;
        tlkapi_printf(1, "[0x%04X]: S:%04d ~ E:%04d", (*(u16*)pAttr->attrValue), pGroup->startHandle, pGroup->endHandle);
    }

    return true;
}

void app_audio_acceptor_init(void)
{
    /* Register GAP/GATT/DIS/BAS server */
    blc_svc_addCoreGroup();
    blc_svc_addBasGroup();
    
#ifndef __BOOT_SWITCH_APP1__
    blc_svc_addOtaGroup();
#endif

    u8 setRank = (appCtrl.leaRole == ACL_ROLE_PERIPHERAL) ? 2 : 1;
    u32 loc = (appCtrl.leaRole == ACL_ROLE_PERIPHERAL) ? BLC_AUDIO_LOCATION_FLAG_FR : BLC_AUDIO_LOCATION_FLAG_FL;
    pacsParam.sinkAudioLocations = loc;
    pacsParam.sourceAudioLocations = loc;
    csipSetMemberParam.setRank = setRank;
    csipSetMemberParam.setSize = 2;
    smemcpy(csipSetMemberParam.SIRK, appCtrl.sirkCfg, 16);

    //////////// Audio Stream Transitions  Begin /////////////////////////
    blc_audio_registerBapUnicastServer(&unicastSvrParam); //BAP Unicast Server init
    blc_audio_registerCSISControlServer(&csipSetMemberParam); //CSIP Set Member init
    //////////// Audio Stream Transitions  End /////////////////////////


    //////////// Capture and Rendering Control  Begin /////////////////////////
    blc_audio_registerVCSControlServer(&vcpRendererParam); //VCP Volume Renderer init
    blc_audio_registerMICSControlServer(&defaultMicpParam); //MICP Microphone Device init
    //////////// Capture and Rendering Control  End /////////////////////////


    //////////// Content Control  Begin /////////////////////////
    blc_audio_registerCallControlClient(NULL); //CCP Call Control Client init
    blc_audio_registerMediaControlClient(NULL); //MCP Media Control Client init
    //////////// Content Control  End /////////////////////////

    //TMAP Server init
    blc_audio_registerTMASControlServer(&tmasParam);

    /* Initialize EXT ADV module && audio announcement (
     * RSI can only be obtained after audio initialization. */
    app_ext_adv_init();


    blc_gatts_printfServicesInfo(0xFFFF);

}
#endif
