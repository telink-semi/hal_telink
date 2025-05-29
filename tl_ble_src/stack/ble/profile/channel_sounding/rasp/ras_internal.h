/********************************************************************************************************
 * @file    rass_internal.h
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

#define RAS_IOPTEST_LOSTSEGMENTS      (0)
#define RAS_IOPTEST_ORD_007           (0)
#define RAS_IOPTEST_ORD_008           (0)
#define RAS_IOPTEST_ORD_001           (0)
#define RAS_IOPTEST_RRD_003           (0)
#define RAS_IOPTEST_ORD_PAIR_03       (0)

#define RAS_DEBUG_PRINTBUFFERS        (0)
#define RAS_DEBUG_PRINTQUEUE          (0)
#define RAS_DEBUG_PRINTSENT           (0)
#define RAS_DEBUG_STEP_LEN_VALIDATION (1)

#define RAS_REALTIME_PROT_FLOW        (1)

#ifndef TTF_EN
    #define TTF_EN 0
#endif

#if (TTF_EN)
    #define RAS_DEBUG_DEBUGWAIT (1)
#endif

#ifndef RAS_DEBUG_DEBUGWAIT
    #define RAS_DEBUG_DEBUGWAIT           (0)
#endif

#ifndef RAS_DEBUG
    #define RAS_DEBUG 0
#endif

#ifndef RAS_STEP_FILTER
    #define RAS_STEP_FILTER 0
#endif

#define RAS_CP_MSG_HEADER_SIZE          1
#define RAS_CP_CHECK_CMD_OPCODE(opcode) (opcode >= RAS_CP_CMD_MAX_OPCODE)
#define RAS_MAX_CS_CONFIG               4
#define RAS_INVALID_INDEX_PROCEDURE     0xFFFF //TODO: 65535 is a valid procedure counter - TODO: consider if we need this special value
#define RAS_LOST_SEGMENT_WILDCARD       0xFF

#if(Google_SRS)
/**
 * @brief Google SRS Response value
 */
typedef struct __attribute__((packed))
{
    u8  type;  // 0x00 data(data read from responder) // 0x01 reply(Reply write by requester, ignore following data)
    u32 inline_pct: 1;
    u32 mode0ChanMap: 1;
    u32 prefered_aclIntval: 1;
    u32 rfu: 29;
} google_srs_local_cap_t;
#endif
/**
 * @brief the RAS response values.
 */
typedef enum
{
    CS_RAS_REPEAT = 0, //we use 0 for no response code to keep the message in the queue

    CS_RAS_RFU = 0x00, //we use 0 for no response code to keep the message in the queue
    CS_RAS_SUCCESS,    //0x01
    CS_RAS_OPCODE_NOT_SUPPORTED,
    CS_RAS_INVALID_PARAMETER,
    CS_RAS_SUCCESS_PERSISTED,
    CS_RAS_ABORT_UNSUCCESSFUL,
    CS_RAS_PROCEDURE_NOT_COMPLETED,
    CS_RAS_SERVER_BUSY,
    CS_RAS_NO_RECORDS_FOUND, //0x08
} blt_ras_response_enum;

/**
 * @brief ras server att handles
 */
typedef struct{
    u16 baseHandle;
    u8  endHdl;
    u16 rasFeatureHandle;
    u16 realtimeProcedureDataHandle;      //real time procedure data attribute handle
    u16 realtimeProcedureCccHandle;       //real time procedure ccc attribute handle
    u16 ondemandProcedureDataHandle;      //on demand procedure data attribute handle
    u16 ondemandProcedureCccHandle;       //on demand procedure ccc attribute handle
    u16 rasControlPointHandle;            //ras control point attribute handle
    u16 rasControlPointCccHandle;         //ras control point ccc attribute handle
    u16 rangingDataReadyDataHandle;       //ranging data ready attribute data handle
    u16 rangingDataReadyCccHandle;        //ranging data ready attribute ccc handle
    u16 rangingDataOverwrittenDataHandle; //stored ranging Overwritten data attribute data handle
    u16 rangingDataOverwrittenCccHandle;  //stored ranging Overwritten data attribute ccc handle
} blt_rasc_att_hdl_t;

