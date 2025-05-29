/********************************************************************************************************
 * @file    esl.c
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

#define STATE_TIMEOUT_MSEC (60 * 60 * 1000)

typedef void (*eslTimerCallback_t)(void);

typedef enum
{
    ESL_PA_STATE_IDLE,
    ESL_PA_STATE_ESTABLISHED,
} eslPaState_t;

typedef struct
{
    bool active;
    u32  responsesMask;
    u8   responses[MAX_ESL_PAYLOAD_SIZE];
    u8   numResponses;
    u16  syncHandle;
    u16  evtCounter;
    u8   subevt;
    u8   rspSlot;
} electronicShelfLabelPendingCommand_t;

typedef struct
{
    bool                                 eslAddressPresent             : 1;
    bool                                 eslResponseKeyMaterialPresent : 1;
    bool                                 apSyncKeyMaterialPresent      : 1;
    bool                                 eslCurrentAbsoluteTimePresent : 1;
    bool                                 disconnectAfterPASync         : 1;
    bool                                 updateCompleteDone            : 1;
    blc_esls_eslAddress_t                eslAddress;
    blc_aes_ccm_crypt_t                  apSyncCcmCrypt;
    blc_aes_ccm_crypt_t                  eslResponseCcmCrypt;
    blc_esls_state_t                     state;
    eslPaState_t                         paState;
    u16                                  paSyncHandle;
    u8                                   paSubevent;
    u16                                  apConnHandle;
    electronicShelfLabelPendingCommand_t pendingCommand;
    u32                                  tick;
    u32                                  timeoutInMsec;
    eslTimerCallback_t                   timerCb;
} electronicShelfLabelState_t;

_attribute_data_retention_ static electronicShelfLabelState_t eslState;

static void blt_esl_updateState(u16 connHandle, blc_esls_state_t newState);

static bool blt_esl_configuration_complete(void)
{
    return (eslState.apSyncKeyMaterialPresent && eslState.eslCurrentAbsoluteTimePresent && eslState.eslAddressPresent && eslState.eslCurrentAbsoluteTimePresent);
}

static void blt_esl_unsynchronize(void)
{
    if (eslState.paState == ESL_PA_STATE_ESTABLISHED) {
        blc_ll_periodicAdvertisingTerminateSync(eslState.paSyncHandle);
        eslState.paState = ESL_PA_STATE_IDLE;
    }

    blt_esl_updateState(0xFFFF, BLC_ESLS_STATE_UNSYNCHRONIZED);
}

static void blt_esl_unassociate(void)
{
    eslState.eslAddressPresent             = false;
    eslState.eslResponseKeyMaterialPresent = false;
    eslState.apSyncKeyMaterialPresent      = false;
    eslState.eslCurrentAbsoluteTimePresent = false;
    memset(&eslState.apSyncCcmCrypt, 0, sizeof(eslState.apSyncCcmCrypt));
    memset(&eslState.eslResponseCcmCrypt, 0, sizeof(eslState.eslResponseCcmCrypt));

    blt_esl_updateState(0xFFFF, BLC_ESLS_STATE_UNASSOCIATED);
}

void blc_eslp_esl_unassociate(void)
{
    if (eslState.paState == ESL_PA_STATE_ESTABLISHED) {
        blc_ll_periodicAdvertisingTerminateSync(eslState.paSyncHandle);
        BLT_ESL_LOG("Cancel PA SYNC...\r\n");
        eslState.paState = ESL_PA_STATE_IDLE;
    }

    blt_esl_unassociate();
}

void blc_eslp_esl_loop(void)
{
    if (eslState.timeoutInMsec && clock_time_exceed(eslState.tick, 1000)) {
        u32 last_tick = eslState.tick;
        u32 msec_passed;

        eslState.tick = clock_time();

        msec_passed = ((eslState.tick - last_tick) / (SYSTEM_TIMER_TICK_1US * 1000));
        if (msec_passed >= eslState.timeoutInMsec) {
            eslState.timeoutInMsec = 0;
            eslState.timerCb();
        } else {
            eslState.timeoutInMsec -= msec_passed;
        }
    }
}

static void blt_esl_timerReload(u32 timeoutInMsec, eslTimerCallback_t cb)
{
    eslState.tick          = clock_time();
    eslState.timeoutInMsec = timeoutInMsec;
    eslState.timerCb       = cb;
}

static void blt_esl_timerStop(void)
{
    eslState.timeoutInMsec = 0;
}

static void blt_esl_updateState(u16 connHandle, blc_esls_state_t newState)
{
    blc_esls_state_t        prevState;
    blc_eslp_esl_stateEvt_t evt;

    prevState = eslState.state;
    if (prevState == newState) {
        return;
    }

    eslState.state = newState;

    if (eslState.state == BLC_ESLS_STATE_UNASSOCIATED) {
        blc_eslss_clearEslId(connHandle);
        blt_esl_timerStop();
    } else if (eslState.state == BLC_ESLS_STATE_UNSYNCHRONIZED) {
        /*
         * ESLS spec 2.7.3.5. Unsynchroznized state "If the ESL is not moved to the
         * Updating state for 60 minutes, then the ESL shall transition to the
         * Unassociated state"
         */
        blt_esl_timerReload(STATE_TIMEOUT_MSEC, blt_esl_unassociate);
    } else if (eslState.state == BLC_ESLS_STATE_SYNCHRONIZED) {
        /*
         * ESLS spec 2.7.3.5. Unsynchroznized state "If the ESL is not moved to the
         * Updating state for 60 minutes, then the ESL shall transition to the
         * Unassociated state"
         */
        blt_esl_timerReload(STATE_TIMEOUT_MSEC, blt_esl_unsynchronize);
    } else {
        blt_esl_timerStop();
    }

    BLT_ESL_LOG("State %d -> %d", prevState, eslState.state);

    evt.state = eslState.state;
    blt_prf_sendEvent(connHandle, ESL_EVT_ESLP_ESL_STATE, (u8 *)&evt, sizeof(evt));
}

