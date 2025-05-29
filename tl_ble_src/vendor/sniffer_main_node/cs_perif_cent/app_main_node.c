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
#include "app_cs.h"
#include "app_main_node.h"
#include "app_parse_char.h"
#if (APP_TRANSPORT_CANFD_ENABLE)
    #include "../tcan4x5x/TCAN4550.h"
#endif
#include "math.h"

#include "algorithm/hadm/gcc10/cs_cal.h"


#if (MAIN_NODE_ROLE_SELECT == MAIN_NODE_CS_PERIPHERAL_CENTRAL)

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

_attribute_ble_data_retention_ volatile u8 log_sniffer_enable    = 0;
_attribute_ble_data_retention_ volatile u8 receive_bus_rssi_flag = 0;
_attribute_ble_data_retention_ u16 app_cs_distance_curConnHandle = 0;//only one connection is currently supported for CS ranging
_attribute_ble_data_retention_ u32 app_cs_distance_update_tick = 0;

_attribute_ble_data_retention_ node_setting_t nodeSetting;

_attribute_ble_data_retention_ kalmanFilter_t snifKalman[REMOTE_DEVICE_MAX_NUM][CHECK_SNIFFER_INDEX_MAX + 1];
_attribute_ble_data_retention_ float          printLatestDist[REMOTE_DEVICE_MAX_NUM][CHECK_SNIFFER_INDEX_MAX + 1][CS_DISTANCE_TYPE_SUPPORT_MAX] = {0};
_attribute_ble_data_retention_ u8             ampFilterCnt[REMOTE_DEVICE_MAX_NUM][CHECK_SNIFFER_INDEX_MAX + 1] = {0};

#if (APP_CS_THREE_POINT_POSITIONING_TAG_EN)
_attribute_ble_data_retention_ u8 nodeDistCnt[REMOTE_DEVICE_MAX_NUM][CHECK_SNIFFER_INDEX_MAX + 1] = {0};
#endif

    #if (APP_TRANSPORT_CANFD_ENABLE)
        #if (APP_CAN_PM_ENABLE)
_attribute_ble_data_retention_ u32 can_sleep_pending_tick = 0;
        #endif
/* When acting as the peripheral role, without the channel sounding feature, the buffer size used by one connection.
 +------------+----------------+---------------+----------------+
 | connHandle | main_node_rssi |  sub_node_id  |  sub_node_rssi |
 +------------+----------------+---------------+----------------+
 |   1Byte    |     1Byte      |      1Byte    |      1Byte     |
 +-----------------------------+--------------------------------+
 |             2Byte           |   2*CHECK_SNIFFER_INDEX_MAX    |
 +-----------------------------+--------------------------------+
 |                  2+2*CHECK_SNIFFER_INDEX_MAX                 |
 +--------------------------------------------------------------+
 */
_attribute_ble_data_retention_ u8  rssi_buf_peripheralRole[ACL_PERIPHR_MAX_NUM][2 + 2 * CHECK_SNIFFER_INDEX_MAX];
_attribute_ble_data_retention_ u32 rx_rssi_tick_Per[ACL_PERIPHR_MAX_NUM][CHECK_SNIFFER_INDEX_MAX];
_attribute_ble_data_retention_ u32 rssi_check_tick_Per = 0;

/* When acting as the central role, with channel sounding feature, the buffer size used by one connection.
 +------------+----------------+-----------------+---------------+----------------+-------------------+
 | connHandle | main_node_rssi | main_ndoe_cs_dis|  sub_node_id  |  sub_node_rssi |  sub_ndoe_cs_dis  |
 +------------+----------------+-----------------+---------------+----------------+-------------------+
 |   1Byte    |     1Byte      |      4Byte      |     1Byte     |      1Byte     |        4Byte      |
 +-----------------------------------------------+----------------------------------------------------+
 |                    6Byte                      |            6*CHECK_SNIFFER_INDEX_MAX               |
 +-----------------------------------------------+----------------------------------------------------+
 |                                     6+6*CHECK_SNIFFER_INDEX_MAX                                    |
 +----------------------------------------------------------------------------------------------------+
 */
_attribute_ble_data_retention_ u8  rssi_buf_centralRole[ACL_CENTRAL_MAX_NUM][6 + 6 * CHECK_SNIFFER_INDEX_MAX];
_attribute_ble_data_retention_ u32 rx_rssi_tick_Cen[ACL_CENTRAL_MAX_NUM][CHECK_SNIFFER_INDEX_MAX];
_attribute_ble_data_retention_ u32 rssi_check_tick_Cen = 0;

_attribute_ble_data_retention_ u32 app_rssi_report_tick[REMOTE_DEVICE_MAX_NUM][2] = {0};// 0:last tick, 1: last two tick

_attribute_ble_data_retention_ u32 cs_dis_tick_Cen[ACL_CENTRAL_MAX_NUM][CHECK_SNIFFER_INDEX_MAX]; // main_node cs
_attribute_ble_data_retention_ u32 cs_dis_check_tick_Cen = 0;
    #endif

    #define CS_EVT_DATA_BUS_SIZE (1856)
_attribute_ble_data_retention_ u8           csEvtDataBusTx[CS_EVT_DATA_BUS_SIZE];   // for bus transmit
_attribute_ble_data_retention_ u8           csEvtDataBusRx[CS_EVT_DATA_BUS_SIZE];   // for bus receive
_attribute_ble_data_retention_ volatile u8  csEvtSplitNum = 0;
_attribute_ble_data_retention_ volatile u32 curProcCounter = 0x00000000;                    //TODO, Need to define to multiple groups, consider multiple connections and config_id scenarios
_attribute_ble_data_retention_ u8           backupSubEvtResultHeader[16];                   //TODO, Need to define to multiple groups, consider multiple connections and config_id scenarios
_attribute_ble_data_retention_ u32          backupSubEvtResult_tick = 0;                    //TODO, Need to define to multiple groups, consider multiple connections and config_id scenarios

_attribute_ble_data_retention_ u8  rssi_crtl[SPP_TXFIFO_SIZE];
_attribute_ble_data_retention_ u8  connection_status_update_flag[REMOTE_DEVICE_MAX_NUM];
_attribute_ble_data_retention_ u32 connection_status_update_tick[REMOTE_DEVICE_MAX_NUM];
_attribute_ble_data_retention_ u8  connection_status_check_flag[REMOTE_DEVICE_MAX_NUM];
_attribute_ble_data_retention_ u8  connection_status_update_instantly[REMOTE_DEVICE_MAX_NUM];
_attribute_ble_data_retention_ u32 bus_error_tick = 0;
_attribute_ble_data_retention_ u32 app_bus_tx_tick = 0;

    #if (APP_TRANSPORT_UART_ENABLE)
_attribute_ble_data_retention_ u8 uart_dma_send_done_flag = 1;
    #endif

    #if (APP_TRANSPORT_CANFD_ENABLE)
int canfd_send_data_handle(u16 sid, u8 *pData, u32 len);
    #endif

_attribute_ble_data_retention_ app_cs_param_setting_t appCsParamSetting;
_attribute_ble_data_retention_ float app_cs_distance_offset = 0.1;  // unit: m

void app_cs_dist_clean(u8 connIdx, u8 nodeId);
void app_rssi_combine_report(u16 connHandle, u8 connIdx, u8 role);

_attribute_ram_code_ u8 check_sum(u8 *pdata, u32 len)
{
    u8 sum = 0;
    for (u32 i = 0; i < len; i++) {
        sum += pdata[i];
    }
    return sum;
}

void snif_main_node_connection_setup(u16 connHandle, u8 role)
{
    if ((role == ACL_ROLE_PERIPHERAL) || (role == ACL_ROLE_CENTRAL)) {
        u8 idx = blc_sniffer_getAclConnectionIndex(connHandle);
        if (idx > REMOTE_DEVICE_MAX_NUM) {
            tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] ConnHandle index invalid! %s\n", __FUNCTION__);
            return;
        }

        connection_status_update_flag[idx] = connHandle;
        connection_status_update_tick[idx] = clock_time();
        foreach (j, CHECK_SNIFFER_INDEX_MAX) {
            connection_status_check_flag[idx] |= BIT(j);
        }
    }

    #if (APP_TRANSPORT_CANFD_ENABLE && APP_CAN_PM_ENABLE)
    can_sleep_pending_tick = 0;
    if (gpio_read(TCAN4550_GPIO_WKREQ_N) == 1) {
        /* tcan4550 in sleep mode, reset */
        tcan4550_reset_hw();
        Init_CAN();
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
    #if (ACL_CENTRAL_SMP_ENABLE)
        // delete this device information(mac_address and distributed keys...) on FLash
        dev_char_info_t *dev_info = dev_char_info_search_by_connhandle(connHandle);
        if (dev_info) {
            blc_smp_deleteBondingPeripheralInfo_by_PeerMacAddress(dev_info->peer_adrType, dev_info->peer_addr);
            tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] delete peer device", &central_disconnect_connhandle, 2);
        }
    #endif
        sniffer_unpair_enable = 0;
    }

    u8 idx = blc_sniffer_getAclConnectionIndex(connHandle);
    if (idx > REMOTE_DEVICE_MAX_NUM) {
        tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] ConnHandle index invalid! %s\n", __FUNCTION__);
        return;
    }
    connection_status_update_flag[idx] = 0;

    #if (APP_TRANSPORT_CANFD_ENABLE)
    int role = dev_char_get_conn_role_by_connhandle(connHandle);
    if (role == ACL_ROLE_CENTRAL) {
        u8 *ptr = rssi_buf_centralRole[idx];
        blc_app_memory_set(ptr, 0xFF, sizeof(rssi_buf_centralRole[0]), sizeof(rssi_buf_centralRole), 0x11270000 | __LINE__);
    } else if (role == ACL_ROLE_PERIPHERAL) {
        u8 *ptr = rssi_buf_peripheralRole[idx];
        blc_app_memory_set(ptr, 0xFF, sizeof(rssi_buf_peripheralRole[0]), sizeof(rssi_buf_peripheralRole), 0x11280000 | __LINE__);
    }
    #endif

    app_cs_dist_clean(idx, 0xFF); // main node
    foreach (i, nodeSetting.subNodeNumber) {
        app_cs_dist_clean(idx, i); // sub node
    }

    if (app_cs_distance_curConnHandle == connHandle) {
        app_cs_distance_curConnHandle = 0;
        app_cs_distance_update_tick = 0;
    }
}

void snif_main_node_connection_update(u16 connHandle)
{
    u8 idx = blc_sniffer_getAclConnectionIndex(connHandle);
    if (idx > REMOTE_DEVICE_MAX_NUM) {
        tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] ConnHandle index invalid! %s\n", __FUNCTION__);
        return;
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

    u8 idx = blc_sniffer_getAclConnectionIndex(connHandle);
    if (idx > REMOTE_DEVICE_MAX_NUM) {
        tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] ConnHandle index invalid! %s\n", __FUNCTION__);
        return;
    }

    connection_status_update_flag[idx] = connHandle;
    connection_status_update_tick[idx] = clock_time();
    foreach (j, CHECK_SNIFFER_INDEX_MAX) {
        connection_status_check_flag[idx] |= BIT(j);
    }
    connection_status_update_instantly[idx] = connHandle;
}

/**
 * @brief      Update Sniffer listening status for CS config complete event.
 * @param[in]  param            - CS config complete event parameter, refer to 'hci_le_csConfigCompleteEvt_t'.
 * @return     none
 */
