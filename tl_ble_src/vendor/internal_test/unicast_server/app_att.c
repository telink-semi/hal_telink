/********************************************************************************************************
 * @file    app_att.c
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
#include "app_att.h"

#if (INTER_TEST_MODE == TEST_CIS_AUDIO_SERVER)

extern app_audio_ctrl_t appCtrl;

void app_ext_adv_init(void)
{
    blc_csiss_getResolvableSetIdentifier(appCtrl.rsi);

    /* Extended ADV module and ADV Set Parameters buffer initialization */
    blc_ll_initExtendedAdvModule_initExtendedAdvSetParamBuffer(app_extAdvSetParam_buf, APP_EXT_ADV_SETS_NUMBER);
    blc_ll_initExtendedAdvDataBuffer(app_extAdvData_buf, APP_EXT_ADV_DATA_LENGTH);

    blc_ll_setExtAdvParam(ADV_HANDLE0, ADV_EVT_PROP_EXTENDED_CONNECTABLE_UNDIRECTED, ADV_INTERVAL_30MS, ADV_INTERVAL_30MS, BLT_ENABLE_ADV_ALL, OWN_ADDRESS_PUBLIC, BLE_ADDR_PUBLIC, NULL, ADV_FP_NONE, TX_POWER_3dBm, BLE_PHY_1M, 0, BLE_PHY_1M, ADV_SID_0, 0);
    const u8 audioAnnouncementAdvData[] =
        {
            13,
            DT_COMPLETE_LOCAL_NAME,
            't',
            'l',
            'k',
            '_',
            'l',
            'e',
            '_',
            'a',
            'u',
            'd',
            'i',
            'o',
            2,
            DT_FLAGS,
            0x05,                                  // BLE limited discoverable mode and BR/EDR not supported
            5,
            DT_INCOMPLETE_LIST_16BIT_SERVICE_UUID, // incomplete list of service class UUIDs (0x184F, 0x1850)
            (SERVICE_UUID_AUDIO_STREAM_CONTROL & 0xFF),
            (SERVICE_UUID_AUDIO_STREAM_CONTROL & 0xFF00) >> 8,
            (SERVICE_UUID_PUBLISHED_AUDIO_CAPABILITIES & 0xFF),
            (SERVICE_UUID_PUBLISHED_AUDIO_CAPABILITIES & 0xFF00) >> 8,
            3,
            DT_APPEARANCE,
            (DEFAULT_DEV_APPEARE & 0xFF),
            (DEFAULT_DEV_APPEARE & 0xFF00) >> 8,
            7,
            DT_CSIP_RSI,
            appCtrl.rsi[0],
            appCtrl.rsi[1],
            appCtrl.rsi[2],
            appCtrl.rsi[3],
            appCtrl.rsi[4],
            appCtrl.rsi[5],
            /////////////////// (CAP, BAP, TMAP Announcement) Unicast Server AD format when connectable and available to receive or transmit audio data /////////////////
            4,
            DT_SERVICE_DATA,
            //------------ Common Audio Service UUID ------------
            (SERVICE_UUID_COMMON_AUDIO & 0xFF),
            (SERVICE_UUID_COMMON_AUDIO & 0xFF00) >> 8,
            //------------ Announcement Type ------------
            BLC_AUDIO_TARGETED_ANNOUNCEMENT,

            17,
            DT_SERVICE_DATA,
            //------------ Audio Stream Control Service UUID ------------
            (SERVICE_UUID_AUDIO_STREAM_CONTROL & 0xFF),
            (SERVICE_UUID_AUDIO_STREAM_CONTROL & 0xFF00) >> 8,
            //------------ Announcement Type ------------
            BLC_AUDIO_TARGETED_ANNOUNCEMENT,
            //------------ Available Audio Contexts ----------------
            (APP_AUDIO_SUPPORTED_CONTEXTS & 0xFF),
            (APP_AUDIO_SUPPORTED_CONTEXTS & 0xFF00) >> 8, //sinkContext
            (APP_AUDIO_SUPPORTED_CONTEXTS & 0xFF),
            (APP_AUDIO_SUPPORTED_CONTEXTS & 0xFF00) >> 8, //sourceContext
            //------------ Metadata_Length, and Metadata ----------------
            0x08, //Metadata_Length
            //     Preferred_Audio_Contexts
            0x03,
            BLC_AUDIO_METATYPE_PREFERRED_CONTEXTS,
            (APP_AUDIO_PREFERRED_CONTEXTS & 0xFF),
            (APP_AUDIO_PREFERRED_CONTEXTS & 0xFF00) >> 8,
            //     Streaming_Audio_Contexts
            0x03,
            BLC_AUDIO_METATYPE_STREAMING_CONTEXTS,
            (APP_AUDIO_STREAMING_CONTEXTS & 0xFF),
            (APP_AUDIO_STREAMING_CONTEXTS & 0xFF00) >> 8,
            5,
            DT_SERVICE_DATA,
            //------------ TMA Service UUID ------------
            U16_LO(SERVICE_UUID_TELEPHONY_AND_MEDIA_AUDIO),
            U16_HI(SERVICE_UUID_TELEPHONY_AND_MEDIA_AUDIO),
            //------------
            BLC_TMAP_ROLE_CALL_TERMINAL | BLC_TMAP_ROLE_UNICAST_MEDIA_RECEIVER,
            0x00,
        };
    blc_ll_setExtAdvData(ADV_HANDLE0, sizeof(audioAnnouncementAdvData), (u8 *)audioAnnouncementAdvData);
    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE0, 0, 0);
}

    #if (APP_SCENE == APP_SCENE_TWS)
        //////////// Audio parameters setting  Begin /////////////////////////
        #define CISP_SET_MEMBER_RANK_ID 2 //1 OR 2
        #if (CISP_SET_MEMBER_RANK_ID == 1)
            #define AUDIO_LOCATION_FLAG_USED BLC_AUDIO_LOCATION_FLAG_FL
        #elif (CISP_SET_MEMBER_RANK_ID == 2)
            #define AUDIO_LOCATION_FLAG_USED BLC_AUDIO_LOCATION_FLAG_FR
        #else
            #error "Acceptor CSIP set member rank ID error(range:1 ~ 2)"
        #endif

