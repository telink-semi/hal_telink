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


#define RAS_CP_MSG_HEADER_SIZE                          2
#define RAS_CP_CHECK_CMD_OPCODE(opcode)         (opcode >= RAS_CP_CMD_MAX_OPCODE)


enum {
    RAS_ERR_NO_RECORDS_FOUND = 0x80,
    RAS_ERR_PROCEDURE_NOT_COMPLETED,
};

/**
 * @brief Ranging Data common.
 */
/**
 * @brief the data structure of Segmentation Header.
 */
typedef struct __attribute__((packed)) {
    u8 firstSegment:    1;
    u8 lastSegment:     1;
    u8 segmentCounter:  6;
} ras_segmentationHeader_t;


/**
 * @brief the data structure of Ranging Data.
 */
typedef struct __attribute__((packed)){
    ras_segmentationHeader_t header;
    u8 data[0];
} ras_raningData_t;

/**
 * @brief Live Ranging Data.
 */
/**
 * @brief the data structure of real-time Ranging Data.
 */
typedef struct __attribute__((packed)){
    ras_segmentationHeader_t header;
    u8 rangingDataBodySegment[0];
} ras_liveRaningData_t;

/**
 * @brief Stored Ranging Data.
 */
/**
 * @brief the data structure of on-demand Ranging Data.
 */
typedef struct __attribute__((packed)){
    u32 recordNumber;
    u8 rangingData[0];
} ras_storedRangingDataBody_t;

/**
 * @brief the data structure of Stored Ranging Data.
 */
typedef struct __attribute__((packed)){
    ras_segmentationHeader_t header;
    u8 storedRangingDataSegment[0];
} ras_storedRaningData_t;

/**
 * @brief RAS Control Point.
 */

/**
 * @brief RAS-CP command opcode enumerate.
 */
typedef enum{
    RAS_CP_CMD_OPCODE_GET_REPORT_RECORDS,
    RAS_CP_CMD_OPCODE_ACK_STORED_RECORDS,
    RAS_CP_CMD_OPCODE_GET_RECORD_SEGMENT,
    RAS_CP_CMD_OPCODE_ABORD_OPERATION,
    RAS_CP_CMD_MAX_OPCODE,
} blt_ras_cp_command_opcode_enum;

/**
 * @brief RAS-CP response opcode enumerate.
 */
typedef enum{
    RAS_CP_RSP_OPCODE_COMPLETE_REPORT_RECORDS,
    RAS_CP_RSP_OPCODE_COMPLETE_RECORD_SEGMENT,
} blt_ras_cp_response_opcode_enum;

/**
 * @brief RAS-CP operator enumerate.
 */
typedef enum{
    RAS_CP_OPERATOR_NULL,
    RAS_CP_OPERATOR_ONE_RECORD = 0x02,
    RAS_CP_OPERATOR_GREATER_OR_EQUAL,
} blt_ras_cp_operator_enum;

/**
 * @brief the operands structure of Get one Report Records.
 */
typedef struct __attribute__((packed))  {
    u16 procedureCounter;
} operands_getOneRecord_t;

/**
 * @brief the operands structure of Get Greater than or equal to Report Records.
 */
typedef struct __attribute__((packed))  {
    u16 recordNum;
} operands_getMoreRecord_t;

/**
 * @brief the operands structure of Ack one Stored Records.
 */
typedef struct __attribute__((packed))  {
    u16 procedureCounter;
} operands_ackStoredOneRecord_t;

/**
 * @brief the operands structure of Ack Greater than or equal to Stored Records.
 */
typedef struct __attribute__((packed))  {
    u16 recordNum;
} operands_ackStoredMoreRecord_t;

/**
 * @brief the operands structure of Get Record Segments.
 */
typedef struct __attribute__((packed))  {
    u16 recordNum;
    u16 startAbsoluteSegment;
    u16 endAbsoluteSegment;
} operands_getRecordSegments_t;