void snif_main_node_cs_config_complete_event(u8 *param)
{
    if (nodeSetting.subNodeNumber) {
        tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] %s\r\n", __FUNCTION__);
        hci_le_csConfigCompleteEvt_t *ptr = (hci_le_csConfigCompleteEvt_t *)param;

        spp_main_node_cmd_cs_event_tx_t *pTxBuff = (spp_main_node_cmd_cs_event_tx_t *)(spp_tx_fifo.p + spp_tx_fifo.wptr * spp_tx_fifo.size);
        spp_tx_fifo.wptr == (spp_tx_fifo.num - 1) ? spp_tx_fifo.wptr = 0 : spp_tx_fifo.wptr++;

        u8 hciEvent_len = sizeof(hci_le_csConfigCompleteEvt_t);

        pTxBuff->cmdId         = SNIFFER_CMD_CS_EVENT;
        pTxBuff->dataLen       = hciEvent_len + 4;     // 4 = u8(snifferIndex) + u16(snifferHandle) + u8(checksum)
        pTxBuff->dmaLen        = pTxBuff->dataLen + 4; // 4 = u16(cmdId) + u16(dataLen)
        pTxBuff->snifferIndex  = LOCAL_ALL_SNIFFER_INDEX;
        pTxBuff->snifferHandle = ptr->Connection_Handle;
        memcpy(pTxBuff->cs_event_data, param, hciEvent_len);
    }
}

/**
 * @brief      Update Sniffer listening status for CS security enable complete event.
 * @param[in]  param            - CS config complete event parameter, refer to 'hci_le_csSecurityEnableCompleteEvt_t'.
 * @return     none
 */
void snif_main_node_cs_security_enable_complete_event(u8 *param)
{
    if (nodeSetting.subNodeNumber) {
        tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] %s\r\n", __FUNCTION__);
        hci_le_csSecurityEnableCompleteEvt_t *ptr = (hci_le_csSecurityEnableCompleteEvt_t *)param;

        spp_main_node_cmd_cs_event_tx_t *pTxBuff = (spp_main_node_cmd_cs_event_tx_t *)(spp_tx_fifo.p + spp_tx_fifo.wptr * spp_tx_fifo.size);
        spp_tx_fifo.wptr == (spp_tx_fifo.num - 1) ? spp_tx_fifo.wptr = 0 : spp_tx_fifo.wptr++;

        u8 csSecurityParam[40];
        u8 csSecurityParam_len;

        extern u32 blc_ll_getCsSecurityParam(u16 connHandle, u8 * csSecurityParam);
        csSecurityParam_len = blc_ll_getCsSecurityParam(ptr->Connection_Handle, csSecurityParam);

        //tlkapi_send_string_data(APP_SNIF_LOG_EN, "[APP][SNIF] csSecurityParam", csSecurityParam, csSecurityParam_len);

        u8 hciEvent_len = sizeof(hci_le_csSecurityEnableCompleteEvt_t);

        pTxBuff->cmdId         = SNIFFER_CMD_CS_EVENT;
        pTxBuff->dataLen       = hciEvent_len + 4 + csSecurityParam_len; // 4 = u8(snifferIndex) + u16(snifferHandle) + u8(checksum)
        pTxBuff->dmaLen        = pTxBuff->dataLen + 4;                   // 4 = u16(cmdId) + u16(dataLen)
        pTxBuff->snifferIndex  = LOCAL_ALL_SNIFFER_INDEX;
        pTxBuff->snifferHandle = ptr->Connection_Handle;
        memcpy((u8 *)&pTxBuff->cs_event_data, param, hciEvent_len);
        u8 *targetAddress = pTxBuff->cs_event_data + hciEvent_len;
        blc_app_memory_copy(targetAddress, csSecurityParam, csSecurityParam_len, sizeof(csSecurityParam), 0x11250000 | __LINE__);
    }
}

/**
 * @brief      Update Sniffer listening status for CS procedure enable complete event.
 * @param[in]  param            - CS procedure enable complete event parameter, refer to 'hci_le_csProcedureEnableCompleteEvt_t'.
 * @return     none
 */
void snif_main_node_cs_procedure_enable_complete_event(u8 *param)
{
    if (nodeSetting.subNodeNumber) {
        tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] %s\r\n", __FUNCTION__);
        hci_le_csProcedureEnableCompleteEvt_t *ptr = (hci_le_csProcedureEnableCompleteEvt_t *)param;

        if (ptr->state) {
            app_cs_distance_curConnHandle = ptr->Connection_Handle;
            app_cs_distance_update_tick = clock_time()|1;
        }
        else {
            app_cs_distance_curConnHandle = 0;
            app_cs_distance_update_tick = 0;
        }

        spp_main_node_cmd_cs_event_tx_t *pTxBuff = (spp_main_node_cmd_cs_event_tx_t *)(spp_tx_fifo.p + spp_tx_fifo.wptr * spp_tx_fifo.size);
        spp_tx_fifo.wptr == (spp_tx_fifo.num - 1) ? spp_tx_fifo.wptr = 0 : spp_tx_fifo.wptr++;

        u8 csProceduretTimingParam[12];
        u8 csProceduretTimingParam_len;

        extern u32 blc_ll_getCsProceduretTimingParam(u16 connHandle, u8 ConfigID, u8 * csProceduretTimingParam);
        csProceduretTimingParam_len = blc_ll_getCsProceduretTimingParam(ptr->Connection_Handle, ptr->Config_ID, csProceduretTimingParam);

        //tlkapi_send_string_data(APP_SNIF_LOG_EN, "[APP][SNIF] csProceduretTimingParam", csProceduretTimingParam, csProceduretTimingParam_len);

        u8 hciEvent_len = sizeof(hci_le_csProcedureEnableCompleteEvt_t);

        pTxBuff->cmdId         = SNIFFER_CMD_CS_EVENT;
        pTxBuff->dataLen       = hciEvent_len + 4 + csProceduretTimingParam_len; // 4 = u8(snifferIndex) + u16(snifferHandle) + u8(checksum)
        pTxBuff->dmaLen        = pTxBuff->dataLen + 4;                           // 4 = u16(cmdId) + u16(dataLen)
        pTxBuff->snifferIndex  = LOCAL_ALL_SNIFFER_INDEX;
        pTxBuff->snifferHandle = ptr->Connection_Handle;
        memcpy((u8 *)&pTxBuff->cs_event_data, param, hciEvent_len);
        u8 *targetAddress = pTxBuff->cs_event_data + hciEvent_len;
        blc_app_memory_copy(targetAddress, csProceduretTimingParam, csProceduretTimingParam_len, sizeof(csProceduretTimingParam), 0x11260000 | __LINE__);
    }
}

void snif_main_node_cs_procedure_subevent_result_event(u8 *param)
{
    hci_le_csSubeventResultEvt_t *subEvt = (hci_le_csSubeventResultEvt_t *)param;

    //tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] subevent: ConnHandle=0x%02X, ProcCnt=0x%04X, stepNum=%d, abortReason=0x%02X\r\n", subEvt->Connection_Handle, subEvt->Procedure_Counter, subEvt->Num_Steps_Reported, subEvt->Abort_Reason);
    if (subEvt->Abort_Reason) {
        tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] subevent Abort: ConnHandle=0x%02X, ProcCnt=0x%04X, stepNum=%d, abortReason=0x%02X\r\n", subEvt->Connection_Handle, subEvt->Procedure_Counter, subEvt->Num_Steps_Reported, subEvt->Abort_Reason);
    }
}

void snif_main_node_cs_procedure_subevent_result_continue_event(u8 *param)
{
    hci_le_csSubeventResultContinueEvt_t *subEvt = (hci_le_csSubeventResultContinueEvt_t *)param;

    //tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] subeventConti: ConnHandle=0x%02X, stepNum=%d, abortReason=0x%02X\r\n", subEvt->Connection_Handle, subEvt->Num_Steps_Reported, subEvt->Abort_Reason);
    if (subEvt->Abort_Reason) {
        tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] subeventConti Abort: ConnHandle=0x%02X, stepNum=%d, abortReason=0x%02X\r\n", subEvt->Connection_Handle, subEvt->Num_Steps_Reported, subEvt->Abort_Reason);
    }
}

void snif_main_node_cs_ras_client_data_event(u16 rangingCounter, u8 curSubNodeIndex, u16 connHandle, u16 dataLen, u8 *pData)
{
    spp_cmd_cs_ras_event_t *p = (spp_cmd_cs_ras_event_t *)csEvtDataBusTx;

    p->dmaLen             = sizeof(cmd_cs_ras_event_t);
    p->data.cmdId         = SNIFFER_CMD_CS_RAS_CLIENT_DATA_EVENT;
    //p->data.dataLen       = p->dmaLen - 4;
    p->data.snifferIndex  = curSubNodeIndex;
    p->data.snifferHandle = connHandle;
    p->data.rangingCounter = rangingCounter;

    memcpy(p->data.pData, pData, dataLen);
    p->dmaLen += dataLen;
    p->dmaLen += 1; /* check sum */
    p->data.dataLen = p->dmaLen - 4;
    csEvtDataBusTx[p->dmaLen + 3] = check_sum((u8 *)&p->data.cmdId, p->dmaLen - 1);

    //tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] ras_client_data Tx: ConnHandle=0x%02X, rngCnt=0x%03X, subNode=%d, dataLen=%d, ckSum=0x%02X\r\n", connHandle, rangingCounter, curSubNodeIndex, p->data.dataLen-6, csEvtDataBusTx[p->dmaLen + 3]);

    /* Due to the length of the ras data, it is necessary to split the data into smaller
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
    u16 totalLen  = p->dmaLen;
    u32 packetNum = (totalLen + SPP_SLIPT_PACKET_LEN - 1) / SPP_SLIPT_PACKET_LEN;
    u32 remain    = totalLen % SPP_SLIPT_PACKET_LEN; /* last packet */

    for (u8 j = 0; j < packetNum; j++) {
        u8 *pTxBuff = (u8 *)(spp_tx_fifo.p + spp_tx_fifo.wptr * spp_tx_fifo.size);
        spp_tx_fifo.wptr == (spp_tx_fifo.num - 1) ? spp_tx_fifo.wptr = 0 : spp_tx_fifo.wptr++;

        spp_cmd_cs_event_slipt_t *ptr    = (spp_cmd_cs_event_slipt_t *)pTxBuff;
        ptr->dmaLen                      = sizeof(cmd_cs_event_slipt_t);
        ptr->data.cmdId                  = SNIFFER_CMD_CS_RAS_CLIENT_DATA_EVENT;
        ptr->data.dataLen                = p->dmaLen - 4;
        ptr->data.snifferIndex           = curSubNodeIndex;
        ptr->data.snifferHandle          = connHandle;
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

        //tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] ras_client_data split: j=%d, p->dataLen=%d, checkSem=%02x\r\n", j, ptr->data.dataLen, pTxBuff[ptr->dmaLen + 3]);
    }
}

