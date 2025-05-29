/********************************************************************************************************
 * @file    app_sub_node.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    2020.06
 *
 * @par     Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
 *
 *          Redistribution and use in source and binary forms, with or without
 *          modification, are permitted provided that the following conditions are met:
 *
 *              1. Redistributions of source code must retain the above copyright
 *              notice, this list of conditions and the following disclaimer.
 *
 *              2. Unless for usage inside a TELINK integrated circuit, redistributions
 *              in binary form must reproduce the above copyright notice, this list of
 *              conditions and the following disclaimer in the documentation and/or other
 *              materials provided with the distribution.
 *
 *              3. Neither the name of TELINK, nor the names of its contributors may be
 *              used to endorse or promote products derived from this software without
 *              specific prior written permission.
 *
 *              4. This software, with or without modification, must only be used with a
 *              TELINK integrated circuit. All other usages are subject to written permission
 *              from TELINK and different commercial license may apply.
 *
 *              5. Licensee shall be solely responsible for any claim to the extent arising out of or
 *              relating to such deletion(s), modification(s) or alteration(s).
 *
 *          THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *          ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *          WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *          DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY
 *          DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *          (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *          LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *          ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *          (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *          SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************************************/
#ifndef APP_SUB_NODE_H_
#define APP_SUB_NODE_H_


#include "app_config.h"
#include "stack/ble/controller/ll/acl_conn/acl_sniffer/acl_sniffer.h"
#include "stack/ble/controller/ll/chn_sound/cs_sniffer/cs_sniffer.h"


#if (MONITOR_ROLE_SELECT == MONITOR_CS_CENTRAL_PERIPHERAL)


    #if (APP_TRANSPORT_UART_ENABLE)
extern u32       my_spp_rx_fifo_tick_record[];
extern my_fifo_t spp_rx_fifo;
    #endif


    #define SNIFFER_CMD_SYNC_REQ                                        0xF9 //for STANDARD MODE
    #define SNIFFER_CMD_RSSI                                            0xFA
    #define SNIFFER_CMD_SYNC_RSP                                        0xFB

    #define SNIFFER_CMD_RSSI_DATA_LEN                                   6 // spp_sniffer_cmd_rssi_tx_t
    #define SNIFFER_CMD_SYNC_RSP_DATA_LEN                               5 // spp_sniffer_cmd_sync_rsp_tx_t

    #define SNIFFER_CMD_CS_EVENT                                        0xD1
    #define SNIFFER_CMD_CS_HCI_EVENT_PROCEDURE_SUBEVENT_RESULT          0xD2
    #define SNIFFER_CMD_CS_HCI_EVENT_PROCEDURE_SUBEVENT_RESULT_CONTINUE 0xD4
    #define SNIFFER_CMD_CS_RAS_CLIENT_DATA_EVENT                        0xD6
    #define SNIFFER_CMD_CS_RAS_SERVER_DATA_EVENT                        0xD8
    #define SNIFFER_CMD_CS_DISTANCE                                     0xDA

    #define FLASH_ADDRESS_SNIFFER_SETTING                               0xA0000
    #define FLASH_ADDRESS_CS_PARAM_SETTING                              0xA1000

typedef struct
{
    u16 cmdId;
    u16 dataLen;
} spp_sub_node_cmd_rx_t;

typedef struct __attribute__((packed))
{
    u16 cmdId;
    u16 dataLen;
    u32 transmit_time;   //unit: (1/24) us
    u8  syncHandle;
    u8  rsvd1[3];
    u32 expectTime_time; //unit: (1/24) us
    u8  rsvd2[12];
    u32 sync_timeout;    //unit: (1/24) us
    u8  rsvd3[8];
    u8  checksum;
} spp_sub_node_cmd_sync_req_rx_t;

typedef struct __attribute__((packed))
{
    u32 dmaLen;
    u16 cmdId;
    u16 dataLen;
    u8  snifferIndex;
    u16 snifferHandle;
    u8  rssi;
    u8  snifferChannel : 6;
    u8  deviceType     : 2;
    u8  checksum;
} spp_sub_node_cmd_rssi_tx_t;

typedef struct __attribute__((packed))
{
    u32 dmaLen;
    u16 cmdId;
    u16 dataLen;
    u8  snifferIndex;
    u16 snifferHandle;
    u8  status;
    u8  checksum;
} spp_sub_node_cmd_sync_rsp_tx_t;

typedef struct __attribute__((packed))
{
    u32 dmaLen;
    u16 cmdId;
    u16 dataLen;
    u8  snifferIndex;
    u16 snifferHandle;
} spp_sub_node_cmd_tx_t;

typedef struct __attribute__((packed))
{
    u16 cmdId;
    u16 dataLen;
    u8  snifferIndex;
    u16 snifferHandle;
    u8  cs_event_data[1]; // non-fixed length
    //u8    checksum;      // last byte
} spp_sub_node_cmd_cs_event_rx_t;