/**
 * @brief the operands structure of Abort Operation.
 */
typedef struct __attribute__((packed))  {
} operands_abortOperation_t;

/**
 * @brief the operands structure of Complete Report Records Response.
 */
typedef struct __attribute__((packed))  {
    u16 numOfRecords;
} operands_completeReportRecord_t;

/**
 * @brief the operands structure of Complete Record Segment Response.
 */
typedef struct __attribute__((packed))  {
    u16 recordNum;
    u16 startSegment;
    u16 endSegment;
} operands_completeRecordSegment_t;

/**
 * @brief the operands structure.
 */
typedef struct __attribute__((packed))  {
    union {
        u8 val[0];
        operands_getOneRecord_t getOneRecord;
        operands_getMoreRecord_t getMoreRecord;
        operands_ackStoredOneRecord_t ackStoredOneRecord;
        operands_ackStoredMoreRecord_t ackStoredMoreRecord;
        operands_getRecordSegments_t getRecordSegments;
        operands_abortOperation_t abortOperation;
        operands_completeReportRecord_t completeReportRecord;
        operands_completeRecordSegment_t completeRecordSegment;
    };
} ras_cp_operand_t;

/**
 * @brief the data structure of RAS-CP characteristic.
 */
typedef struct __attribute__((packed))  {
    u8 opcode;
    u8 operator;
    ras_cp_operand_t operand;
} ras_cp_msg_t;

/**
 * @brief the data structure of Ranging Data Ready.
 */
typedef struct __attribute__((packed))  {
    u16 procedureCounter;
} ras_dataReady_t;

/**
 * @brief the data structure of Ranging Data Overwritten.
 */
typedef struct __attribute__((packed))  {
    u16 procedureCounter;
} ras_dataOverwritten_t;

/**
 * @brief RAS client.
 */
typedef enum{
    RASC_RECV_STATE_NULL,
    RASC_RECV_STATE_LIVE_DATA,
    RASC_RECV_STATE_STORED_DATA,
} blt_rasc_recv_state_enum;

/**
 * @brief RAS server.
 */
#define RASS_MSG_TYPE_NULL                                  0
#define RASS_MSG_TYPE_REPORT_LIVE_RANGING_DATA              1
#define RASS_MSG_TYPE_REPORT_STORED_RANGING_DATA            2
#define RASS_MSG_TYPE_REPORT_RANGING_DATA_READY             3
#define RASS_MSG_TYPE_REPORT_RANGING_DATA_OVERWRITTEN       4
#define RASS_MSG_TYPE_COMPLETE_REPORT_RECORDS_RESPONSE      5
#define RASS_MSG_TYPE_COMPLETE_REPORT_SEGMENT_RESPONSE      6

typedef struct __attribute__((packed))  {
    u8* pData;
    u16 pDataLen;
    u16 segmentCount;
    u16 segmentIndex;
    u16 segmentEnd;
    u16 MTU;
} rass_report_liveRangingData_t, rass_report_storedRangingData_t, rass_report_rangingData_t;

typedef struct __attribute__((packed))  {
    u16 procedureCounter;
} rass_report_rangingDataReady_t, rass_report_rangingDataOverwritten_t;

typedef struct __attribute__((packed))  {
    u32 numOfRecords;
} rass_report_reportRecordsResponse_t;

typedef struct __attribute__((packed))  {
    u16 recordNumber;
    u16 segmentStart;
    u16 segmentEnd;
} rass_report_recordSegmentResponse_t;

typedef struct __attribute__((packed))  {
    struct __attribute__((packed)) rass_msg* next;
    u8 type;
    u16 connHandle;
    union{
        rass_report_liveRangingData_t live;
        rass_report_storedRangingData_t store;
        rass_report_rangingDataReady_t ready;
        rass_report_rangingDataOverwritten_t overwritten;
        rass_report_reportRecordsResponse_t records;
        rass_report_recordSegmentResponse_t segment;
    };
} rass_msg_t;


