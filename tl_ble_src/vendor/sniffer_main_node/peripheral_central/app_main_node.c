/********************************************************************************************************
 * @file    app_main_node.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "../default_att.h"

#include "app.h"
#include "app_ui.h"
#include "app_main_node.h"
#if (APP_TRANSPORT_CANFD_ENABLE)
    #include "../tcan4x5x/TCAN4550.h"
#endif

#if (MAIN_NODE_ROLE_SELECT == MAIN_NODE_PERIPHERAL_CENTRAL)

_attribute_ble_data_retention_ u32 my_spp_rx_fifo_tick_record[SPP_RXFIFO_NUM];
_attribute_ble_data_retention_ u8 __attribute__((aligned(4))) spp_rx_fifo_b[SPP_RXFIFO_SIZE * SPP_RXFIFO_NUM] = {0};
_attribute_ble_data_retention_ my_fifo_t                      spp_rx_fifo                                     = {
    SPP_RXFIFO_SIZE,
    SPP_RXFIFO_NUM,
    0,
    0,
    spp_rx_fifo_b,
};

_attribute_ble_data_retention_ u8 __attribute__((aligned(4))) spp_tx_fifo_b[SPP_TXFIFO_SIZE * SPP_TXFIFO_NUM] = {0};
_attribute_ble_data_retention_ my_fifo_t                      spp_tx_fifo                                     = {
    SPP_TXFIFO_SIZE,
    SPP_TXFIFO_NUM,
    0,
    0,
    spp_tx_fifo_b,
};

_attribute_ble_data_retention_ volatile u8 log_sniffer_enable    = 1;
_attribute_ble_data_retention_ volatile u8 receive_bus_rssi_flag = 0;

    #if (APP_TRANSPORT_CANFD_ENABLE)
_attribute_ble_data_retention_ u32 can_sleep_pending_tick = 0;
_attribute_ble_data_retention_ u32 rx_rssi_tick[REMOTE_DEVICE_MAX_NUM][CHECK_SNIFFER_INDEX_MAX];
_attribute_ble_data_retention_ u8  rssi_buffer[REMOTE_DEVICE_MAX_NUM][1 + 1 + 2 * CHECK_SNIFFER_INDEX_MAX]; // (main_node_connHandle+main_node_rssi+(sub_node_id+sub_node_rssi)*CHECK_SNIFFER_INDEX_MAX)*REMOTE_DEVICE_MAX_NUM
    #endif

_attribute_ble_data_retention_ u8  rssi_crtl[SPP_TXFIFO_SIZE];
_attribute_ble_data_retention_ u8  connection_status_update_flag[REMOTE_DEVICE_MAX_NUM];
_attribute_ble_data_retention_ u32 connection_status_update_tick[REMOTE_DEVICE_MAX_NUM];
_attribute_ble_data_retention_ u8  connection_status_check_flag[REMOTE_DEVICE_MAX_NUM];
_attribute_ble_data_retention_ u8  connection_status_update_instantly[REMOTE_DEVICE_MAX_NUM];

    #if (APP_TRANSPORT_UART_ENABLE)
_attribute_ble_data_retention_ u8 uart_dma_send_done_flag = 1;
    #endif

    #if (APP_CAN_PM_ENABLE)
int canfd_send_data_handle(u16 sid, u8 *pData, u32 len);
    #endif

void snif_main_node_connection_setup(u16 connHandle, u8 role)
{
    if ((role == ACL_ROLE_PERIPHERAL) || (role == ACL_ROLE_CENTRAL)) {
        u8 idx = dev_char_get_conn_index_by_connhandle(connHandle);
        if (idx == INVALID_CONN_IDX) {
            return;
        } else {
            idx &= REMOTE_DEVICE_MAX_MASK;
            if (idx >= REMOTE_DEVICE_MAX_NUM) {
                return;
            }
        }

        connection_status_update_flag[idx] = connHandle;
        connection_status_update_tick[idx] = clock_time();
        foreach (j, CHECK_SNIFFER_INDEX_MAX) {
            connection_status_check_flag[idx] |= BIT(j);
        }
    }

    #if (APP_TRANSPORT_CANFD_ENABLE && APP_CAN_PM_ENABLE)
    if (gpio_read(TCAN4550_GPIO_WKREQ_N) == 1) {
        /* tcan4550 in sleep mode, reset */
        tcan4550_reset_hw();
        Init_CAN();
        can_sleep_pending_tick = 0;
        /* wake-up sub node */
        u8 temp[4] = {0x03, 0x03, 0x03, 0x03};
        canfd_send_data_handle(0x5555, temp, 4);
    }
    #endif
}