static void blt_eslControlPointCb(u16 connHandle, blc_eslss_controlPointCommandHdr_t *cmd)
{
    switch (cmd->opcode) {
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_PING:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_SERVICE_RESET:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_READ_SENSOR_DATA:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_REFRESH_DISPLAY:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_DISPLAY_IMAGE:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_DISPLAY_TIMED_IMAGE:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_LED_CONTROL:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_LED_TIMED_CONTROL:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_0:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_1:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_2:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_3:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_4:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_5:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_6:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_7:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_8:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_9:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_A:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_B:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_C:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_D:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_E:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_F:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_FACTORY_RESET:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_UNASSOCIATE_FROM_AP:
    {
        u8                                  buffer[sizeof(blc_eslp_esl_controlPointCommant_t) + BLC_ESLS_CMD_RSP_MAX_LENGTH];
        blc_eslp_esl_controlPointCommant_t *evt    = (blc_eslp_esl_controlPointCommant_t *)buffer;
        u16                                 cmdLen = blc_esl_getCommandSize(cmd);

        evt->handle.type       = BLC_ESL_TRANSPORT_TYPE_ACL;
        evt->handle.connHandle = connHandle;
        evt->numCmds           = 1;
        memcpy(evt->cmds, cmd, cmdLen);

        blt_prf_sendEvent(connHandle, ESL_EVT_ESLP_ESL_CONTROL_POINT_CMD, (u8 *)evt, sizeof(*evt) + cmdLen);
        break;
    }
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_UPDATE_COMPLETE:
    {
        if (!blt_esl_configuration_complete()) {
            break;
        }

        if (eslState.paState == ESL_PA_STATE_ESTABLISHED) {
            // ESLP specification 5.4 Updating state "If an ESL receives the Update
            // Complete command and it is synchronized, the ESL shall immediately
            // terminate the ACL connection and transition to the Synchronized state."
            blc_ll_disconnect(connHandle, HCI_ERR_REMOTE_USER_TERM_CONN);
            eslState.updateCompleteDone = true;
        } else {
            // ESLP specification 5.4 Updating state "If an ESL receives the
            // Update Complete command and it is not synchronized, the ESL shall wait
            // for synchronization to be established and then terminate the ACL
            // connection and transition to the Synchronized state."
            eslState.disconnectAfterPASync = true;
        }
        break;
    }
    default:
        break;
    }
}

