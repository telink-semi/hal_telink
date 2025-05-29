/********************************************************************************************************
 * @file    app.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "app.h"
#include "app_buffer.h"
#include "app_ui.h"
#include "../default_att.h"


#if (INTER_TEST_MODE == TEST_CS_SUBEVENT)


    #define CS_SUBEVENT_TEST_NULL      0
    #define CS_SUBEVENT_TEST_INITIATOR 1
    #define CS_SUBEVENT_TEST_REFLECTOR 2
    #define CS_SUBEVENT_TEST_ONLY_M0S1 8
    #define CS_SUBEVENT_TEST_ONLY_M1S0 4

    #define FLASH_ADR_CS_TEST_MODE     0xF1000
    #define FLASH_ADR_CS_STEP_MODE     0xF1010 //0x10~2F, 32 step
    #define FLASH_ADR_CS_CHANNEL_DIFF  0xF1030


_attribute_ble_data_retention_ volatile u8 cs_subevent_test_mode = CS_SUBEVENT_TEST_NULL;


_attribute_ble_data_retention_ int central_smp_pending = 0; // SMP: security & encryption;


_attribute_ble_data_retention_ volatile int ble_btxbrx_pre_state = 0;

enum
{
    CS_STEP_NULL   = 0,
    CS_STEP_MODE_0 = 1,
    CS_STEP_MODE_1 = 2,
    CS_STEP_MODE_2 = 3,
    CS_STEP_MODE_3 = 4,
};

    #define CS_TEST_STEP_MODE_ARRAY_NUM 32
//_attribute_ble_data_retention_        volatile u8 step_mode_array[CS_TEST_STEP_MODE_ARRAY_NUM] = {CS_STEP_MODE_2, CS_STEP_NULL};
//_attribute_ble_data_retention_        volatile u8 step_mode_array[CS_TEST_STEP_MODE_ARRAY_NUM] = {CS_STEP_MODE_0, CS_STEP_MODE_2, CS_STEP_NULL};
_attribute_ble_data_retention_ volatile u8 step_mode_array[CS_TEST_STEP_MODE_ARRAY_NUM] = {CS_STEP_MODE_0, CS_STEP_MODE_2, CS_STEP_MODE_2, CS_STEP_MODE_2, CS_STEP_MODE_2, CS_STEP_NULL};
//_attribute_ble_data_retention_        volatile u8 step_mode_array[CS_TEST_STEP_MODE_ARRAY_NUM] = {CS_STEP_MODE_0, CS_STEP_MODE_1, CS_STEP_NULL};
//_attribute_ble_data_retention_        volatile u8 step_mode_array[CS_TEST_STEP_MODE_ARRAY_NUM] = {CS_STEP_MODE_0, CS_STEP_MODE_1, CS_STEP_MODE_1, CS_STEP_MODE_2, CS_STEP_MODE_1, CS_STEP_NULL};
//_attribute_ble_data_retention_        volatile u8 step_mode_array[CS_TEST_STEP_MODE_ARRAY_NUM] = {CS_STEP_MODE_0, CS_STEP_MODE_1, CS_STEP_MODE_1, CS_STEP_MODE_1, CS_STEP_MODE_1, CS_STEP_NULL};

_attribute_ble_data_retention_ volatile u8 test_T_FCS_us = 150; //T_FCS = {15 µs, 20 µs, 30 µs, 40 µs, 50 µs, 60 µs, 80 µs, 100 µs, 120 µs, 150 µs}
_attribute_ble_data_retention_ volatile u8 test_T_IP1_us = 145; //T_IP1 = {10 µs, 20 µs, 30 µs, 40 µs, 50 µs, 60 µs, 80 µs, 145 µs}
_attribute_ble_data_retention_ volatile u8 test_T_IP2_us = 145; //T_IP2 = {10 µs, 20 µs, 30 µs, 40 µs, 50 µs, 60 µs, 80 µs, 145 µs}
_attribute_ble_data_retention_ volatile u8 test_T_PM_us  = 40;  //T_PM = {10 µs, 20 µs, 40 µs}
_attribute_ble_data_retention_ volatile u8 test_CS_DRBG  = 0;   //CS_DRBG = {0, BIT(1), BIT(0), BIT(1)|BIT(0)}//The lowest two bits, The higher bit BIT(1) corresponds to whether the previous tone extension exists, and the lower bit BIT(0) corresponds to whether the latter tone extension exists
//_attribute_ble_data_retention_        volatile u8 test_CS_DRBG = BIT(1)|BIT(0);

    #define CS_RTT_AA_MAX_NUM 255
_attribute_ble_data_retention_ volatile s16 step_mode_ToA_ToD[CS_RTT_AA_MAX_NUM] = {0};
_attribute_ble_data_retention_ volatile u8  step_mode_channel[CS_RTT_AA_MAX_NUM] = {0};
_attribute_ble_data_retention_ volatile int cur_step_mode_1_CollectDataNum       = 0;

_attribute_ble_data_retention_ volatile u8 cs_test_rx_update_flag                                                 = 1;
_attribute_ble_data_retention_ u8          cs_test_rx_buff[DMA_CS_RFRX_MAX_DMA_LEN * CS_TEST_STEP_MODE_ARRAY_NUM] = {0};

_attribute_ble_data_retention_ volatile u8 cs_channel_diff = 2;
    #define CS_CHANNEL_MAP_ENABLE_FLAG 0xEE
//from 26 ~ 88, diff = 2
_attribute_ble_data_retention_ volatile u8 cs_channel_map[CS_TEST_STEP_MODE_ARRAY_NUM] =
    {26, 28, 30, 32, 34, 40, 60, 48, 62, 56, 38, 64, 44, 36, 54, 42, 58, 50, 46, 52, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88};
//from 0 ~ 31
_attribute_ble_data_retention_ volatile u8 cs_channel_idx[CS_TEST_STEP_MODE_ARRAY_NUM] =
    {0, 1, 2, 3, 4, 7, 17, 11, 18, 15, 6, 19, 9, 5, 14, 8, 16, 12, 10, 13, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};

_attribute_ble_data_retention_ volatile u8 cs_subevent_end_flag                       = 0;
_attribute_ble_data_retention_ volatile u8 cs_subevent_mode_type                      = 0;
_attribute_ble_data_retention_ volatile u8 cs_subevent_non_mode_0_step_num            = 0;
_attribute_ble_data_retention_ volatile u8 cs_subevent_non_mode_0_initiator_num       = 0;
_attribute_ble_data_retention_ volatile u8 cs_subevent_non_mode_0_reflector_num       = 0;
_attribute_ble_data_retention_ volatile u8 cs_subevent_non_mode_0_initiator_data_flag = 0;
_attribute_ble_data_retention_ volatile u8 cs_subevent_non_mode_0_reflector_data_flag = 0;

_attribute_ble_data_retention_ volatile int initi_pct[CS_TEST_STEP_MODE_ARRAY_NUM * 2] = {0};
_attribute_ble_data_retention_ volatile int refle_pct[CS_TEST_STEP_MODE_ARRAY_NUM * 2] = {0};
_attribute_ble_data_retention_ volatile u8  initi_tone_QI[CS_TEST_STEP_MODE_ARRAY_NUM] = {0};
_attribute_ble_data_retention_ volatile u8  refle_tone_QI[CS_TEST_STEP_MODE_ARRAY_NUM] = {0};

/**
 * @brief       Channel Sounding Subevent test.
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void cs_subevent_test(void)
{
    if (ble_btxbrx_pre_state && !blm_btxbrx_state) {
        st_ll_conn_t *pc_conn = (st_ll_conn_t *)&blms[blms_conn_sel];

        if (bltRxPkt.rx_header_tick && (aclConn_param.conn_rx_num == 1) && (pc_conn->conn_inst > 30) && ((pc_conn->conn_inst & 0x03) == 0x02)) {
            if (cs_subevent_test_mode == CS_SUBEVENT_TEST_INITIATOR) {
                if (blms_conn_sel == 4) {
                    //                  st_lls_conn_t *ps_conn =  (st_lls_conn_t *)&blmsSlave[bls_conn_sel];

                    u32 tick_cs_subevent_start = bltRxPkt.rx_timeStamp + SYSTEM_TIMER_TICK_1MS;
                    cs_mode0_rx_flag           = 0;

                    while (!tick1_exceed_tick2(clock_time(), tick_cs_subevent_start))
                        ;

                    cs_subevent_end_flag = 0;
                    blt_cs_subevent_rf_init();

                    u32 tick_step_start     = tick_cs_subevent_start;
                    cs_procedure_start_tick = tick_cs_subevent_start;

                    u8 csChannel = pc_conn->conn_chn;

                    cs_subevent_non_mode_0_step_num = 0;

                    u8 i = 0;
                    while (step_mode_array[i]) {
                        if (cs_channel_diff == CS_CHANNEL_MAP_ENABLE_FLAG) {
                            csChannel = cs_channel_map[i];
                        }

                        test_CS_DRBG = i & 0x03;

                        if (step_mode_array[i] == CS_STEP_MODE_0) {
                            DBG_CHN8_HIGH;
                            tick_step_start = ble_cs_initiator_mode0_test(tick_step_start, pc_conn->aclAccessAddr, csChannel, bltPHYs.cur_llPhy, test_T_FCS_us, test_T_IP1_us);
                            DBG_CHN8_LOW;
                        } else if (step_mode_array[i] == CS_STEP_MODE_1) {
                            DBG_CHN8_HIGH;
                            tick_step_start = ble_cs_initiator_mode1_test(tick_step_start, pc_conn->aclAccessAddr, csChannel, bltPHYs.cur_llPhy, test_T_FCS_us, test_T_IP1_us);
                            DBG_CHN8_LOW;
                            cs_subevent_non_mode_0_step_num++;
                        } else if (step_mode_array[i] == CS_STEP_MODE_2) {
                            DBG_CHN8_HIGH;
                            tick_step_start = ble_cs_initiator_mode2_test(tick_step_start, pc_conn->aclAccessAddr, csChannel, bltPHYs.cur_llPhy, test_T_FCS_us, test_T_PM_us, test_T_IP2_us, test_CS_DRBG);
                            DBG_CHN8_LOW;
                            cs_subevent_non_mode_0_step_num++;
                        }
                        //                      else if(step_mode_array[i] == CS_STEP_MODE_3){
                        //
                        //                      }

                        if (cs_channel_diff != CS_CHANNEL_MAP_ENABLE_FLAG) {
                            csChannel += cs_channel_diff;
                        }

                        i++;
                    }

                    if ((pc_conn->conn_inst) == 34 && cs_test_rx_update_flag) {
                        cs_test_rx_update_flag = 0;
                        u8 step_run_num        = i;
                        if (step_run_num) {
                            smemcpy(cs_test_rx_buff, cs_rx_buff - DMA_CS_RFRX_MAX_DMA_LEN * step_run_num, DMA_CS_RFRX_MAX_DMA_LEN * step_run_num);
                        }
                    }

                    blt_cs_subevent_rf_deinit(0);

                    cs_subevent_end_flag                 = 1;
                    cs_subevent_non_mode_0_initiator_num = 0;
                }
            } else if (cs_subevent_test_mode == CS_SUBEVENT_TEST_REFLECTOR) {
                if (blms_conn_sel == 0) {
                    u32 tick_cs_subevent_start = pc_conn->ap_tick_mark + IRQ_BTX_DELAY_US + SYSTEM_TIMER_TICK_1MS + SCHE_NEW_TASK_MARGIN_US * SYSTEM_TIMER_TICK_1US;
                    cs_mode0_rx_flag           = 0;

                    while (!tick1_exceed_tick2(clock_time(), tick_cs_subevent_start))
                        ;

                    cs_subevent_end_flag = 0;
                    blt_cs_subevent_rf_init();

                    u32 tick_step_start     = tick_cs_subevent_start;
                    cs_procedure_start_tick = tick_cs_subevent_start;

                    u8 csChannel = pc_conn->conn_chn;

                    cs_subevent_non_mode_0_step_num = 0;

                    u8 i = 0;
                    while (step_mode_array[i]) {
                        if (cs_channel_diff == CS_CHANNEL_MAP_ENABLE_FLAG) {
                            csChannel = cs_channel_map[i];
                        }

                        test_CS_DRBG = i & 0x03;

                        if (step_mode_array[i] == CS_STEP_MODE_0) {
                            DBG_CHN4_HIGH;
                            tick_step_start = ble_cs_reflector_mode0_test(tick_step_start, pc_conn->aclAccessAddr, csChannel, bltPHYs.cur_llPhy, test_T_FCS_us, test_T_IP1_us);
                            DBG_CHN4_LOW;
                        } else if (step_mode_array[i] == CS_STEP_MODE_1) {
                            DBG_CHN4_HIGH;
                            tick_step_start = ble_cs_reflector_mode1_test(tick_step_start, pc_conn->aclAccessAddr, csChannel, bltPHYs.cur_llPhy, test_T_FCS_us, test_T_IP1_us);
                            DBG_CHN4_LOW;
                            cs_subevent_non_mode_0_step_num++;
                        } else if (step_mode_array[i] == CS_STEP_MODE_2) {
                            DBG_CHN4_HIGH;
                            tick_step_start = ble_cs_reflector_mode2_test(tick_step_start, pc_conn->aclAccessAddr, csChannel, bltPHYs.cur_llPhy, test_T_FCS_us, test_T_PM_us, test_T_IP2_us, test_CS_DRBG);
                            DBG_CHN4_LOW;
                            cs_subevent_non_mode_0_step_num++;
                        }
                        //                      else if(step_mode_array[i] == CS_STEP_MODE_3){
                        //
                        //                      }

                        if (cs_channel_diff != CS_CHANNEL_MAP_ENABLE_FLAG) {
                            csChannel += cs_channel_diff;
                        }

                        i++;
                    }

                    if ((pc_conn->conn_inst) == 34 && cs_test_rx_update_flag) {
                        cs_test_rx_update_flag = 0;
                        u8 step_run_num        = i;
                        if (step_run_num) {
                            smemcpy(cs_test_rx_buff, cs_rx_buff - DMA_CS_RFRX_MAX_DMA_LEN * step_run_num, DMA_CS_RFRX_MAX_DMA_LEN * step_run_num);
                        }
                    }

                    blt_cs_subevent_rf_deinit(0);

                    cs_subevent_end_flag                 = 1;
                    cs_subevent_non_mode_0_reflector_num = 0;
                }
            }
        }
    }

    ble_btxbrx_pre_state = blm_btxbrx_state;
}

    /**
 * @brief       Channel Sounding Subevent test by gpio trigger.
 * @param[in]   none
 * @return      none
 */
    #if (GPIO_TRIGGER_TEST_ENABLE)
_attribute_ram_code_ void gpio_trigger_cs_subevent_test(void)
{
    if (cs_subevent_test_mode == CS_SUBEVENT_TEST_INITIATOR) {
        u32 tick_cs_subevent_start = clock_time();

        blt_cs_subevent_rf_init();

        u32 tick_step_start     = tick_cs_subevent_start;
        cs_procedure_start_tick = tick_cs_subevent_start;

        u8 csChannel = pc_conn->conn_chn;

        u8 i = 0;
        while (step_mode_array[i]) {
            if (step_mode_array[i] == CS_STEP_MODE_0) {
                DBG_CHN8_HIGH;
                tick_step_start = ble_cs_initiator_mode0_test(tick_step_start, pc_conn->aclAccessAddr, csChannel, bltPHYs.cur_llPhy, test_T_FCS_us, test_T_IP1_us);
                DBG_CHN8_LOW;
            }
            //      else if(step_mode_array[i] == CS_STEP_MODE_1){
            //
            //      }
            else if (step_mode_array[i] == CS_STEP_MODE_2) {
                DBG_CHN8_HIGH;
                tick_step_start = ble_cs_initiator_mode2_test(tick_step_start, pc_conn->aclAccessAddr, csChannel, bltPHYs.cur_llPhy, test_T_FCS_us, test_T_PM_us, test_T_IP2_us, test_CS_DRBG);
                DBG_CHN8_LOW;
            }
            //      else if(step_mode_array[i] == CS_STEP_MODE_3){
            //
            //      }

            csChannel += 2;
            i++;
        }

        if (pc_conn->conn_inst == 32) {
            u32 rx_dma_len = 0;
            BYTE_TO_UINT16(rx_dma_len, cs_rx_buff - DMA_CS_RFRX_MAX_DMA_LEN);
            if (rx_dma_len <= DMA_CS_RFRX_MAX_DMA_LEN - 4) {
                smemcpy(cs_test_rx_buff, cs_rx_buff - DMA_CS_RFRX_MAX_DMA_LEN, rx_dma_len + 4);
            }
        }

        blt_cs_subevent_rf_deinit(0);
    } else if (cs_subevent_test_mode == CS_SUBEVENT_TEST_REFLECTOR) {
        u32 tick_cs_subevent_start = clock_time();

        blt_cs_subevent_rf_init();

        u32 tick_step_start     = tick_cs_subevent_start;
        cs_procedure_start_tick = tick_cs_subevent_start;

        u8  csChannel  = 2; //2404
        u32 accessAddr = 0xf8118ac9;

        u8 i = 0;
        while (step_mode_array[i]) {
            if (step_mode_array[i] == CS_STEP_MODE_0) {
                DBG_CHN4_HIGH;
                tick_step_start = ble_cs_reflector_mode0_test(tick_step_start, accessAddr, csChannel, bltPHYs.cur_llPhy, test_T_FCS_us, test_T_IP1_us);
                DBG_CHN4_LOW;
            }
            //      else if(step_mode_array[i] == CS_STEP_MODE_1){
            //
            //      }
            else if (step_mode_array[i] == CS_STEP_MODE_2) {
                DBG_CHN4_HIGH;
                tick_step_start = ble_cs_reflector_mode2_test(tick_step_start, accessAddr, csChannel, bltPHYs.cur_llPhy, test_T_FCS_us, test_T_PM_us, test_T_IP2_us, test_CS_DRBG);
                DBG_CHN4_LOW;
            }
            //      else if(step_mode_array[i] == CS_STEP_MODE_3){
            //
            //      }

            //      csChannel += 2;
            i++;
        }

        blt_cs_subevent_rf_deinit(0);
    }
}
    #endif

