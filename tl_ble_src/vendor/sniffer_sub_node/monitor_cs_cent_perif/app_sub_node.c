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
#include "app_cs.h"
#include "app_sub_node.h"
#if (APP_TRANSPORT_CANFD_ENABLE)
    #include "../tcan4x5x/TCAN4550.h"
#endif
#include "math.h"

#include "algorithm/hadm/gcc10/cs_cal.h"


#if (MONITOR_ROLE_SELECT == MONITOR_CS_CENTRAL_PERIPHERAL)

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

_attribute_ble_data_retention_ volatile u8 log_sniffer_enable = 0;

_attribute_ble_data_retention_ node_setting_t nodeSetting;

#define FILTER_DISTANCE_NUM     3
_attribute_ble_data_retention_ kalmanFilter_t snifKalman[ACL_CENTRAL_MAX_NUM][FILTER_DISTANCE_NUM];
_attribute_ble_data_retention_ u32 bus_error_tick = 0;
_attribute_ble_data_retention_ u32 app_bus_tx_tick = 0;

    #if (APP_TRANSPORT_UART_ENABLE)
_attribute_ble_data_retention_ u8 uart_dma_send_done_flag = 1;
    #endif

    #define CS_EVT_DATA_BUS_SIZE (1856)
_attribute_ble_data_retention_ u8 csEvtDataBusTx[CS_EVT_DATA_BUS_SIZE];     // for bus transmit
_attribute_ble_data_retention_ u8 csEvtDataBusRx[CS_EVT_DATA_BUS_SIZE];     // for bus receive
_attribute_ble_data_retention_ u8 csEvtDataBusRx_2[CS_EVT_DATA_BUS_SIZE];   // for bus receive

    #if (APP_TRANSPORT_CANFD_ENABLE)
        #if (APP_CAN_PM_ENABLE)
_attribute_ble_data_retention_ u8  can_wake_up_flag       = 0;
_attribute_ble_data_retention_ u32 can_sleep_pending_tick = 1;
        #endif
    #endif

_attribute_ble_data_retention_ volatile u8  csEvtSplitNum = 0;
_attribute_ble_data_retention_ volatile u32 curRasCounter = 0x00000000;    //TODO, Need to define to multiple groups, consider multiple connections and config_id scenarios

_attribute_ble_data_retention_ app_cs_param_setting_t appCsParamSetting;

_attribute_ram_code_ u8 check_sum(u8 *pdata, u32 len)
{
    u8 sum = 0;
    for (u32 i = 0; i < len; i++) {
        sum += pdata[i];
    }
    return sum;
}

void app_parse_printf(const char *format, ...)
{
}

#if (KALMAN_FILTER_ENABLE)
void snif_kalman_Filter_init(void)
{
    for (int i = 0; i < ACL_CENTRAL_MAX_NUM; i++) {
        for (int j = 0; j < FILTER_DISTANCE_NUM; j++) {
            snifKalman[i][j].state          = 0.0;
            snifKalman[i][j].err_cov        = 1.0;
            snifKalman[i][j].proc_noise_cov = appCsParamSetting.kalmanNoiseCov * 0.0001; //default value: 0.003;
            snifKalman[i][j].msr_noise_cov  = 0.01;
            snifKalman[i][j].kal_gain       = 0.0;
            snifKalman[i][j].update_tick    = clock_time();
        }
    }
}

float snif_kalman_filter_update(kalmanFilter_t *pkf, float measurement)
{
    if (isnan(measurement)) {
        measurement = pkf->state;
    }

    #if (!APP_TRANSPORT_CANFD_ENABLE)
    /* judge if update has timeout */
    if (clock_time_exceed(pkf->update_tick, 5 * 1000 * 1000)) //5s
    {
        pkf->state    = 0.0;
        pkf->err_cov  = 1.0;
        pkf->kal_gain = 0.0;
    }
    pkf->update_tick = clock_time();
    #endif

    pkf->err_cov += pkf->proc_noise_cov;
    pkf->kal_gain = pkf->err_cov / (pkf->err_cov + pkf->msr_noise_cov);
    pkf->state += pkf->kal_gain * (measurement - pkf->state);
    pkf->err_cov = (1 - pkf->kal_gain) * pkf->err_cov;

    return pkf->state;
}
#endif

void snif_sub_node_cs_distacne_process(u16 connHandle, u16 rangingCounter, float distance1, float distance2)
{
    u8 connIdx = blc_sniffer_getAclConnectionIndex(connHandle);
    if (connIdx >= REMOTE_DEVICE_MAX_NUM) {
        tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] ConnHandle index invalid! %s\n", __FUNCTION__);
        return;
    }

    float distance;
    if (appCsParamSetting.rangingAlgMode == (BLC_RANGING_ALGORITHM_1 & BLC_RANGING_ALGORITHM_2)) {
        distance = distance2;
    }
    else if((appCsParamSetting.rangingAlgMode == BLC_RANGING_ALGORITHM_1) || (appCsParamSetting.rangingAlgMode == BLC_RANGING_ALGORITHM_2)) {
        distance = distance1;
    }
    else {
        distance = 0.0;
        tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] distance error for rangingAlgMode mask not set");
    }

    #if 0
        distance = cs_filter(connIdx, node_id, distance);
    #else
    //distance = snif_kalman_filter_update(&snifKalman[connIdx][(nodeId == 0xFF) ? (CHECK_SNIFFER_INDEX_MAX) : nodeId], distance);
    #endif

    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] dist: sub%d=%.2f\n", nodeSetting.deviceIdx, distance);
}

    #if (APP_TRANSPORT_CANFD_ENABLE)
void canfd_rxdata_handle(u8 *data, u8 len)
{
    u8 *p                                        = spp_rx_fifo.p + spp_rx_fifo.wptr * spp_rx_fifo.size;
    my_spp_rx_fifo_tick_record[spp_rx_fifo.wptr] = clock_time();
    blc_app_memory_copy(p, data, len, SPP_RXFIFO_SIZE, 0x11120000 | __LINE__);
    (spp_rx_fifo.wptr == (spp_rx_fifo.num - 1)) ? spp_rx_fifo.wptr = 0 : spp_rx_fifo.wptr++;
        #if (APP_CAN_PM_ENABLE)
    can_sleep_pending_tick = clock_time() | 1;
        #endif
}

int canfd_send_data_handle(u16 sid, u8 *pData, u32 len)
{
    int state;

    state = can_fd_data_send(sid, pData, len);
    if (state != 0) {
        if(bus_error_tick){
            if(clock_time_exceed(bus_error_tick, 2000*1000)){
                bus_error_tick = 0;
                tcan4550_init();
            }
        }
        else{
            bus_error_tick = clock_time()|1;
        }
        tlkapi_printf(APP_CAN_LOG_EN, "[APP][CAN] can_fd_data_send error:%d, ID:0x%x\n", state, sid);
    }
    else{
        bus_error_tick = 0;
        app_bus_tx_tick = clock_time();
    }

        #if (APP_CAN_PM_ENABLE)
    can_sleep_pending_tick = clock_time() | 1;
        #endif

    return state;
}
    #endif

