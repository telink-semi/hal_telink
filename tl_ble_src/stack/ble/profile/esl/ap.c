/********************************************************************************************************
 * @file    ap.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    12,2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

typedef struct
{
    blc_esls_keyMaterial_t apSyncKey;
    blc_aes_ccm_crypt_t    apSyncCcmCrypt;
} accessPointState_t;

_attribute_ble_data_retention_ accessPointState_t apState;

static void blt_esl_clientCb(u16 connHandle, int evtID, u8 *data, u16 dataLen)
{
    blt_prf_sendEvent(connHandle, evtID, data, dataLen);
}

void blc_eslp_ap_registerAccessPoint(const void *param)
{
    const accessPointParam_t     *apParam = (const accessPointParam_t *)param;
    electronicShelfLabelRecord_t *record;

    blc_esl_registerESLSControlClient(apParam->eslscParam);
    blc_eslsc_setElectronicShelfLabelCback(blt_esl_clientCb);
    blc_eslp_ap_setAPSyncKey(apParam->apSyncKey);

    for (u16 i = 0; i < blc_eslp_apEslRecordsNum; i++) {
        record = blc_eslp_getEslRecord(i);
        memset(record, 0, sizeof(*record));
    }
}

void blc_eslp_ap_setAPSyncKey(blc_esls_keyMaterial_t *apSyncKey)
{
    apState.apSyncKey = *apSyncKey;
    blt_crypto_init_ccm_adv(apSyncKey->sessionKey, apSyncKey->IV, &apState.apSyncCcmCrypt);
}

static electronicShelfLabelRecord_t *blt_elsp_ap_findEslRecord(blc_esls_eslAddress_t *eslAddress)
{
    electronicShelfLabelRecord_t *record;

    for (u16 i = 0; i < blc_eslp_apEslRecordsNum; i++) {
        record = blc_eslp_getEslRecord(i);
        if (record->recordInUse && record->eslAddress.eslId == eslAddress->eslId && record->eslAddress.groupId == eslAddress->groupId) {
            return record;
        }
    }

    return NULL;
}

static electronicShelfLabelRecord_t *blt_elsp_ap_addEslRecord(blc_esls_eslAddress_t *eslAddress)
{
    electronicShelfLabelRecord_t *record;

    for (u16 i = 0; i < blc_eslp_apEslRecordsNum; i++) {
        record = blc_eslp_getEslRecord(i);
        if (!record->recordInUse) {
            record->recordInUse = true;
            record->eslAddress  = *eslAddress;
            return record;
        }
    }

    return NULL;
}

ble_sts_t blc_eslp_ap_writeEslAddress(u16 connHandle, blc_esls_eslAddress_t *eslAddress, prf_write_cb_t cb)
{
    electronicShelfLabelRecord_t *esl = blt_elsp_ap_findEslRecord(eslAddress);

    if (!esl) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    return blc_eslsc_writeEslAddress(connHandle, eslAddress, cb);
}

ble_sts_t blc_eslp_ap_writeApSyncKeyMaterial(u16 connHandle, prf_write_cb_t cb)
{
    return blc_eslsc_writeApSyncKeyMaterial(connHandle, &apState.apSyncKey, cb);
}

ble_sts_t blc_eslp_ap_writeEslResponseKeyMaterial(u16 connHandle, blc_esls_eslAddress_t *eslAddress, prf_write_cb_t cb)
{
    electronicShelfLabelRecord_t *esl = blt_elsp_ap_findEslRecord(eslAddress);

    if (!esl) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    return blc_eslsc_writeEslResponseKeyMaterial(connHandle, &esl->eslResponseKey, cb);
}

ble_sts_t blc_eslp_ap_writeEslCurrentAbsolutTime(u16 connHandle, u32 currentAbsoluteTime, prf_write_cb_t cb)
{
    return blc_eslsc_writeEslCurrentAbsolutTime(connHandle, currentAbsoluteTime, cb);
}

ble_sts_t blc_eslp_ap_writeControlPoint(u16 connHandle, blc_eslss_controlPointCommandHdr_t *cmd, prf_write_cb_t cb)
{
    return blc_eslsc_writeControlPoint(connHandle, cmd, cb);
}

ble_sts_t blc_eslp_ap_writeControlPointNoRsp(u16 connHandle, blc_eslss_controlPointCommandHdr_t *cmd)
{
    return blc_eslsc_writeControlPointNoRsp(connHandle, cmd);
}

ble_sts_t blc_eslp_ap_addEsl(blc_esls_eslAddress_t *eslAddress, blc_esls_keyMaterial_t *eslResponseKey)
{
    electronicShelfLabelRecord_t *esl = blt_elsp_ap_findEslRecord(eslAddress);

    if (esl || eslAddress->groupId >= blc_eslp_apGroupsNum) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    // Check if there is record registered with this address
    esl = blt_elsp_ap_addEslRecord(eslAddress);
    if (!esl) {
        return GATT_ERR_DATA_LENGTH_EXCEED_MEM_RESTRICTION;
    }

    esl->eslResponseKey = *eslResponseKey;
    blt_crypto_init_ccm_adv(eslResponseKey->sessionKey, eslResponseKey->IV, &esl->eslResponseCcmCrypt);

    return BLE_SUCCESS;
}

void blc_eslp_ap_removeEsl(blc_esls_eslAddress_t *eslAddress)
{
    electronicShelfLabelRecord_t *esl = blt_elsp_ap_findEslRecord(eslAddress);

    if (esl) {
        memset(esl, 0, sizeof(*esl));
    }
}

static bool blc_eslp_apDecryptAdvData(u8 *encData, u8 encDataLen, u8 *outData, u8 *outDataLen, blc_esls_eslAddress_t *eslAddress)
{
    electronicShelfLabelRecord_t *esl = blt_elsp_ap_findEslRecord(eslAddress);

    if (!esl) {
        return false;
    }

    return blt_crypto_ccm_dec_adv(encData, encDataLen, &esl->eslResponseCcmCrypt, outData, outDataLen) == 0;
}

static bool blc_eslp_apEncryptEslCommand(u8 randomizer[5], u8 *rawData, u8 rawDataLen, u8 *encData, u8 *encDataLen)
{
    blt_crypto_ccm_enc_adv(randomizer, rawData, rawDataLen, &apState.apSyncCcmCrypt, encData, encDataLen);

    return true;
}

static u16 createPawrCommand(u8 *payload, u16 len, u8 groupId, u8 numCmd, blc_eslss_controlPointCommandHdr_t *cmd)
{
    u8  randomizer[RANDOMIZER_SIZE];
    u8 *eslPayload    = &payload[3];
    u8  eslPayloadLen = 0;
    u8  cmdLen        = 0;
    u16 payloadLenMax;

    // Check how many bytes may be written
    if (len <= sizeof(randomizer) + MIC_SIZE + 2 + 2 + 1) {
        return 0;
    }

    payloadLenMax = len - (sizeof(randomizer) + MIC_SIZE + 2 + 2 + 1);

    for (u8 i = 0; i < numCmd; i++) {
        u8 cmdBuf[BLC_ESLS_CMD_RSP_MAX_LENGTH];

        // First, check if we have esl response key
        if (cmd->eslId != BLC_ESLS_ESL_ID_BROADCAST) {
            blc_esls_eslAddress_t address = {
                .eslId   = cmd->eslId,
                .groupId = groupId,
            };

            if (!blt_elsp_ap_findEslRecord(&address)) {
                return 0;
            }
        }

        cmdLen = ble_eslsc_eslCommandPayloadWrite(cmdBuf, sizeof(cmdBuf), cmd);
        cmd    = (blc_eslss_controlPointCommandHdr_t *)(((u8 *)cmd) + blc_esl_getCommandSize(cmd));
        /*
         * ESLP specification 5.3.1.3 Specific requirements for commands: "any number of
         * TLVs may be included provided that the maximum ESL Payload size of 48 octets
         * is not exceeded."
         */
        if (eslPayloadLen + cmdLen > payloadLenMax) {
            return 0;
        }

        // Write the response
        memcpy(eslPayload, cmdBuf, cmdLen);
        eslPayloadLen += cmdLen;
        eslPayload += cmdLen;
    }
    payload[2] = groupId;
    payload[1] = DT_ELECTRONIC_SHELF_LABEL;
    payload[0] = eslPayloadLen + 2;

    generateRandomNum(sizeof(randomizer), randomizer);
    blc_eslp_apEncryptEslCommand(randomizer, payload, eslPayloadLen + 3, &payload[2], &eslPayloadLen);
    payload[0] = eslPayloadLen + 1;
    payload[1] = DT_ENCRYPTED_ADVERTISING_DATA;

    return eslPayloadLen + 2;
}

