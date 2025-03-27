/********************************************************************************************************
 * @file    app_audio_call.c
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

#if (UNICAST_SERVER_SELECT == UNICAST_SERVER_HEADSET)
extern app_audio_ctrl_t appCtrl;


/**
 * @brief       Call event callback in APP layer,used to inform user about 'call state' and 'call information'
 * @param[in]   connHandle - ACL connect handle.
 * @param[in]   evtID      - Call event ID.
 * @param[in]   pData      - Additional data.
 * @param[in]   dataLen    - Additional data length.
 * @return      none
 */
void app_call_event_callback(u16 connHandle, int evtID, u8 *pData, u16 dataLen)
{
    switch(evtID)
    {

        case AUDIO_EVT_GTBS_BEARER_PROVIDER_NAME:
        {
            blc_gtbsc_bearerProviderName_t *pEvt = (blc_gtbsc_bearerProviderName_t*)pData;
            tlkapi_printf(APP_LOG_EN,"bearer provider name[%.*s]",pEvt->nameLen,pEvt->providerName);
        }
        break;

        case AUDIO_EVT_GTBS_BEARER_TECHNOLOGY:
        {
            blc_gtbsc_technology_t *pEvt = (blc_gtbsc_technology_t*)pData;
            const char *name = blc_gtbs_getBearerTechnologyName((u8)pEvt->technology);
            tlkapi_printf(APP_LOG_EN,"bearer technology:%s",name);
        }
        break;

        case AUDIO_EVT_GTBS_BEARER_URI_SCHEMES_SUPP_LIST:
        {
            blc_gtbsc_uriSchemeSuppList_t *pEvt = (blc_gtbsc_uriSchemeSuppList_t*)pData;
            tlkapi_printf(APP_LOG_EN,"bearer URI Schemes support list[%.*s]",pEvt->suppLen,pEvt->uriSchemeSuppList);
        }
        break;

        case AUDIO_EVT_GTBS_BEARER_SIGNAL_STRENGTH:
        {
            blc_gtbsc_signalStrength_t *pEvt = (blc_gtbsc_signalStrength_t*)pData;
            tlkapi_printf(APP_LOG_EN,"bearer signal strength:%d",pEvt->signalStrength);
        }
        break;

        case AUDIO_EVT_GTBS_BEARER_LIST_CURRENT_CALL:
        {
            blc_gtbsc_listCurrentCallsEvt_t *pEvt = (blc_gtbsc_listCurrentCallsEvt_t*)pData;
            u8 index = 0;
            u8 offset = 0;
            while(offset<pEvt->listLen)
            {
                blc_gtbsc_list_curr_call_t *pCall = (blc_gtbsc_list_curr_call_t *)&pEvt->currentListCall[offset];
                tlkapi_printf(APP_LOG_EN,"current call[%d]",index);
                tlkapi_printf(APP_LOG_EN,"call index[%d],state[%d],callFlags[%d]",pCall->callIndex,pCall->state,pCall->callFlags);
                tlkapi_printf(APP_LOG_EN,"call Uri[%.*s]",(pCall->listItemLen-3),pCall->pCallUri);
                index++;
                offset =offset+pCall->listItemLen+1;
            }
        }
        break;

        case AUDIO_EVT_GTBS_STATUS_FLAGS:
        {
            blc_gtbsc_statusFlagsEvt_t *pEvt = (blc_gtbsc_statusFlagsEvt_t*)pData;
            tlkapi_printf(APP_LOG_EN,"state flags:%s", blc_gtbs_getStatusFlagsDescription(pEvt->statusFlags));
        }
        break;

        case AUDIO_EVT_GTBS_INCOMING_CALL_TGT_URI:
        {
            blc_gtbsc_incomingCallTgtUriEvt_t *pEvt = (blc_gtbsc_incomingCallTgtUriEvt_t*)pData;
            tlk_printf("Call_Index[%d]", pEvt->uri.callIndex);
            tlk_printf("incoming Call Target Bearer URI[%.*s]",pEvt->uriLen,pEvt->uri.info);
        }
        break;

        case AUDIO_EVT_GTBS_CALL_STATE:
        {
            blc_gtbsc_listCallStateEvt_t *pEvt = (blc_gtbsc_listCallStateEvt_t*)pData;
            u8 index = 0;
            u8 offset = 0;
            while(offset<pEvt->stateLen)
            {
                tlk_printf("Call state");
                tlk_printf("Call Index[%d],Call State[%d],Call Flags[%d]",pEvt->state[index].callIndex,pEvt->state[index].state,pEvt->state[index].callFlags);
                offset+=3;
                index++;
            }
        }
        break;

        case AUDIO_EVT_GTBS_TERM_REASON:
        {
            blc_gtbsc_termRsnEvt_t *pTermRsnEvt = (blc_gtbsc_termRsnEvt_t*)pData;
            tlk_printf("Call Index[%d]",pTermRsnEvt->callIndex);
            tlk_printf("Terminate Reason",blt_gtbs_getTerminationReasonName(pTermRsnEvt->termRsn));
        }
        break;

        case AUDIO_EVT_GTBS_INCOMING_CALL:
        {
            blc_gtbsc_incomingCallEvt_t *pEvt = (blc_gtbsc_incomingCallEvt_t*)pData;
            tlk_printf("Call_Index[%d]", pEvt->call.callIndex);
            tlk_printf("incoming Call URI[%.*s]",pEvt->callLen,pEvt->call.info);
        }
        break;

        case AUDIO_EVT_GTBS_CALL_FRIENDLY_NAME:
        {
            blc_gtbsc_friendlyNameEvt_t *pEvt = (blc_gtbsc_friendlyNameEvt_t*)pData;
            tlk_printf("Call_Index[%d]", pEvt->name.callIndex);
            tlk_printf("incoming Call URI[%.*s]",pEvt->nameLen,pEvt->name.info);
        }
        break;

        case AUDIO_EVT_GTBS_CCP_NTF_RESULT_CODE:
        {
            blc_gtbsc_ccpNtfResultCodesEvt_t *pEvt = (blc_gtbsc_ccpNtfResultCodesEvt_t*)pData;
            tlk_printf("Call Control Point Notification: Requested Opcode[0x%x] Call_Index[0x%x] Result Code[0x%0x]", pEvt->reqOpcode, pEvt->callIndex, pEvt->resultCode);
            if(pEvt->resultCode == GTBS_NTF_RESULT_CODE_SUCCESS)
            {
                tlk_printf("        The opcode write was successful");
            }
            else
            {
                tlk_printf("        The opcode write was failed:0x%x", pEvt->resultCode);
            }
        }
        break;

        default:
        break;
    }
}