void snif_main_node_cs_ras_server_data_event(u16 rangingCounter, u8 curSubNodeIndex, u16 connHandle, u16 dataLen, u8 *pData)
{
    spp_cmd_cs_ras_event_t *p = (spp_cmd_cs_ras_event_t *)csEvtDataBusTx;

    p->dmaLen             = sizeof(cmd_cs_ras_event_t);
    p->data.cmdId         = SNIFFER_CMD_CS_RAS_SERVER_DATA_EVENT;
    //p->data.dataLen       = p->dmaLen - 4;
    p->data.snifferIndex  = curSubNodeIndex;
    p->data.snifferHandle = connHandle;
    p->data.rangingCounter = rangingCounter;

    memcpy(p->data.pData, pData, dataLen);
    p->dmaLen += dataLen;
    p->dmaLen += 1; /* check sum */
    p->data.dataLen = p->dmaLen - 4;
    csEvtDataBusTx[p->dmaLen + 3] = check_sum((u8 *)&p->data.cmdId, p->dmaLen - 1);

    //tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] ras_server_data Tx: ConnHandle=0x%02X, rngCnt=0x%03X, subNode=%d, dataLen=%d, ckSum=0x%02X\r\n", connHandle, rangingCounter, curSubNodeIndex, p->data.dataLen-6, csEvtDataBusTx[p->dmaLen + 3]);

    /* Due to the length of the ras data, it is necessary to split the data into smaller
     * packets on the transmitter end and reassemble them on the receiving end.
     +-------+---------+---------------+----------------+-----------+----------------------+-----------+
     | cmdId | dataLen | snifferIndex  |  snifferHandle | packetIdx |        data          | checkSum  |
     |-------------------------------------------------------------------------------------------------|
     | 2Byte |  2Byte  |    1Byte      |     2Byte      |   1Byte   | SPP_SLIPT_PACKET_LEN |   1Byte   |
     +-------+---------+---------------+----------------+-----------+----------------------+-----------+
     * index:  bit0~bit6  0~127 sequence number
     *         bit7       0     start/continuation fraspp_cmd_cs_subevent_tgment
     *                    1     complete
     */
    u16 totalLen  = p->dmaLen;
    u32 packetNum = (totalLen + SPP_SLIPT_PACKET_LEN - 1) / SPP_SLIPT_PACKET_LEN;
    u32 remain    = totalLen % SPP_SLIPT_PACKET_LEN; /* last packet */

    for (u8 j = 0; j < packetNum; j++) {
        u8 *pTxBuff = (u8 *)(spp_tx_fifo.p + spp_tx_fifo.wptr * spp_tx_fifo.size);
        spp_tx_fifo.wptr == (spp_tx_fifo.num - 1) ? spp_tx_fifo.wptr = 0 : spp_tx_fifo.wptr++;

        spp_cmd_cs_event_slipt_t *ptr    = (spp_cmd_cs_event_slipt_t *)pTxBuff;
        ptr->dmaLen                      = sizeof(cmd_cs_event_slipt_t);
        ptr->data.cmdId                  = SNIFFER_CMD_CS_RAS_SERVER_DATA_EVENT;
        ptr->data.dataLen                = p->dmaLen - 4;
        ptr->data.snifferIndex           = curSubNodeIndex;
        ptr->data.snifferHandle          = connHandle;
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

        //tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] ras_server_data split: j=%d, p->dataLen=%d, checkSem=%02x\r\n", j, ptr->data.dataLen, pTxBuff[ptr->dmaLen + 3]);
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

#define CS_DIST_FILTER_NUM_LESS 5
#define CS_DIST_FILTER_NUM_MID  10
#define CS_DIST_FILTER_NUM_MANY 15
#define CS_DIST_FILTER_NUM_MORE 21
#if (APP_CS_THREE_POINT_POSITIONING_TAG_EN)
    #define CS_DIST_NUM         CS_DIST_FILTER_NUM_MORE
#else
    #define CS_DIST_NUM         CS_DIST_FILTER_NUM_LESS
#endif
#if (CS_DIST_NUM != 5 && CS_DIST_NUM != 10 && CS_DIST_NUM != 15 && CS_DIST_NUM != 21)
    #error "CS_DIST_FILTER_NUM set error !!!"
#endif
_attribute_ble_data_retention_ float store_cs_dist_value[REMOTE_DEVICE_MAX_NUM][CHECK_SNIFFER_INDEX_MAX + 1][CS_DIST_NUM];
_attribute_ble_data_retention_ float sort_cs_dist_value[REMOTE_DEVICE_MAX_NUM][CHECK_SNIFFER_INDEX_MAX + 1][CS_DIST_NUM];
_attribute_ble_data_retention_ u8    cs_dist_number[REMOTE_DEVICE_MAX_NUM][CHECK_SNIFFER_INDEX_MAX + 1];

_attribute_ram_code_ float cs_filter(u8 connIdx, u8 node_id, float distance)
{
    if (connIdx >= REMOTE_DEVICE_MAX_NUM) {
        return 0;
    }

    if ((node_id >= CHECK_SNIFFER_INDEX_MAX) && (node_id != 0xFF)) {
        return 0;
    }

    if (node_id == 0xFF) {
        /* The data storage location of the main node is at the end of the array. */
        node_id = CHECK_SNIFFER_INDEX_MAX;
    }

    //tlkapi_printf(APP_CAN_LOG_EN, "[APP][SNIF] %s: connIdx=%d, node_id=%d, distance=%n",connIdx, node_id, distance);

    if (isnan(distance)) {
        distance = store_cs_dist_value[connIdx][node_id][cs_dist_number[connIdx][node_id]];
    }

    float *pStore = (float *)&store_cs_dist_value[connIdx][node_id][0];
    float *pSort  = (float *)&sort_cs_dist_value[connIdx][node_id][0];
    //store
    if (cs_dist_number[connIdx][node_id] < CS_DIST_NUM) {
        pStore[cs_dist_number[connIdx][node_id]] = distance;
        cs_dist_number[connIdx][node_id]++;
        return distance;
    } else {
        for (u8 i = 0; i < (CS_DIST_NUM - 1); i++) {
            pStore[i] = pStore[i + 1];
        }
        pStore[CS_DIST_NUM - 1] = distance;
    }

    for (u8 cnt = 0; cnt < CS_DIST_NUM; cnt++) {
        pSort[cnt] = pStore[cnt];
    }

    //sort
    u8    i = 0, j = 0;
    float tmp_sort = 0;
    for (i = 0; i < CS_DIST_NUM - 1; i++) {
        for (j = 0; j < CS_DIST_NUM - 1 - i; j++) {
            if (pSort[j] > pSort[j + 1]) {
                tmp_sort     = pSort[j];
                pSort[j]     = pSort[j + 1];
                pSort[j + 1] = tmp_sort;
            }
        }
    }

    float dist_gauss_average;
    #if (0)
        //Maximum value
        dist_gauss_average = pSort[CS_DIST_NUM - 1];
    #elif (1)
        //smoothness in the center
        #if (CS_DIST_NUM == CS_DIST_FILTER_NUM_MORE)
            dist_gauss_average = (pSort[2] + 2 * pSort[3] + 3 * pSort[4] + 5 * pSort[5] + 7 * pSort[6] + 9 * pSort[7] + 11 * pSort[8] + 13 * pSort[9] + 15 * pSort[10]\
                                + pSort[18] + 2 * pSort[17] + 3 * pSort[16] + 5 * pSort[15] + 7 * pSort[14] + 9 * pSort[13] + 11 * pSort[12] + 13 * pSort[11]) / 117;
        #elif (CS_DIST_NUM == CS_DIST_FILTER_NUM_MANY)
            dist_gauss_average = (pSort[2] + 2 * pSort[3] + 3 * pSort[4] + 5 * pSort[5] + 7 * pSort[6] + 9 * pSort[7]\
                                + pSort[12] + 2 * pSort[11] + 3 * pSort[10] + 5 * pSort[9] + 7 * pSort[8]) / 45;
        #elif (CS_DIST_NUM == CS_DIST_FILTER_NUM_MID)
            dist_gauss_average = (pSort[2] + 2 * pSort[3] + 3 * pSort[4] + 8 * pSort[5] + 3 * pSort[6] + 2 * pSort[7] + pSort[8]) / 20;
        #elif (CS_DIST_NUM == CS_DIST_FILTER_NUM_LESS)
            dist_gauss_average = (pSort[0] + 3 * pSort[1] + 8 * pSort[2] + 3 * pSort[3] + pSort[4]) / 16;
        #endif
    #endif

    return dist_gauss_average;
}

void app_cs_dist_clean(u8 connIdx, u8 nodeId)
{
    if (connIdx >= REMOTE_DEVICE_MAX_NUM) {
        return;
    }

    if ((nodeId >= CHECK_SNIFFER_INDEX_MAX) && (nodeId != 0xFF)) {
        return;
    }
    if (nodeId == 0xFF) {
        /* The data storage location of the main node is at the end of the array. */
        nodeId = CHECK_SNIFFER_INDEX_MAX;
    }

    u8 *pStore = (u8 *)&store_cs_dist_value[connIdx][nodeId][0];
    u8 *pSort  = (u8 *)&sort_cs_dist_value[connIdx][nodeId][0];
    memset(pStore, 0, sizeof(store_cs_dist_value[connIdx][nodeId]));
    memset(pSort, 0, sizeof(sort_cs_dist_value[connIdx][nodeId]));
    cs_dist_number[connIdx][nodeId] = 0;

    snifKalman[connIdx][nodeId].state          = 0.0;
    snifKalman[connIdx][nodeId].err_cov        = 1.0;
    snifKalman[connIdx][nodeId].proc_noise_cov = appCsParamSetting.kalmanNoiseCov * 0.0001; //default value: 0.003;
    snifKalman[connIdx][nodeId].msr_noise_cov  = 0.01;
    snifKalman[connIdx][nodeId].kal_gain       = 0.0;
    snifKalman[connIdx][nodeId].update_tick    = 0;

    foreach (i, CS_DISTANCE_TYPE_SUPPORT_MAX) {
        printLatestDist[connIdx][nodeId][i] = 0;
    }

    ampFilterCnt[connIdx][nodeId] = 0;

    #if (APP_CS_THREE_POINT_POSITIONING_TAG_EN)
        nodeDistCnt[connIdx][nodeId] = 0;
    #endif
}

#if (APP_TRANSPORT_CANFD_ENABLE)
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

    return state;
}

void subnode_report_rssi_combine(u16 connHandle, u8 nodeId, u8 rssi)
{
    u8 connIdx = blc_sniffer_getAclConnectionIndex(connHandle);
    if (connIdx >= REMOTE_DEVICE_MAX_NUM) {
        tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] ConnHandle index invalid! %s\n", __FUNCTION__);
        return;
    }
    if (nodeId >= CHECK_SNIFFER_INDEX_MAX) {
        tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] Sub node index invalid! %s\n", __FUNCTION__);
        return;
    }

    //u8 main_node_rssi = blc_ll_getAclLatestAvgRSSI(connHandle);
    //tlkapi_printf(0, "[APP][SNIF] %s: connHandle=%02X, connIdx=%d, sub_node_id=%02X, main_node_rssi=%d, sub_node_rssi=%d\n", __FUNCTION__, connHandle, connIdx, nodeId, main_node_rssi -110, rssi - 110);

    u8 *ptr = NULL;
    int role = dev_char_get_conn_role_by_connhandle(connHandle);
    if ((role == ACL_ROLE_CENTRAL) || (role == ACL_ROLE_PERIPHERAL)) {
        if (role == ACL_ROLE_CENTRAL) {
            rx_rssi_tick_Cen[connIdx][nodeId] = 1;

            ptr    = rssi_buf_centralRole[connIdx];
            ptr[0] = connHandle;
            ptr    = ptr + (nodeId * 6 + 6);
            ptr[0] = nodeId;
            ptr[1] = rssi - 110;

            if (clock_time_exceed(rssi_check_tick_Cen, (nodeSetting.reportIntvl * 10 + 50) * 1000)) {
                rssi_check_tick_Cen = clock_time();
                for (u32 i = 0; i < ACL_CENTRAL_MAX_NUM; i++) {
                    for (u32 j = 0; j < CHECK_SNIFFER_INDEX_MAX; j++) {
                        if (rx_rssi_tick_Cen[i][j] == 0) {
                            ptr = rssi_buf_centralRole[i];
                            ptr = ptr + (j * 6 + 6);
                            memset(ptr + 1, 0, 5); /* clear sub_node_rssi and sub_node_cs_distance */
                            app_cs_dist_clean(connHandle, nodeId);
                        }
                    }
                }
                memset((u8 *)rx_rssi_tick_Cen, 0, sizeof(rx_rssi_tick_Cen));
            }    u8 *ptr = NULL;

        } else if (role == ACL_ROLE_PERIPHERAL) {
            #if (ACL_PERIPHR_MAX_NUM)
                connIdx -= ACL_CENTRAL_MAX_NUM;
                rx_rssi_tick_Per[connIdx][nodeId] = 1;

                ptr    = rssi_buf_peripheralRole[connIdx];
                ptr[0] = connHandle;
                ptr    = ptr + (nodeId * 2 + 2);
                ptr[0] = nodeId;
                ptr[1] = rssi - 110;

                if (clock_time_exceed(rssi_check_tick_Per, (nodeSetting.reportIntvl * 10 + 50) * 1000)) {
                    rssi_check_tick_Per = clock_time();
                    for (u32 i = 0; i < ACL_PERIPHR_MAX_NUM; i++) {
                        for (u32 j = 0; j < CHECK_SNIFFER_INDEX_MAX; j++) {
                            if (rx_rssi_tick_Per[i][j] == 0) {
                                ptr    = rssi_buf_peripheralRole[i];
                                ptr    = ptr + (j * 2 + 2);
                                ptr[1] = 0; /* clear sub_node_rssi */
                            }
                        }
                    }
                    memset((u8 *)rx_rssi_tick_Per, 0, sizeof(rx_rssi_tick_Per));
                }
            #endif
        }

        if (nodeSetting.rssiCombFlag) {
            #if (APP_TRANSPORT_CANFD_ENABLE)
                app_rssi_combine_report(connHandle, connIdx, role);
            #endif
        }
    }
    else {
        tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] Unknown role! %s\n", __FUNCTION__);
    }
}