/**
 * @brief   BLE Advertising data
 */
const u8 tbl_advData[] = {
    10,
    DT_COMPLETE_LOCAL_NAME,
    'i',
    'n',
    't',
    'e',
    's',
    't',
    '_',
    'C',
    'S',
    2,
    DT_FLAGS,
    0x05, // BLE limited discoverable mode and BR/EDR not supported
    3,
    DT_APPEARANCE,
    0x80,
    0x01, // 384, Generic Remote Control, Generic category
    5,
    DT_INCOMPLETE_LIST_16BIT_SERVICE_UUID,
    0x12,
    0x18,
    0x0F,
    0x18, // incomplete list of service class UUIDs (0x1812, 0x180F)
};

/**
 * @brief   BLE Scan Response Packet data
 */
const u8 tbl_scanRsp[] = {
    10,
    DT_COMPLETE_LOCAL_NAME,
    'i',
    'n',
    't',
    'e',
    's',
    't',
    '_',
    'C',
    'S',
};


/**
 * @brief      BLE Adv report event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int AA_dbg_adv_rpt = 0;
u32 tick_adv_rpt   = 0;

int app_le_adv_report_event_handle(u8 *p)
{
    event_adv_report_t *pa   = (event_adv_report_t *)p;
    s8                  rssi = pa->data[pa->len];

    #if 0 //debug, print ADV report number every 5 seconds
        AA_dbg_adv_rpt ++;
        if(clock_time_exceed(tick_adv_rpt, 5000000)){
            tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][EVT] Adv report", pa->mac, 6);
            tick_adv_rpt = clock_time();
        }
    #endif

    /*********************** Central Create connection demo: Key press or ADV pair packet triggers pair  ********************/
    #if (ACL_CENTRAL_SMP_ENABLE)
    if (central_smp_pending) { //if previous connection SMP not finish, can not create a new connection
        return 1;
    }
    #endif

    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
    if (central_sdp_pending) { //if previous connection SDP not finish, can not create a new connection
        return 1;
    }
    #endif

    if (central_disconnect_connhandle) { //one ACL connection central role is in un_pair disconnection flow, do not create a new one
        return 1;
    }

    int central_auto_connect = 0;
    int user_manual_pairing  = 0;

    //manual pairing methods 1: key press triggers
    user_manual_pairing = central_pairing_enable && (rssi > -66); //button trigger pairing(RSSI threshold, short distance)

    #if (ACL_CENTRAL_SMP_ENABLE)
    central_auto_connect = blc_smp_searchBondingPeripheralDevice_by_PeerMacAddress(pa->adr_type, pa->mac);
    #endif

    if (central_auto_connect || user_manual_pairing) {
        /* send create connection command to Controller, trigger it switch to initiating state. After this command, Controller
         * will scan all the ADV packets it received but not report to host, to find the specified device(mac_adr_type & mac_adr),
         * then send a "CONN_REQ" packet, enter to connection state and send a connection complete event
         * (HCI_SUB_EVT_LE_CONNECTION_COMPLETE) to Host*/
        u8 status = blc_ll_createConnection(SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, INITIATE_FP_ADV_SPECIFY, pa->adr_type, pa->mac, OWN_ADDRESS_PUBLIC, CONN_INTERVAL_31P25MS, CONN_INTERVAL_48P75MS, 0, CONN_TIMEOUT_4S, 0, 0xFFFF);


        if (status == BLE_SUCCESS) { //create connection success
            tlkapi_send_string_data(APP_LOG_EN, "[APP][CMD] create connection success", pa->mac, 6);
        }
    }
    /*********************** Central Create connection demo code end  *******************************************************/


    return 0;
}

/**
 * @brief      BLE Connection complete event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int app_le_connection_complete_event_handle(u8 *p)
{
    hci_le_connectionCompleteEvt_t *pConnEvt = (hci_le_connectionCompleteEvt_t *)p;

    if (pConnEvt->status == BLE_SUCCESS) {
        tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][EVT] Connection complete event", &pConnEvt->connHandle, sizeof(hci_le_connectionCompleteEvt_t) - 2);

        dev_char_info_insert_by_conn_event(pConnEvt);

        cur_step_mode_1_CollectDataNum = 0;

        if (pConnEvt->role == ACL_ROLE_CENTRAL)         // central role, process SMP and SDP if necessary
        {
    #if (ACL_CENTRAL_SMP_ENABLE)
            central_smp_pending = pConnEvt->connHandle; // this connection need SMP
    #endif


    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
            memset(&cur_sdp_device, 0, sizeof(dev_char_info_t));
            cur_sdp_device.conn_handle  = pConnEvt->connHandle;
            cur_sdp_device.peer_adrType = pConnEvt->peerAddrType;
            memcpy(cur_sdp_device.peer_addr, pConnEvt->peerAddr, 6);

            u8         temp_buff[sizeof(dev_att_t)];
            dev_att_t *pdev_att = (dev_att_t *)temp_buff;

            /* att_handle search in flash, if success, load char_handle directly from flash, no need SDP again */
            if (dev_char_info_search_peer_att_handle_by_peer_mac(pConnEvt->peerAddrType, pConnEvt->peerAddr, pdev_att)) {
                //cur_sdp_device.char_handle[1] =                                   //Speaker
                cur_sdp_device.char_handle[2] = pdev_att->char_handle[2]; //OTA
                cur_sdp_device.char_handle[3] = pdev_att->char_handle[3]; //consume report
                cur_sdp_device.char_handle[4] = pdev_att->char_handle[4]; //normal key report
                //cur_sdp_device.char_handle[6] =                                   //BLE Module, SPP Server to Client
                //cur_sdp_device.char_handle[7] =                                   //BLE Module, SPP Client to Server

                /* add the peer device att_handle value to conn_dev_list */
                dev_char_info_add_peer_att_handle(&cur_sdp_device);
            } else {
                central_sdp_pending = pConnEvt->connHandle; // mark this connection need SDP

        #if (ACL_CENTRAL_SMP_ENABLE)
                    //service discovery initiated after SMP done, trigger it in "GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE" event callBack.
        #else
                app_register_service(&app_service_discovery); //No SMP, service discovery can initiated now
        #endif
            }
    #endif
        }
    }

    return 0;
}

/**
 * @brief      BLE Disconnection event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int app_disconnect_event_handle(u8 *p)
{
    hci_disconnectionCompleteEvt_t *pDisConn = (hci_disconnectionCompleteEvt_t *)p;
    tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][EVT] disconnect event", &pDisConn->connHandle, 3);

    //terminate reason
    if (pDisConn->reason == HCI_ERR_CONN_TIMEOUT) {                 //connection timeout

    } else if (pDisConn->reason == HCI_ERR_REMOTE_USER_TERM_CONN) { //peer device send terminate command on link layer

    } else if (pDisConn->reason == HCI_ERR_CONN_TERM_BY_LOCAL_HOST) {
    } else {
    }


    /* if previous connection SMP & SDP not finished, clear flag */
    #if (ACL_CENTRAL_SMP_ENABLE)
    if (central_smp_pending == pDisConn->connHandle) {
        central_smp_pending = 0;
    }
    #endif
    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
    if (central_sdp_pending == pDisConn->connHandle) {
        central_sdp_pending = 0;
    }
    #endif

    if (central_disconnect_connhandle == pDisConn->connHandle) { //un_pair disconnection flow finish, clear flag
        central_disconnect_connhandle = 0;
    }

    dev_char_info_delete_by_connhandle(pDisConn->connHandle);


    return 0;
}

/**
 * @brief      BLE Connection update complete event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int app_le_connection_update_complete_event_handle(u8 *p)
{
    hci_le_connectionUpdateCompleteEvt_t *pUpt = (hci_le_connectionUpdateCompleteEvt_t *)p;
    tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][EVT] Connection Update Event", &pUpt->connHandle, 8);

    if (pUpt->status == BLE_SUCCESS) {
    }

    return 0;
}

//////////////////////////////////////////////////////////
// event call back
//////////////////////////////////////////////////////////
/**
 * @brief      BLE controller event handler call-back.
 * @param[in]  h       event type
 * @param[in]  p       Pointer point to event parameter buffer.
 * @param[in]  n       the length of event parameter.
 * @return
 */
int app_controller_event_callback(u32 h, u8 *p, int n)
{
    if (h & HCI_FLAG_EVENT_BT_STD) //Controller HCI event
    {
        u8 evtCode = h & 0xff;

        //------------ disconnect -------------------------------------
        if (evtCode == HCI_EVT_DISCONNECTION_COMPLETE) //connection terminate
        {
            app_disconnect_event_handle(p);
        } else if (evtCode == HCI_EVT_LE_META)         //LE Event
        {
            u8 subEvt_code = p[0];

            //------hci le event: le connection complete event---------------------------------
            if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_COMPLETE) // connection complete
            {
                app_le_connection_complete_event_handle(p);
            }
            //--------hci le event: le adv report event ----------------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT) // ADV packet
            {
                //after controller is set to scan state, it will report all the adv packet it received by this event

                app_le_adv_report_event_handle(p);
            }
            //------hci le event: le connection update complete event-------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_UPDATE_COMPLETE) // connection update
            {
                app_le_connection_update_complete_event_handle(p);
            }
            //------HCI LE event: LE CS Subevent Result event-------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_CS_SUBEVENT_RESULT) {
                hci_le_csSubeventResultEvt_t *pCsSubevent = (hci_le_csSubeventResultEvt_t *)p;

                u16 data_len = sizeof(hci_le_csSubeventResultEvt_t) + 3 + pCsSubevent->Step_Mode->len;
                tlkapi_printf(APP_LOG_EN, "CS Subevent Result,mode=%d,data_len=%d,Step_len=%d,%s", pCsSubevent->Step_Mode->mode, data_len, pCsSubevent->Step_Mode->len, hex_to_str(pCsSubevent, data_len));

                if (pCsSubevent->Step_Mode->mode == STEP_MODE_1) {
                    u8               mode1Result[CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY];
                    cs_step_mode1_t *pMode1 = (cs_step_mode1_t *)pCsSubevent->Step_Mode->data;
                    if ((pMode1->Packet_Quality == CS_STEP_RECEIVE_PACKET_QUALITY_HIGH) && (pMode1->Packet_NADM < CS_STEP_RECEIVE_PACKET_NADM_UNLIKELY)) {
                        step_mode_ToA_ToD[cur_step_mode_1_CollectDataNum] = (s16)(pMode1->ToA_ToD[0] + (pMode1->ToA_ToD[1] << 8));
                        step_mode_channel[cur_step_mode_1_CollectDataNum] = pCsSubevent->Step_Mode->channel;
                        cur_step_mode_1_CollectDataNum++;

                        s64 total_ToA_ToD   = 0;
                        s16 average_ToA_ToD = 0;
                        if (cur_step_mode_1_CollectDataNum == CS_RTT_AA_MAX_NUM) {
                            printf("-----ToA_ToD-----\n");
                            foreach (i, cur_step_mode_1_CollectDataNum) {
                                total_ToA_ToD += step_mode_ToA_ToD[i];
                                printf("%hd,", step_mode_ToA_ToD[i]);
                                if ((i + 1) % 16 == 0) {
                                    printf("\n");
                                }
                            }
                            printf("\n");
                            printf("-----channel-----\n");
                            foreach (i, cur_step_mode_1_CollectDataNum) {
                                printf("%hd,", step_mode_channel[i]);
                                if ((i + 1) % 16 == 0) {
                                    printf("\n");
                                }
                            }
                            printf("\n");
                            printf("[APP][CS]mode_1\n");
                            printf("    total_ToA_ToD=%lld\n", total_ToA_ToD);
                            printf("    CollectDataNum=%d\n", cur_step_mode_1_CollectDataNum);

                            total_ToA_ToD *= 10000;
                            average_ToA_ToD                = (total_ToA_ToD / cur_step_mode_1_CollectDataNum) / 10000;
                            cur_step_mode_1_CollectDataNum = 0;

                            printf("    average_ToA_ToD=%hd\n", average_ToA_ToD);
                            tlkapi_printf(APP_CONTR_EVT_LOG_EN, "[APP][CS]mode_1 average_ToA_ToD=%d", average_ToA_ToD);
                        }
                    }
                } else if (pCsSubevent->Step_Mode->mode == STEP_MODE_2) {
                    s16 tone_PCT_I;
                    s16 tone_PCT_Q;
                    u8  step_Tone_QI;

                    u8               mode2Result[CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1];
                    cs_step_mode2_t *pMode2 = (cs_step_mode2_t *)pCsSubevent->Step_Mode->data;

                    cs_subevent_mode_type = pCsSubevent->Step_Mode->mode;

                    tone_PCT_I = (pMode2->Tone[0].Tone_PCT[0] + ((pMode2->Tone[0].Tone_PCT[1] & 0xF) << 8));
                    if (tone_PCT_I & BIT(11)) {
                        tone_PCT_I |= 0xF000;
                    }
                    tone_PCT_Q = (((pMode2->Tone[0].Tone_PCT[1] & 0xF0) >> 4) + (pMode2->Tone[0].Tone_PCT[2] << 4));
                    if (tone_PCT_Q & BIT(11)) {
                        tone_PCT_Q |= 0xF000;
                    }
                    step_Tone_QI = pMode2->Tone[0].Tone_Quality_Indicator;

                    if (cs_subevent_test_mode == CS_SUBEVENT_TEST_INITIATOR) {
                        initi_pct[cs_subevent_non_mode_0_initiator_num * 2]     = (s32)tone_PCT_I;
                        initi_pct[cs_subevent_non_mode_0_initiator_num * 2 + 1] = (s32)tone_PCT_Q;
                        initi_tone_QI[cs_subevent_non_mode_0_initiator_num]     = step_Tone_QI;
                        cs_subevent_non_mode_0_initiator_num++;
                    } else if (cs_subevent_test_mode == CS_SUBEVENT_TEST_REFLECTOR) {
                        refle_pct[cs_subevent_non_mode_0_reflector_num * 2]     = (s32)tone_PCT_I;
                        refle_pct[cs_subevent_non_mode_0_reflector_num * 2 + 1] = (s32)tone_PCT_Q;
                        refle_tone_QI[cs_subevent_non_mode_0_reflector_num]     = step_Tone_QI;
                        cs_subevent_non_mode_0_reflector_num++;
                    }
                }
            }
            //------HCI LE event: LE CS Subevent Result Continue event-------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_CS_SUBEVENT_RESULT_CONTINUE) {
                hci_le_csSubeventResultContinueEvt_t *pCsSubevent = (hci_le_csSubeventResultContinueEvt_t *)p;

                u16 data_len = sizeof(hci_le_csSubeventResultContinueEvt_t) + 3 + pCsSubevent->Step_Mode->len;
                tlkapi_printf(APP_CONTR_EVT_LOG_EN, "[APP][CS]SubeventResultContinue,mode=%d,len=%d,%s", pCsSubevent->Step_Mode->mode, data_len, hex_to_str(pCsSubevent, data_len));
            }
        }
    }


    return 0;
}