/**
 * @brief ras server nvinfo for storage
 */
typedef struct{
    blt_rasc_att_hdl_t att;
    // u8 rasFeatureProperties;
    u8 realtimeProcedureDataProperties;  //real time procedure data attribute properties
    u8 ondemandProcedureDataProperties;  //on demand procedure data attribute properties
    u8 rasControlPointProperties;        //ras control point attribute properties
    u8 rangingDataReadyProperties;       //ranging data ready attribute properties
    u8 rangingDataOverwrittenProperties; //stored ranging Overwritten data attribute properties
#if(RAS_PERSISTENT_FILTER)
    blt_ras_filter_t filter;
#endif
} blt_rasc_nv_info_t;

/**
 * @brief Ranging Data common.
 */
/**
 * @brief the data structure of Segmentation Header.
 */
typedef struct __attribute__((packed))
{
    union
    {
        u8 raw;

        struct __attribute__((packed))
        {
            u8 firstSegment   : 1;
            u8 lastSegment    : 1;
            u8 segmentCounter : 6;
        } data;
    };
} blt_ras_segmentation_header_t;

/**
 * @brief the data structure of Ranging Data.
 */
typedef struct __attribute__((packed))
{
    blt_ras_segmentation_header_t header;
    u8                            data[0];
} blt_ras_segmentation_ranging_data_t;

/**
 * @brief RAS-CP command opcode enumerate.
 */
typedef enum
{
    RAS_CP_CMD_OPCODE_GET_RANGING_DATA,
    RAS_CP_CMD_OPCODE_ACK_RANGING_DATA,
    RAS_CP_CMD_OPCODE_GET_LOST_RANGING_DATA,
    RAS_CP_CMD_OPCODE_ABORT_OPERATION,
    RAS_CP_CMD_OPCODE_FILTER,
    RAS_CP_CMD_MAX_OPCODE,
} blt_ras_cp_command_opcode_enum;

/**
 * @brief RAS-CP command opcode feature mask.
 */
typedef enum
{
    RAS_FEATURE_REALTIME_MASK = 0xFE, //b11111110,
    RAS_FEATURE_GETLOST_MASK  = 0xFD, //b11111101,
    RAS_FEATURE_ABORT_MASK    = 0xFB, //b11111011,
    RAS_FEATURE_FILTER_MASK   = 0xF7, //b11110111,
    RAS_FEATURE_ACTIVE_MASK   = 0xFF, //b11111111
} blt_ras_cp_command_opcode_feature_mask;

/**
 * @brief RAS-CP response opcode enumerate.
 */
typedef enum
{
    RAS_CP_RSP_OPCODE_COMPLETE_REPORT_RECORDS,
    RAS_CP_RSP_OPCODE_COMPLETE_RECORD_SEGMENT,
    RAS_CP_RSP_OPCODE_RESPONSE_CODE,
} blt_ras_cp_response_opcode_enum;

/**
 * @brief the operands structure of Get one Report Records.
 */
typedef struct __attribute__((packed))
{
    u16 rangingCounter;
} blt_ras_operands_get_one_record_t;

/**
 * @brief the operands structure of Ack one On demand Records.
 */
typedef struct __attribute__((packed))
{
    u16 rangingCounter;
} blt_ras_operands_ack_one_record_t;

/**
 * @brief the operands structure of Get Lost Segments.
 */
typedef struct __attribute__((packed))
{
    u16 rangingCounter;
    u8 startSegmentAsIndex; //u8 startSegment; ES-26224, ES-26235
    u8 endSegmentAsIndex; //u8 endSegment; ES-26224, ES-26235
} blt_ras_operands_get_lost_segments_t;

/**
 * @brief the operands structure of Abort Operation.
 */
typedef struct __attribute__((packed))
{
} blt_ras_operands_abort_operation_t;

/**
 * @brief the operands structure of Filter Operation.
 */