void subnode_cs_distance_combine(u16 connHandle, u8 nodeId, float dis)
{
    u8 connIdx = blc_sniffer_getAclConnectionIndex(connHandle);
    if (connIdx >= REMOTE_DEVICE_MAX_NUM) {
        tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] ConnHandle index invalid! %s\n", __FUNCTION__);
        return;
    }
    if ((nodeId >= CHECK_SNIFFER_INDEX_MAX) && (nodeId != 0xFF)) {
        tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] Sub node index invalid! %s\n", __FUNCTION__);
        return;
    }

    tlkapi_printf(0, "[APP][CS] %s: connHandle=%04X, sub_node_id=%02X, dis=%f,idx=%d!\n", __FUNCTION__, connHandle, nodeId, dis, connIdx);

    u8 *ptr = NULL;
    int role = dev_char_get_conn_role_by_connhandle(connHandle);
    if (role == ACL_ROLE_CENTRAL) {
        cs_dis_tick_Cen[connIdx][nodeId] = 1;
        ptr                              = rssi_buf_centralRole[connIdx];
        if (nodeId < CHECK_SNIFFER_INDEX_MAX) {
            ptr += (nodeId * 6 + 6);
        }
        u32 diss = dis * 1000;
        ptr[2]   = diss & 0xFF;
        ptr[3]   = (diss >> 8) & 0xFF;
        ptr[4]   = (diss >> 16) & 0xFF;
        ptr[5]   = (diss >> 24) & 0xFF;

        if (clock_time_exceed(cs_dis_check_tick_Cen, (5000 * 1000))) {
            cs_dis_check_tick_Cen = clock_time();
            for (u32 i = 0; i < ACL_CENTRAL_MAX_NUM; i++) {
                for (u32 j = 0; j < CHECK_SNIFFER_INDEX_MAX; j++) {
                    if (cs_dis_tick_Cen[i][j] == 0) {
                        ptr = rssi_buf_centralRole[i];
                        ptr = ptr + (j * 6 + 6);
                        memset(ptr + 2, 0, 4); /* clear sub_node_cs_distance */
                        app_cs_dist_clean(connHandle, nodeId);
                    }
                }
            }
            memset((u8 *)cs_dis_tick_Cen, 0, sizeof(cs_dis_tick_Cen));
        }
    } else if (role == ACL_ROLE_PERIPHERAL) {
        tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] Channel sounding peripheral role!\n");
    } else {
        tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] Unknown role! %s\n", __FUNCTION__);
    }
}
    #endif


#if (MEDIAN_FILTER_ENABLE)

    #define MEDIAN_WIN_SIZE 5 // must be odd to make sure have median value in the slip window
float medianWin[MEDIAN_WIN_SIZE] = {0};
int   median_count               = 0;

float getMedian(float *window, int window_size)
{
    return window[window_size / 2];
}

void insertAndRemove(float *window, float new_value, float old_value, int window_size)
{
    // remove
    int i = 0;
    while (i < window_size && window[i] != old_value) {
        i++;
    }

    while (i < window_size - 1) {
        window[i] = window[i + 1];
        i++;
    }

    // insert
    i = 0;
    while (i < window_size - 1 && window[i] < new_value) {
        i++;
    }

    for (int j = window_size - 1; j > i; j--) {
        window[j] = window[j - 1];
    }

    window[i] = new_value;
}

float medianFilterRealTime(float new_value, float *window, int *count, int window_size)
{
    float median;
    if (*count < window_size) {
        window[*count] = new_value;
        (*count)++;

        // sort
        for (int i = 0; i < *count - 1; i++) {
            for (int j = 0; j < *count - i - 1; j++) {
                if (window[j] > window[j + 1]) {
                    int temp      = window[j];
                    window[j]     = window[j + 1];
                    window[j + 1] = temp;
                }
            }
        }
    } else {
        // update window
        insertAndRemove(window, new_value, window[0], window_size);
    }

    median = getMedian(window, *count);

    return median;
}
#endif

/**
 * @brief      First filter, limit amplitude.
 * @param[in]  distance: the origin distance.
 * @param[in]  lastValidDistance: last distance.
 * @param[in]  connIdx: acl connection index.
 * @param[in]  nodeId: node index.
 * @@return
 */
static float ampLimitFilter(float distance, float lastValidDistance, u8 connIdx, u8 nodeId)
{
    float filt_dis;

    if (distance < 0.01f || distance > 150.0f) {
        filt_dis = lastValidDistance;
        return filt_dis;
    }

    filt_dis = distance;

    if (lastValidDistance != 0.0) {
        float trend = (abs(distance - lastValidDistance));

        #if (APP_CS_THREE_POINT_POSITIONING_TAG_EN)
            if (trend > 4.0f) {
                if (distance > lastValidDistance) {
                    filt_dis = lastValidDistance + 1.0f;
                } else {
                    filt_dis = lastValidDistance - 1.0f;
                }
            }
        #else
            if (trend > 4.0f) {
                if (ampFilterCnt[connIdx][(nodeId == 0xFF) ? (CHECK_SNIFFER_INDEX_MAX) : nodeId] > 0) {
                    ampFilterCnt[connIdx][(nodeId == 0xFF) ? (CHECK_SNIFFER_INDEX_MAX) : nodeId] = 0;

                    float throb_dis = trend / 4;
                    if (throb_dis > 3.0f) {
                        throb_dis = 3.0f;
                    }

                    if (distance > lastValidDistance) {
                        filt_dis = lastValidDistance + throb_dis;
                    } else {
                        filt_dis = lastValidDistance - throb_dis;
                    }
                }
                else {
                    ampFilterCnt[connIdx][(nodeId == 0xFF) ? (CHECK_SNIFFER_INDEX_MAX) : nodeId]++;

                    filt_dis = lastValidDistance;
                }
            }
        #endif

        #if (0)
            if (MAX_ANT_PATHS_SUPPORT > 2) {
                /* if antenna paths greater than 2, use smaller distance amplitude limit filter */
                if (trend > 8.0f) {
                    if (distance > lastValidDistance) {
                        filt_dis = lastValidDistance + 2.0f;
                    } else {
                        filt_dis = lastValidDistance - 2.0f;
                    }
                }
            }
            else {
                /* if antenna paths less than 2, use larger distance amplitude limit filter */
                if (trend > 12.0f) {
                    if (distance > lastValidDistance) {
                        filt_dis = lastValidDistance + 3.0f;
                    } else {
                        filt_dis = lastValidDistance - 3.0f;
                    }
                }
            }
        #endif
    }

    return filt_dis;
}