/**
 * @brief      BLE host event handler call-back.
 * @param[in]  h       event type
 * @param[in]  para    Pointer point to event parameter buffer.
 * @param[in]  n       the length of event parameter.
 * @return
 */
int app_host_event_callback(u32 h, u8 *para, int n)
{
    u8 event = h & 0xFF;

    switch (event) {
    case GAP_EVT_SMP_PAIRING_BEGIN:
    {
    } break;

    case GAP_EVT_SMP_PAIRING_SUCCESS:
    {
    } break;

    case GAP_EVT_SMP_PAIRING_FAIL:
    {
    #if (ACL_CENTRAL_SMP_ENABLE)
        gap_smp_pairingFailEvt_t *pEvt = (gap_smp_pairingFailEvt_t *)para;

        if (dev_char_get_conn_role_by_connhandle(pEvt->connHandle) == ACL_ROLE_CENTRAL) {
            if (central_smp_pending == pEvt->connHandle) {
                central_smp_pending = 0;
                tlkapi_send_string_data(APP_SMP_LOG_EN, "[APP][SMP] paring fail", &pEvt->connHandle, sizeof(gap_smp_pairingFailEvt_t));
            }
        }
    #endif
    } break;

    case GAP_EVT_SMP_CONN_ENCRYPTION_DONE:
    {
        gap_smp_connEncDoneEvt_t *pEvt = (gap_smp_connEncDoneEvt_t *)para;
        tlkapi_send_string_data(APP_SMP_LOG_EN, "[APP][SMP] Connection encryption done event", &pEvt->connHandle, sizeof(gap_smp_connEncDoneEvt_t));
    } break;

    case GAP_EVT_SMP_SECURITY_PROCESS_DONE:
    {
        gap_smp_connEncDoneEvt_t *pEvt = (gap_smp_connEncDoneEvt_t *)para;
        tlkapi_send_string_data(APP_SMP_LOG_EN, "[APP][SMP] Security process done event", &pEvt->connHandle, sizeof(gap_smp_connEncDoneEvt_t));

        if (dev_char_get_conn_role_by_connhandle(pEvt->connHandle) == ACL_ROLE_CENTRAL) {
    #if (ACL_CENTRAL_SMP_ENABLE)
            if (dev_char_get_conn_role_by_connhandle(pEvt->connHandle) == ACL_ROLE_CENTRAL) {
                if (central_smp_pending == pEvt->connHandle) {
                    central_smp_pending = 0;
                }
            }
    #endif

    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)                       //SMP finish
            if (central_sdp_pending == pEvt->connHandle) {    //SDP is pending
                app_register_service(&app_service_discovery); //start SDP now
            }
    #endif
        }
    } break;

    case GAP_EVT_SMP_TK_DISPLAY:
    {
    } break;

    case GAP_EVT_SMP_TK_REQUEST_PASSKEY:
    {
    } break;

    case GAP_EVT_SMP_TK_REQUEST_OOB:
    {
    } break;

    case GAP_EVT_SMP_TK_NUMERIC_COMPARE:
    {
    } break;

    case GAP_EVT_ATT_EXCHANGE_MTU:
    {
    } break;

    case GAP_EVT_GATT_HANDLE_VALUE_CONFIRM:
    {
    } break;

    default:
        break;
    }

    return 0;
}

/**
 * @brief       This function is used to send consumer HID report by USB.
 * @param[in]   conn     - connection handle
 * @param[in]   p        - Pointer point to data buffer.
 * @return
 */
void att_keyboard_media(u16 conn, u8 *p)
{
    u16 consumer_key = p[0] | p[1] << 8;


    #if (1 && UI_LED_ENABLE) //Demo effect: when peripheral send Vol+/Vol- to central, LED GPIO toggle to show the result
    if (consumer_key == MKEY_VOL_UP) {
        gpio_toggle(GPIO_LED_GREEN);
    } else if (consumer_key == MKEY_VOL_DN) {
        gpio_toggle(GPIO_LED_BLUE);
    }
    #endif
}

/**
 * @brief       This function is used to send HID report by USB.
 * @param[in]   conn     - connection handle
 * @param[in]   p        - Pointer point to data buffer.
 * @return
 */
void att_keyboard(u16 conn, u8 *p)
{
}

    #define HID_HANDLE_CONSUME_REPORT  25
    #define HID_HANDLE_KEYBOARD_REPORT 29
    #define AUDIO_HANDLE_MIC           52
    #define OTA_HANDLE_DATA            48

/**
 * @brief      BLE GATT data handler call-back.
 * @param[in]  connHandle     connection handle.
 * @param[in]  pkt             Pointer point to data packet buffer.
 * @return
 */
int app_gatt_data_handler(u16 connHandle, u8 *pkt)
{
    if (dev_char_get_conn_role_by_connhandle(connHandle) == ACL_ROLE_CENTRAL) //GATT data for Central
    {
    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
        if (central_sdp_pending == connHandle) {                              //ATT service discovery is ongoing on this conn_handle
            //when service discovery function is running, all the ATT data from peripheral
            //will be processed by it,  user can only send your own att cmd after  service discovery is over
            host_att_client_handler(connHandle, pkt); //handle this ATT data by service discovery process
        }
    #endif

        rf_packet_att_t *pAtt = (rf_packet_att_t *)pkt;

        //so any ATT data before service discovery will be dropped
        dev_char_info_t *dev_info = dev_char_info_search_by_connhandle(connHandle);
        if (dev_info) {
            //-------   user process ------------------------------------------------
            u16 attHandle = pAtt->handle;

            if (pAtt->opcode == ATT_OP_HANDLE_VALUE_NOTI) {
                //---------------   consumer key --------------------------
    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
                if (attHandle == dev_info->char_handle[3]) // Consume Report In (Media Key)
    #else
                if (attHandle == HID_HANDLE_CONSUME_REPORT) //Demo device(825x_ble_sample) Consume Report AttHandle value is 25
    #endif
                {
                    att_keyboard_media(connHandle, pAtt->dat);
                }
                //---------------   keyboard key --------------------------
    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
                else if (attHandle == dev_info->char_handle[4]) // Key Report In
    #else
                else if (attHandle == HID_HANDLE_KEYBOARD_REPORT) // Demo device(825x_ble_sample) Key Report AttHandle value is 29
    #endif
                {
                    att_keyboard(connHandle, pAtt->dat);
                }
    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
                else if (attHandle == dev_info->char_handle[0]) // AUDIO Notify
    #else
                else if (attHandle == AUDIO_HANDLE_MIC) // Demo device(825x_ble_remote) Key Report AttHandle value is 52
    #endif
                {

                } else {
                }
            } else if (pAtt->opcode == ATT_OP_HANDLE_VALUE_IND) {
            }
        }

        if (!(pAtt->opcode & 0x01)) {
            switch (pAtt->opcode) {
            case ATT_OP_FIND_INFO_REQ:
            case ATT_OP_FIND_BY_TYPE_VALUE_REQ:
            case ATT_OP_READ_BY_TYPE_REQ:
            case ATT_OP_READ_BY_GROUP_TYPE_REQ:
                blc_gatt_pushErrResponse(connHandle, pAtt->opcode, pAtt->handle, ATT_ERR_ATTR_NOT_FOUND);
                break;
            case ATT_OP_READ_REQ:
            case ATT_OP_READ_BLOB_REQ:
            case ATT_OP_READ_MULTI_REQ:
            case ATT_OP_WRITE_REQ:
            case ATT_OP_PREPARE_WRITE_REQ:
                blc_gatt_pushErrResponse(connHandle, pAtt->opcode, pAtt->handle, ATT_ERR_INVALID_HANDLE);
                break;
            case ATT_OP_EXECUTE_WRITE_REQ:
            case ATT_OP_HANDLE_VALUE_CFM:
            case ATT_OP_WRITE_CMD:
            case ATT_OP_SIGNED_WRITE_CMD:
                //ignore
                break;
            default: //no action
                break;
            }
        }
    } else { //GATT data for Peripheral
    }


    return 0;
}


    #if (BATT_CHECK_ENABLE) //battery check must do before OTA relative operation

_attribute_data_retention_ u32 lowBattDet_tick = 0;

/**
 * @brief       this function is used to process battery power.
 *              The low voltage protection threshold 2.0V is an example and reference value. Customers should
 *              evaluate and modify these thresholds according to the actual situation. If users have unreasonable designs
 *              in the hardware circuit, which leads to a decrease in the stability of the power supply network, the
 *              safety thresholds must be increased as appropriate.
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void user_battery_power_check(u16 alarm_vol_mv)
{
    /*For battery-powered products, as the battery power will gradually drop, when the voltage is low to a certain
      value, it will cause many problems.
        a) When the voltage is lower than operating voltage range of chip, chip can no longer guarantee stable operation.
        b) When the battery voltage is low, due to the unstable power supply, the write and erase operations
            of Flash may have the risk of error, causing the program firmware and user data to be modified abnormally,
            and eventually causing the product to fail. */
    u8 battery_check_returnValue = 0;
    if (analog_read(USED_DEEP_ANA_REG) & LOW_BATT_FLG) {
        battery_check_returnValue = app_battery_power_check(alarm_vol_mv + 200);
    } else {
        battery_check_returnValue = app_battery_power_check(alarm_vol_mv);
    }
    if (battery_check_returnValue) {
        analog_write_reg8(USED_DEEP_ANA_REG, analog_read_reg8(USED_DEEP_ANA_REG) & (~LOW_BATT_FLG)); //clr
    } else {
        #if (UI_LED_ENABLE)                                                                          //led indicate
        for (int k = 0; k < 3; k++) {
            gpio_write(GPIO_LED_BLUE, LED_ON_LEVEL);
            sleep_us(200000);
            gpio_write(GPIO_LED_BLUE, !LED_ON_LEVEL);
            sleep_us(200000);
        }
        #endif
        analog_write_reg8(USED_DEEP_ANA_REG, analog_read_reg8(USED_DEEP_ANA_REG) | LOW_BATT_FLG); //mark

        #if (UI_KEYBOARD_ENABLE)
        u32 pin[] = KB_DRIVE_PINS;
        for (int i = 0; i < (sizeof(pin) / sizeof(*pin)); i++) {
            cpu_set_gpio_wakeup(pin[i], 1, 1);              //drive pin pad high wakeup deepsleep
        }

        cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_PAD, 0); //deepsleep
        #endif
    }
}

    #endif


int user_client_2_server_write_callback(u16 connHandle, void *p)
{
    rf_packet_att_data_t *req = (rf_packet_att_data_t *)p;

    if (cs_subevent_test_mode == CS_SUBEVENT_TEST_INITIATOR) {
        //Type + Len + Tone_QI + PCT_I_2Byte + PCT_Q_2Byte + ... + Tone_QI + PCT_I_2Byte + PCT_Q_2Byte
        if (req->dat[0] == STEP_MODE_2) {
            u8 len = req->dat[1];

            printf("[APP][GATT]write_CB,mode=%d,len=%d\n", req->dat[0], len);

            u8  step_num = len / 5;
            s16 tone_PCT_I;
            s16 tone_PCT_Q;
            foreach (i, step_num) {
                refle_tone_QI[i] = req->dat[2 + i * 5];

                tone_PCT_I = req->dat[2 + i * 5 + 1] + (req->dat[2 + i * 5 + 2] << 8);
                if (tone_PCT_I & BIT(11)) {
                    tone_PCT_I |= 0xF000;
                }

                tone_PCT_Q = req->dat[2 + i * 5 + 3] + (req->dat[2 + i * 5 + 4] << 8);
                if (tone_PCT_Q & BIT(11)) {
                    tone_PCT_Q |= 0xF000;
                }

                refle_pct[i * 2]     = (s32)tone_PCT_I;
                refle_pct[i * 2 + 1] = (s32)tone_PCT_Q;
            }

            printf("  refle_pct:");
            foreach (i, step_num * 2) {
                printf("%d,", refle_pct[i]);
            }
            printf("\n");

            printf("  refle_tone_QI:");
            foreach (i, step_num) {
                printf("%d,", refle_tone_QI[i]);
            }
            printf("\n");

            cs_subevent_non_mode_0_reflector_data_flag = 1;
        }
    }

    return 1;
}

///////////////////////////////////////////

/**
 * @brief       user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]   none
 * @return      none
 */