static void blt_eslCb(u16 connHandle, int evtID, u8 *data, u16 dataLen)
{
    (void)dataLen;

    switch (evtID) {
    case ESL_EVT_ESLSS_ESL_ADDRESS:
    {
        blc_eslss_eslAddressEvt_t *pEvt = (blc_eslss_eslAddressEvt_t *)data;
        blc_eslp_esl_addressEvt_t  evt  = {
              .address = pEvt->eslAddress,
        };

        eslState.eslAddressPresent = true;
        eslState.eslAddress        = pEvt->eslAddress;

        blc_eslss_setEslId(connHandle, pEvt->eslAddress.eslId);

        /*
         * In case we are reconfigured - drop pa sync if we are not synced to another subevent.
         * TODO: Check if we can configure subevent instead of dropping sync
         */
        if ((eslState.paState == ESL_PA_STATE_ESTABLISHED) && (eslState.paSubevent != pEvt->eslAddress.groupId)) {
            BLT_ESL_LOG("Change group, cancel PA SYNC...\r\n");

            blc_ll_periodicAdvertisingTerminateSync(eslState.paSyncHandle);
            eslState.paState = ESL_PA_STATE_IDLE;
        }

        blt_prf_sendEvent(connHandle, ESL_EVT_ESLP_ESL_ADDRESS, (u8 *)&evt, sizeof(evt));

        break;
    }
    case ESL_EVT_ESLSS_AP_SYNC_KEY_MATERIAL:
    {
        blc_eslss_apSyncKeyMaterialEvt_t *pEvt = (blc_eslss_apSyncKeyMaterialEvt_t *)data;
        blc_eslp_esl_keyMaterialEvt_t     evt  = {
                 .key = pEvt->apSyncKeyMaterial,
        };

        eslState.apSyncKeyMaterialPresent = true;
        blt_crypto_init_ccm_adv(pEvt->apSyncKeyMaterial.sessionKey, pEvt->apSyncKeyMaterial.IV, &eslState.apSyncCcmCrypt);

        blt_prf_sendEvent(connHandle, ESL_EVT_ESLP_ESL_AP_SYNC_KEY_MATERIAL, (u8 *)&evt, sizeof(evt));

        break;
    }
    case ESL_EVT_ESLSS_ESL_RESPONSE_KEY_MATERIAL:
    {
        blc_eslss_eslResponseKeyMaterialEvt_t *pEvt = (blc_eslss_eslResponseKeyMaterialEvt_t *)data;
        blc_eslp_esl_keyMaterialEvt_t          evt  = {
                      .key = pEvt->eslResponseKeyMaterial,
        };

        eslState.eslResponseKeyMaterialPresent = true;
        blt_crypto_init_ccm_adv(pEvt->eslResponseKeyMaterial.sessionKey, pEvt->eslResponseKeyMaterial.IV, &eslState.eslResponseCcmCrypt);

        blt_prf_sendEvent(connHandle, ESL_EVT_ESLP_ESL_ESL_RESPONSE_KEY_MATERIAL, (u8 *)&evt, sizeof(evt));

        break;
    }
    case ESL_EVT_ESLSS_ESL_CURRENT_ABSOLUTE_TIME:
    {
        blc_eslss_eslCurrentAbsoluteTimeEvt_t *pEvt = (blc_eslss_eslCurrentAbsoluteTimeEvt_t *)data;
        blc_eslp_esl_currentAbsoluteTimeEvt_t  evt  = {
              .time = pEvt->eslCurrentAbsoluteTime,
        };

        eslState.eslCurrentAbsoluteTimePresent = true;

        blt_prf_sendEvent(connHandle, ESL_EVT_ESLP_ESL_CURRENT_ABSOLUTE_TIME, (u8 *)&evt, sizeof(evt));

        break;
    }
    case ESL_EVT_ESLSS_ESL_CONTROL_POINT_COMMAND:
    {
        bls_eslss_elsControlPointCommandEvt_t *pEvt = (bls_eslss_elsControlPointCommandEvt_t *)data;

        blt_eslControlPointCb(connHandle, (blc_eslss_controlPointCommandHdr_t *)pEvt->cmd);

        break;
    }
    default:
        break;
    }
}