static void blt_eslp_ap_updateRspSlot(accessPointPendingResponses_t *expectedResponses, u8 rspSlot, u8 eslId)
{
    // First, cleanup the previously assigned to this esl id rsp slots
    for (u8 i = 0; i < sizeof(expectedResponses->espectedResponseSlots) * 8; i++) {
        if ((expectedResponses->espectedResponseSlots & (1 << i)) && expectedResponses->eslId[i] == eslId) {
            expectedResponses->espectedResponseSlots &= ~(1 << i);
        }
    }

    expectedResponses->espectedResponseSlots |= 1 << rspSlot;
    expectedResponses->eslId[rspSlot] = eslId;
}

static void blt_eslp_ap_setupExpectedResponses(u8 numCmd, blc_eslss_controlPointCommandHdr_t *cmd, accessPointPendingResponses_t *expectedResponses)
{
    memset(expectedResponses, 0, sizeof(*expectedResponses));

    for (u8 i = 0; i < numCmd; i++) {
        if (cmd->eslId != BLC_ESLS_ESL_ID_BROADCAST) {
            // Set rsp slot
            blt_eslp_ap_updateRspSlot(expectedResponses, i, cmd->eslId);
        }

        cmd = (blc_eslss_controlPointCommandHdr_t *)(((u8 *)cmd) + blc_esl_getCommandSize(cmd));
    }
}