//Unicast server parameter configure
const blc_audio_pacParam_t sinkPac = {
    .codecId = {
                .id        = 0x06,
                .companyID = 0x0000,
                .vendorID  = 0x0000,
                },
    .codecSpec = {
                .samplingFreq     = BLC_AUDIO_SUPP_FREQ_FLAG_16000 | BLC_AUDIO_SUPP_FREQ_FLAG_24000 | BLC_AUDIO_SUPP_FREQ_FLAG_32000 | BLC_AUDIO_SUPP_FREQ_FLAG_48000,
                .frameDurations   = BLC_AUDIO_SUPP_DURATION_FLAG_10 | BLC_AUDIO_SUPP_DURATION_FLAG_7_5,
                .channelCounts    = BLC_AUDIO_CHANNEL_COUNTS_1,
                .minPerCodecFrame = 0x0010,
                .maxPerCodecFrame = 0x00F0,
                .maxPerSdu        = 1,
                },
    .metadata = {
                .preferredContexts = BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BLC_AUDIO_CONTEXT_TYPE_MEDIA,
                .StreamingContexts = BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BLC_AUDIO_CONTEXT_TYPE_MEDIA,
                }
};
const blc_audio_pacParam_t sourcePac = {
    .codecId = {
                .id        = 0x06,
                .companyID = 0x0000,
                .vendorID  = 0x0000,
                },
    .codecSpec = {
                .samplingFreq     = BLC_AUDIO_SUPP_FREQ_FLAG_16000 | BLC_AUDIO_SUPP_FREQ_FLAG_24000 | BLC_AUDIO_SUPP_FREQ_FLAG_32000 | BLC_AUDIO_SUPP_FREQ_FLAG_48000,
                .frameDurations   = BLC_AUDIO_SUPP_DURATION_FLAG_10 | BLC_AUDIO_SUPP_DURATION_FLAG_7_5,
                .channelCounts    = BLC_AUDIO_CHANNEL_COUNTS_1,
                .minPerCodecFrame = 0x0010,
                .maxPerCodecFrame = 0x00F0,
                .maxPerSdu        = 1,
                },
    .metadata = {
                .preferredContexts = BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BLC_AUDIO_CONTEXT_TYPE_MEDIA,
                .StreamingContexts = BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BLC_AUDIO_CONTEXT_TYPE_MEDIA,
                }
};
//CSIP Set Member parameter configure
const blc_csiss_regParam_t csipSetMemberParam =
    {
        .setRank       = CISP_SET_MEMBER_RANK_ID,
        .setSize       = 2,
        .lockedTimeout = 60, //unit:s
        .SIRK_type     = BLT_CSIS_PLAIN_TEXT_SIRK,
        .SIRK          = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0f, 0x10},
};
    #elif (APP_SCENE == APP_SCENE_HEADSET_EP1_MULTIPLEXING)
        //////////// Audio parameters setting  Begin /////////////////////////
        #define CISP_SET_MEMBER_RANK_ID  1 //1 OR 2
        #define AUDIO_LOCATION_FLAG_USED BLC_AUDIO_LOCATION_FLAG_FR | BLC_AUDIO_LOCATION_FLAG_FL