void blc_eslp_esl_registerElectronicShelfLabel(const void *param)
{
    const electronicShelfLabelParam_t *eslParam = (const electronicShelfLabelParam_t *)param;
    memset(&eslState, 0, sizeof(eslState));
    eslState.state        = BLC_ESLS_STATE_UNASSOCIATED;
    eslState.paState      = ESL_PA_STATE_IDLE;
    eslState.apConnHandle = 0xFFFF;

    blc_esl_registerESLSControlServer(eslParam->eslssParam);
    blc_eslss_setElectronicShelfLabelCback(blt_eslCb);
}

bool ble_eslp_esl_is_pa_synchronized(void)
{
    return eslState.paState == ESL_PA_STATE_ESTABLISHED;
}

static bool blc_eslp_eslDecryptAdvData(u8 *encData, u8 encDataLen, u8 *outData, u8 *outDataLen)
{
    if (!eslState.apSyncKeyMaterialPresent) {
        return false;
    }

    return blt_crypto_ccm_dec_adv(encData, encDataLen, &eslState.apSyncCcmCrypt, outData, outDataLen) == 0;
}

static bool blc_eslp_eslEncryptEslResponse(u8 randomizer[5], u8 *rawData, u8 rawDataLen, u8 *encData, u8 *encDataLen)
{
    if (!eslState.eslResponseKeyMaterialPresent) {
        return false;
    }

    blt_crypto_ccm_enc_adv(randomizer, rawData, rawDataLen, &eslState.eslResponseCcmCrypt, encData, encDataLen);

    return true;
}

static u16 createPawrResponse(u8 *payload, u16 len, u8 numRsp, blc_eslss_controlPointResponseHdr_t *rsp)
{
    // ESL payload + randomizer + MIC + [length, ESL_TAG] + [length, ENC_ADV_TAG]
    blc_eslss_controlPointResponseHdr_t *cachedRsp     = (blc_eslss_controlPointResponseHdr_t *)eslState.pendingCommand.responses;
    u8                                   eslPayloadLen = 0;
    u16                                  payloadLenMax;
    u8                                   randomizer[RANDOMIZER_SIZE];
    u8                                  *eslPayload = &payload[2];

    if (!eslState.pendingCommand.active) {
        return 0;
    }

    eslState.pendingCommand.active = false;

    // Check how many bytes may be written
    if (len <= sizeof(randomizer) + MIC_SIZE + 2 + 2) {
        return 0;
    }

    payloadLenMax = len - (sizeof(randomizer) + MIC_SIZE + 2 + 2);

    if (blt_calBit1Number(eslState.pendingCommand.responsesMask) + numRsp != eslState.pendingCommand.numResponses) {
        BLT_ESL_LOG("Error: expected: %d, received: %d", eslState.pendingCommand.numResponses - blt_calBit1Number(eslState.pendingCommand.responsesMask), numRsp);
        return 0;
    }

    for (u8 i = 0; i < eslState.pendingCommand.numResponses; i++) {
        u8  rspBuf[BLC_ESLS_CMD_RSP_MAX_LENGTH];
        u16 rspLen;

        if (eslState.pendingCommand.responsesMask & (1 << i)) {
            // Reponse already cached
            rspLen    = blc_eslss_eslResponsePayloadWrite(rspBuf, sizeof(rspBuf), cachedRsp);
            cachedRsp = (blc_eslss_controlPointResponseHdr_t *)((u8 *)cachedRsp) + blc_esl_getResponseSize(cachedRsp);
        } else {
            rspLen = blc_eslss_eslResponsePayloadWrite(rspBuf, sizeof(rspBuf), rsp);
            rsp    = (blc_eslss_controlPointResponseHdr_t *)((u8 *)rsp) + blc_esl_getResponseSize(rsp);
        }

        /*
         * ESLP spec 5.3.1.4 Specific requirements for ESL responses "If including all the requested
         * response TLVs would cause the maximum ESL Payload size of 48 octets to be exceeded, then the ESL
         * shall substitute the Capacity Limit Error response described in the ESL Service [5] for one or more such
         * response TLVs, such that the maximum ESL Payload size is not exceeded in the response."
         * According to the above, check if we have enough room for this response, and optionally error responses
         * for the rest.
         */
        if ((rspLen + (eslState.pendingCommand.numResponses - (i + 1)) * BLC_ESLS_CMD_RSP_MIN_LENGTH) > (payloadLenMax - eslPayloadLen)) {
            blc_eslss_controlPointResponseError_t error = {
                .hdr.opcode = BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_ERROR,
                .error      = BLC_ESLSS_ERROR_CODE_CAPACITY_LIMIT,
            };

            BLT_ESL_LOG("Failed to put [%d] response, capacity error", i);

            // Set error response instead of original one
            rspLen = blc_eslss_eslResponsePayloadWrite(rspBuf, sizeof(rspBuf), &error.hdr);
        }

        if (eslPayloadLen + rspLen > payloadLenMax) {
            BLT_ESL_LOG("Error: Not enough room to store response [%d]", i);
            return 0;
        }

        // Write the response
        memcpy(eslPayload, rspBuf, rspLen);
        eslPayloadLen += rspLen;
        eslPayload += rspLen;
    }

    payload[1] = DT_ELECTRONIC_SHELF_LABEL;
    payload[0] = eslPayloadLen + 1;

    generateRandomNum(sizeof(randomizer), randomizer);
    blc_eslp_eslEncryptEslResponse(randomizer, payload, eslPayloadLen + 2, &payload[2], &eslPayloadLen);
    payload[0] = eslPayloadLen + 1;
    payload[1] = DT_ENCRYPTED_ADVERTISING_DATA;

    return eslPayloadLen + 2;
}