u16 rasClientDataLen = 0;//for test
u16 rasClientRangingCounter = 0;//for test
void snif_sub_node_rx_data_process(void)
{
    if (spp_rx_fifo.wptr == spp_rx_fifo.rptr) {
        return;
    }

    spp_sub_node_cmd_rx_t *rx_cmd      = (spp_sub_node_cmd_rx_t *)(spp_rx_fifo.p + spp_rx_fifo.rptr * spp_rx_fifo.size);
    u32                    spp_rx_tick = my_spp_rx_fifo_tick_record[spp_rx_fifo.rptr];
    spp_rx_fifo.rptr == (spp_rx_fifo.num - 1) ? spp_rx_fifo.rptr = 0 : spp_rx_fifo.rptr++;

    #if (UI_LED_ENABLE)
        //rx data from bus
        gpio_toggle(GPIO_LED_BLUE);
    #endif

    if (rx_cmd->cmdId == SNIFFER_CMD_SYNC_REQ) {
        spp_sub_node_cmd_sync_req_rx_t *common_cmd = (spp_sub_node_cmd_sync_req_rx_t *)rx_cmd;
        //tlkapi_send_string_data(APP_SNIF_LOG_EN, "[APP][SNIF] Rx SYNC_REQ", (u8*)&common_cmd->cmdId, common_cmd->dataLen + 4);

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
            spp_common_cmd->snifferIndex                   = nodeSetting.deviceIdx;
            spp_common_cmd->snifferHandle                  = common_cmd->syncHandle;
            spp_common_cmd->status                         = err;
            spp_common_cmd->dmaLen                         = spp_common_cmd->dataLen + 4;

            checkSum = 0;
            checkSum += spp_common_cmd->cmdId;
            checkSum += spp_common_cmd->dataLen;
            checkSum += spp_common_cmd->snifferIndex;
            checkSum += spp_common_cmd->snifferHandle;
            checkSum += spp_common_cmd->status;
            spp_common_cmd->checksum = checkSum;

            tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] SYNC_RSP:0x%x,err:0x%x\n", common_cmd->syncHandle, spp_common_cmd->status);
        } else {
            if (log_sniffer_enable) {
                tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] SYNC_REQ:0x%x\n", common_cmd->syncHandle);
            }
        }
    }
    else if (rx_cmd->cmdId == SNIFFER_CMD_CS_EVENT) {
        spp_sub_node_cmd_cs_event_rx_t *common_cmd = (spp_sub_node_cmd_cs_event_rx_t *)rx_cmd;

        u8 idx = common_cmd->snifferHandle & REMOTE_DEVICE_MAX_MASK;
        if (idx >= REMOTE_DEVICE_MAX_NUM) {
            return;
        }

        tlkapi_send_string_data(APP_SNIF_LOG_EN, "[APP][SNIF] Rx CS_EVENT", (u8 *)&common_cmd->cmdId, common_cmd->dataLen + 4);

        u8  checklen = common_cmd->dataLen + 3; // exclude checksum, 3 = u16(cmdId) + u16(dataLen) - u8(checksum)
        u8  checkSum = 0;
        u8  k;
        u8 *prx = (u8 *)common_cmd;

        for (k = 0; k < checklen; k++) {
            checkSum += *(prx + k);
        }

        u32 err = 0;
        if (checkSum != *(prx + k)) {
            err = CS_SNIFFER_PARAMETER_CHECKSUM_ERR;
        } else if ((common_cmd->snifferIndex == nodeSetting.deviceIdx) || (common_cmd->snifferIndex == LOCAL_ALL_SNIFFER_INDEX)) {
            extern int blc_ll_updateCsSnifferParam(u8 * cmd);
            err = blc_ll_updateCsSnifferParam((u8 *)&common_cmd->cs_event_data);
        }

        if (err) {
            tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] CMD_CS_EVENT Index:%d, Handle:0x%x, Error:0x%x\n", common_cmd->snifferIndex, common_cmd->snifferHandle, err);
        }
    }
    #if (CS_DISTANCE_CALC_SUB_NODE_EN)
    else if (rx_cmd->cmdId == SNIFFER_CMD_CS_RAS_CLIENT_DATA_EVENT) {
        /* Due to the length of the cs ras client event data, it is necessary to split the data into smaller
         * packets on the transmitter end and reassemble them on the receiving end.
         +-------+---------+---------------+----------------+-----------+----------------------+-----------+
         | cmdId | dataLen | snifferIndex  |  snifferHandle | packetIdx |        data          | checkSum  |
         |-------------------------------------------------------------------------------------------------|
         | 2Byte |  2Byte  |    1Byte      |     2Byte      |   1Byte   | SPP_SLIPT_PACKET_LEN |   1Byte   |
         +-------+---------+---------------+----------------+-----------+----------------------+-----------+
         * index:  bit0~bit6  0~127 sequence number
         *         bit7       0     start/continuation fragment
         *                    1     complete
         */
        u8                   *p     = (u8 *)rx_cmd;
        cmd_cs_event_slipt_t *slipt = (cmd_cs_event_slipt_t *)rx_cmd;

        if (slipt->snifferIndex >= LOCAL_SNIFFER_NUM_MAX) {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] CS_RAS_CLIENT slipt data, snifferIndex invalid! subnodeId=%d, sliptIndex=%d\n", slipt->snifferIndex, slipt->packetIdx);
            return;
        }

        /* focus only on data that matches current node */
        if (slipt->snifferIndex != nodeSetting.deviceIdx) {
            return;
        }

        u8 idx = slipt->snifferHandle & REMOTE_DEVICE_MAX_MASK;
        if (idx >= REMOTE_DEVICE_MAX_NUM) {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] CS_RAS_CLIENT slipt data, snifferHandle invalid! subnodeId=%d, snifHandle=%d, sliptIndex=%d\n", slipt->snifferIndex, slipt->snifferHandle, slipt->packetIdx);
            csEvtSplitNum = 0;
            curRasCounter = 0;
            return;
        }

        u8 check = check_sum(p, slipt->dataLen + 3);
        if (check != p[slipt->dataLen + 3]) {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] CS_RAS_CLIENT slipt data, check sum error! subnodeId=%d, sliptIndex=%d, %02X, %02X\n", slipt->snifferIndex, slipt->packetIdx, check, p[slipt->dataLen + 3]);
            csEvtSplitNum = 0;
            curRasCounter = 0;
            return;
        }

        csEvtSplitNum++;
        /* first packet */
        if ((slipt->packetIdx & 0x7F) == 0) {
            memset(csEvtDataBusRx, 0, CS_EVT_DATA_BUS_SIZE);
            csEvtSplitNum = 0;
            curRasCounter = 0;
        }

        if (csEvtSplitNum != (slipt->packetIdx & 0x7F)) {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] CS_RAS_CLIENT slipt data, not the expected packetIdx! subnodeId=%d, csEvtSplitNum=%02X, slipt->packetIdx=%02X\n", slipt->snifferIndex, csEvtSplitNum, slipt->packetIdx);

            csEvtSplitNum = 0;
            curRasCounter = 0;
            return;
        }

        u16 offset = SPP_SLIPT_PACKET_LEN * (slipt->packetIdx & 0x7F);
        u8  len    = slipt->dataLen - 5; /* u8(snifferIndex), u16(snifferHandle), u8(packetIdx), u8(checkSum) */
        if (len <= SPP_SLIPT_PACKET_LEN) {
            memcpy(csEvtDataBusRx + offset, slipt->pData, slipt->dataLen - 5);
            tlkapi_printf(0, "[APP][BUS] CS_RAS_CLIENT slipt data, subnodeId=%d, packetIndex=%02X, datalen=%d\r\n", slipt->snifferIndex, slipt->packetIdx, slipt->dataLen - 5);
        } else {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] CS_RAS_CLIENT slipt data, length invalid! subnodeId=%d, packetIndex=%02X, datalen=%d\r\n", slipt->snifferIndex, slipt->packetIdx, slipt->dataLen - 5);
            csEvtSplitNum = 0;
            curRasCounter = 0;
            return;
        }

        /* last packet */
        if (slipt->packetIdx & 0x80) {
            /* All packets have been correctly received. */

            if (csEvtSplitNum != (slipt->packetIdx & 0x7f)) {
                tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] CS_RAS_CLIENT reassemble data, split packet loss! subnodeId=%d, sliptIndex=%d\n", slipt->snifferIndex, slipt->packetIdx);
                csEvtSplitNum = 0;
                curRasCounter = 0;
                return;
            }

            cmd_cs_ras_event_t *rasEvt = (cmd_cs_ras_event_t *)csEvtDataBusRx;
            u8                 sum     = check_sum(csEvtDataBusRx, rasEvt->dataLen + 3);
            if (sum == csEvtDataBusRx[rasEvt->dataLen + 3]) {
                curRasCounter                     = rasEvt->rangingCounter | 0x80000000;

                u16 rasDataLen = rasEvt->dataLen - 6;

                rasClientDataLen = rasDataLen;
                rasClientRangingCounter = rasEvt->rangingCounter;

                //tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] CS_RAS_CLIENT data Rx OK: subIdx=%d, snifHandle=0x%02X, ProcCnt=0x%04X, rasDataLen=%d\r\n", slipt->snifferIndex, slipt->snifferHandle, rasEvt->rangingCounter, rasDataLen);
            } else {
                tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] CS_RAS_CLIENT reassemble data, check sum error! subnodeId=%d, packetIndex=%02X, %02X, %02X\r\n", slipt->snifferIndex, slipt->packetIdx, sum, csEvtDataBusRx[rasEvt->dataLen + 3]);
                csEvtSplitNum = 0;
                curRasCounter = 0;
            }
        }
    }
    else if (rx_cmd->cmdId == SNIFFER_CMD_CS_RAS_SERVER_DATA_EVENT) {
        /* Due to the length of the cs ras serve event data, it is necessary to split the data into smaller
         * packets on the transmitter end and reassemble them on the receiving end.
         +-------+---------+---------------+----------------+-----------+----------------------+-----------+
         | cmdId | dataLen | snifferIndex  |  snifferHandle | packetIdx |        data          | checkSum  |
         |-------------------------------------------------------------------------------------------------|
         | 2Byte |  2Byte  |    1Byte      |     2Byte      |   1Byte   | SPP_SLIPT_PACKET_LEN |   1Byte   |
         +-------+---------+---------------+----------------+-----------+----------------------+-----------+
         * index:  bit0~bit6  0~127 sequence number
         *         bit7       0     start/continuation fragment
         *                    1     complete
         */
        u8                   *p     = (u8 *)rx_cmd;
        cmd_cs_event_slipt_t *slipt = (cmd_cs_event_slipt_t *)rx_cmd;

        if (slipt->snifferIndex >= LOCAL_SNIFFER_NUM_MAX) {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] CS_RAS_CLIENT slipt data, snifferIndex invalid! subnodeId=%d, sliptIndex=%d\n", slipt->snifferIndex, slipt->packetIdx);
            return;
        }

        /* focus only on data that matches current node */
        if (slipt->snifferIndex != nodeSetting.deviceIdx) {
            return;
        }

        u8 idx = slipt->snifferHandle & REMOTE_DEVICE_MAX_MASK;
        if (idx >= REMOTE_DEVICE_MAX_NUM) {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] CS_RAS_CLIENT slipt data, snifferHandle invalid! subnodeId=%d, snifHandle=%d, sliptIndex=%d\n", slipt->snifferIndex, slipt->snifferHandle, slipt->packetIdx);
            csEvtSplitNum = 0;
            curRasCounter = 0;
            return;
        }

        u8 check = check_sum(p, slipt->dataLen + 3);
        if (check != p[slipt->dataLen + 3]) {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] CS_RAS_SERVER slipt data, check sum error! subnodeId=%d, sliptIndex=%d, %02X, %02X\n", slipt->snifferIndex, slipt->packetIdx, check, p[slipt->dataLen + 3]);
            csEvtSplitNum = 0;
            curRasCounter = 0;
            return;
        }

        csEvtSplitNum++;
        /* first packet */
        if ((slipt->packetIdx & 0x7F) == 0) {
            memset(csEvtDataBusRx_2, 0, CS_EVT_DATA_BUS_SIZE);
            csEvtSplitNum = 0;
            curRasCounter = 0;
        }

        if (csEvtSplitNum != (slipt->packetIdx & 0x7F)) {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] CS_RAS_SERVER slipt data, not the expected packetIdx! subnodeId=%d, csEvtSplitNum=%02X, slipt->packetIdx=%02X\n", slipt->snifferIndex, csEvtSplitNum, slipt->packetIdx);

            csEvtSplitNum = 0;
            curRasCounter = 0;
            return;
        }

        u16 offset = SPP_SLIPT_PACKET_LEN * (slipt->packetIdx & 0x7F);
        u8  len    = slipt->dataLen - 5; /* u8(snifferIndex), u16(snifferHandle), u8(packetIdx), u8(checkSum) */
        if (len <= SPP_SLIPT_PACKET_LEN) {
            memcpy(csEvtDataBusRx_2 + offset, slipt->pData, slipt->dataLen - 5);
            tlkapi_printf(0, "[APP][BUS] CS_RAS_SERVER slipt data, subnodeId=%d, packetIndex=%02X, datalen=%d\r\n", slipt->snifferIndex, slipt->packetIdx, slipt->dataLen - 5);
        } else {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] CS_RAS_SERVER slipt data, length invalid! subnodeId=%d, packetIndex=%02X, datalen=%d\r\n", slipt->snifferIndex, slipt->packetIdx, slipt->dataLen - 5);
            csEvtSplitNum = 0;
            curRasCounter = 0;
            return;
        }

        /* last packet */
        if (slipt->packetIdx & 0x80) {
            /* All packets have been correctly received. */

            if (csEvtSplitNum != (slipt->packetIdx & 0x7f)) {
                tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] CS_RAS_SERVER reassemble data, split packet loss! subnodeId=%d, sliptIndex=%d\n", slipt->snifferIndex, slipt->packetIdx);
                csEvtSplitNum = 0;
                curRasCounter = 0;
                return;
            }

            cmd_cs_ras_event_t *rasEvt = (cmd_cs_ras_event_t *)csEvtDataBusRx_2;
            u8                 sum     = check_sum(csEvtDataBusRx_2, rasEvt->dataLen + 3);
            if (sum == csEvtDataBusRx_2[rasEvt->dataLen + 3]) {
                curRasCounter                     = rasEvt->rangingCounter | 0x80000000;

                u16 rasDataLen = rasEvt->dataLen - 6;

                //tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] CS_RAS_SERVER data Rx OK: subIdx=%d, snifHandle=0x%02X, ProcCnt=0x%04X, rasDataLen=%d\r\n", rasEvt->snifferIndex, rasEvt->snifferHandle, rasEvt->rangingCounter, rasDataLen);

                float distance[CS_DISTANCE_TYPE_SUPPORT_MAX] = {0.0};

                if(rasClientDataLen && rasDataLen) {
                    if(rasClientRangingCounter == rasEvt->rangingCounter) {
                        extern s32 snif_sub_node_calculate_cs_distacne(u16 snifHandle, u16 rasRangingCounter, u8 mainMode, u16 rasLocalLen, u16 rasRemoteLen, u8 *procCtrlInitiator, u8 *procCtrlReflector, float *distance);
                        s32 retVal = snif_sub_node_calculate_cs_distacne(rasEvt->snifferHandle, rasEvt->rangingCounter, 2, rasClientDataLen, rasDataLen, csEvtDataBusRx+9, csEvtDataBusRx_2+9, distance);

                        float cur_distance;
                        if (appCsParamSetting.rangingAlgMode & BLC_RANGING_ALGORITHM_3) {
                            cur_distance = distance[2];
                        }
                        else if(appCsParamSetting.rangingAlgMode & BLC_RANGING_ALGORITHM_2) {
                            cur_distance = distance[1];
                        }
                        else if(appCsParamSetting.rangingAlgMode & BLC_RANGING_ALGORITHM_1) {
                            cur_distance = distance[0];
                        }
                        else {
                            cur_distance = 0.0;
                            tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] distance error for rangingAlgMode mask not set");
                        }

                        tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] cs_dist, rngCnt=0x%X: sub%d=%.2f, retVal=%d\n", rasEvt->rangingCounter, nodeSetting.deviceIdx, cur_distance, retVal);

                        if(retVal == CS_DIST_SUCCESS) {
                            u8 *pTxBuff = (u8 *)(spp_tx_fifo.p + spp_tx_fifo.wptr * spp_tx_fifo.size);
                            spp_tx_fifo.wptr == (spp_tx_fifo.num - 1) ? spp_tx_fifo.wptr = 0 : spp_tx_fifo.wptr++;

                            spp_sub_node_cmd_cs_distanc_tx_t *ptr = (spp_sub_node_cmd_cs_distanc_tx_t *)pTxBuff;
                            ptr->dmaLen                           = sizeof(spp_sub_node_cmd_cs_distanc_tx_t) - 4; // exclude dmaLen
                            ptr->cmdId                            = SNIFFER_CMD_CS_DISTANCE;
                            ptr->dataLen                          = ptr->dmaLen - 4; // exclude cmdId and dataLen
                            ptr->snifferIndex                     = nodeSetting.deviceIdx;
                            ptr->snifferHandle                    = rasEvt->snifferHandle;
                            ptr->rangingCounter                   = rasEvt->rangingCounter;
                            ptr->distance                         = cur_distance;
                            ptr->checksum                         = check_sum((u8 *)&ptr->cmdId, ptr->dmaLen - 1);
                        }
                    }

                    rasClientDataLen = 0;
                    rasClientRangingCounter = 0xFFFF;
                }
            } else {
                tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] CS_RAS_SERVER reassemble data, check sum error! subnodeId=%d, packetIndex=%02X, %02X, %02X\r\n", slipt->snifferIndex, slipt->packetIdx, sum, csEvtDataBusRx_2[rasEvt->dataLen + 3]);
                csEvtSplitNum = 0;
                curRasCounter = 0;
            }
        }
    }
    #endif
}

