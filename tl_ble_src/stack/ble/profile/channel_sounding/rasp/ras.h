/********************************************************************************************************
 * @file    ras.h
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

#include "ras_server_buf.h"
#include "ras_client_buf.h"
#include "ras_data.h"

/**
 * @brief       ranging profile log api.
 */
#define BLC_RAS_LOG(fmt, ...)      BLC_PRF_DBG(PRF_DBG_RAS_EN, "[RAS]" fmt, ##__VA_ARGS__)
#define BLC_RAS_DATA_LOG(fmt, ...) BLC_PRF_DBG(PRF_DBG_RAS_DATA_EN, "[RAS_DATA]" fmt, ##__VA_ARGS__)

#ifndef RAS_TIMEOUT_EN
    #define RAS_TIMEOUT_EN 1
#endif

/**
 * @brief the enum of RAS timeout event type.
 */
typedef enum
{
    RAS_TIMER_INVALID,
    RAS_TIMER_ONDEMAND_DATAREADY,
    RAS_TIMER_ONDEMAND_DATA,
    RAS_TIMER_REALTIME_DATA,
    RAS_TIMER_LOCAL_DATA,
} blc_ras_timer_type_enum;

/**
 * @brief structure type storing a single subevent
 */
typedef struct __attribute__((packed))
{
    u8 *pSubEvt;
    u16 subEvtLen;
} blc_rass_subevt_data_t;

/**
 * @brief the data structure of ranging header, as defined in Ranging Service specification - PROCEDURE_HEAD_LEN could be changed to sizeof(blc_rass_prot_head_t)
 */
typedef union __attribute__((packed))
{
    u8 raw[4];

    struct
    {
        u16 procedureCounter : 12; //procedure counter - stores only lower 12 bits of the CS procedure
        u8  proCountCfgID    : 4;  //CS configuration identifier
        s8  selectedTxPower;       //Transmit power level used for CS procedure. Range: -127 to 20 dBm
        u8  numAntennaPaths : 8;   //antennaPathsMask in RAS protocol format - different data encoding
    } data;
} blc_rass_prot_head_t;

/**
 * @brief structure type storing a full procedure in either procedure or protocol format
 */
typedef struct __attribute__((packed))
{
    u8 *pData;   //pointer to a complete procedure - after combining subevents, NULL otherwise
    u16 dataLen; //sum of subEvtLen, avoiding calculate it each time
} blc_rass_proc_data_t;

/**
 * @brief the data structure of procedure data storage and its metadata in either subevent or procedure format
 */
typedef struct __attribute__((packed))
{
    blc_rass_subevt_data_t subEvtData[CS_SUBEVENT_PER_PROCEDURE_MAX];
    blc_rass_proc_data_t   proc;
    blc_rass_prot_head_t   procedureHead; //needed mainly for realtime
    u16                    subEvtNum;
    u16                    rangingCounter;
#if (RAS_TIMEOUT_EN)
    u32 timestamp;
#endif
} blt_ras_proc_ctrl_t;

#define CS_REALTIME_SUBEVENT_MAX (CS_SUBEVENT_PER_PROCEDURE_MAX * 2)

typedef union __attribute__((packed))
{
    u8 raw[2];

    struct
    {
        u16 procedureCounter : 12;
        u16 last             : 4;
    } data;
} blc_rass_realtime_flag_t;

/**
 * @brief structure type storing a single subevent
 */
typedef struct __attribute__((packed))
{
    u8                      *pSubEvt;
    u16                      subEvtLen;
    blc_rass_realtime_flag_t flag;
} blc_rass_realtime_prot_subevt_data_t;

typedef struct __attribute__((packed))
{
    blc_rass_realtime_prot_subevt_data_t subEvtData[CS_REALTIME_SUBEVENT_MAX];
    u8                                   wptr;
    u8                                   rptr;
} blt_ras_realtime_prot_ctrl_t;

/**
 * @brief timeouts configuration
 */