ble_sts_t blc_eslp_esl_controlPointResponse(blc_esl_handle_t handle, u8 numRsp, blc_eslss_controlPointResponseHdr_t *rsp)
{
    // ESL payload + randomizer + MIC + [length, ESL_TAG] + [length, ENC_ADV_TAG]
    u8        payload[MAX_ESL_PAYLOAD_SIZE + RANDOMIZER_SIZE + MIC_SIZE + 2 + 2];
    ble_sts_t status;
    u8        rspLen;

    if (handle.type == BLC_ESL_TRANSPORT_TYPE_ACL) {
        if (numRsp != 1) {
            return GATT_ERR_INVALID_PARAMETER;
        }

        return blc_eslss_updateEslControlPointResponse(handle.connHandle, rsp);
    }

    rspLen = createPawrResponse(payload, sizeof(payload), numRsp, rsp);
    if (!rspLen) {
        BLT_ESL_LOG("Error: Failed to create response");
        return GATT_ERR_UNSPECIFIED;
    }

    status = blc_hci_le_setPAwRsync_rspData(eslState.pendingCommand.syncHandle, eslState.pendingCommand.evtCounter, eslState.pendingCommand.subevt, eslState.pendingCommand.subevt,
                                            eslState.pendingCommand.rspSlot, rspLen, payload);
    if (status != BLE_SUCCESS) {
        BLT_ESL_LOG("Error: Failed to send response");
    }

    return status;
}

static void blt_eslPastRcvd(u8 *packet, int length)
{
    hci_le_periodicAdvSyncTransferRcvdEvt_V2_t *pPastEvt = (hci_le_periodicAdvSyncTransferRcvdEvt_V2_t *)packet;

    (void)length;

    if (pPastEvt->connHandle != eslState.apConnHandle) {
        blc_ll_periodicAdvertisingTerminateSync(pPastEvt->syncHandle);
        return;
    }

    if (pPastEvt->status == 0 && eslState.paState == ESL_PA_STATE_IDLE) {
        u8 subevent = eslState.eslAddress.groupId;
        u8 status;

        /*
         * "A synchronization packet in which the Group_ID field has the value N shall be transmitted in a Periodic
         * Advertising with Responses subevent with the subevent number N; therefore, the Group_ID maps to a
         * specific Periodic Advertising with Responses subevent." ESLP specification 5.3.1.3 Specific requirements
         * for commands.
         */
        if (!blt_esl_configuration_complete() || pPastEvt->num_subevt <= subevent) {
            blc_ll_periodicAdvertisingTerminateSync(pPastEvt->syncHandle);
            if (eslState.eslAddressPresent) {
                BLT_ESL_LOG("Num subevents: 0x%02x Group ID: 0x%02X", pPastEvt->num_subevt, eslState.eslAddress.groupId);
            } else {
                BLT_ESL_LOG("PAST received while no ESL address present");
            }
            goto terminate_sync;
        }

        status = blc_hci_le_setPeriodicSyncSubevent(pPastEvt->syncHandle, 0, 1, &subevent);
        BLT_ESL_LOG("[PAST] Set periodic sync subevent status: 0x%02x", status);
        if (status != BLE_SUCCESS) {
            goto terminate_sync;
        }

        eslState.paState      = ESL_PA_STATE_ESTABLISHED;
        eslState.paSyncHandle = pPastEvt->syncHandle;
        eslState.paSubevent   = subevent;
        if (eslState.disconnectAfterPASync) {
            blc_ll_disconnect(pPastEvt->connHandle, HCI_ERR_REMOTE_USER_TERM_CONN);
            blt_esl_updateState(0xFFFF, BLC_ESLS_STATE_SYNCHRONIZED);
            eslState.disconnectAfterPASync = false;
            eslState.updateCompleteDone    = true;
        }

        return;

terminate_sync:
        blc_ll_periodicAdvertisingTerminateSync(pPastEvt->syncHandle);
        eslState.disconnectAfterPASync = false;
    }
}