void snif_sub_node_tx_data_process(void)
{
    u32 bus_tx_delay_us;
    #if (APP_TRANSPORT_UART_ENABLE)
        bus_tx_delay_us = 500;
    #elif (APP_TRANSPORT_LIN_ENABLE)
        bus_tx_delay_us = 300;//TODO need adjust
    #elif (APP_TRANSPORT_CANFD_ENABLE)
        bus_tx_delay_us = 200;
    #else
        bus_tx_delay_us = 600;
    #endif
    if (clock_time_exceed(app_bus_tx_tick, bus_tx_delay_us)) {
        app_bus_tx_tick = clock_time();
    } else {
        return;
    }

    #if (APP_TRANSPORT_UART_ENABLE || APP_TRANSPORT_LIN_ENABLE)
    if (!uart_dma_send_done_flag) {
        return;
    }
    #endif

    DBG_SNIF_CHN12_HIGH;
    if (spp_tx_fifo.wptr != spp_tx_fifo.rptr) {
        spp_sub_node_cmd_tx_t *tx_common_cmd = (spp_sub_node_cmd_tx_t *)(spp_tx_fifo.p + spp_tx_fifo.rptr * spp_tx_fifo.size);
    #if (!APP_TRANSPORT_CANFD_ENABLE)
        spp_tx_fifo.rptr == (spp_tx_fifo.num - 1) ? spp_tx_fifo.rptr = 0 : spp_tx_fifo.rptr++;
    #endif

    #if (APP_TRANSPORT_CANFD_ENABLE)
        int res = canfd_send_data_handle(nodeSetting.dataId, (u8 *)&tx_common_cmd->cmdId, tx_common_cmd->dmaLen);
        if (res == 0) {
            spp_tx_fifo.rptr == (spp_tx_fifo.num - 1) ? spp_tx_fifo.rptr = 0 : spp_tx_fifo.rptr++;
            DBG_SNIF_CHN13_TOGGLE;
        } else {
            DBG_SNIF_CHN14_TOGGLE;
        }
    #elif (APP_TRANSPORT_UART_ENABLE)
        u32 uart_tx_start_tick     = clock_time();
        u32 uart_transmit_max_time = UART_TX_WAIT_MAX_BYTE * 10 * 1000 * 1000 / UART_BAUD_RATE; //transmit time (us)
        while (!clock_time_exceed(uart_tx_start_tick, uart_transmit_max_time)) {
            if (uart_dma_send_done_flag) {
                u8  checkSum = 0;
                u8  checklen = tx_common_cmd->dataLen + 3; // exclude dmaLen and checksum, 3 = u16(cmdId) + u16(dataLen) - u8(checksum)
                u8  k;
                u8 *ptx = (u8 *)(tx_common_cmd);

                for (k = 0; k < checklen; k++) {
                    checkSum += *(ptx + k + 4); // skip dmaLen
                }
                *(ptx + k + 4) = checkSum;

                if (uart_send_dma(UART_MODULE_SEL, (u8 *)&tx_common_cmd->cmdId, tx_common_cmd->dmaLen)) {
                    uart_dma_send_done_flag = 0;
                    app_bus_tx_tick = clock_time();
                }
                break;
            }
        }
        if (clock_time_exceed(uart_tx_start_tick, uart_transmit_max_time)) {
            //reach send timeout
            uart_dma_send_done_flag = 1;
        }
    #endif
    }
    DBG_SNIF_CHN12_LOW;
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
    spp_common_cmd->snifferIndex               = nodeSetting.deviceIdx;
    spp_common_cmd->snifferHandle              = snifferHandle;
    spp_common_cmd->rssi                       = rssi_smooth;
    spp_common_cmd->snifferChannel             = snifferChannel;
    spp_common_cmd->deviceType                 = snifferType;
    spp_common_cmd->dmaLen                     = spp_common_cmd->dataLen + 4;

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
    spp_common_cmd->snifferIndex                   = nodeSetting.deviceIdx;
    spp_common_cmd->snifferHandle                  = pa->snifHandle;
    spp_common_cmd->status                         = pa->status;
    spp_common_cmd->dmaLen                         = spp_common_cmd->dataLen + 4;

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

        #if (APP_CAN_PM_ENABLE)
    /* RISING_EDGE      low-high -> sleep
         * FALLING_EDGE     high-low -> wake-up
         * */
    if (can_wake_up_flag) {
        can_wake_up_flag = 0;

        tcan4550_init();

        can_sleep_pending_tick = clock_time() | 1;

        //tlkapi_printf(APP_SNIF_LOG_EN,"[APP][SNIF] remote wake-up TCAN4550\r\n");
    }

    if (can_sleep_pending_tick && clock_time_exceed(can_sleep_pending_tick, 10 * 1000 * 1000)) {
        can_sleep_pending_tick = 0;
        u8 res                 = tcan4550_enter_sleep();
        //tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] TCAN4550 enter sleep, %d\r\n", res);
    }
        #endif

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
        if (sniffer_sync_rsp_flag[i] && (clock_time_exceed(sniffer_sync_rsp_tick[i], nodeSetting.deviceIdx * sniffer_data_send_delay))) {
            spp_sub_node_cmd_sync_rsp_tx_t *spp_common_cmd = (spp_sub_node_cmd_sync_rsp_tx_t *)date_sync_rsp[i];

            canfd_send_data_handle(nodeSetting.dataId, (u8 *)&spp_common_cmd->cmdId, spp_common_cmd->dataLen + 4);
            sniffer_sync_rsp_flag[i] = 0;

            if (log_sniffer_enable || spp_common_cmd->status) {
                tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] CMD_SYNC_RSP:0x%x,idx_%d,%d\n", spp_common_cmd->snifferHandle, spp_common_cmd->snifferIndex, spp_common_cmd->status);
            }
        }

        if (sniffer_rssi_report_flag[i] && (clock_time_exceed(sniffer_rssi_report_tick[i], nodeSetting.reportIntvl * 1000 + nodeSetting.deviceIdx * sniffer_data_send_delay))) {
            spp_sub_node_cmd_rssi_tx_t *spp_common_cmd = (spp_sub_node_cmd_rssi_tx_t *)rssi_report[i];

            sniffer_rssi_report_tick[i] = clock_time();
            canfd_send_data_handle(nodeSetting.dataId, (u8 *)&spp_common_cmd->cmdId, spp_common_cmd->dataLen + 4);
            sniffer_rssi_report_flag[i] = 0;

            if (log_sniffer_enable) {
                tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] RSSI:0x%x,idx_%d,%d,chl:%d,rssi:%d\n", spp_common_cmd->snifferHandle, spp_common_cmd->snifferIndex, spp_common_cmd->deviceType, spp_common_cmd->snifferChannel, spp_common_cmd->rssi - 110);
            }
        }
    }

    snif_sub_node_tx_data_process();

    #elif (APP_TRANSPORT_UART_ENABLE)
    // data_send delay nodeSetting.deviceIdx * sniffer_data_send_delay
    static _attribute_ble_data_retention_ u32 sniffer_data_send_delay = 500 + (1000000 * 10 * sizeof(spp_sub_node_cmd_rssi_tx_t)) / UART_BAUD_RATE; // unit us

    foreach (i, REMOTE_DEVICE_MAX_NUM) {
        if (sniffer_sync_rsp_flag[i] && (clock_time_exceed(sniffer_sync_rsp_tick[i], nodeSetting.deviceIdx * sniffer_data_send_delay))) {
            spp_sub_node_cmd_sync_rsp_tx_t *spp_common_cmd = (spp_sub_node_cmd_sync_rsp_tx_t *)date_sync_rsp[i];

            u32 uart_tx_start_tick     = clock_time();
            u32 uart_transmit_max_time = UART_TX_WAIT_MAX_BYTE * 10 * 1000 * 1000 / UART_BAUD_RATE; // 100 bytes transmit time (us)s
            while (!clock_time_exceed(uart_tx_start_tick, uart_transmit_max_time)) {
                if (uart_dma_send_done_flag) {
                    if (uart_send_dma(UART_MODULE_SEL, (u8 *)&spp_common_cmd->cmdId, spp_common_cmd->dataLen + 4)) {
                        uart_dma_send_done_flag = 0;
                        app_bus_tx_tick = clock_time();
                    }
                    break;
                }
            }
            if (clock_time_exceed(uart_tx_start_tick, uart_transmit_max_time)) {
                //reach send timeout
                uart_dma_send_done_flag = 1;
            }
            sniffer_sync_rsp_flag[i] = 0;

            if (log_sniffer_enable || spp_common_cmd->status) {
                tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] CMD_SYNC_RSP:0x%x,idx_%d,%d\n", spp_common_cmd->snifferHandle, spp_common_cmd->snifferIndex, spp_common_cmd->status);
            }
        }

        if (sniffer_rssi_report_flag[i] && (clock_time_exceed(sniffer_rssi_report_tick[i], nodeSetting.deviceIdx * sniffer_data_send_delay))) {
            spp_sub_node_cmd_rssi_tx_t *spp_common_cmd = (spp_sub_node_cmd_rssi_tx_t *)rssi_report[i];

            u32 uart_tx_start_tick     = clock_time();
            u32 uart_transmit_max_time = UART_TX_WAIT_MAX_BYTE * 10 * 1000 * 1000 / UART_BAUD_RATE; // 100 bytes transmit time (us)
            while (!clock_time_exceed(uart_tx_start_tick, uart_transmit_max_time)) {
                if (uart_dma_send_done_flag) {
                    if (uart_send_dma(UART_MODULE_SEL, (u8 *)&spp_common_cmd->cmdId, spp_common_cmd->dataLen + 4)) {
                        uart_dma_send_done_flag = 0;
                        app_bus_tx_tick = clock_time();
                    }
                    break;
                }
            }
            if (clock_time_exceed(uart_tx_start_tick, uart_transmit_max_time)) {
                //reach send timeout
                uart_dma_send_done_flag = 1;
            }
            sniffer_rssi_report_flag[i] = 0;

            if (log_sniffer_enable) {
                tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] RSSI:0x%x,idx_%d,%d,chl:%d,rssi:%d\n", spp_common_cmd->snifferHandle, spp_common_cmd->snifferIndex, spp_common_cmd->deviceType, spp_common_cmd->snifferChannel, spp_common_cmd->rssi - 110);
            }
        }
    }
    #endif

    #if (UI_LED_ENABLE)
    static _attribute_ble_data_retention_ u32 tick_str;
    if (clock_time_exceed(tick_str, 500 * 1000 * (nodeSetting.deviceIdx + 1))) {
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

void snif_sub_node_cs_procedure_subevent_result_event(u8 *param)
{
    hci_le_csSubeventResultEvt_t *subEvt = (hci_le_csSubeventResultEvt_t *)param;
    spp_cmd_cs_subevent_t        *p      = (spp_cmd_cs_subevent_t *)csEvtDataBusTx;

    p->dmaLen             = sizeof(cmd_cs_subevent_t);
    p->data.cmdId         = SNIFFER_CMD_CS_HCI_EVENT_PROCEDURE_SUBEVENT_RESULT;
    p->data.dataLen       = p->dmaLen - 4;
    p->data.snifferIndex  = nodeSetting.deviceIdx;
    p->data.snifferHandle = subEvt->Connection_Handle;

    u8 *stepData = param;
    u32 totalLen = sizeof(hci_le_csSubeventResultEvt_t);
    stepData += totalLen;
    for (u32 i = 0; i < subEvt->Num_Steps_Reported; i++) {
        totalLen += 3; /* mode, channel, len */
        totalLen += stepData[2];
        stepData += (3 + stepData[2]);
    }
    memcpy(p->data.pData, param, totalLen);
    p->dmaLen += totalLen;
    p->dmaLen += 1; /* check sum */
    p->data.dataLen           = p->dmaLen - 4;
    csEvtDataBusTx[p->dmaLen + 3] = check_sum((u8 *)&p->data.cmdId, p->dmaLen - 1);

    //tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] subevent: ConnHandle=0x%02X, ProcCnt=0x%04X, stepNum=%d, stepTotalLen=%d, checkSum=0x%02X, abortReason=0x%02X\r\n", subEvt->Connection_Handle, subEvt->Procedure_Counter, subEvt->Num_Steps_Reported, totalLen, csEvtDataBusTx[p->dmaLen + 3], subEvt->Abort_Reason);
    if (subEvt->Abort_Reason) {
        tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] subevent Abort: ConnHandle=0x%02X, ProcCnt=0x%04X, stepNum=%d, stepTotalLen=%d, checkSum=0x%02X, abortReason=0x%02X\r\n", subEvt->Connection_Handle, subEvt->Procedure_Counter, subEvt->Num_Steps_Reported, totalLen, csEvtDataBusTx[p->dmaLen + 3], subEvt->Abort_Reason);
    }

    /* Due to the length of the subevent result data, it is necessary to split the data into smaller
     * packets on the transmitter end and reassemble them on the receiving end.
     +-------+---------+---------------+----------------+-----------+----------------------+-----------+
     | cmdId | dataLen | snifferIndex  |  snifferHandle | packetIdx |        data          | checkSum  |
     |-------------------------------------------------------------------------------------------------|
     | 2Byte |  2Byte  |    1Byte      |     2Byte      |   1Byte   | SPP_SLIPT_PACKET_LEN |   1Byte   |
     +-------+---------+---------------+----------------+-----------+----------------------+-----------+
     * index:  bit0~bit6  0~127 sequence number
     *         bit7       0     start/continuation fragment
     *                    1     complete
     */
    totalLen      = p->dmaLen;
    u32 packetNum = (totalLen + SPP_SLIPT_PACKET_LEN - 1) / SPP_SLIPT_PACKET_LEN;
    u32 remain    = totalLen % SPP_SLIPT_PACKET_LEN; /* last packet */
    //tlkapi_printf(1, "subevent: totalLen=%d, totalNum=%d, remain=%d\r\n", totalLen, packetNum, remain);

    for (u8 j = 0; j < packetNum; j++) {
        DBG_SNIF_CHN6_TOGGLE;
        u8 *pTxBuff = (u8 *)(spp_tx_fifo.p + spp_tx_fifo.wptr * spp_tx_fifo.size);
        spp_tx_fifo.wptr == (spp_tx_fifo.num - 1) ? spp_tx_fifo.wptr = 0 : spp_tx_fifo.wptr++;

        spp_cmd_cs_event_slipt_t *ptr    = (spp_cmd_cs_event_slipt_t *)pTxBuff;
        ptr->dmaLen                      = sizeof(cmd_cs_event_slipt_t);
        ptr->data.cmdId                  = SNIFFER_CMD_CS_HCI_EVENT_PROCEDURE_SUBEVENT_RESULT;
        ptr->data.dataLen                = p->dmaLen - 4;
        ptr->data.snifferIndex           = nodeSetting.deviceIdx;
        ptr->data.snifferHandle          = subEvt->Connection_Handle;
        ptr->data.packetIdx              = j;

        u8 packetLen = SPP_SLIPT_PACKET_LEN;
        /* last packet */
        if (j == (packetNum - 1)) {
            ptr->data.packetIdx |= 0x80;
            packetLen = remain;
        }

        memcpy(ptr->data.pData, csEvtDataBusTx + 4 + (SPP_SLIPT_PACKET_LEN * j), packetLen);
        ptr->dmaLen += packetLen;
        ptr->dmaLen += 1; /* check sum */
        ptr->data.dataLen = ptr->dmaLen - 4;

        pTxBuff[ptr->dmaLen + 3] = check_sum((u8 *)&ptr->data.cmdId, ptr->dmaLen - 1);

        tlkapi_send_string_data(0, "subevent slipt j", pTxBuff, ptr->dmaLen + 4);

        tlkapi_printf(0, "subevent split: p->dataLen=%d, checkSem=%02x\r\n", ptr->data.dataLen, pTxBuff[ptr->dmaLen + 3]);
    }

//    blc_ras_csSubeventResultData(subEvt);
}