#if (RAS_TIMEOUT_EN)
    /**
 * @brief       RAS server / RAS client local procedure timeout
 *              RAS server - when expired will remove the local procedure
 *              RAS client - when expired will call CS_EVT_TIMEOUT event with type = RAS_TIMER_LOCAL_DATA
 */
    #define PROCEDURE_DATA_TIMEOUT (SYSTEM_TIMER_TICK_1S * 10) // RAS 3.2.1

    /**
 * @brief       RAS client timeout for first segment transmisstion for ondemand
 *              RAS client - when expired will do abort and call CS_EVT_TIMEOUT event with type = RAS_TIMER_ONDEMAND_DATA
 */
    #define ON_DEMAND_DATA_TIMEOUT_INTERVAL (SYSTEM_TIMER_TICK_1S * 5) // RAS 4.5.4.1

    /**
 * @brief       RAS client timeout for further segment transmisstion for ondemand
 *              RAS client - when expired will do abort amd call CS_EVT_TIMEOUT event with type = RAS_TIMER_ONDEMAND_DATA
 */
    #define ON_DEMAND_DATA_TIMEOUT_INTERVAL_CONTINUE (SYSTEM_TIMER_TICK_1S * 1)

    /**
 * @brief       RAS client timeout for lack of data ready from RAS server
 *              RAS client - will call CS_EVT_TIMEOUT event with type = RAS_TIMER_ONDEMAND_DATAREADY
 */
    #define RANGING_DATA_READY_TIMEOUT (SYSTEM_TIMER_TICK_1S * 5) // RAS 4.4.3.1

    /**
 * @brief       RAS client timeout for first segment transmisstion for realtime
 *              RAS client - will call CS_EVT_TIMEOUT event with type = RAS_TIMER_REALTIME_DATA
 */
    #define REALTIME_DATA_TIMEOUT (SYSTEM_TIMER_TICK_1S * 5) // RAS 4.4.1.1

    /**
 * @brief       RAS client timeout for further segment transmisstion for realtime
 *              RAS client - will call CS_EVT_TIMEOUT event with type = RAS_TIMER_REALTIME_DATA
 */
    #define REALTIME_DATA_TIMEOUT_CONTINUE (SYSTEM_TIMER_TICK_1S * 1)
#endif

/**
 * @brief the data structure of RAS timeout event.
 */
typedef struct
{
    u16                     connHandle;
    u16                     rangingCounter;
    blc_ras_timer_type_enum type;
} blc_ras_timeout_evt_t;

/**
 * @brief the data structure of RAS data ready event.
 */
typedef struct
{
    u16                     connHandle;
    u16                     rangingCounter;
} blc_ras_data_ready_evt_t;

/**
 * @brief the data structure of RAS lost segments event.
 */
typedef struct
{
    u16                     connHandle;
    u16                     rangingCounter;
    u8                      segmentStart;
    u8                      segmentEnd;
} blc_ras_lost_segments_evt_t;

/**
 * @brief the data structure of RAS procedure ack event.
 */
typedef struct
{
    u16                     connHandle;
    u16                     rangingCounter;
} blc_ras_procedure_ack_evt_t;

/**
 * @brief RAS events enum
 */
typedef enum
{
    CS_EVT_RACC_START = CS_EVT_TYPE_RASC,
    CS_EVT_RANGING_DATA,
    CS_EVT_OVERWRITTEN,
    CS_EVT_TIMEOUT,
    CS_EVT_DATA_READY,
    CS_EVT_LOST_SEGMENTS,
    CS_EVT_PROCEDURE_ACK,
    CS_EVT_RASC_START = CS_EVT_TYPE_RASS,
    CS_EVT_LOCAL_RANGING_DATA,
} blc_ras_evt_enum;

/**
 * @brief the data structure of ranging data event.
 */
typedef struct __attribute__((packed))
{
    u16  connHandle;
    u16  rangingCounter;
    u16  rangingDataLen;
    u8  *rangingData;
    bool realtimeDataLost;
} blc_rasc_ranging_data_evt_t;

/**
 * @brief the data structure of local Ranging Data.
 */
typedef struct __attribute__((packed))
{
    u16 connHandle;
    u8 *dataPtr;
    u16 dataLen;
} blc_rasc_local_ranging_data_evt_t;

/**
 * @brief the data structure of remote overwritten event.
 */
typedef struct __attribute__((packed))
{
    u16 connHandle;
    u16 rangingCounter;
} blc_rasc_overwritten_evt_t;

/******************************* ranging Profile Client Start **********************************************************************/

