/********************************************************************************************************
 * @file    esl.h
 *
 * @brief   This is the header file for BLE SDK
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
#pragma once

#include "tl_common.h"
/*
 * ESLP specification 5.3.1 Synchronization packet:
 * "The ESL Payload shall be less than or equal to 48 octets in length."
 */
#define MAX_ESL_PAYLOAD_SIZE (48)

/*
 * 2 octets for ESL tag + length
 * 5 octets for Randomizer
 * 48 octets for max ESL payload size
 * 4 bytes for MIC
 */
#define MAX_ESL_ENCRYPTED_DATA_LENGTH (2 + 5 + MAX_ESL_PAYLOAD_SIZE + 4)

#define RANDOMIZER_SIZE               (5)
#define MIC_SIZE                      (4)

#include "esl_config.h"
#include "ap_buf.h"

u8 *blc_eslp_getAdvTypeInfo(u8 *pAdvDat, u32 len, data_type_t advType, u8 *outLen);

#define BLT_ESL_LOG(fmt, ...) BLC_PROFILE_DEBUG(DBG_PRF_MASK_ESL_ESLP_LOG, "[ESL]" fmt, ##__VA_ARGS__)

/******************************* ESLP Electonic Shelf Label Start **********************************************************************/

typedef struct
{
    const blc_eslss_regParam_t *eslssParam;
} electronicShelfLabelParam_t;

typedef enum
{
    BLC_ESL_TRANSPORT_TYPE_ACL,
    BLC_ESL_TRANSPORT_TYPE_ADV,
} blc_esl_transportType_t;

typedef struct
{
    blc_esl_transportType_t type;
    u16                     connHandle;
} blc_esl_handle_t;

//ESLP ESL Event ID
typedef enum
{
    ESL_EVT_ESLP_ESL_START = ESL_EVT_TYPE_ESL,
    ESL_EVT_ESLP_ESL_CONTROL_POINT_CMD,
    ESL_EVT_ESLP_ESL_CURRENT_ABSOLUTE_TIME,
    ESL_EVT_ESLP_ESL_AP_SYNC_KEY_MATERIAL,
    ESL_EVT_ESLP_ESL_ESL_RESPONSE_KEY_MATERIAL,
    ESL_EVT_ESLP_ESL_ADDRESS,
    ESL_EVT_ESLP_ESL_STATE,
} eslp_esl_evt_enum;

typedef struct
{ //Event ID: ESL_EVT_ESLP_ESL_CURRENT_ABSOLUTE_TIME
    u32 time;
} blc_eslp_esl_currentAbsoluteTimeEvt_t;

typedef struct
{ //Event ID: ESL_EVT_ESLP_ESL_AP_SYNC_KEY_MATERIAL
    //Event ID: ESL_EVT_ESLP_ESL_ESL_RESPONSE_KEY_MATERIAL
    blc_esls_keyMaterial_t key;
} blc_eslp_esl_keyMaterialEvt_t;

typedef struct
{ //Event ID: ESL_EVT_ESLP_ESL_AP_SYNC_KEY_MATERIAL
    //Event ID: ESL_EVT_ESLP_ESL_ESL_RESPONSE_KEY_MATERIAL
    blc_esls_eslAddress_t address;
} blc_eslp_esl_addressEvt_t;

typedef struct
{ //Event ID: ESL_EVT_ESLP_ESL_STATE
    blc_esls_state_t state;
} blc_eslp_esl_stateEvt_t;

typedef struct
{ //Event ID: ESL_EVT_ESLP_ESL_CONTROL_POINT_CMD
    blc_esl_handle_t handle;
    u8               numCmds;
    u8               cmds[0];
} blc_eslp_esl_controlPointCommant_t;

/**
 * @brief       This function serves to register ESLP Electronic Shel Label
 * @param[in]   refer to 'electronicShelfLabelParam_t'
 * @return      none.
 */