void snif_sub_node_cs_procedure_subevent_result_continue_event(u8 *param)
{
    hci_le_csSubeventResultContinueEvt_t *subEvt = (hci_le_csSubeventResultContinueEvt_t *)param;
    spp_cmd_cs_subevent_t                *p      = (spp_cmd_cs_subevent_t *)csEvtDataBusTx;

    p->dmaLen             = sizeof(cmd_cs_subevent_t);
    p->data.cmdId         = SNIFFER_CMD_CS_HCI_EVENT_PROCEDURE_SUBEVENT_RESULT_CONTINUE;
    p->data.dataLen       = p->dmaLen - 4;
    p->data.snifferIndex  = nodeSetting.deviceIdx;
    p->data.snifferHandle = subEvt->Connection_Handle;

    u8 *stepData = param;
    u32 totalLen = sizeof(hci_le_csSubeventResultContinueEvt_t);
    stepData += totalLen;
    for (u32 i = 0; i < subEvt->Num_Steps_Reported; i++) {
        totalLen += 3; /* mode, channel, len */
        totalLen += stepData[2];
        stepData += (3 + stepData[2]);
    }
    memcpy(p->data.pData, param, totalLen);
    p->dmaLen += totalLen;
    p->dmaLen += 1; /* check sum */
    p->data.dataLen           = p->dmaLen - 4;
    csEvtDataBusTx[p->dmaLen + 3] = check_sum((u8 *)&p->data.cmdId, p->dmaLen - 1);

    //tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] subeventConti: ConnHandle=0x%02X, stepNum=%d, stepTotalLen=%d, checkSum=0x%02X, abortReason=0x%02X\r\n", subEvt->Connection_Handle, subEvt->Num_Steps_Reported, totalLen, csEvtDataBusTx[p->dmaLen + 3], subEvt->Abort_Reason);
    if (subEvt->Abort_Reason) {
        tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] subeventConti Abort: ConnHandle=0x%02X, stepNum=%d, stepTotalLen=%d, checkSum=0x%02X, abortReason=0x%02X\r\n", subEvt->Connection_Handle, subEvt->Num_Steps_Reported, totalLen, csEvtDataBusTx[p->dmaLen + 3], subEvt->Abort_Reason);
    }

    /* Due to the length of the subevent result data, it is necessary to split the data into smaller
    * packets on the transmitter end and reassemble them on the receiving end.
    +-------+---------+---------------+----------------+-----------+----------------------+-----------+
    | cmdId | dataLen | snifferIndex  |  snifferHandle | packetIdx |        data          | checkSum  |
    |-------------------------------------------------------------------------------------------------|
    | 2Byte |  2Byte  |    1Byte      |     2Byte      |   1Byte   | SPP_SLIPT_PACKET_LEN |   1Byte   |
    +-------+---------+---------------+----------------+-----------+----------------------+-----------+
    * index:  bit0~bit6  0~127 sequence number
    *         bit7       0     start/continuation fragment
    *                    1     complete
    */
    totalLen      = p->dmaLen;
    u32 packetNum = (totalLen + SPP_SLIPT_PACKET_LEN - 1) / SPP_SLIPT_PACKET_LEN;
    u32 remain    = totalLen % SPP_SLIPT_PACKET_LEN; /* last packet */
    //tlkapi_printf(1, "subevent: totalLen=%d, totalNum=%d, remain=%d\r\n", totalLen, packetNum, remain);

    for (u8 j = 0; j < packetNum; j++) {
        DBG_SNIF_CHN8_TOGGLE;
        u8 *pTxBuff = (u8 *)(spp_tx_fifo.p + spp_tx_fifo.wptr * spp_tx_fifo.size);
        spp_tx_fifo.wptr == (spp_tx_fifo.num - 1) ? spp_tx_fifo.wptr = 0 : spp_tx_fifo.wptr++;

        spp_cmd_cs_event_slipt_t *ptr    = (spp_cmd_cs_event_slipt_t *)pTxBuff;
        ptr->dmaLen                      = sizeof(cmd_cs_event_slipt_t);
        ptr->data.cmdId                  = SNIFFER_CMD_CS_HCI_EVENT_PROCEDURE_SUBEVENT_RESULT_CONTINUE;
        ptr->data.dataLen                = p->dmaLen - 4;
        ptr->data.snifferIndex           = nodeSetting.deviceIdx;
        ptr->data.snifferHandle          = subEvt->Connection_Handle;
        ptr->data.packetIdx              = j;

        u8 packetLen = SPP_SLIPT_PACKET_LEN;
        /* last packet */
        if (j == (packetNum - 1)) {
            ptr->data.packetIdx |= 0x80;
            packetLen = remain;
        }

        memcpy(ptr->data.pData, csEvtDataBusTx + 4 + (SPP_SLIPT_PACKET_LEN * j), packetLen);
        ptr->dmaLen += packetLen;
        ptr->dmaLen += 1; /* check sum */
        ptr->data.dataLen = ptr->dmaLen - 4;

        pTxBuff[ptr->dmaLen + 3] = check_sum((u8 *)&ptr->data.cmdId, ptr->dmaLen - 1);

        tlkapi_printf(0, "subevent continue split: p->dataLen=%d, checkSem=%02x\r\n", ptr->data.dataLen, pTxBuff[ptr->dmaLen + 3]);
    }