typedef struct __attribute__((packed))
{
    u16 cmdId;
    u16 dataLen;
    u8  snifferIndex;
    u16 snifferHandle;
    u8  pData[0];
} cmd_cs_subevent_t;

typedef struct __attribute__((packed))
{
    u32               dmaLen;
    cmd_cs_subevent_t data;
} spp_cmd_cs_subevent_t;

typedef struct __attribute__((packed))
{
    u16 cmdId;
    u16 dataLen;
    u8  snifferIndex;
    u16 snifferHandle;
    u8  packetIdx;
    u8  pData[0];
} cmd_cs_event_slipt_t;

typedef struct __attribute__((packed))
{
    u32                  dmaLen;
    cmd_cs_event_slipt_t data;
} spp_cmd_cs_event_slipt_t;

typedef struct __attribute__((packed))
{
    u16 cmdId;
    u16 dataLen;
    u8  snifferIndex;
    u16 snifferHandle;
    u16 rangingCounter;
    u8  pData[0];
} cmd_cs_ras_event_t;

typedef struct __attribute__((packed))
{
    u32                dmaLen;
    cmd_cs_ras_event_t data;
} spp_cmd_cs_ras_event_t;

typedef struct __attribute__((packed))
{
    u32 dmaLen;
    u16 cmdId;
    u16 dataLen;
    u8  snifferIndex;
    u16 snifferHandle;
    u16 rangingCounter;
    float distance;
    u8  checksum;
} spp_sub_node_cmd_cs_distanc_tx_t;

typedef struct __attribute__((packed))
{
    u16 subNodeNumber; /* for main node, range from 0 to 6 */
    u16 syncReqId;     /* for main node, MAIN_NODE_TO_SUB_NODE_SYNC */
    u16 reportIntvl;   /* for all node, unit: 1ms */
    u16 reportId;      /* for main node, MAIN_NODE_TO_ECU_REPORT */
    u16 deviceIdx;     /* for sub node, range from 0 to 5 */
    u16 dataId;        /* for sub node, SUB_NODE_TO_MAIN_NODE_DATA */
    u16 rspSyncId;     /* for sub node, SUB_NODE_TO_MAIN_NODE_RSP */
    u16 u16_rsvd1;     /* for sub node */
} node_setting_t;

typedef struct __attribute__((packed))
{
    u16 procedureInterval; /* for CS Procedure Interval, Number of ACL connection events */
    u16 subeventLen;       /* for reserved, CS Subevent Len, unit: 1us, Range: 1250 us to 65000 us */
    u8  rangingAlgMode;    /* for CS ranging algorithm mode, reference to blc_ranging_algorithm_enum */
    u8  kalmanNoiseCov;    /* for CS distance kalman filter parameter proc_noise_cov, unit: 0.0001, Range: 1 to 200 */
    u8  channelMap[10];    /* for CS Channel Map, This parameter contains 80 1-bit fields */
    s16 distanceOffset;    /* for CS distance offset, unit: 10cm, Range: -500 to 500 */
    u8  aclRfPowerIdx;     /* for ACL RF power, unit: 1 dBm, Range: 0 to 10, convert to ACL rf power level index */
    u8  csRfPowerLevel;    /* for CS RF power, unit: 1 dBm, Range: 0 to 10, convert to CS rf power level */
} app_cs_param_setting_t;

extern _attribute_ble_data_retention_ app_cs_param_setting_t appCsParamSetting;


u8 check_sum(u8 *pdata, u32 len);

void snif_sub_node_cs_distacne_process(u16 connHandle, u16 rangingCounter, float distance1, float distance2);

/**
 * @brief      Process received data by CAN or UART.
 * @param[in]  none
 * @return     none
 */
void snif_sub_node_rx_data_process(void);

/**
 * @brief      Control the data interaction(RX and TX) by CAN or UART.
 * @param[in]  none
 * @return     none
 */
void snif_sub_node_control_process(void);

/**
 * @brief      CS Subevent result HCI Event.
 * @param[in]  param            - CS  subevent result parameter, refer to 'hci_le_csSubeventResultEvt_t'.
 * @return     none
 */
void snif_sub_node_cs_procedure_subevent_result_event(u8 *param);

/**
 * @brief      CS Subevent result continue HCI Event.
 * @param[in]  param            - CS  subevent result continue parameter, refer to 'hci_le_csSubeventResultContinueEvt_t'.
 * @return     none
 */
void snif_sub_node_cs_procedure_subevent_result_continue_event(u8 *param);

/**
 * @brief      sniffer sub node initialization
 * @param[in]  none
 * @return     none.
 */
void snif_sub_node_init(void);

#endif /* MONITOR_ROLE_SELECT == MONITOR_CS_CENTRAL_PERIPHERAL */

#endif /* APP_SUB_NODE_H_ */