typedef union __attribute__((packed))
{
    u16 raw;

    struct
    {
        u16 mode          : 2;
        u16 filterBitMask : 14;
    } bits;
} blt_ras_operands_filter_operation_t;

/**
 * @brief the operands structure of Complete Report Records Response.
 */
typedef struct __attribute__((packed))
{
    u16 rangingCounter;
} blt_ras_operands_complete_record_t;

/**
 * @brief the operands structure of Complete Record Segment Response.
 */
typedef struct __attribute__((packed))
{
    u16 rangingCounter;
    u8  startSegment;
    u8  endSegment;
} blt_ras_operands_complete_lost_segment_t;

/**
 * @brief the operands structure of Complete Record Segment Response.
 */
typedef struct __attribute__((packed))
{
    u8 responseCode;
} blt_ras_operands_response_code_t;

/**
 * @brief the operands structure.
 */
typedef struct __attribute__((packed))
{
    union
    {
        u8                                       val[6]; //operand size is 6 octets
        blt_ras_operands_get_one_record_t        getOneRecord;
        blt_ras_operands_ack_one_record_t        ackOneRecord;
        blt_ras_operands_get_lost_segments_t     getLostSegments;
        blt_ras_operands_abort_operation_t       abortOperation;
        blt_ras_operands_filter_operation_t      filterOperation;
        blt_ras_operands_complete_record_t       completeRecord;
        blt_ras_operands_complete_lost_segment_t completeLostSegment;
        blt_ras_operands_response_code_t         responseCode;
    };
} blt_ras_cp_operand_t;

/**
 * @brief the data structure of RAS-CP characteristic.
 */
typedef struct __attribute__((packed))
{
    u8                   opcode;
    blt_ras_cp_operand_t operand;
} blt_ras_cp_msg_t;

/**
 * @brief the data structure of Ranging Data Ready.
 */
typedef struct __attribute__((packed))
{
    u16 rangingCounter;
} blt_ras_dataReady_t;

/**
 * @brief the data structure of Ranging Data Overwritten.
 */
typedef struct __attribute__((packed))
{
    u16 rangingCounter;
} blt_ras_data_overwritten_t;

/**
 * @brief RAS client receive states.
 */
typedef enum
{
    RASC_RECV_STATE_NULL,
    RASC_RECV_STATE_REALTIME_DATA,
    RASC_RECV_STATE_ONDEMAND_DATA,
} blt_rasc_recv_state_enum;

/**
 * @brief RAS server message types.
 */
typedef enum
{
    RASS_MSG_TYPE_NULL                            = 0,
    RASS_MSG_TYPE_REPORT_REAL_TIME_PROCEDURE_DATA = 1,
    RASS_MSG_TYPE_REPORT_ON_DEMAND_PROCEDURE_DATA = 2,
    RASS_MSG_TYPE_REPORT_RANGING_DATA_READY       = 3,
    RASS_MSG_TYPE_REPORT_RANGING_DATA_OVERWRITTEN = 4,
    RASS_MSG_TYPE_COMPLETE_RECORD_RESPONSE        = 5,
    RASS_MSG_TYPE_COMPLETE_LOST_SEGMENT_RESPONSE  = 6,
    RASS_MSG_TYPE_RESPONSE_CODE_RESPONSE          = 7,
} blt_rass_msg_type_enum;

/**
 * @brief Enum defining if the incoming request is for all segments or only selected lost ones.
 */
typedef enum
{
    RAS_ONDEMAND_ALL,
    RAS_ONDEMAND_LOST,
} blt_ras_ondemand_type_enum;

/**
 * @brief RASCP client state enum
 */
typedef enum {
    RAS_CP_STATE_NULL,
    RAS_CP_STATE_EXPECTED_GET_RESPONSE,
    RAS_CP_STATE_EXPECTED_LOST_RESPONSE,
} blt_rasc_rascp_state_enum;

/**
 * @brief Complex RAS server report structures built with initials for acting out with reports
 */