ble_sts_t blc_eslp_ap_writePawrCommand(u8 groupId, u8 numCmd, blc_eslss_controlPointCommandHdr_t *cmd)
{
    accessPointPendingCommand_t *pendingCommand = blc_eslp_getPendingCommand(groupId);

    if (!pendingCommand) {
        BLT_ESL_LOG("invalid group id");
        return GATT_ERR_INVALID_PARAMETER;
    }

    if (pendingCommand->inProgress) {
        BLT_ESL_LOG("Pending command");
        return GATT_ERR_INVALID_PARAMETER;
    }

    pendingCommand->payloadLen = createPawrCommand(pendingCommand->payload, sizeof(pendingCommand->payload), groupId, numCmd, cmd);
    if (!pendingCommand->payloadLen) {
        BLT_ESL_LOG("Failed to create pawr command");
        return GATT_ERR_INVALID_PARAMETER;
    }

    blt_eslp_ap_setupExpectedResponses(numCmd, cmd, &pendingCommand->pendingResponses);
    pendingCommand->inProgress = true;

    return BLE_SUCCESS;
}

static void blt_esl_periodicAdvertisingSubeventDataRequest(u8 *p)
{
    accessPointPendingCommand_t          *pendingCommand;
    accessPointPendingResponses_t        *pendingResponse;
    hci_le_periodicAdvSubevtDataReqEvt_t *pPasdr = (hci_le_periodicAdvSubevtDataReqEvt_t *)p;
    u8                                    subevtDataReq[pPasdr->subevtDataCount * (sizeof(pdaSubevtData_subevtCfg_t) + sizeof(pendingCommand->payload))];
    pdaSubevtData_subevtCfg_t            *pSubevtCfg = (pdaSubevtData_subevtCfg_t *)subevtDataReq;

    for (int i = 0; i < pPasdr->subevtDataCount; i++) {
        u8 subevent_idx            = pPasdr->subevtStart + i;
        pSubevtCfg->subevent_idx   = subevent_idx;
        pSubevtCfg->rsp_slot_start = 0;
        pSubevtCfg->rsp_slot_count = 0;

        pendingCommand  = blc_eslp_getPendingCommand(subevent_idx);
        pendingResponse = blc_eslp_getPendingResponse(subevent_idx);
        if (pendingCommand && pendingCommand->inProgress) {
            blc_eslp_ap_pawrCommandSentEvt_t evt = {
                .status  = BLE_SUCCESS,
                .groupId = subevent_idx,
            };

            if (pendingCommand->pendingResponses.espectedResponseSlots) {
                for (u8 j = 0; j < MAX_RSP_SLOTS; j++) {
                    // No expected response
                    if (!(pendingCommand->pendingResponses.espectedResponseSlots & (1 << j))) {
                        continue;
                    }

                    if (pSubevtCfg->rsp_slot_count) {
                        pSubevtCfg->rsp_slot_count = j - pSubevtCfg->rsp_slot_start + 1;
                    } else {
                        pSubevtCfg->rsp_slot_start = j;
                        pSubevtCfg->rsp_slot_count = 1;
                    }
                }
            }

            pSubevtCfg->subevt_data_len = pendingCommand->payloadLen;
            memcpy(pSubevtCfg->pSubevt_data, pendingCommand->payload, pendingCommand->payloadLen);
            pendingCommand->inProgress = false;
            *pendingResponse           = pendingCommand->pendingResponses;

            blt_prf_sendEvent(0xFFFF, ESL_EVT_ESLP_AP_PAWR_COMMAND_SENT, (u8 *)&evt, sizeof(evt));
        } else {
            pSubevtCfg->subevt_data_len = 0;
        }

        //Pointer to the next buffer
        pSubevtCfg = (pdaSubevtData_subevtCfg_t *)((u8 *)pSubevtCfg + sizeof(pdaSubevtData_subevtCfg_t) + pSubevtCfg->subevt_data_len);
    }

    blc_ll_setPeriodicAdvSubeventData(pPasdr->advHandle, pPasdr->subevtDataCount, (pdaSubevtData_subevtCfg_t *)subevtDataReq);
}

