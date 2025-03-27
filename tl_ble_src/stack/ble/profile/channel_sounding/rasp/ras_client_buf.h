/********************************************************************************************************
 * @file    ras_client_buf.h
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


/**
 * @brief the cmd of get lost procedure segment.
 */
typedef struct __attribute__((packed))  {
    u16 recordNumber;
    u16 segmentStart;
    u16 segmentEnd;
} ras_getLostProcSegment_t;

#define RECORD_LOST_SEGMENT_BUFF_LEN    8

/**
 * @brief the buffer of record lost segment.
 */
typedef struct {
    u16 index;
    ras_getLostProcSegment_t segment[RECORD_LOST_SEGMENT_BUFF_LEN];
} ras_recordLostSegment_t;

/**
 * @brief the data structure of ranging procedure data.
 */
typedef struct __attribute__((packed))  {
    u16 procedureCounter;
    u16 expectSegmentCounter;
    u16 rangingDataLen;
    u8 rangingData[4096];
    ras_recordLostSegment_t ras_segment;
} ras_rangingProcData_t;

#define RECORD_ONDEMAND_BUFF_LEN    1

/**
 * @brief the data structure of get lost procedure segment.
 */
typedef struct __attribute__((packed))  {
    u8  rcvSegmentLostStart;
    u16 bffIndex;
    u16 expectSegmentCounter;
    u16 recordNumber;
    u16 segmentStart;
    u16 segmentEnd;
} ras_getLostData_t;

/**
 * @brief the data structure of ranging data to application .
 */
typedef struct __attribute__((packed))  {
    u8 onDemandDataFlag;
    u16 rangingDataIndex;
    ras_getLostData_t   lost_data_ctl;
    ras_rangingProcData_t  proc_data[RECORD_ONDEMAND_BUFF_LEN];
} ras_rangingData_t;

/**
 * @brief the data structure of RAS Client basic info.
 */
typedef struct __attribute__((packed))  {
    gattc_sub_ccc_msg_t ntfInput;
    /* Characteristic value handle */
    u8 rasFeatureProperties;
    u16 rasFeatureHandle;

    u8 liveRangingDataProperties;       //live ranging data attribute properties
    u16 liveRangingDataHandle;          //live ranging data attribute handle

    u8 storedRangingDataProperties;     //stored ranging data attribute properties
    u16 storedRangingDataHandle;        //stored ranging data attribute handle

    u8 rasControlPointProperties;       //ras control point attribute properties
    u16 rasControlPointHandle;          //ras control point attribute handle

    u8 rangingDataReadyProperties;      //ranging data ready attribute properties
    u16 rangingDataReadyHandle;         //ranging data ready attribute handle

    u8 rangingDataOverwrittenProperties;//stored ranging Overwritten data attribute properties
    u16 rangingDataOverwrittenHandle;   //stored ranging Overwritten data attribute handle

    u8 recvState;

    svc_ras_feature_t ras_feature;
    ras_rangingData_t rang_data;

} blc_ras_client_t;

/**
 * @brief the data structure of register RAS Client parameter.
 */
typedef struct __attribute__((packed)) {

} blc_rasc_regParam_t;