typedef struct __attribute__((packed))
{
    u8 *pData;
    u16 pDataLen;
    u16 segmentCount;
    u16 segmentIndex;   // absolute start send index, tracks remaining segments to be sent
    u16 segmentIndexEnd;
    u16 segmentOffset;  //temporary soution for realtime in chunks
    u16 segmentStart;   //only relevant to lost
    u16 segmentEnd;     //only relevant to lost
    u16 MTU;
    u16 rangingCounter; //only relevant for ondemand
    u8  lost;           //lost - only relevant for ondemand
    u8  last;           //last - only relevant for realtime
    u8  isRealtime;     //only relevant for realtime
} blt_rass_report_realtime_procedure_data_t, blt_rass_report_ondemand_procedure_data_t, blt_rass_report_ranging_data_t;

/**
 * @brief Basic RAS server report structures built with initials for acting out with reports
 */
typedef struct __attribute__((packed))
{
    u16 rangingCounter;
} blt_rass_report_ranging_data_ready_t, blt_rass_report_ranging_data_overwritten_t, blt_rass_report_complete_record_response_t;

/**
 * @brief RAS server lost segment response structure
 */
typedef struct __attribute__((packed))
{
    u16 rangingCounter;
    u16 segmentStart;
    u16 segmentEnd;
} blt_rass_report_lost_segment_response_t;

/**
 * @brief RAS server standard response structure
 */
typedef struct __attribute__((packed))
{
    u8 responseCode;
} blt_rass_report_response_code_response_t;

/**
 * @brief RAS server message union - for storing in BLE handling queue
 */
typedef struct __attribute__((packed)) rass_msg
{
    struct __attribute__((packed)) rass_msg *next;
    u8                                       type;
    u16                                      connHandle;

    union
    {
        blt_rass_report_realtime_procedure_data_t  realtime;
        blt_rass_report_ondemand_procedure_data_t  ondemand;
        blt_rass_report_ranging_data_ready_t       ready;
        blt_rass_report_ranging_data_overwritten_t overwritten;
        blt_rass_report_complete_record_response_t records;
        blt_rass_report_lost_segment_response_t    segment;
        blt_rass_report_response_code_response_t   response;
    };
} blt_rass_msg_t;

/**
 * @brief RAS client timeout handling record
 */
typedef struct __attribute__((packed))
{
    u16                connHandle;
    u32                timestamp;
    u32                interval;
    u16                rangingCounter;
    u8                 type; //blc_ras_timer_type_enum
    blt_ras_timer_cb_t cb;
} blt_rasc_timeout_record_t;

/**
 * @brief the main data structure of RAS Client
 */
typedef struct __attribute__((packed))
{
    gattc_sub_ccc_msg_t ntfInput;
    /* Characteristic value handle */
    u16 rasFeatureHandle;
    u16 realtimeProcedureDataHandle;      //real time procedure data attribute handle
    u16 realtimeProcedureCccHandle;       //real time procedure ccc attribute handle
    u16 ondemandProcedureDataHandle;      //on demand procedure data attribute handle
    u16 ondemandProcedureCccHandle;       //on demand procedure ccc attribute handle
    u16 rasControlPointHandle;            //ras control point attribute handle
    u16 rasControlPointCccHandle;         //ras control point ccc attribute handle
    u16 rangingDataReadyDataHandle;       //ranging data ready attribute data handle
    u16 rangingDataReadyCccHandle;        //ranging data ready attribute ccc handle
    u16 rangingDataOverwrittenDataHandle; //stored ranging Overwritten data attribute data handle
    u16 rangingDataOverwrittenCccHandle;  //stored ranging Overwritten data attribute ccc handle

    // u8 rasFeatureProperties;
    u8  realtimeProcedureDataProperties;  //real time procedure data attribute properties
    u16 realtimeProcedureCccValue;        //real time procedure ccc attribute value
    u8  ondemandProcedureDataProperties;  //on demand procedure data attribute properties
    u16 ondemandProcedureCccValue;        //on demand procedure ccc attribute value
    u8  rasControlPointProperties;        //ras control point attribute properties
    u16 rasControlPointCccValue;          //ras control point ccc attribute value
    u8  rangingDataReadyProperties;       //ranging data ready attribute properties
    u16 rangingDataReadyCccValue;         //ranging data ready attribute ccc value
    u8  rangingDataOverwrittenProperties; //stored ranging Overwritten data attribute properties
    u16 rangingDataOverwrittenCccValue;   //stored ranging Overwritten data attribute ccc value

    u16 rangingDataReadyValue;            //ranging counter read with a ranging data ready read
    u16 rangingDataOverwrittenValue;      //ranging counter read with a ranging data overwritten read

    u8  recvState;
    u16 requestedRangingCounter;

    //offloaded for asynchronous handling
    blt_rasc_get_lost_proc_segment_t asyncLostSegment;
    bool                             asyncSegmentLostFlag;
    bool                             asyncDataReady;

    //for synchronization between local and remote data to avoid a situation, where remote data gets received and app handler executed before local CS data gets handled
    bool remoteDataReady;
    bool localDataReady;
    u8   localReadyCnt;

    blt_rasc_rascp_state_enum rasCPState;

    svc_ras_feature_t      ras_feature;
    blt_ras_ranging_data_t rang_data;
#if (RAS_TIMEOUT_EN)
    blt_rasc_timeout_record_t timeoutRecord;
#endif
#if(RAS_IOPTEST_ENABLE)
    u8 iopTestingManual;
#endif
} blc_rasc_client_t;