int blc_rass_procedureDataReady(u16 connHandle, u8 procedureCounter);
int blc_rass_procedureDataOverwritten(u16 connHandle, u16 procedureCounter);

void blt_rass_initialOnDemandProcedureData(u16 connHandle, u8* pData, u16 pDataLen ,u8 lost, u16 start_index , u16 end_index);
void blt_rass_initialCompleteProcedureDataResponse(u16 connHandle, u16 numOfRecords);
void blt_rass_initialCompleteLostProcedureSegmentResponse(u16 connHandle, u16 recordNumber, u16 segmentStart, u16 segmentEnd);



/******************************* ranging Profile Client Start **********************************************************************/

int blt_rasc_writeGetSpecificRecord(u16 connHandle, u32 procedureCounter, prf_write_cb_t writeCb);
int blt_rasc_writeGetRecordsGreaterEqual(u16 connHandle, u32 recordNum, prf_write_cb_t writeCb);
int blt_rasc_writeAckSpecificRecord(u16 connHandle, u32 procedureCounter, prf_write_cb_t writeCb);
int blt_rasc_writeAckRecordsGreaterEqual(u16 connHandle, u32 recordNum, prf_write_cb_t writeCb);
int blt_rasc_writeGetRecordSegments(u16 connHandle, u16 recordNum, u16 startSegment, u16 endSegment, prf_write_cb_t writeCb);

/******************************* ranging Profile Client end **********************************************************************/

/******************************* ranging Profile client buff start **********************************************************************/
/**
 * @brief the data structure of RAS Client process control.
 */
typedef struct __attribute__((packed))  blc_ras_client_ctrl{
    blc_prf_proc_t process;
    blc_ras_client_t* pRasPrfClient[STACK_PRF_ACL_CONN_MAX_NUM];
} blc_ras_client_ctrl_t;


/**
 * @brief       ranging profile get client control buffer.
 * @param[in]   instIndx: ACL connect index.
 * @return      ranging profile client control buffer pointer.
 */
blc_ras_client_t *blt_ras_prf_getClientControlBuffer(u8 instIdx);

/******************************* ranging Profile client buff end **********************************************************************/

/******************************* ranging Profile server buff Start **********************************************************************/

/**
 * @brief the data structure of RAS server.
 */
typedef struct __attribute__((packed))  {
    u16 rasFeatureHandle;               //ras feature attribute handle
    u16 liveRangingDataHandle;          //live ranging data attribute handle
    u16 storedRangingDataHandle;        //stored ranging data attribute handle
    u16 controlPointHandle;             //control point attribute handle
    u16 rangingDataReadyHandle;         //ranging data ready attribute handle
    u16 rangingDataOverwrittenHandle;   //ranging data overwritten attribute handle
    u8 procDataExchgMechanism;
} blc_ras_server_t;

/**
 * @brief the data structure of RAS server process controller.
 */
typedef struct __attribute__((packed))  blc_ras_server_ctrl{
    blc_prf_proc_t process;
    blc_ras_server_t rasPrfServer;
} blc_ras_server_ctrl_t;


/******************************* ranging Profile server buff end **********************************************************************/

/******************************* ranging Profile server data Start **********************************************************************/
/*
 +------------------------------------------------------------------------------------+
 | recordNumber | procedure header | subevent header |   32*mode0   |    224*mode1    |
 |--------------+------------------+-----------------+--------------+-----------------|
 |     4byte    |       5byte      |  32*10=320byte  | 32*8=256byte | 224*15=3360byte |
 +--------------+------------------+-----------------+--------------+-----------------+
 *4+5+320+256+3360 = 3945 byte
 */
#define PROCEDURE_DATA_LEN                  (4*1024)
#define PROCEDURE_COUNT                     (2)
#define SIZELIMIT                           (2048)