u8 *blc_eslp_getAdvTypeInfo(u8 *pAdvDat, u32 len, data_type_t advType, u8 *outLen)
{
    u8  adLen = 0;
    u8 *p     = pAdvDat;

    while (len) {
        adLen = p[0];
        if (p[1] == advType) {
            *outLen = adLen - 1;
            return p + 2;
        }

        if (len > (u8)(adLen + 1)) {
            len -= (adLen + 1);
            p += (adLen + 1);
        } else {
            len = 0;
        }
    }

    *outLen = 0;

    return NULL;
}

static void blt_eslParsePayload(hci_le_periodicAdvReportEvtV2_t *pPdaReport, u8 *eslPayload, u8 eslPayloadLen)
{
    u8                                  evtData[sizeof(blc_eslp_esl_controlPointCommant_t) + MAX_ESL_PAYLOAD_SIZE];
    blc_eslp_esl_controlPointCommant_t *evt    = (blc_eslp_esl_controlPointCommant_t *)evtData;
    blc_esl_handle_t                    handle = {
                           .type = BLC_ESL_TRANSPORT_TYPE_ADV,
    };
    u16  cmdOffset, rspOffset;
    u8   rspSlot, rspIdx = 0;
    u8   cmdNum           = 0;
    u8   eslPayloadOffset = 0;
    bool rspSlotPresent;

    eslState.pendingCommand.responsesMask = 0;
    cmdOffset = rspOffset = 0;

    rspSlotPresent = blc_eslss_eslCommandPayloadGetRspSlot(eslPayload, eslPayloadLen, &rspSlot);

    while (true) {
        u16 cmdLen, rspLen, consumed;

        consumed = blc_eslss_eslCommandPayloadParse(&eslPayload[eslPayloadOffset], eslPayloadLen - eslPayloadOffset, (blc_eslss_controlPointCommandHdr_t *)(evt->cmds + cmdOffset),
                                                    &cmdLen, (blc_eslss_controlPointResponseHdr_t *)&eslState.pendingCommand.responses[rspOffset], &rspLen);
        if (!consumed) {
            // No more commands
            break;
        }

        eslPayloadOffset += consumed;

        if (rspLen) {
            eslState.pendingCommand.responsesMask |= 1 << rspIdx;
            rspIdx++;
        } else if (cmdLen) {
            cmdNum++;
            // Check if this command requires response based
            if (((blc_eslss_controlPointCommandHdr_t *)(evt->cmds + cmdOffset))->eslId != BLC_ESLS_ESL_ID_BROADCAST) {
                rspIdx++;
            }
        } else {
            continue;
        }

        cmdOffset += cmdLen;
        rspOffset += rspLen;
    }

    if (!cmdOffset && !rspOffset) {
        // No commands and no responses, ignore
        return;
    }

    // If rspSlotPresent is set, ESL device is supposed to respond
    if (rspSlotPresent) {
        eslState.pendingCommand.active       = true;
        eslState.pendingCommand.rspSlot      = rspSlot;
        eslState.pendingCommand.evtCounter   = pPdaReport->paEventCounter;
        eslState.pendingCommand.syncHandle   = pPdaReport->syncHandle;
        eslState.pendingCommand.subevt       = pPdaReport->subevent;
        eslState.pendingCommand.numResponses = rspIdx;
    } else {
        eslState.pendingCommand.active = false;
    }

    if (cmdNum) {
        // Send event to application
        evt->handle.type = BLC_ESL_TRANSPORT_TYPE_ADV;
        evt->numCmds     = cmdNum;
        blt_prf_sendEvent(0xFFFF, ESL_EVT_ESLP_ESL_CONTROL_POINT_CMD, (u8 *)evtData, sizeof(*evt) + cmdOffset);
    } else if (eslState.pendingCommand.active) {
        blc_eslp_esl_controlPointResponse(handle, 0, NULL);
    }
}