/**
 * @brief       Converting lost segment from rolling segment format to index. Count is necessary. Server always has this information.
 * warning      - usage of this function with RAS_LOST_SEGMENT_WILDCARD is not allowed
 * @param[in]   rollingSegment - rolling segment to be converted
 * @param[in]   segmentCount - total segment count
 * @return      lost segment in index format
 */
u8 blt_ras_rollingSegmentToIndex(u8 rollingSegment, u8 segmentCount);

/**
 * @brief       Converting lost segment from index format to rolling segment format. Count is necessary. Server always has this information.
 * warning      - RAS_LOST_SEGMENT_WILDCARD can be used with it. segmentCount only needed for last segment identification
 * @param[in]   index - index of the segment to be converted
 * @param[in]   segmentCount - total segment count
 * @return      lost segment in rolling segment format
 */
u8 blt_ras_indexToRollingSegment(u8 index, u8 segmentCount);

/******************************* ranging Profile server data Start **********************************************************************/

/**
 * @brief Procedure header length, consists of: u16 procedureCounter:12, u8 proCountCfgID:4, u8 selectedTxPower and u8 numAntennaPaths
 */
#define PROCEDURE_HEAD_LEN (4)

/**
 * @brief Subvevent header length
 */
#define SUBEVENT_HEAD_LEN (8)

/**
 * @brief Function for delaying the run until the logs get fully printed
 * warning - may influence timing or even delay certain execution causing an error / timeout
 */
void debugwait(void);

/**
 * @brief Coding of CS procedure / subevent DONE or ABORT information in the subevent result event / continue event
 */
typedef enum
{
    CS_PROC_DONE    = 0x00,
    CS_PROC_ABORT   = 0x0f,
    CS_SUBEVT_DONE  = 0x00,
    CS_SUBEVT_ABORT = 0x0f,
} blt_ras_cs_done_state_enum;

/**
 * @brief the data structure of subevent header for ranging data, as defined in Ranging Service specification - SUBEVENT_HEAD_LEN without pSubeventStepMetadata and pSubeventRangingData
 */
typedef struct __attribute__((packed))
{
    u16 startAclConnEvent;
    u16 frequencyCompensation;
    u8  procedureDoneStatus : 4;
    u8  subeventDoneStatus  : 4;
    u8  rangingAbortReason  : 4;
    u8  subeventAbortReason : 4;
    s8  referencePowerLevel;
    u8  numStepsReported;
    u8  pSubeventStepMetadata[0]; //Bit0 (0-Aborted, 1-Success) Bit1-2 Modetype Bit3-7 RFU
    u8  pSubeventRangingData[0];
} blc_rass_data_body_t;

/**
 * @brief the data structure representing step metadata in RAS protocol format
 */