//    blc_ras_csSubeventResultContinueData(subEvt);
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
            tlkapi_send_string_u8s(APP_SNIF_LOG_EN, "[APP][UART] uart0_irq UART_RX_ERR");
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
    if (spp_tx_fifo.wptr == spp_tx_fifo.rptr) {
        return -1;
    } else {
        snif_sub_node_tx_data_process();
    }
    return 0;
}
    #endif

void snif_set_rf_tx_power(u8 acl_tx_power, u8 cs_tx_power)
{
#if (MCU_CORE_TYPE == MCU_CORE_B92)
    /* for ACL RF power, unit: 1 dBm, Range: 0 to 10, convertCS_USE_TX_POWER_LEVEL to ACL rf power level index */
    u8 acl_tx_power_level_index;
    if (acl_tx_power == 10) {
        acl_tx_power_level_index = RF_POWER_INDEX_P9p90dBm;
    }
    else if (acl_tx_power == 9) {
        acl_tx_power_level_index = RF_POWER_INDEX_P9p15dBm;
    }
    else if (acl_tx_power == 8) {
        acl_tx_power_level_index = RF_POWER_INDEX_P8p25dBm;
    }
    else if (acl_tx_power == 7) {
        acl_tx_power_level_index = RF_POWER_INDEX_P7p00dBm;
    }
    else if (acl_tx_power == 6) {
        acl_tx_power_level_index = RF_POWER_INDEX_P6p32dBm;
    }
    else if (acl_tx_power == 5) {
        acl_tx_power_level_index = RF_POWER_INDEX_P5p21dBm;
    }
    else if (acl_tx_power == 4) {
        acl_tx_power_level_index = RF_POWER_INDEX_P4p02dBm;
    }
    else if (acl_tx_power == 3) {
        acl_tx_power_level_index = RF_POWER_INDEX_P3p00dBm;
    }
    else if (acl_tx_power == 2) {
        acl_tx_power_level_index = RF_POWER_INDEX_P2p01dBm;
    }
    else if (acl_tx_power == 1) {
        acl_tx_power_level_index = RF_POWER_INDEX_P1p03dBm;
    }
    else if (acl_tx_power == 0) {
        acl_tx_power_level_index = RF_POWER_INDEX_P0p31dBm;
    }
    else {
        acl_tx_power_level_index = MY_RF_POWER_INDEX;
    }
    rf_set_power_level_index(acl_tx_power_level_index);

    /* for CS RF power, unit: 1 dBm, Range: 0 to 10, convert to CS rf power level */
    u8 cs_tx_power_level;
    if (cs_tx_power == 10) {
        cs_tx_power_level = RF_POWER_P9p90dBm;
    }
    else if (cs_tx_power == 9) {
        cs_tx_power_level = RF_POWER_P9p15dBm;
    }
    else if (cs_tx_power == 8) {
        cs_tx_power_level = RF_POWER_P8p25dBm;
    }
    else if (cs_tx_power == 7) {
        cs_tx_power_level = RF_POWER_P7p00dBm;
    }
    else if (cs_tx_power == 6) {
        cs_tx_power_level = RF_POWER_P6p32dBm;
    }
    else if (cs_tx_power == 5) {
        cs_tx_power_level = RF_POWER_P5p21dBm;
    }
    else if (cs_tx_power == 4) {
        cs_tx_power_level = RF_POWER_P4p02dBm;
    }
    else if (cs_tx_power == 3) {
        cs_tx_power_level = RF_POWER_P3p00dBm;
    }
    else if (cs_tx_power == 2) {
        cs_tx_power_level = RF_POWER_P2p01dBm;
    }
    else if (cs_tx_power == 1) {
        cs_tx_power_level = RF_POWER_P1p03dBm;
    }
    else if (cs_tx_power == 0) {
        cs_tx_power_level = RF_POWER_P0p31dBm;
    }
    else {
        cs_tx_power_level = CS_USE_TX_POWER_LEVEL;
    }
    blc_cs_set_tx_power_level(cs_tx_power_level);
#else
    #error "!!! snif_set_rf_tx_power is not a chip supported by the sniffer SDK !!!"
#endif
}