static void ble_esl_sendPawrRspRcvdError(blc_esls_eslAddress_t *address, ble_sts_t status)
{
    blc_eslp_ap_pawrResponseRcvd_t pEvt = {
        .address = *address,
        .status  = status,
        .numRsp  = 0,
    };

    blt_prf_sendEvent(0xFFFF, ESL_EVT_ESLP_AP_PAWR_RESPONSE_RCVD, (u8 *)&pEvt, sizeof(pEvt));
}

static void ble_esl_parseResponses(blc_esls_eslAddress_t *address, u8 *eslPayload, u16 eslPayloadLen)
{
    u8                              evtData[sizeof(blc_eslp_ap_pawrCommandSentEvt_t) + MAX_ESL_PAYLOAD_SIZE];
    blc_eslp_ap_pawrResponseRcvd_t *pEvt        = (blc_eslp_ap_pawrResponseRcvd_t *)evtData;
    u8                             *responseBuf = (u8 *)pEvt->rsp;
    u16                             rspOffset, parsed;

    rspOffset = parsed = 0;

    pEvt->address = *address;
    pEvt->numRsp  = 0;
    pEvt->status  = BLE_SUCCESS;

    while (parsed < eslPayloadLen) {
        u16 rspLen, consumed;

        consumed = blc_eslsc_eslResponsePayloadParse(&eslPayload[parsed], eslPayloadLen - parsed, (blc_eslss_controlPointResponseHdr_t *)responseBuf + rspOffset, &rspLen);
        if (!consumed) {
            // No more responses
            break;
        }

        parsed += consumed;

        if (rspLen) {
            pEvt->numRsp++;
            rspOffset += rspLen;
        }
    }

    blt_prf_sendEvent(0xFFFF, ESL_EVT_ESLP_AP_PAWR_RESPONSE_RCVD, (u8 *)pEvt, sizeof(*pEvt) + rspOffset);
}