static void blt_eslPeriodicAdvReportV2(u8 *packet, int length)
{
    hci_le_periodicAdvReportEvtV2_t *pPdaReport = (hci_le_periodicAdvReportEvtV2_t *)packet;
    u8                               decryptedData[MAX_ESL_PAYLOAD_SIZE + 2];
    u8                               encDataLen, eslPayloadLen, decryptedDataLen;
    u8                              *encData;
    u8                              *eslPayload;

    (void)length;

    if (eslState.state != BLC_ESLS_STATE_SYNCHRONIZED) {
        return;
    }

    // First, check if encrypted data is present in adv data
    encData = blc_eslp_getAdvTypeInfo(pPdaReport->data, pPdaReport->dataLength, DT_ENCRYPTED_ADVERTISING_DATA, &encDataLen);
    if (!encData || encDataLen > MAX_ESL_ENCRYPTED_DATA_LENGTH) {
        goto discard;
    }

    // Next, try to decode it
    if (!blc_eslp_eslDecryptAdvData(encData, encDataLen, decryptedData, &decryptedDataLen)) {
        goto discard;
    }

    eslPayload = blc_eslp_getAdvTypeInfo(decryptedData, decryptedDataLen, DT_ELECTRONIC_SHELF_LABEL, &eslPayloadLen);
    if (!eslPayload) {
        BLT_ESL_LOG("No ESL Payload in encrypted data");
        goto discard;
    }

    if (eslPayloadLen > MAX_ESL_PAYLOAD_SIZE) {
        BLT_ESL_LOG("ESL payload exceeds max size");
        goto discard;
    }

    if (eslPayload[0] != eslState.eslAddress.groupId) {
        BLT_ESL_LOG("Group ID mismatch: expected %d received %d", eslState.eslAddress.groupId, eslPayload[0]);
        goto discard;
    }

    // Reload timer
    blt_esl_timerReload(STATE_TIMEOUT_MSEC, blt_esl_unsynchronize);

    blt_eslParsePayload(pPdaReport, &eslPayload[1], eslPayloadLen - 1);

discard:
    blc_hci_le_setPAwRsync_rspData(pPdaReport->syncHandle, pPdaReport->paEventCounter, pPdaReport->subevent, pPdaReport->subevent, 0, 0, NULL);
}

static void blt_eslPeriodicAdvSyncLost(u8 *p, int len)
{
    (void)len;
    (void)p;

    if (eslState.state == BLC_ESLS_STATE_SYNCHRONIZED) {
        blt_esl_updateState(0xFFFF, BLC_ESLS_STATE_UNSYNCHRONIZED);
    }

    eslState.paState = ESL_PA_STATE_IDLE;
}

