/********************************************************************************************************
 * @file    ap_buf.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "esl_config.h"

const u16 blc_eslp_apEslRecordsNum = ESLP_AP_ESL_RECORDS;
const u8  blc_eslp_apGroupsNum     = ESLP_AP_MAX_GROUPS;

static _attribute_ble_data_retention_ electronicShelfLabelRecord_t eslRecords[ESLP_AP_ESL_RECORDS];
static accessPointPendingCommand_t                                 pendingCommands[ESLP_AP_MAX_GROUPS];
static accessPointPendingResponses_t                               pendingResponses[ESLP_AP_MAX_GROUPS];

electronicShelfLabelRecord_t *blc_eslp_getEslRecord(u16 idx)
{
    return idx >= ARRAY_SIZE(eslRecords) ? NULL : &eslRecords[idx];
}

accessPointPendingCommand_t *blc_eslp_getPendingCommand(u8 idx)
{
    return idx >= ARRAY_SIZE(pendingCommands) ? NULL : &pendingCommands[idx];
}

accessPointPendingResponses_t *blc_eslp_getPendingResponse(u8 idx)
{
    return idx >= ARRAY_SIZE(pendingResponses) ? NULL : &pendingResponses[idx];
}
