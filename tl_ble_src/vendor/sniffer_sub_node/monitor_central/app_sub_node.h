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


#if (MONITOR_ROLE_SELECT == MONITOR_CENTRAL)


#if (APP_TRANSPORT_UART_ENABLE)
extern u32 my_spp_rx_fifo_tick_record[];
extern my_fifo_t spp_rx_fifo;
#endif


#define         SNIFFER_CMD_SYNC_REQ            0xF9    //for STANDARD MODE
#define         SNIFFER_CMD_RSSI                0xFA
#define         SNIFFER_CMD_SYNC_RSP            0xFB

#define         SNIFFER_CMD_RSSI_DATA_LEN       6   // spp_sniffer_cmd_rssi_tx_t
#define         SNIFFER_CMD_SYNC_RSP_DATA_LEN   5   // spp_sniffer_cmd_sync_rsp_tx_t


typedef struct __attribute__((packed)) {
    u16     cmdId;
    u16     dataLen;
    u32     transmit_time;  //unit: (1/24) us
    u8      syncHandle;
    u8      rsvd1[3];
    u32     expectTime_time;//unit: (1/24) us
    u8      rsvd2[12];
    u32     sync_timeout;   //unit: (1/24) us
    u8      rsvd3[8];
    u8      checksum;
} spp_sub_node_cmd_sync_req_rx_t;

typedef struct __attribute__((packed)) {
    u32     dmaLen;
    u16     cmdId;
    u16     dataLen;
    u8      snifferIndex;
    u16     snifferHandle;
    u8      rssi;
    u8      snifferChannel  :6;
    u8      deviceType      :2;
    u8      checksum;
} spp_sub_node_cmd_rssi_tx_t;

typedef struct __attribute__((packed)) {
    u32     dmaLen;
    u16     cmdId;
    u16     dataLen;
    u8      snifferIndex;
    u16     snifferHandle;
    u8      status;
    u8      checksum;
} spp_sub_node_cmd_sync_rsp_tx_t;


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
 * @brief      sniffer sub node initialization
 * @param[in]  none
 * @return     none.
 */
void snif_sub_node_init(void);

#endif /* MONITOR_ROLE_SELECT == MONITOR_CENTRAL */

#endif /* APP_SUB_NODE_H_ */
