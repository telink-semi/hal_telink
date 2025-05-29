/********************************************************************************************************
 * @file    app_sub_node.c
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

#include "app.h"
#include "app_buffer.h"
#include "app_ui.h"
#include "app_sub_node.h"
#if (APP_TRANSPORT_CANFD_ENABLE)
    #include "../tcan4x5x/TCAN4550.h"
#endif

#if (MONITOR_ROLE_SELECT == MONITOR_CENTRAL_PERIPHERAL)

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

_attribute_ble_data_retention_ u8  rssi_report[REMOTE_DEVICE_MAX_NUM][SPP_TXFIFO_SIZE];
_attribute_ble_data_retention_ u8  sniffer_rssi_report_flag[REMOTE_DEVICE_MAX_NUM];
_attribute_ble_data_retention_ u32 sniffer_rssi_report_tick[REMOTE_DEVICE_MAX_NUM];

_attribute_ble_data_retention_ u8  date_sync_rsp[REMOTE_DEVICE_MAX_NUM][SPP_TXFIFO_SIZE];
_attribute_ble_data_retention_ u8  sniffer_sync_rsp_flag[REMOTE_DEVICE_MAX_NUM];
_attribute_ble_data_retention_ u32 sniffer_sync_rsp_tick[REMOTE_DEVICE_MAX_NUM];

    #if (APP_TRANSPORT_UART_ENABLE)
_attribute_ble_data_retention_ u8 uart_dma_send_done_flag = 1;
    #endif

    #if (APP_TRANSPORT_CANFD_ENABLE)
        #if (APP_CAN_PM_ENABLE)
_attribute_ble_data_retention_ u8  can_wake_up_flag       = 0;
_attribute_ble_data_retention_ u32 can_sleep_pending_tick = 1;
        #endif
    #endif

    #if (APP_TRANSPORT_CANFD_ENABLE)
void canfd_rxdata_handle(u8 *data, u8 len)
{
    u8 *p                                        = spp_rx_fifo.p + spp_rx_fifo.wptr * spp_rx_fifo.size;
    my_spp_rx_fifo_tick_record[spp_rx_fifo.wptr] = clock_time();
    blc_app_memory_copy(p, data, len, SPP_RXFIFO_SIZE, 0x11120000 | __LINE__);
    (spp_rx_fifo.wptr == (spp_rx_fifo.num - 1)) ? spp_rx_fifo.wptr = 0 : spp_rx_fifo.wptr++;
    can_sleep_pending_tick = clock_time() | 1;
}

int canfd_send_data_handle(u16 sid, u8 *pData, u32 len)
{
    int state;

    state = can_fd_data_send(sid, pData, len);
    if (state != 0) {
        //Fail
        tlkapi_printf(APP_CAN_LOG_EN, "[APP][CAN] can_fd_data_send error:%d, ID:0x%x\n", state, sid);
    }
    can_sleep_pending_tick = clock_time() | 1;
    return state;
}
    #endif


void snif_sub_node_rx_data_process(void)
{
    if (spp_rx_fifo.wptr == spp_rx_fifo.rptr) {
        return;
    } else {
        spp_sub_node_cmd_sync_req_rx_t *common_cmd  = (spp_sub_node_cmd_sync_req_rx_t *)(spp_rx_fifo.p + spp_rx_fifo.rptr * spp_rx_fifo.size);
        u32                             spp_rx_tick = my_spp_rx_fifo_tick_record[spp_rx_fifo.rptr];
        spp_rx_fifo.rptr == (spp_rx_fifo.num - 1) ? spp_rx_fifo.rptr = 0 : spp_rx_fifo.rptr++;

    #if (UI_LED_ENABLE)
        //rx data from bus
        gpio_toggle(GPIO_LED_BLUE);
    #endif

        if (common_cmd->cmdId == SNIFFER_CMD_SYNC_REQ) {
            //tlkapi_send_string_data(APP_SNIF_LOG_EN, "[APP][SNIF] Rx", (u8*)&common_cmd->cmdId, common_cmd->dataLen + 4);

            u8  checklen = sizeof(spp_sub_node_cmd_sync_req_rx_t) - 1; // exclude checksum
            u8  checkSum = 0;
            u8 *prx      = (u8 *)common_cmd;
            foreach (i, checklen) {
                checkSum += *(prx + i);
            }

            u32 err = 0;
            if (checkSum != common_cmd->checksum) {
                err = SNIFFER_PARAMETER_CHECKSUM_ERR;
            } else {
    #if (APP_TRANSPORT_CANFD_ENABLE)
                u32 transmit_cost = 1313 * SYSTEM_TIMER_TICK_1US; //1313.88us
    #elif (APP_TRANSPORT_UART_ENABLE)
                u32 transmit_cost = (1000000 * 10) / UART_BAUD_RATE;                // one Byte cost, unit us
                transmit_cost *= (common_cmd->dataLen + 4) * SYSTEM_TIMER_TICK_1US; //Note: software processing latency needs to be considered
    #endif
                u32 peer_diff_expectTime_time = (u32)(common_cmd->expectTime_time - common_cmd->transmit_time);

                //Convert sniffer's listening anchor point
                common_cmd->expectTime_time = spp_rx_tick - transmit_cost + peer_diff_expectTime_time;

                err = blc_ll_updateAclSnifferSync((u8 *)&common_cmd->syncHandle);
            }

            if (err) {
                u8 idx = common_cmd->syncHandle & REMOTE_DEVICE_MAX_MASK;
                if (idx >= REMOTE_DEVICE_MAX_NUM) {
                    return;
                }

                sniffer_sync_rsp_tick[idx] = clock_time();
                sniffer_sync_rsp_flag[idx] = common_cmd->syncHandle;

                spp_sub_node_cmd_sync_rsp_tx_t *spp_common_cmd = (spp_sub_node_cmd_sync_rsp_tx_t *)date_sync_rsp[idx];
                spp_common_cmd->cmdId                          = SNIFFER_CMD_SYNC_RSP;
                spp_common_cmd->dataLen                        = SNIFFER_CMD_SYNC_RSP_DATA_LEN;
    #if (APP_TRANSPORT_CANFD_ENABLE)
                spp_common_cmd->snifferIndex = can_fd_cfg.deviceId;
    #elif (APP_TRANSPORT_UART_ENABLE)
                spp_common_cmd->snifferIndex = APP_SNIFFER_INDEX;
    #endif
                spp_common_cmd->snifferHandle = common_cmd->syncHandle;
                spp_common_cmd->status        = err;
                spp_common_cmd->dmaLen        = spp_common_cmd->dataLen + 4;

                checkSum = 0;
                checkSum += spp_common_cmd->cmdId;
                checkSum += spp_common_cmd->dataLen;
                checkSum += spp_common_cmd->snifferIndex;
                checkSum += spp_common_cmd->snifferHandle;
                checkSum += spp_common_cmd->status;
                spp_common_cmd->checksum = checkSum;

                tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] SYNC_RSP:0x%x,err:0x%x\n", common_cmd->syncHandle, spp_common_cmd->status);
            } else {
                tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] SYNC_REQ:0x%x\n", common_cmd->syncHandle);
            }
        }
    }
}

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

/**
 * @brief      callBack function of LinkLayer Event "BLT_EV_FLAG_SNIFFER_RSSI_REPORT"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
_attribute_ram_code_ void user_sniffer_rssi_report(u8 e, u8 *p, int n)
{
    (void)n;
    (void)e;
    acl_sniffer_rssi_reportEvt_t *pa = (acl_sniffer_rssi_reportEvt_t *)p;

    u8 snifferType    = pa->type;
    u8 snifferChannel = pa->snifChannel;
    u8 snifferHandle  = pa->snifHandle;
    u8 rssi           = pa->rssi;
    u8 rssi_smooth;

    u8 idx = snifferHandle & REMOTE_DEVICE_MAX_MASK;
    if (idx >= REMOTE_DEVICE_MAX_NUM) {
        return;
    }

    #if (APP_TRANSPORT_UART_ENABLE)
    sniffer_rssi_report_tick[idx] = clock_time();
    #endif

    sniffer_rssi_report_flag[idx] = snifferHandle;

    if (snifferType == 2 || snifferType == 1) { //2:peer-master RSSI, 1:peer-slave RSSI
        rssi_smooth = rssi_filter(idx, rssi);
    } else {
        rssi_smooth = rssi;
    }

    spp_sub_node_cmd_rssi_tx_t *spp_common_cmd = (spp_sub_node_cmd_rssi_tx_t *)rssi_report[idx];
    spp_common_cmd->cmdId                      = SNIFFER_CMD_RSSI;
    spp_common_cmd->dataLen                    = SNIFFER_CMD_RSSI_DATA_LEN;
    #if (APP_TRANSPORT_CANFD_ENABLE)
    spp_common_cmd->snifferIndex = can_fd_cfg.deviceId;
    #elif (APP_TRANSPORT_UART_ENABLE)
    spp_common_cmd->snifferIndex = APP_SNIFFER_INDEX;
    #endif
    spp_common_cmd->snifferHandle  = snifferHandle;
    spp_common_cmd->rssi           = rssi_smooth;
    spp_common_cmd->snifferChannel = snifferChannel;
    spp_common_cmd->deviceType     = snifferType;
    spp_common_cmd->dmaLen         = spp_common_cmd->dataLen + 4;

    u8 checkSum = 0;
    checkSum += spp_common_cmd->cmdId;
    checkSum += spp_common_cmd->dataLen;
    checkSum += spp_common_cmd->snifferIndex;
    checkSum += spp_common_cmd->snifferHandle;
    checkSum += spp_common_cmd->rssi;
    checkSum += (spp_common_cmd->deviceType << 6) | spp_common_cmd->snifferChannel;
    spp_common_cmd->checksum = checkSum;

    #if (DEBUG_SNIFFER_REPORT_INSTANT_EN)
    spp_common_cmd->dmaLen += 1;
    rssi_report[idx][spp_common_cmd->dmaLen + 4 - 1] = p[3];
    #endif
}

/**
 * @brief      callBack function of LinkLayer Event "BLT_EV_FLAG_SNIFFER_SYNC_STATUS"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
_attribute_ram_code_ void user_sniffer_sync_status(u8 e, u8 *p, int n)
{
    (void)n;
    (void)e;
    acl_sniffer_sync_statusEvt_t *pa = (acl_sniffer_sync_statusEvt_t *)p;

    u8 idx = pa->snifHandle & REMOTE_DEVICE_MAX_MASK;
    if (idx >= REMOTE_DEVICE_MAX_NUM) {
        return;
    }

    sniffer_sync_rsp_tick[idx] = clock_time();
    sniffer_sync_rsp_flag[idx] = pa->snifHandle;

    spp_sub_node_cmd_sync_rsp_tx_t *spp_common_cmd = (spp_sub_node_cmd_sync_rsp_tx_t *)date_sync_rsp[idx];
    spp_common_cmd->cmdId                          = SNIFFER_CMD_SYNC_RSP;
    spp_common_cmd->dataLen                        = SNIFFER_CMD_SYNC_RSP_DATA_LEN;
    #if (APP_TRANSPORT_CANFD_ENABLE)
    spp_common_cmd->snifferIndex = can_fd_cfg.deviceId;
    #elif (APP_TRANSPORT_UART_ENABLE)
    spp_common_cmd->snifferIndex = APP_SNIFFER_INDEX;
    #endif
    spp_common_cmd->snifferHandle = pa->snifHandle;
    spp_common_cmd->status        = pa->status;
    spp_common_cmd->dmaLen        = spp_common_cmd->dataLen + 4;

    u8 checkSum = 0;
    checkSum += spp_common_cmd->cmdId;
    checkSum += spp_common_cmd->dataLen;
    checkSum += spp_common_cmd->snifferIndex;
    checkSum += spp_common_cmd->snifferHandle;
    checkSum += spp_common_cmd->status;
    spp_common_cmd->checksum = checkSum;
}

void snif_sub_node_control_process(void)
{
    #if (APP_TRANSPORT_CANFD_ENABLE)
    // sniffer_data_send_delay
    static _attribute_ble_data_retention_ u32 sniffer_data_send_delay = 500 + 285; // unit us

    /* RISING_EDGE      low-high -> sleep
     * FALLING_EDGE     high-low -> wake-up
     * */
    if (can_wake_up_flag) {
        can_wake_up_flag = 0;

        tcan4550_init();

        can_sleep_pending_tick = clock_time() | 1;

        //tlkapi_printf(APP_SNIF_LOG_EN,"remote wake-up\n");
    }

    if (can_sleep_pending_tick && clock_time_exceed(can_sleep_pending_tick, 10 * 1000 * 1000)) {
        can_sleep_pending_tick = 0;
        u8 res                 = tcan4550_enter_sleep();
        //tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] tcan4550 enter sleep, %d\r\n", res);
    }

    if (gpio_read(TCAN4550_GPIO_INT_N) == 0) {
        tcan4550_isr();
    }

    snif_sub_node_rx_data_process();

    if (tcan_reset_flag) {
        tcan_reset_flag = 0;
        tcan4550_reset_hw();
        Init_CAN();
    }

    foreach (i, REMOTE_DEVICE_MAX_NUM) {
        if (sniffer_sync_rsp_flag[i] && (clock_time_exceed(sniffer_sync_rsp_tick[i], can_fd_cfg.deviceId * sniffer_data_send_delay))) {
            spp_sub_node_cmd_sync_rsp_tx_t *spp_common_cmd = (spp_sub_node_cmd_sync_rsp_tx_t *)date_sync_rsp[i];

            canfd_send_data_handle(SNIFFER_TO_SLAVE_SYNC_SID, (u8 *)&spp_common_cmd->cmdId, spp_common_cmd->dataLen + 4);
            sniffer_sync_rsp_flag[i] = 0;

            tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] CMD_SYNC_RSP:0x%x,idx_%d,%d\n", spp_common_cmd->snifferHandle, spp_common_cmd->snifferIndex, spp_common_cmd->status);
        }

        if (sniffer_rssi_report_flag[i] && (clock_time_exceed(sniffer_rssi_report_tick[i], can_fd_cfg.reportInterval * 1000 + can_fd_cfg.deviceId * sniffer_data_send_delay))) {
            spp_sub_node_cmd_rssi_tx_t *spp_common_cmd = (spp_sub_node_cmd_rssi_tx_t *)rssi_report[i];

            sniffer_rssi_report_tick[i] = clock_time();
            canfd_send_data_handle(can_fd_cfg.canId, (u8 *)&spp_common_cmd->cmdId, spp_common_cmd->dataLen + 4);
            sniffer_rssi_report_flag[i] = 0;

            tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] RSSI:0x%x,idx_%d,%d,chl:%d,rssi:%d\n", spp_common_cmd->snifferHandle, spp_common_cmd->snifferIndex, spp_common_cmd->deviceType, spp_common_cmd->snifferChannel, spp_common_cmd->rssi - 110);
        }
    }
    #elif (APP_TRANSPORT_UART_ENABLE)
    // data_send delay APP_SNIFFER_INDEX * sniffer_data_send_delay
    static _attribute_ble_data_retention_ u32 sniffer_data_send_delay = 500 + (1000000 * 10 * sizeof(spp_sub_node_cmd_rssi_tx_t)) / UART_BAUD_RATE; // unit us

    foreach (i, REMOTE_DEVICE_MAX_NUM) {
        if (sniffer_sync_rsp_flag[i] && (clock_time_exceed(sniffer_sync_rsp_tick[i], APP_SNIFFER_INDEX * sniffer_data_send_delay))) {
            spp_sub_node_cmd_sync_rsp_tx_t *spp_common_cmd = (spp_sub_node_cmd_sync_rsp_tx_t *)date_sync_rsp[i];

            u32 uart_tx_start_tick     = clock_time();
            u32 uart_transmit_max_time = UART_TX_WAIT_MAX_BYTE * 10 * 1000 * 1000 / UART_BAUD_RATE; // 100 bytes transmit time (us)s
            while (!clock_time_exceed(uart_tx_start_tick, uart_transmit_max_time)) {
                if (uart_dma_send_done_flag) {
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
            sniffer_sync_rsp_flag[i] = 0;

            tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] CMD_SYNC_RSP:0x%x,idx_%d,%d\n", spp_common_cmd->snifferHandle, spp_common_cmd->snifferIndex, spp_common_cmd->status);
        }

        if (sniffer_rssi_report_flag[i] && (clock_time_exceed(sniffer_rssi_report_tick[i], APP_SNIFFER_INDEX * sniffer_data_send_delay))) {
            spp_sub_node_cmd_rssi_tx_t *spp_common_cmd = (spp_sub_node_cmd_rssi_tx_t *)rssi_report[i];

            u32 uart_tx_start_tick     = clock_time();
            u32 uart_transmit_max_time = UART_TX_WAIT_MAX_BYTE * 10 * 1000 * 1000 / UART_BAUD_RATE; // 100 bytes transmit time (us)
            while (!clock_time_exceed(uart_tx_start_tick, uart_transmit_max_time)) {
                if (uart_dma_send_done_flag) {
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
            sniffer_rssi_report_flag[i] = 0;

            tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] RSSI:0x%x,idx_%d,%d,chl:%d,rssi:%d\n", spp_common_cmd->snifferHandle, spp_common_cmd->snifferIndex, spp_common_cmd->deviceType, spp_common_cmd->snifferChannel, spp_common_cmd->rssi - 110);
        }
    }
    #endif

    #if (UI_LED_ENABLE)
    static _attribute_ble_data_retention_ u32 tick_str;
    if (clock_time_exceed(tick_str, 500 * 1000 * (APP_SNIFFER_INDEX + 1))) {
        tick_str = clock_time();
        gpio_toggle(GPIO_LED_WHITE);

        //led show monitor state
        if (blc_ll_getAclSnifferSlvSyncNumber() || blc_ll_getAclSnifferMstSyncNumber()) {
            gpio_write(GPIO_LED_RED, 1);
        } else {
            gpio_write(GPIO_LED_RED, 0);
        }
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

    u8 *uart_rx_addr = (spp_rx_fifo.p + (spp_rx_fifo.wptr & (spp_rx_fifo.num - 1)) * spp_rx_fifo.size);
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
        snif_sub_node_rx_data_process();
    }
    return 0;
}

int tx_to_uart_cb(void)
{
    return 0;
}
    #endif


/**
 * @brief      sniffer sub node initialization
 * @param[in]  none
 * @return     none.
 */
void snif_sub_node_init(void)
{
    //////////// Monitor Initialization  Begin /////////////////////////
    blc_ll_initAclSnifferSlv_module();
    blc_ll_addAclSnifferSlvSyncEarlyTime(200);
    //blc_ll_setAclSnifferSlvReportRssiType(RSSI_TYPE_ALL);//for testing
    blc_ll_setAclSnifferSlv1stSyncWinMaxEnable(1);

    blc_ll_initAclSnifferMst_module();
    blc_ll_addAclSnifferMstSyncEarlyTime(200);
    //blc_ll_setAclSnifferMstReportRssiType(RSSI_TYPE_ALL);//for testing
    blc_ll_setAclSnifferMst1stSyncWinMaxEnable(1);

    blc_ll_registerTelinkControllerEventCallback(BLT_EV_FLAG_SNIFFER_RSSI_REPORT, &user_sniffer_rssi_report);
    blc_ll_registerTelinkControllerEventCallback(BLT_EV_FLAG_SNIFFER_SYNC_STATUS, &user_sniffer_sync_status);
    //////////// Monitor Initialization  End /////////////////////////

    //////////// Feature Initialization Begin /////////////////////////
    blc_ll_initChannelSelectionAlgorithm_2_feature(); //main_node support CSA#2
    /* Attention: sniffer support PHY switch (1M, 2M, Coded) */
    #if (APP_PHY_SWITCHING_ENABLE)
    blc_ll_init2MPhyCodedPhy_feature();
    #endif
    blc_ll_setAclSnifferMaxRxBufferLen(ACL_CONN_MAX_RX_OCTETS);

    tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] Feature:CSA #2,PHY Switching,Rx DLE:%d\n", ACL_CONN_MAX_RX_OCTETS);
    //////////// Feature Initialization End /////////////////////////

    //////////// UART/CAN Initialization  Begin /////////////////////////
    #if (APP_TRANSPORT_UART_ENABLE)
    user_uart_init();
    blc_register_hci_handler(rx_from_uart_cb, tx_to_uart_cb); //customized uart handler
    #elif (APP_TRANSPORT_CANFD_ENABLE)
    tcan4550_init();
    #endif
    //////////// UART/CAN Initialization  End /////////////////////////

    tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] snif_sub_node_init:Index_%d,M%dS%d,BandRate_%d\n", APP_SNIFFER_INDEX, ACL_CENTRAL_MAX_NUM, ACL_PERIPHR_MAX_NUM, UART_BAUD_RATE);
}

#endif /* MONITOR_ROLE_SELECT == MONITOR_CENTRAL_PERIPHERAL */