_attribute_no_inline_ void user_init_normal(void)
{
    //////////////////////////// basic hardware Initialization  Begin //////////////////////////////////
    /* random number generator must be initiated here( in the beginning of user_init_normal).
     * When deepSleep retention wakeUp, no need initialize again */
    random_generator_init();

    #if (TLKAPI_DEBUG_ENABLE)
    tlkapi_debug_init();
    blc_debug_enableStackLog(STK_LOG_NONE);
    #endif

    #if (BATT_CHECK_ENABLE)
    /*The SDK must do a quick low battery detect during user initialization instead of waiting
      until the main_loop. The reason for this process is to avoid application errors that the device
      has already working at low power.
      Considering the working voltage of MCU and the working voltage of flash, if the Demo is set below 2.0V,
      the chip will alarm and deep sleep (Due to PM does not work in the current version of B92, it does not go
      into deepsleep), and once the chip is detected to be lower than 2.0V, it needs to wait until the voltage rises to 2.2V,
      the chip will resume normal operation. Consider the following points in this design:
        At 2.0V, when other modules are operated, the voltage may be pulled down and the flash will not
        work normally. Therefore, it is necessary to enter deepsleep below 2.0V to ensure that the chip no
        longer runs related modules;
        When there is a low voltage situation, need to restore to 2.2V in order to make other functions normal,
        this is to ensure that the power supply voltage is confirmed in the charge and has a certain amount of
        power, then start to restore the function can be safer.*/

    user_battery_power_check(2000);
    #endif

    blc_readFlashSize_autoConfigCustomFlashSector();

    /* attention that this function must be called after "blc readFlashSize_autoConfigCustomFlashSector" !!!*/
    blc_app_loadCustomizedParameters_normal();

    //////////////////////////// basic hardware Initialization  End /////////////////////////////////


    //////////////////////////// BLE stack Initialization  Begin //////////////////////////////////

    u8 mac_public[6];
    u8 mac_random_static[6];

    blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);


    //////////// LinkLayer Initialization  Begin /////////////////////////
    blc_ll_initBasicMCU();

    blc_ll_initStandby_module(mac_public);

    blc_ll_initLegacyAdvertising_module();

    blc_ll_initLegacyScanning_module();

    blc_ll_initLegacyInitiating_module();

    blc_ll_initAclConnection_module();
    blc_ll_initAclCentralRole_module();
    blc_ll_initAclPeriphrRole_module();

    #if (MCU_CORE_TYPE == MCU_CORE_B92)
    flash_read_page(FLASH_ADR_CS_TEST_MODE, 1, &cs_subevent_test_mode);
    #endif
    if (cs_subevent_test_mode == CS_SUBEVENT_TEST_INITIATOR) {
        blc_ll_setMaxConnectionNumber(0, 1); //initiator
        tlkapi_send_string_data(APP_LOG_EN, "[APP][INI] MaxConnectionNumber: m0s1, initiator", 0, 0);
        printf("[APP][INI] MaxConnectionNumber: m0s1, initiator\n");
    } else if (cs_subevent_test_mode == CS_SUBEVENT_TEST_REFLECTOR) {
        blc_ll_setMaxConnectionNumber(1, 0); //reflector
        tlkapi_send_string_data(APP_LOG_EN, "[APP][INI] MaxConnectionNumber: m1s0, reflector", 0, 0);
        printf("[APP][INI] MaxConnectionNumber: m1s0, reflector\n");
    } else if (cs_subevent_test_mode == CS_SUBEVENT_TEST_ONLY_M1S0) {
        blc_ll_setMaxConnectionNumber(1, 0); //m1s0
        tlkapi_send_string_data(APP_LOG_EN, "[APP][INI] MaxConnectionNumber: m1s0", 0, 0);
        printf("[APP][INI] MaxConnectionNumber: m1s0\n");
    } else if (cs_subevent_test_mode == CS_SUBEVENT_TEST_ONLY_M0S1) {
        blc_ll_setMaxConnectionNumber(0, 1); //m0s1
        tlkapi_send_string_data(APP_LOG_EN, "[APP][INI] MaxConnectionNumber: m0s1", 0, 0);
        printf("[APP][INI] MaxConnectionNumber: m0s1\n");
    } else {
        blc_ll_setMaxConnectionNumber(ACL_CENTRAL_MAX_NUM, ACL_PERIPHR_MAX_NUM);
        tlkapi_send_string_data(APP_LOG_EN, "[APP][INI] MaxConnectionNumber: m4s4", 0, 0);
        printf("[APP][INI] MaxConnectionNumber: m4s4\n");
    }

    #if (MCU_CORE_TYPE == MCU_CORE_B92)
    u8 step_mode[CS_TEST_STEP_MODE_ARRAY_NUM] = {CS_STEP_NULL};
    flash_read_page(FLASH_ADR_CS_STEP_MODE, CS_TEST_STEP_MODE_ARRAY_NUM, &step_mode);
    if (step_mode[0] == CS_STEP_MODE_0) {
        u8 i = 0;
        while (step_mode[i] <= CS_STEP_MODE_3) {
            step_mode_array[i] = step_mode[i];
            i++;
            if (i >= CS_TEST_STEP_MODE_ARRAY_NUM - 1) {
                break;
            }
        }
        step_mode_array[i] = CS_STEP_NULL;
    }
    printf("[APP][INI] step_mode_array = ");
    array_printf(step_mode_array, CS_TEST_STEP_MODE_ARRAY_NUM);

    u8 channel_diff[2];
    flash_read_page(FLASH_ADR_CS_CHANNEL_DIFF, 1, &channel_diff);
    if (channel_diff[0] != 0xFF) {
        cs_channel_diff = channel_diff[0];
    }
    if (cs_channel_diff == CS_CHANNEL_MAP_ENABLE_FLAG) {
        printf("[APP][INI] cs_channel_map enable:");
        array_printf(cs_channel_map, CS_TEST_STEP_MODE_ARRAY_NUM);
    } else {
        printf("[APP][INI] cs_channel_diff = %d\n", cs_channel_diff);
    }
    #endif

    blc_ll_setAclConnMaxOctetsNumber(ACL_CONN_MAX_RX_OCTETS, ACL_CENTRAL_MAX_TX_OCTETS, ACL_PERIPHR_MAX_TX_OCTETS);

    /* all ACL connection share same RX FIFO */
    blc_ll_initAclConnRxFifo(app_acl_rx_fifo, ACL_RX_FIFO_SIZE, ACL_RX_FIFO_NUM);
    /* ACL Central TX FIFO */
    blc_ll_initAclCentralTxFifo(app_acl_cen_tx_fifo, ACL_CENTRAL_TX_FIFO_SIZE, ACL_CENTRAL_TX_FIFO_NUM, ACL_CENTRAL_MAX_NUM);
    /* ACL Peripheral TX FIFO */
    blc_ll_initAclPeriphrTxFifo(app_acl_per_tx_fifo, ACL_PERIPHR_TX_FIFO_SIZE, ACL_PERIPHR_TX_FIFO_NUM, ACL_PERIPHR_MAX_NUM);

    blc_ll_setAclCentralBaseConnectionInterval(CONN_INTERVAL_31P25MS);


    //////////// LinkLayer Initialization  End /////////////////////////


    //////////// HCI Initialization  Begin /////////////////////////
    blc_hci_registerControllerDataHandler(blc_l2cap_pktHandler);

    blc_hci_registerControllerEventHandler(app_controller_event_callback); //controller hci event to host all processed in this func

    //bluetooth event
    blc_hci_setEventMask_cmd(HCI_EVT_MASK_DISCONNECTION_COMPLETE);

    //bluetooth low energy(LE) event
    blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_CONNECTION_COMPLETE | HCI_LE_EVT_MASK_ADVERTISING_REPORT | HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE);
    blc_hci_le_setEventMask_2_cmd(HCI_LE_EVT_MASK_2_CS_SUBEVENT_RESULT);


    u8 error_code = blc_contr_checkControllerInitialization();
    if (error_code != INIT_SUCCESS) {
        /* It's recommended that user set some UI alarm to know the exact error, e.g. LED shine, print log */
        write_log32(0x88880000 | error_code);
    #if (TLKAPI_DEBUG_ENABLE)
        tlkapi_send_string_data(APP_LOG_EN, "[APP][INI] Controller INIT ERROR", &error_code, 1);
        while (1) {
            tlkapi_debug_handler();
        }
    #else
        while (1)
            ;
    #endif
    }
    //////////// HCI Initialization  End /////////////////////////


    //////////// Host Initialization  Begin /////////////////////////
    /* Host Initialization */
    /* GAP initialization must be done before any other host feature initialization !!! */
    blc_gap_init();

    /* L2CAP data buffer Initialization */
    blc_l2cap_initAclCentralBuffer(app_cen_l2cap_rx_buf, CENTRAL_L2CAP_BUFF_SIZE, NULL, 0);
    blc_l2cap_initAclPeripheralBuffer(app_per_l2cap_rx_buf, PERIPHR_L2CAP_BUFF_SIZE, app_per_l2cap_tx_buf, PERIPHR_L2CAP_BUFF_SIZE);

    blc_att_setCentralRxMtuSize(CENTRAL_ATT_RX_MTU);    ///must be placed after "blc_gap_init"
    blc_att_setPeripheralRxMtuSize(PERIPHR_ATT_RX_MTU); ///must be placed after "blc_gap_init"

    /* GATT Initialization */
    my_gatt_init();
    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
    host_att_register_idle_func(main_idle_loop);
    #endif
    blc_spp_registerWriteCallback(user_client_2_server_write_callback);
    blc_gatt_register_data_handler(app_gatt_data_handler);

    /* SMP Initialization */
    #if (ACL_PERIPHR_SMP_ENABLE || ACL_CENTRAL_SMP_ENABLE)

    blc_smp_configPairingSecurityInfoStorageAddressAndSize(flash_sector_smp_storage, FLASH_SMP_PAIRING_MAX_SIZE);
    #endif

    #if (ACL_PERIPHR_SMP_ENABLE)                                               //Peripheral SMP Enable
    blc_smp_setSecurityLevel_periphr(Unauthenticated_Pairing_with_Encryption); //LE_Security_Mode_1_Level_2
    #else
    blc_smp_setSecurityLevel_periphr(No_Security);
    #endif

    #if (ACL_CENTRAL_SMP_ENABLE)
    blc_smp_setSecurityLevel_central(Unauthenticated_Pairing_with_Encryption); //LE_Security_Mode_1_Level_2
    #else
    blc_smp_setSecurityLevel_central(No_Security);
    #endif

    blc_smp_smpParamInit();


    //host(GAP/SMP/GATT/ATT) event process: register host event callback and set event mask
    blc_gap_registerHostEventHandler(app_host_event_callback);
    blc_gap_setEventMask(GAP_EVT_MASK_SMP_PAIRING_BEGIN |
                         GAP_EVT_MASK_SMP_PAIRING_SUCCESS |
                         GAP_EVT_MASK_SMP_PAIRING_FAIL |
                         GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE);
    //////////// Host Initialization  End /////////////////////////

    //////////////////////////// BLE stack Initialization  End //////////////////////////////////


    //////////////////////////// User Configuration for BLE application ////////////////////////////
    blc_ll_setAdvData(tbl_advData, sizeof(tbl_advData));
    blc_ll_setScanRspData(tbl_scanRsp, sizeof(tbl_scanRsp));
    blc_ll_setAdvParam(ADV_INTERVAL_50MS, ADV_INTERVAL_50MS, ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
    #if (GPIO_TRIGGER_TEST_ENABLE)
    blc_ll_setAdvEnable(BLC_ADV_DISABLE); //ADV disable, test mode
    #else
    blc_ll_setAdvEnable(BLC_ADV_ENABLE); //ADV enable
    #endif

    blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_WINDOW_50MS, OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
    #if (GPIO_TRIGGER_TEST_ENABLE)
    blc_ll_setScanEnable(BLC_SCAN_DISABLE, DUP_FILTER_DISABLE); //SCAN disable, test mode
    #else
    blc_ll_setScanEnable(BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);
    #endif

    if (cs_subevent_test_mode == CS_SUBEVENT_TEST_INITIATOR || cs_subevent_test_mode == CS_SUBEVENT_TEST_REFLECTOR) {
        rf_set_power_level_index(RF_POWER_P9dBm);
        blc_ll_initCsRxFifo_test();
        u32 hadm_version = get_version();
        tlkapi_send_string_data(APP_LOG_EN, "[APP][HADM] hadm_version", &hadm_version, 4);
        printf("[APP][HADM] hadm_version = %d.%d.%d\n", (((hadm_version) >> 16) & 0xFF), (((hadm_version) >> 8) & 0xFF), ((hadm_version) & 0xFF));
        extern _attribute_ble_data_retention_ u8 cs_rx_fifo_b[];
        printf("  Address cs_test_rx_buff=0x%x, cs_rx_fifo_b=0x%x, CS_RX_FIFO_SIZE=%d\n", &cs_test_rx_buff, &cs_rx_fifo_b, DMA_CS_RFRX_MAX_DMA_LEN);
    } else {
        rf_set_power_level_index(RF_POWER_P3dBm);
    }

    #if (BLE_APP_PM_ENABLE)
    blc_ll_initPowerManagement_module();
    blc_pm_setSleepMask(PM_SLEEP_LEG_ADV | PM_SLEEP_LEG_SCAN | PM_SLEEP_ACL_PERIPHR | PM_SLEEP_ACL_CENTRAL);
    #endif


    #if (GPIO_TRIGGER_TEST_ENABLE)
    gpio_function_en(GPIO_TRIGGER_PIN);
    gpio_output_dis(GPIO_TRIGGER_PIN);
    gpio_input_en(GPIO_TRIGGER_PIN);
    gpio_set_up_down_res(GPIO_TRIGGER_PIN, GPIO_PIN_PULLDOWN_100K);

    //if disable gpio interrupt,choose disable gpio mask , use interface gpio_clr_irq_mask instead of gpio_irq_dis,if use gpio_irq_dis,may generate a false interrupt.
    gpio_set_irq(GPIO_TRIGGER_PIN, INTR_RISING_EDGE);
    plic_interrupt_enable(IRQ_GPIO);

    plic_set_priority(IRQ_ZB_RT, IRQ_PRI_LEV1);
    #endif
    ////////////////////////////////////////////////////////////////////////////////////////////////
}

/**
 * @brief       user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]   none
 * @return      none
 */
void user_init_deepRetn(void)
{
    //blc_app_loadCustomizedParameters_deepRetn();
}

void app_process_power_management(void)
{
    #if (BLE_APP_PM_ENABLE)
    //Log needs to be output ASAP, and UART invalid after suspend. So Log disable sleep.
    //User tasks can go into suspend, but no deep sleep. So we use manual latency.
    if (tlkapi_debug_isBusy()) {
        blc_pm_setSleepMask(PM_SLEEP_DISABLE);
    } else {
        int user_task_flg = 0;

        blc_pm_setSleepMask(PM_SLEEP_LEG_ADV | PM_SLEEP_LEG_SCAN | PM_SLEEP_ACL_PERIPHR | PM_SLEEP_ACL_CENTRAL);

        #if (UI_KEYBOARD_ENABLE)
        user_task_flg |= user_task_flg || scan_pin_need || key_not_released;
        #endif

        if (user_task_flg) {
            bls_pm_setManualLatency(0);
        }
    }
    #endif
}

/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////

/**
 * @brief     BLE main idle loop
 * @param[in]  none.
 * @return     none.
 */
int main_idle_loop(void)
{
    ////////////////////////////////////// BLE entry /////////////////////////////////
    blc_sdk_main_loop();

    if (cs_subevent_test_mode == CS_SUBEVENT_TEST_INITIATOR || cs_subevent_test_mode == CS_SUBEVENT_TEST_REFLECTOR) {
        blt_ll_cs_main_loop_test();
    }

    ////////////////////////////////////// Debug entry /////////////////////////////////
    #if (TLKAPI_DEBUG_ENABLE)
    tlkapi_debug_handler();
    #endif

    ////////////////////////////////////// UI entry /////////////////////////////////
    #if (BATT_CHECK_ENABLE)
    /*The frequency of low battery detect is controlled by the variable lowBattDet_tick, which is executed every
         500ms in the demo. Users can modify this time according to their needs.*/
    if (battery_get_detect_enable() && clock_time_exceed(lowBattDet_tick, 500000)) {
        lowBattDet_tick = clock_time();
        user_battery_power_check(BAT_DEEP_THRESHOLD_MV);
    }
    #endif

    #if (UI_KEYBOARD_ENABLE)
    proc_keyboard(0, 0, 0);
    #endif


    proc_central_role_unpair();

    ////////////////////////////////////// PM entry /////////////////////////////////
    app_process_power_management();

    return 0; //must return 0 due to SDP flow
}