#if (KALMAN_FILTER_ENABLE)
void snif_kalman_Filter_init(void)
{
    for (int i = 0; i < REMOTE_DEVICE_MAX_NUM; i++) {
        for (int j = 0; j < (CHECK_SNIFFER_INDEX_MAX + 1); j++) {
            snifKalman[i][j].state          = 0.0;
            snifKalman[i][j].err_cov        = 1.0;
            snifKalman[i][j].proc_noise_cov = appCsParamSetting.kalmanNoiseCov * 0.0001; //default value: 0.003;
            snifKalman[i][j].msr_noise_cov  = 0.01;
            snifKalman[i][j].kal_gain       = 0.0;
            snifKalman[i][j].update_tick    = 0;
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
    if (pkf->update_tick && (clock_time_exceed(pkf->update_tick, 5 * 1000 * 1000))) //5s
    {
        pkf->state    = 0.0;
        pkf->err_cov  = 1.0;
        pkf->kal_gain = 0.0;
    }
    pkf->update_tick = clock_time()|1;
    #endif

    pkf->err_cov += pkf->proc_noise_cov;
    pkf->kal_gain = pkf->err_cov / (pkf->err_cov + pkf->msr_noise_cov);
    pkf->state += pkf->kal_gain * (measurement - pkf->state);
    pkf->err_cov = (1 - pkf->kal_gain) * pkf->err_cov;

    return pkf->state;
}
#endif

void snif_main_node_cs_distacne_process(u16 connHandle, u16 rangingCounter, float distance1, float distance2, float distance3)
{
    u8 connIdx = blc_sniffer_getAclConnectionIndex(connHandle);
    if (connIdx >= REMOTE_DEVICE_MAX_NUM) {
        tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] ConnHandle index invalid! %s\n", __FUNCTION__);
        return;
    }

    if (dev_char_info_is_connection_state_by_conn_handle(connHandle) == FALSE) {
        /* currently connHandle not in the connected state */
        return;
    }

    float distance;
    if (appCsParamSetting.rangingAlgMode & BLC_RANGING_ALGORITHM_3) {
        distance = distance3;
    }
    else if(appCsParamSetting.rangingAlgMode & BLC_RANGING_ALGORITHM_2) {
        distance = distance2;
    }
    else if(appCsParamSetting.rangingAlgMode & BLC_RANGING_ALGORITHM_1) {
        distance = distance1;
    }
    else {
        distance = 0.0;
        tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] distance error for rangingAlgMode mask not set");
    }

    if (isnan(distance) || isinf(distance)) {
        distance = 0.0;
    }

    u8 nodeId = blc_sniffer_getSubNodeIndexByCsCounter(rangingCounter);
    if (nodeId != CS_COUNTER_CONVERT_SUB_NODE_INDEX_INVALID) {
        /* current is sub node index */
        if (nodeId >= CHECK_SNIFFER_INDEX_MAX) {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] subIdx invalid! %s, %d\n", __FUNCTION__, nodeId);
            return;
        }
        else {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] cs_dist, rngCnt=0x%X, sub%d=%.2f\n", rangingCounter, nodeId, distance);
            #if (APP_CS_THREE_POINT_POSITIONING_TAG_EN)
                app_parse_printf("sub%d=%.2f\n", nodeId, distance);
            #endif
        }
    }
    else {
        tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] cs_dist, rngCnt=0x%X, main=%.2f\n", rangingCounter, distance);
        #if (APP_CS_THREE_POINT_POSITIONING_TAG_EN)
            app_parse_printf("main=%.2f\n", distance);
        #endif
    }

    if (app_cs_distance_curConnHandle == connHandle) {
        app_cs_distance_update_tick = clock_time()|1;
    }

    #if (APP_CS_THREE_POINT_POSITIONING_TAG_EN)
        if (rangingCounter < ((nodeSetting.subNodeNumber + 1) * 2)) {
            /* only care about rangingCounter data from the start of round 3 */
            return;
        }
    #endif

    if(distance != 0.0) {
        /* remove distance offset */
        float random_dist = (trng_rand() % 10 + 10) / 100.0f;
        distance = ((distance - app_cs_distance_offset) > 0.0f) ? (distance - app_cs_distance_offset) : random_dist;
    }

    #if (APP_CS_THREE_POINT_POSITIONING_TAG_EN)
        distance = cs_filter(connIdx, nodeId, distance);

        printLatestDist[connIdx][(nodeId == 0xFF) ? (CHECK_SNIFFER_INDEX_MAX) : nodeId][0] = distance;

        //app_parse_printf("[APP][CS] node_0x%X, dist=%.2f\n", nodeId, printLatestDist[connIdx][(nodeId == 0xFF) ? (CHECK_SNIFFER_INDEX_MAX) : nodeId][0]);

        dev_char_info_t *dev_char_info = dev_char_info_search_by_connhandle(connHandle);
        u8 peerMAC[6];
        if (dev_char_info) {
            memcpy(peerMAC, dev_char_info->peer_addr, 6);
        }
        else {
            memset(peerMAC, 0xFF, 6);
        }
        tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] dist: main=%.2f, sub0=%.2f, sub1=%.2f, peerMAC={%X %X %X %X %X %X}\n", printLatestDist[connIdx][CHECK_SNIFFER_INDEX_MAX][0], printLatestDist[connIdx][0][0], printLatestDist[connIdx][1][0], \
                      peerMAC[5], peerMAC[4], peerMAC[3], peerMAC[2], peerMAC[1], peerMAC[0]);

        if (distance) {
            nodeDistCnt[connIdx][(nodeId == 0xFF) ? (CHECK_SNIFFER_INDEX_MAX) : nodeId]++;

            /*
              "title": "cs_dist", "main":{ "dist":[12.7]}, "sub0":{ "dist":[12.1]}, "sub1":{ "dist":[12.3]}, "peerMAC":"3C CF B4 25 A4 FE"}
               */
            #if (0) //no need real-time output, only final output once
                if (nodeDistCnt[connIdx][CHECK_SNIFFER_INDEX_MAX]) {
                    u8 dist_allNodeValid_flag = 1;
                    foreach (i, nodeSetting.subNodeNumber) {
                        if (nodeDistCnt[connIdx][i] == 0) {
                            dist_allNodeValid_flag = 0;
                            break;
                        }
                    }

                    if (dist_allNodeValid_flag) {
                        app_parse_printf("{\"title\":\"cs_dist\",\"main\":{\"dist\":[%.1f]}, ", printLatestDist[connIdx][CHECK_SNIFFER_INDEX_MAX][0]);
                        app_parse_printf("\"sub0\":{\"dist\":[%.1f]},", printLatestDist[connIdx][0][0]);
                        app_parse_printf("\"sub1\":{\"dist\":[%.1f]},", printLatestDist[connIdx][1][0]);
                        app_parse_printf("\"peerMAC\":\"%X %X %X %X %X %X\"}\n", peerMAC[5], peerMAC[4], peerMAC[3], peerMAC[2], peerMAC[1], peerMAC[0]);
                    }
                }
            #endif

            if (nodeDistCnt[connIdx][CHECK_SNIFFER_INDEX_MAX] > CS_DIST_NUM) {
                u8 dist_allNodeSufficient_flag = 1;
                foreach (i, nodeSetting.subNodeNumber) {
                    if (nodeDistCnt[connIdx][i] <= CS_DIST_NUM) {
                        dist_allNodeSufficient_flag = 0;
                        break;
                    }
                }

                if (dist_allNodeSufficient_flag) {
                    app_parse_printf("{ \"title\": \"cs_dist\", \"main\":{ \"dist\":[%.1f]}, ", printLatestDist[connIdx][CHECK_SNIFFER_INDEX_MAX][0]);
                    app_parse_printf("\"sub0\":{ \"dist\":[%.1f]}, ", printLatestDist[connIdx][0][0]);
                    app_parse_printf("\"sub1\":{ \"dist\":[%.1f]}, ", printLatestDist[connIdx][1][0]);
                    app_parse_printf("\"peerMAC\":\"%X %X %X %X %X %X\"}\n", peerMAC[5], peerMAC[4], peerMAC[3], peerMAC[2], peerMAC[1], peerMAC[0]);

                    app_parse_printf("all node distance sufficient, will disconnect\n");
                    blc_ll_disconnect(connHandle, HCI_ERR_RESERVED2);
                }
            }
         }
    #else
        #if (1) // use filter
            float original_dist = distance;

            float last_dist = snifKalman[connIdx][(nodeId == 0xFF) ? (CHECK_SNIFFER_INDEX_MAX) : nodeId].state;

            float alf_dist = ampLimitFilter(distance, last_dist, connIdx, nodeId);

            #if 0
                distance = cs_filter(connIdx, node_id, distance);
            #else
                distance = snif_kalman_filter_update(&snifKalman[connIdx][(nodeId == 0xFF) ? (CHECK_SNIFFER_INDEX_MAX) : nodeId], alf_dist);
            #endif
        #endif

        #if (0) // for test the main node distance of different filters
            printLatestDist[connIdx][CHECK_SNIFFER_INDEX_MAX][0] = original_dist;
            printLatestDist[connIdx][0][0] = alf_dist;
            printLatestDist[connIdx][1][0] = distance;//kalman_filter
            printLatestDist[connIdx][2][0] = 0.0;
            printLatestDist[connIdx][3][0] = last_dist;
        #else
            printLatestDist[connIdx][(nodeId == 0xFF) ? (CHECK_SNIFFER_INDEX_MAX) : nodeId][0] = distance;
        #endif
        /*
            {"title":"cs_dist","main":{"dist":[1.2]},"sub0":{"dist":[3.1]},"sub1":{"dist":[5.0]},"sub2":{"dist":[7.2]},"sub3":{ "dist":[9.3]}}
             */
        app_parse_printf("{\"title\":\"cs_dist\",\"main\":{\"dist\":[%.1f]},", printLatestDist[connIdx][CHECK_SNIFFER_INDEX_MAX][0]);
        app_parse_printf("\"sub0\":{\"dist\":[%.1f]},", printLatestDist[connIdx][0][0]);
        app_parse_printf("\"sub1\":{\"dist\":[%.1f]},", printLatestDist[connIdx][1][0]);
        app_parse_printf("\"sub2\":{\"dist\":[%.1f]},", printLatestDist[connIdx][2][0]);
        app_parse_printf("\"sub3\":{\"dist\":[%.1f]}}\n", printLatestDist[connIdx][3][0]);

        tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] dist: main=%.2f, sub0=%.2f, sub1=%.2f, sub2=%.2f, sub3=%.2f\n", printLatestDist[connIdx][CHECK_SNIFFER_INDEX_MAX][0], printLatestDist[connIdx][0][0], printLatestDist[connIdx][1][0], printLatestDist[connIdx][2][0], printLatestDist[connIdx][3][0]);
    #endif

    #if (APP_TRANSPORT_CANFD_ENABLE)
        subnode_cs_distance_combine(connHandle, nodeId, distance);
    #endif
}