void snif_main_node_disconnect(u16 connHandle)
{
    extern u16 sniffer_unpair_enable;
    if (sniffer_unpair_enable) {
    // delete this device information(mac_address and distributed keys...) on FLash
    #if (ACL_CENTRAL_SMP_ENABLE)
        dev_char_info_t *dev_info = dev_char_info_search_by_connhandle(connHandle);
        if (dev_info) {
            blc_smp_deleteBondingPeripheralInfo_by_PeerMacAddress(dev_info->peer_adrType, dev_info->peer_addr);
        }
    #endif
        sniffer_unpair_enable = 0;
    }

    u8 idx = dev_char_get_conn_index_by_connhandle(connHandle);
    if (idx == INVALID_CONN_IDX) {
        return;
    } else {
        idx &= REMOTE_DEVICE_MAX_MASK;
        if (idx >= REMOTE_DEVICE_MAX_NUM) {
            return;
        }
    }
    connection_status_update_flag[idx] = 0;
    #if (APP_TRANSPORT_CANFD_ENABLE)
    u8 *ptr = rssi_buffer[idx];
    blc_app_memory_set(ptr, 0xFF, CHECK_SNIFFER_INDEX_MAX * 2 + 1 + 1, sizeof(rssi_buffer) / REMOTE_DEVICE_MAX_NUM, 0x11270000 | __LINE__);
    #endif
}

void snif_main_node_connection_update(u16 connHandle)
{
    u8 idx = dev_char_get_conn_index_by_connhandle(connHandle);
    if (idx == INVALID_CONN_IDX) {
        return;
    } else {
        idx &= REMOTE_DEVICE_MAX_MASK;
        if (idx >= REMOTE_DEVICE_MAX_NUM) {
            return;
        }
    }
    connection_status_update_flag[idx] = connHandle;
    connection_status_update_tick[idx] = clock_time();
    foreach (j, CHECK_SNIFFER_INDEX_MAX) {
        connection_status_check_flag[idx] |= BIT(j);
    }
    connection_status_update_instantly[idx] = connHandle;
}