void distance_estimation_mode_2_loop(void)
{
    if (cs_subevent_end_flag && cs_subevent_non_mode_0_step_num) {
        if (cs_subevent_test_mode == CS_SUBEVENT_TEST_INITIATOR) {
            if (cs_subevent_non_mode_0_initiator_num == cs_subevent_non_mode_0_step_num) {
                printf("initiator pct data\n");
                printf("  non_mode_0_step_num = %d\n", cs_subevent_non_mode_0_step_num);

                printf("  initi_pct:");
                foreach (i, cs_subevent_non_mode_0_step_num * 2) {
                    printf("%d,", initi_pct[i]);
                }
                printf("\n");

                printf("  initi_tone_QI:");
                foreach (i, cs_subevent_non_mode_0_step_num) {
                    printf("%d,", initi_tone_QI[i]);
                }
                printf("\n");

                cs_subevent_non_mode_0_step_num = 0;

                cs_subevent_non_mode_0_initiator_data_flag = 1;
            }
        } else if (cs_subevent_test_mode == CS_SUBEVENT_TEST_REFLECTOR) {
            if (cs_subevent_non_mode_0_reflector_num == cs_subevent_non_mode_0_step_num) {
                printf("reflector distance\n");
                printf("  non_mode_0_step_num = %d\n", cs_subevent_non_mode_0_step_num);

                printf("  refle_pct:");
                foreach (i, cs_subevent_non_mode_0_step_num * 2) {
                    printf("%d,", refle_pct[i]);
                }
                printf("\n");

                printf("  refle_tone_QI:");
                foreach (i, cs_subevent_non_mode_0_step_num) {
                    printf("%d,", refle_tone_QI[i]);
                }
                printf("\n");

                //Type + Len + Tone_QI + PCT_I_2Byte + PCT_Q_2Byte + ... + Tone_QI + PCT_I_2Byte + PCT_Q_2Byte
                u8 write_buff[150];
                u8 len        = 2 + cs_subevent_non_mode_0_step_num * 5;
                write_buff[0] = cs_subevent_mode_type;
                write_buff[1] = len - 2;
                foreach (i, cs_subevent_non_mode_0_step_num) {
                    write_buff[2 + i * 5]     = refle_tone_QI[i];
                    write_buff[2 + i * 5 + 1] = U16_LO(refle_pct[i * 2]);
                    write_buff[2 + i * 5 + 2] = U16_HI(refle_pct[i * 2]);
                    write_buff[2 + i * 5 + 3] = U16_LO(refle_pct[i * 2 + 1]);
                    write_buff[2 + i * 5 + 4] = U16_HI(refle_pct[i * 2 + 1]);
                }

                blc_attc_sendWriteCommand(0x80, SPP_CLIENT_TO_SERVER_DP_H, write_buff, len);

                cs_subevent_non_mode_0_step_num = 0;
            }
        }
    }

    if (cs_subevent_test_mode == CS_SUBEVENT_TEST_INITIATOR) {
        if (cs_subevent_non_mode_0_initiator_data_flag && cs_subevent_non_mode_0_reflector_data_flag) {
            cs_subevent_non_mode_0_initiator_data_flag = 0;
            cs_subevent_non_mode_0_reflector_data_flag = 0;


            float   distanceReal = 5.74346;
            float   distPhaseDiff, distMusic, likeliness, EVDCap, T2WRDiffMean;
            int     nIterMaxEig, nIterPS, nSigCnt;
            complex ipmpct[40];
            complex tmp;
            int     cali[40 * 2] = {1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0};

            //result of 4m, ipm is I from tesCollectData, pct is Q from  tesCollectData
            int ipm[40 * 2];
            int pct[40 * 2];

            u8 initi_num = 0;
            u8 refle_num = 0;
            u8 drop_num  = 0;

            foreach (i, cs_subevent_non_mode_0_initiator_num) {
                if (initi_tone_QI[i] < CS_STEP_RECEIVE_TONE_QUALITY_LOW) {
                    initi_num++;
                }
                if (refle_tone_QI[i] < CS_STEP_RECEIVE_TONE_QUALITY_LOW) {
                    refle_num++;
                }
            }

            drop_num = cs_subevent_non_mode_0_initiator_num - min(initi_num, refle_num);
            printf("initi_num=%d,refle_num=%d,drop_num=%d\n", initi_num, refle_num, drop_num);

            int channel;
            //channel= cs_subevent_non_mode_0_initiator_num - 1;//first channel quality abnormal
            channel = cs_subevent_non_mode_0_initiator_num - drop_num;

            if (!channel) {
                printf("  error:channel_num=%d\n", channel);
            } else if (!cs_channel_diff) {
                printf("  error:channel_diff=%d\n", cs_channel_diff);
            } else {
                DBG_CHN5_HIGH; //test configuration consumption time start

                float fstep;
                if (cs_channel_diff == CS_CHANNEL_MAP_ENABLE_FLAG) {
                    fstep = 1e6 * 2;

                    foreach (i, channel) {
                        ipm[(cs_channel_idx[i + drop_num + 1] - (drop_num + 1)) * 2]     = initi_pct[2 * drop_num + i * 2];
                        ipm[(cs_channel_idx[i + drop_num + 1] - (drop_num + 1)) * 2 + 1] = initi_pct[2 * drop_num + i * 2 + 1];
                        pct[(cs_channel_idx[i + drop_num + 1] - (drop_num + 1)) * 2]     = refle_pct[2 * drop_num + i * 2];
                        pct[(cs_channel_idx[i + drop_num + 1] - (drop_num + 1)) * 2 + 1] = refle_pct[2 * drop_num + i * 2 + 1];
                        //                      ipm[(cs_channel_idx[i + 2] - 2) * 2] = initi_pct[2 + i * 2];
                        //                      ipm[(cs_channel_idx[i + 2] - 2) * 2 + 1] = initi_pct[2 + i * 2 + 1];
                        //                      pct[(cs_channel_idx[i + 2] - 2) * 2] = refle_pct[2 + i * 2];
                        //                      pct[(cs_channel_idx[i + 2] - 2) * 2 + 1] = refle_pct[2 + i * 2 + 1];
                    }

                    printf("initi_tone_sort_QI:");
                    foreach (i, channel * 2) {
                        printf("%d,", ipm[i]);
                    }
                    printf("\n");
                } else {
                    fstep = 1e6 * cs_channel_diff;

                    foreach (i, channel * 2) {
                        ipm[i] = initi_pct[i + 2 * drop_num];
                        pct[i] = refle_pct[i + 2 * drop_num];
                    }
                }
                parameterConstTes para = tesInit(channel, fstep);

                calcIpmPct(ipm, pct, cali, ipmpct, para);
                int ipmpctInMatlab[CHANNUM * 2] = {30146020, 156227700, 134747226, 82848742, 158901426, 47021348, 144297142, -79154570, 136412536, -89915432, -13535568, -161800976, -99283440, -127645652, -112387520, -115841280, -107487742, 119707886, -159155040, 18077158, -94687256, 128115928, -75694470, 139750636, 82432276, 133051288, 97454646, 121003562, 117470224, -99082352, 151796866, -10346302, -57361344, -139867008, 77525964, -139860540, 44723074, -152277862, -77710826, -138187764, -88794588, -130610170, -156766690, 13409224, -148091580, 51592660, -82813008, 132376910, -63297420, 140742560, 67864104, 137345592, 91204224, 121381104, 147965572, 26492744, 149373404, 6463796, 71599684, -129945452, 67362982, -131287032, 45508654, -139998178, -66244548, -141454964, -96666790, -122452330, -155197776, -2658680, -147060824, 46174428, 54409798, 143409964, -49072630, 143491544, -28428712, 147295420, 110432130, 99028858};

                //phase based ranging
                float T2WR[40];
                distPhaseDiff = tesPhase(ipmpct, T2WR, &(T2WRDiffMean), para);
                distMusic     = tesMusic(ipmpct, T2WR, T2WRDiffMean, para, &(likeliness), &(nIterMaxEig), &(nIterPS), &(nSigCnt), &(EVDCap));
                DBG_CHN5_LOW; //test configuration consumption time post

                printf("initiator distance\n");
                printf("  channel=%d,diff=%d\n", channel, cs_channel_diff);
                printf("  likeliness = %f\n", likeliness);
                printf("  realDistance = %f, phaseDiffDistance = %f, musicDistance = %f\n", distanceReal, distPhaseDiff, distMusic);
                printf("\n");
            }
        }
    }
}

/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
_attribute_no_inline_ void main_loop(void)
{
    main_idle_loop();

    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
    simple_sdp_loop();
    #endif

    distance_estimation_mode_2_loop();

    #if (DBG_ACL_PACKET_LOSS_RECORD_EN)
    static _attribute_data_retention_ u32 tick_aclPacketLoss = 0;
    if (ll_acl_connectEventNum && (ll_acl_connectEventNum % 1000 == 0)) {
        if (clock_time_exceed(tick_aclPacketLoss, 100 * 1000)) {
            tick_aclPacketLoss = clock_time();

            float packetLossRate;
            packetLossRate = (float)(ll_acl_connectEventNum - ll_acl_packetReceiveNum);
            packetLossRate = packetLossRate / (float)ll_acl_connectEventNum;
            tlkapi_printf(1, "ACL connectEventNum=%d,packetReceiveNum=%d, packetLossRate=%f\n", ll_acl_connectEventNum, ll_acl_packetReceiveNum, packetLossRate);
            printf("ACL connectEventNum=%d,packetReceiveNum=%d, packetLossRate=%f\n", ll_acl_connectEventNum, ll_acl_packetReceiveNum, packetLossRate);
        }
    }
    #endif
}

void initiator_Tone_PCT_test()
{
    int hadm_version = get_version();
    printf("[APP][HADM] hadm_version = %d.%d.%d\n", (((hadm_version) >> 16) & 0xFF), (((hadm_version) >> 8) & 0xFF), ((hadm_version) & 0xFF));
    printf("  initiator_Tone_PCT_test\n");

    #if (1) //if not defined, add initialization
    int                    Init_IQData[240 * 2 * 40];
    volatile unsigned int  init_tr_turnaround_time_pos[50] __attribute__((aligned(4)));
    volatile unsigned int  init_tr_turnaround_time_neg[50] __attribute__((aligned(4)));
    volatile unsigned int  initiator_iq_start_tstamp[50] __attribute__((aligned(4)));
    volatile unsigned int  initiator_iq_sync_tstamp[50] __attribute__((aligned(4)));
    volatile unsigned char Tone_qualityIndicator[40];
    #endif

    float cfo                    = -16636.466797;
    float angleStep              = 2 * PI * cfo / SAMPLERATE;
    float thresGood              = 286.0;
    float thresBad               = 2.25;
    float compArr[160 * 2]       = {0.0};
    float if_adjustment[CHANNUM] = {-61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500};
    //need to scan CHANNUM both at reflector and initiator side
    int ipm[CHANNUM * 2] = {0};
    //int pct[CHANNUM*2];//for reflector
    calcCompensate(compArr, 160, angleStep);

    int i     = 27;
    int IQLen = 160; //IQ sample num

    int IQ_data_20[320] = {-286535, -244768, -280475, -251324, -273791, -258392, -265991, -266435, -256773, -275202, -246542, -284024, -236326, -292249, -227268, -299261, -219695, -304949, -213200, -309781, -207184, -314320, -201007, -318717, -193975, -322937, -185818, -327117, -176973, -331482, -168108, -335980, -159428, -340390, -150705, -344651, -141533, -348809, -131419, -352748, -119920, -356354, -107252, -359821, -94472, -363259, -82663, -366361, -72042, -368780, -62130, -370543, -52542, -371933, -43274, -373102, -34821, -374041, -27673, -374830, -22043, -375500, -17179, -375795, -11411, -375578, -3348, -375073, 6961, -374706, 18266, -374460, 29168, -374067, 38996, -373339, 47722, -372251, 55907, -370659, 64143, -368670, 72590, -366699, 80796, -365132, 88577, -363696, 96415, -361882, 105021, -359474, 114481, -356577, 124317, -353307, 133971, -349855, 143031, -346463, 151342, -343221, 159208, -339688, 167373, -335440, 176606, -330306, 187036, -324373, 198294, -317573, 209743, -310012, 220701, -302207, 230446, -294880, 238605, -288309, 245479, -282543, 251606, -277519, 257224, -273070, 262389, -268682, 267394, -263825, 272658, -258217, 278339, -251808, 284455, -244325, 290977, -235748, 297741, -226583, 304206, -217772, 309849, -209907, 314543, -203100, 318596, -197098, 322286, -191361, 325853, -185180, 329563, -178272, 333518, -171044, 337388, -164118, 340807, -157578, 343807, -150952, 346803, -143693, 350018, -135276, 353458, -125404, 357026, -114200, 360646, -102111, 363971, -89393, 366743, -75816, 369075, -61115, 371198, -45729, 372995, -30605, 374185, -16518, 374689, -3729, 374665, 7875, 374238, 18627, 373608, 28777, 373051, 38256, 372690, 46541, 372155, 53427, 371074, 59438, 369564, 65452, 368073, 71868, 366658, 78732, 365046, 86230, 362926, 94771, 360199, 104444, 356896, 114837, 353343, 125057, 350094, 134186, 347559, 141694, 345314, 148022, 342588, 154426, 338723, 162240, 333579, 171785, 327268, 182457, 320385, 193298, 313858, 203479, 308294, 212443, 303242, 220347, 297640, 228024, 290594, 236489, 282145, 245984, 272972, 255850, 264165, 265105, 256601, 273052, 250292, 279469, 244177, 284899, 237267, 290315, 229360, 296372, 220963, 302863, 212410, 309163, 203646, 314996, 194540, 320503, 185158, 325793, 175717, 330792, 166631, 335463, 158094, 339840, 149683, 343824, 140355, 347417, 129377, 350894, 117187, 354480, 105082, 357979, 94188, 360969, 84898, 363311, 77090, 365207, 70178, 366805, 63347, 368212, 55957, 369681, 47809, 371309, 38772, 372780, 28776, 373783, 18055, 374343, 7388, 374713, -2641, 374797, -12088, 374423, -21585, 373585, -31462, 372546, -41693, 371411, -51997, 370242, -62214, 369065};
    for (int j = 0; j < IQLen; j++) {
        Init_IQData[MAXTESLEN * 2 * i + j * 2]     = IQ_data_20[j * 2];
        Init_IQData[MAXTESLEN * 2 * i + j * 2 + 1] = IQ_data_20[j * 2 + 1];
    }

    initiator_iq_start_tstamp[0]   = 34317;
    init_tr_turnaround_time_pos[i] = 323138;
    init_tr_turnaround_time_neg[i] = 328100;
    initiator_iq_start_tstamp[i]   = 331236;
    Tone_qualityIndicator[i]       = 0xFF;

    Tone_qualityIndicator[i] = calcTesInfo(&Init_IQData[MAXTESLEN * 2 * i], compArr, 160, init_tr_turnaround_time_neg[i] - initiator_iq_start_tstamp[0], initiator_iq_start_tstamp[i] - initiator_iq_start_tstamp[0], cfo, if_adjustment[i], &(ipm[2 * i]), thresGood, thresBad);

    int PCT_I_uncompress = ipm[2 * i];
    int PCT_Q_uncompress = ipm[2 * i + 1];

    compressTesInfo(ipm, CHANNUM * 2, 15);

    printf("[APP][HADM] test\n");
    printf("  channel[%d] = %d\n", i, (i * 2 + 2402));
    printf("  PCT_I_uncompress[%d] = %d\n", i, PCT_I_uncompress);
    printf("  PCT_Q_uncompress[%d] = %d\n", i, PCT_Q_uncompress);
    printf("  imp_I[%d] = %d\n", i, ipm[2 * i]);
    printf("  imp_Q[%d] = %d\n", i, ipm[2 * i + 1]);
    printf("  init_tr_turnaround_time_pos[%d] = %d\n", i, init_tr_turnaround_time_pos[i]);
    printf("  init_tr_turnaround_time_neg[%d] = %d\n", i, init_tr_turnaround_time_neg[i]);
    printf("  initiator_iq_start_tstamp[%d] = %d\n", i, initiator_iq_start_tstamp[i]);
    printf("  initiator_iq_start_tstamp[0] = %d\n", initiator_iq_start_tstamp[0]);
    printf("  Tone_qualityIndicatorInit = %d\n", Tone_qualityIndicator[i]);
    printf("Init_IQData\n");

    for (int j = 0; j < MAXTESLEN * 2; j++) {
        printf("%8d ", Init_IQData[i * (MAXTESLEN * 2) + j]);
        if ((i * (MAXTESLEN * 2) + j + 1) % 16 == 0) {
            printf("\n");
        }
    }

    /*
     * @ Print Log
        [APP][HADM] hadm_version = 1.1.18
            [APP][HADM] test
              channel[27] = 2456
              PCT_I_uncompress[27] = -48830796
              PCT_Q_uncompress[27] = -34816856
              imp_I[27] = -11922    //compress to 15Bit
              imp_Q[27] = -8501     //compress to 15Bit
              init_tr_turnaround_time_pos[27] = 323138
              init_tr_turnaround_time_neg[27] = 328100
              initiator_iq_start_tstamp[27] = 331236
              initiator_iq_start_tstamp[0] = 34317
              Tone_qualityIndicatorInit = 0
     */
}