static void snif_load_setting_from_flash(void)
{
    flash_read_page(FLASH_ADDRESS_SNIFFER_SETTING, sizeof(nodeSetting), (u8 *)&nodeSetting);

    /* for main node */
    if (nodeSetting.subNodeNumber > LOCAL_SNIFFER_NUM_MAX) {
        nodeSetting.subNodeNumber = 0;
    }

    if (nodeSetting.syncReqId == 0xFFFF) {
        nodeSetting.syncReqId = MAIN_NODE_TO_SUB_NODE_SYNC_SID;
    }

    if ((nodeSetting.reportIntvl > 10000) || (nodeSetting.reportIntvl < 30)) {
        //unit: 1ms
        nodeSetting.reportIntvl = 100;
    }

    if (nodeSetting.reportId == 0xFFFF) {
        nodeSetting.reportId = MAIN_NODE_TO_ECU_REPORT_SID;
    }

    /* for sub node */
    if (nodeSetting.deviceIdx > LOCAL_SNIFFER_INDEX_5) {
        nodeSetting.deviceIdx = LOCAL_SNIFFER_INDEX_0;
    }

    if (nodeSetting.dataId == 0xFFFF) {
        nodeSetting.dataId = SUB_NODE_TO_MAIN_NODE_DATA_SID_0;
    }

    if (nodeSetting.rspSyncId == 0xFFFF) {
        nodeSetting.rspSyncId = SUB_NODE_TO_MAIN_NODE_RSP_SID;
    }

    tlkapi_printf(APP_SNIF_LOG_EN, "[APP][INI] sub node setting from flash address 0x%06X, regAddr 0x%X\n", FLASH_ADDRESS_SNIFFER_SETTING, (void *)&nodeSetting);
    tlkapi_printf(APP_SNIF_LOG_EN, " ------------------------------\n");
    tlkapi_printf(APP_SNIF_LOG_EN, " param              value  unit\n");
    tlkapi_printf(APP_SNIF_LOG_EN, " ------------------------------\n");
    tlkapi_printf(APP_SNIF_LOG_EN, "  main node\n");
    tlkapi_printf(APP_SNIF_LOG_EN, "    subNodeNum       %04d\n", nodeSetting.subNodeNumber);
    tlkapi_printf(APP_SNIF_LOG_EN, "    syncReqId      0x%04X\n", nodeSetting.syncReqId);
    tlkapi_printf(APP_SNIF_LOG_EN, "    reportId       0x%04X\n", nodeSetting.reportId);
    tlkapi_printf(APP_SNIF_LOG_EN, "  sub node\n");
    tlkapi_printf(APP_SNIF_LOG_EN, "    deviceIdx        %04d\n", nodeSetting.deviceIdx);
    tlkapi_printf(APP_SNIF_LOG_EN, "    dataId         0x%04X\n", nodeSetting.dataId);
    tlkapi_printf(APP_SNIF_LOG_EN, "    rspSyncId      0x%04X\n", nodeSetting.rspSyncId);
    tlkapi_printf(APP_SNIF_LOG_EN, "  all node\n");
    tlkapi_printf(APP_SNIF_LOG_EN, "    reportIntvl      %04d  ms\n", nodeSetting.reportIntvl);
}