typedef union __attribute__((packed))
{
    u8 raw[1];

    struct
    {
        u8 mode       : 2;
        u8 RFU        : 5;
        u8 abortedBit : 1;
    } data;
} blc_rass_step_head_t;

/**
 * @brief the data structure of all stored procedures
 */
typedef struct __attribute__((packed))
{
    blt_ras_proc_ctrl_t procCtrl[RAS_PROCEDURE_COUNT];
    u8                  storedNum;
    s8                  selectedTxPower;
} blt_ras_data_ctrl_t;

/**
 * @brief the data structure of procedure data storage and its metadata in protocol format
 */
typedef struct __attribute__((packed))
{
    blc_rass_proc_data_t   prot;
    u16                    rangingCounter;
#if (RAS_TIMEOUT_EN)
    u32 timestamp;
#endif
} blt_ras_prot_ctrl_t;

/**
 * @brief the data structure of procedure data query - internal!
 * warning - there is a similarly called public one - blc_ras_query_result_t
 */
typedef struct __attribute__((packed))
{
    u8                   status; //1: found index, 0: not fuond index
    u16                  index;
    blt_ras_proc_ctrl_t *procData;
} blt_rass_procedure_query_result_t;

/**
 * @brief the data structure of protocol data query - internal!
 * warning - there is a similarly called public one - blc_ras_query_result_t
 */
typedef struct __attribute__((packed))
{
    u8                   status; //1: found index, 0: not fuond index
    u16                  index;
    blt_ras_prot_ctrl_t *protData;
} blt_rass_protocol_query_result_t;

/**
 * @brief a structure for storing CS config information. Only the fields relevant to RAS
 */
typedef struct __attribute__((packed))
{
    u8 valid;
    u8 role;
    u8 rttType;
} blc_rass_cs_config_t;

/**
 * @brief a structure storing the whole local dataset of RAS data. Used durectly by RAS server and on Central side by the app.
 */
typedef struct __attribute__((packed))
{
    blt_ras_data_ctrl_t  dataCtrl;
    blt_ras_filter_t     filter;
    blc_rass_cs_config_t config[RAS_MAX_CS_CONFIG];
} blt_ras_dataset_t;

/**
 * @brief returning an instance of dataset for the connHandle
 */
blt_ras_dataset_t *blc_ras_getDataset(u16 connHandle);

/**
 * @brief writer the pointer of an instance of dataset for the connHandle
 */
void blc_ras_writeDataset(int index, blt_ras_dataset_t *pointer);

/**
 * @brief querying local ras dataset for a procedure with chosen ranging counter
 */
blt_rass_procedure_query_result_t blt_rass_procedureQuery(blt_ras_dataset_t *rasDataset, u16 rangingCounter);

/**
 * @brief remove a procedure from the local ras dataset. Valid for both - RAS server and RAS client.
 */
blt_ras_response_enum blt_ras_procedureDeleteLocal(blt_ras_dataset_t *rasDataset, u16 index);

/**
 * @brief clear all procedures from the local ras dataset. Valid for both - RAS server and RAS client.
 */
void blt_ras_clearAndInitializeLocal(blt_ras_dataset_t *rasDataset);
/**
 * @brief check remote and local device ranging counter, if local device procedure is history data, will be deleted.
 */
void blt_ras_RangingCounterQuery(u16 connHandle, blt_ras_dataset_t *rasDataset, u16 rangingCounter);
/**
 * @brief initiate the filter with default value. Valid for both - RAS server and RAS client.
 */
ble_sts_t blt_ras_initFilterDefault(blt_ras_dataset_t *rasDataset);

/**
 * @brief set filter using mode and filterValue locally - no sending takes place. Valid for both - RAS server and RAS client.
 */
ble_sts_t blt_ras_setFilterMode(blt_ras_dataset_t *rasDataset, u8 mode, u16 filterValue);

/**
 * @brief RAS data when informing RAS server about a completed local procedure, it calls this function
 */
ble_sts_t blt_rass_procedureDataReady(u16 connHandle, u16 rangingCounter);