void initiator_Tone_PCT_test_2()
{
    int hadm_version = get_version();
    printf("[APP][HADM] hadm_version = %d.%d.%d\n", (((hadm_version) >> 16) & 0xFF), (((hadm_version) >> 8) & 0xFF), ((hadm_version) & 0xFF));
    printf("  initiator_Tone_PCT_test_2\n");

    int IQLen = 160; //IQ sample num, 40us * 4 = 160

    int           Init_IQData[LL_CS_STEP_IQ_NUM_MAX];
    unsigned int  init_tr_turnaround_time_pos;
    unsigned int  init_tr_turnaround_time_neg;
    unsigned int  initiator_iq_start_tstamp;
    unsigned int  initiator_cs_start_tstamp;
    unsigned char Tone_qualityIndicator;

    float cfo                            = -16636.466797;
    float angleStep                      = 2 * PI * cfo / SAMPLERATE;
    float thresGood                      = 286.0;
    float thresBad                       = 2.25;
    float compArr[LL_CS_STEP_IQ_NUM_MAX] = {0.0};
    float if_adjustment[CHANNUM]         = {-61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500, -61.0351562500, 122.0703125000, -61.0351562500};
    //need to scan CHANNUM both at reflector and initiator side
    int ipm[2] = {0}; //for initiator
    //int pct[2] = {0};//for reflector
    calcCompensate(compArr, IQLen, angleStep);

    int i = 27;

    int IQ_data_20[320] = {-286535, -244768, -280475, -251324, -273791, -258392, -265991, -266435, -256773, -275202, -246542, -284024, -236326, -292249, -227268, -299261, -219695, -304949, -213200, -309781, -207184, -314320, -201007, -318717, -193975, -322937, -185818, -327117, -176973, -331482, -168108, -335980, -159428, -340390, -150705, -344651, -141533, -348809, -131419, -352748, -119920, -356354, -107252, -359821, -94472, -363259, -82663, -366361, -72042, -368780, -62130, -370543, -52542, -371933, -43274, -373102, -34821, -374041, -27673, -374830, -22043, -375500, -17179, -375795, -11411, -375578, -3348, -375073, 6961, -374706, 18266, -374460, 29168, -374067, 38996, -373339, 47722, -372251, 55907, -370659, 64143, -368670, 72590, -366699, 80796, -365132, 88577, -363696, 96415, -361882, 105021, -359474, 114481, -356577, 124317, -353307, 133971, -349855, 143031, -346463, 151342, -343221, 159208, -339688, 167373, -335440, 176606, -330306, 187036, -324373, 198294, -317573, 209743, -310012, 220701, -302207, 230446, -294880, 238605, -288309, 245479, -282543, 251606, -277519, 257224, -273070, 262389, -268682, 267394, -263825, 272658, -258217, 278339, -251808, 284455, -244325, 290977, -235748, 297741, -226583, 304206, -217772, 309849, -209907, 314543, -203100, 318596, -197098, 322286, -191361, 325853, -185180, 329563, -178272, 333518, -171044, 337388, -164118, 340807, -157578, 343807, -150952, 346803, -143693, 350018, -135276, 353458, -125404, 357026, -114200, 360646, -102111, 363971, -89393, 366743, -75816, 369075, -61115, 371198, -45729, 372995, -30605, 374185, -16518, 374689, -3729, 374665, 7875, 374238, 18627, 373608, 28777, 373051, 38256, 372690, 46541, 372155, 53427, 371074, 59438, 369564, 65452, 368073, 71868, 366658, 78732, 365046, 86230, 362926, 94771, 360199, 104444, 356896, 114837, 353343, 125057, 350094, 134186, 347559, 141694, 345314, 148022, 342588, 154426, 338723, 162240, 333579, 171785, 327268, 182457, 320385, 193298, 313858, 203479, 308294, 212443, 303242, 220347, 297640, 228024, 290594, 236489, 282145, 245984, 272972, 255850, 264165, 265105, 256601, 273052, 250292, 279469, 244177, 284899, 237267, 290315, 229360, 296372, 220963, 302863, 212410, 309163, 203646, 314996, 194540, 320503, 185158, 325793, 175717, 330792, 166631, 335463, 158094, 339840, 149683, 343824, 140355, 347417, 129377, 350894, 117187, 354480, 105082, 357979, 94188, 360969, 84898, 363311, 77090, 365207, 70178, 366805, 63347, 368212, 55957, 369681, 47809, 371309, 38772, 372780, 28776, 373783, 18055, 374343, 7388, 374713, -2641, 374797, -12088, 374423, -21585, 373585, -31462, 372546, -41693, 371411, -51997, 370242, -62214, 369065};
    for (int j = 0; j < IQLen; j++) {
        Init_IQData[j * 2]     = IQ_data_20[j * 2];
        Init_IQData[j * 2 + 1] = IQ_data_20[j * 2 + 1];
    }

    initiator_cs_start_tstamp   = 34317;
    init_tr_turnaround_time_pos = 323138;
    init_tr_turnaround_time_neg = 328100;
    initiator_iq_start_tstamp   = 331236;
    Tone_qualityIndicator       = 0xFF;

    Tone_qualityIndicator = calcTesInfo(&Init_IQData[0], compArr, IQLen, init_tr_turnaround_time_neg - initiator_cs_start_tstamp, initiator_iq_start_tstamp - initiator_cs_start_tstamp, cfo, if_adjustment[i], &(ipm[0]), thresGood, thresBad);

    int PCT_I_uncompress = ipm[0];
    int PCT_Q_uncompress = ipm[1];

    compressTesInfo(ipm, 2, 15);

    printf("[APP][HADM] test\n");
    printf("  channel[%d] = %d\n", i, (i * 2 + 2402));
    printf("  PCT_I_uncompress[%d] = %d\n", i, PCT_I_uncompress);
    printf("  PCT_Q_uncompress[%d] = %d\n", i, PCT_Q_uncompress);
    printf("  imp_I[%d] = %d\n", i, ipm[0]);
    printf("  imp_Q[%d] = %d\n", i, ipm[1]);
    printf("  init_tr_turnaround_time_pos[%d] = %d\n", i, init_tr_turnaround_time_pos);
    printf("  init_tr_turnaround_time_neg[%d] = %d\n", i, init_tr_turnaround_time_neg);
    printf("  initiator_iq_start_tstamp[%d] = %d\n", i, initiator_iq_start_tstamp);
    printf("  initiator_cs_start_tstamp = %d\n", initiator_cs_start_tstamp);
    printf("  Tone_qualityIndicatorInit = %d\n", Tone_qualityIndicator);
    printf("Init_IQData\n");

    for (int j = 0; j < IQLen * 2; j++) {
        printf("%8d ", Init_IQData[j]);
        if ((j + 1) % 16 == 0) {
            printf("\n");
        }
    }

    /*
     * @ Print Log
        [APP][HADM] hadm_version = 1.1.18
        [APP][HADM] hadm_version = 1.1.23
        [APP][HADM] hadm_version = 1.1.24
        [APP][HADM] hadm_version = 1.1.25
            [APP][HADM] test
              channel[27] = 2456
              PCT_I_uncompress[27] = -48830796
              PCT_Q_uncompress[27] = -34816856
              imp_I[27] = -11922    //compress to 15Bit
              imp_Q[27] = -8501     //compress to 15Bit
              init_tr_turnaround_time_pos[27] = 323138
              init_tr_turnaround_time_neg[27] = 328100
              initiator_iq_start_tstamp[27] = 331236
              initiator_cs_start_tstamp = 34317
              Tone_qualityIndicatorInit = 0
     */
}

void distance_estimation_RTT_test()
{
    int hadm_version = get_version();
    printf("[APP][HADM] hadm_version = %d.%d.%d\n", (((hadm_version) >> 16) & 0xFF), (((hadm_version) >> 8) & 0xFF), ((hadm_version) & 0xFF));
    printf("  distance_estimation_RTT_test\n");

    int nAverage = 40;
    //aaCode:8e,89,be,d6
    int                         aaCode[32] = {0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1};
    parameterPesCalcDistanceSDK paraPesSDK = pesCalcDistanceInitSDK(nAverage);

    printf("start RTT CalcDistance!\n");

    short cte_initiator_example[40] = {6559, 6809, 6809, 6809, 6809, 6809, 6559, 6809, 6559, 6559, 6559, 6309, 6309, 6559, 6309, 6559, 6309, 6559, 6309, 6309, 5809, 6059, 6059, 6059, 6059, 5809, 5809, 5559, 5559, 5559, 5809, 5309, 5559, 5309, 5059, 5559, 5309, 5059, 5059, 5059};
    short cte_reflector_example[40] = {6750, 6500, 6750, 6750, 6750, 6750, 6500, 6750, 6500, 6500, 6500, 6250, 6250, 6500, 6250, 6500, 6250, 6500, 6500, 6000, 5750, 6000, 6000, 6000, 6000, 5750, 5500, 5750, 5500, 5500, 5500, 5500, 5500, 5250, 5000, 5250, 5250, 5250, 5000, 5250};
    //  short cte_initiator_example[40] = {6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550,6550};
    //  short cte_reflector_example[40] = {6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500,6500};

    int cte_initiator_flag[40] = {0};
    int cte_reflector_flag[40] = {0};
    int cte_flag[40]           = {0};
    for (int i = 0; i < 40; i++) {
        cte_initiator_flag[i] = 1;
        cte_reflector_flag[i] = 1;
        cte_flag[i]           = cte_initiator_flag[i] * cte_reflector_flag[i];
    }

    float distPesSync1[40] = {0};
    float distPesSync      = pesCalcDistSDK(cte_initiator_example, cte_reflector_example, cte_flag, distPesSync1, paraPesSDK);
    printf("distPesSync = %f, matla:3.953513039874999\n", distPesSync);
    //  printf("distPesSync = %f, theoretical:3.75\n", distPesSync);//for 6550 <---> 6500

    for (int j = 0; j < 40; j++) {
        printf("%8d ", distPesSync1[j]);
        if ((j + 1) % 16 == 0) {
            printf("\n");
        }
    }

    /*
     * @ Print Log
        [APP][HADM] hadm_version = 1.1.18
        [APP][HADM] hadm_version = 1.1.23
        [APP][HADM] hadm_version = 1.1.24
        [APP][HADM] hadm_version = 1.1.25
            start RTT CalcDistance!
            distPesSync = 3.953512, matla:3.953513039874999
                -1070816941 1077356722 1074901008 1074901008 1074901008 1074901008 1074901008 1074901008 1074901008 1074901008 1074901008 1074901008 1074901008 1074901008 1074901008 1074901008
                1074901008 1074901008 -1070816941 1077356722 1074901008 1074901008 1074901008 1074901008 1074901008 1074901008 1077356722 -1070816941 1074901008 1074901008 1077356722 -1070816941
                1074901008 1074901008 1074901008 1077356722 1074901008 -1070816941 1074901008 -1070816941
     */

    /*
     * @ Print Log
        [APP][HADM] hadm_version = 1.1.18
            start RTT CalcDistance!
            distPesSync = 3.747406, theoretical:3.75
                1074657967 1074657967 1074657967 1074657967 1074657967 1074657967 1074657967 1074657967 1074657967 1074657967 1074657967 1074657967 1074657967 1074657967 1074657967 1074657967
                1074657967 1074657967 1074657967 1074657967 1074657967 1074657967 1074657967 1074657967 1074657967 1074657967 1074657967 1074657967 1074657967 1074657967 1074657967 1074657967
                1074657967 1074657967 1074657967 1074657967 1074657967 1074657967 1074657967 1074657967
     */
}

void estimation_calcPesInfoSDK_test()
{
    int hadm_version = get_version();
    printf("[APP][HADM] hadm_version = %d.%d.%d\n", (((hadm_version) >> 16) & 0xFF), (((hadm_version) >> 8) & 0xFF), ((hadm_version) & 0xFF));
    printf("  estimation_calcPesInfoSDK_test\n");

    int nAverage = 40;
    //aaCode:8e,89,be,d6
    int aaCode[32] = {0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1};
    int dataRate   = 1e6; // for 1M case now
    int offset     = 20;  //skip first offset symbol.

    float adThr[2]         = {0.15, 0.8};
    float adStep[2]        = {-0.01, 0.01};
    int   internalDelay[2] = {325441, 80000}; //1 for initiator, 2 for reflector

    printf("  INITIATOR\n");
    //ROLE, 0 for initiator, 1 for reflector
    parameterPesCollectDataSDK paraPesSDK = pesCollectDataInitSDK(nAverage, 0, dataRate, aaCode, 32, internalDelay, adThr, adStep);

    int   tx_timestamp1[40]      = {29067, 43158, 57249, 71340, 85434, 99525, 113619, 127710, 141804, 155895, 169986, 184080, 198174, 212265, 226359, 240450, 254544, 268635, 282729, 296823, 310917, 325008, 339102, 353193, 367287, 381378, 395472, 409566, 423657, 437751, 451842, 465936, 480027, 494121, 508215, 522306, 536400, 550491, 564585, 578676};
    int   iq_sync_tstamp1[40]    = {37659, 51753, 65844, 79935, 94029, 108120, 122211, 136305, 150396, 164487, 178578, 192669, 206763, 220857, 234948, 249042, 263133, 277227, 291318, 305412, 319500, 333594, 347688, 361779, 375873, 389961, 404055, 418146, 432237, 446331, 460425, 474513, 488607, 502698, 516789, 530886, 544977, 559065, 573159, 587250};
    short cte_sync1_matlab[40]   = {6559, 6809, 6809, 6809, 6809, 6809, 6559, 6809, 6559, 6559, 6559, 6309, 6309, 6559, 6309, 6559, 6309, 6559, 6309, 6309, 5809, 6059, 6059, 6059, 6059, 5809, 5809, 5559, 5559, 5559, 5809, 5309, 5559, 5309, 5059, 5559, 5309, 5059, 5059, 5059};
    int   T_SY_CENTER_DELTA_INIT = 384000; //(2+40+150)*1e-6/0.5e-9

    short cte_sync1[40] = {0};
    calcPesInfoSDK(tx_timestamp1, iq_sync_tstamp1, T_SY_CENTER_DELTA_INIT, cte_sync1, paraPesSDK);

    for (int i = 0; i < paraPesSDK.n; i++) {
        printf("    %hd,%hd,%hd,%hd\n", i, cte_sync1[i], cte_sync1_matlab[i], cte_sync1[i] - cte_sync1_matlab[i]);
        cte_sync1[i] = 0;
    }

    printf("\n");
    printf("  REFLECTOR\n");
    parameterPesCollectDataSDK paraPesSDK_refl = pesCollectDataInitSDK(nAverage, 1, dataRate, aaCode, 32, internalDelay, adThr, adStep);

    int   tx_timestamp2[40]      = {36633, 50724, 64815, 78909, 93000, 107091, 121182, 135276, 149367, 163458, 177549, 191640, 205734, 219828, 233919, 248013, 262104, 276198, 290289, 304380, 318471, 332565, 346656, 360750, 374841, 388932, 403023, 417117, 431208, 445302, 459393, 473484, 487575, 501669, 515760, 529854, 543945, 558036, 572127, 586221};
    int   iq_sync_tstamp2[40]    = {30984, 45078, 59166, 73260, 87351, 101442, 115536, 129627, 143721, 157812, 171903, 185997, 200091, 214182, 228276, 242367, 256461, 270552, 284643, 298740, 312834, 326925, 341016, 355110, 369201, 383295, 397389, 411480, 425574, 439668, 453759, 467850, 481941, 496038, 510132, 524223, 538314, 552405, 566499, 580590};
    short cte_sync2_matlab[40]   = {6750, 6500, 6750, 6750, 6750, 6750, 6500, 6750, 6500, 6500, 6500, 6250, 6250, 6500, 6250, 6500, 6250, 6500, 6500, 6000, 5750, 6000, 6000, 6000, 6000, 5750, 5500, 5750, 5500, 5500, 5500, 5500, 5500, 5250, 5000, 5250, 5250, 5250, 5000, 5250};
    int   T_SY_CENTER_DELTA_REFL = 384000; //(2+40+150)*1e-6/0.5e-9

    short cte_sync2[40] = {0};
    calcPesInfoSDK(tx_timestamp2, iq_sync_tstamp2, T_SY_CENTER_DELTA_REFL, cte_sync2, paraPesSDK_refl);

    for (int i = 0; i < paraPesSDK_refl.n; i++) {
        printf("    %hd,%hd,%hd,%hd\n", i, cte_sync2[i], cte_sync2_matlab[i], cte_sync2[i] - cte_sync2_matlab[i]);
        cte_sync2[i] = 0;
    }

    printf("\n");
    printf("  single step INITIATOR\n");
    /**
     *  First param: number of step
     */
    for (int i = 0; i < 40; i++) {
        parameterPesCollectDataSDK paraPesSDK_single = pesCollectDataInitSDK(1, 0, dataRate, aaCode, 32, internalDelay, adThr, adStep);
        calcPesInfoSDK(&tx_timestamp1[i], &iq_sync_tstamp1[i], T_SY_CENTER_DELTA_INIT, &cte_sync1[i], paraPesSDK_single);
        printf("    %hd,%hd,%hd,%hd\n", i, cte_sync1[i], cte_sync1_matlab[i], cte_sync1[i] - cte_sync1_matlab[i]);
        cte_sync1[i] = 0;
    }

    printf("\n");
    printf("  single step INITIATOR, increase data\n");
    u32   t_sy_center_delta_init = 194 * 2 * 1000; //(44+5+145)*1e-6/0.5e-9
    u32   tx_on_start_tstamp     = 0;
    u32   rx_pkt_iq_sync_tstamp  = 6000;
    short cte_initiator          = 6550;           //for test

    float cfo             = 11994.81;
    int   IQLen           = 240;
    int   IQData[240 * 2] = {12891, 32832, -393137, 36620, 8192, 1, -472159, 331776, 2272, -222207, 131080, 36357, 241664, 141, -466862, 311296, 88, 131584, 16, 237152, 30435, 219804, -28086, 202461, -88353, 194636, -111316, 197569, -79828, 206148, -8900, 213473, 64223, 215026, 101402, 210634, 85154, 204627, 29427, 202661, -28231, 206499, -49418, 211703, -18236, 211203, 50086, 201581, 119005, 186457, 151549, 174460, 131406, 173881, 73262, 186555, 14275, 205169, -8667, 216905, 18815, 212129, 81293, 191406, 143945, 165669, 172750, 149641, 153400, 153143, 99956, 174657, 46675, 201045, 27020, 214622, 53331, 204466, 110747, 173829, 167163, 138867, 191991, 118984, 173070, 125475, 124066, 154463, 76554, 187943, 60472, 203661, 85803, 189012, 137942, 149524, 187857, 106177, 208930, 83054, 191367, 93282, 147933, 130938, 106607, 173077, 93191, 192457, 115485, 174480, 159990, 126864, 201455, 75403, 217757, 48670, 201619, 61295, 164795, 105049, 131170, 152534, 121706, 172823, 141549, 150525, 178316, 95953, 211182, 39027, 223149, 11170, 209884, 27134, 181935, 76363, 157945, 128344, 152347, 150461, 166941, 127725, 192277, 72080, 215357, 14162, 226009, -15180, 219233, -1175, 194856, 49836, 154673, 115727, 99562, 171610, 30736, 200753, -45966, 198360, -117841, 166990, -171073, 112344, -198981, 43608, -205751, -24450, -202825, -73235, -201605, -86968, -206077, -59292, -208756, 3187, -194376, 82324, -150070, 155248, -75022, 203152, 18188, 215882, 109123, 191322, 177816, 133706, 213016, 54392, 217339, -27703, 205524, -89861, 194441, -113527, 192456, -91146, 194927, -30448, 187858, 48240, 157664, 120569, 101253, 170654, 32690, 196961, -21535, 207704, -36519, 211226, -3742, 208785, 64023, 193763, 139606, 155912, 194596, 89508, 212704, 1407, 197555, -87602, 168788, -151684, 148913, -172634, 149960, -149425, 169515, -99607, 194944, -52763, 210606, -36318, 204025, -61357, 169782, -116773, 109765, -176687, 30573, -214292, -57492, -213508, -139692, -172292, -198175, -100455, -219528, -13431, -202321, 73113, -159654, 145446, -114283, 191287, -89519, 202533, -97825, 181133, -132958, 142168, -172007, 108011, -189609, 97073, -173622, 115162, -132222, 153165, -88350, 191582, -66652, 209856, -79724, 196637, -120516, 152958, -166982, 87981, -195222, 12739, -191632, -62464, -155580, -128047, -95482, -175708, -23488, -199933, 48587, -198358, 111978, -171571, 160965, -122828, 190935, -57813, 197800, 15816, 179610, 88275, 137571, 147754, 75512, 183611, 1102, 192674, -71601, 182842, -122845, 169165, -135490, 165020, -105466, 175210, -46982, 194091, 11874, 209975, 41426, 213301, 26787, 203754, -21511, 190006, -71995, 181864, -91597, 182719, -65960, 188860, -7101, 194732, 54274, 197303, 85728, 197121, 69965, 195749, 13120, 191365, -61298, 176182, -128082, 140654, -173244, 82874, -195925, 15779, -202831, -36480, -201600, -51141, -196827, -20735, -189469, 40282, -180090, 100353, -172612, 127677, -173066, 109170, -183348, 59094, -197466, 9823, -204862, -7959, -198172, 17533, -178449, 73754, -155738, 131089, -143851, 158952, -150956, 142081, -171259, 86307, -186695, 10567, -178706, -65733, -141771, -129751, -88119, -175403, -42267, -200780, -28066, -205773, -54487, -191496, -107983, -162813, -159173, -131648, -180351, -114193, -161965, -121064, -117036, -148614, -73287, -179972, -57651, -194822, -80927, -181295, -130648, -143478, -178287, -101011, -197397, -78406, -179514, -89023, -137829, -125680, -98321, -165262, -85080, -183028, -106646, -166481, -150671, -122332, -191471, -74253, -206819, -49779, -191313, -62771, -157793, -103946, -127791, -146898, -118927, -164583, -135907, -144199, -168755, -94420, -199628, -41575, -213387, -15661, -203996, -32004, -172457, -82049, -121969, -140261, -56608, -180910, 16415, -191420, 84897, -177102, 132461, -156184, 143970, -147939, 114360, -159915, 56549, -184120, -904, -205006, -28025, -211381, -11097, -201884, 38954, -183877};
    //calcPesNadm use nadm adtype = [1,2];
    int   adtype = 2;
    float rdm    = 0.0; // Detector Metrics

    for (int i = 0; i < 40; i++) {
        /**
         *  First param: number of step
         */
        DBG_CHN5_HIGH;     //test hadm algorithm time start
        parameterPesCollectDataSDK paraPesSDK_increase = pesCollectDataInitSDK(1, 0, dataRate, aaCode, 32, internalDelay, adThr, adStep);
        DBG_CHN5_LOW;      //test hadm algorithm time post

        DBG_CHN5_HIGH;     //test hadm algorithm time start
        calcPesInfoSDK(&tx_on_start_tstamp, &rx_pkt_iq_sync_tstamp, t_sy_center_delta_init, &cte_initiator, paraPesSDK_increase);
        DBG_CHN5_LOW;      //test hadm algorithm time post

        DBG_CHN5_HIGH;     //test hadm algorithm time start
        int pkIdx     = calcPesIdx(dataRate, aaCode, 32, offset, IQData, IQLen, cfo, paraPesSDK_increase);
        int searchWin = 3; //for adtype = 2, searchWin = 3 due to mismatch, for adtype = 1, search = 1;
        int nadm      = calcPesNadm(IQData, IQLen, cfo, adtype, pkIdx, searchWin, &rdm, paraPesSDK_increase);
        DBG_CHN5_LOW;      //test hadm algorithm time post

        printf("    %hd,%hd,%hd,%hd\n", i, tx_on_start_tstamp, rx_pkt_iq_sync_tstamp, cte_initiator);
        //printf("rdm = %f, nadm = 0.043694 if adtype == 1, nadm = 0.991918951314286 if adtype == 2\n", rdm);
        printf("        nadm = %d, 0 for signal\n", nadm);

        rx_pkt_iq_sync_tstamp += 10;
    }

    /*
     * @ Print Log
        [APP][HADM] hadm_version = 1.1.18
        [APP][HADM] hadm_version = 1.1.23//algorithm library has print in calcPesInfoSDK()
        [APP][HADM] hadm_version = 1.1.24
        [APP][HADM] hadm_version = 1.1.25
        [APP][HADM] hadm_version = 1.1.27
     */
}