/*******************************************************************************
 *  Channel Sounding
 ******************************************************************************/
static void cs_load_setting_from_flash(void)
{
    flash_read_page(FLASH_ADDRESS_CS_PARAM_SETTING, sizeof(app_cs_param_setting_t), (u8 *)&appCsParamSetting);

    /* for CS Procedure Interval, Number of ACL connection events */
    if ((appCsParamSetting.procedureInterval == 0xFFFF) || (!appCsParamSetting.procedureInterval)) {
        if(nodeSetting.subNodeNumber) {
            if (MAX_ANT_PATHS_SUPPORT > 2) {
                appCsParamSetting.procedureInterval = 12;
            }
            else {
                appCsParamSetting.procedureInterval = 10;
            }
        }
        else {
            appCsParamSetting.procedureInterval = 20;
        }
    }

    /* for reserved, CS Subevent Len, unit: 1us, Range: 1250 us to 65000 us */
    if ((appCsParamSetting.subeventLen < 1250) || (appCsParamSetting.subeventLen > 65000)) {
        appCsParamSetting.subeventLen = 40000;
    }

    /* for CS ranging algorithm mode, reference to blc_ranging_algorithm_enum */
    if ((appCsParamSetting.rangingAlgMode < BLC_RANGING_ALGORITHM_1) || (appCsParamSetting.rangingAlgMode > BLC_RANGING_ALGORITHM_2)) {
        appCsParamSetting.rangingAlgMode = BLC_RANGING_ALGORITHM_2;
    }

    /* for CS distance kalman filter parameter proc_noise_cov, unit: 0.0001, Range: 1 to 200 */
    if ((appCsParamSetting.kalmanNoiseCov < 1) || (appCsParamSetting.kalmanNoiseCov > 200)) {
        appCsParamSetting.kalmanNoiseCov = 30;// 0.003 = 30 * 0.0001
    }

    /* for CS Channel Map, This parameter contains 80 1-bit fields */
    /*
     * This parameter contains 80 1-bit fields
     * Channels n = 0, 1, 23, 24, 25, 77, and 78 shall be ignored and shall be set to zero. At least 15 channels shall be enabled.
     * The most significant bit (bit 79) is reserved for future use.
     *
     * All valid  channels
     * 0xFC, 0xFF, 0x7F,  0xFC,  0xFF,  0xFF,  0xFF,  0xFF,  0xFF,  0x1F
     * 2~7 , 8~15, 16~22, 26~31, 32~39, 40~47, 48~55, 56~63, 64~71, 72~76
     */
    u8 csChannelMapEmpty[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    if (!memcmp(appCsParamSetting.channelMap, csChannelMapEmpty, 10)) {
        //Channel number 48
        appCsParamSetting.channelMap[0] = 0x00;
        appCsParamSetting.channelMap[1] = 0x00;
        appCsParamSetting.channelMap[2] = 0x00;
        appCsParamSetting.channelMap[3] = 0xF0;
        appCsParamSetting.channelMap[4] = 0xFF;
        appCsParamSetting.channelMap[5] = 0xFF;
        appCsParamSetting.channelMap[6] = 0xFF;
        appCsParamSetting.channelMap[7] = 0xFF;
        appCsParamSetting.channelMap[8] = 0xFF;
        appCsParamSetting.channelMap[9] = 0x0F;
    }

    /* for CS distance offset, unit: 10cm, Range: -500 to 500 */
    if (abs(appCsParamSetting.distanceOffset <= 500) && (appCsParamSetting.distanceOffset != -1)) {
        //app_cs_distance_offset = appCsParamSetting.distanceOffset / 100.0f;
    }

    /* for ACL RF power, unit: 1 dBm, Range: 0 to 10, convert to ACL rf power level index */
    if (appCsParamSetting.aclRfPowerIdx > 10) {
        appCsParamSetting.aclRfPowerIdx = 9;
    }

    /* for CS RF power, unit: 1 dBm, Range: 0 to 10, convert to CS rf power level */
    if (appCsParamSetting.csRfPowerLevel > 10) {
        appCsParamSetting.csRfPowerLevel = 7;
    }

    snif_set_rf_tx_power(appCsParamSetting.aclRfPowerIdx, appCsParamSetting.csRfPowerLevel);

    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] Param Setting from flash address 0x%06X, regAddr 0x%X\n", FLASH_ADDRESS_CS_PARAM_SETTING, (void *)&appCsParamSetting);
    tlkapi_printf(APP_CS_LOG_EN, "  procedureInterval:%d\n", appCsParamSetting.procedureInterval);
    tlkapi_printf(APP_CS_LOG_EN, "  subeventLen:%d us\n", appCsParamSetting.subeventLen);
    tlkapi_printf(APP_CS_LOG_EN, "  rangingAlgMode:0x%02X\n", appCsParamSetting.rangingAlgMode);
    tlkapi_printf(APP_CS_LOG_EN, "  kalmanNoiseCov:%d unit:0.0001\n", appCsParamSetting.kalmanNoiseCov);
    tlkapi_send_string_data(APP_CS_LOG_EN, "  channelMap", appCsParamSetting.channelMap, 10);
    tlkapi_printf(APP_CS_LOG_EN, "  aclRfPowerIdx:%d dBm\n", appCsParamSetting.aclRfPowerIdx);
    tlkapi_printf(APP_CS_LOG_EN, "  csRfPowerLevel:%d dBm\n", appCsParamSetting.csRfPowerLevel);
}

