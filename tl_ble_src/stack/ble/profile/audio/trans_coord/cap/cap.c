/********************************************************************************************************
 * @file    cap.c
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
#include "cap.h"


const u8 gAppAudioAclMaxNum        = APP_AUDIO_ACL_MAX_CONN;
const u8 gAppAudioAclCentralNum    = ACL_CENTRAL_MAX_NUM;
const u8 gAppAudioAclPeripheralNum = ACL_PERIPHR_MAX_NUM;

//CAP Role Initiator, initialize Content Control, include MCP Media Control Server and CCP Call Control Server.
void blc_cap_initiatorContentCtrl(void)
{
    blc_audio_registerMediaControlServer(NULL); //TODO: current version not support
    blc_audio_registerCallControlServer(NULL);  //TODO: current version not support
}

//CAP Role Initiator, initialize Audio Stream Transitions, include CSIP Set Coordinator and BAP Unicast Client.
void blc_cap_initiatorStreamTrans(void)
{
    blc_audio_registerBapUnicastClient(NULL);  //BAP Unicast Client init
    blc_audio_registerCSISControlClient(NULL); //CSIP Set Coordinator init
}

//CAP Role Commander, initialize Capture and Rendering Control, include VCP Volume Controller and MICP Microphone Controller.
void blc_cap_commanderCaptureRenderingCtrl(void)
{
    blc_audio_registerVCSControlClient(NULL);  //VCP Volume Controller init
    blc_audio_registerMICSControlClient(NULL); //MICP Microphone Controller init
}

//CAP Role Commander, initialize Audio Stream Transitions, include CSIP Set Coordinator and BAP Broadcast Assistant.
void blc_cap_commanderStreamTrans(void)
{
    blc_audio_registerBroadcastAssistant(NULL); //BAP Broadcast Assistant(Scan Delegator) init
    blc_audio_registerCSISControlClient(NULL);  //CSIP Set Coordinator init
}

//CAP Role Acceptor, initialize Content Control, include MCP Media Control Client and CCP Call Control Client.
void blc_cap_acceptorContentCtrl(void)
{
    blc_audio_registerCallControlClient(NULL);  //CCP Call Control Client init
    blc_audio_registerMediaControlClient(NULL); //MCP Media Control Client init
}

//CAP Role Acceptor, initialize Capture and Rendering Control, include VCP Volume Renderer and MICP Microphone Device.
void blc_cap_acceptorCaptureRenderingCtrl(void)
{
    blc_audio_registerVCSControlServer(&defaultVcpRendererParam); //VCP Volume Renderer init
    blc_audio_registerMICSControlServer(&defaultMicpParam);       //MICP Microphone Device init
}

//CAP Role Acceptor, initialize Audio Stream Transitions, include CSIP Set Member, BAP Unicast Server, BAP Broadcast Sink.
//Only Unicast Audio Audio Stream Transitions
const blc_bapus_regParam_t defaultUnicastSvrParam =
    {
        .pAscsParam = NULL,
        .pPacsParam = &defaultPacsParam,
};

void blc_cap_acceptorUnicastStreamTrans(void)
{
    blc_audio_registerBapUnicastServer(&defaultUnicastSvrParam);     //BAP Unicast Server init
    blc_audio_registerCSISControlServer(&defaultCsipSetMemberParam); //CSIP Set Member init
}

//Only Broadcast Audio Audio Stream Transitions
const blc_bapbs_regParam_t defaultBcstSinkParam = {
    .pBassParam = NULL,
    .pPacsParam = &defaultPacsParam,
};

void blc_cap_acceptorBcstStreamTrans(void)
{
    blc_audio_registerBapBroadcastSink(&defaultBcstSinkParam);       //BAP Broadcast Sink(Scan Delegator) init
    blc_audio_registerCSISControlServer(&defaultCsipSetMemberParam); //CSIP Set Member init
}

//Audio Audio Stream Transitions include Unicast and Broadcast
void blc_cap_acceptorStreamTrans(void)
{
    blc_cap_acceptorUnicastStreamTrans();
    blc_cap_acceptorBcstStreamTrans();
}

void blc_cap_initUnicastInitiator(void)
{
    //////////// Unicast Audio Stream Transitions /////////////////////////
    blc_cap_initiatorStreamTrans();

    //////////// Capture and Rendering Control( * Commander support VCP Volume Controller) /////////////////////////
    blc_cap_commanderCaptureRenderingCtrl();
}

void blc_cap_initBcstCommander(void)
{
    //////////// Broadcast Audio Stream Transitions /////////////////////////
    blc_cap_commanderStreamTrans();

    //////////// Capture and Rendering Control( *Broadcast Commander only support VCP Volume Controller) /////////////////////////
    blc_audio_registerVCSControlClient(NULL); //VCP Volume Controller init
}

void blc_cap_initUnicastAcceptor(void)
{
    ////////////Unicast Audio Stream Transitions/////////////////////////
    blc_cap_acceptorUnicastStreamTrans();

    blc_cap_acceptorContentCtrl();

    blc_cap_acceptorCaptureRenderingCtrl();
}

void blc_cap_initBcstAcceptor(void)
{
    ////////////Broadcast Audio Stream Transitions/////////////////////////
    blc_cap_acceptorBcstStreamTrans();

    blc_cap_acceptorCaptureRenderingCtrl();
}

void blc_cap_initAudioAcceptor(void)
{
    blc_cap_acceptorStreamTrans();

    blc_cap_acceptorContentCtrl();

    blc_cap_acceptorCaptureRenderingCtrl();
}

const blc_capAnnouncement_t capGeneralAnnouncement = {
    .ltv.len          = 4,
    .ltv.type         = DT_SERVICE_DATA,
    .casUuid          = SERVICE_UUID_COMMON_AUDIO,
    .announcementType = BLC_AUDIO_GENERAL_ANNOUNCEMENT,
};

const blc_capAnnouncement_t capTargetAnnouncement = {
    .ltv.len          = 4,
    .ltv.type         = DT_SERVICE_DATA,
    .casUuid          = SERVICE_UUID_COMMON_AUDIO,
    .announcementType = BLC_AUDIO_TARGETED_ANNOUNCEMENT,
};