/**
 * @brief      callBack function of LinkLayer Event "BLT_EV_FLAG_ACL_EVERY_CONN_EVENT"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
_attribute_ram_code_ void user_acl_every_connection_event (u8 e, u8 *p, int n)
{
    acl_every_conn_eventEvt_t *pa = (acl_every_conn_eventEvt_t *)p;

    u16 connHandle = pa->connHandle;

    u8 connIdx = blc_sniffer_getAclConnectionIndex(connHandle);
    if (connIdx >= REMOTE_DEVICE_MAX_NUM) {
        tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] ConnHandle index invalid! %s\n", __FUNCTION__);
        return;
    }

    u8 main_node_rssi = blc_ll_getAclLatestAvgRSSI(connHandle);
    main_node_rssi    = rssi_filter(connIdx, main_node_rssi);

    u8 *ptr = NULL;
    int role = dev_char_get_conn_role_by_connhandle(connHandle);
    if ((role == ACL_ROLE_CENTRAL) || (role == ACL_ROLE_PERIPHERAL)) {
        if (role == ACL_ROLE_CENTRAL) {
            ptr    = rssi_buf_centralRole[connIdx];
            ptr[0] = connHandle;
            ptr[1] = main_node_rssi - 110;
        } else if (role == ACL_ROLE_PERIPHERAL) {
            #if (ACL_PERIPHR_MAX_NUM)
                connIdx -= ACL_CENTRAL_MAX_NUM;
                ptr    = rssi_buf_peripheralRole[connIdx];
                ptr[0] = connHandle;
                ptr[1] = main_node_rssi - 110;
            #endif
        }

        if (nodeSetting.rssiCombFlag) {
            #if (APP_TRANSPORT_CANFD_ENABLE)
                app_rssi_combine_report(connHandle, connIdx, role);
            #endif
        }
    }
    else {
        tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] Unknown role! %s\n", __FUNCTION__);
    }
}

#if (APP_TRANSPORT_CANFD_ENABLE)
void app_rssi_combine_report(u16 connHandle, u8 connIdx, u8 role)
{
    if (clock_time_exceed(app_rssi_report_tick[connIdx][0], ((nodeSetting.reportIntvl - 20) * 1000)) || \
            clock_time_exceed(app_rssi_report_tick[connIdx][1], ((nodeSetting.reportIntvl * 2  - 40) * 1000))) {
        app_rssi_report_tick[connIdx][1] = app_rssi_report_tick[connIdx][0];
        app_rssi_report_tick[connIdx][0] = clock_time()|1;
        u8 rssi_buf_start_idx;

        app_parse_printf("\n");
        if (role == ACL_ROLE_CENTRAL) {
            if (connHandle == 0x80) {
                rssi_buf_start_idx = 0;
                app_parse_printf("{\"title\":\"0x80_rssi\",");
            }
            else if (connHandle == 0x81) {
                rssi_buf_start_idx = 1;
                app_parse_printf("{\"title\":\"0x81_rssi\",");
            }
            else if (connHandle == 0x82) {
                rssi_buf_start_idx = 2;
                app_parse_printf("{\"title\":\"0x82_rssi\",");
            }
            else if (connHandle == 0x83) {
                rssi_buf_start_idx = 3;
                app_parse_printf("{\"title\":\"0x83_rssi\",");
            }

            app_parse_printf("\"main\":{\"rssi\":[%d]},", (s8)rssi_buf_centralRole[rssi_buf_start_idx][1]);
            app_parse_printf("\"sub0\":{\"rssi\":[%d]},", (s8)rssi_buf_centralRole[rssi_buf_start_idx][6+1]);
            app_parse_printf("\"sub1\":{\"rssi\":[%d]},", (s8)rssi_buf_centralRole[rssi_buf_start_idx][12+1]);
            app_parse_printf("\"sub2\":{\"rssi\":[%d]},", (s8)rssi_buf_centralRole[rssi_buf_start_idx][18+1]);
            app_parse_printf("\"sub3\":{\"rssi\":[%d]}}\n", (s8)rssi_buf_centralRole[rssi_buf_start_idx][24+1]);

            tlkapi_printf(APP_RSSI_LOG_EN, "[APP][RSSI] 0x%x_rssi: main=%d, sub0=%d, sub1=%d, sub2=%d, sub3=%d\n",
                          connHandle, (s8)rssi_buf_centralRole[rssi_buf_start_idx][1],
                          (s8)rssi_buf_centralRole[rssi_buf_start_idx][6+1], (s8)rssi_buf_centralRole[rssi_buf_start_idx][12+1],
                          (s8)rssi_buf_centralRole[rssi_buf_start_idx][18+1], (s8)rssi_buf_centralRole[rssi_buf_start_idx][24+1]);
        } else if (role == ACL_ROLE_PERIPHERAL) {
            if (connHandle == 0x44) {
                rssi_buf_start_idx = 0;
                app_parse_printf("{ \"title\": \"0x44_rssi\", ");
            }
            else if (connHandle == 0x45) {
                rssi_buf_start_idx = 1;
                app_parse_printf("{ \"title\": \"0x45_rssi\", ");
            }
            else if (connHandle == 0x46) {
                rssi_buf_start_idx = 2;
                app_parse_printf("{ \"title\": \"0x46_rssi\", ");
            }
            else if (connHandle == 0x47) {
                rssi_buf_start_idx = 3;
                app_parse_printf("{ \"title\": \"0x47_rssi\", ");
            }

            app_parse_printf("\"main\":{ \"rssi\":[%d]}, ", (s8)rssi_buf_peripheralRole[rssi_buf_start_idx][1]);
            app_parse_printf("\"sub0\":{ \"rssi\":[%d]}, ", (s8)rssi_buf_peripheralRole[rssi_buf_start_idx][2+1]);
            app_parse_printf("\"sub1\":{ \"rssi\":[%d]}, ", (s8)rssi_buf_peripheralRole[rssi_buf_start_idx][4+1]);
            app_parse_printf("\"sub2\":{ \"rssi\":[%d]}, ", (s8)rssi_buf_peripheralRole[rssi_buf_start_idx][6+1]);
            app_parse_printf("\"sub3\":{ \"rssi\":[%d]}}\n", (s8)rssi_buf_peripheralRole[rssi_buf_start_idx][8+1]);

            tlkapi_printf(APP_RSSI_LOG_EN, "[APP][RSSI] 0x%x_rssi: main=%d, sub0=%d, sub1=%d, sub2=%d, sub3=%d\n",
                          connHandle, (s8)rssi_buf_peripheralRole[rssi_buf_start_idx][1],
                          (s8)rssi_buf_peripheralRole[rssi_buf_start_idx][2+1], (s8)rssi_buf_peripheralRole[rssi_buf_start_idx][4+1],
                          (s8)rssi_buf_peripheralRole[rssi_buf_start_idx][6+1], (s8)rssi_buf_peripheralRole[rssi_buf_start_idx][8+1]);
        }
    }
}
#endif

void snif_main_node_rx_data_process(void)
{
    if (spp_rx_fifo.wptr == spp_rx_fifo.rptr) {
        return;
    }
    DBG_SNIF_CHN10_HIGH;
    spp_main_node_cmd_rx_t *rx_common_cmd = (spp_main_node_cmd_rx_t *)(spp_rx_fifo.p + spp_rx_fifo.rptr * spp_rx_fifo.size);
    spp_rx_fifo.rptr == (spp_rx_fifo.num - 1) ? spp_rx_fifo.rptr = 0 : spp_rx_fifo.rptr++;
    //tlkapi_send_string_data(APP_SNIF_LOG_EN, "[APP][SNIF] Rx", (u8*)&rx_common_cmd->cmdId, rx_common_cmd->dataLen+4);

    #if (UI_LED_ENABLE)
    //rx data from bus
    gpio_toggle(GPIO_LED_BLUE);
    #endif

    u8 idx = blc_sniffer_getAclConnectionIndex(rx_common_cmd->snifferHandle);
    if (idx > REMOTE_DEVICE_MAX_NUM) {
        tlkapi_printf(APP_SNIF_LOG_EN, "[APP][BUS] snifferHandle index invalid! %s\n", __FUNCTION__);
        tlkapi_send_string_data(APP_SNIF_LOG_EN, "[APP][BUS] idx error", (u8 *)rx_common_cmd, 64);
        return;
    }

    if (rx_common_cmd->cmdId == SNIFFER_CMD_RSSI) {
        spp_main_node_cmd_rssi_rx_t *common_cmd = (spp_main_node_cmd_rssi_rx_t *)rx_common_cmd;

        u8  rx_snifferIndex   = common_cmd->snifferIndex;
        u16 rx_snifferHandle  = common_cmd->snifferHandle;
        u8  rx_snifferRssi    = common_cmd->rssi;
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
                tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] RSSI:0x%x,idx_%d,%d,chl:%d,rssi:%d\n", rx_snifferHandle, rx_snifferIndex, rx_deviceType, rx_snifferChannel, rx_snifferRssi - 110);
            }
            #if (APP_TRANSPORT_CANFD_ENABLE)
                subnode_report_rssi_combine(rx_snifferHandle, rx_snifferIndex, rx_snifferRssi);
                receive_bus_rssi_flag = 1;
            #endif
        } else {
            tlkapi_printf(APP_SNIF_LOG_EN, "[APP][BUS] Rx RSSI, check sum error! subnodeId=%d, snifHandle=0x%02x\r\n", common_cmd->snifferIndex, common_cmd->snifferHandle);
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

            if (log_sniffer_enable || common_cmd->status) {
                tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] SYNC_RSP:0x%x,idx_%d,%d\n", common_cmd->snifferHandle, common_cmd->snifferIndex, common_cmd->status);
            }
        } else {
            tlkapi_printf(APP_SNIF_LOG_EN, "[APP][BUS] Rx SYNC_RSP, check sum error! subnodeId=%d, snifHandle=0x%02x\r\n", common_cmd->snifferIndex, common_cmd->snifferHandle);
        }
    }
    /* 0xD2 - CS Procedure subevent result. */
    else if (rx_common_cmd->cmdId == SNIFFER_CMD_CS_HCI_EVENT_PROCEDURE_SUBEVENT_RESULT) {
        DBG_SNIF_CHN6_HIGH;
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
        u8                   *p     = (u8 *)rx_common_cmd;
        cmd_cs_event_slipt_t *slipt = (cmd_cs_event_slipt_t *)rx_common_cmd;

        if (slipt->snifferIndex >= CHECK_SNIFFER_INDEX_MAX) {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] Subevent slipt data, snifferIndex invalid! subnodeId=%d, sliptIndex=%d\n", slipt->snifferIndex, slipt->packetIdx);
            csEvtSplitNum = 0;
            curProcCounter = 0;
            return;
        }

        u8 check = check_sum(p, slipt->dataLen + 3);
        if (check != p[slipt->dataLen + 3]) {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] Subevent slipt data, check sum error! subnodeId=%d, sliptIndex=%d, %02X, %02X\n", slipt->snifferIndex, slipt->packetIdx, check, p[slipt->dataLen + 3]);
            csEvtSplitNum = 0;
            curProcCounter = 0;
            return;
        }

        csEvtSplitNum++;
        /* first packet */
        if ((slipt->packetIdx & 0x7F) == 0) {
            memset(csEvtDataBusRx, 0, CS_EVT_DATA_BUS_SIZE);
            csEvtSplitNum = 0;
            curProcCounter = 0;
        }

        if (csEvtSplitNum != (slipt->packetIdx & 0x7F)) {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] Subevent slipt data, not the expected packetIdx! subnodeId=%d, csEvtSplitNum=%02X, slipt->packetIdx=%02X\n", slipt->snifferIndex, csEvtSplitNum, slipt->packetIdx);

            csEvtSplitNum = 0;
            curProcCounter = 0;
            return;
        }

        u16 offset = SPP_SLIPT_PACKET_LEN * (slipt->packetIdx & 0x7F);
        u8  len    = slipt->dataLen - 5; /* u8(snifferIndex), u16(snifferHandle), u8(packetIdx), u8(checkSum) */
        if (len <= SPP_SLIPT_PACKET_LEN) {
            memcpy(csEvtDataBusRx + offset, slipt->pData, slipt->dataLen - 5);
            tlkapi_printf(0, "[APP][BUS] Subevent slipt data, subnodeId=%d, packetIndex=%02X, datalen=%d\r\n", slipt->snifferIndex, slipt->packetIdx, slipt->dataLen - 5);
        } else {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] Subevent slipt data, length invalid! subnodeId=%d, packetIndex=%02X, datalen=%d\r\n", slipt->snifferIndex, slipt->packetIdx, slipt->dataLen - 5);
            csEvtSplitNum = 0;
            curProcCounter = 0;
            return;
        }

        /* last packet */
        if (slipt->packetIdx & 0x80) {
            /* All packets have been correctly received. */

            if (csEvtSplitNum != (slipt->packetIdx & 0x7f)) {
                tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] Subevent reassemble data, split packet loss! subnodeId=%d, sliptIndex=%d\n", slipt->snifferIndex, slipt->packetIdx);
                csEvtSplitNum = 0;
                curProcCounter = 0;
                return;
            }

            cmd_cs_subevent_t *subevt = (cmd_cs_subevent_t *)csEvtDataBusRx;
            u8                 sum    = check_sum(csEvtDataBusRx, subevt->dataLen + 3);
            if (sum == csEvtDataBusRx[subevt->dataLen + 3]) {
                hci_le_csSubeventResultEvt_t *ptr          = (hci_le_csSubeventResultEvt_t *)(csEvtDataBusRx + 7); /* cmdId, dataLen, snifferIndex, snifferHandle */
                curProcCounter                             = ptr->Procedure_Counter | 0x80000000;
                //tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] Subevent all packets have been correctly received! curProcCnt=%08X\r\n", curProcCounter);

                /* 0x1 = Partial results with more to follow for the CS subevent */
                if (ptr->Subevent_Done_Status == 0x01) {
                    memcpy(backupSubEvtResultHeader, ptr, sizeof(backupSubEvtResultHeader));
                    backupSubEvtResult_tick = clock_time() | 1;
                }

                //tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] Subevent Rx OK: subIdx=%d, snifHandle=0x%02X, ProcCnt=0x%04X, stepNum=%d, abortReason=0x%02X\r\n", slipt->snifferIndex, slipt->snifferHandle, ptr->Procedure_Counter, ptr->Num_Steps_Reported, ptr->Abort_Reason);
                if (ptr->Abort_Reason) {
                    tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] Subevent Abort: subIdx=%d, snifHandle=0x%02X, ProcCnt=0x%04X, stepNum=%d, abortReason=0x%02X\r\n", slipt->snifferIndex, slipt->snifferHandle, ptr->Procedure_Counter, ptr->Num_Steps_Reported, ptr->Abort_Reason);
                }

                extern void app_le_cs_subevent_result_event_handle(u8 * p);
                app_le_cs_subevent_result_event_handle((u8 *)ptr);
            } else {
                tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] Subevent reassemble data, check sum error! subnodeId=%d, packetIndex=%02X, %02X, %02X\r\n", slipt->snifferIndex, slipt->packetIdx, sum, csEvtDataBusRx[subevt->dataLen + 3]);
                csEvtSplitNum = 0;
                curProcCounter = 0;
            }
        }

        DBG_SNIF_CHN6_LOW;
    }
    /* 0xD4 - CS Procedure subevent result continue. */
    else if (rx_common_cmd->cmdId == SNIFFER_CMD_CS_HCI_EVENT_PROCEDURE_SUBEVENT_RESULT_CONTINUE) {
        DBG_SNIF_CHN8_HIGH;
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
        u8                   *p     = (u8 *)rx_common_cmd;
        cmd_cs_event_slipt_t *slipt = (cmd_cs_event_slipt_t *)rx_common_cmd;

        if (slipt->snifferIndex >= CHECK_SNIFFER_INDEX_MAX) {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] Subevent continue split data, snifferIndex invalid! subnodeId=%d, sliptIndex=%d\n", slipt->snifferIndex, slipt->packetIdx);
            return;
        }

        if ((curProcCounter & 0x80000000) == 0) {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] Subevent continue split data, subevent result lost!"
                             "subnodeId=%d, packetIndex=%02X, curProcCnt=%d\r\n",
                          slipt->snifferIndex,
                          slipt->packetIdx,
                          curProcCounter);
            csEvtSplitNum = 0;
            curProcCounter = 0;
            return;
        }

        u8 check = check_sum(p, slipt->dataLen + 3);
        if (check != p[slipt->dataLen + 3]) {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] Subevent continue split data, data checkSum error! subnodeId=%d, sliptIndex=%d, %02X, %02X\n", slipt->snifferIndex, slipt->packetIdx, check, p[slipt->dataLen + 3]);
            csEvtSplitNum = 0;
            curProcCounter = 0;
            return;
        }

        csEvtSplitNum++;
        /* first packet */
        if ((slipt->packetIdx & 0x7F) == 0) {
            memset(csEvtDataBusRx, 0, CS_EVT_DATA_BUS_SIZE);
            csEvtSplitNum = 0;
        }

        if (csEvtSplitNum != (slipt->packetIdx & 0x7F)) {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] Subevent continue split data, not the expected packetIdx! subnodeId=%d, csEvtSplitNum=%02X, slipt->packetIdx=%02X\n", slipt->snifferIndex, csEvtSplitNum, slipt->packetIdx);
            csEvtSplitNum = 0;
            curProcCounter = 0;
            return;
        }

        u16 offset = SPP_SLIPT_PACKET_LEN * (slipt->packetIdx & 0x7F);
        u8  len    = slipt->dataLen - 5;
        if (len <= SPP_SLIPT_PACKET_LEN) {
            memcpy(csEvtDataBusRx + offset, slipt->pData, slipt->dataLen - 5);
            tlkapi_printf(0, "[APP][BUS] Subevent continue slipt data, subnodeId=%d, packetIndex=%02X, datalen=%d\r\n", slipt->snifferIndex, slipt->packetIdx, slipt->dataLen - 5);
        } else {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] Subevent continue slipt data, length invalid! subnodeId=%d, packetIndex=%02X, datalen=%d\r\n", slipt->snifferIndex, slipt->packetIdx, slipt->dataLen - 5);
            curProcCounter = 0;
            csEvtSplitNum = 0;
            return;
        }

        /* last packet */
        if (slipt->packetIdx & 0x80) {
            /* All packets have been correctly received. */

            if (csEvtSplitNum != (slipt->packetIdx & 0x7f)) {
                tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] Subevent continue reassemble data, split packet loss! "
                                 "subnodeId=%d, sliptIndex=%d\n",
                              slipt->snifferIndex,
                              slipt->packetIdx);
                csEvtSplitNum = 0;
                curProcCounter = 0;
                return;
            }

            cmd_cs_subevent_t *subevt = (cmd_cs_subevent_t *)csEvtDataBusRx;
            u8                 sum    = check_sum(csEvtDataBusRx, subevt->dataLen + 3);
            if (sum == csEvtDataBusRx[subevt->dataLen + 3]) {
                //tlkapi_printf(1, "[APP][BUS] Subevent continue all packets have been correctly received.\r\n");
                hci_le_csSubeventResultContinueEvt_t *ptr = (hci_le_csSubeventResultContinueEvt_t *)(csEvtDataBusRx + 7);
                //if((curProcCounter & 0x0000FFFF) == ptr->Procedure_Counter){
                if (1) {
                    //tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] SubeventConti Rx OK: subIdx=%d, snifHandle=0x%02X, stepNum=%d, abortReason=0x%02X\r\n", slipt->snifferIndex, slipt->snifferHandle, ptr->Num_Steps_Reported, ptr->Abort_Reason);
                    if (ptr->Abort_Reason) {
                        tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] SubeventConti Abort: ProcCnt=0x%04X, subIdx=%d, snifHandle=0x%02X, stepNum=%d, abortReason=0x%02X\r\n", curProcCounter, slipt->snifferIndex, slipt->snifferHandle, ptr->Num_Steps_Reported, ptr->Abort_Reason);
                    }

                    if (backupSubEvtResult_tick) {
                        backupSubEvtResult_tick = 0;
                    }

                    extern void app_le_cs_subevent_result_continue_event_handle(u8 * p);
                    app_le_cs_subevent_result_continue_event_handle((u8 *)ptr);
                } else {
                    //tlkapi_printf(1, "[APP][BUS] Subevent continue reassemble data, Procedure_Counter does not match! subnodeId=%d, sliptIndex=%d, ptr->Procedure_Counter=%08X, curProcCnt=%08X\n",
                    //                 slipt->snifferIndex, slipt->packetIdx, ptr->Procedure_Counter, curProcCnt);
                    //curProcCounter = 0;
                }
            } else {
                tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] Subevent continue reassemble data, check sum error, subnodeId=%d, packetIndex=%02X, %02X, %02X\r\n", slipt->snifferIndex, slipt->packetIdx, sum, csEvtDataBusRx[subevt->dataLen + 3]);
                curProcCounter = 0;
            }
        }

        DBG_SNIF_CHN8_LOW;
    }
    else if (rx_common_cmd->cmdId == SNIFFER_CMD_CS_DISTANCE) {
        u8 *p                                          = (u8 *)rx_common_cmd;
        spp_main_node_cmd_cs_distance_rx_t *common_cmd = (spp_main_node_cmd_cs_distance_rx_t *)rx_common_cmd;

        if (common_cmd->snifferIndex >= CHECK_SNIFFER_INDEX_MAX) {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] CMD_CS_DISTANCE, snifferIndex invalid! subnodeId=%d\n", common_cmd->snifferIndex);
            return;
        }

        u8 check = check_sum(p, common_cmd->dataLen + 3);
        if (check != p[common_cmd->dataLen + 3]) {
            tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] CMD_CS_DISTANCE, data checkSum error! subnodeId=%d, %02X, %02X\n", common_cmd->snifferIndex, check, p[common_cmd->dataLen + 3]);
            return;
        }

        /* sub node current only support ALG2 */
        snif_main_node_cs_distacne_process(common_cmd->snifferHandle, common_cmd->rangingCounter, 0.0, common_cmd->distance, 0.0);
    }
    DBG_SNIF_CHN10_LOW;
}