/**
 * @brief      sniffer sub node initialization
 * @param[in]  none
 * @return     none.
 */
void snif_sub_node_init(void)
{
    snif_load_setting_from_flash();
    cs_load_setting_from_flash();

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

    blc_ll_initCsSnifferSubNode_module(nodeSetting.deviceIdx + 1);
    //////////// Monitor Initialization  End /////////////////////////

    //////////// BLE Feature Initialization Begin /////////////////////////
    blc_ll_initChannelSelectionAlgorithm_2_feature(); //main_node support CSA#2
    /* Attention: sniffer support PHY switch (1M, 2M, Coded) */
    #if (APP_PHY_SWITCHING_ENABLE)
        blc_ll_init2MPhyCodedPhy_feature();
    #endif
    blc_ll_setAclSnifferMaxRxBufferLen(ACL_CONN_MAX_RX_OCTETS);

    tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] Feature:CSA #2,PHY Switching,Rx DLE:%d\n", ACL_CONN_MAX_RX_OCTETS);
    //////////// BLE Feature Initialization End /////////////////////////

    //////////// UART/CAN Initialization  Begin /////////////////////////
    #if (APP_TRANSPORT_UART_ENABLE)
        user_uart_init();
        blc_register_hci_handler(rx_from_uart_cb, tx_to_uart_cb); //customized uart handler
        extern void blc_ll_register_user_irq_handler_cb(user_irq_handler_cb_t cb);
        blc_ll_register_user_irq_handler_cb(uart0_irq_handler);
    #elif (APP_TRANSPORT_CANFD_ENABLE)
        tcan4550_init();
    #endif
    //////////// UART/CAN Initialization  End /////////////////////////

    //////////// APP CS Sniffer Initialization  Begin /////////////////////////
    #if (KALMAN_FILTER_ENABLE)
        //Initialize Kalman distance filter
        snif_kalman_Filter_init();
    #endif

    //Enable distance calculate algorithm, see blc_ranging_algorithm_enum.
    blc_cs_enableAlgoMask(appCsParamSetting.rangingAlgMode);
    //////////// APP CS Sniffer Initialization  End /////////////////////////

    tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] snif_sub_node_init:Index_%d,M%dS%d,BandRate_%d\n", nodeSetting.deviceIdx, ACL_CENTRAL_MAX_NUM, ACL_PERIPHR_MAX_NUM, UART_BAUD_RATE);
    tlkapi_printf(APP_SNIF_LOG_EN, "    Build time: %s %s\r\n", __DATE__, __TIME__);
}

#endif /* MONITOR_ROLE_SELECT == MONITOR_CS_CENTRAL_PERIPHERAL */
