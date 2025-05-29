/********************************************************************************************************
 * @file    ap_buf.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    01,2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#pragma once

#define MAX_RSP_SLOTS 24

typedef struct
{
    bool                   recordInUse;
    blc_esls_eslAddress_t  eslAddress;
    blc_esls_keyMaterial_t eslResponseKey;
    blc_aes_ccm_crypt_t    eslResponseCcmCrypt;
} electronicShelfLabelRecord_t;

typedef struct
{
    u32 espectedResponseSlots;
    u8  eslId[MAX_RSP_SLOTS];
} accessPointPendingResponses_t;

typedef struct
{
    bool                          inProgress;
    u8                            payload[MAX_ESL_PAYLOAD_SIZE + RANDOMIZER_SIZE + MIC_SIZE + 2 + 2];
    u8                            payloadLen;
    accessPointPendingResponses_t pendingResponses;
} accessPointPendingCommand_t;

extern const u16 blc_eslp_apEslRecordsNum;
extern const u8  blc_eslp_apGroupsNum;

/**
 * @brief           Retrieves the Electronic Shelf Label (ESL) record.
 * @param[in]       idx - The index of the ESL record to retrieve.
 * @return          electronicShelfLabelRecord_t* - Pointer to the ESL record.
 */
electronicShelfLabelRecord_t *blc_eslp_getEslRecord(u16 idx);

/**
 * @brief           Retrieves the pending command for the access point.
 * @param[in]       idx - The index of the pending command to retrieve.
 * @return          accessPointPendingCommand_t* - Pointer to the pending command structure.
 */
accessPointPendingCommand_t *blc_eslp_getPendingCommand(u8 idx);

/**
 * @brief           Retrieves the pending responses for the access point.
 * @param[in]       idx - The index of the pending response to retrieve.
 * @return          accessPointPendingResponses_t* - Pointer to the pending responses structure.
 */
accessPointPendingResponses_t *blc_eslp_getPendingResponse(u8 idx);