/**
 * @brief       This function serves to excute the accept operaiton,if success the call will convert to active state.
 * @param[in]   connHandle - ACL connect handle.
 * @return      none
 */
void app_audio_call_accept(u16 connHandle,u8 callIndex)
{
    u16 callCnt;
    blc_gtbs_call_state_t calls[STACK_AUDIO_CALL_MEMBERS_MAX_NUM];
    blc_gtbsc_getCallState(connHandle, &callCnt, calls);

    for(u8 i=0;i<callCnt;i++)
    {
        if(calls[i].callIndex == callIndex)
        {
            if(calls[i].state == GTBS_CALL_STATE_INCOMING)
            {
                blc_gtbsc_writeAcceptIncomingCall(connHandle, callIndex);
                tlk_printf("Accept,Call Index[0x%x]",callIndex);
            }
        }
    }
}

/**
 * @brief       This function serves to terminate the call.
 * @param[in]   connHandle - ACL connect handle.
 * @return      none
 */
void app_audio_call_termiante(u16 connHandle,u8 callIndex)
{
    u16 callCnt;
    blc_gtbs_call_state_t calls[STACK_AUDIO_CALL_MEMBERS_MAX_NUM];
    blc_gtbsc_getCallState(connHandle, &callCnt, calls);

    for(u8 i=0;i<callCnt;i++)
    {
        if(calls[i].callIndex == callIndex)
        {
            blc_gtbsc_writeTerminateCall(connHandle, callIndex);
            tlk_printf("Terminate,Call Index[0x%x]",callIndex);
        }
    }
}
#endif