void snif_main_node_tx_data_process(void)
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

    if (spp_tx_fifo.wptr != spp_tx_fifo.rptr) {
        DBG_SNIF_CHN4_HIGH;
        spp_main_node_cmd_tx_t *tx_common_cmd = (spp_main_node_cmd_tx_t *)(spp_tx_fifo.p + spp_tx_fifo.rptr * spp_tx_fifo.size);

    #if (APP_TRANSPORT_CANFD_ENABLE)
        u8  checkSum = 0;
        u8  checklen = tx_common_cmd->dataLen + 3; // exclude dmaLen and checksum, 3 = u16(cmdId) + u16(dataLen) - u8(checksum)
        u8  k;
        u8 *ptx = (u8 *)(tx_common_cmd);
        for (k = 0; k < checklen; k++) {
            checkSum += *(ptx + k + 4); // skip dmaLen
        }
        *(ptx + k + 4) = checkSum;

        u8 tx_status = 0;
        u8 res = canfd_send_data_handle(MAIN_NODE_TO_SUB_NODE_SYNC_SID, (u8 *)&tx_common_cmd->cmdId, tx_common_cmd->dmaLen);
        if (res == 0) {
            spp_tx_fifo.rptr == (spp_tx_fifo.num - 1) ? spp_tx_fifo.rptr = 0 : spp_tx_fifo.rptr++;
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
                    spp_tx_fifo.rptr == (spp_tx_fifo.num - 1) ? spp_tx_fifo.rptr = 0 : spp_tx_fifo.rptr++;
                }
                break;
            }
        }
        if (clock_time_exceed(uart_tx_start_tick, uart_transmit_max_time)) {
            //reach send timeout
            uart_dma_send_done_flag = 1;
        }
    #endif

        DBG_SNIF_CHN4_LOW;
    }
}