//Unicast server parameter configure
const blc_audio_pacParam_t sinkPac = {
    .codecId = {
                .id        = 0x06,
                .companyID = 0x0000,
                .vendorID  = 0x0000,
                },
    .codecSpec = {
                .samplingFreq     = BLC_AUDIO_SUPP_FREQ_FLAG_16000 | BLC_AUDIO_SUPP_FREQ_FLAG_24000 | BLC_AUDIO_SUPP_FREQ_FLAG_32000 | BLC_AUDIO_SUPP_FREQ_FLAG_48000,
                .frameDurations   = BLC_AUDIO_SUPP_DURATION_FLAG_10 | BLC_AUDIO_SUPP_DURATION_FLAG_7_5,
                .channelCounts    = BLC_AUDIO_CHANNEL_COUNTS_2,
                .minPerCodecFrame = 0x0010,
                .maxPerCodecFrame = 0x00F0,
                .maxPerSdu        = 2,
                },
    .metadata = {
                .preferredContexts = BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BLC_AUDIO_CONTEXT_TYPE_MEDIA,
                .StreamingContexts = BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BLC_AUDIO_CONTEXT_TYPE_MEDIA,
                }
};
const blc_audio_pacParam_t sourcePac = {
    .codecId = {
                .id        = 0x06,
                .companyID = 0x0000,
                .vendorID  = 0x0000,
                },
    .codecSpec = {
                .samplingFreq     = BLC_AUDIO_SUPP_FREQ_FLAG_16000 | BLC_AUDIO_SUPP_FREQ_FLAG_24000 | BLC_AUDIO_SUPP_FREQ_FLAG_32000 | BLC_AUDIO_SUPP_FREQ_FLAG_48000,
                .frameDurations   = BLC_AUDIO_SUPP_DURATION_FLAG_10 | BLC_AUDIO_SUPP_DURATION_FLAG_7_5,
                .channelCounts    = BLC_AUDIO_CHANNEL_COUNTS_2,
                .minPerCodecFrame = 0x0010,
                .maxPerCodecFrame = 0x00F0,
                .maxPerSdu        = 2,
                },
    .metadata = {
                .preferredContexts = BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BLC_AUDIO_CONTEXT_TYPE_MEDIA,
                .StreamingContexts = BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BLC_AUDIO_CONTEXT_TYPE_MEDIA,
                }
};
//CSIP Set Member parameter configure
const blc_csiss_regParam_t csipSetMemberParam =
    {
        .setRank       = CISP_SET_MEMBER_RANK_ID,
        .setSize       = 1,
        .lockedTimeout = 60, //unit:s
        .SIRK_type     = BLT_CSIS_PLAIN_TEXT_SIRK,
        .SIRK          = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0f, 0x10},
};
    #elif (APP_SCENE == APP_SCENE_HEADSET_EP2)
        //////////// Audio parameters setting  Begin /////////////////////////
        #define CISP_SET_MEMBER_RANK_ID  1
        #define AUDIO_LOCATION_FLAG_USED BLC_AUDIO_LOCATION_FLAG_FL | BLC_AUDIO_LOCATION_FLAG_FR