void blc_eslp_esl_registerElectronicShelfLabel(const void *param);

/**
 * @brief           This function sends response to control point command
 * @param handle    Transport handle
 * @param numRsp    Number of responses
 * @param rsp       Array of responses
 * @return          status
 */
ble_sts_t blc_eslp_esl_controlPointResponse(blc_esl_handle_t handle, u8 numRsp, blc_eslss_controlPointResponseHdr_t *rsp);

/**
 * @brief   This function handles stack events
 * @param h HCI event type
 * @param p packet pointer
 * @param n packet length
 */
void blc_eslp_esl_stackEventCallback(u32 h, u8 *p, int n);

/**
 * @brief   This function handles host events
 * @param h HCI event type
 * @param p packet pointer
 * @param n packet length
 */
void blc_eslp_esl_hostEventCallback(u32 h, u8 *p, int n);

/**
 * @brief   This function is used to serve internal timers
 */
void blc_eslp_esl_loop(void);

/**
 * @brief   This function is used to unassociate from AP. It will drop PA sync if exists and clear
 *          ESL address and Keys.
 */
void blc_eslp_esl_unassociate(void);

/**
 * @brief   This function returns PA sync state of ESL.
 * @return  True if ESL is synchronized, false otherwise
 */
bool ble_eslp_esl_is_pa_synchronized(void);

/******************************* ESLP Electonic Shelf Label End **********************************************************************/

/******************************* ESLP Access Point Start **********************************************************************/

typedef struct
{
    u8                          numGroups;
    blc_esls_keyMaterial_t     *apSyncKey;
    const blc_eslsc_regParam_t *eslscParam;
} accessPointParam_t;

//ESLP ESL Event ID
typedef enum
{
    ESL_EVT_ESLP_AP_START = ESL_EVT_TYPE_AP,
    ESL_EVT_ESLP_AP_PAWR_COMMAND_SENT,
    ESL_EVT_ESLP_AP_PAWR_RESPONSE_RCVD,
} eslp_ap_evt_enum;

typedef struct
{ //Event ID: ESL_EVT_ESLP_AP_PAWR_COMMAND_SENT
    u8        groupId;
    ble_sts_t status;
} blc_eslp_ap_pawrCommandSentEvt_t;

typedef struct
{ //Event ID: ESL_EVT_ESLP_AP_PAWR_RESPONSE_RCVD
    blc_esls_eslAddress_t               address;
    ble_sts_t                           status;
    u8                                  numRsp;
    blc_eslss_controlPointResponseHdr_t rsp[0];
} blc_eslp_ap_pawrResponseRcvd_t;

/**
 * @brief       This function serves to register the ESLP Access Point.
 * @param[in]   param - Refer to 'accessPointParam_t'.
 * @return      None
 */
void blc_eslp_ap_registerAccessPoint(const void *param);


/**
 * @brief       This function sets the AP Sync Key Material.
 * @param[in]   apSyncKey - The AP Sync Key.
 * @return      None
 */
void blc_eslp_ap_setAPSyncKey(blc_esls_keyMaterial_t *apSyncKey);


/**
 * @brief       This function writes the ESL address to a remote device.
 * @param[in]   connHandle - The connection handle.
 * @param[in]   eslAddress - Pointer to the ESL address.
 * @param[in]   cb - Write response callback.
 * @return      ble_sts_t - Status of the operation.
 */
ble_sts_t blc_eslp_ap_writeEslAddress(u16 connHandle, blc_esls_eslAddress_t *eslAddress, prf_write_cb_t cb);


/**
 * @brief       This function writes the AP Sync Key Material set already with
 *              blc_eslp_ap_setAPSyncKey() or blc_eslp_ap_registerAccessPoint().
 * @param[in]   connHandle - The connection handle.
 * @param[in]   cb - Write response callback.
 * @return      ble_sts_t - Status of the operation.
 */
ble_sts_t blc_eslp_ap_writeApSyncKeyMaterial(u16 connHandle, prf_write_cb_t cb);