/**
 * @brief      callBack function of LinkLayer Event "BLT_EV_FLAG_CHANNEL_MAP_UPDATE"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
_attribute_ram_code_ void user_channel_map_update(u8 e, u8 *p, int n)
{
    (void)e;
    (void)n;
    acl_channel_map_updateEvt_t *pa = (acl_channel_map_updateEvt_t *)p;

    u16 connHandle = pa->connHandle;

    u8 idx = dev_char_get_conn_index_by_connhandle(connHandle);
    if (idx == INVALID_CONN_IDX) {
        return;
    } else {
        idx &= REMOTE_DEVICE_MAX_MASK;
        if (idx >= REMOTE_DEVICE_MAX_NUM) {
            return;
        }
    }
    connection_status_update_flag[idx] = connHandle;
    connection_status_update_tick[idx] = clock_time();
    foreach (j, CHECK_SNIFFER_INDEX_MAX) {
        connection_status_check_flag[idx] |= BIT(j);
    }
    connection_status_update_instantly[idx] = connHandle;
}


    #if (APP_TRANSPORT_CANFD_ENABLE)
        #define RSSI_FILTER_NUM_LESS 5
        #define RSSI_FILTER_NUM_MORE 10
        #define RSSI_NUM             RSSI_FILTER_NUM_LESS
        #if (RSSI_NUM != 5 && RSSI_NUM != 10)
            #error "RSSI_FILTER_NUM set error !!!"
        #endif
_attribute_ram_code_ u8 rssi_filter(u8 idx, u8 rssi)
{
    static u8 store_rssi_value[REMOTE_DEVICE_MAX_NUM][RSSI_NUM];
    static u8 sort_rssi_value[REMOTE_DEVICE_MAX_NUM][RSSI_NUM];
    static u8 rssi_index[REMOTE_DEVICE_MAX_NUM];

    idx &= REMOTE_DEVICE_MAX_MASK;
    if (idx >= REMOTE_DEVICE_MAX_NUM) {
        return 0;
    }

    //store
    if (rssi_index[idx] < RSSI_NUM) {
        store_rssi_value[idx][rssi_index[idx]] = rssi;
        rssi_index[idx]++;
        return rssi;
    } else {
        for (u8 i = 0; i < RSSI_NUM - 1; i++) {
            store_rssi_value[idx][i] = store_rssi_value[idx][i + 1];
        }
        store_rssi_value[idx][RSSI_NUM - 1] = rssi;
    }

    for (u8 cnt = 0; cnt < RSSI_NUM; cnt++) {
        sort_rssi_value[idx][cnt] = store_rssi_value[idx][cnt];
    }

    //sort
    u8 i = 0, j = 0;
    u8 tmp_sort = 0;
    for (i = 0; i < RSSI_NUM - 1; i++) {
        for (j = 0; j < RSSI_NUM - 1 - i; j++) {
            if (sort_rssi_value[idx][j] > sort_rssi_value[idx][j + 1]) {
                tmp_sort                    = sort_rssi_value[idx][j];
                sort_rssi_value[idx][j]     = sort_rssi_value[idx][j + 1];
                sort_rssi_value[idx][j + 1] = tmp_sort;
            }
        }
    }

    u16 rssi_gauss_average;
        #if (1)
    //Maximum value
    rssi_gauss_average = sort_rssi_value[idx][RSSI_NUM - 1];
        #elif (0)
            //smoothness in the center
            #if (RSSI_NUM == RSSI_FILTER_NUM_MORE)
    rssi_gauss_average = (sort_rssi_value[idx][2] + 2 * sort_rssi_value[idx][3] + 3 * sort_rssi_value[idx][4] + 8 * sort_rssi_value[idx][5] + 3 * sort_rssi_value[idx][6] + 2 * sort_rssi_value[idx][7] + sort_rssi_value[idx][8]) / 20;
            #elif (RSSI_NUM == RSSI_FILTER_NUM_LESS)
    rssi_gauss_average = (sort_rssi_value[idx][0] + 3 * sort_rssi_value[idx][1] + 8 * sort_rssi_value[idx][2] + 3 * sort_rssi_value[idx][3] + sort_rssi_value[idx][4]) / 16;
            #endif
        #endif

    return rssi_gauss_average;
}

void canfd_rxdata_handle(u8 *data, u8 len)
{
    u8 *p                                        = spp_rx_fifo.p + spp_rx_fifo.wptr * spp_rx_fifo.size;
    my_spp_rx_fifo_tick_record[spp_rx_fifo.wptr] = clock_time();
    blc_app_memory_copy(p, data, len, SPP_RXFIFO_SIZE, 0x11290000 | __LINE__);
    (spp_rx_fifo.wptr == (spp_rx_fifo.num - 1)) ? spp_rx_fifo.wptr = 0 : spp_rx_fifo.wptr++;
}

int canfd_send_data_handle(u16 sid, u8 *pData, u32 len)
{
    int state;

    state = can_fd_data_send(sid, pData, len);
    if (state != 0) {
        //Fail
        tlkapi_printf(APP_CAN_LOG_EN, "[APP][CAN] can_fd_data_send error:%d, ID:0x%x\n", state, sid);
    }

    return state;
}
    #endif


void snif_main_node_rx_data_process(void)
{
    if (spp_rx_fifo.wptr != spp_rx_fifo.rptr) {
        spp_main_node_cmd_rx_t *rx_common_cmd = (spp_main_node_cmd_rx_t *)(spp_rx_fifo.p + spp_rx_fifo.rptr * spp_rx_fifo.size);
        spp_rx_fifo.rptr == (spp_rx_fifo.num - 1) ? spp_rx_fifo.rptr = 0 : spp_rx_fifo.rptr++;
        //tlkapi_send_string_data(APP_SNIF_LOG_EN, "[APP][SNIF] Rx", (u8*)&rx_common_cmd->cmdId, rx_common_cmd->dataLen+4);

    #if (UI_LED_ENABLE)
        //rx data from bus
        gpio_toggle(GPIO_LED_BLUE);
    #endif

        u8 idx = dev_char_get_conn_index_by_connhandle(rx_common_cmd->snifferHandle);
        if (idx == INVALID_CONN_IDX) {
            return;
        } else {
            idx &= REMOTE_DEVICE_MAX_MASK;
            if (idx >= REMOTE_DEVICE_MAX_NUM) {
                return;
            }
        }

        if (rx_common_cmd->cmdId == SNIFFER_CMD_RSSI) {
            spp_main_node_cmd_rssi_rx_t *common_cmd = (spp_main_node_cmd_rssi_rx_t *)rx_common_cmd;

            u8  rx_snifferIndex   = common_cmd->snifferIndex;
            u16 rx_snifferHandle  = common_cmd->snifferHandle;
            u8  rx_rssi           = common_cmd->rssi;
            u8  rx_deviceType     = common_cmd->deviceType;
            u8  rx_snifferChannel = common_cmd->snifferChannel;

            u8 checkSum = 0;
            checkSum += common_cmd->cmdId;
            checkSum += common_cmd->dataLen;
            checkSum += common_cmd->snifferIndex;
            checkSum += common_cmd->snifferHandle;
            checkSum += common_cmd->rssi;
            checkSum += (common_cmd->deviceType << 6) | common_cmd->snifferChannel;

            if (checkSum == common_cmd->checksum) {
                if (log_sniffer_enable) {
                    tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] RSSI:0x%x,idx_%d,%d,chl:%d,rssi:%d\n", rx_snifferHandle, rx_snifferIndex, rx_deviceType, rx_snifferChannel, rx_rssi - 110);
                }
    #if (APP_TRANSPORT_CANFD_ENABLE)
                u8 slaverssi = blc_ll_getAclLatestAvgRSSI(rx_snifferHandle);
                slaverssi    = rssi_filter(rx_snifferHandle & 0x0F, slaverssi);

                u8 *ptr = rssi_buffer[idx];
                ptr[0]  = rx_snifferHandle;
                ptr[1]  = slaverssi - 110;
                ptr     = ptr + (rx_snifferIndex * 2 + 2);
                ptr[0]  = rx_snifferIndex;
                ptr[1]  = rx_rssi - 110;

                static u32 loss_packet_tick        = 0;
                rx_rssi_tick[idx][rx_snifferIndex] = 1;
                //if(clock_time_exceed(loss_packet_tick, (can_fd_cfg.reportInterval*3+50)*1000))
                if (clock_time_exceed(loss_packet_tick, (100 * 3 + 50) * 1000)) //TODO nodeSetting.reportIntvl
                {
                    loss_packet_tick = clock_time();

                    for (u32 i = 0; i < REMOTE_DEVICE_MAX_NUM; i++) {
                        for (u32 j = 0; j < CHECK_SNIFFER_INDEX_MAX; j++) {
                            if (rx_rssi_tick[i][j] == 0) {
                                ptr    = rssi_buffer[i];
                                ptr    = ptr + (j * 2 + 2);
                                ptr[1] = 0;
                            }
                        }
                    }

                    memset((u8 *)rx_rssi_tick, 0, sizeof(rx_rssi_tick));
                }

                receive_bus_rssi_flag = 1;
    #endif
            }
        } else if (rx_common_cmd->cmdId == SNIFFER_CMD_SYNC_RSP) {
            spp_main_node_cmd_sync_rsp_rx_t *common_cmd = (spp_main_node_cmd_sync_rsp_rx_t *)rx_common_cmd;

            u8 checkSum = 0;
            checkSum += common_cmd->cmdId;
            checkSum += common_cmd->dataLen;
            checkSum += common_cmd->snifferIndex;
            checkSum += common_cmd->snifferHandle;
            checkSum += common_cmd->status;

            if (checkSum == common_cmd->checksum) {
                if (common_cmd->status == SNIFFER_SYNC_CREATE) {
                    //                  connection_status_check_flag[idx] &= ~BIT(common_cmd->snifferIndex);
                    if (!connection_status_check_flag[idx]) {
                        connection_status_update_flag[idx] = 0;
                    }
                } else if ((common_cmd->status == SNIFFER_SEEK_IN_PROGRESS) || (common_cmd->status == SNIFFER_USER_STOP_EFFECTIVE)) {
                } else {
                    if (dev_char_info_is_connection_state_by_conn_handle(common_cmd->snifferHandle)) {
                        connection_status_update_flag[idx] = common_cmd->snifferHandle;
                        connection_status_update_tick[idx] = clock_time();
                        if (common_cmd->snifferIndex < CHECK_SNIFFER_INDEX_MAX) {
                            connection_status_check_flag[idx] |= BIT(common_cmd->snifferIndex);
                        }
                        connection_status_update_instantly[idx] = common_cmd->snifferHandle;
                    }
                }

                if (log_sniffer_enable) {
                    tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] SYNC_RSP:0x%x,idx_%d,%d\n", common_cmd->snifferHandle, common_cmd->snifferIndex, common_cmd->status);
                }
            }
        }
    }
    #if (APP_TRANSPORT_CANFD_ENABLE)
    if (1 == receive_bus_rssi_flag) {
        static u32 report_rssi_tick = 0;
        //if(clock_time_exceed(report_rssi_tick, (can_fd_cfg.reportInterval-5)*1000))
        if (clock_time_exceed(report_rssi_tick, (100 - 5) * 1000)) //TODO nodeSetting.reportIntvl
        {
            report_rssi_tick = clock_time();
            if (blc_ll_getCurrentMasterRoleNumber()) {
                //peer-peripheral RSSI
                canfd_send_data_handle(SLAVE_TO_ECU_SID, (u8 *)rssi_buffer, (sizeof(rssi_buffer) / 2));
            }
            if (blc_ll_getCurrentSlaveRoleNumber()) {
                //peer-central RSSI
                canfd_send_data_handle(SLAVE_TO_ECU_SID, (u8 *)&(rssi_buffer[REMOTE_DEVICE_MAX_NUM / 2]), (sizeof(rssi_buffer) / 2));
            }
            receive_bus_rssi_flag = 0;
        }
    }
    #endif
}

void snif_main_node_control_process(void)
{
    #if (APP_TRANSPORT_CANFD_ENABLE)
    if ((gpio_read(TCAN4550_GPIO_WKREQ_N) == 0) && ((acl_conn_central_num + acl_conn_periphr_num) == 0)) {
        if (can_sleep_pending_tick == 0) {
            can_sleep_pending_tick = clock_time() | 1;
        } else {
            if (clock_time_exceed(can_sleep_pending_tick, 10 * 1000 * 1000)) {
                u8 res = tcan4550_enter_sleep();
                //tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] TCAN4550 enter sleep mode, %d\r\n", res);
            }
        }
    }

    if (gpio_read(TCAN4550_GPIO_INT_N) == 0) {
        tcan4550_isr();
    }

    snif_main_node_rx_data_process();

    if (tcan_reset_flag) {
        tcan_reset_flag = 0;
        tcan4550_reset_hw();
        Init_CAN();
    }
    #endif

    foreach (i, REMOTE_DEVICE_MAX_NUM) {
        if (connection_status_update_flag[i]) {
            u8 update_flag = 0;

            if (connection_status_update_instantly[i]) {
                update_flag = 1;
            }

            if (clock_time_exceed(connection_status_update_tick[i], 200 * 1000)) {
                update_flag = 1;
            }

            if (update_flag) {
                spp_main_node_cmd_sync_req_tx_t *spp_common_cmd = (spp_main_node_cmd_sync_req_tx_t *)rssi_crtl;

                u8 role = dev_char_get_conn_role_by_connhandle(connection_status_update_flag[i]);
                u8 rtn  = 0xFF;

                if (role == ACL_ROLE_PERIPHERAL) {
                    rtn = blc_ll_getAclSlaveConnectionTimingParameter(connection_status_update_flag[i], spp_common_cmd->param);
                } else if (role == ACL_ROLE_CENTRAL) {
                    rtn = blc_ll_getAclMasterConnectionTimingParameter(connection_status_update_flag[i], spp_common_cmd->param);
                }

                if (BLE_SUCCESS == rtn) {
                    spp_common_cmd->cmdId         = SNIFFER_CMD_SYNC_REQ;
                    spp_common_cmd->dataLen       = SNIFFER_CMD_SYNC_REQ_DATA_LEN;
                    spp_common_cmd->dmaLen        = spp_common_cmd->dataLen + 4;
                    spp_common_cmd->transmit_time = clock_time();

                    u8  checkSum = 0;
                    u8  checklen = sizeof(spp_main_node_cmd_sync_req_tx_t) - 5; // exclude dmaLen and checksum
                    u8 *ptx      = (u8 *)(spp_common_cmd);
                    foreach (j, checklen) {
                        checkSum += *(ptx + j + 4);                             // skip dmaLen
                    }
                    spp_common_cmd->checksum = checkSum;
    #if (APP_TRANSPORT_CANFD_ENABLE)
                    u32 nowTick = clock_time();
                    u8  canSend = 1;
                    while (canfd_send_data_handle(SLAVE_TO_SNIFFER_SYNC_SID, (u8 *)&spp_common_cmd->cmdId, spp_common_cmd->dataLen + 4) != 0) {
                        spp_common_cmd->transmit_time = clock_time();
                        checkSum                      = 0;
                        foreach (k, checklen) {
                            checkSum += *(ptx + k + 4); // skip dmaLen
                        }
                        spp_common_cmd->checksum = checkSum;
                        if (clock_time_exceed(nowTick, 100 * 1000)) {
                            canSend = 0;
                            break;
                        }
                    }
    #elif (APP_TRANSPORT_UART_ENABLE)
                    u32 uart_tx_start_tick     = clock_time();
                    u32 uart_transmit_max_time = UART_TX_WAIT_MAX_BYTE * 10 * 1000 * 1000 / UART_BAUD_RATE; // 100 bytes transmit time (us)
                    while (!clock_time_exceed(uart_tx_start_tick, uart_transmit_max_time)) {
                        if (uart_dma_send_done_flag) {
                            spp_common_cmd->transmit_time = clock_time();
                            checkSum                      = 0;
                            foreach (k, checklen) {
                                checkSum += *(ptx + k + 4); // skip dmaLen
                            }
                            spp_common_cmd->checksum = checkSum;
                            if (uart_send_dma(UART_MODULE_SEL, (u8 *)&spp_common_cmd->cmdId, spp_common_cmd->dataLen + 4)) {
                                uart_dma_send_done_flag = 0;
                            }
                            break;
                        }
                    }
                    if (clock_time_exceed(uart_tx_start_tick, uart_transmit_max_time)) {
                        //reach send timeout
                        uart_dma_send_done_flag = 1;
                    }
    #endif

    #if (APP_TRANSPORT_CANFD_ENABLE)
                    if (canSend == 1) {
                        if (connection_status_update_instantly[i]) {
                            connection_status_update_instantly[i] = 0;
                        }
                        connection_status_update_tick[i] = clock_time();
                    }
    #elif (APP_TRANSPORT_UART_ENABLE)
                    if (connection_status_update_instantly[i]) {
                        connection_status_update_instantly[i] = 0;
                    }
                    connection_status_update_tick[i] = clock_time();
    #endif
                    if (log_sniffer_enable) {
                        //tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] SYNC_REQ:0x%x\n",connection_status_update_flag[i]);
                    }
                }
            }
        }
    }

    #if (UI_LED_ENABLE)
    static _attribute_ble_data_retention_ u32 tick_str;
    if (clock_time_exceed(tick_str, 200 * 1000)) {
        tick_str = clock_time();
        gpio_toggle(GPIO_LED_WHITE);
    }
    #endif
}


    #if (APP_TRANSPORT_UART_ENABLE)
void user_uart_init(void)
{
    unsigned short div;
    unsigned char  bwpc;

    uart_reset(UART_MODULE_SEL);
    uart_set_pin(UART_MODULE_SEL, UART_MODULE_TX_PIN, UART_MODULE_RX_PIN);

    uart_cal_div_and_bwpc(UART_BAUD_RATE, sys_clk.pclk * 1000 * 1000, &div, &bwpc);
    uart_init(UART_MODULE_SEL, div, bwpc, UART_PARITY_NONE, UART_STOP_BIT_ONE);

    uart_set_tx_dma_config(UART_MODULE_SEL, UART_DMA_CHANNEL_TX);
    uart_set_rx_dma_config(UART_MODULE_SEL, UART_DMA_CHANNEL_RX);

    uart_clr_irq_mask(UART_MODULE_SEL, UART_RX_IRQ_MASK | UART_TX_IRQ_MASK | UART_TXDONE_MASK | UART_RXDONE_MASK);
    uart_clr_irq_status(UART_MODULE_SEL, UART_TXDONE_IRQ_STATUS);
    uart_set_rx_timeout(UART_MODULE_SEL, bwpc, 12, UART_BW_MUL3);
    uart_set_irq_mask(UART_MODULE_SEL, UART_RXDONE_MASK | UART_TXDONE_MASK);

    plic_interrupt_enable(UART_MODULE_SEL == UART0_MODULE ? IRQ_UART0 : IRQ_UART1);
    plic_set_priority(UART_MODULE_SEL == UART0_MODULE ? IRQ_UART0 : IRQ_UART1, 1);


    u8 *uart_rx_addr = (spp_rx_fifo_b + (spp_rx_fifo.wptr & (spp_rx_fifo.num - 1)) * spp_rx_fifo.size);
    //uart_recbuff_init(uart_rx_addr, spp_rx_fifo.size);
    uart_receive_dma(UART_MODULE_SEL, uart_rx_addr, spp_rx_fifo.size);
    uart_clr_irq_status(UART_MODULE_SEL, UART_TXDONE_IRQ_STATUS);
    //tlkapi_send_string_data(APP_LOG_EN, "[APP][INI] uart init.", 0, 0);
}

/**
 * @brief       uart receive data irq handler.
 * @param[in]   none.
 * @return      none.
 */