void distance_estimation_Phase_test()
{
    int hadm_version = get_version();
    printf("[APP][HADM] hadm_version = %d.%d.%d\n", (((hadm_version) >> 16) & 0xFF), (((hadm_version) >> 8) & 0xFF), ((hadm_version) & 0xFF));
    printf("  distance_estimation_Phase_test\n");

    float   distanceReal = 5.8769;
    float   distPhaseDiff, distMusic, likeliness, EVDCap, T2WRDiffMean;
    int     nIterMaxEig, nIterPS, nSigCnt;
    complex ipmpct[40];
    complex tmp;
    //result of 0m, conj(I*Q)
    //int cali[40*2] = {1024,0,958,-361,816,-619,786,-656,940,-406,923,-443,1024,-14,824,-608,915,-460,914,-462,935,418,846,-578,797,642,1021,81,1022,65,1011,-160,825,-607,1024,-3,844,-579,1004,202,845,-578,881,522,995,-243,925,439,955,-370,400,943,1019,101,1015,133,998,229,909,471,908,474,1004,204,833,596,1002,-213,1003,209,971,324,965,343,982,290,931,-426,797,643};
    int cali[40 * 2] = {1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0};

    //result of 4m, ipm is I from tesCollectData, pct is Q from  tesCollectData
    int ipm[40 * 2] = {-1570, -8805, 5629, 6907, -5518, 7261, 6602, 6256, 4037, 8119, 6191, 6575, -1401, -8912, 1593, -8855, -6094, 6598, 1994, 8735, 8412, -3008, 7747, -4401, -7028, -5391, 8804, 593, 8767, 169, 3567, 7986, -4721, 7331, -7806, 4424, 8931, 454, 8913, 640, -7509, 4778, -6427, 6128, 8857, -336, 8301, 3041, -627, -8770, -4625, 7433, -5678, 6624, -5291, 6890, -8253, -2662, 7964, 3357, 8247, -2442, 8571, -565, -2279, -8559, 8781, -954, -6912, -5470, -8735, -747, -8744, -209, -5838, -6470, 6942, -5159, -7601, -4063};
    int pct[40 * 2] = {-8894, 126, 8876, 219, 2570, -8713, 7645, -4843, 2545, -8644, -3923, -8091, 7901, -4229, 8534, -2758, 8057, 3909, -4893, 7480, -4732, 7556, -8906, -371, -8401, 2749, 6462, 5977, 6208, -6184, 4240, -7599, -8208, 2784, -7614, 4629, 6589, -5951, -6254, -6291, -4785, 7474, 8770, 1247, -8830, 510, 2059, 8590, -5700, -6670, -5042, -7147, 1032, -8640, -4944, -7103, -7588, 4090, -3132, -7994, 2377, -8240, 1531, -8427, 6150, -6316, -3031, -8292, 5756, -6656, 6659, -5743, -5833, -6544, -6207, -6098, -8255, 2661, -7789, 3655};

    int               channum = 40;
    float             fstep   = 2e6;
    parameterConstTes para    = tesInit(channum, fstep);

    calcIpmPct(ipm, pct, cali, ipmpct, para);
    int ipmpctInMatlab[CHANNUM * 2] = {30146020, 156227700, 134747226, 82848742, 158901426, 47021348, 144297142, -79154570, 136412536, -89915432, -13535568, -161800976, -99283440, -127645652, -112387520, -115841280, -107487742, 119707886, -159155040, 18077158, -94687256, 128115928, -75694470, 139750636, 82432276, 133051288, 97454646, 121003562, 117470224, -99082352, 151796866, -10346302, -57361344, -139867008, 77525964, -139860540, 44723074, -152277862, -77710826, -138187764, -88794588, -130610170, -156766690, 13409224, -148091580, 51592660, -82813008, 132376910, -63297420, 140742560, 67864104, 137345592, 91204224, 121381104, 147965572, 26492744, 149373404, 6463796, 71599684, -129945452, 67362982, -131287032, 45508654, -139998178, -66244548, -141454964, -96666790, -122452330, -155197776, -2658680, -147060824, 46174428, 54409798, 143409964, -49072630, 143491544, -28428712, 147295420, 110432130, 99028858};

    //phase based ranging
    float T2WR[40];
    distPhaseDiff = tesPhase(ipmpct, T2WR, &(T2WRDiffMean), para);
    distMusic     = tesMusic(ipmpct, T2WR, T2WRDiffMean, para, &(likeliness), &(nIterMaxEig), &(nIterPS), &(nSigCnt), &(EVDCap));

    printf("channum = %d\n", channum);
    int mark = 1;
    for (int i = 0; i < channum; i++) {
        if ((ipmpct[i].real != ipmpctInMatlab[2 * i]) || (ipmpct[i].imag != ipmpctInMatlab[2 * i + 1])) {
            printf("  %d,%d,%d,%d,%d\n", i, ipmpct[i].real, ipmpct[i].imag, ipmpctInMatlab[2 * i], ipmpctInMatlab[2 * i + 1]);
            mark = 0;
        }
    }
    if (mark) {
        printf("  calcIpmPct done!\n");
    }
    printf("  likeliness = %f\n", likeliness);
    printf("  realDistance = %f, phaseDiffDistance = %f, musicDistance = %f\n", distanceReal, distPhaseDiff, distMusic);
    printf("\n");
    printf("\n");

    channum = 10;
    para    = tesInit(channum, fstep);
    calcIpmPct(ipm, pct, cali, ipmpct, para);
    //phase based ranging
    distPhaseDiff = tesPhase(ipmpct, T2WR, &(T2WRDiffMean), para);
    distMusic     = tesMusic(ipmpct, T2WR, T2WRDiffMean, para, &(likeliness), &(nIterMaxEig), &(nIterPS), &(nSigCnt), &(EVDCap));
    printf("channum = %d\n", channum);
    mark = 1;
    for (int i = 0; i < channum; i++) {
        if ((ipmpct[i].real != ipmpctInMatlab[2 * i]) || (ipmpct[i].imag != ipmpctInMatlab[2 * i + 1])) {
            printf("  %d,%d,%d,%d,%d\n", i, ipmpct[i].real, ipmpct[i].imag, ipmpctInMatlab[2 * i], ipmpctInMatlab[2 * i + 1]);
            mark = 0;
        }
    }
    if (mark) {
        printf("  calcIpmPct done!\n");
    }
    printf("  likeliness = %f\n", likeliness);
    printf("  realDistance = %f, phaseDiffDistance = %f, musicDistance = %f\n", distanceReal, distPhaseDiff, distMusic);
    printf("\n");
    printf("\n");

    channum = 4;
    para    = tesInit(channum, fstep);
    calcIpmPct(ipm, pct, cali, ipmpct, para);
    //phase based ranging
    distPhaseDiff = tesPhase(ipmpct, T2WR, &(T2WRDiffMean), para);
    distMusic     = tesMusic(ipmpct, T2WR, T2WRDiffMean, para, &(likeliness), &(nIterMaxEig), &(nIterPS), &(nSigCnt), &(EVDCap));
    printf("channum = %d\n", channum);
    mark = 1;
    for (int i = 0; i < channum; i++) {
        if ((ipmpct[i].real != ipmpctInMatlab[2 * i]) || (ipmpct[i].imag != ipmpctInMatlab[2 * i + 1])) {
            printf("  %d,%d,%d,%d,%d\n", i, ipmpct[i].real, ipmpct[i].imag, ipmpctInMatlab[2 * i], ipmpctInMatlab[2 * i + 1]);
            mark = 0;
        }
    }
    if (mark) {
        printf("  calcIpmPct done!\n");
    }
    printf("  likeliness = %f\n", likeliness);
    printf("  realDistance = %f, phaseDiffDistance = %f, musicDistance = %f\n", distanceReal, distPhaseDiff, distMusic);
    printf("\n");
    printf("\n");

    /*
     * @ Print Log
        [APP][HADM] hadm_version = 1.1.27
     */
}