/**
 * @brief RAS data when informing RAS server about a completed subevent, it calls this function
 */
ble_sts_t blt_rass_procedureDataReadyIntermediate(u16 connHandle, blt_ras_proc_ctrl_t *procCtrl, u8 last);

/**
 * @brief Check RAS server report format weather is realtime or not.
 */
ble_sts_t blt_rass_IsRealTimeReport(u16 connHandle);
/**
 * @brief RAS data informing RAS server about an overwritten record
 */
ble_sts_t blc_rass_procedureDataOverwritten(u16 connHandle, u16 rangingCounter);

/**
 * @brief for converting a procedure in "subevent result" (procedure) format to "RAS ranging data" (protocol) format
 */
ble_sts_t blt_rass_procedureDataToProtocolData(blt_ras_prot_ctrl_t *outputProtCtrl, blt_ras_proc_ctrl_t *inputProcCtrl, blt_ras_dataset_t *rasDataset);

/**
 * @brief for converting a subevent in "subevent result" (procedure) format to "RAS ranging data" (protocol) format
 */
ble_sts_t blt_rass_procedureSubeventToProtocolSubevent(blc_rass_subevt_data_t *outputProtSubEvt, blc_rass_subevt_data_t *inputProcSubEvt, blc_rass_prot_head_t *procedureHead, u8 first, blt_ras_dataset_t *rasDataset);

/**
 * @brief Decompress - part only executed on RAS client, reverse of blt_rass_procedureDataToProtocolData
 */
ble_sts_t blt_rasc_protocolDataToProcedureData(blt_ras_proc_ctrl_t *procCtrlRemoteOutput, blt_ras_proc_ctrl_t *protCtrlRemoteInput, blt_ras_dataset_t *localRasDataset);

/**
 * @brief set local data ready flag
 */
void blt_rasc_setLocalDataReady(u16 connHandle);

/**
 * @brief send profile event to app
 */
void blt_rasc_issueDataReadyAppEvent(u16 connHandle);

#if(RAS_PERSISTENT_FILTER)
/**
 * @brief for exposing filter pointer to store and load persistent filter data to/from flash
 */
blt_ras_filter_t* blt_ras_getFilter(u16 connHandle);
#endif
/******************************* ranging Profile server data end **********************************************************************/


/******************************* ranging Profile Client Start **********************************************************************/

/**
 * @brief the data structure of RAS Client process control.
 */
typedef struct __attribute__((packed)) blc_ras_client_ctrl
{
#if (defined(HOST_V2_ENABLE))
    struct blc_prf_process process;
#else
    blc_prf_proc_t     process;
#endif
    blc_rasc_client_t *pRasPrfClient[LL_MAX_ACL_CEN_NUM + LL_MAX_ACL_PER_NUM];
} blc_ras_client_ctrl_t;

/**
 * @brief       informs RAS client that a new CS procedure has started
 * @param[in]   connHandle: ACL handle.
 * @return      BLE_SUCCESS - success
 *              other       - error
 */
ble_sts_t blt_rasc_newProcedure(u16 connHandle);

/******************************* ranging Profile Client end **********************************************************************/

/******************************* ranging Profile client buff start **********************************************************************/


/******************************* ranging Profile client buff end **********************************************************************/

/******************************* ranging Profile server buff Start **********************************************************************/

/**
 * @brief storing the active rascp request
 */
typedef struct __attribute__((packed))
{
    u8  active;    //active ondemand rascp request 0 - false, 1 - true
    u16 activeMTU; //we cannot handle lost segments if MTU has changed
    u16 rangingCounter;
    u16 startSegment;
    u16 endSegment;
} blt_rass_active_rascp_opcode_t;

/**
 * @brief the data structure of RAS server.
 */