/**
 * @brief       This function writes the ESL Response Key Material to a remote device.
 *              This key can be provided with blc_eslp_ap_addEsl().
 * @param[in]   connHandle - The connection handle.
 * @param[in]   eslAddress - Pointer to the ESL address.
 * @param[in]   cb - Write response callback.
 * @return      ble_sts_t - Status of the operation.
 */
ble_sts_t blc_eslp_ap_writeEslResponseKeyMaterial(u16 connHandle, blc_esls_eslAddress_t *eslAddress, prf_write_cb_t cb);


/**
 * @brief       This function writes the ESL Current Absolute Time.
 * @param[in]   connHandle - The connection handle.
 * @param[in]   currentAbsoluteTime - The current absolute time.
 * @param[in]   cb - Write response callback.
 * @return      ble_sts_t - Status of the operation.
 */
ble_sts_t blc_eslp_ap_writeEslCurrentAbsolutTime(u16 connHandle, u32 currentAbsoluteTime, prf_write_cb_t cb);


/**
 * @brief       This function writes the ESL Control Point.
 * @param[in]   connHandle - The connection handle.
 * @param[in]   cmd - Pointer to the command header.
 * @param[in]   cb - Write response callback.
 * @return      ble_sts_t - Status of the operation.
 */
ble_sts_t blc_eslp_ap_writeControlPoint(u16 connHandle, blc_eslss_controlPointCommandHdr_t *cmd, prf_write_cb_t cb);


/**
 * @brief       This function writes the ESL Control Point without response.
 * @param[in]   connHandle - The connection handle.
 * @param[in]   cmd - Pointer to the command header.
 * @param[in]   cb - Write response callback (optional, not used in this case).
 * @return      ble_sts_t - Status of the operation.
 */
ble_sts_t blc_eslp_ap_writeControlPointNoRsp(u16 connHandle, blc_eslss_controlPointCommandHdr_t *cmd);


/**
 * @brief       This function adds an ESL with its address and response key.
 *              This allows sending and receiving ESL payload through PAwR.
 *              This function will fail if:
 *               - There is no room for a new ESL device.
 *               - A device with the given address already exists.
 *               - The group ID is not available (see ESLP_AP_MAX_GROUPS configuration macro).
 * @param[in]   eslAddress - Pointer to the ESL address.
 * @param[in]   eslResponseKey - Pointer to the ESL response key.
 * @return      ble_sts_t - Status of the operation.
 */
ble_sts_t blc_eslp_ap_addEsl(blc_esls_eslAddress_t *eslAddress, blc_esls_keyMaterial_t *eslResponseKey);


/**
 * @brief       This function removes the ESL record.
 * @param[in]   eslAddress - Pointer to the ESL address.
 * @return      None
 */
void blc_eslp_ap_removeEsl(blc_esls_eslAddress_t *eslAddress);


/**
 * @brief       This function schedules sending of the PAwR command.
 *              This function will fail if:
 *               - The ESL ID in the command does not match any ESL record (except for broadcast commands).
 *               - There is a pending command.
 *               - The ESL payload would exceed the maximum length.
 * @param[in]   groupId - The group ID (maps to a specific subevent).
 * @param[in]   numCmd - The number of commands.
 * @param[in]   cmd - Pointer to the array of commands.
 * @return      ble_sts_t - Status of the operation.
 */
ble_sts_t blc_eslp_ap_writePawrCommand(u8 groupId, u8 numCmd, blc_eslss_controlPointCommandHdr_t *cmd);


/**
 * @brief       This function handles stack events.
 * @param[in]   h - The HCI event type.
 * @param[in]   p - Pointer to the packet.
 * @param[in]   n - The length of the packet.
 * @return      None
 */
void blc_eslp_ap_stackEventCallback(u32 h, u8 *p, int n);

/******************************* ESLP Access Point End **********************************************************************/
