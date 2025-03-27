/********************************************************************************************************
 * @file    app_main_node.h
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
#ifndef APP_MAIN_NODE_H_
#define APP_MAIN_NODE_H_


#include "app_config.h"
#include "stack/ble/controller/ll/acl_conn/acl_sniffer/acl_sniffer.h"


#if (MAIN_NODE_ROLE_SELECT == MAIN_NODE_PERIPHERAL)

extern volatile u8 log_sniffer_enable;


#if (APP_TRANSPORT_UART_ENABLE)
extern u32 my_spp_rx_fifo_tick_record[];
extern my_fifo_t spp_rx_fifo;
#endif


#define         SNIFFER_CMD_SYNC_REQ            0xF9    //for STANDARD MODE
#define         SNIFFER_CMD_RSSI                0xFA
#define         SNIFFER_CMD_SYNC_RSP            0xFB

#define         SNIFFER_CMD_SYNC_PARAM_LEN      32
#define         SNIFFER_CMD_SYNC_REQ_DATA_LEN   SNIFFER_CMD_SYNC_PARAM_LEN + 5  // spp_main_node_cmd_sync_req_tx_t


typedef struct __attribute__((packed)) {
    u16     cmdId;
    u16     dataLen;
    u8      snifferIndex;
    u16     snifferHandle;
} spp_main_node_cmd_rx_t;

typedef struct __attribute__((packed)) {
    u16     cmdId;
    u16     dataLen;
    u8      snifferIndex;
    u16     snifferHandle;
    u8      rssi;
    u8      snifferChannel  :6;
    u8      deviceType      :2;
    u8      checksum;
} spp_main_node_cmd_rssi_rx_t;

typedef struct __attribute__((packed)) {
    u16     cmdId;
    u16     dataLen;
    u8      snifferIndex;
    u16     snifferHandle;
    u8      status;
    u8      checksum;
} spp_main_node_cmd_sync_rsp_rx_t;


typedef struct __attribute__((packed)) {
    u32     dmaLen;
    u16     cmdId;
    u16     dataLen;
    u32     transmit_time;  //unit: (1/24) us
    u8      param[SNIFFER_CMD_SYNC_PARAM_LEN];
    u8      checksum;
} spp_main_node_cmd_sync_req_tx_t;

extern _attribute_ble_data_retention_ volatile u8 receive_bus_rssi_flag;

/**
 * @brief      Update Sniffer listening status after connecting successfully.
 * @param[in]  connHandle       - connection handle.
 * @param[in]  role             - ACL Connection role,  1: ACL Peripheral role, 0: ACL Central role.
 * @return     none
 */
void snif_main_node_connection_setup(u16 connHandle, u8 role);

/**
 * @brief      Update Sniffer listening status list after disconnection.
 * @param[in]  connHandle       - connection handle.
 * @return     none
 */
void snif_main_node_disconnect(u16 connHandle);

/**
 * @brief      Update Sniffer listening status list when ACL param changes during connection.
 * @param[in]  connHandle       - connection handle.
 * @return     none
 */
void snif_main_node_connection_update(u16 connHandle);
/**
 * @brief      Process received data by CAN or UART.
 * @param[in]  none
 * @return     none
 */
void snif_main_node_rx_data_process(void);

/**
 * @brief      Control the data interaction(RX and TX) by CAN or UART.
 * @param[in]  none
 * @return     none
 */
void snif_main_node_control_process(void);


/**
 * @brief      sniffer main node initialization
 * @param[in]  none
 * @return     none.
 */
void snif_main_node_init(void);

#endif /* MAIN_NODE_ROLE_SELECT == MAIN_NODE_PERIPHERAL */

#endif /* APP_MAIN_NODE_H_ */