typedef struct __attribute__((packed))
{
    u16 rasFeatureHandle;                 //ras feature attribute handle
    u16 realtimeProcedureDataHandle;      //real time procedure data attribute handle
    u16 realtimeProcedureCccHandle;       //real time procedure ccc handle
    u16 ondemandProcedureDataHandle;      //on demand procedure data attribute handle
    u16 ondemandProcedureCccHandle;       //on demand procedure ccc handle
    u16 controlPointDataHandle;           //control point attribute handle
    u16 controlPointCccHandle;            //control point ccc handle
    u16 rangingDataReadyDataHandle;       //ranging data ready attribute data handle
    u16 rangingDataReadyCccHandle;        //ranging data ready attribute ccc handle
    u16 rangingDataOverwrittenDataHandle; //ranging data overwritten attribute data handle
    u16 rangingDataOverwrittenCccHandle;  //ranging data overwritten attribute ccc handle
#if(Google_SRS)
    u16 googleSrsReadCapHandle;           //google specical ranging service read cap handle
    u16 googleSrsReadCapCccHandle;        //google specical ranging service read cap ccc handle
    u16 googleSrsSendCmdHandle;           //google specical ranging service send command handle
    u16 googleSrsSendCmdCccHandle;        //google specical ranging service send command ccc handle
#endif
} blt_rass_server_t;

/**
 * @brief RAS server metadata for currently active realtime procedure
 */
typedef struct __attribute__((packed))
{
    u16 rangingCounter;
    u16 realtimeCurrentSegmentIndex;
} blt_rass_active_realtime_t;

/**
 * @brief ras server nvinfo for storage
 */
typedef struct{
    u16 realtimeProcedureCccValue;        //real time procedure ccc value
    u16 ondemandProcedureCccValue;        //on demand procedure ccc value
    u16 rasControlPointCccValue;        //ras control point ccc value
    u16 rangingDataReadyCccValue;        //ranging data ready ccc value
    u16 rangingDataOverwrittenCccValue;    //stored ranging data overwritten ccc value
#if(RAS_PERSISTENT_FILTER)
    blt_ras_filter_t filter;
#endif
} blt_rass_nv_info_t;

/**
 * @brief the data structure of RAS server assigned to handling a single RAS client.
 */
typedef struct __attribute__((packed))
{
    blt_ras_prot_ctrl_t protCtrl[RAS_PROCEDURE_COUNT]; //gets allocated on ondemand subscription //can be optimized further with malloc/free only if needed
    s8 protStoredNum;
#if(TTF_EN)
    bool stopGetEnabled;
    bool stopRspEnabled;
#endif

    u16 realtimeProcedureCccValue;
    u16 ondemandProcedureCccValue;
    u16 rasControlPointCccValue;
    u16 rangingDataReadyCccValue;
    u16 rangingDataOverwrittenCccValue;

    u8                             procDataExchgMechanism; //blc_rass_procedure_data_exchange_mechanism_enum
    u8                             busy;
    u8                             realtime_subevent_busy;
    u8                             pendingIndCfm;          //for handling indication confirmations
    u8                            *dataNtfIndBuffer;
    blt_rass_active_rascp_opcode_t activeOnDemand;         //needed for proper Retrieve Lost Ranging Data Segments proces behaviour
    blt_rass_active_realtime_t     realtime;               //only needed for realtime
#if (RAS_REALTIME_PROT_FLOW)
    blt_ras_realtime_prot_ctrl_t   realtime_prot_flow;
#endif
} blt_rass_server_client_t;

/**
 * @brief the data structure of RAS server process controller.
 */
typedef struct __attribute__((packed)) blc_ras_server_ctrl
{
#if (defined(HOST_V2_ENABLE))
    struct blc_prf_process process;
#else
    blc_prf_proc_t     process;
#endif
    blt_rass_server_t         rasPrfServer;
    blt_rass_server_client_t *rasServerClients[LL_MAX_ACL_CEN_NUM + LL_MAX_ACL_PER_NUM];
} blt_ras_server_ctrl_t;

/**
 * @brief       informs RAS server that a new CS procedure has started
 * @param[in]   connHandle: ACL handle.
 * @return      BLE_SUCCESS - success
 *              other       - error
 */
ble_sts_t blt_rass_newProcedure(u16 connHandle);


#define member_sizeof(type, member) (sizeof(((type *)0)->member))

/******************************* ranging Profile server buff end **********************************************************************/