//Unicast server parameter configure
const blc_audio_pacParam_t sinkPac = {
    .codecId = {
                .id        = 0x06,
                .companyID = 0x0000,
                .vendorID  = 0x0000,
                },
    .codecSpec = {
                .samplingFreq     = BLC_AUDIO_SUPP_FREQ_FLAG_16000 | BLC_AUDIO_SUPP_FREQ_FLAG_24000 | BLC_AUDIO_SUPP_FREQ_FLAG_32000 | BLC_AUDIO_SUPP_FREQ_FLAG_48000,
                .frameDurations   = BLC_AUDIO_SUPP_DURATION_FLAG_10 | BLC_AUDIO_SUPP_DURATION_FLAG_7_5,
                .channelCounts    = BLC_AUDIO_CHANNEL_COUNTS_1,
                .minPerCodecFrame = 0x0010,
                .maxPerCodecFrame = 0x00F0,
                .maxPerSdu        = 1,
                },
    .metadata = {
                .preferredContexts = BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BLC_AUDIO_CONTEXT_TYPE_MEDIA,
                .StreamingContexts = BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BLC_AUDIO_CONTEXT_TYPE_MEDIA,
                }
};
const blc_audio_pacParam_t sourcePac = {
    .codecId = {
                .id        = 0x06,
                .companyID = 0x0000,
                .vendorID  = 0x0000,
                },
    .codecSpec = {
                .samplingFreq     = BLC_AUDIO_SUPP_FREQ_FLAG_16000 | BLC_AUDIO_SUPP_FREQ_FLAG_24000 | BLC_AUDIO_SUPP_FREQ_FLAG_32000 | BLC_AUDIO_SUPP_FREQ_FLAG_48000,
                .frameDurations   = BLC_AUDIO_SUPP_DURATION_FLAG_10 | BLC_AUDIO_SUPP_DURATION_FLAG_7_5,
                .channelCounts    = BLC_AUDIO_CHANNEL_COUNTS_1,
                .minPerCodecFrame = 0x0010,
                .maxPerCodecFrame = 0x00F0,
                .maxPerSdu        = 1,
                },
    .metadata = {
                .preferredContexts = BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BLC_AUDIO_CONTEXT_TYPE_MEDIA,
                .StreamingContexts = BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BLC_AUDIO_CONTEXT_TYPE_MEDIA,
                }
};
//CSIP Set Member parameter configure
const blc_csiss_regParam_t csipSetMemberParam =
    {
        .setRank       = CISP_SET_MEMBER_RANK_ID,
        .setSize       = 1,
        .lockedTimeout = 60, //unit:s
        .SIRK_type     = BLT_CSIS_PLAIN_TEXT_SIRK,
        .SIRK          = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0f, 0x10},
};
    #endif

const blc_pacss_regParam_t pacsParam = {
    .sinkPacNum              = 1,
    .sinkPac                 = &sinkPac,
    .sinkAudioLocations      = AUDIO_LOCATION_FLAG_USED,
    .sourcePacNum            = 1,
    .sourcePac               = &sourcePac,
    .sourceAudioLocations    = AUDIO_LOCATION_FLAG_USED,
    .availableSinkContexts   = BLC_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BLC_AUDIO_CONTEXT_TYPE_MEDIA,
    .availableSourceContexts = BLC_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BLC_AUDIO_CONTEXT_TYPE_MEDIA,
    .supportedSinkContexts   = BLC_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BLC_AUDIO_CONTEXT_TYPE_MEDIA,
    .supportedSourceContexts = BLC_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BLC_AUDIO_CONTEXT_TYPE_MEDIA,
};
const blc_bapus_regParam_t unicastSvrParam = {
    .pAscsParam = NULL,
    .pPacsParam = &pacsParam,
};

//VCP Volume Renderer parameter configure
const blc_vcss_regParam_t vcpRendererParam = {
    .vcsParam = {
                 .step   = 20,
                 .volume = 20,
                 .mute   = false,
                 },
};
//TMAS role parameter configure
const blc_tmass_regParam_t tmasParam = {
    .role = BLC_TMAP_ROLE_CALL_TERMINAL | BLC_TMAP_ROLE_UNICAST_MEDIA_RECEIVER,
};

//////////// Audio parameters setting  End /////////////////////////

void app_audio_acceptor_init(void)
{
    /* Register GAP/GATT/DIS/BAS server */
    blc_svc_addCoreGroup();
    blc_svc_addDisGroup();
    blc_svc_addBasGroup();


    //////////// Audio Stream Transitions  Begin /////////////////////////
    blc_audio_registerBapUnicastServer(&unicastSvrParam);     //BAP Unicast Server init
    blc_audio_registerCSISControlServer(&csipSetMemberParam); //CSIP Set Member init
    //////////// Audio Stream Transitions  End /////////////////////////


    //////////// Capture and Rendering Control  Begin /////////////////////////
    blc_audio_registerVCSControlServer(&vcpRendererParam);  //VCP Volume Renderer init
    blc_audio_registerMICSControlServer(&defaultMicpParam); //MICP Microphone Device init
    //////////// Capture and Rendering Control  End /////////////////////////


    //////////// Content Control  Begin /////////////////////////
    blc_audio_registerCallControlClient(NULL);  //CCP Call Control Client init
    blc_audio_registerMediaControlClient(NULL); //MCP Media Control Client init
    //////////// Content Control  End /////////////////////////

    //TMAP Server init
    blc_audio_registerTMASControlServer(&tmasParam);

    /* Initialize EXT ADV module && audio announcement (
     * RSI can only be obtained after audio initialization. */
    app_ext_adv_init();
}


#endif /* INTER_TEST_MODE */