/**
 * @brief       register ranging profile client controller.
 * @param[in]   param: initial parameter.
 * @return      none.
 */
void blc_rap_registerRasProfileControlClient(const blc_rasc_regParam_t *param);

/**
 * @brief       get ras features available on connected server. Available after connection.
 * @param[in]   connHandle: ACL connect handle.
 * @param[in/out] feature - svc_ras_feature_t type variable passed as reference, into which the features will be read
 * @return      BLE_SUCCESS - success
 *              other       - error
 */
ble_sts_t blc_rasc_getRasFeature(u16 connHandle, svc_ras_feature_t **feature);

/**
 * @brief       RAP config complete event handler
 * @param[in]   pConfigComplete Pointer to event parameter buffer.
 * @return      BLE_SUCCESS - success
 *              other       - error
 */
ble_sts_t blc_rap_csConfigComplete(hci_le_csConfigCompleteEvt_t *pConfigComplete);

/**
 * @brief       read RAS feature
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   readCb: callback for read
 * @return      BLE_SUCCESS - success
 *              other       - error
 */
ble_sts_t blc_rasc_readRasFeature(u16 connHandle, prf_read_cb_t readCb);

/**
 * @brief       read RAS data ready
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   readCb: callback for read
 * @return      BLE_SUCCESS - success
 *              other       - error
 */
ble_sts_t blc_rasc_readRasDataReady(u16 connHandle, prf_read_cb_t readCb);

/**
 * @brief       read RAS data overwritten
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   readCb: callback for read
 * @return      BLE_SUCCESS - success
 *              other       - error
 */
ble_sts_t blc_rasc_readRasDataOverwritten(u16 connHandle, prf_read_cb_t readCb);

/**
 * @brief       Write ccc configuration value for ondemand
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   value: input value
 * @param[in]   writeCb: callback for write
 * @return      BLE_SUCCESS - success
 *              other       - error
 */
ble_sts_t blc_rapc_writeOnDemandCcc(u16 connHandle, u16 value, prf_write_cb_t writeCb);

/**
 * @brief       Write ccc configuration value for realtime
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   value: input value
 * @param[in]   writeCb: callback for write
 * @return      BLE_SUCCESS - success
 *              other       - error
 */
ble_sts_t blc_rapc_writeRealtimeCcc(u16 connHandle, u16 value, prf_write_cb_t writeCb);

/**
 * @brief       Set filter configuration using mode and its value
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   mode: mode - between 0 and 3
 * @param[in]   filterValue: input filter value
 * @param[in]   writeCb: callback for write
 * @return      BLE_SUCCESS - success
 *              other       - error
 */
ble_sts_t blc_rapc_setFilterModeAndValue(u16 connHandle, u8 mode, u16 filterValue, prf_write_cb_t writeCb);

/**
 * @brief       Set filter configuration using mode and its value
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   filterSetting: lowest 2 bits are the mode, bits 2-15 are the filterValue - as in RAS spec
 * @param[in]   writeCb: callback for write
 * @return      BLE_SUCCESS - success
 *              other       - error
 */
ble_sts_t blc_rapc_setFilterSetting(u16 connHandle, u16 filterSetting, prf_write_cb_t writeCb);

/**
 * @brief       Trigger abort operation
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   writeCb: callback for write
 * @return      BLE_SUCCESS - success
 *              other       - error
 */
ble_sts_t blc_rapc_writeAbortOperation(u16 connHandle, prf_write_cb_t writeCb);

/**
 * @brief       clear local procedure data
 * @param[in]   connHandle: ACL connect handle.
 * @return      BLE_SUCCESS - success
 *              other       - error
 */
ble_sts_t blc_rap_clearAndInitializeLocal(u16 connHandle);

/**
 * @brief       clear remote procedure data received from the server by the client
 * @param[in]   connHandle: ACL connect handle.
 * @return      BLE_SUCCESS - success
 *              other       - error
 */
ble_sts_t blc_rapc_clearAndInitializeRemote(u16 connHandle);

#if (RAS_LOGIC_MANUAL)
/**
 * @brief       sending a get specific procedure manually
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   rangingCounter: selected ranging counter
 * @param[in]   writeCb: callback for write
 * @return      BLE_SUCCESS - success
 *              other       - error
 */