_attribute_ram_code_sec_ void uart0_irq_handler(void)
{
    if (uart_get_irq_status(UART_MODULE_SEL, UART_TXDONE_IRQ_STATUS)) {
        uart_dma_send_done_flag = 1;

        /* after txdone 5us, the interrupt occurred*/
        //gpio_toggle(GPIO_LED_GREEN);
        uart_clr_irq_status(UART_MODULE_SEL, UART_TXDONE_IRQ_STATUS);
    }

    if (uart_get_irq_status(UART_MODULE_SEL, UART_RXDONE_IRQ_STATUS)) {
            /* after rxdone 42us, the interrupt occurred*/
        #if 0 // for test
        /* Get the length of Rx data */
        u32 rxLen= uart_get_dma_rev_data_len(UART_MODULE_SEL, UART_DMA_CHANNEL_RX);
        u8* pData = spp_rx_fifo.p + spp_rx_fifo.wptr * spp_rx_fifo.size;
        if(uart_send_dma(UART_MODULE_SEL, pData, rxLen)){
            uart_dma_send_done_flag = 0;
        }
        #endif

        my_spp_rx_fifo_tick_record[spp_rx_fifo.wptr] = clock_time();
        spp_rx_fifo.wptr == (spp_rx_fifo.num - 1) ? spp_rx_fifo.wptr = 0 : spp_rx_fifo.wptr++;
        u8 *p = spp_rx_fifo.p + spp_rx_fifo.wptr * spp_rx_fifo.size;
        /* Clear RxDone state */
        uart_clr_irq_status(UART_MODULE_SEL, UART_RXDONE_IRQ_STATUS);
        uart_receive_dma(UART_MODULE_SEL, p, spp_rx_fifo.size); //[!!important - must]

        if ((uart_get_irq_status(UART_MODULE_SEL, UART_RX_ERR))) {
            uart_clr_irq_status(UART_MODULE_SEL, UART_RXDONE_IRQ_STATUS);
        }
    }
}
PLIC_ISR_REGISTER(uart0_irq_handler, IRQ_UART0)