void snif_main_node_control_process(void)
{
    #if (APP_TRANSPORT_CANFD_ENABLE)
        #if (APP_CAN_PM_ENABLE)
    if ((gpio_read(TCAN4550_GPIO_WKREQ_N) == 0) && (blc_ll_getCurrentConnectionNumber() == 0)) {
        if (can_sleep_pending_tick == 0) {
            can_sleep_pending_tick = clock_time() | 1;
        } else {
            if (clock_time_exceed(can_sleep_pending_tick, 10 * 1000 * 1000)) {
                u8 res = tcan4550_enter_sleep();
                //tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] TCAN4550 enter sleep mode, %d\r\n", res);
            }
        }
    }
        #endif

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

            if (!nodeSetting.subNodeNumber) {
                update_flag = 0;
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
                    while (canfd_send_data_handle(MAIN_NODE_TO_SUB_NODE_SYNC_SID, (u8 *)&spp_common_cmd->cmdId, spp_common_cmd->dataLen + 4) != 0) {
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

    #if (APP_TRANSPORT_CANFD_ENABLE)
    snif_main_node_tx_data_process();
    #endif

    /* Ensure the integrity of SubEvtResult and SubEvtResultContinue data of sub nodes */
    if ((backupSubEvtResult_tick) && clock_time_exceed(backupSubEvtResult_tick, 50 * 1000)) {
        hci_le_csSubeventResultEvt_t         *pEvt = (hci_le_csSubeventResultEvt_t *)backupSubEvtResultHeader;
        u8                                    abortSubEvtResultContinue[9];
        hci_le_csSubeventResultContinueEvt_t *pEvtConti = (hci_le_csSubeventResultContinueEvt_t *)abortSubEvtResultContinue;

        pEvtConti->Subevent_Code     = pEvt->Subevent_Code;
        pEvtConti->Connection_Handle = pEvt->Connection_Handle;
        pEvtConti->Config_ID         = pEvt->Config_ID;

        pEvtConti->Procedure_Done_Status = pEvt->Procedure_Done_Status;
        pEvtConti->Subevent_Done_Status  = 0x0F;                               //0xF = Current CS Subevent aborted
        pEvtConti->Abort_Reason          = 0xF0 | (pEvt->Abort_Reason & 0x0F); //0xF0 = Subevent Abort because of unspecified reasons

        pEvtConti->Num_Antenna_Paths  = pEvt->Num_Antenna_Paths;
        pEvtConti->Num_Steps_Reported = 0;

        tlkapi_printf(APP_CS_LOG_EN, "[APP][BUS] SubEvtResultContinue timeout Abort, Set Subevent Abort: %x, %x\r\n", pEvtConti->Procedure_Done_Status, pEvtConti->Subevent_Done_Status);

        extern void app_le_cs_subevent_result_continue_event_handle(u8 * p);
        app_le_cs_subevent_result_continue_event_handle((u8 *)pEvtConti);

        backupSubEvtResult_tick = 0;

        if (curProcCounter) {
            curProcCounter = 0;
        }
    }

    if ((app_cs_distance_curConnHandle) && (clock_time_exceed(app_cs_distance_update_tick, APP_CS_DISTANCE_TIMEOUT_SECONDS * 1000 * 1000))) {
        tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] 0x%x cs_distance_update_tick not update more than %d second, will reboot\r\n", app_cs_distance_curConnHandle, APP_CS_DISTANCE_TIMEOUT_SECONDS);
        app_start_reboot();
    }

    #if (APP_TRANSPORT_CANFD_ENABLE)
        if (1 == receive_bus_rssi_flag) {
            static u32 report_rssi_tick = 0;
            if (clock_time_exceed(report_rssi_tick, (nodeSetting.reportIntvl - 5) * 1000)) {
                DBG_SNIF_CHN3_HIGH;
                report_rssi_tick = clock_time();
                receive_bus_rssi_flag = 0;

                #if (0)
                    if (blc_ll_getCurrentMasterRoleNumber()) {
                        //peer-peripheral RSSI and cs_distance
                        for (u8 i = 0; i < ACL_CENTRAL_MAX_NUM; i++) {
                            canfd_send_data_handle(nodeSetting.reportId, (u8 *)rssi_buf_centralRole[i], sizeof(rssi_buf_centralRole[0]));
                        }
                    }

                    if (blc_ll_getCurrentSlaveRoleNumber()) {
                        //peer-central RSSI
                        canfd_send_data_handle(nodeSetting.reportId, (u8 *)rssi_buf_peripheralRole, sizeof(rssi_buf_peripheralRole));
                    }
                #endif

                DBG_SNIF_CHN3_LOW;
            }
        }
    #endif

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
        DBG_SNIF_CHN2_HIGH;
        uart_dma_send_done_flag = 1;

        /* after txdone 5us, the interrupt occurred*/
        //gpio_toggle(GPIO_LED_GREEN);
        uart_clr_irq_status(UART_MODULE_SEL, UART_TXDONE_IRQ_STATUS);
        DBG_SNIF_CHN2_LOW;
    }

    if (uart_get_irq_status(UART_MODULE_SEL, UART_RXDONE_IRQ_STATUS)) {
        DBG_SNIF_CHN3_HIGH;
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
        DBG_SNIF_CHN3_LOW;
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
    if (spp_tx_fifo.wptr == spp_tx_fifo.rptr) {
        return -1;
    } else {
        snif_main_node_tx_data_process();
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
    u8 subNodeNum_min = min(CHECK_SNIFFER_INDEX_MAX, LOCAL_SNIFFER_NUM_MAX);
    if (nodeSetting.subNodeNumber > subNodeNum_min) {
        #if (APP_CS_THREE_POINT_POSITIONING_TAG_EN)
            nodeSetting.subNodeNumber = 2;
        #else
            nodeSetting.subNodeNumber = 0;
        #endif
    }

    if (nodeSetting.rssiCombFlag == 0xFF) {
        nodeSetting.rssiCombFlag = 0;
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

    tlkapi_printf(APP_SNIF_LOG_EN, "[APP][INI] main node setting from flash address 0x%06X, regAddr 0x%X\n", FLASH_ADDRESS_SNIFFER_SETTING, (void *)&nodeSetting);
    tlkapi_printf(APP_SNIF_LOG_EN, " ------------------------------\n");
    tlkapi_printf(APP_SNIF_LOG_EN, " param              value  unit\n");
    tlkapi_printf(APP_SNIF_LOG_EN, " ------------------------------\n");
    tlkapi_printf(APP_SNIF_LOG_EN, "  main node\n");
    tlkapi_printf(APP_SNIF_LOG_EN, "    subNodeNum       %04d\n", nodeSetting.subNodeNumber);
    tlkapi_printf(APP_SNIF_LOG_EN, "    rssiCombFlag     %04d\n", nodeSetting.rssiCombFlag);
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
        #if (APP_CS_THREE_POINT_POSITIONING_TAG_EN)
            appCsParamSetting.procedureInterval = 23;
        #else
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
        #endif
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
        #if (APP_CS_THREE_POINT_POSITIONING_TAG_EN)
            //Channel number 60
            appCsParamSetting.channelMap[0] = 0x00;
            appCsParamSetting.channelMap[1] = 0xC0;
            appCsParamSetting.channelMap[2] = 0x7F;
            appCsParamSetting.channelMap[3] = 0xFC;
            appCsParamSetting.channelMap[4] = 0xFF;
            appCsParamSetting.channelMap[5] = 0xFF;
            appCsParamSetting.channelMap[6] = 0xFF;
            appCsParamSetting.channelMap[7] = 0xFF;
            appCsParamSetting.channelMap[8] = 0xFF;
            appCsParamSetting.channelMap[9] = 0x1F;
        #else
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
        #endif
    }

    /* for CS distance offset, unit: 10cm, Range: -500 to 500 */
    if (abs(appCsParamSetting.distanceOffset <= 500) && (appCsParamSetting.distanceOffset != -1)) {
        app_cs_distance_offset = appCsParamSetting.distanceOffset / 100.0f;
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

    /* for select which node to print CS HCI summary information, Bit(0) is main node, Bit(1) is sub node 0, Bit(2) is sub node 1 ... */
    if (appCsParamSetting.printCsHciInforMask >= BIT(CHECK_SNIFFER_INDEX_MAX+1)) {
        appCsParamSetting.printCsHciInforMask = 0;
    }

    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] Param Setting from flash address 0x%06X, regAddr 0x%X\n", FLASH_ADDRESS_CS_PARAM_SETTING, (void *)&appCsParamSetting);
    tlkapi_printf(APP_CS_LOG_EN, "  procedureInterval:%d\n", appCsParamSetting.procedureInterval);
    tlkapi_printf(APP_CS_LOG_EN, "  subeventLen:%d us\n", appCsParamSetting.subeventLen);
    tlkapi_printf(APP_CS_LOG_EN, "  rangingAlgMode:0x%02X\n", appCsParamSetting.rangingAlgMode);
    tlkapi_printf(APP_CS_LOG_EN, "  kalmanNoiseCov:%d unit:0.0001\n", appCsParamSetting.kalmanNoiseCov);
    tlkapi_send_string_data(APP_CS_LOG_EN, "  channelMap", appCsParamSetting.channelMap, 10);
    tlkapi_printf(APP_CS_LOG_EN, "  distanceOffset:%.1f m\n", app_cs_distance_offset);
    tlkapi_printf(APP_CS_LOG_EN, "  aclRfPowerIdx:%d dBm\n", appCsParamSetting.aclRfPowerIdx);
    tlkapi_printf(APP_CS_LOG_EN, "  csRfPowerLevel:%d dBm\n", appCsParamSetting.csRfPowerLevel);
    tlkapi_printf(APP_CS_LOG_EN, "  printCsHciInforMask:0x%02X\n", appCsParamSetting.printCsHciInforMask);
}

/**
 * @brief      sniffer main node initialization
 * @param[in]  none
 * @return     none.
 */
void snif_main_node_init(void)
{
    snif_load_setting_from_flash();
    cs_load_setting_from_flash();

    //////////// UART/CAN Initialization  Begin /////////////////////////
    #if (APP_TRANSPORT_UART_ENABLE)
        user_uart_init();
        blc_register_hci_handler(rx_from_uart_cb, tx_to_uart_cb); //customized uart handler
        extern void blc_ll_register_user_irq_handler_cb(user_irq_handler_cb_t cb);
        blc_ll_register_user_irq_handler_cb(uart0_irq_handler);
    #elif (APP_TRANSPORT_CANFD_ENABLE)
        tcan4550_init();
        memset((u8 *)rssi_buf_peripheralRole, 0xFF, sizeof(rssi_buf_peripheralRole));
        memset((u8 *)rssi_buf_centralRole, 0xFF, sizeof(rssi_buf_centralRole));
    #endif
    //////////// UART/CAN Initialization  End /////////////////////////

    //////////// Sniffer Initialization  Begin /////////////////////////
    blc_ll_initCsSnifferMainNode_module(nodeSetting.subNodeNumber + 1);

    blc_ll_setAclMasterConnParamUpdateRspLatency(0);

    blc_ll_registerTelinkControllerEventCallback(BLT_EV_FLAG_CHANNEL_MAP_UPDATE, &user_channel_map_update);
    blc_ll_registerTelinkControllerEventCallback(BLT_EV_FLAG_ACL_EVERY_CONN_EVENT, &user_acl_every_connection_event);
    //////////// Sniffer Initialization  End /////////////////////////

    //////////// APP CS Sniffer Initialization  Begin /////////////////////////
    #if (KALMAN_FILTER_ENABLE)
        //Initialize Kalman distance filter
        snif_kalman_Filter_init();
    #endif

    //Enable distance calculate algorithm, see blc_ranging_algorithm_enum.
    blc_cs_enableAlgoMask(appCsParamSetting.rangingAlgMode);
    //////////// APP CS Sniffer Initialization  End /////////////////////////

    #if (APP_CS_THREE_POINT_POSITIONING_TAG_EN)
        tlkapi_send_string_data(APP_SNIF_LOG_EN, "[APP][SNIF] APP_CS_THREE_POINT_POSITIONING_TAG_EN=1", 0, 0);
    #endif
    tlkapi_printf(APP_SNIF_LOG_EN, "[APP][SNIF] snif_main_node_init:M%dS%d,BandRate_%d\n", ACL_CENTRAL_MAX_NUM, ACL_PERIPHR_MAX_NUM, UART_BAUD_RATE);
    tlkapi_printf(APP_SNIF_LOG_EN, "    Build time: %s %s\r\n", __DATE__, __TIME__);
}

#endif /* MAIN_NODE_ROLE_SELECT == MAIN_NODE_CS_PERIPHERAL_CENTRAL */
