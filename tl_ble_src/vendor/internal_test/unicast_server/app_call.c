/********************************************************************************************************
 * @file    app_call.c
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

#if (INTER_TEST_MODE == TEST_CIS_AUDIO_SERVER)

void app_call_event_callback(u16 connHandle, int evtID, u8 *pData, u16 dataLen)
{
    switch(evtID)
    {

        case AUDIO_EVT_GTBS_BEARER_LIST_CURRENT_CALL:
        {
            blc_gtbsc_listCurrentCallsEvt_t *pEvt = (blc_gtbsc_listCurrentCallsEvt_t*)pData;
            blc_gtbsc_list_curr_call_t *pListCurrCalls = NULL;
            u8 callMembersCnt = pEvt->callMembersCnt;
            for(int i = 0; i < callMembersCnt; i++){
                pListCurrCalls = pEvt->pListCurrCalls + i;
                tlk_printf("EVT listCurrentCalls: Call State [%d] Call_Index[0x%x] Call_State[0x%x] Call_Flags[0x%x] Call_URI[%s]", i, pListCurrCalls->callIndex, pListCurrCalls->state, pListCurrCalls->callFlags, pListCurrCalls->pCallUri);
            }
        }
        break;

        case AUDIO_EVT_GTBS_STATUS_FLAGS:
        {
            blc_gtbsc_statusFlagsEvt_t *pEvt = (blc_gtbsc_statusFlagsEvt_t*)pData;
            tlk_printf("EVT Status Flags:[%d]", pEvt->statusFlags);

            switch (pEvt->statusFlags) {
                case 0b00:
                    tlk_printf("        Inband ringtone disabled");
                    tlk_printf("        Server is not in silent mode");
                    break;
                case 0b01:
                    tlk_printf("        Inband ringtone enabled");
                    tlk_printf("        Server is not in silent mode'");
                    break;
                case 0b10:
                    tlk_printf("        Inband ringtone disabled");
                    tlk_printf("        Server is in silent mode");
                    break;
                case 0b11:
                    tlk_printf("        Inband ringtone enabled");
                    tlk_printf("        Server is in silent mode");
                    break;
                default:
                    tlk_printf("        unknown status flags");
                    break;
            }
        }
        break;

        case AUDIO_EVT_GTBS_INCOMING_CALL_TGT_URI:
        {
            blc_gtbsc_incomingCallTgtUriEvt_t *pEvt = (blc_gtbsc_incomingCallTgtUriEvt_t*)pData;
            tlk_printf("EVT Incoming Call Target Bearer URI: Call_Index[0x%x] Call_URI[%s]", pEvt->uri.callIndex, pEvt->uri.info);
        }
        break;

        case AUDIO_EVT_GTBS_CALL_STATE:
        {
            blc_gtbsc_listCallStateEvt_t *pEvt = (blc_gtbsc_listCallStateEvt_t*)pData;
            blc_gtbs_call_state_t *pCallState = NULL;
            u8 callMembersCnt = pEvt->callMembersCnt;
            for(int i = 0; i < callMembersCnt; i++){
                pCallState = pEvt->pCallState + i;
                tlk_printf("EVT Call State: [%d] Call_Index[0x%x] Call_State[0x%x] Call_Flags[0x%x]", i, pCallState->callIndex, pCallState->state, pCallState->callFlags);
            }
        }
        break;

        case AUDIO_EVT_GTBS_TERM_REASON:
        {
            blc_gtbsc_termRsnEvt_t *pTermRsnEvt = (blc_gtbsc_termRsnEvt_t*)pData;
            tlk_printf("EVT Termination Reason: Call_Index[%d] termRsn[%d]", pTermRsnEvt->callIndex, pTermRsnEvt->termRsn);
        }
        break;

        case AUDIO_EVT_GTBS_INCOMING_CALL:
        {
            blc_gtbsc_incomingCallEvt_t *pEvt = (blc_gtbsc_incomingCallEvt_t*)pData;
            tlk_printf("EVT Incoming Call: Call_Index[0x%x] URI[%s]", pEvt->call.callIndex, pEvt->call.info);
        }
        break;

        case AUDIO_EVT_GTBS_CALL_FRIENDLY_NAME:
        {
            blc_gtbsc_friendlyNameEvt_t *pEvt = (blc_gtbsc_friendlyNameEvt_t*)pData;
            tlk_printf("EVT Call Friendly Name: Call_Index[0x%x] URI[%s]", pEvt->name.callIndex, pEvt->name.info);
        }
        break;

        case AUDIO_EVT_GTBS_CCP_NTF_RESULT_CODE:
        {
            blc_gtbsc_ccpNtfResultCodesEvt_t *pEvt = (blc_gtbsc_ccpNtfResultCodesEvt_t*)pData;
            tlk_printf("EVT Call Control Point Notification: Requested Opcode[0x%x] Call_Index[0x%x] Result Code[0x%0x]", pEvt->reqOpcode, pEvt->callIndex, pEvt->resultCode);
            if(pEvt->resultCode == GTBS_NTF_RESULT_CODE_SUCCESS){
                tlk_printf("        The opcode write was successful");
            }
            else{
                tlk_printf("        The opcode write was failed:0x%x", pEvt->resultCode);
            }
        }
        break;

        default:
        break;
    }
}

#endif /* INTER_TEST_MODE */