ble_sts_t blt_rapc_writeGetSpecificRecord(u16 connHandle, u16 rangingCounter, prf_write_cb_t writeCb);

/**
 * @brief       sending a write ack specific record manually
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   rangingCounter: selected ranging counter
 * @param[in]   writeCb: callback for write
 * @return      BLE_SUCCESS - success
 *              other       - error
 */
ble_sts_t blt_rapc_writeAckSpecificRecord(u16 connHandle, u16 rangingCounter, prf_write_cb_t writeCb);

/**
 * @brief       writing a get record segment to rascp
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   rangingCounter: selected ranging counter
 * @param[in]   startSegment: selected start segment
 * @param[in]   endSegment: selected end segment
 * @param[in]   writeCb: callback for write
 * @return      BLE_SUCCESS - success
 *              other       - error
 */
ble_sts_t blt_rapc_writeGetRecordSegments(u16 connHandle, u16 rangingCounter, u16 startSegment, u16 endSegment, prf_write_cb_t writeCb);
#endif

/******************************* ranging Profile Client end **********************************************************************/

/******************************* ranging Profile server Start **********************************************************************/

/**
 * @brief       register ranging profile server controller.
 * @param[in]   param: initial parameter.
 * @return      none.
 */
void blc_rap_registerRasProfileControlServer(const blc_rass_regParam_t *param);

/******************************* ranging Profile server end **********************************************************************/

/**
 * @brief       IOP testing related functions - TO BE REMOVED FROM RELEASE
 */

#if(RAS_IOPTEST_ENABLE)
/**
 * @brief       disable automatic handling of exchanges with the RAS server. Not tested extensively. Used for IOP purposes.
 */
void blc_rasc_iop_enableRasClientManualTesting(u16 connHandle);

/**
 * @brief       initiating configs with relevant parameters, manually
 */
void blc_ras_iop_initConfig(u16 connHandle, u8 role, u8 rttType);

/**
 * @brief       issuing any RASCP command manually
 */
ble_sts_t blc_rasc_iop_issueAnyCommandControlPoint(u16 connHandle, u8 opcode, void* operand, u16 operandLen, prf_write_cb_t writeCb);

/**
 * @brief       sending a get specific procedure manually
 */
ble_sts_t blc_rapc_iop_writeGetSpecificRecord(u16 connHandle, u16 rangingCounter, prf_write_cb_t writeCb);

/**
 * @brief       sending a write ack specific record manually, even without it being available
 */
ble_sts_t blc_rapc_iop_writeAckSpecificRecord(u16 connHandle, u16 rangingCounter, prf_write_cb_t writeCb);

/**
 * @brief       sending back a complete procedure response manually
 */
ble_sts_t blc_rass_iop_initialCompleteProcedureDataResponse(u16 connHandle, u16 rangingCounter);

/**
 * @brief       sending back a complete lost segment response manually
 */
ble_sts_t blc_rass_iop_initialCompleteLostProcedureSegmentResponse(u16 connHandle, u16 rangingCounter, u16 segmentStart, u16 segmentEnd);

/**
 * @brief       sending back a response code manually
 */
ble_sts_t blc_rass_iop_initialResponseCode(u16 connHandle, u8 responseCode);

/**
 * @brief       writing a get record segment to rascp
 */
ble_sts_t blc_rasc_iop_writeGetRecordSegments(u16 connHandle, u16 rangingCounter, u16 startSegment, u16 endSegment, prf_write_cb_t writeCb);

/**
 * @brief       setting a ccc value using a characteristic handle
 */
ble_sts_t blc_rapc_iop_setCccValue(u16 connHandle, u16 cccHandle, u16 value, prf_write_cb_t writeCb);
#endif

#if(TTF_EN)
/**
 * @brief       don't act on get command if enabled, 0 - disabled, 1 - enabled
 */
void blc_rass_iop_stopGet(u16 connHandle, bool stopGetEnabled);

/**
 * @brief       don't send get response if enabled, 0 - disabled, 1 - enabled
 */
void blc_rass_iop_stopRsp(u16 connHandle, bool stopRspEnabled);

/**
 * @brief       executing a get command locally, as if a get was received from the client
 */
att_err_t blc_rass_iop_issueGetReportOneRecord(u16 connHandle, u16 rangingCounter);
#endif