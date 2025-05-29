/********************************************************************************************************
 * @file    sihui_vcd_def.h
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
#ifndef STACK_BLE_DEBUG_SIHUI_VCD_DEF_H_
#define STACK_BLE_DEBUG_SIHUI_VCD_DEF_H_


#if (DBG_SLAVE_CONN_UPDATE)
    #define SLET_upt_cmd_1  10
    #define SLET_upt_cmd_2  11
    #define SLET_upt_cmd_3  12
    #define SLET_upt_cmd_4  13
    #define SLET_upt_sync_1 20
    #define SLET_upt_sync_2 21
    #define SLET_upt_sync_3 22
    #define SLET_upt_sync_4 23
#endif

#if 0
    #define SLET_05_rx_crc     5
    #define SLET_06_rx_1st     6
    #define SLET_07_rx_new     7
    #define SLET_10_tx         10
    #define SLET_11_c_cmdDone  11
    #define SLET_12_c_1stRxTmt 12
    #define SLET_13_c_rxTmt    13
    #define SLET_14_c_rxCrc2   14
#endif


#if 0
    #define SLEV_txFifo_push  17
    #define SLEV_txFifo_empty 18
    #define SLEV_txFifo_RX    19
    #define SLEV_txFifo_post  20
    #define SLEV_test_event   31

    #define SL16_tf_hw_push   1
    #define SL16_tf_sw_push   2
    #define SL16_tf_hw_load1  3
    #define SL16_tf_sw_load1  4
    #define SL16_tf_hw_load2  5
    #define SL16_tf_sw_load2  6
    #define SL16_tf_hw_RX     7
    #define SL16_tf_sw_RX     8
    #define SL16_tf_hw_TX     9
    #define SL16_tf_sw_TX     10
    #define SL16_tf_hw_post   11
    #define SL16_tf_sw_post   12
    #define SL16_tf_save      13

    #define SL16_seq_notify   15
    #define SL16_seq_write    16
    #define SL16_test_2B      17
#endif


#endif /* STACK_BLE_DEBUG_SIHUI_VCD_DEF_H_ */