static void blt_esl_parsePeriodicAdvertisingSubeventResponseReport(u8 subevent, pawrRspReportDat_t *report)
{
    accessPointPendingResponses_t *responses;
    u8                             responseSlot = report->responseSlot;
    u8                             dataStatus   = report->dataStatus;

    responses = blc_eslp_getPendingResponse(subevent);
    if (!responses) {
        return;
    }

    if ((1 << responseSlot) & responses->espectedResponseSlots) {
        blc_esls_eslAddress_t address = {
            .groupId = subevent,
            .eslId   = responses->eslId[responseSlot],
        };

        responses->espectedResponseSlots &= ~(1 << responseSlot);

        if (dataStatus != 0xFF) {
            u8  decryptedData[MAX_ESL_PAYLOAD_SIZE + 2];
            u8  encDataLen, decryptedDataLen, eslPayloadLen;
            u8 *eslPayload;
            u8 *encData;

            encData = blc_eslp_getAdvTypeInfo(report->data, report->dataLength, DT_ENCRYPTED_ADVERTISING_DATA, &encDataLen);
            if (!encData || encDataLen > MAX_ESL_ENCRYPTED_DATA_LENGTH) {
                BLT_ESL_LOG("No encrypted data in response");
                goto failed;
            }

            // Next, try to decode it
            if (!blc_eslp_apDecryptAdvData(encData, encDataLen, decryptedData, &decryptedDataLen, &address)) {
                BLT_ESL_LOG("Failed to decrypt response");
                goto failed;
            }

            eslPayload = blc_eslp_getAdvTypeInfo(decryptedData, decryptedDataLen, DT_ELECTRONIC_SHELF_LABEL, &eslPayloadLen);
            if (!eslPayload) {
                BLT_ESL_LOG("No ESL Payload in encrypted data");
                goto failed;
            }

            if (eslPayloadLen > MAX_ESL_PAYLOAD_SIZE) {
                BLT_ESL_LOG("ESL Payload too long");
                goto failed;
            }

            ble_esl_parseResponses(&address, eslPayload, eslPayloadLen);

            return;
        }
failed:
        ble_esl_sendPawrRspRcvdError(&address, GATT_ERR_UNSPECIFIED);
    }
}

static void blt_esl_periodicAdvertisingSubeventResponseReport(u8 *p)
{
    hci_le_periodicAdvRspReportEvt_t *pPasdr = (hci_le_periodicAdvRspReportEvt_t *)p;
    u16                               len    = 0;

    if (pPasdr->Subevent >= blc_eslp_apGroupsNum) {
        return;
    }

    for (int i = 0; i < pPasdr->Num_Responses; i++) {
        blt_esl_parsePeriodicAdvertisingSubeventResponseReport(pPasdr->Subevent, &pPasdr->rspReportDat[i]);
        len += sizeof(hci_le_periodicAdvRspReportEvt_t) + sizeof(pawrRspReportDat_t) + pPasdr->rspReportDat[i].dataLength;
        pPasdr = (hci_le_periodicAdvRspReportEvt_t *)(p + len);
    }
}

void blc_eslp_ap_stackEventCallback(u32 h, u8 *p, int n)
{
    (void)n;

    if (h & HCI_FLAG_EVENT_BT_STD) {
        u8 evtCode = h & 0xff;

        if (evtCode == HCI_EVT_LE_META) {
            u8 subEvt_code = p[0];
            if (subEvt_code == HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SUBEVENT_DATA_REQUEST) {
                blt_esl_periodicAdvertisingSubeventDataRequest(p);
            } else if (subEvt_code == HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_RESPONSE_REPORT) {
                blt_esl_periodicAdvertisingSubeventResponseReport(p);
            }
        }
    }
}