int rx_from_uart_cb(void)
{
    if (spp_rx_fifo.wptr == spp_rx_fifo.rptr) {
        return -1;
    } else {
        snif_main_node_rx_data_process();
    }
    return 0;
}

int tx_to_uart_cb(void)
{
    return 0;
}
    #endif


/**
 * @brief      sniffer main node initialization
 * @param[in]  none
 * @return     none.
 */
void snif_main_node_init(void)
{
    //////////// UART/CAN Initialization  Begin /////////////////////////
    #if (APP_TRANSPORT_UART_ENABLE)
    user_uart_init();
    blc_register_hci_handler(rx_from_uart_cb, tx_to_uart_cb); //customized uart handler
    #elif (APP_TRANSPORT_CANFD_ENABLE)
    tcan4550_init();
    memset((u8 *)rssi_buffer, 0xFF, sizeof(rssi_buffer));
    #endif
    //////////// UART/CAN Initialization  End /////////////////////////

    blc_ll_setAclMasterConnParamUpdateRspLatency(0);

    blc_ll_registerTelinkControllerEventCallback(BLT_EV_FLAG_CHANNEL_MAP_UPDATE, &user_channel_map_update);

    tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] snif_main_node_init:M%dS%d,BandRate_%d\n", ACL_CENTRAL_MAX_NUM, ACL_PERIPHR_MAX_NUM, UART_BAUD_RATE);
}

#endif /* MAIN_NODE_ROLE_SELECT == MAIN_NODE_PERIPHERAL_CENTRAL */
