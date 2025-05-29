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
 * @brief maximum expected number of lost segment entries - consecutive lost segments get stored as a single entry
 */
#define RAS_LOST_SEGMENT_RECORDS_COUNT 8

/**
 * @brief RAS characteristics read
 */
typedef enum {
    BLT_RASC_READ_RAS_FEATURE,
    BLT_RASC_READ_RAS_DATA_READY,
    BLT_RASC_READ_RAS_DATA_OVERWRITTEN,
    BLT_RASC_READ_MAX,
} blt_rasc_read_t;

/**
 * @brief the get lost segment command
 */
typedef struct __attribute__((packed))
{
    u16 rangingCounter;
    u8  segmentStartAsIndex;
    u8  segmentEndAsIndex;
} blt_rasc_get_lost_proc_segment_t;

/**
 * @brief buffer for storing lost segment records
 */
typedef struct __attribute__((packed))
{
    u16                              lostSegmentEntriesCount;
    blt_rasc_get_lost_proc_segment_t segment[RAS_LOST_SEGMENT_RECORDS_COUNT];
} blt_rasc_record_lost_segment_t;

/**
 * @brief a node type for storing segments incoming from RAS server on the RAS client side, before they get combined to full received procedure
 */
typedef struct __attribute__((packed)) ras_segment_node
{
    struct __attribute__((packed)) ras_segment_node *next;
    u8                                              *pData;
    u8                                               index;
    u8                                               dataLen;
} blt_rasc_segment_node_t;

/**
 * @brief the data structure of ranging procedure data.
 */
typedef struct __attribute__((packed))
{
    u16                            rangingCounter;
    u8                             finalSegmentReceived;
    u8                             segmentCount;
    u8                             realtimeDataLost;
    u16                            expectedSegmentAsIndex;
    u16                            rangingDataLen;
    u8                            *rangingData;
    blt_rasc_segment_node_t       *head;
    blt_rasc_record_lost_segment_t ras_segment;
} blt_rasc_ranging_proc_data_t;

/**
 * @brief lost segments control structure
 */
typedef struct __attribute__((packed))
{
    u8  lostSegmentsFlag;
    u16 expectedSegmentAsIndex;
    u16 rangingCounter;
    u16 segmentStartAsIndex;
    u16 segmentEndAsIndex;
} blt_rasc_get_lost_data_t;

/**
 * @brief the data structure for storing received ranging data and related lost segments metadata
 */
typedef struct __attribute__((packed))
{
    blt_rasc_get_lost_data_t     lost_data_ctl;
    blt_rasc_ranging_proc_data_t proc_data;
} blt_ras_ranging_data_t;

/**
 * @brief timeout event callback prototype
 */
typedef ble_sts_t (*blt_ras_timer_cb_t)(u16, u16, u8); //(u16, u16, blc_ras_timer_type_enum)

/**
 * @brief the data structure of register RAS Client parameter.
 */
typedef struct __attribute__((packed))
{
} blc_rasc_regParam_t;