static void blt_eslDisconnectEvt(u8 *p, int len)
{
    hci_disconnectionCompleteEvt_t *evt = (hci_disconnectionCompleteEvt_t *)p;

    (void)len;

    if (eslState.apConnHandle != evt->connHandle) {
        return;
    }

    eslState.apConnHandle          = 0xFFFF;
    eslState.disconnectAfterPASync = false;
    if (eslState.updateCompleteDone) {
        if (eslState.paState == ESL_PA_STATE_ESTABLISHED) {
            blt_esl_updateState(0xFFFF, BLC_ESLS_STATE_SYNCHRONIZED);
        } else {
            blt_esl_updateState(0xFFFF, BLC_ESLS_STATE_UNSYNCHRONIZED);
        }

        eslState.updateCompleteDone = false;
    } else {
        /*
         * ESLS specification 2.7.3.2.1 Link loss: "If the connection is lost owing to
         * link loss occurring in the Configuring state before the configuration of
         * the ESL has been completed, then the ESL shall transition to the Unassociated
         * state described in Section 2.7.3.1. However, if the connection is lost
         * owing to link loss after the configuration of the ESL has been successfully
         * completed, the ESL shall transition to the Unsynchronized state described in
         * Section 2.7.3.5."
         */
        if (eslState.paState == ESL_PA_STATE_ESTABLISHED) {
            blc_ll_periodicAdvertisingTerminateSync(eslState.paSyncHandle);
            BLT_ESL_LOG("Terminate PA SYNC...\r\n");
            eslState.paState = ESL_PA_STATE_IDLE;
        }

        if (blt_esl_configuration_complete()) {
            blt_esl_updateState(0xFFFF, BLC_ESLS_STATE_UNSYNCHRONIZED);
        } else {
            blt_esl_unassociate();
        }
    }
}

static void blt_eslDeviceConnected(u16 connHandle, u8 *addr, u8 addrType)
{
    smp_param_save_t smp_param_load;

    if (blc_smp_loadBondingInfoByAddr(0, 0, addrType, addr, &smp_param_load)) {
        eslState.apConnHandle = connHandle;
        blt_esl_updateState(connHandle, BLC_ESLS_STATE_UPDATING);
    }
}

static void blt_eslEnhancedConnectionComplete(u8 *p, int len)
{
    hci_le_enhancedConnCompleteEvt_t *evt = (hci_le_enhancedConnCompleteEvt_t *)p;

    (void)len;

    if (evt->status == BLE_SUCCESS) {
        blt_eslDeviceConnected(evt->connHandle, evt->PeerAddr, evt->PeerAddrType);
    }
}

static void blt_eslConnectionComplete(u8 *p, int len)
{
    hci_le_connectionCompleteEvt_t *evt = (hci_le_connectionCompleteEvt_t *)p;

    (void)len;

    if (evt->status == BLE_SUCCESS) {
        blt_eslDeviceConnected(evt->connHandle, evt->peerAddr, evt->peerAddrType);
    }
}

void blc_eslp_esl_hostEventCallback(u32 h, u8 *p, int n)
{
    u8 event = h & 0xFF;

    (void)n;

    switch (event) {
    case GAP_EVT_SMP_SECURITY_PROCESS_DONE:
    {
        gap_smp_securityProcessDoneEvt_t *pEvt = (gap_smp_securityProcessDoneEvt_t *)p;

        /*
         * ESLS specification 2.7.3.1 Unassociated state: "If the Bonding procedure is
         * successfully completed with the Client, then the ESL shall transition to
         * the Configuring state described in Section 2.7.3.2."
         */
        if (eslState.state == BLC_ESLS_STATE_UNASSOCIATED) {
            blt_esl_updateState(pEvt->connHandle, BLC_ESLS_STATE_CONFIGURING);
            eslState.apConnHandle = pEvt->connHandle;
        }
        break;
    }
    default:
        break;
    }
}

void blc_eslp_esl_stackEventCallback(u32 h, u8 *p, int n)
{
    if (h & HCI_FLAG_EVENT_BT_STD) {
        u8 evtCode = h & 0xff;

        if (evtCode == HCI_EVT_DISCONNECTION_COMPLETE) {
            blt_eslDisconnectEvt(p, n);
        }

        if (evtCode == HCI_EVT_LE_META) {
            u8 subEvt_code = p[0];

            if (subEvt_code == HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_TRANSFER_RECEIVED_V2) {
                blt_eslPastRcvd(p, n);
            } else if (subEvt_code == HCI_SUB_EVT_LE_ENHANCED_CONNECTION_COMPLETE) {
                blt_eslEnhancedConnectionComplete(p, n);
            } else if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_COMPLETE) {
                blt_eslConnectionComplete(p, n);
            } else if (subEvt_code == HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_REPORT_V2) {
                blt_eslPeriodicAdvReportV2(p, n);
            } else if (subEvt_code == HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_LOST) {
                blt_eslPeriodicAdvSyncLost(p, n);
            }
        }
    }
}