void distance_estimation_Phase_test_2()
{
    //int init_ipm_1[4*2] = {-1570, -8805, 5629, 6907, -5518, 7261, 6602, 6256};
    //int refl_pct_1[4*2] = {-8894, 126, 8876, 219, 2570, -8713, 7645, -4843};
    //int init_ipm_2[4*2] = {-1570, -8805, 5629, 6907, -5518, 7261, 6602, 6256};
    //int refl_pct_2[4*2] = {-8894, 126, 8876, 219, 2570, -8713, 7645, -4843};
    //int init_ipm_3[4*2] = {-1570, -8805, 5629, 6907, -5518, 7261, 6602, 6256};
    //int refl_pct_3[4*2] = {-8894, 126, 8876, 219, 2570, -8713, 7645, -4843};
    //int init_ipm_4[4*2] = {-1570, -8805, 5629, 6907, -5518, 7261, 6602, 6256};
    //int refl_pct_4[4*2] = {-8894, 126, 8876, 219, 2570, -8713, 7645, -4843};
    //int init_ipm_5[4*2] = {-1570, -8805, 5629, 6907, -5518, 7261, 6602, 6256};
    //int refl_pct_5[4*2] = {-8894, 126, 8876, 219, 2570, -8713, 7645, -4843};

    int init_ipm_1[4 * 2] = {-969, 1261, -379, 1544, 849, -1338, 705, 1405};
    int refl_pct_1[4 * 2] = {-1426, -842, -257, -1631, 1512, -647, -1034, -1280};
    int init_ipm_2[4 * 2] = {-920, 1284, -176, 1566, 961, 1244, 1564, 46};
    int refl_pct_2[4 * 2] = {479, -1568, 1163, -1149, -481, -1554, 1205, -1086};
    int init_ipm_3[4 * 2] = {-840, 1336, -1279, 920, -1508, -439, 1252, 939};
    int refl_pct_3[4 * 2] = {-274, 1614, 1630, 96, 1294, -984, -213, 1609};
    int init_ipm_4[4 * 2] = {1456, -543, 1270, 960, -1329, -875, 1586, 29};
    int refl_pct_4[4 * 2] = {1542, 468, 495, -1579, -1212, 1119, 1361, -923};
    int init_ipm_5[4 * 2] = {939, 1275, -1564, -248, 911, -1285, 660, -1433};
    int refl_pct_5[4 * 2] = {1526, -608, -1632, -143, 1511, 621, 1554, -489};

    int init_ipm_6[4 * 2]  = {-1570, -8805, 5629, 6907, -5518, 7261, 6602, 6256};
    int refl_pct_6[4 * 2]  = {-8894, 126, 8876, 219, 2570, -8713, 7645, -4843};
    int init_ipm_7[4 * 2]  = {-1570, -8805, 5629, 6907, -5518, 7261, 6602, 6256};
    int refl_pct_7[4 * 2]  = {-8894, 126, 8876, 219, 2570, -8713, 7645, -4843};
    int init_ipm_8[4 * 2]  = {-1570, -8805, 5629, 6907, -5518, 7261, 6602, 6256};
    int refl_pct_8[4 * 2]  = {-8894, 126, 8876, 219, 2570, -8713, 7645, -4843};
    int init_ipm_9[4 * 2]  = {-1570, -8805, 5629, 6907, -5518, 7261, 6602, 6256};
    int refl_pct_9[4 * 2]  = {-8894, 126, 8876, 219, 2570, -8713, 7645, -4843};
    int init_ipm_10[4 * 2] = {-1570, -8805, 5629, 6907, -5518, 7261, 6602, 6256};
    int refl_pct_10[4 * 2] = {-8894, 126, 8876, 219, 2570, -8713, 7645, -4843};

    int hadm_version = get_version();
    printf("[APP][HADM] hadm_version = %d.%d.%d\n", (((hadm_version) >> 16) & 0xFF), (((hadm_version) >> 8) & 0xFF), ((hadm_version) & 0xFF));
    printf("  distance_estimation_Phase_test_2\n");

    float   distanceReal = 5.74346;
    float   distPhaseDiff, distMusic, likeliness, EVDCap, T2WRDiffMean;
    int     nIterMaxEig, nIterPS, nSigCnt;
    complex ipmpct[40];
    complex tmp;
    //result of 0m, conj(I*Q)
    //int cali[40*2] = {1024,0,958,-361,816,-619,786,-656,940,-406,923,-443,1024,-14,824,-608,915,-460,914,-462,935,418,846,-578,797,642,1021,81,1022,65,1011,-160,825,-607,1024,-3,844,-579,1004,202,845,-578,881,522,995,-243,925,439,955,-370,400,943,1019,101,1015,133,998,229,909,471,908,474,1004,204,833,596,1002,-213,1003,209,971,324,965,343,982,290,931,-426,797,643};
    int cali[40 * 2] = {1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0, 1024, 0};

    //result of 4m, ipm is I from tesCollectData, pct is Q from  tesCollectData
    int ipm[4 * 2];
    int pct[4 * 2];

    int               channum = 4;
    float             fstep   = 2e6;
    parameterConstTes para    = tesInit(channum, fstep);

    foreach (i, 8) {
        ipm[i] = init_ipm_1[i];
        pct[i] = refl_pct_1[i];
    }
    calcIpmPct(ipm, pct, cali, ipmpct, para);
    int ipmpctInMatlab[CHANNUM * 2] = {30146020, 156227700, 134747226, 82848742, 158901426, 47021348, 144297142, -79154570, 136412536, -89915432, -13535568, -161800976, -99283440, -127645652, -112387520, -115841280, -107487742, 119707886, -159155040, 18077158, -94687256, 128115928, -75694470, 139750636, 82432276, 133051288, 97454646, 121003562, 117470224, -99082352, 151796866, -10346302, -57361344, -139867008, 77525964, -139860540, 44723074, -152277862, -77710826, -138187764, -88794588, -130610170, -156766690, 13409224, -148091580, 51592660, -82813008, 132376910, -63297420, 140742560, 67864104, 137345592, 91204224, 121381104, 147965572, 26492744, 149373404, 6463796, 71599684, -129945452, 67362982, -131287032, 45508654, -139998178, -66244548, -141454964, -96666790, -122452330, -155197776, -2658680, -147060824, 46174428, 54409798, 143409964, -49072630, 143491544, -28428712, 147295420, 110432130, 99028858};

    //phase based ranging
    float T2WR[40];
    distPhaseDiff = tesPhase(ipmpct, T2WR, &(T2WRDiffMean), para);
    distMusic     = tesMusic(ipmpct, T2WR, T2WRDiffMean, para, &(likeliness), &(nIterMaxEig), &(nIterPS), &(nSigCnt), &(EVDCap));

    printf("round 1\n");
    printf("  channum = %d\n", channum);
    int mark = 1;
    for (int i = 0; i < channum; i++) {
        if ((ipmpct[i].real != ipmpctInMatlab[2 * i]) || (ipmpct[i].imag != ipmpctInMatlab[2 * i + 1])) {
            printf("  %d,%d,%d,%d,%d\n", i, ipmpct[i].real, ipmpct[i].imag, ipmpctInMatlab[2 * i], ipmpctInMatlab[2 * i + 1]);
            mark = 0;
        }
    }
    if (mark) {
        printf("  calcIpmPct done!\n");
    }
    printf("  likeliness = %f\n", likeliness);
    printf("  realDistance = %f, phaseDiffDistance = %f, musicDistance = %f\n", distanceReal, distPhaseDiff, distMusic);
    printf("\n");
    printf("\n");

    foreach (i, 8) {
        ipm[i] = init_ipm_2[i];
        pct[i] = refl_pct_2[i];
    }
    calcIpmPct(ipm, pct, cali, ipmpct, para);
    //phase based ranging
    distPhaseDiff = tesPhase(ipmpct, T2WR, &(T2WRDiffMean), para);
    distMusic     = tesMusic(ipmpct, T2WR, T2WRDiffMean, para, &(likeliness), &(nIterMaxEig), &(nIterPS), &(nSigCnt), &(EVDCap));
    printf("round 2\n");
    printf("  channum = %d\n", channum);
    mark = 1;
    for (int i = 0; i < channum; i++) {
        if ((ipmpct[i].real != ipmpctInMatlab[2 * i]) || (ipmpct[i].imag != ipmpctInMatlab[2 * i + 1])) {
            printf("  %d,%d,%d,%d,%d\n", i, ipmpct[i].real, ipmpct[i].imag, ipmpctInMatlab[2 * i], ipmpctInMatlab[2 * i + 1]);
            mark = 0;
        }
    }
    if (mark) {
        printf("  calcIpmPct done!\n");
    }
    printf("  likeliness = %f\n", likeliness);
    printf("  realDistance = %f, phaseDiffDistance = %f, musicDistance = %f\n", distanceReal, distPhaseDiff, distMusic);
    printf("\n");
    printf("\n");

    foreach (i, 8) {
        ipm[i] = init_ipm_3[i];
        pct[i] = refl_pct_3[i];
    }
    calcIpmPct(ipm, pct, cali, ipmpct, para);
    //phase based ranging
    distPhaseDiff = tesPhase(ipmpct, T2WR, &(T2WRDiffMean), para);
    distMusic     = tesMusic(ipmpct, T2WR, T2WRDiffMean, para, &(likeliness), &(nIterMaxEig), &(nIterPS), &(nSigCnt), &(EVDCap));
    printf("round 3\n");
    printf("  channum = %d\n", channum);
    mark = 1;
    for (int i = 0; i < channum; i++) {
        if ((ipmpct[i].real != ipmpctInMatlab[2 * i]) || (ipmpct[i].imag != ipmpctInMatlab[2 * i + 1])) {
            printf("  %d,%d,%d,%d,%d\n", i, ipmpct[i].real, ipmpct[i].imag, ipmpctInMatlab[2 * i], ipmpctInMatlab[2 * i + 1]);
            mark = 0;
        }
    }
    if (mark) {
        printf("  calcIpmPct done!\n");
    }
    printf("  likeliness = %f\n", likeliness);
    printf("  realDistance = %f, phaseDiffDistance = %f, musicDistance = %f\n", distanceReal, distPhaseDiff, distMusic);
    printf("\n");
    printf("\n");

    foreach (i, 8) {
        ipm[i] = init_ipm_4[i];
        pct[i] = refl_pct_4[i];
    }
    calcIpmPct(ipm, pct, cali, ipmpct, para);
    //phase based ranging
    distPhaseDiff = tesPhase(ipmpct, T2WR, &(T2WRDiffMean), para);
    distMusic     = tesMusic(ipmpct, T2WR, T2WRDiffMean, para, &(likeliness), &(nIterMaxEig), &(nIterPS), &(nSigCnt), &(EVDCap));
    printf("round 4\n");
    printf("  channum = %d\n", channum);
    mark = 1;
    for (int i = 0; i < channum; i++) {
        if ((ipmpct[i].real != ipmpctInMatlab[2 * i]) || (ipmpct[i].imag != ipmpctInMatlab[2 * i + 1])) {
            printf("  %d,%d,%d,%d,%d\n", i, ipmpct[i].real, ipmpct[i].imag, ipmpctInMatlab[2 * i], ipmpctInMatlab[2 * i + 1]);
            mark = 0;
        }
    }
    if (mark) {
        printf("  calcIpmPct done!\n");
    }
    printf("  likeliness = %f\n", likeliness);
    printf("  realDistance = %f, phaseDiffDistance = %f, musicDistance = %f\n", distanceReal, distPhaseDiff, distMusic);
    printf("\n");
    printf("\n");

    foreach (i, 8) {
        ipm[i] = init_ipm_5[i];
        pct[i] = refl_pct_5[i];
    }
    calcIpmPct(ipm, pct, cali, ipmpct, para);
    //phase based ranging
    distPhaseDiff = tesPhase(ipmpct, T2WR, &(T2WRDiffMean), para);
    distMusic     = tesMusic(ipmpct, T2WR, T2WRDiffMean, para, &(likeliness), &(nIterMaxEig), &(nIterPS), &(nSigCnt), &(EVDCap));
    printf("round 5\n");
    printf("  channum = %d\n", channum);
    mark = 1;
    for (int i = 0; i < channum; i++) {
        if ((ipmpct[i].real != ipmpctInMatlab[2 * i]) || (ipmpct[i].imag != ipmpctInMatlab[2 * i + 1])) {
            printf("  %d,%d,%d,%d,%d\n", i, ipmpct[i].real, ipmpct[i].imag, ipmpctInMatlab[2 * i], ipmpctInMatlab[2 * i + 1]);
            mark = 0;
        }
    }
    if (mark) {
        printf("  calcIpmPct done!\n");
    }
    printf("  likeliness = %f\n", likeliness);
    printf("  realDistance = %f, phaseDiffDistance = %f, musicDistance = %f\n", distanceReal, distPhaseDiff, distMusic);
    printf("\n");
    printf("\n");

    foreach (i, 8) {
        ipm[i] = init_ipm_6[i];
        pct[i] = refl_pct_6[i];
    }
    calcIpmPct(ipm, pct, cali, ipmpct, para);
    //phase based ranging
    distPhaseDiff = tesPhase(ipmpct, T2WR, &(T2WRDiffMean), para);
    distMusic     = tesMusic(ipmpct, T2WR, T2WRDiffMean, para, &(likeliness), &(nIterMaxEig), &(nIterPS), &(nSigCnt), &(EVDCap));
    printf("round 6\n");
    printf("  channum = %d\n", channum);
    mark = 1;
    for (int i = 0; i < channum; i++) {
        if ((ipmpct[i].real != ipmpctInMatlab[2 * i]) || (ipmpct[i].imag != ipmpctInMatlab[2 * i + 1])) {
            printf("  %d,%d,%d,%d,%d\n", i, ipmpct[i].real, ipmpct[i].imag, ipmpctInMatlab[2 * i], ipmpctInMatlab[2 * i + 1]);
            mark = 0;
        }
    }
    if (mark) {
        printf("  calcIpmPct done!\n");
    }
    printf("  likeliness = %f\n", likeliness);
    printf("  realDistance = %f, phaseDiffDistance = %f, musicDistance = %f\n", distanceReal, distPhaseDiff, distMusic);
    printf("\n");
    printf("\n");

    foreach (i, 8) {
        ipm[i] = init_ipm_7[i];
        pct[i] = refl_pct_7[i];
    }
    calcIpmPct(ipm, pct, cali, ipmpct, para);
    //phase based ranging
    distPhaseDiff = tesPhase(ipmpct, T2WR, &(T2WRDiffMean), para);
    distMusic     = tesMusic(ipmpct, T2WR, T2WRDiffMean, para, &(likeliness), &(nIterMaxEig), &(nIterPS), &(nSigCnt), &(EVDCap));
    printf("round 7\n");
    printf("  channum = %d\n", channum);
    mark = 1;
    for (int i = 0; i < channum; i++) {
        if ((ipmpct[i].real != ipmpctInMatlab[2 * i]) || (ipmpct[i].imag != ipmpctInMatlab[2 * i + 1])) {
            printf("  %d,%d,%d,%d,%d\n", i, ipmpct[i].real, ipmpct[i].imag, ipmpctInMatlab[2 * i], ipmpctInMatlab[2 * i + 1]);
            mark = 0;
        }
    }
    if (mark) {
        printf("  calcIpmPct done!\n");
    }
    printf("  likeliness = %f\n", likeliness);
    printf("  realDistance = %f, phaseDiffDistance = %f, musicDistance = %f\n", distanceReal, distPhaseDiff, distMusic);
    printf("\n");
    printf("\n");

    foreach (i, 8) {
        ipm[i] = init_ipm_8[i];
        pct[i] = refl_pct_8[i];
    }
    calcIpmPct(ipm, pct, cali, ipmpct, para);
    //phase based ranging
    distPhaseDiff = tesPhase(ipmpct, T2WR, &(T2WRDiffMean), para);
    distMusic     = tesMusic(ipmpct, T2WR, T2WRDiffMean, para, &(likeliness), &(nIterMaxEig), &(nIterPS), &(nSigCnt), &(EVDCap));
    printf("round 8\n");
    printf("  channum = %d\n", channum);
    mark = 1;
    for (int i = 0; i < channum; i++) {
        if ((ipmpct[i].real != ipmpctInMatlab[2 * i]) || (ipmpct[i].imag != ipmpctInMatlab[2 * i + 1])) {
            printf("  %d,%d,%d,%d,%d\n", i, ipmpct[i].real, ipmpct[i].imag, ipmpctInMatlab[2 * i], ipmpctInMatlab[2 * i + 1]);
            mark = 0;
        }
    }
    if (mark) {
        printf("  calcIpmPct done!\n");
    }
    printf("  likeliness = %f\n", likeliness);
    printf("  realDistance = %f, phaseDiffDistance = %f, musicDistance = %f\n", distanceReal, distPhaseDiff, distMusic);
    printf("\n");
    printf("\n");

    foreach (i, 8) {
        ipm[i] = init_ipm_9[i];
        pct[i] = refl_pct_9[i];
    }
    calcIpmPct(ipm, pct, cali, ipmpct, para);
    //phase based ranging
    distPhaseDiff = tesPhase(ipmpct, T2WR, &(T2WRDiffMean), para);
    distMusic     = tesMusic(ipmpct, T2WR, T2WRDiffMean, para, &(likeliness), &(nIterMaxEig), &(nIterPS), &(nSigCnt), &(EVDCap));
    printf("round 9\n");
    printf("  channum = %d\n", channum);
    mark = 1;
    for (int i = 0; i < channum; i++) {
        if ((ipmpct[i].real != ipmpctInMatlab[2 * i]) || (ipmpct[i].imag != ipmpctInMatlab[2 * i + 1])) {
            printf("  %d,%d,%d,%d,%d\n", i, ipmpct[i].real, ipmpct[i].imag, ipmpctInMatlab[2 * i], ipmpctInMatlab[2 * i + 1]);
            mark = 0;
        }
    }
    if (mark) {
        printf("  calcIpmPct done!\n");
    }
    printf("  likeliness = %f\n", likeliness);
    printf("  realDistance = %f, phaseDiffDistance = %f, musicDistance = %f\n", distanceReal, distPhaseDiff, distMusic);
    printf("\n");
    printf("\n");

    foreach (i, 8) {
        ipm[i] = init_ipm_10[i];
        pct[i] = refl_pct_10[i];
    }
    calcIpmPct(ipm, pct, cali, ipmpct, para);
    //phase based ranging
    distPhaseDiff = tesPhase(ipmpct, T2WR, &(T2WRDiffMean), para);
    distMusic     = tesMusic(ipmpct, T2WR, T2WRDiffMean, para, &(likeliness), &(nIterMaxEig), &(nIterPS), &(nSigCnt), &(EVDCap));
    printf("round 10\n");
    printf("  channum = %d\n", channum);
    mark = 1;
    for (int i = 0; i < channum; i++) {
        if ((ipmpct[i].real != ipmpctInMatlab[2 * i]) || (ipmpct[i].imag != ipmpctInMatlab[2 * i + 1])) {
            printf("  %d,%d,%d,%d,%d\n", i, ipmpct[i].real, ipmpct[i].imag, ipmpctInMatlab[2 * i], ipmpctInMatlab[2 * i + 1]);
            mark = 0;
        }
    }
    if (mark) {
        printf("  calcIpmPct done!\n");
    }
    printf("  likeliness = %f\n", likeliness);
    printf("  realDistance = %f, phaseDiffDistance = %f, musicDistance = %f\n", distanceReal, distPhaseDiff, distMusic);
    printf("\n");
    printf("\n");

    /*
     * @ Print Log
        [APP][HADM] hadm_version = 1.1.27
     */
}

#endif