#define PROCEDURE_RECEIVE_UART              (0)
#define PROCEDURE_RECEIVE_BLE               (1)
#define SUBEVENT_RESULT_OVERFLOW_CHECK      (1)
#define PROCEDURE_HEAD_LEN                  (3)
#define SUBEVENT_HEAD_LEN                   (9)
#define STEP_HEAD_LEN                       (3)
#define PROCEDURE_COUNTER_SIZE              (2)

extern u8 ras_data_buf[];
extern u8 procedure_ctrl_buf[];

#if PROCEDURE_RECEIVE_UART
extern u8 ras_data_buf_uart[];
extern u8 procedure_ctrl_buf_uart[];
#endif

extern u8 ras_data_buf_ble[];
extern u8 procedure_ctrl_buf_ble[];

/**
 * @brief the state of CS profile.
 */
typedef enum{
    CS_RAS_SUCCESS,
    CS_RAS_NOT_FOUND,
    CS_PROC_DONE = 0x00,
    CS_PROC_MORE,
    CS_PROC_ABORT = 0x0f,
    CS_SUBEVT_DONE = 0x00,
    CS_SUBEVT_MORE,
    CS_SUBEVT_ABORT = 0x0f,
}cs_done_state_enum;

/**
 * @brief the data structure of procedure data body part.
 */
typedef struct __attribute__((packed))  {
    u8  subeventIndex;
    u16 startAclConnEvent;
    u16 frequencyCompensation;
    u8  procedureDoneStatus;
    u8  subeventDoneStatus;
    u8  referencePowerLevel;
    u8  numStepsReported;
    u8  pSubeventRangingData[0];
}blc_rass_data_body_t;

/**
 * @brief the data structure of procedure data body part.
 */
typedef struct __attribute__((packed))  {
    u8  proCountCfgID;
    u8  selectedTxPower;
    u8  numAntennaPaths;
    blc_rass_data_body_t    pSubEvtData[0];
}blc_rass_data_t;

/**
 * @brief the data structure of procedure data body.
 */
typedef struct __attribute__((packed))  {
    u16 procedureCounter;
    blc_rass_data_t rangingData[0];
}blc_rass_stored_data_t;

/**
 * @brief the data structure of procedure data store basic info.
 */
typedef struct __attribute__((packed))  {
    u8  procIndex;
    u8* procStartaddr;
    u16 subEvtsNums;
    u8* subeventPtr;
    u16 procDataLen;
}blc_rass_describle_t;

/**
 * @brief the data structure of procedure data store.
 */
typedef struct __attribute__((packed))  {
    u8  storedNum;
    u32 storedTotalSize;
    blc_rass_describle_t procDataDes[0];
}blc_rass_data_ctrl_t;

/**
 * @brief the data structure of procedure data header info.
 */
typedef struct __attribute__((packed))  {
    u16 procedureCounter;
    u8  proCountCfgID;
    u8  selectedTxPower;
    u8  numAntennaPaths;
}blc_rass_proc_head_t;

/**
 * @brief the data structure of procedure data parse.
 */
typedef struct __attribute__((packed))  {
    u8  status; //1: found index, 0: not fuond index
    u16 index;
    u16 procLen;
    u8* procPtr;
}blc_rass_query_result_t;

/**
 * @brief     for get real-time procedure data address and length info.
 * @param[in] index: the counter info of procedure.
 * @return    blc_rass_query_result_t.
 */
blc_rass_query_result_t blc_rass_procedureQueryIndexLiving(u16 index);

/**
 * @brief     for get on-demand procedure data address and length info.
 * @param[in] index: the counter info of procedure.
 * @return    blc_rass_query_result_t.
 */
blc_rass_query_result_t blc_rass_procedureQueryIndexStored(u16 index);

/**
 * @brief     for delete procedure data.
 * @param[in] index: the counter info of procedure.
 * @return    int.
 */
int blc_rass_procedureDeleteIndex(u16 index);


/******************************* ranging Profile server data end **********************************************************************/
